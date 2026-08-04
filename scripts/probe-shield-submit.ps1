[CmdletBinding()]
param(
    [string]$Build = 'build-c54-flag1',
    [string]$Target = 'smash64ds-battle-playable-proof-hwtri',
    [ValidateRange(1, 8)][int]$RunnerSlot = 7,
    [ValidateRange(30, 1800)][int]$TimeoutSeconds = 900,
    [string]$Artifact = ''
)

# DOES THE SHIELD TREE REACH THE MATRIX BUILDER AND THE DL SUBMITTER AT ALL?
#
# The 0x4F fix changed the shield root's local matrix from identity to the
# fighter joint's world matrix (measured: translate 109.7/1842/-36, scale 9.33)
# and the rendered top screen did not move by one pixel. Either the shield tree
# never reaches ndsRendererAdapterBuildDObjLocalMatrix, or it never reaches
# ndsRendererAdapterSubmitStageDL, or what is on screen is not this effect.
#
# Both are static functions inlined away as callees, so the probe conditions on
# the DObj pointer instead: the shield's display proc hands over the root, and
# the two breakpoints below arm only after that.

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
. (Join-Path $PSScriptRoot 'lib\gdb-markers.ps1')
. (Join-Path $PSScriptRoot 'lib\build-output.ps1')

$gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe'
$nm = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-nm.exe'
$rom = Resolve-Smash64DSBuildOutput -Root $root -Target $Target -Build $Build -Extension '.nds'
$elf = Resolve-Smash64DSBuildOutput -Root $root -Target $Target -Build $Build -Extension '.elf'
if ([string]::IsNullOrWhiteSpace($Artifact)) {
    $Artifact = Join-Path $root ('artifacts\verification\' +
        (Get-Date -Format 'yyyy-MM-dd') + '_shield-submit-probe.txt')
}
$context = Initialize-MelonDSVerifierContext `
    -Root $root -MelonDS '' -RunnerSlot $RunnerSlot -NoBuild
$melon_dir = Split-Path -Parent $context.MelonDSPath
$log_dir = Get-MelonDSVerifierLogDir -Root $root -RunnerSlot $RunnerSlot
$stdout = Join-Path $log_dir 'melonds.shield-submit-probe.stdout.log'
$stderr = Join-Path $log_dir 'melonds.shield-submit-probe.stderr.log'
$config_state = $null
$emulator = $null

$required = @(
    'efManagerShieldProcDisplay',
    'ndsRendererAdapterBuildDObjLocalMatrix',
    'ndsRendererAdapterSubmitStageDL',
    'gSCManagerBattleState',
    'gNdsEffectRendererTriangleCount',
    # A gdb batch aborts the REST of its commands on the first absent symbol,
    # so every name the command list reads is checked here, not just the
    # breakpoint targets.
    'gNdsRendererAdapterEffectPrepMask',
    'sNdsRendererAdapterStageWorldCache',
    'sNdsRendererAdapterDObjWorldCache',
    'gNdsEffectDLCfgMask',
    'gNdsEffectDLCfgMvT',
    'gNdsEffectDLMatrixSeed',
    'gNdsEffectDLVtx0',
    'gNdsRendererAdapterMvpRecalcPerspScaCount'
)
$symbols = & $nm $elf | ForEach-Object { ($_ -split '\s+')[-1] }
$missing = $required | Where-Object { $symbols -notcontains $_ }
if ($missing.Count -gt 0) {
    throw ("Shield submit probe symbols absent from {0}: {1}" -f $elf, ($missing -join ', '))
}

try {
    $config_state = Enable-MelonDSGdbConfig `
        -MelonDSPath $context.MelonDSPath `
        -GdbPort $context.GdbPort -Persistent -MuteAudio
    Remove-Item -LiteralPath $stdout, $stderr -Force -ErrorAction SilentlyContinue
    $emulator = Start-Process `
        -FilePath $context.MelonDSPath `
        -ArgumentList $rom `
        -WorkingDirectory $melon_dir `
        -RedirectStandardOutput $stdout `
        -RedirectStandardError $stderr `
        -WindowStyle Hidden `
        -PassThru
    Wait-MelonDSGdbListener -Process $emulator -Port $context.GdbPort | Out-Null

    $commands = @(
        'set pagination off',
        'set confirm off',
        'set remotetimeout 20',
        ("target remote 127.0.0.1:{0}" -f $context.GdbPort),

        'set $sroot = 0',
        'set $schild = 0',
        'set $locals = 0',
        'set $dls = 0',
        'set $execs = 0',
        'set $sdl = 0',

        'break *efManagerShieldProcDisplay',
        'break *ndsRendererAdapterBuildDObjLocalMatrix',
        'break *ndsRendererAdapterSubmitStageDL',
        'break *ndsRendererExecuteDisplayListWithVertexCache',
        'break *ndsRendererAdapterPrepareInitialMatrices',
        'condition 5 ($schild != 0) && ($r0 == $schild)',
        'disable 5',
        'condition 2 ($sroot != 0) && (($r0 == $sroot) || ($r0 == $schild))',
        'condition 3 ($sroot != 0) && (($r0 == $sroot) || ($r0 == $schild))',
        'condition 4 $sroot != 0',
        'disable 2',
        'disable 3',
        'disable 4',

        'commands 1',
        'silent',
        'set $sroot = (DObj *)((GObj *)$r0)->obj',
        'set $schild = (DObj *)$sroot->child',
        'printf "ARMED tr=%d root=%p child=%p\n", gSCManagerBattleState->time_remain, $sroot, $schild',
        'enable 2',
        'enable 3',
        'enable 5',
        'disable 1',
        'continue',
        'end',

        'commands 2',
        'silent',
        'set $locals = $locals + 1',
        'printf "LOCAL n=%d dobj=%p tr=%d\n", $locals, $r0, gSCManagerBattleState->time_remain',
        # The 0x4F helper''s own guards, read AT BUILD TIME rather than at the
        # display proc: attach, its parent_gobj, its FTParts, and the world
        # matrix row the helper copies. Whichever of these is 0 is the reason
        # the helper returned FALSE and the root fell back to identity.
        'if $r0 == $sroot',
        'set $a = (DObj *)((DObj *)$r0)->user_data.p',
        'printf "GUARD att=%p pgobj=%p parts=%p\n", $a, $a->parent_gobj, (FTParts *)$a->user_data.p',
        'printf "GUARDW k0=0x%x w=%f,%f,%f r0x=%f\n", ((DObj *)$r0)->xobjs[0]->kind, ((FTParts *)$a->user_data.p)->mtx_translate[3][0], ((FTParts *)$a->user_data.p)->mtx_translate[3][1], ((FTParts *)$a->user_data.p)->mtx_translate[3][2], ((FTParts *)$a->user_data.p)->mtx_translate[0][0]',
        'end',
        'if ($locals + $dls) < 12',
        'continue',
        'end',
        'end',

        'commands 3',
        'silent',
        'set $dls = $dls + 1',
        'set $sdl = (Gfx *)$r1',
        'printf "SUBMIT n=%d dobj=%p dl=%p tris=%u\n", $dls, $r0, $r1, gNdsEffectRendererTriangleCount',
        # The effect-submit publish block writes these AFTER the executor
        # returns, so at this entry they describe the PREVIOUS effect submit --
        # one frame of lag, and that is the whole cost of reading a global
        # instead of a stack argument. cfg bit 0 projection, bit 1 modelview;
        # seed is the hardware_matrix_seed_count delta, which is 1 only if the
        # config's matrices composed into a valid traversal matrix.
        'printf "DLCFG n=%u cfg=%u mvt=%d,%d,%d seed=%u mcmd=%u xf=%u hwv=%u hwt=%u\n", gNdsEffectDLPublishCount, gNdsEffectDLCfgMask, gNdsEffectDLCfgMvT[0], gNdsEffectDLCfgMvT[1], gNdsEffectDLCfgMvT[2], gNdsEffectDLMatrixSeed, gNdsEffectDLMatrixCmd, gNdsEffectDLXformVertexCount, gNdsEffectDLHwVertexCount, gNdsEffectDLHwTriangleCount',
        'printf "DLVTX0 x=%d y=%d z=%d w=%d blocker=%u cmds=%u op=%u\n", gNdsEffectDLVtx0[0], gNdsEffectDLVtx0[1], gNdsEffectDLVtx0[2], gNdsEffectDLVtx0[3], gNdsEffectDLBlocker, gNdsEffectDLCommandCount, gNdsEffectDLFirstOpcode',
        # The prim/env this effect proc emitted into its DL head span. mask 3
        # means both recovered; prim RGBA for a P1 human shield is
        # dEFManagerShieldColors[0] = white with alpha 0xC0, env = red 0xC0.
        'printf "DLCOL mask=%u prim=0x%08x env=0x%08x\n", gNdsEffectDLColorMask, gNdsEffectDLPrimColor, gNdsEffectDLEnvColor',
        'enable 4',
        'if ($locals + $dls) < 12',
        'continue',
        'end',
        'end',

        'commands 4',
        'silent',
        # THE CONFIG POINTER IS VALIDATED BY A REGISTER, NOT BY ITSELF. Cycle 53
        # read the config through $r1 and "proved" the pointer with
        # max_commands == 8192 -- a field read THROUGH the pointer under test,
        # which proves nothing; cycle 54 got all-zero words on the next ROM and
        # retracted the whole seam. Both runs lacked an INDEPENDENT check.
        # There is one: ndsRendererAdapterSubmitStageDL passes the SAME &state
        # as config.user and as callback_user, and callback_user is $r3 at this
        # entry (0x2024768 is the symbol address; its first instruction is the
        # `push`, so r0-r3 still hold the arguments). config->user == $r3 is a
        # field agreeing with a register -- sound where a self-read is not.
        'printf "EXEC dl=%p shielddl=%p persist=%d effect=%d\n", $r0, $sdl, sNdsRendererAdapterStagePersistentActive, sNdsRendererAdapterEffectSubmitActive',
        'printf "CFGID r1=%p r2=%p r3=%p user=%p match=%d\n", $r1, $r2, $r3, ((void **)$r1)[12], (int)(((void **)$r1)[12] == (void *)$r3)',
        'printf "CFGRAW depth=%u cmds=%u lcmds=%u proj=%p mv=%p\n", ((unsigned int *)$r1)[0], ((unsigned int *)$r1)[1], ((unsigned int *)$r1)[2], ((void **)$r1)[3], ((void **)$r1)[4]',
        'set $mv = ((int **)$r1)[4]',
        'if $mv != 0',
        'printf "CFGMV t=%d,%d,%d r0=%d,%d,%d\n", $mv[12], $mv[13], $mv[14], $mv[0], $mv[1], $mv[2]',
        'end',
        # The world matrix the prep actually built, read from the file-static
        # stage-world cache instead of from any argument register. With prep
        # mask bit 1 clear (the battle camera folds LookAt into the projection)
        # ndsRendererAdapterPrepareInitialMatrices copies dobj_world straight
        # into the modelview, so this IS what config.initial_modelview must
        # carry -- an independent second opinion on the line above.
        'set $wi = 0',
        'while $wi < sNdsRendererAdapterStageWorldCacheCount',
        'if (sNdsRendererAdapterStageWorldCache[$wi].dobj == $sroot) || (sNdsRendererAdapterStageWorldCache[$wi].dobj == $schild)',
        'set $ws = sNdsRendererAdapterStageWorldCache[$wi].world_slot',
        'printf "WORLD i=%d dobj=%p slot=%d vf=%u t=%d,%d,%d r0=%d,%d,%d\n", $wi, sNdsRendererAdapterStageWorldCache[$wi].dobj, $ws, sNdsRendererAdapterStageWorldCache[$wi].validated_frame, sNdsRendererAdapterDObjWorldCache[$ws].world.m[3][0], sNdsRendererAdapterDObjWorldCache[$ws].world.m[3][1], sNdsRendererAdapterDObjWorldCache[$ws].world.m[3][2], sNdsRendererAdapterDObjWorldCache[$ws].world.m[0][0], sNdsRendererAdapterDObjWorldCache[$ws].world.m[0][1], sNdsRendererAdapterDObjWorldCache[$ws].world.m[0][2]',
        'end',
        'set $wi = $wi + 1',
        'end',
        # File statics, so these are sound reads where the stack and the
        # argument registers are not.
        'printf "CAMCACHE count=%u frame=%u cobj0=%p pv0=%u mv0=%u\n", sNdsRendererAdapterCameraCacheCount, sNdsRendererAdapterCameraCacheFrame, sNdsRendererAdapterCameraCache[0].cobj, sNdsRendererAdapterCameraCache[0].projection_valid, sNdsRendererAdapterCameraCache[0].modelview_valid',
        'printf "SWCACHE ptr=%p count=%u attempted=%u\n", sNdsRendererAdapterStageWorldCache, sNdsRendererAdapterStageWorldCacheCount, sNdsRendererAdapterStageWorldCacheAllocationAttempted',
        # 1 cam-proj valid, 2 cam-mv valid, 4 world valid, 8/16 proj/mv pointer
        # non-NULL before the 0x47 rewrite, 32/64 after it. 0x7d is the healthy
        # battle value: the camera folds LookAt into the projection, so bit 1 is
        # legitimately clear and both pointers are handed over non-NULL.
        'printf "PREPMASK n=%u mask=0x%x\n", gNdsRendererAdapterEffectPrepCount, gNdsRendererAdapterEffectPrepMask',
        # The MVP-recalc rewrite. persp_sca counts the kind-44 branch, which is
        # the shield billboard; detected/applied/reject/mismatch are the shared
        # 0x47 ledger. persp_sca climbing with applied means the shield took the
        # rewrite; reject climbing instead names the camera guard.
        'printf "RECALC persp_sca=%u detected=%u applied=%u reject=%u mismatch=%u\n", gNdsRendererAdapterMvpRecalcPerspScaCount, gNdsRendererAdapterCustom47DetectedCount, gNdsRendererAdapterCustom47AppliedCount, gNdsRendererAdapterCustom47RejectCount, gNdsRendererAdapterCustom47TranslationMismatchCount',
        'set $execs = $execs + 1',
        'if $execs < 14',
        'continue',
        'end',
        'end',

        # r0-r2 only. The three stacked arguments (projection_ptr, modelview,
        # modelview_ptr) were read here from $sp/$sp+8 and came back 0x1 and a
        # rotating pair of addresses -- stack reads are not sound on this
        # remote, and the config read at breakpoint 4 answers the same question
        # from a pointer deref instead.
        'commands 5',
        'silent',
        'printf "PREP dobj=%p cobj=%p persist=%d\n", $r0, $r1, $r2',
        'continue',
        'end',

        'continue',
        'printf "SUBMITDONE locals=%d dls=%d execs=%d tris=%u dobjdraw=%u reject=%u\n", $locals, $dls, $execs, gNdsEffectRendererTriangleCount, gNdsEffectRendererDObjDrawCount, gNdsEffectRendererRejectedDrawCount',
        'printf "DLCFGDONE n=%u cfg=%u mvt=%d,%d,%d seed=%u mcmd=%u xf=%u hwv=%u hwt=%u\n", gNdsEffectDLPublishCount, gNdsEffectDLCfgMask, gNdsEffectDLCfgMvT[0], gNdsEffectDLCfgMvT[1], gNdsEffectDLCfgMvT[2], gNdsEffectDLMatrixSeed, gNdsEffectDLMatrixCmd, gNdsEffectDLXformVertexCount, gNdsEffectDLHwVertexCount, gNdsEffectDLHwTriangleCount',
        'detach',
        'quit'
    )

    $capture = Invoke-GdbMarkerScript `
        -Gdb $gdb -Elf $elf -Root $root -Commands $commands `
        -ScriptName 'shield_submit_probe.gdb' `
        -TimeoutSeconds $TimeoutSeconds
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Artifact) | Out-Null
    Set-Content -LiteralPath $Artifact -Value $capture.Stdout
    $capture.Stdout -split "`n" |
        Where-Object { $_ -match 'ARMED|LOCAL|SUBMIT' } |
        ForEach-Object { Write-Output $_ }
    Write-Output "probe capture: $Artifact"
}
finally {
    if ($null -ne $emulator) {
        Stop-Process -Id $emulator.Id -Force -ErrorAction SilentlyContinue
    }
    if ($null -ne $config_state) {
        Restore-MelonDSGdbConfig -State $config_state
    }
}
