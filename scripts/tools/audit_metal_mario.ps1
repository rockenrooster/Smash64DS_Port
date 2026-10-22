param([string]$Build = 'walk-u16', [int]$Slot = 8, [string]$Out = '')
# Stability audit for Metal Mario (walk portrait 13, his kind): the 1P
# campaign, then both bonus practices on a fresh SD image. Lines in $Out
# (default builds/probes/audit-metal-mario.txt).
. "$PSScriptRoot\common.ps1"
if ($Out -eq '') { $Out = Join-Path $S64Probes 'audit-metal-mario.txt' }
& "$PSScriptRoot\probe_campaign.ps1" -Build $Build -Tag 'au-mm13' -Content 0 -Portrait 13 -Seconds 600 -Slot $Slot -FailBt | Out-Null
$L = Get-Content (Join-Path $S64Probes 'au-mm13.log')
$heap = @($L | Select-String '^X enter scene=\d+ stage=\d+ prevHeapLow=(\d+)' |
    ForEach-Object { [long]$_.Matches[0].Groups[1].Value } | Where-Object { $_ -lt 4000000000 })
$stages = @($L | Select-String '^X enter scene=14 stage=(\d+)' | ForEach-Object { [int]$_.Matches[0].Groups[1].Value })
$commit = @($L | Select-String '^X commit' | Select-Object -First 1 | ForEach-Object { $_.Line })
$bad = @($L | Select-String '^X (OOM|DABT|FAIL)' | Select-Object -First 3 | ForEach-Object { $_.Line })
$lastStage = if ($stages.Count) { $stages[-1] } else { -1 }
$minHeap = if ($heap.Count) { ($heap | Measure-Object -Minimum).Minimum } else { -1 }
"campaign $commit stage_last=$lastStage heap_min=$minHeap bad=$($bad -join ' | ')" | Add-Content $Out
$img = Join-Path $S64Repo 'artifacts\sd\audit-mm.img'
$fresh = $true
foreach ($d in 2, 3) {
    $tag = "ab-mm13-$d"
    & "$PSScriptRoot\probe_bonus.ps1" -Build $Build -Tag $tag -Content 0 -Portrait 13 -Downs $d -Seconds 75 -Slot $Slot `
        -FailBt -Image $img -Fresh:$fresh | Out-Null
    $fresh = $false
    $L = Get-Content (Join-Path $S64Probes "$tag.log")
    $c = @($L | Select-String '^X bcommit' | Select-Object -First 1 | ForEach-Object { $_.Line })
    $a = @($L | Select-String '^X enter scene=(19|20) prev=53 .*' | Select-Object -First 1 |
        ForEach-Object { $_.Line.Substring(0, [Math]::Min(170, $_.Line.Length)) })
    $bad = @($L | Select-String '^X (OOM|DABT|FAIL)' | Select-Object -First 2 | ForEach-Object { $_.Line })
    "bonus tab=$d $c | $a | bad=$($bad -join ' | ')" | Add-Content $Out
}
"DONE" | Add-Content $Out
