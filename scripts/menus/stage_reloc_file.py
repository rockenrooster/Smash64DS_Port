#!/usr/bin/env python3
"""Stage one BattleShip reloc sprite file for the DS runtime (P2-6/P2-7).

WHY
---
Every imported scene draws its sprites by reloc symbol:

    lbRelocGetFileData(Sprite *, files[n], &llSC1PStageClear1StageTextSprite)

The port resolves that symbol through five hand-maintained surfaces, and a
file missing from any one of them fails at runtime: an unknown symbol counts
a resolve failure and returns NULL, and a sprite with no geometry row is left
mixed-width after the blanket u32 byte swap. Until 2026-09-04 the rows were
written by hand per file (IFCommon*, MNVSResults), which is why the 1P tally
waited on "72 reloc rows". This script derives every row from the two
read-only authorities and writes all five surfaces idempotently:

  offsets   decomp/BattleShip-main/include/reloc_data.us.h
            (symbol -> payload offset, ll<File>FileID -> file id)
  geometry  decomp/BattleShip-main/BattleShip_o2r/<dir>/<File>
            (the libultra Sprite record at every *Sprite offset, read with the
            UI-kit bake's own RELO parser so the two cannot disagree)

SURFACES WRITTEN
  include/reloc_data.h                     FileID token extern, the
                                           NDS_<F>_RELOC_SYMBOLS(X) rows and
                                           their extern expansion
  src/port/diagnostics_mp_taskman_state.c  the token global and the
                                           definition expansion
  src/port/reloc_backend_assets.c          NDS_RELOC_ASSET_<F>, the
                                           token -> asset line, the
                                           known-symbol rows, the Sprite
                                           geometry rows, the ledger class
  src/nds/nds_reloc_assets.c               asset id -> NitroFS path
  Makefile                                 the file in a staging list
                                           (--list), and the list in
                                           NDS_NITROFS_RELOC_FILES if new

Non-sprite symbols (DObjDesc, AnimJoint, MapHeader, ...) get offset rows but
no geometry row -- they need their own normalizer -- and the script names
them so nobody assumes a sprite-only file. A file already staged by hand
(its id in nds_reloc_assets.c) is refused, not duplicated.

USAGE
  python scripts/menus/stage_reloc_file.py --file SC1PStageClear1 --list NDS_1P_RELOC_FILES
  python scripts/menus/stage_reloc_file.py --file SC1PStageClear1 --check
  python scripts/menus/stage_reloc_file.py --file SC1PStageClear1 --dry-run
"""
from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import generate_mn_ui_kit as kit  # noqa: E402  (the RELO/Sprite parser)

FMT = {0: "G_IM_FMT_RGBA", 1: "G_IM_FMT_YUV", 2: "G_IM_FMT_CI",
       3: "G_IM_FMT_IA", 4: "G_IM_FMT_I"}
# 4 is BattleShip's own G_IM_SIZ_4c (decomp PR/gbi.h:435, port PR/mbi.h:14): a
# compressed 4-bit intensity bitmap that lbCommonMakeSObjForGObj expands with
# lbCommonDecodeSpriteBitmapsSiz4b (lbcommon.c:2844) before the SObj is made.
SIZ = {0: "G_IM_SIZ_4b", 1: "G_IM_SIZ_8b", 2: "G_IM_SIZ_16b", 3: "G_IM_SIZ_32b",
       4: "G_IM_SIZ_4c"}

O2R_HEADER = 0x40  # the OLER resource header; the RELO fields follow it


