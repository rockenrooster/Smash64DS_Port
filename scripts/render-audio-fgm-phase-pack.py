#!/usr/bin/env python3
"""Build the bounded P1 FGM pack from BattleShip's original audio.

The runtime consumes only this predecoded Nintendo DS IMA-ADPCM pack.  This
script is the sole conversion step and deliberately reads the read-only
BattleShip O2R payloads so the selected IDs stay traceable to their original
FGM UCD, articulation, sound, wavetable, pitch, and duration.
"""

from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
import math
import re
import struct
import sys
from pathlib import Path


PACK_MAGIC = b"FGM1"
PACK_VERSION = 4
PACK_HEADER = struct.Struct("<4sHHII")
PACK_ENTRY = struct.Struct("<HHIIIHHBBHIHH")
PACK_ENVELOPE_POINT = struct.Struct("<HBB")
FGM_TIMER_MICROSECONDS = 5750
FGM_OUTPUT_RATE = 32000
MAX_PACK_BYTES = 512 * 1024
RUNTIME_CACHE_BYTES = (52 * 1024) + (3 * 28 * 1024) + (4 * 16 * 1024)
MAX_RESIDENT_BYTES = 128 * 1024  # historical Phase-C comparison only
PUBLIC_EXCITED_ID = 626
PUBLIC_EXCITED_SAMPLE_COUNT = 104204
PUBLIC_EXCITED_RAMP_SAMPLES = 184
PUBLIC_EXCITED_MIXER_MINIMUM = 1
PUBLIC_EXCITED_IMA_PREDICTOR = -4553
PUBLIC_EXCITED_IMA_INDEX = 65
PUBLIC_EXCITED_GUARD_NIBBLES = (8, 9)
PUBLIC_EXCITED_LOOP_POINT_WORDS = 1
REPEAT_ORACLE_CYCLES = 3
SOURCE_SINE_TABLE_SHA256 = (
    "bc184c0dbd76adecf7ff264d39cc58456546173beba727f189d2716dd8eabf16")

FULL_COVERAGE_IDS = (
    626, 470, 469, 467, 490, 74, 363, 364, 372, 373, 374, 430, 439,
    292, 370, 289, 300, 303, 154, 77, 215, 40, 38, 37, 34, 32, 31,
    375, 429, 431, 435, 440, 19, 41, 42, 43, 185, 186, 187, 189, 190,
    217, 218, 219, 216, 28, 2, 0, 188,
)
FULL_PROGRAM_AOT_IDS = frozenset((
    154, 40, 38, 37, 34, 32, 31,
    375, 429, 431, 435, 440, 19, 41, 42, 43, 185, 186, 187, 189, 190,
    217, 218, 219, 216, 28, 2, 0, 188,
))

ATTACK_ACTION_AUDIT_SHA256 = (
    "ae7690adc1d646e8c0a755510064a324c6ff59f4f578a2f6fdd719351744c601")
ATTACK_CUE_AUDIT_SHA256 = (
    "8e520123996038b06edbd9cd2c3194734b9d7d08bde89159271ff3872a15e69e")
ATTACK_DIRECT_CALL_COUNTS = {
    19: 4,
    41: 17,
    42: 21,
    43: 11,
    185: 2,
    186: 1,
    187: 2,
    189: 1,
    190: 7,
    215: 1,
    217: 1,
    218: 2,
    219: 2,
}
ATTACK_CUE_AUDIT = (
    {
        "id": 19,
        "name": "nSYAudioFGMCatch",
        "root_program_sha256":
            "9fad1e2cea571fa7c2a0407a5ab8e401e9b1460c56229eb9674b934b9c8961a0",
        "blockers": (
            "ucd_pitch_schedule",
            "articulation_pitch_and_volume_schedule",
            "resident_pack_cap",
        ),
    },
    {
        "id": 41,
        "name": "nSYAudioFGMLightSwingL",
        "root_program_sha256":
            "2c127d35837004afaafe6904dd1b02e24bd6cdc29bfe4af7133aeff49b260955",
        "blockers": ("source_custom_fx_bus", "resident_pack_cap"),
    },
    {
        "id": 42,
        "name": "nSYAudioFGMLightSwingM",
        "root_program_sha256":
            "85fe213b07b57a985e1083326f460eb79543547f3481362555daa4341c8995a1",
        "blockers": ("source_custom_fx_bus",),
    },
    {
        "id": 43,
        "name": "nSYAudioFGMLightSwingS",
        "root_program_sha256":
            "56f323a51d99829631b90e5f4cec63a30068e017f22ec39ea135b3e07dae333e",
        "blockers": ("source_custom_fx_bus",),
    },
    {
        "id": 185,
        "name": "nSYAudioFGMFoxSpecialN",
        "root_program_sha256":
            "17238042b674146a514375fb79b5c4af03bd8b0734757fbe2f3c5143163e066a",
        "blockers": (
            "source_sample_loop",
            "articulation_infinite_pitch_and_volume_loop",
            "resident_pack_cap",
        ),
    },
    {
        "id": 186,
        "name": "nSYAudioFGMFoxSpecialHiStart",
        "root_program_sha256":
            "d43bbdf6fbb80605811fda12db256d4eddd04915c6344fa277fe9e2a5daf0823",
        "blockers": (
            "ucd_pitch_schedule",
            "articulation_volume_schedule",
            "resident_pack_cap",
        ),
    },
    {
        "id": 187,
        "name": "nSYAudioFGMFoxSpecialHiFly",
        "root_program_sha256":
            "720103f7048fee9ea9cd24c5383c865de2d063ff7d7eafba659c337027a36b6f",
        "blockers": (
            "simultaneous_fork_voice_0",
            "ucd_pitch_schedule",
            "fork_volume_schedule",
            "fork_source_custom_fx_bus",
            "resident_pack_cap",
        ),
    },
    {
        "id": 189,
        "name": "nSYAudioFGMFoxSpecialLwStart",
        "root_program_sha256":
            "2f6e924d16e5107e8557234d5c9806ba5ff99d86d9d0dd5b671ad7d5dfe7156d",
        "blockers": (
            "ucd_t5_pitch_schedule",
            "source_rate_above_u16",
            "ucd_volume_schedule",
            "source_custom_fx_bus",
        ),
    },
    {
        "id": 190,
        "name": "nSYAudioFGMFoxAttackAirLw",
        "root_program_sha256":
            "84a6c9a138201870077c8f6d2461040e94494e28082790285687d58a9b27df40",
        "blockers": (
            "ucd_pitch_schedule",
            "source_rate_above_u16",
            "source_custom_fx_bus",
        ),
    },
    {
        "id": 215,
        "name": "nSYAudioFGMMarioSpecialN",
        "root_program_sha256":
            "c9f584ac64297bfca52605e5bd01c3d42a31126f7d6e3e73cc4e65b9743cc6ac",
        "blockers": (),
    },
    {
        "id": 217,
        "name": "nSYAudioFGMMarioSpecialHiJump",
        "root_program_sha256":
            "58e4d5252df98dda45a46cabaddfeae9a93b9917dcaa9020e63e9f3e2f45c09a",
        "blockers": (
            "ucd_pitch_schedule",
            "articulation_spawn_mod_47",
            "resident_pack_cap",
        ),
    },
    {
        "id": 218,
        "name": "nSYAudioFGMMarioUnkSwing1",
        "root_program_sha256":
            "e599e2bf74db3900b0e653a72c5d29a7330cabd17e5e05a5b5f9f91392446f23",
        "blockers": (
            "ucd_pitch_schedule",
            "aot_custom_fx_tail_exceeds_resident_pack_cap",
            "source_overlap_exceeds_handle_capacity",
        ),
    },
    {
        "id": 219,
        "name": "nSYAudioFGMMarioUnkSwing2",
        "root_program_sha256":
            "84a6c9a138201870077c8f6d2461040e94494e28082790285687d58a9b27df40",
        "blockers": (
            "ucd_pitch_schedule",
            "source_rate_above_u16",
            "source_custom_fx_bus",
        ),
    },
)

# These selectors are intentionally explicit.  Any upstream layout or program
# change fails generation instead of silently selecting a different sound.
SELECTED = (
    {
        "id": 626,
        "name": "nSYAudioVoicePublicExcited",
        "articulation": 460,
        "sound": 320,
        "pitch_code": 12,
        "duration_ticks": 1200,
        "ucd_volume": 223,
        "articulation_pitch_cents": -1200,
        "loop": True,
        "wave_base": 2966600,
        "wave_length": 15876,
        "loop_start": 1,
        "loop_end": 28215,
        "expected_retained_samples": PUBLIC_EXCITED_SAMPLE_COUNT,
    },
    {
        "id": 470,
        "name": "nSYAudioVoiceAnnounceThree",
        "articulation": 331,
        "sound": 208,
        "pitch_code": 13,
        "duration_ticks": 99,
        "ucd_volume": 220,
        "articulation_pitch_cents": -1200,
        "loop": False,
        "wave_base": 1757880,
        "wave_length": 6328,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 9109,
    },
    {
        "id": 469,
        "name": "nSYAudioVoiceAnnounceTwo",
        "articulation": 332,
        "sound": 209,
        "pitch_code": 13,
        "duration_ticks": 100,
        "ucd_volume": 220,
        "articulation_pitch_cents": -1200,
        "loop": False,
        "wave_base": 1764208,
        "wave_length": 6454,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 9201,
    },
    {
        "id": 467,
        "name": "nSYAudioVoiceAnnounceOne",
        "articulation": 333,
        "sound": 210,
        "pitch_code": 13,
        "duration_ticks": 85,
        "ucd_volume": 230,
        "articulation_pitch_cents": -1200,
        "loop": False,
        "wave_base": 1770664,
        "wave_length": 6102,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 7821,
    },
    {
        "id": 490,
        "name": "nSYAudioVoiceAnnounceGo",
        "articulation": 334,
        "sound": 211,
        "pitch_code": 13,
        "duration_ticks": 150,
        "ucd_volume": 255,
        "articulation_pitch_cents": -1200,
        "loop": False,
        "wave_base": 1776768,
        "wave_length": 8910,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 13801,
    },
    {
        "id": 74,
        "name": "nSYAudioFGMFoxLanding",
        "kind": "movement",
        "render_program": 72,
        "articulation": 3,
        "sound": 1,
        "notes": ((8, 7, 3),),
        "duration_ticks": 3,
        "ucd_volume": 180,
        "articulation_pitch_cents": 700,
        "loop": False,
        "wave_base": 14224,
        "wave_length": 2944,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 621,
        "root_fork_programs": (72,),
        "root_program_sha256":
            "c5fb3a31fc2383118516512dda33fdd0f670a91a490bfc333c27b21e66d6f4a0",
        "render_program_sha256":
            "9b37506dc57cc43b255fa175bfb1e9256fc4c955ae00e4bd600bf4ab123781cf",
        "articulation_program_sha256":
            "300492238b0d3e3b82ac86f63da05c445083fe1aafa2a6d10d7b4bf4f59b7576",
        "aot_source_schedule": True,
        "source_actions": (
            {"action": "dFoxMainMotion_LandingAirX_0x010C",
             "trigger_game_tick": 0, "call": "ftMotionPlayFGM"},
            {"action": "dFoxMainMotion_LandingAirX_0x0124",
             "trigger_game_tick": 0, "call": "ftMotionPlayFGM"},
            {"action": "dFoxMainMotion_LandingAirX_0x018C",
             "trigger_game_tick": 0, "call": "ftMotionPlayFGM"},
            {"action": "dFoxMainMotion_LandingAirF",
             "trigger_game_tick": 0, "call": "ftMotionPlayFGM"},
            {"action": "dFoxMainMotion_LandingAirB",
             "trigger_game_tick": 0, "call": "ftMotionPlayFGM"},
            {"action": "dFoxMainMotion_LandingAirX_0x177C",
             "trigger_game_tick": 0, "call": "ftMotionPlayFGM"},
        ),
        "source_action_file":
            "decomp/src/relocData/208_FoxMainMotion.c",
        "fidelity_debt": (),
    },
    {
        "id": 363,
        "name": "nSYAudioVoiceFoxJumpAerial",
        "kind": "voice",
        "articulation": 227,
        "sound": 108,
        "notes": ((13, 7, 45),),
        "duration_ticks": 45,
        "ucd_volume": 222,
        "articulation_pitch_cents": -1200,
        "loop": False,
        "wave_base": 922072,
        "wave_length": 2116,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 3760,
        "root_fork_programs": (),
        "root_program_sha256":
            "126427d6140813a00aadc14a7c4f51ec2cdeb013a1b030b7f2b9e29c08898b08",
        "render_program_sha256":
            "126427d6140813a00aadc14a7c4f51ec2cdeb013a1b030b7f2b9e29c08898b08",
        "articulation_program_sha256":
            "eff2f55d748352dca4be41a0377216dba9f6ab9a65b68438f09913a514f3a8e3",
        "aot_source_schedule": True,
        "source_actions": (
            {"action": "dFoxMainMotion_JumpAerialB",
             "trigger_game_tick": 0, "call": "ftMotionPlayVoice"},
        ),
        "source_action_file":
            "decomp/src/relocData/208_FoxMainMotion.c",
        "fidelity_debt": (),
    },
    {
        "id": 364,
        "name": "nSYAudioVoiceFoxEscape",
        "kind": "voice",
        "articulation": 221,
        "sound": 102,
        "notes": ((13, 7, 20), (13, 7, 20), (13, 7, 20),
                  (13, 7, 5)),
        "duration_ticks": 65,
        "ucd_volume": 220,
        "articulation_pitch_cents": -1200,
        "loop": False,
        "wave_base": 903200,
        "wave_length": 1638,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 2912,
        "root_fork_programs": (),
        "root_program_sha256":
            "9baa16746dc4d654749dce8e6dd786d13faa021e275593f654ec5ad92a14f89e",
        "render_program_sha256":
            "9baa16746dc4d654749dce8e6dd786d13faa021e275593f654ec5ad92a14f89e",
        "articulation_program_sha256":
            "fac513d6d196e7a9ea445e98c30dc7d837063108c99902c4a2a88a5e08b3d8d9",
        "aot_source_schedule": True,
        "source_actions": (
            {"action": "dFoxMainMotion_TechB_0x418",
             "trigger_game_tick": 4, "call": "ftMotionPlayVoice"},
            {"action": "dFoxMainMotion_RollB",
             "trigger_game_tick": 4, "call": "ftMotionPlayVoice"},
        ),
        "source_action_file":
            "decomp/src/relocData/208_FoxMainMotion.c",
        "fidelity_debt": (),
    },
    {
        "id": 372,
        "name": "nSYAudioVoiceFoxSmash1",
        "kind": "voice",
        "articulation": 224,
        "sound": 105,
        "notes": ((13, 7, 46),),
        "duration_ticks": 46,
        "ucd_volume": 223,
        "articulation_pitch_cents": -1200,
        "loop": False,
        "wave_base": 913944,
        "wave_length": 946,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 1680,
        "root_fork_programs": (),
        "root_program_sha256":
            "a31ea9a2eab7861b43ed92cd237af6b19d19976aa34809f82aa3d56aad1d21d9",
        "render_program_sha256":
            "a31ea9a2eab7861b43ed92cd237af6b19d19976aa34809f82aa3d56aad1d21d9",
        "articulation_program_sha256":
            "df3b56b4f1866778aa5ebc10009959cc50f3e09d602ed8ae991d5fc091112224",
        "fidelity_debt": (),
    },
    {
        "id": 373,
        "name": "nSYAudioVoiceFoxSmash2",
        "kind": "voice",
        "articulation": 225,
        "sound": 106,
        "notes": ((13, 7, 30), (13, 7, 30)),
        "duration_ticks": 60,
        "ucd_volume": 255,
        "articulation_pitch_cents": -1200,
        "loop": False,
        "wave_base": 914896,
        "wave_length": 2746,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 4880,
        "root_fork_programs": (),
        "root_program_sha256":
            "31e0f930408f183574b932348e9f39249ca52bf77be66977d2eedfddd3fba7e3",
        "render_program_sha256":
            "31e0f930408f183574b932348e9f39249ca52bf77be66977d2eedfddd3fba7e3",
        "articulation_program_sha256":
            "2294d0495fe6cfefe4a4a8fef0dc424ab7c240b076f3f43548d9b1a4cabdd419",
        "fidelity_debt": (),
    },
    {
        "id": 374,
        "name": "nSYAudioVoiceFoxSmash3",
        "kind": "voice",
        "articulation": 226,
        "sound": 107,
        "notes": ((13, 7, 40), (13, 7, 46)),
        "duration_ticks": 86,
        "ucd_volume": 235,
        "articulation_pitch_cents": -1200,
        "loop": False,
        "wave_base": 917648,
        "wave_length": 4420,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 7856,
        "root_fork_programs": (),
        "root_program_sha256":
            "94945442f30c506fa46780c19c6219fb7ad77dc22ad7f703341cccc0adfc63c4",
        "render_program_sha256":
            "94945442f30c506fa46780c19c6219fb7ad77dc22ad7f703341cccc0adfc63c4",
        "articulation_program_sha256":
            "eebd142b7b78621d8940cf10d430d7084bef448db6cc3838bfcb5675878cdf46",
        "fidelity_debt": (),
    },
    {
        "id": 430,
        "name": "nSYAudioVoiceMarioSmash2",
        "kind": "voice",
        "articulation": 297,
        "sound": 174,
        "notes": ((13, 7, 4), (12, 7, 32), (11, 7, 50),
                  (10, 7, 150)),
        "duration_ticks": 236,
        "ucd_volume": 180,
        "articulation_pitch_cents": -1170,
        "loop": False,
        "wave_base": 1432240,
        "wave_length": 9694,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 17232,
        "root_fork_programs": (),
        "root_program_sha256":
            "95e58d8f6340197445020e8239b623ff469056e6fffa35eaab5763bb9b2a1a11",
        "render_program_sha256":
            "95e58d8f6340197445020e8239b623ff469056e6fffa35eaab5763bb9b2a1a11",
        "articulation_program_sha256":
            "ce7ca7fb5d393e272ce037e6929cb3d29e221112ad53093429b8d0d5808221f1",
        "fidelity_debt": ("ucd_pitch_automation",),
    },
    {
        "id": 439,
        "name": "nSYAudioVoiceMarioDead",
        "kind": "ko",
        "articulation": 306,
        "sound": 183,
        "notes": ((13, 7, 6), (13, 7, 20), (13, 7, 30),
                  (12, 7, 40)),
        "duration_ticks": 96,
        "ucd_volume": 200,
        "articulation_pitch_cents": -1199,
        "loop": False,
        "wave_base": 1502992,
        "wave_length": 5140,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 8838,
        "root_fork_programs": (),
        "root_program_sha256":
            "fe49ea59dc5b1286afefa3db0b6b71958ba1ff398b0558e3d959877000109914",
        "render_program_sha256":
            "fe49ea59dc5b1286afefa3db0b6b71958ba1ff398b0558e3d959877000109914",
        "articulation_program_sha256":
            "6c41de24317700de64f7999a9fc6945878b42f65fb55537f6ae3a6c689f99e23",
        "fidelity_debt": ("ucd_pitch_automation",),
    },
    {
        "id": 292,
        "name": "nSYAudioFGMMarioDeadSlam",
        "kind": "ko",
        "render_program": 287,
        "articulation": 187,
        "sound": 28,
        "notes": ((13, 7, 33), (13, 7, 20)),
        "duration_ticks": 53,
        "ucd_volume": 200,
        "articulation_pitch_cents": -1100,
        "loop": False,
        "wave_base": 251360,
        "wave_length": 3762,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 5168,
        "root_fork_programs": (287,),
        "root_program_sha256":
            "64523939186fd3d63f5440b5ec78784dac4e10c76456ebaa75671e4bfd9a85c2",
        "render_program_sha256":
            "634c9b1217b933f51dde97353d62e908fa1082943114d6dbe72bb188a3f33776",
        "articulation_program_sha256":
            "bbcff809d0113bec03d327dd08e85ef84fe10c8b18ba2f922b581416a958de0b",
        "fidelity_debt": ("articulation_volume_modulation",
                          "articulation_pitch_automation"),
    },
    {
        "id": 370,
        "name": "nSYAudioVoiceFoxDead",
        "kind": "ko",
        "articulation": 223,
        "sound": 104,
        "notes": ((13, 7, 50), (13, 7, 40), (13, 7, 30)),
        "duration_ticks": 120,
        "ucd_volume": 235,
        "articulation_pitch_cents": -1200,
        "loop": False,
        "wave_base": 908424,
        "wave_length": 5518,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 9808,
        "root_fork_programs": (),
        "root_program_sha256":
            "9ff31a2c34193eb957a4c9c258e07f6b901bd2c70102f6c0078a30d6b00fc3e4",
        "render_program_sha256":
            "9ff31a2c34193eb957a4c9c258e07f6b901bd2c70102f6c0078a30d6b00fc3e4",
        "articulation_program_sha256":
            "22022da7f182ddc58defdac1ae7411305109d7a9611bd0280c3ccc2573fd5807",
        "fidelity_debt": (),
    },
    {
        "id": 289,
        "name": "nSYAudioFGMFoxDeadSlam",
        "kind": "ko",
        "render_program": 287,
        "articulation": 187,
        "sound": 28,
        "notes": ((13, 7, 33), (13, 7, 20)),
        "duration_ticks": 53,
        "ucd_volume": 200,
        "articulation_pitch_cents": -1100,
        "loop": False,
        "wave_base": 251360,
        "wave_length": 3762,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 5168,
        "root_fork_programs": (287,),
        "root_program_sha256":
            "64523939186fd3d63f5440b5ec78784dac4e10c76456ebaa75671e4bfd9a85c2",
        "render_program_sha256":
            "634c9b1217b933f51dde97353d62e908fa1082943114d6dbe72bb188a3f33776",
        "articulation_program_sha256":
            "bbcff809d0113bec03d327dd08e85ef84fe10c8b18ba2f922b581416a958de0b",
        "fidelity_debt": ("articulation_volume_modulation",
                          "articulation_pitch_automation"),
    },
    {
        "id": 300,
        "name": "nSYAudioFGMFoxDownBounce",
        "kind": "movement",
        "render_program": 298,
        "articulation": 187,
        "sound": 28,
        "notes": ((12, 7, 10), (12, 7, 15)),
        "duration_ticks": 25,
        "ucd_volume": 130,
        "articulation_pitch_cents": -1100,
        "loop": False,
        "wave_base": 251360,
        "wave_length": 3762,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 2301,
        "root_fork_programs": (298,),
        "root_program_sha256":
            "0a7645ae1249ff5140ddbf80859b52c127b73d2b80e0b97d90cc3b61b0c4b262",
        "render_program_sha256":
            "9ed69d587dab562768d6321d349477c4f522c0b65115fb7cb2c1f27d5b27c4c2",
        "articulation_program_sha256":
            "bbcff809d0113bec03d327dd08e85ef84fe10c8b18ba2f922b581416a958de0b",
        "aot_modulator_index": 22,
        "aot_modulator": {
            "shape": 0,
            "target": 11,
            "postproc": 0,
            "init_phase": 49,
            "period": 100.0,
            "amplitude": 50.0,
            "offset": 50.0,
        },
        "aot_source_schedule": True,
        "source_actions": (
            {"action": "nFTCommonStatusDownBounceU",
             "trigger_game_tick": 0, "call": "func_800269C0_275C0"},
            {"action": "nFTCommonStatusDownBounceD",
             "trigger_game_tick": 0, "call": "func_800269C0_275C0"},
        ),
        "source_action_file":
            "decomp/src/ft/ftcommon/ftcommondownwaitbounce.c",
        "fidelity_debt": (),
    },
    {
        "id": 303,
        "name": "nSYAudioFGMMarioDownBounce",
        "kind": "movement",
        "render_program": 298,
        "articulation": 187,
        "sound": 28,
        "notes": ((12, 7, 10), (12, 7, 15)),
        "duration_ticks": 25,
        "ucd_volume": 130,
        "articulation_pitch_cents": -1100,
        "loop": False,
        "wave_base": 251360,
        "wave_length": 3762,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 2301,
        "root_fork_programs": (298,),
        "root_program_sha256":
            "0a7645ae1249ff5140ddbf80859b52c127b73d2b80e0b97d90cc3b61b0c4b262",
        "render_program_sha256":
            "9ed69d587dab562768d6321d349477c4f522c0b65115fb7cb2c1f27d5b27c4c2",
        "articulation_program_sha256":
            "bbcff809d0113bec03d327dd08e85ef84fe10c8b18ba2f922b581416a958de0b",
        "aot_modulator_index": 22,
        "aot_modulator": {
            "shape": 0,
            "target": 11,
            "postproc": 0,
            "init_phase": 49,
            "period": 100.0,
            "amplitude": 50.0,
            "offset": 50.0,
        },
        "aot_source_schedule": True,
        "source_actions": (
            {"action": "nFTCommonStatusDownBounceU",
             "trigger_game_tick": 0, "call": "func_800269C0_275C0"},
            {"action": "nFTCommonStatusDownBounceD",
             "trigger_game_tick": 0, "call": "func_800269C0_275C0"},
        ),
        "source_action_file":
            "decomp/src/ft/ftcommon/ftcommondownwaitbounce.c",
        "fidelity_debt": (),
    },
    {
        "id": 154,
        "name": "nSYAudioFGMDeadExplodeL",
        "kind": "ko",
        "articulation": 163,
        "sound": 0,
        "notes": ((2, 7, 200), (3, 7, 100)),
        "duration_ticks": 300,
        "ucd_volume": 255,
        "articulation_pitch_cents": -1200,
        "loop": False,
        "source_loop_infinite": True,
        "wave_base": 0,
        "wave_length": 14220,
        "loop_start": 20868,
        "loop_end": 25137,
        "expected_retained_samples": 14913,
        "root_fork_programs": (685,),
        "omitted_fork_programs": (685,),
        "root_program_sha256":
            "22c33d1163d54e9e661037c6850d401580fbe2690003d7db80a83b44560e7fdb",
        "render_program_sha256":
            "22c33d1163d54e9e661037c6850d401580fbe2690003d7db80a83b44560e7fdb",
        "omitted_fork_program_sha256": (
            "ee5e3c31780c8e09482ca18a29c429aeced10923e37da32c39fa0860c04f80c5",
        ),
        "articulation_program_sha256":
            "a6ebcc72a0293708770674b0e871961bf6a5223c52ae4d0b6d31ea993e8fb6b8",
        "fidelity_debt": ("ucd_pitch_automation", "omitted_fork_voice_685"),
    },
    {
        "id": 77,
        "name": "nSYAudioFGMMarioLanding",
        "kind": "mario",
        "render_program": 72,
        "articulation": 3,
        "sound": 1,
        "notes": ((8, 7, 3),),
        "duration_ticks": 3,
        "ucd_volume": 180,
        "articulation_pitch_cents": 700,
        "loop": False,
        "wave_base": 14224,
        "wave_length": 2944,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 621,
        "root_fork_programs": (72,),
        "root_program_sha256":
            "c5fb3a31fc2383118516512dda33fdd0f670a91a490bfc333c27b21e66d6f4a0",
        "render_program_sha256":
            "9b37506dc57cc43b255fa175bfb1e9256fc4c955ae00e4bd600bf4ab123781cf",
        "articulation_program_sha256":
            "300492238b0d3e3b82ac86f63da05c445083fe1aafa2a6d10d7b4bf4f59b7576",
        "aot_source_schedule": True,
        "fidelity_debt": (),
    },
)

