param(
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$RunnerSlot = 6,
    [string]$Build = 'build-bugs-samus-roll',
    [switch]$NoBuild
)

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
. (Join-Path $PSScriptRoot 'lib\gdb-markers.ps1')

function Assert-SamusWeaponTransform {
    param([bool]$Condition, [string]$Message, [string]$Evidence = '')
    if (-not $Condition) {
        if ($Evidence) { throw "$Message`n$Evidence" }
        throw $Message
    }
}

$target = 'smash64ds-battle-playable-fast-hwtri'
$buildDir = Join-Path $root (Join-Path 'builds' $Build)
$rom = Join-Path $buildDir "$target.nds"
$elf = Join-Path $buildDir "$target.elf"
$config = Join-Path $buildDir 'nds_build_config.h'
$sceneConfig = Join-Path $buildDir 'nds_scene_harness_config.h'

if (-not $NoBuild) {
    & make -C $root "TARGET=$target" "BUILD=$Build" `
        'NDS_P2_LUIGI=1' 'NDS_P2_DONKEY=1' 'NDS_P2_CAPTAIN=1' `
        'NDS_P2_SAMUS=1' 'NDS_P2_PROOF_FIGHTER0=3'
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

foreach ($path in @($rom, $elf, $config, $sceneConfig)) {
    Assert-SamusWeaponTransform (Test-Path -LiteralPath $path -PathType Leaf) `
        "Samus weapon-transform proof input is missing: $path"
}

$configText = Get-Content -LiteralPath $config -Raw
$sceneText = Get-Content -LiteralPath $sceneConfig -Raw
foreach ($definition in @(
    '#define NDS_DEV_LIVE_INPUT_PREVIEW 0',
    '#define NDS_HARNESS_FAST_LOGIC 1',
    '#define NDS_P2_SAMUS 1',
    '#define NDS_P2_PROOF_FIGHTER0 3',
    '#define NDS_IMPORT_BATTLESHIP_BATTLE_PLAYABLE 1'
)) {
    Assert-SamusWeaponTransform $configText.Contains($definition) `
        "Samus weapon-transform build is missing required definition: $definition" $config
}
Assert-SamusWeaponTransform $sceneText.Contains('#define NDS_DEV_SCENE_HARNESS 163') `
    'Samus weapon-transform proof must run the mode-163 battle_playable harness.' $sceneConfig

# No status, input, weapon, matrix or position value is poked here. The existing
# natural-special tour supplies controller B/down-B input and BattleShip owns
# Start->Loop->Wait->Start->End, Charge Shot launch and Bomb creation. GDB only
# reads counters once the bounded battle publishes its scene-boundary result.
$ctx = Initialize-MelonDSVerifierContext -Root $root -MelonDS $MelonDS `
    -RunnerSlot $RunnerSlot -NoBuild
$state = $null
$emu = $null
try {
    $state = Enable-MelonDSGdbConfig -MelonDSPath $ctx.MelonDSPath `
        -GdbPort $ctx.GdbPort -Persistent -MuteAudio
    $emu = Start-Process -FilePath $ctx.MelonDSPath -ArgumentList $rom `
        -WorkingDirectory (Split-Path $ctx.MelonDSPath) `
        -WindowStyle Hidden -PassThru
    Wait-MelonDSGdbListener -Process $emu -Port $ctx.GdbPort | Out-Null

    $commands = @(
        'set pagination off',
        'set confirm off',
        'set remotetimeout 20',
        ("target remote 127.0.0.1:{0}" -f $ctx.GdbPort),
        'tbreak osStopThread if gNdsSceneBoundaryResult != 0',
        'continue',
        'printf "SAMUS_WEAPON_RENDER=draw:%u,submit:%u,visible:%u,tri:%u,texready:%u,texreject:%u,reject:%u,callback:%u,kindmask:%#x,custom46:%u\n", gNdsWeaponRendererDObjDrawCount, gNdsWeaponRendererSubmitCount, gNdsWeaponRendererVisibleDrawCount, gNdsWeaponRendererTriangleCount, gNdsWeaponRendererTextureReadyCount, gNdsWeaponRendererTextureRejectCount, gNdsWeaponRendererRejectedDrawCount, gNdsWeaponRendererCallbackKind, gNdsWeaponRendererKindMask, gNdsRendererAdapterCustom46AppliedCount',
        'printf "SAMUS_STAGE_ROUTE=prepared:%u,captured:%u,hwsubmit:%u,manual:%u,nonstage:%u,drawcb:%u\n", gNdsStageGCDrawAllLoopPrepared, gNdsStageGCDrawAllLoopCapturedDisplayCount, gNdsStageGCDrawAllLoopHardwareSubmitCount, gNdsStageGCDrawAllLoopManualDisplayCallCount, gNdsStageGCDrawAllLoopNonStageCaptureCount, gNdsStageGCDrawAllLoopDObjDrawCallbackCount',
        'printf "SAMUS_WEAPONS=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%#x,%u\n", gNdsFighterSamusSpecialsSlot, gNdsFighterSamusSpecialsNPressFrames, gNdsFighterSamusSpecialsNStartFrames, gNdsFighterSamusSpecialsNLoopFrames, gNdsFighterSamusSpecialsNChargeMax, gNdsFighterSamusSpecialsNFullWaitFrames, gNdsFighterSamusSpecialsNReleasePressFrames, gNdsFighterSamusSpecialsNEndFrames, gNdsFighterSamusSpecialsNReleaseWaitFrames, gNdsFighterSamusSpecialsLwPressFrames, gNdsFighterSamusSpecialsLwFrames, gNdsFighterSamusSpecialsLwWaitFrames, gNdsSamusBombMakeCount, gNdsSamusBombMakeSuccessCount, gNdsRendererAdapterKind46AppliedCount, gNdsRendererAdapterCustom46AppliedCount, gNdsRendererAdapterCustom47TranslationMismatchCount, gNdsFighterNaturalCombatStallCount, gNdsFighterNaturalMovesetMask, gNdsSceneBoundaryResult',
        'detach',
        'quit'
    )
    Invoke-GdbMarkerScript -Gdb $Gdb -Elf $elf -Root $root `
        -Commands $commands -ScriptName 'p2-samus-weapon-transforms.gdb' `
        -TimeoutSeconds 300 | Out-Null

    $stdoutPath = Join-Path $env:SMASH64DS_VERIFY_TEMP_DIR 'p2-samus-weapon-transforms.gdb.out'
    $stdout = Get-Content -LiteralPath $stdoutPath -Raw
    $match = [regex]::Match($stdout,
        'SAMUS_WEAPONS=(\d+),(\d+),(\d+),(\d+),(\d+),(\d+),(\d+),(\d+),(\d+),(\d+),(\d+),(\d+),(\d+),(\d+),(\d+),(\d+),(\d+),(\d+),(0x[0-9a-fA-F]+),(\d+)')
    Assert-SamusWeaponTransform $match.Success 'Samus weapon-transform marker is missing.' $stdout
    $v = @($match.Groups[1..20] | ForEach-Object { $_.Value })
    $naturalMask = [Convert]::ToUInt32($v[18].Substring(2), 16)

    Assert-SamusWeaponTransform ([int]$v[1] -gt 0 -and [int]$v[2] -gt 0 -and [int]$v[3] -gt 0) `
        'Samus neutral-B did not traverse source Start/Loop from controller input.' $stdout
    Assert-SamusWeaponTransform ([int]$v[4] -eq 7 -and [int]$v[5] -gt 0) `
        'Samus did not naturally reach source full charge level 7 and return to Wait.' $stdout
    Assert-SamusWeaponTransform ([int]$v[6] -gt 0 -and [int]$v[7] -gt 0 -and [int]$v[8] -ge 60) `
        'Samus full Charge Shot did not naturally enter End, launch and settle.' $stdout
    Assert-SamusWeaponTransform ([int]$v[9] -gt 0 -and [int]$v[10] -gt 0 -and [int]$v[11] -ge 60) `
        'Samus down-B did not naturally execute and settle after Bomb creation.' $stdout
    Assert-SamusWeaponTransform ([int]$v[12] -gt 0 -and [int]$v[13] -gt 0) `
        'Samus down-B reached its status but the source Bomb factory did not create an object.' $stdout
    Assert-SamusWeaponTransform ([int]$v[14] -gt 0) `
        'Charge Shot never applied BattleShip built-in matrix kind 46.' $stdout
    Assert-SamusWeaponTransform ([int]$v[15] -gt 0) `
        'Samus Bomb never applied BattleShip custom matrix 0x46 (decimal 70).' $stdout
    Assert-SamusWeaponTransform ([int]$v[16] -eq 0) `
        'An MVP-recalc application changed the already-composed translation row.' $stdout
    Assert-SamusWeaponTransform ([int]$v[17] -eq 0) 'Samus natural battle accumulated a stall.' $stdout
    Assert-SamusWeaponTransform ($naturalMask -eq 0x7ff) `
        ('Controller-driven common moveset regressed: got 0x{0:x}' -f $naturalMask) $stdout
    Assert-SamusWeaponTransform ([int]$v[19] -ne 0) 'Bounded battle did not publish a scene-boundary result.' $stdout

    Write-Output ('P2-3 Samus weapon transforms passed: charge={0}/{1}/{2} full={3} ' +
        'release={4}/{5} bomb={6}/{7} kind46={8} custom0x46={9} translationMismatch=0 stalls=0.' -f
        [int]$v[2], [int]$v[3], [int]$v[4], [int]$v[5], [int]$v[7], [int]$v[8],
        [int]$v[10], [int]$v[11], [int]$v[14], [int]$v[15])
    Write-Output $match.Value
}
finally {
    if (($null -ne $emu) -and -not $emu.HasExited) {
        Stop-Process -Id $emu.Id -Force -ErrorAction SilentlyContinue
    }
    if ($null -ne $state) { Restore-MelonDSGdbConfig -State $state }
}
