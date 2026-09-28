#!/usr/bin/env python3
"""Lane 2: typed byte partition of a DObj-style relocData file.

Sources (all read-only, all in-tree):
  decomp/BattleShip-main/decomp/src/relocData/<id>_<Name>.c      typed labels
  decomp/BattleShip-main/decomp/src/relocData/<id>_<Name>.reloc  intern edges
  decomp/BattleShip-main/BattleShip_o2r/<dir>/<Name>              real bytes
  src/import/battleship_efmanager_symbols.h                       ll* roots

The splitter that produced the .c files names every block with its type and,
for 90+ % of them, its absolute payload offset (`Tex_0x0008`,
`gap_0x0000_sub_0x5948`).  Offsets for the rest are recovered from (a) the
generated ll* root table, (b) intern edges: slot = off(src)+so reads
(BE32 & 0xFFFF)*4 = off(dst)+do, iterated to a fixpoint.  This is the same
placement model scripts/items/item_memory_closure.py uses for file 86 (whose
own report this tool reproduces, see lane2_classify_86.py).

Classes (byte totals over the O2R payload):
  display_list  Gfx arrays (trimmed at first ENDDL 0xDF), DObjDLLink lists
  vertices      Vtx arrays
  texture       u8 *Tex* arrays; palette = u16 *palette*/LUT arrays
  animation     AObjEvent32 scripts / AnimJoint / MatAnimJoint pointer tables
  material      MObjSub + sprite/palette pointer tables
  dobjdesc      DObjDesc trees
  attribute     ITAttributes / WPAttributes / ITAttackEvent tables (file 251)
  other_data    typed data that is none of the above (u32 tables etc.)
  pad           PAD()/gap filler and alignment slack inside placed intervals
  unplaced      bytes in intervals owned only by labels this tool could not place
"""
from __future__ import annotations

import re
import struct
import sys
from collections import defaultdict
from pathlib import Path

import lane2_o2r as o2r

REPO = o2r.REPO
RELOC_DIR = REPO / "decomp/BattleShip-main/decomp/src/relocData"
EF_SYMBOLS = REPO / "src/import/battleship_efmanager_symbols.h"

ELEM = {"Gfx": 8, "Vtx": 16, "u8": 1, "u16": 2, "u32": 4, "DObjDesc": 44,
        "DObjDLLink": 8, "MObjSub": 120}


def embedded_offset(label: str):
    nums = re.findall(r"(?<!sub)(?<!post)_0x([0-9A-Fa-f]+)", label)
    if not nums:
        return None
    base = int(nums[-1], 16)
    m = re.search(r"_sub_0x([0-9A-Fa-f]+)", label)
    if m:
        base += int(m.group(1), 16)
    return base


def parent_relative(label: str, offsets: dict):
    m = re.search(r"(_sub_0x[0-9A-Fa-f]+)$", label)
    if not m:
        return None
    parent = label[: -len(m.group(0))]
    if parent in offsets:
        return offsets[parent] + int(m.group(1).split("_0x")[1], 16)
    return None


def categorize(label: str, typ: str | None) -> str:
    low = label.lower()
    if "_gfx_" in low or low.endswith(".dl") or "_dl" in low or "_dllink" in low:
        return "display_list"
    if "_vtx" in low:
        return "vertices"
    if "_tex" in low or "tex_" in low:
        return "texture"
    if "_lut" in low or "palette" in low or "tlut" in low:
        return "palette"
    if typ == "Gfx":
        return "display_list"
    if typ == "DObjDLLink":
        return "display_list"
    if typ == "Vtx":
        return "vertices"
    if typ and typ.startswith("MObjSub") or "mobjsub" in low or "mobj" in low:
        return "material"
    if "dobjdesc" in low or "dobj" in low:
        return "dobjdesc"
    if "matanim" in low or "animjoint" in low or "anim" in low:
        return "animation"
    if "attributes" in low or "attackevent" in low:
        return "attribute"
    if "gap" in low or "remainder" in low:
        return "pad"
    if typ in ("u32", "u32 *"):
        return "other_data"
    return "other_data"