# These source cues are deliberately audited but not packed.  Each retunes an
# already-running voice after playback starts; the current DS entry format has
# no frequency schedule, so first-note playback would be behaviorally wrong.
EXCLUDED_SOURCE_CUES = (
    {
        "id": 375,
        "name": "nSYAudioVoiceFoxDamage",
        "kind": "voice",
        "articulation": 222,
        "sound": 103,
        "notes": ((13, 7, 40), (12, 7, 35)),
        "duration_ticks": 75,
        "ucd_volume": 215,
        "articulation_pitch_cents": -1200,
        "loop": False,
        "wave_base": 904840,
        "wave_length": 3582,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 6368,
        "root_fork_programs": (),
        "root_program_sha256":
            "5b8f4ad012a8f6bcfc306b797ca1d40ca61f6b4a14e0149d59f89a9ef0707f82",
        "render_program_sha256":
            "5b8f4ad012a8f6bcfc306b797ca1d40ca61f6b4a14e0149d59f89a9ef0707f82",
        "articulation_program_sha256":
            "96c8e7d9f930325621d2561698a1a711ab95ad24e2c51b9a80d7713111168afe",
        "fidelity_debt": ("ucd_pitch_automation",),
        "exclusion_reason":
            "continuous_voice_pitch_schedule_not_representable",
    },
    {
        "id": 429,
        "name": "nSYAudioVoiceMarioSmash1",
        "kind": "voice",
        "articulation": 296,
        "sound": 173,
        "notes": ((12, 7, 6), (12, 7, 20), (11, 7, 30),
                  (10, 7, 40)),
        "duration_ticks": 96,
        "ucd_volume": 200,
        "articulation_pitch_cents": -1199,
        "loop": False,
        "wave_base": 1430184,
        "wave_length": 2052,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 3648,
        "root_fork_programs": (),
        "root_program_sha256":
            "305f617ac74e0c0d714d7a107d18d481bdfbd8d0c26d29f1dd1d00593f104269",
        "render_program_sha256":
            "305f617ac74e0c0d714d7a107d18d481bdfbd8d0c26d29f1dd1d00593f104269",
        "articulation_program_sha256":
            "213f87188a79bd2c5ac6c58d1162a54912cc95f33afaa742be3c35f71311c2a3",
        "fidelity_debt": ("ucd_pitch_automation",),
        "exclusion_reason":
            "continuous_voice_pitch_schedule_not_representable",
    },
    {
        "id": 431,
        "name": "nSYAudioVoiceMarioSmash3",
        "kind": "voice",
        "articulation": 298,
        "sound": 175,
        "notes": ((12, 7, 6), (12, 7, 20), (11, 7, 30),
                  (10, 7, 40)),
        "duration_ticks": 96,
        "ucd_volume": 180,
        "articulation_pitch_cents": -1199,
        "loop": False,
        "wave_base": 1441936,
        "wave_length": 2206,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 3920,
        "root_fork_programs": (),
        "root_program_sha256":
            "ffccc361f7892f37fde0c9c9c4697aeeb957f4db25982a1d0e0e8461f3a2b111",
        "render_program_sha256":
            "ffccc361f7892f37fde0c9c9c4697aeeb957f4db25982a1d0e0e8461f3a2b111",
        "articulation_program_sha256":
            "05da0a2d82e126953a3a3f6fb217fe045dbab46c5c21db52d3addf5e0f4e94a1",
        "fidelity_debt": ("ucd_pitch_automation",),
        "exclusion_reason":
            "continuous_voice_pitch_schedule_not_representable",
    },
    {
        "id": 435,
        "name": "nSYAudioVoiceMarioJump",
        "kind": "voice",
        "articulation": 302,
        "sound": 179,
        "notes": ((12, 7, 6), (12, 7, 20), (12, 7, 30),
                  (9, 7, 40)),
        "duration_ticks": 96,
        "ucd_volume": 200,
        "articulation_pitch_cents": -1160,
        "loop": False,
        "wave_base": 1482688,
        "wave_length": 1882,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 3344,
        "root_fork_programs": (),
        "root_program_sha256":
            "820e3ad7c1f3c07e61e0a3047e722314a89aac24ca934fd3c3dae8af1ef3e866",
        "render_program_sha256":
            "820e3ad7c1f3c07e61e0a3047e722314a89aac24ca934fd3c3dae8af1ef3e866",
        "articulation_program_sha256":
            "4fc9031eda94767f7e8f989a5cf9bf02f5ac1091519ac817bd22da87e6fc34d2",
        "fidelity_debt": ("ucd_pitch_automation",),
        "exclusion_reason":
            "combined_ucd_and_articulation_pitch_schedule_not_representable",
    },
    {
        "id": 440,
        "name": "nSYAudioVoiceMarioDamage",
        "kind": "voice",
        "articulation": 307,
        "sound": 184,
        "notes": ((13, 7, 6), (13, 7, 20), (12, 7, 50),
                  (12, 7, 40)),
        "duration_ticks": 116,
        "ucd_volume": 190,
        "articulation_pitch_cents": -1199,
        "loop": False,
        "wave_base": 1508136,
        "wave_length": 5544,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 9856,
        "root_fork_programs": (),
        "root_program_sha256":
            "845b4218667280d1bede59a6a651ce370b09ab652e2f68b15246138fd5b4b596",
        "render_program_sha256":
            "845b4218667280d1bede59a6a651ce370b09ab652e2f68b15246138fd5b4b596",
        "articulation_program_sha256":
            "3c9d6e4bd2d00ea0dc117b5757a516271097d7189bee6d2e33fae826d6981e80",
        "fidelity_debt": ("ucd_pitch_automation",),
        "exclusion_reason":
            "continuous_voice_pitch_schedule_not_representable",
    },
)

SELECTED += (
    {
        "id": 215,
        "name": "nSYAudioFGMMarioSpecialN",
        "kind": "attack",
        "articulation": 42,
        "sound": 19,
        "notes": ((13, 7, 5), (13, 7, 10)),
        "duration_ticks": 15,
        "ucd_volume": 250,
        "articulation_pitch_cents": 0,
        "loop": False,
        "wave_base": 191464,
        "wave_length": 1224,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 2176,
        "root_fork_programs": (),
        "root_program_sha256":
            "c9f584ac64297bfca52605e5bd01c3d42a31126f7d6e3e73cc4e65b9743cc6ac",
        "render_program_sha256":
            "c9f584ac64297bfca52605e5bd01c3d42a31126f7d6e3e73cc4e65b9743cc6ac",
        "articulation_program_sha256":
            "78e320e6ee2a2832cb2f3635016b5b46d13fa820dccf4651d7effcd36ee5c7dd",
        "fidelity_debt": (),
    },
    {
        "id": 40,
        "name": "nSYAudioFGMPunchS",
        "kind": "hit",
        "action_contract": "fighter punch-kind small collision",
        "source_callsites": (
            "ftmain.c:dFTMainHitCollisionFGMs[punch][small]",
            "ftmain.c:ftMainPlayHitSFX->lbCommonMakePositionFGM",
        ),
        "source_pan_behavior": "attacker TopN x through lbCommonMakePositionFGM",
        "articulation": 31,
        "sound": 11,
        "notes": ((12, 7, 48), (12, 7, 20)),
        "duration_ticks": 68,
        "ucd_volume": 170,
        "articulation_pitch_cents": -300,
        "loop": False,
        "wave_base": 120848,
        "wave_length": 9234,
        "loop_start": 0,
        "loop_end": 0,
        "retain_full_source": True,
        "expected_retained_samples": 16416,
        "root_fork_programs": (655,),
        "omitted_fork_programs": (655,),
        "root_program_sha256":
            "28f536fe3e90955bbedb7cddaf62f405433d5e8eab4ea3c501988d4b56c5b4a3",
        "render_program_sha256":
            "28f536fe3e90955bbedb7cddaf62f405433d5e8eab4ea3c501988d4b56c5b4a3",
        "omitted_fork_program_sha256": (
            "75fe85b876585bbdd7b5688160d81e53d77fe6068fee46531d49641cd2540f40",
        ),
        "articulation_program_sha256":
            "783c596ffcb39d9338e3e3ab7fb99db0cb7751cac24ecdf841b8e2b043417298",
        "fidelity_debt": ("ucd_volume_automation",
                          "omitted_fork_voice_655"),
    },
    {
        "id": 38,
        "name": "nSYAudioFGMPunchM",
        "kind": "hit",
        "action_contract": "fighter punch-kind medium collision",
        "source_callsites": (
            "ftmain.c:dFTMainHitCollisionFGMs[punch][medium]",
            "ftmain.c:ftMainPlayHitSFX->lbCommonMakePositionFGM",
        ),
        "source_pan_behavior": "attacker TopN x through lbCommonMakePositionFGM",
        "articulation": 29,
        "sound": 11,
        "notes": ((14, 7, 48), (13, 7, 48), (13, 7, 15)),
        "duration_ticks": 111,
        "ucd_volume": 250,
        "articulation_pitch_cents": 50,
        "loop": False,
        "wave_base": 120848,
        "wave_length": 9234,
        "loop_start": 0,
        "loop_end": 0,
        "retain_full_source": True,
        "expected_retained_samples": 16416,
        "root_fork_programs": (654,),
        "omitted_fork_programs": (654,),
        "root_program_sha256":
            "d04ce6abe4111930deb668c4cfc78e8a1e82462f2935e6978eef88661e3d1639",
        "render_program_sha256":
            "d04ce6abe4111930deb668c4cfc78e8a1e82462f2935e6978eef88661e3d1639",
        "omitted_fork_program_sha256": (
            "a287c6195f269a6729a74db38a6aa2f3b707a19d44ffba4ef4f21750530daee5",
        ),
        "articulation_program_sha256":
            "580678b358c9a14cd9879a965edbeb70009f96155be3e041b7d819cc882bcc63",
        "fidelity_debt": ("ucd_pitch_automation", "ucd_volume_automation",
                          "articulation_pitch_modulation",
                          "omitted_fork_voice_654"),
    },
    {
        "id": 37,
        "name": "nSYAudioFGMPunchL",
        "kind": "hit",
        "action_contract": "fighter punch-kind large collision",
        "source_callsites": (
            "ftmain.c:dFTMainHitCollisionFGMs[punch][large]",
            "ftmain.c:ftMainPlayHitSFX->lbCommonMakePositionFGM",
        ),
        "source_pan_behavior": "attacker TopN x through lbCommonMakePositionFGM",
        "articulation": 30,
        "sound": 11,
        "notes": ((16, 7, 45), (16, 7, 45), (15, 7, 45),
                  (15, 7, 20)),
        "duration_ticks": 155,
        "ucd_volume": 255,
        "articulation_pitch_cents": 300,
        "loop": False,
        "wave_base": 120848,
        "wave_length": 9234,
        "loop_start": 0,
        "loop_end": 0,
        "retain_full_source": True,
        "expected_retained_samples": 16416,
        "root_fork_programs": (653,),
        "omitted_fork_programs": (653,),
        "root_program_sha256":
            "80d72e74f0d4de80cb03e4a78f45907892bf5ebf1e464394917a7157337fd249",
        "render_program_sha256":
            "80d72e74f0d4de80cb03e4a78f45907892bf5ebf1e464394917a7157337fd249",
        "omitted_fork_program_sha256": (
            "450624587294d4be7b473cfba7223e1a5cbdfe3bfe45b630fe96bac4ce7a17ed",
        ),
        "articulation_program_sha256":
            "4d3bbb2b0f80ef41f06f9c4812a795e6a12989855c7f5320fd1e5d7f957cfd27",
        "fidelity_debt": ("ucd_pitch_automation", "ucd_volume_automation",
                          "articulation_pitch_modulation",
                          "omitted_fork_voice_653"),
    },
    {
        "id": 34,
        "name": "nSYAudioFGMKickS",
        "kind": "hit",
        "action_contract": "fighter kick-kind small collision",
        "source_callsites": (
            "ftmain.c:dFTMainHitCollisionFGMs[kick][small]",
            "ftmain.c:ftMainPlayHitSFX->lbCommonMakePositionFGM",
        ),
        "source_pan_behavior": "attacker TopN x through lbCommonMakePositionFGM",
        "articulation": 34,
        "sound": 16,
        "notes": ((6, 7, 50), (6, 7, 20)),
        "duration_ticks": 70,
        "ucd_volume": 190,
        "articulation_pitch_cents": -888,
        "loop": False,
        "wave_base": 165960,
        "wave_length": 6310,
        "loop_start": 0,
        "loop_end": 0,
        "retain_full_source": True,
        "expected_retained_samples": 11216,
        "root_fork_programs": (658,),
        "omitted_fork_programs": (658,),
        "root_program_sha256":
            "9b0c4611ee9ce3034b129dd41cd3e88fa5bacf7fb7e797e6872aac0bc94fade5",
        "render_program_sha256":
            "9b0c4611ee9ce3034b129dd41cd3e88fa5bacf7fb7e797e6872aac0bc94fade5",
        "omitted_fork_program_sha256": (
            "75fe85b876585bbdd7b5688160d81e53d77fe6068fee46531d49641cd2540f40",
        ),
        "articulation_program_sha256":
            "a83fb7f2cb1c8190e22fe4cd76afc24697de9041d7b0729b5ffc226d67a5bfcd",
        "fidelity_debt": ("ucd_volume_automation",
                          "omitted_fork_voice_658"),
    },
    {
        "id": 32,
        "name": "nSYAudioFGMKickM",
        "kind": "hit",
        "action_contract": "fighter kick-kind medium collision",
        "source_callsites": (
            "ftmain.c:dFTMainHitCollisionFGMs[kick][medium]",
            "ftmain.c:ftMainPlayHitSFX->lbCommonMakePositionFGM",
        ),
        "source_pan_behavior": "attacker TopN x through lbCommonMakePositionFGM",
        "articulation": 35,
        "sound": 16,
        "notes": ((8, 7, 48), (8, 7, 48), (8, 7, 15)),
        "duration_ticks": 111,
        "ucd_volume": 230,
        "articulation_pitch_cents": -900,
        "loop": False,
        "wave_base": 165960,
        "wave_length": 6310,
        "loop_start": 0,
        "loop_end": 0,
        "retain_full_source": True,
        "expected_retained_samples": 11216,
        "root_fork_programs": (657,),
        "omitted_fork_programs": (657,),
        "root_program_sha256":
            "420b243a1616819719fac1e41e538c938049be781dffaa397560811b07d2cf24",
        "render_program_sha256":
            "420b243a1616819719fac1e41e538c938049be781dffaa397560811b07d2cf24",
        "omitted_fork_program_sha256": (
            "91c40dab5937cf9d5e3e9c50bf838d662061e9ffe2e9f9a5a7fcb63a2978afaa",
        ),
        "articulation_program_sha256":
            "1ee86a59e8fa37a8d78be0a286f908f67f2a44018dc39863444f52222cb47b9c",
        "fidelity_debt": ("ucd_volume_automation",
                          "omitted_fork_voice_657"),
    },
    {
        "id": 31,
        "name": "nSYAudioFGMKickL",
        "kind": "hit",
        "action_contract": "fighter kick-kind large collision",
        "source_callsites": (
            "ftmain.c:dFTMainHitCollisionFGMs[kick][large]",
            "ftmain.c:ftMainPlayHitSFX->lbCommonMakePositionFGM",
        ),
        "source_pan_behavior": "attacker TopN x through lbCommonMakePositionFGM",
        "articulation": 33,
        "sound": 16,
        "notes": ((10, 7, 44), (10, 7, 45), (10, 7, 45),
                  (10, 7, 17)),
        "duration_ticks": 151,
        "ucd_volume": 255,
        "articulation_pitch_cents": -900,
        "loop": False,
        "wave_base": 165960,
        "wave_length": 6310,
        "loop_start": 0,
        "loop_end": 0,
        "retain_full_source": True,
        "expected_retained_samples": 11216,
        "root_fork_programs": (656,),
        "omitted_fork_programs": (656,),
        "root_program_sha256":
            "11c5a67bd6baa8730dace7c881508222177acb1d710670d4efeee4671a602c0c",
        "render_program_sha256":
            "11c5a67bd6baa8730dace7c881508222177acb1d710670d4efeee4671a602c0c",
        "omitted_fork_program_sha256": (
            "eee8f09b8d3cdc1b0a04eb7c2563b74039d51fe6fba2fd0d302a41da9f6cc272",
        ),
        "articulation_program_sha256":
            "566a178f2e69b1e045900a590c78e4064780a57cca2864e5d83ceb0472f9c725",
        "fidelity_debt": ("ucd_volume_automation",
                          "omitted_fork_voice_656"),
    },
    {
        "id": 216,
        "name": "nSYAudioFGMMarioSpecialHiCoin",
        "kind": "hit",
        "runtime_excluded": True,
        "action_contract": "Mario special-hi coin-kind fighter collision",
        "source_callsites": (
            "ftmain.c:dFTMainHitCollisionFGMs[coin][all-levels]",
            "ftmain.c:ftMainPlayHitSFX->lbCommonMakePositionFGM",
        ),
        "source_pan_behavior": "attacker TopN x through lbCommonMakePositionFGM",
        "runtime_excluded_reasons": (
            "source_composite_fork_not_rendered",
            "source_note_stop_live_pitch_and_loop_behavior_not_rendered",
            "fork_668_initial_65875_hz_exceeds_ds_u16_frequency",
        ),
        "articulation": 51,
        "sound": 22,
        "notes": ((12, 7, 10), (0, 7, 2), (17, 7, 40),
                  (17, 7, 40)),
        "duration_ticks": 92,
        "ucd_volume": 170,
        "articulation_pitch_cents": 0,
        "loop": True,
        "wave_base": 199064,
        "wave_length": 918,
        "loop_start": 10,
        "loop_end": 1625,
        "render_source_loop": True,
        "expected_retained_samples": 20458,
        "root_fork_programs": (668,),
        "omitted_fork_programs": (668,),
        "root_program_sha256":
            "27f9f2691fd4c5b25cfac411a75270e418990e5719788f71f7dd020ea8a7816f",
        "render_program_sha256":
            "27f9f2691fd4c5b25cfac411a75270e418990e5719788f71f7dd020ea8a7816f",
        "omitted_fork_program_sha256": (
            "ff46bd44f488c9b61ae7df69ab8a6706c8638e86a4d19739425de35f533efc82",
        ),
        "articulation_program_sha256":
            "aaad5c2c434005ef3ce87adead9c89da0bbfdd883488b0e59594a8b145295e9f",
        "fidelity_debt": ("ucd_pitch_automation",
                          "omitted_fork_voice_668"),
    },
    {
        "id": 28,
        "name": "nSYAudioFGMBurnS",
        "kind": "hit",
        "runtime_excluded": True,
        "action_contract": "Mario Fireball weapon collision BurnS component",
        "source_callsites": (
            "204_MarioSpecial1.c:fireball weapon attribute sfx=28",
            "ftmain.c:ftMainUpdateDamageStatWeapon->func_800269C0_275C0",
            "wpmariofireball.c:wpMarioFireballProcHit also starts ID 0",
        ),
        "source_pan_behavior": "centered func_800269C0_275C0 weapon hit",
        "runtime_excluded_reasons": (
            "source_loop_and_envelope_not_aot_rendered",
            "paired_fireball_proc_hit_id_0_is_not_behavior_exact",
        ),
        "articulation": 65,
        "sound": 27,
        "notes": ((13, 7, 50), (13, 7, 40), (13, 7, 70)),
        "duration_ticks": 160,
        "ucd_volume": 150,
        "articulation_pitch_cents": 0,
        "loop": True,
        "wave_base": 235736,
        "wave_length": 15624,
        "loop_start": 13840,
        "loop_end": 27456,
        "render_source_loop": True,
        "expected_retained_samples": 29441,
        "root_fork_programs": (),
        "root_program_sha256":
            "e2709974888dccc4920fcf1bd21bdb6171ffd8f38846b38f516f21aa4adfa298",
        "render_program_sha256":
            "e2709974888dccc4920fcf1bd21bdb6171ffd8f38846b38f516f21aa4adfa298",
        "articulation_program_sha256":
            "39cd6d6668a90146cdef1f14fcfd817aa715ee258af68599973edfe8b4d3e425",
        "fidelity_debt": (),
    },
    {
        "id": 2,
        "name": "nSYAudioFGMFireShoot1",
        "kind": "hit",
        "runtime_excluded": True,
        "action_contract": "Fox Blaster weapon collision",
        "source_callsites": (
            "210_FoxSpecial1.c:blaster weapon attribute sfx=2",
            "ftmain.c:ftMainUpdateDamageStatWeapon->func_800269C0_275C0",
        ),
        "source_pan_behavior": "centered func_800269C0_275C0 weapon hit",
        "runtime_excluded_reasons": (
            "source_envelope_not_aot_rendered",
            "source_custom_fx_route_not_rendered",
        ),
        "articulation": 7,
        "sound": 4,
        "notes": ((18, 7, 50), (18, 7, 140)),
        "duration_ticks": 190,
        "ucd_volume": 200,
        "articulation_pitch_cents": 300,
        "loop": False,
        "wave_base": 28560,
        "wave_length": 17046,
        "loop_start": 0,
        "loop_end": 0,
        "retain_full_source": True,
        "expected_retained_samples": 30304,
        "root_fork_programs": (),
        "root_program_sha256":
            "a79a589363657a4069e95f2b5d8d1f1cb589f17aa4d87ba8021c2a16e0063746",
        "render_program_sha256":
            "a79a589363657a4069e95f2b5d8d1f1cb589f17aa4d87ba8021c2a16e0063746",
        "articulation_program_sha256":
            "25baf51195b0172ac10261cc3368f6fac20147a94e42f2ca777aab29ab13a6b3",
        "fidelity_debt": (),
    },
    {
        "id": 0,
        "name": "nSYAudioFGMExplodeS",
        "kind": "hit",
        "runtime_excluded": True,
        "action_contract": "Mario Fireball proc-hit/shield/setoff/absorb explosion",
        "source_callsites": (
            "wpmariofireball.c:wpMarioFireballProcHit->func_800269C0_275C0(0)",
        ),
        "source_pan_behavior": "centered func_800269C0_275C0 proc hit",
        "runtime_excluded_reasons": (
            "source_envelope_not_aot_rendered",
            "source_custom_fx_route_not_rendered",
        ),
        "articulation": 7,
        "sound": 4,
        "notes": ((3, 7, 20), (3, 7, 30), (3, 7, 85)),
        "duration_ticks": 135,
        "ucd_volume": 220,
        "articulation_pitch_cents": 300,
        "loop": False,
        "wave_base": 28560,
        "wave_length": 17046,
        "loop_start": 0,
        "loop_end": 0,
        "retain_full_source": True,
        "expected_retained_samples": 30304,
        "root_fork_programs": (),
        "root_program_sha256":
            "7874ec9371696e630f3e27f81fa10ff9661013ada5c1b880b5f3fdfe054d5a36",
        "render_program_sha256":
            "7874ec9371696e630f3e27f81fa10ff9661013ada5c1b880b5f3fdfe054d5a36",
        "articulation_program_sha256":
            "25baf51195b0172ac10261cc3368f6fac20147a94e42f2ca777aab29ab13a6b3",
        "fidelity_debt": (),
    },
    {
        "id": 188,
        "name": "nSYAudioFGMFoxSpecialLwHit",
        "kind": "hit_inventory",
        "runtime_excluded": True,
        "action_contract": "Fox reflector successful-reflect motion cue",
        "source_callsites": (
            "208_FoxMainMotion.c:dFoxMainMotion_Reflecting->ftMotionPlayFGM(188)",
        ),
        "source_pan_behavior": "centered fighter motion FGM",
        "runtime_excluded_reasons": (
            "no_natural_mode_163_call_observed",
            "source_live_pitch_and_envelope_not_aot_rendered",
            "source_custom_fx_route_not_rendered",
        ),
        "articulation": 45,
        "sound": 12,
        "notes": ((18, 7, 1), (20, 7, 1), (12, 7, 2),
                  (15, 7, 2), (18, 7, 1), (15, 7, 2),
                  (8, 7, 1), (8, 7, 2), (11, 7, 2),
                  (13, 7, 2), (15, 7, 6)),
        "duration_ticks": 22,
        "ucd_volume": 255,
        "articulation_pitch_cents": -1200,
        "loop": True,
        "wave_base": 130088,
        "wave_length": 4320,
        "loop_start": 100,
        "loop_end": 7664,
        "expected_retained_samples": 7680,
        "root_fork_programs": (),
        "root_program_sha256":
            "a974ed1ff5e44afae9c6dc701e489ed7836dcb70a11b36c244b171782a5f26e5",
        "render_program_sha256":
            "a974ed1ff5e44afae9c6dc701e489ed7836dcb70a11b36c244b171782a5f26e5",
        "articulation_program_sha256":
            "a58c3cff3972a3140b9a507942634ac9d2df4eb6b310e9e01c0db0797ef852ae",
        "fidelity_debt": (),
    },
)

