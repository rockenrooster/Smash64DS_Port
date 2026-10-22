param([string]$Build = 'build-sweep-s61', [int]$Gkind = 6, [int[]]$Contents = @(), [string]$Tag = 'sweep',
      [string]$Rosters = '', [int]$Slot = 11, [string]$Wait = '', [string]$WaitFile = '',
      [string[]]$Boot = @(), [int[]]$Gkinds = @(), [string[]]$MoreGlobals = @(),
      [string]$ArtDir = 'artifacts\performance\p8-sweep')
# Four-CPU WORK P50/P95 on a gate ROM (build_gate_rom.ps1): four mirrors of
# each P4 content (-Contents, 0 = the cast, the preset original roster) or
# -Rosters "k0,k1,k2,k3/c0,c1,c2,c3;..." (fighter kind and content per slot),
# on stage -Gkind or each of -Gkinds. Ring dump of 1,960 samples from frame 2,
# one run at a time on -Slot. Per run: <ArtDir>/<Tag>-<run>-g<gkind>.{csv,json,txt}
# (the CSV is the replay digest: scripts/compare-replay-digest.py); one line a
# run in builds/probes/sweep-<Tag>.out.
. "$PSScriptRoot\common.ps1"
$art = Join-Path $S64Repo $ArtDir
New-Item -ItemType Directory -Force $art | Out-Null
$out = Join-Path $S64Probes "sweep-$Tag.out"
if ($WaitFile -ne '') {
    while (-not (Select-String -Path $WaitFile -Pattern ([regex]::Escape($Wait)) -Quiet -ErrorAction SilentlyContinue)) {
        Start-Sleep -Seconds 20
    }
}
$runs = @()
foreach ($c in $Contents) {
    if ($c -eq 0) { $runs += @{ Name = 'orig'; Kinds = '4294967295'; Content = '0' } }
    else {
        $k = $S64ContentParent[$c]
        $runs += @{ Name = "c$c"; Kinds = ([string]([uint32]($k * 0x01010101))); Content = ([string]([uint32]($c * 0x01010101))) }
    }
}
if ($Rosters -ne '') {
    foreach ($r in $Rosters.Split(';')) {
        $kc = $r.Split('/'); $ks = $kc[0].Split(',') | ForEach-Object { [int]$_ }; $cs = $kc[1].Split(',') | ForEach-Object { [int]$_ }
        $kw = 0; $cw = 0
        for ($i = 0; $i -lt 4; $i++) { $kw = $kw -bor ($ks[$i] -shl (8 * $i)); $cw = $cw -bor ($cs[$i] -shl (8 * $i)) }
        $runs += @{ Name = ('r' + ($cs -join '-')); Kinds = ([string]([uint32]$kw)); Content = ([string]([uint32]$cw)) }
    }
}
$extras = @('gNdsTaskmanGeneralHeapFreeMin', 'gNdsNativeFailureLabCount', 'gNdsFtrLeanDeclineCount', 'gNdsP2StageProgLoads',
    'gNdsP2StageProgDeclines', 'gNdsR2AnimCacheMisses', 'gNdsR2AnimCacheHits', 'gNdsAudioStorageRequests',
    'gNdsR2AnimCacheArenaReservedBytes', 'gNdsAObjEvent32NormalizeScriptCount', 'gNdsAObjEvent32NormalizedHighWater',
    'gNdsAObjEvent32CapacityLimit', 'gNdsAObjEvent32NormalizeFailCount', 'gNdsP2StageProgSegments', 'gNdsP2StageProgBytes',
    'gNdsP2StageProgReason', 'gNdsNativeStageLentLayerSkips', 'gNdsBattleDeadSpanServedBytes', 'gNdsBattleDeadSpanLentBytes') + $MoreGlobals
$stages = if ($Gkinds.Count -gt 0) { $Gkinds } else { @($Gkind) }
foreach ($g in $stages) {
    foreach ($r in $runs) {
        $name = "$Tag-$($r.Name)-g$g"
        & (Join-Path $S64Repo 'scripts\sample-tick-hud-buckets.ps1') -Target smash64ds-p2-fourcpu-tickhud-hwtri -Build $Build -NoBuild -RingDump `
            -Samples 1960 -StartFrame 2 -RunnerSlot $Slot -RowsCsv "$art\$name.csv" -JsonOut "$art\$name.json" `
            -BootSetGlobals (@("gNdsLabFourCpuGkind=$g", "gNdsLabFourCpuKinds=$($r.Kinds)", "gNdsLabP4Content=$($r.Content)") + $Boot) `
            -ExtraGlobals $extras *> "$art\$name.txt"
        $line = "$name :: "
        if (Test-Path "$art\$name.json") {
            $j = Get-Content "$art\$name.json" -Raw | ConvertFrom-Json
            $w = $j.buckets | Where-Object { $_.bucket -eq 'WORK' }
            $s = $j.buckets | Where-Object { $_.bucket -eq 'SRC' }
            $ex = ($j.extras | ForEach-Object { "$($_.name -replace '^gNds','')=$($_.value)" }) -join ' '
            $line += "WORK p50=$($w.p50) p95=$($w.p95) max=$($w.max) SRC p95=$($s.p95) vbi2=$($j.vbi2) vbi3=$($j.vbi3) vbi4=$($j.vbi4) vbi5+=$($j.vbi5plus) total=$($j.vbiTotal) $ex"
        } else { $line += 'NO JSON: ' + ((Get-Content "$art\$name.txt" -Tail 3) -join ' / ') }
        $line | Add-Content $out
    }
    "SWEEP DONE g$g" | Add-Content $out
}
