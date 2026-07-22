param(
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$GdbPort = 4333,
    [int]$RunnerSlot = -1,
    [switch]$NoBuild,
    [int]$DelaySeconds = 5,
    [switch]$ImportBattleShipFTManager,
    [switch]$ImportBattleShipFTComputer,
    [switch]$ImportBattleShipVSResults,
    [switch]$ImportBattleShipBattlePlayable,
    [switch]$ImportBattleShipIFCommon,
    [switch]$ImportBattleShipMarioFireball,
    [switch]$ImportBattleShipFoxBlaster,
    [switch]$ImportBattleShipEffectManager,
    [switch]$ImportBattleShipFoxReflector,
    [switch]$ImportBattleShipMarioSpecialHi,
    [switch]$ImportBattleShipMarioSpecialLw,
    [switch]$ImportBattleShipFoxSpecialHi,
    [switch]$ImportBattleShipAudioAssets,
    [switch]$ImportBattleShipAudioBGM,
    [switch]$ImportBattleShipNormalMoveset,
    [switch]$HardwareTriangles,
    [switch]$BattlePlayable,
    [switch]$RealtimePresentation,
    [switch]$LiveInputPreview,
    [switch]$CPUOpponentProof,
    [switch]$MatchLifecycleProof,
    [switch]$OneMinuteMatchProof,
    [switch]$RequireLocked30Pacing,
    [switch]$RendererBenchmarkOnly,
    [ValidateRange(0,2)][int]$RendererProfileLevel = 2,
    [switch]$RendererM2DetailedLedger,
    [switch]$RendererM3Phase0Profile,
    [ValidateRange(0,1)][int]$NativeStageGeneratedSegment0Enable = 0,
    [switch]$Task29GXCensus,
    [switch]$Task34StageStreamCensus,
    [ValidateRange(0,2)][int]$Task36HwComposeMode = 0,
    [switch]$Task22WallpaperRunLab,
    [ValidateRange(0,1)][int]$FastWallpaperAffineMode = 0,
    [ValidateRange(0,1)][int]$RendererScreenSpaceCensusMode = 0,
    [ValidateRange(0,1)][int]$RenderEconomyMode = 0,
    [ValidateRange(0,255)][int]$RenderEconomyOwnerMask = 0,
    [ValidateRange(0,1)][int]$Task9FloatCensusMode = 0,
    [ValidateRange(0,1)][int]$Task9FloatItcmMode = 1,
    [ValidateRange(0,1)][int]$Task9FloatPhase2Mode = 1,
    [ValidateRange(0,1)][int]$Task16FloatCompareMode = 0,
    [ValidateRange(0,1)][int]$Task16FloatI2fMode = 0,
    [ValidateRange(0,1)][int]$Task16FloatAddSubMode = 0,
    [ValidateRange(0,1)][int]$Task9StateHashMode = 0,
    [ValidateRange(0,1)][int]$Task20StackProfileMode = 0,
    [ValidateRange(0,1)][int]$Task32DrawHotTextMode = 0,
    [string]$Task9StateHashExportPath = '',
    [ValidateRange(0,1024)][int]$RendererBenchmarkSamples = 0,
    [ValidateRange(0,1000000)][int]$RendererBenchmarkStartFrame = 0,
    [ValidateSet('None','KO','Rebirth','Late','TimeUp')]
    [string]$RendererBenchmarkStartEvent = 'None',
    [switch]$PhaseMatrixMode,
    [ValidateRange(5,3600)][int]$RendererBenchmarkTimeoutSeconds = 30,
    [ValidateRange(0,9)][int]$RendererFastRunMode = 0,
    [ValidateRange(0,1)][int]$StaticTextureAotMode = 0,
    [ValidateRange(0,1)][int]$IFCommonHybridOamMode = 0,
    [switch]$RequireZeroPostGoTextureFence,
    [ValidateRange(0,1)][int]$FoxCpuMode = 0,
    [ValidateRange(0,1)][int]$WallpaperIncrementalMode = 0,
    [ValidateRange(0,1)][int]$LowerTextHudMode = 1,
    [string]$RendererBenchmarkExportPath = '',
    [string]$RendererBenchmarkScreenshot = '',
    [switch]$Task25RPacingTrace,
    [string]$Task25RLifecycleExportPath = '',
    [Parameter(Mandatory = $true)][string]$Harness,
    [Parameter(Mandatory = $true)][string]$Target,
    [Parameter(Mandatory = $true)][string]$Build,
    [Parameter(Mandatory = $true)][int]$ExpectedMode,
    [Parameter(Mandatory = $true)][int]$ExpectedHarnessSceneCurr,
    [Parameter(Mandatory = $true)][int]$ExpectedHarnessScenePrev,
    [Parameter(Mandatory = $true)][string]$Label,
    [Parameter(Mandatory = $true)][string]$HarnessSelectMessage
)
$ErrorActionPreference = 'Stop'
$usesPublishedIntrinsicRendererDefaults = $Target -in @(
    'smash64ds-battle-playable-hwtri',
    'smash64ds-battle-playable-proof-hwtri'
)
$usesIntrinsicTask36Replay = $usesPublishedIntrinsicRendererDefaults
$usesIntrinsicTask16FloatHelpers = $Target -in @(
    'smash64ds-battle-playable-hwtri',
    'smash64ds-battle-playable-proof-hwtri',
    'smash64ds-battle-playable-freeze-diagnostics-on-hwtri',
    'smash64ds-battle-playable-freeze-diagnostics-off-hwtri'
)
$usesIntrinsicNativeStageGeneratedSegment0 = $Target -in @(
    'smash64ds-battle-playable-hwtri',
    'smash64ds-battle-playable-proof-hwtri',
    'smash64ds-battle-playable-freeze-diagnostics-on-hwtri',
    'smash64ds-battle-playable-freeze-diagnostics-off-hwtri'
)
$usesIntrinsicFastWallpaper = $Target -in @(
    'smash64ds-battle-playable-hwtri',
    'smash64ds-battle-playable-proof-hwtri',
    'smash64ds-battle-playable-freeze-diagnostics-on-hwtri',
    'smash64ds-battle-playable-freeze-diagnostics-off-hwtri'
)
$usesIntrinsicTask32DrawHotText = $Target -in @(
    'smash64ds-battle-playable-hwtri',
    'smash64ds-battle-playable-proof-hwtri',
    'smash64ds-battle-playable-freeze-diagnostics-on-hwtri',
    'smash64ds-battle-playable-freeze-diagnostics-off-hwtri'
)
$effectiveTask16FloatCompareMode = if (
    $usesIntrinsicTask16FloatHelpers) { 1 } else { $Task16FloatCompareMode }
$effectiveTask16FloatI2fMode = if (
    $usesIntrinsicTask16FloatHelpers) { 1 } else { $Task16FloatI2fMode }
$effectiveTask16FloatAddSubMode = if (
    $usesIntrinsicTask16FloatHelpers) { 1 } else { $Task16FloatAddSubMode }
$effectiveTask36HwComposeMode = if (
    $usesIntrinsicTask36Replay) { 2 } else { $Task36HwComposeMode }
$staticTextureAotSelected =
    $PSBoundParameters.ContainsKey('StaticTextureAotMode')
$staticTextureAotSelected =
    $staticTextureAotSelected -or $RequireZeroPostGoTextureFence
$foxCpuModeSelected = $PSBoundParameters.ContainsKey('FoxCpuMode')
$nativeStageGeneratedSegment0Selected =
    $PSBoundParameters.ContainsKey('NativeStageGeneratedSegment0Enable') -or
    $usesIntrinsicNativeStageGeneratedSegment0
$effectiveNativeStageGeneratedSegment0Enable = if (
    $usesIntrinsicNativeStageGeneratedSegment0) {
    1
} else {
    $NativeStageGeneratedSegment0Enable
}
$effectiveStaticTextureAotMode = if ($usesPublishedIntrinsicRendererDefaults) {
    1
} else {
    $StaticTextureAotMode
}
$effectiveIFCommonHybridOamMode = if ($usesPublishedIntrinsicRendererDefaults) {
    0
} else {
    $IFCommonHybridOamMode
}
$effectiveFastWallpaperAffineMode = if ($usesIntrinsicFastWallpaper) {
    1
} else {
    $FastWallpaperAffineMode
}
$effectiveTask32DrawHotTextMode = if ($usesIntrinsicTask32DrawHotText) {
    1
} else {
    $Task32DrawHotTextMode
}
$m4CandidateEvidence =
    ($effectiveStaticTextureAotMode -eq 1) -and
    ($RendererFastRunMode -eq 9)
$m3StageOnlyBenchmarkWindow =
    $BattlePlayable -and $RealtimePresentation -and
    ($ExpectedMode -eq 163) -and
    ($RendererProfileLevel -eq 1) -and
    ($RendererFastRunMode -eq 9) -and
    ($RendererBenchmarkStartFrame -eq 438) -and
    ($RendererBenchmarkSamples -eq 8)
$naturalScreenSpaceCensusWindow =
    ($RendererScreenSpaceCensusMode -eq 1) -and
    ($RendererBenchmarkSamples -ge 512) -and
    ($FoxCpuMode -eq 1) -and
    ($RendererBenchmarkStartEvent -eq 'None')
if ($OneMinuteMatchProof -and -not $MatchLifecycleProof) {
    throw 'OneMinuteMatchProof requires MatchLifecycleProof.'
}
if ($OneMinuteMatchProof -and -not $RealtimePresentation) {
    throw 'OneMinuteMatchProof requires realtime presentation so wall-clock pacing is hardware-vblank anchored.'
}
if ($Task25RPacingTrace -and -not (
        $OneMinuteMatchProof -and $MatchLifecycleProof -and
        $RealtimePresentation -and ($RendererProfileLevel -eq 0))) {
    throw 'Task25RPacingTrace requires the realtime profile-0 one-minute lifecycle proof.'
}
if ($Task25RLifecycleExportPath -and -not $Task25RPacingTrace) {
    throw 'Task25RLifecycleExportPath requires Task25RPacingTrace.'
}
if ($OneMinuteMatchProof -and
    (-not $ImportBattleShipAudioAssets -or -not $ImportBattleShipAudioBGM)) {
    throw 'OneMinuteMatchProof requires audio asset and BGM marker capture.'
}
if (($Task9StateHashMode -eq 1) -and -not $MatchLifecycleProof) {
    throw 'Task9StateHashMode requires the deterministic match-lifecycle proof.'
}
if ($Task9StateHashExportPath -and ($Task9StateHashMode -ne 1)) {
    throw 'Task9StateHashExportPath requires Task9StateHashMode 1.'
}
if ($RendererBenchmarkOnly -and ($RendererBenchmarkSamples -eq 0)) {
    throw 'RendererBenchmarkOnly requires RendererBenchmarkSamples greater than zero.'
}
if (($RendererBenchmarkStartEvent -ne 'None') -and
    ($RendererBenchmarkStartFrame -ne 0)) {
    throw 'RendererBenchmarkStartEvent and RendererBenchmarkStartFrame are mutually exclusive.'
}
if (($RendererBenchmarkStartEvent -ne 'None') -and
    -not $RendererBenchmarkOnly) {
    throw 'RendererBenchmarkStartEvent requires RendererBenchmarkOnly.'
}
if ($PhaseMatrixMode -and -not (
        $BattlePlayable -and $RealtimePresentation -and
        ($ExpectedMode -eq 163) -and ($RendererProfileLevel -eq 1) -and
        ($RendererFastRunMode -eq 9) -and ($FoxCpuMode -eq 1) -and
        ($WallpaperIncrementalMode -eq 1) -and
        ($RendererBenchmarkSamples -eq 8) -and
        ((($RendererBenchmarkStartEvent -eq 'None') -and
          ($RendererBenchmarkStartFrame -in @(438, 600, 672, 1398))) -or
         (($RendererBenchmarkStartEvent -ne 'None') -and
          ($RendererBenchmarkStartFrame -eq 0))))) {
    throw 'PhaseMatrixMode requires mode 163/profile 1/fast 9/live Fox/production wallpaper and one exact eight-frame phase gate.'
}
if ($RendererM2DetailedLedger -and ($RendererProfileLevel -ne 1)) {
    throw 'RendererM2DetailedLedger requires RendererProfileLevel 1.'
}
if ($Task29GXCensus -and
    (($RendererProfileLevel -ne 1) -or ($RendererFastRunMode -ne 9))) {
    throw 'Task29GXCensus requires profile 1 and complete-stage fast-run mode 9.'
}
if ($Task34StageStreamCensus -and
    (($RendererProfileLevel -ne 1) -or
     ($RendererFastRunMode -ne 9) -or
     ($RendererBenchmarkSamples -ne 8) -or
     ($RendererBenchmarkStartFrame -le 0))) {
    throw 'Task34StageStreamCensus requires profile 1, fast mode 9, and one exact eight-frame gate.'
}
if ($RendererM3Phase0Profile -and
    (($RendererProfileLevel -ne 1) -or ($RendererFastRunMode -ne 9))) {
    throw 'RendererM3Phase0Profile requires RendererProfileLevel 1 and RendererFastRunMode 9.'
}
if (($effectiveNativeStageGeneratedSegment0Enable -eq 1) -and
    ($RendererFastRunMode -ne 9)) {
    throw 'NativeStageGeneratedSegment0Enable=1 requires RendererFastRunMode 9.'
}
if (($Task20StackProfileMode -eq 1) -and
    ($RendererProfileLevel -ne 1)) {
    throw 'Task20StackProfileMode requires RendererProfileLevel 1.'
}
if (($Task20StackProfileMode -eq 1) -and $PhaseMatrixMode) {
    throw 'Task20StackProfileMode is a separate startup stack census and cannot contaminate the phase matrix.'
}
if (($Task20StackProfileMode -eq 1) -and -not $MatchLifecycleProof -and
    ($RendererBenchmarkSamples -lt 1)) {
    throw 'Task20StackProfileMode requires a lifecycle proof or at least one benchmark sample for startup census capture.'
}
if (($RendererScreenSpaceCensusMode -eq 1) -and
    (($RendererProfileLevel -ne 1) -or ($RendererFastRunMode -ne 9) -or
     ($RendererBenchmarkStartFrame -le 0))) {
    throw 'RendererScreenSpaceCensusMode requires RendererProfileLevel 1, RendererFastRunMode 9, and an exact positive start frame.'
}
if (($RenderEconomyMode -eq 1) -and
    (($RendererProfileLevel -ne 1) -or ($RendererFastRunMode -ne 9))) {
    throw 'RenderEconomyMode requires RendererProfileLevel 1 and RendererFastRunMode 9.'
}
if (($RenderEconomyMode -eq 0) -and ($RenderEconomyOwnerMask -ne 0)) {
    throw 'RenderEconomyOwnerMask requires RenderEconomyMode 1.'
}
if ($RequireZeroPostGoTextureFence -and -not $HardwareTriangles) {
    throw 'RequireZeroPostGoTextureFence requires HardwareTriangles.'
}
if ($RequireZeroPostGoTextureFence -and -not $BattlePlayable) {
    throw 'RequireZeroPostGoTextureFence requires BattlePlayable.'
}
if ($RequireZeroPostGoTextureFence -and ($effectiveStaticTextureAotMode -ne 1)) {
    throw 'RequireZeroPostGoTextureFence requires StaticTextureAotMode 1.'
}
if ($RequireZeroPostGoTextureFence -and -not $OneMinuteMatchProof -and
    ($RendererBenchmarkSamples -eq 0)) {
    throw 'RequireZeroPostGoTextureFence requires a sampled benchmark window or OneMinuteMatchProof.'
}
if ($RequireZeroPostGoTextureFence -and
    ($RendererBenchmarkSamples -gt 0) -and ($RendererProfileLevel -lt 1)) {
    throw 'RequireZeroPostGoTextureFence sampled windows require RendererProfileLevel 1 or 2.'
}
if ($RequireZeroPostGoTextureFence -and $foxCpuModeSelected -and
    ($FoxCpuMode -eq 0)) {
    throw 'RequireZeroPostGoTextureFence requires the CPU/countdown path; fast iteration deliberately does not arm M4.'
}
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
. (Join-Path $PSScriptRoot 'lib\gdb-markers.ps1')
. (Join-Path $PSScriptRoot 'lib\build-output.ps1')
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$ImportBattleShipFTManager = $true
$task9FloatRoutineNames = @(
    'fadd', 'fsub', 'frsub', 'fmul', 'fdiv',
    'fcmpeq', 'fcmplt', 'fcmple', 'fcmpge', 'fcmpgt', 'fcmpun',
    'f2iz', 'f2uiz', 'i2f', 'ui2f', 'l2f', 'ul2f', 'f2d', 'd2f',
    'dadd', 'dsub', 'drsub', 'dmul', 'ddiv',
    'dcmpeq', 'dcmplt', 'dcmple', 'dcmpge', 'dcmpgt', 'dcmpun',
    'd2iz', 'i2d', 'ui2d', 'l2d', 'ul2d'
)
if ($BattlePlayable) {
    $ImportBattleShipBattlePlayable = $true
}
if ($CPUOpponentProof -or $MatchLifecycleProof) {
    $ImportBattleShipFTComputer = $true
    $LiveInputPreview = $true
}
if ($MatchLifecycleProof) {
    $CPUOpponentProof = $true
    $ImportBattleShipVSResults = $true
}
if ($CPUOpponentProof) {
    $FoxCpuMode = 1
    $foxCpuModeSelected = $true
}
$fastIterationUnarmedM4 =
    ($RendererFastRunMode -in @(7, 8, 9)) -and
    $foxCpuModeSelected -and ($FoxCpuMode -eq 0)
$preBattleSelectorSelected =
    $staticTextureAotSelected -or $foxCpuModeSelected
$verifierContext = Initialize-MelonDSVerifierContext `
    -Root $root `
    -MelonDS $MelonDS `
    -RunnerSlot $RunnerSlot `
    -GdbPort $GdbPort `
    -GdbPortExplicit:$PSBoundParameters.ContainsKey('GdbPort') `
    -NoBuild:$NoBuild
$rom = Resolve-Smash64DSBuildOutput -Root $root -Target $Target -Build $Build -Extension '.nds'
$elf = Resolve-Smash64DSBuildOutput -Root $root -Target $Target -Build $Build -Extension '.elf'
$melonDsPath = $verifierContext.MelonDSPath
$melonDsDir = Split-Path -Parent $melonDsPath
$logDir = Get-MelonDSVerifierLogDir -Root $root -RunnerSlot (Get-MelonDSActiveRunnerSlot)
$stdout = Join-Path $logDir "melonds.$($Harness.Replace('_','-'))-harness.stdout.log"
$stderr = Join-Path $logDir "melonds.$($Harness.Replace('_','-'))-harness.stderr.log"
$scriptName = "_$($Harness)_harness.gdb"
$task34StageDumpFiles = @()
if ($Task34StageStreamCensus) {
    $task34DumpToken = [guid]::NewGuid().ToString('N')
    $task34StageDumpFiles = @(for ($sampleIndex = 0;
        $sampleIndex -lt $RendererBenchmarkSamples; $sampleIndex++) {
        [PSCustomObject]@{
            Sample = $sampleIndex
            Entries = Join-Path $logDir (
                "task34-$task34DumpToken-$sampleIndex-entries.bin")
            Words = Join-Path $logDir (
                "task34-$task34DumpToken-$sampleIndex-words.bin")
        }
    })
}
$configState = $null
$emulator = $null
$benchmarkMakeIdentity = $null
$benchmarkRomIdentity = $null
$benchmarkElfIdentity = $null
$benchmarkMelonIdentity = $null
$benchmarkMelonConfigSha256 = $null
$usesRetainedWallpaper = $false
$usesFastWallpaper = $effectiveFastWallpaperAffineMode -eq 1
function Assert-Condition {
    param([bool]$Condition, [string]$Message, [string]$Context)
    if (-not $Condition) { throw "$Message`n$Context" }
}
function Get-Ints {
    param([System.Text.RegularExpressions.Match]$Match)
    $values = @()
    for ($i = 1; $i -lt $Match.Groups.Count; $i++) {
        $text = $Match.Groups[$i].Value
        if ($text -like '0x*') { $values += [int64](Convert-MarkerUInt32 $text) }
        else { $values += [int64]$text }
    }
    return $values
}
function Test-DamageDigits {
    param([int64]$Damage, [int64]$DigitCount, [int64]$DigitsPack)
    if ($Damage -le 0) { return $false }
    if ($Damage -gt 999) { $Damage = 999 }
    $damageText = [string]$Damage
    $expected = @(10)
    for ($i = $damageText.Length - 1; $i -ge 0; $i--) {
        $expected += [int]([string]$damageText[$i])
    }
    if ($DigitCount -ne $expected.Count) { return $false }
    for ($i = 0; $i -lt $expected.Count; $i++) {
        $digit = ($DigitsPack -shr (8 * $i)) -band 0xff
        if ($digit -ne $expected[$i]) { return $false }
    }
    return $true
}
function Get-Median {
    param([int64[]]$Values)
    $ordered = @($Values | Sort-Object)
    if ($ordered.Count -eq 0) { return 0 }
    $middle = [int]($ordered.Count / 2)
    if (($ordered.Count % 2) -eq 0) {
        return [int64](($ordered[$middle - 1] + $ordered[$middle]) / 2)
    }
    return [int64]$ordered[$middle]
}
function Get-Percentile95 {
    param([int64[]]$Values)
    $ordered = @($Values | Sort-Object)
    if ($ordered.Count -eq 0) { return 0 }
    $index = [Math]::Max(0, [Math]::Ceiling($ordered.Count * 0.95) - 1)
    return [int64]$ordered[$index]
}
function Get-AdjacentChurn {
    param([int64[]]$Values)
    if ($Values.Count -eq 0) { return '0/0' }
    $changes = 0
    for ($i = 1; $i -lt $Values.Count; $i++) {
        if ($Values[$i] -ne $Values[$i - 1]) { $changes++ }
    }
    $distinct = @($Values | Sort-Object -Unique).Count
    return "$changes/$distinct"
}
function Get-AdjacentHitChangeCounts {
    param([int64[]]$Values, [int]$Lane)
    $opportunities = [Math]::Max(0, $Values.Count - 1)
    $changes = 0
    for ($i = 1; $i -lt $Values.Count; $i++) {
        if ($Values[$i] -ne $Values[$i - 1]) { $changes++ }
    }
    return [ordered]@{
        lane = $Lane
        samples = $Values.Count
        opportunities = $opportunities
        hits = $opportunities - $changes
        changes = $changes
    }
}
function Get-MedianP95 {
    param([int64[]]$Values)
    return "$(Get-Median $Values)/$(Get-Percentile95 $Values)"
}
function Get-UnsignedMarkerMatches {
    param([string]$Text, [string]$Name, [int]$FieldCount)
    $fieldPattern = ((1..$FieldCount | ForEach-Object { '([0-9]+)' }) -join ',')
    return [regex]::Matches($Text, ([regex]::Escape("$Name=") + $fieldPattern))
}
function Get-SampleFieldValues {
    param([System.Collections.IEnumerable]$Samples, [int]$FieldIndex)
    $values = @()
    foreach ($sample in $Samples) {
        $values += [int64]$sample[$FieldIndex]
    }
    return [int64[]]$values
}
function Get-RatioBasisPoints {
    param([int64]$Numerator, [int64]$Denominator)
    if ($Denominator -le 0) { return [int64]0 }
    return [int64](($Numerator * 10000) / $Denominator)
}
function Test-RendererUploadPair {
    param([int64]$Count, [int64]$Bytes)
    # Every renderer profile may sample either a resident frame or one of the
    # exact animated-water upload phases. Profile 2 uses an independent
    # bytewise decoder, but its cache phase is not synchronized to this probe.
    return (($Count -eq 0 -and $Bytes -eq 0) -or
            ($Count -eq 1 -and ($Bytes -eq 4096 -or $Bytes -eq 32768)) -or
            ($Count -eq 2 -and $Bytes -eq 36864))
}
function Get-RendererUploadSequenceHash {
    param([int64[]]$Counts, [int64[]]$Bytes)
    $pairs = for ($i = 0; $i -lt $Counts.Count; $i++) {
        "$($Counts[$i]):$($Bytes[$i])"
    }
    $sha = [System.Security.Cryptography.SHA256]::Create()
    try {
        $digest = $sha.ComputeHash(
            [System.Text.Encoding]::UTF8.GetBytes(($pairs -join ';')))
        return ([BitConverter]::ToString($digest)).Replace('-', '')
    } finally {
        $sha.Dispose()
    }
}
function Get-RendererOwnerBenchmarkCommand {
    param([ValidateRange(0,2)][int]$OwnerIndex)
    $owner = "gNdsRendererProfileOwners[$OwnerIndex]"
    return ('printf "OWNER_BENCH=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsRendererProfileFrameCount, {0}, {1}.exclusive_ticks, {1}.selected_count, {1}.source_command_count, {1}.vertex_command_count, {1}.source_vertex_count, {1}.triangle_command_count, {1}.triangle_count, {1}.submit_class_count[0], {1}.submit_class_count[1], {1}.submit_class_count[2], {1}.submit_class_count[3], {1}.submit_class_count[4], {1}.submit_class_count[5], {1}.submit_class_count[6], {1}.submit_class_count[7], {1}.material_operation_count, {1}.matrix_change_count, {1}.texture_change_count, {1}.run_count, {1}.entry_state_hash, {1}.exit_state_hash, {1}.entry_vertex_cache_hash, {1}.exit_vertex_cache_hash, {1}.entry_resolver_hash, {1}.exit_resolver_hash, {1}.entry_global_hash, {1}.exit_global_hash, {1}.topology_signature, {1}.selected_event_signature, {1}.camera_signature, {1}.dobj_matrix_signature, {1}.material_signature, {1}.light_signature, {1}.texture_signature, {1}.semantic_output_hash' -f $OwnerIndex, $owner)
}
function Get-RendererM2BenchmarkCommand {
    param([ValidateRange(1,2)][int]$OwnerIndex)
    $owner = "gNdsRendererProfileOwners[$OwnerIndex]"
    $fields = @(
        'm2_contract_capture_ticks', 'm2_collection_ticks',
        'm2_owner_validation_ticks', 'm2_census_ticks',
        'm2_camera_fetch_ticks', 'm2_hash_parent_lookup_ticks',
        'm2_local_matrix_ticks', 'm2_world_affine_ticks',
        'm2_world_camera_ticks', 'm2_final_compose_ticks',
        'm2_material_ticks', 'm2_production_total_ticks',
        'm2_production_preflight_state_ticks',
        'm2_lighting_shading_ticks', 'm2_root_gx_ticks',
        'm2_run_prepare_ticks', 'm2_corner_emit_account_ticks',
        'm2_owner_residual_ticks', 'm2_production_success_count',
        'm2_production_failure_count',
        'm2_production_phase_overlap_count',
        'm2_owner_phase_overlap_count', 'm2_schedule_joint_count',
        'm2_schedule_match_count', 'm2_binding_count',
        'm2_binding_match_count', 'm2_xobj_count',
        'm2_xobj_kind_4b_count', 'm2_xobj_kind_2_count',
        'm2_xobj_other_count', 'm2_xobj_null_count', 'm2_parts_count',
        'm2_parts_matrix_mode0_count', 'm2_parts_matrix_mode1_count',
        'm2_parts_matrix_mode3_count', 'm2_parts_matrix_other_count',
        'm2_animlock_active', 'm2_camera_fetch_count',
        'm2_world_matrix_request_count',
        'm2_world_matrix_cache_hit_count', 'm2_local_matrix_build_count',
        'm2_world_affine_count', 'm2_world_camera_count',
        'm2_final_compose_count', 'm2_root_gx_count',
        'm2_lighting_epoch_count', 'm2_run_prepare_count',
        'm2_corner_emit_run_count'
    )
    $format = ((1..(2 + $fields.Count) | ForEach-Object { '%u' }) -join ',')
    $arguments = @('gNdsRendererProfileFrameCount', "$OwnerIndex")
    $arguments += @($fields | ForEach-Object { "$owner.$_" })
    return ('printf "M2_BENCH={0}\n", {1}' -f
        $format, ($arguments -join ', '))
}
function Get-RendererSemanticBenchmarkCommand {
    $format = ((1..38 | ForEach-Object { '%u' }) -join ',')
    $arguments = @(
        'gNdsRendererProfileFrameCount',
        'gNdsRendererSemanticOutputHash',
        'gNdsRendererSemanticOutputHash2',
        'gNdsRendererSemanticEventCount',
        'gNdsRendererSemanticOverflowCount'
    )
    foreach ($ownerIndex in 0..2) {
        $owner = "gNdsRendererProfileOwners[$ownerIndex]"
        $arguments += @(
            "$owner.semantic_output_hash",
            "$owner.semantic_output_hash2",
            "$owner.semantic_event_count",
            "$owner.semantic_overflow_count",
            "$owner.semantic_occurrence_count",
            "$owner.semantic_first_owner_occurrence",
            "$owner.semantic_first_list_ordinal",
            "$owner.semantic_first_branch_path",
            "$owner.semantic_first_command_index",
            "$owner.semantic_first_tri2_half",
            "$owner.semantic_first_outcome"
        )
    }
    return ('printf "RENDER_SEMANTIC={0}\n", {1}' -f $format, ($arguments -join ', '))
}
function Get-Task20CoroutineCensusCommands {
    return @(
        'printf "TASK31_CENSUS_SUMMARY=%u,%u,%u,%u,%u,%u\n", gNdsTask20CoroutineCensusCount, gNdsTask20CoroutineCensusOverflowCount, gNdsTask20CoroutineCensusLiveCount, gNdsTask20CoroutineCensusPeakLiveCount, gNdsTask20CoroutineCensusLargeLiveCount, gNdsTask20CoroutineCensusPeakLargeLiveCount',
        'set $task31_census_index = 0',
        'while $task31_census_index < gNdsTask20CoroutineCensusCount && $task31_census_index < 64',
        'printf "TASK31_CENSUS=%u,%u,%u,%u,%u,%u,%u,%u\n", $task31_census_index, gNdsTask20CoroutineCensus[$task31_census_index].owner_id, gNdsTask20CoroutineCensus[$task31_census_index].requested_stack_size, gNdsTask20CoroutineCensus[$task31_census_index].actual_stack_size, gNdsTask20CoroutineCensus[$task31_census_index].stack_base, gNdsTask20CoroutineCensus[$task31_census_index].coroutine_address, gNdsTask20CoroutineCensus[$task31_census_index].state, gNdsTask20CoroutineCensus[$task31_census_index].high_water',
        'set $task31_census_index = $task31_census_index + 1',
        'end'
    )
}
function Get-BenchmarkMakeIdentity {
    param([string[]]$BaseMakeArgs)

    $identityArgs = @($BaseMakeArgs) + 'print-benchmark-flags'
    # The verifier imports a `make` wrapper that deliberately honors
    # SMASH64DS_VERIFY_NO_BUILD.  Identity is a read-only Makefile query and
    # must still run for same-ROM -NoBuild samples, so invoke the application.
    $makeExe = (Get-Command make.exe -CommandType Application).Source
    $identityOutput = @(& $makeExe @identityArgs 2>&1 |
        ForEach-Object { "$_" })
    if ($LASTEXITCODE -ne 0) {
        throw "Makefile benchmark-flag query failed.`n$($identityOutput -join "`n")"
    }
    $values = @{}
    foreach ($line in $identityOutput) {
        $match = [regex]::Match($line, '^BENCH_MAKE_([A-Z0-9_]+)=(.*)$')
        if ($match.Success) {
            $values[$match.Groups[1].Value] = $match.Groups[2].Value
        }
    }
    $required = @(
        'TARGET', 'HARNESS', 'HARNESS_ID', 'PROFILE', 'SHIP_TELEMETRY',
        'TICK_HUD', 'M2_DETAILED_LEDGER',
        'M3_PHASE0_PROFILE', 'NATIVE_STAGE_GENERATED_SEGMENT0_ENABLE',
        'TASK29_GX_CENSUS', 'TASK34_STAGE_STREAM_CENSUS',
        'TASK36_HW_COMPOSE',
        'TASK22_WALLPAPER_RUN_LAB',
        'SCREEN_SPACE_CENSUS', 'RENDER_ECONOMY',
        'RENDER_ECONOMY_OWNER_MASK', 'RENDERER_BENCHMARK_MODE',
        'FAST_RUN_DEFAULT',
        'SCENE_MIP_CACHE_LAB', 'FAST_WALLPAPER_AFFINE',
        'BATTLE_STATIC_TEXTURE_DEFAULT',
        'IFCOMMON_HYBRID_OAM', 'TASK9_FLOAT_CENSUS', 'TASK9_FLOAT_ITCM',
        'TASK9_FLOAT_PHASE2', 'TASK16_FLOAT_COMPARE', 'TASK16_FLOAT_I2F',
        'TASK16_FLOAT_ADDSUB', 'TASK9_STATE_HASH',
        'TASK20_STACK_PROFILE', 'TASK32_DRAW_HOT_TEXT',
        'CFLAGS_COMMON', 'CFLAGS_RENDERER', 'CFLAGS_SCENE'
    )
    foreach ($key in $required) {
        if (-not $values.ContainsKey($key)) {
            throw "Makefile benchmark-flag query omitted BENCH_MAKE_$key.`n$($identityOutput -join "`n")"
        }
    }
    return [PSCustomObject]@{
        Target = $values.TARGET
        Harness = $values.HARNESS
        HarnessId = [int]$values.HARNESS_ID
        Profile = [int]$values.PROFILE
        ShipTelemetry = [int]$values.SHIP_TELEMETRY
        TickHud = [int]$values.TICK_HUD
        M2DetailedLedger = [int]$values.M2_DETAILED_LEDGER
        M3Phase0Profile = [int]$values.M3_PHASE0_PROFILE
        NativeStageGeneratedSegment0Enable =
            [int]$values.NATIVE_STAGE_GENERATED_SEGMENT0_ENABLE
        Task29GXCensus = [int]$values.TASK29_GX_CENSUS
        Task34StageStreamCensus = [int]$values.TASK34_STAGE_STREAM_CENSUS
        Task36HwComposeMode = [int]$values.TASK36_HW_COMPOSE
        Task22WallpaperRunLab = [int]$values.TASK22_WALLPAPER_RUN_LAB
        ScreenSpaceCensusMode = [int]$values.SCREEN_SPACE_CENSUS
        RenderEconomyMode = [int]$values.RENDER_ECONOMY
        RenderEconomyOwnerMask = [int]$values.RENDER_ECONOMY_OWNER_MASK
        RendererBenchmarkMode = [int]$values.RENDERER_BENCHMARK_MODE
        FastRunDefault = [int]$values.FAST_RUN_DEFAULT
        SceneMipCacheLab = [int]$values.SCENE_MIP_CACHE_LAB
        FastWallpaperAffine = [int]$values.FAST_WALLPAPER_AFFINE
        BattleStaticTextureDefault =
            [int]$values.BATTLE_STATIC_TEXTURE_DEFAULT
        IFCommonHybridOamMode = [int]$values.IFCOMMON_HYBRID_OAM
        Task9FloatCensusMode = [int]$values.TASK9_FLOAT_CENSUS
        Task9FloatItcmMode = [int]$values.TASK9_FLOAT_ITCM
        Task9FloatPhase2Mode = [int]$values.TASK9_FLOAT_PHASE2
        Task16FloatCompareMode = [int]$values.TASK16_FLOAT_COMPARE
        Task16FloatI2fMode = [int]$values.TASK16_FLOAT_I2F
        Task16FloatAddSubMode = [int]$values.TASK16_FLOAT_ADDSUB
        Task9StateHashMode = [int]$values.TASK9_STATE_HASH
        Task20StackProfileMode = [int]$values.TASK20_STACK_PROFILE
        Task32DrawHotTextMode = [int]$values.TASK32_DRAW_HOT_TEXT
        CommonCFlags = $values.CFLAGS_COMMON
        RendererCFlags = $values.CFLAGS_RENDERER
        SceneCFlags = $values.CFLAGS_SCENE
    }
}
function Get-BenchmarkFileIdentity {
    param([Parameter(Mandatory=$true)][string]$Path)

    $item = Get-Item -LiteralPath $Path
    return [PSCustomObject]@{
        Sha256 = (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash
        Bytes = [int64]$item.Length
        LastWriteUtc = $item.LastWriteTimeUtc.ToString('o')
    }
}
function Complete-Task9StateHashCapture {
    param(
        [System.Collections.IEnumerable]$Summary,
        [System.Collections.IEnumerable]$Records,
        [string]$GdbOutput
    )

    Assert-Condition (@($Summary).Count -eq 1) `
        "Task 9 state hash emitted $(@($Summary).Count) summaries instead of one." `
        $GdbOutput
    $summaryValues = Get-Ints @($Summary)[0]
    Assert-Condition (
        $summaryValues[0] -ge 3600 -and
        $summaryValues[0] -le $summaryValues[2] -and
        $summaryValues[1] -eq 0 -and
        @($Records).Count -eq $summaryValues[0]
    ) "Task 9 state hash did not cover every deterministic match update without overflow (summary=$($summaryValues -join ',') rows=$(@($Records).Count))." `
        $GdbOutput
    $rows = @($Records | ForEach-Object { , @(Get-Ints $_) })
    for ($index = 0; $index -lt $rows.Count; $index++) {
        $row = $rows[$index]
        Assert-Condition (
            $row[0] -eq $index -and $row[3] -gt 0 -and
            $row[4] -gt 0 -and $row[5] -eq 0
        ) "Task 9 state hash row $index has incomplete coverage or overflowed (row=$($row -join ','))." `
            $GdbOutput
    }
    if ($Task9StateHashExportPath) {
        $resolvedPath = if ([System.IO.Path]::IsPathRooted(
                $Task9StateHashExportPath)) {
            $Task9StateHashExportPath
        } else {
            Join-Path $root $Task9StateHashExportPath
        }
        $parent = Split-Path -Parent $resolvedPath
        if ($parent -and -not (Test-Path -LiteralPath $parent)) {
            New-Item -ItemType Directory -Path $parent -Force | Out-Null
        }
        $export = [ordered]@{
            schema = 1
            kind = 'smash64ds-task9-full-active-game-state-hash'
            target = $Target
            build = $Build
            task9FloatItcmMode = $Task9FloatItcmMode
            task9FloatPhase2Mode = $Task9FloatPhase2Mode
            task16FloatCompareMode = $effectiveTask16FloatCompareMode
            task16FloatI2fMode = $effectiveTask16FloatI2fMode
            task16FloatAddSubMode = $effectiveTask16FloatAddSubMode
            task9StateHashMode = $Task9StateHashMode
            coverage = [ordered]@{
                source = 'post-scVSBattleFuncUpdate'
                pointerPolicy = 'general/graphics relative; static code/data region canonical'
                records = 'battle scene camera ground controllers collision active GObj/process/DObj/SObj/CObj/AObj/MObj fighter/item/weapon/effect state'
                updates = [int64]$summaryValues[0]
                overflow = [int64]$summaryValues[1]
            }
            artifacts = [ordered]@{
                rom = Get-BenchmarkFileIdentity -Path $rom
                elf = Get-BenchmarkFileIdentity -Path $elf
            }
            rows = @($rows)
        }
        Set-Content -LiteralPath $resolvedPath -Encoding utf8 `
            -Value ($export | ConvertTo-Json -Depth 7)
    }
    return [PSCustomObject]@{
        Summary = $summaryValues
        Rows = $rows
    }
}
function Complete-Task25RPacingTrace {
    param(
        [System.Collections.IEnumerable]$Matches,
        [int64[]]$Pacing,
        [string]$GdbOutput
    )

    $phaseNames = @('countdown', 'earlyCombat', 'lateCombat', 'koRebirth', 'results')
    $rows = @($Matches | ForEach-Object { , @(Get-Ints $_) })
    Assert-Condition ($rows.Count -eq $Pacing[3]) `
        "Task 25R profile-0 trace captured $($rows.Count) of $($Pacing[3]) presentations." `
        $GdbOutput
    $histogram = [ordered]@{ vblank2 = 0; vblank3 = 0; vblank4 = 0; vblank5Plus = 0 }
    $phaseHistograms = @($phaseNames | ForEach-Object {
        [ordered]@{ phase = $_; vblank2 = 0; vblank3 = 0; vblank4 = 0; vblank5Plus = 0; slips = 0 }
    })
    $previous = @(0) * 14
    $previousVBlank = $null
    $totalSlips = [int64]0

    for ($index = 0; $index -lt $rows.Count; $index++) {
        $row = $rows[$index]
        Assert-Condition ($row[1] -eq ($index + 1) -and
            $row[2] -eq (2 * $row[1]) -and $row[3] -eq 0) `
            "Task 25R profile-0 trace lost fixed-two accounting at presentation $($index + 1)." `
            $GdbOutput
        $activePhase = -1
        $intervalSlips = [int64]0
        for ($phase = 0; $phase -lt 5; $phase++) {
            $presentDelta = [int64]$row[4 + $phase] - $previous[4 + $phase]
            $slipDelta = [int64]$row[9 + $phase] - $previous[9 + $phase]
            Assert-Condition ($presentDelta -ge 0 -and $slipDelta -ge 0) `
                "Task 25R profile-0 phase counters regressed at presentation $($index + 1)." `
                $GdbOutput
            if ($presentDelta -eq 1) {
                Assert-Condition ($activePhase -eq -1) `
                    "Task 25R profile-0 trace attributed one presentation to multiple phases." `
                    $GdbOutput
                $activePhase = $phase
            } else {
                Assert-Condition ($presentDelta -eq 0 -and $slipDelta -eq 0) `
                    "Task 25R profile-0 trace changed a non-owning phase counter." `
                    $GdbOutput
            }
            $intervalSlips += $slipDelta
        }
        Assert-Condition ($activePhase -ge 0) `
            "Task 25R profile-0 trace did not attribute presentation $($index + 1) to one phase." `
            $GdbOutput
        $interval = 2 + $intervalSlips
        if ($null -ne $previousVBlank) {
            Assert-Condition (($row[0] - $previousVBlank) -eq $interval) `
                "Task 25R profile-0 VBlank delta disagreed with cumulative slip accounting at presentation $($index + 1)." `
                $GdbOutput
        }
        $bucket = if ($interval -eq 2) { 'vblank2' } elseif ($interval -eq 3) {
            'vblank3'
        } elseif ($interval -eq 4) { 'vblank4' } else { 'vblank5Plus' }
        $histogram[$bucket]++
        $phaseHistograms[$activePhase][$bucket]++
        $phaseHistograms[$activePhase].slips += $intervalSlips
        $totalSlips += $intervalSlips
        $previous = $row
        $previousVBlank = $row[0]
    }
    for ($phase = 0; $phase -lt 5; $phase++) {
        Assert-Condition ($previous[4 + $phase] -eq $Pacing[12 + $phase] -and
            $previous[9 + $phase] -eq $Pacing[17 + $phase]) `
            "Task 25R profile-0 trace did not reconcile phase $phase with BPLAY_PACE." `
            $GdbOutput
    }
    Assert-Condition ($previous[1] -eq $Pacing[3] -and
        $previous[2] -eq $Pacing[2] -and $previous[3] -eq $Pacing[11] -and
        $totalSlips -eq (($Pacing[17..21] | Measure-Object -Sum).Sum)) `
        'Task 25R profile-0 trace did not reconcile its final pacing totals.' `
        $GdbOutput

    return [PSCustomObject]@{
        rows = $rows
        histogram = $histogram
        phases = $phaseHistograms
        presentations = [int64]$Pacing[3]
        sourceUpdates = [int64]$Pacing[2]
        presentationRateX10 = [int64]$Pacing[6]
        sourceUpdateRateX10 = [int64]$Pacing[7]
        slips = $totalSlips
        stable30 = ($histogram.vblank3 -eq 0 -and
            $histogram.vblank4 -eq 0 -and $histogram.vblank5Plus -eq 0)
    }
}
if (-not $env:DEVKITPRO) { $env:DEVKITPRO = 'C:/devkitPro' }
if (-not $env:DEVKITARM) { $env:DEVKITARM = 'C:/devkitPro/devkitARM' }
$renderEconomyCompileOwnerMask = if ($RenderEconomyMode -eq 1) { 255 } else { 0 }
$makeArgs = @('-C', $root, "TARGET=$Target", "BUILD=$Build", "NDS_DEV_SCENE_HARNESS=$Harness", '-j16')
$makeArgs += "NDS_RENDERER_PROFILE_LEVEL=$RendererProfileLevel"
$makeArgs += "NDS_RENDERER_M2_DETAILED_LEDGER=$([int]$RendererM2DetailedLedger.IsPresent)"
$makeArgs += "NDS_RENDERER_M3_PHASE0_PROFILE=$([int]$RendererM3Phase0Profile.IsPresent)"
$makeArgs += "NDS_NATIVE_STAGE_GENERATED_SEGMENT0_ENABLE=$effectiveNativeStageGeneratedSegment0Enable"
$makeArgs += "NDS_TASK29_GX_CENSUS=$([int]$Task29GXCensus.IsPresent)"
$makeArgs += "NDS_TASK34_STAGE_STREAM_CENSUS=$([int]$Task34StageStreamCensus.IsPresent)"
$makeArgs += "NDS_TASK36_HW_COMPOSE=$effectiveTask36HwComposeMode"
$makeArgs += "NDS_TASK22_WALLPAPER_RUN_LAB=$([int]$Task22WallpaperRunLab.IsPresent)"
$makeArgs += "NDS_FAST_WALLPAPER_AFFINE=$effectiveFastWallpaperAffineMode"
$makeArgs += "NDS_RENDERER_SCREEN_SPACE_CENSUS=$RendererScreenSpaceCensusMode"
$makeArgs += "NDS_RENDER_ECONOMY=$RenderEconomyMode"
$makeArgs += "NDS_RENDER_ECONOMY_OWNER_MASK=$renderEconomyCompileOwnerMask"
$makeArgs += "NDS_IFCOMMON_HYBRID_OAM=$IFCommonHybridOamMode"
$makeArgs += "NDS_TASK9_FLOAT_CENSUS=$Task9FloatCensusMode"
$makeArgs += "NDS_TASK9_FLOAT_ITCM=$Task9FloatItcmMode"
$makeArgs += "NDS_TASK9_FLOAT_PHASE2=$Task9FloatPhase2Mode"
$makeArgs += "NDS_TASK16_FLOAT_COMPARE=$effectiveTask16FloatCompareMode"
$makeArgs += "NDS_TASK16_FLOAT_I2F=$effectiveTask16FloatI2fMode"
$makeArgs += "NDS_TASK16_FLOAT_ADDSUB=$effectiveTask16FloatAddSubMode"
$makeArgs += "NDS_TASK9_STATE_HASH=$Task9StateHashMode"
$makeArgs += "NDS_TASK20_STACK_PROFILE=$Task20StackProfileMode"
$makeArgs += "NDS_TASK32_DRAW_HOT_TEXT=$effectiveTask32DrawHotTextMode"
if ($ImportBattleShipFTManager) {
    $makeArgs += 'NDS_IMPORT_BATTLESHIP_FTMANAGER=1'
}
if ($ImportBattleShipFTComputer) {
    $makeArgs += 'NDS_IMPORT_BATTLESHIP_FTCOMPUTER=1'
}
if ($ImportBattleShipVSResults) {
    $makeArgs += 'NDS_IMPORT_BATTLESHIP_VS_RESULTS=1'
}
if ($ImportBattleShipBattlePlayable) {
    $makeArgs += 'NDS_IMPORT_BATTLESHIP_BATTLE_PLAYABLE=1'
}
if ($ImportBattleShipIFCommon) {
    $makeArgs += 'NDS_IMPORT_BATTLESHIP_IFCOMMON=1'
}
if ($ImportBattleShipMarioFireball) {
    $makeArgs += 'NDS_IMPORT_BATTLESHIP_MARIO_FIREBALL=1'
}
if ($ImportBattleShipFoxBlaster) {
    $makeArgs += 'NDS_IMPORT_BATTLESHIP_FOX_BLASTER=1'
}
if ($ImportBattleShipEffectManager) {
    $makeArgs += 'NDS_IMPORT_BATTLESHIP_EFFECT_MANAGER=1'
}
if ($ImportBattleShipFoxReflector) {
    $makeArgs += 'NDS_IMPORT_BATTLESHIP_FOX_REFLECTOR=1'
}
if ($ImportBattleShipMarioSpecialHi) {
    $makeArgs += 'NDS_IMPORT_BATTLESHIP_MARIO_SPECIAL_HI=1'
}
if ($ImportBattleShipMarioSpecialLw) {
    $makeArgs += 'NDS_IMPORT_BATTLESHIP_MARIO_SPECIAL_LW=1'
}
if ($ImportBattleShipFoxSpecialHi) {
    $makeArgs += 'NDS_IMPORT_BATTLESHIP_FOX_SPECIAL_HI=1'
}
if ($ImportBattleShipAudioAssets) {
    $makeArgs += 'NDS_IMPORT_BATTLESHIP_AUDIO_ASSETS=1'
}
if ($ImportBattleShipAudioBGM) {
    $makeArgs += 'NDS_IMPORT_BATTLESHIP_AUDIO_BGM=1'
}
if ($ImportBattleShipNormalMoveset) {
    $makeArgs += 'NDS_IMPORT_BATTLESHIP_NORMAL_MOVESET=1'
}
if ($HardwareTriangles) {
    $makeArgs += 'NDS_RENDERER_HW_TRIANGLES=1'
}
if ($LiveInputPreview) {
    $makeArgs += 'NDS_DEV_LIVE_INPUT_PREVIEW=1'
}
if ($RealtimePresentation) {
    $makeArgs += 'NDS_HARNESS_FAST_LOGIC=0'
} else {
    $makeArgs += 'NDS_HARNESS_FAST_LOGIC=1'
}
if (-not $NoBuild) {
    & make @makeArgs
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}
if (-not (Test-Path $rom) -or -not (Test-Path $elf)) {
    throw "$Label harness build did not produce the expected ROM and ELF."
}
$bg0BuildDirectory = Resolve-Smash64DSBuildPath -Root $root -Build $Build
$bg0BuildConfig = Join-Path $bg0BuildDirectory 'nds_build_config.h'
Assert-Condition (Test-Path -LiteralPath $bg0BuildConfig -PathType Leaf) `
    'Built fast-wallpaper configuration is missing; refusing stale evidence.' `
    $bg0BuildConfig
$bg0BuildConfigText = Get-Content -LiteralPath $bg0BuildConfig -Raw
Assert-Condition (
    $bg0BuildConfigText -match '(?m)^#define NDS_SHIP_TELEMETRY 1$' -and
    $bg0BuildConfigText -match '(?m)^#define NDS_TICK_HUD 0$'
) 'GDB proof runs require full telemetry and must not use TICKHUD.' `
    $bg0BuildConfigText
Assert-Condition ($bg0BuildConfigText -match
    "(?m)^#define NDS_FAST_WALLPAPER_AFFINE $effectiveFastWallpaperAffineMode$") `
    'Built fast-wallpaper configuration does not match the requested selector.' `
    $bg0BuildConfigText
Assert-Condition ($bg0BuildConfigText -match
    "(?m)^#define NDS_TASK36_HW_COMPOSE $effectiveTask36HwComposeMode$") `
    'Built Task 36 hardware-compose configuration does not match the requested selector.' `
    $bg0BuildConfigText
if ($nativeStageGeneratedSegment0Selected) {
    $task26BuildDirectory = Resolve-Smash64DSBuildPath `
        -Root $root -Build $Build
    $task26BuildConfig = Join-Path $task26BuildDirectory 'nds_build_config.h'
    Assert-Condition (Test-Path -LiteralPath $task26BuildConfig -PathType Leaf) `
        'Built generated-segment0 configuration is missing; refusing stale -NoBuild evidence.' `
        $task26BuildConfig
    $task26BuildConfigText = Get-Content -LiteralPath $task26BuildConfig -Raw
    Assert-Condition ($task26BuildConfigText -match
        "(?m)^#define NDS_NATIVE_STAGE_GENERATED_SEGMENT0_ENABLE $effectiveNativeStageGeneratedSegment0Enable$") `
        'Built generated-segment0 configuration does not match the requested selector; refusing stale -NoBuild evidence.' `
        $task26BuildConfigText
    if ($RendererProfileLevel -eq 1) {
        $nm = Join-Path $env:DEVKITARM 'bin/arm-none-eabi-nm.exe'
        if (-not (Test-Path -LiteralPath $nm -PathType Leaf)) {
            throw "ARM symbol reader not found: $nm"
        }
        $nmOutput = @(& $nm $elf 2>&1 | ForEach-Object { "$_" })
        if ($LASTEXITCODE -ne 0) {
            throw "ARM symbol query failed.`n$($nmOutput -join "`n")"
        }
        $hasGeneratedAttempt = [bool]($nmOutput -match
            '\bgNdsRendererM3GeneratedSegment0AttemptCount$')
        $hasGeneratedHot = [bool]($nmOutput -match
            '\bsNdsNativeStageSegment0HotRuns$')
        $expectGeneratedSymbols =
            ($effectiveNativeStageGeneratedSegment0Enable -eq 1)
        Assert-Condition (
            ($hasGeneratedAttempt -eq $expectGeneratedSymbols) -and
            ($hasGeneratedHot -eq $expectGeneratedSymbols)
        ) 'Built profile-1 ELF generated-segment0 symbols do not match the requested selector; refusing stale -NoBuild evidence.' `
            ($nmOutput -join "`n")
    }
}
if ($Task29GXCensus) {
    $task29BuildDirectory = Resolve-Smash64DSBuildPath `
        -Root $root -Build $Build
    $task29BuildConfig = Join-Path $task29BuildDirectory 'nds_build_config.h'
    $task29BuildConfigText = Get-Content -LiteralPath $task29BuildConfig -Raw
    Assert-Condition (
        (Test-Path -LiteralPath $task29BuildConfig -PathType Leaf) -and
        ($task29BuildConfigText -match
            '(?m)^#define NDS_TASK29_GX_CENSUS 1$')
    ) 'Built Task 29 GX census configuration is absent or stale.' `
        $task29BuildConfig
    $task29Nm = Join-Path $env:DEVKITARM 'bin/arm-none-eabi-nm.exe'
    $task29NmOutput = @(& $task29Nm $elf 2>&1 | ForEach-Object { "$_" })
    Assert-Condition (
        $LASTEXITCODE -eq 0 -and
        ($task29NmOutput -match '\bgNdsTask29GXCommandCount$') -and
        ($task29NmOutput -match '\bgNdsTask29GXNeverSuppressMask$')
    ) 'Built Task 29 GX census ELF lacks its fail-closed command/mask exports.' `
        ($task29NmOutput -join "`n")
}
if ($Task34StageStreamCensus) {
    $task34BuildDirectory = Resolve-Smash64DSBuildPath -Root $root -Build $Build
    $task34BuildConfig = Join-Path $task34BuildDirectory 'nds_build_config.h'
    $task34BuildConfigText = Get-Content -LiteralPath $task34BuildConfig -Raw
    $task34Nm = Join-Path $env:DEVKITARM 'bin/arm-none-eabi-nm.exe'
    $task34NmOutput = @(& $task34Nm $elf 2>&1 | ForEach-Object { "$_" })
    Assert-Condition (
        (Test-Path -LiteralPath $task34BuildConfig -PathType Leaf) -and
        ($task34BuildConfigText -match
            '(?m)^#define NDS_TASK34_STAGE_STREAM_CENSUS 1$') -and
        ($task34BuildConfigText -match
            ("(?m)^#define NDS_TASK29_GX_CENSUS {0}$" -f
             [int]$Task29GXCensus.IsPresent)) -and
        ($task34BuildConfigText -match
            '(?m)^#define NDS_FAST_WALLPAPER_AFFINE 0$') -and
        $LASTEXITCODE -eq 0 -and
        ($task34NmOutput -match '\bgNdsTask34StageStreamEntries$') -and
        ($task34NmOutput -match '\bgNdsTask34StageStreamCaptureEnabled$') -and
        ($task34NmOutput -match '\bgNdsTaskmanArenaChosenSize$') -and
        ($task34NmOutput -match '\bgNdsTaskmanArenaAllocFailCount$')
    ) 'Built Task 34 standalone stream census is stale, affine-enabled, or missing its arena/stream exports.' `
        (($task34NmOutput + $task34BuildConfigText) -join "`n")
}
if ($Task9FloatItcmMode -eq 1) {
    $task9BuildDirectory = Resolve-Smash64DSBuildPath `
        -Root $root -Build $Build
    & (Join-Path $PSScriptRoot 'check-task9-float-itcm.ps1') `
        -Elf $elf `
        -BuildDirectory $task9BuildDirectory `
        -Phase2Mode $Task9FloatPhase2Mode `
        -Task16CompareMode $effectiveTask16FloatCompareMode `
        -Task16I2fMode $effectiveTask16FloatI2fMode `
        -Task16AddSubMode $effectiveTask16FloatAddSubMode
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}
if (($Task9FloatItcmMode -eq 1) -or
    ($Target -match '^smash64ds-battle-playable-(?:hwtri|coarse(?:-[a-z-]+)?-hwtri|forensic-hwtri)$')) {
    & (Join-Path $PSScriptRoot 'check-renderer-itcm-placement.ps1') `
        -Elf $elf `
        -BenchmarkAblation:($Target -like '*triangle-noop*')
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}
& (Join-Path $PSScriptRoot 'check-task20-dtcm-layout.ps1') -Elf $elf
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
if (-not (Test-Path $melonDsPath)) { throw "melonDS executable not found: $melonDsPath" }
if (-not (Test-Path $Gdb)) { throw "GDB executable not found: $Gdb" }
if (($RendererBenchmarkSamples -gt 0) -or $Task25RPacingTrace) {
    $benchmarkMakeIdentity = Get-BenchmarkMakeIdentity -BaseMakeArgs $makeArgs
    $isCpuPrepNoGx = $benchmarkMakeIdentity.RendererBenchmarkMode -eq 2
    Assert-Condition ($benchmarkMakeIdentity.Target -eq $Target -and
        $benchmarkMakeIdentity.Harness -eq $Harness -and
        $benchmarkMakeIdentity.HarnessId -eq $ExpectedMode -and
        $benchmarkMakeIdentity.Profile -eq $RendererProfileLevel -and
        $benchmarkMakeIdentity.ShipTelemetry -eq 1 -and
        $benchmarkMakeIdentity.TickHud -eq 0 -and
        $benchmarkMakeIdentity.M2DetailedLedger -eq
            [int]$RendererM2DetailedLedger.IsPresent -and
        $benchmarkMakeIdentity.M3Phase0Profile -eq
            [int]$RendererM3Phase0Profile.IsPresent -and
        $benchmarkMakeIdentity.NativeStageGeneratedSegment0Enable -eq
            $effectiveNativeStageGeneratedSegment0Enable -and
        $benchmarkMakeIdentity.Task29GXCensus -eq
            [int]$Task29GXCensus.IsPresent -and
        $benchmarkMakeIdentity.Task34StageStreamCensus -eq
            [int]$Task34StageStreamCensus.IsPresent -and
        $benchmarkMakeIdentity.Task36HwComposeMode -eq
            $effectiveTask36HwComposeMode -and
        $benchmarkMakeIdentity.Task22WallpaperRunLab -eq
            [int]$Task22WallpaperRunLab.IsPresent -and
        $benchmarkMakeIdentity.FastWallpaperAffine -eq
            $effectiveFastWallpaperAffineMode -and
        $benchmarkMakeIdentity.ScreenSpaceCensusMode -eq
            $RendererScreenSpaceCensusMode -and
        $benchmarkMakeIdentity.RenderEconomyMode -eq
            $RenderEconomyMode -and
        $benchmarkMakeIdentity.RenderEconomyOwnerMask -eq
            $renderEconomyCompileOwnerMask -and
        $benchmarkMakeIdentity.Task9FloatCensusMode -eq
            $Task9FloatCensusMode -and
        $benchmarkMakeIdentity.Task9FloatItcmMode -eq
            $Task9FloatItcmMode -and
        $benchmarkMakeIdentity.Task9FloatPhase2Mode -eq
            $Task9FloatPhase2Mode -and
        $benchmarkMakeIdentity.Task16FloatCompareMode -eq
            $effectiveTask16FloatCompareMode -and
        $benchmarkMakeIdentity.Task16FloatI2fMode -eq
            $effectiveTask16FloatI2fMode -and
        $benchmarkMakeIdentity.Task16FloatAddSubMode -eq
            $effectiveTask16FloatAddSubMode -and
        $benchmarkMakeIdentity.Task20StackProfileMode -eq
            $Task20StackProfileMode -and
        $benchmarkMakeIdentity.Task32DrawHotTextMode -eq
            $effectiveTask32DrawHotTextMode -and
        $benchmarkMakeIdentity.IFCommonHybridOamMode -eq
            $effectiveIFCommonHybridOamMode) `
        'Makefile benchmark identity does not match the requested verifier target/harness/profile/M2/M3/Task26/Task29/M4/Task11/IFCommon/Task9/Task16 configuration.' `
        ($benchmarkMakeIdentity | Format-List | Out-String)
    if ($usesPublishedIntrinsicRendererDefaults) {
        Assert-Condition (
            $benchmarkMakeIdentity.FastRunDefault -eq 9 -and
            $benchmarkMakeIdentity.NativeStageGeneratedSegment0Enable -eq 1 -and
            $benchmarkMakeIdentity.SceneMipCacheLab -eq 0 -and
            $benchmarkMakeIdentity.FastWallpaperAffine -eq 1 -and
            $benchmarkMakeIdentity.BattleStaticTextureDefault -eq 1
        ) 'Published battle renderer build identity is not the intrinsic M3/M4 9/0/1 configuration.' `
            ($benchmarkMakeIdentity | Format-List | Out-String)
    }
    if ($benchmarkMakeIdentity.RendererBenchmarkMode -eq 4) {
        Assert-Condition (
            $effectiveStaticTextureAotMode -eq 0 -and
            $benchmarkMakeIdentity.BattleStaticTextureDefault -eq 0
        ) 'WARM_NO_UPLOAD is a static-off animated-refresh ablation and cannot run with effective static residency.' `
            ($benchmarkMakeIdentity | Format-List | Out-String)
    }
    $benchmarkRomIdentity = Get-BenchmarkFileIdentity -Path $rom
    $benchmarkElfIdentity = Get-BenchmarkFileIdentity -Path $elf
    $melonItem = Get-Item -LiteralPath $melonDsPath
    $melonVersion = $melonItem.VersionInfo.ProductVersion
    if ([string]::IsNullOrWhiteSpace($melonVersion)) {
        $melonVersion = $melonItem.VersionInfo.FileVersion
    }
    if ([string]::IsNullOrWhiteSpace($melonVersion)) {
        $melonVersion = 'unknown'
    }
    $benchmarkMelonIdentity = [PSCustomObject]@{
        Version = $melonVersion
        Sha256 = (Get-FileHash -LiteralPath $melonDsPath -Algorithm SHA256).Hash
        Path = $melonDsPath
    }
}
New-Item -ItemType Directory -Path $logDir -Force | Out-Null
try {
    $configState = Enable-MelonDSGdbConfig -MelonDSPath $melonDsPath `
        -GdbPort $verifierContext.GdbPort `
        -Persistent:([bool]$verifierContext.PersistentConfig) `
        -MuteAudio:$OneMinuteMatchProof
    if (($RendererBenchmarkSamples -gt 0) -or $Task25RPacingTrace) {
        $benchmarkMelonConfigSha256 =
            (Get-FileHash -LiteralPath $configState.Config -Algorithm SHA256).Hash
    }
    Remove-Item $stdout, $stderr -Force -ErrorAction SilentlyContinue
    $emulator = Start-Process -FilePath $melonDsPath `
        -ArgumentList $rom `
        -WorkingDirectory $melonDsDir `
        -RedirectStandardOutput $stdout `
        -RedirectStandardError $stderr `
        -WindowStyle Hidden `
        -PassThru
    Wait-MelonDSGdbListener -Process $emulator -Port $verifierContext.GdbPort | Out-Null
    $benchmarkScreenshotCommands = @()
    if (-not [string]::IsNullOrWhiteSpace($RendererBenchmarkScreenshot)) {
        $visibilityDir = [System.IO.Path]::GetFullPath(
            (Join-Path $root 'artifacts\visibility'))
        $screenshotPath = if ([System.IO.Path]::IsPathRooted(
                $RendererBenchmarkScreenshot)) {
            [System.IO.Path]::GetFullPath($RendererBenchmarkScreenshot)
        } else {
            [System.IO.Path]::GetFullPath(
                (Join-Path $root $RendererBenchmarkScreenshot))
        }
        $visibilityPrefix = $visibilityDir.TrimEnd('\', '/') +
            [System.IO.Path]::DirectorySeparatorChar
        if (-not $screenshotPath.StartsWith(
                $visibilityPrefix,
                [System.StringComparison]::OrdinalIgnoreCase)) {
            throw "Renderer benchmark screenshots must stay under '$visibilityDir'."
        }
        $captureScript = [System.IO.Path]::GetFullPath(
            (Join-Path $PSScriptRoot 'capture-running-melonds-window.ps1'))
        $captureCommand =
            'shell powershell.exe -NoProfile -ExecutionPolicy Bypass -File "{0}" -EmulatorProcessId {1} -Output "{2}"' -f
            $captureScript.Replace('\', '/'),
            $emulator.Id,
            $screenshotPath.Replace('\', '/')
        $benchmarkScreenshotCommands = @(
            ('if $renderer_benchmark_samples == {0}' -f
                ($RendererBenchmarkSamples - 1)),
            $captureCommand,
            'end'
        )
    }
    $task25rScreenshotCommands = if ($Task25RPacingTrace -and
        -not [string]::IsNullOrWhiteSpace($RendererBenchmarkScreenshot)) {
        @(
            'if gNdsRendererProfileFrameCount == 607',
            $captureCommand,
            'end'
        )
    } else { @() }
    # The natural combat chain runs ~1000+ bounded updates; battle_playable
    # continues into an input-driven KO -> Rebirth -> Wait. The lifecycle ROM
    # uses a one-minute source timer and gets one GDB session after startup;
    # repeated attach/detach cycles are unreliable in the melonDS GDB stub.
    $minimumDelay = if ($preBattleSelectorSelected) {
        # Runtime selectors must be installed after C initialization but
        # before VSBattle setup consumes them.
        0
    } elseif (($RendererBenchmarkStartFrame -gt 0) -or
              ($RendererBenchmarkStartEvent -ne 'None')) {
        # An exact frame gate is the warmup. Attach before the scene starts so
        # a caller delay cannot race past the requested synchronized window.
        0
    } elseif ($OneMinuteMatchProof) {
        # Attach near startup, prove the exact 1:00 state, then let one GDB
        # session run to Results. This avoids a fragile wall-clock sleep.
        0
    } elseif ($MatchLifecycleProof) {
        85
    } elseif ($BattlePlayable -and $RealtimePresentation -and
              ($RendererBenchmarkSamples -gt 0)) {
        # The synchronized breakpoint itself captures the requested warm
        # sample span. Eight seconds lets both canonical texture uploads
        # settle while keeping 128-frame profiles inside one logic-clock
        # epoch at the current uncapped benchmark rate.
        8
    } elseif ($BattlePlayable -and $RealtimePresentation) {
        12
    } elseif ($BattlePlayable) {
        3
    } else {
        15
    }
    $effectiveDelaySeconds = if ($preBattleSelectorSelected -or
                                 ($RendererBenchmarkStartFrame -gt 0) -or
                                 ($RendererBenchmarkStartEvent -ne 'None')) {
        0
    } else {
        [Math]::Max($DelaySeconds, $minimumDelay)
    }
    if ($effectiveDelaySeconds -gt 0) {
        Start-Sleep -Seconds $effectiveDelaySeconds
    }
    $gdbRemoteTimeoutSeconds = if ($OneMinuteMatchProof) { 300 } else { 5 }
    $gdbCaptureTimeoutSeconds = if ($OneMinuteMatchProof) {
        300
    } else {
        $RendererBenchmarkTimeoutSeconds
    }
    $gdbCommands = @(
        'set pagination off',
        'set confirm off',
        ("set remotetimeout {0}" -f $gdbRemoteTimeoutSeconds),
        ("target remote 127.0.0.1:{0}" -f (Get-MelonDSActiveGdbPort)),
        'printf "AOBJ32=%u,%u,%u,%u,%#x,%#x\n", gNdsAObjEvent32NormalizeScriptCount, gNdsAObjEvent32NormalizeCommandCount, gNdsAObjEvent32NormalizeReuseCount, gNdsAObjEvent32NormalizeFailCount, gNdsAObjEvent32NormalizeFirstSourceWord, gNdsAObjEvent32NormalizeFirstNativeWord',
        'printf "AOBJ32_FAIL=%u,%u,%#x,%#x,%u,%#x\n", gNdsAObjEvent32NormalizeLastFailReason, gNdsAObjEvent32NormalizeLastFailOwner, gNdsAObjEvent32NormalizeLastFailAddress, gNdsAObjEvent32NormalizeLastFailWord, gNdsAObjEvent32NormalizeLastFailOpcode, gNdsAObjEvent32NormalizeLastFailFlags',
        'printf "MOBJ_ATTACH=%u,%u,%u,%#x,%#x\n", gNdsMObjSubAttachNormalizeCount, gNdsMObjSubAttachNativeCount, gNdsMObjSubAttachFailCount, gNdsMObjSubAttachFirstSourceFlags, gNdsMObjSubAttachFirstNativeFlags',
        'printf "HARN=%#x,%u,%u,%u,%#x\n", gNdsSceneHarnessResult, gNdsSceneHarnessMode, gNdsSceneHarnessSceneCurr, gNdsSceneHarnessScenePrev, gNdsSceneHarnessReservedMask',
        'printf "SCENE=%u,%u,%u\n", gSCManagerSceneData.scene_curr, gSCManagerSceneData.scene_prev, gSCManagerSceneData.gkind',
        'printf "BUILD_MODE=%#x,%#x,%#x\n", gNdsBuildModeCanonicalWord, gNdsBuildModeShippedWord, gNdsBuildModeFastWord',
        'printf "VS_TRANS=%#x,%#x\n", gNdsVSModeStartTransitionResult, gNdsVSModeStartTransitionMask',
        'printf "PV_TRANS=%#x,%#x\n", gNdsPlayersVSReadyTransitionResult, gNdsPlayersVSReadyTransitionMask',
        'printf "MAPS_TRANS=%#x,%#x,%u\n", gNdsMapsSelectTransitionResult, gNdsMapsSelectTransitionMask, gNdsMapsSelectTransitionSelectedGKind',
        'printf "PREV_LOOP=%#x,%#x,%#x,%#x,%u\n", gNdsFighterMarioFoxPreviewLoopResult, gNdsFighterMarioFoxPreviewLoopSafeResult, gNdsFighterMarioFoxPreviewLoopMask, gNdsFighterMarioFoxPreviewLoopDeferredMask, gNdsFighterMarioFoxPreviewLoopCount',
        'printf "GCRUNALL_LOOP=%#x,%#x,%#x,%#x,%u,%u,%u\n", gNdsFighterMarioFoxGCRunAllLoopResult, gNdsFighterMarioFoxGCRunAllLoopSafeResult, gNdsFighterMarioFoxGCRunAllLoopMask, gNdsFighterMarioFoxGCRunAllLoopDeferredMask, gNdsFighterMarioFoxGCRunAllLoopCount, gNdsFighterGCRunAllLoopFrameMax, gNdsFighterGCRunAllLoopUpdateMax',
        'printf "GCRUNALL_TASKMAN=%u,%u,%u,%u,%u,%u\n", gNdsFighterGCRunAllLoopPrepared, gNdsFighterGCRunAllLoopTaskmanUpdateCount, gNdsFighterGCRunAllLoopVSBattleUpdateCount, gNdsFighterGCRunAllLoopBaseVSBattleUpdateCount, gNdsFighterGCRunAllLoopRunAllCount, gNdsTaskmanBoundedUpdateCount',
        'printf "NAT_MOTION=%#x,%#x,%#x,%u,%u,%u,%u,%u,%#x,%u,%u\n", gNdsFighterNaturalMotionResult, gNdsFighterNaturalMotionSafeResult, gNdsFighterNaturalMotionMask, gNdsFighterNaturalMotionPrepared, gNdsFighterNaturalMotionUpdateCount, gNdsFighterNaturalMotionBaseVSBattleUpdateCount, gNdsFighterNaturalMotionRunAllCount, gNdsFighterNaturalMotionControllerReadCount, gNdsFighterNaturalMotionManagerMask, gNdsFighterNaturalMotionGObjDelta, gNdsFighterNaturalMotionUnsafeCount',
        'printf "NAT_FIG=%u,%u,%u,%u\n", gNdsFighterNaturalMotionFigatreeAttachCount, gNdsFighterNaturalMotionFigatreeNullCount, gNdsFighterNaturalMotionFigatreeTableInvalidCount, gNdsFighterNaturalMotionFigatreeAnimInvalidCount',
        'printf "NAT_WAIT=%u,%u,%u,%u,%u,%u,%#x,%#x,%#x,%#x\n", gNdsFighterNaturalMotionP0WaitFrameCount, gNdsFighterNaturalMotionP1WaitFrameCount, gNdsFighterNaturalMotionP0AnimAdvanceCount, gNdsFighterNaturalMotionP1AnimAdvanceCount, gNdsFighterNaturalMotionP0ValidJointCount, gNdsFighterNaturalMotionP1ValidJointCount, gNdsFighterNaturalMotionP0AnimStartBits, gNdsFighterNaturalMotionP1AnimStartBits, gNdsFighterNaturalMotionP0AnimFinalBits, gNdsFighterNaturalMotionP1AnimFinalBits',
        'printf "NAT_WALK=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsFighterNaturalMotionWalkInputFrame, gNdsFighterNaturalMotionP0WalkFrameCount, gNdsFighterNaturalMotionP1WalkFrameCount, gNdsFighterNaturalMotionP0StatusStart, gNdsFighterNaturalMotionP1StatusStart, gNdsFighterNaturalMotionP0StatusFinal, gNdsFighterNaturalMotionP1StatusFinal, gNdsFighterNaturalMotionP0WalkStatus, gNdsFighterNaturalMotionP1WalkStatus, gNdsFighterNaturalMotionP0WalkMotion, gNdsFighterNaturalMotionP1WalkMotion',
        'printf "NAT_CHAIN=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsFighterNaturalCombatPhase, gNdsFighterNaturalCombatPhaseFrames, gNdsFighterNaturalCombatStallCount, gNdsFighterNaturalCombatP0DashFrames, gNdsFighterNaturalCombatP1DashFrames, gNdsFighterNaturalCombatP0RunFrames, gNdsFighterNaturalCombatP1RunFrames, gNdsFighterNaturalCombatP0RunBrakeFrames, gNdsFighterNaturalCombatP1RunBrakeFrames, gNdsFighterNaturalCombatP0TurnFrames, gNdsFighterNaturalCombatP1TurnFrames, gNdsFighterNaturalCombatApproachDXMilli',
        'printf "NAT_ATTACK=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsFighterNaturalCombatAttackerSlot, gNdsFighterNaturalCombatVictimSlot, gNdsFighterNaturalCombatAttackStatusFrames, gNdsFighterNaturalCombatAttackMotionFinal, gNdsFighterNaturalCombatHitboxActiveFrames, gNdsFighterNaturalCombatAttackRetryCount, gNdsFighterNaturalCombatVictimDamageStatus, gNdsFighterNaturalCombatVictimDamageFrames, gNdsFighterNaturalCombatVictimStartPercent, gNdsFighterNaturalCombatVictimFinalPercent, gNdsFighterNaturalCombatVictimKnockbackMilli, gNdsFighterNaturalCombatVictimRecoverWaitFrames',
        'printf "NAT_MOVESET=%#x,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%d,%u,%u,%u,%d,%u,%u\n", gNdsFighterNaturalMovesetMask, gNdsFighterNaturalMovesetPhase, gNdsFighterNaturalMovesetPhaseFrames, gNdsFighterNaturalMovesetTiltS3Frames, gNdsFighterNaturalMovesetTiltHi3Frames, gNdsFighterNaturalMovesetTiltLw3Frames, gNdsFighterNaturalMovesetTiltHitboxFrames, gNdsFighterNaturalMovesetSmashFrames, gNdsFighterNaturalMovesetSmashHitboxFrames, gNdsFighterNaturalMovesetAerialFrames, gNdsFighterNaturalMovesetAerialHitboxFrames, gNdsFighterNaturalMovesetLandingFrames, gNdsFighterNaturalMovesetCatchFrames, gNdsFighterNaturalMovesetCatchWaitFrames, gNdsFighterNaturalMovesetThrowFrames, gNdsFighterNaturalMovesetThrownFrames, gNdsFighterNaturalMovesetThrowRecoverFrames, gNdsFighterNaturalMovesetAttackerStatus, gNdsFighterNaturalMovesetAttackerMotion, gNdsFighterNaturalMovesetAttackerGA, gNdsFighterNaturalMovesetAttackerRootYMilli, gNdsFighterNaturalMovesetVictimStatus, gNdsFighterNaturalMovesetVictimMotion, gNdsFighterNaturalMovesetVictimGA, gNdsFighterNaturalMovesetVictimRootYMilli, gNdsFighterNaturalMovesetThrowDamageBefore, gNdsFighterNaturalMovesetThrowDamageAfter',
        'printf "NAT_HITBOX=%u,%u,%u,%u,%u,%d,%d,%d,%d,%d,%d,%d,%d,%d,%#x\n", gNdsFighterDashRunAttackEventLastPlayer, gNdsFighterDashRunAttackEventLastStatus, gNdsFighterDashRunAttackEventLastState, gNdsFighterDashRunAttackEventLastAttackID, gNdsFighterDashRunAttackEventLastGroupID, gNdsFighterDashRunAttackEventLastDamage, gNdsFighterDashRunAttackEventLastSize, gNdsFighterDashRunAttackEventLastOffsetX, gNdsFighterDashRunAttackEventLastOffsetY, gNdsFighterDashRunAttackEventLastOffsetZ, gNdsFighterDashRunAttackEventLastAngle, gNdsFighterDashRunAttackEventLastKBG, gNdsFighterDashRunAttackEventLastKBW, gNdsFighterDashRunAttackEventLastBKB, gNdsFighterDashRunAttackEventLastFlags',
        'printf "NAT_HITLAG=%u,%u\n", gNdsFighterNaturalCombatP0HitlagFrames, gNdsFighterNaturalCombatP1HitlagFrames',
        'printf "NAT_GUARD=%u,%u,%u\n", gNdsFighterNaturalCombatGuardOnFrames, gNdsFighterNaturalCombatGuardFrames, gNdsFighterNaturalCombatGuardOffFrames',
        'printf "BPLAY_GEOM=%u,%#x,%#x,%#x,%#x,%#x,%#x,%#x,%u,%u,%u,%d,%d,%u,%u,%u,%u,%u,%u\n", gNdsSCVSBattleStageGroundDataReady, (unsigned int)gMPCollisionGroundData, (unsigned int)gMPCollisionGeometry, (unsigned int)(gMPCollisionGeometry ? gMPCollisionGeometry->line_info : 0), (unsigned int)(gMPCollisionGeometry ? gMPCollisionGeometry->vertex_links : 0), (unsigned int)(gMPCollisionGeometry ? gMPCollisionGeometry->vertex_id : 0), (unsigned int)(gMPCollisionGeometry ? gMPCollisionGeometry->vertex_data : 0), gNdsPupupuGroundDeferredMask, gNdsStageCollisionLoopGeometryReady, gNdsStageCollisionLoopGroundDataReady, gNdsStageCollisionLoopFloorLineCount, gNdsStageCollisionLoopFloorLineMin, gNdsStageCollisionLoopFloorLineMaxExclusive, gNdsStageMPSweepFloorLoopLineSweepDiffCallCount, gNdsStageMPSweepFloorLoopLineSweepDiffHitCount, gNdsStageMPSweepFloorLoopLineSweepDiffMissCount, gNdsStageMPSweepFloorLoopLineSweepVisitCount, gNdsStageMPSweepFloorLoopLineSweepCandidateCount, gNdsStageMPSweepFloorLoopLineSweepSameHitCount',
        'printf "BPLAY_WALLPAPER=%u,%#x\n", gNdsStagePupupuWallpaperPtrReady, (unsigned int)((gMPCollisionGroundData != 0) ? gMPCollisionGroundData->wallpaper : 0)',
        'printf "GCRUNALL_RUN=%u,%u,%u,%u,%u,%u\n", gNdsFighterGCRunAllLoopOldProcessPauseCount, gNdsFighterGCRunAllLoopNonTargetGObjVisitCount, gNdsFighterGCRunAllLoopNonTargetProcessPauseCount, gNdsFighterGCRunAllLoopTargetProcessPreserveCount, gNdsFighterGCRunAllLoopGObjCountBefore, gNdsFighterGCRunAllLoopGObjCountAfter',
        'printf "GCRUNALL_PROCESS=%u,%u,%u,%u,%u,%u,%u\n", gNdsFighterGCRunAllLoopP0ProcessAttachCount, gNdsFighterGCRunAllLoopP1ProcessAttachCount, gNdsFighterGCRunAllLoopP0GObjProcessRunCount, gNdsFighterGCRunAllLoopP1GObjProcessRunCount, gNdsFighterGCRunAllLoopP0ProcCallbackCount, gNdsFighterGCRunAllLoopP1ProcCallbackCount, gNdsFighterGCRunAllLoopProcessAttachEscapeCount',
        'printf "GCRUNALL_INPUT=%u,%u,%u,%u,%u,%u,%u,%u,%#x,%#x,%#x,%#x,%u,%u\n", gNdsFighterGCRunAllLoopP0PlaybackApplyCount, gNdsFighterGCRunAllLoopP1PlaybackApplyCount, gNdsFighterGCRunAllLoopP0ControllerToFTInputCount, gNdsFighterGCRunAllLoopP1ControllerToFTInputCount, gNdsFighterGCRunAllLoopP0DirectFTInputWriteCount, gNdsFighterGCRunAllLoopP1DirectFTInputWriteCount, gNdsFighterGCRunAllLoopP0DashTapEligibleCount, gNdsFighterGCRunAllLoopP1DashTapEligibleCount, gNdsFighterGCRunAllLoopP0ButtonTapMask, gNdsFighterGCRunAllLoopP1ButtonTapMask, gNdsFighterGCRunAllLoopP0ButtonHoldMask, gNdsFighterGCRunAllLoopP1ButtonHoldMask, gNdsFighterGCRunAllLoopP0JumpButtonTapCount, gNdsFighterGCRunAllLoopP1JumpButtonTapCount',
        'printf "GCRUNALL_STATUS=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%#x,%#x,%#x,%#x\n", gNdsFighterGCRunAllLoopP0StatusStart, gNdsFighterGCRunAllLoopP1StatusStart, gNdsFighterGCRunAllLoopP0MotionStart, gNdsFighterGCRunAllLoopP1MotionStart, gNdsFighterGCRunAllLoopP0StatusFinal, gNdsFighterGCRunAllLoopP1StatusFinal, gNdsFighterGCRunAllLoopP0MotionFinal, gNdsFighterGCRunAllLoopP1MotionFinal, gNdsFighterGCRunAllLoopP0GAFinal, gNdsFighterGCRunAllLoopP1GAFinal, gNdsFighterGCRunAllLoopP0StatusVisitMask, gNdsFighterGCRunAllLoopP1StatusVisitMask, gNdsFighterGCRunAllLoopP0TransitionMask, gNdsFighterGCRunAllLoopP1TransitionMask',
        'printf "GCRUNALL_VISITS=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsFighterGCRunAllLoopP0WaitVisitCount, gNdsFighterGCRunAllLoopP1WaitVisitCount, gNdsFighterGCRunAllLoopP0WalkVisitCount, gNdsFighterGCRunAllLoopP1WalkVisitCount, gNdsFighterGCRunAllLoopP0DashVisitCount, gNdsFighterGCRunAllLoopP1DashVisitCount, gNdsFighterGCRunAllLoopP0RunVisitCount, gNdsFighterGCRunAllLoopP1RunVisitCount, gNdsFighterGCRunAllLoopP0RunBrakeVisitCount, gNdsFighterGCRunAllLoopP1RunBrakeVisitCount, gNdsFighterGCRunAllLoopP0KneeBendVisitCount, gNdsFighterGCRunAllLoopP1KneeBendVisitCount, gNdsFighterGCRunAllLoopP0JumpVisitCount, gNdsFighterGCRunAllLoopP1JumpVisitCount, gNdsFighterGCRunAllLoopP0FallVisitCount, gNdsFighterGCRunAllLoopP1FallVisitCount, gNdsFighterGCRunAllLoopP0LandingVisitCount, gNdsFighterGCRunAllLoopP1LandingVisitCount',
        'printf "GCRUNALL_CALLS=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsFighterGCRunAllLoopP0FrameCount, gNdsFighterGCRunAllLoopP1FrameCount, gNdsFighterGCRunAllLoopP0Completed, gNdsFighterGCRunAllLoopP1Completed, gNdsFighterGCRunAllLoopP0InterruptCount, gNdsFighterGCRunAllLoopP1InterruptCount, gNdsFighterGCRunAllLoopP0PhysicsCount, gNdsFighterGCRunAllLoopP1PhysicsCount, gNdsFighterGCRunAllLoopP0MapCount, gNdsFighterGCRunAllLoopP1MapCount',
        'printf "GCRUNALL_DRAW=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%#x,%#x\n", gNdsFighterGCRunAllLoopPreviewWidth, gNdsFighterGCRunAllLoopPreviewHeight, gNdsFighterGCRunAllLoopPreviewPitch, gNdsFighterGCRunAllLoopPreviewReady, gNdsFighterGCRunAllLoopPreviewCommitDelta, gNdsFighterGCRunAllLoopDrawFrameCount, gNdsFighterGCRunAllLoopDisplayCallbackCount, gNdsFighterGCRunAllLoopP0DisplayCallbackCount, gNdsFighterGCRunAllLoopP1DisplayCallbackCount, gNdsFighterGCRunAllLoopP0CandidateCount, gNdsFighterGCRunAllLoopP1CandidateCount, gNdsFighterGCRunAllLoopP0DrawnDObjCount, gNdsFighterGCRunAllLoopP1DrawnDObjCount, gNdsFighterGCRunAllLoopTotalPixelCount, gNdsFighterGCRunAllLoopP0ColorChecksum, gNdsFighterGCRunAllLoopP1ColorChecksum',
        'printf "GCRUNALL_SCREEN=%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n", gNdsFighterGCRunAllLoopP0ScreenXStart, gNdsFighterGCRunAllLoopP1ScreenXStart, gNdsFighterGCRunAllLoopP0ScreenXFinal, gNdsFighterGCRunAllLoopP1ScreenXFinal, gNdsFighterGCRunAllLoopP0ScreenXDelta, gNdsFighterGCRunAllLoopP1ScreenXDelta, gNdsFighterGCRunAllLoopP0ScreenYFloor, gNdsFighterGCRunAllLoopP1ScreenYFloor, gNdsFighterGCRunAllLoopP0ScreenRise, gNdsFighterGCRunAllLoopP1ScreenRise',
        'printf "GCRUNALL_MOVE=%d,%d,%d,%d,%d,%d,%d,%d,%u,%u,%u,%u\n", gNdsFighterGCRunAllLoopP0FloorYMilli, gNdsFighterGCRunAllLoopP1FloorYMilli, gNdsFighterGCRunAllLoopP0RootDeltaXMilli, gNdsFighterGCRunAllLoopP1RootDeltaXMilli, gNdsFighterGCRunAllLoopP0RootRiseMilli, gNdsFighterGCRunAllLoopP1RootRiseMilli, gNdsFighterGCRunAllLoopP0RootYFinalMilli, gNdsFighterGCRunAllLoopP1RootYFinalMilli, gNdsFighterGCRunAllLoopP0RootDirectionOK, gNdsFighterGCRunAllLoopP1RootDirectionOK, gNdsFighterGCRunAllLoopP0FloorOK, gNdsFighterGCRunAllLoopP1FloorOK',
        'printf "GCRUNALL_TRANS=%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsFighterGCRunAllLoopFallDetectCount, gNdsFighterGCRunAllLoopLandingDetectCount, gNdsFighterGCRunAllLoopSetGroundCount, gNdsFighterGCRunAllLoopSetAirCount, gNdsFighterGCRunAllLoopWaitSetStatusCount, gNdsFighterGCRunAllLoopRunBrakeEndCount, gNdsFighterGCRunAllLoopJumpAnimEndCount, gNdsFighterGCRunAllLoopLandingEndCount, gNdsFighterGCRunAllLoopDeferredInterruptCheckCount',
        'printf "GCRUNALL_SAFE=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsFighterGCRunAllLoopGObjDelta, gNdsFighterGCRunAllLoopUnexpectedStatusCount, gNdsFighterGCRunAllLoopDeniedStatusCount, gNdsFighterGCRunAllLoopProcessAttachEscapeCount, gNdsFighterGCRunAllLoopDisplayProbeCount, gNdsFighterGCRunAllLoopGameplayUpdateCount, gNdsFighterGCRunAllLoopDrawCallCount, gNdsFighterGCRunAllLoopMatrixCallCount, gNdsFighterGCRunAllLoopRootYDriftCount, gNdsFighterGCRunAllLoopGADriftCount',
        'printf "PLATFORM_DL_PREVIEW=%u,%u,%u,%u\n", gNdsOriginalDLPreviewReady, gNdsOriginalDLPreviewWidth, gNdsOriginalDLPreviewHeight, gNdsOriginalDLPreviewCommitCount',
        'printf "BOUNDARY=%#x,%u\n", gNdsSceneBoundaryResult, gNdsSceneBoundaryKind',
        'printf "LIVE_PAD=%u,%u,%#x,%#x,%#x,%d,%d,%#x,%d,%d,%u,%u,%#x,%#x,%d,%d,%#x,%#x,%d,%d,%d\n", gNdsControllerLiveReadCount, gNdsControllerLiveMapCount, gNdsControllerLiveConnectedMask, gNdsPlatformHeldKeys, gNdsControllerLivePad0Button, gNdsControllerLivePad0StickX, gNdsControllerLivePad0StickY, gNdsControllerLivePad1Button, gNdsControllerLivePad1StickX, gNdsControllerLivePad1StickY, gNdsControllerPlaybackEnabled, gNdsControllerPlaybackReadCount, gSYControllerDevices[0].button_hold, gSYControllerDevices[0].button_tap, gSYControllerDevices[0].stick_range.x, gSYControllerDevices[0].stick_range.y, gSYControllerDevices[1].button_hold, gSYControllerDevices[1].button_tap, gSYControllerDevices[1].stick_range.x, gSYControllerDevices[1].stick_range.y, gNdsFighterBattlePlayableFinalXMilli',
        'detach',
        'quit'
    )
    if ($OneMinuteMatchProof) {
        # BattleShip ifcommon.c:2533-2542 creates the 1:00 timer before the
        # first VSBattle update. Stop there once, then run naturally until the
        # imported Results recorder has observed its complete source setup.
        $task9StateStartCommands = if ($Task9StateHashMode -eq 1) {
            @(
                'set variable gNdsTask9StateHashArmed = 0',
                'set variable gNdsTask9StateHashCount = 0',
                'set variable gNdsTask9StateHashOverflow = 0',
                'set variable gNdsTask9StateHashArmed = 1'
            )
        } else { @() }
        $task9StateFinishCommands = if ($Task9StateHashMode -eq 1) {
            @(
                'set variable gNdsTask9StateHashArmed = 0',
                'printf "TASK9_STATE_SUMMARY=%u,%u,%u\n", gNdsTask9StateHashCount, gNdsTask9StateHashOverflow, 4096',
                'set $task9_state_index = 0',
                'while $task9_state_index < gNdsTask9StateHashCount',
                'printf "TASK9_STATE=%u,%u,%u,%u,%u,%u\n", $task9_state_index, gNdsTask9StateHashes[$task9_state_index].hash1, gNdsTask9StateHashes[$task9_state_index].hash2, gNdsTask9StateHashes[$task9_state_index].bytes, gNdsTask9StateHashes[$task9_state_index].records, gNdsTask9StateHashes[$task9_state_index].overflow',
                'set $task9_state_index = $task9_state_index + 1',
                'end'
            )
        } else { @() }
        $task25rPacingCommands = if ($Task25RPacingTrace) {
            @(
                'break ndsBattlePlayableFrameCompleteMarker if gNdsBattlePlayablePacingRestartRequested == 0 && gNdsBattlePlayablePacingPresentedFrames > 0',
                'commands',
                'silent',
                'printf "TASK25R_PACE=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", sNdsBattlePlayableLastPresentVBlank, gNdsBattlePlayablePacingPresentedFrames, gNdsBattlePlayablePacingLogicFrames, gNdsBattlePlayablePacingCadenceViolationCount, gNdsBattlePlayablePacingPhasePresentCount[0], gNdsBattlePlayablePacingPhasePresentCount[1], gNdsBattlePlayablePacingPhasePresentCount[2], gNdsBattlePlayablePacingPhasePresentCount[3], gNdsBattlePlayablePacingPhasePresentCount[4], gNdsBattlePlayablePacingPhaseSlipCount[0], gNdsBattlePlayablePacingPhaseSlipCount[1], gNdsBattlePlayablePacingPhaseSlipCount[2], gNdsBattlePlayablePacingPhaseSlipCount[3], gNdsBattlePlayablePacingPhaseSlipCount[4]'
            ) + $task25rScreenshotCommands + @(
                'continue',
                'end'
            )
        } else { @() }
        $matchWaitCommands = @(
            'tbreak scVSBattleFuncUpdate if gSCManagerBattleState->time_limit == 1 && gSCManagerBattleState->time_remain == 3600 && gSCManagerBattleState->time_passed == 0',
            'continue',
            'printf "MATCH_START=%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gSCManagerSceneData.scene_curr, gSCManagerBattleState->game_status, gSCManagerBattleState->time_limit, gSCManagerBattleState->time_remain, gSCManagerBattleState->time_passed, sIFCommonTimerIsStarted, ((FTStruct *)gGCCommonLinks[3]->user_data.p)->is_control_disable, ((FTStruct *)gGCCommonLinks[3]->link_next->user_data.p)->is_control_disable, gNdsMemoryLedgerArenaHeadroom'
        ) + $task9StateStartCommands + $task25rPacingCommands + @(
            'set variable gNdsBattlePlayablePacingRestartRequested = 1',
            'tbreak ndsRendererHardwareDiscardBattleStaticTextures',
            'continue',
            'printf "VSB_MEMARENA=%#x,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsMemoryLedgerResult, gNdsMemoryLedgerScene, gNdsMemoryLedgerGeneration, gNdsMemoryLedgerArenaCapacity, gNdsMemoryLedgerArenaUsed, gNdsMemoryLedgerArenaHighWater, gNdsMemoryLedgerArenaHeadroom, gNdsMemoryLedgerDLBytes, gNdsMemoryLedgerGraphicsBytes, gNdsMemoryLedgerRdpBytes, gNdsMemoryLedgerFigatreeHeapSize',
            'printf "VSB_MEMRELOC=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsMemoryLedgerRelocFiles, gNdsMemoryLedgerRelocBytes, gNdsMemoryLedgerRelocStageBytes, gNdsMemoryLedgerRelocFighterBytes, gNdsMemoryLedgerRelocInterfaceBytes, gNdsMemoryLedgerRelocMenuBytes, gNdsMemoryLedgerRelocOpeningBytes, gNdsMemoryLedgerRelocOtherBytes, gNdsMemoryLedgerRelocStaleFiles, gNdsMemoryLedgerRelocStaleBytes',
            'printf "VSB_MEMEVICT=%u,%u\n", gNdsMemoryLedgerEvictedFiles, gNdsMemoryLedgerEvictedBytes',
            'tbreak ndsMNVSResultsRecordFrame if gNdsVSResultsResult == 0x56535231',
            'continue'
        ) + $task9StateFinishCommands
        $gdbCommands = @(
            $gdbCommands[0..3]
            $matchWaitCommands
            $gdbCommands[4..($gdbCommands.Count - 1)]
        )
    }
    if ($BattlePlayable -and $RealtimePresentation -and $HardwareTriangles -and -not $MatchLifecycleProof) {
        if ($RendererBenchmarkSamples -gt 0) {
            $coarseBenchmarkCommands = @()
            if ($RendererProfileLevel -ge 1) {
                $coarseBenchmarkCommands += 'printf "COARSE_BENCH=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsRendererProfileFrameCount, gNdsRendererProfileLoopWallTicks, gNdsRendererProfileInputTicks, gNdsRendererProfileUpdateTicks, gNdsRendererProfileSourceUpdateTicks, gNdsRendererProfileAudioUpdateTicks, gNdsRendererProfilePresentActiveTicks, gNdsRendererProfileVBlankWaitTicks, gNdsRendererProfileBeginFrameTicks, gNdsRendererProfileDrawTicks, gNdsRendererProfileWallpaperTicks, gNdsRendererProfileOwners[0].exclusive_ticks, gNdsRendererProfileOwners[1].exclusive_ticks, gNdsRendererProfileOwners[2].exclusive_ticks, gNdsRendererProfileForegroundTicks, gNdsRendererProfileHudTicks, gNdsRendererProfileFlushTicks, gNdsRendererProfilePostVBlankTicks, gNdsRendererProfileThreadTicks, gNdsRendererProfileDrawResidualTicks, gNdsRendererProfilePresentResidualTicks, gNdsRendererProfileLoopResidualTicks, gNdsRendererProfileConservationErrorTicks, gNdsRendererProfileLogicTick, gNdsRendererProfileSourceUpdate1Ticks, gNdsRendererProfileSourceUpdate2Ticks, gNdsRendererProfilePresentIntervalVBlanks'
                $coarseBenchmarkCommands += 'printf "STAGE0_BENCH=%u,%u\n", gNdsRendererProfileFrameCount, gNdsRendererProfileStageLayer0Ticks'
                $coarseBenchmarkCommands += 'printf "GX_BOUNDARY=%u,%u,%u,%u,%u,%u,%u\n", gNdsRendererProfileFrameCount, gNdsRendererProfileGXStatusBeforeFlush, gNdsRendererProfileGXControlBeforeFlush, gNdsRendererProfileGXStatusAfterFlush, gNdsRendererProfileGXStatusPostVBlank, gNdsRendererProfileGXControlPostVBlank, gNdsRendererProfileFlushTicks'
                $coarseBenchmarkCommands += 'printf "TEXTURE_PHASE_BENCH=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsRendererProfileFrameCount, gNdsRendererProfileTextureConvertTicks, gNdsRendererProfileTextureUploadTicks, gNdsRendererProfileTextureUploads, gNdsRendererProfileTextureUploadBytes, gNdsRendererProfileTexturePairOracleChecks, gNdsRendererProfileTexturePairOracleMismatches, gNdsRendererProfileTextureVBlankQueuedUploads, gNdsRendererProfileTextureVBlankQueuedBytes, gNdsRendererProfileTextureVBlankCommittedUploads, gNdsRendererProfileTextureVBlankCommitTicks, gNdsRendererProfileTextureVBlankOutsideCount, gNdsRendererProfileTextureVBlankFallbackCount, gNdsRendererProfileTextureVBlankStartLine, gNdsRendererProfileTextureVBlankEndLine'
                $coarseBenchmarkCommands += 'printf "FAST_BENCH=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsRendererProfileFrameCount, gNdsRendererFastRunMode, gNdsRendererFastRunCount, gNdsRendererFastTriangleCount, gNdsRendererFastOwnerTriangleCount[0], gNdsRendererFastOwnerTriangleCount[1], gNdsRendererFastOwnerTriangleCount[2], gNdsRendererFastFallbackCount[0], gNdsRendererFastFallbackCount[1], gNdsRendererFastFallbackCount[2]'
                if ($RenderEconomyMode -eq 1) {
                    $coarseBenchmarkCommands += 'printf "ECONOMY_BENCH=%u,%u,%u,%u,%u\n", gNdsRendererProfileFrameCount, gNdsRendererEconomyActiveOwnerMask, gNdsRendererEconomyAppliedOwnerMask, gNdsRendererEconomySkippedRunCount, gNdsRendererEconomySkippedTriangleCount'
                }
                if (($RendererProfileLevel -eq 1) -and
                    ($RendererFastRunMode -eq 9)) {
                    $topologyFaultInjectionExpression = if ($RendererM3Phase0Profile) {
                        'gNdsRendererM3TopologyFaultInjectionCount'
                    } else { '0' }
                    $topologyFaultRevalidationExpression = if ($RendererM3Phase0Profile) {
                        'gNdsRendererM3TopologyFaultRevalidationCount'
                    } else { '0' }
                    $coarseBenchmarkCommands += ('printf "M3_STAGE=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsRendererProfileFrameCount, gNdsRendererM3PreflightAttemptCount, gNdsRendererM3PreflightSuccessCount, gNdsRendererM3PreflightFallbackCount, gNdsRendererM3SegmentCount, gNdsRendererM3SegmentMask, gNdsRendererM3PostArmFailureCount, gNdsRendererM3DObjCount, gNdsRendererM3BindingCount, gNdsRendererM3RunCount, gNdsRendererM3TriangleCount, gNdsRendererM3ResidentEpochCount, gNdsRendererM3MaterialShadowCount, gNdsRendererM3MaterialCommitCount, gNdsRendererM3CrossRunCount, gNdsRendererM3CrossTriangleCount, gNdsRendererM3CrossForeignCornerCount, gNdsRendererM3TopologyFullValidationCount, gNdsRendererM3TopologyCacheHitCount, gNdsRendererM3TopologyStampMismatchCount, {0}, {1}' -f $topologyFaultInjectionExpression, $topologyFaultRevalidationExpression)
                    if ($Task36HwComposeMode -gt 0) {
                        $task36MismatchExpression =
                            if ($RendererM3Phase0Profile) {
                                'gNdsRendererTask36RigidConstancyMismatchCount'
                            } else { '0' }
                        $task36DynamicLoExpression =
                            if ($RendererM3Phase0Profile) {
                                'gNdsRendererTask36ObservedDynamicMaskLo'
                            } else { '0' }
                        $task36DynamicHiExpression =
                            if ($RendererM3Phase0Profile) {
                                'gNdsRendererTask36ObservedDynamicMaskHi'
                            } else { '0' }
                        $coarseBenchmarkCommands += ('printf "TASK36_HW=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsRendererProfileFrameCount, gNdsRendererTask36HardwareComposedDObjCount, gNdsRendererTask36CameraLoadCount, gNdsRendererTask36WorldMultCount, {0}, {1}, {2}, gNdsRendererTask36AdapterRejectReason, gNdsRendererTask36RendererRejectReason, gNdsRendererTask36PrepareRunRejectReason, gNdsRendererM3PreflightFallbackCount, gNdsRendererM3PostArmFailureCount' -f $task36MismatchExpression, $task36DynamicLoExpression, $task36DynamicHiExpression)
                        if ($Task36HwComposeMode -eq 2) {
                            $coarseBenchmarkCommands += 'printf "TASK36_REPLAY=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsRendererProfileFrameCount, gNdsRendererTask36ReplayState, gNdsRendererTask36BakeAttemptCount, gNdsRendererTask36BakeSuccessCount, gNdsRendererTask36BakeFailureCount, gNdsRendererTask36ReplayFrameCount, gNdsRendererTask36ReplaySegmentCount, gNdsRendererTask36ReplayRunCount, gNdsRendererTask36ReplayWordCount, gNdsRendererTask36ReplayFallbackCount, gNdsRendererTask36ReplayArenaRejectCount, gNdsRendererTask36ReplayMaterialRejectCount, gNdsRendererTask36ReplayCaptureWordCount, gNdsTaskmanArenaChosenSize, gNdsTaskmanArenaAllocFailCount, sNdsRendererTask36ReplayOwner.word_count, sNdsRendererTask36ReplayOwner.captured_segment_mask, sNdsRendererTask36ReplayOwner.capture_fault, sNdsRendererTask36ReplayOwner.capture_active'
                        }
                    }
                    if ($benchmarkMakeIdentity.NativeStageGeneratedSegment0Enable -eq 1) {
                        $coarseBenchmarkCommands += 'printf "M3_GEN0=%u,1,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsRendererProfileFrameCount, gNdsRendererM3GeneratedSegment0AttemptCount, gNdsRendererM3GeneratedSegment0SuccessCount, gNdsRendererM3GeneratedSegment0PreGxFallbackCount, gNdsRendererM3GeneratedSegment0RunCount, gNdsRendererM3GeneratedSegment0TriangleCount, gNdsRendererM3GeneratedSegment0EpochCount, gNdsRendererM3GeneratedSegment0MaterialCount, gNdsRendererM3GeneratedSegment0CertificateValidationCount'
                        if ($RendererM3Phase0Profile) {
                            $coarseBenchmarkCommands += 'printf "M3_GEN0_SHADOW=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsRendererProfileFrameCount, gNdsRendererM3GeneratedSegment0AttemptCount, gNdsRendererM3GeneratedSegment0RunCount, gNdsRendererM3GeneratedSegment0ShadowDenseCount, gNdsRendererM3GeneratedSegment0ShadowStateEntryCount, gNdsRendererM3GeneratedSegment0ShadowSyncCount, gNdsRendererM3GeneratedSegment0EpochCount, gNdsRendererM3GeneratedSegment0TriangleCount, gNdsRendererM3GeneratedSegment0ShadowFieldComparisonCount, gNdsRendererM3GeneratedSegment0ShadowMismatchCount, gNdsRendererM3GeneratedSegment0ShadowFaultInjectedCount, gNdsRendererM3GeneratedSegment0ShadowFaultRejectedCount, gNdsRendererM3GeneratedSegment0ShadowLiveFaultInjectedCount, gNdsRendererM3GeneratedSegment0ShadowLiveFaultRejectedCount, gNdsRendererM3GeneratedSegment0ShadowLiveFaultRevalidatedCount'
                        }
                    }
                    if ($RendererM3Phase0Profile) {
                        $coarseBenchmarkCommands += 'printf "M3_PHASE0=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsRendererProfileFrameCount, gNdsRendererM3Phase0PreflightTicks, gNdsRendererM3Phase0PrepareRunTicks, gNdsRendererM3Phase0VertexPrepareTicks, gNdsRendererM3Phase0NearTransformTicks, gNdsRendererM3Phase0RunTransitionTicks, gNdsRendererM3Phase0RawEmitTicks, gNdsRendererM3Phase0RangeEmitTicks, gNdsRendererM3Phase0NoZEmitTicks, gNdsRendererM3Phase0NoZMatrixTicks, gNdsRendererM3Phase0AccountingTicks, gNdsRendererM3Phase0CommitTicks, gNdsRendererM3Phase0TimerReadCount, gNdsRendererM3Phase0TimerSpanCount, gNdsRendererM3Phase0CalibrationTicks, gNdsRendererM3Phase0CalibrationIntervals, gNdsRendererM3Phase0PreparedDenseCount, gNdsRendererM3Phase0NearTransformCount, gNdsRendererM3Phase0NoZMatrixCount'
                        $coarseBenchmarkCommands += 'printf "M3_RESIDUAL=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsRendererProfileFrameCount, gNdsRendererM3ResidualPrepareTicks, gNdsRendererM3ResidualVertexTicks, gNdsRendererM3ResidualNearTicks, gNdsRendererM3ResidualKeyTicks, gNdsRendererM3ResidualKeyHitCount, gNdsRendererM3ResidualKeyMissCount, gNdsRendererM3ResidualKeyByteCount, gNdsRendererM3ResidualRunCount, gNdsRendererM3ResidualDenseCount, gNdsRendererM3ResidualNearCount'
                        # Debugger-only FNV-1a prepared-output stability
                        # census: no target state, cache, or reuse mechanism
                        # is added to the measured ROM.
                        $coarseBenchmarkCommands += @(
                            'printf "M3_PREPARED=%u,%u,%u", gNdsRendererProfileFrameCount, sNdsRendererAdapterNativeStageWorkspace.frame.topology_generation, sNdsRendererAdapterNativeStageWorkspace.frame.topology_stamp',
                            'set $m3_live_prefix_hash = (unsigned int)2166136261',
                            'set $m3_live_ptr = (unsigned char *)&sNdsRendererAdapterNativeStageWorkspace.frame.topology_generation',
                            'set $m3_live_byte = 0',
                            'while $m3_live_byte < 8',
                            'set $m3_live_prefix_hash = (unsigned int)(($m3_live_prefix_hash ^ (unsigned int)$m3_live_ptr[$m3_live_byte]) * (unsigned int)16777619)',
                            'set $m3_live_byte = $m3_live_byte + 1',
                            'end',
                            'set $m3_live_ptr = (unsigned char *)sNdsRendererAdapterNativeStageWorkspace.frame.config',
                            'set $m3_live_byte = 0',
                            'while $m3_live_byte < sizeof(*sNdsRendererAdapterNativeStageWorkspace.frame.config)',
                            'set $m3_live_prefix_hash = (unsigned int)(($m3_live_prefix_hash ^ (unsigned int)$m3_live_ptr[$m3_live_byte]) * (unsigned int)16777619)',
                            'set $m3_live_byte = $m3_live_byte + 1',
                            'end',
                            'set $m3_live_segment = 0',
                            'while $m3_live_segment < 8',
                            'set $m3_live_hash = $m3_live_prefix_hash',
                            'set $m3_live_run = (unsigned int)sNdsNativeStageSegments[$m3_live_segment].first_run',
                            'set $m3_live_run_end = $m3_live_run + (unsigned int)sNdsNativeStageSegments[$m3_live_segment].run_count',
                            'while $m3_live_run < $m3_live_run_end',
                            'set $m3_live_ptr = (unsigned char *)&sNdsNativeStageOwnerExecution.runs[$m3_live_run]',
                            'set $m3_live_byte = 0',
                            'while $m3_live_byte < sizeof(sNdsNativeStageOwnerExecution.runs[0])',
                            'set $m3_live_hash = (unsigned int)(($m3_live_hash ^ (unsigned int)$m3_live_ptr[$m3_live_byte]) * (unsigned int)16777619)',
                            'set $m3_live_byte = $m3_live_byte + 1',
                            'end',
                            'set $m3_live_dense_offset = (unsigned int)sNdsNativeStageValidationCache.prepared_dense_offsets[$m3_live_run]',
                            'set $m3_live_dense_end = (unsigned int)sNdsNativeStageValidationCache.prepared_dense_offsets[$m3_live_run + 1]',
                            'while $m3_live_dense_offset < $m3_live_dense_end',
                            'set $m3_live_ptr = (unsigned char *)&sNdsNativeStageValidationCache.prepared_dense_indices[$m3_live_dense_offset]',
                            'set $m3_live_byte = 0',
                            'while $m3_live_byte < 2',
                            'set $m3_live_hash = (unsigned int)(($m3_live_hash ^ (unsigned int)$m3_live_ptr[$m3_live_byte]) * (unsigned int)16777619)',
                            'set $m3_live_byte = $m3_live_byte + 1',
                            'end',
                            'set $m3_live_dense = (unsigned int)sNdsNativeStageValidationCache.prepared_dense_indices[$m3_live_dense_offset]',
                            'set $m3_live_ptr = (unsigned char *)&sNdsNativeStagePreparedDense[$m3_live_dense]',
                            'set $m3_live_byte = 0',
                            'while $m3_live_byte < 6',
                            'set $m3_live_hash = (unsigned int)(($m3_live_hash ^ (unsigned int)$m3_live_ptr[$m3_live_byte]) * (unsigned int)16777619)',
                            'set $m3_live_byte = $m3_live_byte + 1',
                            'end',
                            'set $m3_live_dense_offset = $m3_live_dense_offset + 1',
                            'end',
                            'set $m3_live_run = $m3_live_run + 1',
                            'end',
                            'printf ",%u", $m3_live_hash',
                            'set $m3_live_segment = $m3_live_segment + 1',
                            'end',
                            'set $m3_live_material = 0',
                            'while $m3_live_material < 4',
                            'set $m3_live_hash = $m3_live_prefix_hash',
                            'set $m3_live_ptr = (unsigned char *)&sNdsRendererAdapterNativeStageWorkspace.materials[$m3_live_material]',
                            'set $m3_live_byte = 0',
                            'while $m3_live_byte < sizeof(sNdsRendererAdapterNativeStageWorkspace.materials[0])',
                            'set $m3_live_hash = (unsigned int)(($m3_live_hash ^ (unsigned int)$m3_live_ptr[$m3_live_byte]) * (unsigned int)16777619)',
                            'set $m3_live_byte = $m3_live_byte + 1',
                            'end',
                            'set $m3_live_ptr = (unsigned char *)&sNdsRendererAdapterNativeStageWorkspace.material_curr[$m3_live_material]',
                            'set $m3_live_byte = 0',
                            'while $m3_live_byte < 4',
                            'set $m3_live_hash = (unsigned int)(($m3_live_hash ^ (unsigned int)$m3_live_ptr[$m3_live_byte]) * (unsigned int)16777619)',
                            'set $m3_live_byte = $m3_live_byte + 1',
                            'end',
                            'set $m3_live_ptr = (unsigned char *)&sNdsRendererAdapterNativeStageWorkspace.material_next[$m3_live_material]',
                            'set $m3_live_byte = 0',
                            'while $m3_live_byte < 4',
                            'set $m3_live_hash = (unsigned int)(($m3_live_hash ^ (unsigned int)$m3_live_ptr[$m3_live_byte]) * (unsigned int)16777619)',
                            'set $m3_live_byte = $m3_live_byte + 1',
                            'end',
                            'printf ",%u", $m3_live_hash',
                            'set $m3_live_material = $m3_live_material + 1',
                            'end',
                            'printf "\n"'
                        )
                        $coarseBenchmarkCommands += 'printf "M3_WHISPY=%u,%u,%u,%u,%u\n", gNdsRendererProfileFrameCount, (unsigned int)gGRCommonStruct.pupupu.whispy_status, (unsigned int)gGRCommonStruct.pupupu.whispy_wind_wait, (unsigned int)gGRCommonStruct.pupupu.whispy_wind_duration, (unsigned int)gGRCommonStruct.pupupu.whispy_blink_wait'
                        $coarseBenchmarkCommands += 'printf "G2_STATE=%u,%u,%u,%u,%u,%u,%u\n", gNdsRendererProfileFrameCount, gNdsRendererM3G2TextureParamWriteCount, gNdsRendererM3G2TextureParamSkipCount, gNdsRendererM3G2MatrixModeWriteCount, gNdsRendererM3G2MatrixModeSkipCount, gNdsRendererM3G2PolyFmtWriteCount, gNdsRendererM3G2PolyFmtSkipCount'
                        $coarseBenchmarkCommands += 'printf "PHASE05=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsRendererProfileFrameCount, gNdsRendererPhase05WallpaperSetupTicks, gNdsRendererPhase05WallpaperXMapTicks, gNdsRendererPhase05WallpaperYMapTicks, gNdsRendererPhase05WallpaperWriteTicks, gNdsRendererPhase05WallpaperCommitTicks, gNdsRendererPhase05PresentHardwareTicks, gNdsRendererPhase05GCDrawAllTicks, gNdsRendererPhase05StageTransitionTicks, gNdsRendererPhase05FighterWrapperTicks, gNdsRendererPhase05FrameResetTicks, gNdsRendererPhase05PresentTailTicks, gNdsRendererPhase05ProfileBookkeepingTicks, gNdsRendererPhase05ProfilePublishTicks, gNdsRendererPhase05FlushPrepTicks, gNdsRendererPhase05TimerReadCount, gNdsRendererPhase05TimerSpanCount, gNdsRendererPhase05CalibrationTicks, gNdsRendererPhase05CalibrationIntervals, gNdsRendererPhase05WallpaperRowCount, gNdsRendererPhase05WallpaperPixelWriteCount'
                        $coarseBenchmarkCommands += 'printf "WALL_RUNS=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsRendererProfileFrameCount, gNdsRendererPhase05WallpaperFullRowCount, gNdsRendererPhase05WallpaperIncrementalRowCount, gNdsRendererPhase05WallpaperChangedXCount, gNdsRendererPhase05WallpaperChangedRunCount, gNdsRendererPhase05WallpaperLongestChangedRun, gNdsRendererPhase05WallpaperRunGE2Count, gNdsRendererPhase05WallpaperRunGE2Pixels, gNdsRendererPhase05WallpaperRunGE4Count, gNdsRendererPhase05WallpaperRunGE4Pixels, gNdsRendererPhase05WallpaperRunGE8Count, gNdsRendererPhase05WallpaperRunGE8Pixels, gNdsRendererPhase05WallpaperScalarStoreCount, gNdsRendererPhase05WallpaperPackedStoreCount, gNdsRendererPhase05WallpaperDmaPixelCount, gNdsRendererPhase05WallpaperCopyPixelCount, gNdsRendererPhase05WallpaperPixelWriteCount'
                    }
                }
                if ($m4CandidateEvidence) {
                    $coarseBenchmarkCommands += 'printf "M4_WATER_STILL=%u,%u,%u,%u\n", gNdsRendererProfileFrameCount, gNdsPupupuWaterStillFreezeCount, gNdsPupupuWaterStillFreezeFailCount, gNdsPupupuWaterStillFreezeResult'
                }
                $coarseBenchmarkCommands += 'printf "M4_STATIC=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsRendererProfileFrameCount, gNdsRendererBattleStaticTextureEnabled, gNdsRendererBattleStaticTexturePrepareCount, gNdsRendererBattleStaticTexturePrepareFailCount, gNdsRendererBattleStaticTexturePreparedCount, gNdsRendererBattleStaticTexturePreparedBytes, gNdsRendererBattleStaticTextureArmCount, gNdsRendererBattleStaticTextureSeenMask, gNdsRendererBattleStaticTextureOwnerMask, gNdsRendererBattleStaticTextureViolationCount, gNdsRendererBattleStaticTexturePinnedHitCount'
                $coarseBenchmarkCommands += 'printf "M4_FENCE=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsRendererProfileFrameCount, gNdsRendererBattleTextureFenceFirstClassPlus1, gNdsRendererBattleTextureFenceFirstFrame, gNdsRendererBattleTextureFenceCounts[0], gNdsRendererBattleTextureFenceCounts[1], gNdsRendererBattleTextureFenceCounts[2], gNdsRendererBattleTextureFenceCounts[3], gNdsRendererBattleTextureFenceCounts[4], gNdsRendererBattleTextureFenceCounts[5], gNdsRendererBattleTextureFenceCounts[6], gNdsRendererBattleTextureFenceCounts[7], gNdsRendererBattleTextureFenceCounts[8], gNdsRendererBattleTextureFenceCounts[9]'
                if ($Task29GXCensus) {
                    $coarseBenchmarkCommands += @(
                        'printf "TASK29_GX_META=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsTask29GXFrame, gNdsTask29GXTotalCommandCount, gNdsTask29GXTotalWordCount, gNdsTask29GXTotalRepeatCount, gNdsTask29GXStreamHashA, gNdsTask29GXStreamHashB, gNdsTask29GXBoundaryHashA, gNdsTask29GXBoundaryHashB, gNdsTask29GXBoundaryCount, gNdsTask29GXFaultCount, gNdsTask29GXNeverSuppressMask',
                        'printf "TASK29_GX_CLASS=%u", gNdsTask29GXFrame',
                        'set $task29_gx_class = 0',
                        'while $task29_gx_class < 22',
                        'printf ",%u,%u,%u", gNdsTask29GXCommandCount[$task29_gx_class], gNdsTask29GXWordCount[$task29_gx_class], gNdsTask29GXRepeatCount[$task29_gx_class]',
                        'set $task29_gx_class = $task29_gx_class + 1',
                        'end',
                        'printf "\n"',
                        'set $task29_gx_owner = 0',
                        'while $task29_gx_owner < 4',
                        'printf "TASK29_GX_OWNER=%u,%u,%u,%u", gNdsTask29GXFrame, $task29_gx_owner, gNdsTask29GXOwnerHashA[$task29_gx_owner], gNdsTask29GXOwnerHashB[$task29_gx_owner]',
                        'set $task29_gx_class = 0',
                        'while $task29_gx_class < 22',
                        'printf ",%u,%u,%u", gNdsTask29GXOwnerCommandCount[$task29_gx_owner][$task29_gx_class], gNdsTask29GXOwnerWordCount[$task29_gx_owner][$task29_gx_class], gNdsTask29GXOwnerRepeatCount[$task29_gx_owner][$task29_gx_class]',
                        'set $task29_gx_class = $task29_gx_class + 1',
                        'end',
                        'printf "\n"',
                        'set $task29_gx_owner = $task29_gx_owner + 1',
                        'end'
                    )
                }
                if ($Task34StageStreamCensus) {
                        $coarseBenchmarkCommands +=
                            'printf "TASK34_STAGE_META=%u,%u,%u,%u,%u\n", gNdsTask34StageStreamFrame, gNdsTask34StageStreamEntryCount, gNdsTask34StageStreamWordCount, gNdsTask34StageStreamOverflowCount, gNdsTask34StageStreamFaultCount'
                        foreach ($dump in $task34StageDumpFiles) {
                            $entriesPath = [IO.Path]::GetRelativePath(
                                $root, $dump.Entries).Replace('\', '/')
                            $wordsPath = [IO.Path]::GetRelativePath(
                                $root, $dump.Words).Replace('\', '/')
                            $coarseBenchmarkCommands += @(
                                ('if $renderer_benchmark_samples == {0}' -f
                                    $dump.Sample),
                                ('dump binary memory {0} &gNdsTask34StageStreamEntries[0] &gNdsTask34StageStreamEntries[gNdsTask34StageStreamEntryCount]' -f
                                    $entriesPath),
                                ('dump binary memory {0} &gNdsTask34StageStreamWords[0] &gNdsTask34StageStreamWords[gNdsTask34StageStreamWordCount]' -f
                                    $wordsPath),
                                'end'
                            )
                        }
                        $coarseBenchmarkCommands += @(
                            ('if $renderer_benchmark_samples == {0}' -f
                                ($RendererBenchmarkSamples - 1)),
                            'set variable gNdsTask34StageStreamCaptureEnabled = 0',
                            'end'
                        )
                }
                if ($Task9FloatCensusMode -eq 1) {
                    $coarseBenchmarkCommands += @(
                        'printf "TASK9_FLOAT_PAIR=%u,%u", gNdsRendererProfileFrameCount, gNdsTask9FloatCensusUpdateCount',
                        'set $task9_float_index = 0',
                        'while $task9_float_index < 35',
                        'printf ",%u", gNdsTask9FloatCensusPair[$task9_float_index]',
                        'set $task9_float_index = $task9_float_index + 1',
                        'end',
                        'printf "\n"',
                        'if $renderer_benchmark_samples == 0',
                        'printf "TASK9_FLOAT_TIMER=%u,%u\n", gNdsTask9FloatTimerReadPairTicks, gNdsTask9FloatTimerReadPairCount',
                        'set $task9_float_index = 0',
                        'while $task9_float_index < 35',
                        'printf "TASK9_FLOAT_COST=%u,%u,%u,%u,%u,%u,%u\n", $task9_float_index, gNdsTask9FloatCostTicks[$task9_float_index], gNdsTask9FloatCostCalls[$task9_float_index], gNdsTask9FloatCostMin[$task9_float_index], gNdsTask9FloatCostMax[$task9_float_index], gNdsTask9FloatCensusMin[$task9_float_index], gNdsTask9FloatCensusMax[$task9_float_index]',
                        'set $task9_float_index = $task9_float_index + 1',
                        'end',
                        'end'
                    )
                }
                if ($RendererM2DetailedLedger) {
                    foreach ($ownerIndex in 1..2) {
                        $coarseBenchmarkCommands += Get-RendererM2BenchmarkCommand -OwnerIndex $ownerIndex
                    }
                    $coarseBenchmarkCommands += 'printf "M2_SHADE=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsRendererProfileFrameCount, gNdsRendererM2ShadeEpochCount, gNdsRendererM2ShadeKeyHitCount, gNdsRendererM2ShadeResidentHitCount, gNdsRendererM2ShadeHashCollisionCount, gNdsRendererM2ShadeDenseVisitCount, gNdsRendererM2ShadeComputeCount, gNdsRendererM2ShadeLutComputeCount, gNdsRendererM2ShadePreparedComputeCount, gNdsRendererM2ShadeAliasCopyCount, gNdsRendererM2ShadeMaterialPackCount, gNdsRendererM2ShadeOwnerEpochCount[0], gNdsRendererM2ShadeOwnerKeyHitCount[0], gNdsRendererM2ShadeOwnerResidentHitCount[0], gNdsRendererM2ShadeOwnerEpochCount[1], gNdsRendererM2ShadeOwnerKeyHitCount[1], gNdsRendererM2ShadeOwnerResidentHitCount[1]'
                }
                if ($benchmarkMakeIdentity.RendererBenchmarkMode -eq 2) {
                    $coarseBenchmarkCommands += 'printf "SINK_BENCH=%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsRendererProfileFrameCount, gNdsRendererBenchmarkSinkCursor, gNdsRendererBenchmarkSinkWordCount, gNdsRendererBenchmarkSinkCalibrationWords, gNdsRendererBenchmarkSinkCalibrationTicks, gNdsRendererBenchmarkSinkOwnerWords[0], gNdsRendererBenchmarkSinkOwnerWords[1], gNdsRendererBenchmarkSinkOwnerWords[2]'
                    if ($RendererFastRunMode -eq 9) {
                        $coarseBenchmarkCommands += 'printf "M3_GEN0_GX=%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsRendererProfileFrameCount, gNdsRendererBenchmarkSinkWordCount, gNdsRendererBenchmarkSinkHashA, gNdsRendererBenchmarkSinkHashB, gNdsRendererBenchmarkSegment0SinkWords, gNdsRendererBenchmarkSegment0SinkHashA, gNdsRendererBenchmarkSegment0SinkHashB, gNdsRendererBenchmarkSegment0SinkArmFaults'
                        $coarseBenchmarkCommands += @(
                            ('if $renderer_benchmark_samples == {0}' -f
                                ($RendererBenchmarkSamples - 1)),
                            'printf "M3_GEN0_TRACE_SUMMARY=%u,%u\n", gNdsRendererProfileFrameCount, gNdsRendererBenchmarkSegment0SinkWords',
                            'set $m3_gen0_word = 0',
                            'while $m3_gen0_word < gNdsRendererBenchmarkSegment0SinkWords && $m3_gen0_word < 3072',
                            'printf "M3_GEN0_TRACE_WORD=%u,%u\n", $m3_gen0_word, gNdsRendererBenchmarkSegment0Trace[$m3_gen0_word]',
                            'set $m3_gen0_word = $m3_gen0_word + 1',
                            'end',
                            'set $m3_gen0_run = 0',
                            'while $m3_gen0_run < 26',
                            'printf "M3_GEN0_TRACE_RUN=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", $m3_gen0_run, gNdsRendererBenchmarkSegment0RunWords[$m3_gen0_run], gNdsRendererBenchmarkSegment0RunHashA[$m3_gen0_run], gNdsRendererBenchmarkSegment0RunHashB[$m3_gen0_run], gNdsRendererBenchmarkSegment0RunRawTextureName[$m3_gen0_run], gNdsRendererBenchmarkSegment0RunTextureEpochPlus1[$m3_gen0_run], gNdsRendererBenchmarkSegment0RunTextureImage[$m3_gen0_run], gNdsRendererBenchmarkSegment0RunTextureTlut[$m3_gen0_run], gNdsRendererBenchmarkSegment0RunTextureKeyHashA[$m3_gen0_run], gNdsRendererBenchmarkSegment0RunTextureKeyHashB[$m3_gen0_run], gNdsRendererBenchmarkSegment0RunTextureDescriptor[$m3_gen0_run], gNdsRendererBenchmarkSegment0RunTextureParams[$m3_gen0_run]',
                            'set $m3_gen0_run = $m3_gen0_run + 1',
                            'end',
                            'end'
                        )
                    }
                }
                if ($benchmarkMakeIdentity.RendererBenchmarkMode -eq 4) {
                    $coarseBenchmarkCommands += 'printf "WARM_BENCH=%u,%u,%u\n", gNdsRendererProfileFrameCount, gNdsRendererBenchmarkSuppressedTextureUploads, gNdsRendererBenchmarkSuppressedTextureUploadBytes'
                }
                if ($RendererProfileLevel -ge 2) {
                    foreach ($ownerIndex in 0..2) {
                        $coarseBenchmarkCommands += Get-RendererOwnerBenchmarkCommand -OwnerIndex $ownerIndex
                    }
                    $coarseBenchmarkCommands += Get-RendererSemanticBenchmarkCommand
                }
            }
            if ($Task20StackProfileMode -eq 1) {
                $coarseBenchmarkCommands += @(
                    'printf "TASK20_STACK=%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsTask20GameplayStackBase, gNdsTask20GameplayStackSize, gNdsTask20GameplayStackHighWater, gNdsTask20MainStackBottom, gNdsTask20MainStackPoisonStart, gNdsTask20MainStackTop, gNdsTask20MainStackHighWater, gNdsTask20SampleCount'
                )
                $coarseBenchmarkCommands += Get-Task20CoroutineCensusCommands
            }
            $benchmarkTriangleSymbol = if ($RendererBenchmarkOnly -and
                ($RendererFastRunMode -ne 9)) {
                'gNdsRendererBenchmarkTriangleCount'
            } else {
                'gNdsRendererProfileHardwareTriangles'
            }
            $rendererBenchmarkCommand =
                'printf "RENDER_BENCH=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsRendererProfileLevel, gNdsRendererProfileFrameCount, gNdsRendererProfilePresentTicks, gNdsRendererProfileDrawTicks, gNdsRendererProfileStageAdapterTicks, gNdsRendererProfileMaterialTicks, gNdsRendererProfileMatrixTicks, gNdsRendererProfileDLTicks, gNdsRendererProfileTextureTicks, gNdsRendererProfileTriangleSubmitTicks, gNdsRendererProfileVertexSubmitTicks, {0}, gNdsRendererProfileOracleSamples, gNdsRendererProfileTextureUploads, gNdsRendererProfileTextureUploadBytes' -f $benchmarkTriangleSymbol
            $benchmarkStartGateCommands = @()
            $benchmarkEventBreakpointCommands = @()
            $benchmarkRuntimeControlCommands = @()
            $benchmarkInitialControlCommands = @()
            $benchmarkCensusFinishCommands = @()
            $benchmarkWarmInitial = 0
            if (-not $usesPublishedIntrinsicRendererDefaults) {
                $benchmarkInitialControlCommands +=
                    ('set variable gNdsRendererFastRunMode = {0}' -f
                     $RendererFastRunMode)
            }
            if ($RenderEconomyMode -eq 1) {
                $benchmarkInitialControlCommands +=
                    ('set variable gNdsRendererEconomyConfiguredOwnerMask = {0}' -f
                     $RenderEconomyOwnerMask)
            }
            $benchmarkInitialControlCommands +=
                ('set variable gNdsSObjWallpaperIncrementalMode = {0}' -f
                 $WallpaperIncrementalMode)
            $benchmarkInitialControlCommands +=
                ('set variable gNdsIFCommonHUDLowerTextMode = {0}' -f
                 $LowerTextHudMode)
            if ($RendererBenchmarkStartFrame -gt 0) {
                $benchmarkWarmInitial = 1
                # Reapply these controls at the frame marker after C runtime
                # initialization; an immediate GDB attach can precede .data.
                if (-not $usesPublishedIntrinsicRendererDefaults) {
                    $benchmarkRuntimeControlCommands +=
                        ('set variable gNdsRendererFastRunMode = {0}' -f
                         $RendererFastRunMode)
                }
                if ($RenderEconomyMode -eq 1) {
                    $benchmarkRuntimeControlCommands +=
                        ('set variable gNdsRendererEconomyConfiguredOwnerMask = {0}' -f
                         $RenderEconomyOwnerMask)
                }
                $benchmarkRuntimeControlCommands +=
                    ('set variable gNdsSObjWallpaperIncrementalMode = {0}' -f
                     $WallpaperIncrementalMode)
                $benchmarkRuntimeControlCommands +=
                    ('set variable gNdsIFCommonHUDLowerTextMode = {0}' -f
                     $LowerTextHudMode)
                if ($RendererScreenSpaceCensusMode -eq 1) {
                    $benchmarkStartGateCommands += @(
                        ('if gNdsRendererProfileFrameCount == {0}' -f
                            ($RendererBenchmarkStartFrame - 1)),
                        'set variable gNdsRendererScreenSpaceCensusResetRequested = 1',
                        'set variable gNdsRendererScreenSpaceCensusArmed = 1',
                        'continue',
                        'end'
                    )
                }
                if ($Task34StageStreamCensus) {
                    $benchmarkStartGateCommands += @(
                        ('if gNdsRendererProfileFrameCount == {0}' -f
                            ($RendererBenchmarkStartFrame - 1)),
                        'set variable gNdsTask34StageStreamCaptureEnabled = 1',
                        'continue',
                        'end'
                    )
                }
                $benchmarkStartGateCommands +=
                    ('if gNdsRendererProfileFrameCount < {0}' -f
                     $RendererBenchmarkStartFrame)
                $benchmarkStartGateCommands += 'continue'
                $benchmarkStartGateCommands += 'end'
            }
            elseif ($RendererBenchmarkStartEvent -eq 'KO') {
                # The FGM trace is populated by the imported common-death
                # source path. Gate the first sample on the natural KO event,
                # not on a guessed frame number or scripted combat state.
                $benchmarkWarmInitial = 1
                $benchmarkStartGateCommands +=
                    'if gNdsAudioFgmKoTraceCount == 0'
                $benchmarkStartGateCommands += 'continue'
                $benchmarkStartGateCommands += 'end'
                $benchmarkStartGateCommands +=
                    'set $renderer_benchmark_event = 1'
            }
            elseif ($RendererBenchmarkStartEvent -eq 'Rebirth') {
                # BattleShip enters every normal respawn through this exact
                # source transition before the first rebirth frame is drawn.
                $benchmarkWarmInitial = 1
                $benchmarkEventBreakpointCommands = @(
                    'break ftCommonRebirthDownSetStatus'
                    'commands'
                    'silent'
                    'set $renderer_benchmark_event = 1'
                    'continue'
                    'end'
                )
                $benchmarkStartGateCommands +=
                    'if $renderer_benchmark_event == 0'
                $benchmarkStartGateCommands += 'continue'
                $benchmarkStartGateCommands += 'end'
            }
            elseif ($RendererBenchmarkStartEvent -eq 'Late') {
                # Current one-minute Boundary cannot reach presentation frame
                # 3300. Gate the intended late phase on the source timer.
                $benchmarkWarmInitial = 1
                $benchmarkStartGateCommands +=
                    'if gSCManagerBattleState->time_limit != 1 || gSCManagerBattleState->time_passed < 3300'
                $benchmarkStartGateCommands += 'continue'
                $benchmarkStartGateCommands += 'end'
                $benchmarkStartGateCommands +=
                    'set $renderer_benchmark_event = 1'
            }
            elseif ($RendererBenchmarkStartEvent -eq 'TimeUp') {
                # Eight final battle presents at the 1:00 boundary. Results is
                # proven separately because it does not publish battle owners.
                $benchmarkWarmInitial = 1
                $benchmarkStartGateCommands +=
                    'if gSCManagerBattleState->time_limit != 1 || gSCManagerBattleState->time_remain > 16'
                $benchmarkStartGateCommands += 'continue'
                $benchmarkStartGateCommands += 'end'
                $benchmarkStartGateCommands +=
                    'set $renderer_benchmark_event = 1'
            }
            if ($RendererScreenSpaceCensusMode -eq 1) {
                $benchmarkCensusFinishCommands = @(
                    ('if $renderer_benchmark_samples == {0}' -f
                        ($RendererBenchmarkSamples - 1)),
                    'set variable gNdsRendererScreenSpaceCensusArmed = 0',
                    'printf "SCREEN_CENSUS_SUMMARY=%u,%u\n", gNdsRendererScreenSpaceCensusFrameCount, gNdsRendererScreenSpaceCensusOverflowCount',
                    'set $screen_census_owner = 0',
                    'while $screen_census_owner < 3',
                    'set $screen_census_part = 0',
                    'while $screen_census_part < 42',
                    'if gNdsRendererScreenSpaceCensus[$screen_census_owner][$screen_census_part].triangle_count != 0',
                    'printf "SCREEN_CENSUS_ROW=%u,%u,%u,%u,%u,%u,%u,%u,%u\n", $screen_census_owner, $screen_census_part, gNdsRendererScreenSpaceCensus[$screen_census_owner][$screen_census_part].identity, gNdsRendererScreenSpaceCensus[$screen_census_owner][$screen_census_part].triangle_count, gNdsRendererScreenSpaceCensus[$screen_census_owner][$screen_census_part].area_lt_1px_count, gNdsRendererScreenSpaceCensus[$screen_census_owner][$screen_census_part].area_lt_4px_count, gNdsRendererScreenSpaceCensus[$screen_census_owner][$screen_census_part].invalid_count, (unsigned int)gNdsRendererScreenSpaceCensus[$screen_census_owner][$screen_census_part].area2_q8_sum, (unsigned int)(gNdsRendererScreenSpaceCensus[$screen_census_owner][$screen_census_part].area2_q8_sum >> 32)',
                    'end',
                    'set $screen_census_part = $screen_census_part + 1',
                    'end',
                    'set $screen_census_owner = $screen_census_owner + 1',
                    'end',
                    'set $screen_census_owner = 0',
                    'while $screen_census_owner < 8',
                    'printf "SCREEN_CENSUS_STAGE_OWNER=%u,%u,%u\n", $screen_census_owner, (unsigned int)gNdsRendererScreenSpaceStageOwnerTicks[$screen_census_owner], (unsigned int)(gNdsRendererScreenSpaceStageOwnerTicks[$screen_census_owner] >> 32)',
                    'set $screen_census_owner = $screen_census_owner + 1',
                    'end',
                    'end'
                )
            }
            $benchmarkEventSampleCommands = if (
                $RendererBenchmarkStartEvent -ne 'None') {
                @('printf "RENDER_EVENT=%u,%u,%u\n", gNdsRendererProfileFrameCount, gNdsAudioFgmKoTraceCount, $renderer_benchmark_event')
            } else {
                @()
            }
            $gdbCommands = @(
                $gdbCommands[0..3]
                $benchmarkInitialControlCommands
                'set $renderer_benchmark_samples = 0'
                ('set $renderer_benchmark_warm = {0}' -f $benchmarkWarmInitial)
                'set $renderer_benchmark_event = 0'
                $benchmarkEventBreakpointCommands
                'break ndsBattlePlayableFrameCompleteMarker'
                'commands'
                'silent'
                $benchmarkRuntimeControlCommands
                $benchmarkStartGateCommands
                'if $renderer_benchmark_warm == 0'
                'set $renderer_benchmark_warm = 1'
                'else'
                'if gNdsBattlePlayablePacingResult != 0'
                $rendererBenchmarkCommand
                $coarseBenchmarkCommands
                $benchmarkEventSampleCommands
                $benchmarkCensusFinishCommands
                $benchmarkScreenshotCommands
                'set $renderer_benchmark_samples = $renderer_benchmark_samples + 1'
                'end'
                'end'
                ('if $renderer_benchmark_samples < {0}' -f $RendererBenchmarkSamples)
                'continue'
                'end'
                'end'
                'continue'
                $gdbCommands[4..($gdbCommands.Count - 1)]
            )
        } else {
            # Sample cumulative weapon ownership at one completed frame, then
            # advance exactly once. The delta is an independent source-owner
            # census for the terminal renderer frame; do not infer it from the
            # renderer geometry that it is intended to validate.
            $weaponFrameCommands = @(
                'set $weapon_capture_base = gNdsWeaponRendererCaptureCount'
                'set $weapon_dobj_base = gNdsWeaponRendererDObjDrawCount'
                'set $weapon_submit_base = gNdsWeaponRendererSubmitCount'
                'set $weapon_visible_base = gNdsWeaponRendererVisibleDrawCount'
                'set $weapon_triangle_base = gNdsWeaponRendererTriangleCount'
                'set $weapon_texture_ready_base = gNdsWeaponRendererTextureReadyCount'
                'set $weapon_texture_reject_base = gNdsWeaponRendererTextureRejectCount'
                'set $weapon_noz_base = gNdsWeaponRendererNoZCount'
                'set $weapon_fireball_submit_base = gNdsWeaponRendererFireballSubmitCount'
                'set $weapon_fireball_triangle_base = gNdsWeaponRendererFireballTriangleCount'
                'set $weapon_fireball_visible_base = gNdsWeaponRendererFireballVisibleDrawCount'
                'set $weapon_rejected_base = gNdsWeaponRendererRejectedDrawCount'
                'tbreak ndsBattlePlayableFrameCompleteMarker'
                'continue'
                'printf "WEAPON_FRAME=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsWeaponRendererCaptureCount - $weapon_capture_base, gNdsWeaponRendererDObjDrawCount - $weapon_dobj_base, gNdsWeaponRendererSubmitCount - $weapon_submit_base, gNdsWeaponRendererVisibleDrawCount - $weapon_visible_base, gNdsWeaponRendererTriangleCount - $weapon_triangle_base, gNdsWeaponRendererTextureReadyCount - $weapon_texture_ready_base, gNdsWeaponRendererTextureRejectCount - $weapon_texture_reject_base, gNdsWeaponRendererNoZCount - $weapon_noz_base, gNdsWeaponRendererFireballSubmitCount - $weapon_fireball_submit_base, gNdsWeaponRendererFireballTriangleCount - $weapon_fireball_triangle_base, gNdsWeaponRendererFireballVisibleDrawCount - $weapon_fireball_visible_base, gNdsWeaponRendererRejectedDrawCount - $weapon_rejected_base'
            )
            $armWaitCommands = if ($usesPublishedIntrinsicRendererDefaults) {
                @(
                    'tbreak ndsRendererHardwareArmBattleStaticTextures'
                    'continue'
                )
            } else {
                @()
            }
            $realtimeReadyCondition =
                'gNdsBattlePlayablePacingResult != 0'
            if ($foxCpuModeSelected -and ($FoxCpuMode -eq 1)) {
                # Source countdown mode must advance through GO before its CPU
                # path and timer assertions are meaningful.
                $realtimeReadyCondition +=
                    ' && gNdsFTComputerStickFrames != 0'
            }
            $gdbCommands = @(
                $gdbCommands[0..3]
                $armWaitCommands
                ('tbreak ndsBattlePlayableFrameCompleteMarker if {0}' -f
                 $realtimeReadyCondition)
                'continue'
                $weaponFrameCommands
                $gdbCommands[4..($gdbCommands.Count - 1)]
            )
        }
    }
    elseif ($BattlePlayable -and -not $RealtimePresentation -and -not $MatchLifecycleProof) {
        # Fast-logic mode can still be inside its expensive final draw after
        # the gameplay proof completes. Sample only after taskman teardown has
        # stopped BGM and published the scene boundary.
        $gdbCommands = @(
            $gdbCommands[0..3]
            'tbreak osStopThread if gNdsSceneBoundaryResult != 0'
            'continue'
            $gdbCommands[4..($gdbCommands.Count - 1)]
        )
    }
    if ($preBattleSelectorSelected) {
        # Install one shared pre-battle stop for all runtime selectors. This
        # must be added after the flow-specific waits above so it executes
        # before them without creating two scVSBattleStartBattle breakpoints.
        $preBattleSetupCommands = @(
            'tbreak scVSBattleStartBattle',
            'continue'
        )
        if ($Task34StageStreamCensus) {
            $preBattleSetupCommands += @(
                'printf "TASK34_ARENA_BOOT=%u,%u\n", gNdsTaskmanArenaChosenSize, gNdsTaskmanArenaAllocFailCount',
                'if gNdsTaskmanArenaChosenSize != 1376256 || gNdsTaskmanArenaAllocFailCount != 0',
                'printf "TASK34_ARENA_REJECT=%u,%u\n", gNdsTaskmanArenaChosenSize, gNdsTaskmanArenaAllocFailCount',
                'quit 86',
                'end'
            )
        }
        if ($staticTextureAotSelected -and
            -not $usesPublishedIntrinsicRendererDefaults) {
            $preBattleSetupCommands +=
                ('set variable gNdsRendererBattleStaticTextureEnabled = {0}' -f
                 $StaticTextureAotMode)
        }
        if ($foxCpuModeSelected) {
            $preBattleSetupCommands +=
                ('set variable gNdsBattlePlayableFoxCpuEnabled = {0}' -f
                 $FoxCpuMode)
        }
        if ($MatchLifecycleProof -and -not $OneMinuteMatchProof) {
            if ($Task9StateHashMode -eq 1) {
                $preBattleSetupCommands += @(
                    'set variable gNdsTask9StateHashArmed = 0',
                    'set variable gNdsTask9StateHashCount = 0',
                    'set variable gNdsTask9StateHashOverflow = 0',
                    'set variable gNdsTask9StateHashArmed = 1'
                )
            }
            $preBattleSetupCommands += @(
                'tbreak ndsMNVSResultsRecordFrame if gNdsVSResultsResult == 0x56535231',
                'continue'
            )
            if ($Task9StateHashMode -eq 1) {
                $preBattleSetupCommands += @(
                    'set variable gNdsTask9StateHashArmed = 0',
                    'printf "TASK9_STATE_SUMMARY=%u,%u,%u\n", gNdsTask9StateHashCount, gNdsTask9StateHashOverflow, 4096',
                    'set $task9_state_index = 0',
                    'while $task9_state_index < gNdsTask9StateHashCount',
                    'printf "TASK9_STATE=%u,%u,%u,%u,%u,%u\n", $task9_state_index, gNdsTask9StateHashes[$task9_state_index].hash1, gNdsTask9StateHashes[$task9_state_index].hash2, gNdsTask9StateHashes[$task9_state_index].bytes, gNdsTask9StateHashes[$task9_state_index].records, gNdsTask9StateHashes[$task9_state_index].overflow',
                    'set $task9_state_index = $task9_state_index + 1',
                    'end'
                )
            }
        }
        $gdbCommands = @(
            $gdbCommands[0..3]
            $preBattleSetupCommands
            $gdbCommands[4..($gdbCommands.Count - 1)]
        )
    }
    if ($HardwareTriangles) {
        $beforeDetach = $gdbCommands[0..($gdbCommands.Count - 3)]
        $afterDetach = $gdbCommands[($gdbCommands.Count - 2)..($gdbCommands.Count - 1)]
        $hardwareCommands = @(
            'printf "RENDER_PROFILE_LEVEL=%u\n", gNdsRendererProfileLevel',
            'printf "RENDER_M2_DETAILED_LEDGER=%u\n", gNdsRendererM2DetailedLedger',
            'printf "PLATFORM_HW=%u,%u,%u,%u,%#x,%#x\n", gNdsHardwareRendererSubmittedFrameCount, gNdsHardwareRendererFlushCount, gNdsHardwareRendererPolyRamCount, gNdsHardwareRendererVertexRamCount, gNdsHardwareRendererStatus, gNdsHardwareRendererControl',
            'printf "STAGE_GCDRAWALL_HW=%u,%u,%u,%u,%u,%u,%u,%u,%u,%#x,%u,%u\n", gNdsStageGCDrawAllLoopHardwareSubmitCount, gNdsStageGCDrawAllLoopHardwareTriangleCount, gNdsStageGCDrawAllLoopHardwareZBufferTriangleCount, gNdsStageGCDrawAllLoopHardwareProjectedDepthTriangleCount, gNdsStageGCDrawAllLoopHardwareDecalDepthTriangleCount, gNdsStageGCDrawAllLoopHardwareTextureBindCount, gNdsStageGCDrawAllLoopHardwareTextureUploadCount, gNdsStageGCDrawAllLoopHardwareTextureReadyCount, gNdsStageGCDrawAllLoopHardwareTextureRejectCount, gNdsStageGCDrawAllLoopHardwareTextureFormatMask, gNdsStageGCDrawAllLoopHardwareTextureMaxWidth, gNdsStageGCDrawAllLoopHardwareTextureMaxHeight',
            'printf "RENDER_STAGE_CARRY=%u,%u,%u,%u,%u,%u,%u\n", (gNdsStageGCDrawAllLoopHardwareCarrySeedCount < gNdsStageGCDrawAllLoopHardwareCarryCaptureCount) ? gNdsStageGCDrawAllLoopHardwareCarrySeedCount : gNdsStageGCDrawAllLoopHardwareCarryCaptureCount, (gNdsStageGCDrawAllLoopHardwareCarrySeedCount < gNdsStageGCDrawAllLoopHardwareCarryCaptureCount) ? gNdsStageGCDrawAllLoopHardwareCarrySeedCount : gNdsStageGCDrawAllLoopHardwareCarryCaptureCount, gNdsStageGCDrawAllLoopHardwareCarryTextureSeedCount, gNdsStageGCDrawAllLoopHardwareCarryTileSeedCount, gNdsStageGCDrawAllLoopHardwareCarryShortTextureSeedCount, gNdsStageGCDrawAllLoopHardwareCarryShortTileSeedCount, gNdsStageGCDrawAllLoopHardwareCarrySegmentSeedCount',
            'printf "STAGE_GCDRAWALL_HW_FTR=%u,%u\n", gNdsStageGCDrawAllLoopHardwareFighterSubmitCount, gNdsStageGCDrawAllLoopHardwareFighterTriangleCount',
            'printf "WEAPON_RENDER=%u,%u,%u,%u,%u,%u,%u,%#x,%u,%u,%u,%#x,%#x,%u,%u,%u,%u\n", gNdsWeaponRendererCaptureCount, gNdsWeaponRendererDObjDrawCount, gNdsWeaponRendererSubmitCount, gNdsWeaponRendererVisibleDrawCount, gNdsWeaponRendererTriangleCount, gNdsWeaponRendererTextureReadyCount, gNdsWeaponRendererTextureRejectCount, gNdsWeaponRendererKindMask, gNdsWeaponRendererCallbackKind, gNdsWeaponRendererNoZCount, gNdsWeaponRendererMovingDrawCount, gNdsWeaponRendererLastXBits, gNdsWeaponRendererLastYBits, gNdsWeaponRendererFireballSubmitCount, gNdsWeaponRendererFireballTriangleCount, gNdsWeaponRendererFireballVisibleDrawCount, gNdsWeaponRendererRejectedDrawCount',
        'printf "FTR_DISPLAY_CONTRACT=%u,%u,%u,%u,%#x,%u,%u,%u,%u,%#x,%#x,%u,%u,%#x,%#x\n", gNdsFighterDisplayContractSelectedCount, gNdsFighterDisplayContractHiddenCount, gNdsFighterDisplayContractNoTextureCount, gNdsFighterDisplayContractSubmittedCount, gNdsFighterDisplayContractGeometryMode, gNdsFighterDisplayContractLightCount, gNdsFighterDisplayContractLightDirectionCount, gNdsFighterDisplayContractBoundsPassCount, gNdsFighterDisplayContractBoundsFailCount, gNdsFighterDisplayContractBoundsXBits, gNdsFighterDisplayContractBoundsYBits, gNdsFighterDLAllDrawP0SelectedCount, gNdsFighterDLAllDrawP1SelectedCount, gNdsFighterDisplayContractCycleType, gNdsFighterDisplayContractRenderMode',
            'printf "FTR_LIGHT_SEED=%u,%#x,%#x\n", gNdsFighterDisplayContractMaterialLightSeedCount, gNdsFighterDisplayContractMaterialLight1, gNdsFighterDisplayContractMaterialLight2',
            'printf "DLALL_SCREEN=%d,%d,%d,%d,%d,%d,%d,%d,%#x,%#x,%#x,%#x\n", gNdsFighterDLAllDrawP0ScreenMinX, gNdsFighterDLAllDrawP0ScreenMaxX, gNdsFighterDLAllDrawP0ScreenMinY, gNdsFighterDLAllDrawP0ScreenMaxY, gNdsFighterDLAllDrawP1ScreenMinX, gNdsFighterDLAllDrawP1ScreenMaxX, gNdsFighterDLAllDrawP1ScreenMinY, gNdsFighterDLAllDrawP1ScreenMaxY, gNdsFighterDLAllDrawP0RootXBeforeBits, gNdsFighterDLAllDrawP0RootXAfterBits, gNdsFighterDLAllDrawP1RootXBeforeBits, gNdsFighterDLAllDrawP1RootXAfterBits',
            'printf "RENDER_PROFILE=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsRendererProfileFrameCount, gNdsRendererProfileUpdateTicks, gNdsRendererProfilePresentTicks, gNdsRendererProfileDrawTicks, gNdsRendererProfileHudTicks, gNdsRendererProfileStageAdapterTicks, gNdsRendererProfileMaterialTicks, gNdsRendererProfileMatrixTicks, gNdsRendererProfileDLTicks, gNdsRendererProfileTextureTicks, gNdsRendererProfileTextureConvertTicks, gNdsRendererProfileTextureUploadTicks, gNdsRendererProfileTextureUploads, gNdsRendererProfileTextureUploadBytes, gNdsRendererProfileTextureBinds, gNdsRendererProfileHardwareVertices, gNdsRendererProfileHardwareTriangles, gNdsRendererProfileHardwareOverLimit',
            'printf "RENDER_BATCH=%u,%u,%u,%u,%u\n", gNdsRendererProfileHardwareBatchBeginCount, gNdsRendererProfileHardwareBatchReuseCount, gNdsRendererProfileHardwareBatchEndCount, gNdsRendererProfileTexturePrepareCount, gNdsRendererProfileTexturePrepareReuseCount',
            'printf "RENDER_TOPOLOGY=%u,%u,%u,%u\n", gNdsRendererProfileImmutableListCount, gNdsRendererProfileTrustedCommandCount, gNdsRendererProfileValidatedCommandCount, gNdsRendererProfileTriangleRunReuseCount',
            'printf "RENDER_COST=%u,%u\n", gNdsRendererProfileTriangleSubmitTicks, gNdsRendererProfileVertexSubmitTicks',
            'printf "RENDER_CI4LUT=%u,%u,%u,%u\n", gNdsRendererProfileCi4LutBuildCount, gNdsRendererProfileCi4LutReuseCount, gNdsRendererProfileCi4IndexCacheBuildCount, gNdsRendererProfileCi4IndexCacheReuseCount',
            'printf "RENDER_CI4MAP=%u,%u\n", gNdsRendererProfileCi4RepresentativePixelCount, gNdsRendererProfileCi4ReusePixelCount',
            'printf "RENDER_TEXHASH=%u,%u,%u,%u,%u\n", gNdsRendererProfileTextureLookupCallCount, gNdsRendererProfileTextureLookupProbeCount, gNdsRendererProfileTextureLookupActiveHitCount, gNdsRendererProfileTextureLookupTableHitCount, gNdsRendererProfileTextureLookupMissCount',
            'printf "RENDER_ORACLE=%u,%u,%u\n", gNdsRendererProfileOracleSamples, gNdsRendererProfileOracleMismatches, gNdsRendererProfileOracleMaxDelta',
            'printf "RENDER_MATRIX=%u,%u,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n", gNdsRendererProfileMatrixLoadCount, gNdsRendererProfileMatrixScaleWorld, gNdsRendererProfileProjectionM00, gNdsRendererProfileProjectionM11, gNdsRendererProfileProjectionM22, gNdsRendererProfileProjectionM32, gNdsRendererProfileModelviewM00, gNdsRendererProfileModelviewM11, gNdsRendererProfileModelviewM22, gNdsRendererProfileModelviewM30, gNdsRendererProfileModelviewM31, gNdsRendererProfileModelviewM32',
            'printf "RENDER_ADAPTER_CACHE=%u,%u,%u,%u,%u,%u\n", gNdsRendererProfileCameraMatrixCacheHitCount, gNdsRendererProfileCameraMatrixCacheMissCount, gNdsRendererProfileCameraMatrixCacheOverflowCount, gNdsRendererProfileDObjWorldCacheHitCount, gNdsRendererProfileDObjWorldCacheMissCount, gNdsRendererProfileDObjWorldCacheOverflowCount',
            'printf "RENDER_STAGE_WORLD_CACHE=%u,%u,%u,%u,%u,%u\n", gNdsRendererProfileStageWorldPersistentHitCount, gNdsRendererProfileStageWorldPersistentMissCount, gNdsRendererProfileStageWorldPersistentRejectCount, gNdsRendererProfileStageWorldPersistentOverflowCount, gNdsRendererProfileStageWorldPersistentOracleSampleCount, gNdsRendererProfileStageWorldPersistentOracleMismatchCount',
            'printf "RENDER_AFFINE_MATRIX=%u,%u,%u\n", gNdsRendererProfileAffineMatrixSamples, gNdsRendererProfileAffineMatrixMismatches, gNdsRendererProfileAffineMatrixMaxDelta',
            'printf "RENDER_RAW_MATRIX=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsRendererProfileRawCurrentCandidateCount, gNdsRendererProfileRawCurrentRangeRejectCount, gNdsRendererProfileRawCrossMatrixCount, gNdsRendererProfileMatrixPosTestSamples, gNdsRendererProfileMatrixPosTestMismatches, gNdsRendererProfileMatrixPosTestMaxError, gNdsRendererProfileMatrixPosTestWSignMismatches, gNdsRendererProfileMatrixPosTestClipMismatches, gNdsRendererProfileMatrixPosTestMatrixWordSamples, gNdsRendererProfileMatrixPosTestDropped',
            'printf "RENDER_SUBMIT=%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsRendererProfileSubmitRawCurrentCount, gNdsRendererProfileSubmitRawSnapshotCount, gNdsRendererProfileSubmitProjectedCrossCount, gNdsRendererProfileSubmitProjectedNoZCount, gNdsRendererProfileSubmitProjectedDecalCount, gNdsRendererProfileSubmitProjectedPrimDepthCount, gNdsRendererProfileSubmitProjectedRangeOrMatrixCount, gNdsRendererProfileSubmitRejectCount, (6 * gNdsRendererProfileSubmitProjectedNoZCount) + (9 * (gNdsRendererProfileSubmitProjectedCrossCount + gNdsRendererProfileSubmitProjectedDecalCount + gNdsRendererProfileSubmitProjectedPrimDepthCount + gNdsRendererProfileSubmitProjectedRangeOrMatrixCount))',
            'printf "RENDER_HWDIV=%u,%u,%u,%u,%u\n", gNdsRendererProfileHardwareDivideSummary & 0xfff, (gNdsRendererProfileHardwareDivideSummary >> 12) & 0xff, (gNdsRendererProfileHardwareDivideSummary >> 20) & 0xff, (gNdsRendererProfileHardwareDivideSummary >> 28) & 1, (gNdsRendererProfileHardwareDivideSummary >> 29) & 1',
            'printf "RENDER_LAZY=%u,%u,%u,%u,%u,%u\n", gNdsRendererProfileSourceVertexLoadCount, gNdsRendererProfileCPUTransformCount, gNdsRendererProfileTransformCacheHitCount, gNdsRendererProfileMatrixSnapshotCreateCount, gNdsRendererProfileMatrixSnapshotReuseCount, gNdsRendererProfileMatrixSnapshotOverflowCount',
            'printf "RENDER_VERTEX=%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%u\n", gNdsRendererProfileRawVertexMinX, gNdsRendererProfileRawVertexMaxX, gNdsRendererProfileRawVertexMinY, gNdsRendererProfileRawVertexMaxY, gNdsRendererProfileRawVertexMinZ, gNdsRendererProfileRawVertexMaxZ, gNdsRendererProfileHWVertexMinX, gNdsRendererProfileHWVertexMaxX, gNdsRendererProfileHWVertexMinY, gNdsRendererProfileHWVertexMaxY, gNdsRendererProfileHWVertexMinZ, gNdsRendererProfileHWVertexMaxZ, gNdsRendererProfileHWVertexSaturateCount',
            'printf "RENDER_DEPTH=%u,%d,%d,%d,%d,%u,%d,%d,%d,%d,%u,%d,%d,%d,%d\n", gNdsRendererDepthStageSamples, gNdsRendererDepthStageMin, gNdsRendererDepthStageMax, gNdsRendererDepthStageWMin, gNdsRendererDepthStageWMax, gNdsRendererDepthFighterP0Samples, gNdsRendererDepthFighterP0Min, gNdsRendererDepthFighterP0Max, gNdsRendererDepthFighterP0WMin, gNdsRendererDepthFighterP0WMax, gNdsRendererDepthFighterP1Samples, gNdsRendererDepthFighterP1Min, gNdsRendererDepthFighterP1Max, gNdsRendererDepthFighterP1WMin, gNdsRendererDepthFighterP1WMax',
            'printf "RENDER_CLIP=%u\n", gNdsRendererProfileHWVertexSaturateCount',
            'printf "RENDER_TEXTURE=%u,%u,%u,%u,%u,%u,%u,%u,%d,%d,%d,%d\n", gNdsRendererProfileTextureSourceTexels, gNdsRendererProfileTextureGreenTexels, gNdsRendererProfileTextureNonWhiteTexels, gNdsRendererProfileTexturedVertexCount, gNdsRendererProfileTextureSampleCount, gNdsRendererProfileTextureSampleGreenCount, gNdsRendererProfileTextureSampleNonWhiteCount, gNdsRendererProfileTextureCacheAliasAvoidCount, gNdsRendererProfileTextureCoordMinS, gNdsRendererProfileTextureCoordMaxS, gNdsRendererProfileTextureCoordMinT, gNdsRendererProfileTextureCoordMaxT',
            'printf "RENDER_TEXEL1=%u,%u,%u,%#x,%u,%#x,%#x,%#x,%#x,%u,%u,%u\n", gNdsRendererProfileTexel1CompositeCount, gNdsRendererProfileTexel1LoadMatchCount, gNdsRendererProfileTexel1RejectCount, gNdsRendererProfileTexel1RejectReasonMask, gNdsRendererProfileTexel1LastFraction, gNdsRendererProfileTexel1LastImage0, gNdsRendererProfileTexel1LastImage1, gNdsRendererProfileTexel1LastTileState, gNdsRendererProfileTexel1LastPrimaryState, gNdsRendererProfileTexel1FractionRefreshCount, gNdsRendererProfileTextureCacheEvictCount, gNdsRendererProfileTextureCi4DirectPixels',
            'printf "RENDER_TEXUSE=%u,%u,%u,%u,%u,%u,%#x,%#x,%#x,%#x,%#x\n", gNdsRendererProfileUseTextureRejectNoStatsCount, gNdsRendererProfileUseTextureRejectStateOffCount, gNdsRendererProfileUseTextureRejectNoCombineCount, gNdsRendererProfileUseTextureRejectPrimitiveDecalCount, gNdsRendererProfileUseTextureRejectNoTexel0Count, gNdsRendererProfileUseTextureImplicitOnCount, gNdsRendererProfileUseTextureRejectFirstReason, gNdsRendererProfileUseTextureRejectFirstFlags, gNdsRendererProfileUseTextureRejectFirstW0, gNdsRendererProfileUseTextureRejectFirstW1, gNdsRendererProfileUseTextureRejectFirstGeometry',
            'printf "RENDER_TEXFMT=%#x,%#x,%#x,%#x,%#x\n", gNdsRendererProfileTextureConvertFormatMask, gNdsRendererProfileTextureBindFormatMask, gNdsRendererProfileTexturePaletteFormatMask, gNdsRendererProfileTextureRejectFormatMask, gNdsRendererProfileTextureRejectReasonMask',
            'printf "RENDER_TEXLANE=%#x,%u,%u,%#x,%#x,%#x,%#x\n", gNdsRendererProfileTextureLaneLayoutMask, gNdsRendererProfileTextureLaneByteAccessCount, gNdsRendererProfileTextureLaneHalfwordAccessCount, gNdsRendererProfileTextureLaneByteFormatMask, gNdsRendererProfileTextureLaneHalfwordFormatMask, gNdsRendererProfileTextureLaneByteMap, gNdsRendererProfileTextureLaneHalfwordMap',
            'printf "RENDER_COMBINE=%u,%u,%u,%u,%u,%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x\n", gNdsRendererProfileCombineModeCount, gNdsRendererProfileCombineModeDistinctCount, gNdsRendererProfileLitShadeCombineCount, gNdsRendererProfileMaterialCombineCount, gNdsRendererProfileProjectedSubmitFallbackCount, gNdsRendererProfileCombineMode0W0, gNdsRendererProfileCombineMode0W1, gNdsRendererProfileCombineMode1W0, gNdsRendererProfileCombineMode1W1, gNdsRendererProfileCombineMode2W0, gNdsRendererProfileCombineMode2W1, gNdsRendererProfileCombineMode3W0, gNdsRendererProfileCombineMode3W1',
            'printf "RENDER_LIGHT=%u,%u,%u\n", gNdsRendererProfileLightColorCommands, gNdsRendererProfileLightDirectionCommands, gNdsRendererProfileLightFallbackCount',
            'printf "M4_FENCE_FINAL=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsRendererProfileFrameCount, gNdsRendererBattleStaticTextureEnabled, gNdsRendererBattleStaticTexturePrepareCount, gNdsRendererBattleStaticTexturePrepareFailCount, gNdsRendererBattleStaticTexturePreparedCount, gNdsRendererBattleStaticTexturePreparedBytes, gNdsRendererBattleStaticTextureArmCount, gNdsRendererBattleStaticTextureTeardownCount, gNdsRendererBattleStaticTextureSeenMask, gNdsRendererBattleStaticTextureOwnerMask, gNdsRendererBattleStaticTextureViolationCount, gNdsRendererBattleStaticTexturePinnedHitCount, gNdsRendererBattleTextureFenceFirstClassPlus1, gNdsRendererBattleTextureFenceFirstFrame, gNdsRendererBattleTextureFenceCounts[0], gNdsRendererBattleTextureFenceCounts[1], gNdsRendererBattleTextureFenceCounts[2], gNdsRendererBattleTextureFenceCounts[3], gNdsRendererBattleTextureFenceCounts[4], gNdsRendererBattleTextureFenceCounts[5], gNdsRendererBattleTextureFenceCounts[6], gNdsRendererBattleTextureFenceCounts[7], gNdsRendererBattleTextureFenceCounts[8], gNdsRendererBattleTextureFenceCounts[9]',
            'printf "FAST_FINAL=%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsRendererFastRunMode, gNdsRendererFastRunCount, gNdsRendererFastTriangleCount, gNdsRendererFastOwnerTriangleCount[0], gNdsRendererFastOwnerTriangleCount[1], gNdsRendererFastOwnerTriangleCount[2], gNdsRendererFastFallbackCount[0], gNdsRendererFastFallbackCount[1], gNdsRendererFastFallbackCount[2]',
            'printf "M4_WATER_STILL_FINAL=%u,%u,%u\n", gNdsPupupuWaterStillFreezeCount, gNdsPupupuWaterStillFreezeFailCount, gNdsPupupuWaterStillFreezeResult'
        )
        if ($m4CandidateEvidence) {
            $hardwareCommands += 'printf "VRAM_BANKS=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", *(volatile unsigned char *)0x04000240, *(volatile unsigned char *)0x04000241, *(volatile unsigned char *)0x04000242, *(volatile unsigned char *)0x04000243, *(volatile unsigned char *)0x04000244, *(volatile unsigned char *)0x04000245, *(volatile unsigned char *)0x04000246, *(volatile unsigned char *)0x04000248, gNdsRendererBattleStaticTextureFirstAddress, gNdsRendererBattleStaticTextureEndAddress, gNdsRendererBattleStaticTextureAllocationSpanBytes, gNdsRendererBattleStaticTextureBankMask'
        }
        if ($RendererProfileLevel -ge 2) {
            $hardwareCommands += 'printf "STAGE_DEPTH_TRACE=%u,%u,%#x,%u,%u,%d,%d,%u,%d,%d,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsRendererStageDepthTraceCount, gNdsRendererStageDepthTraceOverflowCount, gNdsRendererStageDepthTraceHash, gNdsRendererStageDepthTraceNoZCollisionCount, gNdsRendererStageDepthTraceBackgroundCount, gNdsRendererStageDepthTraceBackgroundMin, gNdsRendererStageDepthTraceBackgroundMax, gNdsRendererStageDepthTraceForegroundCount, gNdsRendererStageDepthTraceForegroundMin, gNdsRendererStageDepthTraceForegroundMax, gNdsRendererStageDepthTraceClassCount[0], gNdsRendererStageDepthTraceClassCount[1], gNdsRendererStageDepthTraceClassCount[2], gNdsRendererStageDepthTraceClassCount[3], gNdsRendererStageDepthTraceClassCount[4], gNdsRendererStageDepthTraceClassCount[5], gNdsRendererStageDepthTraceClassCount[6], gNdsRendererStageDepthTraceClassCount[7]'
        }
        if ($BattlePlayable -and $RealtimePresentation) {
            $hardwareCommands += 'printf "SOBJ_WALL_CACHE=%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsSObjWallpaperCacheBuildCount, gNdsSObjWallpaperCacheHitCount, gNdsSObjWallpaperCacheFastDrawCount, gNdsSObjWallpaperCacheFallbackCount, gNdsSObjWallpaperCacheWidth, gNdsSObjWallpaperCacheHeight, gNdsSObjWallpaperCacheOpaquePixels, gNdsSObjWallpaperCacheBuildTicks, gNdsSObjWallpaperCacheDrawTicks'
            $hardwareCommands += 'printf "SOBJ_WALL_FINAL=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsSObjWallpaperFinalDirectCount, gNdsSObjWallpaperFinalSkipCount, gNdsSObjWallpaperFinalKeyChangeCount, gNdsSObjWallpaperFinalPixelWriteCount, gNdsSObjBackgroundStagingClearBytes, gNdsSObjForegroundStagingClearBytes, gNdsOriginalSpriteBg2ClearBytes, gNdsOriginalSpriteBg2CopyBytes, gNdsOriginalSpriteBg2FinalWriteBytes, gNdsOriginalSpriteBg3ClearBytes, gNdsOriginalSpriteBg3CopyBytes, gNdsOriginalSpriteBg3FinalWriteBytes'
            if ($usesFastWallpaper) {
                if ($RendererProfileLevel -ge 1) {
                    $hardwareCommands += 'printf "FAST_WALLPAPER=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%#x,%u,%u\n", gNdsFastWallpaperState, gNdsFastWallpaperSeedAttemptCount, gNdsFastWallpaperSeedSuccessCount, gNdsFastWallpaperSeedFailureCount, gNdsFastWallpaperStaticDegradedCount, gNdsFastWallpaperSeedTicks, gNdsFastWallpaperQueueCount, gNdsFastWallpaperApplyCount, gNdsFastWallpaperUnchangedSkipCount, gNdsFastWallpaperClampXCount, gNdsFastWallpaperClampYCount, gNdsFastWallpaperClampScaleCount, gNdsFastWallpaperInvalidTransformCount, gNdsFastWallpaperReusePreviousCount, gNdsFastWallpaperAffineLastTicks, gNdsFastWallpaperPostReadySoftwareDrawCount, gNdsFastWallpaperPostReadyPixelWriteCount, gNdsFastWallpaperSeedHash, gNdsFastWallpaperSeedOpaquePixelCount, gNdsFastWallpaperSeedRestoreMismatchCount'
                } else {
                    # Detailed BG-0 timing/churn counters deliberately link out
                    # of profile 0. Preserve the marker schema without asking
                    # GDB to read discarded symbols.
                    $hardwareCommands += 'printf "FAST_WALLPAPER=%u,%u,%u,%u,%u,0,0,0,0,0,0,0,0,0,0,%u,%u,%#x,%u,%u\n", gNdsFastWallpaperState, gNdsFastWallpaperSeedAttemptCount, gNdsFastWallpaperSeedSuccessCount, gNdsFastWallpaperSeedFailureCount, gNdsFastWallpaperStaticDegradedCount, gNdsFastWallpaperPostReadySoftwareDrawCount, gNdsFastWallpaperPostReadyPixelWriteCount, gNdsFastWallpaperSeedHash, gNdsFastWallpaperSeedOpaquePixelCount, gNdsFastWallpaperSeedRestoreMismatchCount'
                }
            }
            # Profile 0 intentionally links out post-VBlank timing state. Keep
            # the marker's stable 31-field schema without forcing a diagnostic
            # symbol into the user-facing ROM solely for GDB sampling.
            $postVBlankTicksExpression = if ($RendererProfileLevel -ge 1) {
                'gNdsRendererProfilePostVBlankTicks'
            } else {
                '0'
            }
            $hardwareCommands += ('printf "IFCOMMON_OAM=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%#x,%u,%u,%u,%u,%u,%u\n", gNdsIFCommonNativeOamEnabled, gNdsIFCommonNativeOamPrepareCount, gNdsIFCommonNativeOamPrepareSuccessCount, gNdsIFCommonNativeOamPrepareFailCount, gNdsIFCommonNativeOamPrepareTicks, gNdsIFCommonNativeOamPrepareBytes, gNdsIFCommonNativeOamPrepareAssets, gNdsIFCommonNativeOamPrepareTiles, gNdsIFCommonNativeOamPrepareProfileFrame, gNdsIFCommonNativeOamPreparePaletteBytes, gNdsIFCommonNativeOamHotConvertCount, gNdsIFCommonNativeOamRuntimeUploadBytes, gNdsIFCommonNativeOamFrameBeginTicks, gNdsIFCommonNativeOamFrameTicks, gNdsIFCommonNativeOamFrameCommitTicks, gNdsIFCommonNativeOamFrameCommitCalls, gNdsIFCommonNativeOamFrameClearedObjects, gNdsIFCommonNativeOamFrameIdle, gNdsIFCommonNativeOamIdleFrameCount, gNdsIFCommonNativeOamFrameRecognizedCalls, gNdsIFCommonNativeOamFrameDrawCalls, gNdsIFCommonNativeOamFrameFallbackCalls, gNdsIFCommonNativeOamFrameSObjCount, gNdsIFCommonNativeOamFrameObjectCount, gNdsIFCommonNativeOamFrameSemanticHash, gNdsIFCommonNativeOamLastFallbackReason, gNdsIFCommonNativeOamCommitCount, {0}, gNdsRendererProfileFrameCount, gNdsOriginalSpriteBg3CopyBytes, gNdsSObjForegroundStagingClearBytes' -f $postVBlankTicksExpression)
            if ($RendererProfileLevel -ge 2) {
                $hardwareCommands += 'printf "SOBJ_WALL_ORACLE=%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsSObjWallpaperMapOracleCheckCount, gNdsSObjWallpaperMapOracleMismatchCount, gNdsSObjWallpaperPixelOracleCheckCount, gNdsSObjWallpaperPixelOracleMismatchCount, gNdsSObjWallpaperOracleFirstKind, gNdsSObjWallpaperOracleFirstIndex, gNdsSObjWallpaperOracleFirstExpected, gNdsSObjWallpaperOracleFirstActual'
            }
        }
        if ($usesRetainedWallpaper) {
            $hardwareCommands += 'printf "SCENE_MIP_CACHE=%u,%u,%u,%u,%#x,%u,%u,%u,%u,%u,%#x,%#x\n", gNdsSceneMipCacheState, gNdsSceneMipCacheCaptureCount, gNdsSceneMipCacheUploadCount, gNdsSceneMipCacheFailureCount, gNdsSceneMipCacheLastHash, gNdsSceneMipCacheLastNonzeroPixels, gNdsSceneMipCacheSeedDrawCount, gNdsSceneMipCacheDrawCount, gNdsSceneMipCacheFallbackCount, gNdsSceneMipCacheSelectedMip, gNdsSceneMipCacheTargetDistanceBits, gNdsSceneMipCacheSelectedMipMask'
            $hardwareCommands += 'printf "SCENE_WALL_AFFINE=%u,%u,%u,%u,%d,%d,%d,%d\n", gNdsSceneWallpaperAffineQueueCount, gNdsSceneWallpaperAffineApplyCount, gNdsSceneWallpaperAffineCoverageFailureCount, gNdsSceneWallpaperAffineLastTicks, gNdsSceneWallpaperAffineHdx, gNdsSceneWallpaperAffineVdy, gNdsSceneWallpaperAffineDx, gNdsSceneWallpaperAffineDy'
        }
        $gdbCommands = @($beforeDetach + $hardwareCommands + $afterDetach)
    }
    if ($BattlePlayable) {
        $beforeDetach = $gdbCommands[0..($gdbCommands.Count - 3)]
        $afterDetach = $gdbCommands[($gdbCommands.Count - 2)..($gdbCommands.Count - 1)]
        $battlePlayableCommands = @(
            'printf "BPLAY_KO=%#x,%#x,%u,%u,%u,%u,%u,%u,%u\n", gNdsFighterBattlePlayableResult, gNdsFighterBattlePlayableMask, gNdsFighterBattlePlayableVictimSlot, gNdsFighterBattlePlayableVictimStockStart, gNdsFighterBattlePlayableVictimStockFinal, gNdsFighterBattlePlayableBattleStockStart, gNdsFighterBattlePlayableBattleStockFinal, gNdsFighterBattlePlayableFallsStart, gNdsFighterBattlePlayableFallsFinal',
            'printf "BPLAY_STATUS=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsFighterBattlePlayableDeadFrames, gNdsFighterBattlePlayableRebirthDownFrames, gNdsFighterBattlePlayableRebirthStandFrames, gNdsFighterBattlePlayableRebirthWaitFrames, gNdsFighterBattlePlayableFallAfterRebirthFrames, gNdsFighterBattlePlayableWaitAfterRebirthFrames, gNdsFighterBattlePlayableFinalStatus, gNdsFighterBattlePlayableFinalGA, gNdsFighterBattlePlayableFinalIsRebirth, gNdsFighterBattlePlayableKOStickFrames',
            'printf "BPLAY_MAP=%u,%u,%u,%u,%u,%#x,%#x,%d,%d,%d,%d,%d,%u,%u\n", gNdsFighterBattlePlayableMapCallCount, gNdsFighterBattlePlayableMapHitCount, gNdsFighterBattlePlayableMapFloorHitCount, gNdsFighterBattlePlayableMapCliffHitCount, gNdsFighterBattlePlayableMapCeilHitCount, gNdsFighterBattlePlayableMapLastMaskStat, gNdsFighterBattlePlayableMapLastMaskCurr, gNdsFighterBattlePlayableFinalXMilli, gNdsFighterBattlePlayableFinalYMilli, gNdsFighterBattlePlayableFinalVelXMilli, gNdsFighterBattlePlayableFinalVelYMilli, gNdsFighterBattlePlayableFinalFloorDistMilli, gNdsFighterBattlePlayableFinalFloor, gNdsFighterBattlePlayableFinalIsGhost',
            'printf "BPLAY_FT=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", ((FTStruct *)gGCCommonLinks[3]->user_data.p)->player, ((FTStruct *)gGCCommonLinks[3]->user_data.p)->fkind, ((FTStruct *)gGCCommonLinks[3]->user_data.p)->pkind, ((FTStruct *)gGCCommonLinks[3]->user_data.p)->is_ghost, ((FTStruct *)gGCCommonLinks[3]->user_data.p)->hitstatus, ((FTStruct *)gGCCommonLinks[3]->user_data.p)->special_hitstatus, ((FTStruct *)gGCCommonLinks[3]->user_data.p)->star_hitstatus, ((FTStruct *)gGCCommonLinks[3]->user_data.p)->status_id, ((FTStruct *)gGCCommonLinks[3]->user_data.p)->motion_id, ((FTStruct *)gGCCommonLinks[3]->user_data.p)->percent_damage, ((FTStruct *)gGCCommonLinks[3]->link_next->user_data.p)->player, ((FTStruct *)gGCCommonLinks[3]->link_next->user_data.p)->fkind, ((FTStruct *)gGCCommonLinks[3]->link_next->user_data.p)->pkind, ((FTStruct *)gGCCommonLinks[3]->link_next->user_data.p)->is_ghost, ((FTStruct *)gGCCommonLinks[3]->link_next->user_data.p)->hitstatus, ((FTStruct *)gGCCommonLinks[3]->link_next->user_data.p)->special_hitstatus, ((FTStruct *)gGCCommonLinks[3]->link_next->user_data.p)->star_hitstatus, ((FTStruct *)gGCCommonLinks[3]->link_next->user_data.p)->status_id, ((FTStruct *)gGCCommonLinks[3]->link_next->user_data.p)->motion_id, ((FTStruct *)gGCCommonLinks[3]->link_next->user_data.p)->percent_damage',
            'printf "BPLAY_HURT=%u,%d,%u,%d,%u,%d,%u,%d,%u,%d,%u,%d\n", ((FTStruct *)gGCCommonLinks[3]->user_data.p)->damage_colls[0].hitstatus, ((FTStruct *)gGCCommonLinks[3]->user_data.p)->damage_colls[0].joint_id, ((FTStruct *)gGCCommonLinks[3]->user_data.p)->damage_colls[1].hitstatus, ((FTStruct *)gGCCommonLinks[3]->user_data.p)->damage_colls[1].joint_id, ((FTStruct *)gGCCommonLinks[3]->user_data.p)->damage_colls[2].hitstatus, ((FTStruct *)gGCCommonLinks[3]->user_data.p)->damage_colls[2].joint_id, ((FTStruct *)gGCCommonLinks[3]->link_next->user_data.p)->damage_colls[0].hitstatus, ((FTStruct *)gGCCommonLinks[3]->link_next->user_data.p)->damage_colls[0].joint_id, ((FTStruct *)gGCCommonLinks[3]->link_next->user_data.p)->damage_colls[1].hitstatus, ((FTStruct *)gGCCommonLinks[3]->link_next->user_data.p)->damage_colls[1].joint_id, ((FTStruct *)gGCCommonLinks[3]->link_next->user_data.p)->damage_colls[2].hitstatus, ((FTStruct *)gGCCommonLinks[3]->link_next->user_data.p)->damage_colls[2].joint_id',
            'printf "BPLAY_ATTACK=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", ((FTStruct *)gGCCommonLinks[3]->user_data.p)->is_attack_active, ((FTStruct *)gGCCommonLinks[3]->user_data.p)->attack_colls[0].attack_state, ((FTStruct *)gGCCommonLinks[3]->user_data.p)->attack_colls[1].attack_state, ((FTStruct *)gGCCommonLinks[3]->user_data.p)->attack_colls[2].attack_state, ((FTStruct *)gGCCommonLinks[3]->user_data.p)->attack_colls[3].attack_state, ((FTStruct *)gGCCommonLinks[3]->link_next->user_data.p)->is_attack_active, ((FTStruct *)gGCCommonLinks[3]->link_next->user_data.p)->attack_colls[0].attack_state, ((FTStruct *)gGCCommonLinks[3]->link_next->user_data.p)->attack_colls[1].attack_state, ((FTStruct *)gGCCommonLinks[3]->link_next->user_data.p)->attack_colls[2].attack_state, ((FTStruct *)gGCCommonLinks[3]->link_next->user_data.p)->attack_colls[3].attack_state',
            'printf "BPLAY_PACE=%#x,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsBattlePlayablePacingResult, gNdsBattlePlayablePacingMode, gNdsBattlePlayablePacingLogicFrames, gNdsBattlePlayablePacingPresentedFrames, gNdsBattlePlayablePacingDrawCalls, gNdsBattlePlayablePacingTimerTicks, gNdsBattlePlayablePacingPresentFpsX10, gNdsBattlePlayablePacingLogicFpsX10, gNdsBattlePlayablePacingVBlanks, gNdsBattlePlayablePacingPresentIntervalMin, gNdsBattlePlayablePacingPresentIntervalMax, gNdsBattlePlayablePacingCadenceViolationCount, gNdsBattlePlayablePacingPhasePresentCount[0], gNdsBattlePlayablePacingPhasePresentCount[1], gNdsBattlePlayablePacingPhasePresentCount[2], gNdsBattlePlayablePacingPhasePresentCount[3], gNdsBattlePlayablePacingPhasePresentCount[4], gNdsBattlePlayablePacingPhaseSlipCount[0], gNdsBattlePlayablePacingPhaseSlipCount[1], gNdsBattlePlayablePacingPhaseSlipCount[2], gNdsBattlePlayablePacingPhaseSlipCount[3], gNdsBattlePlayablePacingPhaseSlipCount[4]',
            'printf "FPS_HUD=%u,%u,%u,%u\n", gNdsBattlePlayableHudFpsX10, gNdsBattlePlayableHudFpsSampleCount, gNdsBattlePlayableHudFpsFrameWindow, gNdsBattlePlayableHudFpsTickWindow',
            'printf "BATTLE_TEXT_HUD=%u,%u,%#x,%u,%u,%u,%u,%u,%#x,%#x\n", gNdsBattleTextHudRenderCount, gNdsBattleTextHudChangeCount, gNdsBattleTextHudFingerprint, gNdsBattleTextHudTimeSeconds, gNdsBattleTextHudP0Damage, gNdsBattleTextHudP1Damage, gNdsBattleTextHudP0Stock, gNdsBattleTextHudP1Stock, gNdsBattleTextHudActiveMask, gNdsBattleTextHudShowDamageMask',
            'printf "MEMARENA=%#x,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsMemoryLedgerResult, gNdsMemoryLedgerScene, gNdsMemoryLedgerGeneration, gNdsMemoryLedgerArenaCapacity, gNdsMemoryLedgerArenaUsed, gNdsMemoryLedgerArenaHighWater, gNdsMemoryLedgerArenaHeadroom, gNdsMemoryLedgerDLBytes, gNdsMemoryLedgerGraphicsBytes, gNdsMemoryLedgerRdpBytes, gNdsMemoryLedgerFigatreeHeapSize',
            'printf "MEMRELOC=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsMemoryLedgerRelocFiles, gNdsMemoryLedgerRelocBytes, gNdsMemoryLedgerRelocStageBytes, gNdsMemoryLedgerRelocFighterBytes, gNdsMemoryLedgerRelocInterfaceBytes, gNdsMemoryLedgerRelocMenuBytes, gNdsMemoryLedgerRelocOpeningBytes, gNdsMemoryLedgerRelocOtherBytes, gNdsMemoryLedgerRelocStaleFiles, gNdsMemoryLedgerRelocStaleBytes',
            'printf "MEMEVICT=%u,%u\n", gNdsMemoryLedgerEvictedFiles, gNdsMemoryLedgerEvictedBytes'
        )
        $battlePlayableCommands += 'printf "BPLAY_START=%u,%u,%u,%u,%u,%u,%u,%u\n", gSCManagerBattleState->game_status, gSCManagerBattleState->time_limit, gSCManagerBattleState->time_remain, gSCManagerBattleState->time_passed, sIFCommonTimerIsStarted, ((FTStruct *)gGCCommonLinks[3]->user_data.p)->is_control_disable, ((FTStruct *)gGCCommonLinks[3]->link_next->user_data.p)->is_control_disable, gNdsBattlePlayablePacingLogicFrames'
        $gdbCommands = @($beforeDetach + $battlePlayableCommands + $afterDetach)
    }
    if ($ImportBattleShipFTComputer) {
        $beforeDetach = $gdbCommands[0..($gdbCommands.Count - 3)]
        $afterDetach = $gdbCommands[($gdbCommands.Count - 2)..($gdbCommands.Count - 1)]
        $computerCommands = @(
            'printf "CPU_CONFIG=%u,%u,%u,%u,%u,%u,%#x,%u,%u\n", gSCManagerBattleState->players[0].pkind, gSCManagerBattleState->players[1].pkind, gSCManagerBattleState->players[1].level, gSCManagerBattleState->pl_count, gSCManagerBattleState->cp_count, gSCManagerBattleState->time_limit, gSCManagerBattleState->item_toggles, gSCManagerBattleState->item_appearance_rate, gNdsBattlePlayableFoxCpuEnabled',
            'printf "CPU_AI=%u,%u,%u,%u,%#x,%#x,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%d,%d,%d,%d\n", gNdsFTComputerSetupCount, gNdsFTComputerDamageDetectCount, gNdsFTComputerProcessCount, gNdsFTComputerTargetFrames, gNdsFTComputerObjectiveMask, gNdsFTComputerBehaviorMask, gNdsFTComputerInputChangeCount, gNdsFTComputerStickFrames, gNdsFTComputerButtonAFrames, gNdsFTComputerButtonBFrames, gNdsFTComputerButtonZFrames, gNdsFTComputerAttackFrames, gNdsFTComputerHitboxFrames, gNdsFTComputerGuardFrames, gNdsFTComputerRecoveryFrames, gNdsFTComputerStatusChangeCount, gNdsFTComputerFinalStatus, gNdsFTComputerFinalGA, gNdsFTComputerFinalInputKind, gNdsFTComputerMarioDamageMax, gNdsFTComputerFloorLineCount, gNdsFTComputerStartXMilli, gNdsFTComputerMinXMilli, gNdsFTComputerMaxXMilli, gNdsFTComputerFinalXMilli'
        )
        $gdbCommands = @($beforeDetach + $computerCommands + $afterDetach)
    }
    if ($MatchLifecycleProof) {
        $beforeDetach = $gdbCommands[0..($gdbCommands.Count - 3)]
        $afterDetach = $gdbCommands[($gdbCommands.Count - 2)..($gdbCommands.Count - 1)]
        $lifecycleCommands = @(
            'printf "VSB_END=%#x,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsSCVSBattleLifecycleResult, gNdsSCVSBattleLifecycleArenaAdapterCount, gNdsSCVSBattleLifecycleTaskmanExitCount, gNdsSCVSBattleLifecycleTaskmanStatus, gNdsSCVSBattleLifecycleTimeLimit, gNdsSCVSBattleLifecycleTimeRemain, gNdsSCVSBattleLifecycleTimePassed, gNdsSCVSBattleLifecycleGameStatus, gNdsSCVSBattleLifecycleScenePrev, gNdsSCVSBattleLifecycleSceneCurr, gNdsSCVSBattleLifecycleIsSuddenDeath',
            'printf "VS_RESULTS=%#x,%#x,%u,%u,%u,%u,%u,%u,%u\n", gNdsVSResultsResult, gNdsVSResultsMask, gNdsVSResultsStartCount, gNdsVSResultsTickCount, gNdsVSResultsLoadedFileCount, gNdsVSResultsFighterCount, gNdsVSResultsGObjCount, gNdsVSResultsSObjCount, gNdsVSResultsKind',
            'printf "VS_RESULTS_FIGHTERS=%u,%#x,%d,%u,%#x,%d\n", gNdsVSResultsFighterPlace[0], gNdsVSResultsFighterStatus[0], gNdsVSResultsFighterMotion[0], gNdsVSResultsFighterPlace[1], gNdsVSResultsFighterStatus[1], gNdsVSResultsFighterMotion[1]',
            'printf "VS_RESULTS_DISPLAY=%u,%u,%u,%u,%u\n", gNdsOriginalSpritePreviewReady, gNdsOriginalSpritePreviewCommitCount, gNdsOriginalSpritePreviewDisplayWidth, gNdsOriginalSpritePreviewDisplayHeight, gNdsBattleTextHudClearCount'
        )
        if ($OneMinuteMatchProof) {
            $lifecycleCommands += 'printf "MATCH_SAFETY=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsMemoryLedgerRelocStaleFiles, gNdsMemoryLedgerRelocStaleBytes, gNdsFighterNaturalMotionUnsafeCount, gNdsFighterNaturalMotionFigatreeTableInvalidCount, gNdsFighterNaturalMotionFigatreeAnimInvalidCount, gNdsAObjEvent32NormalizeFailCount, gNdsMObjSubAttachFailCount, gNdsStagePupupuExternalFixupFailCount, gNdsFighterMarioFoxExternalFixupFailCount, gNdsAudioBgmOpenFailCount, gNdsAudioBgmReadFailCount, gNdsAudioBgmUnsafeWriteCount, gNdsAudioBgmOverrunCount, gNdsStageCollisionLoopNoGeometryCount, gNdsStageCollisionLoopOutOfRangeLineCount, gNdsStageCollisionLoopBadVertexCount, gNdsFighterDisplayContractBoundsFailCount'
            $lifecycleCommands += 'printf "DAMAGE_FLOOR=%u,%u,%u,%u,%u,%u,%u,%u,%d,%d,%d,%d,%d,%d,%#x,%#x,%u,%u,%u\n", gNdsCollisionRuntimeDiagnostics.damage_check_calls, gNdsCollisionRuntimeDiagnostics.damage_proc_calls, gNdsCollisionRuntimeDiagnostics.damage_floor_tests, gNdsCollisionRuntimeDiagnostics.damage_floor_hits, gNdsCollisionRuntimeDiagnostics.damage_floor_landings, gNdsCollisionRuntimeDiagnostics.damage_floor_edge_deferred, gNdsCollisionRuntimeDiagnostics.damage_results, gNdsCollisionRuntimeDiagnostics.damage_invalid, gNdsCollisionRuntimeDiagnostics.damage_last_line, gNdsCollisionRuntimeDiagnostics.damage_last_status, gNdsCollisionRuntimeDiagnostics.damage_last_root_y_before_milli, gNdsCollisionRuntimeDiagnostics.damage_last_root_y_after_milli, gNdsCollisionRuntimeDiagnostics.damage_last_pos_diff_y_milli, gNdsCollisionRuntimeDiagnostics.damage_last_angle_y_milli, gNdsCollisionRuntimeDiagnostics.damage_last_mask_curr, gNdsCollisionRuntimeDiagnostics.damage_last_mask_stat, gNdsCollisionRuntimeDiagnostics.floor_flat_ascending_accepts, gNdsCollisionRuntimeDiagnostics.floor_adj_ambiguous, (gNdsCollisionRuntimeDiagnostics.damage_last_status >= nFTCommonStatusDamageStart && gNdsCollisionRuntimeDiagnostics.damage_last_status <= nFTCommonStatusDamageFall)'
        }
        if ($Task20StackProfileMode -eq 1) {
            $lifecycleCommands += 'printf "TASK20_STACK=%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsTask20GameplayStackBase, gNdsTask20GameplayStackSize, gNdsTask20GameplayStackHighWater, gNdsTask20MainStackBottom, gNdsTask20MainStackPoisonStart, gNdsTask20MainStackTop, gNdsTask20MainStackHighWater, gNdsTask20SampleCount'
            $lifecycleCommands += Get-Task20CoroutineCensusCommands
        }
        $gdbCommands = @($beforeDetach + $lifecycleCommands + $afterDetach)
    }
    if ($ImportBattleShipIFCommon) {
        $beforeDetach = $gdbCommands[0..($gdbCommands.Count - 3)]
        $afterDetach = $gdbCommands[($gdbCommands.Count - 2)..($gdbCommands.Count - 1)]
        $hudCommands = @(
            'printf "IFHUD=%u,%#x,%u,%u,%u,%u,%u,%u,%#x,%#x,%u,%u,%u,%u,%u,%u\n", gNdsIFCommonHUDRecordCount, gNdsIFCommonHUDObjectMask, gNdsIFCommonHUDP0DamageCurrent, gNdsIFCommonHUDP1DamageCurrent, gNdsIFCommonHUDP0DamageMax, gNdsIFCommonHUDP1DamageMax, gNdsIFCommonHUDP0DigitCount, gNdsIFCommonHUDP1DigitCount, gNdsIFCommonHUDP0Digits, gNdsIFCommonHUDP1Digits, gNdsIFCommonHUDP0StockCurrent, gNdsIFCommonHUDP1StockCurrent, gNdsIFCommonHUDP0StockMin, gNdsIFCommonHUDP1StockMin, gNdsIFCommonHUDP0StockMax, gNdsIFCommonHUDP1StockMax',
            'printf "IFHUD_LOWER=%#x,%#x,%#x,%#x,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsIFCommonHUDActivePlayerMask, gNdsIFCommonHUDShowDamageMask, gNdsIFCommonHUDSingleStockMask, gNdsIFCommonHUDCPUPlayerMask, gNdsIFCommonHUDP0FighterKind, gNdsIFCommonHUDP1FighterKind, gNdsIFCommonHUDP0Level, gNdsIFCommonHUDP1Level, gNdsIFCommonHUDP0LowerStock, gNdsIFCommonHUDP1LowerStock, gNdsIFCommonHUDTimeRemain, gNdsIFCommonHUDTimerLimit, gNdsIFCommonHUDTimerStarted, gNdsIFCommonHUDGameStatus, gNdsIFCommonHUDLowerRouteCount, gNdsIFCommonHUDLowerTimerRouteCount',
            'printf "IFHUD_ROUTE=%u,%#x,%u,%u\n", gNdsIFCommonHUDLowerTextMode, gNdsIFCommonHUDLowerRouteMask, gNdsIFCommonHUDLowerStockRouteCount, gNdsIFCommonHUDTopGenericPassCount'
        )
        $gdbCommands = @($beforeDetach + $hudCommands + $afterDetach)
    }
    if ($ImportBattleShipMarioFireball -or $ImportBattleShipFoxBlaster) {
        $beforeDetach = $gdbCommands[0..($gdbCommands.Count - 3)]
        $afterDetach = $gdbCommands[($gdbCommands.Count - 2)..($gdbCommands.Count - 1)]
        $projectileCommands = @(
            'printf "PROJECTILE=%#x,%#x,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%#x,%#x,%u,%u,%#x\n", gNdsFighterProjectileProofResult, gNdsFighterProjectileProofMask, gNdsFighterProjectileProofActorSlot, gNdsFighterProjectileProofActorKind, gNdsFighterProjectileProofBPressFrames, gNdsFighterProjectileProofSpecialStatusFrames, gNdsFighterProjectileProofSpecialMotion, gNdsFighterProjectileProofAccessoryFrames, gNdsFighterProjectileProofFlag0Frames, gNdsFighterProjectileProofSpawnCallCount, gNdsFighterProjectileProofSpawnSuccessCount, gNdsFighterProjectileProofUpdateDestroyCount, gNdsFighterProjectileProofMapDestroyCount, gNdsFighterProjectileProofHitDestroyCount, gNdsFighterProjectileProofWeaponFrames, gNdsFighterProjectileProofWeaponCountMax, gNdsFighterProjectileProofKindMask, gNdsFighterProjectileProofAttackStateMask, gNdsFighterProjectileProofDamageMax, gNdsFighterProjectileProofLifetimeMax, gNdsFighterProjectileProofMapMask'
        )
        $gdbCommands = @($beforeDetach + $projectileCommands + $afterDetach)
    }
    if ($ImportBattleShipFoxReflector) {
        $beforeDetach = $gdbCommands[0..($gdbCommands.Count - 3)]
        $afterDetach = $gdbCommands[($gdbCommands.Count - 2)..($gdbCommands.Count - 1)]
        $reflectorCommands = @(
            'printf "REFLECTOR=%#x,%#x,%u,%u,%u,%u,%u,%u,%u,%d,%u,%u,%u,%d,%d,%u,%u,%u,%u,%u,%u,%u,%d,%d,%u,%u\n", gNdsFighterReflectorProofResult, gNdsFighterReflectorProofMask, gNdsFighterReflectorProofFoxSlot, gNdsFighterReflectorProofProjectileSlot, gNdsFighterReflectorProofDownBPressFrames, gNdsFighterReflectorProofStartFrames, gNdsFighterReflectorProofLoopFrames, gNdsFighterReflectorProofHitFrames, gNdsFighterReflectorProofIsReflectFrames, gNdsFighterReflectorProofReflectLRBeforeHit, gNdsFighterReflectorProofReflectLRClearFrames, gNdsFighterReflectorProofHitSetCallCount, gNdsFighterReflectorProofFireballProcCount, gNdsFighterReflectorProofFireballVelXBefore, gNdsFighterReflectorProofFireballVelXAfter, gNdsFighterReflectorProofFireballOwnerKind, gNdsFighterReflectorProofFireballCanReflect, gNdsFighterReflectorProofFireballCanAbsorb, gNdsFighterReflectorProofFireballCanShield, gNdsFighterReflectorProofFireballAttackCount, gNdsFighterReflectorProofFireballDamage, gNdsFighterReflectorProofFireballSizeMilli, gNdsFighterReflectorProofFireballDXMilli, gNdsFighterReflectorProofFireballDYMilli, gNdsFighterReflectorProofSpecialSizeMilli, gNdsFighterReflectorProofSpecialResist'
        )
        $gdbCommands = @($beforeDetach + $reflectorCommands + $afterDetach)
    }
    if ($ImportBattleShipMarioSpecialHi -or $ImportBattleShipMarioSpecialLw -or $ImportBattleShipFoxSpecialHi) {
        $beforeDetach = $gdbCommands[0..($gdbCommands.Count - 3)]
        $afterDetach = $gdbCommands[($gdbCommands.Count - 2)..($gdbCommands.Count - 1)]
        $specialsCommands = @(
            'printf "SPECIALS=%#x,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%d,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%d\n", gNdsFighterSpecialsProofMask, gNdsFighterSpecialsProofPhase, gNdsFighterSpecialsProofPhaseFrames, gNdsFighterSpecialsMarioSlot, gNdsFighterSpecialsFoxSlot, gNdsFighterSpecialsMarioHiPressFrames, gNdsFighterSpecialsMarioHiFrames, gNdsFighterSpecialsMarioAirHiFrames, gNdsFighterSpecialsMarioFallSpecialFrames, gNdsFighterSpecialsMarioLandingFallSpecialFrames, gNdsFighterSpecialsMarioHiWaitFrames, gNdsFighterSpecialsMarioHiRootYMilli, gNdsFighterSpecialsMarioLwPressFrames, gNdsFighterSpecialsMarioLwFrames, gNdsFighterSpecialsMarioAirLwFrames, gNdsFighterSpecialsMarioLwDustEffectCount, gNdsFighterSpecialsMarioLwWaitFrames, gNdsFighterSpecialsFoxHiPressFrames, gNdsFighterSpecialsFoxHiStartFrames, gNdsFighterSpecialsFoxHiHoldFrames, gNdsFighterSpecialsFoxHiTravelFrames, gNdsFighterSpecialsFoxHiEndFrames, gNdsFighterSpecialsFoxHiBoundFrames, gNdsFighterSpecialsFoxHiWaitFrames, gNdsFighterSpecialsFoxHiRootYMilli'
        )
        $gdbCommands = @($beforeDetach + $specialsCommands + $afterDetach)
    }
    if ($ImportBattleShipAudioAssets) {
        $beforeDetach = $gdbCommands[0..($gdbCommands.Count - 3)]
        $afterDetach = $gdbCommands[($gdbCommands.Count - 2)..($gdbCommands.Count - 1)]
        $audioCommands = @(
            'printf "AUDIO_ASSET=%#x,%#x,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsAudioAssetResult, gNdsAudioAssetMask, gNdsAudioAssetOpenCount, gNdsAudioAssetOpenFailCount, gNdsAudioAssetFormatFailCount, gNdsAudioAssetShortReadCount, gNdsAudioAssetRawBytes, gNdsAudioAssetResidentBytes, gNdsAudioAssetScratchMaxBytes, gNdsAudioAssetSeqCount, gNdsAudioAssetSeqFirstOffset, gNdsAudioAssetSeqFirstLength, gNdsAudioAssetSeqMaxLength, gNdsAudioAssetBank1BankCount, gNdsAudioAssetBank1InstrumentCount, gNdsAudioAssetBank1WaveCount, gNdsAudioAssetBank1SampleRate, gNdsAudioAssetBank2BankCount, gNdsAudioAssetBank2InstrumentCount, gNdsAudioAssetBank2WaveCount, gNdsAudioAssetBank2SampleRate, gNdsAudioAssetFgmUnkCount, gNdsAudioAssetFgmTableCount, gNdsAudioAssetFgmUcodeCount',
            'printf "AUDIO_FGM_KO=%#x,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsAudioFgmKoPlayMask, gNdsAudioFgmKoPlayCounts[0], gNdsAudioFgmKoPlayCounts[1], gNdsAudioFgmKoPlayCounts[2], gNdsAudioFgmKoPlayCounts[3], gNdsAudioFgmKoPlayCounts[4], gNdsAudioFgmKoTraceCount, gNdsAudioFgmKoTrace[0], gNdsAudioFgmKoTrace[1], gNdsAudioFgmKoTrace[2], gNdsAudioFgmIncludedLookupFailCount, gNdsAudioFgmPlayFailCount, gNdsAudioFgmPoolExhaustCount, gNdsAudioFgmGenerationMismatchCount',
            'printf "AUDIO_FGM_MISS=%u,%u,%u:%u,%u:%u,%u:%u,%u:%u,%u:%u,%u:%u,%u:%u,%u:%u,%u:%u,%u:%u,%u:%u,%u:%u,%u:%u,%u:%u,%u:%u,%u:%u\n", gNdsAudioFgmMissRingCount, gNdsAudioFgmMissRingNext, gNdsAudioFgmMissRingIDs[0], gNdsAudioFgmMissRingCounts[0], gNdsAudioFgmMissRingIDs[1], gNdsAudioFgmMissRingCounts[1], gNdsAudioFgmMissRingIDs[2], gNdsAudioFgmMissRingCounts[2], gNdsAudioFgmMissRingIDs[3], gNdsAudioFgmMissRingCounts[3], gNdsAudioFgmMissRingIDs[4], gNdsAudioFgmMissRingCounts[4], gNdsAudioFgmMissRingIDs[5], gNdsAudioFgmMissRingCounts[5], gNdsAudioFgmMissRingIDs[6], gNdsAudioFgmMissRingCounts[6], gNdsAudioFgmMissRingIDs[7], gNdsAudioFgmMissRingCounts[7], gNdsAudioFgmMissRingIDs[8], gNdsAudioFgmMissRingCounts[8], gNdsAudioFgmMissRingIDs[9], gNdsAudioFgmMissRingCounts[9], gNdsAudioFgmMissRingIDs[10], gNdsAudioFgmMissRingCounts[10], gNdsAudioFgmMissRingIDs[11], gNdsAudioFgmMissRingCounts[11], gNdsAudioFgmMissRingIDs[12], gNdsAudioFgmMissRingCounts[12], gNdsAudioFgmMissRingIDs[13], gNdsAudioFgmMissRingCounts[13], gNdsAudioFgmMissRingIDs[14], gNdsAudioFgmMissRingCounts[14], gNdsAudioFgmMissRingIDs[15], gNdsAudioFgmMissRingCounts[15]'
        )
        $gdbCommands = @($beforeDetach + $audioCommands + $afterDetach)
    }
    if ($ImportBattleShipAudioBGM) {
        $beforeDetach = $gdbCommands[0..($gdbCommands.Count - 3)]
        $afterDetach = $gdbCommands[($gdbCommands.Count - 2)..($gdbCommands.Count - 1)]
        $audioBgmCommands = @(
            'printf "AUDIO_BGM=%#x,%#x,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsAudioBgmResult, gNdsAudioBgmMask, gNdsAudioBgmPlaying, gNdsAudioBgmTrackID, gNdsAudioBgmVolume, gNdsAudioBgmPlayCalls, gNdsAudioBgmStopCalls, gNdsAudioBgmCheckCalls, gNdsAudioBgmSetVolumeCalls, gNdsAudioBgmOpenFailCount, gNdsAudioBgmReadFailCount, gNdsAudioBgmUnsupportedTrackCount, gNdsAudioBgmReadBytes, gNdsAudioBgmResidentBytes, gNdsAudioBgmChunkBytes, gNdsAudioBgmChunkPlayCount, gNdsAudioBgmStoppedOnTeardown, gNdsAudioBgmElapsedFrames, gNdsAudioBgmStreamedBytes, gNdsAudioBgmStreamBytesPerSecond, gNdsAudioBgmExpectedBytesPerSecond, gNdsAudioBgmLoopCount, gNdsAudioBgmRefillCount, gNdsAudioBgmPlaybackPositionBytes, gNdsAudioBgmWritePositionBytes, gNdsAudioBgmPlaybackHalf, gNdsAudioBgmWriteHalf, gNdsAudioBgmUnsafeWriteCount, gNdsAudioBgmTimerTicks, gNdsAudioBgmPlaybackBytes, gNdsAudioBgmPlaybackLoopCount, gNdsAudioBgmOverrunCount',
            'printf "AUDIO_BGM_ADPCM=%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsAudioBgmHeaderFailCount, gNdsAudioBgmPacketFailCount, gNdsAudioBgmPreparedCount, gNdsAudioBgmSeamStartCount, gNdsAudioBgmSeamMissCount, gNdsAudioBgmTimerEventDropCount, gNdsAudioBgmWorkerWakeCount, gNdsAudioBgmErrorStopCount, gNdsAudioBgmErrorCleanupFailCount'
        )
        $gdbCommands = @($beforeDetach + $audioBgmCommands + $afterDetach)
    }
    try {
        $gdbStdout = (Invoke-GdbMarkerScript -Gdb $Gdb -Elf $elf -Root $root `
            -Commands $gdbCommands -ScriptName $scriptName `
            -TimeoutSeconds $gdbCaptureTimeoutSeconds).Stdout
    } catch {
        if ($Task34StageStreamCensus -and
            $_.Exception.Message.StartsWith('GDB marker capture timed out')) {
            $timeoutEvidence = $_.Exception.Message
            try {
                $autopsy = Invoke-GdbMarkerScript -Gdb $Gdb -Elf $elf `
                    -Root $root -ScriptName ($scriptName + '.autopsy') `
                    -TimeoutSeconds 15 -Commands @(
                        'set pagination off',
                        "target remote 127.0.0.1:$($verifierContext.GdbPort)",
                        'printf "TASK34_STALL_ARENA=%u,%u\n", gNdsTaskmanArenaChosenSize, gNdsTaskmanArenaAllocFailCount',
                        'info registers pc lr sp',
                        'x/i $pc',
                        'bt',
                        'detach',
                        'quit'
                    )
                $autopsyEvidence = "$($autopsy.Stdout)`n$($autopsy.Stderr)"
            } catch {
                $autopsyEvidence = "AUTOPSY_ATTACH_FAILED: $($_.Exception.Message)"
            }
            throw "$timeoutEvidence`nTask 34 timeout autopsy:`n$autopsyEvidence"
        }
        throw
    }
    $aobj32 = [regex]::Match($gdbStdout, 'AOBJ32=([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $mobjAttach = [regex]::Match($gdbStdout, 'MOBJ_ATTACH=([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $harn = [regex]::Match($gdbStdout, 'HARN=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0)')
    $scene = [regex]::Match($gdbStdout, 'SCENE=([0-9]+),([0-9]+),([0-9]+)')
    $buildMode = [regex]::Match($gdbStdout, 'BUILD_MODE=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $vs = [regex]::Match($gdbStdout, 'VS_TRANS=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $pv = [regex]::Match($gdbStdout, 'PV_TRANS=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $maps = [regex]::Match($gdbStdout, 'MAPS_TRANS=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $prev = [regex]::Match($gdbStdout, 'PREV_LOOP=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $loop = [regex]::Match($gdbStdout, 'GCRUNALL_LOOP=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+)')
    $taskman = [regex]::Match($gdbStdout, 'GCRUNALL_TASKMAN=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $natural = [regex]::Match($gdbStdout, 'NAT_MOTION=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+)')
    $naturalFig = [regex]::Match($gdbStdout, 'NAT_FIG=([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $naturalWait = [regex]::Match($gdbStdout, 'NAT_WAIT=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $naturalWalk = [regex]::Match($gdbStdout, 'NAT_WALK=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $naturalChain = [regex]::Match($gdbStdout, 'NAT_CHAIN=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $naturalAttack = [regex]::Match($gdbStdout, 'NAT_ATTACK=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $naturalMoveset = [regex]::Match($gdbStdout, 'NAT_MOVESET=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),([0-9]+),([0-9]+)')
    $naturalHitlag = [regex]::Match($gdbStdout, 'NAT_HITLAG=([0-9]+),([0-9]+)')
    $naturalGuard = [regex]::Match($gdbStdout, 'NAT_GUARD=([0-9]+),([0-9]+),([0-9]+)')
    $battlePlayableKO = [regex]::Match($gdbStdout, 'BPLAY_KO=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $battlePlayableStatus = [regex]::Match($gdbStdout, 'BPLAY_STATUS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $battlePlayablePacing = [regex]::Match($gdbStdout, 'BPLAY_PACE=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $task25rPacing = if ($Task25RPacingTrace) {
        @(Get-UnsignedMarkerMatches -Text $gdbStdout `
            -Name 'TASK25R_PACE' -FieldCount 14)
    } else { @() }
    $battlePlayableFpsHud = [regex]::Match($gdbStdout, 'FPS_HUD=([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $battleTextHud = [regex]::Match($gdbStdout, 'BATTLE_TEXT_HUD=([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $battlePlayableStart = [regex]::Match($gdbStdout, 'BPLAY_START=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $battlePlayableStartValues = Get-Ints $battlePlayableStart
    $preGoState =
        $battlePlayableStart.Success -and
        $battlePlayableStartValues[0] -eq 0 -and
        $battlePlayableStartValues[1] -eq 1 -and
        $battlePlayableStartValues[2] -eq 3600 -and
        $battlePlayableStartValues[3] -eq 0 -and
        $battlePlayableStartValues[4] -eq 0 -and
        $battlePlayableStartValues[5] -eq 1 -and
        $battlePlayableStartValues[6] -eq 1
    $memoryArena = [regex]::Match($gdbStdout, 'MEMARENA=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $memoryReloc = [regex]::Match($gdbStdout, 'MEMRELOC=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $memoryEvict = [regex]::Match($gdbStdout, 'MEMEVICT=([0-9]+),([0-9]+)')
    $vsbMemoryArena = [regex]::Match($gdbStdout, 'VSB_MEMARENA=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $vsbMemoryReloc = [regex]::Match($gdbStdout, 'VSB_MEMRELOC=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $vsbMemoryEvict = [regex]::Match($gdbStdout, 'VSB_MEMEVICT=([0-9]+),([0-9]+)')
    $computerConfig = [regex]::Match($gdbStdout, 'CPU_CONFIG=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+)')
    $computerAI = [regex]::Match($gdbStdout, 'CPU_AI=([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+)')
    $battleLifecycle = [regex]::Match($gdbStdout, 'VSB_END=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $vsResults = [regex]::Match($gdbStdout, 'VS_RESULTS=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $vsResultsFighters = [regex]::Match($gdbStdout, 'VS_RESULTS_FIGHTERS=([0-9]+),(0x[0-9a-fA-F]+|0),(-?[0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(-?[0-9]+)')
    $vsResultsDisplay = [regex]::Match($gdbStdout, 'VS_RESULTS_DISPLAY=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $matchStart = [regex]::Match($gdbStdout, 'MATCH_START=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $task9StateSummary = @(Get-UnsignedMarkerMatches -Text $gdbStdout `
        -Name 'TASK9_STATE_SUMMARY' -FieldCount 3)
    $task9StateRecords = @(Get-UnsignedMarkerMatches -Text $gdbStdout `
        -Name 'TASK9_STATE' -FieldCount 6)
    $task20StackRecords = @(Get-UnsignedMarkerMatches -Text $gdbStdout `
        -Name 'TASK20_STACK' -FieldCount 8)
    $task20CensusSummaryRecords = @(Get-UnsignedMarkerMatches -Text $gdbStdout `
        -Name 'TASK31_CENSUS_SUMMARY' -FieldCount 6)
    $task20CensusRecords = @(Get-UnsignedMarkerMatches -Text $gdbStdout `
        -Name 'TASK31_CENSUS' -FieldCount 8)
    $task20StackRows = @()
    $task20CensusRows = @()
    $task20StackEvidence = $null
    $task20StackSummary = ''
    if ($Task20StackProfileMode -eq 1) {
        Assert-Condition ($task20StackRecords.Count -gt 0) `
            'Task 20 stack profile emitted no watermark record.' $gdbStdout
        $task20StackRows = @($task20StackRecords | ForEach-Object {
            , @(Get-Ints $_)
        })
        $task20StackLast = $task20StackRows[-1]
        Assert-Condition ($task20CensusSummaryRecords.Count -gt 0) `
            'Task 31 coroutine census emitted no summary record.' $gdbStdout
        $task20CensusSummary = Get-Ints $task20CensusSummaryRecords[-1]
        $task20CensusCount = [int]$task20CensusSummary[0]
        Assert-Condition ($task20CensusCount -gt 0 -and
            $task20CensusCount -le 64 -and
            $task20CensusSummary[1] -eq 0 -and
            $task20CensusRecords.Count -ge $task20CensusCount) `
            'Task 31 coroutine census was empty, overflowed, or omitted rows.' `
            $gdbStdout
        $task20CensusRows = @($task20CensusRecords |
            Select-Object -Last $task20CensusCount |
            ForEach-Object { , @(Get-Ints $_) })
        for ($task20CensusIndex = 0;
             $task20CensusIndex -lt $task20CensusRows.Count;
             $task20CensusIndex++) {
            $task20CensusRow = $task20CensusRows[$task20CensusIndex]
            $task20ExpectedStackSize = if ($task20CensusRow[1] -lt 100) {
                16 * 1024
            } else {
                4 * 1024
            }
            Assert-Condition ($task20CensusRow[0] -eq $task20CensusIndex -and
                $task20CensusRow[2] -eq $task20ExpectedStackSize -and
                $task20CensusRow[3] -eq $task20ExpectedStackSize -and
                $task20CensusRow[4] -ne 0 -and
                (($task20CensusRow[4] -band 7) -eq 0) -and
                $task20CensusRow[5] -ne 0 -and
                ($task20CensusRow[6] -eq 1 -or
                 $task20CensusRow[6] -eq 2) -and
                $task20CensusRow[7] -le $task20CensusRow[3]) `
                'Task 31 coroutine census row lost owner sizing, address, state, or watermark integrity.' `
                $gdbStdout
        }
        $task20CensusLiveRows = @($task20CensusRows |
            Where-Object { $_[6] -eq 1 })
        $task20CensusLargeLiveRows = @($task20CensusLiveRows |
            Where-Object { $_[3] -ge (16 * 1024) })
        Assert-Condition ($task20CensusSummary[2] -eq
                $task20CensusLiveRows.Count -and
            $task20CensusSummary[3] -ge $task20CensusSummary[2] -and
            $task20CensusSummary[4] -eq
                $task20CensusLargeLiveRows.Count -and
            $task20CensusSummary[5] -ge $task20CensusSummary[4] -and
            $task20CensusSummary[5] -le $task20CensusSummary[3]) `
            'Task 31 coroutine census live/peak conservation failed.' $gdbStdout
        $task20SampleCountValid = if ($MatchLifecycleProof) {
            $task20StackLast[7] -ge 1
        } else {
            @($task20StackRows | Where-Object { $_[7] -ne 1 }).Count -eq 0
        }
        $task20GameplayCapacity = [int64]$task20StackLast[1]
        $task20GameplayHighWater = [int64]$task20StackLast[2]
        $task20DtcmGap = [int64]$task20StackLast[5] -
            [int64]$task20StackLast[3]
        $task20MainHighWater = [int64]$task20StackLast[6]
        $task20GameplayCensusRows = @($task20CensusRows |
            Where-Object { $_[4] -eq $task20StackLast[0] })
        Assert-Condition ($task20GameplayCensusRows.Count -eq 1) `
            'Task 31 census did not identify exactly one owner for the measured gameplay stack.' `
            $gdbStdout
        $task20GameplayOwnerId = [int64]$task20GameplayCensusRows[0][1]
        Assert-Condition ($task20StackLast[0] -ne 0 -and
            (($task20StackLast[0] -band 7) -eq 0) -and
            $task20GameplayCapacity -eq 16384 -and
            $task20GameplayHighWater -gt 0 -and
            $task20GameplayHighWater -le ($task20GameplayCapacity - 64) -and
            $task20StackLast[3] -lt $task20StackLast[4] -and
            $task20StackLast[4] -lt $task20StackLast[5] -and
            (($task20StackLast[3] -band 3) -eq 0) -and
            (($task20StackLast[5] -band 3) -eq 0) -and
            $task20MainHighWater -gt 0 -and
            $task20MainHighWater -le ($task20DtcmGap - 64) -and
            $task20SampleCountValid) `
            'Task 20 stack profile lost its exact capacity, alignment, 64-byte guards, or watermark.' `
            $gdbStdout
        $task20RawNeed = $task20GameplayHighWater + 64 +
            $task20MainHighWater
        $task20MarginNeed = $task20GameplayHighWater + 1024 + 64 +
            $task20MainHighWater + 1024
        $task20RawFit = if ($task20RawNeed -le $task20DtcmGap) {
            'FIT'
        } else {
            'NO_FIT'
        }
        $task20StackEvidence = [ordered]@{
            scope = if ($MatchLifecycleProof) {
                'startup/final request-gated'
            } else {
                'startup-only'
            }
            recordCount = $task20StackRows.Count
            latest = [ordered]@{
                gameplayBase = [int64]$task20StackLast[0]
                gameplayCapacity = $task20GameplayCapacity
                gameplayHighWater = $task20GameplayHighWater
                mainBottom = [int64]$task20StackLast[3]
                mainPoisonStart = [int64]$task20StackLast[4]
                mainTop = [int64]$task20StackLast[5]
                mainHighWater = $task20MainHighWater
                sampleCount = [int64]$task20StackLast[7]
            }
            fit = [ordered]@{
                dtcmGap = $task20DtcmGap
                guardBytes = 64
                rawNeed = $task20RawNeed
                rawDelta = $task20DtcmGap - $task20RawNeed
                rawFits = $task20RawNeed -le $task20DtcmGap
                marginNeed = $task20MarginNeed
                marginDelta = $task20DtcmGap - $task20MarginNeed
                marginFits = $task20MarginNeed -le $task20DtcmGap
            }
            census = [ordered]@{
                count = $task20CensusCount
                overflowCount = [int64]$task20CensusSummary[1]
                liveCount = [int64]$task20CensusSummary[2]
                peakLiveCount = [int64]$task20CensusSummary[3]
                largeLiveCount = [int64]$task20CensusSummary[4]
                peakLargeLiveCount = [int64]$task20CensusSummary[5]
                gameplayOwnerId = $task20GameplayOwnerId
                rows = @($task20CensusRows | ForEach-Object {
                    [ordered]@{
                        index = [int64]$_[0]
                        ownerId = [int64]$_[1]
                        requestedStackSize = [int64]$_[2]
                        actualStackSize = [int64]$_[3]
                        stackBase = [int64]$_[4]
                        coroutineAddress = [int64]$_[5]
                        state = [int64]$_[6]
                        highWater = [int64]$_[7]
                    }
                })
            }
        }
        $task20StackSummary =
            "Task20 startup/final stack: records=$($task20StackRecords.Count) samples=$($task20StackLast[7]) gameplay=id$task20GameplayOwnerId/$($task20StackLast[0])/$task20GameplayCapacity/hwm$task20GameplayHighWater mainPostInit=$($task20StackLast[3])..$($task20StackLast[4])..$($task20StackLast[5])/hwm$task20MainHighWater dtcmGap=$task20DtcmGap fit=$task20RawFit/rawNeed$task20RawNeed/delta$($task20DtcmGap-$task20RawNeed) marginNeed$task20MarginNeed/delta$($task20DtcmGap-$task20MarginNeed) guards=64 census=$task20CensusCount/live$($task20CensusSummary[2])/peak$($task20CensusSummary[3])/large$($task20CensusSummary[4])/largePeak$($task20CensusSummary[5])/overflow$($task20CensusSummary[1])"
    }
    $matchSafety = [regex]::Match($gdbStdout, 'MATCH_SAFETY=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $damageFloor = [regex]::Match($gdbStdout, 'DAMAGE_FLOOR=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+)')
    $run = [regex]::Match($gdbStdout, 'GCRUNALL_RUN=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $process = [regex]::Match($gdbStdout, 'GCRUNALL_PROCESS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $input = [regex]::Match($gdbStdout, 'GCRUNALL_INPUT=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+)')
    $status = [regex]::Match($gdbStdout, 'GCRUNALL_STATUS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $visits = [regex]::Match($gdbStdout, 'GCRUNALL_VISITS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $calls = [regex]::Match($gdbStdout, 'GCRUNALL_CALLS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $draw = [regex]::Match($gdbStdout, 'GCRUNALL_DRAW=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $screen = [regex]::Match($gdbStdout, 'GCRUNALL_SCREEN=(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+)')
    $move = [regex]::Match($gdbStdout, 'GCRUNALL_MOVE=(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $trans = [regex]::Match($gdbStdout, 'GCRUNALL_TRANS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $safe = [regex]::Match($gdbStdout, 'GCRUNALL_SAFE=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $platform = [regex]::Match($gdbStdout, 'PLATFORM_DL_PREVIEW=([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $platformHw = [regex]::Match($gdbStdout, 'PLATFORM_HW=([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $rendererProfileMarker = [regex]::Match($gdbStdout, 'RENDER_PROFILE_LEVEL=([0-9]+)')
    $rendererM2DetailedLedgerMarker = [regex]::Match(
        $gdbStdout, 'RENDER_M2_DETAILED_LEDGER=([0-9]+)')
    if ($RendererBenchmarkSamples -gt 0) {
        Assert-Condition (
            $rendererM2DetailedLedgerMarker.Success -and
            [int]$rendererM2DetailedLedgerMarker.Groups[1].Value -eq
                [int]$RendererM2DetailedLedger.IsPresent
        ) 'Runtime ELF M2 detailed-ledger identity does not match the requested benchmark configuration.' $gdbStdout
    }
    $stageHardware = [regex]::Match($gdbStdout, 'STAGE_GCDRAWALL_HW=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+)')
    $stageCarry = [regex]::Match($gdbStdout, 'RENDER_STAGE_CARRY=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageHardwareFighter = [regex]::Match($gdbStdout, 'STAGE_GCDRAWALL_HW_FTR=([0-9]+),([0-9]+)')
    $weaponRenderer = [regex]::Match($gdbStdout, 'WEAPON_RENDER=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $weaponFrame = [regex]::Match($gdbStdout, 'WEAPON_FRAME=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $fighterDisplayContract = [regex]::Match($gdbStdout, 'FTR_DISPLAY_CONTRACT=([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $renderProfile = [regex]::Match($gdbStdout, 'RENDER_PROFILE=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $rendererBenchmark = [regex]::Matches($gdbStdout, 'RENDER_BENCH=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $rendererBenchmarkEvent = [regex]::Matches(
        $gdbStdout,
        'RENDER_EVENT=([0-9]+),([0-9]+),([0-9]+)')
    $coarseBenchmark = @()
    $ownerBenchmark = @()
    $gxBoundaryBenchmark = @()
    $stage0Benchmark = @()
    $sinkBenchmark = @()
    $warmBenchmark = @()
    $texturePhaseBenchmark = @()
    $fastRunBenchmark = @()
    $m3StageBenchmark = @()
    $task36HwBenchmark = @()
    $task36ReplayBenchmark = @()
    $m3GeneratedSegment0Benchmark = @()
    $m3GeneratedSegment0ShadowBenchmark = @()
    $m3GeneratedSegment0GxBenchmark = @()
    $m3GeneratedSegment0TraceSummary = @()
    $m3GeneratedSegment0TraceWords = @()
    $m3GeneratedSegment0TraceRuns = @()
    $m3GeneratedSegment0TraceSummaryValues = @()
    $m3GeneratedSegment0TraceWordValues = @()
    $m3GeneratedSegment0TraceRunValues = @()
    $m3Phase0Benchmark = @()
    $m3ResidualBenchmark = @()
    $m3PreparedBenchmark = @()
    $m3WhispyBenchmark = @()
    $g2StateBenchmark = @()
    $task29GxMetaBenchmark = @()
    $task29GxClassBenchmark = @()
    $task29GxOwnerBenchmark = @()
    $task34ArenaBoot = @()
    $task34StageMetaBenchmark = @()
    $task34StageEntryValues = [Collections.Generic.List[object]]::new()
    $task34StageWordValues = [Collections.Generic.List[object]]::new()
    $phase05Benchmark = @()
    $wallRunBenchmark = @()
    $m4WaterStillBenchmark = @()
    $m4StaticBenchmark = @()
    $m4FenceBenchmark = @()
    $m2Benchmark = @()
    $m2ShadeBenchmark = @()
    $rendererSemanticBenchmark = @()
    $task9FloatPairBenchmark = @()
    $task9FloatCostBenchmark = @()
    $task9FloatTimerBenchmark = @()
    $economyBenchmark = @()
    $screenSpaceCensusSummary = @()
    $screenSpaceCensusRows = @()
    $screenSpaceCensusStageOwners = @()
    if ($Task34StageStreamCensus) {
        $task34ArenaBootRows = @(Get-UnsignedMarkerMatches -Text $gdbStdout `
            -Name 'TASK34_ARENA_BOOT' -FieldCount 2)
        Assert-Condition ($task34ArenaBootRows.Count -eq 1) `
            'Task 34 standalone census did not publish one pre-capture arena row.' `
            $gdbStdout
        $task34ArenaBoot = Get-Ints $task34ArenaBootRows[0]
        Assert-Condition (
            $task34ArenaBoot[0] -eq 1376256 -and
            $task34ArenaBoot[1] -eq 0
        ) "Task 34 standalone census degraded the adaptive arena before capture (actual=$($task34ArenaBoot -join ','))." $gdbStdout
    }
    $m4FenceFinal = @(Get-UnsignedMarkerMatches -Text $gdbStdout `
        -Name 'M4_FENCE_FINAL' -FieldCount 24)
    $fastFinal = @(Get-UnsignedMarkerMatches -Text $gdbStdout `
        -Name 'FAST_FINAL' -FieldCount 9)
    $m4WaterStillFinal = @(Get-UnsignedMarkerMatches -Text $gdbStdout `
        -Name 'M4_WATER_STILL_FINAL' -FieldCount 3)
    if (($RendererProfileLevel -ge 1) -and ($RendererBenchmarkSamples -gt 0)) {
        $coarseBenchmark = @(Get-UnsignedMarkerMatches -Text $gdbStdout -Name 'COARSE_BENCH' -FieldCount 27)
        $gxBoundaryBenchmark = @(Get-UnsignedMarkerMatches -Text $gdbStdout -Name 'GX_BOUNDARY' -FieldCount 7)
        $stage0Benchmark = @(Get-UnsignedMarkerMatches -Text $gdbStdout -Name 'STAGE0_BENCH' -FieldCount 2)
        $texturePhaseBenchmark = @(Get-UnsignedMarkerMatches -Text $gdbStdout -Name 'TEXTURE_PHASE_BENCH' -FieldCount 15)
        $fastRunBenchmark = @(Get-UnsignedMarkerMatches -Text $gdbStdout -Name 'FAST_BENCH' -FieldCount 10)
        if ($RenderEconomyMode -eq 1) {
            $economyBenchmark = @(Get-UnsignedMarkerMatches -Text $gdbStdout -Name 'ECONOMY_BENCH' -FieldCount 5)
        }
        if ($RendererScreenSpaceCensusMode -eq 1) {
            $screenSpaceCensusSummary = @(Get-UnsignedMarkerMatches -Text $gdbStdout -Name 'SCREEN_CENSUS_SUMMARY' -FieldCount 2)
            $screenSpaceCensusRows = @(Get-UnsignedMarkerMatches -Text $gdbStdout -Name 'SCREEN_CENSUS_ROW' -FieldCount 9)
            $screenSpaceCensusStageOwners = @(Get-UnsignedMarkerMatches -Text $gdbStdout -Name 'SCREEN_CENSUS_STAGE_OWNER' -FieldCount 3)
        }
        if ($Task9FloatCensusMode -eq 1) {
            $task9FloatPairBenchmark = @(Get-UnsignedMarkerMatches -Text $gdbStdout -Name 'TASK9_FLOAT_PAIR' -FieldCount 37)
            $task9FloatCostBenchmark = @(Get-UnsignedMarkerMatches -Text $gdbStdout -Name 'TASK9_FLOAT_COST' -FieldCount 7)
            $task9FloatTimerBenchmark = @(Get-UnsignedMarkerMatches -Text $gdbStdout -Name 'TASK9_FLOAT_TIMER' -FieldCount 2)
        }
        if ($Task29GXCensus) {
            $task29GxMetaBenchmark = @(Get-UnsignedMarkerMatches `
                -Text $gdbStdout -Name 'TASK29_GX_META' -FieldCount 11)
            $task29GxClassBenchmark = @(Get-UnsignedMarkerMatches `
                -Text $gdbStdout -Name 'TASK29_GX_CLASS' -FieldCount 67)
            $task29GxOwnerBenchmark = @(Get-UnsignedMarkerMatches `
                -Text $gdbStdout -Name 'TASK29_GX_OWNER' -FieldCount 70)
        }
        if ($Task34StageStreamCensus) {
                $task34StageMetaBenchmark = @(Get-UnsignedMarkerMatches `
                    -Text $gdbStdout -Name 'TASK34_STAGE_META' -FieldCount 5)
                Assert-Condition (
                    $task34StageMetaBenchmark.Count -eq
                        $RendererBenchmarkSamples -and
                    $task34StageDumpFiles.Count -eq
                        $RendererBenchmarkSamples
                ) "Task 34 E1 did not publish one meta row and dump pair per sample." $gdbStdout
                for ($sampleIndex = 0;
                     $sampleIndex -lt $RendererBenchmarkSamples;
                     $sampleIndex++) {
                    $meta = Get-Ints $task34StageMetaBenchmark[$sampleIndex]
                    $dump = $task34StageDumpFiles[$sampleIndex]
                    $entryBytes = [IO.File]::ReadAllBytes($dump.Entries)
                    $wordBytes = [IO.File]::ReadAllBytes($dump.Words)
                    Assert-Condition (
                        $entryBytes.Length -eq (8 * $meta[1]) -and
                        $wordBytes.Length -eq (4 * $meta[2])
                    ) "Task 34 E1 binary dump size diverged at frame $($meta[0])." (
                        "entries=$($entryBytes.Length)/$((8 * $meta[1])) words=$($wordBytes.Length)/$((4 * $meta[2]))")
                    for ($entryIndex = 0;
                         $entryIndex -lt $meta[1]; $entryIndex++) {
                        $offset = 8 * $entryIndex
                        Assert-Condition ($entryBytes[$offset + 7] -eq 0) `
                            "Task 34 E1 binary entry has a nonzero reserved byte." `
                            "frame=$($meta[0]) entry=$entryIndex"
                        $task34StageEntryValues.Add(@(
                            $meta[0],
                            $entryIndex,
                            [BitConverter]::ToUInt16($entryBytes, $offset + 2),
                            $entryBytes[$offset + 4],
                            [BitConverter]::ToUInt16($entryBytes, $offset),
                            $entryBytes[$offset + 5],
                            $entryBytes[$offset + 6]))
                    }
                    for ($wordIndex = 0;
                         $wordIndex -lt $meta[2]; $wordIndex++) {
                        $task34StageWordValues.Add(@(
                            $meta[0],
                            $wordIndex,
                            [BitConverter]::ToUInt32(
                                $wordBytes, 4 * $wordIndex)))
                    }
                }
        }
        if (($RendererProfileLevel -eq 1) -and
            ($RendererFastRunMode -eq 9)) {
            $m3StageBenchmark = @(Get-UnsignedMarkerMatches -Text $gdbStdout -Name 'M3_STAGE' -FieldCount 22)
            if ($Task36HwComposeMode -gt 0) {
                $task36HwBenchmark = @(Get-UnsignedMarkerMatches -Text $gdbStdout -Name 'TASK36_HW' -FieldCount 12)
                if ($Task36HwComposeMode -eq 2) {
                    $task36ReplayBenchmark = @(Get-UnsignedMarkerMatches -Text $gdbStdout -Name 'TASK36_REPLAY' -FieldCount 19)
                }
            }
            if ($benchmarkMakeIdentity.NativeStageGeneratedSegment0Enable -eq 1) {
                $m3GeneratedSegment0Benchmark = @(Get-UnsignedMarkerMatches -Text $gdbStdout -Name 'M3_GEN0' -FieldCount 10)
                if ($RendererM3Phase0Profile) {
                    $m3GeneratedSegment0ShadowBenchmark = @(Get-UnsignedMarkerMatches -Text $gdbStdout -Name 'M3_GEN0_SHADOW' -FieldCount 15)
                }
            }
            if ($RendererM3Phase0Profile) {
                $m3Phase0Benchmark = @(Get-UnsignedMarkerMatches -Text $gdbStdout -Name 'M3_PHASE0' -FieldCount 19)
                $m3ResidualBenchmark = @(Get-UnsignedMarkerMatches -Text $gdbStdout -Name 'M3_RESIDUAL' -FieldCount 11)
                $m3PreparedBenchmark = @(Get-UnsignedMarkerMatches -Text $gdbStdout -Name 'M3_PREPARED' -FieldCount 15)
                $m3WhispyBenchmark = @(Get-UnsignedMarkerMatches -Text $gdbStdout -Name 'M3_WHISPY' -FieldCount 5)
                $g2StateBenchmark = @(Get-UnsignedMarkerMatches -Text $gdbStdout -Name 'G2_STATE' -FieldCount 7)
                $phase05Benchmark = @(Get-UnsignedMarkerMatches -Text $gdbStdout -Name 'PHASE05' -FieldCount 21)
                $wallRunBenchmark = @(Get-UnsignedMarkerMatches -Text $gdbStdout -Name 'WALL_RUNS' -FieldCount 17)
            }
        }
        if ($m4CandidateEvidence) {
            $m4WaterStillBenchmark = @(Get-UnsignedMarkerMatches -Text $gdbStdout -Name 'M4_WATER_STILL' -FieldCount 4)
        }
        $m4StaticBenchmark = @(Get-UnsignedMarkerMatches -Text $gdbStdout -Name 'M4_STATIC' -FieldCount 11)
        $m4FenceBenchmark = @(Get-UnsignedMarkerMatches -Text $gdbStdout -Name 'M4_FENCE' -FieldCount 13)
        if ($RendererM2DetailedLedger) {
            $m2Benchmark = @(Get-UnsignedMarkerMatches -Text $gdbStdout -Name 'M2_BENCH' -FieldCount 50)
            $m2ShadeBenchmark = @(Get-UnsignedMarkerMatches -Text $gdbStdout -Name 'M2_SHADE' -FieldCount 17)
        }
        if ($benchmarkMakeIdentity.RendererBenchmarkMode -eq 2) {
            $sinkBenchmark = @(Get-UnsignedMarkerMatches -Text $gdbStdout -Name 'SINK_BENCH' -FieldCount 8)
            if ($RendererFastRunMode -eq 9) {
                $m3GeneratedSegment0GxBenchmark = @(Get-UnsignedMarkerMatches -Text $gdbStdout -Name 'M3_GEN0_GX' -FieldCount 8)
                $m3GeneratedSegment0TraceSummary = @(Get-UnsignedMarkerMatches -Text $gdbStdout -Name 'M3_GEN0_TRACE_SUMMARY' -FieldCount 2)
                $m3GeneratedSegment0TraceWords = @(Get-UnsignedMarkerMatches -Text $gdbStdout -Name 'M3_GEN0_TRACE_WORD' -FieldCount 2)
                $m3GeneratedSegment0TraceRuns = @(Get-UnsignedMarkerMatches -Text $gdbStdout -Name 'M3_GEN0_TRACE_RUN' -FieldCount 12)
            }
        }
        if ($benchmarkMakeIdentity.RendererBenchmarkMode -eq 4) {
            $warmBenchmark = @(Get-UnsignedMarkerMatches -Text $gdbStdout -Name 'WARM_BENCH' -FieldCount 3)
        }
        if ($RendererProfileLevel -ge 2) {
            $ownerBenchmark = @(Get-UnsignedMarkerMatches -Text $gdbStdout -Name 'OWNER_BENCH' -FieldCount 37)
            $rendererSemanticBenchmark = @(Get-UnsignedMarkerMatches -Text $gdbStdout -Name 'RENDER_SEMANTIC' -FieldCount 38)
        }
    }
    $renderBatch = [regex]::Match($gdbStdout, 'RENDER_BATCH=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $renderTopology = [regex]::Match($gdbStdout, 'RENDER_TOPOLOGY=([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $renderCost = [regex]::Match($gdbStdout, 'RENDER_COST=([0-9]+),([0-9]+)')
    $renderCi4Lut = [regex]::Match($gdbStdout, 'RENDER_CI4LUT=([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $renderCi4Map = [regex]::Match($gdbStdout, 'RENDER_CI4MAP=([0-9]+),([0-9]+)')
    $renderTexHash = [regex]::Match($gdbStdout, 'RENDER_TEXHASH=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $wallpaperCache = [regex]::Match($gdbStdout, 'SOBJ_WALL_CACHE=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $wallpaperFinal = [regex]::Match($gdbStdout, 'SOBJ_WALL_FINAL=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $wallpaperOracle = [regex]::Match($gdbStdout, 'SOBJ_WALL_ORACLE=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $fastWallpaper = [regex]::Match($gdbStdout, 'FAST_WALLPAPER=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+)')
    $ifCommonOamFieldPattern = ((0..30 | ForEach-Object {
        if ($_ -eq 24) { '(0x[0-9a-fA-F]+|0)' }
        else { '([0-9]+)' }
    }) -join ',')
    $ifCommonOam = [regex]::Match(
        $gdbStdout, ([regex]::Escape('IFCOMMON_OAM=') +
                     $ifCommonOamFieldPattern))
    $vramBanks = [regex]::Match($gdbStdout, 'VRAM_BANKS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $sceneMipCache = [regex]::Match($gdbStdout, 'SCENE_MIP_CACHE=([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $sceneWallAffine = [regex]::Match($gdbStdout, 'SCENE_WALL_AFFINE=([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+)')
    $renderOracle = [regex]::Match($gdbStdout, 'RENDER_ORACLE=([0-9]+),([0-9]+),([0-9]+)')
    $renderMatrix = [regex]::Match($gdbStdout, 'RENDER_MATRIX=([0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+)')
    $renderAdapterCache = [regex]::Match($gdbStdout, 'RENDER_ADAPTER_CACHE=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $renderStageWorldCache = [regex]::Match($gdbStdout, 'RENDER_STAGE_WORLD_CACHE=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $renderAffineMatrix = [regex]::Match($gdbStdout, 'RENDER_AFFINE_MATRIX=([0-9]+),([0-9]+),([0-9]+)')
    $renderRawMatrix = [regex]::Match($gdbStdout, 'RENDER_RAW_MATRIX=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $renderSubmit = [regex]::Match($gdbStdout, 'RENDER_SUBMIT=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $renderHardwareDivide = [regex]::Match($gdbStdout, 'RENDER_HWDIV=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $renderLazy = [regex]::Match($gdbStdout, 'RENDER_LAZY=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $renderVertex = [regex]::Match($gdbStdout, 'RENDER_VERTEX=(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+)')
    $renderDepth = [regex]::Match($gdbStdout, 'RENDER_DEPTH=([0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+)')
    $stageDepthTrace = [regex]::Match($gdbStdout, 'STAGE_DEPTH_TRACE=([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $renderClip = [regex]::Match($gdbStdout, 'RENDER_CLIP=([0-9]+)')
    $renderTexture = [regex]::Match($gdbStdout, 'RENDER_TEXTURE=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+)')
    $renderTexel1 = [regex]::Match($gdbStdout, 'RENDER_TEXEL1=([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+)')
    $renderTexUse = [regex]::Match($gdbStdout, 'RENDER_TEXUSE=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $renderTexFmt = [regex]::Match($gdbStdout, 'RENDER_TEXFMT=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $renderTexLane = [regex]::Match($gdbStdout, 'RENDER_TEXLANE=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $renderCombine = [regex]::Match($gdbStdout, 'RENDER_COMBINE=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $renderLight = [regex]::Match($gdbStdout, 'RENDER_LIGHT=([0-9]+),([0-9]+),([0-9]+)')
    $fighterLightSeed = [regex]::Match($gdbStdout, 'FTR_LIGHT_SEED=([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $ifHud = [regex]::Match($gdbStdout, 'IFHUD=([0-9]+),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $ifHudLower = [regex]::Match($gdbStdout, 'IFHUD_LOWER=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $ifHudRoute = [regex]::Match($gdbStdout, 'IFHUD_ROUTE=([0-9]+),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+)')
    $projectile = [regex]::Match($gdbStdout, 'PROJECTILE=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0)')
    $reflector = [regex]::Match($gdbStdout, 'REFLECTOR=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+),([0-9]+)')
    $specials = [regex]::Match($gdbStdout, 'SPECIALS=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+)')
    $audioAsset = [regex]::Match($gdbStdout, 'AUDIO_ASSET=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $audioFgmKo = [regex]::Match($gdbStdout, 'AUDIO_FGM_KO=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $audioFgmMissPairPattern = ((0..15 | ForEach-Object {
        '([0-9]+):([0-9]+)'
    }) -join ',')
    $audioFgmMiss = [regex]::Match(
        $gdbStdout,
        ([regex]::Escape('AUDIO_FGM_MISS=') +
         '([0-9]+),([0-9]+),' + $audioFgmMissPairPattern))
    if ($ImportBattleShipAudioAssets) {
        Assert-Condition $audioFgmMiss.Success `
            'Audio FGM unsupported-ID census marker is missing.' $gdbStdout
    }
    $audioBgm = [regex]::Match($gdbStdout, 'AUDIO_BGM=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $audioBgmAdpcm = [regex]::Match($gdbStdout, 'AUDIO_BGM_ADPCM=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $boundary = [regex]::Match($gdbStdout, 'BOUNDARY=(0x[0-9a-fA-F]+|0),([0-9]+)')
    $m4FenceFinalSummary = ''
    $m4FenceFinalPass = $true
    $m4FenceFinalValues = @()
    $expectedM4TeardownCount = if ($OneMinuteMatchProof) { 1 } else { 0 }
    $expectedM4SeenMask = if ($OneMinuteMatchProof) { 0xffffff } else { 0x3fffff }
    $expectedM4OwnerMask = if ($OneMinuteMatchProof) { 0x1f } else { 0x7 }
    if ($RequireZeroPostGoTextureFence) {
        Assert-Condition ($m4FenceFinal.Count -eq 1) `
            "M4 terminal texture fence captured $($m4FenceFinal.Count) records instead of one." `
            $gdbStdout
        $m4FenceFinalValues = Get-Ints $m4FenceFinal[0]
        $m4FenceFinalCountSum = [int64]0
        foreach ($fenceCount in $m4FenceFinalValues[14..23]) {
            $m4FenceFinalCountSum += $fenceCount
        }
        $m4FenceFinalPass =
            $m4FenceFinalValues[1] -eq 1 -and
            $m4FenceFinalValues[2] -eq 1 -and
            $m4FenceFinalValues[3] -eq 0 -and
            $m4FenceFinalValues[4] -eq 24 -and
            $m4FenceFinalValues[5] -eq 136192 -and
            $m4FenceFinalValues[6] -eq 1 -and
            $m4FenceFinalValues[7] -eq $expectedM4TeardownCount -and
            $m4FenceFinalValues[10] -eq 0 -and
            $m4FenceFinalValues[12] -eq 0 -and
            $m4FenceFinalValues[13] -eq 0 -and
            $m4FenceFinalCountSum -eq 0
        if ($isCpuPrepNoGx) {
            Assert-Condition ($m4FenceFinalCountSum -eq 0) `
                'CPU_PREP_NO_GX unexpectedly recorded post-GO texture-fence activity.' `
                $gdbStdout
        }
        elseif (-not $Task25RPacingTrace -and -not $PhaseMatrixMode) {
            Assert-Condition $m4FenceFinalPass `
                "M4 terminal post-GO texture fence failed (actual=$($m4FenceFinalValues -join ','))." `
                $gdbStdout
        }
        if (-not $isCpuPrepNoGx -and -not $OneMinuteMatchProof -and ($RendererBenchmarkSamples -gt 0)) {
            Assert-Condition (
                ($m4FenceFinalValues[8] -band $expectedM4SeenMask) -eq
                    $expectedM4SeenMask -and
                ($m4FenceFinalValues[9] -band $expectedM4OwnerMask) -eq
                    $expectedM4OwnerMask -and
                $m4FenceFinalValues[11] -gt 0
            ) 'M4 strict benchmark did not cover every prepared owner through pinned hits.' $gdbStdout
        }
        $m4FenceFinalSummary =
            "M4 terminal post-GO texture fence: pass=$m4FenceFinalPass teardown=$($m4FenceFinalValues[7]) first=$($m4FenceFinalValues[12])/$($m4FenceFinalValues[13]) counts=$($m4FenceFinalValues[14..23] -join ',')"
    }
    $publishedRendererDefaultsSummary = ''
    if ($usesPublishedIntrinsicRendererDefaults) {
        Assert-Condition ($fastFinal.Count -eq 1) `
            "Published M3 terminal owner captured $($fastFinal.Count) records instead of one." `
            $gdbStdout
        Assert-Condition ($m4WaterStillFinal.Count -eq 1) `
            "Published frozen-water state captured $($m4WaterStillFinal.Count) records instead of one." `
            $gdbStdout
        Assert-Condition ($m4FenceFinal.Count -eq 1) `
            "Published M4 terminal texture fence captured $($m4FenceFinal.Count) records instead of one." `
            $gdbStdout
        $publishedFast = Get-Ints $fastFinal[0]
        $publishedWater = Get-Ints $m4WaterStillFinal[0]
        $publishedM4 = Get-Ints $m4FenceFinal[0]
        $publishedBanks = Get-Ints $vramBanks
        $publishedFenceCountSum = [int64]0
        foreach ($fenceCount in $publishedM4[14..23]) {
            $publishedFenceCountSum += $fenceCount
        }
        Assert-Condition (
            $publishedFast[0] -eq 9 -and
            $publishedFast[1] -eq 121 -and
            $publishedFast[2] -eq 828 -and
            $publishedFast[3] -eq 202 -and
            $publishedFast[4] -eq 320 -and
            $publishedFast[5] -eq 306 -and
            $publishedFast[6] -eq 0 -and
            $publishedFast[7] -eq 0 -and
            $publishedFast[8] -eq 0
        ) "Published ROM did not naturally execute the exact M3 121-run/828-triangle owner (actual=$($publishedFast -join ','))." $gdbStdout
        Assert-Condition (
            $publishedWater[0] -eq 2 -and
            $publishedWater[1] -eq 0 -and
            $publishedWater[2] -eq 1
        ) "Published ROM did not retain the exact two-object frozen-water state (actual=$($publishedWater -join ','))." $gdbStdout
        Assert-Condition (
            $publishedM4[1] -eq 1 -and
            $publishedM4[2] -eq 1 -and
            $publishedM4[3] -eq 0 -and
            $publishedM4[4] -eq 24 -and
            $publishedM4[5] -eq 136192 -and
            $publishedM4[6] -eq 1 -and
            $publishedM4[7] -eq $expectedM4TeardownCount -and
            $publishedM4[8] -eq $expectedM4SeenMask -and
            $publishedM4[9] -eq $expectedM4OwnerMask -and
            $publishedM4[10] -eq 0 -and
            $publishedM4[11] -gt 0 -and
            $publishedM4[12] -eq 0 -and
            $publishedM4[13] -eq 0 -and
            $publishedFenceCountSum -eq 0
        ) "Published ROM did not preserve the complete M4 residency lifecycle and zero post-GO fence (actual=$($publishedM4 -join ','))." $gdbStdout
        Assert-Condition (
            $vramBanks.Success -and
            $publishedBanks[0] -eq 0x83 -and
            $publishedBanks[1] -eq 0x8b -and
            $publishedBanks[2] -eq 0x81 -and
            $publishedBanks[3] -eq 0x89 -and
            $publishedBanks[4] -eq 0x82 -and
            $publishedBanks[5] -eq 0x83 -and
            $publishedBanks[6] -eq 0x8b -and
            $publishedBanks[7] -eq 0x81 -and
            $publishedBanks[8] -eq 0x06800000 -and
            $publishedBanks[9] -eq 0x06821400 -and
            $publishedBanks[10] -eq 136192 -and
            $publishedBanks[11] -eq 3
        ) "Published ROM did not naturally preserve the IFCommon OBJ/source-alpha palette banks and the exact contiguous bank-A/B static span (actual=$($publishedBanks -join ','))." $gdbStdout
        $publishedRendererDefaultsSummary =
            " intrinsicM3=9/$($publishedFast[1])/$($publishedFast[2]) intrinsicM4=24/$($publishedM4[5])/hits$($publishedM4[11])/fence0 water=2/0/1"
    }
    Assert-Condition ($aobj32.Success -and [int64]$aobj32.Groups[4].Value -eq 0) 'AObjEvent32 source-command normalization rejected a live script.' $gdbStdout
    $aobjSummary = " aobj32=$($aobj32.Groups[1].Value)/$($aobj32.Groups[2].Value)/reuse$($aobj32.Groups[3].Value)/fail0"
    Assert-Condition ($harn.Success -and (Convert-MarkerUInt32 $harn.Groups[1].Value) -eq 0x4841524e -and [int]$harn.Groups[2].Value -eq $ExpectedMode -and [int]$harn.Groups[3].Value -eq $ExpectedHarnessSceneCurr -and [int]$harn.Groups[4].Value -eq $ExpectedHarnessScenePrev -and (Convert-MarkerUInt32 $harn.Groups[5].Value) -eq 0) $HarnessSelectMessage $gdbStdout
    if ($MatchLifecycleProof) {
        # scvsbattle.c:559-560 changes VSBattle to VS Results after task return.
        Assert-Condition ($scene.Success -and [int]$scene.Groups[1].Value -eq 24 -and [int]$scene.Groups[2].Value -eq 22 -and [int]$scene.Groups[3].Value -eq 6) 'Completed Pupupu battle did not enter VS Results from VSBattle.' $gdbStdout
    } else {
        Assert-Condition ($scene.Success -and [int]$scene.Groups[1].Value -eq 22 -and [int]$scene.Groups[2].Value -eq 21 -and [int]$scene.Groups[3].Value -eq 6) 'Live scene is not Pupupu VSBattle from Maps.' $gdbStdout
    }
    if ($RealtimePresentation -and $LiveInputPreview -and $HardwareTriangles) {
        $bm = Get-Ints $buildMode
        Assert-Condition ($buildMode.Success -and $bm[0] -eq 0x43414e4f -and $bm[1] -eq 0x53484950 -and $bm[2] -eq 0) 'Canonical shipped ROM build marker did not report CANO/SHIP realtime config.' $gdbStdout
    } elseif (-not $RealtimePresentation) {
        $bm = Get-Ints $buildMode
        Assert-Condition ($buildMode.Success -and $bm[2] -eq 0x46415354) 'Fast-logic verifier build marker did not report FAST config.' $gdbStdout
    }
    if ($ImportBattleShipFTComputer) {
        $cc = Get-Ints $computerConfig
        $expectedTimeLimit = 1
        Assert-Condition ($computerConfig.Success -and $cc[0] -eq 0 -and $cc[1] -eq 1 -and $cc[2] -eq 3 -and $cc[3] -eq 1 -and $cc[4] -eq 1 -and $cc[5] -eq $expectedTimeLimit -and $cc[6] -eq 0 -and $cc[7] -eq 0 -and $cc[8] -eq $FoxCpuMode) 'Mode 163 did not preserve the items-off Mario human versus Fox level-3 CPU match and selected Fox CPU decision mode.' $gdbStdout
    }
    $task9StateCapture = $null
    if ($Task9StateHashMode -eq 1) {
        $task9StateCapture = Complete-Task9StateHashCapture `
            -Summary $task9StateSummary `
            -Records $task9StateRecords `
            -GdbOutput $gdbStdout
        $task9StateSummaryValues = $task9StateCapture.Summary
        $task9StateRows = $task9StateCapture.Rows
    }
    if ($OneMinuteMatchProof) {
        $start = Get-Ints $matchStart
        $safety = Get-Ints $matchSafety
        $df = Get-Ints $damageFloor
        $bp = Get-Ints $battlePlayablePacing
        $ma = Get-Ints $vsbMemoryArena
        $mr = Get-Ints $vsbMemoryReloc
        $me = Get-Ints $vsbMemoryEvict
        $cpu = Get-Ints $computerAI
        $life = Get-Ints $battleLifecycle
        $results = Get-Ints $vsResults
        $resultsFighters = Get-Ints $vsResultsFighters
        $resultsDisplay = Get-Ints $vsResultsDisplay
        $fdc = Get-Ints $fighterDisplayContract
        $ab = Get-Ints $audioBgm
        $fgmKo = Get-Ints $audioFgmKo
        $fgmMiss = Get-Ints $audioFgmMiss
        $audioResidentBytes = if ($audioBgm.Success) { $ab[13] } else { 0 }
        $task25rPacingEvidence = if ($Task25RPacingTrace) {
            Complete-Task25RPacingTrace -Matches $task25rPacing `
                -Pacing $bp -GdbOutput $gdbStdout
        } else { $null }

        # BattleShip ifcommon.c:2533-2542 initializes one minute as 3,600
        # source tics; :2472-2529 decrements it and invokes the unchanged Time
        # Up callback at zero. Exact Wait/start state prevents a late attach
        # from masquerading as a full-duration match.
        Assert-Condition ($matchStart.Success -and $start[0] -eq 22 -and
            $start[1] -eq 0 -and $start[2] -eq 1 -and
            $start[3] -eq 3600 -and $start[4] -eq 0 -and
            $start[5] -eq 0 -and $start[6] -eq 1 -and
            $start[7] -eq 1) `
            'One-minute match did not begin at the exact locked 1:00 Wait state.' `
            $gdbStdout
        Assert-Condition ($battlePlayablePacing.Success -and
            $bp[0] -eq 0x42505443 -and $bp[1] -eq 0 -and
            $bp[2] -eq (2 * $bp[3]) -and $bp[3] -gt 0 -and
            $bp[4] -eq $bp[3] -and $bp[5] -gt 0 -and
            $bp[6] -gt 0 -and $bp[6] -le 305 -and
            $bp[8] -gt 0 -and $bp[9] -ge 2 -and
            $bp[10] -ge $bp[9] -and $bp[11] -eq 0 -and
            (($bp[12] + $bp[13] + $bp[14] + $bp[15] + $bp[16]) -eq
             $bp[3])) `
            'One-minute match did not retain exactly two committed source updates per presented frame.' `
            $gdbStdout
        $phaseRatesX10 = @()
        for ($phase = 0; $phase -lt 5; $phase++) {
            $phasePresents = [int64]$bp[12 + $phase]
            $phaseVBlanks = (2 * $phasePresents) +
                [int64]$bp[17 + $phase]
            $phaseRateX10 = if ($phaseVBlanks -gt 0) {
                [int64][Math]::Floor(((1200 * $phasePresents) +
                    [Math]::Floor($phaseVBlanks / 2)) / $phaseVBlanks)
            } else { 0 }
            $phaseRatesX10 += $phaseRateX10
            if (($phasePresents -gt 0) -and ($bp[17 + $phase] -eq 0)) {
                Assert-Condition ($phaseRateX10 -ge 590 -and
                    $phaseRateX10 -le 610) `
                    "One-minute phase $phase held 30 presents/s but did not retain 59..61 source updates/s and 1x source-timer speed." `
                    $gdbStdout
            }
        }

        # This gate intentionally proves CPU activity, not a scripted combat
        # outcome. ftcomputer.c owns the observed target/objective/input path.
        # Guard is reported below but is not required here: level-3 defense is
        # opportunity/RNG-dependent, and a neutral human does not guarantee a
        # shield decision during every one-minute match.
        Assert-Condition ($computerAI.Success -and $cpu[0] -eq 1 -and
            $cpu[1] -ge 2 -and $cpu[2] -ge 3600 -and $cpu[3] -gt 0 -and
            (($cpu[4] -band 0x4) -eq 0x4) -and $cpu[6] -gt 0 -and
            $cpu[7] -gt 0 -and $cpu[8] -gt 0 -and $cpu[9] -gt 0 -and
            $cpu[10] -gt 0 -and $cpu[11] -gt 0 -and $cpu[12] -gt 0 -and
            $cpu[15] -gt 0 -and
            $cpu[19] -gt 0 -and $cpu[20] -gt 0 -and
            ($cpu[23] - $cpu[22]) -ge 50000) `
            'Imported level-3 Fox CPU was not naturally active across the one-minute match.' `
            $gdbStdout

        # ftcommondead.c:183-190,227-231 and :476-488 enqueue the exact
        # character voice/slam plus DeadExplodeL triplet. Side/down deaths use
        # character/character/explode order; up-star deaths use explode first.
        $koTrace = @($fgmKo[7], $fgmKo[8], $fgmKo[9])
        $marioRegular = ($koTrace -join ',') -eq '439,292,154'
        $marioUpStar = ($koTrace -join ',') -eq '154,439,292'
        $foxRegular = ($koTrace -join ',') -eq '370,289,154'
        $foxUpStar = ($koTrace -join ',') -eq '154,370,289'
        $marioCounts = $fgmKo[1] -gt 0 -and $fgmKo[2] -gt 0 -and
            $fgmKo[5] -gt 0 -and (($fgmKo[0] -band 0x13) -eq 0x13)
        $foxCounts = $fgmKo[3] -gt 0 -and $fgmKo[4] -gt 0 -and
            $fgmKo[5] -gt 0 -and (($fgmKo[0] -band 0x1c) -eq 0x1c)
        $audioFgmKoPass = $audioFgmKo.Success -and $fgmKo[6] -ge 3 -and
            ((($marioRegular -or $marioUpStar) -and $marioCounts) -or
             (($foxRegular -or $foxUpStar) -and $foxCounts)) -and
            $fgmKo[10] -eq 0 -and $fgmKo[11] -eq 0 -and
            $fgmKo[12] -eq 0 -and $fgmKo[13] -eq 0
        if (-not $Task25RPacingTrace) {
            Assert-Condition $audioFgmKoPass `
                'Natural one-minute combat did not play one exact source KO FGM triplet cleanly.' `
                $gdbStdout
        }

        # scvsbattle.c:513-560 returns from the battle task, scores the match,
        # and changes VSBattle (22) to VS Results (24). Results creates the
        # wallpaper, text, and fighters at source tics 80/120.
        Assert-Condition ($battleLifecycle.Success -and
            $life[0] -eq 0x5642454e -and $life[1] -ge 1 -and
            $life[2] -ge 1 -and $life[3] -eq 1 -and $life[4] -eq 1 -and
            $life[5] -eq 0 -and $life[6] -eq 3600 -and
            $life[7] -eq 7 -and $life[8] -eq 22 -and $life[9] -eq 24 -and
            ($life[10] -eq 0 -or $life[10] -eq 1)) `
            'Original one-minute timer/end flow did not reach Time Up and transition VSBattle to VS Results.' `
            $gdbStdout
        if ($Task25RPacingTrace) {
            Assert-Condition ($life[2] -eq 1) `
                'Task 25R lifecycle trace did not observe exactly one battle teardown.' `
                $gdbStdout
        }
        Assert-Condition ($vsResults.Success -and
            $results[0] -eq 0x56535231 -and
            (($results[1] -band 0x1f) -eq 0x1f) -and
            $results[2] -ge 1 -and $results[3] -ge 120 -and
            $results[4] -eq 8 -and $results[5] -ge 2 -and
            $results[6] -gt 0 -and $results[7] -gt 0 -and
            $results[8] -eq 0) `
            'Original time-royal VS Results scene did not finish source setup.' `
            $gdbStdout
        $fighter0StatusOk = if ($resultsFighters[0] -eq 0) {
            $resultsFighters[1] -ge 0x10001 -and
                $resultsFighters[1] -le 0x10003 -and
                $resultsFighters[2] -ge 1 -and $resultsFighters[2] -le 3
        } else {
            $resultsFighters[1] -eq 0x10005 -and $resultsFighters[2] -eq 5
        }
        $fighter1StatusOk = if ($resultsFighters[3] -eq 0) {
            $resultsFighters[4] -ge 0x10001 -and
                $resultsFighters[4] -le 0x10003 -and
                $resultsFighters[5] -ge 1 -and $resultsFighters[5] -le 3
        } else {
            $resultsFighters[4] -eq 0x10005 -and $resultsFighters[5] -eq 5
        }
        Assert-Condition ($vsResultsFighters.Success -and
            $resultsFighters[0] -ne $resultsFighters[3] -and
            $fighter0StatusOk -and $fighter1StatusOk) `
            'One-minute VS Results fighters did not enter source win/lose status paths.' `
            $gdbStdout
        Assert-Condition ($vsResultsDisplay.Success -and
            $resultsDisplay[1] -gt 0 -and $resultsDisplay[2] -eq 256 -and
            $resultsDisplay[3] -eq 192) `
            'One-minute VS Results did not commit a full DS frame.' `
            $gdbStdout

        # The arena ledger records the VSBattle high-water mark. Require the
        # same conservative reserve used by the integrated battle verifier
        # after accounting for the resident BGM buffer.
        Assert-Condition ($audioBgm.Success -and $audioResidentBytes -eq 16392) `
            'One-minute match did not report the resident BGM allocation used by the reserve gate.' `
            $gdbStdout
        $arenaLedgerValid = $vsbMemoryArena.Success -and
            $ma[0] -eq 0x4d4c4544 -and $ma[1] -eq 22 -and
            $ma[3] -ge 0x130000 -and $ma[5] -le $ma[3] -and
            $ma[6] -eq ($ma[3] - $ma[5]) -and
            $ma[7] -eq 163840 -and
            $ma[8] -eq 106496 -and $ma[9] -eq 49152 -and $ma[10] -gt 0
        $reserveBytes = [int64]$ma[6] - $audioResidentBytes
        $reservePass = $reserveBytes -ge 131072
        Assert-Condition $arenaLedgerValid `
            'One-minute match arena ledger was internally inconsistent.' `
            $gdbStdout
        if (-not $Task25RPacingTrace) {
            Assert-Condition $reservePass `
                'One-minute match did not preserve the 128 KiB battle arena reserve.' `
                $gdbStdout
        }
        Assert-Condition ($vsbMemoryReloc.Success -and $mr[0] -gt 0 -and
            $mr[1] -gt 0 -and $mr[2] -gt 0 -and $mr[3] -gt 0 -and
            $mr[4] -gt 0 -and $mr[5] -eq 0 -and $mr[6] -eq 0 -and
            $mr[8] -eq 0 -and $mr[9] -eq 0 -and $vsbMemoryEvict.Success) `
            'One-minute match reloc ledger found stale or missing resident groups.' `
            $gdbStdout
        Assert-Condition ($matchSafety.Success -and
            @($safety[0..15] | Where-Object { $_ -ne 0 }).Count -eq 0) `
            'One-minute match observed a stale-reloc, normalization, fixup, audio-safety, or collision-range failure.' `
            $gdbStdout
        # gmCameraCheckTargetInBounds is the source magnify/culling predicate,
        # not a memory-safety check. Natural KO/rebirth motion may leave that
        # visibility envelope. Part selection/submission and target-bounds
        # decisions have different granularities, so gate each domain without
        # inventing a false equality between them. The focused camera-
        # containment verifier retains its zero-failure gate.
        Assert-Condition ($fighterDisplayContract.Success -and
            $fdc[0] -gt 0 -and $fdc[3] -eq $fdc[0] -and
            ($fdc[7] + $fdc[8]) -gt 0 -and
            $safety[16] -eq $fdc[8]) `
            'One-minute fighter part submission or source visibility-bound accounting was inconsistent.' `
            $gdbStdout

        if ($m4FenceFinalSummary) { Write-Output $m4FenceFinalSummary }
        if ($task20StackSummary) { Write-Output $task20StackSummary }
        if ($Task25RLifecycleExportPath) {
            $resolvedTask25RPath = if ([System.IO.Path]::IsPathRooted(
                    $Task25RLifecycleExportPath)) {
                $Task25RLifecycleExportPath
            } else {
                Join-Path $root $Task25RLifecycleExportPath
            }
            $task25rParent = Split-Path -Parent $resolvedTask25RPath
            if ($task25rParent -and -not (Test-Path -LiteralPath $task25rParent)) {
                New-Item -ItemType Directory -Path $task25rParent -Force |
                    Out-Null
            }
            $task25rGitHead = (& git -C $root rev-parse HEAD).Trim()
            $task25rGitStatus = @(& git -C $root status --short)
            $task25rFocusedDiff = @(& git -C $root diff -- Makefile include/nds src/nds src/port scripts)
            $task25rExport = [ordered]@{
                schema = 1
                kind = 'smash64ds-task25r-profile0-lifecycle'
                identity = [ordered]@{
                    gitHead = $task25rGitHead
                    gitStatus = $task25rGitStatus
                    focusedDiff = $task25rFocusedDiff
                    submodules = @(& git -C $root submodule status)
                    target = $Target
                    build = $Build
                    makeCommand = @('make') + @($makeArgs)
                    make = $benchmarkMakeIdentity
                    artifacts = [ordered]@{
                        rom = [ordered]@{
                            path = $rom
                            sha256 = $benchmarkRomIdentity.Sha256
                            bytes = $benchmarkRomIdentity.Bytes
                        }
                        elf = [ordered]@{
                            path = $elf
                            sha256 = $benchmarkElfIdentity.Sha256
                            bytes = $benchmarkElfIdentity.Bytes
                        }
                        screenshot = if ($screenshotPath -and
                            (Test-Path -LiteralPath $screenshotPath)) {
                            [ordered]@{
                                path = $screenshotPath
                                sha256 = (Get-FileHash -LiteralPath $screenshotPath `
                                    -Algorithm SHA256).Hash
                            }
                        } else { $null }
                    }
                }
                pacing = $task25rPacingEvidence
                gates = [ordered]@{
                    exactTwoUpdatesPerPresentation = ($bp[2] -eq (2 * $bp[3]))
                    zeroDebtOrCatchUp = $true
                    cadenceViolationCount = [int64]$bp[11]
                    exactlyOneTeardown = ($life[2] -eq 1)
                    reserveBytes = $reserveBytes
                    reserveFloorBytes = [int64]131072
                    reservePass = $reservePass
                    exactKoFgmTriplet = $audioFgmKoPass
                    zeroPostGoTextureFence = $m4FenceFinalPass
                    stable30 = [bool]$task25rPacingEvidence.stable30
                }
                lifecycle = [ordered]@{
                    matchStart = $start
                    pacingFinal = $bp
                    battleEnd = $life
                    results = $results
                    memoryArena = $ma
                    memoryReloc = $mr
                    memoryEvict = $me
                    safety = $safety
                    audioFgmKo = $fgmKo
                    m4FenceFinal = $m4FenceFinalValues
                }
            }
            Set-Content -LiteralPath $resolvedTask25RPath -Encoding utf8 `
                -Value ($task25rExport | ConvertTo-Json -Depth 9)
        }
        $task9StateSummaryText = if ($Task9StateHashMode -eq 1) {
            " stateHash=$($task9StateSummaryValues[0])/overflow$($task9StateSummaryValues[1])"
        } else { '' }
        $lifecycleResult = if ($Task25RPacingTrace) { 'captured' } else { 'passed' }
        Write-Output ("$Label one-minute match lifecycle ${lifecycleResult}: logic/present=$($bp[2])/$($bp[3]) timer=$($start[3])->$($life[5])/$($life[6]) phaseRate=$($phaseRatesX10 -join '/')x0.1 phaseSlip=$($bp[17..21] -join '/') boundsOutside=$($fdc[8])/$($fdc[7]+$fdc[8]) CPU=$($cpu[2]) inputs=$($cpu[6]) attack=$($cpu[11])/$($cpu[12]) guard=$($cpu[13]) recover=$($cpu[14]) KO=$($koTrace -join '/') mask=0x$('{0:x}' -f $fgmKo[0]) koExact=$audioFgmKoPass fgmMiss=$($fgmMiss[0])/$($fgmMiss[1]) scene=$($life[8])->$($life[9]) results=$($results[3]) reserve=$($ma[6])-$audioResidentBytes stale=$($mr[8])/$($mr[9]) safety=0 evict=$($me[0])/$($me[1]) floorDamage=$($df[4])/$($df[3]) checks=$($df[0]) edgeDeferred=$($df[5]) line=$($df[8]) root=$($df[10])->$($df[11])$task9StateSummaryText")
        return
    }
    if ($ExpectedMode -eq 54) {
        Assert-Condition ($vs.Success -and (Convert-MarkerUInt32 $vs.Groups[1].Value) -eq 0x56535452 -and ((Convert-MarkerUInt32 $vs.Groups[2].Value) -band 0xff) -eq 0xff) 'Menu-chain VS Mode -> PlayersVS transition did not pass.' $gdbStdout
        Assert-Condition ($pv.Success -and (Convert-MarkerUInt32 $pv.Groups[1].Value) -eq 0x50565452 -and ((Convert-MarkerUInt32 $pv.Groups[2].Value) -band 0xff) -eq 0xff) 'Menu-chain PlayersVS -> Maps transition did not pass.' $gdbStdout
        Assert-Condition ($maps.Success -and (Convert-MarkerUInt32 $maps.Groups[1].Value) -eq 0x4d53454c -and ((Convert-MarkerUInt32 $maps.Groups[2].Value) -band 0xff) -eq 0xff -and [int]$maps.Groups[3].Value -eq 6) 'Menu-chain Maps -> VSBattle transition did not pass.' $gdbStdout
    }
    if ($ImportBattleShipFTManager) {
        $nat = Get-Ints $natural
        $nfig = Get-Ints $naturalFig
        $nw = Get-Ints $naturalWait
        $nwalk = Get-Ints $naturalWalk
        $nc = Get-Ints $naturalChain
        $na = Get-Ints $naturalAttack
        $nh = Get-Ints $naturalHitlag
        $ng = Get-Ints $naturalGuard
        if ($ImportBattleShipNormalMoveset) {
            $nm = Get-Ints $naturalMoveset
        }
        $hardwareSummary = ''
        if ($BattlePlayable -and $RealtimePresentation) {
            $bp = Get-Ints $battlePlayablePacing
            $fpsHud = Get-Ints $battlePlayableFpsHud
            $wallSeconds = [double]$bp[5] / 33513982.0
            Assert-Condition (
                $battlePlayablePacing.Success -and
                $bp[0] -eq 0x42505443 -and $bp[1] -eq 0 -and
                $bp[2] -eq (2 * $bp[3]) -and $bp[3] -ge 180 -and
                $bp[4] -eq $bp[3] -and $bp[5] -gt 0 -and
                $bp[6] -gt 0 -and
                ($Task34StageStreamCensus -or $bp[6] -le 305) -and
                $bp[8] -gt 0 -and $bp[9] -ge 2 -and
                $bp[10] -ge $bp[9] -and $bp[11] -eq 0 -and
                (($bp[12] + $bp[13] + $bp[14] + $bp[15] + $bp[16]) -eq
                 $bp[3])
            ) 'battle_playable locked-30 pacing failed the exact 2:1 update/present ratio, 30Hz present cap, draw count, cadence, or phase accounting contract.' $gdbStdout
            $phaseRatesX10 = @()
            for ($phase = 0; $phase -lt 5; $phase++) {
                $phasePresents = [int64]$bp[12 + $phase]
                $phaseVBlanks = (2 * $phasePresents) + [int64]$bp[17 + $phase]
                $phaseRateX10 = if ($phaseVBlanks -gt 0) {
                    [int64][Math]::Floor(((1200 * $phasePresents) +
                        [Math]::Floor($phaseVBlanks / 2)) / $phaseVBlanks)
                } else { 0 }
                $phaseRatesX10 += $phaseRateX10
                if (($phasePresents -gt 0) -and ($bp[17 + $phase] -eq 0)) {
                    Assert-Condition ($phaseRateX10 -ge 590 -and
                        $phaseRateX10 -le 610) `
                        "battle_playable phase $phase held 30 presents/s but did not retain 59..61 source updates/s and 1x source-timer speed." `
                        $gdbStdout
                }
            }
            if (-not $RendererBenchmarkOnly) {
                $fpsHudExpected = if ($fpsHud[3] -gt 0) {
                    [int64][Math]::Floor(
                        (([int64]$fpsHud[2] * 33513982 * 10) +
                         [Math]::Floor($fpsHud[3] / 2)) / $fpsHud[3])
                } else {
                    0
                }
                Assert-Condition ($battlePlayableFpsHud.Success -and
                    $fpsHud[0] -gt 0 -and $fpsHud[0] -le 700 -and
                    $fpsHud[1] -gt 0 -and $fpsHud[2] -gt 0 -and
                    $fpsHud[3] -ge 16756991 -and
                    $fpsHud[0] -eq $fpsHudExpected) `
                    'battle_playable lower-screen rolling FPS counter did not sample actual presentation cadence.' $gdbStdout
            }
            if ($ImportBattleShipIFCommon -and -not $RendererBenchmarkOnly) {
                $textHud = Get-Ints $battleTextHud
                $sourceHud = Get-Ints $ifHud
                $sourceLower = Get-Ints $ifHudLower
                $sourceRoute = Get-Ints $ifHudRoute
                $sourceStart = Get-Ints $battlePlayableStart
                $routeOam = Get-Ints $ifCommonOam
                $expectedHudSeconds = if ($sourceLower[10] -eq 0) {
                    0
                } elseif ($sourceLower[10] -eq $sourceLower[11]) {
                    [Math]::Floor($sourceLower[10] / 60)
                } else {
                    [Math]::Floor(($sourceLower[10] + 59) / 60)
                }
                $lowerRoutingOk = if ($sourceRoute[0] -eq 1) {
                    $sourceRoute[1] -eq 0x3 -and
                    $sourceLower[14] -eq
                        ($sourceLower[15] + $sourceRoute[2]) -and
                    $sourceLower[15] -gt 0 -and
                    $sourceRoute[2] -gt 0
                } else {
                    $sourceRoute[0] -eq 0
                }
                $lowerTimerStateOk = if ($usesRetainedWallpaper) {
                    $sourceLower[10] -gt 0 -and
                    $sourceLower[10] -lt $sourceLower[11] -and
                    $sourceLower[12] -eq 1 -and
                    $sourceLower[13] -eq 1
                } else {
                    $sourceLower[10] -le $sourceLower[11] -and
                    (($sourceLower[12] -eq 0) -or
                     ($sourceLower[12] -eq 1))
                }
                # BattleShip's 90-tick sleep resumes countdown creation during
                # update 91. Profile 2
                # deliberately samples frame 45, so admit only the exact native-
                # OAM idle state before that source threshold. Every later
                # sample, including retained canonical post-GO, must have seen a
                # top interface GObj; exact drawing remains CUTG_EXACT's job.
                $preCountdownNativeIdle =
                    ($RendererProfileLevel -eq 2) -and
                    $battlePlayableStart.Success -and
                    $sourceStart[0] -eq 0 -and $sourceStart[1] -eq 1 -and
                    $sourceStart[2] -eq 3600 -and $sourceStart[3] -eq 0 -and
                    $sourceStart[4] -eq 0 -and $sourceStart[5] -eq 1 -and
                    $sourceStart[6] -eq 1 -and $sourceStart[7] -le 90 -and
                    $sourceRoute[3] -eq 0 -and $ifCommonOam.Success -and
                    $routeOam[0] -eq 1 -and $routeOam[1] -eq 1 -and
                    $routeOam[2] -eq 1 -and $routeOam[3] -eq 0 -and
                    $routeOam[10] -eq 0 -and $routeOam[11] -eq 0 -and
                    $routeOam[15] -eq 0 -and $routeOam[16] -eq 0 -and
                    $routeOam[17] -eq 1 -and $routeOam[19] -eq 0 -and
                    $routeOam[20] -eq 0 -and $routeOam[21] -eq 0 -and
                    $routeOam[22] -eq 0 -and $routeOam[23] -eq 0 -and
                    $routeOam[24] -eq 0x49464f41 -and
                    $routeOam[25] -eq 0 -and $routeOam[26] -eq 0
                $topPresentationRouteOk =
                    ($sourceRoute[3] -gt 0) -or $preCountdownNativeIdle
                Assert-Condition (
                    $ifHudLower.Success -and $ifHudRoute.Success -and
                    $sourceLower[0] -eq 0x3 -and
                    (($sourceLower[1] -band $sourceLower[0]) -eq
                     $sourceLower[1]) -and
                    (($sourceLower[2] -band $sourceLower[0]) -eq
                     $sourceLower[2]) -and
                    $sourceLower[3] -eq 0x2 -and
                    $sourceLower[4] -eq 0 -and $sourceLower[5] -eq 1 -and
                    $sourceLower[7] -eq 3 -and
                    $sourceLower[8] -gt 0 -and $sourceLower[9] -gt 0 -and
                    $sourceLower[11] -eq 3600 -and
                    $lowerTimerStateOk -and
                    $sourceRoute[0] -eq $LowerTextHudMode -and
                    $lowerRoutingOk -and $topPresentationRouteOk
                ) 'BattleShip timer/stock callbacks were not routed narrowly below, or countdown/GO lacked top-interface evidence outside the exact pre-update-91 native-OAM idle window.' $gdbStdout
                Assert-Condition (
                    $battleTextHud.Success -and
                    $textHud[0] -gt 0 -and $textHud[1] -gt 0 -and
                    $textHud[2] -ne 0 -and
                    $textHud[3] -eq $expectedHudSeconds -and
                    $textHud[4] -eq [Math]::Min($sourceHud[2], 999) -and
                    $textHud[5] -eq [Math]::Min($sourceHud[3], 999) -and
                    $textHud[6] -eq $sourceLower[8] -and
                    $textHud[7] -eq $sourceLower[9] -and
                    $textHud[8] -eq $sourceLower[0] -and
                    $textHud[9] -eq $sourceLower[1]
                ) 'Lower-screen text HUD did not match BattleShip timer, Mario/Fox identity, damage, stock-icon, or visibility state.' $gdbStdout
            }
            if ($usesRetainedWallpaper) {
                $bs = Get-Ints $battlePlayableStart
                $postGoState =
                    ($bs[0] -eq 1 -and $bs[1] -eq 1 -and
                     $bs[2] -gt 0 -and $bs[3] -gt 0 -and
                     ($bs[2] + $bs[3]) -eq 3600 -and
                     $bs[4] -eq 1 -and $bs[5] -eq 0 -and
                     $bs[6] -eq 0)
                Assert-Condition (
                    $battlePlayableStart.Success -and
                    ($bs[7] -eq $bp[2]) -and
                    $postGoState
                ) 'Cut G did not reach the source GO state with a running timer and both fighter controls unlocked.' $gdbStdout
            }
            if (($bp[6] -lt 295) -or ($bp[6] -gt 305)) {
                Write-Warning ("$Label locked-30 presentation slipped below its target: present=$($bp[6]) x0.1 fps; holds-30 remains a published metric until renderer cuts fit every two-vblank phase budget.")
            }
            if ($HardwareTriangles) {
                $hw = Get-Ints $platformHw
                $shw = Get-Ints $stageHardware
                $scarry = Get-Ints $stageCarry
                $shwf = Get-Ints $stageHardwareFighter
                $wr = Get-Ints $weaponRenderer
                $wframe = Get-Ints $weaponFrame
                $fdc = Get-Ints $fighterDisplayContract
                $rp = Get-Ints $renderProfile
                $rb = Get-Ints $renderBatch
                $rtopo = Get-Ints $renderTopology
                $rcost = Get-Ints $renderCost
                $rci4lut = Get-Ints $renderCi4Lut
                $rci4map = Get-Ints $renderCi4Map
                $rth = Get-Ints $renderTexHash
                $wc = Get-Ints $wallpaperCache
                $wf = Get-Ints $wallpaperFinal
                $wo = Get-Ints $wallpaperOracle
                if ($usesFastWallpaper) {
                    $fw = Get-Ints $fastWallpaper
                }
                if ($usesRetainedWallpaper) {
                    $ioam = Get-Ints $ifCommonOam
                    # Native OBJ bytes exclude the two source-alpha contour atlases, which live in texture VRAM.
                    $expectedIfCommonPrepareBytes = if ($effectiveIFCommonHybridOamMode -eq 1) { 31168 } else { 41728 }
                    $expectedIfCommonPaletteBytes = if ($effectiveIFCommonHybridOamMode -eq 1) { 544 } else { 32 }
                    $smc = Get-Ints $sceneMipCache
                    $swa = Get-Ints $sceneWallAffine
                }
                $ro = Get-Ints $renderOracle
                $rm = Get-Ints $renderMatrix
                $rac = Get-Ints $renderAdapterCache
                $rswc = Get-Ints $renderStageWorldCache
                $ram = Get-Ints $renderAffineMatrix
                $rrm = Get-Ints $renderRawMatrix
                $rs = Get-Ints $renderSubmit
                $rhdiv = Get-Ints $renderHardwareDivide
                $rlazy = Get-Ints $renderLazy
                $rv = Get-Ints $renderVertex
                $rd = Get-Ints $renderDepth
                $rclip = Get-Ints $renderClip
                $rt = Get-Ints $renderTexture
                $rt1 = Get-Ints $renderTexel1
                $ma = Get-Ints $mobjAttach
                $rtu = Get-Ints $renderTexUse
                $rtf = Get-Ints $renderTexFmt
                $rtl = Get-Ints $renderTexLane
                $rc = Get-Ints $renderCombine
                $rl = Get-Ints $renderLight
                $fls = Get-Ints $fighterLightSeed
                $benchmarkSummary = ''
                $benchmarkIdentitySummary = ''
                $benchmarkMetricSummary = ''
                $benchmarkChurnSummary = ''
                $coarseMetricSummary = ''
                $coarseResidualRatioSummary = ''
                $gxBoundarySummary = ''
                $stage0Summary = ''
                $sinkMetricSummary = ''
                $warmMetricSummary = ''
                $texturePhaseMetricSummary = ''
                $fastRunMetricSummary = ''
                $m3StageMetricSummary = ''
                $task36ReplayMetricSummary = ''
                $m3GeneratedSegment0MetricSummary = ''
                $m3GeneratedSegment0ShadowMetricSummary = ''
                $m3GeneratedSegment0GxMetricSummary = ''
                $m3Phase0MetricSummary = ''
                $m3ResidualMetricSummary = ''
                $m3PreparedMetricSummary = ''
                $m3PreparedCounts = [ordered]@{
                    comparison = 'adjacent-within-window'
                    boundary = 'unknown'
                    firstSampleIsOpportunity = $false
                    samples = 0
                    opportunities = 0
                    segments = @()
                    materials = @()
                }
                $g2StateMetricSummary = ''
                $phase05MetricSummary = ''
                $wallRunMetricSummary = ''
                $m4WaterStillMetricSummary = ''
                $m4StaticMetricSummary = ''
                $m4FenceMetricSummary = ''
                $m2MetricSummary = ''
                $m2ShadeMetricSummary = ''
                $semanticMetricSummary = ''
                $semanticChurnSummary = ''
                $semanticProvenanceSummaries = @()
                $ownerCensusSummaries = @()
                $ownerChurnSummaries = @()
                $task9FloatSummaries = @()
                $task9FloatRows = @()
                $task9Pairs = @()
                $task9Timer = @()
                $economySamples = @()
                $screenSpaceCensusRowValues = @()
                $screenSpaceCensusStageOwnerValues = @()
                if ($RendererBenchmarkSamples -gt 0) {
                    Assert-Condition ($rendererBenchmark.Count -eq $RendererBenchmarkSamples) "Renderer benchmark captured $($rendererBenchmark.Count) of $RendererBenchmarkSamples requested warm frames." $gdbStdout
                    $benchFrames = @($rendererBenchmark | ForEach-Object { [int64]$_.Groups[2].Value })
                    if ($RendererBenchmarkStartFrame -gt 0) {
                        Assert-Condition (
                            $benchFrames[0] -eq $RendererBenchmarkStartFrame -and
                            $benchFrames[-1] -eq
                                ($RendererBenchmarkStartFrame +
                                 $RendererBenchmarkSamples - 1)
                        ) "Renderer benchmark missed requested exact frame window $RendererBenchmarkStartFrame..$($RendererBenchmarkStartFrame + $RendererBenchmarkSamples - 1); captured $($benchFrames[0])..$($benchFrames[-1])." $gdbStdout
                    }
                    if ($RendererBenchmarkStartEvent -ne 'None') {
                        Assert-Condition (
                            $rendererBenchmarkEvent.Count -eq
                                $RendererBenchmarkSamples
                        ) "Renderer event benchmark captured $($rendererBenchmarkEvent.Count) of $RendererBenchmarkSamples event markers." $gdbStdout
                        $firstEvent = Get-Ints $rendererBenchmarkEvent[0]
                        Assert-Condition (
                            $firstEvent[0] -eq $benchFrames[0] -and
                            $firstEvent[2] -eq 1 -and
                            (($RendererBenchmarkStartEvent -ne 'KO') -or
                             ($firstEvent[1] -gt 0))
                        ) "Renderer benchmark did not start on the requested natural $RendererBenchmarkStartEvent event." $gdbStdout
                    }
                    $benchPresent = @($rendererBenchmark | ForEach-Object { [int64]$_.Groups[3].Value })
                    $benchDraw = @($rendererBenchmark | ForEach-Object { [int64]$_.Groups[4].Value })
                    $benchStage = @($rendererBenchmark | ForEach-Object { [int64]$_.Groups[5].Value })
                    $benchMaterial = @($rendererBenchmark | ForEach-Object { [int64]$_.Groups[6].Value })
                    $benchMatrix = @($rendererBenchmark | ForEach-Object { [int64]$_.Groups[7].Value })
                    $benchDL = @($rendererBenchmark | ForEach-Object { [int64]$_.Groups[8].Value })
                    $benchTexture = @($rendererBenchmark | ForEach-Object { [int64]$_.Groups[9].Value })
                    $benchSubmit = @($rendererBenchmark | ForEach-Object { [int64]$_.Groups[10].Value })
                    $benchVertex = @($rendererBenchmark | ForEach-Object { [int64]$_.Groups[11].Value })
                    $benchUploadCount = @($rendererBenchmark | ForEach-Object { [int64]$_.Groups[14].Value })
                    $benchUploadBytes = @($rendererBenchmark | ForEach-Object { [int64]$_.Groups[15].Value })
                    $benchSetup = @()
                    $benchScan = @()
                    if ($RenderEconomyMode -eq 1) {
                        Assert-Condition (
                            $economyBenchmark.Count -eq $RendererBenchmarkSamples
                        ) "Renderer economy captured $($economyBenchmark.Count) of $RendererBenchmarkSamples synchronized records." $gdbStdout
                    }
                    for ($sampleIndex = 0; $sampleIndex -lt $rendererBenchmark.Count; $sampleIndex++) {
                        $sample = $rendererBenchmark[$sampleIndex]
                        $sampleProfile = [int64]$sample.Groups[1].Value
                        $sampleFrame = [int64]$sample.Groups[2].Value
                        $sampleDL = [int64]$sample.Groups[8].Value
                        $sampleSubmit = [int64]$sample.Groups[10].Value
                        $sampleVertex = [int64]$sample.Groups[11].Value
                        $sampleTriangles = [int64]$sample.Groups[12].Value
                        $sampleOracle = [int64]$sample.Groups[13].Value
                        $sampleUploadCount = [int64]$sample.Groups[14].Value
                        $sampleUploadBytes = [int64]$sample.Groups[15].Value
                        $sampleTriangleContract = if (
                            $RenderEconomyMode -eq 1) {
                            $economy = Get-Ints $economyBenchmark[$sampleIndex]
                            $expectedMask = $RenderEconomyOwnerMask
                            $economySamples += , $economy
                            if ($expectedMask -eq 0) {
                                ($economy[0] -eq $sampleFrame) -and
                                ($economy[1] -eq 0) -and
                                ($economy[2] -eq 0) -and
                                ($economy[3] -eq 0) -and
                                ($economy[4] -eq 0) -and
                                ($sampleTriangles -eq 828)
                            } else {
                                ($economy[0] -eq $sampleFrame) -and
                                ($economy[1] -eq $expectedMask) -and
                                ($economy[2] -eq $expectedMask) -and
                                ($economy[3] -gt 0) -and
                                ($economy[4] -gt 0) -and
                                ($sampleTriangles -eq (828 - $economy[4]))
                            }
                        } elseif ($naturalScreenSpaceCensusWindow) {
                            $sampleTriangles -ge 202
                        } elseif ($PhaseMatrixMode -or
                            $Task22WallpaperRunLab -or
                            $m3StageOnlyBenchmarkWindow -or
                            ($RendererBenchmarkStartEvent -ne 'None')) {
                            $sampleTriangles -ge 202
                        } else {
                            $sampleTriangles -eq 828
                        }
                        Assert-Condition ($sampleProfile -eq $RendererProfileLevel -and $sampleTriangleContract -and (($RendererProfileLevel -ge 2 -and $sampleOracle -eq 2484) -or ($RendererProfileLevel -lt 2 -and $sampleOracle -eq 0))) 'Renderer benchmark sampled the wrong profile or violated the selected live-CPU/transient or exact 828-triangle/2,484-oracle contract.' $gdbStdout
                        Assert-Condition (Test-RendererUploadPair $sampleUploadCount $sampleUploadBytes) "Renderer benchmark sampled an invalid texture upload pair $sampleUploadCount/$sampleUploadBytes at frame $sampleFrame." $gdbStdout
                        if ($sampleIndex -gt 0) {
                            $previousFrame = [int64]$rendererBenchmark[$sampleIndex - 1].Groups[2].Value
                            Assert-Condition ($sampleFrame -eq ($previousFrame + 1)) "Renderer benchmark frame window is not synchronized: frame $sampleFrame followed $previousFrame." $gdbStdout
                        }
                        Assert-Condition ($sampleDL -ge $sampleSubmit -and $sampleSubmit -ge $sampleVertex) 'Renderer benchmark sampled incoherent nested DL/triangle/vertex costs.' $gdbStdout
                        $benchSetup += $sampleSubmit - $sampleVertex
                        $benchScan += $sampleDL - $sampleSubmit
                    }
                    $uploadSequenceHash = Get-RendererUploadSequenceHash $benchUploadCount $benchUploadBytes
                    if ($RendererScreenSpaceCensusMode -eq 1) {
                        Assert-Condition (
                            $screenSpaceCensusSummary.Count -eq 1 -and
                            $screenSpaceCensusStageOwners.Count -eq 8
                        ) 'Screen-space census omitted its summary or eight stage-owner tick rows.' $gdbStdout
                        $censusSummaryValues = Get-Ints $screenSpaceCensusSummary[0]
                        Assert-Condition (
                            $censusSummaryValues[0] -eq $RendererBenchmarkSamples -and
                            $censusSummaryValues[1] -eq 0 -and
                            $screenSpaceCensusRows.Count -gt 0
                        ) 'Screen-space census did not cover the exact sampled window without overflow.' $gdbStdout
                        $screenSpaceCensusRowValues = @(
                            $screenSpaceCensusRows | ForEach-Object {
                                , @(Get-Ints $_)
                            })
                        $screenSpaceCensusStageOwnerValues = @(
                            $screenSpaceCensusStageOwners | ForEach-Object {
                                , @(Get-Ints $_)
                            })
                        $censusStageTriangles = [int64](
                            ($screenSpaceCensusRowValues |
                                Where-Object { $_[0] -eq 0 } |
                                ForEach-Object { [int64]$_[3] } |
                                Measure-Object -Sum).Sum)
                        $censusMarioTriangles = [int64](
                            ($screenSpaceCensusRowValues |
                                Where-Object { $_[0] -eq 1 } |
                                ForEach-Object { [int64]$_[3] } |
                                Measure-Object -Sum).Sum)
                        $censusFoxTriangles = [int64](
                            ($screenSpaceCensusRowValues |
                                Where-Object { $_[0] -eq 2 } |
                                ForEach-Object { [int64]$_[3] } |
                                Measure-Object -Sum).Sum)
                        $censusInvalidTriangles = [int64](
                            ($screenSpaceCensusRowValues |
                                ForEach-Object { [int64]$_[6] } |
                                Measure-Object -Sum).Sum)
                        $censusStageOwnerCoverage = @(
                            $screenSpaceCensusStageOwnerValues |
                                Where-Object {
                                    ($_[0] -ge 0) -and ($_[0] -lt 8) -and
                                    (($_[1] -ne 0) -or ($_[2] -ne 0))
                                }).Count
                        Assert-Condition (
                            $censusStageTriangles -eq
                                (202 * $RendererBenchmarkSamples) -and
                            $censusMarioTriangles -gt 0 -and
                            ($censusMarioTriangles % 320) -eq 0 -and
                            $censusFoxTriangles -gt 0 -and
                            ($censusFoxTriangles % 306) -eq 0 -and
                            $censusInvalidTriangles -eq 0 -and
                            $censusStageOwnerCoverage -eq 8
                        ) "Screen-space census lost exact stage coverage, whole fighter packets, valid projections, or stage-owner timing (stage=$censusStageTriangles Mario=$censusMarioTriangles Fox=$censusFoxTriangles invalid=$censusInvalidTriangles owners=$censusStageOwnerCoverage)." $gdbStdout
                    }
                    $benchmarkMetricSummary = "Renderer benchmark: samples=$RendererBenchmarkSamples frames=$($benchFrames[0])..$($benchFrames[-1]) median/p95 ticks present=$((Get-Median $benchPresent))/$((Get-Percentile95 $benchPresent)) draw=$((Get-Median $benchDraw))/$((Get-Percentile95 $benchDraw)) stage=$((Get-Median $benchStage))/$((Get-Percentile95 $benchStage)) material=$((Get-Median $benchMaterial))/$((Get-Percentile95 $benchMaterial)) matrix=$((Get-Median $benchMatrix))/$((Get-Percentile95 $benchMatrix)) dl=$((Get-Median $benchDL))/$((Get-Percentile95 $benchDL)) texture=$((Get-Median $benchTexture))/$((Get-Percentile95 $benchTexture)) submit=$((Get-Median $benchSubmit))/$((Get-Percentile95 $benchSubmit)) vertex=$((Get-Median $benchVertex))/$((Get-Percentile95 $benchVertex)) setup=$((Get-Median $benchSetup))/$((Get-Percentile95 $benchSetup)) scan=$((Get-Median $benchScan))/$((Get-Percentile95 $benchScan)) uploads=$(Get-MedianP95 $benchUploadCount)/$(Get-MedianP95 $benchUploadBytes) uploadSequenceSha256=$uploadSequenceHash"
                    $benchmarkChurnSummary = "Renderer benchmark churn (adjacent changes/distinct values): present=$((Get-AdjacentChurn $benchPresent)) draw=$((Get-AdjacentChurn $benchDraw)) stage=$((Get-AdjacentChurn $benchStage)) material=$((Get-AdjacentChurn $benchMaterial)) matrix=$((Get-AdjacentChurn $benchMatrix)) dl=$((Get-AdjacentChurn $benchDL)) texture=$((Get-AdjacentChurn $benchTexture)) submit=$((Get-AdjacentChurn $benchSubmit)) vertex=$((Get-AdjacentChurn $benchVertex)) setup=$((Get-AdjacentChurn $benchSetup)) scan=$((Get-AdjacentChurn $benchScan))"
                    # Profile 0 intentionally emits only the non-nested
                    # RENDER_BENCH window. Keep its JSON schema stable with
                    # empty coarse/forensic collections instead of indexing
                    # arrays that are created only by profiles 1/2.
                    $coarseSamples = [System.Collections.Generic.List[object]]::new()
                    $texturePhaseSamples = [System.Collections.Generic.List[object]]::new()
                    $fastRunSamples = [System.Collections.Generic.List[object]]::new()
                    $m3StageSamples = [System.Collections.Generic.List[object]]::new()
                    $m3Phase0Samples = [System.Collections.Generic.List[object]]::new()
                    $m3ResidualSamples = [System.Collections.Generic.List[object]]::new()
                    $m3PreparedSamples = [System.Collections.Generic.List[object]]::new()
                    $m3WhispySamples = [System.Collections.Generic.List[object]]::new()
                    $task29GxMetaSamples = [System.Collections.Generic.List[object]]::new()
                    $task29GxClassSamples = [System.Collections.Generic.List[object]]::new()
                    $task29GxOwnerSamples = [System.Collections.Generic.List[object]]::new()
                    $task34StageMetaSamples = [System.Collections.Generic.List[object]]::new()
                    $task34StageEntrySamples = [System.Collections.Generic.List[object]]::new()
                    $task34StageWordSamples = [System.Collections.Generic.List[object]]::new()
                    $phase05Samples = [System.Collections.Generic.List[object]]::new()
                    $wallRunSamples = [System.Collections.Generic.List[object]]::new()
                    $m4WaterStillSamples = [System.Collections.Generic.List[object]]::new()
                    $m4StaticSamples = [System.Collections.Generic.List[object]]::new()
                    $m4FenceSamples = [System.Collections.Generic.List[object]]::new()
                    $semanticSamples = [System.Collections.Generic.List[object]]::new()
                    $ownerSamples = @(
                        [System.Collections.Generic.List[object]]::new(),
                        [System.Collections.Generic.List[object]]::new(),
                        [System.Collections.Generic.List[object]]::new()
                    )
                    $m2Samples = @(
                        [System.Collections.Generic.List[object]]::new(),
                        [System.Collections.Generic.List[object]]::new()
                    )
                    $m2CombinedSamples =
                        [System.Collections.Generic.List[object]]::new()
                    $m2ShadeSamples =
                        [System.Collections.Generic.List[object]]::new()
                    $logicTickResetCount = 0
                    if ($RendererProfileLevel -ge 1) {
                        Assert-Condition ($coarseBenchmark.Count -eq $RendererBenchmarkSamples) "Coarse renderer benchmark captured $($coarseBenchmark.Count) of $RendererBenchmarkSamples synchronized frames." $gdbStdout
                        Assert-Condition ($gxBoundaryBenchmark.Count -eq $RendererBenchmarkSamples) "GX boundary benchmark captured $($gxBoundaryBenchmark.Count) of $RendererBenchmarkSamples synchronized records." $gdbStdout
                        Assert-Condition ($stage0Benchmark.Count -eq $RendererBenchmarkSamples) "Stage layer-0 benchmark captured $($stage0Benchmark.Count) of $RendererBenchmarkSamples synchronized records." $gdbStdout
                        if ($RendererM2DetailedLedger) {
                            Assert-Condition ($m2Benchmark.Count -eq (2 * $RendererBenchmarkSamples)) "M2 renderer benchmark captured $($m2Benchmark.Count) of $(2 * $RendererBenchmarkSamples) synchronized fighter records." $gdbStdout
                            Assert-Condition ($m2ShadeBenchmark.Count -eq $RendererBenchmarkSamples) "M2 shade census captured $($m2ShadeBenchmark.Count) of $RendererBenchmarkSamples synchronized records." $gdbStdout
                        }
                        if ($RendererProfileLevel -ge 2) {
                            Assert-Condition ($ownerBenchmark.Count -eq (3 * $RendererBenchmarkSamples)) "Owner renderer benchmark captured $($ownerBenchmark.Count) of $(3 * $RendererBenchmarkSamples) synchronized owner records." $gdbStdout
                            Assert-Condition ($rendererSemanticBenchmark.Count -eq $RendererBenchmarkSamples) "Renderer semantic benchmark captured $($rendererSemanticBenchmark.Count) of $RendererBenchmarkSamples synchronized records." $gdbStdout
                        }
                        $coarseSamples = [System.Collections.Generic.List[object]]::new()
                        $gxBoundarySamples = [System.Collections.Generic.List[object]]::new()
                        $stage0Samples = [System.Collections.Generic.List[object]]::new()
                        $sinkSamples = [System.Collections.Generic.List[object]]::new()
                        $warmSamples = [System.Collections.Generic.List[object]]::new()
                        $texturePhaseSamples = [System.Collections.Generic.List[object]]::new()
                        $fastRunSamples = [System.Collections.Generic.List[object]]::new()
                        $m3StageSamples = [System.Collections.Generic.List[object]]::new()
                        $m3GeneratedSegment0Samples = [System.Collections.Generic.List[object]]::new()
                        $m3GeneratedSegment0ShadowSamples = [System.Collections.Generic.List[object]]::new()
                        $m3GeneratedSegment0GxSamples = [System.Collections.Generic.List[object]]::new()
                        $m3GeneratedSegment0TraceWordValues = @()
                        $m3GeneratedSegment0TraceRunValues = @()
                        $m3Phase0Samples = [System.Collections.Generic.List[object]]::new()
                        $m3ResidualSamples = [System.Collections.Generic.List[object]]::new()
                        $m3PreparedSamples = [System.Collections.Generic.List[object]]::new()
                        $m3WhispySamples = [System.Collections.Generic.List[object]]::new()
                        $g2StateSamples = [System.Collections.Generic.List[object]]::new()
                        $task36HwSamples = [System.Collections.Generic.List[object]]::new()
                        $task36ReplaySamples = [System.Collections.Generic.List[object]]::new()
                        $task29GxMetaSamples = [System.Collections.Generic.List[object]]::new()
                        $task29GxClassSamples = [System.Collections.Generic.List[object]]::new()
                        $task29GxOwnerSamples = [System.Collections.Generic.List[object]]::new()
                        $task34StageMetaSamples = [System.Collections.Generic.List[object]]::new()
                        $task34StageEntrySamples = [System.Collections.Generic.List[object]]::new()
                        $task34StageWordSamples = [System.Collections.Generic.List[object]]::new()
                        $m4WaterStillSamples = [System.Collections.Generic.List[object]]::new()
                        $m4StaticSamples = [System.Collections.Generic.List[object]]::new()
                        $m4FenceSamples = [System.Collections.Generic.List[object]]::new()
                        $semanticSamples = [System.Collections.Generic.List[object]]::new()
                        $logicTickResetCount = 0
                        $drawResidualRatios = @()
                        $presentResidualRatios = @()
                        $loopResidualRatios = @()
                        $ownerSamples = @(
                            [System.Collections.Generic.List[object]]::new(),
                            [System.Collections.Generic.List[object]]::new(),
                            [System.Collections.Generic.List[object]]::new()
                        )
                        $m2Samples = @(
                            [System.Collections.Generic.List[object]]::new(),
                            [System.Collections.Generic.List[object]]::new()
                        )
                        $m2CombinedSamples =
                            [System.Collections.Generic.List[object]]::new()
                        $m2ShadeSamples =
                            [System.Collections.Generic.List[object]]::new()
                        if ($benchmarkMakeIdentity.RendererBenchmarkMode -eq 2) {
                            Assert-Condition ($sinkBenchmark.Count -eq $RendererBenchmarkSamples) "CPU_PREP_NO_GX sink benchmark captured $($sinkBenchmark.Count) of $RendererBenchmarkSamples synchronized records." $gdbStdout
                            if ($RendererFastRunMode -eq 9) {
                                Assert-Condition ($m3GeneratedSegment0GxBenchmark.Count -eq $RendererBenchmarkSamples) "Task 26 GX sink captured $($m3GeneratedSegment0GxBenchmark.Count) of $RendererBenchmarkSamples synchronized records." $gdbStdout
                                Assert-Condition ($m3GeneratedSegment0TraceSummary.Count -eq 1) "Task 26 exact trace captured $($m3GeneratedSegment0TraceSummary.Count) terminal summaries." $gdbStdout
                                $m3GeneratedSegment0TraceSummaryValues = Get-Ints $m3GeneratedSegment0TraceSummary[0]
                                Assert-Condition (
                                    $m3GeneratedSegment0TraceSummaryValues[0] -eq
                                        $benchFrames[-1] -and
                                    $m3GeneratedSegment0TraceSummaryValues[1] -gt 0 -and
                                    $m3GeneratedSegment0TraceSummaryValues[1] -le 3072 -and
                                    $m3GeneratedSegment0TraceWords.Count -eq
                                        $m3GeneratedSegment0TraceSummaryValues[1] -and
                                    $m3GeneratedSegment0TraceRuns.Count -eq 26
                                ) 'Task 26 exact typed trace is incomplete, oversized, or not synchronized to the terminal sample.' $gdbStdout
                                $m3GeneratedSegment0TraceWordValues = @(
                                    $m3GeneratedSegment0TraceWords |
                                        ForEach-Object { , @(Get-Ints $_) })
                                for ($traceIndex = 0;
                                     $traceIndex -lt $m3GeneratedSegment0TraceWordValues.Count;
                                     $traceIndex++) {
                                    Assert-Condition (
                                        $m3GeneratedSegment0TraceWordValues[$traceIndex][0] -eq
                                            $traceIndex
                                    ) "Task 26 exact trace index drifted at word $traceIndex." $gdbStdout
                                }
                                $m3GeneratedSegment0TraceRunValues = @(
                                    $m3GeneratedSegment0TraceRuns |
                                        ForEach-Object { , @(Get-Ints $_) })
                                for ($runIndex = 0;
                                     $runIndex -lt $m3GeneratedSegment0TraceRunValues.Count;
                                     $runIndex++) {
                                    $runTrace = $m3GeneratedSegment0TraceRunValues[$runIndex]
                                    $previousRunWords = if ($runIndex -eq 0) {
                                        0
                                    } else {
                                        $m3GeneratedSegment0TraceRunValues[$runIndex - 1][1]
                                    }
                                    Assert-Condition (
                                        $runTrace[0] -eq $runIndex -and
                                        $runTrace[1] -gt $previousRunWords -and
                                        $runTrace[1] -le
                                            $m3GeneratedSegment0TraceSummaryValues[1] -and
                                        $runTrace[4] -gt 0 -and
                                        $runTrace[5] -ge 1 -and
                                        $runTrace[5] -le 22 -and
                                        $runTrace[6] -ne 0 -and
                                        $runTrace[6] -ne 0xffffffff -and
                                        $runTrace[8] -ne 0 -and
                                        $runTrace[9] -ne 0 -and
                                        $runTrace[10] -ne 0
                                    ) "Task 26 run checkpoint $runIndex is incomplete or lacks an actual normalized texture descriptor." $gdbStdout
                                }
                                Assert-Condition (
                                    $m3GeneratedSegment0TraceRunValues[-1][1] -eq
                                        $m3GeneratedSegment0TraceSummaryValues[1]
                                ) 'Task 26 final run checkpoint does not close the exact segment trace.' $gdbStdout
                            }
                        }
                        if ($benchmarkMakeIdentity.RendererBenchmarkMode -eq 4) {
                            Assert-Condition ($warmBenchmark.Count -eq $RendererBenchmarkSamples) "WARM_NO_UPLOAD benchmark captured $($warmBenchmark.Count) of $RendererBenchmarkSamples synchronized records." $gdbStdout
                        }
                        Assert-Condition ($fastRunBenchmark.Count -eq $RendererBenchmarkSamples) "Fast-run benchmark captured $($fastRunBenchmark.Count) of $RendererBenchmarkSamples synchronized records." $gdbStdout
                        if ($RendererFastRunMode -eq 9) {
                            Assert-Condition ($m3StageBenchmark.Count -eq $RendererBenchmarkSamples) "M3 stage benchmark captured $($m3StageBenchmark.Count) of $RendererBenchmarkSamples synchronized records." $gdbStdout
                        if ($Task36HwComposeMode -gt 0) {
                            Assert-Condition ($task36HwBenchmark.Count -eq $RendererBenchmarkSamples) "Task 36 hardware-compose benchmark captured $($task36HwBenchmark.Count) of $RendererBenchmarkSamples synchronized records." $gdbStdout
                            if ($Task36HwComposeMode -eq 2) {
                                Assert-Condition ($task36ReplayBenchmark.Count -eq $RendererBenchmarkSamples) "Task 36 replay benchmark captured $($task36ReplayBenchmark.Count) of $RendererBenchmarkSamples synchronized records." $gdbStdout
                            }
                        }
                        if ($benchmarkMakeIdentity.NativeStageGeneratedSegment0Enable -eq 1) {
                            Assert-Condition ($m3GeneratedSegment0Benchmark.Count -eq $RendererBenchmarkSamples) "Task 26 generated segment-0 benchmark captured $($m3GeneratedSegment0Benchmark.Count) of $RendererBenchmarkSamples synchronized records." $gdbStdout
                            if ($RendererM3Phase0Profile) {
                                Assert-Condition ($m3GeneratedSegment0ShadowBenchmark.Count -eq $RendererBenchmarkSamples) "Task 26 segment-0 shadow captured $($m3GeneratedSegment0ShadowBenchmark.Count) of $RendererBenchmarkSamples synchronized records." $gdbStdout
                            }
                        }
                        if ($RendererM3Phase0Profile) {
                            Assert-Condition ($m3Phase0Benchmark.Count -eq $RendererBenchmarkSamples) "M3 Phase-0 benchmark captured $($m3Phase0Benchmark.Count) of $RendererBenchmarkSamples synchronized records." $gdbStdout
                            Assert-Condition ($m3ResidualBenchmark.Count -eq $RendererBenchmarkSamples) "Task 23R residual benchmark captured $($m3ResidualBenchmark.Count) of $RendererBenchmarkSamples synchronized records." $gdbStdout
                            Assert-Condition ($m3PreparedBenchmark.Count -eq $RendererBenchmarkSamples) "M3 prepared-output stability census captured $($m3PreparedBenchmark.Count) of $RendererBenchmarkSamples synchronized records." $gdbStdout
                            Assert-Condition ($m3WhispyBenchmark.Count -eq $RendererBenchmarkSamples) "M3 Whispy source-state census captured $($m3WhispyBenchmark.Count) of $RendererBenchmarkSamples synchronized records." $gdbStdout
                            Assert-Condition ($g2StateBenchmark.Count -eq $RendererBenchmarkSamples) "G2 state benchmark captured $($g2StateBenchmark.Count) of $RendererBenchmarkSamples synchronized records." $gdbStdout
                            Assert-Condition ($phase05Benchmark.Count -eq $RendererBenchmarkSamples) "Phase-0.5 benchmark captured $($phase05Benchmark.Count) of $RendererBenchmarkSamples synchronized records." $gdbStdout
                            Assert-Condition ($wallRunBenchmark.Count -eq $RendererBenchmarkSamples) "Wallpaper run census captured $($wallRunBenchmark.Count) of $RendererBenchmarkSamples synchronized records." $gdbStdout
                        }
                        }
                        if ($m4CandidateEvidence) {
                            Assert-Condition ($m4WaterStillBenchmark.Count -eq $RendererBenchmarkSamples) "M4 still-water benchmark captured $($m4WaterStillBenchmark.Count) of $RendererBenchmarkSamples synchronized records." $gdbStdout
                        }
                        Assert-Condition ($m4StaticBenchmark.Count -eq $RendererBenchmarkSamples) "M4 static-texture benchmark captured $($m4StaticBenchmark.Count) of $RendererBenchmarkSamples synchronized records." $gdbStdout
                        Assert-Condition ($m4FenceBenchmark.Count -eq $RendererBenchmarkSamples) "M4 post-GO texture fence captured $($m4FenceBenchmark.Count) of $RendererBenchmarkSamples synchronized records." $gdbStdout
                        Assert-Condition ($texturePhaseBenchmark.Count -eq $RendererBenchmarkSamples) "Texture-phase benchmark captured $($texturePhaseBenchmark.Count) of $RendererBenchmarkSamples synchronized records." $gdbStdout
                        if ($Task29GXCensus) {
                            Assert-Condition (
                                $task29GxMetaBenchmark.Count -eq $RendererBenchmarkSamples -and
                                $task29GxClassBenchmark.Count -eq $RendererBenchmarkSamples -and
                                $task29GxOwnerBenchmark.Count -eq
                                    (4 * $RendererBenchmarkSamples)
                            ) "Task 29 GX census is incomplete (meta=$($task29GxMetaBenchmark.Count) class=$($task29GxClassBenchmark.Count) owner=$($task29GxOwnerBenchmark.Count))." $gdbStdout
                        }
                        if ($Task34StageStreamCensus) {
                            Assert-Condition (
                                $task34StageMetaBenchmark.Count -eq
                                    $RendererBenchmarkSamples -and
                                $task34StageEntryValues.Count -gt 0 -and
                                $task34StageWordValues.Count -gt 0
                            ) "Task 34 E1 raw stream census is incomplete (meta=$($task34StageMetaBenchmark.Count) entries=$($task34StageEntryValues.Count) words=$($task34StageWordValues.Count))." $gdbStdout
                        }
                        $expectedLogicTickDelta = if (
                            $BattlePlayable -and $RealtimePresentation -and
                            ($ExpectedMode -eq 163)) {
                            2
                        } else {
                            1
                        }
                        for ($sampleIndex = 0; $sampleIndex -lt $RendererBenchmarkSamples; $sampleIndex++) {
                            $coarse = Get-Ints $coarseBenchmark[$sampleIndex]
                            $render = Get-Ints $rendererBenchmark[$sampleIndex]
                            $frame = $coarse[0]
                            $loopWall = $coarse[1]
                            $update = $coarse[3]
                            $sourceUpdate = $coarse[4]
                            $audioUpdate = $coarse[5]
                            $presentActive = $coarse[6]
                            $vblankWait = $coarse[7]
                            $drawTicks = $coarse[9]
                            $drawKnown = $coarse[10] + $coarse[11] + $coarse[12] + $coarse[13] + $coarse[14]
                            $presentKnown = $coarse[8] + $drawTicks + $coarse[15] + $coarse[16] + $coarse[17] + $coarse[18]
                            $loopKnown = $coarse[2] + $update + $presentActive + $vblankWait
                            $expectedPresentActive = if ($render[2] -ge $vblankWait) { $render[2] - $vblankWait } else { 0 }
                            $expectedDrawResidual = if ($drawTicks -ge $drawKnown) { $drawTicks - $drawKnown } else { 0 }
                            $expectedPresentResidual = if ($presentActive -ge $presentKnown) { $presentActive - $presentKnown } else { 0 }
                            $expectedLoopResidual = if ($loopWall -ge $loopKnown) { $loopWall - $loopKnown } else { 0 }
                            $expectedConservationError = [int64]0
                            if ($vblankWait -gt $render[2]) { $expectedConservationError += $vblankWait - $render[2] }
                            if ($drawKnown -gt $drawTicks) { $expectedConservationError += $drawKnown - $drawTicks }
                            if ($presentKnown -gt $presentActive) { $expectedConservationError += $presentKnown - $presentActive }
                            if ($loopKnown -gt $loopWall) { $expectedConservationError += $loopKnown - $loopWall }

                            Assert-Condition ($frame -eq $render[1] -and $drawTicks -eq $render[3]) "Coarse renderer benchmark frame $frame is not synchronized with RENDER_BENCH." $gdbStdout
                            if ($Task29GXCensus) {
                                $task29Meta = Get-Ints $task29GxMetaBenchmark[$sampleIndex]
                                $task29Class = Get-Ints $task29GxClassBenchmark[$sampleIndex]
                                $task29ClassCommandSum = [int64]0
                                $task29ClassWordSum = [int64]0
                                $task29ClassRepeatSum = [int64]0
                                $task29OwnerCommandSums = @(0L, 0L, 0L, 0L)
                                $task29OwnerWordSums = @(0L, 0L, 0L, 0L)
                                $task29OwnerRepeatSums = @(0L, 0L, 0L, 0L)
                                $task29OwnerClassCommandSums = @(0L) * 22
                                $task29OwnerClassWordSums = @(0L) * 22
                                $task29OwnerClassRepeatSums = @(0L) * 22
                                $task29FastRun = Get-Ints `
                                    $fastRunBenchmark[$sampleIndex]

                                Assert-Condition (
                                    $task29Meta[0] -eq $frame -and
                                    $task29Class[0] -eq $frame -and
                                    $task29Meta[1] -gt 0 -and
                                    $task29Meta[2] -ge $task29Meta[1] -and
                                    $task29Meta[3] -le $task29Meta[1] -and
                                    $task29Meta[4] -ne 0 -and
                                    $task29Meta[5] -ne 0 -and
                                    $task29Meta[6] -ne 0 -and
                                    $task29Meta[7] -ne 0 -and
                                    $task29Meta[8] -gt 0 -and
                                    $task29Meta[9] -eq 0 -and
                                    $task29Meta[10] -eq 3374912
                                ) "Task 29 GX meta census failed synchronization, hash, boundary, fault, or side-effect classification at frame $frame (actual=$($task29Meta -join ','))." $gdbStdout

                                for ($task29ClassIndex = 0;
                                     $task29ClassIndex -lt 22;
                                     $task29ClassIndex++) {
                                    $task29ClassBase = 1 + (3 * $task29ClassIndex)
                                    $task29Commands = [int64]$task29Class[$task29ClassBase]
                                    $task29Words = [int64]$task29Class[$task29ClassBase + 1]
                                    $task29Repeats = [int64]$task29Class[$task29ClassBase + 2]
                                    Assert-Condition (
                                        $task29Words -ge $task29Commands -and
                                        $task29Repeats -le $task29Commands
                                    ) "Task 29 GX class $task29ClassIndex has invalid command/word/repeat conservation at frame $frame." $gdbStdout
                                    $task29ClassCommandSum += $task29Commands
                                    $task29ClassWordSum += $task29Words
                                    $task29ClassRepeatSum += $task29Repeats
                                }
                                Assert-Condition (
                                    $task29ClassCommandSum -eq $task29Meta[1] -and
                                    $task29ClassWordSum -eq $task29Meta[2] -and
                                    $task29ClassRepeatSum -eq $task29Meta[3] -and
                                    $task29Class[1 + (3 * 16)] -gt 0 -and
                                    $task29Class[1 + (3 * 20)] -eq
                                        (3 * $render[11]) -and
                                    $task29Class[2 + (3 * 20)] -eq
                                        (6 * $render[11]) -and
                                    $task29Class[1 + (3 * 21)] -eq 1
                                ) "Task 29 GX class totals no longer conserve the synchronized actual-triangle stream and one flush at frame $frame." $gdbStdout

                                for ($task29OwnerIndex = 0;
                                     $task29OwnerIndex -lt 4;
                                     $task29OwnerIndex++) {
                                    $task29Owner = Get-Ints $task29GxOwnerBenchmark[
                                        (4 * $sampleIndex) + $task29OwnerIndex]
                                    Assert-Condition (
                                        $task29Owner[0] -eq $frame -and
                                        $task29Owner[1] -eq $task29OwnerIndex -and
                                        $task29Owner[2] -ne 0 -and
                                        $task29Owner[3] -ne 0
                                    ) "Task 29 GX owner $task29OwnerIndex is not synchronized or hashed at frame $frame." $gdbStdout
                                    for ($task29ClassIndex = 0;
                                         $task29ClassIndex -lt 22;
                                         $task29ClassIndex++) {
                                        $task29OwnerBase = 4 + (3 * $task29ClassIndex)
                                        $task29OwnerCommands = [int64]$task29Owner[$task29OwnerBase]
                                        $task29OwnerWords = [int64]$task29Owner[$task29OwnerBase + 1]
                                        $task29OwnerRepeats = [int64]$task29Owner[$task29OwnerBase + 2]
                                        $task29OwnerCommandSums[$task29OwnerIndex] +=
                                            $task29OwnerCommands
                                        $task29OwnerWordSums[$task29OwnerIndex] +=
                                            $task29OwnerWords
                                        $task29OwnerRepeatSums[$task29OwnerIndex] +=
                                            $task29OwnerRepeats
                                        $task29OwnerClassCommandSums[$task29ClassIndex] +=
                                            $task29OwnerCommands
                                        $task29OwnerClassWordSums[$task29ClassIndex] +=
                                            $task29OwnerWords
                                        $task29OwnerClassRepeatSums[$task29ClassIndex] +=
                                            $task29OwnerRepeats
                                    }
                                    $task29GxOwnerSamples.Add($task29Owner)
                                }
                                for ($task29ClassIndex = 0;
                                     $task29ClassIndex -lt 22;
                                     $task29ClassIndex++) {
                                    $task29ClassBase = 1 + (3 * $task29ClassIndex)
                                    Assert-Condition (
                                        $task29OwnerClassCommandSums[$task29ClassIndex] -eq
                                            $task29Class[$task29ClassBase] -and
                                        $task29OwnerClassWordSums[$task29ClassIndex] -eq
                                            $task29Class[$task29ClassBase + 1] -and
                                        $task29OwnerClassRepeatSums[$task29ClassIndex] -eq
                                            $task29Class[$task29ClassBase + 2]
                                    ) "Task 29 GX owner partition does not conserve class $task29ClassIndex at frame $frame." $gdbStdout
                                }
                                Assert-Condition (
                                    $task29OwnerClassCommandSums[20] -eq
                                        (3 * $render[11]) -and
                                    (Get-Ints $task29GxOwnerBenchmark[(4 * $sampleIndex)])[64] -eq
                                        (3 * ($render[11] - $task29FastRun[5] -
                                              $task29FastRun[6])) -and
                                    (Get-Ints $task29GxOwnerBenchmark[(4 * $sampleIndex) + 1])[64] -eq
                                        (3 * $task29FastRun[5]) -and
                                    (Get-Ints $task29GxOwnerBenchmark[(4 * $sampleIndex) + 2])[64] -eq
                                        (3 * $task29FastRun[6]) -and
                                    (Get-Ints $task29GxOwnerBenchmark[(4 * $sampleIndex) + 3])[64] -eq 0
                                ) "Task 29 GX vertex ownership diverged from the synchronized fast-owner lifecycle or failed to assign all residual triangles to the stage at frame $frame." $gdbStdout
                                $task29GxMetaSamples.Add($task29Meta)
                                $task29GxClassSamples.Add($task29Class)
                            }
                            if ($Task34StageStreamCensus) {
                                $task34Meta = Get-Ints `
                                    $task34StageMetaBenchmark[$sampleIndex]
                                $task34FrameEntries = @(
                                    $task34StageEntryValues | Where-Object {
                                        $_[0] -eq $frame
                                    })
                                $task34FrameWords = @(
                                    $task34StageWordValues | Where-Object {
                                        $_[0] -eq $frame
                                    })
                                Assert-Condition (
                                    $task34Meta[0] -eq $frame -and
                                    $task34Meta[1] -gt 0 -and
                                    $task34Meta[2] -gt 0 -and
                                    $task34Meta[3] -eq 0 -and
                                    $task34Meta[4] -eq 0 -and
                                    $task34FrameEntries.Count -eq
                                        $task34Meta[1] -and
                                    $task34FrameWords.Count -eq
                                        $task34Meta[2]
                                ) "Task 34 standalone native-stage stream is empty or hit an overflow/fault at frame $frame (meta=$($task34Meta -join ','))." $gdbStdout
                                $task34WordCursor = 0
                                for ($task34EntryIndex = 0;
                                     $task34EntryIndex -lt
                                        $task34FrameEntries.Count;
                                     $task34EntryIndex++) {
                                    $task34Entry =
                                        $task34FrameEntries[$task34EntryIndex]
                                    Assert-Condition (
                                        $task34Entry[1] -eq
                                            $task34EntryIndex -and
                                        $task34Entry[2] -lt 57 -and
                                        $task34Entry[3] -lt 22 -and
                                        $task34Entry[4] -eq
                                            $task34WordCursor -and
                                        $task34Entry[5] -le 16 -and
                                        $task34Entry[6] -lt 8
                                    ) "Task 34 entry $task34EntryIndex is out of order, unowned, or out of range at frame $frame (actual=$($task34Entry -join ','))." $gdbStdout
                                    $task34WordCursor += $task34Entry[5]
                                }
                                Assert-Condition (
                                    $task34WordCursor -eq $task34Meta[2]
                                ) "Task 34 entry word spans do not close the frame-$frame stream." $gdbStdout
                                for ($task34WordIndex = 0;
                                     $task34WordIndex -lt
                                        $task34FrameWords.Count;
                                     $task34WordIndex++) {
                                    Assert-Condition (
                                        $task34FrameWords[$task34WordIndex][1] -eq
                                            $task34WordIndex
                                    ) "Task 34 word $task34WordIndex is out of order at frame $frame." $gdbStdout
                                }
                                $task34StageMetaSamples.Add($task34Meta)
                                foreach ($task34Entry in $task34FrameEntries) {
                                    $task34StageEntrySamples.Add($task34Entry)
                                }
                                foreach ($task34Word in $task34FrameWords) {
                                    $task34StageWordSamples.Add($task34Word)
                                }
                            }
                            if ($sampleIndex -gt 0) {
                                $previousCoarse = Get-Ints $coarseBenchmark[$sampleIndex - 1]
                                $logicTickContinues =
                                    $coarse[23] -eq (
                                        $previousCoarse[23] +
                                        $expectedLogicTickDelta)
                                # BattleShip ifcommon.c:3173-3178 starts the
                                # one-minute timer after the Ready/Go wait by
                                # resetting the scheduler tic counter once.
                                $logicTimerStartReset =
                                    ($coarse[23] -eq 1) -and
                                    ($previousCoarse[23] -ge 300) -and
                                    ($logicTickResetCount -eq 0)
                                if ($logicTimerStartReset) {
                                    $logicTickResetCount++
                                }
                                Assert-Condition ($frame -eq ($previousCoarse[0] + 1) -and ($logicTickContinues -or $logicTimerStartReset)) "Coarse renderer benchmark frame/logic window is not contiguous at the required $expectedLogicTickDelta source ticks per present or the single source timer-start reset at frame $frame tick $($coarse[23])." $gdbStdout
                            }
                            Assert-Condition (($sourceUpdate + $audioUpdate) -le $update) "Coarse renderer benchmark update subphases exceed update wall time at frame $frame." $gdbStdout
                            Assert-Condition (($coarse[24] + $coarse[25]) -eq $sourceUpdate -and $coarse[26] -ge 2) "Coarse renderer benchmark did not retain two individually timed source updates and an exact presentation interval at frame $frame." $gdbStdout
                            Assert-Condition ($presentActive -eq $expectedPresentActive -and $coarse[19] -eq $expectedDrawResidual -and $coarse[20] -eq $expectedPresentResidual -and $coarse[21] -eq $expectedLoopResidual -and $coarse[22] -eq $expectedConservationError) "Coarse renderer benchmark residual equations failed at frame $frame." $gdbStdout
                            Assert-Condition ($loopWall -gt 0 -and ($coarse[22] * 100) -le ($loopWall * 2)) "Coarse renderer benchmark conservation error exceeded 2 percent at frame $frame." $gdbStdout
                            Assert-Condition (($presentActive -eq 0) -or (($coarse[20] * 100) -le ($presentActive * 2))) "Coarse renderer benchmark present residual exceeded 2 percent at frame $frame." $gdbStdout

                            $gxBoundary = Get-Ints $gxBoundaryBenchmark[$sampleIndex]
                            Assert-Condition ($gxBoundary[0] -eq $frame -and $gxBoundary[4] -ne 0 -and $gxBoundary[5] -ne 0 -and $gxBoundary[6] -eq $coarse[16]) "GX boundary benchmark is not synchronized or lacks a post-VBlank completion sample at coarse frame $frame." $gdbStdout
                            $gxBoundarySamples.Add($gxBoundary)
                            $stage0 = Get-Ints $stage0Benchmark[$sampleIndex]
                            Assert-Condition (
                                $stage0[0] -eq $frame -and
                                ((($RendererFastRunMode -eq 9) -and
                                  ($stage0[1] -eq 0)) -or
                                 (($RendererFastRunMode -ne 9) -and
                                  ($stage0[1] -gt 0)))
                            ) "Stage layer-0 benchmark did not match the selected owner at frame $frame (actual=$($stage0 -join ','))." $gdbStdout
                            $stage0Samples.Add($stage0)
                            if ($benchmarkMakeIdentity.RendererBenchmarkMode -eq 2) {
                                $sink = Get-Ints $sinkBenchmark[$sampleIndex]
                                $sinkOwnerWords = $sink[5] + $sink[6] + $sink[7]
                                Assert-Condition ($sink[0] -eq $frame -and $sink[1] -eq $sink[2] -and $sink[2] -gt 0 -and $sink[3] -eq 1024 -and $sink[4] -gt 0 -and $sinkOwnerWords -eq $sink[2]) "CPU_PREP_NO_GX sink record is not synchronized, owner-conserved, or calibrated at frame $frame." $gdbStdout
                                $sinkSamples.Add($sink)
                                if ($RendererFastRunMode -eq 9) {
                                    $m3GeneratedSegment0Gx = Get-Ints `
                                        $m3GeneratedSegment0GxBenchmark[$sampleIndex]
                                    Assert-Condition (
                                        $m3GeneratedSegment0Gx[0] -eq $frame -and
                                        $m3GeneratedSegment0Gx[1] -eq $sink[2] -and
                                        $m3GeneratedSegment0Gx[4] -gt 0 -and
                                        $m3GeneratedSegment0Gx[4] -le $sink[2] -and
                                        $m3GeneratedSegment0Gx[7] -eq 0
                                    ) "Task 26 GX-word sink is not synchronized, bounded to segment 0, or balanced at frame $frame (actual=$($m3GeneratedSegment0Gx -join ','))." $gdbStdout
                                    $m3GeneratedSegment0GxSamples.Add(
                                        $m3GeneratedSegment0Gx)
                                }
                            }
                            if ($benchmarkMakeIdentity.RendererBenchmarkMode -eq 4) {
                                $warm = Get-Ints $warmBenchmark[$sampleIndex]
                                Assert-Condition ($warm[0] -eq $frame -and (Test-RendererUploadPair $warm[1] $warm[2])) "WARM_NO_UPLOAD sampled an invalid suppressed refresh pair $($warm[1])/$($warm[2]) at frame $frame." $gdbStdout
                                $warmSamples.Add($warm)
                            }
                            $texturePhase = Get-Ints $texturePhaseBenchmark[$sampleIndex]
                            Assert-Condition ($texturePhase[0] -eq $frame -and (Test-RendererUploadPair $texturePhase[3] $texturePhase[4])) "Texture-phase benchmark is not synchronized or sampled invalid uploads at frame $frame." $gdbStdout
                            if ($RendererProfileLevel -ge 2) {
                                Assert-Condition ($texturePhase[6] -eq 0 -and (($texturePhase[3] -eq 0 -and $texturePhase[5] -eq 0) -or ($texturePhase[3] -gt 0 -and $texturePhase[5] -gt 0))) "Palette-pair texture oracle failed or did not track the sampled refreshes at frame $frame." $gdbStdout
                            }
                            if (($RendererProfileLevel -lt 2) -and ($benchmarkMakeIdentity.RendererBenchmarkMode -eq 0) -and ($texturePhase[3] -gt 0)) {
                                Assert-Condition ($texturePhase[7] -eq $texturePhase[3] -and $texturePhase[8] -eq $texturePhase[4] -and $texturePhase[9] -eq $texturePhase[3] -and $texturePhase[11] -eq 0 -and $texturePhase[12] -eq 0 -and $texturePhase[13] -ge 192 -and $texturePhase[14] -ge 192) "Animated texture refresh did not queue and commit exactly inside VBlank at frame $frame." $gdbStdout
                            } elseif ($RendererProfileLevel -ge 2) {
                                Assert-Condition ($texturePhase[7] -eq 0 -and $texturePhase[8] -eq 0 -and $texturePhase[9] -eq 0 -and $texturePhase[11] -eq 0 -and $texturePhase[12] -eq 0) "Forensic texture oracle unexpectedly allocated or executed the shipping VBlank staging path at frame $frame." $gdbStdout
                            }
                            $texturePhaseSamples.Add($texturePhase)
                            $fastRun = Get-Ints $fastRunBenchmark[$sampleIndex]
                            Assert-Condition ($fastRun[0] -eq $frame -and $fastRun[1] -eq $RendererFastRunMode -and ($fastRun[4] + $fastRun[5] + $fastRun[6]) -eq $fastRun[3]) "Fast-run accounting is not synchronized, in the selected mode, or owner-conserved at frame $frame." $gdbStdout
                            if ($RendererFastRunMode -eq 0) {
                                Assert-Condition ($fastRun[2] -eq 0 -and $fastRun[3] -eq 0) "Generic laboratory mode unexpectedly executed fast runs at frame $frame." $gdbStdout
                            } elseif (($RendererProfileLevel -eq 1) -and
                                      ($RendererFastRunMode -in @(7, 8)) -and
                                      ($benchmarkMakeIdentity.RendererBenchmarkMode -eq 1)) {
                                # TRIANGLE_NOOP deliberately removes the three
                                # stage fast-submit runs. The generated fighter
                                # owner remains the measured subject: 67 runs,
                                # all 626 Mario/Fox triangles, and no fighter
                                # fallback. Stage state rejects are expected.
                                Assert-Condition ($fastRun[2] -eq 67 -and $fastRun[3] -eq 626 -and $fastRun[4] -eq 0 -and $fastRun[5] -eq 320 -and $fastRun[6] -eq 306 -and $fastRun[7] -eq 59 -and $fastRun[8] -eq 0 -and $fastRun[9] -eq 0) "TRIANGLE_NOOP did not preserve the exact 67-run/626-triangle generated fighter-owner floor and 0/320/306 owner partition at frame $frame (actual=$($fastRun -join ','))." $gdbStdout
                            } elseif (($RendererProfileLevel -eq 1) -and
                                      ($RendererFastRunMode -in @(7, 8))) {
                                Assert-Condition ($fastRun[2] -eq 70 -and $fastRun[3] -eq 686 -and $fastRun[4] -eq 60 -and $fastRun[5] -eq 320 -and $fastRun[6] -eq 306 -and $fastRun[7] -eq 29 -and $fastRun[8] -eq 0 -and $fastRun[9] -eq 0) "Production native fighter owner did not preserve exact 70-run/686-triangle accounting, 60/320/306 owner partition, and 29/0/0 fallback partition at frame $frame (actual=$($fastRun -join ','))." $gdbStdout
                            } elseif (($RendererProfileLevel -eq 1) -and
                                      ($RendererFastRunMode -eq 9)) {
                                if ($naturalScreenSpaceCensusWindow) {
                                    $naturalFighterTrianglesValid =
                                        ($fastRun[5] -in @(0, 320)) -and
                                        ($fastRun[6] -in @(0, 306))
                                    $naturalNonFastTriangles =
                                        [int64]$render[11] - [int64]$fastRun[3]
                                    Assert-Condition (
                                        $fastRun[2] -ge 54 -and
                                        $fastRun[3] -eq
                                            (202 + $fastRun[5] + $fastRun[6]) -and
                                        $fastRun[4] -eq 202 -and
                                        $naturalFighterTrianglesValid -and
                                        $naturalNonFastTriangles -ge 0 -and
                                        $naturalNonFastTriangles -le 384 -and
                                        $fastRun[7] -eq 0 -and
                                        $fastRun[8] -eq 0 -and
                                        $fastRun[9] -eq 0
                                    ) "M3 natural census window lost the exact stage/whole-fighter owner packets, bounded non-fast fighter/effect geometry, or zero-fallback accounting at frame $frame (actual=$($fastRun -join ',') rendered=$($render[11]))." $gdbStdout
                                } elseif ($RendererBenchmarkStartEvent -in
                                    @('KO', 'Rebirth')) {
                                    $eventFighterTrianglesValid =
                                        ($fastRun[5] -in @(0, 320)) -and
                                        ($fastRun[6] -in @(0, 306)) -and
                                        (($fastRun[5] + $fastRun[6]) -ge 306)
                                    Assert-Condition (
                                        $fastRun[2] -gt 0 -and
                                        $fastRun[3] -ge 508 -and
                                        $fastRun[4] -eq 202 -and
                                        $eventFighterTrianglesValid -and
                                        ($render[11] - $fastRun[3]) -eq 16 -and
                                        $fastRun[7] -eq 0 -and
                                        $fastRun[8] -eq 0 -and
                                        $fastRun[9] -eq 0
                                    ) "M3 natural-event window lost the exact stage/fighter owners, 16-triangle death/rebirth effect, or zero-fallback accounting at frame $frame (actual=$($fastRun -join ',') rendered=$($render[11]))." $gdbStdout
                                } elseif ($m3StageOnlyBenchmarkWindow) {
                                    Assert-Condition ($fastRun[2] -ge 54 -and $fastRun[3] -ge 202 -and $fastRun[4] -eq 202 -and $fastRun[7] -eq 0 -and $fastRun[8] -eq 0 -and $fastRun[9] -eq 0) "M3 countdown-stage window did not preserve the exact 202-triangle stage owner, positive transient fast-owner census, and zero fallbacks at frame $frame (actual=$($fastRun -join ',') rendered=$($render[11]))." $gdbStdout
                                } elseif ($PhaseMatrixMode -or
                                    $Task22WallpaperRunLab) {
                                    Assert-Condition (
                                        $fastRun[2] -gt 0 -and
                                        $fastRun[3] -eq
                                            ($fastRun[4] + $fastRun[5] +
                                             $fastRun[6]) -and
                                        $fastRun[4] -eq 202 -and
                                        ($fastRun[5] -eq 0 -or
                                         $fastRun[5] -eq 320) -and
                                        ($fastRun[6] -eq 0 -or
                                         $fastRun[6] -eq 306) -and
                                        $render[11] -ge $fastRun[3] -and
                                        $fastRun[7] -eq 0 -and
                                        $fastRun[8] -eq 0 -and
                                        $fastRun[9] -eq 0
                                    ) "M3 live-CPU window lost exact stage/fighter owner accounting or zero-fallback behavior at frame $frame (actual=$($fastRun -join ',') rendered=$($render[11]))." $gdbStdout
                                } else {
                                    Assert-Condition ($fastRun[2] -eq 121 -and $fastRun[3] -eq 828 -and $fastRun[4] -eq 202 -and $fastRun[5] -eq 320 -and $fastRun[6] -eq 306 -and $fastRun[7] -eq 0 -and $fastRun[8] -eq 0 -and $fastRun[9] -eq 0) "M3 complete-stage owner did not preserve exact 121-run/828-triangle accounting, 202/320/306 owner partition, and zero fallbacks at frame $frame (actual=$($fastRun -join ','))." $gdbStdout
                                }
                                $m3 = Get-Ints $m3StageBenchmark[$sampleIndex]
                                Assert-Condition ($m3[0] -eq $frame -and $m3[1] -eq ($m3[2] + $m3[3]) -and $m3[2] -gt 0 -and $m3[4] -eq 8 -and $m3[5] -eq 255 -and $m3[6] -eq 0 -and $m3[7] -eq 57 -and $m3[8] -eq 42 -and $m3[9] -eq 54 -and $m3[10] -eq 202 -and $m3[11] -eq 49 -and $m3[12] -eq 4 -and $m3[13] -eq 4 -and $m3[14] -eq 5 -and $m3[15] -eq 10 -and $m3[16] -eq 15 -and $m3[17] -gt 0 -and $m3[18] -gt 0) "M3 stage owner did not preflight and commit the exact 8-segment/57-DObj/42-binding/54-run/202-triangle contract through a validated topology cache at frame $frame (actual=$($m3 -join ','))." $gdbStdout
                                if ($RendererM3Phase0Profile) {
                                    Assert-Condition ($m3[17] -ge 2 -and $m3[19] -eq 1 -and $m3[20] -eq 1 -and $m3[21] -eq 1) "M3 topology fault injection did not force exactly one successful full revalidation at frame $frame (actual=$($m3 -join ','))." $gdbStdout
                                } else {
                                    Assert-Condition ($m3[19] -eq 0 -and $m3[20] -eq 0 -and $m3[21] -eq 0) "Production M3 topology cache unexpectedly injected or observed a stamp fault at frame $frame (actual=$($m3 -join ','))." $gdbStdout
                                }
                                if ($sampleIndex -gt 0) {
                                    $previousM3 = Get-Ints $m3StageBenchmark[$sampleIndex - 1]
                                    Assert-Condition ($m3[1] -eq ($previousM3[1] + 1) -and $m3[2] -eq ($previousM3[2] + 1) -and $m3[3] -eq $previousM3[3] -and $m3[17] -eq $previousM3[17] -and $m3[18] -eq ($previousM3[18] + 1) -and $m3[19] -eq $previousM3[19] -and $m3[20] -eq $previousM3[20] -and $m3[21] -eq $previousM3[21]) "M3 stage owner did not remain continuously armed on cache hits across frame $frame (previous=$($previousM3 -join ',') actual=$($m3 -join ','))." $gdbStdout
                                }
                                $m3StageSamples.Add($m3)
                                if ($Task36HwComposeMode -gt 0) {
                                    $task36 = Get-Ints $task36HwBenchmark[$sampleIndex]
                                    $task36ExpectedDynamicLo =
                                        if ($RendererM3Phase0Profile) {
                                            0x1ff00000
                                        } else { 0 }
                                    $task36ExpectedDynamicHi =
                                        if ($RendererM3Phase0Profile) {
                                            0x7e
                                        } else { 0 }
                                    Assert-Condition (
                                        $task36[0] -eq $frame -and
                                        $task36[1] -eq 26 -and
                                        (($Task36HwComposeMode -ne 2) -or
                                         (($task36[2] -eq 3) -and
                                          ($task36[3] -eq 31))) -and
                                        $task36[2] -gt 0 -and
                                        $task36[3] -ge $task36[1] -and
                                        $task36[4] -eq 0 -and
                                        $task36[5] -eq
                                            $task36ExpectedDynamicLo -and
                                        $task36[6] -eq
                                            $task36ExpectedDynamicHi -and
                                        $task36[7] -eq 0 -and
                                        $task36[8] -eq 0 -and
                                        $task36[9] -eq 0 -and
                                        $task36[11] -eq 0
                                    ) "Task 36 hardware compose lost its rigid/dynamic partition, engagement, or zero-rejection/post-arm contract at frame $frame (actual=$($task36 -join ','))." $gdbStdout
                                    $task36HwSamples.Add($task36)
                                    if ($Task36HwComposeMode -eq 2) {
                                        $task36Replay = Get-Ints `
                                            $task36ReplayBenchmark[$sampleIndex]
                                        Assert-Condition (
                                            $task36Replay[0] -eq $frame -and
                                            $task36Replay[1] -eq 2 -and
                                            $task36Replay[2] -eq 1 -and
                                            $task36Replay[3] -eq 1 -and
                                            $task36Replay[4] -eq 0 -and
                                            $task36Replay[5] -gt 0 -and
                                            $task36Replay[6] -eq 3 -and
                                            $task36Replay[7] -eq 33 -and
                                            $task36Replay[8] -gt 0 -and
                                            $task36Replay[8] -eq
                                                $task36Replay[12] -and
                                            $task36Replay[12] -eq 3916 -and
                                            $task36Replay[9] -eq 0 -and
                                            $task36Replay[10] -eq 0 -and
                                            $task36Replay[11] -eq 0 -and
                                            $task36Replay[13] -eq 0x150000 -and
                                            $task36Replay[14] -eq 0 -and
                                            $task36Replay[15] -eq 3916 -and
                                            $task36Replay[16] -eq 0xA1 -and
                                            $task36Replay[17] -eq 0 -and
                                            $task36Replay[18] -eq 0
                                        ) "Task 36 replay lost READY/one-bake/3-segment/33-run/full-arena/zero-fallback engagement at frame $frame (actual=$($task36Replay -join ','))." $gdbStdout
                                        if ($sampleIndex -gt 0) {
                                            $previousTask36Replay = Get-Ints `
                                                $task36ReplayBenchmark[$sampleIndex - 1]
                                            Assert-Condition (
                                                $task36Replay[5] -eq
                                                    ($previousTask36Replay[5] + 1)
                                            ) "Task 36 replay did not advance exactly once across frame $frame." $gdbStdout
                                        }
                                        $task36ReplaySamples.Add(
                                            $task36Replay)
                                    }
                                }
                                if ($benchmarkMakeIdentity.NativeStageGeneratedSegment0Enable -eq 1) {
                                    $m3GeneratedSegment0 = Get-Ints `
                                        $m3GeneratedSegment0Benchmark[$sampleIndex]
                                    Assert-Condition (
                                        $m3GeneratedSegment0[0] -eq $frame -and
                                        $m3GeneratedSegment0[1] -eq 1 -and
                                        $m3GeneratedSegment0[2] -eq 1 -and
                                        $m3GeneratedSegment0[3] -eq 1 -and
                                        $m3GeneratedSegment0[4] -eq 0 -and
                                        $m3GeneratedSegment0[5] -eq 26 -and
                                        $m3GeneratedSegment0[6] -eq 54 -and
                                        $m3GeneratedSegment0[7] -eq 22 -and
                                        $m3GeneratedSegment0[8] -eq 0 -and
                                        $m3GeneratedSegment0[9] -gt 0
                                    ) "Task 26 generated segment-0 path lost its exact pre-GX 26-run/54-triangle/22-epoch/no-material contract at frame $frame (actual=$($m3GeneratedSegment0 -join ','))." $gdbStdout
                                    $m3GeneratedSegment0Samples.Add(
                                        $m3GeneratedSegment0)
                                    if ($RendererM3Phase0Profile) {
                                        $m3GeneratedSegment0Shadow = Get-Ints `
                                            $m3GeneratedSegment0ShadowBenchmark[$sampleIndex]
                                        Assert-Condition (
                                            $m3GeneratedSegment0Shadow[0] -eq $frame -and
                                            $m3GeneratedSegment0Shadow[1] -eq 1 -and
                                            $m3GeneratedSegment0Shadow[2] -eq 26 -and
                                            $m3GeneratedSegment0Shadow[3] -eq 108 -and
                                            $m3GeneratedSegment0Shadow[4] -eq 123 -and
                                            $m3GeneratedSegment0Shadow[5] -eq 90 -and
                                            $m3GeneratedSegment0Shadow[6] -eq 22 -and
                                            $m3GeneratedSegment0Shadow[7] -eq 54 -and
                                            $m3GeneratedSegment0Shadow[8] -eq 733 -and
                                            $m3GeneratedSegment0Shadow[9] -eq 0 -and
                                            $m3GeneratedSegment0Shadow[10] -eq 1 -and
                                            $m3GeneratedSegment0Shadow[11] -eq 1 -and
                                            $m3GeneratedSegment0Shadow[12] -eq 1 -and
                                            $m3GeneratedSegment0Shadow[13] -eq 1 -and
                                            $m3GeneratedSegment0Shadow[14] -eq 1
                                        ) "Task 26 segment-0 shadow lost exact output coverage, generation-fault rejection, or live-operand pre-GX rejection/revalidation at frame $frame (actual=$($m3GeneratedSegment0Shadow -join ','))." $gdbStdout
                                        $m3GeneratedSegment0ShadowSamples.Add(
                                            $m3GeneratedSegment0Shadow)
                                    }
                                }
                                if ($RendererM3Phase0Profile) {
                                    $m3Prepared = Get-Ints $m3PreparedBenchmark[$sampleIndex]
                                    Assert-Condition (
                                        $m3Prepared[0] -eq $frame -and
                                        $m3Prepared[1] -gt 0 -and
                                        $m3Prepared[2] -gt 0
                                    ) "M3 prepared-output stability census lost frame/topology synchronization at frame $frame (actual=$($m3Prepared -join ','))." $gdbStdout
                                    $m3PreparedSamples.Add($m3Prepared)
                                    $m3Whispy = Get-Ints $m3WhispyBenchmark[$sampleIndex]
                                    if ($RendererBenchmarkStartFrame -eq 672) {
                                        $expectedWhispy = if ($sampleIndex -lt 4) {
                                            @(1, (7 - (2 * $sampleIndex)), 0,
                                              (71 - (2 * $sampleIndex)))
                                        } else {
                                            @(3, 0, 0,
                                              (71 - (2 * $sampleIndex)))
                                        }
                                        Assert-Condition (
                                            $m3Whispy[0] -eq $frame -and
                                            (($m3Whispy[1..4] -join ',') -eq
                                             ($expectedWhispy -join ','))
                                        ) "M3 natural Whispy Wait-to-Open/material-animation boundary drifted at frame $frame (actual=$($m3Whispy -join ',') expected=$($expectedWhispy -join ','))." $gdbStdout
                                    } else {
                                        Assert-Condition ($m3Whispy[0] -eq $frame -and $m3Whispy[1] -le 5) "M3 Whispy source-state census lost synchronization or status bounds at frame $frame (actual=$($m3Whispy -join ','))." $gdbStdout
                                    }
                                    $m3WhispySamples.Add($m3Whispy)
                                    $phase0 = Get-Ints $m3Phase0Benchmark[$sampleIndex]
                                    $expectedPhase0Counts = if (($benchmarkMakeIdentity.NativeStageGeneratedSegment0Enable -eq 1) -and $RendererM3Phase0Profile) { @(1639, 811, 16, 420, 334, 146) } else { @(1319, 651, 16, 312, 226, 146) }
                                    Assert-Condition ($phase0[0] -eq $frame -and $phase0[12] -eq $expectedPhase0Counts[0] -and $phase0[13] -eq $expectedPhase0Counts[1] -and $phase0[15] -eq $expectedPhase0Counts[2] -and $phase0[16] -eq $expectedPhase0Counts[3] -and $phase0[17] -eq $expectedPhase0Counts[4] -and $phase0[18] -eq $expectedPhase0Counts[5]) "M3 Phase-0 timer/count census drifted at frame $frame (actual=$($phase0 -join ','))." $gdbStdout
                                    Assert-Condition ($phase0[2] -ge $phase0[3] -and $phase0[3] -ge $phase0[4] -and $phase0[8] -ge $phase0[9] -and $phase0[1] -ge $phase0[2] -and $phase0[11] -ge ($phase0[5] + $phase0[6] + $phase0[7] + $phase0[8] + $phase0[10])) "M3 Phase-0 nested bucket conservation failed at frame $frame (actual=$($phase0 -join ','))." $gdbStdout
                                    $m3Phase0Samples.Add($phase0)
                                    $m3Residual = Get-Ints $m3ResidualBenchmark[$sampleIndex]
                                    Assert-Condition (
                                        $m3Residual[0] -eq $frame -and
                                        ($m3Residual[5] + $m3Residual[6]) -eq 1 -and
                                        $m3Residual[7] -eq 484 -and
                                        $m3Residual[8] -eq 28 -and
                                        $m3Residual[9] -eq 204 -and
                                        $m3Residual[10] -eq 118 -and
                                        $m3Residual[1] -ge $m3Residual[2] -and
                                        $m3Residual[2] -ge $m3Residual[3]
                                    ) "Task 23R residual key/cost census drifted at frame $frame (actual=$($m3Residual -join ','))." $gdbStdout
                                    $m3ResidualSamples.Add($m3Residual)
                                    $g2State = Get-Ints $g2StateBenchmark[$sampleIndex]
                                    if ($PhaseMatrixMode) {
                                        $expectedG2State = if (
                                            ($RendererBenchmarkStartFrame -eq 600) -or
                                            ($RendererBenchmarkStartEvent -in @('KO','Rebirth'))) {
                                            @(41, 13, 138, 0, 54, 32)
                                        } elseif (
                                            (($RendererBenchmarkStartFrame -eq 672) -and
                                             ($sampleIndex -ge 4))) {
                                            @(44, 13, 166, 0, 70, 36)
                                        } elseif (
                                            ($RendererBenchmarkStartFrame -eq 672) -or
                                            (($RendererBenchmarkStartEvent -eq 'TimeUp') -and
                                             ($sampleIndex -ge 6))) {
                                            @(44, 13, 166, 0, 70, 34)
                                        } else {
                                            @(44, 13, 164, 0, 69, 34)
                                        }
                                        Assert-Condition (
                                            $g2State[0] -eq $frame -and
                                            (($g2State[1..6] -join ',') -eq
                                             ($expectedG2State -join ','))
                                        ) "Phase-matrix G2 state census drifted at frame $frame (actual=$($g2State -join ',') expected=$($expectedG2State -join ','))." $gdbStdout
                                    } elseif ($Task22WallpaperRunLab -and
                                        (($RendererBenchmarkStartFrame -ne 438) -or
                                         ($RendererBenchmarkStartEvent -ne 'None'))) {
                                        Assert-Condition (
                                            $g2State[0] -eq $frame -and
                                            $g2State[1] -ge 0 -and $g2State[2] -ge 0 -and
                                            $g2State[3] -ge 0 -and $g2State[4] -ge 0 -and
                                            $g2State[5] -ge 0 -and $g2State[6] -ge 0
                                        ) "Task22 lab G2 state census lost synchronization or produced a negative count at frame $frame (actual=$($g2State -join ','))." $gdbStdout
                                    } else {
                                        Assert-Condition ($g2State[0] -eq $frame -and $g2State[1] -eq 44 -and $g2State[2] -eq 13 -and $g2State[3] -eq 164 -and $g2State[4] -eq 0 -and $g2State[5] -eq 69 -and $g2State[6] -eq 34) "G2 state write/skip census drifted at frame $frame (actual=$($g2State -join ','))." $gdbStdout
                                    }
                                    $g2StateSamples.Add($g2State)
                                    $phase05 = Get-Ints $phase05Benchmark[$sampleIndex]
                                    $phase05WallpaperSubtotal =
                                        [int64]$phase05[1] + [int64]$phase05[2] +
                                        [int64]$phase05[3] + [int64]$phase05[4] +
                                        [int64]$phase05[5]
                                    $phase05StageExecute =
                                        [int64]$coarse[11] - [int64]$phase05[8]
                                    $phase05GCDrawShell =
                                        [int64]$phase05[7] - $phase05StageExecute -
                                        [int64]$coarse[12] - [int64]$coarse[13] -
                                        [int64]$coarse[14]
                                    $phase05FighterShell = [int64]$phase05[9]
                                    $phase05PresentShell =
                                        [int64]$phase05[6] -
                                        ([int64]$phase05[7] + [int64]$phase05[8] +
                                         [int64]$phase05[9])
                                    $phase05NamedDrawShell =
                                        $phase05GCDrawShell + $phase05FighterShell +
                                        $phase05PresentShell
                                    $phase05PixelWritesValid = if ($WallpaperIncrementalMode -eq 0) {
                                        $phase05[20] -eq 49152
                                    } else {
                                        ($phase05[20] -ge 0) -and ($phase05[20] -le 49152)
                                    }
                                    $phase05WallpaperInactive =
                                        (($PhaseMatrixMode -and
                                          ($RendererBenchmarkStartEvent -in @('Late','TimeUp'))) -or
                                         ($m4CandidateEvidence -and $fastIterationUnarmedM4)) -and
                                        ($phase05[19] -eq 0) -and ($phase05[20] -eq 0)
                                    Assert-Condition ($phase05[0] -eq $frame -and $phase05[18] -eq 16 -and (($phase05[19] -eq 192 -and $phase05PixelWritesValid) -or $phase05WallpaperInactive) -and $phase05[15] -gt $phase05[16] -and $phase05[16] -gt 0) "Phase-0.5 timer/count census drifted at frame $frame (actual=$($phase05 -join ','))." $gdbStdout
                                    Assert-Condition ($phase05WallpaperSubtotal -le $coarse[10] -and $coarse[11] -ge $phase05[8] -and $phase05[7] -ge $phase05StageExecute -and $phase05[9] -gt 0 -and $phase05[6] -ge ($phase05[7] + $phase05[8] + $phase05[9]) -and $coarse[19] -ge $phase05NamedDrawShell -and $coarse[20] -ge ($phase05[10] + $phase05[11] + $phase05[14]) -and $coarse[21] -ge ($phase05[12] + $phase05[13])) "Phase-0.5 nested conservation failed at frame $frame (phase05=$($phase05 -join ',') coarse=$($coarse -join ','))." $gdbStdout
                                    $phase05Samples.Add($phase05)
                                    $wallRuns = Get-Ints $wallRunBenchmark[$sampleIndex]
                                    Assert-Condition ($wallRuns[0] -eq $frame -and ($wallRuns[1] + $wallRuns[2]) -eq $phase05[19] -and $wallRuns[3] -le 256 -and $wallRuns[4] -le $wallRuns[3] -and $wallRuns[5] -le $wallRuns[3] -and $wallRuns[7] -le $wallRuns[3] -and $wallRuns[9] -le $wallRuns[7] -and $wallRuns[11] -le $wallRuns[9] -and ([int64]$wallRuns[12] + (2 * [int64]$wallRuns[13]) + [int64]$wallRuns[14] + [int64]$wallRuns[15]) -eq [int64]$wallRuns[16] -and $wallRuns[16] -eq $phase05[20]) "Wallpaper run/store census failed exact row, nesting, or physical-write conservation at frame $frame (actual=$($wallRuns -join ','))." $gdbStdout
                                    $wallRunSamples.Add($wallRuns)
                                }
                            } else {
                                Assert-Condition ($fastRun[2] -gt 0 -and $fastRun[3] -gt 0) "Selected laboratory fast mode executed no fast triangles at frame $frame." $gdbStdout
                            }
                            $fastRunSamples.Add($fastRun)
                            if ($m4CandidateEvidence) {
                                $waterStill = Get-Ints $m4WaterStillBenchmark[$sampleIndex]
                                Assert-Condition ($waterStill[0] -eq $frame -and $waterStill[1] -eq 2 -and $waterStill[2] -eq 0 -and $waterStill[3] -eq 1) "M4 water did not freeze exactly two source-initial materials before gameplay at frame $frame (actual=$($waterStill -join ','))." $gdbStdout
                                if ($sampleIndex -gt 0) {
                                    $previousWaterStill = Get-Ints $m4WaterStillBenchmark[$sampleIndex - 1]
                                    Assert-Condition ($waterStill[1] -eq $previousWaterStill[1] -and $waterStill[2] -eq $previousWaterStill[2] -and $waterStill[3] -eq $previousWaterStill[3]) "M4 still-water freeze state changed during gameplay at frame $frame." $gdbStdout
                                }
                                $m4WaterStillSamples.Add($waterStill)
                            }
                            $m4Static = Get-Ints $m4StaticBenchmark[$sampleIndex]
                            Assert-Condition ($m4Static[0] -eq $frame -and
                                $m4Static[1] -eq $effectiveStaticTextureAotMode) "M4 static-texture accounting is not synchronized or in the effective mode at frame $frame." $gdbStdout
                            if ($isCpuPrepNoGx -and
                                ($effectiveStaticTextureAotMode -eq 1)) {
                                Assert-Condition (
                                    $m4Static[2] -eq 1 -and
                                    $m4Static[3] -eq 1 -and
                                    @($m4Static[4..10] |
                                        Where-Object { $_ -ne 0 }).Count -eq 0
                                ) "CPU_PREP_NO_GX did not retain the expected mocked static-texture preparation failure at frame $frame (actual=$($m4Static -join ','))." $gdbStdout
                            } elseif ($effectiveStaticTextureAotMode -eq 1) {
                                if ($fastIterationUnarmedM4) {
                                    Assert-Condition (
                                        $m4Static[2] -eq 1 -and
                                        $m4Static[3] -eq 0 -and
                                        $m4Static[4] -eq 24 -and
                                        $m4Static[5] -eq 136192 -and
                                        @($m4Static[6..10] |
                                            Where-Object { $_ -ne 0 }).Count -eq 0
                                    ) "Fast iteration did not preserve the prepared, deliberately unarmed M4 corpus at frame $frame (actual=$($m4Static -join ','))." $gdbStdout
                                } else {
                                    Assert-Condition (
                                        $m4Static[2] -eq 1 -and
                                        $m4Static[3] -eq 0 -and
                                        $m4Static[4] -eq 24 -and
                                        $m4Static[5] -eq 136192 -and
                                        $m4Static[6] -eq 1 -and
                                        ($m4Static[7] -band 0x3fffff) -eq
                                            0x3fffff -and
                                        ($m4Static[8] -band 0x7) -eq 0x7 -and
                                        $m4Static[9] -eq 0 -and
                                        $m4Static[10] -gt 0
                                    ) "M4 static-texture AOT corpus was not fully prepared, armed, covered, and pinned at frame $frame (actual=$($m4Static -join ','))." $gdbStdout
                                }
                            } else {
                                Assert-Condition (
                                    @($m4Static[2..10] |
                                        Where-Object { $_ -ne 0 }).Count -eq 0
                                ) "M4 static-texture control mode performed work at frame $frame (actual=$($m4Static -join ','))." $gdbStdout
                            }
                            $m4StaticSamples.Add($m4Static)
                            $m4Fence = Get-Ints $m4FenceBenchmark[$sampleIndex]
                            Assert-Condition ($m4Fence[0] -eq $frame) `
                                "M4 post-GO texture fence is not synchronized at frame $frame." `
                                $gdbStdout
                            $m4FenceCountSum = [int64]0
                            foreach ($fenceCount in $m4Fence[3..12]) {
                                $m4FenceCountSum += $fenceCount
                            }
                            if ($m4FenceCountSum -eq 0) {
                                Assert-Condition (
                                    $m4Fence[1] -eq 0 -and $m4Fence[2] -eq 0
                                ) "M4 zero-count fence published a first diagnostic at frame $frame." $gdbStdout
                            } else {
                                Assert-Condition (
                                    $m4Fence[1] -ge 1 -and
                                    $m4Fence[1] -le 10 -and
                                    $m4Fence[2] -gt 0 -and
                                    $m4Fence[2] -le $frame
                                ) "M4 nonzero fence lacks a valid first class/frame at frame $frame." $gdbStdout
                            }
                            if ($sampleIndex -gt 0) {
                                $previousM4Fence = Get-Ints $m4FenceBenchmark[$sampleIndex - 1]
                                $m4FenceMonotonic = $true
                                foreach ($fenceIndex in 3..12) {
                                    if ($m4Fence[$fenceIndex] -lt
                                        $previousM4Fence[$fenceIndex]) {
                                        $m4FenceMonotonic = $false
                                    }
                                }
                                Assert-Condition $m4FenceMonotonic `
                                    "M4 cumulative fence counters regressed at frame $frame." `
                                    $gdbStdout
                                if ($previousM4Fence[1] -ne 0) {
                                    Assert-Condition (
                                        $m4Fence[1] -eq $previousM4Fence[1] -and
                                        $m4Fence[2] -eq $previousM4Fence[2]
                                    ) "M4 first fence diagnostic changed at frame $frame." $gdbStdout
                                }
                            }
                            if ($RequireZeroPostGoTextureFence -and
                                -not $PhaseMatrixMode) {
                                Assert-Condition (
                                    @($m4Fence[1..12] |
                                        Where-Object { $_ -ne 0 }).Count -eq 0
                                ) "M4 strict post-GO texture fence failed at frame $frame (actual=$($m4Fence -join ','))." $gdbStdout
                            }
                            $m4FenceSamples.Add($m4Fence)
                            if ($RendererM2DetailedLedger) {
                                $m2Shade = Get-Ints $m2ShadeBenchmark[$sampleIndex]
                                Assert-Condition (
                                    $m2Shade[0] -eq $frame -and
                                    $m2Shade[1] -eq 49 -and
                                    $m2Shade[1] -eq ($m2Shade[11] + $m2Shade[14]) -and
                                    $m2Shade[2] -eq ($m2Shade[12] + $m2Shade[15]) -and
                                    $m2Shade[3] -eq ($m2Shade[13] + $m2Shade[16]) -and
                                    $m2Shade[11] -eq 18 -and
                                    $m2Shade[14] -eq 31 -and
                                    $m2Shade[3] -le $m2Shade[2] -and
                                    $m2Shade[2] -le $m2Shade[1] -and
                                    $m2Shade[4] -eq 0 -and
                                    $m2Shade[5] -eq ($m2Shade[6] + $m2Shade[9]) -and
                                    $m2Shade[6] -eq ($m2Shade[7] + $m2Shade[8]) -and
                                    $m2Shade[10] -le $m2Shade[5]
                                ) "M2 shade census lost frame, owner, exact-key, producer-residency, or dense-work conservation at frame $frame (actual=$($m2Shade -join ','))." $gdbStdout
                                $m2ShadeSamples.Add($m2Shade)
                                $m2Combined = [int64[]]::new(50)
                                $m2Combined[0] = $frame
                                for ($m2OwnerOffset = 0;
                                     $m2OwnerOffset -lt 2;
                                     $m2OwnerOffset++) {
                                    $m2 = Get-Ints $m2Benchmark[
                                        (2 * $sampleIndex) + $m2OwnerOffset]
                                    $m2OwnerId = $m2OwnerOffset + 1
                                    Assert-Condition (
                                        $m2[0] -eq $frame -and
                                        $m2[1] -eq $m2OwnerId
                                    ) "M2 owner $m2OwnerId record is not synchronized at frame $frame." $gdbStdout
                                    $m2Samples[$m2OwnerOffset].Add($m2)
                                    foreach ($m2Field in 2..49) {
                                        $m2Combined[$m2Field] +=
                                            $m2[$m2Field]
                                    }

                                    if (($RendererFastRunMode -eq 8) -and
                                        ($benchmarkMakeIdentity.RendererBenchmarkMode -in @(0,1))) {
                                        $expectedJoints = if ($m2OwnerId -eq 1) { 25 } else { 27 }
                                        $expectedBindings = if ($m2OwnerId -eq 1) { 14 } else { 18 }
                                        $expectedLocalBuilds = $expectedJoints - 1
                                        $expectedWorldAffines = $expectedLocalBuilds
                                        $expectedComposeCount = $expectedBindings
                                        $ownerPhaseTicks = [int64]0
                                        foreach ($tickField in 2..13) {
                                            $ownerPhaseTicks += $m2[$tickField]
                                        }
                                        $ownerPhaseTicks += $m2[19]
                                        $productionPhaseTicks = [int64]0
                                        foreach ($tickField in 14..18) {
                                            $productionPhaseTicks +=
                                                $m2[$tickField]
                                        }
                                        Assert-Condition (
                                            $ownerPhaseTicks -eq
                                                $coarse[11 + $m2OwnerId] -and
                                            $productionPhaseTicks -eq $m2[13] -and
                                            $m2[20] -eq 1 -and
                                            $m2[21] -eq 0 -and
                                            $m2[22] -eq 0 -and
                                            $m2[23] -eq 0
                                        ) "M2 owner $m2OwnerId phase ledger did not conserve its exclusive/production walls at frame $frame." $gdbStdout
                                        Assert-Condition (
                                            $m2[24] -eq $expectedJoints -and
                                            $m2[25] -eq $expectedJoints -and
                                            $m2[26] -eq $expectedBindings -and
                                            $m2[27] -eq $expectedBindings
                                        ) "M2 owner $m2OwnerId live joint/binding schedule drifted from $expectedJoints/$expectedBindings at frame $frame." $gdbStdout
                                        Assert-Condition (
                                            $m2[28] -eq
                                                ($m2[29] + $m2[30] + $m2[31]) -and
                                            $m2[29] -eq $expectedJoints -and
                                            $m2[31] -eq 0 -and
                                            $m2[33] -eq $expectedJoints -and
                                            $m2[33] -eq
                                                ($m2[34] + $m2[35] +
                                                 $m2[36] + $m2[37]) -and
                                            $m2[38] -le 1
                                        ) "M2 owner $m2OwnerId XObj/FTParts/animlock census is internally inconsistent at frame $frame." $gdbStdout
                                        Assert-Condition (
                                            $m2[39] -eq 1 -and
                                            $m2[40] -eq $expectedBindings -and
                                            $m2[41] -le $m2[40] -and
                                            $m2[42] -eq $expectedLocalBuilds -and
                                            $m2[43] -eq $expectedWorldAffines -and
                                            $m2[44] -le $expectedBindings -and
                                            $m2[45] -eq $expectedComposeCount
                                        ) "M2 owner $m2OwnerId matrix phase counts are not source/binding conserved at frame $frame." $gdbStdout
                                        if ($benchmarkMakeIdentity.RendererBenchmarkMode -eq 0) {
                                            Assert-Condition (
                                                $m2[46] -eq $expectedBindings -and
                                                $m2[47] -gt 0 -and
                                                $m2[48] -gt 0 -and
                                                $m2[49] -gt 0
                                            ) "M2 owner $m2OwnerId production phase counts are incomplete at frame $frame." $gdbStdout
                                        } else {
                                            Assert-Condition (
                                                $m2[46] -eq 0 -and
                                                $m2[48] -eq 0 -and
                                                $m2[49] -gt 0
                                            ) "M2 owner $m2OwnerId TRIANGLE_NOOP phase census is inconsistent at frame $frame." $gdbStdout
                                        }
                                    } elseif (($RendererFastRunMode -eq 7) -and
                                        ($benchmarkMakeIdentity.RendererBenchmarkMode -in @(0,1))) {
                                        $expectedJoints = if ($m2OwnerId -eq 1) { 25 } else { 27 }
                                        $expectedBindings = if ($m2OwnerId -eq 1) { 14 } else { 18 }
                                        $expectedEpochs = if ($m2OwnerId -eq 1) { 18 } else { 31 }
                                        $expectedRuns = if ($m2OwnerId -eq 1) { 30 } else { 37 }
                                        $ownerPhaseTicks = [int64]0
                                        foreach ($tickField in 2..13) {
                                            $ownerPhaseTicks += $m2[$tickField]
                                        }
                                        $ownerPhaseTicks += $m2[19]
                                        $productionPhaseTicks = [int64]0
                                        foreach ($tickField in 14..18) {
                                            $productionPhaseTicks += $m2[$tickField]
                                        }
                                        Assert-Condition (
                                            $ownerPhaseTicks -eq
                                                $coarse[11 + $m2OwnerId] -and
                                            $productionPhaseTicks -eq $m2[13] -and
                                            $m2[20] -eq 1 -and
                                            $m2[21] -eq 0 -and
                                            $m2[22] -eq 0 -and
                                            $m2[23] -eq 0
                                        ) "M2 hierarchy owner $m2OwnerId did not conserve exclusive/production phases or fell back at frame $frame." $gdbStdout
                                        Assert-Condition (
                                            $m2[24] -eq $expectedJoints -and
                                            $m2[25] -eq $expectedJoints -and
                                            $m2[26] -eq $expectedBindings -and
                                            $m2[27] -eq $expectedBindings -and
                                            $m2[28] -eq
                                                ($m2[29] + $m2[30] + $m2[31]) -and
                                            $m2[29] -eq $expectedJoints -and
                                            $m2[31] -eq 0 -and
                                            $m2[33] -eq $expectedJoints -and
                                            $m2[33] -eq
                                                ($m2[34] + $m2[35] +
                                                 $m2[36] + $m2[37]) -and
                                            $m2[38] -eq 0
                                        ) "M2 hierarchy owner $m2OwnerId joint/binding/XObj/FTParts census drifted at frame $frame." $gdbStdout
                                        Assert-Condition (
                                            $m2[39] -eq 1 -and
                                            $m2[40] -eq 0 -and
                                            $m2[41] -eq 0 -and
                                            $m2[42] -eq $expectedJoints -and
                                            $m2[43] -eq 0 -and
                                            $m2[44] -eq 0 -and
                                            $m2[45] -eq 0
                                        ) "M2 hierarchy owner $m2OwnerId retained a CPU world/camera/final-compose pass at frame $frame." $gdbStdout
                                        $expectedRootGX = if (
                                            $benchmarkMakeIdentity.RendererBenchmarkMode -eq 0
                                        ) { $expectedBindings } else { 0 }
                                        Assert-Condition (
                                            $m2[46] -eq $expectedRootGX -and
                                            $m2[47] -eq $expectedEpochs -and
                                            $m2[48] -eq $expectedRuns -and
                                            $m2[49] -eq $expectedRuns
                                        ) "M2 hierarchy owner $m2OwnerId did not preserve exact root/epoch/run commit counts at frame $frame." $gdbStdout
                                    }
                                }
                                if (($RendererFastRunMode -in @(7, 8)) -and
                                    ($benchmarkMakeIdentity.RendererBenchmarkMode -eq 0)) {
                                    Assert-Condition (
                                        $m2Combined[46] -eq 32 -and
                                        $m2Combined[47] -eq 49 -and
                                        $m2Combined[48] -eq 67 -and
                                        $m2Combined[49] -eq 67
                                    ) "M2 combined production census did not conserve 32 roots, 49 epochs, and 67 runs at frame $frame." $gdbStdout
                                }
                                $m2CombinedSamples.Add($m2Combined)
                            }
                            $drawResidualRatios += Get-RatioBasisPoints $coarse[19] $drawTicks
                            $presentResidualRatios += Get-RatioBasisPoints $coarse[20] $presentActive
                            $loopResidualRatios += Get-RatioBasisPoints $coarse[21] $loopWall

                            if ($RendererProfileLevel -ge 2) {
                                $ownerTriangleTotal = [int64]0
                                $ownerRunTotal = [int64]0
                                $ownerClassTotals = [int64[]]::new(8)
                                $expectedOwnerTriangles = @(202, 320, 306)
                                for ($ownerIndex = 0; $ownerIndex -lt 3; $ownerIndex++) {
                                    $owner = Get-Ints $ownerBenchmark[(3 * $sampleIndex) + $ownerIndex]
                                    Assert-Condition ($owner[0] -eq $frame -and $owner[1] -eq $ownerIndex -and $owner[2] -eq $coarse[11 + $ownerIndex]) "Owner renderer benchmark record $ownerIndex is not synchronized at frame $frame." $gdbStdout
                                    $ownerClassTotal = [int64]0
                                    foreach ($classIndex in 0..7) {
                                        $ownerClassTotal += $owner[9 + $classIndex]
                                        $ownerClassTotals[$classIndex] +=
                                            $owner[9 + $classIndex]
                                    }
                                    $ownerTriangleTotal += $owner[8]
                                    $ownerRunTotal += $owner[20]
                                    $boundaryHashesPresent = $true
                                    foreach ($hashIndex in 21..28) {
                                        if ($owner[$hashIndex] -eq 0) {
                                            $boundaryHashesPresent = $false
                                        }
                                    }
                                    Assert-Condition ($owner[8] -eq $expectedOwnerTriangles[$ownerIndex] -and $ownerClassTotal -eq $owner[8] -and $boundaryHashesPresent) "Owner renderer benchmark record $ownerIndex did not preserve its exact triangle/class partition or nonzero entry/exit boundary hashes at frame $frame." $gdbStdout
                                    $ownerSamples[$ownerIndex].Add($owner)
                                }
                                Assert-Condition ($ownerTriangleTotal -eq 828 -and $ownerRunTotal -eq 121 -and $ownerClassTotals[0] -eq 648 -and $ownerClassTotals[1] -eq 0 -and $ownerClassTotals[2] -eq 44 -and $ownerClassTotals[3] -eq 126 -and $ownerClassTotals[4] -eq 0 -and $ownerClassTotals[5] -eq 0 -and $ownerClassTotals[6] -eq 10 -and $ownerClassTotals[7] -eq 0) "Owner renderer benchmark did not conserve exact 828 triangles, 648/44/126/10 classes, and 121 runs at frame $frame." $gdbStdout
                                $semantic = Get-Ints $rendererSemanticBenchmark[$sampleIndex]
                                $ownerEventCount = [int64]0
                                $ownerOverflowCount = [int64]0
                                Assert-Condition ($semantic[0] -eq $frame -and $semantic[1] -ne 0 -and $semantic[2] -ne 0 -and $semantic[3] -eq 828 -and $semantic[3] -eq $render[11] -and $semantic[4] -eq 0) "Renderer semantic frame hash/count/overflow contract failed at frame $frame." $gdbStdout
                                for ($ownerIndex = 0; $ownerIndex -lt 3; $ownerIndex++) {
                                    $semanticBase = 5 + (11 * $ownerIndex)
                                    $ownerProfile = Get-Ints $ownerBenchmark[(3 * $sampleIndex) + $ownerIndex]
                                    $ownerEventCount += $semantic[$semanticBase + 2]
                                    $ownerOverflowCount += $semantic[$semanticBase + 3]
                                    Assert-Condition ($semantic[$semanticBase] -ne 0 -and $semantic[$semanticBase + 1] -ne 0 -and $semantic[$semanticBase] -eq $ownerProfile[36] -and $semantic[$semanticBase + 2] -eq $ownerProfile[8] -and $semantic[$semanticBase + 3] -eq 0 -and $semantic[$semanticBase + 4] -gt 0) "Renderer semantic owner $ownerIndex hash/count/occurrence contract failed at frame $frame." $gdbStdout
                                    Assert-Condition ($semantic[$semanticBase + 5] -lt $semantic[$semanticBase + 4] -and $semantic[$semanticBase + 6] -lt $ownerProfile[3] -and $semantic[$semanticBase + 7] -ne 0 -and $semantic[$semanticBase + 8] -lt $ownerProfile[4] -and $semantic[$semanticBase + 9] -le 1 -and $semantic[$semanticBase + 10] -le 3) "Renderer semantic owner $ownerIndex first provenance/outcome was out of bounds at frame $frame." $gdbStdout
                                }
                                Assert-Condition ($ownerEventCount -eq $semantic[3] -and $ownerOverflowCount -eq $semantic[4]) "Renderer semantic owner totals did not conserve frame events/overflow at frame $frame." $gdbStdout
                                $semanticSamples.Add($semantic)
                            }
                            $coarseSamples.Add($coarse)
                        }

                        $coarseLoop = Get-SampleFieldValues $coarseSamples 1
                        $coarseInput = Get-SampleFieldValues $coarseSamples 2
                        $coarseUpdate = Get-SampleFieldValues $coarseSamples 3
                        $coarseSourceUpdate = Get-SampleFieldValues $coarseSamples 4
                        $coarseAudioUpdate = Get-SampleFieldValues $coarseSamples 5
                        $coarseActive = Get-SampleFieldValues $coarseSamples 6
                        $coarseWait = Get-SampleFieldValues $coarseSamples 7
                        $coarseBegin = Get-SampleFieldValues $coarseSamples 8
                        $coarseDraw = Get-SampleFieldValues $coarseSamples 9
                        $coarseWallpaper = Get-SampleFieldValues $coarseSamples 10
                        $coarseStage = Get-SampleFieldValues $coarseSamples 11
                        $coarseMario = Get-SampleFieldValues $coarseSamples 12
                        $coarseFox = Get-SampleFieldValues $coarseSamples 13
                        $coarseForeground = Get-SampleFieldValues $coarseSamples 14
                        $coarseHud = Get-SampleFieldValues $coarseSamples 15
                        $coarseFlush = Get-SampleFieldValues $coarseSamples 16
                        $coarsePostVBlank = Get-SampleFieldValues $coarseSamples 17
                        $coarseThreads = Get-SampleFieldValues $coarseSamples 18
                        $coarseDrawResidual = Get-SampleFieldValues $coarseSamples 19
                        $coarsePresentResidual = Get-SampleFieldValues $coarseSamples 20
                        $coarseLoopResidual = Get-SampleFieldValues $coarseSamples 21
                        $coarseConservation = Get-SampleFieldValues $coarseSamples 22
                        $coarseLogic = Get-SampleFieldValues $coarseSamples 23
                        $coarseMetricSummary = "Renderer coarse benchmark: samples=$RendererBenchmarkSamples frames=$($benchFrames[0])..$($benchFrames[-1]) logic=$($coarseLogic[0])..$($coarseLogic[-1]) median/p95 ticks loop=$(Get-MedianP95 $coarseLoop) input=$(Get-MedianP95 $coarseInput) update=$(Get-MedianP95 $coarseUpdate) sourceUpdate=$(Get-MedianP95 $coarseSourceUpdate) audioUpdate=$(Get-MedianP95 $coarseAudioUpdate) active=$(Get-MedianP95 $coarseActive) wait=$(Get-MedianP95 $coarseWait) begin=$(Get-MedianP95 $coarseBegin) draw=$(Get-MedianP95 $coarseDraw) wallpaper=$(Get-MedianP95 $coarseWallpaper) stage=$(Get-MedianP95 $coarseStage) Mario=$(Get-MedianP95 $coarseMario) Fox=$(Get-MedianP95 $coarseFox) foreground=$(Get-MedianP95 $coarseForeground) hud=$(Get-MedianP95 $coarseHud) flush=$(Get-MedianP95 $coarseFlush) postVBlank=$(Get-MedianP95 $coarsePostVBlank) threads=$(Get-MedianP95 $coarseThreads) drawResidual=$(Get-MedianP95 $coarseDrawResidual) presentResidual=$(Get-MedianP95 $coarsePresentResidual) loopResidual=$(Get-MedianP95 $coarseLoopResidual) conservationError=$(Get-MedianP95 $coarseConservation)"
                        if ($Task9FloatCensusMode -eq 1) {
                            Assert-Condition ($task9FloatPairBenchmark.Count -eq $RendererBenchmarkSamples) "Task 9 float census captured $($task9FloatPairBenchmark.Count) of $RendererBenchmarkSamples pair records." $gdbStdout
                            Assert-Condition ($task9FloatCostBenchmark.Count -eq $task9FloatRoutineNames.Count) "Task 9 float census captured $($task9FloatCostBenchmark.Count) of $($task9FloatRoutineNames.Count) cost records." $gdbStdout
                            Assert-Condition ($task9FloatTimerBenchmark.Count -eq 1) 'Task 9 float census did not capture its timer-read calibration.' $gdbStdout
                            $task9Timer = Get-Ints $task9FloatTimerBenchmark[0]
                            Assert-Condition ($task9Timer[1] -eq 256) 'Task 9 float census timer-read calibration count changed.' $gdbStdout
                            $task9TimerMean = [int64][Math]::Round(
                                [double]$task9Timer[0] / [double]$task9Timer[1])
                            $task9Pairs = @($task9FloatPairBenchmark | ForEach-Object {
                                , @(Get-Ints $_)
                            })
                            for ($sampleIndex = 0; $sampleIndex -lt $task9Pairs.Count; $sampleIndex++) {
                                Assert-Condition ($task9Pairs[$sampleIndex][0] -eq $benchFrames[$sampleIndex]) 'Task 9 float pair census is not synchronized with the renderer frame.' $gdbStdout
                                if ($sampleIndex -gt 0) {
                                    Assert-Condition (($task9Pairs[$sampleIndex][1] - $task9Pairs[$sampleIndex - 1][1]) -eq 2) 'Task 9 float census did not observe exactly two updates per sampled frame.' $gdbStdout
                                }
                            }
                            for ($routineIndex = 0; $routineIndex -lt $task9FloatRoutineNames.Count; $routineIndex++) {
                                $cost = Get-Ints $task9FloatCostBenchmark[$routineIndex]
                                Assert-Condition ($cost[0] -eq $routineIndex) 'Task 9 float cost records changed order.' $gdbStdout
                                $pairCounts = @($task9Pairs | ForEach-Object {
                                    [int64]$_[2 + $routineIndex]
                                })
                                $rawMean = if ($cost[2] -gt 0) {
                                    [int64][Math]::Round([double]$cost[1] / [double]$cost[2])
                                } else { 0 }
                                $measuredCost = [Math]::Max(0, $rawMean - $task9TimerMean)
                                $measuredMin = if ($cost[2] -gt 0) {
                                    [Math]::Max(0, $cost[3] - $task9TimerMean)
                                } else { 0 }
                                $measuredMax = if ($cost[2] -gt 0) {
                                    [Math]::Max(0, $cost[4] - $task9TimerMean)
                                } else { 0 }
                                $perUpdateMean = [double](($pairCounts | Measure-Object -Sum).Sum) /
                                    [double](2 * $pairCounts.Count)
                                $tickBound = [int64]$cost[6] * [int64]$measuredMax
                                $row = [ordered]@{
                                    index = $routineIndex
                                    routine = $task9FloatRoutineNames[$routineIndex]
                                    pairP50 = Get-Median $pairCounts
                                    pairP95 = Get-Percentile95 $pairCounts
                                    perUpdateMean = $perUpdateMean
                                    singleUpdateMin = [int64]$cost[5]
                                    singleUpdateMax = [int64]$cost[6]
                                    costSamples = [int64]$cost[2]
                                    measuredCostTicks = [int64]$measuredCost
                                    measuredMinTicks = [int64]$measuredMin
                                    measuredMaxTicks = [int64]$measuredMax
                                    tickBound = $tickBound
                                }
                                $task9FloatRows += [PSCustomObject]$row
                                if (($row.pairP95 -gt 0) -or ($row.costSamples -gt 0)) {
                                    $meanText = $row.perUpdateMean.ToString('0.###', [System.Globalization.CultureInfo]::InvariantCulture)
                                    $task9FloatSummaries += "Task9 float $($row.routine): pairP50/P95=$($row.pairP50)/$($row.pairP95) perUpdateMean=$meanText singleUpdateMin/Max=$($row.singleUpdateMin)/$($row.singleUpdateMax) costTicks=$($row.measuredCostTicks)[$($row.measuredMinTicks)..$($row.measuredMaxTicks)] samples=$($row.costSamples) bound=$($row.tickBound)"
                                }
                            }
                            $task9FloatSummaries = @("Task9 float census: frames=$($benchFrames[0])..$($benchFrames[-1]) updates=$($task9Pairs[0][1])..$($task9Pairs[-1][1]) timerReadPair=$task9TimerMean ticks") + $task9FloatSummaries
                        }
                        $coarseResidualRatioSummary = "Renderer coarse residual ratios (median/p95 basis points): draw=$(Get-MedianP95 $drawResidualRatios) present=$(Get-MedianP95 $presentResidualRatios) loop=$(Get-MedianP95 $loopResidualRatios)"
                        $gxBoundarySummary = "Renderer GX boundary: samples=$RendererBenchmarkSamples frames=$($benchFrames[0])..$($benchFrames[-1]) adjacent changes/distinct values statusBefore=$(Get-AdjacentChurn (Get-SampleFieldValues $gxBoundarySamples 1)) controlBefore=$(Get-AdjacentChurn (Get-SampleFieldValues $gxBoundarySamples 2)) statusAfterFlush=$(Get-AdjacentChurn (Get-SampleFieldValues $gxBoundarySamples 3)) statusPostVBlank=$(Get-AdjacentChurn (Get-SampleFieldValues $gxBoundarySamples 4)) controlPostVBlank=$(Get-AdjacentChurn (Get-SampleFieldValues $gxBoundarySamples 5)) median/p95 ticks flush=$(Get-MedianP95 (Get-SampleFieldValues $gxBoundarySamples 6))"
                        $stage0Summary = "Renderer stage layer-0 benchmark: samples=$RendererBenchmarkSamples frames=$($benchFrames[0])..$($benchFrames[-1]) median/p95 ticks=$(Get-MedianP95 (Get-SampleFieldValues $stage0Samples 1)) adjacent changes/distinct values=$(Get-AdjacentChurn (Get-SampleFieldValues $stage0Samples 1))"
                        if ($benchmarkMakeIdentity.RendererBenchmarkMode -eq 2) {
                            $sinkMetricSummary = "Renderer CPU_PREP_NO_GX sink: samples=$RendererBenchmarkSamples words=$(Get-MedianP95 (Get-SampleFieldValues $sinkSamples 2)) cursor=$(Get-MedianP95 (Get-SampleFieldValues $sinkSamples 1)) stageWords=$(Get-MedianP95 (Get-SampleFieldValues $sinkSamples 5)) MarioWords=$(Get-MedianP95 (Get-SampleFieldValues $sinkSamples 6)) FoxWords=$(Get-MedianP95 (Get-SampleFieldValues $sinkSamples 7)) calibrationWords=$(Get-MedianP95 (Get-SampleFieldValues $sinkSamples 3)) calibrationTicks=$(Get-MedianP95 (Get-SampleFieldValues $sinkSamples 4))"
                            if ($RendererFastRunMode -eq 9) {
                                $m3GeneratedSegment0GxLast =
                                    $m3GeneratedSegment0GxSamples[-1]
                                $m3GeneratedSegment0GxMetricSummary = "Renderer Task 26 GX sink: totalWords=$($m3GeneratedSegment0GxLast[1]) totalHash=$($m3GeneratedSegment0GxLast[2])/$($m3GeneratedSegment0GxLast[3]) segment0Words=$($m3GeneratedSegment0GxLast[4]) segment0Hash=$($m3GeneratedSegment0GxLast[5])/$($m3GeneratedSegment0GxLast[6]) armFaults=$($m3GeneratedSegment0GxLast[7])"
                            }
                        }
                        if ($benchmarkMakeIdentity.RendererBenchmarkMode -eq 4) {
                            Assert-Condition (((Get-SampleFieldValues $warmSamples 1) | Measure-Object -Sum).Sum -gt 0) 'WARM_NO_UPLOAD window did not observe any animated texture refresh to suppress.' $gdbStdout
                            $warmMetricSummary = "Renderer WARM_NO_UPLOAD: samples=$RendererBenchmarkSamples suppressedUploads=$(Get-MedianP95 (Get-SampleFieldValues $warmSamples 1)) suppressedBytes=$(Get-MedianP95 (Get-SampleFieldValues $warmSamples 2))"
                        }
                        $fastRunMetricSummary = "Renderer fast raw runs: mode=$RendererFastRunMode samples=$RendererBenchmarkSamples runs=$(Get-MedianP95 (Get-SampleFieldValues $fastRunSamples 2)) triangles=$(Get-MedianP95 (Get-SampleFieldValues $fastRunSamples 3)) stage=$(Get-MedianP95 (Get-SampleFieldValues $fastRunSamples 4)) Mario=$(Get-MedianP95 (Get-SampleFieldValues $fastRunSamples 5)) Fox=$(Get-MedianP95 (Get-SampleFieldValues $fastRunSamples 6)) fallbackState=$(Get-MedianP95 (Get-SampleFieldValues $fastRunSamples 7)) fallbackVertex=$(Get-MedianP95 (Get-SampleFieldValues $fastRunSamples 8)) fallbackCommand=$(Get-MedianP95 (Get-SampleFieldValues $fastRunSamples 9))"
                        if ($RendererFastRunMode -eq 9) {
                            $m3Last = $m3StageSamples[-1]
                            $m3StageMetricSummary = "Renderer M3 stage owner: attempts/success/fallback=$($m3Last[1])/$($m3Last[2])/$($m3Last[3]) segments/mask=$($m3Last[4])/$($m3Last[5]) postArm=$($m3Last[6]) dobjs/bindings/runs/triangles/epochs=$($m3Last[7])/$($m3Last[8])/$($m3Last[9])/$($m3Last[10])/$($m3Last[11]) materials=$($m3Last[12])/$($m3Last[13]) cross=$($m3Last[14])/$($m3Last[15])/$($m3Last[16]) topology=full$($m3Last[17])/hit$($m3Last[18])/mismatch$($m3Last[19])/inject$($m3Last[20])/revalidate$($m3Last[21])"
                            if ($Task36HwComposeMode -eq 2) {
                                $task36ReplayLast = $task36ReplaySamples[-1]
                                $task36ReplayMetricSummary = "Renderer Task 36 replay: state=$($task36ReplayLast[1]) bake=$($task36ReplayLast[2])/$($task36ReplayLast[3])/$($task36ReplayLast[4]) frames=$($task36ReplayLast[5]) segments/runs/words=$($task36ReplayLast[6])/$($task36ReplayLast[7])/$($task36ReplayLast[8]) fallback/arena/material=$($task36ReplayLast[9])/$($task36ReplayLast[10])/$($task36ReplayLast[11]) arena=$('{0:X}' -f $task36ReplayLast[13])/$($task36ReplayLast[14])"
                            }
                            if ($benchmarkMakeIdentity.NativeStageGeneratedSegment0Enable -eq 1) {
                                $m3GeneratedSegment0Last =
                                    $m3GeneratedSegment0Samples[-1]
                                $m3GeneratedSegment0MetricSummary = "Renderer Task 26 segment 0: attempts/success/fallback=$($m3GeneratedSegment0Last[2])/$($m3GeneratedSegment0Last[3])/$($m3GeneratedSegment0Last[4]) runs/triangles/epochs/materials=$($m3GeneratedSegment0Last[5])/$($m3GeneratedSegment0Last[6])/$($m3GeneratedSegment0Last[7])/$($m3GeneratedSegment0Last[8]) certificateValidations=$($m3GeneratedSegment0Last[9])"
                                if ($RendererM3Phase0Profile) {
                                    $m3GeneratedSegment0ShadowLast =
                                        $m3GeneratedSegment0ShadowSamples[-1]
                                    $m3GeneratedSegment0ShadowMetricSummary = "Renderer Task 26 shadow: runs/dense/state/sync/epochs/triangles=$($m3GeneratedSegment0ShadowLast[2])/$($m3GeneratedSegment0ShadowLast[3])/$($m3GeneratedSegment0ShadowLast[4])/$($m3GeneratedSegment0ShadowLast[5])/$($m3GeneratedSegment0ShadowLast[6])/$($m3GeneratedSegment0ShadowLast[7]) fields/mismatches=$($m3GeneratedSegment0ShadowLast[8])/$($m3GeneratedSegment0ShadowLast[9]) faultInjected/rejected=$($m3GeneratedSegment0ShadowLast[10])/$($m3GeneratedSegment0ShadowLast[11])"
                                }
                            }
                            if ($RendererM3Phase0Profile) {
                                $phase0AttributeExclusive = @($m3Phase0Samples | ForEach-Object { [int64]$_[3] - [int64]$_[4] })
                                $phase0PrepareResidual = @($m3Phase0Samples | ForEach-Object { [int64]$_[2] - [int64]$_[3] })
                                $phase0PreflightResidual = @($m3Phase0Samples | ForEach-Object { [int64]$_[1] - [int64]$_[2] })
                                $phase0NoZExclusive = @($m3Phase0Samples | ForEach-Object { [int64]$_[8] - [int64]$_[9] })
                                $phase0CommitResidual = @($m3Phase0Samples | ForEach-Object { [int64]$_[11] - ([int64]$_[5] + [int64]$_[6] + [int64]$_[7] + [int64]$_[8] + [int64]$_[10]) })
                                $phase0CalibrationPerRead = @($m3Phase0Samples | ForEach-Object { [int64]$_.Item(14) / [int64]$_.Item(15) })
                                $phase0Last = $m3Phase0Samples[-1]
                                $m3Phase0MetricSummary = "Renderer M3 Phase 0: samples=$RendererBenchmarkSamples preflight=$(Get-MedianP95 (Get-SampleFieldValues $m3Phase0Samples 1)) prepareRuns=$(Get-MedianP95 (Get-SampleFieldValues $m3Phase0Samples 2)) attributeExclusive=$(Get-MedianP95 $phase0AttributeExclusive) nearTransform=$(Get-MedianP95 (Get-SampleFieldValues $m3Phase0Samples 4)) prepareResidual=$(Get-MedianP95 $phase0PrepareResidual) preflightResidual=$(Get-MedianP95 $phase0PreflightResidual) beginBind=$(Get-MedianP95 (Get-SampleFieldValues $m3Phase0Samples 5)) raw=$(Get-MedianP95 (Get-SampleFieldValues $m3Phase0Samples 6)) range=$(Get-MedianP95 (Get-SampleFieldValues $m3Phase0Samples 7)) noZInclusive=$(Get-MedianP95 (Get-SampleFieldValues $m3Phase0Samples 8)) noZMatrix=$(Get-MedianP95 (Get-SampleFieldValues $m3Phase0Samples 9)) noZExclusive=$(Get-MedianP95 $phase0NoZExclusive) accounting=$(Get-MedianP95 (Get-SampleFieldValues $m3Phase0Samples 10)) commit=$(Get-MedianP95 (Get-SampleFieldValues $m3Phase0Samples 11)) commitResidual=$(Get-MedianP95 $phase0CommitResidual) timerReads/spans=$($phase0Last[12])/$($phase0Last[13]) calibrationTotal/perRead=$(Get-MedianP95 (Get-SampleFieldValues $m3Phase0Samples 14))/$(Get-MedianP95 $phase0CalibrationPerRead) counts=dense$($phase0Last[16])/near$($phase0Last[17])/matrix$($phase0Last[18])"
                                $m3ResidualAvoided = @($m3ResidualSamples | ForEach-Object { [int64]$_[1] - [int64]$_[3] })
                                $m3ResidualHits = [int64](($m3ResidualSamples | ForEach-Object { $_[5] } | Measure-Object -Sum).Sum)
                                $m3ResidualMisses = [int64](($m3ResidualSamples | ForEach-Object { $_[6] } | Measure-Object -Sum).Sum)
                                $m3ResidualOpportunities = $m3ResidualHits + $m3ResidualMisses
                                $m3ResidualHitRate = if ($m3ResidualOpportunities -gt 0) { [Math]::Round((100.0 * $m3ResidualHits) / $m3ResidualOpportunities, 2) } else { 0.0 }
                                $m3ResidualMetricSummary = "Renderer Task 23R residual: prepare=$(Get-MedianP95 (Get-SampleFieldValues $m3ResidualSamples 1)) vertex=$(Get-MedianP95 (Get-SampleFieldValues $m3ResidualSamples 2)) near=$(Get-MedianP95 (Get-SampleFieldValues $m3ResidualSamples 3)) avoidedUpper=$(Get-MedianP95 $m3ResidualAvoided) key=$(Get-MedianP95 (Get-SampleFieldValues $m3ResidualSamples 4)) hits/misses/rate=$m3ResidualHits/$m3ResidualMisses/$m3ResidualHitRate% keyBytes=$($m3ResidualSamples[-1][7]) counts=runs$($m3ResidualSamples[-1][8])/dense$($m3ResidualSamples[-1][9])/near$($m3ResidualSamples[-1][10])"
                                $m3PreparedCounts = [ordered]@{
                                    comparison = 'adjacent-within-window'
                                    boundary = 'unknown'
                                    firstSampleIsOpportunity = $false
                                    samples = $m3PreparedSamples.Count
                                    opportunities = [Math]::Max(
                                        0, $m3PreparedSamples.Count - 1)
                                    segments = @(for ($i = 0; $i -lt 8; $i++) {
                                        Get-AdjacentHitChangeCounts `
                                            -Values (Get-SampleFieldValues $m3PreparedSamples (3 + $i)) `
                                            -Lane $i
                                    })
                                    materials = @(for ($i = 0; $i -lt 4; $i++) {
                                        Get-AdjacentHitChangeCounts `
                                            -Values (Get-SampleFieldValues $m3PreparedSamples (11 + $i)) `
                                            -Lane $i
                                    })
                                }
                                $m3PreparedSegmentChurn = @($m3PreparedCounts.segments | ForEach-Object { "s$($_.lane)=$($_.hits)/$($_.changes)" })
                                $m3PreparedMaterialChurn = @($m3PreparedCounts.materials | ForEach-Object { "m$($_.lane)=$($_.hits)/$($_.changes)" })
                                $m3PreparedMetricSummary = "Renderer M3 camera-independent prepared-output FNV (in-window adjacent hits/changes; boundary unknown): topology=$(Get-AdjacentChurn (Get-SampleFieldValues $m3PreparedSamples 1))/$(Get-AdjacentChurn (Get-SampleFieldValues $m3PreparedSamples 2)) segments=$($m3PreparedSegmentChurn -join ',') materials=$($m3PreparedMaterialChurn -join ',')"
                                $g2StateMetricSummary = "Renderer G2 state writes/skips: samples=$RendererBenchmarkSamples texture=$(Get-MedianP95 (Get-SampleFieldValues $g2StateSamples 1))/$(Get-MedianP95 (Get-SampleFieldValues $g2StateSamples 2)) matrixMode=$(Get-MedianP95 (Get-SampleFieldValues $g2StateSamples 3))/$(Get-MedianP95 (Get-SampleFieldValues $g2StateSamples 4)) polyFmt=$(Get-MedianP95 (Get-SampleFieldValues $g2StateSamples 5))/$(Get-MedianP95 (Get-SampleFieldValues $g2StateSamples 6))"
                                $phase05WallpaperSubtotal = @($phase05Samples | ForEach-Object { [int64]$_[1] + [int64]$_[2] + [int64]$_[3] + [int64]$_[4] + [int64]$_[5] })
                                $phase05WallpaperResidual = @(for ($i = 0; $i -lt $phase05Samples.Count; $i++) { [int64]$coarseSamples[$i][10] - [int64]$phase05WallpaperSubtotal[$i] })
                                $phase05StageExecute = @(for ($i = 0; $i -lt $phase05Samples.Count; $i++) { [int64]$coarseSamples[$i][11] - [int64]$phase05Samples[$i][8] })
                                $phase05GCDrawShell = @(for ($i = 0; $i -lt $phase05Samples.Count; $i++) { [int64]$phase05Samples[$i][7] - [int64]$phase05StageExecute[$i] - [int64]$coarseSamples[$i][12] - [int64]$coarseSamples[$i][13] - [int64]$coarseSamples[$i][14] })
                                $phase05FighterShell = Get-SampleFieldValues $phase05Samples 9
                                $phase05PresentShell = @(for ($i = 0; $i -lt $phase05Samples.Count; $i++) { [int64]$phase05Samples[$i][6] - ([int64]$phase05Samples[$i][7] + [int64]$phase05Samples[$i][8] + [int64]$phase05Samples[$i][9]) })
                                $phase05DrawOuter = @(for ($i = 0; $i -lt $phase05Samples.Count; $i++) { [int64]$coarseSamples[$i][19] - ([int64]$phase05GCDrawShell[$i] + [int64]$phase05FighterShell[$i] + [int64]$phase05PresentShell[$i]) })
                                $phase05PresentOuter = @(for ($i = 0; $i -lt $phase05Samples.Count; $i++) { [int64]$coarseSamples[$i][20] - ([int64]$phase05Samples[$i][10] + [int64]$phase05Samples[$i][11] + [int64]$phase05Samples[$i][14]) })
                                $phase05LoopOuter = @(for ($i = 0; $i -lt $phase05Samples.Count; $i++) { [int64]$coarseSamples[$i][21] - ([int64]$phase05Samples[$i][12] + [int64]$phase05Samples[$i][13]) })
                                $phase05CalibrationPerRead = @($phase05Samples | ForEach-Object { [int64]$_.Item(17) / [int64]$_.Item(18) })
                                $phase05Last = $phase05Samples[-1]
                                $phase05MetricSummary = "Renderer Phase 0.5: samples=$RendererBenchmarkSamples wallpaper setup/x/y/write/commit=$(Get-MedianP95 (Get-SampleFieldValues $phase05Samples 1))/$(Get-MedianP95 (Get-SampleFieldValues $phase05Samples 2))/$(Get-MedianP95 (Get-SampleFieldValues $phase05Samples 3))/$(Get-MedianP95 (Get-SampleFieldValues $phase05Samples 4))/$(Get-MedianP95 (Get-SampleFieldValues $phase05Samples 5)) wallpaperResidual=$(Get-MedianP95 $phase05WallpaperResidual) presentHardware=$(Get-MedianP95 (Get-SampleFieldValues $phase05Samples 6)) gcShell=$(Get-MedianP95 $phase05GCDrawShell) transition=$(Get-MedianP95 (Get-SampleFieldValues $phase05Samples 8)) fighterShell=$(Get-MedianP95 $phase05FighterShell) presentShell=$(Get-MedianP95 $phase05PresentShell) drawOuter=$(Get-MedianP95 $phase05DrawOuter) reset/tail/flushPrep=$(Get-MedianP95 (Get-SampleFieldValues $phase05Samples 10))/$(Get-MedianP95 (Get-SampleFieldValues $phase05Samples 11))/$(Get-MedianP95 (Get-SampleFieldValues $phase05Samples 14)) presentOuter=$(Get-MedianP95 $phase05PresentOuter) bookkeeping/publish/loopOuter=$(Get-MedianP95 (Get-SampleFieldValues $phase05Samples 12))/$(Get-MedianP95 (Get-SampleFieldValues $phase05Samples 13))/$(Get-MedianP95 $phase05LoopOuter) timerReads/spans=$($phase05Last[15])/$($phase05Last[16]) calibrationTotal/perRead=$(Get-MedianP95 (Get-SampleFieldValues $phase05Samples 17))/$(Get-MedianP95 $phase05CalibrationPerRead) rows/pixels=$($phase05Last[19])/$($phase05Last[20])"
                                $wallRunLast = $wallRunSamples[-1]
                                $wallRunMetricSummary = "Wallpaper runs: full/incrementalRows=$($wallRunLast[1])/$($wallRunLast[2]) changedX/runs/max=$($wallRunLast[3])/$($wallRunLast[4])/$($wallRunLast[5]) ge2=$($wallRunLast[6])/$($wallRunLast[7]) ge4=$($wallRunLast[8])/$($wallRunLast[9]) ge8=$($wallRunLast[10])/$($wallRunLast[11]) scalar16/packed32/dma/copy/pixels=$($wallRunLast[12])/$($wallRunLast[13])/$($wallRunLast[14])/$($wallRunLast[15])/$($wallRunLast[16])"
                            }
                        }
                        if ($m4CandidateEvidence) {
                            $waterStillLast = $m4WaterStillSamples[-1]
                            $m4WaterStillMetricSummary = "Renderer M4 frozen water: frozen/fail/result=$($waterStillLast[1])/$($waterStillLast[2])/$($waterStillLast[3]) sourceTriangles=12 preparedBytes=36864"
                        }
                        $m4StaticMetricSummary = "Renderer M4 static textures: mode=$effectiveStaticTextureAotMode samples=$RendererBenchmarkSamples prepare=$($m4StaticSamples[0][2]) fail=$($m4StaticSamples[0][3]) prepared=$($m4StaticSamples[0][4]) bytes=$($m4StaticSamples[0][5]) arm=$($m4StaticSamples[0][6]) seenMask=0x$('{0:x}' -f $m4StaticSamples[-1][7]) ownerMask=0x$('{0:x}' -f $m4StaticSamples[-1][8]) violation=$($m4StaticSamples[-1][9]) pinnedHits=$($m4StaticSamples[0][10])..$($m4StaticSamples[-1][10])"
                        $m4FenceLast = $m4FenceSamples[-1]
                        $m4FenceMetricSummary = "Renderer M4 post-GO texture fence: samples=$RendererBenchmarkSamples first=$($m4FenceLast[1])/$($m4FenceLast[2]) convert=$($m4FenceLast[3]) decode=$($m4FenceLast[4]) alloc=$($m4FenceLast[5]) fileIO=$($m4FenceLast[6]) glCreate=$($m4FenceLast[7]) glUpload=$($m4FenceLast[8]) glDelete=$($m4FenceLast[9]) evict=$($m4FenceLast[10]) refresh=$($m4FenceLast[11]) fallback=$($m4FenceLast[12])"
                        $texturePhaseMetricSummary = "Renderer texture phases: samples=$RendererBenchmarkSamples convert=$(Get-MedianP95 (Get-SampleFieldValues $texturePhaseSamples 1)) queue=$(Get-MedianP95 (Get-SampleFieldValues $texturePhaseSamples 2)) uploads=$(Get-MedianP95 (Get-SampleFieldValues $texturePhaseSamples 3))/$(Get-MedianP95 (Get-SampleFieldValues $texturePhaseSamples 4)) pairOracle=$(Get-MedianP95 (Get-SampleFieldValues $texturePhaseSamples 5))/$(Get-MedianP95 (Get-SampleFieldValues $texturePhaseSamples 6)) vblankQueued=$(Get-MedianP95 (Get-SampleFieldValues $texturePhaseSamples 7))/$(Get-MedianP95 (Get-SampleFieldValues $texturePhaseSamples 8)) committed=$(Get-MedianP95 (Get-SampleFieldValues $texturePhaseSamples 9)) commitTicks=$(Get-MedianP95 (Get-SampleFieldValues $texturePhaseSamples 10)) outside/fallback=$(Get-MedianP95 (Get-SampleFieldValues $texturePhaseSamples 11))/$(Get-MedianP95 (Get-SampleFieldValues $texturePhaseSamples 12)) lines=$(Get-MedianP95 (Get-SampleFieldValues $texturePhaseSamples 13))/$(Get-MedianP95 (Get-SampleFieldValues $texturePhaseSamples 14))"
                        if ($RendererM2DetailedLedger -and
                            ($m2CombinedSamples.Count -eq $RendererBenchmarkSamples)) {
                            $m2Subtotal = @($m2CombinedSamples |
                                ForEach-Object {
                                    [int64]$_[6] + [int64]$_[7] +
                                    [int64]$_[8] + [int64]$_[9] +
                                    [int64]$_[10] + [int64]$_[11] +
                                    [int64]$_[16]
                                })
                            $m2SubtotalMedian = Get-Median $m2Subtotal
                            $m2SubtotalGate = if ($m2SubtotalMedian -ge 130000) {
                                'PASS_GE_130K'
                            } else {
                                'BELOW_130K'
                            }
                            $m2MetricSummary = "Renderer M2 fighter phases: samples=$RendererBenchmarkSamples capture=$(Get-MedianP95 (Get-SampleFieldValues $m2CombinedSamples 2)) collection=$(Get-MedianP95 (Get-SampleFieldValues $m2CombinedSamples 3)) validation=$(Get-MedianP95 (Get-SampleFieldValues $m2CombinedSamples 4)) census=$(Get-MedianP95 (Get-SampleFieldValues $m2CombinedSamples 5)) camera=$(Get-MedianP95 (Get-SampleFieldValues $m2CombinedSamples 6)) hashParent=$(Get-MedianP95 (Get-SampleFieldValues $m2CombinedSamples 7)) local=$(Get-MedianP95 (Get-SampleFieldValues $m2CombinedSamples 8)) worldAffine=$(Get-MedianP95 (Get-SampleFieldValues $m2CombinedSamples 9)) worldCamera=$(Get-MedianP95 (Get-SampleFieldValues $m2CombinedSamples 10)) compose=$(Get-MedianP95 (Get-SampleFieldValues $m2CombinedSamples 11)) material=$(Get-MedianP95 (Get-SampleFieldValues $m2CombinedSamples 12)) production=$(Get-MedianP95 (Get-SampleFieldValues $m2CombinedSamples 13)) preflightState=$(Get-MedianP95 (Get-SampleFieldValues $m2CombinedSamples 14)) lighting=$(Get-MedianP95 (Get-SampleFieldValues $m2CombinedSamples 15)) rootGX=$(Get-MedianP95 (Get-SampleFieldValues $m2CombinedSamples 16)) runPrepare=$(Get-MedianP95 (Get-SampleFieldValues $m2CombinedSamples 17)) emitAccount=$(Get-MedianP95 (Get-SampleFieldValues $m2CombinedSamples 18)) residual=$(Get-MedianP95 (Get-SampleFieldValues $m2CombinedSamples 19)) worldCameraComposeRootSubtotal=$(Get-MedianP95 $m2Subtotal) gate=$m2SubtotalGate"
                            $m2ShadeEpochTotal = [int64](
                                ($m2ShadeSamples | ForEach-Object { $_[1] } |
                                    Measure-Object -Sum).Sum)
                            $m2ShadeKeyHitTotal = [int64](
                                ($m2ShadeSamples | ForEach-Object { $_[2] } |
                                    Measure-Object -Sum).Sum)
                            $m2ShadeResidentHitTotal = [int64](
                                ($m2ShadeSamples | ForEach-Object { $_[3] } |
                                    Measure-Object -Sum).Sum)
                            $m2ShadeMetricSummary = "Renderer M2 shade census: samples=$RendererBenchmarkSamples epochs/keyHits/residentHits=$m2ShadeEpochTotal/$m2ShadeKeyHitTotal/$m2ShadeResidentHitTotal key/residentRateBp=$(Get-RatioBasisPoints $m2ShadeKeyHitTotal $m2ShadeEpochTotal)/$(Get-RatioBasisPoints $m2ShadeResidentHitTotal $m2ShadeEpochTotal) dense/compute/lut/prepared/alias/material=$(Get-MedianP95 (Get-SampleFieldValues $m2ShadeSamples 5))/$(Get-MedianP95 (Get-SampleFieldValues $m2ShadeSamples 6))/$(Get-MedianP95 (Get-SampleFieldValues $m2ShadeSamples 7))/$(Get-MedianP95 (Get-SampleFieldValues $m2ShadeSamples 8))/$(Get-MedianP95 (Get-SampleFieldValues $m2ShadeSamples 9))/$(Get-MedianP95 (Get-SampleFieldValues $m2ShadeSamples 10)) collisions=$(Get-MedianP95 (Get-SampleFieldValues $m2ShadeSamples 4))"
                        }

                        $ownerLabels = @('stage', 'Mario', 'Fox')
                        if ($RendererProfileLevel -ge 2) {
                            for ($ownerIndex = 0; $ownerIndex -lt 3; $ownerIndex++) {
                                $samples = $ownerSamples[$ownerIndex]
                                $submitClasses = @()
                                foreach ($classIndex in 0..7) {
                                    $submitClasses += "class$classIndex=$(Get-MedianP95 (Get-SampleFieldValues $samples (9 + $classIndex)))"
                                }
                                $ownerCensusSummaries += "Renderer owner census $($ownerLabels[$ownerIndex]): samples=$RendererBenchmarkSamples median/p95 counts selected=$(Get-MedianP95 (Get-SampleFieldValues $samples 3)) sourceCommands=$(Get-MedianP95 (Get-SampleFieldValues $samples 4)) vertexCommands=$(Get-MedianP95 (Get-SampleFieldValues $samples 5)) sourceVertices=$(Get-MedianP95 (Get-SampleFieldValues $samples 6)) triangleCommands=$(Get-MedianP95 (Get-SampleFieldValues $samples 7)) triangles=$(Get-MedianP95 (Get-SampleFieldValues $samples 8)) $($submitClasses -join ' ') materialOps=$(Get-MedianP95 (Get-SampleFieldValues $samples 17)) matrixChanges=$(Get-MedianP95 (Get-SampleFieldValues $samples 18)) textureChanges=$(Get-MedianP95 (Get-SampleFieldValues $samples 19)) runs=$(Get-MedianP95 (Get-SampleFieldValues $samples 20))"
                                $ownerChurnSummaries += "Renderer owner churn ($RendererBenchmarkSamples frames; adjacent changes/distinct values) $($ownerLabels[$ownerIndex]): stateEntry=$(Get-AdjacentChurn (Get-SampleFieldValues $samples 21)) stateExit=$(Get-AdjacentChurn (Get-SampleFieldValues $samples 22)) cacheEntry=$(Get-AdjacentChurn (Get-SampleFieldValues $samples 23)) cacheExit=$(Get-AdjacentChurn (Get-SampleFieldValues $samples 24)) resolverEntry=$(Get-AdjacentChurn (Get-SampleFieldValues $samples 25)) resolverExit=$(Get-AdjacentChurn (Get-SampleFieldValues $samples 26)) globalEntry=$(Get-AdjacentChurn (Get-SampleFieldValues $samples 27)) globalExit=$(Get-AdjacentChurn (Get-SampleFieldValues $samples 28)) topology=$(Get-AdjacentChurn (Get-SampleFieldValues $samples 29)) selected=$(Get-AdjacentChurn (Get-SampleFieldValues $samples 30)) camera=$(Get-AdjacentChurn (Get-SampleFieldValues $samples 31)) DObj=$(Get-AdjacentChurn (Get-SampleFieldValues $samples 32)) material=$(Get-AdjacentChurn (Get-SampleFieldValues $samples 33)) light=$(Get-AdjacentChurn (Get-SampleFieldValues $samples 34)) texture=$(Get-AdjacentChurn (Get-SampleFieldValues $samples 35)) semantic=$(Get-AdjacentChurn (Get-SampleFieldValues $samples 36))"
                            }
                            $semanticOwnerMetrics = @()
                            $semanticOwnerHashChurn = @()
                            for ($ownerIndex = 0; $ownerIndex -lt 3; $ownerIndex++) {
                                $semanticBase = 5 + (11 * $ownerIndex)
                                $semanticOwnerMetrics += "$($ownerLabels[$ownerIndex])Events=$(Get-MedianP95 (Get-SampleFieldValues $semanticSamples ($semanticBase + 2))) $($ownerLabels[$ownerIndex])Overflow=$(Get-MedianP95 (Get-SampleFieldValues $semanticSamples ($semanticBase + 3))) $($ownerLabels[$ownerIndex])Occurrences=$(Get-MedianP95 (Get-SampleFieldValues $semanticSamples ($semanticBase + 4)))"
                                $semanticOwnerHashChurn += "$($ownerLabels[$ownerIndex])Hash1=$(Get-AdjacentChurn (Get-SampleFieldValues $semanticSamples $semanticBase)) $($ownerLabels[$ownerIndex])Hash2=$(Get-AdjacentChurn (Get-SampleFieldValues $semanticSamples ($semanticBase + 1)))"
                                $semanticProvenanceSummaries += "Renderer semantic first provenance ($RendererBenchmarkSamples frames; adjacent changes/distinct values) $($ownerLabels[$ownerIndex]): occurrence=$(Get-AdjacentChurn (Get-SampleFieldValues $semanticSamples ($semanticBase + 5))) list=$(Get-AdjacentChurn (Get-SampleFieldValues $semanticSamples ($semanticBase + 6))) branch=$(Get-AdjacentChurn (Get-SampleFieldValues $semanticSamples ($semanticBase + 7))) command=$(Get-AdjacentChurn (Get-SampleFieldValues $semanticSamples ($semanticBase + 8))) tri2Half=$(Get-AdjacentChurn (Get-SampleFieldValues $semanticSamples ($semanticBase + 9))) outcome=$(Get-AdjacentChurn (Get-SampleFieldValues $semanticSamples ($semanticBase + 10)))"
                            }
                            $semanticMetricSummary = "Renderer semantic benchmark: samples=$RendererBenchmarkSamples frames=$($benchFrames[0])..$($benchFrames[-1]) median/p95 counts frameEvents=$(Get-MedianP95 (Get-SampleFieldValues $semanticSamples 3)) frameOverflow=$(Get-MedianP95 (Get-SampleFieldValues $semanticSamples 4)) $($semanticOwnerMetrics -join ' ')"
                            $semanticChurnSummary = "Renderer semantic hash churn (adjacent changes/distinct values): frameHash1=$(Get-AdjacentChurn (Get-SampleFieldValues $semanticSamples 1)) frameHash2=$(Get-AdjacentChurn (Get-SampleFieldValues $semanticSamples 2)) $($semanticOwnerHashChurn -join ' ')"
                        } else {
                            $ownerCensusSummaries += 'Renderer owner detailed census: unavailable (profile 1 coarse timing only)'
                            $semanticChurnSummary = 'Renderer semantic hash churn: unavailable (profile 1 coarse timing only)'
                        }
                    }
                    $logicTickStart = $null
                    $logicTickEnd = $null
                    if ($RendererProfileLevel -ge 1) {
                        $logicTickStart = [int64]$coarseLogic[0]
                        $logicTickEnd = [int64]$coarseLogic[-1]
                    }
                    $benchmarkIdentity = [ordered]@{
                        schema = 1
                        gitHead = (& git -C $root rev-parse HEAD).Trim()
                        gitStatus = @(& git -C $root status --short)
                        focusedDiff = @(& git -C $root diff -- Makefile include/nds src/nds src/port scripts)
                        submodules = @(& git -C $root submodule status)
                        makeCommand = @('make') + @($makeArgs)
                        target = $benchmarkMakeIdentity.Target
                        build = $Build
                        harness = [ordered]@{
                            name = $benchmarkMakeIdentity.Harness
                            id = $benchmarkMakeIdentity.HarnessId
                        }
                        rendererProfile = $benchmarkMakeIdentity.Profile
                        rendererM2DetailedLedger =
                            $benchmarkMakeIdentity.M2DetailedLedger
                        rendererM3Phase0Profile =
                            $benchmarkMakeIdentity.M3Phase0Profile
                        task36HwComposeMode =
                            $benchmarkMakeIdentity.Task36HwComposeMode
                        nativeStageGeneratedSegment0Enable =
                            $benchmarkMakeIdentity.NativeStageGeneratedSegment0Enable
                        task29GxCensus =
                            [bool]($benchmarkMakeIdentity.Task29GXCensus -eq 1)
                        task20StackProfileMode =
                            $benchmarkMakeIdentity.Task20StackProfileMode
                        task32DrawHotTextMode =
                            $benchmarkMakeIdentity.Task32DrawHotTextMode
                        task22WallpaperRunLab =
                            $benchmarkMakeIdentity.Task22WallpaperRunLab
                        rendererScreenSpaceCensusMode =
                            $benchmarkMakeIdentity.ScreenSpaceCensusMode
                        renderEconomyMode =
                            $benchmarkMakeIdentity.RenderEconomyMode
                        renderEconomyOwnerMask =
                            $benchmarkMakeIdentity.RenderEconomyOwnerMask
                        runtimeM2DetailedLedger =
                            [int]$rendererM2DetailedLedgerMarker.Groups[1].Value
                        rendererBenchmarkMode =
                            $benchmarkMakeIdentity.RendererBenchmarkMode
                        rendererFastRunDefault =
                            $benchmarkMakeIdentity.FastRunDefault
                        sceneMipCacheLab =
                            $benchmarkMakeIdentity.SceneMipCacheLab
                        fastWallpaperAffine =
                            $benchmarkMakeIdentity.FastWallpaperAffine
                        battleStaticTextureDefault =
                            $benchmarkMakeIdentity.BattleStaticTextureDefault
                        staticTextureAotMode = $StaticTextureAotMode
                        effectiveStaticTextureAotMode =
                            $effectiveStaticTextureAotMode
                        ifCommonHybridOamMode =
                            $benchmarkMakeIdentity.IFCommonHybridOamMode
                        task9FloatCensusMode =
                            $benchmarkMakeIdentity.Task9FloatCensusMode
                        task9FloatItcmMode =
                            $benchmarkMakeIdentity.Task9FloatItcmMode
                        task9FloatPhase2Mode =
                            $benchmarkMakeIdentity.Task9FloatPhase2Mode
                        task16FloatCompareMode =
                            $benchmarkMakeIdentity.Task16FloatCompareMode
                        task16FloatI2fMode =
                            $benchmarkMakeIdentity.Task16FloatI2fMode
                        task16FloatAddSubMode =
                            $benchmarkMakeIdentity.Task16FloatAddSubMode
                        requestedIfCommonHybridOamMode =
                            $IFCommonHybridOamMode
                        foxCpuMode = $FoxCpuMode
                        wallpaperIncrementalMode = $WallpaperIncrementalMode
                        phaseMatrixMode = [bool]$PhaseMatrixMode
                        lowerTextHudMode = $LowerTextHudMode
                        effectiveCFlags = [ordered]@{
                            common = $benchmarkMakeIdentity.CommonCFlags
                            renderer = $benchmarkMakeIdentity.RendererCFlags
                            scene = $benchmarkMakeIdentity.SceneCFlags
                            source = 'make print-benchmark-flags'
                        }
                        artifacts = [ordered]@{
                            rom = [ordered]@{
                                path = $rom
                                sha256 = $benchmarkRomIdentity.Sha256
                                bytes = $benchmarkRomIdentity.Bytes
                                lastWriteUtc = $benchmarkRomIdentity.LastWriteUtc
                            }
                            elf = [ordered]@{
                                path = $elf
                                sha256 = $benchmarkElfIdentity.Sha256
                                bytes = $benchmarkElfIdentity.Bytes
                                lastWriteUtc = $benchmarkElfIdentity.LastWriteUtc
                            }
                        }
                        melonDS = [ordered]@{
                            version = $benchmarkMelonIdentity.Version
                            executableSha256 = $benchmarkMelonIdentity.Sha256
                            path = $benchmarkMelonIdentity.Path
                            configPath = $configState.Config
                            configSha256 = $benchmarkMelonConfigSha256
                            settings = [ordered]@{
                                limitFPS = $false
                                jit = $false
                                gdbArm9Port = $configState.GdbPort
                                gdbArm7Port = $configState.Arm7Port
                            }
                        }
                        sampling = [ordered]@{
                            warmupFrames = [Math]::Max(0, ([int64]$benchFrames[0] - 1))
                            samples = $RendererBenchmarkSamples
                            requestedStartFrame = $RendererBenchmarkStartFrame
                            requestedStartEvent = $RendererBenchmarkStartEvent
                            frameStart = [int64]$benchFrames[0]
                            frameEnd = [int64]$benchFrames[-1]
                            logicTickStart = $logicTickStart
                            logicTickEnd = $logicTickEnd
                            logicTickTimerStartResets = $logicTickResetCount
                            delaySeconds = $effectiveDelaySeconds
                        }
                    }
                    $benchmarkIdentitySummary =
                        'BENCH_IDENTITY=' +
                        ($benchmarkIdentity | ConvertTo-Json -Compress -Depth 6)
                    if ($RendererBenchmarkExportPath) {
                        $resolvedExportPath = if ([System.IO.Path]::IsPathRooted(
                                $RendererBenchmarkExportPath)) {
                            $RendererBenchmarkExportPath
                        } else {
                            Join-Path $root $RendererBenchmarkExportPath
                        }
                        $exportParent = Split-Path -Parent $resolvedExportPath
                        if ($exportParent -and -not (Test-Path -LiteralPath $exportParent)) {
                            New-Item -ItemType Directory -Path $exportParent -Force |
                                Out-Null
                        }
                        $benchmarkExport = [ordered]@{
                            schema = 1
                            kind = 'smash64ds-renderer-fast-raw-benchmark'
                            identity = $benchmarkIdentity
                            fastRunMode = $RendererFastRunMode
                            staticTextureAotMode = $StaticTextureAotMode
                            ifCommonHybridOamMode =
                                $effectiveIFCommonHybridOamMode
                            requireZeroPostGoTextureFence =
                                [bool]$RequireZeroPostGoTextureFence
                            foxCpuMode = $FoxCpuMode
                            wallpaperIncrementalMode = $WallpaperIncrementalMode
                            fastWallpaperAffineMode =
                                $effectiveFastWallpaperAffineMode
                            fastWallpaper = @(
                                if ($usesFastWallpaper -and
                                    $fastWallpaper.Success) {
                                    Get-Ints $fastWallpaper
                                }
                            )
                            task20StackProfileMode =
                                [bool]($Task20StackProfileMode -eq 1)
                            task20Stack = $task20StackEvidence
                            task22WallpaperRunLab = [bool]$Task22WallpaperRunLab
                            task29GxCensus = [bool]$Task29GXCensus
                            task34StageStreamCensus =
                                [bool]$Task34StageStreamCensus
                            task36HwComposeMode =
                                $effectiveTask36HwComposeMode
                            nativeStageGeneratedSegment0Enable =
                                $benchmarkMakeIdentity.NativeStageGeneratedSegment0Enable
                            phaseMatrixMode = [bool]$PhaseMatrixMode
                            lowerTextHudMode = $LowerTextHudMode
                            rendererScreenSpaceCensusMode =
                                $RendererScreenSpaceCensusMode
                            renderEconomyMode = $RenderEconomyMode
                            renderEconomyOwnerMask = $RenderEconomyOwnerMask
                            m3PreparedOutputCounts = $m3PreparedCounts
                            samples = [ordered]@{
                                renderer = @($rendererBenchmark | ForEach-Object {
                                    , @(Get-Ints $_)
                                })
                                coarse = @($coarseSamples)
                                texturePhases = @($texturePhaseSamples)
                                fastRaw = @($fastRunSamples)
                                sink = @($sinkSamples)
                                task20Stack = @($task20StackRows)
                                task20CoroutineCensus = @($task20CensusRows)
                                economy = @($economySamples)
                                screenSpaceCensusRows =
                                    @($screenSpaceCensusRowValues)
                                screenSpaceCensusStageOwners =
                                    @($screenSpaceCensusStageOwnerValues)
                                m3Stage = @($m3StageSamples)
                                task36Hw = @($task36HwSamples)
                                task36Replay = @($task36ReplaySamples)
                                m3GeneratedSegment0 =
                                    @($m3GeneratedSegment0Samples)
                                m3GeneratedSegment0Shadow =
                                    @($m3GeneratedSegment0ShadowSamples)
                                m3GeneratedSegment0Gx =
                                    @($m3GeneratedSegment0GxSamples)
                                m3GeneratedSegment0TraceSummary =
                                    @($m3GeneratedSegment0TraceSummaryValues)
                                m3GeneratedSegment0TraceWords =
                                    @($m3GeneratedSegment0TraceWordValues)
                                m3GeneratedSegment0TraceRuns =
                                    @($m3GeneratedSegment0TraceRunValues)
                                m3Phase0 = @($m3Phase0Samples)
                                m3Residual = @($m3ResidualSamples)
                                m3PreparedOutput = @($m3PreparedSamples)
                                m3WhispySourceState = @($m3WhispySamples)
                                g2State = @($g2StateSamples)
                                task29GxMeta = @($task29GxMetaSamples)
                                task29GxClass = @($task29GxClassSamples)
                                task29GxOwner = @($task29GxOwnerSamples)
                                task34ArenaBoot = @($task34ArenaBoot)
                                task34StageMeta = @($task34StageMetaSamples)
                                task34StageEntries =
                                    @($task34StageEntrySamples)
                                task34StageWords = @($task34StageWordSamples)
                                phase05 = @($phase05Samples)
                                wallpaperRuns = @($wallRunSamples)
                                m4WaterStill = @($m4WaterStillSamples)
                                m4Static = @($m4StaticSamples)
                                m4Fence = @($m4FenceSamples)
                                owners = @(
                                    @($ownerSamples[0]),
                                    @($ownerSamples[1]),
                                    @($ownerSamples[2])
                                )
                                m2Fighters = @(
                                    @($m2Samples[0]),
                                    @($m2Samples[1])
                                )
                                m2Combined = @($m2CombinedSamples)
                                m2Shade = @($m2ShadeSamples)
                                semantic = @($semanticSamples)
                                task9FloatPairs = @($task9Pairs)
                                task9FloatRows = @($task9FloatRows)
                                task9FloatTimer = @($task9Timer)
                                wallpaperOracle = @($wo)
                            }
                        }
                        Set-Content -LiteralPath $resolvedExportPath -Encoding utf8 `
                            -Value ($benchmarkExport | ConvertTo-Json -Depth 8)
                    }
                }
                if ($RendererBenchmarkOnly) {
                    Assert-Condition (($benchmarkMakeIdentity.RendererBenchmarkMode -gt 0) -or ($RendererFastRunMode -eq 9)) 'Benchmark-only verifier was not built with a renderer benchmark mode and did not select the complete-stage owner.' ($benchmarkMakeIdentity | Format-List | Out-String)
                    if ($m4CandidateEvidence) {
                        $banks = Get-Ints $vramBanks
                        $expectedBankF = 0x83
                        $expectedBankG = 0x8b
                        $bankModesValid =
                            $vramBanks.Success -and
                            $banks[0] -eq 0x83 -and $banks[1] -eq 0x8b -and
                            $banks[2] -eq 0x81 -and $banks[3] -eq 0x89 -and
                            $banks[4] -eq 0x82 -and
                            $banks[5] -eq $expectedBankF -and
                            $banks[6] -eq $expectedBankG -and
                            $banks[7] -eq 0x81
                        if ($isCpuPrepNoGx) {
                            Assert-Condition ($bankModesValid -and
                                @($banks[8..11] |
                                    Where-Object { $_ -ne 0 }).Count -eq 0) "CPU_PREP_NO_GX unexpectedly claimed a real static-texture VRAM span (actual=$($banks -join ','))." $gdbStdout
                        } else {
                            Assert-Condition ($bankModesValid -and
                                $banks[8] -eq 0x06800000 -and
                                $banks[9] -eq 0x06821400 -and
                                $banks[10] -eq 136192 -and
                                $banks[11] -eq 3) "M4 VRAM bank ownership or exact contiguous bank-A/B static span does not match the battle contract (actual=$($banks -join ','))." $gdbStdout
                        }
                    }
                    if ($benchmarkMakeIdentity.RendererBenchmarkMode -eq 2) {
                        $sinkProfile = Get-Ints $renderProfile
                        $sinkBatch = Get-Ints $renderBatch
                        $sinkSubmit = Get-Ints $renderSubmit
                        $sinkPlatform = Get-Ints $platformHw
                        Assert-Condition ($renderProfile.Success -and $sinkProfile[15] -eq 2484 -and $sinkProfile[16] -eq 828 -and $sinkProfile[17] -eq 0) 'CPU_PREP_NO_GX did not execute exact 2,484-vertex/828-triangle CPU preparation.' $gdbStdout
                        $expectedSinkTexturePrepareBegin =
                            if ($RendererFastRunMode -eq 9) { 49 } else { 98 }
                        $expectedSinkTexturePrepareReuse =
                            if ($RendererFastRunMode -eq 9) { 725 } else { 730 }
                        Assert-Condition ($renderBatch.Success -and $sinkBatch[0] -eq 121 -and $sinkBatch[1] -eq 707 -and $sinkBatch[2] -eq 121 -and $sinkBatch[3] -eq $expectedSinkTexturePrepareBegin -and $sinkBatch[4] -eq $expectedSinkTexturePrepareReuse) 'CPU_PREP_NO_GX drifted from exact batch and texture-preparation policy.' $gdbStdout
                        $expectedSinkRawSubmitCount = if ($RendererFastRunMode -eq 9) { 658 } else { 648 }
                        $expectedSinkRangeSubmitCount = if ($RendererFastRunMode -eq 9) { 0 } else { 10 }
                        Assert-Condition ($renderSubmit.Success -and $sinkSubmit[0] -eq $expectedSinkRawSubmitCount -and $sinkSubmit[1] -eq 0 -and $sinkSubmit[2] -eq 44 -and $sinkSubmit[3] -eq 126 -and $sinkSubmit[4] -eq 0 -and $sinkSubmit[5] -eq 0 -and $sinkSubmit[6] -eq $expectedSinkRangeSubmitCount -and $sinkSubmit[7] -eq 0) "CPU_PREP_NO_GX drifted from the exact $expectedSinkRawSubmitCount/44/126/$expectedSinkRangeSubmitCount submit-class partition." $gdbStdout
                        Assert-Condition ($platformHw.Success -and $sinkPlatform[0] -gt 0 -and $sinkPlatform[2] -eq 0 -and $sinkPlatform[3] -eq 0) 'CPU_PREP_NO_GX unexpectedly emitted GX geometry or failed to complete benchmark frames.' $gdbStdout
                    }
                    if ($benchmarkMakeIdentity.RendererBenchmarkMode -eq 4) {
                        $warmProfile = Get-Ints $renderProfile
                        $warmBatch = Get-Ints $renderBatch
                        $warmSubmit = Get-Ints $renderSubmit
                        $warmPlatform = Get-Ints $platformHw
                        Assert-Condition ($renderProfile.Success -and $warmProfile[12] -eq 0 -and $warmProfile[13] -eq 0 -and $warmProfile[15] -eq 2484 -and $warmProfile[16] -eq 828 -and $warmProfile[17] -eq 0) 'WARM_NO_UPLOAD did not preserve exact geometry or leaked a real texture upload into the warm window.' $gdbStdout
                        Assert-Condition ($renderBatch.Success -and $warmBatch[0] -eq 121 -and $warmBatch[1] -eq 707 -and $warmBatch[2] -eq 121 -and $warmBatch[3] -eq 98 -and $warmBatch[4] -eq 730) 'WARM_NO_UPLOAD drifted from exact batch and texture-preparation policy.' $gdbStdout
                        Assert-Condition ($renderSubmit.Success -and $warmSubmit[0] -eq 648 -and $warmSubmit[1] -eq 0 -and $warmSubmit[2] -eq 44 -and $warmSubmit[3] -eq 126 -and $warmSubmit[4] -eq 0 -and $warmSubmit[5] -eq 0 -and $warmSubmit[6] -eq 10 -and $warmSubmit[7] -eq 0) 'WARM_NO_UPLOAD drifted from the exact 648/44/126/10 submit-class partition.' $gdbStdout
                        # The terminal hardware counters are sampled at a later
                        # live animation/logic frame than the synchronized CPU
                        # window. Clipping can leave the final observed RAM one
                        # triangle below the canonical frozen 715/2167 value;
                        # exact 828 CPU triangles, submit classes, batches, and
                        # submitted/flush frame conservation are the benchmark
                        # geometry contract.
                        Assert-Condition ($platformHw.Success -and $warmPlatform[0] -gt 0 -and $warmPlatform[0] -eq $warmPlatform[1] -and $warmPlatform[2] -gt 0 -and $warmPlatform[3] -gt 0) 'WARM_NO_UPLOAD failed to submit and flush live GX geometry.' $gdbStdout
                    }
                    Write-Output $benchmarkIdentitySummary
                    Write-Output $benchmarkMetricSummary
                    Write-Output $benchmarkChurnSummary
                    Write-Output $coarseMetricSummary
                    $task9FloatSummaries | ForEach-Object { Write-Output $_ }
                    Write-Output $coarseResidualRatioSummary
                    Write-Output $gxBoundarySummary
                    Write-Output $stage0Summary
                    if ($sinkMetricSummary) { Write-Output $sinkMetricSummary }
                    if ($warmMetricSummary) { Write-Output $warmMetricSummary }
                    if ($texturePhaseMetricSummary) { Write-Output $texturePhaseMetricSummary }
                    if ($fastRunMetricSummary) { Write-Output $fastRunMetricSummary }
                    if ($m3StageMetricSummary) { Write-Output $m3StageMetricSummary }
                    if ($task36ReplayMetricSummary) { Write-Output $task36ReplayMetricSummary }
                    if ($m3GeneratedSegment0MetricSummary) { Write-Output $m3GeneratedSegment0MetricSummary }
                    if ($m3GeneratedSegment0ShadowMetricSummary) { Write-Output $m3GeneratedSegment0ShadowMetricSummary }
                    if ($m3GeneratedSegment0GxMetricSummary) { Write-Output $m3GeneratedSegment0GxMetricSummary }
                    if ($m3Phase0MetricSummary) { Write-Output $m3Phase0MetricSummary }
                    if ($m3ResidualMetricSummary) { Write-Output $m3ResidualMetricSummary }
                    if ($m3PreparedMetricSummary) { Write-Output $m3PreparedMetricSummary }
                    if ($g2StateMetricSummary) { Write-Output $g2StateMetricSummary }
                    if ($phase05MetricSummary) { Write-Output $phase05MetricSummary }
                    if ($wallRunMetricSummary) { Write-Output $wallRunMetricSummary }
                    if ($m4WaterStillMetricSummary) { Write-Output $m4WaterStillMetricSummary }
                    if ($m4StaticMetricSummary) { Write-Output $m4StaticMetricSummary }
                    if ($m4FenceMetricSummary) { Write-Output $m4FenceMetricSummary }
                    if ($m4FenceFinalSummary) { Write-Output $m4FenceFinalSummary }
                    if ($m2MetricSummary) { Write-Output $m2MetricSummary }
                    if ($m2ShadeMetricSummary) { Write-Output $m2ShadeMetricSummary }
                    $ownerCensusSummaries | ForEach-Object { Write-Output $_ }
                    if ($task20StackSummary) { Write-Output $task20StackSummary }
                    Write-Output "$Label renderer benchmark-only sample passed."
                    return
                }
                Assert-Condition ($rendererProfileMarker.Success -and [int64]$rendererProfileMarker.Groups[1].Value -eq $RendererProfileLevel) "Canonical realtime HW build did not report renderer profile level $RendererProfileLevel." $gdbStdout
                if ($RendererProfileLevel -ge 2) {
                    Assert-Condition $stageDepthTrace.Success 'Profile-2 renderer did not publish the exact stage depth-order trace.' $gdbStdout
                    $sdt = Get-Ints $stageDepthTrace
                    Assert-Condition (
                        $sdt[0] -eq 202 -and $sdt[1] -eq 0 -and
                        $sdt[2] -eq 0x3bb26905 -and $sdt[3] -eq 0 -and
                        $sdt[4] -eq 72 -and $sdt[5] -eq 4024 -and
                        $sdt[6] -eq 4095 -and $sdt[7] -eq 54 -and
                        $sdt[8] -eq -4022 -and $sdt[9] -eq -3969 -and
                        $sdt[10] -eq 66 -and $sdt[11] -eq 0 -and
                        $sdt[12] -eq 0 -and $sdt[13] -eq 126 -and
                        $sdt[14] -eq 0 -and $sdt[15] -eq 0 -and
                        $sdt[16] -eq 10 -and $sdt[17] -eq 0) `
                        'Profile-2 stage class/depth order diverged from BattleShip callback order or reused a synthetic painter depth.' $gdbStdout
                }
                Assert-Condition $renderTexHash.Success 'Canonical realtime HW build did not publish texture lookup accounting.' $gdbStdout
                if ($RendererProfileLevel -lt 2) {
                    # The resident texture cache survives frame boundaries, so a
                    # completed warm frame may legitimately have zero misses.
                    # Mode 7 resolves every fighter texture through the indexed
                    # table during whole-owner preflight, then commits prepared
                    # descriptors without repeating an active-entry lookup.
                    $activeHitCoverage = ($rth[2] -gt 0) -or
                        (($RendererFastRunMode -eq 7) -and ($rth[3] -gt 0))
                    Assert-Condition ($rth[0] -gt 0 -and $activeHitCoverage -and $rth[3] -gt 0 -and ($rth[2] + $rth[3] + $rth[4]) -eq $rth[0] -and $rth[1] -ge ($rth[3] + $rth[4]) -and $rth[1] -lt (4 * $rth[0])) 'Performance/coarse texture hash lookup lacked mode-applicable active/table coverage, exact accounting, or bounded probes.' $gdbStdout
                } else {
                    Assert-Condition (($rth | Measure-Object -Sum).Sum -eq 0) 'Forensic renderer unexpectedly used the performance texture hash lookup.' $gdbStdout
                }
                Assert-Condition ($platformHw.Success -and $hw[0] -gt 0 -and $hw[0] -eq $hw[1]) 'Canonical realtime HW build did not flush submitted DS 3D frames.' $gdbStdout
                Assert-Condition ($hw[2] -gt 0 -and $hw[3] -gt 0) 'Canonical realtime HW build submitted CPU-side triangles but DS GX polygon/vertex RAM stayed empty.' $gdbStdout
                if ($usesFastWallpaper) {
                    $expectedFastWallpaperTerminalState = if (
                        $MatchLifecycleProof) {
                        0
                    } else {
                        2
                    }
                    Assert-Condition (
                        $fastWallpaper.Success -and
                        $fw[0] -eq $expectedFastWallpaperTerminalState -and
                        $fw[1] -eq 1 -and
                        $fw[2] -eq 1 -and $fw[3] -eq 0 -and
                        $fw[4] -eq 0 -and $fw[15] -eq 0 -and
                        $fw[16] -eq 0 -and $fw[17] -ne 0 -and
                        $fw[18] -ge 36864 -and $fw[19] -eq 0
                    ) 'BG-0 did not retain one valid opaque seed or re-entered software/pixel writes after READY.' $gdbStdout
                    if ($RendererProfileLevel -ge 1) {
                        Assert-Condition (
                            $fw[5] -gt 0 -and $fw[6] -gt 0 -and
                            $fw[7] -gt 0 -and $fw[14] -gt 0
                        ) 'BG-0 profile build did not publish seed/queue/apply timing evidence.' $gdbStdout
                    }
                }
                $stageFrameCount = if ($usesRetainedWallpaper) {
                    $smc[6] + $smc[7]
                } else {
                    $hw[0]
                }
                $nonFireballSubmitCount = $wr[2] - $wr[13]
                $expectedWeaponKindMask =
                    $(if ($wr[13] -gt 0) { 0x1 } else { 0x0 }) -bor
                    $(if ($nonFireballSubmitCount -gt 0) { 0x2 } else { 0x0 })
                $weaponLedgerValid =
                    $weaponRenderer.Success -and
                    $wr[0] -eq $wr[1] -and $wr[1] -eq $wr[2] -and
                    $wr[2] -eq $wr[3] -and $wr[4] -eq (2 * $wr[2]) -and
                    $wr[5] -eq $wr[13] -and $wr[6] -eq 0 -and
                    $nonFireballSubmitCount -ge 0 -and
                    $wr[7] -eq $expectedWeaponKindMask -and
                    $wr[9] -eq $wr[2] -and
                    $wr[14] -eq (2 * $wr[13]) -and
                    $wr[15] -eq $wr[13] -and $wr[16] -eq 0
                if ($wr[2] -eq 0) {
                    $weaponLedgerValid = $weaponLedgerValid -and
                        (($wr | Measure-Object -Sum).Sum -eq 0)
                } else {
                    $weaponLedgerValid = $weaponLedgerValid -and
                        $wr[8] -eq 0x444c4831
                }
                Assert-Condition $weaponLedgerValid 'Canonical realtime HW build published an incoherent source weapon display ledger.' $gdbStdout
                # Every completed source traversal contributes the exact
                # 42-list / 202-triangle Dream Land base. Link-14 source
                # weapons are additive two-triangle quads. Do not admit the old
                # unmarked 22/44 setup traversal: aggregate tolerance could hide
                # an equal loss in a later gameplay frame.
                if ($RendererFastRunMode -eq 9) {
                    # The complete-stage owner bypasses these generic counters.
                    # They retain one pre-arm 42/202 startup traversal plus the
                    # lifetime source-weapon ledger.
                    $stageBaseSubmitCount = $shw[0] - $wr[2]
                    $stageBaseTriangleCount = $shw[1] - $wr[4]
                    $stageStartupSubmitCount = 0
                    $stageStartupTriangleCount = 0
                    $stageStartupValid =
                        $stageBaseSubmitCount -eq 42 -and
                        $stageBaseTriangleCount -eq 202
                } else {
                    $stageBaseSubmitCount = $shw[0] - $wr[2]
                    $stageBaseTriangleCount = $shw[1] - $wr[4]
                    $stageStartupSubmitCount =
                        $stageBaseSubmitCount - (42 * $stageFrameCount)
                    $stageStartupTriangleCount =
                        $stageBaseTriangleCount - (202 * $stageFrameCount)
                    $stageStartupValid =
                        $stageStartupSubmitCount -eq 0 -and
                        $stageStartupTriangleCount -eq 0
                }
                Assert-Condition ($stageHardware.Success -and $stageStartupValid -and $shw[1] -eq ($shw[2] + $shw[3]) -and $shw[5] -gt 0 -and $shw[6] -gt 0 -and $shw[7] -gt 0 -and $shw[8] -eq 0) 'Canonical realtime HW build drifted from the exact base + source-weapon stage contract or retained an unmarked setup traversal.' $gdbStdout
                Assert-Condition ($stageCarry.Success -and $scarry[0] -eq $scarry[1] -and $scarry[0] -gt 8 -and $scarry[2] -gt 0 -and $scarry[3] -gt 0 -and $scarry[4] -gt 0 -and $scarry[5] -gt 0) 'Canonical realtime HW build did not prove persistent stage DObj texture/tile carry.' $gdbStdout
                if ($RendererBenchmarkStartEvent -ne 'None') {
                    Assert-Condition (
                        $stageHardwareFighter.Success -and
                        $shwf[0] -gt 0 -and $shwf[1] -gt 0
                    ) 'Natural-event renderer window did not retain live source fighter ownership.' $gdbStdout
                } elseif ($usesRetainedWallpaper) {
                    Assert-Condition (
                        $sceneMipCache.Success -and
                        $smc[0] -eq 2 -and
                        $smc[1] -eq 1 -and
                        $smc[2] -eq 0 -and
                        $smc[3] -eq 0 -and
                        $smc[4] -ne 0 -and
                        $smc[5] -ge 36864 -and
                        $smc[6] -eq 1 -and
                        $smc[7] -gt 0 -and
                        $smc[8] -eq 0 -and
                        $smc[9] -eq 0 -and
                        $smc[10] -ne 0 -and
                        $smc[11] -eq 1 -and
                        $hw[0] -eq ($smc[6] + $smc[7])
                    ) 'Cut G did not retain exactly one complete BG2 wallpaper seed and continue natural live scene frames.' $gdbStdout
                    Assert-Condition (
                        $stageHardwareFighter.Success -and
                        $shwf[0] -eq (2 * $smc[7]) -and
                        $shwf[1] -eq (626 * $smc[7])
                    ) 'Cut G live frames drifted from the exact two-owner/626-triangle fighter contract.' $gdbStdout
                } else {
                    Assert-Condition ($stageHardwareFighter.Success -and $shwf[0] -eq (2 * $hw[0]) -and $shwf[1] -eq (626 * $hw[0])) 'Canonical realtime HW build drifted from the exact per-frame two-owner/626-triangle fighter contract.' $gdbStdout
                }
                # ftdisplaymain.c:1164-1242 sets this preamble and traverses only
                # source-selected, visible, textured fighter part display lists.
                Assert-Condition ($fighterDisplayContract.Success -and $fdc[0] -gt 0 -and $fdc[3] -gt 0 -and $fdc[0] -ge $fdc[3] -and ($fdc[0] - $fdc[3]) -le 64 -and (($fdc[4] -band 0x222005) -eq 0x222005) -and $fdc[5] -gt 0 -and $fdc[6] -gt 0 -and $fdc[7] -gt 0 -and $fdc[8] -eq 0 -and $fdc[11] -gt 0 -and $fdc[12] -gt 0 -and $fdc[13] -eq 0x00100000 -and $fdc[14] -eq [Convert]::ToUInt32('c4112078', 16)) 'Canonical realtime HW build did not preserve the original fighter display selection, lighting, geometry, cycle, render-mode, and visibility contract.' $gdbStdout
                # Source link-14 weapons use BattleShip wpDisplayDrawNormal's
                # no-Z path. The adjacent completed-frame delta above supplies
                # an independent owner census for this terminal geometry.
                if ($RendererBenchmarkStartEvent -ne 'None') {
                    $terminalWeaponQuadCount = 0
                    $terminalWeaponFrameValid = $rs[7] -eq 0
                } elseif ($RendererBenchmarkSamples -gt 0) {
                    # Synchronized benchmark windows already require exact base
                    # geometry on every sampled frame and deliberately reject a
                    # terminal weapon rather than inventing a cross-window delta.
                    $terminalWeaponQuadCount = 0
                    $terminalWeaponFrameValid = $rs[3] -eq 126
                } else {
                    $terminalWeaponFrameValid =
                        $weaponFrame.Success -and
                        $wframe[0] -eq $wframe[1] -and
                        $wframe[1] -eq $wframe[2] -and
                        $wframe[2] -eq $wframe[3] -and
                        $wframe[4] -eq (2 * $wframe[2]) -and
                        $wframe[5] -eq $wframe[8] -and
                        $wframe[8] -le $wframe[2] -and
                        $wframe[6] -eq 0 -and
                        $wframe[7] -eq $wframe[2] -and
                        $wframe[9] -eq (2 * $wframe[8]) -and
                        $wframe[10] -eq $wframe[8] -and
                        $wframe[11] -eq 0
                    $terminalWeaponQuadCount = $wframe[2]
                }
                if ($RendererBenchmarkStartEvent -ne 'None') {
                    $lastBenchmarkTriangles =
                        [int64]$rendererBenchmark[$rendererBenchmark.Count - 1].Groups[12].Value
                    Assert-Condition (
                        $renderProfile.Success -and
                        $terminalWeaponFrameValid -and
                        $rp[15] -eq (3 * $rp[16]) -and
                        $rp[16] -eq $lastBenchmarkTriangles -and
                        $rp[16] -ge 202 -and $rp[17] -eq 0
                    ) 'Natural-event terminal frame lost triangle/vertex conservation or renderer synchronization.' $gdbStdout
                } else {
                    Assert-Condition ($renderProfile.Success -and $terminalWeaponFrameValid -and $rp[15] -eq (2484 + (6 * $terminalWeaponQuadCount)) -and $rp[16] -eq (828 + (2 * $terminalWeaponQuadCount)) -and $rp[17] -eq 0) 'Canonical realtime HW build drifted from the exact base plus source-weapon renderer geometry contract.' $gdbStdout
                }
                if ($effectiveStaticTextureAotMode -eq 1) {
                    Assert-Condition ($rp[12] -eq 0 -and $rp[13] -eq 0) 'Static-resident realtime HW build performed a gameplay texture upload.' $gdbStdout
                } else {
                    Assert-Condition (Test-RendererUploadPair $rp[12] $rp[13]) 'Realtime HW build reported an invalid canonical texture upload count/byte pair.' $gdbStdout
                }
                $expectedBatchBegin = 121
                $expectedBatchReuse = 707
                $expectedBatchEnd = 121
                if (($RendererProfileLevel -lt 2) -and
                    ($RendererFastRunMode -in @(7, 8, 9))) {
                    # The production owner keeps identical policy epochs but
                    # carries 18 compatible fighter batches across old root
                    # boundaries. Mode 9 also halves stage texture-prepare
                    # begins by owning the complete prepared stage packet.
                    $expectedBatchBegin = 103
                    $expectedBatchReuse = 725
                    $expectedBatchEnd = 103
                }
                $expectedBatchBegin += $terminalWeaponQuadCount
                $expectedBatchReuse += $terminalWeaponQuadCount
                $expectedBatchEnd += $terminalWeaponQuadCount
                $expectedTexturePrepareBegin =
                    $(if ($RendererFastRunMode -eq 9) { 49 } else { 98 }) +
                    $terminalWeaponQuadCount
                $expectedTexturePrepareReuse =
                    $(if ($RendererFastRunMode -eq 9) { 725 } else { 730 }) +
                    $terminalWeaponQuadCount
                if ($RendererBenchmarkStartEvent -ne 'None') {
                    Assert-Condition (
                        $renderBatch.Success -and
                        $rb[0] -gt 0 -and $rb[0] -eq $rb[2] -and
                        $rb[1] -ge $rb[0] -and
                        $rb[3] -gt 0 -and $rb[4] -ge $rb[3]
                    ) 'Natural-event terminal frame published incoherent batch or texture-prepare accounting.' $gdbStdout
                } else {
                    Assert-Condition ($renderBatch.Success -and $rb[0] -eq $expectedBatchBegin -and $rb[1] -eq $expectedBatchReuse -and $rb[2] -eq $expectedBatchEnd -and $rb[3] -eq $expectedTexturePrepareBegin -and $rb[4] -eq $expectedTexturePrepareReuse) "Canonical realtime HW build drifted from exact source-weapon-aware batch and texture-prepare accounting." $gdbStdout
                }
                if ($effectiveStaticTextureAotMode -eq 1) {
                    Assert-Condition ($renderCi4Lut.Success -and (($rci4lut | Measure-Object -Sum).Sum -eq 0)) 'Static-resident renderer unexpectedly rebuilt or reused live CI4 conversion tables.' $gdbStdout
                    Assert-Condition ($renderCi4Map.Success -and (($rci4map | Measure-Object -Sum).Sum -eq 0)) 'Static-resident renderer unexpectedly resolved live water representative pixels.' $gdbStdout
                } elseif ($RendererProfileLevel -lt 2) {
                    Assert-Condition ($renderCi4Lut.Success -and $rci4lut[2] -eq 2 -and $rci4lut[3] -gt $rci4lut[2]) 'Performance/coarse renderer did not build exactly two immutable CI4 source-index planes and reuse them across live water addressing.' $gdbStdout
                    Assert-Condition ($renderCi4Map.Success -and $rci4map[0] -gt 0 -and $rci4map[1] -ge $rci4map[0]) 'Performance/coarse renderer did not resolve live large-water pixels through exact representative address/phase classes.' $gdbStdout
                } else {
                    Assert-Condition ($renderCi4Lut.Success -and $rci4lut[2] -eq 0 -and $rci4lut[3] -eq 0) 'Forensic renderer unexpectedly bypassed its independent bytewise CI4 source decoder.' $gdbStdout
                    Assert-Condition ($renderCi4Map.Success -and $rci4map[0] -eq 0 -and $rci4map[1] -eq 0) 'Forensic renderer unexpectedly reused performance CI4 representative maps.' $gdbStdout
                }
                if ($RendererProfileLevel -ge 2) {
                    Assert-Condition ($renderTopology.Success -and $rtopo[0] -gt 0 -and $rtopo[1] -gt 0 -and $rtopo[2] -gt 0) 'Profiled renderer did not hoist immutable reloc-list validation while retaining dynamic-list fallback validation.' $gdbStdout
                    Assert-Condition ($renderCost.Success -and $rcost[0] -gt 0 -and $rcost[1] -gt 0 -and $rcost[0] -ge $rcost[1]) 'Profiled renderer did not report a coherent triangle/vertex submission cost split.' $gdbStdout
                    if (($effectiveStaticTextureAotMode -eq 0) -and
                        ($rp[12] -gt 0)) {
                        Assert-Condition ($renderCi4Lut.Success -and ($rci4lut[0] + $rci4lut[1]) -gt 0) 'Profiled renderer uploaded animated textures without building or reusing the exact CI4 palette-pair LUT.' $gdbStdout
                    }
                } elseif ($RendererProfileLevel -eq 1) {
                    Assert-Condition ($renderTopology.Success -and (($rtopo | Measure-Object -Sum).Sum -eq 0) -and $renderCost.Success -and (($rcost | Measure-Object -Sum).Sum -eq 0)) 'Low-frequency O2 coarse profile unexpectedly retained detailed command/triangle profiling.' $gdbStdout
                }
                if ($usesRetainedWallpaper) {
                    Assert-Condition (
                        $wallpaperCache.Success -and
                        $wc[0] -eq 1 -and $wc[1] -eq 0 -and
                        $wc[2] -eq 1 -and $wc[3] -eq 0 -and
                        $wc[4] -eq 300 -and $wc[5] -eq 220 -and
                        $wc[6] -eq 66000 -and
                        $wc[7] -gt 0 -and $wc[8] -gt 0
                    ) 'Cut G did not construct exactly one opaque Dream Land wallpaper seed.' $gdbStdout
                    Assert-Condition (
                        $wallpaperFinal.Success -and
                        $wf[0] -eq 1 -and $wf[1] -eq 0 -and
                        $wf[2] -eq 1 -and $wf[3] -eq 49152 -and
                        $wf[4] -eq 0 -and $wf[5] -eq 0 -and
                        $wf[6] -eq 0 -and $wf[7] -eq 0 -and
                        $wf[8] -eq 98304 -and
                        $wf[9] -eq 0 -and $wf[10] -eq 0 -and
                        $wf[11] -eq 0
                    ) 'Cut G did not retain the one full BG2 seed while eliminating generic foreground staging and BG3 copies.' $gdbStdout
                    Assert-Condition (
                        $ifCommonOam.Success -and
                        $ioam[0] -eq 1 -and
                        $ioam[1] -eq 1 -and $ioam[2] -eq 1 -and
                        $ioam[3] -eq 0 -and $ioam[4] -gt 0 -and
                        $ioam[5] -eq $expectedIfCommonPrepareBytes -and
                        $ioam[6] -eq 16 -and $ioam[7] -eq 25 -and
                        $ioam[9] -eq $expectedIfCommonPaletteBytes -and
                        $ioam[10] -eq 0 -and $ioam[11] -eq 0 -and
                        $ioam[12] -eq 0 -and $ioam[13] -eq 0 -and
                        $ioam[14] -eq 0 -and $ioam[15] -eq 0 -and
                        $ioam[16] -eq 0 -and $ioam[17] -eq 1 -and
                        $ioam[18] -gt 0 -and $ioam[19] -eq 0 -and
                        $ioam[20] -eq 0 -and $ioam[21] -eq 0 -and
                        $ioam[22] -eq 0 -and $ioam[23] -eq 0 -and
                        $ioam[24] -eq 0x49464f41 -and
                        $ioam[25] -eq 0 -and $ioam[26] -gt 0 -and
                        (($RendererProfileLevel -ge 1) -or
                         ($ioam[27] -eq 0)) -and
                        $ioam[28] -eq $hw[0] -and
                        $ioam[29] -eq 0 -and $ioam[30] -eq 0
                    ) 'Cut G native countdown owner did not preserve prepared source assets, cumulative commits, exact idle cleanup, or zero hot conversion/upload.' $gdbStdout
                    Assert-Condition (
                        $sceneWallAffine.Success -and
                        $swa[0] -eq $smc[7] -and
                        $swa[1] -eq ($swa[0] + $smc[6]) -and
                        $swa[2] -eq 0 -and
                        $swa[3] -gt 0 -and $swa[3] -le 35000 -and
                        $swa[4] -gt 0 -and $swa[4] -le 0x7fff -and
                        $swa[5] -gt 0 -and $swa[5] -le 0x7fff -and
                        $swa[6] -ge 0 -and $swa[7] -ge 0 -and
                        (($swa[4] -ne 256) -or ($swa[5] -ne 256) -or
                         ($swa[6] -ne 0) -or ($swa[7] -ne 0))
                    ) 'Cut G BG2 affine updates lacked exact frame conservation, coverage, nonidentity motion, or the 35K-tick ceiling.' $gdbStdout
                } elseif ($usesFastWallpaper) {
                    Assert-Condition (
                        $wallpaperCache.Success -and
                        $wc[0] -eq 1 -and $wc[1] -eq 0 -and
                        $wc[2] -eq 1 -and $wc[3] -eq 0 -and
                        $wc[4] -eq 300 -and $wc[5] -eq 220 -and
                        $wc[6] -eq 66000 -and
                        $wc[7] -gt 0 -and $wc[8] -gt 0
                    ) 'BG-0 did not decode and software-compose exactly one Dream Land seed.' $gdbStdout
                    Assert-Condition (
                        $wallpaperFinal.Success -and
                        $wf[0] -eq 1 -and $wf[1] -eq 0 -and
                        $wf[2] -eq 1 -and $wf[3] -gt 0 -and
                        $wf[3] -le 49152 -and $wf[4] -eq 0 -and
                        $wf[6] -eq 0 -and $wf[7] -eq 0 -and
                        $wf[8] -eq (2 * $wf[3]) -and $wf[11] -eq 0
                    ) 'BG-0 performed more than its one admitted BG2 software seed or disturbed BG3 ownership.' $gdbStdout
                } else {
                    Assert-Condition ($wallpaperCache.Success -and $wc[0] -eq 1 -and $wc[1] -ge 1 -and $wc[2] -eq ($wc[0] + $wc[1]) -and $wc[3] -eq 0 -and $wc[4] -eq 300 -and $wc[5] -eq 220 -and $wc[6] -eq 66000 -and $wc[7] -gt 0 -and $wc[8] -gt 0) 'Canonical realtime HW build did not construct once and reuse the exact opaque Dream Land wallpaper decode cache.' $gdbStdout
                    $isTopHudBenchmarkBaseline =
                        ($RendererBenchmarkSamples -gt 0) -and
                        ($LowerTextHudMode -eq 0)
                    # Countdown/GO intentionally remain on the top screen, so
                    # their bounded foreground staging and final clear remain
                    # cumulative even after the steady HUD is routed below.
                    $foregroundTrafficOk =
                        $wf[5] -le ([int64]131072 * $wf[0]) -and
                        $wf[9] -le 131072 -and
                        $wf[10] -le ([int64]98304 * $wf[0])
                    if ($isTopHudBenchmarkBaseline) {
                        $foregroundTrafficOk = $foregroundTrafficOk -and
                            $wf[5] -gt 0 -and $wf[10] -gt 0
                    }
                    Assert-Condition ($wallpaperFinal.Success -and $wf[0] -eq $wc[2] -and $wf[0] -eq ($wf[1] + $wf[2]) -and $wf[2] -gt 0 -and $wf[3] -gt 0 -and $wf[3] -le (49152 * $wf[2]) -and $wf[4] -eq 0 -and $foregroundTrafficOk -and $wf[6] -eq 0 -and $wf[7] -eq 0 -and $wf[8] -eq (2 * $wf[3]) -and $wf[11] -eq 0) 'Canonical realtime HW build did not retain bounded exact final BG2/BG3 ownership or had unexpected full-screen clears/staging/copies for the selected lower-text HUD mode.' $gdbStdout
                }
                if ($RendererProfileLevel -ge 2) {
                    Assert-Condition ($wallpaperOracle.Success -and $wo[0] -gt 0 -and $wo[1] -eq 0 -and $wo[2] -gt 0 -and $wo[3] -eq 0 -and $wo[4] -eq 0 -and $wo[5] -eq 0 -and $wo[6] -eq 0 -and $wo[7] -eq 0) 'Forensic wallpaper recurrence/pixel oracle found an exact-output mismatch.' $gdbStdout
                }
                Assert-Condition ($renderClip.Success -and $rclip[0] -eq $rv[12]) 'Canonical realtime HW build did not report a consistent clipping/saturation marker.' $gdbStdout
                if ($fastIterationUnarmedM4 -and
                    ($effectiveStaticTextureAotMode -eq 1)) {
                    Assert-Condition ($renderTexel1.Success -and
                        @($rt1 | Where-Object { $_ -ne 0 }).Count -eq 0) 'Fast iteration did not leave the deliberately unarmed TEXEL0/TEXEL1 path idle.' $gdbStdout
                } elseif ($effectiveStaticTextureAotMode -eq 1) {
                    if ($effectiveTask36HwComposeMode -eq 2) {
                        Assert-Condition ($renderTexel1.Success -and
                            @($rt1 | Where-Object { $_ -ne 0 }).Count -eq 0) 'Task 36 replay re-entered live frozen-water material evaluation, refresh, eviction, or direct-CI4 gameplay work.' $gdbStdout
                    } else {
                        Assert-Condition ($renderTexel1.Success -and $rt1[0] -eq 2 -and $rt1[1] -eq $rt1[0] -and $rt1[2] -eq 0 -and $rt1[9] -eq 0 -and $rt1[10] -eq 0 -and $rt1[11] -eq 0) 'Static-resident Dream Land water drifted from its two live frozen-water TEXEL0/TEXEL1 material matches or performed refresh, eviction, or direct-CI4 gameplay work.' $gdbStdout
                    }
                } else {
                    # Legacy static-off control: the terminal frame may reuse
                    # its resident composite, while the scene-lifetime refresh
                    # and direct-pixel counters prove the animated fallback.
                    Assert-Condition ($renderTexel1.Success -and $rt1[1] -eq $rt1[0] -and $rt1[2] -eq 0 -and $rt1[9] -gt 0 -and $rt1[10] -eq 0 -and $rt1[11] -gt 0) 'Static-off realtime HW control did not refresh the source Dream Land TEXEL0/TEXEL1 water material without eviction.' $gdbStdout
                }
                Assert-Condition ($mobjAttach.Success -and $ma[0] -ge 4 -and $ma[1] -eq 0 -and $ma[2] -eq 0 -and $ma[3] -eq 0x0200 -and $ma[4] -eq 0x006b) 'Canonical realtime HW build did not normalize the live water and Whispy mixed-width O2R MObjSub fields at the BattleShip attachment boundary.' $gdbStdout
                Assert-Condition ($renderRawMatrix.Success -and $rrm[0] -gt 0) 'Canonical realtime HW build found no current-matrix ordinary-Z raw candidates.' $gdbStdout
                $submitTotal = $rs[0] + $rs[1] + $rs[2] + $rs[3] + $rs[4] + $rs[5] + $rs[6]
                $expectedProjectedDivisions = (6 * $rs[3]) + (9 * ($rs[2] + $rs[4] + $rs[5] + $rs[6]))
                $preCutoverProjectedDivisions = (9 * ($rs[0] + $rs[1] + $rs[2] + $rs[4] + $rs[5] + $rs[6])) + (6 * $rs[3])
                $expectedProjectedFallbackCount = $rs[2] + $rs[4] + $rs[5] + $rs[6]
                if ($RendererFastRunMode -eq 9) {
                    # The complete-stage owner keeps 126 projected exceptions;
                    # its ten slightly out-of-range triangles now use a scaled
                    # raw matrix so GX performs the required near-plane clip.
                    $expectedProjectedFallbackCount = 126
                }
                if (($RendererProfileLevel -lt 2) -and
                    ($RendererFastRunMode -in @(7, 8))) {
                    # The production native-fighter owner keeps the canonical
                    # cross-matrix submit class, but GX palette restores now
                    # remove those 44 triangles from projected fallback.
                    $expectedProjectedFallbackCount -= $rs[2]
                }
                if ($RendererProfileLevel -ge 2) {
                    # Forensic command diagnostics intentionally accumulate across
                    # the whole capture, while submit-class counters describe the
                    # completed frame. Every captured hardware frame has the same
                    # stable canonical geometry contract.
                    $expectedProjectedFallbackCount *= $hw[0]
                }
                $rawMatrixPartitionValid =
                    $rs[0] -eq $rrm[0] -and
                    $rs[2] -eq $rrm[2] -and
                    $(if ($RendererFastRunMode -eq 9) {
                        $rrm[1] -eq 0
                    } else {
                        $rs[6] -eq $rrm[1]
                    })
                $expectedRawSubmitCount = if ($RendererFastRunMode -eq 9) { 658 } else { 648 }
                $expectedRangeSubmitCount = if ($RendererFastRunMode -eq 9) { 0 } else { 10 }
                if ($RendererBenchmarkStartEvent -ne 'None') {
                    Assert-Condition (
                        $renderSubmit.Success -and
                        $rs[0] -gt 0 -and $rs[1] -eq 0 -and
                        $rs[2] -ge 0 -and $rs[3] -ge 126 -and
                        $rs[4] -eq 0 -and $rs[5] -eq 0 -and
                        $rs[6] -eq 0 -and $rs[7] -eq 0 -and
                        $rawMatrixPartitionValid -and
                        $submitTotal -eq $rp[16]
                    ) 'Natural-event terminal frame lost hybrid submit-class conservation or reported a rejected triangle.' $gdbStdout
                } else {
                    Assert-Condition ($renderSubmit.Success -and $rs[0] -eq $expectedRawSubmitCount -and $rs[1] -eq 0 -and $rs[2] -eq 44 -and $rs[3] -eq (126 + (2 * $terminalWeaponQuadCount)) -and $rs[4] -eq 0 -and $rs[5] -eq 0 -and $rs[6] -eq $expectedRangeSubmitCount -and $rs[7] -eq 0 -and $rawMatrixPartitionValid -and $submitTotal -eq $rp[16]) 'Canonical realtime HW build drifted from the exact base plus source-weapon hybrid submit-class partition.' $gdbStdout
                }
                Assert-Condition ($rs[8] -eq $expectedProjectedDivisions -and ($rs[8] * 4) -lt $preCutoverProjectedDivisions) 'Canonical realtime HW build did not sharply reduce and exactly account projected division demand.' $gdbStdout
                $hardwareDivideEvaluations = $rhdiv[0] + $rhdiv[1] + $rhdiv[2]
                Assert-Condition ($renderHardwareDivide.Success -and $rhdiv[3] -eq 0 -and $rhdiv[4] -eq 0) 'Canonical realtime projected hardware divider reported a zero denominator or exact-result mismatch.' $gdbStdout
                if ($RendererProfileLevel -lt 2) {
                    Assert-Condition ($hardwareDivideEvaluations -eq 0) 'Shipping-equivalent profile retained hot-loop hardware-divider telemetry.' $gdbStdout
                } else {
                    $expectedHardwareDivideEvaluations = $rs[8] + (3 * ($rs[2] + $rs[4] + $rs[5] + $rs[6]))
                    Assert-Condition ($rhdiv[0] -gt 0 -and $hardwareDivideEvaluations -eq $expectedHardwareDivideEvaluations) 'Forensic renderer did not compare every live DS hardware quotient with the former exact C result.' $gdbStdout
                }
                if ($RendererFastRunMode -eq 9) {
                    Assert-Condition ($renderLazy.Success -and $rlazy[0] -gt 0 -and $rlazy[1] -eq 0 -and $rlazy[2] -eq 0 -and $rlazy[3] -eq 0 -and $rlazy[4] -eq 0 -and $rlazy[5] -eq 0) 'Complete-stage owner unexpectedly entered the generic transform or matrix-snapshot path.' $gdbStdout
                } else {
                    Assert-Condition ($renderLazy.Success -and $rlazy[0] -gt 0 -and $rlazy[3] -gt 0 -and $rlazy[4] -gt 0 -and $rlazy[5] -eq 0) 'Canonical realtime HW matrix snapshot table lacked natural load/create/reuse coverage or overflowed.' $gdbStdout
                }
                Assert-Condition ($renderCombine.Success -and $rc[4] -eq $expectedProjectedFallbackCount) 'Canonical realtime HW projected-fallback accounting does not match the exceptional source-Z classes.' $gdbStdout
                Assert-Condition ($fighterLightSeed.Success -and $fls[0] -gt 0 -and $fls[1] -eq [Convert]::ToUInt32('ffffff00', 16) -and $fls[2] -eq [Convert]::ToUInt32('4c4c4c00', 16)) 'Canonical realtime HW build did not seed the fighter RSP light state from the selected source MObj material.' $gdbStdout
                if ($RendererProfileLevel -ge 2) {
                    Assert-Condition ($rlazy[1] -eq $rlazy[0] -and $rlazy[2] -gt 0) 'Forensic renderer did not eagerly transform every source vertex before exercising transform-cache hits.' $gdbStdout
                    Assert-Condition ($renderOracle.Success -and $ro[0] -eq (2484 + (6 * $terminalWeaponQuadCount)) -and $ro[1] -eq 0 -and $ro[2] -eq 0) 'Forensic realtime HW drifted from the exact source-weapon-aware CPU 20.12 oracle contract.' $gdbStdout
                    Assert-Condition ($renderMatrix.Success) 'Forensic realtime HW build did not report loaded GX matrix ranges.' $gdbStdout
                    Assert-Condition ($renderVertex.Success) 'Forensic realtime HW build did not report submitted vertex ranges.' $gdbStdout
                    Assert-Condition ($renderDepth.Success -and $rd[0] -gt 0 -and $rd[5] -gt 0 -and $rd[10] -gt 0) 'Forensic realtime HW build did not report source-depth samples for the stage, Mario, and Fox.' $gdbStdout
                    Assert-Condition ($rd[1] -ge -4096 -and $rd[2] -le 4095 -and $rd[1] -le $rd[2] -and $rd[3] -gt 0 -and $rd[3] -le $rd[4] -and $rd[6] -ge -4096 -and $rd[7] -le 4095 -and $rd[6] -le $rd[7] -and $rd[8] -gt 0 -and $rd[8] -le $rd[9] -and $rd[11] -ge -4096 -and $rd[12] -le 4095 -and $rd[11] -le $rd[12] -and $rd[13] -gt 0 -and $rd[13] -le $rd[14]) 'Forensic realtime HW source-depth samples left signed 20.12 NDC or reported invalid clip W.' $gdbStdout
                    Assert-Condition ($renderTexture.Success -and $rt[0] -gt 0 -and $rt[1] -gt 0 -and $rt[2] -gt 0 -and $rt[3] -gt 0 -and $rt[4] -gt 0 -and $rt[5] -gt 0 -and $rt[6] -gt 0 -and $rt[8] -lt $rt[9] -and $rt[10] -lt $rt[11]) 'Forensic realtime HW build did not prove Dream Land texture data and texcoords reached GX submission.' $gdbStdout
                    Assert-Condition ($rt1[3] -eq 0 -and $rt1[4] -le 0xff -and $rt1[5] -ne 0 -and $rt1[6] -ne 0 -and $rt1[5] -ne $rt1[6]) 'Forensic realtime HW build did not retain TEXEL0/TEXEL1 source-state diagnostics.' $gdbStdout
                    Assert-Condition ($renderTexUse.Success) 'Forensic realtime HW build did not report texture-use rejection classes.' $gdbStdout
                    Assert-Condition ($renderTexFmt.Success -and ((($effectiveStaticTextureAotMode -eq 1) -and ($rtf[0] -eq 0)) -or (($effectiveStaticTextureAotMode -eq 0) -and ($rtf[0] -ne 0))) -and $rtf[1] -ne 0 -and $rtf[4] -eq 0) 'Forensic realtime HW texture format accounting did not match the selected static-residency mode.' $gdbStdout
                    Assert-Condition ($renderTexLane.Success -and (($rtl[0] -band 0x2) -ne 0) -and $rtl[1] -gt 0 -and $rtl[2] -gt 0 -and (($rtl[3] -band 0x100) -ne 0) -and $rtl[5] -eq 0x00010203 -and $rtl[6] -eq 0x02030001) 'Forensic realtime HW build did not prove O2R texture byte-lane decoding on the active CI4 path.' $gdbStdout
                    Assert-Condition ($rc[0] -gt 0 -and $rc[1] -gt 0) 'Forensic realtime HW build did not report source combine modes.' $gdbStdout
                    Assert-Condition ($renderLight.Success -and $rl[2] -eq 0) 'Forensic realtime HW build reported a source-light fallback.' $gdbStdout
                    Assert-Condition ($renderAdapterCache.Success -and $rac[0] -gt $rac[1] -and $rac[1] -gt 0 -and $rac[2] -eq 0 -and $rac[3] -gt 0 -and $rac[4] -gt 0 -and $rac[5] -eq 0) 'Forensic realtime HW build did not reuse frame-local camera/DObj matrices without cache overflow.' $gdbStdout
                    Assert-Condition ($renderStageWorldCache.Success -and $rswc[0] -gt 0 -and $rswc[3] -eq 0 -and $rswc[4] -gt 0 -and $rswc[5] -eq 0) 'Forensic persistent stage-world cache did not reuse exact source keys or its uncached matrix shadow mismatched.' $gdbStdout
                    Assert-Condition ($renderAffineMatrix.Success -and $ram[0] -gt 0 -and $ram[1] -eq 0 -and $ram[2] -eq 0) 'Forensic affine DObj matrix path drifted from the former exact generic multiply.' $gdbStdout
                    Assert-Condition ($rrm[3] -gt 0 -and $rrm[4] -eq 0 -and $rrm[5] -le 16 -and $rrm[6] -eq 0 -and $rrm[7] -eq 0 -and $rrm[8] -gt 0 -and $rrm[9] -eq 0) 'Forensic corrected composed GX matrix PosTest failed homogeneous, W-sign, clip, matrix-word, or capacity checks.' $gdbStdout
                } elseif ($RendererFastRunMode -eq 9) {
                    Assert-Condition ($rlazy[1] -eq 0 -and $rlazy[2] -eq 0) 'Complete-stage owner unexpectedly retained generic CPU transform or projected-cache work.' $gdbStdout
                    Assert-Condition ($renderOracle.Success -and $ro[0] -eq 0 -and $ro[1] -eq 0 -and $ro[2] -eq 0) 'Complete-stage owner still performed forensic oracle transforms.' $gdbStdout
                    Assert-Condition ($rv[12] -eq 0) 'Complete-stage owner saturated a submitted DS vertex.' $gdbStdout
                    Assert-Condition ($rt[0] -eq 0 -and $rt[3] -eq 0 -and $rt[4] -eq 0 -and $rc[0] -eq 0 -and $rc[1] -eq 0) 'Complete-stage owner still published per-vertex or per-command forensic diagnostics.' $gdbStdout
                    Assert-Condition ($rrm[3] -eq 0 -and $rrm[4] -eq 0 -and $rrm[5] -eq 0 -and $rrm[6] -eq 0 -and $rrm[7] -eq 0 -and $rrm[8] -eq 0 -and $rrm[9] -eq 0) 'Complete-stage owner still ran the GX PosTest oracle.' $gdbStdout
                } else {
                    Assert-Condition ($rlazy[1] -gt 0 -and (2 * $rlazy[1]) -lt $rlazy[0] -and $rlazy[2] -gt 0) 'Performance/coarse renderer did not defer a majority of CPU vertex transforms or reuse projected exceptions.' $gdbStdout
                    Assert-Condition ($renderOracle.Success -and $ro[0] -eq 0 -and $ro[1] -eq 0 -and $ro[2] -eq 0) 'Performance/coarse realtime HW build still performed forensic oracle transforms.' $gdbStdout
                    Assert-Condition ($rv[12] -eq 0) 'Performance/coarse realtime HW build saturated a submitted DS vertex.' $gdbStdout
                    Assert-Condition ($rt[0] -eq 0 -and $rt[3] -eq 0 -and $rt[4] -eq 0 -and $rc[0] -eq 0 -and $rc[1] -eq 0) 'Performance/coarse realtime HW build still published per-vertex or per-command forensic diagnostics.' $gdbStdout
                    Assert-Condition ($rrm[3] -eq 0 -and $rrm[4] -eq 0 -and $rrm[5] -eq 0 -and $rrm[6] -eq 0 -and $rrm[7] -eq 0 -and $rrm[8] -eq 0 -and $rrm[9] -eq 0) 'Performance/coarse realtime HW build still ran the GX PosTest oracle.' $gdbStdout
                }
                $hardwareSummary = " rprof=$RendererProfileLevel$benchmarkSummary gxram=$($hw[2])/$($hw[3]) gxstat=0x{0:x}/ctrl=0x{1:x} ftrContract=$($fdc[0])/$($fdc[3])/geom0x{17:x}/cycle0x{18:x}/rm0x{19:x}/light$($fdc[5])/$($fdc[6])/bounds$($fdc[7])/$($fdc[8]) oracle=$($ro[0])/$($ro[1])/$($ro[2]) batch=$($rb[0])/$($rb[1])/$($rb[2])/texprep$($rb[3])/$($rb[4]) wallCache=$($wc[0])/$($wc[1])/$($wc[2])/fb$($wc[3])/src$($wc[4])x$($wc[5])/$($wc[6])/ticks$($wc[7])/$($wc[8]) wallFinal=direct$($wf[0])/skip$($wf[1])/change$($wf[2])/px$($wf[3])/stage$($wf[4])/$($wf[5])/bg2$($wf[6])/$($wf[7])/$($wf[8])/bg3$($wf[9])/$($wf[10])/$($wf[11]) mtx=load$($rm[0])/scale$($rm[1])/p$($rm[2]),$($rm[3]),$($rm[4]),$($rm[5])/mv$($rm[6]),$($rm[7]),$($rm[8]),$($rm[9]),$($rm[10]),$($rm[11]) submit=raw$($rs[0])/snap$($rs[1])/cross$($rs[2])/noz$($rs[3])/dec$($rs[4])/prim$($rs[5])/range$($rs[6])/rej$($rs[7])/div$($rs[8]) lazy=load$($rlazy[0])/xf$($rlazy[1])/hit$($rlazy[2])/new$($rlazy[3])/reuse$($rlazy[4])/ovf$($rlazy[5]) rawcand=$($rrm[0])/$($rrm[1])/$($rrm[2]) postest=$($rrm[3])/$($rrm[4])/e$($rrm[5])/w$($rrm[6])/c$($rrm[7])/mw$($rrm[8])/drop$($rrm[9]) vraw=$($rv[0])..$($rv[1])/$($rv[2])..$($rv[3])/$($rv[4])..$($rv[5]) vhw=$($rv[6])..$($rv[7])/$($rv[8])..$($rv[9])/$($rv[10])..$($rv[11]) depth=stage$($rd[0]):$($rd[1])..$($rd[2])/p0$($rd[5]):$($rd[6])..$($rd[7])/p1$($rd[10]):$($rd[11]) clip=$($rclip[0]) texProof=$($rt[1])/$($rt[2])/$($rt[3]) sample=$($rt[5])/$($rt[6])/$($rt[4]) alias=$($rt[7]) st=$($rt[8])..$($rt[9])/$($rt[10])..$($rt[11]) texUse=$($rtu[0])/$($rtu[1])/$($rtu[2])/$($rtu[3])/$($rtu[4])/impl$($rtu[5])/first0x{7:x}/flags0x{8:x}/w0x{9:x}/w1x{10:x}/geom0x{11:x} texFmt=conv0x{2:x}/bind0x{3:x}/pal0x{4:x}/rej0x{5:x}/why0x{6:x} texLane=layout0x{12:x}/byte$($rtl[1])/half$($rtl[2])/bFmt0x{13:x}/hFmt0x{14:x}/bMap0x{15:x}/hMap0x{16:x} stageCarry=$($scarry[0])/$($scarry[1])/tex$($scarry[2])/tile$($scarry[3])/short$($scarry[4])/$($scarry[5])/seg$($scarry[6]) stageAcct=frames$stageFrameCount/boot$stageStartupSubmitCount,$stageStartupTriangleCount/weapon$($wr[2]),$($wr[4])/terminal$terminalWeaponQuadCount combine=$($rc[0])/$($rc[1])/lit$($rc[2])/mat$($rc[3])/proj$($rc[4]) light=$($rl[0])/$($rl[1])/$($rl[2]) profile=present$($rp[2])/draw$($rp[3])/stage$($rp[5])/mat$($rp[6])/dl$($rp[8])/tex$($rp[9])/conv$($rp[10])/upload$($rp[11]) texUploads=$($rp[12])/$($rp[13]) binds=$($rp[14]) vtx=$($rp[15]) tri=$($rp[16])" -f $hw[4], $hw[5], $rtf[0], $rtf[1], $rtf[2], $rtf[3], $rtf[4], $rtu[6], $rtu[7], $rtu[8], $rtu[9], $rtu[10], $rtl[0], $rtl[3], $rtl[4], $rtl[5], $rtl[6], $fdc[4], $fdc[13], $fdc[14]
                $hardwareSummary += " texDirect=$($rt1[11])"
                $hardwareSummary += $publishedRendererDefaultsSummary
                if ($usesFastWallpaper) {
                    $hardwareSummary += " fastWall=state$($fw[0])/seed$($fw[1])/$($fw[2])/$($fw[3])/degraded$($fw[4])/seedTicks$($fw[5])/queue$($fw[6])/apply$($fw[7])/skip$($fw[8])/clamp$($fw[9])/$($fw[10])/$($fw[11])/invalid$($fw[12])/reuse$($fw[13])/affine$($fw[14])/post$($fw[15])/$($fw[16])/hash0x$('{0:x}' -f $fw[17])/opaque$($fw[18])/restore$($fw[19])"
                }
                if ($RendererProfileLevel -ge 2) {
                    $hardwareSummary += " wallOracle=$($wo[0])/$($wo[1])/$($wo[2])/$($wo[3])"
                    $hardwareSummary += " texel1state=0x{0:x}/0x{1:x}" -f $rt1[7], $rt1[8]
                    $hardwareSummary += " adapterCache=$($rac[0])/$($rac[1])/$($rac[2])/$($rac[3])/$($rac[4])/$($rac[5])"
                    $hardwareSummary += " stageWorld=$($rswc[0])/$($rswc[1])/reject$($rswc[2])/overflow$($rswc[3])/oracle$($rswc[4])/$($rswc[5])"
                    $hardwareSummary += " affineMtx=$($ram[0])/$($ram[1])/$($ram[2])"
                }
                if ($RendererProfileLevel -ge 1) {
                    $hardwareSummary += " hdiv=$($rhdiv[0])/$($rhdiv[1])/$($rhdiv[2])/z$($rhdiv[3])/mis$($rhdiv[4])"
                    $hardwareSummary += " topology=$($rtopo[0])/$($rtopo[1])/$($rtopo[2])/$($rtopo[3])"
                    $hardwareSummary += " cost=$($rcost[0])/$($rcost[1])"
                    $hardwareSummary += " ci4lut=$($rci4lut[0])/$($rci4lut[1])/idx$($rci4lut[2])/$($rci4lut[3])"
                    $hardwareSummary += " ci4map=$($rci4map[0])/$($rci4map[1])"
                }
                $hardwareSummary += " texHash=$($rth[0])/$($rth[1])/$($rth[2])/$($rth[3])/$($rth[4])"
            }
            if ($LiveInputPreview) {
                $livePad = [regex]::Match($gdbStdout, 'LIVE_PAD=([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(-?[0-9]+),(-?[0-9]+),(0x[0-9a-fA-F]+|0),(-?[0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(-?[0-9]+),(-?[0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+)')
                $lpv = Get-Ints $livePad
                Assert-Condition ($livePad.Success -and $lpv[0] -gt 0 -and $lpv[1] -gt 0 -and (($lpv[2] -band 3) -eq 3) -and $lpv[7] -eq 0 -and $lpv[8] -eq 0 -and $lpv[9] -eq 0 -and $lpv[10] -eq 0 -and $lpv[11] -eq 0 -and $lpv[16] -eq 0 -and $lpv[17] -eq 0 -and $lpv[18] -eq 0 -and $lpv[19] -eq 0) 'Canonical realtime build did not use live DS input with connected-neutral pad1.' $gdbStdout
            }
            if ($ImportBattleShipFTComputer -and ($FoxCpuMode -eq 1)) {
                $cpu = Get-Ints $computerAI
                if ($BattlePlayable -and $preGoState) {
                    Assert-Condition (
                        $computerAI.Success -and
                        $cpu[0] -eq 1 -and $cpu[1] -ge 2 -and
                        $cpu[2] -eq 0 -and $cpu[3] -eq 0 -and
                        $cpu[6] -eq 0 -and $cpu[7] -eq 0 -and
                        $cpu[8] -eq 0 -and $cpu[9] -eq 0 -and
                        $cpu[10] -eq 0 -and $cpu[11] -eq 0 -and
                        $cpu[12] -eq 0 -and $cpu[13] -eq 0
                    ) 'Cut G ran imported Fox CPU control before the source GO transition.' $gdbStdout
                } else {
                    Assert-Condition ($computerAI.Success -and $cpu[0] -eq 1 -and $cpu[1] -ge 2 -and $cpu[2] -gt 0 -and $cpu[3] -gt 0 -and $cpu[7] -gt 0 -and $cpu[20] -gt 0) 'Canonical realtime build did not run the imported Fox CPU setup/process/target/movement path.' $gdbStdout
                }
                $hardwareSummary += " cpu=setup$($cpu[0])/proc$($cpu[2])/target$($cpu[3])/stick$($cpu[7])/obj0x$('{0:x}' -f $cpu[4])"
            } elseif ($ImportBattleShipFTComputer) {
                $cpu = Get-Ints $computerAI
                Assert-Condition (
                    $computerAI.Success -and
                    $cpu[0] -eq 1 -and $cpu[1] -ge 2 -and
                    $cpu[2] -eq 0 -and $cpu[3] -eq 0 -and
                    $cpu[4] -eq 0 -and $cpu[5] -eq 0 -and
                    $cpu[6] -eq 0 -and $cpu[7] -eq 0 -and
                    $cpu[8] -eq 0 -and $cpu[9] -eq 0 -and
                    $cpu[10] -eq 0 -and $cpu[11] -eq 0 -and
                    $cpu[12] -eq 0 -and $cpu[13] -eq 0 -and
                    $cpu[14] -eq 0 -and $cpu[15] -eq 0
                ) 'Fox-CPU-paused mode did not preserve original setup while suppressing its imported decision/input path.' $gdbStdout
                $hardwareSummary += " cpu=paused/setup$($cpu[0])/proc$($cpu[2])"
            }
            if ($usesRetainedWallpaper -and $ImportBattleShipIFCommon) {
                $ih = Get-Ints $ifHud
                $p0DigitsOk = ($ih[4] -eq 0) -or
                    (Test-DamageDigits -Damage $ih[4] -DigitCount $ih[6] -DigitsPack $ih[8])
                $p1DigitsOk = ($ih[5] -eq 0) -or
                    (Test-DamageDigits -Damage $ih[5] -DigitCount $ih[7] -DigitsPack $ih[9])
                Assert-Condition (
                    $ifHud.Success -and $ih[0] -gt 0 -and
                    (($ih[1] -band 0x3) -eq 0x3) -and
                    $ih[2] -le $ih[4] -and $ih[3] -le $ih[5] -and
                    $p0DigitsOk -and $p1DigitsOk
                ) 'Cut G did not preserve coherent source IFCommon damage-interface state.' $gdbStdout
                $hardwareSummary += " start=$($bs[0])/timer$($bs[4])/$($bs[2])/$($bs[3])/lock$($bs[5])/$($bs[6]) hud=0x$('{0:x}' -f $ih[1])"
            }
            if ($ImportBattleShipAudioBGM) {
                $ab = Get-Ints $audioBgm
                $ad = Get-Ints $audioBgmAdpcm
                Assert-Condition ($audioBgm.Success -and $audioBgmAdpcm.Success -and $ab[0] -eq 0x42474d31 -and (($ab[1] -band 0x1) -eq 0x1) -and (($ab[2] -eq 1) -or ($ab[6] -ge 1)) -and $ab[3] -eq 0 -and $ab[4] -eq 0x7800 -and $ab[5] -ge 1 -and $ab[9] -eq 0 -and $ab[10] -eq 0 -and $ab[11] -eq 0 -and $ab[13] -eq 16392 -and $ab[14] -ge 4 -and $ab[14] -le 8196 -and $ab[19] -ge 42100 -and $ab[19] -le 46100 -and $ab[20] -eq 44100 -and $ab[22] -ge 4 -and $ab[23] -lt 2886710 -and (($ab[24] -eq 0) -or ($ab[24] -eq 8196)) -and (($ab[25] -eq 0) -or ($ab[25] -eq 1)) -and (($ab[26] -eq 0) -or ($ab[26] -eq 1)) -and $ab[25] -ne $ab[26] -and $ab[27] -eq 0 -and $ab[28] -gt 0 -and $ab[29] -gt 0 -and $ad[0] -eq 0 -and $ad[1] -eq 0 -and $ad[2] -gt 0 -and $ad[3] -gt 0 -and $ad[4] -eq 0 -and $ad[5] -eq 0 -and $ad[7] -eq 0 -and $ad[8] -eq 0) 'Minimal BGM ADPCM realtime smoke failed rate, residency, packet, seam, or cleanup guards.' $gdbStdout
            }
            if ($RendererBenchmarkSamples -gt 0) {
                Write-Output $benchmarkIdentitySummary
                Write-Output $benchmarkMetricSummary
                Write-Output $benchmarkChurnSummary
                if ($RendererProfileLevel -ge 1) {
                    Write-Output $coarseMetricSummary
                    $task9FloatSummaries | ForEach-Object { Write-Output $_ }
                    Write-Output $coarseResidualRatioSummary
                    Write-Output $gxBoundarySummary
                    Write-Output $stage0Summary
                    if ($texturePhaseMetricSummary) { Write-Output $texturePhaseMetricSummary }
                    if ($fastRunMetricSummary) { Write-Output $fastRunMetricSummary }
                    if ($m3StageMetricSummary) { Write-Output $m3StageMetricSummary }
                    if ($task36ReplayMetricSummary) { Write-Output $task36ReplayMetricSummary }
                    if ($m3GeneratedSegment0MetricSummary) { Write-Output $m3GeneratedSegment0MetricSummary }
                    if ($m3GeneratedSegment0ShadowMetricSummary) { Write-Output $m3GeneratedSegment0ShadowMetricSummary }
                    if ($m3GeneratedSegment0GxMetricSummary) { Write-Output $m3GeneratedSegment0GxMetricSummary }
                    if ($m3Phase0MetricSummary) { Write-Output $m3Phase0MetricSummary }
                    if ($m3ResidualMetricSummary) { Write-Output $m3ResidualMetricSummary }
                    if ($m3PreparedMetricSummary) { Write-Output $m3PreparedMetricSummary }
                    if ($g2StateMetricSummary) { Write-Output $g2StateMetricSummary }
                    if ($phase05MetricSummary) { Write-Output $phase05MetricSummary }
                    if ($wallRunMetricSummary) { Write-Output $wallRunMetricSummary }
                    if ($m4WaterStillMetricSummary) { Write-Output $m4WaterStillMetricSummary }
                    if ($m4StaticMetricSummary) { Write-Output $m4StaticMetricSummary }
                    if ($m4FenceMetricSummary) { Write-Output $m4FenceMetricSummary }
                    if ($m4FenceFinalSummary) { Write-Output $m4FenceFinalSummary }
                    if ($m2MetricSummary) { Write-Output $m2MetricSummary }
                    if ($m2ShadeMetricSummary) { Write-Output $m2ShadeMetricSummary }
                    $ownerCensusSummaries | ForEach-Object { Write-Output $_ }
                    $ownerChurnSummaries | ForEach-Object { Write-Output $_ }
                    if ($RendererProfileLevel -ge 2) {
                        Write-Output $semanticMetricSummary
                        Write-Output $semanticChurnSummary
                        $semanticProvenanceSummaries | ForEach-Object { Write-Output $_ }
                    } else {
                        Write-Output $semanticChurnSummary
                    }
                }
                if ($task20StackSummary) { Write-Output $task20StackSummary }
            }
            Write-Output ("$Label realtime pacing smoke passed: frames=$($bp[3]) fps=$($bp[6])/$($bp[7]) ticks=$($bp[5])$hardwareSummary$aobjSummary")
            return
        }
        if ($CPUOpponentProof) {
            $cpu = Get-Ints $computerAI
            # ftcomputer.c:6326 selects Attack inside 350 units; :7591-7592
            # dispatches that objective, and :3440-3460 emits A/B/Z commands.
            # ftmain.c:198-327 owns the resulting live attack-collision state.
            Assert-Condition ($computerAI.Success -and $cpu[0] -eq 1 -and $cpu[1] -ge 2 -and $cpu[2] -ge 1000 -and $cpu[3] -gt 0 -and (($cpu[4] -band 0x4) -eq 0x4) -and $cpu[6] -gt 0 -and $cpu[7] -gt 0 -and $cpu[8] -gt 0 -and $cpu[9] -gt 0 -and $cpu[10] -gt 0 -and $cpu[11] -gt 0 -and $cpu[12] -gt 0 -and $cpu[13] -gt 0 -and $cpu[15] -gt 0 -and $cpu[19] -gt 0 -and $cpu[20] -gt 0 -and ($cpu[23] - $cpu[22]) -ge 50000) 'Imported Fox CPU did not naturally target, move, attack with live hitboxes, guard, and damage Mario on Dream Land.' $gdbStdout
            if ($MatchLifecycleProof) {
                $life = Get-Ints $battleLifecycle
                $results = Get-Ints $vsResults
                $resultsFighters = Get-Ints $vsResultsFighters
                $resultsDisplay = Get-Ints $vsResultsDisplay
                # ifcommon.c:2472-2529 decrements the source timer and dispatches
                # Time Up at zero; :3144-3152 requests LoadScene after the end
                # interface; scvsbattle.c:513-560 owns the VS Results transition.
                # ifcommon.c:2486-2537 converts the configured minute count to
                # source tics and expires through the unchanged timer path.
                Assert-Condition ($battleLifecycle.Success -and $life[0] -eq 0x5642454e -and $life[1] -ge 1 -and $life[2] -ge 1 -and $life[3] -eq 1 -and $life[4] -eq 1 -and $life[5] -eq 0 -and $life[6] -ge 3600 -and $life[7] -eq 7 -and $life[8] -eq 22 -and $life[9] -eq 24 -and ($life[10] -eq 0 -or $life[10] -eq 1)) 'Original one-minute timer/end flow did not return through LoadScene and transition VSBattle to VS Results.' $gdbStdout
                Assert-Condition ($vsResults.Success -and $results[0] -eq 0x56535231 -and (($results[1] -band 0x1f) -eq 0x1f) -and $results[2] -ge 1 -and $results[3] -ge 120 -and $results[4] -eq 8 -and $results[5] -ge 2 -and $results[6] -gt 0 -and $results[7] -gt 0) 'Original VS Results scene did not load all source assets and advance through wallpaper/results/fighter creation.' $gdbStdout
                # mnvsresults.c:869-928 selects Win1..Win3 for place 0 and
                # Lose for other places; scsubsysfighter.c:74-77 installs it.
                $fighter0StatusOk = if ($resultsFighters[0] -eq 0) { $resultsFighters[1] -ge 0x10001 -and $resultsFighters[1] -le 0x10003 -and $resultsFighters[2] -ge 1 -and $resultsFighters[2] -le 3 } else { $resultsFighters[1] -eq 0x10005 -and $resultsFighters[2] -eq 5 }
                $fighter1StatusOk = if ($resultsFighters[3] -eq 0) { $resultsFighters[4] -ge 0x10001 -and $resultsFighters[4] -le 0x10003 -and $resultsFighters[5] -ge 1 -and $resultsFighters[5] -le 3 } else { $resultsFighters[4] -eq 0x10005 -and $resultsFighters[5] -eq 5 }
                Assert-Condition ($vsResultsFighters.Success -and $resultsFighters[0] -ne $resultsFighters[3] -and $fighter0StatusOk -and $fighter1StatusOk) 'Original VS Results fighters did not enter the source win/lose status and submotion paths.' $gdbStdout
                Assert-Condition ($vsResultsDisplay.Success -and $resultsDisplay[1] -gt 0 -and $resultsDisplay[2] -eq 256 -and $resultsDisplay[3] -eq 192 -and $resultsDisplay[4] -ge 1) 'Original VS Results SObj display did not commit a full DS frame or clear the battle-only lower HUD.' $gdbStdout
                if ($task20StackSummary) { Write-Output $task20StackSummary }
                Write-Output ("$Label one-minute lifecycle passed: adapter=$($life[1]) taskman=$($life[2]) ticks=$($life[6]) sudden=$($life[10]) scene=$($life[8])->$($life[9]) results=$($results[3])/files$($results[4])/fighters$($results[5])/sobjs$($results[7])")
            }
            Write-Output ("$Label original CPU proof passed: setup=$($cpu[0]) process=$($cpu[2]) target=$($cpu[3]) objective=0x$('{0:x}' -f $cpu[4]) behavior=0x$('{0:x}' -f $cpu[5]) inputs=$($cpu[6]) stick=$($cpu[7]) buttons=$($cpu[8])/$($cpu[9])/$($cpu[10]) attack=$($cpu[11])/$($cpu[12]) guard=$($cpu[13]) recover=$($cpu[14]) status=$($cpu[15]) damage=$($cpu[19]) x=$($cpu[21])/$($cpu[22])..$($cpu[23])/$($cpu[24])")
            return
        }
        $movementOnly = (($ExpectedMode -eq 39) -or ($ExpectedMode -eq 40))
        $liveHitOnly = (($ExpectedMode -eq 53) -or ($ExpectedMode -eq 54) -or ($ExpectedMode -eq 161) -or ($ExpectedMode -eq 162))
        $naturalMaskOk = if ($BattlePlayable) { (($nat[2] -band 0x6ffff) -eq 0x6ffff) } elseif ($movementOnly) { (($nat[2] -band 0x7fff) -eq 0x7fff) } elseif ($liveHitOnly) { (($nat[2] -band 0x3fdff) -eq 0x3fdff) } else { (($nat[2] -band 0xfffff) -eq 0xfffff) }
        $naturalAttackDamageOk = if ($BattlePlayable) { ($na[6] -gt 0 -and $na[7] -gt 0) } else { ($na[6] -gt 0 -and $na[7] -gt 0 -and $na[9] -gt $na[8]) }
        Assert-Condition ($natural.Success -and $nat[0] -eq 0x464e4d50 -and $nat[1] -eq 0x464e4d53 -and $naturalMaskOk -and $nat[3] -eq 1 -and $nat[4] -gt 0 -and $nat[5] -gt 0 -and $nat[6] -gt 0 -and $nat[7] -gt 0 -and (($nat[8] -band 0x3) -eq 0x3) -and $nat[10] -eq 0) 'Natural-motion manager runtime proof failed.' $gdbStdout
        Assert-Condition ($naturalFig.Success -and $nfig[0] -gt 0 -and $nfig[2] -eq 0 -and $nfig[3] -eq 0) 'Natural-motion figatree attach proof failed.' $gdbStdout
        Assert-Condition ($naturalWait.Success -and $nw[0] -ge 300 -and $nw[1] -ge 300 -and $nw[2] -gt 0 -and $nw[3] -gt 0 -and $nw[4] -ge 300 -and $nw[5] -ge 300) 'Natural-motion Wait animation proof failed.' $gdbStdout
        Assert-Condition ($naturalWalk.Success -and $nwalk[0] -gt 0 -and $nwalk[1] -ge 8 -and $nwalk[2] -ge 8 -and $nwalk[7] -gt 0 -and $nwalk[8] -gt 0 -and $nwalk[9] -gt 0 -and $nwalk[10] -gt 0) 'Natural-motion Walk transition proof failed.' $gdbStdout
        # Phase 14 == Done; phase 19 == battle_playable Done after KO/Rebirth.
        $expectedNaturalPhase = if ($BattlePlayable) { 19 } else { 14 }
        $naturalPhaseOk = if ($liveHitOnly) { ($nc[0] -ge 11) } else { ($nc[0] -eq $expectedNaturalPhase) }
        Assert-Condition ($naturalChain.Success -and $naturalPhaseOk -and $nc[2] -eq 0 -and $nc[3] -ge 2 -and $nc[4] -ge 2 -and $nc[5] -ge 8 -and $nc[6] -ge 8 -and $nc[7] -ge 2 -and $nc[8] -ge 2 -and $nc[9] -ge 1 -and $nc[10] -ge 1) 'Natural combat movement chain (dash/run/brake/turn) proof failed.' $gdbStdout
        if (-not $movementOnly) {
            $naturalRecoveryOk = if ($liveHitOnly) { $true } else { ($na[11] -gt 0) }
            Assert-Condition ($naturalAttack.Success -and $na[2] -gt 0 -and $na[4] -gt 0 -and $naturalAttackDamageOk -and $na[10] -gt 0 -and $naturalRecoveryOk) 'Natural attack->hit->damage lifecycle proof failed.' $gdbStdout
            Assert-Condition ($naturalHitlag.Success -and $nh[0] -gt 0 -and $nh[1] -gt 0) 'Natural hitlag proof failed on attacker/victim.' $gdbStdout
            if (-not $liveHitOnly) {
                Assert-Condition ($naturalGuard.Success -and $ng[0] -gt 0 -and $ng[1] -ge 10 -and $ng[2] -gt 0) 'Natural guard on/hold/off proof failed.' $gdbStdout
            }
        }
        $projectileSummary = ''
        if ($ImportBattleShipMarioFireball -or $ImportBattleShipFoxBlaster) {
            $pj = Get-Ints $projectile
            $expectedKind = if ($ImportBattleShipFoxReflector) { 0 } elseif ($ImportBattleShipFoxBlaster) { 1 } else { 0 }
            $projectileObserved = ($pj[14] -ge 3) -or ($pj[13] -gt 0)
            Assert-Condition ($projectile.Success -and $pj[0] -eq 0x50524f4a -and (($pj[1] -band 0x3f) -eq 0x3f) -and $pj[4] -gt 0 -and $pj[5] -gt 0 -and $projectileObserved -and $pj[15] -gt 0 -and (($pj[16] -band (1 -shl $expectedKind)) -ne 0) -and $pj[17] -ne 0 -and $pj[18] -gt 0) 'Natural projectile special proof failed.' $gdbStdout
            $projectileSummary = " projectile=actor$($pj[2])/kind$($pj[3]) b=$($pj[4]) status=$($pj[5]) accessory=$($pj[7]) flag0=$($pj[8]) spawn=$($pj[9]) ok=$($pj[10]) destroy=$($pj[11])/$($pj[12])/$($pj[13]) weaponFrames=$($pj[14]) max=$($pj[15]) kindMask=0x$('{0:x}' -f $pj[16]) attackMask=0x$('{0:x}' -f $pj[17]) dmg=$($pj[18]) life=$($pj[19]) map=0x$('{0:x}' -f $pj[20])"
        }
        if ($ImportBattleShipFoxReflector) {
            $rf = Get-Ints $reflector
            Assert-Condition ($reflector.Success -and $rf[0] -eq 0x52464c43 -and (($rf[1] -band 0xff) -eq 0xff) -and $rf[4] -gt 0 -and $rf[5] -gt 0 -and $rf[6] -gt 0 -and $rf[7] -gt 0 -and $rf[8] -gt 0 -and $rf[9] -ne 0 -and $rf[10] -gt 0 -and $rf[11] -gt 0 -and $rf[12] -gt 0 -and (($rf[13] -lt 0 -and $rf[14] -gt 0) -or ($rf[13] -gt 0 -and $rf[14] -lt 0)) -and $rf[15] -eq 1 -and $rf[16] -eq 1 -and $rf[19] -gt 0 -and $rf[20] -gt 0 -and $rf[21] -gt 0 -and $rf[24] -gt 0) 'Natural Fox reflector projectile proof failed.' $gdbStdout
            $projectileSummary += " reflector=0x$('{0:x}' -f $rf[1]) fox$($rf[2]) proj$($rf[3]) shine=$($rf[5])/$($rf[6])/$($rf[7]) reflect=$($rf[8]) lr=$($rf[9]) clear=$($rf[10]) proc=$($rf[12]) vx=$($rf[13])->$($rf[14]) owner=$($rf[15]) attrs=ref$($rf[16])/abs$($rf[17])/shield$($rf[18])/count$($rf[19])/dmg$($rf[20])/size$($rf[21]) delta=$($rf[22])/$($rf[23]) special=$($rf[24])/$($rf[25])"
        }
        $specialsSummary = ''
        if ($ImportBattleShipMarioSpecialHi -or $ImportBattleShipMarioSpecialLw -or $ImportBattleShipFoxSpecialHi) {
            $sp = Get-Ints $specials
            $expectedSpecialMask = 0
            if ($ImportBattleShipMarioSpecialHi) { $expectedSpecialMask = $expectedSpecialMask -bor 0x000f }
            if ($ImportBattleShipMarioSpecialLw) { $expectedSpecialMask = $expectedSpecialMask -bor 0x0070 }
            if ($ImportBattleShipFoxSpecialHi) { $expectedSpecialMask = $expectedSpecialMask -bor 0x0f80 }
            Assert-Condition ($specials.Success -and (($sp[0] -band $expectedSpecialMask) -eq $expectedSpecialMask) -and $sp[1] -eq 7) 'Natural remaining-specials proof failed.' $gdbStdout
            if ($ImportBattleShipMarioSpecialHi) {
                Assert-Condition ($sp[5] -gt 0 -and $sp[6] -gt 0 -and $sp[10] -ge 10 -and $sp[11] -gt 1000) 'Natural Mario Super Jump Punch status/launch/fall-special proof failed.' $gdbStdout
            }
            if ($ImportBattleShipMarioSpecialLw) {
                Assert-Condition ($sp[12] -gt 0 -and (($sp[13] -gt 0) -or ($sp[14] -gt 0)) -and $sp[15] -gt 0 -and $sp[16] -ge 10) 'Natural Mario Tornado status/effect/settle proof failed.' $gdbStdout
            }
            if ($ImportBattleShipFoxSpecialHi) {
                Assert-Condition ($sp[17] -gt 0 -and $sp[18] -gt 0 -and $sp[19] -gt 0 -and $sp[20] -gt 0 -and (($sp[21] -gt 0) -or ($sp[22] -gt 0)) -and $sp[23] -ge 10) 'Natural Fox Fire Fox status ladder proof failed.' $gdbStdout
            }
            $specialsSummary = " specials=0x$('{0:x}' -f $sp[0]) phase=$($sp[1]) mhi=$($sp[5])/$($sp[6])/$($sp[8])/$($sp[9])/$($sp[10]) y=$($sp[11]) mlw=$($sp[12])/$($sp[13])/$($sp[14]) dust=$($sp[15]) wait=$($sp[16]) foxhi=$($sp[17])/$($sp[18])/$($sp[19])/$($sp[20])/$($sp[21])/$($sp[22])/$($sp[23]) y=$($sp[24])"
        }
        $audioSummary = ''
        $audioBgmSummary = ''
        $audioBgmResidentBytes = 0
        if ($ImportBattleShipAudioAssets) {
            $aa = Get-Ints $audioAsset
            $fm = Get-Ints $audioFgmMiss
            Assert-Condition ($audioAsset.Success -and $aa[0] -eq 0x41554431 -and $aa[1] -eq 0xff -and $aa[2] -eq 8 -and $aa[3] -eq 0 -and $aa[4] -eq 0 -and $aa[5] -eq 0 -and $aa[6] -eq 4422960 -and $aa[7] -eq 0 -and $aa[8] -le 65536 -and $aa[9] -eq 47 -and $aa[10] -eq 380 -and $aa[11] -eq 7999 -and $aa[12] -gt 0 -and $aa[13] -eq 1 -and $aa[14] -eq 42 -and $aa[15] -eq 117 -and $aa[16] -eq 32000 -and $aa[17] -eq 1 -and $aa[18] -eq 1 -and $aa[19] -eq 322 -and $aa[20] -eq 44100 -and $aa[21] -eq 100 -and $aa[22] -eq 464 -and $aa[23] -eq 695) 'Original audio asset parse-only proof failed.' $gdbStdout
            $audioSummary = " audio=seq$($aa[9]) bank1=$($aa[13])/$($aa[14])/$($aa[15])@$($aa[16]) bank2=$($aa[17])/$($aa[18])/$($aa[19])@$($aa[20]) fgm=$($aa[21])/$($aa[22])/$($aa[23]) raw=$($aa[6]) resident=$($aa[7]) scratch=$($aa[8]) fgmMiss=$($fm[0])/$($fm[1])"
        }
        if ($ImportBattleShipAudioBGM) {
            $ab = Get-Ints $audioBgm
            $ad = Get-Ints $audioBgmAdpcm
            $audioBgmResidentBytes = $ab[13]
            Assert-Condition ($audioBgm.Success -and $audioBgmAdpcm.Success -and $ab[0] -eq 0x42474d31 -and (($ab[1] -band 0x3) -eq 0x3) -and $ab[2] -eq 0 -and $ab[3] -eq 0 -and $ab[4] -eq 0x7800 -and $ab[5] -ge 1 -and $ab[6] -ge 1 -and $ab[9] -eq 0 -and $ab[10] -eq 0 -and $ab[11] -eq 0 -and $ab[12] -gt 16392 -and $ab[13] -eq 16392 -and $ab[14] -ge 4 -and $ab[14] -le 8196 -and $ab[15] -ge 1 -and $ab[16] -eq 1 -and $ab[17] -ge 3200 -and $ab[19] -ge 42100 -and $ab[19] -le 46100 -and $ab[20] -eq 44100 -and $ab[22] -ge 4 -and $ab[23] -lt 2886710 -and (($ab[24] -eq 0) -or ($ab[24] -eq 8196)) -and (($ab[25] -eq 0) -or ($ab[25] -eq 1)) -and (($ab[26] -eq 0) -or ($ab[26] -eq 1)) -and $ab[25] -ne $ab[26] -and $ab[27] -eq 0 -and $ab[28] -gt 0 -and $ab[29] -gt 0 -and $ab[31] -eq 0 -and $ad[0] -eq 0 -and $ad[1] -eq 0 -and $ad[2] -gt 0 -and $ad[3] -gt 0 -and $ad[4] -eq 0 -and $ad[5] -eq 0 -and $ad[7] -eq 0 -and $ad[8] -eq 0) 'Minimal BGM ADPCM proof failed natural stop, rate, residency, packet, seam, or cleanup guards.' $gdbStdout
            $audioBgmSummary = " bgm=track$($ab[3]) play=$($ab[5]) stop=$($ab[6]) refills=$($ab[22]) read=$($ab[12]) rate=$($ab[19]) loop=$($ab[21]) seams=$($ad[3])/$($ad[4]) resident=$($ab[13])"
        }
        $movesetSummary = ''
        if ($ImportBattleShipNormalMoveset) {
            Assert-Condition ($naturalMoveset.Success -and (($nm[0] -band 0x7ff) -eq 0x7ff) -and $nm[1] -eq 15 -and $nm[3] -gt 0 -and $nm[4] -gt 0 -and $nm[5] -gt 0 -and $nm[6] -gt 0 -and $nm[7] -gt 0 -and $nm[8] -gt 0 -and $nm[9] -gt 0 -and $nm[11] -gt 0 -and $nm[12] -gt 0 -and $nm[13] -gt 0 -and $nm[14] -gt 0 -and $nm[15] -gt 0 -and $nm[16] -ge 10 -and $nm[26] -gt $nm[25]) 'Natural normal-moveset tilt/smash/aerial/grab/throw proof failed.' $gdbStdout
            $movesetSummary = " moveset=0x$('{0:x}' -f $nm[0]) phase=$($nm[1]) tilt=$($nm[3])/$($nm[4])/$($nm[5]) smash=$($nm[7]) aerial=$($nm[9]) landing=$($nm[11]) grab=$($nm[12])/$($nm[13]) throw=$($nm[14])/$($nm[15])/$($nm[16]) throwDmg=$($nm[25])->$($nm[26])"
        }
        $battlePlayableSummary = ''
        if ($BattlePlayable) {
            $bpk = Get-Ints $battlePlayableKO
            $bps = Get-Ints $battlePlayableStatus
            $ma = Get-Ints $memoryArena
            $mr = Get-Ints $memoryReloc
            $me = Get-Ints $memoryEvict
            $bp = Get-Ints $battlePlayablePacing
            $victimStockDelta = $bpk[3] - $bpk[4]
            $battleStockDelta = $bpk[5] - $bpk[6]
            $fallsDelta = $bpk[8] - $bpk[7]
            Assert-Condition ($battlePlayablePacing.Success -and $bp[0] -eq 0x42505443 -and $bp[1] -eq 1 -and $bp[2] -ge 3200 -and $bp[3] -eq 0 -and $bp[4] -eq 0 -and $bp[5] -gt 0) 'battle_playable fast-logic pacing marker did not prove explicit unthrottled verification mode.' $gdbStdout
            Assert-Condition ($battlePlayableKO.Success -and $bpk[0] -eq 0x42504c59 -and (($bpk[1] -band 0xff) -eq 0xff) -and $victimStockDelta -gt 0 -and $victimStockDelta -eq $battleStockDelta -and $victimStockDelta -eq $fallsDelta) 'battle_playable stock/fall KO proof failed.' $gdbStdout
            Assert-Condition ($battlePlayableStatus.Success -and $bps[0] -gt 0 -and $bps[1] -gt 0 -and $bps[2] -gt 0 -and $bps[3] -gt 0 -and $bps[5] -ge 8 -and $bps[6] -eq 10 -and $bps[7] -eq 0 -and $bps[8] -eq 0 -and $bps[9] -gt 0) 'battle_playable Dead/Rebirth/return-control proof failed.' $gdbStdout
            $interfaceBytesOk = if ($ImportBattleShipIFCommon) { $mr[4] -gt 0 } else { $true }
            $expectedMemoryArenaScene = if ($OneMinuteMatchProof) { 24 } else { 22 }
            Assert-Condition ($memoryArena.Success -and $ma[0] -eq 0x4d4c4544 -and $ma[1] -eq $expectedMemoryArenaScene -and $ma[3] -ge 0x130000 -and ($ma[6] - $audioBgmResidentBytes) -ge 131072 -and $ma[7] -eq 163840 -and $ma[8] -eq 106496 -and $ma[9] -eq 49152 -and $ma[10] -gt 0) 'battle_playable memory arena ledger failed reserve or final-scene taskman buffer assertions.' $gdbStdout
            Assert-Condition ($memoryReloc.Success -and $mr[0] -gt 0 -and $mr[1] -gt 0 -and $mr[2] -gt 0 -and $mr[3] -gt 0 -and $interfaceBytesOk -and $mr[5] -eq 0 -and $mr[6] -eq 0 -and $mr[8] -eq 0 -and $mr[9] -eq 0) 'battle_playable reloc memory ledger found stale or missing resident groups.' $gdbStdout
            Assert-Condition ($memoryEvict.Success) 'battle_playable reloc eviction ledger was not printed.' $gdbStdout
            $battlePlayableSummary = " bplay=stock$($bpk[3])->$($bpk[4]) falls$($bpk[7])->$($bpk[8]) dead=$($bps[0]) rebirth=$($bps[1])/$($bps[2])/$($bps[3]) recover=$($bps[4])/$($bps[5])"
            if ($RealtimePresentation) {
                $wallSeconds = [double]$bp[5] / 33513982.0
                $phaseRatesX10 = @()
                for ($phase = 0; $phase -lt 5; $phase++) {
                    $phasePresents = [int64]$bp[12 + $phase]
                    $phaseVBlanks = (2 * $phasePresents) +
                        [int64]$bp[17 + $phase]
                    $phaseRatesX10 += if ($phaseVBlanks -gt 0) {
                        [int64][Math]::Floor(((1200 * $phasePresents) +
                            [Math]::Floor($phaseVBlanks / 2)) / $phaseVBlanks)
                    } else { 0 }
                }
                $battlePlayableSummary += (' pace=locked30 logic={0} present={1} ratio=2:1 fps=logic{2}/present{3}x0.1 wall={4:F3}s interval={5}..{6} cadenceBad={7} phasePresent={8}/{9}/{10}/{11}/{12} phaseSlip={13}/{14}/{15}/{16}/{17} phaseUpdateRate={18}/{19}/{20}/{21}/{22}x0.1' -f
                    $bp[2], $bp[3], $bp[7], $bp[6], $wallSeconds,
                    $bp[9], $bp[10], $bp[11],
                    $bp[12], $bp[13], $bp[14], $bp[15], $bp[16],
                    $bp[17], $bp[18], $bp[19], $bp[20], $bp[21],
                    $phaseRatesX10[0], $phaseRatesX10[1],
                    $phaseRatesX10[2], $phaseRatesX10[3],
                    $phaseRatesX10[4])
            } else {
                $battlePlayableSummary += " pace=fast logic$($bp[2]) draw$($bp[4])"
            }
            $battlePlayableSummary += " mem=head$($ma[6]) reloc$($mr[1]) stage$($mr[2]) fighter$($mr[3]) if$($mr[4]) stale$($mr[8])/$($mr[9]) evict$($me[0])/$($me[1])"
            $battlePlayableSummary += $movesetSummary
            $battlePlayableSummary += $specialsSummary
            $battlePlayableSummary += $audioSummary
            $battlePlayableSummary += $audioBgmSummary
            if ($ImportBattleShipIFCommon) {
                $ih = Get-Ints $ifHud
                $victimSlot = [int]$bpk[2]
                $damageMax = @($ih[4], $ih[5])
                $digitCounts = @($ih[6], $ih[7])
                $digitPacks = @($ih[8], $ih[9])
                $stockCurrent = @($ih[10], $ih[11])
                $stockMin = @($ih[12], $ih[13])
                $stockMax = @($ih[14], $ih[15])
                $stockDelta = $stockMax[$victimSlot] - $stockMin[$victimSlot]
                $stockCurrentMatchesFinal = ($stockCurrent[$victimSlot] -eq $bpk[4]) -or ($stockCurrent[$victimSlot] -eq ($bpk[4] + 1))
                Assert-Condition ($ifHud.Success -and $ih[0] -gt 0 -and (($ih[1] -band 0x33) -eq 0x33)) 'Imported IFCommon HUD objects were not observed for both players.' $gdbStdout
                Assert-Condition ((Test-DamageDigits -Damage $damageMax[$victimSlot] -DigitCount $digitCounts[$victimSlot] -DigitsPack $digitPacks[$victimSlot])) 'Imported IFCommon percent digit images did not match the victim damage value.' $gdbStdout
                Assert-Condition ($stockMax[$victimSlot] -ge $bpk[3] -and $stockDelta -ge $victimStockDelta -and $stockCurrentMatchesFinal) 'Imported IFCommon stock icon display did not decrement after KO.' $gdbStdout
                $battlePlayableSummary += " hud=dmg$($damageMax[$victimSlot])/digits0x$('{0:x}' -f $digitPacks[$victimSlot]) stock$($stockMax[$victimSlot])->$($stockMin[$victimSlot])"
            }
        }
        if ($HardwareTriangles) {
            $hw = Get-Ints $platformHw
            $ro = Get-Ints $renderOracle
            $rrm = Get-Ints $renderRawMatrix
            $rs = Get-Ints $renderSubmit
            $rhdiv = Get-Ints $renderHardwareDivide
            $rlazy = Get-Ints $renderLazy
            $rswc = Get-Ints $renderStageWorldCache
            $rp = Get-Ints $renderProfile
            Assert-Condition ($platformHw.Success -and $hw[0] -gt 0 -and $hw[0] -eq $hw[1]) 'Boundary hardware draw did not flush submitted DS 3D frames.' $gdbStdout
            $shw = Get-Ints $stageHardware
            Assert-Condition ($stageHardware.Success -and $shw[0] -gt 8) 'Boundary hardware replay did not exceed the old bounded DObj submit slice.' $gdbStdout
            Assert-Condition ($shw[1] -gt 0) 'Boundary hardware replay did not submit hardware triangles.' $gdbStdout
            Assert-Condition ($shw[1] -eq ($shw[2] + $shw[3])) 'Boundary hardware depth accounting does not match submitted triangles.' $gdbStdout
            Assert-Condition ($shw[3] -gt 0) 'Boundary hardware replay did not preserve source no-z projected-depth submission.' $gdbStdout
            Assert-Condition ($shw[5] -gt 0 -and $shw[6] -gt 0 -and $shw[7] -gt 0 -and $shw[9] -ne 0 -and $shw[10] -gt 0 -and $shw[11] -gt 0) 'Boundary hardware replay did not bind/upload a ready texture.' $gdbStdout
            Assert-Condition ($shw[8] -eq 0) 'Boundary hardware replay still rejects source-loaded stage texture state.' $gdbStdout
            $shwf = Get-Ints $stageHardwareFighter
            Assert-Condition ($stageHardwareFighter.Success -and $shwf[0] -eq 2 -and $shwf[1] -gt 0) 'Boundary hardware replay did not submit selected fighter triangles.' $gdbStdout
            Assert-Condition ($renderOracle.Success -and $ro[0] -gt 0 -and $ro[1] -eq 0 -and $ro[2] -eq 0) 'Boundary hardware submitted vertex positions drifted from the CPU 20.12 oracle.' $gdbStdout
            Assert-Condition ($renderRawMatrix.Success -and $rrm[0] -gt 0 -and $rrm[3] -gt 0 -and $rrm[4] -eq 0 -and $rrm[5] -le 16 -and $rrm[6] -eq 0 -and $rrm[7] -eq 0 -and $rrm[8] -gt 0 -and $rrm[9] -eq 0) 'Boundary corrected composed GX matrix PosTest failed natural-frame coverage or correctness.' $gdbStdout
            $submitTotal = $rs[0] + $rs[1] + $rs[2] + $rs[3] + $rs[4] + $rs[5] + $rs[6]
            $expectedProjectedDivisions = (6 * $rs[3]) + (9 * ($rs[2] + $rs[4] + $rs[5] + $rs[6]))
            Assert-Condition ($renderSubmit.Success -and $renderProfile.Success -and $rs[0] -eq $rrm[0] -and $rs[0] -gt 0 -and $rs[2] -eq $rrm[2] -and $rs[3] -gt 0 -and $rs[6] -eq $rrm[1] -and $rs[7] -eq 0 -and $submitTotal -eq $rp[16] -and $rs[8] -eq $expectedProjectedDivisions) 'Boundary hybrid raw/projected class or projected-division accounting drifted.' $gdbStdout
            $expectedHardwareDivideEvaluations = $rs[8] + (3 * ($rs[2] + $rs[4] + $rs[5] + $rs[6]))
            Assert-Condition ($renderHardwareDivide.Success -and $rhdiv[0] -gt 0 -and ($rhdiv[0] + $rhdiv[1] + $rhdiv[2]) -eq $expectedHardwareDivideEvaluations -and $rhdiv[3] -eq 0 -and $rhdiv[4] -eq 0) 'Boundary forensic hardware-divider coverage or exact-result oracle drifted.' $gdbStdout
            Assert-Condition ($renderStageWorldCache.Success -and $rswc[0] -eq 0 -and $rswc[1] -eq 57 -and $rswc[2] -eq 0 -and $rswc[3] -eq 0 -and $rswc[4] -eq 0 -and $rswc[5] -eq 0) 'Boundary first-frame stage-world cache did not build all 57 exact nodes before reuse.' $gdbStdout
            Assert-Condition ($renderLazy.Success -and $rlazy[0] -gt 0 -and $rlazy[1] -eq $rlazy[0] -and $rlazy[2] -gt 0 -and $rlazy[3] -gt 0 -and $rlazy[4] -gt 0 -and $rlazy[5] -eq 0) 'Boundary forensic snapshot/lazy-transform accounting drifted or overflowed.' $gdbStdout
            $hardwareSummary = " hwflush=$($hw[0])/$($hw[1]) hwsubmit=$($shw[0]) hwtri=$($shw[1]) hwdepth=z$($shw[2])/proj$($shw[3])/decal$($shw[4]) hwtex=bind$($shw[5])/upload$($shw[6])/ready$($shw[7])/reject$($shw[8])/fmt$($shw[9])/max$($shw[10])x$($shw[11]) hwftr=$($shwf[0])/$($shwf[1]) oracle=$($ro[0])/$($ro[1])/$($ro[2]) submit=raw$($rs[0])/snap$($rs[1])/cross$($rs[2])/noz$($rs[3])/range$($rs[6])/rej$($rs[7])/div$($rs[8]) hdiv=$($rhdiv[0])/$($rhdiv[1])/$($rhdiv[2])/z$($rhdiv[3])/mis$($rhdiv[4]) stageWorld=$($rswc[0])/$($rswc[1])/reject$($rswc[2])/overflow$($rswc[3]) lazy=load$($rlazy[0])/xf$($rlazy[1])/hit$($rlazy[2])/new$($rlazy[3])/reuse$($rlazy[4])/ovf$($rlazy[5]) rawcand=$($rrm[0])/$($rrm[1])/$($rrm[2]) postest=$($rrm[3])/$($rrm[4])/e$($rrm[5])/mw$($rrm[8])"
        }
        Assert-Condition ($boundary.Success -and (Convert-MarkerUInt32 $boundary.Groups[1].Value) -eq 0x53434e45 -and [int]$boundary.Groups[2].Value -eq 22) 'VSBattle did not park at the bounded scene boundary after natural-motion proof.' $gdbStdout
        Write-Output ("$Label ftmanager natural-combat harness passed: wait={0}/{1} walk={2}/{3} dash={4}/{5} run={6}/{7} attack={8} hitbox={9} dmg={10}->{11} status={12} knock={13} guard={14}/{15}/{16} retries={17} updates={18} mask=0x{19:x}{20}{21}{22}{23}" -f $nw[0], $nw[1], $nwalk[1], $nwalk[2], $nc[3], $nc[4], $nc[5], $nc[6], $na[2], $na[4], $na[8], $na[9], $na[6], $na[10], $ng[0], $ng[1], $ng[2], $na[5], $nat[4], $nat[2], $projectileSummary, $battlePlayableSummary, $hardwareSummary, $aobjSummary)
        return
    }
    $prv = Get-Ints $prev
    Assert-Condition ($prev.Success -and $prv[0] -eq 0x46504c56 -and $prv[1] -eq 0x46505653 -and (($prv[2] -band 0x1fff) -eq 0x1fff) -and $prv[3] -eq 0xff -and $prv[4] -eq 2) 'Prerequisite preview-loop proof did not pass.' $gdbStdout
    $lp = Get-Ints $loop
    Assert-Condition ($loop.Success -and $lp[0] -eq 0x4647414c -and $lp[1] -eq 0x46474153 -and (($lp[2] -band 0x1fff) -eq 0x1fff) -and $lp[3] -eq 0xff -and $lp[4] -eq 2 -and $lp[5] -ge 160 -and $lp[6] -ge 220) 'gcRunAll-loop result/mask did not pass.' $gdbStdout
    $tm = Get-Ints $taskman
    Assert-Condition ($tm[0] -eq 1 -and $tm[1] -gt 0 -and $tm[2] -gt 0 -and $tm[3] -eq $tm[2] -and $tm[4] -gt 0 -and $tm[5] -ge $tm[1]) 'gcRunAll-loop taskman path failed.' $gdbStdout
    $rn = Get-Ints $run
    Assert-Condition ($rn[0] -gt 0 -and $rn[1] -gt 0 -and $rn[3] -ge 2 -and $rn[4] -eq $rn[5]) 'gcRunAll pause/preserve/GObj stability setup failed.' $gdbStdout
    $pc = Get-Ints $process
    Assert-Condition ($pc[0] -eq 1 -and $pc[1] -eq 1 -and $pc[2] -gt 0 -and $pc[3] -gt 0 -and $pc[4] -eq $pc[2] -and $pc[5] -eq $pc[3] -and $pc[6] -eq 0) 'gcRunAll process attach/callback path failed.' $gdbStdout
    $inp = Get-Ints $input
    Assert-Condition ($inp[0] -gt 0 -and $inp[1] -gt 0 -and $inp[2] -gt 0 -and $inp[3] -gt 0 -and $inp[4] -eq 0 -and $inp[5] -eq 0 -and $inp[6] -gt 0 -and $inp[7] -gt 0 -and $inp[8] -ne 0 -and $inp[9] -ne 0 -and $inp[10] -ne 0 -and $inp[11] -ne 0 -and $inp[12] -gt 0 -and $inp[13] -gt 0) 'gcRunAll input bridge failed or wrote FTStruct directly.' $gdbStdout
    $st = Get-Ints $status
    Assert-Condition (($st[0..9] -join ',') -eq '10,10,4,4,10,10,4,4,0,0') 'gcRunAll start/final state was not Wait/Ground.' $gdbStdout
    Assert-Condition ((($st[10] -band 0x3ff) -eq 0x3ff) -and (($st[11] -band 0x3ff) -eq 0x3ff) -and (($st[12] -band 0x7ff) -eq 0x7ff) -and (($st[13] -band 0x7ff) -eq 0x7ff)) 'gcRunAll status/transition masks were incomplete.' $gdbStdout
    $vi = Get-Ints $visits
    Assert-Condition ((@($vi | Where-Object { $_ -le 0 }).Count -eq 0)) 'gcRunAll state visit counters were incomplete.' $gdbStdout
    $ca = Get-Ints $calls
    Assert-Condition ($ca[2] -eq 1 -and $ca[3] -eq 1 -and $ca[0] -gt 0 -and $ca[1] -gt 0 -and $ca[0] -le 180 -and $ca[1] -le 180 -and (@($ca[4..9] | Where-Object { $_ -le 0 }).Count -eq 0)) 'gcRunAll frame callback counters failed.' $gdbStdout
    $dr = Get-Ints $draw
    Assert-Condition ($dr[0] -eq 96 -and $dr[1] -eq 72 -and $dr[2] -ge 96 -and $dr[3] -eq 1 -and $dr[4] -ge 7 -and $dr[5] -ge 7 -and $dr[6] -ge 14 -and $dr[7] -ge 7 -and $dr[8] -ge 7 -and $dr[9] -ge 14 -and $dr[10] -ge 18 -and $dr[11] -ge 14 -and $dr[12] -ge 18 -and $dr[13] -gt 0 -and $dr[14] -ne 0 -and $dr[15] -ne 0) 'gcRunAll draw/DObj/pixel markers failed.' $gdbStdout
    $sc = Get-Ints $screen
    Assert-Condition ($sc[4] -ne 0 -and $sc[5] -ne 0 -and $sc[8] -gt 0 -and $sc[9] -gt 0) 'gcRunAll screen movement markers failed.' $gdbStdout
    $mv = Get-Ints $move
    Assert-Condition ($mv[2] -ne 0 -and $mv[3] -ne 0 -and $mv[4] -gt 0 -and $mv[5] -gt 0 -and $mv[6] -eq $mv[0] -and $mv[7] -eq $mv[1] -and $mv[8] -eq 1 -and $mv[9] -eq 1 -and $mv[10] -eq 1 -and $mv[11] -eq 1) 'gcRunAll movement/floor markers failed.' $gdbStdout
    $tr = Get-Ints $trans
    Assert-Condition ($tr[0] -ge 2 -and $tr[1] -ge 2 -and $tr[2] -ge 2 -and $tr[3] -ge 2 -and $tr[4] -ge 4 -and $tr[5] -ge 2 -and $tr[6] -ge 2 -and $tr[7] -ge 2 -and $tr[8] -gt 0) 'gcRunAll transition counters failed.' $gdbStdout
    $sf = Get-Ints $safe
    Assert-Condition ((@($sf[0..9] | Where-Object { $_ -ne 0 }).Count -eq 0)) 'gcRunAll safe escape counters were not zero.' $gdbStdout
    $pd = Get-Ints $platform
    Assert-Condition ($pd[0] -eq 1 -and $pd[1] -eq 96 -and $pd[2] -eq 72 -and $pd[3] -ge $dr[4]) 'Platform original-DL preview markers failed.' $gdbStdout
    Assert-Condition ($boundary.Success -and (Convert-MarkerUInt32 $boundary.Groups[1].Value) -eq 0x53434e45 -and [int]$boundary.Groups[2].Value -eq 22) 'VSBattle did not park at the bounded scene boundary after gcRunAll proof.' $gdbStdout
    Write-Output ("$Label harness passed: scene=22/21 gcRunAll={0} callbacks={1}/{2} draws={3} pixels={4} visits=0x{5:x}/0x{6:x} transitions=0x{7:x}/0x{8:x} screen-dx={9}/{10} screen-rise={11}/{12} final=Wait/Ground safe=1{13}" -f $tm[4], $pc[4], $pc[5], $dr[5], $dr[13], $st[10], $st[11], $st[12], $st[13], $sc[4], $sc[5], $sc[8], $sc[9], $aobjSummary)
} finally {
    if ($null -ne $emulator) {
        $emulator.Refresh()
        if (-not $emulator.HasExited) {
            Stop-Process -Id $emulator.Id -Force
            $emulator.WaitForExit()
        }
    }
    Restore-MelonDSGdbConfig -State $configState
    Remove-GdbMarkerTemps -Root $root -ScriptName $scriptName
    Remove-Item $stdout, $stderr -Force -ErrorAction SilentlyContinue
}
