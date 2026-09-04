#!/usr/bin/env python3
"""Control-clock differential for the fighter pose engine's fixed-point clock.

The source advances every fighter joint's figatree with a float32 clock
(`decomp/BattleShip-main/decomp/src/ft/ftanim.c` ftAnimParseDObjFigatree:
`anim_wait -= anim_speed`, return while `anim_wait > 0`, then parse commands
while `anim_wait <= 0`, each `*Block`/`Block`/`SetFlags` command adding its
integer frame count to `anim_wait`). The port runs the same machine on a Q12
integer (`src/nds/nds_ft_pose.c:638-645`, waits entered as `payload << 12` at
lines 697/740/775/802/834/865/890, speed converted once per update by
`ndsR2F32ToFixed` at line 1160, `include/nds/nds_anim_fixed.h:114`).

This script runs BOTH clocks tick by tick over the speeds the game actually
sets and the command-wait streams the live clips actually carry, and reports
every command boundary or End that lands on a different tick. It is the test
the comment block at `src/nds/nds_ft_pose.c:74-120` calls for. It is a
measurement, not a gate: exit status is always 0.

WHAT IS MODELLED
  * float32 chain: source order, every add/sub rounded to binary32 (exact
    rational then nearest-even, no double rounding); attach tick is the
    AOBJ_ANIM_CHANGED path with frame_begin 0 (`anim_wait = -anim_frame = 0`,
    then parse), which is what every live speed below enters through
    (`ftMainSetStatus(..., 0.0F, speed, ...)`).
  * fixed chain: a bit-exact model of `ndsR2F32ToFixed(speed, bits)` for
    bits in {12, 16, 20, 24} (round half away from zero on the magnitude,
    saturation when the post-normalisation shift exceeds 7), waits as
    `payload << bits`, subtract, parse while `<= 0`.
  * the published frame (`gobj->anim_frame`, `nds_ft_pose.c:1208-1212`,
    exact Q -> f32) against the landing interrupt threshold
    `FTCOMMON_LANDING_INTERRUPT_BEGIN` 4.0F (`ftcommonlanding.c:40`), as a
    secondary boundary.
WHAT IS NOT
  * hitlag, speed changes mid-clip, non-zero frame_begin, Loop paths, the
    motion-script `script_wait` chain (`ftmain.c:697-747`, still float32 in
    the port). Every live clip used here has no Loop; the script refuses a
    looping stream rather than unrolling it.

SPEED SET (file:line evidence in the docstrings of `speed_table`):
  DObj default 1.0; landing light 1.0 / heavy 0.5; item swing 1.0/0.75/2.0
  via F_PCT_TO_DEC; FallSpecial landing lag per fighter; AttackAir smooth
  landing F_PCT_TO_DEC(flag1) where the fighter's LandingAir slot is NULL
  (only then is `ftCommonLandingAirNullSetStatus` reached,
  `ftcommonattackair.c:60-67`); rebound 16.0F / ((f32)damage * 1.62F + 4.0F)
  (`ftcommonrebound.c:44`, `ftmain.c:2021` REGION_US, `relocData/*Main.c`
  rebound_anim_length 16.0f for every fighter) for damage 1..100; plus wall
  damage 2.0F, Giant Punch charge 2.0F and Samus charge-shot start speeds.

WAIT STREAMS: decoded from the o2r bank through `scripts/ftanim_reloc_probe.py`
for the clips the landing and rebound motions point at (`ftdata.c`
dFT*MotionDescs rows 25/26/52/194 -> LandingAirX, row 71 -> ClangRecoil;
asset ids from `scripts/fighters/fighter_production_manifest.json` for
Mario/Fox, bank index 19/57 for the others, corroborated by the 16-frame
rebound invariant). Mario/Fox streams are also embedded so the run is
reproducible without the bank. A synthetic sweep covers single waits 1..64,
all pairs 1..16, all triples 1..8 and a seeded random set of 4..8-command
sequences with waits 1..16.

Run: python scripts/fighters/test_pose_clock_differential.py
Writes artifacts/verification/2026-09-04_pose-clock-differential.txt.
"""
from __future__ import annotations

import argparse
import importlib.util
import itertools
import pathlib
import random
import struct
import sys
from fractions import Fraction

ROOT = pathlib.Path(__file__).resolve().parents[2]
BANK = ROOT / "decomp" / "BattleShip-main" / "BattleShip_o2r" / "reloc_animations"
PROBE = ROOT / "scripts" / "ftanim_reloc_probe.py"
OUT = ROOT / "artifacts" / "verification" / "2026-09-04_pose-clock-differential.txt"

PRECISIONS = (12, 16, 20, 24)
TICK_CAP = 250000
INTERRUPT_BEGIN = 4          # FTCOMMON_LANDING_INTERRUPT_BEGIN 4.0F, integer frames
SHIPPED = ("Mario", "Fox")   # the fighters the port ships today

