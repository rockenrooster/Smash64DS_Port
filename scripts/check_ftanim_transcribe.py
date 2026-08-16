#!/usr/bin/env python3
"""Guard the two figatree-parser invariants a restructured port can still break.

`src/import/battleship_ftanim.c` reimplements `ftAnimParseDObjFigatree`
port-side. It began (`514fad238da`) as a line-for-line transcription, and this
checker began as a token-stream equality proof against the decomp body.

THAT PROOF IS NO LONGER EXPRESSIBLE, and pretending otherwise is why this file
sat RED and unwired for weeks. `69ce92e279f` (Requirement 4) restructured the
port body: the track table is built lazily through `NDS_R2_FTANIM_TRACKS()`
instead of unconditionally before the loop, every AObj slot carries a second
fixed-point representation behind `q`, the tail walk became
`ndsR2AnimAdvanceTail`, loop-invariants were hoisted into `len_new`, and a
runaway guard was added. None of that is a substitution, so no inverse rule
table can recover the decomp text from it.

What survives the restructure is what actually matters, so that is what is
checked here -- per opcode arm, because the two bodies still have the SAME
sixteen case labels in the same order:

  PROOF 1 -- ADVANCE SEQUENCE. `AObjAnimAdvance` is `p++` over the event
  stream. It appears more than once per expression and advances CONDITIONALLY
  on a toggle bit, so miscounting one desynchronises the animation stream,
  which moves hitboxes. Each arm's ordered sequence of advances (and the field
  each one reads) must equal the decomp arm's, exactly.

  PROOF 2 -- HOIST LIVENESS. `69ce92e279f` hoisted `-anim_wait - anim_speed`
  out of the per-track loop into `len_new` in all four write arms but added the
  ASSIGNMENT to only one; the other three read a function-local reset to `0.0F`
  every call. Opcodes 4+5 are 82.7% of items-off write commands and are in an
  arm that never assigned, so every new segment started at phase 0 and shipped
  that way. A token-stream diff could never have seen it -- every token was
  present, only in the wrong arm. So: any local the switch assigns at arm level
  must be assigned before it is read, in EVERY arm that reads it.

Proof 2 is checked against the port alone; it is a dataflow property, not a
comparison. Both are deliberately strict: anything unaccounted for is a
failure, not a warning.

Usage:
    python scripts/check_ftanim_transcribe.py [--verbose]
"""

import argparse
import collections
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

# The port hides advances inside transcription macros so a slip cannot differ
# between call sites. Expand them here -- textually, to the decomp's own shape --
# before counting. Every rule must fire; one that matches nothing means the port
# drifted and this checker silently stopped covering it.
MACROS = [
    ("NDS_R2_FTANIM_PAYLOAD();",
     "payload = (%s->command.toggle) ? %s->u : 0.0F;" % (ADV, ADV)),
    ("NDS_R2_FTANIM_TARGET(0)",
     "ftAnimGetTargetValue(%s->s, i + nGCAnimTrackJointStart, 0)" % ADV),
    ("NDS_R2_FTANIM_TARGET(1)",
     "ftAnimGetTargetValue(%s->s, i + nGCAnimTrackJointStart, 1)" % ADV),
]

# Port macros that provably carry no `AObjAnimAdvance`. Anything else still
# spelled `NDS_R2_FTANIM_*` after expansion fails: a new macro could be hiding
# an advance from proof 1, which is the one thing this checker must never miss.
NO_ADVANCE_MACROS = ("NDS_R2_FTANIM_ENSURE", "NDS_R2_FTANIM_TRACKS")

# Locals the port body may read in an arm without assigning it there, with the
# reason. Keep this EMPTY unless a real one appears -- an entry here is a hole
# in proof 2, and defect 1 is what fits through such a hole.
HOIST_EXEMPT = {}


def body_of(text: str, name: str) -> str:
    """Return the brace-balanced body of `void <name>(DObj *root_dobj)`."""
    m = re.search(r"void\s+%s\s*\(\s*DObj\s*\*\s*root_dobj\s*\)\s*\{" % name,
                  text)
    if not m:
        sys.exit("FAIL: %s not found." % name)
    return balanced(text, m.end() - 1)


