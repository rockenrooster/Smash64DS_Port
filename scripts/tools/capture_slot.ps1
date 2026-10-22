param([int]$Slot = 11, [Parameter(Mandatory)][string]$Out)
# Capture runner slot $Slot's melonDS window, found by its slot path (never by
# name: memory never-kill-by-melonds-name) -- called from a gdb breakpoint's
# `shell` while the target is stopped.
. "$PSScriptRoot\common.ps1"
$path = Join-Path $S64Repo "emulators\melonds-runners\slot$Slot\melonDS.exe"
$p = Get-Process -ErrorAction SilentlyContinue | Where-Object { $_.Path -eq $path } | Select-Object -First 1
if ($null -eq $p) { "no slot$Slot process"; exit 0 }
try {
    New-Item -ItemType Directory -Force (Split-Path $Out) | Out-Null
    & (Join-Path $S64Repo 'scripts\capture-running-melonds-window.ps1') -EmulatorProcessId $p.Id -Output $Out *> $null
    "captured $Out"
} catch { "capture failed: $_" }
