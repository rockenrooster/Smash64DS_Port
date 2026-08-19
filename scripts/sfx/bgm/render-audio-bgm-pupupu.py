#!/usr/bin/env python3
"""Render a BattleShip BGM sequence to a DS-friendly audio stream.

This is intentionally a small compatibility renderer for the port's audible
BGM gates. It derives each stream from the original O2R sequence/bank files
and does not use hand-authored notes or third-party audio.
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
import sys as _sys
from pathlib import Path as _Path

_scripts_root = _Path(__file__).resolve().parent
while _scripts_root.name != "scripts":
    _scripts_root = _scripts_root.parent
if str(_scripts_root) not in _sys.path:
    _sys.path.insert(0, str(_scripts_root))
import _paths  # noqa: E402  -- puts every scripts/ area folder on sys.path



SEQ_INDEX_PUPUPU = 0
OUTPUT_SAMPLE_RATE = 22050
DEFAULT_GAIN = 0.22
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


def collect_notes(cseq_to_mid, seq: bytes):
    tempo_us = 500000
    ticks_per_quarter = struct.unpack_from(">I", seq, 64)[0]
    programs = [0] * 16
    volumes = [100] * 16
    active = {}
    notes = []

    for tick, _sort_key, _track_id, event in iter_midi_events(cseq_to_mid, seq):
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
            elif kind4 == 0xB0 and int(d1) == 7 and d2 is not None:
                volumes[channel] = int(d2)
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
        return int(seconds * OUTPUT_SAMPLE_RATE)

    for note in notes:
        note["start"] = tick_to_sample(note["tick"])
        note["end"] = max(note["start"] + 80, tick_to_sample(note["end_tick"]))

    return notes, tempo_us


def collect_loop_metadata(cseq_to_mid, seq: bytes, tempo_us: int, notes: list):
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
        return (ticks * tempo_us * OUTPUT_SAMPLE_RATE) // (ticks_per_quarter * 1000000)

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
    wave = by_off.get(wave_off)
    if not wave or wave.get("kind") != "ALWaveTable":
        return [], 32000
    if wave.get("type") != 0:
        return [], 32000

    book = by_off.get(wave.get("book_off"))
    if not book:
        return [], 32000
    encoded = tbl[wave["base"] : wave["base"] + wave["length"]]
    pcm = audio_codec.adpcm_decode(
        encoded,
        book["entries"],
        book["order"],
        book["npredictors"],
        initial_state=[0] * book["order"],
    )
    return pcm, 32000


def render(notes, decode_ctl, audio_codec, by_off, bank, tbl: bytes, gain: float):
    if not notes:
        raise RuntimeError("sequence did not produce any notes")

    total_samples = max(note["end"] for note in notes) + OUTPUT_SAMPLE_RATE
    mix = [0.0] * total_samples
    wave_cache = {}

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
        pcm, source_rate = wave_cache[wave_off]
        if not pcm:
            continue

        key_base = int(keymap.get("keyBase", note["note"]))
        detune_cents = int(keymap.get("detune", 0))
        ratio = math.pow(2.0, (note["note"] - key_base + detune_cents / 100.0) / 12.0)
        source_step = (source_rate / OUTPUT_SAMPLE_RATE) * ratio
        scale = (
            gain
            * (note["velocity"] / 127.0)
            * (note["volume"] / 127.0)
            * (sound.get("sampleVolume", 127) / 127.0)
        )
        start = note["start"]
        requested = max(1, note["end"] - note["start"])
        max_out = min(requested + 2200, int(len(pcm) / max(source_step, 0.001)))
        fade_start = max(0, max_out - 700)
        source_pos = 0.0

        for out_i in range(max_out):
            src_i = int(source_pos)
            frac = source_pos - src_i
            if src_i + 1 >= len(pcm):
                break
            sample = pcm[src_i] * (1.0 - frac) + pcm[src_i + 1] * frac
            env = 1.0
            if out_i >= fade_start:
                env = max(0.0, (max_out - out_i) / max(1, max_out - fade_start))
            dest = start + out_i
            if dest >= len(mix):
                break
            mix[dest] += (sample / 32768.0) * scale * env
            source_pos += source_step

    pcm16 = bytearray()
    for sample in mix:
        value = int(max(-1.0, min(1.0, sample)) * 32767.0)
        pcm16 += struct.pack("<h", value)
    return bytes(pcm16)


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
    ctl = read_o2r_payload(audio_root / "B1_sounds1_ctl")
    tbl = read_o2r_payload(audio_root / "B1_sounds1_tbl")

    seq = read_seq(sbk, args.sequence_index)
    notes, tempo_us = collect_notes(cseq_to_mid, seq)
    loop = collect_loop_metadata(cseq_to_mid, seq, tempo_us, notes)
    decoded = decode_ctl.walk(ctl)
    by_off = {item["offset"]: item for item in decoded}
    bank = next(item for item in decoded if item.get("kind") == "ALBank")
    pcm = render(notes, decode_ctl, audio_codec, by_off, bank, tbl, args.gain)

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
            f"{args.sequence_index} + B1_sounds1_ctl/tbl"
        ),
        "tool": "scripts/sfx/bgm/render-audio-bgm-pupupu.py",
        "sample_rate": OUTPUT_SAMPLE_RATE,
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
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
