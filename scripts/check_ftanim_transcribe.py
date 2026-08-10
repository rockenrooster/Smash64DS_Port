#!/usr/bin/env python3
"""Prove the port figatree parser is a FAITHFUL transcription of the decomp one.

`src/import/battleship_ftanim.c` reimplements `ftAnimParseDObjFigatree` port-side
so the fighter path stops paying 45 `bl __aeabi_*` per call. The arithmetic
substitutions are each proven exact elsewhere (`check_ftanim_target_exact.py`,
and the reciprocal table is compile-time-folded). What is NOT proven by those is
the transcription itself: ~250 lines of control flow where `AObjAnimAdvance` is
`p++`, appears more than once per expression, and advances CONDITIONALLY on a
toggle bit. Miscount one and the animation stream desynchronises -- which moves
hitboxes, so it must not be left to review by eye.

This checker applies the INVERSE substitutions to the port body and compares it
to the decomp body as a token stream (whitespace- and wrapping-insensitive, since
the port body is reformatted to this repo's width). A slip fails here.

It is deliberately strict: anything it cannot account for is a failure, not a
warning. The only tolerated differences are declared in ALLOWED below.

Usage:
    python scripts/check_ftanim_transcribe.py [--verbose]
"""

import argparse
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PORT = os.path.join(ROOT, "src", "import", "battleship_ftanim.c")
DECOMP = os.path.join(
    ROOT, "decomp", "BattleShip-main", "decomp", "src", "ft", "ftanim.c"
)

EV = "root_dobj->anim_joint.event16"
ADV = "AObjAnimAdvance(%s)" % EV

# Inverse of every substitution the port body makes. Applied to the PORT text to
# recover the decomp text. Order matters: longest/most specific first.
INVERSE = [
    # The three transcription macros expand back to their decomp originals.
    ("NDS_R2_FTANIM_PAYLOAD();",
     "payload = (%s->command.toggle) ? %s->u : 0.0F;" % (ADV, ADV)),
    ("NDS_R2_FTANIM_TARGET(0)",
     "ftAnimGetTargetValue(%s->s, i + nGCAnimTrackJointStart, 0)" % ADV),
    ("NDS_R2_FTANIM_TARGET(1)",
     "ftAnimGetTargetValue(%s->s, i + nGCAnimTrackJointStart, 1)" % ADV),
    ("NDS_R2_FTANIM_ENSURE();",
     "if (track_aobjs[i] == NULL) { track_aobjs[i] = "
     "gcAddAObjForDObj(root_dobj, i + nGCAnimTrackJointStart); }"),
    # The exact-conversion helper wrapping a raw u16 read.
    ("ndsR2U16ToF32(%s->u)" % ADV, "%s->u" % ADV),
    # The comparison predicates.
    ("NDS_FCMP_NE_C(root_dobj->anim_wait, AOBJ_ANIM_NULL)",
     "root_dobj->anim_wait != AOBJ_ANIM_NULL"),
    ("NDS_FCMP_EQ_C(root_dobj->anim_wait, AOBJ_ANIM_CHANGED)",
     "root_dobj->anim_wait == AOBJ_ANIM_CHANGED"),
    ("NDS_FCMP_GT0(root_dobj->anim_wait)", "root_dobj->anim_wait > 0.0F"),
    ("NDS_FCMP_LE0(root_dobj->anim_wait)", "root_dobj->anim_wait <= 0.0F"),
    # The reciprocal table, and the integer form of the payload-not-zero guard.
    ("ndsR2Recip(payload_u)", "1.0F / payload"),
    ("payload_u != 0u", "payload != 0.0F"),
    # Cast added only to silence a signed/unsigned comparison warning.
    ("(s32)ARRAY_COUNT(track_aobjs)", "ARRAY_COUNT(track_aobjs)"),
]

# Tokens present in exactly one side on purpose. Each entry is a token sequence
# to DELETE from the named side before comparing, with the reason it exists.
#
# Keep these SPECIFIC. `drop` removes every occurrence, so a short generic
# sequence would silently delete legitimate code -- whitelisting a bare
# "break ;" here would have removed all fourteen of the parser's real breaks and
# turned this checker into a rubber stamp. The `#else` arms it was meant to
# excuse are handled properly by `preprocess` instead.
ALLOWED = [
    ("port", "gNdsR2FtAnimParseCalls ++ ;",
     "call counter, so the measuring run can prove the parser was reached"),
    ("port", "u32 payload_u ;",
     "integer payload kept alongside the f32 for the table index"),
]


def body_of(text: str, name: str) -> str:
    """Return the brace-balanced body of `void <name>(DObj *root_dobj)`."""
    m = re.search(r"void\s+%s\s*\(\s*DObj\s*\*\s*root_dobj\s*\)\s*\{" % name,
                  text)
    if not m:
        sys.exit("FAIL: %s not found." % name)
    i = m.end() - 1
    depth = 0
    for j in range(i, len(text)):
        if text[j] == "{":
            depth += 1
        elif text[j] == "}":
            depth -= 1
            if depth == 0:
                return text[i + 1:j]
    sys.exit("FAIL: unbalanced braces in %s." % name)


