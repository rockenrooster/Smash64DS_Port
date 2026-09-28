#!/usr/bin/env python3
"""Lane 1: typed-source tiling oracle.

decomp/BattleShip-main/decomp/src/relocData/<id>_<Name>.c are the decomp's
100%-typed masters of every reloc file (relocData.md: "0 / 17,082,000 bytes
still untyped").  Each is a sequence of top-level declarations in file order, so
summing sizeof(decl) reproduces every symbol's offset.  This module parses those
declarations (US branch of REGION guards) into a complete tiling
[(offset, size, ctype, name)] of a file.  The tiling is validated three ways by
the caller: total == O2R payload size, name suffix _0xNNN == running offset, and
every relocation slot lies at a 4-byte field of a pointer-bearing type.

It is used as an INDEPENDENT ORACLE next to the reachability walker
(lane1_classify.py); both are reported.
"""
from __future__ import annotations

import glob
import re
from dataclasses import dataclass

import lane1_o2r as o

RELOC_SRC = o.REPO / "decomp" / "BattleShip-main" / "decomp" / "src" / "relocData"

# sizeof for the element types that occur in the stage typed files (MIPS o32, big endian).
SIZEOF = {
    "u8": 1, "s8": 1, "char": 1, "u16": 2, "s16": 2, "u32": 4, "s32": 4, "f32": 4, "float": 4,
    "int": 4, "Gfx": 8, "Vtx": 16, "DObjDesc": 44, "DObjDLLink": 8, "Vec3f": 12, "Vec3h": 6,
    "MObjSub": 0x78, "MPVertexData": 6, "MPVertexLinks": 4, "MPLineInfo": 18, "MPMapObjData": 6,
    "MPGeometryData": 28, "MPGroundData": 0xA8, "MPItemWeights": 20,
    "Sprite": 0x48, "Bitmap": 16,
    "GRSectorDesc": 48, "SYInterpDesc": 24,
}
PTR = 4

# Class of each ctype (for the byte tiling).
CTYPE_CLASS = {
    "Gfx": "gfx", "Vtx": "vtx", "DObjDesc": "dobjdesc", "DObjDLLink": "dltab",
    "MObjSub": "mobj", "MPVertexData": "coll", "MPVertexLinks": "coll", "MPLineInfo": "coll",
    "MPMapObjData": "coll", "MPGeometryData": "coll", "MPGroundData": "hdr", "MPItemWeights": "hdr",
    "Sprite": "sprite", "Bitmap": "sprite",
}


@dataclass
class Decl:
    offset: int
    size: int
    ctype: str
    name: str
    count: int
    tex: bool = False
    is_ptr: bool = False
    line: int = 0


def strip_comments(src: str) -> str:
    out = []
    i = 0
    n = len(src)
    while i < n:
        if src.startswith("/*", i):
            j = src.find("*/", i + 2)
            i = n if j < 0 else j + 2
            out.append(" ")
        elif src.startswith("//", i):
            j = src.find("\n", i)
            i = n if j < 0 else j
        else:
            out.append(src[i])
            i += 1
    return "".join(out)


def us_branch(src: str) -> str:
    """Resolve #if defined(REGION_JP) / #ifndef / #else / #endif to the US arm; leave other
    preprocessor lines alone (they are dropped later)."""
    lines = src.split("\n")
    out = []
    stack = []      # each: (is_region_guard, keep_current)
    keep = True
    for ln in lines:
        s = ln.strip()
        m = re.match(r"#\s*(if|ifdef|ifndef|elif|else|endif)\b(.*)", s)
        if m:
            kw, rest = m.group(1), m.group(2)
            if kw in ("if", "ifdef", "ifndef"):
                region = "REGION_JP" in rest or "REGION_US" in rest
                if region:
                    jp_true = ("REGION_JP" in rest) != (kw == "ifndef" or "!" in rest.split("REGION")[0])
                    # condition true for US?
                    if "REGION_US" in rest:
                        us_true = (kw != "ifndef") and ("!" not in rest)
                    else:
                        us_true = (kw == "ifndef") or ("!" in rest)
                    stack.append((True, keep, us_true))
                    keep = keep and us_true
                else:
                    stack.append((False, keep, True))
                continue
            if kw == "else" and stack:
                region, outer, arm = stack[-1]
                if region:
                    keep = outer and (not arm)
                continue
            if kw == "elif" and stack:
                continue
            if kw == "endif" and stack:
                region, outer, arm = stack.pop()
                keep = outer
                continue
        if keep:
            out.append(ln)
    return "\n".join(out)


def split_statements(src: str):
    """Yield (text, start_line) for top-level statements ending in ';' at depth 0."""
    depth = 0
    cur = []
    line_no = 1
    start_line = 1
    i = 0
    n = len(src)
    while i < n:
        ch = src[i]
        if ch == "\n":
            line_no += 1
        if ch == "#" and (i == 0 or src[i - 1] == "\n") and depth == 0:
            j = src.find("\n", i)
            j = n if j < 0 else j
            i = j
            continue
        if not cur and ch.isspace():
            i += 1
            continue
        if not cur:
            start_line = line_no
        cur.append(ch)
        if ch in "{(":
            depth += 1
        elif ch in "})":
            depth -= 1
        elif ch == ";" and depth == 0:
            yield "".join(cur).strip(), start_line
            cur = []
        i += 1


