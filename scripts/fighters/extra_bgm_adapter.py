#!/usr/bin/env python3
"""Convert Meta Knight's complete donor victory score into native BGA1 audio.

The source CSEQ and seven used instruments are qualified against the linked
donor before the existing BGM renderer/packet encoder consumes them. Playback
uses the current ARM7 BGM streaming service. No runtime sequence interpreter,
new player, donor-parent theme or generated reference write is introduced.
"""
from __future__ import annotations

import argparse
from collections import Counter
import json
from pathlib import Path
import re
import struct
import sys

import extra_audio_adapter as audio
import vadpcm_decode


class BgmAdmissionError(audio.AudioAdmissionError):
    pass


def u32(data: bytes, offset: int) -> int:
    if offset < 0 or offset + 4 > len(data):
        raise BgmAdmissionError("source music span outside donor")
    return struct.unpack_from(">I", data, offset)[0]


def rom_mapping(main: str) -> tuple[int, int]:
    match = re.search(r"origin\s+(0x[0-9a-fA-F]+)\s+base\s+(0x[0-9a-fA-f]+)", main)
    if match is None:
        raise BgmAdmissionError("missing appended donor address mapping")
    return int(match[1], 16), int(match[2], 16)


def resolve_score(rom: bytes, score: bytes) -> tuple[int, int]:
    """MIDI.asm's actual ROM table, including relocated sequence storage."""
    table = u32(rom, 0x3D768)
    if table + 4 > len(rom):
        raise BgmAdmissionError("music table outside donor")
    revision, count = struct.unpack_from(">HH", rom, table)
    if revision != 0x5331 or not 1 <= count < 0x8000:
        raise BgmAdmissionError("invalid donor music table")
    matches = []
    for index in range(count):
        relative, size = struct.unpack_from(">II", rom, table + 4 + index * 8)
        start = table + relative
        if size == len(score) and start + size <= len(rom) and rom[start:start + size] == score:
            matches.append((index, start))
    if len(matches) != 1:
        raise BgmAdmissionError("source victory score has no unique linked donor music ID")
    return matches[0]


def closure_offsets(by_offset: dict, instrument_offsets: set[int]) -> set[int]:
    """The complete ALInstrument -> sound/envelope/keymap/wave/book/loop graph."""
    result: set[int] = set()
    pending = list(instrument_offsets)
    while pending:
        offset = pending.pop()
        if offset in result:
            continue
        item = by_offset.get(offset)
        if item is None:
            raise BgmAdmissionError(f"missing reachable music object {offset:#x}")
        result.add(offset)
        kind = item["kind"]
        if kind == "ALInstrument":
            pending.extend(item["soundArray_offs"])
        elif kind == "ALSound":
            pending.extend(item[key] for key in ("env_off", "keyMap_off", "wavetable_off") if item[key])
        elif kind == "ALWaveTable":
            pending.extend(item[key] for key in ("loop_off", "book_off") if item[key])
        elif kind not in ("ALEnvelope", "ALKeyMap", "ALADPCMBook", "ALADPCMloop"):
            raise BgmAdmissionError(f"unsupported reachable music object {kind}")
    return result


def struct_bytes(item: dict, decoder) -> int:
    if item["kind"] == "ALInstrument":
        count = item["soundCount"]
        return 16 + count * 4 + (4 if count & 1 else 0)
    if item["kind"] == "ALADPCMBook":
        return decoder.book_alignment(len(item["entries"]) * 2)
    return decoder.SIZE_OF[item["kind"]][1]


