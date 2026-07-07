#!/usr/bin/env python3
"""Render BattleShip's Pupupu BGM sequence to a DS-friendly PCM16 stream.

This is intentionally a small compatibility renderer for the port's first
audible BGM gate. It derives the stream from the original O2R sequence/bank
files and does not use hand-authored notes or third-party audio.
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


SEQ_INDEX_PUPUPU = 0
OUTPUT_SAMPLE_RATE = 22050
DEFAULT_GAIN = 0.22


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


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--repo", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument(
        "--output",
        type=Path,
        default=Path("assets/audio/bgm_pupupu_pcm16.raw"),
    )
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

    seq = read_seq(sbk, SEQ_INDEX_PUPUPU)
    notes, tempo_us = collect_notes(cseq_to_mid, seq)
    decoded = decode_ctl.walk(ctl)
    by_off = {item["offset"]: item for item in decoded}
    bank = next(item for item in decoded if item.get("kind") == "ALBank")
    pcm = render(notes, decode_ctl, audio_codec, by_off, bank, tbl, args.gain)

    output = (repo / args.output).resolve() if not args.output.is_absolute() else args.output
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(pcm)
    digest = hashlib.sha256(pcm).hexdigest()

    metadata = {
        "source": "BattleShip_o2r/audio/S1_music_sbk sequence 0 + B1_sounds1_ctl/tbl",
        "tool": "scripts/render-audio-bgm-pupupu.py",
        "sample_rate": OUTPUT_SAMPLE_RATE,
        "format": "signed PCM16LE mono raw",
        "bytes": len(pcm),
        "sha256": digest,
        "sequence_index": SEQ_INDEX_PUPUPU,
        "note_count": len(notes),
        "tempo_us_per_quarter": tempo_us,
        "gain": args.gain,
    }
    output.with_suffix(".json").write_text(json.dumps(metadata, indent=2) + "\n")

    print(f"rendered {output}")
    print(f"bytes={len(pcm)} sample_rate={OUTPUT_SAMPLE_RATE} sha256={digest}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