def count_initializer(body: str) -> int:
    """Number of top-level elements in a brace initializer body (text between outer braces)."""
    depth = 0
    cnt = 0
    seen = False
    for ch in body:
        if ch in "{(":
            depth += 1
            seen = True if depth == 1 and ch == "{" else seen
        elif ch in "})":
            depth -= 1
        elif ch == "," and depth == 0:
            if seen or cnt >= 0:
                pass
        # handled below
    # simple robust count: split on commas at depth 0, ignore empty trailing element
    parts = []
    depth = 0
    cur = []
    for ch in body:
        if ch in "{(":
            depth += 1
        elif ch in "})":
            depth -= 1
        if ch == "," and depth == 0:
            parts.append("".join(cur))
            cur = []
        else:
            cur.append(ch)
    parts.append("".join(cur))
    return len([p for p in parts if p.strip()])


DECL_RE = re.compile(
    r"^(?:static\s+)?(?:const\s+)?(?P<type>[A-Za-z_][A-Za-z0-9_]*)\s*(?P<stars>(?:\*\s*)*)\s*(?P<name>[A-Za-z_][A-Za-z0-9_]*)\s*(?P<dims>(?:\[[^\]]*\]\s*)*)\s*(?:=\s*(?P<init>.*))?$",
    re.S)


def parse_file(fid: int):
    g = glob.glob(str(RELOC_SRC / f"{fid}_*.c"))
    if not g:
        return None, "no typed source"
    src = open(g[0], encoding="utf-8", errors="replace").read()
    raw_lines = src  # keep for tex annotations
    tex_offsets = {}
    src = strip_comments(src)
    src = us_branch(src)
    decls = []
    problems = []
    off = 0
    for stmt, line_no in split_statements(src):
        if not stmt or stmt.startswith("extern") or stmt.startswith("typedef") or stmt.startswith("#"):
            continue
        m = re.match(r"^PAD\(\s*(\d+)\s*\)\s*;?$", stmt)
        if m:
            n = int(m.group(1))
            decls.append(Decl(off, n, "PAD", "PAD", n, line=line_no))
            off += n
            continue
        body = stmt.rstrip(";").strip()
        m = DECL_RE.match(body)
        if not m:
            problems.append(f"{fid}:{line_no}: unparsed: {body[:70]!r}")
            continue
        t = m.group("type")
        stars = m.group("stars").replace(" ", "")
        name = m.group("name")
        dims = re.findall(r"\[([^\]]*)\]", m.group("dims") or "")
        init = m.group("init")
        is_ptr = len(stars) > 0
        base = PTR if is_ptr else SIZEOF.get(t)
        if t == "void" and not is_ptr:
            problems.append(f"{fid}:{line_no}: void non-pointer {name}")
            continue
        if t in ("AObjEvent32", "MObjSub") and is_ptr:
            base = PTR
        if base is None:
            problems.append(f"{fid}:{line_no}: unknown type {t} ({name})")
            continue
        count = 1
        if dims:
            d0 = dims[0].strip()
            if d0 == "":
                if init is None:
                    problems.append(f"{fid}:{line_no}: empty dim without init {name}")
                    continue
                ib = init.strip()
                if ib.startswith("{") and ib.endswith("}"):
                    count = count_initializer(ib[1:-1])
                else:
                    problems.append(f"{fid}:{line_no}: odd init for {name}")
                    continue
            else:
                try:
                    count = int(d0, 0)
                except ValueError:
                    problems.append(f"{fid}:{line_no}: nonliteral dim {d0!r} for {name}")
                    continue
            for extra in dims[1:]:
                try:
                    count *= int(extra.strip(), 0)
                except ValueError:
                    problems.append(f"{fid}:{line_no}: nonliteral dim {extra!r} for {name}")
        size = base * count
        # alignment: MIPS aligns each object to its type alignment (u16:2, 4-byte types:4, Vtx/Gfx: 8?)
        align = 1
        if is_ptr or t in ("u32", "s32", "f32", "float", "int", "MObjSub", "DObjDesc", "DObjDLLink", "Vec3f",
                           "MPGeometryData", "MPGroundData", "MPItemWeights", "Sprite", "Bitmap",
                           "GRSectorDesc", "SYInterpDesc"):
            align = 4
        elif t in ("u16", "s16", "Vec3h", "MPVertexData", "MPVertexLinks", "MPLineInfo", "MPMapObjData"):
            align = 2
        elif t in ("Gfx", "Vtx"):
            align = 8
        if align > 1 and off % align:
            pad = align - (off % align)
            decls.append(Decl(off, pad, "ALIGN", "ALIGN", pad, line=line_no))
            off += pad
        decls.append(Decl(off, size, t + ("*" if is_ptr else ""), name, count, is_ptr=is_ptr, line=line_no))
        off += size
    return decls, problems


def check(fid: int, verbose=False):
    decls, problems = parse_file(fid)
    f = o.index()[fid]
    if decls is None:
        return None
    end = decls[-1].offset + decls[-1].size if decls else 0
    bad_names = 0
    named = 0
    for d in decls:
        m = re.search(r"_0x([0-9A-Fa-f]+)$", d.name)
        if m and d.ctype not in ("PAD", "ALIGN"):
            named += 1
            if int(m.group(1), 16) != d.offset:
                bad_names += 1
    return {"fid": fid, "size": f.data_size, "typed_end": end, "decls": len(decls), "problems": problems,
            "named": named, "bad_names": bad_names}


if __name__ == "__main__":
    import sys
    ids = [int(x) for x in sys.argv[1:]] or [104]
    for fid in ids:
        r = check(fid)
        print(r)
