#!/usr/bin/env python3
"""P4 audio: a Remix content's sounds as a DS FGM extension pack.

Smash Remix adds each fighter's voices with FGM.asm's
``add_sound(name, sample_rate, fgm_type, reverb, fgm_length)``: a VADPCM
``.aifc`` in the donor tree, one new FGM id (bass prints "Added <name>" /
"FGM_ID: 0x..." for each), and one of three microcode templates -- voice,
chant (a crowd chant) or sleep. BattleShip's own decoders
(``decomp/BattleShip-main/decomp/tools/extract_fgm.py``) read those
templates as the same UCD and articulation programs the vanilla cues use (a
chant is PublicFox's program), so every DS parameter here comes from the
vanilla pack's own formulas (``scripts/sfx/render-audio-fgm-phase-pack.py``):
the channel rate from the articulation pitch and the note, the channel
volume from the UCD and articulation volumes, the pause bit from the UCD,
and the same IMA-ADPCM encoder.

One choice is the DS's own: Remix's automatic note length (the file size /
177) is shorter than most of its voice samples; on the N64 the voice rings
out through its envelope's release after the note ends. The DS cue plays the
whole sample (its duration is at least the sample's).

Output (ROM-derived, so under the ignored build tree only): ``fgm_p4.bin``
in the vanilla pack's layout with magic ``FGP4`` -- header, 32-byte entries,
IMA bodies on 32-byte boundaries -- and ``fgm_p4.json`` with each cue's
derivation.

    python scripts/p4/p4_audio.py --staging builds/p4-staging/remix-5e04fe7 \
        --content FALCO --out builds/<build>/p4/audio
"""
from __future__ import annotations

import argparse
import subprocess
import hashlib
import importlib.util
import json
import math
import re
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
RENDERER = ROOT / "scripts" / "sfx" / "render-audio-fgm-phase-pack.py"
BGM_RENDERER = ROOT / "scripts" / "sfx" / "bgm" / "render-audio-bgm.py"
GENERATOR = ROOT / "scripts" / "p4" / "generate_p4_fighter.py"
# The DS's BGM tracks reach nSYAudioBGM ids below this; a content's victory
# id at or above it is a Remix song (MIDI.asm).
VANILLA_BGM_COUNT = 0x2F
BGM_NO_LOOP = 0xFFFFFFFF
EXTRACT_FGM = ROOT / "decomp" / "BattleShip-main" / "decomp" / "tools" / "extract_fgm.py"

PACK_MAGIC = b"FGP4"
PACK_VERSION = 1
FLAG_PAUSE_WITH_GAME = 1 << 1

# FGM.asm: rate bytes and template types.
SAMPLE_RATES = {0x20: "SAMPLE_RATE_16000", 0x60: "SAMPLE_RATE_32000"}
FGM_TYPES = {"FGM_TYPE_VOICE": 0, "FGM_TYPE_CHANT": 1, "FGM_TYPE_SLEEP": 2}

# The content name in FGM.asm paths ("Falco/sounds/66") per P4 content.
CONTENT_DIRS = {"FALCO": "Falco"}


def load_module(path: Path, name: str):
    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot import {path}")
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


def fgm_ids(bass_out: str) -> dict[str, int]:
    """bass.out's "Added <name>" / "FGM_ID: 0x..." pairs."""
    ids = {}
    for match in re.finditer(r"Added (\S+)\s*\nFGM_ID: 0x([0-9A-Fa-f]+)", bass_out):
        ids[match.group(1)] = int(match.group(2), 16)
    return ids


