[CmdletBinding()]
param(
    [string]$Build = 'build-r2-a5i3',
    [string]$Target = 'smash64ds-battle-playable-tickhud-hwtri',
    [ValidateRange(1, 8)][int]$RunnerSlot = 7,
    [ValidateRange(30, 400)][int]$TimeoutSeconds = 300
)

# ONE BATCHED PROBE FOR THE WHOLE PARTICLE/VFX CLUSTER, per BUG_FIXING_PROCESS
# v2: "plant one batched probe build that answers every open question in it",
# and rung 2 of the evidence ladder -- GDB on an EXISTING ROM, no build.
#
# It answers the contract rows that were blocking BUGS.md rows 1, 4 and 10 by
# reading the number that actually decides each one, rather than the number that
# is easy to reach:
#
# BATTLE PHASE ONLY. Results-reachable rows (the confetti) need their own short
# run at their own trigger; waiting through a match for them here is the exact
# anti-pattern the process names.
#
#   Whispy (row 1)  -- the emitter origin dust_xf->translate and the side
#                      lr_players chose, read at the line after the source
#                      assigns them. PREDICTED: exactly (-715, 100) or
#                      (-205, 100) from dGRPupupuWhispyDustEffectPositions,
#                      i.e. within 320 of the tree at -525 (ground.h:19). If it
#                      matches, the source placement is intact and the reported
#                      "too far away" is downstream -- the tree's own drawn
#                      position or the camera -- and NOT the effect.
#                      Also latches pc->size, whose script value is the contract.
#   Hit spark (row 4) -- the position efManagerDamageNormalLightMakeEffect is
#                      handed. PREDICTED: within a fighter's reach of a
#                      fighter, i.e. |x| < 2000; the reported symptom is sparks
#                      "across the stage", which would show as an x far from
#                      either fighter.
#   Star-KO (row 7)  -- whether efManagerSparkleWhiteDeadMakeEffect is reached
#                      at all under KO stress, and with what position.
#   Rebirth (row 3)  -- whether the halo maker returns non-NULL on the natural
#                      respawn path.
#
# Every value is latched in a `silent` breakpoint callback that continues
# immediately, so the guest is never held still and nothing here changes timing
# enough to move the behaviour being measured. One printf at the end, because a
# gdb batch abandons the rest of its commands on the first error and eighty
# separate reads would risk all of them on one bad symbol.

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
. (Join-Path $PSScriptRoot 'lib\gdb-markers.ps1')
. (Join-Path $PSScriptRoot 'lib\build-output.ps1')

$gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe'
$rom = Resolve-Smash64DSBuildOutput -Root $root -Target $Target -Build $Build -Extension '.nds'
$elf = Resolve-Smash64DSBuildOutput -Root $root -Target $Target -Build $Build -Extension '.elf'
$artifact = Join-Path $root 'artifacts\verification\vfx-contract-probe.txt'
$capture_helper = Join-Path $PSScriptRoot 'capture-running-melonds-window.ps1'
$context = Initialize-MelonDSVerifierContext `
    -Root $root -MelonDS '' -RunnerSlot $RunnerSlot -NoBuild
$melon_dir = Split-Path -Parent $context.MelonDSPath
$log_dir = Get-MelonDSVerifierLogDir -Root $root -RunnerSlot $RunnerSlot
$stdout = Join-Path $log_dir 'melonds.vfx-contract-probe.stdout.log'
$stderr = Join-Path $log_dir 'melonds.vfx-contract-probe.stderr.log'
$config_state = $null
$emulator = $null

# Every symbol this batch names, checked before the batch runs. A gdb command
# file abandons everything after its first error, so one absent symbol would
# silently cost the whole probe -- that has already happened once on this
# project and the rule is recorded in soak-freeze-watch.ps1.
$required = @(
    'grPupupuWhispyDustMakeEffect', 'gGRCommonStruct',
    'efManagerDamageNormalLightMakeEffect',
    'efManagerSparkleWhiteDeadMakeEffect', 'efManagerRebirthHaloMakeEffect',
    'efManagerDeadExplodeMakeEffect',
    'ndsBattlePlayableFrameCompleteMarker', 'gNdsParticleQuadMissCount',
    'gLBParticleTransformsUsedNum', 'gNdsParticleTransformsMax',
    'sLBParticleStructsAllocLinks',
    'gNdsBattlePlayablePacingPresentedFrames'
)
$nm = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-nm.exe'
$symbols = & $nm $elf | ForEach-Object { ($_ -split '\s+')[-1] }
$missing = $required | Where-Object { $symbols -notcontains $_ }
if ($missing.Count -gt 0) {
    throw "probe symbols absent from $elf : $($missing -join ', ')"
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

        # latches
        'set $whispy_calls = 0',
        'set $dust_frames = 0',
        'set $frame_stops = 0',
        'set $forced = 0',
        'set $want = 0',
        'set $leaf_frames = 0',
        'set $leaf_x = 0.0',
        'set $whispy_lr = -1',
        'set $whispy_x = 0.0',
        'set $whispy_y = 0.0',
        'set $whispy_rot = 0.0',
        'set $whispy_scale = 0.0',
        'set $slot1_frames = 0',
        'set $slot1_x = 0.0',
        'set $slot1_y = 0.0',
        'set $slot1_size = 0.0',
        'set $slot1_absmax = 0.0',
        'set $draw_calls = 0',
        'set $draw_masks = 0',
        'set $slot1_xf = 0',
        'set $slot1_xf_seen = 0',
        'set $slot1_xf_null = 0',
        'set $slot1_xf_status = -1',
        'set $slot1_xf_tx = 0.0',
        'set $slot1_xf_ty = 0.0',
        'set $pc1 = 0',
        'set $slot1_walked = 0',
        'set $slot1_xf_at_emitter = 0',
        'set $slot1_xf_elsewhere = 0',
        'set $slot1_xf_gen = -1',
        'set $spark_calls = 0', 'set $impact_calls = 0', 'set $doff_x = 0.0', 'set $doff_y = 0.0', 'set $djoint = 0', 'set $atk_x = 0.0', 'set $atk_y = 0.0', 'set $dmg_x = 0.0', 'set $dmg_y = 0.0', 'set $spark_ptr = 0',
        'set $spark_x = 0.0',
        'set $spark_y = 0.0',
        'set $spark_absmax = 0.0',
        'set $dead_calls = 0',
        'set $dead_x = 0.0',
        'set $dead_y = 0.0',
        'set $rebirth_calls = 0',
        'set $mouth_x = 0.0',
        'set $mouth_y = 0.0',
        'set $mouth_sx = 0.0',
        'set $eyes_x = 0.0',
        'set $eyes_y = 0.0',
        'set $ground_x = 0.0',
        'set $ground_y = 0.0',
        'set $ground_sx = 0.0',
        'set $eyes_dl = -1',
        'set $eyes_mobj = -1',
        'set $mouth_dl = -1',
        'set $mouth_mobj = -1',
        'set $eyes_desc_dl = 0',
        'set $mouth_desc_dl = 0',
        'set $eyes_desc_w0 = 0',
        'set $mouth_desc_w0 = 0',
        'set $explode_calls = 0',
        'set $cap_rebirth = 0',
        'set $cap_explode = 0',
        'set $cap_star = 0',

        # Call count only. grpupupu.c:507 was tried as the read site and gdb
        # resolved it to THREE locations because -Os inlines the maker, so a
        # latch taken there can be sampled in a copy that has not yet stored the
        # global -- which is how the first run reported xf_reads=0 and nearly
        # bought a wrong conclusion. Reading the function's own `xf`/`pc` locals
        # was tried before that and gdb answered "value has been optimized out",
        # which is what -Os does to a local that is dead after its last store.
        # Every value below is therefore read from a GLOBAL at the frame marker,
        # which is one location and cannot be confounded.
        'break grPupupuWhispyDustMakeEffect',
        'commands',
        'silent',
        'set $whispy_calls = $whispy_calls + 1',
        'set $want = 1',
        'continue',
        'end',

        # THE ROW-1 DISCRIMINATOR. efDisplayInitAll (efdisplay.c:79-98) builds
        # four draw GObjs and gives each one an alloc-link camera_mask: links
        # 0|2 on layer 18, link 1 on layer 15, link 3 on 25, link 4 on 10.
        # lbParticleDrawTextures skips any link whose bit is clear in the GObj
        # it was handed, so ORing every mask that actually reaches the draw says
        # which links are dispatched at all. Whispy's dust and leaves are link 1,
        # alone on layer 15 -- and every effect repaired this session (combat 0,
        # KO burst 2, confetti 4) lives on 18 or 10. If bit 1 never appears here,
        # layer 15 is not being walked and that is the whole bug.
        'break lbParticleDrawTextures',
        'commands',
        'silent',
        'set $draw_calls = $draw_calls + 1',
        'set $draw_masks = $draw_masks | gobj->camera_mask',
        'continue',
        'end',

        # Break on the ndsBase* IMPLEMENTATION, not the plain name. The plain
        # name resolved to "2 locations" at address 0x0 because the ROM carries
        # both efManagerDamageNormalLightMakeEffect (0x0209384c, an 8-byte
        # wrapper) and ndsBaseEFManagerDamageNormalLightMakeEffect (0x020919d0,
        # the real body) -- so gdb split the breakpoint and ran the callback
        # where r0 is not the argument. That is why two consecutive runs latched
        # spark_calls=6 with every position exactly 0.0 and looked, wrongly,
        # like sparks spawning at the origin.
        #
        # Read the position through $r0 rather than `pos`: the signature is
        # (Vec3f *pos, s32, s32, sb32) so pos is the first argument and is in r0
        # at entry, while the NAME is optimized out at -Os.
        # `break *FUNC`, with the star. Plain `break FUNC` skips the prologue to
        # the first statement, and by then r0 has been reused -- which is why
        # pointing the breakpoint at the real implementation fixed the "2
        # locations at 0x0" split and STILL read every position as 0.0. The star
        # form stops at the exact entry address, where r0 is still argument one.
        # Read the CALLER's live local, not the callee's registers. Three
        # attempts at the callee all returned 0.0 -- the ndsBase implementation
        # (which fixed a real "2 locations at 0x0" split), $r0, and `break *FUNC`
        # for the pre-prologue entry -- so $r0 is not readable on this remote,
        # consistent with inferior calls SIGILLing here. battleship_efmanager.c
        # :1379 is the port wrapper's tail call, where `pos` is a named
        # parameter in a TU with its own debug info.
        # NOT breaking at gmcollision.c:1979 to split attack_pos from damage_pos.
        # Tried on 2026-08-02: the line resolves fine (0x206fb2e) but -Os has
        # discarded both locals, gdb answers `No symbol "attack_pos" in current
        # context`, and because a gdb batch abandons everything after its first
        # error that ONE read cost the whole probe. Registers are not a way
        # around it either -- $r0 is unreadable on this remote. Split that step
        # statically, or with a temporary counter compiled into the port, not
        # from here.
        # Splits the midpoint WITHOUT a build. attack_pos derives straight from
        # ft_attack_coll->pos_curr/pos_prev, and ft_attack_coll is used ON the
        # ftmain.c:2734 call line, so unlike the dead attack_pos/damage_pos
        # locals at gmcollision.c:1979 the compiler has to keep it. If pos_curr
        # is zero the attack side is the fault; if it is sane then damage_pos
        # must be its exact negative, which can only come from the victim's part
        # matrix, and that names the part-matrix pipeline instead.
        'break ftmain.c:2734',
        'commands',
        'silent',
        'set $impact_calls = $impact_calls + 1',
        'set $atk_x = ft_attack_coll->pos_curr.x',
        'set $atk_y = ft_attack_coll->pos_curr.y',
        # And `pos` itself, the caller's own local at the same line -- the exact
        # Vec3f whose address is handed to the maker. attack_pos is provably
        # sane here, so if `pos` is already zero at the call site then
        # gmCollisionGetFighterAttackDamagePosition zeroed it despite a good
        # attack side; if `pos` is sane then something between this line and the
        # callee loses it.
        'set $dmg_x = pos.x',
        'set $dmg_y = pos.y',
        # The last unmeasured INPUT. damage_pos starts as damage_coll->offset and
        # is then multiplied by the victim's part matrix, so if the offset itself
        # is wrong the matrix is innocent. hitlog is live here -- its
        # attacker_player is used on this very line -- and offset is plain
        # pointer-chasing, no call, so this dodges the ftGetParts SIGILL.
        'set $doff_x = hitlog->damage_coll->offset.x',
        'set $doff_y = hitlog->damage_coll->offset.y',
        'set $djoint = (unsigned long)hitlog->damage_coll->joint',
        'continue',
        'end',

        'break battleship_efmanager.c:1379',
        'commands',
        'silent',
        'set $spark_calls = $spark_calls + 1',
        # THE CONTROL. Four read paths have now returned pos->x == 0.0 and the
        # last is a clean single-location breakpoint on a named parameter, so
        # "the position really is the origin" is the leading reading -- which
        # would itself be row 4's bug, sparks at stage centre instead of at the
        # contact. But four failures and four zeroes look identical from here.
        # `player` and `size` are siblings of `pos` in the same frame and are
        # known non-zero (size is the caller's magnitude, player is 0 or 1, and
        # a Fox hit gives 1). If they read sensibly, the frame is readable and
        # the zero position is real. If they read 0 too, the frame is not
        # readable and nothing from this breakpoint means anything.
        'set $spark_ptr = (unsigned long)pos', 'set $spark_player = player',
        'set $spark_size = size',
        'set $spark_p = pos',
        'set $spark_x = $spark_p->x',
        'set $spark_y = $spark_p->y',
        'if $spark_p->x > $spark_absmax',
        'set $spark_absmax = $spark_p->x',
        'end',
        'if -$spark_p->x > $spark_absmax',
        'set $spark_absmax = -$spark_p->x',
        'end',
        'continue',
        'end',

        'break ndsBaseEFManagerSparkleWhiteDeadMakeEffect',
        'commands',
        'silent',
        'set $dead_calls = $dead_calls + 1',
        'set $dead_x = pos->x',
        'set $dead_y = pos->y',
        'if $cap_star == 0',
        'set $cap_star = 6',
        'end',
        'continue',
        'end',

        'break efManagerRebirthHaloMakeEffect',
        'commands',
        'silent',
        'set $rebirth_calls = $rebirth_calls + 1',
        'set $cap_rebirth = 24',
        'continue',
        'end',

        # The KO burst itself, for BUGS row 9 ("it gets clipped or something so
        # I can't see it fully"). Six frames after the maker, which is inside
        # dEFManagerDeadExplodeGenID's own animation rather than at its first
        # frame, so a burst that dies partway through shows as a partial frame
        # rather than as nothing.
        'break efManagerDeadExplodeMakeEffect',
        'commands',
        'silent',
        'set $explode_calls = $explode_calls + 1',
        'if $cap_explode == 0',
        'set $cap_explode = 6',
        'end',
        'continue',
        'end',

        # THE UNAMBIGUOUS READ, and the run governor.
        #
        # It also FORCES the wind, once. grPupupuInitAll seeds whispy_wind_wait
        # to syUtilsRandIntRange(1140) + 960 and grPupupuWhispyUpdateStop reseeds
        # it the same way, so the gap between blows is 16 to 35 SOURCE seconds
        # (grvars.h). The first version of this probe stopped after 450 frames
        # and read whispy_calls=0 -- not a finding about Whispy, just a run
        # shorter than one wind cycle. Writing the countdown down to 4 changes
        # WHEN the blow happens and nothing about WHERE: the emitter position is
        # dGRPupupuWhispyDustEffectPositions[lr_players], a two-entry constant
        # table, and lr_players is chosen from fighter positions at blow time.
        # Both entries are a pass for the question being asked.
        'break ndsBattlePlayableFrameCompleteMarker',
        'commands',
        'silent',
        'set $frame_stops = $frame_stops + 1',
        'if ($frame_stops > 60) && ($forced < 3) && (gGRCommonStruct.pupupu.whispy_wind_wait > 4)',
        'set gGRCommonStruct.pupupu.whispy_wind_wait = 4',
        'set $forced = $forced + 1',
        'end',
        # LATCH ONLY ON THE FRAME AFTER A BLOW, never every frame.
        #
        # grPupupuFlowersFrontLoopEnd ejects the dust structs by generator id at
        # the end of each blow and does NOT null gGRCommonStruct.pupupu.dust_xf,
        # so the pointer dangles into a recycled pool slot for the 16-35 seconds
        # until the next blow. That is source-faithful -- the only readers are
        # guarded by whispy status, and the next maker overwrites it -- but it
        # makes a per-frame sample worthless. The first run of this probe read
        # 653 non-NULL frames out of 900 and reported translate (2238.58,
        # 134.93) with rotate.y 0 at lr_players 0, which is not the source's
        # (-715|-205, 100) and not its DTOR32(180) either. That was not a
        # finding about Whispy; it was 650 samples of whatever last used the
        # slot. $want is set by the maker's own breakpoint and consumed here one
        # frame later, when the assignment at grpupupu.c:500 has definitely run
        # and nothing can have recycled the transform yet.
        'if ($want == 1) && (gGRCommonStruct.pupupu.dust_xf != 0)',
        'set $want = 0',
        'set $dust_frames = $dust_frames + 1',
        'set $whispy_lr = gGRCommonStruct.pupupu.lr_players',
        'set $whispy_x = gGRCommonStruct.pupupu.dust_xf->translate.x',
        'set $whispy_y = gGRCommonStruct.pupupu.dust_xf->translate.y',
        'set $whispy_rot = gGRCommonStruct.pupupu.dust_xf->rotate.y',
        'set $whispy_scale = gGRCommonStruct.pupupu.dust_xf->scale.x',
        # WHERE THE DS ACTUALLY DRAWS THE TREE. The emitter matching
        # dGRPupupuWhispyDustEffectPositions only proves the effect agrees with
        # BattleShip's numbers; it says nothing about whether the DS puts the
        # tree where those numbers assume. map_gobj[0] is the eyes and [1] the
        # mouth (grpupupu.c), both children of the ground, so their translate is
        # the face position the dust is supposed to come out of. Compare against
        # GRPUPUPU_WHISPY_POS_X -525 and against the emitter y of 100.
        'if gGRCommonStruct.pupupu.map_gobj[1] != 0',
        'set $mouth_x = ((DObj *)gGRCommonStruct.pupupu.map_gobj[1]->obj)->translate.vec.f.x',
        'set $mouth_y = ((DObj *)gGRCommonStruct.pupupu.map_gobj[1]->obj)->translate.vec.f.y',
        'set $mouth_sx = ((DObj *)gGRCommonStruct.pupupu.map_gobj[1]->obj)->scale.vec.f.x',
        'end',
        'if gGRCommonStruct.pupupu.map_gobj[0] != 0',
        'set $eyes_x = ((DObj *)gGRCommonStruct.pupupu.map_gobj[0]->obj)->translate.vec.f.x',
        'set $eyes_y = ((DObj *)gGRCommonStruct.pupupu.map_gobj[0]->obj)->translate.vec.f.y',
        'end',
        # DOES THE DS DRAW WHISPY'S FACE AT ALL? In the owner's N64 capture the
        # eyes and mouth are the only thing identifying WHICH trunk is Whispy,
        # and dust appearing beside a faceless trunk would read as coming from
        # nowhere -- which is a complete explanation of "spawning too far away
        # from Whispy the Tree" that has nothing to do with the emitter. A DObj
        # with a NULL display list is built, positioned, walked and invisible.
        # WALK THE TREE, do not read the root. A DObj root with dl == NULL is
        # the ordinary shape of a scene-graph node -- the geometry hangs off
        # ->child and ->sib_next -- so "root dl is 0" proves nothing at all.
        # eyes_dl counts nodes in the eyes subtree that actually carry a display
        # list; 0 across the whole walk is the only reading that means no face.
        'set $eyes_dl = 0',
        'set $eyes_mobj = 0',
        # POINTER WALK, NEVER AN INFERIOR CALL. `set $d = gcGetTreeDObjNext($d)`
        # was tried and the target took SIGILL at 0x0262f594 -- gdb cannot call
        # a function on this remote without getting the ARM/Thumb entry state
        # right, and a crashed inferior costs the whole run. Root plus one level
        # of children and their siblings is enough to answer the question.
        # ASSET SIDE vs RUNTIME SIDE, which is the only question left on row 1.
        # grPupupuMakeMapGObj hands gcSetupCustomDObjs a DObjDesc at
        # map_head + offset (0x10f0 eyes, 0x1770 mouth) and a DObjDesc is 44
        # bytes with its display-list pointer at +4. If the desc's pointer is
        # non-zero and the built DObj's is zero, the loss is in
        # gcSetupCustomDObjs and it is fixable here. If the desc's is zero too,
        # Dream Land's map file never carried the face and it is an asset job.
        'set $mh = (char *)gGRCommonStruct.pupupu.map_head',
        'if $mh != 0',
        'set $eyes_desc_dl = ((unsigned int *)($mh + 0x10f0))[1]',
        'set $mouth_desc_dl = ((unsigned int *)($mh + 0x1770))[1]',
        'set $eyes_desc_w0 = ((unsigned int *)($mh + 0x10f0))[0]',
        'set $mouth_desc_w0 = ((unsigned int *)($mh + 0x1770))[0]',
        'end',
        'set $d = ((DObj *)gGRCommonStruct.pupupu.map_gobj[0]->obj)',
        'if $d->dl != 0',
        'set $eyes_dl = $eyes_dl + 1',
        'end',
        'set $d = $d->child',
        'while $d != 0',
        'if $d->dl != 0',
        'set $eyes_dl = $eyes_dl + 1',
        'end',
        'if $d->mobj != 0',
        'set $eyes_mobj = $eyes_mobj + 1',
        'end',
        'set $d = $d->sib_next',
        'end',
        'set $mouth_dl = 0',
        'set $mouth_mobj = 0',
        'set $d = ((DObj *)gGRCommonStruct.pupupu.map_gobj[1]->obj)',
        'if $d->dl != 0',
        'set $mouth_dl = $mouth_dl + 1',
        'end',
        'set $d = $d->child',
        'while $d != 0',
        'if $d->dl != 0',
        'set $mouth_dl = $mouth_dl + 1',
        'end',
        'if $d->mobj != 0',
        'set $mouth_mobj = $mouth_mobj + 1',
        'end',
        'set $d = $d->sib_next',
        'end',
        'if gGCCommonLinks[1] != 0',
        'set $ground_x = ((DObj *)gGCCommonLinks[1]->obj)->translate.vec.f.x',
        'set $ground_y = ((DObj *)gGCCommonLinks[1]->obj)->translate.vec.f.y',
        'set $ground_sx = ((DObj *)gGCCommonLinks[1]->obj)->scale.vec.f.x',
        'end',
        'if gGRCommonStruct.pupupu.leaves_xf != 0',
        'set $leaf_frames = $leaf_frames + 1',
        'set $leaf_x = gGRCommonStruct.pupupu.leaves_xf->translate.x',
        'end',
        'end',
        # Alloc-link slot 1 IS the Whispy link. LBPARTICLE_MASK_GENLINK(0) is 8
        # and lbparticle.c:324 files a struct under bank_id >> 3, so the dust and
        # leaves are the only things in slot 1 -- efdisplay.c:87 draws exactly
        # that slot through the DL-15 CLD GObj. pos is LOCAL to the transform, so
        # this plus whispy_x is the world position, and the running absmax says
        # how far downwind the sheet actually reaches.
        # One screenshot, taken while the core is HALTED in the middle of a
        # blow. The measured numbers say the emitter is exactly where BattleShip
        # puts it, so the remaining half of "spawning too far away from Whispy
        # the Tree" is a question about what the owner sees, and the owner is
        # the oracle for that (docs render-fidelity doctrine). Second blow, not
        # the first, so the wind is in its steady state rather than its opening
        # frame. gdb holds the emulator here, so the window is showing the frame
        # being measured and not a later one.
        # EARLY in the blow, not mid-flight. The first capture was taken at
        # slot1_frames 120 and showed no dust anywhere on screen -- by then the
        # sheet has swept 1,359 units downwind of the emitter and left the
        # camera. Frame 20 is while the particles are still near
        # dGRPupupuWhispyDustEffectPositions, which is the frame that answers
        # "is it coming out of the tree".
        # EVENT CAPTURES. Each maker arms a countdown and the frame marker
        # spends it, so the screenshot lands a fixed number of frames INTO the
        # effect rather than on the frame it was constructed -- an effect is
        # rarely visible on frame 0, and the KO burst in particular is reported
        # as partially visible, which only a mid-animation frame can show.
        'if $cap_rebirth > 0',
        'set $cap_rebirth = $cap_rebirth - 1',
        'if $cap_rebirth == 0',
        ('shell pwsh -NoProfile -ExecutionPolicy Bypass -File "' +
            $capture_helper + '" -EmulatorProcessId ' + $emulator.Id +
            ' -Output "artifacts/visibility/' +
            '2026-08-01_rebirth-halo-probe.png"'),
        'end',
        'end',
        'if $cap_explode > 0',
        'set $cap_explode = $cap_explode - 1',
        'if $cap_explode == 0',
        ('shell pwsh -NoProfile -ExecutionPolicy Bypass -File "' +
            $capture_helper + '" -EmulatorProcessId ' + $emulator.Id +
            ' -Output "artifacts/visibility/' +
            '2026-08-01_ko-burst-probe.png"'),
        'end',
        'end',
        'if $cap_star > 0',
        'set $cap_star = $cap_star - 1',
        'if $cap_star == 0',
        ('shell pwsh -NoProfile -ExecutionPolicy Bypass -File "' +
            $capture_helper + '" -EmulatorProcessId ' + $emulator.Id +
            ' -Output "artifacts/visibility/' +
            '2026-08-01_star-ko-probe.png"'),
        'end',
        'end',

        'if ($slot1_frames == 20) && (sLBParticleStructsAllocLinks[1] != 0)',
        # pwsh, NOT powershell. scripts/lib/melonds.ps1:349 uses a PS7 ternary,
        # so every harness script that dot-sources it is a parse error under
        # Windows PowerShell 5.1 -- which is what `powershell` resolves to. The
        # whole harness is driven from pwsh, so this only surfaces when a script
        # is launched from somewhere that does not already know that, such as
        # gdb's `shell`.
        # Date the capture at RUN time, not at edit time. This was a hardcoded
        # 2026-08-01 and the 2026-08-02 re-observation silently overwrote it, so
        # the artifact directory claimed a file was a day older than its
        # contents -- the one thing a visibility artifact must never lie about,
        # since the whole point of the directory is comparing across dates.
        ('shell pwsh -NoProfile -ExecutionPolicy Bypass -File "' +
            $capture_helper + '" -EmulatorProcessId ' + $emulator.Id +
            ' -Output "artifacts/visibility/' +
            (Get-Date -Format 'yyyy-MM-dd') + '_whispy-blow-probe.png"'),
        'end',

        'if sLBParticleStructsAllocLinks[1] != 0',
        'set $slot1_frames = $slot1_frames + 1',
        'set $slot1_x = sLBParticleStructsAllocLinks[1]->pos.x',
        'set $slot1_y = sLBParticleStructsAllocLinks[1]->pos.y',
        'set $slot1_size = sLBParticleStructsAllocLinks[1]->size',
        'if sLBParticleStructsAllocLinks[1]->pos.x > $slot1_absmax',
        'set $slot1_absmax = sLBParticleStructsAllocLinks[1]->pos.x',
        'end',
        'if -sLBParticleStructsAllocLinks[1]->pos.x > $slot1_absmax',
        'set $slot1_absmax = -sLBParticleStructsAllocLinks[1]->pos.x',
        'end',
        # pc->pos above is the PRE-transform position and it has always looked
        # sane. ndsParticleTransformForDraw (battleship_lbparticle.c:1161-1194)
        # then multiplies it by pc->xf->affine, so a transform pointing at an
        # ejected slot turns a sane pos into a garbage world position and the
        # quad rasterizes nowhere. grPupupuFlowersFrontLoopEnd ejects without
        # nulling, and this run already reported a ground transform reading
        # -1.9e20. Latch the pointer, its status, and its translate: a status
        # outside 0..2 or a translate in the e20 range is the ejected slot.
        # WALK the list, do not read its head. Reading
        # sLBParticleStructsAllocLinks[1]->xf once per frame samples whatever
        # particle happens to be at the head, which need not be Whispy's dust at
        # all -- slot 1 is whatever GENLINK(0) put there. A published "root
        # cause" was built on that head read on 2026-08-02 and had to be
        # retracted, because 554 samples of one pointer is not a statement about
        # 554 particles. Count particles whose transform translate is the
        # emitter's -715 against those holding anything else, so the two
        # populations are separated instead of conflated.
        'set $pc1 = sLBParticleStructsAllocLinks[1]',
        'while $pc1 != 0',
        'set $slot1_walked = $slot1_walked + 1',
        'if $pc1->xf == 0',
        'set $slot1_xf_null = $slot1_xf_null + 1',
        'else',
        'set $slot1_xf_seen = $slot1_xf_seen + 1',
        'if ($pc1->xf->translate.x > -716) && ($pc1->xf->translate.x < -714)',
        'set $slot1_xf_at_emitter = $slot1_xf_at_emitter + 1',
        'else',
        'set $slot1_xf_elsewhere = $slot1_xf_elsewhere + 1',
        'set $slot1_xf_tx = $pc1->xf->translate.x',
        'set $slot1_xf_ty = $pc1->xf->translate.y',
        'set $slot1_xf_gen = $pc1->generator_id',
        'end',
        'end',
        'set $pc1 = $pc1->next',
        'end',
        'end',
        # The callback stops itself rather than a separate tbreak at the same
        # address: two breakpoints on one PC, one of them auto-continuing, means
        # the continue resumes straight past the other's stop and the batch
        # never regains control. That cost one 260-second timeout.
        'if $frame_stops < 900',
        'continue',
        'end',
        'end',

        # BATTLE PHASE ONLY, and only long enough for two forced blows. The
        # process doc forbids waiting through a match for a trigger that is not
        # at the end of one; Results-only rows (the confetti) need their own
        # short run at their own trigger.
        'continue',

        ('printf "VFXCONTRACT whispy_calls=%d lr=%d whispy_x=%f whispy_y=%f whispy_rotY=%f whispy_scale=%f ' +
            'slot1_frames=%d slot1_x=%f slot1_y=%f slot1_size=%f slot1_absmax=%f ' +
            'spark_calls=%d spark_x=%f spark_y=%f spark_absmax=%f spark_player=%d spark_size=%d spark_ptr=%08lx impact=%d atk=%f,%f pos=%f,%f doff=%f,%f djoint=%08lx ' +
            'dead_calls=%d dead_x=%f dead_y=%f rebirth_calls=%d explode_calls=%d ' +
            'frame_stops=%d forced=%d dust_frames=%d leaf_frames=%d leaf_x=%f ' +
            'mouth=%f,%f mouth_sx=%f eyes=%f,%f ground=%f,%f ground_sx=%f ' +
            'eyes_dl=%d eyes_mobj=%d mouth_dl=%d mouth_mobj=%d ' +
            'eyes_desc=%#x,%#x mouth_desc=%#x,%#x ' +
            'transforms_used=%u transforms_max=%u structs_max=%u ' +
            # The COUNT says how much was refused; only the MASK says WHAT. Row 1
            # turns on whether Whispy's own dust and leaf textures are among the
            # refusals -- a 6.7% miss rate is equally consistent with "the wind is
            # invisible" and "some unrelated tail effect is".
            'miss=%u emit=%u missmask=%08x,%08x missframes=%08x ' +
            'cam_dist=%f cam_fovy=%f WSUB ok=%u fail=%u clamped=%u at=%f,%f sz=%f cam_vw=%d cam_vh=%d ' +
            'draw_calls=%d draw_masks=%llx ' +
            'walked=%d xf_seen=%d xf_null=%d at_emitter=%d elsewhere=%d xf_t=%f,%f gen=%d\n", ' +
            '$whispy_calls, $whispy_lr, ' +
            '$whispy_x, $whispy_y, $whispy_rot, $whispy_scale, ' +
            '$slot1_frames, $slot1_x, $slot1_y, $slot1_size, $slot1_absmax, ' +
            '$spark_calls, $spark_x, $spark_y, $spark_absmax, ' + '$spark_player, $spark_size, $spark_ptr, ' + '$impact_calls, $atk_x, $atk_y, $dmg_x, $dmg_y, ' + '$doff_x, $doff_y, $djoint, ' +
            '$dead_calls, $dead_x, $dead_y, $rebirth_calls, $explode_calls, ' +
            '$frame_stops, $forced, ' +
            '$dust_frames, $leaf_frames, $leaf_x, ' +
            '$mouth_x, $mouth_y, $mouth_sx, $eyes_x, $eyes_y, ' +
            '$ground_x, $ground_y, $ground_sx, ' +
            '$eyes_dl, $eyes_mobj, $mouth_dl, $mouth_mobj, ' +
            '$eyes_desc_w0, $eyes_desc_dl, $mouth_desc_w0, $mouth_desc_dl, ' +
            'gLBParticleTransformsUsedNum, gNdsParticleTransformsMax, ' +
            'gNdsParticleStructsMax, ' +
            'gNdsParticleQuadMissCount, gNdsParticleQuadEmitCount, ' +
            'gNdsParticleQuadMissMask[0], gNdsParticleQuadMissMask[1], ' +
            'gNdsParticleQuadMissFrameMask, ' +
            'gGMCameraStruct.target_dist, gGMCameraStruct.fovy, ' +
            'gNdsWhispySubmitOk, gNdsWhispySubmitFail, gNdsWhispyDrawClamped, ' + 'gNdsWhispyDrawX, gNdsWhispyDrawY, gNdsWhispyDrawSize, ' +             'gGMCameraStruct.viewport_width, gGMCameraStruct.viewport_height, ' +
            '$draw_calls, $draw_masks, ' +
            '$slot1_walked, $slot1_xf_seen, $slot1_xf_null, ' + '$slot1_xf_at_emitter, $slot1_xf_elsewhere, ' + '$slot1_xf_tx, $slot1_xf_ty, $slot1_xf_gen'),
        'detach',
        'quit'
    )

    $capture = Invoke-GdbMarkerScript `
        -Gdb $gdb -Elf $elf -Root $root -Commands $commands `
        -ScriptName 'vfx_contract_probe.gdb' `
        -TimeoutSeconds $TimeoutSeconds
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $artifact) | Out-Null
    Set-Content -LiteralPath $artifact -Value $capture
    $capture | Select-String -Pattern 'VFXCONTRACT'
    Write-Output "probe capture: $artifact"
}
finally {
    if ($emulator -ne $null) {
        Stop-Process -Id $emulator.Id -Force -ErrorAction SilentlyContinue
    }
    if ($config_state -ne $null) {
        Restore-MelonDSGdbConfig -State $config_state
    }
}
