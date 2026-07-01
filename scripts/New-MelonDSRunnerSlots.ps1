[CmdletBinding()]
param(
    [ValidateRange(1,32)][int]$Count,
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [switch]$Force,
    [switch]$List
)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$sourceMelonDS = if ([System.IO.Path]::IsPathRooted($MelonDS)) {
    $MelonDS
} else {
    Join-Path $root $MelonDS
}
$runnerRoot = Join-Path $root 'emulators\melonds-runners'
function New-RunnerRow {
    param(
        [int]$Slot,
        [string]$Path,
        [int]$Arm9Port,
        [int]$Arm7Port,
        [string]$Config,
        [string]$Status
    )
    [PSCustomObject]@{
        slot = $Slot
        melonDS = $Path
        arm9Port = $Arm9Port
        arm7Port = $Arm7Port
        config = $Config
        status = $Status
    }
}
function New-MelonDSExeLinkOrCopy {
    param(
        [string]$Source,
        [string]$Destination
    )
    if (Test-Path -LiteralPath $Destination) {
        if (-not $Force) { return 'existing' }
        Remove-Item -LiteralPath $Destination -Force
    }
    try {
        New-Item -ItemType HardLink -Path $Destination -Target $Source | Out-Null
        return 'hardlink'
    } catch {
        Copy-Item -LiteralPath $Source -Destination $Destination -Force
        return 'copy'
    }
}
if (-not $List -and -not (Test-Path -LiteralPath $sourceMelonDS)) {
    throw "melonDS executable not found: $sourceMelonDS"
}
$rows = @()
for ($slot = 0; $slot -lt $Count; $slot++) {
    $slotDir = Join-Path $runnerRoot "slot$slot"
    $slotMelonDS = Join-Path $slotDir 'melonDS.exe'
    $slotConfig = Join-Path $slotDir 'melonDS.toml'
    $arm9Port = Get-MelonDSRunnerPort -RunnerSlot $slot -Cpu ARM9
    $arm7Port = Get-MelonDSRunnerPort -RunnerSlot $slot -Cpu ARM7
    $status = if (Test-Path -LiteralPath $slotMelonDS) { 'exists' } else { 'missing' }
    if (-not $List) {
        New-Item -ItemType Directory -Force -Path $slotDir | Out-Null
        $status = New-MelonDSExeLinkOrCopy -Source $sourceMelonDS -Destination $slotMelonDS
        $sourceConfig = Join-Path (Split-Path -Parent $sourceMelonDS) 'melonDS.toml'
        if ($Force -or -not (Test-Path -LiteralPath $slotConfig)) {
            if (Test-Path -LiteralPath $sourceConfig) {
                Copy-Item -LiteralPath $sourceConfig -Destination $slotConfig -Force
            } else {
                Set-Content -LiteralPath $slotConfig -Value '' -NoNewline
            }
        }
        Set-MelonDSGdbConfig `
            -MelonDSPath $slotMelonDS `
            -GdbPort $arm9Port `
            -Arm7Port $arm7Port `
            -Persistent | Out-Null
    }
    $rows += New-RunnerRow `
        -Slot $slot `
        -Path $slotMelonDS `
        -Arm9Port $arm9Port `
        -Arm7Port $arm7Port `
        -Config $slotConfig `
        -Status $status
}
$rows | Format-Table slot, arm9Port, arm7Port, status, melonDS, config -AutoSize