def add_sound_rows(fgm_asm: str, content_dir: str) -> list[dict]:
    """FGM.asm's add_sound calls for one content, in declaration order."""
    rows = []
    pattern = re.compile(
        r"^\s*add_sound\(\s*([^,\s]+)\s*,\s*(\w+)\s*,\s*(\w+)\s*,\s*(-?\w+)\s*,\s*(-?\w+)\s*\)",
        re.M)
    for match in pattern.finditer(fgm_asm):
        name, rate, kind, reverb, length = match.groups()
        if not name.startswith(content_dir + "/"):
            continue
        if kind not in FGM_TYPES:
            raise ValueError(f"{name}: unknown FGM type {kind}")
        rate_byte = {"SAMPLE_RATE_16000": 0x20, "SAMPLE_RATE_32000": 0x60}.get(rate)
        if rate_byte is None:
            raise ValueError(f"{name}: unknown sample rate {rate}")
        rows.append({
            "name": name,
            "rate_byte": rate_byte,
            "type": kind,
            "reverb": int(reverb, 0),
            "length": int(length, 0),
        })
    for match in re.finditer(r"^\s*add_sound_advanced\(\s*([^,\s]+)", fgm_asm, re.M):
        if match.group(1).startswith(content_dir + "/"):
            raise ValueError(f"{match.group(1)}: add_sound_advanced is not ported")
    return rows


def read_aifc(path: Path) -> dict:
    """A VADPCM AIFC: the codebook (stoc VADPCMCODES) and the SSND frames."""
    data = path.read_bytes()
    if data[:4] != b"FORM" or data[8:12] != b"AIFC":
        raise ValueError(f"{path}: not an AIFC file")
    form_size = struct.unpack(">I", data[4:8])[0]
    book = None
    frames = None
    comm = None
    pos = 12
    while pos + 8 <= len(data):
        chunk = data[pos:pos + 4]
        size = struct.unpack(">I", data[pos + 4:pos + 8])[0]
        body = data[pos + 8:pos + 8 + size]
        if chunk == b"COMM":
            channels, sample_frames, bits = struct.unpack(">hIh", body[:8])
            comm = {"channels": channels, "frames": sample_frames, "bits": bits,
                    "compression": body[18:22].decode("latin-1")}
        elif chunk == b"APPL" and body[4:16] == b"\x0bVADPCMCODES":
            version, order, npredictors = struct.unpack(">hhh", body[16:22])
            count = order * npredictors * 8
            book = {"version": version, "order": order, "npredictors": npredictors,
                    "entries": list(struct.unpack(f">{count}h", body[22:22 + count * 2]))}
        elif chunk == b"SSND":
            offset = struct.unpack(">I", body[:4])[0]
            frames = body[8 + offset:]
        pos += 8 + size + (size & 1)
    if comm is None or book is None or frames is None:
        raise ValueError(f"{path}: missing COMM, VADPCMCODES or SSND")
    if comm["channels"] != 1 or comm["compression"] != "VAPC":
        raise ValueError(f"{path}: not mono VADPCM ({comm})")
    return {"form_size": form_size, "comm": comm, "book": book,
            "vadpcm": frames[:len(frames) - (len(frames) % 9)]}


def templates(rate_byte: int, kind: str, reverb: int, length: int,
              sound_index: int, articulation_index: int) -> tuple[bytes, bytes]:
    """FGM.asm's add_microcode (UCD) and add_sfx_fgm (articulation) bytes."""
    art = bytes([0x60]) + (0x8000 | sound_index).to_bytes(2, "big") + bytes([rate_byte])
    ref = (0x8000 | articulation_index).to_bytes(2, "big")
    note_len = (0x8000 | length).to_bytes(2, "big")
    if kind == "FGM_TYPE_VOICE":
        art += bytes([0xFB, 0x50, 0x30, min(reverb, 127)]) + bytes.fromhex("089F407F") + b"\x70"
        ucd = (bytes.fromhex("DE00") + b"\xD1" + ref + bytes.fromhex("D2FFD3D4D224D5FF") +
               b"\x6F" + note_len + b"\xD0")
    elif kind == "FGM_TYPE_CHANT":
        art += bytes.fromhex("306420FB5A0B747F") + b"\x70"
        ucd = (bytes.fromhex("DE04") + b"\xD1" + ref + bytes.fromhex("D2FFD37FD224DC26D5FF") +
               b"\x6F" + note_len + b"\xD0")
    else:
        art += bytes.fromhex("FB500A2C6470")
        ucd = (bytes.fromhex("DE00") + b"\xD1" + ref +
               bytes.fromhex("D2FFDC0AD3C8D2A4D5FF") + b"\x77" + note_len +
               bytes.fromhex("D5FF") + b"\x77" + note_len +
               bytes.fromhex("D5FF") + b"\x77" + note_len + b"\xD0")
    return ucd, art


