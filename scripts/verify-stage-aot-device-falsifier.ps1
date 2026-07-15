param(
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$GdbPort = 4333,
    [int]$RunnerSlot = -1,
    [switch]$NoBuild,
    [switch]$List,
    [ValidateRange(4,64)][int]$Samples = 8,
    [ValidateRange(0,1000000)][int]$StartFrame = 438,
    [ValidateRange(5,600)][int]$TimeoutSeconds = 90,
    [int]$DelaySeconds = 5
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

. (Join-Path $PSScriptRoot 'lib\build-output.ps1')

$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$target = 'smash64ds-battle-playable-forensic-hwtri'
$build = 'build-stage-aot-device-falsifier-profile2'
$harness = 'battle_playable'
$expectedMode = 163
$profile = 2
$buildPath = Resolve-Smash64DSBuildPath -Root $root -Build $build
$rom = Resolve-Smash64DSBuildOutput `
    -Root $root -Target $target -Build $build -Extension '.nds'
$elf = Resolve-Smash64DSBuildOutput `
    -Root $root -Target $target -Build $build -Extension '.elf'
$exportPath = Join-Path $buildPath 'stage-aot-device-falsifier.json'
$transcriptPath = Join-Path $buildPath 'stage-aot-device-falsifier.log'
$verifier = Join-Path $PSScriptRoot `
    'verify-battle-mariofox-gcrunall-loop-harness.ps1'

function Assert-Condition {
    param([bool]$Condition, [string]$Message)
    if (-not $Condition) { throw $Message }
}

function Test-DescendantPath {
    param([string]$Parent, [string]$Child)
    $parentFull = [System.IO.Path]::GetFullPath($Parent).TrimEnd('\', '/') + `
        [System.IO.Path]::DirectorySeparatorChar
    $childFull = [System.IO.Path]::GetFullPath($Child)
    return $childFull.StartsWith(
        $parentFull,
        [System.StringComparison]::OrdinalIgnoreCase)
}

function Get-FileIdentity {
    param([string]$Path)
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        return [PSCustomObject]@{ Exists = $false; Hash = ''; Bytes = 0 }
    }
    $item = Get-Item -LiteralPath $Path
    return [PSCustomObject]@{
        Exists = $true
        Hash = (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash
        Bytes = [int64]$item.Length
    }
}

function Assert-SameFileIdentity {
    param([string]$Path, [psobject]$Before)
    $after = Get-FileIdentity -Path $Path
    Assert-Condition ($after.Exists -eq $Before.Exists) `
        "Published output existence changed: $Path"
    if ($Before.Exists) {
        Assert-Condition `
            (($after.Hash -eq $Before.Hash) -and ($after.Bytes -eq $Before.Bytes)) `
            "Published output changed during isolated falsifier: $Path"
    }
}

function Get-Int64Rows {
    param([System.Collections.IEnumerable]$Rows)
    $result = @()
    foreach ($row in $Rows) {
        $values = @($row | ForEach-Object { [int64]$_ })
        $result += ,$values
    }
    return $result
}

function Get-FieldValues {
    param([object[]]$Rows, [int]$Index)
    return [int64[]]@($Rows | ForEach-Object { [int64]$_[$Index] })
}

function Get-Churn {
    param([int64[]]$Values)
    if ($Values.Count -eq 0) { return '0/0' }
    $changes = 0
    for ($i = 1; $i -lt $Values.Count; $i++) {
        if ($Values[$i] -ne $Values[$i - 1]) { $changes++ }
    }
    $distinct = @($Values | Sort-Object -Unique).Count
    return "$changes/$distinct"
}

function Assert-SingleValue {
    param([object[]]$Rows, [int]$Index, [int64]$Expected, [string]$Label)
    $values = Get-FieldValues -Rows $Rows -Index $Index
    $bad = @($values | Where-Object { $_ -ne $Expected })
    Assert-Condition ($bad.Count -eq 0) `
        "$Label was not exactly $Expected in every sampled frame: $($values -join ',')"
}

