#!/usr/bin/env python3
"""Compile the Mario/Fox FIGATREE corpus into compact typed track rows, and
prove the rows reproduce the shipped parser exactly.

Task 3 of the owner's queue: *animation representation*. The measured mechanism
is the PARSE half of the animation lane -- `ndsR2FtAnimParseDObjFigatree` +
`ndsR2AnimBuildTrackTable` + `ndsR2AnimTargetValue` + `ndsR2AnimAObjToQConvert`,
41,376 tk/fr at rank-80 on `build-c192-sitr-profile-gxc`
(`artifacts/performance/2026-08-15_sitr-direct-children/`). It re-derives AObj
node fields from static ROM bytes 96.94 times a marginal frame.

WHAT THIS IS NOT. It is **not** the 20-byte baked-write-record bank
(`ftanim_bake.py` / `generate_ftanim_dense_bank.py`), which the owner closed:
that form is 3.68x the source stream on this corpus and cannot be resident. A
write record stores `value_base`, `rate_base`, `length` and `length_invert` per
(command, track); every one of those is DERIVABLE at run time --
`value_base`/`rate_base` chain from the previous row of the same track,
`length` is the clock, `length_invert` is a table lookup on the frame count. So
the row stores only what is authored: the kind, the track mask, the frame count
and the s16 target words.

AUTHORED VALUES ARE STORED UNCHANGED, and that is a size result as much as a
fidelity one. `ndsR2AnimTargetValue` scales an s16 by a per-track power of two,
so the Q12 word it writes needs up to 26 bits and is a pure left shift of the
authored word (`ndsR2AnimArgToQ`, `nds_anim_fixed.h:166`). Storing the s16 and
shifting at run time is exact AND half the size of storing the Q word.

THE PROOF HAS THREE LAYERS, because a single one of them would be a tautology.

  A. EMITTER FIDELITY. The pack is decoded back with its own reader and the
     recovered (kind, track mask, frame count, per-track words, jump target)
     sequence is compared against the same sequence derived from the o2r file.
     Catches wrong offsets, dropped or reordered commands, a wrong mask, a lost
     rate word, a mis-targeted jump.
  B. SEMANTIC EQUIVALENCE. The proven host model
     (`ftanim_script_model.run_commands`, driven from the o2r bytes) and a
     dense replayer (driven from the PACK bytes) execute the same scripts over
     `--passes` loop iterations, and their per-command, per-track state, their
     `anim_wait` timeline and their callback tag sequence are compared. Neither
     side can see the other's input, which is what makes this a test.
  C. QUANTISATION EXACTNESS. Layers A and B are representation independent, so
     they would still pass if the runtime shift were wrong. Layer C closes that
     by evaluating the shipped `ndsR2AnimTargetValue(..., q=1)` expression over
     every (track, value-or-rate, authored word) triple the corpus actually
     contains and comparing it to the shift the runtime would apply.

WHAT THE TWO ARMS SHARE, stated because a shared decoder is how this campaign
once got `mismatch = 0` against a wrong bit order: both arms read the o2r bank
through `ftanim_reloc_probe`, and both take the parser's semantics from the same
transcription. **Two defect classes are therefore invisible to every layer here**
-- a wrong o2r reader, and a wrong model of the shipped parser. Neither is
closed by anything in this file, and neither is claimed to be:

  * the reader is validated by EXECUTION -- the same module emits
    `battlepack_fox.bin`, which the shipping ROM's own C parser consumes and
    the game plays;
  * the parser transcription is guarded by `check_ftanim_transcribe.py`, which
    compares the port body against the decomp body as a token stream, and by
    `check_ftanim_target_exact.py` for the value scaling.

A run of this script alone is not evidence about either. Say so when quoting it.
"""

from __future__ import annotations

import argparse
import collections
import hashlib
import importlib.util
import json
import pathlib
import struct
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
BANK = ROOT / "decomp" / "BattleShip-main" / "BattleShip_o2r" / "reloc_animations"