def last(program: list[list], op: str, default=None):
    value = default
    for row in program:
        if row[0] == op:
            value = row[1]
    return value


def build(staging: Path, content: str, out_dir: Path) -> dict:
    renderer = load_module(RENDERER, "fgm_phase_pack_renderer")
    extract = load_module(EXTRACT_FGM, "extract_fgm")
    sys.path.insert(0, str(ROOT / "scripts" / "sfx"))
    import vadpcm_decode  # noqa: E402

    content_dir = CONTENT_DIRS[content]
    ids = fgm_ids((staging / "bass.out").read_text(encoding="utf-8", errors="replace"))
    rows = add_sound_rows((staging / "src" / "FGM.asm").read_text(encoding="utf-8"),
                          content_dir)
    if not rows:
        raise ValueError(f"{content}: no add_sound rows in FGM.asm")
    tick_us = renderer.FGM_TIMER_MICROSECONDS
    records = []
    for row in rows:
        fgm_id = ids.get(row["name"])
        if fgm_id is None:
            raise ValueError(f"{row['name']}: no FGM_ID in bass.out")
        aifc = read_aifc(staging / "src" / f"{row['name']}.aifc")
        # FGM.asm: an explicit length, else the file's FORM size / 177.
        length = row["length"] if row["length"] != -1 else aifc["form_size"] // 177
        ucd_bytes, art_bytes = templates(row["rate_byte"], row["type"], row["reverb"],
                                         length, 0x1F0, fgm_id)
        ucd = extract._disassemble_ucd_entry(ucd_bytes)
        art = extract._disassemble_entry(art_bytes)
        notes = [r for r in ucd if r[0] == "note"]
        pitch = last(art, "pitch", 0)
        art_volume = last(art, "vol", 127)
        ucd_volume = last(ucd, "set_volume", 255)
        unk1f = last(ucd, "set_unk1F", 0)
        if not notes or any(n[1] != notes[0][1] for n in notes):
            raise ValueError(f"{row['name']}: notes {notes} change pitch")
        frequency = renderer.note_frequency_hz(pitch, notes[0][1])
        volume = renderer.ds_volume(ucd_volume, art_volume)
        pcm = vadpcm_decode.adpcm_decode(aifc["vadpcm"], aifc["book"]["entries"],
                                         aifc["book"]["order"], aifc["book"]["npredictors"])
        note_samples = [round(n[3] * tick_us * frequency / 1_000_000) for n in notes]
        if len(notes) == 1:
            runtime_pcm = list(pcm)
        else:
            # Each note restarts the sample (a new voice per note): lay the
            # sample at each note's start, silence between.
            runtime_pcm = []
            for count in note_samples:
                part = list(pcm[:count])
                runtime_pcm.extend(part + [0] * (count - len(part)))
        while len(runtime_pcm) > 1 and runtime_pcm[-1] == 0:
            runtime_pcm.pop()
        sample_ticks = math.ceil(len(runtime_pcm) * 1_000_000 / (frequency * tick_us))
        duration = max(sum(n[3] for n in notes), sample_ticks)
        ima = renderer.ima_encode(runtime_pcm)
        decoded = renderer.ima_decode(ima, len(runtime_pcm))
        metrics = renderer.audio_metrics(runtime_pcm, decoded)
        if metrics["ima_snr_db"] < 14.0:
            ima = renderer.ima_encode_lookahead(runtime_pcm)
            decoded = renderer.ima_decode(ima, len(runtime_pcm))
            metrics = renderer.audio_metrics(runtime_pcm, decoded)
        if len(ima) > renderer.MAX_CUE_IMA_BYTES:
            raise ValueError(f"{row['name']}: {len(ima)} B IMA exceeds a cache slot")
        records.append({
            "id": fgm_id,
            "name": row["name"],
            "type": row["type"],
            "flags": FLAG_PAUSE_WITH_GAME if (unk1f & 0x80) else 0,
            "frequency": frequency,
            "volume": volume,
            "pan": 64,
            "duration_ticks": duration,
            "note_ticks": [n[3] for n in notes],
            "sample_count": len(runtime_pcm),
            "ima": ima,
            "ucd_program": ucd,
            "articulation_program": art,
            "source_frames": aifc["comm"]["frames"],
            "ima_snr_db": metrics["ima_snr_db"],
        })

    records.sort(key=lambda r: r["id"])
    header_bytes = renderer.PACK_HEADER.size
    entry_bytes = renderer.PACK_ENTRY.size
    cursor = (header_bytes + entry_bytes * len(records) + 31) & ~31
    body = bytearray()
    for record in records:
        record["data_offset"] = cursor + len(body)
        body += record["ima"]
        body += bytes((-len(body)) & 31)
    entries = bytearray()
    for record in records:
        entries += renderer.PACK_ENTRY.pack(
            record["id"], record["flags"], record["data_offset"], len(record["ima"]),
            record["sample_count"], record["frequency"], record["duration_ticks"],
            record["volume"], record["pan"], 0, 0, 0, 0)
    pack_size = cursor + len(body)
    pack = (renderer.PACK_HEADER.pack(PACK_MAGIC, PACK_VERSION, len(records), pack_size, 0) +
            bytes(entries))
    pack += bytes(cursor - len(pack)) + bytes(body)
    if len(pack) != pack_size:
        raise AssertionError("P4 FGM pack size accounting mismatch")
    out_dir.mkdir(parents=True, exist_ok=True)
    (out_dir / "fgm_p4.bin").write_bytes(pack)
    manifest = {
        "format": "FGP4", "version": PACK_VERSION, "content": content,
        "pack_bytes": len(pack), "pack_sha256": hashlib.sha256(pack).hexdigest(),
        "entries": [{k: v for k, v in r.items() if k != "ima"} for r in records],
    }
    (out_dir / "fgm_p4.json").write_text(json.dumps(manifest, indent=1) + "\n",
                                         encoding="utf-8", newline="\n")
    return manifest


