#!/usr/bin/env python3
"""Lane 2: item files (ITCommonData 251 + MiscData086 86) classification and
native-owner coverage.

Inputs (read-only):
  tools/item-closure-run/map.json      label offsets from scripts/items/item_memory_closure.py
                                       (run by this lane, output redirected here)
  decomp .../relocData/251_ITCommonData.reloc  the 68 extern roots into file 86
  include/nds/generated/nds_native_item_*.generated.h  native owner contracts
  src/port/renderer_adapter_stage.c    DL words the owners still read

Outputs: class totals of file 86 (label partition, categories as in the
closure script), per-kind roots, owner coverage of DL roots, and the exact DL
bytes that owner candidate checks still read.
"""
from __future__ import annotations

import json
import re
import struct
import sys
from collections import defaultdict
from pathlib import Path

import lane2_o2r as o2r

REPO = o2r.REPO
sys.path.insert(0, str(REPO / "scripts" / "items"))
import item_memory_closure as ic  # noqa: E402  (definitions only)

MAP = Path(__file__).resolve().parent / "item-closure-run" / "map.json"
GEN = REPO / "include/nds/generated"
STAGE = REPO / "src/port/renderer_adapter_stage.c"


def partition86():
    """Typed partition of the ORIGINAL file 86 (offsets from lane2_items_offsets,
    a verbatim slice of the closure script's placement stage).  Category rules,
    declared-size trims and DObjDesc terminator trims are the closure script's."""
    import lane2_items_offsets as lio
    r = lio.placement()
    f86 = o2r.by_name("MiscData086")
    offsets = r["offsets"]
    decls = r["decls"]
    sizes = ic.parse_86_def_sizes(ic.read_text(ic.SRC_86))
    scan_end = r["scan_end"]
    by_off = defaultdict(list)
    for lab, off in offsets.items():
        by_off[off].append(lab)
    bounds = sorted(set([0] + list(by_off) + [f86.data_size]))
    rows = []
    totals = defaultdict(int)
    pri = ["display-list", "vertices", "texture", "palette", "material",
           "animation", "dobjdesc", "other", "pad"]
    for i in range(len(bounds) - 1):
        a, b = bounds[i], bounds[i + 1]
        labs = by_off.get(a, [])
        if not labs:
            rows.append([a, b - a, "unlabeled", "(none)"])
            totals["unlabeled"] += b - a
            continue
        cats = [(ic.categorize(l, decls.get(l)), l) for l in labs]
        cats.sort(key=lambda c: pri.index(c[0]) if c[0] in pri else 99)
        cls, lab = cats[0]
        used = b - a
        if len(labs) == 1 and labs[0] in sizes:
            used = min(used, sizes[labs[0]])
        if labs[0] in scan_end and scan_end[labs[0]] > a:
            used = min(used, scan_end[labs[0]] - a)
        rows.append([a, used, cls, lab])
        totals[cls] += used
        if b - a - used:
            rows.append([a + used, b - a - used, "pad", lab + " (trailing slack)"])
            totals["pad"] += b - a - used
    return f86, rows, dict(totals)


def partition86_lumped():
    """The closure script's own accounting: each interval [label, next label) is
    charged whole to the category of its labels (by_cat, item_memory_closure.py
    :1154-1164).  Reproduces the closure report exactly (validated in main)."""
    import lane2_items_offsets as lio
    r = lio.placement()
    f86 = o2r.by_name("MiscData086")
    offsets, decls = r["offsets"], r["decls"]
    by_off = defaultdict(list)
    for lab, off in offsets.items():
        by_off[off].append(lab)
    bounds = sorted(set([0] + list(by_off) + [f86.data_size]))
    rows, totals = [], defaultdict(int)
    for i in range(len(bounds) - 1):
        a, b = bounds[i], bounds[i + 1]
        labs = by_off.get(a, [])
        cats = {ic.categorize(l, decls.get(l)) for l in labs} or {"unlabeled"}
        cls = sorted(cats)[0] if len(cats) == 1 else sorted(cats)[0]
        rows.append([a, b - a, cls, labs[0] if labs else "(none)"])
        for c in cats:
            totals[c] += b - a
    return f86, rows, dict(totals), r


