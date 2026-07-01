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
$rom = Join-Path $root 'smash64ds-menu-chain.nds'
$elf = Join-Path $root 'smash64ds-menu-chain.elf'
$melonDsPath = $verifierContext.MelonDSPath
$melonDsDir = Split-Path -Parent $melonDsPath
$logDir = Get-MelonDSVerifierLogDir -Root $root -RunnerSlot (Get-MelonDSActiveRunnerSlot)
$stdout = Join-Path $logDir 'melonds.menu-chain-vsbattle-harness.stdout.log'
$stderr = Join-Path $logDir 'melonds.menu-chain-vsbattle-harness.stderr.log'
$configState = $null
$emulator = $null
$scriptName = '_menu_chain_vsbattle_harness.gdb'
if (-not $env:DEVKITPRO) { $env:DEVKITPRO = 'C:/devkitPro' }
if (-not $env:DEVKITARM) { $env:DEVKITARM = 'C:/devkitPro/devkitARM' }
& make -C $root TARGET=smash64ds-menu-chain BUILD=build-menu-chain-vsbattle-harness NDS_DEV_SCENE_HARNESS=menu_chain_vsbattle -j16
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
if (-not (Test-Path $rom) -or -not (Test-Path $elf)) {
    throw 'Menu chain harness build did not produce smash64ds-menu-chain.nds and smash64ds-menu-chain.elf.'
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
        'printf "SCENE=%u,%u\n", gSCManagerSceneData.scene_curr, gSCManagerSceneData.scene_prev',
        'printf "VS_TRANS=%#x,%#x,%u,%u,%u\n", gNdsVSModeStartTransitionResult, gNdsVSModeStartTransitionMask, gNdsVSModeStartTransitionSceneCurrFinal, gNdsVSModeStartTransitionScenePrevFinal, gNdsVSModeStartTransitionCleanupCount',
        'printf "PV_SETUP=%#x,%#x,%#x,%#x,%#x,%u\n", gNdsPlayersVSOriginalStartResult, gNdsPlayersVSOriginalFuncStartResult, gNdsPlayersVSOriginalRelocResult, gNdsPlayersVSOriginalSetupResult, gNdsPlayersVSOriginalSetupMask, gNdsPlayersVSOriginalLoadedFileCount',
        'printf "PV_TRANS=%#x,%#x,%u,%#x,%u,%u,%u,%u,%u,%u\n", gNdsPlayersVSReadyTransitionResult, gNdsPlayersVSReadyTransitionMask, gNdsPlayersVSReadyTransitionUpdateCount, gNdsPlayersVSReadyTransitionInputMask, gNdsPlayersVSReadyTransitionScenePrevBefore, gNdsPlayersVSReadyTransitionSceneCurrBefore, gNdsPlayersVSReadyTransitionScenePrevFinal, gNdsPlayersVSReadyTransitionSceneCurrFinal, gNdsPlayersVSReadyTransitionPlayerCount, gNdsPlayersVSReadyTransitionStageSelect',
        'printf "MAPS_SETUP=%#x,%#x,%#x,%#x,%#x,%u,%u,%u,%u,%#x\n", gNdsMapsOriginalStartResult, gNdsMapsOriginalFuncStartResult, gNdsMapsOriginalRelocResult, gNdsMapsOriginalSetupResult, gNdsMapsOriginalSetupMask, gNdsMapsOriginalLoadedFileCount, gNdsMapsOriginalCursorSlot, gNdsMapsOriginalGroundKind, gNdsMapsOriginalPreviewDeferred, gNdsMapsOriginalDeferredMask',
        'printf "MAPS_PREVIEW=%#x,%#x,%u,%u,%u,%u\n", gNdsMapsOriginalPreviewResult, gNdsMapsOriginalPreviewMask, gNdsMapsOriginalPreviewGObjCount, gNdsMapsOriginalPreviewWallpaperMade, gNdsMapsOriginalPreviewModelMade, gNdsMapsOriginalPreviewDObjCount',
        'printf "STAGE_RELOC=%#x,%#x,%#x,%u,%u,%u,%u,%u,%u,%u\n", gNdsStagePupupuRelocResult, gNdsStagePupupuRelocAssetMask, gNdsStagePupupuRelocDependencyMask, gNdsStagePupupuExternalFixupCount, gNdsStagePupupuExternalFixupFailCount, gNdsStagePupupuInternalFixupCount, gNdsStagePupupuMapHeaderOffset, gNdsStagePupupuGroundDataPtrReady, gNdsStagePupupuWallpaperPtrReady, gNdsStagePupupuGeometryPtrReady',
        'printf "MAPS_TRANS=%#x,%#x,%u,%#x,%u,%u,%u,%u,%u\n", gNdsMapsSelectTransitionResult, gNdsMapsSelectTransitionMask, gNdsMapsSelectTransitionUpdateCount, gNdsMapsSelectTransitionInputMask, gNdsMapsSelectTransitionScenePrevBefore, gNdsMapsSelectTransitionSceneCurrBefore, gNdsMapsSelectTransitionScenePrevFinal, gNdsMapsSelectTransitionSceneCurrFinal, gNdsMapsSelectTransitionSelectedGKind',
        'printf "VSB_SETUP=%#x,%#x,%#x,%#x,%#x,%u,%u,%u,%u,%u,%#x,%u,%u,%u,%u,%u,%u,%u\n", gNdsSCVSBattleOriginalStartResult, gNdsSCVSBattleOriginalFuncStartResult, gNdsSCVSBattleOriginalRelocResult, gNdsSCVSBattleOriginalSetupResult, gNdsSCVSBattleOriginalSetupMask, gNdsSCVSBattleOriginalLoadedFileCount, gNdsSCVSBattleOriginalGObjCount, gNdsSCVSBattleOriginalCameraCount, gNdsSCVSBattleOriginalFighterGObjCount, gNdsSCVSBattleOriginalActivePlayerMask, gNdsSCVSBattleCompatMask, gNdsSCVSBattleOriginalPlayerCount, gNdsSCVSBattleOriginalCpuCount, gNdsSCVSBattleOriginalGameRule, gNdsSCVSBattleOriginalStock, gNdsSCVSBattleOriginalGKind, gNdsSCVSBattleOriginalSceneCurr, gNdsSCVSBattleOriginalScenePrev',
        'printf "VSB_STAGE=%#x,%#x,%u,%u,%u,%u,%#x\n", gNdsSCVSBattleStageResult, gNdsSCVSBattleStageMask, gNdsSCVSBattleStageGKind, gNdsSCVSBattleStageGroundDataReady, gNdsSCVSBattleStageGeometryReady, gNdsSCVSBattleStageMapNodesReady, gNdsSCVSBattleStageDeferredMask',
        'printf "PUPUPU_GROUND=%#x,%#x,%#x,%#x,%#x,%u,%u,%u,%u,%#x\n", gNdsPupupuGroundSetupResult, gNdsPupupuGroundDisplayResult, gNdsPupupuGroundGObjResult, gNdsPupupuGroundSetupMask, gNdsPupupuGroundDeferredMask, gNdsPupupuGroundLayerGObjCount, gNdsPupupuGroundMapGObjCount, gNdsPupupuGroundMapHeadReady, gNdsPupupuGroundParticleBankID, gNdsPupupuGroundMapHeadOffset',
        'printf "PUPUPU_GOBJS=%#x,%#x,%#x,%#x,%u,%u,%u,%u,%u\n", gNdsPupupuGroundLayerGObjMask, gNdsPupupuGroundMapGObjMask, gNdsPupupuGroundLayerDObjMask, gNdsPupupuGroundLayerMObjMask, gNdsPupupuGroundRootGObjID, gNdsPupupuGroundWhispyEyesGObjID, gNdsPupupuGroundWhispyMouthGObjID, gNdsPupupuGroundFlowersBackGObjID, gNdsPupupuGroundFlowersFrontGObjID',
        'printf "PUPUPU_COUNTS=%u,%u,%u,%u,%u,%u\n", gNdsPupupuGroundGObjCountBefore, gNdsPupupuGroundGObjCountAfter, gNdsPupupuGroundDObjCountAfter, gNdsPupupuGroundMObjCountAfter, gNdsPupupuGroundAObjCountAfter, gNdsPupupuGroundNonPupupuStubCallCount',
        'printf "PUPUPU_UPDATE_ZERO=%#x,%u\n", gNdsPupupuUpdateResult, gNdsPupupuUpdateTickCount',
        'printf "VSB_COMPAT=%#x,%#x,%#x,%#x,%#x,%#x,%u\n", gNdsSCVSBattleCompatCameraMask, gNdsSCVSBattleCompatInterfaceMask, gNdsSCVSBattleCompatManagerMask, gNdsSCVSBattleCompatAudioMask, gNdsSCVSBattleCompatSpawnMask, gNdsSCVSBattleCompatMask, gNdsSCVSBattleLastFGM',
        'printf "VSB_UPDATE=%#x,%u\n", gNdsSCVSBattleOriginalUpdateResult, gNdsSCVSBattleOriginalUpdateCount',
        'printf "TASK=%u,%u,%u,%u,%u\n", gNdsTaskmanLoopReached, gNdsTaskmanSceneUpdateSet, gNdsTaskmanSceneDrawSet, gNdsTaskmanControllerAutoRead, gNdsSceneBoundaryKind',
        'printf "ROOM_TITLE=%u,%#x,%#x\n", gNdsOpeningRoomTickCount, gNdsTitleOriginalStartResult, gNdsTitleDrawResult',
        'detach',
        'quit'
    )
    $gdbStdout = (Invoke-GdbMarkerScript -Gdb $Gdb -Elf $elf -Root $root -Commands $gdbCommands -ScriptName $scriptName).Stdout
    $harn = [regex]::Match($gdbStdout, 'HARN=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0)')
    $scene = [regex]::Match($gdbStdout, 'SCENE=([0-9]+),([0-9]+)')
    $vs = [regex]::Match($gdbStdout, 'VS_TRANS=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+)')
    $pvSetup = [regex]::Match($gdbStdout, 'PV_SETUP=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $pvTrans = [regex]::Match($gdbStdout, 'PV_TRANS=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $mapsSetup = [regex]::Match($gdbStdout, 'MAPS_SETUP=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0)')
    $mapsPreview = [regex]::Match($gdbStdout, 'MAPS_PREVIEW=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageReloc = [regex]::Match($gdbStdout, 'STAGE_RELOC=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $mapsTrans = [regex]::Match($gdbStdout, 'MAPS_TRANS=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $vsbSetup = [regex]::Match($gdbStdout, 'VSB_SETUP=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $vsbStage = [regex]::Match($gdbStdout, 'VSB_STAGE=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0)')
    $pupupuGround = [regex]::Match($gdbStdout, 'PUPUPU_GROUND=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0)')
    $pupupuGObjs = [regex]::Match($gdbStdout, 'PUPUPU_GOBJS=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $pupupuCounts = [regex]::Match($gdbStdout, 'PUPUPU_COUNTS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $pupupuUpdateZero = [regex]::Match($gdbStdout, 'PUPUPU_UPDATE_ZERO=(0x[0-9a-fA-F]+|0),([0-9]+)')
    $vsbCompat = [regex]::Match($gdbStdout, 'VSB_COMPAT=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $vsbUpdate = [regex]::Match($gdbStdout, 'VSB_UPDATE=(0x[0-9a-fA-F]+|0),([0-9]+)')
    $task = [regex]::Match($gdbStdout, 'TASK=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $roomTitle = [regex]::Match($gdbStdout, 'ROOM_TITLE=([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    if (-not $harn.Success -or
        (Convert-MarkerUInt32 $harn.Groups[1].Value) -ne 0x4841524e -or
        [int]$harn.Groups[2].Value -ne 7 -or
        [int]$harn.Groups[3].Value -ne 9 -or
        [int]$harn.Groups[4].Value -ne 1) {
        throw "Menu-chain harness did not start at VSMode from Title.`n$gdbStdout"
    }
    if (-not $vs.Success -or
        (Convert-MarkerUInt32 $vs.Groups[1].Value) -ne 0x56535452 -or
        ((Convert-MarkerUInt32 $vs.Groups[2].Value) -band 0xff) -ne 0xff -or
        [int]$vs.Groups[3].Value -ne 16 -or
        [int]$vs.Groups[4].Value -ne 9 -or
        [int]$vs.Groups[5].Value -ne 1) {
        throw "VSMode did not transition to PlayersVS through original code.`n$gdbStdout"
    }
    if (-not $pvSetup.Success -or
        (Convert-MarkerUInt32 $pvSetup.Groups[1].Value) -ne 0x50565354 -or
        (Convert-MarkerUInt32 $pvSetup.Groups[2].Value) -ne 0x50564653 -or
        (Convert-MarkerUInt32 $pvSetup.Groups[3].Value) -ne 0x5056524c -or
        (Convert-MarkerUInt32 $pvSetup.Groups[4].Value) -ne 0x50565355 -or
        ((Convert-MarkerUInt32 $pvSetup.Groups[5].Value) -band 0xff) -ne 0xff -or
        [int]$pvSetup.Groups[6].Value -ne 7) {
        throw "PlayersVS setup did not run in the menu-chain harness.`n$gdbStdout"
    }
    if (-not $pvTrans.Success -or
        (Convert-MarkerUInt32 $pvTrans.Groups[1].Value) -ne 0x50565452 -or
        ((Convert-MarkerUInt32 $pvTrans.Groups[2].Value) -band 0xff) -ne 0xff -or
        (Convert-MarkerUInt32 $pvTrans.Groups[4].Value) -ne 0x1000 -or
        [int]$pvTrans.Groups[5].Value -ne 9 -or
        [int]$pvTrans.Groups[6].Value -ne 16 -or
        [int]$pvTrans.Groups[7].Value -ne 16 -or
        [int]$pvTrans.Groups[8].Value -ne 21 -or
        [int]$pvTrans.Groups[9].Value -lt 2 -or
        [int]$pvTrans.Groups[10].Value -ne 1) {
        throw "PlayersVS did not transition to Maps through original ready/start logic.`n$gdbStdout"
    }
    if (-not $mapsSetup.Success -or
        (Convert-MarkerUInt32 $mapsSetup.Groups[1].Value) -ne 0x4d415053 -or
        (Convert-MarkerUInt32 $mapsSetup.Groups[2].Value) -ne 0x4d504653 -or
        (Convert-MarkerUInt32 $mapsSetup.Groups[3].Value) -ne 0x4d50524c -or
        (Convert-MarkerUInt32 $mapsSetup.Groups[4].Value) -ne 0x4d505355 -or
        ((Convert-MarkerUInt32 $mapsSetup.Groups[5].Value) -band 0xff) -ne 0xff -or
        [int]$mapsSetup.Groups[6].Value -ne 5 -or
        [int]$mapsSetup.Groups[8].Value -ne 6 -or
        [int]$mapsSetup.Groups[9].Value -ne 0 -or
        (Convert-MarkerUInt32 $mapsSetup.Groups[10].Value) -ne 0) {
        throw "Maps setup did not run in the menu-chain harness.`n$gdbStdout"
    }
    if (-not $mapsPreview.Success -or
        (Convert-MarkerUInt32 $mapsPreview.Groups[1].Value) -ne 0x50555056 -or
        ((Convert-MarkerUInt32 $mapsPreview.Groups[2].Value) -band 0xff) -ne 0xff -or
        [int]$mapsPreview.Groups[3].Value -lt 1 -or
        [int]$mapsPreview.Groups[4].Value -ne 1 -or
        [int]$mapsPreview.Groups[5].Value -ne 1 -or
        [int]$mapsPreview.Groups[6].Value -lt 1) {
        throw "Pupupu Maps preview proof failed in the menu-chain harness.`n$gdbStdout"
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
        throw "Pupupu stage relocation proof failed in the menu-chain harness.`n$gdbStdout"
    }
    if (-not $mapsTrans.Success -or
        (Convert-MarkerUInt32 $mapsTrans.Groups[1].Value) -ne 0x4d53454c -or
        ((Convert-MarkerUInt32 $mapsTrans.Groups[2].Value) -band 0xff) -ne 0xff -or
        (Convert-MarkerUInt32 $mapsTrans.Groups[4].Value) -ne 0x8000 -or
        [int]$mapsTrans.Groups[5].Value -ne 16 -or
        [int]$mapsTrans.Groups[6].Value -ne 21 -or
        [int]$mapsTrans.Groups[7].Value -ne 21 -or
        [int]$mapsTrans.Groups[8].Value -ne 22 -or
        [int]$mapsTrans.Groups[9].Value -ne 6) {
        throw "Maps did not transition to VSBattle through original A-select logic.`n$gdbStdout"
    }
    if (-not $vsbSetup.Success -or
        (Convert-MarkerUInt32 $vsbSetup.Groups[1].Value) -ne 0x56425354 -or
        (Convert-MarkerUInt32 $vsbSetup.Groups[2].Value) -ne 0x56424653 -or
        (Convert-MarkerUInt32 $vsbSetup.Groups[3].Value) -ne 0x5642524c -or
        (Convert-MarkerUInt32 $vsbSetup.Groups[4].Value) -ne 0x56425355 -or
        ((Convert-MarkerUInt32 $vsbSetup.Groups[5].Value) -band 0x7f) -ne 0x7f -or
        [int]$vsbSetup.Groups[6].Value -ne 8 -or
        [int]$vsbSetup.Groups[8].Value -lt 1 -or
        [int]$vsbSetup.Groups[9].Value -lt 2 -or
        [int]$vsbSetup.Groups[10].Value -ne 3 -or
        ((Convert-MarkerUInt32 $vsbSetup.Groups[11].Value) -band 0xff) -ne 0xff -or
        [int]$vsbSetup.Groups[12].Value -lt 2 -or
        [int]$vsbSetup.Groups[13].Value -ne 0 -or
        [int]$vsbSetup.Groups[16].Value -ne 6 -or
        [int]$vsbSetup.Groups[17].Value -ne 22 -or
        [int]$vsbSetup.Groups[18].Value -ne 21) {
        throw "Imported VSBattle setup proof failed after the menu chain.`n$gdbStdout"
    }
    if (-not $vsbStage.Success -or
        (Convert-MarkerUInt32 $vsbStage.Groups[1].Value) -ne 0x50555042 -or
        ((Convert-MarkerUInt32 $vsbStage.Groups[2].Value) -band 0xff) -ne 0xff -or
        [int]$vsbStage.Groups[3].Value -ne 6 -or
        [int]$vsbStage.Groups[4].Value -ne 1 -or
        [int]$vsbStage.Groups[5].Value -ne 1 -or
        [int]$vsbStage.Groups[6].Value -ne 1 -or
        ((Convert-MarkerUInt32 $vsbStage.Groups[7].Value) -band 0x3) -ne 0x3) {
        throw "Pupupu stage adoption proof failed after the menu chain.`n$gdbStdout"
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
        throw "Bounded original Pupupu ground setup proof failed after the menu chain.`n$gdbStdout"
    }
    if (-not $pupupuGObjs.Success -or
        ((Convert-MarkerUInt32 $pupupuGObjs.Groups[1].Value) -eq 0) -or
        ((Convert-MarkerUInt32 $pupupuGObjs.Groups[2].Value) -band 0xf) -ne 0xf -or
        ((Convert-MarkerUInt32 $pupupuGObjs.Groups[3].Value) -eq 0) -or
        [int]$pupupuGObjs.Groups[6].Value -eq 0 -or
        [int]$pupupuGObjs.Groups[7].Value -eq 0 -or
        [int]$pupupuGObjs.Groups[8].Value -eq 0 -or
        [int]$pupupuGObjs.Groups[9].Value -eq 0) {
        throw "Bounded original Pupupu ground GObj graph proof failed after the menu chain.`n$gdbStdout"
    }
    if (-not $pupupuCounts.Success -or
        [int]$pupupuCounts.Groups[2].Value -le [int]$pupupuCounts.Groups[1].Value -or
        [int]$pupupuCounts.Groups[6].Value -ne 0) {
        throw "Pupupu ground object counts did not prove bounded object creation after the menu chain.`n$gdbStdout"
    }
    if (-not $pupupuUpdateZero.Success -or
        (Convert-MarkerUInt32 $pupupuUpdateZero.Groups[1].Value) -ne 0 -or
        [int]$pupupuUpdateZero.Groups[2].Value -ne 0) {
        throw "Setup-only menu-chain VSBattle harness unexpectedly ran the Pupupu update probe.`n$gdbStdout"
    }
    if (-not $vsbCompat.Success -or
        ((Convert-MarkerUInt32 $vsbCompat.Groups[1].Value) -band 0x3f) -ne 0x3f -or
        ((Convert-MarkerUInt32 $vsbCompat.Groups[2].Value) -band 0x37ff) -ne 0x37ff -or
        ((Convert-MarkerUInt32 $vsbCompat.Groups[3].Value) -band 0x1ff) -ne 0x1ff -or
        ((Convert-MarkerUInt32 $vsbCompat.Groups[4].Value) -band 0x12) -ne 0x12 -or
        ((Convert-MarkerUInt32 $vsbCompat.Groups[5].Value) -band 0x3) -ne 0x3 -or
        ((Convert-MarkerUInt32 $vsbCompat.Groups[6].Value) -band 0xff) -ne 0xff -or
        [int]$vsbCompat.Groups[7].Value -ne 626) {
        throw "VSBattle compatibility stub coverage failed after the menu chain.`n$gdbStdout"
    }
    if (-not $vsbUpdate.Success -or (Convert-MarkerUInt32 $vsbUpdate.Groups[1].Value) -ne 0x56425550 -or [int]$vsbUpdate.Groups[2].Value -ne 1) {
        throw "VSBattle interface update tick was not proven after the menu chain.`n$gdbStdout"
    }
    if (-not $scene.Success -or [int]$scene.Groups[1].Value -ne 22 -or [int]$scene.Groups[2].Value -ne 21) {
        throw "Final scene state is not VSBattle from Maps.`n$gdbStdout"
    }
    if (-not $task.Success -or [int]$task.Groups[1].Value -ne 1 -or [int]$task.Groups[5].Value -ne 22) {
        throw "Taskman did not park at the VSBattle stub boundary.`n$gdbStdout"
    }
    if (-not $roomTitle.Success -or [int]$roomTitle.Groups[1].Value -ne 0 -or (Convert-MarkerUInt32 $roomTitle.Groups[2].Value) -ne 0 -or (Convert-MarkerUInt32 $roomTitle.Groups[3].Value) -ne 0) {
        throw "Menu-chain harness replayed opening/title work.`n$gdbStdout"
    }
    Write-Output ("Menu-chain VSBattle harness passed: VS->PV mask={0}, PV->Maps mask={1}, Maps preview={2}, Maps->VSBattle mask={3}, stage={4}, ground={5}, final={6}/{7}" -f $vs.Groups[2].Value, $pvTrans.Groups[2].Value, $mapsPreview.Groups[2].Value, $mapsTrans.Groups[2].Value, $vsbStage.Groups[2].Value, $pupupuGround.Groups[4].Value, $scene.Groups[1].Value, $scene.Groups[2].Value)
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