IMA_INDEX_TABLE = (
    -1, -1, -1, -1, 2, 4, 6, 8,
    -1, -1, -1, -1, 2, 4, 6, 8,
)
IMA_STEP_TABLE = (
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31,
    34, 37, 41, 45, 50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 130,
    143, 157, 173, 190, 209, 230, 253, 279, 307, 337, 371, 408, 449,
    494, 544, 598, 658, 724, 796, 876, 963, 1060, 1166, 1282, 1411,
    1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024, 3327, 3660, 4026,
    4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442,
    11487, 12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623,
    27086, 29794, 32767,
)


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def json_sha256(value) -> str:
    encoded = json.dumps(value, sort_keys=True,
                         separators=(",", ":")).encode("utf-8")
    return sha256(encoded)


def load_module(path: Path, name: str):
    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot import {path}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def read_o2r_payload(path: Path) -> tuple[bytes, bytes]:
    wrapped = path.read_bytes()
    if len(wrapped) < 0x44:
        raise ValueError(f"short O2R wrapper: {path}")
    raw_size = struct.unpack_from("<I", wrapped, 0x40)[0]
    payload = wrapped[0x44:]
    if len(payload) != raw_size:
        raise ValueError(
            f"O2R raw-size mismatch for {path}: {len(payload)} != {raw_size}")
    return wrapped, payload


def first_program_arg(program: list[list], op: str) -> int:
    values = [int(row[1]) for row in program if row[0] == op]
    if len(values) != 1:
        raise ValueError(f"expected one {op}, found {len(values)}")
    return values[0]


def validate_ucd(root_program: list[list], program: list[list],
                 selector: dict) -> None:
    expected_root_hash = selector.get("root_program_sha256")
    if (expected_root_hash is not None and
            json_sha256(root_program) != expected_root_hash):
        raise ValueError(f"FGM {selector['id']} root UCD program changed")
    expected_render_hash = selector.get("render_program_sha256")
    if (expected_render_hash is not None and
            json_sha256(program) != expected_render_hash):
        raise ValueError(f"FGM {selector['id']} render UCD program changed")

    root_forks = tuple(int(row[1]) for row in root_program
                       if row[0] == "fork_voice")
    if root_forks != tuple(selector.get("root_fork_programs", ())):
        raise ValueError(
            f"FGM {selector['id']} root forks changed: {root_forks}")
    omitted_forks = tuple(selector.get("omitted_fork_programs", ()))
    omitted_hashes = tuple(selector.get(
        "omitted_fork_program_sha256", ()))
    if len(omitted_forks) != len(omitted_hashes):
        raise ValueError(f"FGM {selector['id']} omitted-fork fixture mismatch")

    if expected_render_hash is None:
        forbidden = {"fork_voice", "mark_loop", "jump_loop", "vol_delta",
                     "pan_delta", "set_t5_neg2400", "set_t5_neg4800"}
        present = {row[0] for row in program}
        if present & forbidden:
            raise ValueError(
                f"FGM {selector['id']} is no longer a bounded voice: "
                f"{sorted(present & forbidden)}")
    if first_program_arg(program, "set_articulation") != selector["articulation"]:
        raise ValueError(f"FGM {selector['id']} articulation changed")
    volumes = [int(row[1]) for row in program if row[0] == "set_volume"]
    if not volumes or volumes[0] != selector["ucd_volume"]:
        raise ValueError(f"FGM {selector['id']} volume changed")
    notes = [row for row in program if row[0] == "note"]
    if "notes" in selector:
        expected_notes = selector["notes"]
    else:
        expected_notes = ((selector["pitch_code"], 7,
                           selector["duration_ticks"]),)
    expected_notes = [["note", *values] for values in expected_notes]
    if notes != expected_notes:
        raise ValueError(f"FGM {selector['id']} note program changed: {notes}")
    if not program or program[-1] != ["stop_voice"]:
        raise ValueError(f"FGM {selector['id']} no longer ends in stop_voice")


def validate_articulation(program: list[list], selector: dict) -> None:
    expected_hash = selector.get("articulation_program_sha256")
    if expected_hash is not None and json_sha256(program) != expected_hash:
        raise ValueError(
            f"FGM {selector['id']} articulation program changed")
    triggers = [int(row[1]) for row in program if row[0] == "trigger"]
    if triggers != [selector["sound"]]:
        raise ValueError(
            f"FGM {selector['id']} trigger changed: {triggers}")
    pitches = [int(row[1]) for row in program if row[0] == "pitch"]
    if (not pitches or
            pitches[0] != selector["articulation_pitch_cents"]):
        raise ValueError(f"FGM {selector['id']} pitch changed: {pitches}")
    if expected_hash is None:
        unsupported = {row[0] for row in program} - {
            "trigger", "pitch", "unk36", "vol", "end"
        }
        if unsupported:
            raise ValueError(
                f"FGM {selector['id']} articulation gained unsupported ops: "
                f"{sorted(unsupported)}")
    if not program or program[-1][0] != "end":
        raise ValueError(f"FGM {selector['id']} articulation has no end")


def fgm_voice_source_audit(program_id: int, ucd: dict,
                           articulations: dict, instrument: dict,
                           ctl_by_offset: dict, source_tbl: bytes,
                           audio_codec) -> dict:
    program = ucd["entries"][program_id]["program"]
    articulation_id = first_program_arg(program, "set_articulation")
    articulation = articulations["entries"][articulation_id]["program"]
    sound_id = first_program_arg(articulation, "trigger")
    sound_offset = instrument["soundArray_offs"][sound_id]
    sound = ctl_by_offset[sound_offset]
    wave = ctl_by_offset[sound["wavetable_off"]]
    book = ctl_by_offset[wave["book_off"]]
    loop = ctl_by_offset[wave["loop_off"]] if wave["loop_off"] else None
    vadpcm = source_tbl[wave["base"]:wave["base"] + wave["length"]]
    frame_bytes = len(vadpcm) - (len(vadpcm) % 9)
    pcm = audio_codec.adpcm_decode(
        vadpcm[:frame_bytes], book["entries"], book["order"],
        book["npredictors"])

    root_volume = 255
    root_fx_scale = 64
    root_pan = 64
    cut_before_note_end = False
    note_tick = 0
    previous_duration = None
    previous_cut_before_note_end = False
    articulation_pitches = [int(row[1]) for row in articulation
                            if row[0] == "pitch"]
    if not articulation_pitches:
        raise ValueError(f"FGM voice {program_id} has no articulation pitch")
    initial_articulation_pitch = articulation_pitches[0]
    note_schedule = []
    root_volume_schedule = []
    root_pan_schedule = []
    for row in program:
        if row[0] == "set_unk1E":
            cut_before_note_end = (int(row[1]) & 0x80) != 0
        elif row[0] == "set_volume":
            root_volume = int(row[1])
            root_volume_schedule.append({"tick": note_tick,
                                         "value": root_volume})
        elif row[0] == "vol_delta":
            root_volume = max(0, min(255, root_volume + int(row[1])))
            root_volume_schedule.append({"tick": note_tick,
                                         "value": root_volume})
        elif row[0] == "set_unk2C":
            root_fx_scale = int(row[1])
        elif row[0] == "unk2C_delta":
            root_fx_scale = max(
                0, min(127, root_fx_scale + int(row[1])))
        elif row[0] == "set_pan":
            root_pan = int(row[1])
            root_pan_schedule.append({"tick": note_tick,
                                      "value": root_pan})
        elif row[0] == "pan_delta":
            root_pan = max(0, min(127, root_pan + int(row[1])))
            root_pan_schedule.append({"tick": note_tick,
                                      "value": root_pan})
        elif row[0] == "note":
            duration = int(row[3])
            pitch_code = int(row[1])
            starts_new_voice = (
                previous_duration is None or
                (previous_cut_before_note_end and previous_duration > 1))
            note_schedule.append({
                "start_tick": note_tick,
                "duration_ticks": duration,
                "pitch_code": pitch_code,
                "root_volume": root_volume,
                "starts_new_voice": starts_new_voice,
                "cut_before_note_end": cut_before_note_end,
                "initial_frequency_hz": note_frequency_hz(
                    initial_articulation_pitch, pitch_code),
                "release_ramp_start_tick": (
                    note_tick + duration - 1
                    if cut_before_note_end and duration > 1 else None),
            })
            note_tick += duration
            previous_duration = duration
            previous_cut_before_note_end = cut_before_note_end

    articulation_tick = 0
    pitch_schedule = []
    volume_schedule = []
    fx_mix_schedule = []
    modulation_schedule = []
    articulation_fx = 0
    for row in articulation:
        op = row[0]
        if op == "pitch":
            pitch_schedule.append({"tick": articulation_tick,
                                   "cents": int(row[1])})
        elif op == "vol":
            volume_schedule.append({"tick": articulation_tick,
                                    "value": int(row[1])})
        elif op == "unk36":
            value = int(row[1])
            articulation_fx = (value if value <= 127 else
                               max(0, min(127,
                                          articulation_fx + value - 192)))
            fx_mix_schedule.append({
                "tick": articulation_tick,
                "articulation_unk36": articulation_fx,
                "root_unk2c": root_fx_scale,
                "effective_fx_mix": (
                    articulation_fx * (root_fx_scale >> 1) >> 7),
            })
        elif op in ("spawn_mod", "stop_mod"):
            modulation_schedule.append({"tick": articulation_tick,
                                        "command": row[:-1]})
        articulation_tick += int(row[-1])

    loop_audit = None
    if loop is not None:
        loop_audit = {
            "start": loop["start"],
            "end": loop["end"],
            "count": loop["count"],
            "count_signed": -1 if loop["count"] == 0xffffffff else
                loop["count"],
            "state": loop.get("state", []),
            "state_sha256": json_sha256(loop.get("state", [])),
        }
    return {
        "program_id": program_id,
        "ucd_program": program,
        "ucd_program_sha256": json_sha256(program),
        "fork_program_ids": [int(row[1]) for row in program
                             if row[0] == "fork_voice"],
        "root_volume_schedule": root_volume_schedule,
        "root_pan_schedule": root_pan_schedule,
        "root_fx_scale_unk2c": root_fx_scale,
        "cut_before_note_end": cut_before_note_end,
        "note_schedule": note_schedule,
        "articulation_id": articulation_id,
        "articulation_program": articulation,
        "articulation_program_sha256": json_sha256(articulation),
        "articulation_pitch_schedule": pitch_schedule,
        "articulation_volume_schedule": volume_schedule,
        "articulation_fx_mix_schedule": fx_mix_schedule,
        "articulation_modulation_schedule": modulation_schedule,
        "requires_custom_fx": any(
            point["effective_fx_mix"] != 0 for point in fx_mix_schedule),
        "sound_id": sound_id,
        "sound_offset": sound_offset,
        "sound_sample_pan_ignored_by_fgm": sound["samplePan"],
        "sound_sample_volume_ignored_by_fgm": sound["sampleVolume"],
        "wave_base": wave["base"],
        "wave_length": wave["length"],
        "source_vadpcm_frame_bytes": frame_bytes,
        "source_vadpcm_trailing_bytes": len(vadpcm) - frame_bytes,
        "source_vadpcm_sha256": sha256(vadpcm),
        "source_pcm_samples": len(pcm),
        "source_pcm_sha256": ima_pcm_sha256(pcm),
        "adpcm_book_order": book["order"],
        "adpcm_book_predictors": book["npredictors"],
        "adpcm_book_sha256": json_sha256(book["entries"]),
        "source_loop": loop_audit,
        "source_stop_retrigger_policy": (
            "each note records the then-current set_unk1E high-bit; a set "
            "high-bit and duration_gt_1 fades out for one 184-sample block "
            "at duration_minus_1, otherwise the next note updates the live "
            "voice until stop_voice"),
    }


