#!/usr/bin/env python3
"""Build-time DS AnimClip pack for the P1 matchup, and prove it equivalent.

NATIVE BATTLE KERNEL, slice 1 (`docs/architecture/RUNTIME2_NATIVE_BATTLE_KERNEL.md`).

WHAT THIS IS NOT. It is not slice 32's dense bank
(`scripts/generate_ftanim_dense_bank.py`, `include/nds/nds_anim_dense.h`). That
one EXPANDS each script into resolved per-track write records and measured
4.46x the source -- proven correct, dead on size, and its verdict stands. This
pack keeps the command stream exactly as the runtime consumes it and deletes
the FILE architecture around it instead: no FAT read, no O2R header, no u32
byte swap, no threaded internal-fixup chain, no derived table bound, no u16 lane
unswap, no per-command bit re-encode, no absolute pointers, no loaded-file
registration, no status-node aliasing, no token->path discovery.

THE CONSUMER DECIDES THE FORMAT, and there is exactly one:
`ndsR2FtAnimParseDObjFigatree` (`src/import/battleship_ftanim.c:492`) walks
`root_dobj->anim_joint.event16` -- an `AObjEvent16 *` -- one u16 at a time,
reading `command.opcode/.flags/.toggle`, `->u` and `->s`. So a clip IS a u16
array in the native bitfield order, plus one entry point per animated DObj.
Everything else in the O2R file exists only to get those bytes into RAM.

  file (O2R)                     clip (this pack)
  ------------------------------ ---------------------------------------
  0x50 header + extern ids       gone (build time)
  u32 big-endian payload         native u16 stream (swapped at build time)
  threaded fixup chain           gone; the chain only ever produced the
                                 entry-point offsets, which are now a table
  absolute pointer table         u16 word offsets, position independent
  derived table bound            explicit script_count
  MSB-first command bits         native bitfield order, applied at build time

PACKED CHANNELS STAY PACKED. Nothing here converts a value. The stream is
copied word for word after the two transforms the ROM itself applies
(`ndsRelocApplyWordByteSwap` then `ndsRelocNormalizeFighterAObj16File`), so a
channel that packs bits in an f32 slot -- `ndsBaseGcPlayMObjMatAnim`'s five
0xRRGGBBAA tracks -- is byte-identical here whatever it means. This pack cannot
repeat the blanket-fixed-point mistake because it does no arithmetic at all.

DEDUPLICATION IS EXACT, NOT LOSSY. Identical script byte runs are stored once.
Two entry points that resolve to the same bytes produce the same u16 sequence,
so the consumer cannot tell -- and the runtime never writes through
`anim_joint.event16` (proven: every write in the parser targets the AObj, the
DObj or the GObj; see the immutability inventory in the architecture doc).

Usage:
  python scripts/generate_battlepack_anim.py --bank <dir> [--out pack.bin]
      [--verify] [--json report.json] [--no-dedup]
"""

from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
import pathlib
import struct
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
DEFAULT_BANK = (ROOT / "decomp" / "BattleShip-main" / "BattleShip_o2r"
                / "reloc_animations")

MAGIC = b"BPA1"
VERSION = 1

# reloc_backend_assets.c:3059-3074 -- these four carry AObjEvent32 scripts and
# are NOT part of an AObj16 pack. Named, never silently skipped.
AOBJ32_IDS = {
    0x279, 0x27A, 0x309, 0x30A,  # Mario/Fox entry effects
    0x3A4, 0x3A5,                # Donkey entry effects
    0x442, 0x443,                # Samus entry effects
    0x670, 0x671, 0x672, 0x673,  # Captain entry/Falcon Flyer effects
}

# nds_reloc_assets.c:138-141.
MARIO_FIRST, MARIO_LAST = 0x1F3, 0x281
FOX_FIRST, FOX_LAST = 0x282, 0x31F
DONKEY_FIRST, DONKEY_LAST = 0x320, 0x3B8
SAMUS_FIRST, SAMUS_LAST = 0x3B9, 0x44E
LUIGI_FIRST, LUIGI_LAST = 0x44F, 0x45A
CAPTAIN_FIRST, CAPTAIN_LAST = 0x5E8, 0x67F

# The item-flavoured clips, PROVEN excludable from the linked battle ELF rather
# than guessed from names: every function that can set an item status is a
# two-byte `bx lr` stub and the ELF holds no item spawner at all
# (BATTLEPACK_ANIMATION.md section 11.2). Items are off for P1 by contract.
ITEM_IDS = ([0x24C + i for i in range(18)] + [0x281] +
            [0x2DD + i for i in range(18)] + [0x31F])
