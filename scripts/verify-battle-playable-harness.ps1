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
    [switch]$ImportBattleShipAudioAssets
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
$target = 'smash64ds-battle-playable-hwtri'
$build = 'build-battle-playable-hwtri-harness'
if ($ImportBattleShipNormalMoveset -or
    $ImportBattleShipMarioFireball -or
    $ImportBattleShipFoxBlaster -or
    $ImportBattleShipEffectManager -or
    $ImportBattleShipFoxReflector -or
    $ImportBattleShipMarioSpecialHi -or
    $ImportBattleShipMarioSpecialLw -or
    $ImportBattleShipFoxSpecialHi) {
    $suffix = @()
    if ($ImportBattleShipNormalMoveset) { $suffix += 'moveset' }
    if ($ImportBattleShipMarioFireball) { $suffix += 'fireball' }
    if ($ImportBattleShipFoxBlaster) { $suffix += 'blaster' }
    if ($ImportBattleShipEffectManager) { $suffix += 'effect' }
    if ($ImportBattleShipFoxReflector) { $suffix += 'reflector' }
    if ($ImportBattleShipMarioSpecialHi) { $suffix += 'mariohi' }
    if ($ImportBattleShipMarioSpecialLw) { $suffix += 'mariolw' }
    if ($ImportBattleShipFoxSpecialHi) { $suffix += 'foxhi' }
    if ($ImportBattleShipAudioAssets) { $suffix += 'audio' }
    $target = "$target-$($suffix -join '-')"
    $build = "$build-$($suffix -join '-')"
}
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
    -HardwareTriangles `
    -Harness 'battle_playable' `
    -Target $target `
    -Build $build `
    -ExpectedMode 163 `
    -ExpectedHarnessSceneCurr 22 `
    -ExpectedHarnessScenePrev 21 `
    -Label 'battle_playable Pupupu stock KO' `
    -HarnessSelectMessage 'battle_playable harness did not select Pupupu VSBattle from Maps.'
