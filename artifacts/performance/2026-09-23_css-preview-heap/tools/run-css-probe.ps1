param(
    [Parameter(Mandatory = $true)][string]$Build,
    [string]$Target = 'smash64ds-p2-shell-freeplay-hwtri',
    [int]$RunnerSlot = 7,
    [int]$DelaySeconds = 30,
    [string]$Out = ''
)
# CSS preview coverage on a MENU_WALK ROM: let the walk tour park on all
# twelve fighters, then attach once and read the tour masks, the preview
# pack loader counters and the heap.
$ErrorActionPreference = 'Stop'
$root = 'D:\Stuff\DevFolder\Smash64DS_Port'
. (Join-Path $root 'scripts\lib\melonds.ps1')
. (Join-Path $root 'scripts\lib\build-output.ps1')
$gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe'
$context = Initialize-MelonDSVerifierContext `
    -Root $root -MelonDS (Join-Path $root 'emulators\melonds\melonDS.exe') `
    -RunnerSlot $RunnerSlot -NoBuild:$true
$rom = Resolve-Smash64DSBuildOutput -Root $root -Target $Target -Build $Build -Extension '.nds'
$elf = Resolve-Smash64DSBuildOutput -Root $root -Target $Target -Build $Build -Extension '.elf'
$temp = Get-MelonDSVerifierTempDir -Root $root -RunnerSlot $RunnerSlot
$configState = $null
$emulator = $null
try {
    $configState = Enable-MelonDSGdbConfig `
        -MelonDSPath $context.MelonDSPath -GdbPort $context.GdbPort `
        -Arm7Port $context.Arm7Port -Persistent:([bool]$context.PersistentConfig) -MuteAudio
    $emulator = Start-Process -FilePath $context.MelonDSPath -ArgumentList $rom `
        -WorkingDirectory (Split-Path -Parent $context.MelonDSPath) `
        -WindowStyle Hidden -PassThru
    Wait-MelonDSGdbListener -Process $emulator -Port $context.GdbPort | Out-Null
    Start-Sleep -Seconds $DelaySeconds
    $script = Join-Path $temp 'css-probe.gdb'
    [System.IO.File]::WriteAllLines($script, @(
        'set pagination off', 'set confirm off', 'set remotetimeout 60',
        "target remote 127.0.0.1:$($context.GdbPort)",
        'printf "SCENE curr=%u\n", gSCManagerSceneData.scene_curr',
        'printf "TOUR kind=0x%03x drew=0x%03x done=%u notready=%u\n", gNdsMenuShellCssWalkTourKindMask, gNdsMenuShellCssWalkTourDrewMask, gNdsMenuShellCssWalkTourDoneCount, gNdsMenuShellCssWalkTourNotReadyCount',
        'print/u gNdsMenuShellCssWalkTourTriangles',
        'print/u gNdsMenuShellCssLoaderWitness',
        'printf "PACK loads=%u fail=%u failkind=%u bytes=%u halt=%u commit=%u cancel=%u steps=%u\n", gNdsPreviewPackLoadCount, gNdsPreviewPackFailure, gNdsPreviewPackFailureKind, gNdsPreviewPackDataBytes, gNdsPreviewPackHaltDecline, gNdsPreviewPackStageCommitCount, gNdsPreviewPackStageCancelCount, gNdsPreviewPackStepCount',
        'printf "ACQ n=%u load=%u fin=%u fail=%u retry=%u hit=%u cached=%u cap=%u\n", gNdsPlayersVSPreviewAcquireCount, gNdsPlayersVSPreviewAcquireLoadCount, gNdsPlayersVSPreviewAcquireLoadFinishCount, gNdsPlayersVSPreviewAcquireFailCount, gNdsPlayersVSPreviewAcquireRetryCount, gNdsPlayersVSPreviewAcquireHitCount, gNdsPlayersVSPreviewAcquireCachedHitCount, gNdsPlayersVSPreviewResidentCapacityFailCount',
        'printf "PREV draw=%u frames=%u visible=0x%x selected=0x%x ready=0x%x prepare=0x%x mainfail=0x%x ownerfail=0x%x animfail=0x%x subfail=0x%x dwellreq=%u dwellcommit=%u dwellskip=%u\n", gNdsPlayersVSPreviewDrawCount, gNdsPlayersVSPreviewFrameCount, gNdsPlayersVSPreviewVisibleMask, gNdsPlayersVSPreviewSelectedMask, gNdsPlayersVSPreviewResidentReadyMask, gNdsPlayersVSPreviewResidentPrepareMask, gNdsPlayersVSPreviewResidentMainFailMask, gNdsPlayersVSPreviewResidentOwnerFailMask, gNdsPlayersVSPreviewResidentAnimFailMask, gNdsPlayersVSPreviewResidentSubmotionFailMask, gNdsPlayersVSPreviewDwellRequestCount, gNdsPlayersVSPreviewDwellCommitCount, gNdsPlayersVSPreviewDwellSkipCount',
        'printf "ORIG setup=0x%x setupres=%d start=%d reloc=%d loaded=%u deferred=0x%x\n", gNdsPlayersVSOriginalSetupMask, gNdsPlayersVSOriginalSetupResult, gNdsPlayersVSOriginalStartResult, gNdsPlayersVSOriginalRelocResult, gNdsPlayersVSOriginalLoadedFileCount, gNdsPlayersVSOriginalDeferredMask',
        'print gNdsPlayersVSPreviewStatus',
        'print/x sNdsPlayersVSSharedResidentBase',
        'print/x sNdsPlayersVSResidentPoolsReady',
        'print/x sNdsPlayersVSResidentBlocks[0].base',
        'print/x sNdsPlayersVSResidentBlocks[1].base',
        'print/x sNdsPlayersVSResidentBlocks[2].base',
        'print/x sNdsPlayersVSResidentBlocks[3].base',
        'print/x gSYTaskmanGeneralHeap',
        'printf "ANIMCACHE reserved=%u used=%u count=%u fail=%u failskips=%u overflows=%u\n", gNdsR2AnimCacheArenaReservedBytes, gNdsR2AnimCacheArenaUsedBytes, gNdsR2AnimCacheArenaReserveCount, gNdsR2AnimCacheArenaReserveFailCount, gNdsR2AnimCacheArenaReserveFailSkips, gNdsR2AnimCacheArenaOverflows',
        'printf "HEAP free=%u freemin=%u arena=%u\n", (unsigned)gSYTaskmanGeneralHeap.end - (unsigned)gSYTaskmanGeneralHeap.ptr, gNdsTaskmanGeneralHeapFreeMin, gNdsTaskmanArenaChosenSize',
        'printf "TRI p0=%u\n", gNdsFighterDLAllDrawP0HardwareTriangleCount',
        'printf "NATIVE fail=%u\n", gNdsRendererNativeFailure.count',
        'print gNdsRendererNativeFailure',
        'detach', 'quit'
    ))
    $result = & $gdb -q -batch -x $script $elf 2>&1
    if ($Out -ne '') { $result | Set-Content -LiteralPath $Out }
    $result | Select-Object -Last 40
}
finally {
    if ($emulator -and -not $emulator.HasExited) {
        Stop-Process -Id $emulator.Id -Force -ErrorAction SilentlyContinue
    }
    if ($configState) { Restore-MelonDSGdbConfig -State $configState }
}
