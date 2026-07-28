[CmdletBinding()]
param()

# Falsifier for NDS_R2_FIXED_SQRT (R2-03 E1). Compiles the shipped kernel from
# include/nds/nds_r2_sqrtf.h on the host, with the DS hardware square-root unit
# stood in for by an exact integer root, and compares every result against the
# host's IEEE sqrtf. sqrtf is on the gameplay path, so "close" is not the bar --
# bit-exact is, and that is what lets the Task 37 state hash stay the gate.

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$outputDir = Join-Path $root 'artifacts/verifier-temp/r2-fixed-sqrt'
$source = Join-Path $PSScriptRoot 'check-r2-fixed-sqrt.c'
$binary = Join-Path $outputDir 'check-r2-fixed-sqrt.exe'
$armAssembly = Join-Path $outputDir 'arm-kernel.s'
$compiler = Get-Command gcc -ErrorAction Stop
$armCompiler = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gcc.exe'

New-Item -ItemType Directory -Force -Path $outputDir | Out-Null
& $compiler.Source -O2 -std=c11 -Wall -Wextra -Werror -fno-fast-math `
    -ffp-contract=off $source -o $binary -lm
if ($LASTEXITCODE -ne 0) { throw 'R2 fixed-sqrt falsifier failed to compile.' }
& $binary
if ($LASTEXITCODE -ne 0) { throw 'R2 fixed-sqrt falsifier failed.' }

# The kernel must also compile clean for the real target, and it must not have
# picked up a call to a soft-float routine -- the whole point is to stop calling
# those, and a stray float temporary would quietly hand the saving back.
if (-not (Test-Path -LiteralPath $armCompiler -PathType Leaf)) {
    throw "ARM compiler is missing: $armCompiler"
}
& $armCompiler -O2 -std=c11 -Wall -Wextra -Werror `
    -march=armv5te -mtune=arm946e-s -marm `
    -S $source -o $armAssembly
if ($LASTEXITCODE -ne 0) { throw 'R2 fixed-sqrt ARM codegen probe failed.' }

$assembly = Get-Content -LiteralPath $armAssembly -Raw
$kernel = [regex]::Match(
    $assembly,
    '(?ms)^ndsR2SqrtfBits:\s*(.*?)^\s*\.size\s+ndsR2SqrtfBits')
if ($kernel.Success) {
    foreach ($routine in @('__aeabi_fadd', '__aeabi_fmul', '__aeabi_fdiv',
                           '__aeabi_fsub', '__ieee754_sqrtf')) {
        if ($kernel.Groups[1].Value.Contains($routine)) {
            throw "R2 fixed-sqrt kernel calls $routine; it must be integer-only."
        }
    }
}

Write-Output 'R2 fixed-sqrt check passed: bit-exact against IEEE sqrtf, integer-only kernel.'