class Container(kit.RelocFile):
    """kit.RelocFile with the extern-list-aware header the port's own loader
    uses (nds_reloc_assets.c ndsRelocAssetReadHeaderFromFile): the data size
    sits at 0x4c + 2 * extern_count and the payload right after it, so a file
    with external relocations (SC1PTrainingMode, SCExplainMain) parses. A
    pointer that is not in the internal reloc list is external; geometry
    never dereferences one, so it is returned raw instead of refused."""

    def __init__(self, path: Path) -> None:
        raw = path.read_bytes()
        if len(raw) < O2R_HEADER + 16:
            raise kit.ConvertError(f"{path.name}: shorter than a RELO header")
        if raw[4:8] != b"OLER":
            raise kit.ConvertError(f"{path.name}: not an OLER container")
        self.path = path
        self.file_id = int.from_bytes(raw[O2R_HEADER:O2R_HEADER + 4], "little")
        self.reloc_intern = int.from_bytes(raw[O2R_HEADER + 4:O2R_HEADER + 6], "little")
        self.reloc_extern = int.from_bytes(raw[O2R_HEADER + 6:O2R_HEADER + 8], "little")
        self.extern_count = int.from_bytes(raw[O2R_HEADER + 8:O2R_HEADER + 12], "little")
        ids_at = O2R_HEADER + 12
        self.extern_ids = [int.from_bytes(raw[ids_at + 2 * i:ids_at + 2 * i + 2], "little")
                           for i in range(self.extern_count)]
        size_at = ids_at + 2 * self.extern_count
        self.data_size = int.from_bytes(raw[size_at:size_at + 4], "little")
        data_at = size_at + 4
        if data_at + self.data_size > len(raw):
            raise kit.ConvertError(
                f"{path.name}: declared payload {self.data_size} exceeds file")
        self.payload = bytearray(raw[data_at:data_at + self.data_size])
        self.pointer_targets: dict[int, int] = {}
        self._walk_internal_relocs()

    def pointer(self, off: int) -> int:
        if off in self.pointer_targets:
            return self.pointer_targets[off]
        return int.from_bytes(self.payload[off:off + 4], "big")

HEADER = "include/reloc_data.h"
DIAG = "src/port/diagnostics_mp_taskman_state.c"
BACKEND = "src/port/reloc_backend_assets.c"
ASSETS = "src/nds/nds_reloc_assets.c"
MAKEFILE = "Makefile"

# Anchors: each is the last line of the block the same kind of row already
# lives in, so a new file lands beside its hand-written predecessors.
HEADER_ANCHOR = "#undef NDS_DECLARE_TRANSITION_RELOC_SYMBOL\n"
DIAG_ANCHOR = "#undef NDS_DEFINE_TRANSITION_RELOC_SYMBOL\n"
BACKEND_DEFINE_ANCHOR = "#define NDS_RELOC_ASSET_MN_VS_RESULTS 0x22u\n"
BACKEND_TOKEN_ANCHOR = ("    if (token == ndsRelocFileID(&llMNVSResultsFileID)) "
                        "return NDS_RELOC_ASSET_MN_VS_RESULTS;\n")
BACKEND_KNOWN_ANCHOR = "    NDS_VS_RESULTS_RELOC_SYMBOLS(NDS_KNOWN_ASSET_SYMBOL)\n"
BACKEND_TABLE_START = "sNdsBattleInterfaceSpriteDescs[] = {"
ASSETS_ANCHOR = '    { 0x22, 0x22, "nitro:/reloc/reloc_menus/MNVSResults" },\n'
MAKE_LIST_ANCHOR = "NDS_VSBATTLE_RELOC_FILES := \\\n"
MAKE_FOREACH_ANCHOR = ("\t$(foreach file,$(NDS_VSBATTLE_RELOC_FILES),"
                       "$(NITROFS_DIR)/reloc/$(file)) \\\n")

INTERFACE_DIRS = {"reloc_interface"}


class StageError(RuntimeError):
    pass


def macro_token(name: str) -> str:
    """SC1PStageClear1 -> SC1P_STAGE_CLEAR1, IFCommonTimer -> IF_COMMON_TIMER.

    A digit stays glued to the capital after it (SC1P, MN1P) unless that
    capital starts a word (Clear1Foo -> CLEAR1_FOO)."""
    spaced = re.sub(r"(?<=[a-z])(?=[A-Z])|(?<=[A-Z0-9])(?=[A-Z][a-z])", "_", name)
    return spaced.upper()