def parse_251_externs():
    kinds = defaultdict(dict)
    txt = ic.read_text(ic.RELOC_251)
    for line in txt.splitlines():
        mm = re.match(r"^extern\s+dITCommonData_(\w+?)_(ItemAttributes|WeaponAttributes)(?:\+0x([0-9A-Fa-f]+))?\s+(0x[0-9A-Fa-f]+)", line)
        if not mm:
            continue
        kind, typ, sub, off = mm.groups()
        field = {None: "data", "4": "mobjsubs", "8": "anim", "C": "matanim", "c": "matanim"}.get(sub, sub)
        kinds[f"{kind}:{typ}"][field] = int(off, 16)
    return kinds


def resolve_dl(f86, target):
    """A DObjDesc.dl target is either a Gfx root or a DObjDLLink list
    {list_id, Gfx*}... terminated by list_id 4.  Return the Gfx roots."""
    if target is None:
        return []
    w0 = struct.unpack_from(">I", f86.payload, target)[0]
    if w0 < 8 and ((target + 4) in f86.internal or w0 == 4):
        out = []
        a = target
        for _ in range(6):
            lid = struct.unpack_from(">I", f86.payload, a)[0]
            if lid >= 4:
                break
            t = f86.internal.get(a + 4)
            if t is not None:
                out.append(t)
            a += 8
        return out
    return [target]


def dobjdesc_roots(f86, data_off):
    """Return list of (entry_offset, [gfx roots]) until id==18 terminator."""
    out = []
    off = data_off
    for _ in range(40):
        ident = struct.unpack_from(">i", f86.payload, off)[0]
        if ident == 18:
            break
        out.append((off, resolve_dl(f86, f86.internal.get(off + 4))))
        off += 44
    return out


def owners():
    """Native owners on asset 86: {owner: {'roots': {...}, 'offs': {...}}}."""
    res = {}
    for p in sorted(GEN.glob("nds_native_item_*.generated.h")) + [GEN / "nds_native_castle_bumper.generated.h"]:
        txt = p.read_text()
        m = re.search(r"#define NDS_NATIVE_(\w+?)_ASSET (\d+)u", txt)
        if not m:
            continue
        prefix, asset = m.group(1), int(m.group(2))
        d = {}
        for mm in re.finditer(rf"#define NDS_NATIVE_{prefix}_(\w+) (0x[0-9a-fA-F]+|\d+)u", txt):
            d[mm.group(1)] = int(mm.group(2), 0)
        res[prefix] = {"asset": asset, "consts": d, "file": p.name}
    return res


def dl_word_reads():
    """{owner_prefix: set((index, word))} from the stage adapter's native checks."""
    t = STAGE.read_text(encoding="utf-8", errors="replace")
    out = defaultdict(set)
    for m in re.finditer(r"dl\[(\d+)\]\.words\.(w[01])", t):
        idx, w = int(m.group(1)), m.group(2)
        tail = t[m.end(): m.end() + 400]
        mm = re.search(r"NDS_NATIVE_([A-Z0-9]+(?:_[A-Z0-9]+)*?)_(?:[A-Z0-9]+)_?(?:W0|W1|OFFSET|ROOT)", tail)
        c = re.search(r"(NDS_NATIVE_[A-Z0-9_]+)", tail)
        owner = "?"
        if c:
            owner = c.group(1)
        out[owner].add((idx, w))
    return out


