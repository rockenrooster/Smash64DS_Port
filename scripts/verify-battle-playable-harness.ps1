param(
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$GdbPort = 3333,
    [int]$RunnerSlot = -1,
    [switch]$NoBuild,
    [int]$DelaySeconds = 5,
    [switch]$ImportBattleShipNormalMoveset,
    [switch]$ImportBattleShipMarioFireball,
    [switch]$ImportBattleShipFoxBlaster
)
$ErrorActionPreference = 'Stop'
$target = 'smash64ds-battle-playable-hwtri'
$build = 'build-battle-playable-hwtri-harness'
if ($ImportBattleShipNormalMoveset -or
    $ImportBattleShipMarioFireball -or
    $ImportBattleShipFoxBlaster) {
    $suffix = @()
    if ($ImportBattleShipNormalMoveset) { $suffix += 'moveset' }
    if ($ImportBattleShipMarioFireball) { $suffix += 'fireball' }
    if ($ImportBattleShipFoxBlaster) { $suffix += 'blaster' }
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
    -HardwareTriangles `
    -Harness 'battle_playable' `
    -Target $target `
    -Build $build `
    -ExpectedMode 163 `
    -ExpectedHarnessSceneCurr 22 `
    -ExpectedHarnessScenePrev 21 `
    -Label 'battle_playable Pupupu stock KO' `
    -HarnessSelectMessage 'battle_playable harness did not select Pupupu VSBattle from Maps.'