def strip_comments(s: str) -> str:
    s = re.sub(r"/\*.*?\*/", " ", s, flags=re.S)
    return re.sub(r"//[^\n]*", " ", s)


def preprocess(s: str) -> str:
    """Resolve `#if defined(SSB64_TARGET_NDS)` the way this build resolves it.

    The decomp file on disk already carries the runaway patch, whose guards are
    real preprocessor conditionals. Comparing the raw text would drag the DEAD
    `#else` arms into the token stream -- which is how this checker first
    reported a bogus divergence on a `break;` that our build never compiles.
    """
    out, i = [], 0
    pat = re.compile(r"^[ \t]*#[ \t]*(if|ifdef|else|endif)\b[^\n]*\n",
                     re.M)
    depth_taken = []
    for m in pat.finditer(s):
        kind = m.group(1)
        out.append((m.start(), m.end(), kind, m.group(0)))
    if not out:
        return s
    result, pos, skipping = [], 0, 0
    for start, end, kind, line in out:
        if skipping == 0:
            result.append(s[pos:start])
        if kind in ("if", "ifdef"):
            taken = "SSB64_TARGET_NDS" in line
            depth_taken.append(taken)
            if not taken and skipping == 0:
                skipping = len(depth_taken)
        elif kind == "else":
            if depth_taken:
                was = depth_taken[-1]
                depth_taken[-1] = not was
                if was and skipping == 0:
                    skipping = len(depth_taken)
                elif not was and skipping == len(depth_taken):
                    skipping = 0
        elif kind == "endif":
            if skipping == len(depth_taken):
                skipping = 0
            if depth_taken:
                depth_taken.pop()
        pos = end
    if skipping == 0:
        result.append(s[pos:])
    return "".join(result)


def flatten(s: str) -> str:
    """Collapse every whitespace run to one space.

    Both bodies are wrapped to their own file's width, so the inverse rules --
    written on one line -- cannot match the shipped text otherwise. Safe because
    the comparison that decides the verdict is on tokens, not characters.

    Padding just inside parentheses goes too: a call wrapped after its open
    paren is the normal shape here, and a rule written on one line would
    otherwise miss it and report itself uncovered.
    """
    s = re.sub(r"\s+", " ", s)
    s = re.sub(r"\(\s+", "(", s)
    return re.sub(r"\s+\)", ")", s)


def tokens(s: str) -> list:
    return re.findall(r"[A-Za-z_][A-Za-z0-9_]*|0[xX][0-9a-fA-F]+|"
                      r"\d+\.\d*[fFuU]?|\.\d+[fFuU]?|\d+[uUfFlL]*|"
                      r"->|\+\+|--|<<|>>|<=|>=|==|!=|&&|\|\||[^\s]", s)


def drop(seq: list, sub: list) -> list:
    """Remove every occurrence of the token subsequence `sub` from `seq`."""
    if not sub:
        return seq
    out, i, n = [], 0, len(sub)
    while i < len(seq):
        if seq[i:i + n] == sub:
            i += n
        else:
            out.append(seq[i])
            i += 1
    return out


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--verbose", action="store_true")
    args = ap.parse_args()

    port_raw = body_of(open(PORT, encoding="utf-8", errors="replace").read(),
                       "ndsR2FtAnimParseDObjFigatree")
    dec_raw = body_of(open(DECOMP, encoding="utf-8", errors="replace").read(),
                      "ftAnimParseDObjFigatree")

    port = flatten(strip_comments(port_raw))
    dec = flatten(strip_comments(preprocess(dec_raw)))

    # Confirm every inverse substitution actually fires. A rule that matches
    # nothing means the port body drifted and this checker silently stopped
    # covering it -- exactly the failure mode a checker must not have.
    unused = []
    for src, dst in INVERSE:
        if src not in port:
            unused.append(src)
        port = port.replace(src, dst)
    if unused:
        print("FAIL: %d inverse rule(s) matched nothing, so this checker is no "
              "longer covering the shipped body:" % len(unused))
        for u in unused:
            print("    %s" % u)
        return 1

    pt, dt = tokens(port), tokens(dec)
    for side, text, _why in ALLOWED:
        sub = tokens(text)
        if side == "port":
            pt = drop(pt, sub)
        else:
            dt = drop(dt, sub)

    if pt == dt:
        print("ftanim transcription fidelity -- inverse-substituted port body vs "
              "decomp body")
        print("GREEN: %d tokens, identical. Every AObjAnimAdvance, branch and "
              "assignment\n       matches; the only differences are the %d "
              "declared ones." % (len(pt), len(ALLOWED)))
        return 0

    # Report the first divergence with context, which is what a slip looks like.
    k = 0
    while k < min(len(pt), len(dt)) and pt[k] == dt[k]:
        k += 1
    lo = max(0, k - 12)
    print("RED: token streams diverge at index %d (port %d tokens, decomp %d)."
          % (k, len(pt), len(dt)))
    print("  common prefix ...: %s" % " ".join(pt[lo:k]))
    print("  port     next 14 : %s" % " ".join(pt[k:k + 14]))
    print("  decomp   next 14 : %s" % " ".join(dt[k:k + 14]))
    if args.verbose:
        print("\n--- inverse-substituted port body ---\n%s" % port)
    return 1


if __name__ == "__main__":
    sys.exit(main())