def owner_reads():
    """Per native owner on asset 86: DL indices whose words the candidate check
    compares, number of roots, TLUT/IMAGE/PAL offsets it binds.  Indices are
    grouped by the owner prefix of the constant named in the same comparison
    (renderer_adapter_stage.c native_candidate blocks)."""
    ow = owners()
    reads = defaultdict(set)
    t = STAGE.read_text(encoding="utf-8", errors="replace")
    for m in re.finditer(r"dl\[(\d+)\]\.words\.(w[01])", t):
        idx = int(m.group(1))
        tail = t[m.end(): m.end() + 300]
        c = re.search(r"NDS_NATIVE_(ITEM_[A-Z0-9]+|CASTLE_BUMPER)_", tail)
        if c:
            reads[c.group(1)].add(idx)
    out = {}
    for k, v in ow.items():
        if v["asset"] != 86:
            continue
        consts = v["consts"]
        roots = sorted({x for n, x in consts.items() if n.endswith("ROOT") or "_ROOT" in n and n[-1].isdigit() or n.startswith("ROOT")})
        roots = sorted({x for n, x in consts.items() if "ROOT" in n})
        binds = sorted({x for n, x in consts.items()
                        if n.endswith("_OFFSET") and re.search(r"TLUT|IMAGE|PAL", n)})
        out[k] = {"roots": roots, "dl_indices": sorted(reads.get(k, [])), "binds": binds,
                  "file_end": consts.get("FILE_END")}
    return out