def excluded_hit_source_audit(selector: dict, ucd: dict,
                              articulations: dict, instrument: dict,
                              ctl_by_offset: dict, source_tbl: bytes,
                              audio_codec) -> dict:
    root_program = ucd["entries"][selector["id"]]["program"]
    validate_ucd(root_program, root_program, selector)
    root_voice = fgm_voice_source_audit(
        selector["id"], ucd, articulations, instrument, ctl_by_offset,
        source_tbl, audio_codec)
    validate_articulation(root_voice["articulation_program"], selector)
    voices = [root_voice]
    for fork_id, expected_hash in zip(
            selector.get("omitted_fork_programs", ()),
            selector.get("omitted_fork_program_sha256", ())):
        fork_voice = fgm_voice_source_audit(
            fork_id, ucd, articulations, instrument, ctl_by_offset,
            source_tbl, audio_codec)
        if fork_voice["ucd_program_sha256"] != expected_hash:
            raise ValueError(
                f"FGM {selector['id']} omitted fork {fork_id} changed")
        voices.append(fork_voice)
    return {
        "id": selector["id"],
        "name": selector["name"],
        "entry_kind": selector["kind"],
        "runtime_included": False,
        "action_contract": selector["action_contract"],
        "source_callsites": list(selector["source_callsites"]),
        "source_pan_behavior": selector["source_pan_behavior"],
        "runtime_excluded_reasons": list(
            selector["runtime_excluded_reasons"]),
        "voices": voices,
    }


def source_custom_fx_audit(repo_root: Path) -> dict:
    source_path = repo_root / "decomp/BattleShip-main/decomp/src/sys/audio.c"
    source = source_path.read_text(encoding="utf-8")
    mixer_path = (repo_root /
                  "decomp/BattleShip-main/decomp/src/libultra/n_audio/n_env.c")
    mixer_source = mixer_path.read_text(encoding="utf-8")
    scene_path = (repo_root /
                  "decomp/BattleShip-main/decomp/src/sc/scmanager.c")
    scene_source = scene_path.read_text(encoding="utf-8")
    match = re.search(
        r"s32 dSYAudioCustomFXParams\[.*?\]\s*=\s*\{(.*?)\};",
        source, re.DOTALL)
    if match is None:
        raise ValueError("BattleShip custom FX parameter table not found")
    params = [int(value) for value in re.findall(r"-?\d+", match.group(1))]
    if len(params) != 114 or params[:2] != [14, 19200]:
        raise ValueError("BattleShip custom FX parameter table changed")
    mixer_fragments = (
        "case AL_FX_CUSTOM:\tparam = c->params;",
        "tmp = ((s32)param->volume * (s32)param->volume) >> 15;",
        "e->em_dryamt = n_eqpower[param->fxMix];",
        "e->em_wetamt = n_eqpower[N_EQPOWER_LENGTH - param->fxMix - 1];",
        "param = (arg0->unk36 * (arg0->unk3C >> 1)) >> 7;",
        "n_alSynSetVol(&arg0->voice, 0, D_8009EDD0_406D0.unk_alsound_0x44);",
        "D_8009EDD0_406D0.unk_alsound_0x44 = (184000000 / n_syn->outputRate);",
        "param, param3, 0);",
    )
    missing = [fragment for fragment in mixer_fragments
               if fragment not in mixer_source]
    if missing:
        raise ValueError(
            f"BattleShip n_env mixer contract changed: {missing}")
    if "syAudioSetFXType(AL_FX_CUSTOM);" not in scene_source:
        raise ValueError("BattleShip scene custom FX selection changed")
    return {
        "scene_selection": "scmanager.c:syAudioSetFXType(AL_FX_CUSTOM)",
        "parameter_source": "sys/audio.c:dSYAudioCustomFXParams",
        "parameter_count": len(params),
        "parameters": params,
        "parameters_sha256": json_sha256(params),
        "mixer_source": "libultra/n_audio/n_env.c",
        "mixer_source_sha256": sha256(mixer_source.encode("utf-8")),
        "output_rate_hz": FGM_OUTPUT_RATE,
        "parameter_ramp_microseconds": 184000000 // FGM_OUTPUT_RATE,
        "parameter_ramp_samples": 184,
        "start_voice_attack_microseconds": 0,
        "volume_curve": "(signed_volume * signed_volume) >> 15",
        "pan_curve": "n_eqpower[pan] / n_eqpower[127-pan]",
        "fx_mix_formula": "articulation_unk36 * (root_unk2c >> 1) >> 7",
        "routing": (
            "n_env.c maps effective fxMix through n_eqpower to dry/wet "
            "amounts and AL_FX_CUSTOM's 14-section delay network"),
    }


def initial_ima_index(samples: list[int]) -> int:
    if len(samples) < 2:
        return 0
    target = max(7, abs(samples[1] - samples[0]))
    return min(range(len(IMA_STEP_TABLE)),
               key=lambda index: abs(IMA_STEP_TABLE[index] - target))


def ima_encode(samples: list[int]) -> bytes:
    if not samples:
        raise ValueError("cannot encode an empty PCM stream")
    predictor = int(samples[0])
    index = initial_ima_index(samples)
    out = bytearray(struct.pack("<hBB", predictor, index, 0))
    nibbles: list[int] = []

    for sample in samples[1:]:
        step = IMA_STEP_TABLE[index]
        delta = int(sample) - predictor
        code = 0
        if delta < 0:
            code = 8
            delta = -delta
        diff = step >> 3
        if delta >= step:
            code |= 4
            delta -= step
            diff += step
        if delta >= (step >> 1):
            code |= 2
            delta -= step >> 1
            diff += step >> 1
        if delta >= (step >> 2):
            code |= 1
            diff += step >> 2
        predictor += -diff if (code & 8) else diff
        predictor = max(-32768, min(32767, predictor))
        index += IMA_INDEX_TABLE[code]
        index = max(0, min(88, index))
        nibbles.append(code)

    for pos in range(0, len(nibbles), 2):
        lo = nibbles[pos]
        hi = nibbles[pos + 1] if pos + 1 < len(nibbles) else 0
        out.append(lo | (hi << 4))
    while len(out) & 3:
        out.append(0)
    return bytes(out)


def ima_apply_nibble(predictor: int, index: int,
                     code: int) -> tuple[int, int]:
    if not 0 <= code <= 15:
        raise ValueError(f"invalid IMA nibble: {code}")
    step = IMA_STEP_TABLE[index]
    diff = step >> 3
    if code & 4:
        diff += step
    if code & 2:
        diff += step >> 1
    if code & 1:
        diff += step >> 2
    predictor += -diff if (code & 8) else diff
    predictor = max(-32768, min(32767, predictor))
    index += IMA_INDEX_TABLE[code]
    index = max(0, min(88, index))
    return predictor, index


def ima_encode_loop_body(samples: list[int], predictor: int, index: int,
                         guard_nibbles: tuple[int, ...]) -> bytes:
    """Encode every loop sample as a nibble after one DS IMA state word."""
    if not samples:
        raise ValueError("cannot encode an empty IMA loop body")
    if not -32768 <= predictor <= 32767 or not 0 <= index <= 88:
        raise ValueError("invalid initial IMA loop state")

    initial_predictor = predictor
    initial_index = index
    nibbles: list[int] = []
    for sample in samples:
        step = IMA_STEP_TABLE[index]
        delta = int(sample) - predictor
        code = 0
        if delta < 0:
            code = 8
            delta = -delta
        if delta >= step:
            code |= 4
            delta -= step
        if delta >= (step >> 1):
            code |= 2
            delta -= step >> 1
        if delta >= (step >> 2):
            code |= 1
        predictor, index = ima_apply_nibble(predictor, index, code)
        nibbles.append(code)
    if any(not 0 <= code <= 15 for code in guard_nibbles):
        raise ValueError("invalid IMA loop guard nibble")
    nibbles.extend(guard_nibbles)
    if len(nibbles) & 1:
        raise ValueError("IMA loop body plus guards must fill whole bytes")

    out = bytearray(struct.pack("<hBB", initial_predictor,
                                initial_index, 0))
    for pos in range(0, len(nibbles), 2):
        out.append(nibbles[pos] | (nibbles[pos + 1] << 4))
    if len(out) & 3:
        raise ValueError("IMA loop body plus guards must fill whole words")
    return bytes(out)


def ima_decode_nibbles(encoded: bytes, nibble_count: int,
                       data_offset: int = 4) -> list[int]:
    """Decode data nibbles from the state in an IMA header."""
    if len(encoded) < 4 or nibble_count < 0 or data_offset < 4:
        raise ValueError("invalid IMA nibble stream")
    predictor, index, reserved = struct.unpack_from("<hBB", encoded, 0)
    if index > 88 or reserved != 0:
        raise ValueError("invalid IMA state header")
    available = (len(encoded) - data_offset) * 2
    if nibble_count > available:
        raise ValueError("short IMA nibble stream")

    out: list[int] = []
    for value in encoded[data_offset:]:
        for code in (value & 0x0F, value >> 4):
            if len(out) >= nibble_count:
                return out
            predictor, index = ima_apply_nibble(predictor, index, code)
            out.append(predictor)
    return out


def ima_ds_repeat_cycles(encoded: bytes, loop_point_words: int,
                         loop_length_words: int, cycle_count: int,
                         restore_loop_state: bool) -> dict:
    """Model DS IMA PNT/LEN playback and its latched loop decoder state."""
    if (len(encoded) < 4 or (len(encoded) & 3) or
            loop_point_words < 1 or loop_length_words < 1 or
            cycle_count < 1):
        raise ValueError("invalid DS IMA repeat geometry")

    loop_offset = loop_point_words * 4
    loop_end = loop_offset + loop_length_words * 4
    if loop_offset < 4 or loop_offset > len(encoded) or loop_end > len(encoded):
        raise ValueError("DS IMA PNT/LEN exceeds the encoded stream")

    # The channel reads the IMA state word once.  Any nibbles before PNT then
    # advance the decoder to the state the DS latches for repeat playback.
    predictor, index, reserved = struct.unpack_from("<hBB", encoded, 0)
    if index > 88 or reserved != 0:
        raise ValueError("invalid DS IMA state header")
    for value in encoded[4:loop_offset]:
        for code in (value & 0x0F, value >> 4):
            predictor, index = ima_apply_nibble(predictor, index, code)
    loop_state = (predictor, index)

    cycle_pcm = []
    cycle_end_states = []
    for cycle in range(cycle_count):
        if cycle > 0 and restore_loop_state:
            predictor, index = loop_state
        decoded = []
        for value in encoded[loop_offset:loop_end]:
            for code in (value & 0x0F, value >> 4):
                predictor, index = ima_apply_nibble(predictor, index, code)
                decoded.append(predictor)
        cycle_pcm.append(decoded)
        cycle_end_states.append((predictor, index))
    return {
        "loop_state": loop_state,
        "cycle_pcm": cycle_pcm,
        "cycle_end_states": cycle_end_states,
    }


def ima_pcm_sha256(samples: list[int]) -> str:
    return sha256(struct.pack(f"<{len(samples)}h", *samples))


