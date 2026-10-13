#!/usr/bin/env python3
"""P6 authored export: a base content's export plus an authored content's spec.

An authored content (scripts/p4/contents.json `base`, docs/P6/P6_PLAN.md A7)
has no Remix donor to export. Its export directory is written here, in the
layout scripts/p4/remix_export.py writes (resolved.json, events.json,
summary.txt), from its base content's export and its committed spec
(scripts/p6/<name>/spec.json). Every P4 rule after the export then runs on it
unchanged.

The base's data stays the base's: its files, clips, scripts and tables keep
their ids, so the authored content shares them with the base in a match. The
export gains the content's identity (`content`, `base`) and, from the spec,
what the authored content changes. An empty spec, or none, reproduces the base
export but for identity (F4): the A1 null clone.

    p6_author.py --base-export builds/p4-staging/export-x/roy --content ike \\
        --out builds/p4-staging/export-x/ike [--spec scripts/p6/ike/spec.json]

The output derives from the user's ROM through the base export: build output
only, never tracked.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import struct
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[1]
sys.path.insert(0, str(HERE.parent / "p4"))
import p4_contents  # noqa: E402

# Spec keys the author applies (A7 adds file substitutions, appended motions
# and statuses, kind-table rows, attribute deltas, sword trails and colour
# animations). Identity keys only describe the spec.
SPEC_IDENTITY = {"schema", "content", "comment"}
SPEC_SCHEMA = "p6-spec-v1"
# Spec keys this author applies.
SPEC_APPLIED = {"files", "motions"}
# "files": the content's own main and model file (model_writer.py), by role:
# the descriptor slot each replaces.
FILE_ROLES = {"main": 0, "model": 3}
# New file ids live in the P6 window (P6_PLAN.md A1 record: 0x1A00-0x1BFF),
# 0x100 a content: its main and model first, its clips from ANIM_FIRST.
P6_FILE_WINDOW = range(0x1A00, 0x1C00)
CONTENT_WINDOW = {"ike": 0x1A00, "palutena": 0x1B00}
ANIM_FIRST = 0x20
# Pointer commands' edges in events.json (remix_export.py's kinds).
EDGE_KIND = {"Goto": "goto", "Subroutine": "call", "SetParallelScript": "parallel",
             "SetDamageThrown": "damage-thrown"}


class AuthorError(SystemExit):
    pass


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def load_spec(path: Path | None, name: str) -> tuple[dict, str | None]:
    """The content's spec ({} when it has none) and its digest."""
    if path is None or not path.exists():
        return {}, None
    spec = json.loads(path.read_text(encoding="utf-8"))
    if spec.get("schema") != SPEC_SCHEMA:
        raise AuthorError(f"{path}: schema {spec.get('schema')!r}, expected {SPEC_SCHEMA}")
    if spec.get("content", name) != name:
        raise AuthorError(f"{path}: spec of {spec['content']!r}, not {name!r}")
    unknown = sorted(set(spec) - SPEC_IDENTITY - SPEC_APPLIED)
    if unknown:
        raise AuthorError(f"{path}: spec keys {unknown} need the authored export (A7)")
    return spec, sha256(path)


def author(base_export: Path, name: str, spec_path: Path | None, out: Path) -> dict:
    registry = p4_contents.load_registry()
    row = p4_contents.row_of(name, registry)
    if "base" not in row:
        raise AuthorError(f"{name}: not an authored content (no base in contents.json)")
    base = p4_contents.row_of(row["base"], registry)
    resolved_path = base_export / "resolved.json"
    if not resolved_path.exists():
        raise AuthorError(f"{resolved_path}: no export of the base {base['name']}")
    resolved = json.loads(resolved_path.read_text(encoding="utf-8"))
    if resolved.get("fighter") != base["remix"] or "content" in resolved:
        raise AuthorError(f"{base_export}: not {base['name']}'s Remix export")
    spec, spec_sha = load_spec(spec_path, name)

    resolved["content"] = name
    resolved["base"] = base["name"]
    resolved["authored"] = {
        "base_resolved_sha256": sha256(resolved_path),
        "base_events_sha256": sha256(base_export / "events.json"),
        "spec_sha256": spec_sha,
    }
    out.mkdir(parents=True, exist_ok=True)
    substitute_files(resolved, spec.get("files") or {}, out)
    events = json.loads((base_export / "events.json").read_text(encoding="utf-8"))
    author_motions(resolved, events, spec.get("motions") or {}, name, out)
    (out / "events.json").write_text(json.dumps(events, indent=1) + "\n", encoding="utf-8", newline="\n")
    summary = base_export / "summary.txt"
    text = summary.read_text(encoding="utf-8") if summary.exists() else ""
    (out / "summary.txt").write_text(
        f"authored content {name} (P6): base {base['name']}'s export, "
        f"spec {'none' if spec_sha is None else spec_sha[:12]}\n" + text,
        encoding="utf-8", newline="\n")
    # resolved.json last: the Makefile's target, so a stopped run leaves none.
    (out / "resolved.json").write_text(json.dumps(resolved, indent=1) + "\n",
                                       encoding="utf-8", newline="\n")
    return {"content": name, "base": base["name"], "spec": spec_sha, "keys": sorted(spec)}


