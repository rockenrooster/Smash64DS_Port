#!/usr/bin/env python3
"""Admit EXTRA AIFC cues through the existing native FGM1-v4 audio pack.

The appender's generated FGM.asm and SOUND_ADD_LIST supply cue identities.
FGM.add_sound/add_microcode/add_sfx_fgm define their programs; BattleShip's
decoder and the existing DS AOT/IMA producer perform all audio conversion.
No N64 audio interpreter or second runtime bank is introduced.
"""
from __future__ import annotations

import argparse
import ast
from dataclasses import dataclass
import hashlib
import importlib.util
import json
import math
from pathlib import Path
import re
import struct
import sys

_scripts = Path(__file__).resolve().parents[1]
if str(_scripts) not in sys.path:
    sys.path.insert(0, str(_scripts))
import _paths  # noqa: E402,F401
import vadpcm_decode  # noqa: E402


HEADER = struct.Struct("<4sHHII")
ENTRY = struct.Struct("<HHIIIHHBBHIHH")
POINT = struct.Struct("<HBB")


class AudioAdmissionError(ValueError):
    """Missing or malformed source audio; never substitute another cue."""


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def load_module(path: Path, name: str):
    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None:
        raise AudioAdmissionError(f"cannot import source producer {path}")
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


@dataclass(frozen=True)
class Aifc:
    form_size: int
    file_size: int
    sample_count: int
    sample_rate: int
    order: int
    predictors: int
    book: tuple[int, ...]
    vadpcm: bytes
    trailing_sample_bytes: int