def qualify_bank(rom: bytes, main: str, ctl: bytes, tbl: bytes,
                 bank: dict, by_offset: dict, programs: set[int], decoder) -> dict:
    """Prove reuse of the original instrument bank for every reached program.

    MIDI.asm relocates and extends the bank map, then retains original pointer
    identities for its first 43 instruments. If a used instrument/sample differs,
    fail rather than quietly synthesizing an unrelated original-bank instrument.
    """
    ctl_origin, tbl_origin = u32(rom, 0x3D75C), u32(rom, 0x3D760)
    relative = u32(rom, ctl_origin + 4)
    origin, base = rom_mapping(main)
    address = 0x800472D0 + relative  # MIDI.INST_CTL_TABLE_PC
    donor_bank = address - base + origin if address >= base else ctl_origin + relative
    if donor_bank + 12 > len(rom):
        raise BgmAdmissionError("donor music bank outside ROM")
    count, flags, pad, rate, percussion = struct.unpack_from(">hBBiI", rom, donor_bank)
    if count < len(bank["instArray_offs"]) or (flags, pad, rate, percussion) != (
            bank["flags"], bank["pad"], bank["sampleRate"], bank["percussion_off"]):
        raise BgmAdmissionError("donor original music bank contract changed")
    used = set()
    for program in programs:
        if not 0 <= program < len(bank["instArray_offs"]):
            raise BgmAdmissionError(f"unconverted added donor instrument {program}")
        original_offset = bank["instArray_offs"][program]
        if not original_offset or u32(rom, donor_bank + 12 + program * 4) != original_offset:
            raise BgmAdmissionError(f"donor music instrument mapping changed: {program}")
        used.add(original_offset)
    reached = closure_offsets(by_offset, used)
    waves = []
    for offset in sorted(reached):
        item = by_offset[offset]
        size = struct_bytes(item, decoder)
        actual = rom[ctl_origin + offset:ctl_origin + offset + size]
        expected = ctl[offset:offset + size]
        if len(actual) != size or actual != expected:
            raise BgmAdmissionError(f"donor music {item['kind']} changed at {offset:#x}")
        if item["kind"] == "ALWaveTable":
            if item["type"] != 0 or item["length"] <= 0 or not item["book_off"]:
                raise BgmAdmissionError("unconverted donor music wave")
            start, length = item["base"], item["length"]
            encoded = tbl[start:start + length]
            if len(encoded) != length or rom[tbl_origin + start:tbl_origin + start + length] != encoded:
                raise BgmAdmissionError(f"donor music sample bytes changed at {start:#x}")
            waves.append({"offset": offset, "base": start, "bytes": length,
                          "sha256": audio.sha256(encoded)})
    return {"donor_ctl_rom_offset": ctl_origin, "donor_tbl_rom_offset": tbl_origin,
            "donor_instrument_count": count, "original_instrument_count": bank["instCount"],
            "used_programs": sorted(programs), "used_instrument_offsets": sorted(used),
            "reachable_struct_count": len(reached), "reachable_kind_counts": dict(Counter(by_offset[o]["kind"] for o in reached)),
            "reachable_struct_sha256": audio.sha256(b"".join(ctl[o:o + struct_bytes(by_offset[o], decoder)] for o in sorted(reached))),
            "original_instrument_graph_and_samples_exact": True, "waves": waves}


def qualify_note_sound(renderer, decoder, by_offset: dict, bank: dict,
                       tbl: bytes, note: dict) -> int:
    sound = renderer.select_sound(decoder, by_offset, bank, note["program"], note["note"], note["velocity"])
    if sound is None:
        raise BgmAdmissionError("score note has no source sound")
    keymap = by_offset.get(sound["keyMap_off"])
    if (keymap is None or not keymap["keyMin"] <= note["note"] <= keymap["keyMax"] or
            not keymap["velocityMin"] <= note["velocity"] <= keymap["velocityMax"]):
        raise BgmAdmissionError("score note would use a fallback sound")
    if not sound["env_off"] or sound["env_off"] not in by_offset:
        raise BgmAdmissionError("score sound lacks its source envelope")
    decoded, _, _, _ = renderer.decode_wave(vadpcm_decode, by_offset, tbl, sound["wavetable_off"])
    if not decoded or not any(decoded):
        raise BgmAdmissionError("score sound has no audible source wave")
    return sound["offset"]


