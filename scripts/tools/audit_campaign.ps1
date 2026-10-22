param([string]$Build = 'walk-u16', [int]$Seconds = 480, [int]$Slot = 11, [string[]]$Only = @(), [string]$Out = '')
# Stability audit: the 1P campaign for every fighter (the twelve as v<name>,
# Metal Mario as vmmario, the nineteen contents as c<id>), one walk run each on
# slot $Slot: scenes reached, heap low-water per scene, native failures,
# OOM/abort halts. One line a run in $Out (default
# builds/probes/audit-campaign.txt); -Only picks runs by name.
. "$PSScriptRoot\common.ps1"
if ($Out -eq '') { $Out = Join-Path $S64Probes 'audit-campaign.txt' }
$runs = @()
$names = @('luigi', 'mario', 'dk', 'link', 'samus', 'captain', 'ness', 'yoshi', 'kirby', 'fox', 'pikachu', 'purin')
for ($i = 0; $i -lt 12; $i++) { $runs += , @("v$($names[$i])", 0, $i) }
$runs += , @('vmmario', 0, 12)
foreach ($c in @(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 14, 15, 16, 17, 18, 19, 20)) { $runs += , @("c$c", $c, 255) }
foreach ($r in $runs) {
    if ($Only.Count -gt 0 -and ($Only -notcontains $r[0])) { continue }
    $tag = "au-$($r[0])"
    & "$PSScriptRoot\probe_campaign.ps1" -Build $Build -Tag $tag -Content $r[1] -Portrait $r[2] -Seconds $Seconds -Slot $Slot -FailBt | Out-Null
    $log = Join-Path $S64Probes "$tag.log"
    $lines = if (Test-Path $log) { Get-Content $log } else { @() }
    $enter = @($lines | Select-String '^X enter scene=(\d+)' | ForEach-Object { [int]$_.Matches[0].Groups[1].Value })
    $heap = @($lines | Select-String '^X enter scene=\d+ stage=\d+ prevHeapLow=(\d+)' |
        ForEach-Object { [long]$_.Matches[0].Groups[1].Value } | Where-Object { $_ -lt 4000000000 })
    $native = @($lines | Select-String 'native=(\d+)' | ForEach-Object { [int]$_.Matches[0].Groups[1].Value })
    $bad = @($lines | Select-String '^X (OOM|DABT|FAIL)' | Select-Object -First 3 | ForEach-Object { $_.Line })
    $minHeap = if ($heap.Count) { ($heap | Measure-Object -Minimum).Minimum } else { -1 }
    $maxNative = if ($native.Count) { ($native | Measure-Object -Maximum).Maximum } else { -1 }
    $stages = @($lines | Select-String '^X enter scene=14 stage=(\d+)' | ForEach-Object { [int]$_.Matches[0].Groups[1].Value })
    $lastStage = if ($stages.Count) { $stages[-1] } else { -1 }
    $last = if ($enter.Count) { $enter[-1] } else { -1 }
    "$($r[0]) content=$($r[1]) portrait=$($r[2]) scenes=$($enter.Count) last=$last stage_last=$lastStage heap_min=$minHeap native_max=$maxNative bad=$($bad -join ' | ')" |
        Add-Content -LiteralPath $Out
}
"DONE" | Add-Content -LiteralPath $Out
