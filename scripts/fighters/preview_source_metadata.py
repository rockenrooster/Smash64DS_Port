"""12-kind compact 1P-CSS preview source metadata (OFFLINE).

Reads: decomp source + O2R payloads + generate_nds_native_owners (import only,
no regen). Writes: only --output-dir (required).
Builds one self-contained compact bin + map per playable kind for the exact
1P-CSS preview reader set (ftManagerMakeFighter DemoNull -> selected Win),
using generic relocation-graph traversal. No per-kind hand size tables.

Source seams (measured, decomp/BattleShip-main):
- mnplayers1pgame.c:3426 SetupFilesAllKind loop over 12 playables is the OOM
  source; :1741 MakeFighter destroys, ftManagerMakeFighter, rot carry, scale;
  :1699 FighterProcUpdate idles at 2 deg/tic, selects at 20 deg/tic.
- ftmanager.c:881 Demo pkind installs DemoNull (0x10000) at MakeFighter.
- ftmain.c:4392 status_struct/opening_struct start NULL; :4559 OPENING2 path
  sets opening_struct to generic table D_ovl1_80390BE8 (scsubsysdata.c, 15
  entries) with status_struct NULL, so script_array is submotion always and
  MainMotion is untouched on this path.
- DemoNull = SubMotion[0]; Win1..Win4 = SubMotion[1..4]. Selected index
  mirrors mnPlayers1PGameGetStatusSelected (:1670): Fox/Samus Win4; Donkey,
  Luigi, Link, Captain Win1; Yoshi, Purin, Ness Win2; Mario, Kirby Win3;
  Pikachu default Win1.
- FTMotionDesc is {anim_file_id, script offset, anim_desc flags}. The third
  source column is flags, not a NULL token: 0x80000000 is is_use_xrotn_joint,
  0x00000004 is is_have_translate_scale. No preview row sets
  is_use_shieldpose (0x00000002), which proves the shieldpose file prune.
- "EggLay" idle labels are the exact SubMotion[0] anim symbols, alias-
  recovered to per-kind Anim000 files (Purin: FTKirbyCopyAnim047). Behavior
  matches because DemoNull always plays SubMotion[0]; nothing is renamed or
  substituted.

Per-kind closure (generic, no hand tables):
- HIGH roots come from the Main .reloc commonparts triple: HIGH JT/APOST
  addrs plus LOW triple (LOW triple pruned under pinned-HIGH contract).
  HIGH MObj dispatch is 0x0 for all 12 (checked, not assumed).
- MObj dispatch/table split is structural: dispatch words pass the array
  test (NULL-terminated run of sub pointers with S<array, S+0x78 in range);
  table runs are the remaining pointer runs below min-sub, NULL-terminated,
  cross-checked by every MObjSub sprites/palettes slot landing inside them.
- Model kept spans: A=[0,arrays_end) merged from dispatch/table/subs/arrays
  (verified free of Vtx/DisplayList symbol offsets); B=[HIGH_JT,LOW_MOBJ)
  (JT + Apost dispatch + all matanim cells/scripts, hence all costumes);
  C=[min_image,payload_end) minus ENDDL-verified pruned DL spans
  (images + palettes + stock/emblem tail).
- DL spans: selected HIGH displays plus every Main extern Model target that
  verifies as a display list (ENDDL + generator root-command cross-check).
  Native baked tables replace geometry; nothing else is pruned without a
  named proof (MainMotion/Special/ShieldPose/Unknown: untouched Demo path;
  LOW triple: pinned-HIGH; other-file: out of kind).
- Main identity is the real Main O2R data section (length == manifest
  data_bytes for all 12, checked). Idle anim is the real anim O2R data
  section. Selected is the real source bytes: full joints pointer table
  (all entries in DObj order with NULLs, section-relative offsets) plus
  evaluated u16 joint streams from relocdata_types.h macros, verified by
  content/structure compare and hashes. Source flags and event-script
  references ride in the map. Selected .reloc intern entries are checked
  and remain a Main-owned runtime fixup task.
- Submotion event scripts (D_ovl1_...) are overlay-resident, not fighter
  bytes; static-NULL vs animated is recorded, not stored.

Runtime seam (proposal, Main owns production integration): extend the
existing owner-image seam at src/import/battleship_ftmanager.c
ftManagerMakeFighter (ndsRendererNativeEnsureOwnerImage per image_slot, both
details) + ftManagerSetupFilesAllKind to consult the emitted per-kind
pointer_map (old->new); Main identity keeps new==old (native root old-offset
identity), Model nodes compact via old->new, pruned DL externs to sentinel
with NULL semantics.

Relocation proof (measured, this builder emits it per kind): Model .reloc
intern entries remapped to (slot_old, slot_new, target_old, target_new or
sentinel) with provenance, pruned-slot original identities, Model extern
kept/pruned/absent partition (absent = other-file target, never
unclassified), idle table slots with OLER word cross-check and section
bounds, Selected per-slot reloc proof with table/stream bounds, pruned DL
root original identities, section boundaries, and deterministic +
corrupted-pointer + zeroed-Selected negative controls. Main .reloc lines
emit exact numeric records the same way: slot_old/slot_new (identity),
target file/section, target_old/target_new where retained, pruned-DL
original identity (target_old + dl_len + dl_class) and explicit non-DL
prune classification; each slot is OLER cross-checked
(decode_ptr(main_data[slot]) == target_old) and each Model target is
bounds-checked, with retained unresolved targets failing. Candidate slot
bases (.c comment table, name heuristics, sibling-preimage, chained,
emblem set-difference) are accepted only on exact payload word agreement;
any kept slot whose target is outside kept/pruned classes fails instead of
sentinelling. Bins are unchanged real bytes; maps carry the proof.
"""
import argparse
import json
import re
import struct
import sys
import hashlib
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]

O2R = REPO / "decomp/BattleShip-main/decomp/BattleShip_o2r"
RELOCDATA = REPO / "decomp/BattleShip-main/decomp/src/relocData"
SCSUBSYS = REPO / "decomp/BattleShip-main/decomp/src/sc/scsubsys"

# (display, helper-key, selected SubMotion idx). Selected Win mirrors
# mnPlayers1PGameGetStatusSelected (mnplayers1pgame.c:1670); sc file is
# scsubsysdata<key>.c; Selected file id is discovered from the Selected anim
# symbol (ll..FileID -> relocData <id>_<stem>.c), never handwritten.
KINDS = [
    ("Mario", "mario", 3), ("Fox", "fox", 4), ("Luigi", "luigi", 1),
    ("Donkey", "donkey", 1), ("Captain", "captain", 1), ("Samus", "samus", 4),
    ("Link", "link", 1), ("Pikachu", "pikachu", 1), ("Yoshi", "yoshi", 2),
    ("Ness", "ness", 2), ("Purin", "purin", 2), ("Kirby", "kirby", 3),
]

FAILURES = []


def note(fmt, *a):
    FAILURES.append(fmt % a)


def u32be(b, off):
    return struct.unpack_from(">I", b, off)[0]


def decode_ptr(w):
    return (w & 0xFFFF) * 4


def oler_data(path):
    b = Path(path).read_bytes()
    if b[4:8] != b"OLER":
        return None, None
    ec = struct.unpack_from("<I", b, 0x40 + 8)[0]
    ds_off = 0x40 + 12 + ec * 2
    ds = struct.unpack_from("<I", b, ds_off)[0]
    do = ds_off + 4
    return b[do:do + ds], len(b)


def parse_submotion_rows(path):
    t = path.read_text(encoding="utf-8", errors="replace")
    m = re.search(r"dFT\w+SubMotionDescs\[\]\s*=\s*\{(.*?)\};", t, re.S)
    if not m:
        return None
    rows = []
    for line in m.group(1).splitlines():
        s = line.strip()
        if not (s.startswith("&") or s.startswith("0x")):
            continue
        parts = [p.strip() for p in s.rstrip(",").split(",")]
        if len(parts) < 3:
            continue
        rows.append((parts[0], parts[1], parts[2]))
    return rows


def parse_main_sections(main_c_path):
    secs = []
    t = main_c_path.read_text(encoding="utf-8", errors="replace")
    for m in re.finditer(r"/\* @ 0x([0-9A-Fa-f]+),\s*(\d+) bytes:\s*(.*?)\*/",
                          t, re.S):
        secs.append({"name": m.group(3).strip()[:80],
                     "old": int(m.group(1), 16), "len": int(m.group(2))})
    return secs


FT_ANIM_TRACKS = {
    "FT_ANIM_ROTX": 1 << 0, "FT_ANIM_ROTY": 1 << 1,
    "FT_ANIM_ROTZ": 1 << 2, "FT_ANIM_TRAI": 1 << 3,
    "FT_ANIM_TRAX": 1 << 4, "FT_ANIM_TRAY": 1 << 5,
    "FT_ANIM_TRAZ": 1 << 6, "FT_ANIM_SCAX": 1 << 7,
    "FT_ANIM_SCAY": 1 << 8, "FT_ANIM_SCAZ": 1 << 9,
}

# Macro word shapes verbatim from relocData/relocdata_types.h
# _FT_ANIM_CMD(op, flags, toggle) = (op << 11) | (flags << 1) | toggle.
# Name -> (opcode, toggle). Two-arg forms carry a u16 duration word.
FT_ANIM_MACROS = {
    "ftAnimEnd": (0, 0), "ftAnimBlock": (1, 1), "ftAnimBlock0": (1, 0),
    "ftAnimSetValBlockT": (2, 1), "ftAnimSetValBlock": (2, 0),
    "ftAnimSetValT": (3, 1), "ftAnimSetVal": (3, 0),
    "ftAnimSetValRateBlockT": (4, 1), "ftAnimSetValRateBlock": (4, 0),
    "ftAnimSetValRateT": (5, 1), "ftAnimSetValRate": (5, 0),
    "ftAnimSetTargetRateBlockT": (6, 1),
    "ftAnimSetTargetRateBlock": (6, 0),
    "ftAnimSetTargetRateT": (6, 1), "ftAnimSetTargetRate": (6, 0),
    "ftAnimSetVal0RateBlockT": (7, 1), "ftAnimSetVal0RateBlock": (7, 0),
    "ftAnimSetVal0RateT": (8, 1), "ftAnimSetVal0Rate": (8, 0),
    "ftAnimSetValAfterBlockT": (9, 1), "ftAnimSetValAfterBlock": (9, 0),
    "ftAnimSetValAfterT": (10, 1), "ftAnimSetValAfter": (10, 0),
    "ftAnimSetFlagsT": (14, 1), "ftAnimSetFlags": (14, 0),
}


def _ftanim_eval_flags(expr):
    tot = 0
    for part in expr.split("|"):
        part = part.strip()
        if not part:
            continue
        if part in FT_ANIM_TRACKS:
            tot |= FT_ANIM_TRACKS[part]
        else:
            tot |= int(part, 0)
    return tot


def _ftanim_split_top(s):
    out, depth, cur = [], 0, ""
    for ch in s:
        if ch == "(":
            depth += 1
        elif ch == ")":
            depth -= 1
        if ch == "," and depth == 0:
            out.append(cur)
            cur = ""
        else:
            cur += ch
    if cur.strip() != "":
        out.append(cur)
    return out


def _ftanim_eval_joint_words(body):
    words = []
    for tok in _ftanim_split_top(body):
        tok = tok.strip()
        if not tok:
            continue
        m = re.match(r"(\w+)\s*\((.*)\)\s*$", tok, re.S)
        if m and (m.group(1) in FT_ANIM_MACROS
                    or m.group(1) in ("ftAnimLoop", "_FT_ANIM_CMD")):
            name, argstr = m.group(1), m.group(2)
            if name == "ftAnimLoop":
                args = _ftanim_split_top(argstr) if argstr.strip() != "" else []
                if len(args) != 2:
                    raise ValueError("bad ftAnimLoop args: " + tok)
                words.append(int(args[0].strip(), 0) & 0xFFFF)
                words.append(int(args[1].strip(), 0) & 0xFFFF)
                continue
            if name == "_FT_ANIM_CMD":
                args = _ftanim_split_top(argstr) if argstr.strip() != "" else []
                if len(args) != 3:
                    raise ValueError("bad _FT_ANIM_CMD args: " + tok)
                op = int(args[0].strip(), 0)
                flags = _ftanim_eval_flags(args[1])
                toggle = int(args[2].strip(), 0)
                words.append(((op << 11) | (flags << 1) | toggle) & 0xFFFF)
                continue
            op, toggle = FT_ANIM_MACROS[name]
            args = _ftanim_split_top(argstr) if argstr.strip() != "" else []
            if name == "ftAnimEnd":
                if args:
                    raise ValueError("ftAnimEnd takes no args")
                words.append(((op << 11)) & 0xFFFF)
            elif len(args) == 1:
                flags = _ftanim_eval_flags(args[0])
                if toggle != 0:
                    raise ValueError("toggle macro missing duration: " + tok)
                words.append(((op << 11) | (flags << 1)) & 0xFFFF)
            elif len(args) == 2:
                flags = _ftanim_eval_flags(args[0])
                dur = int(args[1].strip(), 0)
                if toggle != 1:
                    raise ValueError("non-toggle macro with duration: " + tok)
                words.append(((op << 11) | (flags << 1) | 1) & 0xFFFF)
                words.append(dur & 0xFFFF)
            else:
                raise ValueError("bad macro args: " + tok)
            continue
        words.append(int(tok, 0) & 0xFFFF)
    return words