$publishedTargets = @('smash64ds', 'smash64ds-battle-playable-hwtri')
Assert-Condition ($publishedTargets -notcontains $target) `
    "The device falsifier target unexpectedly became publishable: $target"
Assert-Condition (Test-DescendantPath -Parent $buildPath -Child $rom) `
    "ROM is not isolated under the falsifier build directory: $rom"
Assert-Condition (Test-DescendantPath -Parent $buildPath -Child $elf) `
    "ELF is not isolated under the falsifier build directory: $elf"
Assert-Condition (Test-DescendantPath -Parent $buildPath -Child $exportPath) `
    "Benchmark export is not isolated under the falsifier build directory: $exportPath"
Assert-Condition (Test-DescendantPath -Parent $buildPath -Child $transcriptPath) `
    "Verifier transcript is not isolated under the falsifier build directory: $transcriptPath"

$verifierText = Get-Content -LiteralPath $verifier -Raw
$requiredDeviceMarkers = @(
    'OWNER_BENCH=',
    'RENDER_SEMANTIC=',
    'RENDER_STAGE_WORLD_CACHE=',
    'RENDER_AFFINE_MATRIX=',
    'RENDER_RAW_MATRIX=',
    'STAGE_GCDRAWALL_HW='
)
foreach ($marker in $requiredDeviceMarkers) {
    Assert-Condition ($verifierText.Contains($marker)) `
        "Required profile-2 device marker is absent from the verifier: $marker"
}
Assert-Condition ($verifierText.Contains("kind = 'smash64ds-renderer-fast-raw-benchmark'")) `
    'The verifier no longer exports the expected benchmark schema.'

if ($List) {
    Write-Output 'M3 stage AOT device falsifier (lab-only; no build/emulator in -List mode)'
    Write-Output "target=$target"
    Write-Output "build=$buildPath"
    Write-Output "rom=$rom"
    Write-Output "elf=$elf"
    Write-Output "export=$exportPath"
    Write-Output "transcript=$transcriptPath"
    Write-Output "harness=$harness mode=$expectedMode profile=$profile samples=$Samples start=$StartFrame"
    Write-Output ('requiredMarkers=' + ($requiredDeviceMarkers -join ','))
    Write-Output 'candidate=42 lists / 886 commands / 302 vertices / 54 runs / 202 triangles'
    Write-Output 'M3_AOT_READY=UNPROVED (list mode validates isolation and marker availability only)'
    return
}

$publishedOutputs = @(
    (Join-Path $root 'smash64ds.nds'),
    (Join-Path $root 'smash64ds.elf'),
    (Join-Path $root 'smash64ds-battle-playable-hwtri.nds'),
    (Join-Path $root 'smash64ds-battle-playable-hwtri.elf')
)
$publishedBefore = @{}
foreach ($path in $publishedOutputs) {
    $publishedBefore[$path] = Get-FileIdentity -Path $path
}

$rootForensicOutputs = @('.nds', '.elf', '.map', '.sym', '.ds.gba') |
    ForEach-Object { Join-Path $root "$target$_" }
foreach ($path in $rootForensicOutputs) {
    Assert-Condition (-not (Test-Path -LiteralPath $path)) `
        "Refusing to run while a nonpublished forensic output exists at repo root: $path"
}