def substitute_files(resolved: dict, files: dict, out: Path) -> None:
    """The spec's own files replace the base's in the descriptor and the
    file closure: each copied into <out>/o2r under its new id, the closure
    row rebuilt from it and every other row's references renamed. The files
    are build outputs (builds/p6-art/<name>/), never tracked (D10)."""
    if not files:
        return
    desc = resolved["descriptor"]
    mapping = {}
    o2r = out / "o2r"
    o2r.mkdir(parents=True, exist_ok=True)
    rows = {}
    for role, entry in sorted(files.items()):
        if role not in FILE_ROLES:
            raise AuthorError(f"spec files: unknown role {role!r} (one of {sorted(FILE_ROLES)})")
        old = desc["file_ids"][FILE_ROLES[role]]
        new = int(entry["id"], 0)
        if new not in P6_FILE_WINDOW:
            raise AuthorError(f"spec files {role}: id {new:#x} outside the P6 window")
        src = REPO / entry["from"]
        if not src.exists():
            raise AuthorError(f"spec files {role}: {src} not built (the look's writer makes it)")
        blob = src.read_bytes()
        if blob[4:8] != b"OLER" or struct.unpack_from("<I", blob, 0x40)[0] != new:
            raise AuthorError(f"spec files {role}: {src} is not an O2R container of file {new:#x}")
        n = struct.unpack_from("<I", blob, 0x48)[0]
        externs = list(struct.unpack_from(f"<{n}H", blob, 0x4C))
        size = struct.unpack_from("<I", blob, 0x4C + 2 * n)[0]
        (o2r / f"{new:04x}").write_bytes(blob)
        mapping[old] = new
        rows[old] = {"file_id": new, "vanilla_id": False, "bytes": size, "externs": externs}
        resolved["authored"].setdefault("files", {})[role] = [old, new]
        resolved["authored"].setdefault("files_sha256", {})[role] = hashlib.sha256(blob).hexdigest()
    desc["file_ids"] = [mapping.get(x, x) for x in desc["file_ids"]]
    closure = []
    for row in resolved["file_closure"]:
        if row["file_id"] in rows:
            closure.append(rows[row["file_id"]])
            continue
        if any(x in mapping for x in row.get("externs", ())):
            raise AuthorError(f"file {row['file_id']:#x} names a replaced file; it would need its own copy")
        closure.append(row)
    resolved["file_closure"] = closure