def qualify_controls(events: list, notes: list, by_offset: dict, bank: dict) -> dict:
    """Bound all source controls and the score's actual oscillator engagement."""
    counts = Counter()
    fx = []
    pressure = []
    for tick, _order, _track, event in events:
        if event[0] != "midi":
            continue
        _, status, key, value = event
        kind = status & 0xF0
        if kind == 0xB0:
            counts[key] += 1
            if key not in (7, 10, 20, 21, 22, 23, 6, 100, 101):
                raise BgmAdmissionError(f"unclassified victory score controller {key}")
            if key in (7, 10, 21) and tick != 0:
                raise BgmAdmissionError("victory score has live gain/pan automation needing conversion")
            if key in (22, 23):
                fx.append({"tick": tick, "channel": status & 15, "controller": key, "value": value})
        elif kind == 0xD0:
            pressure.append({"tick": tick, "channel": status & 15, "value": key})
        elif kind not in (0xC0, 0xE0):
            raise BgmAdmissionError(f"unclassified victory score MIDI event {kind:#x}")
    if any(row["value"] for row in pressure):
        raise BgmAdmissionError("nonzero channel pressure requires oscillator schedule conversion")
    instruments = [by_offset[bank["instArray_offs"][program]] for program in sorted({n["program"] for n in notes})]
    if any(inst["tremType"] for inst in instruments):
        raise BgmAdmissionError("victory score tremolo requires gain schedule conversion")
    if any(inst["vibType"] not in (0, 128) for inst in instruments):
        raise BgmAdmissionError("victory score initial vibrato ratio requires conversion")
    # n_env.c initializes channels through __n_initChanState (n_seqplayer.c),
    # setting unk_0x10 = 0. VIBRATO_SIN starts at unity, and its callback's
    # ((vibrato-1)*unk_0x10/127)+1 is exactly unity with this score's pressure.
    return {"controller_counts": {str(k): v for k, v in sorted(counts.items())},
            "channel_pressure_events": pressure, "effective_vibrato_modulation": 0,
            "source_vibrato_types": sorted({inst["vibType"] for inst in instruments}),
            "source_oscillator_neutral_proof": "__n_initChanState unk_0x10=0; no nonzero channel pressure; VIBRATO_SIN initial ratio1 and callbacks weighted by zero",
            "source_fx_controls": fx, "source_rpn_controls": "CC6/100/101 are ignored by the source __n_CSPHandleMIDIMsg default branch"}


def score_overrides(rom: bytes, symbols: str, main: str, music_id: int) -> dict:
    origin, base = rom_mapping(main)
    result = {}
    for name, width in (("priority_override_table", 4), ("bend_range_override_table", 4), ("master_volume_override_table", 1)):
        matches = re.findall(rf"^([0-9a-fA-F]{{8}}) MIDI\.{name}$", symbols, re.M)
        if len(matches) != 1:
            raise BgmAdmissionError(f"missing donor source {name}")
        offset = int(matches[0], 16) - base + origin + music_id * width
        value = u32(rom, offset) if width == 4 else rom[offset]
        result[name] = value
        if value:
            raise BgmAdmissionError(f"victory score has an unconverted source {name}")
    return result


