#!/usr/bin/env python3
"""MF experiment, step 1: the motion corpus as the runtime sees it.

Yields every fighter clip as its exact BPS1 byte image, grouped by kind:

  * pack clips   -- read from `assets/animation/ftanim_stream_pack.bin` (BPS1;
                    reader `src/nds/nds_reloc_assets.c:1230-1445`), directory
                    checked (bounds, 16 B clip alignment, no overlap, totals);
  * O2R clips    -- Pikachu / Yoshi / Ness / Purin's own files are not in the
                    pack. They go through the pack producer's own normaliser
                    (`scripts/generate_battlepack_anim.py:read_clip`, imported,
                    not edited) and the same per-clip layout `emit_stream_pack`
                    writes (slot table + 4 B-aligned runs, in-clip dedup). The
                    re-emitter is PROVEN on the pack: it must reproduce every
                    packed clip byte for byte before its O2R output is trusted.

Clip -> kind comes from ID segments (generated header
`include/nds/generated/nds_fighter_production.generated.h`), and clip ->
motion membership from each kind's `dFT<Kind>MotionDescs` table
(`decomp/BattleShip-main/decomp/src/ft/ftdata.c`), mapped through the generated
ID rows (P2 kinds) and `sNdsRelocMarioBattleAnimFileIDs`
(`src/port/reloc_backend_assets.c:3951-4275`, Mario/Fox).

Usage:
  python scripts/motion/mf_corpus.py [--json OUT] [--no-verify-pack]
The parsed corpus is cached (pickle) under the system temp dir so the
compression experiments do not re-run the O2R normaliser.
"""

from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
import os
import pathlib
import pickle
import re
import struct
import sys
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[2]
PACK = ROOT / "assets" / "animation" / "ftanim_stream_pack.bin"
BANK = ROOT / "decomp" / "BattleShip-main" / "BattleShip_o2r" / "reloc_animations"
GEN_H = ROOT / "include" / "nds" / "generated" / "nds_fighter_production.generated.h"
FTDATA = ROOT / "decomp" / "BattleShip-main" / "decomp" / "src" / "ft" / "ftdata.c"
ASSETS_C = ROOT / "src" / "port" / "reloc_backend_assets.c"
CACHE = pathlib.Path(tempfile.gettempdir()) / "smash64ds_mf_corpus_v1.pickle"

BPS1_HEADER = struct.Struct("<4sIIIIIII")
BPS1_DIR = struct.Struct("<II")

KINDS = ("mario", "fox", "donkey", "samus", "luigi", "link", "yoshi",
         "captain", "kirby", "pikachu", "purin", "ness")

# Dictionary families: a kind whose motion table borrows another kind's clips
# shares that kind's clip bank (Luigi plays Mario clips, Purin plays Kirby
# clips; generated.h NDS_P2_PURIN_ANIM_SEGMENTS / ftdata.c:2699-2712).
FAMILY_OF = {"luigi": "mario", "purin": "kirby"}

# Bank segments: every clip id belongs to exactly one bank (the kind whose
# files it is). ids from generated.h *_ANIM_SEGMENTS and
# generate_battlepack_anim.py:81-89.
SEGMENTS = [
    ("mario", 0x1F3, 0x281, "FTMarioAnim", None),
    ("fox", 0x282, 0x31F, "FTFoxAnim", None),
    ("donkey", 0x320, 0x3B8, "FTDonkeyAnim", None),
    ("samus", 0x3B9, 0x44E, "FTSamusAnim", None),
    ("luigi", 0x44F, 0x45A, "FTLuigiAnim", None),
    ("link", 0x45B, 0x4EA, "FTLinkAnim", None),
    ("kirby", 0x4EB, 0x5A4, "FTKirbyAnim", None),
    ("purin", 0x5A5, 0x5DD, "FTKirbyCopyAnim", 0x5A5),
    ("kirby", 0x5DE, 0x5DF, "FTKirbyCopyAnim", 0x5A5),
    ("purin", 0x5E0, 0x5E7, "FTPurinAnim", 0x5E0),
    ("captain", 0x5E8, 0x67F, "FTCaptainAnim", None),
    ("ness", 0x680, 0x716, "FTNessAnim", 0x680),
    ("yoshi", 0x717, 0x7A4, "FTYoshiAnim", 0x717),
    ("pikachu", 0x7A5, 0x831, "FTPikachuAnim", 0x7A5),
]


def bank_of(asset_id):
    for kind, lo, hi, _prefix, _base in SEGMENTS:
        if lo <= asset_id <= hi:
            return kind
    return None


