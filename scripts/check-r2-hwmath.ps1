[CmdletBinding()]
param()

# Falsifier for include/nds/nds_r2_hwmath.h -- the correctly-rounded IEEE-754
# single divide built on the ARM9 64/32 divide unit. Compiles the SHIPPED kernel
# on the host with the hardware unit stood in for by an exact C divide, and
# grades it bit-for-bit against the host's own IEEE divide over six exhaustive
# 2^23 significand axes plus a bounded random sweep.
#
# It proves the ALGORITHM half only, and says so: that the DS unit itself
# returns the exact truncated quotient and remainder is graded in the ROM by
# src/port/nds_r2_hwmath_bench.c against the portable software divide.

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$outputDir = Join-Path $root 'artifacts/verifier-temp/r2-hwmath'
$source = Join-Path $PSScriptRoot 'check-r2-hwmath.c'
$binary = Join-Path $outputDir 'check-r2-hwmath.exe'
$armAssembly = Join-Path $outputDir 'arm-kernel.s'
$compiler = Get-Command gcc -ErrorAction Stop
$armCompiler = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gcc.exe'

New-Item -ItemType Directory -Force -Path $outputDir | Out-Null
& $compiler.Source -O2 -std=c11 -Wall -Wextra -Werror -fno-fast-math `
    -ffp-contract=off $source -o $binary -lm
if ($LASTEXITCODE -ne 0) { throw 'R2 hwmath falsifier failed to compile.' }
& $binary
if ($LASTEXITCODE -ne 0) { throw 'R2 hwmath falsifier failed.' }

# The kernel must compile clean for the real target and must not have picked up
# a soft-float call or a libgcc 64-bit divide. Either would hand back exactly
# the cost this exists to delete, and neither would fail any other gate.
if (-not (Test-Path -LiteralPath $armCompiler -PathType Leaf)) {
    throw "ARM compiler is missing: $armCompiler"
}
# The probe is a SEPARATE translation unit, not this checker compiled for ARM:
# the checker models the unit with a C divide, so grepping its assembly would
# find __aeabi_ldivmod every time and prove nothing about the ROM.
$armProbe = Join-Path $PSScriptRoot 'check-r2-hwmath-arm-probe.c'
& $armCompiler -O2 -std=c11 -Wall -Wextra -Werror `
    -march=armv5te -mtune=arm946e-s -marm `
    -S $armProbe -o $armAssembly
if ($LASTEXITCODE -ne 0) { throw 'R2 hwmath ARM codegen probe failed.' }

$assembly = Get-Content -LiteralPath $armAssembly -Raw
$bodies = @('ndsR2HwMathProbeDivBits', 'ndsR2HwMathProbeCfxDiv64',
            'ndsR2HwMathProbeCfxIsqrt64')
foreach ($body in $bodies) {
    $kernel = [regex]::Match(
        $assembly, "(?ms)^${body}:\s*(.*?)^\s*\.size\s+${body}")
    if (-not $kernel.Success) {
        throw "R2 hwmath ARM codegen probe found no $body body."
    }
    foreach ($routine in @('__aeabi_fadd', '__aeabi_fmul', '__aeabi_fdiv',
                           '__aeabi_fsub', '__aeabi_ldivmod',
                           '__aeabi_uldivmod', '__udivmoddi4', '__divdi3',
                           '__udivdi3', '__aeabi_lmul')) {
        if ($kernel.Groups[1].Value.Contains($routine)) {
            throw "R2 hwmath $body calls $routine; it must reach the unit only."
        }
    }
}
# And the probe must actually touch the units -- an empty body would pass every
# grep above, so this is the control that can fail.
if (-not ($assembly -match '0x4000280|67108864')) {
    throw 'R2 hwmath ARM probe never reaches the divide/sqrt register block.'
}

Write-Output 'R2 hwmath check passed: fdiv bit-identical to IEEE on six exhaustive significand axes, rounding and exact-quotient controls live, no tie reachable (asserted, not assumed), decline path asserted, no soft-float or libgcc divide in the ARM kernel.'
