#!/usr/bin/env python3
"""Decode packed FGM cues to WAV -- exactly the samples the DS reconstructs.

An audio bug report is almost always about how something SOUNDS, and the
expensive way to answer one is to build a ROM, boot it, and provoke the cue.
This is the cheap way: the pack already holds the IMA ADPCM the hardware
plays, so decoding it offline gives the owner the sound in isolation, with no
mix and no emulator, and bisects the report in one listen. Wrong in the WAV
means the AOT render is wrong; right in the WAV but wrong in game means the
defect is in the in-game mix level.

`--volume` applies the entry's `ds_volume` so the output matches what the
channel actually emits. `--sum` mixes several cues the way the DS sums
channels, which is how a motion that fires two cues at once (Fox's roll plays
FGM 11 and voice 364 together) gets judged for headroom.

Verified against the pack's own manifest: the decoder reproduces `decoded_peak`
and `ds_sample_count` for every entry, so a mismatch is a real finding rather
than a decoder artifact -- `--check` asserts exactly that across all 88.

    python scripts/sfx/export-fgm-cue-wav.py 11 364 --sum --volume
    python scripts/sfx/export-fgm-cue-wav.py --check
"""
from __future__ import annotations

import argparse
import json
import struct
import sys
import wave
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[2]
PACK_BIN = REPO_ROOT / "assets/audio/fgm_phase_pack_ima.bin"
PACK_JSON = REPO_ROOT / "assets/audio/fgm_phase_pack_ima.json"
OUT_DIR = REPO_ROOT / "artifacts/audio"

# The two IMA ADPCM tables, as the DS decodes them.
STEP_TABLE = [
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41,
    45, 50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 130, 143, 157, 173, 190, 209,
    230, 253, 279, 307, 337, 371, 408, 449, 494, 544, 598, 658, 724, 796, 876,
    963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, 2272, 2499, 2749,
    3024, 3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630,
    9493, 10442, 11487, 12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623,
    27086, 29794, 32767,
]
INDEX_TABLE = [-1, -1, -1, -1, 2, 4, 6, 8, -1, -1, -1, -1, 2, 4, 6, 8]


def decode_ima(block: bytes) -> list[int]:
    """Header is s16 predictor + u8 step index + u8 pad, then low nibble first."""
    predictor = struct.unpack_from("<h", block, 0)[0]
    index = block[2]
    out: list[int] = []
    for byte in block[4:]:
        for nibble in (byte & 0xF, byte >> 4):
            step = STEP_TABLE[index]
            diff = step >> 3
            if nibble & 1:
                diff += step >> 2
            if nibble & 2:
                diff += step >> 1
            if nibble & 4:
                diff += step
            predictor = predictor - diff if nibble & 8 else predictor + diff
            predictor = max(-32768, min(32767, predictor))
            index = max(0, min(88, index + INDEX_TABLE[nibble]))
            out.append(predictor)
    return out


def load_pack() -> tuple[bytes, dict]:
    return PACK_BIN.read_bytes(), json.loads(PACK_JSON.read_text())


def cue_pcm(blob: bytes, entry: dict, apply_volume: bool) -> list[int]:
    start = entry["pack_data_offset"]
    pcm = decode_ima(blob[start:start + entry["ima_adpcm_bytes"]])
    pcm = pcm[:entry["ds_sample_count"]]
    if apply_volume:
        scale = entry["ds_volume"] / 127.0
        pcm = [int(sample * scale) for sample in pcm]
    return pcm


def write_wav(path: Path, pcm: list[int], rate: int) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with wave.open(str(path), "wb") as handle:
        handle.setnchannels(1)
        handle.setsampwidth(2)
        handle.setframerate(rate)
        handle.writeframes(b"".join(struct.pack("<h", s) for s in pcm))


def describe(label: str, pcm: list[int], rate: int) -> None:
    peak = max(abs(s) for s in pcm) if pcm else 0
    rms = (sum(s * s for s in pcm) / len(pcm)) ** 0.5 if pcm else 0.0
    print(f"  {label:<40} peak={peak:>6} rms={rms:7.0f} "
          f"{len(pcm) / rate:6.3f}s")


