param(
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$GdbPort = 3333,
    [int]$RunnerSlot = -1,
    [switch]$NoBuild,
    [int]$DelaySeconds = 5
)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
. (Join-Path $PSScriptRoot 'lib\gdb-markers.ps1')
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$verifierContext = Initialize-MelonDSVerifierContext `
    -Root $root `
    -MelonDS $MelonDS `
    -RunnerSlot $RunnerSlot `
    -GdbPort $GdbPort `
    -GdbPortExplicit:$PSBoundParameters.ContainsKey('GdbPort') `
    -NoBuild:$NoBuild
$rom = Join-Path $root 'smash64ds-battle-pupupu.nds'
$elf = Join-Path $root 'smash64ds-battle-pupupu.elf'
$melonDsPath = $verifierContext.MelonDSPath
$melonDsDir = Split-Path -Parent $melonDsPath
$logDir = Get-MelonDSVerifierLogDir -Root $root -RunnerSlot (Get-MelonDSActiveRunnerSlot)
$stdout = Join-Path $logDir 'melonds.battle-pupupu-stage-harness.stdout.log'
$stderr = Join-Path $logDir 'melonds.battle-pupupu-stage-harness.stderr.log'
$configState = $null
$emulator = $null
$scriptName = '_battle_pupupu_stage_harness.gdb'
if (-not $env:DEVKITPRO) { $env:DEVKITPRO = 'C:/devkitPro' }
if (-not $env:DEVKITARM) { $env:DEVKITARM = 'C:/devkitPro/devkitARM' }
& make -C $root TARGET=smash64ds-battle-pupupu BUILD=build-battle-pupupu-stage-harness NDS_DEV_SCENE_HARNESS=battle_pupupu_stage -j16
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
if (-not (Test-Path $rom) -or -not (Test-Path $elf)) {
    throw 'Battle Pupupu stage harness build did not produce smash64ds-battle-pupupu.nds and smash64ds-battle-pupupu.elf.'
}
if (-not (Test-Path $melonDsPath)) { throw "melonDS executable not found: $melonDsPath" }
if (-not (Test-Path $Gdb)) { throw "GDB executable not found: $Gdb" }
New-Item -ItemType Directory -Path $logDir -Force | Out-Null
try {
    $configState = Enable-MelonDSGdbConfig -MelonDSPath $melonDsPath -GdbPort $verifierContext.GdbPort -Persistent:([bool]$verifierContext.PersistentConfig)
    Remove-Item $stdout, $stderr -Force -ErrorAction SilentlyContinue
    $emulator = Start-Process -FilePath $melonDsPath `
        -ArgumentList $rom `
        -WorkingDirectory $melonDsDir `
        -RedirectStandardOutput $stdout `
        -RedirectStandardError $stderr `
        -WindowStyle Hidden `
        -PassThru
    Wait-MelonDSGdbListener -Process $emulator -Port $verifierContext.GdbPort | Out-Null
    Start-Sleep -Seconds ([Math]::Max($DelaySeconds, 1))
    $gdbCommands = @(
        'set pagination off',
        'set confirm off',
        'set remotetimeout 5',
        ("target remote 127.0.0.1:{0}" -f (Get-MelonDSActiveGdbPort)),
        'printf "HARN=%#x,%u,%u,%u,%#x\n", gNdsSceneHarnessResult, gNdsSceneHarnessMode, gNdsSceneHarnessSceneCurr, gNdsSceneHarnessScenePrev, gNdsSceneHarnessReservedMask',
        'printf "SCENE=%u,%u,%u\n", gSCManagerSceneData.scene_curr, gSCManagerSceneData.scene_prev, gSCManagerSceneData.gkind',
        'printf "VSB_SETUP=%#x,%#x,%#x,%#x,%#x,%#x,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsSCVSBattleOriginalStartResult, gNdsSCVSBattleOriginalFuncStartResult, gNdsSCVSBattleOriginalRelocResult, gNdsSCVSBattleOriginalSetupResult, gNdsSCVSBattleOriginalUpdateResult, gNdsSCVSBattleOriginalSetupMask, gNdsSCVSBattleOriginalLoadedFileCount, gNdsSCVSBattleOriginalGKind, gNdsSCVSBattleOriginalGameRule, gNdsSCVSBattleOriginalTime, gNdsSCVSBattleOriginalStock, gNdsSCVSBattleOriginalIsTeam, gNdsSCVSBattleOriginalPlayerCount, gNdsSCVSBattleOriginalCpuCount',
        'printf "VSB_PLAYERS=%u,%u,%u,%u,%u,%u,%d,%d\n", gNdsSCVSBattleOriginalActivePlayerCount, gNdsSCVSBattleOriginalFighterCreateCount, gNdsSCVSBattleOriginalP0FKind, gNdsSCVSBattleOriginalP1FKind, gNdsSCVSBattleOriginalActivePlayerMask, gNdsSCVSBattleOriginalFighterGObjCount, gNdsSCVSBattleOriginalP0LR, gNdsSCVSBattleOriginalP1LR',
        'printf "STAGE_RELOC=%#x,%#x,%#x,%u,%u,%u,%u,%u,%u,%u\n", gNdsStagePupupuRelocResult, gNdsStagePupupuRelocAssetMask, gNdsStagePupupuRelocDependencyMask, gNdsStagePupupuExternalFixupCount, gNdsStagePupupuExternalFixupFailCount, gNdsStagePupupuInternalFixupCount, gNdsStagePupupuMapHeaderOffset, gNdsStagePupupuGroundDataPtrReady, gNdsStagePupupuGeometryPtrReady, gNdsStagePupupuMapNodesPtrReady',
        'printf "STAGE_BATTLE=%#x,%#x,%u,%u,%u,%u,%#x,%#x,%#x\n", gNdsSCVSBattleStageResult, gNdsSCVSBattleStageMask, gNdsSCVSBattleStageGKind, gNdsSCVSBattleStageGroundDataReady, gNdsSCVSBattleStageGeometryReady, gNdsSCVSBattleStageMapNodesReady, gNdsSCVSBattleStageBGM, gNdsSCVSBattleStageLightAngleXBits, gNdsSCVSBattleStageDeferredMask',
        'printf "PUPUPU_GROUND=%#x,%#x,%#x,%#x,%#x,%u,%u,%u,%u,%#x\n", gNdsPupupuGroundSetupResult, gNdsPupupuGroundDisplayResult, gNdsPupupuGroundGObjResult, gNdsPupupuGroundSetupMask, gNdsPupupuGroundDeferredMask, gNdsPupupuGroundLayerGObjCount, gNdsPupupuGroundMapGObjCount, gNdsPupupuGroundMapHeadReady, gNdsPupupuGroundParticleBankID, gNdsPupupuGroundMapHeadOffset',
        'printf "PUPUPU_GOBJS=%#x,%#x,%#x,%#x,%u,%u,%u,%u,%u\n", gNdsPupupuGroundLayerGObjMask, gNdsPupupuGroundMapGObjMask, gNdsPupupuGroundLayerDObjMask, gNdsPupupuGroundLayerMObjMask, gNdsPupupuGroundRootGObjID, gNdsPupupuGroundWhispyEyesGObjID, gNdsPupupuGroundWhispyMouthGObjID, gNdsPupupuGroundFlowersBackGObjID, gNdsPupupuGroundFlowersFrontGObjID',
        'printf "PUPUPU_COUNTS=%u,%u,%u,%u,%u,%u\n", gNdsPupupuGroundGObjCountBefore, gNdsPupupuGroundGObjCountAfter, gNdsPupupuGroundDObjCountAfter, gNdsPupupuGroundMObjCountAfter, gNdsPupupuGroundAObjCountAfter, gNdsPupupuGroundNonPupupuStubCallCount',
        'printf "PUPUPU_UPDATE_ZERO=%#x,%u\n", gNdsPupupuUpdateResult, gNdsPupupuUpdateTickCount',
        'printf "TASK=%u,%u,%u,%u,%u,%u\n", gNdsTaskmanLoopReached, gNdsTaskmanSceneUpdateSet, gNdsTaskmanSceneDrawSet, gNdsTaskmanControllerAutoRead, gNdsSceneBoundaryKind, gNdsTaskmanBoundedUpdateCount',
        'printf "ROOM_TITLE=%u,%#x,%#x\n", gNdsOpeningRoomTickCount, gNdsTitleOriginalStartResult, gNdsTitleDrawResult',
        'detach',
        'quit'
    )
    $gdbStdout = (Invoke-GdbMarkerScript -Gdb $Gdb -Elf $elf -Root $root -Commands $gdbCommands -ScriptName $scriptName).Stdout
    $harn = [regex]::Match($gdbStdout, 'HARN=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0)')
    $scene = [regex]::Match($gdbStdout, 'SCENE=([0-9]+),([0-9]+),([0-9]+)')
    $setup = [regex]::Match($gdbStdout, 'VSB_SETUP=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $players = [regex]::Match($gdbStdout, 'VSB_PLAYERS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+)')
    $stageReloc = [regex]::Match($gdbStdout, 'STAGE_RELOC=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageBattle = [regex]::Match($gdbStdout, 'STAGE_BATTLE=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $pupupuGround = [regex]::Match($gdbStdout, 'PUPUPU_GROUND=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0)')
    $pupupuGObjs = [regex]::Match($gdbStdout, 'PUPUPU_GOBJS=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $pupupuCounts = [regex]::Match($gdbStdout, 'PUPUPU_COUNTS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $pupupuUpdateZero = [regex]::Match($gdbStdout, 'PUPUPU_UPDATE_ZERO=(0x[0-9a-fA-F]+|0),([0-9]+)')
    $task = [regex]::Match($gdbStdout, 'TASK=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $roomTitle = [regex]::Match($gdbStdout, 'ROOM_TITLE=([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    if (-not $harn.Success -or
        (Convert-MarkerUInt32 $harn.Groups[1].Value) -ne 0x4841524e -or
        [int]$harn.Groups[2].Value -ne 8 -or
        [int]$harn.Groups[3].Value -ne 22 -or
        [int]$harn.Groups[4].Value -ne 21 -or
        (Convert-MarkerUInt32 $harn.Groups[5].Value) -ne 0) {
        throw "Battle Pupupu stage harness did not select direct VSBattle from Maps.`n$gdbStdout"
    }
    if (-not $scene.Success -or [int]$scene.Groups[1].Value -ne 22 -or [int]$scene.Groups[2].Value -ne 21 -or [int]$scene.Groups[3].Value -ne 6) {
        throw "Live scene state is not Pupupu VSBattle from Maps.`n$gdbStdout"
    }
    if (-not $setup.Success -or
        (Convert-MarkerUInt32 $setup.Groups[1].Value) -ne 0x56425354 -or
        (Convert-MarkerUInt32 $setup.Groups[2].Value) -ne 0x56424653 -or
        (Convert-MarkerUInt32 $setup.Groups[3].Value) -ne 0x5642524c -or
        (Convert-MarkerUInt32 $setup.Groups[4].Value) -ne 0x56425355 -or
        (Convert-MarkerUInt32 $setup.Groups[5].Value) -ne 0x56425550 -or
        ((Convert-MarkerUInt32 $setup.Groups[6].Value) -band 0x7f) -ne 0x7f -or
        [int]$setup.Groups[7].Value -ne 8 -or
        [int]$setup.Groups[8].Value -ne 6 -or
        [int]$setup.Groups[9].Value -ne 1 -or
        [int]$setup.Groups[10].Value -ne 3 -or
        [int]$setup.Groups[11].Value -ne 2 -or
        [int]$setup.Groups[12].Value -ne 0 -or
        [int]$setup.Groups[13].Value -ne 2 -or
        [int]$setup.Groups[14].Value -ne 0) {
        throw "Imported VSBattle setup proof failed for Pupupu stage.`n$gdbStdout"
    }
    if (-not $players.Success -or
        [int]$players.Groups[1].Value -ne 2 -or
        [int]$players.Groups[2].Value -ne 2 -or
        [int]$players.Groups[3].Value -ne 0 -or
        [int]$players.Groups[4].Value -ne 1 -or
        [int]$players.Groups[5].Value -ne 3 -or
        [int]$players.Groups[6].Value -ne 2 -or
        [int]$players.Groups[7].Value -ne 1 -or
        [int]$players.Groups[8].Value -ne -1) {
        throw "Pupupu VSBattle player/fighter descriptor proof failed.`n$gdbStdout"
    }
    if (-not $stageReloc.Success -or
        (Convert-MarkerUInt32 $stageReloc.Groups[1].Value) -ne 0x50555052 -or
        ((Convert-MarkerUInt32 $stageReloc.Groups[2].Value) -band 0x1f) -ne 0x1f -or
        ((Convert-MarkerUInt32 $stageReloc.Groups[3].Value) -band 0x1f) -ne 0x1f -or
        [int]$stageReloc.Groups[4].Value -lt 9 -or
        [int]$stageReloc.Groups[5].Value -ne 0 -or
        [int]$stageReloc.Groups[7].Value -ne 0x14 -or
        [int]$stageReloc.Groups[8].Value -ne 1 -or
        [int]$stageReloc.Groups[9].Value -ne 1 -or
        [int]$stageReloc.Groups[10].Value -ne 1) {
        throw "Pupupu stage relocation proof failed in VSBattle.`n$gdbStdout"
    }
    if (-not $stageBattle.Success -or
        (Convert-MarkerUInt32 $stageBattle.Groups[1].Value) -ne 0x50555042 -or
        ((Convert-MarkerUInt32 $stageBattle.Groups[2].Value) -band 0xff) -ne 0xff -or
        [int]$stageBattle.Groups[3].Value -ne 6 -or
        [int]$stageBattle.Groups[4].Value -ne 1 -or
        [int]$stageBattle.Groups[5].Value -ne 1 -or
        [int]$stageBattle.Groups[6].Value -ne 1 -or
        ((Convert-MarkerUInt32 $stageBattle.Groups[9].Value) -band 0x3) -ne 0x3) {
        throw "Pupupu stage adoption proof failed in VSBattle.`n$gdbStdout"
    }
    if (-not $pupupuGround.Success -or
        (Convert-MarkerUInt32 $pupupuGround.Groups[1].Value) -ne 0x50554753 -or
        (Convert-MarkerUInt32 $pupupuGround.Groups[2].Value) -ne 0x50554744 -or
        (Convert-MarkerUInt32 $pupupuGround.Groups[3].Value) -ne 0x5055474f -or
        ((Convert-MarkerUInt32 $pupupuGround.Groups[4].Value) -band 0x3ff) -ne 0x3ff -or
        [int]$pupupuGround.Groups[6].Value -lt 1 -or
        [int]$pupupuGround.Groups[7].Value -ne 4 -or
        [int]$pupupuGround.Groups[8].Value -ne 1 -or
        [int]$pupupuGround.Groups[9].Value -lt 1 -or
        (Convert-MarkerUInt32 $pupupuGround.Groups[10].Value) -ne 0x10f0) {
        throw "Bounded original Pupupu ground setup proof failed.`n$gdbStdout"
    }
    if (-not $pupupuGObjs.Success -or
        ((Convert-MarkerUInt32 $pupupuGObjs.Groups[1].Value) -eq 0) -or
        ((Convert-MarkerUInt32 $pupupuGObjs.Groups[2].Value) -band 0xf) -ne 0xf -or
        ((Convert-MarkerUInt32 $pupupuGObjs.Groups[3].Value) -eq 0) -or
        [int]$pupupuGObjs.Groups[6].Value -eq 0 -or
        [int]$pupupuGObjs.Groups[7].Value -eq 0 -or
        [int]$pupupuGObjs.Groups[8].Value -eq 0 -or
        [int]$pupupuGObjs.Groups[9].Value -eq 0) {
        throw "Bounded original Pupupu ground GObj graph proof failed.`n$gdbStdout"
    }
    if (-not $pupupuCounts.Success -or
        [int]$pupupuCounts.Groups[2].Value -le [int]$pupupuCounts.Groups[1].Value -or
        [int]$pupupuCounts.Groups[6].Value -ne 0) {
        throw "Pupupu ground object counts did not prove bounded original object creation.`n$gdbStdout"
    }
    if (-not $pupupuUpdateZero.Success -or
        (Convert-MarkerUInt32 $pupupuUpdateZero.Groups[1].Value) -ne 0 -or
        [int]$pupupuUpdateZero.Groups[2].Value -ne 0) {
        throw "Setup-only Pupupu stage harness unexpectedly ran the update probe.`n$gdbStdout"
    }
    if (-not $task.Success -or [int]$task.Groups[1].Value -ne 1 -or [int]$task.Groups[5].Value -ne 22 -or [int]$task.Groups[6].Value -lt 1) {
        throw "Taskman did not park at the VSBattle setup boundary.`n$gdbStdout"
    }
    if (-not $roomTitle.Success -or [int]$roomTitle.Groups[1].Value -ne 0 -or (Convert-MarkerUInt32 $roomTitle.Groups[2].Value) -ne 0 -or (Convert-MarkerUInt32 $roomTitle.Groups[3].Value) -ne 0) {
        throw "Battle Pupupu stage harness replayed opening/title work.`n$gdbStdout"
    }
    Write-Output ("Battle Pupupu stage harness passed: scene=22/21 gkind={0} stage={1} ground={2} layers={3} mapGObjs={4} players={5} fighters={6}" -f $scene.Groups[3].Value, $stageBattle.Groups[2].Value, $pupupuGround.Groups[4].Value, $pupupuGround.Groups[6].Value, $pupupuGround.Groups[7].Value, $setup.Groups[13].Value, $players.Groups[2].Value)
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
