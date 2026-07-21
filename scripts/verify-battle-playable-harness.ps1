param(
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$GdbPort = 4333,
    [int]$RunnerSlot = -1,
    [switch]$NoBuild,
    [int]$DelaySeconds = 5,
    [switch]$ImportBattleShipEffectManager,
    [switch]$ImportBattleShipFoxReflector,
    [switch]$ImportBattleShipNormalMoveset,
    [switch]$ImportBattleShipMarioFireball,
    [switch]$ImportBattleShipFoxBlaster,
    [switch]$ImportBattleShipMarioSpecialHi,
    [switch]$ImportBattleShipMarioSpecialLw,
    [switch]$ImportBattleShipFoxSpecialHi,
    [switch]$ImportBattleShipAudioAssets,
    [switch]$ImportBattleShipAudioBGM,
    [switch]$ImportBattleShipFTComputer,
    [switch]$RealtimePresentation,
    [switch]$LiveInputPreview,
    [switch]$CPUOpponentProof,
    [switch]$MatchLifecycleProof,
    [switch]$OneMinuteMatchProof,
    [switch]$RequireLocked30Pacing,
    [int]$RendererProfileLevel = -1,
    [ValidateRange(0,9)][int]$RendererFastRunMode = 0,
    [ValidateRange(0,1)][int]$NativeStageGeneratedSegment0Enable = 0,
    [ValidateRange(0,2)][int]$Task36HwComposeMode = 0,
    [ValidateRange(0,1)][int]$StaticTextureAotMode = 0,
    [ValidateRange(0,1)][int]$IFCommonHybridOamMode = 0,
    [ValidateRange(0,1)][int]$FastWallpaperAffineMode = 0,
    [ValidateRange(0,1)][int]$FoxCpuMode = 1,
    [switch]$RequireZeroPostGoTextureFence,
    [switch]$RendererM2DetailedLedger,
    [ValidateRange(0,1)][int]$Task9FloatItcmMode = 1,
    [ValidateRange(0,1)][int]$Task9FloatPhase2Mode = 1,
    [ValidateRange(0,1)][int]$Task16FloatCompareMode = 0,
    [ValidateRange(0,1)][int]$Task16FloatI2fMode = 0,
    [ValidateRange(0,1)][int]$Task16FloatAddSubMode = 0,
    [ValidateRange(0,256)][int]$RendererBenchmarkSamples = 0,
    [ValidateRange(0,1000000)][int]$RendererBenchmarkStartFrame = 0,
    [ValidateRange(5,600)][int]$RendererBenchmarkTimeoutSeconds = 30,
    [switch]$Task25RPacingTrace,
    [string]$Task25RLifecycleExportPath = '',
    [string]$RendererBenchmarkScreenshot = ''
)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
$selectedGdbPort = if (($RunnerSlot -ge 0) -and
    -not $PSBoundParameters.ContainsKey('GdbPort')) {
    Get-MelonDSRunnerPort -RunnerSlot $RunnerSlot -Cpu ARM9
} else {
    $GdbPort
}
if (($RendererProfileLevel -lt -1) -or ($RendererProfileLevel -gt 2)) {
    throw 'RendererProfileLevel must be -1 (automatic), 0, 1, or 2.'
}
if ($OneMinuteMatchProof -and -not $MatchLifecycleProof) {
    throw 'OneMinuteMatchProof requires MatchLifecycleProof.'
}
if ($OneMinuteMatchProof -and
    (($RendererFastRunMode -ne 9) -or ($Task36HwComposeMode -ne 2) -or
     ($StaticTextureAotMode -ne 1) -or
     ($IFCommonHybridOamMode -ne 0) -or
     ($FastWallpaperAffineMode -ne 1) -or
     -not $RequireZeroPostGoTextureFence)) {
    throw 'OneMinuteMatchProof requires the published-equivalent hardware renderer configuration: mode 9, mip 0, fast wallpaper 1, static textures 1, bitmap OAM 0, and the strict post-GO fence.'
}
$ImportBattleShipNormalMoveset = $true
$ImportBattleShipMarioFireball = $true
$ImportBattleShipFoxBlaster = $true
$ImportBattleShipEffectManager = $true
$ImportBattleShipFoxReflector = $true
$ImportBattleShipMarioSpecialHi = $true
$ImportBattleShipMarioSpecialLw = $true
$ImportBattleShipFoxSpecialHi = $true
$ImportBattleShipAudioAssets = $true
$ImportBattleShipAudioBGM = $true
$target = 'smash64ds-battle-playable-fast-hwtri'
$build = 'build-battle-playable-hwtri-harness'
$harness = 'battle_playable'
if ($OneMinuteMatchProof) {
    # Keep the full-expiry hardware gate artifact-isolated from the canonical
    # user ROM while exercising its renderer residency and one-minute rule.
    $target = 'smash64ds-battle-playable-one-minute-match-hwtri'
    $build = 'build-battle-playable-one-minute-match-hwtri-harness'
    $LiveInputPreview = $true
} elseif ($RealtimePresentation) {
    if ($RendererProfileLevel -lt 0) { $RendererProfileLevel = 0 }
    if ($RendererProfileLevel -eq 0) {
        $target = 'smash64ds-battle-playable-hwtri'
        $build = 'build-battle-playable-canonical-hwtri-harness'
    } elseif ($RendererProfileLevel -eq 1) {
        $target = 'smash64ds-battle-playable-coarse-hwtri'
        $build = 'build-battle-playable-coarse-hwtri-harness'
    } else {
        $target = 'smash64ds-battle-playable-forensic-hwtri'
        $build = 'build-battle-playable-forensic-hwtri-harness'
    }
    $LiveInputPreview = $true
} elseif ($CPUOpponentProof) {
    $target = 'smash64ds-battle-playable-cpu-proof'
    $build = 'build-battle-playable-cpu-proof-harness'
    $LiveInputPreview = $true
}
if ($RendererProfileLevel -lt 0) {
    # Legacy non-realtime state labs keep profiling off. The dedicated one-
    # minute release gate selects realtime profile 0 and fixed-two pacing.
    $RendererProfileLevel = if ($OneMinuteMatchProof) { 0 } else { 2 }
}
if (($target -eq 'smash64ds-battle-playable-hwtri') -and
    -not $PSBoundParameters.ContainsKey('RendererFastRunMode')) {
    $RendererFastRunMode = 9
}
if ($MatchLifecycleProof) {
    $harness = 'battle_playable_match_lifecycle'
} elseif ($RealtimePresentation -and ($RendererProfileLevel -lt 2)) {
    # Profiles 0/1 share the latency-optimized mode-163 realtime variant;
    # profile 2 retains the size-optimized forensic build variant.
    $harness = 'battle_playable_realtime'
}
$hardwareTriangles = $target -like '*-hwtri'
$rendererMakeEnvironment = if ($OneMinuteMatchProof) {
    @{
        NDS_RENDERER_FAST_RUN_DEFAULT = '9'
        NDS_TASK36_HW_COMPOSE = '2'
        NDS_SCENE_MIP_CACHE_LAB = '0'
        NDS_FAST_WALLPAPER_AFFINE = '1'
        NDS_RENDERER_BATTLE_STATIC_TEXTURE_DEFAULT = '1'
        NDS_DEBUG_HUD = '0'
    }
} else { @{} }
$previousRendererMakeEnvironment = @{}
foreach ($name in $rendererMakeEnvironment.Keys) {
    $previousRendererMakeEnvironment[$name] =
        [Environment]::GetEnvironmentVariable($name, 'Process')
    [Environment]::SetEnvironmentVariable(
        $name, $rendererMakeEnvironment[$name], 'Process')
}
try {
& (Join-Path $PSScriptRoot 'verify-battle-mariofox-gcrunall-loop-harness.ps1') `
    -MelonDS $MelonDS `
    -Gdb $Gdb `
    -GdbPort $selectedGdbPort `
    -RunnerSlot $RunnerSlot `
    -NoBuild:$NoBuild `
    -DelaySeconds $DelaySeconds `
    -BattlePlayable `
    -ImportBattleShipIFCommon `
    -ImportBattleShipNormalMoveset:$ImportBattleShipNormalMoveset `
    -ImportBattleShipMarioFireball:$ImportBattleShipMarioFireball `
    -ImportBattleShipFoxBlaster:$ImportBattleShipFoxBlaster `
    -ImportBattleShipEffectManager:$ImportBattleShipEffectManager `
    -ImportBattleShipFoxReflector:$ImportBattleShipFoxReflector `
    -ImportBattleShipMarioSpecialHi:$ImportBattleShipMarioSpecialHi `
    -ImportBattleShipMarioSpecialLw:$ImportBattleShipMarioSpecialLw `
    -ImportBattleShipFoxSpecialHi:$ImportBattleShipFoxSpecialHi `
    -ImportBattleShipAudioAssets:$ImportBattleShipAudioAssets `
    -ImportBattleShipAudioBGM:$ImportBattleShipAudioBGM `
    -ImportBattleShipFTComputer:$ImportBattleShipFTComputer `
    -HardwareTriangles:$hardwareTriangles `
    -RealtimePresentation:$RealtimePresentation `
    -LiveInputPreview:$LiveInputPreview `
    -CPUOpponentProof:$CPUOpponentProof `
    -MatchLifecycleProof:$MatchLifecycleProof `
    -OneMinuteMatchProof:$OneMinuteMatchProof `
    -RequireLocked30Pacing:$RequireLocked30Pacing `
    -RendererProfileLevel $RendererProfileLevel `
    -RendererFastRunMode $RendererFastRunMode `
    -NativeStageGeneratedSegment0Enable $NativeStageGeneratedSegment0Enable `
    -Task36HwComposeMode $Task36HwComposeMode `
    -StaticTextureAotMode $StaticTextureAotMode `
    -IFCommonHybridOamMode $IFCommonHybridOamMode `
    -FastWallpaperAffineMode $FastWallpaperAffineMode `
    -FoxCpuMode $FoxCpuMode `
    -RequireZeroPostGoTextureFence:$RequireZeroPostGoTextureFence `
    -RendererM2DetailedLedger:$RendererM2DetailedLedger `
    -Task9FloatItcmMode $Task9FloatItcmMode `
    -Task9FloatPhase2Mode $Task9FloatPhase2Mode `
    -Task16FloatCompareMode $Task16FloatCompareMode `
    -Task16FloatI2fMode $Task16FloatI2fMode `
    -Task16FloatAddSubMode $Task16FloatAddSubMode `
    -RendererBenchmarkSamples $RendererBenchmarkSamples `
    -RendererBenchmarkStartFrame $RendererBenchmarkStartFrame `
    -RendererBenchmarkTimeoutSeconds $RendererBenchmarkTimeoutSeconds `
    -Task25RPacingTrace:$Task25RPacingTrace `
    -Task25RLifecycleExportPath $Task25RLifecycleExportPath `
    -RendererBenchmarkScreenshot $RendererBenchmarkScreenshot `
    -Harness $harness `
    -Target $target `
    -Build $build `
    -ExpectedMode 163 `
    -ExpectedHarnessSceneCurr 22 `
    -ExpectedHarnessScenePrev 21 `
    -Label 'battle_playable Pupupu' `
    -HarnessSelectMessage 'battle_playable harness did not select Pupupu VSBattle from Maps.'
$ownerExitCode = $LASTEXITCODE
} finally {
    foreach ($name in $rendererMakeEnvironment.Keys) {
        if ($null -eq $previousRendererMakeEnvironment[$name]) {
            Remove-Item "Env:$name" -ErrorAction SilentlyContinue
        } else {
            [Environment]::SetEnvironmentVariable(
                $name, $previousRendererMakeEnvironment[$name], 'Process')
        }
    }
}
if ($ownerExitCode -ne 0) { exit $ownerExitCode }
