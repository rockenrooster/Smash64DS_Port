#!/usr/bin/env python3
"""P4 content list: the build's compiled contents as C rows.

Reads the registry (scripts/p4/contents.json) and each enabled content's
export, and writes into the build-local P4 directory:

- nds_p4_contents.generated.h: NDS_P4_CONTENT_ROWS(X), one row per enabled
  content, X(id, Title, NAME, name, parent_kind, model_file_id, main_file_id).
  Every per-content site in shared code (content rows, renderer owner slots,
  native-owner tables, profile owners, the character select's preview kinds)
  expands this list instead of naming a fighter. Beside it, the code each
  content runs (code_row): NDS_P4_CODE_<NAME> for every source file
  src/port/nds_p4_<name>.c this build compiles, and NDS_P4_CODE_ROWS(X),
  X(id, Title, code_id, CodeTitle).
- nds_p4_contents_data.generated.inc: one #include per content of its
  generated data (scripts/p4/generate_p4_fighter.py), for the single data TU
  src/port/nds_p4_data.c. The generated symbols carry the content's title, so
  the contents share one translation unit.

Nothing here derives from ROM bytes beyond the export's file ids.
"""
from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
REGISTRY = HERE / "contents.json"
PORT_DIR = HERE.parents[1] / "src" / "port"

# Remix define_character parents (the vanilla kinds) -> FTKind enumerators.
PARENT_KINDS = {
    "MARIO": "nFTKindMario", "FOX": "nFTKindFox", "DONKEY": "nFTKindDonkey",
    "SAMUS": "nFTKindSamus", "LUIGI": "nFTKindLuigi", "LINK": "nFTKindLink",
    "YOSHI": "nFTKindYoshi", "CAPTAIN": "nFTKindCaptain", "KIRBY": "nFTKindKirby",
    "PIKACHU": "nFTKindPikachu", "JIGGLY": "nFTKindPurin", "JIGGLYPUFF": "nFTKindPurin",
    "NESS": "nFTKindNess",
}


def load_registry(path: Path = REGISTRY) -> list[dict]:
    rows = json.loads(path.read_text(encoding="utf-8"))["contents"]
    ids, names = set(), set()
    for row in rows:
        if row["id"] in ids or row["name"] in names:
            raise SystemExit(f"{path}: duplicate content {row}")
        if not (1 <= row["id"] <= 20):
            raise SystemExit(f"{path}: content id {row['id']} out of range 1..20")
        if ("remix" in row) == ("base" in row):
            raise SystemExit(f"{path}: {row['name']} needs one of remix (a Remix "
                             "fighter) or base (an authored content)")
        ids.add(row["id"])
        names.add(row["name"])
    by = {row["name"]: row for row in rows}
    for row in rows:
        if "base" in row and "remix" not in by.get(row["base"], {}):
            raise SystemExit(f"{path}: {row['name']}: base {row['base']!r} is not a Remix content")
    return rows


def row_of(name: str, registry: list[dict] | None = None) -> dict:
    registry = registry if registry is not None else load_registry()
    for row in registry:
        if row["name"] == name:
            return row
    raise SystemExit(f"unknown P4 content {name!r} (scripts/p4/contents.json)")


def source_row(row: dict, registry: list[dict] | None = None) -> dict:
    """The Remix content whose donor data a content starts from: itself, or an
    authored content's base (P6). Its Remix name keys the donor's tables (its
    sounds, its select-screen art, its Kirby power) until the authored
    content's spec gives its own."""
    return row_of(row["base"], registry) if "base" in row else row


def table_key(name: str, registry: list[dict] | None = None) -> str:
    """The name the build scripts' per-content tables (special files, entry
    ports, articles, colour animations) are keyed by: an authored content's
    export starts as its base's, so it takes the base's rows."""
    return source_row(row_of(name, registry), registry)["name"]


def token(row: dict) -> str:
    """The content's selection token: its Remix name, or NAME for an authored
    content (the character select's cell, the UI kit's gate art)."""
    return row.get("remix", row["name"].upper())


def code_row(row: dict, registry: list[dict] | None = None) -> dict:
    """The content whose code a content runs: its own (src/port/nds_p4_<name>.c,
    or none), or, for an authored content without a source file of its own,
    its base's: its routines, hooks and overrides, as the base runs them."""
    if "base" in row and not (PORT_DIR / f"nds_p4_{row['name']}.c").exists():
        return row_of(row["base"], registry)
    return row


def enabled_rows(names: list[str], registry: list[dict] | None = None) -> list[dict]:
    """The registry rows for `names`, in registry order (image slots follow it)."""
    registry = registry if registry is not None else load_registry()
    known = {r["name"]: r for r in registry}
    for name in names:
        if name not in known:
            raise SystemExit(f"unknown P4 content {name!r} (scripts/p4/contents.json)")
    return [r for r in registry if r["name"] in names]