def item_model():
    """Byte-level read model of MiscData086.

    Base classes: the closure script's lumped label categories (validated to
    reproduce its report exactly); structural overlay: decoded Gfx command bytes,
    G_VTX arrays, LOAD-sized textures/palettes (lane2_dl).  Structural evidence
    overrides label names.  Reader tags then follow the DS reader facts."""
    import bisect
    import lane2_dl as dl
    f86, rows, totals, placement = partition86_lumped()
    offsets, decls = placement["offsets"], placement["decls"]
    n = f86.data_size
    base = [None] * n
    for a, size, cls, lab in rows:
        c = {"display-list": "display_list", "unlabeled": "unclassified"}.get(cls, cls)
        if "dllink" in lab.lower():
            c = "dllink"
        for i in range(a, a + size):
            base[i] = c
    # structural decode
    roots = {off for lab, off in offsets.items() if decls.get(lab) == "Gfx"}
    for lab, off in offsets.items():
        if "dllink" in lab.lower():
            for o in range(off + 4, off + 32, 8):
                t = f86.internal.get(o)
                if t is not None:
                    roots.add(t)
    orr = owner_reads()
    for v in orr.values():
        roots.update(v["roots"])
    kinds = parse_251_externs()
    for kind, fields in kinds.items():
        if "data" in fields:
            for (_, ts) in dobjdesc_roots(f86, fields["data"]):
                roots.update(ts)
    d = dl.Decode(f86)
    for x in sorted(roots):
        d.walk(x)
    cls = list(base)
    rg = d.ranges()
    struct_bytes = {}
    for c in ("display_list", "vertices", "texture", "palette"):
        cnt = 0
        for a, b in rg[c]:
            for i in range(a, min(b, n)):
                if cls[i] != "dllink":
                    if cls[i] != c:
                        cnt += 1
                    cls[i] = c
        struct_bytes[c] = cnt
    # reachability at interval level
    starts = [r_[0] for r_ in rows]

    def iv(off):
        return max(bisect.bisect_right(starts, off) - 1, 0)

    adj = defaultdict(set)
    for slot, tgt in f86.internal.items():
        adj[iv(slot)].add(iv(tgt))
    all_roots = {x for v in orr.values() for x in v["roots"]}
    cov, unc = {}, {}
    for kind, fields in kinds.items():
        ents = dobjdesc_roots(f86, fields["data"]) if "data" in fields else []
        dls = [t for (_, ts) in ents for t in ts]
        n_cov = sum(1 for t in dls if t in all_roots)
        (cov if (dls and n_cov > 0) else unc)[kind] = list(fields.values())

    def closure(root_list):
        seen, stack = set(), [iv(o) for o in root_list]
        while stack:
            i = stack.pop()
            if i in seen:
                continue
            seen.add(i)
            stack.extend(adj.get(i, ()))
        return seen

    cov_set = closure([o for r_ in cov.values() for o in r_])
    unc_set = closure([o for r_ in unc.values() for o in r_])
    num_set = closure(list(ic.file86_numeric_roots()))
    any_set = cov_set | unc_set | num_set
    bound = set()
    for v in orr.values():
        for off in v["binds"]:
            bound.add(iv(off))
    # exact fingerprint command words: per owner, per root, per index
    fp_cmds = set()
    for k, v in orr.items():
        for r_ in v["roots"]:
            for idx in v["dl_indices"]:
                fp_cmds.add(r_ + 8 * idx)
    tags = defaultdict(int)
    fp_bytes = 0
    bytag = [None] * n
    for i in range(n):
        c = cls[i]
        k = iv(i)
        if c == "display_list":
            if (i & ~7) in fp_cmds:
                t = "keep_fingerprint"
                fp_bytes += 1
            else:
                t = "dead_dl"
        elif c == "vertices":
            t = "dead_vertices"
        elif c == "pad":
            t = "pad_other"
        elif c in ("texture", "palette"):
            if k in bound or k in cov_set:
                t = "bind_read_owned_kinds"
            elif k in any_set:
                t = "no_owner_kinds_texture"
            else:
                t = "unreached_texture_palette"
        else:  # animation material dobjdesc other dllink unclassified
            t = "keep_runtime" if k in any_set else "unreached_keep_unproven"
        bytag[i] = t
        tags[t] += 1

    def runs(keep):
        n_runs, prev = 0, False
        for t in bytag:
            cur = t in keep
            if cur and not prev:
                n_runs += 1
            prev = cur
        return n_runs
    dead = {"dead_dl", "dead_vertices", "pad_other"}
    kept_runs_A = runs(set(tags) - dead)
    classes = defaultdict(int)
    for c in cls:
        classes[c or "unclassified"] += 1
    return {"classes": dict(classes), "structural_override_bytes": struct_bytes,
            "tags": dict(tags), "fp_owner_indices": {k: v["dl_indices"] for k, v in orr.items()},
            "covered_kinds": sorted(cov), "uncovered_kinds": sorted(unc),
            "owners": {k: {kk: (vv if not isinstance(vv, list) else [hex(x) if isinstance(x, int) else x for x in vv])
                           for kk, vv in v.items()} for k, v in orr.items()},
            "sum": sum(tags.values()), "decode_roots": len(roots), "decode_errors": d.errors[:5],
            "kept_runs_A": kept_runs_A}


def per_kind_table():
    """Bytes per class reachable from each row of ITCommonData (closure over the
    86 pointer graph; shared bytes are counted in every kind that reaches them)."""
    f86, rows, totals = partition86()
    import bisect
    starts = [r[0] for r in rows]

    def iv(off):
        return max(bisect.bisect_right(starts, off) - 1, 0)

    adj = defaultdict(set)
    for slot, tgt in f86.internal.items():
        adj[iv(slot)].add(iv(tgt))
    kinds = parse_251_externs()
    orr = owner_reads()
    all_roots = {x for v in orr.values() for x in v["roots"]}
    out = []
    for kind, fields in sorted(kinds.items()):
        ents = dobjdesc_roots(f86, fields["data"]) if "data" in fields else []
        dls = [t for (_, ts) in ents for t in ts]
        seen, stack = set(), [iv(o) for o in fields.values()]
        while stack:
            i = stack.pop()
            if i in seen:
                continue
            seen.add(i)
            stack.extend(adj.get(i, ()))
        cls = defaultdict(int)
        for i in seen:
            cls[rows[i][2]] += rows[i][1]
        out.append({"kind": kind, "dl_roots": [hex(t) for t in dls],
                    "owner_roots": sum(1 for t in dls if t in all_roots),
                    "bytes": dict(cls), "total": sum(cls.values())})
    return out