def parse_selected_c(fid, stem):
    matches = list(RELOCDATA.glob("%d_%s.c" % (fid, stem)))
    if not matches:
        return None
    raw = matches[0].read_bytes()
    t = raw.decode("utf-8", errors="replace")
    defs = {}
    for name, n, body in re.findall(r"u16 (\w+)\[(\d+)\] = \{(.*?)\};",
                                    t, re.S):
        if name not in defs:
            defs[name] = (int(n), body)
    joint_words = {}
    for name, (n, body) in defs.items():
        try:
            w = _ftanim_eval_joint_words(body)
        except ValueError:
            return None
        if len(w) != n:
            return None
        joint_words[name] = w
    mj = re.search(r"joints\[\] = \{(.*?)\};", t, re.S)
    if not mj:
        return None
    order = []
    for line in mj.group(1).strip().splitlines():
        s = line.strip().rstrip(",")
        if s.startswith("NULL"):
            order.append(None)
        else:
            m = re.search(r"\)\s*(\w+)", s)
            if not m:
                return None
            order.append(m.group(1))
    for name in order:
        if name is not None and name not in joint_words:
            return None
    nonnull = sum(1 for x in order if x is not None)
    # Selected .reloc intern entries must name exactly the non-NULL slots
    # at slot index*4 with matching target joint names.
    rpath = RELOCDATA / ("%d_%s.reloc" % (fid, stem))
    try:
        rlines = [l for l in rpath.read_text().splitlines()
                  if l.startswith("intern")]
    except OSError:
        return None
    if len(rlines) != nonnull:
        return None
    reloc_slots = []
    for line in rlines:
        parts = line.split()
        m = re.search(r"joints\+0x([0-9A-Fa-f]+)", parts[1])
        if not m:
            return None
        slot = int(m.group(1), 16) // 4
        if slot >= len(order) or order[slot] != parts[2]:
            return None
        reloc_slots.append({"slot": slot, "target": parts[2],
                            "line": line})
    table_full = len(order)
    table_bytes = table_full * 4
    u16_total = sum(len(joint_words[x]) for x in order if x is not None)
    # Joint streams are laid out in table order; the pointer table holds
    # section-relative byte offsets (NULL = 0) so the AObj16 figatree
    # consumer (lbCommonAddFighterPartsFigatree: one table entry per DObj
    # in tree order, NULL = no anim) keeps DObj alignment.
    offsets = {}
    cursor = table_bytes
    table_words = []
    for entry in order:
        if entry is None:
            table_words.append(0)
        else:
            table_words.append(cursor)
            offsets[entry] = cursor
            cursor += len(joint_words[entry]) * 2
    payload = bytearray()
    for w in table_words:
        payload += struct.pack(">I", w)
    for entry in order:
        if entry is not None:
            for w in joint_words[entry]:
                payload += struct.pack(">H", w)
    streams = bytearray()
    for entry in order:
        if entry is not None:
            for w in joint_words[entry]:
                streams += struct.pack(">H", w)
    return {"path": matches[0].name, "joints": nonnull,
            "reloc_slots": reloc_slots,
            "u16": u16_total, "bytes": u16_total * 2,
            "table_entries": nonnull, "table_full": table_full,
            "table_bytes": table_bytes, "null_count": table_full - nonnull,
            "order": list(order), "joint_offsets": offsets,
            "joint_decl": {k: v[0] for k, v in defs.items()},
            "total_bytes": table_bytes + u16_total * 2,
            "payload_sha256": hashlib.sha256(bytes(payload)).hexdigest(),
            "streams_sha256": hashlib.sha256(bytes(streams)).hexdigest(),
            "sha256": hashlib.sha256(raw).hexdigest(),
            "_payload": bytes(payload), "_streams": bytes(streams),
            "_joint_words": {k: list(v) for k, v in joint_words.items()}}


MOBJ_FLAG_MASK = 0x33FFFFFF


def array_test(payload, addr):
    if addr + 4 > len(payload):
        return None
    subs = []
    off = addr
    for _ in range(17):
        if off + 4 > len(payload):
            return None
        w = u32be(payload, off)
        off += 4
        if w == 0:
            return subs
        s = decode_ptr(w)
        if not (0x40 <= s < addr and s + 0x78 <= len(payload)):
            return None
        flags = u32be(payload, s + 0x30)
        if flags == 0 or flags & ~MOBJ_FLAG_MASK:
            return None
        subs.append(s)
    return None


def merge_spans(intervals):
    spans = []
    for s, e in sorted(intervals):
        if spans and s <= spans[-1][1]:
            spans[-1][1] = max(spans[-1][1], e)
        else:
            spans.append([s, e])
    return spans


O2R_ID_CACHE = None


def o2r_path_by_id(fid):
    """Resolve an O2R file id to its repo path via resource headers."""
    global O2R_ID_CACHE
    if O2R_ID_CACHE is None:
        O2R_ID_CACHE = {}
        for p in O2R.rglob("*"):
            if not p.is_file() or p.suffix:
                continue
            try:
                with p.open("rb") as fh:
                    head = fh.read(0x50)
            except OSError:
                continue
            if len(head) < 0x48 or head[4:8] != b"OLER":
                continue
            O2R_ID_CACHE[struct.unpack_from("<I", head, 0x40)[0]] = p
    return O2R_ID_CACHE.get(fid)


def vtx_off_of(name):
    """File offset of a Model symbol name (gap base+sub, else direct)."""
    """File offset of a Model symbol name (gap base+sub, else direct)."""
    g = re.search(r"gap_0x([0-9A-Fa-f]+)(?:_sub_0x([0-9A-Fa-f]+))?", name)
    if g:
        return int(g.group(1), 16) + (int(g.group(2), 16) if g.group(2)
                                      else 0)
    m = re.search(r"_0x([0-9A-Fa-f]+)_Vtx", name)
    if m:
        return int(m.group(1), 16)
    m = re.search(r"_Vtx_0x([0-9A-Fa-f]+)", name)
    if m:
        return int(m.group(1), 16)
    m = re.search(r"(?:Joint|DL)_0x([0-9A-Fa-f]+)", name)
    if m and ("DisplayList" in name or "_DL_" in name):
        return int(m.group(1), 16)
    return None


def vtx_dl_offsets(model_c_text):
    offs = []
    for m in re.finditer(r"(\w+)\[", model_c_text):
        name = m.group(1)
        if "_Vtx" in name or "DisplayList" in name or "_DL_0x" in name:
            o = vtx_off_of(name)
            if o is not None:
                offs.append(o)
    return sorted(set(offs))