def special_storage(export_root: Path, name: str, slot: int,
                    registry: list[dict] | None = None) -> str:
    """The global an own special file `slot` (1-4) of a content loads into
    (generate_p4_fighter.py OWN_SPECIAL_FILES, p4_articles.py's roots):
    gNdsP4<Title>Special<slot>. An authored content whose export holds its
    base's file in the slot shares the base's global: the base's ported code,
    which it runs, reads the file there."""
    row = row_of(name, registry)
    owner = row
    if "base" in row:
        base = row_of(row["base"], registry)

        def ids(n: str) -> list[int]:
            resolved = json.loads((export_root / n / "resolved.json").read_text(encoding="utf-8"))
            return resolved["descriptor"]["file_ids"]

        if ids(name)[4 + slot] == ids(base["name"])[4 + slot]:
            owner = base
    return f"gNdsP4{owner['title']}Special{slot}"


def export_facts(export_root: Path, name: str) -> dict:
    resolved = json.loads((export_root / name / "resolved.json").read_text(encoding="utf-8"))
    files = resolved["descriptor"]["file_ids"]
    parent = resolved["parent"]
    if parent not in PARENT_KINDS:
        raise SystemExit(f"{name}: unsupported parent {parent}")
    return {"parent_kind": PARENT_KINDS[parent], "main": files[0], "model": files[3],
            "attributes": resolved["descriptor"]["o_attributes"]}


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--list", action="store_true",
                    help="print name:NAME:TOKEN:id:BASE per registry row (the Makefile's view; "
                         "TOKEN the Remix name or NAME, BASE an authored content's base or -)")
    ap.add_argument("--contents", help="enabled content names, space separated")
    ap.add_argument("--export-root", type=Path)
    ap.add_argument("--out", type=Path, help="the build's P4 directory")
    args = ap.parse_args()

    if args.list:
        print(" ".join(f"{r['name']}:{r['name'].upper()}:{token(r)}:{r['id']}:{r.get('base', '-')}"
                       for r in load_registry()))
        return 0
    if args.contents is None or args.export_root is None or args.out is None:
        ap.error("--contents, --export-root and --out are required")
    registry = load_registry()
    rows = enabled_rows(args.contents.split(), registry)
    header = [
        "/* Generated by scripts/p4/p4_contents.py from scripts/p4/contents.json and",
        " * the enabled contents' exports. Do not edit. */",
        "#ifndef NDS_P4_CONTENTS_GENERATED_H",
        "#define NDS_P4_CONTENTS_GENERATED_H",
        "",
        "/* X(id, Title, NAME, name, parent_kind, model_file_id, main_file_id) */",
        "#define NDS_P4_CONTENT_ROWS(X) \\",
    ]
    data = ["/* Generated by scripts/p4/p4_contents.py. Do not edit. */"]
    for row in rows:
        facts = export_facts(args.export_root, row["name"])
        header.append(f"    X({row['id']}, {row['title']}, {row['name'].upper()}, {row['name']}, "
                      f"{facts['parent_kind']}, {facts['model']:#x}u, {facts['main']:#x}u) \\")
        data.append(f"#include \"{row['name']}/nds_p4_{row['name']}.generated.c\"")
    header += ["    /* end */",
               f"#define NDS_P4_CONTENT_MAX_ID {max((r['id'] for r in rows), default=0)}", ""]
    # The code each content runs (code_row): a content's own source file, or
    # an authored content's base's, which then compiles even when the base
    # itself is not enabled. NDS_P4_CODE_<NAME> gates a source file's body;
    # NDS_P4_CODE_ID_<NAME> is its content id for ndsP4CodeContent compares.
    codes = {c["name"]: c for c in (code_row(r, registry) for r in rows)
             if (PORT_DIR / f"nds_p4_{c['name']}.c").exists()}
    header.append("/* Source files this build compiles (src/port/nds_p4_<name>.c), by the")
    header.append(" * content whose code they are. */")
    for r in registry:
        header.append(f"#define NDS_P4_CODE_{r['name'].upper()} {1 if r['name'] in codes else 0}")
        if r["name"] in codes:
            header.append(f"#define NDS_P4_CODE_ID_{r['name'].upper()} {r['id']}u")
    header += ["/* X(id, Title, code_id, CodeTitle): the content whose routines, hooks and",
               " * overrides each content runs. */",
               "#define NDS_P4_CODE_ROWS(X) \\"]
    for r in rows:
        c = code_row(r, registry)
        header.append(f"    X({r['id']}, {r['title']}, {c['id']}, {c['title']}) \\")
    header += ["    /* end */", "/* The same by title, for token pasting (nds_p4.c's hook rows). */"]
    for r in rows:
        header.append(f"#define NDS_P4_CODE_TITLE_{r['title']} {code_row(r, registry)['title']}")
    header += ["", "#endif /* NDS_P4_CONTENTS_GENERATED_H */", ""]
    args.out.mkdir(parents=True, exist_ok=True)
    # Every TU sees the list through nds_p4.h: an unchanged file keeps its
    # mtime, so a re-run (make re-runs this whenever an input is newer) does
    # not rebuild the world.
    for path, text in ((args.out / "nds_p4_contents.generated.h", "\n".join(header)),
                       (args.out / "nds_p4_contents_data.generated.inc", "\n".join(data) + "\n")):
        if not path.exists() or path.read_text(encoding="utf-8") != text:
            path.write_text(text, encoding="utf-8", newline="\n")
    print(f"{len(rows)} contents: {' '.join(r['name'] for r in rows)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