def parse_c(path: Path):
    text = path.read_text(encoding="utf-8", errors="replace")
    defs = []  # in file order: (type, label, count|None)
    for m in re.finditer(
            r"^(?!extern)([A-Za-z_][\w\s\*]*?)\s+(\w+)\s*(?:\[(\d*)\])?\s*=\s*\{",
            text, re.M):
        typ = re.sub(r"\s+", "", m.group(1))
        defs.append((typ, m.group(2), int(m.group(3)) if m.group(3) else None))
    decl = {}
    for m in re.finditer(r"^extern\s+([A-Za-z_][\w\s\*]*?)\s+(\w+)\s*\[\]", text, re.M):
        decl[m.group(2)] = re.sub(r"\s+", "", m.group(1))
    return text, defs, decl


def parse_reloc(path: Path):
    edges = []
    for line in path.read_text(encoding="utf-8", errors="replace").splitlines():
        parts = line.split()
        if len(parts) < 3 or parts[0] != "intern":
            continue
        sm = re.match(r"(\w+)(?:\+0x([0-9A-Fa-f]+))?$", parts[1])
        dm = re.match(r"(\w+)(?:\+0x([0-9A-Fa-f]+))?$", parts[2])
        if not sm or not dm:
            continue
        so = int(sm.group(2), 16) if sm.group(2) else 0
        if re.match(r"0x[0-9A-Fa-f]+$", dm.group(1)):
            edges.append((sm.group(1), so, None, int(dm.group(1), 16)))
        else:
            edges.append((sm.group(1), so, dm.group(1),
                          int(dm.group(2), 16) if dm.group(2) else 0))
    return edges


def ll_roots(prefix: str):
    """llEFCommonEffects1Foo = 0xNNNN  ->  normalised name -> offset."""
    out = {}
    for m in re.finditer(r"uintptr_t\s+ll(" + prefix + r"\w*)\s*=\s*(0x[0-9A-Fa-f]+)u?",
                         EF_SYMBOLS.read_text()):
        out[m.group(1)] = int(m.group(2), 16)
    return out


def norm(name: str) -> str:
    return re.sub(r"[_]", "", name)


def partition(reloc_stem: str, o2r_name: str, label_prefix: str, ll_prefix: str | None):
    f = o2r.by_name(o2r_name)
    text, defs, decl = parse_c(RELOC_DIR / f"{reloc_stem}.c")
    edges = parse_reloc(RELOC_DIR / f"{reloc_stem}.reloc")
    payload = f.payload
    typ_of = {label: typ for typ, label, _ in defs}
    for lab, t in decl.items():
        typ_of.setdefault(lab, t)
    cnt_of = {label: c for _, label, c in defs}
    labels = [label for _, label, _ in defs]
    labelset = set(labels)
    for s, _, d, _ in edges:
        labelset.add(s)
        if d:
            labelset.add(d)
    offsets = {}
    for lab in sorted(labelset):          # sorted: set order is hash-randomised per process
        e = embedded_offset(lab)
        if e is not None and e < f.data_size:
            offsets[lab] = e
    # A `<DL>_DLLink` label is the link list the splitter emitted for the display list
    # `<DL>`; its name carries the DL's offset, but it lives right AFTER the list
    # (E.g. EF1: Gfx[14] at 0x5948, DObjDLLink[2] at 0x59B8).  Two labels at one
    # offset made the class/extent of that interval depend on set iteration order.
    for lab in sorted(labelset):
        if lab.endswith("_DLLink") and lab in offsets:
            base = lab[: -len("_DLLink")]
            if (base in offsets and offsets[base] == offsets[lab]
                    and typ_of.get(base) == "Gfx" and cnt_of.get(base)):
                new = offsets[base] + ELEM["Gfx"] * cnt_of[base]
                if new < f.data_size:
                    offsets[lab] = new
    anchored = 0
    if ll_prefix:
        roots = ll_roots(ll_prefix)
        rn = {norm(k): v for k, v in roots.items()}
        for lab in sorted(labelset):
            n = norm(lab)
            n = re.sub(r"^d", "", n)
            for k, v in rn.items():
                if n == k:
                    if lab in offsets and offsets[lab] != v:
                        pass
                    offsets.setdefault(lab, v)
                    anchored += 1
    changed = True
    while changed:
        changed = False
        for lab in sorted(labelset):
            if lab in offsets:
                continue
            rel = parent_relative(lab, offsets)
            if rel is not None and 0 <= rel < f.data_size:
                offsets[lab] = rel
                changed = True
        for s, so, d, do in edges:
            if s in offsets and d and d not in offsets:
                slot = offsets[s] + so
                if slot + 4 <= f.data_size:
                    w = struct.unpack_from(">I", payload, slot)[0]
                    cand = (w & 0xFFFF) * 4 - do
                    if 0 <= cand < f.data_size and cand % 4 == 0:
                        offsets[d] = cand
                        changed = True
            if d and d in offsets and s not in offsets:
                # find src slot via chain: unknown; skip
                pass
    unplaced = sorted(l for l in labelset if l not in offsets)
    # pointer model check
    ok = tot = 0
    for s, so, d, do in edges:
        if s in offsets and d and d in offsets:
            slot = offsets[s] + so
            if slot + 4 <= f.data_size:
                w = struct.unpack_from(">I", payload, slot)[0]
                tot += 1
                ok += ((w & 0xFFFF) * 4 == offsets[d] + do)
    return f, typ_of, cnt_of, offsets, unplaced, (ok, tot), edges