def parse_decomp(root: Path) -> tuple[dict[str, int], dict[str, int]]:
    text = (root / "decomp/BattleShip-main/include/reloc_data.us.h").read_text(
        errors="replace")
    pattern = r"#define\s+(ll\w+)\s+\(\(intptr_t\)(0x[0-9a-fA-F]+|\d+)\)"
    symbols: dict[str, int] = {}
    for name, value in re.findall(pattern, text):
        symbols[name] = int(value, 0)
    file_ids = {name[2:-6]: value for name, value in symbols.items()
                if name.endswith("FileID") and name.startswith("ll")}
    if not file_ids:
        raise StageError("reloc_data.us.h yielded no ll*FileID rows")
    return file_ids, symbols


def file_symbols(name: str, file_ids: dict[str, int],
                 symbols: dict[str, int]) -> dict[str, int]:
    """Symbols owned by <name>: the longest ll<File> prefix wins, so
    MNBackupClearHeaderOption's sprite is not counted under MNBackupClear."""
    out: dict[str, int] = {}
    for sym, off in symbols.items():
        if sym.endswith("FileID") or not sym.startswith("ll"):
            continue
        owners = [f for f in file_ids if sym.startswith("ll" + f)]
        if not owners:
            continue
        if max(owners, key=len) == name:
            out[sym] = off
    return dict(sorted(out.items(), key=lambda kv: kv[1]))


def find_container(root: Path, name: str) -> Path:
    base = root / "decomp/BattleShip-main/BattleShip_o2r"
    hits = sorted(p for p in base.glob(f"*/{name}") if p.is_file())
    if len(hits) != 1:
        raise StageError(f"{name}: {len(hits)} o2r containers under {base}")
    return hits[0]


