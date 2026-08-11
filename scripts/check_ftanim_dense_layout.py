#!/usr/bin/env python3
"""Assert the dense-track layout agrees between the emitter and the runtime.

Slice 32. `generate_ftanim_dense_bank.py` writes the bank in Python and
`include/nds/nds_anim_dense.h` reads it in C. They are two halves of one binary
format in two languages, and nothing else makes them agree -- the header's
`_Static_assert`s pin the C side only, and the emitter's `struct.Struct` pins
the Python side only. A disagreement between them compiles, links, boots, and
produces one joint of one animation that is subtly wrong: no crash, no counter,
nothing a screenshot shows.

So this compares them directly: field order, offsets, widths, total stride, and
the three Q constants. It parses the header rather than importing a duplicate
list, because a checker that keeps its own copy of the layout is a third thing
to drift.

Cheap and host-only; it needs neither `decomp/` nor a build.
"""

from __future__ import annotations

import importlib.util
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
HEADER = ROOT / "include" / "nds" / "nds_anim_dense.h"

# struct.Struct codes -> (C type, size). The emitter's format string is read
# field by field and matched against the header's declarations in order.
CODE = {"i": ("s32", 4), "h": ("s16", 2), "H": ("u16", 2), "B": ("u8", 1)}


def header_fields(text):
    """(name, ctype, count) for each member of NDSAnimDenseTrack, in order."""
    body = re.search(r"typedef struct NDSAnimDenseTrack\s*\{(.*?)\}", text,
                     re.S)
    if not body:
        sys.exit("FAIL: NDSAnimDenseTrack not found in %s" % HEADER)
    out = []
    for line in body.group(1).split("\n"):
        line = re.sub(r"/\*.*?\*/", "", line).strip()
        m = re.match(r"^(s32|s16|u16|u8)\s+(\w+)(?:\[(\d+)\])?\s*;$", line)
        if m:
            out.append((m.group(2), m.group(1),
                        int(m.group(3)) if m.group(3) else 1))
    return out


def header_asserts(text):
    """{field: offset} from the _Static_assert offsetof lines, plus the size."""
    offsets = {name: int(off) for name, off in re.findall(
        r"offsetof\(NDSAnimDenseTrack,\s*(\w+)\)\s*==\s*(\d+)", text)}
    size = re.search(r"sizeof\(NDSAnimDenseTrack\)\s*==\s*(\d+)", text)
    return offsets, (int(size.group(1)) if size else None)


def header_constants(text):
    return {m.group(1): int(m.group(2)) for m in re.finditer(
        r"#define\s+(NDS_ANIM_DENSE_\w*_Q)\s+(\d+)", text)}


def main() -> int:
    text = HEADER.read_text(encoding="utf-8", errors="replace")
    fields = header_fields(text)
    offsets, declared_size = header_asserts(text)
    consts = header_constants(text)

    spec = importlib.util.spec_from_file_location(
        "gen", ROOT / "scripts" / "generate_ftanim_dense_bank.py")
    gen = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(gen)

    fmt = gen.RECORD.format
    if isinstance(fmt, bytes):
        fmt = fmt.decode()
    codes = [c for c in fmt if c != "<"]

    problems = []

    # 1. The C struct's own offsets must be internally consistent.
    off = 0
    computed = {}
    for name, ctype, count in fields:
        width = {"s32": 4, "s16": 2, "u16": 2, "u8": 1}[ctype]
        computed[name] = off
        off += width * count
    if declared_size != off:
        problems.append("header asserts sizeof == %s but its members total %d"
                        % (declared_size, off))
    for name, want in offsets.items():
        if computed.get(name) != want:
            problems.append("header asserts offsetof(%s) == %d, members give %s"
                            % (name, want, computed.get(name)))

    # 2. The emitter's record must be the same size.
    if gen.RECORD.size != declared_size:
        problems.append("emitter RECORD is %d bytes, header asserts %s"
                        % (gen.RECORD.size, declared_size))

    # 3. Field for field, in order, ignoring the C pad array vs the emitter's
    #    single pad word -- both are dead space, but they must total the same.
    c_seq = []
    for name, ctype, count in fields:
        c_seq.extend([(name, ctype)] * count)
    py_seq = [CODE[c] for c in codes]
    if len(c_seq) != len(py_seq):
        problems.append("header has %d scalar members, emitter packs %d"
                        % (len(c_seq), len(py_seq)))
    else:
        for i, ((name, ctype), (pytype, _w)) in enumerate(zip(c_seq, py_seq)):
            if ctype != pytype:
                problems.append("member %d (%s): header %s, emitter %s"
                                % (i, name, ctype, pytype))

    # 4. The Q constants must match the emitter's.
    for cname, pyname in (("NDS_ANIM_DENSE_LENGTH_Q", "LENGTH_Q"),
                          ("NDS_ANIM_DENSE_RATE_BASE_Q", "RATE_BASE_Q"),
                          ("NDS_ANIM_DENSE_LENGTH_INVERT_Q",
                           "LENGTH_INVERT_Q")):
        if consts.get(cname) != getattr(gen, pyname):
            problems.append("%s is %s in the header, %s in the emitter"
                            % (cname, consts.get(cname), getattr(gen, pyname)))

    print("header members  : %s"
          % ", ".join("%s:%s" % (n, t) for n, t, _c in fields))
    print("emitter format  : %s (%d bytes)" % (fmt, gen.RECORD.size))
    print("Q constants     : %s" % consts)

    if problems:
        print("\nFAIL: the dense-track layout disagrees between the emitter "
              "and the runtime header.")
        for p in problems:
            print("   " + p)
        print("These are two halves of one binary format. A disagreement here "
              "boots fine and animates one joint wrongly.")
        return 1
    print("\nFTANIM_DENSE_LAYOUT=AGREED  nds_anim_dense.h and "
          "generate_ftanim_dense_bank.py describe the same %d-byte record, "
          "field for field, with the same Q constants" % gen.RECORD.size)
    return 0


if __name__ == "__main__":
    sys.exit(main())