# scvsbattle P1: items are off and every item SetStatus in the shipped battle ELF
# is a `bx lr` stub -- proven in BATTLEPACK_ANIMATION.md 11.2, not by name.
ITEM_IDS = frozenset([0x24C + i for i in range(18)] + [0x281] +
                     [0x2DD + i for i in range(18)] + [0x31F])

MAGIC = b"FTTP"
VERSION = 1

# Row kinds. The opcode surface is closed at 15
# (`check_ftanim_opcode_surface.py`); these nine are the classes the six
# write-families and three control commands collapse to once each `*Block`
# half folds into a header bit.
K_END, K_BLOCK, K_LINEAR, K_CUBIC2, K_RATE, K_CUBIC0, K_STEP, K_ADDLEN, K_LOOP = range(9)
KIND_NAME = ("End", "Block", "Linear", "Cubic2", "Rate", "Cubic0", "Step",
             "AddLen", "Loop")

OP_TO_KIND = {0: K_END, 1: K_BLOCK,
              2: K_LINEAR, 3: K_LINEAR,
              4: K_CUBIC2, 5: K_CUBIC2,
              6: K_RATE,
              7: K_CUBIC0, 8: K_CUBIC0,
              9: K_STEP, 10: K_STEP,
              11: K_ADDLEN, 13: K_LOOP}
BLOCK_OPS = frozenset((2, 4, 7, 9))                     # advance anim_wait
WORDS_PER_TRACK = {K_LINEAR: 1, K_CUBIC2: 2, K_RATE: 1, K_CUBIC0: 1, K_STEP: 1}

# Opcodes 12 (SetTranslateInterp) and 14 (SetFlags) are absent on purpose: they
# write `aobj->interpolate` and `dobj->flags`, which a track row cannot carry.
# A clip containing one is REFUSED, never silently mis-encoded.
UNSUPPORTED_OPS = {12: "SetTranslateInterp", 14: "SetFlags"}

HDR_MASK, HDR_FRAMES, HDR_BLOCK = 4, 14, 15

# battleship_ftanim.c:187 -- the shift per (track class, value/rate). ids 3 and
# 7 are TraI, whose 1/16384-3e-12 is not a power of two.
FRAC_SHIFT = (9, 2, 12, 0, 9, 5, 13, 0)
TRACK_ID = (0, 0, 0, 3, 1, 1, 1, 2, 2, 2)               # mask bit -> value id
TRACK_NAME = ("RotX", "RotY", "RotZ", "TraI", "TraX", "TraY", "TraZ",
              "ScaX", "ScaY", "ScaZ")
AQ_VF, AQ_RF, AQ_IF, AQ_LF = 12, 16, 30, 12
TRAI_SCALE = 1.0 / 16384.0 - 3.0 / 1000000000000.0


def _load(name):
    spec = importlib.util.spec_from_file_location(name, ROOT / "scripts" / (name + ".py"))
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


probe = _load("ftanim_reloc_probe")
model = _load("ftanim_script_model")


# --------------------------------------------------------------------------- #
# The shipped quantisation, in Python, for layer C.

def f32_to_fixed(v: float, bits: int) -> int:
    """`ndsR2F32ToFixed` -- round to nearest, magnitude symmetric."""
    scaled = v * (1 << bits)
    return int(scaled + 0.5) if scaled >= 0 else -int(-scaled + 0.5)


def arg_to_q(arg: int, shift: int) -> int:
    """`ndsR2AnimArgToQ` (nds_anim_fixed.h:166)."""
    if shift >= 0:
        return arg << shift
    mag = -arg if arg < 0 else arg
    mag = (mag + (1 << (-shift - 1))) >> -shift
    return -mag if arg < 0 else mag


def target_value_q(arg: int, bit: int, is_rate: bool) -> int:
    """`ndsR2AnimTargetValue(arg, bit + JointStart, is_rate, q=1)`, verbatim."""
    tid = TRACK_ID[bit] + (4 if is_rate else 0)
    if tid in (3, 7):
        return f32_to_fixed(arg * TRAI_SCALE, AQ_VF)
    return arg_to_q(arg, AQ_VF - FRAC_SHIFT[tid])