$powerShellExe = (Get-Process -Id $PID).Path
$childArguments = @(
    '-NoProfile',
    '-ExecutionPolicy', 'Bypass',
    '-File', $verifier,
    '-MelonDS', $MelonDS,
    '-Gdb', $Gdb,
    '-RunnerSlot', [string]$RunnerSlot,
    '-DelaySeconds', [string]$DelaySeconds,
    '-BattlePlayable',
    '-RealtimePresentation',
    '-LiveInputPreview',
    '-HardwareTriangles',
    '-ImportBattleShipIFCommon',
    '-ImportBattleShipNormalMoveset',
    '-ImportBattleShipMarioFireball',
    '-ImportBattleShipFoxBlaster',
    '-ImportBattleShipEffectManager',
    '-ImportBattleShipFoxReflector',
    '-ImportBattleShipMarioSpecialHi',
    '-ImportBattleShipMarioSpecialLw',
    '-ImportBattleShipFoxSpecialHi',
    '-ImportBattleShipAudioAssets',
    '-ImportBattleShipAudioBGM',
    '-ImportBattleShipFTComputer',
    '-RendererProfileLevel', [string]$profile,
    '-RendererBenchmarkSamples', [string]$Samples,
    '-RendererBenchmarkStartFrame', [string]$StartFrame,
    '-RendererBenchmarkTimeoutSeconds', [string]$TimeoutSeconds,
    '-RendererFastRunMode', '0',
    '-WallpaperIncrementalMode', '0',
    '-LowerTextHudMode', '1',
    '-RendererBenchmarkExportPath', $exportPath,
    '-Harness', $harness,
    '-Target', $target,
    '-Build', $build,
    '-ExpectedMode', [string]$expectedMode,
    '-ExpectedHarnessSceneCurr', '22',
    '-ExpectedHarnessScenePrev', '21',
    '-Label', 'M3 stage AOT device falsifier',
    '-HarnessSelectMessage', 'M3 device falsifier did not select Pupupu VSBattle.'
)
if (($RunnerSlot -lt 0) -or $PSBoundParameters.ContainsKey('GdbPort')) {
    $childArguments += @('-GdbPort', [string]$GdbPort)
}
if ($NoBuild) { $childArguments += '-NoBuild' }

$childOutput = @(& $powerShellExe @childArguments 2>&1 | ForEach-Object { "$_" })
$childExitCode = $LASTEXITCODE
$transcriptParent = Split-Path -Parent $transcriptPath
if (-not (Test-Path -LiteralPath $transcriptParent)) {
    New-Item -ItemType Directory -Path $transcriptParent -Force | Out-Null
}
Set-Content -LiteralPath $transcriptPath -Encoding utf8 -Value $childOutput
$childOutput | Write-Output

