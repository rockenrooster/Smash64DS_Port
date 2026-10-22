param([string]$Build = 'walk-u16', [int]$Slot = 10, [int]$Seconds = 75, [string[]]$Only = @(), [string]$Out = '',
      [string]$Image = '')
# Stability audit: both bonus practices (tab 2 Break the Targets, tab 3 Board
# the Platforms) through the native bonus select for every fighter -- the
# twelve (v0-v11), Metal Mario (vmm) and the nineteen contents (c<id>) -- one
# probe at a time on its own slot and SD image: the board kind, the saved
# record row, per-scene heap low-water, native failures, halts. One line a run
# in $Out (default builds/probes/audit-bonus.txt).
. "$PSScriptRoot\common.ps1"
if ($Out -eq '') { $Out = Join-Path $S64Probes 'audit-bonus.txt' }
if ($Image -eq '') { $Image = Join-Path $S64Repo 'artifacts\sd\audit-bonus.img' }
$more = @('break ndsP7OnePSaveEnd', 'commands', 'silent',
  'printf "X saveend kind=%d sel=%u b1=%u/%u b2=%u/%u\n", sNdsP7OnePRunKind, sNdsP7OnePRunSel, sNdsP7OnePPlayerRow.bonus1_task_count, sNdsP7OnePPlayerRow.bonus1_time, sNdsP7OnePPlayerRow.bonus2_task_count, sNdsP7OnePPlayerRow.bonus2_time',
  'continue', 'end')
$runs = @()
for ($i = 0; $i -lt 12; $i++) { $runs += , @("v$i", 0, $i) }
$runs += , @('vmm', 0, 12)
foreach ($c in @(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 14, 15, 16, 17, 18, 19, 20)) { $runs += , @("c$c", $c, 255) }
$fresh = $true
foreach ($r in $runs) {
    if ($Only.Count -gt 0 -and ($Only -notcontains $r[0])) { continue }
    foreach ($d in @(2, 3)) {
        $tag = "ab-$($r[0])-$d"
        & "$PSScriptRoot\probe_bonus.ps1" -Build $Build -Tag $tag -Content $r[1] -Portrait $r[2] -Downs $d `
            -Seconds $Seconds -Slot $Slot -FailBt -More $more -Image $Image -Fresh:$fresh | Out-Null
        $fresh = $false
        $log = Join-Path $S64Probes "$tag.log"
        $L = if (Test-Path $log) { Get-Content $log } else { @() }
        $commit = @($L | Select-String '^X bcommit' | Select-Object -First 1 | ForEach-Object { $_.Line -replace '^X bcommit ', '' })
        $save = @($L | Select-String '^X saveend' | Select-Object -First 1 | ForEach-Object { $_.Line -replace '^X saveend ', '' })
        $board = @($L | Select-String '^X enter scene=(19|20) prev=53 prevHeapLow=(\d+) .*native=(\d+).* board=(\d+)' |
            Select-Object -First 1 |
            ForEach-Object { "scene=$($_.Matches[0].Groups[1].Value) heap=$($_.Matches[0].Groups[2].Value) native=$($_.Matches[0].Groups[3].Value) board=$($_.Matches[0].Groups[4].Value)" })
        $bad = @($L | Select-String '^X (OOM|DABT|FAIL)' | Select-Object -First 2 | ForEach-Object { $_.Line })
        "$($r[0]) tab=$d commit=[$($commit -join '')] save=[$($save -join '')] after=[$($board -join '')] bad=$($bad -join ' | ')" |
            Add-Content -LiteralPath $Out
    }
}
"DONE" | Add-Content -LiteralPath $Out
