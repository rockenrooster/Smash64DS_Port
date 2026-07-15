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
$payload = Join-Path $repoRoot 'assets/renderer/pupupu_water_tiled_aot.bin'
$gcc = 'C:/devkitPro/devkitARM/bin/arm-none-eabi-gcc.exe'
$sizeTool = 'C:/devkitPro/devkitARM/bin/arm-none-eabi-size.exe'
$makefile = Join-Path $repoRoot 'Makefile'

Assert-Condition (Test-Path -LiteralPath $generator -PathType Leaf) `
    'Tiled-water generator is absent.'
Assert-Condition (Test-Path -LiteralPath $module -PathType Leaf) `
    'Tiled-water generated module is absent.'
Assert-Condition (Test-Path -LiteralPath $payload -PathType Leaf) `
    'Tiled-water generated residency payload is absent.'
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

# This packet remains absent from default/published builds until its device
# gate passes.  A single explicit lab selector is allowed to link the compact
# state/geometry module and package the NitroFS residency payload so the same
# ROM can exercise the real pre-GO path.
$makeText = Get-Content -LiteralPath $makefile -Raw
Assert-Condition ($makeText.Contains(
    'NDS_RENDERER_M4_WATER_TILED_AOT ?= 0')) `
    'Tiled-water lab selector is not default-off.'
Assert-Condition ($makeText.Contains(
    'ifeq ($(NDS_RENDERER_M4_WATER_TILED_AOT),1)')) `
    'Tiled-water source/payload is not guarded by its explicit lab selector.'
Assert-Condition ($makeText.Contains(
    'CFILES += pupupu_water_tiled_aot.c')) `
    'Tiled-water lab selector no longer links the compact ARM module.'
Assert-Condition ($makeText.Contains(
    '$(NITROFS_DIR)/renderer/pupupu_water_tiled_aot.bin')) `
    'Tiled-water lab selector no longer packages the residency payload.'
Assert-Condition (-not $makeText.Contains(
    'override NDS_RENDERER_M4_WATER_TILED_AOT := 1')) `
    'A published or named target enabled tiled-water before its device gate.'
$headerText = Get-Content -LiteralPath $header -Raw
$moduleText = Get-Content -LiteralPath $module -Raw
$payloadFile = Get-Item -LiteralPath $payload
$payloadHash = (Get-FileHash -LiteralPath $payload -Algorithm SHA256).Hash.ToLowerInvariant()
Assert-Condition ($payloadFile.Length -eq 167936) `
    "Tiled-water payload is $($payloadFile.Length) bytes, expected 167936."
Assert-Condition ($payloadHash -eq 'af052624915c87205bfbff8e0db1e3365d3d7a30758fba74b32fdaf39c364982') `
    "Tiled-water payload SHA256 changed: $payloadHash"
Assert-Condition ($headerText.Contains(
    '#define NDS_PUPUPU_WATER_TILED_RESIDENCY_PAYLOAD_BYTES 167936u')) `
    'Tiled-water header lost the exact residency payload size.'
Assert-Condition ($headerText.Contains(
    '#define NDS_PUPUPU_WATER_TILED_SECONDARY_ATLAS_OFFSET 131072u')) `
    'Tiled-water header lost the secondary-atlas payload offset.'
Assert-Condition ($headerText.Contains(
    '#define NDS_PUPUPU_WATER_TILED_PALETTES_OFFSET 147456u')) `
    'Tiled-water header lost the palette payload offset.'
Assert-Condition ($headerText.Contains('u16 state_cell_count;')) `
    'Frame API no longer names the rectangular state-grid count explicitly.'
Assert-Condition ($headerText.Contains('Each plan cell''s cell_index')) `
    'Frame API no longer documents plan-cell to state-grid indexing.'
Assert-Condition ($moduleText.Contains('out_frame->state_cell_count = 64u;')) `
    'Large pond frame view no longer exposes its 64-cell state grid.'
Assert-Condition ($moduleText.Contains('out_frame->state_cell_count = 8u;')) `
    'Small pond frame view no longer exposes its 8-cell state grid.'
Assert-Condition ($moduleText.Contains(
    'out_assets->plan_cell_count = NDS_PUPUPU_WATER_TILED_PLAN_CELL_COUNT;')) `
    'Asset API no longer exposes its separate occupied plan-cell count.'
Assert-Condition ($headerText.Contains(
    '#define NDS_PUPUPU_WATER_TILED_LOGICAL_CELL_SUBMISSIONS 68u')) `
    'Tiled-water contract lost its logical plan-cell submission count.'
Assert-Condition ($headerText.Contains(
    '#define NDS_PUPUPU_WATER_TILED_GX_BEGIN_BATCHES_MAX 2u')) `
    'Tiled-water contract no longer limits the one-pass atlas batches.'
Assert-Condition ($headerText.Contains(
    '#define NDS_PUPUPU_WATER_TILED_EMITTED_VERTICES 414u')) `
    'Tiled-water contract lost its expanded GL_TRIANGLES vertex count.'
Assert-Condition ($headerText.Contains(
    '#define NDS_PUPUPU_WATER_TILED_VERTEX_ATTRIBUTE_WRITES 1242u')) `
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
    Assert-Condition ($textBytes -eq 144) `
        "Generated tiled-water ARM text is $textBytes bytes, expected 144."
    Assert-Condition ($rodataBytes -eq 10960) `
        "Generated tiled-water ARM rodata is $rodataBytes bytes, expected 10960."
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
    "repeat=$repeat payload=$($payloadFile.Length) payload_sha256=$payloadHash " +
    "text=$textBytes rodata=$rodataBytes data=$dataBytes bss=$bssBytes " +
    'production_linked=0 lab_linked=1'
)
