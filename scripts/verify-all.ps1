param(
    [switch]$Build,
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe'
)

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$powerShellExe = (Get-Process -Id $PID).Path

function Invoke-VerifyScript {
    param(
        [string]$Script,
        [string[]]$Arguments
    )

    $argList = @(
        '-NoProfile',
        '-ExecutionPolicy', 'Bypass',
        '-File', $Script
    ) + $Arguments

    $process = Start-Process -FilePath $powerShellExe `
        -ArgumentList $argList `
        -WorkingDirectory $root `
        -NoNewWindow `
        -Wait `
        -PassThru
    if ($process.ExitCode -ne 0) {
        exit $process.ExitCode
    }
}

if ($Build) {
    if (-not $env:DEVKITPRO) { $env:DEVKITPRO = 'C:/devkitPro' }
    if (-not $env:DEVKITARM) { $env:DEVKITARM = 'C:/devkitPro/devkitARM' }
    & make -C $root -j4
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

Invoke-VerifyScript `
    -Script (Join-Path $PSScriptRoot 'verify-runtime.ps1') `
    -Arguments @('-MelonDS', $MelonDS, '-Gdb', $Gdb)

Invoke-VerifyScript `
    -Script (Join-Path $PSScriptRoot 'verify-opening-skip.ps1') `
    -Arguments @('-MelonDS', $MelonDS, '-Gdb', $Gdb)

Invoke-VerifyScript `
    -Script (Join-Path $PSScriptRoot 'verify-opening-movie-speed.ps1') `
    -Arguments @('-MelonDS', $MelonDS, '-Gdb', $Gdb)

Invoke-VerifyScript `
    -Script (Join-Path $PSScriptRoot 'verify-title-boundary.ps1') `
    -Arguments @('-MelonDS', $MelonDS, '-Gdb', $Gdb)

Invoke-VerifyScript `
    -Script (Join-Path $PSScriptRoot 'verify-title-harness.ps1') `
    -Arguments @('-MelonDS', $MelonDS, '-Gdb', $Gdb)

Invoke-VerifyScript `
    -Script (Join-Path $PSScriptRoot 'verify-vs-setup-harness.ps1') `
    -Arguments @('-MelonDS', $MelonDS, '-Gdb', $Gdb)

Invoke-VerifyScript `
    -Script (Join-Path $PSScriptRoot 'verify-vs-start-transition-harness.ps1') `
    -Arguments @('-MelonDS', $MelonDS, '-Gdb', $Gdb)

Invoke-VerifyScript `
    -Script (Join-Path $PSScriptRoot 'verify-players-vs-setup-harness.ps1') `
    -Arguments @('-MelonDS', $MelonDS, '-Gdb', $Gdb)

Invoke-VerifyScript `
    -Script (Join-Path $PSScriptRoot 'verify-maps-setup-harness.ps1') `
    -Arguments @('-MelonDS', $MelonDS, '-Gdb', $Gdb)

Invoke-VerifyScript `
    -Script (Join-Path $PSScriptRoot 'verify-menu-chain-vsbattle-harness.ps1') `
    -Arguments @('-MelonDS', $MelonDS, '-Gdb', $Gdb)

Write-Output 'Full verification passed.'