def parse_aifc(data: bytes) -> Aifc:
    """Decode chunk framing, including EXTRA's known stale FORM size field.

    Its files retain the uncompressed AIFF FORM size after compression. The
    donor macro explicitly uses that field for note length, and SSND's own size
    for VADPCM bytes. Neither size may replace the other.
    """
    if len(data) < 12 or data[:4] != b"FORM" or data[8:12] != b"AIFC":
        raise AudioAdmissionError("expected FORM/AIFC source")
    form_size = struct.unpack_from(">I", data, 4)[0]
    chunks: dict[bytes, bytes] = {}
    offset = 12
    while offset < len(data):
        if offset + 8 > len(data):
            raise AudioAdmissionError("truncated AIFC chunk header")
        tag, count = struct.unpack_from(">4sI", data, offset)
        end = offset + 8 + count
        if end + (count & 1) > len(data):
            raise AudioAdmissionError(f"truncated AIFC {tag!r} chunk")
        payload = data[offset + 8:end]
        if tag in chunks and tag in (b"COMM", b"SSND"):
            raise AudioAdmissionError(f"duplicate AIFC {tag!r} chunk")
        if tag == b"APPL":
            if payload[:4] == b"stoc" and payload[4:16] == b"\x0bVADPCMCODES":
                if b"BOOK" in chunks:
                    raise AudioAdmissionError("duplicate VADPCM codebook")
                chunks[b"BOOK"] = payload[16:]
        else:
            chunks[tag] = payload
        offset = end + (count & 1)
    if not all(tag in chunks for tag in (b"COMM", b"SSND", b"BOOK")):
        raise AudioAdmissionError("missing AIFC COMM/SSND/VADPCMCODES")
    comm = chunks[b"COMM"]
    if len(comm) < 22:
        raise AudioAdmissionError("short AIFC COMM")
    channels, samples, bits = struct.unpack_from(">HIH", comm)
    if channels != 1 or bits != 16 or comm[18:22] != b"VAPC":
        raise AudioAdmissionError("native admission requires mono s16 VAPC")
    exponent, mantissa = struct.unpack_from(">HQ", comm, 8)
    if exponent & 0x8000 or exponent in (0, 0x7FFF):
        raise AudioAdmissionError("invalid AIFC extended sample rate")
    rate = math.ldexp(mantissa, exponent - 16383 - 63)
    if not rate.is_integer() or int(rate) not in (16000, 32000):
        raise AudioAdmissionError(f"unsupported AIFC sample rate {rate}")
    book = chunks[b"BOOK"]
    if len(book) < 6:
        raise AudioAdmissionError("short VADPCM codebook")
    version, order, predictors = struct.unpack_from(">HHH", book)
    ncoeffs = order * predictors * 8
    if (version != 1 or not 1 <= order <= 8 or not 1 <= predictors <= 16 or
            len(book) != 6 + ncoeffs * 2):
        raise AudioAdmissionError("invalid VADPCM codebook shape")
    coeffs = struct.unpack_from(f">{ncoeffs}h", book, 6)
    ssnd = chunks[b"SSND"]
    if len(ssnd) < 8 or struct.unpack_from(">II", ssnd) != (0, 0):
        raise AudioAdmissionError("unsupported SSND offset/block size")
    # EXTRA's macro inserts the whole SSND payload, including its optional
    # single zero alignment byte. The source decoder ignores incomplete frames.
    raw = ssnd[8:]
    frame_size = len(raw) // 9 * 9
    trailing = raw[frame_size:]
    if len(trailing) > 1 or any(trailing):
        raise AudioAdmissionError("SSND has non-frame trailing data")
    # Several pinned EXTRA cues declare one more COMM sample than their encoded
    # complete frames. The N64 producer uses the SSND frame extent instead.
    if not frame_size or not 0 < samples <= (frame_size // 9) * 16 + 1:
        raise AudioAdmissionError("COMM sample count exceeds VADPCM frames")
    if (frame_size // 9) * 16 - samples >= 16:
        raise AudioAdmissionError("COMM sample count discards whole VADPCM frames")
    for header in raw[:frame_size:9]:
        if (header & 15) >= predictors or header >> 4 > 12:
            raise AudioAdmissionError("VADPCM frame has invalid predictor/scale")
    return Aifc(form_size, len(data), samples, int(rate), order, predictors,
                coeffs, raw[:frame_size], len(trailing))


def config_sounds(text: str) -> tuple[dict[str, str], str]:
    """Read the deliberately narrow sounds/announcer config interface.

    Rates, types, effects and durations come from the generated add_sound rows,
    so there is no second YAML macro evaluator or dependency on ambient PyYAML.
    """
    sounds: dict[str, str] = {}
    in_sounds = False
    announcer = None
    for line in text.splitlines():
        if line == "sounds:":
            in_sounds = True
            continue
        if line and not line.startswith((" ", "#")):
            in_sounds = False
        if in_sounds:
            row = re.fullmatch(r'  ["\']?([A-Fa-f0-9]{4})["\']?:\s*([A-Za-z0-9_]+)\s*', line)
            if row:
                if row[1].upper() in sounds or row[2] in sounds.values():
                    raise AudioAdmissionError("duplicate sound placeholder/name")
                sounds[row[1].upper()] = row[2]
            elif line.strip() and not line.lstrip().startswith("#"):
                raise AudioAdmissionError(f"unsupported sounds config row: {line}")
        row = re.fullmatch(r'announcer_fgm:\s*["\']?([A-Fa-f0-9]{4})["\']?\s*', line)
        if row:
            announcer = row[1].upper()
    if not sounds or announcer not in sounds:
        raise AudioAdmissionError("missing sounds/announcer source identity")
    return sounds, announcer


def resolved_sound_ids(log: str, expected_names: set[str]) -> dict[str, int]:
    matches = re.findall(r"^SOUND_ADD_LIST\(character\):\s*(.+)$", log, re.M)
    mappings = []
    for value in matches:
        try:
            parsed = ast.literal_eval(value)
        except (ValueError, SyntaxError) as error:
            raise AudioAdmissionError("invalid appender sound identity") from error
        if isinstance(parsed, dict) and set(parsed) == expected_names:
            mappings.append({name: int(cue, 16) for name, cue in parsed.items()})
    if len(mappings) != 1:
        raise AudioAdmissionError("expected one complete resolved character sound map")
    result = mappings[0]
    if len(set(result.values())) != len(result) or any(not 0 <= cue < 0x8000 for cue in result.values()):
        raise AudioAdmissionError("resolved FGM IDs are duplicated/out of range")
    return result


def generated_sound_rows(text: str, character: str) -> list[dict]:
    pattern = re.compile(
        rf"^\s*add_sound\(\.\./build/extra_characters/{re.escape(character)}/sounds/"
        r"([A-Za-z0-9_]+),\s*SAMPLE_RATE_(16000|32000),\s*FGM_TYPE_(VOICE|CHANT|SLEEP),"
        r"\s*(\d+),\s*(-1|0x[0-9A-Fa-f]+|\d+)\)\s*$", re.M)
    rows = [{"name": match[1], "sample_rate": int(match[2]),
             "type": match[3], "reverb": int(match[4]),
             "length": int(match[5], 0)} for match in pattern.finditer(text)]
    if not rows or len({row["name"] for row in rows}) != len(rows):
        raise AudioAdmissionError("missing/duplicate generated add_sound rows")
    if any(not 0 <= row["reverb"] <= 127 for row in rows):
        raise AudioAdmissionError("generated sound reverb outside source domain")
    candidate_lines = [line for line in text.splitlines()
                       if "add_sound(" in line and f"/{character}/sounds/" in line]
    if len(candidate_lines) != len(rows):
        raise AudioAdmissionError("unsupported generated character sound macro")
    return rows


def verify_resolved_inputs(manifest: dict, character: str,
                           inputs: dict[str, bytes]) -> None:
    if manifest.get("phase") != "resolved" or manifest.get("character") != character:
        raise AudioAdmissionError("audio admission requires a resolved character donor")
    outputs = {row["path"]: row for row in manifest.get("outputs", [])}
    for path, data in inputs.items():
        record = outputs.get(path)
        if (record is None or record.get("size") != len(data) or
                record.get("sha256") != sha256(data)):
            raise AudioAdmissionError(f"resolved donor input identity changed: {path}")


def microcode(row: dict, aifc: Aifc, articulation: int = 0,
              sound: int = 0) -> tuple[bytes, bytes]:
    """Literal expansion of the pinned FGM.asm add_sound macros.

    Articulation/sound indices are remapped to zero in an isolated host bank;
    event-facing FGM IDs remain the appender's resolved donor identities.
    """
    ticks = aifc.form_size // 177 if row["length"] == -1 else row["length"]
    if not 0 < ticks < 0x8000 or articulation >= 0x8000 or sound >= 0x8000:
        raise AudioAdmissionError("invalid source note duration/index")
    length = struct.pack(">H", ticks | 0x8000)
    prefix = b"\xde" + (b"\x04" if row["type"] == "CHANT" else b"\x00")
    prefix += b"\xd1" + struct.pack(">H", articulation | 0x8000) + b"\xd2\xff"
    if row["type"] == "VOICE":
        ucd = prefix + b"\xd3\xd4\xd2\x24\xd5\xff\x6f" + length + b"\xd0"
    elif row["type"] == "CHANT":
        ucd = prefix + b"\xd3\x7f\xd2\x24\xdc\x26\xd5\xff\x6f" + length + b"\xd0"
    elif row["type"] == "SLEEP":
        ucd = prefix + b"\xdc\x0a\xd3\xc8\xd2\xa4" + (b"\xd5\xff\x77" + length) * 3 + b"\xd0"
    else:
        raise AudioAdmissionError(f"unsupported source FGM type {row['type']}")
    art = b"\x60" + struct.pack(">H", sound | 0x8000)
    art += b"\x20" if row["sample_rate"] == 16000 else b"\x60"
    if row["type"] == "VOICE":
        art += b"\xfb\x50\x30" + bytes([row["reverb"]]) + b"\x08\x9f\x40\x7f\x70"
    elif row["type"] == "CHANT":
        art += b"\x30\x64\x20\xfb\x5a\x0b\x74\x7f\x70"
    else:
        art += b"\xfb\x50\x0a\x2c\x64\x70"
    return ucd, art


def linked_microcode(rom: bytes, main_source: str, cue_id: int,
                     expected_ucd: bytes, expected_art: bytes,
                     decoder) -> tuple[bytes, bytes]:
    """Check donor cue tables and actual linked programs before native baking.

    All constants below are authored patch locations/addresses in the pinned
    FGM.asm and MIDI.asm. The appended-code mapping comes from main.asm's
    origin/base pair; it is never guessed from a ROM-size heuristic.
    """
    def be32(offset: int) -> int:
        if offset < 0 or offset + 4 > len(rom):
            raise AudioAdmissionError("linked audio table outside donor")
        return struct.unpack_from(">I", rom, offset)[0]

    mapping = re.search(r"origin\s+(0x[0-9a-fA-F]+)\s+base\s+(0x[0-9a-fA-F]+)", main_source)
    if mapping is None or len(rom) < 0x1883BC or rom[:4] != bytes.fromhex("80371240"):
        raise AudioAdmissionError("missing linked donor appended-code mapping")
    origin, base = int(mapping[1], 16), int(mapping[2], 16)
    count = struct.unpack_from(">H", rom, 0x1883BA)[0]
    difference = (((count + 1) * 8 + 0xF) & 0xFFF0) - 0x35C0

    def program_offset(table: int, pc: int, index: int) -> int:
        if index >= be32(table):
            raise AudioAdmissionError("linked audio index exceeds donor table")
        target = pc + be32(table + 4 + index * 4)
        offset = target - base + origin
        if target < base or offset >= len(rom):
            raise AudioAdmissionError("EXTRA audio program outside appended donor code")
        return offset

    ucd_offset = program_offset(be32(0x3D798), 0x80076D50 + difference, cue_id)
    raw_ucd = rom[ucd_offset:ucd_offset + len(expected_ucd)]
    program = decoder._disassemble_ucd_entry(raw_ucd)
    articulation_rows = [row[1] for row in program if row[0] == "set_articulation"]
    if len(articulation_rows) != 1:
        raise AudioAdmissionError("linked EXTRA cue has no unique articulation")
    art_offset = program_offset(be32(0x3D790), 0x80073F80 + difference, articulation_rows[0])
    raw_art = rom[art_offset:art_offset + len(expected_art)]
    articulation = decoder._disassemble_entry(raw_art)
    trigger_rows = [row[1] for row in articulation if row[0] == "trigger"]
    if len(trigger_rows) != 1:
        raise AudioAdmissionError("linked EXTRA articulation has no unique sample")
    # The isolated host bank's indices are zero. Compare every other byte of
    # the compiled donor microcode, including durations, effects and priority.
    normalized_ucd = bytearray(raw_ucd)
    normalized_art = bytearray(raw_art)
    normalized_ucd[3:5] = expected_ucd[3:5]
    normalized_art[1:3] = expected_art[1:3]
    if bytes(normalized_ucd) != expected_ucd or bytes(normalized_art) != expected_art:
        raise AudioAdmissionError(f"linked source audio macro differs for FGM {cue_id}")
    return raw_ucd, raw_art


def build_extension(repo_root: Path, source_dir: Path, donor_dir: Path,
                    output_rate: int = 16000,
                    reference_root: Path | None = None) -> tuple[list[dict], dict]:
    """Return native pack records plus source/PCM admission metadata in memory."""
    renderer = load_module(repo_root / "scripts/sfx/render-audio-fgm-phase-pack.py",
                           "extra_fgm_renderer")
    reference_root = reference_root or repo_root
    decoder = load_module(reference_root / "decomp/BattleShip-main/decomp/tools/extract_fgm.py",
                          "extra_fgm_source_decoder")
    config_bytes = (source_dir / "config.yaml").read_bytes()
    placeholders, announcer = config_sounds(config_bytes.decode("utf-8"))
    log_bytes = (donor_dir / "appender.log").read_bytes()
    # The appender's Windows log can contain locale-encoded progress text. Its
    # identity row is ASCII and is parsed exactly; retain/hash the raw log.
    ids = resolved_sound_ids(log_bytes.decode("utf-8", errors="replace"), set(placeholders.values()))
    asm_path = donor_dir / "extra/src/FGM.asm"
    asm_bytes = asm_path.read_bytes()
    rows = generated_sound_rows(asm_bytes.decode("utf-8"), source_dir.name)
    if {row["name"] for row in rows} != set(ids):
        raise AudioAdmissionError("generated sounds disagree with config/appender")
    if [ids[row["name"]] for row in rows] != list(range(min(ids.values()), max(ids.values()) + 1)):
        raise AudioAdmissionError("generated sound ordering disagrees with resolved IDs")
    donor_rom = (donor_dir / "extra/ssb64asm_extra_review.z64").read_bytes()
    main_bytes = (donor_dir / "extra/main.asm").read_bytes()
    manifest_bytes = (donor_dir / "donor-manifest.json").read_bytes()
    manifest = json.loads(manifest_bytes)
    verify_resolved_inputs(manifest, source_dir.name,
                           {"extra/ssb64asm_extra_review.z64": donor_rom,
                            "extra/main.asm": main_bytes,
                            "extra/src/FGM.asm": asm_bytes})
    samples_per_tick = renderer.fgm_samples_per_tick(output_rate)
    sine_table, sine_bytes = renderer.source_sine_table(
        reference_root / "decomp/BattleShip-main/decomp/src/sys/sintable.c")
    # These literal add_sound expansions contain no modulation opcode. A
    # changed macro cannot introduce one because linked byte comparison fails.
    modulators = {"entries": []}
    records, metadata = [], []
    for row in rows:
        source_path = source_dir / "sounds" / (row["name"] + ".aifc")
        source_bytes = source_path.read_bytes()
        staged_bytes = (donor_dir / "extra/build/extra_characters" / source_dir.name /
                        "sounds" / source_path.name).read_bytes()
        verify_resolved_inputs(manifest, source_dir.name,
                               {f"extra/build/extra_characters/{source_dir.name}/sounds/{source_path.name}": staged_bytes})
        if source_bytes != staged_bytes:
            raise AudioAdmissionError(f"donor changed source AIFC {source_path.name}")
        wave = parse_aifc(source_bytes)
        if wave.sample_rate != row["sample_rate"]:
            raise AudioAdmissionError(f"source/generated sample rate disagreement: {source_path.name}")
        raw_ucd, raw_art = microcode(row, wave)
        donor_ucd, donor_art = linked_microcode(
            donor_rom, main_bytes.decode("utf-8"), ids[row["name"]],
            raw_ucd, raw_art, decoder)
        program = decoder._disassemble_ucd_entry(raw_ucd)
        articulation = decoder._disassemble_entry(raw_art)
        # The source macro always emits two-byte indices; the isolated zero
        # index canonically re-encodes in one byte. Verify decoded semantics.
        if (decoder._disassemble_ucd_entry(decoder._assemble_ucd_entry(program)) != program or
                decoder._disassemble_entry(decoder._assemble_entry(articulation)) != articulation):
            raise AudioAdmissionError(f"source microcode roundtrip failed: {row['name']}")
        bank = {1: {"wavetable_off": 2, "samplePan": 0, "sampleVolume": 0},
                2: {"base": 0, "length": len(wave.vadpcm), "book_off": 3,
                    "loop_off": 0, "type": 0},
                3: {"order": wave.order, "npredictors": wave.predictors,
                    "entries": list(wave.book)}}
        pcm, audit = renderer.render_fgm_program_voice_aot(
            0, {"entries": [{"program": program}]},
            {"entries": [{"program": articulation}]}, modulators,
            {"soundArray_offs": [1]}, bank, wave.vadpcm, vadpcm_decode,
            sine_table, output_rate)
        ima = renderer.ima_encode(pcm)
        decoded = renderer.ima_decode(ima, len(pcm))
        metrics = renderer.audio_metrics(pcm, decoded)
        if metrics["ima_snr_db"] < 14:
            ima = renderer.ima_encode_lookahead(pcm)
            decoded = renderer.ima_decode(ima, len(pcm))
            metrics = renderer.audio_metrics(pcm, decoded)
        if (metrics["decoded_peak"] == 0 or metrics["decoded_rms"] <= 0 or
                metrics["ima_snr_db"] < 14):
            raise AudioAdmissionError(f"native conversion acoustic failure: {row['name']}: {metrics}")
        if len(ima) > renderer.MAX_CUE_IMA_BYTES:
            raise AudioAdmissionError(f"native cue exceeds cache capacity: {row['name']}")
        duration = len(pcm) // samples_per_tick
        if len(pcm) % samples_per_tick or duration != audit["duration_ticks"]:
            raise AudioAdmissionError("AOT sample extent disagrees with source notes")
        pause_with_game = any(op[0] == "set_unk1F" and op[1] & 0x80 for op in program)
        flags = 2 if pause_with_game else 0
        records.append({"id": ids[row["name"]], "flags": flags, "ima": ima,
                        "sample_count": len(pcm), "frequency": output_rate,
                        "duration_ticks": duration, "volume": 127, "pan": 64,
                        "sound": 0, "envelope": [], "loop_point_words": 0})
        metadata.append({
            "id": ids[row["name"]], "name": row["name"],
            "source_placeholder": next(key for key, name in placeholders.items() if name == row["name"]),
            "source_type": row["type"], "source_reverb": row["reverb"],
            "source_aifc_sha256": sha256(source_bytes),
            "source_aifc_bytes": wave.file_size, "source_form_size": wave.form_size,
            "source_form_size_matches_file": wave.form_size + 8 == wave.file_size,
            "source_comm_samples": wave.sample_count,
            "source_vadpcm_samples": len(wave.vadpcm) // 9 * 16,
            "source_ssnd_trailing_bytes": wave.trailing_sample_bytes,
            "source_ucd_bytes": raw_ucd.hex(), "source_articulation_bytes": raw_art.hex(),
            "linked_donor_ucd_bytes": donor_ucd.hex(),
            "linked_donor_articulation_bytes": donor_art.hex(),
            "linked_donor_ucd_program": decoder._disassemble_ucd_entry(donor_ucd),
            "linked_donor_articulation_program": decoder._disassemble_entry(donor_art),
            "source_ucd_program": program, "source_articulation_program": articulation,
            "source_duration_ticks": duration, "ds_frequency_hz": output_rate,
            "ds_sample_count": len(pcm), "ds_volume": 127, "ds_pan": 64,
            "ds_flags": flags, "pause_with_game": pause_with_game,
            "ima_adpcm_bytes": len(ima), "ima_adpcm_sha256": sha256(ima),
            "runtime_fidelity_debt": (["AL_FX_CUSTOM wet delay tail unqualified"]
                                      if audit["requires_custom_fx"] else []),
            "aot_pcm_sha256": renderer.ima_pcm_sha256(pcm),
            "acoustic_oracle": audit, **metrics,
        })
    identity = {"config_sha256": sha256(config_bytes),
                "generated_fgm_asm_sha256": sha256(asm_bytes),
                "appender_log_sha256": sha256(log_bytes),
                "linked_donor_sha256": sha256(donor_rom),
                "linked_main_asm_sha256": sha256(main_bytes),
                "resolved_donor_manifest_sha256": sha256(manifest_bytes),
                "source_sine_table_sha256": sha256(sine_bytes),
                "producer_sha256": sha256(Path(__file__).read_bytes()),
                "renderer_sha256": sha256((repo_root / "scripts/sfx/render-audio-fgm-phase-pack.py").read_bytes()),
                "vadpcm_decoder_sha256": sha256((repo_root / "scripts/sfx/vadpcm_decode.py").read_bytes()),
                "source_microcode_decoder_sha256": sha256((reference_root / "decomp/BattleShip-main/decomp/tools/extract_fgm.py").read_bytes())}
    return records, {"schema": "p4-extra-native-audio-v1", "character": source_dir.name,
                     "status": "CONVERTED_NOT_RUNTIME_QUALIFIED", "runtime_conversion": False,
                     "announcer_fgm": ids[placeholders[announcer]],
                     "crowd_chant_fgm": [entry["id"] for entry in metadata if entry["source_type"] == "CHANT"],
                     "placeholder_to_fgm": {key: ids[name] for key, name in placeholders.items()},
                     "sources": identity, "entries": metadata,
                     "checks_owed": ["natural CSS announcer and gameplay cue/channel/mix/PCM engagement",
                                     "source crowd behavior and sleep stopping/retrigger paths",
                                     "AL_FX_CUSTOM wet output and mix behavior",
                                     "full native match resource/cadence verification"]}


def unpack_records(pack: bytes) -> list[dict]:
    """Validate and recover native records without changing existing cue bytes."""
    if len(pack) < HEADER.size:
        raise AudioAdmissionError("short native FGM header")
    magic, version, count, size, _ = HEADER.unpack_from(pack)
    if magic != b"FGM1" or version != 4 or size != len(pack):
        raise AudioAdmissionError("invalid native FGM1-v4 header")
    data_start = HEADER.size + count * ENTRY.size
    if data_start > len(pack) or not count:
        raise AudioAdmissionError("invalid native FGM entry directory")
    result = []
    for index in range(count):
        (cue, flags, data_offset, data_bytes, samples, frequency, duration,
         volume, pan, sound, envelope_offset, envelope_count, loop) = ENTRY.unpack_from(pack, HEADER.size + index * ENTRY.size)
        if (not data_bytes or data_bytes % 4 or data_offset < data_start or
                data_offset + data_bytes > len(pack) or not samples or
                not frequency or not duration or volume > 127 or pan > 127):
            raise AudioAdmissionError(f"invalid native FGM record {index}")
        if envelope_count and (envelope_offset < data_start or envelope_offset + envelope_count * POINT.size > len(pack)):
            raise AudioAdmissionError("native FGM envelope outside pack")
        if not envelope_count and envelope_offset:
            raise AudioAdmissionError("native FGM empty envelope has an offset")
        envelope = [{"tick": tick, "ds_volume": level, "event_flags": event}
                    for tick, level, event in struct.iter_unpack(
                        POINT.format, pack[envelope_offset:envelope_offset + envelope_count * POINT.size])]
        result.append({"id": cue, "flags": flags, "ima": pack[data_offset:data_offset + data_bytes],
                       "sample_count": samples, "frequency": frequency,
                       "duration_ticks": duration, "volume": volume, "pan": pan,
                       "sound": sound, "envelope": envelope, "loop_point_words": loop})
    if len({record["id"] for record in result}) != len(result):
        raise AudioAdmissionError("duplicate native FGM IDs")
    sample_end = max(ENTRY.unpack_from(pack, HEADER.size + index * ENTRY.size)[2] +
                     ENTRY.unpack_from(pack, HEADER.size + index * ENTRY.size)[3] for index in range(count))
    envelope_cursor = sample_end
    for index in range(count):
        row = ENTRY.unpack_from(pack, HEADER.size + index * ENTRY.size)
        if row[11]:
            if row[10] != envelope_cursor:
                raise AudioAdmissionError("native FGM envelopes are not contiguous")
            envelope_cursor += row[11] * POINT.size
    if envelope_cursor != len(pack):
        raise AudioAdmissionError("native FGM pack tail unowned")
    return result


def serialize_records(records: list[dict], mapping_hash_lo: int) -> bytes:
    samples = bytearray()
    offsets: dict[bytes, int] = {}
    data_start = HEADER.size + ENTRY.size * len(records)
    sample_positions = []
    for record in records:
        ima = record["ima"]
        if ima not in offsets:
            offsets[ima] = data_start + len(samples)
            samples += ima
        sample_positions.append(offsets[ima])
    envelopes = bytearray()
    directory = bytearray()
    for record, offset in zip(records, sample_positions):
        envelope_offset = data_start + len(samples) + len(envelopes) if record["envelope"] else 0
        for point in record["envelope"]:
            envelopes += POINT.pack(point["tick"], point["ds_volume"], point.get("event_flags", 0))
        directory += ENTRY.pack(record["id"], record["flags"], offset, len(record["ima"]),
                                record["sample_count"], record["frequency"], record["duration_ticks"],
                                record["volume"], record["pan"], record["sound"], envelope_offset,
                                len(record["envelope"]), record["loop_point_words"])
    size = data_start + len(samples) + len(envelopes)
    pack = HEADER.pack(b"FGM1", 4, len(records), size, mapping_hash_lo) + directory + samples + envelopes
    unpack_records(pack)
    return bytes(pack)


def append_extension(base_pack: bytes, records: list[dict], metadata: dict) -> tuple[bytes, dict]:
    """Preserve all original samples/envelopes and append admitted fighter cues."""
    base_records = unpack_records(base_pack)
    if {row["id"] for row in base_records} & {row["id"] for row in records}:
        raise AudioAdmissionError("new fighter FGM identity collides with base pack")
    mapping = json.dumps({"base_pack_sha256": sha256(base_pack), "extension": metadata},
                         sort_keys=True, separators=(",", ":")).encode()
    mapping_hash = sha256(mapping)
    mapping_lo = int.from_bytes(bytes.fromhex(mapping_hash)[:4], "little")
    pack = serialize_records(base_records + records, mapping_lo)
    if unpack_records(pack)[:len(base_records)] != base_records:
        raise AssertionError("native audio extension changed original cue records")
    result = dict(metadata)
    result.update({"base_pack_sha256": sha256(base_pack), "base_entry_count": len(base_records),
                   "entry_count": len(base_records) + len(records), "pack_bytes": len(pack),
                   "pack_sha256": sha256(pack), "mapping_sha256": mapping_hash,
                   "mapping_sha256_lo": f"0x{mapping_lo:08x}",
                   "original_records_preserved": True,
                   "largest_extension_cue_bytes": max(len(row["ima"]) for row in records)})
    return pack, result


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo-root", type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument("--source-dir", type=Path, required=True)
    parser.add_argument("--reference-root", type=Path,
                        help="checkout holding the pinned BattleShip O2R audio inputs; read only")
    parser.add_argument("--donor-dir", type=Path, required=True)
    parser.add_argument("--base-pack", type=Path, required=True)
    parser.add_argument("--out-bin", type=Path, required=True)
    parser.add_argument("--out-json", type=Path, required=True)
    args = parser.parse_args()
    records, metadata = build_extension(args.repo_root.resolve(), args.source_dir.resolve(), args.donor_dir.resolve(),
                                        reference_root=args.reference_root.resolve() if args.reference_root else None)
    pack, metadata = append_extension(args.base_pack.read_bytes(), records, metadata)
    for output, data in ((args.out_bin, pack), (args.out_json, (json.dumps(metadata, indent=2, sort_keys=True) + "\n").encode())):
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_bytes(data)
    print(f"native EXTRA audio: {metadata['entry_count']} cues, {len(pack)} bytes, sha256={metadata['pack_sha256']}")
    print(f"runtime pins: entries={metadata['entry_count']}, bytes={len(pack)}, mapping={metadata['mapping_sha256_lo']}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
