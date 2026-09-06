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
$fixture = Join-Path $PSScriptRoot 'gbi_packets_fixture.c'
$include = Join-Path $root 'include'
$sdkInclude = Join-Path $root 'decomp/BattleShip-main/decomp/include'
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
if (-not (Test-Path -LiteralPath $sdkInclude -PathType Container)) {
    throw "Source SDK include not found: $sdkInclude"
}

$tempBase = [System.IO.Path]::GetFullPath([System.IO.Path]::GetTempPath())
$tempDir = [System.IO.Path]::GetFullPath(
    (Join-Path $tempBase ('smash64ds-gbi-packets-' + [guid]::NewGuid())))
if (-not $tempDir.StartsWith(
        $tempBase, [System.StringComparison]::OrdinalIgnoreCase)) {
    throw "Refusing temporary output outside $tempBase"
}
[void](New-Item -ItemType Directory -Path $tempDir)
$portExe = Join-Path $tempDir 'gbi-packets-port-host.exe'
$sdkExe = Join-Path $tempDir 'gbi-packets-sdk-host.exe'
$armObject = Join-Path $tempDir 'gbi-packets-arm9.o'

try {
    # Port header under test.
    Invoke-Checked -Program $HostCC -Arguments @(
        '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror',
        '-DGBI_PACKETS_HOST_MAIN=1',
        '-I', $include, $fixture, '-o', $portExe
    )
    $portOutput = & $portExe
    if ($LASTEXITCODE -ne 0) {
        throw "Host GBI packets port fixture failed with exit $LASTEXITCODE"
    }
    # Actual source SDK macros (F3DEX2, the shipped GBI_UCODE) as reference.
    Invoke-Checked -Program $HostCC -Arguments @(
        '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror',
        '-DGBI_PACKETS_HOST_MAIN=1',
        '-DSSB64DS_GBI_REFERENCE_SDK=1',
        '-D_LANGUAGE_C', '-DF3DEX_GBI_2',
        '-I', $sdkInclude, $fixture, '-o', $sdkExe
    )
    $sdkOutput = & $sdkExe
    if ($LASTEXITCODE -ne 0) {
        throw "Host GBI packets SDK fixture failed with exit $LASTEXITCODE"
    }
    # The port binary asserts the negative control itself (exit 3 on a
    # MODULATEI/MODULATEI_PRIM collision); here the cross-binary words must
    # be identical line for line.
    $portText = ($portOutput -join "`n")
    $sdkText = ($sdkOutput -join "`n")
    if ($portText -ne $sdkText) {
        $portLines = ($portOutput -join "`n") -split "`n"
        $sdkLines = ($sdkOutput -join "`n") -split "`n"
        $width = [Math]::Max($portLines.Count, $sdkLines.Count)
        for ($i = 0; $i -lt $width; $i++) {
            $a = if ($i -lt $portLines.Count) { $portLines[$i] } else { '<missing>' }
            $b = if ($i -lt $sdkLines.Count) { $sdkLines[$i] } else { '<missing>' }
            if ($a -ne $b) {
                Write-Output ("mismatch line {0}: port=[{1}] sdk=[{2}]" -f $i, $a, $b)
            }
        }
        throw 'Port GBI packet words differ from the source SDK expansion.'
    }
    Write-Output ("gbi packets host comparison passed: {0} lines" -f
        ($portOutput.Count))

    Invoke-Checked -Program $ArmCC -Arguments @(
        '-std=c11', '-Os', '-mcpu=arm946e-s', '-marm', '-ffreestanding',
        '-fno-builtin', '-Wall', '-Wextra', '-Werror',
        '-DGBI_PACKETS_HOST_MAIN=0',
        '-I', $include, '-c', $fixture, '-o', $armObject
    )
    $armNm = Join-Path (Split-Path -Parent $ArmCC) 'arm-none-eabi-nm.exe'
    $symbols = & $armNm '-g' '--defined-only' $armObject
    if ($LASTEXITCODE -ne 0) {
        throw 'Unable to inspect ARM9 GBI packets fixture object.'
    }
    if (-not ($symbols -match 'smash64dsGBIPacketsFixture')) {
        throw 'ARM9 GBI packets fixture lost its device-callable entry point.'
    }
    Write-Output ("gbi packets ARM946E-S contract passed: {0} bytes" -f
        (Get-Item -LiteralPath $armObject).Length)
}
finally {
    if (Test-Path -LiteralPath $tempDir) {
        Remove-Item -LiteralPath $tempDir -Recurse -Force
    }
}
