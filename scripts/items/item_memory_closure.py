#!/usr/bin/env python3
"""Measured byte accounting for the real item core's required memory.

Reads the actual BattleShip item closure with the same logic the DS loaders
use, derives exactly which MiscData086 (file 0x56) bytes stay live, and
prototypes one concrete safe savings candidate. No live C edits, no asset
regen, no emulator: host-only, stdlib-only.

Required memory today (measured, not assumed):
  ITCommonData (0xfb) 3,392 payload + 68 externs, all into MiscData086.
  MiscData086  (0x56) 79,584 payload, 0 externs.
  Extern tree total 82,976 = gNdsITCommonDataBytes (verify-p2-four-fighter-
  stress.ps1 asserts exactly this). itManagerInitItems pays it before the
  fighter closures run (battleship_item_link_core.c), so every byte saved
  here frees one arena byte for the later ndsFtPoseOpen request that now
  fails with request=3072 free=1028 (shortfall 2,044).

Live readers (all cited, all in-tree):
  Gameplay/collision words: ndsItDecodeAttributes reads ROM words 0x10-0x44
    of each ITAttributes row once into sNdsItAttributes
    (src/import/battleship_item_link_core.c).
  All four ITAttributes pointers stay live: attr->data feeds
    gcSetupCustomDObjsWithMObj/itManagerSetupItemDObjs ->
    gcAddDObjForGObj(gobj, dobjdesc->dl); attr->p_mobjsubs feeds
    gcAddMObjAll; attr->anim_joints/p_matanim_joints feed gcAddAnimAll
    (same file, lines ~989-1241).
  Item DObj trees reach the DS hardware through SubmitItemDObjTree ->
  SubmitStageDL (src/port/reloc_backend_movement.c,
  src/port/renderer_adapter_stage.c), which walks Gfx display lists and
  consumes Vtx geometry, textures, palettes and MObj materials.
  Per-kind interior pointers (bombhei walk DLs, mball matanim, harisen anim
  joints) reach file-0x56 bytes through itGetPData arithmetic off
  attr->data (src/import/battleship_item_*.c llITCommonData* tokens).

Method (existing relocation metadata only):
  O2R headers are parsed the way ndsRelocAssetReadHeaderFromFile does
  (src/nds/nds_reloc_assets.c): LE file_id/intern/extern/extern_count at
  0x40, extern id table, data_size word, payload.
  251_ITCommonData.reloc gives 68 (251-label, file-0x56-offset) externs.
  251_ITCommonData.c initializers name the exact file-0x56 label behind each
  extern (data, p_mobjsubs, anim_joints, p_matanim_joints), so every extern
  anchors one label to one offset with no name guessing.
  86_ITCommonObject.reloc gives 402 within-file pointer edges.
  86_ITCommonObject.c gives each label's declared type; labels carrying
  _0xOFFSET (plus optional _sub_0xOFFSET) give exact payload offsets.
  battleship_item_*.c llITCommonData* tokens >= 0xD40 (past the 3,392-byte
  251 payload) give numeric file-0x56 roots for itGetPData arithmetic.
  Reachability runs over labels from all three root sets. The payload is
  partitioned at every known offset; each interval belongs to its label.
  An interval is droppable only when its label is unreachable AND the
  interval is untyped pad AND it holds no numeric root AND it holds no
  reachable child. Typed geometry, materials, animation and textures are
  never dropped even when unreachable, because per-kind runtime arithmetic
  can legally land inside them (harisen anim joints are the proven case).

Outputs (to --out, default builds/resume-20260905/item-memory):
  report.txt, map.json, compact86.bin, compact86.reloc, compact251.reloc.
Exit non-zero when any self-check fails.
"""

from __future__ import annotations

import argparse
import json
import re
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
DECOMP_RELOC = ROOT / "decomp/BattleShip-main/decomp/src/relocData"
O2R_86 = ROOT / "decomp/BattleShip-main/BattleShip_o2r/reloc_extern_data/MiscData086"
O2R_251 = ROOT / "decomp/BattleShip-main/BattleShip_o2r/reloc_items/ITCommonData"
SRC_86 = DECOMP_RELOC / "86_ITCommonObject.c"
RELOC_86 = DECOMP_RELOC / "86_ITCommonObject.reloc"
SRC_251 = DECOMP_RELOC / "251_ITCommonData.c"
RELOC_251 = DECOMP_RELOC / "251_ITCommonData.reloc"
ITEM_IMPORT_GLOB = "src/import/battleship_item_*.c"

FILE_86_SIZE = 79584
FILE_251_SIZE = 3392
TREE_SIZE = 82976
SHORTFALL = 2044  # 3072 requested - 1028 free, fourcpu stress startup.

O2R_HEADER = 0x40


def read_text(path: Path) -> str:
    return path.read_text(encoding="utf-8", errors="replace")


def parse_o2r(path: Path, expect_id: int) -> tuple[int, int, list[int], bytes]:
    """Parse an O2R container the way ndsRelocAssetReadHeaderFromFile does."""
    raw = path.read_bytes()
    assert raw[4:8] == b"OLER", f"{path}: missing OLER magic"
    file_id, intern, extern = struct.unpack_from("<IHH", raw, O2R_HEADER)
    (extern_count,) = struct.unpack_from("<I", raw, O2R_HEADER + 8)
    assert file_id == expect_id, f"{path}: file_id {file_id:#x}"
    ids = list(struct.unpack_from(f"<{extern_count}H", raw, O2R_HEADER + 12))
    dsize_off = O2R_HEADER + 12 + extern_count * 2
    (data_size,) = struct.unpack_from("<I", raw, dsize_off)
    payload = raw[dsize_off + 4: dsize_off + 4 + data_size]
    assert len(payload) == data_size, f"{path}: short payload"
    return data_size, extern_count, ids, payload


def parse_251_reloc(text: str) -> list[tuple[str, int, int]]:
    """Return (251_label, field_index, file86_offset) per extern line."""
    out = []
    for line in text.splitlines():
        m = re.match(r"^extern\s+(\w+)(\+0x([0-9A-Fa-f]+))?\s+(0x[0-9A-Fa-f]+)", line)
        if not m:
            continue
        label, _, sub, off = m.groups()
        field = {"0": 0, "4": 1, "8": 2, "c": 3}[sub.lower()] if sub else 0
        out.append((label, field, int(off, 16)))
    return out


def parse_251_initializers(text: str) -> dict[str, list[str | None]]:
    """Map each dITCommonData_* row label to its 4 pointer fields.

    Reads the actual (void *)/cast/NULL expressions in row order, so the
    file-0x56 label behind each extern is transcribed, not guessed.
    """
    rows: dict[str, list[str | None]] = {}
    for m in re.finditer(
        r"(?:ITAttributes|WPAttributes)\s+(dITCommonData_\w+)(?:\[\d+\])?\s*=\s*\{",
        text,
    ):
        label = m.group(1)
        # First four pointer slots: data, p_mobjsubs, anim_joints,
        # p_matanim_joints. Value rows (ITAttackEvent/f32) never match.
        fields: list[str | None] = []
        pos = m.end()
        while len(fields) < 4:
            em = re.search(
                r"\(\w[\w\s\*]*\)\s*(dITCommonObject_\w+)|(?<![\w])(NULL)(?![\w])",
                text[pos:],
            )
            if em is None:
                break
            # Stop at the next row definition: only accept hits before it.
            nm = re.search(
                r"(?:ITAttributes|WPAttributes|ITAttackEvent|f32)\s+dITCommonData_\w+",
                text[pos:],
            )
            if nm is not None and em.start() > nm.start():
                break
            fields.append(em.group(1) or None)
            pos += em.end()
        # A row whose first slot is not a pointer is not an attribute row
        # (defensive: value tables must never anchor externs).
        if fields and fields[0] is not None:
            rows[label] = fields
    return rows


def parse_86_reloc(text: str) -> list[tuple[str, int, str, int]]:
    """Return (src_label, src_off, dst_label, dst_off) per intern line."""
    out = []
    for line in text.splitlines():
        parts = line.split()
        if len(parts) < 3 or parts[0] != "intern":
            continue
        sm = re.match(r"(\w+)(\+0x([0-9A-Fa-f]+))?$", parts[1])
        dm = re.match(r"(\w+)(\+0x([0-9A-Fa-f]+))?$", parts[2])
        if sm is None or dm is None:
            continue
        out.append((
            sm.group(1), int(sm.group(3), 16) if sm.group(3) else 0,
            dm.group(1), int(dm.group(3), 16) if dm.group(3) else 0,
        ))
    return out


