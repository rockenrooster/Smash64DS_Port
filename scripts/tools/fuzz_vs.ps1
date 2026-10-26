param([string]$Build = 'walk-u26', [int]$Slot = 9, [int]$Seed = 1, [int]$Laps = 10, [int]$Seconds = 0,
      [string]$Tag = '', [string]$Out = '')
# P8 S2 seeded fuzz on a walk ROM (build_walk_rom.ps1): the walk's VS route
# (character select, stage select, match, Results, back) for $Laps laps, each
# match drawn from $Seed: four CPU fighters (an original kind or a P4 content,
# a level 1-9, mirrors on distinct costumes), the stage (one of the sixteen VS
# stages), the rules (time 1-3 minutes, or 1-2 stocks), items (a rate and a
# subset of the walk's switches) and sometimes teams. No lab code: the draw is
# made here, and gdb writes lap k's descriptor into gNdsMatchConfig at every
# ndsMatchConfigApply (the character select's and stage select's own commits)
# and the stage again at scVSBattleStartScene, where the lap counter moves on.
# Writes are whole words (byte writes kill melonDS's stub) with X packets off.
# Record: builds/probes/<Tag>.log -- X LAP (the lap's draw), X enter (scene,
# heap low-water, native failures), X FAIL / DABT / OOM; a DONE line with the
# seed in $Out (default builds/probes/audit-fuzz.txt). Rerun one seed to
# reproduce a failure.
. "$PSScriptRoot\common.ps1"
if ($Tag -eq '') { $Tag = "fuzz-s$Seed" }
if ($Out -eq '') { $Out = Join-Path $S64Probes 'audit-fuzz.txt' }
if ($Seconds -eq 0) { $Seconds = 120 + 300 * $Laps }
$rng = [System.Random]::new($Seed)
$stages = @(0, 1, 2, 3, 4, 5, 6, 7, 8, 13, 14, 16, 41, 42, 43, 44)
$contents = @($S64ContentParent.Keys | Sort-Object)
$defines = @()
$plan = @()
for ($lap = 0; $lap -lt $Laps; $lap++) {
    $team = ($rng.Next(4) -eq 0)
    $slots = @()
    for ($i = 0; $i -lt 4; $i++) {
        if ($rng.Next(2) -eq 0) { $c = 0; $k = $rng.Next(12) }
        else { $c = $contents[$rng.Next($contents.Count)]; $k = $S64ContentParent[$c] }
        $costume = @($slots | Where-Object { ($_.K -eq $k) -and ($_.C -eq $c) }).Count
        # Teams: slots 0 and 1 on red and blue, so a team match always has two;
        # slots 2 and 3 on red, blue or green.
        $t = $(if (-not $team) { $i } elseif ($i -lt 2) { $i } else { $rng.Next(3) })
        $slots += [pscustomobject]@{ K = $k; C = $c; L = 1 + $rng.Next(9); Costume = $costume; Team = $t }
    }
    $g = $stages[$rng.Next($stages.Count)]
    if ($rng.Next(2) -eq 0) { $rules = 1; $time = 1 + $rng.Next(3); $stocks = 1 }
    else { $rules = 2; $time = 100; $stocks = 1 + $rng.Next(2) }
    $rate = $rng.Next(6)
    $mask = [uint32]$rng.Next(0x10000) -bor ([uint32]$rng.Next(0x10000) -shl 16)
    $lines = @("define fuzzlap$lap")
    for ($i = 0; $i -lt 4; $i++) {
        $s = $slots[$i]
        $f = "gNdsMatchConfig.fighters[$i]"
        $lines += "set var *(unsigned int *)&$f.fkind = (*(unsigned int *)&$f.fkind & 0xff000000) | $($s.L * 0x10000 + 1 * 0x100 + $s.K)"
        $lines += "set var *(unsigned int *)&$f.team = (*(unsigned int *)&$f.team & 0xffff0000) | $($s.Costume * 0x100 + $s.Team)"
        $lines += "set var *(unsigned int *)&$f.stock_count = (*(unsigned int *)&$f.stock_count & 0xff00ffff) | $($s.C * 0x10000)"
    }
    $lines += "set var *(unsigned int *)&gNdsMatchConfig.gkind = (*(unsigned int *)&gNdsMatchConfig.gkind & 0xffff0000) | $($rules * 0x100 + $g)"
    $lines += "set var *(unsigned int *)&gNdsMatchConfig.time_limit = (*(unsigned int *)&gNdsMatchConfig.time_limit & 0x00ff0000) | $([uint32]($rate * 0x1000000 + $stocks * 0x100 + $time))"
    $lines += "set var *(unsigned int *)&gNdsMatchConfig.damage_ratio = (*(unsigned int *)&gNdsMatchConfig.damage_ratio & 0xffff00ff) | $([int]$team * 0x100)"
    $lines += "set var gNdsMatchConfig.item_toggles = gNdsMatchConfig.item_toggles & $mask"
    $lines += 'end'
    $lines += "define fuzzstage$lap"
    $lines += "set var `$fz_a = (unsigned int)&gSCManagerSceneData.gkind"
    $lines += "set var `$fz_s = (`$fz_a & 3) * 8"
    $lines += "set var *(unsigned int *)(`$fz_a & ~3) = (*(unsigned int *)(`$fz_a & ~3) & ~(0xff << `$fz_s)) | ($g << `$fz_s)"
    $lines += "printf `"X LAP $lap seed=$Seed gk=$g rules=$rules time=$time stocks=$stocks items=$rate/0x$('{0:x8}' -f $mask) team=$([int]$team) fk=$(($slots | ForEach-Object { "$($_.K)" }) -join ',') c=$(($slots | ForEach-Object { "$($_.C)" }) -join ',') lv=$(($slots | ForEach-Object { "$($_.L)" }) -join ',')\n`""
    $lines += 'end'
    $defines += $lines
    $plan += "lap $lap gk=$g rules=$rules time=$time stocks=$stocks items=$rate team=$([int]$team) " +
        (($slots | ForEach-Object { "$($_.K)/$($_.C)/L$($_.L)" }) -join ' ')
}
$lines = $defines + @(
  'set $fz_lap = 0',
  # The descriptor's layout this tool writes by word (include/nds/nds_match_config.h).
  'printf "X OFS team=%u stock=%u size=%u gkind=%u time=%u damage=%u toggles=%u\n", (unsigned int)&gNdsMatchConfig.fighters[0].team - (unsigned int)&gNdsMatchConfig, (unsigned int)&gNdsMatchConfig.fighters[0].stock_count - (unsigned int)&gNdsMatchConfig, (unsigned int)&gNdsMatchConfig.fighters[1] - (unsigned int)&gNdsMatchConfig, (unsigned int)&gNdsMatchConfig.gkind - (unsigned int)&gNdsMatchConfig, (unsigned int)&gNdsMatchConfig.time_limit - (unsigned int)&gNdsMatchConfig, (unsigned int)&gNdsMatchConfig.damage_ratio - (unsigned int)&gNdsMatchConfig, (unsigned int)&gNdsMatchConfig.item_toggles - (unsigned int)&gNdsMatchConfig',
  'break ndsMatchConfigApply', 'commands', 'silent',
  "if `$fz_lap < $Laps", 'eval "fuzzlap%d", $fz_lap', 'end',
  'continue', 'end',
  'break scVSBattleStartScene', 'commands', 'silent',
  "if `$fz_lap < $Laps", 'eval "fuzzstage%d", $fz_lap', 'end',
  'set $fz_lap = $fz_lap + 1',
  'continue', 'end',
  'break ndsSceneManagerEnter', 'commands', 'silent',
  'printf "X enter scene=%u prev=%u prevHeapLow=%u native=%u lean=%u gkind=%u fk=%u,%u,%u,%u c=%u,%u,%u,%u laps=%u ev32=%u\n", gSCManagerSceneData.scene_curr, gSCManagerSceneData.scene_prev, gNdsTaskmanGeneralHeapFreeMin, gNdsRendererNativeFailure.count, gNdsFtrLeanDeclineCount, gSCManagerSceneData.gkind, gSCManagerTransferBattleState.players[0].fkind, gSCManagerTransferBattleState.players[1].fkind, gSCManagerTransferBattleState.players[2].fkind, gSCManagerTransferBattleState.players[3].fkind, gNdsP4PlayerContent[0], gNdsP4PlayerContent[1], gNdsP4PlayerContent[2], gNdsP4PlayerContent[3], gNdsMenuShellWalkLoops, gNdsAObjEvent32NormalizeFailCount',
  'set var gNdsTaskmanGeneralHeapFreeMin = 0xffffffff',
  'continue', 'end',
  'break __arm_excpt_dabt', 'commands', 'printf "X DABT lr-8=%x lap=%u\n", $lr-8, $fz_lap', 'bt 8', 'end',
  'break ndsSyMallocOverflowHalt', 'commands', 'printf "X OOM req=%u head=%u lap=%u\n", gNdsSyMallocOverflowRequest, gNdsSyMallocOverflowHeadroom, $fz_lap', 'bt 12', 'end',
  'break ndsRendererRecordNativeFailure', 'commands', 'silent',
  'printf "X FAIL dom=%u scene=%u id=%x st=%u root=%u mat=%x reason=%u lap=%u\n", domain, scene, identity, status, root, material, reason, $fz_lap',
  'bt 10', 'continue', 'end')
$planPath = Join-Path $S64Probes "$Tag-plan.txt"
(@("fuzz seed=$Seed laps=$Laps build=$Build") + $plan) | Set-Content $planPath
& "$PSScriptRoot\probe_session.ps1" -Build $Build -Slot $Slot -Tag $Tag -Seconds $Seconds -Every 300 `
    -Pokes @('set remote X-packet off', "set var gNdsMenuShellWalkBudget = $Laps") -Lines $lines | Out-Null
$log = Join-Path $S64Probes "$Tag.log"
$laps = @(Select-String -Path $log -Pattern '^X LAP ').Count
$bad = @(Select-String -Path $log -Pattern '^X (FAIL|DABT|OOM)').Count
"FUZZ $Tag seed=$Seed laps=$laps/$Laps failures=$bad DONE" | Add-Content -LiteralPath $Out
"FUZZ $Tag seed=$Seed laps=$laps/$Laps failures=$bad (plan $planPath, log $log)"
