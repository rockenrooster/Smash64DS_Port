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
    [ValidateSet('none','grab','special','linkboomerang','shieldflick','shieldroll')][string]$Pump = 'none',
    [ValidateRange(-80,80)][int]$StickX = 0,
    [ValidateRange(-80,80)][int]$StickY = 0,
    [ValidateRange(-500,500)][int]$Teleport = 0,
    [string]$Condition = '',
    [string]$Condition2 = ''
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
        'gNdsRendererZebesAcidBindCount',
        'gNdsRendererZebesAcidWantsTexel1TrueCount',
        'gNdsRendererZebesAcidWantsTexel1FalseCount',
        'gNdsTaruCannCaptureCount','gNdsTaruCannFireInputCount',
        'gNdsTaruCannFireAutoCount','gNdsTaruCannLaunchCount',
        'gNdsTaruCannLaunchAngle','gNdsTaruCannLaunchRotate',
        'gNdsTaruCannLaunchKnockback',
        'gNdsRendererStageOwnerFirstRejectReason','gNdsRendererStageOwnerRejectCount',
        'gNdsRendererAdapterSectorArwingMtxCount',
        'gNdsNativeStageFilterPhase8RunCount','gNdsNativeStageFilterPhase16RunCount',
        'gNdsNativeStageFilterPhaseActivePacket','gNdsNativeStageFilterPhaseBlobPacket',
        'gNdsNativeStageRoofSnapSerial',
        'gNdsNativeStageRoofSnapValid',
        'gNdsNativeStageRoofSnapGiven',
        'gNdsNativeStageRoofSnapEmitted',
        'sNdsRendererAdapterNativeStageWorkspace',
        'gNdsInishiePakkunCandidateStep',
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
        'gNdsPlayersVSPreviewAcquireRetryCount','gNdsPlayersVSPreviewDwellCommitCount',
        'gNdsRendererFastOwnerTriangleCount',
        'gNdsFighterDLAllDrawP0HardwareTriangleCount',
        'gNdsFighterDLAllDrawP1HardwareTriangleCount',
        'gNdsFtrRejectCountBySlot','gNdsFtrRejectStatusBySlot',
        'gNdsFtrRejectReasonBySlot','ndsControllerPlaybackSetEnabled',
        'ndsControllerPlaybackSetConnectedMask','ndsControllerPlaybackSetPad')) {
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
    $commands = @('set pagination off','set confirm off','set remotetimeout 30',
        ("target remote 127.0.0.1:{0}" -f $context.GdbPort),
        'break ndsSceneManagerEnter','commands','silent',
        'printf "DIAG_WALK_SCENE=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gSCManagerSceneData.scene_curr, gSCManagerSceneData.scene_prev, gNdsMenuShellWalkSteps, gNdsMenuShellInputCount, gNdsMenuShellTransitionCount, gNdsMenuShellCssStartCount, gNdsMenuShellCssStartDeniedCount, gNdsPlayersVSPreviewAcquireLoadCount, gNdsPlayersVSPreviewAcquireLoadFinishCount, gNdsPlayersVSPreviewAcquireRetryCount, gNdsPlayersVSPreviewDwellCommitCount',
        'set variable gNdsMenuShellWalkBudget = 1',
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
        'continue','end',
        'tbreak ndsMenuShellSssCommit','commands','silent',
        'printf "DIAG_WALK_SSS_COMMIT=%u,%u,%u,%u\n", sMenuTics, gNdsMenuShellWalkSteps, gNdsMenuShellWalkLoops, gNdsMenuShellSssWalkTargetGkind',
        'continue','end')
    if ($Condition -eq '') {
        # Legacy path: fixed present count, no forced input. Unchanged.
        $commands += @('tbreak scVSBattleStartBattle','continue','delete',
            'tbreak ndsBattlePlayableFrameCompleteMarker',
            ('ignore $bpnum ' + ($Presents - 1)), 'continue')
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
            'tbreak scVSBattleStartBattle','commands','silent',
            'call ndsControllerPlaybackSetEnabled(1)',
            'call ndsControllerPlaybackSetConnectedMask(1)',
            'call ndsControllerPlaybackSetPad(0, 0, 0, 0)',
            'call ndsControllerPlaybackSetPad(1, 0, 0, 0)',
            'set $p0tri_prev = gNdsFighterDLAllDrawP0HardwareTriangleCount',
            'set $p1tri_prev = gNdsFighterDLAllDrawP1HardwareTriangleCount',
            'set $p0rej_prev = gNdsFtrRejectCountBySlot[0]',
            'set $p1rej_prev = gNdsFtrRejectCountBySlot[1]',
            'set $native_prev = gNdsRendererNativeFailure.count',
            'end','continue','delete',
            'break ndsBattlePlayableFrameCompleteMarker',
            'commands','silent',
            'set $fttick = $fttick + 1')
        switch ($Pump) {
            'grab' {
                # Z held always (shield); A tapped 2 of every 30 frames. Grab
                # from common ground needs Z hold + A tap
                # (ftcommoncatch1.c:134); a static pad taps once and Entry
                # eats it, so the tap must repeat. Victim stays pinned beside
                # the attacker until acquisition: p_translate is the fighter
                # world position (map.h:34) of Vec3f x,y,z (ssb_types.h:10).
                $commands += @('if (($fttick % 30) < 2)',
                    'call ndsControllerPlaybackSetPad(0, 0xA000, 0, 0)',
                    'call ndsControllerPlaybackSetPad(1, 0, 0, 0)')
                $commands += @('else',
                    'call ndsControllerPlaybackSetPad(0, 0x2000, 0, 0)',
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
                    ('call ndsControllerPlaybackSetPad(0, 0x4000, ' + $StickX + ', ' + $StickY + ')'),
                    'call ndsControllerPlaybackSetPad(1, 0, 0, 0)')
                $commands += @('else',
                    'call ndsControllerPlaybackSetPad(0, 0, 0, 0)',
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
                    'call ndsControllerPlaybackSetPad(0, 0x4000, 0, 0)',
                    'else','call ndsControllerPlaybackSetPad(0, 0, 0, 0)',
                    'end','else','call ndsControllerPlaybackSetPad(0, 0, 0, 0)',
                    'end')
            }
            'shieldflick' {
                # Z held always; stick flicked 10 of every 20 frames. Escape
                # needs a stick edge during guard, which a static full-deflect
                # never provides.
                $commands += @('if (($fttick % 20) < 10)',
                    ('call ndsControllerPlaybackSetPad(0, 0x2000, ' + $StickX + ', 0)'),
                    'else',
                    'call ndsControllerPlaybackSetPad(0, 0x2000, 0, 0)',
                    'end')
            }
            'shieldroll' {
                # Establish Guard with neutral Z before introducing a stick
                # edge. This proves the escape came from shield rather than a
                # coincident dash/catch path.
                $commands += @('if $ftphase == 0',
                    'call ndsControllerPlaybackSetPad(0, 0x2000, 0, 0)',
                    'else','if (($fttick % 20) < 2)',
                    ('call ndsControllerPlaybackSetPad(0, 0x2000, ' + $StickX + ', 0)'),
                    'else','call ndsControllerPlaybackSetPad(0, 0x2000, 0, 0)',
                    'end','end')
            }
            default {
                $commands += @('call ndsControllerPlaybackSetPad(0, 0, 0, 0)')
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
    $commands += @(
        'printf "DIAG_STATE=%u,%u,%u,%u,%u\n", gSCManagerSceneData.scene_curr, gSCManagerBattleState->gkind, gSCManagerBattleState->time_passed, gSCManagerBattleState->pl_count, gSCManagerBattleState->cp_count',
        'printf "DIAG_NATIVE=%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsRendererNativeFailure.count, gNdsRendererNativeFailure.domain, gNdsRendererNativeFailure.scene, gNdsRendererNativeFailure.identity, gNdsRendererNativeFailure.status, gNdsRendererNativeFailure.root, gNdsRendererNativeFailure.material, gNdsRendererNativeFailure.reason',
        'printf "DIAG_STAGE_OWNER=%u,%u,%u\n", gNdsRendererStageOwnerFirstRejectReason, gNdsRendererStageOwnerRejectCount, sNdsRendererAdapterNativeStageWorkspace.dobj_count',
        'printf "DIAG_SECTOR_ARWING_MTX=%u\n", gNdsRendererAdapterSectorArwingMtxCount',
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
        # The particle env-colour variant cache. The KO blast pillar's pixel is
        # (PRIM - ENV) * TEXEL + ENV, so dropping ENV collapses it to thin
        # streaks -- the owner's exact reported symptom. A bake means the lerp
        # reached a palette, a fallback means it did not. Nobody has ever read
        # these live, so the pillar has never been measured, only reasoned about.
        'printf "DIAG_PARTICLE_ENV=%u,%u,%u\n", gNdsParticleEnvVariantBakeCount, gNdsParticleEnvVariantHitCount, gNdsParticleEnvVariantFallbackCount',
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
        if (($result.owner_triangles | Measure-Object -Sum).Sum -eq 0) { throw 'No native owner emitted triangles on the sampled frame.' }
    }
    if (-not $NoCapture -and -not (Test-Path -LiteralPath $result.capture)) { throw 'Screenshot was not produced.' }
    if ((Get-FileHash -LiteralPath $Rom).Hash -ne $result.rom_sha256 -or
        (Get-FileHash -LiteralPath $Elf).Hash -ne $result.elf_sha256) { throw 'ROM or ELF changed during diagnosis.' }
    $result.transport = 'ok'
    $actionFailure = @($actionPhases | Where-Object outcome -ne 'submitted_drawn').Count -ne 0
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