class Plan:
    def __init__(self, root: Path, name: str) -> None:
        file_ids, symbols = parse_decomp(root)
        if name not in file_ids:
            raise StageError(f"{name}: no ll{name}FileID in reloc_data.us.h")
        self.name = name
        self.file_id = file_ids[name]
        self.token = macro_token(name)
        self.symbols = file_symbols(name, file_ids, symbols)
        if not self.symbols:
            raise StageError(f"{name}: owns no symbols")
        self.container = find_container(root, name)
        self.dir = self.container.parent.name
        reloc = Container(self.container)
        if reloc.file_id != self.file_id:
            raise StageError(f"{name}: container id {reloc.file_id:#x} != "
                             f"symbol table id {self.file_id:#x}")
        self.payload_size = reloc.data_size
        self.rows: list[tuple[str, int, int, int, int, int, int]] = []
        self.other: list[str] = []
        self.bad_displist: list[str] = []
        for sym, off in self.symbols.items():
            if off >= reloc.data_size:
                raise StageError(f"{sym}: offset {off:#x} beyond payload "
                                 f"{reloc.data_size:#x}")
            if not sym.endswith("Sprite"):
                self.other.append(sym)
                continue
            sprite = reloc.sprite(off)
            ndisplist = reloc.s16(off + 42)
            if ndisplist != 12 * sprite.nbitmaps + 24:
                self.bad_displist.append(
                    f"{sym} ndisplist {ndisplist} != {12 * sprite.nbitmaps + 24}")
            if sprite.bmfmt not in FMT or sprite.bmsiz not in SIZ:
                raise StageError(f"{sym}: bmfmt {sprite.bmfmt} bmsiz "
                                 f"{sprite.bmsiz} is not a libultra format")
            self.rows.append((sym, off, sprite.width, sprite.height,
                              sprite.nbitmaps, sprite.bmfmt, sprite.bmsiz))
        self.classifier = ("ndsRelocAssetIsInterface" if self.dir in INTERFACE_DIRS
                           else "ndsRelocAssetIsMenu")

    # -- text each surface must contain ------------------------------------

    @property
    def asset(self) -> str:
        return f"NDS_RELOC_ASSET_{self.token}"

    @property
    def macro(self) -> str:
        return f"NDS_{self.token}_RELOC_SYMBOLS"

    def header_block(self) -> str:
        rows = [f"    X({self.asset}, {sym}, 0x{off:04x}u)"
                for sym, off in self.symbols.items()]
        body = " \\\n".join(rows)
        return (f"\n/* {self.name} (reloc file 0x{self.file_id:x}, {self.dir}): "
                f"staged by scripts/menus/stage_reloc_file.py. */\n"
                f"extern uintptr_t ll{self.name}FileID;\n\n"
                f"#define {self.macro}(X) \\\n{body}\n\n"
                f"#define NDS_DECLARE_{self.token}_RELOC_SYMBOL(asset, name, value) "
                f"extern uintptr_t name;\n"
                f"{self.macro}(NDS_DECLARE_{self.token}_RELOC_SYMBOL)\n"
                f"#undef NDS_DECLARE_{self.token}_RELOC_SYMBOL\n")

    def diag_block(self) -> str:
        return (f"\nuintptr_t ll{self.name}FileID = 0x{self.file_id:x}u;\n"
                f"#define NDS_DEFINE_{self.token}_RELOC_SYMBOL(asset, name, value) "
                f"uintptr_t name = value;\n"
                f"{self.macro}(NDS_DEFINE_{self.token}_RELOC_SYMBOL)\n"
                f"#undef NDS_DEFINE_{self.token}_RELOC_SYMBOL\n")

    def define_line(self) -> str:
        return (f"#define {self.asset} 0x{self.file_id:x}u "
                f"/* {self.dir}/{self.name}, stage_reloc_file.py */\n")

    def token_line(self) -> str:
        return (f"    if (token == ndsRelocFileID(&ll{self.name}FileID)) "
                f"return {self.asset};\n")

    def known_line(self) -> str:
        return f"    {self.macro}(NDS_KNOWN_ASSET_SYMBOL)\n"

    def geometry_rows(self) -> list[str]:
        return [f"    {{ {self.asset}, 0x{off:04x}u, {w}u, {h}u, {n}u,\n"
                f"      {FMT[fmt]}, {SIZ[siz]} }}"
                for _sym, off, w, h, n, fmt, siz in self.rows]

    def geometry_block(self) -> str:
        if not self.rows:
            return ""
        head = (f"    /* {self.name} (reloc asset 0x{self.file_id:x}), "
                f"stage_reloc_file.py. */\n")
        return ",\n" + head + ",\n".join(self.geometry_rows())

    def case_line(self) -> str:
        return f"    case {self.asset}:\n"

    def assets_line(self) -> str:
        return (f"    {{ 0x{self.file_id:x}, 0x{self.file_id:x}, "
                f'"nitro:/reloc/{self.dir}/{self.name}" }},\n')

    def make_entry(self) -> str:
        return f"{self.dir}/{self.name}"


# -- surface edits --------------------------------------------------------

def read(root: Path, rel: str) -> str:
    return (root / rel).read_bytes().decode("utf-8").replace("\r\n", "\n")


def write(root: Path, rel: str, text: str, original: str) -> None:
    raw = (root / rel).read_bytes()
    if b"\r\n" in raw:
        text = text.replace("\n", "\r\n")
    if text != original:
        (root / rel).write_bytes(text.encode("utf-8"))


def insert_after(text: str, anchor: str, block: str, what: str) -> str:
    if anchor not in text:
        raise StageError(f"{what}: anchor not found: {anchor.strip()!r}")
    if text.count(anchor) != 1:
        raise StageError(f"{what}: anchor is not unique: {anchor.strip()!r}")
    return text.replace(anchor, anchor + block, 1)


def staged_ids(assets_text: str) -> dict[int, str]:
    out: dict[int, str] = {}
    for value, path in re.findall(
            r"\{\s*(0x[0-9a-fA-F]+|\d+)\s*,\s*(?:0x[0-9a-fA-F]+|\d+)\s*,\s*\"([^\"]+)\"",
            assets_text):
        out[int(value, 0)] = path
    return out


