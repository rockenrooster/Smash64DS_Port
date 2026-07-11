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
    [switch]$RequireRealtime60Fps
)
$ErrorActionPreference = 'Stop'
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
if ($RealtimePresentation) {
    $target = 'smash64ds-battle-playable-canonical-hwtri'
    $build = 'build-battle-playable-canonical-hwtri-harness'
    $LiveInputPreview = $true
} elseif ($CPUOpponentProof) {
    $target = 'smash64ds-battle-playable-cpu-proof'
    $build = 'build-battle-playable-cpu-proof-harness'
    $LiveInputPreview = $true
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
    -RequireRealtime60Fps:$RequireRealtime60Fps `
    -Harness 'battle_playable' `
    -Target $target `
    -Build $build `
    -ExpectedMode 163 `
    -ExpectedHarnessSceneCurr 22 `
    -ExpectedHarnessScenePrev 21 `
    -Label 'battle_playable Pupupu' `
    -HarnessSelectMessage 'battle_playable harness did not select Pupupu VSBattle from Maps.'