foreach ($path in $publishedOutputs) {
    Assert-SameFileIdentity -Path $path -Before $publishedBefore[$path]
}
foreach ($path in $rootForensicOutputs) {
    Assert-Condition (-not (Test-Path -LiteralPath $path)) `
        "The isolated run wrote a forensic output at repo root: $path"
}
Assert-Condition ($childExitCode -eq 0) `
    "Profile-2 device verifier failed with exit code $childExitCode. Exact transcript: $transcriptPath"

$outputText = $childOutput -join "`n"
foreach ($summaryPattern in @(
    'Renderer owner census stage:',
    'Renderer owner churn .* stage:',
    'Renderer semantic benchmark:'
)) {
    Assert-Condition ([regex]::IsMatch($outputText, $summaryPattern)) `
        "Profile-2 verifier succeeded without required summary marker: $summaryPattern"
}

Assert-Condition (Test-Path -LiteralPath $exportPath -PathType Leaf) `
    "Profile-2 verifier did not produce its isolated benchmark export: $exportPath"
$data = Get-Content -LiteralPath $exportPath -Raw | ConvertFrom-Json
Assert-Condition ([int]$data.schema -eq 1) 'Unexpected outer benchmark schema.'
Assert-Condition ($data.kind -eq 'smash64ds-renderer-fast-raw-benchmark') `
    "Unexpected benchmark kind: $($data.kind)"
Assert-Condition ([int]$data.identity.schema -eq 1) 'Unexpected identity schema.'
Assert-Condition ($data.identity.target -eq $target) `
    "Benchmark target mismatch: $($data.identity.target)"
Assert-Condition ($data.identity.build -eq $build) `
    "Benchmark build mismatch: $($data.identity.build)"
Assert-Condition ($data.identity.harness.name -eq $harness) `
    "Benchmark harness mismatch: $($data.identity.harness.name)"
Assert-Condition ([int]$data.identity.harness.id -eq $expectedMode) `
    "Benchmark mode mismatch: $($data.identity.harness.id)"
Assert-Condition ([int]$data.identity.rendererProfile -eq $profile) `
    "Benchmark profile mismatch: $($data.identity.rendererProfile)"
Assert-Condition ([int]$data.identity.sampling.samples -eq $Samples) `
    "Benchmark sample-count mismatch: $($data.identity.sampling.samples)"
Assert-Condition (
    (Test-DescendantPath -Parent $buildPath -Child $data.identity.artifacts.rom.path) -and
    (Test-DescendantPath -Parent $buildPath -Child $data.identity.artifacts.elf.path)
) 'Benchmark identity references an artifact outside the isolated build directory.'

$ownerGroups = @($data.samples.owners)
Assert-Condition ($ownerGroups.Count -eq 3) `
    "Expected three owner groups, found $($ownerGroups.Count)."
$stageRows = @(Get-Int64Rows -Rows @($ownerGroups[0]))
Assert-Condition ($stageRows.Count -eq $Samples) `
    "Expected $Samples stage samples, found $($stageRows.Count)."
foreach ($row in $stageRows) {
    Assert-Condition ($row.Count -eq 37) `
        "Expected 37 fields in each OWNER_BENCH row, found $($row.Count)."
}

Assert-SingleValue $stageRows 1 0 'Stage owner index'
Assert-SingleValue $stageRows 3 42 'Stage selected-list count'
Assert-SingleValue $stageRows 4 886 'Stage source-command count'
Assert-SingleValue $stageRows 6 302 'Stage source-vertex count'
Assert-SingleValue $stageRows 7 0 'Stage source triangle-command count'
Assert-SingleValue $stageRows 8 202 'Stage triangle count'
Assert-SingleValue $stageRows 20 54 'Stage run count'

foreach ($row in $stageRows) {
    $classTotal = [int64]0
    foreach ($index in 9..16) { $classTotal += [int64]$row[$index] }
    Assert-Condition ($classTotal -eq 202) `
        "Stage submit classes did not sum to 202 triangles at frame $($row[0]): $classTotal"
}

$topology = Get-FieldValues -Rows $stageRows -Index 29
$selection = Get-FieldValues -Rows $stageRows -Index 30
Assert-Condition (@($topology | Where-Object { $_ -eq 0 }).Count -eq 0) `
    'Stage topology signature was zero in the sample window.'
Assert-Condition ((@($selection | Sort-Object -Unique).Count -eq 1) -and
    ($selection[0] -ne 0)) 'Stage selected-event signature was zero or changed in the sample window.'

$firstFrame = [int64]$stageRows[0][0]
$lastFrame = [int64]$stageRows[-1][0]
$churn = [ordered]@{
    topology = Get-Churn $topology
    selected = Get-Churn $selection
    camera = Get-Churn (Get-FieldValues $stageRows 31)
    dobj = Get-Churn (Get-FieldValues $stageRows 32)
    material = Get-Churn (Get-FieldValues $stageRows 33)
    light = Get-Churn (Get-FieldValues $stageRows 34)
    texture = Get-Churn (Get-FieldValues $stageRows 35)
    semantic = Get-Churn (Get-FieldValues $stageRows 36)
}

Write-Output "M3 device falsifier passed: profile=$profile frames=$firstFrame..$lastFrame samples=$Samples"
Write-Output 'stageCandidate=42 lists / 886 commands / 302 vertices / 54 runs / 202 triangles'
Write-Output ('stageChurn(adjacent/distinct)=' + (($churn.GetEnumerator() |
    ForEach-Object { "$($_.Key)=$($_.Value)" }) -join ' '))
Write-Output 'dynamicBlockers=topology,camera,DObj,material,texture,semantic signatures must be represented or invalidated by an eventual AOT plan'
Write-Output 'M3_AOT_READY=UNPROVED (this falsifies a static candidate; it does not implement or certify AOT rendering)'