def balanced(text: str, open_brace: int) -> str:
    depth = 0
    for j in range(open_brace, len(text)):
        if text[j] == "{":
            depth += 1
        elif text[j] == "}":
            depth -= 1
            if depth == 0:
                return text[open_brace + 1:j]
    sys.exit("FAIL: unbalanced braces at offset %d." % open_brace)


def strip_comments(s: str) -> str:
    s = re.sub(r"/\*.*?\*/", " ", s, flags=re.S)
    return re.sub(r"//[^\n]*", " ", s)


def flatten(s: str) -> str:
    """Collapse whitespace so rules written on one line match wrapped text."""
    s = re.sub(r"\s+", " ", s)
    s = re.sub(r"\(\s+", "(", s)
    return re.sub(r"\s+\)", ")", s)


def tokens(s: str) -> list:
    return re.findall(r"[A-Za-z_][A-Za-z0-9_]*|0[xX][0-9a-fA-F]+|"
                      r"\d+\.\d*[fFuU]?|\.\d+[fFuU]?|\d+[uUfFlL]*|"
                      r"->|\+\+|--|<<|>>|<=|>=|==|!=|&&|\|\||[^\s]", s)


def switch_arms(body: str, who: str) -> "collections.OrderedDict":
    """Split `switch (command_kind) { ... }` into label-tuple -> arm text."""
    m = re.search(r"switch\s*\(\s*command_kind\s*\)\s*\{", body)
    if not m:
        sys.exit("FAIL: %s has no `switch (command_kind)`." % who)
    inner = balanced(body, m.end() - 1)

    label = re.compile(r"(?:case\s+([A-Za-z_][A-Za-z0-9_]*)\s*:|(default)\s*:)")
    hits = list(label.finditer(inner))
    if not hits:
        sys.exit("FAIL: %s switch has no case labels." % who)

    arms, labels = collections.OrderedDict(), []
    for k, h in enumerate(hits):
        labels.append(h.group(1) or h.group(2))
        end = hits[k + 1].start() if k + 1 < len(hits) else len(inner)
        text = inner[h.end():end]
        # Consecutive labels (fallthrough) share one arm; only the last carries
        # the body. A label followed immediately by another label is empty.
        if text.strip():
            arms[tuple(labels)] = text
            labels = []
    if labels:
        sys.exit("FAIL: %s switch ends on a label with no body." % who)
    return arms


def advances(arm: str) -> list:
    """Ordered `AObjAnimAdvance` reads in one arm, each with its field."""
    out = []
    for m in re.finditer(re.escape(ADV) + r"\s*(->\s*[A-Za-z_][A-Za-z0-9_.]*)?",
                         arm):
        field = m.group(1)
        out.append(re.sub(r"\s+", "", field) if field else "<bare>")
    return out


