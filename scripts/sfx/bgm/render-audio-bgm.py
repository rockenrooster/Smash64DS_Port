#!/usr/bin/env python3
"""Render a BattleShip BGM sequence to a DS-friendly audio stream.

This is intentionally a small compatibility renderer for the port's audible
BGM gates. It derives each stream from the original O2R sequence/bank files
and does not use hand-authored notes or third-party audio.
"""

from __future__ import annotations

import argparse
from array import array
import hashlib
import importlib.util
import json
import math
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



# Sequence indices are the gmMusicID enum values: decomp gm/gmsound.h:31-40
# runs sequentially from 0 with no explicit values and no REGION_US arm inside
# the BGM range, so Dream Land is 0, Zebes 1, Mushroom Kingdom 2, its hurry
# variant 3, Sector Z 4, Congo Jungle 5, Peach's Castle 6, Saffron City 7,
# Yoshi's Island 8, Hyrule Castle 9. Pass --sequence-index and --output to
# render any of them; the default renders Dream Land, which is what this
# script was written for and what every checked-in metadata file cites under
# its old name, render-audio-bgm-pupupu.py.
SEQ_INDEX_PUPUPU = 0
SOURCE_BANK_SAMPLE_RATE = 32000
OUTPUT_SAMPLE_RATE = 22050
SOURCE_MAX_PITCH_RATIO = 1.99996
DEFAULT_GAIN = 0.22
MASTER_VOLUME_CONTROLLER = 21
PITCH_BEND_RANGE_CONTROLLER = 0x14
SEQUENCE_PLAYER_DEFAULT_MASTER_VOLUME = 100
# Dream Land is the accepted reference stream. Its generated payload is a
# byte-for-byte regression guard, so sequence 0 deliberately retains the old
# direct 32 kHz -> 22.05 kHz per-voice render. Every other sequence mixes at
# the source bank rate first, then band-limits the completed mix to 22.05 kHz.
LEGACY_DIRECT_RESAMPLE_SEQUENCES = frozenset((SEQ_INDEX_PUPUPU,))
# BattleShip's SYAudioSettings field order is intentionally counter-intuitive:
# bank1 is B1_sounds2, bank2 is B1_sounds1, and syAudioMakeBGMPlayers binds
# sSYAudioSequenceBank2 to every compressed-sequence player. Keep these names
# tied to the code binding, not to the container number.
BGM_SEQUENCE_BANK_CTL = "B1_sounds1_ctl"
BGM_SEQUENCE_BANK_TBL = "B1_sounds1_tbl"
BGM_SEQUENCE_BANK_SOURCE = f"{BGM_SEQUENCE_BANK_CTL}/tbl"
BGM_SEQUENCE_BANK_BINDING = f"sSYAudioSequenceBank2 -> {BGM_SEQUENCE_BANK_SOURCE}"
BGM_IMA_MAGIC = b"BGA1"
BGM_IMA_VERSION = 1
BGM_IMA_PACKET_SAMPLES = 16384
BGM_IMA_HEADER = struct.Struct("<4sHHIIIIIIII")
BGM_IMA_PACKET = struct.Struct("<II")
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


def load_module(path: Path, name: str):
    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot import {path}")
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


def read_o2r_payload(path: Path) -> bytes:
    data = path.read_bytes()
    if len(data) < 0x44:
        raise RuntimeError(f"{path} is too small for an O2R wrapper")
    return data[0x44:]


def read_seq(raw_sbk: bytes, index: int) -> bytes:
    if struct.unpack_from(">H", raw_sbk, 0)[0] != 0x5331:
        raise RuntimeError("S1_music_sbk does not have the expected sequence header")
    count = struct.unpack_from(">H", raw_sbk, 2)[0]
    if index >= count:
        raise RuntimeError(f"sequence index {index} outside count {count}")
    offset, length = struct.unpack_from(">II", raw_sbk, 4 + index * 8)
    return raw_sbk[offset : offset + length]


def iter_midi_events(cseq_to_mid, seq: bytes):
    track_offsets = list(struct.unpack_from(">16I", seq, 0))
    valid = [(track_id, offset) for track_id, offset in enumerate(track_offsets) if offset]
    valid.sort(key=lambda item: item[1])
    events = []
    for index, (track_id, offset) in enumerate(valid):
        end = valid[index + 1][1] if index + 1 < len(valid) else len(seq)
        for tick, sort_key, event in cseq_to_mid.parse_track_to_events(seq, offset, end):
            events.append((tick, sort_key, track_id, event))
    events.sort(key=lambda item: (item[0], item[1], item[2]))
    return events


