param([string]$Build = 'build-soak-s31', [int]$Slot = 12, [int]$Seconds = 400, [string[]]$Only = @(), [string]$Out = '',
      [string]$Image = '', [int]$ItemRate = 5,
      # Wait for a campaign run on $PairSlot to be 45-200 s old before each match
      # starts, so neither run's start lands in the other's DLDI init dip
      # (memory runner-slots-and-extras-limits); -PairSlot 0 starts at once.
      [int]$PairSlot = 11, [string]$PairGlob = 'au-*.log')
# Stability audit: four-CPU matches on the soak ROM (build_soak_rom.ps1: the
# gate ROM with NDS_R2_SOAK_MATCH_MINUTES), every item at the very high rate
# (-ItemRate 5), every fighter once, a stage a match (m1-m8). Per scene: heap
# low-water, presented frames, native failures, lean declines, event-script
# refusals; halts (DABT/OOM) and the first native failure. One line a match
# in $Out (default builds/probes/audit-soak.txt).
. "$PSScriptRoot\common.ps1"
if ($Out -eq '') { $Out = Join-Path $S64Probes 'audit-soak.txt' }
if ($Image -eq '') { $Image = Join-Path $S64Repo "artifacts\sd\soak-slot$Slot.img" }
function SlotKinds($s) {
    if ($s -like 'c*') { $c = [int]$s.Substring(1); return @($S64ContentParent[$c], $c) }
    return @([int]$s, 0)
}
$matchList = @(
  @('m1', 0, @('0', '1', '2', '3')), @('m2', 1, @('4', '5', '6', '7')), @('m3', 2, @('8', '9', '10', '11')),
  @('m4', 3, @('c1', 'c2', 'c3', 'c4')), @('m5', 4, @('c5', 'c6', 'c7', 'c8')), @('m6', 5, @('c9', 'c10', 'c11', 'c13')),
  @('m7', 7, @('c14', 'c15', 'c16', 'c17')), @('m8', 8, @('c18', 'c19', 'c20', '0')))
$pairPath = "*melonds-runners\slot$PairSlot\*"
$first = $true
foreach ($m in $matchList) {
    if ($Only.Count -gt 0 -and ($Only -notcontains $m[0])) { continue }
    if ($PairSlot -gt 0) {
        while ($true) {
            $busy = @(Get-Process melonDS -ErrorAction SilentlyContinue | Where-Object { $_.Path -like $pairPath })
            if ($busy.Count -eq 0) {
                Start-Sleep -Seconds 30
                if (@(Get-Process melonDS -ErrorAction SilentlyContinue | Where-Object { $_.Path -like $pairPath }).Count -eq 0) { break }
                continue
            }
            $newest = Get-ChildItem -Path $S64Probes -Filter $PairGlob | Sort-Object CreationTime -Descending | Select-Object -First 1
            $age = if ($newest) { ((Get-Date) - $newest.CreationTime).TotalSeconds } else { 9999 }
            if ($age -ge 45 -and $age -le 200) { break }
            Start-Sleep -Seconds 10
        }
    }
    $kw = [uint32]0; $cw = [uint32]0
    for ($i = 0; $i -lt 4; $i++) {
        $kc = SlotKinds $m[2][$i]
        $kw = $kw -bor ([uint32]$kc[0] -shl (8 * $i)); $cw = $cw -bor ([uint32]$kc[1] -shl (8 * $i))
    }
    $tag = "soak-$($m[0])"
    $pokes = @("set var gNdsLabFourCpuGkind = $($m[1])", "set var gNdsLabFourCpuKinds = $kw",
               "set var gNdsLabP4Content = $cw", "set var gNdsLabItemRate = $ItemRate")
    $lines = @(
      'break ndsSceneManagerEnter', 'commands', 'silent',
      'printf "X enter scene=%u prevHeapLow=%u frames=%u native=%u lean=%u ev32fail=%u\n", gSCManagerSceneData.scene_curr, gNdsTaskmanGeneralHeapFreeMin, gNdsBattlePlayablePacingPresentedFrames, gNdsRendererNativeFailure.count, gNdsFtrLeanDeclineCount, gNdsAObjEvent32NormalizeFailCount',
      'set var gNdsTaskmanGeneralHeapFreeMin = 0xffffffff',
      'continue', 'end',
      'break __arm_excpt_dabt', 'commands', 'printf "X DABT lr-8=%x\n", $lr-8', 'bt 8', 'end',
      'break ndsSyMallocOverflowHalt', 'commands', 'printf "X OOM req=%u head=%u\n", gNdsSyMallocOverflowRequest, gNdsSyMallocOverflowHeadroom', 'bt 12', 'end',
      'break ndsRendererRecordNativeFailure', 'commands', 'silent',
      'printf "X FAIL dom=%u scene=%u id=%x st=%u root=%u mat=%x reason=%u\n", domain, scene, identity, status, root, material, reason',
      'bt 10', 'disable $bpnum', 'continue', 'end')
    & "$PSScriptRoot\probe_session.ps1" -Build $Build -Rom 'smash64ds-p2-fourcpu-tickhud-hwtri' -Slot $Slot -Tag $tag `
        -Seconds $Seconds -Every 60 -Pokes $pokes -Lines $lines -Image $Image -Fresh:$first | Out-Null
    $first = $false
    $log = Join-Path $S64Probes "$tag.log"
    $L = if (Test-Path $log) { Get-Content $log } else { @() }
    $enters = @($L | Select-String '^X enter ' | ForEach-Object { $_.Line -replace '^X enter ', '' })
    $bad = @($L | Select-String '^X (OOM|DABT|FAIL)' | Select-Object -First 3 | ForEach-Object { $_.Line })
    "$($m[0]) gkind=$($m[1]) roster=$($m[2] -join ',') enters=[$($enters -join ' ; ')] bad=$($bad -join ' | ')" |
        Add-Content -LiteralPath $Out
}
"DONE" | Add-Content -LiteralPath $Out
