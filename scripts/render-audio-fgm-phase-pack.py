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
import struct
import sys
from pathlib import Path


PACK_MAGIC = b"FGM1"
PACK_VERSION = 3
PACK_HEADER = struct.Struct("<4sHHII")
PACK_ENTRY = struct.Struct("<HHIIIHHBBHIHH")
PACK_ENVELOPE_POINT = struct.Struct("<HBB")
FGM_TIMER_MICROSECONDS = 5750
FGM_OUTPUT_RATE = 32000
MAX_RESIDENT_BYTES = 160 * 1024
PUBLIC_EXCITED_ID = 626
PUBLIC_EXCITED_SAMPLE_COUNT = 104204
PUBLIC_EXCITED_RAMP_SAMPLES = 184
PUBLIC_EXCITED_MIXER_MINIMUM = 1
PUBLIC_EXCITED_IMA_PREDICTOR = -4553
PUBLIC_EXCITED_IMA_INDEX = 65
PUBLIC_EXCITED_GUARD_NIBBLES = (8, 9)
PUBLIC_EXCITED_LOOP_POINT_WORDS = 1
REPEAT_ORACLE_CYCLES = 3

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
        "expected_retained_samples": 6688,
        "root_fork_programs": (287,),
        "root_program_sha256":
            "64523939186fd3d63f5440b5ec78784dac4e10c76456ebaa75671e4bfd9a85c2",
        "render_program_sha256":
            "634c9b1217b933f51dde97353d62e908fa1082943114d6dbe72bb188a3f33776",
        "articulation_program_sha256":
            "bbcff809d0113bec03d327dd08e85ef84fe10c8b18ba2f922b581416a958de0b",
        "fidelity_debt": ("articulation_pitch_modulation",),
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
        "expected_retained_samples": 6688,
        "root_fork_programs": (287,),
        "root_program_sha256":
            "64523939186fd3d63f5440b5ec78784dac4e10c76456ebaa75671e4bfd9a85c2",
        "render_program_sha256":
            "634c9b1217b933f51dde97353d62e908fa1082943114d6dbe72bb188a3f33776",
        "articulation_program_sha256":
            "bbcff809d0113bec03d327dd08e85ef84fe10c8b18ba2f922b581416a958de0b",
        "fidelity_debt": ("articulation_pitch_modulation",),
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
        "expected_retained_samples": 6688,
        "root_fork_programs": (298,),
        "root_program_sha256":
            "0a7645ae1249ff5140ddbf80859b52c127b73d2b80e0b97d90cc3b61b0c4b262",
        "render_program_sha256":
            "9ed69d587dab562768d6321d349477c4f522c0b65115fb7cb2c1f27d5b27c4c2",
        "articulation_program_sha256":
            "bbcff809d0113bec03d327dd08e85ef84fe10c8b18ba2f922b581416a958de0b",
        "fidelity_debt": ("articulation_pitch_modulation",),
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
        "fidelity_debt": (),
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
    },
    {
        "id": 42,
        "name": "nSYAudioFGMLightSwingM",
        "kind": "attack",
        "articulation": 174,
        "sound": 71,
        "notes": ((11, 7, 15), (11, 7, 20)),
        "duration_ticks": 35,
        "ucd_volume": 190,
        "articulation_pitch_cents": 550,
        "loop": False,
        "wave_base": 691264,
        "wave_length": 2916,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 5184,
        "root_fork_programs": (),
        "root_program_sha256":
            "85fe213b07b57a985e1083326f460eb79543547f3481362555daa4341c8995a1",
        "render_program_sha256":
            "85fe213b07b57a985e1083326f460eb79543547f3481362555daa4341c8995a1",
        "articulation_program_sha256":
            "7a17a55c4ba3daec625c6334dcb9189080b82bfea648864c1042b1d861f4e69f",
        "fidelity_debt": (),
    },
    {
        "id": 43,
        "name": "nSYAudioFGMLightSwingS",
        "kind": "attack",
        "articulation": 174,
        "sound": 71,
        "notes": ((8, 7, 15), (8, 7, 20)),
        "duration_ticks": 35,
        "ucd_volume": 160,
        "articulation_pitch_cents": 550,
        "loop": False,
        "wave_base": 691264,
        "wave_length": 2916,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 5184,
        "root_fork_programs": (),
        "root_program_sha256":
            "56f323a51d99829631b90e5f4cec63a30068e017f22ec39ea135b3e07dae333e",
        "render_program_sha256":
            "56f323a51d99829631b90e5f4cec63a30068e017f22ec39ea135b3e07dae333e",
        "articulation_program_sha256":
            "7a17a55c4ba3daec625c6334dcb9189080b82bfea648864c1042b1d861f4e69f",
        "fidelity_debt": (),
    },
    {
        "id": 190,
        "name": "nSYAudioFGMFoxAttackAirLw",
        "kind": "attack",
        "articulation": 174,
        "sound": 71,
        "notes": ((10, 7, 3), (15, 7, 4), (20, 7, 4),
                  (12, 7, 4), (5, 7, 10)),
        "duration_ticks": 25,
        "ucd_volume": 160,
        "articulation_pitch_cents": 550,
        "loop": False,
        "wave_base": 691264,
        "wave_length": 2916,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 5184,
        "root_fork_programs": (),
        "root_program_sha256":
            "84a6c9a138201870077c8f6d2461040e94494e28082790285687d58a9b27df40",
        "render_program_sha256":
            "84a6c9a138201870077c8f6d2461040e94494e28082790285687d58a9b27df40",
        "articulation_program_sha256":
            "7a17a55c4ba3daec625c6334dcb9189080b82bfea648864c1042b1d861f4e69f",
        "fidelity_debt": ("ucd_pitch_automation",),
    },
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
        "id": 218,
        "name": "nSYAudioFGMMarioUnkSwing1",
        "kind": "attack",
        "articulation": 174,
        "sound": 71,
        "notes": ((8, 7, 3), (13, 7, 4), (12, 7, 5),
                  (15, 7, 5), (8, 7, 10)),
        "duration_ticks": 27,
        "ucd_volume": 160,
        "articulation_pitch_cents": 550,
        "loop": False,
        "wave_base": 691264,
        "wave_length": 2916,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 5184,
        "root_fork_programs": (),
        "root_program_sha256":
            "e599e2bf74db3900b0e653a72c5d29a7330cabd17e5e05a5b5f9f91392446f23",
        "render_program_sha256":
            "e599e2bf74db3900b0e653a72c5d29a7330cabd17e5e05a5b5f9f91392446f23",
        "articulation_program_sha256":
            "7a17a55c4ba3daec625c6334dcb9189080b82bfea648864c1042b1d861f4e69f",
        "fidelity_debt": ("ucd_pitch_automation",),
    },
    {
        "id": 219,
        "name": "nSYAudioFGMMarioUnkSwing2",
        "kind": "attack",
        "articulation": 174,
        "sound": 71,
        "notes": ((10, 7, 3), (15, 7, 4), (20, 7, 4),
                  (12, 7, 4), (5, 7, 10)),
        "duration_ticks": 25,
        "ucd_volume": 160,
        "articulation_pitch_cents": 550,
        "loop": False,
        "wave_base": 691264,
        "wave_length": 2916,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 5184,
        "root_fork_programs": (),
        "root_program_sha256":
            "84a6c9a138201870077c8f6d2461040e94494e28082790285687d58a9b27df40",
        "render_program_sha256":
            "84a6c9a138201870077c8f6d2461040e94494e28082790285687d58a9b27df40",
        "articulation_program_sha256":
            "7a17a55c4ba3daec625c6334dcb9189080b82bfea648864c1042b1d861f4e69f",
        "fidelity_debt": ("ucd_pitch_automation",),
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
    if first_program_arg(program, "set_volume") != selector["ucd_volume"]:
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
    if pitch_modulated:
        retained_samples = len(pcm)
        strategy = "untrimmed_articulation_pitch_modulation"
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
    retained = pcm[:retained_samples]
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
        "trim_source_samples_removed": len(pcm) - retained_samples,
        "trim_applied": retained_samples < len(pcm),
        "trim_retained_source_prefix_pcm_sha256": ima_pcm_sha256(retained),
        "trim_retained_prefix_exact": retained == pcm[:retained_samples],
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


def build_pack(repo_root: Path) -> tuple[bytes, dict]:
    tools_dir = repo_root / "decomp/BattleShip-main/decomp/tools"
    extract_fgm = load_module(tools_dir / "extract_fgm.py", "extract_fgm")
    decode_ctl = load_module(tools_dir / "decode_ctl.py", "decode_ctl")
    audio_codec = load_module(tools_dir / "audio_codec.py", "audio_codec")
    audio_dir = repo_root / "decomp/BattleShip-main/BattleShip_o2r/audio"

    source_wrapped: dict[str, bytes] = {}
    source_raw: dict[str, bytes] = {}
    for name in ("fgm_tbl", "fgm_ucd", "B1_sounds2_ctl",
                 "B1_sounds2_tbl"):
        source_wrapped[name], source_raw[name] = read_o2r_payload(
            audio_dir / name)

    ucd = extract_fgm.decode_fgm_ucd(source_raw["fgm_ucd"])
    articulations = extract_fgm.decode_fgm_tbl(source_raw["fgm_tbl"])
    ctl_structs = decode_ctl.walk(source_raw["B1_sounds2_ctl"])
    ctl_by_offset = {entry["offset"]: entry for entry in ctl_structs}
    banks = [entry for entry in ctl_structs if entry["kind"] == "ALBank"]
    if len(banks) != 1 or banks[0]["sampleRate"] != 44100:
        raise ValueError("unexpected B1_sounds2 bank layout")
    bank = banks[0]
    if len(bank["instArray_offs"]) != 1:
        raise ValueError("unexpected B1_sounds2 instrument layout")
    instrument = ctl_by_offset[bank["instArray_offs"][0]]

    mapping_source = [
        {key: value for key, value in selector.items()}
        for selector in SELECTED
    ]
    mapping_json = json.dumps(mapping_source, sort_keys=True,
                              separators=(",", ":")).encode("utf-8")
    mapping_sha = sha256(mapping_json)
    mapping_sha_lo = int.from_bytes(bytes.fromhex(mapping_sha)[:4], "little")

    records = []
    metadata_entries = []
    for entry_index, selector in enumerate(SELECTED):
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
        if selector["id"] == PUBLIC_EXCITED_ID:
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
        else:
            runtime_pcm, trim = trim_proof(
                selector, ucd_program, pcm, frequency)
            acoustic_oracle = {}
            loop_strategy = "none"
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
            "root_ucd_program_id": selector["id"],
            "root_ucd_program": root_program,
            "render_ucd_program_id": render_program_id,
            "ucd_program": ucd_program,
            "root_fork_programs": list(selector.get(
                "root_fork_programs", ())),
            "omitted_fork_programs": list(selector.get(
                "omitted_fork_programs", ())),
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
                "pre_mixer target mapped from 0..32767 to DS 0..127"),
            "runtime_fidelity_debt": list(selector.get(
                "fidelity_debt", ())),
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
    if len(pack) > MAX_RESIDENT_BYTES:
        raise ValueError(
            f"FGM pack exceeds {MAX_RESIDENT_BYTES // 1024} KiB: "
            f"{len(pack)} bytes")

    metadata = {
        "format": "BattleShip P1 FGM pack / Nintendo DS IMA ADPCM",
        "format_version": PACK_VERSION,
        "entry_count": len(records),
        "entry_bytes": PACK_ENTRY.size,
        "entry_final_u16": "ds_loop_point_words",
        "envelope_point_bytes": PACK_ENVELOPE_POINT.size,
        "header_bytes": PACK_HEADER.size,
        "resident_bytes": len(pack),
        "resident_limit_bytes": MAX_RESIDENT_BYTES,
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
        "known_runtime_fidelity_debt": [
            "The regular KO and Fox movement entries retain their exact "
            "source wavetable, duration-proven prefix, initial pitch, and "
            "volume envelope; source pitch automation is not yet scheduled "
            "on DS channels.",
            "DeadExplodeL currently renders its primary UCD voice; forked "
            "source voice 685 remains explicit metadata fidelity debt.",
            "Attack entries 190, 218, and 219 retain their exact source "
            "wavetable, initial pitch, and duration; later UCD pitch changes "
            "are not yet scheduled on DS channels.",
        ],
        "sources": {
            name: {
                "wrapped_sha256": sha256(source_wrapped[name]),
                "raw_sha256": sha256(source_raw[name]),
                "raw_bytes": len(source_raw[name]),
            }
            for name in source_raw
        },
        "entries": metadata_entries,
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
