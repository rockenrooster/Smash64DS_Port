param(
    [string]$BattleShipRoot = ''
)

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path

if ([string]::IsNullOrWhiteSpace($BattleShipRoot)) {
    $BattleShipRoot = Join-Path $root 'decomp/BattleShip-main/decomp'
}
$BattleShipRoot = (Resolve-Path -LiteralPath $BattleShipRoot).Path

$upstreamHeader = Join-Path $BattleShipRoot 'src/gm/gmsound.h'
$localHeader = Join-Path $root 'include/gm/gmsound.h'
if (-not (Test-Path -LiteralPath $upstreamHeader -PathType Leaf)) {
    throw "BattleShip sound header not found: $upstreamHeader"
}
if (-not (Test-Path -LiteralPath $localHeader -PathType Leaf)) {
    throw "Local sound compatibility header not found: $localHeader"
}

$compilerCommand = Get-Command gcc -ErrorAction SilentlyContinue
if ($null -eq $compilerCommand) {
    $devkitCompiler = if ([string]::IsNullOrWhiteSpace($env:DEVKITARM)) {
        $null
    }
    else {
        Join-Path $env:DEVKITARM 'bin/arm-none-eabi-gcc.exe'
    }
    if (($null -ne $devkitCompiler) -and
        (Test-Path -LiteralPath $devkitCompiler -PathType Leaf)) {
        $compiler = $devkitCompiler
    }
    else {
        throw 'A C compiler is required (gcc or DEVKITARM/bin/arm-none-eabi-gcc.exe).'
    }
}
else {
    $compiler = $compilerCommand.Source
}

$localText = Get-Content -LiteralPath $localHeader -Raw
$audioNames = @(
    [regex]::Matches(
        $localText,
        '\bnSYAudio(?:BGM|FGM|Voice)[A-Za-z0-9_]*\b'
    ) |
        ForEach-Object { $_.Value } |
        Sort-Object -Unique
)
if ($audioNames.Count -eq 0) {
    throw 'No local audio IDs were found to compare.'
}

# Rename the local compatibility subset while including BattleShip, then let
# the compiler evaluate implicit values and aliases on both sides.
$source = [System.Collections.Generic.List[string]]::new()
$source.Add('#define gmMusicID BattleShip_gmMusicID')
$source.Add('#define gmFGMVoiceID BattleShip_gmFGMVoiceID')
foreach ($name in $audioNames) {
    $source.Add("#define $name BattleShip_$name")
}
$source.Add('#include "src/gm/gmsound.h"')
foreach ($name in $audioNames) {
    $source.Add("#undef $name")
}
$source.Add('#undef gmFGMVoiceID')
$source.Add('#undef gmMusicID')
$source.Add('#include "gm/gmsound.h"')
foreach ($name in $audioNames) {
    $source.Add(('_Static_assert((int){0} == (int)BattleShip_{0}, ' +
        '"{0} differs from BattleShip REGION_US");') -f $name)
}
$source.Add('int main(void) { return 0; }')

$compilerArgs = @(
    '-std=c11',
    '-fsyntax-only',
    '-x',
    'c',
    '-',
    '-DREGION_US',
    '-D_LANGUAGE_C',
    "-I$(Join-Path $BattleShipRoot 'include')",
    "-I$BattleShipRoot",
    "-I$(Join-Path $root 'include')"
)
$compilerOutput = ($source -join "`n") |
    & $compiler @compilerArgs 2>&1
if ($LASTEXITCODE -ne 0) {
    $details = ($compilerOutput | Out-String).Trim()
    throw "Audio ID fixture comparison failed.`n$details"
}

Write-Output (
    'Audio ID fixtures passed: {0} local constants match BattleShip REGION_US.' -f
    $audioNames.Count
)
