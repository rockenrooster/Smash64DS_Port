param(
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$GdbPort = 3333,
    [int]$RunnerSlot = -1,
    [switch]$NoBuild
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
$runnerSlot = Get-MelonDSActiveRunnerSlot
$selectedGdbPort = Get-MelonDSActiveGdbPort
$tempDir = Get-MelonDSVerifierTempDir -Root $root -RunnerSlot $runnerSlot
$rom = Join-Path $root 'smash64ds.nds'
$elf = Join-Path $root 'smash64ds.elf'
$melonDsPath = $verifierContext.MelonDSPath
$melonDsDir = Split-Path -Parent $melonDsPath
$config = Join-Path $melonDsDir 'melonDS.toml'
$logDir = Get-MelonDSVerifierLogDir -Root $root -RunnerSlot $runnerSlot
$stdout = Join-Path $logDir 'melonds.verify.stdout.log'
$stderr = Join-Path $logDir 'melonds.verify.stderr.log'
$gdbStdoutPath = Join-Path $tempDir '_verify_markers.gdb.out'
$gdbStderrPath = Join-Path $tempDir '_verify_markers.gdb.err'
$originalConfig = $null
$emulator = $null
$runtimeStopwatch = $null
if (-not (Test-Path $rom) -or -not (Test-Path $elf)) {
    throw 'Build smash64ds.nds and smash64ds.elf before runtime verification.'
}
if (-not (Test-Path $melonDsPath)) {
    throw "melonDS executable not found: $melonDsPath"
}
New-Item -ItemType Directory -Path $logDir -Force | Out-Null
try {
    if (Test-Path $config) {
        $originalConfig = Get-Content $config -Raw
    } else {
        $seed = Start-Process -FilePath $melonDsPath -ArgumentList $rom `
            -WorkingDirectory $melonDsDir -WindowStyle Hidden -PassThru
        Start-Sleep -Seconds 1
        if (-not $seed.HasExited) { Stop-Process -Id $seed.Id -Force }
        Start-Sleep -Milliseconds 250
    }
    $text = Get-Content $config -Raw
    $gdbSectionPattern = '(?s)\[Instance0\.Gdb\](?:(?!\r?\n\[).)*?'
    $enabled = $text -replace `
        '(?s)(\[Instance0\.Gdb\](?:(?!\r?\n\[).)*?\bEnabled\s*=\s*)false', `
        '${1}true'
    $enabled = $enabled -replace `
        '(?s)(\[Instance0\.Gdb\](?:(?!\r?\n\[).)*?\bEnable\s*=\s*)false', `
        '${1}true'
    if ($enabled -notmatch "$gdbSectionPattern\bEnabled\s*=\s*true") {
        $enabled = $enabled -replace `
            '(?m)(^\[Instance0\.Gdb\]\s*\r?\n)', `
            "`$1Enabled = true`r`n"
    }
    if ($enabled -notmatch "$gdbSectionPattern\bEnable\s*=\s*true") {
        $enabled = $enabled -replace `
            '(?m)(^\[Instance0\.Gdb\]\s*\r?\n)', `
            "`$1Enable = true`r`n"
    }
    if ($enabled -notmatch "$gdbSectionPattern\bEnabled\s*=\s*true" -or
        $enabled -notmatch "$gdbSectionPattern\bEnable\s*=\s*true") {
        throw 'Could not enable the melonDS ARM9 GDB stub.'
    }
    Set-Content $config -Value $enabled -NoNewline
    Set-MelonDSGdbConfig -MelonDSPath $melonDsPath -GdbPort $selectedGdbPort -Persistent:($runnerSlot -ge 0) | Out-Null
    Remove-Item $stdout, $stderr -Force -ErrorAction SilentlyContinue
    $emulator = Start-Process -FilePath $melonDsPath -ArgumentList $rom `
        -WorkingDirectory $melonDsDir -WindowStyle Hidden `
        -RedirectStandardOutput $stdout -RedirectStandardError $stderr `
        -PassThru
    $runtimeStopwatch = [System.Diagnostics.Stopwatch]::StartNew()
    # Startup uses 55 original updates, Opening Room advances to its natural
    # movie handoff, Opening Portraits runs to Mario, eight bounded name-card
    # scenes run to OpeningRun, then the bounded action-scene bridge reaches
    # Title. Sample inside the maintained Title boundary window; waiting too
    # long can let later bounded test paths replace the startup diagnostics.
    Start-Sleep -Seconds 135
    $listener = $null
    for ($i = 0; $i -lt 60; $i++) {
        $emulator.Refresh()
        if ($emulator.HasExited) {
            throw "melonDS exited before the ARM9 GDB sample point (exit $($emulator.ExitCode))."
        }
        $listener = Get-NetTCPConnection -LocalPort $selectedGdbPort -State Listen `
            -ErrorAction SilentlyContinue |
            Where-Object { $_.OwningProcess -eq $emulator.Id } |
            Select-Object -First 1
        if ($null -ne $listener) {
            break
        }
        Start-Sleep -Milliseconds 500
    }
    if ($null -eq $listener) {
        throw "melonDS did not open the ARM9 GDB listener on 127.0.0.1:$selectedGdbPort."
    }
    # Capture GDB output via System.Diagnostics.Process with redirected stderr.
    # The native `& $Gdb ... 2>&1` form turns GDB's benign debug-info warnings
    # (e.g. the calico common.h path baked into the devkitARM GDB binary) into a
    # terminating NativeCommandError under $ErrorActionPreference = 'Stop', which
    # aborts before any marker check runs. Redirecting stderr to a variable
    # isolates those warnings while still surfacing real GDB errors through the
    # exit code and empty stdout.
    $gdbScriptPath = Join-Path $tempDir '_verify_markers.gdb'
    $gdbCommands = @(
        'set pagination off',
        'set confirm off',
        "target remote 127.0.0.1:$selectedGdbPort",
        'printf "AOBJ32=%u,%u,%u,%u,%#x,%#x\n", gNdsAObjEvent32NormalizeScriptCount, gNdsAObjEvent32NormalizeCommandCount, gNdsAObjEvent32NormalizeReuseCount, gNdsAObjEvent32NormalizeFailCount, gNdsAObjEvent32NormalizeFirstSourceWord, gNdsAObjEvent32NormalizeFirstNativeWord',
        'printf "AOBJ32_FAIL=%u,%u,%#x,%#x,%u,%#x\n", gNdsAObjEvent32NormalizeLastFailReason, gNdsAObjEvent32NormalizeLastFailOwner, gNdsAObjEvent32NormalizeLastFailAddress, gNdsAObjEvent32NormalizeLastFailWord, gNdsAObjEvent32NormalizeLastFailOpcode, gNdsAObjEvent32NormalizeLastFailFlags',
        'printf "AOBJ32_COLOR=%u\n", gNdsAObjEvent32ColorCorrectionCount',
        'printf "SELFTEST=%#x\n", gNdsBootSelfTestResult',
        'printf "BOOT=%#x\n", gNdsOriginalBootStage',
        'printf "SCHED=%u\n", sSYSchedulerTicCount',
        'printf "CONTROLLERS=%u\n", gSYControllerConnectedNum',
        'printf "CONTROLLER_POLLS=%u\n", gNdsControllerPollCount',
        'printf "VIDEO=%#x\n", gNdsVideoBootstrapResult',
        'printf "RELOC_NITRO=%#x\n", gNdsRelocAssetInitResult',
        'printf "RELOC_HEADERS=%u\n", gNdsRelocAssetHeaderReadCount',
        'printf "RELOC_PAYLOADS=%u\n", gNdsRelocAssetPayloadReadCount',
        'printf "RELOC_OPEN_FAILS=%u\n", gNdsRelocAssetOpenFailCount',
        'printf "RELOC_FORMAT_FAILS=%u\n", gNdsRelocAssetFormatFailCount',
        'printf "RELOC_SHORT_READS=%u\n", gNdsRelocAssetShortReadCount',
        'printf "SCENE=%#x\n", gNdsSceneBoundaryResult',
        'printf "SCENE_KIND=%u\n", gNdsSceneBoundaryKind',
        'printf "STARTUP=%#x\n", gNdsStartupTaskmanResult',
        'printf "STARTUP_KIND=%u\n", gNdsStartupTaskmanSceneKind',
        'printf "STARTUP_DL0=%u\n", gNdsStartupTaskmanDL0Size',
        'printf "STARTUP_DL1=%u\n", gNdsStartupTaskmanDL1Size',
        'printf "STARTUP_CONTROLLER=%u\n", gNdsStartupTaskmanControllerSet',
        'printf "STARTUP_FUNC=%#x\n", gNdsStartupFuncStartResult',
        'printf "STARTUP_SKIP=%u\n", gNdsStartupSkipAllowWait',
        'printf "STARTUP_OPENING=%u\n", gNdsStartupProceedOpening',
        'printf "STARTUP_GOBJS=%u\n", gNdsStartupGObjCreateCount',
        'printf "STARTUP_CAMERAS=%u\n", gNdsStartupCameraCreateCount',
        'printf "STARTUP_RELOC=%u\n", gNdsStartupRelocInitCount',
        'printf "STARTUP_SPRITES=%u\n", gNdsStartupSpriteCreateCount',
        'printf "STARTUP_FADES=%u\n", gNdsStartupFadeCreateCount',
        'printf "STARTUP_PARENT=%u\n", gNdsStartupWallpaperParentValid',
        'printf "STARTUP_LOGO_X=%u\n", gNdsStartupLogoPosX',
        'printf "STARTUP_LOGO_Y=%u\n", gNdsStartupLogoPosY',
        'printf "STARTUP_FASTCOPY=%u\n", gNdsStartupLogoFastcopyCleared',
        'printf "STARTUP_LOGO_RELOC=%#x\n", gNdsStartupLogoRelocResult',
        'printf "STARTUP_LOGO_RELOC_SIZE=%u\n", gNdsStartupLogoRelocSize',
        'printf "STARTUP_LOGO_SWAP_COUNT=%u\n", gNdsStartupLogoRelocWordSwapCount',
        'printf "STARTUP_LOGO_POINTER_COUNT=%u\n", gNdsStartupLogoRelocPointerFixupCount',
        'printf "STARTUP_LOGO_DRAW=%#x\n", gNdsStartupLogoDrawResult',
        'printf "STARTUP_LOGO_BLOCKER=%u\n", gNdsStartupLogoDrawBlocker',
        'printf "STARTUP_LOGO_CALLBACKS=%u\n", gNdsStartupLogoDrawCallbackCount',
        'printf "STARTUP_LOGO_DRAW_UPDATE=%u\n", gNdsStartupLogoDrawUpdateCount',
        'printf "STARTUP_LOGO_DRAW_W=%u\n", gNdsStartupLogoDrawWidth',
        'printf "STARTUP_LOGO_DRAW_H=%u\n", gNdsStartupLogoDrawHeight',
        'printf "STARTUP_LOGO_DRAW_FMT=%u\n", gNdsStartupLogoDrawFormat',
        'printf "STARTUP_LOGO_DRAW_SIZ=%u\n", gNdsStartupLogoDrawSize',
        'printf "STARTUP_LOGO_DRAW_BM=%u\n", gNdsStartupLogoDrawBitmaps',
        'printf "STARTUP_LOGO_DRAW_PIX=%u\n", gNdsStartupLogoDrawPixels',
        'printf "STARTUP_LOGO_DRAW_GOBJ=%u\n", gNdsStartupLogoDrawGObjID',
        'printf "STARTUP_LOGO_DRAW_KIND=%u\n", gNdsStartupLogoDrawGObjObjKind',
        'printf "STARTUP_LOGO_DRAW_ATTR=%#x\n", gNdsStartupLogoDrawSObjAttr',
        'printf "STARTUP_LOGO_DRAW_TEXSHUF=%u,%u\n", gNdsStartupLogoDrawTexshuf, gNdsStartupLogoDrawTexshufSamples',
        'printf "STARTUP_LOGO_DISPLAY=%u,%u\n", gNdsOriginalSpritePreviewDisplayWidth, gNdsOriginalSpritePreviewDisplayHeight',
        'printf "STARTUP_LOGO_DRAW_VISIBLE=%u\n", gNdsStartupLogoDrawVisibleSObjCount',
        'printf "STARTUP_ACTOR_FUNC=%u\n", gNdsStartupActorFuncSet',
        'printf "STARTUP_PROC_KIND=%u\n", gNdsStartupWallpaperProcessKind',
        'printf "STARTUP_PROC_PRIORITY=%u\n", gNdsStartupWallpaperProcessPriority',
        'printf "STARTUP_DISPLAY=%u\n", gNdsStartupWallpaperDisplaySet',
        'printf "STARTUP_CAM_MASK=%u\n", gNdsStartupWallpaperCameraMaskLow',
        'printf "STARTUP_DEFAULT_COLOR=%#x\n", gNdsStartupDefaultCameraColor',
        'printf "TASKMAN=%#x\n", gNdsTaskmanBridgeResult',
        'printf "TASKMAN_CONTEXTS=%u\n", gNdsTaskmanContexts',
        'printf "TASKMAN_GFXNUM=%u\n", gNdsTaskmanTaskGfxNum',
        'printf "TASKMAN_GFXHEAP=%u\n", gNdsTaskmanGraphicsHeapSize',
        'printf "TASKMAN_RDP_KIND=%u\n", gNdsTaskmanRdpKind',
        'printf "TASKMAN_RDP_SIZE=%u\n", gNdsTaskmanRdpBufferSize',
        'printf "TASKMAN_MALLOCS=%u\n", gNdsTaskmanMallocCount',
        'printf "STARTUP_TASKMAN_MALLOCS=%u\n", gNdsStartupTaskmanMallocCount',
        'printf "TASKMAN_USED=%u\n", gNdsTaskmanGeneralHeapUsed',
        'printf "TASKMAN_DL_VALID=%u\n", gNdsTaskmanDLContextsValid',
        'printf "TASKMAN_AUTOREAD=%u\n", gNdsTaskmanControllerAutoRead',
        'printf "TASKMAN_UPDATE=%u\n", gNdsTaskmanSceneUpdateSet',
        'printf "TASKMAN_DRAW=%u\n", gNdsTaskmanSceneDrawSet',
        'printf "TASKMAN_LIGHTS=%u\n", gNdsTaskmanLightsSet',
        'printf "TASKMAN_LOOP=%u\n", gNdsTaskmanLoopReached',
        'printf "TASKMAN_UPDATES=%u\n", gNdsTaskmanBoundedUpdateCount',
        'printf "TASKMAN_SKIP_AFTER=%u\n", gNdsTaskmanPostUpdateSkip',
        'printf "TASKMAN_GOBJ_SLEEPS=%u\n", gNdsTaskmanGObjThreadSleeps',
        'printf "TASKMAN_LOGO_POST_X=%u\n", gNdsTaskmanPostUpdateLogoPosX',
        'printf "TASKMAN_LOGO_POST_Y=%u\n", gNdsTaskmanPostUpdateLogoPosY',
        'printf "TASKMAN_OPENING=%u\n", gNdsTaskmanPostUpdateOpening',
        'printf "TASKMAN_POST_SCENE=%u\n", gNdsTaskmanPostUpdateSceneKind',
        'printf "TASKMAN_POST_PREV=%u\n", gNdsTaskmanPostUpdateScenePrev',
        'printf "TASKMAN_POST_STATUS=%u\n", gNdsTaskmanPostUpdateStatus',
        'printf "TASKMAN_POST_GOBJS=%u\n", gNdsTaskmanPostUpdateGObjCount',
        'printf "TASKMAN_POST_FADES=%u\n", gNdsTaskmanPostUpdateFadeCount',
        'printf "TASKMAN_CLEANUP=%#x\n", gNdsTaskmanCleanupResult',
        'printf "TASKMAN_CLEANUP_QUEUES=%u\n", gNdsTaskmanCleanupQueuesEmpty',
        'printf "TASKMAN_CLEANUP_MODE=%u\n", gNdsTaskmanCleanupMode',
        'printf "TASKMAN_RETURNS=%u\n", gNdsTaskmanReturnCount',
        'printf "OPENING_ROOM_DISPATCH=%u\n", gNdsOpeningRoomDispatchCount',
        'printf "OPENING_ROOM_START=%#x\n", gNdsOpeningRoomStartResult',
        'printf "OPENING_ROOM_FUNC_START=%#x\n", gNdsOpeningRoomFuncStartResult',
        'printf "OPENING_ROOM_RELOC=%#x\n", gNdsOpeningRoomRelocResult',
        'printf "OPENING_ROOM_RELOC_INIT=%u\n", gNdsOpeningRoomRelocInitCount',
        'printf "OPENING_ROOM_RELOC_LOAD=%u\n", gNdsOpeningRoomRelocLoadCount',
        'printf "OPENING_ROOM_RELOC_MASK=%#x\n", gNdsOpeningRoomRelocFileMask',
        'printf "OPENING_ROOM_RELOC_HEADER_MASK=%#x\n", gNdsOpeningRoomRelocHeaderMask',
        'printf "OPENING_ROOM_RELOC_PAYLOAD_MASK=%#x\n", gNdsOpeningRoomRelocPayloadMask',
        'printf "OPENING_ROOM_RELOC_DATA=%u\n", gNdsOpeningRoomRelocContentReady',
        'printf "OPENING_ROOM_RELOC_FIXUP=%u\n", gNdsOpeningRoomRelocFixupReady',
        'printf "OPENING_ROOM_RELOC_BYTES=%u\n", gNdsOpeningRoomRelocBytesLoaded',
        'printf "OPENING_ROOM_RELOC_LAST_ID=%u\n", gNdsOpeningRoomRelocLastFileID',
        'printf "OPENING_ROOM_RELOC_LAST_SIZE=%u\n", gNdsOpeningRoomRelocLastSize',
        'printf "OPENING_ROOM_RELOC_WORD_SWAP_MASK=%#x\n", gNdsOpeningRoomRelocWordSwapMask',
        'printf "OPENING_ROOM_RELOC_WORD_SWAP_COUNT=%u\n", gNdsOpeningRoomRelocWordSwapCount',
        'printf "OPENING_ROOM_RELOC_WORD_SWAP_FAILS=%u\n", gNdsOpeningRoomRelocWordSwapFailCount',
        'printf "OPENING_ROOM_RELOC_POINTER_MASK=%#x\n", gNdsOpeningRoomRelocPointerFixupMask',
        'printf "OPENING_ROOM_RELOC_POINTER_COUNT=%u\n", gNdsOpeningRoomRelocPointerFixupCount',
        'printf "OPENING_ROOM_RELOC_POINTER_FAILS=%u\n", gNdsOpeningRoomRelocPointerFixupFailCount',
        'printf "OPENING_ROOM_RELOC_SYMBOL_COUNT=%u\n", gNdsOpeningRoomRelocSymbolResolveCount',
        'printf "OPENING_ROOM_RELOC_SYMBOL_FAILS=%u\n", gNdsOpeningRoomRelocSymbolResolveFailCount',
        'printf "OPENING_ROOM_RELOC_SYMBOL_PROBE_MASK=%#x\n", gNdsOpeningRoomRelocSymbolProbeMask',
        'printf "OPENING_ROOM_RELOC_SYMBOL_LAST=%u\n", gNdsOpeningRoomRelocLastSymbolOffset',
        'printf "OPENING_ROOM_RELOC_MOBJ_NORMALIZE=%u,%u,%#x\n", gNdsOpeningRoomRelocMObjSubNormalizeCount, gNdsOpeningRoomRelocMObjSubNormalizeFailCount, gNdsOpeningRoomRelocMObjSubFirstFlags',
        'printf "OPENING_ROOM_RELOC_MOBJ_SOURCE=%#x,%u,%u,%u,%u,%u,%#x,%#x\n", gNdsOpeningRoomRelocMObjSubSourceResult, gNdsOpeningRoomRelocMObjSubNormalizeCount, gNdsOpeningRoomRelocMObjSubTextureFlagCount, gNdsOpeningRoomRelocMObjSubZeroFlagCount, gNdsOpeningRoomRelocMObjSubPrimColorCount, gNdsOpeningRoomRelocMObjSubLightCount, gNdsOpeningRoomRelocMObjSubFirstTextureOffset, gNdsOpeningRoomRelocMObjSubFirstTextureFlags',
        'printf "OPENING_ROOM_FIRST_EVENT=%#x\n", gNdsOpeningRoomFirstEventResult',
        'printf "OPENING_ROOM_FIRST_EVENT_TICK=%u\n", gNdsOpeningRoomFirstEventTick',
        'printf "OPENING_ROOM_FIRST_EVENT_MASK=%#x\n", gNdsOpeningRoomFirstEventProbeMask',
        'printf "OPENING_ROOM_FIRST_EVENT_PENCILS_DOBJ=%u\n", gNdsOpeningRoomFirstEventPencilsDObjOffset',
        'printf "OPENING_ROOM_FIRST_EVENT_PENCILS_ANIM=%u\n", gNdsOpeningRoomFirstEventPencilsAnimOffset',
        'printf "OPENING_ROOM_FIRST_EVENT_DATA=%#x\n", gNdsOpeningRoomFirstEventDataResult',
        'printf "OPENING_ROOM_FIRST_EVENT_DATA_MASK=%#x\n", gNdsOpeningRoomFirstEventDataMask',
        'printf "OPENING_ROOM_FIRST_EVENT_DOBJ_ENTRIES=%u\n", gNdsOpeningRoomFirstEventPencilsDObjEntries',
        'printf "OPENING_ROOM_FIRST_EVENT_DOBJ_DLS=%u\n", gNdsOpeningRoomFirstEventPencilsDLPtrs',
        'printf "OPENING_ROOM_FIRST_EVENT_ANIM_JOINTS=%u\n", gNdsOpeningRoomFirstEventPencilsAnimJoints',
        'printf "OPENING_ROOM_FIRST_EVENT_ANIM_OPCODE=%u\n", gNdsOpeningRoomFirstEventPencilsAnimFirstOpcode',
        'printf "OPENING_ROOM_FIRST_EVENT_RUN=%#x\n", gNdsOpeningRoomFirstEventRunResult',
        'printf "OPENING_ROOM_FIRST_EVENT_DEFERRED=%#x\n", gNdsOpeningRoomFirstEventDeferredMask',
        'printf "OPENING_ROOM_FIGHTER_DEFER=%#x\n", gNdsOpeningRoomFighterDeferredResult',
        'printf "OPENING_ROOM_FIGHTER_DEFER_KIND=%u\n", gNdsOpeningRoomFighterDeferredKind',
        'printf "OPENING_ROOM_TICK380_DEFER=%#x\n", gNdsOpeningRoomTick380DeferredResult',
        'printf "OPENING_ROOM_TICK380_DEFERRED=%#x\n", gNdsOpeningRoomTick380DeferredMask',
        'printf "OPENING_ROOM_TICK450_RUN=%#x\n", gNdsOpeningRoomTick450RunResult',
        'printf "OPENING_ROOM_TICK450_DEFERRED=%#x\n", gNdsOpeningRoomTick450DeferredMask',
        'printf "OPENING_ROOM_OUTSIDE_ASSET=%#x\n", gNdsOpeningRoomOutsideAssetMask',
        'printf "OPENING_ROOM_OUTSIDE_DL_OFFSET=%u\n", gNdsOpeningRoomOutsideDisplayListOffset',
        'printf "OPENING_ROOM_OUTSIDE_CREATE=%#x\n", gNdsOpeningRoomOutsideCreateResult',
        'printf "OPENING_ROOM_OUTSIDE_CREATE_MASK=%#x\n", gNdsOpeningRoomOutsideCreateMask',
        'printf "OPENING_ROOM_OUTSIDE_CREATE_GOBJS=%u\n", gNdsOpeningRoomOutsideCreateGObjCount',
        'printf "OPENING_ROOM_OUTSIDE_GOBJ_DELTA=%u\n", gNdsOpeningRoomOutsideGObjDelta',
        'printf "OPENING_ROOM_OUTSIDE_DOBJ_DELTA=%u\n", gNdsOpeningRoomOutsideDObjDelta',
        'printf "OPENING_ROOM_OUTSIDE_XOBJ_DELTA=%u\n", gNdsOpeningRoomOutsideXObjDelta',
        'printf "OPENING_ROOM_OUTSIDE_DISPLAY=%u\n", gNdsOpeningRoomOutsideDisplaySet',
        'printf "OPENING_ROOM_HAZE_ASSET=%#x\n", gNdsOpeningRoomHazeAssetMask',
        'printf "OPENING_ROOM_HAZE_DL_OFFSET=%u\n", gNdsOpeningRoomHazeDisplayListOffset',
        'printf "OPENING_ROOM_HAZE_CREATE=%#x\n", gNdsOpeningRoomHazeCreateResult',
        'printf "OPENING_ROOM_HAZE_CREATE_MASK=%#x\n", gNdsOpeningRoomHazeCreateMask',
        'printf "OPENING_ROOM_HAZE_CREATE_GOBJS=%u\n", gNdsOpeningRoomHazeCreateGObjCount',
        'printf "OPENING_ROOM_HAZE_GOBJ_DELTA=%u\n", gNdsOpeningRoomHazeGObjDelta',
        'printf "OPENING_ROOM_HAZE_DOBJ_DELTA=%u\n", gNdsOpeningRoomHazeDObjDelta',
        'printf "OPENING_ROOM_HAZE_XOBJ_DELTA=%u\n", gNdsOpeningRoomHazeXObjDelta',
        'printf "OPENING_ROOM_HAZE_DISPLAY=%u\n", gNdsOpeningRoomHazeDisplaySet',
        'printf "OPENING_ROOM_SUNLIGHT_ASSET=%#x\n", gNdsOpeningRoomSunlightAssetMask',
        'printf "OPENING_ROOM_SUNLIGHT_DL_OFFSET=%u\n", gNdsOpeningRoomSunlightDisplayListOffset',
        'printf "OPENING_ROOM_SUNLIGHT_CREATE=%#x\n", gNdsOpeningRoomSunlightCreateResult',
        'printf "OPENING_ROOM_SUNLIGHT_CREATE_MASK=%#x\n", gNdsOpeningRoomSunlightCreateMask',
        'printf "OPENING_ROOM_SUNLIGHT_CREATE_GOBJS=%u\n", gNdsOpeningRoomSunlightCreateGObjCount',
        'printf "OPENING_ROOM_SUNLIGHT_GOBJ_DELTA=%u\n", gNdsOpeningRoomSunlightGObjDelta',
        'printf "OPENING_ROOM_SUNLIGHT_DOBJ_DELTA=%u\n", gNdsOpeningRoomSunlightDObjDelta',
        'printf "OPENING_ROOM_SUNLIGHT_XOBJ_DELTA=%u\n", gNdsOpeningRoomSunlightXObjDelta',
        'printf "OPENING_ROOM_SUNLIGHT_DISPLAY=%u\n", gNdsOpeningRoomSunlightDisplaySet',
        'printf "OPENING_ROOM_DESK_ASSET=%#x\n", gNdsOpeningRoomDeskAssetMask',
        'printf "OPENING_ROOM_DESK_DOBJ_OFFSET=%u\n", gNdsOpeningRoomDeskDObjOffset',
        'printf "OPENING_ROOM_DESK_CREATE=%#x\n", gNdsOpeningRoomDeskCreateResult',
        'printf "OPENING_ROOM_DESK_CREATE_MASK=%#x\n", gNdsOpeningRoomDeskCreateMask',
        'printf "OPENING_ROOM_DESK_CREATE_GOBJS=%u\n", gNdsOpeningRoomDeskCreateGObjCount',
        'printf "OPENING_ROOM_DESK_GOBJ_DELTA=%u\n", gNdsOpeningRoomDeskGObjDelta',
        'printf "OPENING_ROOM_DESK_DOBJ_DELTA=%u\n", gNdsOpeningRoomDeskDObjDelta',
        'printf "OPENING_ROOM_DESK_XOBJ_DELTA=%u\n", gNdsOpeningRoomDeskXObjDelta',
        'printf "OPENING_ROOM_DESK_DISPLAY=%u\n", gNdsOpeningRoomDeskDisplaySet',
        'printf "OPENING_ROOM_SUNLIGHT_EJECT=%#x\n", gNdsOpeningRoomSunlightEjectResult',
        'printf "OPENING_ROOM_SUNLIGHT_EJECT_BEFORE=%u\n", gNdsOpeningRoomSunlightEjectBeforeGObjCount',
        'printf "OPENING_ROOM_SUNLIGHT_EJECT_AFTER=%u\n", gNdsOpeningRoomSunlightEjectAfterGObjCount',
        'printf "OPENING_ROOM_SUNLIGHT_EJECT_UNLINKED=%#x\n", gNdsOpeningRoomSunlightEjectUnlinkedMask',
        'printf "OPENING_ROOM_CLOSEUP_OVERLAY_CREATE=%#x\n", gNdsOpeningRoomCloseUpOverlayCreateResult',
        'printf "OPENING_ROOM_CLOSEUP_OVERLAY_CREATE_MASK=%#x\n", gNdsOpeningRoomCloseUpOverlayCreateMask',
        'printf "OPENING_ROOM_CLOSEUP_OVERLAY_CREATE_TICK=%u\n", gNdsOpeningRoomCloseUpOverlayCreateTick',
        'printf "OPENING_ROOM_CLOSEUP_OVERLAY_CREATE_GOBJS=%u\n", gNdsOpeningRoomCloseUpOverlayCreateGObjCount',
        'printf "OPENING_ROOM_CLOSEUP_OVERLAY_GOBJ_DELTA=%u\n", gNdsOpeningRoomCloseUpOverlayGObjDelta',
        'printf "OPENING_ROOM_CLOSEUP_OVERLAY_DISPLAY=%u\n", gNdsOpeningRoomCloseUpOverlayDisplaySet',
        'printf "OPENING_ROOM_CLOSEUP_OVERLAY_ALPHA=%u\n", gNdsOpeningRoomCloseUpOverlayAlphaInit',
        'printf "OPENING_ROOM_TICK500_RUN=%#x\n", gNdsOpeningRoomTick500RunResult',
        'printf "OPENING_ROOM_TICK500_DEFERRED=%#x\n", gNdsOpeningRoomTick500DeferredMask',
        'printf "OPENING_ROOM_TICK560_RUN=%#x\n", gNdsOpeningRoomTick560RunResult',
        'printf "OPENING_ROOM_TICK560_DEFERRED=%#x\n", gNdsOpeningRoomTick560DeferredMask',
        'printf "OPENING_ROOM_DRAW=%#x\n", gNdsOpeningRoomDrawResult',
        'printf "OPENING_ROOM_DRAW_BLOCKER=%u\n", gNdsOpeningRoomDrawBlocker',
        'printf "OPENING_ROOM_DRAW_TICK=%u\n", gNdsOpeningRoomDrawTickCount',
        'printf "OPENING_ROOM_DRAW_FRAME=%u\n", gNdsOpeningRoomDrawFrameCount',
        'printf "OPENING_ROOM_DRAW_PROBES=%u,%u\n", gNdsOpeningRoomDrawProbeCount, gNdsOpeningRoomDrawReuseCount',
        'printf "OPENING_ROOM_DRAW_CAMERAS=%u\n", gNdsOpeningRoomDrawCameraCallbackCount',
        'printf "OPENING_ROOM_DRAW_DISPLAYS=%u\n", gNdsOpeningRoomDrawDisplayCallbackCount',
        'printf "OPENING_ROOM_DRAW_DOBJS=%u\n", gNdsOpeningRoomDrawDObjCallbackCount',
        'printf "OPENING_ROOM_DRAW_CAMERA_MASK=%#x\n", gNdsOpeningRoomDrawFirstCameraMaskLow',
        'printf "OPENING_ROOM_DRAW_CAMERA_PRIORITY=%u\n", gNdsOpeningRoomDrawFirstCameraPriority',
        'printf "OPENING_ROOM_DRAW_CAMERA_FLAGS=%#x\n", gNdsOpeningRoomDrawFirstCameraFlags',
        'printf "OPENING_ROOM_DRAW_CAMERA_XOBJS=%u,%u,%u\n", gNdsOpeningRoomDrawFirstCameraXObjCount, gNdsOpeningRoomDrawFirstCameraXObjKind0, gNdsOpeningRoomDrawFirstCameraXObjKind1',
        'printf "OPENING_ROOM_DRAW_CAMERA_VIEWPORT=%d,%d,%d,%d\n", gNdsOpeningRoomDrawFirstCameraViewportScaleX, gNdsOpeningRoomDrawFirstCameraViewportScaleY, gNdsOpeningRoomDrawFirstCameraViewportTransX, gNdsOpeningRoomDrawFirstCameraViewportTransY',
        'printf "RDP_DEFAULT_VIEWPORT=%u,%d,%d,%d,%d,%d,%d\n", gNdsRdpDefaultViewportSetCount, gNdsRdpDefaultViewportScaleX, gNdsRdpDefaultViewportScaleY, gNdsRdpDefaultViewportTransX, gNdsRdpDefaultViewportTransY, gNdsRdpDefaultViewportScaleZ, gNdsRdpDefaultViewportTransZ',
        'printf "OPENING_ROOM_DRAW_CAMERA_PERSP=%d,%d,%d\n", gNdsOpeningRoomDrawFirstCameraNear100, gNdsOpeningRoomDrawFirstCameraFar100, gNdsOpeningRoomDrawFirstCameraFovY100',
        'printf "OPENING_ROOM_DRAW_CAMERA_EYE=%d,%d,%d\n", gNdsOpeningRoomDrawFirstCameraEyeX100, gNdsOpeningRoomDrawFirstCameraEyeY100, gNdsOpeningRoomDrawFirstCameraEyeZ100',
        'printf "OPENING_ROOM_DRAW_CAMERA_AT=%d,%d,%d\n", gNdsOpeningRoomDrawFirstCameraAtX100, gNdsOpeningRoomDrawFirstCameraAtY100, gNdsOpeningRoomDrawFirstCameraAtZ100',
        'printf "OPENING_ROOM_DRAW_OBJECT_LINK=%u\n", gNdsOpeningRoomDrawFirstObjectDLLink',
        'printf "OPENING_ROOM_DRAW_OBJECT_ID=%u\n", gNdsOpeningRoomDrawFirstObjectID',
        'printf "OPENING_ROOM_DRAW_OBJECT_KIND=%u\n", gNdsOpeningRoomDrawFirstObjectKind',
        'printf "OPENING_ROOM_DRAW_CALLBACK=%#x\n", gNdsOpeningRoomDrawFirstCallback',
        'printf "OPENING_ROOM_DRAW_DOBJ_DL=%#x\n", gNdsOpeningRoomDrawFirstDObjDL',
        'printf "OPENING_ROOM_DRAW_DOBJ_META=%#x\n", gNdsOpeningRoomDrawFirstDObjMeta',
        'printf "OPENING_ROOM_DRAW_MATERIAL_CANDIDATE=%#x,%u,%#x,%u\n", gNdsOpeningRoomDrawMaterialCandidateResult, gNdsOpeningRoomDrawMaterialCandidateCount, gNdsOpeningRoomDrawMaterialCandidateCameraMaskLow, gNdsOpeningRoomDrawMaterialCandidateCameraPriority',
        'printf "OPENING_ROOM_DRAW_MATERIAL_OBJECT=%u,%u,%u,%#x\n", gNdsOpeningRoomDrawMaterialCandidateObjectDLLink, gNdsOpeningRoomDrawMaterialCandidateObjectID, gNdsOpeningRoomDrawMaterialCandidateObjectKind, gNdsOpeningRoomDrawMaterialCandidateCallback',
        'printf "OPENING_ROOM_DRAW_MATERIAL_DOBJ=%#x,%#x\n", gNdsOpeningRoomDrawMaterialCandidateDObjDL, gNdsOpeningRoomDrawMaterialCandidateDObjMeta',
        'printf "OPENING_ROOM_DRAW_MATERIAL_MOBJ=%u,%#x,%#x,%#x\n", gNdsOpeningRoomDrawMaterialCandidateMObjCount, gNdsOpeningRoomDrawMaterialCandidateMObjFlags, gNdsOpeningRoomDrawMaterialCandidateMObjEffectiveFlags, gNdsOpeningRoomDrawMaterialCandidateMObjMask',
        'printf "OPENING_ROOM_DRAW_MATERIAL_MOBJ_IDS=%u,%u,%u,%d\n", gNdsOpeningRoomDrawMaterialCandidateMObjTextureCurr, gNdsOpeningRoomDrawMaterialCandidateMObjTextureNext, gNdsOpeningRoomDrawMaterialCandidateMObjPaletteIndex, gNdsOpeningRoomDrawMaterialCandidateMObjLfrac100',
        'printf "OPENING_ROOM_DRAW_MATERIAL_MOBJ_FORMAT=%u,%u,%u,%u\n", gNdsOpeningRoomDrawMaterialCandidateMObjFormat, gNdsOpeningRoomDrawMaterialCandidateMObjSize, gNdsOpeningRoomDrawMaterialCandidateMObjBlockFormat, gNdsOpeningRoomDrawMaterialCandidateMObjBlockSize',
        'printf "OPENING_ROOM_DRAW_MATERIAL_MOBJ_TILE=%u,%u,%u,%u\n", gNdsOpeningRoomDrawMaterialCandidateMObjTileWidth, gNdsOpeningRoomDrawMaterialCandidateMObjTileHeight, gNdsOpeningRoomDrawMaterialCandidateMObjScrollWidth, gNdsOpeningRoomDrawMaterialCandidateMObjScrollHeight',
        'printf "OPENING_ROOM_DRAW_MATERIAL_MOBJ_ST=%d,%d,%d,%d\n", gNdsOpeningRoomDrawMaterialCandidateMObjScaleS100, gNdsOpeningRoomDrawMaterialCandidateMObjScaleT100, gNdsOpeningRoomDrawMaterialCandidateMObjTranslateS100, gNdsOpeningRoomDrawMaterialCandidateMObjTranslateT100',
        'printf "OPENING_ROOM_DRAW_MATERIAL_MOBJ_ARRAYS=%#x,%#x\n", gNdsOpeningRoomDrawMaterialCandidateMObjSpriteArray, gNdsOpeningRoomDrawMaterialCandidateMObjPaletteArray',
        'printf "OPENING_ROOM_DRAW_MATERIAL_MOBJ_PTRS=%#x,%#x,%#x\n", gNdsOpeningRoomDrawMaterialCandidateMObjSpriteCurr, gNdsOpeningRoomDrawMaterialCandidateMObjSpriteNext, gNdsOpeningRoomDrawMaterialCandidateMObjPalettePtr',
        'printf "OPENING_ROOM_DRAW_TEXTURE_MATERIAL=%#x,%u,%u,%u,%u,%u\n", gNdsOpeningRoomDrawTextureMaterialResult, gNdsOpeningRoomDrawTextureMaterialCandidateCount, gNdsOpeningRoomDrawTextureMaterialMObjCount, gNdsOpeningRoomDrawTextureMaterialSpriteArrayCount, gNdsOpeningRoomDrawTextureMaterialSpriteCurrCount, gNdsOpeningRoomDrawTextureMaterialSpriteNextCount',
        'printf "OPENING_ROOM_DRAW_TEXTURE_MATERIAL_OBJECT=%u,%u,%u,%#x\n", gNdsOpeningRoomDrawTextureMaterialObjectDLLink, gNdsOpeningRoomDrawTextureMaterialObjectID, gNdsOpeningRoomDrawTextureMaterialObjectKind, gNdsOpeningRoomDrawTextureMaterialCallback',
        'printf "OPENING_ROOM_DRAW_TEXTURE_MATERIAL_DOBJ=%#x,%#x\n", gNdsOpeningRoomDrawTextureMaterialDObjDL, gNdsOpeningRoomDrawTextureMaterialDObjMeta',
        'printf "OPENING_ROOM_DRAW_TEXTURE_MATERIAL_MOBJ=%#x,%#x,%#x,%u,%u\n", gNdsOpeningRoomDrawTextureMaterialMObjFlags, gNdsOpeningRoomDrawTextureMaterialMObjEffectiveFlags, gNdsOpeningRoomDrawTextureMaterialMObjMask, gNdsOpeningRoomDrawTextureMaterialMObjTextureCurr, gNdsOpeningRoomDrawTextureMaterialMObjTextureNext',
        'printf "OPENING_ROOM_DRAW_TEXTURE_MATERIAL_PTRS=%#x,%#x,%#x\n", gNdsOpeningRoomDrawTextureMaterialMObjSpriteArray, gNdsOpeningRoomDrawTextureMaterialMObjSpriteCurr, gNdsOpeningRoomDrawTextureMaterialMObjSpriteNext',
        'printf "OPENING_ROOM_DRAW_MATERIAL_BRANCH=%#x,%u,%u,%u,%u\n", gNdsOpeningRoomDrawMaterialBranchResult, gNdsOpeningRoomDrawMaterialBranchMObjCount, gNdsOpeningRoomDrawMaterialBranchSegmentCommands, gNdsOpeningRoomDrawMaterialBranchTableCommands, gNdsOpeningRoomDrawMaterialBranchGeneratedCommands',
        'printf "OPENING_ROOM_DRAW_MATERIAL_BRANCH_FIRST=%#x,%u,%u,%u\n", gNdsOpeningRoomDrawMaterialBranchFirstMask, gNdsOpeningRoomDrawMaterialBranchFirstGeneratedCommands, gNdsOpeningRoomDrawMaterialBranchFirstTextureScaleS, gNdsOpeningRoomDrawMaterialBranchFirstTextureScaleT',
        'printf "OPENING_ROOM_DRAW_MATERIAL_BRANCH_TILE=%d,%d,%d,%d\n", gNdsOpeningRoomDrawMaterialBranchFirstTileUls, gNdsOpeningRoomDrawMaterialBranchFirstTileUlt, gNdsOpeningRoomDrawMaterialBranchFirstTileLrs, gNdsOpeningRoomDrawMaterialBranchFirstTileLrt',
        'printf "OPENING_ROOM_DRAW_MATERIAL_BRANCH_SCROLL=%d,%d,%d,%d\n", gNdsOpeningRoomDrawMaterialBranchFirstScrollUls, gNdsOpeningRoomDrawMaterialBranchFirstScrollUlt, gNdsOpeningRoomDrawMaterialBranchFirstScrollLrs, gNdsOpeningRoomDrawMaterialBranchFirstScrollLrt',
        'printf "OPENING_ROOM_DRAW_MATERIAL_BRANCH_LOAD=%u,%u\n", gNdsOpeningRoomDrawMaterialBranchFirstLoadBlockTexels, gNdsOpeningRoomDrawMaterialBranchFirstLoadBlockDxt',
        'printf "OPENING_ROOM_DRAW_MATERIAL_EMIT=%#x,%u,%#x,%u,%u,%u\n", gNdsOpeningRoomDrawMaterialEmitResult, gNdsOpeningRoomDrawMaterialEmitBlocker, gNdsOpeningRoomDrawMaterialEmitUnsupportedMask, gNdsOpeningRoomDrawMaterialEmitMObjCount, gNdsOpeningRoomDrawMaterialEmitTableCommands, gNdsOpeningRoomDrawMaterialEmitGeneratedCommands',
        'printf "OPENING_ROOM_DRAW_MATERIAL_EMIT_HEAP=%#x,%#x,%#x,%u\n", gNdsOpeningRoomDrawMaterialEmitHeapStart, gNdsOpeningRoomDrawMaterialEmitBranchStart, gNdsOpeningRoomDrawMaterialEmitHeapAfter, gNdsOpeningRoomDrawMaterialEmitHeapBytes',
        'printf "OPENING_ROOM_DRAW_MATERIAL_EMIT_OPS=%u,%u,%u,%u\n", gNdsOpeningRoomDrawMaterialEmitFirstTableOp, gNdsOpeningRoomDrawMaterialEmitFirstBranchOp0, gNdsOpeningRoomDrawMaterialEmitFirstBranchOp1, gNdsOpeningRoomDrawMaterialEmitFirstBranchOp2',
        'printf "OPENING_ROOM_DRAW_MATERIAL_EMIT_W0=%#x,%#x,%#x\n", gNdsOpeningRoomDrawMaterialEmitFirstBranchW0_0, gNdsOpeningRoomDrawMaterialEmitFirstBranchW0_1, gNdsOpeningRoomDrawMaterialEmitFirstBranchW0_2',
        'printf "OPENING_ROOM_DRAW_MATERIAL_EMIT_W1=%#x,%#x,%#x\n", gNdsOpeningRoomDrawMaterialEmitFirstBranchW1_0, gNdsOpeningRoomDrawMaterialEmitFirstBranchW1_1, gNdsOpeningRoomDrawMaterialEmitFirstBranchW1_2',
        'printf "OPENING_ROOM_DL_PREVIEW=%#x\n", gNdsOpeningRoomDLPreviewResult',
        'printf "OPENING_ROOM_DL_PREVIEW_BLOCKER=%u\n", gNdsOpeningRoomDLPreviewBlocker',
        'printf "OPENING_ROOM_DL_PREVIEW_COMMANDS=%u\n", gNdsOpeningRoomDLPreviewCommandCount',
        'printf "OPENING_ROOM_DL_PREVIEW_VERTICES=%u\n", gNdsOpeningRoomDLPreviewVertexCount',
        'printf "OPENING_ROOM_DL_PREVIEW_TRIANGLES=%u\n", gNdsOpeningRoomDLPreviewTriangleCount',
        'printf "OPENING_ROOM_DL_PREVIEW_PIXELS=%u\n", gNdsOpeningRoomDLPreviewPixelCount',
        'printf "OPENING_ROOM_DL_PREVIEW_FIRST_OPCODE=%#x\n", gNdsOpeningRoomDLPreviewFirstOpcode',
        'printf "OPENING_ROOM_DL_PREVIEW_UNSUPPORTED=%#x\n", gNdsOpeningRoomDLPreviewUnsupportedOpcode',
        'printf "OPENING_ROOM_DL_PREVIEW_CMD_SHAPE=%u,%u,%u,%u,%u,%u,%u\n", gNdsOpeningRoomDLPreviewVertexCommandCount, gNdsOpeningRoomDLPreviewTriangleCommandCount, gNdsOpeningRoomDLPreviewSyncCommandCount, gNdsOpeningRoomDLPreviewEndCommandCount, gNdsOpeningRoomDLPreviewBranchCommandCount, gNdsOpeningRoomDLPreviewOtherModeCommandCount, gNdsOpeningRoomDLPreviewUnsupportedCommandCount',
        'printf "OPENING_ROOM_DL_PREVIEW_BRANCH=%u,%u,%u,%u,%#x\n", gNdsOpeningRoomDLPreviewBranchCallCount, gNdsOpeningRoomDLPreviewBranchJumpCount, gNdsOpeningRoomDLPreviewSegmentResolveCount, gNdsOpeningRoomDLPreviewColorCommandCount, gNdsOpeningRoomDLPreviewPrimColor',
        'printf "OPENING_ROOM_DL_PREVIEW_FIRST_DL=%#x\n", gNdsOpeningRoomDLPreviewFirstDL',
        'printf "OPENING_ROOM_DL_PREVIEW_TRANSFORM=%#x\n", gNdsOpeningRoomDLPreviewTransformMask',
        'printf "OPENING_ROOM_DL_PREVIEW_XOBJS=%u\n", gNdsOpeningRoomDLPreviewXObjCount',
        'printf "OPENING_ROOM_DL_PREVIEW_XOBJ_KIND=%u\n", gNdsOpeningRoomDLPreviewFirstXObjKind',
        'printf "OPENING_ROOM_DL_PREVIEW_TRANSLATE=%d,%d,%d\n", gNdsOpeningRoomDLPreviewTranslateX100, gNdsOpeningRoomDLPreviewTranslateY100, gNdsOpeningRoomDLPreviewTranslateZ100',
        'printf "OPENING_ROOM_DL_PREVIEW_ROTATE=%d,%d,%d\n", gNdsOpeningRoomDLPreviewRotateX100, gNdsOpeningRoomDLPreviewRotateY100, gNdsOpeningRoomDLPreviewRotateZ100',
        'printf "OPENING_ROOM_DL_PREVIEW_SCALE=%d,%d,%d\n", gNdsOpeningRoomDLPreviewScaleX100, gNdsOpeningRoomDLPreviewScaleY100, gNdsOpeningRoomDLPreviewScaleZ100',
        'printf "OPENING_ROOM_DL_PREVIEW_BOUNDS=%d,%d,%d,%d\n", gNdsOpeningRoomDLPreviewMinX, gNdsOpeningRoomDLPreviewMaxX, gNdsOpeningRoomDLPreviewMinY, gNdsOpeningRoomDLPreviewMaxY',
        'printf "OPENING_ROOM_DL_PREVIEW_PROJECTION=%#x,%u,%u\n", gNdsOpeningRoomDLPreviewProjectionMask, gNdsOpeningRoomDLPreviewProjectionMode, gNdsOpeningRoomDLPreviewProjectionBlocker',
        'printf "OPENING_ROOM_DL_PREVIEW_PROJECTED=%u,%u\n", gNdsOpeningRoomDLPreviewProjectedVertexCount, gNdsOpeningRoomDLPreviewProjectedTriangleCount',
        'printf "OPENING_ROOM_DL_PREVIEW_PROJECTED_BOUNDS=%d,%d,%d,%d\n", gNdsOpeningRoomDLPreviewProjectedMinX, gNdsOpeningRoomDLPreviewProjectedMaxX, gNdsOpeningRoomDLPreviewProjectedMinY, gNdsOpeningRoomDLPreviewProjectedMaxY',
        'printf "OPENING_ROOM_DL_PREVIEW_PROJECTED_DEPTH=%d,%d\n", gNdsOpeningRoomDLPreviewProjectedMinDepth100, gNdsOpeningRoomDLPreviewProjectedMaxDepth100',
        'printf "OPENING_ROOM_DL_PREVIEW_GEOMETRY=%u,%#x,%#x,%#x,%#x\n", gNdsOpeningRoomDLPreviewGeometryCommandCount, gNdsOpeningRoomDLPreviewGeometryClearMask, gNdsOpeningRoomDLPreviewGeometrySetMask, gNdsOpeningRoomDLPreviewGeometryFinalMode, gNdsOpeningRoomDLPreviewGeometryFlags',
        'printf "OPENING_ROOM_DL_PREVIEW_GEOMETRY_TRIS=%u,%u,%u,%u\n", gNdsOpeningRoomDLPreviewGeometryPositiveWinding, gNdsOpeningRoomDLPreviewGeometryNegativeWinding, gNdsOpeningRoomDLPreviewGeometryZeroArea, gNdsOpeningRoomDLPreviewGeometryDrawnTriangles',
        'printf "OPENING_ROOM_DL_PREVIEW_FALLBACK=%u,%u\n", gNdsOpeningRoomDLPreviewFallbackAxis, gNdsOpeningRoomDLPreviewFallbackArea',
        'printf "OPENING_ROOM_DL_PREVIEW_RENDERER=%u,%u,%u,%u\n", gNdsOpeningRoomDLPreviewRendererParsedCommandCount, gNdsOpeningRoomDLPreviewRendererStateCommandCount, gNdsOpeningRoomDLPreviewRendererSkippedCommandCount, gNdsOpeningRoomDLPreviewRendererRenderedCommandCount',
        'printf "OPENING_ROOM_DL_PREVIEW_RENDERER_TEX=%#x,%#x,%u,%u,%u,%u,%u,%u,%#x\n", gNdsOpeningRoomDLPreviewRendererTextureMask, gNdsOpeningRoomDLPreviewRendererTextureImage, gNdsOpeningRoomDLPreviewRendererTextureFormat, gNdsOpeningRoomDLPreviewRendererTextureSize, gNdsOpeningRoomDLPreviewRendererTextureImageWidth, gNdsOpeningRoomDLPreviewRendererTextureLoadTexels, gNdsOpeningRoomDLPreviewRendererTextureSetTileCount, gNdsOpeningRoomDLPreviewRendererTextureCommandCount, gNdsOpeningRoomDLPreviewRendererTextureStateFlags',
        'printf "OPENING_ROOM_DL_PREVIEW_RENDERER_TILE=%u,%u,%u,%u,%#x,%u,%u\n", gNdsOpeningRoomDLPreviewRendererTextureTileWidth, gNdsOpeningRoomDLPreviewRendererTextureTileHeight, gNdsOpeningRoomDLPreviewRendererTextureRenderTile, gNdsOpeningRoomDLPreviewRendererTextureRenderTileLine, gNdsOpeningRoomDLPreviewRendererTextureRenderTileFlags, gNdsOpeningRoomDLPreviewRendererTextureLoadBlockLrs, gNdsOpeningRoomDLPreviewRendererTextureLoadBlockDxt',
        'printf "OPENING_ROOM_DL_PREVIEW_PRESENT=%u,%u,%u,%u,%u\n", gNdsOriginalDLPreviewReady, gNdsOriginalDLPreviewWidth, gNdsOriginalDLPreviewHeight, gNdsOriginalDLPreviewCommitCount, gNdsOriginalDLPreviewDrawCount',
        'printf "OPENING_ROOM_DL_PREVIEW_TEX_MASK=%#x\n", gNdsOpeningRoomDLPreviewTextureMask',
        'printf "OPENING_ROOM_DL_PREVIEW_TEX_IMAGE=%#x\n", gNdsOpeningRoomDLPreviewTextureImage',
        'printf "OPENING_ROOM_DL_PREVIEW_TEX_FMT=%u\n", gNdsOpeningRoomDLPreviewTextureFormat',
        'printf "OPENING_ROOM_DL_PREVIEW_TEX_SIZE=%u\n", gNdsOpeningRoomDLPreviewTextureSize',
        'printf "OPENING_ROOM_DL_PREVIEW_TEX_IMAGE_W=%u\n", gNdsOpeningRoomDLPreviewTextureImageWidth',
        'printf "OPENING_ROOM_DL_PREVIEW_TEX_TILE=%u,%u\n", gNdsOpeningRoomDLPreviewTextureTileWidth, gNdsOpeningRoomDLPreviewTextureTileHeight',
        'printf "OPENING_ROOM_DL_PREVIEW_TEX_TEXELS=%u,%u\n", gNdsOpeningRoomDLPreviewTextureTexelWidth, gNdsOpeningRoomDLPreviewTextureTexelHeight',
        'printf "OPENING_ROOM_DL_PREVIEW_TEX_READY=%u,%u\n", gNdsOpeningRoomDLPreviewTextureReady, gNdsOpeningRoomDLPreviewTextureSampleMode',
        'printf "OPENING_ROOM_DL_PREVIEW_TEX_LOAD=%u\n", gNdsOpeningRoomDLPreviewTextureLoadTexels',
        'printf "OPENING_ROOM_DL_PREVIEW_TEX_LOADBLOCK=%u,%u,%u,%u,%u\n", gNdsOpeningRoomDLPreviewTextureLoadBlockTile, gNdsOpeningRoomDLPreviewTextureLoadBlockUls, gNdsOpeningRoomDLPreviewTextureLoadBlockUlt, gNdsOpeningRoomDLPreviewTextureLoadBlockLrs, gNdsOpeningRoomDLPreviewTextureLoadBlockDxt',
        'printf "OPENING_ROOM_DL_PREVIEW_TEX_TILESIZE_RAW=%u,%u,%u,%u,%u\n", gNdsOpeningRoomDLPreviewTextureTileSizeTile, gNdsOpeningRoomDLPreviewTextureTileSizeUls, gNdsOpeningRoomDLPreviewTextureTileSizeUlt, gNdsOpeningRoomDLPreviewTextureTileSizeLrs, gNdsOpeningRoomDLPreviewTextureTileSizeLrt',
        'printf "OPENING_ROOM_DL_PREVIEW_TEX_SAMPLES=%u\n", gNdsOpeningRoomDLPreviewTextureSamplePixels',
        'printf "OPENING_ROOM_DL_PREVIEW_TEX_SETTILE=%u\n", gNdsOpeningRoomDLPreviewTextureSetTileCount',
        'printf "OPENING_ROOM_DL_PREVIEW_TEX_COMBINE=%#x,%#x\n", gNdsOpeningRoomDLPreviewTextureCombineW0, gNdsOpeningRoomDLPreviewTextureCombineW1',
        'printf "OPENING_ROOM_DL_PREVIEW_TEX_COMBINE_MODE=%u,%#x,%u\n", gNdsOpeningRoomDLPreviewTextureCombineMode, gNdsOpeningRoomDLPreviewTextureCombineFlags, gNdsOpeningRoomDLPreviewTextureModulatedPixels',
        'printf "OPENING_ROOM_DL_PREVIEW_TEX_RENDERTILE=%u,%u,%u,%u\n", gNdsOpeningRoomDLPreviewTextureRenderTile, gNdsOpeningRoomDLPreviewTextureRenderTileLine, gNdsOpeningRoomDLPreviewTextureRenderTileTmem, gNdsOpeningRoomDLPreviewTextureRenderTilePalette',
        'printf "OPENING_ROOM_DL_PREVIEW_TEX_TILEMODE=%u,%u,%u,%u,%u,%u,%#x\n", gNdsOpeningRoomDLPreviewTextureRenderTileCms, gNdsOpeningRoomDLPreviewTextureRenderTileCmt, gNdsOpeningRoomDLPreviewTextureRenderTileMasks, gNdsOpeningRoomDLPreviewTextureRenderTileMaskt, gNdsOpeningRoomDLPreviewTextureRenderTileShifts, gNdsOpeningRoomDLPreviewTextureRenderTileShiftt, gNdsOpeningRoomDLPreviewTextureRenderTileFlags',
        'printf "OPENING_ROOM_DL_PREVIEW_TEX_TEXTURE=%u,%u,%u,%#x\n", gNdsOpeningRoomDLPreviewTextureCommandCount, gNdsOpeningRoomDLPreviewTextureScaleS, gNdsOpeningRoomDLPreviewTextureScaleT, gNdsOpeningRoomDLPreviewTextureStateFlags',
        'printf "OPENING_ROOM_DL_PREVIEW_TEX_TEXTURE_MODE=%u,%u,%u,%u\n", gNdsOpeningRoomDLPreviewTextureLevel, gNdsOpeningRoomDLPreviewTextureTile, gNdsOpeningRoomDLPreviewTextureOn, gNdsOpeningRoomDLPreviewTextureXParam',
        'printf "OPENING_ROOM_DL_PREVIEW_MATERIAL=%u,%#x,%#x,%#x\n", gNdsOpeningRoomDLPreviewMaterialCount, gNdsOpeningRoomDLPreviewMaterialFlags, gNdsOpeningRoomDLPreviewMaterialEffectiveFlags, gNdsOpeningRoomDLPreviewMaterialMask',
        'printf "OPENING_ROOM_DL_PREVIEW_MATERIAL_IDS=%u,%u,%u,%d\n", gNdsOpeningRoomDLPreviewMaterialTextureCurr, gNdsOpeningRoomDLPreviewMaterialTextureNext, gNdsOpeningRoomDLPreviewMaterialPaletteIndex, gNdsOpeningRoomDLPreviewMaterialLfrac100',
        'printf "OPENING_ROOM_DL_PREVIEW_MATERIAL_FORMAT=%u,%u,%u,%u\n", gNdsOpeningRoomDLPreviewMaterialFormat, gNdsOpeningRoomDLPreviewMaterialSize, gNdsOpeningRoomDLPreviewMaterialBlockFormat, gNdsOpeningRoomDLPreviewMaterialBlockSize',
        'printf "OPENING_ROOM_DL_PREVIEW_MATERIAL_TILE=%u,%u,%u,%u\n", gNdsOpeningRoomDLPreviewMaterialTileWidth, gNdsOpeningRoomDLPreviewMaterialTileHeight, gNdsOpeningRoomDLPreviewMaterialScrollWidth, gNdsOpeningRoomDLPreviewMaterialScrollHeight',
        'printf "OPENING_ROOM_DL_PREVIEW_MATERIAL_ST=%d,%d,%d,%d\n", gNdsOpeningRoomDLPreviewMaterialScaleS100, gNdsOpeningRoomDLPreviewMaterialScaleT100, gNdsOpeningRoomDLPreviewMaterialTranslateS100, gNdsOpeningRoomDLPreviewMaterialTranslateT100',
        'printf "OPENING_ROOM_DL_PREVIEW_MATERIAL_PTRS=%#x,%#x,%#x\n", gNdsOpeningRoomDLPreviewMaterialSpriteCurr, gNdsOpeningRoomDLPreviewMaterialSpriteNext, gNdsOpeningRoomDLPreviewMaterialPalettePtr',
        'printf "OPENING_ROOM_DL_PREVIEW_MATERIAL_BRANCH=%#x,%u,%u,%u,%u\n", gNdsOpeningRoomDLPreviewMaterialBranchResult, gNdsOpeningRoomDLPreviewMaterialBranchBlocker, gNdsOpeningRoomDLPreviewMaterialBranchCommandCount, gNdsOpeningRoomDLPreviewMaterialBranchPrimCount, gNdsOpeningRoomDLPreviewMaterialBranchEndCount',
        'printf "OPENING_ROOM_DL_PREVIEW_MATERIAL_BRANCH_OPS=%u,%u,%#x\n", gNdsOpeningRoomDLPreviewMaterialBranchFirstOp, gNdsOpeningRoomDLPreviewMaterialBranchSecondOp, gNdsOpeningRoomDLPreviewMaterialBranchUnsupportedOp',
        'printf "OPENING_ROOM_DL_PREVIEW_MATERIAL_BRANCH_PRIM=%#x,%u,%u\n", gNdsOpeningRoomDLPreviewMaterialBranchPrimColor, gNdsOpeningRoomDLPreviewMaterialBranchPrimLod, gNdsOpeningRoomDLPreviewMaterialBranchPrimM',
        'printf "OPENING_ROOM_MATERIAL_DL_PROBE=%#x,%u,%#x\n", gNdsOpeningRoomMaterialDLProbeResult, gNdsOpeningRoomMaterialDLProbeBlocker, gNdsOpeningRoomMaterialDLProbeFirstDL',
        'printf "OPENING_ROOM_MATERIAL_DL_PROBE_SHAPE=%u,%u,%u,%u\n", gNdsOpeningRoomMaterialDLProbeCommandCount, gNdsOpeningRoomMaterialDLProbeVertexCount, gNdsOpeningRoomMaterialDLProbeTriangleCount, gNdsOpeningRoomMaterialDLProbeUnsupportedCommandCount',
        'printf "OPENING_ROOM_MATERIAL_DL_PROBE_CMDS=%u,%u,%u,%u,%u,%u\n", gNdsOpeningRoomMaterialDLProbeVertexCommandCount, gNdsOpeningRoomMaterialDLProbeTriangleCommandCount, gNdsOpeningRoomMaterialDLProbeSyncCommandCount, gNdsOpeningRoomMaterialDLProbeEndCommandCount, gNdsOpeningRoomMaterialDLProbeBranchCommandCount, gNdsOpeningRoomMaterialDLProbeOtherModeCommandCount',
        'printf "OPENING_ROOM_MATERIAL_DL_PROBE_OPS=%#x,%#x\n", gNdsOpeningRoomMaterialDLProbeFirstOpcode, gNdsOpeningRoomMaterialDLProbeUnsupportedOpcode',
        'printf "OPENING_ROOM_MATERIAL_DL_EXPAND=%#x,%u,%#x,%#x,%#x\n", gNdsOpeningRoomMaterialDLExpandResult, gNdsOpeningRoomMaterialDLExpandBlocker, gNdsOpeningRoomMaterialDLExpandFirstDL, gNdsOpeningRoomMaterialDLExpandFirstBranchDL, gNdsOpeningRoomMaterialDLExpandFirstResolvedBranchDL',
        'printf "OPENING_ROOM_MATERIAL_DL_EXPAND_SHAPE=%u,%u,%u,%u\n", gNdsOpeningRoomMaterialDLExpandCommandCount, gNdsOpeningRoomMaterialDLExpandVertexCount, gNdsOpeningRoomMaterialDLExpandTriangleCount, gNdsOpeningRoomMaterialDLExpandUnsupportedCommandCount',
        'printf "OPENING_ROOM_MATERIAL_DL_EXPAND_CMDS=%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsOpeningRoomMaterialDLExpandVertexCommandCount, gNdsOpeningRoomMaterialDLExpandTriangleCommandCount, gNdsOpeningRoomMaterialDLExpandSyncCommandCount, gNdsOpeningRoomMaterialDLExpandEndCommandCount, gNdsOpeningRoomMaterialDLExpandBranchCommandCount, gNdsOpeningRoomMaterialDLExpandBranchCallCount, gNdsOpeningRoomMaterialDLExpandBranchJumpCount, gNdsOpeningRoomMaterialDLExpandSegmentResolveCount, gNdsOpeningRoomMaterialDLExpandOtherModeCommandCount',
        'printf "OPENING_ROOM_MATERIAL_DL_EXPAND_OPS=%#x,%#x,%u,%u\n", gNdsOpeningRoomMaterialDLExpandFirstOpcode, gNdsOpeningRoomMaterialDLExpandUnsupportedOpcode, gNdsOpeningRoomMaterialDLExpandColorCommandCount, gNdsOpeningRoomMaterialDLExpandMaxDepth',
        'printf "OPENING_ROOM_SPOTLIGHT_ASSET=%#x\n", gNdsOpeningRoomSpotlightAssetMask',
        'printf "OPENING_ROOM_SPOTLIGHT_DL_OFFSET=%u\n", gNdsOpeningRoomSpotlightDisplayListOffset',
        'printf "OPENING_ROOM_SPOTLIGHT_MOBJ_OFFSET=%u\n", gNdsOpeningRoomSpotlightMObjOffset',
        'printf "OPENING_ROOM_SPOTLIGHT_MATANIM_OFFSET=%u\n", gNdsOpeningRoomSpotlightMatAnimOffset',
        'printf "OPENING_ROOM_SPOTLIGHT_CREATE=%#x\n", gNdsOpeningRoomSpotlightCreateResult',
        'printf "OPENING_ROOM_SPOTLIGHT_CREATE_MASK=%#x\n", gNdsOpeningRoomSpotlightCreateMask',
        'printf "OPENING_ROOM_SPOTLIGHT_CREATE_TICK=%u\n", gNdsOpeningRoomSpotlightCreateTick',
        'printf "OPENING_ROOM_SPOTLIGHT_CREATE_GOBJS=%u\n", gNdsOpeningRoomSpotlightCreateGObjCount',
        'printf "OPENING_ROOM_SPOTLIGHT_GOBJ_DELTA=%u\n", gNdsOpeningRoomSpotlightGObjDelta',
        'printf "OPENING_ROOM_SPOTLIGHT_DOBJ_DELTA=%u\n", gNdsOpeningRoomSpotlightDObjDelta',
        'printf "OPENING_ROOM_SPOTLIGHT_XOBJ_DELTA=%u\n", gNdsOpeningRoomSpotlightXObjDelta',
        'printf "OPENING_ROOM_SPOTLIGHT_MOBJ_DELTA=%u\n", gNdsOpeningRoomSpotlightMObjDelta',
        'printf "OPENING_ROOM_SPOTLIGHT_AOBJ_DELTA=%u\n", gNdsOpeningRoomSpotlightAObjDelta',
        'printf "OPENING_ROOM_SPOTLIGHT_DISPLAY=%u\n", gNdsOpeningRoomSpotlightDisplaySet',
        'printf "OPENING_ROOM_SPOTLIGHT_PROCESS=%u\n", gNdsOpeningRoomSpotlightProcessSet',
        'printf "OPENING_ROOM_SPOTLIGHT_MOBJ=%u\n", gNdsOpeningRoomSpotlightMObjSet',
        'printf "OPENING_ROOM_SPOTLIGHT_MATANIM=%u\n", gNdsOpeningRoomSpotlightMatAnimSet',
        'printf "OPENING_ROOM_SPOTLIGHT_POSITION=%u\n", gNdsOpeningRoomSpotlightPositionSet',
        'printf "OPENING_ROOM_OVERLAY_CREATE=%#x\n", gNdsOpeningRoomOverlayCreateResult',
        'printf "OPENING_ROOM_OVERLAY_DISPLAY=%u\n", gNdsOpeningRoomOverlayDisplaySet',
        'printf "OPENING_ROOM_OVERLAY_ALPHA=%u\n", gNdsOpeningRoomOverlayAlphaInit',
        'printf "OPENING_ROOM_OVERLAY_CREATE_GOBJS=%u\n", gNdsOpeningRoomOverlayCreateGObjCount',
        'printf "OPENING_ROOM_OVERLAY_EJECT=%#x\n", gNdsOpeningRoomOverlayEjectResult',
        'printf "OPENING_ROOM_OVERLAY_EJECT_BEFORE=%u\n", gNdsOpeningRoomOverlayEjectBeforeGObjCount',
        'printf "OPENING_ROOM_OVERLAY_EJECT_AFTER=%u\n", gNdsOpeningRoomOverlayEjectAfterGObjCount',
        'printf "OPENING_ROOM_OVERLAY_EJECT_UNLINKED=%#x\n", gNdsOpeningRoomOverlayEjectUnlinkedMask',
        'printf "OPENING_ROOM_SCENE1_CAMERA_CREATE=%#x\n", gNdsOpeningRoomScene1CameraCreateResult',
        'printf "OPENING_ROOM_SCENE1_CAMERA_CREATE_MASK=%#x\n", gNdsOpeningRoomScene1CameraCreateMask',
        'printf "OPENING_ROOM_SCENE1_CAMERA_CREATE_GOBJS=%u\n", gNdsOpeningRoomScene1CameraCreateGObjCount',
        'printf "OPENING_ROOM_SCENE1_CAMERA_GOBJ_DELTA=%u\n", gNdsOpeningRoomScene1CameraGObjDelta',
        'printf "OPENING_ROOM_SCENE1_CAMERA_COBJ_DELTA=%u\n", gNdsOpeningRoomScene1CameraCObjDelta',
        'printf "OPENING_ROOM_SCENE1_CAMERA_XOBJ_DELTA=%u\n", gNdsOpeningRoomScene1CameraXObjDelta',
        'printf "OPENING_ROOM_SCENE1_CAMERA_AOBJ_DELTA=%u\n", gNdsOpeningRoomScene1CameraAObjDelta',
        'printf "OPENING_ROOM_SCENE1_CAMERA_DISPLAY=%u\n", gNdsOpeningRoomScene1CameraDisplaySet',
        'printf "OPENING_ROOM_SCENE1_CAMERA_PROCESS=%u\n", gNdsOpeningRoomScene1CameraProcessSet',
        'printf "OPENING_ROOM_SCENE1_CAMERA_ANIM=%u\n", gNdsOpeningRoomScene1CameraAnimSet',
        'printf "OPENING_ROOM_SCENE1_CAMERA_VIEWPORT=%u\n", gNdsOpeningRoomScene1CameraViewportSet',
        'printf "OPENING_ROOM_SCENE1_CAMERA_DLBUFFER=%u\n", gNdsOpeningRoomScene1CameraDLBufferSet',
        'printf "OPENING_ROOM_SCENE2_CAMERA_ASSET=%#x\n", gNdsOpeningRoomScene2CameraAssetMask',
        'printf "OPENING_ROOM_SCENE2_CAMERA_ANIM_OFFSET=%u\n", gNdsOpeningRoomScene2CameraAnimOffset',
        'printf "OPENING_ROOM_SCENE2_CAMERA_EJECT=%#x\n", gNdsOpeningRoomScene2CameraEjectResult',
        'printf "OPENING_ROOM_SCENE2_CAMERA_EJECT_MASK=%#x\n", gNdsOpeningRoomScene2CameraEjectMask',
        'printf "OPENING_ROOM_SCENE2_CAMERA_EJECT_BEFORE_GOBJS=%u\n", gNdsOpeningRoomScene2CameraEjectBeforeGObjCount',
        'printf "OPENING_ROOM_SCENE2_CAMERA_EJECT_AFTER_GOBJS=%u\n", gNdsOpeningRoomScene2CameraEjectAfterGObjCount',
        'printf "OPENING_ROOM_SCENE2_CAMERA_EJECT_BEFORE_CAMERAS=%u\n", gNdsOpeningRoomScene2CameraEjectBeforeCameraCount',
        'printf "OPENING_ROOM_SCENE2_CAMERA_EJECT_AFTER_CAMERAS=%u\n", gNdsOpeningRoomScene2CameraEjectAfterCameraCount',
        'printf "OPENING_ROOM_SCENE2_CAMERA_CREATE=%#x\n", gNdsOpeningRoomScene2CameraCreateResult',
        'printf "OPENING_ROOM_SCENE2_CAMERA_CREATE_MASK=%#x\n", gNdsOpeningRoomScene2CameraCreateMask',
        'printf "OPENING_ROOM_SCENE2_CAMERA_CREATE_GOBJS=%u\n", gNdsOpeningRoomScene2CameraCreateGObjCount',
        'printf "OPENING_ROOM_SCENE2_CAMERA_GOBJ_DELTA=%u\n", gNdsOpeningRoomScene2CameraGObjDelta',
        'printf "OPENING_ROOM_SCENE2_CAMERA_COBJ_DELTA=%u\n", gNdsOpeningRoomScene2CameraCObjDelta',
        'printf "OPENING_ROOM_SCENE2_CAMERA_XOBJ_DELTA=%u\n", gNdsOpeningRoomScene2CameraXObjDelta',
        'printf "OPENING_ROOM_SCENE2_CAMERA_AOBJ_DELTA=%u\n", gNdsOpeningRoomScene2CameraAObjDelta',
        'printf "OPENING_ROOM_SCENE2_CAMERA_DISPLAY=%u\n", gNdsOpeningRoomScene2CameraDisplaySet',
        'printf "OPENING_ROOM_SCENE2_CAMERA_PROCESS=%u\n", gNdsOpeningRoomScene2CameraProcessSet',
        'printf "OPENING_ROOM_SCENE2_CAMERA_ANIM=%u\n", gNdsOpeningRoomScene2CameraAnimSet',
        'printf "OPENING_ROOM_SCENE2_CAMERA_VIEWPORT=%u\n", gNdsOpeningRoomScene2CameraViewportSet',
        'printf "OPENING_ROOM_SCENE2_CAMERA_DLBUFFER=%u\n", gNdsOpeningRoomScene2CameraDLBufferSet',
        'printf "OPENING_ROOM_CLOSEUP_CAMERA_CREATE=%#x\n", gNdsOpeningRoomCloseUpOverlayCameraCreateResult',
        'printf "OPENING_ROOM_CLOSEUP_CAMERA_CREATE_MASK=%#x\n", gNdsOpeningRoomCloseUpOverlayCameraCreateMask',
        'printf "OPENING_ROOM_CLOSEUP_CAMERA_CREATE_GOBJS=%u\n", gNdsOpeningRoomCloseUpOverlayCameraCreateGObjCount',
        'printf "OPENING_ROOM_CLOSEUP_CAMERA_GOBJ_DELTA=%u\n", gNdsOpeningRoomCloseUpOverlayCameraGObjDelta',
        'printf "OPENING_ROOM_CLOSEUP_CAMERA_COBJ_DELTA=%u\n", gNdsOpeningRoomCloseUpOverlayCameraCObjDelta',
        'printf "OPENING_ROOM_CLOSEUP_CAMERA_XOBJ_DELTA=%u\n", gNdsOpeningRoomCloseUpOverlayCameraXObjDelta',
        'printf "OPENING_ROOM_CLOSEUP_CAMERA_DISPLAY=%u\n", gNdsOpeningRoomCloseUpOverlayCameraDisplaySet',
        'printf "OPENING_ROOM_CLOSEUP_CAMERA_VIEWPORT=%u\n", gNdsOpeningRoomCloseUpOverlayCameraViewportSet',
        'printf "OPENING_ROOM_WALLPAPER_CAMERA_CREATE=%#x\n", gNdsOpeningRoomWallpaperCameraCreateResult',
        'printf "OPENING_ROOM_WALLPAPER_CAMERA_CREATE_MASK=%#x\n", gNdsOpeningRoomWallpaperCameraCreateMask',
        'printf "OPENING_ROOM_WALLPAPER_CAMERA_CREATE_GOBJS=%u\n", gNdsOpeningRoomWallpaperCameraCreateGObjCount',
        'printf "OPENING_ROOM_WALLPAPER_CAMERA_GOBJ_DELTA=%u\n", gNdsOpeningRoomWallpaperCameraGObjDelta',
        'printf "OPENING_ROOM_WALLPAPER_CAMERA_COBJ_DELTA=%u\n", gNdsOpeningRoomWallpaperCameraCObjDelta',
        'printf "OPENING_ROOM_WALLPAPER_CAMERA_XOBJ_DELTA=%u\n", gNdsOpeningRoomWallpaperCameraXObjDelta',
        'printf "OPENING_ROOM_WALLPAPER_CAMERA_DISPLAY=%u\n", gNdsOpeningRoomWallpaperCameraDisplaySet',
        'printf "OPENING_ROOM_WALLPAPER_CAMERA_VIEWPORT=%u\n", gNdsOpeningRoomWallpaperCameraViewportSet',
        'printf "OPENING_ROOM_LOGO_CAMERA_ASSET=%#x\n", gNdsOpeningRoomLogoCameraAssetMask',
        'printf "OPENING_ROOM_LOGO_CAMERA_ANIM_OFFSET=%u\n", gNdsOpeningRoomLogoCameraAnimOffset',
        'printf "OPENING_ROOM_LOGO_CAMERA_CREATE=%#x\n", gNdsOpeningRoomLogoCameraCreateResult',
        'printf "OPENING_ROOM_LOGO_CAMERA_CREATE_MASK=%#x\n", gNdsOpeningRoomLogoCameraCreateMask',
        'printf "OPENING_ROOM_LOGO_CAMERA_CREATE_GOBJS=%u\n", gNdsOpeningRoomLogoCameraCreateGObjCount',
        'printf "OPENING_ROOM_LOGO_CAMERA_GOBJ_DELTA=%u\n", gNdsOpeningRoomLogoCameraGObjDelta',
        'printf "OPENING_ROOM_LOGO_CAMERA_COBJ_DELTA=%u\n", gNdsOpeningRoomLogoCameraCObjDelta',
        'printf "OPENING_ROOM_LOGO_CAMERA_XOBJ_DELTA=%u\n", gNdsOpeningRoomLogoCameraXObjDelta',
        'printf "OPENING_ROOM_LOGO_CAMERA_AOBJ_DELTA=%u\n", gNdsOpeningRoomLogoCameraAObjDelta',
        'printf "OPENING_ROOM_LOGO_CAMERA_DISPLAY=%u\n", gNdsOpeningRoomLogoCameraDisplaySet',
        'printf "OPENING_ROOM_LOGO_CAMERA_PROCESS=%u\n", gNdsOpeningRoomLogoCameraProcessSet',
        'printf "OPENING_ROOM_LOGO_CAMERA_ANIM=%u\n", gNdsOpeningRoomLogoCameraAnimSet',
        'printf "OPENING_ROOM_LOGO_CAMERA_VIEWPORT=%u\n", gNdsOpeningRoomLogoCameraViewportSet',
        'printf "OPENING_ROOM_LOGO_ASSET=%#x\n", gNdsOpeningRoomLogoAssetMask',
        'printf "OPENING_ROOM_LOGO_DOBJ_OFFSET=%u\n", gNdsOpeningRoomLogoDObjOffset',
        'printf "OPENING_ROOM_LOGO_MOBJ_OFFSET=%u\n", gNdsOpeningRoomLogoMObjOffset',
        'printf "OPENING_ROOM_LOGO_MATANIM_OFFSET=%u\n", gNdsOpeningRoomLogoMatAnimOffset',
        'printf "OPENING_ROOM_LOGO_CREATE=%#x\n", gNdsOpeningRoomLogoCreateResult',
        'printf "OPENING_ROOM_LOGO_CREATE_MASK=%#x\n", gNdsOpeningRoomLogoCreateMask',
        'printf "OPENING_ROOM_LOGO_CREATE_GOBJS=%u\n", gNdsOpeningRoomLogoCreateGObjCount',
        'printf "OPENING_ROOM_LOGO_GOBJ_DELTA=%u\n", gNdsOpeningRoomLogoGObjDelta',
        'printf "OPENING_ROOM_LOGO_DOBJ_DELTA=%u\n", gNdsOpeningRoomLogoDObjDelta',
        'printf "OPENING_ROOM_LOGO_XOBJ_DELTA=%u\n", gNdsOpeningRoomLogoXObjDelta',
        'printf "OPENING_ROOM_LOGO_MOBJ_DELTA=%u\n", gNdsOpeningRoomLogoMObjDelta',
        'printf "OPENING_ROOM_LOGO_AOBJ_DELTA=%u\n", gNdsOpeningRoomLogoAObjDelta',
        'printf "OPENING_ROOM_LOGO_DISPLAY=%u\n", gNdsOpeningRoomLogoDisplaySet',
        'printf "OPENING_ROOM_LOGO_MOBJ=%u\n", gNdsOpeningRoomLogoMObjSet',
        'printf "OPENING_ROOM_LOGO_MATANIM=%u\n", gNdsOpeningRoomLogoMatAnimSet',
        'printf "OPENING_ROOM_LOGO_EJECT=%#x\n", gNdsOpeningRoomLogoEjectResult',
        'printf "OPENING_ROOM_LOGO_EJECT_BEFORE=%u\n", gNdsOpeningRoomLogoEjectBeforeGObjCount',
        'printf "OPENING_ROOM_LOGO_EJECT_AFTER=%u\n", gNdsOpeningRoomLogoEjectAfterGObjCount',
        'printf "OPENING_ROOM_LOGO_EJECT_UNLINKED=%#x\n", gNdsOpeningRoomLogoEjectUnlinkedMask',
        'printf "OPENING_ROOM_BOSS_SHADOW_ASSET=%#x\n", gNdsOpeningRoomBossShadowAssetMask',
        'printf "OPENING_ROOM_BOSS_SHADOW_DL_OFFSET=%u\n", gNdsOpeningRoomBossShadowDisplayListOffset',
        'printf "OPENING_ROOM_BOSS_SHADOW_ANIM_OFFSET=%u\n", gNdsOpeningRoomBossShadowAnimOffset',
        'printf "OPENING_ROOM_BOSS_SHADOW_CREATE=%#x\n", gNdsOpeningRoomBossShadowCreateResult',
        'printf "OPENING_ROOM_BOSS_SHADOW_CREATE_MASK=%#x\n", gNdsOpeningRoomBossShadowCreateMask',
        'printf "OPENING_ROOM_BOSS_SHADOW_CREATE_GOBJS=%u\n", gNdsOpeningRoomBossShadowCreateGObjCount',
        'printf "OPENING_ROOM_BOSS_SHADOW_GOBJ_DELTA=%u\n", gNdsOpeningRoomBossShadowGObjDelta',
        'printf "OPENING_ROOM_BOSS_SHADOW_DOBJ_DELTA=%u\n", gNdsOpeningRoomBossShadowDObjDelta',
        'printf "OPENING_ROOM_BOSS_SHADOW_XOBJ_DELTA=%u\n", gNdsOpeningRoomBossShadowXObjDelta',
        'printf "OPENING_ROOM_BOSS_SHADOW_AOBJ_DELTA=%u\n", gNdsOpeningRoomBossShadowAObjDelta',
        'printf "OPENING_ROOM_BOSS_SHADOW_PROCESS=%u\n", gNdsOpeningRoomBossShadowProcessSet',
        'printf "OPENING_ROOM_BOSS_SHADOW_DISPLAY=%u\n", gNdsOpeningRoomBossShadowDisplaySet',
        'printf "OPENING_ROOM_BOSS_SHADOW_ANIM=%u\n", gNdsOpeningRoomBossShadowAnimSet',
        'printf "OPENING_ROOM_BOSS_SHADOW_EJECT=%#x\n", gNdsOpeningRoomBossShadowEjectResult',
        'printf "OPENING_ROOM_BOSS_SHADOW_EJECT_BEFORE=%u\n", gNdsOpeningRoomBossShadowEjectBeforeGObjCount',
        'printf "OPENING_ROOM_BOSS_SHADOW_EJECT_AFTER=%u\n", gNdsOpeningRoomBossShadowEjectAfterGObjCount',
        'printf "OPENING_ROOM_BOSS_SHADOW_EJECT_UNLINKED=%#x\n", gNdsOpeningRoomBossShadowEjectUnlinkedMask',
        'printf "OPENING_ROOM_PENCILS_CREATE=%#x\n", gNdsOpeningRoomPencilsCreateResult',
        'printf "OPENING_ROOM_PENCILS_MASK=%#x\n", gNdsOpeningRoomPencilsCreateMask',
        'printf "OPENING_ROOM_PENCILS_GOBJ_DELTA=%u\n", gNdsOpeningRoomPencilsGObjDelta',
        'printf "OPENING_ROOM_PENCILS_DOBJ_DELTA=%u\n", gNdsOpeningRoomPencilsDObjDelta',
        'printf "OPENING_ROOM_PENCILS_XOBJ_DELTA=%u\n", gNdsOpeningRoomPencilsXObjDelta',
        'printf "OPENING_ROOM_PENCILS_AOBJ_DELTA=%u\n", gNdsOpeningRoomPencilsAObjDelta',
        'printf "OPENING_ROOM_PENCILS_PROCESS=%u\n", gNdsOpeningRoomPencilsProcessSet',
        'printf "OPENING_ROOM_PENCILS_DISPLAY=%u\n", gNdsOpeningRoomPencilsDisplaySet',
        'printf "OPENING_ROOM_PENCILS_TREE=%u\n", gNdsOpeningRoomPencilsDObjTreeCount',
        'printf "OPENING_ROOM_PENCILS_ROOTS=%u\n", gNdsOpeningRoomPencilsAnimRootCount',
        'printf "OPENING_ROOM_UPDATE=%#x\n", gNdsOpeningRoomUpdateResult',
        'printf "OPENING_ROOM_TICKS=%u\n", gNdsOpeningRoomTickCount',
        'printf "OPENING_ROOM_GOBJS=%u\n", gNdsOpeningRoomGObjCount',
        'printf "OPENING_ROOM_CAMERAS=%u\n", gNdsOpeningRoomCameraCount',
        'printf "OPENING_ROOM_DL0=%u\n", gNdsOpeningRoomDL0Size',
        'printf "OPENING_ROOM_DL1=%u\n", gNdsOpeningRoomDL1Size',
        'printf "OPENING_ROOM_GFXHEAP=%u\n", gNdsOpeningRoomGraphicsHeapSize',
        'printf "OPENING_ROOM_RDP_SIZE=%u\n", gNdsOpeningRoomRdpBufferSize',
        'printf "OPENING_ROOM_MALLOCS=%u\n", gNdsOpeningRoomMallocCount',
        'printf "OPENING_ROOM_PRE_ASSET=%#x\n", gNdsOpeningRoomPreAssetResult',
        'printf "OPENING_ROOM_CONTROLLER_CHECKS=%u\n", gNdsOpeningRoomControllerCheckCount',
        'printf "OPENING_ROOM_PULLED_KIND=%u\n", gNdsOpeningRoomPulledFighterKind',
        'printf "OPENING_ROOM_DROPPED_KIND=%u\n", gNdsOpeningRoomDroppedFighterKind',
        'printf "OPENING_ROOM_SKIP_TITLE=%u\n", gNdsOpeningRoomSkipToTitleCount',
        'printf "OPENING_MOVIE_ROOM_HANDOFF=%#x\n", gNdsOpeningMovieRoomHandoffResult',
        'printf "OPENING_MOVIE_ROOM_HANDOFF_TICK=%u\n", gNdsOpeningMovieRoomHandoffTick',
        'printf "OPENING_MOVIE_ROOM_HANDOFF_SCENE=%u\n", gNdsOpeningMovieRoomHandoffScene',
        'printf "OPENING_PORTRAITS_DISPATCH=%u\n", gNdsOpeningPortraitsDispatchCount',
        'printf "OPENING_PORTRAITS_START=%#x\n", gNdsOpeningPortraitsStartResult',
        'printf "OPENING_PORTRAITS_FUNC_START=%#x\n", gNdsOpeningPortraitsFuncStartResult',
        'printf "OPENING_PORTRAITS_UPDATE=%#x\n", gNdsOpeningPortraitsUpdateResult',
        'printf "OPENING_PORTRAITS_RELOC=%#x\n", gNdsOpeningPortraitsRelocResult',
        'printf "OPENING_PORTRAITS_SPRITE_NORM=%u,%u\n", gNdsOpeningPortraitsSpriteNormalizeCount, gNdsOpeningPortraitsSpriteNormalizeFailCount',
        'printf "OPENING_PORTRAITS_TICKS=%u\n", gNdsOpeningPortraitsTickCount',
        'printf "OPENING_PORTRAITS_DRAW=%#x\n", gNdsOpeningPortraitsDrawResult',
        'printf "OPENING_PORTRAITS_DRAW_BLOCKER=%u\n", gNdsOpeningPortraitsDrawBlocker',
        'printf "OPENING_PORTRAITS_DRAW_CALLBACKS=%u\n", gNdsOpeningPortraitsDrawCallbackCount',
        'printf "OPENING_PORTRAITS_DRAW_VISIBLE=%u\n", gNdsOpeningPortraitsDrawVisibleSObjCount',
        'printf "OPENING_PORTRAITS_DRAW_META=%u,%u,%u,%u,%u,%u\n", gNdsOpeningPortraitsDrawWidth, gNdsOpeningPortraitsDrawHeight, gNdsOpeningPortraitsDrawFormat, gNdsOpeningPortraitsDrawSize, gNdsOpeningPortraitsDrawBitmaps, gNdsOpeningPortraitsDrawPixels',
        'printf "OPENING_PORTRAITS_NEXT=%#x\n", gNdsOpeningPortraitsNextSceneResult',
        'printf "OPENING_PORTRAITS_NEXT_SCENE=%u\n", gNdsOpeningPortraitsNextSceneKind',
        'printf "OPENING_MARIO_DISPATCH=%u\n", gNdsOpeningMarioDispatchCount',
        'printf "OPENING_MARIO_START=%#x\n", gNdsOpeningMarioStartResult',
        'printf "OPENING_MARIO_FUNC_START=%#x\n", gNdsOpeningMarioFuncStartResult',
        'printf "OPENING_MARIO_UPDATE=%#x\n", gNdsOpeningMarioUpdateResult',
        'printf "OPENING_MARIO_TICKS=%u\n", gNdsOpeningMarioTickCount',
        'printf "OPENING_MARIO_RELOC=%#x\n", gNdsOpeningMarioRelocResult',
        'printf "OPENING_MARIO_SPRITE_NORM=%u,%u\n", gNdsOpeningMarioSpriteNormalizeCount, gNdsOpeningMarioSpriteNormalizeFailCount',
        'printf "OPENING_MARIO_DRAW=%#x\n", gNdsOpeningMarioDrawResult',
        'printf "OPENING_MARIO_DRAW_BLOCKER=%u\n", gNdsOpeningMarioDrawBlocker',
        'printf "OPENING_MARIO_DRAW_CALLBACKS=%u\n", gNdsOpeningMarioDrawCallbackCount',
        'printf "OPENING_MARIO_DRAW_VISIBLE=%u\n", gNdsOpeningMarioDrawVisibleSObjCount',
        'printf "OPENING_MARIO_DRAW_META=%u,%u,%u,%u,%u,%u\n", gNdsOpeningMarioDrawWidth, gNdsOpeningMarioDrawHeight, gNdsOpeningMarioDrawFormat, gNdsOpeningMarioDrawSize, gNdsOpeningMarioDrawBitmaps, gNdsOpeningMarioDrawPixels',
        'printf "OPENING_MARIO_FIGHTER_DEFER=%#x\n", gNdsOpeningMarioFighterDeferredResult',
        'printf "OPENING_MARIO_FIGHTER_DEFER_TICK=%u\n", gNdsOpeningMarioFighterDeferredTick',
        'printf "OPENING_MARIO_NEXT=%#x\n", gNdsOpeningMarioNextSceneResult',
        'printf "OPENING_MARIO_NEXT_SCENE=%u\n", gNdsOpeningMarioNextSceneKind',
        'printf "OPENING_NAME_MASKS=%#x,%#x,%#x,%#x,%#x,%#x\n", gNdsOpeningNameSceneDispatchMask, gNdsOpeningNameSceneFuncStartMask, gNdsOpeningNameSceneUpdateMask, gNdsOpeningNameSceneFighterDeferMask, gNdsOpeningNameSceneNextMask, gNdsOpeningNameSceneDrawMask',
        'printf "OPENING_NAME_COUNTS=%u,%u,%u,%u\n", gNdsOpeningNameSceneDispatchCount, gNdsOpeningNameSceneLastKind, gNdsOpeningNameSceneLastTick, gNdsOpeningNameSceneLastNextKind',
        'printf "OPENING_NAME_DRAW=%#x,%u,%u,%u\n", gNdsOpeningNameSceneDrawResult, gNdsOpeningNameSceneDrawBlocker, gNdsOpeningNameSceneDrawCallbackCount, gNdsOpeningNameSceneDrawVisibleSObjCount',
        'printf "OPENING_NAME_DRAW_META=%u,%u,%u,%u,%u,%u\n", gNdsOpeningNameSceneDrawWidth, gNdsOpeningNameSceneDrawHeight, gNdsOpeningNameSceneDrawFormat, gNdsOpeningNameSceneDrawSize, gNdsOpeningNameSceneDrawBitmaps, gNdsOpeningNameSceneDrawPixels',
        'printf "OPENING_MOVIE_BRIDGE=%#x,%#x,%u,%u,%u\n", gNdsOpeningMovieBridgeResult, gNdsOpeningMovieBridgeMask, gNdsOpeningMovieBridgeCount, gNdsOpeningMovieBridgeLastKind, gNdsOpeningMovieBridgeLastNextKind',
        'printf "OPENING_ACTION_PREVIEW=%#x,%#x,%u,%u,%u\n", gNdsOpeningMovieActionPreviewResult, gNdsOpeningMovieActionPreviewMask, gNdsOpeningMovieActionPreviewCount, gNdsOpeningMovieActionPreviewFrameCount, gNdsOpeningMovieActionPreviewPixels',
        'printf "OPENING_MOVIE_PRESENT_FRAMES=%u\n", gNdsOpeningMoviePresentFrameCount',
        'printf "OPENING_ACTION_PREVIEW_NORM=%u,%u\n", gNdsOpeningMovieActionPreviewSpriteNormalizeCount, gNdsOpeningMovieActionPreviewSpriteNormalizeFailCount',
        'printf "OPENING_ACTION_PREVIEW_META=%u,%u,%u,%u,%u\n", gNdsOpeningMovieActionPreviewLastKind, gNdsOpeningMovieActionPreviewLastWidth, gNdsOpeningMovieActionPreviewLastHeight, gNdsOpeningMovieActionPreviewLastFormat, gNdsOpeningMovieActionPreviewLastSize',
        'printf "OPENING_MOVIE_TITLE=%#x\n", gNdsOpeningMovieTitleResult',
        'printf "TITLE_RELOC=%#x\n", gNdsTitleRelocResult',
        'printf "TITLE_PREVIEW=%#x\n", gNdsTitlePreviewResult',
        'printf "TITLE_DRAW=%#x\n", gNdsTitleDrawResult',
        'printf "TITLE_SPRITE_NORM=%u,%u\n", gNdsTitleSpriteNormalizeCount, gNdsTitleSpriteNormalizeFailCount',
        'printf "TITLE_FIRE_SPRITE_NORM=%u,%u\n", gNdsTitleFireSpriteNormalizeCount, gNdsTitleFireSpriteNormalizeFailCount',
        'printf "TITLE_DRAW_COUNTS=%u,%u,%u,%u\n", gNdsTitleDrawVisibleSObjCount, gNdsTitleDrawRenderableSObjCount, gNdsTitleDrawSObjCount, gNdsTitleDrawPixels',
        'printf "TITLE_DRAW_META=%u,%u,%u,%u\n", gNdsTitleDrawLastWidth, gNdsTitleDrawLastHeight, gNdsTitleDrawLastFormat, gNdsTitleDrawLastSize',
        'printf "TITLE_ORIGINAL=%#x,%#x,%#x,%u,%u,%u,%#x\n", gNdsTitleOriginalStartResult, gNdsTitleOriginalFuncStartResult, gNdsTitleOriginalSetupMask, gNdsTitleOriginalLoadedFileCount, gNdsTitleOriginalGObjCount, gNdsTitleOriginalCameraCount, gNdsTitleOriginalDeferredMask',
        'printf "TITLE_LOGO_FIRE=%#x,%#x,%u,%u,%u,%u,%u\n", gNdsTitleOriginalLogoFireResult, gNdsTitleOriginalLogoFireMask, gNdsTitleOriginalLogoFireGObjDelta, gNdsTitleOriginalLogoFireLinkID, gNdsTitleOriginalLogoFireDLLinkID, gNdsTitleOriginalLogoFireCameraMaskLo, gNdsTitleOriginalLogoFireParticleBank',
        'printf "TITLE_FIRE=%#x,%#x,%u,%u,%u,%u,%u,%u\n", gNdsTitleOriginalFireResult, gNdsTitleOriginalFireMask, gNdsTitleOriginalFireGObjDelta, gNdsTitleOriginalFireSObjDelta, gNdsTitleOriginalFireGObjFlags, gNdsTitleOriginalFireSObjCount, gNdsTitleOriginalFireFrames, gNdsTitleOriginalFireAlpha',
        'printf "TITLE_ORIGINAL_UPDATE=%#x,%u,%u,%u,%u,%u,%u\n", gNdsTitleOriginalUpdateResult, gNdsTitleOriginalUpdateCount, gNdsTitleOriginalLayout, gNdsTitleOriginalTransitionTics, gNdsTitleOriginalStartActorProcess, gNdsTitleOriginalProceedScene, gNdsTitleOriginalProceedWait',
        'printf "PERF_FPS=%u,%u,%u,%u,%u\n", gNdsPerfPresentFps, gNdsPerfLogicFps, gNdsPerfDLDrawFps, gNdsPerfSampleCount, gNdsPerfSampleWindowTicks',
        'printf "PERF_CONTENT=%u,%u,%u,%u\n", gNdsOriginalSpritePreviewCommitCount, gNdsOriginalDLPreviewCommitCount, gNdsPerfPreviewCommitFps, gNdsPerfPreviewCommitCount',
        'printf "FRAMES=%u\n", gNdsFrameCounter',
        'detach'
    )
    # Write as raw lines so GDB's printf \n escapes stay literal.
    [System.IO.File]::WriteAllLines($gdbScriptPath, $gdbCommands)
    Remove-Item $gdbStdoutPath, $gdbStderrPath -Force -ErrorAction SilentlyContinue
    $gdbproc = Start-Process -FilePath $Gdb `
        -ArgumentList @('-q', '-batch', '-x', $gdbScriptPath, $elf) `
        -WorkingDirectory $root `
        -RedirectStandardOutput $gdbStdoutPath `
        -RedirectStandardError $gdbStderrPath `
        -Wait `
        -PassThru
    $gdbproc.WaitForExit()
    $gdbStdout =
        if (Test-Path $gdbStdoutPath) { Get-Content $gdbStdoutPath -Raw }
        else { '' }
    $gdbStderr =
        if (Test-Path $gdbStderrPath) { Get-Content $gdbStderrPath -Raw }
        else { '' }
    if ($gdbproc.ExitCode -ne 0 -or [string]::IsNullOrEmpty($gdbStdout)) {
        throw ("GDB did not complete successfully (exit $($gdbproc.ExitCode))." +
               "`nstdout:$gdbStdout`nstderr:$gdbStderr")
    }
    $gdbOutput = $gdbStdout
    $aobj32 = [regex]::Match($gdbOutput, 'AOBJ32=([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $aobj32Color = [regex]::Match($gdbOutput, 'AOBJ32_COLOR=([0-9]+)')
    $selfTest = [regex]::Match($gdbOutput, 'SELFTEST=(0x[0-9a-fA-F]+)')
    $boot = [regex]::Match($gdbOutput, 'BOOT=(0x[0-9a-fA-F]+)')
    $scheduler = [regex]::Match($gdbOutput, 'SCHED=([0-9]+)')
    $controllers = [regex]::Match($gdbOutput, 'CONTROLLERS=([0-9]+)')
    $controllerPolls = [regex]::Match($gdbOutput, 'CONTROLLER_POLLS=([0-9]+)')
    $video = [regex]::Match($gdbOutput, 'VIDEO=(0x[0-9a-fA-F]+)')
    $relocNitro = [regex]::Match($gdbOutput, 'RELOC_NITRO=(0x[0-9a-fA-F]+)')
    $relocHeaders = [regex]::Match($gdbOutput, 'RELOC_HEADERS=([0-9]+)')
    $relocPayloads = [regex]::Match($gdbOutput, 'RELOC_PAYLOADS=([0-9]+)')
    $relocOpenFails = [regex]::Match($gdbOutput, 'RELOC_OPEN_FAILS=([0-9]+)')
    $relocFormatFails = [regex]::Match($gdbOutput, 'RELOC_FORMAT_FAILS=([0-9]+)')
    $relocShortReads = [regex]::Match($gdbOutput, 'RELOC_SHORT_READS=([0-9]+)')
    $scene = [regex]::Match($gdbOutput, 'SCENE=(0x[0-9a-fA-F]+)')
    $sceneKind = [regex]::Match($gdbOutput, 'SCENE_KIND=([0-9]+)')
    $startup = [regex]::Match($gdbOutput, 'STARTUP=(0x[0-9a-fA-F]+)')
    $startupKind = [regex]::Match($gdbOutput, 'STARTUP_KIND=([0-9]+)')
    $startupDL0 = [regex]::Match($gdbOutput, 'STARTUP_DL0=([0-9]+)')
    $startupDL1 = [regex]::Match($gdbOutput, 'STARTUP_DL1=([0-9]+)')
    $startupController = [regex]::Match($gdbOutput, 'STARTUP_CONTROLLER=([0-9]+)')
    $startupFunc = [regex]::Match($gdbOutput, 'STARTUP_FUNC=(0x[0-9a-fA-F]+)')
    $startupSkip = [regex]::Match($gdbOutput, 'STARTUP_SKIP=([0-9]+)')
    $startupOpening = [regex]::Match($gdbOutput, 'STARTUP_OPENING=([0-9]+)')
    $startupGObjs = [regex]::Match($gdbOutput, 'STARTUP_GOBJS=([0-9]+)')
    $startupCameras = [regex]::Match($gdbOutput, 'STARTUP_CAMERAS=([0-9]+)')
    $startupReloc = [regex]::Match($gdbOutput, 'STARTUP_RELOC=([0-9]+)')
    $startupSprites = [regex]::Match($gdbOutput, 'STARTUP_SPRITES=([0-9]+)')
    $startupFades = [regex]::Match($gdbOutput, 'STARTUP_FADES=([0-9]+)')
    $startupParent = [regex]::Match($gdbOutput, 'STARTUP_PARENT=([0-9]+)')
    $startupLogoX = [regex]::Match($gdbOutput, 'STARTUP_LOGO_X=([0-9]+)')
    $startupLogoY = [regex]::Match($gdbOutput, 'STARTUP_LOGO_Y=([0-9]+)')
    $startupFastcopy = [regex]::Match($gdbOutput, 'STARTUP_FASTCOPY=([0-9]+)')
    $startupLogoReloc = [regex]::Match($gdbOutput, 'STARTUP_LOGO_RELOC=(0x[0-9a-fA-F]+)')
    $startupLogoRelocSize = [regex]::Match($gdbOutput, 'STARTUP_LOGO_RELOC_SIZE=([0-9]+)')
    $startupLogoSwapCount = [regex]::Match($gdbOutput, 'STARTUP_LOGO_SWAP_COUNT=([0-9]+)')
    $startupLogoPointerCount = [regex]::Match($gdbOutput, 'STARTUP_LOGO_POINTER_COUNT=([0-9]+)')
    $startupLogoDraw = [regex]::Match($gdbOutput, 'STARTUP_LOGO_DRAW=(0x[0-9a-fA-F]+)')
    $startupLogoBlocker = [regex]::Match($gdbOutput, 'STARTUP_LOGO_BLOCKER=([0-9]+)')
    $startupLogoCallbacks = [regex]::Match($gdbOutput, 'STARTUP_LOGO_CALLBACKS=([0-9]+)')
    $startupLogoDrawUpdate = [regex]::Match($gdbOutput, 'STARTUP_LOGO_DRAW_UPDATE=([0-9]+)')
    $startupLogoDrawWidth = [regex]::Match($gdbOutput, 'STARTUP_LOGO_DRAW_W=([0-9]+)')
    $startupLogoDrawHeight = [regex]::Match($gdbOutput, 'STARTUP_LOGO_DRAW_H=([0-9]+)')
    $startupLogoDrawFormat = [regex]::Match($gdbOutput, 'STARTUP_LOGO_DRAW_FMT=([0-9]+)')
    $startupLogoDrawSize = [regex]::Match($gdbOutput, 'STARTUP_LOGO_DRAW_SIZ=([0-9]+)')
    $startupLogoDrawBitmaps = [regex]::Match($gdbOutput, 'STARTUP_LOGO_DRAW_BM=([0-9]+)')
    $startupLogoDrawPixels = [regex]::Match($gdbOutput, 'STARTUP_LOGO_DRAW_PIX=([0-9]+)')
    $startupLogoDrawAttr = [regex]::Match($gdbOutput, 'STARTUP_LOGO_DRAW_ATTR=(0x[0-9a-fA-F]+)')
    $startupLogoDrawTexshuf = [regex]::Match($gdbOutput, 'STARTUP_LOGO_DRAW_TEXSHUF=([0-9]+),([0-9]+)')
    $startupLogoDisplay = [regex]::Match($gdbOutput, 'STARTUP_LOGO_DISPLAY=([0-9]+),([0-9]+)')
    $startupActorFunc = [regex]::Match($gdbOutput, 'STARTUP_ACTOR_FUNC=([0-9]+)')
    $startupProcKind = [regex]::Match($gdbOutput, 'STARTUP_PROC_KIND=([0-9]+)')
    $startupProcPriority = [regex]::Match($gdbOutput, 'STARTUP_PROC_PRIORITY=([0-9]+)')
    $startupDisplay = [regex]::Match($gdbOutput, 'STARTUP_DISPLAY=([0-9]+)')
    $startupCamMask = [regex]::Match($gdbOutput, 'STARTUP_CAM_MASK=([0-9]+)')
    $startupDefaultColor = [regex]::Match($gdbOutput, 'STARTUP_DEFAULT_COLOR=(0x[0-9a-fA-F]+)')
    $taskman = [regex]::Match($gdbOutput, 'TASKMAN=(0x[0-9a-fA-F]+)')
    $taskmanContexts = [regex]::Match($gdbOutput, 'TASKMAN_CONTEXTS=([0-9]+)')
    $taskmanGfxNum = [regex]::Match($gdbOutput, 'TASKMAN_GFXNUM=([0-9]+)')
    $taskmanGfxHeap = [regex]::Match($gdbOutput, 'TASKMAN_GFXHEAP=([0-9]+)')
    $taskmanRdpKind = [regex]::Match($gdbOutput, 'TASKMAN_RDP_KIND=([0-9]+)')
    $taskmanRdpSize = [regex]::Match($gdbOutput, 'TASKMAN_RDP_SIZE=([0-9]+)')
    $taskmanMallocs = [regex]::Match($gdbOutput, 'TASKMAN_MALLOCS=([0-9]+)')
    $startupTaskmanMallocs = [regex]::Match($gdbOutput, 'STARTUP_TASKMAN_MALLOCS=([0-9]+)')
    $taskmanUsed = [regex]::Match($gdbOutput, 'TASKMAN_USED=([0-9]+)')
    $taskmanDLValid = [regex]::Match($gdbOutput, 'TASKMAN_DL_VALID=([0-9]+)')
    $taskmanAutoRead = [regex]::Match($gdbOutput, 'TASKMAN_AUTOREAD=([0-9]+)')
    $taskmanUpdate = [regex]::Match($gdbOutput, 'TASKMAN_UPDATE=([0-9]+)')
    $taskmanDraw = [regex]::Match($gdbOutput, 'TASKMAN_DRAW=([0-9]+)')
    $taskmanLights = [regex]::Match($gdbOutput, 'TASKMAN_LIGHTS=([0-9]+)')
    $taskmanLoop = [regex]::Match($gdbOutput, 'TASKMAN_LOOP=([0-9]+)')
    $taskmanUpdates = [regex]::Match($gdbOutput, 'TASKMAN_UPDATES=([0-9]+)')
    $taskmanSkipAfter = [regex]::Match($gdbOutput, 'TASKMAN_SKIP_AFTER=([0-9]+)')
    $taskmanGObjSleeps = [regex]::Match($gdbOutput, 'TASKMAN_GOBJ_SLEEPS=([0-9]+)')
    $taskmanLogoPostX = [regex]::Match($gdbOutput, 'TASKMAN_LOGO_POST_X=([0-9]+)')
    $taskmanLogoPostY = [regex]::Match($gdbOutput, 'TASKMAN_LOGO_POST_Y=([0-9]+)')
    $taskmanOpening = [regex]::Match($gdbOutput, 'TASKMAN_OPENING=([0-9]+)')
    $taskmanPostScene = [regex]::Match($gdbOutput, 'TASKMAN_POST_SCENE=([0-9]+)')
    $taskmanPostPrev = [regex]::Match($gdbOutput, 'TASKMAN_POST_PREV=([0-9]+)')
    $taskmanPostStatus = [regex]::Match($gdbOutput, 'TASKMAN_POST_STATUS=([0-9]+)')
    $taskmanPostGObjs = [regex]::Match($gdbOutput, 'TASKMAN_POST_GOBJS=([0-9]+)')
    $taskmanPostFades = [regex]::Match($gdbOutput, 'TASKMAN_POST_FADES=([0-9]+)')
    $taskmanCleanup = [regex]::Match($gdbOutput, 'TASKMAN_CLEANUP=(0x[0-9a-fA-F]+)')
    $taskmanCleanupQueues = [regex]::Match($gdbOutput, 'TASKMAN_CLEANUP_QUEUES=([0-9]+)')
    $taskmanCleanupMode = [regex]::Match($gdbOutput, 'TASKMAN_CLEANUP_MODE=([0-9]+)')
    $taskmanReturns = [regex]::Match($gdbOutput, 'TASKMAN_RETURNS=([0-9]+)')
    $openingRoomDispatch = [regex]::Match($gdbOutput, 'OPENING_ROOM_DISPATCH=([0-9]+)')
    $openingRoomStart = [regex]::Match($gdbOutput, 'OPENING_ROOM_START=(0x[0-9a-fA-F]+)')
    $openingRoomFuncStart = [regex]::Match($gdbOutput, 'OPENING_ROOM_FUNC_START=(0x[0-9a-fA-F]+)')
    $openingRoomReloc = [regex]::Match($gdbOutput, 'OPENING_ROOM_RELOC=(0x[0-9a-fA-F]+)')
    $openingRoomRelocInit = [regex]::Match($gdbOutput, 'OPENING_ROOM_RELOC_INIT=([0-9]+)')
    $openingRoomRelocLoad = [regex]::Match($gdbOutput, 'OPENING_ROOM_RELOC_LOAD=([0-9]+)')
    $openingRoomRelocMask = [regex]::Match($gdbOutput, 'OPENING_ROOM_RELOC_MASK=(0x[0-9a-fA-F]+)')
    $openingRoomRelocHeaderMask = [regex]::Match($gdbOutput, 'OPENING_ROOM_RELOC_HEADER_MASK=(0x[0-9a-fA-F]+)')
    $openingRoomRelocPayloadMask = [regex]::Match($gdbOutput, 'OPENING_ROOM_RELOC_PAYLOAD_MASK=(0x[0-9a-fA-F]+)')
    $openingRoomRelocData = [regex]::Match($gdbOutput, 'OPENING_ROOM_RELOC_DATA=([0-9]+)')
    $openingRoomRelocFixup = [regex]::Match($gdbOutput, 'OPENING_ROOM_RELOC_FIXUP=([0-9]+)')
    $openingRoomRelocBytes = [regex]::Match($gdbOutput, 'OPENING_ROOM_RELOC_BYTES=([0-9]+)')
    $openingRoomRelocLastID = [regex]::Match($gdbOutput, 'OPENING_ROOM_RELOC_LAST_ID=([0-9]+)')
    $openingRoomRelocLastSize = [regex]::Match($gdbOutput, 'OPENING_ROOM_RELOC_LAST_SIZE=([0-9]+)')
    $openingRoomRelocWordSwapMask = [regex]::Match($gdbOutput, 'OPENING_ROOM_RELOC_WORD_SWAP_MASK=(0x[0-9a-fA-F]+)')
    $openingRoomRelocWordSwapCount = [regex]::Match($gdbOutput, 'OPENING_ROOM_RELOC_WORD_SWAP_COUNT=([0-9]+)')
    $openingRoomRelocWordSwapFails = [regex]::Match($gdbOutput, 'OPENING_ROOM_RELOC_WORD_SWAP_FAILS=([0-9]+)')
    $openingRoomRelocPointerMask = [regex]::Match($gdbOutput, 'OPENING_ROOM_RELOC_POINTER_MASK=(0x[0-9a-fA-F]+)')
    $openingRoomRelocPointerCount = [regex]::Match($gdbOutput, 'OPENING_ROOM_RELOC_POINTER_COUNT=([0-9]+)')
    $openingRoomRelocPointerFails = [regex]::Match($gdbOutput, 'OPENING_ROOM_RELOC_POINTER_FAILS=([0-9]+)')
    $openingRoomRelocSymbolCount = [regex]::Match($gdbOutput, 'OPENING_ROOM_RELOC_SYMBOL_COUNT=([0-9]+)')
    $openingRoomRelocSymbolFails = [regex]::Match($gdbOutput, 'OPENING_ROOM_RELOC_SYMBOL_FAILS=([0-9]+)')
    $openingRoomRelocSymbolProbeMask = [regex]::Match($gdbOutput, 'OPENING_ROOM_RELOC_SYMBOL_PROBE_MASK=(0x[0-9a-fA-F]+)')
    $openingRoomRelocSymbolLast = [regex]::Match($gdbOutput, 'OPENING_ROOM_RELOC_SYMBOL_LAST=([0-9]+)')
    $openingRoomRelocMObjNormalize = [regex]::Match($gdbOutput, 'OPENING_ROOM_RELOC_MOBJ_NORMALIZE=([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0)')
    $openingRoomRelocMObjSource = [regex]::Match($gdbOutput, 'OPENING_ROOM_RELOC_MOBJ_SOURCE=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),((?:0x[0-9a-fA-F]+)|0)')
    $openingRoomFirstEvent = [regex]::Match($gdbOutput, 'OPENING_ROOM_FIRST_EVENT=(0x[0-9a-fA-F]+)')
    $openingRoomFirstEventTick = [regex]::Match($gdbOutput, 'OPENING_ROOM_FIRST_EVENT_TICK=([0-9]+)')
    $openingRoomFirstEventMask = [regex]::Match($gdbOutput, 'OPENING_ROOM_FIRST_EVENT_MASK=(0x[0-9a-fA-F]+)')
    $openingRoomFirstEventPencilsDObj = [regex]::Match($gdbOutput, 'OPENING_ROOM_FIRST_EVENT_PENCILS_DOBJ=([0-9]+)')
    $openingRoomFirstEventPencilsAnim = [regex]::Match($gdbOutput, 'OPENING_ROOM_FIRST_EVENT_PENCILS_ANIM=([0-9]+)')
    $openingRoomFirstEventData = [regex]::Match($gdbOutput, 'OPENING_ROOM_FIRST_EVENT_DATA=(0x[0-9a-fA-F]+)')
    $openingRoomFirstEventDataMask = [regex]::Match($gdbOutput, 'OPENING_ROOM_FIRST_EVENT_DATA_MASK=(0x[0-9a-fA-F]+)')
    $openingRoomFirstEventDObjEntries = [regex]::Match($gdbOutput, 'OPENING_ROOM_FIRST_EVENT_DOBJ_ENTRIES=([0-9]+)')
    $openingRoomFirstEventDObjDLs = [regex]::Match($gdbOutput, 'OPENING_ROOM_FIRST_EVENT_DOBJ_DLS=([0-9]+)')
    $openingRoomFirstEventAnimJoints = [regex]::Match($gdbOutput, 'OPENING_ROOM_FIRST_EVENT_ANIM_JOINTS=([0-9]+)')
    $openingRoomFirstEventAnimOpcode = [regex]::Match($gdbOutput, 'OPENING_ROOM_FIRST_EVENT_ANIM_OPCODE=([0-9]+)')
    $openingRoomFirstEventRun = [regex]::Match($gdbOutput, 'OPENING_ROOM_FIRST_EVENT_RUN=(0x[0-9a-fA-F]+)')
    $openingRoomFirstEventDeferred = [regex]::Match($gdbOutput, 'OPENING_ROOM_FIRST_EVENT_DEFERRED=(0x[0-9a-fA-F]+)')
    $openingRoomFighterDeferred = [regex]::Match($gdbOutput, 'OPENING_ROOM_FIGHTER_DEFER=(0x[0-9a-fA-F]+)')
    $openingRoomFighterDeferredKind = [regex]::Match($gdbOutput, 'OPENING_ROOM_FIGHTER_DEFER_KIND=([0-9]+)')
    $openingRoomTick380Deferred = [regex]::Match($gdbOutput, 'OPENING_ROOM_TICK380_DEFER=(0x[0-9a-fA-F]+)')
    $openingRoomTick380DeferredMask = [regex]::Match($gdbOutput, 'OPENING_ROOM_TICK380_DEFERRED=(0x[0-9a-fA-F]+)')
    $openingRoomTick450Run = [regex]::Match($gdbOutput, 'OPENING_ROOM_TICK450_RUN=(0x[0-9a-fA-F]+)')
    $openingRoomTick450Deferred = [regex]::Match($gdbOutput, 'OPENING_ROOM_TICK450_DEFERRED=(0x[0-9a-fA-F]+|0)')
    $openingRoomOutsideAsset = [regex]::Match($gdbOutput, 'OPENING_ROOM_OUTSIDE_ASSET=(0x[0-9a-fA-F]+)')
    $openingRoomOutsideDLOffset = [regex]::Match($gdbOutput, 'OPENING_ROOM_OUTSIDE_DL_OFFSET=([0-9]+)')
    $openingRoomOutsideCreate = [regex]::Match($gdbOutput, 'OPENING_ROOM_OUTSIDE_CREATE=(0x[0-9a-fA-F]+)')
    $openingRoomOutsideCreateMask = [regex]::Match($gdbOutput, 'OPENING_ROOM_OUTSIDE_CREATE_MASK=(0x[0-9a-fA-F]+)')
    $openingRoomOutsideCreateGObjs = [regex]::Match($gdbOutput, 'OPENING_ROOM_OUTSIDE_CREATE_GOBJS=([0-9]+)')
    $openingRoomOutsideGObjDelta = [regex]::Match($gdbOutput, 'OPENING_ROOM_OUTSIDE_GOBJ_DELTA=([0-9]+)')
    $openingRoomOutsideDObjDelta = [regex]::Match($gdbOutput, 'OPENING_ROOM_OUTSIDE_DOBJ_DELTA=([0-9]+)')
    $openingRoomOutsideXObjDelta = [regex]::Match($gdbOutput, 'OPENING_ROOM_OUTSIDE_XOBJ_DELTA=([0-9]+)')
    $openingRoomOutsideDisplay = [regex]::Match($gdbOutput, 'OPENING_ROOM_OUTSIDE_DISPLAY=([0-9]+)')
    $openingRoomHazeAsset = [regex]::Match($gdbOutput, 'OPENING_ROOM_HAZE_ASSET=(0x[0-9a-fA-F]+)')
    $openingRoomHazeDLOffset = [regex]::Match($gdbOutput, 'OPENING_ROOM_HAZE_DL_OFFSET=([0-9]+)')
    $openingRoomHazeCreate = [regex]::Match($gdbOutput, 'OPENING_ROOM_HAZE_CREATE=(0x[0-9a-fA-F]+)')
    $openingRoomHazeCreateMask = [regex]::Match($gdbOutput, 'OPENING_ROOM_HAZE_CREATE_MASK=(0x[0-9a-fA-F]+)')
    $openingRoomHazeCreateGObjs = [regex]::Match($gdbOutput, 'OPENING_ROOM_HAZE_CREATE_GOBJS=([0-9]+)')
    $openingRoomHazeGObjDelta = [regex]::Match($gdbOutput, 'OPENING_ROOM_HAZE_GOBJ_DELTA=([0-9]+)')
    $openingRoomHazeDObjDelta = [regex]::Match($gdbOutput, 'OPENING_ROOM_HAZE_DOBJ_DELTA=([0-9]+)')
    $openingRoomHazeXObjDelta = [regex]::Match($gdbOutput, 'OPENING_ROOM_HAZE_XOBJ_DELTA=([0-9]+)')
    $openingRoomHazeDisplay = [regex]::Match($gdbOutput, 'OPENING_ROOM_HAZE_DISPLAY=([0-9]+)')
    $openingRoomSunlightAsset = [regex]::Match($gdbOutput, 'OPENING_ROOM_SUNLIGHT_ASSET=(0x[0-9a-fA-F]+)')
    $openingRoomSunlightDLOffset = [regex]::Match($gdbOutput, 'OPENING_ROOM_SUNLIGHT_DL_OFFSET=([0-9]+)')
    $openingRoomSunlightCreate = [regex]::Match($gdbOutput, 'OPENING_ROOM_SUNLIGHT_CREATE=(0x[0-9a-fA-F]+)')
    $openingRoomSunlightCreateMask = [regex]::Match($gdbOutput, 'OPENING_ROOM_SUNLIGHT_CREATE_MASK=(0x[0-9a-fA-F]+)')
    $openingRoomSunlightCreateGObjs = [regex]::Match($gdbOutput, 'OPENING_ROOM_SUNLIGHT_CREATE_GOBJS=([0-9]+)')
    $openingRoomSunlightGObjDelta = [regex]::Match($gdbOutput, 'OPENING_ROOM_SUNLIGHT_GOBJ_DELTA=([0-9]+)')
    $openingRoomSunlightDObjDelta = [regex]::Match($gdbOutput, 'OPENING_ROOM_SUNLIGHT_DOBJ_DELTA=([0-9]+)')
    $openingRoomSunlightXObjDelta = [regex]::Match($gdbOutput, 'OPENING_ROOM_SUNLIGHT_XOBJ_DELTA=([0-9]+)')
    $openingRoomSunlightDisplay = [regex]::Match($gdbOutput, 'OPENING_ROOM_SUNLIGHT_DISPLAY=([0-9]+)')
    $openingRoomDeskAsset = [regex]::Match($gdbOutput, 'OPENING_ROOM_DESK_ASSET=(0x[0-9a-fA-F]+)')
    $openingRoomDeskDObjOffset = [regex]::Match($gdbOutput, 'OPENING_ROOM_DESK_DOBJ_OFFSET=([0-9]+)')
    $openingRoomDeskCreate = [regex]::Match($gdbOutput, 'OPENING_ROOM_DESK_CREATE=(0x[0-9a-fA-F]+)')
    $openingRoomDeskCreateMask = [regex]::Match($gdbOutput, 'OPENING_ROOM_DESK_CREATE_MASK=(0x[0-9a-fA-F]+)')
    $openingRoomDeskCreateGObjs = [regex]::Match($gdbOutput, 'OPENING_ROOM_DESK_CREATE_GOBJS=([0-9]+)')
    $openingRoomDeskGObjDelta = [regex]::Match($gdbOutput, 'OPENING_ROOM_DESK_GOBJ_DELTA=([0-9]+)')
    $openingRoomDeskDObjDelta = [regex]::Match($gdbOutput, 'OPENING_ROOM_DESK_DOBJ_DELTA=([0-9]+)')
    $openingRoomDeskXObjDelta = [regex]::Match($gdbOutput, 'OPENING_ROOM_DESK_XOBJ_DELTA=([0-9]+)')
    $openingRoomDeskDisplay = [regex]::Match($gdbOutput, 'OPENING_ROOM_DESK_DISPLAY=([0-9]+)')
    $openingRoomSunlightEject = [regex]::Match($gdbOutput, 'OPENING_ROOM_SUNLIGHT_EJECT=(0x[0-9a-fA-F]+)')
    $openingRoomSunlightEjectBefore = [regex]::Match($gdbOutput, 'OPENING_ROOM_SUNLIGHT_EJECT_BEFORE=([0-9]+)')
    $openingRoomSunlightEjectAfter = [regex]::Match($gdbOutput, 'OPENING_ROOM_SUNLIGHT_EJECT_AFTER=([0-9]+)')
    $openingRoomSunlightEjectUnlinked = [regex]::Match($gdbOutput, 'OPENING_ROOM_SUNLIGHT_EJECT_UNLINKED=(0x[0-9a-fA-F]+)')
    $openingRoomCloseUpOverlayCreate = [regex]::Match($gdbOutput, 'OPENING_ROOM_CLOSEUP_OVERLAY_CREATE=(0x[0-9a-fA-F]+)')
    $openingRoomCloseUpOverlayCreateMask = [regex]::Match($gdbOutput, 'OPENING_ROOM_CLOSEUP_OVERLAY_CREATE_MASK=(0x[0-9a-fA-F]+)')
    $openingRoomCloseUpOverlayCreateTick = [regex]::Match($gdbOutput, 'OPENING_ROOM_CLOSEUP_OVERLAY_CREATE_TICK=([0-9]+)')
    $openingRoomCloseUpOverlayCreateGObjs = [regex]::Match($gdbOutput, 'OPENING_ROOM_CLOSEUP_OVERLAY_CREATE_GOBJS=([0-9]+)')
    $openingRoomCloseUpOverlayGObjDelta = [regex]::Match($gdbOutput, 'OPENING_ROOM_CLOSEUP_OVERLAY_GOBJ_DELTA=([0-9]+)')
    $openingRoomCloseUpOverlayDisplay = [regex]::Match($gdbOutput, 'OPENING_ROOM_CLOSEUP_OVERLAY_DISPLAY=([0-9]+)')
    $openingRoomCloseUpOverlayAlpha = [regex]::Match($gdbOutput, 'OPENING_ROOM_CLOSEUP_OVERLAY_ALPHA=([0-9]+)')
    $openingRoomTick500Run = [regex]::Match($gdbOutput, 'OPENING_ROOM_TICK500_RUN=(0x[0-9a-fA-F]+)')
    $openingRoomTick500Deferred = [regex]::Match($gdbOutput, 'OPENING_ROOM_TICK500_DEFERRED=(0x[0-9a-fA-F]+)')
    $openingRoomTick560Run = [regex]::Match($gdbOutput, 'OPENING_ROOM_TICK560_RUN=(0x[0-9a-fA-F]+)')
    $openingRoomTick560Deferred = [regex]::Match($gdbOutput, 'OPENING_ROOM_TICK560_DEFERRED=(0x[0-9a-fA-F]+)')
    $openingRoomDraw = [regex]::Match($gdbOutput, 'OPENING_ROOM_DRAW=(0x[0-9a-fA-F]+)')
    $openingRoomDrawBlocker = [regex]::Match($gdbOutput, 'OPENING_ROOM_DRAW_BLOCKER=([0-9]+)')
    $openingRoomDrawTick = [regex]::Match($gdbOutput, 'OPENING_ROOM_DRAW_TICK=([0-9]+)')
    $openingRoomDrawFrame = [regex]::Match($gdbOutput, 'OPENING_ROOM_DRAW_FRAME=([0-9]+)')
    $openingRoomDrawProbes = [regex]::Match($gdbOutput, 'OPENING_ROOM_DRAW_PROBES=([0-9]+),([0-9]+)')
    $openingRoomDrawCameras = [regex]::Match($gdbOutput, 'OPENING_ROOM_DRAW_CAMERAS=([0-9]+)')
    $openingRoomDrawDisplays = [regex]::Match($gdbOutput, 'OPENING_ROOM_DRAW_DISPLAYS=([0-9]+)')
    $openingRoomDrawDObjs = [regex]::Match($gdbOutput, 'OPENING_ROOM_DRAW_DOBJS=([0-9]+)')
    $openingRoomDrawCameraMask = [regex]::Match($gdbOutput, 'OPENING_ROOM_DRAW_CAMERA_MASK=(0x[0-9a-fA-F]+)')
    $openingRoomDrawCameraPriority = [regex]::Match($gdbOutput, 'OPENING_ROOM_DRAW_CAMERA_PRIORITY=([0-9]+)')
    $openingRoomDrawCameraFlags = [regex]::Match($gdbOutput, 'OPENING_ROOM_DRAW_CAMERA_FLAGS=(0x[0-9a-fA-F]+)')
    $openingRoomDrawCameraXObjs = [regex]::Match($gdbOutput, 'OPENING_ROOM_DRAW_CAMERA_XOBJS=([0-9]+),([0-9]+),([0-9]+)')
    $openingRoomDrawCameraViewport = [regex]::Match($gdbOutput, 'OPENING_ROOM_DRAW_CAMERA_VIEWPORT=(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+)')
    $rdpDefaultViewport = [regex]::Match($gdbOutput, 'RDP_DEFAULT_VIEWPORT=([0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+)')
    $openingRoomDrawCameraPersp = [regex]::Match($gdbOutput, 'OPENING_ROOM_DRAW_CAMERA_PERSP=(-?[0-9]+),(-?[0-9]+),(-?[0-9]+)')
    $openingRoomDrawCameraEye = [regex]::Match($gdbOutput, 'OPENING_ROOM_DRAW_CAMERA_EYE=(-?[0-9]+),(-?[0-9]+),(-?[0-9]+)')
    $openingRoomDrawCameraAt = [regex]::Match($gdbOutput, 'OPENING_ROOM_DRAW_CAMERA_AT=(-?[0-9]+),(-?[0-9]+),(-?[0-9]+)')
    $openingRoomDrawObjectLink = [regex]::Match($gdbOutput, 'OPENING_ROOM_DRAW_OBJECT_LINK=([0-9]+)')
    $openingRoomDrawObjectID = [regex]::Match($gdbOutput, 'OPENING_ROOM_DRAW_OBJECT_ID=([0-9]+)')
    $openingRoomDrawObjectKind = [regex]::Match($gdbOutput, 'OPENING_ROOM_DRAW_OBJECT_KIND=([0-9]+)')
    $openingRoomDrawCallback = [regex]::Match($gdbOutput, 'OPENING_ROOM_DRAW_CALLBACK=(0x[0-9a-fA-F]+)')
    $openingRoomDrawDObjDL = [regex]::Match($gdbOutput, 'OPENING_ROOM_DRAW_DOBJ_DL=(0x[0-9a-fA-F]+)')
    $openingRoomDrawDObjMeta = [regex]::Match($gdbOutput, 'OPENING_ROOM_DRAW_DOBJ_META=(0x[0-9a-fA-F]+)')
    $openingRoomDrawMaterialCandidate = [regex]::Match($gdbOutput, 'OPENING_ROOM_DRAW_MATERIAL_CANDIDATE=(0x[0-9a-fA-F]+|0),([0-9]+),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $openingRoomDrawMaterialObject = [regex]::Match($gdbOutput, 'OPENING_ROOM_DRAW_MATERIAL_OBJECT=([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0)')
    $openingRoomDrawMaterialDObj = [regex]::Match($gdbOutput, 'OPENING_ROOM_DRAW_MATERIAL_DOBJ=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $openingRoomDrawMaterialMObj = [regex]::Match($gdbOutput, 'OPENING_ROOM_DRAW_MATERIAL_MOBJ=([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $openingRoomDrawMaterialMObjIds = [regex]::Match($gdbOutput, 'OPENING_ROOM_DRAW_MATERIAL_MOBJ_IDS=([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+)')
    $openingRoomDrawMaterialMObjFormat = [regex]::Match($gdbOutput, 'OPENING_ROOM_DRAW_MATERIAL_MOBJ_FORMAT=([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $openingRoomDrawMaterialMObjTile = [regex]::Match($gdbOutput, 'OPENING_ROOM_DRAW_MATERIAL_MOBJ_TILE=([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $openingRoomDrawMaterialMObjST = [regex]::Match($gdbOutput, 'OPENING_ROOM_DRAW_MATERIAL_MOBJ_ST=(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+)')
    $openingRoomDrawMaterialMObjArrays = [regex]::Match($gdbOutput, 'OPENING_ROOM_DRAW_MATERIAL_MOBJ_ARRAYS=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $openingRoomDrawMaterialMObjPtrs = [regex]::Match($gdbOutput, 'OPENING_ROOM_DRAW_MATERIAL_MOBJ_PTRS=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $openingRoomDrawTextureMaterial = [regex]::Match($gdbOutput, 'OPENING_ROOM_DRAW_TEXTURE_MATERIAL=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $openingRoomDrawTextureMaterialObject = [regex]::Match($gdbOutput, 'OPENING_ROOM_DRAW_TEXTURE_MATERIAL_OBJECT=([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0)')
    $openingRoomDrawTextureMaterialDObj = [regex]::Match($gdbOutput, 'OPENING_ROOM_DRAW_TEXTURE_MATERIAL_DOBJ=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $openingRoomDrawTextureMaterialMObj = [regex]::Match($gdbOutput, 'OPENING_ROOM_DRAW_TEXTURE_MATERIAL_MOBJ=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+)')
    $openingRoomDrawTextureMaterialPtrs = [regex]::Match($gdbOutput, 'OPENING_ROOM_DRAW_TEXTURE_MATERIAL_PTRS=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $openingRoomDrawMaterialBranch = [regex]::Match($gdbOutput, 'OPENING_ROOM_DRAW_MATERIAL_BRANCH=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $openingRoomDrawMaterialBranchFirst = [regex]::Match($gdbOutput, 'OPENING_ROOM_DRAW_MATERIAL_BRANCH_FIRST=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+)')
    $openingRoomDrawMaterialBranchTile = [regex]::Match($gdbOutput, 'OPENING_ROOM_DRAW_MATERIAL_BRANCH_TILE=(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+)')
    $openingRoomDrawMaterialBranchScroll = [regex]::Match($gdbOutput, 'OPENING_ROOM_DRAW_MATERIAL_BRANCH_SCROLL=(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+)')
    $openingRoomDrawMaterialBranchLoad = [regex]::Match($gdbOutput, 'OPENING_ROOM_DRAW_MATERIAL_BRANCH_LOAD=([0-9]+),([0-9]+)')
    $openingRoomDrawMaterialEmit = [regex]::Match($gdbOutput, 'OPENING_ROOM_DRAW_MATERIAL_EMIT=(0x[0-9a-fA-F]+|0),([0-9]+),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+)')
    $openingRoomDrawMaterialEmitHeap = [regex]::Match($gdbOutput, 'OPENING_ROOM_DRAW_MATERIAL_EMIT_HEAP=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $openingRoomDrawMaterialEmitOps = [regex]::Match($gdbOutput, 'OPENING_ROOM_DRAW_MATERIAL_EMIT_OPS=([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $openingRoomDrawMaterialEmitW0 = [regex]::Match($gdbOutput, 'OPENING_ROOM_DRAW_MATERIAL_EMIT_W0=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $openingRoomDrawMaterialEmitW1 = [regex]::Match($gdbOutput, 'OPENING_ROOM_DRAW_MATERIAL_EMIT_W1=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $openingRoomDLPreview = [regex]::Match($gdbOutput, 'OPENING_ROOM_DL_PREVIEW=(0x[0-9a-fA-F]+)')
    $openingRoomDLPreviewBlocker = [regex]::Match($gdbOutput, 'OPENING_ROOM_DL_PREVIEW_BLOCKER=([0-9]+)')
    $openingRoomDLPreviewCommands = [regex]::Match($gdbOutput, 'OPENING_ROOM_DL_PREVIEW_COMMANDS=([0-9]+)')
    $openingRoomDLPreviewVertices = [regex]::Match($gdbOutput, 'OPENING_ROOM_DL_PREVIEW_VERTICES=([0-9]+)')
    $openingRoomDLPreviewTriangles = [regex]::Match($gdbOutput, 'OPENING_ROOM_DL_PREVIEW_TRIANGLES=([0-9]+)')
    $openingRoomDLPreviewPixels = [regex]::Match($gdbOutput, 'OPENING_ROOM_DL_PREVIEW_PIXELS=([0-9]+)')
    $openingRoomDLPreviewFirstOpcode = [regex]::Match($gdbOutput, 'OPENING_ROOM_DL_PREVIEW_FIRST_OPCODE=(0x[0-9a-fA-F]+)')
    $openingRoomDLPreviewUnsupported = [regex]::Match($gdbOutput, 'OPENING_ROOM_DL_PREVIEW_UNSUPPORTED=(0x[0-9a-fA-F]+|0)')
    $openingRoomDLPreviewCommandShape = [regex]::Match($gdbOutput, 'OPENING_ROOM_DL_PREVIEW_CMD_SHAPE=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $openingRoomDLPreviewBranch = [regex]::Match($gdbOutput, 'OPENING_ROOM_DL_PREVIEW_BRANCH=([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0)')
    $openingRoomDLPreviewFirstDL = [regex]::Match($gdbOutput, 'OPENING_ROOM_DL_PREVIEW_FIRST_DL=(0x[0-9a-fA-F]+)')
    $openingRoomDLPreviewTransform = [regex]::Match($gdbOutput, 'OPENING_ROOM_DL_PREVIEW_TRANSFORM=(0x[0-9a-fA-F]+)')
    $openingRoomDLPreviewXObjs = [regex]::Match($gdbOutput, 'OPENING_ROOM_DL_PREVIEW_XOBJS=([0-9]+)')
    $openingRoomDLPreviewXObjKind = [regex]::Match($gdbOutput, 'OPENING_ROOM_DL_PREVIEW_XOBJ_KIND=([0-9]+)')
    $openingRoomDLPreviewTranslate = [regex]::Match($gdbOutput, 'OPENING_ROOM_DL_PREVIEW_TRANSLATE=(-?[0-9]+),(-?[0-9]+),(-?[0-9]+)')
    $openingRoomDLPreviewRotate = [regex]::Match($gdbOutput, 'OPENING_ROOM_DL_PREVIEW_ROTATE=(-?[0-9]+),(-?[0-9]+),(-?[0-9]+)')
    $openingRoomDLPreviewScale = [regex]::Match($gdbOutput, 'OPENING_ROOM_DL_PREVIEW_SCALE=(-?[0-9]+),(-?[0-9]+),(-?[0-9]+)')
    $openingRoomDLPreviewBounds = [regex]::Match($gdbOutput, 'OPENING_ROOM_DL_PREVIEW_BOUNDS=(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+)')
    $openingRoomDLPreviewProjection = [regex]::Match($gdbOutput, 'OPENING_ROOM_DL_PREVIEW_PROJECTION=(0x[0-9a-fA-F]+),([0-9]+),([0-9]+)')
    $openingRoomDLPreviewProjected = [regex]::Match($gdbOutput, 'OPENING_ROOM_DL_PREVIEW_PROJECTED=([0-9]+),([0-9]+)')
    $openingRoomDLPreviewProjectedBounds = [regex]::Match($gdbOutput, 'OPENING_ROOM_DL_PREVIEW_PROJECTED_BOUNDS=(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+)')
    $openingRoomDLPreviewProjectedDepth = [regex]::Match($gdbOutput, 'OPENING_ROOM_DL_PREVIEW_PROJECTED_DEPTH=(-?[0-9]+),(-?[0-9]+)')
    $openingRoomDLPreviewGeometry = [regex]::Match($gdbOutput, 'OPENING_ROOM_DL_PREVIEW_GEOMETRY=([0-9]+),(0x[0-9a-fA-F]+),(0x[0-9a-fA-F]+),(0x[0-9a-fA-F]+),(0x[0-9a-fA-F]+)')
    $openingRoomDLPreviewGeometryTris = [regex]::Match($gdbOutput, 'OPENING_ROOM_DL_PREVIEW_GEOMETRY_TRIS=([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $openingRoomDLPreviewFallback = [regex]::Match($gdbOutput, 'OPENING_ROOM_DL_PREVIEW_FALLBACK=([0-9]+),([0-9]+)')
    $openingRoomDLPreviewRenderer = [regex]::Match($gdbOutput, 'OPENING_ROOM_DL_PREVIEW_RENDERER=([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $openingRoomDLPreviewRendererTex = [regex]::Match($gdbOutput, 'OPENING_ROOM_DL_PREVIEW_RENDERER_TEX=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0)')
    $openingRoomDLPreviewRendererTile = [regex]::Match($gdbOutput, 'OPENING_ROOM_DL_PREVIEW_RENDERER_TILE=([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+)')
    $openingRoomDLPreviewPresent = [regex]::Match($gdbOutput, 'OPENING_ROOM_DL_PREVIEW_PRESENT=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $openingRoomDLPreviewTexMask = [regex]::Match($gdbOutput, 'OPENING_ROOM_DL_PREVIEW_TEX_MASK=(0x[0-9a-fA-F]+)')
    $openingRoomDLPreviewTexImage = [regex]::Match($gdbOutput, 'OPENING_ROOM_DL_PREVIEW_TEX_IMAGE=(0x[0-9a-fA-F]+)')
    $openingRoomDLPreviewTexFormat = [regex]::Match($gdbOutput, 'OPENING_ROOM_DL_PREVIEW_TEX_FMT=([0-9]+)')
    $openingRoomDLPreviewTexSize = [regex]::Match($gdbOutput, 'OPENING_ROOM_DL_PREVIEW_TEX_SIZE=([0-9]+)')
    $openingRoomDLPreviewTexImageWidth = [regex]::Match($gdbOutput, 'OPENING_ROOM_DL_PREVIEW_TEX_IMAGE_W=([0-9]+)')
    $openingRoomDLPreviewTexTile = [regex]::Match($gdbOutput, 'OPENING_ROOM_DL_PREVIEW_TEX_TILE=([0-9]+),([0-9]+)')
    $openingRoomDLPreviewTexTexels = [regex]::Match($gdbOutput, 'OPENING_ROOM_DL_PREVIEW_TEX_TEXELS=([0-9]+),([0-9]+)')
    $openingRoomDLPreviewTexReady = [regex]::Match($gdbOutput, 'OPENING_ROOM_DL_PREVIEW_TEX_READY=([0-9]+),([0-9]+)')
    $openingRoomDLPreviewTexLoad = [regex]::Match($gdbOutput, 'OPENING_ROOM_DL_PREVIEW_TEX_LOAD=([0-9]+)')
    $openingRoomDLPreviewTexLoadBlock = [regex]::Match($gdbOutput, 'OPENING_ROOM_DL_PREVIEW_TEX_LOADBLOCK=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $openingRoomDLPreviewTexTileSizeRaw = [regex]::Match($gdbOutput, 'OPENING_ROOM_DL_PREVIEW_TEX_TILESIZE_RAW=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $openingRoomDLPreviewTexSamples = [regex]::Match($gdbOutput, 'OPENING_ROOM_DL_PREVIEW_TEX_SAMPLES=([0-9]+)')
    $openingRoomDLPreviewTexSetTile = [regex]::Match($gdbOutput, 'OPENING_ROOM_DL_PREVIEW_TEX_SETTILE=([0-9]+)')
    $openingRoomDLPreviewTexCombine = [regex]::Match($gdbOutput, 'OPENING_ROOM_DL_PREVIEW_TEX_COMBINE=(0x[0-9a-fA-F]+),(0x[0-9a-fA-F]+)')
    $openingRoomDLPreviewTexCombineMode = [regex]::Match($gdbOutput, 'OPENING_ROOM_DL_PREVIEW_TEX_COMBINE_MODE=([0-9]+),(0x[0-9a-fA-F]+),([0-9]+)')
    $openingRoomDLPreviewTexRenderTile = [regex]::Match($gdbOutput, 'OPENING_ROOM_DL_PREVIEW_TEX_RENDERTILE=([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $openingRoomDLPreviewTexTileMode = [regex]::Match($gdbOutput, 'OPENING_ROOM_DL_PREVIEW_TEX_TILEMODE=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+)')
    $openingRoomDLPreviewTexTexture = [regex]::Match($gdbOutput, 'OPENING_ROOM_DL_PREVIEW_TEX_TEXTURE=([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+)')
    $openingRoomDLPreviewTexTextureMode = [regex]::Match($gdbOutput, 'OPENING_ROOM_DL_PREVIEW_TEX_TEXTURE_MODE=([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $openingRoomDLPreviewMaterial = [regex]::Match($gdbOutput, 'OPENING_ROOM_DL_PREVIEW_MATERIAL=([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $openingRoomDLPreviewMaterialIds = [regex]::Match($gdbOutput, 'OPENING_ROOM_DL_PREVIEW_MATERIAL_IDS=([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+)')
    $openingRoomDLPreviewMaterialFormat = [regex]::Match($gdbOutput, 'OPENING_ROOM_DL_PREVIEW_MATERIAL_FORMAT=([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $openingRoomDLPreviewMaterialTile = [regex]::Match($gdbOutput, 'OPENING_ROOM_DL_PREVIEW_MATERIAL_TILE=([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $openingRoomDLPreviewMaterialST = [regex]::Match($gdbOutput, 'OPENING_ROOM_DL_PREVIEW_MATERIAL_ST=(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+)')
    $openingRoomDLPreviewMaterialPtrs = [regex]::Match($gdbOutput, 'OPENING_ROOM_DL_PREVIEW_MATERIAL_PTRS=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $openingRoomDLPreviewMaterialBranch = [regex]::Match($gdbOutput, 'OPENING_ROOM_DL_PREVIEW_MATERIAL_BRANCH=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $openingRoomDLPreviewMaterialBranchOps = [regex]::Match($gdbOutput, 'OPENING_ROOM_DL_PREVIEW_MATERIAL_BRANCH_OPS=([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0)')
    $openingRoomDLPreviewMaterialBranchPrim = [regex]::Match($gdbOutput, 'OPENING_ROOM_DL_PREVIEW_MATERIAL_BRANCH_PRIM=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+)')
    $openingRoomMaterialDLProbe = [regex]::Match($gdbOutput, 'OPENING_ROOM_MATERIAL_DL_PROBE=(0x[0-9a-fA-F]+|0),([0-9]+),(0x[0-9a-fA-F]+|0)')
    $openingRoomMaterialDLProbeShape = [regex]::Match($gdbOutput, 'OPENING_ROOM_MATERIAL_DL_PROBE_SHAPE=([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $openingRoomMaterialDLProbeCmds = [regex]::Match($gdbOutput, 'OPENING_ROOM_MATERIAL_DL_PROBE_CMDS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $openingRoomMaterialDLProbeOps = [regex]::Match($gdbOutput, 'OPENING_ROOM_MATERIAL_DL_PROBE_OPS=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $openingRoomMaterialDLExpand = [regex]::Match($gdbOutput, 'OPENING_ROOM_MATERIAL_DL_EXPAND=(0x[0-9a-fA-F]+|0),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $openingRoomMaterialDLExpandShape = [regex]::Match($gdbOutput, 'OPENING_ROOM_MATERIAL_DL_EXPAND_SHAPE=([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $openingRoomMaterialDLExpandCmds = [regex]::Match($gdbOutput, 'OPENING_ROOM_MATERIAL_DL_EXPAND_CMDS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $openingRoomMaterialDLExpandOps = [regex]::Match($gdbOutput, 'OPENING_ROOM_MATERIAL_DL_EXPAND_OPS=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+)')
    $openingRoomSpotlightAsset = [regex]::Match($gdbOutput, 'OPENING_ROOM_SPOTLIGHT_ASSET=(0x[0-9a-fA-F]+)')
    $openingRoomSpotlightDLOffset = [regex]::Match($gdbOutput, 'OPENING_ROOM_SPOTLIGHT_DL_OFFSET=([0-9]+)')
    $openingRoomSpotlightMObjOffset = [regex]::Match($gdbOutput, 'OPENING_ROOM_SPOTLIGHT_MOBJ_OFFSET=([0-9]+)')
    $openingRoomSpotlightMatAnimOffset = [regex]::Match($gdbOutput, 'OPENING_ROOM_SPOTLIGHT_MATANIM_OFFSET=([0-9]+)')
    $openingRoomSpotlightCreate = [regex]::Match($gdbOutput, 'OPENING_ROOM_SPOTLIGHT_CREATE=(0x[0-9a-fA-F]+)')
    $openingRoomSpotlightCreateMask = [regex]::Match($gdbOutput, 'OPENING_ROOM_SPOTLIGHT_CREATE_MASK=(0x[0-9a-fA-F]+)')
    $openingRoomSpotlightCreateTick = [regex]::Match($gdbOutput, 'OPENING_ROOM_SPOTLIGHT_CREATE_TICK=([0-9]+)')
    $openingRoomSpotlightCreateGObjs = [regex]::Match($gdbOutput, 'OPENING_ROOM_SPOTLIGHT_CREATE_GOBJS=([0-9]+)')
    $openingRoomSpotlightGObjDelta = [regex]::Match($gdbOutput, 'OPENING_ROOM_SPOTLIGHT_GOBJ_DELTA=([0-9]+)')
    $openingRoomSpotlightDObjDelta = [regex]::Match($gdbOutput, 'OPENING_ROOM_SPOTLIGHT_DOBJ_DELTA=([0-9]+)')
    $openingRoomSpotlightXObjDelta = [regex]::Match($gdbOutput, 'OPENING_ROOM_SPOTLIGHT_XOBJ_DELTA=([0-9]+)')
    $openingRoomSpotlightMObjDelta = [regex]::Match($gdbOutput, 'OPENING_ROOM_SPOTLIGHT_MOBJ_DELTA=([0-9]+)')
    $openingRoomSpotlightAObjDelta = [regex]::Match($gdbOutput, 'OPENING_ROOM_SPOTLIGHT_AOBJ_DELTA=([0-9]+)')
    $openingRoomSpotlightDisplay = [regex]::Match($gdbOutput, 'OPENING_ROOM_SPOTLIGHT_DISPLAY=([0-9]+)')
    $openingRoomSpotlightProcess = [regex]::Match($gdbOutput, 'OPENING_ROOM_SPOTLIGHT_PROCESS=([0-9]+)')
    $openingRoomSpotlightMObj = [regex]::Match($gdbOutput, 'OPENING_ROOM_SPOTLIGHT_MOBJ=([0-9]+)')
    $openingRoomSpotlightMatAnim = [regex]::Match($gdbOutput, 'OPENING_ROOM_SPOTLIGHT_MATANIM=([0-9]+)')
    $openingRoomSpotlightPosition = [regex]::Match($gdbOutput, 'OPENING_ROOM_SPOTLIGHT_POSITION=([0-9]+)')
    $openingRoomOverlayCreate = [regex]::Match($gdbOutput, 'OPENING_ROOM_OVERLAY_CREATE=(0x[0-9a-fA-F]+)')
    $openingRoomOverlayDisplay = [regex]::Match($gdbOutput, 'OPENING_ROOM_OVERLAY_DISPLAY=([0-9]+)')
    $openingRoomOverlayAlpha = [regex]::Match($gdbOutput, 'OPENING_ROOM_OVERLAY_ALPHA=([0-9]+)')
    $openingRoomOverlayCreateGObjs = [regex]::Match($gdbOutput, 'OPENING_ROOM_OVERLAY_CREATE_GOBJS=([0-9]+)')
    $openingRoomOverlayEject = [regex]::Match($gdbOutput, 'OPENING_ROOM_OVERLAY_EJECT=(0x[0-9a-fA-F]+)')
    $openingRoomOverlayEjectBefore = [regex]::Match($gdbOutput, 'OPENING_ROOM_OVERLAY_EJECT_BEFORE=([0-9]+)')
    $openingRoomOverlayEjectAfter = [regex]::Match($gdbOutput, 'OPENING_ROOM_OVERLAY_EJECT_AFTER=([0-9]+)')
    $openingRoomOverlayEjectUnlinked = [regex]::Match($gdbOutput, 'OPENING_ROOM_OVERLAY_EJECT_UNLINKED=(0x[0-9a-fA-F]+)')
    $openingRoomScene1CameraCreate = [regex]::Match($gdbOutput, 'OPENING_ROOM_SCENE1_CAMERA_CREATE=(0x[0-9a-fA-F]+)')
    $openingRoomScene1CameraCreateMask = [regex]::Match($gdbOutput, 'OPENING_ROOM_SCENE1_CAMERA_CREATE_MASK=(0x[0-9a-fA-F]+)')
    $openingRoomScene1CameraCreateGObjs = [regex]::Match($gdbOutput, 'OPENING_ROOM_SCENE1_CAMERA_CREATE_GOBJS=([0-9]+)')
    $openingRoomScene1CameraGObjDelta = [regex]::Match($gdbOutput, 'OPENING_ROOM_SCENE1_CAMERA_GOBJ_DELTA=([0-9]+)')
    $openingRoomScene1CameraCObjDelta = [regex]::Match($gdbOutput, 'OPENING_ROOM_SCENE1_CAMERA_COBJ_DELTA=([0-9]+)')
    $openingRoomScene1CameraXObjDelta = [regex]::Match($gdbOutput, 'OPENING_ROOM_SCENE1_CAMERA_XOBJ_DELTA=([0-9]+)')
    $openingRoomScene1CameraAObjDelta = [regex]::Match($gdbOutput, 'OPENING_ROOM_SCENE1_CAMERA_AOBJ_DELTA=([0-9]+)')
    $openingRoomScene1CameraDisplay = [regex]::Match($gdbOutput, 'OPENING_ROOM_SCENE1_CAMERA_DISPLAY=([0-9]+)')
    $openingRoomScene1CameraProcess = [regex]::Match($gdbOutput, 'OPENING_ROOM_SCENE1_CAMERA_PROCESS=([0-9]+)')
    $openingRoomScene1CameraAnim = [regex]::Match($gdbOutput, 'OPENING_ROOM_SCENE1_CAMERA_ANIM=([0-9]+)')
    $openingRoomScene1CameraViewport = [regex]::Match($gdbOutput, 'OPENING_ROOM_SCENE1_CAMERA_VIEWPORT=([0-9]+)')
    $openingRoomScene1CameraDLBuffer = [regex]::Match($gdbOutput, 'OPENING_ROOM_SCENE1_CAMERA_DLBUFFER=([0-9]+)')
    $openingRoomScene2CameraAsset = [regex]::Match($gdbOutput, 'OPENING_ROOM_SCENE2_CAMERA_ASSET=(0x[0-9a-fA-F]+)')
    $openingRoomScene2CameraAnimOffset = [regex]::Match($gdbOutput, 'OPENING_ROOM_SCENE2_CAMERA_ANIM_OFFSET=([0-9]+)')
    $openingRoomScene2CameraEject = [regex]::Match($gdbOutput, 'OPENING_ROOM_SCENE2_CAMERA_EJECT=(0x[0-9a-fA-F]+)')
    $openingRoomScene2CameraEjectMask = [regex]::Match($gdbOutput, 'OPENING_ROOM_SCENE2_CAMERA_EJECT_MASK=(0x[0-9a-fA-F]+)')
    $openingRoomScene2CameraEjectBeforeGObjs = [regex]::Match($gdbOutput, 'OPENING_ROOM_SCENE2_CAMERA_EJECT_BEFORE_GOBJS=([0-9]+)')
    $openingRoomScene2CameraEjectAfterGObjs = [regex]::Match($gdbOutput, 'OPENING_ROOM_SCENE2_CAMERA_EJECT_AFTER_GOBJS=([0-9]+)')
    $openingRoomScene2CameraEjectBeforeCameras = [regex]::Match($gdbOutput, 'OPENING_ROOM_SCENE2_CAMERA_EJECT_BEFORE_CAMERAS=([0-9]+)')
    $openingRoomScene2CameraEjectAfterCameras = [regex]::Match($gdbOutput, 'OPENING_ROOM_SCENE2_CAMERA_EJECT_AFTER_CAMERAS=([0-9]+)')
    $openingRoomScene2CameraCreate = [regex]::Match($gdbOutput, 'OPENING_ROOM_SCENE2_CAMERA_CREATE=(0x[0-9a-fA-F]+)')
    $openingRoomScene2CameraCreateMask = [regex]::Match($gdbOutput, 'OPENING_ROOM_SCENE2_CAMERA_CREATE_MASK=(0x[0-9a-fA-F]+)')
    $openingRoomScene2CameraCreateGObjs = [regex]::Match($gdbOutput, 'OPENING_ROOM_SCENE2_CAMERA_CREATE_GOBJS=([0-9]+)')
    $openingRoomScene2CameraGObjDelta = [regex]::Match($gdbOutput, 'OPENING_ROOM_SCENE2_CAMERA_GOBJ_DELTA=([0-9]+)')
    $openingRoomScene2CameraCObjDelta = [regex]::Match($gdbOutput, 'OPENING_ROOM_SCENE2_CAMERA_COBJ_DELTA=([0-9]+)')
    $openingRoomScene2CameraXObjDelta = [regex]::Match($gdbOutput, 'OPENING_ROOM_SCENE2_CAMERA_XOBJ_DELTA=([0-9]+)')
    $openingRoomScene2CameraAObjDelta = [regex]::Match($gdbOutput, 'OPENING_ROOM_SCENE2_CAMERA_AOBJ_DELTA=([0-9]+)')
    $openingRoomScene2CameraDisplay = [regex]::Match($gdbOutput, 'OPENING_ROOM_SCENE2_CAMERA_DISPLAY=([0-9]+)')
    $openingRoomScene2CameraProcess = [regex]::Match($gdbOutput, 'OPENING_ROOM_SCENE2_CAMERA_PROCESS=([0-9]+)')
    $openingRoomScene2CameraAnim = [regex]::Match($gdbOutput, 'OPENING_ROOM_SCENE2_CAMERA_ANIM=([0-9]+)')
    $openingRoomScene2CameraViewport = [regex]::Match($gdbOutput, 'OPENING_ROOM_SCENE2_CAMERA_VIEWPORT=([0-9]+)')
    $openingRoomScene2CameraDLBuffer = [regex]::Match($gdbOutput, 'OPENING_ROOM_SCENE2_CAMERA_DLBUFFER=([0-9]+)')
    $openingRoomCloseUpCameraCreate = [regex]::Match($gdbOutput, 'OPENING_ROOM_CLOSEUP_CAMERA_CREATE=(0x[0-9a-fA-F]+)')
    $openingRoomCloseUpCameraCreateMask = [regex]::Match($gdbOutput, 'OPENING_ROOM_CLOSEUP_CAMERA_CREATE_MASK=(0x[0-9a-fA-F]+)')
    $openingRoomCloseUpCameraCreateGObjs = [regex]::Match($gdbOutput, 'OPENING_ROOM_CLOSEUP_CAMERA_CREATE_GOBJS=([0-9]+)')
    $openingRoomCloseUpCameraGObjDelta = [regex]::Match($gdbOutput, 'OPENING_ROOM_CLOSEUP_CAMERA_GOBJ_DELTA=([0-9]+)')
    $openingRoomCloseUpCameraCObjDelta = [regex]::Match($gdbOutput, 'OPENING_ROOM_CLOSEUP_CAMERA_COBJ_DELTA=([0-9]+)')
    $openingRoomCloseUpCameraXObjDelta = [regex]::Match($gdbOutput, 'OPENING_ROOM_CLOSEUP_CAMERA_XOBJ_DELTA=([0-9]+)')
    $openingRoomCloseUpCameraDisplay = [regex]::Match($gdbOutput, 'OPENING_ROOM_CLOSEUP_CAMERA_DISPLAY=([0-9]+)')
    $openingRoomCloseUpCameraViewport = [regex]::Match($gdbOutput, 'OPENING_ROOM_CLOSEUP_CAMERA_VIEWPORT=([0-9]+)')
    $openingRoomWallpaperCameraCreate = [regex]::Match($gdbOutput, 'OPENING_ROOM_WALLPAPER_CAMERA_CREATE=(0x[0-9a-fA-F]+)')
    $openingRoomWallpaperCameraCreateMask = [regex]::Match($gdbOutput, 'OPENING_ROOM_WALLPAPER_CAMERA_CREATE_MASK=(0x[0-9a-fA-F]+)')
    $openingRoomWallpaperCameraCreateGObjs = [regex]::Match($gdbOutput, 'OPENING_ROOM_WALLPAPER_CAMERA_CREATE_GOBJS=([0-9]+)')
    $openingRoomWallpaperCameraGObjDelta = [regex]::Match($gdbOutput, 'OPENING_ROOM_WALLPAPER_CAMERA_GOBJ_DELTA=([0-9]+)')
    $openingRoomWallpaperCameraCObjDelta = [regex]::Match($gdbOutput, 'OPENING_ROOM_WALLPAPER_CAMERA_COBJ_DELTA=([0-9]+)')
    $openingRoomWallpaperCameraXObjDelta = [regex]::Match($gdbOutput, 'OPENING_ROOM_WALLPAPER_CAMERA_XOBJ_DELTA=([0-9]+)')
    $openingRoomWallpaperCameraDisplay = [regex]::Match($gdbOutput, 'OPENING_ROOM_WALLPAPER_CAMERA_DISPLAY=([0-9]+)')
    $openingRoomWallpaperCameraViewport = [regex]::Match($gdbOutput, 'OPENING_ROOM_WALLPAPER_CAMERA_VIEWPORT=([0-9]+)')
    $openingRoomLogoCameraAsset = [regex]::Match($gdbOutput, 'OPENING_ROOM_LOGO_CAMERA_ASSET=(0x[0-9a-fA-F]+)')
    $openingRoomLogoCameraAnimOffset = [regex]::Match($gdbOutput, 'OPENING_ROOM_LOGO_CAMERA_ANIM_OFFSET=([0-9]+)')
    $openingRoomLogoCameraCreate = [regex]::Match($gdbOutput, 'OPENING_ROOM_LOGO_CAMERA_CREATE=(0x[0-9a-fA-F]+)')
    $openingRoomLogoCameraCreateMask = [regex]::Match($gdbOutput, 'OPENING_ROOM_LOGO_CAMERA_CREATE_MASK=(0x[0-9a-fA-F]+)')
    $openingRoomLogoCameraCreateGObjs = [regex]::Match($gdbOutput, 'OPENING_ROOM_LOGO_CAMERA_CREATE_GOBJS=([0-9]+)')
    $openingRoomLogoCameraGObjDelta = [regex]::Match($gdbOutput, 'OPENING_ROOM_LOGO_CAMERA_GOBJ_DELTA=([0-9]+)')
    $openingRoomLogoCameraCObjDelta = [regex]::Match($gdbOutput, 'OPENING_ROOM_LOGO_CAMERA_COBJ_DELTA=([0-9]+)')
    $openingRoomLogoCameraXObjDelta = [regex]::Match($gdbOutput, 'OPENING_ROOM_LOGO_CAMERA_XOBJ_DELTA=([0-9]+)')
    $openingRoomLogoCameraAObjDelta = [regex]::Match($gdbOutput, 'OPENING_ROOM_LOGO_CAMERA_AOBJ_DELTA=([0-9]+)')
    $openingRoomLogoCameraDisplay = [regex]::Match($gdbOutput, 'OPENING_ROOM_LOGO_CAMERA_DISPLAY=([0-9]+)')
    $openingRoomLogoCameraProcess = [regex]::Match($gdbOutput, 'OPENING_ROOM_LOGO_CAMERA_PROCESS=([0-9]+)')
    $openingRoomLogoCameraAnim = [regex]::Match($gdbOutput, 'OPENING_ROOM_LOGO_CAMERA_ANIM=([0-9]+)')
    $openingRoomLogoCameraViewport = [regex]::Match($gdbOutput, 'OPENING_ROOM_LOGO_CAMERA_VIEWPORT=([0-9]+)')
    $openingRoomLogoAsset = [regex]::Match($gdbOutput, 'OPENING_ROOM_LOGO_ASSET=(0x[0-9a-fA-F]+)')
    $openingRoomLogoDObjOffset = [regex]::Match($gdbOutput, 'OPENING_ROOM_LOGO_DOBJ_OFFSET=([0-9]+)')
    $openingRoomLogoMObjOffset = [regex]::Match($gdbOutput, 'OPENING_ROOM_LOGO_MOBJ_OFFSET=([0-9]+)')
    $openingRoomLogoMatAnimOffset = [regex]::Match($gdbOutput, 'OPENING_ROOM_LOGO_MATANIM_OFFSET=([0-9]+)')
    $openingRoomLogoCreate = [regex]::Match($gdbOutput, 'OPENING_ROOM_LOGO_CREATE=(0x[0-9a-fA-F]+)')
    $openingRoomLogoCreateMask = [regex]::Match($gdbOutput, 'OPENING_ROOM_LOGO_CREATE_MASK=(0x[0-9a-fA-F]+)')
    $openingRoomLogoCreateGObjs = [regex]::Match($gdbOutput, 'OPENING_ROOM_LOGO_CREATE_GOBJS=([0-9]+)')
    $openingRoomLogoGObjDelta = [regex]::Match($gdbOutput, 'OPENING_ROOM_LOGO_GOBJ_DELTA=([0-9]+)')
    $openingRoomLogoDObjDelta = [regex]::Match($gdbOutput, 'OPENING_ROOM_LOGO_DOBJ_DELTA=([0-9]+)')
    $openingRoomLogoXObjDelta = [regex]::Match($gdbOutput, 'OPENING_ROOM_LOGO_XOBJ_DELTA=([0-9]+)')
    $openingRoomLogoMObjDelta = [regex]::Match($gdbOutput, 'OPENING_ROOM_LOGO_MOBJ_DELTA=([0-9]+)')
    $openingRoomLogoAObjDelta = [regex]::Match($gdbOutput, 'OPENING_ROOM_LOGO_AOBJ_DELTA=([0-9]+)')
    $openingRoomLogoDisplay = [regex]::Match($gdbOutput, 'OPENING_ROOM_LOGO_DISPLAY=([0-9]+)')
    $openingRoomLogoMObj = [regex]::Match($gdbOutput, 'OPENING_ROOM_LOGO_MOBJ=([0-9]+)')
    $openingRoomLogoMatAnim = [regex]::Match($gdbOutput, 'OPENING_ROOM_LOGO_MATANIM=([0-9]+)')
    $openingRoomLogoEject = [regex]::Match($gdbOutput, 'OPENING_ROOM_LOGO_EJECT=(0x[0-9a-fA-F]+)')
    $openingRoomLogoEjectBefore = [regex]::Match($gdbOutput, 'OPENING_ROOM_LOGO_EJECT_BEFORE=([0-9]+)')
    $openingRoomLogoEjectAfter = [regex]::Match($gdbOutput, 'OPENING_ROOM_LOGO_EJECT_AFTER=([0-9]+)')
    $openingRoomLogoEjectUnlinked = [regex]::Match($gdbOutput, 'OPENING_ROOM_LOGO_EJECT_UNLINKED=(0x[0-9a-fA-F]+)')
    $openingRoomBossShadowAsset = [regex]::Match($gdbOutput, 'OPENING_ROOM_BOSS_SHADOW_ASSET=(0x[0-9a-fA-F]+)')
    $openingRoomBossShadowDLOffset = [regex]::Match($gdbOutput, 'OPENING_ROOM_BOSS_SHADOW_DL_OFFSET=([0-9]+)')
    $openingRoomBossShadowAnimOffset = [regex]::Match($gdbOutput, 'OPENING_ROOM_BOSS_SHADOW_ANIM_OFFSET=([0-9]+)')
    $openingRoomBossShadowCreate = [regex]::Match($gdbOutput, 'OPENING_ROOM_BOSS_SHADOW_CREATE=(0x[0-9a-fA-F]+)')
    $openingRoomBossShadowCreateMask = [regex]::Match($gdbOutput, 'OPENING_ROOM_BOSS_SHADOW_CREATE_MASK=(0x[0-9a-fA-F]+)')
    $openingRoomBossShadowCreateGObjs = [regex]::Match($gdbOutput, 'OPENING_ROOM_BOSS_SHADOW_CREATE_GOBJS=([0-9]+)')
    $openingRoomBossShadowGObjDelta = [regex]::Match($gdbOutput, 'OPENING_ROOM_BOSS_SHADOW_GOBJ_DELTA=([0-9]+)')
    $openingRoomBossShadowDObjDelta = [regex]::Match($gdbOutput, 'OPENING_ROOM_BOSS_SHADOW_DOBJ_DELTA=([0-9]+)')
    $openingRoomBossShadowXObjDelta = [regex]::Match($gdbOutput, 'OPENING_ROOM_BOSS_SHADOW_XOBJ_DELTA=([0-9]+)')
    $openingRoomBossShadowAObjDelta = [regex]::Match($gdbOutput, 'OPENING_ROOM_BOSS_SHADOW_AOBJ_DELTA=([0-9]+)')
    $openingRoomBossShadowProcess = [regex]::Match($gdbOutput, 'OPENING_ROOM_BOSS_SHADOW_PROCESS=([0-9]+)')
    $openingRoomBossShadowDisplay = [regex]::Match($gdbOutput, 'OPENING_ROOM_BOSS_SHADOW_DISPLAY=([0-9]+)')
    $openingRoomBossShadowAnim = [regex]::Match($gdbOutput, 'OPENING_ROOM_BOSS_SHADOW_ANIM=([0-9]+)')
    $openingRoomBossShadowEject = [regex]::Match($gdbOutput, 'OPENING_ROOM_BOSS_SHADOW_EJECT=(0x[0-9a-fA-F]+)')
    $openingRoomBossShadowEjectBefore = [regex]::Match($gdbOutput, 'OPENING_ROOM_BOSS_SHADOW_EJECT_BEFORE=([0-9]+)')
    $openingRoomBossShadowEjectAfter = [regex]::Match($gdbOutput, 'OPENING_ROOM_BOSS_SHADOW_EJECT_AFTER=([0-9]+)')
    $openingRoomBossShadowEjectUnlinked = [regex]::Match($gdbOutput, 'OPENING_ROOM_BOSS_SHADOW_EJECT_UNLINKED=(0x[0-9a-fA-F]+)')
    $openingRoomPencilsCreate = [regex]::Match($gdbOutput, 'OPENING_ROOM_PENCILS_CREATE=(0x[0-9a-fA-F]+)')
    $openingRoomPencilsMask = [regex]::Match($gdbOutput, 'OPENING_ROOM_PENCILS_MASK=(0x[0-9a-fA-F]+)')
    $openingRoomPencilsGObjDelta = [regex]::Match($gdbOutput, 'OPENING_ROOM_PENCILS_GOBJ_DELTA=([0-9]+)')
    $openingRoomPencilsDObjDelta = [regex]::Match($gdbOutput, 'OPENING_ROOM_PENCILS_DOBJ_DELTA=([0-9]+)')
    $openingRoomPencilsXObjDelta = [regex]::Match($gdbOutput, 'OPENING_ROOM_PENCILS_XOBJ_DELTA=([0-9]+)')
    $openingRoomPencilsAObjDelta = [regex]::Match($gdbOutput, 'OPENING_ROOM_PENCILS_AOBJ_DELTA=([0-9]+)')
    $openingRoomPencilsProcess = [regex]::Match($gdbOutput, 'OPENING_ROOM_PENCILS_PROCESS=([0-9]+)')
    $openingRoomPencilsDisplay = [regex]::Match($gdbOutput, 'OPENING_ROOM_PENCILS_DISPLAY=([0-9]+)')
    $openingRoomPencilsTree = [regex]::Match($gdbOutput, 'OPENING_ROOM_PENCILS_TREE=([0-9]+)')
    $openingRoomPencilsRoots = [regex]::Match($gdbOutput, 'OPENING_ROOM_PENCILS_ROOTS=([0-9]+)')
    $openingRoomUpdate = [regex]::Match($gdbOutput, 'OPENING_ROOM_UPDATE=(0x[0-9a-fA-F]+)')
    $openingRoomTicks = [regex]::Match($gdbOutput, 'OPENING_ROOM_TICKS=([0-9]+)')
    $openingRoomGObjs = [regex]::Match($gdbOutput, 'OPENING_ROOM_GOBJS=([0-9]+)')
    $openingRoomCameras = [regex]::Match($gdbOutput, 'OPENING_ROOM_CAMERAS=([0-9]+)')
    $openingRoomDL0 = [regex]::Match($gdbOutput, 'OPENING_ROOM_DL0=([0-9]+)')
    $openingRoomDL1 = [regex]::Match($gdbOutput, 'OPENING_ROOM_DL1=([0-9]+)')
    $openingRoomGfxHeap = [regex]::Match($gdbOutput, 'OPENING_ROOM_GFXHEAP=([0-9]+)')
    $openingRoomRdpSize = [regex]::Match($gdbOutput, 'OPENING_ROOM_RDP_SIZE=([0-9]+)')
    $openingRoomMallocs = [regex]::Match($gdbOutput, 'OPENING_ROOM_MALLOCS=([0-9]+)')
    $openingRoomPreAsset = [regex]::Match($gdbOutput, 'OPENING_ROOM_PRE_ASSET=(0x[0-9a-fA-F]+)')
    $openingRoomControllerChecks = [regex]::Match($gdbOutput, 'OPENING_ROOM_CONTROLLER_CHECKS=([0-9]+)')
    $openingRoomPulledKind = [regex]::Match($gdbOutput, 'OPENING_ROOM_PULLED_KIND=([0-9]+)')
    $openingRoomDroppedKind = [regex]::Match($gdbOutput, 'OPENING_ROOM_DROPPED_KIND=([0-9]+)')
    $openingRoomSkipTitle = [regex]::Match($gdbOutput, 'OPENING_ROOM_SKIP_TITLE=([0-9]+)')
    $openingMovieRoomHandoff = [regex]::Match($gdbOutput, 'OPENING_MOVIE_ROOM_HANDOFF=(0x[0-9a-fA-F]+|0)')
    $openingMovieRoomHandoffTick = [regex]::Match($gdbOutput, 'OPENING_MOVIE_ROOM_HANDOFF_TICK=([0-9]+)')
    $openingMovieRoomHandoffScene = [regex]::Match($gdbOutput, 'OPENING_MOVIE_ROOM_HANDOFF_SCENE=([0-9]+)')
    $openingPortraitsDispatch = [regex]::Match($gdbOutput, 'OPENING_PORTRAITS_DISPATCH=([0-9]+)')
    $openingPortraitsStart = [regex]::Match($gdbOutput, 'OPENING_PORTRAITS_START=(0x[0-9a-fA-F]+|0)')
    $openingPortraitsFuncStart = [regex]::Match($gdbOutput, 'OPENING_PORTRAITS_FUNC_START=(0x[0-9a-fA-F]+|0)')
    $openingPortraitsUpdate = [regex]::Match($gdbOutput, 'OPENING_PORTRAITS_UPDATE=(0x[0-9a-fA-F]+|0)')
    $openingPortraitsReloc = [regex]::Match($gdbOutput, 'OPENING_PORTRAITS_RELOC=(0x[0-9a-fA-F]+|0)')
    $openingPortraitsSpriteNorm = [regex]::Match($gdbOutput, 'OPENING_PORTRAITS_SPRITE_NORM=([0-9]+),([0-9]+)')
    $openingPortraitsTicks = [regex]::Match($gdbOutput, 'OPENING_PORTRAITS_TICKS=([0-9]+)')
    $openingPortraitsDraw = [regex]::Match($gdbOutput, 'OPENING_PORTRAITS_DRAW=(0x[0-9a-fA-F]+|0)')
    $openingPortraitsDrawBlocker = [regex]::Match($gdbOutput, 'OPENING_PORTRAITS_DRAW_BLOCKER=([0-9]+)')
    $openingPortraitsDrawCallbacks = [regex]::Match($gdbOutput, 'OPENING_PORTRAITS_DRAW_CALLBACKS=([0-9]+)')
    $openingPortraitsDrawVisible = [regex]::Match($gdbOutput, 'OPENING_PORTRAITS_DRAW_VISIBLE=([0-9]+)')
    $openingPortraitsDrawMeta = [regex]::Match($gdbOutput, 'OPENING_PORTRAITS_DRAW_META=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $openingPortraitsNext = [regex]::Match($gdbOutput, 'OPENING_PORTRAITS_NEXT=(0x[0-9a-fA-F]+|0)')
    $openingPortraitsNextScene = [regex]::Match($gdbOutput, 'OPENING_PORTRAITS_NEXT_SCENE=([0-9]+)')
    $openingMarioDispatch = [regex]::Match($gdbOutput, 'OPENING_MARIO_DISPATCH=([0-9]+)')
    $openingMarioStart = [regex]::Match($gdbOutput, 'OPENING_MARIO_START=(0x[0-9a-fA-F]+|0)')
    $openingMarioFuncStart = [regex]::Match($gdbOutput, 'OPENING_MARIO_FUNC_START=(0x[0-9a-fA-F]+|0)')
    $openingMarioUpdate = [regex]::Match($gdbOutput, 'OPENING_MARIO_UPDATE=(0x[0-9a-fA-F]+|0)')
    $openingMarioTicks = [regex]::Match($gdbOutput, 'OPENING_MARIO_TICKS=([0-9]+)')
    $openingMarioReloc = [regex]::Match($gdbOutput, 'OPENING_MARIO_RELOC=(0x[0-9a-fA-F]+|0)')
    $openingMarioSpriteNorm = [regex]::Match($gdbOutput, 'OPENING_MARIO_SPRITE_NORM=([0-9]+),([0-9]+)')
    $openingMarioDraw = [regex]::Match($gdbOutput, 'OPENING_MARIO_DRAW=(0x[0-9a-fA-F]+|0)')
    $openingMarioDrawBlocker = [regex]::Match($gdbOutput, 'OPENING_MARIO_DRAW_BLOCKER=([0-9]+)')
    $openingMarioDrawCallbacks = [regex]::Match($gdbOutput, 'OPENING_MARIO_DRAW_CALLBACKS=([0-9]+)')
    $openingMarioDrawVisible = [regex]::Match($gdbOutput, 'OPENING_MARIO_DRAW_VISIBLE=([0-9]+)')
    $openingMarioDrawMeta = [regex]::Match($gdbOutput, 'OPENING_MARIO_DRAW_META=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $openingMarioFighterDefer = [regex]::Match($gdbOutput, 'OPENING_MARIO_FIGHTER_DEFER=(0x[0-9a-fA-F]+|0)')
    $openingMarioFighterDeferTick = [regex]::Match($gdbOutput, 'OPENING_MARIO_FIGHTER_DEFER_TICK=([0-9]+)')
    $openingMarioNext = [regex]::Match($gdbOutput, 'OPENING_MARIO_NEXT=(0x[0-9a-fA-F]+|0)')
    $openingMarioNextScene = [regex]::Match($gdbOutput, 'OPENING_MARIO_NEXT_SCENE=([0-9]+)')
    $openingNameMasks = [regex]::Match($gdbOutput, 'OPENING_NAME_MASKS=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $openingNameCounts = [regex]::Match($gdbOutput, 'OPENING_NAME_COUNTS=([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $openingNameDraw = [regex]::Match($gdbOutput, 'OPENING_NAME_DRAW=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+)')
    $openingNameDrawMeta = [regex]::Match($gdbOutput, 'OPENING_NAME_DRAW_META=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $openingMovieBridge = [regex]::Match($gdbOutput, 'OPENING_MOVIE_BRIDGE=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+)')
    $openingActionPreview = [regex]::Match($gdbOutput, 'OPENING_ACTION_PREVIEW=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+)')
    $openingMoviePresentFrames = [regex]::Match($gdbOutput, 'OPENING_MOVIE_PRESENT_FRAMES=([0-9]+)')
    $openingActionPreviewNorm = [regex]::Match($gdbOutput, 'OPENING_ACTION_PREVIEW_NORM=([0-9]+),([0-9]+)')
    $openingActionPreviewMeta = [regex]::Match($gdbOutput, 'OPENING_ACTION_PREVIEW_META=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $openingMovieTitle = [regex]::Match($gdbOutput, 'OPENING_MOVIE_TITLE=(0x[0-9a-fA-F]+|0)')
    $titleReloc = [regex]::Match($gdbOutput, 'TITLE_RELOC=(0x[0-9a-fA-F]+|0)')
    $titlePreview = [regex]::Match($gdbOutput, 'TITLE_PREVIEW=(0x[0-9a-fA-F]+|0)')
    $titleDraw = [regex]::Match($gdbOutput, 'TITLE_DRAW=(0x[0-9a-fA-F]+|0)')
    $titleSpriteNorm = [regex]::Match($gdbOutput, 'TITLE_SPRITE_NORM=([0-9]+),([0-9]+)')
    $titleFireSpriteNorm = [regex]::Match($gdbOutput, 'TITLE_FIRE_SPRITE_NORM=([0-9]+),([0-9]+)')
    $titleDrawCounts = [regex]::Match($gdbOutput, 'TITLE_DRAW_COUNTS=([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $titleDrawMeta = [regex]::Match($gdbOutput, 'TITLE_DRAW_META=([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $titleOriginal = [regex]::Match($gdbOutput, 'TITLE_ORIGINAL=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0)')
    $titleLogoFire = [regex]::Match($gdbOutput, 'TITLE_LOGO_FIRE=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $titleFire = [regex]::Match($gdbOutput, 'TITLE_FIRE=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $titleOriginalUpdate = [regex]::Match($gdbOutput, 'TITLE_ORIGINAL_UPDATE=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $perfFps = [regex]::Match($gdbOutput, 'PERF_FPS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $perfContent = [regex]::Match($gdbOutput, 'PERF_CONTENT=([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $frames = [regex]::Match($gdbOutput, 'FRAMES=([0-9]+)')
    if (-not $aobj32.Success -or
        [int]$aobj32.Groups[1].Value -le 0 -or
        [int]$aobj32.Groups[2].Value -le 0 -or
        [int]$aobj32.Groups[4].Value -ne 0 -or
        -not $aobj32Color.Success -or
        [int]$aobj32Color.Groups[1].Value -le 0) {
        throw "Source AObjEvent32 command/color decoding did not remain active and failure-free.`n$gdbOutput"
    }
    if (-not $selfTest.Success -or $selfTest.Groups[1].Value -ne '0x50415353') {
        throw "Queue/thread self-test failed.`n$gdbOutput"
    }
    if (-not $boot.Success -or $boot.Groups[1].Value -ne '0x53430007') {
        throw "Original boot-chain check failed.`n$gdbOutput"
    }
    if (-not $scheduler.Success -or [int]$scheduler.Groups[1].Value -le 0) {
        throw "Original scheduler did not process VBlank.`n$gdbOutput"
    }
    if (-not $controllers.Success -or [int]$controllers.Groups[1].Value -ne 1) {
        throw "Original controller discovery failed.`n$gdbOutput"
    }
    if (-not $controllerPolls.Success -or
        [int]$controllerPolls.Groups[1].Value -le 0) {
        throw "Original controller thread did not poll input.`n$gdbOutput"
    }
    if (-not $video.Success -or $video.Groups[1].Value -ne '0x56494430') {
        throw "Original video task bootstrap failed.`n$gdbOutput"
    }
    if (-not $relocNitro.Success -or
        $relocNitro.Groups[1].Value.ToLowerInvariant() -ne '0x4e465349') {
        throw "NitroFS relocation asset init failed.`n$gdbOutput"
    }
    if (-not $relocHeaders.Success -or [int]$relocHeaders.Groups[1].Value -lt 49) {
        throw "Opening Room relocation O2R headers were not read from NitroFS.`n$gdbOutput"
    }
    if (-not $relocPayloads.Success -or [int]$relocPayloads.Groups[1].Value -lt 33) {
        throw "Startup, Opening movie, and Title relocation O2R payloads were not loaded from NitroFS.`n$gdbOutput"
    }
    if (-not $relocOpenFails.Success -or [int]$relocOpenFails.Groups[1].Value -ne 0 -or`
        -not $relocFormatFails.Success -or [int]$relocFormatFails.Groups[1].Value -ne 0 -or`
        -not $relocShortReads.Success -or [int]$relocShortReads.Groups[1].Value -ne 0) {
        throw "NitroFS relocation asset loader reported a read/format failure.`n$gdbOutput"
    }
    if (-not $scene.Success -or
        $scene.Groups[1].Value.ToLowerInvariant() -ne '0x53434e45') {
        throw "Original scene manager did not reach a scene boundary.`n$gdbOutput"
    }
    if (-not $sceneKind.Success -or [int]$sceneKind.Groups[1].Value -ne 1) {
        throw "Opening movie did not reach the Title scene boundary.`n$gdbOutput"
    }
    if (-not $startup.Success -or
        $startup.Groups[1].Value.ToLowerInvariant() -ne '0x53545254') {
        throw "Original startup scene did not reach task-manager start.`n$gdbOutput"
    }
    if (-not $startupKind.Success -or [int]$startupKind.Groups[1].Value -ne 27) {
        throw "Original startup scene did not preserve scene kind 27.`n$gdbOutput"
    }
    if (-not $startupDL0.Success -or [int]$startupDL0.Groups[1].Value -ne 10240) {
        throw "Original startup task setup DL0 size was not preserved.`n$gdbOutput"
    }
    if (-not $startupDL1.Success -or [int]$startupDL1.Groups[1].Value -ne 10240) {
        throw "Original startup task setup DL1 size was not preserved.`n$gdbOutput"
    }
    if (-not $startupController.Success -or
        [int]$startupController.Groups[1].Value -ne 1) {
        throw "Original startup task setup did not use syControllerFuncRead.`n$gdbOutput"
    }
    if (-not $startupFunc.Success -or
        $startupFunc.Groups[1].Value.ToLowerInvariant() -ne '0x46535452') {
        throw "Original startup func_start did not run.`n$gdbOutput"
    }
    if (-not $startupSkip.Success -or [int]$startupSkip.Groups[1].Value -ne 8) {
        throw "Original startup skip delay was not initialized.`n$gdbOutput"
    }
    if (-not $startupOpening.Success -or
        [int]$startupOpening.Groups[1].Value -ne 0) {
        throw "Original startup opening flag was not initialized false.`n$gdbOutput"
    }
    if (-not $startupGObjs.Success -or [int]$startupGObjs.Groups[1].Value -ne 4) {
        throw "Original startup did not request the expected GObjs.`n$gdbOutput"
    }
    if (-not $startupCameras.Success -or [int]$startupCameras.Groups[1].Value -ne 2) {
        throw "Original startup did not request the expected cameras.`n$gdbOutput"
    }
    if (-not $startupReloc.Success -or [int]$startupReloc.Groups[1].Value -ne 1) {
        throw "Original startup did not initialize relocation once.`n$gdbOutput"
    }
    if (-not $startupSprites.Success -or [int]$startupSprites.Groups[1].Value -ne 1) {
        throw "Original startup did not create the N64 logo sprite object.`n$gdbOutput"
    }
    if (-not $startupFades.Success -or [int]$startupFades.Groups[1].Value -ne 1) {
        throw "Original startup did not request the fade actor.`n$gdbOutput"
    }
    if (-not $startupParent.Success -or [int]$startupParent.Groups[1].Value -ne 1) {
        throw "Startup logo SObj is not attached to the wallpaper GObj.`n$gdbOutput"
    }
    if (-not $startupLogoX.Success -or [int]$startupLogoX.Groups[1].Value -ne 96) {
        throw "Original startup logo X position was not preserved.`n$gdbOutput"
    }
    if (-not $startupLogoY.Success -or [int]$startupLogoY.Groups[1].Value -ne 220) {
        throw "Original startup logo Y position was not preserved.`n$gdbOutput"
    }
    if (-not $startupFastcopy.Success -or [int]$startupFastcopy.Groups[1].Value -ne 1) {
        throw "Original startup did not clear SP_FASTCOPY on the logo sprite.`n$gdbOutput"
    }
    if (-not $startupLogoReloc.Success -or
        $startupLogoReloc.Groups[1].Value.ToLowerInvariant() -ne '0x4c524c43' -or`
        -not $startupLogoRelocSize.Success -or
        [int]$startupLogoRelocSize.Groups[1].Value -ne 29712 -or`
        -not $startupLogoSwapCount.Success -or
        [int]$startupLogoSwapCount.Groups[1].Value -ne 7428 -or`
        -not $startupLogoPointerCount.Success -or
        [int]$startupLogoPointerCount.Groups[1].Value -ne 9) {
        throw "Startup N64 logo O2R relocation was not loaded and fixed up.`n$gdbOutput"
    }
    if (-not $startupLogoDraw.Success -or
        $startupLogoDraw.Groups[1].Value.ToLowerInvariant() -ne '0x4c445257' -or`
        -not $startupLogoBlocker.Success -or
        [int]$startupLogoBlocker.Groups[1].Value -ne 0 -or`
        -not $startupLogoCallbacks.Success -or
        [int]$startupLogoCallbacks.Groups[1].Value -lt 1 -or`
        -not $startupLogoDrawUpdate.Success -or
        [int]$startupLogoDrawUpdate.Groups[1].Value -ne 17 -or`
        -not $startupLogoDrawWidth.Success -or
        [int]$startupLogoDrawWidth.Groups[1].Value -ne 128 -or`
        -not $startupLogoDrawHeight.Success -or
        [int]$startupLogoDrawHeight.Groups[1].Value -ne 108 -or`
        -not $startupLogoDrawFormat.Success -or
        [int]$startupLogoDrawFormat.Groups[1].Value -ne 0 -or`
        -not $startupLogoDrawSize.Success -or
        [int]$startupLogoDrawSize.Groups[1].Value -ne 2 -or`
        -not $startupLogoDrawBitmaps.Success -or
        [int]$startupLogoDrawBitmaps.Groups[1].Value -ne 8 -or`
        -not $startupLogoDrawPixels.Success -or
        [int]$startupLogoDrawPixels.Groups[1].Value -le 1000) {
        throw "Startup N64 logo did not render through the bounded SObj draw path.`n$gdbOutput"
    }
    if (-not $startupLogoDisplay.Success -or
        [int]$startupLogoDisplay.Groups[1].Value -lt 128 -or
        [int]$startupLogoDisplay.Groups[2].Value -lt 108) {
        throw "Original sprite visual preview is not retained on the DS screen.`n$gdbOutput"
    }
    if (-not $startupLogoDrawAttr.Success -or
        ((Convert-MarkerUInt32 $startupLogoDrawAttr.Groups[1].Value) -band 0x200) -eq 0 -or`
        -not $startupLogoDrawTexshuf.Success -or
        [int]$startupLogoDrawTexshuf.Groups[1].Value -ne 1 -or
        [int]$startupLogoDrawTexshuf.Groups[2].Value -le 1000) {
        throw "Startup N64 logo preview did not prove the original SP_TEXSHUF odd-row swizzle path.`n$gdbOutput"
    }
    if (-not $startupActorFunc.Success -or [int]$startupActorFunc.Groups[1].Value -ne 1) {
        throw "Startup actor GObj did not preserve mnStartupActorFuncRun.`n$gdbOutput"
    }
    if (-not $startupProcKind.Success -or [int]$startupProcKind.Groups[1].Value -ne 0) {
        throw "Startup wallpaper process kind was not preserved as a thread.`n$gdbOutput"
    }
    if (-not $startupProcPriority.Success -or
        [int]$startupProcPriority.Groups[1].Value -ne 1) {
        throw "Startup wallpaper process priority was not preserved.`n$gdbOutput"
    }
    if (-not $startupDisplay.Success -or [int]$startupDisplay.Groups[1].Value -ne 1) {
        throw "Startup wallpaper display callback/link was not preserved.`n$gdbOutput"
    }
    if (-not $startupCamMask.Success -or [int]$startupCamMask.Groups[1].Value -ne 1) {
        throw "Startup wallpaper camera mask was not preserved.`n$gdbOutput"
    }
    if (-not $startupDefaultColor.Success -or
        $startupDefaultColor.Groups[1].Value.ToLowerInvariant() -ne '0xff') {
        throw "Startup default camera fill color was not preserved.`n$gdbOutput"
    }
    if (-not $taskman.Success -or
        $taskman.Groups[1].Value.ToLowerInvariant() -ne '0x5441534b') {
        throw "Taskman bridge setup did not run.`n$gdbOutput"
    }
    if (-not $taskmanContexts.Success -or
        [int]$taskmanContexts.Groups[1].Value -ne 2) {
        throw "Startup taskman contexts were not preserved.`n$gdbOutput"
    }
    if (-not $taskmanGfxNum.Success -or
        [int]$taskmanGfxNum.Groups[1].Value -ne 1) {
        throw "Startup taskman graphics task count was not preserved.`n$gdbOutput"
    }
    if (-not $taskmanGfxHeap.Success -or
        [int]$taskmanGfxHeap.Groups[1].Value -ne 10240) {
        throw "Startup graphics heap size was not preserved.`n$gdbOutput"
    }
    if (-not $taskmanRdpKind.Success -or
        [int]$taskmanRdpKind.Groups[1].Value -ne 2) {
        throw "Startup RDP output buffer kind was not set like original taskman.`n$gdbOutput"
    }
    if (-not $taskmanRdpSize.Success -or
        [int]$taskmanRdpSize.Groups[1].Value -ne 49152) {
        throw "Startup RDP output buffer size was not preserved.`n$gdbOutput"
    }
    if (-not $startupTaskmanMallocs.Success -or
        [int]$startupTaskmanMallocs.Groups[1].Value -ne 36) {
        throw "Taskman bridge did not perform the expected startup allocations.`n$gdbOutput"
    }
    if (-not $taskmanMallocs.Success -or
        [int]$taskmanMallocs.Groups[1].Value -le
        [int]$startupTaskmanMallocs.Groups[1].Value) {
        throw "Opening Room task setup did not add taskman allocations.`n$gdbOutput"
    }
    if (-not $taskmanUsed.Success -or
        [int]$taskmanUsed.Groups[1].Value -le 90000) {
        throw "Taskman bridge heap usage is too small for startup setup.`n$gdbOutput"
    }
    if (-not $taskmanDLValid.Success -or
        [int]$taskmanDLValid.Groups[1].Value -ne 2) {
        throw "Startup display-list buffers were not allocated for both contexts.`n$gdbOutput"
    }
    if (-not $taskmanAutoRead.Success -or
        [int]$taskmanAutoRead.Groups[1].Value -ne 1) {
        throw "Startup controller auto-read contract was not represented.`n$gdbOutput"
    }
    if (-not $taskmanUpdate.Success -or [int]$taskmanUpdate.Groups[1].Value -ne 1) {
        throw "Startup scene update callback was not preserved.`n$gdbOutput"
    }
    if (-not $taskmanDraw.Success -or [int]$taskmanDraw.Groups[1].Value -ne 1) {
        throw "Startup scene draw callback was not preserved.`n$gdbOutput"
    }
    if (-not $taskmanLights.Success -or [int]$taskmanLights.Groups[1].Value -ne 1) {
        throw "Startup lights callback was not preserved.`n$gdbOutput"
    }
    if (-not $taskmanLoop.Success -or [int]$taskmanLoop.Groups[1].Value -ne 1) {
        throw "Original taskman did not reach the parked task loop boundary.`n$gdbOutput"
    }
    if (-not $taskmanUpdates.Success -or
        [int]$taskmanUpdates.Groups[1].Value -ne 55) {
        throw "Original taskman did not run through the bounded startup transition updates.`n$gdbOutput"
    }
    if (-not $taskmanSkipAfter.Success -or
        [int]$taskmanSkipAfter.Groups[1].Value -ne 0) {
        throw "Startup actor update did not exhaust the skip delay.`n$gdbOutput"
    }
    if (-not $taskmanGObjSleeps.Success -or
        [int]$taskmanGObjSleeps.Groups[1].Value -ne 55) {
        throw "Startup GObj thread did not run through the opening transition sleep path.`n$gdbOutput"
    }
    if (-not $taskmanLogoPostX.Success -or
        [int]$taskmanLogoPostX.Groups[1].Value -ne 96) {
        throw "Startup logo post-update X position was not preserved.`n$gdbOutput"
    }
    if (-not $taskmanLogoPostY.Success -or
        [int]$taskmanLogoPostY.Groups[1].Value -ne 65) {
        throw "Startup logo thread did not reach the final logo Y position.`n$gdbOutput"
    }
    if (-not $taskmanOpening.Success -or
        [int]$taskmanOpening.Groups[1].Value -ne 1) {
        throw "Startup logo thread did not set the proceed-opening flag.`n$gdbOutput"
    }
    if (-not $taskmanPostScene.Success -or
        [int]$taskmanPostScene.Groups[1].Value -ne 28) {
        throw "Startup actor did not request nSCKindOpeningRoom.`n$gdbOutput"
    }
    if (-not $taskmanPostPrev.Success -or
        [int]$taskmanPostPrev.Groups[1].Value -ne 27) {
        throw "Startup actor did not preserve nSCKindStartup as previous scene.`n$gdbOutput"
    }
    if (-not $taskmanPostStatus.Success -or
        [int]$taskmanPostStatus.Groups[1].Value -ne 1) {
        throw "Startup actor did not request the taskman load-scene break.`n$gdbOutput"
    }
    if (-not $taskmanPostGObjs.Success -or
        [int]$taskmanPostGObjs.Groups[1].Value -ne 0) {
        throw "Taskman common update did not eject startup GObjs after load-scene request.`n$gdbOutput"
    }
    if (-not $taskmanPostFades.Success -or
        [int]$taskmanPostFades.Groups[1].Value -ne 2) {
        throw "Startup logo thread did not request the second fade actor.`n$gdbOutput"
    }
    if (-not $taskmanCleanup.Success -or
        $taskmanCleanup.Groups[1].Value.ToLowerInvariant() -ne '0x434c4e50') {
        throw "Taskman DS seam did not complete the original cleanup tail.`n$gdbOutput"
    }
    if (-not $taskmanCleanupQueues.Success -or
        [int]$taskmanCleanupQueues.Groups[1].Value -ne 1) {
        throw "Taskman cleanup did not drain its message queues.`n$gdbOutput"
    }
    if (-not $taskmanCleanupMode.Success -or
        [int]$taskmanCleanupMode.Groups[1].Value -ne 2) {
        throw "Taskman cleanup did not restore the original terminal mode.`n$gdbOutput"
    }
    if (-not $taskmanReturns.Success -or
        [int]$taskmanReturns.Groups[1].Value -ne 11) {
        throw "Taskman seam did not return through startup, room, portraits, and eight name-card scenes.`n$gdbOutput"
    }
    if (-not $openingRoomDispatch.Success -or
        [int]$openingRoomDispatch.Groups[1].Value -ne 1) {
        throw "Original scene manager did not dispatch Opening Room exactly once.`n$gdbOutput"
    }
    if (-not $openingRoomStart.Success -or
        $openingRoomStart.Groups[1].Value.ToLowerInvariant() -ne '0x4f525354') {
        throw "Imported Opening Room StartScene did not run.`n$gdbOutput"
    }
    if (-not $openingRoomFuncStart.Success -or
        $openingRoomFuncStart.Groups[1].Value.ToLowerInvariant() -ne '0x4f524653') {
        throw "Imported Opening Room func_start did not run.`n$gdbOutput"
    }
    if (-not $openingRoomReloc.Success -or
        $openingRoomReloc.Groups[1].Value.ToLowerInvariant() -ne '0x4f52524c' -or`
        -not $openingRoomRelocInit.Success -or
        [int]$openingRoomRelocInit.Groups[1].Value -ne 1 -or`
        -not $openingRoomRelocLoad.Success -or
        [int]$openingRoomRelocLoad.Groups[1].Value -ne 8 -or`
        -not $openingRoomRelocMask.Success -or
        $openingRoomRelocMask.Groups[1].Value.ToLowerInvariant() -ne '0xff' -or`
        -not $openingRoomRelocHeaderMask.Success -or
        $openingRoomRelocHeaderMask.Groups[1].Value.ToLowerInvariant() -ne '0xff' -or`
        -not $openingRoomRelocPayloadMask.Success -or
        $openingRoomRelocPayloadMask.Groups[1].Value.ToLowerInvariant() -ne '0xff') {
        throw "Opening Room did not resolve its original relocation file list.`n$gdbOutput"
    }
    if (-not $openingRoomRelocData.Success -or
        [int]$openingRoomRelocData.Groups[1].Value -ne 1) {
        throw "Opening Room relocation payloads were not marked ready after NitroFS load.`n$gdbOutput"
    }
    if (-not $openingRoomRelocFixup.Success -or
        [int]$openingRoomRelocFixup.Groups[1].Value -ne 0) {
        throw "Opening Room mixed-width relocation/render fixups should remain deferred.`n$gdbOutput"
    }
    if (-not $openingRoomRelocBytes.Success -or
        [int]$openingRoomRelocBytes.Groups[1].Value -ne 329248 -or`
        -not $openingRoomRelocLastID.Success -or
        [int]$openingRoomRelocLastID.Groups[1].Value -ne 90 -or`
        -not $openingRoomRelocLastSize.Success -or
        [int]$openingRoomRelocLastSize.Groups[1].Value -ne 158928) {
        throw "Opening movie relocation payload byte accounting is not the expected O2R resource set.`n$gdbOutput"
    }
    if (-not $openingRoomRelocWordSwapMask.Success -or
        $openingRoomRelocWordSwapMask.Groups[1].Value.ToLowerInvariant() -ne '0xff' -or`
        -not $openingRoomRelocWordSwapCount.Success -or
        [int]$openingRoomRelocWordSwapCount.Groups[1].Value -ne 82312 -or`
        -not $openingRoomRelocWordSwapFails.Success -or
        [int]$openingRoomRelocWordSwapFails.Groups[1].Value -ne 0) {
        throw "Opening Room relocation blanket word byte-swap did not match the staged O2R files.`n$gdbOutput"
    }
    if (-not $openingRoomRelocPointerMask.Success -or
        $openingRoomRelocPointerMask.Groups[1].Value.ToLowerInvariant() -ne '0xff' -or`
        -not $openingRoomRelocPointerCount.Success -or
        [int]$openingRoomRelocPointerCount.Groups[1].Value -ne 711 -or`
        -not $openingRoomRelocPointerFails.Success -or
        [int]$openingRoomRelocPointerFails.Groups[1].Value -ne 0) {
        throw "Opening Room internal relocation pointer-chain fixups did not match the staged O2R files.`n$gdbOutput"
    }
    if (-not $openingRoomRelocSymbolCount.Success -or
        [int]$openingRoomRelocSymbolCount.Groups[1].Value -lt 43 -or`
        -not $openingRoomRelocSymbolFails.Success -or
        [int]$openingRoomRelocSymbolFails.Groups[1].Value -ne 0 -or`
        -not $openingRoomRelocSymbolLast.Success -or
        [int]$openingRoomRelocSymbolLast.Groups[1].Value -le 0) {
        throw "Opening Room relocation symbol-offset probes did not resolve through the DS backend.`n$gdbOutput"
    }
    if (-not $openingRoomRelocMObjNormalize.Success -or
        [int]$openingRoomRelocMObjNormalize.Groups[1].Value -ne 18 -or
        [int]$openingRoomRelocMObjNormalize.Groups[2].Value -ne 0 -or
        $openingRoomRelocMObjNormalize.Groups[3].Value.ToLowerInvariant() -ne '0x200' -or`
        -not $openingRoomRelocMObjSource.Success -or
        $openingRoomRelocMObjSource.Groups[1].Value.ToLowerInvariant() -ne '0x4f524d54' -or
        [int]$openingRoomRelocMObjSource.Groups[2].Value -ne 18 -or
        [int]$openingRoomRelocMObjSource.Groups[3].Value -ne 0 -or
        [int]$openingRoomRelocMObjSource.Groups[4].Value -ne 0 -or
        [int]$openingRoomRelocMObjSource.Groups[5].Value -ne 18 -or
        [int]$openingRoomRelocMObjSource.Groups[6].Value -ne 1 -or
        $openingRoomRelocMObjSource.Groups[7].Value.ToLowerInvariant() -ne '0xffffffff' -or
        $openingRoomRelocMObjSource.Groups[8].Value.ToLowerInvariant() -notin @('0', '0x0')) {
        throw "Opening Room MVCommon MObjSub mixed-width/source scan did not match the imported source slice.`n$gdbOutput"
    }
    if (-not $openingRoomFirstEvent.Success -or
        $openingRoomFirstEvent.Groups[1].Value.ToLowerInvariant() -ne '0x4f524631' -or`
        -not $openingRoomFirstEventTick.Success -or
        [int]$openingRoomFirstEventTick.Groups[1].Value -ne 280 -or`
        -not $openingRoomFirstEventMask.Success -or
        $openingRoomFirstEventMask.Groups[1].Value.ToLowerInvariant() -ne '0x3' -or`
        -not $openingRoomFirstEventPencilsDObj.Success -or
        [int]$openingRoomFirstEventPencilsDObj.Groups[1].Value -ne 44728 -or`
        -not $openingRoomFirstEventPencilsAnim.Success -or
        [int]$openingRoomFirstEventPencilsAnim.Groups[1].Value -ne 44912) {
        throw "Opening Room first tick-280 asset references were not proven through the DS reloc backend.`n$gdbOutput"
    }
    if (-not $openingRoomFirstEventData.Success -or
        $openingRoomFirstEventData.Groups[1].Value.ToLowerInvariant() -ne '0x4f524644' -or`
        -not $openingRoomFirstEventDataMask.Success -or
        $openingRoomFirstEventDataMask.Groups[1].Value.ToLowerInvariant() -ne '0xf' -or`
        -not $openingRoomFirstEventDObjEntries.Success -or
        [int]$openingRoomFirstEventDObjEntries.Groups[1].Value -ne 4 -or`
        -not $openingRoomFirstEventDObjDLs.Success -or
        [int]$openingRoomFirstEventDObjDLs.Groups[1].Value -ne 3 -or`
        -not $openingRoomFirstEventAnimJoints.Success -or
        [int]$openingRoomFirstEventAnimJoints.Groups[1].Value -ne 3 -or`
        -not $openingRoomFirstEventAnimOpcode.Success -or
        [int]$openingRoomFirstEventAnimOpcode.Groups[1].Value -ne 3) {
        throw "Opening Room first tick-280 pencils descriptor/animation data shape was not proven.`n$gdbOutput"
    }
    if (-not $openingRoomFirstEventRun.Success -or
        $openingRoomFirstEventRun.Groups[1].Value.ToLowerInvariant() -ne '0x4f523238' -or`
        -not $openingRoomFirstEventDeferred.Success -or
        $openingRoomFirstEventDeferred.Groups[1].Value.ToLowerInvariant() -ne '0x1' -or`
        -not $openingRoomFighterDeferred.Success -or
        $openingRoomFighterDeferred.Groups[1].Value.ToLowerInvariant() -ne '0x4f524646' -or`
        -not $openingRoomFighterDeferredKind.Success -or`
        -not $openingRoomPulledKind.Success -or
        [int]$openingRoomFighterDeferredKind.Groups[1].Value -ne
        [int]$openingRoomPulledKind.Groups[1].Value) {
        throw "Opening Room tick-280 update did not run with the expected deferred fighter boundary.`n$gdbOutput"
    }
    if (-not $openingRoomTick380Deferred.Success -or
        $openingRoomTick380Deferred.Groups[1].Value.ToLowerInvariant() -ne '0x4f523338' -or`
        -not $openingRoomTick380DeferredMask.Success -or
        $openingRoomTick380DeferredMask.Groups[1].Value.ToLowerInvariant() -ne '0x1') {
        throw "Opening Room tick-380 fighter-status boundary was not explicitly deferred.`n$gdbOutput"
    }
    if (-not $openingRoomTick450Run.Success -or
        $openingRoomTick450Run.Groups[1].Value.ToLowerInvariant() -ne '0x4f523435' -or`
        -not $openingRoomTick450Deferred.Success -or
        $openingRoomTick450Deferred.Groups[1].Value.ToLowerInvariant() -ne '0' -or`
        -not $openingRoomOutsideAsset.Success -or
        $openingRoomOutsideAsset.Groups[1].Value.ToLowerInvariant() -ne '0x1' -or`
        -not $openingRoomOutsideDLOffset.Success -or
        [int]$openingRoomOutsideDLOffset.Groups[1].Value -ne 147968 -or`
        -not $openingRoomOutsideCreate.Success -or
        $openingRoomOutsideCreate.Groups[1].Value.ToLowerInvariant() -ne '0x4f524f55' -or`
        -not $openingRoomOutsideCreateMask.Success -or
        $openingRoomOutsideCreateMask.Groups[1].Value.ToLowerInvariant() -ne '0xf' -or`
        -not $openingRoomOutsideCreateGObjs.Success -or
        [int]$openingRoomOutsideCreateGObjs.Groups[1].Value -ne 11 -or`
        -not $openingRoomOutsideGObjDelta.Success -or
        [int]$openingRoomOutsideGObjDelta.Groups[1].Value -ne 1 -or`
        -not $openingRoomOutsideDObjDelta.Success -or
        [int]$openingRoomOutsideDObjDelta.Groups[1].Value -ne 1 -or`
        -not $openingRoomOutsideXObjDelta.Success -or
        [int]$openingRoomOutsideXObjDelta.Groups[1].Value -ne 1 -or`
        -not $openingRoomOutsideDisplay.Success -or
        [int]$openingRoomOutsideDisplay.Groups[1].Value -ne 1 -or`
        -not $openingRoomHazeAsset.Success -or
        $openingRoomHazeAsset.Groups[1].Value.ToLowerInvariant() -ne '0x1' -or`
        -not $openingRoomHazeDLOffset.Success -or
        [int]$openingRoomHazeDLOffset.Groups[1].Value -ne 39160 -or`
        -not $openingRoomHazeCreate.Success -or
        $openingRoomHazeCreate.Groups[1].Value.ToLowerInvariant() -ne '0x4f52485a' -or`
        -not $openingRoomHazeCreateMask.Success -or
        $openingRoomHazeCreateMask.Groups[1].Value.ToLowerInvariant() -ne '0xf' -or`
        -not $openingRoomHazeCreateGObjs.Success -or
        [int]$openingRoomHazeCreateGObjs.Groups[1].Value -ne 12 -or`
        -not $openingRoomHazeGObjDelta.Success -or
        [int]$openingRoomHazeGObjDelta.Groups[1].Value -ne 1 -or`
        -not $openingRoomHazeDObjDelta.Success -or
        [int]$openingRoomHazeDObjDelta.Groups[1].Value -ne 1 -or`
        -not $openingRoomHazeXObjDelta.Success -or
        [int]$openingRoomHazeXObjDelta.Groups[1].Value -ne 1 -or`
        -not $openingRoomHazeDisplay.Success -or
        [int]$openingRoomHazeDisplay.Groups[1].Value -ne 1 -or`
        -not $openingRoomSunlightAsset.Success -or
        $openingRoomSunlightAsset.Groups[1].Value.ToLowerInvariant() -ne '0x1' -or`
        -not $openingRoomSunlightDLOffset.Success -or
        [int]$openingRoomSunlightDLOffset.Groups[1].Value -ne 149256 -or`
        -not $openingRoomSunlightCreate.Success -or
        $openingRoomSunlightCreate.Groups[1].Value.ToLowerInvariant() -ne '0x4f525343' -or`
        -not $openingRoomSunlightCreateMask.Success -or
        $openingRoomSunlightCreateMask.Groups[1].Value.ToLowerInvariant() -ne '0xf' -or`
        -not $openingRoomSunlightCreateGObjs.Success -or
        [int]$openingRoomSunlightCreateGObjs.Groups[1].Value -ne 13 -or`
        -not $openingRoomSunlightGObjDelta.Success -or
        [int]$openingRoomSunlightGObjDelta.Groups[1].Value -ne 1 -or`
        -not $openingRoomSunlightDObjDelta.Success -or
        [int]$openingRoomSunlightDObjDelta.Groups[1].Value -ne 1 -or`
        -not $openingRoomSunlightXObjDelta.Success -or
        [int]$openingRoomSunlightXObjDelta.Groups[1].Value -ne 1 -or`
        -not $openingRoomSunlightDisplay.Success -or
        [int]$openingRoomSunlightDisplay.Groups[1].Value -ne 1 -or`
        -not $openingRoomDeskAsset.Success -or
        $openingRoomDeskAsset.Groups[1].Value.ToLowerInvariant() -ne '0x1' -or`
        -not $openingRoomDeskDObjOffset.Success -or
        [int]$openingRoomDeskDObjOffset.Groups[1].Value -ne 36344 -or`
        -not $openingRoomDeskCreate.Success -or
        $openingRoomDeskCreate.Groups[1].Value.ToLowerInvariant() -ne '0x4f524453' -or`
        -not $openingRoomDeskCreateMask.Success -or
        $openingRoomDeskCreateMask.Groups[1].Value.ToLowerInvariant() -ne '0xf' -or`
        -not $openingRoomDeskCreateGObjs.Success -or
        [int]$openingRoomDeskCreateGObjs.Groups[1].Value -ne 14 -or`
        -not $openingRoomDeskGObjDelta.Success -or
        [int]$openingRoomDeskGObjDelta.Groups[1].Value -ne 1 -or`
        -not $openingRoomDeskDObjDelta.Success -or
        [int]$openingRoomDeskDObjDelta.Groups[1].Value -le 0 -or`
        -not $openingRoomDeskXObjDelta.Success -or
        [int]$openingRoomDeskXObjDelta.Groups[1].Value -le 0 -or`
        -not $openingRoomDeskDisplay.Success -or
        [int]$openingRoomDeskDisplay.Groups[1].Value -ne 1 -or`
        -not $openingRoomSunlightEject.Success -or
        $openingRoomSunlightEject.Groups[1].Value.ToLowerInvariant() -ne '0x4f525345' -or`
        -not $openingRoomSunlightEjectBefore.Success -or
        [int]$openingRoomSunlightEjectBefore.Groups[1].Value -ne 15 -or`
        -not $openingRoomSunlightEjectAfter.Success -or
        [int]$openingRoomSunlightEjectAfter.Groups[1].Value -ne 15 -or`
        -not $openingRoomSunlightEjectUnlinked.Success -or
        $openingRoomSunlightEjectUnlinked.Groups[1].Value.ToLowerInvariant() -ne '0x3' -or`
        -not $openingRoomCloseUpOverlayCreate.Success -or
        $openingRoomCloseUpOverlayCreate.Groups[1].Value.ToLowerInvariant() -ne '0x4f52434f' -or`
        -not $openingRoomCloseUpOverlayCreateMask.Success -or
        $openingRoomCloseUpOverlayCreateMask.Groups[1].Value.ToLowerInvariant() -ne '0x7' -or`
        -not $openingRoomCloseUpOverlayCreateTick.Success -or
        [int]$openingRoomCloseUpOverlayCreateTick.Groups[1].Value -ne 450 -or`
        -not $openingRoomCloseUpOverlayCreateGObjs.Success -or
        [int]$openingRoomCloseUpOverlayCreateGObjs.Groups[1].Value -ne 15 -or`
        -not $openingRoomCloseUpOverlayGObjDelta.Success -or
        [int]$openingRoomCloseUpOverlayGObjDelta.Groups[1].Value -ne 1 -or`
        -not $openingRoomCloseUpOverlayDisplay.Success -or
        [int]$openingRoomCloseUpOverlayDisplay.Groups[1].Value -ne 1 -or`
        -not $openingRoomCloseUpOverlayAlpha.Success -or
        [int]$openingRoomCloseUpOverlayAlpha.Groups[1].Value -ne 0) {
        throw "Opening Room tick-450 close-up overlay boundary did not run as expected.`n$gdbOutput"
    }
    if (-not $openingRoomTick500Run.Success -or
        $openingRoomTick500Run.Groups[1].Value.ToLowerInvariant() -ne '0x4f523530' -or`
        -not $openingRoomTick500Deferred.Success -or
        $openingRoomTick500Deferred.Groups[1].Value.ToLowerInvariant() -ne '0x1' -or`
        -not $openingRoomSpotlightAsset.Success -or
        $openingRoomSpotlightAsset.Groups[1].Value.ToLowerInvariant() -ne '0x7' -or`
        -not $openingRoomSpotlightDLOffset.Success -or
        [int]$openingRoomSpotlightDLOffset.Groups[1].Value -ne 142872 -or`
        -not $openingRoomSpotlightMObjOffset.Success -or
        [int]$openingRoomSpotlightMObjOffset.Groups[1].Value -ne 142480 -or`
        -not $openingRoomSpotlightMatAnimOffset.Success -or
        [int]$openingRoomSpotlightMatAnimOffset.Groups[1].Value -ne 143120 -or`
        -not $openingRoomSpotlightCreate.Success -or
        $openingRoomSpotlightCreate.Groups[1].Value.ToLowerInvariant() -ne '0x4f52534c' -or`
        -not $openingRoomSpotlightCreateMask.Success -or
        $openingRoomSpotlightCreateMask.Groups[1].Value.ToLowerInvariant() -ne '0xff' -or`
        -not $openingRoomSpotlightCreateTick.Success -or
        [int]$openingRoomSpotlightCreateTick.Groups[1].Value -ne 500 -or`
        -not $openingRoomSpotlightCreateGObjs.Success -or
        [int]$openingRoomSpotlightCreateGObjs.Groups[1].Value -ne 15 -or`
        -not $openingRoomSpotlightGObjDelta.Success -or
        [int]$openingRoomSpotlightGObjDelta.Groups[1].Value -ne 1 -or`
        -not $openingRoomSpotlightDObjDelta.Success -or
        [int]$openingRoomSpotlightDObjDelta.Groups[1].Value -ne 1 -or`
        -not $openingRoomSpotlightXObjDelta.Success -or
        [int]$openingRoomSpotlightXObjDelta.Groups[1].Value -ne 1 -or`
        -not $openingRoomSpotlightMObjDelta.Success -or
        [int]$openingRoomSpotlightMObjDelta.Groups[1].Value -ne 2 -or`
        -not $openingRoomSpotlightDisplay.Success -or
        [int]$openingRoomSpotlightDisplay.Groups[1].Value -ne 1 -or`
        -not $openingRoomSpotlightProcess.Success -or
        [int]$openingRoomSpotlightProcess.Groups[1].Value -ne 1 -or`
        -not $openingRoomSpotlightMObj.Success -or
        [int]$openingRoomSpotlightMObj.Groups[1].Value -ne 1 -or`
        -not $openingRoomSpotlightMatAnim.Success -or
        [int]$openingRoomSpotlightMatAnim.Groups[1].Value -ne 1 -or`
        -not $openingRoomSpotlightPosition.Success -or
        [int]$openingRoomSpotlightPosition.Groups[1].Value -ne 1) {
        throw "Opening Room tick-500 spotlight boundary did not run as expected.`n$gdbOutput"
    }
    if (-not $openingRoomTick560Run.Success -or
        $openingRoomTick560Run.Groups[1].Value.ToLowerInvariant() -ne '0x4f523536' -or`
        -not $openingRoomTick560Deferred.Success -or
        $openingRoomTick560Deferred.Groups[1].Value.ToLowerInvariant() -ne '0x1' -or`
        -not $openingRoomScene2CameraAsset.Success -or
        $openingRoomScene2CameraAsset.Groups[1].Value.ToLowerInvariant() -ne '0x1' -or`
        -not $openingRoomScene2CameraAnimOffset.Success -or
        [int]$openingRoomScene2CameraAnimOffset.Groups[1].Value -ne 0 -or`
        -not $openingRoomScene2CameraEject.Success -or
        $openingRoomScene2CameraEject.Groups[1].Value.ToLowerInvariant() -ne '0x4f523245' -or`
        -not $openingRoomScene2CameraEjectMask.Success -or
        $openingRoomScene2CameraEjectMask.Groups[1].Value.ToLowerInvariant() -ne '0x7' -or`
        -not $openingRoomScene2CameraEjectBeforeGObjs.Success -or
        [int]$openingRoomScene2CameraEjectBeforeGObjs.Groups[1].Value -ne 15 -or`
        -not $openingRoomScene2CameraEjectAfterGObjs.Success -or
        [int]$openingRoomScene2CameraEjectAfterGObjs.Groups[1].Value -ne 15 -or`
        -not $openingRoomScene2CameraEjectBeforeCameras.Success -or
        [int]$openingRoomScene2CameraEjectBeforeCameras.Groups[1].Value -ne 6 -or`
        -not $openingRoomScene2CameraEjectAfterCameras.Success -or
        [int]$openingRoomScene2CameraEjectAfterCameras.Groups[1].Value -ne 4 -or`
        -not $openingRoomScene2CameraCreate.Success -or
        $openingRoomScene2CameraCreate.Groups[1].Value.ToLowerInvariant() -ne '0x4f523243' -or`
        -not $openingRoomScene2CameraCreateMask.Success -or
        $openingRoomScene2CameraCreateMask.Groups[1].Value.ToLowerInvariant() -ne '0x1ff' -or`
        -not $openingRoomScene2CameraCreateGObjs.Success -or
        [int]$openingRoomScene2CameraCreateGObjs.Groups[1].Value -ne 15 -or`
        -not $openingRoomScene2CameraGObjDelta.Success -or
        [int]$openingRoomScene2CameraGObjDelta.Groups[1].Value -ne 2 -or`
        -not $openingRoomScene2CameraCObjDelta.Success -or
        [int]$openingRoomScene2CameraCObjDelta.Groups[1].Value -ne 2 -or`
        -not $openingRoomScene2CameraXObjDelta.Success -or
        [int]$openingRoomScene2CameraXObjDelta.Groups[1].Value -ne 4 -or`
        -not $openingRoomScene2CameraAObjDelta.Success -or
        [int]$openingRoomScene2CameraAObjDelta.Groups[1].Value -ne 0 -or`
        -not $openingRoomScene2CameraDisplay.Success -or
        [int]$openingRoomScene2CameraDisplay.Groups[1].Value -ne 1 -or`
        -not $openingRoomScene2CameraProcess.Success -or
        [int]$openingRoomScene2CameraProcess.Groups[1].Value -ne 1 -or`
        -not $openingRoomScene2CameraAnim.Success -or
        [int]$openingRoomScene2CameraAnim.Groups[1].Value -ne 1 -or`
        -not $openingRoomScene2CameraViewport.Success -or
        [int]$openingRoomScene2CameraViewport.Groups[1].Value -ne 1 -or`
        -not $openingRoomScene2CameraDLBuffer.Success -or
        [int]$openingRoomScene2CameraDLBuffer.Groups[1].Value -ne 1) {
        throw "Opening Room tick-560 Scene 2 camera boundary did not run as expected.`n$gdbOutput"
    }
    $validOpeningRoomDrawCallbacks = @(
        '0x44545245',
        '0x44544c4b',
        '0x444c4e4b',
        '0x444c4831'
    )
    $openingRoomDrawCallbackHex =
        if ($openingRoomDrawCallback.Success) {
            $openingRoomDrawCallback.Groups[1].Value.ToLowerInvariant()
        } else {
            ''
        }
    if (-not $openingRoomDraw.Success -or
        $openingRoomDraw.Groups[1].Value.ToLowerInvariant() -ne '0x4f524457' -or`
        -not $openingRoomDrawBlocker.Success -or
        [int]$openingRoomDrawBlocker.Groups[1].Value -ne 3 -or`
        -not $openingRoomDrawTick.Success -or
        [int]$openingRoomDrawTick.Groups[1].Value -ne 1320 -or`
        -not $openingRoomDrawFrame.Success -or
        [int]$openingRoomDrawFrame.Groups[1].Value -ne 1 -or`
        -not $openingRoomDrawProbes.Success -or
        [int]$openingRoomDrawProbes.Groups[1].Value -ne 2 -or
        [int]$openingRoomDrawProbes.Groups[2].Value -lt 20 -or`
        -not $openingRoomDrawCameras.Success -or
        [int]$openingRoomDrawCameras.Groups[1].Value -le 0 -or`
        -not $openingRoomDrawDisplays.Success -or
        [int]$openingRoomDrawDisplays.Groups[1].Value -le 0 -or`
        -not $openingRoomDrawDObjs.Success -or
        [int]$openingRoomDrawDObjs.Groups[1].Value -le 0 -or`
        -not $openingRoomDrawCameraMask.Success -or
        $openingRoomDrawCameraMask.Groups[1].Value.ToLowerInvariant() -eq '0x0' -or`
        -not $openingRoomDrawCameraPriority.Success -or
        [uint32]($openingRoomDrawCameraPriority.Groups[1].Value) -eq [uint32]::MaxValue -or`
        -not $openingRoomDrawCameraFlags.Success -or
        $openingRoomDrawCameraFlags.Groups[1].Value.ToLowerInvariant() -ne '0x4' -or`
        -not $openingRoomDrawCameraXObjs.Success -or
        [int]$openingRoomDrawCameraXObjs.Groups[1].Value -ne 2 -or
        [int]$openingRoomDrawCameraXObjs.Groups[2].Value -ne 3 -or
        [int]$openingRoomDrawCameraXObjs.Groups[3].Value -ne 8 -or`
        -not $openingRoomDrawCameraViewport.Success -or
        [int]$openingRoomDrawCameraViewport.Groups[1].Value -ne 600 -or
        [int]$openingRoomDrawCameraViewport.Groups[2].Value -ne 440 -or
        [int]$openingRoomDrawCameraViewport.Groups[3].Value -ne 640 -or
        [int]$openingRoomDrawCameraViewport.Groups[4].Value -ne 480 -or`
        -not $openingRoomDrawCameraPersp.Success -or
        [int]$openingRoomDrawCameraPersp.Groups[1].Value -le 0 -or
        [int]$openingRoomDrawCameraPersp.Groups[2].Value -le [int]$openingRoomDrawCameraPersp.Groups[1].Value -or
        [int]$openingRoomDrawCameraPersp.Groups[3].Value -le 0 -or`
        -not $openingRoomDrawCameraEye.Success -or`
        -not $openingRoomDrawCameraAt.Success -or`
        -not $openingRoomDrawObjectLink.Success -or
        [uint32]($openingRoomDrawObjectLink.Groups[1].Value) -eq [uint32]::MaxValue -or`
        -not $openingRoomDrawObjectID.Success -or`
        -not $openingRoomDrawObjectKind.Success -or
        [int]$openingRoomDrawObjectKind.Groups[1].Value -ne 1 -or`
        -not $openingRoomDrawCallback.Success -or
        $validOpeningRoomDrawCallbacks -notcontains $openingRoomDrawCallbackHex -or`
        -not $openingRoomDrawDObjDL.Success -or
        $openingRoomDrawDObjDL.Groups[1].Value.ToLowerInvariant() -eq '0x0' -or`
        -not $openingRoomDrawDObjMeta.Success -or
        $openingRoomDrawDObjMeta.Groups[1].Value.ToLowerInvariant() -eq '0x0') {
        throw "Bounded Opening Room draw did not reach a precise DObj display-list blocker.`n$gdbOutput"
    }
    $openingRoomDrawMaterialCandidateResultHex =
        if ($openingRoomDrawMaterialCandidate.Success) {
            $openingRoomDrawMaterialCandidate.Groups[1].Value.ToLowerInvariant()
        } else {
            ''
        }
    $openingRoomDrawMaterialCandidateCameraMaskHex =
        if ($openingRoomDrawMaterialCandidate.Success) {
            $openingRoomDrawMaterialCandidate.Groups[3].Value.ToLowerInvariant()
        } else {
            ''
        }
    $openingRoomDrawMaterialObjectCallbackHex =
        if ($openingRoomDrawMaterialObject.Success) {
            $openingRoomDrawMaterialObject.Groups[4].Value.ToLowerInvariant()
        } else {
            ''
        }
    $openingRoomDrawMaterialDObjDLHex =
        if ($openingRoomDrawMaterialDObj.Success) {
            $openingRoomDrawMaterialDObj.Groups[1].Value.ToLowerInvariant()
        } else {
            ''
        }
    $openingRoomDrawMaterialDObjMetaHex =
        if ($openingRoomDrawMaterialDObj.Success) {
            $openingRoomDrawMaterialDObj.Groups[2].Value.ToLowerInvariant()
        } else {
            '0'
        }
    $openingRoomDrawMaterialDObjMetaValue =
        if ($openingRoomDrawMaterialDObjMetaHex.StartsWith('0x')) {
            [Convert]::ToUInt32($openingRoomDrawMaterialDObjMetaHex.Substring(2), 16)
        } else {
            [Convert]::ToUInt32($openingRoomDrawMaterialDObjMetaHex, 10)
        }
    if (-not $openingRoomDrawMaterialCandidate.Success -or
        $openingRoomDrawMaterialCandidateResultHex -ne '0x4f524d43' -or
        [int]$openingRoomDrawMaterialCandidate.Groups[2].Value -lt 1 -or
        (@('0', '0x0') -contains $openingRoomDrawMaterialCandidateCameraMaskHex) -or
        [uint32]($openingRoomDrawMaterialCandidate.Groups[4].Value) -eq [uint32]::MaxValue -or`
        -not $openingRoomDrawMaterialObject.Success -or
        [uint32]($openingRoomDrawMaterialObject.Groups[1].Value) -eq [uint32]::MaxValue -or
        [int]$openingRoomDrawMaterialObject.Groups[3].Value -ne 1 -or
        $validOpeningRoomDrawCallbacks -notcontains $openingRoomDrawMaterialObjectCallbackHex -or`
        -not $openingRoomDrawMaterialDObj.Success -or
        (@('0', '0x0') -contains $openingRoomDrawMaterialDObjDLHex) -or
        (($openingRoomDrawMaterialDObjMetaValue -band 0x9) -ne 0x9)) {
        throw "Bounded Opening Room draw did not expose a material-bearing original DObj candidate.`n$gdbOutput"
    }
    $openingRoomDrawMaterialMObjEffectiveHex =
        if ($openingRoomDrawMaterialMObj.Success) {
            $openingRoomDrawMaterialMObj.Groups[3].Value.ToLowerInvariant()
        } else {
            ''
        }
    $openingRoomDrawMaterialMObjFlagsValue =
        if ($openingRoomDrawMaterialMObj.Success) {
            Convert-MarkerUInt32 $openingRoomDrawMaterialMObj.Groups[2].Value
        } else {
            0
        }
    $openingRoomDrawMaterialMObjMaskHex =
        if ($openingRoomDrawMaterialMObj.Success) {
            $openingRoomDrawMaterialMObj.Groups[4].Value.ToLowerInvariant()
        } else {
            '0'
        }
    $openingRoomDrawMaterialMObjMaskValue =
        if ($openingRoomDrawMaterialMObjMaskHex.StartsWith('0x')) {
            [Convert]::ToUInt32($openingRoomDrawMaterialMObjMaskHex.Substring(2), 16)
        } else {
            [Convert]::ToUInt32($openingRoomDrawMaterialMObjMaskHex, 10)
        }
    $openingRoomDrawMaterialBranchFirstMaskHex =
        if ($openingRoomDrawMaterialBranchFirst.Success) {
            $openingRoomDrawMaterialBranchFirst.Groups[1].Value.ToLowerInvariant()
        } else {
            '0'
        }
    $openingRoomDrawMaterialBranchFirstMaskValue =
        if ($openingRoomDrawMaterialBranchFirstMaskHex.StartsWith('0x')) {
            [Convert]::ToUInt32($openingRoomDrawMaterialBranchFirstMaskHex.Substring(2), 16)
        } else {
            [Convert]::ToUInt32($openingRoomDrawMaterialBranchFirstMaskHex, 10)
        }
    if (-not $openingRoomDrawMaterialMObj.Success -or
        [int]$openingRoomDrawMaterialMObj.Groups[1].Value -lt 1 -or
        $openingRoomDrawMaterialMObjFlagsValue -ne [uint32]0x200 -or
        (@('0', '0x0') -contains $openingRoomDrawMaterialMObjEffectiveHex) -or
        (($openingRoomDrawMaterialMObjMaskValue -band 0x3) -ne 0x3) -or`
        -not $openingRoomDrawMaterialMObjIds.Success -or
        [uint32]$openingRoomDrawMaterialMObjIds.Groups[1].Value -eq [uint32]::MaxValue -or`
        -not $openingRoomDrawMaterialMObjFormat.Success -or
        [uint32]$openingRoomDrawMaterialMObjFormat.Groups[1].Value -eq [uint32]::MaxValue -or
        [uint32]$openingRoomDrawMaterialMObjFormat.Groups[2].Value -eq [uint32]::MaxValue -or`
        -not $openingRoomDrawMaterialMObjTile.Success -or`
        -not $openingRoomDrawMaterialMObjST.Success -or`
        -not $openingRoomDrawMaterialMObjArrays.Success -or`
        -not $openingRoomDrawMaterialMObjPtrs.Success) {
        throw "Bounded Opening Room material candidate did not expose a usable first MObj contract.`n$gdbOutput"
    }
    if (-not $openingRoomDrawTextureMaterial.Success -or`
        -not $openingRoomDrawTextureMaterialObject.Success -or`
        -not $openingRoomDrawTextureMaterialDObj.Success -or`
        -not $openingRoomDrawTextureMaterialMObj.Success -or`
        -not $openingRoomDrawTextureMaterialPtrs.Success) {
        throw "Bounded Opening Room texture-material scan did not expose maintained diagnostics.`n$gdbOutput"
    }
    $openingRoomDrawTextureMaterialResult =
        Convert-MarkerUInt32 $openingRoomDrawTextureMaterial.Groups[1].Value
    $openingRoomDrawTextureMaterialCandidateCount =
        [uint32]$openingRoomDrawTextureMaterial.Groups[2].Value
    $openingRoomDrawTextureMaterialMObjCount =
        [uint32]$openingRoomDrawTextureMaterial.Groups[3].Value
    $openingRoomDrawTextureMaterialSpriteArrayCount =
        [uint32]$openingRoomDrawTextureMaterial.Groups[4].Value
    $openingRoomDrawTextureMaterialSpriteCurrCount =
        [uint32]$openingRoomDrawTextureMaterial.Groups[5].Value
    $openingRoomDrawTextureMaterialSpriteNextCount =
        [uint32]$openingRoomDrawTextureMaterial.Groups[6].Value
    $openingRoomDrawTextureMaterialObjectCallbackHex =
        $openingRoomDrawTextureMaterialObject.Groups[4].Value.ToLowerInvariant()
    $openingRoomDrawTextureMaterialDObjDLHex =
        $openingRoomDrawTextureMaterialDObj.Groups[1].Value.ToLowerInvariant()
    $openingRoomDrawTextureMaterialDObjMetaValue =
        Convert-MarkerUInt32 $openingRoomDrawTextureMaterialDObj.Groups[2].Value
    $openingRoomDrawTextureMaterialEffectiveFlags =
        Convert-MarkerUInt32 $openingRoomDrawTextureMaterialMObj.Groups[2].Value
    $openingRoomDrawTextureMaterialMask =
        Convert-MarkerUInt32 $openingRoomDrawTextureMaterialMObj.Groups[3].Value
    $openingRoomDrawTextureMaterialSpriteArrayHex =
        $openingRoomDrawTextureMaterialPtrs.Groups[1].Value.ToLowerInvariant()
    $openingRoomDrawTextureMaterialSpriteCurrHex =
        $openingRoomDrawTextureMaterialPtrs.Groups[2].Value.ToLowerInvariant()
    $openingRoomDrawTextureMaterialSpriteNextHex =
        $openingRoomDrawTextureMaterialPtrs.Groups[3].Value.ToLowerInvariant()
    if ($openingRoomDrawTextureMaterialResult -eq 0) {
        if ($openingRoomDrawTextureMaterialCandidateCount -ne 0 -or
            $openingRoomDrawTextureMaterialMObjCount -ne 0 -or
            $openingRoomDrawTextureMaterialSpriteArrayCount -ne 0 -or
            $openingRoomDrawTextureMaterialSpriteCurrCount -ne 0 -or
            $openingRoomDrawTextureMaterialSpriteNextCount -ne 0) {
            throw "Bounded Opening Room texture-material scan reported counts without a candidate marker.`n$gdbOutput"
        }
    } elseif ($openingRoomDrawTextureMaterialResult -eq 0x4f525458) {
        if ($openingRoomDrawTextureMaterialCandidateCount -lt 1 -or
            $openingRoomDrawTextureMaterialMObjCount -lt 1 -or
            $openingRoomDrawTextureMaterialSpriteArrayCount -gt $openingRoomDrawTextureMaterialMObjCount -or
            $openingRoomDrawTextureMaterialSpriteCurrCount -gt $openingRoomDrawTextureMaterialSpriteArrayCount -or
            $openingRoomDrawTextureMaterialSpriteNextCount -gt $openingRoomDrawTextureMaterialSpriteArrayCount -or
            [uint32]($openingRoomDrawTextureMaterialObject.Groups[1].Value) -eq [uint32]::MaxValue -or
            [int]$openingRoomDrawTextureMaterialObject.Groups[3].Value -ne 1 -or
            $validOpeningRoomDrawCallbacks -notcontains $openingRoomDrawTextureMaterialObjectCallbackHex -or
            (@('0', '0x0') -contains $openingRoomDrawTextureMaterialDObjDLHex) -or
            (($openingRoomDrawTextureMaterialDObjMetaValue -band 0x9) -ne 0x9) -or
            (($openingRoomDrawTextureMaterialEffectiveFlags -band 0x80) -eq 0) -or
            (($openingRoomDrawTextureMaterialMask -band 0x8) -eq 0) -or
            (@('0', '0x0') -contains $openingRoomDrawTextureMaterialSpriteArrayHex)) {
            throw "Bounded Opening Room texture-material scan found an invalid texture-source candidate.`n$gdbOutput"
        }
        if (($openingRoomDrawTextureMaterialSpriteCurrCount -gt 0) -and
            (@('0', '0x0') -contains $openingRoomDrawTextureMaterialSpriteCurrHex)) {
            throw "Bounded Opening Room texture-material scan counted a current sprite without preserving its pointer.`n$gdbOutput"
        }
        if (($openingRoomDrawTextureMaterialSpriteNextCount -gt 0) -and
            (@('0', '0x0') -contains $openingRoomDrawTextureMaterialSpriteNextHex)) {
            throw "Bounded Opening Room texture-material scan counted a next sprite without preserving its pointer.`n$gdbOutput"
        }
    } else {
        throw "Bounded Opening Room texture-material scan returned an unknown marker.`n$gdbOutput"
    }
    $openingRoomDrawMaterialExpectedGenerated =
        [int]$openingRoomDrawMaterialMObj.Groups[1].Value * 2
    if (-not $openingRoomDrawMaterialBranch.Success -or
        $openingRoomDrawMaterialBranch.Groups[1].Value.ToLowerInvariant() -ne '0x4f524d42' -or
        [int]$openingRoomDrawMaterialBranch.Groups[2].Value -ne [int]$openingRoomDrawMaterialMObj.Groups[1].Value -or
        [int]$openingRoomDrawMaterialBranch.Groups[3].Value -ne 1 -or
        [int]$openingRoomDrawMaterialBranch.Groups[4].Value -ne [int]$openingRoomDrawMaterialMObj.Groups[1].Value -or
        [int]$openingRoomDrawMaterialBranch.Groups[5].Value -ne $openingRoomDrawMaterialExpectedGenerated -or`
        -not $openingRoomDrawMaterialBranchFirst.Success -or
        (($openingRoomDrawMaterialBranchFirstMaskValue -band 0x4023) -ne 0x4023) -or
        (($openingRoomDrawMaterialBranchFirstMaskValue -band 0x440) -ne 0) -or
        [int]$openingRoomDrawMaterialBranchFirst.Groups[2].Value -ne 2 -or
        [int]$openingRoomDrawMaterialBranch.Groups[5].Value -lt [int]$openingRoomDrawMaterialBranchFirst.Groups[2].Value -or`
        -not $openingRoomDrawMaterialBranchTile.Success -or`
        -not $openingRoomDrawMaterialBranchScroll.Success -or`
        -not $openingRoomDrawMaterialBranchLoad.Success) {
        throw "Bounded Opening Room material candidate did not match the original gcDrawMObjForDObj branch-list contract.`n$gdbOutput"
    }
    if ((($openingRoomDrawMaterialMObjMaskValue -band 0x8) -ne 0) -and
        ((($openingRoomDrawMaterialBranchFirstMaskValue -band 0x2000) -eq 0) -or
         [int]$openingRoomDrawMaterialBranchFirst.Groups[3].Value -le 0 -or
         [int]$openingRoomDrawMaterialBranchFirst.Groups[4].Value -le 0)) {
        throw "Opening Room material branch did not preserve the first-MObj texture command contract.`n$gdbOutput"
    }
    if ((($openingRoomDrawMaterialMObjMaskValue -band 0x20) -ne 0) -and
        ((($openingRoomDrawMaterialBranchFirstMaskValue -band 0x800) -eq 0) -or
         [int]$openingRoomDrawMaterialBranchTile.Groups[3].Value -le [int]$openingRoomDrawMaterialBranchTile.Groups[1].Value -or
         [int]$openingRoomDrawMaterialBranchTile.Groups[4].Value -le [int]$openingRoomDrawMaterialBranchTile.Groups[2].Value)) {
        throw "Opening Room material branch did not preserve the first-MObj tile-size contract.`n$gdbOutput"
    }
    if ((($openingRoomDrawMaterialMObjMaskValue -band 0x40) -ne 0) -and
        ((($openingRoomDrawMaterialBranchFirstMaskValue -band 0x1000) -eq 0) -or
         [int]$openingRoomDrawMaterialBranchScroll.Groups[3].Value -le [int]$openingRoomDrawMaterialBranchScroll.Groups[1].Value -or
         [int]$openingRoomDrawMaterialBranchScroll.Groups[4].Value -le [int]$openingRoomDrawMaterialBranchScroll.Groups[2].Value)) {
        throw "Opening Room material branch did not preserve the first-MObj scroll tile-size contract.`n$gdbOutput"
    }
    if ((($openingRoomDrawMaterialBranchFirstMaskValue -band 0x200) -ne 0) -and
        ([int]$openingRoomDrawMaterialBranchLoad.Groups[1].Value -le 0 -or
         [int]$openingRoomDrawMaterialBranchLoad.Groups[2].Value -le 0)) {
        throw "Opening Room material branch did not preserve the first-MObj load-block contract.`n$gdbOutput"
    }
    $openingRoomDrawMaterialEmitResultHex =
        if ($openingRoomDrawMaterialEmit.Success) {
            $openingRoomDrawMaterialEmit.Groups[1].Value.ToLowerInvariant()
        } else {
            ''
        }
    $openingRoomDrawMaterialEmitUnsupportedValue =
        if ($openingRoomDrawMaterialEmit.Success) {
            Convert-MarkerUInt32 $openingRoomDrawMaterialEmit.Groups[3].Value
        } else {
            0xffffffff
        }
    $openingRoomDrawMaterialEmitTableCommands =
        if ($openingRoomDrawMaterialEmit.Success) {
            [uint32]$openingRoomDrawMaterialEmit.Groups[5].Value
        } else {
            0
        }
    $openingRoomDrawMaterialEmitGeneratedCommands =
        if ($openingRoomDrawMaterialEmit.Success) {
            [uint32]$openingRoomDrawMaterialEmit.Groups[6].Value
        } else {
            0
        }
    $openingRoomDrawMaterialEmitHeapStart =
        if ($openingRoomDrawMaterialEmitHeap.Success) {
            Convert-MarkerUInt32 $openingRoomDrawMaterialEmitHeap.Groups[1].Value
        } else {
            0
        }
    $openingRoomDrawMaterialEmitBranchStart =
        if ($openingRoomDrawMaterialEmitHeap.Success) {
            Convert-MarkerUInt32 $openingRoomDrawMaterialEmitHeap.Groups[2].Value
        } else {
            0
        }
    $openingRoomDrawMaterialEmitHeapAfter =
        if ($openingRoomDrawMaterialEmitHeap.Success) {
            Convert-MarkerUInt32 $openingRoomDrawMaterialEmitHeap.Groups[3].Value
        } else {
            0
        }
    $openingRoomDrawMaterialEmitBytes =
        if ($openingRoomDrawMaterialEmitHeap.Success) {
            [uint32]$openingRoomDrawMaterialEmitHeap.Groups[4].Value
        } else {
            0
        }
    $openingRoomDrawMaterialEmitExpectedBytes =
        ($openingRoomDrawMaterialEmitTableCommands +
         $openingRoomDrawMaterialEmitGeneratedCommands) * 8
    $openingRoomDrawMaterialEmitExpectedBranchStart =
        [uint64]$openingRoomDrawMaterialEmitHeapStart +
        ([uint64]$openingRoomDrawMaterialEmitTableCommands * 8)
    $openingRoomDrawMaterialEmitExpectedHeapAfter =
        [uint64]$openingRoomDrawMaterialEmitHeapStart +
        [uint64]$openingRoomDrawMaterialEmitExpectedBytes
    $openingRoomDrawMaterialEmitFirstW0 =
        if ($openingRoomDrawMaterialEmitW0.Success) {
            Convert-MarkerUInt32 $openingRoomDrawMaterialEmitW0.Groups[1].Value
        } else {
            0
        }
    $openingRoomDrawMaterialEmitSecondW0 =
        if ($openingRoomDrawMaterialEmitW0.Success) {
            Convert-MarkerUInt32 $openingRoomDrawMaterialEmitW0.Groups[2].Value
        } else {
            0
        }
    $openingRoomDrawMaterialEmitThirdW0 =
        if ($openingRoomDrawMaterialEmitW0.Success) {
            Convert-MarkerUInt32 $openingRoomDrawMaterialEmitW0.Groups[3].Value
        } else {
            0
        }
    $openingRoomDrawMaterialEmitSecondOp =
        if ($openingRoomDrawMaterialEmitOps.Success) {
            [int]$openingRoomDrawMaterialEmitOps.Groups[3].Value
        } else {`
            -1
        }
    $openingRoomDrawMaterialEmitEndW1 =
        if ($openingRoomDrawMaterialEmitW1.Success) {
            Convert-MarkerUInt32 $openingRoomDrawMaterialEmitW1.Groups[2].Value
        } else {
            1
        }
    $openingRoomDrawMaterialEmitTexturePtr =
        if ($openingRoomDrawMaterialEmitW1.Success) {
            Convert-MarkerUInt32 $openingRoomDrawMaterialEmitW1.Groups[2].Value
        } else {
            1
        }
    $openingRoomDrawMaterialSpriteArray =
        if ($openingRoomDrawMaterialMObjArrays.Success) {
            Convert-MarkerUInt32 $openingRoomDrawMaterialMObjArrays.Groups[1].Value
        } else {
            1
        }
    $openingRoomDrawMaterialSpriteCurr =
        if ($openingRoomDrawMaterialMObjPtrs.Success) {
            Convert-MarkerUInt32 $openingRoomDrawMaterialMObjPtrs.Groups[1].Value
        } else {
            1
        }
    if (-not $openingRoomDrawMaterialEmit.Success -or
        $openingRoomDrawMaterialEmitResultHex -ne '0x4f524d45' -or
        [int]$openingRoomDrawMaterialEmit.Groups[2].Value -ne 0 -or
        $openingRoomDrawMaterialEmitUnsupportedValue -ne 0 -or
        [uint32]$openingRoomDrawMaterialEmit.Groups[4].Value -ne [uint32]$openingRoomDrawMaterialBranch.Groups[2].Value -or
        $openingRoomDrawMaterialEmitTableCommands -ne [uint32]$openingRoomDrawMaterialBranch.Groups[4].Value -or
        $openingRoomDrawMaterialEmitGeneratedCommands -ne [uint32]$openingRoomDrawMaterialBranch.Groups[5].Value -or`
        -not $openingRoomDrawMaterialEmitHeap.Success -or
        $openingRoomDrawMaterialEmitHeapStart -eq 0 -or
        [uint64]$openingRoomDrawMaterialEmitBranchStart -ne $openingRoomDrawMaterialEmitExpectedBranchStart -or
        [uint64]$openingRoomDrawMaterialEmitHeapAfter -ne $openingRoomDrawMaterialEmitExpectedHeapAfter -or
        $openingRoomDrawMaterialEmitBytes -ne $openingRoomDrawMaterialEmitExpectedBytes -or`
        -not $openingRoomDrawMaterialEmitOps.Success -or
        [int]$openingRoomDrawMaterialEmitOps.Groups[1].Value -ne 0xde -or
        [int]$openingRoomDrawMaterialEmitOps.Groups[2].Value -ne 0xfa -or
        [int]$openingRoomDrawMaterialEmitOps.Groups[3].Value -ne 0xdf -or
        [int]$openingRoomDrawMaterialEmitOps.Groups[4].Value -ne 0 -or`
        -not $openingRoomDrawMaterialEmitW0.Success -or
        (($openingRoomDrawMaterialEmitFirstW0 -shr 24) -ne 0xfa) -or
        (($openingRoomDrawMaterialEmitSecondW0 -shr 24) -ne 0xdf) -or
        ($openingRoomDrawMaterialEmitThirdW0 -ne 0) -or`
        -not $openingRoomDrawMaterialEmitW1.Success -or
        $openingRoomDrawMaterialEmitEndW1 -ne 0) {
        throw "Opening Room material branch emission did not match the bounded original-shaped Gfx table/stream contract.`n$gdbOutput"
    }
    if (($openingRoomDrawMaterialEmitSecondOp -eq 0xfd) -and
        ($openingRoomDrawMaterialEmitTexturePtr -eq 0) -and
        (($openingRoomDrawMaterialSpriteArray -ne 0) -or
         ($openingRoomDrawMaterialSpriteCurr -ne 0))) {
        throw "Opening Room material emitted a null SETTIMG pointer without matching null MObj sprite source data.`n$gdbOutput"
    }
    $openingRoomDLPreviewMaterialBranchResultHex =
        if ($openingRoomDLPreviewMaterialBranch.Success) {
            $openingRoomDLPreviewMaterialBranch.Groups[1].Value.ToLowerInvariant()
        } else { '' }
    $openingRoomDLPreviewMaterialBranchUnsupported =
        if ($openingRoomDLPreviewMaterialBranchOps.Success) {
            Convert-MarkerUInt32 $openingRoomDLPreviewMaterialBranchOps.Groups[3].Value
        } else {
            0xffffffff
        }
    $openingRoomDLPreviewMaterialBranchPrimColor =
        if ($openingRoomDLPreviewMaterialBranchPrim.Success) {
            Convert-MarkerUInt32 $openingRoomDLPreviewMaterialBranchPrim.Groups[1].Value
        } else {
            0
        }
    $openingRoomDrawMaterialEmitPrimColor =
        if ($openingRoomDrawMaterialEmitW1.Success) {
            Convert-MarkerUInt32 $openingRoomDrawMaterialEmitW1.Groups[1].Value
        } else {
            0xffffffff
        }
    if (-not $openingRoomDLPreviewMaterialBranch.Success -or
        $openingRoomDLPreviewMaterialBranchResultHex -ne '0x4f524d50' -or
        [int]$openingRoomDLPreviewMaterialBranch.Groups[2].Value -ne 0 -or
        [uint32]$openingRoomDLPreviewMaterialBranch.Groups[3].Value -ne $openingRoomDrawMaterialEmitGeneratedCommands -or
        [uint32]$openingRoomDLPreviewMaterialBranch.Groups[4].Value -ne [uint32]$openingRoomDrawMaterialEmit.Groups[4].Value -or
        [uint32]$openingRoomDLPreviewMaterialBranch.Groups[5].Value -ne $openingRoomDrawMaterialEmitTableCommands -or`
        -not $openingRoomDLPreviewMaterialBranchOps.Success -or
        [int]$openingRoomDLPreviewMaterialBranchOps.Groups[1].Value -ne 0xfa -or
        [int]$openingRoomDLPreviewMaterialBranchOps.Groups[2].Value -ne 0xdf -or
        $openingRoomDLPreviewMaterialBranchUnsupported -ne 0 -or`
        -not $openingRoomDLPreviewMaterialBranchPrim.Success -or
        $openingRoomDLPreviewMaterialBranchPrimColor -ne $openingRoomDrawMaterialEmitPrimColor -or
        [int]$openingRoomDLPreviewMaterialBranchPrim.Groups[2].Value -ne 0 -or
        [int]$openingRoomDLPreviewMaterialBranchPrim.Groups[3].Value -ne 0) {
        throw "Opening Room DL preview did not consume all detached prim-color material branch streams.`n$gdbOutput"
    }
    if (-not $openingRoomMaterialDLProbe.Success -or
        (Convert-MarkerUInt32 $openingRoomMaterialDLProbe.Groups[1].Value) -ne 0 -or
        [int]$openingRoomMaterialDLProbe.Groups[2].Value -ne 3 -or
        (Convert-MarkerUInt32 $openingRoomMaterialDLProbe.Groups[3].Value) -eq 0 -or`
        -not $openingRoomMaterialDLProbeShape.Success -or
        [int]$openingRoomMaterialDLProbeShape.Groups[1].Value -ne 29 -or
        [int]$openingRoomMaterialDLProbeShape.Groups[2].Value -ne 4 -or
        [int]$openingRoomMaterialDLProbeShape.Groups[3].Value -ne 4 -or
        [int]$openingRoomMaterialDLProbeShape.Groups[4].Value -ne 2 -or`
        -not $openingRoomMaterialDLProbeCmds.Success -or
        [int]$openingRoomMaterialDLProbeCmds.Groups[1].Value -ne 2 -or
        [int]$openingRoomMaterialDLProbeCmds.Groups[2].Value -ne 2 -or
        [int]$openingRoomMaterialDLProbeCmds.Groups[3].Value -ne 7 -or
        [int]$openingRoomMaterialDLProbeCmds.Groups[4].Value -ne 1 -or
        [int]$openingRoomMaterialDLProbeCmds.Groups[5].Value -ne 2 -or
        [int]$openingRoomMaterialDLProbeCmds.Groups[6].Value -ne 0 -or`
        -not $openingRoomMaterialDLProbeOps.Success -or
        (Convert-MarkerUInt32 $openingRoomMaterialDLProbeOps.Groups[1].Value) -ne 0xe7 -or
        (Convert-MarkerUInt32 $openingRoomMaterialDLProbeOps.Groups[2].Value) -ne 0xde) {
        throw "Opening Room material DObj display-list probe did not preserve the expected bounded G_DL blocker contract.`n$gdbOutput"
    }
    if (-not $openingRoomMaterialDLExpand.Success -or
        (Convert-MarkerUInt32 $openingRoomMaterialDLExpand.Groups[1].Value) -ne 0x4f524d58 -or
        [int]$openingRoomMaterialDLExpand.Groups[2].Value -ne 0 -or
        (Convert-MarkerUInt32 $openingRoomMaterialDLExpand.Groups[3].Value) -eq 0 -or
        (Convert-MarkerUInt32 $openingRoomMaterialDLExpand.Groups[4].Value) -ne 0x0e000000 -or
        (Convert-MarkerUInt32 $openingRoomMaterialDLExpand.Groups[5].Value) -eq 0 -or`
        -not $openingRoomMaterialDLExpandShape.Success -or
        [int]$openingRoomMaterialDLExpandShape.Groups[1].Value -ne 42 -or
        [int]$openingRoomMaterialDLExpandShape.Groups[2].Value -ne 4 -or
        [int]$openingRoomMaterialDLExpandShape.Groups[3].Value -ne 4 -or
        [int]$openingRoomMaterialDLExpandShape.Groups[4].Value -ne 0 -or`
        -not $openingRoomMaterialDLExpandCmds.Success -or
        [int]$openingRoomMaterialDLExpandCmds.Groups[1].Value -ne 2 -or
        [int]$openingRoomMaterialDLExpandCmds.Groups[2].Value -ne 2 -or
        [int]$openingRoomMaterialDLExpandCmds.Groups[3].Value -ne 7 -or
        [int]$openingRoomMaterialDLExpandCmds.Groups[4].Value -ne 6 -or
        [int]$openingRoomMaterialDLExpandCmds.Groups[5].Value -ne 5 -or
        [int]$openingRoomMaterialDLExpandCmds.Groups[6].Value -ne 5 -or
        [int]$openingRoomMaterialDLExpandCmds.Groups[7].Value -ne 0 -or
        [int]$openingRoomMaterialDLExpandCmds.Groups[8].Value -ne 2 -or
        [int]$openingRoomMaterialDLExpandCmds.Groups[9].Value -ne 0 -or`
        -not $openingRoomMaterialDLExpandOps.Success -or
        (Convert-MarkerUInt32 $openingRoomMaterialDLExpandOps.Groups[1].Value) -ne 0xe7 -or
        (Convert-MarkerUInt32 $openingRoomMaterialDLExpandOps.Groups[2].Value) -ne 0 -or
        [int]$openingRoomMaterialDLExpandOps.Groups[3].Value -ne 5 -or
        [int]$openingRoomMaterialDLExpandOps.Groups[4].Value -ne 2) {
        throw "Opening Room material DObj display-list expansion did not resolve the segment-E branch contract.`n$gdbOutput"
    }
    if (-not $rdpDefaultViewport.Success -or
        [int]$rdpDefaultViewport.Groups[1].Value -le 0 -or
        [int]$rdpDefaultViewport.Groups[2].Value -ne 640 -or
        [int]$rdpDefaultViewport.Groups[3].Value -ne 480 -or
        [int]$rdpDefaultViewport.Groups[4].Value -ne 640 -or
        [int]$rdpDefaultViewport.Groups[5].Value -ne 480 -or
        [int]$rdpDefaultViewport.Groups[6].Value -ne 511 -or
        [int]$rdpDefaultViewport.Groups[7].Value -ne 511) {
        throw "Original RDP default viewport contract did not run with the expected 320x240 default values.`n$gdbOutput"
    }
    if (-not $openingRoomDLPreview.Success -or
        $openingRoomDLPreview.Groups[1].Value.ToLowerInvariant() -ne '0x4f524450' -or`
        -not $openingRoomDLPreviewBlocker.Success -or
        [int]$openingRoomDLPreviewBlocker.Groups[1].Value -ne 0 -or`
        -not $openingRoomDLPreviewCommands.Success -or
        [int]$openingRoomDLPreviewCommands.Groups[1].Value -ne 42 -or`
        -not $openingRoomDLPreviewVertices.Success -or
        [int]$openingRoomDLPreviewVertices.Groups[1].Value -ne 4 -or`
        -not $openingRoomDLPreviewTriangles.Success -or
        [int]$openingRoomDLPreviewTriangles.Groups[1].Value -ne 4 -or`
        -not $openingRoomDLPreviewPixels.Success -or
        [int]$openingRoomDLPreviewPixels.Groups[1].Value -le 0 -or`
        -not $openingRoomDLPreviewFirstOpcode.Success -or
        $openingRoomDLPreviewFirstOpcode.Groups[1].Value.ToLowerInvariant() -ne '0xe7' -or`
        -not $openingRoomDLPreviewUnsupported.Success -or
        $openingRoomDLPreviewUnsupported.Groups[1].Value.ToLowerInvariant() -notin @('0x0', '0') -or`
        -not $openingRoomDLPreviewCommandShape.Success -or
        [int]$openingRoomDLPreviewCommandShape.Groups[1].Value -ne 2 -or
        [int]$openingRoomDLPreviewCommandShape.Groups[2].Value -ne 2 -or
        [int]$openingRoomDLPreviewCommandShape.Groups[3].Value -ne 7 -or
        [int]$openingRoomDLPreviewCommandShape.Groups[4].Value -ne 6 -or
        [int]$openingRoomDLPreviewCommandShape.Groups[5].Value -ne 5 -or
        [int]$openingRoomDLPreviewCommandShape.Groups[6].Value -ne 0 -or
        [int]$openingRoomDLPreviewCommandShape.Groups[7].Value -ne 0 -or`
        -not $openingRoomDLPreviewBranch.Success -or
        [int]$openingRoomDLPreviewBranch.Groups[1].Value -ne 5 -or
        [int]$openingRoomDLPreviewBranch.Groups[2].Value -ne 0 -or
        [int]$openingRoomDLPreviewBranch.Groups[3].Value -ne 2 -or
        [int]$openingRoomDLPreviewBranch.Groups[4].Value -ne 5 -or
        $openingRoomDLPreviewBranch.Groups[5].Value.ToLowerInvariant() -ne '0xffffffff' -or`
        -not $openingRoomDLPreviewFirstDL.Success -or
        $openingRoomDLPreviewFirstDL.Groups[1].Value.ToLowerInvariant() -eq '0x0' -or`
        -not $openingRoomDLPreviewTransform.Success -or
        $openingRoomDLPreviewTransform.Groups[1].Value.ToLowerInvariant() -ne '0x1f' -or`
        -not $openingRoomDLPreviewXObjs.Success -or
        [int]$openingRoomDLPreviewXObjs.Groups[1].Value -lt 1 -or`
        -not $openingRoomDLPreviewXObjKind.Success -or
        [int]$openingRoomDLPreviewXObjKind.Groups[1].Value -ne 28 -or`
        -not $openingRoomDLPreviewTranslate.Success -or`
        -not $openingRoomDLPreviewRotate.Success -or
        [int]$openingRoomDLPreviewRotate.Groups[1].Value -ne 0 -or
        [int]$openingRoomDLPreviewRotate.Groups[2].Value -ne 0 -or
        [int]$openingRoomDLPreviewRotate.Groups[3].Value -ne 0 -or`
        -not $openingRoomDLPreviewScale.Success -or
        [int]$openingRoomDLPreviewScale.Groups[1].Value -le 0 -or
        [int]$openingRoomDLPreviewScale.Groups[2].Value -ne 100 -or
        [int]$openingRoomDLPreviewScale.Groups[3].Value -ne [int]$openingRoomDLPreviewScale.Groups[1].Value -or`
        -not $openingRoomDLPreviewBounds.Success -or
        [int]$openingRoomDLPreviewBounds.Groups[2].Value -le [int]$openingRoomDLPreviewBounds.Groups[1].Value -or
        [int]$openingRoomDLPreviewBounds.Groups[4].Value -le [int]$openingRoomDLPreviewBounds.Groups[3].Value -or`
        -not $openingRoomDLPreviewProjection.Success -or
        # mvopeningroom.c:325-338 installs the original Scene1 camera
        # animation, which objanim.c:2488-2813 parses before this preview.
        $openingRoomDLPreviewProjection.Groups[1].Value.ToLowerInvariant() -ne '0x7f' -or
        [int]$openingRoomDLPreviewProjection.Groups[2].Value -ne 1 -or
        [int]$openingRoomDLPreviewProjection.Groups[3].Value -ne 0 -or`
        -not $openingRoomDLPreviewProjected.Success -or
        [int]$openingRoomDLPreviewProjected.Groups[1].Value -ne 4 -or
        [int]$openingRoomDLPreviewProjected.Groups[2].Value -ne 4 -or`
        -not $openingRoomDLPreviewProjectedBounds.Success -or
        [int]$openingRoomDLPreviewProjectedBounds.Groups[2].Value -le [int]$openingRoomDLPreviewProjectedBounds.Groups[1].Value -or
        [int]$openingRoomDLPreviewProjectedBounds.Groups[4].Value -le [int]$openingRoomDLPreviewProjectedBounds.Groups[3].Value -or`
        -not $openingRoomDLPreviewProjectedDepth.Success -or
        [int]$openingRoomDLPreviewProjectedDepth.Groups[1].Value -le 0 -or
        [int]$openingRoomDLPreviewProjectedDepth.Groups[2].Value -lt [int]$openingRoomDLPreviewProjectedDepth.Groups[1].Value -or`
        -not $openingRoomDLPreviewGeometry.Success -or
        [int]$openingRoomDLPreviewGeometry.Groups[1].Value -ne 2 -or
        $openingRoomDLPreviewGeometry.Groups[2].Value.ToLowerInvariant() -ne '0xd9ffffff' -or
        $openingRoomDLPreviewGeometry.Groups[3].Value.ToLowerInvariant() -ne '0x20000' -or
        $openingRoomDLPreviewGeometry.Groups[4].Value.ToLowerInvariant() -ne '0x20000' -or
        $openingRoomDLPreviewGeometry.Groups[5].Value.ToLowerInvariant() -ne '0x21' -or`
        -not $openingRoomDLPreviewGeometryTris.Success -or
        (([int]$openingRoomDLPreviewGeometryTris.Groups[1].Value +
          [int]$openingRoomDLPreviewGeometryTris.Groups[2].Value) -le 0) -or
        [int]$openingRoomDLPreviewGeometryTris.Groups[4].Value -ne 4 -or`
        -not $openingRoomDLPreviewFallback.Success -or
        [int]$openingRoomDLPreviewFallback.Groups[1].Value -notin @(1, 2) -or
        [uint32]$openingRoomDLPreviewFallback.Groups[2].Value -le 0 -or`
        -not $openingRoomDLPreviewRenderer.Success -or
        [int]$openingRoomDLPreviewRenderer.Groups[1].Value -ne 42 -or
        [int]$openingRoomDLPreviewRenderer.Groups[2].Value -le 0 -or
        [int]$openingRoomDLPreviewRenderer.Groups[3].Value -ne 13 -or
        [int]$openingRoomDLPreviewRenderer.Groups[4].Value -ne 2 -or`
        -not $openingRoomDLPreviewRendererTex.Success -or
        $openingRoomDLPreviewRendererTex.Groups[1].Value.ToLowerInvariant() -ne '0x3f' -or
        $openingRoomDLPreviewRendererTex.Groups[2].Value.ToLowerInvariant() -eq '0x0' -or
        [int]$openingRoomDLPreviewRendererTex.Groups[3].Value -ne 4 -or
        [int]$openingRoomDLPreviewRendererTex.Groups[4].Value -ne 2 -or
        [int]$openingRoomDLPreviewRendererTex.Groups[5].Value -ne 1 -or
        [int]$openingRoomDLPreviewRendererTex.Groups[6].Value -ne 512 -or
        [int]$openingRoomDLPreviewRendererTex.Groups[7].Value -ne 4 -or
        [int]$openingRoomDLPreviewRendererTex.Groups[8].Value -ne 1 -or
        $openingRoomDLPreviewRendererTex.Groups[9].Value.ToLowerInvariant() -ne '0xf' -or`
        -not $openingRoomDLPreviewRendererTile.Success -or
        [int]$openingRoomDLPreviewRendererTile.Groups[1].Value -ne 32 -or
        [int]$openingRoomDLPreviewRendererTile.Groups[2].Value -ne 32 -or
        [int]$openingRoomDLPreviewRendererTile.Groups[3].Value -ne 0 -or
        [int]$openingRoomDLPreviewRendererTile.Groups[4].Value -ne 4 -or
        $openingRoomDLPreviewRendererTile.Groups[5].Value.ToLowerInvariant() -ne '0xb7' -or
        [int]$openingRoomDLPreviewRendererTile.Groups[6].Value -ne 511 -or
        [int]$openingRoomDLPreviewRendererTile.Groups[7].Value -ne 512 -or`
        -not $openingRoomDLPreviewPresent.Success -or
        [int]$openingRoomDLPreviewPresent.Groups[1].Value -ne 1 -or
        [int]$openingRoomDLPreviewPresent.Groups[2].Value -ne 96 -or
        [int]$openingRoomDLPreviewPresent.Groups[3].Value -ne 72 -or
        [int]$openingRoomDLPreviewPresent.Groups[4].Value -lt 1 -or
        [int]$openingRoomDLPreviewPresent.Groups[5].Value -lt 1) {
        throw "Opening Room DObj display-list preview did not parse and rasterize the bounded material-backed original DL slice.`n$gdbOutput"
    }
    if (-not $openingRoomDLPreviewTexMask.Success -or
        $openingRoomDLPreviewTexMask.Groups[1].Value.ToLowerInvariant() -ne '0x3f' -or`
        -not $openingRoomDLPreviewTexImage.Success -or
        $openingRoomDLPreviewTexImage.Groups[1].Value.ToLowerInvariant() -eq '0x0' -or`
        -not $openingRoomDLPreviewTexFormat.Success -or
        [int]$openingRoomDLPreviewTexFormat.Groups[1].Value -ne 4 -or`
        -not $openingRoomDLPreviewTexSize.Success -or
        [int]$openingRoomDLPreviewTexSize.Groups[1].Value -ne 2 -or`
        -not $openingRoomDLPreviewTexImageWidth.Success -or
        [int]$openingRoomDLPreviewTexImageWidth.Groups[1].Value -ne 1 -or`
        -not $openingRoomDLPreviewTexTile.Success -or
        [int]$openingRoomDLPreviewTexTile.Groups[1].Value -ne 16 -or
        [int]$openingRoomDLPreviewTexTile.Groups[2].Value -ne 32 -or`
        -not $openingRoomDLPreviewTexTexels.Success -or
        [int]$openingRoomDLPreviewTexTexels.Groups[1].Value -ne 16 -or
        [int]$openingRoomDLPreviewTexTexels.Groups[2].Value -ne 16 -or`
        -not $openingRoomDLPreviewTexReady.Success -or
        [int]$openingRoomDLPreviewTexReady.Groups[1].Value -ne 1 -or
        [int]$openingRoomDLPreviewTexReady.Groups[2].Value -ne 2 -or`
        -not $openingRoomDLPreviewTexLoad.Success -or
        [int]$openingRoomDLPreviewTexLoad.Groups[1].Value -ne 256 -or`
        -not $openingRoomDLPreviewTexLoadBlock.Success -or
        [int]$openingRoomDLPreviewTexLoadBlock.Groups[1].Value -ne 7 -or
        [int]$openingRoomDLPreviewTexLoadBlock.Groups[2].Value -ne 0 -or
        [int]$openingRoomDLPreviewTexLoadBlock.Groups[3].Value -ne 0 -or
        [int]$openingRoomDLPreviewTexLoadBlock.Groups[4].Value -ne 255 -or
        [int]$openingRoomDLPreviewTexLoadBlock.Groups[5].Value -ne 1024 -or`
        -not $openingRoomDLPreviewTexTileSizeRaw.Success -or
        [int]$openingRoomDLPreviewTexTileSizeRaw.Groups[1].Value -ne 0 -or
        [int]$openingRoomDLPreviewTexTileSizeRaw.Groups[2].Value -ne 0 -or
        [int]$openingRoomDLPreviewTexTileSizeRaw.Groups[3].Value -ne 0 -or
        [int]$openingRoomDLPreviewTexTileSizeRaw.Groups[4].Value -ne 60 -or
        [int]$openingRoomDLPreviewTexTileSizeRaw.Groups[5].Value -ne 124 -or`
        -not $openingRoomDLPreviewTexSamples.Success -or
        [int]$openingRoomDLPreviewTexSamples.Groups[1].Value -le 0 -or`
        -not $openingRoomDLPreviewTexSetTile.Success -or
        [int]$openingRoomDLPreviewTexSetTile.Groups[1].Value -ne 4 -or`
        -not $openingRoomDLPreviewTexCombine.Success -or
        $openingRoomDLPreviewTexCombine.Groups[1].Value.ToLowerInvariant() -ne '0xfc6f96df' -or
        $openingRoomDLPreviewTexCombine.Groups[2].Value.ToLowerInvariant() -ne '0xff2e7f3f' -or`
        -not $openingRoomDLPreviewTexCombineMode.Success -or
        [int]$openingRoomDLPreviewTexCombineMode.Groups[1].Value -ne 0 -or
        $openingRoomDLPreviewTexCombineMode.Groups[2].Value.ToLowerInvariant() -ne '0x1' -or
        [int]$openingRoomDLPreviewTexCombineMode.Groups[3].Value -ne 0 -or`
        -not $openingRoomDLPreviewTexRenderTile.Success -or
        [int]$openingRoomDLPreviewTexRenderTile.Groups[1].Value -ne 0 -or
        [int]$openingRoomDLPreviewTexRenderTile.Groups[2].Value -ne 4 -or
        [int]$openingRoomDLPreviewTexRenderTile.Groups[3].Value -ne 0 -or
        [int]$openingRoomDLPreviewTexRenderTile.Groups[4].Value -ne 0 -or`
        -not $openingRoomDLPreviewTexTileMode.Success -or
        [int]$openingRoomDLPreviewTexTileMode.Groups[1].Value -ne 2 -or
        [int]$openingRoomDLPreviewTexTileMode.Groups[2].Value -ne 2 -or
        [int]$openingRoomDLPreviewTexTileMode.Groups[3].Value -ne 5 -or
        [int]$openingRoomDLPreviewTexTileMode.Groups[4].Value -ne 5 -or
        [int]$openingRoomDLPreviewTexTileMode.Groups[5].Value -ne 0 -or
        [int]$openingRoomDLPreviewTexTileMode.Groups[6].Value -ne 0 -or
        $openingRoomDLPreviewTexTileMode.Groups[7].Value.ToLowerInvariant() -ne '0xb7' -or`
        -not $openingRoomDLPreviewTexTexture.Success -or
        [int]$openingRoomDLPreviewTexTexture.Groups[1].Value -ne 1 -or
        [int]$openingRoomDLPreviewTexTexture.Groups[2].Value -ne 65535 -or
        [int]$openingRoomDLPreviewTexTexture.Groups[3].Value -ne 65535 -or
        $openingRoomDLPreviewTexTexture.Groups[4].Value.ToLowerInvariant() -ne '0xf' -or`
        -not $openingRoomDLPreviewTexTextureMode.Success -or
        [int]$openingRoomDLPreviewTexTextureMode.Groups[1].Value -ne 0 -or
        [int]$openingRoomDLPreviewTexTextureMode.Groups[2].Value -ne 0 -or
        [int]$openingRoomDLPreviewTexTextureMode.Groups[3].Value -ne 1 -or
        [int]$openingRoomDLPreviewTexTextureMode.Groups[4].Value -ne 0) {
        throw "Opening Room DObj display-list preview did not prove the material-candidate texture command boundary.`n$gdbOutput"
    }
    $openingRoomDLPreviewMaterialRawHex =
        if ($openingRoomDLPreviewMaterial.Success) {
            $openingRoomDLPreviewMaterial.Groups[2].Value.ToLowerInvariant()
        } else { '' }
    $openingRoomDLPreviewMaterialEffectiveHex =
        if ($openingRoomDLPreviewMaterial.Success) {
            $openingRoomDLPreviewMaterial.Groups[3].Value.ToLowerInvariant()
        } else { '' }
    $openingRoomDLPreviewMaterialMaskHex =
        if ($openingRoomDLPreviewMaterial.Success) {
            $openingRoomDLPreviewMaterial.Groups[4].Value.ToLowerInvariant()
        } else { '' }
    $openingRoomDLPreviewMaterialSpriteCurrHex =
        if ($openingRoomDLPreviewMaterialPtrs.Success) {
            $openingRoomDLPreviewMaterialPtrs.Groups[1].Value.ToLowerInvariant()
        } else { '' }
    $openingRoomDLPreviewMaterialSpriteNextHex =
        if ($openingRoomDLPreviewMaterialPtrs.Success) {
            $openingRoomDLPreviewMaterialPtrs.Groups[2].Value.ToLowerInvariant()
        } else { '' }
    $openingRoomDLPreviewMaterialPalettePtrHex =
        if ($openingRoomDLPreviewMaterialPtrs.Success) {
            $openingRoomDLPreviewMaterialPtrs.Groups[3].Value.ToLowerInvariant()
        } else { '' }
    if (-not $openingRoomDLPreviewMaterial.Success -or
        [int]$openingRoomDLPreviewMaterial.Groups[1].Value -ne 2 -or
        $openingRoomDLPreviewMaterialRawHex -ne '0x200' -or
        $openingRoomDLPreviewMaterialEffectiveHex -ne '0x200' -or
        $openingRoomDLPreviewMaterialMaskHex -ne '0x403' -or`
        -not $openingRoomDLPreviewMaterialIds.Success -or
        [int]$openingRoomDLPreviewMaterialIds.Groups[1].Value -ne 0 -or
        [int]$openingRoomDLPreviewMaterialIds.Groups[2].Value -ne 0 -or
        [int]$openingRoomDLPreviewMaterialIds.Groups[3].Value -ne 0 -or
        [int]$openingRoomDLPreviewMaterialIds.Groups[4].Value -ne 0 -or`
        -not $openingRoomDLPreviewMaterialFormat.Success -or
        [int]$openingRoomDLPreviewMaterialFormat.Groups[1].Value -ne 4 -or
        [int]$openingRoomDLPreviewMaterialFormat.Groups[2].Value -ne 2 -or
        [int]$openingRoomDLPreviewMaterialFormat.Groups[3].Value -ne 4 -or
        [int]$openingRoomDLPreviewMaterialFormat.Groups[4].Value -ne 1 -or`
        -not $openingRoomDLPreviewMaterialTile.Success -or
        [int]$openingRoomDLPreviewMaterialTile.Groups[1].Value -ne 16 -or
        [int]$openingRoomDLPreviewMaterialTile.Groups[2].Value -ne 32 -or
        [int]$openingRoomDLPreviewMaterialTile.Groups[3].Value -ne 16 -or
        [int]$openingRoomDLPreviewMaterialTile.Groups[4].Value -ne 32 -or`
        -not $openingRoomDLPreviewMaterialST.Success -or
        [int]$openingRoomDLPreviewMaterialST.Groups[1].Value -ne 100 -or
        [int]$openingRoomDLPreviewMaterialST.Groups[2].Value -ne 100 -or
        [int]$openingRoomDLPreviewMaterialST.Groups[3].Value -ne 0 -or
        [int]$openingRoomDLPreviewMaterialST.Groups[4].Value -ne 0 -or`
        -not $openingRoomDLPreviewMaterialPtrs.Success -or
        (@('0', '0x0') -notcontains $openingRoomDLPreviewMaterialSpriteCurrHex) -or
        (@('0', '0x0') -notcontains $openingRoomDLPreviewMaterialSpriteNextHex) -or
        (@('0', '0x0') -notcontains $openingRoomDLPreviewMaterialPalettePtrHex) -or`
        -not $openingRoomDLPreviewMaterialBranch.Success -or
        $openingRoomDLPreviewMaterialBranch.Groups[1].Value.ToLowerInvariant() -ne '0x4f524d50' -or
        [int]$openingRoomDLPreviewMaterialBranch.Groups[2].Value -ne 0 -or
        [int]$openingRoomDLPreviewMaterialBranch.Groups[3].Value -ne 4 -or
        [int]$openingRoomDLPreviewMaterialBranch.Groups[4].Value -ne 2 -or
        [int]$openingRoomDLPreviewMaterialBranch.Groups[5].Value -ne 2 -or`
        -not $openingRoomDLPreviewMaterialBranchOps.Success -or
        [int]$openingRoomDLPreviewMaterialBranchOps.Groups[1].Value -ne 250 -or
        [int]$openingRoomDLPreviewMaterialBranchOps.Groups[2].Value -ne 223 -or
        $openingRoomDLPreviewMaterialBranchOps.Groups[3].Value.ToLowerInvariant() -notin @('0x0', '0') -or`
        -not $openingRoomDLPreviewMaterialBranchPrim.Success -or
        $openingRoomDLPreviewMaterialBranchPrim.Groups[1].Value.ToLowerInvariant() -ne '0xffffffff' -or
        [int]$openingRoomDLPreviewMaterialBranchPrim.Groups[2].Value -ne 0 -or
        [int]$openingRoomDLPreviewMaterialBranchPrim.Groups[3].Value -ne 0) {
        throw "Opening Room material-backed DObj preview did not prove the selected MObj and branch contract.`n$gdbOutput"
    }
    if (-not $openingRoomOverlayCreate.Success -or
        $openingRoomOverlayCreate.Groups[1].Value.ToLowerInvariant() -ne '0x4f524f43' -or`
        -not $openingRoomOverlayDisplay.Success -or
        [int]$openingRoomOverlayDisplay.Groups[1].Value -ne 1 -or`
        -not $openingRoomOverlayAlpha.Success -or
        [int]$openingRoomOverlayAlpha.Groups[1].Value -ne 255 -or`
        -not $openingRoomOverlayCreateGObjs.Success -or
        [int]$openingRoomOverlayCreateGObjs.Groups[1].Value -ne 8 -or`
        -not $openingRoomOverlayEject.Success -or
        $openingRoomOverlayEject.Groups[1].Value.ToLowerInvariant() -ne '0x4f524f45' -or`
        -not $openingRoomOverlayEjectBefore.Success -or
        [int]$openingRoomOverlayEjectBefore.Groups[1].Value -ne 15 -or`
        -not $openingRoomOverlayEjectAfter.Success -or
        [int]$openingRoomOverlayEjectAfter.Groups[1].Value -ne 15 -or`
        -not $openingRoomOverlayEjectUnlinked.Success -or
        $openingRoomOverlayEjectUnlinked.Groups[1].Value.ToLowerInvariant() -ne '0x3') {
        throw "Opening Room overlay GObj creation/ejection boundary did not run as expected.`n$gdbOutput"
    }
    if (-not $openingRoomScene1CameraCreate.Success -or
        $openingRoomScene1CameraCreate.Groups[1].Value.ToLowerInvariant() -ne '0x4f523143' -or`
        -not $openingRoomScene1CameraCreateMask.Success -or
        $openingRoomScene1CameraCreateMask.Groups[1].Value.ToLowerInvariant() -ne '0x1ff' -or`
        -not $openingRoomScene1CameraCreateGObjs.Success -or
        [int]$openingRoomScene1CameraCreateGObjs.Groups[1].Value -ne 4 -or`
        -not $openingRoomScene1CameraGObjDelta.Success -or
        [int]$openingRoomScene1CameraGObjDelta.Groups[1].Value -ne 2 -or`
        -not $openingRoomScene1CameraCObjDelta.Success -or
        [int]$openingRoomScene1CameraCObjDelta.Groups[1].Value -ne 2 -or`
        -not $openingRoomScene1CameraXObjDelta.Success -or
        [int]$openingRoomScene1CameraXObjDelta.Groups[1].Value -ne 4 -or`
        -not $openingRoomScene1CameraAObjDelta.Success -or
        [int]$openingRoomScene1CameraAObjDelta.Groups[1].Value -ne 0 -or`
        -not $openingRoomScene1CameraDisplay.Success -or
        [int]$openingRoomScene1CameraDisplay.Groups[1].Value -ne 1 -or`
        -not $openingRoomScene1CameraProcess.Success -or
        [int]$openingRoomScene1CameraProcess.Groups[1].Value -ne 1 -or`
        -not $openingRoomScene1CameraAnim.Success -or
        [int]$openingRoomScene1CameraAnim.Groups[1].Value -ne 1 -or`
        -not $openingRoomScene1CameraViewport.Success -or
        [int]$openingRoomScene1CameraViewport.Groups[1].Value -ne 1 -or`
        -not $openingRoomScene1CameraDLBuffer.Success -or
        [int]$openingRoomScene1CameraDLBuffer.Groups[1].Value -ne 1) {
        throw "Opening Room Scene 1 camera creation boundary did not run as expected.`n$gdbOutput"
    }
    if (-not $openingRoomCloseUpCameraCreate.Success -or
        $openingRoomCloseUpCameraCreate.Groups[1].Value.ToLowerInvariant() -ne '0x4f524343' -or`
        -not $openingRoomCloseUpCameraCreateMask.Success -or
        $openingRoomCloseUpCameraCreateMask.Groups[1].Value.ToLowerInvariant() -ne '0x1f' -or`
        -not $openingRoomCloseUpCameraCreateGObjs.Success -or
        [int]$openingRoomCloseUpCameraCreateGObjs.Groups[1].Value -ne 5 -or`
        -not $openingRoomCloseUpCameraGObjDelta.Success -or
        [int]$openingRoomCloseUpCameraGObjDelta.Groups[1].Value -ne 1 -or`
        -not $openingRoomCloseUpCameraCObjDelta.Success -or
        [int]$openingRoomCloseUpCameraCObjDelta.Groups[1].Value -ne 1 -or`
        -not $openingRoomCloseUpCameraXObjDelta.Success -or
        [int]$openingRoomCloseUpCameraXObjDelta.Groups[1].Value -ne 0 -or`
        -not $openingRoomCloseUpCameraDisplay.Success -or
        [int]$openingRoomCloseUpCameraDisplay.Groups[1].Value -ne 1 -or`
        -not $openingRoomCloseUpCameraViewport.Success -or
        [int]$openingRoomCloseUpCameraViewport.Groups[1].Value -ne 1) {
        throw "Opening Room close-up overlay camera creation boundary did not run as expected.`n$gdbOutput"
    }
    if (-not $openingRoomWallpaperCameraCreate.Success -or
        $openingRoomWallpaperCameraCreate.Groups[1].Value.ToLowerInvariant() -ne '0x4f525743' -or`
        -not $openingRoomWallpaperCameraCreateMask.Success -or
        $openingRoomWallpaperCameraCreateMask.Groups[1].Value.ToLowerInvariant() -ne '0x1f' -or`
        -not $openingRoomWallpaperCameraCreateGObjs.Success -or
        [int]$openingRoomWallpaperCameraCreateGObjs.Groups[1].Value -ne 6 -or`
        -not $openingRoomWallpaperCameraGObjDelta.Success -or
        [int]$openingRoomWallpaperCameraGObjDelta.Groups[1].Value -ne 1 -or`
        -not $openingRoomWallpaperCameraCObjDelta.Success -or
        [int]$openingRoomWallpaperCameraCObjDelta.Groups[1].Value -ne 1 -or`
        -not $openingRoomWallpaperCameraXObjDelta.Success -or
        [int]$openingRoomWallpaperCameraXObjDelta.Groups[1].Value -ne 0 -or`
        -not $openingRoomWallpaperCameraDisplay.Success -or
        [int]$openingRoomWallpaperCameraDisplay.Groups[1].Value -ne 1 -or`
        -not $openingRoomWallpaperCameraViewport.Success -or
        [int]$openingRoomWallpaperCameraViewport.Groups[1].Value -ne 1) {
        throw "Opening Room wallpaper-camera creation boundary did not run as expected.`n$gdbOutput"
    }
    if (-not $openingRoomLogoCameraAsset.Success -or
        $openingRoomLogoCameraAsset.Groups[1].Value.ToLowerInvariant() -ne '0x1' -or`
        -not $openingRoomLogoCameraAnimOffset.Success -or
        [int]$openingRoomLogoCameraAnimOffset.Groups[1].Value -ne 0 -or`
        -not $openingRoomLogoCameraCreate.Success -or
        $openingRoomLogoCameraCreate.Groups[1].Value.ToLowerInvariant() -ne '0x4f52434d' -or`
        -not $openingRoomLogoCameraCreateMask.Success -or
        $openingRoomLogoCameraCreateMask.Groups[1].Value.ToLowerInvariant() -ne '0x7f' -or`
        -not $openingRoomLogoCameraCreateGObjs.Success -or
        [int]$openingRoomLogoCameraCreateGObjs.Groups[1].Value -ne 7 -or`
        -not $openingRoomLogoCameraGObjDelta.Success -or
        [int]$openingRoomLogoCameraGObjDelta.Groups[1].Value -ne 1 -or`
        -not $openingRoomLogoCameraCObjDelta.Success -or
        [int]$openingRoomLogoCameraCObjDelta.Groups[1].Value -ne 1 -or`
        -not $openingRoomLogoCameraXObjDelta.Success -or
        [int]$openingRoomLogoCameraXObjDelta.Groups[1].Value -ne 2 -or`
        -not $openingRoomLogoCameraAObjDelta.Success -or
        [int]$openingRoomLogoCameraAObjDelta.Groups[1].Value -ne 0 -or`
        -not $openingRoomLogoCameraDisplay.Success -or
        [int]$openingRoomLogoCameraDisplay.Groups[1].Value -ne 1 -or`
        -not $openingRoomLogoCameraProcess.Success -or
        [int]$openingRoomLogoCameraProcess.Groups[1].Value -ne 1 -or`
        -not $openingRoomLogoCameraAnim.Success -or
        [int]$openingRoomLogoCameraAnim.Groups[1].Value -ne 1 -or`
        -not $openingRoomLogoCameraViewport.Success -or
        [int]$openingRoomLogoCameraViewport.Groups[1].Value -ne 1) {
        throw "Opening Room logo-camera creation boundary did not run as expected.`n$gdbOutput"
    }
    if (-not $openingRoomLogoAsset.Success -or
        $openingRoomLogoAsset.Groups[1].Value.ToLowerInvariant() -ne '0x7' -or`
        -not $openingRoomLogoDObjOffset.Success -or
        [int]$openingRoomLogoDObjOffset.Groups[1].Value -ne 115880 -or`
        -not $openingRoomLogoMObjOffset.Success -or
        [int]$openingRoomLogoMObjOffset.Groups[1].Value -ne 113760 -or`
        -not $openingRoomLogoMatAnimOffset.Success -or
        [int]$openingRoomLogoMatAnimOffset.Groups[1].Value -ne 116012 -or`
        -not $openingRoomLogoCreate.Success -or
        $openingRoomLogoCreate.Groups[1].Value.ToLowerInvariant() -ne '0x4f524c43' -or`
        -not $openingRoomLogoCreateMask.Success -or
        $openingRoomLogoCreateMask.Groups[1].Value.ToLowerInvariant() -ne '0x3f' -or`
        -not $openingRoomLogoCreateGObjs.Success -or
        [int]$openingRoomLogoCreateGObjs.Groups[1].Value -ne 9 -or`
        -not $openingRoomLogoGObjDelta.Success -or
        [int]$openingRoomLogoGObjDelta.Groups[1].Value -ne 1 -or`
        -not $openingRoomLogoDObjDelta.Success -or
        [int]$openingRoomLogoDObjDelta.Groups[1].Value -ne 2 -or`
        -not $openingRoomLogoXObjDelta.Success -or
        [int]$openingRoomLogoXObjDelta.Groups[1].Value -ne 4 -or`
        -not $openingRoomLogoMObjDelta.Success -or
        [int]$openingRoomLogoMObjDelta.Groups[1].Value -ne 1 -or`
        -not $openingRoomLogoAObjDelta.Success -or
        [int]$openingRoomLogoAObjDelta.Groups[1].Value -ne 0 -or`
        -not $openingRoomLogoDisplay.Success -or
        [int]$openingRoomLogoDisplay.Groups[1].Value -ne 1 -or`
        -not $openingRoomLogoMObj.Success -or
        [int]$openingRoomLogoMObj.Groups[1].Value -ne 1 -or`
        -not $openingRoomLogoMatAnim.Success -or
        [int]$openingRoomLogoMatAnim.Groups[1].Value -ne 1 -or`
        -not $openingRoomLogoEject.Success -or
        $openingRoomLogoEject.Groups[1].Value.ToLowerInvariant() -ne '0x4f524c45' -or`
        -not $openingRoomLogoEjectBefore.Success -or
        [int]$openingRoomLogoEjectBefore.Groups[1].Value -ne 15 -or`
        -not $openingRoomLogoEjectAfter.Success -or
        [int]$openingRoomLogoEjectAfter.Groups[1].Value -ne 15 -or`
        -not $openingRoomLogoEjectUnlinked.Success -or
        $openingRoomLogoEjectUnlinked.Groups[1].Value.ToLowerInvariant() -ne '0x3') {
        throw "Opening Room logo GObj creation/ejection boundary did not run as expected.`n$gdbOutput"
    }
    if (-not $openingRoomBossShadowAsset.Success -or
        $openingRoomBossShadowAsset.Groups[1].Value.ToLowerInvariant() -ne '0x3' -or`
        -not $openingRoomBossShadowDLOffset.Success -or
        [int]$openingRoomBossShadowDLOffset.Groups[1].Value -ne 128912 -or`
        -not $openingRoomBossShadowAnimOffset.Success -or
        [int]$openingRoomBossShadowAnimOffset.Groups[1].Value -ne 129316 -or`
        -not $openingRoomBossShadowCreate.Success -or
        $openingRoomBossShadowCreate.Groups[1].Value.ToLowerInvariant() -ne '0x4f524243' -or`
        -not $openingRoomBossShadowCreateMask.Success -or
        $openingRoomBossShadowCreateMask.Groups[1].Value.ToLowerInvariant() -ne '0x3f' -or`
        -not $openingRoomBossShadowCreateGObjs.Success -or
        [int]$openingRoomBossShadowCreateGObjs.Groups[1].Value -ne 10 -or`
        -not $openingRoomBossShadowGObjDelta.Success -or
        [int]$openingRoomBossShadowGObjDelta.Groups[1].Value -ne 1 -or`
        -not $openingRoomBossShadowDObjDelta.Success -or
        [int]$openingRoomBossShadowDObjDelta.Groups[1].Value -ne 1 -or`
        -not $openingRoomBossShadowXObjDelta.Success -or
        [int]$openingRoomBossShadowXObjDelta.Groups[1].Value -ne 1 -or`
        -not $openingRoomBossShadowAObjDelta.Success -or
        [int]$openingRoomBossShadowAObjDelta.Groups[1].Value -ne 0 -or`
        -not $openingRoomBossShadowProcess.Success -or
        [int]$openingRoomBossShadowProcess.Groups[1].Value -ne 1 -or`
        -not $openingRoomBossShadowDisplay.Success -or
        [int]$openingRoomBossShadowDisplay.Groups[1].Value -ne 1 -or`
        -not $openingRoomBossShadowAnim.Success -or
        [int]$openingRoomBossShadowAnim.Groups[1].Value -ne 1 -or`
        -not $openingRoomBossShadowEject.Success -or
        $openingRoomBossShadowEject.Groups[1].Value.ToLowerInvariant() -ne '0x4f524245' -or`
        -not $openingRoomBossShadowEjectBefore.Success -or
        [int]$openingRoomBossShadowEjectBefore.Groups[1].Value -ne 15 -or`
        -not $openingRoomBossShadowEjectAfter.Success -or
        [int]$openingRoomBossShadowEjectAfter.Groups[1].Value -ne 15 -or`
        -not $openingRoomBossShadowEjectUnlinked.Success -or
        $openingRoomBossShadowEjectUnlinked.Groups[1].Value.ToLowerInvariant() -ne '0x3') {
        throw "Opening Room boss-shadow GObj creation/ejection boundary did not run as expected.`n$gdbOutput"
    }
    if (-not $openingRoomPencilsCreate.Success -or
        $openingRoomPencilsCreate.Groups[1].Value.ToLowerInvariant() -ne '0x4f525043' -or`
        -not $openingRoomPencilsMask.Success -or
        $openingRoomPencilsMask.Groups[1].Value.ToLowerInvariant() -ne '0x3f' -or`
        -not $openingRoomPencilsGObjDelta.Success -or
        [int]$openingRoomPencilsGObjDelta.Groups[1].Value -ne 1 -or`
        -not $openingRoomPencilsDObjDelta.Success -or
        [int]$openingRoomPencilsDObjDelta.Groups[1].Value -ne 3 -or`
        -not $openingRoomPencilsXObjDelta.Success -or
        [int]$openingRoomPencilsXObjDelta.Groups[1].Value -ne 6 -or`
        -not $openingRoomPencilsAObjDelta.Success -or
        # mvopeningroom.c:317-322 attaches and immediately plays the source
        # pencils animation instead of leaving its AObj table dormant.
        [int]$openingRoomPencilsAObjDelta.Groups[1].Value -ne 18 -or`
        -not $openingRoomPencilsProcess.Success -or
        [int]$openingRoomPencilsProcess.Groups[1].Value -ne 1 -or`
        -not $openingRoomPencilsDisplay.Success -or
        [int]$openingRoomPencilsDisplay.Groups[1].Value -ne 1 -or`
        -not $openingRoomPencilsTree.Success -or
        [int]$openingRoomPencilsTree.Groups[1].Value -ne 3 -or`
        -not $openingRoomPencilsRoots.Success -or
        [int]$openingRoomPencilsRoots.Groups[1].Value -ne 1) {
        throw "Original Opening Room pencils object creation path did not build the expected GObj/DObj tree.`n$gdbOutput"
    }
    if (-not $openingRoomUpdate.Success -or
        $openingRoomUpdate.Groups[1].Value.ToLowerInvariant() -ne '0x4f525550' -or`
        -not $openingRoomTicks.Success -or
        [int]$openingRoomTicks.Groups[1].Value -ne 1320) {
        throw "Imported Opening Room actor did not reach the natural 22-second movie handoff.`n$gdbOutput"
    }
    if (-not $openingRoomGObjs.Success -or
        [int]$openingRoomGObjs.Groups[1].Value -ne 14 -or`
        -not $openingRoomCameras.Success -or
        [int]$openingRoomCameras.Groups[1].Value -ne 6) {
        throw "Opening Room did not create its original actor/default/Scene1/close-up/wallpaper/logo-camera/spotlight slice.`n$gdbOutput"
    }
    if (-not $openingRoomDL0.Success -or
        [int]$openingRoomDL0.Groups[1].Value -ne 12000 -or`
        -not $openingRoomDL1.Success -or
        [int]$openingRoomDL1.Groups[1].Value -ne 4096 -or`
        -not $openingRoomGfxHeap.Success -or
        [int]$openingRoomGfxHeap.Groups[1].Value -ne 32768 -or`
        -not $openingRoomRdpSize.Success -or
        [int]$openingRoomRdpSize.Groups[1].Value -ne 49152) {
        throw "Opening Room task setup sizes do not match the original scene.`n$gdbOutput"
    }
    if (-not $openingRoomMallocs.Success -or
        [int]$openingRoomMallocs.Groups[1].Value -le 0) {
        throw "Opening Room task setup did not allocate through original taskman.`n$gdbOutput"
    }
    if (-not $openingRoomPreAsset.Success -or
        $openingRoomPreAsset.Groups[1].Value.ToLowerInvariant() -ne '0x4f525041') {
        throw "Opening Room did not pass the verified pre-asset boundary.`n$gdbOutput"
    }
    if (-not $openingRoomControllerChecks.Success -or
        [int]$openingRoomControllerChecks.Groups[1].Value -ne 1311) {
        throw "Original Opening Room controller gate did not run through the natural handoff tick.`n$gdbOutput"
    }
    $openingKinds = @(0, 1, 2, 3, 5, 6, 8, 9)
    if (-not $openingRoomPulledKind.Success -or`
        -not $openingRoomDroppedKind.Success -or
        $openingKinds -notcontains [int]$openingRoomPulledKind.Groups[1].Value -or
        $openingKinds -notcontains [int]$openingRoomDroppedKind.Groups[1].Value -or
        [int]$openingRoomPulledKind.Groups[1].Value -eq
        [int]$openingRoomDroppedKind.Groups[1].Value) {
        throw "Original Opening Room fighter selection did not produce two valid distinct kinds.`n$gdbOutput"
    }
    if (-not $openingRoomSkipTitle.Success -or
        [int]$openingRoomSkipTitle.Groups[1].Value -ne 0) {
        throw "Headless verifier unexpectedly triggered the Opening Room skip path.`n$gdbOutput"
    }
    if (-not $openingMovieRoomHandoff.Success -or
        $openingMovieRoomHandoff.Groups[1].Value.ToLowerInvariant() -ne '0x4f4d5248' -or`
        -not $openingMovieRoomHandoffTick.Success -or
        [int]$openingMovieRoomHandoffTick.Groups[1].Value -ne 1320 -or`
        -not $openingMovieRoomHandoffScene.Success -or
        [int]$openingMovieRoomHandoffScene.Groups[1].Value -ne 29) {
        throw "Opening Room did not request the original Opening Portraits scene at the movie handoff.`n$gdbOutput"
    }
    if (-not $openingPortraitsDispatch.Success -or
        [int]$openingPortraitsDispatch.Groups[1].Value -ne 1 -or`
        -not $openingPortraitsStart.Success -or
        $openingPortraitsStart.Groups[1].Value.ToLowerInvariant() -ne '0x4f505354' -or`
        -not $openingPortraitsFuncStart.Success -or
        $openingPortraitsFuncStart.Groups[1].Value.ToLowerInvariant() -ne '0x4f504653') {
        throw "Imported Opening Portraits scene did not start through the original scene manager/taskman path.`n$gdbOutput"
    }
    if (-not $openingPortraitsUpdate.Success -or
        $openingPortraitsUpdate.Groups[1].Value.ToLowerInvariant() -ne '0x4f505550' -or`
        -not $openingPortraitsTicks.Success -or
        [int]$openingPortraitsTicks.Groups[1].Value -ne 150) {
        throw "Imported Opening Portraits scene did not run to its first original next-scene request.`n$gdbOutput"
    }
    if (-not $openingPortraitsReloc.Success -or
        $openingPortraitsReloc.Groups[1].Value.ToLowerInvariant() -ne '0x4f50524c' -or`
        -not $openingPortraitsSpriteNorm.Success -or
        [int]$openingPortraitsSpriteNorm.Groups[1].Value -ne 9 -or
        [int]$openingPortraitsSpriteNorm.Groups[2].Value -ne 0) {
        throw "Opening Portraits original sprite payloads were not normalized for the DS preview path.`n$gdbOutput"
    }
    if (-not $openingPortraitsDraw.Success -or
        $openingPortraitsDraw.Groups[1].Value.ToLowerInvariant() -ne '0x4f504457' -or`
        -not $openingPortraitsDrawBlocker.Success -or
        [int]$openingPortraitsDrawBlocker.Groups[1].Value -ne 0 -or`
        -not $openingPortraitsDrawCallbacks.Success -or
        [int]$openingPortraitsDrawCallbacks.Groups[1].Value -lt 1 -or`
        -not $openingPortraitsDrawVisible.Success -or
        [int]$openingPortraitsDrawVisible.Groups[1].Value -lt 4 -or`
        -not $openingPortraitsDrawMeta.Success -or
        [int]$openingPortraitsDrawMeta.Groups[1].Value -ne 300 -or
        [int]$openingPortraitsDrawMeta.Groups[2].Value -ne 55 -or
        [int]$openingPortraitsDrawMeta.Groups[3].Value -ne 0 -or
        [int]$openingPortraitsDrawMeta.Groups[4].Value -ne 2 -or
        [int]$openingPortraitsDrawMeta.Groups[5].Value -lt 1 -or
        [int]$openingPortraitsDrawMeta.Groups[6].Value -le 1000) {
        throw "Opening Portraits did not render an original SObj sprite through the bounded DS preview path.`n$gdbOutput"
    }
    if (-not $openingPortraitsNext.Success -or
        $openingPortraitsNext.Groups[1].Value.ToLowerInvariant() -ne '0x4f504e58' -or`
        -not $openingPortraitsNextScene.Success -or
        [int]$openingPortraitsNextScene.Groups[1].Value -ne 30) {
        throw "Opening Portraits did not hand off to the original Opening Mario scene boundary.`n$gdbOutput"
    }
    if (-not $openingMarioDispatch.Success -or
        [int]$openingMarioDispatch.Groups[1].Value -ne 1 -or`
        -not $openingMarioStart.Success -or
        $openingMarioStart.Groups[1].Value.ToLowerInvariant() -ne '0x4f4d5354' -or`
        -not $openingMarioFuncStart.Success -or
        $openingMarioFuncStart.Groups[1].Value.ToLowerInvariant() -ne '0x4f4d4653') {
        throw "Imported Opening Mario scene did not start through the original scene manager/taskman path.`n$gdbOutput"
    }
    if (-not $openingMarioUpdate.Success -or
        $openingMarioUpdate.Groups[1].Value.ToLowerInvariant() -ne '0x4f4d5550' -or`
        -not $openingMarioTicks.Success -or
        [int]$openingMarioTicks.Groups[1].Value -ne 60) {
        throw "Imported Opening Mario scene did not run to its first original next-scene request.`n$gdbOutput"
    }
    if (-not $openingMarioReloc.Success -or
        $openingMarioReloc.Groups[1].Value.ToLowerInvariant() -ne '0x4f4d524c' -or`
        -not $openingMarioSpriteNorm.Success -or
        [int]$openingMarioSpriteNorm.Groups[1].Value -lt 5 -or
        [int]$openingMarioSpriteNorm.Groups[2].Value -ne 0) {
        throw "Opening Mario original name-card sprite payloads were not normalized for the DS preview path.`n$gdbOutput"
    }
    if (-not $openingMarioDraw.Success -or
        $openingMarioDraw.Groups[1].Value.ToLowerInvariant() -ne '0x4f4d4457' -or`
        -not $openingMarioDrawBlocker.Success -or
        [int]$openingMarioDrawBlocker.Groups[1].Value -ne 0 -or`
        -not $openingMarioDrawCallbacks.Success -or
        [int]$openingMarioDrawCallbacks.Groups[1].Value -lt 1 -or`
        -not $openingMarioDrawVisible.Success -or
        [int]$openingMarioDrawVisible.Groups[1].Value -lt 5 -or`
        -not $openingMarioDrawMeta.Success -or
        [int]$openingMarioDrawMeta.Groups[1].Value -le 0 -or
        [int]$openingMarioDrawMeta.Groups[2].Value -le 0 -or
        [int]$openingMarioDrawMeta.Groups[3].Value -ne 3 -or
        [int]$openingMarioDrawMeta.Groups[4].Value -ne 1 -or
        [int]$openingMarioDrawMeta.Groups[5].Value -lt 1 -or
        [int]$openingMarioDrawMeta.Groups[6].Value -le 100) {
        throw "Opening Mario did not render original name-card SObjs through the bounded DS preview path.`n$gdbOutput"
    }
    if (-not $openingMarioFighterDefer.Success -or
        $openingMarioFighterDefer.Groups[1].Value.ToLowerInvariant() -ne '0x4f4d4644' -or`
        -not $openingMarioFighterDeferTick.Success -or
        [int]$openingMarioFighterDeferTick.Groups[1].Value -ne 15) {
        throw "Opening Mario did not record the bounded fighter/ground branch deferral at tick 15.`n$gdbOutput"
    }
    if (-not $openingMarioNext.Success -or
        $openingMarioNext.Groups[1].Value.ToLowerInvariant() -ne '0x4f4d4e58' -or`
        -not $openingMarioNextScene.Success -or
        [int]$openingMarioNextScene.Groups[1].Value -ne 31) {
        throw "Opening Mario did not hand off to the original Opening Donkey scene boundary.`n$gdbOutput"
    }
    if (-not $openingNameMasks.Success -or
        (Convert-MarkerUInt32 $openingNameMasks.Groups[1].Value) -ne 0xfe -or
        (Convert-MarkerUInt32 $openingNameMasks.Groups[2].Value) -ne 0xfe -or
        (Convert-MarkerUInt32 $openingNameMasks.Groups[3].Value) -ne 0xfe -or
        (Convert-MarkerUInt32 $openingNameMasks.Groups[4].Value) -ne 0xfe -or
        (Convert-MarkerUInt32 $openingNameMasks.Groups[5].Value) -ne 0xfe -or
        (Convert-MarkerUInt32 $openingNameMasks.Groups[6].Value) -ne 0xff) {
        throw "Imported Donkey/Link/Samus/Yoshi/Kirby/Fox/Pikachu name-card scenes did not all run, draw, and hand off.`n$gdbOutput"
    }
    if (-not $openingNameCounts.Success -or
        [int]$openingNameCounts.Groups[1].Value -ne 7 -or
        [int]$openingNameCounts.Groups[2].Value -ne 36 -or
        [int]$openingNameCounts.Groups[3].Value -ne 60 -or
        [int]$openingNameCounts.Groups[4].Value -ne 38) {
        throw "Bounded imported name-card scenes did not end at Pikachu -> OpeningRun.`n$gdbOutput"
    }
    if (-not $openingNameDraw.Success -or
        $openingNameDraw.Groups[1].Value.ToLowerInvariant() -ne '0x4f4e4457' -or
        [int]$openingNameDraw.Groups[2].Value -ne 0 -or
        [int]$openingNameDraw.Groups[3].Value -lt 7 -or
        [int]$openingNameDraw.Groups[4].Value -lt 3 -or`
        -not $openingNameDrawMeta.Success -or
        [int]$openingNameDrawMeta.Groups[1].Value -le 0 -or
        [int]$openingNameDrawMeta.Groups[2].Value -le 0 -or
        [int]$openingNameDrawMeta.Groups[3].Value -ne 3 -or
        [int]$openingNameDrawMeta.Groups[4].Value -ne 1 -or
        [int]$openingNameDrawMeta.Groups[5].Value -lt 1 -or
        [int]$openingNameDrawMeta.Groups[6].Value -le 100) {
        throw "Imported name-card scenes did not render original SObjs through the bounded DS preview path.`n$gdbOutput"
    }
    if (-not $openingMovieBridge.Success -or
        $openingMovieBridge.Groups[1].Value.ToLowerInvariant() -ne '0x4f4d4252' -or
        (Convert-MarkerUInt32 $openingMovieBridge.Groups[2].Value) -ne 0x1ff -or
        [int]$openingMovieBridge.Groups[3].Value -ne 9 -or
        [int]$openingMovieBridge.Groups[4].Value -ne 46 -or
        [int]$openingMovieBridge.Groups[5].Value -ne 1) {
        throw "Bounded action-scene bridge did not follow the original OpeningRun -> Title scene sequence.`n$gdbOutput"
    }
    if (-not $openingActionPreview.Success -or
        $openingActionPreview.Groups[1].Value.ToLowerInvariant() -ne '0x4f4d4150' -or
        (Convert-MarkerUInt32 $openingActionPreview.Groups[2].Value) -ne 0x1ff -or
        [int]$openingActionPreview.Groups[3].Value -ne 9 -or
        [int]$openingActionPreview.Groups[4].Value -ne 324 -or
        [int]$openingActionPreview.Groups[5].Value -le 70000 -or`
        -not $openingMoviePresentFrames.Success -or
        [int]$openingMoviePresentFrames.Groups[1].Value -le 340 -or`
        -not $openingActionPreviewNorm.Success -or
        [int]$openingActionPreviewNorm.Groups[1].Value -ne 3 -or
        [int]$openingActionPreviewNorm.Groups[2].Value -ne 0 -or`
        -not $openingActionPreviewMeta.Success -or
        [int]$openingActionPreviewMeta.Groups[1].Value -ne 46 -or
        [int]$openingActionPreviewMeta.Groups[2].Value -ne 320 -or
        [int]$openingActionPreviewMeta.Groups[3].Value -ne 264 -or
        [int]$openingActionPreviewMeta.Groups[4].Value -ne 0 -or
        [int]$openingActionPreviewMeta.Groups[5].Value -ne 2) {
        throw "Bounded action-scene bridge did not render and pace original action-era preview sprites.`n$gdbOutput"
    }
    if (-not $openingMovieTitle.Success -or
        $openingMovieTitle.Groups[1].Value.ToLowerInvariant() -ne '0x4f4d5449') {
        throw "Opening movie did not dispatch the Title scene boundary.`n$gdbOutput"
    }
    if (-not $titleReloc.Success -or
        $titleReloc.Groups[1].Value.ToLowerInvariant() -ne '0x5449524c' -or`
        -not $titleSpriteNorm.Success -or
        [int]$titleSpriteNorm.Groups[1].Value -ne 10 -or
        [int]$titleSpriteNorm.Groups[2].Value -ne 0) {
        throw "Title MNTitle O2R sprites were not loaded and normalized for the DS preview path.`n$gdbOutput"
    }
    if (-not $titlePreview.Success -or
        $titlePreview.Groups[1].Value.ToLowerInvariant() -ne '0x54495056' -or`
        -not $titleDraw.Success -or
        $titleDraw.Groups[1].Value.ToLowerInvariant() -ne '0x54494457' -or`
        -not $titleDrawCounts.Success -or
        [int]$titleDrawCounts.Groups[1].Value -ne 10 -or
        [int]$titleDrawCounts.Groups[2].Value -ne 10 -or
        [int]$titleDrawCounts.Groups[3].Value -ne 10 -or
        [int]$titleDrawCounts.Groups[4].Value -le 1000 -or`
        -not $titleDrawMeta.Success -or
        [int]$titleDrawMeta.Groups[1].Value -le 0 -or
        [int]$titleDrawMeta.Groups[2].Value -le 0) {
        throw "Title MNTitle sprites did not render through the bounded DS preview path.`n$gdbOutput"
    }
    if (-not $titleOriginal.Success -or
        $titleOriginal.Groups[1].Value.ToLowerInvariant() -ne '0x54495354' -or
        $titleOriginal.Groups[2].Value.ToLowerInvariant() -ne '0x54494653' -or
        (Convert-MarkerUInt32 $titleOriginal.Groups[3].Value) -ne 0xf -or
        [int]$titleOriginal.Groups[4].Value -ne 2 -or
        [int]$titleOriginal.Groups[5].Value -lt 2 -or
        [int]$titleOriginal.Groups[6].Value -ne 4 -or
        (Convert-MarkerUInt32 $titleOriginal.Groups[7].Value) -ne 0x1e) {
        throw "Imported original mnTitle.c boundary did not run the bounded original task/file/actor/camera setup.`n$gdbOutput"
    }
    if (-not $titleFireSpriteNorm.Success -or
        [int]$titleFireSpriteNorm.Groups[1].Value -ne 30 -or
        [int]$titleFireSpriteNorm.Groups[2].Value -ne 0) {
        throw "MNTitleFireAnim frame sprites were not normalized before the original Title fire updater ran.`n$gdbOutput"
    }
    if (-not $titleLogoFire.Success -or
        $titleLogoFire.Groups[1].Value.ToLowerInvariant() -ne '0x544c4643' -or
        (Convert-MarkerUInt32 $titleLogoFire.Groups[2].Value) -ne 0x3f -or
        [int]$titleLogoFire.Groups[3].Value -ne 1 -or
        [int]$titleLogoFire.Groups[4].Value -ne 4 -or
        [int]$titleLogoFire.Groups[5].Value -ne 3 -or
        [int]$titleLogoFire.Groups[6].Value -ne 1 -or
        [int]$titleLogoFire.Groups[7].Value -ne 0) {
        throw "Imported original mnTitle.c logo-fire object boundary did not run as expected.`n$gdbOutput"
    }
    if (-not $titleFire.Success -or
        $titleFire.Groups[1].Value.ToLowerInvariant() -ne '0x54464952' -or
        (Convert-MarkerUInt32 $titleFire.Groups[2].Value) -ne 0xfff -or
        [int]$titleFire.Groups[3].Value -ne 1 -or
        [int]$titleFire.Groups[4].Value -ne 2 -or
        [int]$titleFire.Groups[5].Value -ne 1 -or
        [int]$titleFire.Groups[6].Value -ne 2 -or
        [int]$titleFire.Groups[7].Value -ne 786432 -or
        [int]$titleFire.Groups[8].Value -ne 0) {
        throw "Imported original mnTitle.c fire object boundary did not run as expected.`n$gdbOutput"
    }
    if (-not $titleOriginalUpdate.Success -or
        $titleOriginalUpdate.Groups[1].Value.ToLowerInvariant() -ne '0x54495550' -or
        [int]$titleOriginalUpdate.Groups[2].Value -ne 1 -or
        [int]$titleOriginalUpdate.Groups[3].Value -ne 0 -or
        [int]$titleOriginalUpdate.Groups[4].Value -ne 1 -or
        [int]$titleOriginalUpdate.Groups[5].Value -ne 1 -or
        [int]$titleOriginalUpdate.Groups[6].Value -ne 0 -or
        [int]$titleOriginalUpdate.Groups[7].Value -ne 3) {
        throw "Imported original mnTitle.c boundary did not run exactly one safe bounded update tick.`n$gdbOutput"
    }
    if (-not $frames.Success -or [int]$frames.Groups[1].Value -le 0) {
        throw "Frame loop did not advance.`n$gdbOutput"
    }
    if (-not $perfFps.Success -or
        [int]$perfFps.Groups[1].Value -le 0 -or
        [int]$perfFps.Groups[1].Value -gt 240 -or
        [int]$perfFps.Groups[4].Value -le 0 -or
        [int]$perfFps.Groups[5].Value -lt 60) {
        throw "FPS/perf sample did not publish a plausible bounded sample.`n$gdbOutput"
    }
    if (-not $perfContent.Success -or
        [int]$perfContent.Groups[1].Value -le 0 -or
        [int]$perfContent.Groups[2].Value -le 0 -or
        [int]$perfContent.Groups[4].Value -le 0) {
        throw "Preview-content cadence counters did not publish expected commit evidence.`n$gdbOutput"
    }
    $hostSeconds =
        if ($null -ne $runtimeStopwatch) {
            [Math]::Max($runtimeStopwatch.Elapsed.TotalSeconds, 0.001)
        } else {
            0.001
        }
    $verifyFps = [double]$frames.Groups[1].Value / $hostSeconds
    Write-Output ("Runtime verification passed ($($frames.Groups[1].Value) frames, " +
                  "fps=$($perfFps.Groups[1].Value)/up=$($perfFps.Groups[2].Value)/dl=$($perfFps.Groups[3].Value), " +
                  "cv=$($perfContent.Groups[3].Value)/ch=$($perfContent.Groups[4].Value), " +
                  "verifyfps={0:N2})." -f $verifyFps)
} finally {
    if ($null -ne $emulator) {
        $emulator.Refresh()
        if (-not $emulator.HasExited) {
            Stop-Process -Id $emulator.Id -Force
            $emulator.WaitForExit()
        }
    }
    if ($runnerSlot -lt 0) {
        if ($null -ne $originalConfig) {
            Set-Content $config -Value $originalConfig -NoNewline
        } else {
            Remove-Item $config -Force -ErrorAction SilentlyContinue
        }
    }
    Remove-Item $stdout, $stderr `
        -Force -ErrorAction SilentlyContinue
    Remove-GdbMarkerTemps -Root $root
}
