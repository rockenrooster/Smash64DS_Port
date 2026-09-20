[CmdletBinding()]
param(
    [string]$MelonDS = '',
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [ValidateRange(1,12)][int]$RunnerSlot = 6,
    [ValidateRange(60,600)][int]$TimeoutSeconds = 300,
    [string]$Build = 'build-p2-yoshi-tour-full',
    [string]$Target = 'smash64ds-battle-playable-proof-hwtri',
    [switch]$FastLogic,
    [switch]$ShellFlow,
    [switch]$DebugWatch,
    [switch]$DebugEvents,
    [switch]$DebugEggAdmission,
    [switch]$DebugEggTexturePrep,
    [switch]$DebugRootProgram,
    [switch]$DebugContractCapture,
    [string]$Artifact = ''
)

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
. (Join-Path $PSScriptRoot 'lib\gdb-markers.ps1')

function Assert-YoshiTour {
    param([bool]$Condition, [string]$Message, [string]$Evidence = '')
    if ($Condition) { return }
    if ($Evidence) { throw "$Message`n$Evidence" }
    throw $Message
}

$buildDir = Join-Path $root (Join-Path 'builds' $Build)
$rom = Join-Path $buildDir ($Target + '.nds')
$elf = Join-Path $buildDir ($Target + '.elf')
$config = Join-Path $buildDir 'nds_build_config.h'
foreach ($path in @($rom,$elf,$config)) {
    Assert-YoshiTour (Test-Path -LiteralPath $path -PathType Leaf) `
        "Missing Yoshi tour input: $path"
}
$text = Get-Content -LiteralPath $config -Raw
foreach ($definition in @(
    ('#define NDS_DEV_LIVE_INPUT_PREVIEW ' + $(if ($FastLogic) { '0' } else { '1' })),
    ('#define NDS_HARNESS_FAST_LOGIC ' + $(if ($FastLogic) { '1' } else { '0' })),
    '#define NDS_P2_YOSHI 1',
    '#define NDS_P2_1P_GAME 1',
    '#define NDS_P2_COMPACT_BATTLE_FIGHTERS 1',
    '#define NDS_P2_PROOF_FIGHTER0 6',
    '#define NDS_P2_YOSHI_BUG_PROOF 1',
    ('#define NDS_R2_PATH ' + $(if ($FastLogic) { '0' } else { '1' })),
    ('#define NDS_HARNESS_FAST_PRESENT_ON_REQUEST ' + $(if ($FastLogic) { '1' } else { '0' }))
)) {
    Assert-YoshiTour $text.Contains($definition) `
        "Yoshi tour build is missing: $definition"
}
if ($ShellFlow) {
    foreach ($definition in @(
        '#define NDS_P2_MENU_SHELL 1',
        '#define NDS_P2_MENU_WALK 1u'
    )) {
        Assert-YoshiTour $text.Contains($definition) `
            "Yoshi shell proof build is missing: $definition"
    }
}
foreach ($stage in @(
    'YOSTER','CASTLE','JUNGLE','ZEBES','HYRULE','YAMABUKI','INISHIE','SECTOR')) {
    Assert-YoshiTour $text.Contains("#define NDS_P2_STAGE_$stage 1") `
        "Yoshi tour build is missing VS stage $stage"
}

if ([string]::IsNullOrWhiteSpace($Artifact)) {
    $Artifact = Join-Path $root ('artifacts\verification\' +
        (Get-Date -Format 'yyyy-MM-dd') + '_yoshi-tour-full.txt')
} elseif (-not [IO.Path]::IsPathRooted($Artifact)) {
    $Artifact = Join-Path $root $Artifact
}

$ctx = Initialize-MelonDSVerifierContext -Root $root -MelonDS $MelonDS `
    -RunnerSlot $RunnerSlot -NoBuild
$state = $null
$emu = $null
try {
    $state = Enable-MelonDSGdbConfig -MelonDSPath $ctx.MelonDSPath `
        -GdbPort $ctx.GdbPort -Persistent -MuteAudio
    $emu = Start-Process -FilePath $ctx.MelonDSPath -ArgumentList $rom `
        -WorkingDirectory (Split-Path -Parent $ctx.MelonDSPath) `
        -WindowStyle Hidden -PassThru
    Wait-MelonDSGdbListener -Process $emu -Port $ctx.GdbPort | Out-Null

    $commands = @(
        'set pagination off',
        'set confirm off',
        'set breakpoint pending off',
        'set remotetimeout 20',
        ("target remote 127.0.0.1:{0}" -f $ctx.GdbPort),
        'break __excpt_entry',
        'commands',
        'silent',
        'printf "YOSHI_TOUR_EXCEPTION pc=%#x lr=%#x\n",$pc,$lr',
        'bt 12',
        'detach',
        'quit 2',
        'end'
    )
    if ($ShellFlow) {
        # Walk-only diagnostic seam in the CSS. It is range/admission checked by
        # ndsMenuShellCssWalkTargetKind, then the screen's own START path commits
        # it into the match descriptor. No fighter or scene state is poked.
        $commands += @(
            'set gNdsMenuShellCssWalkTargetKind = 6',
            'set gNdsMenuShellCssWalkTargetKind2 = 1'
        )
    }
    if ($DebugWatch) {
        $commands += @(
            'watch gNdsYoshiBugTourPhase',
            'commands',
            'silent',
            'printf "YOSHI_WATCH phase=%u frames=%u input=%u retry=%u prepared=%u result=%u done=%u stall=%u heap=%u gmax=%d gactive=%d\n",gNdsYoshiBugTourPhase,gNdsYoshiBugTourFrames,gNdsYoshiBugTourInputCount,gNdsYoshiBugTourRetryCount,gNdsFighterNaturalMotionPrepared,gNdsFighterNaturalMotionResult,gNdsYoshiBugTourDone,gNdsYoshiBugTourStallPhase,(unsigned int)gSYTaskmanGeneralHeap.end-(unsigned int)gSYTaskmanGeneralHeap.ptr,sGCCommonsMaxNum,sGCCommonsActiveNum',
            'continue',
            'end',
            'watch gNdsFighterNaturalMotionPrepared',
            'commands',
            'silent',
            'printf "YOSHI_WATCH prepared=%u phase=%u result=%u done=%u stall=%u\n",gNdsFighterNaturalMotionPrepared,gNdsYoshiBugTourPhase,gNdsFighterNaturalMotionResult,gNdsYoshiBugTourDone,gNdsYoshiBugTourStallPhase',
            'continue',
            'end',
            'watch gNdsFighterNaturalMotionResult',
            'commands',
            'silent',
            'printf "YOSHI_WATCH result=%u phase=%u prepared=%u done=%u stall=%u\n",gNdsFighterNaturalMotionResult,gNdsYoshiBugTourPhase,gNdsFighterNaturalMotionPrepared,gNdsYoshiBugTourDone,gNdsYoshiBugTourStallPhase',
            'continue',
            'end'
        )
    }
    if ($DebugEvents) {
        $commands += @(
            'tbreak src/port/reloc_backend_movement.c:6365',
            'commands',
            'silent',
            'printf "YOSHI_EVENT prepared phase=%u frames=%u prepared=%u result=%u status=%d ga=%d\n",gNdsYoshiBugTourPhase,gNdsYoshiBugTourFrames,gNdsFighterNaturalMotionPrepared,gNdsFighterNaturalMotionResult,((FTStruct *)sNdsFighterManagerLiveGObjs[0]->user_data.p)->status_id,((FTStruct *)sNdsFighterManagerLiveGObjs[0]->user_data.p)->ga',
            'continue',
            'end',
            'break wpYoshiEggThrowMakeWeapon',
            'commands',
            'silent',
            'printf "YOSHI_EVENT eggmake phase=%u frames=%u prepared=%u result=%u heap=%u gmax=%d gactive=%d\n",gNdsYoshiBugTourPhase,gNdsYoshiBugTourFrames,gNdsFighterNaturalMotionPrepared,gNdsFighterNaturalMotionResult,(unsigned int)gSYTaskmanGeneralHeap.end-(unsigned int)gSYTaskmanGeneralHeap.ptr,sGCCommonsMaxNum,sGCCommonsActiveNum',
            'continue',
            'end',
            'break ftCommonWaitSetStatus if $r0 == sNdsFighterManagerLiveGObjs[0]',
            'commands',
            'silent',
            'printf "YOSHI_EVENT wait phase=%u frames=%u status=%d ga=%d prepared=%u result=%u\n",gNdsYoshiBugTourPhase,gNdsYoshiBugTourFrames,((FTStruct *)((GObj *)$r0)->user_data.p)->status_id,((FTStruct *)((GObj *)$r0)->user_data.p)->ga,gNdsFighterNaturalMotionPrepared,gNdsFighterNaturalMotionResult',
            'continue',
            'end',
            'break ftYoshiSpecialNSetStatus if $r0 == sNdsFighterManagerLiveGObjs[0]',
            'commands',
            'silent',
            'printf "YOSHI_EVENT neutral phase=%u frames=%u prepared=%u result=%u\n",gNdsYoshiBugTourPhase,gNdsYoshiBugTourFrames,gNdsFighterNaturalMotionPrepared,gNdsFighterNaturalMotionResult',
            'continue',
            'end',
            'break ftYoshiSpecialNCatchProcCatch if $r0 == sNdsFighterManagerLiveGObjs[0]',
            'commands',
            'silent',
            'printf "YOSHI_EVENT neutralcatch phase=%u frames=%u prepared=%u result=%u\n",gNdsYoshiBugTourPhase,gNdsYoshiBugTourFrames,gNdsFighterNaturalMotionPrepared,gNdsFighterNaturalMotionResult',
            'continue',
            'end',
            'break ndsRendererNativeFighterSetRootProgram if slot == 8 && program != 0',
            'commands',
            'silent',
            'printf \"YOSHI_EVENT rootprogram=%u phase=%u frames=%u status=%d triangles=%u\\n\",program,gNdsYoshiBugTourPhase,gNdsYoshiBugTourFrames,((FTStruct *)sNdsFighterManagerLiveGObjs[0]->user_data.p)->status_id,gNdsFighterDLAllDrawP0HardwareTriangleCount',
            'continue',
            'end',
            'break ndsBaseFTCommonCatchWaitSetStatus if $r0 == sNdsFighterManagerLiveGObjs[0]',
            'commands',
            'silent',
            'printf "YOSHI_EVENT catchwait phase=%u frames=%u prepared=%u result=%u\n",gNdsYoshiBugTourPhase,gNdsYoshiBugTourFrames,gNdsFighterNaturalMotionPrepared,gNdsFighterNaturalMotionResult',
            'continue',
            'end',
            'break ndsBaseFTCommonThrowSetStatus if $r0 == sNdsFighterManagerLiveGObjs[0]',
            'commands',
            'silent',
            'printf "YOSHI_EVENT throw phase=%u frames=%u prepared=%u result=%u\n",gNdsYoshiBugTourPhase,gNdsYoshiBugTourFrames,gNdsFighterNaturalMotionPrepared,gNdsFighterNaturalMotionResult',
            'continue',
            'end'
        )
    }
    if ($DebugEggAdmission) {
        $commands += @(
            'tbreak src/import/battleship_wpmanager_core.c:522 if wp_desc->kind == nWPKindEggThrow',
            'commands',
            'silent',
            'printf "YOSHI_CTOR attr=%p raw=%p mapped=%p model=%p gobj=%p dobj=%p live_dl=%p kind=%d\n",attr,attr->data,weapon_data,gFTDataYoshiModel,weapon_gobj,weapon_gobj->obj,((DObj *)weapon_gobj->obj)->dl,wp_desc->kind',
            'detach',
            'quit'
        )
    }
    if ($DebugEggTexturePrep) {
        $commands += @(
            'tbreak ndsYoshiEggEnsureTexture',
            'continue',
            'printf "YOSHI_TEXPREP game=%d phase=%u prepared=%u total=%u go=%u shared=%u reject=%u\n",gSCManagerBattleState->game_status,gNdsYoshiBugTourPhase,gNdsFighterNaturalMotionPrepared,gNdsYoshiBugHostWatchdogTotalUpdates,gNdsYoshiBugHostWatchdogGoUpdates,gNdsYoshiBugProofSharedEggDrawCount,gNdsWeaponRendererRejectedDrawCount',
            'detach',
            'quit'
        )
    }
    if ($DebugRootProgram) {
        $commands += @(
            'tbreak ndsFighterDrawPlanResolve if owner_slot == 8 && ((FTStruct *)sNdsFighterManagerLiveGObjs[0]->user_data.p)->status_id == nFTYoshiStatusSpecialNCatch',
            'continue',
            'p/x collection->selected_count',
            'p/x expected_asset_id',
            'p/x gNdsYoshiBugTourPhase',
            'p/x ((FTStruct *)sNdsFighterManagerLiveGObjs[0]->user_data.p)->status_id',
            'finish',
            'p/x $r0',
            'p/x gNdsFtrDeclineStage',
            'p/x gNdsFtrDeclineSelected',
            'detach',
            'quit'
        )
    }
    if ($DebugContractCapture) {
        $commands += @(
            'tbreak src/port/renderer_adapter_fighter.c:4962 if gNdsYoshiBugTourPhase == 7 && fp->nds_slot == 0',
            'continue',
            'set $ycroot = (DObj *)fighter_gobj->obj',
            'set $ycj9 = fp->joints[9]',
            'set $ycj31 = fp->joints[31]',
            'printf "YOSHI_CAPTURE event=%u overflow=%u memoState=%u memoHit=%u memoValid=%u memoEvents=%u hidden=%u notex=%u invis=%u mag=%u ignore=%u cammode=%d display=%d root=%p flags=%#x dl=%p child=%p j9=%p flags=%#x dl=%p parent=%p j31=%p flags=%#x dl=%p parent=%p\n",sNdsFighterDisplayContract.event_count,sNdsFighterDisplayContract.selected_overflow_count,sNdsFtrDrawMemoState,sNdsFtrDrawMemoHit,sNdsFtrDrawMemo[fp->nds_slot].valid,sNdsFtrDrawMemo[fp->nds_slot].event_count,gNdsFighterDisplayContractHiddenCount,gNdsFighterDisplayContractNoTextureCount,fp->is_invisible,fp->is_magnify_show,fp->is_magnify_ignore,fp->camera_mode,fp->display_mode,$ycroot,$ycroot->flags,$ycroot->dl,$ycroot->child,$ycj9,($ycj9!=0)?$ycj9->flags:0,($ycj9!=0)?$ycj9->dl:0,($ycj9!=0)?$ycj9->parent:0,$ycj31,($ycj31!=0)?$ycj31->flags:0,($ycj31!=0)?$ycj31->dl:0,($ycj31!=0)?$ycj31->parent:0',
            'detach',
            'quit'
        )
    }
    # The one-shot Up-B event/GObj diagnostics below localized the failure to
    # the source 25 KiB GObj latch. Keep them available in the script history,
    # but do not put per-frame GDB breakpoints on the final acceptance run.
    if ($false) {
        $commands += @(
        # One-shot Up-B event attachment census.  The source creates EggThrow
        # only after dYoshiMainMotion_EggThrowGround sets motion flag2=1 at
        # async frame 4.  If the tour stalls at AwaitUpEgg, this tells us
        # whether the compact full-ROM attached the right mainmotion script or
        # whether the event interpreter skipped a valid script.
        'break ftYoshiSpecialHiProcUpdate',
        'set $yupbp = $bpnum',
        'commands $yupbp',
        'silent',
        'set $yu = (FTStruct *)((GObj *)$r0)->user_data.p',
        'printf "YOSHI_UPSCRIPT motion=%d frame=%f base=%p desc=%#x script=%p wait=%f flag2=%d anim=%#x\n",$yu->motion_id,((GObj *)$r0)->anim_frame,*$yu->data->p_file_mainmotion,$yu->data->mainmotion->motion_desc[$yu->motion_id].offset,$yu->motion_scripts[0][0].p_script,$yu->motion_scripts[0][0].script_wait,$yu->motion_vars.flags.flag2,$yu->anim_desc.word',
        'x/12wx $yu->motion_scripts[0][0].p_script',
        'disable $yupbp',
        'continue',
        'end',
        'break ftYoshiSpecialHiSwitchStatusAir',
        'set $yupbair = $bpnum',
        'commands $yupbair',
        'silent',
        'set $yua = (FTStruct *)((GObj *)$r0)->user_data.p',
        'printf "YOSHI_UP_AIRSWITCH frame=%f status=%d motion=%d script=%p wait=%f flag2=%d floor=%d mask=%#x\n",((GObj *)$r0)->anim_frame,$yua->status_id,$yua->motion_id,$yua->motion_scripts[0][0].p_script,$yua->motion_scripts[0][0].script_wait,$yua->motion_vars.flags.flag2,$yua->coll_data.floor_line_id,$yua->coll_data.mask_curr',
        'continue',
        'end',
        'break ftMainParseMotionEvent if ((FTStruct *)$r1)->fkind == 6 && $r3 == 23',
        'commands',
        'silent',
        'printf "YOSHI_FLAG2_PARSE status=%d motion=%d frame=%f value=%u script=%p wait=%f\n",((FTStruct *)$r1)->status_id,((FTStruct *)$r1)->motion_id,((GObj *)$r0)->anim_frame,((FTMotionEventDefault *)((FTMotionScript *)$r2)->p_script)->value,((FTMotionScript *)$r2)->p_script,((FTMotionScript *)$r2)->script_wait',
        'continue',
        'end',
        'break ftMainUpdateMotionEventsForward if ((FTStruct *)((GObj *)$r0)->user_data.p)->fkind == 6 && (((FTStruct *)((GObj *)$r0)->user_data.p)->status_id == 222 || ((FTStruct *)((GObj *)$r0)->user_data.p)->status_id == 223)',
        'commands',
        'silent',
        'set $yuf = (FTStruct *)((GObj *)$r0)->user_data.p',
        'printf "YOSHI_FORWARD status=%d motion=%d frame=%f script=%p wait=%f flag2=%d\n",$yuf->status_id,$yuf->motion_id,((GObj *)$r0)->anim_frame,$yuf->motion_scripts[0][0].p_script,$yuf->motion_scripts[0][0].script_wait,$yuf->motion_vars.flags.flag2',
        'continue',
        'end',
        'break ftYoshiSpecialHiUpdateEggVars if ((FTStruct *)((GObj *)$r0)->user_data.p)->motion_vars.flags.flag2 != 0',
        'commands',
        'silent',
        'set $yue = (FTStruct *)((GObj *)$r0)->user_data.p',
        'printf "YOSHI_EGGVAR frame=%f status=%d flag2=%u egg=%p\n",((GObj *)$r0)->anim_frame,$yue->status_id,$yue->motion_vars.flags.flag2,$yue->status_vars.yoshi.specialhi.egg_gobj',
        'continue',
        'end',
        'break wpYoshiEggThrowMakeWeapon',
        'commands',
        'silent',
        'printf "YOSHI_EGGMAKE caller=%p frame=%f\n",$r0,((GObj *)$r0)->anim_frame',
        'continue',
        'end',
        'break src/import/battleship_wpmanager_core.c:336 if ((FTStruct *)parent_gobj->user_data.p)->fkind == 6',
        'commands',
        'silent',
        'printf "YOSHI_WP_POOL wp=%p refusal=%u high=%u gmax=%d gactive=%d\n",wp,gNdsWeaponPoolRefusalCount,gNdsWeaponPoolLiveHighWater,sGCCommonsMaxNum,sGCCommonsActiveNum',
        'continue',
        'end',
        'break src/import/battleship_wpmanager_core.c:344 if ((FTStruct *)parent_gobj->user_data.p)->fkind == 6',
        'commands',
        'silent',
        'printf "YOSHI_WP_GOBJ gobj=%p refusal=%u high=%u gmax=%d gactive=%d heap=%u heapmin=%u\n",weapon_gobj,gNdsWeaponPoolRefusalCount,gNdsWeaponPoolLiveHighWater,sGCCommonsMaxNum,sGCCommonsActiveNum,(unsigned int)gSYTaskmanGeneralHeap.end-(unsigned int)gSYTaskmanGeneralHeap.ptr,gNdsTaskmanGeneralHeapFreeMin',
        'printf "YOSHI_MEM arena=%u refine=%u aobj=%u/%u effect=%u/%u animres=%u animuse=%u shield=%u pack=%u preview=%u libc=%u\n",gNdsTaskmanArenaChosenSize,gNdsTaskmanArenaRefineBytes,gNdsR2AObjPoolCount,gNdsR2AObjPoolBytes,gNdsEffectPoolDepth,gNdsEffectPoolFreeMin,gNdsR2AnimCacheArenaReservedBytes,gNdsR2AnimCacheArenaUsedBytes,gNdsShieldPoseResidentBytes,gNdsBattlePackResidentBytes,gNdsPreviewPackDataBytes,gNdsTaskmanLibcRuntimeHighWater',
        'continue',
        'end'
        )
    }
    $commands += @(
        # Unique noinline terminal after Up-B, Neutral-B, common Catch and
        # common Throw all returned naturally. The guest writes its telemetry
        # back before entering this anchor, so GDB never races cached counters.
        'tbreak ndsYoshiBugTourProofStop',
        'continue',
        'set $yg = sNdsFighterManagerLiveGObjs[0]',
        'set $yf = (FTStruct *)$yg->user_data.p',
        'set $og = sNdsFighterManagerLiveGObjs[1]',
        'set $of = (FTStruct *)$og->user_data.p',
        'printf "YOSHI_LIVE status=%d motion=%d frame=%f fig=%p script=%p wait=%f flag2=%d anim=%#x\n",$yf->status_id,$yf->motion_id,$yg->anim_frame,$yf->figatree,$yf->motion_scripts[0][0].p_script,$yf->motion_scripts[0][0].script_wait,$yf->motion_vars.flags.flag2,$yf->anim_desc.word',
        'printf "YOSHI_PAIR y=%f/%f s=%d ga=%d lr=%d floor=%d other=%f/%f s=%d ga=%d lr=%d floor=%d\n",$yf->coll_data.p_translate->x,$yf->coll_data.p_translate->y,$yf->status_id,$yf->ga,$yf->lr,$yf->coll_data.floor_line_id,$of->coll_data.p_translate->x,$of->coll_data.p_translate->y,$of->status_id,$of->ga,$of->lr,$of->coll_data.floor_line_id',
       $(if ($FastLogic) {
            'printf "YOSHI_WATCHDOG_FAST not-applicable fixture=%u\n",gNdsYoshiBugTourFixtureCount'
        } else {
            'printf "YOSHI_WATCHDOG fired=%u updates=%u total=%u go=%u phase=%u result=%#x prepared=%u enabled=%u game=%d\n",gNdsYoshiBugHostWatchdogFired,gNdsYoshiBugHostWatchdogUpdates,gNdsYoshiBugHostWatchdogTotalUpdates,gNdsYoshiBugHostWatchdogGoUpdates,gNdsYoshiBugHostWatchdogPhase,gNdsYoshiBugHostWatchdogNaturalResult,gNdsYoshiBugHostWatchdogPrepared,gNdsYoshiBugHostWatchdogUpdateEnabled,gSCManagerBattleState->game_status'
        }),
        'printf "YOSHI_RENDER cap=%u dobj=%u cb=%#x kinds=%#x submit=%u visible=%u tri=%u texready=%u texreject=%u reject=%u fail=%u/%u/%u/%#x/%#x/%#x/%u\n",gNdsWeaponRendererCaptureCount,gNdsWeaponRendererDObjDrawCount,gNdsWeaponRendererCallbackKind,gNdsWeaponRendererKindMask,gNdsWeaponRendererSubmitCount,gNdsWeaponRendererVisibleDrawCount,gNdsWeaponRendererTriangleCount,gNdsWeaponRendererTextureReadyCount,gNdsWeaponRendererTextureRejectCount,gNdsWeaponRendererRejectedDrawCount,gNdsRendererNativeFailure.count,gNdsRendererNativeFailure.domain,gNdsRendererNativeFailure.scene,gNdsRendererNativeFailure.identity,gNdsRendererNativeFailure.root,gNdsRendererNativeFailure.material,gNdsRendererNativeFailure.reason',
        'printf "YOSHI_CTOR count=%u raw=%#x mapped=%#x model=%#x live=%#x\n",gNdsYoshiEggCtorCount,gNdsYoshiEggCtorRawData,gNdsYoshiEggCtorMappedData,gNdsYoshiEggCtorModelBase,gNdsYoshiEggCtorLiveDL',
        'printf "YOSHI_PLAN gen=%u keygen=%u valid=%u program=%u selected=%u route=%u tried=%u\n",sNdsFighterStatusGeneration[0],sNdsFighterDrawPlan[0].key_status_generation,sNdsFighterDrawPlan[0].valid,sNdsFighterDrawPlan[0].data.root_program,sNdsFighterDrawPlan[0].data.collection.selected_count,gNdsFtrPlanRoute,gNdsFtrRootProgramsTried',
        'printf "YOSHI_HIDDEN anim=%#x j31=%#x/%#x j9=%#x/%#x\n",gNdsYoshiBugTourNeutralAnimDesc,gNdsYoshiBugTourNeutralJoint31,gNdsYoshiBugTourNeutralJoint31DL,gNdsYoshiBugTourNeutralJoint9,gNdsYoshiBugTourNeutralJoint9DL',
        'printf "YOSHI_VIS invisible=%u mag=%u ignore=%u bx=%#x by=%#x pass=%u fail=%u cam=%d\n",$yf->is_invisible,$yf->is_magnify_show,$yf->is_magnify_ignore,gNdsFighterDisplayContractBoundsXBits,gNdsFighterDisplayContractBoundsYBits,gNdsFighterDisplayContractBoundsPassCount,gNdsFighterDisplayContractBoundsFailCount,(gGCCurrentCamera != 0) ? gGCCurrentCamera->id : -1',
        'set $yr = (DObj *)$yg->obj',
        'set $yj9 = $yf->joints[9]',
        'set $yj31 = $yf->joints[31]',
        'printf "YOSHI_TREE root=%p flags=%#x dl=%p child=%p j9=%p flags=%#x dl=%p parent=%p j31=%p flags=%#x dl=%p parent=%p\n",$yr,$yr->flags,$yr->dl,$yr->child,$yj9,($yj9!=0)?$yj9->flags:0,($yj9!=0)?$yj9->dl:0,($yj9!=0)?$yj9->parent:0,$yj31,($yj31!=0)?$yj31->flags:0,($yj31!=0)?$yj31->dl:0,($yj31!=0)?$yj31->parent:0',
        $(if ($FastLogic) {
            'printf "YOSHI_PRESENT request=%u consume=%u\n",gNdsHarnessFastPresentRequestCount,gNdsHarnessFastPresentConsumeCount'
        } else {
            'printf "YOSHI_PRESENT realtime\n"'
        }),
        'printf "YOSHI_TOUR phase=%u done=%u inputs=%u up=%u egg=%u neutral=%u catch=%u nprog=%u ndraw=%u cwait=%u cprog=%u cdraw=%u throw=%u tprog=%u tdraw=%u tret=%u retry=%u stall=%u entry=%u shared=%u lay=%u native=%u frej=%u wrej=%u erej=%u\n",gNdsYoshiBugTourPhase,gNdsYoshiBugTourDone,gNdsYoshiBugTourInputCount,gNdsYoshiBugTourUpStatusObserved,gNdsYoshiBugTourEggThrowObserved,gNdsYoshiBugTourNeutralStatusObserved,gNdsYoshiBugTourNeutralCatchObserved,gNdsYoshiBugTourNeutralProgramObserved,gNdsYoshiBugTourNeutralDrawObserved,gNdsYoshiBugTourCatchWaitObserved,gNdsYoshiBugTourCatchProgramObserved,gNdsYoshiBugTourCatchDrawObserved,gNdsYoshiBugTourThrowStatusObserved,gNdsYoshiBugTourThrowProgramObserved,gNdsYoshiBugTourThrowDrawObserved,gNdsYoshiBugTourThrowReturnObserved,gNdsYoshiBugTourRetryCount,gNdsYoshiBugTourStallPhase,gNdsYoshiBugProofEntryEggDrawCount,gNdsYoshiBugProofSharedEggDrawCount,gNdsYoshiBugProofEggLayDrawCount,gNdsYoshiBugTourNativeFailureDelta,gNdsYoshiBugTourFighterRejectDelta,gNdsYoshiBugTourWeaponRejectDelta,gNdsYoshiBugTourEffectRejectDelta',
        'printf "YOSHI_MEMORY heap=%u heapmin=%u aobj=%u/%u latch=%u/%u/%u gmax=%d gactive=%d\n",(unsigned int)gSYTaskmanGeneralHeap.end-(unsigned int)gSYTaskmanGeneralHeap.ptr,gNdsTaskmanGeneralHeapFreeMin,gNdsR2AObjPoolCount,gNdsR2AObjPoolBytes,gNdsIFCommonGObjLatchReserveApplyCount,gNdsIFCommonGObjLatchBase,gNdsIFCommonGObjLatchLimit,sGCCommonsMaxNum,sGCCommonsActiveNum',
        'detach',
        'quit'
    )
    Invoke-GdbMarkerScript -Gdb $Gdb -Elf $elf -Root $root `
        -Commands $commands -ScriptName 'yoshi-tour-full.gdb' `
        -TimeoutSeconds $TimeoutSeconds | Out-Null

    $temp = $env:SMASH64DS_VERIFY_TEMP_DIR
    if ([string]::IsNullOrWhiteSpace($temp)) {
        $temp = Join-Path $root ('artifacts\verifier-temp\slot' + $RunnerSlot)
    }
    $outPath = Join-Path $temp 'yoshi-tour-full.gdb.out'
    $errPath = Join-Path $temp 'yoshi-tour-full.gdb.err'
    $stdout = Get-Content -LiteralPath $outPath -Raw
    $stderr = if (Test-Path $errPath) { Get-Content -LiteralPath $errPath -Raw } else { '' }
    $evidence = $stdout + $stderr
    Assert-YoshiTour ($stdout -notmatch 'YOSHI_TOUR_EXCEPTION') `
        'Yoshi tour hit an ARM exception.' $evidence
    $m = [regex]::Match($stdout,
        'YOSHI_TOUR phase=(\d+) done=(\d+) inputs=(\d+) up=(\d+) egg=(\d+) neutral=(\d+) catch=(\d+) nprog=(\d+) ndraw=(\d+) cwait=(\d+) cprog=(\d+) cdraw=(\d+) throw=(\d+) tprog=(\d+) tdraw=(\d+) tret=(\d+) retry=(\d+) stall=(\d+) entry=(\d+) shared=(\d+) lay=(\d+) native=(\d+) frej=(\d+) wrej=(\d+) erej=(\d+)')
    Assert-YoshiTour $m.Success 'Yoshi tour terminal marker missing.' $evidence
   if ($FastLogic) {
        $fixture = [regex]::Match($stdout,
            'YOSHI_WATCHDOG_FAST not-applicable fixture=(\d+)')
        Assert-YoshiTour ($fixture.Success -and
            ([uint64]$fixture.Groups[1].Value -eq 1)) `
            'Yoshi fast proof did not use exactly one bounded source-Wait fixture.' $evidence
        $present = [regex]::Match($stdout,
            'YOSHI_PRESENT request=(\d+) consume=(\d+)')
        Assert-YoshiTour ($present.Success -and
            ([uint64]$present.Groups[1].Value -gt 0) -and
            ([uint64]$present.Groups[2].Value -gt 0)) `
            'Yoshi fast proof did not consume its proof-only presentation request.' $evidence
       $watchdog = [PSCustomObject]@{
           Success = $true
            Value = $fixture.Value
       }
    } else {
        $watchdog = [regex]::Match($stdout,
            'YOSHI_WATCHDOG fired=(\d+) updates=(\d+) total=(\d+) go=(\d+) phase=(\d+) result=(0x[0-9a-fA-F]+|0) prepared=(\d+) enabled=(\d+) game=(-?\d+)')
        Assert-YoshiTour $watchdog.Success 'Yoshi host-watchdog marker missing.' $evidence
    }
    $memory = [regex]::Match($stdout,
        'YOSHI_MEMORY heap=(\d+) heapmin=(\d+) aobj=(\d+)/(\d+) latch=(\d+)/(\d+)/(\d+) gmax=(-?\d+) gactive=(\d+)')
    Assert-YoshiTour $memory.Success 'Yoshi memory/latch marker missing.' $evidence
    $v = 1..25 | ForEach-Object { [uint64]$m.Groups[$_].Value }
    Assert-YoshiTour (($v[0] -eq 15) -and ($v[1] -eq 1) -and ($v[2] -ge 4)) `
        'Yoshi tour did not reach its natural terminal phase.' `
        ($m.Value + "`n" + $watchdog.Value)
    foreach ($i in 3..15) {
        Assert-YoshiTour ($v[$i] -gt 0) `
            "Yoshi tour engagement field $i remained zero." $m.Value
    }
    if ($FastLogic) {
        Assert-YoshiTour (($v[19] -gt 0) -and ($v[20] -gt 0)) `
            'Yoshi fast diagnostic Up-B/EggLay owners emitted no hardware triangles.' $m.Value
    } else {
        Assert-YoshiTour (($v[18] -gt 0) -and ($v[19] -gt 0) -and
            ($v[20] -gt 0)) `
            'Yoshi intro/Up-B/EggLay native owners emitted no hardware triangles.' $m.Value
    }
    Assert-YoshiTour (($v[21] -eq 0) -and ($v[22] -eq 0) -and
        ($v[23] -eq 0) -and ($v[24] -eq 0)) `
        'Yoshi tour introduced native/fighter/weapon/effect renderer failures.' $m.Value
    $mv = 1..9 | ForEach-Object { [int64]$memory.Groups[$_].Value }
    Assert-YoshiTour (($mv[2] -eq 224) -and ($mv[3] -eq 8064)) `
        'Two-player Yoshi proof did not use the measured 224-entry AObj pool.' $memory.Value
    if (-not $FastLogic) {
        Assert-YoshiTour (($mv[4] -eq 1) -and ($mv[5] -gt 0) -and
            ($mv[6] -eq ($mv[5] + 8)) -and ($mv[7] -eq $mv[6])) `
            'Bounded eight-GObj post-latch reserve did not engage exactly once.' $memory.Value
    }

    $hash = (Get-FileHash -Algorithm SHA256 -LiteralPath $rom).Hash
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Artifact) |
        Out-Null
    @(
        'YOSHI_TOUR_FULL=PASS',
        "ROM_SHA256=$hash",
        "BUILD=$Build",
        "TARGET=$Target",
        $m.Value,
        $watchdog.Value,
        $memory.Value
    ) | Set-Content -LiteralPath $Artifact
    Get-Content -LiteralPath $Artifact
}
finally {
    if (($null -ne $emu) -and -not $emu.HasExited) {
        Stop-Process -Id $emu.Id -Force -ErrorAction SilentlyContinue
    }
    if ($null -ne $state) { Restore-MelonDSGdbConfig -State $state }
}