# --------------------------------------------------------------------------- #
# binary32, done exactly.

def f32(x: float) -> float:
    return struct.unpack("<f", struct.pack("<f", x))[0]


def f32_bits(x: float) -> int:
    return struct.unpack("<I", struct.pack("<f", x))[0]


def f32_of(fr: Fraction) -> float:
    """Nearest-even binary32 of an exact rational. No intermediate double, so a
    decimal literal or a quotient is rounded ONCE, the way the compiler and the
    FPU round it."""
    if fr == 0:
        return 0.0
    sign = -1.0 if fr < 0 else 1.0
    fr = abs(fr)
    e = fr.numerator.bit_length() - fr.denominator.bit_length()
    while fr >= Fraction(2) ** (e + 1):
        e += 1
    while fr < Fraction(2) ** e:
        e -= 1
    if e < -126:
        raise ValueError("subnormal speeds are not modelled")
    scaled = fr * Fraction(2) ** (23 - e)
    n, rem = divmod(scaled.numerator, scaled.denominator)
    rem = Fraction(rem, scaled.denominator)
    if rem > Fraction(1, 2) or (rem == Fraction(1, 2) and (n & 1)):
        n += 1
    if n == 1 << 24:
        n >>= 1
        e += 1
    return sign * n * 2.0 ** (e - 23)


def lit(text: str) -> float:
    """A C float literal: the decimal rounded once to binary32."""
    return f32_of(Fraction(text))


def f32_add(a: float, b: float) -> float:
    """binary32 a + b. The double sum is exact for every operand pair this
    script produces (TwoSum proves it per call); otherwise fall back to the
    rational path so the rounding is still a single nearest-even step."""
    s = a + b
    bb = s - a
    err = (a - (s - bb)) + (b - bb)
    if err != 0.0:
        return f32_of(Fraction(a) + Fraction(b))
    return f32(s)


def f32_mul(a: float, b: float) -> float:
    return f32_of(Fraction(a) * Fraction(b))


def f32_div(a: float, b: float) -> float:
    return f32_of(Fraction(a) / Fraction(b))


# --------------------------------------------------------------------------- #
# `ndsR2F32ToFixed`, bit for bit (include/nds/nds_anim_fixed.h:114-157).

def nds_f32_to_fixed(v: float, bits: int):
    """Returns (q, saturated)."""
    raw = f32_bits(v)
    exp = (raw >> 23) & 0xFF
    neg = (raw & 0x80000000) != 0
    if exp == 0:
        return 0, False
    if exp == 0xFF:
        return (-0x7FFFFFFF if neg else 0x7FFFFFFF), True
    mant = (raw & 0x7FFFFF) | 0x800000
    shift = (exp - 127) - 23 + bits
    sat = False
    if shift >= 0:
        if shift > 7:
            mant = 0x7FFFFFFF
            sat = True
        else:
            mant <<= shift
    elif shift < -24:
        mant = 0
    else:
        mant = (mant + (1 << (-shift - 1))) >> -shift
    return (-mant if neg else mant), sat


def reference_round(v: float, bits: int) -> int:
    """Round half away from zero of the exact f32 value scaled by 2^bits --
    the arithmetic the converter is documented to perform; used to check the
    bit model against it."""
    fr = Fraction(v) * (1 << bits)
    mag = abs(fr)
    n = mag.numerator // mag.denominator
    if mag - n >= Fraction(1, 2):
        n += 1
    return -n if fr < 0 else n


# --------------------------------------------------------------------------- #
# The two clocks.

def float_chain(speed: float, waits, thresholds=(INTERRUPT_BEGIN,)):
    """Returns (ticks, end_tick, threshold_ticks). ticks[i] is the tick on
    which command i's wait was added (its predecessor's boundary crossed);
    end_tick is the tick the End command is reached (status end for statuses
    on ftAnimEndSetWait). None end = did not finish within the cap."""
    wait = 0.0
    frame = 0.0
    idx = 0
    ticks = []
    thr = {}
    n = len(waits)
    while wait <= 0.0:                       # attach tick: CHANGED, frame_begin 0
        if idx == n:
            return ticks, 0, thr
        wait = f32_add(wait, float(waits[idx]))
        ticks.append(0)
        idx += 1
    if speed <= 0.0:
        return ticks, None, thr
    limit = min(TICK_CAP, int(sum(waits) / speed) + 16)
    for tick in range(1, limit + 1):
        wait = f32_add(wait, -speed)
        frame = f32_add(frame, speed)
        for t in thresholds:
            if t not in thr and frame >= float(t):
                thr[t] = tick
        if wait > 0.0:
            continue
        while wait <= 0.0:
            if idx == n:
                return ticks, tick, thr
            wait = f32_add(wait, float(waits[idx]))
            ticks.append(tick)
            idx += 1
    return ticks, None, thr


