param(
    [switch]$Fast
)

$ErrorActionPreference = 'Stop'

function Assert-Condition {
    param(
        [bool]$Condition,
        [string]$Message
    )
    if (-not $Condition) {
        throw $Message
    }
}

$repoRoot = Split-Path -Parent $PSScriptRoot
$generator = Join-Path $PSScriptRoot 'generate_pupupu_water_tiled_aot.py'
$module = Join-Path $repoRoot 'src/nds/pupupu_water_tiled_aot.c'
$header = Join-Path $repoRoot 'include/nds/pupupu_water_tiled_aot.h'
$gcc = 'C:/devkitPro/devkitARM/bin/arm-none-eabi-gcc.exe'
$sizeTool = 'C:/devkitPro/devkitARM/bin/arm-none-eabi-size.exe'
$makefile = Join-Path $repoRoot 'Makefile'

Assert-Condition (Test-Path -LiteralPath $generator -PathType Leaf) `
    'Tiled-water generator is absent.'
Assert-Condition (Test-Path -LiteralPath $module -PathType Leaf) `
    'Tiled-water generated module is absent.'
Assert-Condition (Test-Path -LiteralPath $gcc -PathType Leaf) `
    'devkitARM GCC is absent at the canonical path.'
Assert-Condition (Test-Path -LiteralPath $sizeTool -PathType Leaf) `
    'devkitARM size tool is absent at the canonical path.'

$repeat = if ($Fast) { 1 } else { 2 }
$oldNoBytecode = $env:PYTHONDONTWRITEBYTECODE
$env:PYTHONDONTWRITEBYTECODE = '1'
try {
    & python $generator --repo-root $repoRoot --check --repeat $repeat
    Assert-Condition ($LASTEXITCODE -eq 0) `
        'Exact tiled-water generation/oracle check failed.'
}
finally {
    $env:PYTHONDONTWRITEBYTECODE = $oldNoBytecode
}

# This packet is intentionally a host/device preflight. Adding it to the live
# source list before the pre-GO VRAM transition is proven would consume the
# ARM9 image/RAM reserve and falsely claim M4 integration.
$makeText = Get-Content -LiteralPath $makefile -Raw
Assert-Condition (-not $makeText.Contains('pupupu_water_tiled_aot')) `
    'Tiled-water host packet entered the production Makefile before its device gate.'
$headerText = Get-Content -LiteralPath $header -Raw
$moduleText = Get-Content -LiteralPath $module -Raw
Assert-Condition ($headerText.Contains('u16 state_cell_count;')) `
    'Frame API no longer names the rectangular state-grid count explicitly.'
Assert-Condition ($headerText.Contains('cell_index addresses the corresponding 64-entry large or')) `
    'Frame API no longer documents plan-cell to state-grid indexing.'
Assert-Condition ($moduleText.Contains('out_frame->state_cell_count = 64u;')) `
    'Large pond frame view no longer exposes its 64-cell state grid.'
Assert-Condition ($moduleText.Contains('out_frame->state_cell_count = 8u;')) `
    'Small pond frame view no longer exposes its 8-cell state grid.'
Assert-Condition ($moduleText.Contains(
    'out_assets->plan_cell_count = NDS_PUPUPU_WATER_TILED_PLAN_CELL_COUNT;')) `
    'Asset API no longer exposes its separate occupied plan-cell count.'
Assert-Condition ($headerText.Contains(
    '#define NDS_PUPUPU_WATER_TILED_LOGICAL_CELL_SUBMISSIONS 136u')) `
    'Tiled-water contract lost its logical plan-cell submission count.'
Assert-Condition ($headerText.Contains(
    '#define NDS_PUPUPU_WATER_TILED_PLANNED_GX_BEGIN_BATCHES 2u')) `
    'Tiled-water contract no longer batches one shared-atlas GX BEGIN per pass.'
Assert-Condition ($headerText.Contains(
    '#define NDS_PUPUPU_WATER_TILED_EMITTED_VERTICES 828u')) `
    'Tiled-water contract lost its expanded GL_TRIANGLES vertex count.'
Assert-Condition ($headerText.Contains(
    '#define NDS_PUPUPU_WATER_TILED_VERTEX_ATTRIBUTE_WRITES 2484u')) `
    'Tiled-water contract lost its TEXCOORD/VTX16 register-write count.'
Assert-Condition ($headerText.Contains(
    '#define NDS_PUPUPU_WATER_TILED_DEVICE_TICK_BUDGET 40000u')) `
    'Tiled-water device falsifier lost its conservative submission budget.'
Assert-Condition ($headerText.Contains(
    '#define NDS_PUPUPU_WATER_TILED_MIN_NET_OWNER_DRAW_SAVING 100000u')) `
    'Tiled-water device falsifier lost its net owner/draw keep threshold.'

$tempBase = [IO.Path]::GetFullPath($env:TEMP).TrimEnd('\') + '\'
$temp = [IO.Path]::GetFullPath((Join-Path $env:TEMP `
    ('smash64ds-pupupu-water-' + [guid]::NewGuid().ToString('N'))))