def runtime_shift_q(arg: int, bit: int, is_rate: bool) -> int:
    """What a dense runtime does instead: one shift, from a per-track table.

    Layer C compares this against `target_value_q` over the corpus alphabet.
    TraI keeps the shipped float expression -- it is one track of ten and there
    is no exact shift for 1/16384 - 3e-12.
    """
    tid = TRACK_ID[bit] + (4 if is_rate else 0)
    if tid in (3, 7):
        return f32_to_fixed(arg * TRAI_SCALE, AQ_VF)
    sh = AQ_VF - FRAC_SHIFT[tid]
    if sh >= 0:
        return arg << sh
    mag = -arg if arg < 0 else arg
    mag = (mag + (1 << (-sh - 1))) >> -sh
    return -mag if arg < 0 else mag


# --------------------------------------------------------------------------- #
# Corpus reading.

def read_clip(path: pathlib.Path):
    f = probe.load(path.read_bytes())
    if f is None or f["file_id"] in probe.AOBJ32_IDS:
        return None
    fx = probe.fixups(f)
    if not fx:
        return None
    table = probe.normalize(f, fx)
    offs = sorted(set(fx.values()))
    return {"asset_id": f["file_id"], "name": path.name,
            "scripts": [probe.decode_script(f["data"], f["size"], t) for t in offs],
            "src_bytes": f["size"] - table, "table_bytes": table,
            "file_bytes": f["size"]}


def corpus(exclude=frozenset()):
    out, skipped = [], []
    for path in sorted(p for p in BANK.iterdir()
                       if p.name.startswith(("FTMarioAnim", "FTFoxAnim"))):
        clip = read_clip(path)
        if clip is None:
            continue
        (skipped if clip["asset_id"] in exclude else out).append(
            clip["asset_id"] if clip["asset_id"] in exclude else clip)
    return out, skipped


# --------------------------------------------------------------------------- #
# The row form, as a host-side tuple: this is the ONLY description of a row and
# both the emitter and the reader go through it, so they cannot drift.

def row_of_command(c):
    """(kind, mask, frames, words, is_block) for one decoded command."""
    op = c["op"]
    if op in UNSUPPORTED_OPS:
        raise ValueError("opcode %d (%s) is not representable as a track row"
                         % (op, UNSUPPORTED_OPS[op]))
    kind = OP_TO_KIND[op]
    per = WORDS_PER_TRACK.get(kind, 0)
    mask, words = 0, []
    for bit, w in c["targets"]:
        mask |= 1 << bit
        words.extend(w[:per])
    return (kind, mask, c["payload"], tuple(words), op in BLOCK_OPS)


def encode_script(cmds):
    """One joint script -> (row bytes, union track mask).

    `cmds` is `decode_script` output: commands in EXECUTION order, ending at an
    `End` or at a synthetic `cyclic` marker naming the pc the run returns to.
    The marker is dropped -- the real `Loop` that caused it already carries the
    jump -- and every jump is a byte offset RELATIVE to its own row, so the
    stream is position independent and needs no load-time fixups.
    """
    plan, pc_to_off, off = [], {}, 0
    for c in cmds:
        if c.get("cyclic"):
            break
        kind, mask, frames, words, is_block = row_of_command(c)
        pc_to_off[c["pc"]] = off
        n = 1 + (1 if frames is not None else 0) + \
            (1 if kind == K_LOOP else 0) + len(words)
        plan.append((c, kind, mask, frames, words, is_block, off))
        off += 2 * n

    blob = bytearray()
    union = 0
    for c, kind, mask, frames, words, is_block, base in plan:
        hdr = kind | (mask << HDR_MASK)
        if frames is not None:
            hdr |= 1 << HDR_FRAMES
        if is_block:
            hdr |= 1 << HDR_BLOCK
        blob += struct.pack("<H", hdr)
        if frames is not None:
            blob += struct.pack("<H", frames)
        if kind == K_LOOP:
            tgt = pc_to_off.get(c["jump"])
            if tgt is None:
                raise ValueError("Loop at 0x%x jumps to unvisited pc 0x%x"
                                 % (c["pc"], c["jump"]))
            blob += struct.pack("<h", tgt - base)
        for w in words:
            blob += struct.pack("<h", w)
        union |= mask
    return bytes(blob), union


