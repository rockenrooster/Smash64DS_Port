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
$rom = Join-Path $root 'smash64ds-maps.nds'
$elf = Join-Path $root 'smash64ds-maps.elf'
$melonDsPath = $verifierContext.MelonDSPath
$melonDsDir = Split-Path -Parent $melonDsPath
$logDir = Get-MelonDSVerifierLogDir -Root $root -RunnerSlot (Get-MelonDSActiveRunnerSlot)
$stdout = Join-Path $logDir 'melonds.maps-setup-harness.stdout.log'
$stderr = Join-Path $logDir 'melonds.maps-setup-harness.stderr.log'
$configState = $null
$emulator = $null
$scriptName = '_maps_setup_harness.gdb'
if (-not $env:DEVKITPRO) { $env:DEVKITPRO = 'C:/devkitPro' }
if (-not $env:DEVKITARM) { $env:DEVKITARM = 'C:/devkitPro/devkitARM' }
& make -C $root TARGET=smash64ds-maps BUILD=build-maps-setup-harness NDS_DEV_SCENE_HARNESS=maps_setup -j16
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
if (-not (Test-Path $rom) -or -not (Test-Path $elf)) {
    throw 'Maps setup harness build did not produce smash64ds-maps.nds and smash64ds-maps.elf.'
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
        'printf "MAPS_SETUP=%#x,%#x,%#x,%#x,%#x,%u,%u,%u,%u,%u,%u,%u,%u,%#x\n", gNdsMapsOriginalStartResult, gNdsMapsOriginalFuncStartResult, gNdsMapsOriginalRelocResult, gNdsMapsOriginalSetupResult, gNdsMapsOriginalSetupMask, gNdsMapsOriginalLoadedFileCount, gNdsMapsOriginalGObjCount, gNdsMapsOriginalCameraCount, gNdsMapsOriginalSObjCount, gNdsMapsOriginalMainGObjID, gNdsMapsOriginalCursorSlot, gNdsMapsOriginalGroundKind, gNdsMapsOriginalPreviewDeferred, gNdsMapsOriginalDeferredMask',
        'printf "MAPS_PREVIEW=%#x,%#x,%u,%#x,%u,%u,%u,%u\n", gNdsMapsOriginalPreviewResult, gNdsMapsOriginalPreviewMask, gNdsMapsOriginalPreviewGObjCount, gNdsMapsOriginalPreviewLayerGObjMask, gNdsMapsOriginalPreviewWallpaperMade, gNdsMapsOriginalPreviewModelMade, gNdsMapsOriginalPreviewDObjCount, gNdsMapsOriginalPreviewMObjCount',
        'printf "STAGE_RELOC=%#x,%#x,%#x,%u,%u,%u,%u,%u,%u,%u\n", gNdsStagePupupuRelocResult, gNdsStagePupupuRelocAssetMask, gNdsStagePupupuRelocDependencyMask, gNdsStagePupupuExternalFixupCount, gNdsStagePupupuExternalFixupFailCount, gNdsStagePupupuInternalFixupCount, gNdsStagePupupuMapHeaderOffset, gNdsStagePupupuGroundDataPtrReady, gNdsStagePupupuWallpaperPtrReady, gNdsStagePupupuGeometryPtrReady',
        'printf "MAPS_TRANS=%#x,%#x\n", gNdsMapsSelectTransitionResult, gNdsMapsSelectTransitionMask',
        'printf "TASK=%u,%u,%u,%u,%u\n", gNdsTaskmanLoopReached, gNdsTaskmanSceneUpdateSet, gNdsTaskmanSceneDrawSet, gNdsTaskmanControllerAutoRead, gNdsSceneBoundaryKind',
        'printf "ROOM_TITLE=%u,%#x,%#x\n", gNdsOpeningRoomTickCount, gNdsTitleOriginalStartResult, gNdsTitleDrawResult',
        'detach',
        'quit'
    )
    $gdbStdout = (Invoke-GdbMarkerScript -Gdb $Gdb -Elf $elf -Root $root -Commands $gdbCommands -ScriptName $scriptName).Stdout
    $harn = [regex]::Match($gdbStdout, 'HARN=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0)')
    $scene = [regex]::Match($gdbStdout, 'SCENE=([0-9]+),([0-9]+)')
    $setup = [regex]::Match($gdbStdout, 'MAPS_SETUP=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0)')
    $preview = [regex]::Match($gdbStdout, 'MAPS_PREVIEW=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageReloc = [regex]::Match($gdbStdout, 'STAGE_RELOC=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $trans = [regex]::Match($gdbStdout, 'MAPS_TRANS=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $task = [regex]::Match($gdbStdout, 'TASK=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $roomTitle = [regex]::Match($gdbStdout, 'ROOM_TITLE=([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    if (-not $harn.Success -or
        (Convert-MarkerUInt32 $harn.Groups[1].Value) -ne 0x4841524e -or
        [int]$harn.Groups[2].Value -ne 6 -or
        [int]$harn.Groups[3].Value -ne 21 -or
        [int]$harn.Groups[4].Value -ne 16) {
        throw "Maps setup harness did not select Maps from PlayersVS.`n$gdbStdout"
    }
    if (-not $scene.Success -or [int]$scene.Groups[1].Value -ne 21 -or [int]$scene.Groups[2].Value -ne 16) {
        throw "Live scene state is not Maps from PlayersVS.`n$gdbStdout"
    }
    if (-not $setup.Success -or
        (Convert-MarkerUInt32 $setup.Groups[1].Value) -ne 0x4d415053 -or
        (Convert-MarkerUInt32 $setup.Groups[2].Value) -ne 0x4d504653 -or
        (Convert-MarkerUInt32 $setup.Groups[3].Value) -ne 0x4d50524c -or
        (Convert-MarkerUInt32 $setup.Groups[4].Value) -ne 0x4d505355 -or
        ((Convert-MarkerUInt32 $setup.Groups[5].Value) -band 0xff) -ne 0xff -or
        [int]$setup.Groups[6].Value -ne 5 -or
        [int]$setup.Groups[8].Value -lt 1 -or
        [int]$setup.Groups[9].Value -lt 10 -or
        [int]$setup.Groups[12].Value -ne 6 -or
        [int]$setup.Groups[13].Value -ne 0 -or
        (Convert-MarkerUInt32 $setup.Groups[14].Value) -ne 0) {
        throw "Imported Maps setup proof failed.`n$gdbStdout"
    }
    if (-not $preview.Success -or
        (Convert-MarkerUInt32 $preview.Groups[1].Value) -ne 0x50555056 -or
        ((Convert-MarkerUInt32 $preview.Groups[2].Value) -band 0xff) -ne 0xff -or
        [int]$preview.Groups[3].Value -lt 1 -or
        (Convert-MarkerUInt32 $preview.Groups[4].Value) -eq 0 -or
        [int]$preview.Groups[5].Value -ne 1 -or
        [int]$preview.Groups[6].Value -ne 1 -or
        [int]$preview.Groups[7].Value -lt 1) {
        throw "Pupupu Maps preview proof failed.`n$gdbStdout"
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
        throw "Pupupu stage relocation proof failed in Maps setup.`n$gdbStdout"
    }
    if (-not $trans.Success -or (Convert-MarkerUInt32 $trans.Groups[1].Value) -ne 0 -or (Convert-MarkerUInt32 $trans.Groups[2].Value) -ne 0) {
        throw "Maps setup harness unexpectedly ran the A-select transition.`n$gdbStdout"
    }
    if (-not $task.Success -or [int]$task.Groups[1].Value -ne 1 -or [int]$task.Groups[5].Value -ne 21) {
        throw "Taskman did not park at the Maps boundary.`n$gdbStdout"
    }
    if (-not $roomTitle.Success -or [int]$roomTitle.Groups[1].Value -ne 0 -or (Convert-MarkerUInt32 $roomTitle.Groups[2].Value) -ne 0 -or (Convert-MarkerUInt32 $roomTitle.Groups[3].Value) -ne 0) {
        throw "Maps setup harness replayed opening/title work.`n$gdbStdout"
    }
    Write-Output ("Maps setup harness passed: scene=21/16 setup={0} files={1} preview={2} assets={3} slot={4} gkind={5}" -f $setup.Groups[5].Value, $setup.Groups[6].Value, $preview.Groups[2].Value, $stageReloc.Groups[2].Value, $setup.Groups[11].Value, $setup.Groups[12].Value)
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