def remix_songs(bass_out: str) -> dict[int, str]:
    """bass.out's MIDI lines: "Added MIDI_<NAME>(<path>)" and
    "MIDI_<NAME>_ID: 0x.." -> {id: path relative to the donor src/}."""
    paths = {m.group(1): m.group(2) for m in
             re.finditer(r"Added MIDI_(\w+)\(\.\./src/([^)]+)\)", bass_out)}
    songs = {}
    for m in re.finditer(r"^MIDI_(\w+)_ID: 0x([0-9A-Fa-f]+)", bass_out, re.M):
        if m.group(1) in paths:
            songs[int(m.group(2), 16)] = paths[m.group(1)]
    return songs


def build_bgm(staging: Path, content: str, export_dir: Path, out_dir: Path) -> list[dict]:
    """The content's victory song, when it is a Remix one: rendered on the
    vanilla sequence bank by the vanilla BGM renderer, plus its track row."""
    generator = load_module(GENERATOR, "generate_p4_fighter")
    rom = generator.R.Rom(staging / "ssb64asm.z64")
    resolved = json.loads((export_dir / "resolved.json").read_text(encoding="utf-8"))
    bgm_id = generator.present_rows(rom, resolved["kind_tables"])["victory_bgm"]
    if bgm_id < VANILLA_BGM_COUNT:
        return []
    songs = remix_songs((staging / "bass.out").read_text(encoding="utf-8", errors="replace"))
    rel = songs.get(bgm_id)
    if rel is None:
        raise ValueError(f"{content}: victory BGM {bgm_id:#x} has no MIDI line in bass.out")
    source = staging / "src" / rel
    name = f"bgm_p4_{bgm_id:02x}"
    output = out_dir / f"{name}.bin"
    subprocess.run([sys.executable, str(BGM_RENDERER), "--repo", str(ROOT),
                    "--sequence-file", str(source), "--output", str(output)],
                   check=True, capture_output=True)
    meta = json.loads(output.with_suffix(".json").read_text(encoding="utf-8"))
    looping = bool(meta["looping"])
    return [{
        "id": bgm_id, "name": name, "source": rel,
        "path": f"nitro:/p4/audio/{name}.bin",
        "stream_bytes": meta["source_pcm_bytes"],
        "loop_start_bytes": meta["loop_start_byte"] if looping else 0,
        "asset_bytes": meta["bytes"],
        "packet_count": meta["packet_count"],
        "loop_packet": meta["loop_packet_index"] if looping else BGM_NO_LOOP,
        "loop_record": meta["loop_record_offset"] if looping else 0,
        "looping": looping,
        "ima_snr_db": meta.get("ima_snr_db"),
    }]


