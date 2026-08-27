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
import sys as _sys
from pathlib import Path as _Path

_scripts_root = _Path(__file__).resolve().parent
while _scripts_root.name != "scripts":
    _scripts_root = _scripts_root.parent
if str(_scripts_root) not in _sys.path:
    _sys.path.insert(0, str(_scripts_root))
import _paths  # noqa: E402  -- puts every scripts/ area folder on sys.path



PACK_MAGIC = b"FGM1"
PACK_VERSION = 4
PACK_HEADER = struct.Struct("<4sHHII")
PACK_ENTRY = struct.Struct("<HHIIIHHBBHIHH")
PACK_ENVELOPE_POINT = struct.Struct("<HBB")
FGM_TIMER_MICROSECONDS = 5750
FGM_OUTPUT_RATE = 32000
# A ROM budget on a NitroFS payload, and nothing else -- the runtime never holds
# the pack. nds_audio_fgm.c reads a 1,808-byte header into .bss and streams each
# cue into the fixed 200 KiB slot cache below, so pack growth costs ROM, which
# PROJECT_GOAL says to spend. 512 KiB stopped the five P1 announcer lines
# (TIME UP, GAME SET, winner-is, Mario, Fox) at 526,928 bytes with no runtime
# reason, so it went to 768 KiB. 768 KiB then stopped the seven crowd cues at
# 887,160 bytes for the same non-reason, so it is 1 MiB now. Those seven leave
# the shared-sample-37 dedup when they start rendering their own note schedules
# and fork voices, which is where the growth comes from and is the point of the
# change; ROM is the cheapest thing this project spends.
#
# THE CONSTRAINT THAT IS REAL is the largest cache slot: a cue whose IMA body
# does not fit one can never be played, and until 2026-07-31 nothing checked it
# -- the pack cap was standing in for a bound it does not actually express.
# MAX_CUE_IMA_BYTES does now.
# P2-3 adds complete per-fighter voice banks.  This is a ROM-file ceiling, NOT
# the runtime cache: nds_audio_fgm continues to stream into the independently
# capped 200 KiB resident cache.  The old 1 MiB P1-era ceiling left only ~26 KiB
# after Luigi and could not admit DK's source voice bank without either dropping
# source cues or making a fake resident-RAM tradeoff.  Give the roster room to
# grow while keeping the real cache/slot gates unchanged and checked below.
MAX_PACK_BYTES = 2 * 1024 * 1024
RUNTIME_CACHE_BYTES = (52 * 1024) + (3 * 28 * 1024) + (4 * 16 * 1024)
MAX_CUE_IMA_BYTES = 52 * 1024
MAX_RESIDENT_BYTES = 128 * 1024  # historical Phase-C comparison only
PUBLIC_EXCITED_ID = 626
# 621 PublicWin is the SECOND cue on this wave, and it is the reason the
# renderer below stopped being keyed on 626's id. Both are articulation 460 /
# sound 320 / wave 2966600+15876 with the same `loop_start=1, loop_end=28215`;
# they differ only in note (12 vs 9), duration (1200 vs 950 ticks) and UCD
# volume. So the AOT loop-then-quadratic-ramp render is the SAME law, and the
# only 626-shaped thing in it was the pinned sample count -- now read off the
# selector. A DS hardware repeat cannot serve either of them: the articulation
# feeds a volume ramp across the loop, which is exactly what a hardware repeat
# reproduces bit-identically and therefore cannot ramp.
PUBLIC_WIN_ID = 621
PUBLIC_NO_CONTEST_ID = 624
LOOPED_FANFARE_AOT_IDS = frozenset((
    PUBLIC_EXCITED_ID, PUBLIC_WIN_ID, PUBLIC_NO_CONTEST_ID))
PUBLIC_EXCITED_SAMPLE_COUNT = 104204
PUBLIC_WIN_SAMPLE_COUNT = 69369
# 624 is the third reachable cue on articulation 460 / sound 320. Its source
# note is pitch 10 for 1200 ticks, so the same source-derived length law used by
# 621/626 gives ceil(1200 * 5750 us * 13454 Hz) = 92,833 samples.
PUBLIC_NO_CONTEST_SAMPLE_COUNT = 92833
PUBLIC_EXCITED_RAMP_SAMPLES = 184
PUBLIC_EXCITED_MIXER_MINIMUM = 1
PUBLIC_EXCITED_IMA_PREDICTOR = -4553
PUBLIC_EXCITED_IMA_INDEX = 65
PUBLIC_EXCITED_GUARD_NIBBLES = (8, 9)
PUBLIC_EXCITED_LOOP_POINT_WORDS = 1
REPEAT_ORACLE_CYCLES = 3
# BUGS.md #3.  Whispy's gust is the one packed cue whose source loop has to
# survive onto DS.  Its note schedule outlives the sample -- that is why
# trim_proof retains all 13360 samples rather than a shorter reachable
# prefix -- so a one-shot puffs once and stops while the hazard keeps
# pushing for the rest of the 470 ticks.
#
# 626 answered the same problem by rendering the loop out AOT, but that
# costs one buffer per second of sound and this cue runs 2.70 s.  A DS
# hardware loop costs nothing: the channel latches predictor/index when it
# reaches PNT and restores them on every repeat, so putting PNT at the
# first word after the IMA header makes the latched state the header state
# and every cycle decodes bit-identically by construction.
#
# The body is the source loop plus four samples of the source's own
# 12-sample tail, taken because 13300 nibbles do not fill whole words.
# Real audio for the alignment debt beats synthetic guard nibbles: the
# extra samples are what the source itself plays after loop_end.
WHISPY_WIND_ID = 285
WHISPY_WIND_LOOP_POINT_WORDS = 1
WHISPY_WIND_ALIGNMENT_TAIL_SAMPLES = 4
WHISPY_WIND_GUARD_NIBBLES = ()
WHISPY_WIND_IMA_PREDICTOR = 335
WHISPY_WIND_IMA_INDEX = 56
SOURCE_SINE_TABLE_SHA256 = (
    "bc184c0dbd76adecf7ff264d39cc58456546173beba727f189d2716dd8eabf16")

# P2-3 Samus gameplay audio that can already be represented exactly by the
# source-program AOT path.  Keep the genuinely harder sequencer cases OUT of
# this list: Charge0..7 loop forever with live articulation/modulator state,
# ShootF's one-pass AOT body exceeds the largest cache slot, and SpecialHi has
# a cross-voice modulator.  Those are separate representation work, not cues to
# flatten until they happen to fit.
SAMUS_NON_CHARGE_AUDIO = (
    (17, "nSYAudioFGMHeavySwing1"),
    (22, "nSYAudioFGMShockL"),
    (23, "nSYAudioFGMShockM"),
    (24, "nSYAudioFGMShockS"),
    (81, "nSYAudioFGMSamusLanding"),
    (92, "nSYAudioFGMSamusJumpAerial"),
    (103, "nSYAudioFGMGroundGrind4"),
    (114, "nSYAudioFGMSamusFoot"),
    (128, "nSYAudioFGMGroundBrakeGrind"),
    (236, "nSYAudioFGMSamusSpecialNShootL"),
    (237, "nSYAudioFGMSamusSpecialNShootM"),
    (238, "nSYAudioFGMSamusSpecialNShootS"),
    (247, "nSYAudioFGMSamusSpecialLw"),
    (248, "nSYAudioFGMSamusCatchGrappleBeam"),
    (250, "nSYAudioFGMSamusUnkSwing"),
    (251, "nSYAudioFGMSamusUnkCharge"),
    (296, "nSYAudioFGMSamusDeadSlam"),
    (307, "nSYAudioFGMSamusDownBounce"),
    (573, "nSYAudioVoiceSamusSmash1"),
    (574, "nSYAudioVoiceSamusSmash2"),
    (575, "nSYAudioVoiceSamusSmash3"),
    (576, "nSYAudioVoiceSamusDeadUp"),
    (577, "nSYAudioVoiceSamusFura"),
    (578, "nSYAudioVoiceSamusAttackHi4"),
    (579, "nSYAudioVoiceSamusUnkSlash"),
    (580, "nSYAudioVoiceSamusAppeal"),
    (581, "nSYAudioVoiceSamusDamage"),
    (582, "nSYAudioVoiceSamusDead"),
    (613, "nSYAudioVoicePublicSamus"),
    (639, "nSYAudioFGMCharacterUnkZip10"),
)
SAMUS_NON_CHARGE_RENDER_PROGRAMS = {
    114: 105,   # bare fork -> DonkeyFoot program
    296: 287,   # bare fork -> shared DeadSlam program
    307: 298,   # bare fork -> shared DownBounce program
    639: 630,   # bare CharacterUnkZip10 fork -> its actual voiced program
}
SAMUS_NON_CHARGE_SELECTOR_SHA256 = (
    "3bd0dabb33f7fb4df3944d807f169b297cac294c23f33b3ad5400d3a11d9ead4")

