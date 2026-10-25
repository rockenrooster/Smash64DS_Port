param([string]$Build = 'walk-u26', [int]$Slot = 9, [int]$Seconds = 600, [string]$Tag = 'ledger',
      [string]$Rom = 'smash64ds', [string[]]$Pokes = @(), [switch]$Frames)
# P8 S4, the load ledger: on a walk ROM (its menus drive themselves), each
# scene change -- the scene it leaves and the one it enters, and the VBlanks
# from the old scene's exit hold (ndsPlatformTransitionHoldBegin stamps
# gNdsTransitionHoldVBlank) to the new scene's first present
# (ndsPlatformTransitionThaw). The thaw's inlined copies all carry the
# breakpoint, so a hold is counted once, at its first thaw. -Frames also
# stops at every ndsPlatformEndFrame (slow): VBlanks between frames per
# scene, the menus' spikes being the frames past two VBlanks.
# Record: builds/probes/<Tag>.log ("LOAD", "FRAME" lines); summary:
# builds/probes/<Tag>-ledger.txt.
. "$PSScriptRoot\common.ps1"
$ErrorActionPreference = 'Stop'
$lines = @('break ndsPlatformTransitionThaw', 'commands', 'silent',
           'printf "LOAD hold=%u prev=%u curr=%u from=%u at=%u\n", sNdsTransitionHold, gSCManagerSceneData.scene_prev, gSCManagerSceneData.scene_curr, gNdsTransitionHoldVBlank, sVBlankCount',
           'continue', 'end')
if ($Frames) {
    $lines += @('break ndsPlatformEndFrame', 'commands', 'silent',
                'printf "FRAME scene=%u at=%u\n", gSCManagerSceneData.scene_curr, sVBlankCount',
                'continue', 'end')
}
& "$PSScriptRoot\probe_session.ps1" -Build $Build -Rom $Rom -Slot $Slot -Tag $Tag -Seconds $Seconds `
    -Pokes $Pokes -Lines $lines | Out-Null
$log = Join-Path $S64Probes "$Tag.log"
$first = @{}
$gaps = @{}
$last = $null
foreach ($line in (Get-Content $log)) {
    if ($line -match '^LOAD hold=(\d+) prev=(\d+) curr=(\d+) from=(\d+) at=(\d+)') {
        if ([int]$Matches[1] -eq 0) { continue }   # no hold: the thaw returned at once
        $from = [int64]$Matches[4]
        $at = [int64]$Matches[5]
        if ((-not $first.ContainsKey($from)) -or ($at -lt $first[$from].At)) {
            $first[$from] = [pscustomobject]@{ Key = "$($Matches[2])->$($Matches[3])"; At = $at }
        }
    } elseif ($line -match '^FRAME scene=(\d+) at=(\d+)') {
        $scene = [int]$Matches[1]
        $at = [int64]$Matches[2]
        if (($null -ne $last) -and ($last.Scene -eq $scene)) {
            if (-not $gaps.ContainsKey($scene)) { $gaps[$scene] = New-Object System.Collections.Generic.List[int64] }
            $gaps[$scene].Add($at - $last.At)
        }
        $last = [pscustomobject]@{ Scene = $scene; At = $at }
    }
}
$loads = @{}
foreach ($f in $first.GetEnumerator()) {
    if (-not $loads.ContainsKey($f.Value.Key)) { $loads[$f.Value.Key] = New-Object System.Collections.Generic.List[int64] }
    $loads[$f.Value.Key].Add($f.Value.At - $f.Key)
}
$ms = 1000.0 / 59.8261
$out = @("load ledger $Build ($Seconds s, slot $Slot): scene prev->curr, holds, median and longest VBlanks (ms)")
foreach ($r in ($loads.GetEnumerator() | Sort-Object { ($_.Value | Measure-Object -Maximum).Maximum } -Descending)) {
    $v = $r.Value | Sort-Object
    $med = $v[[int][math]::Floor(($v.Count - 1) / 2)]
    $out += ('{0,-8} n={1,-3} median={2,4} VB ({3,5:N0} ms)  max={4,4} VB ({5,5:N0} ms)' -f $r.Key, $v.Count, $med,
             ($med * $ms), $v[-1], ($v[-1] * $ms))
}
if ($Frames) {
    $out += 'frames per scene: count, frames of 1 / 2 / 3+ VBlanks, longest'
    foreach ($r in ($gaps.GetEnumerator() | Sort-Object Name)) {
        $v = $r.Value
        $one = ($v | Where-Object { $_ -le 1 }).Count
        $two = ($v | Where-Object { $_ -eq 2 }).Count
        $more = ($v | Where-Object { $_ -ge 3 }).Count
        $out += ('scene {0,-3} n={1,-6} 1VB={2,-6} 2VB={3,-6} 3+VB={4,-5} max={5} VB' -f $r.Name, $v.Count, $one, $two,
                 $more, (($v | Measure-Object -Maximum).Maximum))
    }
}
$summary = Join-Path $S64Probes "$Tag-ledger.txt"
$out | Set-Content $summary
$out