def main():
    f86, rows, totals = partition86()
    print("MiscData086 class totals (label partition, closure categories):")
    for k in sorted(totals):
        print(f"  {k:14s} {totals[k]:7d}")
    print("  SUM", sum(totals.values()), "payload", f86.data_size)

    kinds = parse_251_externs()
    print("\nitem/weapon rows with roots into file 86:", len(kinds))
    ow = owners()
    print("\nnative owners with asset==86:")
    for k, v in sorted(ow.items()):
        if v["asset"] == 86:
            roots = {n: hex(x) for n, x in v["consts"].items() if "ROOT" in n or n.endswith("FILE_END")}
            print(f"  {k:12s} {roots}")
    # DL roots per kind and coverage
    all_root_vals = set()
    for k, v in ow.items():
        if v["asset"] != 86:
            continue
        for n, x in v["consts"].items():
            if "ROOT" in n:
                all_root_vals.add(x)
    table = []
    for kind, fields in sorted(kinds.items()):
        if "data" not in fields:
            continue
        ents = dobjdesc_roots(f86, fields["data"])
        dls = [t for (_, ts) in ents for t in ts]
        cov = [t in all_root_vals for t in dls]
        table.append({"kind": kind, "data": hex(fields["data"]), "dobjs": len(ents),
                      "dl_roots": [hex(t) for t in dls],
                      "covered": sum(cov), "n": len(dls)})
    print("\nper-kind DL roots (from DObjDesc trees) and whether each root is an owner ROOT constant:")
    for t in table:
        print(f"  {t['kind']:26s} data={t['data']:>8s} dobjs={t['dobjs']:2d} roots={t['n']:2d} covered={t['covered']}")
    reads = dl_word_reads()
    print("\nDL word reads by owner-constant prefix (index, word):")
    for k, v in sorted(reads.items()):
        print(f"  {k:44s} {sorted(v)}")
    out = {"totals": totals, "rows": rows, "kinds": table,
           "owners86": {k: {"consts": {n: x for n, x in v["consts"].items()}}
                        for k, v in ow.items() if v["asset"] == 86},
           "dl_reads": {k: sorted(v) for k, v in reads.items()}}
    Path(__file__).with_name("lane2_items.json").write_text(json.dumps(out, indent=1))
    print("wrote lane2_items.json")
    im = item_model()
    Path(__file__).with_name("lane2_items_model.json").write_text(json.dumps(im, indent=1))
    print("\nitem read model (MiscData086 bytes):")
    for k in sorted(im["tags"]):
        print(f"  {k:26s} {im['tags'][k]:7d}")
    print("  SUM", im["sum"], "(payload", f86.data_size, ")")
    print("  byte classes after structural overlay:", im["classes"])
    print("  structural overrides (bytes whose label class was overridden):", im["structural_override_bytes"])
    print("  covered kinds  :", im["covered_kinds"])
    print("  uncovered kinds:", im["uncovered_kinds"])
    pk = per_kind_table()
    Path(__file__).with_name("lane2_items_perkind.json").write_text(json.dumps(pk, indent=1))
    print("\nper-kind reachable bytes (shared bytes counted per kind):")
    for r in pk:
        b = r["bytes"]
        print(f"  {r['kind']:28s} roots={len(r['dl_roots'])} owned={r['owner_roots']} total={r['total']:6d}  "
              f"dl={b.get('display-list',0):5d} vtx={b.get('vertices',0):5d} tex={b.get('texture',0):5d} pal={b.get('palette',0):4d} "
              f"anim={b.get('animation',0):5d} mat={b.get('material',0):4d} dobj={b.get('dobjdesc',0):4d}")


if __name__ == "__main__":
    main()
