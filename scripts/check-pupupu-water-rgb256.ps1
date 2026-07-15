param(
    [string]$SourceRoot = (Join-Path $PSScriptRoot '..'),
    [string]$Python = 'python',
    [string]$HostCC = '',
    [string]$ArmCC = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gcc.exe'
)

$ErrorActionPreference = 'Stop'

function Invoke-Checked {
    param(
        [Parameter(Mandatory = $true)][string]$Program,
        [Parameter(Mandatory = $true)][string[]]$Arguments
    )

    & $Program @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "$Program failed with exit code $LASTEXITCODE"
    }
}

function Get-ExactFileContract {
    param([Parameter(Mandatory = $true)][string]$Path)

    return [pscustomobject]@{
        Bytes = (Get-Item -LiteralPath $Path).Length
        Sha256 = (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant()
    }
}

function Get-ArmSymbolSize {
    param(
        [Parameter(Mandatory = $true)][string[]]$Lines,
        [Parameter(Mandatory = $true)][string]$Name
    )

    foreach ($line in $Lines) {
        if ($line -match ('^[0-9a-fA-F]+\s+([0-9a-fA-F]+)\s+[Tt]\s+' + [regex]::Escape($Name) + '$')) {
            return [Convert]::ToInt32($Matches[1], 16)
        }
    }
    throw "ARM9 object lost code symbol $Name"
}

function Get-ArmInstructionCount {
    param(
        [Parameter(Mandatory = $true)][string]$Objdump,
        [Parameter(Mandatory = $true)][string]$Object,
        [Parameter(Mandatory = $true)][string]$Symbol
    )

    $section = ".text.$Symbol"
    $lines = & $Objdump '-d' '-j' $section $Object
    if ($LASTEXITCODE -ne 0) {
        throw "Unable to disassemble ARM9 section $section"
    }
    return @($lines | Where-Object {
        $_ -match '^\s*[0-9a-fA-F]+:\s+[0-9a-fA-F]{8}\s+'
    }).Count
}

$sourceRootPath = (Resolve-Path $SourceRoot).Path
$generator = Join-Path $PSScriptRoot 'generate_pupupu_water_rgb256_microbench.py'
$microbench = Join-Path $PSScriptRoot 'pupupu_water_rgb256_microbench.c'
$fixturePath = Join-Path $PSScriptRoot 'fixtures\pupupu_water_rgb256_expected.json'

foreach ($required in @($generator, $microbench, $fixturePath)) {
    if (-not (Test-Path -LiteralPath $required -PathType Leaf)) {
        throw "Required RGB256 microbenchmark input is absent: $required"
    }
}
if ([string]::IsNullOrWhiteSpace($HostCC)) {
    $hostCommand = Get-Command gcc.exe -ErrorAction SilentlyContinue
    if ($null -eq $hostCommand) {
        $hostCommand = Get-Command clang.exe -ErrorAction SilentlyContinue
    }
    if ($null -eq $hostCommand) {
        throw 'No host C compiler (gcc.exe or clang.exe) is available.'
    }
    $HostCC = $hostCommand.Source
}
if (-not (Test-Path -LiteralPath $ArmCC -PathType Leaf)) {
    throw "ARM9 compiler not found: $ArmCC"
}

$armBin = Split-Path -Parent $ArmCC
$armNm = Join-Path $armBin 'arm-none-eabi-nm.exe'
$armSize = Join-Path $armBin 'arm-none-eabi-size.exe'
$armObjdump = Join-Path $armBin 'arm-none-eabi-objdump.exe'
foreach ($tool in @($armNm, $armSize, $armObjdump)) {
    if (-not (Test-Path -LiteralPath $tool -PathType Leaf)) {
        throw "ARM9 inspection tool not found: $tool"
    }
}

$fixture = Get-Content -LiteralPath $fixturePath -Raw | ConvertFrom-Json
if (($fixture.parity.index_byte_mismatches -ne 0) -or
    ($fixture.parity.alpha_mismatches -ne 0) -or
    ($fixture.parity.opaque_rgb555_mismatches -ne 0) -or
    ($fixture.parity.visible_pixel_mismatches -ne 0)) {
    throw 'Pinned RGB256 fixture no longer has exact hardware-visible parity.'
}
if ($fixture.source.oracle_pixels -ne 3024896) {
    throw "Pinned RGB256 oracle count changed: $($fixture.source.oracle_pixels)"
}

$tempBase = [System.IO.Path]::GetFullPath([System.IO.Path]::GetTempPath())
$tempDir = [System.IO.Path]::GetFullPath(
    (Join-Path $tempBase ('smash64ds-pupupu-rgb256-' + [guid]::NewGuid()))
)
if ((-not $tempDir.StartsWith($tempBase, [System.StringComparison]::OrdinalIgnoreCase)) -or
    (-not ([System.IO.Path]::GetFileName($tempDir)).StartsWith('smash64ds-pupupu-rgb256-'))) {
    throw "Refusing temporary output outside the guarded temp prefix: $tempDir"
}
[void](New-Item -ItemType Directory -Path $tempDir)

$header = Join-Path $tempDir 'pupupu_water_rgb256_corpus.generated.h'
$hostSource = Join-Path $tempDir 'pupupu_water_rgb256_microbench.c'
$expectedMap = Join-Path $tempDir 'expected-map.bin'
$expectedPalette = Join-Path $tempDir 'expected-palette.bin'
$actualMap = Join-Path $tempDir 'actual-map.bin'
$actualPalette = Join-Path $tempDir 'actual-palette.bin'
$hostExe = Join-Path $tempDir 'pupupu-water-rgb256-host.exe'
$armObject = Join-Path $tempDir 'pupupu-water-rgb256-arm9.o'

try {
    Invoke-Checked -Program $Python -Arguments @(
        '-B',
        $generator,
        '--repo-root', $sourceRootPath,
        '--fixture', $fixturePath,
        '--emit-header', $header,
        '--emit-map-oracle', $expectedMap,
        '--emit-palette-oracle', $expectedPalette
    )
    Copy-Item -LiteralPath $microbench -Destination $hostSource

    Invoke-Checked -Program $HostCC -Arguments @(
        '-std=c11',
        '-O3',
        '-Wall',
        '-Wextra',
        '-Werror',
        '-fno-strict-aliasing',
        '-DPUPUPU_WATER_RGB256_HOST_MAIN=1',
        '-I', $tempDir,
        $hostSource,
        '-o', $hostExe
    )
    $hostOutput = & $hostExe $actualMap $actualPalette
    if ($LASTEXITCODE -ne 0) {
        throw "Host RGB256 microbenchmark failed with exit code $LASTEXITCODE"
    }
    $hostOutput | ForEach-Object { Write-Output $_ }
    $hostParity = $hostOutput | Where-Object {
        $_ -match '^PUPUPU_WATER_RGB256_HOST_PARITY_OK '
    } | Select-Object -First 1
    if ($null -eq $hostParity) {
        throw 'Host RGB256 microbenchmark omitted its parity contract.'
    }
    if ($hostParity -notmatch 'scratch_bytes=(\d+)') {
        throw 'Host RGB256 microbenchmark omitted scratch size.'
    }
    $scratchBytes = [int]$Matches[1]

    $expectedMapContract = Get-ExactFileContract $expectedMap
    $actualMapContract = Get-ExactFileContract $actualMap
    $expectedPaletteContract = Get-ExactFileContract $expectedPalette
    $actualPaletteContract = Get-ExactFileContract $actualPalette
    if (($expectedMapContract.Bytes -ne $actualMapContract.Bytes) -or
        ($expectedMapContract.Sha256 -ne $actualMapContract.Sha256)) {
        throw 'Host RGB256 map bytes differ from the 3,024,896-byte oracle.'
    }
    if (($expectedPaletteContract.Bytes -ne $actualPaletteContract.Bytes) -or
        ($expectedPaletteContract.Sha256 -ne $actualPaletteContract.Sha256)) {
        throw 'Host RGB256 palette bytes differ from the per-case oracle.'
    }
    if (($actualMapContract.Sha256 -ne $fixture.source.map_oracle_sha256) -or
        ($actualPaletteContract.Sha256 -ne $fixture.source.palette_oracle_sha256)) {
        throw 'Host RGB256 output hashes differ from the pinned fixture.'
    }
    Write-Output ((
        'PUPUPU_WATER_RGB256_EXACT_BYTES_OK map_bytes={0} map_sha256={1} ' +
        'palette_bytes={2} palette_sha256={3}'
    ) -f $actualMapContract.Bytes, $actualMapContract.Sha256,
        $actualPaletteContract.Bytes, $actualPaletteContract.Sha256)

    Invoke-Checked -Program $ArmCC -Arguments @(
        '-std=c11',
        '-O3',
        '-mcpu=arm946e-s',
        '-marm',
        '-ffreestanding',
        '-fno-builtin',
        '-fno-strict-aliasing',
        '-ffunction-sections',
        '-fdata-sections',
        '-fstack-usage',
        '-Wall',
        '-Wextra',
        '-Werror',
        '-I', $tempDir,
        '-c', $hostSource,
        '-o', $armObject
    )

    $symbols = @(& $armNm '-S' '--size-sort' $armObject)
    if ($LASTEXITCODE -ne 0) {
        throw 'Unable to inspect ARM9 RGB256 symbols.'
    }
    $undefined = @(& $armNm '-u' $armObject | Where-Object {
        -not [string]::IsNullOrWhiteSpace($_)
    })
    if ($LASTEXITCODE -ne 0) {
        throw 'Unable to inspect ARM9 RGB256 undefined symbols.'
    }
    if ($undefined.Count -ne 0) {
        throw "ARM9 RGB256 object has external linkage: $($undefined -join ', ')"
    }
    $symbolText = $symbols -join "`n"
    if ($symbolText -match '(?i)(\bgl[A-Z_]|\bGX_|\bGFX_|\bdma|\bvideo)') {
        throw 'ARM9 RGB256 object unexpectedly references GX/video/DMA symbols.'
    }

    $coreSymbols = @(
        'pupupuWaterRgb256BuildAxis',
        'smash64dsPupupuWaterRgb256Prepare',
        'smash64dsPupupuWaterRgb256ExpandCi4',
        'smash64dsPupupuWaterRgb256Generate'
    )
    $coreCodeBytes = 0
    $coreInstructions = 0
    foreach ($symbol in $coreSymbols) {
        $coreCodeBytes += Get-ArmSymbolSize -Lines $symbols -Name $symbol
        $coreInstructions += Get-ArmInstructionCount `
            -Objdump $armObjdump -Object $armObject -Symbol $symbol
    }
    $microbenchCodeBytes = Get-ArmSymbolSize `
        -Lines $symbols -Name 'smash64dsPupupuWaterRgb256Microbench'
    $metadataBytes =
        [int]$fixture.production_footprint.new_constant_pair_index_bytes +
        [int]$fixture.production_footprint.new_constant_alpha_prefix_bytes
    $stackFiles = @(Get-ChildItem -LiteralPath $tempDir -Filter '*.su' -File)
    if ($stackFiles.Count -eq 0) {
        throw 'ARM9 RGB256 build did not emit stack-usage evidence.'
    }
    $maxStackBytes = 0
    foreach ($stackFile in $stackFiles) {
        foreach ($line in Get-Content -LiteralPath $stackFile.FullName) {
            if ($line -match '\t(\d+)\t') {
                $maxStackBytes = [Math]::Max($maxStackBytes, [int]$Matches[1])
            }
        }
    }
    if ($maxStackBytes -eq 0) {
        throw 'ARM9 RGB256 stack-usage evidence could not be parsed.'
    }
    if ($scratchBytes -gt 8192) {
        throw "RGB256 independent scratch exceeded 8 KiB: $scratchBytes"
    }
    if ($maxStackBytes -gt 256) {
        throw "RGB256 ARM9 stack frame exceeded 256 bytes: $maxStackBytes"
    }
    if (($coreCodeBytes + $metadataBytes) -gt 4096) {
        throw "RGB256 production code/metadata exceeded 4 KiB: $($coreCodeBytes + $metadataBytes)"
    }
    $sizeOutput = @(& $armSize '-A' $armObject)
    if ($LASTEXITCODE -ne 0) {
        throw 'Unable to inspect ARM9 RGB256 section sizes.'
    }
    $sizeOutput | ForEach-Object { Write-Output $_ }
    Write-Output ((
        'PUPUPU_WATER_RGB256_ARM946E_S_NO_GX_OK core_code_bytes={0} ' +
        'core_instructions={1} metadata_bytes={2} independent_scratch_bytes={3} ' +
        'max_stack_bytes={4} microbench_code_bytes={5} undefined_symbols=0'
    ) -f $coreCodeBytes, $coreInstructions, $metadataBytes, $scratchBytes,
        $maxStackBytes, $microbenchCodeBytes)

    $baselineMedian = [int]$fixture.comparison_to_retained_rgb16.rgb16_convert_plus_stage_median_ticks
    $baselineP95 = [int]$fixture.comparison_to_retained_rgb16.rgb16_convert_plus_stage_p95_ticks
    $candidateMedian = [int]$fixture.comparison_to_retained_rgb16.rgb256_conservative_active_median_estimated_cycles
    $candidateWorst = [int]$fixture.comparison_to_retained_rgb16.rgb256_conservative_worst_estimated_cycles
    $baselineUpload = [int]$fixture.comparison_to_retained_rgb16.rgb16_worst_full_map_upload_bytes
    $candidateUpload = [int]$fixture.comparison_to_retained_rgb16.rgb256_worst_map_plus_palette_upload_bytes
    $medianDelta = $candidateMedian - $baselineMedian
    $worstDelta = $candidateWorst - $baselineP95
    $uploadSaved = $baselineUpload - $candidateUpload
    $recommendation = (
        ($candidateMedian -le $baselineMedian) -and
        ($candidateWorst -le [Math]::Floor($baselineP95 * 1.10)) -and
        ($candidateUpload -le [Math]::Ceiling($baselineUpload * 0.51))
    )
    $decision = if ($recommendation) {
        'JUSTIFIED_NARROW_8_FRAME_FALSIFIER'
    }
    else {
        'NO_GO_RUNTIME_FALSIFIER'
    }
    Write-Output ((
        'PUPUPU_WATER_RGB256_STATIC_ESTIMATE map_large_median={0} ' +
        'map_large_worst={1} map_small_median={2} map_small_worst={3} ' +
        'palette={4} cold_decode_per_source={5} active_frame_median={6} ' +
        'active_frame_worst={7}'
    ) -f $fixture.arm_static_estimator.owners.large.median_cycles,
        $fixture.arm_static_estimator.owners.large.worst_cycles,
        $fixture.arm_static_estimator.owners.small.median_cycles,
        $fixture.arm_static_estimator.owners.small.worst_cycles,
        $fixture.arm_static_estimator.palette_prepare_cycles,
        $fixture.arm_static_estimator.cold_source_decode_cycles_per_texture,
        $candidateMedian, $candidateWorst)
    Write-Output ((
        'PUPUPU_WATER_RGB256_UPLOAD_MODEL conservative_cycle_bytes={0} ' +
        'conservative_average_bytes_per_frame={1} visible_dedup_cycle_bytes={2} ' +
        'visible_dedup_average_bytes_per_frame={3} peak_bytes={4}'
    ) -f $fixture.cycle_update_model.conservative_key_change.bytes.cycle_total,
        $fixture.cycle_update_model.conservative_key_change.bytes.average_per_all_frames,
        $fixture.cycle_update_model.visible_map_dedup.bytes.cycle_total,
        $fixture.cycle_update_model.visible_map_dedup.bytes.average_per_all_frames,
        $candidateUpload)
    Write-Output ((
        'PUPUPU_WATER_RGB256_FALSIFIER_DECISION result={0} milestone4_complete=NO ' +
        'median_delta_vs_retained={1} worst_delta_vs_retained_p95={2} ' +
        'peak_upload_bytes_saved={3} palette_vram_mapping=STILL_REQUIRED ' +
        'timing_evidence=STATIC_NOT_DEVICE'
    ) -f $decision, $medianDelta, $worstDelta, $uploadSaved)
}
finally {
    if ((Test-Path -LiteralPath $tempDir) -and
        $tempDir.StartsWith($tempBase, [System.StringComparison]::OrdinalIgnoreCase) -and
        ([System.IO.Path]::GetFileName($tempDir)).StartsWith('smash64ds-pupupu-rgb256-')) {
        Remove-Item -LiteralPath $tempDir -Recurse -Force
    }
}