def build_victory_bgm(repo_root: Path, reference_root: Path, source_dir: Path,
                      donor_dir: Path) -> tuple[bytes, dict]:
    renderer_path = repo_root / "scripts/sfx/bgm/render-audio-bgm.py"
    renderer = audio.load_module(renderer_path, "extra_bgm_renderer")
    tools = repo_root / "decomp/BattleShip-main/decomp/tools"
    cseq = audio.load_module(tools / "cseq_to_mid.py", "extra_bgm_cseq")
    decoder = audio.load_module(tools / "decode_ctl.py", "extra_bgm_ctl")
    score = (source_dir / "victory_theme.bin").read_bytes()
    staged_path = f"extra/build/extra_characters/{source_dir.name}/victory_theme.bin"
    staged = (donor_dir / staged_path).read_bytes()
    if staged != score:
        raise BgmAdmissionError("staged victory score differs from pinned source")
    rom = (donor_dir / "extra/ssb64asm_extra_review.z64").read_bytes()
    main = (donor_dir / "extra/main.asm").read_bytes()
    symbols = (donor_dir / "extra/review-symbols.log").read_bytes()
    midi_asm = (donor_dir / "extra/src/midi.asm").read_bytes()
    manifest_bytes = (donor_dir / "donor-manifest.json").read_bytes()
    audio.verify_resolved_inputs(json.loads(manifest_bytes), source_dir.name,
                                 {staged_path: staged, "extra/ssb64asm_extra_review.z64": rom,
                                  "extra/main.asm": main, "extra/review-symbols.log": symbols,
                                  "extra/src/midi.asm": midi_asm})
    music_id, score_offset = resolve_score(rom, score)
    overrides = score_overrides(rom, symbols.decode("ascii"), main.decode("utf-8"), music_id)
    audio_dir = reference_root / "decomp/BattleShip-main/BattleShip_o2r/audio"
    ctl = renderer.read_o2r_payload(audio_dir / renderer.BGM_SEQUENCE_BANK_CTL)
    tbl = renderer.read_o2r_payload(audio_dir / renderer.BGM_SEQUENCE_BANK_TBL)
    structs = decoder.walk(ctl)
    by_offset = {item["offset"]: item for item in structs}
    banks = [item for item in structs if item["kind"] == "ALBank"]
    if len(banks) != 1 or banks[0]["sampleRate"] != renderer.SOURCE_BANK_SAMPLE_RATE:
        raise BgmAdmissionError("unexpected original music bank")
    bank = banks[0]
    bend_timelines, bend_metadata = renderer.collect_pitch_bends(cseq, score, bank, by_offset)
    notes, tempo = renderer.collect_notes(cseq, score, renderer.SOURCE_BANK_SAMPLE_RATE, bend_timelines)
    events = renderer.iter_midi_events(cseq, score)
    note_on_count = sum(event[3][0] == "note_on" for event in events)
    note_off_count = sum(event[3][0] == "note_off" for event in events)
    if not notes or note_on_count != len(notes) or note_off_count != len(notes):
        raise BgmAdmissionError("victory score lost or unmatched source notes")
    loop = renderer.collect_loop_metadata(cseq, score, tempo, notes, renderer.SOURCE_BANK_SAMPLE_RATE)
    if loop["looping"]:
        raise BgmAdmissionError("source victory loop requires explicit lifecycle admission")
    bank_audit = qualify_bank(rom, main.decode("utf-8"), ctl, tbl, bank, by_offset,
                              {note["program"] for note in notes}, decoder)
    controls = qualify_controls(events, notes, by_offset, bank)
    sounds = sorted({qualify_note_sound(renderer, decoder, by_offset, bank, tbl, note) for note in notes})
    pcm = renderer.render(notes, decoder, vadpcm_decode, by_offset, bank, tbl,
                          renderer.DEFAULT_GAIN, renderer.SOURCE_BANK_SAMPLE_RATE)
    output_notes, _ = renderer.collect_notes(cseq, score, renderer.OUTPUT_SAMPLE_RATE, bend_timelines)
    target_samples = max(note["end"] for note in output_notes) + renderer.OUTPUT_SAMPLE_RATE
    pcm = renderer.bandlimited_resample_pcm16(pcm, renderer.SOURCE_BANK_SAMPLE_RATE,
                                             renderer.OUTPUT_SAMPLE_RATE, target_samples)
    payload, format_metadata = renderer.build_ima_packets(pcm, 0, False)
    samples = struct.unpack(f"<{len(pcm) // 2}h", pcm)
    if not any(samples) or format_metadata["ima_snr_db"] < 14:
        raise BgmAdmissionError("victory audio conversion failed acoustic engagement")
    identity = {"victory_score_sha256": audio.sha256(score), "victory_score_bytes": len(score),
                "linked_donor_sha256": audio.sha256(rom), "resolved_manifest_sha256": audio.sha256(manifest_bytes),
                "generated_midi_asm_sha256": audio.sha256(midi_asm),
                "source_ctl_sha256": audio.sha256(ctl), "source_tbl_sha256": audio.sha256(tbl),
                "producer_sha256": audio.sha256(Path(__file__).read_bytes()),
                "renderer_sha256": audio.sha256(renderer_path.read_bytes()),
                "cseq_decoder_sha256": audio.sha256((tools / "cseq_to_mid.py").read_bytes()),
                "ctl_decoder_sha256": audio.sha256((tools / "decode_ctl.py").read_bytes()),
                "vadpcm_decoder_sha256": audio.sha256((repo_root / "scripts/sfx/vadpcm_decode.py").read_bytes())}
    metadata = {"schema": "p4-extra-native-victory-bgm-v1", "character": source_dir.name,
                "status": "CONVERTED_NOT_RUNTIME_QUALIFIED", "runtime_conversion": False,
                "music_id": music_id, "linked_score_rom_offset": score_offset,
                "note_count": len(notes), "source_note_on_count": note_on_count,
                "source_note_off_count": note_off_count,
                "channels": sorted({note["channel"] for note in notes}),
                "tempo_us_per_quarter": tempo, "source_division": u32(score, 64),
                "source_overrides": overrides, "source_bank": bank_audit, "source_controls": controls,
                "used_sound_offsets": sounds, "sources": identity, "gain": renderer.DEFAULT_GAIN,
                "sample_rate": renderer.OUTPUT_SAMPLE_RATE, "mix_sample_rate": renderer.SOURCE_BANK_SAMPLE_RATE,
                "source_pcm_bytes": len(pcm), "source_pcm_sha256": audio.sha256(pcm),
                "bytes": len(payload), "sha256": audio.sha256(payload),
                "looping": False, "loop_start_byte": 0,
                "native_path": "audio/bgm_win_meta_knight_ima.bin",
                "runtime_fidelity_debt": ["mono completed mix through the existing BGM owner", "source custom FX wet output remains unqualified"],
                "checks_owed": ["natural Meta Knight win/Results engagement", "ARM7 packet progress, finite completion and return to Results music", "source PCM/listen comparison and cadence/resource qualification"],
                **bend_metadata, **format_metadata}
    return payload, metadata