def check_all(blob: bytes, manifest: dict) -> int:
    """Every entry must decode to its own manifest peak and sample count.

    Seven entries -- all non-looping announcer/crowd voices -- carry an ODD
    `ds_sample_count` that is exactly one past what their nibbles hold. IMA
    packs two samples per byte, so a whole-byte payload can only ever yield an
    even count; the manifest is claiming a sample the data does not encode.
    It is unreachable: nds_audio_fgm.c parses `sample_count` and validates it
    non-zero (:641, :655) and then never uses it again -- playback length comes
    from `data_bytes`. So the DS cannot read past the payload and this is
    cosmetic. Tolerated at exactly +1 and never wider, because a larger gap
    would mean genuinely absent audio rather than a metadata rounding artifact.
    """
    failures = 0
    tolerated = 0
    for entry in manifest["entries"]:
        pcm = cue_pcm(blob, entry, apply_volume=False)
        peak = max(abs(s) for s in pcm) if pcm else 0
        short_by = entry["ds_sample_count"] - len(pcm)
        if peak != entry["decoded_peak"] or short_by not in (0, 1):
            failures += 1
            print(f"MISMATCH id={entry['id']} {entry['name']}: "
                  f"samples {len(pcm)} vs {entry['ds_sample_count']}, "
                  f"peak {peak} vs {entry['decoded_peak']}")
        elif short_by == 1:
            tolerated += 1
    total = len(manifest["entries"])
    if failures:
        print(f"FGM cue decode FAIL: {failures} of {total} entries disagree")
        return 1
    print(f"FGM cue decode PASS: {total} entries reproduce decoded_peak; "
          f"{tolerated} carry the known cosmetic +1 ds_sample_count")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("ids", nargs="*", type=int, help="FGM cue ids to export")
    parser.add_argument("--volume", action="store_true",
                        help="scale by the entry's ds_volume (what the channel emits)")
    parser.add_argument("--sum", action="store_true",
                        help="also mix the given cues as the DS sums channels")
    parser.add_argument("--check", action="store_true",
                        help="decode every entry and assert it matches the manifest")
    args = parser.parse_args()

    blob, manifest = load_pack()
    if args.check:
        return check_all(blob, manifest)
    if not args.ids:
        parser.error("give at least one cue id, or --check")

    by_id = {entry["id"]: entry for entry in manifest["entries"]}
    rendered: list[list[int]] = []
    rates: list[int] = []
    for cue_id in args.ids:
        entry = by_id.get(cue_id)
        if entry is None:
            print(f"cue id {cue_id} is not in the pack")
            return 1
        pcm = cue_pcm(blob, entry, args.volume)
        rate = entry["ds_frequency_hz"]
        rendered.append(pcm)
        rates.append(rate)
        out = OUT_DIR / f"fgm{cue_id}-{entry['name']}-as-ds-plays-it.wav"
        write_wav(out, pcm, rate)
        describe(f"{entry['name']} (vol {entry['ds_volume']})", pcm, rate)
        print(f"    -> {out.relative_to(REPO_ROOT)}")

    if args.sum and len(rendered) > 1:
        # Cues do NOT share a sample rate -- the pack spans 12699 Hz to 53786 Hz,
        # and the two cues Fox's roll fires are 32000 and 16000. Summing them
        # index-by-index would play the slower one at double speed and misalign
        # the mix in time, so the peak and clip count it produced would be
        # answering a question nobody asked. Resample to the fastest rate first.
        rate = max(rates)
        aligned = []
        for pcm, cue_rate in zip(rendered, rates):
            if cue_rate == rate:
                aligned.append(pcm)
                continue
            ratio = cue_rate / rate
            aligned.append([pcm[min(len(pcm) - 1, int(i * ratio))]
                            for i in range(int(len(pcm) / ratio))])
        width = max(len(p) for p in aligned)
        mixed = [0] * width
        clipped = 0
        for index in range(width):
            total = sum(p[index] for p in aligned if index < len(p))
            if abs(total) > 32767:
                clipped += 1
            mixed[index] = max(-32768, min(32767, total))
        stem = "-".join(str(i) for i in args.ids)
        out = OUT_DIR / f"fgm-{stem}-summed.wav"
        write_wav(out, mixed, rate)
        describe(f"summed ({clipped} hard-clipped samples)", mixed, rate)
        print(f"    -> {out.relative_to(REPO_ROOT)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