def write_bgm_rows(tracks: list[dict], out_dir: Path):
    """Rows nds_audio_bgm.c appends to sNdsAudioBgmTracks in P4 builds; each
    opens with its comma, as the table's conditional rows do."""
    lines = ["/* Generated by scripts/p4/p4_audio.py: the P4 contents' Remix songs. */"]
    for t in tracks:
        lines += [
            f"    /* {t['source']} */",
            "    ,",
            "    {",
            f"        {t['id']:#x},",
            f"        \"{t['path']}\",",
            f"        {t['stream_bytes']}u, {t['loop_start_bytes']}u, {t['asset_bytes']}u,",
            f"        {t['packet_count']}u, {t['loop_packet']:#x}u, {t['loop_record']}u,",
            f"        {'TRUE' if t['looping'] else 'FALSE'}",
            "    }",
        ]
    (out_dir / "nds_p4_bgm.generated.inc").write_text("\n".join(lines) + "\n",
                                                        encoding="utf-8", newline="\n")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("--staging", type=Path, required=True)
    parser.add_argument("--content", action="append", required=True,
                        choices=sorted(CONTENT_DIRS))
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--export-root", type=Path, required=True,
                        help="remix_export.py output root (<content>/resolved.json)")
    args = parser.parse_args()
    if len(args.content) != 1:
        raise SystemExit("one content per pack for now")
    manifest = build(args.staging, args.content[0], args.out)
    tracks = []
    for content in args.content:
        tracks += build_bgm(args.staging, content,
                            args.export_root / CONTENT_DIRS[content].lower(), args.out)
    write_bgm_rows(tracks, args.out)
    for t in tracks:
        print(f"  bgm {t['id']:#x} {t['source']}: {t['asset_bytes']} B, "
              f"{t['packet_count']} packets, looping {t['looping']}")
    print(f"p4 audio: {manifest['content']} {len(manifest['entries'])} cues, "
          f"{manifest['pack_bytes']} bytes")
    for entry in manifest["entries"]:
        print(f"  {entry['id']:#05x} {entry['name']:<26} {entry['type']:<15} "
              f"{entry['frequency']:>5} Hz vol {entry['volume']:>3} "
              f"{entry['duration_ticks']:>4} ticks {entry['sample_count']:>6} samples "
              f"{entry['ima_snr_db']:.1f} dB")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