def fixed_chain(speed_q: int, waits, bits: int, thresholds=(INTERRUPT_BEGIN,)):
    wait = 0
    frame = 0
    idx = 0
    ticks = []
    thr = {}
    n = len(waits)
    while wait <= 0:
        if idx == n:
            return ticks, 0, thr
        wait += waits[idx] << bits
        ticks.append(0)
        idx += 1
    if speed_q <= 0:
        return ticks, None, thr
    limit = min(TICK_CAP, (sum(waits) << bits) // speed_q + 16)
    tq = {t: t << bits for t in thresholds}
    for tick in range(1, limit + 1):
        wait -= speed_q
        frame += speed_q
        for t in thresholds:
            if t not in thr and frame >= tq[t]:
                thr[t] = tick
        if wait > 0:
            continue
        while wait <= 0:
            if idx == n:
                return ticks, tick, thr
            wait += waits[idx] << bits
            ticks.append(tick)
            idx += 1
    return ticks, None, thr


# --------------------------------------------------------------------------- #
# Speeds.

def pct(x_int: int) -> float:
    """F_PCT_TO_DEC(x) = (float)((x)*0.01F) with an integer x (macros.h:63)."""
    return f32_mul(f32(float(x_int)), lit("0.01"))


def rebound_speed(damage: int) -> float:
    """ftmain.c:2021 (REGION_US) then ftcommonrebound.c:44, each op binary32."""
    attack_rebound = f32_add(f32_mul(f32(float(damage)), lit("1.62")), lit("4.0"))
    return f32_div(lit("16.0"), attack_rebound)


def samus_charge_speed(level: int) -> float:
    """ftsamusspecialn.c:322-329, FTSAMUS_CHARGE_MAX 7 (ftsamus.h:8)."""
    ret = f32_div(f32(float(level)), f32(7.0))
    return f32_add(f32_mul(lit("-0.16000003"), ret), lit("1.0"))


def speed_table():
    """Each entry: label, value, pairing, fighters (None = every fighter),
    sweep flag, gameplay note. Provenance:
      objman.c:1382 DObj default 1.0F
      ftcommon.h:149-150 landing heavy 0.5F / light 1.0F
      ftcommondata.c:326-346 item swing 100/75/200 % via F_PCT_TO_DEC
      ftmario.h:11 0.28F, ftfox.h:20 0.34F, ftdonkey.h:27 0.3F,
      ftcaptain.h:19 0.65F, ftlink.h:21 0.65F (REGION_US; Makefile:3650),
      ftness.h:30 F_PCT_TO_DEC(17.0F) (REGION_US), ftpikachu.h:34 0.4F,
      ftsamus.h:23 0.4F -- all reach the clock through
      ftCommonFallSpecialSetStatus -> ftCommonLandingFallSpecialSetStatus
      -> ftCommonLandingSetStatusParam -> ftMainSetStatus(..., 0.0F, lag, ...)
      ftcommonattackair.c:67 F_PCT_TO_DEC(flag1); flag1 authored by
      ftMotionCommandSetFlag1 in relocData/*MainMotion.c AttackAir scripts;
      the NULL LandingAir slots are ftdata.c dFT*MotionDescs rows 189-193.
      ftcommonwalldamage (ftcommon) 2.0F; ftdonkey.h:12 2.0F; Samus above."""
    rows = []

    def add(label, value, pairing, fighters=None, sweep=False, note=""):
        rows.append({"label": label, "value": value, "pairing": pairing,
                     "fighters": fighters, "sweep": sweep, "note": note})

    add("DObj default 1.0F", lit("1.0"), "generic")
    add("LandingLight 1.0F", lit("1.0"), "landing", note="status ends on ftAnimEndSetWait")
    add("LandingHeavy 0.5F", lit("0.5"), "landing", note="status ends on ftAnimEndSetWait")
    add("ItemSwing 100% -> F_PCT_TO_DEC", pct(100), "generic")
    add("ItemSwing 75% (BatSwing4)", pct(75), "generic")
    add("ItemSwing 200% (Harisen)", pct(200), "generic")
    add("WallDamage 2.0F", lit("2.0"), "generic")
    add("GiantPunch charge 2.0F", lit("2.0"), "generic")
    for lv in range(0, 8):
        add("Samus charge start level %d" % lv, samus_charge_speed(lv), "generic")

    fall = [("Mario", "Super Jump Punch", lit("0.28")),
            ("Fox", "Fire Fox", lit("0.34")),
            ("Donkey", "Spinning Kong", lit("0.3")),
            ("Captain", "Falcon Dive", lit("0.65")),
            ("Link", "Spin Attack (US)", lit("0.65")),
            ("Ness", "PK Thunder (US) F_PCT_TO_DEC(17.0F)", f32_mul(lit("17.0"), lit("0.01"))),
            ("Pikachu", "Quick Attack", lit("0.4")),
            ("Samus", "Screw Attack", lit("0.4"))]
    for fighter, move, v in fall:
        add("%s FallSpecial landing lag (%s)" % (fighter, move), v, "landing",
            fighters={fighter}, note="LandingFallSpecial: status ends on ftAnimEndSetWait")

    # (fighter, aerial, flag1) where the LandingAir slot is NULL so
    # ftCommonLandingAirNullSetStatus(F_PCT_TO_DEC(flag1)) is the path taken.
    null_air = [("Mario", "nair", 50), ("Mario", "dair", 20),
                ("Luigi", "nair", 50), ("Luigi", "dair", 20),    # Luigi plays Mario's clips
                ("Fox", "nair", 50), ("Fox", "uair", 20), ("Fox", "dair", 20),
                ("Donkey", "nair", 50), ("Donkey", "uair", 20), ("Donkey", "dair", 20),
                ("Samus", "nair", 50), ("Samus", "fair", 20), ("Samus", "dair", 20),
                ("Captain", "nair", 50), ("Captain", "uair", 50), ("Captain", "dair", 40),
                ("Ness", "nair", 50),
                ("Pikachu", "nair", 50), ("Pikachu", "bair", 50),
                ("Yoshi", "nair", 50), ("Yoshi", "uair", 50), ("Yoshi", "dair", 50),
                # Link's desc rows could not be aligned to the enum (see report);
                # its AttackAir scripts author 50/20/30, listed as possible.
                ("Link", "nair?", 50), ("Link", "uair?", 20), ("Link", "dair?", 30)]
    for fighter, air, flag1 in null_air:
        fl = {"Luigi": "Mario"}.get(fighter, fighter)
        add("%s LandingAirNull %s (flag1 %d)" % (fighter, air, flag1), pct(flag1), "landing",
            fighters={fl}, note="LandingAirNull: status ends on ftAnimEndSetWait")

    for flag1 in range(1, 101):
        add("flag1 sweep %d" % flag1, pct(flag1), "landing", sweep=True)
    for d in range(1, 101):
        add("Rebound damage %d" % d, rebound_speed(d), "rebound", sweep=True,
            note="status ends on rebound_timer, not the clip: End shift is pose-only")
    return rows


# --------------------------------------------------------------------------- #
# Wait streams.

# Bank clips: (fighter, clip, asset id, bank index, how the id was confirmed).
CLIPS = [
    ("Mario", "LandingAirX", 518, 19, "manifest id 518; ftdata.c rows 25/26/52/194"),
    ("Mario", "ClangRecoil", 556, 57, "manifest id 556; ftdata.c row 71"),
    ("Fox", "LandingAirX", 661, 19, "manifest id 661; ftdata.c rows 25/26/52/194"),
    ("Fox", "ClangRecoil", 699, 57, "manifest id 699; ftdata.c row 71"),
    ("Donkey", "LandingAirX", 819, 19, "bank index 19 (inferred; 16-frame rebound twin corroborates)"),
    ("Donkey", "ClangRecoil", 857, 57, "bank index 57 (inferred; 16 frames = rebound_anim_length)"),
    ("Samus", "LandingAirX", 972, 19, "bank index 19 (inferred)"),
    ("Samus", "ClangRecoil", 1010, 57, "bank index 57 (inferred; 16 frames)"),
    ("Link", "LandingAirX", 1134, 19, "bank index 19 (inferred)"),
    ("Link", "ClangRecoil", 1172, 57, "bank index 57 (inferred; 16 frames)"),
    ("Captain", "LandingAirX", 1531, 19, "bank index 19 (inferred)"),
    ("Captain", "ClangRecoil", 1569, 57, "bank index 57 (inferred; 16 frames)"),
    ("Ness", "LandingAirX", 1683, 19, "bank index 19 (inferred)"),
    ("Ness", "ClangRecoil", 1721, 57, "bank index 57 (inferred; 16 frames)"),
]
CLIP_KIND = {"LandingAirX": "landing", "ClangRecoil": "rebound"}
# Luigi's desc rows point at Mario's animation files (ftdata.c dFTLuigiMotionDescs).
CLIP_FIGHTERS = {"Mario": {"Mario", "Luigi"}}

# The shipped fighters' streams, unique per-joint wait tuples with the joint
# entries that carry them, decoded 2026-09-04 from the bank. The run checks
# these against a live decode whenever the bank is present.
EMBEDDED = {
    518: [((0, 1, 6), [0, 2]), ((0, 7), [1]), ((0, 4, 1, 1, 1), [4, 10]),
          ((0, 5, 1, 1), [5]), ((0, 3, 4), [7]), ((7,), [8, 13]),
          ((0, 6, 1), [11, 18, 20, 23]), ((3, 4), [12]), ((0, 2, 1, 3, 1), [15, 16, 21])],
    556: [((0, 4, 7, 5), [0, 2, 7]), ((16,), [1, 8, 13]),
          ((0, 1, 1, 1, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1), [4]),
          ((0, 5, 1, 3, 1, 1, 1, 2, 1, 1), [5]), ((0, 6, 5, 5), [6]),
          ((0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1), [10]),
          ((0, 1, 1, 1, 1, 3, 1, 3, 1, 1, 1, 1, 1), [11]), ((0, 2, 10, 4), [12]),
          ((0, 3, 1, 2, 1, 1, 1, 1, 2, 1, 1, 1, 1), [15]),
          ((0, 3, 1, 1, 1, 1, 1, 4, 1, 2, 1), [16]),
          ((0, 2, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1), [18]),
          ((0, 1, 1, 1, 1, 1, 1, 2, 1, 2, 1, 1, 1, 1, 1), [20]),
          ((0, 1, 1, 2, 1, 4, 1, 3, 1, 1, 1), [21]),
          ((0, 1, 1, 1, 1, 1, 1, 3, 1, 1, 1, 1, 1, 1, 1), [23])],
    661: [((0, 4, 4), [0]), ((0, 8), [1, 8]), ((0, 1, 2, 1, 4), [2]),
          ((0, 1, 1, 1, 1, 1, 1, 1, 1), [4]), ((0, 1, 1, 1, 1, 2, 1, 1), [5]),
          ((8,), [6, 13]), ((0, 2, 2, 4), [7, 12]), ((0, 2, 1, 1, 1, 1, 1, 1), [10]),
          ((0, 2, 1, 2, 1, 1, 1), [11]), ((0, 6, 1, 1), [15, 16]), ((0, 3, 1, 3, 1), [18]),
          ((0, 2, 1, 4, 1), [20]), ((0, 1, 1, 4, 1, 1), [21]), ((0, 2, 1, 1, 3, 1), [23]),
          ((0, 4, 2, 2), [24, 25])],
    699: [((0, 4, 7, 5), [0, 2, 7]), ((16,), [1, 13]),
          ((0, 1, 1, 4, 1, 1, 1, 3, 1, 1, 1, 1), [4]), ((0, 1, 1, 6, 1, 3, 1, 2, 1), [5]),
          ((0, 5, 1, 5, 1, 1, 1, 1, 1), [10]), ((0, 8, 1, 3, 1, 2, 1), [11]),
          ((0, 4, 4, 4, 2, 2), [12]), ((0, 2, 1, 3, 1, 1, 6, 1, 1), [15]),
          ((0, 1, 1, 1, 1, 6, 1, 4, 1), [16]), ((0, 1, 1, 1, 1, 5, 1, 1, 1, 1, 2, 1), [18]),
          ((0, 5, 1, 1, 1, 1, 3, 1, 1, 1, 1), [20]), ((0, 4, 1, 5, 1, 2, 1, 1, 1), [21]),
          ((0, 5, 1, 1, 1, 6, 1, 1), [23]), ((0, 4, 3, 9), [24]), ((0, 4, 8, 4), [25])],
}
WAIT_OPS = {1, 2, 4, 7, 9, 14}   # Block, SetVal0RateBlock, SetValBlock, SetValRateBlock, SetValAfterBlock, SetFlags


def load_probe():
    if not PROBE.exists():
        return None
    spec = importlib.util.spec_from_file_location("ftanim_reloc_probe", PROBE)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


def decode_clip(probe, path: pathlib.Path):
    """(file_id, [(wait tuple, [joints])], loop_count, max_payload) or None."""
    f = probe.load(path.read_bytes())
    if f is None or f["file_id"] in probe.AOBJ32_IDS:
        return None
    fx = probe.fixups(f)
    table = probe.normalize(f, fx)
    entries = [fx.get(j * 4) for j in range(table // 4)]
    seqs = {}
    loops = 0
    max_payload = 0
    for j, off in enumerate(entries):
        if off is None:
            continue
        waits = []
        for c in probe.decode_script(f["data"], f["size"], off):
            if c.get("cyclic") or c["op"] == 13:
                loops += 1
            if c["payload"]:
                max_payload = max(max_payload, c["payload"])
            if c["op"] in WAIT_OPS:
                waits.append(c["payload"] or 0)
        seqs.setdefault(tuple(waits), []).append(j)
    return f["file_id"], sorted(seqs.items(), key=lambda kv: kv[1][0]), loops, max_payload


def real_streams(probe, notes):
    """Wait streams from the bank, falling back to EMBEDDED for Mario/Fox."""
    streams = []
    have_bank = probe is not None and BANK.exists()
    for fighter, clip, asset_id, index, how in CLIPS:
        seqs = None
        if have_bank:
            path = BANK / ("FT%sAnim%03d" % (fighter, index))
            if path.exists():
                got = decode_clip(probe, path)
                if got is None:
                    notes.append("%s %s: bank file %s is not a figatree, skipped" % (fighter, clip, path.name))
                elif got[0] != asset_id:
                    notes.append("%s %s: bank file %s has id %d, expected %d, skipped"
                                 % (fighter, clip, path.name, got[0], asset_id))
                elif got[2]:
                    notes.append("%s %s: clip loops (%d Loop commands), refused" % (fighter, clip, got[2]))
                else:
                    seqs = got[1]
                    if asset_id in EMBEDDED:
                        same = sorted(EMBEDDED[asset_id]) == sorted(seqs)
                        notes.append("%s %s id %d: bank decode %s the embedded streams"
                                     % (fighter, clip, asset_id, "MATCHES" if same else "DIFFERS FROM"))
                    notes.append("%s %s id %d: %d joint entries, %d distinct streams, max payload %d, %s"
                                 % (fighter, clip, asset_id, sum(len(j) for _, j in seqs), len(seqs), got[3], how))
            else:
                notes.append("%s %s: %s absent from the bank" % (fighter, clip, path.name))
        if seqs is None and asset_id in EMBEDDED:
            seqs = EMBEDDED[asset_id]
            notes.append("%s %s id %d: using the embedded streams (%s)" % (fighter, clip, asset_id, how))
        if seqs is None:
            continue
        for waits, joints in seqs:
            streams.append({"label": "%s %s id%d j%s" % (fighter, clip, asset_id, joints),
                            "waits": tuple(waits), "kind": CLIP_KIND[clip],
                            "fighters": CLIP_FIGHTERS.get(fighter, {fighter}),
                            "shipped": fighter in SHIPPED, "clip": clip, "fighter": fighter})
    return streams


def synthetic_streams(random_count: int):
    out = []
    for w in range(1, 65):
        out.append({"label": "single wait %d" % w, "waits": (w,), "kind": "single"})
    for a, b in itertools.product(range(1, 17), repeat=2):
        out.append({"label": "pair", "waits": (a, b), "kind": "synthetic"})
    for t in itertools.product(range(1, 9), repeat=3):
        out.append({"label": "triple", "waits": t, "kind": "synthetic"})
    rng = random.Random(20260904)
    for _ in range(random_count):
        n = rng.randint(4, 8)
        out.append({"label": "random", "waits": tuple(rng.randint(1, 16) for _ in range(n)),
                    "kind": "synthetic"})
    return out


# --------------------------------------------------------------------------- #
# Case generation and comparison.

def pairing_kind(speed, stream):
    sk = stream["kind"]
    if speed["sweep"]:
        if sk == "single":
            return "sweep-synthetic"
        if sk == speed["pairing"]:
            return "live"
        return None
    if sk in ("single", "synthetic"):
        return "synthetic"
    if speed["pairing"] == "generic":
        return "generic"
    if speed["pairing"] != sk:
        return None
    if speed["fighters"] is None or (speed["fighters"] & stream["fighters"]):
        return "live"
    return "stand-in"


def fmt_speed(v: float) -> str:
    return "%.9g (0x%08x)" % (v, f32_bits(v))


def run(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--out", type=pathlib.Path, default=OUT)
    ap.add_argument("--random", type=int, default=500, help="random synthetic sequences (seeded)")
    ap.add_argument("--no-bank", action="store_true", help="ignore the o2r bank, use embedded streams only")
    ap.add_argument("--list-limit", type=int, default=60,
                    help="mismatch rows printed per precision on stdout (file gets all)")
    args = ap.parse_args(argv)

    lines = []
    notes = []
    probe = None if args.no_bank else load_probe()
    speeds = speed_table()
    streams = real_streams(probe, notes) + synthetic_streams(args.random)

    lines.append("Pose clock differential: float32 source clock vs ndsR2F32ToFixed Q-clock")
    lines.append("bank: %s" % ("present, decoded live" if (probe and BANK.exists() and not args.no_bank)
                              else "absent or disabled, embedded Mario/Fox streams only"))
    for n in notes:
        lines.append("  " + n)

    # ---- conversion model self-check and saturation ------------------------
    lines.append("")
    lines.append("== Converter model and saturation ==")
    agree = 0
    total = 0
    sat_rows = []
    for s in speeds:
        for bits in PRECISIONS:
            q, sat = nds_f32_to_fixed(s["value"], bits)
            total += 1
            if q == reference_round(s["value"], bits) or sat:
                agree += 1
            if sat:
                sat_rows.append((bits, s["label"], s["value"]))
    lines.append("bit model == round-half-away(v * 2^bits) on %d/%d (speed, bits) pairs" % (agree, total))
    for bits in PRECISIONS:
        q16, sat16 = nds_f32_to_fixed(lit("16.0"), bits)
        vmax = max(s["value"] for s in speeds)
        lines.append("Q%d: 16.0F -> %d %s; largest live speed %.9g -> %d %s; first saturating magnitude 2^%d; "
                     "largest wait/length payload that fits s32 = %d frames (NDS_R2_AQ_LEN_MAX budgets 1024)"
                     % (bits, q16, "SATURATES" if sat16 else "ok", vmax, nds_f32_to_fixed(vmax, bits)[0],
                        "SATURATES" if nds_f32_to_fixed(vmax, bits)[1] else "ok",
                        31 - bits, (1 << (31 - bits)) - 1))
    if sat_rows:
        for bits, label, v in sat_rows:
            lines.append("  SATURATED at Q%d: %s = %.9g" % (bits, label, v))
    else:
        lines.append("no live speed saturates at any tested precision")

    # ---- the differential ----------------------------------------------------
    cases = []
    for s in speeds:
        for st in streams:
            kind = pairing_kind(s, st)
            if kind is None:
                continue
            cases.append((s, st, kind))
    lines.append("")
    lines.append("== Cases ==")
    kinds = {}
    for _, _, k in cases:
        kinds[k] = kinds.get(k, 0) + 1
    lines.append("%d cases: %s" % (len(cases), ", ".join("%s %d" % kv for kv in sorted(kinds.items()))))
    lines.append("streams: %d real (%d landing, %d rebound), %d synthetic (64 singles, 256 pairs, 512 triples, %d random)"
                 % (sum(1 for st in streams if st["kind"] in ("landing", "rebound")),
                    sum(1 for st in streams if st["kind"] == "landing"),
                    sum(1 for st in streams if st["kind"] == "rebound"),
                    sum(1 for st in streams if st["kind"] in ("single", "synthetic")), args.random))

    float_cache = {}
    results = {bits: [] for bits in PRECISIONS}
    unfinished = 0
    for s, st, kind in cases:
        key = (s["value"], st["waits"])
        if key not in float_cache:
            float_cache[key] = float_chain(s["value"], st["waits"])
        f_ticks, f_end, f_thr = float_cache[key]
        if f_end is None:
            unfinished += 1
            continue
        for bits in PRECISIONS:
            q, _ = nds_f32_to_fixed(s["value"], bits)
            x_ticks, x_end, x_thr = fixed_chain(q, st["waits"], bits)
            first = None
            for i, (a, b) in enumerate(zip(f_ticks, x_ticks)):
                if a != b:
                    first = (i, a, b)
                    break
            end_diff = (x_end is None) or (x_end != f_end)
            thr_diff = f_thr.get(INTERRUPT_BEGIN) != x_thr.get(INTERRUPT_BEGIN)
            if first is not None or end_diff or thr_diff:
                results[bits].append({
                    "speed": s, "stream": st, "kind": kind, "first": first,
                    "f_end": f_end, "x_end": x_end, "q": q,
                    "f_thr": f_thr.get(INTERRUPT_BEGIN), "x_thr": x_thr.get(INTERRUPT_BEGIN),
                    "late": (None if x_end is None else x_end - f_end)})
    if unfinished:
        lines.append("%d cases did not finish within the tick cap (speed 0 or cap) and were skipped" % unfinished)

    # ---- per-precision tables ----------------------------------------------
    stdout_extra = []
    for bits in PRECISIONS:
        rows = results[bits]
        n_cases = len(cases) - unfinished
        clock_rows = [r for r in rows if r["first"] is not None or r["x_end"] != r["f_end"]]
        thr_only = [r for r in rows if r not in clock_rows]
        live = [r for r in clock_rows if r["kind"] == "live"]
        live_shipped = [r for r in live if r["stream"].get("shipped")]
        lates = [r["late"] for r in clock_rows if r["late"] is not None]
        lines.append("")
        lines.append("== Q%d ==" % bits)
        lines.append("total cases %d; boundary/End mismatches %d (live %d, live-shipped %d, stand-in %d, generic %d, "
                     "synthetic %d, sweep-synthetic %d); interrupt-threshold-only mismatches %d"
                     % (n_cases, len(clock_rows), len(live), len(live_shipped),
                        sum(1 for r in clock_rows if r["kind"] == "stand-in"),
                        sum(1 for r in clock_rows if r["kind"] == "generic"),
                        sum(1 for r in clock_rows if r["kind"] == "synthetic"),
                        sum(1 for r in clock_rows if r["kind"] == "sweep-synthetic"), len(thr_only)))
        if lates:
            lines.append("End lateness (fixed - float) over mismatching cases: min %+d, max %+d ticks; "
                         "End moved in %d of them" % (min(lates), max(lates), sum(1 for l in lates if l != 0)))
        header = ("kind | speed source | speed | Q | wait stream | first differing command (idx: float tick -> fixed tick)"
                  " | End float -> fixed | interrupt>=4 float -> fixed")
        lines.append(header)
        order = {"live": 0, "stand-in": 1, "generic": 2, "sweep-synthetic": 3, "synthetic": 4}
        rows_sorted = sorted(rows, key=lambda r: (order[r["kind"]], r["speed"]["label"], r["stream"]["label"]))
        for r in rows_sorted:
            first = "-" if r["first"] is None else "%d: %d -> %d" % r["first"]
            lines.append("%s | %s | %s | %d | %s %s | %s | %s -> %s | %s -> %s"
                         % (r["kind"], r["speed"]["label"], fmt_speed(r["speed"]["value"]), r["q"],
                            r["stream"]["label"], r["stream"]["waits"], first,
                            r["f_end"], r["x_end"], r["f_thr"], r["x_thr"]))
        stdout_extra.append((bits, len(rows_sorted)))

    # ---- gameplay summary for the shipped fighters at every precision --------
    lines.append("")
    lines.append("== Live cases on the shipped fighters (Mario, Fox), by precision ==")
    lines.append("A landing-clip End shift is a status-end shift (ftAnimEndSetWait reads anim_frame <= 0 on the End tick);")
    lines.append("a ClangRecoil End shift is pose-only (the rebound status ends on rebound_timer). An intermediate")
    lines.append("boundary shift moves that joint's keyframe by the stated ticks.")
    for bits in PRECISIONS:
        live = [r for r in results[bits] if r["kind"] == "live" and r["stream"].get("shipped")
                and (r["first"] is not None or r["x_end"] != r["f_end"])]
        by_speed = {}
        for r in live:
            by_speed.setdefault(r["speed"]["label"], []).append(r)
        lines.append("Q%d: %d live shipped cases mismatch across %d speed sources" % (bits, len(live), len(by_speed)))
        for label in sorted(by_speed):
            rs = by_speed[label]
            ends = sorted(set((r["f_end"], r["x_end"]) for r in rs))
            clips = sorted(set(r["stream"]["clip"] for r in rs))
            worst = max((r["late"] or 0) for r in rs)
            lines.append("  %s on %s: %d joint streams; End float->fixed %s; max End lateness %+d; %d of them shift the End"
                         % (label, "/".join(clips), len(rs), ends, worst,
                            sum(1 for r in rs if r["x_end"] != r["f_end"])))

    # ---- precision closure verdict -------------------------------------------
    lines.append("")
    lines.append("== Precision closure ==")
    for bits in PRECISIONS:
        live_all = [r for r in results[bits] if r["kind"] == "live" and (r["first"] is not None or r["x_end"] != r["f_end"])]
        live_named = [r for r in live_all if not r["speed"]["sweep"]]
        lines.append("Q%d: live mismatches %d (named live speeds %d, sweeps %d); all-case mismatches %d"
                     % (bits, len(live_all), len(live_named), len(live_all) - len(live_named),
                        sum(1 for r in results[bits] if r["first"] is not None or r["x_end"] != r["f_end"])))
    closing = [bits for bits in PRECISIONS
               if not any(r["kind"] == "live" and (r["first"] is not None or r["x_end"] != r["f_end"])
                          for r in results[bits])]
    lines.append("precision at which every live case matches: %s" % (closing if closing else "NONE of 12/16/20/24"))

    lines.append("")
    lines.append("== Synthetic coverage ==")
    lines.append("single initial waits 1..64 (every non-sweep speed, and both sweeps); all pairs with waits 1..16;")
    lines.append("all triples with waits 1..8; %d seeded random sequences of 4..8 commands with waits 1..16"
                 " (seed 20260904); every non-sweep speed runs against all of these." % args.random)

    text = "\n".join(lines) + "\n"
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(text, encoding="utf-8")

    # stdout: everything except the long per-precision row lists, which are capped.
    printed = 0
    cap = args.list_limit
    in_table = False
    for ln in lines:
        if ln.startswith("== Q") and ln.endswith(" =="):
            in_table = True
            printed = 0
            print(ln)
            continue
        if ln.startswith("== ") and not ln.startswith("== Q"):
            in_table = False
        if in_table and " | " in ln and not ln.startswith("kind |"):
            printed += 1
            if printed > cap:
                if printed == cap + 1:
                    print("  ... (%d more rows in %s)" % (0, args.out))
                continue
        print(ln)
    print("full result written to %s" % args.out)
    return 0


if __name__ == "__main__":
    sys.exit(run())
