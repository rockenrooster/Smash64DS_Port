[CmdletBinding()]
param(
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$GdbPort = 4593,
    [int]$RunnerSlot = -1,
    [string]$Build = 'build-task10-hardware-calibration',
    [switch]$NoBuild
)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
. (Join-Path $PSScriptRoot 'lib\build-output.ps1')

$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$target = 'smash64ds-task10-hardware-calibration'
$context = Initialize-MelonDSVerifierContext `
    -Root $root -MelonDS $MelonDS -RunnerSlot $RunnerSlot `
    -GdbPort $GdbPort -GdbPortExplicit:$PSBoundParameters.ContainsKey('GdbPort') `
    -NoBuild:$NoBuild
$rom = Resolve-Smash64DSBuildOutput `
    -Root $root -Target $target -Build $Build -Extension '.nds'
$elf = Resolve-Smash64DSBuildOutput `
    -Root $root -Target $target -Build $Build -Extension '.elf'
$temp = Get-MelonDSVerifierTempDir -Root $root -RunnerSlot $RunnerSlot
$gdbScript = Join-Path $temp 'task10-hardware-calibration.gdb'
$gdbOut = Join-Path $temp 'task10-hardware-calibration.gdb.out'
$gdbErr = Join-Path $temp 'task10-hardware-calibration.gdb.err'
$emulatorOut = Join-Path $temp 'task10-hardware-calibration.melonds.out'
$emulatorErr = Join-Path $temp 'task10-hardware-calibration.melonds.err'
$configState = $null
$emulator = $null

try {
    if (-not $NoBuild) {
        if (-not $env:DEVKITPRO) { $env:DEVKITPRO = 'C:/devkitPro' }
        if (-not $env:DEVKITARM) { $env:DEVKITARM = 'C:/devkitPro/devkitARM' }
        make -C $root "TARGET=$target" "BUILD=$Build" -j16
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    }
    foreach ($path in @($rom, $elf, $Gdb)) {
        if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
            throw "Required Task 10 file is missing: $path"
        }
    }
    & (Join-Path $PSScriptRoot 'check-task10-hardware-calibration.ps1') `
        -Elf $elf

    $configState = Enable-MelonDSGdbConfig `
        -MelonDSPath $context.MelonDSPath -GdbPort $context.GdbPort `
        -Arm7Port $context.Arm7Port `
        -Persistent:([bool]$context.PersistentConfig) -MuteAudio
    Remove-Item $gdbOut, $gdbErr, $emulatorOut, $emulatorErr `
        -Force -ErrorAction SilentlyContinue
    $emulator = Start-Process -FilePath $context.MelonDSPath `
        -ArgumentList $rom `
        -WorkingDirectory (Split-Path -Parent $context.MelonDSPath) `
        -RedirectStandardOutput $emulatorOut `
        -RedirectStandardError $emulatorErr `
        -WindowStyle Hidden -PassThru
    Wait-MelonDSGdbListener -Process $emulator -Port $context.GdbPort | Out-Null

    [System.IO.File]::WriteAllLines($gdbScript, @(
        'set pagination off',
        'set confirm off',
        'set remotetimeout 30',
        "target remote 127.0.0.1:$($context.GdbPort)",
        'break ndsTask10HardwareCalibrationCompleteMarker',
        'continue',
        'printf "TASK10=%#x,%u,%u,%u,%u,%u,%#x\n", gNdsTask10HardwareCalibrationComplete, gNdsTask10HardwareCalibrationResults[0], gNdsTask10HardwareCalibrationResults[1], gNdsTask10HardwareCalibrationResults[2], gNdsTask10HardwareCalibrationResults[3], gNdsTask10HardwareCalibrationResults[4], gNdsTask10HardwareCalibrationSink',
        'detach'))
    $gdbProcess = Start-Process -FilePath $Gdb `
        -ArgumentList @('-q', '-batch', '-x', $gdbScript, $elf) `
        -WorkingDirectory $root `
        -RedirectStandardOutput $gdbOut `
        -RedirectStandardError $gdbErr `
        -Wait -PassThru
    if ($gdbProcess.ExitCode -ne 0) {
        throw "Task 10 GDB run failed: $(Get-Content $gdbErr -Raw)"
    }
    $output = Get-Content $gdbOut -Raw
    $match = [regex]::Match(
        $output,
        'TASK10=(0x[0-9a-fA-F]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+)')
    if (-not $match.Success -or $match.Groups[1].Value -ne '0x50415353') {
        throw "Task 10 lab did not complete: $output"
    }
    $ticks = 2..6 | ForEach-Object { [uint64]$match.Groups[$_].Value }
    if (@($ticks | Where-Object { $_ -eq 0u }).Count -ne 0) {
        throw "Task 10 lab returned a zero-tick bench: $output"
    }

    $stamp = Get-Date -Format 'yyyy-MM-dd_HHmmss-fffffff'
    $screenshot = Join-Path $root `
        "artifacts\visibility\${stamp}_task10-hardware-calibration.png"
    Start-Sleep -Seconds 1
    & (Join-Path $PSScriptRoot 'capture-running-melonds-window.ps1') `
        -EmulatorProcessId $emulator.Id -Output $screenshot
    Add-Type -AssemblyName System.Drawing
    $bitmap = [System.Drawing.Bitmap]::FromFile($screenshot)
    try {
        $darkSamples = 0
        for ($y = 356; $y -lt [Math]::Min(656, $bitmap.Height); $y += 4) {
            for ($x = 8; $x -lt [Math]::Min(408, $bitmap.Width); $x += 4) {
                $pixel = $bitmap.GetPixel($x, $y)
                if (($pixel.R + $pixel.G + $pixel.B) -lt 300) {
                    $darkSamples++
                }
            }
        }
        if ($darkSamples -lt 50) {
            throw "Task 10 summary capture is blank ($darkSamples dark samples)."
        }
    } finally {
        $bitmap.Dispose()
    }
    $romHash = (Get-FileHash -LiteralPath $rom -Algorithm SHA256).Hash
    $configHash = (Get-FileHash -LiteralPath $configState.Config `
        -Algorithm SHA256).Hash
    $version = (Get-Item -LiteralPath $context.MelonDSPath).VersionInfo.FileVersion
    Write-Output ("Task 10 melonDS passed: ALU-ITCM={0} MEM-THMB={1} MEM-ARM={2} CACHE4K={3} GX-BRST={4} ticks; ROM={5} SHA256={6}; melonDS={7} config={8}; screenshot={9}" -f
        $ticks[0], $ticks[1], $ticks[2], $ticks[3], $ticks[4],
        $rom, $romHash, $version, $configHash, $screenshot)
} finally {
    if ($null -ne $emulator) {
        $emulator.Refresh()
        if (-not $emulator.HasExited) {
            Stop-Process -Id $emulator.Id -Force
            $emulator.WaitForExit()
        }
    }
    Restore-MelonDSGdbConfig -State $configState
    Remove-Item $gdbScript, $gdbOut, $gdbErr, $emulatorOut, $emulatorErr `
        -Force -ErrorAction SilentlyContinue
}