def _load_module(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


# ---------------------------------------------------------------- BPS1 pack
def load_pack(path=PACK):
    """-> (header dict, {asset_id: clip bytes}); raises on any inconsistency."""
    blob = path.read_bytes()
    (magic, version, blob_bytes, clip_count, first_id, last_id, dir_off,
     data_off) = BPS1_HEADER.unpack_from(blob, 0)
    if magic != b"BPS1" or version != 1 or blob_bytes != len(blob):
        raise SystemExit("not a BPS1 v1 pack of the right size: %r" % path)
    dense = last_id - first_id + 1
    if dir_off + dense * BPS1_DIR.size > data_off:
        raise SystemExit("directory overlaps data")
    clips, spans = {}, []
    for row in range(dense):
        off, size = BPS1_DIR.unpack_from(blob, dir_off + row * BPS1_DIR.size)
        if size == 0:
            if off != 0:
                raise SystemExit("row %d: size 0 with offset %d" % (row, off))
            continue
        if off < data_off or off + size > len(blob) or (off & 15):
            raise SystemExit("row %d: bad span (%d, %d)" % (row, off, size))
        clips[first_id + row] = blob[off:off + size]
        spans.append((off, size))
    spans.sort()
    for (a_off, a_size), (b_off, _b) in zip(spans, spans[1:]):
        if a_off + a_size > b_off:
            raise SystemExit("clip spans overlap at %d" % b_off)
    if len(clips) != clip_count:
        raise SystemExit("directory names %d clips, header says %d"
                         % (len(clips), clip_count))
    header = {"blob_bytes": blob_bytes, "clip_count": clip_count,
              "first_id": first_id, "last_id": last_id, "dir_off": dir_off,
              "data_off": data_off, "dense_rows": dense,
              "payload_bytes": sum(len(c) for c in clips.values()),
              "sha256": hashlib.sha256(blob).hexdigest()}
    return header, clips


# ------------------------------------------------ BPS1 re-emitter (O2R clips)
def bps1_clip_bytes(clip):
    """One `read_clip` result -> its BPS1 clip image.

    Transcribes the per-clip body of `emit_stream_pack`
    (generate_battlepack_anim.py:547-583): u32 slot table (byte offset from
    the clip base, 0 = NULL slot), then each distinct run once, every run
    start 4 B aligned with zero fill."""
    table_bytes = len(clip["slot_entry"]) * 4
    stream = bytearray()
    pool, run_offsets = {}, []
    for run in clip["runs"]:
        key = bytes(run)
        if key in pool:
            run_offsets.append(pool[key])
            continue
        while len(stream) & 3:
            stream.append(0)
        offset = table_bytes + len(stream)
        pool[key] = offset
        run_offsets.append(offset)
        stream += run
    table = bytearray()
    for index in clip["slot_entry"]:
        table += struct.pack("<I", 0 if index is None else run_offsets[index])
    return bytes(table + stream)


def generated_rows():
    """{symbol: (asset_id, path-or-None)} from every X(...) row of generated.h,
    plus the Mario/Fox pointer table in reloc_backend_assets.c."""
    rows = {}
    text = GEN_H.read_text()
    for m in re.finditer(r"X\((ll\w+FileID),\s*(0x[0-9a-f]+)u(?:,\s*\"([^\"]*)\")?\)",
                         text):
        sym, aid, path = m.group(1), int(m.group(2), 16), m.group(3)
        rows.setdefault(sym, (aid, path))
    text = ASSETS_C.read_text(errors="replace")
    for m in re.finditer(r"&(llFT(?:Mario|Fox)Anim\w+FileID), /\* (\d+) \*/", text):
        rows.setdefault(m.group(1), (int(m.group(2)), None))
    return rows


def aobj32_ids():
    """Every AObj32 clip id the generated manifest names, plus the pack
    producer's own list."""
    text = GEN_H.read_text()
    ids = set()
    for block in re.finditer(r"#define NDS_P2_\w+_AOBJ32_ASSET_ROWS\(X\)((?:[^\n]*\\\n)*[^\n]*)",
                             text):
        for m in re.finditer(r"X\(ll\w+FileID,\s*(0x[0-9a-f]+)u\)", block.group(1)):
            ids.add(int(m.group(1), 16))
    return ids


def motion_tables(rows):
    """{kind: [(symbol, asset_id or None), ...]} in table order."""
    text = FTDATA.read_text(errors="replace")
    out = {}
    for kind in KINDS:
        name = kind.capitalize()
        m = re.search(r"FTMotionDesc dFT%sMotionDescs\[\] =\s*\{(.*?)\n\};" % name,
                      text, re.S)
        if not m:
            raise SystemExit("no dFT%sMotionDescs in ftdata.c" % name)
        entries = []
        for line in m.group(1).splitlines():
            s = re.search(r"\{\s*&(ll\w+FileID)", line)
            if s:
                sym = s.group(1)
                entries.append((sym, rows.get(sym, (None, None))[0]))
            elif re.search(r"\{\s*(NULL|0)\s*,", line):
                entries.append((None, None))
        out[kind] = entries
    return out


def build_corpus(verify_pack=True, log=print):
    gba = _load_module("generate_battlepack_anim",
                       ROOT / "scripts" / "generate_battlepack_anim.py")
    probe = _load_module("ftanim_reloc_probe",
                         ROOT / "scripts" / "ftanim_reloc_probe.py")
    # read_clip consults the module-global AOBJ32_IDS; extend it IN MEMORY with
    # the manifest's AObj32 rows for the kinds the pack never covered.
    a32 = aobj32_ids() | set(gba.AOBJ32_IDS)
    gba.AOBJ32_IDS = a32

    header, pack = load_pack()
    log("pack: %d clips, payload %d B, blob %d B" %
        (header["clip_count"], header["payload_bytes"], header["blob_bytes"]))

    clips = {}          # asset_id -> dict(kind, name, bytes, source)
    skipped = []
    for aid, data in pack.items():
        clips[aid] = {"id": aid, "bank": bank_of(aid), "bytes": data,
                      "source": "pack", "name": None}

    reemit_ok = reemit_bad = 0
    prefixes = tuple(sorted({s[3] for s in SEGMENTS}))
    pack_prefixes = ("FTMarioAnim", "FTFoxAnim", "FTDonkeyAnim", "FTSamusAnim",
                     "FTLuigiAnim", "FTLinkAnim", "FTKirbyAnim", "FTCaptainAnim")
    for path in sorted(BANK.iterdir()):
        if not path.name.startswith(prefixes):
            continue
        if path.name.startswith(pack_prefixes) and not verify_pack:
            continue
        clip = gba.read_clip(probe, path)
        if clip is None:
            skipped.append((path.name, "unreadable"))
            continue
        aid = clip["asset_id"]
        if clip["aobj32"]:
            skipped.append((path.name, "AObj32 0x%x" % aid))
            continue
        if clip.get("spline"):
            skipped.append((path.name, "spline TraI 0x%x" % aid))
            continue
        img = bps1_clip_bytes(clip)
        if aid in pack:
            if img == pack[aid]:
                reemit_ok += 1
            else:
                reemit_bad += 1
                log("RE-EMIT MISMATCH %s 0x%x" % (path.name, aid))
            clips[aid]["name"] = path.name
            continue
        if bank_of(aid) is None:
            skipped.append((path.name, "no bank 0x%x" % aid))
            continue
        clips[aid] = {"id": aid, "bank": bank_of(aid), "bytes": img,
                      "source": "o2r", "name": path.name,
                      "o2r_file_bytes": path.stat().st_size,
                      "o2r_payload_bytes": clip["payload_bytes"]}
    log("re-emitter vs pack: %d identical, %d mismatched" % (reemit_ok, reemit_bad))
    if reemit_bad:
        raise SystemExit("re-emitter is not the pack's layout; O2R clips untrusted")

    rows = generated_rows()
    tables = motion_tables(rows)
    names = {}
    for sym, (aid, _p) in rows.items():
        m = re.match(r"llFT([A-Z][a-z]+)Anim(\w+)FileID", sym)
        if m and aid is not None:
            names.setdefault(aid, m.group(2))
    return {"header": header, "clips": clips, "skipped": skipped,
            "reemit_ok": reemit_ok, "reemit_bad": reemit_bad,
            "tables": tables, "names": names, "aobj32": sorted(a32)}


def load_corpus(rebuild=False, verify_pack=True, log=print):
    if CACHE.exists() and not rebuild:
        with CACHE.open("rb") as fh:
            c = pickle.load(fh)
        if c.get("pack_sha256") == hashlib.sha256(PACK.read_bytes()).hexdigest():
            return c
    c = build_corpus(verify_pack=verify_pack, log=log)
    c["pack_sha256"] = c["header"]["sha256"]
    with CACHE.open("wb") as fh:
        pickle.dump(c, fh)
    return c


# ------------------------------------------------------------ motion classes
# Name classes, used only to price per-match subsets (cross-checked against
# artifacts/performance/2026-09-22_p2-2p8-architecture-baseline/
# INVESTIGATION_RESIDENCY.md Q2).
RE_ITEM = re.compile(r"Item|StarRod|Hammer|FireFlower|Harisen|Bat[A-Z]|BatSwing|"
                     r"LGun|RayGun|Sword(?!Spin)|Swing|Heavy|Tomato|Carry|Lift")
RE_TAUNT = re.compile(r"^(Taunt|Appeal)")
RE_PIPE = re.compile(r"(Enter|Exit)Pipe")
VICTIM = {  # clip-name prefix -> opponent kind whose presence requires it
    "ThrownDK": "donkey", "ThrownDKPulled": "donkey",
    "FalconDivePulled": "captain", "EggLayPulled": "yoshi",
    "ThrownMarioBros": ("mario", "luigi"), "ThrownFox": "fox",
    "ThrownFoxB": "fox", "ThrownFoxFStart": "fox",
}
KIRBY_COPY = {  # Kirby copy-ability clip name -> victim kind(s)
    "LuigiFireballGround": ("mario", "luigi"), "LuigiFireballAir": ("mario", "luigi"),
    "MarioFireballGround": ("mario", "luigi"), "MarioFireballAir": ("mario", "luigi"),
    "ChargePunchStartGround": "donkey", "ChargePunchGround": "donkey",
    "ChargePunchGroundFull": "donkey", "ChargeStartAir": "donkey",
    "ChargePunchAir": "donkey", "ChargePunchAirFull": "donkey",
    "ChargeShotStart": "samus", "Charging": "samus", "ShootingChargeShot": "samus",
    "ChargeShotAir": "samus", "ShootingChargeShotAir": "samus",
    "LaserGround": "fox", "LaserAir": "fox",
    "DKStaringGround": "yoshi", "DKStaringAir": "yoshi",
    "ThunderJoltGround": "pikachu", "ThunderJoltAir": "pikachu",
    "PKFireGround": "ness", "PKFireAir": "ness",
    "BoomerangMiss": "link", "BoomerangCatch": "link",
    "BoomerangAirMiss": "link", "BoomerangAirCatch": "link",
    "FalconPunchGround": "captain", "FalconPunchAir": "captain",
    "EggLayGround": "yoshi", "EggThrowGround": "yoshi", "EggThrowAir": "yoshi",
    "EggThrowEndAir": "yoshi",
    "PoundGround": "purin", "PoundAir": "purin",
}


def _victim_of(name):
    return VICTIM.get(name)


def classify(kind, name):
    """-> (class, required_opponents or None)."""
    if name is None:
        return ("core", None)
    if kind == "kirby" and name in KIRBY_COPY:
        return ("copy", KIRBY_COPY[name])
    v = _victim_of(name)
    if v is not None:
        return ("victim", v)
    if RE_PIPE.search(name):
        return ("pipe", None)
    if RE_TAUNT.search(name):
        return ("taunt", None)
    if RE_ITEM.search(name):
        return ("item", None)
    return ("core", None)


def kind_main_ids(corpus, kind):
    """Distinct clip ids the kind's main motion table names that are real
    AObj16 clips in the corpus (AObj32 entry clips and unmapped symbols are
    reported by the caller)."""
    ids, seen = [], set()
    for sym, aid in corpus["tables"][kind]:
        if aid is None or aid in seen:
            continue
        seen.add(aid)
        if aid in corpus["clips"]:
            ids.append(aid)
    return ids


def unreferenced_ids(corpus, kind):
    """Own-bank clips the main table does not name (one per kind at most,
    e.g. Captain FalconDiveEnd2). Kept in every resident set: conservative,
    and it is what the Q2 investigation's per-kind totals already counted."""
    main = set(kind_main_ids(corpus, kind))
    return sorted(a for a, x in corpus["clips"].items()
                  if x["bank"] == kind and a not in main)


def match_set(corpus, kind, opponents, stage_pipes=True, items=True,
              taunt=True, unreferenced=True):
    """Clip ids one fighter of `kind` needs resident for a match against
    `opponents` (kinds). victim/copy clips only for present opponents."""
    present = set(opponents)
    out = []
    candidates = list(kind_main_ids(corpus, kind))
    if unreferenced:
        candidates += unreferenced_ids(corpus, kind)
    for aid in candidates:
        name = corpus["names"].get(aid)
        cls, who = classify(kind, name)
        if cls == "pipe" and not stage_pipes:
            continue
        if cls == "item" and not items:
            continue
        if cls == "taunt" and not taunt:
            continue
        if cls in ("victim", "copy"):
            whos = who if isinstance(who, tuple) else (who,)
            if not (present & set(whos)):
                continue
        out.append(aid)
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--json", type=pathlib.Path)
    ap.add_argument("--rebuild", action="store_true")
    args = ap.parse_args()
    c = load_corpus(rebuild=args.rebuild)
    clips = c["clips"]
    print("pack header:", {k: v for k, v in c["header"].items() if k != "sha256"})
    print("re-emitter: %d identical / %d mismatched" % (c["reemit_ok"], c["reemit_bad"]))
    print("skipped (not AObj16-packable):")
    for n, why in c["skipped"]:
        print("   ", n, why)
    by_bank = {}
    for clip in clips.values():
        by_bank.setdefault(clip["bank"], []).append(clip)
    summary = {"banks": {}, "kinds": {}}
    print("\n%-8s %6s %10s %8s %6s" % ("bank", "clips", "bytes", "max", "src"))
    for bank in KINDS:
        cl = by_bank.get(bank, [])
        tot = sum(len(x["bytes"]) for x in cl)
        mx = max((len(x["bytes"]) for x in cl), default=0)
        src = ",".join(sorted({x["source"] for x in cl}))
        o2r = sum(x.get("o2r_file_bytes", 0) for x in cl)
        print("%-8s %6d %10d %8d %6s %s" % (bank, len(cl), tot, mx, src,
                                          ("o2r file bytes %d" % o2r) if o2r else ""))
        summary["banks"][bank] = {"clips": len(cl), "bytes": tot, "max": mx,
                                  "source": src, "o2r_file_bytes": o2r}
    print("\nmain-table membership per kind (distinct AObj16 clip ids):")
    print("%-8s %6s %9s %8s %7s %6s %6s %7s %7s  unmapped/aobj32" %
          ("kind", "clips", "all", "core", "items", "taunt", "pipes", "victim", "copy"))
    for kind in KINDS:
        ids = kind_main_ids(c, kind)
        cls_bytes = {}
        for aid in ids:
            cls, _ = classify(kind, c["names"].get(aid))
            cls_bytes[cls] = cls_bytes.get(cls, 0) + len(clips[aid]["bytes"])
        unmapped = [s for s, a in c["tables"][kind] if s and a is None]
        notclip = sorted({a for s, a in c["tables"][kind]
                          if a is not None and a not in clips})
        allb = sum(len(clips[a]["bytes"]) for a in ids)
        print("%-8s %6d %9d %8d %7d %6d %6d %7d %7d  %d/%s" % (
            kind, len(ids), allb, cls_bytes.get("core", 0),
            cls_bytes.get("item", 0), cls_bytes.get("taunt", 0),
            cls_bytes.get("pipe", 0), cls_bytes.get("victim", 0),
            cls_bytes.get("copy", 0), len(set(unmapped)),
            ",".join("0x%x" % a for a in notclip)))
        summary["kinds"][kind] = {"clips": len(ids), "all_main_bytes": allb,
                                  "class_bytes": cls_bytes,
                                  "unmapped_symbols": sorted(set(unmapped)),
                                  "table_ids_not_aobj16_clips": notclip}
    # Cross-check against INVESTIGATION_RESIDENCY.md Q2: stress roster, Dream
    # Land (no pipes), items on, taunt kept, victim/copy only when present.
    roster = ("donkey", "samus", "link", "kirby")
    q2 = {"donkey": 316688, "samus": 283748, "link": 300008, "kirby": 320468}
    print("\nstress roster tight set (Dream Land) vs Q2:")
    tot = 0
    summary["stress_tight"] = {}
    for kind in roster:
        opp = [k for k in roster if k != kind]
        ids = match_set(c, kind, opp, stage_pipes=False)
        b = sum(len(clips[a]["bytes"]) for a in ids)
        tot += b
        summary["stress_tight"][kind] = {"clips": len(ids), "bytes": b}
        print("  %-7s %4d clips %8d B   (Q2 %d, diff %+d)" %
              (kind, len(ids), b, q2[kind], b - q2[kind]))
    print("  total %d B (Q2 1,220,912, diff %+d)" % (tot, tot - 1220912))
    if args.json:
        args.json.write_text(json.dumps(summary, indent=1, sort_keys=True))
    return 0


if __name__ == "__main__":
    sys.exit(main())