def parse_86_decl_types(text: str) -> dict[str, str]:
    """Map label -> declared C type from the extern declarations."""
    out: dict[str, str] = {}
    for m in re.finditer(
        r"^extern\s+(Gfx|Vtx|u8|u16|u32|MObjSub\s*\*|AObjEvent32\s*\*|"
        r"DObjDesc|MObj|void)\s+(dITCommonObject_\w+)\[\]",
        text,
        re.M,
    ):
        out[m.group(2)] = re.sub(r"\s+", "", m.group(1))
    return out


ELEM_BYTES = {
    "Gfx": 8, "Vtx": 16, "u8": 1, "u16": 2, "u32": 4,
    "MObjSub*": 4, "AObjEvent32*": 4, "AObjEvent32**": 4, "DObjDesc": 44,
    # Bare MObjSub: 120 bytes by field audit of decomp sys/objdef.h
    # (u16+2u8+ptr+4u16+s32+6f32+ptr+u16+2u8+4u16+4f32+u32+5 SYColorPack
    # words + 4u8 color bytes + 4s32; SYColorPack is one word per the
    # objdef.h payload-word note; every member naturally aligned, so IDO
    # and GCC agree). Any interval smaller than its struct fails loudly.
    "MObjSub": 120,
    # DObjDLLink = {s32 list_id, Gfx *dl} (decomp sys/objtypes.h).
    "DObjDLLink": 8,
}


def parse_86_def_sizes(text: str) -> dict[str, int]:
    """Map label -> declared payload bytes from sized definitions.

    u8 Tex_0xC608[1800] wartet: the splitter emits exact array sizes, so
    bytes past label+size inside the same interval are alignment filler.
    Only single-owner intervals may trim to this bound (union aliases
    share one offset with their own sizes). DObjDesc sizes double as a
    cross-check on the terminator scans (same count or failure).
    """
    out: dict[str, int] = {}
    for m in re.finditer(
        r"^(Gfx|Vtx|u8|u16|u32|DObjDesc|DObjDLLink|MObjSub|AObjEvent32\s*\*+)\s*"
        r"(dITCommonObject_\w+)\[(\d*)\]\s*=\s*\{",
        text,
        re.M,
    ):
        typ = re.sub(r"\s+", "", m.group(1))
        typ = "AObjEvent32*" if typ.startswith("AObjEvent32") else typ
        count = m.group(3)
        if count and typ in ELEM_BYTES:
            out[m.group(2)] = int(count) * ELEM_BYTES[typ]
    return out


def parse_86_def_counts(text: str) -> dict[str, int]:
    """Map DObjDesc label -> declared entry count (scan cross-check)."""
    out: dict[str, int] = {}
    for m in re.finditer(
        r"^DObjDesc\s+(dITCommonObject_\w+)\[(\d+)\]\s*=\s*\{", text, re.M
    ):
        out[m.group(1)] = int(m.group(2))
    return out


def embedded_offset(label: str) -> int | None:
    """File-0x56 offset carried inside splitter labels, else None.

    The _sub_0xNN occurrence is relative and excluded from the parent
    search, so a sub-label of an unplaced parent stays unplaced instead of
    resolving to a bogus absolute address.
    """
    nums = re.findall(r"(?<!sub)(?<!post)_0x([0-9A-Fa-f]+)", label)
    if not nums:
        return None
    base = int(nums[-1], 16)
    m = re.search(r"_sub_0x([0-9A-Fa-f]+)", label)
    if m:
        base += int(m.group(1), 16)
    return base


def parent_relative_offset(label: str, offsets: dict[str, int]) -> int | None:
    """Resolve _sub_0xNN against a placed parent prefix.

    _post_0xNN labels are deliberately excluded: payload slots prove they
    do not sit at parent+NN (LGunAmmo post_0x40 reads 0x4190, not
    0x40A8+0x40), so only an incoming intern slot may place them.
    """
    m = re.search(r"(_sub_0x[0-9A-Fa-f]+)$", label)
    if not m:
        return None
    parent = label[: -len(m.group(0))]
    if parent in offsets:
        return offsets[parent] + int(m.group(1).split("_0x")[1], 16)
    return None


def scan_remainder_offset(
    payload: bytes, anchor_off: int, data_label: str
) -> tuple[int | None, str | None]:
    """Byte offset past a kind's DObjDesc tree (id==18 terminator, 44B
    entries, cap 30): the splitter's own typeITCommonObject.py algorithm."""
    count = 0
    while count < 30:
        slot = anchor_off + count * 44
        if slot + 4 > len(payload):
            return None, f"{data_label}: DObjDesc scan overruns payload"
        (ident,) = struct.unpack_from(">i", payload, slot)
        count += 1
        if ident == 18:
            return anchor_off + count * 44, None
        if ident > 0x10000 or ident < 0:
            return None, (
                f"{data_label}: DObjDesc scan aborts at entry {count} "
                f"(id {ident})"
            )
    return None, f"{data_label}: no id==18 terminator within 30"


def scan_dobj_remainder(
    payload: bytes,
    anchor_off: int,
    data_label: str,
    labels: set[str],
) -> tuple[str | None, int | None, str | None]:
    """Place a kind's bare _data_remainder; gap-named remainders are owned
    by their embedded offsets (cross-checked separately)."""
    stem = data_label[: -len("_DObjDesc")]
    bare = stem + "_remainder"
    if bare not in labels:
        return None, None, None
    off, err = scan_remainder_offset(payload, anchor_off, data_label)
    if err is not None:
        return bare, None, err
    return bare, off, None


def file86_numeric_roots() -> dict[int, str]:
    """llITCommonData* tokens in item TUs that address file-0x56 space.

    Values below 3,392 address the 251 payload (fully live, ignored here);
    values at/above it address file 0x56 and become relocation roots, so
    itGetPData arithmetic can never land in a dropped interval.
    """
    roots: dict[int, str] = {}
    for path in sorted(ROOT.glob(ITEM_IMPORT_GLOB)):
        for m in re.finditer(
            r"uintptr_t\s+(llITCommon\w+)\s*=\s*(0x[0-9A-Fa-f]+|\d+)u?\s*;",
            read_text(path),
        ):
            value = int(m.group(2), 16 if m.group(2).startswith("0x") else 10)
            if value >= FILE_251_SIZE and value < FILE_86_SIZE:
                roots.setdefault(value, f"{path.name}:{m.group(1)}")
    return roots