def decode_row(rows, off):
    """(kind, mask, frames, words, is_block, jump_abs, next_off) out of a pack."""
    hdr = struct.unpack_from("<H", rows, off)[0]
    kind = hdr & 0xF
    mask = (hdr >> HDR_MASK) & 0x3FF
    p = off + 2
    frames = None
    if (hdr >> HDR_FRAMES) & 1:
        frames = struct.unpack_from("<H", rows, p)[0]
        p += 2
    jump = None
    if kind == K_LOOP:
        jump = off + struct.unpack_from("<h", rows, p)[0]
        p += 2
    per = WORDS_PER_TRACK.get(kind, 0)
    words = []
    if per:
        n = bin(mask).count("1") * per
        words = list(struct.unpack_from("<%dh" % n, rows, p))
        p += 2 * n
    return (kind, mask, frames, tuple(words), bool((hdr >> HDR_BLOCK) & 1),
            jump, p)


CLIP_DIR = struct.Struct("<HHII")       # asset_id, script_count, tab_off, rows_bytes
# 6 bytes, not 8: `row_bytes` was carried for the corpus hash and is derivable by
# decoding, so paying 2 B x 4,901 scripts for it is 9,802 B of pure directory.
SCRIPT_ENT = struct.Struct("<IH")       # row_off, union_mask
PACK_HDR = struct.Struct("<4sHHIII")


def build_pack(clips, dedup=True):
    rows, seen = bytearray(), {}
    shared = shared_bytes = 0
    tab, dirs = [], []
    for clip in clips:
        first, total = len(tab), 0
        for cmds in clip["scripts"]:
            blob, union = encode_script(cmds)
            if dedup and blob in seen:
                off = seen[blob]
                shared += 1
                shared_bytes += len(blob)
            else:
                off = len(rows)
                rows += blob
                if dedup:
                    seen[blob] = off
            tab.append(SCRIPT_ENT.pack(off, union))
            total += len(blob)
        dirs.append(CLIP_DIR.pack(clip["asset_id"], len(clip["scripts"]),
                                  first * SCRIPT_ENT.size, total))
    d, t = b"".join(dirs), b"".join(tab)
    head = PACK_HDR.pack(MAGIC, VERSION, len(dirs), len(d), len(t), len(rows))
    return head + d + t + bytes(rows), {
        "header": len(head), "clip_dir": len(d), "script_tab": len(t),
        "rows": len(rows), "total": len(head) + len(d) + len(t) + len(rows),
        "shared_scripts": shared, "shared_bytes": shared_bytes}


def pack_reader(blob):
    magic, version, nclip, nd, nt, nr = PACK_HDR.unpack_from(blob, 0)
    if magic != MAGIC or version != VERSION:
        raise ValueError("not an FTTP v%d pack" % VERSION)
    o = PACK_HDR.size
    dirs, o = blob[o:o + nd], o + nd
    tab, o = blob[o:o + nt], o + nt
    rows = blob[o:o + nr]
    index = {}
    for i in range(nclip):
        aid, n, tab_off, _b = CLIP_DIR.unpack_from(dirs, i * CLIP_DIR.size)
        index[aid] = [SCRIPT_ENT.unpack_from(tab, tab_off + j * SCRIPT_ENT.size)
                      for j in range(n)]
    return index, rows


# --------------------------------------------------------------------------- #
# Layer B: the candidate arm. Reads ONLY the pack.

