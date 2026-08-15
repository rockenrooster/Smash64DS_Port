[CmdletBinding()]
param(
    [string]$Build = 'build-c141-position',
    [string]$Target = 'smash64ds-battle-playable-tickhud-hwtri',
    [ValidateRange(1, 8)][int]$RunnerSlot = 8,
    [ValidateRange(30, 600)][int]$TimeoutSeconds = 240,
    [switch]$Standing,
    [switch]$TraceCrouchExit,
    [switch]$DumpHurtboxes,
    # Stop immediately after the first natural shot accepted while Mario is in
    # the requested pose. This is the cheap post-fix mode: it proves the live
    # attack centre/radius without waiting forever for a collision that is now
    # expected NOT to happen.
    [switch]$InspectShotOnly,
    [ValidatePattern('^[A-Za-z0-9][A-Za-z0-9._-]*$')]
    [string]$EvidenceLabel = '2026-08-12_fox-crouch-collision'
)

# BUGS.md: on N64 Mario can usually crouch under Fox's Blaster, while the port
# still reports a hit. This is a GAMEPLAY probe, deliberately separate from the
# presentation-only beam/muzzle alignment work.
#
# Contract from BattleShip:
#   * wpmanager.c:206 halves FoxSpecial1's size=40 -> attack radius 20.
#   * ftmain.c:3388-3408 tests each live Mario FTDamageColl in order and passes
#     the exact colliding pointer to ftMainUpdateDamageStatWeapon.
#   * gmcollision.c:1498 rebuilds that hurtbox joint's inverse/scale before
#     gmCollisionTestRectangle, so by the time the damage-stat function runs the
#     joint mtx_translate is the exact matrix the successful collision used.
#
# This script supplies only NATURAL player input. It never writes fighter
# position, status, animation, weapon position, collision data, or damage. In
# crouch mode it holds Down until source SquatWait (status 29), then waits for a
# real Fox Blaster hit and records the exact laser and hurtbox geometry that
# produced it. -Standing leaves Mario neutral and records the standing control.

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
. (Join-Path $PSScriptRoot 'lib\gdb-markers.ps1')
. (Join-Path $PSScriptRoot 'lib\build-output.ps1')

$gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe'
$nm = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-nm.exe'
$objdump = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-objdump.exe'
$rom = Resolve-Smash64DSBuildOutput -Root $root -Target $Target -Build $Build -Extension '.nds'
$elf = Resolve-Smash64DSBuildOutput -Root $root -Target $Target -Build $Build -Extension '.elf'
$artifact = Join-Path $root ('artifacts\verification\' + $EvidenceLabel + '.txt')
$context = Initialize-MelonDSVerifierContext -Root $root -MelonDS '' -RunnerSlot $RunnerSlot -NoBuild
$melonDir = Split-Path -Parent $context.MelonDSPath
$logDir = Get-MelonDSVerifierLogDir -Root $root -RunnerSlot $RunnerSlot
$stdout = Join-Path $logDir 'melonds.fox-crouch-collision.stdout.log'
$stderr = Join-Path $logDir 'melonds.fox-crouch-collision.stderr.log'
$configState = $null
$emulator = $null
$modeValue = if ($Standing) { '0' } else { '1' }

function Get-ElfSymbolAddress {
    param([Parameter(Mandatory=$true)][string]$Name)
    $line = & $nm $elf | Where-Object { $_ -match ("\s" + [regex]::Escape($Name) + '$') } | Select-Object -First 1
    if (-not $line) { throw "ELF symbol '$Name' is absent from $elf" }
    return [Convert]::ToUInt32(($line -split '\s+')[0], 16)
}

$pads = Get-ElfSymbolAddress 'sControllerPlaybackPads'
$connected = Get-ElfSymbolAddress 'sControllerPlaybackConnectedMask'
$enabled = Get-ElfSymbolAddress 'sControllerPlaybackEnabled'
$required = @(
    'scVSBattleStartBattle',
    'ndsBattlePlayableFrameCompleteMarker',
    'battleship_wpFoxBlasterMakeWeapon',
    'gmCollisionCheckWeaponAttackFighterDamageCollide',
    'gSCManagerBattleState',
    'gNdsBattlePlayableFoxCpuEnabled'
)
$symbols = & $nm $elf | ForEach-Object { ($_ -split '\s+')[-1] }
$missing = @($required | Where-Object { $symbols -notcontains $_ })
if ($missing.Count -gt 0) {
    throw ('Fox crouch probe symbols absent: ' + ($missing -join ', '))
}

# Optimized-build rule from BUG_FIXING_PROCESS.md: if we must break inside a
# function, prove the address from disassembly. The collision helper tail-calls
# no one: its BL to gmCollisionTestRectangle leaves the boolean result in r0;
# r5 still holds damage_coll and r7 still holds attack_coll. Derive the address
# immediately after that BL from THIS ELF rather than pinning c141's 0x0207fe24.
$collisionDisassembly = @(& $objdump -d `
    --disassemble=gmCollisionCheckWeaponAttackFighterDamageCollide $elf)
$rectangleLine = -1
for ($i = 0; $i -lt $collisionDisassembly.Count; $i++) {
    if ($collisionDisassembly[$i] -match '<gmCollisionTestRectangle>') {
        $rectangleLine = $i
        break
    }
}
if (($rectangleLine -lt 0) -or ($rectangleLine + 1 -ge $collisionDisassembly.Count) -or
    ($collisionDisassembly[$rectangleLine + 1] -notmatch '^\s*([0-9a-fA-F]+):')) {
    throw 'Could not derive the post-gmCollisionTestRectangle result address from this ELF.'
}
$collisionResultAddress = [Convert]::ToUInt32($matches[1], 16)

try {
    $configState = Enable-MelonDSGdbConfig -MelonDSPath $context.MelonDSPath `
        -GdbPort $context.GdbPort -Persistent -MuteAudio
    Remove-Item -LiteralPath $stdout, $stderr, $artifact -Force -ErrorAction SilentlyContinue
    # No screenshot is taken here; keep the diagnostic emulator out of the
    # owner's way while GDB owns the observation surface.
    $emulator = Start-Process -FilePath $context.MelonDSPath -ArgumentList $rom `
        -WorkingDirectory $melonDir -RedirectStandardOutput $stdout `
        -RedirectStandardError $stderr -WindowStyle Hidden -PassThru
    Wait-MelonDSGdbListener -Process $emulator -Port $context.GdbPort | Out-Null

    $commands = @(
        'set pagination off',
        'set confirm off',
        'set remotetimeout 20',
        ("target remote 127.0.0.1:{0}" -f $context.GdbPort),
        'tbreak scVSBattleStartBattle',
        'continue',
        'set variable gNdsBattlePlayableFoxCpuEnabled = 1',
        # Give the scene one completed frame so the fighter pointers and input
        # pipeline are live before external playback takes ownership.
        'tbreak ndsBattlePlayableFrameCompleteMarker',
        'continue',
        'set $mario_gobj = (GObj *)gSCManagerBattleState->players[0].fighter_gobj',
        'set $mario = (FTStruct *)$mario_gobj->user_data.p',
        ('set {{unsigned int}}0x{0:x8} = 0' -f $pads),
        ('set {{signed char}}0x{0:x8} = 0' -f ($pads + 2)),
        ('set {{signed char}}0x{0:x8} = {1}' -f ($pads + 3), $(if ($Standing) { 0 } else { -80 })),
        ('set {{unsigned int}}0x{0:x8} = 3' -f $connected),
        ('set {{unsigned int}}0x{0:x8} = 1' -f $enabled)
    )
    if (-not $Standing) {
        $commands += @(
            # Source status IDs: Wait=10, Squat=28, SquatWait=29.
            # A held Down from Wait selects SquatSetStatusNoPass, so this first
            # natural SquatWait is already the exact crouch state being tested;
            # do not require a particular Dream Land line ordering here.
            'break ndsBattlePlayableFrameCompleteMarker if $mario->status_id == 29',
            'continue',
            'printf "CROUCH_READY status=%d floor=%d root=%f,%f,%f anim=%f inputy=%d\n", $mario->status_id, $mario->coll_data.floor_line_id, ((DObj *)$mario_gobj->obj)->translate.vec.f.x, ((DObj *)$mario_gobj->obj)->translate.vec.f.y, ((DObj *)$mario_gobj->obj)->translate.vec.f.z, $mario->anim_frame, $mario->input.pl.stick_range.y',
            'delete breakpoints'
        )
    } else {
        $commands += @(
            'break ndsBattlePlayableFrameCompleteMarker if $mario->status_id == 10',
            'continue',
            'printf "STAND_READY status=%d root=%f,%f,%f anim=%f inputy=%d\n", $mario->status_id, ((DObj *)$mario_gobj->obj)->translate.vec.f.x, ((DObj *)$mario_gobj->obj)->translate.vec.f.y, ((DObj *)$mario_gobj->obj)->translate.vec.f.z, $mario->anim_frame, $mario->input.pl.stick_range.y',
            'delete breakpoints'
        )
    }
    if ($DumpHurtboxes) {
        # A GDB inferior call is NOT usable here. The first version of this dump
        # ran `call func_ovl2_800EDBA4($dc->joint)` per slot; on 2026-08-12 that
        # hung the target (core ended in libfat get_fat) and the whole run died
        # by timeout after printing only HURT_DUMP_READY. CLAUDE.OPUS.md already
        # forbids guest calls from this stub for the allocator; it holds for any
        # guest function. The probe build computes the same source matrices
        # in-guest instead (ndsPositionProbeCaptureMarioHurtboxes), so the dump
        # is now pure memory reads.
        #
        # One run captures BOTH poses: the hook latches the first Wait update
        # into slots 0..10 and the first SquatWait update into slots 11..21, so
        # standing and crouching geometry cannot be split across two ROMs, two
        # seeds, or two respawns.
        $nativeRequired = @(
            'gNdsMarioHurtProbeCaptureMask',
            'gNdsMarioHurtProbeJoint',
            'gNdsMarioHurtProbeCenterY',
            'gNdsMarioHurtProbeExtentY',
            'gNdsMarioHurtProbeJointY',
            'gNdsMarioHurtProbeRootY'
        )
        $nativeMissing = @($nativeRequired | Where-Object { $symbols -notcontains $_ })
        if ($nativeMissing.Count -gt 0) {
            throw ('-DumpHurtboxes needs a NDS_R2_POSITION_PROBE build; ' +
                   $Build + ' is missing: ' + ($nativeMissing -join ', '))
        }
        # Give the in-guest hook one more completed frame to latch the pose the
        # status breakpoint just reported, then read the mask before decoding.
        $commands += @(
            'tbreak ndsBattlePlayableFrameCompleteMarker',
            'continue',
            ('printf "HURT_DUMP_READY mode={0} status=%d mask=%#x floor=%d flags=%#x attr_size=%f root_scale=%f,%f,%f rootY_stand=%f rootY_crouch=%f\n", $mario->status_id, gNdsMarioHurtProbeCaptureMask, $mario->coll_data.floor_line_id, $mario->coll_data.floor_flags, $mario->attr->size, ((DObj *)$mario_gobj->obj)->scale.vec.f.x, ((DObj *)$mario_gobj->obj)->scale.vec.f.y, ((DObj *)$mario_gobj->obj)->scale.vec.f.z, gNdsMarioHurtProbeRootY[0], gNdsMarioHurtProbeRootY[1]' -f $modeValue)
        )
        foreach ($slotIndex in 0..10) {
            $crouchIndex = $slotIndex + 11
            $commands += @(
                # The source descriptor (joint id, offset, half-size, hitstatus)
                # is read live and reported once per slot; the per-pose rows
                # below carry only what the pose actually changes.
                ("set `$dc = &`$mario->damage_colls[$slotIndex]"),
                ('printf "HURT_DESC slot={0} joint=%d hit=%d off=%f,%f,%f half=%f,%f,%f\n", $dc->joint_id, $dc->hitstatus, $dc->offset.x, $dc->offset.y, $dc->offset.z, $dc->size.x, $dc->size.y, $dc->size.z' -f $slotIndex),
                ('printf "HURT_BOX mode=0 slot={0} joint=%d center_y=%f extent_y=%f lo=%f hi=%f joint_t_y=%f\n", gNdsMarioHurtProbeJoint[{0}], gNdsMarioHurtProbeCenterY[{0}], gNdsMarioHurtProbeExtentY[{0}], gNdsMarioHurtProbeCenterY[{0}]-gNdsMarioHurtProbeExtentY[{0}], gNdsMarioHurtProbeCenterY[{0}]+gNdsMarioHurtProbeExtentY[{0}], gNdsMarioHurtProbeJointY[{0}]' -f $slotIndex),
                ('printf "HURT_BOX mode=1 slot={0} joint=%d center_y=%f extent_y=%f lo=%f hi=%f joint_t_y=%f\n", gNdsMarioHurtProbeJoint[{1}], gNdsMarioHurtProbeCenterY[{1}], gNdsMarioHurtProbeExtentY[{1}], gNdsMarioHurtProbeCenterY[{1}]-gNdsMarioHurtProbeExtentY[{1}], gNdsMarioHurtProbeCenterY[{1}]+gNdsMarioHurtProbeExtentY[{1}], gNdsMarioHurtProbeJointY[{1}]' -f $slotIndex, $crouchIndex)
            )
        }
        $commands += @('detach', 'quit')
    } elseif ($TraceCrouchExit) {
        if ($Standing) {
            throw '-TraceCrouchExit cannot be combined with -Standing.'
        }
        $commands += @(
            # Hardware-watch only the live source status word after a confirmed
            # SquatWait. This answers why a held-down Mario later appears as
            # Wait without perturbing input or status ourselves.
            'watch $mario->status_id',
            'continue',
            ('printf "CROUCH_EXIT status=%d floor=%d flags=%#x fp_input_y=%d controller_y=%d desc_y=%d raw_y=%d pad_y=%d reads=%u publishes=%u suppressed=%u\n", $mario->status_id, $mario->coll_data.floor_line_id, $mario->coll_data.floor_flags, $mario->input.pl.stick_range.y, gSYControllerDevices[0].stick_range.y, sSYControllerDescs[0].unk0F, sSYControllerData[0].stick_y, *(signed char *)0x{0:x8}, gNdsControllerReadCount, gNdsControllerPublishCount, gNdsControllerPublishSuppressedCount' -f ($pads + 3)),
            'detach',
            'quit'
        )
    } else {
    $commands += @(
        # Mario can be KO'd/reborn while waiting for Fox's AI to choose Neutral
        # B. Re-resolve the CURRENT player-0 fighter at every source Blaster
        # maker entry and accept only a shot whose target is still in the state
        # under test. This keeps a stale pre-KO GObj from masquerading as a
        # standing Mario -- exactly what the first version of this probe did.
        'break battleship_wpFoxBlasterMakeWeapon',
        'commands',
        'silent',
        'set $mario_gobj = (GObj *)gSCManagerBattleState->players[0].fighter_gobj',
        'set $mario = (FTStruct *)$mario_gobj->user_data.p',
        $(if ($Standing) {
            'if $mario->status_id != 10'
        } else {
            'if $mario->status_id != 29'
        }),
        'continue',
        'end',
        'end',
        'continue',
        # Still AT the accepted maker entry, so the ARM ABI guarantees r0 is
        # Fox's fighter_gobj and r1 the spawn Vec3f that ftfoxspecialn.c:20-25
        # just filled. Read both before `finish` unwinds them. Raw float
        # indexing avoids depending on a Vec3f debug type being present.
        #
        # Source invariant (ftfoxspecialn.c:20-25 + gmCollisionGetFighterParts-
        # WorldPosition): spawn == joint17_translate + 60 * joint17_X_basis.
        # gmCollisionGetFighterPartsWorldPosition has just built that matrix, so
        # mtx_translate is the exact one the source spawn used.
        'set $fox_gobj = (GObj *)$r0',
        'set $fox_fp = (FTStruct *)$fox_gobj->user_data.p',
        'set $j17 = (FTParts *)$fox_fp->joints[17]->user_data.p',
        'set $spawn_x = ((float *)$r1)[0]',
        'set $spawn_y = ((float *)$r1)[1]',
        'set $spawn_z = ((float *)$r1)[2]',
        'set $expect_x = ($j17->mtx_translate[0][0] * 60) + $j17->mtx_translate[3][0]',
        'set $expect_y = ($j17->mtx_translate[0][1] * 60) + $j17->mtx_translate[3][1]',
        'set $expect_z = ($j17->mtx_translate[0][2] * 60) + $j17->mtx_translate[3][2]',
        'printf "FOX_MUZZLE spawn=%f,%f,%f expect=%f,%f,%f d=%f,%f,%f j17_t=%f,%f,%f fox_lr=%d fox_status=%d\n", $spawn_x, $spawn_y, $spawn_z, $expect_x, $expect_y, $expect_z, $spawn_x-$expect_x, $spawn_y-$expect_y, $spawn_z-$expect_z, $j17->mtx_translate[3][0], $j17->mtx_translate[3][1], $j17->mtx_translate[3][2], $fox_fp->lr, $fox_fp->status_id',
        'delete breakpoints',
        # `finish` is top-level here, not inside a breakpoint command list, so
        # r0 is the source weapon GObj returned by the accepted maker call.
        'finish',
        'set $fox_weapon = (GObj *)$r0',
        'set $fox_wp = (WPStruct *)$fox_weapon->user_data.p',
        'set $fox_attack = &$fox_wp->attack_coll',
        ('printf "FOX_SHOT_READY weapon=%p attack=%p size=%f mario_status=%d floor=%d flags=%#x inputy=%d pad_y=%d enabled=%u\n", $fox_weapon, $fox_attack, $fox_attack->size, $mario->status_id, $mario->coll_data.floor_line_id, $mario->coll_data.floor_flags, $mario->input.pl.stick_range.y, *(signed char *)0x{0:x8}, *(unsigned int *)0x{1:x8}' -f ($pads + 3), $enabled),
        $(if ($InspectShotOnly) {
            # attack_pos is free-list storage until the source updater consumes
            # the New-state offsets. Stop AFTER that updater, not at maker
            # return, or a stale previous weapon can look like the live line.
            'break wpProcessUpdateHitPositions if (GObj *)$r0 == $fox_weapon' + "`ncontinue`nfinish`n" +
            'printf "FOX_COLL_LINE root_y=%f curr_y=%f prev_y=%f radius=%f lower=%f offset0_y=%f offset1_y=%f lr=%d\n", ((DObj *)$fox_weapon->obj)->translate.vec.f.y, $fox_attack->attack_pos[0].pos_curr.y, $fox_attack->attack_pos[0].pos_prev.y, $fox_attack->size, $fox_attack->attack_pos[0].pos_curr.y-$fox_attack->size, $fox_attack->offsets[0].y, $fox_attack->offsets[1].y, $fox_wp->lr' + "`ndetach`nquit"
        } else { '' }),
        # The maker-entry r1 read is taken after GDB's prologue skip and so is
        # NOT trustworthy on its own. Re-read the weapon's OWN spawned position
        # here, through the returned GObj, as an independent witness of where
        # the shot actually is. If these disagree with FOX_MUZZLE's spawn, the
        # register read was the liar, not the game.
        'printf "FOX_SPAWNED root=%f,%f,%f attack0_curr=%f,%f,%f attack0_prev=%f,%f,%f\n", ((DObj *)$fox_weapon->obj)->translate.vec.f.x, ((DObj *)$fox_weapon->obj)->translate.vec.f.y, ((DObj *)$fox_weapon->obj)->translate.vec.f.z, $fox_attack->attack_pos[0].pos_curr.x, $fox_attack->attack_pos[0].pos_curr.y, $fox_attack->attack_pos[0].pos_curr.z, $fox_attack->attack_pos[0].pos_prev.x, $fox_attack->attack_pos[0].pos_prev.y, $fox_attack->attack_pos[0].pos_prev.z',
        # Disassembly of this exact ELF proves the result site: r0 is the
        # gmCollisionTestRectangle boolean, r7 is attack_coll, and r5 is
        # damage_coll. Stop only on TRUE for this shot against Mario's own
        # damage-coll array. This avoids all optimized debug-parameter aliases.
        # At the proven result site r5 has already been advanced by +20 to
        # &damage_coll->offset (see the exact disassembly at +0x5a), so subtract
        # that source-struct offset both in the condition and when decoding it.
        ('break *0x{0:x8} if ($r0 != 0) && ((WPAttackColl *)$r7 == $fox_attack) && ((FTDamageColl *)((char *)$r5 - 20) >= &$mario->damage_colls[0]) && ((FTDamageColl *)((char *)$r5 - 20) < &$mario->damage_colls[11])' -f $collisionResultAddress),
        'continue',
        'set $dc = (FTDamageColl *)((char *)$r5 - 20)',
        'set $parts = (FTParts *)$dc->joint->user_data.p',
        'set $slot = $dc - &$mario->damage_colls[0]',
        'set $attack_id = (int)$r9',
        'set $ay = $fox_attack->attack_pos[$attack_id].pos_curr.y',
        'printf "FOX_HIT_POS attack_id=%d curr=%f,%f,%f prev=%f,%f,%f mario_joint_x=%f\n", $attack_id, $fox_attack->attack_pos[$attack_id].pos_curr.x, $fox_attack->attack_pos[$attack_id].pos_curr.y, $fox_attack->attack_pos[$attack_id].pos_curr.z, $fox_attack->attack_pos[$attack_id].pos_prev.x, $fox_attack->attack_pos[$attack_id].pos_prev.y, $fox_attack->attack_pos[$attack_id].pos_prev.z, $parts->mtx_translate[3][0]',
        'set $cy = (($parts->mtx_translate[0][1] * $dc->offset.x) + ($parts->mtx_translate[1][1] * $dc->offset.y) + ($parts->mtx_translate[2][1] * $dc->offset.z)) + $parts->mtx_translate[3][1]',
        'set $b0 = $parts->mtx_translate[0][1]',
        'set $b1 = $parts->mtx_translate[1][1]',
        'set $b2 = $parts->mtx_translate[2][1]',
        'if $b0 < 0',
        'set $b0 = -$b0',
        'end',
        'if $b1 < 0',
        'set $b1 = -$b1',
        'end',
        'if $b2 < 0',
        'set $b2 = -$b2',
        'end',
        'set $ey = ($b0 * $dc->size.x) + ($b1 * $dc->size.y) + ($b2 * $dc->size.z)',
        ('printf "FOX_CROUCH_HIT mode=%d status=%d anim=%f slot=%d joint=%d laser_y=%f laser_prev_y=%f radius=%f center_y=%f extent_y=%f hurt_lo=%f hurt_hi=%f off=%f,%f,%f size=%f,%f,%f scale=%f,%f,%f root=%f,%f,%f\n", ' +
            $modeValue + ', $mario->status_id, $mario->anim_frame, $slot, $dc->joint_id, $ay, $fox_attack->attack_pos[$attack_id].pos_prev.y, $fox_attack->size, $cy, $ey, $cy-$ey, $cy+$ey, $dc->offset.x, $dc->offset.y, $dc->offset.z, $dc->size.x, $dc->size.y, $dc->size.z, $parts->vec_scale.x, $parts->vec_scale.y, $parts->vec_scale.z, ((DObj *)$mario_gobj->obj)->translate.vec.f.x, ((DObj *)$mario_gobj->obj)->translate.vec.f.y, ((DObj *)$mario_gobj->obj)->translate.vec.f.z'),
        'printf "HURT_MTX=%f,%f,%f,%f;%f,%f,%f,%f;%f,%f,%f,%f;%f,%f,%f,%f\n", $parts->mtx_translate[0][0],$parts->mtx_translate[0][1],$parts->mtx_translate[0][2],$parts->mtx_translate[0][3],$parts->mtx_translate[1][0],$parts->mtx_translate[1][1],$parts->mtx_translate[1][2],$parts->mtx_translate[1][3],$parts->mtx_translate[2][0],$parts->mtx_translate[2][1],$parts->mtx_translate[2][2],$parts->mtx_translate[2][3],$parts->mtx_translate[3][0],$parts->mtx_translate[3][1],$parts->mtx_translate[3][2],$parts->mtx_translate[3][3]',
        'detach',
        'quit'
    )
    }

    $capture = Invoke-GdbMarkerScript -Gdb $gdb -Elf $elf -Root $root `
        -Commands $commands -ScriptName 'fox_crouch_collision_probe.gdb' `
        -TimeoutSeconds $TimeoutSeconds
    [System.IO.Directory]::CreateDirectory((Split-Path -Parent $artifact)) | Out-Null
    [System.IO.File]::WriteAllText($artifact, $capture.Stdout, [System.Text.Encoding]::UTF8)
    $markers = @($capture.Stdout -split "`r?`n" | Where-Object {
        $_ -match '^(CROUCH_READY|CROUCH_EXIT|STAND_READY|HURT_DUMP_READY|HURT_DESC|HURT_BOX|FOX_SHOT_READY|FOX_SPAWNED|FOX_COLL_LINE|FOX_MUZZLE|FOX_HIT_POS|FOX_CROUCH_HIT|HURT_MTX)'
    })
    $markers | ForEach-Object { Write-Output $_ }
    if ($DumpHurtboxes) {
        if (-not ($markers -match '^HURT_BOX')) {
            throw 'Fox crouch hurtbox dump completed without any active Mario hurtboxes.'
        }
        # An unlatched pose reads as an all-zero row, which looks exactly like a
        # collapsed hurtbox. Demand the in-guest capture mask instead: bit 0 is
        # the Wait pose, bit 1 the SquatWait pose.
        $ready = @($markers | Where-Object { $_ -match '^HURT_DUMP_READY' })[0]
        $mask = 0
        if ($ready -match 'mask=0x([0-9a-fA-F]+)') {
            $mask = [Convert]::ToInt32($matches[1], 16)
        }
        $needed = if ($Standing) { 1 } else { 3 }
        if (($mask -band $needed) -ne $needed) {
            throw ("Mario pose capture incomplete: mask=0x{0:x} needs 0x{1:x} " -f $mask, $needed) +
                  '(bit0=Wait, bit1=SquatWait). The rows below are unlatched, not measured.'
        }
    }
    if ((-not $DumpHurtboxes) -and $TraceCrouchExit -and -not ($markers -match '^CROUCH_EXIT')) {
        throw 'Fox crouch trace completed without observing Mario leave SquatWait.'
    }
    if ($InspectShotOnly -and -not ($markers -match '^FOX_COLL_LINE')) {
        throw 'Fox crouch shot inspection completed without a live collision-line marker.'
    }
    if ((-not $DumpHurtboxes) -and (-not $TraceCrouchExit) -and
        (-not $InspectShotOnly) -and -not ($markers -match '^FOX_CROUCH_HIT')) {
        throw 'Fox crouch probe completed without a natural Blaster damage hit.'
    }
    Write-Output "probe capture: $artifact"
} finally {
    if ($null -ne $emulator) {
        $emulator.Refresh()
        if (-not $emulator.HasExited) {
            Stop-Process -Id $emulator.Id -Force -ErrorAction SilentlyContinue
            $emulator.WaitForExit()
        }
    }
    if ($null -ne $configState) {
        Restore-MelonDSGdbConfig -State $configState
    }
}