def ima_repeat_oracle(encoded: bytes, loop_point_words: int,
                      body_samples: int,
                      guard_nibbles: tuple[int, ...]) -> dict:
    """Prove the DS PNT/LEN state machine restores one stable loop cycle."""
    loop_offset = loop_point_words * 4
    loop_length_words = (len(encoded) // 4) - loop_point_words
    cycle_samples = loop_length_words * 8
    alignment_debt_samples = len(guard_nibbles)
    if (loop_offset != 4 or
            cycle_samples != body_samples + alignment_debt_samples or
            loop_offset + loop_length_words * 4 != len(encoded)):
        raise ValueError("IMA loop point/length fixture changed")

    raw_nibbles = []
    for value in encoded[loop_offset:]:
        raw_nibbles.extend((value & 0x0F, value >> 4))
    if tuple(raw_nibbles[body_samples:]) != guard_nibbles:
        raise ValueError("IMA loop alignment guard nibbles changed")

    repeated = ima_ds_repeat_cycles(
        encoded, loop_point_words, loop_length_words,
        REPEAT_ORACLE_CYCLES, True)
    cycle_hashes = [ima_pcm_sha256(cycle)
                    for cycle in repeated["cycle_pcm"]]
    if len(set(cycle_hashes)) != 1:
        raise ValueError("DS IMA restored repeat cycles drifted")

    # A decoder that carries the end state into the next cycle must diverge.
    # This negative control prevents a per-cycle header reset from masquerading
    # as a DS loop-state proof.
    carried = ima_ds_repeat_cycles(
        encoded, loop_point_words, loop_length_words,
        REPEAT_ORACLE_CYCLES, False)
    carried_hashes = [ima_pcm_sha256(cycle)
                      for cycle in carried["cycle_pcm"]]
    missing_restore_detected = (
        carried_hashes[0] == cycle_hashes[0] and
        all(carried_hash != cycle_hashes[0]
            for carried_hash in carried_hashes[1:]))
    if not missing_restore_detected:
        raise ValueError("DS IMA missing-loop-state negative control failed")

    # Exercise the exact historical wiring failures: PNT pointing at the
    # header, and LEN receiving the full buffer size instead of bytes after
    # PNT.  Both must be rejected by the same state machine used above.
    wrong_pnt_detected = False
    try:
        ima_ds_repeat_cycles(
            encoded, loop_point_words - 1, loop_length_words,
            REPEAT_ORACLE_CYCLES, True)
    except ValueError:
        wrong_pnt_detected = True
    if not wrong_pnt_detected:
        raise ValueError("DS IMA wrong-PNT negative control failed")

    wrong_len_detected = False
    try:
        ima_ds_repeat_cycles(
            encoded, loop_point_words,
            loop_length_words + loop_point_words,
            REPEAT_ORACLE_CYCLES, True)
    except ValueError:
        wrong_len_detected = True
    if not wrong_len_detected:
        raise ValueError("DS IMA wrong-LEN negative control failed")

    loop_predictor, loop_index = repeated["loop_state"]
    end_predictor, end_index = repeated["cycle_end_states"][0]
    return {
        "ds_repeat_oracle_model": "header_once_pnt_latch_len_restore",
        "ds_repeat_oracle_cycles": REPEAT_ORACLE_CYCLES,
        "ds_repeat_oracle_loop_predictor": loop_predictor,
        "ds_repeat_oracle_loop_index": loop_index,
        "ds_repeat_oracle_cycle_end_predictor": end_predictor,
        "ds_repeat_oracle_cycle_end_index": end_index,
        "ds_repeat_oracle_missing_restore_detected": (
            missing_restore_detected),
        "ds_repeat_oracle_missing_restore_cycle_2_pcm_sha256": (
            carried_hashes[1]),
        "ds_repeat_oracle_wrong_pnt_detected": wrong_pnt_detected,
        "ds_repeat_oracle_wrong_len_detected": wrong_len_detected,
        "ds_repeat_cycle_source_samples": body_samples,
        "ds_repeat_cycle_alignment_debt_samples": alignment_debt_samples,
        "ds_repeat_cycle_samples": cycle_samples,
        "ds_repeat_cycle_pcm_sha256": cycle_hashes[0],
    }


def ima_decode(encoded: bytes, sample_count: int) -> list[int]:
    if len(encoded) < 4 or sample_count == 0:
        raise ValueError("invalid IMA stream")
    predictor, index, _reserved = struct.unpack_from("<hBB", encoded, 0)
    if index > 88:
        raise ValueError("invalid IMA initial index")
    out = [predictor]
    for value in encoded[4:]:
        for code in (value & 0x0F, value >> 4):
            if len(out) >= sample_count:
                return out
            step = IMA_STEP_TABLE[index]
            diff = step >> 3
            if code & 4:
                diff += step
            if code & 2:
                diff += step >> 1
            if code & 1:
                diff += step >> 2
            predictor += -diff if (code & 8) else diff
            predictor = max(-32768, min(32767, predictor))
            index += IMA_INDEX_TABLE[code]
            index = max(0, min(88, index))
            out.append(predictor)
    if len(out) != sample_count:
        raise ValueError(f"short IMA decode: {len(out)} != {sample_count}")
    return out


def audio_metrics(original: list[int], decoded: list[int]) -> dict:
    if len(original) != len(decoded) or not decoded:
        raise ValueError("metric stream mismatch")
    peak = max(abs(value) for value in decoded)
    rms = math.sqrt(sum(value * value for value in decoded) / len(decoded))
    signal = sum(value * value for value in original)
    error = sum((left - right) ** 2 for left, right in zip(original, decoded))
    snr = 99.0 if error == 0 else 10.0 * math.log10(signal / error)
    return {
        "decoded_peak": peak,
        "decoded_rms": round(rms, 3),
        "ima_snr_db": round(snr, 3),
    }


def ds_volume(ucd_volume: int, articulation_volume: int) -> int:
    """Map BattleShip's exact integer FGM gain product onto DS 0..127."""
    n64_volume = source_volume_target(ucd_volume, articulation_volume)
    return min(127, (n64_volume * 127 + 16383) // 32767)


def source_volume_target(ucd_volume: int, articulation_volume: int) -> int:
    ucd_player_volume = (ucd_volume * 127) >> 7
    return (articulation_volume * ucd_player_volume * 127) >> 7


def source_quadratic_target(ucd_volume: int,
                            articulation_volume: int) -> int:
    target = source_volume_target(ucd_volume, articulation_volume)
    return (target * target) >> 15


def articulation_envelope(program: list[list], selector: dict) -> list[dict]:
    tick = 0
    points = []
    for row in program:
        if row[0] == "vol":
            art_volume = int(row[1])
            if tick < selector["duration_ticks"]:
                points.append({
                    "tick": tick,
                    "articulation_volume": art_volume,
                    "source_pre_mixer_target": source_volume_target(
                        selector["ucd_volume"], art_volume),
                    "source_quadratic_target": source_quadratic_target(
                        selector["ucd_volume"], art_volume),
                    "ds_volume": ds_volume(selector["ucd_volume"],
                                           art_volume),
                })
            tick += int(row[2])
        elif row[0] == "end":
            if tick < selector["duration_ticks"]:
                points.append({
                    "tick": tick,
                    "articulation_volume": 0,
                    "source_pre_mixer_target": 0,
                    "source_quadratic_target": 0,
                    "ds_volume": 0,
                })
    if not points or points[0]["tick"] != 0:
        points.insert(0, {
            "tick": 0,
            "articulation_volume": 127,
            "source_pre_mixer_target": source_volume_target(
                selector["ucd_volume"], 127),
            "source_quadratic_target": source_quadratic_target(
                selector["ucd_volume"], 127),
            "ds_volume": ds_volume(selector["ucd_volume"], 127),
        })
    return points


def f32(value: float) -> float:
    """Round one operation to the source engine's IEEE-754 f32 value."""
    return struct.unpack(">f", struct.pack(">f", value))[0]


def source_sine_table(path: Path) -> tuple[list[int], bytes]:
    wrapped = path.read_bytes()
    if sha256(wrapped) != SOURCE_SINE_TABLE_SHA256:
        raise ValueError("BattleShip source sine table changed")
    values = [int(value, 16) for value in re.findall(
        rb"0x([0-9A-Fa-f]{4})", wrapped)]
    if len(values) != 2048 or values[0] != 0 or values[1024] != 32768:
        raise ValueError("unexpected BattleShip source sine table")
    return values, wrapped


def validate_source_actions(repo_root: Path, selector: dict) -> list[dict]:
    actions = [dict(action) for action in selector.get("source_actions", ())]
    if not actions:
        return []
    source_path = (repo_root / "decomp/BattleShip-main" /
                   selector["source_action_file"])
    source = source_path.read_text(encoding="utf-8")

    if selector["id"] in (300, 303):
        call = "func_800269C0_275C0(dFTCommonDataDownBounceSFX[fp->fkind])"
        if source.count(call) != 1:
            raise ValueError("DownBounce source callsite changed")
        status = re.search(
            r"void\s+ftCommonDownBounceSetStatus\([^)]*\)\s*\{(.*?)\n\}",
            source, re.DOTALL)
        if (status is None or
                status.group(1).find("ftMainSetStatus") < 0 or
                status.group(1).find("ftCommonDownBounceUpdateEffects") <
                status.group(1).find("ftMainSetStatus")):
            raise ValueError("DownBounce status-entry trigger changed")
        common_data = (repo_root / "decomp/BattleShip-main/decomp/src/ft/"
                       "ftcommondata.c").read_text(encoding="utf-8")
        expected_mapping_count = 2 if selector["id"] == 300 else 6
        if common_data.count(selector["name"]) != expected_mapping_count:
            raise ValueError("DownBounce fighter-to-FGM mapping changed")
        return actions

    cue = f"{actions[0]['call']}({selector['name']})"
    if source.count(cue) != len(actions):
        raise ValueError(
            f"FGM {selector['id']} source callsite count changed")
    for action in actions:
        if action["call"] != actions[0]["call"]:
            raise ValueError(f"FGM {selector['id']} mixes source call types")
        match = re.search(
            rf"ftMotionCommand\s+{re.escape(action['action'])}\[\]\s*=\s*"
            rf"\{{(.*?)\n\}};", source, re.DOTALL)
        if match is None or match.group(1).count(cue) != 1:
            raise ValueError(
                f"FGM {selector['id']} action {action['action']} changed")
        prefix = match.group(1).split(cue, 1)[0]
        trigger_tick = sum(int(value) for value in re.findall(
            r"ftMotionCommandWait(?:Async)?\((\d+)\)", prefix))
        if trigger_tick != action["trigger_game_tick"]:
            raise ValueError(
                f"FGM {selector['id']} action trigger timing changed")
    return actions


def articulation_tick_schedule(program: list[list], selector: dict,
                               modulator: dict | None,
                               sine_table: list[int]) -> list[dict]:
    events: dict[int, list[list]] = {}
    event_tick = 0
    for row in program:
        events.setdefault(event_tick, []).append(row)
        event_tick += int(row[-1])

    volume = 127
    pitch_cents = 0
    phase = 0.0
    modulator_active = False
    note_tick = 0
    notes = []
    for pitch_code, _duration_code, duration_ticks in selector["notes"]:
        notes.append((note_tick, note_tick + duration_ticks, pitch_code))
        note_tick += duration_ticks
    if note_tick != selector["duration_ticks"]:
        raise ValueError(f"FGM {selector['id']} note duration changed")

    schedule = []
    for tick in range(selector["duration_ticks"]):
        ended = False
        for row in events.get(tick, []):
            if row[0] == "vol":
                value = int(row[1])
                volume = (value if value <= 127 else
                          min(127, max(0, volume + value - 192)))
            elif row[0] == "pitch":
                value = int(row[1])
                pitch_cents = (min(1200, max(-1200, value))
                               if -1200 <= value <= 1200 else
                               min(1200, max(-1200,
                                             pitch_cents + value - 2400)))
            elif row[0] == "spawn_mod":
                if (modulator is None or int(row[1]) != 0 or
                        int(row[2]) != selector["aot_modulator_index"]):
                    raise ValueError(
                        f"FGM {selector['id']} AOT modulator binding changed")
                phase = f32(f32(f32(modulator["period"]) *
                                f32(modulator["init_phase"])) *
                            f32(1.0 / 256.0))
                modulator_active = True
            elif row[0] == "stop_mod":
                if int(row[1]) != 0:
                    raise ValueError(
                        f"FGM {selector['id']} AOT modulator slot changed")
                modulator_active = False
            elif row[0] == "end":
                ended = True
            elif row[0] not in ("trigger", "unk36", "pan",
                                "mark_loop", "jump_loop"):
                raise ValueError(
                    f"FGM {selector['id']} AOT gained unsupported op {row[0]}")

        sine_index = None
        if modulator_active:
            assert modulator is not None
            if (modulator["shape"] != 0 or modulator["target"] != 11 or
                    modulator["postproc"] != 0):
                raise ValueError(
                    f"FGM {selector['id']} AOT modulator semantics changed")
            phase = f32(phase + f32(1.0))
            if f32(modulator["period"]) < phase:
                phase = f32(phase - f32(modulator["period"]))
            phase_ratio = f32(phase / f32(modulator["period"]))
            sine_index = int(f32(phase_ratio * f32(4096.0))) & 0xFFF
            angle = f32(sine_table[sine_index & 0x7FF] / f32(65536.0))
            if sine_index & 0x800:
                angle = f32(-angle)
            mod_value = f32(f32(angle * f32(modulator["amplitude"])) +
                            f32(modulator["offset"]))
            volume = int(min(127.0, max(0.0,
                                       f32(mod_value + float(volume)))))
        if ended:
            volume = 0

        pitch_code = next((pitch for start, end, pitch in notes
                           if start <= tick < end), 0)
        if pitch_code == 0:
            frequency = 0
        else:
            frequency = note_frequency_hz(pitch_cents, pitch_code)
        pre_mixer_target = source_volume_target(
            selector["ucd_volume"], volume)
        quadratic_target = source_quadratic_target(
            selector["ucd_volume"], volume)
        schedule.append({
            "tick": tick,
            "articulation_volume": volume,
            "articulation_pitch_cents": pitch_cents,
            "note_pitch_code": pitch_code,
            "frequency_hz": frequency,
            "ds_volume": ds_volume(selector["ucd_volume"], volume),
            "source_pre_mixer_target": pre_mixer_target,
            "source_quadratic_target": quadratic_target,
            "modulator_sine_index": sine_index,
        })
    return schedule


def render_modulated_voice_aot(pcm: list[int], selector: dict,
                               program: list[list], modulator: dict | None,
                               sine_table: list[int],
                               output_frequency: int) -> tuple[list[int], dict]:
    schedule = articulation_tick_schedule(
        program, selector, modulator, sine_table)
    sample_count = min(
        len(pcm),
        ((selector["duration_ticks"] * FGM_TIMER_MICROSECONDS *
          output_frequency + 999999) // 1000000) + 1)
    if sample_count != selector["expected_retained_samples"]:
        raise ValueError(
            f"FGM {selector['id']} AOT sample extent changed: {sample_count}")
    maximum_target = max(point["source_quadratic_target"]
                         for point in schedule)
    channel_volume = min(
        127, (maximum_target * 127 + 16383) // 32767)
    if channel_volume <= 0:
        raise ValueError(f"FGM {selector['id']} AOT is silent")

    rendered = []
    step_volume_negative = []
    source_phase = 0.0
    for sample_index in range(sample_count):
        tick = min(
            selector["duration_ticks"] - 1,
            (sample_index * 1000000) //
            (output_frequency * FGM_TIMER_MICROSECONDS))
        source_index = int(source_phase)
        if source_index >= len(pcm):
            raise ValueError(f"FGM {selector['id']} AOT exceeded source PCM")
        fraction = source_phase - source_index
        if source_index + 1 == len(pcm):
            if fraction != 0.0:
                raise ValueError(
                    f"FGM {selector['id']} AOT exceeded source PCM")
            source_sample = pcm[source_index]
        else:
            source_sample = int(round(
                pcm[source_index] * (1.0 - fraction) +
                pcm[source_index + 1] * fraction))
        gain_numerator, gain_denominator = public_excited_gain_fraction(
            sample_index, output_frequency, schedule)
        scaled = round_div_signed(
            source_sample * gain_numerator * 127,
            gain_denominator * 32767 * channel_volume)
        rendered.append(min(32767, max(-32768, scaled)))
        step_scaled = round_div_signed(
            source_sample * schedule[tick]["source_quadratic_target"] * 127,
            32767 * channel_volume)
        step_volume_negative.append(min(32767, max(-32768, step_scaled)))
        source_phase += schedule[tick]["frequency_hz"] / output_frequency

    schedule_changes = [
        point for index, point in enumerate(schedule)
        if (index == 0 or modulator is not None or
            (point["articulation_volume"],
             point["articulation_pitch_cents"],
             point["note_pitch_code"]) !=
            (schedule[index - 1]["articulation_volume"],
             schedule[index - 1]["articulation_pitch_cents"],
             schedule[index - 1]["note_pitch_code"]))
    ]
    return rendered, {
        "aot_strategy": "source_articulation_pitch_volume_schedule",
        "aot_runtime_automation": False,
        "aot_output_frequency_hz": output_frequency,
        "aot_output_samples": len(rendered),
        "aot_source_phase_end": round(source_phase, 6),
        "aot_constant_hardware_volume": channel_volume,
        "aot_volume_model": "source_quadratic_n_micro_184_sample_ramps",
        "aot_ramp_output_rate_hz": FGM_OUTPUT_RATE,
        "aot_ramp_samples": PUBLIC_EXCITED_RAMP_SAMPLES,
        "aot_modulator_index": selector.get("aot_modulator_index"),
        "aot_modulator": dict(modulator) if modulator is not None else None,
        "aot_full_tick_count": len(schedule),
        "aot_schedule_sha256": json_sha256(schedule),
        "aot_rendered_pcm_sha256": sha256(struct.pack(
            f"<{len(rendered)}h", *rendered)),
        "aot_step_volume_negative_pcm_sha256": sha256(struct.pack(
            f"<{len(step_volume_negative)}h", *step_volume_negative)),
        "aot_step_volume_negative_rejected":
            step_volume_negative != rendered,
        "source_effective_tick_schedule": schedule_changes,
    }


def _fgm_relative_u7(value: int, current: int) -> int:
    return (value if value <= 127 else
            min(127, max(0, current + value - 192)))


def _fgm_relative_pitch(value: int, current: int) -> int:
    return (min(1200, max(-1200, value))
            if -1200 <= value <= 1200 else
            min(1200, max(-1200, current + value - 2400)))


def _fgm_modulator_value(state: dict, sine_table: list[int]) -> float:
    modulator = state["modulator"]
    shape = int(modulator["shape"])
    period = f32(modulator["period"])
    phase = f32(state["phase"] + f32(1.0))
    if period < phase:
        phase = f32(phase - period)
    state["phase"] = phase
    amplitude = f32(modulator["amplitude"])
    offset = f32(modulator["offset"])
    if shape == 0:
        sine_index = int(f32(f32(phase / period) * f32(4096.0))) & 0xFFF
        angle = f32(sine_table[sine_index & 0x7FF] / f32(65536.0))
        if sine_index & 0x800:
            angle = f32(-angle)
        return f32(f32(angle * amplitude) + offset)
    if shape == 1:
        return amplitude if f32(period / f32(2.0)) < phase else offset
    if shape == 2:
        return f32(f32(amplitude * phase) / period + offset)
    if shape == 3:
        return f32(f32(amplitude * f32(period - phase)) / period + offset)
    raise ValueError(f"unsupported deterministic FGM modulator shape {shape}")


def articulation_program_states(program: list[list], modulators: dict,
                                sine_table: list[int],
                                tick_count: int) -> list[dict]:
    pc = 0
    loop_pc = 0
    next_tick = 0
    stopped = False
    volume = 127
    pitch = 0
    fx_mix = 0
    active_modulators: dict[int, dict] = {}
    states = []
    for tick in range(tick_count):
        guard = 0
        while not stopped and next_tick <= tick:
            guard += 1
            if guard > 1024 or pc >= len(program):
                raise ValueError("unbounded FGM articulation program")
            row = program[pc]
            pc += 1
            op = row[0]
            if op == "vol":
                volume = _fgm_relative_u7(int(row[1]), volume)
            elif op == "pitch":
                pitch = _fgm_relative_pitch(int(row[1]), pitch)
            elif op == "unk36":
                fx_mix = _fgm_relative_u7(int(row[1]), fx_mix)
            elif op == "spawn_mod":
                slot = int(row[1])
                modulator = modulators["entries"][int(row[2])]
                active_modulators[slot] = {
                    "modulator": modulator,
                    "phase": f32(f32(f32(modulator["period"]) *
                                     f32(modulator["init_phase"])) *
                                 f32(1.0 / 256.0)),
                }
            elif op == "stop_mod":
                active_modulators.pop(int(row[1]), None)
            elif op == "mark_loop":
                loop_pc = pc
            elif op == "jump_loop":
                pc = loop_pc
            elif op == "end":
                stopped = True
            elif op not in ("trigger", "pan"):
                raise ValueError(f"unsupported FGM articulation op {op}")
            wait = int(row[-1])
            if stopped:
                break
            if wait != 0:
                next_tick = tick + wait
                break
        for slot in sorted(active_modulators):
            modulator = active_modulators[slot]["modulator"]
            value = _fgm_modulator_value(active_modulators[slot], sine_table)
            target = int(modulator["target"])
            if target == 10:
                volume = int(min(127.0, max(0.0, value)))
            elif target == 11:
                volume = int(min(127.0, max(0.0, value + volume)))
            elif target == 12:
                pitch = int(min(1200.0, max(-1200.0, value)))
            elif target == 13:
                pitch = int(min(1200.0, max(-1200.0, value + pitch)))
            else:
                raise ValueError(f"unsupported FGM AOT modulator target {target}")
        states.append({"volume": volume, "pitch": pitch,
                       "fx_mix": fx_mix})
    return states


def fgm_program_notes(program: list[list]) -> tuple[list[dict], list[dict]]:
    root_volume = 255
    cut_before_note_end = False
    pitch_offset = 0
    tick = 0
    previous_duration = None
    previous_cut = False
    notes = []
    forks = []
    for row in program:
        op = row[0]
        if op == "set_unk1E":
            cut_before_note_end = (int(row[1]) & 0x80) != 0
        elif op == "set_volume":
            root_volume = int(row[1])
        elif op == "vol_delta":
            root_volume = min(255, max(0, root_volume + int(row[1])))
        elif op == "set_t5_neg2400":
            pitch_offset = -2400
        elif op == "set_t5_neg4800":
            pitch_offset = -4800
        elif op == "fork_voice":
            forks.append({"program_id": int(row[1]), "start_tick": tick})
        elif op == "note":
            duration = int(row[3])
            starts_new = (previous_duration is None or
                          (previous_cut and previous_duration > 1))
            notes.append({
                "start_tick": tick,
                "end_tick": tick + duration,
                "duration_ticks": duration,
                "pitch_code": int(row[1]),
                "pitch_offset_cents": pitch_offset,
                "root_volume": root_volume,
                "starts_new_voice": starts_new,
                "release_tick": (tick + duration - 1
                                 if cut_before_note_end and duration > 1
                                 else None),
            })
            pitch_offset = 0
            tick += duration
            previous_duration = duration
            previous_cut = cut_before_note_end
    if not notes:
        raise ValueError("FGM program has no notes")
    return notes, forks


def decode_fgm_program_voice(program_id: int, ucd: dict,
                             articulations: dict, instrument: dict,
                             ctl_by_offset: dict, source_tbl: bytes,
                             audio_codec) -> tuple[dict, list[int]]:
    audit = fgm_voice_source_audit(
        program_id, ucd, articulations, instrument, ctl_by_offset,
        source_tbl, audio_codec)
    sound = ctl_by_offset[audit["sound_offset"]]
    wave = ctl_by_offset[sound["wavetable_off"]]
    book = ctl_by_offset[wave["book_off"]]
    vadpcm = source_tbl[wave["base"]:wave["base"] + wave["length"]]
    frame_bytes = len(vadpcm) - (len(vadpcm) % 9)
    pcm = audio_codec.adpcm_decode(
        vadpcm[:frame_bytes], book["entries"], book["order"],
        book["npredictors"])
    return audit, pcm


def render_fgm_program_voice_aot(program_id: int, ucd: dict,
                                 articulations: dict, modulators: dict,
                                 instrument: dict, ctl_by_offset: dict,
                                 source_tbl: bytes, audio_codec,
                                 sine_table: list[int]) -> tuple[list[int], dict]:
    audit, pcm = decode_fgm_program_voice(
        program_id, ucd, articulations, instrument, ctl_by_offset,
        source_tbl, audio_codec)
    notes, forks = fgm_program_notes(audit["ucd_program"])
    tick_count = max(note["end_tick"] for note in notes)
    articulation_states = articulation_program_states(
        audit["articulation_program"], modulators, sine_table, tick_count)
    loop = audit["source_loop"]
    output = []
    source_phase = 0.0
    voice_start_tick = 0
    active_root_volume = 255
    previous_target = 0
    for tick in range(tick_count):
        note = next(note for note in notes
                    if note["start_tick"] <= tick < note["end_tick"])
        if tick == note["start_tick"] and note["starts_new_voice"]:
            source_phase = 0.0
            voice_start_tick = tick
            active_root_volume = note["root_volume"]
            local_state = articulation_states[0]
            previous_target = source_quadratic_target(
                active_root_volume, local_state["volume"])
        local_tick = tick - voice_start_tick
        if local_tick >= len(articulation_states):
            local_tick = len(articulation_states) - 1
        state = articulation_states[local_tick]
        target = source_quadratic_target(active_root_volume, state["volume"])
        if note["release_tick"] == tick:
            target = 0
        frequency = round(FGM_OUTPUT_RATE * (2.0 ** (
            (state["pitch"] + note["pitch_code"] * 100 - 1300 +
             note["pitch_offset_cents"]) / 1200.0)))
        for sample_in_tick in range(184):
            if loop is not None:
                loop_start = int(loop["start"])
                loop_end = int(loop["end"])
                if source_phase >= loop_end:
                    source_phase = loop_start + ((source_phase - loop_start) %
                                                  (loop_end - loop_start))
            source_index = int(source_phase)
            if source_index >= len(pcm):
                source_sample = 0
            else:
                fraction = source_phase - source_index
                right = pcm[source_index + 1] if source_index + 1 < len(pcm) else pcm[source_index]
                source_sample = int(round(
                    pcm[source_index] * (1.0 - fraction) + right * fraction))
            gain = previous_target + round_div_signed(
                (target - previous_target) * (sample_in_tick + 1), 184)
            output.append(min(32767, max(-32768,
                round_div_signed(source_sample * gain, 32767))))
            source_phase += frequency / FGM_OUTPUT_RATE
        previous_target = target
    return output, {
        "program_id": program_id,
        "duration_ticks": tick_count,
        "forks": forks,
        "requires_custom_fx": audit["requires_custom_fx"],
        "source_audit": audit,
    }


def render_fgm_composite_aot(program_id: int, ucd: dict,
                             articulations: dict, modulators: dict,
                             instrument: dict, ctl_by_offset: dict,
                             source_tbl: bytes, audio_codec,
                             sine_table: list[int]) -> tuple[list[int], dict]:
    root, root_meta = render_fgm_program_voice_aot(
        program_id, ucd, articulations, modulators, instrument,
        ctl_by_offset, source_tbl, audio_codec, sine_table)
    voices = [(0, root, root_meta)]
    for fork in root_meta["forks"]:
        rendered, metadata = render_fgm_program_voice_aot(
            fork["program_id"], ucd, articulations, modulators, instrument,
            ctl_by_offset, source_tbl, audio_codec, sine_table)
        voices.append((fork["start_tick"] * 184, rendered, metadata))
    sample_count = max(offset + len(rendered)
                       for offset, rendered, _metadata in voices)
    mixed = [0] * sample_count
    for offset, rendered, _metadata in voices:
        for index, sample in enumerate(rendered):
            mixed[offset + index] = min(32767, max(-32768,
                                                   mixed[offset + index] + sample))
    return mixed, {
        "aot_strategy": "source_program_schedule_and_simultaneous_forks",
        "aot_output_frequency_hz": FGM_OUTPUT_RATE,
        "aot_output_samples": sample_count,
        "duration_ticks": (sample_count + 183) // 184,
        "voice_program_ids": [metadata["program_id"]
                              for _offset, _rendered, metadata in voices],
        "source_custom_fx_dry_only": any(
            metadata["requires_custom_fx"]
            for _offset, _rendered, metadata in voices),
        "aot_rendered_pcm_sha256": ima_pcm_sha256(mixed),
    }


def round_div_signed(numerator: int, denominator: int) -> int:
    if denominator <= 0:
        raise ValueError("rounding denominator must be positive")
    if numerator < 0:
        return -((-numerator + denominator // 2) // denominator)
    return (numerator + denominator // 2) // denominator


def note_frequency_hz(articulation_pitch_cents: int,
                      pitch_code: int) -> int:
    note_pitch_cents = pitch_code * 100 - 1300
    return round(FGM_OUTPUT_RATE * (2.0 ** (
        (articulation_pitch_cents + note_pitch_cents) / 1200.0)))


def render_source_loop(pcm: list[int], loop_start: int, loop_end: int,
                       sample_count: int) -> list[int]:
    if not (0 <= loop_start < loop_end <= len(pcm)):
        raise ValueError("invalid finite source-loop extent")
    rendered = list(pcm[:min(sample_count, loop_end)])
    loop = pcm[loop_start:loop_end]
    while len(rendered) < sample_count:
        rendered.extend(loop[:sample_count - len(rendered)])
    return rendered


def trim_proof(selector: dict, program: list[list], pcm: list[int],
               initial_frequency: int) -> tuple[list[int], dict]:
    notes = [row for row in program if row[0] == "note"]
    segments = []
    schedule_samples = 0
    for row in notes:
        frequency = note_frequency_hz(
            selector["articulation_pitch_cents"], int(row[1]))
        duration_ticks = int(row[3])
        numerator = duration_ticks * FGM_TIMER_MICROSECONDS * frequency
        segment_samples = (numerator + 999999) // 1000000
        schedule_samples += segment_samples
        segments.append({
            "pitch_code": int(row[1]),
            "duration_ticks": duration_ticks,
            "frequency_hz": frequency,
            "ceiling_samples": segment_samples,
        })
    schedule_reach = schedule_samples + 1
    current_numerator = (selector["duration_ticks"] *
                         FGM_TIMER_MICROSECONDS * initial_frequency)
    current_consumption = ((current_numerator + 999999) // 1000000) + 1
    pitch_modulated = "articulation_pitch_modulation" in selector.get(
        "fidelity_debt", ())
    if selector.get("render_source_loop", False):
        retained_samples = max(schedule_reach, current_consumption)
        strategy = "finite_source_loop_duration_render"
    elif pitch_modulated:
        retained_samples = len(pcm)
        strategy = "untrimmed_articulation_pitch_modulation"
    elif selector.get("retain_full_source", False):
        retained_samples = len(pcm)
        strategy = "untrimmed_shared_source_reuse"
    else:
        retained_samples = min(
            len(pcm), max(schedule_reach, current_consumption))
        strategy = (
            "source_note_schedule_and_current_ds_consumption_"
            "with_one_sample_ceiling")
    if retained_samples != selector["expected_retained_samples"]:
        raise ValueError(
            f"FGM {selector['id']} retained-sample proof changed: "
            f"{retained_samples}")
    if selector.get("render_source_loop", False):
        retained = render_source_loop(
            pcm, selector["loop_start"], selector["loop_end"],
            retained_samples)
        prefix_samples = min(retained_samples, selector["loop_end"])
    else:
        retained = pcm[:retained_samples]
        prefix_samples = retained_samples
    return retained, {
        "trim_strategy": strategy,
        "trim_source_note_segments": segments,
        "trim_source_schedule_samples_before_guard": schedule_samples,
        "trim_source_schedule_reach_samples": schedule_reach,
        "trim_current_ds_consumption_before_guard_samples":
            current_consumption - 1,
        "trim_current_ds_consumption_reach_samples": current_consumption,
        "trim_one_sample_ceiling": 1,
        "trim_proven_reachable_samples": retained_samples,
        "trim_source_samples_removed": max(
            0, len(pcm) - retained_samples),
        "trim_applied": retained_samples < len(pcm),
        "trim_retained_source_prefix_pcm_sha256": ima_pcm_sha256(retained),
        "trim_retained_prefix_exact": (
            retained[:prefix_samples] == pcm[:prefix_samples]),
        "finite_source_loop_replay_samples": (
            max(0, retained_samples - selector["loop_end"])
            if selector.get("render_source_loop", False) else 0),
    }


def public_excited_source_indices(selector: dict) -> list[int]:
    loop_start = selector["loop_start"]
    loop_end = selector["loop_end"]
    loop_length = loop_end - loop_start
    if loop_start != 1 or loop_length <= 0:
        raise ValueError("PublicExcited source loop contract changed")
    indices = [0]
    while len(indices) < PUBLIC_EXCITED_SAMPLE_COUNT:
        remaining = PUBLIC_EXCITED_SAMPLE_COUNT - len(indices)
        indices.extend(range(loop_start, loop_start + min(loop_length,
                                                          remaining)))
    return indices


def public_excited_gain_fraction(sample_index: int, frequency: int,
                                 envelope: list[dict]) -> tuple[int, int]:
    mixer_position_numerator = sample_index * FGM_OUTPUT_RATE
    current_target = PUBLIC_EXCITED_MIXER_MINIMUM
    for point in envelope:
        command_position = (point["tick"] *
                            PUBLIC_EXCITED_RAMP_SAMPLES * frequency)
        if mixer_position_numerator < command_position:
            break
        target = point["source_quadratic_target"]
        ramp_end = command_position + (
            PUBLIC_EXCITED_RAMP_SAMPLES * frequency)
        if mixer_position_numerator < ramp_end:
            elapsed = mixer_position_numerator - command_position
            denominator = PUBLIC_EXCITED_RAMP_SAMPLES * frequency
            numerator = (current_target * denominator +
                         (target - current_target) * elapsed)
            return numerator, denominator
        current_target = target
    return current_target, 1


def render_public_excited(pcm: list[int], selector: dict,
                          frequency: int,
                          envelope: list[dict]) -> tuple[list[int], dict]:
    indices = public_excited_source_indices(selector)
    maximum_target = max(point["source_quadratic_target"]
                         for point in envelope)
    hardware_volume = min(
        127, (maximum_target * 127 + 16383) // 32767)
    if hardware_volume == 0:
        raise ValueError("PublicExcited hardware gain resolved to zero")
    rendered = []
    for sample_index, source_index in enumerate(indices):
        gain_numerator, gain_denominator = public_excited_gain_fraction(
            sample_index, frequency, envelope)
        rendered_sample = round_div_signed(
            int(pcm[source_index]) * gain_numerator * 127,
            gain_denominator * 32767 * hardware_volume)
        rendered.append(max(-32768, min(32767, rendered_sample)))

    linear = []
    for sample_index, source_index in enumerate(indices):
        quadratic_numerator, quadratic_denominator = (
            public_excited_gain_fraction(sample_index, frequency, envelope))
        # The old defect used the pre-mixer target. Reconstruct the same ramp
        # timing with those targets so the negative control differs only in law.
        linear_envelope = [dict(point,
                                source_quadratic_target=point[
                                    "source_pre_mixer_target"])
                           for point in envelope]
        linear_numerator, linear_denominator = (
            public_excited_gain_fraction(sample_index, frequency,
                                         linear_envelope))
        del quadratic_numerator, quadratic_denominator
        linear.append(round_div_signed(
            int(pcm[source_index]) * linear_numerator,
            linear_denominator * 32767))

    missing_preroll_indices = [
        selector["loop_start"] +
        (index % (selector["loop_end"] - selector["loop_start"]))
        for index in range(PUBLIC_EXCITED_SAMPLE_COUNT)
    ]
    missing_preroll = []
    for sample_index, source_index in enumerate(missing_preroll_indices):
        gain_numerator, gain_denominator = public_excited_gain_fraction(
            sample_index, frequency, envelope)
        missing_preroll.append(round_div_signed(
            int(pcm[source_index]) * gain_numerator * 127,
            gain_denominator * 32767 * hardware_volume))

    command_points = []
    previous_target = PUBLIC_EXCITED_MIXER_MINIMUM
    for point in envelope:
        start_numerator = (point["tick"] *
                           PUBLIC_EXCITED_RAMP_SAMPLES * frequency)
        end_numerator = start_numerator + (
            PUBLIC_EXCITED_RAMP_SAMPLES * frequency)
        command_points.append({
            "tick": point["tick"],
            "start_sample_ceiling": ((start_numerator +
                                       FGM_OUTPUT_RATE - 1) //
                                      FGM_OUTPUT_RATE),
            "end_sample_ceiling": ((end_numerator + FGM_OUTPUT_RATE - 1) //
                                    FGM_OUTPUT_RATE),
            "start_quadratic_target": previous_target,
            "end_quadratic_target": point["source_quadratic_target"],
        })
        previous_target = point["source_quadratic_target"]
    silent_tail_start = command_points[-1]["end_sample_ceiling"]
    source_index_bytes = struct.pack(
        f"<{len(indices)}I", *indices)
    return rendered, {
        "model": "source_loop_then_quadratic_n_micro_184_sample_ramps",
        "sample_count": len(rendered),
        "source_index_sha256": sha256(source_index_bytes),
        "source_first_pass_samples": selector["loop_end"],
        "source_loop_start": selector["loop_start"],
        "source_loop_end": selector["loop_end"],
        "source_loop_samples": selector["loop_end"] - selector["loop_start"],
        "source_former_loop_boundary_starts": list(range(
            selector["loop_end"], len(rendered),
            selector["loop_end"] - selector["loop_start"])),
        "source_pre_roll_present": indices[0] == 0,
        "source_order_exact": all(
            indices[index] == (0 if index == 0 else
                               selector["loop_start"] +
                               ((index - 1) % (selector["loop_end"] -
                                               selector["loop_start"])))
            for index in range(len(indices))),
        "ramp_output_rate_hz": FGM_OUTPUT_RATE,
        "ramp_samples": PUBLIC_EXCITED_RAMP_SAMPLES,
        "ramp_microseconds": (PUBLIC_EXCITED_RAMP_SAMPLES * 1000000 //
                              FGM_OUTPUT_RATE),
        "command_points": command_points,
        "maximum_quadratic_target": maximum_target,
        "constant_hardware_volume": hardware_volume,
        "constant_hardware_gain_numerator": hardware_volume,
        "constant_hardware_gain_denominator": 127,
        "silent_tail_start_sample": silent_tail_start,
        "rendered_pcm_sha256": ima_pcm_sha256(rendered),
        "linear_gain_negative_pcm_sha256": ima_pcm_sha256(linear),
        "linear_gain_negative_rejected": linear != rendered,
        "missing_preroll_negative_pcm_sha256": ima_pcm_sha256(
            missing_preroll),
        "missing_preroll_negative_rejected": missing_preroll != rendered,
    }


def region_us_motion_arrays(path: Path) -> list[dict]:
    """Read the active REGION_US motion-command arrays in source order."""
    lines = path.read_text(encoding="utf-8").splitlines()
    active = True
    regions: list[tuple[bool, bool]] = []
    filtered = []
    for line in lines:
        stripped = line.strip()
        match = re.fullmatch(r"#if defined\(REGION_(JP|US)\)", stripped)
        if match:
            condition = match.group(1) == "US"
            regions.append((active, condition))
            active = active and condition
            continue
        if stripped == "#else" and regions:
            parent_active, condition = regions[-1]
            active = parent_active and not condition
            regions[-1] = (parent_active, not condition)
            continue
        if stripped == "#endif" and regions:
            parent_active, _condition = regions.pop()
            active = parent_active
            continue
        if stripped.startswith("#if") or stripped.startswith("#elif"):
            raise ValueError(f"unsupported motion preprocessor branch: {line}")
        if active:
            filtered.append(line)
    if regions:
        raise ValueError(f"unterminated motion preprocessor branch: {path}")

    array_start = re.compile(
        r"^ftMotionCommand\s+(\w+)\[\]\s*=\s*\{$")
    call = re.compile(r"^\s*(ftMotion\w+)\((.*)\),\s*$")
    audited_calls = {
        "ftMotionCommandWaitAsync", "ftMotionCommandWait",
        "ftMotionCommandLoopBegin", "ftMotionCommandLoopEnd",
        "ftMotionCommandGoto", "ftMotionCommandSetParallelScript",
        "ftMotionPlayFGM", "ftMotionCommandPlayFGMStoreInfo",
        "ftMotionCommandEnd", "ftMotionCommandPauseScript",
    }
    arrays = []
    current = None
    for line in filtered:
        start = array_start.fullmatch(line.strip())
        if start:
            if current is not None:
                raise ValueError(f"nested motion array in {path}")
            current = {"name": start.group(1), "program": []}
            continue
        if current is None:
            continue
        if line.strip() == "};":
            arrays.append(current)
            current = None
            continue
        normalized = re.sub(r"/\*.*?\*/", "", line).split("//", 1)[0]
        command = call.fullmatch(normalized.rstrip())
        if command and command.group(1) in audited_calls:
            current["program"].append({
                "op": command.group(1),
                "args": " ".join(command.group(2).split()),
            })
    if current is not None:
        raise ValueError(f"unterminated motion array in {path}")
    return arrays


def build_attack_action_audit(repo_root: Path) -> dict:
    fixtures = {entry["name"]: entry["id"] for entry in ATTACK_CUE_AUDIT}
    source_specs = (
        ("Mario", "decomp/BattleShip-main/decomp/src/relocData/"
         "202_MarioMainMotion.c"),
        ("Fox", "decomp/BattleShip-main/decomp/src/relocData/"
         "208_FoxMainMotion.c"),
    )
    arrays = []
    for fighter, relative_path in source_specs:
        for array in region_us_motion_arrays(repo_root / relative_path):
            arrays.append({
                "fighter": fighter,
                "source": relative_path,
                **array,
            })
    by_name = {array["name"]: array for array in arrays}
    if len(by_name) != len(arrays):
        raise ValueError("duplicate REGION_US motion-array name")
    next_array = {
        array["name"]: (arrays[index + 1]["name"]
                        if index + 1 < len(arrays) and
                        arrays[index + 1]["source"] == array["source"]
                        else None)
        for index, array in enumerate(arrays)
    }

    def target_name(args: str) -> str | None:
        return args if re.fullmatch(r"d(?:Mario|Fox)MainMotion_\w+", args) \
            else None

    def execute(array_name: str, tick: int = 0, base_tick: int = 0,
                stack: tuple[str, ...] = ()) -> list[dict]:
        if array_name in stack or len(stack) > 12:
            raise ValueError(f"motion control recursion at {array_name}")
        array = by_name[array_name]
        program = array["program"]
        events = []
        pc = 0
        loops: list[list[int]] = []
        terminated = False
        steps = 0
        while pc < len(program):
            steps += 1
            if steps > 4096:
                raise ValueError(f"motion control expansion overflow: {array_name}")
            row = program[pc]
            op = row["op"]
            args = row["args"]
            if op == "ftMotionCommandWaitAsync":
                tick = base_tick + int(args, 0)
            elif op == "ftMotionCommandWait":
                tick += int(args, 0)
            elif op == "ftMotionCommandLoopBegin":
                loops.append([pc + 1, int(args, 0)])
            elif op == "ftMotionCommandLoopEnd":
                if not loops:
                    raise ValueError(f"motion LoopEnd without LoopBegin: {array_name}")
                loops[-1][1] -= 1
                if loops[-1][1] > 0:
                    pc = loops[-1][0]
                    continue
                loops.pop()
            elif op in ("ftMotionPlayFGM",
                        "ftMotionCommandPlayFGMStoreInfo"):
                if args in fixtures:
                    events.append({
                        "fgm_id": fixtures[args],
                        "fgm_name": args,
                        "trigger_tick": tick,
                        "callsite": array_name,
                        "call": op,
                    })
            elif op == "ftMotionCommandSetParallelScript":
                target = target_name(args)
                if target in by_name:
                    events.extend(execute(
                        target, tick, tick, stack + (array_name,)))
            elif op == "ftMotionCommandGoto":
                target = target_name(args)
                if target in by_name:
                    events.extend(execute(
                        target, tick, base_tick, stack + (array_name,)))
                terminated = True
                break
            elif op in ("ftMotionCommandEnd", "ftMotionCommandPauseScript"):
                terminated = True
                break
            pc += 1
        if loops:
            raise ValueError(f"unterminated motion loop: {array_name}")
        fallthrough = next_array[array_name]
        if not terminated and fallthrough is not None:
            events.extend(execute(
                fallthrough, tick, base_tick, stack + (array_name,)))
        return events

    direct_counts = {fgm_id: 0 for fgm_id in ATTACK_DIRECT_CALL_COUNTS}
    callsites = []
    for array in arrays:
        direct = []
        for row in array["program"]:
            if row["op"] not in (
                    "ftMotionPlayFGM",
                    "ftMotionCommandPlayFGMStoreInfo"):
                continue
            fgm_id = fixtures.get(row["args"])
            if fgm_id is not None:
                direct_counts[fgm_id] += 1
                direct.append(fgm_id)
        if direct:
            callsites.append({
                "fighter": array["fighter"],
                "source": array["source"],
                "callsite": array["name"],
                "direct_fgm_ids": direct,
                "control_program": array["program"],
                "direct_events": execute(array["name"]),
            })
    if direct_counts != ATTACK_DIRECT_CALL_COUNTS:
        raise ValueError(
            f"attack direct-call counts changed: {direct_counts}")

    relevant = {callsite["callsite"] for callsite in callsites}
    changed = True
    while changed:
        changed = False
        for array in arrays:
            if array["name"] in relevant:
                continue
            links = [target_name(row["args"])
                     for row in array["program"]
                     if row["op"] in (
                         "ftMotionCommandGoto",
                         "ftMotionCommandSetParallelScript")]
            terminal = any(row["op"] in (
                "ftMotionCommandGoto", "ftMotionCommandEnd",
                "ftMotionCommandPauseScript") for row in array["program"])
            if (any(link in relevant for link in links) or
                    (not terminal and next_array[array["name"]] in relevant)):
                relevant.add(array["name"])
                changed = True

    actions = []
    for array in arrays:
        if array["name"] not in relevant:
            continue
        events = execute(array["name"])
        if events:
            actions.append({
                "fighter": array["fighter"],
                "source": array["source"],
                "action": array["name"],
                "events": events,
            })
    audit = {
        "region": "REGION_US",
        "wait_async_semantics": "absolute action tick",
        "wait_semantics": "relative ticks",
        "direct_call_counts": direct_counts,
        "callsites": callsites,
        "actions": actions,
    }
    digest = json_sha256(audit)
    if digest != ATTACK_ACTION_AUDIT_SHA256:
        raise ValueError(
            f"attack action audit changed: {digest}")
    return {
        "sha256": digest,
        "region": audit["region"],
        "wait_async_semantics": audit["wait_async_semantics"],
        "wait_semantics": audit["wait_semantics"],
        "direct_call_counts": direct_counts,
        "callsite_count": len(callsites),
        "action_count": len(actions),
        "callsites": [{
            "fighter": callsite["fighter"],
            "source": callsite["source"],
            "callsite": callsite["callsite"],
            "direct_fgm_ids": callsite["direct_fgm_ids"],
        } for callsite in callsites],
        "actions": [{
            "fighter": action["fighter"],
            "source": action["source"],
            "action": action["action"],
            "events": action["events"],
        } for action in actions],
    }


def attack_custom_fx_contract(repo_root: Path) -> dict:
    manager_path = Path(
        "decomp/BattleShip-main/decomp/src/sc/scmanager.c")
    engine_path = Path(
        "decomp/BattleShip-main/decomp/src/libultra/n_audio/n_env.c")
    audio_path = Path(
        "decomp/BattleShip-main/decomp/src/sys/audio.c")
    manager = (repo_root / manager_path).read_text(encoding="utf-8")
    engine = (repo_root / engine_path).read_text(encoding="utf-8")
    audio = (repo_root / audio_path).read_text(encoding="utf-8")
    required_manager = "syAudioSetFXType(AL_FX_CUSTOM);"
    required_engine = (
        "param = (arg0->unk36 * (arg0->unk3C >> 1)) >> 7;",
        "n_alSynSetFXMix(&arg0->voice, param);",
        "param3 = (arg0->unk36 * (arg0->unk3C >> 1)) >> 7;",
        "n_alSynStartVoiceParams(&arg0->voice, arg0->unk40,",
        "temp_s0->unkALWhatever8009EDD0_siz34_0x2C = 0x40;",
        "else arg0->unkALWhatever8009EDD0_siz34_0x28->unk3C = "
        "arg0->unkALWhatever8009EDD0_siz34_0x2C;",
        "ptr = _n_saveBuffer(r, r->input, input, ptr);",
        "if (d->fbcoef)",
    )
    if required_manager not in manager or any(
            token not in engine for token in required_engine):
        raise ValueError("BattleShip custom FGM FX-bus contract changed")

    table_match = re.search(
        r"s32 dSYAudioCustomFXParams\[.*?\]\s*=\s*\{(.*?)\};",
        audio, re.DOTALL)
    if table_match is None:
        raise ValueError("BattleShip custom FX parameter table moved")
    table = [int(value) for value in re.findall(
        r"-?\d+", table_match.group(1))]
    if len(table) < 2:
        raise ValueError("BattleShip custom FX parameter table is short")
    section_count, delay_samples = table[:2]
    if len(table) != 2 + (section_count * 8):
        raise ValueError("BattleShip custom FX parameter table shape changed")
    sections = [table[2 + (index * 8):10 + (index * 8)]
                for index in range(section_count)]
    nonzero_gain_outputs = [section[1] for section in sections
                            if section[4] != 0]
    if not nonzero_gain_outputs:
        raise ValueError("BattleShip custom FX lost all output taps")
    return {
        "source_fx_type": "AL_FX_CUSTOM",
        "source_manager": manager_path.as_posix(),
        "source_manager_call": required_manager,
        "source_engine": engine_path.as_posix(),
        "source_effect_table": audio_path.as_posix(),
        "source_effect_table_sha256": json_sha256(table),
        "source_effect_section_count": section_count,
        "source_effect_delay_samples": delay_samples,
        "source_effect_latest_nonzero_gain_output_tap_samples": max(
            nonzero_gain_outputs),
        "source_effect_nonzero_feedback_sections": sum(
            section[2] != 0 for section in sections),
        "source_effect_feedback_tail": "exact finite silence horizon not proven",
        "source_effect_state_scope": "shared aux-bus circular delay",
        "source_articulation_opcode": "unk36",
        "source_default_voice_fx": 64,
        "source_mix_law": "(articulation_fx * (voice_fx >> 1)) >> 7",
        "source_voice_calls": ["n_alSynSetFXMix", "n_alSynStartVoiceParams"],
        "ds_resident_pack_fx_fields": 0,
        "ds_runtime_behavior": "dry hardware channel",
        "qualification": "blocked until custom FX-bus behavior is reproduced",
    }


def build_fgm218_feasibility(repo_root: Path, action_audit: dict,
                             cue_audit: dict, fx_contract: dict,
                             resident_bytes: int) -> dict:
    cue = next((item for item in cue_audit["cues"]
                if item["id"] == 218), None)
    if cue is None:
        raise ValueError("FGM 218 source audit disappeared")

    actions = []
    for action in action_audit["actions"]:
        ticks = [event["trigger_tick"] for event in action["events"]
                 if event["fgm_id"] == 218]
        if ticks:
            actions.append({"action": action["action"], "trigger_ticks": ticks})
    expected_ticks = list(range(4, 41, 3))
    if len(actions) != 2 or any(
            action["trigger_ticks"] != expected_ticks for action in actions):
        raise ValueError("FGM 218 tornado call schedule changed")

    duration_ticks = cue["voice"]["duration_ticks"]
    max_live_voices = max(
        sum(start <= tick < start + duration_ticks for start in expected_ticks)
        for tick in range(expected_ticks[0],
                          expected_ticks[-1] + duration_ticks + 1))

    header_path = Path("include/nds/nds_audio_fgm.h")
    header = (repo_root / header_path).read_text(encoding="utf-8")
    handle_match = re.search(
        r"^#define NDS_AUDIO_FGM_HANDLE_CAPACITY (\d+)u$",
        header, re.MULTILINE)
    if handle_match is None:
        raise ValueError("DS FGM handle capacity definition moved")
    handle_capacity = int(handle_match.group(1))

    custom_fx_commands = cue["voice"]["articulation"][
        "custom_fx_mix_commands"]
    if custom_fx_commands != [["unk36", 100, 0]]:
        raise ValueError("FGM 218 custom FX articulation changed")
    articulation_fx = custom_fx_commands[0][1]
    voice_fx = fx_contract["source_default_voice_fx"]
    effective_fx_mix = (articulation_fx * (voice_fx >> 1)) >> 7

    def ima_storage_bytes(sample_count: int) -> int:
        unaligned = 4 + (sample_count // 2)
        return (unaligned + 3) & ~3

    dry_samples = (
        duration_ticks * FGM_TIMER_MICROSECONDS * FGM_OUTPUT_RATE + 999999
    ) // 1000000
    dry_ima_bytes = ima_storage_bytes(dry_samples)
    minimum_wet_samples = (
        fx_contract[
            "source_effect_latest_nonzero_gain_output_tap_samples"] + 1)
    minimum_wet_ima_bytes = ima_storage_bytes(minimum_wet_samples)
    dry_projected_bytes = resident_bytes + PACK_ENTRY.size + dry_ima_bytes
    minimum_wet_projected_bytes = (
        resident_bytes + PACK_ENTRY.size + minimum_wet_ima_bytes)

    if not (
            duration_ticks == 27 and
            max_live_voices == 9 and
            handle_capacity == 8 and
            effective_fx_mix == 25 and
            dry_samples == 4968 and
            dry_ima_bytes == 2488 and
            minimum_wet_samples == 17601 and
            minimum_wet_ima_bytes == 8804):
        raise ValueError("FGM 218 feasibility boundary changed")

    return {
        "id": 218,
        "decision": "fail_closed",
        "qualified": False,
        "experiment_boundary": (
            "one AOT-baked Nintendo DS IMA entry per natural source call"),
        "source_actions": actions,
        "source_calls_per_action": len(expected_ticks),
        "source_retrigger_period_ticks": 3,
        "source_voice_duration_ticks": duration_ticks,
        "source_stop_semantics": (
            "each voice stops at call tick + 27; live interval is half-open"),
        "source_max_live_voices": max_live_voices,
        "ds_handle_capacity_source": header_path.as_posix(),
        "ds_handle_capacity": handle_capacity,
        "overlap_handle_shortfall": max_live_voices - handle_capacity,
        "source_articulation_fx": articulation_fx,
        "source_inherited_voice_fx": voice_fx,
        "source_effective_fx_mix": effective_fx_mix,
        "source_effect_feedback_tail": fx_contract[
            "source_effect_feedback_tail"],
        "resident_bytes_before_candidate": resident_bytes,
        "resident_limit_bytes": MAX_RESIDENT_BYTES,
        "resident_free_bytes_before_candidate": (
            MAX_RESIDENT_BYTES - resident_bytes),
        "aot_pack_entry_bytes": PACK_ENTRY.size,
        "dry_aot_samples": dry_samples,
        "dry_aot_ima_bytes": dry_ima_bytes,
        "dry_projected_pack_bytes": dry_projected_bytes,
        "dry_projected_headroom_bytes": (
            MAX_RESIDENT_BYTES - dry_projected_bytes),
        "minimum_wet_timeline_basis": (
            "latest configured nonzero-gain custom-FX output tap + 1"),
        "minimum_wet_timeline_samples": minimum_wet_samples,
        "minimum_wet_ima_bytes": minimum_wet_ima_bytes,
        "minimum_wet_projected_pack_bytes": minimum_wet_projected_bytes,
        "minimum_wet_pack_overflow_bytes": (
            minimum_wet_projected_bytes - MAX_RESIDENT_BYTES),
        "runtime_conversion_allowed": False,
        "runtime_allocation_allowed": False,
        "blockers": list(cue["blockers"]),
    }


def build_attack_cue_audit(ucd: dict, articulations: dict,
                           modulators: dict, ctl_by_offset: dict,
                           instrument: dict, source_tbl: bytes,
                           audio_codec) -> dict:
    def duration_for(row: list, duration_table: list[int]) -> int:
        code = int(row[2])
        if code == 0:
            return 0
        if 1 <= code <= 6:
            return duration_table[code - 1]
        if code == 7 and row[3] is not None:
            return int(row[3])
        raise ValueError(f"invalid UCD duration row: {row}")

    sound_cache: dict[int, dict] = {}

    def audit_sound(sound_index: int) -> dict:
        if sound_index in sound_cache:
            return sound_cache[sound_index]
        sound_offset = instrument["soundArray_offs"][sound_index]
        sound = ctl_by_offset[sound_offset]
        wave = ctl_by_offset[sound["wavetable_off"]]
        if wave["type"] != 0:
            raise ValueError(f"attack sound {sound_index} is not VADPCM")
        book = ctl_by_offset[wave["book_off"]]
        loop = ctl_by_offset[wave["loop_off"]] if wave["loop_off"] else None
        vadpcm = source_tbl[wave["base"]:wave["base"] + wave["length"]]
        if len(vadpcm) != wave["length"]:
            raise ValueError(f"short attack VADPCM sound {sound_index}")
        pcm = audio_codec.adpcm_decode(
            vadpcm, book["entries"], book["order"], book["npredictors"])
        ima = ima_encode(pcm)
        result = {
            "sound_index": sound_index,
            "sound_offset": sound_offset,
            "sample_pan": sound["samplePan"],
            "sample_volume": sound["sampleVolume"],
            "source_sound_gain_fields_used_by_fgm": False,
            "wave_base": wave["base"],
            "source_vadpcm_bytes": wave["length"],
            "source_vadpcm_sha256": sha256(vadpcm),
            "source_pcm_samples": len(pcm),
            "source_pcm_sha256": ima_pcm_sha256(pcm),
            "ds_ima_bytes_if_resident": len(ima),
            "ds_ima_sha256_if_resident": sha256(ima),
            "source_loop": ({
                "start": loop["start"],
                "end": loop["end"],
                "count": loop["count"],
            } if loop else None),
        }
        sound_cache[sound_index] = result
        return result

    def audit_articulation(articulation_index: int) -> dict:
        program = articulations["entries"][articulation_index]["program"]
        tick = 0
        timed_program = []
        for row in program:
            wait = int(row[-1]) if len(row) > 1 else 0
            timed_program.append({"tick": tick, "command": row})
            tick += wait
        triggers = [int(row[1]) for row in program if row[0] == "trigger"]
        mods = []
        for row in program:
            if row[0] == "spawn_mod":
                mods.append({
                    "id": int(row[1]),
                    "index": int(row[2]),
                    "wait_ticks": int(row[3]),
                    "program": modulators["entries"][int(row[2])],
                })
        return {
            "articulation_index": articulation_index,
            "program": program,
            "program_sha256": json_sha256(program),
            "timed_program": timed_program,
            "program_linear_duration_ticks": tick,
            "pitch_commands": [row for row in program if row[0] == "pitch"],
            "volume_commands": [row for row in program if row[0] == "vol"],
            "pan_commands": [row for row in program if row[0] == "pan"],
            "custom_fx_mix_commands": [
                row for row in program if row[0] == "unk36"],
            "spawn_modulators": mods,
            "loop_commands": [row for row in program
                              if row[0] in ("mark_loop", "jump_loop")],
            "triggered_sounds": [audit_sound(index) for index in triggers],
        }

    def audit_voice(program_id: int, inherited_pan: int = 64,
                    stack: tuple[int, ...] = ()) -> dict:
        if program_id in stack:
            raise ValueError(f"recursive attack UCD fork: {program_id}")
        program = ucd["entries"][program_id]["program"]
        articulation_ids = [int(row[1]) for row in program
                            if row[0] == "set_articulation"]
        if len(articulation_ids) != 1:
            raise ValueError(
                f"attack UCD {program_id} articulation count changed")
        articulation = audit_articulation(articulation_ids[0])
        initial_pitches = articulation["pitch_commands"]
        if not initial_pitches:
            raise ValueError(f"attack articulation {articulation_ids[0]} has no pitch")
        articulation_pitch = int(initial_pitches[0][1])
        tick = 0
        duration_table = [0] * 6
        volume = None
        pan = inherited_pan
        t5 = 0
        notes = []
        volumes = []
        pans = [{"tick": 0, "value": inherited_pan, "source": "FGM default"}]
        forks = []
        for row in program:
            op = row[0]
            if op == "set_dur_table":
                duration_table = [int(value) for value in row[1:7]]
            elif op == "set_volume":
                volume = int(row[1])
                volumes.append({"tick": tick, "value": volume,
                                "command": row})
            elif op == "vol_delta":
                volume = max(0, min(255, int(volume or 0) + int(row[1])))
                volumes.append({"tick": tick, "value": volume,
                                "command": row})
            elif op == "set_pan":
                pan = int(row[1])
                pans.append({"tick": tick, "value": pan, "command": row})
            elif op == "pan_delta":
                pan = max(0, min(127, pan + int(row[1])))
                pans.append({"tick": tick, "value": pan, "command": row})
            elif op == "set_t5_neg2400":
                t5 = -2400
            elif op == "set_t5_neg4800":
                t5 = -4800
            elif op == "fork_voice":
                fork_id = int(row[1])
                forks.append({
                    "spawn_tick": tick,
                    "source_scheduler_countdown_ticks": 1,
                    "voice": audit_voice(
                        fork_id, pan, stack + (program_id,)),
                })
            elif op == "note":
                duration = duration_for(row, duration_table)
                pitch_code = int(row[1])
                note_cents = pitch_code * 100 - 1300 + t5
                net_cents = articulation_pitch + note_cents
                frequency = round(FGM_OUTPUT_RATE * (2.0 ** (
                    net_cents / 1200.0)))
                notes.append({
                    "start_tick": tick,
                    "duration_ticks": duration,
                    "pitch_code": pitch_code,
                    "duration_code": int(row[2]),
                    "t5_cents": t5,
                    "note_pitch_cents": note_cents,
                    "initial_articulation_pitch_cents": articulation_pitch,
                    "initial_source_frequency_hz": frequency,
                    "frequency_exceeds_u16": frequency > 0xFFFF,
                    "volume": volume,
                    "pan": pan,
                })
                t5 = 0
                tick += duration
        return {
            "program_id": program_id,
            "program": program,
            "program_sha256": json_sha256(program),
            "duration_ticks": tick,
            "notes": notes,
            "volume_schedule": volumes,
            "pan_schedule": pans,
            "forks": forks,
            "articulation": articulation,
        }

    cues = []
    for fixture in ATTACK_CUE_AUDIT:
        voice = audit_voice(fixture["id"])
        if voice["program_sha256"] != fixture["root_program_sha256"]:
            raise ValueError(f"attack FGM {fixture['id']} root UCD changed")
        pending_voices = [voice]
        rates_above_u16 = False
        while pending_voices:
            pending_voice = pending_voices.pop()
            rates_above_u16 = rates_above_u16 or any(
                note["frequency_exceeds_u16"]
                for note in pending_voice["notes"])
            pending_voices.extend(
                fork["voice"] for fork in pending_voice["forks"])
        if rates_above_u16 != (
                "source_rate_above_u16" in fixture["blockers"]):
            raise ValueError(
                f"attack FGM {fixture['id']} u16-rate blocker changed")
        cues.append({
            "id": fixture["id"],
            "name": fixture["name"],
            "qualified": not fixture["blockers"],
            "blockers": list(fixture["blockers"]),
            "effective_fgm_pan": 64,
            "voice": voice,
        })

    fireball = next(cue for cue in cues if cue["id"] == 215)
    fireball_voice = fireball["voice"]
    fireball_art = fireball_voice["articulation"]
    fireball_sound = fireball_art["triggered_sounds"]
    if not (
            fireball_voice["duration_ticks"] == 15 and
            [note["initial_source_frequency_hz"]
             for note in fireball_voice["notes"]] == [32000, 32000] and
            fireball_voice["forks"] == [] and
            len(fireball_voice["pan_schedule"]) == 1 and
            fireball_art["program_sha256"] ==
            "78e320e6ee2a2832cb2f3635016b5b46d13fa820dccf4651d7effcd36ee5c7dd" and
            fireball_art["custom_fx_mix_commands"] == [] and
            fireball_art["spawn_modulators"] == [] and
            fireball_art["loop_commands"] == [] and
            len(fireball_sound) == 1 and
            fireball_sound[0]["sound_index"] == 19 and
            fireball_sound[0]["wave_base"] == 191464 and
            fireball_sound[0]["source_vadpcm_bytes"] == 1224 and
            fireball_sound[0]["source_pcm_samples"] == 2176 and
            fireball_sound[0]["source_loop"] is None and
            fireball_sound[0]["ds_ima_bytes_if_resident"] == 1092 and
            fireball_sound[0]["ds_ima_sha256_if_resident"] ==
            "7ed82ac09a350207bb4107b598447567516fd03ad40324953d041507a234ef78"):
        raise ValueError("FGM 215 exact source qualification changed")

    digest = json_sha256(cues)
    if digest != ATTACK_CUE_AUDIT_SHA256:
        raise ValueError(f"attack cue audit changed: {digest}")
    return {
        "sha256": digest,
        "qualified_ids": [cue["id"] for cue in cues if cue["qualified"]],
        "excluded_ids": [cue["id"] for cue in cues if not cue["qualified"]],
        "cues": cues,
    }


def build_pack(repo_root: Path) -> tuple[bytes, dict]:
    tools_dir = repo_root / "decomp/BattleShip-main/decomp/tools"
    extract_fgm = load_module(tools_dir / "extract_fgm.py", "extract_fgm")
    decode_ctl = load_module(tools_dir / "decode_ctl.py", "decode_ctl")
    audio_codec = load_module(tools_dir / "audio_codec.py", "audio_codec")
    audio_dir = repo_root / "decomp/BattleShip-main/BattleShip_o2r/audio"

    source_wrapped: dict[str, bytes] = {}
    source_raw: dict[str, bytes] = {}
    for name in ("fgm_tbl", "fgm_ucd", "fgm_unk", "B1_sounds2_ctl",
                 "B1_sounds2_tbl"):
        source_wrapped[name], source_raw[name] = read_o2r_payload(
            audio_dir / name)

    ucd = extract_fgm.decode_fgm_ucd(source_raw["fgm_ucd"])
    articulations = extract_fgm.decode_fgm_tbl(source_raw["fgm_tbl"])
    modulators = extract_fgm.decode_fgm_unk(source_raw["fgm_unk"])
    sine_table, sine_source = source_sine_table(
        repo_root / "decomp/BattleShip-main/decomp/src/sys/sintable.c")
    ctl_structs = decode_ctl.walk(source_raw["B1_sounds2_ctl"])
    ctl_by_offset = {entry["offset"]: entry for entry in ctl_structs}
    banks = [entry for entry in ctl_structs if entry["kind"] == "ALBank"]
    if len(banks) != 1 or banks[0]["sampleRate"] != 44100:
        raise ValueError("unexpected B1_sounds2 bank layout")
    bank = banks[0]
    if len(bank["instArray_offs"]) != 1:
        raise ValueError("unexpected B1_sounds2 instrument layout")
    instrument = ctl_by_offset[bank["instArray_offs"][0]]

    excluded_entries = []
    for selector in EXCLUDED_SOURCE_CUES:
        root_program = ucd["entries"][selector["id"]]["program"]
        render_program_id = selector.get("render_program", selector["id"])
        ucd_program = ucd["entries"][render_program_id]["program"]
        validate_ucd(root_program, ucd_program, selector)
        art_program = articulations["entries"][
            selector["articulation"]]["program"]
        validate_articulation(art_program, selector)

        sound_offset = instrument["soundArray_offs"][selector["sound"]]
        sound = ctl_by_offset[sound_offset]
        wave = ctl_by_offset[sound["wavetable_off"]]
        book = ctl_by_offset[wave["book_off"]]
        loop = ctl_by_offset[wave["loop_off"]] if wave["loop_off"] else None
        if (wave["type"] != 0 or wave["base"] != selector["wave_base"] or
                wave["length"] != selector["wave_length"]):
            raise ValueError(f"excluded FGM {selector['id']} wavetable changed")
        actual_loop = (loop["start"], loop["end"]) if loop else (0, 0)
        if actual_loop != (selector["loop_start"], selector["loop_end"]):
            raise ValueError(
                f"excluded FGM {selector['id']} loop changed: {actual_loop}")
        vadpcm = source_raw["B1_sounds2_tbl"][
            wave["base"]:wave["base"] + wave["length"]]
        pcm = audio_codec.adpcm_decode(vadpcm, book["entries"],
                                       book["order"], book["npredictors"])
        if len(pcm) != selector["expected_retained_samples"]:
            raise ValueError(
                f"excluded FGM {selector['id']} sample count changed")

        note_rows = [row for row in ucd_program if row[0] == "note"]
        note_tick = 0
        note_schedule = []
        for row in note_rows:
            note_schedule.append({
                "tick": note_tick,
                "pitch_code": int(row[1]),
                "velocity": int(row[2]),
                "duration_ticks": int(row[3]),
            })
            note_tick += int(row[3])
        articulation_tick = 0
        articulation_pitch = []
        articulation_volume = []
        for row in art_program:
            if row[0] == "pitch":
                articulation_pitch.append({
                    "tick": articulation_tick,
                    "cents": int(row[1]),
                    "duration_ticks": int(row[2]),
                })
            elif row[0] == "vol":
                articulation_volume.append({
                    "tick": articulation_tick,
                    "value": int(row[1]),
                    "duration_ticks": int(row[2]),
                })
            articulation_tick += int(row[-1])
        if len(note_schedule) <= 1 and len(articulation_pitch) <= 1:
            raise ValueError(
                f"excluded FGM {selector['id']} no longer needs scheduling")
        excluded_entries.append({
            "id": selector["id"],
            "name": selector["name"],
            "reason": selector["exclusion_reason"],
            "root_ucd_program_id": selector["id"],
            "render_ucd_program_id": render_program_id,
            "root_ucd_program_sha256": json_sha256(root_program),
            "render_ucd_program_sha256": json_sha256(ucd_program),
            "articulation_index": selector["articulation"],
            "articulation_program_sha256": json_sha256(art_program),
            "source_sound_index": selector["sound"],
            "source_sample_count": len(pcm),
            "source_duration_ticks": note_tick,
            "source_note_schedule": note_schedule,
            "source_articulation_pitch_schedule": articulation_pitch,
            "source_articulation_volume_schedule": articulation_volume,
            "source_ucd_pan_ops": [row for row in ucd_program
                                   if row[0] in ("set_pan", "pan_delta")],
            "source_articulation_pan_ops": [row for row in art_program
                                            if row[0] == "pan"],
            "source_fork_programs": [int(row[1]) for row in root_program
                                     if row[0] == "fork_voice"],
            "source_spawn_mod_ops": [row for row in art_program
                                     if row[0] == "spawn_mod"],
            "source_loop_start": actual_loop[0],
            "source_loop_end": actual_loop[1],
        })

    declared_selectors = {
        int(selector["id"]): dict(selector)
        for selector in (*SELECTED, *EXCLUDED_SOURCE_CUES)
    }
    attack_cue_by_id = {int(cue["id"]): cue for cue in ATTACK_CUE_AUDIT}
    runtime_selected = []
    for fgm_id in FULL_COVERAGE_IDS:
        selector = declared_selectors.get(fgm_id)
        if selector is None:
            cue = attack_cue_by_id[fgm_id]
            program = ucd["entries"][fgm_id]["program"]
            if json_sha256(program) != cue["root_program_sha256"]:
                raise ValueError(f"FGM {fgm_id} root UCD program changed")
            articulation_id = first_program_arg(program, "set_articulation")
            art_program = articulations["entries"][articulation_id]["program"]
            sound_id = first_program_arg(art_program, "trigger")
            sound_offset = instrument["soundArray_offs"][sound_id]
            sound = ctl_by_offset[sound_offset]
            wave = ctl_by_offset[sound["wavetable_off"]]
            loop = ctl_by_offset[wave["loop_off"]] if wave["loop_off"] else None
            notes = tuple(tuple(int(value) for value in row[1:])
                          for row in program if row[0] == "note")
            volumes = [int(row[1]) for row in program
                       if row[0] == "set_volume"]
            pitches = [int(row[1]) for row in art_program
                       if row[0] == "pitch"]
            forks = tuple(int(row[1]) for row in program
                          if row[0] == "fork_voice")
            selector = {
                "id": fgm_id,
                "name": cue["name"],
                "kind": "attack",
                "articulation": articulation_id,
                "sound": sound_id,
                "notes": notes,
                "duration_ticks": sum(note[2] for note in notes),
                "ucd_volume": volumes[0],
                "articulation_pitch_cents": pitches[0],
                "loop": loop is not None,
                "source_loop_infinite": loop is not None,
                "wave_base": wave["base"],
                "wave_length": wave["length"],
                "loop_start": loop["start"] if loop else 0,
                "loop_end": loop["end"] if loop else 0,
                "expected_retained_samples": 1,
                "root_fork_programs": forks,
                "omitted_fork_programs": forks,
                "root_program_sha256": cue["root_program_sha256"],
                "render_program_sha256": cue["root_program_sha256"],
                "omitted_fork_program_sha256": tuple(
                    json_sha256(ucd["entries"][fork]["program"])
                    for fork in forks),
                "articulation_program_sha256": json_sha256(art_program),
                "fidelity_debt": (),
            }
        selector.pop("runtime_excluded", None)
        if fgm_id in FULL_PROGRAM_AOT_IDS:
            selector["aot_full_program"] = True
        runtime_selected.append(selector)
    if tuple(int(selector["id"]) for selector in runtime_selected) != FULL_COVERAGE_IDS:
        raise AssertionError("full FGM coverage order changed")
    excluded_hit_cues = []
    attack_actions = build_attack_action_audit(repo_root)
    attack_fx = attack_custom_fx_contract(repo_root)
    attack_cues = build_attack_cue_audit(
        ucd, articulations, modulators, ctl_by_offset, instrument,
        source_raw["B1_sounds2_tbl"], audio_codec)
    mapping_source = [
        {key: value for key, value in selector.items()}
        for selector in runtime_selected
    ]
    mapping_json = json.dumps(mapping_source, sort_keys=True,
                              separators=(",", ":")).encode("utf-8")
    mapping_sha = sha256(mapping_json)
    mapping_sha_lo = int.from_bytes(bytes.fromhex(mapping_sha)[:4], "little")

    records = []
    metadata_entries = []
    for entry_index, selector in enumerate(runtime_selected):
        source_actions = validate_source_actions(repo_root, selector)
        root_program = ucd["entries"][selector["id"]]["program"]
        render_program_id = selector.get("render_program", selector["id"])
        ucd_program = ucd["entries"][render_program_id]["program"]
        validate_ucd(root_program, ucd_program, selector)
        for fork_id, expected_hash in zip(
                selector.get("omitted_fork_programs", ()),
                selector.get("omitted_fork_program_sha256", ())):
            fork_program = ucd["entries"][fork_id]["program"]
            if json_sha256(fork_program) != expected_hash:
                raise ValueError(
                    f"FGM {selector['id']} omitted fork {fork_id} changed")
        art_program = articulations["entries"][
            selector["articulation"]]["program"]
        validate_articulation(art_program, selector)

        sound_offset = instrument["soundArray_offs"][selector["sound"]]
        sound = ctl_by_offset[sound_offset]
        wave = ctl_by_offset[sound["wavetable_off"]]
        book = ctl_by_offset[wave["book_off"]]
        loop = ctl_by_offset[wave["loop_off"]] if wave["loop_off"] else None
        if (wave["type"] != 0 or wave["base"] != selector["wave_base"] or
                wave["length"] != selector["wave_length"]):
            raise ValueError(f"FGM {selector['id']} wavetable changed")
        actual_loop = (loop["start"], loop["end"]) if loop else (0, 0)
        if actual_loop != (selector["loop_start"], selector["loop_end"]):
            raise ValueError(f"FGM {selector['id']} loop changed: {actual_loop}")
        source_loop_infinite = selector.get(
            "source_loop_infinite", selector["loop"])
        if source_loop_infinite != bool(loop):
            raise ValueError(f"FGM {selector['id']} loop flag changed")

        vadpcm = source_raw["B1_sounds2_tbl"][
            wave["base"]:wave["base"] + wave["length"]]
        if len(vadpcm) != wave["length"] or len(vadpcm) < 9:
            raise ValueError(f"FGM {selector['id']} invalid VADPCM extent")
        pcm = audio_codec.adpcm_decode(vadpcm, book["entries"],
                                       book["order"], book["npredictors"])
        first_pitch_code = (selector["notes"][0][0]
                            if "notes" in selector
                            else selector["pitch_code"])
        note_pitch_cents = first_pitch_code * 100 - 1300
        net_pitch_cents = (selector["articulation_pitch_cents"] +
                           note_pitch_cents)
        frequency = note_frequency_hz(
            selector["articulation_pitch_cents"], first_pitch_code)
        envelope = articulation_envelope(art_program, selector)
        if selector.get("aot_full_program"):
            root_duration_ticks = selector["duration_ticks"]
            runtime_pcm, acoustic_oracle = render_fgm_composite_aot(
                selector["id"], ucd, articulations, modulators, instrument,
                ctl_by_offset, source_raw["B1_sounds2_tbl"], audio_codec,
                sine_table)
            selector["duration_ticks"] = acoustic_oracle["duration_ticks"]
            frequency = FGM_OUTPUT_RATE
            loop_strategy = "source_program_aot"
            flags = 0
            loop_point_words = 0
            packed_envelope = []
            volume = 127
            trim = {
                "trim_strategy": "source_program_schedule_and_forks_aot",
                "trim_source_samples_removed": 0,
                "trim_applied": True,
                "trim_retained_source_prefix_pcm_sha256": None,
                "trim_retained_prefix_exact": False,
                "trim_proven_reachable_samples": len(runtime_pcm),
                "trim_one_sample_ceiling": 1,
                "source_root_duration_ticks": root_duration_ticks,
            }
            old_loop_ima = b""
        elif selector["id"] == PUBLIC_EXCITED_ID:
            runtime_pcm, acoustic_oracle = render_public_excited(
                pcm, selector, frequency, envelope)
            loop_strategy = "finite_source_loop_aot"
            flags = 0
            loop_point_words = 0
            packed_envelope = []
            volume = acoustic_oracle["constant_hardware_volume"]
            trim = {
                "trim_strategy": "finite_source_loop_duration_render",
                "trim_source_samples_removed": 0,
                "trim_applied": False,
                "trim_retained_source_prefix_pcm_sha256": None,
                "trim_retained_prefix_exact": True,
                "trim_proven_reachable_samples": len(runtime_pcm),
                "trim_one_sample_ceiling": 1,
            }
            old_loop_pcm = pcm[selector["loop_start"]:selector["loop_end"]]
            old_loop_ima = ima_encode_loop_body(
                old_loop_pcm, PUBLIC_EXCITED_IMA_PREDICTOR,
                PUBLIC_EXCITED_IMA_INDEX, PUBLIC_EXCITED_GUARD_NIBBLES)
            old_loop_decoded = ima_decode_nibbles(
                old_loop_ima, len(old_loop_pcm),
                PUBLIC_EXCITED_LOOP_POINT_WORDS * 4)
            acoustic_oracle.update({
                "old_hardware_loop_negative_ima_bytes": len(old_loop_ima),
                "old_hardware_loop_negative_ima_sha256": sha256(old_loop_ima),
                "old_hardware_loop_decoded_clipped_sample_count": sum(
                    abs(value) >= 32767 for value in old_loop_decoded),
            })
        elif selector.get("aot_source_schedule"):
            modulator = None
            if "aot_modulator_index" in selector:
                modulator = modulators["entries"][
                    selector["aot_modulator_index"]]
                if modulator != selector["aot_modulator"]:
                    raise ValueError(
                        f"FGM {selector['id']} source modulator changed")
            runtime_pcm, acoustic_oracle = render_modulated_voice_aot(
                pcm, selector, art_program, modulator, sine_table, frequency)
            loop_strategy = "none"
            flags = 0
            loop_point_words = 0
            packed_envelope = []
            volume = acoustic_oracle["aot_constant_hardware_volume"]
            trim = {
                "trim_strategy":
                    "source_articulation_pitch_volume_schedule_aot",
                "trim_source_samples_removed": 0,
                "trim_applied": True,
                "trim_retained_source_prefix_pcm_sha256": None,
                "trim_retained_prefix_exact": False,
                "trim_proven_reachable_samples": len(runtime_pcm),
                "trim_one_sample_ceiling": 1,
            }
            old_loop_ima = b""
        else:
            runtime_pcm, trim = trim_proof(
                selector, ucd_program, pcm, frequency)
            acoustic_oracle = {}
            loop_strategy = (
                "finite_source_loop_aot"
                if selector.get("render_source_loop", False) else "none")
            flags = 0
            loop_point_words = 0
            packed_envelope = envelope[1:]
            volume = envelope[0]["ds_volume"]
            old_loop_ima = b""
        ima = ima_encode(runtime_pcm)
        decoded_ima = ima_decode(ima, len(runtime_pcm))
        metrics = audio_metrics(runtime_pcm, decoded_ima)
        if metrics["decoded_peak"] == 0 or metrics["decoded_rms"] <= 0:
            raise ValueError(f"FGM {selector['id']} decoded to silence")
        if selector["id"] == PUBLIC_EXCITED_ID:
            boundary_starts = acoustic_oracle[
                "source_former_loop_boundary_starts"]
            boundary_deltas = [
                abs(decoded_ima[index] - decoded_ima[index - 1])
                for index in boundary_starts
            ]
            adjacent_deltas = [
                abs(right - left)
                for left, right in zip(decoded_ima, decoded_ima[1:])
            ]
            silent_tail = decoded_ima[
                acoustic_oracle["silent_tail_start_sample"]:]
            tail_peak = max(abs(value) for value in silent_tail)
            tail_rms = math.sqrt(sum(value * value for value in silent_tail) /
                                 len(silent_tail))
            acoustic_oracle.update({
                "decoded_pcm_sha256": ima_pcm_sha256(decoded_ima),
                "decoded_former_loop_boundary_deltas": boundary_deltas,
                "decoded_former_loop_boundary_max_delta": max(
                    boundary_deltas),
                "decoded_adjacent_max_delta": max(adjacent_deltas),
                "decoded_former_loop_boundaries_bounded": max(
                    boundary_deltas) <= max(adjacent_deltas),
                "decoded_clipped_sample_count": sum(
                    abs(value) >= 32767 for value in decoded_ima),
                "rendered_clipped_sample_count": sum(
                    abs(value) >= 32767 for value in runtime_pcm),
                "decoded_silent_tail_samples": len(silent_tail),
                "decoded_silent_tail_peak": tail_peak,
                "decoded_silent_tail_rms": round(tail_rms, 6),
                "old_hardware_loop_negative_rejected": (
                    old_loop_ima != ima and flags == 0 and
                    loop_point_words == 0),
            })
            acoustic_oracle["decoded_clipping_not_regressed"] = (
                acoustic_oracle["rendered_clipped_sample_count"] == 0 and
                acoustic_oracle["decoded_clipped_sample_count"] <=
                acoustic_oracle[
                    "old_hardware_loop_decoded_clipped_sample_count"])
        records.append({
            "id": selector["id"],
            "flags": flags,
            "ima": ima,
            "sample_count": len(runtime_pcm),
            "frequency": frequency,
            "duration_ticks": selector["duration_ticks"],
            "volume": volume,
            "pan": 64,
            "sound": selector["sound"],
            "envelope": packed_envelope,
            "loop_point_words": loop_point_words,
        })
        metadata_entries.append({
            "entry_index": entry_index,
            "entry_kind": selector.get("kind", "phase"),
            "phase_index": (entry_index
                            if selector.get("kind", "phase") == "phase"
                            else None),
            "id": selector["id"],
            "name": selector["name"],
            "source_action_file": selector.get("source_action_file"),
            "source_actions": source_actions,
            "root_ucd_program_id": selector["id"],
            "root_ucd_program": root_program,
            "render_ucd_program_id": render_program_id,
            "ucd_program": ucd_program,
            "root_fork_programs": list(selector.get(
                "root_fork_programs", ())),
            "omitted_fork_programs": ([] if selector.get("aot_full_program")
                                      else list(selector.get(
                                          "omitted_fork_programs", ()))),
            "articulation_index": selector["articulation"],
            "articulation_program": art_program,
            "source_sound_index": selector["sound"],
            "sound_offset": sound_offset,
            "sound_sample_pan": sound["samplePan"],
            "sound_sample_volume": sound["sampleVolume"],
            "wave_base": wave["base"],
            "source_vadpcm_bytes": wave["length"],
            "source_vadpcm_frame_bytes": wave["length"] -
                (wave["length"] % 9),
            "source_vadpcm_trailing_bytes": wave["length"] % 9,
            "source_vadpcm_sha256": sha256(vadpcm),
            "source_pcm_samples": len(pcm),
            "source_pcm_sha256": sha256(struct.pack(
                f"<{len(pcm)}h", *pcm)),
            "source_loop_start": selector["loop_start"],
            "source_loop_end": selector["loop_end"],
            "source_loop_infinite": source_loop_infinite,
            "ds_loop_strategy": loop_strategy,
            "ds_loop_flag": flags,
            "ds_loop_point_words": loop_point_words,
            "ds_loop_length_words": (
                (len(ima) // 4) - loop_point_words),
            "ds_ima_header_predictor": struct.unpack_from("<h", ima, 0)[0],
            "ds_ima_header_index": ima[2],
            "ds_ima_loop_body_nibbles": 0,
            "ds_ima_guard_nibbles": [],
            "ds_initial_prefix_samples_dropped": 0,
            "ds_trailing_samples_dropped": 0,
            "ds_sample_count": len(runtime_pcm),
            "net_pitch_cents": net_pitch_cents,
            "ds_frequency_hz": frequency,
            "source_duration_ticks": selector["duration_ticks"],
            "source_duration_microseconds": (
                selector["duration_ticks"] * FGM_TIMER_MICROSECONDS),
            "ds_volume": volume,
            "ds_pan": 64,
            "ds_initial_volume": volume,
            "packed_envelope_count": len(packed_envelope),
            "source_volume_envelope": envelope,
            "source_sound_gain_fields_used_by_fgm": False,
            "source_volume_mapping": (
                "pre_mixer=((articulation_volume*((ucd_volume*127)>>7)*"
                "127)>>7); mixer=(pre_mixer*pre_mixer)>>15; N_MICRO "
                "linearly ramps each target over 184 samples"
                if selector["id"] == PUBLIC_EXCITED_ID else
                "source articulation volume/LFO targets mapped to DS gain "
                "and baked with the pitch schedule into the AOT sample"
                if selector.get("aot_source_schedule") else
                "pre_mixer target mapped from 0..32767 to DS 0..127"),
            "runtime_fidelity_debt": ([] if selector.get("aot_full_program")
                                      else list(selector.get(
                                          "fidelity_debt", ()))),
            "ima_adpcm_bytes": len(ima),
            "ima_adpcm_sha256": sha256(ima),
            "acoustic_oracle": acoustic_oracle,
            **trim,
            **metrics,
        })

    data_offset = PACK_HEADER.size + len(records) * PACK_ENTRY.size
    sample_body = bytearray()
    envelope_body = bytearray()
    entries_blob = bytearray()
    cursor = data_offset
    sample_offsets: dict[bytes, int] = {}
    for record in records:
        ima = record["ima"]
        if ima in sample_offsets:
            record["data_offset"] = sample_offsets[ima]
            record["sample_body_deduplicated"] = True
        else:
            record["data_offset"] = cursor
            sample_offsets[ima] = cursor
            record["sample_body_deduplicated"] = False
            sample_body += ima
            cursor += len(ima)
    for record, metadata_entry in zip(records, metadata_entries):
        metadata_entry["pack_data_offset"] = record["data_offset"]
        metadata_entry["sample_body_deduplicated"] = record[
            "sample_body_deduplicated"]
    envelope_cursor = cursor
    for record in records:
        envelope = record["envelope"]
        record["envelope_offset"] = envelope_cursor if envelope else 0
        for point in envelope:
            envelope_body += PACK_ENVELOPE_POINT.pack(
                point["tick"], point["ds_volume"], 0)
            envelope_cursor += PACK_ENVELOPE_POINT.size
        entries_blob += PACK_ENTRY.pack(
            record["id"], record["flags"], record["data_offset"],
            len(record["ima"]),
            record["sample_count"], record["frequency"],
            record["duration_ticks"], record["volume"], record["pan"],
            record["sound"], record["envelope_offset"], len(envelope),
            record["loop_point_words"])
    pack_size = data_offset + len(sample_body) + len(envelope_body)
    pack = (PACK_HEADER.pack(PACK_MAGIC, PACK_VERSION, len(records),
                             pack_size, mapping_sha_lo) +
            bytes(entries_blob) + bytes(sample_body) + bytes(envelope_body))
    if len(pack) != pack_size:
        raise AssertionError("pack size accounting mismatch")
    if len(pack) > MAX_PACK_BYTES:
        raise ValueError(
            f"FGM pack exceeds {MAX_PACK_BYTES // 1024} KiB: "
            f"{len(pack)} bytes")
    fgm218_feasibility = build_fgm218_feasibility(
        repo_root, attack_actions, attack_cues, attack_fx, len(pack))

    # One bounded feasibility cut for the naturally observed small-kick cue.
    # Fork 658 runs for 200 FGM ticks, so any exact 32 kHz fused capture must
    # span at least this many samples even before AL_FX_CUSTOM delay output.
    id34_fork_ticks = 200
    id34_root_ticks = 70
    id34_fused_samples = round(
        id34_fork_ticks * FGM_TIMER_MICROSECONDS * FGM_OUTPUT_RATE /
        1_000_000)
    id34_root_samples = round(
        id34_root_ticks * FGM_TIMER_MICROSECONDS * FGM_OUTPUT_RATE /
        1_000_000)
    id34_fused_add_bytes = (
        len(ima_encode([0] * id34_fused_samples)) + PACK_ENTRY.size)
    id34_paired_add_bytes = (
        len(ima_encode([0] * id34_root_samples)) +
        len(ima_encode([0] * id34_fused_samples)) +
        2 * PACK_ENTRY.size)
    id34_feasibility = {
        "id": 34,
        "decision": "primary_source_aot",
        "measurement": "32_khz_ds_ima_storage_lower_bound",
        "source_root_program": 34,
        "source_fork_program": 658,
        "source_root_duration_ticks": id34_root_ticks,
        "source_fork_duration_ticks": id34_fork_ticks,
        "fused_minimum_samples": id34_fused_samples,
        "current_pack_bytes": len(pack),
        "pack_limit_bytes": MAX_RESIDENT_BYTES,
        "pack_headroom_bytes": MAX_RESIDENT_BYTES - len(pack),
        "fused_minimum_add_bytes": id34_fused_add_bytes,
        "fused_minimum_total_bytes": len(pack) + id34_fused_add_bytes,
        "fused_minimum_over_limit_bytes": (
            len(pack) + id34_fused_add_bytes - MAX_RESIDENT_BYTES),
        "paired_minimum_add_bytes": id34_paired_add_bytes,
        "paired_minimum_total_bytes": len(pack) + id34_paired_add_bytes,
        "paired_minimum_over_limit_bytes": (
            len(pack) + id34_paired_add_bytes - MAX_RESIDENT_BYTES),
        "excluded_from_lower_bound": (
            "AL_FX_CUSTOM delay output and acoustic-tail samples"),
        "conclusion": (
            "dry fused lower bound exceeds the resident cap; retain the exact "
            "primary BattleShip sample as the bounded P1 presentation"),
    }

    metadata = {
        "format": "BattleShip P1 FGM pack / Nintendo DS IMA ADPCM",
        "source_region": "REGION_US",
        "format_version": PACK_VERSION,
        "entry_count": len(records),
        "entry_bytes": PACK_ENTRY.size,
        "entry_final_u16": "ds_loop_point_words",
        "envelope_point_bytes": PACK_ENVELOPE_POINT.size,
        "header_bytes": PACK_HEADER.size,
        "resident_bytes": len(pack),
        "resident_limit_bytes": RUNTIME_CACHE_BYTES,
        "pack_limit_bytes": MAX_PACK_BYTES,
        "mapping_sha256": mapping_sha,
        "mapping_sha256_lo": f"0x{mapping_sha_lo:08x}",
        "pack_sha256": sha256(pack),
        "source_fgm_timer_microseconds": FGM_TIMER_MICROSECONDS,
        "source_fgm_output_rate_hz": FGM_OUTPUT_RATE,
        "runtime_conversion": False,
        "unique_sample_count": len(sample_offsets),
        "unique_sample_bytes": len(sample_body),
        "non_loop_sample_sha256": sha256(b"".join(
            record["ima"] for record in records
            if record["flags"] == 0)),
        "non_loop_envelope_sha256": sha256(b"".join(
            PACK_ENVELOPE_POINT.pack(point["tick"], point["ds_volume"], 0)
            for record in records if record["flags"] == 0
            for point in record["envelope"])),
        "strict_hit_contact_status": "full_source_program_aot",
        "runtime_excluded_hit_ids": [entry["id"]
                                     for entry in excluded_hit_cues],
        "excluded_hit_cues": excluded_hit_cues,
        "hit_contact_feasibility_experiment": id34_feasibility,
        "source_custom_fx": source_custom_fx_audit(repo_root),
        "known_runtime_fidelity_debt": [
            "Entries carrying articulation or UCD automation debt retain "
            "their source wavetable and bounded initial DS state, but their "
            "listed pitch or volume automation is not yet reproduced.",
            "AL_FX_CUSTOM cues currently ship their exact dry source-program "
            "render while the wet delay tail remains a named listen lever.",
        ],
        "attack_activation_qualification": {
            "source_action_audit": attack_actions,
            "source_custom_fx_bus_contract": attack_fx,
            "fgm_218_feasibility": fgm218_feasibility,
            **attack_cues,
        },
        "sources": {
            name: {
                "wrapped_sha256": sha256(source_wrapped[name]),
                "raw_sha256": sha256(source_raw[name]),
                "raw_bytes": len(source_raw[name]),
            }
            for name in source_raw
        },
        "prior_excluded_source_audit": excluded_entries,
        "excluded_entries": [],
        "entries": metadata_entries,
    }
    metadata["sources"]["source_sine_table"] = {
        "sha256": sha256(sine_source),
        "bytes": len(sine_source),
        "entries": len(sine_table),
    }
    return pack, metadata


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--repo-root", type=Path,
                        default=Path(__file__).resolve().parents[1])
    parser.add_argument("--out-bin", type=Path,
                        default=Path("assets/audio/fgm_phase_pack_ima.bin"))
    parser.add_argument("--out-json", type=Path,
                        default=Path("assets/audio/fgm_phase_pack_ima.json"))
    parser.add_argument("--check", action="store_true",
                        help="rebuild in memory and compare existing outputs")
    args = parser.parse_args()
    repo_root = args.repo_root.resolve()
    out_bin = args.out_bin
    out_json = args.out_json
    if not out_bin.is_absolute():
        out_bin = repo_root / out_bin
    if not out_json.is_absolute():
        out_json = repo_root / out_json

    pack, metadata = build_pack(repo_root)
    json_bytes = (json.dumps(metadata, indent=2, sort_keys=True) + "\n").encode()
    if args.check:
        failures = []
        if not out_bin.is_file() or out_bin.read_bytes() != pack:
            failures.append(str(out_bin))
        if not out_json.is_file() or out_json.read_bytes() != json_bytes:
            failures.append(str(out_json))
        if failures:
            print("stale FGM phase pack: " + ", ".join(failures),
                  file=sys.stderr)
            return 1
        print(f"FGM phase pack fixture PASS: {len(pack)} bytes, "
              f"sha256={metadata['pack_sha256']}")
        return 0

    out_bin.parent.mkdir(parents=True, exist_ok=True)
    out_json.parent.mkdir(parents=True, exist_ok=True)
    out_bin.write_bytes(pack)
    out_json.write_bytes(json_bytes)
    print(f"wrote {out_bin} ({len(pack)} bytes)")
    print(f"wrote {out_json}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