FULL_COVERAGE_IDS = (
    626, 470, 469, 467, 490, 74, 363, 364, 372, 373, 374, 430, 439,
    292, 370, 289, 300, 303, 154, 77, 215, 40, 38, 37, 34, 32, 31,
    375, 429, 431, 435, 440, 19, 41, 42, 43, 185, 186, 187, 189, 190,
    217, 218, 219, 216, 28, 2, 0, 188,
    # BUGS.md #4/#6/#8.  Appended rather than interleaved so the existing
    # entries keep their pack order; the mapping hash changes either way.
    436, 432, 362, 433, 360, 12, 285,
    # The announcer lines a P1 match reaches but could not play: TIME UP and
    # GAME SET at match end, then the Results sequence -- "this game's winner
    # is", then the winner's name, then the two countdown numbers above three.
    # Appended for the same reason.
    527, 488, 534, 499, 486, 472, 471,
    # And the crowd's win roar that opens the Results sequence.
    621,
    # BUGS.md crowd row: the eleven cues ft/ftpublic.c reaches in a P1 match --
    # Fox/Mario chants, three gasps, cheer, amazed, gasp-clap, three damage
    # reactions.
    605, 609, 615, 616, 617, 618, 619, 620, 622, 623, 625,
    # The miss ring's loudest survivor, the altitude warning, and the grind --
    # all three cues a natural match asked for and did not get on 2026-08-01.
    96, 153, 85,
    # And the five only a BOTH-CPU stress match reaches: dodge, shield on/off,
    # pause, and Fox's ledge teeter.
    11, 13, 14, 278, 369,
    # The last two the ring named once those five stopped appearing in it: the
    # zoom pulse and Fox's win voice.
    271, 368,
    # And three from the first run in which every fireball spawned and the match
    # reached SUDDEN DEATH: a light swing, the Sudden Death announcement, and
    # Fox's selection voice.
    18, 365, 514,
    # ACTIONABLE, NOT DONE: FGM 17 is the only id a clean 5-minute both-CPU soak
    # still asked for and did not get (2026-08-03) -- MissRingIDs[0]=17 twice,
    # UnsupportedCallCount=2, nothing else in the ring. Appending it here the way
    # every id above was appended raises KeyError: 17 at build_pack, because 17 is
    # in neither the declared selectors (SELECTED / EXCLUDED_SOURCE_CUES) nor
    # ATTACK_CUE_AUDIT, so the pack has no root-program hash to check it against.
    # Adding it therefore means authoring a new audit entry, not editing a list.
    # Left out deliberately: it costs two plays per match, and at id 17 it sits
    # among the swings (18 LightSwingLw1, 19 Catch), not with the crowd cues the
    # open BUGS.md row is about -- that row's cut-off was the release window, and
    # nds_audio_fgm.c:1014 already holds the fix.
    #
    # P2-1c-1. The 2D UI kit's SFX seam (src/nds/nds_ui_kit.c:ndsUiKitSfx,
    # sNdsUiKitSfxIds = {164, 158, 165}) already asks for these with the
    # source's own ids and the pack did not carry them -- proven by the miss
    # ring, not inferred (2026-08-17 P2-1c evidence:
    # `UKMISS ring=3 id0=164 c0=17 id1=165 c1=6`). Ids re-verified by fully
    # parsing gm/gmsound.h's gmFGMVoiceID enum with REGION_US honored (the
    # enum is not a line count -- REGION_US conditionals shift it by ten
    # entries before this run of names) and cross-checked against every id
    # this file already pins: Escape 11, GuardOn 13, GamePause 278,
    # FoxLanding 74, MarioLanding 77, UnkGrind4 85, AltitudeWarn 153,
    # DeadExplodeL 154 all land exactly where this file already has them,
    # then MenuSelect 158, MenuScroll1 163, MenuScroll2 164, MenuDenied 165
    # follow in the same parse. 163 MenuScroll1 has no live caller yet (the
    # kit only wires move/confirm/back to 164/158/165); packed anyway per the
    # board row's exact id list, for whichever P2-1d direction wires it.
    158, 163, 164, 165,
    # P2-1d-1. The title screen's own confirm cue, mntitle.c:501 --
    # ndsMenuShellUpdateTitle's seam already asks for it with the source's own
    # id and the pack did not carry it, proven by the miss ring rather than
    # inferred (P2-1d evidence: `MSMISS ring=1 id0=157 c0=1`, the only cue any
    # menu screen misses). Id re-verified by fully parsing gm/gmsound.h's
    # gmFGMVoiceID enum with REGION_US honored (a Python parser, not
    # hand-counting) and cross-checked against every id this file already
    # pins -- Escape 11, GuardOn 13, GamePause 278, FoxLanding 74,
    # MarioLanding 77, UnkGrind4 85, AltitudeWarn 153, DeadExplodeL 154,
    # MenuSelect 158, MenuScroll1 163, MenuScroll2 164, MenuDenied 165 all
    # land exactly where this file already has them, and 157 lands one below
    # MenuSelect's 158 -- nSYAudioFGMTitlePressStart, immediately before it in
    # the enum, exactly where the name says it should sit.
    157,
    # P2-1e-1. The character select's own audio seam (nds_menu_shell.c,
    # NDS_CSS_FGM_ANNOUNCE_WHOOSH/_GRAB/_SLOT_WHOOSH/NDS_CSS_VOICE_FREE_FOR_ALL)
    # already asks for these four with the source's own ids -- transcribed at
    # P2-1e landing so the gap could be MEASURED rather than guessed -- and the
    # pack did not carry them, proven by the miss ring, not inferred (2026-08-18
    # P2-1e evidence: `MSMISS ring=4 id0=512 c0=1 id1=127 c1=1 id2=121 c2=2
    # id3=167 c3=1`, counts x3 on the three-lap walk arm). Ids re-verified by
    # fully parsing gm/gmsound.h's gmFGMVoiceID enum with REGION_US honored (a
    # Python parser, not hand-counting) and cross-checked against all eighteen
    # ids this file already pins (thirteen FGM anchors -- Escape 11, GuardOn 13,
    # FoxLanding 74, MarioLanding 77, UnkGrind4 85, AltitudeWarn 153,
    # DeadExplodeL 154, TitlePressStart 157, MenuSelect 158, MenuScroll1 163,
    # MenuScroll2 164, MenuDenied 165, GamePause 278 -- plus five BGM anchors
    # 0/12/16/22/44 in gmMusicID) -- all landed exactly where this file already
    # has them before any of the four below was trusted: 121
    # nSYAudioFGMMarioDash (the CSS announce whoosh), 127 nSYAudioFGMSamusDash
    # (the token-grab whoosh), 167 nSYAudioFGMPlayerSlotWhoosh (the kind-toggle
    # whoosh), 512 nSYAudioVoiceAnnounceFreeForAll (the game-mode call at CSS
    # entry).
    121, 127, 167, 512,
    # P2-1f-1. The stage select's own confirm cue (NDS_SSS_FGM_CONFIRM in
    # nds_menu_shell.c, mnmaps.c:1470's A/START confirm) already asks for it
    # with the source's own id -- transcribed at the SSS screen's own landing
    # so the gap could be MEASURED rather than guessed -- and the pack did not
    # carry it, proven by the miss ring, not inferred (2026-08-18 P2-1f
    # evidence: `MSMISS ring=1 id0=159 c0=1` one-pass, `c0=3` three-lap). Id
    # re-verified by fully parsing gm/gmsound.h's gmFGMVoiceID enum with
    # REGION_US honored (a Python parser, not hand-counting) and cross-checked
    # against all eighteen ids this file already pins before trusting the new
    # one: 159 nSYAudioFGMStageSelect lands exactly one above MenuSelect's
    # 158, matching the name and matching this port's own hand-curated
    # `include/gm/gmsound.h` (`nSYAudioFGMStageSelect = 159` already present
    # there from earlier curation, unused for packing until this row) -- a
    # second, independent corroboration.
    159,
    # P2-1N (3)+(4). Two more cues the shell already ASKS for with the
    # source's own ids, gaps proven by the miss ring on the owner's own
    # build (2026-08-19): 166 nSYAudioFGMPlayerSlotClose -- the shutter's
    # arrival cue (mnPlayersVSShutterProcUpdate) -- and 526
    # nSYAudioVoiceAnnounceTeamBattle, the mode toggle's announcer line
    # (the FFA half, 512, has been packed since P2-1e-1). Both ids
    # re-verified against the port's hand-curated include/gm/gmsound.h
    # (nSYAudioFGMPlayerSlotClose = 166 one below the packed whoosh 167;
    # nSYAudioVoiceAnnounceTeamBattle = 526), and both derive as simple
    # single-voice articulations -- no forks, no loops.
    166,
    526,
    # P2-3 Luigi CSS: mnPlayersVSAnnounceFighter indexes fkind 4 to the source
    # announcer id 498. `--derive 498` proves this is a single 220-tick voice,
    # no forks/loops; append so every prior pack entry keeps its order.
    498,
    # P2-3 Luigi selected animation: dFTLuigiSubMotionDescs[1] runs
    # D_ovl1_80391754, which plays LuigiFuraFura twice. The focused CSS walk
    # reached this exact source command and the runtime miss ring named 421.
    421,
    # P2-3 Donkey Kong. BattleShip's DonkeyMain/MainMotion, CSS selected clip,
    # announcer table and ftpublic fighter-call table can reach the complete DK
    # voice run 324..336, announcer 483 and crowd chant 603. Keep the bank
    # contiguous and source-named rather than waiting for each omitted cue to
    # surface independently in the miss ring.
    324, 325, 326, 327, 328, 329, 330, 331, 332, 333, 334, 335, 336, 483, 603,
    # No Contest Results. The source Results announcer reaches both cues when
    # VSBattle exits through pause-quit (`is_reset`): announcer at tic 2, crowd
    # response at tic 71. They were previously omitted because only normal timed
    # results had been audited.
    502, 624,
    # P2-3 Captain Falcon. His ten FGM cues, twenty-two of his twenty-three
    # voices, his announcer line and his crowd chant. 356 FuraSleep is the one
    # omission and it is a MEASURED one -- see the block comment on his
    # selectors above. Appended so every prior entry keeps its pack order.
    73, 106, 117, 180, 181, 182, 183, 184, 288, 299,
    337, 338, 339, 340, 341, 342, 343, 344, 345, 346, 347, 348, 349, 350,
    351, 352, 353, 354, 355, 357, 358, 359, 485, 604,
    # P2-3f14 Donkey Kong's FGM bank -- his voices landed at P2-3 and his
    # sound effects never did. Every FGM cue 212_DonkeyMainMotion.c,
    # 213_DonkeyMain.c and the shared DownBounce table reach, plus the shared
    # 10 DonkeySlap2 that 175/176 fork and Captain/Kirby/Purin/Yoshi also
    # play. Appended so every prior entry keeps its pack order.
    9, 10, 72, 105, 116, 175, 176, 177, 178, 179, 287, 298,
    # P2-3f15 Luigi's voice bank -- the contiguous source run 416..428 minus
    # the already-packed 421, plus his crowd chant 608. He had two cues packed
    # and every gameplay voice failing closed. Appended so every prior entry
    # keeps its pack order.
    416, 417, 418, 419, 420, 422, 423, 424, 425, 426, 427, 428, 608,
    # P2-3f16 fighter entry audio. BattleShip's entry motion scripts request
    # 214 for both Mario and Luigi's pipe, 191 for Fox's Arwing and 59 for DK's
    # barrel smash. Appended so all previous pack ordinals remain stable.
    214, 191, 59,
    # P2-3 Samus CSS. The real shell walk reaches both source-owned cues as
    # soon as Samus becomes resident/selectable: mnPlayersVSAnnounceFighter
    # asks for 513 (AnnounceSamus), while dFTSamusSubMotionDescs[3]/[4] run
    # scsubsysdatasamus.c's selected script and play 264 (BladeDraw) at tic 100.
    # Both were named by the runtime miss ring; append so prior ordinals stay
    # stable. Gameplay-bank closure follows in the Samus fighter row rather
    # than hiding these two menu failures behind a silent fallback.
    513, 264,
    # P2-3 Samus gameplay bank, excluding only the five representation classes
    # named above (Charge0..7 is one class).  Appending preserves every previous
    # pack ordinal and makes the roster addition complete by source inventory,
    # rather than waiting for random CPU play to discover each miss.
    *(fgm_id for fgm_id, _name in SAMUS_NON_CHARGE_AUDIO),
)
FULL_PROGRAM_AOT_IDS = frozenset((
    154, 40, 38, 37, 34, 32, 31,
    375, 429, 431, 435, 440, 19, 41, 42, 43, 185, 186, 187, 189, 190,
    217, 218, 219, 216, 28, 2, 0, 188,
    # 85's first note asks for 90,510 Hz, past the pack entry's u16 frequency
    # field -- the same `source_rate_above_u16` blocker 189/190/219 carry, and
    # the same answer: bake the note schedule into the samples and store 32,000.
    85,
    # Escape, GuardOn, GuardOff, GamePause, FoxOttotto. All five carry multi-note
    # schedules with no forks, which is exactly what this render is for.
    11, 13, 14, 278, 369,
    # Magnify: five blips and five rests. The rest ticks matter to the timing,
    # which is the whole reason a schedule gets baked rather than approximated.
    271,
    # LightSwingLw1 and FoxSelected: two and three notes, no forks.
    # P2-1N (3). 166 nSYAudioFGMPlayerSlotClose: six notes, no forks -- the
    # same multi-note no-fork class as Escape/GuardOn above.
    166,
    # P2-3 LuigiFuraFura: four notes, no forks. Preserve the source note
    # schedule AOT instead of holding the first note for the whole cue.
    421,
    # P2-3 Captain Falcon's twenty-two voices and both Falcon Punch halves.
    # Every one is a multi-note schedule; 183/184 are bare fork roots whose
    # render program (186/187) carries the schedule, and 187 forks again --
    # which is exactly what the composite renderer is for.
    183, 184,
    337, 338, 339, 340, 341, 342, 343, 344, 345, 346, 347, 348, 349, 350,
    351, 352, 353, 354, 355, 357, 358, 359,
    # P2-3f14 Donkey Kong. Nine of his eleven FGM cues are multi-note
    # schedules; 175 BossSlam and 176 BossUnk1 additionally fork 10
    # DonkeySlap2, which the composite renderer fuses. The flat path would
    # hold each cue's FIRST note for its whole duration -- 179 DonkeyCharge
    # is pitch codes 13/3/12 in nine ticks and would come out monotone.
    9, 10, 175, 176, 177, 178, 179,
    # P2-3f15 Luigi's twelve unpacked voices. Every one is a multi-note
    # schedule with no forks -- the same class 421 FuraFura already joined
    # above, and the same class Falcon's twenty-two are in.
    416, 417, 418, 419, 420, 422, 423, 424, 425, 426, 427, 428,
    # P2-3f16 entry audio. 214's two pitch-code-0 RESTS are the pipe rhythm;
    # 191 and 59 are also multi-note schedules. Baking all three preserves the
    # source sequencer exactly instead of holding the first note for the cue.
    214, 191, 59,
    # P2-3 Samus CSS selected pose. 264 BladeDraw is two consecutive 50-tick
    # notes on one voice (the second does NOT set starts_new_voice). Preserve
    # that source sequencer/control-field transition by baking the full 100-tick
    # program AOT; it is far below the cache-slot ceiling.
    264,
    # Samus's bounded gameplay cues. The source-program renderer preserves note
    # changes, wavetable loops used inside finite notes, deterministic
    # articulation modulation, and forked voices before producing DS IMA.
    *(fgm_id for fgm_id, _name in SAMUS_NON_CHARGE_AUDIO),
    18, 365,
    # 153 AltitudeWarn -- the cue the owner picked out BY NAME as "a new SFX I
    # don't recognise". Articulation 150 sweeps pitch 550 -> 2390 cents inside
    # mark_loop/jump_loop, and it sat on the DS hardware-repeat path, which this
    # file already records as unable to ramp. Full note on the selector.
    153,
    # 12 DeadUpStar, the other cue the owner named by filename. It declared
    # source_loop_infinite and shipped ds_loop_flag 0 -- debt
    # `source_loop_not_reproduced` -- so it sounded for 0.425 s of a 0.863 s
    # note and stopped halfway. Articulation 83 ends with `vol 127, 150`, a
    # 150-tick hold, which the flat path cannot express and this render can.
    12,
    # BUGS.md "Some Crowd noise audio cues get cut off (the for big hits)".
    # Every OTHER mechanism for that row is now measured and cleared: the cue
    # rates are arithmetically right, the two ds_volume 0 cues are real
    # fade-ins, ndsAudioFgmUpdate does step envelopes, the assets are not
    # truncated (ds_trailing_samples_dropped 0 across the family), and channel
    # contention reads 0 premature retires against 188 channel reuses in a
    # 5-minute both-CPU soak. What is left is the debt these seven declared all
    # along, and it is not subtle:
    #   615, 618, 620, 625  ucd_pitch_automation -- the flat path bakes the
    #       FIRST note's rate for the whole cue, so 620 holds its opening
    #       53,786 Hz through a schedule that falls to 47,918 and the gasp runs
    #       ~10% fast and stops early.
    #   616, 618, 619, 620, 623  omitted_fork_voice -- the source layers a
    #       SECOND voice on top and the pack rendered only the first. 623
    #       DamageM forks 625 DamageS, so the big-hit crowd reaction the owner
    #       named is literally half its source. That is the "cut off".
    # This render answers both at once: it walks the note schedule and mixes
    # the simultaneous forks, which is what the seven existing fused repairs
    # already do.
    615, 616, 618, 619, 620, 623, 625,
    # 617 AND 622 BELONG HERE TOO, and the reasoning that excluded them was
    # asking the wrong question (owner refiled the row 2026-08-06 pointing at
    # fgm622 and saying the sample itself sounds incomplete -- it does).
    #
    # The old note kept 605, 609, 617 and 622 on the flat path because "their
    # only DECLARED debt is untrimmed_shared_source_reuse, a dedup note, not a
    # defect". True as far as it goes: neither has a fork and neither has pitch
    # automation. But the test never asked whether the cue has a MULTI-NOTE
    # SCHEDULE, which the flat path also cannot express -- it renders one
    # one-shot, so every note after the first is silence.
    #
    #   617 GaspS   notes (6,7,70)(6,7,180)          250 ticks = 1,437 ms
    #               sample 33,408 @ 29,344 Hz                  = 1,138 ms  -299
    #   622 DamageL notes (7,7,80)(7,7,100)(7,7,200)  380 ticks = 2,185 ms
    #               sample 44,800 @ 31,089 Hz                  = 1,441 ms  -744
    #
    # 605 and 609 carry `pitch_code`, not `notes` -- genuinely single-note, so
    # they stay flat and the dedup note really is their only debt.
    #
    # Tick math is validated against this file's own figure: 623 is 150 ticks
    # and nds_audio_fgm.c calls it an "862 ms note"; 150 * 5.75 ms = 862.5.
    617, 622,
    # P2-1e-1. 121 nSYAudioFGMMarioDash forks straight to 118 FoxDash (no local
    # notes of its own) and 118's first note (pitch code 20, +700 cents) asks
    # for 71,838 Hz -- past the pack entry's u16 frequency field, the same
    # `source_rate_above_u16` shape as 85/189/190/219 above. Same answer: bake
    # the note schedule into the samples and store FGM_OUTPUT_RATE.
    121,
    # P2-1f-1. 159 nSYAudioFGMStageSelect is not a fork-only root like 121 --
    # its own program has a real local note (pitch code 6, 180 ticks) -- but it
    # ALSO forks TWO voices at tick 0, before that note: 163 MenuScroll1 (its
    # own two-note program) and 6 UnkSmallPing1 (its own five-note program).
    # That is the same "local notes plus fork(s)" shape as 154/616/618/619/
    # 620/623/625 above, and the flat path can express only one voice, so a
    # flat render would omit both layers -- the exact `omitted_fork_voice`
    # debt this file spent 2026-08-02/08-06 clearing off the crowd cues.
    # Nothing forces the omission here: both forks are cheap (33/15 ticks) and
    # the fused render this file already proves correct for 154/616-625/121
    # handles two simultaneous forks exactly the same way it handles one --
    # render_fgm_composite_aot mixes every voice in root_meta["forks"], not
    # just the first. Full-program AOT keeps the confirm chime complete
    # instead of shipping a shorter, quieter partial of it.
    159,
    # P2-3 DK's fighter voices are multi-note source programs. The
    # flat pack path holds only the first note/rate; full-program AOT preserves
    # every note, reset, source loop and articulation change at the 32 kHz DS
    # output rate.  324 is deliberately NOT in this set: its three 400/410-tick
    # retriggers bake to 112 KiB of ADPCM, larger than the real 52 KiB runtime
    # cache slot.  Its selector below uses compact source-note replay instead --
    # one source sample plus two timed hardware retriggers -- so ROM and RAM do
    # not scale with the 7.0-second source schedule. Announcer 483 and crowd
    # chant 603 are single-note and stay on the cheaper flat path.
    325, 326, 327, 328, 329, 330, 331, 332, 333, 334, 335, 336,
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
    # The five announcer lines the P1 match still played silently. Every field
    # below came out of `--derive`, not out of a guess: the countdown four above
    # were hand-authored and that is why this row read as "an extraction job per
    # cue". They are the same shape -- one articulation, one trigger, one note,
    # no forks, no loop -- and they continue the same articulation/sound/wave
    # runs (334/211 -> 336/213 -> 337/214 -> 338/215).
    {
        "id": 527,
        "name": "nSYAudioVoiceAnnounceTimeUp",
        "articulation": 336,
        "sound": 213,
        "pitch_code": 13,
        "duration_ticks": 150,
        "ucd_volume": 230,
        "articulation_pitch_cents": -1200,
        "loop": False,
        "wave_base": 1794968,
        "wave_length": 9532,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 13801,
    },
    {
        # Two notes, not one: a 60-tick rest at pitch 0 before the line, which
        # is why this cannot use the pitch_code/duration_ticks shorthand.
        "id": 488,
        "name": "nSYAudioVoiceAnnounceGameSet",
        "articulation": 337,
        "sound": 214,
        "notes": ((0, 7, 60), (13, 7, 150)),
        "duration_ticks": 210,
        "ucd_volume": 230,
        "articulation_pitch_cents": -1200,
        "loop": False,
        "wave_base": 1804504,
        "wave_length": 9054,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 16096,
    },
    {
        "id": 534,
        "name": "nSYAudioVoiceAnnounceWinnerIs",
        "articulation": 338,
        "sound": 215,
        "pitch_code": 13,
        "duration_ticks": 350,
        "ucd_volume": 245,
        "articulation_pitch_cents": -1200,
        "loop": False,
        "wave_base": 1813560,
        "wave_length": 14482,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 25744,
    },
    {
        "id": 499,
        "name": "nSYAudioVoiceAnnounceMario",
        "articulation": 309,
        "sound": 186,
        "pitch_code": 13,
        "duration_ticks": 150,
        "ucd_volume": 230,
        "articulation_pitch_cents": -1200,
        "loop": False,
        "wave_base": 1526488,
        "wave_length": 7264,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 12912,
    },
    {
        "id": 486,
        "name": "nSYAudioVoiceAnnounceFox",
        "articulation": 312,
        "sound": 189,
        "pitch_code": 13,
        "duration_ticks": 150,
        "ucd_volume": 230,
        "articulation_pitch_cents": -620,
        "loop": False,
        "wave_base": 1552392,
        "wave_length": 13824,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 19294,
    },
    # And the two the miss ring caught only after the five above stopped
    # appearing in it: the countdown announces FIVE and FOUR before the three
    # that were already packed. Nobody had noticed, because until the ring
    # printed ids rather than a count there was nothing to notice.
    {
        "id": 472,
        "name": "nSYAudioVoiceAnnounceFive",
        "articulation": 329,
        "sound": 206,
        "pitch_code": 13,
        "duration_ticks": 90,
        "ucd_volume": 240,
        "articulation_pitch_cents": -1200,
        "loop": False,
        "wave_base": 1746032,
        "wave_length": 6256,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 8281,
    },
    {
        "id": 471,
        "name": "nSYAudioVoiceAnnounceFour",
        "articulation": 330,
        "sound": 207,
        "pitch_code": 13,
        "duration_ticks": 90,
        "ucd_volume": 220,
        "articulation_pitch_cents": -1200,
        "loop": False,
        "wave_base": 1752288,
        "wave_length": 5590,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 8281,
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
        # Same reason as 439 below, and more pronounced: the notes descend
        # 13 -> 12 -> 11 -> 10 and the last is 150 of the 236 ticks, so this cue
        # is a falling yell and the flat path rendered it as one held pitch.
        # The owner's other trigger for the unfamiliar sound is "when i knock
        # him off stage via a big hit", which is when this plays.
        "aot_source_schedule": True,
        "fidelity_debt": (),
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
        # Rendered on the source schedule so the note pitches survive. The notes
        # are (13, 13, 13, 12): the cue FALLS a semitone on its last and longest
        # note, and the flat single-rate path baked one pitch for all four and
        # threw that away. On a death cry the fall is the recognisable part, and
        # the owner reports an unfamiliar sound "right before someone dies via
        # upwards KO boundary" -- which is this cue's trigger exactly.
        "aot_source_schedule": True,
        "fidelity_debt": (),
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
    # BUGS.md #4: Mario's grounded jump voice (435) was packed but the aerial
    # jump voice was not, so the first jump was audible and the double jump was
    # silent.  Source callsite: decomp/src/relocData/202_MarioMainMotion.c:118,
    # ftMotionPlayVoice(nSYAudioVoiceMarioJumpAerial).
    {
        "id": 436,
        "name": "nSYAudioVoiceMarioJumpAerial",
        "kind": "voice",
        "articulation": 303,
        "sound": 180,
        "notes": ((12, 7, 6), (13, 7, 20), (12, 7, 60), (12, 7, 40)),
        "duration_ticks": 126,
        "ucd_volume": 180,
        "articulation_pitch_cents": -1199,
        "loop": False,
        "wave_base": 1484576,
        "wave_length": 5598,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 9952,
        "root_fork_programs": (),
        "root_program_sha256":
            "9be7d608e449e9eab8752772afa2a48acd18344db452a8d28a28028cd42f3695",
        "render_program_sha256":
            "9be7d608e449e9eab8752772afa2a48acd18344db452a8d28a28028cd42f3695",
        "articulation_program_sha256":
            "6c2e50943202a72b9efecd7c0ec688e260eac0dc14b6eed710341424dffe6561",
        "fidelity_debt": (),
    },
    # BUGS.md #6: Mario down-B (tornado) voice.  Source callsites:
    # 202_MarioMainMotion.c:920, :1389, :1415.
    {
        "id": 432,
        "name": "nSYAudioVoiceMarioSpecialLw",
        "kind": "voice",
        "articulation": 299,
        "sound": 176,
        "notes": ((12, 7, 6), (12, 7, 20), (12, 7, 30), (11, 7, 40),
                  (11, 7, 60)),
        "duration_ticks": 156,
        "ucd_volume": 180,
        "articulation_pitch_cents": -1199,
        "loop": False,
        "wave_base": 1444144,
        "wave_length": 8524,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 13556,
        "root_fork_programs": (),
        "root_program_sha256":
            "64b6d773fa65de93b0254fc29bd4ec68c5eacf65bd3bd97910881840323377d5",
        "render_program_sha256":
            "64b6d773fa65de93b0254fc29bd4ec68c5eacf65bd3bd97910881840323377d5",
        "articulation_program_sha256":
            "3dadacc3929a34bd0a16a6c73cb8302ef025a6f09d649b1a4b8d2dea1e3eb720",
        "fidelity_debt": (),
    },
    # BUGS.md #6: Fox up-B (Firefox) voice.  Source callsites:
    # 208_FoxMainMotion.c:1529, :1547, :1559, :1577.
    {
        "id": 362,
        "name": "nSYAudioVoiceFoxSpecialHi",
        "kind": "voice",
        "articulation": 228,
        "sound": 109,
        "notes": ((13, 7, 50), (13, 7, 50), (13, 7, 25)),
        "duration_ticks": 125,
        "ucd_volume": 230,
        "articulation_pitch_cents": -1200,
        "loop": False,
        "wave_base": 924192,
        "wave_length": 5868,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 10432,
        "root_fork_programs": (),
        "root_program_sha256":
            "0cc403a4f1dd31785a4162ca950246a5785a04602260565d69cae2334daa1740",
        "render_program_sha256":
            "0cc403a4f1dd31785a4162ca950246a5785a04602260565d69cae2334daa1740",
        "articulation_program_sha256":
            "750323e3e1db3ac8691fbf442899e4beeee66267591e8b1b974170ac9b13da30",
        "fidelity_debt": (),
    },
    # BUGS.md #8: the two star-KO voices.  ftCommonDeadUpStarSetStatus plays
    # fp->attr->deadup_sfx, which 203_MarioMain.c:277 and 209_FoxMain.c:298 set
    # to these IDs.  Neither was packed, so the upward KO was silent.
    {
        "id": 433,
        "name": "nSYAudioVoiceMarioDeadUp",
        "kind": "voice",
        "articulation": 300,
        "sound": 177,
        "notes": ((13, 7, 70), (13, 7, 60), (12, 7, 130), (12, 7, 300),
                  (12, 7, 200)),
        "duration_ticks": 760,
        "ucd_volume": 210,
        "articulation_pitch_cents": -1199,
        "loop": False,
        "wave_base": 1452672,
        "wave_length": 24768,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 44032,
        "root_fork_programs": (),
        "root_program_sha256":
            "105de9339b91cd01414a46e411c34bb0f8ca527866b3ee1b592402e6d5979ed2",
        "render_program_sha256":
            "105de9339b91cd01414a46e411c34bb0f8ca527866b3ee1b592402e6d5979ed2",
        "articulation_program_sha256":
            "f29a67af46018219ed4f286d7640121d16fd0915e092c9da48cd0289e8ac2d6e",
        "fidelity_debt": (),
    },
    {
        "id": 360,
        "name": "nSYAudioVoiceFoxDeadUp",
        "kind": "voice",
        "articulation": 233,
        "sound": 114,
        "notes": ((13, 7, 100), (13, 7, 100), (13, 7, 100), (13, 7, 30)),
        "duration_ticks": 330,
        "ucd_volume": 236,
        "articulation_pitch_cents": -1200,
        "loop": False,
        "wave_base": 957768,
        "wave_length": 16290,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 28960,
        "root_fork_programs": (),
        "root_program_sha256":
            "64f2f0543e44242ec4070d378bb433050a7828133e3df124abeaef56b10231d0",
        "render_program_sha256":
            "64f2f0543e44242ec4070d378bb433050a7828133e3df124abeaef56b10231d0",
        "articulation_program_sha256":
            "efd2adf87e736a73856f6224ad762ca518b4a903d4cdb2473d8fc4f7faac59bd",
        "fidelity_debt": (),
    },
    # BUGS.md #8: the star-KO spark itself, played from C rather than a motion
    # script (ftcommondead.c:363), which is why it has no relocData callsite.
    # The source wave carries a tail loop the DS entry format does not
    # reproduce, so this is declared the same way DeadExplodeL is.
    {
        "id": 12,
        "name": "nSYAudioFGMDeadUpStar",
        # Deliberately not "ko": that kind is the regular side/down-KO contract
        # (439/292/370/289/154) asserted by check-audio-runtime-fixtures.ps1 and
        # mirrored by ndsAudioFgmKoIndex.  This is the star KO.
        "kind": "starko",
        "articulation": 83,
        "sound": 40,
        "notes": ((13, 7, 100), (13, 7, 50)),
        "duration_ticks": 150,
        "ucd_volume": 180,
        "articulation_pitch_cents": 0,
        "loop": False,
        "source_loop_infinite": True,
        "wave_base": 344720,
        "wave_length": 7660,
        "loop_start": 11619,
        "loop_end": 13590,
        "expected_retained_samples": 13616,
        "root_fork_programs": (),
        "root_program_sha256":
            "896391657abdce0d470fd87284b3c94926c9b6e41cd9b18df9fdde73336b0f9c",
        "render_program_sha256":
            "896391657abdce0d470fd87284b3c94926c9b6e41cd9b18df9fdde73336b0f9c",
        "articulation_program_sha256":
            "7a3da92cfdceb3f4ab27bc8b5344f28c59d98074ff1d352f64aea08d82397fee",
        "fidelity_debt": ("source_loop_not_reproduced",),
    },
    # BUGS.md #3: Whispy's wind gust.  Unlike the star-KO ping this one loops
    # over nearly its whole body (48..13348 of 13360), which is what makes it a
    # sustained gust rather than a one-shot, so it is the only packed cue that
    # ships as a DS hardware loop -- see the WHISPY_WIND_* constants.  The
    # runtime's duration clock ends it after the source's 470 ticks.
    {
        "id": 285,
        "name": "nSYAudioFGMPupupuWhispyWind",
        "kind": "stage",
        "articulation": 451,
        "sound": 3,
        "notes": ((12, 7, 220), (12, 7, 250)),
        "duration_ticks": 470,
        "ucd_volume": 255,
        "articulation_pitch_cents": -1200,
        "loop": True,
        "wave_base": 21040,
        "wave_length": 7516,
        "loop_start": 48,
        "loop_end": 13348,
        "expected_retained_samples": 13360,
        "hardware_loop": {
            "alignment_tail_samples": WHISPY_WIND_ALIGNMENT_TAIL_SAMPLES,
            "loop_point_words": WHISPY_WIND_LOOP_POINT_WORDS,
            "guard_nibbles": WHISPY_WIND_GUARD_NIBBLES,
            "ima_predictor": WHISPY_WIND_IMA_PREDICTOR,
            "ima_index": WHISPY_WIND_IMA_INDEX,
        },
        "root_fork_programs": (),
        "root_program_sha256":
            "dbcc2506dda733515bcb1857723b257b8289c34044682ee0f4ccbc8a300a43d6",
        "render_program_sha256":
            "dbcc2506dda733515bcb1857723b257b8289c34044682ee0f4ccbc8a300a43d6",
        "articulation_program_sha256":
            "e68756e5a496a341437e5d376744e9982f8d0bea7ffab502c1b17b1c002fd90c",
        "fidelity_debt": (),
    },
    # BUGS.md Results row, the last FGM on it.  621 PublicWin is the crowd's
    # win roar, queued at Results scene start (mnvsresults.c) 81 ticks before
    # "this game's winner is".  It is the second cue on 626's wave and takes
    # 626's render law unchanged -- see LOOPED_FANFARE_AOT_IDS.  Appended, so
    # the existing pack order is untouched.
    {
        "id": 621,
        "name": "nSYAudioVoicePublicWin",
        "kind": "results",
        "articulation": 460,
        "sound": 320,
        "pitch_code": 9,
        "duration_ticks": 950,
        "ucd_volume": 190,
        "articulation_pitch_cents": -1200,
        "loop": True,
        "wave_base": 2966600,
        "wave_length": 15876,
        "loop_start": 1,
        "loop_end": 28215,
        "expected_retained_samples": PUBLIC_WIN_SAMPLE_COUNT,
    },
    # BUGS.md crowd row.  The eleven cues a P1 Mario-vs-Fox match can actually
    # reach, all of them requested by ft/ftpublic.c once the actor is imported
    # (NDS_IMPORT_BATTLESHIP_FT_PUBLIC): the two chants
    # dFTCommonDataPublicFighterCallFGMs picks for Mario (609) and Fox (605),
    # and the nine reactions ftPublicDecideCall/DecideCommon/PlayCliffReact
    # choose between. No Contest is packed separately below because Results
    # reaches it only on the pause-quit / `is_reset` branch.
    #
    # Every field came out of `--derive`, including the three that used to be
    # unobtainable without running the generator and reading its error
    # (source_pcm_samples, the fork program hashes, render_program_sha256).
    #
    # SHAPE, and why it is not the AOT path: these are one-shot crowd noises
    # with a two-to-four note schedule and, on five of them, one fork voice.
    # Rendering the schedule AOT at 32 kHz would cost ~212 KB for cues whose
    # audible content is a crowd; retaining the source wavetable and playing it
    # at the first note's rate is what the already-shipping punch/kick cues do
    # (40/38/37), for the same reason, and it is the trade PROJECT_GOAL's
    # sacrifice order names first.  The debt is recorded per entry.
    #
    # 616/618/620/622/625 share wave 298216+25200 and 617/623 share
    # 323416+18792, which is why retain_full_source is on all of them: a shared
    # wave cannot be trimmed to one cue's schedule.
    {
        "id": 605,
        "name": "nSYAudioVoicePublicFox",
        "kind": "crowd",
        "articulation": 122,
        "sound": 53,
        "pitch_code": 13,
        "duration_ticks": 320,
        "ucd_volume": 255,
        "articulation_pitch_cents": -1190,
        "loop": False,
        "wave_base": 460432,
        "wave_length": 17820,
        "loop_start": 0,
        "loop_end": 0,
        "retain_full_source": True,
        "expected_retained_samples": 31680,
        "root_fork_programs": (),
        "root_program_sha256":
            "60cadbe007e6410eaa6fc470f1e3f3cc22b84b1b18c615fb08d528216ef915d9",
        "render_program_sha256":
            "60cadbe007e6410eaa6fc470f1e3f3cc22b84b1b18c615fb08d528216ef915d9",
        "articulation_program_sha256":
            "6c3344f1fed0f3caf4942824e428e01afef7a7c285a810d94fa884213f396ddf",
        "fidelity_debt": ("untrimmed_shared_source_reuse",),
    },
    {
        "id": 609,
        "name": "nSYAudioVoicePublicMario",
        "kind": "crowd",
        "articulation": 126,
        "sound": 57,
        "pitch_code": 13,
        "duration_ticks": 320,
        "ucd_volume": 255,
        "articulation_pitch_cents": -1190,
        "loop": False,
        "wave_base": 533080,
        "wave_length": 19216,
        "loop_start": 0,
        "loop_end": 0,
        "retain_full_source": True,
        "expected_retained_samples": 34160,
        "root_fork_programs": (),
        "root_program_sha256":
            "c0a8858b35f1271e067f31d5050bfe0a755a08ca2e91ce9ec4d0b3defa627360",
        "render_program_sha256":
            "c0a8858b35f1271e067f31d5050bfe0a755a08ca2e91ce9ec4d0b3defa627360",
        "articulation_program_sha256":
            "6eeeeeaefb1399d9f88250fd74fa1ae1121c5f7ca850319b777df58100051e09",
        "fidelity_debt": ("untrimmed_shared_source_reuse",),
    },
    {
        "id": 615,
        "name": "nSYAudioVoicePublicGaspL",
        "kind": "crowd",
        "articulation": 461,
        "sound": 321,
        "notes": ((13, 7, 50), (12, 7, 260)),
        "duration_ticks": 310,
        "ucd_volume": 255,
        "articulation_pitch_cents": -1200,
        "loop": False,
        "wave_base": 2982480,
        "wave_length": 16272,
        "loop_start": 0,
        "loop_end": 0,
        "retain_full_source": True,
        "expected_retained_samples": 28928,
        "root_fork_programs": (),
        "root_program_sha256":
            "18cf80735120d4c33787f5f2826b92666e2e40295442210d70594aa21d24abe1",
        "render_program_sha256":
            "18cf80735120d4c33787f5f2826b92666e2e40295442210d70594aa21d24abe1",
        "articulation_program_sha256":
            "84245c84db4272b08f9d0072687d1a0c6b821f478766b2eb47db832591889e6c",
        "fidelity_debt": ("ucd_pitch_automation",
                          "untrimmed_shared_source_reuse"),
    },
    {
        "id": 616,
        "name": "nSYAudioVoicePublicGaspM",
        "kind": "crowd",
        "articulation": 149,
        "sound": 37,
        "notes": ((5, 7, 100), (5, 7, 100), (5, 7, 30)),
        "duration_ticks": 230,
        "ucd_volume": 255,
        "articulation_pitch_cents": 550,
        "loop": False,
        "wave_base": 298216,
        "wave_length": 25200,
        "loop_start": 0,
        "loop_end": 0,
        "retain_full_source": True,
        "expected_retained_samples": 44800,
        "root_fork_programs": (650,),
        "omitted_fork_programs": (650,),
        "root_program_sha256":
            "4bc9881e6f0ac7b9ebf19aa4c295525e4d54522d0a7365b16bd560bda865773f",
        "render_program_sha256":
            "4bc9881e6f0ac7b9ebf19aa4c295525e4d54522d0a7365b16bd560bda865773f",
        "omitted_fork_program_sha256": (
            "e69459c34c2e09b772f1fb729223a1e67413e4082ba3a9c1e78d3924a8630793",
        ),
        "articulation_program_sha256":
            "ca91273f625d186e74e838e0dfebf55c132f046d6def24db6a3be61d3f678db5",
        "fidelity_debt": ("untrimmed_shared_source_reuse",
                          "omitted_fork_voice_650"),
    },
    {
        "id": 617,
        "name": "nSYAudioVoicePublicGaspS",
        "kind": "crowd",
        "articulation": 104,
        "sound": 38,
        "notes": ((6, 7, 70), (6, 7, 180)),
        "duration_ticks": 250,
        "ucd_volume": 190,
        "articulation_pitch_cents": 550,
        "loop": False,
        "wave_base": 323416,
        "wave_length": 18792,
        "loop_start": 0,
        "loop_end": 0,
        "retain_full_source": True,
        "expected_retained_samples": 33408,
        "root_fork_programs": (),
        "root_program_sha256":
            "8e459d0d33afe2e9ca543572496b90124a3da49ac9e111bc36fb12ebf8f2a31e",
        "render_program_sha256":
            "8e459d0d33afe2e9ca543572496b90124a3da49ac9e111bc36fb12ebf8f2a31e",
        "articulation_program_sha256":
            "bb4edab4e82d0ffa6a07ea66c83810937e06326a07be08d738cfe798b9d92e41",
        "fidelity_debt": ("untrimmed_shared_source_reuse",),
    },
    {
        "id": 618,
        "name": "nSYAudioVoicePublicCheer",
        "kind": "crowd",
        "articulation": 77,
        "sound": 37,
        "notes": ((8, 7, 80), (7, 7, 100), (7, 7, 200)),
        "duration_ticks": 380,
        "ucd_volume": 210,
        "articulation_pitch_cents": 550,
        "loop": False,
        "wave_base": 298216,
        "wave_length": 25200,
        "loop_start": 0,
        "loop_end": 0,
        "retain_full_source": True,
        "expected_retained_samples": 44800,
        "root_fork_programs": (627,),
        "omitted_fork_programs": (627,),
        "root_program_sha256":
            "eee2a02d366581e090c6e234c1cd0626aeeef831e1364639f4a11c9297a23a1d",
        "render_program_sha256":
            "eee2a02d366581e090c6e234c1cd0626aeeef831e1364639f4a11c9297a23a1d",
        "omitted_fork_program_sha256": (
            "a877ebac0b95ed5bfb48a99e1fe4815dad053c2ab8bb17cea87e33cecdef7a18",
        ),
        "articulation_program_sha256":
            "b8711b929c3d4e402ebdd8c9793f234e550d7c495ce5e9d7354abb0a15e65d2b",
        "fidelity_debt": ("ucd_pitch_automation",
                          "untrimmed_shared_source_reuse",
                          "omitted_fork_voice_627"),
    },
    {
        "id": 619,
        "name": "nSYAudioVoicePublicAmazed",
        "kind": "crowd",
        "articulation": 56,
        "sound": 24,
        "notes": ((5, 7, 80), (5, 7, 180)),
        "duration_ticks": 260,
        "ucd_volume": 230,
        "articulation_pitch_cents": 550,
        "loop": False,
        "wave_base": 200920,
        "wave_length": 17568,
        "loop_start": 0,
        "loop_end": 0,
        "retain_full_source": True,
        "expected_retained_samples": 31232,
        "root_fork_programs": (676,),
        "omitted_fork_programs": (676,),
        "root_program_sha256":
            "243ec30c1eb99f73fe48ab83f6fef50f0145e30e0ed9b639b837638da82ff7a2",
        "render_program_sha256":
            "243ec30c1eb99f73fe48ab83f6fef50f0145e30e0ed9b639b837638da82ff7a2",
        "omitted_fork_program_sha256": (
            "3017848166a868da1b818559d84704073db02999cf20bd5d37f77477bfa9d516",
        ),
        "articulation_program_sha256":
            "516d2cd41e614d0cadcf3d2c0810767972ac3bf1c59b436cf96d8abb1805baf0",
        "fidelity_debt": ("untrimmed_shared_source_reuse",
                          "omitted_fork_voice_676"),
    },
    {
        "id": 620,
        "name": "nSYAudioVoicePublicGaspClap",
        "kind": "crowd",
        "articulation": 79,
        "sound": 37,
        "notes": ((10, 7, 25), (9, 7, 25), (8, 7, 25), (8, 7, 100)),
        "duration_ticks": 175,
        "ucd_volume": 190,
        "articulation_pitch_cents": 1199,
        "loop": False,
        "wave_base": 298216,
        "wave_length": 25200,
        "loop_start": 0,
        "loop_end": 0,
        "retain_full_source": True,
        "expected_retained_samples": 44800,
        "root_fork_programs": (684,),
        "omitted_fork_programs": (684,),
        "root_program_sha256":
            "213d86b3fc77049de55891d21d575c826ed348329b69464fe315f105eed7e22c",
        "render_program_sha256":
            "213d86b3fc77049de55891d21d575c826ed348329b69464fe315f105eed7e22c",
        "omitted_fork_program_sha256": (
            "0eeeeb6e17b17ad91e2fb64c40427f7384e1a0ba895e3a9ce31f604414addaeb",
        ),
        "articulation_program_sha256":
            "81f29b206773f85fce23a2160a170220ac48c60d3b7fcb13a146f661f23bba87",
        "fidelity_debt": ("ucd_pitch_automation",
                          "untrimmed_shared_source_reuse",
                          "omitted_fork_voice_684"),
    },
    {
        "id": 622,
        "name": "nSYAudioVoicePublicDamageL",
        "kind": "crowd",
        "articulation": 77,
        "sound": 37,
        "notes": ((7, 7, 80), (7, 7, 100), (7, 7, 200)),
        "duration_ticks": 380,
        "ucd_volume": 255,
        "articulation_pitch_cents": 550,
        "loop": False,
        "wave_base": 298216,
        "wave_length": 25200,
        "loop_start": 0,
        "loop_end": 0,
        "retain_full_source": True,
        "expected_retained_samples": 44800,
        "root_fork_programs": (),
        "root_program_sha256":
            "cba3ab56d54c335b9f6855e719fac3979d611e159f1c788eb29a7a25d65cef77",
        "render_program_sha256":
            "cba3ab56d54c335b9f6855e719fac3979d611e159f1c788eb29a7a25d65cef77",
        "articulation_program_sha256":
            "b8711b929c3d4e402ebdd8c9793f234e550d7c495ce5e9d7354abb0a15e65d2b",
        "fidelity_debt": ("untrimmed_shared_source_reuse",),
    },
    {
        "id": 623,
        "name": "nSYAudioVoicePublicDamageM",
        "kind": "crowd",
        "articulation": 78,
        "sound": 38,
        "notes": ((13, 7, 30), (13, 7, 120)),
        "duration_ticks": 150,
        "ucd_volume": 240,
        "articulation_pitch_cents": -200,
        "loop": False,
        "wave_base": 323416,
        "wave_length": 18792,
        "loop_start": 0,
        "loop_end": 0,
        "retain_full_source": True,
        "expected_retained_samples": 33408,
        "root_fork_programs": (625,),
        "omitted_fork_programs": (625,),
        "root_program_sha256":
            "381531d5d0af5aec2e45e7661841b479ee56e5883a00eb55d58cfca8a018b2d6",
        "render_program_sha256":
            "381531d5d0af5aec2e45e7661841b479ee56e5883a00eb55d58cfca8a018b2d6",
        # 625 is itself a packed cue below, so this fork is "omitted" only in
        # the sense that 623 does not render it into its own sample -- the
        # sound exists and the runtime can play it.
        "omitted_fork_program_sha256": (
            "59f94288d1b698bf4af564d8332c8fb0ca781266e30268db88890dd35f1fce9b",
        ),
        "articulation_program_sha256":
            "13c25afbef0268d2db7052a9090baef23b9b9ca76200a447044e1d5d30bd10b9",
        "fidelity_debt": ("untrimmed_shared_source_reuse",
                          "omitted_fork_voice_625"),
    },
    {
        "id": 625,
        "name": "nSYAudioVoicePublicDamageS",
        "kind": "crowd",
        "articulation": 79,
        "sound": 37,
        "notes": ((9, 7, 25), (10, 7, 25), (9, 7, 25), (8, 7, 100)),
        "duration_ticks": 175,
        "ucd_volume": 160,
        "articulation_pitch_cents": 1199,
        "loop": False,
        "wave_base": 298216,
        "wave_length": 25200,
        "loop_start": 0,
        "loop_end": 0,
        "retain_full_source": True,
        "expected_retained_samples": 44800,
        "root_fork_programs": (),
        "root_program_sha256":
            "59f94288d1b698bf4af564d8332c8fb0ca781266e30268db88890dd35f1fce9b",
        "render_program_sha256":
            "59f94288d1b698bf4af564d8332c8fb0ca781266e30268db88890dd35f1fce9b",
        "articulation_program_sha256":
            "81f29b206773f85fce23a2160a170220ac48c60d3b7fcb13a146f661f23bba87",
        "fidelity_debt": ("ucd_pitch_automation",
                          "untrimmed_shared_source_reuse"),
    },
    # The natural-match miss ring's loudest survivor: a live match asks for 96
    # six times a minute. It was not "a looped cue needing 285's treatment" --
    # its source loop is 10739..13001 of 13040 samples, and the note schedule
    # (55 ticks = 0.316 s) runs OUT before the sample does (13,040 at ~28,509 Hz
    # = 0.457 s), so a one-shot plays the whole audible cue. The actual blocker
    # was that its articulation has no `pitch` op at all, which
    # validate_articulation required; zero cents is what "no pitch op" means.
    {
        "id": 96,
        "name": "nSYAudioFGMGroundGrind2",
        "kind": "motion",
        "articulation": 76,
        "sound": 36,
        "notes": ((11, 7, 20), (11, 7, 35)),
        "duration_ticks": 55,
        "ucd_volume": 135,
        "articulation_pitch_cents": 0,
        "loop": True,
        "wave_base": 290880,
        "wave_length": 7336,
        "loop_start": 10739,
        "loop_end": 13001,
        "retain_full_source": True,
        "expected_retained_samples": 13040,
        "root_fork_programs": (),
        "root_program_sha256":
            "5164f802cb2f0ebe19aa473a6728ac5192879905394584c0d0375c7f01b2dd32",
        "render_program_sha256":
            "5164f802cb2f0ebe19aa473a6728ac5192879905394584c0d0375c7f01b2dd32",
        "articulation_program_sha256":
            "2776aba8ac785095e1304a4aa18f95ef50bf299b19a612649be6b1d4a2aeebaf",
        "fidelity_debt": ("source_loop_not_reproduced",
                          "untrimmed_shared_source_reuse"),
    },
    # 153 AltitudeWarn, the second DS hardware loop, and the reason the WHISPY_*
    # constants became a per-cue spec. It is Whispy's exact shape: the note
    # schedule (300 ticks = 1.725 s) OUTLIVES the sample (22,208 at ~29,344 Hz =
    # 0.757 s), so a one-shot cuts the warning off two thirds of the way
    # through. The source loop is 18880..22044 = 3,164 samples; four samples of
    # the source's own tail take it to 3,168 = 396 whole words after a one-word
    # PNT, which is the same alignment-debt trade 285 makes and for the same
    # reason -- real audio beats synthetic guard nibbles.
    {
        "id": 153,
        "name": "nSYAudioFGMAltitudeWarn",
        "kind": "motion",
        "articulation": 150,
        "sound": 34,
        "notes": ((6, 7, 100), (6, 7, 100), (6, 7, 100)),
        "duration_ticks": 300,
        "ucd_volume": 200,
        "articulation_pitch_cents": 550,
        "loop": True,
        "wave_base": 273040,
        "wave_length": 12492,
        "loop_start": 18880,
        "loop_end": 22044,
        "expected_retained_samples": 22208,
        # NO "hardware_loop" HERE ANY MORE, and the reason is the one this file
        # already states for 621/626 sixty lines up: "a DS hardware repeat
        # reproduces the loop bit-identically and therefore cannot ramp."
        #
        # 153 is that case with PITCH instead of volume. Articulation 150 opens
        # `pitch 550`, then inside mark_loop/jump_loop it steps `pitch 2390` --
        # about an octave and a half of sweep, repeating. That sweep IS the
        # altitude warning; a siren that does not slide is not a siren.
        #
        # The hardware-repeat render also kept only the loop region: source is
        # 22,208 samples with the loop at 18,880..22,044, and the pack shipped
        # 3,168 -- the tail alone, with the 0.64 s attack discarded, while this
        # selector's own expected_retained_samples said 22,208.
        #
        # Net effect on the owner's ear, and this is the cue they identified by
        # name out of all 88: 1.725 s of sweeping warning came out as a 0.108 s
        # monotone blip on repeat, 16x short and unrecognisable. It fires on the
        # right trigger -- being knocked high -- which is why it read as "a new
        # SFX I don't recognise" rather than as a missing one.
        #
        # FULL_PROGRAM_AOT_IDS walks the articulation and bakes the sweep into
        # the samples at FGM_OUTPUT_RATE, which is the same answer 85/189/190/219
        # already take for schedules the entry fields cannot express.
        "root_fork_programs": (),
        "root_program_sha256":
            "1157bb10b51cd2ff8e356bb88f6b03aba6857e6ef067134cee1d6abe5f308a30",
        "render_program_sha256":
            "1157bb10b51cd2ff8e356bb88f6b03aba6857e6ef067134cee1d6abe5f308a30",
        "articulation_program_sha256":
            "318f72b9338b63745e96aedc4562d2629d38bf5dd7599567ca1d5e16fec963d5",
        "fidelity_debt": (),
    },
    # 85 UnkGrind4, the last of the three cues the miss ring caught, and the one
    # BUGS.md said "needs a decision about how the DS should represent a rate
    # above the register's range". It does not: the register is fine.
    #
    # Three notes at pitch codes 20/24/12 under an articulation at +1100 cents,
    # so the first note asks for 32000 * 2^((1100 + 700)/1200) = 90,510 Hz. That
    # is past 0xFFFF, which is the width of `u16 frequency` in the PACK ENTRY
    # (nds_audio_fgm.c:46) -- not a DS limit. The channel timer reaches roughly
    # a megahertz; the field is ours. So `source_rate_above_u16` is a statement
    # about one entry field, and widening it would move a 32-byte layout that
    # three static asserts and a checker pin, for one cue.
    #
    # 189, 190 and 219 carry the same blocker and are already answered: render
    # the whole source program AOT at FGM_OUTPUT_RATE and bake the pitch
    # schedule into the samples. The entry then stores 32,000 like every other
    # AOT cue and the note sequence is inside the PCM. 85 has exactly that shape
    # -- a bounded three-note schedule, no forks -- so it joins
    # FULL_PROGRAM_AOT_IDS and needs no new machinery. 14 ticks is 2,576 samples
    # at 184/tick, i.e. about 1.3 KB of IMA.
    {
        "id": 85,
        "name": "nSYAudioFGMUnkGrind4",
        "kind": "motion",
        "articulation": 172,
        "sound": 5,
        "notes": ((20, 7, 4), (24, 7, 4), (12, 7, 6)),
        "duration_ticks": 14,
        "ucd_volume": 250,
        "articulation_pitch_cents": 1100,
        "loop": True,
        "wave_base": 45608,
        "wave_length": 21880,
        "loop_start": 89,
        "loop_end": 38880,
        # Unused on this path and deliberately not a real figure: the full
        # program render walks the note schedule and never trims a source
        # prefix, so nothing reads it. Same 1 the auto-derived attack selectors
        # carry for the same reason (see the FULL_COVERAGE_IDS loop). A real
        # sample count here would be a fixture nothing checks.
        "expected_retained_samples": 1,
        "root_fork_programs": (),
        "root_program_sha256":
            "dd8ae3a7e2bb5ab9c5ee7dbd470ec9d3405093639dc15da4bfde509004901b81",
        "render_program_sha256":
            "dd8ae3a7e2bb5ab9c5ee7dbd470ec9d3405093639dc15da4bfde509004901b81",
        "articulation_program_sha256":
            "e4b6796c4107d12978ed8cad07addb9864720420c84a12ecda8a794daf7cffd6",
        "fidelity_debt": (),
    },
    # The five a BOTH-CPU stress match asks for that a single-CPU one never
    # reached, caught on the 2026-08-01 crowd-actor soak's miss ring: Escape x3,
    # GuardOn x2, GuardOff, GamePause and Fox's ledge teeter. Every one is core
    # P1 -- dodging, shielding, pausing, and the voice Fox makes on a ledge --
    # and every one is bounded with no fork voices, so all five take the same
    # full-program AOT render 85 does and need no new machinery.
    {
        "id": 11,
        "name": "nSYAudioFGMEscape",
        "kind": "motion",
        "articulation": 54,
        "sound": 5,
        "notes": ((12, 7, 30), (13, 7, 40), (12, 7, 20)),
        "duration_ticks": 90,
        "ucd_volume": 255,
        "articulation_pitch_cents": -100,
        "loop": True,
        "wave_base": 45608,
        "wave_length": 21880,
        "loop_start": 89,
        "loop_end": 38880,
        "expected_retained_samples": 1,
        "root_fork_programs": (),
        "root_program_sha256":
            "b4c2cc3fc96b9ff5a632db8e4113429a389b28d025afba9ceb379c327d434fe5",
        "render_program_sha256":
            "b4c2cc3fc96b9ff5a632db8e4113429a389b28d025afba9ceb379c327d434fe5",
        "articulation_program_sha256":
            "ef251b927a0ce7c98e1e450689e38b024c43e8e261f83d76ba7b46aea4a39479",
        "fidelity_debt": (),
    },
    {
        "id": 13,
        "name": "nSYAudioFGMGuardOn",
        "kind": "motion",
        "articulation": 176,
        "sound": 72,
        "notes": ((18, 7, 3), (6, 7, 2), (25, 7, 3), (11, 7, 3), (23, 7, 5)),
        "duration_ticks": 16,
        "ucd_volume": 180,
        "articulation_pitch_cents": -50,
        "loop": False,
        "wave_base": 694184,
        "wave_length": 9820,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 1,
        "root_fork_programs": (),
        "root_program_sha256":
            "49955ac7eb03e5460f9eeaa46777d71cd64afde745527f88d188b0c605a8ca9b",
        "render_program_sha256":
            "49955ac7eb03e5460f9eeaa46777d71cd64afde745527f88d188b0c605a8ca9b",
        "articulation_program_sha256":
            "a25919a3f06f4109d0788f13a8164eedef404320c5f1d68974b6fa303d19755f",
        "fidelity_debt": (),
    },
    {
        "id": 14,
        "name": "nSYAudioFGMGuardOff",
        "kind": "motion",
        "articulation": 177,
        "sound": 72,
        "notes": ((8, 7, 6), (6, 7, 5)),
        "duration_ticks": 11,
        "ucd_volume": 220,
        "articulation_pitch_cents": -1199,
        "loop": False,
        "wave_base": 694184,
        "wave_length": 9820,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 1,
        "root_fork_programs": (),
        "root_program_sha256":
            "c5b218838808504400292a22724a45d608defbeebd3efb65d06b1bd875db1423",
        "render_program_sha256":
            "c5b218838808504400292a22724a45d608defbeebd3efb65d06b1bd875db1423",
        "articulation_program_sha256":
            "cd4f49149e6cf37bf4f85dd0cdb2fa346893852bb0078cce2f8b957825a490c4",
        "fidelity_debt": (),
    },
    {
        "id": 278,
        "name": "nSYAudioFGMGamePause",
        "kind": "motion",
        "articulation": 17,
        "sound": 10,
        "notes": ((0, 7, 1), (1, 7, 7), (8, 7, 6), (24, 7, 7), (12, 7, 20)),
        "duration_ticks": 41,
        "ucd_volume": 255,
        "articulation_pitch_cents": 550,
        "loop": False,
        "wave_base": 119296,
        "wave_length": 1548,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 1,
        "root_fork_programs": (),
        "root_program_sha256":
            "895a7c6ab79cdd58f0c1239771e510384113e83ff4b8f09dc8028373bc043760",
        "render_program_sha256":
            "895a7c6ab79cdd58f0c1239771e510384113e83ff4b8f09dc8028373bc043760",
        "articulation_program_sha256":
            "f014df4f7f920736f102d2844f1a79c4d86896f0e5d9b6ba7fb786c1765a3f0a",
        "fidelity_debt": (),
    },
    {
        "id": 369,
        "name": "nSYAudioVoiceFoxOttotto",
        "kind": "voice",
        "articulation": 227,
        "sound": 108,
        "notes": ((13, 7, 20), (13, 7, 25)),
        "duration_ticks": 45,
        "ucd_volume": 200,
        "articulation_pitch_cents": -1200,
        "loop": False,
        "wave_base": 922072,
        "wave_length": 2116,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 1,
        "root_fork_programs": (),
        "root_program_sha256":
            "936d76e4c3a6aefa7d96fe841e0022ea92cce2d9f61a4123ba72327b04751a68",
        "render_program_sha256":
            "936d76e4c3a6aefa7d96fe841e0022ea92cce2d9f61a4123ba72327b04751a68",
        "articulation_program_sha256":
            "eff2f55d748352dca4be41a0377216dba9f6ab9a65b68438f09913a514f3a8e3",
        "fidelity_debt": (),
    },
    # And the last two the miss ring named, on the soak that followed those
    # five: Magnify x4 and Fox's win voice x1. Magnify is the zoom pulse -- five
    # 16-pitch blips separated by rests, which is precisely a bounded fork-free
    # schedule and so takes the same AOT render. FoxWin is a single 90-tick note
    # and takes the ordinary announcer path 472/471 use.
    {
        "id": 271,
        "name": "nSYAudioFGMMagnify",
        "kind": "interface",
        "articulation": 17,
        "sound": 10,
        "notes": ((16, 7, 2), (0, 7, 7), (16, 7, 3), (0, 7, 7),
                  (16, 7, 3), (0, 7, 7), (16, 7, 3), (0, 7, 7),
                  (16, 7, 3), (0, 7, 7)),
        "duration_ticks": 49,
        "ucd_volume": 150,
        "articulation_pitch_cents": 550,
        "loop": False,
        "wave_base": 119296,
        "wave_length": 1548,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 1,
        "root_fork_programs": (),
        "root_program_sha256":
            "988f8ab18eda014485ebf60c6a0182cb9739e96412e39b21933c2104c392828b",
        "render_program_sha256":
            "988f8ab18eda014485ebf60c6a0182cb9739e96412e39b21933c2104c392828b",
        "articulation_program_sha256":
            "f014df4f7f920736f102d2844f1a79c4d86896f0e5d9b6ba7fb786c1765a3f0a",
        "fidelity_debt": (),
    },
    {
        "id": 368,
        "name": "nSYAudioVoiceFoxWin",
        "kind": "voice",
        "articulation": 234,
        "sound": 110,
        "pitch_code": 13,
        "duration_ticks": 90,
        "ucd_volume": 240,
        "articulation_pitch_cents": -1200,
        "loop": False,
        "wave_base": 930064,
        "wave_length": 2052,
        "loop_start": 0,
        "loop_end": 0,
        # The whole source wave. The schedule reaches further than the wave
        # does -- 90 ticks against 3,648 samples at half rate -- so the trim is
        # the wave itself and a one-shot plays every audible sample of it.
        "expected_retained_samples": 3648,
    },
    # And three more, from the first run in which every fireball spawned and the
    # match went to SUDDEN DEATH: a light swing, the Sudden Death announcement,
    # and Fox's selection voice. The ring only names what the run reached, so
    # fixing the GObj cap uncovered a livelier match than any previous soak.
    {
        "id": 18,
        "name": "nSYAudioFGMLightSwingLw1",
        "kind": "attack",
        "articulation": 135,
        "sound": 18,
        "notes": ((15, 7, 30), (10, 7, 30)),
        "duration_ticks": 60,
        "ucd_volume": 155,
        "articulation_pitch_cents": -300,
        "loop": False,
        "wave_base": 187792,
        "wave_length": 3672,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 1,
        "root_fork_programs": (),
        "root_program_sha256":
            "91ce235de708540d681e44239bf28e64d315b6cee4a5b3c5690f540b3056cbde",
        "render_program_sha256":
            "91ce235de708540d681e44239bf28e64d315b6cee4a5b3c5690f540b3056cbde",
        "articulation_program_sha256":
            "d6388c2fb81e17372954b210419bc26cf2b47fbe0cee47bfd6c851bc68ba078c",
        "fidelity_debt": (),
    },
    {
        "id": 365,
        "name": "nSYAudioVoiceFoxSelected",
        "kind": "voice",
        "articulation": 229,
        "sound": 110,
        "notes": ((13, 7, 30), (13, 7, 30), (13, 7, 6)),
        "duration_ticks": 66,
        "ucd_volume": 170,
        "articulation_pitch_cents": -1200,
        "loop": False,
        "wave_base": 930064,
        "wave_length": 2052,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 1,
        "root_fork_programs": (),
        "root_program_sha256":
            "cbeda4b6abca1579f6b645cc08f6c0d7795684fdae7a65bfc031a041c84bf954",
        "render_program_sha256":
            "cbeda4b6abca1579f6b645cc08f6c0d7795684fdae7a65bfc031a041c84bf954",
        "articulation_program_sha256":
            "470c9ce22de750baf54a0ff54f913c1ac54b8767fa82719ca2bd3fa70c5a804c",
        "fidelity_debt": (),
    },
    {
        "id": 514,
        "name": "nSYAudioVoiceAnnounceSuddenDeath",
        "articulation": 335,
        "sound": 212,
        "pitch_code": 13,
        "duration_ticks": 150,
        "ucd_volume": 230,
        "articulation_pitch_cents": -1200,
        "loop": False,
        "wave_base": 1785680,
        "wave_length": 9288,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 13801,
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

# P2-1c-1. All four fields (articulation/sound/notes/wave) came out of
# `--derive 158,163,164,165`, not a guess. 158/163/164 share wave_base 119296
# (the same short UI click, pitch/volume-varied per context by articulation
# 18/17/17) and each note schedule's ceiling reach exceeds the 2,752-sample
# source by a wide margin, so all three retain the full untrimmed source --
# the same shape as every other flat single-wave UI/voice cue in this file.
SELECTED += (
    {
        "id": 158,
        "name": "nSYAudioFGMMenuSelect",
        "kind": "menu",
        "articulation": 18,
        "sound": 10,
        "notes": ((4, 7, 4), (11, 7, 3), (18, 7, 5), (20, 7, 12)),
        "duration_ticks": 24,
        "ucd_volume": 130,
        "articulation_pitch_cents": 320,
        "loop": False,
        "wave_base": 119296,
        "wave_length": 1548,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 2752,
        "root_fork_programs": (),
        "root_program_sha256":
            "6c3209b1777b82cf3f5d6a889fa44acccc074061b4ace5b0839abfd970ad9b95",
        "render_program_sha256":
            "6c3209b1777b82cf3f5d6a889fa44acccc074061b4ace5b0839abfd970ad9b95",
        "articulation_program_sha256":
            "ede797dd2729ec039339e9c53f2afa1007769693deb1ecdc76ace3547d178e30",
        "fidelity_debt": (),
    },
    {
        "id": 163,
        "name": "nSYAudioFGMMenuScroll1",
        "kind": "menu",
        "articulation": 17,
        "sound": 10,
        "notes": ((7, 7, 8), (7, 7, 25)),
        "duration_ticks": 33,
        "ucd_volume": 170,
        "articulation_pitch_cents": 550,
        "loop": False,
        "wave_base": 119296,
        "wave_length": 1548,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 2752,
        "root_fork_programs": (),
        "root_program_sha256":
            "191531a69a863630319ef40c5b12e0ccb48cf4d5eb3c88c442201884122acd1b",
        "render_program_sha256":
            "191531a69a863630319ef40c5b12e0ccb48cf4d5eb3c88c442201884122acd1b",
        "articulation_program_sha256":
            "f014df4f7f920736f102d2844f1a79c4d86896f0e5d9b6ba7fb786c1765a3f0a",
        "fidelity_debt": (),
    },
    {
        "id": 164,
        "name": "nSYAudioFGMMenuScroll2",
        "kind": "menu",
        "articulation": 17,
        "sound": 10,
        "notes": ((3, 7, 8), (3, 7, 16)),
        "duration_ticks": 24,
        "ucd_volume": 135,
        "articulation_pitch_cents": 550,
        "loop": False,
        "wave_base": 119296,
        "wave_length": 1548,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 2752,
        "root_fork_programs": (),
        "root_program_sha256":
            "64032b63e43dc958035b0870994f7dab79157ec7e5f4b405bb76ec644f2f7ea9",
        "render_program_sha256":
            "64032b63e43dc958035b0870994f7dab79157ec7e5f4b405bb76ec644f2f7ea9",
        "articulation_program_sha256":
            "f014df4f7f920736f102d2844f1a79c4d86896f0e5d9b6ba7fb786c1765a3f0a",
        "fidelity_debt": (),
    },
    {
        # Two notes bracket a rest, not one held note -- (13,7,7) blip,
        # (0,7,5) rest, (13,7,70) hold -- the same shape as 216 and 28 above,
        # and like them this cannot use the pitch_code/duration_ticks
        # shorthand. The 70-tick final note (402 ms) far outlives the raw
        # 1,664-sample source (~52 ms at its own rate); the source wave
        # itself declares a loop (loop_start=2, loop_end=1652), which is what
        # the N64 replays for the note's full hold. Rendered on the same
        # `render_source_loop` software path as 216 MarioSpecialHiCoin and 28
        # BurnS -- a proven pattern, and simpler and lower-risk than a DS
        # hardware repeat (285/153's path): no hand-derived IMA predictor/
        # index seed, just the source loop region replayed to the note's
        # proven reach before a normal one-shot IMA encode.
        "id": 165,
        "name": "nSYAudioFGMMenuDenied",
        "kind": "menu",
        "articulation": 15,
        "sound": 8,
        "notes": ((13, 7, 7), (0, 7, 5), (13, 7, 70)),
        "duration_ticks": 82,
        "ucd_volume": 180,
        "articulation_pitch_cents": 0,
        "loop": True,
        "wave_base": 112584,
        "wave_length": 936,
        "loop_start": 2,
        "loop_end": 1652,
        "render_source_loop": True,
        "expected_retained_samples": 15089,
        "root_fork_programs": (),
        "root_program_sha256":
            "a2bd6c78a59eff2844cd0ce7d39ad1b14bac6fde08f66ea1da945ee542ede099",
        "render_program_sha256":
            "a2bd6c78a59eff2844cd0ce7d39ad1b14bac6fde08f66ea1da945ee542ede099",
        "articulation_program_sha256":
            "feb1bdd0f1134b0cb6081f033e16d0721619c2cd7aa1b0736d5191de7db95076",
        "fidelity_debt": (),
    },
)

# P2-1d-1. All fields (articulation/sound/notes/wave) came out of
# `--derive 157`, not a guess. A flat two-note schedule (both notes pitch
# code 9, i.e. one fixed rate throughout) whose ceiling reach -- 7,627
# samples on the first note's own schedule, 7,626 on the cue's whole
# duration_ticks against that same rate -- exceeds the 4,128-sample decoded
# source by a wide margin, so like every other flat single-wave UI cue in
# this file (158/163/164 above) the source's own `min(len(pcm), ...)` bound
# collapses to the full untrimmed source: expected_retained_samples equals
# the decoded sample count exactly, not a trimmed schedule length.
SELECTED += (
    {
        "id": 157,
        "name": "nSYAudioFGMTitlePressStart",
        "kind": "menu",
        "articulation": 92,
        "sound": 21,
        "notes": ((9, 7, 10), (9, 7, 28)),
        "duration_ticks": 38,
        "ucd_volume": 220,
        "articulation_pitch_cents": 550,
        "loop": False,
        "wave_base": 196736,
        "wave_length": 2322,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 4128,
        "root_fork_programs": (),
        "root_program_sha256":
            "cc16a109c4a8bfd4a4c7e1ac3ae28e2b1a6cccdc46e710383d6438329547c3fa",
        "render_program_sha256":
            "cc16a109c4a8bfd4a4c7e1ac3ae28e2b1a6cccdc46e710383d6438329547c3fa",
        "articulation_program_sha256":
            "c11c77d213ceb7403e2f5921cdfb7994d11cf42d60bc5d75b4d2a37b0ea6c2a5",
        "fidelity_debt": (),
    },
)

# P2-1e-1. All four fields (articulation/sound/notes/wave) came out of
# `--derive 121,127,167,512`, not a guess.
SELECTED += (
    {
        # 121 nSYAudioFGMMarioDash has no `set_articulation` of its own -- its
        # root UCD program is just `set_unk2D 0 / set_unk1E 255 / fork_voice 118
        # / stop_voice` -- so, like FoxLanding (74, above), it plays entirely
        # through a fork: program 118 is nSYAudioFGMFoxDash's own program (Mario
        # and Fox share one dash sound, matching the source comment on
        # nSYAudioFGMMarioDash, "// Also Luigi" -- a shared-cue family). Every
        # field below (articulation/sound/notes/wave/volume/pitch) is 118's own,
        # read via `--derive 118`, exactly as validate_ucd requires for a
        # render_program entry.
        "id": 121,
        "name": "nSYAudioFGMMarioDash",
        "kind": "menu",
        "render_program": 118,
        "articulation": 3,
        "sound": 1,
        "notes": ((20, 7, 3), (15, 7, 8)),
        "duration_ticks": 11,
        "ucd_volume": 210,
        "articulation_pitch_cents": 700,
        "loop": False,
        "wave_base": 14224,
        "wave_length": 2944,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 4545,
        "root_fork_programs": (118,),
        "root_program_sha256":
            "575eadac8121cd2ef1f31b11dc3f14865943b88bceed0dfcfeaee291f8d02cfe",
        "render_program_sha256":
            "707014fffb8375076a1dfb4b57da6b2e85719397837bab988116fcca6d85a8bd",
        "articulation_program_sha256":
            "300492238b0d3e3b82ac86f63da05c445083fe1aafa2a6d10d7b4bf4f59b7576",
        "fidelity_debt": (),
    },
    {
        # 127 nSYAudioFGMSamusDash: a plain two-note program, no fork. Its
        # 6,880-sample source is retained whole -- the note schedule's own
        # ceiling reach already meets it, the same "collapses to the full
        # untrimmed source" shape as 158/163/164/157 above.
        "id": 127,
        "name": "nSYAudioFGMSamusDash",
        "kind": "menu",
        "articulation": 4,
        "sound": 2,
        "notes": ((1, 7, 3), (24, 7, 14)),
        "duration_ticks": 17,
        "ucd_volume": 180,
        "articulation_pitch_cents": 1111,
        "loop": False,
        "wave_base": 17168,
        "wave_length": 3870,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 6880,
        "root_fork_programs": (),
        "root_program_sha256":
            "ff68f1bdc2917b4536fa3048f20476dfbbdf7f3c7074e9e9d1ac7797c3babbc0",
        "render_program_sha256":
            "ff68f1bdc2917b4536fa3048f20476dfbbdf7f3c7074e9e9d1ac7797c3babbc0",
        "articulation_program_sha256":
            "b7c818ca65e5ec1b70608c8e0ea939dc9b4f98be40cfae13645c4fb74e2a9ae2",
        "fidelity_debt": (),
    },
    {
        # 167 nSYAudioFGMPlayerSlotWhoosh shares its wave_base/wave_length
        # (21040/7516) and loop_start/loop_end (48/13348) with 285 Whispy Wind
        # above -- the CSS's kind-toggle whoosh and the stage's wind gust are
        # the SAME source sample, different articulation (25, not 451) and a
        # much shorter note schedule (40 ticks, not 470). That schedule reach
        # (3,681 samples) falls well short of the 13,360-sample decoded source
        # AND short of loop_start, so this renders on the plain (non-forked,
        # non-hardware-loop) trim path like every ordinary one-shot cue in this
        # file -- confirmed by running the real trim_proof computation before
        # authoring this entry, not assumed from the wave's own loop flag.
        # `loop: True` here is honest bookkeeping about the WAVE's structural
        # loop declaration (validated against the source ctl data), not a
        # request for DS hardware repeat -- no "hardware_loop" key, so the
        # sample is encoded once and stops, the same as any short click.
        "id": 167,
        "name": "nSYAudioFGMPlayerSlotWhoosh",
        "kind": "menu",
        "articulation": 25,
        "sound": 3,
        "notes": ((12, 7, 10), (13, 7, 10), (12, 7, 10), (10, 7, 10)),
        "duration_ticks": 40,
        "ucd_volume": 215,
        "articulation_pitch_cents": -1100,
        "loop": True,
        "wave_base": 21040,
        "wave_length": 7516,
        "loop_start": 48,
        "loop_end": 13348,
        "expected_retained_samples": 3681,
        "root_fork_programs": (),
        "root_program_sha256":
            "c48d8e62b4ee5d95d0487fb86c7fdf85240aa89697ccde2f8403c3610bf19098",
        "render_program_sha256":
            "c48d8e62b4ee5d95d0487fb86c7fdf85240aa89697ccde2f8403c3610bf19098",
        "articulation_program_sha256":
            "6692337582f54b1f309d0b653b771acaf8de940b2fdc6225f46f57624f22a82a",
        "fidelity_debt": (),
    },
    {
        # 512 nSYAudioVoiceAnnounceFreeForAll: a single 260-tick held note, no
        # fork. Its 34,448-sample source trims to 33,829 -- the schedule's
        # reach falls just short of the raw decode's tail, a genuine (if
        # small) trim rather than a full retain.
        "id": 512,
        "name": "nSYAudioVoiceAnnounceFreeForAll",
        "kind": "menu",
        "articulation": 324,
        "sound": 201,
        "notes": ((13, 7, 260),),
        "duration_ticks": 260,
        "ucd_volume": 240,
        "articulation_pitch_cents": -600,
        "loop": False,
        "wave_base": 1677288,
        "wave_length": 19378,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 33829,
        "root_fork_programs": (),
        "root_program_sha256":
            "2ad845c6f0ae6e795b09f52b6507de76b37190490e3e685ea3f7b0ce0bce9efd",
        "render_program_sha256":
            "2ad845c6f0ae6e795b09f52b6507de76b37190490e3e685ea3f7b0ce0bce9efd",
        "articulation_program_sha256":
            "0983cc40b68cf2dd44248302e1cc1b481acdfef0615223a26a39a55af6553f59",
        "fidelity_debt": (),
    },
)

# P2-1f-1. All fields came out of `--derive 159`, not a guess. Unlike 121
# above, 159 nSYAudioFGMStageSelect has a real local note of its own (pitch
# code 6, 180 ticks) -- but its root program ALSO forks two voices at tick 0,
# before that note runs: 163 nSYAudioFGMMenuScroll1 (two notes, 33 ticks) and
# 6 nSYAudioFGMUnkSmallPing1 (five notes, 15 ticks). Declared in
# FULL_PROGRAM_AOT_IDS so render_fgm_composite_aot mixes all three voices
# (root + both forks, every one starting at sample 0) into one baked sample,
# the same mechanism 154/616/618/619/620/623/625 above already prove correct
# for a root note plus a fork -- it does not stop at the first fork.
SELECTED += (
    {
        "id": 159,
        "name": "nSYAudioFGMStageSelect",
        "kind": "menu",
        "articulation": 84,
        "sound": 40,
        "notes": ((6, 7, 180),),
        "duration_ticks": 180,
        "ucd_volume": 160,
        "articulation_pitch_cents": -1000,
        "loop": True,
        "wave_base": 344720,
        "wave_length": 7660,
        "loop_start": 11619,
        "loop_end": 13590,
        "expected_retained_samples": 13616,
        "root_fork_programs": (163, 6),
        # Both forks are themselves independently packed cues (163 since
        # P2-1c-1, 6 not otherwise packed), but that is not why this omission
        # is safe -- FULL_PROGRAM_AOT_IDS membership above means the built
        # pack's own metadata reports zero omission for this id (the fused
        # render actually includes both), the same as every other
        # fork-declaring id in FULL_PROGRAM_AOT_IDS. These two fields are the
        # hash-pinned proof that the forked programs did not silently change
        # underneath the fusion.
        "omitted_fork_programs": (163, 6),
        "omitted_fork_program_sha256": (
            "191531a69a863630319ef40c5b12e0ccb48cf4d5eb3c88c442201884122acd1b",
            "b21992d98d31c8f49152b5aaf624d544da054bb41e5ce005b30b4b5452ec93df",
        ),
        "root_program_sha256":
            "322849140b9950e3ebb61ec49dae97f6061adfafdc050279444f58ace677fe62",
        "render_program_sha256":
            "322849140b9950e3ebb61ec49dae97f6061adfafdc050279444f58ace677fe62",
        "articulation_program_sha256":
            "dac41667edc35bfd590f9d30dda62096c6c84242fdb03775c7e1a63a96b4b249",
        "fidelity_debt": ("omitted_fork_voice_163", "omitted_fork_voice_6"),
    },
    {
        "id": 166,
        "name": "nSYAudioFGMPlayerSlotClose",
        "kind": "menu",
        "articulation": 19,
        "sound": 11,
        "notes": ((3, 7, 2), (8, 7, 2), (15, 7, 10), (13, 7, 8), (13, 7, 8),
                  (13, 7, 10)),
        "duration_ticks": 40,
        "ucd_volume": 130,
        "articulation_pitch_cents": -800,
        "loop": False,
        "wave_base": 120848,
        "wave_length": 9234,
        "loop_start": 0,
        "loop_end": 0,
        "root_program_sha256":
            "e598004d903a1a7c5acabc234217c89a5f9141bfbca036d2343384edcc516068",
        "articulation_program_sha256":
            "9df0c9055565421b5124b395c6f65991176a6bb51243be0aa757d14c3fcdfef6",
        "expected_retained_samples": 0,
    },
    {
        "id": 526,
        "name": "nSYAudioVoiceAnnounceTeamBattle",
        "kind": "announcer",
        "articulation": 325,
        "sound": 202,
        "notes": ((13, 7, 222),),
        "duration_ticks": 222,
        "ucd_volume": 220,
        "articulation_pitch_cents": -1200,
        "loop": False,
        "wave_base": 1696672,
        "wave_length": 11314,
        "loop_start": 0,
        "loop_end": 0,
        "root_program_sha256":
            "4762734b18b6214f15f17c5b651e4e61c6ca721a8ea16573e922ad975b7b394f",
        "articulation_program_sha256":
            "e2205989f924ca5f43cc26512d2abea3af6e66c77674c3a44742a41a589f293b",
        "expected_retained_samples": 20112,
    },
    {
        "id": 498,
        "name": "nSYAudioVoiceAnnounceLuigi",
        "kind": "announcer",
        "articulation": 317,
        "sound": 194,
        "notes": ((13, 7, 220),),
        "duration_ticks": 220,
        "ucd_volume": 230,
        "articulation_pitch_cents": -1200,
        "loop": False,
        "wave_base": 1601296,
        "wave_length": 11088,
        "loop_start": 0,
        "loop_end": 0,
        "root_program_sha256":
            "87ed4ceb641645f10af0936cc459b2417e8e53a59fb798f0337b45fb3f4ee273",
        "articulation_program_sha256":
            "57fd6dcf6df6d48967db76007f23fbc593f186d368802dbaaa71ab4bc62d577f",
        "expected_retained_samples": 19712,
    },
    {
        "id": 421,
        "name": "nSYAudioVoiceLuigiFuraFura",
        "kind": "voice",
        "articulation": 301,
        "sound": 178,
        "notes": ((14, 7, 6), (15, 7, 20), (15, 7, 30), (14, 7, 40)),
        "duration_ticks": 96,
        "ucd_volume": 165,
        "articulation_pitch_cents": -1199,
        "loop": False,
        "wave_base": 1477440,
        "wave_length": 5248,
        "loop_start": 0,
        "loop_end": 0,
        "root_program_sha256":
            "92f7f0ac4370beb29c1cedb5262a3156eb82a3905fdf78050ae26305cc693dae",
        "articulation_program_sha256":
            "2643fb1b244b6e4e4deb64b3c4180213b2703cd2cd7fa52facb47cde3d4ab705",
        "expected_retained_samples": 9328,
    },
)

# P2-3 Donkey Kong voice bank. Every field below is source-derived by
# `--derive 324,...,336,483,603`; the expected sample extents for the thirteen
# fighter voices are the full-program AOT extents (184 DS output samples per
# source program tick), while the announcer and crowd chant are single-note
# source decodes. This is deliberately explicit so an upstream BattleShip audio
# program change fails the hash pins instead of silently changing DK's voice.
SELECTED += (
    {
        "id": 324,
        "name": "nSYAudioVoiceDonkeyFuraSleep",
        "kind": "voice",
        "articulation": 115,
        "sound": 48,
        "notes": ((14, 7, 400), (14, 7, 410), (14, 7, 410)),
        "duration_ticks": 1220,
        "ucd_volume": 220,
        "articulation_pitch_cents": -1200,
        "loop": False,
        "wave_base": 394080,
        "wave_length": 20880,
        "loop_start": 0,
        "loop_end": 0,
        # Three source notes all use the same pitch/sample and each explicitly
        # starts a new voice (set_unk1E has bit 7 set).  Keep one decoded source
        # wave in the DS cache and replay it at ticks 400 and 810 instead of
        # baking 1,220 * 184 samples into ROM.  This is the DS-native equivalent
        # of BattleShip's sequencer and keeps the cue below the 52 KiB slot.
        "runtime_note_replay": True,
        "expected_retained_samples": 37120,
        "root_program_sha256":
            "b498a7a70268fcd1bd1273b0ff905eb1b341c0e0d730bf0cfeffcc460a1dc1ef",
        "articulation_program_sha256":
            "81f1d1099acf6377a38e8caa3b02572dc6b8572bc6e657dee23e3198e21cf88d",
    },
    {
        "id": 325,
        "name": "nSYAudioVoiceDonkeyAppeal",
        "kind": "voice",
        "articulation": 193,
        "sound": 76,
        "notes": ((4, 7, 15), (3, 7, 5), (2, 7, 30), (1, 7, 30),
                  (24, 7, 30), (23, 7, 70)),
        "duration_ticks": 180,
        "ucd_volume": 240,
        "articulation_pitch_cents": 0,
        "loop": False,
        "wave_base": 729896,
        "wave_length": 5824,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 33120,
        "root_program_sha256":
            "ce7c2716d064f99196d6184f483af3b398cf8ee8f740d9b7d2719d7f77f2c829",
        "articulation_program_sha256":
            "f58e7ea2e9d9594fa5050f912f10fdb573b9766e761142d67aded0cd220e3518",
    },
    {
        "id": 326,
        "name": "nSYAudioVoiceDonkeySmash1",
        "kind": "voice",
        "articulation": 194,
        "sound": 77,
        "notes": ((5, 7, 5), (4, 7, 5), (3, 7, 20), (2, 7, 30),
                  (1, 7, 15)),
        "duration_ticks": 75,
        "ucd_volume": 250,
        "articulation_pitch_cents": -400,
        "loop": False,
        "wave_base": 735720,
        "wave_length": 3870,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 13800,
        "root_program_sha256":
            "6fdc5fbce631b8b0e95e8b4c92d9945a2d290dbbcfc5fc801b8350566af35cb4",
        "articulation_program_sha256":
            "979e2d77304292bcdf12962e7ed8e4842a337a5f7b2e175b58bcde6f23f9d5b4",
    },
    {
        "id": 327,
        "name": "nSYAudioVoiceDonkeySmash2",
        "kind": "voice",
        "articulation": 195,
        "sound": 78,
        "notes": ((3, 7, 10), (2, 7, 10), (1, 7, 20), (1, 7, 20),
                  (24, 7, 20)),
        "duration_ticks": 80,
        "ucd_volume": 240,
        "articulation_pitch_cents": -360,
        "loop": False,
        "wave_base": 739592,
        "wave_length": 4302,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 14720,
        "root_program_sha256":
            "10996c15c2ec7eb8220e715d66d8780eb0efb9325e0d00745ce5eacb6829c1fa",
        "articulation_program_sha256":
            "c14b911dbfd3f2b63b64c014e2674d05a65c8a6c0bbb8aa64af4e136e81a429b",
    },
    {
        "id": 328,
        "name": "nSYAudioVoiceDonkeySmash3",
        "kind": "voice",
        "articulation": 195,
        "sound": 78,
        "notes": ((3, 7, 5), (1, 7, 5), (24, 7, 6), (1, 7, 7),
                  (20, 7, 10), (18, 7, 9)),
        "duration_ticks": 42,
        "ucd_volume": 236,
        "articulation_pitch_cents": -360,
        "loop": False,
        "wave_base": 739592,
        "wave_length": 4302,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 7728,
        "root_program_sha256":
            "cc3df5ce11159998e51c62f81f237200bd7d4fc5e064cbf6ccd293f1ec4fb82e",
        "articulation_program_sha256":
            "c14b911dbfd3f2b63b64c014e2674d05a65c8a6c0bbb8aa64af4e136e81a429b",
    },
    {
        "id": 329,
        "name": "nSYAudioVoiceDonkeySpecialN",
        "kind": "voice",
        "articulation": 193,
        "sound": 76,
        "notes": ((15, 7, 5), (17, 7, 75), (17, 7, 15), (16, 7, 30),
                  (15, 7, 15), (14, 7, 15), (13, 7, 30)),
        "duration_ticks": 185,
        "ucd_volume": 250,
        "articulation_pitch_cents": 0,
        "loop": False,
        "wave_base": 729896,
        "wave_length": 5824,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 34040,
        "root_program_sha256":
            "2b75c25bc26ab8f6829a711a900d57cf45943461159ff6501abf2665ad00a234",
        "articulation_program_sha256":
            "f58e7ea2e9d9594fa5050f912f10fdb573b9766e761142d67aded0cd220e3518",
    },
    {
        "id": 330,
        "name": "nSYAudioVoiceDonkeyDeadUp",
        "kind": "voice",
        "articulation": 196,
        "sound": 79,
        "notes": ((6, 7, 10), (22, 7, 30), (2, 7, 60), (1, 7, 90)),
        "duration_ticks": 190,
        "ucd_volume": 222,
        "articulation_pitch_cents": 0,
        "loop": False,
        "wave_base": 743896,
        "wave_length": 10090,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 34960,
        "root_program_sha256":
            "1725716f0132da1ccad8703e90d389fd465f22e7d3d913ce3cb9e9073e459636",
        "articulation_program_sha256":
            "fc739f21cce8b4f4718baa48559da57c19a1f3b2f4ef4a3804599531ae920749",
    },
    {
        "id": 331,
        "name": "nSYAudioVoiceDonkeyFuraFura",
        "kind": "voice",
        "articulation": 197,
        "sound": 80,
        "notes": ((15, 7, 20), (11, 7, 30), (14, 7, 50), (11, 7, 100)),
        "duration_ticks": 200,
        "ucd_volume": 200,
        "articulation_pitch_cents": 50,
        "loop": True,
        "wave_base": 753992,
        "wave_length": 6526,
        "loop_start": 977,
        "loop_end": 11541,
        "expected_retained_samples": 36800,
        "root_program_sha256":
            "168595dc7323ef5ea22a72da43c5674543ea308bc0d74e42a803a557d0f7eff7",
        "articulation_program_sha256":
            "cf529ecdf00d25043aa39b4dffa969129dd64e3e6cd578f54292bac37487b591",
    },
    {
        "id": 332,
        "name": "nSYAudioVoiceDonkeyDamage",
        "kind": "voice",
        "articulation": 198,
        "sound": 76,
        "notes": ((2, 7, 10), (24, 7, 15), (22, 7, 15), (21, 7, 30),
                  (22, 7, 20)),
        "duration_ticks": 90,
        "ucd_volume": 249,
        "articulation_pitch_cents": 0,
        "loop": False,
        "wave_base": 729896,
        "wave_length": 5824,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 16560,
        "root_program_sha256":
            "5ca95ed8b284c6dc348cc9d9404b6c87b3da6834e115b4e84f1f74875bf9f5d8",
        "articulation_program_sha256":
            "5ca6fa2dd66001e076163a11620ef4ff8c24124d9fbbdc180a964d12ba795fa4",
    },
    {
        "id": 333,
        "name": "nSYAudioVoiceDonkeyDead1",
        "kind": "voice",
        "articulation": 199,
        "sound": 81,
        "notes": ((1, 7, 10), (1, 7, 20), (24, 7, 20), (23, 7, 20),
                  (22, 7, 20), (21, 7, 20), (20, 7, 20)),
        "duration_ticks": 130,
        "ucd_volume": 245,
        "articulation_pitch_cents": 0,
        "loop": False,
        "wave_base": 760520,
        "wave_length": 5562,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 23920,
        "root_program_sha256":
            "1d68628baf350be5972b9b72dd6b762ae6513d76f263fe0af69db1fe9d51a7a6",
        "articulation_program_sha256":
            "2140c615c31d140dd24f9874de8b4cc4a7bb79f6193198494258597b4fb20a07",
    },
    {
        "id": 334,
        "name": "nSYAudioVoiceDonkeyHeavyGet",
        "kind": "voice",
        "articulation": 200,
        "sound": 82,
        "notes": ((7, 7, 6), (7, 7, 20), (7, 7, 30), (7, 7, 40)),
        "duration_ticks": 96,
        "ucd_volume": 246,
        "articulation_pitch_cents": -600,
        "loop": False,
        "wave_base": 766088,
        "wave_length": 13096,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 17664,
        "root_program_sha256":
            "75981d057f2844a2889534db23be32a5bb45f28a4913c35fd12305b0f96bd7cf",
        "articulation_program_sha256":
            "0b3bd31de449f30351704d43bb21e8c7dee86de05de277f3efe9fd08268e10b4",
    },
    {
        "id": 335,
        "name": "nSYAudioVoiceDonkeyHeavyUnk",
        "kind": "voice",
        "articulation": 200,
        "sound": 82,
        "notes": ((14, 7, 6), (18, 7, 20), (9, 7, 30), (10, 7, 10),
                  (2, 7, 10)),
        "duration_ticks": 76,
        "ucd_volume": 250,
        "articulation_pitch_cents": -600,
        "loop": False,
        "wave_base": 766088,
        "wave_length": 13096,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 13984,
        "root_program_sha256":
            "5bf84c5bfc2a855a816a0dff8589750c29ab5f87f0484edbbba1335b90d2ecdb",
        "articulation_program_sha256":
            "0b3bd31de449f30351704d43bb21e8c7dee86de05de277f3efe9fd08268e10b4",
    },
    {
        "id": 336,
        "name": "nSYAudioVoiceDonkeyDead2",
        "kind": "voice",
        "articulation": 199,
        "sound": 81,
        "notes": ((22, 7, 8), (2, 7, 10), (5, 7, 12), (1, 7, 14),
                  (23, 7, 16), (22, 7, 9), (21, 7, 9), (20, 7, 7),
                  (19, 7, 7), (18, 7, 10), (17, 7, 12), (16, 7, 12)),
        "duration_ticks": 126,
        "ucd_volume": 190,
        "articulation_pitch_cents": 0,
        "loop": False,
        "wave_base": 760520,
        "wave_length": 5562,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 23184,
        "root_program_sha256":
            "5d8d7bc65b3bdcd39f5aa246d9568bab240e568d1a8fb578e718580e63351e8b",
        "articulation_program_sha256":
            "2140c615c31d140dd24f9874de8b4cc4a7bb79f6193198494258597b4fb20a07",
    },
    {
        "id": 483,
        "name": "nSYAudioVoiceAnnounceDonkey",
        "kind": "announcer",
        "articulation": 310,
        "sound": 187,
        "notes": ((13, 7, 200),),
        "duration_ticks": 200,
        "ucd_volume": 230,
        "articulation_pitch_cents": -1200,
        "loop": False,
        "wave_base": 1533752,
        "wave_length": 11728,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 18401,
        "root_program_sha256":
            "5532aec38f6b3ef52436ffaa9263f9be47039bf1ebdb3b8a9cf89e078f106d28",
        "articulation_program_sha256":
            "aaa497bd93f13ff026792103afc157a25117ebbde86ba0c6099f215ce8cd8a65",
    },
    {
        "id": 603,
        "name": "nSYAudioVoicePublicDonkey",
        "kind": "crowd",
        "articulation": 120,
        "sound": 51,
        "notes": ((13, 7, 320),),
        "duration_ticks": 320,
        "ucd_volume": 255,
        "articulation_pitch_cents": -1190,
        "loop": False,
        "wave_base": 424040,
        "wave_length": 18918,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 29613,
        "root_program_sha256":
            "f8465bca110ef46023a8e3682e8974ab74c2c83854fc9352ab5be36b32a1b0d1",
        "articulation_program_sha256":
            "ad771315bcc763d9730edf9e9004099211c71c2615ee87df330035e9ba638791",
    },
)


# P2-3 Captain Falcon voice + FGM bank.  He landed selectable at P2-3f8 with
# ZERO cues packed -- 0 of 10 FGM and 0 of 23 voices -- so every cue his motion
# scripts, his FTAttributes lanes, his CSS clip and the announcer/crowd tables
# asked for failed closed and he played silent.  Every field below is source
# derived by `--derive 73,105,106,116,117,180,181,182,183,184,186,187,287,288,
# 298,299,337..355,357,358,359,485,604`, not guessed, and each cue's
# expected_retained_samples is the extent this file's own renderer computes --
# authored by letting the validator name it, not by pasting a plausible number.
#
# THE INVENTORY IS THE SOURCE'S, NOT A GUESS AT WHAT HE NEEDS.  It is every
# `nSYAudio{FGM,Voice}Captain*` enumerator in gm/gmsound.h, and the reachability
# is `235_CaptainMainMotion.c` + `236_CaptainMain.c` + `scsubsysdatacaptain.c` +
# ft/ftcommondata.c's shared DownBounce table.
#
# ONE CUE IS OMITTED, AND THE REASON IS MEASURED, NOT A PREFERENCE.  356
# nSYAudioVoiceCaptainFuraSleep is three notes over 710 source ticks; the
# full-program AOT render is 710 * 184 = 130,640 samples = **65,324 IMA bytes**
# against MAX_CUE_IMA_BYTES = 53,248, the largest runtime cache slot.  The
# generator's own guard says so in as many words -- "FGM cue body exceeds the
# largest runtime cache slot (53248 bytes), so it can never be played:
# 356=65324".  DK's FuraSleep (324) answers the same problem with
# `runtime_note_replay`, and Falcon's cannot take it: that path requires every
# note to share one pitch code and his are 13 / 12 / 13, so a replay would have
# to carry a per-note frequency the 4-byte envelope point has no room for.
# Extending the pack format is a row of its own; until then this one voice
# stays silent and is recorded as remaining in docs/p2/fighters/falcon.md.
SELECTED += (
    # 73 CaptainLanding is its own root program, byte-identical to 72 --
    # the program MarioLanding (77) plays through a fork. Same articulation 3,
    # sound 1 and 14224+2944 wave the whole landing/foot/dash family shares.
    {
        "id": 73,
        "name": "nSYAudioFGMCaptainLanding",
        "kind": "movement",
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
        "root_fork_programs": (),
        "root_program_sha256":
            "9b37506dc57cc43b255fa175bfb1e9256fc4c955ae00e4bd600bf4ab123781cf",
        "render_program_sha256":
            "9b37506dc57cc43b255fa175bfb1e9256fc4c955ae00e4bd600bf4ab123781cf",
        "articulation_program_sha256":
            "300492238b0d3e3b82ac86f63da05c445083fe1aafa2a6d10d7b4bf4f59b7576",
        "aot_source_schedule": True,
    },
    # 106 CaptainFoot and 117 CaptainDash are bare fork_voice roots, like 121
    # MarioDash: every field below is the FORK's (105 / 116), read via
    # `--derive 105,116`, exactly as validate_ucd requires for a
    # render_program entry.
    {
        "id": 106,
        "name": "nSYAudioFGMCaptainFoot",
        "kind": "movement",
        "render_program": 105,
        "articulation": 3,
        "sound": 1,
        "notes": ((8, 7, 4),),
        "duration_ticks": 4,
        "ucd_volume": 125,
        "articulation_pitch_cents": 700,
        "loop": False,
        "wave_base": 14224,
        "wave_length": 2944,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 828,
        "root_fork_programs": (105,),
        "root_program_sha256":
            "180ce825a8bb3595a9eeeb5534910063695d34d9784b48d4215cb77e385b8aa6",
        "render_program_sha256":
            "6ad3a20c66b60ffe8e20807e64e24beca263293fe724decd005cc5506dcd0c5c",
        "articulation_program_sha256":
            "300492238b0d3e3b82ac86f63da05c445083fe1aafa2a6d10d7b4bf4f59b7576",
    },
    {
        "id": 117,
        "name": "nSYAudioFGMCaptainDash",
        "kind": "movement",
        "render_program": 116,
        "articulation": 3,
        "sound": 1,
        "notes": ((15, 7, 4),),
        "duration_ticks": 4,
        "ucd_volume": 210,
        "articulation_pitch_cents": 700,
        "loop": False,
        "wave_base": 14224,
        "wave_length": 2944,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 1239,
        "root_fork_programs": (116,),
        "root_program_sha256":
            "c1866eb8edef1feb61e8d47e75b8b50b54046f309f3cf526155c86f7076345e5",
        "render_program_sha256":
            "3f78fcfec264926e5f61cbd71db9b2207ff1fe9814b594c236b788465015460e",
        "articulation_program_sha256":
            "300492238b0d3e3b82ac86f63da05c445083fe1aafa2a6d10d7b4bf4f59b7576",
    },
    # 180/181 AppearCar1/2 are the Falcon Flyer's engine, and they share the
    # 21040+7516 wave (source loop 48..13348) with 285 Whispy Wind and 167
    # PlayerSlotWhoosh. `loop: True` here is honest bookkeeping about the
    # WAVE's structural loop, not a request for a DS hardware repeat: no
    # "hardware_loop" key, so each renders on the plain trim path and the
    # note schedule's own reach decides the extent -- the same call 167 made.
    {
        "id": 180,
        "name": "nSYAudioFGMCaptainAppearCar1",
        "kind": "entry",
        "articulation": 40,
        "sound": 3,
        "notes": ((6, 7, 5), (8, 7, 5), (10, 7, 5), (13, 7, 10),
                  (17, 7, 70), (22, 7, 5), (17, 7, 10), (11, 7, 20),
                  (8, 7, 20), (6, 7, 10), (3, 7, 10)),
        "duration_ticks": 170,
        "ucd_volume": 255,
        "articulation_pitch_cents": -1200,
        "loop": True,
        "wave_base": 21040,
        "wave_length": 7516,
        "loop_start": 48,
        "loop_end": 13348,
        "expected_retained_samples": 13360,
        "root_fork_programs": (),
        "root_program_sha256":
            "3e8c2df9c55f36e5d108b00ab845352c842cca79c54f6dfb56eb9c124a6c4f40",
        "render_program_sha256":
            "3e8c2df9c55f36e5d108b00ab845352c842cca79c54f6dfb56eb9c124a6c4f40",
        "articulation_program_sha256":
            "4def39c266003113eac3f3ee08a985a6a8e300163346206778be85d04f348f48",
    },
    {
        "id": 181,
        "name": "nSYAudioFGMCaptainAppearCar2",
        "kind": "entry",
        "articulation": 102,
        "sound": 3,
        "notes": ((6, 7, 10), (3, 7, 10), (1, 7, 10), (24, 7, 10),
                  (22, 7, 10), (18, 7, 10), (13, 7, 10), (8, 7, 10)),
        "duration_ticks": 80,
        "ucd_volume": 200,
        "articulation_pitch_cents": 0,
        "loop": True,
        "wave_base": 21040,
        "wave_length": 7516,
        "loop_start": 48,
        "loop_end": 13348,
        "expected_retained_samples": 13360,
        "root_fork_programs": (),
        "root_program_sha256":
            "d4c0e74733e284a4683a6bbf11ee446b0889bc388e3ea1af883dbc65856f6b1c",
        "render_program_sha256":
            "d4c0e74733e284a4683a6bbf11ee446b0889bc388e3ea1af883dbc65856f6b1c",
        "articulation_program_sha256":
            "61af1d28697d3014136b390401283372d29363a8410cb8ac1d457236d0244191",
    },
    {
        "id": 182,
        "name": "nSYAudioFGMCaptainSpecialHi",
        "kind": "special",
        "articulation": 41,
        "sound": 18,
        "notes": ((4, 7, 25), (13, 7, 26), (15, 7, 18), (11, 7, 30)),
        "duration_ticks": 99,
        "ucd_volume": 200,
        "articulation_pitch_cents": 200,
        "loop": False,
        "wave_base": 187792,
        "wave_length": 3672,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 6528,
        "root_fork_programs": (),
        "root_program_sha256":
            "6811b3731892570d1c5e597bde867c5d092f11688a9621f641fed0bf51f1cb61",
        "render_program_sha256":
            "6811b3731892570d1c5e597bde867c5d092f11688a9621f641fed0bf51f1cb61",
        "articulation_program_sha256":
            "2ef0820111e81fdf63b8137ae0a91b477efd00f048dc3add60e11642132ab2c6",
    },
    # 183/184 SpecialNStart/Punch -- FALCON PUNCH -- are fork roots too (186 /
    # 187), and 187's own program forks again, so they render through
    # FULL_PROGRAM_AOT_IDS: the composite renderer walks the render program's
    # schedule AND its forks rather than holding one note.
    {
        "id": 183,
        "name": "nSYAudioFGMCaptainSpecialNStart",
        "kind": "special",
        "render_program": 186,
        "articulation": 147,
        "sound": 11,
        "notes": ((2, 7, 20), (3, 7, 20), (5, 7, 100)),
        "duration_ticks": 140,
        "ucd_volume": 190,
        "articulation_pitch_cents": -1199,
        "loop": False,
        "wave_base": 120848,
        "wave_length": 9234,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 1,
        "root_fork_programs": (186,),
        "root_program_sha256":
            "82950305ea95a3994159361277602148a59ebc20d287cbe0b42c6b70917e236d",
        "render_program_sha256":
            "d43bbdf6fbb80605811fda12db256d4eddd04915c6344fa277fe9e2a5daf0823",
        "articulation_program_sha256":
            "f7113ab8647992854edff34f15d561bafd9e41dbe33e2ef86794583102aaf503",
    },
    {
        "id": 184,
        "name": "nSYAudioFGMCaptainSpecialNPunch",
        "kind": "special",
        "render_program": 187,
        "articulation": 146,
        "sound": 11,
        "notes": ((10, 7, 20), (15, 7, 10), (12, 7, 10), (13, 7, 60),
                  (10, 7, 200)),
        "duration_ticks": 300,
        "ucd_volume": 255,
        "articulation_pitch_cents": -1199,
        "loop": False,
        "wave_base": 120848,
        "wave_length": 9234,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 1,
        "root_fork_programs": (187,),
        "root_program_sha256":
            "35e6353ebe290f7cbc8fec68c661fafe7da45832dfed604eacfae64cd7384e64",
        "render_program_sha256":
            "720103f7048fee9ea9cd24c5383c865de2d063ff7d7eafba659c337027a36b6f",
        "articulation_program_sha256":
            "c7e95d11dbb41563050c833c151841c3495e5bae0d53b32f726d57a5c93982b5",
    },
    # 288 CaptainDeadSlam forks 287, the SAME program Mario's 292 and Fox's 289
    # fork -- one shared slam, three fighters. 299 CaptainDownBounce forks 298
    # the way Fox's 300 and Mario's 303 do.
    {
        "id": 288,
        "name": "nSYAudioFGMCaptainDeadSlam",
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
    },
    {
        "id": 299,
        "name": "nSYAudioFGMCaptainDownBounce",
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
    },
    # The twenty-two packed voices. All render through the full source program
    # (multi-note schedules, no local forks), so each extent is
    # duration_ticks * 184 DS output samples, the same law DK's bank follows.
    # 356 FuraSleep is NOT here -- see the note after this block.
    {
        "id": 337,
        "name": "nSYAudioVoiceCaptainAppeal",
        "kind": "voice",
        "articulation": 212,
        "sound": 94,
        "notes": ((13, 7, 150), (13, 7, 13), (13, 7, 50)),
        "duration_ticks": 213,
        "ucd_volume": 170,
        "articulation_pitch_cents": -1200,
        "loop": False,
        "wave_base": 830848,
        "wave_length": 11476,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 1,
        "root_fork_programs": (),
        "root_program_sha256":
            "1660bafe6bcc8b495532d5c2caa1ae3fa733669e6b84373e5018e97f04ae15a0",
        "render_program_sha256":
            "1660bafe6bcc8b495532d5c2caa1ae3fa733669e6b84373e5018e97f04ae15a0",
        "articulation_program_sha256":
            "b9d90805390fb7e21d5c85cc6ec67512d935ae6deea69d5010c1dc96a183d449",
    },
    {
        "id": 338,
        "name": "nSYAudioVoiceCaptainSpecialHi",
        "kind": "voice",
        "articulation": 213,
        "sound": 95,
        "notes": ((13, 7, 50), (13, 7, 50), (13, 7, 20)),
        "duration_ticks": 120,
        "ucd_volume": 215,
        "articulation_pitch_cents": -1200,
        "loop": False,
        "wave_base": 842328,
        "wave_length": 6426,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 1,
        "root_fork_programs": (),
        "root_program_sha256":
            "f131291b16337036b7db7f5c444b8123cba3c2d37e431dc50b223aa56084d8e2",
        "render_program_sha256":
            "f131291b16337036b7db7f5c444b8123cba3c2d37e431dc50b223aa56084d8e2",
        "articulation_program_sha256":
            "490e9fb43ea1389bd91f120ac2684add8c2ffe27680d38f4848007d3aade28d9",
    },
    {
        "id": 339,
        "name": "nSYAudioVoiceCaptainSmash1",
        "kind": "voice",
        "articulation": 204,
        "sound": 86,
        "notes": ((13, 7, 49),),
        "duration_ticks": 49,
        "ucd_volume": 230,
        "articulation_pitch_cents": -1200,
        "loop": False,
        "wave_base": 790920,
        "wave_length": 2484,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 1,
        "root_fork_programs": (),
        "root_program_sha256":
            "30f6590d05891c1faafc3a15723f1b8123f45ead6c05e36c4a35acf3e6fab326",
        "render_program_sha256":
            "30f6590d05891c1faafc3a15723f1b8123f45ead6c05e36c4a35acf3e6fab326",
        "articulation_program_sha256":
            "e063a5f2be5a6d555c9400732e4efda28a0cedb9850259d9273a4d6daf94942f",
    },
    {
        "id": 340,
        "name": "nSYAudioVoiceCaptainSmash2",
        "kind": "voice",
        "articulation": 205,
        "sound": 87,
        "notes": ((13, 7, 30), (13, 7, 30), (13, 7, 25)),
        "duration_ticks": 85,
        "ucd_volume": 235,
        "articulation_pitch_cents": -1200,
        "loop": False,
        "wave_base": 793408,
        "wave_length": 3448,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 1,
        "root_fork_programs": (),
        "root_program_sha256":
            "889d224d5ed9fb900eb7f05757a1c2f6b138b7dfd0cd0c04c25d6a6847023387",
        "render_program_sha256":
            "889d224d5ed9fb900eb7f05757a1c2f6b138b7dfd0cd0c04c25d6a6847023387",
        "articulation_program_sha256":
            "9de8d492f057b4ad0c4554a895a51826c9feeb8b004281b15fb6fbde0e0367c4",
    },
    {
        "id": 341,
        "name": "nSYAudioVoiceCaptainSmash3",
        "kind": "voice",
        "articulation": 206,
        "sound": 88,
        "notes": ((13, 7, 10), (13, 7, 20), (13, 7, 40)),
        "duration_ticks": 70,
        "ucd_volume": 223,
        "articulation_pitch_cents": -1200,
        "loop": False,
        "wave_base": 796856,
        "wave_length": 3654,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 1,
        "root_fork_programs": (),
        "root_program_sha256":
            "5b112cfcb5c2a03ce30db34570eff269468facb3c7e16fa899e9efddc08b0f93",
        "render_program_sha256":
            "5b112cfcb5c2a03ce30db34570eff269468facb3c7e16fa899e9efddc08b0f93",
        "articulation_program_sha256":
            "2177cd5fbc6c97d4166274e8c3a9d6e3f7552606ca2c4c12c04cd788c8f8e676",
    },
    {
        "id": 342,
        "name": "nSYAudioVoiceCaptainSmash4",
        "kind": "voice",
        "articulation": 220,
        "sound": 99,
        "notes": ((13, 7, 167),),
        "duration_ticks": 167,
        "ucd_volume": 240,
        "articulation_pitch_cents": -1200,
        "loop": False,
        "wave_base": 877800,
        "wave_length": 3862,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 1,
        "root_fork_programs": (),
        "root_program_sha256":
            "19291a461df886a5309ec40a454ce4d1719fdf27006fedd0f4683d8a6b3bb508",
        "render_program_sha256":
            "19291a461df886a5309ec40a454ce4d1719fdf27006fedd0f4683d8a6b3bb508",
        "articulation_program_sha256":
            "c474df3925b175b4830e473b1f5c992d9fa6bd2ebcb54c77047e1d19bb300da0",
    },
    {
        "id": 343,
        "name": "nSYAudioVoiceCaptainFinalComeOn",
        "kind": "voice",
        "articulation": 219,
        "sound": 101,
        "notes": ((13, 7, 105),),
        "duration_ticks": 105,
        "ucd_volume": 240,
        "articulation_pitch_cents": -1200,
        "loop": False,
        "wave_base": 897736,
        "wave_length": 5464,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 1,
        "root_fork_programs": (),
        "root_program_sha256":
            "d1aca2a5fbcc4a5654b9ec6d46c35862dc073daea25c57f2086d96d217f2769e",
        "render_program_sha256":
            "d1aca2a5fbcc4a5654b9ec6d46c35862dc073daea25c57f2086d96d217f2769e",
        "articulation_program_sha256":
            "0b68d6835f062b76e9376eb61041b82b36500a41f2d113a6bbef23c127949610",
    },
    {
        "id": 344,
        "name": "nSYAudioVoiceCaptainSmash5",
        "kind": "voice",
        "articulation": 217,
        "sound": 99,
        "notes": ((13, 7, 30), (13, 7, 30), (13, 7, 15)),
        "duration_ticks": 75,
        "ucd_volume": 220,
        "articulation_pitch_cents": -1200,
        "loop": False,
        "wave_base": 877800,
        "wave_length": 3862,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 1,
        "root_fork_programs": (),
        "root_program_sha256":
            "39cf5c3fd564b40b4414f2f8fa1ddd414cf06a659e3f9afffc18dffe095809ab",
        "render_program_sha256":
            "39cf5c3fd564b40b4414f2f8fa1ddd414cf06a659e3f9afffc18dffe095809ab",
        "articulation_program_sha256":
            "6932524dffa812810606477eb006762168a4985726241f04e0ca5da0054ef399",
    },
    {
        "id": 345,
        "name": "nSYAudioVoiceCaptainAttackS4",
        "kind": "voice",
        "articulation": 211,
        "sound": 93,
        "notes": ((13, 7, 50), (13, 7, 40), (13, 7, 30)),
        "duration_ticks": 120,
        "ucd_volume": 255,
        "articulation_pitch_cents": -1200,
        "loop": False,
        "wave_base": 825288,
        "wave_length": 5554,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 1,
        "root_fork_programs": (),
        "root_program_sha256":
            "9bf603c6f66594aed01cab9f6c53393710fcc5b6d6bb5e4f46cc93a7171727a5",
        "render_program_sha256":
            "9bf603c6f66594aed01cab9f6c53393710fcc5b6d6bb5e4f46cc93a7171727a5",
        "articulation_program_sha256":
            "9a0cb60eb9482b6fb25bec842db2355ac94904a2ed1ba342896bdda95425d299",
    },
    {
        "id": 346,
        "name": "nSYAudioVoiceCaptainSpecialLw",
        "kind": "voice",
        "articulation": 210,
        "sound": 92,
        "notes": ((13, 7, 40), (13, 7, 40), (13, 7, 60)),
        "duration_ticks": 140,
        "ucd_volume": 205,
        "articulation_pitch_cents": -1200,
        "loop": False,
        "wave_base": 817824,
        "wave_length": 7462,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 1,
        "root_fork_programs": (),
        "root_program_sha256":
            "961bcf5fcc3f87ee542d865377c5f883855ef9c0a521ced79dc555a855352eba",
        "render_program_sha256":
            "961bcf5fcc3f87ee542d865377c5f883855ef9c0a521ced79dc555a855352eba",
        "articulation_program_sha256":
            "bac85644a7baa68ac0c999151b2b403e991b6786db8daf1c85c9d377b3d3037f",
    },
    {
        "id": 347,
        "name": "nSYAudioVoiceCaptainSpecialNPunch",
        "kind": "voice",
        "articulation": 209,
        "sound": 91,
        "notes": ((13, 7, 50), (13, 7, 50), (13, 7, 60)),
        "duration_ticks": 160,
        "ucd_volume": 230,
        "articulation_pitch_cents": -1200,
        "loop": False,
        "wave_base": 809376,
        "wave_length": 8442,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 1,
        "root_fork_programs": (),
        "root_program_sha256":
            "a6919e1a87c02d9c6db9beed394cf37c03e55dc3659dfe0d2b610584f20c152e",
        "render_program_sha256":
            "a6919e1a87c02d9c6db9beed394cf37c03e55dc3659dfe0d2b610584f20c152e",
        "articulation_program_sha256":
            "adb5d1a7cd9764ac816bb83cd2e1cd372dfaf56e06ae14d31890c2047150ec54",
    },
    {
        "id": 348,
        "name": "nSYAudioVoiceCaptainSpecialNFalcon",
        "kind": "voice",
        "articulation": 208,
        "sound": 90,
        "notes": ((13, 7, 95),),
        "duration_ticks": 95,
        "ucd_volume": 220,
        "articulation_pitch_cents": -1200,
        "loop": False,
        "wave_base": 804296,
        "wave_length": 5076,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 1,
        "root_fork_programs": (),
        "root_program_sha256":
            "8bbc58ee8289e6e0f75dbb7f830a15e1aecf34eeffd73e82e1296b3a57832442",
        "render_program_sha256":
            "8bbc58ee8289e6e0f75dbb7f830a15e1aecf34eeffd73e82e1296b3a57832442",
        "articulation_program_sha256":
            "b65d65c1200143e656d0ff3b27f950da3b795cb6cd07163e9ad454e45d958502",
    },
    {
        "id": 349,
        "name": "nSYAudioVoiceCaptainDeadUp",
        "kind": "voice",
        "articulation": 216,
        "sound": 98,
        "notes": ((13, 7, 50), (13, 7, 50), (13, 7, 50), (13, 7, 70)),
        "duration_ticks": 220,
        "ucd_volume": 190,
        "articulation_pitch_cents": -1200,
        "loop": False,
        "wave_base": 864952,
        "wave_length": 12844,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 1,
        "root_fork_programs": (),
        "root_program_sha256":
            "76f947ab46fde683355bb3cf35b7095bc536b18717634aace607a2449b580604",
        "render_program_sha256":
            "76f947ab46fde683355bb3cf35b7095bc536b18717634aace607a2449b580604",
        "articulation_program_sha256":
            "06fe64d1c39d7bb72478d52d2093795175bd37caa45e03cf23f7810f1931f518",
    },
    {
        "id": 350,
        "name": "nSYAudioVoiceCaptainFuraFura",
        "kind": "voice",
        "articulation": 214,
        "sound": 96,
        "notes": ((13, 7, 50), (13, 7, 50), (13, 7, 80)),
        "duration_ticks": 180,
        "ucd_volume": 200,
        "articulation_pitch_cents": -1200,
        "loop": False,
        "wave_base": 848760,
        "wave_length": 11088,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 1,
        "root_fork_programs": (),
        "root_program_sha256":
            "a8806899ca1c8285c0c87b12646c2274c753b272fb68ba732dbf29e74d03632b",
        "render_program_sha256":
            "a8806899ca1c8285c0c87b12646c2274c753b272fb68ba732dbf29e74d03632b",
        "articulation_program_sha256":
            "5368fb66b5cacd7a9636900fcc67c3551b005d98744cbf8334bbdd8bb78af28f",
    },
    {
        "id": 351,
        "name": "nSYAudioVoiceCaptainDamage",
        "kind": "voice",
        "articulation": 202,
        "sound": 84,
        "notes": ((13, 7, 30), (13, 7, 20), (13, 7, 20)),
        "duration_ticks": 70,
        "ucd_volume": 245,
        "articulation_pitch_cents": -1200,
        "loop": False,
        "wave_base": 781152,
        "wave_length": 3772,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 1,
        "root_fork_programs": (),
        "root_program_sha256":
            "652e4a9b74397aaa11f8ad7010b7ff31dfbcf336e450e5bdef9fbec5df36b63e",
        "render_program_sha256":
            "652e4a9b74397aaa11f8ad7010b7ff31dfbcf336e450e5bdef9fbec5df36b63e",
        "articulation_program_sha256":
            "54fde1eebeb0cd6c98a44d20cb45212d006bc5bcc4b63fb22a52cb81351d71d3",
    },
    {
        "id": 352,
        "name": "nSYAudioVoiceCaptainUnkPing1",
        "kind": "voice",
        "articulation": 188,
        "sound": 10,
        "notes": ((13, 7, 60),),
        "duration_ticks": 60,
        "ucd_volume": 200,
        "articulation_pitch_cents": 0,
        "loop": False,
        "wave_base": 119296,
        "wave_length": 1548,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 1,
        "root_fork_programs": (),
        "root_program_sha256":
            "f776d329bd1202525e4ede61f1129474273ca6d52ab65afaebf8749f9fd71893",
        "render_program_sha256":
            "f776d329bd1202525e4ede61f1129474273ca6d52ab65afaebf8749f9fd71893",
        "articulation_program_sha256":
            "ff485f899743f05506ab29c92513c24018dc9c8242c0a3edb08724f472ab5c68",
    },
    {
        "id": 353,
        "name": "nSYAudioVoiceCaptainJumpAerial",
        "kind": "voice",
        "articulation": 201,
        "sound": 83,
        "notes": ((13, 7, 20), (13, 7, 17)),
        "duration_ticks": 37,
        "ucd_volume": 190,
        "articulation_pitch_cents": -1200,
        "loop": False,
        "wave_base": 779184,
        "wave_length": 1962,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 1,
        "root_fork_programs": (),
        "root_program_sha256":
            "58a50a1393e2744da9b6ea688ff76cd50029b619d4776b4fa95763816d521a71",
        "render_program_sha256":
            "58a50a1393e2744da9b6ea688ff76cd50029b619d4776b4fa95763816d521a71",
        "articulation_program_sha256":
            "9858ddf999a0a858b27d30a6d5d8bcb16f29f6c1063efdf1def272006176eb1a",
    },
    {
        "id": 354,
        "name": "nSYAudioVoiceCaptainHeavyGet",
        "kind": "voice",
        "articulation": 215,
        "sound": 97,
        "notes": ((13, 7, 50), (13, 7, 55)),
        "duration_ticks": 105,
        "ucd_volume": 180,
        "articulation_pitch_cents": -1200,
        "loop": False,
        "wave_base": 859848,
        "wave_length": 5104,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 1,
        "root_fork_programs": (),
        "root_program_sha256":
            "57ef2fefa9e86c6c418ec952d08a504471346110b5481edd5c3572e68eeed921",
        "render_program_sha256":
            "57ef2fefa9e86c6c418ec952d08a504471346110b5481edd5c3572e68eeed921",
        "articulation_program_sha256":
            "831a9b393186a47f67415a90d1bcdb8aa4b5dae22258f7e7dedc1e09bbf3791b",
    },
    {
        "id": 355,
        "name": "nSYAudioVoiceCaptainDead",
        "kind": "voice",
        "articulation": 203,
        "sound": 85,
        "notes": ((13, 7, 50), (13, 7, 40), (13, 7, 20)),
        "duration_ticks": 110,
        "ucd_volume": 230,
        "articulation_pitch_cents": -1200,
        "loop": False,
        "wave_base": 784928,
        "wave_length": 5986,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 1,
        "root_fork_programs": (),
        "root_program_sha256":
            "8aee09ed8ef1edecb1a8ceeef9538669b20f4626d8153bc6783ec5a06c9e7166",
        "render_program_sha256":
            "8aee09ed8ef1edecb1a8ceeef9538669b20f4626d8153bc6783ec5a06c9e7166",
        "articulation_program_sha256":
            "ef911e71ed250511b1f4286267d0dd7f097ed48dd2d4dba6124f1c64d868ad3d",
    },
    {
        "id": 357,
        "name": "nSYAudioVoiceCaptainUnkQuick",
        "kind": "voice",
        "articulation": 207,
        "sound": 89,
        "notes": ((13, 7, 10), (13, 7, 20), (13, 7, 30)),
        "duration_ticks": 60,
        "ucd_volume": 160,
        "articulation_pitch_cents": -1200,
        "loop": False,
        "wave_base": 800512,
        "wave_length": 3780,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 1,
        "root_fork_programs": (),
        "root_program_sha256":
            "507fc61380b1b901c4c4daf1cd030a1ce877ed259775c1c5c53528bc2f922dd8",
        "render_program_sha256":
            "507fc61380b1b901c4c4daf1cd030a1ce877ed259775c1c5c53528bc2f922dd8",
        "articulation_program_sha256":
            "d22909d824d2c6eca192dd372862028a1066a62e269ec48b874b5ee2bb81ea96",
    },
    {
        "id": 358,
        "name": "nSYAudioVoiceCaptainUnkPing2",
        "kind": "voice",
        "articulation": 188,
        "sound": 10,
        "notes": ((13, 7, 60),),
        "duration_ticks": 60,
        "ucd_volume": 255,
        "articulation_pitch_cents": 0,
        "loop": False,
        "wave_base": 119296,
        "wave_length": 1548,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 1,
        "root_fork_programs": (),
        "root_program_sha256":
            "616495a728197ba769242e986f965837c976bcc5694c0a5f7fdd9fe5e72abb67",
        "render_program_sha256":
            "616495a728197ba769242e986f965837c976bcc5694c0a5f7fdd9fe5e72abb67",
        "articulation_program_sha256":
            "ff485f899743f05506ab29c92513c24018dc9c8242c0a3edb08724f472ab5c68",
    },
    {
        "id": 359,
        "name": "nSYAudioVoiceCaptainUnkPing3",
        "kind": "voice",
        "articulation": 188,
        "sound": 10,
        "notes": ((13, 7, 60),),
        "duration_ticks": 60,
        "ucd_volume": 255,
        "articulation_pitch_cents": 0,
        "loop": False,
        "wave_base": 119296,
        "wave_length": 1548,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 1,
        "root_fork_programs": (),
        "root_program_sha256":
            "616495a728197ba769242e986f965837c976bcc5694c0a5f7fdd9fe5e72abb67",
        "render_program_sha256":
            "616495a728197ba769242e986f965837c976bcc5694c0a5f7fdd9fe5e72abb67",
        "articulation_program_sha256":
            "ff485f899743f05506ab29c92513c24018dc9c8242c0a3edb08724f472ab5c68",
    },
    # The announcer line the character select and the Results "winner is"
    # sequence both index by fkind, and the crowd's Falcon chant from
    # ft/ftpublic.c's fighter-call table. Single-note source decodes, like
    # DK's 483/603.
    {
        "id": 485,
        "name": "nSYAudioVoiceAnnounceCaptain",
        "kind": "announcer",
        "articulation": 319,
        "sound": 196,
        "notes": ((13, 7, 300),),
        "duration_ticks": 300,
        "ucd_volume": 240,
        "articulation_pitch_cents": -1200,
        "loop": False,
        "wave_base": 1622680,
        "wave_length": 11556,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 20544,
        "root_fork_programs": (),
        "root_program_sha256":
            "73ad11789f9700012cdc668b6b5208dd8627a69b60abdb5f71a32f91fb07651c",
        "render_program_sha256":
            "73ad11789f9700012cdc668b6b5208dd8627a69b60abdb5f71a32f91fb07651c",
        "articulation_program_sha256":
            "5b545ce86d7c4c9c8ef14ef9ff9490bd11a1316bfe33c9945242b56dd2414692",
    },
    {
        "id": 604,
        "name": "nSYAudioVoicePublicCaptain",
        "kind": "crowd",
        "articulation": 121,
        "sound": 52,
        "notes": ((13, 7, 320),),
        "duration_ticks": 320,
        "ucd_volume": 255,
        "articulation_pitch_cents": -1190,
        "loop": False,
        "wave_base": 442960,
        "wave_length": 17470,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 29613,
        "root_fork_programs": (),
        "root_program_sha256":
            "688d03802bd41516278349c5c99f5bc2528b66e4694d075a548c053f2959b9bd",
        "render_program_sha256":
            "688d03802bd41516278349c5c99f5bc2528b66e4694d075a548c053f2959b9bd",
        "articulation_program_sha256":
            "428eeb95721da6a973bd4b5ebf0cdfebbb9ca8a2c2574045e6d2f518dbe42a96",
    },
)

# No Contest Results audio. Both selectors are direct transcriptions of
# `--derive 502,624`. 624 shares articulation 460 / sound 320 / source loop with
# PublicWin/PublicExcited, including the articulation's volume ramp, so it uses
# the same AOT looped-fanfare renderer rather than a DS hardware repeat.
SELECTED += (
    {
        "id": 502,
        "name": "nSYAudioVoiceAnnounceNoContest",
        "kind": "results",
        "articulation": 339,
        "sound": 216,
        "pitch_code": 13,
        "duration_ticks": 400,
        "ucd_volume": 245,
        "articulation_pitch_cents": -1200,
        "loop": False,
        "wave_base": 1828048,
        "wave_length": 15840,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 28160,
        "root_program_sha256":
            "25f5a5e89afbb74f1125053cf66b4a81fccf4acee6a7b5b9a5f06db6d839f439",
        "articulation_program_sha256":
            "132ea72a8650626b9ad6ce8bc9135b01d550f7e52337eb8f6d36b1d912462c62",
    },
    {
        "id": 624,
        "name": "nSYAudioVoicePublicNoContest",
        "kind": "results",
        "articulation": 460,
        "sound": 320,
        "pitch_code": 10,
        "duration_ticks": 1200,
        "ucd_volume": 180,
        "articulation_pitch_cents": -1200,
        "loop": True,
        "wave_base": 2966600,
        "wave_length": 15876,
        "loop_start": 1,
        "loop_end": 28215,
        "expected_retained_samples": PUBLIC_NO_CONTEST_SAMPLE_COUNT,
        "root_program_sha256":
            "685511dedc41b987c69ae3fed42c37ee321236aa4e2f7fee9c1fc84406b36623",
        "articulation_program_sha256":
            "6539fb2ac7b671fe7f7a0a87282d231e839c417c0cd5c6c9e36a680b8e893a3b",
    },
)


# P2-3 Donkey Kong FGM bank.  He received his VOICE bank at P2-3 (324..336,
# announcer 483, crowd 603) and never his SOUND EFFECTS: every FGM cue his
# motion scripts, his `dead_fgm_ids[1]` and ft/ftcommondata.c's shared
# DownBounce table ask for fell into the miss ring.  Measured, not assumed --
# the P2-3f13 argmax shell run counted 90 DK FGM requests in one minute with
# `DonkeyCharge` alone asked for 56 times, every one of them missing.
#
# THE INVENTORY IS THE SOURCE'S.  Every `nSYAudioFGMDonkey*` enumerator in
# gm/gmsound.h (REGION_US honored) plus the three `nSYAudioFGMBoss*` cues his
# own motion arrays play, with reachability read out of
# `212_DonkeyMainMotion.c` + `213_DonkeyMain.c` + `scsubsysdatadonkey.c` +
# ft/ftcommondata.c's DownBounce table: 9 Slap1, 72 Landing, 105 Foot,
# 116 Dash, 175 BossSlam, 176 BossUnk1, 177 BossUnk2, 178 Spin, 179 Charge,
# 287 DeadSlam (his `dead_fgm_ids[1]`, 0x11f) and 298 DownBounce.  10
# DonkeySlap2 rides with them: it is not DK's -- Captain/Kirby/Purin/Yoshi
# play it and 175/176 fork it -- and it is one of the seven roster-wide shared
# cues P2-3f13 left open, so packing it here closes a Falcon gap too.
#
# THREE OF THESE ARE ALREADY IN THE PACK AS SOMEBODY ELSE'S RENDER PROGRAM,
# which is the strongest possible cross-check on this block: 287 is the slam
# program Mario's 292, Fox's 289 and Captain's 288 all fork, and its
# `root_program_sha256` here is byte-for-byte the `render_program_sha256`
# those three already pin; 298 is the DownBounce program 299/300/303 fork;
# 105 and 116 are the programs Captain's 106 Foot and 117 Dash fork.  Their
# extents (5,168 / 2,301 / 828 / 1,239) came out of the generator's own
# validator and land exactly on the numbers those entries carry.
#
# Every field is `--derive 9,10,72,105,116,175,176,177,178,179,287,298`, and
# every `expected_retained_samples` was authored by letting this file's own
# renderer name it.  Nine cues carry multi-note schedules with no fork or a
# fork of 10, so they render through FULL_PROGRAM_AOT_IDS -- the flat path
# would hold DonkeyCharge's first note for all nine ticks and drop the pitch
# codes 3 and 12 that make it a charge.  Largest new body is 176 BossUnk1 at
# 10,584 B, well inside the 53,248-byte largest runtime cache slot; the pack
# grows 39,640 B and the runtime cache is untouched.
SELECTED += (
    {
        "id": 9,
        "name": "nSYAudioFGMDonkeySlap1",
        "kind": "attack",
        "articulation": 458,
        "sound": 4,
        "notes": ((12, 7, 10), (12, 7, 30), (10, 7, 30)),
        "duration_ticks": 70,
        "ucd_volume": 255,
        "articulation_pitch_cents": -1200,
        "loop": False,
        "wave_base": 28560,
        "wave_length": 17046,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 1,
        "root_fork_programs": (),
        "root_program_sha256":
            "4ce14a1ea9876c41c96b817b4e182b96aec1f1675f26aba3c04d398c70bd9093",
        "render_program_sha256":
            "4ce14a1ea9876c41c96b817b4e182b96aec1f1675f26aba3c04d398c70bd9093",
        "articulation_program_sha256":
            "99589e4ed1453dc1a2be2e29bd56e79f6c65e65c4bab19fda79bb8c88a45c406",
    },
    {
        "id": 10,
        "name": "nSYAudioFGMDonkeySlap2",
        "kind": "attack",
        "articulation": 458,
        "sound": 4,
        "notes": ((8, 7, 20), (5, 7, 20)),
        "duration_ticks": 40,
        "ucd_volume": 255,
        "articulation_pitch_cents": -1200,
        "loop": False,
        "wave_base": 28560,
        "wave_length": 17046,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 1,
        "root_fork_programs": (),
        "root_program_sha256":
            "4188a8eff23ac1e13dec40ec806d3a815e5a5688c4a582c5c93bf6f0d68d117b",
        "render_program_sha256":
            "4188a8eff23ac1e13dec40ec806d3a815e5a5688c4a582c5c93bf6f0d68d117b",
        "articulation_program_sha256":
            "99589e4ed1453dc1a2be2e29bd56e79f6c65e65c4bab19fda79bb8c88a45c406",
    },
    {
        "id": 72,
        "name": "nSYAudioFGMDonkeyLanding",
        "kind": "movement",
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
        "root_fork_programs": (),
        "root_program_sha256":
            "9b37506dc57cc43b255fa175bfb1e9256fc4c955ae00e4bd600bf4ab123781cf",
        "render_program_sha256":
            "9b37506dc57cc43b255fa175bfb1e9256fc4c955ae00e4bd600bf4ab123781cf",
        "articulation_program_sha256":
            "300492238b0d3e3b82ac86f63da05c445083fe1aafa2a6d10d7b4bf4f59b7576",
        "aot_source_schedule": True,
    },
    {
        "id": 105,
        "name": "nSYAudioFGMDonkeyFoot",
        "kind": "movement",
        "articulation": 3,
        "sound": 1,
        "notes": ((8, 7, 4),),
        "duration_ticks": 4,
        "ucd_volume": 125,
        "articulation_pitch_cents": 700,
        "loop": False,
        "wave_base": 14224,
        "wave_length": 2944,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 828,
        "root_fork_programs": (),
        "root_program_sha256":
            "6ad3a20c66b60ffe8e20807e64e24beca263293fe724decd005cc5506dcd0c5c",
        "render_program_sha256":
            "6ad3a20c66b60ffe8e20807e64e24beca263293fe724decd005cc5506dcd0c5c",
        "articulation_program_sha256":
            "300492238b0d3e3b82ac86f63da05c445083fe1aafa2a6d10d7b4bf4f59b7576",
    },
    {
        "id": 116,
        "name": "nSYAudioFGMDonkeyDash",
        "kind": "movement",
        "articulation": 3,
        "sound": 1,
        "notes": ((15, 7, 4),),
        "duration_ticks": 4,
        "ucd_volume": 210,
        "articulation_pitch_cents": 700,
        "loop": False,
        "wave_base": 14224,
        "wave_length": 2944,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 1239,
        "root_fork_programs": (),
        "root_program_sha256":
            "3f78fcfec264926e5f61cbd71db9b2207ff1fe9814b594c236b788465015460e",
        "render_program_sha256":
            "3f78fcfec264926e5f61cbd71db9b2207ff1fe9814b594c236b788465015460e",
        "articulation_program_sha256":
            "300492238b0d3e3b82ac86f63da05c445083fe1aafa2a6d10d7b4bf4f59b7576",
    },
    {
        "id": 175,
        "name": "nSYAudioFGMBossSlam",
        "kind": "attack",
        "articulation": 458,
        "sound": 4,
        "notes": ((12, 7, 10), (12, 7, 30), (10, 7, 30)),
        "duration_ticks": 70,
        "ucd_volume": 235,
        "articulation_pitch_cents": -1200,
        "loop": False,
        "wave_base": 28560,
        "wave_length": 17046,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 1,
        "root_fork_programs": (10,),
        "root_program_sha256":
            "d4d37f393645b0140c7f821b31cfbfeca4536bf6de1fecb5c6f609b014692f1d",
        "render_program_sha256":
            "d4d37f393645b0140c7f821b31cfbfeca4536bf6de1fecb5c6f609b014692f1d",
        "articulation_program_sha256":
            "99589e4ed1453dc1a2be2e29bd56e79f6c65e65c4bab19fda79bb8c88a45c406",
    },
    {
        "id": 176,
        "name": "nSYAudioFGMBossUnk1",
        "kind": "attack",
        "articulation": 108,
        "sound": 26,
        "notes": ((15, 7, 10), (13, 7, 20), (10, 7, 35), (8, 7, 50)),
        "duration_ticks": 115,
        "ucd_volume": 230,
        "articulation_pitch_cents": 0,
        "loop": False,
        "wave_base": 225728,
        "wave_length": 10008,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 1,
        "root_fork_programs": (10,),
        "root_program_sha256":
            "7838e2b3c2e95c8399d332c403456c5b218a5c5ac68cb8bcfbe239ea237fecf3",
        "render_program_sha256":
            "7838e2b3c2e95c8399d332c403456c5b218a5c5ac68cb8bcfbe239ea237fecf3",
        "articulation_program_sha256":
            "509d63083feddf8b69ee3ffbb5cdde510e1a081444dcd0d8b56a7ea2b8621736",
    },
    {
        "id": 177,
        "name": "nSYAudioFGMBossUnk2",
        "kind": "attack",
        "articulation": 7,
        "sound": 4,
        "notes": ((17, 7, 5), (15, 7, 10), (12, 7, 30), (5, 7, 50)),
        "duration_ticks": 95,
        "ucd_volume": 240,
        "articulation_pitch_cents": 300,
        "loop": False,
        "wave_base": 28560,
        "wave_length": 17046,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 1,
        "root_fork_programs": (),
        "root_program_sha256":
            "d62c53393cb191541ce49c80ba21c3c802ceb55b6bf7be4fdf73a44a794c0f35",
        "render_program_sha256":
            "d62c53393cb191541ce49c80ba21c3c802ceb55b6bf7be4fdf73a44a794c0f35",
        "articulation_program_sha256":
            "25baf51195b0172ac10261cc3368f6fac20147a94e42f2ca777aab29ab13a6b3",
    },
    {
        "id": 178,
        "name": "nSYAudioFGMDonkeySpin",
        "kind": "attack",
        "articulation": 174,
        "sound": 71,
        "notes": ((2, 7, 3), (3, 7, 4), (12, 7, 5), (6, 7, 5), (2, 7, 10)),
        "duration_ticks": 27,
        "ucd_volume": 160,
        "articulation_pitch_cents": 550,
        "loop": False,
        "wave_base": 691264,
        "wave_length": 2916,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 1,
        "root_fork_programs": (),
        "root_program_sha256":
            "83bc6862155f8d5a4a506ea431f12e07c2b378be3c19b9d966dc9fe18e14e286",
        "render_program_sha256":
            "83bc6862155f8d5a4a506ea431f12e07c2b378be3c19b9d966dc9fe18e14e286",
        "articulation_program_sha256":
            "7a17a55c4ba3daec625c6334dcb9189080b82bfea648864c1042b1d861f4e69f",
    },
    {
        "id": 179,
        "name": "nSYAudioFGMDonkeyCharge",
        "kind": "attack",
        "articulation": 174,
        "sound": 71,
        "notes": ((13, 7, 2), (3, 7, 2), (12, 7, 5)),
        "duration_ticks": 9,
        "ucd_volume": 145,
        "articulation_pitch_cents": 550,
        "loop": False,
        "wave_base": 691264,
        "wave_length": 2916,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 1,
        "root_fork_programs": (),
        "root_program_sha256":
            "2ae0eb3e067d97456a1c95f1bea047e0dcddfbd559e83a71d5f9ee865bae2cc7",
        "render_program_sha256":
            "2ae0eb3e067d97456a1c95f1bea047e0dcddfbd559e83a71d5f9ee865bae2cc7",
        "articulation_program_sha256":
            "7a17a55c4ba3daec625c6334dcb9189080b82bfea648864c1042b1d861f4e69f",
    },
    {
        "id": 287,
        "name": "nSYAudioFGMDonkeyDeadSlam",
        "kind": "ko",
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
        "root_fork_programs": (),
        "root_program_sha256":
            "634c9b1217b933f51dde97353d62e908fa1082943114d6dbe72bb188a3f33776",
        "render_program_sha256":
            "634c9b1217b933f51dde97353d62e908fa1082943114d6dbe72bb188a3f33776",
        "articulation_program_sha256":
            "bbcff809d0113bec03d327dd08e85ef84fe10c8b18ba2f922b581416a958de0b",
    },
    {
        "id": 298,
        "name": "nSYAudioFGMDonkeyDownBounce",
        "kind": "movement",
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
        "root_fork_programs": (),
        "root_program_sha256":
            "9ed69d587dab562768d6321d349477c4f522c0b65115fb7cb2c1f27d5b27c4c2",
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
    },
)


# P2-3 Luigi voice bank.  He has been selectable since the CSS carried him, and
# the only two cues packed for him were his announcer line (498) and his
# selected-clip FuraFura (421).  Everything his gameplay asks for -- his three
# smash voices, his damage, his star KO, his dead cry, his jumps, his Down-B
# and his crowd chant -- failed closed, which is why P2-3f12's normalizer fix
# was LATENT: reversing his `FTAttributes` lanes changed which unpacked id he
# asked for.  The P2-3f13 argmax run named 422 and 427 in the miss ring.
#
# THE INVENTORY IS THE SOURCE'S.  Every `nSYAudioVoiceLuigi*` enumerator in
# gm/gmsound.h (REGION_US honored) -- the contiguous run 416..428 -- plus
# `nSYAudioVoicePublicLuigi` (608), his row in ft/ftcommondata.c's
# `dFTCommonDataPublicFighterCallFGMs`.  Reachability is `220_LuigiMainMotion.c`
# (Smash1, SpecialLw, Jump, JumpAerial, FuraFura), `221_LuigiMain.c`'s
# `FTAttributes` block at 0x580 (dead 427, deadup 420, damage 422, smash
# 416/417/418, heavyget 426) and `scsubsysdataluigi.c`.  425 Lets and 428
# HereWe are the two the source marks unused; they are packed anyway, because
# the bank is contiguous in the source and a fighter whose bank is complete
# never has to be reopened -- `mnsoundtest.c` already names 428.
#
# 421 FuraFura is NOT repeated here: it landed at P2-3 and keeps its pack
# order.  His KO slam is Mario's own `nSYAudioFGMMarioDeadSlam` (292, already
# packed) and his DownBounce is Mario's 303 -- the source spells both that way,
# so neither is a Luigi cue and neither is missing.
#
# Every field is `--derive 416,417,418,419,420,422,423,424,425,426,427,428,608`
# and every extent came out of this file's own renderer.  The twelve voices are
# multi-note schedules with no forks, so they render through
# FULL_PROGRAM_AOT_IDS at `duration_ticks * 184` -- the law DK's and Falcon's
# banks already follow.  608 is a single 330-tick note and renders flat, like
# every other crowd chant (603/604/605/609).  Largest body is 420 DeadUp at
# 32,664 B, inside the 53,248-byte largest runtime cache slot; nothing here
# needs `runtime_note_replay` and nothing is refused.
SELECTED += (
    {
        "id": 416,
        "name": "nSYAudioVoiceLuigiSmash1",
        "kind": "voice",
        "articulation": 274,
        "sound": 152,
        "notes": ((13, 7, 10), (14, 7, 12), (14, 7, 15)),
        "duration_ticks": 37,
        "ucd_volume": 188,
        "articulation_pitch_cents": -1199,
        "loop": False,
        "wave_base": 1224216,
        "wave_length": 1882,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 1,
        "root_fork_programs": (),
        "root_program_sha256":
            "c864e4f0d7afcdf198c0c99656a49beae4d71c67cc8a5b9fcab0b08772890573",
        "render_program_sha256":
            "c864e4f0d7afcdf198c0c99656a49beae4d71c67cc8a5b9fcab0b08772890573",
        "articulation_program_sha256":
            "5ba60b029e0195117f862d1e00f0d2b20657f8332d0710d1f8ea420fa215a14f",
    },
    {
        "id": 417,
        "name": "nSYAudioVoiceLuigiSmash2",
        "kind": "voice",
        "articulation": 297,
        "sound": 174,
        "notes": ((13, 7, 6), (14, 7, 30), (15, 7, 50), (14, 7, 70),
                  (13, 7, 70)),
        "duration_ticks": 226,
        "ucd_volume": 180,
        "articulation_pitch_cents": -1170,
        "loop": False,
        "wave_base": 1432240,
        "wave_length": 9694,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 1,
        "root_fork_programs": (),
        "root_program_sha256":
            "f241e8a55eee9c80c70821e7bd695c5e078b44407b9542823cafac7c9c624e05",
        "render_program_sha256":
            "f241e8a55eee9c80c70821e7bd695c5e078b44407b9542823cafac7c9c624e05",
        "articulation_program_sha256":
            "ce7ca7fb5d393e272ce037e6929cb3d29e221112ad53093429b8d0d5808221f1",
    },
    {
        "id": 418,
        "name": "nSYAudioVoiceLuigiSmash3",
        "kind": "voice",
        "articulation": 275,
        "sound": 153,
        "notes": ((14, 7, 25), (13, 7, 25), (12, 7, 50), (11, 7, 50)),
        "duration_ticks": 150,
        "ucd_volume": 188,
        "articulation_pitch_cents": -1199,
        "loop": False,
        "wave_base": 1226104,
        "wave_length": 5724,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 1,
        "root_fork_programs": (),
        "root_program_sha256":
            "e9d308f227c598743e8b3cb13ddee8064be44a3c26b7d0a96d00feda89673561",
        "render_program_sha256":
            "e9d308f227c598743e8b3cb13ddee8064be44a3c26b7d0a96d00feda89673561",
        "articulation_program_sha256":
            "50c506394344eefebe421c4cf684e7b3933eccc9eccaf797a02aa6b49dc5e546",
    },
    {
        "id": 419,
        "name": "nSYAudioVoiceLuigiSpecialLw",
        "kind": "voice",
        "articulation": 299,
        "sound": 176,
        "notes": ((13, 7, 6), (14, 7, 20), (14, 7, 30), (15, 7, 40),
                  (14, 7, 30), (13, 7, 30)),
        "duration_ticks": 156,
        "ucd_volume": 180,
        "articulation_pitch_cents": -1199,
        "loop": False,
        "wave_base": 1444144,
        "wave_length": 8524,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 1,
        "root_fork_programs": (),
        "root_program_sha256":
            "02e27c3331b07303183dacf0bf65859037abf271a7a72ef28eaf1efda0f5181a",
        "render_program_sha256":
            "02e27c3331b07303183dacf0bf65859037abf271a7a72ef28eaf1efda0f5181a",
        "articulation_program_sha256":
            "3dadacc3929a34bd0a16a6c73cb8302ef025a6f09d649b1a4b8d2dea1e3eb720",
    },
    {
        "id": 420,
        "name": "nSYAudioVoiceLuigiDeadUp",
        "kind": "voice",
        "articulation": 276,
        "sound": 154,
        "notes": ((14, 7, 60), (15, 7, 150), (15, 7, 145)),
        "duration_ticks": 355,
        "ucd_volume": 180,
        "articulation_pitch_cents": -1199,
        "loop": False,
        "wave_base": 1231832,
        "wave_length": 18136,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 1,
        "root_fork_programs": (),
        "root_program_sha256":
            "2e59a8393402f81de5350f872d63f292b1c6a5cd246c7e9270ca401d3a1b42de",
        "render_program_sha256":
            "2e59a8393402f81de5350f872d63f292b1c6a5cd246c7e9270ca401d3a1b42de",
        "articulation_program_sha256":
            "5c37ff15161e2dabdbedf325c4171dc914e3ae41a6a0a85de2765abc3a26af88",
    },
    {
        "id": 422,
        "name": "nSYAudioVoiceLuigiDamage",
        "kind": "voice",
        "articulation": 277,
        "sound": 155,
        "notes": ((15, 7, 30), (14, 7, 40), (14, 7, 45)),
        "duration_ticks": 115,
        "ucd_volume": 180,
        "articulation_pitch_cents": -1199,
        "loop": False,
        "wave_base": 1249968,
        "wave_length": 8334,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 1,
        "root_fork_programs": (),
        "root_program_sha256":
            "4789844ad3251e15da5b2fa86de5ac82464ff1ccbf8a3c82bdb17d19944b01fc",
        "render_program_sha256":
            "4789844ad3251e15da5b2fa86de5ac82464ff1ccbf8a3c82bdb17d19944b01fc",
        "articulation_program_sha256":
            "1d9f35bace819a21fbd19254671358c9ea1d1117dfe5e75d0942dab4fd9984e4",
    },
    {
        "id": 423,
        "name": "nSYAudioVoiceLuigiJump",
        "kind": "voice",
        "articulation": 278,
        "sound": 156,
        "notes": ((20, 7, 5), (17, 7, 5), (14, 7, 5), (12, 7, 15),
                  (12, 7, 20)),
        "duration_ticks": 50,
        "ucd_volume": 180,
        "articulation_pitch_cents": -1199,
        "loop": False,
        "wave_base": 1258304,
        "wave_length": 2692,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 1,
        "root_fork_programs": (),
        "root_program_sha256":
            "5b465bc5f2357dce2a1f8108575b89fb0764dc70ad7b24e3e4b62962142e3f45",
        "render_program_sha256":
            "5b465bc5f2357dce2a1f8108575b89fb0764dc70ad7b24e3e4b62962142e3f45",
        "articulation_program_sha256":
            "0014673eb07e69334c5e4485a25f7aa5f878eae9f875713f9c34f3c6a7a8ec49",
    },
    {
        "id": 424,
        "name": "nSYAudioVoiceLuigiJumpAerial",
        "kind": "voice",
        "articulation": 279,
        "sound": 157,
        "notes": ((15, 7, 30), (15, 7, 45), (14, 7, 45)),
        "duration_ticks": 120,
        "ucd_volume": 180,
        "articulation_pitch_cents": -1199,
        "loop": False,
        "wave_base": 1261000,
        "wave_length": 6912,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 1,
        "root_fork_programs": (),
        "root_program_sha256":
            "f76e561520abaf670da52160bdfeeef62531f8f32319bcb3afc7e49f7486fa83",
        "render_program_sha256":
            "f76e561520abaf670da52160bdfeeef62531f8f32319bcb3afc7e49f7486fa83",
        "articulation_program_sha256":
            "af4e91e9505d2ebe4091b19ebf66922f006c805cd8327d18612a8f446c7b41f8",
    },
    {
        "id": 425,
        "name": "nSYAudioVoiceLuigiLets",
        "kind": "voice",
        "articulation": 304,
        "sound": 181,
        "notes": ((14, 7, 6), (14, 7, 20), (14, 7, 30), (15, 7, 100),
                  (15, 7, 40)),
        "duration_ticks": 196,
        "ucd_volume": 200,
        "articulation_pitch_cents": -1199,
        "loop": False,
        "wave_base": 1490176,
        "wave_length": 8334,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 1,
        "root_fork_programs": (),
        "root_program_sha256":
            "4bb679e01f6835ceb3aa92cd233bc3f89f612e89547a20499ed99b62de83843a",
        "render_program_sha256":
            "4bb679e01f6835ceb3aa92cd233bc3f89f612e89547a20499ed99b62de83843a",
        "articulation_program_sha256":
            "10a0689e9c3df76051aaa857d41ce266df7bf20ac3d4710484617b1fc00f1ce8",
    },
    {
        "id": 426,
        "name": "nSYAudioVoiceLuigiHeavyGet",
        "kind": "voice",
        "articulation": 305,
        "sound": 182,
        "notes": ((15, 7, 6), (15, 7, 20), (15, 7, 30), (15, 7, 40)),
        "duration_ticks": 96,
        "ucd_volume": 160,
        "articulation_pitch_cents": -1199,
        "loop": False,
        "wave_base": 1498512,
        "wave_length": 4474,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 1,
        "root_fork_programs": (),
        "root_program_sha256":
            "210b7e0c8addd04e6001f0856b3a3fffd14799887871b0c83daed862ecb9851b",
        "render_program_sha256":
            "210b7e0c8addd04e6001f0856b3a3fffd14799887871b0c83daed862ecb9851b",
        "articulation_program_sha256":
            "37aca1add9fc14fd789c7d447c8ba113cfc7542f631bd2ec39357e64f9268919",
    },
    {
        "id": 427,
        "name": "nSYAudioVoiceLuigiDead",
        "kind": "voice",
        "articulation": 280,
        "sound": 158,
        "notes": ((14, 7, 10), (15, 7, 19), (15, 7, 15)),
        "duration_ticks": 44,
        "ucd_volume": 180,
        "articulation_pitch_cents": -1199,
        "loop": False,
        "wave_base": 1267912,
        "wave_length": 2808,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 1,
        "root_fork_programs": (),
        "root_program_sha256":
            "20cd17ea0943a2d223031eea48c250a5ad24b54acddc6bee1f980873e706ee64",
        "render_program_sha256":
            "20cd17ea0943a2d223031eea48c250a5ad24b54acddc6bee1f980873e706ee64",
        "articulation_program_sha256":
            "0dbbfe112c99753517939bb6ac525683b9b955eacb3704506d651ca9f111ccc9",
    },
    {
        "id": 428,
        "name": "nSYAudioVoiceLuigiHereWe",
        "kind": "voice",
        "articulation": 308,
        "sound": 185,
        "notes": ((14, 7, 30), (15, 7, 60), (14, 7, 70), (14, 7, 100)),
        "duration_ticks": 260,
        "ucd_volume": 210,
        "articulation_pitch_cents": -1199,
        "loop": False,
        "wave_base": 1513680,
        "wave_length": 12808,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 1,
        "root_fork_programs": (),
        "root_program_sha256":
            "573d0f857793aa3f259481bcb57fe8615bdd8eda39e8747da668688e962f470f",
        "render_program_sha256":
            "573d0f857793aa3f259481bcb57fe8615bdd8eda39e8747da668688e962f470f",
        "articulation_program_sha256":
            "0e9a36307d5582658fb466b4f3b9ece8b1397f4dea54ea2a93f74c7a53f2c9ea",
    },
    {
        "id": 608,
        "name": "nSYAudioVoicePublicLuigi",
        "kind": "crowd",
        "articulation": 125,
        "sound": 56,
        "notes": ((13, 7, 330),),
        "duration_ticks": 330,
        "ucd_volume": 255,
        "articulation_pitch_cents": -1190,
        "loop": False,
        "wave_base": 513840,
        "wave_length": 19234,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 30538,
        "root_fork_programs": (),
        "root_program_sha256":
            "2927538536aa7ea8801aeead6e933b4899b90b2dffa7ea8db08bdc5bd04c22c8",
        "render_program_sha256":
            "2927538536aa7ea8801aeead6e933b4899b90b2dffa7ea8db08bdc5bd04c22c8",
        "articulation_program_sha256":
            "7a0b66d03fdd5839200c6126ed36b860a7a3fd934f6e7cd2144a1dc3f0e2f742",
    },
)


# P2-3f16 fighter entry audio. The miss-ring census after P2-3f15 exposed the
# first audible inconsistency in four of the five landed fighters: Falcon's
# Flyer already sounded, while Mario, Luigi, Fox and DK entered silently.
# BattleShip names the calls directly in the entry motion programs:
#   dMarioMainMotion_Appear1 / dLuigiMainMotion_Appear1 -> MarioDokan (214)
#   dFoxMainMotion_Appear                              -> FoxAppearArwing (191)
#   dDonkeyMainMotion_Appear1                         -> ContainerSmash (59)
# 59 is intentionally shared with crate/barrel items; it is still DK's entry
# cue because the source plays it beside nEFKindBoxSmash during his arrival.
#
# Every field below comes from `--derive 214,191,59`. All three are multi-note
# schedules with no forks and therefore use FULL_PROGRAM_AOT_IDS. 214 contains
# two pitch-code-0 RESTS; they are real silence in the source pipe rhythm, not
# a playback-rate note. 214 and 191 also report structural WAVE loops, as do
# existing Falcon entry cues; no hardware_loop key is added because the UCD
# note schedule owns the finite extent. The AOT PCM extents are 105/133/160
# ticks at 184 samples per tick; the encoded IMA bodies measure
# 9,664 / 12,240 / 14,724 bytes after their codec headers/alignment.
SELECTED += (
    {
        "id": 214,
        "name": "nSYAudioFGMMarioDokan",
        "kind": "entry",
        "articulation": 116,
        "sound": 49,
        "notes": ((14, 7, 19), (0, 7, 24), (14, 7, 19), (0, 7, 24),
                  (14, 7, 19)),
        "duration_ticks": 105,
        "ucd_volume": 115,
        "articulation_pitch_cents": 0,
        "loop": True,
        "wave_base": 414960,
        "wave_length": 2412,
        "loop_start": 2147,
        "loop_end": 4267,
        "expected_retained_samples": 1,
        "root_fork_programs": (),
        "root_program_sha256":
            "8455f76a40dc63784012e7897b53173916d19bda6f96e6220cf7facd777f44f9",
        "render_program_sha256":
            "8455f76a40dc63784012e7897b53173916d19bda6f96e6220cf7facd777f44f9",
        "articulation_program_sha256":
            "04e1fb94c40ade67e9d41802007ab8039ffe982df0426e55c7d49928cb23e720",
    },
    {
        "id": 191,
        "name": "nSYAudioFGMFoxAppearArwing",
        "kind": "entry",
        "articulation": 47,
        "sound": 3,
        "notes": ((13, 7, 6), (17, 7, 13), (19, 7, 11), (22, 7, 15),
                  (24, 7, 20), (21, 7, 20), (19, 7, 11), (17, 7, 7),
                  (15, 7, 15), (12, 7, 15)),
        "duration_ticks": 133,
        "ucd_volume": 255,
        "articulation_pitch_cents": -1200,
        "loop": True,
        "wave_base": 21040,
        "wave_length": 7516,
        "loop_start": 48,
        "loop_end": 13348,
        "expected_retained_samples": 1,
        "root_fork_programs": (),
        "root_program_sha256":
            "1e056f5a026eb571c29ae22be6dc3912cec81a313380cbbb82dd7cd351c04709",
        "render_program_sha256":
            "1e056f5a026eb571c29ae22be6dc3912cec81a313380cbbb82dd7cd351c04709",
        "articulation_program_sha256":
            "fabc35e7e913b76e43150c47794b97bf25d4a3da17e105167c6e51595e685c9f",
    },
    {
        "id": 59,
        "name": "nSYAudioFGMContainerSmash",
        "kind": "entry",
        "articulation": 449,
        "sound": 16,
        "notes": ((25, 7, 2), (21, 7, 2), (18, 7, 2), (4, 7, 2),
                  (21, 7, 2), (15, 7, 10), (17, 7, 10), (16, 7, 10),
                  (15, 7, 40), (16, 7, 20), (15, 7, 20), (14, 7, 20),
                  (13, 7, 20)),
        "duration_ticks": 160,
        "ucd_volume": 230,
        "articulation_pitch_cents": -1200,
        "loop": False,
        "wave_base": 165960,
        "wave_length": 6310,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 1,
        "root_fork_programs": (),
        "root_program_sha256":
            "4d46f233953fa0b8c2566f9ffe8873c3dfa835814e8d6de569e26f734a85de6d",
        "render_program_sha256":
            "4d46f233953fa0b8c2566f9ffe8873c3dfa835814e8d6de569e26f734a85de6d",
        "articulation_program_sha256":
            "e27aeab2aef5e00910b9446f09af075be1054b9a652201bead8399cb9ba2c9cc",
    },
)

# P2-3 Samus CSS audio. Every field is source-derived by `--derive 513,264`.
# 513 is a single held announcer note. 264 is two consecutive 50-tick notes on
# one source voice; FULL_PROGRAM_AOT_IDS preserves the mid-program control-field
# transition rather than flattening the pair into one 100-tick hold.
SELECTED += (
    {
        "id": 513,
        "name": "nSYAudioVoiceAnnounceSamus",
        "kind": "announcer",
        "articulation": 311,
        "sound": 188,
        "notes": ((13, 7, 150),),
        "duration_ticks": 150,
        "ucd_volume": 230,
        "articulation_pitch_cents": -1200,
        "loop": False,
        "wave_base": 1545480,
        "wave_length": 6912,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 12288,
        "root_program_sha256":
            "18ffab3684f165eb8443f995428542fc3192c64a994b49994f4b4fb42582c665",
        "articulation_program_sha256":
            "56bca0c22f851c499fc0f2c58af9a80a3ea33ab9f6dbba34a4ae649814c66a61",
    },
    {
        "id": 264,
        "name": "nSYAudioFGMBladeDraw",
        "kind": "menu",
        "articulation": 91,
        "sound": 44,
        "notes": ((12, 7, 50), (12, 7, 50)),
        "duration_ticks": 100,
        "ucd_volume": 190,
        "articulation_pitch_cents": 0,
        "loop": False,
        "wave_base": 366640,
        "wave_length": 12708,
        "loop_start": 0,
        "loop_end": 0,
        "expected_retained_samples": 1,
        "root_fork_programs": (),
        "root_program_sha256":
            "e97b2e29073ec2b2309fcb14486c3394930cca74470a722316afadb1b1893b38",
        "render_program_sha256":
            "e97b2e29073ec2b2309fcb14486c3394930cca74470a722316afadb1b1893b38",
        "articulation_program_sha256":
            "4ccf4ee03d7a8846351770b97019b2d3fd4fca37d7fb77e61380ddc0d63215cd",
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

    if expected_render_hash is None and not selector.get("aot_full_program", False):
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
    # An articulation with NO `pitch` op applies no pitch offset, which is zero
    # cents -- not a malformed program. This used to require at least one, and
    # 96 GroundGrind2 (which a live match asks for six times a minute) has none,
    # so it was rejected as written and stayed silent. `--derive` reports its
    # articulation_pitch_cents as null for the same reason; declare 0.
    effective_pitch = pitches[0] if pitches else 0
    if effective_pitch != selector["articulation_pitch_cents"]:
        raise ValueError(f"FGM {selector['id']} pitch changed: {pitches}")
    if expected_hash is None:
        unsupported = {row[0] for row in program} - {
            "trigger", "pitch", "unk36", "vol", "end"
        }
        if unsupported:
            raise ValueError(
                f"FGM {selector['id']} articulation gained unsupported ops: "
                f"{sorted(unsupported)}")
    if not program:
        raise ValueError(f"FGM {selector['id']} articulation has no end")
    if program[-1][0] != "end":
        # An articulation may terminate by looping instead of ending: crowd
        # cues 616 GaspM and 617 GaspS close with `jump_loop` back to their own
        # `mark_loop`, so their automation repeats until the note schedule runs
        # out. That is a terminator, not a truncated program, and the pinned
        # hash above already fixes the exact bytes. A bare `jump_loop` with no
        # `mark_loop` still fails: that would be a program that fell off its
        # end.
        if (program[-1][0] != "jump_loop" or
                not any(row[0] == "mark_loop" for row in program)):
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
    # The source articulation VM starts from a zero-cent pitch offset.  Several
    # DK voice articulations (193/198/199) simply never issue `pitch`; that is
    # the same valid zero-offset case validate_articulation already accepts for
    # FGM 96, not a malformed voice.  Keeping the audit's initial state aligned
    # with the VM lets the full-program AOT renderer preserve DK's note schedule
    # instead of forcing those cues onto the lossy first-note fallback.
    initial_articulation_pitch = articulation_pitches[0] if articulation_pitches else 0
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


# OWNER-DIRECTED LEVEL TRIMS, by FGM id. These deliberately depart from the
# source level, so they live in one visible table rather than as a magic number
# at the assignment: anything here is a judgement call the owner made by ear and
# can revise, not something derived from the sequence data.
#
# 11 nSYAudioFGMEscape -- the rolling dodge. Everything about the cue measures
# source-exact (3-note program, articulation 54, all three modulators, no
# clipping, 33rd of 88 by effective RMS) and the owner still hears it as too
# loud, which is the case this table exists for. 127 -> 96 is about -2.5 dB.
# PROJECT_GOAL puts audio fidelity first in the sacrifice order, so trading a
# little of it for the owner's ear is the cheap and sanctioned direction.
#
# Applies to ds_volume and ds_initial_volume. A cue that also ships a PACKED
# ENVELOPE would have its later points override this, so check
# packed_envelope_count before trusting a trim on one; FGM 11's is 0.
# 127 -> 96 -> 68 -> 48, all on 2026-08-02, each step on the owner's ear.
# 96/127 is -2.4 dB, 68/96 another -3.0, 48/68 another -3.0: -8.4 dB total
# against the source. Halving perceived loudness is about -10 dB, so the cue is
# now most of the way there; the owner's wording moved "too loud" -> "sounds
# better, do one more volume down pass", so this is the pass he asked for and
# the table takes another only if he asks again.
#
# 12 nSYAudioFGMDeadUpStar is a DIFFERENT KIND OF ENTRY and the distinction
# matters: it moves the cue TOWARD the source, not away from it. Its own
# `ucd_volume` is 180 of 255 and its two notes carry velocities 100 and 50, so
# the N64 never plays it at full scale. FULL_PROGRAM_AOT_IDS -- which this cue
# needs, because it is the only thing that recovers its dropped source loop --
# normalises the render to 127 and ships `packed_envelope_count 0`, so the DS
# played it about 3 dB hot with no decay on the second note. The owner heard that
# as *"still sounds too harsh compared to original n64 cue"*.
# 127 * 180/255 = 90, which is the source's own scaling restored rather than a
# judgement. If it is still harsh the next suspect is not the level: SNR is
# 16.086 dB against a 14.0 floor, i.e. audible IMA quantisation, and that wants
# FGM_ENCODE_HEADROOM -- which needs ds_volume room under 127, and this trim is
# what creates it.
FGM_OWNER_VOLUME_TRIM = {
    11: 48,
    12: 90,
}

# Pre-encode scale factor, applied with a matching ds_volume rise so the cue is
# exactly as loud as before. See the use site for why: IMA rails on full-scale
# input and 36 of 88 cues decode at peak 32768. Only cues with ds_volume room
# under 127 can be compensated; the generator refuses rather than clip the
# volume, so a bad entry here fails the render instead of shipping quieter.
FGM_ENCODE_HEADROOM = {
    # Empty since 2026-08-02. FGM 12 was the only entry -- ds_volume 41 -> 82 to
    # clear its full-scale decode -- and it moved to FULL_PROGRAM_AOT_IDS to get
    # its dropped source loop back, where volume is normalised to 127 and there
    # is no room to compensate. The guard below caught that rather than shipping
    # the cue at half loudness: it asked for volume 254 and refused.
    # The mechanism stays for the next cue that needs it; 35 others still decode
    # at full scale and the ones with ds_volume headroom can use this.
}


def fgm_owner_volume_trim(fgm_id: int, volume: int) -> int:
    trimmed = FGM_OWNER_VOLUME_TRIM.get(int(fgm_id))
    if trimmed is None:
        return volume
    return min(int(volume), int(trimmed))


def _fgm_modulator_value(state: dict, sine_table: list[int]) -> float:
    """One LFO tick, transcribed from the engine's own switch.

    `decomp/BattleShip-main/decomp/src/libultra/n_audio/n_env.c:4090-4200`,
    whose fields map to this dict as `_0x8` period, `_0xC` amplitude, `_0x10`
    offset, `_0x14` phase.

    Shapes 4, 5 and 8 are the sample-and-hold / random-lerp family and call
    `randFloat1`/`randFloat2`; they are not reproducible offline and stay
    unsupported. Everything else is deterministic.
    """
    modulator = state["modulator"]
    shape = int(modulator["shape"])
    period = f32(modulator["period"])
    phase = f32(state["phase"] + f32(1.0))
    # The one-shot ramps CLAMP where the periodic shapes WRAP -- n_env.c:4158
    # and :4172 both assign `phase = period` past the end rather than
    # subtracting it, which is the entire difference between shape 6 and
    # shape 2, and between shape 7 and shape 3. Their value expressions are
    # identical to the periodic pair's.
    if shape in (6, 7):
        if period < phase:
            phase = period
    elif period < phase:
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
    if shape in (2, 6):
        # Shipped FGM programs intentionally instantiate a dependent saw with
        # period 0 and let a lower-priority modulator fill that period in later
        # on the same voice.  The N64 float divide produces +/-Inf on the first
        # tick and the target clamp below turns that into an ordinary endpoint;
        # Python raises ZeroDivisionError instead, so model the hardware result
        # explicitly rather than rejecting valid source data (Samus Charge0-6).
        if period == 0.0:
            numerator = f32(amplitude * phase)
            return float("inf") if numerator >= 0.0 else float("-inf")
        return f32(f32(amplitude * phase) / period + offset)
    if shape in (3, 7):
        if period == 0.0:
            numerator = f32(amplitude * f32(period - phase))
            return float("inf") if numerator >= 0.0 else float("-inf")
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
                # n_env.c copies the table row into a per-voice modulator
                # instance.  It must not alias the immutable decoded table:
                # targets 16+ can rewrite period/amplitude/offset/phase of this
                # instance (or another active instance) at runtime.
                modulator = dict(modulators["entries"][int(row[2])])
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
            target = int(modulator["target"])
            value = _fgm_modulator_value(active_modulators[slot], sine_table)
            if target == 10:
                volume = int(min(127.0, max(0.0, value)))
            elif target == 11:
                volume = int(min(127.0, max(0.0, value + volume)))
            elif target == 12:
                pitch = int(min(1200.0, max(-1200.0, value)))
            elif target == 13:
                pitch = int(min(1200.0, max(-1200.0, value + pitch)))
            elif target >= 16:
                # n_env.c:4215-4308. Targets 16..23 rewrite THIS active
                # modulator. Targets 24+ select another active modulator by
                # `(target - 24) / 8`, then map the low three bits back onto
                # fields 16..23.  Despite the old extractor comment calling
                # this "cross-mod another voice", arg0->unk44 is the sorted
                # linked list of active MODULATORS on the same FGM voice. This
                # is observable source behavior: Samus Charge0-6 use target 32
                # to drive modulator #1's period, and Screw Attack uses target
                # 28 to drive modulator #0's offset.
                if target < 24:
                    target_state = active_modulators[slot]
                    field = target
                else:
                    encoded = target - 24
                    target_slot = encoded // 8
                    field = 16 + (encoded % 8)
                    target_state = active_modulators.get(target_slot)
                    if target_state is None:
                        # Source walks arg0->unk44 and simply drops the write
                        # when that id is not active.
                        continue
                target_modulator = target_state["modulator"]
                if field == 16:
                    target_modulator["period"] = f32(value)
                elif field == 17:
                    target_modulator["period"] = f32(
                        target_modulator["period"] + value)
                elif field == 18:
                    target_modulator["amplitude"] = f32(value)
                elif field == 19:
                    target_modulator["amplitude"] = f32(
                        target_modulator["amplitude"] + value)
                elif field == 20:
                    target_modulator["offset"] = f32(value)
                elif field == 21:
                    target_modulator["offset"] = f32(
                        target_modulator["offset"] + value)
                elif field == 22:
                    target_state["phase"] = f32(value)
                elif field == 23:
                    target_state["phase"] = f32(target_state["phase"] + value)
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


def first_sounding_pitch_code(selector: dict) -> int:
    """The pitch the cue is actually VOICED at, skipping a leading rest.

    Pitch code 0 is a rest. The DS pack plays one sample at one rate, so the
    rate has to come from the first entry that makes a sound -- and taking
    `notes[0]` unconditionally does not, whenever a program opens with silence.

    Exactly one P1 cue does: FGM 488 GAME SET, whose program is a 60-tick rest
    at pitch 0 followed by the line at pitch 13, and whose own selector comment
    has said so since it was authored. Every other announcer line is a single
    note 13, so GAME SET shipped 1,300 cents -- thirteen semitones -- below the
    rest of them, which is the owner's "sounds really low pitched, not correct
    sounding" in BUGS.md. Nothing detected it because the pack self-checks
    against its own derivation and the derivation had the same bug.

    Note that the rest also still counts toward duration_ticks, so the line
    starts 60 ticks (one second) earlier on DS than in the source. That is a
    separate, unreported difference and is left alone deliberately: it is a
    timing question, the owner reported pitch, and fixing it means either
    padding the sample with a second of silence or delaying the play call.
    """
    if "notes" not in selector:
        return selector["pitch_code"]
    for pitch_code, _duration_code, _duration_ticks in selector["notes"]:
        if pitch_code != 0:
            return pitch_code
    return selector["notes"][0][0]


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


def hardware_loop_spec(selector: dict) -> dict:
    """Per-cue DS hardware-repeat constants.

    These were WHISPY_WIND_* module constants while 285 was the only hardware
    loop, which is why BUGS.md kept describing a second one as "machinery that
    needs generalising". It is five numbers per cue, and four of the five are
    the same for every cue on this path.
    """
    return selector["hardware_loop"]


def render_hardware_loop(pcm: list[int],
                         selector: dict) -> tuple[list[int], int, int]:
    """Slice the source loop into a word-aligned DS PNT/LEN loop body."""
    spec = hardware_loop_spec(selector)
    loop_start = selector["loop_start"]
    loop_end = selector["loop_end"]
    if not (0 < loop_start < loop_end <= len(pcm)):
        raise ValueError("invalid hardware-loop extent")
    if len(pcm) != selector["expected_retained_samples"]:
        raise ValueError(
            f"FGM {selector['id']} retained-sample proof changed: {len(pcm)}")
    body = list(pcm[loop_start:loop_end + spec["alignment_tail_samples"]])
    # One DS word holds eight nibbles, and LEN counts whole words after PNT.
    if len(body) & 7:
        raise ValueError(
            f"FGM {selector['id']} loop body is not word aligned: {len(body)}")
    # Every repeat re-enters at PNT with the latched state, so the sample the
    # decoder is predicting from is the one immediately before loop_start.
    predictor = int(pcm[loop_start - 1])
    index = initial_ima_index([predictor, body[0]])
    if (predictor != spec["ima_predictor"] or
            index != spec["ima_index"]):
        raise ValueError(
            f"FGM {selector['id']} loop IMA seed changed: "
            f"predictor={predictor} index={index}")
    return body, predictor, index


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


def looped_fanfare_sample_count(selector: dict, frequency: int) -> int:
    """How long the source plays this cue, in DS samples at its own rate.

    The AOT render length was a pinned magic number while 626 was the only cue
    on this path, so nothing said where it came from and a second cue had no
    way to obtain one. It is just the note's duration in source ticks converted
    to samples, ceiling: 1200 ticks x 5750 us x 15102 Hz reproduces 626's
    104,204 exactly. The selector still pins the result -- a derivation that
    silently changes length is as bad as a wrong constant -- but the pin is now
    checked against the source rather than trusted.
    """
    numerator = (selector["duration_ticks"] * FGM_TIMER_MICROSECONDS *
                 frequency)
    return (numerator + 999999) // 1000000


def public_excited_source_indices(selector: dict) -> list[int]:
    loop_start = selector["loop_start"]
    loop_end = selector["loop_end"]
    loop_length = loop_end - loop_start
    if loop_start != 1 or loop_length <= 0:
        raise ValueError("PublicExcited source loop contract changed")
    sample_count = selector["expected_retained_samples"]
    indices = [0]
    while len(indices) < sample_count:
        remaining = sample_count - len(indices)
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
    derived_samples = looped_fanfare_sample_count(selector, frequency)
    if derived_samples != selector["expected_retained_samples"]:
        raise ValueError(
            f"FGM {selector['id']} looped-fanfare length moved: source says "
            f"{derived_samples}, selector pins "
            f"{selector['expected_retained_samples']}")
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
        for index in range(selector["expected_retained_samples"])
    ]
    missing_preroll = []
    for sample_index, source_index in enumerate(missing_preroll_indices):
        gain_numerator, gain_denominator = public_excited_gain_fraction(
            sample_index, frequency, envelope)
        # Clamped exactly like `rendered` above. It was not, and 626 never
        # noticed because its higher UCD volume resolves a higher constant
        # hardware gain and therefore a smaller software multiplier; 621's 190
        # overflows int16 and the control crashed on `struct.pack`. Leaving it
        # unclamped also made this a WEAKER control -- it would have differed
        # from the render in two ways, the missing pre-roll and the saturation,
        # so "they differ" would not have isolated the pre-roll.
        missing_preroll.append(max(-32768, min(32767, round_div_signed(
            int(pcm[source_index]) * gain_numerator * 127,
            gain_denominator * 32767 * hardware_volume))))

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


def build_samus_non_charge_selectors(
        ucd: dict, articulations: dict, modulators: dict,
        ctl_by_offset: dict, instrument: dict, source_tbl: bytes,
        audio_codec, sine_table: list[int]) -> list[dict]:
    """Derive and hash-pin the bounded Samus gameplay audio inventory."""
    selectors = []
    for fgm_id, name in SAMUS_NON_CHARGE_AUDIO:
        root_program = ucd["entries"][fgm_id]["program"]
        render_program_id = SAMUS_NON_CHARGE_RENDER_PROGRAMS.get(fgm_id, fgm_id)
        program = ucd["entries"][render_program_id]["program"]
        articulation_id = first_program_arg(program, "set_articulation")
        art_program = articulations["entries"][articulation_id]["program"]
        sound_id = first_program_arg(art_program, "trigger")
        sound = ctl_by_offset[instrument["soundArray_offs"][sound_id]]
        wave = ctl_by_offset[sound["wavetable_off"]]
        loop = ctl_by_offset[wave["loop_off"]] if wave["loop_off"] else None
        notes = tuple(tuple(int(value) for value in row[1:])
                      for row in program if row[0] == "note")
        volumes = [int(row[1]) for row in program if row[0] == "set_volume"]
        pitches = [int(row[1]) for row in art_program if row[0] == "pitch"]
        root_forks = tuple(int(row[1]) for row in root_program
                           if row[0] == "fork_voice")
        rendered, _metadata = render_fgm_composite_aot(
            render_program_id, ucd, articulations, modulators, instrument,
            ctl_by_offset, source_tbl, audio_codec, sine_table)
        selector = {
            "id": fgm_id,
            "name": name,
            "kind": "samus",
            "articulation": articulation_id,
            "sound": sound_id,
            "notes": notes,
            "duration_ticks": sum(note[2] for note in notes),
            "ucd_volume": volumes[0],
            "articulation_pitch_cents": pitches[0] if pitches else 0,
            "loop": loop is not None,
            "wave_base": wave["base"],
            "wave_length": wave["length"],
            "loop_start": loop["start"] if loop else 0,
            "loop_end": loop["end"] if loop else 0,
            "expected_retained_samples": len(rendered),
            "root_fork_programs": root_forks,
            "root_program_sha256": json_sha256(root_program),
            "render_program_sha256": json_sha256(program),
            "articulation_program_sha256": json_sha256(art_program),
            "fidelity_debt": (),
        }
        if render_program_id != fgm_id:
            selector["render_program"] = render_program_id
        selectors.append(selector)
    digest = json_sha256(selectors)
    if digest != SAMUS_NON_CHARGE_SELECTOR_SHA256:
        raise ValueError(f"Samus non-Charge selector audit changed: {digest}")
    return selectors


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
    for selector in build_samus_non_charge_selectors(
            ucd, articulations, modulators, ctl_by_offset, instrument,
            source_raw["B1_sounds2_tbl"], audio_codec, sine_table):
        fgm_id = int(selector["id"])
        if fgm_id in declared_selectors:
            raise ValueError(f"Samus FGM {fgm_id} is already declared")
        declared_selectors[fgm_id] = selector
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
        first_pitch_code = first_sounding_pitch_code(selector)
        note_pitch_cents = first_pitch_code * 100 - 1300
        net_pitch_cents = (selector["articulation_pitch_cents"] +
                           note_pitch_cents)
        frequency = note_frequency_hz(
            selector["articulation_pitch_cents"], first_pitch_code)
        envelope = articulation_envelope(art_program, selector)
        if selector.get("runtime_note_replay"):
            # P2-3 DK FuraSleep (324) is three long notes which all retrigger the
            # same source wave at the same pitch. Baking the whole 1,220-tick
            # timeline costs 112 KiB of IMA and can never fit the real 52 KiB
            # streaming slot. Preserve the source sequencer shape instead: one
            # cached wave plus timed restart/volume events in the existing 4-byte
            # envelope stream. The fourth byte was reserved/zero in pack v4;
            # bit0 now means "restart this entry's sample before applying volume".
            notes, forks = fgm_program_notes(ucd_program)
            if forks:
                raise ValueError(
                    f"FGM {selector['id']} compact replay gained fork voices")
            if source_loop_infinite:
                raise ValueError(
                    f"FGM {selector['id']} compact replay gained a source loop")
            if (not notes or not all(note["starts_new_voice"] for note in notes) or
                    len({note["pitch_code"] for note in notes}) != 1):
                raise ValueError(
                    f"FGM {selector['id']} compact replay note contract changed")
            pitch_ops = [int(row[1]) for row in art_program if row[0] == "pitch"]
            if pitch_ops != [selector["articulation_pitch_cents"]]:
                raise ValueError(
                    f"FGM {selector['id']} compact replay pitch automation changed")
            art_points = articulation_envelope(art_program, selector)
            if not art_points or art_points[0]["tick"] != 0:
                raise ValueError(
                    f"FGM {selector['id']} compact replay lost initial volume")
            packed_envelope = []
            initial_art_volume = art_points[0]["articulation_volume"]
            volume = ds_volume(notes[0]["root_volume"], initial_art_volume)
            for note_index, note in enumerate(notes):
                note_start = int(note["start_tick"])
                note_end = note_start + int(note["duration_ticks"])
                if note_index != 0:
                    packed_envelope.append({
                        "tick": note_start,
                        "articulation_volume": initial_art_volume,
                        "source_pre_mixer_target": source_volume_target(
                            note["root_volume"], initial_art_volume),
                        "source_quadratic_target": source_quadratic_target(
                            note["root_volume"], initial_art_volume),
                        "ds_volume": ds_volume(
                            note["root_volume"], initial_art_volume),
                        "event_flags": 1,
                    })
                for point in art_points[1:]:
                    point_tick = note_start + int(point["tick"])
                    if point_tick >= note_end:
                        break
                    art_volume = int(point["articulation_volume"])
                    packed_envelope.append({
                        "tick": point_tick,
                        "articulation_volume": art_volume,
                        "source_pre_mixer_target": source_volume_target(
                            note["root_volume"], art_volume),
                        "source_quadratic_target": source_quadratic_target(
                            note["root_volume"], art_volume),
                        "ds_volume": ds_volume(note["root_volume"], art_volume),
                        "event_flags": 0,
                    })
            packed_envelope.sort(key=lambda point: int(point["tick"]))
            if len(pcm) != selector["expected_retained_samples"]:
                raise ValueError(
                    f"FGM {selector['id']} compact source extent changed: {len(pcm)}")
            runtime_pcm = pcm
            loop_strategy = "source_note_retrigger_metadata"
            flags = 0
            loop_point_words = 0
            acoustic_oracle = {
                "aot_strategy": "source_sample_plus_timed_note_retriggers",
                "runtime_retrigger_count": len(notes) - 1,
                "runtime_note_starts": [int(note["start_tick"]) for note in notes],
                "runtime_note_root_volumes": [int(note["root_volume"]) for note in notes],
                "runtime_note_pitch_code": int(notes[0]["pitch_code"]),
                "runtime_event_count": len(packed_envelope),
            }
            trim = {
                "trim_strategy": "full_source_wave_runtime_note_replay",
                "trim_source_samples_removed": 0,
                "trim_applied": False,
                "trim_retained_source_prefix_pcm_sha256": ima_pcm_sha256(pcm),
                "trim_retained_prefix_exact": True,
                "trim_proven_reachable_samples": len(runtime_pcm),
                "trim_one_sample_ceiling": 1,
            }
            old_loop_ima = b""
        elif selector.get("aot_full_program"):
            root_duration_ticks = selector["duration_ticks"]
            # P2-1e-1: render_program_id, not selector["id"] -- 121 MarioDash's
            # own UCD program is a pure fork_voice with zero local notes (no
            # set_articulation of its own), so render_fgm_composite_aot must
            # walk render_program_id's (118 FoxDash's) schedule instead. This is
            # a no-op for every prior aot_full_program id: none of them declare
            # "render_program", so render_program_id already defaulted to
            # selector["id"] for all of them (see the identical default two
            # lines above validate_ucd).
            runtime_pcm, acoustic_oracle = render_fgm_composite_aot(
                render_program_id, ucd, articulations, modulators, instrument,
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
        elif selector["id"] in LOOPED_FANFARE_AOT_IDS:
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
        elif "hardware_loop" in selector:
            runtime_pcm, loop_predictor, loop_index = render_hardware_loop(
                pcm, selector)
            acoustic_oracle = {}
            loop_strategy = "source_loop_ds_hardware"
            flags = 1
            loop_point_words = selector["hardware_loop"]["loop_point_words"]
            packed_envelope = envelope[1:]
            volume = envelope[0]["ds_volume"]
            trim = {
                "trim_strategy": "source_loop_body_ds_hardware_repeat",
                "trim_source_samples_removed": len(pcm) - len(runtime_pcm),
                "trim_applied": True,
                "trim_retained_source_prefix_pcm_sha256": None,
                "trim_retained_prefix_exact": False,
                "trim_proven_reachable_samples": len(runtime_pcm),
                "trim_one_sample_ceiling": 1,
            }
            old_loop_ima = b""
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
        if "hardware_loop" in selector:
            # A looped entry cannot spend its first sample on the IMA header:
            # the header is outside PNT, so a sample parked there would play
            # once and never repeat.  Every sample is a nibble instead, seeded
            # from the state the DS latches at PNT.
            guard_nibbles = tuple(selector["hardware_loop"]["guard_nibbles"])
            ima = ima_encode_loop_body(runtime_pcm, loop_predictor, loop_index,
                                       guard_nibbles)
            decoded_ima = ima_decode_nibbles(
                ima, len(runtime_pcm), loop_point_words * 4)
            acoustic_oracle.update(ima_repeat_oracle(
                ima, loop_point_words, len(runtime_pcm), guard_nibbles))
        else:
            # ENCODE HEADROOM. IMA-ADPCM predicts forward, so a source that
            # already sits at full scale makes the encoder overshoot and rail,
            # and 36 of the 88 cues decode with a peak of 32768 -- one past
            # int16. FGM 12 DeadUpStar is the worst of them at 17.4 dB SNR, and
            # it is the cue the owner named twice: "the high pitch sound is
            # getting clipped or something".
            #
            # Scaling the PCM down before the encode and raising ds_volume by
            # the same factor is loudness-neutral -- the DS multiplies by volume
            # on playback -- so this buys headroom for free wherever ds_volume
            # has room under 127. FGM 12 plays at 41, so halving costs nothing.
            # Cues already at 127 cannot be compensated and are left alone;
            # their clipping is one or two samples at 29-39 dB and inaudible.
            headroom = FGM_ENCODE_HEADROOM.get(int(selector["id"]))
            if headroom is not None:
                compensated = int(round(volume / headroom))
                if compensated > 127:
                    raise ValueError(
                        f"FGM {selector['id']} headroom {headroom} needs volume "
                        f"{compensated}, over the 127 cap")
                volume = max(1, compensated)
                runtime_pcm = [int(round(s * headroom)) for s in runtime_pcm]
            ima = ima_encode(runtime_pcm)
            decoded_ima = ima_decode(ima, len(runtime_pcm))
        metrics = audio_metrics(runtime_pcm, decoded_ima)
        if metrics["decoded_peak"] == 0 or metrics["decoded_rms"] <= 0:
            raise ValueError(f"FGM {selector['id']} decoded to silence")
        if selector["id"] in LOOPED_FANFARE_AOT_IDS:
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
        # Trim HERE, not only in the metadata below. `records` is what
        # PACK_ENTRY.pack writes into the .bin the ROM reads; the metadata dict
        # is documentation. Trimming only the metadata left the manifest saying
        # 96 while the pack still shipped 127 -- and the pack sha256 was
        # unchanged, which is the tell that caught it.
        volume = fgm_owner_volume_trim(selector["id"], volume)
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
            "ds_ima_loop_body_nibbles": (
                len(runtime_pcm) if "hardware_loop" in selector else 0),
            "ds_ima_guard_nibbles": (
                list(selector["hardware_loop"]["guard_nibbles"])
                if "hardware_loop" in selector else []),
            "ds_initial_prefix_samples_dropped": (
                selector["loop_start"]
                if "hardware_loop" in selector else 0),
            "ds_trailing_samples_dropped": (
                len(pcm) - selector["loop_start"] - len(runtime_pcm)
                if "hardware_loop" in selector else 0),
            # ACTIONABLE, cosmetic: seven non-looping voice cues (469, 471, 472,
            # 490, 514, 527, 621) land an ODD count here, one past what their
            # IMA nibbles hold -- two samples per byte can only ever yield an
            # even number, so the odd sample is not encoded. Harmless today
            # because nds_audio_fgm.c parses `sample_count`, validates it
            # non-zero, and then plays from `data_bytes` instead, so nothing
            # reads past the payload. Fix by rounding `runtime_pcm` to an even
            # length before encode, NOT by editing the manifest; doing so
            # reflows the pack sha256 pinned in check-audio-fgm-phase-pack.ps1.
            # `export-fgm-cue-wav.py --check` tolerates exactly +1 and fails
            # wider, so a real truncation still surfaces.
            "ds_sample_count": len(runtime_pcm),
            "net_pitch_cents": net_pitch_cents,
            "ds_frequency_hz": frequency,
            "source_duration_ticks": selector["duration_ticks"],
            "source_duration_microseconds": (
                selector["duration_ticks"] * FGM_TIMER_MICROSECONDS),
            "ds_volume": fgm_owner_volume_trim(selector["id"], volume),
            "ds_pan": 64,
            "ds_initial_volume": fgm_owner_volume_trim(selector["id"], volume),
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
                point["tick"], point["ds_volume"],
                int(point.get("event_flags", 0)))
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
    oversized = [(record["id"], len(record["ima"])) for record in records
                 if len(record["ima"]) > MAX_CUE_IMA_BYTES]
    if oversized:
        raise ValueError(
            "FGM cue body exceeds the largest runtime cache slot "
            f"({MAX_CUE_IMA_BYTES} bytes), so it can never be played: "
            + ", ".join(f"{cue_id}={size}" for cue_id, size in oversized))
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


def derive_selectors(repo_root: Path, fgm_ids: list[int]) -> list[dict]:
    """Print the source-derived descriptor for arbitrary FGM ids.

    Every SELECTED entry is hand-authored, which read as "this generator has no
    per-cue derivation mode" and made each new cue look like a research project.
    It is not: the attack lane in build_pack already walks

        fgm_ucd[id] -> set_articulation -> fgm_tbl[art] -> trigger -> sound id
        -> instrument.soundArray_offs -> B1_sounds2_ctl -> wavetable -> loop

    and that walk produces exactly the fields a SELECTED entry declares. This
    exposes it for any id, so authoring a cue means reading numbers out of the
    source tables rather than guessing them and waiting for validate_ucd to say
    no. Nothing here writes an output: the numbers still have to be pasted into
    SELECTED deliberately, where the sha256 pins catch an upstream layout change.
    """
    tools_dir = repo_root / "decomp/BattleShip-main/decomp/tools"
    extract_fgm = load_module(tools_dir / "extract_fgm.py", "extract_fgm")
    decode_ctl = load_module(tools_dir / "decode_ctl.py", "decode_ctl")
    audio_codec = load_module(tools_dir / "audio_codec.py", "audio_codec")
    audio_dir = repo_root / "decomp/BattleShip-main/BattleShip_o2r/audio"

    raw = {name: read_o2r_payload(audio_dir / name)[1]
           for name in ("fgm_tbl", "fgm_ucd", "B1_sounds2_ctl",
                        "B1_sounds2_tbl")}
    ucd = extract_fgm.decode_fgm_ucd(raw["fgm_ucd"])
    articulations = extract_fgm.decode_fgm_tbl(raw["fgm_tbl"])
    ctl_structs = decode_ctl.walk(raw["B1_sounds2_ctl"])
    ctl_by_offset = {entry["offset"]: entry for entry in ctl_structs}
    bank = [entry for entry in ctl_structs if entry["kind"] == "ALBank"][0]
    instrument = ctl_by_offset[bank["instArray_offs"][0]]

    declared = {int(selector["id"])
                for selector in (*SELECTED, *EXCLUDED_SOURCE_CUES)}
    derived = []
    for fgm_id in fgm_ids:
        entry = ucd["entries"].get(fgm_id) if isinstance(
            ucd["entries"], dict) else (
                ucd["entries"][fgm_id] if fgm_id < len(ucd["entries"]) else None)
        if entry is None:
            derived.append({"id": fgm_id, "error": "no UCD entry"})
            continue
        program = entry["program"]
        row: dict = {
            "id": fgm_id,
            "already_declared": fgm_id in declared,
            "root_program_sha256": json_sha256(program),
            "root_ops": sorted({op[0] for op in program}),
        }
        try:
            articulation_id = first_program_arg(program, "set_articulation")
            art_program = articulations["entries"][articulation_id]["program"]
            sound_id = first_program_arg(art_program, "trigger")
            sound = ctl_by_offset[instrument["soundArray_offs"][sound_id]]
            wave = ctl_by_offset[sound["wavetable_off"]]
            loop = ctl_by_offset[wave["loop_off"]] if wave["loop_off"] else None
            notes = tuple(tuple(int(v) for v in op[1:])
                          for op in program if op[0] == "note")
            volumes = [int(op[1]) for op in program if op[0] == "set_volume"]
            pitches = [int(op[1]) for op in art_program if op[0] == "pitch"]
            row.update({
                "articulation": articulation_id,
                "sound": sound_id,
                "notes": notes,
                "duration_ticks": sum(note[2] for note in notes),
                "ucd_volume": volumes[0] if volumes else None,
                "articulation_pitch_cents": pitches[0] if pitches else None,
                "loop": loop is not None,
                "wave_base": wave["base"],
                "wave_length": wave["length"],
                "loop_start": loop["start"] if loop else 0,
                "loop_end": loop["end"] if loop else 0,
                "fork_voices": tuple(int(op[1]) for op in program
                                     if op[0] == "fork_voice"),
                "articulation_ops": sorted({op[0] for op in art_program}),
                "articulation_program_sha256": json_sha256(art_program),
            })
            # The three fields a SELECTED entry still could not be authored
            # from: the decoded sample count (which `retain_full_source`
            # declares as expected_retained_samples), and the program hashes
            # for the root's fork voices. Without these, adding a forked cue
            # meant pasting a placeholder, running the generator, and reading
            # the correct value out of its error -- once per field, per cue.
            book = ctl_by_offset[wave["book_off"]]
            vadpcm = raw["B1_sounds2_tbl"][
                wave["base"]:wave["base"] + wave["length"]]
            row["source_pcm_samples"] = len(audio_codec.adpcm_decode(
                vadpcm, book["entries"], book["order"],
                book["npredictors"]))
            row["fork_program_sha256"] = tuple(
                json_sha256(ucd["entries"][fork]["program"])
                for fork in row["fork_voices"])
            row["render_program_sha256"] = row["root_program_sha256"]
        except (KeyError, IndexError, ValueError) as error:
            row["error"] = f"{type(error).__name__}: {error}"
        derived.append(row)
    return derived


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--repo-root", type=Path,
                        default=_paths.REPO_ROOT)
    parser.add_argument("--derive", metavar="IDS",
                        help="comma-separated FGM ids: print their "
                             "source-derived selector fields and exit")
    parser.add_argument("--out-bin", type=Path,
                        default=Path("assets/audio/fgm_phase_pack_ima.bin"))
    parser.add_argument("--out-json", type=Path,
                        default=Path("assets/audio/fgm_phase_pack_ima.json"))
    parser.add_argument("--check", action="store_true",
                        help="rebuild in memory and compare existing outputs")
    args = parser.parse_args()
    repo_root = args.repo_root.resolve()
    if args.derive:
        ids = [int(token, 0) for token in args.derive.replace(" ", "").split(",")
               if token]
        print(json.dumps(derive_selectors(repo_root, ids), indent=2,
                         default=list))
        return 0
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
