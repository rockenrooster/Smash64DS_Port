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
AOBJ32_IDS = {0x279, 0x27A, 0x309, 0x30A}

# nds_reloc_assets.c:138-141.
MARIO_FIRST, MARIO_LAST = 0x1F3, 0x281
FOX_FIRST, FOX_LAST = 0x282, 0x31F

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
    bounds = entries[1:] + [f["size"]]
    runs = [bytes(f["data"][s:e]) for s, e in zip(entries, bounds)]
    return {"asset_id": f["file_id"], "aobj32": False, "name": path.name,
            "file_bytes": len(raw), "payload_bytes": f["size"],
            "table_bytes": table_bytes, "entries": entries, "runs": runs,
            "n_slots": len(fx)}


def build(bank: pathlib.Path, dedup=True, exclude=()):
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
        if not path.name.startswith(("FTMarioAnim", "FTFoxAnim")):
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
                                                off_words * 2)
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
    ap.add_argument("--exclude-ids", default="",
                    help="comma-separated asset ids (0x… or decimal) to leave "
                         "out of the resident pack; priced by re-packing, not "
                         "by subtracting a mean")
    args = ap.parse_args()

    exclude = frozenset(int(tok, 0) for tok in args.exclude_ids.split(",")
                        if tok.strip())

    if not args.bank.is_dir():
        print("SKIP: %s absent" % args.bank)
        return 0

    blob, directory, pool, table, stats = build(args.bank,
                                                dedup=not args.no_dedup,
                                                exclude=exclude)
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
