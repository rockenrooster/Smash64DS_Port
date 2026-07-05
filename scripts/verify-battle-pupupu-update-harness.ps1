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
$rom = Join-Path $root 'smash64ds-battle-pupupu-update.nds'
$elf = Join-Path $root 'smash64ds-battle-pupupu-update.elf'
$melonDsPath = $verifierContext.MelonDSPath
$melonDsDir = Split-Path -Parent $melonDsPath
$logDir = Get-MelonDSVerifierLogDir -Root $root -RunnerSlot (Get-MelonDSActiveRunnerSlot)
$stdout = Join-Path $logDir 'melonds.battle-pupupu-update-harness.stdout.log'
$stderr = Join-Path $logDir 'melonds.battle-pupupu-update-harness.stderr.log'
$configState = $null
$emulator = $null
$scriptName = '_battle_pupupu_update_harness.gdb'
if (-not $env:DEVKITPRO) { $env:DEVKITPRO = 'C:/devkitPro' }
if (-not $env:DEVKITARM) { $env:DEVKITARM = 'C:/devkitPro/devkitARM' }
& make -C $root TARGET=smash64ds-battle-pupupu-update BUILD=build-battle-pupupu-update-harness NDS_DEV_SCENE_HARNESS=battle_pupupu_update -j16
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
if (-not (Test-Path $rom) -or -not (Test-Path $elf)) {
    throw 'Battle Pupupu update harness build did not produce smash64ds-battle-pupupu-update.nds and smash64ds-battle-pupupu-update.elf.'
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
        'printf "VSB_SETUP=%#x,%#x,%u,%u,%u,%u\n", gNdsSCVSBattleOriginalSetupResult, gNdsSCVSBattleOriginalSetupMask, gNdsSCVSBattleOriginalLoadedFileCount, gNdsSCVSBattleOriginalPlayerCount, gNdsSCVSBattleOriginalFighterCreateCount, gNdsSCVSBattleOriginalGKind',
        'printf "STAGE_RELOC=%#x,%#x,%u\n", gNdsStagePupupuRelocResult, gNdsStagePupupuRelocAssetMask, gNdsStagePupupuExternalFixupFailCount',
        'printf "STAGE_BATTLE=%#x,%#x,%u,%u,%u,%u\n", gNdsSCVSBattleStageResult, gNdsSCVSBattleStageMask, gNdsSCVSBattleStageGKind, gNdsSCVSBattleStageGroundDataReady, gNdsSCVSBattleStageGeometryReady, gNdsSCVSBattleStageMapNodesReady',
        'printf "PUPUPU_GROUND=%#x,%#x,%u,%#x,%u,%u\n", gNdsPupupuGroundSetupResult, gNdsPupupuGroundSetupMask, gNdsPupupuGroundMapGObjCount, gNdsPupupuGroundMapGObjMask, gNdsPupupuGroundLayerGObjCount, gNdsPupupuGroundNonPupupuStubCallCount',
        'printf "PUPUPU_UPDATE=%#x,%#x,%u,%u,%u,%#x,%#x,%u,%u\n", gNdsPupupuUpdateResult, gNdsPupupuUpdateMask, gNdsPupupuUpdateTickCount, gNdsPupupuUpdateGameStatusBefore, gNdsPupupuUpdateGameStatusAfter, gNdsPupupuUpdateMapGObjMaskBefore, gNdsPupupuUpdateMapGObjMaskAfter, gNdsPupupuUpdateGObjCountBefore, gNdsPupupuUpdateGObjCountAfter',
        'printf "PUPUPU_UPDATE_STATE=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsPupupuUpdateWhispyStatusBefore, gNdsPupupuUpdateWhispyStatusAfterFirst, gNdsPupupuUpdateWhispyStatusAfterFinal, gNdsPupupuUpdateWindWaitBefore, gNdsPupupuUpdateWindWaitAfterFirst, gNdsPupupuUpdateWindWaitAfterFinal, gNdsPupupuUpdateBlinkWaitBefore, gNdsPupupuUpdateBlinkWaitAfterFinal, gNdsPupupuUpdateFlowersBackStatusBefore, gNdsPupupuUpdateFlowersBackStatusAfterFinal, gNdsPupupuUpdateFlowersFrontStatusBefore, gNdsPupupuUpdateFlowersFrontStatusAfterFinal',
        'printf "PUPUPU_UPDATE_SIDEFX=%u,%u,%u,%u\n", gNdsPupupuUpdateVelPushCount, gNdsPupupuUpdateQuakeCount, gNdsPupupuUpdateParticleScriptCount, gNdsPupupuUpdateWindFGMCount',
        'printf "TASK=%u,%u,%u,%u,%u,%u\n", gNdsTaskmanLoopReached, gNdsTaskmanSceneUpdateSet, gNdsTaskmanSceneDrawSet, gNdsTaskmanControllerAutoRead, gNdsSceneBoundaryKind, gNdsTaskmanBoundedUpdateCount',
        'printf "ROOM_TITLE=%u,%#x,%#x\n", gNdsOpeningRoomTickCount, gNdsTitleOriginalStartResult, gNdsTitleDrawResult',
        'detach',
        'quit'
    )
    $gdbStdout = (Invoke-GdbMarkerScript -Gdb $Gdb -Elf $elf -Root $root -Commands $gdbCommands -ScriptName $scriptName).Stdout
    $harn = [regex]::Match($gdbStdout, 'HARN=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0)')
    $scene = [regex]::Match($gdbStdout, 'SCENE=([0-9]+),([0-9]+),([0-9]+)')
    $setup = [regex]::Match($gdbStdout, 'VSB_SETUP=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageReloc = [regex]::Match($gdbStdout, 'STAGE_RELOC=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $stageBattle = [regex]::Match($gdbStdout, 'STAGE_BATTLE=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $ground = [regex]::Match($gdbStdout, 'PUPUPU_GROUND=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+)')
    $update = [regex]::Match($gdbStdout, 'PUPUPU_UPDATE=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+)')
    $state = [regex]::Match($gdbStdout, 'PUPUPU_UPDATE_STATE=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $sidefx = [regex]::Match($gdbStdout, 'PUPUPU_UPDATE_SIDEFX=([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $task = [regex]::Match($gdbStdout, 'TASK=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $roomTitle = [regex]::Match($gdbStdout, 'ROOM_TITLE=([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    if (-not $harn.Success -or
        (Convert-MarkerUInt32 $harn.Groups[1].Value) -ne 0x4841524e -or
        [int]$harn.Groups[2].Value -ne 9 -or
        [int]$harn.Groups[3].Value -ne 22 -or
        [int]$harn.Groups[4].Value -ne 21 -or
        (Convert-MarkerUInt32 $harn.Groups[5].Value) -ne 0) {
        throw "Battle Pupupu update harness did not select direct VSBattle from Maps.`n$gdbStdout"
    }
    if (-not $scene.Success -or [int]$scene.Groups[1].Value -ne 22 -or [int]$scene.Groups[2].Value -ne 21 -or [int]$scene.Groups[3].Value -ne 6) {
        throw "Live scene state is not Pupupu VSBattle from Maps.`n$gdbStdout"
    }
    if (-not $setup.Success -or
        (Convert-MarkerUInt32 $setup.Groups[1].Value) -ne 0x56425355 -or
        ((Convert-MarkerUInt32 $setup.Groups[2].Value) -band 0x6f) -ne 0x6f -or
        [int]$setup.Groups[3].Value -ne 8 -or
        [int]$setup.Groups[4].Value -ne 2 -or
        [int]$setup.Groups[5].Value -ne 2 -or
        [int]$setup.Groups[6].Value -ne 6) {
        throw "Imported VSBattle setup proof failed for Pupupu update.`n$gdbStdout"
    }
    if (-not $stageReloc.Success -or
        (Convert-MarkerUInt32 $stageReloc.Groups[1].Value) -ne 0x50555052 -or
        ((Convert-MarkerUInt32 $stageReloc.Groups[2].Value) -band 0x1f) -ne 0x1f -or
        [int]$stageReloc.Groups[3].Value -ne 0) {
        throw "Pupupu stage relocation proof failed for update harness.`n$gdbStdout"
    }
    if (-not $stageBattle.Success -or
        (Convert-MarkerUInt32 $stageBattle.Groups[1].Value) -ne 0x50555042 -or
        ((Convert-MarkerUInt32 $stageBattle.Groups[2].Value) -band 0xff) -ne 0xff -or
        [int]$stageBattle.Groups[3].Value -ne 6 -or
        [int]$stageBattle.Groups[4].Value -ne 1 -or
        [int]$stageBattle.Groups[5].Value -ne 1 -or
        [int]$stageBattle.Groups[6].Value -ne 1) {
        throw "Pupupu stage adoption proof failed for update harness.`n$gdbStdout"
    }
    if (-not $ground.Success -or
        (Convert-MarkerUInt32 $ground.Groups[1].Value) -ne 0x50554753 -or
        ((Convert-MarkerUInt32 $ground.Groups[2].Value) -band 0x3ff) -ne 0x3ff -or
        [int]$ground.Groups[3].Value -ne 4 -or
        ((Convert-MarkerUInt32 $ground.Groups[4].Value) -band 0xf) -ne 0xf -or
        [int]$ground.Groups[5].Value -lt 1 -or
        [int]$ground.Groups[6].Value -ne 0) {
        throw "Bounded original Pupupu ground setup proof failed before update.`n$gdbStdout"
    }
    if (-not $update.Success -or
        (Convert-MarkerUInt32 $update.Groups[1].Value) -ne 0x50555550 -or
        ((Convert-MarkerUInt32 $update.Groups[2].Value) -band 0xff) -ne 0xff -or
        [int]$update.Groups[3].Value -ne 2 -or
        [int]$update.Groups[4].Value -ne 0 -or
        [int]$update.Groups[5].Value -ne 1 -or
        ((Convert-MarkerUInt32 $update.Groups[6].Value) -band 0xf) -ne 0xf -or
        ((Convert-MarkerUInt32 $update.Groups[7].Value) -band 0xf) -ne 0xf -or
        [int]$update.Groups[8].Value -ne [int]$update.Groups[9].Value) {
        throw "Bounded original Pupupu update probe failed.`n$gdbStdout"
    }
    if (-not $state.Success -or
        [int]$state.Groups[1].Value -ne 0 -or
        [int]$state.Groups[2].Value -ne 1 -or
        [int]$state.Groups[3].Value -ne 1 -or
        [int]$state.Groups[4].Value -ne 2 -or
        [int]$state.Groups[5].Value -ne 2 -or
        [int]$state.Groups[6].Value -ne 1 -or
        [int]$state.Groups[7].Value -ne 3 -or
        [int]$state.Groups[8].Value -ne 1 -or
        [int]$state.Groups[9].Value -ne 0 -or
        [int]$state.Groups[10].Value -ne 0 -or
        [int]$state.Groups[11].Value -ne 0 -or
        [int]$state.Groups[12].Value -ne 0) {
        throw "Pupupu update state did not match safe Sleep -> Wait proof.`n$gdbStdout"
    }
    if (-not $sidefx.Success -or
        [int]$sidefx.Groups[1].Value -ne 0 -or
        [int]$sidefx.Groups[2].Value -ne 0 -or
        [int]$sidefx.Groups[3].Value -ne 0 -or
        [int]$sidefx.Groups[4].Value -ne 0) {
        throw "Pupupu update triggered deferred wind/push/quake/particle side effects.`n$gdbStdout"
    }
    if (-not $task.Success -or [int]$task.Groups[1].Value -ne 1 -or [int]$task.Groups[5].Value -ne 22 -or [int]$task.Groups[6].Value -lt 1) {
        throw "Taskman did not park at the VSBattle update boundary.`n$gdbStdout"
    }
    if (-not $roomTitle.Success -or [int]$roomTitle.Groups[1].Value -ne 0 -or (Convert-MarkerUInt32 $roomTitle.Groups[2].Value) -ne 0 -or (Convert-MarkerUInt32 $roomTitle.Groups[3].Value) -ne 0) {
        throw "Battle Pupupu update harness replayed opening/title work.`n$gdbStdout"
    }
    Write-Output ("Battle Pupupu update harness passed: scene=22/21 update={0} ticks={1} whispy={2}->{3} windWait={4}->{5} sidefx={6}/{7}/{8}/{9}" -f $update.Groups[2].Value, $update.Groups[3].Value, $state.Groups[1].Value, $state.Groups[3].Value, $state.Groups[4].Value, $state.Groups[6].Value, $sidefx.Groups[1].Value, $sidefx.Groups[2].Value, $sidefx.Groups[3].Value, $sidefx.Groups[4].Value)
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
