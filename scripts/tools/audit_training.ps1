param([string]$Build = 'walk-u16', [int]$Slot = 9, [int]$Timeout = 420, [string[]]$Only = @(), [string]$Out = '')
# Stability audit: Training mode for every fighter (the twelve, Metal Mario,
# the nineteen contents as 64+id) through the native Training select, a stage
# each (VS and P5 stages in turn), the next fighter as the CPU; each run ends
# at the return to the Training select (scene 18 after 54) or at a halt. One
# line a run in $Out (default builds/probes/audit-training.txt).
. "$PSScriptRoot\common.ps1"
if ($Out -eq '') { $Out = Join-Path $S64Probes 'audit-training.txt' }
$sels = @(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 13) +
        @(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 14, 15, 16, 17, 18, 19, 20 | ForEach-Object { 64 + $_ })
$stages = @(0, 1, 2, 3, 4, 5, 6, 7, 8, 41, 42, 43, 44, 13, 14, 16)
$melonPath = Join-Path $S64Repo "emulators\melonds-runners\slot$Slot\melonDS.exe"
for ($i = 0; $i -lt $sels.Count; $i++) {
    $k0 = $sels[$i]; $k1 = $sels[($i + 1) % $sels.Count]; $g = $stages[$i % $stages.Count]
    $t = "at-$k0-g$g"
    if ($Only.Count -gt 0 -and ($Only -notcontains "$k0")) { continue }
    $job = Start-Job -ScriptBlock { param($tools, $tag, $g, $k0, $k1, $slot, $build)
        & "$tools\probe_halts.ps1" -Build $build -Tag $tag -Slot $slot -EndScene 18 -EndPrev 54 -Pokes @(
            'set var gNdsMenuShellWalkRoute = 1', 'set var gNdsMenuShellWalk1PModeDowns = 1',
            "set var gNdsMenuShellCssWalkTargetKind = $k0", "set var gNdsMenuShellCssWalkTargetKind2 = $k1",
            "set var gNdsMenuShellSssWalkTargetGkind = $g") 2>&1 } -ArgumentList $PSScriptRoot, $t, $g, $k0, $k1, $Slot, $Build
    if (-not (Wait-Job $job -Timeout $Timeout)) {
        Stop-Job $job
        # By slot path and command line, never by process name.
        Get-CimInstance Win32_Process | Where-Object { $_.ExecutablePath -eq $melonPath } | ForEach-Object { Stop-Process -Id $_.ProcessId -Force }
        Get-CimInstance Win32_Process | Where-Object { $_.Name -eq 'arm-none-eabi-gdb.exe' -and $_.CommandLine -like "*$t.gdb*" } |
            ForEach-Object { Stop-Process -Id $_.ProcessId -Force }
        $tm = ' TIMEOUT'
    } else { $tm = '' }
    Remove-Job $job -Force -ErrorAction SilentlyContinue
    $lines = Get-Content (Join-Path $S64Probes "$t.log") -ErrorAction SilentlyContinue |
        Select-String -Pattern '^x (enter scene=(54|18) |END|HALT|oom|lr-8)' | ForEach-Object { $_.Line -replace '^x ', '' }
    "$t cpu=$k1$tm :: " + ($lines -join ' | ') | Add-Content -LiteralPath $Out
}
"DONE" | Add-Content -LiteralPath $Out
