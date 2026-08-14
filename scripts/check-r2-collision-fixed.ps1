[CmdletBinding()]
param(
    [string]$OutputPath
)

# Falsifier for the whole-cluster fixed-point fighter hurtbox narrow phase.
#
# Two gates, and the second is not optional decoration.
#
# 1. NUMERIC. Compiles include/nds/nds_r2_collision_fixed.h -- the code that
#    ships -- on the host and grades every kernel against a transcription of the
#    decomp float originals, in world units, over the domain the game visits.
#    The enumerable halves (the sine table, the index arithmetic, the integer
#    square root over every live s^2) are exhaustive, not sampled.
#
# 2. CODEGEN. Builds src/port/nds_r2_collision_fixed.c for the real target and
#    checks the produced ARM. Two failures are inexpressible after this:
#      * __aeabi_lmul anywhere. ARMv5TE Thumb has no SMULL, so a 64-bit product
#        compiled -mthumb becomes a library call and the fixed-point form ends
#        up DEARER than the float it replaced (memory: "Thumb hides 64-bit
#        cost"). The Makefile builds this object -marm; this is what notices if
#        that rule is ever lost.
#      * soft float outside ndsR2CollisionFixedBuildLocal. That one function is
#        the declared float edge -- the animation hands it three f32 angles and
#        the table index needs a multiply and a float-to-int -- and it is
#        allowed exactly nine such calls. Anywhere else, a stray float
#        temporary hands the whole saving back, which is the entire reason the
#        kernel exists.

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$outputDir = Join-Path $root 'artifacts/verifier-temp/r2-collision-fixed'
$source = Join-Path $PSScriptRoot 'check-r2-collision-fixed.c'
$kernelSource = Join-Path $root 'src/port/nds_r2_collision_fixed.c'
$binary = Join-Path $outputDir 'check-r2-collision-fixed.exe'
$armAssembly = Join-Path $outputDir 'arm-kernel.s'
$armObject = Join-Path $outputDir 'arm-kernel.o'
$compiler = Get-Command gcc -ErrorAction Stop
$armPrefix = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-'
$armCompiler = $armPrefix + 'gcc.exe'
$armNm = $armPrefix + 'nm.exe'

New-Item -ItemType Directory -Force -Path $outputDir | Out-Null

& $compiler.Source -O2 -std=c11 -Wall -Wextra -Werror -fno-fast-math `
    -ffp-contract=off "-I$(Join-Path $root 'include')" $source -o $binary -lm
if ($LASTEXITCODE -ne 0) { throw 'R2 fixed-collision falsifier failed to compile.' }

# Tee rather than filter. A pattern filter cannot see a multi-line failure, and
# this repo has lost load-bearing lines to Select-Object four times.
$log = Join-Path $outputDir 'check-r2-collision-fixed.txt'
& $binary 2>&1 | Tee-Object -FilePath $log
$numericExit = $LASTEXITCODE
if ($OutputPath) {
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $OutputPath) |
        Out-Null
    Copy-Item -LiteralPath $log -Destination $OutputPath -Force
}
if ($numericExit -ne 0) {
    throw 'R2 fixed-collision falsifier failed: a gated domain is over bound.'
}

if (-not (Test-Path -LiteralPath $armCompiler -PathType Leaf)) {
    throw "ARM compiler is missing: $armCompiler"
}
& $armCompiler -O2 -std=c11 -Wall -Wextra -Werror `
    -march=armv5te -mtune=arm946e-s -marm `
    "-I$(Join-Path $root 'include')" -S $kernelSource -o $armAssembly
if ($LASTEXITCODE -ne 0) { throw 'R2 fixed-collision ARM codegen probe failed.' }
& $armCompiler -O2 -std=c11 -Wall -Wextra -Werror `
    -march=armv5te -mtune=arm946e-s -marm -ffunction-sections -fdata-sections `
    "-I$(Join-Path $root 'include')" -c $kernelSource -o $armObject
if ($LASTEXITCODE -ne 0) { throw 'R2 fixed-collision ARM object build failed.' }

$assembly = Get-Content -LiteralPath $armAssembly
$currentFunction = ''
$softFloat = @('__aeabi_fadd', '__aeabi_fmul', '__aeabi_fdiv', '__aeabi_fsub',
               '__aeabi_dmul', '__aeabi_ddiv', '__aeabi_f2d', '__aeabi_d2f',
               '__aeabi_f2iz', '__aeabi_i2f')
$floatEdge = 'ndsR2CollisionFixedBuildLocal'
# Six angle-to-index conversions: each is one __aeabi_fmul and one
# __aeabi_f2iz, and the three cosines add 90 degrees to the ANGLE first, so
# 6 + 6 + 3 = 15. A budget rather than a boolean because the number is the
# thing worth noticing: it is exactly the count the six lbCommonSin/Cos calls
# already pay, with their six __aeabi_i2f and six __aeabi_fmul on the way OUT
# removed, and everything downstream turned integer.
$floatEdgeBudget = 15
$edgeCalls = 0
$smullCount = 0

foreach ($line in $assembly) {
    if ($line -match '^([A-Za-z_][A-Za-z0-9_]*):\s*$') {
        $currentFunction = $matches[1]
        continue
    }
    if ($line -match '\bsmull\b|\bsmlal\b') { $smullCount++ }
    if ($line -match '\bbl\s+(__[A-Za-z0-9_]+)') {
        $callee = $matches[1]
        if ($callee -eq '__aeabi_lmul') {
            throw ("R2 fixed-collision kernel $currentFunction calls " +
                   '__aeabi_lmul even in ARM mode: the 64-bit products must ' +
                   'be SMULL.')
        }
        if ($softFloat -contains $callee) {
            if ($currentFunction -ne $floatEdge) {
                throw ("R2 fixed-collision kernel $currentFunction calls " +
                       "$callee. Only $floatEdge may touch soft float, and " +
                       'only for the sine-table index.')
            }
            $edgeCalls++
        }
    }
}
if ($edgeCalls -gt $floatEdgeBudget) {
    throw ("R2 fixed-collision $floatEdge makes $edgeCalls soft-float calls, " +
           "budget $floatEdgeBudget.")
}
if ($smullCount -lt 32) {
    throw ("R2 fixed-collision kernel emitted only $smullCount long " +
           'multiplies; the fixed-point path is not being selected.')
}

$sizes = & $armNm --print-size --size-sort $armObject |
    Where-Object { $_ -match ' [Tt] ndsR2CollisionFixed' }
Write-Output 'R2 fixed-collision ARM text, per entry point:'
$sizes | ForEach-Object { Write-Output "  $_" }
Write-Output ("R2 fixed-collision check passed: gated domains within bound, " +
              "$smullCount long multiplies, soft float confined to " +
              "$floatEdge ($edgeCalls calls).")