def author_motions(resolved: dict, events: dict, motions: dict, name: str, out: Path) -> None:
    """The spec's motions (index -> {"anim", "anim_flags", "script"}):
    - "anim": a built clip (an O2R container under builds/, anim.py), copied
      into <out>/o2r under the next id of the content's window and added to
      the file closure;
    - "script": a committed motion script (mscript.py text, one command a
      line, "#" comments), assembled into the authored RAM window
      (remix_rom.AUTHORED_RAM, <out>/authored_ram.bin) and listed in
      events.json as a block the generator copies like Remix's own; pointer
      targets name Remix blocks (ram:/file:) or NULL.
    The motion row takes the new clip and script; the generator reads both
    through OverlayRom."""
    if not motions:
        return
    import mscript
    from remix_rom import AUTHORED_RAM
    if name not in CONTENT_WINDOW:
        raise AuthorError(f"{name}: no P6 file window")
    window = CONTENT_WINDOW[name]
    o2r = out / "o2r"
    o2r.mkdir(parents=True, exist_ok=True)
    rows = {m["index"]: m for m in resolved["motions"]}
    authored = resolved["authored"]
    ram = bytearray()
    next_id = window + ANIM_FIRST
    for key in sorted(motions, key=lambda k: int(k, 0)):
        entry = motions[key]
        index = int(key, 0)
        row = rows.get(index)
        if row is None:
            raise AuthorError(f"spec motions: no motion {index} in the base")
        unknown = set(entry) - {"anim", "anim_flags", "script", "comment"}
        if unknown:
            raise AuthorError(f"spec motions {index}: unknown keys {sorted(unknown)}")
        if "anim" in entry:
            src = REPO / entry["anim"]
            if not src.exists():
                raise AuthorError(f"spec motions {index}: {src} not built (anim.py makes it)")
            blob = bytearray(src.read_bytes())
            if blob[4:8] != b"OLER":
                raise AuthorError(f"spec motions {index}: {src} is not an O2R container")
            fid = next_id
            next_id += 1
            if fid >= window + 0x100:
                raise AuthorError(f"{name}: more clips than its file window holds")
            struct.pack_into("<I", blob, 0x40, fid)
            (o2r / f"{fid:04x}").write_bytes(bytes(blob))
            n = struct.unpack_from("<I", blob, 0x48)[0]
            size = struct.unpack_from("<I", blob, 0x4C + 2 * n)[0]
            resolved["file_closure"].append({"file_id": fid, "vanilla_id": False, "bytes": size, "externs": []})
            row["anim_file_id"] = fid
            row["inherited"]["anim_file_id"] = False
            authored.setdefault("anims", {})[str(index)] = fid
            authored.setdefault("anims_sha256", {})[str(index)] = hashlib.sha256(bytes(blob)).hexdigest()
        if "anim_flags" in entry:
            row["anim_flags"] = int(str(entry["anim_flags"]), 0)
            row["inherited"]["anim_flags"] = False
        if "script" in entry:
            src = REPO / entry["script"]
            lines = [ln.split("#", 1)[0].strip() for ln in src.read_text(encoding="utf-8").splitlines()]
            lines = [ln for ln in lines if ln]
            start = AUTHORED_RAM + len(ram)
            block_key = f"ram:{start:#010x}"
            cmds, edges, exit_ = [], [], None
            for line in lines:
                words, pointers = mscript.assemble(line)
                at = AUTHORED_RAM + len(ram)
                op = line.split()[0]
                rec = {"at": f"ram:{at:#010x}", "op": op, "words": [words[0]]}
                for target in pointers.values():
                    if target == "NULL":
                        rec["to"] = None
                        rec["null"] = True
                    else:
                        rec["to"] = target
                        if op in EDGE_KIND:
                            edges.append({"kind": EDGE_KIND[op], "to": target})
                cmds.append(rec)
                ram += b"".join(struct.pack(">I", w) for w in words)
                if op in ("End", "Return", "Goto"):
                    exit_ = op.lower()
                    break
            if exit_ is None:
                raise AuthorError(f"spec motions {index}: {src} does not end (End, Return or Goto)")
            events["blocks"][block_key] = {"start": block_key, "label": f"{name}.motion{index:#x}",
                                           "referrers": [f"motion:{index:#x}"], "commands": cmds,
                                           "exit": exit_, "edges": edges}
            row["script"] = start
            row["inherited"]["script"] = False
            row["script_root"] = block_key
            authored.setdefault("scripts_sha256", {})[str(index)] = sha256(src)
    if ram:
        (out / "authored_ram.bin").write_bytes(bytes(ram))
        authored["ram"] = {"base": AUTHORED_RAM, "file": "authored_ram.bin", "bytes": len(ram),
                           "sha256": hashlib.sha256(bytes(ram)).hexdigest()}


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--base-export", type=Path, required=True,
                    help="the base content's remix_export.py directory")
    ap.add_argument("--content", required=True, help="the authored content's registry name")
    ap.add_argument("--spec", type=Path, help="its spec (default scripts/p6/<name>/spec.json)")
    ap.add_argument("--out", type=Path, required=True)
    args = ap.parse_args()
    spec = args.spec if args.spec is not None else HERE / args.content / "spec.json"
    done = author(args.base_export, args.content, spec, args.out)
    print(f"p6 author: {done['content']} from {done['base']}, spec "
          f"{'none' if done['spec'] is None else done['spec'][:12]} -> {args.out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
