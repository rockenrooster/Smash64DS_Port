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

$repoRoot = Split-Path -Parent $PSScriptRoot
$generatedRelative = 'src/nds/nds_native_stage_owner.generated.inc'
$generated = Join-Path $repoRoot $generatedRelative
$generator = Join-Path $PSScriptRoot 'generate_nds_native_stage.py'
$hostChecker = Join-Path $PSScriptRoot 'check_nds_native_stage.py'
$armSize = Join-Path (Split-Path -Parent $ArmCC) 'arm-none-eabi-size.exe'
$armNm = Join-Path (Split-Path -Parent $ArmCC) 'arm-none-eabi-nm.exe'

Assert-Condition (Test-Path -LiteralPath $generated -PathType Leaf) `
    'Generated whole-stage packet is absent.'
Assert-Condition (Test-Path -LiteralPath $generator -PathType Leaf) `
    'Whole-stage packet generator is absent.'
Assert-Condition (Test-Path -LiteralPath $hostChecker -PathType Leaf) `
    'Whole-stage exact checker is absent.'
Assert-Condition (Test-Path -LiteralPath $ArmCC -PathType Leaf) `
    "devkitARM GCC is absent: $ArmCC"
Assert-Condition (Test-Path -LiteralPath $armSize -PathType Leaf) `
    "devkitARM size tool is absent: $armSize"
Assert-Condition (Test-Path -LiteralPath $armNm -PathType Leaf) `
    "devkitARM nm tool is absent: $armNm"

$oldNoBytecode = $env:PYTHONDONTWRITEBYTECODE
$env:PYTHONDONTWRITEBYTECODE = '1'
try {
    & python -B $generator --repo-root $repoRoot --check
    Assert-Condition ($LASTEXITCODE -eq 0) `
        'Whole-stage generated include is not exact/current.'
    & python -B $hostChecker
    Assert-Condition ($LASTEXITCODE -eq 0) `
        'Whole-stage host exact checker failed.'
}
finally {
    $env:PYTHONDONTWRITEBYTECODE = $oldNoBytecode
}

# Report the source-link state without blocking the upcoming runtime seam.  The
# generated global ABI symbol makes duplicate inclusion a link-time failure.
$liveReferencePaths = @(
    Get-ChildItem -LiteralPath (Join-Path $repoRoot 'src'), `
        (Join-Path $repoRoot 'include') -Recurse -File |
        Where-Object { $_.Extension -in @('.c', '.h', '.cpp', '.cc', '.s', '.S') } |
        Select-String -SimpleMatch 'nds_native_stage_owner.generated.inc' |
        Select-Object -ExpandProperty Path -Unique
)
Assert-Condition ($liveReferencePaths.Count -le 1) `
    'Whole-stage packet is included by more than one production source/header.'
$productionLinked = if ($liveReferencePaths.Count -eq 1) { 1 } else { 0 }

$tempBase = [IO.Path]::GetFullPath($env:TEMP).TrimEnd('\') + '\'
$temp = [IO.Path]::GetFullPath((Join-Path $env:TEMP `
    ('smash64ds-m3-stage-arm-' + [guid]::NewGuid().ToString('N'))))
Assert-Condition ($temp.StartsWith(
    $tempBase, [StringComparison]::OrdinalIgnoreCase)) `
    'Refusing to create ARM qualification output outside the system temp directory.'
New-Item -ItemType Directory -Path $temp | Out-Null

$textBytes = 0L
$rodataBytes = 0L
$dataBytes = 0L
$bssBytes = 0L
try {
    $translationUnit = Join-Path $temp 'nds_native_stage_qualification.c'
    $object = Join-Path $temp 'nds_native_stage_qualification.o'
    $includeForC = $generated.Replace('\', '/')
    $source = @"
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef signed short s16;

#define static static __attribute__((used))
#include "$includeForC"
#undef static
"@
    [IO.File]::WriteAllText(
        $translationUnit,
        $source,
        [Text.UTF8Encoding]::new($false)
    )

    $compileArgs = @(
        '-std=gnu11',
        '-Wall',
        '-Wextra',
        '-Werror',
        '-Os',
        '-ffreestanding',
        '-ffunction-sections',
        '-fdata-sections',
        '-march=armv5te',
        '-mtune=arm946e-s',
        '-mthumb',
        '-DARM9',
        '-D_LANGUAGE_C',
        '-DSSB64_TARGET_NDS',
        '-c',
        $translationUnit,
        '-o',
        $object
    )
    & $ArmCC @compileArgs
    Assert-Condition ($LASTEXITCODE -eq 0) `
        'Whole-stage packet failed the temporary ARM946E-S compile gate.'

    $nmOutput = & $armNm -g --defined-only $object
    Assert-Condition ($LASTEXITCODE -eq 0) `
        'Unable to inspect whole-stage production packet symbols.'
    $packetSymbols = @(
        $nmOutput | Where-Object {
            $_ -match '\b[Rr]\s+gNdsNativeStageProductionPacketABI$'
        }
    )
    Assert-Condition ($packetSymbols.Count -eq 1) `
        'Whole-stage production ABI symbol is absent or not read-only.'

    $sizeOutput = & $armSize -A $object
    Assert-Condition ($LASTEXITCODE -eq 0) `
        'Unable to inspect the temporary whole-stage ARM object.'
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

    Assert-Condition ($textBytes -eq 0) `
        "Whole-stage qualification object gained $textBytes text bytes."
    Assert-Condition ($rodataBytes -eq 12663) `
        "Whole-stage qualification rodata is $rodataBytes bytes, expected 12663."
    Assert-Condition ($rodataBytes -le (16 * 1024)) `
        "Whole-stage qualification slab is $rodataBytes bytes, over 16 KiB."
    Assert-Condition ($dataBytes -eq 0) `
        "Whole-stage qualification object gained $dataBytes writable data bytes."
    Assert-Condition ($bssBytes -eq 0) `
        "Whole-stage qualification object gained $bssBytes BSS bytes."
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
    'M3_NATIVE_STAGE_ARM_OK ' +
    "text_bytes=$textBytes rodata_bytes=$rodataBytes " +
    "data_bytes=$dataBytes bss_bytes=$bssBytes packet_symbol=1 " +
    "production_linked=$productionLinked"
)