Assert-Condition ($temp.StartsWith($tempBase, [StringComparison]::OrdinalIgnoreCase)) `
    'Refusing to create ARM check output outside the system temporary directory.'
New-Item -ItemType Directory -Path $temp | Out-Null
try {
    $object = Join-Path $temp 'pupupu_water_tiled_aot.o'
    $compileArgs = @(
        '-std=gnu11',
        '-Wall',
        '-Wextra',
        '-Werror',
        '-Os',
        '-ffunction-sections',
        '-fdata-sections',
        '-march=armv5te',
        '-mtune=arm946e-s',
        '-mthumb',
        '-DARM9',
        '-D_LANGUAGE_C',
        '-DSSB64_TARGET_NDS',
        ('-I' + (Join-Path $repoRoot 'include')),
        '-c',
        $module,
        '-o',
        $object
    )
    & $gcc @compileArgs
    Assert-Condition ($LASTEXITCODE -eq 0) `
        'Generated tiled-water module failed the ARM946E-S compile gate.'

    $sizeOutput = & $sizeTool -A $object
    Assert-Condition ($LASTEXITCODE -eq 0) `
        'Unable to inspect the generated tiled-water ARM object.'
    $textBytes = 0L
    $rodataBytes = 0L
    $dataBytes = 0L
    $bssBytes = 0L
    foreach ($line in $sizeOutput) {
        $match = [regex]::Match($line, '^\s*(\.\S+)\s+(\d+)\s+\S+\s*$')
        if (-not $match.Success) {
            continue
        }
        $name = $match.Groups[1].Value
        $bytes = [int64]$match.Groups[2].Value
        if ($name.StartsWith('.text', [StringComparison]::Ordinal)) {
            $textBytes += $bytes
        }
        elseif ($name.StartsWith('.rodata', [StringComparison]::Ordinal)) {
            $rodataBytes += $bytes
        }
        elseif ($name.StartsWith('.data', [StringComparison]::Ordinal)) {
            $dataBytes += $bytes
        }
        elseif ($name.StartsWith('.bss', [StringComparison]::Ordinal)) {
            $bssBytes += $bytes
        }
    }
    Assert-Condition ($textBytes -eq 224) `
        "Generated tiled-water ARM text is $textBytes bytes, expected 224."
    Assert-Condition ($rodataBytes -eq 181408) `
        "Generated tiled-water ARM rodata is $rodataBytes bytes, expected 181408."
    Assert-Condition ($dataBytes -eq 0) `
        "Generated tiled-water ARM object gained $dataBytes writable data bytes."
    Assert-Condition ($bssBytes -eq 0) `
        "Generated tiled-water ARM object gained $bssBytes BSS bytes."
}
finally {
    $resolvedTemp = [IO.Path]::GetFullPath($temp)
    Assert-Condition ($resolvedTemp.StartsWith(
        $tempBase, [StringComparison]::OrdinalIgnoreCase)) `
        'Refusing to remove an ARM check directory outside system temporary storage.'
    if (Test-Path -LiteralPath $resolvedTemp) {
        Remove-Item -LiteralPath $resolvedTemp -Recurse -Force
    }
}

Write-Output (
    'PUPUPU_WATER_TILED_ARM_OK ' +
    "repeat=$repeat text=$textBytes rodata=$rodataBytes data=$dataBytes bss=$bssBytes " +
    'production_linked=0'
)