def runtime_header(metadata: dict) -> str:
    return ("/* Generated by extra_bgm_adapter.py; source-qualified BGA1 stream. */\n"
            "#ifndef NDS_P4_BGM_GENERATED_H\n#define NDS_P4_BGM_GENERATED_H\n"
            f"#define NDS_P4_METAKNIGHT_VICTORY_BGM {metadata['music_id']}u\n"
            f"#define NDS_P4_METAKNIGHT_VICTORY_STREAM_BYTES {metadata['source_pcm_bytes']}u\n"
            f"#define NDS_P4_METAKNIGHT_VICTORY_ASSET_BYTES {metadata['bytes']}u\n"
            f"#define NDS_P4_METAKNIGHT_VICTORY_PACKET_COUNT {metadata['packet_count']}u\n"
            "#endif\n")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo-root", type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument("--reference-root", type=Path, required=True)
    parser.add_argument("--source-dir", type=Path, required=True)
    parser.add_argument("--donor-dir", type=Path, required=True)
    parser.add_argument("--out-bin", type=Path, required=True)
    parser.add_argument("--out-json", type=Path, required=True)
    parser.add_argument("--out-header", type=Path, required=True)
    args = parser.parse_args()
    payload, metadata = build_victory_bgm(args.repo_root.resolve(), args.reference_root.resolve(),
                                         args.source_dir.resolve(), args.donor_dir.resolve())
    for output, data in ((args.out_bin, payload),
                         (args.out_json, (json.dumps(metadata, indent=2, sort_keys=True) + "\n").encode()),
                         (args.out_header, runtime_header(metadata).encode())):
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_bytes(data)
    print(f"Meta victory BGM{metadata['music_id']}: {metadata['note_count']} notes, {metadata['bytes']} B, sha256={metadata['sha256']}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