class Dense:
    """The runtime state a converted joint owns: ten typed tracks, flat.

    That flatness IS the representation change -- no `next`, no `track`, no
    `interpolate`, no 36-byte node reached by pointer. The float shadow exists
    so layer B can compare against the reference in the reference's own domain;
    the Q words are what a DS runtime would hold, and layer C checks them.
    """
    __slots__ = ("kind", "vb", "vt", "rb", "rt", "length", "linv", "q")

    def __init__(self):
        n = 10
        self.kind = [model.KIND_NONE] * n
        self.vb = [0.0] * n
        self.vt = [0.0] * n
        self.rb = [0.0] * n
        self.rt = [0.0] * n
        self.length = [0.0] * n
        self.linv = [1.0] * n
        self.q = [(0, 0, 0, 0) for _ in range(n)]     # vb, vt, rb, rt in Q

    def snapshot(self):
        return tuple((self.kind[i], self.vb[i], self.vt[i], self.rb[i],
                      self.rt[i], self.length[i], self.linv[i], None)
                     for i in range(10))


def replay(rows, start, limit, anim_speed=1.0, break_chain=False):
    d = Dense()
    states, callbacks, waits = [], [], []
    anim_wait, anim_frame = 0.0, 0.0
    off, executed, stopped = start, 0, False
    while executed < limit:
        kind, mask, frames, words, is_block, jump, nxt = decode_row(rows, off)
        executed += 1
        if kind == K_END:
            anim_frame = anim_wait
            callbacks.append(-1)
            stopped = True
            break
        if kind == K_LOOP:
            anim_frame = -anim_wait
            callbacks.append(-2)
            off = jump
            continue
        seg = -anim_wait - anim_speed
        per = WORDS_PER_TRACK.get(kind, 0)
        wi, m = 0, mask
        for bit in range(10):
            if m == 0:
                break
            if m & 1:
                v = words[wi] if per else 0
                r = words[wi + 1] if per == 2 else 0
                wi += per
                _apply(d, bit, kind, frames, v, r, seg, break_chain)
            m >>= 1
        if is_block or kind == K_BLOCK:
            # A `*Block` command with the toggle clear carries no payload word
            # and advances the clock by zero -- `NDS_R2_FTANIM_PAYLOAD()` sets
            # `payload_u = 0` in that case, so the add still happens.
            anim_wait += (frames or 0)
        states.append(d.snapshot())
        waits.append(anim_wait)
        off = nxt
    return {"states": states, "callbacks": callbacks, "waits": waits,
            "stopped": stopped, "anim_frame": anim_frame}


