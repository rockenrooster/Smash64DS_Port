[CmdletBinding()]
param()

# Falsifier for R2-07 L7, the 20.12 collision joint-transform kernel. Compiles
# the SHIPPED kernel from include/nds/nds_r2_collision_mtx.h on the host and
# grades it in world units against a transcription of the decomp's float
# original -- error on a transformed point, because that is the quantity
# collision reads and a matrix-cell bound cannot be compared against a hurtbox.
#
# check-r2-collision-mtx.c has existed since the kernel was written; this
# harness did not, and the header claimed it did. It was run by hand, which is
# why nothing noticed when the header's quoted figures went stale by a
# revision. Registered now so the numbers cannot drift again.

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$outputDir = Join-Path $root 'artifacts/verifier-temp/r2-collision-mtx'
$source = Join-Path $PSScriptRoot 'check-r2-collision-mtx.c'
$binary = Join-Path $outputDir 'check-r2-collision-mtx.exe'
$armAssembly = Join-Path $outputDir 'arm-kernel.s'
$compiler = Get-Command gcc -ErrorAction Stop
$armCompiler = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gcc.exe'

New-Item -ItemType Directory -Force -Path $outputDir | Out-Null
& $compiler.Source -O2 -std=c11 -Wall -Wextra -Werror -fno-fast-math `
    -ffp-contract=off "-I$(Join-Path $root 'include')" $source -o $binary -lm
if ($LASTEXITCODE -ne 0) { throw 'R2 collision-matrix falsifier failed to compile.' }
& $binary
if ($LASTEXITCODE -ne 0) { throw 'R2 collision-matrix falsifier failed.' }

# The kernel must compile clean for the real target, and the wired entry point
# must be integer-only apart from the two conversions at its edges. A stray
# soft-float temporary inside the inverse would hand the whole saving back --
# that is the entire reason this kernel exists (L6: 238,426 cycles/frame over
# 74 calls, 66.2% of the over-gate frame's premium being soft-float).
if (-not (Test-Path -LiteralPath $armCompiler -PathType Leaf)) {
    throw "ARM compiler is missing: $armCompiler"
}
& $armCompiler -O2 -std=c11 -Wall -Wextra -Werror `
    -march=armv5te -mtune=arm946e-s -marm `
    "-I$(Join-Path $root 'include')" -S $source -o $armAssembly
if ($LASTEXITCODE -ne 0) { throw 'R2 collision-matrix ARM codegen probe failed.' }

$assembly = Get-Content -LiteralPath $armAssembly -Raw
foreach ($name in @('ndsR2CollisionInvertMatrixQ12', 'ndsR2CollisionInvertFrame',
                    'ndsR2CollisionWorldToLocal', 'ndsR2CollisionCompose')) {
    $kernel = [regex]::Match(
        $assembly, "(?ms)^$name`:\s*(.*?)^\s*\.size\s+$name")
    if (-not $kernel.Success) { continue }
    foreach ($routine in @('__aeabi_fadd', '__aeabi_fmul', '__aeabi_fdiv',
                           '__aeabi_fsub', '__aeabi_dmul', '__aeabi_ddiv',
                           '__aeabi_f2d', '__aeabi_d2f')) {
        if ($kernel.Groups[1].Value.Contains($routine)) {
            throw "R2 collision kernel $name calls $routine; it must be integer-only."
        }
    }
    # -mthumb has no SMULL, so a 64-bit multiply becomes __aeabi_lmul and the
    # fixed-point form ends up DEARER than the float it replaced (memory:
    # "Thumb hides 64-bit cost"). Every caller must therefore build this ARM.
    if ($kernel.Groups[1].Value.Contains('__aeabi_lmul')) {
        throw "R2 collision kernel $name calls __aeabi_lmul even in ARM mode."
    }
}

Write-Output 'R2 collision-matrix check passed: gated domain within bound, integer-only kernel in ARM mode.'
