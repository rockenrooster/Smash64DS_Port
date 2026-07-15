param(
    [string]$ArmCC = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gcc.exe'
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

function Get-CFunctionBody {
    param(
        [string]$Text,
        [string]$Name
    )

    $signature = [regex]::Match(
        $Text,
        ('(?m)^\s*(?:s32|void)\s+' + [regex]::Escape($Name) + '\s*\(')
    )
    Assert-Condition $signature.Success "Function '$Name' is absent."
    $open = $Text.IndexOf('{', $signature.Index)
    Assert-Condition ($open -ge 0) "Function '$Name' has no body."
    $depth = 0
    for ($index = $open; $index -lt $Text.Length; $index++) {
        if ($Text[$index] -eq '{') {
            $depth++
        }
        elseif ($Text[$index] -eq '}') {
            $depth--
            if ($depth -eq 0) {
                return $Text.Substring($signature.Index,
                    $index - $signature.Index + 1)
            }
        }
    }
    throw "Function '$Name' has an unterminated body."
}

function Get-ObjectTotals {
    param(
        [string[]]$SizeOutput
    )

    $totals = [ordered]@{
        Text = 0L
        Rodata = 0L
        Data = 0L
        Bss = 0L
    }
    foreach ($line in $SizeOutput) {
        $match = [regex]::Match($line, '^\s*(\.\S+)\s+(\d+)\s+\S+\s*$')
        if (-not $match.Success) {
            continue
        }
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
    param(
        [string]$Nm,
        [string]$Object
    )

    $output = @(& $Nm -u $Object)
    Assert-Condition ($LASTEXITCODE -eq 0) `
        "Unable to inspect undefined symbols in '$Object'."
    return @($output | ForEach-Object {
        $match = [regex]::Match($_, '^\s+U\s+(\S+)\s*$')
        Assert-Condition $match.Success "Unparsed nm line: $_"
        $match.Groups[1].Value
    } | Sort-Object -Unique)
}

function Assert-ExactSet {
    param(
        [string[]]$Actual,
        [string[]]$Expected,
        [string]$Label
    )

    $actualSorted = @($Actual | Sort-Object -Unique)
    $expectedSorted = @($Expected | Sort-Object -Unique)
    Assert-Condition (($actualSorted -join "`n") -ceq
        ($expectedSorted -join "`n")) `
        ("$Label changed. actual=[{0}] expected=[{1}]" -f
            ($actualSorted -join ', '), ($expectedSorted -join ', '))
}

function Get-CallRelocations {
    param(
        [string[]]$ObjdumpOutput,
        [string]$Section
    )

    $active = $false
    $calls = @()
    foreach ($line in $ObjdumpOutput) {
        $header = [regex]::Match(
            $line,
            '^RELOCATION RECORDS FOR \[(\.\S+)\]:\s*$'
        )
        if ($header.Success) {
            $active = ($header.Groups[1].Value -ceq $Section)
            continue
        }
        if (-not $active) {
            continue
        }
        $relocation = [regex]::Match(
            $line,
            '^\s*[0-9a-fA-F]+\s+R_ARM_(?:THM_)?CALL\s+(\S+)\s*$'
        )
        if ($relocation.Success) {
            $calls += $relocation.Groups[1].Value
        }
    }
    return @($calls)
}

$repoRoot = Split-Path -Parent $PSScriptRoot
$header = Join-Path $repoRoot 'include/nds/nds_pupupu_water_residency.h'
$source = Join-Path $repoRoot 'src/nds/nds_pupupu_water_residency.c'
$payload = Join-Path $repoRoot 'assets/renderer/pupupu_water_tiled_aot.bin'
$makefile = Join-Path $repoRoot 'Makefile'
$armBin = Split-Path -Parent $ArmCC
$armSize = Join-Path $armBin 'arm-none-eabi-size.exe'
$armNm = Join-Path $armBin 'arm-none-eabi-nm.exe'
$armObjdump = Join-Path $armBin 'arm-none-eabi-objdump.exe'

foreach ($required in @(
    $header, $source, $payload, $makefile, $ArmCC, $armSize, $armNm,
    $armObjdump
)) {
    Assert-Condition (Test-Path -LiteralPath $required -PathType Leaf) `
        "Required M4 residency input is absent: $required"
}

$headerText = Get-Content -LiteralPath $header -Raw
$sourceText = Get-Content -LiteralPath $source -Raw
$makeText = Get-Content -LiteralPath $makefile -Raw
$payloadFile = Get-Item -LiteralPath $payload
$payloadHash = (Get-FileHash -LiteralPath $payload -Algorithm SHA256).Hash.
    ToLowerInvariant()

Assert-Condition ($payloadFile.Length -eq 167936) `
    "M4 residency payload is $($payloadFile.Length) bytes, expected 167936."
Assert-Condition ($payloadHash -ceq
    'af052624915c87205bfbff8e0db1e3365d3d7a30758fba74b32fdaf39c364982') `
    "M4 residency payload SHA256 changed: $payloadHash"
Assert-Condition ($makeText.Contains(
    'NDS_RENDERER_M4_WATER_TILED_AOT ?= 0')) `
    'M4 water selector is no longer default-off.'
Assert-Condition (-not $makeText.Contains(
    'override NDS_RENDERER_M4_WATER_TILED_AOT := 1')) `
    'A named or published target force-enabled unqualified M4 water.'
$labLinkBlock = [regex]::Match(
    $makeText,
    '(?ms)^ifeq \(\$\(NDS_RENDERER_M4_WATER_TILED_AOT\),1\)\s*$' +
    '.*?^endif\s*$'
)
Assert-Condition ($labLinkBlock.Success -and $labLinkBlock.Value.Contains(
    'nds_pupupu_water_residency.c')) `
    'M4 lab selector no longer links the residency module.'
Assert-Condition (([regex]::Matches(
    $makeText,
    '(?m)\bnds_pupupu_water_residency\.c\b'
)).Count -eq 1) `
    'M4 residency module escaped its one explicit lab link block.'
$productionLinked = 0
$labLinked = 1

$payloadBytes = [IO.File]::ReadAllBytes($payload)
$primaryZeroBytes = 0
for ($index = 0; $index -lt 131072; $index++) {
    if ($payloadBytes[$index] -eq 0) {
        $primaryZeroBytes++
    }
}
$usedMaskedBytes = 131072
$usedMaskedZeroBytes = $primaryZeroBytes
$usedMaskedValues = [Collections.Generic.HashSet[int]]::new()
for ($index = 0; $index -lt 131072; $index++) {
    [void]$usedMaskedValues.Add([int]$payloadBytes[$index])
}
for ($tile = 0; $tile -lt 60; $tile++) {
    $tileX = ($tile % 8) * 32
    $tileY = [Math]::Floor($tile / 8) * 8
    for ($y = 0; $y -lt 8; $y++) {
        $row = 131072 + (($tileY + $y) * 256) + $tileX
        for ($x = 0; $x -lt 32; $x++) {
            $value = [int]$payloadBytes[$row + $x]
            $usedMaskedBytes++
            if ($value -eq 0) {
                $usedMaskedZeroBytes++
            }
            [void]$usedMaskedValues.Add($value)
        }
    }
}
Assert-Condition ($usedMaskedBytes -eq 146432) `
    "M4 masked-tile audit covered $usedMaskedBytes bytes, expected 146432."
Assert-Condition ($usedMaskedZeroBytes -eq 39772) `
    "M4 masked tiles have $usedMaskedZeroBytes transparent texels, expected 39772."
Assert-Condition ($usedMaskedValues.Count -eq 91) `
    "M4 masked tiles use $($usedMaskedValues.Count) indices, expected 91 including zero."
$secondaryZeroBytes = 0
for ($index = 131072; $index -lt 147456; $index++) {
    if ($payloadBytes[$index] -eq 0) { $secondaryZeroBytes++ }
}
$secondaryPaddingZeroBytes = $secondaryZeroBytes -
    ($usedMaskedZeroBytes - $primaryZeroBytes)
Assert-Condition ($secondaryPaddingZeroBytes -eq 1024) `
    "M4 secondary atlas has $secondaryPaddingZeroBytes zero padding bytes, expected 1024."

$requiredHeaderFragments = @(
    '#define NDS_RENDERER_M4_WATER_TILED_AOT 0',
    '#define NDS_PUPUPU_WATER_RESIDENCY_TEXTURE_COUNT 2u',
    '#define NDS_PUPUPU_WATER_RESIDENCY_PALETTE_COUNT 40u',
    '#define NDS_PUPUPU_WATER_RESIDENCY_RESOURCE_COUNT 42u',
    '#define NDS_PUPUPU_WATER_RESIDENCY_TEXTURE_BYTES 147456u',
    '#define NDS_PUPUPU_WATER_RESIDENCY_PALETTE_BYTES 20480u',
    '#define NDS_PUPUPU_WATER_RESIDENCY_SCRATCH_BYTES 4096u'
)
foreach ($fragment in $requiredHeaderFragments) {
    Assert-Condition ($headerText.Contains($fragment)) `
        "M4 residency contract lost: $fragment"
}

$apiMatches = @([regex]::Matches(
    $headerText,
    '(?m)^\s*(?:s32|void)\s+(ndsPupupuWaterResidency\w+)\s*\('
))
$apis = @($apiMatches | ForEach-Object { $_.Groups[1].Value })
Assert-ExactSet $apis @(
    'ndsPupupuWaterResidencyBindPrimary',
    'ndsPupupuWaterResidencyBindSecondary',
    'ndsPupupuWaterResidencyPrepare',
    'ndsPupupuWaterResidencyReset'
) 'M4 residency public API'
Assert-Condition (-not [regex]::IsMatch(
    $headerText,
    '(?i)(get.*(?:texture|palette)|(?:texture|palette).*(?:pointer|name|id))')) `
    'M4 residency API exposes raw texture/palette resource identity.'

$requiredSourceFragments = @(
    '#define NDS_PUPUPU_WATER_PAYLOAD_FNV1A32 0x11bdfe52u',
    '#define NDS_PUPUPU_WATER_TEXTURE_VRAM_BASE 0x06800000u',
    '#define NDS_PUPUPU_WATER_TEXTURE_VRAM_END 0x06840000u',
    'GL_RGB256, TEXTURE_SIZE_512, TEXTURE_SIZE_256',
    'GL_RGB256, TEXTURE_SIZE_256, TEXTURE_SIZE_64',
    'Visibility is baked into index zero in all 572 exact masked tiles.',
    'TEXGEN_OFF | GL_TEXTURE_COLOR0_TRANSPARENT',
    'NDS_PUPUPU_WATER_TILED_PRIMARY_ATLAS_BYTES)))',
    '(u32)address * 16u == palette_index * 512u'
)
foreach ($fragment in $requiredSourceFragments) {
    Assert-Condition ($sourceText.Contains($fragment)) `
        "M4 residency implementation lost: $fragment"
}
Assert-Condition ([regex]::IsMatch(
    $sourceText,
    '(?s)GL_RGB256,\s*TEXTURE_SIZE_512,\s*TEXTURE_SIZE_256,\s*' +
    'TEXGEN_OFF\s*\|\s*GL_TEXTURE_COLOR0_TRANSPARENT,\s*' +
    'NDS_PUPUPU_WATER_TILED_PRIMARY_ATLAS_BYTES'
)) 'M4 RGB256 primary atlas lost color-zero transparency.'
Assert-Condition (([regex]::Matches($sourceText, '\bfopen\s*\(')).Count -eq 1) `
    'M4 residency must open the payload exactly once.'
Assert-Condition (-not [regex]::IsMatch($sourceText, '\b(?:fseek|rewind)\s*\(')) `
    'M4 residency payload load is no longer one sequential stream.'

$runtimeForbidden = @(
    'fopen', 'fread', 'fgetc', 'fclose', 'malloc', 'free', 'glGenTextures',
    'glDeleteTextures', 'glTexImage2D', 'glColorTableEXT',
    'glGetColorTableParameterEXT', 'glGetTexturePointer', 'memcpy',
    'vramGetBank', 'vramSetBank', 'glResetTextures'
)
foreach ($name in @(
    'ndsPupupuWaterResidencyBindPrimary',
    'ndsPupupuWaterResidencyBindSecondary'
)) {
    $body = Get-CFunctionBody $sourceText $name
    foreach ($forbidden in $runtimeForbidden) {
        Assert-Condition (-not [regex]::IsMatch(
            $body,
            ('\b' + [regex]::Escape($forbidden) + '\w*\s*\(')
        )) "$name gained gameplay-path operation '$forbidden'."
    }
}

$tempBase = [IO.Path]::GetFullPath($env:TEMP).TrimEnd('\') + '\'
$temp = [IO.Path]::GetFullPath((Join-Path $env:TEMP `
    ('smash64ds-m4-water-residency-' + [guid]::NewGuid().ToString('N'))))
Assert-Condition ($temp.StartsWith(
    $tempBase, [StringComparison]::OrdinalIgnoreCase)) `
    'Refusing to create ARM check output outside system temporary storage.'
New-Item -ItemType Directory -Path $temp | Out-Null

try {
    $offObject = Join-Path $temp 'residency-off.o'
    $onObject = Join-Path $temp 'residency-on.o'
    $common = @(
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
        '-D__NDS__',
        '-DARM9',
        '-D_LANGUAGE_C',
        '-DSSB64_TARGET_NDS',
        ('-I' + (Join-Path $repoRoot 'include')),
        '-IC:/devkitPro/libnds/include',
        '-IC:/devkitPro/calico/include',
        '-fstack-usage',
        '-c',
        $source
    )
    & $ArmCC @common '-o' $offObject
    Assert-Condition ($LASTEXITCODE -eq 0) `
        'Default-off M4 residency failed its ARM946E-S compile gate.'
    & $ArmCC @common '-DNDS_RENDERER_M4_WATER_TILED_AOT=1' '-o' $onObject
    Assert-Condition ($LASTEXITCODE -eq 0) `
        'Enabled M4 residency failed its ARM946E-S compile gate.'

    Assert-ExactSet @(Get-UndefinedSymbols $armNm $offObject) @() `
        'Default-off M4 residency undefined symbols'
    Assert-ExactSet @(Get-UndefinedSymbols $armNm $onObject) @(
        'fclose',
        'fgetc',
        'fopen',
        'fread',
        'free',
        'glAssignColorTable',
        'glBindTexture',
        'glColorTableEXT',
        'glDeleteTextures',
        'glGenTextures',
        'glGetColorTableParameterEXT',
        'glGetTexturePointer',
        'glTexImage2D',
        'malloc',
        'memcpy',
        'memset',
        'vramGetBank',
        'vramRestorePrimaryBanks'
    ) 'Enabled M4 residency undefined symbols'

    $offSize = @(& $armSize -A $offObject)
    Assert-Condition ($LASTEXITCODE -eq 0) `
        'Unable to inspect default-off M4 residency ARM size.'
    $onSize = @(& $armSize -A $onObject)
    Assert-Condition ($LASTEXITCODE -eq 0) `
        'Unable to inspect enabled M4 residency ARM size.'
    $offTotals = Get-ObjectTotals $offSize
    $onTotals = Get-ObjectTotals $onSize
    Assert-Condition ($offTotals.Text -eq 96) `
        "Default-off M4 residency text is $($offTotals.Text), expected 96 bytes."
    Assert-Condition ($offTotals.Rodata -eq 0) `
        "Default-off M4 residency gained $($offTotals.Rodata) rodata bytes."
    Assert-Condition ($offTotals.Data -eq 0) `
        "Default-off M4 residency gained $($offTotals.Data) data bytes."
    Assert-Condition ($offTotals.Bss -eq 76) `
        "Default-off M4 residency BSS is $($offTotals.Bss), expected 76 bytes."
    Assert-Condition ($onTotals.Text -eq 1304) `
        "Enabled M4 residency text is $($onTotals.Text), expected 1304 bytes."
    Assert-Condition ($onTotals.Rodata -eq 46) `
        "Enabled M4 residency rodata is $($onTotals.Rodata), expected 46 bytes."
    Assert-Condition ($onTotals.Data -eq 0) `
        "Enabled M4 residency gained $($onTotals.Data) data bytes."
    Assert-Condition ($onTotals.Bss -eq 248) `
        "Enabled M4 residency BSS is $($onTotals.Bss), expected 248 bytes."

    $objdumpOutput = @(& $armObjdump -r $onObject)
    Assert-Condition ($LASTEXITCODE -eq 0) `
        'Unable to inspect M4 residency ARM relocations.'
    Assert-ExactSet @(Get-CallRelocations $objdumpOutput `
        '.text.ndsPupupuWaterResidencyBindPrimary') @(
        'glAssignColorTable',
        'glBindTexture'
    ) 'Primary-bind ARM call relocations'
    Assert-ExactSet @(Get-CallRelocations $objdumpOutput `
        '.text.ndsPupupuWaterResidencyBindSecondary') @(
        'glAssignColorTable',
        'glBindTexture'
    ) 'Secondary-bind ARM call relocations'

    $stackFiles = @(Get-ChildItem -LiteralPath $temp -Filter '*.su' -File)
    Assert-Condition ($stackFiles.Count -eq 2) `
        'M4 residency ARM build did not emit both stack-usage files.'
    $stackBytes = @()
    foreach ($stackFile in $stackFiles) {
        foreach ($line in Get-Content -LiteralPath $stackFile.FullName) {
            $match = [regex]::Match($line, '\t(\d+)\t')
            if ($match.Success) {
                $stackBytes += [int]$match.Groups[1].Value
            }
        }
    }
    Assert-Condition ($stackBytes.Count -gt 0) `
        'M4 residency stack-usage evidence could not be parsed.'
    $maxStack = ($stackBytes | Measure-Object -Maximum).Maximum
    Assert-Condition ($maxStack -le 64) `
        "M4 residency stack usage is $maxStack bytes, over 64 bytes."
}
finally {
    $resolvedTemp = [IO.Path]::GetFullPath($temp)
    Assert-Condition ($resolvedTemp.StartsWith(
        $tempBase, [StringComparison]::OrdinalIgnoreCase)) `
        'Refusing to remove an ARM check directory outside system temp.'
    if (Test-Path -LiteralPath $resolvedTemp) {
        Remove-Item -LiteralPath $resolvedTemp -Recurse -Force
    }
}

Write-Output (
    'M4_PUPUPU_WATER_RESIDENCY_ARM_OK ' +
    "payload_bytes=$($payloadFile.Length) payload_sha256=$payloadHash " +
    "textures=2 texture_bytes=147456 palettes=40 palette_bytes=20480 " +
    "resources=42 scratch_bytes=4096 primary_address=0x06800000 " +
    "secondary_address=0x06820000 runtime_upload_calls=0 " +
    "runtime_io_calls=0 runtime_alloc_calls=0 max_stack_bytes=$maxStack " +
    "off_text=$($offTotals.Text) on_text=$($onTotals.Text) " +
    "masked_used_bytes=$usedMaskedBytes masked_zero_bytes=$usedMaskedZeroBytes " +
    "masked_indices=$($usedMaskedValues.Count) secondary_padding_zero_bytes=$secondaryPaddingZeroBytes " +
    "production_linked=$productionLinked lab_linked=$labLinked draw_proven=0"
)