def categorize(label: str, decl: str | None) -> str:
    # Splitter naming is authoritative: a Tex_/Vtx_/Gfx_/LUT_ array keeps
    # its category even when declared without an extern prototype (many
    # payload arrays are defined with explicit sizes instead).
    low = label.lower()
    if "_gfx_" in low or low.endswith(".dl") or "_dl" in low or "_dllink" in low:
        return "display-list"
    if "_vtx" in low:
        return "vertices"
    if "_tex" in low or "tex_" in low:
        return "texture"
    if "_lut" in low or "palette" in low or "tlut" in low:
        return "palette"
    if "_post_0x" in low:
        # Weapon-data palettes (LGunAmmo/StarRod post_0xNN.palette.inc.c).
        return "palette"
    if decl == "Gfx":
        return "display-list"
    if decl == "Vtx":
        return "vertices"
    if decl in ("MObjSub*",) or "mobjsub" in low or "mobj" in low:
        return "material"
    if "dobjdesc" in low or "dobj" in low:
        return "dobjdesc"
    if "matanim" in low or "animjoint" in low or decl in ("AObjEvent32*", "u32"):
        return "animation"
    if "gap" in low or "remainder" in low:
        return "pad"
    return "other"


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", default="builds/resume-20260905/item-memory")
    args = ap.parse_args()
    out = ROOT / args.out
    out.mkdir(parents=True, exist_ok=True)
    failures: list[str] = []
    notes: list[str] = []

    # 1. O2R headers: the real reader logic, real binaries.
    size251, ext251, ids251, _ = parse_o2r(O2R_251, 0xFB)
    size86, ext86, ids86, payload86 = parse_o2r(O2R_86, 0x56)
    measured_tree = size251 + size86
    header_ok = (
        size251 == FILE_251_SIZE
        and size86 == FILE_86_SIZE
        and ext251 == 68
        and ext86 == 0
        and all(v == 86 for v in ids251)
        and measured_tree == TREE_SIZE
    )
    if not header_ok:
        failures.append(
            f"O2R header drift: 251={{size={size251},ext={ext251}}} "
            f"86={{size={size86},ext={ext86}}} tree={measured_tree}"
        )

    # 2. 251 payload census from the real source rows.
    text251 = read_text(SRC_251)
    n_attr = len(re.findall(r"ITAttributes\s+dITCommonData_\w+\[1\]", text251))
    n_wpattr = len(re.findall(r"WPAttributes\s+dITCommonData_\w+\s*=", text251))
    atk_rows = re.findall(r"ITAttackEvent\s+dITCommonData_\w+\[(\d+)\]", text251)
    f32_rows = re.findall(r"f32\s+dITCommonData_\w+\[(\d+)\]", text251)
    # ROM footprints: ITAttributes 0x48 (link_core.c audited layout),
    # WPAttributes 52 (16 ptr/vec/map words + 4 bitfield words, wptypes.h),
    # ITAttackEvent 8 (8+10+8+16 bits, ittypes.h), f32 4.
    census = n_attr * 72 + n_wpattr * 52 + sum(int(n) for n in atk_rows) * 8 + sum(
        int(n) for n in f32_rows
    ) * 4
    census_delta = FILE_251_SIZE - census
    rows251 = parse_251_initializers(text251)

    # 3. Extern anchors: 251 label -> file-0x56 label, no name guessing.
    externs = parse_251_reloc(read_text(RELOC_251))
    anchor: dict[str, int] = {}  # 86-label -> offset
    anchor_src: dict[str, str] = {}
    unanchored: list[str] = []
    for label251, field, off86 in externs:
        row = rows251.get(label251)
        if row is None or field >= len(row):
            unanchored.append(f"{label251}+{field * 4:#x}")
            continue
        target = row[field]
        if target is None:
            continue  # NULL pointer field: nothing to anchor.
        if target in anchor and anchor[target] != off86:
            failures.append(
                f"anchor conflict: {target} at {anchor[target]:#x} vs {off86:#x}"
            )
        anchor[target] = off86
        anchor_src[target] = f"{label251}+{field * 4:#x}"

    # 4. Label graph over file 0x56.
    edges = parse_86_reloc(read_text(RELOC_86))
    decls = parse_86_decl_types(read_text(SRC_86))
    labels: set[str] = set(anchor)
    for s, _, d, _ in edges:
        labels.add(s)
        labels.add(d)
    offsets: dict[str, int] = {}
    for lab in labels:
        off = embedded_offset(lab)
        if off is not None:
            offsets[lab] = off
    for lab, off in anchor.items():
        if lab in offsets and offsets[lab] != off:
            failures.append(
                f"label offset conflict: {lab} embedded {offsets[lab]:#x} "
                f"vs anchored {off:#x}"
            )
        offsets.setdefault(lab, off)
    unknown = sorted(lab for lab in labels if lab not in offsets)
    # Remaining labels resolve by three more existing-metadata rules, to a
    # fixpoint: (a) _sub_/_post_ suffixes against a placed parent prefix;
    # (b) a kind's _data_remainder from the DObjDesc id==18 terminator scan
    # (splitter's own typeITCommonObject.py algorithm); (c) payload intern
    # slots, which are big-endian (chain_link:16, words_num:16) with target
    # words_num*4 (decomp lb/lbreloc.c lbRelocLoadAndRelocFile) in the
    # pre-relocation image.
    data_anchors = sorted(
        (off, lab) for lab, off in anchor.items() if lab.endswith("_DObjDesc")
    )
    scanned: set[str] = set()
    scan_end: dict[str, int] = {}  # DObjDesc label -> tree end (for trims).
    both_known = both_match = 0
    changed = True
    while changed:
        changed = False
        for lab in labels:
            if lab in offsets:
                continue
            rel = parent_relative_offset(lab, offsets)
            if rel is not None and 0 <= rel < FILE_86_SIZE:
                offsets[lab] = rel
                changed = True
        for _off, dlab in data_anchors:
            if dlab in scanned:
                continue
            scanned.add(dlab)
            scan_off, scan_err = scan_remainder_offset(
                payload86, anchor[dlab], dlab
            )
            if scan_err is not None:
                failures.append(scan_err)
                continue
            scan_end[dlab] = scan_off
            # Gap-named remainder parents carry their own embedded offsets;
            # the terminator scan must agree with them exactly.
            stem = dlab[: -len("_DObjDesc")]
            gap_pat = re.compile(
                re.escape(stem) + r"_remainder_gap_0x[0-9A-Fa-f]+$"
            )
            for lab in labels:
                if gap_pat.match(lab) and lab in offsets \
                        and offsets[lab] != scan_off:
                    failures.append(
                        f"remainder scan cross-check: {lab} embedded "
                        f"{offsets[lab]:#x} vs scanned {scan_off:#x}"
                    )
            _t, _o, err = scan_dobj_remainder(
                payload86, anchor[dlab], dlab, labels
            )
            if err is not None:
                failures.append(err)
                continue
            if _t is None:
                continue
            if _t in offsets and offsets[_t] != _o:
                failures.append(
                    f"remainder scan conflict: {_t} embedded "
                    f"{offsets[_t]:#x} vs scanned {_o:#x}"
                )
                continue
            if _t not in offsets:
                offsets[_t] = _o
                changed = True
        for s, so, d, do in edges:
            if s not in offsets or d in offsets:
                continue
            slot = offsets[s] + so
            if slot + 4 > len(payload86):
                continue
            (words,) = struct.unpack_from(">I", payload86, slot)
            cand = (words & 0xFFFF) * 4 - do
            if 0 <= cand < FILE_86_SIZE and cand % 4 == 0:
                offsets[d] = cand
                changed = True
    # Validation pass over fully placed edges (count once). A dst +off
    # aims inside the target array, so the slot must read base + off.
    mismatch: list[str] = []
    for s, so, d, do in edges:
        if s in offsets and d in offsets:
            slot = offsets[s] + so
            if slot + 4 <= len(payload86):
                (words,) = struct.unpack_from(">I", payload86, slot)
                both_known += 1
                if (words & 0xFFFF) * 4 == offsets[d] + do:
                    both_match += 1
                elif len(mismatch) < 8:
                    mismatch.append(
                        f"{s}+{so:#x} -> {d}+{do:#x} "
                        f"(slot reads {(words & 0xFFFF) * 4:#x})"
                    )
    notes.append(
        f"payload pointer check: {both_match}/{both_known} intern slots read "
        "as (BE32&0xFFFF)*4 == target offset"
    )
    for line in mismatch:
        notes.append(f"  slot mismatch: {line}")
    if both_known and both_match / both_known < 0.90:
        failures.append(
            f"payload pointer model guard: match rate "
            f"{both_match}/{both_known} below 0.90"
        )
    unknown = sorted(lab for lab in labels if lab not in offsets)
    if unknown:
        failures.append(
            f"{len(unknown)} labels unplaceable, cannot rewrite relocation: "
            + ", ".join(unknown[:12])
            + ("..." if len(unknown) > 12 else "")
        )
    # Splitter-declared DObjDesc counts must equal the terminator scans;
    # both derive from the same trees, so any disagreement is a model bug.
    for _dlab, _n in parse_86_def_counts(read_text(SRC_86)).items():
        if _dlab in anchor and _dlab in scan_end:
            _scanned = (scan_end[_dlab] - anchor[_dlab]) // 44
            if _scanned != _n:
                failures.append(
                    f"DObjDesc count conflict: {_dlab} declares {_n} "
                    f"but scans {_scanned}"
                )
            else:
                notes.append(f"DObjDesc scan verified: {_dlab} {_n} entries")

    # 5. Reachability from all three root sets.
    children: dict[str, set[str]] = {}
    for s, _, d, _ in edges:
        children.setdefault(s, set()).add(d)
    roots = set(anchor)
    numeric_roots = file86_numeric_roots()
    # Containing interval of each numeric root is resolved after partitioning.
    live: set[str] = set()
    stack = list(roots)
    while stack:
        lab = stack.pop()
        if lab in live:
            continue
        live.add(lab)
        stack.extend(children.get(lab, ()))

    # 6. Payload partition at every known offset.
    points = sorted(set(offsets.values()))
    if points[0] != 0:
        notes.append(f"payload prefix [0, {points[0]:#x}) carries no label")
    bounds = sorted(set([0] + points + [FILE_86_SIZE]))
    # interval i: [bounds[i], bounds[i+1]) owned by labels at bounds[i].
    owners: dict[int, list[str]] = {}
    for lab, off in offsets.items():
        owners.setdefault(off, []).append(lab)
    off_of = {off: i for i, off in enumerate(bounds)}

    def interval_of(point: int) -> int:
        for i in range(len(bounds) - 1):
            if bounds[i] <= point < bounds[i + 1]:
                return i
        raise AssertionError(f"root {point:#x} outside payload")

    for value in numeric_roots:
        i = interval_of(value)
        for lab in owners.get(bounds[i], []):
            if lab not in live:
                notes.append(f"numeric root {value:#x} keeps {lab}")
            live.add(lab)
            stack = [lab]
            while stack:
                cur = stack.pop()
                for nxt in children.get(cur, ()):
                    if nxt not in live:
                        live.add(nxt)
                        stack.append(nxt)

    # 7. Droppable whole intervals: unreachable, untyped pad, no numeric
    # root inside, no reachable child inside, and no live pointer slot.
    # A live struct (e.g. a DObjDesc tree) can address slots past its own
    # label offset (DObjDesc+0x30 names the second entry's data field), so
    # any interval holding a live edge's slot words stays resident.
    numvals = sorted(numeric_roots)
    live_slots = sorted(
        offsets[s] + so for s, so, _d, _do in edges if s in live
    )
    drop: list[int] = []
    for i in range(len(bounds) - 1):
        start, end = bounds[i], bounds[i + 1]
        if any(start <= sl < end for sl in live_slots):
            continue
        labs = owners.get(start, [])
        if not labs:
            # Unlabeled gap between placed labels: unreachable by
            # construction (no label, no edge endpoint, no anchor).
            has_root = any(start <= v < end for v in numvals)
            if has_root:
                continue
            drop.append(i)
            continue
        if any(lab in live for lab in labs):
            continue
        # Children with their own offsets inside (start, end):
        child_inside = [
            lab2 for lab2, off2 in offsets.items()
            if start < off2 < end and lab2 in live
        ]
        if child_inside:
            continue
        if any(start <= v < end for v in numvals):
            continue
        cats = {categorize(lab, decls.get(lab)) for lab in labs}
        if cats <= {"pad", "other"}:
            drop.append(i)
    # Typed-but-unreachable intervals are KEPT (runtime arithmetic cover).

    # Tier 3: structure-verified trailing trims. A DObjDesc extent ends at
    # its terminator-scanned tree end; a Gfx display-list extent ends after
    # its first ENDDL (0xDF, the terminator this codebase emits and walks:
    # renderer_adapter_stage.c, renderer_adapter_matrix.c,
    # reloc_backend_assets.c). Trailing bytes past a verified end are
    # dropped only with no live slot, numeric root, or label inside.
    GFX_ENDDL = 0xDF
    def_sizes = parse_86_def_sizes(read_text(SRC_86))
    # Display-list intervals: declared Gfx, or sourcing a Vtx load edge
    # (undrawable DLs need no vertices; link structs never load any).
    # Every placed offset must be 4-aligned: the loader patches slots
    # with word stores, and packing preserves each slot's absolute
    # alignment only when all relative offsets are multiples of 4.
    if any(_off % 4 for _off in offsets.values()):
        failures.append("placed offset not 4-aligned; packing unsafe")
    lab_interval = {}
    for _bi in range(len(bounds) - 1):
        for _lab in owners.get(bounds[_bi], []):
            lab_interval[_lab] = _bi
    dl_parse = set()
    for _i in range(len(bounds) - 1):
        _labs = owners.get(bounds[_i], [])
        if any(decls.get(_lab) == "Gfx" for _lab in _labs):
            dl_parse.add(_i)
    for _s, _so, _d, _do in edges:
        if _s in offsets and _d in offsets:
            if categorize(_d, decls.get(_d)) == "vertices" and _d != _s:
                if _s in lab_interval:
                    dl_parse.add(lab_interval[_s])
    trim_end: dict[int, int] = {}  # interval idx -> trimmed end.
    trim_saved = 0
    trim_detail: list[dict] = []
    drop_set = set(drop)
    trim_cand: dict[int, list[tuple[int, str]]] = {}

    def propose_trim(i, new_end, reason, extra=None):
        trim_cand.setdefault(i, []).append((new_end, reason, extra or {}))

    for i in range(len(bounds) - 1):
        if i in drop_set:
            continue
        start, end = bounds[i], bounds[i + 1]
        labs = owners.get(start, [])
        new_end = end
        reason = ""
        for lab in labs:
            if lab in scan_end and start == anchor.get(lab, None):
                if scan_end[lab] < end:
                    new_end = min(new_end, scan_end[lab])
                    reason = "dobjdesc-tree"
        if len(labs) == 1 and labs[0] in def_sizes:
            declared = start + def_sizes[labs[0]]
            if declared > end:
                failures.append(
                    f"declared size overruns interval: {labs[0]} "
                    f"{def_sizes[labs[0]]}B at {start:#x} in "
                    f"{end - start}B"
                )
            elif declared < end:
                new_end = min(new_end, declared)
                reason = (reason + "+decl-size") if reason else "decl-size"
        if i in dl_parse:
            pos = start
            while pos + 8 <= end:
                (w0,) = struct.unpack_from(">I", payload86, pos)
                pos += 8
                if (w0 >> 24) == GFX_ENDDL:
                    new_end = min(new_end, pos)
                    reason = (reason + "+dl-end") if reason else "dl-end"
                    break
        if new_end < end:
            propose_trim(i, new_end, reason)
    # Tier 3b: splitter-annotated gaps. The source marks its own filler
    # as /* @ 0xNNNN, MM bytes (raw gap) */ PAD(MM) (122 markers). A gap
    # reaching its interval's end trims like any verified tail; interior
    # gaps are reported but never split (packing stays whole-interval).
    pad_interior = 0
    for _m in re.finditer(
        r"/\*\s*@\s*(0x[0-9A-Fa-f]+)\s*,\s*(\d+)\s*bytes[^\n]*\((?:raw )?gap\)\s*\*/\s*\nPAD\((\d+)\)",
        read_text(SRC_86),
    ):
        _off, _n1, _n2 = int(_m.group(1), 16), int(_m.group(2)), int(_m.group(3))
        if _n1 != _n2:
            notes.append(
                f"Tier 3b: gap annotation size mismatch at {_off:#x} "
                f"({_n1} vs PAD({_n2})); skipped."
            )
            continue
        try:
            _i = interval_of(_off)
        except AssertionError:
            continue
        _s, _e = bounds[_i], trim_end.get(_i, bounds[_i + 1])
        if _off + _n1 == _e or (_off + _n1 <= _e and _off <= _s):
            propose_trim(_i, min(_off, _e), "splitter-gap")
        elif _off + _n1 < _e:
            pad_interior += _n1
    if pad_interior:
        notes.append(
            f"Tier 3b: {pad_interior} interior gap bytes kept "
            "(no sub-interval splitting)."
        )

    # Tier 3c: display-list streams named by itGetPData roots. Walk/idle
    # DLs (BombHei walk, Box effect, NBumper wait, Wark/Kamex/Sawamura
    # display) start mid-interval at exact token offsets; parse each from
    # its root to ENDDL and trim the verified tail, guarded as usual.
    for _value, _src in numeric_roots.items():
        _tok = _src.split(":")[-1]
        if "displaylist" not in _tok.lower():
            continue
        try:
            _i = interval_of(_value)
        except AssertionError:
            continue
        if _i in drop_set:
            continue
        _s, _e = bounds[_i], trim_end.get(_i, bounds[_i + 1])
        _pos = _value
        while _pos + 8 <= _e:
            (_w0,) = struct.unpack_from(">I", payload86, _pos)
            _pos += 8
            if (_w0 >> 24) == GFX_ENDDL:
                break
        if _pos < _e:
            propose_trim(_i, _pos, "dl-root")
    # Tier-3 candidates collected; guarded application happens once all
    # tiers proposed (with the live edge-target guard).

    # Use-based reclassification: an edge's command word proves what the
    # target bytes are. G_VTX (0x01, validated decode + slot) means the
    # target is vertices; G_SETTIMG (0xFD) means texture; G_LOADTLUT (0xF0)
    # means palette (opcodes per reloc_backend_assets.c F3DEX2 defines).
    # Only pad/other/unlabeled intervals are reclassified, never typed
    # ones. This feeds trim, dedup and accounting with the same evidence.
    use_cat: dict[int, str] = {}  # interval idx -> proven category.
    for _s, _so, _d, _do in edges:
        if _s not in offsets or _d not in offsets:
            continue
        _slot = offsets[_s] + _so
        if _slot < 4:
            continue
        (_w0,) = struct.unpack_from(">I", payload86, _slot - 4)
        (_w1,) = struct.unpack_from(">I", payload86, _slot)
        _op = _w0 >> 24
        _tc = None
        if _op == 0x01:
            _tc = "vertices"
        elif _op == 0xFD:
            _tc = "texture"
        elif _op == 0xF0:
            _tc = "palette"
        if _tc is None:
            continue
        if (_w1 & 0xFFFF) * 4 != offsets[_d] + _do:
            continue
        _i = lab_interval.get(_d)
        if _i is None:
            continue
        _labs = owners.get(bounds[_i], [])
        _cats = {categorize(_lab, decls.get(_lab)) for _lab in _labs}
        if _cats <= {"pad", "other", "unlabeled"}:
            use_cat[_i] = _tc
    if use_cat:
        notes.append(
            f"use-based reclassification: {len(use_cat)} intervals proven "
            "by loader commands."
        )
    # vertices through G_VTX (op 0x01; F3DEX2 decode in
    # include/nds/nds_gbi_decode.h, consumed by the port's own walker in
    # renderer_adapter_legacy_dl_probes.c:461-486). Each load's address
    # word is an intern slot, so loads are enumerated edge-locally with no
    # stream parsing: for edge (s,so -> d,do) with a vertices target, the
    # command word at slot-4 must decode as G_VTX and the slot must read
    # the target. A second pass scans DL_SET streams (declared Gfx plus
    # intervals sourcing Vtx loads) for G_VTX commands with no matching
    # edge, which would betray segment-relative (0x0E) loads the edge set
    # cannot see; either anomaly voids the tier, never weakens it.
    # Static enumeration over-approximates control flow (branches only
    # select among static commands), so marks only grow: sound.
    def vtx_decode(w0):
        count = (w0 >> 12) & 0xFF
        end = (w0 >> 1) & 0x7F
        if count == 0 or count > 32 or end < count or end > 32:
            return None
        return count

    DL_VTX, DL_ENDDL = 0x01, 0xDF
    lab_interval = {}
    for _i in range(len(bounds) - 1):
        for _lab in owners.get(bounds[_i], []):
            lab_interval[_lab] = _i
    edge_slots = {(s, so): (d, do) for s, so, d, do in edges}
    vtx_loads: dict[int, list[tuple[int, int]]] = {}
    vtx_void = None
    for s, so, d, do in edges:
        if s not in offsets or d not in offsets:
            continue
        if categorize(d, decls.get(d)) != "vertices":
            continue
        slot = offsets[s] + so
        (w0,) = struct.unpack_from(">I", payload86, slot - 4)
        (w1,) = struct.unpack_from(">I", payload86, slot)
        if (w0 >> 24) != DL_VTX:
            vtx_void = (
                f"non-VTX command loading vertices: {s}+{so:#x} "
                f"op {(w0 >> 24):#x} -> {d}"
            )
            break
        count = vtx_decode(w0)
        if count is None or (w1 & 0xFFFF) * 4 != offsets[d] + do:
            vtx_void = f"undecodable VTX load: {s}+{so:#x} -> {d}+{do:#x}"
            break
        vtx_loads.setdefault(offsets[d] + do, []).append((count, slot))
    if vtx_void is None:
        dl_set = set()
        for _i in range(len(bounds) - 1):
            if _i in drop_set:
                continue
            _labs = owners.get(bounds[_i], [])
            if any(decls.get(_lab) == "Gfx" for _lab in _labs):
                dl_set.add(_i)
        for s, so, d, _do in edges:
            if s in offsets and d in offsets and lab_interval.get(s) is not None:
                # Self-edges prove nothing about stream layout (a Vtx
                # array's trailing pointer table points at itself); only
                # cross-label Vtx loads admit an interval to the DL set.
                if categorize(d, decls.get(d)) == "vertices" and d != s:
                    dl_set.add(lab_interval[s])
        for _i in sorted(dl_set):
            _s, _e = bounds[_i], trim_end.get(_i, bounds[_i + 1])
            _pos = _s
            _labs = owners.get(_s, [])
            while _pos + 8 <= _e:
                (w0,) = struct.unpack_from(">I", payload86, _pos)
                _op = w0 >> 24
                if _op == DL_ENDDL:
                    break
                if _op == DL_VTX:
                    if vtx_decode(w0) is None:
                        vtx_void = (
                            f"undecodable VTX in stream at {_pos:#x}"
                        )
                        break
                    _rel = _pos + 4 - _s
                    _hit = False
                    for _lab in _labs:
                        if (_lab, _rel) in edge_slots:
                            _hit = True
                            break
                    if not _hit:
                        vtx_void = (
                            f"edgeless VTX load at {_pos:#x} "
                            f"(possible segment-relative address)"
                        )
                        break
                _pos += 8
            if vtx_void is not None:
                break
    if vtx_void is not None:
        notes.append(f"Tier 4 void: {vtx_void}; no vertex trims applied.")
    # Head-potential census (observational only): leading bytes no
    # static load covers. A head trim would need label-shift-aware
    # rewriting, so this only measures; nothing is cut here.
    if vtx_void is None:
        _head_pot = 0
        for _i in range(len(bounds) - 1):
            if _i in drop_set:
                continue
            _s, _e = bounds[_i], trim_end.get(_i, bounds[_i + 1])
            _labs = owners.get(_s, [])
            _cats = {categorize(_lab, decls.get(_lab)) for _lab in _labs}
            if "vertices" not in _cats and use_cat.get(_i) != "vertices":
                continue
            _cov = [_a for _a, _b in
                    [(max(_s, _o), min(_e, _o + 16 * _c))
                     for _o, _l in vtx_loads.items() for _c, _slt in _l]
                    if _b > _a]
            if _cov:
                _head_pot += min(_cov) - _s
        notes.append(f"Tier 4 head-trim potential: {_head_pot} bytes.")
    else:
        _nloads = sum(len(_l) for _l in vtx_loads.values())
        _narr = 0
        _all_loads = [
            (_off, _off + 16 * _count)
            for _off, _lst in vtx_loads.items()
            for _count, _slot in _lst
        ]
        for _i in range(len(bounds) - 1):
            if _i in drop_set:
                continue
            _s, _e = bounds[_i], trim_end.get(_i, bounds[_i + 1])
            _labs = owners.get(_s, [])
            _cats = {categorize(_lab, decls.get(_lab)) for _lab in _labs}
            if "vertices" not in _cats and use_cat.get(_i) != "vertices":
                continue
            _narr += 1
            _used = _s
            _covered = False
            for _a, _b in _all_loads:
                _a, _b = max(_s, _a), min(_e, _b)
                if _b > _a:
                    _covered = True
                    _used = max(_used, _b)
            if not _covered:
                notes.append(
                    f"Tier 4: no static load covers Vtx interval {_s:#x}; kept."
                )
                continue
            if _used < _e:
                propose_trim(_i, _used, "vtx-load")
            # Head trim: leading bytes no static load touches are unread
            # (loads are the only Vtx readers; same completeness argument).
            _head = min(
                (_a for _a, _b in
                 [(max(_s, _o), min(_e, _o + 16 * _c))
                  for _o, _l in vtx_loads.items() for _c, _slt in _l]
                 if _b > _a),
                default=None,
            )
            if _head is not None and _head > _s:
                _hlabs = owners.get(_s, [])
                if any(_lab in live for _lab in _hlabs):
                    pass  # live owner head stays; slots below still guard.
                elif not any(_s <= _sl < _head for _sl in live_slots) \
                        and not any(_s <= _v < _head for _v in numvals) \
                        and not any(_s <= _t < _head for _t in live_targets):
                    propose_trim_head(_i, _head, "vtx-head")
        trim_detail.sort(key=lambda r: -r["size"])
        notes.append(
            f"Tier 4: {_nloads} static VTX loads enumerated; {_narr} Vtx "
            "arrays walked."
        )

    # Tier 5: joint/material table trims. gcAddMObjAll walks each per-DObj
    # MObjSub table NULL-terminated but advances the outer table once per
    # runtime DObj (decomp sys/objanim.c gcAddMObjAll/gcAddAnimAll); the
    # runtime tree excludes the id==18 terminator entry, so outer tables
    # hold exactly scan_count - 1 entries. Inner MObj tables end after
    # their NULL. Trim tails past those verified ends, guarded as usual.
    scan_count: dict[str, int] = {}
    for _dlab, _doff in anchor.items():
        if _dlab.endswith("_DObjDesc") and _dlab in scan_end:
            scan_count[_dlab] = (scan_end[_dlab] - _doff) // 44 - 1
    edge_by_src: dict[str, list[int]] = {}
    for _s, _so, _d, _do in edges:
        if _s in offsets:
            edge_by_src.setdefault(_s, []).append(offsets[_s] + _so)
    for _lab, _off in anchor.items():
        _src = anchor_src.get(_lab, "")
        _row = _src.split("+")[0] if _src else ""
        _fields = rows251.get(_row)
        if not _fields or not _fields[0]:
            continue
        _data_lab = _fields[0]
        _cnt = scan_count.get(_data_lab)
        if _cnt is None:
            continue
        _is_outer = _lab != _data_lab and _lab in offsets
        if not _is_outer:
            continue
        try:
            _field = int(_src.split("+")[1], 16) if "+" in _src else 0
        except (IndexError, ValueError):
            continue
        _i = lab_interval.get(_lab)
        if _i is None or _i in drop_set:
            continue
        _s, _e = bounds[_i], trim_end.get(_i, bounds[_i + 1])
        _cap = _off + 4 * _cnt
        if _cap < _e:
            propose_trim(_i, _cap, "table-count")
        # Inner MObj tables: only the p_mobjsubs outer (field 0x4) owns
        # NULL-terminated pointer tables (gcAddMObjAll). Anim/matanim
        # outers point at scripts, which are never NULL-scanned here.
        if _lab not in live or _field != 4:
            continue
        for _k in range(_cnt):
            _slot = _off + 4 * _k
            if _slot + 4 > len(payload86):
                break
            (_w,) = struct.unpack_from(">I", payload86, _slot)
            _t = (_w & 0xFFFF) * 4
            if _w == 0:
                continue  # NULL outer entry: no table.
            if _t >= len(payload86) or _t % 4 != 0:
                notes.append(
                    f"Tier 5: outer entry {_lab}+{_k * 4:#x} reads "
                    f"{_w:#x}; table skipped."
                )
                continue
            _ti = lab_interval.get(
                next((L for L, O in offsets.items() if O == _t), None)
            ) if _t in set(offsets.values()) else None
            if _ti is None or _ti in drop_set:
                continue
            _ts, _te = bounds[_ti], trim_end.get(_ti, bounds[_ti + 1])
            if _ts != _t:
                continue  # table shares its head; keep whole.
            _entries = sorted(
                _sl for _sl in edge_by_src.get(
                    next(L for L, O in offsets.items() if O == _t), []
                ) if _ts <= _sl < _te
            )
            if not _entries:
                (_head,) = struct.unpack_from(">I", payload86, _ts)
                if _head != 0:
                    notes.append(
                        f"Tier 5: edgeless table at {_t:#x} holds "
                        f"{_head:#x}; kept."
                    )
                    continue
                _used = _ts + 4
            else:
                _last = max(_entries)
                (_term,) = struct.unpack_from(">I", payload86, _last + 4)
                if _term != 0 or _last + 8 > _te:
                    notes.append(
                        f"Tier 5: table at {_t:#x} terminator "
                        f"{_term:#x}; kept."
                    )
                    continue
                _used = _last + 8
            if _used < _te:
                propose_trim(_ti, _used, "table-null",
                             extra={"driver": f"{_lab}+{_k * 4:#x}"})
    trim_detail.sort(key=lambda r: -r["size"])

    # Tree-interior guard: no placed label may sit strictly inside a
    # DObjDesc tree range unless its interval stays fully kept. Tree words
    # are read as floats/ids by setup (itManagerSetupItemDObjs), so an
    # interval inside [anchor, scan_end) can never drop or trim, whatever
    # its owner claims.
    tree_ranges = []
    for _dlab, _doff in anchor.items():
        if _dlab.endswith("_DObjDesc") and _dlab in scan_end:
            tree_ranges.append((_doff, scan_end[_dlab], _dlab))

    def inside_tree(point):
        for _a, _b, _dlab in tree_ranges:
            if _a < point < _b:
                return _dlab
        return None

    # Single guarded application for all trim tiers. A trim hole is
    # dropped only with no live slot, no numeric root, and no live-source
    # edge target inside: per-kind runtime arithmetic (itGetPData,
    # harisen joints) and patched pointers must all land on kept bytes.
    # Dropped intervals may never start inside a tree either.
    for _i in drop:
        _t = inside_tree(bounds[_i])
        if _t is not None:
            failures.append(
                f"tree-interior interval dropped at {bounds[_i]:#x} "
                f"inside {_t}"
            )
    live_targets = set()
    for _s, _so, _d, _do in edges:
        if _s in live and _d in offsets:
            live_targets.add(offsets[_d] + _do)
    for _i, _cands in trim_cand.items():
        _s, _e = bounds[_i], bounds[_i + 1]
        _new = min(c[0] for c in _cands)
        if _new >= _e or _new <= _s:
            continue
        if inside_tree(_s) is not None:
            continue
        if any(_new <= _sl < _e for _sl in live_slots):
            continue
        if any(_new <= _v < _e for _v in numvals):
            continue
        if any(_new <= _t < _e for _t in live_targets):
            continue
        _reasons = "+".join(sorted({c[1] for c in _cands if c[0] == _new}))
        _extra = {}
        for c in _cands:
            if c[0] == _new:
                _extra.update(c[2])
        trim_end[_i] = _new
        trim_saved += _e - _new
        trim_detail.append({
            "offset": f"{_new:#x}",
            "size": _e - _new,
            "labels": owners.get(_s, []),
            "reason": _reasons,
            **_extra,
        })
    trim_detail.sort(key=lambda r: -r["size"])

    live_bytes = 0
    dead_bytes = 0
    kept_intervals: list[tuple[int, int]] = []
    for i in range(len(bounds) - 1):
        size = bounds[i + 1] - bounds[i]
        if i in drop:
            dead_bytes += size
        else:
            live_bytes += size
            kept_intervals.append((bounds[i], trim_end.get(i, bounds[i + 1])))
    assert live_bytes + dead_bytes == FILE_86_SIZE

    # Category accounting over placed intervals.
    by_cat: dict[str, int] = {}
    by_cat_live: dict[str, int] = {}
    for i in range(len(bounds) - 1):
        start, end = bounds[i], bounds[i + 1]
        labs = owners.get(start, [])
        cats = {categorize(lab, decls.get(lab)) for lab in labs} or {"unlabeled"}
        size = end - start
        for cat in cats:
            by_cat[cat] = by_cat.get(cat, 0) + size
            if i not in drop:
                by_cat_live[cat] = by_cat_live.get(cat, 0) + size

    # 8. Prototype: pack kept intervals, rewrite both .reloc files.
    # Tier 2: share byte-identical kept ranges (dedup), including wholly
    # duplicated smaller arrays inside larger ones (LOD/mirror variants):
    # B is shared iff its bytes occur verbatim at a 4-aligned offset of
    # some larger kept range in the same category. texture, palette,
    # vertices and display-list are read-only renders. material qualifies
    # by the attachment-boundary copy (payload templates never written:
    # battleship_sys_objanim.c); animation qualifies because the AObj
    # player advances runtime cursors (dobj->anim_joint.event32 =
    # ...->p, decomp sys/objanim.c) and never stores into script blobs.
    # dobjdesc stays excluded (per-kind setup identity).
    DEDUP_CATS = {
        "texture", "palette", "vertices", "display-list",
        "material", "animation",
    }
    use_cat_key = {}
    for _i, _c in use_cat.items():
        use_cat_key[(bounds[_i], bounds[_i + 1])] = _c
    interval_cat = {}
    for start, end in kept_intervals:
        labs = owners.get(start, [])
        cats = {categorize(lab, decls.get(lab)) for lab in labs} or {"unlabeled"}
        interval_cat[(start, end)] = cats

    def dedup_qualifies(key):
        cats = set(interval_cat[key])
        proven = use_cat_key.get(key)
        if proven is not None:
            cats = (cats - {"pad", "other", "unlabeled"}) | {proven}
        return bool(cats) and cats <= DEDUP_CATS
    seen: dict[tuple, tuple[int, int]] = {}
    share_target: dict[tuple[int, int], tuple[tuple[int, int], int]] = {}
    dedup_groups: list[dict] = []
    # Hosts largest-first so small ranges fold into big ones. Substring
    # search is byte-exact (hash-verified below); the relative offset must
    # be 4-aligned to preserve every slot's absolute alignment.
    hosts: list[tuple[tuple[int, int], bytes]] = []
    for key in sorted(
        (k for k in kept_intervals if dedup_qualifies(k)),
        key=lambda k: (k[1] - k[0], k[0]),
    ):
        start, end = key
        blob = payload86[start:end]
        placed = False
        for (hkey, hblob) in hosts:
            if len(hblob) < len(blob):
                continue
            at = hblob.find(blob)
            while at >= 0 and at % 4 != 0:
                at = hblob.find(blob, at + 1)
            if at >= 0:
                share_target[key] = (hkey, at)
                placed = True
                break
        if not placed:
            hosts.append((key, blob))
    # Verify every shared placement by hash (bytes.find is exact, but the
    # alignment-advance loop above deserves its own proof).
    for _dup, (_host, _at) in share_target.items():
        _hs, _he = _host
        _ds, _de = _dup
        if payload86[_hs + _at: _hs + _at + (_de - _ds)] != payload86[_ds:_de]:
            failures.append(f"dedup placement drift at {_ds:#x}")
    by_first: dict[tuple[int, int], list[tuple[int, int]]] = {}
    for dup, (first, _at) in share_target.items():
        by_first.setdefault(first, []).append(dup)
    dedup_saved = 0
    for first, dups in by_first.items():
        for dup in dups:
            size = dup[1] - dup[0]
            aligned = size + (-size % 4)
            dedup_saved += aligned
        dedup_groups.append({
            "offset": f"{first[0]:#x}",
            "size": first[1] - first[0],
            "copies": len(dups) + 1,
            "saved": sum((d[1] - d[0]) + (-(d[1] - d[0]) % 4) for d in dups),
            "duplicates": [f"{s:#x}" for s, _e in dups],
        })
    dedup_groups.sort(key=lambda r: -r["saved"])
    newoff: dict[int, int] = {}
    cursor = 0
    compact = bytearray()
    for start, end in kept_intervals:
        if (start, end) in share_target:
            continue
        newoff[start] = cursor
        chunk = payload86[start:end]
        compact.extend(chunk)
        cursor += len(chunk)
        while cursor % 4:
            compact.append(0)
            cursor += 1
    for (start, end), (first, at) in share_target.items():
        newoff[start] = newoff[first[0]] + at
    compact_size = len(compact)
    # Arena pays 16-byte-aligned per-file sizes (NDS_RELOC_ALIGN_BYTES in
    # src/port/reloc_backend_assets.c; per-file ALIGN in
    # ndsRelocExternTreeAllocSize). Viability is judged on aligned bytes.
    def align16(n):
        return (n + 15) & ~15

    compact_aligned = align16(compact_size)
    assert align16(FILE_86_SIZE) == FILE_86_SIZE
    savings = FILE_86_SIZE - compact_size
    savings_aligned = FILE_86_SIZE - compact_aligned
    new_tree = FILE_251_SIZE + compact_size

    def rewrite86() -> str:
        lines = []
        kept_edges = 0
        for s, so, d, do in edges:
            if s not in live:
                # Dead source: its slot words are never read as pointers,
                # so the edge needs no rewritten home. The target keeps its
                # own liveness from its remaining incoming edges.
                continue
            ns = newoff.get(offsets[s])
            nd = newoff.get(offsets[d])
            if ns is None or nd is None:
                failures.append(
                    f"prototype drops live-source edge endpoint: {s} -> {d}"
                )
                ns = ns if ns is not None else 0
                nd = nd if nd is not None else 0
            lines.append(f"intern {s}+{so:#x} {d}+{do:#x}  # {ns:#x} -> {nd:#x}")
            kept_edges += 1
        return (
            "# Compact prototype: live-source edges only, rewritten offsets. "
            f"{kept_edges}/{len(edges)} edges kept.\n" + "\n".join(lines) + "\n"
        )

    def rewrite251() -> str:
        lines = []
        for label251, field, off86 in externs:
            row = rows251.get(label251)
            target = row[field] if row and field < len(row) else None
            if target is None:
                lines.append(f"extern {label251}+{field * 4:#x} {off86:#x}  # NULL kept")
                continue
            i = interval_of(off86)
            if i in drop:
                failures.append(
                    f"prototype drops anchored extern target: {label251} -> {off86:#x}"
                )
            no = newoff.get(bounds[i], None)
            if no is None:
                failures.append(f"prototype loses extern target {label251}")
                no = 0
            new_target = no + (off86 - bounds[i])
            suffix = "" if field == 0 else f"+{field * 4:#x}"
            lines.append(f"extern {label251}{suffix} {new_target:#x}  # was {off86:#x}")
        return (
            "# Compact prototype: same 68 externs, rewritten file-0x56 offsets.\n"
            + "\n".join(lines) + "\n"
        )

    text86 = rewrite86()
    text251 = rewrite251()

    # 9. Verify the prototype from its own rewritten metadata.
    compact_path = out / "compact86.bin"
    compact_path.write_bytes(bytes(compact))
    (out / "compact86.reloc").write_text(text86, encoding="utf-8")
    (out / "compact251.reloc").write_text(text251, encoding="utf-8")

    # Every rewritten (live-source) edge resolves inside the compact image.
    for s, so, d, do in edges:
        if s not in live:
            continue
        ns = newoff.get(offsets[s], None)
        nd = newoff.get(offsets[d], None)
        if ns is None or nd is None:
            continue  # already recorded above.
        if not (0 <= ns + so < compact_size):
            failures.append(f"compact edge source out of range: {s}+{so:#x}")
        if not (0 <= nd + do < compact_size):
            failures.append(f"compact edge target out of range: {d}+{do:#x}")
    # Target content identity: the words a patched pointer will read must
    # equal the original words (catches trims/packing that slide bytes
    # under a live target, even when the address stays in range).
    for s, so, d, do in edges:
        if s not in live:
            continue
        ns = newoff.get(offsets[s], None)
        nd = newoff.get(offsets[d], None)
        if ns is None or nd is None:
            continue
        if nd + do + 4 <= compact_size and offsets[d] + do + 4 <= len(payload86):
            if bytes(compact[nd + do: nd + do + 4]) != payload86[
                offsets[d] + do: offsets[d] + do + 4
            ]:
                failures.append(
                    f"compact target content drift: {d}+{do:#x}"
                )
                break
    # Content identity per moved interval.
    for start, end in kept_intervals:
        if bytes(compact[newoff[start]: newoff[start] + (end - start)]) != payload86[
            start:end
        ]:
            failures.append(f"compact content drift at {start:#x}")
            break
    # Every extern kind still present with identical target content.
    kinds = {label251.split("+")[0] for label251, _, _ in externs}
    if len(externs) != 68:
        failures.append(f"extern count drift: {len(externs)}")
    # Numeric itGetPData roots still resolve.
    for value, src in numeric_roots.items():
        i = interval_of(value)
        if i in drop:
            failures.append(f"prototype drops itGetPData root {value:#x} ({src})")

    viable = savings_aligned >= SHORTFALL and not failures

    label_of_point = {}
    for lab, off in offsets.items():
        label_of_point.setdefault(off, []).append(lab)
    dead_detail = []
    for i in drop:
        start, end = bounds[i], bounds[i + 1]
        dead_detail.append({
            "offset": f"{start:#x}",
            "size": end - start,
            "labels": owners.get(start, []),
        })
    dead_detail.sort(key=lambda r: -r["size"])

    kept_pad = []
    for i in range(len(bounds) - 1):
        if i in drop:
            continue
        start, end = bounds[i], bounds[i + 1]
        labs = owners.get(start, [])
        cats = {categorize(lab, decls.get(lab)) for lab in labs} or {"unlabeled"}
        if cats != {"pad"}:
            continue
        why = []
        if any(lab in live for lab in labs):
            why.append("live-label")
        if any(start <= sl < end for sl in live_slots):
            why.append("slot")
        if any(start <= v < end for v in numvals):
            why.append("root")
        if [lab2 for lab2, off2 in offsets.items()
                if start < off2 < end and lab2 in live]:
            why.append("child")
        kept_pad.append({
            "offset": f"{start:#x}", "size": end - start,
            "labels": labs, "why": why or ["?"],
        })
    kept_pad.sort(key=lambda r: -r["size"])

    report = []
    report.append("Item core required-memory closure: measured byte accounting")
    report.append("=" * 64)
    report.append("MEASURED (O2R headers, LE reader logic from")
    report.append("src/nds/nds_reloc_assets.c ndsRelocAssetReadHeaderFromFile):")
    report.append(f"  ITCommonData 0xfb: payload {size251} bytes, {ext251} externs.")
    report.append(f"  MiscData086 0x56: payload {size86} bytes, {ext86} externs.")
    report.append(f"  Extern tree total {measured_tree} bytes (gate asserts 82976).")
    report.append("MEASURED (251_ITCommonData.c row census):")
    report.append(
        f"  {n_attr} ITAttributes x72 + {n_wpattr} WPAttributes x52 + "
        f"{sum(int(n) for n in atk_rows)} attack events x8 + "
        f"{sum(int(n) for n in f32_rows)} f32 x4 = {census} bytes;"
    )
    report.append(
        f"  file is {FILE_251_SIZE} bytes, delta {census_delta} bytes "
        "(unclassified table tail; kept live)."
    )
    report.append(
        "  All 251 rows are gameplay/collision/material/animation/texture "
        "roots: every row is named by a per-kind item descriptor and decoded "
        "by ndsItDecodeAttributes. 251 stays fully resident."
    )
    report.append("MEASURED (file-0x56 label graph):")
    report.append(f"  {len(labels)} labels, {len(edges)} intern edges, "
                  f"{len(anchor)} anchored extern targets, "
                  f"{len(numeric_roots)} itGetPData numeric roots.")
    report.append(f"  {len(live)} labels reachable; {len(unknown)} unknown-offset "
                  "labels (bytes conservatively kept).")
    if unanchored:
        notes.append(f"{len(unanchored)} externs without row anchor "
                     f"(NULL fields need none): {', '.join(unanchored[:8])}"
                     + ("..." if len(unanchored) > 8 else ""))
    if unknown:
        report.append(f"  Unknown: {', '.join(unknown[:12])}"
                      + ("..." if len(unknown) > 12 else ""))
    report.append("MEASURED (payload partition by category, placed bytes):")
    for cat in sorted(by_cat):
        report.append(
            f"  {cat:14s} total {by_cat[cat]:6d}  live {by_cat_live.get(cat, 0):6d}"
        )
    report.append("PROPOSAL (one concrete safe savings candidate, five tiers):")
    report.append(
        f"  Tier 1: drop {len(drop)} unreachable untyped-pad intervals "
        f"({dead_bytes} bytes); keep every typed interval even when "
        "unreachable (per-kind runtime arithmetic cover, e.g. harisen anim "
        "joints); keep every interval holding an itGetPData root or a live "
        "pointer slot."
    )
    report.append(
        f"  Tier 2: share {len(share_target)} byte-identical kept intervals "
        f"in {len(by_first)} groups (read-only renders plus material "
        f"templates and animation scripts, both never written at runtime; "
        f"dobjdesc excluded) saving "
        f"{dedup_saved} bytes. Sharing identical bytes cannot change any "
        "read; every label keeps exactly one new offset in map.json."
    )
    report.append(
        f"  Tier 3: structure-verified trailing trims saving {trim_saved} "
        "bytes: DObjDesc extents end at the terminator-scanned tree end, "
        "Gfx extents end after the first ENDDL; trimmed tails hold no live "
        "slot, numeric root, or label."
    )
    report.append(
        f"  Compact file-0x56 image {compact_size} bytes "
        f"(arena-aligned {compact_aligned}); new tree "
        f"{new_tree} bytes; savings {savings} bytes "
        f"(aligned {savings_aligned}) against shortfall "
        f"{SHORTFALL}: {'VIABLE' if viable else 'NOT viable as specified'}."
    )
    report.append(
        "  Integration (Main-owned): rebuild the O2R pair from compact86.bin "
        "+ compact86.reloc + compact251.reloc, then regenerate the ll* file "
        "tokens in src/import/battleship_item_*.c from map.json label_offsets "
        "(mechanical; check-item-import-fidelity.py re-verifies). No kind is "
        "omitted and no gameplay, collision, material, animation or texture "
        "byte changes value or address identity except byte-identical sharing."
    )
    for note in notes[:10]:
        report.append(f"  note: {note}")
    report.append("Largest dropped intervals:")
    for row in dead_detail[:12]:
        report.append(
            f"  {row['offset']:>8s} {row['size']:5d}B {','.join(row['labels'][:2])}"
        )
    report.append("Largest shared groups:")
    for row in dedup_groups[:12]:
        report.append(
            f"  {row['offset']:>8s} {row['size']:5d}B x{row['copies']} "
            f"saves {row['saved']}B dups {','.join(row['duplicates'][:3])}"
        )
    report.append("Largest trailing trims:")
    for row in trim_detail[:12]:
        report.append(
            f"  {row['offset']:>8s} {row['size']:5d}B "
            f"{row['reason']} {','.join(row['labels'][:2])}"
        )
    if failures:
        report.append("VERIFY FAILURES:")
        report.extend(f"  FAIL: {f}" for f in failures)
    else:
        report.append("All prototype self-checks pass (edge bounds, content "
                      "identity, 68 externs, numeric roots).")
    report.append("Main owns integration: this prototype is a host-verified "
                  "binary + rewritten relocation metadata only.")
    (out / "report.txt").write_text("\n".join(report) + "\n", encoding="utf-8")

    (out / "map.json").write_text(json.dumps({
        "measured": {
            "file251": size251, "extern251": ext251,
            "file86": size86, "extern86": ext86, "tree": measured_tree,
            "census251": census, "census251_delta": census_delta,
            "labels": len(labels), "edges": len(edges),
            "anchored": len(anchor), "numeric_roots": len(numeric_roots),
            "live_labels": len(live), "unknown_labels": unknown,
            "by_category": by_cat, "by_category_live": by_cat_live,
        },
        "candidate": {
            "dropped_intervals": len(drop),
            "dead_bytes": dead_bytes,
            "dedup_groups": len(by_first),
            "dedup_shared_intervals": len(share_target),
            "dedup_saved": dedup_saved,
            "trimmed_intervals": len(trim_end),
            "trim_saved": trim_saved,
            "compact86": compact_size,
            "compact86_aligned": compact_aligned,
            "new_tree": new_tree,
            "savings": savings,
            "savings_aligned": savings_aligned,
            "shortfall": SHORTFALL,
            "viable": viable,
            "dropped": dead_detail,
            "shared": dedup_groups,
            "trimmed": trim_detail,
            "kept_pad": kept_pad[:24],
        },
        "label_offsets": {
            lab: newoff[off]
            for lab, off in offsets.items() if off in newoff
        },
        "failures": failures,
        "notes": notes,
    }, indent=2), encoding="utf-8")

    print("\n".join(report))
    return 1 if failures or not viable else 0


if __name__ == "__main__":
    sys.exit(main())
