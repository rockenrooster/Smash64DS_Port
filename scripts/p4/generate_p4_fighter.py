#!/usr/bin/env python3
"""P4 source adapter, stage 2: lower one exported Remix fighter into DS build inputs.

Reads remix_export.py's resolved.json/events.json and the staged reference ROM
and writes, under --out (a build directory, never a tracked one, because every
output is derived from the user's ROM):

  o2r/<id>                 Torch SSB64:RELOC containers the DS reloc loader reads:
                           the fighter's donor-only files, plus a synthesized
                           main-motion file = the parent's motion file followed
                           by every reachable Remix-inserted stream and its data,
                           with each Remix absolute pointer turned into an
                           internal relocation, so ftMainSetStatus's ordinary
                           `file head + offset` path runs the donor scripts.
  nds_p4_<name>.generated.c  FTData, motion and menu-motion descriptors, the
                           status-callback overrides and kind-table rows.
  manifest.json            identities, hashes and every classified deviation.

Remix callbacks must map to a hand-ported DS function in CALLBACK_PORTS; vanilla
callbacks map to their decomp names. Anything else fails the generation.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import remix_rom as R  # noqa: E402

NO_SCRIPT = 0x80000000
# DS-synthesized files live above the donor's largest ID (Remix: 5455 files).
SYNTH_FILE_BASE = 0x1600
# ftCommonAppearSetStatus (0x8013DBE0): the address past its fkind switch, the
# entry_script value Remix uses for "no entry effect" (Ganondorf).
ENTRY_SCRIPT_NONE = 0x8013DD68

# Hand-ported native callbacks for donor routines (src/port/nds_p4_*.c).
CALLBACK_PORTS = {
    "Phantasm.ground_subroutine_": "ndsP4FalcoPhantasmGroundInterrupt",
    "Phantasm.air_subroutine_": "ndsP4FalcoPhantasmAirInterrupt",
    "Phantasm.air_physics_": "ndsP4FalcoPhantasmAirPhysics",
    "Phantasm.air_collision_": "ndsP4FalcoPhantasmAirMap",
    # src/port/nds_p4_wolf.c
    "WolfUSP.initial_ground": "ndsP4WolfUSPInitialGround",
    "WolfUSP.initial_air": "ndsP4WolfUSPInitialAir",
    "WolfUSP.main_ground": "ndsP4WolfUSPMainGround",
    "WolfUSP.main_air": "ndsP4WolfUSPMainAir",
    "WolfUSP.change_direction_": "ndsP4WolfUSPChangeDirection",
    "WolfUSP.physics_": "ndsP4WolfUSPPhysics",
    "WolfUSP.collision_": "ndsP4WolfUSPMap",
    "WolfUSP.main_2": "ndsP4WolfUSPMain2",
    "WolfDSP.physics_": "ndsP4WolfDSPPhysics",
    "WolfNSP.air_collision_": "ndsP4WolfNSPAirMap",
    "WolfNSP.main": "ndsP4WolfNSPMain",
    # src/port/nds_p4_bowser.c
    "BowserUSP.air_initial_": "ndsP4BowserUSPAirInitial",
    "BowserUSP.ground_physics_": "ndsP4BowserUSPGroundPhysics",
    "BowserUSP.air_physics_": "ndsP4BowserUSPAirPhysics",
    "BowserDSP.air_physics_": "ndsP4BowserDSPAirPhysics",
    "BowserNSP.ground_initial_": "ndsP4BowserNSPGroundInitial",
    "BowserNSP.air_initial_": "ndsP4BowserNSPAirInitial",
    "BowserNSP.main_": "ndsP4BowserNSPMain",
    "BowserNSP.air_collision_": "ndsP4BowserNSPAirMap",
    "BowserFThrow.main_": "ndsP4BowserFThrowMain",
    "BowserFThrow.collision_": "ndsP4BowserFThrowMap",
    # src/port/nds_p4_marth.c (MarthUSP and MarthNSP are Roy's too)
    "MarthUSP.air_initial_": "ndsP4MarthUSPAirInitial",
    "MarthUSP.ground_initial_": "ndsP4MarthUSPGroundInitial",
    "MarthUSP.main_": "ndsP4MarthUSPMain",
    "MarthUSP.change_direction_": "ndsP4MarthUSPChangeDirection",
    "MarthUSP.physics_": "ndsP4MarthUSPPhysics",
    "MarthUSP.collision_": "ndsP4MarthUSPMap",
    "MarthNSP.ground_1_initial_": "ndsP4MarthNSPGround1Initial",
    "MarthNSP.air_1_initial_": "ndsP4MarthNSPAir1Initial",
    "MarthNSP.ground_main_": "ndsP4MarthNSPGroundMain",
    "MarthNSP.air_main_": "ndsP4MarthNSPAirMain",
    "MarthNSP.ground_collision_": "ndsP4MarthNSPGroundMap",
    "MarthNSP.air_collision_": "ndsP4MarthNSPAirMap",
    "MarthDSP.ground_initial_": "ndsP4MarthDSPGroundInitial",
    "MarthDSP.air_initial_": "ndsP4MarthDSPAirInitial",
    "MarthDSP.main_": "ndsP4MarthDSPMain",
    "MarthDSP.air_physics_": "ndsP4MarthDSPAirPhysics",
    "MarthDSP.ground_collision_": "ndsP4MarthDSPGroundMap",
    "MarthDSP.air_collision_": "ndsP4MarthDSPAirMap",
    # src/port/nds_p4_roy.c
    "RoyDSP.ground_begin_initial_": "ndsP4RoyDSPGroundBeginInitial",
    "RoyDSP.air_begin_initial_": "ndsP4RoyDSPAirBeginInitial",
    "RoyDSP.ground_begin_main_": "ndsP4RoyDSPGroundBeginMain",
    "RoyDSP.air_begin_main_": "ndsP4RoyDSPAirBeginMain",
    "RoyDSP.ground_wait_main_": "ndsP4RoyDSPGroundWaitMain",
    "RoyDSP.air_wait_main_": "ndsP4RoyDSPAirWaitMain",
    "RoyDSP.end_main_": "ndsP4RoyDSPEndMain",
    "RoyDSP.ground_collision_": "ndsP4RoyDSPGroundMap",
    "RoyDSP.air_collision_": "ndsP4RoyDSPAirMap",
    # src/port/nds_p4_wario.c
    "WarioNSP.ground_initial_": "ndsP4WarioNSPGroundInitial",
    "WarioNSP.air_initial_": "ndsP4WarioNSPAirInitial",
    "WarioNSP.ground_move_": "ndsP4WarioNSPGroundMove",
    "WarioNSP.ground_physics_": "ndsP4WarioNSPGroundPhysics",
    "WarioNSP.ground_collision_": "ndsP4WarioNSPGroundMap",
    "WarioNSP.air_move_": "ndsP4WarioNSPAirMove",
    "WarioNSP.air_physics_": "ndsP4WarioNSPAirPhysics",
    "WarioNSP.air_collision_": "ndsP4WarioNSPAirMap",
    "WarioNSP.recoil_move_": "ndsP4WarioNSPRecoilMove",
    "WarioNSP.recoil_physics_": "ndsP4WarioNSPRecoilPhysics",
    "WarioNSP.recoil_ground_collision_": "ndsP4WarioNSPRecoilGroundMap",
    "WarioNSP.recoil_air_collision_": "ndsP4WarioNSPRecoilAirMap",
    "WarioUSP.initial_": "ndsP4WarioUSPInitial",
    "WarioUSP.main_": "ndsP4WarioUSPMain",
    "WarioUSP.change_direction_": "ndsP4WarioUSPChangeDirection",
    "WarioUSP.physics_": "ndsP4WarioUSPPhysics",
    "WarioUSP.collision_": "ndsP4WarioUSPMap",
    "WarioDSP.ground_initial_": "ndsP4WarioDSPGroundInitial",
    "WarioDSP.air_initial_": "ndsP4WarioDSPAirInitial",
    "WarioDSP.ground_move_": "ndsP4WarioDSPGroundMove",
    "WarioDSP.air_move_": "ndsP4WarioDSPAirMove",
    "WarioDSP.physics_": "ndsP4WarioDSPPhysics",
    "WarioDSP.collision_": "ndsP4WarioDSPMap",
    # src/port/nds_p4_peach.c (PeachDSP, the turnip pull, waits for the
    # turnip item: S6)
    "PeachFloat.main_": "ndsP4PeachFloatMain",
    "PeachFloat.interrupt_": "ndsP4PeachFloatInterrupt",
    "PeachNSP.ground_initial_": "ndsP4PeachNSPGroundInitial",
    "PeachNSP.air_initial_": "ndsP4PeachNSPAirInitial",
    "PeachNSP.ground_physics_": "ndsP4PeachNSPGroundPhysics",
    "PeachNSP.air_physics_": "ndsP4PeachNSPAirPhysics",
    "PeachNSP.ground_collision_": "ndsP4PeachNSPGroundMap",
    "PeachNSP.air_collision_": "ndsP4PeachNSPAirMap",
    "PeachUSP.air_initial_": "ndsP4PeachUSPAirInitial",
    "PeachUSP.ground_initial_": "ndsP4PeachUSPGroundInitial",
    "PeachUSP.main_": "ndsP4PeachUSPMain",
    "PeachUSP.open_main_": "ndsP4PeachUSPOpenMain",
    "PeachUSP.close_main_": "ndsP4PeachUSPCloseMain",
    "PeachUSP.change_direction_": "ndsP4PeachUSPChangeDirection",
    "PeachUSP.float_interrupt_": "ndsP4PeachUSPFloatInterrupt",
    "PeachUSP.fall_interrupt_": "ndsP4PeachUSPFallInterrupt",
    "PeachUSP.physics_": "ndsP4PeachUSPPhysics",
    "PeachUSP.float_physics_": "ndsP4PeachUSPFloatPhysics",
    "PeachUSP.collision_": "ndsP4PeachUSPMap",
    # src/port/nds_p4_crash.c
    "CrashNSP.ground_initial_": "ndsP4CrashNSPGroundInitial",
    "CrashNSP.air_initial_": "ndsP4CrashNSPAirInitial",
    "CrashNSP.ground_main_": "ndsP4CrashNSPGroundMain",
    "CrashNSP.air_main_": "ndsP4CrashNSPAirMain",
    "CrashNSP.blocked_main_": "ndsP4CrashNSPBlockedMain",
    "CrashNSP.ground_physics_": "ndsP4CrashNSPGroundPhysics",
    "CrashNSP.air_physics_": "ndsP4CrashNSPAirPhysics",
    "CrashNSP.blocked_air_physics_": "ndsP4CrashNSPBlockedAirPhysics",
    "CrashNSP.ground_collision_": "ndsP4CrashNSPGroundMap",
    "CrashNSP.air_collision_": "ndsP4CrashNSPAirMap",
    "CrashNSP.blocked_ground_collision_": "ndsP4CrashNSPBlockedGroundMap",
    "CrashNSP.blocked_air_collision_": "ndsP4CrashNSPBlockedAirMap",
    "CrashUSP.ground_initial_": "ndsP4CrashUSPGroundInitial",
    "CrashUSP.air_initial_": "ndsP4CrashUSPAirInitial",
    "CrashUSP.main_": "ndsP4CrashUSPMain",
    "CrashUSP.change_direction_": "ndsP4CrashUSPChangeDirection",
    "CrashUSP.physics_": "ndsP4CrashUSPPhysics",
    "CrashUSP.collision_": "ndsP4CrashUSPMap",
    "CrashDSP.initial_": "ndsP4CrashDSPInitial",
    "CrashDSP.dive_air_initial_": "ndsP4CrashDSPDiveAirInitial",
    "CrashDSP.begin_main_": "ndsP4CrashDSPBeginMain",
    "CrashDSP.wait_main_": "ndsP4CrashDSPWaitMain",
    "CrashDSP.turn_main_": "ndsP4CrashDSPTurnMain",
    "CrashDSP.dive_main_": "ndsP4CrashDSPDiveMain",
    "CrashDSP.physics_": "ndsP4CrashDSPPhysics",
    "CrashDSP.dive_physics_": "ndsP4CrashDSPDivePhysics",
    "CrashDSP.dive_air_physics_": "ndsP4CrashDSPDiveAirPhysics",
    "CrashDSP.collision_": "ndsP4CrashDSPMap",
    "CrashDSP.dive_collision_": "ndsP4CrashDSPDiveMap",
    # src/port/nds_p4_lanky.c
    "LankyNSP.ground_initial_": "ndsP4LankyNSPGroundInitial",
    "LankyNSP.air_initial_": "ndsP4LankyNSPAirInitial",
    "LankyNSP.main_": "ndsP4LankyNSPMain",
    "LankyUSP.ground_initial_": "ndsP4LankyUSPGroundInitial",
    "LankyUSP.air_initial_": "ndsP4LankyUSPAirInitial",
    "LankyUSP.begin_main_": "ndsP4LankyUSPBeginMain",
    "LankyUSP.main_": "ndsP4LankyUSPMain",
    "LankyUSP.turn_main_": "ndsP4LankyUSPTurnMain",
    "LankyUSP.end_main_": "ndsP4LankyUSPEndMain",
    "LankyUSP.physics_": "ndsP4LankyUSPPhysics",
    "LankyUSP.damage_physics_": "ndsP4LankyUSPDamagePhysics",
    "LankyUSP.begin_ground_collision_": "ndsP4LankyUSPBeginGroundMap",
    "LankyUSP.begin_air_collision_": "ndsP4LankyUSPBeginAirMap",
    "LankyUSP.collision_": "ndsP4LankyUSPMap",
    "LankyDSP.ground_initial_": "ndsP4LankyDSPGroundInitial",
    "LankyDSP.air_initial_": "ndsP4LankyDSPAirInitial",
    "LankyDSP.begin_main_": "ndsP4LankyDSPBeginMain",
    "LankyDSP.ground_wait_main_": "ndsP4LankyDSPGroundWaitMain",
    "LankyDSP.air_wait_main_": "ndsP4LankyDSPAirWaitMain",
    "LankyDSP.move_main_": "ndsP4LankyDSPMoveMain",
    "LankyDSP.turn_main_": "ndsP4LankyDSPTurnMain",
    "LankyDSP.jumpsquat_main_": "ndsP4LankyDSPJumpSquatMain",
    "LankyDSP.jumpsquat_interrupt_": "ndsP4LankyDSPJumpSquatInterrupt",
    "LankyDSP.jump_main_": "ndsP4LankyDSPJumpMain",
    "LankyDSP.landing_main_": "ndsP4LankyDSPLandingMain",
    "LankyDSP.cancel_main_": "ndsP4LankyDSPCancelMain",
    "LankyDSP.ground_physics_": "ndsP4LankyDSPGroundPhysics",
    "LankyDSP.ground_collision_": "ndsP4LankyDSPGroundMap",
    "LankyDSP.air_collision_": "ndsP4LankyDSPAirMap",
    # src/port/nds_p4_sheik.c
    "SheikUSP.ground_begin_initial_": "ndsP4SheikUSPGroundInitial",
    "SheikUSP.air_begin_initial_": "ndsP4SheikUSPAirInitial",
    "SheikUSP.begin_main_": "ndsP4SheikUSPBeginMain",
    "SheikUSP.ground_begin_collision_": "ndsP4SheikUSPBeginGroundMap",
    "SheikUSP.air_begin_collision_": "ndsP4SheikUSPBeginAirMap",
    "SheikUSP.move_main_": "ndsP4SheikUSPMoveMain",
    "SheikUSP.move_physics_": "ndsP4SheikUSPMovePhysics",
    "SheikUSP.ground_move_collision_": "ndsP4SheikUSPMoveGroundMap",
    "SheikUSP.air_move_collision_": "ndsP4SheikUSPMoveAirMap",
    "SheikUSP.ground_end_main_": "ndsP4SheikUSPGroundEndMain",
    "SheikUSP.air_end_main_": "ndsP4SheikUSPAirEndMain",
    "SheikUSP.end_collision_": "ndsP4SheikUSPEndMap",
    "SheikUSP.end_physics_": "ndsP4SheikUSPEndPhysics",
    "SheikNSP.ground_begin_initial_": "ndsP4SheikNSPGroundInitial",
    "SheikNSP.air_begin_initial_": "ndsP4SheikNSPAirInitial",
    "SheikNSP.begin_main_": "ndsP4SheikNSPBeginMain",
    "SheikNSP.ground_begin_interrupt_": "ndsP4SheikNSPBeginGroundInterrupt",
    "SheikNSP.air_begin_interrupt_": "ndsP4SheikNSPBeginAirInterrupt",
    "SheikNSP.ground_begin_collision_": "ndsP4SheikNSPBeginGroundMap",
    "SheikNSP.air_begin_collision_": "ndsP4SheikNSPBeginAirMap",
    "SheikNSP.charge_main_": "ndsP4SheikNSPChargeMain",
    "SheikNSP.ground_charge_interrupt_": "ndsP4SheikNSPChargeGroundInterrupt",
    "SheikNSP.air_charge_interrupt_": "ndsP4SheikNSPChargeAirInterrupt",
    "SheikNSP.ground_charge_collision_": "ndsP4SheikNSPChargeGroundMap",
    "SheikNSP.air_charge_collision_": "ndsP4SheikNSPChargeAirMap",
    "SheikNSP.shoot_main_": "ndsP4SheikNSPShootMain",
    "SheikNSP.ground_shoot_collision_": "ndsP4SheikNSPShootGroundMap",
    "SheikDSP.initial_": "ndsP4SheikDSPInitial",
    "SheikDSP.main_": "ndsP4SheikDSPMain",
    "SheikDSP.physics_": "ndsP4SheikDSPPhysics",
    "SheikDSP.air_collision_": "ndsP4SheikDSPAirMap",
    "SheikDSP.attack_collision_": "ndsP4SheikDSPAttackMap",
    "SheikDSP.recoil_main_": "ndsP4SheikDSPRecoilMain",
    "SheikDSP.recoil_physics_": "ndsP4SheikDSPRecoilPhysics",
    # src/port/nds_p4_banjo.c
    "BanjoNSP.ground_begin_initial_": "ndsP4BanjoNSPGroundInitial",
    "BanjoNSP.air_begin_initial_": "ndsP4BanjoNSPAirInitial",
    "BanjoNSP.begin_main_": "ndsP4BanjoNSPBeginMain",
    "BanjoNSP.ground_begin_collision_": "ndsP4BanjoNSPBeginGroundMap",
    "BanjoNSP.air_begin_collision_": "ndsP4BanjoNSPBeginAirMap",
    "BanjoNSP.shoot_forward_main_": "ndsP4BanjoNSPShootForwardMain",
    "BanjoNSP.shoot_backward_main_": "ndsP4BanjoNSPShootBackwardMain",
    "BanjoNSP.air_shoot_physics_": "ndsP4BanjoNSPAirShootPhysics",
    "BanjoNSP.ground_shoot_forward_collision_": "ndsP4BanjoNSPShootForwardGroundMap",
    "BanjoNSP.ground_shoot_backward_collision_": "ndsP4BanjoNSPShootBackwardGroundMap",
    "BanjoNSP.air_shoot_forward_collision_": "ndsP4BanjoNSPShootForwardAirMap",
    "BanjoNSP.air_shoot_backward_collision_": "ndsP4BanjoNSPShootBackwardAirMap",
    "BanjoUSP.initial_": "ndsP4BanjoUSPInitial",
    "BanjoUSP.begin_main_": "ndsP4BanjoUSPBeginMain",
    "BanjoUSP.begin_physics_": "ndsP4BanjoUSPBeginPhysics",
    "BanjoUSP.begin_collision_": "ndsP4BanjoUSPBeginMap",
    "BanjoUSP.attack_main_": "ndsP4BanjoUSPAttackMain",
    "BanjoUSP.attack_interupt_": "ndsP4BanjoUSPAttackInterrupt",
    "BanjoUSP.attack_physics_": "ndsP4BanjoUSPAttackPhysics",
    "BanjoUSP.attack_collision_": "ndsP4BanjoUSPAttackMap",
    "BanjoUSP.attack_end_main_": "ndsP4BanjoUSPAttackEndMain",
    "BanjoUSP.collision_": "ndsP4BanjoUSPMap",
    "BanjoUSP.recoil_move_": "ndsP4BanjoUSPRecoilMove",
    "BanjoUSP.recoil_physics_": "ndsP4BanjoUSPRecoilPhysics",
    "BanjoUSP.wall_splat_main_": "ndsP4BanjoUSPWallSplatMain",
    "BanjoUSP.splat_physics_": "ndsP4BanjoUSPSplatPhysics",
    "BanjoDSP.ground_initial_": "ndsP4BanjoDSPGroundInitial",
    "BanjoDSP.air_initial_": "ndsP4BanjoDSPAirInitial",
    "BanjoDSP.aerial_main_": "ndsP4BanjoDSPAerialMain",
    "BanjoDSP.air_move_": "ndsP4BanjoDSPAirMove",
    "BanjoDSP.physics_": "ndsP4BanjoDSPPhysics",
    "BanjoDSP.collision_": "ndsP4BanjoDSPMap",
    "BanjoDSP.grounded_collision_": "ndsP4BanjoDSPGroundMap",
    # src/port/nds_p4_sonic.c
    "SonicNSP.begin_initial_": "ndsP4SonicNSPInitial",
    "SonicNSP.begin_main_": "ndsP4SonicNSPBeginMain",
    "SonicNSP.move_main_": "ndsP4SonicNSPMoveMain",
    "SonicNSP.move_physics_": "ndsP4SonicNSPMovePhysics",
    "SonicNSP.move_collision_": "ndsP4SonicNSPMoveMap",
    "SonicNSP.ground_end_collision_": "ndsP4SonicNSPEndGroundMap",
    "SonicNSP.air_end_collision_": "ndsP4SonicNSPEndAirMap",
    "SonicNSP.ground_recoil_collision_": "ndsP4SonicNSPRecoilGroundMap",
    "SonicNSP.air_recoil_collision_": "ndsP4SonicNSPRecoilAirMap",
    "SonicUSP.ground_initial_": "ndsP4SonicUSPGroundInitial",
    "SonicUSP.air_initial_": "ndsP4SonicUSPAirInitial",
    "SonicUSP.main_air_": "ndsP4SonicUSPMainAir",
    "SonicUSP.interrupt_": "ndsP4SonicUSPInterrupt",
    "SonicUSP.air_physics_": "ndsP4SonicUSPAirPhysics",
    "SonicDSP.ground_charge_initial_": "ndsP4SonicDSPGroundChargeInitial",
    "SonicDSP.air_charge_initial_": "ndsP4SonicDSPAirChargeInitial",
    "SonicDSP.ground_charge_main_": "ndsP4SonicDSPGroundChargeMain",
    "SonicDSP.air_charge_main_": "ndsP4SonicDSPAirChargeMain",
    "SonicDSP.ground_charge_collision_": "ndsP4SonicDSPChargeGroundMap",
    "SonicDSP.air_charge_collision_": "ndsP4SonicDSPChargeAirMap",
    "SonicDSP.ground_move_main_": "ndsP4SonicDSPGroundMoveMain",
    "SonicDSP.ground_move_physics_": "ndsP4SonicDSPGroundMovePhysics",
    "SonicDSP.ground_move_collision_": "ndsP4SonicDSPMoveGroundMap",
    "SonicDSP.air_move_main_": "ndsP4SonicDSPAirMoveMain",
    "SonicDSP.air_move_interrupt_": "ndsP4SonicDSPAirMoveInterrupt",
    "SonicDSP.air_movement_physics_": "ndsP4SonicDSPAirMovementPhysics",
    "SonicDSP.air_move_collision_": "ndsP4SonicDSPMoveAirMap",
    "SonicDSP.ground_end_collision_": "ndsP4SonicDSPEndGroundMap",
    "SonicDSP.air_end_collision_": "ndsP4SonicDSPEndAirMap",
}

# P4: entry_script cases of another kind a content's port carries
# (NDS_P4_ENTRY_PORT): Bowser's Clown Copter in the Falcon Flyer's case.
ENTRY_PORTS = {
    "bowser": 0x8013DD14,
}

# P4 S6: special-file slots (1-4) a content loads into its own storage,
# g<Ident>Special<n>, instead of the parent's globals. Remix gives such a
# fighter its own file pointers and hooks the parent's makers on its id;
# a slot is listed once every one of those readers is ported for it
# (src/port/nds_p4_<name>.c), so it never stands in for the parent's file.
OWN_SPECIAL_FILES = {
    # WolfNSP's shot (1, its graphic 4), the reflector (2), the Wolfen (3)
    # and the Fire Wolf slash (4).
    "wolf": (1, 2, 3, 4),
    # The Clown Copter (2), read only by Yoshi's entry egg, which his
    # Falcon Flyer case never runs.
    "bowser": (2,),
}

# Vanilla file IDs that Remix rewrote with equivalent bytes, so the DS keeps
# the vanilla file the parent already ships (manifest "deviations").
EQUIVALENT_FILES = {
    # Remix rewrote FoxSpecial3 for every Arwing user: three external
    # references to ExternDataBank109+0x19F8 became one internal copy appended
    # at 0x2F80. Same bytes, no new dependency.
    0xA1: "remix copies ExternDataBank109+0x19F8 inline; vanilla bytes equivalent",
}

# Remix define_character parents -> the decomp's fighter names.
PARENT_DECOMP = {
    "MARIO": "Mario", "FOX": "Fox", "DONKEY": "Donkey", "SAMUS": "Samus",
    "LUIGI": "Luigi", "LINK": "Link", "YOSHI": "Yoshi", "CAPTAIN": "Captain",
    "KIRBY": "Kirby", "PIKACHU": "Pikachu", "JIGGLY": "Purin", "NESS": "Ness",
}
DECOMP_FTDATA = Path(__file__).resolve().parents[2] / "decomp" / "BattleShip-main" / "decomp" / "src" / "ft" / "ftdata.c"
DECOMP_SYMBOLS = Path(__file__).resolve().parents[2] / "decomp" / "BattleShip-main" / "decomp" / "symbols" / "symbols_us.txt"

# The DS loader's per-file extern id table (reloc_backend_assets.c
# NDS_RELOC_EXTERN_FILE_ID_CAPACITY in P4 builds): Banjo's main has 208.
MAX_EXTERN_IDS = 208

# Remix's special-move starter tables, in NDS_P4_SPECIAL_* order (nds_p4.h).
SPECIAL_START_TABLES = ("ground_nsp", "air_nsp", "ground_usp", "air_usp", "ground_dsp", "air_dsp")

# The source's jab switches as Remix's tables hold them (Character.asm
# move_jab_3_table, move_rapid_jab_table; nds_p4.h NDSP4Jab): a row is the
# address of a vanilla kind's case, the switch's end (DISABLED, the row of a
# kind with no case), or a Remix routine. The C value of each kind's case is
# the source's (ftcommonattack1.c, ftcommonattack100.c).
JAB_KIND_IDS = {"Mario": 0, "Fox": 1, "Luigi": 4, "Link": 5, "Captain": 7,
                "Kirby": 8, "Purin": 10, "Ness": 11}
JAB3_ACTIONS = {"Mario": "nFTMarioStatusAttack13", "Luigi": "nFTMarioStatusAttack13",
                "Captain": "nFTCaptainStatusAttack13", "Link": "nFTLinkStatusAttack13",
                "Ness": "nFTNessStatusAttack13"}
RAPID_COUNTS = {"Fox": (4, "nFTCommonStatusAttack12"), "Link": (5, "nFTCommonStatusAttack12"),
                "Kirby": (4, "nFTCommonStatusAttack12"), "Purin": (4, "nFTCommonStatusAttack12"),
                "Captain": (6, "nFTCaptainStatusAttack13")}
RAPID_KINDS = ("Fox", "Link", "Kirby", "Purin", "Captain")
JAB3_TIMER_END = 0x8014EBA4   # ftCommonAttack12SetStatus past its switch
JAB3_ACTION_END = 0x8014EC30  # ftCommonAttack13SetStatus past its switch


# The three Remix reflect AI hooks that branch Fox's way for its reflector
# characters (Reflect.asm AI scope); each routine compares the character id
# against a chain of `lli at, id; beq at, v0, <label>`.
REFLECT_FOX_HOOKS = ("Reflect.AI.extend_projectile_reflect_initial_",
                     "Reflect.AI.maintain_reflect_input_",
                     "Reflect.AI.apply_reflect_input_")


def reflect_fox_ids(rom: R.Rom, sym: dict, hook: str) -> set[int]:
    """Character ids `hook` sends to its _fox_reflect label: the immediate of
    the `ori at, zero, id` before each `beq at, v0` / `beq v0, at` there."""
    start, normal, fox = (sym[hook], sym[f"{hook}._normal"], sym[f"{hook}._fox_reflect"])
    ids: set[int] = set()
    at_value = None
    for pc in range(start, normal, 4):
        w = rom.u32_ram(pc)
        op, rs, rt = w >> 26, (w >> 21) & 31, (w >> 16) & 31
        if op == 0x0D and rs == 0 and rt == 1:  # ori at, zero, imm (lli)
            at_value = w & 0xFFFF
        elif op == 0x04 and {rs, rt} == {1, 2}:  # beq at, v0
            target = pc + 4 + (((w & 0xFFFF) ^ 0x8000) - 0x8000) * 4
            if target == fox and at_value is not None:
                ids.add(at_value)
    if not ids:
        raise GenError(f"{hook}: no Fox-branch character ids decoded")
    return ids


# mpCommonSetFighterLandingParams' fkind switch, which Remix turned into the
# grounded_script jump table: the kinds with a case of their own (Fox's row is
# the default, no case).
GROUNDED_CASE_KINDS = {"Mario": 0, "Samus": 3, "Luigi": 4, "Captain": 7, "Purin": 10}

# Remix grounded_script routines a content's port runs instead
# (NDSP4Overrides.on_landing): each ends the switch as the default does, so
# the content takes the default case beside its hook.
GROUNDED_PORTS = {"Peach.grounded_script_"}


def kind_cases(rom: R.Rom, sym: dict, remix_id: int, tables: dict,
               lab_fallback: bool) -> tuple[str, list[dict]]:
    """The content's NDSP4KindCases initializer: the source kind whose case
    of the landing switch Remix's grounded_script row runs (the default is
    NDS_P4_FOREIGN_FKIND), and its pipe_turn byte (Mario's turn in the Dokan
    statuses, marioshared.asm). A Remix routine in the row has no port yet:
    a lab build runs the default and lists it."""
    base = sym["Character.grounded_script.table"]
    by_value = {rom.u32_ram(base + 4 * k): kind for kind, k in GROUNDED_CASE_KINDS.items()}
    default = rom.u32_ram(base + 4 * 1)
    value = int(tables["grounded_script"]["value"], 16)
    fallbacks = []
    if value == default:
        grounded = "NDS_P4_FOREIGN_FKIND"
    elif value in by_value:
        grounded = f"nFTKind{by_value[value]}"
    elif any(sym.get(name) == value for name in GROUNDED_PORTS):
        grounded = "NDS_P4_FOREIGN_FKIND"
    elif lab_fallback:
        grounded = "NDS_P4_FOREIGN_FKIND"
        fallbacks.append({"status": "grounded_script", "slot": "landing",
                          "routine": f"{value:#010x}", "fallback": "default"})
    else:
        raise GenError(f"grounded_script {value:#010x} is a Remix routine with no DS port")
    pipe = rom.read_ram(sym["Character.pipe_turn.table"] + remix_id, 1)[0] != 0
    return f"{{ {grounded}, {int(pipe)}, {{ 0, 0 }} }}", fallbacks


def computer_reflect(rom: R.Rom, sym: dict, remix_id: int) -> str:
    """The content's NDS_P4_COMPUTER_REFLECT_* bits: Character.fighter_reflect
    at its id (a reflector: CPUs hold their projectiles against it, and its own
    CPU flags item hazards), and whether the reflect AI hooks take Fox's branch
    for it (all three agree)."""
    bits = []
    if rom.read_ram(sym["Character.fighter_reflect.table"] + remix_id, 1)[0] != 0:
        bits.append("NDS_P4_COMPUTER_REFLECT_TABLE")
    fox = [remix_id in reflect_fox_ids(rom, sym, hook) for hook in REFLECT_FOX_HOOKS]
    if any(fox) != all(fox):
        raise GenError(f"reflect AI hooks disagree on Fox's branch for id {remix_id}: {fox}")
    if all(fox):
        bits.append("NDS_P4_COMPUTER_REFLECT_FOX")
    return " | ".join(bits) or "0"


def baked_refs(rom: R.Rom, name: str, file_ids: list[int], own: set[int]) -> list[tuple]:
    """(file, dependency, slot, target) for each pointer an entry article's
    file holds into a file outside the content's own: each must be a
    G_SETTIMG image, which the native entry packet carries
    (generate_nds_entry_effects.py --p4 compiles the article with every
    file's images), so the match never loads the dependency for it."""
    import p4_articles  # noqa: E402

    rows = []
    for article in p4_articles.ARTICLES.get(name, ()):
        if not article.get("entry") or "special" not in article:
            continue
        fid = file_ids[4 + article["special"]]
        data = rom.file_bytes(fid)
        for slot, ref in sorted(rom.reloc_slots(fid).items()):
            if ref[0] != "extern" or ref[1] in own:
                continue
            if slot < 4 or data[slot - 4] != 0xFD:
                raise GenError(f"{article['name']} {fid:#x}: pointer {slot:#x} into {ref[1]:#x} "
                               "is not a G_SETTIMG image")
            rows.append((fid, ref[1], slot, ref[2]))
    return rows


def jab_rows(rom: R.Rom, sym: dict, tables: dict) -> str:
    """The content's NDSP4Jab initializer from its eight jab table rows."""
    def rows(table: str) -> tuple[int, dict[int, str]]:
        base = sym[f"Character.{table}.table"]
        by_value: dict[int, str] = {}
        for kind, k in JAB_KIND_IDS.items():
            by_value.setdefault(rom.u32_ram(base + 4 * k), kind)
        return int(tables[table]["value"], 16), by_value

    def words(at: int, n: int) -> list[int]:
        return [rom.u32_ram(at + 4 * i) for i in range(n)]

    def jump(target: int) -> int:
        return 0x08000000 | ((target >> 2) & 0x3FFFFFF)

    value, kinds = rows("jab_3")
    if kinds.get(value) not in ("Mario", "Fox"):
        raise GenError(f"jab_3 {value:#010x} is neither ENABLED nor DISABLED")
    jab3 = int(kinds[value] == "Mario")

    value, kinds = rows("jab_3_timer")
    kind = kinds.get(value)
    if kind == "Fox":
        followup = 0.0
    elif kind in ("Mario", "Luigi", "Captain", "Link", "Ness"):
        followup = 24.0  # FTCOMMON_ATTACK1_FOLLOWUP_FRAMES_DEFAULT
    else:
        # lui at, HI; mtc1 at, fN; j end; swc1 fN, 0x150(v1) (Lanky's 42).
        w = words(value, 4)
        if not ((w[0] >> 16) == 0x3C01 and (w[1] & 0xFFFF07FF) == 0x44810000 and
                w[2] == jump(JAB3_TIMER_END) and (w[3] & 0xFFE0FFFF) == 0xE4600150):
            raise GenError(f"jab_3_timer {value:#010x} is not a timer store")
        followup = struct.unpack(">f", struct.pack(">I", (w[0] & 0xFFFF) << 16))[0]

    value, kinds = rows("jab_3_action")
    kind = kinds.get(value)
    if kind == "Fox":
        jab3_status = "0"
    elif kind in JAB3_ACTIONS:
        jab3_status = JAB3_ACTIONS[kind]
    else:
        # ori t7, r0, ACTION; j end; sw t7, 0x20(sp) (Bowser's, Banjo's, Dedede's).
        w = words(value, 3)
        if not ((w[0] >> 16) == 0x340F and w[1] == jump(JAB3_ACTION_END) and w[2] == 0xAFAF0020):
            raise GenError(f"jab_3_action {value:#010x} is not an action store")
        jab3_status = f"{w[0] & 0xFFFF:#x}"

    value, kinds = rows("rapid_jab")
    if kinds.get(value) not in ("Fox", "Mario"):
        raise GenError(f"rapid_jab {value:#010x} is neither ENABLED nor DISABLED")
    rapid = int(kinds[value] == "Fox")

    value, kinds = rows("rapid_jab_unknown")
    kind = kinds.get(value)
    if kind == "Mario":
        rapid_inputs, rapid_status = 0, "0"
    elif kind in RAPID_COUNTS:
        rapid_inputs, rapid_status = RAPID_COUNTS[kind]
    else:
        raise GenError(f"rapid_jab_unknown {value:#010x} needs a port")
    steps = []
    for table, step in (("rapid_jab_begin_action", "Start"), ("rapid_jab_loop_action", "Loop"),
                        ("rapid_jab_ending_action", "End")):
        value, kinds = rows(table)
        kind = kinds.get(value)
        if kind == "Mario":
            steps.append("0")
        elif kind in RAPID_KINDS:
            steps.append(f"nFT{kind}StatusAttack100{step}")
        else:
            raise GenError(f"{table} {value:#010x} needs a port")
    return (f"{{ {c_float(followup)}, {jab3_status}, {jab3}, {rapid}, {rapid_status}, "
            f"{rapid_inputs}, 0, {', '.join(steps)} }}")

# CPU rows: FTComputerAttack as assembled (input, hit start, hit end, detect
# x near/far, y near/far), Remix's input routine ids past the source's 0x31
# scripts, and the two ai_long_range cases of the source's fkind switch in
# func_ovl3_80138AA8 (no long-range special; the projectile users').
CPU_ATTACK_ROW = ">iiiffff"
CPU_INPUT_BASE = 0x31
AI_LONG_RANGE_NONE = 0x80138ECC
AI_LONG_RANGE_PROJECTILE = 0x80138D24


def parent_storage(parent: str) -> dict:
    """The parent's FTData pointer block (decomp ft/ftdata.c): the globals its
    code reads its shield-pose and special files through, and its particle
    bank. A P4 content keeps the parent's code, so its FTData names the same
    globals."""
    import re

    title = PARENT_DECOMP.get(parent)
    if title is None:
        raise GenError(f"parent {parent} has no decomp FTData")
    text = DECOMP_FTDATA.read_text(encoding="utf-8")
    m = re.search(r"FTData dFT%sData =\s*\{(.*?)\};" % title, text, re.S)
    if m is None:
        raise GenError(f"dFT{title}Data not found in {DECOMP_FTDATA}")
    body = re.sub(r"//[^\n]*", "", m.group(1))
    entries = [e.strip() for e in body.split(",")]

    def ref(index: int) -> str | None:
        e = entries[index]
        if e in ("0", "0x00000000", "NULL"):
            return None
        if not e.startswith("&"):
            raise GenError(f"dFT{title}Data[{index}] = {e!r} is not a reference")
        return e[1:]

    # 9 file IDs and the main size, then p_file_main, mainmotion, submotion,
    # model, shieldpose, special1..4 and the particle bank (ftdata.c rows).
    return {
        "shared_storage": {
            "p_file_shieldpose": ref(14),
            "p_file_special1": ref(15), "p_file_special2": ref(16),
            "p_file_special3": ref(17), "p_file_special4": ref(18),
        },
        "particle": ref(19),
    }


def fighter_spec(remix: str, parent: str) -> dict:
    """A content's DS facts: its registry row (scripts/p4/contents.json) and its
    parent's storage."""
    import p4_contents  # noqa: E402

    for row in p4_contents.load_registry():
        if row["remix"] == remix:
            spec = {"name": row["name"], "title": row["title"], "kind_index": row["id"] - 1}
            spec.update(parent_storage(parent))
            if spec["particle"] is None:
                raise GenError(f"parent {parent} has no particle bank global")
            return spec
    raise GenError(f"{remix}: not in scripts/p4/contents.json")

THROW_DATA_BYTES = 28  # FTThrowHitDesc, the larger of the two SetThrow targets
SPRITE_BYTES = 68      # libultra Sprite (include/PR/sp.h)
SPRITE_READ_BYTES = 56  # through Sprite.bitmap, the last field read


def load_battle_hud(repo_root: Path):
    """scripts/menus/generate_battle_hud.py and its UI decoder, for the stock bake."""
    import importlib.util
    path = repo_root / "scripts" / "menus" / "generate_battle_hud.py"
    spec = importlib.util.spec_from_file_location("p4_generate_battle_hud", path)
    mod = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = mod
    spec.loader.exec_module(mod)
    return mod, mod.load_ui_generator(repo_root)


def ft_sprites(rom: R.Rom, main_id: int, attr_off: int, sprites_field: int) -> dict:
    """The fighter's FTSprites (stock icon, its costume LUTs, series emblem) as
    file references, read through the main file's relocations."""
    slots = rom.reloc_slots(main_id)
    ref = slots.get(attr_off + sprites_field)
    if ref is None or ref[0] != "intern":
        raise GenError(f"{main_id:#x}: FTAttributes.sprites is not an internal pointer")
    base = ref[1]

    def target(off: int) -> tuple[int, int]:
        r = slots.get(off)
        if r is None:
            raise GenError(f"{main_id:#x}+{off:#x}: FTSprites field is not a pointer")
        return (main_id, r[1]) if r[0] == "intern" else (r[1], r[2])

    stock, emblem = target(base), target(base + 8)
    luts_ref = slots.get(base + 4)
    if luts_ref is None or luts_ref[0] != "intern":
        raise GenError(f"{main_id:#x}: stock_luts is not an internal array")
    luts = []
    off = luts_ref[1]
    # The LUT pointer array runs up to the FTSprites struct that follows it.
    while off < base and off in slots:
        luts.append(target(off))
        off += 4
    if not luts or any(f != stock[0] for f, _ in luts):
        raise GenError(f"{main_id:#x}: stock LUTs must share the stock sprite's file")
    return {"stock": stock, "emblem": emblem, "luts": luts}


def present_rows(rom: R.Rom, tables: dict) -> dict:
    """Results/menu presentation from the donor's kind-table rows
    (resultsscreen.asm add_to_results_screen, Character.asm menu tables)."""
    def word(name: str) -> int:
        return int(tables[name]["value"], 16)

    def f32(name: str) -> float:
        return struct.unpack(">f", struct.pack(">I", word(name)))[0]

    name_ptr = word("str_winner_ptr")
    raw = rom.read_ram(name_ptr, 32)
    if b"\x00" not in raw:
        raise GenError("results name string is not terminated in 32 bytes")
    name = raw.split(b"\x00", 1)[0].decode("ascii")
    # winner_bgm is a three-instruction routine: or a0,r0,r0; jal play_bgm;
    # ori a1,r0,<bgm>. Accept exactly that shape.
    routine = struct.unpack(">3I", rom.read_ram(word("winner_bgm"), 12))
    if routine[0] != 0x00002025 or (routine[1] >> 26) != 0x03 or (routine[2] >> 16) != 0x3405:
        raise GenError(f"winner_bgm routine {[hex(w) for w in routine]} is not add_victory_bgm")
    return {
        "results_name": name, "results_name_x": f32("str_winner_lx"),
        "results_name_scale": f32("str_winner_scale"), "results_wins_x": f32("str_wins_lx"),
        "announce_fgm": word("winner_fgm"), "victory_bgm": routine[2] & 0xFFFF,
        "crowd_chant_fgm": word("crowd_chant_fgm"), "menu_zoom": f32("menu_zoom"),
        "default_costumes": bytes.fromhex(tables["default_costume"]["value"]),
    }


def c_float(v: float) -> str:
    text = repr(float(v))
    return text + ("F" if ("e" in text or "." in text) else ".0F")


def sprite_row(rom: R.Rom, fid: int, off: int) -> dict:
    data = rom.file_bytes(fid)
    # Remix packs some emblem Sprites at the very end of a file with the
    # struct's last word (frac_t, unread here) cut off (Ganondorf's model
    # 0x8B0: 64 of 68 bytes). What this reads and the loader normalizes
    # ends at the bitmap pointer.
    if off + SPRITE_READ_BYTES > len(data):
        raise GenError(f"{fid:#x}+{off:#x}: Sprite out of range")
    width, height = struct.unpack_from(">hh", data, off + 4)
    return {"file_id": fid, "offset": off, "width": width, "height": height,
            "bitmaps": struct.unpack_from(">h", data, off + 40)[0],
            "fmt": data[off + 48], "siz": data[off + 49]}


def stock_texture(rom: R.Rom, fid: int, sprite_off: int) -> int:
    """Sprite.bitmap -> Bitmap.buf, both internal pointers in the sprite's file."""
    slots = rom.reloc_slots(fid)
    bm = slots.get(sprite_off + 52)
    if bm is None or bm[0] != "intern":
        raise GenError(f"{fid:#x}+{sprite_off:#x}: stock bitmap is not internal")
    buf = slots.get(bm[1] + 8)
    if buf is None or buf[0] != "intern":
        raise GenError(f"{fid:#x}+{sprite_off:#x}: stock texture is not internal")
    return buf[1]


class GenError(SystemExit):
    pass


def parse_key(key: str) -> tuple:
    kind, rest = key.split(":", 1)
    if kind == "ram":
        return ("ram", int(rest, 16))
    fid, off = rest.split(":")
    return ("file", int(fid, 16), int(off, 16))


def command_length(op: str) -> int:
    from remix_export import EVENTS, REMIX_CUSTOM  # noqa: E402
    for name, length in EVENTS:
        if name == op:
            return length
    for name, length in REMIX_CUSTOM.values():
        if name == op:
            return length
    raise GenError(f"unknown op {op}")


def merge_intervals(spans: list[tuple[int, int]], gap: int = 0x40) -> list[tuple[int, int]]:
    out: list[list[int]] = []
    for lo, hi in sorted(spans):
        if out and lo <= out[-1][1] + gap:
            out[-1][1] = max(out[-1][1], hi)
        else:
            out.append([lo, hi])
    return [(a, b) for a, b in out]


class MotionSynth:
    """Parent motion file + appended donor streams, with one rebuilt intern chain."""

    def __init__(self, rom: R.Rom, parent_motion: int, events: dict, synth_id: int):
        self.rom = rom
        self.parent_motion = parent_motion
        self.synth_id = synth_id
        self.blocks = events["blocks"]
        self.data_refs = events["data_refs"]
        base = bytearray(rom.file_bytes(parent_motion))
        if len(base) % 4:
            raise GenError("parent motion file not word sized")
        self.base_len = len(base)
        spans = []
        for key, block in self.blocks.items():
            loc = parse_key(key)
            if loc[0] != "ram":
                continue
            last = block["commands"][-1] if block["commands"] else None
            end = parse_key(last["at"])[1] + command_length(last["op"]) if last else loc[1]
            spans.append((loc[1], end))
        for ref in self.data_refs:
            loc = parse_key(ref["to"])
            if loc[0] == "ram":
                spans.append((loc[1], loc[1] + THROW_DATA_BYTES))
        self.intervals = merge_intervals(spans)
        self.ram_to_off: list[tuple[int, int, int]] = []
        data = base
        for lo, hi in self.intervals:
            lo &= ~3
            hi = (hi + 3) & ~3
            self.ram_to_off.append((lo, hi, len(data)))
            data += rom.read_ram(lo, hi - lo)
        if len(data) >= 0x40000:
            raise GenError("synthesized motion file exceeds the 16-bit word reloc range")
        self.data = data

    def map_ram(self, addr: int) -> int:
        for lo, hi, off in self.ram_to_off:
            if lo <= addr < hi:
                return off + (addr - lo)
        raise GenError(f"RAM {addr:#010x} not inside any copied interval")

    def map_loc(self, loc: tuple) -> int:
        if loc[0] == "ram":
            return self.map_ram(loc[1])
        if loc[1] != self.parent_motion:
            raise GenError(f"pointer into foreign file {loc[1]:#x} needs an extern slot")
        return loc[2]

    def build(self) -> bytes:
        rom = self.rom
        entry = rom.entry(self.parent_motion)
        slots = rom.reloc_slots(self.parent_motion)
        intern = {off: tgt[1] for off, tgt in slots.items() if tgt[0] == "intern"}
        # Remix pointer words: the second word of each pointer command in a
        # copied RAM block, and the RANDOM_SFX table pointer.
        for key, block in self.blocks.items():
            if not key.startswith("ram:"):
                continue
            for cmd in block["commands"]:
                at = parse_key(cmd["at"])
                target = cmd.get("to") or cmd.get("data")
                if target is None:
                    if cmd["op"] in ("Goto", "Subroutine", "SetParallelScript", "SetThrow",
                                     "SetDamageThrown", "RemixRandomSFX"):
                        raise GenError(f"{cmd['at']}: unresolved pointer")
                    if cmd["op"] == "RemixGotoMovesetFile":
                        # Run as is (nds_p4.c): Remix adds the 16-bit offset
                        # to the loaded motion file, and the synthesized file
                        # keeps that file's bytes at their offsets.
                        if (cmd["words"][0] & 0xFFFF) + 4 > self.base_len:
                            raise GenError(f"{cmd['at']}: GO_TO_FILE target outside the motion file")
                    continue
                slot_off = self.map_ram(at[1] + 4)
                intern[slot_off] = self.map_loc(parse_key(target))
        data = bytearray(self.data)
        order = sorted(intern)
        for i, off in enumerate(order):
            nxt = (order[i + 1] // 4) if i + 1 < len(order) else 0xFFFF
            tgt = intern[off]
            if tgt % 4 or tgt // 4 >= 0xFFFF:
                raise GenError(f"intern target {tgt:#x} not encodable")
            struct.pack_into(">I", data, off, (nxt << 16) | (tgt // 4))
        head = (order[0] // 4) if order else 0xFFFF
        ext = rom.extern_ids(self.parent_motion)
        out = bytearray(R.O2R_HEADER)
        out += struct.pack("<IHHI", self.synth_id, head, entry.reloc_extern, len(ext))
        for x in ext:
            out += struct.pack("<H", x)
        out += struct.pack("<I", len(data))
        out += data
        self.intern_count = len(order)
        return bytes(out)


class _SynthRom:
    """Just enough of remix_rom.Rom for EventDecoder to walk one O2R container."""

    def __init__(self, base: R.Rom, blob: bytes):
        self.base = base
        self.file_id, self.intern, self.extern, n = struct.unpack_from("<IHHI", blob, 0x40)
        self.externs = list(struct.unpack_from(f"<{n}H", blob, 0x4C))
        size_off = 0x4C + 2 * n
        size = struct.unpack_from("<I", blob, size_off)[0]
        self.body = blob[size_off + 4:size_off + 4 + size]

    def file_bytes(self, fid):
        return self.body if fid == self.file_id else self.base.file_bytes(fid)

    def entry(self, fid):
        if fid != self.file_id:
            return self.base.entry(fid)
        return R.TableEntry(False, 0, self.intern, 0, self.extern, len(self.body) // 4)

    def extern_ids(self, fid):
        return self.externs if fid == self.file_id else self.base.extern_ids(fid)

    reloc_slots = R.Rom.reloc_slots

    def u32_ram(self, ram):
        raise R.RomError("synthesized file has no RAM space")


def round_trip(rom: R.Rom, blob: bytes, synth: "MotionSynth", roots: list[tuple[str, int]]) -> int:
    """Decode each root inside the synthesized file and compare the op/word
    stream with the donor decode; pointer words are compared by mapped target."""
    from remix_export import EventDecoder, Failures, script_root  # noqa: E402
    fake = _SynthRom(rom, blob)
    fail = Failures()
    dec = EventDecoder(fake, {}, fake.file_id, fail)
    donor = EventDecoder(rom, {}, synth.parent_motion, Failures())
    checked = 0
    for origin, value in roots:
        if value == NO_SCRIPT:
            continue
        src_root = script_root(value, synth.parent_motion)
        donor.decode(src_root, origin)
        new_root = ("file", fake.file_id, synth.map_ram(value) if value & 0x80000000 else value)
        dec.decode(new_root, origin)
        checked += 1
    if fail.items:
        raise GenError("round trip decode failed: " + "; ".join(fail.items[:5]))

    def stream(blocks, key, seen):
        out = []
        while key and key not in seen:
            seen.add(key)
            b = blocks[key]
            for c in b["commands"]:
                words = c["words"][:1]
                out.append((c["op"], tuple(words)))
            nxt = [e["to"] for e in b["edges"] if e["kind"] == "goto"]
            key = nxt[0] if nxt else None
        return out

    for origin, value in roots:
        if value == NO_SCRIPT:
            continue
        a = stream(donor.blocks, donor.key(script_root(value, synth.parent_motion)), set())
        b = stream(dec.blocks, dec.key(("file", fake.file_id,
                                        synth.map_ram(value) if value & 0x80000000 else value)), set())
        if a != b:
            raise GenError(f"round trip mismatch for {origin}")
    if len(dec.blocks) != len(donor.blocks):
        raise GenError(f"round trip block count {len(dec.blocks)} != donor {len(donor.blocks)}")
    return checked


def c_ident_list(values, per_line=4, fmt="{:#010x}"):
    items = [fmt.format(v) for v in values]
    return ",\n    ".join(", ".join(items[i:i + per_line]) for i in range(0, len(items), per_line))


def retarget_o2r_externs(blob: bytes, mapping: dict[int, int]) -> bytes:
    """Rewrite external file IDs in an O2R container (same count, same order)."""
    n = struct.unpack_from("<I", blob, 0x48)[0]
    out = bytearray(blob)
    for i in range(n):
        off = 0x4C + 2 * i
        fid = struct.unpack_from("<H", blob, off)[0]
        if fid in mapping:
            struct.pack_into("<H", out, off, mapping[fid])
    return bytes(out)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--staging", type=Path, required=True)
    ap.add_argument("--export", type=Path, required=True, help="remix_export.py --out directory")
    ap.add_argument("--out", type=Path, required=True)
    ap.add_argument("--lab-fallback", action="store_true",
                    help="lab builds only: a status with an unported donor routine runs a "
                         "plain end-of-motion set (listed)")
    args = ap.parse_args()

    resolved = json.loads((args.export / "resolved.json").read_text(encoding="utf-8"))
    events = json.loads((args.export / "events.json").read_text(encoding="utf-8"))
    fighter = resolved["fighter"]
    spec = fighter_spec(fighter, resolved["parent"])
    rom = R.Rom(args.staging / "ssb64asm.z64")
    vanilla = R.Rom(args.staging / "roms" / "ssb.rom")
    name, title = spec["name"], spec["title"]
    desc = resolved["descriptor"]
    file_ids = list(desc["file_ids"])
    parent_motion = file_ids[1]
    # The runtime lets the synthesized motion file stand in for the parent's
    # own (battleship_ftmanager.c ndsP4PublishParentMotion), as Remix's load of
    # the parent's motion file by its id does.
    if parent_motion != resolved["parent_descriptor"]["file_ids"][1]:
        raise GenError(f"motion file {parent_motion:#x} is not the parent's")
    # Special files 1-4 load into the parent's globals (parent_storage), which
    # the parent's code reads at the parent's offsets. Remix gives a fighter
    # its own file pointers and patches those readers (Wolf's Wolfen inside
    # Fox's Arwing entry). Until a content has that, a lab build loads the
    # parent's files there, and a shipping build refuses.
    # Exact without a port: the same layout as the parent's file (Remix points
    # the parent's readers at the donor's re-skinned copy: Ganondorf's Falcon
    # Kick reads his own file at Captain's offsets), or a slot the parent
    # has no file in (no parent code reads it).
    parent_ids = resolved["parent_descriptor"]["file_ids"]
    special_standins = []
    own_special = OWN_SPECIAL_FILES.get(name, ())

    def file_layout(fid: int) -> tuple:
        return (len(rom.file_bytes(fid)),
                tuple(sorted((off, t[0]) for off, t in rom.reloc_slots(fid).items())))

    for i in range(5, 9):
        if (i - 4) in own_special:
            if file_ids[i] == 0 or file_ids[i] == parent_ids[i]:
                raise GenError(f"special file {i - 4} is listed as {name}'s own but is "
                               f"{file_ids[i]:#x} (the parent's {parent_ids[i]:#x})")
            continue
        if file_ids[i] == parent_ids[i]:
            continue
        if parent_ids[i] == 0:
            # No parent code reads this slot, but the content's FTData would
            # load it through the parent's storage, and the parent has none
            # (Sheik's special1 under Captain). Only the donor's own
            # routines read it: they need its storage when ported.
            if spec["shared_storage"][f"p_file_special{i - 4}"] is None:
                if not args.lab_fallback:
                    raise GenError(f"special file {i - 4} {file_ids[i]:#x}: the parent has no "
                                   "storage for it; needs the content's own")
                special_standins.append({"status": "files", "slot": f"special{i - 4}",
                                         "routine": f"{file_ids[i]:#x}", "fallback": "0x0"})
                file_ids[i] = 0
            continue
        if file_ids[i] != 0 and file_layout(file_ids[i]) == file_layout(parent_ids[i]):
            continue
        if not args.lab_fallback:
            raise GenError(f"special file {i - 4} {file_ids[i]:#x} differs from the parent's "
                           f"{parent_ids[i]:#x}: needs its own storage and ported readers")
        special_standins.append({"status": "files", "slot": f"special{i - 4}",
                                 "routine": f"{file_ids[i]:#x}",
                                 "fallback": f"{parent_ids[i]:#x}"})
        file_ids[i] = parent_ids[i]

    # Lab builds: the donor files those stand-ins replace are pulled into the
    # main closure only by its header words (checked: no other file names
    # them), so they are neither sized nor loaded; the words read NULL
    # (nds_p4.c ndsP4LabSkipsDependency). Sonic's are 172 KB.
    lab_skip = sorted({int(s["routine"], 16) for s in special_standins} - {0})
    # The parent's file standing in for one: outside the content's closure,
    # so the runtime loads it whole (ndsP4LoadOpenSpecialFiles).
    open_special_mask = 0
    for s in special_standins:
        if int(s["fallback"], 16) != 0:
            open_special_mask |= 1 << (int(s["slot"][len("special"):]) - 1)

    synth_id = SYNTH_FILE_BASE + spec["kind_index"] * 0x10
    if rom.file_bytes(parent_motion) != vanilla.file_bytes(parent_motion):
        raise GenError(f"parent motion {parent_motion:#x} differs from vanilla")
    synth = MotionSynth(rom, parent_motion, events, synth_id)
    motion_o2r = synth.build()

    deviations = []
    o2r_out = args.out / "o2r"
    o2r_out.mkdir(parents=True, exist_ok=True)
    shipped = {}
    for row in resolved["file_closure"]:
        fid = row["file_id"]
        if row["vanilla_id"]:
            if not row["vanilla_identical"]:
                why = EQUIVALENT_FILES.get(fid)
                if why is None:
                    raise GenError(f"vanilla ID {fid:#x} carries donor bytes with no classification")
                deviations.append({"file_id": fid, "class": "equivalent", "why": why})
            continue
        blob = rom.o2r(fid)
        # The main file's references into the parent motion file resolve to the
        # same offsets in the synthesized copy, so the parent file never loads.
        blob = retarget_o2r_externs(blob, {parent_motion: synth_id})
        if len(rom.extern_ids(fid)) > MAX_EXTERN_IDS:
            raise GenError(f"file {fid:#x}: {len(rom.extern_ids(fid))} extern ids, the DS loader holds "
                           f"{MAX_EXTERN_IDS} (NDS_RELOC_EXTERN_FILE_ID_CAPACITY)")
        (o2r_out / f"{fid:04x}").write_bytes(blob)
        shipped[fid] = hashlib.sha256(blob).hexdigest()
    (o2r_out / f"{synth_id:04x}").write_bytes(motion_o2r)
    shipped[synth_id] = hashlib.sha256(motion_o2r).hexdigest()

    # HUD presentation: the stock icon baked exactly as the legacy roster's
    # (generate_battle_hud.stock_asset), and the FTSprites sprites the reloc
    # loader normalizes so the emblem bake reads real header fields.
    repo_root = Path(__file__).resolve().parents[2]
    import ft_layout  # noqa: E402
    fs = ft_sprites(rom, file_ids[0], desc["o_attributes"],
                    ft_layout.layout()["FTAttributes.sprites"])
    stock_fid, stock_off = fs["stock"]
    if stock_fid not in shipped:
        raise GenError(f"stock sprite file {stock_fid:#x} is not a donor file this build ships")
    hud, ui = load_battle_hud(repo_root)
    stock_gfx, stock_palettes = hud.stock_asset(ui, repo_root, {
        "file": f"{stock_fid:04x}", "path": o2r_out / f"{stock_fid:04x}",
        "sprite": stock_off, "texture": stock_texture(rom, stock_fid, stock_off),
        "palettes": [o for _, o in fs["luts"]]})
    sprite_rows = [sprite_row(rom, *fs["stock"]), sprite_row(rom, *fs["emblem"])]
    present = present_rows(rom, resolved["kind_tables"])
    for row in sprite_rows:
        if row["file_id"] not in shipped:
            raise GenError(f"sprite file {row['file_id']:#x} is not a donor file this build ships")

    def script_offset(value: int) -> int:
        if value == NO_SCRIPT:
            return NO_SCRIPT
        if value & 0x80000000:
            return synth.map_ram(value)
        return value

    roots = [(f"motion:{m['index']:#x}", m["script"]) for m in resolved["motions"]]
    roots += [(f"menu:{m['index']:#x}", m["script"]) for m in resolved["menu_motions"]]
    checked = round_trip(rom, motion_o2r, synth, roots)

    motions = resolved["motions"]
    rows = []
    for m in motions:
        rows.append((m["anim_file_id"], script_offset(m["script"]), m["anim_flags"]))
    menus = []
    for m in resolved["menu_motions"]:
        s = m["script"]
        if s != NO_SCRIPT and not (s >= R.REMIX_CODE_RAM or
                                   R.MENU_OVERLAY_RAM[0] <= s < R.MENU_OVERLAY_RAM[1]):
            raise GenError(f"menu motion {m['index']}: vanilla script pointer {s:#x} needs a symbol")
        menus.append((m["anim_file_id"], script_offset(s), m["anim_flags"]))

    # The special status table (0xDC on): Remix's action array, statuses its
    # add_new_action appended included. A slot whose callback is the parent's
    # is inherited at run time from the parent's own table (ndsP4SpecialStatusDescs),
    # so this file never names a parent routine; a slot Remix changed names a
    # vanilla routine or a hand port (CALLBACK_PORTS). In --lab-fallback builds
    # a status with a donor routine that has no port runs a plain
    # end-of-motion set in all four slots, and each such routine is listed in
    # the manifest; shipping builds fail instead.
    slots = ("proc_update", "proc_interrupt", "proc_physics", "proc_map")
    specials = []
    lab_fallbacks = list(special_standins)
    for s in resolved["statuses"]:
        inh = s.get("inherited")
        if inh is None:
            continue  # the common statuses: the shared table
        sid = s["status_id"]
        parent_procs = s.get("parent_procs")
        h = s["sflags"]
        air = (h >> 11) & 1
        procs = []
        unported = []
        for i, proc in enumerate(slots):
            nm = s.get(proc + "_name")
            if parent_procs is not None and inh.get(proc, False):
                procs.append("NDS_P4_PROC_INHERIT")
            elif s[proc] == 0:
                procs.append("NULL")
            elif nm in CALLBACK_PORTS:
                procs.append(CALLBACK_PORTS[nm])
            elif nm and "." not in nm:
                procs.append(nm)
            elif args.lab_fallback:
                unported.append((proc, nm))
                procs.append(None)
            else:
                raise GenError(f"status {sid:#x} {proc}: {nm!r} has no DS port")
        if unported:
            # A status with an unported routine runs a plain end-of-motion set
            # in all four slots: the parent's routines expect the parent's own
            # script (Captain's Falcon Dive procs on Marth's Dolphin Slash
            # caught a fighter the script never gave throw data, and the
            # release faulted), and mixing them with stand-ins is no better.
            procs = list(("ftAnimEndSetFall", "NULL", "ftPhysicsApplyAirVelDriftFastFall",
                          "mpCommonProcFighterWaitOrLanding") if air else
                         ("ftAnimEndSetWait", "NULL", "ftPhysicsApplyGroundVelFriction",
                          "mpCommonSetFighterFallOnEdgeBreak"))
            for proc, nm in unported:
                lab_fallbacks.append({"status": hex(sid), "slot": proc, "routine": nm,
                                      "fallback": "end-of-motion set"})
        specials.append({"status": sid, "name": s.get("name"), "motion_id": s["motion_id"],
                         "attack_id": s["attack_id"], "unused": (h >> 13) & 7,
                         "is_smash_attack": (h >> 12) & 1, "ga": air,
                         "is_projectile": (h >> 10) & 1, "stat_attack_id": h & 0x3FF,
                         "procs": procs})
    if [r["status"] for r in specials] != list(range(0xDC, 0xDC + len(specials))):
        raise GenError("special statuses are not contiguous from 0xDC")

    # The entry rows (nds_p4.h NDSP4Entry). entry_script names a case of the
    # source's switch in ftCommonAppearSetStatus, or the address past it (no
    # effect), or a Remix routine; Remix patches the shared makers to read the
    # fighter's own files. Exact without a port: no effect, or the parent's
    # case on the parent's special files or same-layout copies of them.
    tables = resolved["kind_tables"]
    action = bytes.fromhex(tables["entry_action"]["value"])
    appear = struct.unpack(">ii", action)
    script = int(tables["entry_script"]["value"], 16)
    parent_script = int(tables["entry_script"]["parent_value"], 16)
    entry_lab_fallback = 0
    if script == ENTRY_SCRIPT_NONE:
        entry_effect = "NDS_P4_ENTRY_NONE"
    elif script == parent_script:
        # Every special slot the parent's code reads now holds the
        # parent's file or a same-layout copy (the stand-ins above).
        entry_effect = "NDS_P4_ENTRY_PARENT"
    elif ENTRY_PORTS.get(name) == script:
        # Another kind's case, ported on the content's own files
        # (NDSP4Overrides.entry_case in src/port/nds_p4_<name>.c).
        entry_effect = "NDS_P4_ENTRY_PORT"
    elif args.lab_fallback:
        entry_effect = "NDS_P4_ENTRY_NONE"
        entry_lab_fallback = 1
        lab_fallbacks.append({"status": "entry", "slot": "entry_script",
                              "routine": f"{script:#010x}", "fallback": "NDS_P4_ENTRY_NONE"})
    else:
        raise GenError(f"entry_script {script:#010x} (parent {parent_script:#010x}) needs a port")

    # The special-move starters (nds_p4.h NDS_P4_SPECIAL_*): Remix's per-kind
    # tables that replace dFTCommonSpecial*StatusList. A row equal to the
    # parent's stays NULL (the source table's own entry); a changed row names
    # a vanilla routine or a hand port. In --lab-fallback builds a donor
    # routine with no port starts nothing (ndsP4LabSpecialStandIn): the
    # parent's starter would enter a parent status the donor may never use,
    # with its dead row (Lanky's 0xE1 has no motion, so Mario's Super Jump
    # Punch read a TransN joint that was never made).
    remix_names = None
    decomp_names = None
    special_starts = []
    proto_extra = set()
    for table in SPECIAL_START_TABLES:
        value = int(tables[table]["value"], 16)
        if value == int(tables[table]["parent_value"], 16):
            special_starts.append("NULL")
            continue
        if remix_names is None:
            _, remix_names = R.load_symbols(args.staging / "logfile.log")
            decomp_names = R.load_decomp_symbols(DECOMP_SYMBOLS)
        if value >= R.REMIX_CODE_RAM:
            nm = (remix_names.get(value) or [None])[0]
        else:
            nm = decomp_names.get(value)
        if nm in CALLBACK_PORTS:
            special_starts.append("ndsP4Proc_" + CALLBACK_PORTS[nm])
            proto_extra.add(CALLBACK_PORTS[nm])
        elif nm and "." not in nm:
            special_starts.append("ndsP4Proc_" + nm)
            proto_extra.add(nm)
        elif args.lab_fallback:
            special_starts.append("ndsP4LabSpecialStandIn")
            lab_fallbacks.append({"status": "special", "slot": table,
                                  "routine": nm or f"{value:#010x}", "fallback": "no special"})
        else:
            raise GenError(f"{table} {value:#010x} ({nm}) has no DS port")

    # The jab and rapid-jab rows (S3, nds_p4.h NDSP4Jab).
    jab = jab_rows(rom, R.load_symbols(args.staging / "logfile.log")[0], tables)
    # yoshi_egg (S3): the egg Yoshi lays around this fighter, its
    # ftCommonYoshiEggDesc (effect size, hurtbox offset and size).
    egg = struct.unpack(">7f", bytes.fromhex(tables["yoshi_egg"]["value"]))

    # CPU rows (S7, nds_p4.h NDSP4Fighter.computer_*). ai_behaviour points at
    # the content's attack list (grounded rows, END, aerial rows, END), read
    # as assembled; the Remix input routines it names (ids from 0x31) come
    # from AI.command_table, each up to its last END before the padding.
    # ai_long_range is one of two cases of the source's fkind switch: no
    # long-range special, or the projectile users' walk-and-shoot.
    computer_attacks = []
    if not tables["ai_behaviour"]["same_as_parent"]:
        at = int(tables["ai_behaviour"]["value"], 16)
        ends = 0
        while ends < 2:
            row = struct.unpack(CPU_ATTACK_ROW, rom.read_ram(at, 0x1C))
            at += 0x1C
            computer_attacks.append(row)
            ends += row[0] == -1
            if len(computer_attacks) > 64:
                raise GenError("ai_behaviour: no second END within 64 rows")
    long_range = int(tables["ai_long_range"]["value"], 16)
    if tables["ai_long_range"]["same_as_parent"]:
        computer_long_range = "NDS_P4_COMPUTER_LONG_RANGE_PARENT"
    elif long_range == AI_LONG_RANGE_NONE:
        computer_long_range = "NDS_P4_COMPUTER_LONG_RANGE_NONE"
    elif long_range == AI_LONG_RANGE_PROJECTILE:
        computer_long_range = "NDS_P4_COMPUTER_LONG_RANGE_PROJECTILE"
    else:
        raise GenError(f"ai_long_range {long_range:#010x} is neither source case")
    reflect = computer_reflect(rom, R.load_symbols(args.staging / "logfile.log")[0],
                               resolved["remix_kind_id"])
    # S3: the landing switch's case (grounded_script) and the pipe turn.
    cases, case_fallbacks = kind_cases(rom, R.load_symbols(args.staging / "logfile.log")[0],
                                       resolved["remix_kind_id"], tables, args.lab_fallback)
    lab_fallbacks.extend(case_fallbacks)
    computer_scripts = []
    remix_inputs = sorted({r[0] for r in computer_attacks if r[0] >= CPU_INPUT_BASE})
    if remix_inputs:
        sym, by_addr = R.load_symbols(args.staging / "logfile.log")
        command_table = sym["AI.command_table"]
        starts = sorted(a for a in by_addr if a >= R.REMIX_CODE_RAM)
        for inp in remix_inputs:
            ptr = rom.u32_ram(command_table + 4 * inp)
            if ptr == 0:
                raise GenError(f"CPU input routine {inp:#x} is not in AI.command_table")
            end = min((a for a in starts if a > ptr), default=ptr + 0x100)
            body = rom.read_ram(ptr, end - ptr)
            if b"\xff" not in body:
                raise GenError(f"CPU input routine {inp:#x} has no END")
            computer_scripts.append((inp, body[:body.rindex(b"\xff") + 1]))

    for fid in shipped:
        if fid in lab_skip or fid in (file_ids[0], synth_id):
            continue
        named = set(rom.extern_ids(fid)) & set(lab_skip)
        if named:
            raise GenError(f"file {fid:#x} names skipped donor files {sorted(named)}")

    # ftManagerSetupFileSize's three answers, computed the way
    # generate_fighter_production_manifest.extern_alloc_size does for the
    # legacy roster: 16-byte aligned payloads, each dependency counted once.
    def alloc_size(root: int) -> int:
        seen: set[int] = set()

        def visit(fid: int) -> int:
            if fid in seen:
                return 0
            seen.add(fid)
            if fid == synth_id:
                size, deps = len(synth.data), rom.extern_ids(parent_motion)
            else:
                size, deps = len(rom.file_bytes(fid)), rom.extern_ids(fid)
                deps = [synth_id if d == parent_motion and fid == file_ids[0] else d for d in deps]
            total = (size + 0xF) & ~0xF
            for d in deps:
                if d in lab_skip:
                    continue
                total = (total + 0xF) & ~0xF
                total += visit(d)
            return total
        return visit(root)

    def largest(rows_):
        best = 0
        for a, _, f in rows_:
            if a and not (f & 0x2):  # FTANIM_FLAG_SHIELDPOSE rows load from the pose file
                best = max(best, alloc_size(a))
        return best

    sizes = (alloc_size(file_ids[0]), largest([(m["anim_file_id"], 0, m["anim_flags"]) for m in resolved["motions"]]),
             largest([(m["anim_file_id"], 0, m["anim_flags"]) for m in resolved["menu_motions"]]))

    own = {row["file_id"] for row in resolved["file_closure"] if not row["vanilla_id"]}
    anim_flags: dict[int, int] = {}
    for m in resolved["motions"] + resolved["menu_motions"]:
        a = m["anim_file_id"]
        if a in own and not (m["anim_flags"] & 0x2):
            anim_flags[a] = anim_flags.get(a, 0) | m["anim_flags"]
    anims = [(a, bool(f & 0x8)) for a, f in sorted(anim_flags.items())]
    if any(a >= 0x8000 for a, _ in anims):
        raise GenError("animation id does not fit the 15-bit table encoding")

    ident = f"NdsP4{title}"
    store = dict(spec["shared_storage"])
    own_store = set()
    for n in own_special:
        store[f"p_file_special{n}"] = f"g{ident}Special{n}"
        own_store.add(f"g{ident}Special{n}")
    lines = [
        f"/* Generated by scripts/p4/generate_p4_fighter.py for {fighter} from the staged",
        " * Remix donor build. Derived from the user's ROM: build output only, never tracked. */",
        "#include <nds/nds_p4.h>",
        "",
    ]
    proto = sorted(proto_extra | {p for r in specials for p in r["procs"]
                    if p not in ("NULL", "NDS_P4_PROC_INHERIT")})
    # Each routine through an alias of its own symbol: a vanilla routine a
    # status slot holds may return a value (mpCommonCheckFighterLanding is
    # sb32), and a second prototype would conflict with its header's.
    for p in proto:
        lines.append(f'void ndsP4Proc_{p}(GObj *fighter_gobj) __asm__("{p}");')
    motion_rows = [f"    {{ {a:#06x}, (intptr_t){s:#010x}, {{ .word = {f:#010x} }} }}," for a, s, f in rows]
    lines += ["", f"/* Script fields are offsets into the synthesized motion file until",
              " * ndsP4BindMenuScripts adds its loaded address (opening statuses read",
              " * them as absolute pointers). */",
              f"static FTMotionDesc s{ident}SubMotionDescs[{len(menus)}] = {{"]
    for a, s, f in menus:
        lines.append(f"    {{ {a:#06x}, (intptr_t){s:#010x}, {{ .word = {f:#010x} }} }},")
    lines += ["};", f"static s32 s{ident}SubMotionCount = {len(menus)};",
              f"static void *s{ident}Main;", f"static void *s{ident}MainMotion;",
              f"static void *s{ident}Model;", ""]
    for _, v in store.items():
        if v in own_store:
            lines.append(f"void *{v}; /* own special file (S6) */")
        elif v:
            lines.append(f"extern void *{v};")
    lines.append(f"extern s32 {spec['particle']};")
    lines += ["", f"FTData g{ident}Data = {{"]
    ids = file_ids[:]
    ids[1] = synth_id
    lines.append("    " + ", ".join(f"{x:#x}" for x in ids) + ",")
    lines.append("    0,")
    lines.append(f"    &s{ident}Main, &s{ident}MainMotion, NULL, &s{ident}Model,")
    sp = store["p_file_shieldpose"]
    lines.append(f"    {('&' + sp) if sp else 'NULL'},")
    lines.append("    " + ", ".join((f"&{store[k]}" if store[k] else "NULL") for k in
                                     ("p_file_special1", "p_file_special2", "p_file_special3", "p_file_special4")) + ",")
    lines.append(f"    &{spec['particle']}, 0, 0, 0, 0,")
    lines.append(f"    {desc['o_attributes']:#x},")
    lines.append("    NULL, /* mainmotion: the content's tables, loaded per match (ndsP4LoadTables) */")
    lines.append(f"    (FTMotionDescArray *)s{ident}SubMotionDescs,")
    lines.append(f"    {len(rows)}, &s{ident}SubMotionCount, 0")
    lines += ["};", "",
              "/* Special statuses from 0xDC, Remix's action array: motion, motion attack,",
              " * status flags (unused, smash, ground/air, projectile, status attack) and",
              " * callbacks; NDS_P4_PROC_INHERIT takes the parent's at first use. */",
              f"FTStatusDesc g{ident}SpecialStatusDescs[{len(specials)}] = {{"]
    for r in specials:
        lines.append(f"    /* {r['status']:#x} {r['name'] or ''} */")
        lines.append(f"    {{ {{ {r['motion_id']}, {r['attack_id']} }}, "
                     f"{{ {{ {r['unused']}, {r['is_smash_attack']}, {r['ga']}, "
                     f"{r['is_projectile']}, {r['stat_attack_id']} }} }}, "
                     f"{', '.join(p if p in ('NULL', 'NDS_P4_PROC_INHERIT') else 'ndsP4Proc_' + p for p in r['procs'])} }},")
    lines += ["};", f"const u32 g{ident}SpecialStatusCount = {len(specials)};",
              f"/* Donor routines this lab build runs as fallbacks (manifest lab_fallbacks). */",
              f"const u32 g{ident}LabFallbackCount = {len(lab_fallbacks)};", "",
              "/* The content's files in NitroFS, nitro:/reloc/p4/<id> (ndsP4RelocAssetPath). */",
              f"const u16 g{ident}RelocAssets[] = {{"]
    ids = [f"{fid:#06x}" for fid in sorted(shipped)]
    for i in range(0, len(ids), 12):
        lines.append("    " + ", ".join(ids[i:i + 12]) + ",")
    lines += ["};", f"const u32 g{ident}RelocAssetCount = {len(shipped)};", "",
              "/* The content's own animation files; bit 15 marks AObjEvent32 (FTANIM_FLAG_ANIMJOINT). */",
              f"const u16 g{ident}Anims[] = {{",
              "    " + ", ".join(f"{a | (0x8000 if ev else 0):#06x}" for a, ev in anims) + ",",
              "};", f"const u32 g{ident}AnimCount = {len(anims)};", "",
              "/* FTFileSize: main closure, largest main-motion and menu-motion figatree. */",
              f"const FTFileSize g{ident}FileSize = {{ {sizes[0]}u, {sizes[1]}u, {sizes[2]}u }};", "",
              "/* HUD: the stock icon as one 8x8 OBJ4 cell and its costume LUTs",
              " * (scripts/menus/generate_battle_hud.py stock_asset). */",
              f"const u8 g{ident}StockGfx[{len(stock_gfx)}] = {{"]
    for k in range(0, len(stock_gfx), 16):
        lines.append("    " + ", ".join(f"0x{b:02x}" for b in stock_gfx[k:k + 16]) + ",")
    lines += ["};", f"const u16 g{ident}StockPalettes[{len(stock_palettes)}][16] = {{"]
    for pal in stock_palettes:
        lines.append("    { " + ", ".join(f"0x{v:04x}" for v in pal) + " },")
    lines += ["};", f"const u32 g{ident}StockPaletteCount = {len(stock_palettes)};", "",
              "/* FTSprites sprites the reloc loader normalizes (stock icon, emblem). */",
              f"const NDSP4SpriteDesc g{ident}Sprites[] = {{"]
    for row in sprite_rows:
        lines.append(f"    {{ {row['file_id']:#x}, {row['bitmaps']}, {row['offset']:#x}, "
                     f"{row['width']}, {row['height']}, {row['fmt']}, {row['siz']} }},")
    lines += ["};", f"const u32 g{ident}SpriteCount = {len(sprite_rows)};", "",
              "/* Results/menu presentation (resultsscreen.asm, Character.asm rows). */",
              f"const NDSP4Present g{ident}Present = {{",
              f"    .results_name = \"{present['results_name']}\",",
              f"    .results_name_x = {c_float(present['results_name_x'])},",
              f"    .results_name_scale = {c_float(present['results_name_scale'])},",
              f"    .results_wins_x = {c_float(present['results_wins_x'])},",
              f"    .announce_fgm = {present['announce_fgm']:#x},",
              f"    .victory_bgm = {present['victory_bgm']:#x},",
              f"    .crowd_chant_fgm = {present['crowd_chant_fgm']:#x},",
              f"    .menu_zoom = {c_float(present['menu_zoom'])},",
              "    .default_costumes = { " + ", ".join(str(b) for b in present["default_costumes"]) + " },",
              "};", ""]
    # SwordTrail.asm rows whose character is this content (the SET AFTERIMAGE
    # id is the row's table index). An exporter without the table is an error,
    # not an empty list: the content's trails would silently not draw.
    if "sword_trails" not in resolved:
        raise SystemExit("export has no sword_trails table (re-run remix_export.py)")
    trails = [t for t in resolved["sword_trails"] if t["character"] == resolved["remix_kind_id"]]
    lines += ["/* Remix sword trails (SwordTrail.asm add_sword_trail rows for this character). */",
              f"const NDSP4SwordTrail g{ident}SwordTrails[{max(len(trails), 1)}] = {{"]
    for t in trails:
        if t["id"] > 0xFF or t["model_part"] > 0xFF or t["axis"] > 2:
            raise SystemExit(f"sword trail {t['id']}: unsupported row {t}")
        lines.append(f"    {{ {t['id']}, {t['model_part']}, {t['axis']}, 0, "
                     f"{t['colour_1']:#010x}u, {t['colour_2']:#010x}u, "
                     f"{c_float(t['start'])}, {c_float(t['end'])} }},")
    lines += ["};", f"const u32 g{ident}SwordTrailCount = {len(trails)};", "",
              "/* Entry rows: appear statuses (right, left), effect, lab stand-in. */",
              f"const NDSP4Entry g{ident}Entry = {{ {{ {appear[0]:#x}, {appear[1]:#x} }}, "
              f"{entry_effect}, {entry_lab_fallback}, {{ 0, 0 }} }};", "",
              "/* Special-move starters (NDS_P4_SPECIAL_*); NULL keeps the source table's. */",
              f"const NDSP4SpecialStart g{ident}SpecialStarts[NDS_P4_SPECIAL_COUNT] = {{",
              "    " + ", ".join(special_starts), "};", "",
              "/* Jab rows (S3, nds_p4.h NDSP4Jab): Remix's jab_3, jab_3_timer, jab_3_action,",
              " * rapid_jab and its rows in place of the source's kind tests. */",
              f"const NDSP4Jab g{ident}Jab = {jab};", "",
              "/* yoshi_egg: effect size, hurtbox offset and size (ftCommonYoshiEggDesc). */",
              f"const f32 g{ident}YoshiEgg[7] = {{ " + ", ".join(c_float(v) for v in egg) + " };", "",
              "/* Lab builds: donor special files a stand-in replaces, never loaded. */",
              f"const u16 g{ident}LabSkipFiles[{max(len(lab_skip), 1)}] = {{ "
              + (", ".join(f"{f:#x}" for f in lab_skip) or "0") + " };",
              f"const u32 g{ident}LabSkipFileCount = {len(lab_skip)};",
              "/* Lab builds: special slots whose stand-in (the parent's file, outside",
              " * the content's closure) loads whole; any other slot outside it reads",
              " * NULL, as Remix's status-buffer lookup does. */",
              f"const u8 g{ident}OpenSpecialMask = {open_special_mask:#04x};", "",
              "/* Kind-table cases (S3, nds_p4.h NDSP4KindCases): grounded_script's",
              " * landing case and pipe_turn. */",
              f"const NDSP4KindCases g{ident}KindCases = {cases};", "",
              "/* CPU rows (S7): ai_long_range; the attack list and the Remix input",
              " * routines it names are in the content's tables (ndsP4LoadTables). */",
              f"const u8 g{ident}ComputerLongRange = {computer_long_range};",
              "/* Remix's reflect AI (Reflect.asm): its fighter_reflect row and Fox's",
              " * branch in the three reflect hooks (NDS_P4_COMPUTER_REFLECT_*). */",
              f"const u8 g{ident}ComputerReflect = {reflect};", ""]
    # S15: the native guard-pose package (scripts/p4/p4_shield_pose.py),
    # written beside the generated source for nitro:/fighters/shield_pose/.
    import p4_shield_pose  # noqa: E402

    try:
        guard_blob, guard = p4_shield_pose.build(o2r_out, file_ids[0], desc["o_attributes"],
                                                 resolved["parent_kind_id"])
    except p4_shield_pose.ShieldPoseError as error:
        raise GenError(f"guard pose: {error}")
    (args.out / "shield_pose.bin").write_bytes(guard_blob or b"")
    lines += ["/* Native guard-pose package (S15, nds_p4.h NDSP4ShieldPose): its own",
              " * blob, the parent's package (alias_fkind), or neither (the raw file). */",
              f"const NDSP4ShieldPose g{ident}ShieldPose = {{ {guard['blob_bytes']}, "
              f"{guard['main_asset']:#x}, {guard['shield_asset']:#x}, {guard['dobj_offset']:#x},",
              "    { " + ", ".join(f"{t:#x}" for t in guard["table_offsets"]) + " },",
              "    { " + ", ".join(f"{t:#x}" for t in guard["main_fixup_slots"]) + " },",
              f"    {guard['alias_fkind']}, {guard['base_count']}, {guard['scratch_words']} }};", ""]
    baked = baked_refs(rom, name, file_ids, own | {synth_id})
    lines += ["/* Entry-article texture pointers its native entry packets carry",
              " * (NDSP4BakedRef): resolved NULL, the dependency never loaded for them. */",
              f"const NDSP4BakedRef g{ident}BakedRefs[{max(len(baked), 1)}] = {{"]
    lines += [f"    {{ {o:#x}, {d:#x}, {s:#x}, {t:#x} }}," for o, d, s, t in baked] or ["    { 0, 0, 0, 0 },"]
    lines += ["};", f"const u32 g{ident}BakedRefCount = {len(baked)};", ""]
    attack_rows = [f"        {{ {inp}, {start}, {end}, {c_float(x0)}, {c_float(x1)}, "
                   f"{c_float(y0)}, {c_float(y1)} }},"
                   for inp, start, end, x0, x1, y0, y1 in
                   (computer_attacks or [(-1, 0, 0, 0.0, 0.0, 0.0, 0.0)])]
    script_bytes = b"".join(body for _, body in computer_scripts)
    script_rows = []
    offset = 0
    for inp, body in (computer_scripts or [(0, b"")]):
        script_rows.append(f"        {{ {inp:#x}, {offset}, {len(body)}, 0 }},")
        offset += len(body)
    byte_rows = ["        " + ", ".join(f"0x{b:02x}" for b in (script_bytes or b"\xff")[k:k + 16]) + ","
                 for k in range(0, max(len(script_bytes), 1), 16)]
    tables = [
        f"/* Generated by scripts/p4/generate_p4_fighter.py for {fighter}: the tables only a",
        " * match reads, objcopied from .p4_tables into nitro:/p4/<name>.tab and read",
        " * into the battle heap for the contents in a match (ndsP4LoadTables). No",
        " * pointers: every field is a value or an offset. Build output only. */",
        "#include <stddef.h>",
        "#include <nds/nds_p4.h>",
        "",
        "typedef struct",
        "{",
        "    NDSP4TablesHeader header;",
        f"    FTMotionDesc motions[{len(rows)}];",
        f"    FTComputerAttack attacks[{max(len(computer_attacks), 1)}];",
        f"    NDSP4ComputerScript scripts[{max(len(computer_scripts), 1)}];",
        f"    u8 script_bytes[{max(len(script_bytes), 1)}];",
        f"}} NDSP4{title}Tables;",
        "",
        f"const NDSP4{title}Tables gNdsP4{title}Tables",
        '    __attribute__((section(".p4_tables"), used, aligned(4))) =',
        "{",
        f"    {{ NDS_P4_TABLES_MAGIC, NDS_P4_TABLES_VERSION, sizeof(NDSP4{title}Tables),",
        f"      offsetof(NDSP4{title}Tables, motions), {len(rows)},",
        f"      offsetof(NDSP4{title}Tables, attacks), {len(computer_attacks)},",
        f"      offsetof(NDSP4{title}Tables, scripts), {len(computer_scripts)},",
        f"      offsetof(NDSP4{title}Tables, script_bytes) }},",
        "    {",
    ] + ["    " + r for r in motion_rows] + [
        "    },",
        "    {",
    ] + attack_rows + [
        "    },",
        "    {",
    ] + script_rows + [
        "    },",
        "    {",
    ] + byte_rows + [
        "    },",
        "};",
        "",
    ]
    (args.out / f"nds_p4_{name}.tables.c").write_text("\n".join(tables), encoding="utf-8", newline="\n")
    src = "\n".join(lines)
    (args.out / f"nds_p4_{name}.generated.c").write_text(src, encoding="utf-8", newline="\n")

    manifest = {
        "schema": "p4-ds-fighter-v1", "fighter": fighter,
        "reference_rom_sha1": resolved["provenance"]["reference_rom_sha1"],
        "synthesized_motion": {"file_id": synth_id, "parent": parent_motion,
                               "round_trip_roots": checked,
                               "parent_bytes": synth.base_len, "bytes": len(synth.data),
                               "intern_slots": synth.intern_count,
                               "remix_intervals": [[hex(a), hex(b)] for a, b in synth.intervals]},
        "shipped_o2r_sha256": {f"{k:#x}": v for k, v in sorted(shipped.items())},
        "special_statuses": len(specials),
        "status_changes": [{"status": hex(r["status"]), "procs": r["procs"]} for r in specials
                           if any(p != "NDS_P4_PROC_INHERIT" for p in r["procs"])],
        "lab_fallbacks": lab_fallbacks,
        "lab_skipped_files": [f"{f:#x}" for f in lab_skip],
        "file_size": {"main": sizes[0], "mainmotion_largest_anim": sizes[1],
                      "submotion_largest_anim": sizes[2]},
        "deviations": deviations,
    }
    (args.out / "manifest.json").write_text(json.dumps(manifest, indent=1), encoding="utf-8")
    print(json.dumps({k: manifest[k] for k in ("synthesized_motion", "special_statuses",
                                                "lab_fallbacks", "deviations")}, indent=1))
    return 0


if __name__ == "__main__":
    sys.exit(main())
