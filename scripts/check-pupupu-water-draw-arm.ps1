param(
    [string]$ArmCC = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gcc.exe'
)

$ErrorActionPreference = 'Stop'

function Assert-Condition {
    param([bool]$Condition, [string]$Message)
    if (-not $Condition) { throw $Message }
}

function Get-ArrayBody {
    param([string]$Text, [string]$Pattern, [string]$Label)
    $match = [regex]::Match($Text, "(?s)$Pattern")
    Assert-Condition $match.Success "$Label array is absent."
    return $match.Groups['body'].Value
}

function Get-ObjectTotals {
    param([string[]]$SizeOutput)
    $totals = [ordered]@{ Text = 0L; Rodata = 0L; Data = 0L; Bss = 0L }
    foreach ($line in $SizeOutput) {
        $match = [regex]::Match($line, '^\s*(\.\S+)\s+(\d+)\s+\S+\s*$')
        if (-not $match.Success) { continue }
        $name = $match.Groups[1].Value
        $bytes = [int64]$match.Groups[2].Value
        if ($name.StartsWith('.text', [StringComparison]::Ordinal)) {
            $totals.Text += $bytes
        }
        elseif ($name.StartsWith('.rodata', [StringComparison]::Ordinal)) {
            $totals.Rodata += $bytes
        }
        elseif ($name.StartsWith('.data', [StringComparison]::Ordinal)) {
            $totals.Data += $bytes
        }
        elseif ($name.StartsWith('.bss', [StringComparison]::Ordinal)) {
            $totals.Bss += $bytes
        }
    }
    return [pscustomobject]$totals
}

function Get-UndefinedSymbols {
    param([string]$Nm, [string]$Object)
    $output = @(& $Nm -u $Object)
    Assert-Condition ($LASTEXITCODE -eq 0) "Unable to inspect '$Object'."
    return @($output | ForEach-Object {
        $match = [regex]::Match($_, '^\s+U\s+(\S+)\s*$')
        Assert-Condition $match.Success "Unparsed nm line: $_"
        $match.Groups[1].Value
    } | Sort-Object)
}

function Get-CallRelocations {
    param([string[]]$Output, [string]$Section)
    $active = $false
    $calls = @()
    foreach ($line in $Output) {
        $header = [regex]::Match(
            $line, '^RELOCATION RECORDS FOR \[(\.\S+)\]:\s*$')
        if ($header.Success) {
            $active = ($header.Groups[1].Value -ceq $Section)
            continue
        }
        if (-not $active) { continue }
        $relocation = [regex]::Match(
            $line, '^\s*[0-9a-fA-F]+\s+R_ARM_(?:THM_)?CALL\s+(\S+)\s*$')
        if ($relocation.Success) { $calls += $relocation.Groups[1].Value }
    }
    return @($calls)
}

