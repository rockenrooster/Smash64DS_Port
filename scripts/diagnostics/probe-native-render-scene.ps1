[CmdletBinding()]
param(
    [Parameter(Mandatory)][ValidatePattern('^[A-Za-z0-9_-]+$')][string]$Name,
    [Parameter(Mandatory)][ValidateRange(0,31)][int]$RunnerSlot,
    [Parameter(Mandatory)][ValidateRange(0,8)][int]$StageKind,
    # 255 keeps the walk's own Mario/Fox pair, so a stage case is unchanged.
    # Any other value is an FTKind poked into the character-select walk gate;
    # the guest refuses a fighter this build does not carry, so a case naming
    # an absent fighter fails its own commit assertion rather than quietly
    # measuring Mario.
    [ValidateScript({ $_ -eq 255 -or ($_ -ge 0 -and $_ -le 11) })][int]$Fighter1Kind = 255,
    [ValidateScript({ $_ -eq 255 -or ($_ -ge 0 -and $_ -le 11) })][int]$Fighter2Kind = 255,
    [ValidateRange(1,4800)][int]$Presents = 16,
    [ValidateRange(30,3600)][int]$TimeoutSeconds = 240,
    [Parameter(Mandatory)][string]$Rom,
    [Parameter(Mandatory)][string]$Elf,
    [Parameter(Mandatory)][string]$OutputDirectory,
    [switch]$NoCapture,
    [ValidateSet('none','grab','special','jab','side_smash','linkboomerang','shieldflick','shieldroll')][string]$Pump = 'none',
    [ValidateRange(-80,80)][int]$StickX = 0,
    [ValidateRange(-80,80)][int]$StickY = 0,
    [ValidateRange(-500,500)][int]$Teleport = 0,
    [string]$Condition = '',
    [string]$Condition2 = '',
    [switch]$RequireImpactWave,
    [switch]$RequireScoreAlert,
    [switch]$FighterModelDiagnostics,
    [switch]$YoshiEffectDiagnostics,
    [switch]$StartupFailureDiagnostics,
    [ValidateRange(0,1048576)][int]$StartupMallocAtLeast = 0,
    # Raw GDB lines appended after the connection; one-off first-divergence witnesses.
    [string[]]$ExtraGdbCommands = @()
)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
. (Join-Path $root 'scripts/lib/melonds.ps1')
. (Join-Path $root 'scripts/lib/gdb-markers.ps1')
$slotDir = Join-Path $root "emulators/melonds-runners/slot$RunnerSlot"
$exe = Join-Path $slotDir 'melonDS.exe'
$config = Join-Path $slotDir 'melonDS.toml'
$output = [IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Path $output -Force | Out-Null
$watch = [Diagnostics.Stopwatch]::StartNew()
$lease = [Threading.Mutex]::new($false, "Smash64DS_DiagnosticRunner_$RunnerSlot")
$locked = $false
$emulator = $null
$originalConfig = $null
$canRestoreConfig = $true
$previousStorage = $env:SMASH64DS_VERIFY_STORAGE_DIR
$scriptName = "diagnostic_$Name.gdb"
$transcript = $null
$result = [ordered]@{ name=$Name; slot=$RunnerSlot; stage=$StageKind; presents=$Presents;
    fighter1=$Fighter1Kind; fighter2=$Fighter2Kind;
    transport='failed'; native='unobserved'; error=$null; elapsed_seconds=0 }
$exitCode = 1
try {
    try { $locked = $lease.WaitOne(0) }
    catch [Threading.AbandonedMutexException] { $locked = $true }
    if (-not $locked) { throw "Runner slot $RunnerSlot is already leased." }
    $exe = Resolve-MelonDSRepoExecutablePath -Root $root -MelonDS $exe
    if (-not (Test-Path -LiteralPath $exe -PathType Leaf)) { throw "Missing runner slot $RunnerSlot." }
    $live = @(Get-Process melonDS -ErrorAction SilentlyContinue | Where-Object { $_.Path -eq $exe })
    if ($live.Count) { throw "Runner slot $RunnerSlot already has a live emulator." }
    $Rom = (Resolve-Path -LiteralPath $Rom).Path
    $Elf = (Resolve-Path -LiteralPath $Elf).Path
    $result.rom_sha256 = (Get-FileHash -LiteralPath $Rom).Hash
    $result.elf_sha256 = (Get-FileHash -LiteralPath $Elf).Hash
    # Reject an incompatible ELF before paying the scene startup cost.
    $gdb = Join-Path 'C:/devkitPro/devkitARM/bin' 'arm-none-eabi-gdb.exe'
    $symbolArguments = @('-nx','--batch',$Elf)
    foreach ($symbol in @('ndsSceneManagerEnter','gNdsMenuShellWalkBudget',
        'gNdsMenuShellSssWalkTargetGkind','scVSBattleStartBattle',
        'ndsBattlePlayableFrameCompleteMarker','gSCManagerSceneData',
        'gSCManagerBattleState','gNdsRendererNativeFailure',
        'gNdsEntryShieldTexturePrepareDeclineCount',
        'gNdsEntryEffectNativeNoZGroupDraws','gNdsEntryShieldWitnessPolyFmt',
        'gNdsRendererPrimEnvMaskedBakeCount',
        'gNdsRendererHealSparkleCoverageSubmitCount',
        'gNdsRendererZebesAcidBindCount',
        'gNdsRendererZebesAcidWantsTexel1TrueCount',
        'gNdsRendererZebesAcidWantsTexel1FalseCount',
        'gNdsTaruCannCaptureCount','gNdsTaruCannFireInputCount',
        'gNdsTaruCannFireAutoCount','gNdsTaruCannLaunchCount',
        'gNdsTaruCannLaunchAngle','gNdsTaruCannLaunchRotate',
        'gNdsTaruCannLaunchKnockback',
        'gNdsRendererStageOwnerFirstRejectReason','gNdsRendererStageOwnerRejectCount',
        'gNdsRendererAdapterSectorArwingMtxCount','gNdsSectorArwingBasisDecline',
        'gNdsStageGCDrawAllLoopGroundActorSubmitCount',
        'gNdsStageGCDrawAllLoopGroundActorRejectCount',
        'gNdsSectorLaserCandidateStep','gNdsSectorLaserDrawCount',
        'gNdsSectorLaserSubmitFailCount',
        'gNdsCameraFrameCenterX','gNdsCameraFrameCenterY',
        'gNdsCameraFrameHalfW','gNdsCameraFrameHalfH','gNdsCameraFrameCount',
        'gNdsCameraFighterX','gNdsCameraFighterY','gNdsCameraFramePlayers',
        'gNdsCameraFrameLiveMask','gNdsCameraFrameOffMask','gNdsCameraOffCount',
        'gNdsCameraWorstFrame','gNdsCameraWorstPlayer','gNdsCameraWorstMargin',
        'gNdsCameraWorstX','gNdsCameraWorstY',
        'gNdsCameraWorstCenterX','gNdsCameraWorstCenterY',
        'gNdsCameraWorstHalfW','gNdsCameraWorstHalfH','gNdsCameraWorstMask',
        'gNdsCameraWorstLiveMask','gNdsCameraWorstFighterX',
        'gNdsCameraWorstFighterY',
        'gNdsNativeStageFilterPhase8RunCount','gNdsNativeStageFilterPhase16RunCount',
        'gNdsNativeStageFilterPhaseActivePacket','gNdsNativeStageFilterPhaseBlobPacket',
        'gNdsNativeStageRoofSnapSerial',
        'gNdsNativeStageRoofSnapValid',
        'gNdsNativeStageRoofSnapGiven',
        'gNdsNativeStageRoofSnapEmitted',
        'gNdsNativeStageNoZForeignBindingCount',
        'gNdsNativeStageNoZEnsureWorldCount',
        'gNdsNativeStageEmitShortfallCount',
        'gNdsNativeStageEmitShortfallResidue',
        'gNdsNativeStageCastleRoofClipValid','gNdsNativeStageCastleRoofClipArm',
        'gNdsNativeStageCastleRoofClipRun',
        'gNdsNativeStageCastleRoofClipCornerCount',
        'gNdsNativeStageCastleRoofClipProjectedZ','gNdsNativeStageCastleRoofClipShift',
        'gNdsNativeStageCastleRoofClipDense','gNdsNativeStageCastleRoofClipSubmitV16',
        'gNdsNativeStageCastleRoofClipResult','gNdsNativeStageCastleRoofClipFlags',
        'sNdsRendererAdapterNativeStageWorkspace',
        'gNdsInishiePakkunCandidateStep',
        'gNdsGRHyruleTwisterMapObjCount','gNdsGRHyruleTwisterCountRefusedCount',
        'gNdsGRInishiePowerBlockMapObjCount','gNdsGRInishiePowerBlockCountRefusedCount',
        'gNdsNativeFighterValidateRejectCode',
        'gNdsFtrDeclineStage',
        'gNdsFtrComposeSourceFail',
        'gNdsNativeFighterValidateRejectAbsentBinding',
        'gNdsMenuShellCssWalkTargetKind','gNdsMenuShellCssWalkTargetKind2',
        'ndsMNPlayersVSPreviewFrame','ndsMenuShellCssCommit','ndsMenuShellSssCommit',
        'sMenuTics','sMenuScreen','sMenuWalkCursor','sMenuWalkTimer','sMenuWalkHold',
        'sSssEnterCount','kNdsMenuWalkLengths','kNdsMenuWalkCss','gNdsMenuShellWalkSteps',
        'gNdsMenuShellWalkLoops','gNdsMenuShellInputCount','gNdsMenuShellTransitionCount',
        'gNdsMenuShellCssStartCount','gNdsMenuShellCssStartDeniedCount',
        'gNdsPlayersVSPreviewAcquireLoadCount','gNdsPlayersVSPreviewAcquireLoadFinishCount',
        'gNdsPlayersVSPreviewAcquireRetryCount','gNdsPlayersVSPreviewAcquireFailCount',
        'gNdsPlayersVSPreviewResidentAnimFailMask','gNdsR2AnimCacheArenaReservedBytes',
        'gNdsR2AnimCacheArenaUsedBytes','gNdsR2AnimWarmFailed',
        'gNdsAudioBgmPlaying','gNdsAudioBgmTrackID',
        'gNdsPlayersVSPreviewDwellCommitCount',
        'gNdsRendererFastOwnerTriangleCount',
        'gNdsFighterDLAllDrawP0HardwareTriangleCount',
        'gNdsFighterDLAllDrawP1HardwareTriangleCount',
        'gNdsFtrRejectCountBySlot','gNdsFtrRejectStatusBySlot',
        'gNdsFtrRejectReasonBySlot','ndsControllerPlaybackSetEnabled',
        'ndsControllerPlaybackSetConnectedMask','ndsControllerPlaybackSetPad',
        'ndsRendererSubmitNativeImpactWave')) {
        $symbolArguments += @('-ex',"info address $symbol")
    }
    $symbolOutput = & $gdb @symbolArguments 2>&1
    if ($LASTEXITCODE -ne 0) { throw "Diagnostic ELF symbol preflight failed: $symbolOutput" }
    $result.symbol_preflight = 'pass'
    if (Test-Path -LiteralPath $config) { $originalConfig = [IO.File]::ReadAllBytes($config) }
    $env:SMASH64DS_VERIFY_STORAGE_DIR = Join-Path $slotDir "diagnostics/$Name"
    $result.storage_directory = $env:SMASH64DS_VERIFY_STORAGE_DIR
    # Each child has its own process environment, GDB files, config and disk.
    Remove-Item Env:SMASH64DS_VERIFY_LOG_DIR,Env:SMASH64DS_VERIFY_TEMP_DIR -ErrorAction SilentlyContinue
    $context = Initialize-MelonDSVerifierContext -Root $root -RunnerSlot $RunnerSlot -NoBuild
    $result.arm9_port = $context.GdbPort
    $result.arm7_port = $context.Arm7Port
    Enable-MelonDSGdbConfig -MelonDSPath $exe -GdbPort $context.GdbPort `
        -Persistent -BreakOnStartup -MuteAudio | Out-Null
    $emulator = Start-Process -FilePath $exe -ArgumentList @($Rom) `
        -WorkingDirectory $slotDir -WindowStyle Hidden -PassThru
    $result.emulator_pid = $emulator.Id
    Wait-MelonDSGdbListener -Process $emulator -Port $context.GdbPort | Out-Null
    # devkitARM GDB reports the Windows host charset as CP1252 and this build
    # does not advertise UTF-8 as a supported host charset. Forcing UTF-8 makes
    # the command file fail on line 1 before it can connect. Keep GDB's host
    # charset on auto; the marker run itself decides transport success.
    $commands = @('set pagination off','set print repeats 0','set confirm off','set remotetimeout 30',
        ("target remote 127.0.0.1:{0}" -f $context.GdbPort))
    $commands += $ExtraGdbCommands
    $result.extra_gdb_commands = $ExtraGdbCommands.Count
    if ($YoshiEffectDiagnostics) {
        $commands += 'set $fttick = 0'
        $effectSource = @(Get-Content (Join-Path $root 'src/import/battleship_efmanager.c'))
        $entryStart = @(for ($i=0; $i -lt $effectSource.Count; $i++) {
            if ($effectSource[$i] -match '^GObj \*efManagerYoshiEntryEggMakeEffect\(') { $i }
        })
        $entryReturn = @(for ($i=$entryStart[0]; $i -lt $entryStart[0]+20; $i++) {
            if ($effectSource[$i] -match 'ndsEFManagerEndMappedDesc') { $i+1 }
        })
        if ($entryReturn.Count -ne 1) { throw 'Missing Yoshi entry constructor witness.' }
        $commands += @(('break battleship_efmanager.c:' + $entryReturn[0]), 'commands', 'silent', 'up',
            'printf "DIAG_YOSHI_ENTRY_OBJECT=%#x\n", effect_gobj',
            'if effect_gobj != 0',
            'printf "DIAG_YOSHI_ENTRY_OBJECT_STATE=%u,%u,%u,%f\n", effect_gobj->id, effect_gobj->dl_link_id, effect_gobj->flags, effect_gobj->anim_frame',
            'set $entry_root = (DObj*)effect_gobj->obj',
            'printf "DIAG_YOSHI_ENTRY_TREE=%#x,%#x,%#x,%u\n", $entry_root, $entry_root->dl, $entry_root->child, $entry_root->flags',
            'if $entry_root->child != 0',
            'printf "DIAG_YOSHI_ENTRY_CHILD=%#x,%#x,%u,%f\n", $entry_root->child->dl, $entry_root->child->mobj, $entry_root->child->flags, $entry_root->child->anim_wait',
            'end', 'end', 'down', 'continue', 'end')
        $commands += @('set $yoshi_entry_make = 0', 'set $yoshi_entry_base = 0',
            'set $yoshi_entry_stage = 0', 'set $yoshi_entry_native = 0',
            'break efManagerYoshiEntryEggMakeEffect', 'commands', 'silent',
            'set $yoshi_entry_make = $yoshi_entry_make + 1',
            'printf "DIAG_YOSHI_ENTRY_FILE=%#x\n", gFTDataYoshiSpecial2',
            'printf "DIAG_YOSHI_ENTRY_GOBJ_POOL=%d,%u\n", sGCCommonsMaxNum, sGCCommonsActiveNum',
            'echo DIAG_YOSHI_ENTRY_DESC=', 'output dEFManagerYoshiEntryEggEffectDesc', 'echo \n',
            'continue', 'end',
            'break ndsBaseEFManagerYoshiEntryEggMakeEffect', 'commands', 'silent',
            'set $yoshi_entry_base = $yoshi_entry_base + 1', 'continue', 'end',
            'break ndsRendererAdapterSubmitStageDL if (unsigned)dl == (unsigned)gFTDataYoshiSpecial2 + 0x530',
            'commands', 'silent',
            'if $yoshi_entry_stage == 0',
            'printf "DIAG_YOSHI_ENTRY_ROOT=%#x,%u,%#x\n", dl, dobj->parent_gobj->id, dobj->mobj',
            'if dobj->mobj != 0', 'echo DIAG_YOSHI_ENTRY_MOBJ=', 'output *dobj->mobj', 'echo \n', 'end',
            'x/25xw dl', 'end',
            'set $yoshi_entry_stage = $yoshi_entry_stage + 1', 'continue', 'end',
            'break ndsRendererSubmitNativeYoshiEntryEgg', 'commands', 'silent',
            'set $yoshi_entry_native = $yoshi_entry_native + 1', 'continue', 'end')
        # Source-location witnesses after the hardware triangle increment.
        # They work in normal builds without enabling the old proof-only tour.
        foreach ($effect in @('entryegg','egg','egglay')) {
            $source = Join-Path $root "src/nds/nds_native_yoshi_$effect.exec.inc"
            $lines = @(Get-Content -LiteralPath $source)
            $line = @(for ($i=0; $i -lt $lines.Count; $i++) {
                if ($lines[$i] -match 'stats->hardware_vertex_count \+=|if \(drawn < 0\)') { $i + 1 }
            })
            if ($line.Count -ne 1) { throw "Ambiguous Yoshi $effect post-triangle witness." }
            $commands += @(('set $yoshi_' + $effect + '_draws = 0'),
                ('set $yoshi_' + $effect + '_frame = -1'),
                ('break nds_native_yoshi_' + $effect + '.exec.inc:' + $line[0]),
                'commands','silent')
            $sharedQuad = $lines[$line[0] - 1] -match 'drawn < 0'
            if ($sharedQuad) { $commands += 'if drawn > 0' }
            $commands += @(
                ('set $yoshi_' + $effect + '_draws = $yoshi_' + $effect + '_draws + 1'),
                ('set $yoshi_' + $effect + '_frame = $fttick + 1'))
            if ($sharedQuad) { $commands += 'end' }
            $commands += @('continue','end')
        }
    }
    $startupState = @(
            'printf "DIAG_STARTUP_FIGHTER_VALIDATION=%u,%u,%u,%u\n", gNdsNativeFighterValidateRejectObserved, gNdsNativeFighterValidateRejectExpected, gNdsNativeFighterValidateRejectCount, gNdsFtrDeclineSelected',
            'echo DIAG_STARTUP_FIGHTER_ROOTS=', 'output gNdsNativeFighterValidateRejectOffsets', 'echo \n',
            'printf "DIAG_STARTUP_FIGHTER_OWNER=%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsFtrDeclineStage, gNdsFtrDeclineOwner, gNdsFtrRejectStatusBySlot[0], gNdsFtrRejectReasonBySlot[0], gNdsFtrRejectStatusBySlot[1], gNdsFtrRejectReasonBySlot[1], gNdsNativeOwnerImageFailCount, gNdsNativeFighterValidateRejectCode, gNdsNativeFighterValidateRejectRoot',
            'if (gSCManagerSceneData.scene_curr == 22) && (gSCManagerBattleState->players[1].fighter_gobj != 0)',
            'printf "DIAG_STARTUP_FIGHTER1=%d,%d,%d,%d\n", ((FTStruct*)gSCManagerBattleState->players[1].fighter_gobj->user_data.p)->fkind, ((FTStruct*)gSCManagerBattleState->players[1].fighter_gobj->user_data.p)->status_id, ((FTStruct*)gSCManagerBattleState->players[1].fighter_gobj->user_data.p)->motion_id, ((FTStruct*)gSCManagerBattleState->players[1].fighter_gobj->user_data.p)->detail_curr',
            'end',
            'printf "DIAG_STARTUP_PACK=%u,%u,%u,%u,%u,%u,%#x,%#x,%u,%#x\n", gNdsPreviewPackFailure, gNdsPreviewPackFailureKind, gNdsBattleCoreExternFailure, gNdsBattleCoreExternPatchCount, gNdsBattleCoreExternLoadCount, gNdsRelocExternalFixupFailCount, gNdsRelocExternalFixupFailFirstAsset, gNdsRelocExternalFixupFailFirstDep, gNdsRelocExternalFixupFailIndex, gNdsRelocExternalFixupFailSlot',
            'printf "DIAG_STARTUP_HEAP=%u,%u,%u,%#x,%u,%u\n", gNdsSyMallocOverflowCount, gNdsSyMallocOverflowRequest, gNdsSyMallocOverflowHeadroom, gNdsSyMallocOverflowCallerLR, gNdsTaskmanArenaChosenSize, gNdsTaskmanGeneralHeapFreeMin',
            'printf "DIAG_STARTUP_STAGE=%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsNativeStageBlobReadFailCount, gNdsNativeStageBlobHashMismatchCount, gNdsNativeStagePrepareRunFailStep, gNdsNativeStagePrepareRunFailRun, gNdsNativeStageValidateFullFailStep, gNdsNativeStageValidateFullFailIndex, gNdsNativeStagePacketUnresolvedCount, gNdsNativeStagePacketUnresolvedKind',
            'printf "DIAG_STARTUP_RESOURCE=%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsTaskmanLibcTopChunkMin, gNdsTaskmanLibcRuntimeHighWater, gNdsFtPoseTrackOverflow, gNdsRendererFastFallbackCount[0], gNdsRendererFastFallbackCount[1], gNdsRendererFastFallbackCount[2], gNdsEntryEffectNativeFallbackCount, gNdsRendererStageOwnerRejectCount',
            'printf "DIAG_ENTRY_DESC=%#x,%#x,%#x,%u,%u,%u,%u\n", gFTDataFoxSpecial3, dEFManagerFoxEntryArwingEffectDesc.proc_display, dEFManagerFoxEntryArwingEffectDesc.o_dobjsetup, sNdsEFDeferredCount, gNdsEFDescDisabledCount, gNdsEFDescDeferRecoverCount, gNdsTaskmanHeapGeneration',
            'set $entry_i = 0',
            'while $entry_i < sNdsEFDeferredCount',
            'if sNdsEFDeferredDescs[$entry_i] == &dEFManagerFoxEntryArwingEffectDesc',
            'printf "DIAG_ENTRY_DEFERRED=%u,%#x\n", $entry_i, sNdsEFDeferredProcs[$entry_i]',
            'end',
            'set $entry_i = $entry_i + 1',
            'end',
            'set $entry_i = 0',
            'while $entry_i < sNdsRelocLoadedFileCount',
            'if sNdsRelocLoadedFiles[$entry_i].asset_id == 161',
            'printf "DIAG_ENTRY_FILE=%#x,%u,%u,%u\n", sNdsRelocLoadedFiles[$entry_i].data, sNdsRelocLoadedFiles[$entry_i].data_size, sNdsRelocLoadedFiles[$entry_i].owner_scene, sNdsRelocLoadedFiles[$entry_i].owner_generation',
            'end',
            'set $entry_i = $entry_i + 1',
            'end'
    )
    if ($StartupFailureDiagnostics) {
        foreach ($halt in @('ndsPreviewPackLoadHalt', 'ndsBattleCoreExternHalt',
                            'ndsSyMallocOverflowHalt', '__excpt_entry')) {
            $commands += @(('break ' + $halt), 'commands', 'silent',
                ('printf "DIAG_STARTUP_FAILURE=' + $halt + ',r0:%#x,r1:%#x,pc:%#x,lr:%#x\n", $r0, $r1, $pc, $lr')) +
                $startupState + @('info registers', 'bt 24', 'quit 2', 'end')
        }
    }
    if ($StartupMallocAtLeast -ne 0) {
        $commands += @(
            ('break syMallocSet if (gSCManagerSceneData.scene_curr == 22) && (bp == &gSYTaskmanGeneralHeap) && (size >= ' + $StartupMallocAtLeast + ')'),
            'commands','silent',
            'printf "DIAG_STARTUP_ALLOC=size:%u,free:%u,lr:%#x\n", (unsigned)size, (unsigned)gSYTaskmanGeneralHeap.end-(unsigned)gSYTaskmanGeneralHeap.ptr, $lr',
            'bt 4','continue','end')
    }
    $commands += @(
        'break ndsSceneManagerEnter','commands','silent',
        'printf "DIAG_WALK_SCENE=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gSCManagerSceneData.scene_curr, gSCManagerSceneData.scene_prev, gNdsMenuShellWalkSteps, gNdsMenuShellInputCount, gNdsMenuShellTransitionCount, gNdsMenuShellCssStartCount, gNdsMenuShellCssStartDeniedCount, gNdsPlayersVSPreviewAcquireLoadCount, gNdsPlayersVSPreviewAcquireLoadFinishCount, gNdsPlayersVSPreviewAcquireRetryCount, gNdsPlayersVSPreviewDwellCommitCount',
        'set variable gNdsMenuShellWalkBudget = 1',
        'set variable gNdsNativeStageCastleRoofClipArm = 1',
        ('set variable gNdsMenuShellSssWalkTargetGkind = ' + $StageKind),
        ('set variable gNdsMenuShellCssWalkTargetKind = ' + $Fighter1Kind),
        ('set variable gNdsMenuShellCssWalkTargetKind2 = ' + $Fighter2Kind),
        'continue','end',
        # A native-render probe needs the real Title -> Mode -> VS -> CSS -> SSS
        # routing, CSS START readiness/commit, and SSS stage commit. It does not
        # need Boundary's 32-step CSS production tour or its one-time SSS B
        # back-out proof. Since d99a89f8741 made a preview miss synchronous,
        # those two unrelated proofs can spend many cold closure transactions
        # before the battle marker this script is trying to measure.
        #
        # The first PreviewFrame is inside ndsMenuShellRun, after that function
        # reset the walk statics. Point the existing walk at its final START and
        # wait just past CSS's source-derived 60-tic START arm. Setting the SSS
        # entry counter to one here makes ndsMenuShellSssInit increment it to two,
        # so the first SSS visit takes the existing target-seek/confirm path.
        # No shipping or Boundary behavior changes: these are debugger writes in
        # this diagnostic process only.
        'tbreak ndsMNPlayersVSPreviewFrame','commands','silent',
        # NDS_INPUT_START is bit 5. Refuse to fast-forward if the walk's shape
        # ever changes so this diagnostic cannot silently steer at a new final
        # action and recreate the same marker-timeout failure.
        'if kNdsMenuWalkCss[kNdsMenuWalkLengths[3] - 1].button != 32',
        'printf "DIAG_WALK_FAST_BAD_FINAL=%u,%u\n", kNdsMenuWalkLengths[3], kNdsMenuWalkCss[kNdsMenuWalkLengths[3] - 1].button',
        'quit 1',
        'end',
        'printf "DIAG_WALK_CSS_FAST=%u,%u,%u,%u,%u,%u,%u\n", sMenuTics, sMenuWalkCursor, sMenuWalkTimer, kNdsMenuWalkLengths[3], gNdsPlayersVSPreviewAcquireLoadCount, gNdsPlayersVSPreviewAcquireLoadFinishCount, gNdsPlayersVSPreviewAcquireRetryCount',
        'printf "DIAG_CSS_CACHE_FAST=load:%u finish:%u fail:%u anim:%08x reserve:%u used:%u warmfail:%u\n", gNdsPlayersVSPreviewAcquireLoadCount, gNdsPlayersVSPreviewAcquireLoadFinishCount, gNdsPlayersVSPreviewAcquireFailCount, gNdsPlayersVSPreviewResidentAnimFailMask, gNdsR2AnimCacheArenaReservedBytes, gNdsR2AnimCacheArenaUsedBytes, gNdsR2AnimWarmFailed',
        'set variable sMenuWalkCursor = kNdsMenuWalkLengths[3] - 1',
        'set variable sMenuWalkTimer = 61',
        'set variable sMenuWalkHold = 0',
        'set variable sSssEnterCount = 1',
        'continue','end',
        # These two one-shot stops make a future timeout self-locating. A
        # transcript that has CSS_FAST but no CSS_COMMIT stopped before the
        # accepted START completed; CSS_COMMIT with no SSS_COMMIT stopped in
        # stage select. Scene-entry lines bracket every successful hand-off.
        'tbreak ndsMenuShellCssCommit','commands','silent',
        'printf "DIAG_WALK_CSS_COMMIT=%u,%u,%u,%u,%u,%u,%u,%u\n", sMenuTics, gNdsMenuShellWalkSteps, gNdsMenuShellCssStartCount, gNdsMenuShellCssStartDeniedCount, gNdsPlayersVSPreviewAcquireLoadCount, gNdsPlayersVSPreviewAcquireLoadFinishCount, gNdsPlayersVSPreviewAcquireRetryCount, gNdsPlayersVSPreviewDwellCommitCount',
        'printf "DIAG_CSS_CACHE_COMMIT=load:%u finish:%u fail:%u anim:%08x reserve:%u used:%u warmfail:%u\n", gNdsPlayersVSPreviewAcquireLoadCount, gNdsPlayersVSPreviewAcquireLoadFinishCount, gNdsPlayersVSPreviewAcquireFailCount, gNdsPlayersVSPreviewResidentAnimFailMask, gNdsR2AnimCacheArenaReservedBytes, gNdsR2AnimCacheArenaUsedBytes, gNdsR2AnimWarmFailed',
        'printf "DIAG_CSS_BGM_COMMIT=playing:%u track:%u\n", gNdsAudioBgmPlaying, gNdsAudioBgmTrackID',
        'continue','end',
        'tbreak ndsMenuShellSssCommit','commands','silent',
        'printf "DIAG_WALK_SSS_COMMIT=%u,%u,%u,%u\n", sMenuTics, gNdsMenuShellWalkSteps, gNdsMenuShellWalkLoops, gNdsMenuShellSssWalkTargetGkind',
        'continue','end')
    if ($env:NDS_SECTOR_BASIS_ENTRY_DIAG -eq '1') {
        $commands += @(
            'set $sector_basis_hits = 0',
            'break ndsRendererAdapterSectorArwingBasis','commands','silent',
            'set $sector_basis_hits = $sector_basis_hits + 1',
            'printf "DIAG_SECTOR_BASIS_ENTRY=%u,%p,%p,%p,%p,%p,%p,%d,%d\n", $sector_basis_hits, dobj, dobj->parent_gobj, dobj->aobj, gGRCommonStruct.sector.map_dobjs[11], gSYTaskmanGeneralHeap.start, gSYTaskmanGeneralHeap.ptr, gGRCommonStruct.sector.arwing_laser_count, gGRCommonStruct.sector.arwing_appear_timer',
            'if $sector_basis_hits >= 8','quit','end','continue','end')
    }
    $commands += 'set $impact_wave_hits = 0'
    if ($RequireImpactWave) {
        # Count execution of the real native owner, not a guest/cached counter.
        # Keep this opt-in so unrelated action probes do not pay breakpoint
        # overhead for effects they never intend to exercise.
        $commands += @(
            'break ndsRendererSubmitNativeImpactWave','commands','silent',
            'set $impact_wave_hits = $impact_wave_hits + 1','continue','end')
    }
    if ($Condition -eq '') {
        # Legacy path: fixed present count, no forced input. Unchanged.
        # `tbreak` deletes itself. A bare GDB `delete` here used to remove the
        # pack/OOM/exception diagnostics before battle setup actually ran.
        $commands += @('tbreak scVSBattleStartBattle','continue')
        if ($Presents -gt 1) {
            $commands += @('tbreak ndsBattlePlayableFrameCompleteMarker',
                ('ignore $bpnum ' + ($Presents - 2)), 'continue',
                'set $p0tri_prev = gNdsFighterDLAllDrawP0HardwareTriangleCount',
                'set $p1tri_prev = gNdsFighterDLAllDrawP1HardwareTriangleCount')
        } else {
            $commands += @('set $p0tri_prev = 0', 'set $p1tri_prev = 0')
        }
        $commands += @('tbreak ndsBattlePlayableFrameCompleteMarker',
            'continue')
    } else {
        # Action path: force controller playback at battle start, then pump
        # inputs every frame and stop on the fighter's own state, not a frame
        # count. GDB runs the pump + condition inside one breakpoint's command
        # list: while the condition is false it continues, so the first stop
        # is provably during the action. Wall-clock TimeoutSeconds bounds a
        # condition that never fires.
        if ($Condition.Contains("`n") -or $Condition2.Contains("`n")) { throw 'Conditions must be single lines.' }
        if ($Condition2 -ne '' -and $Condition -eq '') { throw 'Condition2 requires Condition.' }
        $commands += @('set $fttick = 0','set $ftphase = 0',
            # DTCM input writes, as in probe-p2-campaign. Inferior function
            # calls can stop inside the ARM/Thumb GDB call trampoline.
            'define diag_pad',
            'set variable sControllerPlaybackPads[$arg0].button = $arg1',
            'set variable sControllerPlaybackPads[$arg0].stick_x = $arg2',
            'set variable sControllerPlaybackPads[$arg0].stick_y = $arg3',
            'end',
            'tbreak scVSBattleStartBattle','continue',
            'tbreak ndsBattlePlayableFrameCompleteMarker','continue',
            'set variable sControllerPlaybackEnabled = 1',
            'set variable sControllerPlaybackConnectedMask = 1',
            'diag_pad 0 0 0 0',
            'set $p0tri_prev = gNdsFighterDLAllDrawP0HardwareTriangleCount',
            'set $p1tri_prev = gNdsFighterDLAllDrawP1HardwareTriangleCount',
            'set $p0rej_prev = gNdsFtrRejectCountBySlot[0]',
            'set $p1rej_prev = gNdsFtrRejectCountBySlot[1]',
            'set $native_prev = gNdsRendererNativeFailure.count',
            'break ndsBattlePlayableFrameCompleteMarker',
            'commands','silent',
            'set $fttick = $fttick + 1')
        switch ($Pump) {
            'side_smash' {
                $commands += @('if (gSCManagerBattleState->time_passed >= 140) && (($fttick % 60) < 2)',
                    ('diag_pad 0 0x8000 ' + $StickX + ' 0'),
                    'else', 'diag_pad 0 0 0 0', 'end')
            }
            'jab' {
                # Start after GO has cleared so the first short-lived burst
                # can be captured unobscured, not from a stale draw counter.
                $commands += @('if (gSCManagerBattleState->time_passed >= 140) && (($fttick % 6) < 2)',
                    'diag_pad 0 0x8000 0 0', 'else', 'diag_pad 0 0 0 0', 'end')
            }
            'grab' {
                # Z held always (shield); A tapped 2 of every 30 frames. Grab
                # from common ground needs Z hold + A tap
                # (ftcommoncatch1.c:134); a static pad taps once and Entry
                # eats it, so the tap must repeat. Victim stays pinned beside
                # the attacker until acquisition: p_translate is the fighter
                # world position (map.h:34) of Vec3f x,y,z (ssb_types.h:10).
                $commands += @('if (($fttick % 30) < 2)',
                    'diag_pad 0 0xA000 0 0')
                $commands += @('else',
                    'diag_pad 0 0x2000 0 0',
                    'end')
                if ($Teleport -ne 0) {
                    # Pin the target in front of the attacker. The shell can
                    # hand either fighter facing direction into the battle.
                    $commands += @(('set variable ((FTStruct*)gSCManagerBattleState->players[1].fighter_gobj->user_data.p)->coll_data.p_translate->x = ((FTStruct*)gSCManagerBattleState->players[0].fighter_gobj->user_data.p)->coll_data.p_translate->x + (((FTStruct*)gSCManagerBattleState->players[0].fighter_gobj->user_data.p)->lr * ' + $Teleport + ')'),
                        'set variable ((FTStruct*)gSCManagerBattleState->players[1].fighter_gobj->user_data.p)->coll_data.p_translate->y = ((FTStruct*)gSCManagerBattleState->players[0].fighter_gobj->user_data.p)->coll_data.p_translate->y')
                }
            }
            'special' {
                # B pulsed 2 of every 40 frames ( specials trigger on tap;
                # a static hold taps once and Entry eats it). Stick held
                # during the pulse selects the variant (down for down-B).
                $commands += @('if (($fttick % 40) < 2)',
                    ('diag_pad 0 0x4000 ' + $StickX + ' ' + $StickY))
                $commands += @('else',
                    'diag_pad 0 0 0 0',
                    'end')
                if ($Teleport -ne 0) {
                    $commands += @(('set variable ((FTStruct*)gSCManagerBattleState->players[1].fighter_gobj->user_data.p)->coll_data.p_translate->x = ((FTStruct*)gSCManagerBattleState->players[0].fighter_gobj->user_data.p)->coll_data.p_translate->x + (((FTStruct*)gSCManagerBattleState->players[0].fighter_gobj->user_data.p)->lr * ' + $Teleport + ')'),
                        'set variable ((FTStruct*)gSCManagerBattleState->players[1].fighter_gobj->user_data.p)->coll_data.p_translate->y = ((FTStruct*)gSCManagerBattleState->players[0].fighter_gobj->user_data.p)->coll_data.p_translate->y')
                }
            }
            'linkboomerang' {
                # Pulse B until phase 1 proves the boomerang exists, then stop
                # input so that same weapon can return and trigger SpecialNGet.
                $commands += @('if $ftphase == 0',
                    'if (($fttick % 40) < 2)',
                    'diag_pad 0 0x4000 0 0',
                    'else','diag_pad 0 0 0 0',
                    'end','else','diag_pad 0 0 0 0',
                    'end')
            }
            'shieldflick' {
                # Z held always; stick flicked 10 of every 20 frames. Escape
                # needs a stick edge during guard, which a static full-deflect
                # never provides.
                $commands += @('if (($fttick % 20) < 10)',
                    ('diag_pad 0 0x2000 ' + $StickX + ' 0'),
                    'else',
                    'diag_pad 0 0x2000 0 0',
                    'end')
            }
            'shieldroll' {
                # Establish Guard with neutral Z before introducing a stick
                # edge. This proves the escape came from shield rather than a
                # coincident dash/catch path.
                $commands += @('if $ftphase == 0',
                    'diag_pad 0 0x2000 0 0',
                    'else','if (($fttick % 20) < 2)',
                    ('diag_pad 0 0x2000 ' + $StickX + ' 0'),
                    'else','diag_pad 0 0x2000 0 0',
                    'end','end')
            }
            default {
                $commands += @('diag_pad 0 0 ' + $StickX + ' ' + $StickY)
            }
        }
        # A condition hit is read on the frame-complete marker. Delta the
        # per-player triangle and reject counters against the previous marker
        # so a stale run-wide total cannot turn an action-frame skip into PASS.
        $phasePrint = 'printf \"DIAG_ACTION_PHASE=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\\n\", $ftphase + 1, $fttick, ((FTStruct*)gSCManagerBattleState->players[0].fighter_gobj->user_data.p)->status_id, ((FTStruct*)gSCManagerBattleState->players[0].fighter_gobj->user_data.p)->is_invisible, ((FTStruct*)gSCManagerBattleState->players[0].fighter_gobj->user_data.p)->is_shield, ((FTStruct*)gSCManagerBattleState->players[0].fighter_gobj->user_data.p)->is_jostle_ignore, ((FTStruct*)gSCManagerBattleState->players[1].fighter_gobj->user_data.p)->status_id, ((FTStruct*)gSCManagerBattleState->players[1].fighter_gobj->user_data.p)->is_invisible, ((FTStruct*)gSCManagerBattleState->players[0].fighter_gobj->user_data.p)->catch_gobj == gSCManagerBattleState->players[1].fighter_gobj, ((FTStruct*)gSCManagerBattleState->players[1].fighter_gobj->user_data.p)->capture_gobj == gSCManagerBattleState->players[0].fighter_gobj, gNdsFighterDLAllDrawP0HardwareTriangleCount - $p0tri_prev, gNdsFighterDLAllDrawP1HardwareTriangleCount - $p1tri_prev, gNdsFtrRejectCountBySlot[0] - $p0rej_prev, gNdsFtrRejectCountBySlot[1] - $p1rej_prev, gNdsRendererNativeFailure.count - $native_prev, gNdsFtrDeclineStage, gNdsFtrDeclineOwner, gNdsFtrRejectStatusBySlot[0], gNdsFtrRejectReasonBySlot[0], gNdsNativeFighterValidateRejectCode, gNdsNativeFighterValidateRejectRoot, gNdsNativeFighterValidateRejectObserved, gNdsNativeFighterValidateRejectExpected'
        $phasePrint = $phasePrint.Replace('\"','"').Replace('\\n','\n')
        $baseline = @('set $p0tri_prev = gNdsFighterDLAllDrawP0HardwareTriangleCount',
            'set $p1tri_prev = gNdsFighterDLAllDrawP1HardwareTriangleCount',
            'set $p0rej_prev = gNdsFtrRejectCountBySlot[0]',
            'set $p1rej_prev = gNdsFtrRejectCountBySlot[1]',
            'set $native_prev = gNdsRendererNativeFailure.count')
        if ($Condition2 -eq '') {
            $commands += @(('if ' + $Condition),$phasePrint,
            'echo DIAG_COND_MET', 'echo \n',
            'else')
            $commands += $baseline
            $commands += @('continue','end','end','continue')
        } else {
            $commands += @('if $ftphase == 0',('if ' + $Condition),$phasePrint,
                'echo DIAG_COND_MET', 'echo \n','set $ftphase = 1')
            $commands += $baseline
            $commands += @('continue','else')
            $commands += $baseline
            $commands += @('continue','end','else',('if ' + $Condition2),$phasePrint,
                'echo DIAG_COND2_MET', 'echo \n','else')
            $commands += $baseline
            $commands += @('continue','end','end','end','continue')
        }
    }
    if ($StartupFailureDiagnostics) {
        $commands += $startupState + @(
            'printf "DIAG_STARTUP_GO=%u,%u,%u,%u\n", gNdsK0BattleInGo, gSCManagerBattleState->time_passed, gNdsFighterDLAllDrawP0HardwareTriangleCount, gNdsFighterDLAllDrawP1HardwareTriangleCount',
            'printf "DIAG_SHIELD_POSE=%u,%u,%u\n", gNdsShieldPoseLoadCount, gNdsShieldPoseLoadFailCount, gNdsShieldPoseResidentBytes',
            'echo DIAG_ENTRY_ROOT_DRAWS=', 'output gNdsEntryEffectNativeRootDraws', 'echo \n')
    }
    if ($FighterModelDiagnostics) {
        $commands += @('echo DIAG_DIRECT_REJECT=', 'output gNdsRendererNativeDirectReject', 'echo \n')
        if ($Fighter1Kind -eq 9 -or $Fighter2Kind -eq 9) {
            $commands += @('printf "DIAG_PIKACHU_THUNDER=%u,%u,%u,%u\n", gNdsPikachuThunderNativeRoleMask, gNdsPikachuThunderNativeDraws[0], gNdsPikachuThunderNativeDraws[1], gNdsPikachuThunderNativeDraws[2]')
        }
        $commands += @(
            'set $model_fp = (FTStruct*)gSCManagerBattleState->players[0].fighter_gobj->user_data.p',
            'printf "DIAG_MODEL=%d,%d,%d,%d,%u,%#x,%#x,%u\n", $model_fp->fkind, $model_fp->status_id, $model_fp->motion_id, $model_fp->detail_curr, $model_fp->is_invisible, *$model_fp->data->p_file_main, *$model_fp->data->p_file_model, gNdsTaskmanHeapGeneration',
            'echo DIAG_ROOT_PROGRAMS=', 'output/u sNdsNativeFighterRootPrograms', 'echo \n',
            'echo DIAG_OWNER_IMAGES=', 'output sNdsNativeOwnerImage', 'echo \n',
            'set $joint_i = 0',
            'while $joint_i < sizeof($model_fp->joints)/sizeof($model_fp->joints[0])',
            'set $joint = $model_fp->joints[$joint_i]',
            'if ($joint != 0) && ($joint->dl != 0)',
            'printf "DIAG_JOINT=%u,%#x,%#x,%u\n", $joint_i, $joint, $joint->dl, ((FTParts*)$joint->user_data.p)->flags',
            'if ((unsigned)$joint->dl >= 0x02000000) && ((unsigned)$joint->dl < 0x02400000)',
            'x/2wx $joint->dl',
            'end','end',
            'set $joint_i = $joint_i + 1','end')
    }
    if ($YoshiEffectDiagnostics) {
        $commands += 'printf "DIAG_YOSHI_ENTRY_MAKERS=%u,%u,%u,%u\n", $yoshi_entry_make, $yoshi_entry_base, $yoshi_entry_stage, $yoshi_entry_native'
        $commands += @('printf "DIAG_YOSHI_EFFECTS=%u,%u,%u,%u\n", $yoshi_entryegg_draws, $yoshi_egg_draws, $yoshi_egglay_draws, sNdsNativeFighterRootPrograms[8]')
    }
    if ($RequireScoreAlert) {
        $commands += 'printf "DIAG_SCORE_ALERT=%u,%u,%u,%u\n", gNdsBattleHudScoreSubmitCount, gNdsBattleHudScoreDrawCount, gNdsBattleHudScoreFrameMask, sNdsBattleHudScoreCount'
    }
    $commands += @(
        'printf "DIAG_DRAW=%u,%u,%u,%u\n", gNdsFighterDLAllDrawP0HardwareTriangleCount-$p0tri_prev, gNdsFighterDLAllDrawP1HardwareTriangleCount-$p1tri_prev, ((FTStruct*)gSCManagerBattleState->players[0].fighter_gobj->user_data.p)->is_invisible, ((FTStruct*)gSCManagerBattleState->players[1].fighter_gobj->user_data.p)->is_invisible',
        'printf "DIAG_STATE=%u,%u,%u,%u,%u\n", gSCManagerSceneData.scene_curr, gSCManagerBattleState->gkind, gSCManagerBattleState->time_passed, gSCManagerBattleState->pl_count, gSCManagerBattleState->cp_count',
        'printf "DIAG_CAMFRAM=%f,%f,%f,%f,%u\n", gNdsCameraFrameCenterX, gNdsCameraFrameCenterY, gNdsCameraFrameHalfW, gNdsCameraFrameHalfH, gNdsCameraFrameCount',
        'printf "DIAG_CAMFRAM2=%u,%u,%#x,%#x,%u,%u,%#x,%#x,%f,%f,%f,%f,%f,%f,%f\n", gNdsCameraFramePlayers, gNdsCameraOffCount, gNdsCameraFrameLiveMask, gNdsCameraFrameOffMask, gNdsCameraWorstFrame, gNdsCameraWorstPlayer, gNdsCameraWorstMask, gNdsCameraWorstLiveMask, gNdsCameraWorstMargin, gNdsCameraWorstX, gNdsCameraWorstY, gNdsCameraWorstCenterX, gNdsCameraWorstCenterY, gNdsCameraWorstHalfW, gNdsCameraWorstHalfH',
        'printf "DIAG_CAMFRAM3=%f,%f,%f,%f,%f,%f,%f,%f\n", gNdsCameraFighterX[0], gNdsCameraFighterY[0], gNdsCameraFighterX[1], gNdsCameraFighterY[1], gNdsCameraFighterX[2], gNdsCameraFighterY[2], gNdsCameraFighterX[3], gNdsCameraFighterY[3]',
        'printf "DIAG_CAMFRAM4=%f,%f,%f,%f,%f,%f,%f,%f\n", gNdsCameraWorstFighterX[0], gNdsCameraWorstFighterY[0], gNdsCameraWorstFighterX[1], gNdsCameraWorstFighterY[1], gNdsCameraWorstFighterX[2], gNdsCameraWorstFighterY[2], gNdsCameraWorstFighterX[3], gNdsCameraWorstFighterY[3]',
        'printf "DIAG_NATIVE=%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsRendererNativeFailure.count, gNdsRendererNativeFailure.domain, gNdsRendererNativeFailure.scene, gNdsRendererNativeFailure.identity, gNdsRendererNativeFailure.status, gNdsRendererNativeFailure.root, gNdsRendererNativeFailure.material, gNdsRendererNativeFailure.reason',
        'printf "DIAG_IMPACTWAVE=%u\n", $impact_wave_hits',
        'printf "DIAG_STAGE_OWNER=%u,%u,%u\n", gNdsRendererStageOwnerFirstRejectReason, gNdsRendererStageOwnerRejectCount, sNdsRendererAdapterNativeStageWorkspace.dobj_count',
        'printf "DIAG_SECTOR_ARWING_MTX=%u\n", gNdsRendererAdapterSectorArwingMtxCount',
        'printf "DIAG_SECTOR_ARWING_BASIS=%u,%u,%u\n", gNdsSectorArwingBasisDecline, gNdsStageGCDrawAllLoopGroundActorSubmitCount, gNdsStageGCDrawAllLoopGroundActorRejectCount',
        'printf "DIAG_SECTOR_LASER=%u,%u,%u\n", gNdsSectorLaserCandidateStep, gNdsSectorLaserDrawCount, gNdsSectorLaserSubmitFailCount',
        'printf "DIAG_STAGE_RIGID=%#llx\n", sNdsRendererAdapterNativeStageWorkspace.task36_runtime_rigid_mask',
        # Run 9 is Castle binding 3's nine-triangle upper roof packet. The DS
        # treats polygon alpha 0 as wireframe, so expose the exact prepared
        # format that reaches BeginRun rather than inferring alpha from source
        # vertex colours or the N64 combine state.
        'printf "DIAG_STAGE_RUN9_POLY=%#x,%u,%u,%u,%u,%#x,%#x\n", sNdsNativeStageOwnerExecution.runs[9].poly_fmt, (sNdsNativeStageOwnerExecution.runs[9].poly_fmt >> 16) & 31, sNdsNativeStageOwnerExecution.runs[9].textured, sNdsNativeStageOwnerExecution.runs[9].alpha_test, sNdsNativeStageOwnerExecution.runs[9].alpha_ref, sNdsNativeStageOwnerExecution.runs[9].texture_name, sNdsNativeStageOwnerExecution.runs[9].texture_params',
        # Which fighters actually committed, and whether they drew. A fighter
        # case that never commits its kind is measuring Mario; one that commits
        # and emits no owner triangles is a successful empty draw, which the
        # native-only contract forbids just as much as a recorded failure.
        'printf "DIAG_FIGHTER=%u,%u,%d,%d\n", gSCManagerBattleState->players[0].fkind, gSCManagerBattleState->players[1].fkind, gSCManagerBattleState->players[0].total_damage_all, gSCManagerBattleState->players[1].total_damage_given',
        'echo DIAG_OWNERTRI=', 'output gNdsRendererFastOwnerTriangleCount', 'echo \n',
        # A fighter-domain REJECTED_PROGRAM means a native owner was declined at
        # validate. These name which check, which slot and what it expected
        # against what it observed, which is the difference between guessing at
        # a missing model-part variant and knowing the pair.
        'printf "DIAG_FTREJECT_SLOT=%u,%u,%u,%u,%u,%u\n", gNdsFtrRejectCountBySlot[0], gNdsFtrRejectCountBySlot[1], gNdsFtrRejectStatusBySlot[0], gNdsFtrRejectStatusBySlot[1], gNdsFtrRejectReasonBySlot[0], gNdsFtrRejectReasonBySlot[1]',
        'printf "DIAG_FTREJECT=%u,%u,%#x,%#x,%#x,%#x\n", gNdsNativeFighterValidateRejectCode, gNdsNativeFighterValidateRejectSlot, gNdsNativeFighterValidateRejectLow, gNdsNativeFighterValidateRejectRoot, gNdsNativeFighterValidateRejectObserved, gNdsNativeFighterValidateRejectExpected',
        'printf "DIAG_FTDECLINE=%u,%u,%u,%u,%#x,%#x\n", gNdsFtrDeclineStage, gNdsFtrDeclineOwner, gNdsFtrDeclineSelected, gNdsFtrDeclineIndex, gNdsFtrDeclineAssetId, gNdsFtrDeclineDetail',
        'printf "DIAG_FTROOTS=%u\n", gNdsNativeFighterValidateRejectCount',
        # Action witness: attacker status / invisibility / shield plus victim
        # status / invisibility, read off the live FTStructs through the
        # battle state's fighter GObjs (sctypes.h:342, fighter.h:3707,3715).
        # This is what separates DECLINED (FTDECLINE/FTREJECT move) from
        # silently SKIPPED (attacker in action status with is_invisible set,
        # no record) from SUBMITTED (owner triangles nonzero, no decline).
        'printf "DIAG_ACTION=%d,%u,%u,%d,%u\n", ((FTStruct*)gSCManagerBattleState->players[0].fighter_gobj->user_data.p)->status_id, ((FTStruct*)gSCManagerBattleState->players[0].fighter_gobj->user_data.p)->is_invisible, ((FTStruct*)gSCManagerBattleState->players[0].fighter_gobj->user_data.p)->is_shield, ((FTStruct*)gSCManagerBattleState->players[1].fighter_gobj->user_data.p)->status_id, ((FTStruct*)gSCManagerBattleState->players[1].fighter_gobj->user_data.p)->is_invisible',
        # Decline stage 7 is the production matrices contract, which is one
        # call with six ways to fail. These say which, plus the live joint
        # count that the JOINT_MAX bound rejected.
        'printf "DIAG_FTCOMPOSE=%u,%u,%u,%#x\n", gNdsFtrComposeSourceFail, gNdsFtrComposeSourceJoints, gNdsFtrComposeSourceIndex, gNdsFtrComposeSourceAngle',
        'printf "DIAG_FTSETDELTA=%#x,%#x,%#x\n", gNdsNativeFighterValidateRejectAbsentBinding, gNdsNativeFighterValidateRejectForeignIndex, gNdsNativeFighterValidateRejectForeignOffset',
        'echo DIAG_FTVEC=', 'output gNdsNativeFighterValidateRejectOffsets', 'echo \n',
        # Stage-owner reject reason 6 means ndsRendererPrepareNativeStageOwner
        # returned FALSE for the whole stage; these are the steps inside it, so
        # the same run that reports the reject also says which step declined.
        'printf "DIAG_STAGE_PREP=%u,%u,%#x,%u,%u,%u,%u\n", gNdsNativeStageOwnerPrepareFailStep, gNdsNativeStageOwnerPrepareFailSegment, gNdsNativeStageOwnerPrepareGuardMask, gNdsNativeStagePrepareRunFailStep, gNdsNativeStagePrepareRunFailRun, gNdsNativeStageValidateFullFailStep, gNdsNativeStageValidateFullFailIndex',
        # Step 2 is the texture resolve. Its operands and the static corpus's
        # own counters say whether the pin set was prepared at all, whether it
        # was violated, and which image the run could not resolve.
        'printf "DIAG_PAKKUN=%u,%#x,%#x,%u,%u,%u,%#x,%#x,%#x,%#x\n", gNdsInishiePakkunCandidateStep, gNdsInishiePakkunMaterialFlags, gNdsInishiePakkunEffects, gNdsInishiePakkunDrawCount, gNdsInishiePakkunSubmitFailCount, gNdsInishiePakkunSubmitStep, gNdsInishiePakkunImageW0, gNdsInishiePakkunImage, gNdsInishiePakkunProjection, gNdsInishiePakkunModelview',
        'printf "DIAG_STAGE_HAZARD_GUARD=%u,%u,%u,%u\n", gNdsGRHyruleTwisterMapObjCount, gNdsGRHyruleTwisterCountRefusedCount, gNdsGRInishiePowerBlockMapObjCount, gNdsGRInishiePowerBlockCountRefusedCount',
        # The particle env-colour variant cache. The KO blast pillar's pixel is
        # (PRIM - ENV) * TEXEL + ENV, so dropping ENV collapses it to thin
        # streaks -- the owner's exact reported symptom. A bake means the lerp
        # reached a palette, a fallback means it did not. Nobody has ever read
        # these live, so the pillar has never been measured, only reasoned about.
        'printf "DIAG_PARTICLE_ENV=%u,%u,%u\n", gNdsParticleEnvVariantBakeCount, gNdsParticleEnvVariantHitCount, gNdsParticleEnvVariantFallbackCount',
        'printf "DIAG_YOSTER_CLOUD_MASKED=%u\n", gNdsRendererPrimEnvMaskedBakeCount',
        'printf "DIAG_HEAL_SPARKLE_SUBMIT=%u\n", gNdsRendererHealSparkleCoverageSubmitCount',
        # GROUND Thunder Jolt reconnaissance: the OR of every material effects
        # word seen on its six roots, a bitmask of which of the six were walked
        # at all, and failed snapshots. The mask matters because the tree skips
        # DOBJ_FLAG_HIDDEN children silently, so it is the only way to learn how
        # many segments ever become visible.
        'printf "DIAG_THUNDERGROUND=%#x,%#x,%u\n", gNdsThunderGroundEffectsSeen, gNdsThunderGroundRootMask, gNdsThunderGroundSnapshotFailCount',
        'printf "DIAG_STAGE_TEX=%#x,%#x,%#x,%u,%u,%u,%u,%u\n", gNdsNativeStagePrepareRunTexture[0], gNdsNativeStagePrepareRunTexture[1], gNdsNativeStagePrepareRunTexture[2], gNdsRendererBattleStaticTexturePreparedCount, gNdsRendererBattleStaticTexturePrepareFailCount, gNdsRendererBattleStaticTextureViolationCount, gNdsRendererBattleStaticTexturePinnedHitCount, gNdsRendererBattleStaticTextureFailStep',
        'printf "DIAG_SHIELD_PREP=%u\n", gNdsEntryShieldTexturePrepareDeclineCount',
        'printf "DIAG_ENTRY_EFFECT_NOZ=%u\n", gNdsEntryEffectNativeNoZGroupDraws',
        'printf "DIAG_SHIELD_POLYFMT=%#x\n", gNdsEntryShieldWitnessPolyFmt',
        'printf "DIAG_ZEBES_ACID_TEXEL1=%u,%u,%u\n", gNdsRendererZebesAcidBindCount, gNdsRendererZebesAcidWantsTexel1TrueCount, gNdsRendererZebesAcidWantsTexel1FalseCount',
        'printf "DIAG_TARUCANN=%u,%u,%u,%u,%d\n", gNdsTaruCannCaptureCount, gNdsTaruCannFireInputCount, gNdsTaruCannFireAutoCount, gNdsTaruCannLaunchCount, gNdsTaruCannLaunchAngle',
        'echo DIAG_TARUCANN_ROTATE=', 'output gNdsTaruCannLaunchRotate', 'echo \n',
        'echo DIAG_TARUCANN_KNOCKBACK=', 'output gNdsTaruCannLaunchKnockback', 'echo \n',
        'printf "DIAG_STAGE_RUN_SERIAL=%u\n", gNdsNativeStageRoofSnapSerial',
        'echo DIAG_STAGE_RUN_VALID=', 'output gNdsNativeStageRoofSnapValid', 'echo \n',
        'echo DIAG_STAGE_RUN_GIVEN=', 'output gNdsNativeStageRoofSnapGiven', 'echo \n',
        'echo DIAG_STAGE_RUN_EMITTED=', 'output gNdsNativeStageRoofSnapEmitted', 'echo \n',
        'printf "DIAG_CASTLE_ROOF_CLIP_META=%u,%u,%u,%u,%u\n", gNdsNativeStageRoofSnapSerial, gNdsNativeStageCastleRoofClipValid, gNdsNativeStageCastleRoofClipRun, gNdsNativeStageCastleRoofClipCornerCount, gNdsNativeStageCastleRoofClipArm',
        'echo DIAG_CASTLE_ROOF_CLIP_PROJECTED_Z=', 'output/x gNdsNativeStageCastleRoofClipProjectedZ', 'echo \n',
        'echo DIAG_CASTLE_ROOF_CLIP_SHIFT=', 'output/x gNdsNativeStageCastleRoofClipShift', 'echo \n',
        'echo DIAG_CASTLE_ROOF_CLIP_DENSE=', 'output/x gNdsNativeStageCastleRoofClipDense', 'echo \n',
        'echo DIAG_CASTLE_ROOF_CLIP_SUBMIT_V16=', 'output/x gNdsNativeStageCastleRoofClipSubmitV16', 'echo \n',
        'echo DIAG_CASTLE_ROOF_CLIP_RESULT=', 'output/x gNdsNativeStageCastleRoofClipResult', 'echo \n',
        'echo DIAG_CASTLE_ROOF_CLIP_FLAGS=', 'output/x gNdsNativeStageCastleRoofClipFlags', 'echo \n',
        'printf "DIAG_STAGE_SHORTFALL=%u,%u,%u,%u\n", gNdsNativeStageNoZForeignBindingCount, gNdsNativeStageNoZEnsureWorldCount, gNdsNativeStageEmitShortfallCount, gNdsNativeStageEmitShortfallResidue',
        # The Castle roof and the Yoster floor are both emitted losslessly,
        # pass every static gate, and record zero native failures -- so
        # whatever loses them is a RUNTIME decline or a silent cull. These
        # name which one. NoZInsideCullCount is new: until it existed a run
        # culled with all three corners outside read as a success.
        'printf "DIAG_WITNESS=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsNativeStagePrepareRunFailStep, gNdsNativeStagePrepareRunFailRun, gNdsNativeStageValidateFullFailStep, gNdsNativeStagePacketUnresolvedCount, gNdsNativeStagePacketUnresolvedKind, gNdsNativeStageBlobReadFailCount, gNdsNativeStageBlobHashMismatchCount, gNdsNativeStageWarmUploads, gNdsNativeStageWarmUploadCount, gNdsNativeStageNoZInsideCullCount, gNdsNativeStageNearFanCount, gNdsNativeStageNearFanZeroWCount',
        'printf "DIAG_TEXPHASE=%u,%u,%#x,%#x\n", gNdsNativeStageFilterPhase8RunCount, gNdsNativeStageFilterPhase16RunCount, gNdsNativeStageFilterPhaseActivePacket, gNdsNativeStageFilterPhaseBlobPacket',
        'echo DIAG_WITNESSPOLICY=', 'output gNdsNativeStagePrepareRunPolicy', 'echo \n',
        'echo DIAG_WITNESSTEX=', 'output gNdsNativeStagePrepareRunTexture', 'echo \n',
        'echo DIAG_WITNESSCENSUS=', 'output gNdsNativeStageValidateFullCensus', 'echo \n')
    if (-not $NoCapture) {
        $capture = Join-Path $root "artifacts/visibility/$Name.png"
        $helper = Join-Path $root 'scripts/capture-running-melonds-window.ps1'
        $commands += ('shell pwsh -NoProfile -File "' + $helper + '" -EmulatorProcessId ' + $emulator.Id + ' -Output "' + $capture + '"')
        $result.capture = $capture
    }
    $commands += @('detach','quit')
    Invoke-GdbMarkerScript -Gdb $gdb -Elf $Elf -Root $root -Commands $commands `
        -ScriptName $scriptName -TimeoutSeconds $TimeoutSeconds | Out-Null
    $transcript = Join-Path $env:SMASH64DS_VERIFY_TEMP_DIR "$scriptName.out"
    # The shared helper writes .out/.err beside the generated GDB command file.
    if (-not (Test-Path -LiteralPath $transcript)) { $transcript += '.txt' }
    $text = Get-Content -LiteralPath $transcript -Raw
    Copy-Item -LiteralPath $transcript -Destination (Join-Path $output "$Name.gdb.txt")
    $walkTrace = @([regex]::Matches($text,'(?m)^DIAG_WALK_[A-Z_]+=.*\r?$') |
        ForEach-Object { $_.Value.Trim() })
    if ($walkTrace.Count -gt 0) { $result.walk_trace = $walkTrace }
    $state = [regex]::Match($text,'(?m)^DIAG_STATE=([0-9,]+)\r?$')
    $native = [regex]::Match($text,'(?m)^DIAG_NATIVE=([0-9,]+)\r?$')
    if (-not $state.Success -or -not $native.Success) {
        if ($Condition -ne '') { throw 'Missing diagnostic result markers; the action condition never fired within the capture window.' }
        throw 'Missing diagnostic result markers.'
    }
    $result.state = @($state.Groups[1].Value.Split(',') | ForEach-Object { [uint32]$_ })
    $result.native_failure = @($native.Groups[1].Value.Split(',') | ForEach-Object { [uint32]$_ })
    $draw = [regex]::Match($text, '(?m)^DIAG_DRAW=([0-9,]+)\r?$')
    if (-not $draw.Success) { throw 'Missing per-fighter output witness.' }
    $result.fighter_draw = @($draw.Groups[1].Value.Split(',') | ForEach-Object { [uint32]$_ })
    if ($StartupFailureDiagnostics) {
        foreach ($marker in @('pack', 'heap', 'stage', 'resource', 'go')) {
            $hit = [regex]::Match($text, '(?m)^DIAG_STARTUP_' + $marker.ToUpperInvariant() + '=(.*)\r?$')
            if (-not $hit.Success) { throw "Missing startup $marker witness." }
            $result['startup_' + $marker] = @($hit.Groups[1].Value.Trim().Split(',') |
                ForEach-Object { if ($_ -match '^0x') { [Convert]::ToUInt32($_.Substring(2),16) } else { [uint32]$_ } })
        }
        if ($result.startup_pack[0] -ne 0 -or $result.startup_pack[2] -ne 0 -or
            $result.startup_pack[5] -ne 0 -or $result.startup_heap[0] -ne 0 -or
            @($result.startup_stage[0..6] | Where-Object { $_ -ne 0 }).Count -ne 0 -or
            @($result.startup_resource[2..7] | Where-Object { $_ -ne 0 }).Count -ne 0) {
            throw 'Startup resource or native failure counter is nonzero.'
        }
    }
    $impactWave = [regex]::Match($text,'(?m)^DIAG_IMPACTWAVE=([0-9]+)\r?$')
    if (-not $impactWave.Success) { throw 'Missing native impact-wave witness marker.' }
    $result.impact_wave_hits = [uint32]$impactWave.Groups[1].Value
    if ($RequireImpactWave -and $result.impact_wave_hits -eq 0) {
        throw 'Requested impact wave never reached ndsRendererSubmitNativeImpactWave.'
    }
    foreach ($marker in @('DIAG_FTDECLINE','DIAG_FTREJECT','DIAG_FTREJECT_SLOT','DIAG_FTROOTS','DIAG_ACTION')) {
        $hit = [regex]::Match($text,'(?m)^' + $marker + '=(.*)\r?$')
        if ($hit.Success) {
            $result[$marker.ToLowerInvariant().Replace('diag_','')] = $hit.Groups[1].Value
        }
    }
    $result.cond_met = [regex]::IsMatch($text,'(?m)^DIAG_COND_MET\r?$')
    $result.cond2_met = [regex]::IsMatch($text,'(?m)^DIAG_COND2_MET\r?$')
    $actionPhases = @()
    foreach ($phaseHit in [regex]::Matches($text,'(?m)^DIAG_ACTION_PHASE=([0-9,]+)\r?$')) {
        $v = @($phaseHit.Groups[1].Value.Split(',') | ForEach-Object { [uint32]$_ })
        if ($v.Count -ne 23) { throw 'Malformed action-phase witness.' }
        $vanished = if ($v[3] -ne 0 -and $v[7] -ne 0) { 'both' }
            elseif ($v[3] -ne 0) { 'attacker' }
            elseif ($v[7] -ne 0) { 'victim' }
            else { 'neither' }
        $outcome = if ($v[12] -ne 0) { 'declined_recorded' }
            elseif ($v[3] -ne 0 -and $v[10] -eq 0) { 'silently_skipped_attacker' }
            elseif ($v[10] -ne 0) { 'submitted_drawn' }
            else { 'no_attacker_submit' }
        $actionPhases += [PSCustomObject][ordered]@{
            phase=$v[0]; tick=$v[1]; attacker_status=$v[2]; attacker_invisible=$v[3]
            attacker_shield=$v[4]; attacker_jostle_ignore=$v[5]; victim_status=$v[6]
            victim_invisible=$v[7]; attacker_catch_matches_victim=$v[8]
            victim_capture_matches_attacker=$v[9]; attacker_triangles_this_frame=$v[10]
            victim_triangles_this_frame=$v[11]; attacker_rejects_this_frame=$v[12]
            victim_rejects_this_frame=$v[13]; native_failures_this_frame=$v[14]
            decline_stage=$v[15]; decline_owner=$v[16]; reject_status=$v[17]
            reject_reason=$v[18]; validate_code=$v[19]; validate_root=$v[20]
            validate_observed=$v[21]; validate_expected=$v[22]; vanished=$vanished
            outcome=$outcome
        }
    }
    if ($Condition -ne '' -and $actionPhases.Count -eq 0) { throw 'Missing action-phase witness.' }
    $result.action_phases = @($actionPhases)
    if ($result.state[0] -ne 22 -or $result.state[1] -ne $StageKind) { throw 'Probe reached the wrong scene/stage.' }
    if ($Fighter1Kind -ne 255) {
        $fighter = [regex]::Match($text,'(?m)^DIAG_FIGHTER=(-?[0-9,\-]+)\r?$')
        $ownertri = [regex]::Match($text,'(?m)^DIAG_OWNERTRI=\{([0-9, ]+)\}\r?$')
        if (-not $fighter.Success -or -not $ownertri.Success) { throw 'Missing fighter diagnostic markers.' }
        $result.fighter_state = @($fighter.Groups[1].Value.Split(',') | ForEach-Object { [int]$_ })
        $result.owner_triangles = @($ownertri.Groups[1].Value.Split(',') | ForEach-Object { [uint32]$_.Trim() })
        # A case that does not commit its own fighter has measured Mario and
        # would report his verdict under another fighter's name.
        if ($result.fighter_state[0] -ne $Fighter1Kind) { throw 'Probe committed the wrong player-1 fighter.' }
        if ($Fighter2Kind -ne 255 -and $result.fighter_state[1] -ne $Fighter2Kind) { throw 'Probe committed the wrong player-2 fighter.' }
        # Zero recorded failures AND zero drawn triangles is a successful empty
        # draw, which is exactly what the native-only contract forbids.
        # Entry animations may hide model parts independently of is_invisible;
        # their effect witness is authoritative before the source GO clock runs.
        if ($result.state[2] -gt 0 -and
            (($result.fighter_draw[0] -eq 0 -and $result.fighter_draw[2] -eq 0) -or
             ($result.fighter_draw[1] -eq 0 -and $result.fighter_draw[3] -eq 0))) {
            throw 'A source-visible fighter emitted no native triangles on the sampled frame.'
        }
    }
    if (-not $NoCapture -and -not (Test-Path -LiteralPath $result.capture)) { throw 'Screenshot was not produced.' }
    if ((Get-FileHash -LiteralPath $Rom).Hash -ne $result.rom_sha256 -or
        (Get-FileHash -LiteralPath $Elf).Hash -ne $result.elf_sha256) { throw 'ROM or ELF changed during diagnosis.' }
    $result.transport = 'ok'
    $actionFailure = @($actionPhases | Where-Object outcome -ne 'submitted_drawn').Count -ne 0
    if ($RequireScoreAlert) {
        $scores = [regex]::Match($text, '(?m)^DIAG_SCORE_ALERT=([0-9,]+)\r?$')
        if (-not $scores.Success) { throw 'Missing bottom-screen score witness.' }
        $result.score_alert = @($scores.Groups[1].Value.Split(',') | ForEach-Object { [uint32]$_ })
        if ($result.score_alert[1] -eq 0 -or $result.score_alert[2] -ne 3 -or $result.score_alert[3] -eq 0) {
            throw 'Both source score images must actually draw, with a live alert at capture.'
        }
        # A KO legitimately hides its fighter. Only rejection/native faults
        # fail the body-phase record when the requested output is the score.
        $actionFailure = @($actionPhases | Where-Object {
            $_.attacker_rejects_this_frame -ne 0 -or $_.victim_rejects_this_frame -ne 0 -or
            $_.native_failures_this_frame -ne 0
        }).Count -ne 0
    }
    if ($YoshiEffectDiagnostics) {
        $effects = [regex]::Match($text, '(?m)^DIAG_YOSHI_EFFECTS=([0-9,]+)\r?$')
        if (-not $effects.Success) { throw 'Missing Yoshi effect output witness.' }
        $result.yoshi_effect_draws = @($effects.Groups[1].Value.Split(',') | ForEach-Object { [uint32]$_ })
        if ($result.state[2] -eq 0 -and $result.yoshi_effect_draws[0] -gt 0) {
            # The source entry egg replaces the body before GO. Require its
            # actual native output, retaining every per-frame rejection check.
            $actionFailure = @($actionPhases | Where-Object {
                $_.attacker_rejects_this_frame -ne 0 -or $_.victim_rejects_this_frame -ne 0 -or
                $_.native_failures_this_frame -ne 0
            }).Count -ne 0
        }
    }
    $result.native = if ($result.native_failure[0] -eq 0 -and -not $actionFailure) { 'pass' } else { 'fail' }
    $exitCode = if ($result.native -eq 'pass') { 0 } else { 2 }
} catch {
    $result.error = $_.Exception.Message
    # Invoke-GdbMarkerScript kills GDB on timeout, so a second attach cannot
    # recover the guest's last state. Preserve the first session's transcript
    # here as part of the failed case and lift its compact walk markers into the
    # JSON summary. This turns "marker timed out" into a named shell milestone.
    if (($null -eq $transcript) -and $env:SMASH64DS_VERIFY_TEMP_DIR) {
        $transcript = Join-Path $env:SMASH64DS_VERIFY_TEMP_DIR "$scriptName.out"
        if (-not (Test-Path -LiteralPath $transcript)) { $transcript += '.txt' }
    }
    if (($null -ne $transcript) -and (Test-Path -LiteralPath $transcript)) {
        Copy-Item -LiteralPath $transcript -Destination (Join-Path $output "$Name.gdb.txt") -Force
        $failureText = Get-Content -LiteralPath $transcript -Raw
        $walkTrace = @([regex]::Matches($failureText,'(?m)^DIAG_WALK_[A-Z_]+=.*\r?$') |
            ForEach-Object { $_.Value.Trim() })
        if ($walkTrace.Count -gt 0) { $result.walk_trace = $walkTrace }
    }
} finally {
    if ($emulator) {
        try {
            $emulator.Refresh()
            $result.peak_working_set_bytes = $emulator.PeakWorkingSet64
            $result.cpu_seconds = $emulator.TotalProcessorTime.TotalSeconds
        } catch {
            $result.metrics_error = $_.Exception.Message
        } finally {
            Stop-Process -Id $emulator.Id -Force -ErrorAction SilentlyContinue
            try {
                if (-not $emulator.WaitForExit(10000)) { throw 'The owned emulator did not stop.' }
            } catch {
                $result.cleanup_error = $_.Exception.Message
                $result.transport = 'failed'
                $canRestoreConfig = $false
                $exitCode = 1
            }
        }
    }
    if ($canRestoreConfig -and $null -ne $originalConfig) { [IO.File]::WriteAllBytes($config,$originalConfig) }
    $env:SMASH64DS_VERIFY_STORAGE_DIR = $previousStorage
    $result.elapsed_seconds = [Math]::Round($watch.Elapsed.TotalSeconds,3)
    $result | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $output "$Name.json") -Encoding utf8
    if ($locked) { $lease.ReleaseMutex() }
    $lease.Dispose()
}
exit $exitCode
