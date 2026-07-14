#!/usr/bin/env python3
"""Build the bounded P1 FGM phase pack from BattleShip's original audio.

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
PACK_VERSION = 2
PACK_HEADER = struct.Struct("<4sHHII")
PACK_ENTRY = struct.Struct("<HHIIIHHBBHIHH")
PACK_ENVELOPE_POINT = struct.Struct("<HBB")
FGM_TIMER_MICROSECONDS = 5750
FGM_OUTPUT_RATE = 32000
MAX_PHASE_RESIDENT_BYTES = 64 * 1024

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


def validate_ucd(program: list[list], selector: dict) -> None:
    forbidden = {"fork_voice", "mark_loop", "jump_loop", "vol_delta",
                 "pan_delta", "set_t5_neg2400", "set_t5_neg4800"}
    present = {row[0] for row in program}
    if present & forbidden:
        raise ValueError(
            f"FGM {selector['id']} is no longer a bounded single voice: "
            f"{sorted(present & forbidden)}")
    if first_program_arg(program, "set_articulation") != selector["articulation"]:
        raise ValueError(f"FGM {selector['id']} articulation changed")
    if first_program_arg(program, "set_volume") != selector["ucd_volume"]:
        raise ValueError(f"FGM {selector['id']} volume changed")
    notes = [row for row in program if row[0] == "note"]
    if notes != [["note", selector["pitch_code"], 7,
                  selector["duration_ticks"]]]:
        raise ValueError(f"FGM {selector['id']} note program changed: {notes}")
    if not program or program[-1] != ["stop_voice"]:
        raise ValueError(f"FGM {selector['id']} no longer ends in stop_voice")


def validate_articulation(program: list[list], selector: dict) -> None:
    triggers = [int(row[1]) for row in program if row[0] == "trigger"]
    if triggers != [selector["sound"]]:
        raise ValueError(
            f"FGM {selector['id']} trigger changed: {triggers}")
    pitches = [int(row[1]) for row in program if row[0] == "pitch"]
    if pitches != [selector["articulation_pitch_cents"]]:
        raise ValueError(f"FGM {selector['id']} pitch changed: {pitches}")
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
    ucd_player_volume = (ucd_volume * 127) >> 7
    n64_volume = (articulation_volume * ucd_player_volume * 127) >> 7
    return min(127, (n64_volume * 127 + 16383) // 32767)


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
                    "ds_volume": ds_volume(selector["ucd_volume"],
                                           art_volume),
                })
            tick += int(row[2])
        elif row[0] == "end":
            if tick < selector["duration_ticks"]:
                points.append({
                    "tick": tick,
                    "articulation_volume": 0,
                    "ds_volume": 0,
                })
    if not points or points[0]["tick"] != 0:
        points.insert(0, {
            "tick": 0,
            "articulation_volume": 127,
            "ds_volume": ds_volume(selector["ucd_volume"], 127),
        })
    return points


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
    for phase_index, selector in enumerate(SELECTED):
        ucd_program = ucd["entries"][selector["id"]]["program"]
        validate_ucd(ucd_program, selector)
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
        if selector["loop"] != bool(loop):
            raise ValueError(f"FGM {selector['id']} loop flag changed")

        vadpcm = source_raw["B1_sounds2_tbl"][
            wave["base"]:wave["base"] + wave["length"]]
        if len(vadpcm) != wave["length"] or len(vadpcm) < 9:
            raise ValueError(f"FGM {selector['id']} invalid VADPCM extent")
        pcm = audio_codec.adpcm_decode(vadpcm, book["entries"],
                                       book["order"], book["npredictors"])
        # The DS ADPCM channel loops a whole encoded stream cleanly.  Preserve
        # BattleShip's infinite loop range by making that exact range the DS
        # stream; the one-sample pre-roll cannot be represented as a separate
        # ADPCM loop point and is explicitly recorded below instead of being
        # silently replayed on every loop.
        if selector["loop"]:
            runtime_pcm = pcm[selector["loop_start"]:selector["loop_end"]]
            loop_strategy = "full_stream_is_exact_source_loop_range"
        else:
            runtime_pcm = pcm
            loop_strategy = "none"
        ima = ima_encode(runtime_pcm)
        decoded_ima = ima_decode(ima, len(runtime_pcm))
        metrics = audio_metrics(runtime_pcm, decoded_ima)
        if metrics["decoded_peak"] == 0 or metrics["decoded_rms"] <= 0:
            raise ValueError(f"FGM {selector['id']} decoded to silence")

        note_pitch_cents = selector["pitch_code"] * 100 - 1300
        net_pitch_cents = (selector["articulation_pitch_cents"] +
                           note_pitch_cents)
        frequency = round(FGM_OUTPUT_RATE * (2.0 **
                                             (net_pitch_cents / 1200.0)))
        envelope = articulation_envelope(art_program, selector)
        volume = envelope[0]["ds_volume"]
        records.append({
            "id": selector["id"],
            "flags": 1 if selector["loop"] else 0,
            "ima": ima,
            "sample_count": len(runtime_pcm),
            "frequency": frequency,
            "duration_ticks": selector["duration_ticks"],
            "volume": volume,
            "pan": 64,
            "sound": selector["sound"],
            "envelope": envelope[1:],
        })
        metadata_entries.append({
            "phase_index": phase_index,
            "id": selector["id"],
            "name": selector["name"],
            "ucd_program": ucd_program,
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
            "source_loop_infinite": bool(selector["loop"]),
            "ds_loop_strategy": loop_strategy,
            "ds_initial_prefix_samples_dropped": (
                selector["loop_start"] if selector["loop"] else 0),
            "ds_trailing_samples_dropped": (
                len(pcm) - selector["loop_end"] if selector["loop"] else 0),
            "ds_sample_count": len(runtime_pcm),
            "net_pitch_cents": net_pitch_cents,
            "ds_frequency_hz": frequency,
            "source_duration_ticks": selector["duration_ticks"],
            "source_duration_microseconds": (
                selector["duration_ticks"] * FGM_TIMER_MICROSECONDS),
            "ds_volume": volume,
            "ds_pan": 64,
            "ds_initial_volume": volume,
            "source_volume_envelope": envelope,
            "source_sound_gain_fields_used_by_fgm": False,
            "source_volume_mapping": (
                "((articulation_volume * ((ucd_volume * 127) >> 7) * "
                "127) >> 7) linearly mapped from 0..32767 to DS 0..127"),
            "ima_adpcm_bytes": len(ima),
            "ima_adpcm_sha256": sha256(ima),
            **metrics,
        })

    data_offset = PACK_HEADER.size + len(records) * PACK_ENTRY.size
    sample_body = bytearray()
    envelope_body = bytearray()
    entries_blob = bytearray()
    cursor = data_offset
    for record in records:
        record["data_offset"] = cursor
        sample_body += record["ima"]
        cursor += len(record["ima"])
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
            record["sound"], record["envelope_offset"], len(envelope), 0)
    pack_size = data_offset + len(sample_body) + len(envelope_body)
    pack = (PACK_HEADER.pack(PACK_MAGIC, PACK_VERSION, len(records),
                             pack_size, mapping_sha_lo) +
            bytes(entries_blob) + bytes(sample_body) + bytes(envelope_body))
    if len(pack) != pack_size:
        raise AssertionError("pack size accounting mismatch")
    if len(pack) > MAX_PHASE_RESIDENT_BYTES:
        raise ValueError(
            f"phase pack exceeds 64 KiB: {len(pack)} bytes")

    metadata = {
        "format": "BattleShip P1 FGM phase pack / Nintendo DS IMA ADPCM",
        "format_version": PACK_VERSION,
        "entry_count": len(records),
        "entry_bytes": PACK_ENTRY.size,
        "envelope_point_bytes": PACK_ENVELOPE_POINT.size,
        "header_bytes": PACK_HEADER.size,
        "resident_bytes": len(pack),
        "resident_limit_bytes": MAX_PHASE_RESIDENT_BYTES,
        "mapping_sha256": mapping_sha,
        "mapping_sha256_lo": f"0x{mapping_sha_lo:08x}",
        "pack_sha256": sha256(pack),
        "source_fgm_timer_microseconds": FGM_TIMER_MICROSECONDS,
        "source_fgm_output_rate_hz": FGM_OUTPUT_RATE,
        "runtime_conversion": False,
        "known_runtime_fidelity_debt": [
            "PublicExcited drops its one-sample pre-roll so the DS ADPCM "
            "stream can loop the exact BattleShip [1, 28215) range.",
            "The AOT BattleShip volume envelope is scheduled from the "
            "60 Hz game update and uses DS step volume; N64 applied 5.75 ms "
            "ticks with a 5.75 ms synthesizer ramp.",
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
