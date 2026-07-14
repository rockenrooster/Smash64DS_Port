param(
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$GdbPort = 3333,
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
    [switch]$RequireRealtime60Fps,
    [int]$RendererProfileLevel = -1,
    [ValidateRange(0,256)][int]$RendererBenchmarkSamples = 0
)
$ErrorActionPreference = 'Stop'
if (($RendererProfileLevel -lt -1) -or ($RendererProfileLevel -gt 2)) {
    throw 'RendererProfileLevel must be -1 (automatic), 0, 1, or 2.'
}
if ($OneMinuteMatchProof -and -not $MatchLifecycleProof) {
    throw 'OneMinuteMatchProof requires MatchLifecycleProof.'
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
if ($RealtimePresentation) {
    if ($RendererProfileLevel -lt 0) { $RendererProfileLevel = 0 }
    if ($RendererProfileLevel -eq 0) {
        $target = 'smash64ds-battle-playable-canonical-hwtri'
        $build = 'build-battle-playable-canonical-hwtri-harness'
    } elseif ($RendererProfileLevel -eq 1) {
        $target = 'smash64ds-battle-playable-coarse-hwtri'
        $build = 'build-battle-playable-coarse-hwtri-harness'
    } else {
        $target = 'smash64ds-battle-playable-forensic-hwtri'
        $build = 'build-battle-playable-forensic-hwtri-harness'
    }
    $LiveInputPreview = $true
} elseif ($OneMinuteMatchProof) {
    # Keep the full-expiry release gate artifact-isolated from the canonical
    # user ROM while exercising that ROM's same one-minute source rule.
    $target = 'smash64ds-battle-playable-one-minute-match'
    $build = 'build-battle-playable-one-minute-match-harness'
    $LiveInputPreview = $true
} elseif ($CPUOpponentProof) {
    $target = 'smash64ds-battle-playable-cpu-proof'
    $build = 'build-battle-playable-cpu-proof-harness'
    $LiveInputPreview = $true
}
if ($RendererProfileLevel -lt 0) {
    # The one-minute match is a state/memory gate. Keep renderer profiling off
    # so the unthrottled full match reaches Results in a practical verifier
    # window; the canonical realtime verifier owns renderer performance.
    $RendererProfileLevel = if ($OneMinuteMatchProof) { 0 } else { 2 }
}
if ($MatchLifecycleProof) {
    $harness = 'battle_playable_match_lifecycle'
} elseif ($RealtimePresentation -and ($RendererProfileLevel -lt 2)) {
    # Profiles 0/1 share the latency-optimized mode-163 realtime variant;
    # profile 2 retains the size-optimized forensic build variant.
    $harness = 'battle_playable_realtime'
}
$hardwareTriangles = $target -like '*-hwtri'
& (Join-Path $PSScriptRoot 'verify-battle-mariofox-gcrunall-loop-harness.ps1') `
    -MelonDS $MelonDS `
    -Gdb $Gdb `
    -GdbPort $GdbPort `
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
    -RequireRealtime60Fps:$RequireRealtime60Fps `
    -RendererProfileLevel $RendererProfileLevel `
    -RendererBenchmarkSamples $RendererBenchmarkSamples `
    -Harness $harness `
    -Target $target `
    -Build $build `
    -ExpectedMode 163 `
    -ExpectedHarnessSceneCurr 22 `
    -ExpectedHarnessScenePrev 21 `
    -Label 'battle_playable Pupupu' `
    -HarnessSelectMessage 'battle_playable harness did not select Pupupu VSBattle from Maps.'