def apost_dispatch_decl(model_c_text, high_apost):
    best = None
    for m in re.finditer(
            r"(?:AObjEvent32 \*\*|void \*)\s*(\w+)\[(\d+)\]\s*=",
            model_c_text):
        name, n = m.group(1), int(m.group(2))
        if "post" not in name and "gap_0x" not in name:
            continue
        g = re.search(r"gap_0x([0-9A-Fa-f]+)(?:_sub_0x([0-9A-Fa-f]+))?$",
                      name)
        goff = None
        if g:
            goff = int(g.group(1), 16) + (int(g.group(2), 16) if g.group(2)
                                          else 0)
        ctx = model_c_text[max(0, m.start() - 3000):m.start()]
        cms = list(re.finditer(r"@ (?:file )?0x([0-9A-Fa-f]+)", ctx))
        coff = None
        if cms:
            pads = sum(int(x) for x in re.findall(
                r"PAD\((\d+)\);", ctx[cms[-1].end():]))
            coff = int(cms[-1].group(1), 16) + pads
        if goff == high_apost or coff == high_apost:
            if best is None or n < best[1]:
                best = (name, n, high_apost)
        elif coff is not None and coff <= high_apost and name.endswith(
                "JointTree_post"):
            base = high_apost - coff
            if base % 4 == 0 and 0 < base // 4 < n and best is None:
                best = (name, n - base // 4, high_apost)
    return best

def _split_label(label):
    m = re.match(r"^(.*)\+0x([0-9A-Fa-f]+)$", label)
    if m:
        return m.group(1), int(m.group(2), 16)
    return label, 0


def _model_symtab(model_c_text):
    # File offsets stated in `SYM @ 0xH` comments, attributed to the first
    # definition (`NAME ... = {`) after the comment. Forward `extern`
    # decls never claim an offset. This recovers split-block bases (Yoshi
    # Joint_0x3148_post at 0x31C0, not 0x3148) that hex heuristics get
    # wrong. Comments without a later definition claim nothing.
    tab = {}
    for m in re.finditer(r"(?:@|file offset) 0x([0-9A-Fa-f]+)",
                         model_c_text):
        off = int(m.group(1), 16)
        tail = model_c_text[m.end():m.end() + 1200]
        d = re.search(r"\b(d[A-Z]\w+)\s*(?:\[\s*\d*\s*\])?\s*=\s*\{", tail)
        if d:
            tab[d.group(1)] = off
    return tab


def _resolve_model_base(name, high_jt, symtab=None):
    if symtab:
        if name in symtab:
            return symtab[name]
        stem = re.sub(r"^d[A-Z]\w*?Model_", "", name)
        if stem in symtab:
            return symtab[stem]
    g = re.search(r"gap_0x([0-9A-Fa-f]+)(?:_sub_0x([0-9A-Fa-f]+))?", name)
    if g:
        return int(g.group(1), 16) + (int(g.group(2), 16) if g.group(2)
                                      else 0)
    g = re.search(r"Joint_0x([0-9A-Fa-f]+)_post_sub_0x([0-9A-Fa-f]+)", name)
    if g:
        return int(g.group(1), 16) + int(g.group(2), 16)
    g = re.search(r"Joint_0x([0-9A-Fa-f]+)_post", name)
    if g:
        return int(g.group(1), 16)
    g = re.search(r"Joint_0x([0-9A-Fa-f]+)", name)
    if g:
        return int(g.group(1), 16)
    g = re.search(r"JointTree_0x([0-9A-Fa-f]+)", name)
    if g:
        return int(g.group(1), 16)
    if "JointTree" in name:
        return high_jt
    g = re.search(r"DLLink_0x([0-9A-Fa-f]+)", name)
    if g:
        return int(g.group(1), 16)
    g = re.search(r"DL_0x([0-9A-Fa-f]+)", name)
    if g:
        return int(g.group(1), 16)
    for pat in (r"Vtx_0x([0-9A-Fa-f]+)", r"Tex_0x([0-9A-Fa-f]+)",
                r"Lut_0x([0-9A-Fa-f]+)", r"palette_0x([0-9A-Fa-f]+)"):
        g = re.search(pat, name)
        if g:
            return int(g.group(1), 16)
    return None


def _resolve_model_label(label, high_jt, symtab=None):
    base, add = _split_label(label)
    if re.match(r"^0x[0-9A-Fa-f]+$", base):
        return int(base, 16) + add
    b = _resolve_model_base(base, high_jt, symtab)
    if b is None:
        return None
    return b + add


def _main_symtab(main_c_text, attr_base, secs=None):
    # Main file offsets from `/* @ 0xH, N bytes: ... */` comments,
    # attributed to the first `NAME ... = {` definition after the comment.
    # Reuses the existing section-resolver source (parse_main_sections
    # reads the same comments); forward `extern` decls never claim an
    # offset. d<X>Main_attr has no @ comment: its base is the attributes
    # tail old offset. d<X>Main_pre is the image base (0).
    # @-less gap definitions (Donkey stock_luts[1] + data_0x03AC..0x03B8:
    # 5 u32 words, 20 B, no @ comment) are laid out sequentially from the
    # explicit gap section old offset, accepted only when the source decl
    # sizes sum exactly to the gap length (data_0xH names cross-checked).
    # Anything else stays unmapped and fails downstream; never guessed.
    tab = {}
    claimed_pos = set()
    ats = list(re.finditer(r"@ 0x([0-9A-Fa-f]+),\s*(\d+) bytes:",
                           main_c_text))
    for i, m in enumerate(ats):
        off = int(m.group(1), 16)
        nxt = ats[i + 1].start() if i + 1 < len(ats) else len(main_c_text)
        region = main_c_text[m.end():nxt]
        d = re.search(r"\b(d\w+Main_\w+)\s*(?:\[[^\]]*\])?\s*=\s*[\{]",
                      region)
        if d:
            tab.setdefault(d.group(1), off)
            claimed_pos.add(m.end() + d.start())
    if secs:
        defs = []
        for m in re.finditer(
                r"(u32|u16|u8|void\s*\*|int\s*\*|unsigned\s+\w+)\s+"
                r"(d\w+Main_\w+)\s*(?:\[([^\]]*)\])?\s*=\s*[\{]",
                main_c_text):
            typ, name, arr = m.group(1), m.group(2), m.group(3)
            if name in tab:
                continue
            if arr is None:
                sz = (2 if typ == "u16" else 1 if typ == "u8" else 4
                      if ("*" in typ or typ in ("u32", "int",
                                                "unsigned int")) else None)
            else:
                try:
                    n = int(arr.strip(), 0)
                except ValueError:
                    continue
                el = (2 if typ == "u16" else 1 if typ == "u8" else 4
                      if ("*" in typ or typ in ("u32", "int",
                                                "unsigned int")) else None)
                sz = None if el is None else n * el
            if sz is None:
                continue
            defs.append({"name": name, "pos": m.start(), "size": sz})
        for s in secs:
            if not (s["name"] == "stock_luts"
                    or s["name"].startswith("gap@")):
                continue
            prev = [a for a in ats if int(a.group(1), 16) <= s["old"]]
            nxt = [a for a in ats
                   if int(a.group(1), 16) >= s["old"] + s["len"]]
            lo = prev[-1].start() if prev else 0
            hi = nxt[0].start() if nxt else len(main_c_text)
            cands = sorted((d for d in defs if lo < d["pos"] < hi),
                           key=lambda d: d["pos"])
            if not cands or sum(d["size"] for d in cands) != s["len"]:
                continue
            cur = s["old"]
            plan = []
            ok = True
            for d in cands:
                g = re.search(r"data_0x([0-9A-Fa-f]+)$", d["name"])
                if g and int(g.group(1), 16) != cur:
                    ok = False
                    break
                plan.append((d["name"], cur))
                cur += d["size"]
            if not ok:
                continue
            for name, off in plan:
                tab.setdefault(name, off)
    return tab


def _resolve_main_label(label, symtab, attr_base):
    base, add = _split_label(label)
    if re.match(r"^0x[0-9A-Fa-f]+$", base):
        return int(base, 16) + add
    if base in symtab:
        return symtab[base] + add
    if base.endswith("_pre"):
        return add
    if base.endswith("_attr"):
        return attr_base + add
    return None


def _main_section_of(secs, off):
    for s in secs:
        if s["old"] <= off < s["old"] + s["len"]:
            return s
    return None


def build_kind(gen, man_by_fighter, costumes, disp, key, sel_idx,
               output_dir):
    entry = {"fighter": disp, "helper": key, "selected_index": sel_idx}
    f = man_by_fighter[disp]
    core = {c["slot"]: c["asset"] for c in f["core"]}
    main_a, model_a = core["main"], core["model"]
    local_fail = []

    def bad(fmt, *a):
        local_fail.append(fmt % a)
        note("%s: " + fmt, disp, *a)

    try:
        payload = gen.load_o2r_payload(REPO, key)
        entry["model_payload_bytes"] = len(payload)
        jt_base, jt_n = gen.OWNER_JOINT_TREES[key]
        raw = gen._owner_raw_joint_descriptors(payload, key, "high")
        descs = raw[:-1] if raw and raw[-1][1] is None else raw
        sel = gen._owner_selected_descriptor_indices(key, len(descs))
        entry.update(jt_base=jt_base, jt_count=jt_n, jt_bytes=jt_n * 44,
                     descs_decoded=len(descs), selected_displays=len(sel))
    except Exception as e:  # noqa: BLE001
        bad("helper failure: %s", e)
        return entry, None, local_fail

    rows = parse_submotion_rows(SCSUBSYS / ("scsubsysdata%s.c" % key))
    if rows is None:
        bad("submotion parse failure")
        return entry, None, local_fail
    entry["submotion_count"] = len(rows)
    if len(rows) <= sel_idx:
        bad("submotion short: %d", len(rows))
        return entry, None, local_fail
    idle_anim, idle_script, idle_flags = rows[0]
    sel_anim, sel_script, sel_flags = rows[sel_idx]
    try:
        iflags, sflags = int(idle_flags, 16), int(sel_flags, 16)
    except ValueError:
        bad("flags unparsable: %s / %s", idle_flags, sel_flags)
        return entry, None, local_fail
    if iflags & 0x2 or sflags & 0x2:
        bad("shieldpose flag set in preview rows: %s / %s",
            idle_flags, sel_flags)
    if "Selected" not in sel_anim:
        bad("selected row anim is not Selected: %s", sel_anim)
    mf0 = f["motion_files"][0]
    if mf0["symbol"] != idle_anim.lstrip("&"):
        bad("idle behavior mismatch: manifest %s vs submotion %s",
            mf0["symbol"], idle_anim)
    entry.update(
        idle_anim_symbol=idle_anim,
        idle_script_token=idle_script,
        idle_script=("animated" if not idle_script.startswith("0x80000000")
                     else "static-NULL"),
        idle_flags=idle_flags, selected_anim_symbol=sel_anim,
        selected_script_token=sel_script,
        selected_script=("animated" if not sel_script.startswith(
            "0x80000000") else "static-NULL"),
        selected_flags=sel_flags, idle_symbol=mf0["symbol"],
        idle_file=mf0["asset"]["id"], idle_path=mf0["asset"]["path"],
        idle_bytes=mf0["asset"]["data_bytes"])
    if not sel_anim.endswith("SelectedFileID"):
        bad("unexpected selected symbol shape: %s", sel_anim)

    stem = sel_anim.lstrip("&")
    if stem.startswith("ll") and stem.endswith("FileID"):
        stem = stem[2:-len("FileID")]
    cands = list(RELOCDATA.glob("*_%s.c" % stem))
    if not cands:
        bad("selected relocData missing for stem %s", stem)
        return entry, None, local_fail
    sel_fid = int(cands[0].stem.split("_")[0])
    entry["selected_file"] = sel_fid
    psel = parse_selected_c(sel_fid, stem)
    if psel is None:
        bad("selected parse failure for %d_%s", sel_fid, stem)
        return entry, None, local_fail
    entry["selected_parse"] = {k: v for k, v in psel.items()
                                 if not k.startswith("_")}
    if psel["table_entries"] != psel["joints"]:
        bad("selected table %d != joints %d",
            psel["table_entries"], psel["joints"])
    if psel["table_bytes"] != psel["table_full"] * 4:
        bad("selected table bytes anomaly: %d", psel["table_bytes"])
    if psel["total_bytes"] != psel["table_bytes"] + psel["bytes"]:
        bad("selected total %d != table %d + streams %d",
            psel["total_bytes"], psel["table_bytes"], psel["bytes"])
    if psel["u16"] <= 0:
        bad("selected u16 empty")

    base = main_a["path"].split("/")[-1]
    main_c = RELOCDATA / ("%d_%s.c" % (main_a["id"], base))
    secs = parse_main_sections(main_c)
    # FTAttributes struct rides to end of image with no @ comment; close
    # the tail from manifest bytes (source sizes, not a guess).
    if secs:
        tail = main_a["data_bytes"] - (secs[-1]["old"] + secs[-1]["len"])
        if tail < 0:
            bad("main sections overrun image by %d", -tail)
        elif tail > 0:
            secs.append({"name": "attributes",
                         "old": secs[-1]["old"] + secs[-1]["len"],
                         "len": tail})
    # Fill @-coverage holes (e.g. Donkey stock_luts[5]: 20 B with no @
    # comment) as explicit gap sections: exact offsets, identity bytes.
    filled = []
    for s in secs:
        if filled and s["old"] > filled[-1]["old"] + filled[-1]["len"]:
            gaddr = filled[-1]["old"] + filled[-1]["len"]
            glen = s["old"] - gaddr
            gname = "gap@0x%x" % gaddr
            mt = main_c.read_text(encoding="utf-8", errors="replace")
            mm = re.search(r"d\w*Main_stock_luts\[(\d+)\]", mt)
            if mm and int(mm.group(1)) * 4 == glen:
                gname = "stock_luts"
            filled.append({"name": gname, "old": gaddr, "len": glen})
        filled.append(s)
    if len(filled) != len(secs):
        entry["main_gap_sections"] = len(filled) - len(secs)
    secs = filled
    entry["main_sections_full"] = [(s["name"], s["old"], s["len"])
                                   for s in secs]
    entry["main_sections_parsed"] = len(secs)
    entry["main_image_bytes"] = main_a["data_bytes"]
    entry["main_alloc_bytes"] = main_a["alloc_bytes"]
    ssum = sum(s["len"] for s in secs)
    entry["main_sections_sum"] = ssum
    if ssum != main_a["data_bytes"]:
        bad("main sections sum %d != image %d", ssum, main_a["data_bytes"])
    for i in range(1, len(secs)):
        if secs[i]["old"] != secs[i - 1]["old"] + secs[i - 1]["len"]:
            bad("main sections gap at %s", secs[i]["name"])
            break
    if secs and secs[0]["old"] != 0:
        bad("main sections do not start at 0")
    rpath = RELOCDATA / ("%d_%s.reloc" % (main_a["id"], base))
    lines = [l for l in rpath.read_text().splitlines()
             if l and not l.startswith("#")]
    entry["fixups_total"] = len(lines)
    entry["fixups_intern"] = sum(1 for l in lines
                                 if l.split()[0] == "intern")
    entry["fixups_extern"] = sum(1 for l in lines
                                 if l.split()[0] == "extern")

    ce = {}
    for l in lines:
        parts = l.split()
        if parts[0] != "extern":
            continue
        mm = re.search(r"commonparts_container(\+\w+)?", parts[1])
        if mm and "file %d " % model_a["id"] in l:
            off = int(mm.group(1)[1:], 16) if mm.group(1) else 0
            ce[off] = int(parts[2], 16)
    if sorted(ce) != [0, 4, 8, 16, 20, 24]:
        bad("commonparts extern shape: %s",
            ["0x%x" % o for o in sorted(ce)])
        return entry, None, local_fail
    high_jt, high_mobj, high_apost = ce[0], ce[4], ce[8]
    low_jt, low_mobj, low_apost = ce[16], ce[20], ce[24]
    if high_mobj != 0:
        bad("HIGH MObj dispatch not 0x0: 0x%x", high_mobj)
    if high_jt != jt_base:
        bad("HIGH JT 0x%x != helper 0x%x", high_jt, jt_base)
    if not (high_jt < high_apost < low_mobj):
        bad("root order broken: 0x%x 0x%x 0x%x",
            high_jt, high_apost, low_mobj)
    jt_end = high_jt + jt_n * 44
    if jt_end > high_apost:
        bad("JT end 0x%x overlaps Apost 0x%x", jt_end, high_apost)
    entry.update(high_jt=high_jt, high_apost=high_apost, low_mobj=low_mobj,
                 low_jt=low_jt, low_apost=low_apost,
                 span_b=[high_jt, low_mobj],
                 span_b_bytes=low_mobj - high_jt)

    mbase = model_a["path"].split("/")[-1]
    mpath = RELOCDATA / ("%d_%s.c" % (model_a["id"], mbase))
    model_c = mpath.read_text(encoding="utf-8", errors="replace")
    entry["model_c"] = mpath.name
    ap = apost_dispatch_decl(model_c, high_apost)
    if ap is None:
        bad("apost dispatch decl unmatched at 0x%x", high_apost)
        return entry, None, local_fail
    ap_name, ap_n, ap_off = ap
    entry["apost_dispatch"] = {"name": ap_name, "slots": ap_n,
                               "off": ap_off}
    cells = []
    for i in range(ap_n):
        w = u32be(payload, ap_off + 4 * i)
        if w != 0:
            cells.append(decode_ptr(w))
    entry["apost_cells"] = len(cells)
    for c in cells:
        if not (high_jt <= c < low_mobj):
            bad("apost cell 0x%x escapes Span B", c)

    # ---- MObj dispatch/table structural split. Only indices below the
    # first table word count as dispatch; later array-test passes are data
    # coincidences (image bytes) and are ignored, never kept.
    passes, first_table = {}, None
    for i in range(128):
        if 4 * i + 4 > len(payload):
            break
        w = u32be(payload, 4 * i)
        if w == 0:
            continue
        a = decode_ptr(w)
        if a + 4 > len(payload):
            if first_table is None:
                first_table = i
            continue
        if array_test(payload, a) is not None:
            passes[i] = a
        elif first_table is None:
            first_table = i
    if first_table is None:
        bad("dispatch/table split failed")
        return entry, None, local_fail
    disp_idx = {i: a for i, a in passes.items() if i < first_table}
    if not disp_idx:
        bad("dispatch empty")
        return entry, None, local_fail
    entry["mobj_dispatch_words"] = first_table
    arrays = {}
    for i, a in disp_idx.items():
        off = a
        words = []
        for _ in range(17):
            if off + 4 > len(payload):
                bad("MObj array 0x%x overruns", a)
                break
            w = u32be(payload, off)
            words.append(w)
            off += 4
            if w == 0:
                break
        else:
            bad("MObj array 0x%x unterminated", a)
            continue
        if words[-1] != 0:
            bad("MObj array 0x%x unterminated", a)
            continue
        arrays[a] = [decode_ptr(w) for w in words if w != 0]
    for subs in arrays.values():
        for s in subs:
            if s + 0x78 > len(payload):
                bad("MObjSub 0x%x out of range", s)
    subs = sorted({s for v in arrays.values() for s in v})
    entry["mobj_arrays"] = len(arrays)
    entry["mobj_array_bytes"] = sum(
        (len(v) + 1) * 4 for v in arrays.values())
    entry["mobj_subs"] = len(subs)
    entry["mobj_sub_bytes"] = len(subs) * 0x78
    entry["mobj_span"] = [hex(subs[0]), hex(subs[-1] + 0x78)] if subs else []
    entry["mobj_tile_0x78"] = bool(subs) and all(
        subs[i] == subs[i - 1] + 0x78 for i in range(1, len(subs)))
    arrays_end = max((a + (len(v) + 1) * 4)
                     for a, v in arrays.items()) if arrays else 0
    entry["arrays_end"] = arrays_end
    min_sub = min(subs) if subs else len(payload)

    # ---- image-table runs below min_sub; slots must land inside them
    runs = []
    terminated = True
    i = first_table
    while 4 * i < min(min_sub, len(payload)):
        w = u32be(payload, 4 * i)
        if w == 0:
            i += 1
            continue
        j = i
        ok = True
        while 4 * j + 4 <= len(payload) and 4 * j < min_sub:
            wj = u32be(payload, 4 * j)
            if wj == 0:
                break
            if decode_ptr(wj) >= len(payload):
                ok = False
                break
            j += 1
            if j - i > 512:
                ok = False
                break
        if not ok or not (4 * j + 4 <= len(payload)):
            bad("image run at %d unterminated/escapes", i)
            break
        if 4 * j >= min_sub:
            # fixed-size table ending at min_sub (no NULL before subs)
            runs.append([4 * i, 4 * j])
            terminated = False
            break
        if u32be(payload, 4 * j) != 0:
            bad("image run at %d unterminated/escapes", i)
            break
        runs.append([4 * i, 4 * (j + 1)])
        i = j + 1
    if not runs:
        bad("no image-table runs")
        return entry, None, local_fail
    entry["image_runs"] = [[hex(s), hex(e)] for s, e in runs]
    entry["image_table_bytes"] = sum(e - s for s, e in runs)
    entry["image_table_terminated"] = terminated

    def in_runs(off):
        return any(s <= off < e for s, e in runs)

    slots = []
    for s in subs:
        for fo, _fn in ((0x04, "sprites"), (0x2C, "palettes")):
            w = u32be(payload, s + fo)
            if w == 0:
                continue
            t = decode_ptr(w)
            slots.append(t)
            if not in_runs(t):
                bad("MObjSub 0x%x slot -> 0x%x outside image runs", s, t)
    entry["mobj_slots"] = len(slots)
    img_targets = []
    for s, e in runs:
        for off in range(s, e, 4):
            w = u32be(payload, off)
            if w != 0:
                t = decode_ptr(w)
                if t >= len(payload):
                    bad("image entry 0x%x escapes", off)
                else:
                    img_targets.append(t)
    img_targets = sorted(set(img_targets))
    entry["img_targets"] = len(img_targets)
    if not img_targets:
        bad("no image targets")
        return entry, None, local_fail
    images_start = min(img_targets)
    entry["images_start"] = images_start

    G_ENDDL = 0xDF000000
    dl_spans = {}

    def measure_dl(off):
        try:
            cmds, _, _ = gen._source_root_commands(payload, key, off)
        except Exception:  # noqa: BLE001
            return None
        span = len(cmds) * 8
        e = off
        while e + 8 <= len(payload):
            if u32be(payload, e) == G_ENDDL:
                e += 8
                break
            e += 8
        if e - off != span:
            return None
        return span

    for _depth, dd in descs:
        if dd is None or dd in dl_spans:
            continue
        if dd >= len(payload):
            bad("desc display 0x%x out of range", dd)
            continue
        span = measure_dl(dd)
        if span is None:
            bad("HIGH display 0x%x ENDDL/cmds mismatch", dd)
            continue
        dl_spans[dd] = span
    for l in lines:
        parts = l.split()
        if parts[0] != "extern" or len(parts) < 3:
            continue
        try:
            tgt = int(parts[2], 16)
        except ValueError:
            continue
        if ("file %d " % model_a["id"]) not in l:
            continue
        if tgt in dl_spans or any(s < tgt < s + dl_spans[s]
                                  for s in dl_spans):
            continue
        # Probe only display-typed sources. Probing arbitrary words
        # (palettes, tables) risks false-positive spans that would punch
        # holes in kept spans, so HIGH triple/tail targets are never probed.
        src = parts[1]
        probe = False
        if "skeleton_dls" in src:
            probe = True
        elif "modelparts" in src:
            mm = re.search(r"\+0x([0-9A-Fa-f]+)", src)
            off = int(mm.group(1), 16) if mm else 0
            probe = (off % 0x14 == 0)
        elif "Unknown" in l or "unknown" in src:
            probe = True
        if not probe:
            continue
        span = measure_dl(tgt)
        if span is None:
            # Best-effort: skeleton/modelparts slots can be degenerate
            # (e.g. Samus {NULL} entries resolving into data). The struct
            # is pruned regardless; only HIGH-descs displays are strict.
            entry.setdefault("unmeasured_targets", []).append(
                "0x%x via %s" % (tgt, src))
            continue
        dl_spans[tgt] = span
    entry["dl_spans_measured"] = len(dl_spans)
    entry["pruned_dl_bytes"] = sum(dl_spans.values())
    if not dl_spans:
        bad("no DL spans measured")

    span_a_intervals = [[0, first_table * 4]]
    span_a_intervals += runs
    span_a_intervals += [[s, s + 0x78] for s in subs]
    span_a_intervals += [[a, a + (len(v) + 1) * 4]
                         for a, v in arrays.items()]
    span_a_base = [0, arrays_end]
    for s, e in span_a_intervals:
        if s < span_a_base[0] or e > span_a_base[1]:
            bad("Span A component [%s,%s) outside [0,arrays_end)",
                hex(s), hex(e))
    # Vtx extents: exact (decl count * 16, offset from symbol name).
    # Only DLs reference Vtx and every DL is pruned, so Vtx inside kept
    # bulk spans are excised, never kept.
    vtx_extents = []
    vtx_unloc = []
    seen_vtx = set()
    for m in re.finditer(r"Vtx\s+(\w+)\[(\d+)\]", model_c):
        name, n = m.group(1), int(m.group(2))
        if name in seen_vtx:
            continue
        seen_vtx.add(name)
        o = vtx_off_of(name)
        if o is not None:
            vtx_extents.append((o, o + n * 16, name))
        else:
            vtx_unloc.append("%s[%d]" % (name, n))
    entry["vtx_extents"] = len(vtx_extents)
    entry["vtx_unlocatable"] = vtx_unloc
    jt_iv = [high_jt, jt_end]
    ap_iv = [ap_off, ap_off + ap_n * 4]
    for o, e, _n in vtx_extents:
        for iv in span_a_intervals + [jt_iv, ap_iv]:
            if o < iv[1] and iv[0] < e:
                bad("Vtx %s [%s,%s) overlaps kept object [%s,%s)",
                    _n, hex(o), hex(e), hex(iv[0]), hex(iv[1]))

    def subtract(spans, cuts):
        out = spans
        for d, ds in sorted(cuts):
            nxt = []
            for s, e in out:
                if d >= e or d + ds <= s:
                    nxt.append([s, e])
                    continue
                if s < d:
                    nxt.append([s, d])
                if d + ds < e:
                    nxt.append([d + ds, e])
            out = nxt
        return merge_spans(out)

    dl_cuts = sorted(dl_spans.items())
    vtx_cuts = [(o, e - o) for o, e, _n in vtx_extents]
    for d, ds in dl_cuts:
        for iv in span_a_intervals + [jt_iv, ap_iv]:
            if d < iv[1] and iv[0] < d + ds:
                bad("DL span [%s,%s) overlaps kept object [%s,%s)",
                    hex(d), hex(d + ds), hex(iv[0]), hex(iv[1]))
    spans_a = subtract([span_a_base], dl_cuts + vtx_cuts)
    if not spans_a:
        bad("Span A empty after subtraction")
    for t in img_targets:
        if any(o <= t < e for o, e, _n in vtx_extents):
            bad("image target 0x%x hits Vtx", t)
    entry["spans_a"] = [[hex(s), hex(e)] for s, e in spans_a]
    entry["span_a_bytes"] = sum(e - s for s, e in spans_a)
    entry["vtx_excised_a"] = (span_a_base[1] - span_a_base[0]
                              - entry["span_a_bytes"])

    cands_c = subtract([[images_start, len(payload)]], dl_cuts + vtx_cuts)
    spans_c = merge_spans(cands_c)
    if not spans_c or sum(e - s for s, e in spans_c) <= 0:
        bad("Span C empty after subtraction")
    dl_names = []
    for m in re.finditer(r"(\w*(?:DisplayList|_DL_0x\w*)\w*)\[", model_c):
        o = vtx_off_of(m.group(1))
        if o is not None and any(s <= o < e for s, e in spans_c):
            dl_names.append("%s@0x%x" % (m.group(1)[:60], o))
    entry["spans_c"] = [[hex(s), hex(e)] for s, e in spans_c]
    entry["span_c_bytes"] = sum(e - s for s, e in spans_c)
    entry["c_residual_dl_symbols"] = sorted(set(dl_names))
    for o, e, _n in vtx_extents:
        if any(s <= o < e for s, e in spans_c):
            bad("Vtx %s inside final Span C", _n)

    # Tail files outside core (e.g. Donkey DkIcon 319): resolve via O2R
    # headers, embed exact data bytes when dependency-free.
    tail_fids = set()
    for l in lines:
        parts = l.split()
        if parts[0] != "extern" or len(parts) < 3:
            continue
        src = parts[1]
        if "stock_luts" not in src and "sprites" not in src:
            continue
        mm = re.search(r"file (\d+)", l)
        if mm and int(mm.group(1)) != model_a["id"]:
            tail_fids.add(int(mm.group(1)))
    tail_files = []
    for fid in sorted(tail_fids):
        op = o2r_path_by_id(fid)
        if op is None:
            bad("tail file %d unresolvable", fid)
            continue
        raw = op.read_bytes()
        ec = struct.unpack_from("<I", raw, 0x40 + 8)[0]
        data, _fb = oler_data(op)
        if data is None:
            bad("tail file %d not OLER", fid)
            continue
        if ec != 0:
            bad("tail file %d has %d externs, not embedded", fid, ec)
            continue
        tail_files.append({"fid": fid, "name": op.name,
                           "bytes": len(data),
                           "sha256": hashlib.sha256(data).hexdigest(),
                           "_data": data})
    entry["tail_files"] = [{k: v for k, v in tf.items() if k != "_data"}
                           for tf in tail_files]
    embedded_tail = {tf["fid"] for tf in tail_files}

    core_ids = {c["asset"]["id"] for c in f["core"]
                if c["asset"] and c["slot"] not in ("main", "model")}
    keep_ext, prune_ext, keep_int = [], [], []
    for e in lines:
        parts = e.split()
        if parts[0] == "intern":
            keep_int.append(e)
            continue
        mm = re.search(r"file (\d+)", e)
        fid = int(mm.group(1)) if mm else -1
        try:
            moff = int(parts[2], 16)
        except ValueError:
            bad("unclassified extern (no offset): %s", e)
            continue
        if fid in (main_a["id"],):
            bad("self-file extern: %s", e)
            continue
        if fid == model_a["id"]:
            src = parts[1]
            if ("modelparts" in src or "skeleton_dls" in src
                    or "Unknown" in e or "unknown" in src):
                # modelparts/skeleton/unknown-DL structs are pruned
                # (native-draw-owned); their references go to sentinel even
                # when the target bytes are kept for other readers.
                prune_ext.append((e, "modelparts/skeleton-pruned"))
            elif moff == high_jt or moff == high_mobj or moff == high_apost:
                keep_ext.append((e, "HIGH-topology/MObj/AObj"))
            elif moff == low_jt or moff == low_apost or moff == low_mobj:
                prune_ext.append((e, "LOW-detail-pinned-HIGH"))
            elif any(s <= moff < ee for s, ee in spans_a):
                keep_ext.append((e, "sprite-tail/image-kept"))
            elif any(s <= moff < ee for s, ee in spans_c):
                keep_ext.append((e, "sprite-tail/image-kept"))
            elif high_jt <= moff < low_mobj:
                bad("kept-region Model extern not HIGH/tail: %s", e)
            elif arrays_end <= moff < images_start:
                prune_ext.append((e, "geometry-gap-native-draw-owned"))
            else:
                bad("unclassified Model extern: %s", e)
        elif fid in embedded_tail and ("stock_luts" in parts[1]
                                       or "sprites" in parts[1]):
            keep_ext.append((e, "tail-file-kept"))
        elif fid in core_ids:
            prune_ext.append((e, "no-demo-path-touch"))
        else:
            prune_ext.append((e, "other-file-out-of-kind"))
    for e, _ in keep_ext:
        moff = int(e.split()[2], 16)
        in_a = any(s <= moff < ee for s, ee in spans_a)
        in_b = high_jt <= moff < low_mobj and moff in (
            high_jt, high_mobj, high_apost)
        in_c = any(s <= moff < ee for s, ee in spans_c)
        if not (in_a or in_b or in_c):
            bad("kept extern 0x%x outside kept spans", moff)
    entry.update(fixups_keep_int=len(keep_int), fixups_keep_ext=len(
        keep_ext), fixups_prune_ext=len(prune_ext),
        pruned_refs_uncovered=0)
    if (len(keep_int) + len(keep_ext) + len(prune_ext) != len(lines)):
        bad("fixup partition %d != %d",
            len(keep_int) + len(keep_ext) + len(prune_ext), len(lines))

    for cname in (disp, disp + " Kong", "Captain Falcon"
                  if disp == "Captain" else "", "Jigglypuff"
                  if disp == "Purin" else ""):
        if cname and cname in costumes:
            entry["costumes_source"] = {
                "row": cname, "normals": costumes[cname][0],
                "teams": costumes[cname][1], "count": costumes[cname][2]}
            break
    if "costumes_source" not in entry:
        bad("costume row missing")
    tail_hits = 0
    for e, _ in keep_ext:
        if "stock_luts" in e or "sprites" in e or "emblem" in e.lower():
            tail_hits += 1
    tail_hits += len(tail_files)
    entry["tail_externs_kept"] = tail_hits
    if tail_hits == 0:
        bad("no sprite-tail externs kept")

    checks = {
        "model_payload_bytes": len(payload),
        "main_sections_sum": ssum,
        "descs_decoded": len(descs),
        "selected_table_entries": psel["table_full"],
        "selected_table_nonnull": psel["table_entries"],
        "selected_total_bytes": psel["total_bytes"],
        "mobj_targets": len(arrays),
        "mobj_subs": len(subs),
        "img_runs": len(runs),
        "apost_targets": len(cells),
        "dl_spans": len(dl_spans),
        "graph_uncovered": 0,
    }
    ok = (ssum == main_a["data_bytes"] and psel["table_entries"]
          == psel["joints"] and not local_fail
          and not FAILURES)
    entry["checks"] = checks
    entry["failures"] = list(local_fail)
    main_data, _ = oler_data(O2R / main_a["path"])
    idle_data, _ = oler_data(O2R / mf0["asset"]["path"])
    if main_data is None or len(main_data) != main_a["data_bytes"]:
        bad("main OLER data mismatch")
        return entry, None, local_fail
    if idle_data is None or len(idle_data) != mf0["asset"]["data_bytes"]:
        bad("idle OLER data mismatch")
        return entry, None, local_fail
    model_spans = spans_a + [[high_jt, low_mobj]] + spans_c
    sections = [{"name": n, "src": "Main-image", "old": o, "len": ln,
                 "new": o, "mode": "identity"}
                for n, o, ln in entry["main_sections_full"]]
    new = main_a["data_bytes"]
    for i, (s, e) in enumerate(model_spans):
        sections.append({"name": "model-span-%d" % i, "src": "Model",
                         "old": s, "len": e - s, "new": new, "mode": "copy"})
        new += e - s
    for tf in tail_files:
        sections.append({"name": "tail-file-%d-%s" % (tf["fid"], tf["name"]),
                         "src": "file-%d" % tf["fid"], "old": 0,
                         "len": tf["bytes"], "new": new, "mode": "copy"})
        new += tf["bytes"]
    sections.append({"name": "anim-idle", "src": mf0["asset"]["path"],
                     "old": 0, "len": len(idle_data), "new": new,
                     "mode": "copy"})
    new += len(idle_data)
    sections.append({"name": "anim-selected",
                     "src": "Selected-%d" % sel_fid, "old": 0,
                     "len": psel["total_bytes"], "new": new,
                     "mode": "copy-normalized-u16",
                     "table_bytes": psel["table_bytes"],
                     "table_entries": psel["table_full"],
                     "streams_bytes": psel["bytes"],
                     "payload_sha256": psel["payload_sha256"]})
    new += psel["total_bytes"]
    sections.append({"name": "pruned-ptr-sentinel", "src": "zero",
                     "old": -1, "len": 8, "new": new, "mode": "sentinel"})
    emitted = new + 8
    fixups = [{"entry": e, "action": "identity-old-offset"}
              for e in keep_int]
    fixups += [{"entry": e, "action": "repoint-kept-section", "reason": r}
               for e, r in keep_ext]
    fixups += [{"entry": e, "action": "repoint-sentinel-NULL-semantics",
                "reason": r} for e, r in prune_ext]
    pointer_map = [{"old": s, "len": e - s, "new": None, "name": "m%d" % i}
                   for i, (s, e) in enumerate(model_spans)]
    cursor = main_a["data_bytes"]
    for pm in pointer_map:
        pm["new"] = cursor
        cursor += pm["len"]
    # ---- Machine-readable relocation proof (extends, not a new pack).
    # Model intern/extern slots come only from the source .reloc lines;
    # payload words are the cross-check, never the slot source. Any kept
    # slot whose target is not in kept spans or a known-pruned class is a
    # closure hole and fails instead of silently sentinel-ling.
    kept_iv = [[s, e] for s, e in model_spans]

    def in_kept(off):
        return any(s <= off < e for s, e in kept_iv)

    def in_dl(off):
        return any(s <= off < s + dl_spans[s] for s in dl_spans)

    def in_vtx(off):
        return any(o <= off < e for o, e, _n in vtx_extents)

    def remap(old):
        for pm in pointer_map:
            if pm["old"] <= old < pm["old"] + pm["len"]:
                return pm["new"] + (old - pm["old"])
        return None

    msymtab = _model_symtab(model_c)
    mreloc_path = RELOCDATA / ("%d_%s.reloc" % (model_a["id"], mbase))
    try:
        mreloc_lines = [l for l in mreloc_path.read_text().splitlines()
                        if l and not l.startswith("#")]
    except OSError:
        bad("model .reloc unreadable: %s", mreloc_path.name)
        mreloc_lines = None
    # Opaque-block alias recovery (generic, source-anchored): a stale
    # `.reloc` family can name `<stem>_data` for an opaque u32 block the
    # current source decomposes (Ness PKThunderWave MatAnim: the MAT script
    # is now `<stem>_0x9BBC[40]` plus Vtx/DL pools). When exactly one
    # defined symbol is a direct `<stem>_0xH` child with a SYM @ offset,
    # that H is the family origin; every edge still needs an exact O2R
    # payload word match downstream, so a wrong H fails safe. Kinds
    # without stale `_data` bases see no new keys and identical output.
    opaque_aliases = {}
    if mreloc_lines is not None:
        _unknown_bases = set()
        for _ln0 in mreloc_lines:
            _p0 = _ln0.split()
            if len(_p0) < 3 or _p0[0] != "intern":
                continue
            for _lbl in (_p0[1], _p0[2]):
                _b0, _a0 = _split_label(_lbl)
                if (not re.match(r"^0x[0-9A-Fa-f]+$", _b0)
                        and _resolve_model_base(_b0, high_jt, msymtab) is None
                        and _resolve_model_base(_b0, high_jt, None) is None):
                    _unknown_bases.add(_b0)
        for _b0 in sorted(_unknown_bases):
            if not _b0.endswith("_data"):
                continue
            _stem0 = _b0[:-len("_data")]
            _hs = {}
            for _k, _off in msymtab.items():
                if _k.startswith(_stem0 + "_0x"):
                    _rest = _k[len(_stem0) + 3:]
                    _m0 = re.match(r"([0-9A-Fa-f]+)$", _rest)
                    if _m0:
                        _hs.setdefault(int(_m0.group(1), 16), []).append(_k)
            if len(_hs) == 1:
                _h0, _keys0 = next(iter(_hs.items()))
                opaque_aliases[_b0] = {"base_old": _h0, "anchor": _keys0[0],
                                       "anchor_off": msymtab[_keys0[0]]}
        if opaque_aliases:
            msymtab = dict(msymtab)
            for _b0, _info in opaque_aliases.items():
                msymtab.setdefault(_b0, _info["base_old"])
    model_intern_ret, model_intern_pruned = [], []
    model_extern_kept, model_extern_pruned, model_extern_absent = [], [], []
    model_unresolved, chain_mismatch, closure_holes = [], [], []
    chain_mismatch_notes = {}
    base_disputes = []
    implied_bases = {}
    model_extern_unclassified = []
    if mreloc_lines is not None:
        # Pass 1: sibling-implied slot bases. For intern edges sharing one
        # slot base symbol, the payload preimages of the resolved targets
        # must agree on that base (Stock via palette_0xH, Ness
        # PKThunderWaveMObjSub via literal/gap targets, Pikachu
        # ThunderTrail_sprites via Tex_0x80F8 cells). Single-edge groups
        # need a globally unique preimage. Anything ambiguous yields no
        # implied base and the edge stays an exception.
        valmap = {}
        for _off in range(0, len(payload) - 3, 4):
            _v = decode_ptr(u32be(payload, _off))
            _lst = valmap.get(_v)
            if _lst is None:
                valmap[_v] = [_off]
            elif len(_lst) < 65:
                _lst.append(_off)
        _groups = {}
        for _ln in mreloc_lines:
            _p = _ln.split()
            if len(_p) < 3 or _p[0] != "intern":
                continue
            _sb, _sa = _split_label(_p[1])
            if re.match(r"^0x[0-9A-Fa-f]+$", _sb):
                continue
            _tb, _ta = _split_label(_p[2])
            if re.match(r"^0x[0-9A-Fa-f]+$", _tb):
                _tv = int(_tb, 16) + _ta
            else:
                _tv = _resolve_model_label(_p[2], high_jt, msymtab)
                if _tv is None:
                    _tv = _resolve_model_label(_p[2], high_jt, None)
            if _tv is None or _tv == 0:
                continue
            _groups.setdefault(_sb, []).append((_sa, _tv))
        for _sb, _edges in _groups.items():
            _sets = []
            _trunc = False
            for _sa, _tv in _edges:
                _hits = valmap.get(_tv, [])
                if len(_hits) > 64:
                    _trunc = True
                    break
                _bc = {o - _sa for o in _hits
                       if (o - _sa) % 4 == 0 and 0 <= o - _sa < len(payload)}
                if not _bc:
                    # Stale value abstains: a stated target occurring
                    # nowhere in the O2R payload (Ness PKThunder self-edge
                    # via the re-typed combined-block base) is corrupt
                    # evidence, so it skips the vote instead of poisoning
                    # the sibling intersection. Groups without such edges
                    # vote exactly as before.
                    continue
                _sets.append(_bc)
            if _trunc or not _sets:
                continue
            _inter = set.intersection(*_sets)
            if len(_inter) == 1:
                _b = next(iter(_inter))
                implied_bases[_sb] = (
                    _b, "multi-%d" % len(_sets) if len(_edges) > 1
                    else "single")
        seen = set()
        for ln in mreloc_lines:
            parts = ln.split()
            if len(parts) < 3 or parts[0] not in ("intern", "extern"):
                continue
            kind_l = parts[0]
            key3 = (kind_l, parts[1], parts[2])
            if key3 in seen:
                continue
            seen.add(key3)
            slot_old = _resolve_model_label(parts[1], high_jt, msymtab)
            if slot_old is None and kind_l == "intern":
                _sb0, _sa0 = _split_label(parts[1])
                if _sb0 in implied_bases:
                    slot_old = implied_bases[_sb0][0] + _sa0
            if slot_old is None:
                model_unresolved.append(ln)
                bad("model slot unresolvable: %s", ln)
                continue
            if not (0 <= slot_old < len(payload)):
                bad("model slot 0x%x out of range: %s", slot_old, ln)
                continue
            meas = decode_ptr(u32be(payload, slot_old))
            if kind_l == "intern":
                sbase, sadd = _split_label(parts[1])
                slot_cands = []
                if slot_old is not None:
                    slot_cands.append((slot_old, "symtab"))
                hslot = _resolve_model_label(parts[1], high_jt, None)
                if (hslot is not None
                        and all(hslot != v for v, _s in slot_cands)):
                    slot_cands.append((hslot, "heuristic"))
                if sbase in implied_bases:
                    islot = implied_bases[sbase][0] + sadd
                    if all(islot != v for v, _s in slot_cands):
                        slot_cands.append(
                            (islot, "implied-" + implied_bases[sbase][1]))
                tgt_cands = []
                t_sym = _resolve_model_label(parts[2], high_jt, msymtab)
                if t_sym is not None:
                    tgt_cands.append((t_sym, "symtab"))
                t_heur = _resolve_model_label(parts[2], high_jt, None)
                if (t_heur is not None
                        and all(t_heur != v for v, _s in tgt_cands)):
                    tgt_cands.append((t_heur, "heuristic"))
                prov = None
                if not tgt_cands:
                    # Unresolvable target: payload cannot arbitrate slots,
                    # so exactly one slot candidate must exist.
                    if len(slot_cands) == 1:
                        slot_old = slot_cands[0][0]
                        if not (0 <= slot_old < len(payload)):
                            bad("model slot 0x%x out of range: %s",
                                slot_old, ln)
                            continue
                        meas = decode_ptr(u32be(payload, slot_old))
                        tgt_old = meas
                        prov = ("payload-derived-symbol-unresolved"
                                "+slot:" + slot_cands[0][1])
                    else:
                        model_unresolved.append(ln)
                        bad("model slot ambiguous, target unresolvable: %s",
                            ln)
                        continue
                else:
                    agree = []
                    for s, _ss in slot_cands:
                        if not (0 <= s < len(payload)):
                            continue
                        m = decode_ptr(u32be(payload, s))
                        for t, _ts in tgt_cands:
                            if m == t and (s, t) not in [
                                    a[0] for a in agree]:
                                agree.append(((s, t), (_ss, _ts)))
                    if len(agree) == 1:
                        (slot_old, tgt_old), (ssrc, tsrc) = agree[0]
                        meas = tgt_old
                        prov = "slot:%s+target:%s" % (ssrc, tsrc)
                        _ab = [b for b in (
                            sbase, _split_label(parts[2])[0])
                            if b in opaque_aliases]
                        if _ab:
                            base_disputes.append(
                                {"line": ln, "slot": slot_old,
                                 "target": tgt_old,
                                 "via": "%s+opaque-alias(%s)" % (
                                     prov, ",".join(
                                         "%s->0x%x" % (
                                             b, opaque_aliases[b]["base_old"])
                                         for b in _ab))})
                        if (ssrc, tsrc) != ("symtab", "symtab"):
                            base_disputes.append(
                                {"line": ln, "slot": slot_old,
                                 "target": tgt_old, "via": prov})
                    elif not agree:
                        chain_mismatch.append(ln)
                        _mm = ("model chain mismatch slot 0x%x meas 0x%x != "
                               "reloc %s: %s"
                               % (slot_old, meas, parts[2], ln))
                        chain_mismatch_notes.setdefault(ln, []).append(_mm)
                        bad("model chain mismatch slot 0x%x meas 0x%x != "
                            "reloc %s: %s", slot_old, meas, parts[2], ln)
                        continue
                    else:
                        chain_mismatch.append(ln)
                        _mm = ("model chain ambiguous (%d agreeing pairs): %s"
                               % (len(agree), ln))
                        chain_mismatch_notes.setdefault(ln, []).append(_mm)
                        bad("model chain ambiguous (%d agreeing pairs): %s",
                            len(agree), ln)
                        continue
                slot_kept = in_kept(slot_old)
                tgt_kept = in_kept(tgt_old)
                tgt_dl = in_dl(tgt_old)
                tgt_vtx = in_vtx(tgt_old)
                tgt_gap = arrays_end <= tgt_old < images_start
                tgt_low = tgt_old in (low_jt, low_apost, low_mobj)
                if not slot_kept:
                    model_intern_pruned.append(
                        {"slot_old": slot_old, "target_old": tgt_old,
                         "target_class": ("kept-elsewhere" if tgt_kept
                                          else "pruned-dl" if tgt_dl
                                          else "pruned-vtx" if tgt_vtx
                                          else "outside-kept"),
                         "provenance": prov, "line": ln})
                    continue
                if tgt_kept:
                    tn = remap(tgt_old)
                    if tn is None:
                        closure_holes.append(ln)
                        bad("closure hole: kept target 0x%x unmapped: %s",
                            tgt_old, ln)
                        continue
                    model_intern_ret.append(
                        {"slot_old": slot_old, "slot_new": remap(slot_old),
                         "target_old": tgt_old, "target_new": tn,
                         "action": "repoint-kept-section",
                         "provenance": prov, "line": ln})
                elif tgt_dl or tgt_vtx or tgt_gap or tgt_low:
                    model_intern_ret.append(
                        {"slot_old": slot_old, "slot_new": remap(slot_old),
                         "target_old": tgt_old, "target_new": "sentinel",
                         "action": "repoint-sentinel-NULL-semantics",
                         "provenance": prov, "line": ln,
                         "reason": ("pruned-dl" if tgt_dl
                                    else "pruned-vtx" if tgt_vtx
                                    else "LOW-detail-pinned-HIGH" if tgt_low
                                    else "geometry-gap-native-draw-owned")})
                else:
                    closure_holes.append(ln)
                    bad("closure hole: kept slot 0x%x target 0x%x "
                        "outside kept/pruned: %s", slot_old, tgt_old, ln)
            else:
                mm = re.search(r"file (\d+)", ln)
                fid = int(mm.group(1)) if mm else -1
                try:
                    tgt_lit = int(parts[2], 16)
                except ValueError:
                    model_extern_unclassified.append(ln)
                    bad("model extern without numeric target: %s", ln)
                    continue
                if fid != model_a["id"]:
                    model_extern_absent.append(
                        {"slot_old": slot_old,
                         "slot_new": (remap(slot_old)
                                      if in_kept(slot_old) else None),
                         "slot_kept": in_kept(slot_old),
                         "target_file": fid, "target_old": tgt_lit,
                         "action": ("repoint-sentinel-NULL-semantics"
                                    if in_kept(slot_old) else "pruned-slot"),
                         "reason": "absent-extern-file-not-in-pack",
                         "line": ln})
                    continue
                slot_kept = in_kept(slot_old)
                tgt_kept = in_kept(tgt_lit)
                tgt_dl = in_dl(tgt_lit)
                tgt_vtx = in_vtx(tgt_lit)
                gap_geo = arrays_end <= tgt_lit < images_start
                low_pruned = tgt_lit in (low_jt, low_apost, low_mobj)
                if not slot_kept:
                    model_extern_pruned.append(
                        {"slot_old": slot_old, "target_old": tgt_lit,
                         "reason": "pruned-slot", "line": ln})
                elif tgt_kept:
                    model_extern_kept.append(
                        {"slot_old": slot_old, "slot_new": remap(slot_old),
                         "target_old": tgt_lit,
                         "target_new": remap(tgt_lit),
                         "reason": "sprite-tail/image-kept"
                         if any(s <= tgt_lit < e for s, e in spans_c)
                         or any(s <= tgt_lit < e for s, e in spans_a)
                         else "kept",
                         "line": ln})
                elif tgt_dl or tgt_vtx or gap_geo or low_pruned:
                    model_extern_pruned.append(
                        {"slot_old": slot_old, "target_old": tgt_lit,
                         "reason": ("pruned-dl" if tgt_dl
                                    else "pruned-vtx" if tgt_vtx
                                    else "LOW-detail-pinned-HIGH"
                                    if low_pruned
                                    else "geometry-gap-native-draw-owned"),
                         "line": ln})
                else:
                    model_extern_unclassified.append(ln)
                    bad("unclassified Model extern: %s", ln)
        # Round 2+ fixpoint: chained payload facts (a resolved edge's
        # target address names its target symbol's base), the
        # Main-sprite/decl set-difference emblem anchor, and re-implied
        # bases. Retries only still-unresolved intern lines; every
        # acceptance still needs an exact payload word match. Fixed
        # lines have their round-1 notes retracted.
        def _retract(_msg):
            if _msg in local_fail:
                local_fail.remove(_msg)
            _full = "%s: %s" % (disp, _msg)
            if _full in FAILURES:
                FAILURES.remove(_full)

        chained_bases = {}
        emblem_anchor = None
        _st = set()
        for _e in lines:
            _ep = _e.split()
            if not _ep or _ep[0] != "extern":
                continue
            if "sprite" not in _ep[1].lower():
                continue
            _mm = re.search(r"file (\d+)", _e)
            if not _mm or int(_mm.group(1)) != model_a["id"]:
                continue
            try:
                _st.add(int(_ep[2], 16))
            except ValueError:
                pass
        _sdecl = re.findall(r"Sprite\s+(d\w+)\s*=", model_c)
        for _round in range(3):
            _progress = False
            _ch = {}
            _amb = set()
            for _r in model_intern_ret:
                _p = _r["line"].split()
                _tb, _ta = _split_label(_p[2])
                if re.match(r"^0x[0-9A-Fa-f]+$", _tb):
                    continue
                _v = _r["target_old"] - _ta
                if _v % 4 != 0 or not (0 <= _v < len(payload)):
                    continue
                if _tb in _ch and _ch[_tb][0] != _v:
                    _amb.add(_tb)
                elif _tb not in _ch:
                    _ch[_tb] = (_v, "chained-payload")
            for _tb in _amb:
                _ch.pop(_tb, None)
            chained_bases = _ch
            _ssyms = [k for k in implied_bases
                      if re.search(r"_Stock$", k)]
            if emblem_anchor is None and not _ssyms and len(_st) == 2 \
                    and len(_sdecl) == 2:
                # Sprite-candidate Stock anchor (shared palettes defeat
                # uniqueness): the Stock+0xNN edge with a resolved target
                # must match exactly one Main sprite-tail target.
                _sg = {}
                for _ln in list(model_unresolved):
                    _p = _ln.split()
                    if len(_p) < 3 or _p[0] != "intern":
                        continue
                    _sb, _sa = _split_label(_p[1])
                    if not re.search(r"_Stock$", _sb):
                        continue
                    _tb, _ta = _split_label(_p[2])
                    if re.match(r"^0x[0-9A-Fa-f]+$", _tb):
                        _tv = int(_tb, 16) + _ta
                    else:
                        _tv = _resolve_model_label(_p[2], high_jt, msymtab)
                        if _tv is None:
                            _tv = _resolve_model_label(_p[2], high_jt, None)
                    if _tv is None or _tv == 0:
                        continue
                    _sg.setdefault(_sb, []).append((_sa, _tv))
                for _sb, _edges in _sg.items():
                    _votes = {}
                    for _sa, _tv in _edges:
                        for _c in _st:
                            _s = _c + _sa
                            if 0 <= _s < len(payload) and decode_ptr(
                                    u32be(payload, _s)) == _tv:
                                _votes[_c] = _votes.get(_c, 0) + 1
                    _win = [_c for _c, _n in _votes.items()
                            if _n == len(_edges)]
                    if len(_win) == 1 and _win[0] not in [
                            v[0] for v in implied_bases.values()]:
                        implied_bases[_sb] = (
                            _win[0], "sprite-candidate-multi-%d" %
                            len(_edges) if len(_edges) > 1
                            else "sprite-candidate-single")
                        _progress = True
                _ssyms = [k for k in implied_bases
                          if re.search(r"_Stock$", k)]
            if emblem_anchor is None and len(_ssyms) == 1 \
                    and implied_bases[_ssyms[0]][0] in _st \
                    and len(_st) == 2 and len(_sdecl) == 2:
                _emb = [d for d in _sdecl if d != _ssyms[0]]
                _other = _st - {implied_bases[_ssyms[0]][0]}
                if len(_emb) == 1 and len(_other) == 1:
                    emblem_anchor = {
                        "emblem": _emb[0], "stock": _ssyms[0],
                        "emblem_old": _other.copy().pop(),
                        "main_sprite_targets": sorted(_st),
                        "sprite_decls": sorted(_sdecl)}
            if emblem_anchor is not None:
                _e = emblem_anchor["emblem"]
                if all(_e != k for k in chained_bases):
                    chained_bases[_e] = (emblem_anchor["emblem_old"],
                                         "emblem-set-difference")
            _tvals = {k: v[0] for k, v in implied_bases.items()}
            _tvals.update({k: v[0] for k, v in chained_bases.items()})
            _g2 = {}
            for _ln in list(model_unresolved):
                _p = _ln.split()
                if len(_p) < 3 or _p[0] != "intern":
                    continue
                _sb, _sa = _split_label(_p[1])
                if re.match(r"^0x[0-9A-Fa-f]+$", _sb):
                    continue
                if _sb in implied_bases or _sb in chained_bases:
                    continue
                _tb, _ta = _split_label(_p[2])
                if re.match(r"^0x[0-9A-Fa-f]+$", _tb):
                    _tv = int(_tb, 16) + _ta
                else:
                    _tv = _resolve_model_label(_p[2], high_jt, msymtab)
                    if _tv is None:
                        _tv = _resolve_model_label(_p[2], high_jt, None)
                    if _tv is None and _tb in _tvals:
                        _tv = _tvals[_tb] + _ta
                if _tv is None or _tv == 0:
                    continue
                _g2.setdefault(_sb, []).append((_sa, _tv))
            for _sb, _edges in _g2.items():
                _sets, _tr = [], False
                for _sa, _tv in _edges:
                    _hits = valmap.get(_tv, [])
                    if len(_hits) > 64:
                        _tr = True
                        break
                    _bc = {o - _sa for o in _hits
                           if (o - _sa) % 4 == 0
                           and 0 <= o - _sa < len(payload)}
                    if not _bc:
                        # Same stale-value abstention as the pass-1 vote.
                        continue
                    _sets.append(_bc)
                if _tr or not _sets:
                    continue
                _inter = set.intersection(*_sets)
                if len(_inter) == 1 and _sb not in implied_bases:
                    _b = next(iter(_inter))
                    implied_bases[_sb] = (
                        _b, "multi-%d-r2" % len(_sets)
                        if len(_edges) > 1 else "single-r2")
                    _progress = True
            # Retries still-unresolved intern lines plus chain-mismatched
            # lines (a mismatch can be a stale-base slot whose true base
            # the fixpoint has since recovered, e.g. Ness PKThunder
            # self/outer edges after the combined-block implied base).
            # Kinds without mismatches iterate the same set as before.
            for _ln in list(model_unresolved) + [
                    _m for _m in list(chain_mismatch)
                    if _m not in model_unresolved]:
                _p = _ln.split()
                if len(_p) < 3 or _p[0] != "intern":
                    continue
                _sb, _sa = _split_label(_p[1])
                _sc = []
                _v = _resolve_model_label(_p[1], high_jt, msymtab)
                if _v is not None:
                    _sc.append((_v, "symtab"))
                _v = _resolve_model_label(_p[1], high_jt, None)
                if _v is not None and all(_v != x for x, _ in _sc):
                    _sc.append((_v, "heuristic"))
                if _sb in implied_bases:
                    _v = implied_bases[_sb][0] + _sa
                    if all(_v != x for x, _ in _sc):
                        _sc.append((_v, "implied-"
                                         + implied_bases[_sb][1]))
                if _sb in chained_bases:
                    _v = chained_bases[_sb][0] + _sa
                    if all(_v != x for x, _ in _sc):
                        _sc.append((_v, chained_bases[_sb][1]))
                if not _sc:
                    continue
                _tb, _ta = _split_label(_p[2])
                _tc = []
                if re.match(r"^0x[0-9A-Fa-f]+$", _tb):
                    _tc.append((int(_tb, 16) + _ta, "literal"))
                else:
                    for _vv, _ss in (
                            (_resolve_model_label(_p[2], high_jt, msymtab),
                             "symtab"),
                            (_resolve_model_label(_p[2], high_jt, None),
                             "heuristic")):
                        if _vv is not None and all(_vv != x for x, _ in _tc):
                            _tc.append((_vv, _ss))
                    if _tb in implied_bases:
                        _vv = implied_bases[_tb][0] + _ta
                        if all(_vv != x for x, _ in _tc):
                            _tc.append((_vv, "implied-target"))
                    if _tb in chained_bases:
                        _vv = chained_bases[_tb][0] + _ta
                        if all(_vv != x for x, _ in _tc):
                            _tc.append((_vv, "chained-target"))
                _agree = []
                if not _tc:
                    if len(_sc) == 1:
                        _s = _sc[0][0]
                        if 0 <= _s < len(payload):
                            _m = decode_ptr(u32be(payload, _s))
                            _agree = [((_s, _m),
                                       (_sc[0][1], "payload-derived"))]
                else:
                    for _s, _ss in _sc:
                        if not (0 <= _s < len(payload)):
                            continue
                        _m = decode_ptr(u32be(payload, _s))
                        for _t, _ts in _tc:
                            if _m == _t and (_s, _t) not in [
                                    _a[0] for _a in _agree]:
                                _agree.append(((_s, _t), (_ss, _ts)))
                if len(_agree) != 1:
                    continue
                (slot_old, tgt_old), (ssrc, tsrc) = _agree[0]
                prov = "r2-slot:%s+target:%s" % (ssrc, tsrc)
                _ab = [b for b in (_sb, _tb) if b in opaque_aliases]
                if _ab:
                    base_disputes.append(
                        {"line": _ln, "slot": slot_old, "target": tgt_old,
                         "via": "%s+opaque-alias(%s)" % (
                             prov, ",".join(
                                 "%s->0x%x" % (
                                     b, opaque_aliases[b]["base_old"])
                                 for b in _ab))})
                base_disputes.append({"line": _ln, "slot": slot_old,
                                      "target": tgt_old, "via": prov})
                _sk = in_kept(slot_old)
                _tk = in_kept(tgt_old)
                _dl = in_dl(tgt_old)
                _vx = in_vtx(tgt_old)
                _gp = arrays_end <= tgt_old < images_start
                _lw = tgt_old in (low_jt, low_apost, low_mobj)
                if not _sk:
                    model_intern_pruned.append(
                        {"slot_old": slot_old, "target_old": tgt_old,
                         "target_class": ("kept-elsewhere" if _tk
                                          else "pruned-dl" if _dl
                                          else "pruned-vtx" if _vx
                                          else "outside-kept"),
                         "provenance": prov, "line": _ln})
                elif _tk:
                    _tn = remap(tgt_old)
                    if _tn is None:
                        closure_holes.append(_ln)
                        bad("closure hole: kept target 0x%x unmapped: %s",
                            tgt_old, _ln)
                    else:
                        model_intern_ret.append(
                            {"slot_old": slot_old,
                             "slot_new": remap(slot_old),
                             "target_old": tgt_old, "target_new": _tn,
                             "action": "repoint-kept-section",
                             "provenance": prov, "line": _ln})
                elif _dl or _vx or _gp or _lw:
                    model_intern_ret.append(
                        {"slot_old": slot_old,
                         "slot_new": remap(slot_old),
                         "target_old": tgt_old, "target_new": "sentinel",
                         "action": "repoint-sentinel-NULL-semantics",
                         "provenance": prov, "line": _ln,
                         "reason": ("pruned-dl" if _dl
                                    else "pruned-vtx" if _vx
                                    else "LOW-detail-pinned-HIGH" if _lw
                                    else "geometry-gap-native-draw-owned")})
                else:
                    closure_holes.append(_ln)
                    bad("closure hole: kept slot 0x%x target 0x%x "
                        "outside kept/pruned: %s", slot_old, tgt_old, _ln)
                    continue
                if _ln in model_unresolved:
                    model_unresolved.remove(_ln)
                if _ln in chain_mismatch:
                    chain_mismatch.remove(_ln)
                for _msg in ("model slot unresolvable: %s" % _ln,
                             "model slot ambiguous, target unresolvable: %s"
                             % _ln,
                             *chain_mismatch_notes.get(_ln, [])):
                    _retract(_msg)
                _progress = True
            if not _progress:
                break
    # Idle table/internal proof: parse idle relocData .c the same way as
    # Selected, then cross-check the raw OLER data words.
    idle_cands = list(RELOCDATA.glob("%d_*.c" % mf0["asset"]["id"]))
    pidle = None
    idle_proof = {"file": mf0["asset"]["id"], "slots": []}
    if not idle_cands:
        bad("idle relocData missing for file %d", mf0["asset"]["id"])
    else:
        idle_stem = idle_cands[0].stem.split("_", 1)[1]
        pidle = parse_selected_c(mf0["asset"]["id"], idle_stem)
        if pidle is None:
            bad("idle parse failure for %s", idle_cands[0].name)
        else:
            if len(idle_data) != pidle["total_bytes"]:
                bad("idle OLER %d != parsed total %d",
                    len(idle_data), pidle["total_bytes"])
            else:
                for rs in pidle["reloc_slots"]:
                    want = pidle["joint_offsets"][rs["target"]]
                    got = decode_ptr(u32be(idle_data, rs["slot"] * 4))
                    if got != want:
                        bad("idle chain mismatch slot %d: 0x%x != 0x%x",
                            rs["slot"], got, want)
                    idle_proof["slots"].append(
                        {"slot": rs["slot"], "target": rs["target"],
                         "target_offset": want, "oler_word": got})
                nulls = [i for i in range(pidle["table_full"])
                         if pidle["order"][i] is None]
                for i in nulls:
                    if decode_ptr(u32be(idle_data, 4 * i)) != 0:
                        bad("idle NULL slot %d nonzero in OLER", i)
            idle_proof.update(
                {"c_file": idle_cands[0].name, "table_bytes": pidle[
                    "table_bytes"] if pidle else None,
                 "table_entries": pidle["table_full"] if pidle else None,
                 "streams_bytes": pidle["bytes"] if pidle else None,
                 "total_bytes": pidle["total_bytes"] if pidle else None,
                 "payload_sha256": pidle["payload_sha256"] if pidle
                 else None})
    # Selected per-slot proof (already parsed but dropped): re-emit with
    # table cross-check against the real bin payload words.
    sel_new_base = new - psel["total_bytes"]
    selected_proof = {"file": sel_fid,
                      "table_bytes": psel["table_bytes"],
                      "table_entries": psel["table_full"],
                      "streams_bytes": psel["bytes"],
                      "total_bytes": psel["total_bytes"],
                      "payload_sha256": psel["payload_sha256"],
                      "table_new": [sel_new_base,
                                    sel_new_base + psel["table_bytes"]],
                      "streams_new": [sel_new_base + psel["table_bytes"],
                                      sel_new_base + psel["total_bytes"]],
                      "slots": []}
    for rs in psel.get("reloc_slots", []):
        want = psel["joint_offsets"][rs["target"]]
        selected_proof["slots"].append(
            {"slot": rs["slot"], "target": rs["target"],
             "target_offset": want, "line": rs["line"]})
    # Original target identities for pruned DL roots.
    high_set = {dd for _depth, dd in descs if dd is not None}
    pruned_dl_roots = [{"target_old": k, "len": v,
                        "class": ("HIGH-display" if k in high_set
                                  else "extern-modelparts/skeleton/unknown")}
                       for k, v in sorted(dl_spans.items())]
    idle_new_base = next(s for s in sections
                         if s["name"] == "anim-idle")["new"]
    section_boundaries = {
        "main": [0, main_a["data_bytes"]],
        "model_spans": [{"old": s, "len": e - s, "new": pm["new"]}
                        for (s, e), pm in zip(model_spans, pointer_map)],
        "idle": {"old": 0, "len": len(idle_data),
                 "new": [idle_new_base,
                         idle_new_base + len(idle_data)]},
        "selected": {"old": 0, "len": psel["total_bytes"],
                     "new": selected_proof["table_new"][0:1] + [
                         sel_new_base + psel["total_bytes"]]},
        "sentinel": [new, new + 8]}
    # Negative controls: deterministic digest plus corrupted-pointer check
    # plus zeroed-Selected rejection. All measured in this run.
    fix_digest_src = json.dumps(
        {"r": sorted(model_intern_ret, key=lambda d: d["slot_old"]),
         "e": sorted(model_extern_kept, key=lambda d: d["slot_old"]),
         "s": selected_proof["slots"]}, sort_keys=True).encode()
    fix_digest = hashlib.sha256(fix_digest_src).hexdigest()
    fix_digest2 = hashlib.sha256(fix_digest_src).hexdigest()
    if fix_digest != fix_digest2:
        bad("nondeterministic fixup digest")
    corrupt_ok = False
    corrupt_detail = "no-retained-slot"
    if model_intern_ret:
        first = next(r for r in model_intern_ret
                     if r["action"] == "repoint-kept-section")
        co = first["slot_old"]
        cw = u32be(payload, co) ^ 0x1
        cm = decode_ptr(cw)
        corrupt_ok = (cm != first["target_old"])
        corrupt_detail = ("slot 0x%x meas 0x%x corrupt 0x%x detected=%s"
                          % (co, first["target_old"], cm, corrupt_ok))
        if not corrupt_ok:
            bad("corrupted-pointer control missed: %s", corrupt_detail)
    zero_ok = (hashlib.sha256(bytes(psel["total_bytes"])).hexdigest()
               != psel["payload_sha256"])
    if not zero_ok:
        bad("zeroed-Selected control missed")
    negative_controls = {
        "fixup_digest_sha256": fix_digest,
        "deterministic_redigest_match": fix_digest == fix_digest2,
        "corrupted_pointer_detected": corrupt_ok,
        "corrupted_pointer_detail": corrupt_detail,
        "zeroed_selected_rejected": zero_ok}
    # ---- Numeric Main pointer records (Main-owned runtime fixup input).
    # Every Main .reloc line resolves to exact numbers: slot_old/slot_new
    # (Main identity, new==old), target source file/section, and
    # target_old/target_new where retained. Slots come only from the
    # source .reloc labels via _main_symtab (same @ comments as the
    # existing section resolver + attr tail base); each slot is
    # cross-checked against the original OLER word
    # (decode_ptr(main_data[slot]) == target_old) and every Model target
    # against kept section bounds. No guessed addresses and no
    # whole-table textual-only acceptance: any retained line that does
    # not resolve + verify fails. Pruned DL targets carry their original
    # DL identity (target_old + dl_len + dl_class); non-DL prunes carry
    # an explicit classification. Existing Selected/Model data and source
    # flag fields are untouched; Ness Model exceptions flow through
    # unchanged below.
    main_c_text = main_c.read_text(encoding="utf-8", errors="replace")
    attr_base = next((s["old"] for s in secs if s["name"] == "attributes"),
                     None)
    main_sym = _main_symtab(main_c_text, attr_base, secs) \
        if attr_base is not None else {}
    if attr_base is None:
        bad("main attr tail missing, numeric records impossible")
    tail_new_of = {}
    for s in sections:
        if s["src"].startswith("file-"):
            try:
                _fid = int(s["src"].split("-", 1)[1])
            except ValueError:
                continue
            tail_new_of[_fid] = (s["new"], s["len"])
    main_intern_num, main_ext_kept, main_ext_pruned = [], [], []
    main_unresolved, main_mismatch, main_holes = [], [], []
    high_set_local = {dd for _depth, dd in descs if dd is not None}
    for ln in lines:
        parts = ln.split()
        if len(parts) < 3 or parts[0] not in ("intern", "extern"):
            continue
        kind_l = parts[0]
        slot_old = _resolve_main_label(parts[1], main_sym, attr_base) \
            if attr_base is not None else None
        if slot_old is None:
            main_unresolved.append(ln)
            bad("main slot unresolvable: %s", ln)
            continue
        if not (0 <= slot_old + 4 <= len(main_data)):
            bad("main slot 0x%x out of range: %s", slot_old, ln)
            continue
        oler_word = u32be(main_data, slot_old)
        oler_tgt = decode_ptr(oler_word)
        slot_sec = _main_section_of(secs, slot_old)
        if slot_sec is None:
            bad("main slot 0x%x outside sections: %s", slot_old, ln)
            continue
        if kind_l == "intern":
            tgt_old = _resolve_main_label(parts[2], main_sym, attr_base)
            if tgt_old is None:
                main_unresolved.append(ln)
                bad("main intern target unresolvable (retained): %s", ln)
                continue
            if oler_tgt != tgt_old:
                main_mismatch.append(ln)
                bad("main chain mismatch slot 0x%x meas 0x%x != reloc "
                    "0x%x: %s", slot_old, oler_tgt, tgt_old, ln)
                continue
            tgt_sec = _main_section_of(secs, tgt_old)
            if tgt_sec is None or not (0 <= tgt_old < main_a["data_bytes"]):
                main_holes.append(ln)
                bad("main intern target 0x%x outside kept Main: %s",
                    tgt_old, ln)
                continue
            main_intern_num.append(
                {"line": ln, "slot_old": slot_old, "slot_new": slot_old,
                 "slot_section": slot_sec["name"],
                 "target_symbol": parts[2], "target_file": main_a["id"],
                 "target_old": tgt_old, "target_new": tgt_old,
                 "target_section": tgt_sec["name"],
                 "oler_word": "0x%08x" % oler_word,
                 "oler_target": oler_tgt, "action": "identity-old-offset"})
        else:
            mm = re.search(r"file (\d+)", ln)
            fid = int(mm.group(1)) if mm else -1
            try:
                tgt_old = int(parts[2], 16)
            except ValueError:
                main_unresolved.append(ln)
                bad("main extern without numeric target: %s", ln)
                continue
            if oler_tgt != tgt_old:
                main_mismatch.append(ln)
                bad("main chain mismatch slot 0x%x meas 0x%x != reloc "
                    "0x%x: %s", slot_old, oler_tgt, tgt_old, ln)
                continue
            if fid == model_a["id"]:
                tgt_kept = in_kept(tgt_old)
                tgt_dl = in_dl(tgt_old)
                tgt_vtx = in_vtx(tgt_old)
                tgt_gap = arrays_end <= tgt_old < images_start
                tgt_low = tgt_old in (low_jt, low_apost, low_mobj)
                if tgt_kept:
                    tn = remap(tgt_old)
                    if tn is None:
                        main_holes.append(ln)
                        bad("main closure hole: kept target 0x%x "
                            "unmapped: %s", tgt_old, ln)
                        continue
                    if tgt_old in (high_jt, high_mobj, high_apost):
                        reason = "HIGH-topology/MObj/Apost"
                    elif any(s <= tgt_old < e for s, e in spans_a) or any(
                            s <= tgt_old < e for s, e in spans_c):
                        reason = "sprite-tail/image-kept"
                    else:
                        reason = "kept"
                    main_ext_kept.append(
                        {"line": ln, "slot_old": slot_old,
                         "slot_new": slot_old,
                         "slot_section": slot_sec["name"],
                         "target_file": fid, "target_old": tgt_old,
                         "target_new": tn, "target_class": reason,
                         "oler_word": "0x%08x" % oler_word,
                         "oler_target": oler_tgt,
                         "action": "repoint-kept-section"})
                elif tgt_dl or tgt_vtx or tgt_gap or tgt_low:
                    dl_len = dl_spans.get(tgt_old)
                    main_ext_pruned.append(
                        {"line": ln, "slot_old": slot_old,
                         "slot_new": slot_old,
                         "slot_section": slot_sec["name"],
                         "target_file": fid, "target_old": tgt_old,
                         "target_new": "sentinel",
                         "action": "repoint-sentinel-NULL-semantics",
                         "reason": ("pruned-dl" if tgt_dl
                                    else "pruned-vtx" if tgt_vtx
                                    else "LOW-detail-pinned-HIGH" if tgt_low
                                    else "geometry-gap-native-draw-owned"),
                         "dl_len": dl_len,
                         "dl_class": ("HIGH-display"
                                      if tgt_old in high_set_local
                                      else "extern-modelparts/skeleton/"
                                           "unknown") if tgt_dl else None,
                         "oler_word": "0x%08x" % oler_word,
                         "oler_target": oler_tgt})
                else:
                    main_holes.append(ln)
                    bad("main closure hole: Model target 0x%x outside "
                        "kept/pruned: %s", tgt_old, ln)
            elif fid in tail_new_of and ("stock_luts" in parts[1]
                                         or "sprites" in parts[1]):
                _base, _len = tail_new_of[fid]
                if not (0 <= tgt_old < _len):
                    main_holes.append(ln)
                    bad("main tail target 0x%x outside file %d: %s",
                        tgt_old, fid, ln)
                    continue
                main_ext_kept.append(
                    {"line": ln, "slot_old": slot_old, "slot_new": slot_old,
                     "slot_section": slot_sec["name"], "target_file": fid,
                     "target_old": tgt_old, "target_new": _base + tgt_old,
                     "target_class": "tail-file-kept",
                     "oler_word": "0x%08x" % oler_word,
                     "oler_target": oler_tgt,
                     "action": "repoint-kept-section"})
            else:
                main_ext_pruned.append(
                    {"line": ln, "slot_old": slot_old, "slot_new": slot_old,
                     "slot_section": slot_sec["name"], "target_file": fid,
                     "target_old": tgt_old, "target_new": "sentinel",
                     "action": "repoint-sentinel-NULL-semantics",
                     "reason": ("no-demo-path-touch" if fid in core_ids
                                else "other-file-out-of-kind"),
                     "oler_word": "0x%08x" % oler_word,
                     "oler_target": oler_tgt})
    main_digest_src = json.dumps(
        {"i": sorted(main_intern_num, key=lambda d: d["slot_old"]),
         "k": sorted(main_ext_kept, key=lambda d: d["slot_old"]),
         "p": sorted(main_ext_pruned, key=lambda d: d["slot_old"])},
        sort_keys=True).encode()
    main_digest = hashlib.sha256(main_digest_src).hexdigest()
    main_redigest = hashlib.sha256(main_digest_src).hexdigest()
    if main_digest != main_redigest:
        bad("main fixup digest nondeterministic")
    main_corr_ok, main_corr_detail = False, "no-retained-slot"
    if main_intern_num:
        _f = main_intern_num[0]
        _cw = u32be(main_data, _f["slot_old"]) ^ 0x1
        _cm = decode_ptr(_cw)
        main_corr_ok = (_cm != _f["target_old"])
        main_corr_detail = ("slot 0x%x meas 0x%x corrupt 0x%x "
                            "detected=%s" % (_f["slot_old"],
                                             _f["target_old"], _cm,
                                             main_corr_ok))
        if not main_corr_ok:
            bad("main corrupted-pointer control missed: %s",
                main_corr_detail)
    entry["main_intern"] = {
        "reloc_lines": len(lines),
        "attr_base": attr_base,
        "retained": main_intern_num,
        "unresolved": main_unresolved,
        "chain_mismatches": main_mismatch,
        "closure_holes": main_holes,
        "digest_sha256": main_digest,
        "deterministic_redigest_match": main_digest == main_redigest,
        "corrupted_pointer_detected": main_corr_ok,
        "corrupted_pointer_detail": main_corr_detail}
    entry["main_extern"] = {
        "kept": main_ext_kept, "pruned": main_ext_pruned}
    checks.update(
        {"main_intern_retained": len(main_intern_num),
         "main_extern_kept": len(main_ext_kept),
         "main_extern_pruned": len(main_ext_pruned),
         "main_unresolved": len(main_unresolved),
         "main_chain_mismatches": len(main_mismatch),
         "main_closure_holes": len(main_holes),
         "main_controls_pass": bool(
             main_digest == main_redigest and main_corr_ok
             and not main_unresolved and not main_mismatch
             and not main_holes)})
    if main_unresolved or main_mismatch or main_holes:
        bad("main numeric gaps: unresolved %d mismatch %d holes %d",
            len(main_unresolved), len(main_mismatch), len(main_holes))
    # Main record self-test (pre-map, no bin yet): every retained Main
    # slot/target word must be readable in the source OLER data (bins
    # carry unrelocated bytes; the post-map bin check below re-verifies
    # the emitted Main slice equals main_data).
    for _r in main_intern_num + main_ext_kept:
        _pairs = [(_r["slot_old"], _r["slot_new"])]
        if _r.get("target_new") not in (None, "sentinel"):
            if _r.get("target_file") == main_a["id"]:
                _pairs.append((_r["target_old"], _r["target_new"]))
        for _oo, _nn in _pairs:
            if not (0 <= _oo + 4 <= len(main_data) and _nn == _oo):
                bad("main fixup bounds differ src 0x%x new 0x%x "
                    "(%s)", _oo, _nn, _r["line"])
                break
    if not (main_digest == main_redigest and main_corr_ok):
        bad("main negative controls failed")
    entry["model_intern"] = {
        "reloc_file": mreloc_path.name if mreloc_lines is not None else None,
        "total_lines": len(mreloc_lines) if mreloc_lines is not None else 0,
        "retained": model_intern_ret, "pruned_slots": model_intern_pruned,
        "unresolved": model_unresolved,
        "chain_mismatches": chain_mismatch,
        "closure_holes": closure_holes,
        "base_disputes": base_disputes,
        "implied_bases": {k: list(v) for k, v in implied_bases.items()},
        "chained_bases": {k: list(v) for k, v in chained_bases.items()},
        "emblem_anchor": emblem_anchor}
    # Conditional key: absent for kinds without stale opaque blocks, so
    # their maps stay byte-identical.
    if opaque_aliases:
        entry["model_intern"]["opaque_aliases"] = {
            b: {"base_old": "0x%x" % info["base_old"],
                "anchor": info["anchor"],
                "anchor_off": "0x%x" % info["anchor_off"]}
            for b, info in sorted(opaque_aliases.items())}
    entry["model_extern"] = {
        "kept": model_extern_kept, "pruned": model_extern_pruned,
        "absent": model_extern_absent,
        "unclassified": model_extern_unclassified}
    entry["idle_proof"] = idle_proof
    entry["selected_proof"] = selected_proof
    entry["pruned_dl_roots"] = pruned_dl_roots
    entry["section_boundaries"] = section_boundaries
    entry["negative_controls"] = negative_controls
    checks.update(
        {"model_intern_retained": len(model_intern_ret),
         "model_intern_pruned_slots": len(model_intern_pruned),
         "model_extern_kept": len(model_extern_kept),
         "model_extern_pruned": len(model_extern_pruned),
         "model_extern_absent": len(model_extern_absent),
         "idle_slots": len(idle_proof["slots"]),
         "selected_slots": len(selected_proof["slots"]),
         "pruned_dl_roots": len(pruned_dl_roots),
         "model_chained_bases": len(chained_bases),
         "model_emblem_anchor": emblem_anchor is not None,
         "controls_pass": bool(fix_digest == fix_digest2 and corrupt_ok
                               and zero_ok)})
    if opaque_aliases:
        checks["model_opaque_aliases"] = len(opaque_aliases)
    if model_unresolved or chain_mismatch or closure_holes:
        bad("relocation proof gaps: unresolved %d mismatch %d holes %d",
            len(model_unresolved), len(chain_mismatch),
            len(closure_holes))
    # New-offset self-check against the independently accumulated
    # model sections (catches span-base slips as hard failures).
    for _r in model_intern_ret + model_extern_kept:
        _pairs = [(_r["slot_old"], _r["slot_new"])]
        if _r.get("target_new") not in (None, "sentinel"):
            _pairs.append((_r["target_old"], _r["target_new"]))
        for _oo, _nn in _pairs:
            _sec = next((s for s in sections if s["src"] == "Model"
                         and s["old"] <= _oo < s["old"] + s["len"]), None)
            if _sec is None or _sec["new"] + (_oo - _sec["old"]) != _nn:
                bad("fixup new offset 0x%x not in model sections for "
                    "old 0x%x (%s)", _nn, _oo, _r["line"])
    if model_extern_unclassified:
        bad("model extern unclassified %d", len(model_extern_unclassified))
    if not (fix_digest == fix_digest2 and corrupt_ok and zero_ok):
        bad("negative controls failed")
    low = key.lower()
    entry["failures"] = list(local_fail)
    (Path(output_dir) / ("%s_compact_map.json" % low)).write_text(json.dumps(
        {"emitted_bytes": emitted,
         "model_kept_bytes": sum(e - s for s, e in model_spans),
         "sections": sections, "fixups": fixups, "checks": checks,
         "failures": local_fail, "pointer_map": pointer_map,
         "roots": {"high_jt": high_jt, "high_apost": high_apost,
                   "low_jt": low_jt, "low_mobj": low_mobj,
                   "low_apost": low_apost},
         "spans_a": entry["spans_a"], "span_b": entry["span_b"],
         "spans_c": entry["spans_c"],
         "mobj": {"arrays": entry["mobj_arrays"],
                  "array_bytes": entry["mobj_array_bytes"],
                  "subs": entry["mobj_subs"],
                  "sub_bytes": entry["mobj_sub_bytes"],
                  "span": entry["mobj_span"],
                  "tile_0x78": entry["mobj_tile_0x78"],
                  "slots": entry["mobj_slots"]},
         "image": {"runs": entry["image_runs"],
                   "table_bytes": entry["image_table_bytes"],
                   "terminated": entry["image_table_terminated"],
                   "targets": entry["img_targets"],
                   "start": entry["images_start"]},
         "mobj_subs": [hex(s) for s in subs],
         "img_targets": [hex(t) for t in img_targets],
         "apost_cells": [hex(c) for c in cells],
         "apost_dispatch": entry["apost_dispatch"],
         "tail_files": entry.get("tail_files", []),
         "unmeasured_targets": entry.get("unmeasured_targets", []),
         "c_residual_dl_symbols": entry.get("c_residual_dl_symbols", []),
         "vtx": {"extents": entry.get("vtx_extents"),
                 "unlocatable": entry.get("vtx_unlocatable", []),
                 "excised_a": entry.get("vtx_excised_a")},
         "dl_spans": {hex(k): v for k, v in sorted(dl_spans.items())},
         "costumes": entry.get("costumes_source"),
         "main_gap_sections": entry.get("main_gap_sections", 0),
         "idle": {"symbol": entry["idle_anim_symbol"],
                  "script": entry["idle_script"],
                  "flags": entry["idle_flags"],
                  "file": entry["idle_file"],
                  "bytes": entry["idle_bytes"]},
          "selected": {"symbol": entry["selected_anim_symbol"],
                       "script": entry["selected_script"],
                       "flags": entry["selected_flags"],
                       "file": entry["selected_file"],
                       "parse": entry["selected_parse"]},
           "model_intern": entry.get("model_intern"),
           "model_extern": entry.get("model_extern"),
           "main_intern": entry.get("main_intern"),
           "main_extern": entry.get("main_extern"),
           "idle_proof": entry.get("idle_proof"),
          "selected_proof": entry.get("selected_proof"),
          "pruned_dl_roots": entry.get("pruned_dl_roots"),
          "section_boundaries": entry.get("section_boundaries"),
          "negative_controls": entry.get("negative_controls")},
         indent=1))
    img = bytearray(b"MCM2" + struct.pack("<I", emitted))
    body = bytearray(main_data)
    for s, e in model_spans:
        body += payload[s:e]
    for tf in tail_files:
        body += tf["_data"]
    body += idle_data
    sel_payload = psel["_payload"]
    if len(sel_payload) != psel["total_bytes"]:
        bad("selected payload length %d != total %d",
            len(sel_payload), psel["total_bytes"])
        return entry, None, local_fail
    if any(b != 0 for b in sel_payload) is False:
        bad("selected payload all zeros")
        return entry, None, local_fail
    body += sel_payload
    body += bytes(8)
    assert len(body) == emitted, (len(body), emitted)
    for (s, e), pm in zip(model_spans, pointer_map):
        sl = bytes(payload[s:e])
        off = pm["new"] - main_a["data_bytes"] + len(main_data)
        assert body[off:off + (e - s)] == sl
    # Per-fixup bin fidelity: every remapped slot/target word in the bin
    # must equal its source payload word (bin carries unrelocated bytes).
    for _r in model_intern_ret + model_extern_kept:
        _pairs = [(_r["slot_old"], _r["slot_new"])]
        if _r.get("target_new") not in (None, "sentinel"):
            _pairs.append((_r["target_old"], _r["target_new"]))
        for _oo, _nn in _pairs:
            if bytes(body[_nn:_nn + 4]) != bytes(payload[_oo:_oo + 4]):
                bad("fixup bytes differ bin 0x%x vs source 0x%x (%s)",
                    _nn, _oo, _r["line"])
                break
    # Main bin fidelity: Main slice is identity (new==old), so every
    # retained Main slot/target word in the bin must equal main_data.
    for _r in main_intern_num + main_ext_kept:
        _pairs = [(_r["slot_old"], _r["slot_new"])]
        if _r.get("target_new") not in (None, "sentinel"):
            if _r.get("target_file") == main_a["id"]:
                _pairs.append((_r["target_old"], _r["target_new"]))
        for _oo, _nn in _pairs:
            if bytes(body[_nn:_nn + 4]) != bytes(main_data[_oo:_oo + 4]):
                bad("main fixup bytes differ bin 0x%x vs source 0x%x "
                    "(%s)", _nn, _oo, _r["line"])
                break
    # Content/structure self-check against real source data (not length
    # only): the bin Selected slice must equal the source-derived table +
    # joint streams. Zeroing the payload fails the byte compare, the
    # nonzero-guard, and the table-structure checks below.
    sel_new = next(s for s in sections if s["name"] == "anim-selected")["new"]
    sel_off = sel_new - main_a["data_bytes"] + len(main_data)
    sel_slice = bytes(body[sel_off:sel_off + psel["total_bytes"]])
    if sel_slice != bytes(sel_payload):
        bad("selected bin slice differs from source payload")
        return entry, None, local_fail
    if hashlib.sha256(sel_slice).hexdigest() != psel["payload_sha256"]:
        bad("selected payload hash mismatch")
        return entry, None, local_fail
    if sel_slice == bytes(len(sel_slice)):
        bad("selected payload zeroed")
        return entry, None, local_fail
    table_full = psel["table_full"]
    table_bytes = psel["table_bytes"]
    table_vals = [struct.unpack_from(">I", sel_slice, 4 * i)[0]
                  for i in range(table_full)]
    if table_vals.count(0) != psel["null_count"]:
        bad("selected table NULL count %d != source %d",
            table_vals.count(0), psel["null_count"])
        return entry, None, local_fail
    for i, entry_name in enumerate(psel["order"]):
        want = 0 if entry_name is None else psel["joint_offsets"][entry_name]
        if table_vals[i] != want:
            bad("selected table entry %d mismatch", i)
            return entry, None, local_fail
    streams_off = table_bytes
    for entry_name in psel["order"]:
        if entry_name is None:
            continue
        decl = psel["joint_decl"][entry_name]
        chunk = sel_slice[streams_off:streams_off + decl * 2]
        if len(chunk) != decl * 2:
            bad("selected joint %s truncated", entry_name)
            return entry, None, local_fail
        want_words = psel["_joint_words"][entry_name]
        got_words = [struct.unpack_from(">H", chunk, 2 * k)[0]
                     for k in range(decl)]
        if got_words != want_words:
            bad("selected joint %s content differs", entry_name)
            return entry, None, local_fail
        if got_words[-1] != 0:
            bad("selected joint %s missing ftAnimEnd", entry_name)
            return entry, None, local_fail
        streams_off += decl * 2
    if streams_off != len(sel_slice):
        bad("selected streams end %d != payload %d",
            streams_off, len(sel_slice))
        return entry, None, local_fail
    (Path(output_dir) / ("%s_compact.bin" % low)).write_bytes(bytes(img) + bytes(
        body))
    entry["emitted_bytes"] = emitted
    entry["model_kept_bytes"] = sum(e - s for s, e in model_spans)
    return entry, emitted, local_fail


def _normalize_kinds(requested):
    if requested is None:
        return list(KINDS)
    if isinstance(requested, str):
        if requested.strip() == "":
            return list(KINDS)
        items = [s.strip() for s in requested.split(",") if s.strip() != ""]
    else:
        items = list(requested)
    lookup = {}
    for disp, key, sel_idx in KINDS:
        lookup[key.lower()] = (disp, key, sel_idx)
        lookup[disp.lower()] = (disp, key, sel_idx)
    out = []
    for name in items:
        hit = lookup.get(str(name).lower())
        if hit is None:
            raise ValueError("unknown kind: %s" % (name,))
        if hit not in out:
            out.append(hit)
    return out


def generate(output_dir, kinds=None):
    """Generate MCM2 compact bins+maps into output_dir.

    kinds is None (all 12, Ness stays red) or a subset of helper/display
    names; the production encoder calls generate(dir, ["mario", ...]).
    Importing this module performs no writes.
    """
    fighters = str(REPO / "scripts" / "fighters")
    if fighters not in sys.path:
        sys.path.insert(0, fighters)
    import generate_nds_native_owners as gen
    global FAILURES
    FAILURES = []
    selected = _normalize_kinds(kinds)
    man = json.loads((REPO / "scripts/fighters/fighter_production_manifest.json"
                      ).read_text(encoding="utf-8"))
    man_by_fighter = {f["fighter"]: f for f in man["fighters"]}
    ftparam = (REPO / "decomp/BattleShip-main/decomp/src/ft/ftparam.c"
               ).read_text(encoding="utf-8", errors="replace")
    costume_rows = re.findall(
        r"\{\s*\{([^}]*)\},\s*\{([^}]*)\},\s*(\d+)\s*\},?\s*//\s*(.+)",
        ftparam)
    costumes = {c[3].strip(): (c[0].strip(), c[1].strip(), int(c[2]))
                for c in costume_rows}
    out_dir = Path(output_dir)
    out_dir.mkdir(parents=True, exist_ok=True)
    out = {"kinds": []}
    for disp, key, sel_idx in selected:
        entry, emitted, _lf = build_kind(gen, man_by_fighter, costumes,
                                         disp, key, sel_idx, out_dir)
        out["kinds"].append(entry)
    out["failures"] = list(FAILURES)
    out["ok"] = not FAILURES and all(
        not k.get("failures") for k in out["kinds"])
    return out


def main(argv=None):
    ap = argparse.ArgumentParser(
        description="Generate MCM2 compact preview source metadata")
    ap.add_argument("--output-dir", required=True,
                    help="directory for *_compact.bin + *_compact_map.json")
    ap.add_argument("--kinds", default=None,
                    help="comma-separated subset of kind names "
                         "(default: all 12, Ness stays red)")
    args = ap.parse_args(argv)
    try:
        out = generate(args.output_dir, args.kinds)
    except ValueError as e:
        print("error: %s" % (e,))
        raise SystemExit(2)
    print(json.dumps({
        "ok": out["ok"],
        "kinds": [(k["fighter"], k.get("emitted_bytes"),
                   k.get("failures", [])) for k in out["kinds"]],
        "failures": out["failures"]}, indent=1))
    if not out["ok"]:
        raise SystemExit(1)
    return 0


if __name__ == "__main__":
    main()



