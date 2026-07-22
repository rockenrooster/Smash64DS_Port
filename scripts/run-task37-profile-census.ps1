[CmdletBinding()]
param(
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [int]$RunnerSlot = -1,
    [string]$Build = 'build-task37-profile',
    [switch]$NoBuild,
    [ValidateRange(1,1000000)][int]$StartFrame = 438,
    [ValidateRange(1,100000)][int]$Frames = 128,
    [ValidateRange(60,7200)][int]$TimeoutSeconds = 2700,
    [ValidateRange(1,500)][int]$Top = 40,
    [string]$OutDir = ''
)

# Task 37 census driver.
#
# Runs the NDS_TASK37_PROFILE=1 ROM under the repo-owned melonDS build with the
# host ARM9 performance profiler armed, and waits for the ROM to close its own
# census window. The ROM writes a CP15 reset marker at StartFrame and a dump
# marker Frames later, so the CSV describes settled battle frames only -- a
# profiler run without those markers would fold several seconds of boot, title,
# and menu traversal into the same totals.
#
# The emulated cycle counts do not depend on how fast the host runs. They DO
# depend on which melonDS runs: only the repo build models ARMv5 icache/dcache,
# which is the entire reason placement is measurable at all. See
# emulators/README.md.

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
. (Join-Path $PSScriptRoot 'lib\build-output.ps1')

$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$target = 'smash64ds-battle-playable-tickhud-hwtri'

$context = Initialize-MelonDSVerifierContext `
    -Root $root -MelonDS $MelonDS -RunnerSlot $RunnerSlot -NoBuild:$NoBuild
$rom = Resolve-Smash64DSBuildOutput `
    -Root $root -Target $target -Build $Build -Extension '.nds'
$elf = Resolve-Smash64DSBuildOutput `
    -Root $root -Target $target -Build $Build -Extension '.elf'

if ([string]::IsNullOrWhiteSpace($OutDir)) {
    $OutDir = Join-Path $root 'artifacts\task37-census'
}
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
$csv = Join-Path $OutDir 'arm9-profile.csv'
$meta = Join-Path $OutDir 'arm9-profile.meta.txt'
$regions = Join-Path $OutDir 'arm9-profile.regions.csv'
$report = Join-Path $OutDir 'census.txt'
$json = Join-Path $OutDir 'census.json'

$temp = Get-MelonDSVerifierTempDir -Root $root -RunnerSlot $RunnerSlot
$emulatorOut = Join-Path $temp 'task37-census.melonds.out'
$emulatorErr = Join-Path $temp 'task37-census.melonds.err'
$configState = $null
$emulator = $null

try {
    if (-not $NoBuild) {
        if (-not $env:DEVKITPRO) { $env:DEVKITPRO = 'C:/devkitPro' }
        if (-not $env:DEVKITARM) { $env:DEVKITARM = 'C:/devkitPro/devkitARM' }
        make -C $root "TARGET=$target" "BUILD=$Build" `
            'NDS_TASK37_PROFILE=1' "NDS_TASK37_PROFILE_START=$StartFrame" `
            "NDS_TASK37_PROFILE_FRAMES=$Frames" -j16
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    }
    foreach ($path in @($rom, $elf)) {
        if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
            throw "Required census input is missing: $path"
        }
    }

    Remove-Item $csv, $meta, $regions, $emulatorOut, $emulatorErr `
        -Force -ErrorAction SilentlyContinue

    $configState = Enable-MelonDSGdbConfig `
        -MelonDSPath $context.MelonDSPath -GdbPort $context.GdbPort `
        -Arm7Port $context.Arm7Port `
        -Persistent:([bool]$context.PersistentConfig) -MuteAudio

    $env:MELONDS_ARM9_PROFILE_CSV = $csv
    Write-Host "census window: frames $StartFrame..$($StartFrame + $Frames)"
    Write-Host "profiler csv:  $csv"
    $emulator = Start-Process -FilePath $context.MelonDSPath `
        -ArgumentList $rom `
        -WorkingDirectory (Split-Path -Parent $context.MelonDSPath) `
        -RedirectStandardOutput $emulatorOut `
        -RedirectStandardError $emulatorErr `
        -WindowStyle Hidden -PassThru

    # The metadata file is written last, so its appearance means the whole
    # report set is on disk. Size stability guards against reading a partial
    # CSV while the emulator is still flushing.
    $deadline = (Get-Date).AddSeconds($TimeoutSeconds)
    $lastSize = -1L
    $stable = 0
    $done = $false
    while ((Get-Date) -lt $deadline) {
        $emulator.Refresh()
        if ($emulator.HasExited) {
            throw "melonDS exited before the census window closed (exit $($emulator.ExitCode))."
        }
        if ((Test-Path -LiteralPath $meta -PathType Leaf) -and
            (Test-Path -LiteralPath $csv -PathType Leaf)) {
            $size = (Get-Item -LiteralPath $csv).Length
            if (($size -eq $lastSize) -and ($size -gt 0)) {
                $stable++
                if ($stable -ge 3) { $done = $true; break }
            } else {
                $stable = 0
            }
            $lastSize = $size
        }
        Start-Sleep -Milliseconds 1000
    }
    if (-not $done) {
        throw ("Census window did not close within $TimeoutSeconds s. " +
            "The ROM dumps at presented frame $($StartFrame + $Frames); check that " +
            'it was built with NDS_TASK37_PROFILE=1.')
    }
} finally {
    if ($null -ne $emulator) {
        try { $emulator.Refresh(); if (-not $emulator.HasExited) { $emulator.Kill() } } catch {}
    }
    Restore-MelonDSGdbConfig -State $configState
    Remove-Item Env:\MELONDS_ARM9_PROFILE_CSV -ErrorAction SilentlyContinue
}

Write-Host ''
Get-Content -LiteralPath $meta | ForEach-Object { Write-Host "  $_" }
Write-Host ''

$python = if ($env:SMASH64DS_PYTHON) { $env:SMASH64DS_PYTHON } else { 'python' }
$devkitArm = if ($env:DEVKITARM) { $env:DEVKITARM } else { 'C:/devkitPro/devkitARM' }
$readelf = Join-Path $devkitArm 'bin\arm-none-eabi-readelf.exe'
if (-not (Test-Path -LiteralPath $readelf -PathType Leaf)) { $readelf = 'arm-none-eabi-readelf' }
& $python (Join-Path $PSScriptRoot 'task37_census.py') $csv `
    --elf $elf --readelf $readelf --top $Top --json $json |
    Tee-Object -FilePath $report
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
Write-Host ''
Write-Host "wrote $report"
