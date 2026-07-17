[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$outputDir = Join-Path $root 'artifacts/verifier-temp/fighter-matrix-angle-index'
$source = Join-Path $PSScriptRoot 'check-fighter-matrix-angle-index.c'
$binary = Join-Path $outputDir 'check-fighter-matrix-angle-index.exe'
$armAssembly = Join-Path $outputDir 'arm-verifier.s'
$compiler = Get-Command gcc -ErrorAction Stop
$armCompiler = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gcc.exe'

New-Item -ItemType Directory -Force -Path $outputDir | Out-Null
& $compiler.Source -O3 -std=c11 -Wall -Wextra -Werror -fno-fast-math `
    -ffp-contract=off -fexcess-precision=standard $source -o $binary
if ($LASTEXITCODE -ne 0) {
    throw "Fighter matrix angle-index verifier failed to compile."
}
& $binary
if ($LASTEXITCODE -ne 0) {
    throw "Fighter matrix angle-index verifier failed."
}

if (-not (Test-Path -LiteralPath $armCompiler -PathType Leaf)) {
    throw "ARM compiler is missing: $armCompiler"
}
& $armCompiler -O3 -std=c11 -Wall -Wextra -Werror -fno-fast-math `
    -ffp-contract=off -march=armv5te -mtune=arm946e-s -marm `
    -S $source -o $armAssembly
if ($LASTEXITCODE -ne 0) {
    throw "Fighter matrix angle-index ARM codegen probe failed to compile."
}

$assembly = Get-Content -LiteralPath $armAssembly -Raw
$probe = [regex]::Match(
    $assembly,
    '(?ms)^ndsFighterMatrixAngleToIndexCodegenProbe:\s*(.*?)^\s*\.size\s+ndsFighterMatrixAngleToIndexCodegenProbe,\s*\.-ndsFighterMatrixAngleToIndexCodegenProbe\s*$')
if (-not $probe.Success) {
    throw "Fighter matrix angle-index ARM codegen probe body is missing."
}
$probeBody = $probe.Groups[1].Value
if ($probeBody -notmatch '(?m)\bumull\b') {
    throw "Fighter matrix angle-index ARM codegen lost its inline UMULL path."
}
if ($probeBody -match '__aeabi_') {
    throw "Fighter matrix angle-index ARM codegen introduced a runtime helper call."
}

Write-Output 'FIGHTER_MATRIX_ANGLE_INDEX_CODEGEN=PASS mode=ARM umull=1 helper_calls=0'