def _apply(d, bit, kind, frames, v, r, seg, break_chain):
    payload = float(frames or 0)
    if kind == K_RATE:
        d.rt[bit] = float(v)
        d.q[bit] = (d.q[bit][0], d.q[bit][1], d.q[bit][2],
                    target_value_q(v, bit, True))
        return
    if kind == K_ADDLEN:
        d.length[bit] += payload
        return
    qvb, qvt, qrb, qrt = d.q[bit]
    if not break_chain:
        d.vb[bit] = d.vt[bit]
        qvb = qvt
    d.vt[bit] = float(v)
    qvt = target_value_q(v, bit, False)
    if kind == K_STEP:
        d.kind[bit] = model.KIND_STEP
        d.linv[bit] = payload
        d.length[bit] = seg
        d.rt[bit] = 0.0
        qrt = 0
    elif kind == K_LINEAR:
        d.kind[bit] = model.KIND_LINEAR
        if payload:
            d.rb[bit] = (d.vt[bit] - d.vb[bit]) / payload
            dd = (qvt - qvb) << (AQ_RF - AQ_VF)
            h = frames >> 1
            qrb = -((-dd + h) // frames) if dd < 0 else ((dd + h) // frames)
        d.length[bit] = seg
        d.rt[bit] = 0.0
        qrt = 0
    else:                                            # K_CUBIC0 / K_CUBIC2
        d.rb[bit] = d.rt[bit]
        qrb = qrt
        d.rt[bit] = float(r) if kind == K_CUBIC2 else 0.0
        qrt = target_value_q(r, bit, True) if kind == K_CUBIC2 else 0
        d.kind[bit] = model.KIND_CUBIC
        if payload:
            d.linv[bit] = 1.0 / payload
        d.length[bit] = seg
    d.q[bit] = (qvb, qvt, qrb, qrt)


# --------------------------------------------------------------------------- #
# Verification.

def unroll(cmds, passes):
    """Execution-order command list extended over `passes` loop iterations."""
    body = [c for c in cmds if not c.get("cyclic")]
    if not body:
        return []
    by_pc = {c["pc"]: i for i, c in enumerate(body)}
    out, i, taken, guard = [], 0, 0, len(body) * (passes + 1) + 8
    while guard:
        guard -= 1
        c = body[i]
        out.append(c)
        if c["op"] == 0:
            break
        if c["op"] == 13:
            taken += 1
            if taken >= passes or c["jump"] not in by_pc:
                break
            i = by_pc[c["jump"]]
            continue
        i += 1
        if i >= len(body):
            break
    return out


def verify(clips, blob, passes=3, break_chain=False):
    index, rows = pack_reader(blob)
    mm = collections.Counter()
    ex = []
    n = collections.Counter()
    h = hashlib.sha256()

    for clip in clips:
        ents = index.get(clip["asset_id"])
        if ents is None or len(ents) != len(clip["scripts"]):
            mm["clip directory"] += 1
            continue
        for si, cmds in enumerate(clip["scripts"]):
            n["scripts"] += 1
            row_off, union = ents[si]
            h.update(struct.pack("<IH", clip["asset_id"], si))

            # ---- layer A: emitter fidelity -------------------------------- #
            want, off, bad = [], row_off, False
            for c in cmds:
                if c.get("cyclic"):
                    break
                want.append(row_of_command(c))
            got, off = [], row_off
            for _ in range(len(want)):
                kind, mask, frames, words, is_block, jump, nxt = decode_row(rows, off)
                got.append((kind, mask, frames, words, is_block))
                n["rows"] += 1
                off = nxt
            h.update(rows[row_off:off])
            if want != got:
                mm["emitted rows"] += 1
                _note(ex, clip, si, "row %d differs" % _first_diff(want, got))
                bad = True
            u = 0
            for kind, mask, _f, _w, _b in want:
                u |= mask
            if u != union:
                mm["union mask"] += 1
                bad = True
            if bad:
                continue

            # ---- layer B: semantic equivalence ---------------------------- #
            ref_cmds = unroll(cmds, passes)
            ref = model.run_commands(ref_cmds)
            cand = replay(rows, row_off, len(ref_cmds), break_chain=break_chain)
            n["commands"] += len(ref_cmds)
            n["states"] += len(ref.states)
            n["callbacks"] += len(ref.callbacks)

            if len(ref.states) != len(cand["states"]):
                mm["state count"] += 1
                _note(ex, clip, si, "states %d vs %d"
                      % (len(ref.states), len(cand["states"])))
                continue
            for k, ((_pc, op, rs), cs) in enumerate(zip(ref.states, cand["states"])):
                rs = tuple(rs)       # run_commands records a list; cs is a tuple
                if rs != cs:
                    mm["executed semantics"] += 1
                    _note(ex, clip, si, "cmd %d %s track %s"
                          % (k, op, _first_diff(rs, cs)))
                    break
            if [t for _p, t in ref.callbacks] != cand["callbacks"]:
                mm["callback order"] += 1
                _note(ex, clip, si, "callbacks %s vs %s"
                      % ([t for _p, t in ref.callbacks], cand["callbacks"]))
            if ref.waits != cand["waits"]:
                mm["frame boundaries"] += 1
                _note(ex, clip, si, "wait %d" % _first_diff(ref.waits, cand["waits"]))
            if bool(ref.stopped) != bool(cand["stopped"]):
                mm["end behaviour"] += 1
            if ref.stopped and ref.anim_frame != cand["anim_frame"]:
                mm["end anim_frame"] += 1

    return {"mismatches": dict(mm), "total": sum(mm.values()),
            "counts": dict(n), "corpus": h.hexdigest(), "examples": ex[:8]}


def _first_diff(a, b):
    for i in range(max(len(a), len(b))):
        if i >= len(a) or i >= len(b) or a[i] != b[i]:
            return i
    return -1


def _note(ex, clip, si, why):
    if len(ex) < 64:
        ex.append("%s script %d: %s" % (clip["name"], si, why))


def layer_c(clips):
    """Every (track, value/rate, authored word) the corpus contains, checked
    against the shipped `ndsR2AnimTargetValue(..., q=1)` expression."""
    alphabet = set()
    for clip in clips:
        for cmds in clip["scripts"]:
            for c in cmds:
                if c.get("cyclic"):
                    continue
                kind = OP_TO_KIND.get(c["op"])
                per = WORDS_PER_TRACK.get(kind, 0)
                if not per:
                    continue
                for bit, w in c["targets"]:
                    if kind == K_RATE:
                        alphabet.add((bit, True, w[0]))
                    else:
                        alphabet.add((bit, False, w[0]))
                        if per == 2:
                            alphabet.add((bit, True, w[1]))
    bad = [t for t in alphabet
           if runtime_shift_q(t[2], t[0], t[1]) != target_value_q(t[2], t[0], t[1])]
    widths = collections.Counter()
    for bit, is_rate, w in alphabet:
        q = target_value_q(w, bit, is_rate)
        widths[max(1, q.bit_length() + 1)] += 1
    return {"alphabet": len(alphabet), "mismatches": len(bad),
            "max_q_bits": max(widths) if widths else 0,
            "over_s16": sum(v for k, v in widths.items() if k > 16)}


def falsifiers(clips, blob, passes):
    out = []
    ba = bytearray(blob)
    _m, _v, _nc, nd, nt, nr = PACK_HDR.unpack_from(blob, 0)
    rows_at = PACK_HDR.size + nd + nt
    i = rows_at + nr // 2
    ba[i] ^= 0x01
    out.append(("one byte flipped in the row stream",
                verify(clips, bytes(ba), passes)["total"]))
    ba[i] ^= 0x01
    tab_at = PACK_HDR.size + nd
    off, union = SCRIPT_ENT.unpack_from(ba, tab_at)
    SCRIPT_ENT.pack_into(ba, tab_at, off + 2, union)
    out.append(("one script's row offset shifted by one word",
                verify(clips, bytes(ba), passes)["total"]))
    SCRIPT_ENT.pack_into(ba, tab_at, off, union)
    out.append(("candidate drops the value_base <- value_target chain",
                verify(clips, blob, passes, break_chain=True)["total"]))
    return out


# --------------------------------------------------------------------------- #

def scan(clips):
    ops, tracks, union_pop = collections.Counter(), collections.Counter(), collections.Counter()
    max_frames = max_scripts = 0
    cmds_n = segs = words = 0
    for clip in clips:
        max_scripts = max(max_scripts, len(clip["scripts"]))
        for cs in clip["scripts"]:
            u = 0
            for c in cs:
                if c.get("cyclic"):
                    continue
                cmds_n += 1
                ops[c["op"]] += 1
                if c["payload"]:
                    max_frames = max(max_frames, c["payload"])
                kind = OP_TO_KIND.get(c["op"])
                per = WORDS_PER_TRACK.get(kind, 0)
                for bit, w in c["targets"]:
                    tracks[bit] += 1
                    segs += 1
                    words += per
                    u |= 1 << bit
            union_pop[bin(u).count("1")] += 1
    return {"ops": dict(sorted(ops.items())),
            "tracks": {TRACK_NAME[k]: v for k, v in sorted(tracks.items())},
            "max_frames": max_frames, "max_scripts_per_clip": max_scripts,
            "commands": cmds_n, "track_segments": segs, "target_words": words,
            "union_popcount": dict(sorted(union_pop.items()))}


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--items-off", action="store_true")
    ap.add_argument("--fighter", choices=("mario", "fox"))
    ap.add_argument("--no-dedup", action="store_true")
    ap.add_argument("--passes", type=int, default=3)
    ap.add_argument("--verify", action="store_true")
    ap.add_argument("--out")
    ap.add_argument("--json")
    args = ap.parse_args()

    if not BANK.is_dir():
        print("SKIP: %s absent -- run scripts/fetch-battleship-reference.ps1" % BANK)
        return 0

    clips, skipped = corpus(ITEM_IDS if args.items_off else frozenset())
    if args.fighter:
        pre = "FTMarioAnim" if args.fighter == "mario" else "FTFoxAnim"
        clips = [c for c in clips if c["name"].startswith(pre)]

    facts = scan(clips)
    for op in UNSUPPORTED_OPS:
        if facts["ops"].get(op):
            print("REFUSED: opcode %d (%s) occurs %d times -- not representable "
                  "as a track row" % (op, UNSUPPORTED_OPS[op], facts["ops"][op]))
            return 1

    blob, sizes = build_pack(clips, dedup=not args.no_dedup)
    src = sum(c["src_bytes"] for c in clips)
    nscripts = sum(len(c["scripts"]) for c in clips)
    print("clips %d  scripts %d  commands %d  track-segments %d  target words %d"
          % (len(clips), nscripts, facts["commands"], facts["track_segments"],
             facts["target_words"]))
    print("source script-region bytes %d   (entry tables %d, files %d)"
          % (src, sum(c["table_bytes"] for c in clips),
             sum(c["file_bytes"] for c in clips)))
    for k in ("header", "clip_dir", "script_tab", "rows", "total"):
        print("  %-11s %9d" % (k, sizes[k]))
    print("  shared scripts %d (%d B saved)"
          % (sizes["shared_scripts"], sizes["shared_bytes"]))
    print("  PACK / SOURCE SCRIPT BYTES  %.4fx" % (sizes["total"] / src))
    print("  sha256 %s" % hashlib.sha256(blob).hexdigest())
    print("  max frames %d (fits u8: %s)  max scripts/clip %d"
          % (facts["max_frames"], facts["max_frames"] < 256,
             facts["max_scripts_per_clip"]))
    print("  per-script union popcount %s" % facts["union_popcount"])
    print("  opcodes %s" % facts["ops"])
    print("  track usage %s" % facts["tracks"])

    rc = 0
    result = lc = None
    if args.verify:
        lc = layer_c(clips)
        print("\nLAYER C  quantisation alphabet %d  mismatches %d  "
              "max Q width %d bits  values needing >16 bits %d"
              % (lc["alphabet"], lc["mismatches"], lc["max_q_bits"], lc["over_s16"]))
        if lc["mismatches"]:
            rc = 1
        result = verify(clips, blob, args.passes)
        c = result["counts"]
        print("\nLAYERS A+B  (passes=%d)" % args.passes)
        print("  scripts %d  rows %d  commands %d  states %d  callbacks %d"
              % (c.get("scripts", 0), c.get("rows", 0), c.get("commands", 0),
                 c.get("states", 0), c.get("callbacks", 0)))
        print("  corpus %s" % result["corpus"])
        print("  MISMATCHES %d %s" % (result["total"], result["mismatches"] or ""))
        for e in result["examples"]:
            print("    %s" % e)
        if result["total"]:
            rc = 1
        else:
            for why, cnt in falsifiers(clips, blob, args.passes):
                print("  falsifier: %-52s -> %d mismatches" % (why, cnt))
                if cnt == 0:
                    print("  FAIL: a falsifier did not fail -- the test is blind")
                    rc = 1
        print("\nFTANIM_TRACK_PACK=%s" % ("PASS" if rc == 0 else "FAIL"))
    if args.out:
        pathlib.Path(args.out).write_bytes(blob)
    if args.json:
        pathlib.Path(args.json).write_text(json.dumps(
            {"clips": len(clips), "scripts": nscripts, "sizes": sizes,
             "source_bytes": src, "facts": facts, "layer_c": lc,
             "sha256": hashlib.sha256(blob).hexdigest(), "verify": result},
            indent=1))
    return rc


if __name__ == "__main__":
    sys.exit(main())