function Assert-ExactMultiset {
    param([string[]]$Actual, [string[]]$Expected, [string]$Label)
    $actualSorted = @($Actual | Sort-Object)
    $expectedSorted = @($Expected | Sort-Object)
    Assert-Condition (($actualSorted -join "`n") -ceq
        ($expectedSorted -join "`n")) `
        ("$Label changed. actual=[{0}] expected=[{1}]" -f
            ($actualSorted -join ', '), ($expectedSorted -join ', '))
}

$repoRoot = Split-Path -Parent $PSScriptRoot
$header = Join-Path $repoRoot 'include/nds/nds_pupupu_water_draw.h'
$source = Join-Path $repoRoot 'src/nds/nds_pupupu_water_draw.c'
$generated = Join-Path $repoRoot `
    'src/nds/generated/pupupu_water_tiled_aot.generated.inc'
$residency = Join-Path $repoRoot 'src/nds/nds_pupupu_water_residency.c'
$armBin = Split-Path -Parent $ArmCC
$armSize = Join-Path $armBin 'arm-none-eabi-size.exe'
$armNm = Join-Path $armBin 'arm-none-eabi-nm.exe'
$armObjdump = Join-Path $armBin 'arm-none-eabi-objdump.exe'
foreach ($path in @(
    $header, $source, $generated, $residency, $ArmCC, $armSize, $armNm,
    $armObjdump
)) {
    Assert-Condition (Test-Path -LiteralPath $path -PathType Leaf) `
        "Required water-draw input is absent: $path"
}

$headerText = Get-Content -LiteralPath $header -Raw
$sourceText = Get-Content -LiteralPath $source -Raw
$generatedText = Get-Content -LiteralPath $generated -Raw
$residencyText = Get-Content -LiteralPath $residency -Raw

$planBody = Get-ArrayBody $generatedText `
    'sNdsPupupuWaterPlanCells\[68\]\s*=\s*\{(?<body>.*?)\n\};' `
    'Water plan-cell'
$planRows = @([regex]::Matches($planBody, '\{([^{}]+)\}'))
Assert-Condition ($planRows.Count -eq 68) `
    "Water plan has $($planRows.Count) cells, expected 68."
$nextVertex = 0
$triangleCount = 0
for ($index = 0; $index -lt $planRows.Count; $index++) {
    $values = @([regex]::Matches($planRows[$index].Groups[1].Value, '(\d+)u') |
        ForEach-Object { [int]$_.Groups[1].Value })
    Assert-Condition ($values.Count -eq 7) "Plan cell $index shape changed."
    $firstVertex, $owner, $cellIndex, $cellX, $cellY, $vertices, $triangles =
        $values
    $expectedOwner = if ($index -lt 60) { 0 } else { 1 }
    $columns = if ($owner -eq 0) { 4 } else { 1 }
    Assert-Condition (($owner -eq $expectedOwner) -and
        ($firstVertex -eq $nextVertex) -and ($vertices -ge 3) -and
        ($triangles -eq $vertices - 2) -and
        ($cellIndex -eq $cellY * $columns + $cellX)) `
        "Plan cell $index lost source order/contiguous fan geometry."
    $nextVertex += $vertices
    $triangleCount += $triangles
}
$emittedVertices = $triangleCount * 3
Assert-Condition (($nextVertex -eq 274) -and ($triangleCount -eq 138) -and
    ($emittedVertices -eq 414)) `
    "Water geometry model is cells=68 plan_vertices=$nextVertex triangles=$triangleCount emitted=$emittedVertices."

function Get-StateRows {
    param([string]$Text, [string]$Name, [int]$Rows, [int]$Columns)
    $body = Get-ArrayBody $Text `
        ("$Name\[$Rows\]\[$Columns\]\s*=\s*\{(?<body>.*?)\n\};") $Name
    $matches = @([regex]::Matches($body, '\{([^{}]+)\}'))
    Assert-Condition ($matches.Count -eq $Rows) `
        "$Name has $($matches.Count) rows, expected $Rows."
    return @($matches | ForEach-Object {
        $values = @([regex]::Matches($_.Groups[1].Value, '(\d+)u') |
            ForEach-Object { [int]$_.Groups[1].Value })
        Assert-Condition ($values.Count -eq $Columns) `
            "$Name row has $($values.Count) entries, expected $Columns."
        ,$values
    })
}

$largeStates = Get-StateRows $generatedText `
    'sNdsPupupuWaterLargeStates' 38 64
$smallStates = Get-StateRows $generatedText `
    'sNdsPupupuWaterSmallStates' 46 8
foreach ($state in $largeStates) {
    Assert-Condition (($state | Measure-Object -Maximum).Maximum -lt 512) `
        'Large-water state escaped the primary atlas.'
}
foreach ($state in $smallStates) {
    Assert-Condition (($state | Measure-Object -Maximum).Maximum -lt 572) `
        'Small-water state escaped the two exact atlases.'
}
$smallFrameBody = Get-ArrayBody $generatedText `
    'sNdsPupupuWaterSmallFrames\[216\]\s*=\s*\{(?<body>.*?)\n\};' `
    'Small-water frame'
$smallFrames = @([regex]::Matches($smallFrameBody, '(\d+)u') |
    ForEach-Object { [int]$_.Groups[1].Value })
Assert-Condition ($smallFrames.Count -eq 216) `
    "Small-water frame table has $($smallFrames.Count) entries."
$secondaryFrames = 0
foreach ($stateIndex in $smallFrames) {
    Assert-Condition ($stateIndex -lt $smallStates.Count) `
        "Small-water frame state $stateIndex is invalid."
    if (($smallStates[$stateIndex] | Where-Object { $_ -ge 512 }).Count -ne 0) {
        $secondaryFrames++
    }
}
Assert-Condition ($secondaryFrames -eq 48) `
    "Secondary atlas is needed by $secondaryFrames frames, expected 48."

$oldNoBytecode = $env:PYTHONDONTWRITEBYTECODE
$env:PYTHONDONTWRITEBYTECODE = '1'
try {
    $fractionOutput = @(@'
from pathlib import Path
import sys
root = Path(sys.argv[1]).resolve()
sys.path.insert(0, str(root / "scripts"))
import generate_pupupu_water_aot as source
corpus = source.load_source_corpus(root)
censuses = [source.census_keys(corpus.bank104.payload, spec) for spec in corpus.specs]
print(sum(a.fraction != b.fraction for a, b in zip(
    censuses[0].sequence[:216], censuses[1].sequence[:216])))
'@ | python - $repoRoot)
    Assert-Condition ($LASTEXITCODE -eq 0) `
        'Unable to verify exact source water-fraction parity.'
}
finally {
    $env:PYTHONDONTWRITEBYTECODE = $oldNoBytecode
}
Assert-Condition (($fractionOutput.Count -eq 1) -and
    ([int]$fractionOutput[0] -eq 0)) `
    "Large/small source water fractions differ: $fractionOutput"

$requiredHeader = @(
    '#define NDS_PUPUPU_WATER_DRAW_CELL_COUNT 68u',
    '#define NDS_PUPUPU_WATER_DRAW_TRIANGLE_COUNT 138u',
    '#define NDS_PUPUPU_WATER_DRAW_VERTEX_COUNT 414u',
    's32 ndsPupupuWaterDrawPrepared(',
    'caller owns the exact live source parent matrix',
    'optional secondary-atlas cells in source order'
)
foreach ($fragment in $requiredHeader) {
    Assert-Condition ($headerText.Contains($fragment)) `
        "Water draw header contract lost: $fragment"
}
$requiredSource = @(
    'ndsPupupuWaterTiledGetAssets(&expected_assets)',
    '(void)ndsPupupuWaterResidencyBindPrimary(palette_index);',
    '(void)ndsPupupuWaterResidencyBindSecondary(palette_index);',
    'glBegin(GL_TRIANGLE);',
    'glEnd();',
    'vertex->s_q16 - cell_s_q16) >> 12',
    'vertex->t_q16 - cell_t_q16) >> 12',
    '(v16)(vertex->x_q12 >> 8)',
    '(v16)(vertex->z_q12 >> 8)',
    '&vertices[0]',
    '&vertices[triangle + 1u]',
    '&vertices[triangle + 2u]'
)
foreach ($fragment in $requiredSource) {
    Assert-Condition ($sourceText.Contains($fragment)) `
        "Water draw implementation lost: $fragment"
}
Assert-Condition (([regex]::Matches(
    $sourceText, '\bndsPupupuWaterResidencyBindPrimary\s*\(')).Count -eq 1) `
    'Water draw no longer has exactly one primary-bind call site.'
Assert-Condition (([regex]::Matches(
    $sourceText, '\bndsPupupuWaterResidencyBindSecondary\s*\(')).Count -eq 1) `
    'Water draw no longer has exactly one secondary-bind call site.'
Assert-Condition (([regex]::Matches($sourceText, '\bglBegin\s*\(')).Count -eq 1) `
    'Water draw no longer has one shared atlas-batch begin site.'
Assert-Condition (([regex]::Matches($sourceText, '\bglEnd\s*\(')).Count -eq 1) `
    'Water draw no longer has one shared atlas-batch end site.'
$preflightAt = $sourceText.IndexOf('failure = ndsPupupuWaterDrawPreflight')
$firstBindAt = $sourceText.IndexOf(
    '(void)ndsPupupuWaterResidencyBindPrimary(palette_index);')
Assert-Condition (($preflightAt -ge 0) -and ($preflightAt -lt $firstBindAt)) `
    'Water draw no longer preflights before its first GX mutation.'

$forbidden = @(
    'fopen', 'fread', 'fwrite', 'fclose', 'malloc', 'calloc', 'realloc',
    'free', 'memcpy', 'memmove', 'glGenTextures', 'glDeleteTextures',
    'glTexImage2D', 'glColorTableEXT', 'glGetTexturePointer', 'glPolyFmt',
    'glColor', 'glLoadMatrix', 'glMultMatrix'
)
foreach ($name in $forbidden) {
    Assert-Condition (-not [regex]::IsMatch(
        $sourceText, ('\b' + [regex]::Escape($name) + '\w*\s*\('))) `
        "Water draw gained forbidden gameplay operation '$name'."
}
Assert-Condition (([regex]::Matches($residencyText, '\bGL_RGB256\b')).Count -eq 2) `
    'Water residency no longer allocates two RGB256 atlases.'
Assert-Condition (([regex]::Matches(
    $residencyText,
    'TEXGEN_OFF\s*\|\s*GL_TEXTURE_COLOR0_TRANSPARENT')).Count -eq 2) `
    'Water residency lost TEXGEN_OFF/color-zero transparency on an atlas.'

$tempBase = [IO.Path]::GetFullPath($env:TEMP).TrimEnd('\') + '\'
$temp = [IO.Path]::GetFullPath((Join-Path $env:TEMP `
    ('smash64ds-m4-water-draw-' + [guid]::NewGuid().ToString('N'))))
Assert-Condition ($temp.StartsWith(
    $tempBase, [StringComparison]::OrdinalIgnoreCase)) `
    'Refusing water-draw ARM output outside system temporary storage.'
New-Item -ItemType Directory -Path $temp | Out-Null

try {
    $stubDir = Join-Path $temp 'stubs/nds/arm9'
    New-Item -ItemType Directory -Path $stubDir -Force | Out-Null
    [IO.File]::WriteAllText((Join-Path $stubDir 'videoGL.h'), @'
#ifndef WATER_DRAW_VIDEO_GL_STUB_H
#define WATER_DRAW_VIDEO_GL_STUB_H
#include <PR/ultratypes.h>
typedef s16 v16;
typedef s16 t16;
typedef enum { GL_TRIANGLE = 0 } GL_GLBEGIN_ENUM;
void glBegin(GL_GLBEGIN_ENUM mode);
void glEnd(void);
void glTexCoord2t16(t16 u, t16 v);
void glVertex3v16(v16 x, v16 y, v16 z);
#endif
'@)
    $normalObject = Join-Path $temp 'water-draw-normal.o'
    $stubObject = Join-Path $temp 'water-draw-stub.o'
    $common = @(
        '-std=gnu11', '-Wall', '-Wextra', '-Werror', '-Os',
        '-ffunction-sections', '-fdata-sections', '-march=armv5te',
        '-mtune=arm946e-s', '-mthumb', '-D__NDS__', '-DARM9',
        '-D_LANGUAGE_C', '-DSSB64_TARGET_NDS',
        ('-I' + (Join-Path $repoRoot 'include')),
        '-IC:/devkitPro/libnds/include', '-IC:/devkitPro/calico/include',
        '-fstack-usage', '-c', $source
    )
    & $ArmCC @common '-o' $normalObject
    Assert-Condition ($LASTEXITCODE -eq 0) `
        'Water draw failed its ARM946E-S compile gate.'
    $stubCommon = @(
        '-std=gnu11', '-Wall', '-Wextra', '-Werror', '-Os',
        '-ffunction-sections', '-fdata-sections', '-march=armv5te',
        '-mtune=arm946e-s', '-mthumb', '-D__NDS__', '-DARM9',
        '-D_LANGUAGE_C', '-DSSB64_TARGET_NDS',
        ('-I' + (Join-Path $temp 'stubs')),
        ('-I' + (Join-Path $repoRoot 'include')),
        '-IC:/devkitPro/libnds/include', '-IC:/devkitPro/calico/include',
        '-c', $source
    )
    & $ArmCC @stubCommon '-o' $stubObject
    Assert-Condition ($LASTEXITCODE -eq 0) `
        'Water draw failed its stubbed ARM call-surface gate.'

    $normalUndefined = Get-UndefinedSymbols $armNm $normalObject
    Assert-ExactMultiset $normalUndefined @(
        'gNdsPupupuWaterResidencyPrepared',
        'ndsPupupuWaterResidencyBindPrimary',
        'ndsPupupuWaterResidencyBindSecondary',
        'ndsPupupuWaterTiledGetAssets'
    ) 'Water draw production undefined symbols'
    $stubUndefined = Get-UndefinedSymbols $armNm $stubObject
    Assert-ExactMultiset $stubUndefined @(
        'gNdsPupupuWaterResidencyPrepared', 'glBegin', 'glEnd',
        'glTexCoord2t16', 'glVertex3v16',
        'ndsPupupuWaterResidencyBindPrimary',
        'ndsPupupuWaterResidencyBindSecondary',
        'ndsPupupuWaterTiledGetAssets'
    ) 'Water draw stubbed undefined symbols'

    $normalRelocations = @(& $armObjdump -r $normalObject)
    Assert-Condition ($LASTEXITCODE -eq 0) `
        'Unable to inspect water-draw production relocations.'
    Assert-ExactMultiset @(Get-CallRelocations $normalRelocations `
        '.text.ndsPupupuWaterDrawPrepared') @(
        'ndsPupupuWaterDrawAtlas', 'ndsPupupuWaterDrawAtlas',
        'ndsPupupuWaterResidencyBindPrimary',
        'ndsPupupuWaterResidencyBindSecondary',
        'ndsPupupuWaterTiledGetAssets'
    ) 'Water draw prepared call relocations'

    $stubRelocations = @(& $armObjdump -r $stubObject)
    Assert-Condition ($LASTEXITCODE -eq 0) `
        'Unable to inspect water-draw stub relocations.'
    Assert-ExactMultiset @(Get-CallRelocations $stubRelocations `
        '.text.ndsPupupuWaterDrawAtlas') @(
        'glBegin', 'glEnd',
        'glTexCoord2t16', 'glTexCoord2t16', 'glTexCoord2t16',
        'glVertex3v16', 'glVertex3v16', 'glVertex3v16'
    ) 'Water draw exact stubbed GX call sites'

    $sizeOutput = @(& $armSize -A $normalObject)
    Assert-Condition ($LASTEXITCODE -eq 0) `
        'Unable to inspect water-draw ARM size.'
    $totals = Get-ObjectTotals $sizeOutput
    Assert-Condition (($totals.Text -eq 840) -and
        ($totals.Rodata -eq 0) -and ($totals.Data -eq 0) -and
        ($totals.Bss -eq 36)) `
        "Water draw ARM footprint changed: text=$($totals.Text) rodata=$($totals.Rodata) data=$($totals.Data) bss=$($totals.Bss)."
    $stackFiles = @(Get-ChildItem -LiteralPath $temp -Filter '*.su' -File)
    Assert-Condition ($stackFiles.Count -eq 1) `
        'Water draw ARM build did not emit one production stack report.'
    $stackBytes = @()
    foreach ($line in Get-Content -LiteralPath $stackFiles[0].FullName) {
        $match = [regex]::Match($line, '\t(\d+)\t')
        if ($match.Success) { $stackBytes += [int]$match.Groups[1].Value }
    }
    Assert-Condition ($stackBytes.Count -gt 0) `
        'Water draw stack usage could not be parsed.'
    $maxStack = ($stackBytes | Measure-Object -Maximum).Maximum
    Assert-Condition ($maxStack -le 64) `
        "Water draw stack usage is $maxStack bytes, over 64."
}
finally {
    $resolvedTemp = [IO.Path]::GetFullPath($temp)
    Assert-Condition ($resolvedTemp.StartsWith(
        $tempBase, [StringComparison]::OrdinalIgnoreCase)) `
        'Refusing to remove water-draw output outside system temp.'
    if (Test-Path -LiteralPath $resolvedTemp) {
        Remove-Item -LiteralPath $resolvedTemp -Recurse -Force
    }
}

Write-Output (
    'M4_PUPUPU_WATER_DRAW_ARM_OK ' +
    "cells=68 plan_vertices=274 triangles=$triangleCount vertices=$emittedVertices " +
    "source_order=60+8 fraction_mismatches=0 primary_frames=216 secondary_frames=$secondaryFrames " +
    "primary_bind_sites=1 secondary_bind_sites=1 begin_end_sites=1 " +
    "runtime_io_calls=0 runtime_alloc_calls=0 runtime_upload_calls=0 " +
    "text=$($totals.Text) rodata=$($totals.Rodata) bss=$($totals.Bss) " +
    "max_stack_bytes=$maxStack production_linked=0 draw_proven=0"
)