FIGHTER_RANGES = {
    "mario": range(MARIO_FIRST, MARIO_LAST + 1),
    "fox": range(FOX_FIRST, FOX_LAST + 1),
    "donkey": range(DONKEY_FIRST, DONKEY_LAST + 1),
    "samus": range(SAMUS_FIRST, SAMUS_LAST + 1),
    "luigi": range(LUIGI_FIRST, LUIGI_LAST + 1),
    "captain": range(CAPTAIN_FIRST, CAPTAIN_LAST + 1),
}
FIGHTER_PREFIXES = {
    "mario": "FTMarioAnim",
    "fox": "FTFoxAnim",
    "donkey": "FTDonkeyAnim",
    "samus": "FTSamusAnim",
    "luigi": "FTLuigiAnim",
    "captain": "FTCaptainAnim",
}

CLIP_DIR = struct.Struct("<HHII")     # asset_id, script_count, byte_off, bytes
assert CLIP_DIR.size == 12


def _load(name):
    spec = importlib.util.spec_from_file_location(
        name, ROOT / "scripts" / ("%s.py" % name))
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


def asset_id_for(name: str, file_id: int) -> int:
    return file_id


def read_clip(probe, path):
    """One bank file -> (asset_id, [script byte-runs], accounting).

    Reproduces the ROM's pipeline verbatim through `ftanim_reloc_probe`, then
    cuts the normalized payload into one contiguous byte run per entry point.
    The run ends at the next entry point or the end of the payload -- the same
    bound `ndsRelocNormalizeFighterAObj16File` computes for its per-script
    walk (`reloc_backend_assets.c:3470-3481`).
    """
    raw = path.read_bytes()
    f = probe.load(raw)
    if f is None:
        return None
    if f["file_id"] in AOBJ32_IDS:
        return {"asset_id": f["file_id"], "aobj32": True, "file_bytes": len(raw),
                "payload_bytes": f["size"], "runs": [], "table_bytes": 0,
                "name": path.name}
    fx = probe.fixups(f)
    if not fx:
        return None
    table_bytes = probe.normalize(f, fx)
    entries = sorted(set(fx.values()))
    if len(entries) != len(fx):
        raise SystemExit(
            "%s has %d relocation slots sharing %d script targets. The ROM "
            "would normalize a shared script once per slot and pipeline 4c is "
            "not idempotent, so this file's encoding is ambiguous -- resolve "
            "it before packing." % (path.name, len(fx), len(entries)))
    bounds = entries[1:] + [f["size"]]
    # PIPELINE 4c, and leaving it out cost a data abort on 2026-08-15.
    # `probe.normalize` is 4a-4b only (table bound + u16 lane unswap); the ROM
    # then re-encodes every COMMAND word from disk order to the native bitfield
    # order its parser reads. The pack path skips `ndsRelocFinalizeLoadedFile`
    # entirely, so a pack that stores 4b bytes hands the parser `opcode = w &
    # 0x1f` of an MSB-first word -- garbage opcodes, an unterminated walk, and
    # the freeze class `reloc_backend_assets.c:2831` already documents.
    # The runtime normalizes each script exactly once, with word_count running
    # to the next-higher table target (`:3480-3521`); `bounds` is that same
    # quantity, and `len(entries) == len(fx)` on all 301 files, so no script is
    # ever encoded twice.
    for start, end in zip(entries, bounds):
        probe.renormalize_script(f["data"], start, (end - start) // 2)
    runs = [bytes(f["data"][s:e]) for s, e in zip(entries, bounds)]
    # The FIGATREE table, which is what `lbCommonAddFighterPartsFigatree` walks:
    # one word per DObj slot, in tree order, NOT one word per distinct script.
    # Its length is the derived bound (`normalize` returns the first script
    # offset), slots carrying a relocation name a script, and every other slot
    # is whatever the file already held -- reported below, never assumed zero.
    slot_count = table_bytes // 4
    slot_entry, slot_stray = [], 0
    for slot in range(slot_count):
        target = fx.get(slot * 4)
        if target is None:
            word = struct.unpack_from("<I", f["data"], slot * 4)[0]
            if word != 0:
                slot_stray += 1
            slot_entry.append(None)
        else:
            slot_entry.append(entries.index(target))
    return {"asset_id": f["file_id"], "aobj32": False, "name": path.name,
            "file_bytes": len(raw), "payload_bytes": f["size"],
            "table_bytes": table_bytes, "entries": entries, "runs": runs,
            "n_slots": len(fx), "slot_entry": slot_entry,
            "slot_stray": slot_stray}


def build(bank: pathlib.Path, dedup=True, exclude=(), fighter=None):
    """`exclude` is a set of asset ids to leave OUT of the resident pack.

    It exists to price a *split* resident pack (`plan.md` §K1 phase 3's third
    admissible route) without hand-arithmetic on per-clip means: dedup makes a
    subset's cost non-linear, so an exclusion must be re-packed to be priced.
    Every excluded id is reported in `skipped`, never dropped silently, and an
    id that is not in the bank is an error rather than a no-op.
    """
    exclude = frozenset(exclude)
    probe = _load("ftanim_reloc_probe")
    clips, skipped, seen = [], [], set()
    for path in sorted(bank.iterdir()):
        if not path.name.startswith(tuple(FIGHTER_PREFIXES.values())):
            continue
        if ((fighter is not None) and
                (not path.name.startswith(FIGHTER_PREFIXES[fighter]))):
            continue
        clip = read_clip(probe, path)
        if clip is None:
            skipped.append((path.name, "unreadable"))
            continue
        seen.add(clip["asset_id"])
        if clip["aobj32"]:
            skipped.append((clip["name"], "AObjEvent32 (0x%x)" % clip["asset_id"]))
            continue
        if clip["asset_id"] in exclude:
            skipped.append((clip["name"],
                            "excluded (0x%x)" % clip["asset_id"]))
            continue
        clips.append(clip)
    clips.sort(key=lambda c: c["asset_id"])
    missing = sorted(exclude - seen)
    if missing:
        raise SystemExit("--exclude-ids names %d id(s) absent from the bank: %s"
                         % (len(missing), ", ".join("0x%x" % i
                                                    for i in missing)))

    # Clip body = u16 script_count, u16 off[script_count] (word offsets into the
    # clip's own stream), then the streams. Word offsets keep the body
    # position-independent and 2-byte aligned; a clip is one contiguous run so a
    # playback touches one region.
    stream_pool = bytearray()
    pool_index = {}
    body = bytearray()
    directory = []
    dedup_hits = dedup_bytes = 0

    for clip in clips:
        runs = clip["runs"]
        local = bytearray()
        offs = []
        for run in runs:
            key = bytes(run)
            if dedup and key in pool_index:
                offs.append(pool_index[key])
                dedup_hits += 1
                dedup_bytes += len(run)
                continue
            off_words = len(stream_pool) // 2
            if dedup:
                pool_index[key] = off_words
            stream_pool += run
            offs.append(off_words)
        clip["offs"] = offs
        clip["local"] = local
        directory.append(clip)

    # With a shared pool the offsets are pool-global, so they need u32 word
    # offsets once the pool passes 128 KiB. Measured below and reported; the
    # table is emitted as u32 unconditionally so the format cannot silently
    # overflow when the bank grows.
    table = bytearray()
    for clip in directory:
        clip["table_off"] = len(table)
        for off in clip["offs"]:
            table += struct.pack("<I", off)

    header = struct.pack("<4sIIII", MAGIC, VERSION, len(directory),
                         len(table) // 4, len(stream_pool) // 2)
    dir_blob = b"".join(
        CLIP_DIR.pack(c["asset_id"], len(c["offs"]), c["table_off"],
                      sum(len(r) for r in c["runs"]))
        for c in directory)
    blob = header + dir_blob + bytes(table) + bytes(stream_pool)

    stats = {
        "bank": str(bank),
        "clips": len(directory),
        "skipped": skipped,
        "scripts": sum(len(c["offs"]) for c in directory),
        "raw_file_bytes": sum(c["file_bytes"] for c in clips),
        "raw_payload_bytes": sum(c["payload_bytes"] for c in clips),
        "raw_table_bytes": sum(c["table_bytes"] for c in clips),
        "raw_script_bytes": sum(c["payload_bytes"] - c["table_bytes"]
                                for c in clips),
        "pack_header_bytes": len(header),
        "pack_directory_bytes": len(dir_blob),
        "pack_offset_table_bytes": len(table),
        "pack_stream_bytes": len(stream_pool),
        "pack_total_bytes": len(blob),
        "dedup": bool(dedup),
        "dedup_runs_shared": dedup_hits,
        "dedup_bytes_saved": dedup_bytes,
        "sha256": hashlib.sha256(blob).hexdigest(),
    }
    return blob, directory, stream_pool, table, stats


BLOB_MAGIC = b"BPA2"
BLOB_VERSION = 2
BLOB_HEADER = struct.Struct("<4sIIIIIIIII")   # 40 B, padded to 48 below
BLOB_DIR = struct.Struct("<HHI")              # asset_id, slot_count, table_off
BLOB_DIR_OFF = 48
assert BLOB_HEADER.size == 40 and BLOB_DIR.size == 8


def emit_blob(directory):
    """One fighter's clips as a single position-independent resident blob.

    THE CONSUMER IS `lbCommonAddFighterPartsFigatree`
    (`src/port/reloc_backend_compat_shims.c:9044`), which walks ONE WORD PER
    DObj SLOT and hands each word to `ndsRelocResolvePointerFromFileBase`
    (`reloc_backend_assets.c:2850`). That resolver treats a word which is not
    already inside a known file as a BYTE OFFSET FROM THE CONTAINING FILE'S
    BASE, and refuses -- returns NULL, by design, after the 2026-08-02 shield
    freeze -- anything that resolves to an address that is not 4-byte aligned.

    Three consequences, and they are the whole format:
      * the table is per SLOT, not per distinct script (a clip has ~25 slots
        and ~19 distinct scripts; several DObjs share one script);
      * every stored word is a byte offset FROM THE BLOB BASE, so the blob is
        the "file" and one registration covers every clip in it;
      * every script start is padded to a 4-byte boundary, so the resolver's
        alignment refusal can never fire on a pack pointer.

    A slot with no relocation is emitted as 0, which `lbCommonAdd...` already
    treats as "this joint has no animation" (`anim_wait = AOBJ_ANIM_NULL`).
    `slot_stray` counts the slots where that discards a non-zero word; it is
    reported, not assumed.
    """
    stream = bytearray()
    pool = {}
    entry_off = []                    # per clip: [byte offset in stream region]
    for clip in directory:
        offs = []
        for run in clip["runs"]:
            key = bytes(run)
            if key in pool:
                offs.append(pool[key])
                continue
            while (len(stream) & 3) != 0:
                stream.append(0)
            pool[key] = len(stream)
            stream += run
            offs.append(pool[key])
        entry_off.append(offs)

    dir_bytes = BLOB_DIR.size * len(directory)
    table_off = BLOB_DIR_OFF + dir_bytes
    table_off = (table_off + 3) & ~3
    table_bytes = sum(len(c["slot_entry"]) * 4 for c in directory)
    stream_off = (table_off + table_bytes + 15) & ~15

    table = bytearray()
    dir_blob = bytearray()
    for clip, offs in zip(directory, entry_off):
        slot_table_off = table_off + len(table)
        dir_blob += BLOB_DIR.pack(clip["asset_id"], len(clip["slot_entry"]),
                                  slot_table_off)
        for idx in clip["slot_entry"]:
            table += struct.pack(
                "<I", 0 if idx is None else stream_off + offs[idx])
        clip["blob_slot_table_off"] = slot_table_off
        clip["blob_entry_off"] = [stream_off + o for o in offs]

    body = bytes(dir_blob) + bytes(b"\0" * (table_off - BLOB_DIR_OFF - dir_bytes))
    body += bytes(table)
    body += b"\0" * (stream_off - table_off - table_bytes)
    body += bytes(stream)
    while (len(body) & 15) != 0:
        body += b"\0"
    blob_bytes = BLOB_DIR_OFF + len(body)
    ids = sorted(c["asset_id"] for c in directory)
    checksum = 0
    for i in range(0, len(body) - 3, 4):
        checksum = (checksum + struct.unpack_from("<I", body, i)[0]) & 0xFFFFFFFF
    header = BLOB_HEADER.pack(BLOB_MAGIC, BLOB_VERSION, blob_bytes,
                              len(directory), BLOB_DIR_OFF, table_off,
                              stream_off, ids[0] if ids else 0,
                              ids[-1] if ids else 0, checksum)
    blob = header + b"\0" * (BLOB_DIR_OFF - len(header)) + body
    assert len(blob) == blob_bytes
    stats = {
        "blob_bytes": blob_bytes,
        "blob_clips": len(directory),
        "blob_dir_bytes": dir_bytes,
        "blob_slot_table_bytes": table_bytes,
        "blob_slots": table_bytes // 4,
        "blob_stream_bytes": len(stream),
        "blob_stray_slots": sum(c["slot_stray"] for c in directory),
        "blob_first_id": ids[0] if ids else 0,
        "blob_last_id": ids[-1] if ids else 0,
        "blob_checksum": checksum,
        "blob_sha256": hashlib.sha256(blob).hexdigest(),
    }
    return blob, stats


def verify_blob(bank: pathlib.Path, directory, blob):
    """Decode every SLOT out of the emitted blob and compare against the file.

    `verify()` above proves the deduped stream reproduces every distinct script.
    This proves the thing the runtime actually indexes: that slot i of clip c
    names the same script the file's relocation chain named for slot i, decoded
    from the blob's own bytes at the blob's own offsets, with the alignment the
    resolver demands.
    """
    probe = _load("ftanim_reloc_probe")
    mismatches = []
    checked = {"clips": 0, "slots": 0, "linked": 0, "null": 0, "commands": 0}
    for clip in directory:
        f = probe.load((bank / clip["name"]).read_bytes())
        fx = probe.fixups(f)
        probe.normalize(f, fx)
        checked["clips"] += 1
        table = clip["blob_slot_table_off"]
        for slot, idx in enumerate(clip["slot_entry"]):
            checked["slots"] += 1
            word = struct.unpack_from("<I", blob, table + slot * 4)[0]
            if idx is None:
                checked["null"] += 1
                if word != 0:
                    mismatches.append((clip["name"], slot, "expected NULL", word))
                continue
            checked["linked"] += 1
            if (word & 3) != 0 or word >= len(blob):
                mismatches.append((clip["name"], slot, "bad offset", word))
                continue
            try:
                cand = probe.decode_script(blob, len(blob), word, native=True)
            except ValueError as exc:
                mismatches.append((clip["name"], slot, "undecodable", str(exc)))
                continue
            oracle = probe.decode_script(f["data"], f["size"],
                                         clip["entries"][idx])
            o = [_cmd_key(c, oracle[0]["pc"]) for c in oracle]
            k = [_cmd_key(c, cand[0]["pc"]) for c in cand]
            checked["commands"] += len(o)
            if o != k:
                mismatches.append((clip["name"], slot, "command stream", ""))
    return mismatches, checked


STREAM_MAGIC = b"BPS1"
STREAM_VERSION = 1
STREAM_HEADER = struct.Struct("<4sIIIIIII")
STREAM_DIR = struct.Struct("<II")       # absolute file offset, clip bytes
assert STREAM_HEADER.size == 32 and STREAM_DIR.size == 8


def emit_stream_pack(directory):
    """Every source-normalized clip as one independently readable ROM range.

    Unlike BPA2 this pack is never resident. A miss reads exactly one compact
    clip into the fighter's existing figatree heap, registers that heap as the
    loaded-file range, and returns its slot table. Each slot word is a byte
    offset from the clip base, so no byte swap, threaded relocation or AObj16
    normalization remains at runtime. Scripts deduplicate only within a clip;
    cross-clip sharing would turn one acquisition back into scattered reads.
    """
    ids = sorted(c["asset_id"] for c in directory)
    if not ids:
        raise SystemExit("stream pack has no clips")
    first_id, last_id = ids[0], ids[-1]
    dense_count = last_id - first_id + 1
    dir_off = STREAM_HEADER.size
    data_off = (dir_off + dense_count * STREAM_DIR.size + 15) & ~15
    blob = bytearray(data_off)
    dense = [(0, 0)] * dense_count
    local_dedup_runs = local_dedup_bytes = 0
    payload_bytes = 0
    max_clip_bytes = 0

    for clip in directory:
        while (len(blob) & 15) != 0:
            blob.append(0)
        clip_off = len(blob)
        table_bytes = len(clip["slot_entry"]) * 4
        stream = bytearray()
        pool = {}
        run_offsets = []
        for run in clip["runs"]:
            key = bytes(run)
            if key in pool:
                run_offsets.append(pool[key])
                local_dedup_runs += 1
                local_dedup_bytes += len(run)
                continue
            while (len(stream) & 3) != 0:
                stream.append(0)
            offset = table_bytes + len(stream)
            pool[key] = offset
            run_offsets.append(offset)
            stream += run

        table = bytearray()
        for index in clip["slot_entry"]:
            table += struct.pack("<I", 0 if index is None else
                                 run_offsets[index])
        clip_blob = table + stream
        clip_bytes = len(clip_blob)
        if clip_bytes > clip["payload_bytes"]:
            raise SystemExit(
                "%s compact stream grew beyond its source heap: %d > %d" %
                (clip["name"], clip_bytes, clip["payload_bytes"]))
        blob += clip_blob
        dense[clip["asset_id"] - first_id] = (clip_off, clip_bytes)
        clip["stream_clip_off"] = clip_off
        clip["stream_clip_bytes"] = clip_bytes
        payload_bytes += clip_bytes
        max_clip_bytes = max(max_clip_bytes, clip_bytes)

    blob_bytes = len(blob)
    header = STREAM_HEADER.pack(
        STREAM_MAGIC, STREAM_VERSION, blob_bytes, len(directory), first_id,
        last_id, dir_off, data_off)
    blob[:STREAM_HEADER.size] = header
    for i, row in enumerate(dense):
        STREAM_DIR.pack_into(blob, dir_off + i * STREAM_DIR.size, *row)
    stats = {
        "stream_bytes": blob_bytes,
        "stream_clips": len(directory),
        "stream_first_id": first_id,
        "stream_last_id": last_id,
        "stream_dense_entries": dense_count,
        "stream_directory_bytes": dense_count * STREAM_DIR.size,
        "stream_payload_bytes": payload_bytes,
        "stream_max_clip_bytes": max_clip_bytes,
        "stream_local_dedup_runs": local_dedup_runs,
        "stream_local_dedup_bytes": local_dedup_bytes,
        "stream_sha256": hashlib.sha256(blob).hexdigest(),
    }
    return bytes(blob), stats


def verify_stream_pack(bank: pathlib.Path, directory, blob):
    """Decode every emitted slot at its final file offset against BattleShip."""
    probe = _load("ftanim_reloc_probe")
    mismatches = []
    checked = {"clips": 0, "slots": 0, "linked": 0, "null": 0,
               "commands": 0}
    fields = STREAM_HEADER.unpack_from(blob, 0)
    magic, version, blob_bytes, clip_count, first_id, last_id, dir_off, _ = fields
    if ((magic != STREAM_MAGIC) or (version != STREAM_VERSION) or
            (blob_bytes != len(blob)) or (clip_count != len(directory))):
        return [("header", fields)], checked

    for clip in directory:
        row = dir_off + (clip["asset_id"] - first_id) * STREAM_DIR.size
        clip_off, clip_bytes = STREAM_DIR.unpack_from(blob, row)
        if ((clip["asset_id"] > last_id) or
                (clip_off != clip["stream_clip_off"]) or
                (clip_bytes != clip["stream_clip_bytes"]) or
                (clip_off + clip_bytes > len(blob))):
            mismatches.append((clip["name"], "directory", clip_off,
                               clip_bytes))
            continue
        checked["clips"] += 1
        for slot, index in enumerate(clip["slot_entry"]):
            checked["slots"] += 1
            word = struct.unpack_from("<I", blob,
                                      clip_off + slot * 4)[0]
            if index is None:
                checked["null"] += 1
                if word != 0:
                    mismatches.append((clip["name"], slot,
                                       "expected NULL", word))
                continue
            checked["linked"] += 1
            if ((word & 3) != 0) or (word >= clip_bytes):
                mismatches.append((clip["name"], slot, "bad offset", word))
                continue
            try:
                cand = probe.decode_script(blob, clip_off + clip_bytes,
                                           clip_off + word, native=True)
            except ValueError as exc:
                mismatches.append((clip["name"], slot, "undecodable",
                                   str(exc)))
                continue
            source_run = clip["runs"][index]
            try:
                oracle = probe.decode_script(source_run, len(source_run), 0,
                                             native=True)
            except ValueError as exc:
                mismatches.append((clip["name"], slot,
                                   "source run undecodable", str(exc)))
                continue
            o = [_cmd_key(c, oracle[0]["pc"]) for c in oracle]
            k = [_cmd_key(c, cand[0]["pc"]) for c in cand]
            checked["commands"] += len(o)
            if o != k:
                mismatches.append((clip["name"], slot, "command stream", ""))
    return mismatches, checked


def verify(bank: pathlib.Path, directory, stream_pool, table):
    """Phase 4. Decode every clip OUT OF THE PACK and compare against the
    existing parser semantics, driven through slice 32's proven host model.

    The comparison is not "same bytes" -- that would be a tautology. Each side
    is DECODED: the oracle from the O2R file through the ROM's own pipeline, the
    candidate from the pack's stream through the same decoder, and both are then
    RUN through `ftanim_script_model` so duration, per-track key values,
    interpolation kind, joint/channel target, event ordering and end/loop
    behaviour are all compared as executed semantics, not as storage.
    """
    probe = _load("ftanim_reloc_probe")
    model = _load("ftanim_script_model")

    mismatches = []
    checked = {"clips": 0, "scripts": 0, "commands": 0, "states": 0,
               "callbacks": 0, "payload_words": 0}
    corpus = hashlib.sha256()

    for clip in directory:
        path = bank / clip["name"]
        oracle_scripts = probe.scripts_in(path)
        checked["clips"] += 1
        if len(oracle_scripts) != len(clip["offs"]):
            mismatches.append((clip["name"], "script count",
                               len(oracle_scripts), len(clip["offs"])))
            continue
        for ordinal, oracle_cmds in enumerate(oracle_scripts):
            off_words = struct.unpack_from("<I", table,
                                           clip["table_off"] + ordinal * 4)[0]
            where = "%s script %d" % (clip["name"], ordinal)
            checked["scripts"] += 1
            # A corrupt pack must produce a COUNTED mismatch, not a traceback a
            # caller could mistake for a broken tool. Proven by the two
            # falsifiers in BATTLEPACK_ANIMATION.md: a flipped stream byte and a
            # shifted offset both land here.
            try:
                cand_cmds = probe.decode_script(stream_pool, len(stream_pool),
                                                off_words * 2, native=True)
            except ValueError as exc:
                mismatches.append((where, "undecodable", str(exc), ""))
                continue

            # 1. the command stream itself: opcode, flags, payload bits, and
            #    every per-track target word, in order.
            o_norm = [_cmd_key(c, oracle_cmds[0]["pc"]) for c in oracle_cmds]
            c_norm = [_cmd_key(c, cand_cmds[0]["pc"]) for c in cand_cmds]
            checked["commands"] += len(o_norm)
            checked["payload_words"] += sum(
                len(c["targets"]) for c in oracle_cmds)
            if o_norm != c_norm:
                mismatches.append((where, "command stream",
                                   o_norm[:4], c_norm[:4]))
                continue

            # 2. the EXECUTED semantics: per-track state after every command,
            #    the wait timeline, and the callback (event) ordering.
            o_run = model.run_commands(oracle_cmds)
            c_run = model.run_commands(cand_cmds)
            # Callback entries carry the absolute PC of the command that fired
            # them, and the two sides sit at different base addresses BY
            # CONSTRUCTION -- that is the whole point of a position-independent
            # pack. Rebasing is what makes this a test of ORDERING and TAGS
            # rather than of storage addresses; without it every script
            # "mismatches" and the run says nothing.
            o_key = _run_key(o_run, oracle_cmds[0]["pc"])
            c_key = _run_key(c_run, cand_cmds[0]["pc"])
            checked["states"] += len(o_run.states)
            checked["callbacks"] += len(o_run.callbacks)
            if o_key != c_key:
                mismatches.append((where, "executed semantics", "see run", ""))
                continue
            corpus.update(("%s|" % where).encode())
            corpus.update(repr(o_key).encode())

    return mismatches, checked, corpus.hexdigest()


def _cmd_key(cmd, base):
    """A command, made comparable across two different base addresses."""
    return (cmd["op"], cmd["flags"], cmd["payload"],
            tuple(cmd["targets"]),
            None if cmd["jump"] is None else cmd["jump"] - base,
            cmd["cyclic"])


def _bits(v):
    if v is None or isinstance(v, (str, tuple, int)):
        return v
    return struct.pack("<d", float(v))


def _run_key(run, base):
    return ([(op, tuple(tuple(_bits(f) for f in snap) for snap in tracks))
             for _pc, op, tracks in run.states],
            [(pc - base, tag) for pc, tag in run.callbacks],
            _bits(run.anim_wait), run.flags,
            [_bits(w) for w in getattr(run, "waits", [])])


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--bank", type=pathlib.Path, default=DEFAULT_BANK)
    ap.add_argument("--out", type=pathlib.Path)
    ap.add_argument("--json", type=pathlib.Path)
    ap.add_argument("--verify", action="store_true")
    ap.add_argument("--no-dedup", action="store_true")
    ap.add_argument("--fighter", choices=sorted(FIGHTER_RANGES),
                    help="pack ONE fighter's clips; the resident pack is one "
                         "blob per fighter because the two RAM pools are "
                         "separate and neither holds both")
    ap.add_argument("--items-off", action="store_true",
                    help="drop the 38 item-flavoured clips proven unreachable "
                         "in the P1 battle ELF")
    ap.add_argument("--blob-out", type=pathlib.Path,
                    help="emit the resident BPA2 blob (per-slot figatree "
                         "tables, blob-relative byte offsets) to this path")
    ap.add_argument("--stream-out", type=pathlib.Path,
                    help="emit the direct-ROM BPS1 pack: one compact, "
                         "source-normalized range per clip")
    ap.add_argument("--exclude-ids", default="",
                    help="comma-separated asset ids (0x… or decimal) to leave "
                         "out of the resident pack; priced by re-packing, not "
                         "by subtracting a mean")
    args = ap.parse_args()

    exclude = set(int(tok, 0) for tok in args.exclude_ids.split(",")
                  if tok.strip())
    if args.items_off:
        exclude |= set(ITEM_IDS)
    if args.fighter:
        exclude &= set(FIGHTER_RANGES[args.fighter])
    exclude = frozenset(exclude)

    if not args.bank.is_dir():
        print("SKIP: %s absent" % args.bank)
        return 0

    blob, directory, pool, table, stats = build(args.bank,
                                                 dedup=not args.no_dedup,
                                                 exclude=exclude,
                                                 fighter=args.fighter)
    print("bank                    : %s" % stats["bank"])
    print("clips (AObj16)          : %d" % stats["clips"])
    print("scripts (entry points)  : %d" % stats["scripts"])
    print("raw file bytes          : %d" % stats["raw_file_bytes"])
    print("raw payload bytes       : %d" % stats["raw_payload_bytes"])
    print("  of which pointer table: %d" % stats["raw_table_bytes"])
    print("  of which script stream: %d" % stats["raw_script_bytes"])
    print("pack header             : %d" % stats["pack_header_bytes"])
    print("pack clip directory     : %d" % stats["pack_directory_bytes"])
    print("pack offset table       : %d" % stats["pack_offset_table_bytes"])
    print("pack stream             : %d" % stats["pack_stream_bytes"])
    print("PACK TOTAL              : %d bytes (%.1f%% of raw payload)"
          % (stats["pack_total_bytes"],
             100.0 * stats["pack_total_bytes"] / stats["raw_payload_bytes"]))
    print("identical script runs shared : %d (%d bytes saved)"
          % (stats["dedup_runs_shared"], stats["dedup_bytes_saved"]))
    print("sha256                  : %s" % stats["sha256"])
    for name, why in stats["skipped"]:
        print("   skipped %-18s %s" % (name, why))

    if args.verify:
        mismatches, checked, corpus = verify(args.bank, directory, pool, table)
        print("\nEQUIVALENCE (Phase 4)")
        print("  clips decoded        : %d" % checked["clips"])
        print("  scripts compared     : %d" % checked["scripts"])
        print("  commands compared    : %d" % checked["commands"])
        print("  per-track states     : %d" % checked["states"])
        print("  event callbacks      : %d" % checked["callbacks"])
        print("  target words         : %d" % checked["payload_words"])
        print("  corpus hash          : %s" % corpus)
        print("  MISMATCHES           : %d" % len(mismatches))
        for row in mismatches[:8]:
            print("     %s" % (row,))
        stats["verify"] = {"mismatches": len(mismatches), "corpus": corpus,
                           **checked}
        if mismatches:
            print("FAIL: the pack is not equivalent to the file pipeline.")
            if args.json:
                args.json.parent.mkdir(parents=True, exist_ok=True)
                args.json.write_text(json.dumps(stats, indent=1))
            return 1
        print("BATTLEPACK_ANIM_EQUIVALENCE=PASS  mismatch = 0")

    if args.blob_out:
        resident, bstats = emit_blob(directory)
        stats.update(bstats)
        print("\nRESIDENT BLOB (BPA2)")
        for key in ("blob_bytes", "blob_clips", "blob_slots",
                    "blob_dir_bytes", "blob_slot_table_bytes",
                    "blob_stream_bytes", "blob_stray_slots"):
            print("  %-22s : %d" % (key, bstats[key]))
        print("  %-22s : 0x%x .. 0x%x" % ("asset id range",
                                          bstats["blob_first_id"],
                                          bstats["blob_last_id"]))
        print("  %-22s : 0x%08x" % ("checksum", bstats["blob_checksum"]))
        print("  %-22s : %s" % ("sha256", bstats["blob_sha256"]))
        bad, checked = verify_blob(args.bank, directory, resident)
        print("  slots checked          : %d (%d linked, %d null), %d commands"
              % (checked["slots"], checked["linked"], checked["null"],
                 checked["commands"]))
        print("  SLOT MISMATCHES        : %d" % len(bad))
        for row in bad[:8]:
            print("     %s" % (row,))
        stats["verify_blob"] = {"mismatches": len(bad), **checked}
        if bad:
            print("FAIL: the resident blob does not reproduce the file's "
                  "slot -> script mapping.")
            return 1
        print("BATTLEPACK_BLOB_EQUIVALENCE=PASS  slot mismatch = 0")
        args.blob_out.parent.mkdir(parents=True, exist_ok=True)
        args.blob_out.write_bytes(resident)
        print("wrote %s (%d bytes)" % (args.blob_out, len(resident)))

    if args.stream_out:
        stream_blob, sstats = emit_stream_pack(directory)
        stats.update(sstats)
        print("\nDIRECT-ROM STREAM PACK (BPS1)")
        for key in ("stream_bytes", "stream_clips", "stream_dense_entries",
                    "stream_directory_bytes", "stream_payload_bytes",
                    "stream_max_clip_bytes", "stream_local_dedup_runs",
                    "stream_local_dedup_bytes"):
            print("  %-28s : %d" % (key, sstats[key]))
        print("  %-28s : 0x%x .. 0x%x" %
              ("asset id range", sstats["stream_first_id"],
               sstats["stream_last_id"]))
        print("  %-28s : %s" % ("sha256", sstats["stream_sha256"]))
        bad, checked = verify_stream_pack(args.bank, directory, stream_blob)
        print("  slots checked              : %d (%d linked, %d null), %d commands" %
              (checked["slots"], checked["linked"], checked["null"],
               checked["commands"]))
        print("  STREAM MISMATCHES          : %d" % len(bad))
        for row in bad[:8]:
            print("     %s" % (row,))
        stats["verify_stream"] = {"mismatches": len(bad), **checked}
        if bad:
            print("FAIL: direct-ROM stream pack differs from source clips.")
            return 1
        print("BATTLEPACK_STREAM_EQUIVALENCE=PASS  slot mismatch = 0")
        args.stream_out.parent.mkdir(parents=True, exist_ok=True)
        args.stream_out.write_bytes(stream_blob)
        print("wrote %s (%d bytes)" %
              (args.stream_out, len(stream_blob)))

    if args.out:
        args.out.parent.mkdir(parents=True, exist_ok=True)
        args.out.write_bytes(blob)
        print("\nwrote %s (%d bytes)" % (args.out, len(blob)))
    if args.json:
        args.json.parent.mkdir(parents=True, exist_ok=True)
        args.json.write_text(json.dumps(stats, indent=1))
    return 0


if __name__ == "__main__":
    sys.exit(main())