def classify(f, typ_of, cnt_of, offsets, extra_marks=None):
    """Return (class totals, interval rows)."""
    by_off = defaultdict(list)
    for lab, off in offsets.items():
        by_off[off].append(lab)
    bounds = sorted(set([0] + list(by_off) + [f.data_size]))
    rows = []
    totals = defaultdict(int)
    for i in range(len(bounds) - 1):
        a, b = bounds[i], bounds[i + 1]
        labs = sorted(by_off.get(a, []))
        if not labs:
            cls = "unplaced"
            rows.append((a, b - a, cls, "(no label)", 0))
            totals[cls] += b - a
            continue
        # choose the most specific class among co-located labels
        cats = []
        for lab in labs:
            cats.append((categorize(lab, typ_of.get(lab)), lab))
        pri = ["display_list", "vertices", "texture", "palette", "material",
               "animation", "dobjdesc", "attribute", "other_data", "pad"]
        cats.sort(key=lambda c: pri.index(c[0]) if c[0] in pri else 99)
        cls, lab = cats[0]
        size = b - a
        used = size
        t = typ_of.get(lab)
        n = cnt_of.get(lab)
        if n and t in ELEM and len(labs) == 1:
            used = min(size, n * ELEM[t])
        # Gfx: trim at first ENDDL
        if cls == "display_list" and t == "Gfx":
            pos = a
            end = a
            while pos + 8 <= b:
                w0 = struct.unpack_from(">I", f.payload, pos)[0]
                pos += 8
                end = pos
                if (w0 >> 24) == 0xDF:
                    break
            used = min(used, end - a)
        rows.append((a, used, cls, lab, size - used))
        totals[cls] += used
        if size - used:
            totals["pad"] += size - used
    return dict(totals), rows


LAST_TYPES = {}


def run(reloc_stem, o2r_name, label_prefix, ll_prefix, verbose=True):
    f, typ_of, cnt_of, offsets, unplaced, chk, edges = partition(
        reloc_stem, o2r_name, label_prefix, ll_prefix)
    LAST_TYPES.clear()
    LAST_TYPES.update(typ_of)
    totals, rows = classify(f, typ_of, cnt_of, offsets)
    if verbose:
        print(f"== {o2r_name} fid={f.file_id} payload={f.data_size} "
              f"labels_placed={len(offsets)} unplaced={len(unplaced)} "
              f"pointer_check={chk[0]}/{chk[1]}")
        for k in sorted(totals):
            print(f"   {k:14s} {totals[k]:8d}")
        print(f"   {'SUM':14s} {sum(totals.values()):8d}")
        if unplaced:
            print("   unplaced labels:", ", ".join(unplaced[:10]),
                  "..." if len(unplaced) > 10 else "")
    return f, totals, rows, offsets, unplaced


if __name__ == "__main__":
    for stem, name, pre, ll in (
            ("83_EFCommonEffects1", "EFCommonEffects1", "dEFCommonEffects1", "EFCommonEffects1"),
            ("84_EFCommonEffects2", "EFCommonEffects2", "dEFCommonEffects2", "EFCommonEffects2"),
            ("85_EFCommonEffects3", "EFCommonEffects3", "dEFCommonEffects3", "EFCommonEffects3"),
            ("166_IFCommonPlayer", "IFCommonPlayer", "dIFCommonPlayer", None),
            ("86_ITCommonObject", "MiscData086", "dITCommonObject", None)):
        run(stem, name, pre, ll)
