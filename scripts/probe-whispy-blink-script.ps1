[CmdletBinding()]
param(
    [string]$Build = 'build-c128-foxgun',
    [string]$Target = 'smash64ds-battle-playable-tickhud-hwtri',
    [ValidateRange(1, 8)][int]$RunnerSlot = 7,
    [ValidateRange(30, 1800)][int]$TimeoutSeconds = 600,
    [ValidateRange(1, 32)][int]$Events = 6,
    [switch]$Cadence,
    [ValidateRange(4, 400)][int]$Frames = 60,
    [string]$Artifact = ''
)

# BUGS.md row 1, second question. The first probe established WHAT: the eyes
# GObj animation reaches anim_frame 1.0 for one sample and is back to 0 the
# next, so the blink is a one-frame animation. This one asks WHY, and it is a
# read of the script itself rather than another theory.
#
# grpupupu.c:565 hands gcAddAnimAll a joint table at
#   dGRPupupuWhispyEyesAnims[lr][status][0] + map_head
# and a material table at [1] + map_head, which for BLINK is 0 -> NULL. The
# port intercepts that call (battleship_sys_objanim.c:1567): it normalizes
# every joint script first, and if ANY of them is rejected it skips
# ndsBaseGcAddAnimAll ENTIRELY -- no gobj->anim_frame, no dobj->anim_joint.
# So a one-frame playout has two very different explanations:
#
#   (a) THE ADD NEVER HAPPENED. sNdsAObjEvent32NormalizedCount sits at 973 of
#       1,024 after a minute, and reason 12 is exactly "the table would
#       overflow". Then anim_frame never gets set and the 1.0 is somebody
#       else's leftover. Tell: gNdsAObjEvent32NormalizeFailCount moves across
#       the blink frame, and the script words are still in SOURCE layout.
#   (b) THE SCRIPT REALLY IS ONE FRAME. objdef.h:283 packs source words as
#       opcode[31:25] flags[24:15] payload[14:0]; the port repacks them to
#       opcode[6:0] flags[16:7] payload[31:17]. Frame length is the sum of the
#       payloads of the blocking commands, plus one
#       (reloc_backend_diagnostic_recorders.c:244). Wait is opcode 2, End is 0.
#       If every payload decodes to 0 the data says one frame and the fix is
#       upstream of the runtime.
#
# The MOUTH is the control and it costs nothing: grpupupu.c:362 sets the mouth
# to Stretch on the SAME frame the blink fires, through the same gcAddAnimAll,
# but with a non-NULL material table. If the mouth plays out over many frames
# and the eyes do not, the NULL material argument or the eye data is the
# difference; if both end in one frame the runtime is broken for this GObj.
#
# NO REBUILD. Everything is rooted at gGRCommonStruct, dGRPupupuWhispyEyesAnims
# and the port's normalize counters, all of which are globals.

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
        (Get-Date -Format 'yyyy-MM-dd') + '_whispy-blink-script.txt')
}
$context = Initialize-MelonDSVerifierContext `
    -Root $root -MelonDS '' -RunnerSlot $RunnerSlot -NoBuild
$melon_dir = Split-Path -Parent $context.MelonDSPath
$log_dir = Get-MelonDSVerifierLogDir -Root $root -RunnerSlot $RunnerSlot
$stdout = Join-Path $log_dir 'melonds.whispy-blink-script.stdout.log'
$stderr = Join-Path $log_dir 'melonds.whispy-blink-script.stderr.log'
$log_temp = if (-not [string]::IsNullOrWhiteSpace($env:SMASH64DS_VERIFY_TEMP_DIR)) {
    $env:SMASH64DS_VERIFY_TEMP_DIR
} else {
    Join-Path $root 'artifacts\verifier-temp\default'
}
$config_state = $null
$emulator = $null

$required = @(
    'grPupupuUpdateGObjAnims',
    'gGRCommonStruct',
    'dGRPupupuWhispyEyesAnims',
    'gNdsAObjEvent32NormalizeFailCount',
    'gNdsAObjEvent32NormalizeScriptCount',
    'gNdsAObjEvent32NormalizeReuseCount',
    'gNdsAObjEvent32NormalizeLastFailReason'
)
if ($Cadence) {
    $required += @(
        'sNdsRendererAdapterNativeStageWorkspace',
        'sNdsRendererAdapterStageWorldCache',
        'sNdsRendererAdapterStageWorldCacheCount',
        'sNdsRendererAdapterDObjWorldCache'
    )
}
$symbols = & $nm $elf | ForEach-Object { ($_ -split '\s+')[-1] }
$missing = @($required | Where-Object { $symbols -notcontains $_ })
if ($missing.Count -gt 0) {
    throw ("Whispy blink script probe symbols absent from {0}: {1}" -f $elf, ($missing -join ', '))
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

    # Never $fp/$sp/$pc/$lr/$rN: on ARM those ARE registers and the assignment
    # fails as "not an lvalue" against an unrelated line -- which is why the
    # obvious name for the facing side here is $side. gdb-markers.ps1 refuses
    # the script rather than letting it fail mid-run.
    $commands = @(
        'set pagination off',
        'set confirm off',
        'set remotetimeout 20',
        ("target remote 127.0.0.1:{0}" -f $context.GdbPort),

        'set $n = 0',
        'set $pending = 0',
        'set $fail0 = 0',
        'set $scr0 = 0',
        'set $reu0 = 0',
        'break grPupupuUpdateGObjAnims',
        'commands',
        'silent',
        'set $sta = gGRCommonStruct.pupupu.whispy_eyes_status',
        'set $side = gGRCommonStruct.pupupu.lr_players',
        'set $g0 = gGRCommonStruct.pupupu.map_gobj[0]',
        'set $g1 = gGRCommonStruct.pupupu.map_gobj[1]',
        'set $d0 = ($g0 != 0) ? (DObj *)$g0->obj : (DObj *)0',
        # The frame AFTER an add is the only place the result is visible: the
        # breakpoint sits at function entry, so this hit reads what the
        # PREVIOUS hit built. Counter deltas here are the whole of test (a).
        'if $pending != 0',
        'set $pending = 0',
        'printf "WPOST n=%d dfail=%u dscript=%u dreuse=%u lastreason=%u gframe=%f dspeed=%f dwait=%d dframe=%f djoint=%p droot=%d mouthframe=%f\n", $n, gNdsAObjEvent32NormalizeFailCount - $fail0, gNdsAObjEvent32NormalizeScriptCount - $scr0, gNdsAObjEvent32NormalizeReuseCount - $reu0, gNdsAObjEvent32NormalizeLastFailReason, ($g0 != 0) ? $g0->anim_frame : -1, ($d0 != 0) ? $d0->anim_speed : -1, ($d0 != 0) ? $d0->anim_wait : -999, ($d0 != 0) ? $d0->anim_frame : -1, ($d0 != 0) ? $d0->anim_joint.event32 : (void *)0, ($d0 != 0) ? $d0->is_anim_root : -1, ($g1 != 0) ? $g1->anim_frame : -1',
        # The ROOT carries no script (WJ0 is nil): the table is indexed in
        # DObj-tree order, so the `Wait 12` in WJ1 is counted down on a CHILD's
        # anim_wait, not on $d0. Reading only the root is what made the root's
        # zeroes look like a dead animation. anim_wait/anim_frame are f32 -- %f,
        # never %d.
        'set $c1 = ($d0 != 0) ? $d0->child : (DObj *)0',
        'set $c2 = ($c1 != 0) ? $c1->sib_next : (DObj *)0',
        'set $c3 = ($c1 != 0) ? $c1->child : (DObj *)0',
        'printf "  WD1 %p wait=%f frame=%f joint=%p root=%d\n", $c1, ($c1 != 0) ? $c1->anim_wait : -1, ($c1 != 0) ? $c1->anim_frame : -1, ($c1 != 0) ? $c1->anim_joint.event32 : (void *)0, ($c1 != 0) ? $c1->is_anim_root : -1',
        'printf "  WD2 %p wait=%f frame=%f joint=%p root=%d\n", $c2, ($c2 != 0) ? $c2->anim_wait : -1, ($c2 != 0) ? $c2->anim_frame : -1, ($c2 != 0) ? $c2->anim_joint.event32 : (void *)0, ($c2 != 0) ? $c2->is_anim_root : -1',
        'printf "  WD3 %p wait=%f frame=%f joint=%p root=%d\n", $c3, ($c3 != 0) ? $c3->anim_wait : -1, ($c3 != 0) ? $c3->anim_frame : -1, ($c3 != 0) ? $c3->anim_joint.event32 : (void *)0, ($c3 != 0) ? $c3->is_anim_root : -1',
        'end',
        ('if ($sta != -1) && ($n < ' + $Events + ')'),
        'set $n = $n + 1',
        'set $pending = 1',
        'set $fail0 = gNdsAObjEvent32NormalizeFailCount',
        'set $scr0 = gNdsAObjEvent32NormalizeScriptCount',
        'set $reu0 = gNdsAObjEvent32NormalizeReuseCount',
        'set $head = (unsigned int)gGRCommonStruct.pupupu.map_head',
        'set $joff = (unsigned int)dGRPupupuWhispyEyesAnims[$side][$sta][0]',
        'set $moff = (unsigned int)dGRPupupuWhispyEyesAnims[$side][$sta][1]',
        'set $tab = (unsigned int **)($joff + $head)',
        'printf "WPRE n=%d status=%d lr=%d head=%08x jointoff=%08x matoff=%08x table=%p gframe=%f mouthstatus=%d\n", $n, $sta, $side, $head, $joff, $moff, $tab, ($g0 != 0) ? $g0->anim_frame : -1, gGRCommonStruct.pupupu.whispy_mouth_status',
        # The table is a plain array indexed by DObj-tree order and is NOT
        # NULL-terminated at 4, so a blind 4-entry read runs off the end: the
        # 2026-08-12 capture printed WJ3 at 0x06010000 (VRAM) full of texture
        # bytes, which reads exactly like a decodable script. Accept an entry
        # only if it points into main RAM.
        'set $j = 0',
        'while $j < 4',
        'set $s = $tab[$j]',
        'if ($s != 0) && ((unsigned int)$s >= 0x02000000) && ((unsigned int)$s < 0x02400000)',
        'printf "  WJ%d %p : %08x %08x %08x %08x %08x %08x\n", $j, $s, $s[0], $s[1], $s[2], $s[3], $s[4], $s[5]',
        'else',
        'printf "  WJ%d (nil-or-out-of-range) %p\n", $j, $s',
        'end',
        'set $j = $j + 1',
        'end',
        'end',
        ('if ($n < ' + $Events + ') || ($pending != 0)'),
        'continue',
        'end',
        'end',

        'continue',
        'printf "WDONE n=%d\n", $n',
        'detach',
        'quit'
    )

    if ($Cadence) {
        # The 2026-08-12 child read settled that the SOURCE animation is healthy
        # (anim_wait counts the script's Wait 12 down, script attached,
        # is_anim_root=1). The open question is therefore presentation: does the
        # pose actually CHANGE between consecutive presented frames? Sample the
        # eye and mouth child DObjs once per presented frame and let the caller
        # diff the run. Two source ticks per present means anim_wait should fall
        # by 2 each sample while a script is playing; a flat run is the tell.
        $commands = @(
            'set pagination off',
            'set confirm off',
            'set remotetimeout 20',
            ("target remote 127.0.0.1:{0}" -f $context.GdbPort),
            'set $n = 0',
            'set $armed = 0',
            'break ndsBattlePlayableFrameCompleteMarker',
            'commands',
            'silent',
            'set $g0 = gGRCommonStruct.pupupu.map_gobj[0]',
            'set $g1 = gGRCommonStruct.pupupu.map_gobj[1]',
            'set $d0 = ($g0 != 0) ? (DObj *)$g0->obj : (DObj *)0',
            'set $e1 = ($d0 != 0) ? $d0->child : (DObj *)0',
            'set $m0 = ($g1 != 0) ? (DObj *)$g1->obj : (DObj *)0',
            'set $m1 = ($m0 != 0) ? $m0->child : (DObj *)0',
            # ARM on an actually-running animation. anim_wait holds -FLT_MAX
            # when nothing is playing, and Whispy's blinks are sparse: a blind
            # 26-frame sample on 2026-08-12 landed entirely between events and
            # every row was identical, which reads exactly like a frozen face.
            # A flat run only means something if the window contains motion.
            'if ($armed == 0) && ($e1 != 0) && ($e1->anim_wait > -1000000)',
            'set $armed = 1',
            'end',
            'if $armed != 0',
            'set $n = $n + 1',
            # ALL NINE channels plus the rotation angle, on BOTH animating
            # DObjs. A 3-channel sample cannot distinguish "the applier never
            # writes the transform" from "this script drives a channel I did
            # not read", and that distinction is the whole question.
            'printf "WCAD f=%d wait=%f frame=%f\n", $n, ($e1 != 0) ? $e1->anim_wait : -1, ($e1 != 0) ? $e1->anim_frame : -1',
            'printf "  E1 t=%f,%f,%f r=%f,%f,%f a=%f s=%f,%f,%f\n", ($e1 != 0) ? $e1->translate.vec.f.x : -1, ($e1 != 0) ? $e1->translate.vec.f.y : -1, ($e1 != 0) ? $e1->translate.vec.f.z : -1, ($e1 != 0) ? $e1->rotate.vec.f.x : -1, ($e1 != 0) ? $e1->rotate.vec.f.y : -1, ($e1 != 0) ? $e1->rotate.vec.f.z : -1, ($e1 != 0) ? $e1->rotate.a : -1, ($e1 != 0) ? $e1->scale.vec.f.x : -1, ($e1 != 0) ? $e1->scale.vec.f.y : -1, ($e1 != 0) ? $e1->scale.vec.f.z : -1',
            'set $e2 = ($e1 != 0) ? $e1->child : (DObj *)0',
            # ndsRendererAdapterBuildDObjLocalMatrix builds from dobj->xobjs[],
            # dispatched BY KIND -- so an animated scale.y only reaches the
            # matrix if this DObj actually carries a scale XObj. If the blink
            # writes scale.y on a node with no scale transform kind, the pose is
            # computed and then discarded, which is the last unmeasured link.
            'printf "  EX xobjs_num=%d k0=%d k1=%d k2=%d vec=%p vk=%d,%d,%d\n", ($e2 != 0) ? $e2->xobjs_num : -1, (($e2 != 0) && ($e2->xobjs_num > 0) && ($e2->xobjs[0] != 0)) ? $e2->xobjs[0]->kind : -1, (($e2 != 0) && ($e2->xobjs_num > 1) && ($e2->xobjs[1] != 0)) ? $e2->xobjs[1]->kind : -1, (($e2 != 0) && ($e2->xobjs_num > 2) && ($e2->xobjs[2] != 0)) ? $e2->xobjs[2]->kind : -1, ($e2 != 0) ? $e2->vec : (DObjVec *)0, (($e2 != 0) && ($e2->vec != 0)) ? $e2->vec->kinds[0] : -1, (($e2 != 0) && ($e2->vec != 0)) ? $e2->vec->kinds[1] : -1, (($e2 != 0) && ($e2->vec != 0)) ? $e2->vec->kinds[2] : -1',
            'printf "  E2 t=%f,%f,%f r=%f,%f,%f a=%f s=%f,%f,%f wait=%f frame=%f\n", ($e2 != 0) ? $e2->translate.vec.f.x : -1, ($e2 != 0) ? $e2->translate.vec.f.y : -1, ($e2 != 0) ? $e2->translate.vec.f.z : -1, ($e2 != 0) ? $e2->rotate.vec.f.x : -1, ($e2 != 0) ? $e2->rotate.vec.f.y : -1, ($e2 != 0) ? $e2->rotate.vec.f.z : -1, ($e2 != 0) ? $e2->rotate.a : -1, ($e2 != 0) ? $e2->scale.vec.f.x : -1, ($e2 != 0) ? $e2->scale.vec.f.y : -1, ($e2 != 0) ? $e2->scale.vec.f.z : -1, ($e2 != 0) ? $e2->anim_wait : -1, ($e2 != 0) ? $e2->anim_frame : -1',
            # BUGS.md row 1 downstream proof. Slice 44 used to pass
            # allow_stale=TRUE to BuildPersistentStageWorldMatrix on seven of
            # eight frames for these "dynamic" bindings. The source animation
            # above could therefore move while the cached world matrix stayed
            # frozen. Find binding 20's persistent-cache entry and print its
            # generation plus affine rows beside the eye scale so a cadence
            # capture proves the renderer, not merely the animation player.
            'set $bd = sNdsRendererAdapterNativeStageWorkspace.binding_dobjs[20]',
            'set $ci = 0',
            'set $found = -1',
            'while ($ci < sNdsRendererAdapterStageWorldCacheCount) && ($found < 0)',
            'if sNdsRendererAdapterStageWorldCache[$ci].dobj == $bd',
            'set $found = $ci',
            'else',
            'set $ci = $ci + 1',
            'end',
            'end',
            'if ($found >= 0) && (sNdsRendererAdapterDObjWorldCache != 0)',
            'set $ws = (unsigned int)sNdsRendererAdapterStageWorldCache[$found].world_slot',
            'printf "  RW bind=20 cache=%d gen=%u valf=%u slot=%u m=%d,%d,%d,%d|%d,%d,%d,%d|%d,%d,%d,%d|%d,%d,%d,%d\n", $found, sNdsRendererAdapterStageWorldCache[$found].generation, sNdsRendererAdapterStageWorldCache[$found].validated_frame, $ws, sNdsRendererAdapterDObjWorldCache[$ws].world.m[0][0], sNdsRendererAdapterDObjWorldCache[$ws].world.m[0][1], sNdsRendererAdapterDObjWorldCache[$ws].world.m[0][2], sNdsRendererAdapterDObjWorldCache[$ws].world.m[0][3], sNdsRendererAdapterDObjWorldCache[$ws].world.m[1][0], sNdsRendererAdapterDObjWorldCache[$ws].world.m[1][1], sNdsRendererAdapterDObjWorldCache[$ws].world.m[1][2], sNdsRendererAdapterDObjWorldCache[$ws].world.m[1][3], sNdsRendererAdapterDObjWorldCache[$ws].world.m[2][0], sNdsRendererAdapterDObjWorldCache[$ws].world.m[2][1], sNdsRendererAdapterDObjWorldCache[$ws].world.m[2][2], sNdsRendererAdapterDObjWorldCache[$ws].world.m[2][3], sNdsRendererAdapterDObjWorldCache[$ws].world.m[3][0], sNdsRendererAdapterDObjWorldCache[$ws].world.m[3][1], sNdsRendererAdapterDObjWorldCache[$ws].world.m[3][2], sNdsRendererAdapterDObjWorldCache[$ws].world.m[3][3]',
            'else',
            'printf "  RW bind=20 cache-miss target=%p count=%u\n", $bd, sNdsRendererAdapterStageWorldCacheCount',
            'end',
            'end',
            ('if $n < ' + $Frames),
            'continue',
            'end',
            'end',
            'continue',
            'printf "WCADDONE n=%d\n", $n',
            'detach',
            'quit'
        )
    }

    Invoke-GdbMarkerScript `
        -Gdb $gdb -Elf $elf -Root $root -Commands $commands `
        -ScriptName 'whispy_blink_script_probe.gdb' `
        -TimeoutSeconds $TimeoutSeconds | Out-Null
}
finally {
    # From the helper capture file, not its return value: a run that times out
    # waiting for its last event holds the same evidence as one that completes,
    # and only the second throws.
    $captured = Join-Path $log_temp 'whispy_blink_script_probe.gdb.out'
    if (Test-Path -LiteralPath $captured) {
        New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Artifact) |
            Out-Null
        Copy-Item -LiteralPath $captured -Destination $Artifact -Force
        Get-Content -LiteralPath $Artifact |
            Where-Object { $_ -match '^\s*W' } |
            ForEach-Object { Write-Output $_ }
        Write-Output "probe capture: $Artifact"
    }
    if ($null -ne $emulator) {
        Stop-Process -Id $emulator.Id -Force -ErrorAction SilentlyContinue
    }
    if ($null -ne $config_state) {
        Restore-MelonDSGdbConfig -State $config_state
    }
}