def collect_pitch_bends(cseq_to_mid, seq: bytes, bank: dict, by_off: dict):
    """Compile BattleShip's channel pitch-bend state into per-channel events.

    The compact sequence player starts channels from the bank's first available
    instrument, copies ALInstrument.bendRange on program change, lets CC20
    override that range, and interprets 0xE0 as a signed 14-bit value centered
    on 8192. The resulting cents value becomes a channel pitch ratio and retunes
    voices that are already sounding, so note-on-only bend snapshots are not
    sufficient for Jungle/Yamabuki/Castle.
    """
    instrument_offsets = bank.get("instArray_offs", [])
    first_instrument = next((off for off in instrument_offsets if off), None)
    initial_range = 200
    if first_instrument:
        initial = by_off.get(first_instrument)
        if initial and initial.get("kind") == "ALInstrument":
            initial_range = int(initial.get("bendRange", initial_range))

    bend_ranges = [initial_range] * 16
    timelines = [[] for _ in range(16)]
    applied = 0
    max_abs_cents = 0

    for event_order, (tick, _sort_key, _track_id, event) in enumerate(
            iter_midi_events(cseq_to_mid, seq)):
        if event[0] != "midi":
            continue
        _, status, d1, d2 = event
        channel = status & 0xF
        kind4 = status & 0xF0

        if kind4 == 0xC0:
            program = int(d1)
            if 0 <= program < len(instrument_offsets):
                inst_off = instrument_offsets[program]
                inst = by_off.get(inst_off) if inst_off else None
                if inst and inst.get("kind") == "ALInstrument":
                    bend_ranges[channel] = int(inst.get("bendRange", 200))
            continue

        if kind4 == 0xB0 and d2 is not None:
            if int(d1) == PITCH_BEND_RANGE_CONTROLLER:
                value = int(d2)
                bend_ranges[channel] = 1200 if value >= 0x79 else value * 10
            continue

        if kind4 != 0xE0 or d2 is None:
            continue

        bend_value = ((int(d2) << 7) | int(d1)) - 8192
        numerator = bend_ranges[channel] * bend_value
        # C's signed integer division truncates toward zero.
        cents = (abs(numerator) // 8192) * (-1 if numerator < 0 else 1)
        timelines[channel].append((event_order, int(tick), cents))
        applied += 1
        max_abs_cents = max(max_abs_cents, abs(cents))

    return timelines, {
        "pitch_bend_events_applied": applied,
        "pitch_bend_max_abs_cents": max_abs_cents,
    }


def collect_notes(cseq_to_mid, seq: bytes, sample_rate: int = OUTPUT_SAMPLE_RATE,
                  bend_timelines=None):
    tempo_us = 500000
    ticks_per_quarter = struct.unpack_from(">I", seq, 64)[0]
    programs = [0] * 16
    volumes = [100] * 16
    # n_env.c's AL_SEQP_PLAY_EVT initializes N_ALCSPlayer.masterVol to 100;
    # controller 21 replaces that sequence-wide value when present.
    master_volume = SEQUENCE_PLAYER_DEFAULT_MASTER_VOLUME
    active = {}
    notes = []

    for event_order, (tick, _sort_key, _track_id, event) in enumerate(
            iter_midi_events(cseq_to_mid, seq)):
        kind = event[0]

        if kind == "tempo":
            tempo_us = int(event[1])
            continue
        if kind == "midi":
            _, status, d1, d2 = event
            channel = status & 0xF
            kind4 = status & 0xF0
            if kind4 == 0xC0:
                programs[channel] = int(d1)
            elif kind4 == 0xB0 and d2 is not None:
                controller = int(d1)
                if controller == 7:
                    volumes[channel] = int(d2)
                elif controller == MASTER_VOLUME_CONTROLLER:
                    # BattleShip's compressed-sequence player treats CC21 as
                    # sequence master volume. The stage BGMs set it before
                    # their first note (Pupupu 127, Inishie/Hurry 99, Yoster
                    # 86), so baking the active value into each note is
                    # equivalent for those source sequences and costs nothing
                    # on the DS.
                    master_volume = int(d2)
            continue

        if kind == "note_on":
            _, channel, midi_note, velocity = event
            key = (channel, int(midi_note))
            stack = active.setdefault(key, [])
            stack.append(
                {
                    "tick": tick,
                    "channel": channel,
                    "note": int(midi_note),
                    "velocity": int(velocity),
                    "program": programs[channel],
                    "volume": volumes[channel],
                    "master_volume": master_volume,
                    "event_order": event_order,
                }
            )
            continue

        if kind == "note_off":
            _, channel, midi_note, _velocity = event
            key = (channel, int(midi_note))
            stack = active.get(key)
            if not stack:
                continue
            start = stack.pop(0)
            start["end_tick"] = tick
            notes.append(start)

    def tick_to_sample(tick: int) -> int:
        seconds = (tick * tempo_us) / (ticks_per_quarter * 1000000.0)
        return int(seconds * sample_rate)

    for note in notes:
        note["start"] = tick_to_sample(note["tick"])
        note["end"] = max(note["start"] + 80, tick_to_sample(note["end_tick"]))
        note["bend_cents"] = 0
        note["bend_events"] = ()
        if bend_timelines is not None:
            initial_cents = 0
            future = []
            for bend_order, bend_tick, bend_cents in bend_timelines[note["channel"]]:
                if bend_order < note["event_order"]:
                    initial_cents = bend_cents
                    continue
                bend_sample = tick_to_sample(bend_tick)
                future.append((max(0, bend_sample - note["start"]), bend_cents))
            note["bend_cents"] = initial_cents
            note["bend_events"] = tuple(future)
        del note["event_order"]

    return notes, tempo_us


def collect_loop_metadata(cseq_to_mid, seq: bytes, tempo_us: int, notes: list,
                          sample_rate: int = OUTPUT_SAMPLE_RATE):
    """Return a shared loop interval for a rendered mix.

    BattleShip's CSEQ stores an independent AL_CMIDI_LOOPEND_CODE loop on
    EACH channel (n_env.c's __n_alCSeqGetTrackEvent: loop_ct=0xFF means
    "loop forever", jumping back by the event's own encoded byte offset).
    The previous version of this function assumed every channel's loop
    covers the same duration, just entered "a few ticks apart", and took
    an unconditional max() of every channel's raw loopstart/loopend tick
    regardless of that channel's own loop length. That assumption holds
    for Pupupu/Results/Battle Select (every channel agrees on one shared
    period) but broke for Mode Select: P2-1L bug (b1) found 3 channels
    sharing a 7680-tick period and 4 near-silent outlier channels with
    four different, much longer periods (12018/19887/39876/39321 ticks).
    The outlier with the longest period dominated max(), producing a
    loop_start ~14 seconds from the tune's own repeating phrase, stitched
    to a stream end sharing no musical relationship with it -- measured
    offline as a hard cut from a loud, unrelated moment in the piece
    straight into dead silence every single loop. That is exactly "ends,
    then begins again" instead of a continuous loop.

    Fix: take the loop PERIOD (loopend_tick - loopstart_tick) the most
    channels agree on (the mode; ties broken toward the shortest period),
    and anchor the loop phase on the latest-entering agreeing channel's
    own loopstart (preserving the previous function's "keep everyone's
    complete intro" intent, and empirically -- offline PCM seam RMS --
    the best of the candidate anchors: it lands the loop restart on live
    signal with 0 ms of dead air, where the other agreeing channels' own
    anchors each left ~600 ms of silence before their content resumed).
    Then walk forward in whole periods from that anchor until covering the
    mix's actual last rendered note, so the loop is always long enough to
    include every note and never longer than one extra period past it.

    When every channel already agrees (Pupupu, Results, Battle Select)
    this is provably the same loop_start as the old max()/max() reading:
    with one shared period P, max(end_i) == max(start_i) + P for every
    channel i, which is exactly what walking one period from
    max(start_i) computes. The remaining difference for those tracks is
    letting the caller trim the flat trailing render pad down to this
    same period boundary instead of a further arbitrary second of
    silence baked inside the loop (loop_end_byte, below) -- P2-1L bug
    (b1) also measured that flat pad as true digital silence
    (offline RMS 0.0) sitting right before the wrap on Battle Select's
    short ~13 s loop, audible as the same "ends, then begins again"
    defect at smaller scale.
    """

    per_track_starts: dict = {}
    per_track_ends: dict = {}
    ticks_per_quarter = struct.unpack_from(">I", seq, 64)[0]

    for tick, _sort_key, track_id, event in iter_midi_events(cseq_to_mid, seq):
        if event[0] != "marker":
            continue
        if event[1] == "loopstart":
            per_track_starts.setdefault(track_id, []).append(int(tick))
        elif event[1].startswith("loopend"):
            per_track_ends.setdefault(track_id, []).append(int(tick))

    if not per_track_starts or not per_track_ends:
        return {
            "looping": False,
            "loop_start_tick": 0,
            "loop_end_tick": 0,
            "loop_start_tick_min": 0,
            "loop_end_tick_min": 0,
            "loop_start_byte": 0,
            "loop_end_byte": 0,
        }

    starts = [tick for ticks in per_track_starts.values() for tick in ticks]
    ends = [tick for ticks in per_track_ends.values() for tick in ticks]

    # One (loopstart, loopend) pair per channel is what every BattleShip
    # menu/results/battle BGM track rendered so far actually has. A track
    # with more than one of either isn't a "shared master loop" case this
    # per-channel-period grouping covers, so fall back to the flat
    # max()/max() reading rather than guessing which pair belongs together.
    track_periods = {
        track_id: (starts_[0], ends_[0] - starts_[0])
        for track_id, starts_ in per_track_starts.items()
        if len(starts_) == 1 and len(ends_ := per_track_ends.get(track_id, [])) == 1
    }

    def tick_to_sample_duration(ticks: int) -> int:
        return (ticks * tempo_us * sample_rate) // (ticks_per_quarter * 1000000)

    if track_periods:
        period_counts: dict = {}
        for _start, period in track_periods.values():
            period_counts[period] = period_counts.get(period, 0) + 1
        best_count = max(period_counts.values())
        # Deterministic tie-break (never hit by the 4 tracks rendered so
        # far -- each has a clear single majority period): the shortest
        # agreeing period.
        shared_period = min(p for p, c in period_counts.items() if c == best_count)
        agreeing_starts = [s for s, p in track_periods.values() if p == shared_period]
        base_start_tick = max(agreeing_starts)
        period_samples = tick_to_sample_duration(shared_period)
    else:
        base_start_tick = max(starts)
        period_samples = tick_to_sample_duration(max(ends) - base_start_tick)

    if period_samples > 0:
        base_start_sample = tick_to_sample_duration(base_start_tick)
        last_note_sample = max((n["end"] for n in notes), default=base_start_sample)
        remaining = last_note_sample - base_start_sample
        periods_needed = max(1, -(-remaining // period_samples))  # ceil, at least 1
        loop_end_sample = base_start_sample + periods_needed * period_samples
        loop_start_sample = loop_end_sample - period_samples
    else:
        # Degenerate CSEQ (loopend at/before loopstart) -- keep the old
        # flat reading rather than divide by zero.
        loop_start_sample = tick_to_sample_duration(max(starts))
        loop_end_sample = tick_to_sample_duration(max(ends))

    return {
        "looping": True,
        "loop_start_tick": base_start_tick,
        "loop_end_tick": max(ends),
        "loop_start_tick_min": min(starts),
        "loop_end_tick_min": min(ends),
        "loop_start_byte": loop_start_sample * 2,
        "loop_end_byte": loop_end_sample * 2,
    }


def unroll_channel_loops(cseq_to_mid, seq: bytes, notes: list, loop: dict,
                          tempo_us: int,
                          sample_rate: int = OUTPUT_SAMPLE_RATE) -> list:
    """P2-1L (b2 round 2). Replicate each channel's looped notes out to the
    mix's loop_end, the way the real engine plays them.

    n_env.c loops EVERY channel independently and forever (loop_ct=0xFF):
    when a channel reaches its own AL_CMIDI_LOOPEND_CODE it jumps back and
    keeps playing. `collect_notes` walks the converted event stream ONCE, so
    each channel's notes exist only up to its own loopend tick -- and
    `collect_loop_metadata` anchors the mix region on the LATEST-entering
    agreeing channel, which guarantees every earlier-entering channel runs
    dry inside the region by exactly its entry stagger. Measured on Battle
    Select (owner recording, RMS/100ms): channels 0-4 fall silent 1.51 s
    before every wrap -- the recurring "quiet patch" -- while the N64
    reference holds full energy there, because on hardware those channels
    have wrapped and are replaying their own material under channel 14's
    longer tail.

    For each channel with one (loopstart, loopend) marker pair, every note
    with loopstart <= tick < loopend is replicated at +k*(loopend-loopstart)
    for k = 1, 2, ... while the replica starts before the mix's loop_end.
    Channels whose period matches the shared mix period land phase-aligned
    at the wrap (region length is a whole multiple of their period, whatever
    their entry stagger). A channel with a different period (Mode Select's
    near-silent outliers) wraps mid-phrase -- a per-channel phase jump on a
    quiet ornament, priced against a 1.5 s hole in the whole band."""
    if not loop.get("looping"):
        return notes

    ticks_per_quarter = struct.unpack_from(">I", seq, 64)[0]
    per_track = {}
    for tick, _sort_key, track_id, event in iter_midi_events(cseq_to_mid, seq):
        if event[0] != "marker":
            continue
        if event[1] == "loopstart":
            per_track.setdefault(track_id, {})["start"] = int(tick)
        elif event[1].startswith("loopend"):
            per_track.setdefault(track_id, {})["end"] = int(tick)

    def tick_to_sample(tick: int) -> int:
        seconds = (tick * tempo_us) / (ticks_per_quarter * 1000000.0)
        return int(seconds * sample_rate)

    loop_end_sample = loop["loop_end_byte"] // 2
    out = list(notes)
    replicas = 0
    for track_id, marks in per_track.items():
        if "start" not in marks or "end" not in marks:
            continue
        period = marks["end"] - marks["start"]
        if period <= 0:
            continue
        body = [n for n in notes
                if n["channel"] == track_id
                and marks["start"] <= n["tick"] < marks["end"]]
        if not body:
            continue
        k = 1
        while True:
            shift = k * period
            first_start = tick_to_sample(body[0]["tick"] + shift)
            if first_start >= loop_end_sample:
                break
            added = False
            for n in body:
                tick = n["tick"] + shift
                start = tick_to_sample(tick)
                if start >= loop_end_sample:
                    continue
                replica = dict(n)
                replica["tick"] = tick
                replica["end_tick"] = n["end_tick"] + shift
                replica["start"] = start
                replica["end"] = max(start + 80,
                                      tick_to_sample(n["end_tick"] + shift))
                out.append(replica)
                added = True
                replicas += 1
            if not added:
                break
            k += 1
    if replicas:
        print(f"unrolled {replicas} replica notes across channel loops")
    return out


def resolve_instrument(bank, program: int):
    offsets = bank.get("instArray_offs", [])
    if 0 <= program < len(offsets) and offsets[program] != 0:
        return offsets[program]
    percussion = bank.get("percussion_off")
    return percussion if percussion else None


def select_sound(decode_ctl, by_off, bank, program: int, note: int, velocity: int):
    inst_off = resolve_instrument(bank, program)
    if not inst_off:
        return None
    inst = by_off.get(inst_off)
    if not inst or inst.get("kind") != "ALInstrument":
        return None

    fallback = None
    for sound_off in inst.get("soundArray_offs", []):
        sound = by_off.get(sound_off)
        if not sound:
            continue
        keymap = by_off.get(sound.get("keyMap_off"))
        if fallback is None:
            fallback = sound
        if not keymap:
            continue
        key_ok = keymap["keyMin"] <= note <= keymap["keyMax"]
        vel_ok = keymap["velocityMin"] <= velocity <= keymap["velocityMax"]
        if key_ok and vel_ok:
            return sound
    return fallback


def decode_wave(audio_codec, by_off, tbl: bytes, wave_off: int):
    """Return (pcm, source_rate, loop_start, loop_end). loop_start/loop_end
    are None unless the ALWaveTable carries an ALADPCMloop with a nonzero
    count -- n_env.c's n_alLoadParam reads exactly this struct (count==0
    means "no loop" even when the pointer is non-null; -1/0xFFFFFFFF means
    loop forever, the only count this bank's own data ever uses) and
    n_alAdpcmPull loops [start, end) in the source ADPCM stream for as
    long as the voice is held. P2-1L bug (b2) root cause: this function
    used to decode the table once and hand back nothing but the straight
    PCM, so any note held longer than one straight decode pass (a real,
    authored case -- see render()'s sustain notes) ran out of source
    samples and rendered silence for the remainder instead of looping.
    """
    wave = by_off.get(wave_off)
    if not wave or wave.get("kind") != "ALWaveTable":
        return [], SOURCE_BANK_SAMPLE_RATE, None, None
    if wave.get("type") != 0:
        return [], SOURCE_BANK_SAMPLE_RATE, None, None

    book = by_off.get(wave.get("book_off"))
    if not book:
        return [], SOURCE_BANK_SAMPLE_RATE, None, None
    encoded = tbl[wave["base"] : wave["base"] + wave["length"]]
    pcm = audio_codec.adpcm_decode(
        encoded,
        book["entries"],
        book["order"],
        book["npredictors"],
        initial_state=[0] * book["order"],
    )
    loop_start = loop_end = None
    loop = by_off.get(wave.get("loop_off")) if wave.get("loop_off") else None
    if loop and loop.get("kind") == "ALADPCMloop" and loop.get("count", 0) != 0:
        if 0 <= loop["start"] < loop["end"] <= len(pcm):
            loop_start, loop_end = loop["start"], loop["end"]
    return pcm, SOURCE_BANK_SAMPLE_RATE, loop_start, loop_end


def envelope_level(t: int, attack_samples: int, decay_samples: int,
                    attack_level: float, decay_level: float) -> float:
    """ALEnvelope's held (pre-release) shape at t samples after note-on:
    linear ramp 0 -> attackVolume over attackTime, then attackVolume ->
    decayVolume over decayTime, then held at decayVolume (the sustain
    level) until note-off. Matches ALEnvelope's own two-segment model
    (libaudio.h: attackTime/decayTime/releaseTime + attackVolume/
    decayVolume) -- there is no separate "sustain time" field because
    decayVolume IS the sustain level, held until key-off."""
    if t < attack_samples:
        return attack_level * (t / attack_samples) if attack_samples > 0 else attack_level
    t2 = t - attack_samples
    if t2 < decay_samples:
        frac = t2 / decay_samples if decay_samples > 0 else 1.0
        return attack_level + (decay_level - attack_level) * frac
    return decay_level


def source_voice_pitch_ratio(note: dict, key_base: int, detune_cents: int,
                             bend_cents: int) -> float:
    """Match the source voice pitch product and its resampler ratio clamp."""
    base_ratio = math.pow(
        2.0,
        (note["note"] - key_base + detune_cents / 100.0) / 12.0,
    )
    if bend_cents:
        base_ratio *= math.pow(2.0, bend_cents / 1200.0)
    return min(SOURCE_MAX_PITCH_RATIO, base_ratio)


def render(notes, decode_ctl, audio_codec, by_off, bank, tbl: bytes, gain: float,
           sample_rate: int = OUTPUT_SAMPLE_RATE):
    """P2-1L bug (b2). Two fixes against the real engine's own semantics
    (decomp/BattleShip-main/decomp/src/libultra/n_audio/n_env.c):

    1. Wavetable sustain looping (decode_wave, above) -- a held note whose
       gate exceeds one straight decode pass now loops [loop_start,
       loop_end) in the already-decoded PCM instead of running out of
       source and rendering silence. That silence, at the position of
       this song's own longest sustained note, was the "quiet patch" the
       owner's DS.wav re-test located inside the loop body.

    2. Per-instrument ADSR envelope (envelope_level, above) instead of a
       flat "+2200 samples, 700-sample linear fade" tail on every note
       regardless of its authored release. This bank's ALEnvelope data
       (read by select_sound's ALSound -> env_off, unused before this
       fix) gives most of Battle Select's instruments a 25-30 ms release
       -- a fraction of the old fixed ~100 ms tail -- which is the
       measured mechanism behind the owner's "smeared/legato vs the N64's
       staccato" comparison (cross-correlated onset/offset envelopes
       against the owner's n64.wav reference): our notes rang roughly
       3x longer past their own note-off than the source instrument was
       ever authored to.
    """
    if not notes:
        raise RuntimeError("sequence did not produce any notes")

    total_samples = max(note["end"] for note in notes) + sample_rate
    mix = [0.0] * total_samples
    wave_cache = {}
    # Fallback for the (unseen in this bank, but not guaranteed absent)
    # case of a sound with no envelope reference: instant attack, no
    # decay stage, and a release matching the old fixed tail so a missing
    # envelope never regresses into a dropped or clicking note.
    DEFAULT_RELEASE_SAMPLES = 2200

    for note in notes:
        sound = select_sound(
            decode_ctl,
            by_off,
            bank,
            note["program"],
            note["note"],
            note["velocity"],
        )
        if not sound:
            continue
        keymap = by_off.get(sound.get("keyMap_off"), {})
        wave_off = sound.get("wavetable_off")
        if wave_off not in wave_cache:
            wave_cache[wave_off] = decode_wave(audio_codec, by_off, tbl, wave_off)
        pcm, source_rate, loop_start, loop_end = wave_cache[wave_off]
        if not pcm:
            continue

        key_base = int(keymap.get("keyBase", note["note"]))
        detune_cents = int(keymap.get("detune", 0))
        bend_cents = int(note.get("bend_cents", 0))
        ratio = source_voice_pitch_ratio(
            note, key_base, detune_cents, bend_cents)
        source_step = (source_rate / sample_rate) * ratio
        scale = (
            gain
            * (note["velocity"] / 127.0)
            * (note["volume"] / 127.0)
            * (sound.get("sampleVolume", 127) / 127.0)
        )
        # Preserve the exact arithmetic of the accepted Dream Land stream:
        # multiplying by 1.0 would be mathematically harmless, but skipping it
        # when CC21 is 127 guarantees the legacy path performs the old scale
        # operations byte-for-byte.
        master_volume = int(note.get("master_volume", 127))
        if master_volume != 127:
            scale *= master_volume / 127.0

        env = by_off.get(sound.get("env_off"))
        if env:
            attack_samples = round(env["attackTime"] * sample_rate / 1_000_000)
            decay_samples = round(env["decayTime"] * sample_rate / 1_000_000)
            release_samples = round(env["releaseTime"] * sample_rate / 1_000_000)
            attack_level = env["attackVolume"] / 127.0
            decay_level = env["decayVolume"] / 127.0
        else:
            attack_samples = decay_samples = 0
            release_samples = DEFAULT_RELEASE_SAMPLES
            attack_level = decay_level = 1.0

        start = note["start"]
        requested = max(1, note["end"] - note["start"])
        looping = loop_start is not None
        raw_len_limit = requested + release_samples
        if not looping:
            raw_len_limit = min(raw_len_limit, int(len(pcm) / max(source_step, 0.001)))
        max_out = max(1, raw_len_limit)
        level_at_off = envelope_level(requested, attack_samples, decay_samples,
                                       attack_level, decay_level)
        source_pos = 0.0
        bend_events = note.get("bend_events", ())
        bend_event_index = 0

        for out_i in range(max_out):
            while (bend_event_index < len(bend_events)
                   and bend_events[bend_event_index][0] <= out_i):
                bend_cents = int(bend_events[bend_event_index][1])
                ratio = source_voice_pitch_ratio(
                    note, key_base, detune_cents, bend_cents)
                source_step = (source_rate / sample_rate) * ratio
                bend_event_index += 1
            src_i = int(source_pos)
            src_j = src_i + 1
            if looping and src_i >= loop_end:
                span = loop_end - loop_start
                src_i = loop_start + (src_i - loop_start) % span
                src_j = src_i + 1
                if src_j >= loop_end:
                    src_j = loop_start
            if src_j >= len(pcm):
                break
            sample = pcm[src_i] * (1.0 - (source_pos - int(source_pos))) + pcm[src_j] * (source_pos - int(source_pos))
            if out_i < requested:
                env_level = envelope_level(out_i, attack_samples, decay_samples,
                                            attack_level, decay_level)
            elif release_samples > 0:
                env_level = level_at_off * max(0.0, 1.0 - (out_i - requested) / release_samples)
            else:
                env_level = 0.0
            dest = start + out_i
            if dest >= len(mix):
                break
            mix[dest] += (sample / 32768.0) * scale * env_level
            source_pos += source_step

    pcm16 = bytearray()
    for sample in mix:
        value = int(max(-1.0, min(1.0, sample)) * 32767.0)
        pcm16 += struct.pack("<h", value)
    return bytes(pcm16)


def _sinc(value: float) -> float:
    if abs(value) < 1.0e-12:
        return 1.0
    angle = math.pi * value
    return math.sin(angle) / angle


def bandlimited_resample_pcm16(pcm: bytes, source_rate: int, target_rate: int,
                               target_samples: int) -> bytes:
    """Resample one completed mono PCM16 mix with a windowed-sinc low-pass.

    The old renderer changed sample rate independently inside every voice with
    linear interpolation. For a downsample that aliases each voice's content
    above the new Nyquist limit before the voices are even summed. This path
    instead filters the completed source-rate mix and then samples it at the DS
    stream rate. A 32-tap Lanczos-windowed sinc is fully offline, deterministic,
    and carries no ROM/RAM/CPU state into the DS runtime.
    """
    if source_rate <= 0 or target_rate <= 0 or target_samples < 0:
        raise ValueError("invalid resampler rate or target length")
    if len(pcm) & 1:
        raise ValueError("PCM16 input must contain whole samples")
    if source_rate == target_rate:
        wanted = target_samples * 2
        if wanted <= len(pcm):
            return pcm[:wanted]
        return pcm + bytes(wanted - len(pcm))

    source = array("h")
    source.frombytes(pcm)
    if sys.byteorder != "little":
        source.byteswap()
    if not source:
        return bytes(target_samples * 2)

    divisor = math.gcd(source_rate, target_rate)
    source_step = source_rate // divisor
    target_step = target_rate // divisor
    downsample_ratio = min(1.0, target_rate / source_rate)
    # Leave a small transition band below the 22.05 kHz Nyquist edge so the
    # finite window attenuates aliases instead of merely moving the cutoff to
    # the exact edge. 16 samples on each side gives a 32-tap kernel.
    cutoff = downsample_ratio * 0.94
    radius = 16
    kernels = []
    for phase in range(target_step):
        frac = phase / target_step
        weights = []
        total = 0.0
        for offset in range(-radius + 1, radius + 1):
            delta = frac - offset
            if abs(delta) >= radius:
                weight = 0.0
            else:
                weight = cutoff * _sinc(cutoff * delta) * _sinc(delta / radius)
            weights.append((offset, weight))
            total += weight
        if abs(total) < 1.0e-12:
            raise RuntimeError("band-limit kernel has zero DC gain")
        kernels.append(tuple((offset, weight / total) for offset, weight in weights))

    output = array("h")
    append = output.append
    source_len = len(source)
    for out_index in range(target_samples):
        numerator = out_index * source_step
        center = numerator // target_step
        phase = numerator % target_step
        value = 0.0
        for offset, weight in kernels[phase]:
            source_index = center + offset
            if source_index < 0:
                source_index = 0
            elif source_index >= source_len:
                source_index = source_len - 1
            value += source[source_index] * weight
        quantized = int(round(value))
        if quantized < -32768:
            quantized = -32768
        elif quantized > 32767:
            quantized = 32767
        append(quantized)

    if sys.byteorder != "little":
        output.byteswap()
    return output.tobytes()


def initial_ima_index(samples: list[int]) -> int:
    if len(samples) < 2:
        return 0
    target = max(7, abs(samples[1] - samples[0]))
    return min(range(len(IMA_STEP_TABLE)),
               key=lambda index: abs(IMA_STEP_TABLE[index] - target))


def ima_encode_sample(sample: int, predictor: int,
                      index: int) -> tuple[int, int, int]:
    step = IMA_STEP_TABLE[index]
    delta = sample - predictor
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
    predictor += -diff if code & 8 else diff
    predictor = max(-32768, min(32767, predictor))
    index = max(0, min(88, index + IMA_INDEX_TABLE[code]))
    return code, predictor, index


def build_ima_packets(pcm: bytes, loop_start_byte: int,
                      looping: bool) -> tuple[bytes, dict]:
    if len(pcm) == 0 or len(pcm) & 1 or loop_start_byte & 1:
        raise ValueError("PCM and loop offsets must contain whole samples")
    samples = list(struct.unpack(f"<{len(pcm) // 2}h", pcm))
    loop_start_sample = loop_start_byte // 2
    if looping and not 0 < loop_start_sample < len(samples):
        raise ValueError("looping track has an invalid loop start")

    boundaries: list[tuple[int, int]] = []
    split = loop_start_sample if looping else len(samples)
    for start, end in ((0, split), (split, len(samples))):
        for offset in range(start, end, BGM_IMA_PACKET_SAMPLES):
            boundaries.append((offset, min(end, offset + BGM_IMA_PACKET_SAMPLES)))
        if not looping:
            break
    loop_packet_index = (
        sum(1 for start, _end in boundaries if start < split)
        if looping else 0xFFFFFFFF
    )

    predictor = samples[0]
    index = initial_ima_index(samples)
    records: list[bytes] = []
    squared_error = 0
    source_energy = 0
    max_error = 0
    for start, end in boundaries:
        packet_predictor = predictor
        packet_index = index
        codes = []
        for sample in samples[start:end]:
            code, predictor, index = ima_encode_sample(
                sample, predictor, index)
            codes.append(code)
            error = sample - predictor
            squared_error += error * error
            source_energy += sample * sample
            max_error = max(max_error, abs(error))
        while len(codes) & 7:
            codes.append(0)
        payload = bytearray(struct.pack(
            "<hBB", packet_predictor, packet_index, 0))
        for pos in range(0, len(codes), 2):
            payload.append(codes[pos] | (codes[pos + 1] << 4))
        if len(payload) & 3:
            raise AssertionError("DS IMA packet is not word aligned")
        records.append(
            BGM_IMA_PACKET.pack(end - start, len(payload)) + payload)

    loop_record_offset = 0
    if looping:
        loop_record_offset = BGM_IMA_HEADER.size + sum(
            len(record) for record in records[:loop_packet_index])
    flags = 1 if looping else 0
    header = BGM_IMA_HEADER.pack(
        BGM_IMA_MAGIC, BGM_IMA_VERSION, BGM_IMA_HEADER.size,
        OUTPUT_SAMPLE_RATE, len(samples),
        loop_start_sample if looping else 0xFFFFFFFF,
        BGM_IMA_PACKET_SAMPLES, len(records), loop_packet_index,
        loop_record_offset, flags)
    encoded = header + b"".join(records)
    rms_error = math.sqrt(squared_error / len(samples))
    snr_db = (10.0 * math.log10(source_energy / squared_error)
              if squared_error else float("inf"))
    return encoded, {
        "container_magic": BGM_IMA_MAGIC.decode("ascii"),
        "container_version": BGM_IMA_VERSION,
        "header_bytes": BGM_IMA_HEADER.size,
        "packet_samples": BGM_IMA_PACKET_SAMPLES,
        "packet_count": len(records),
        "loop_packet_index": loop_packet_index,
        "loop_record_offset": loop_record_offset,
        "ima_rms_error": rms_error,
        "ima_snr_db": snr_db,
        "ima_max_error": max_error,
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--repo", type=Path, default=_paths.REPO_ROOT)
    parser.add_argument("--sequence-index", type=int, default=SEQ_INDEX_PUPUPU)
    parser.add_argument(
        "--output",
        type=Path,
        default=Path("assets/audio/bgm_pupupu_ima.bin"),
    )
    parser.add_argument(
        "--format", choices=("ima-packets", "pcm16"),
        default="ima-packets")
    parser.add_argument("--gain", type=float, default=DEFAULT_GAIN)
    args = parser.parse_args()

    repo = args.repo.resolve()
    tools = repo / "decomp/BattleShip-main/decomp/tools"
    cseq_to_mid = load_module(tools / "cseq_to_mid.py", "cseq_to_mid")
    decode_ctl = load_module(tools / "decode_ctl.py", "decode_ctl")
    audio_codec = load_module(tools / "audio_codec.py", "audio_codec")

    audio_root = repo / "decomp/BattleShip-main/BattleShip_o2r/audio"
    sbk = read_o2r_payload(audio_root / "S1_music_sbk")
    ctl = read_o2r_payload(audio_root / BGM_SEQUENCE_BANK_CTL)
    tbl = read_o2r_payload(audio_root / BGM_SEQUENCE_BANK_TBL)

    seq = read_seq(sbk, args.sequence_index)
    legacy_direct_resample = args.sequence_index in LEGACY_DIRECT_RESAMPLE_SEQUENCES
    mix_sample_rate = (
        OUTPUT_SAMPLE_RATE if legacy_direct_resample else SOURCE_BANK_SAMPLE_RATE)
    decoded = decode_ctl.walk(ctl)
    by_off = {item["offset"]: item for item in decoded}
    bank = next(item for item in decoded if item.get("kind") == "ALBank")
    bend_timelines, bend_metadata = collect_pitch_bends(
        cseq_to_mid, seq, bank, by_off)
    notes, tempo_us = collect_notes(
        cseq_to_mid, seq, mix_sample_rate, bend_timelines)
    master_volume_values = sorted({
        int(note.get("master_volume", SEQUENCE_PLAYER_DEFAULT_MASTER_VOLUME))
        for note in notes
    })
    loop = collect_loop_metadata(
        cseq_to_mid, seq, tempo_us, notes, mix_sample_rate)
    notes = unroll_channel_loops(
        cseq_to_mid, seq, notes, loop, tempo_us, mix_sample_rate)
    pcm = render(
        notes, decode_ctl, audio_codec, by_off, bank, tbl, args.gain,
        mix_sample_rate)

    if loop["looping"]:
        # The loop must wrap at loop_end_byte (a period boundary of the
        # majority-agreeing channels, computed above), never at whatever
        # the note-driven render happened to extend to -- P2-1L bug (b1):
        # wrapping at the old flat "last note + 1 second" pad instead
        # measured as true digital silence sitting inside every loop
        # (offline RMS 0.0), audible as "ends, then begins again". Pad
        # with silence if the period boundary falls past the rendered
        # notes (nothing but silence lives there anyway); truncate if it
        # falls inside the old flat pad.
        target_bytes = loop["loop_end_byte"]
        if target_bytes > len(pcm):
            pcm = pcm + bytes(target_bytes - len(pcm))
        else:
            pcm = pcm[:target_bytes]

    if not legacy_direct_resample:
        # Compute the target-rate timing with the same integer conversions the
        # legacy renderer used. The mix above stays at 32 kHz; only this final,
        # completed stream crosses the 22.05 kHz boundary.
        output_notes, _output_tempo_us = collect_notes(
            cseq_to_mid, seq, OUTPUT_SAMPLE_RATE, bend_timelines)
        output_loop = collect_loop_metadata(
            cseq_to_mid, seq, tempo_us, output_notes, OUTPUT_SAMPLE_RATE)
        if output_loop["looping"]:
            target_samples = output_loop["loop_end_byte"] // 2
        else:
            target_samples = (
                max(note["end"] for note in output_notes) + OUTPUT_SAMPLE_RATE)
        pcm = bandlimited_resample_pcm16(
            pcm, mix_sample_rate, OUTPUT_SAMPLE_RATE, target_samples)
        loop = output_loop

    output = (repo / args.output).resolve() if not args.output.is_absolute() else args.output
    output.parent.mkdir(parents=True, exist_ok=True)
    source_pcm_digest = hashlib.sha256(pcm).hexdigest()
    if args.format == "ima-packets":
        payload, format_metadata = build_ima_packets(
            pcm, loop["loop_start_byte"], loop["looping"])
        format_name = "Nintendo DS IMA-ADPCM packet stream"
    else:
        payload = pcm
        format_metadata = {}
        format_name = "signed PCM16LE mono raw"
    output.write_bytes(payload)
    digest = hashlib.sha256(payload).hexdigest()

    metadata = {
        "source": (
            "BattleShip_o2r/audio/S1_music_sbk sequence "
            f"{args.sequence_index} + {BGM_SEQUENCE_BANK_SOURCE}"
        ),
        "tool": "scripts/sfx/bgm/render-audio-bgm.py",
        "sample_rate": OUTPUT_SAMPLE_RATE,
        "mix_sample_rate": mix_sample_rate,
        "resample_method": (
            "legacy per-voice linear 32k-to-22.05k (Dream Land regression guard)"
            if legacy_direct_resample else
            "completed 32k mix -> 22.05k 32-tap Lanczos-windowed sinc low-pass"
        ),
        "sequence_bank_binding": BGM_SEQUENCE_BANK_BINDING,
        "master_volume_controller": MASTER_VOLUME_CONTROLLER,
        "master_volume_values": master_volume_values,
        "pitch_bend_range_controller": PITCH_BEND_RANGE_CONTROLLER,
        **bend_metadata,
        "format": format_name,
        "bytes": len(payload),
        "sha256": digest,
        "source_pcm_bytes": len(pcm),
        "source_pcm_sha256": source_pcm_digest,
        "sequence_index": args.sequence_index,
        "note_count": len(notes),
        "tempo_us_per_quarter": tempo_us,
        "gain": args.gain,
        **format_metadata,
        **loop,
    }
    output.with_suffix(".json").write_text(
        json.dumps(metadata, indent=2) + "\n",
        encoding="utf-8",
        newline="\n",
    )

    print(f"rendered {output}")
    print(f"bytes={len(payload)} sample_rate={OUTPUT_SAMPLE_RATE} sha256={digest}")
    print(
        f"bend_events_applied={bend_metadata['pitch_bend_events_applied']} "
        f"bend_max_abs_cents={bend_metadata['pitch_bend_max_abs_cents']}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