def arm_hoist_violations(arm: str, names: set) -> list:
    """Names read before any assignment in this arm."""
    tk = tokens(arm)
    assigned, bad = set(), []
    for i, t in enumerate(tk):
        if t not in names:
            continue
        if i + 1 < len(tk) and tk[i + 1] == "=":
            assigned.add(t)
        elif t not in assigned:
            bad.append(t)
    return sorted(set(bad))


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--verbose", action="store_true")
    args = ap.parse_args()

    if not os.path.isfile(DECOMP):
        print("SKIP: %s absent -- run scripts/fetch-battleship-reference.ps1"
              % DECOMP)
        return 0

    port_body = body_of(open(PORT, encoding="utf-8", errors="replace").read(),
                        "ndsR2FtAnimParseDObjFigatree")
    dec_body = body_of(open(DECOMP, encoding="utf-8", errors="replace").read(),
                       "ftAnimParseDObjFigatree")

    # The decomp is pristine upstream since 2026-08-15 (all ten decomp patches
    # retired), so its body carries no conditionals. If one reappears, the token
    # stream below would silently include a dead arm -- fail instead.
    if re.search(r"^[ \t]*#[ \t]*(if|ifdef|ifndef|else|elif|endif)\b",
                 dec_body, re.M):
        print("FAIL: the decomp parser body contains preprocessor "
              "conditionals.\n      `decomp/` is meant to be byte-pristine "
              "upstream; run\n      `scripts/fetch-battleship-reference.ps1 "
              "-VerifyOnly`.")
        return 1

    port = flatten(strip_comments(port_body))
    dec = flatten(strip_comments(dec_body))

    unused = [src for src, _ in MACROS if src not in port]
    if unused:
        print("FAIL: %d macro expansion(s) matched nothing, so this checker is "
              "no longer covering the shipped body:" % len(unused))
        for u in unused:
            print("    %s" % u)
        return 1
    for src, dst in MACROS:
        port = port.replace(src, dst)

    leftover = sorted(set(re.findall(r"NDS_R2_FTANIM_[A-Z_]+", port)))
    unknown = [m for m in leftover if m not in NO_ADVANCE_MACROS]
    if unknown:
        print("FAIL: unexpanded port macro(s) that may hide an "
              "`AObjAnimAdvance`:")
        for m in unknown:
            print("    %s -- expand it in MACROS, or declare it in "
                  "NO_ADVANCE_MACROS after checking its body." % m)
        return 1

    port_arms = switch_arms(port, "port")
    dec_arms = switch_arms(dec, "decomp")

    if list(port_arms) != list(dec_arms):
        print("RED: the two switches no longer have the same arms.")
        print("  port  : %s" % " | ".join("+".join(k) for k in port_arms))
        print("  decomp: %s" % " | ".join("+".join(k) for k in dec_arms))
        return 1

    # ---- proof 1: advance sequence, per arm ----
    total, bad = 0, []
    for key in port_arms:
        pa, da = advances(port_arms[key]), advances(dec_arms[key])
        total += len(da)
        if pa != da:
            bad.append((key, pa, da))
    if bad:
        print("RED: %d arm(s) advance the event stream differently. An "
              "`AObjAnimAdvance` slip\n     desynchronises the animation "
              "stream, which moves hitboxes." % len(bad))
        for key, pa, da in bad:
            print("  case %s" % "+".join(key))
            print("    port   (%d): %s" % (len(pa), ", ".join(pa) or "-"))
            print("    decomp (%d): %s" % (len(da), ", ".join(da) or "-"))
        return 1

    # ---- proof 2: hoist liveness, port only ----
    # A LOCAL assigned at statement level in any arm is a per-event value; every
    # arm that reads it must assign it first. Restricted to declared locals on
    # purpose: a global written in one arm and read in another is ordinary
    # cross-call state, not a hoist, and including those only invents red.
    declared = set(re.findall(
        r"\b(?:const\s+)?(?:u8|u16|u32|s8|s16|s32|f32|f64|sb32|AObj|DObj)\s*"
        r"\*?\s*([A-Za-z_][A-Za-z0-9_]*)\s*(?:\[[^\]]*\])?\s*[;=,]",
        strip_comments(port_body)))
    if not declared:
        print("FAIL: no locals parsed out of the port body; proof 2 would be "
              "vacuous.")
        return 1

    hoisted = set()
    for arm in port_arms.values():
        tk = tokens(arm)
        for i, t in enumerate(tk):
            if (t in declared and i + 1 < len(tk) and tk[i + 1] == "="
                    and (i == 0 or tk[i - 1] in (";", "{", "}"))):
                hoisted.add(t)
    hoisted -= set(HOIST_EXEMPT)

    violations = []
    for key, arm in port_arms.items():
        for name in arm_hoist_violations(arm, hoisted):
            violations.append((key, name))
    if violations:
        print("RED: %d arm/local pair(s) READ a hoisted local this arm never "
              "assigns.\n     That is defect 1's exact shape "
              "(`69ce92e279f`): the value read is the\n     function-local "
              "initialiser, not this event's." % len(violations))
        for key, name in violations:
            print("  case %-40s reads `%s` unassigned" % ("+".join(key), name))
        return 1

    print("ftanim parser fidelity -- per-arm advance sequence + hoist liveness")
    print("GREEN: %d arms, %d AObjAnimAdvance reads, sequence and fields "
          "identical\n       to the decomp arm-for-arm; %d hoisted local(s) "
          "%sassigned before every\n       read in every arm that reads them."
          % (len(port_arms), total, len(hoisted),
             "(%s) " % ", ".join(sorted(hoisted)) if hoisted else ""))
    if args.verbose:
        for key in port_arms:
            print("  %-46s %s" % ("+".join(key),
                                  ", ".join(advances(port_arms[key])) or "-"))
    return 0


if __name__ == "__main__":
    sys.exit(main())
