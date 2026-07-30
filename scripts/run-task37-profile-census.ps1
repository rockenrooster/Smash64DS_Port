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
    [string]$OutDir = '',
    # Tag every census frame with its own profiler region. Costs one CP15 write
    # per frame inside the window, in profile builds only, and is the difference
    # between "this window costs N" and "the eight frames that miss the gate cost
    # N". R2-03 E53 ran this instrument without it, had to choose its window by
    # WORK-H, and could not separate draw cost from update cost as a result.
    [bool]$PerFrameRegion = $true,
    # Partition the census frames by whether they executed this symbol, and rank
    # every symbol by the per-frame cycle difference. Requires -PerFrameRegion.
    [string]$SplitBySymbol = '',
    # Which loop drives the window. `Battle` counts presented frames, the
    # campaign default. `Results` counts sMNVSResultsTotalTimeTics instead,
    # because the VS Results loop never increments the presented-frame counter --
    # a battle-keyed window would open and dump during the match and describe
    # nothing about Results. Results runs must emulate the entire one-minute
    # match before the window opens, so give them a long -TimeoutSeconds.
    [ValidateSet('Battle','Results')]
    [string]$Scene = 'Battle',
    # Which ROM the window is measured on. The default is the tick-HUD battle
    # ROM every census before 2026-07-30 used. `smash64ds-results-lab-hwtri`
    # boots straight into VS Results with a finished match seeded, which is the
    # difference between a Results profile costing twenty minutes of emulated
    # match and costing seconds. It rides the same Makefile block as the
    # tick-HUD ROM and differs only in the scene it boots, so numbers from the
    # two remain comparable.
    [string]$Target = 'smash64ds-battle-playable-tickhud-hwtri'
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
$target = $Target
$isResults = ($Scene -eq 'Results')
# Battle frame 438 is deep into a settled match; Results tic 131 is where R0
# measured the scene's cost plateau. Only override a default the caller left alone.
if ($isResults -and (-not $PSBoundParameters.ContainsKey('StartFrame'))) {
    $StartFrame = 131
}
if ($isResults -and (-not $PSBoundParameters.ContainsKey('Build'))) {
    $Build = 'build-task37-profile-results'
}

$context = Initialize-MelonDSVerifierContext `
    -Root $root -MelonDS $MelonDS -RunnerSlot $RunnerSlot -NoBuild:$NoBuild
$rom = Resolve-Smash64DSBuildOutput `
    -Root $root -Target $target -Build $Build -Extension '.nds'
$elf = Resolve-Smash64DSBuildOutput `
    -Root $root -Target $target -Build $Build -Extension '.elf'

if ([string]::IsNullOrWhiteSpace($OutDir)) {
    $OutDir = Join-Path $root 'artifacts\task37-census'
}
# melonDS runs with its own working directory (emulators/melonds) and opens
# MELONDS_ARM9_PROFILE_CSV verbatim. A relative -OutDir would therefore resolve
# against the emulator directory, the open would fail against the missing
# folder, and the run would grind through the whole census window writing
# nothing at all -- the failure is silent and costs a full emulator run.
if (-not [System.IO.Path]::IsPathRooted($OutDir)) {
    $OutDir = Join-Path $root $OutDir
}
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
$OutDir = (Resolve-Path -LiteralPath $OutDir).Path
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
            "NDS_TASK37_PROFILE_FRAMES=$Frames" `
            "NDS_TASK37_PROFILE_PER_FRAME_REGION=$([int]$PerFrameRegion)" `
            "NDS_TASK37_PROFILE_RESULTS=$([int]$isResults)"
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
    Write-Host "scene:         $Scene$(if ($isResults) { ' (window counts Results tics, not presented frames)' })"
    Write-Host "census window: frames $StartFrame..$($StartFrame + $Frames)"
    # The ROM sets region r at the END of iteration StartFrame+r-1, so it is
    # iteration StartFrame+r that accumulates into r. Region 0 is everything
    # outside the window.
    Write-Host "per-frame region: $PerFrameRegion (region r = presented frame $StartFrame + r)"
    # DLDI is a performance variable worth ~29,696 WORK-H P95 and it is the
    # retail-parity config, so stamp it rather than leaving a reader to assume.
    $dldi = Get-MelonDSDldiEnabled -ConfigPath $configState.Config
    Write-Host "dldi:          $(if ($null -eq $dldi) { 'unknown' } else { $dldi })"
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
$censusArgs = @($csv, '--elf', $elf, '--readelf', $readelf, '--top', $Top, '--json', $json)
if (-not [string]::IsNullOrWhiteSpace($SplitBySymbol)) {
    if (-not $PerFrameRegion) {
        throw '-SplitBySymbol needs -PerFrameRegion; without it the whole window is one region.'
    }
    $censusArgs += @('--split-by-symbol', $SplitBySymbol)
}
& $python (Join-Path $PSScriptRoot 'task37_census.py') @censusArgs |
    Tee-Object -FilePath $report
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
Write-Host ''
Write-Host "wrote $report"