def edit_header(text: str, plan: Plan) -> str:
    if f"#define {plan.macro}(X)" in text:
        return text
    return insert_after(text, HEADER_ANCHOR, plan.header_block(), HEADER)


def edit_diag(text: str, plan: Plan) -> str:
    if f"uintptr_t ll{plan.name}FileID" in text:
        return text
    return insert_after(text, DIAG_ANCHOR, plan.diag_block(), DIAG)


def edit_backend(text: str, plan: Plan) -> str:
    if f"#define {plan.asset} " not in text:
        for value in re.findall(r"#define NDS_RELOC_ASSET_\w+ (0x[0-9a-fA-F]+|\d+)u", text):
            if int(value, 0) == plan.file_id:
                raise StageError(f"{BACKEND}: file id 0x{plan.file_id:x} already has "
                                 f"an NDS_RELOC_ASSET_ define (hand-staged)")
        text = insert_after(text, BACKEND_DEFINE_ANCHOR, plan.define_line(), BACKEND)
    if plan.token_line() not in text:
        text = insert_after(text, BACKEND_TOKEN_ANCHOR, plan.token_line(), BACKEND)
    if plan.known_line() not in text:
        text = insert_after(text, BACKEND_KNOWN_ANCHOR, plan.known_line(), BACKEND)
    if plan.rows and f"{{ {plan.asset}, 0x" not in text:
        start = text.find(BACKEND_TABLE_START)
        if start < 0:
            raise StageError(f"{BACKEND}: table {BACKEND_TABLE_START!r} not found")
        end = text.find("\n};", start)
        if end < 0:
            raise StageError(f"{BACKEND}: table end not found")
        text = text[:end] + plan.geometry_block() + text[end:]
    if plan.case_line() not in text:
        anchor = (f"static s32 {plan.classifier}(u32 asset_id)\n{{\n"
                  f"    switch (asset_id)\n    {{\n")
        text = insert_after(text, anchor, plan.case_line(), BACKEND)
    return text


def edit_assets(text: str, plan: Plan) -> str:
    if plan.assets_line() in text:
        return text
    ids = staged_ids(text)
    if plan.file_id in ids:
        raise StageError(f"{ASSETS}: file id 0x{plan.file_id:x} already staged as "
                         f"{ids[plan.file_id]}")
    return insert_after(text, ASSETS_ANCHOR, plan.assets_line(), ASSETS)


def edit_makefile(text: str, plan: Plan, list_name: str) -> str:
    entry = plan.make_entry()
    lines = text.split("\n")
    head = f"{list_name} := \\"
    idx = next((i for i, line in enumerate(lines)
                if line == head or line == f"{list_name} :="), None)
    if idx is None:
        block = f"{list_name} := \\\n\t{entry}\n\n"
        if MAKE_LIST_ANCHOR not in text:
            raise StageError(f"{MAKEFILE}: anchor {MAKE_LIST_ANCHOR.strip()!r} missing")
        text = text.replace(MAKE_LIST_ANCHOR, block + MAKE_LIST_ANCHOR, 1)
        foreach = (f"\t$(foreach file,$({list_name}),$(NITROFS_DIR)/reloc/$(file)) \\\n")
        if foreach not in text:
            text = insert_after(text, MAKE_FOREACH_ANCHOR, foreach, MAKEFILE)
        return text
    if lines[idx] == f"{list_name} :=":
        lines[idx] = head
        lines.insert(idx + 1, f"\t{entry}")
        return "\n".join(lines)
    j = idx
    while lines[j].endswith("\\"):
        j += 1
    block_entries = [l.strip().rstrip("\\").strip() for l in lines[idx + 1:j + 1]]
    if entry in block_entries:
        return text
    lines[j] = lines[j] + " \\"
    lines.insert(j + 1, f"\t{entry}")
    return "\n".join(lines)


# -- check ----------------------------------------------------------------

