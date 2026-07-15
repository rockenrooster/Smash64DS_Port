param(
    [string]$SourceRoot = (Join-Path $PSScriptRoot '..'),
    [string]$HostCC = '',
    [string]$ArmCC = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gcc.exe'
)

$ErrorActionPreference = 'Stop'

function Invoke-Checked {
    param(
        [Parameter(Mandatory = $true)][string]$Program,
        [Parameter(Mandatory = $true)][string[]]$Arguments
    )

    & $Program @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "$Program failed with exit code $LASTEXITCODE"
    }
}

$root = (Resolve-Path $SourceRoot).Path
$fixture = Join-Path $PSScriptRoot 'mp_floor_crossing_fixture.c'
$include = Join-Path $root 'include'
if ([string]::IsNullOrWhiteSpace($HostCC)) {
    $hostCommand = Get-Command gcc.exe -ErrorAction SilentlyContinue
    if ($null -eq $hostCommand) {
        $hostCommand = Get-Command clang.exe -ErrorAction SilentlyContinue
    }
    if ($null -eq $hostCommand) {
        throw 'No host C compiler (gcc.exe or clang.exe) is available.'
    }
    $HostCC = $hostCommand.Source
}
if (-not (Test-Path -LiteralPath $ArmCC -PathType Leaf)) {
    throw "ARM9 compiler not found: $ArmCC"
}

$tempBase = [System.IO.Path]::GetFullPath([System.IO.Path]::GetTempPath())
$tempDir = [System.IO.Path]::GetFullPath(
    (Join-Path $tempBase ('smash64ds-mp-floor-' + [guid]::NewGuid())))
if (-not $tempDir.StartsWith(
        $tempBase, [System.StringComparison]::OrdinalIgnoreCase)) {
    throw "Refusing temporary output outside $tempBase"
}
[void](New-Item -ItemType Directory -Path $tempDir)
$hostExe = Join-Path $tempDir 'mp-floor-host.exe'
$armObject = Join-Path $tempDir 'mp-floor-arm9.o'

try {
    Invoke-Checked -Program $HostCC -Arguments @(
        '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror',
        '-DMP_FLOOR_CROSSING_HOST_MAIN=1',
        '-I', $include, $fixture, '-o', $hostExe
    )
    & $hostExe
    if ($LASTEXITCODE -ne 0) {
        throw "Host MP floor fixture failed with exit code $LASTEXITCODE"
    }

    Invoke-Checked -Program $ArmCC -Arguments @(
        '-std=c11', '-Os', '-mcpu=arm946e-s', '-marm', '-ffreestanding',
        '-fno-builtin', '-Wall', '-Wextra', '-Werror',
        '-DMP_FLOOR_CROSSING_HOST_MAIN=0',
        '-I', $include, '-c', $fixture, '-o', $armObject
    )
    $armNm = Join-Path (Split-Path -Parent $ArmCC) 'arm-none-eabi-nm.exe'
    $symbols = & $armNm '-g' '--defined-only' $armObject
    if ($LASTEXITCODE -ne 0) {
        throw 'Unable to inspect ARM9 MP floor fixture object.'
    }
    if (-not ($symbols -match 'smash64dsMPFloorCrossingFixture')) {
        throw 'ARM9 MP floor fixture lost its device-callable entry point.'
    }
    Write-Output ("mp floor crossing ARM946E-S contract passed: {0} bytes" -f
        (Get-Item -LiteralPath $armObject).Length)
}
finally {
    if (Test-Path -LiteralPath $tempDir) {
        Remove-Item -LiteralPath $tempDir -Recurse -Force
    }
}