def check(root: Path, plan: Plan, list_name: str | None) -> list[str]:
    missing: list[str] = []
    header = read(root, HEADER)
    for sym, off in plan.symbols.items():
        if f"X({plan.asset}, {sym}, 0x{off:04x}u)" not in header:
            missing.append(f"{HEADER}: X row for {sym}")
    if f"extern uintptr_t ll{plan.name}FileID;" not in header:
        missing.append(f"{HEADER}: FileID extern")
    diag = read(root, DIAG)
    if f"{plan.macro}(NDS_DEFINE_{plan.token}_RELOC_SYMBOL)" not in diag:
        missing.append(f"{DIAG}: definition expansion")
    if f"uintptr_t ll{plan.name}FileID" not in diag:
        missing.append(f"{DIAG}: FileID global")
    backend = read(root, BACKEND)
    for what, needle in (("asset define", f"#define {plan.asset} 0x{plan.file_id:x}u"),
                         ("token line", plan.token_line()),
                         ("known-symbol rows", plan.known_line()),
                         ("ledger case", plan.case_line())):
        if needle not in backend:
            missing.append(f"{BACKEND}: {what}")
    for row, (sym, *_rest) in zip(plan.geometry_rows(), plan.rows):
        if row not in backend:
            missing.append(f"{BACKEND}: geometry row for {sym}")
    assets = read(root, ASSETS)
    if plan.assets_line() not in assets:
        missing.append(f"{ASSETS}: path row")
    make = read(root, MAKEFILE)
    if f"\t{plan.make_entry()}" not in make:
        missing.append(f"{MAKEFILE}: {plan.make_entry()} in a staging list")
    if list_name and (f"$({list_name})" not in make.split("NDS_NITROFS_RELOC_FILES", 1)[-1]):
        missing.append(f"{MAKEFILE}: {list_name} not in NDS_NITROFS_RELOC_FILES")
    return missing


def report(plan: Plan) -> None:
    print(f"{plan.name}: reloc file 0x{plan.file_id:x} in {plan.dir}, payload "
          f"{plan.payload_size} bytes, {len(plan.symbols)} symbols, "
          f"{len(plan.rows)} sprites, {len(plan.other)} non-sprite symbols")
    for sym in plan.other:
        print(f"  non-sprite (needs its own normalizer): {sym}")
    for line in plan.bad_displist:
        print(f"  WARNING (runtime refuses this sprite): {line}")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("--repo-root", type=Path,
                        default=HERE.parent.parent)
    parser.add_argument("--file", required=True, help="reloc file name, e.g. SC1PStageClear1")
    parser.add_argument("--list", default=None,
                        help="Makefile staging list to add the file to (apply mode)")
    parser.add_argument("--dry-run", action="store_true")
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args(argv)
    root = args.repo_root.resolve()
    try:
        plan = Plan(root, args.file)
        report(plan)
        if args.check:
            missing = check(root, plan, args.list)
            for line in missing:
                print(f"MISSING {line}")
            print(f"{plan.name}: {'OK' if not missing else f'{len(missing)} missing'}")
            return 0 if not missing else 1
        if args.dry_run:
            print(plan.header_block())
            print(plan.diag_block())
            print(plan.define_line() + plan.token_line() + plan.known_line())
            print(plan.geometry_block())
            print(plan.assets_line())
            return 0
        if not args.list:
            raise StageError("--list <MAKEFILE_LIST> is required to apply")
        edits = ((HEADER, edit_header), (DIAG, edit_diag), (BACKEND, edit_backend),
                 (ASSETS, edit_assets))
        pending = []
        for rel, fn in edits:
            original = read(root, rel)
            pending.append((rel, original, fn(original, plan)))
        original = read(root, MAKEFILE)
        pending.append((MAKEFILE, original, edit_makefile(original, plan, args.list)))
        for rel, original, new in pending:
            write(root, rel, new, original)
            print(f"{'edited ' if new != original else 'unchanged'} {rel}")
        missing = check(root, plan, args.list)
        for line in missing:
            print(f"MISSING {line}")
        return 0 if not missing else 1
    except (StageError, kit.ConvertError) as exc:
        print(f"ERROR: {exc}")
        return 2


if __name__ == "__main__":
    sys.exit(main())
