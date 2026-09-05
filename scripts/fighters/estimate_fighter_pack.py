#!/usr/bin/env python3
"""Semantic pack estimator, stage 1: a read-only typed object index.

Specification: ``docs/p2/P2-2-pack-estimator.md`` (which binds
``docs/reviews/Review_Deriving_Fighter_Live_After_Setup_Set.md`` to this
repository).  Nothing here re-derives a figure that document already carries;
the verified inputs are read back from
``scripts/fighters/fighter_production_manifest.json``.

What this stage produces
------------------------
For one fighter's relocData closure, a per-object index carrying

  * symbol name
  * C type (and pointer depth)
  * element count
  * computed byte size (``N * sizeof(TYPE)``, laid out from the decomp headers)
  * file offset

plus the pointer edges recovered from the ``.reloc`` sidecars, split into
intra-file (``intern``) and cross-file (``extern``) with the donor file named.

Offsets are *anchored*, not merely reconstructed.  Three independent sources
are read and cross-checked at every declaration:

  1. ``*Main`` header comments -- ``/* @ 0x039C, 40 bytes: ... */``
  2. Model-file anchor comments -- ``/* MObjSub @ 0xB0 */``,
     ``/* Vtx: Vtx_0x04B0_Vtx @ 0x4B0 (144 vertices) */``,
     ``/* DisplayList: Joint_0x0DB0 @ 0xDB0 (640 bytes, 80 cmds) */``,
     ``/* gap sub-block @ 0x1A878 (was gap+0x57C, 40 bytes) */`` ...
  3. Offsets embedded in symbol names -- ``dKirbyModel_Vtx_0x04B0_Vtx``,
     ``dKirbyModel_gap_0x31CC_sub_0x8EC`` (base + sub)

against a running walk that accumulates ``N * sizeof(TYPE)`` in file order.
Priority is anchor > walk > name; every disagreement is reported by kind and
none is silently resolved.  An anchor's byte count describes a *region* of the
original file, which the generator may have split across several declarations,
so it is checked against everything inside that region rather than against one
declaration.

Two things the corpus needs beyond a line scan, both of which silently
corrupted an earlier draft of this tool:

  * ``#if defined(REGION_JP) / #else`` wraps whole declarations, and both
    branches define the SAME symbol.  Only the selected region (``--region``,
    default US, matching the manifest's ``data_bytes``) is compiled;
    KirbyMainMotion read 158% of its own size before that existed.
  * a macro row in an unbounded array is not always one element:
    ``ftMotionCommandSubroutine(addr)`` expands to an opcode word plus an
    address word.  Such rows are expanded from the header ``#define`` and
    word-counted; a macro with no definition in the headers is refused.

Fail-closed
-----------
The spec's caveat is that the rare-declaration sweep behind it was done by
grep, not a full line audit.  So every top-level statement must be classified;
an unrecognised declaration form is *refused* (recorded with file, line and
text) and never silently skipped.  ``--strict`` turns any refusal or
disagreement into a non-zero exit.  Likewise the struct-layout engine refuses
any C construct it cannot lay out rather than guessing a size, and the enum
scanner stops at the first value it cannot evaluate rather than shifting every
later counter.

Read-only.  This tool writes nothing except an optional ``--json`` report.

Usage
-----
    python scripts/fighters/estimate_fighter_pack.py --fighter Kirby
    python scripts/fighters/estimate_fighter_pack.py --fighter Kirby --json out.json
    python scripts/fighters/estimate_fighter_pack.py --file 328_KirbyModel.c --all
    python scripts/fighters/estimate_fighter_pack.py --ledger
    python scripts/fighters/estimate_fighter_pack.py --ledger --fighter Kirby
"""

from __future__ import annotations

import argparse
import json
import os
import re
import sys
from collections import Counter, OrderedDict

# --------------------------------------------------------------------------
# Repository layout
# --------------------------------------------------------------------------

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
REPO_ROOT = os.path.dirname(os.path.dirname(SCRIPT_DIR))
DECOMP_ROOT = os.path.join(REPO_ROOT, "decomp", "BattleShip-main", "decomp")
RELOCDATA_DIR = os.path.join(DECOMP_ROOT, "src", "relocData")
MANIFEST_PATH = os.path.join(SCRIPT_DIR, "fighter_production_manifest.json")

# Header trees searched for type definitions.  Read-only reference material.
HEADER_DIRS = (
    os.path.join(DECOMP_ROOT, "include"),
    os.path.join(DECOMP_ROOT, "src"),
)


class Refusal(Exception):
    """A construct this tool declines to guess at.  Always surfaced, never eaten."""


# ==========================================================================
# Comment-aware C source splitting
# ==========================================================================


class Comment(object):
    __slots__ = ("text", "line", "index")

    def __init__(self, text, line, index):
        self.text = text
        self.line = line
        self.index = index

    def __repr__(self):  # pragma: no cover - debug aid
        return "Comment(line=%d, %r)" % (self.line, self.text[:60])


def split_comments(text):
    """Blank out comments, returning (code_with_comments_blanked, [Comment]).

    Character positions are preserved so line numbers stay exact.  String and
    character literals are honoured so a ``/*`` inside one is not eaten.
    """
    out = []
    comments = []
    i = 0
    n = len(text)
    line = 1
    # Comments are blanked in place, so every output chunk is the same length
    # as its input and ``i`` is the character index in BOTH strings.  (Using
    # ``len(out)`` here instead would count list elements, not characters, and
    # silently slide every anchor onto the wrong declaration.)
    while i < n:
        ch = text[i]
        if ch == "\n":
            out.append(ch)
            line += 1
            i += 1
        elif text.startswith("/*", i):
            j = text.find("*/", i + 2)
            if j < 0:
                raise Refusal("unterminated /* comment at line %d" % line)
            j += 2
            body = text[i:j]
            comments.append(Comment(body, line, i))
            out.append(re.sub(r"[^\n]", " ", body))
            line += body.count("\n")
            i = j
        elif text.startswith("//", i):
            j = text.find("\n", i)
            if j < 0:
                j = n
            body = text[i:j]
            comments.append(Comment(body, line, i))
            out.append(" " * len(body))
            i = j
        elif ch in "\"'":
            quote = ch
            j = i + 1
            while j < n:
                if text[j] == "\\":
                    j += 2
                    continue
                if text[j] == quote:
                    j += 1
                    break
                if text[j] == "\n":
                    raise Refusal("unterminated literal at line %d" % line)
                j += 1
            else:
                raise Refusal("unterminated literal at line %d" % line)
            out.append(text[i:j])
            i = j
        else:
            out.append(ch)
            i += 1
    return "".join(out), comments


def line_of(text, index):
    return text.count("\n", 0, index) + 1


# The whole relocData corpus uses exactly two preprocessor conditions --
# ``defined(REGION_JP)`` and ``defined(REGION_US)`` -- and nothing else.
# Both branches of one are real declarations of the SAME symbol, so parsing
# both double-counts the file (KirbyMainMotion read 158% of its own size
# before this existed).  Anything but these two conditions is refused.
_COND_RE = re.compile(
    r"^[ \t]*#[ \t]*(if|ifdef|ifndef|elif|else|endif)\b([^\n]*)$", re.M)
_REGION_COND_RE = re.compile(r"^\s*defined\s*\(\s*REGION_(JP|US)\s*\)\s*$")


def blank_inactive_regions(code, region):
    """Blank the branches that the chosen region does not compile.

    Characters are replaced by spaces (newlines kept) so every downstream
    offset -- comment anchors especially -- stays exactly where it was.
    Returns ``(code, [(line, directive_text)])`` where the second element
    lists conditions this tool does not understand.
    """
    chars = list(code)
    unknown = []
    stack = []  # (active_now, any_branch_taken, parent_active)

    def active():
        return all(f[0] for f in stack)

    def blank(a, b):
        for k in range(a, b):
            if chars[k] != "\n":
                chars[k] = " "

    for m in _COND_RE.finditer(code):
        kw, rest = m.group(1), m.group(2).strip()
        line = line_of(code, m.start())
        parent = active()
        if kw in ("if", "ifdef", "ifndef"):
            if kw == "if":
                cm = _REGION_COND_RE.match(rest)
                value = None if cm is None else (cm.group(1) == region)
            else:
                name = rest.split()[0] if rest.split() else ""
                if name in ("REGION_JP", "REGION_US"):
                    value = (name.split("_")[1] == region)
                    if kw == "ifndef":
                        value = not value
                else:
                    value = None
            if value is None:
                unknown.append((line, m.group(0).strip()))
                value = True  # keep the text; the refusal is reported
            stack.append([bool(value), bool(value), parent])
        elif kw == "elif":
            if not stack:
                unknown.append((line, m.group(0).strip()))
                continue
            unknown.append((line, m.group(0).strip()))
            stack[-1][0] = False
        elif kw == "else":
            if not stack:
                unknown.append((line, m.group(0).strip()))
                continue
            stack[-1][0] = not stack[-1][1]
        elif kw == "endif":
            if stack:
                stack.pop()
        # the directive line itself never contributes bytes
        blank(m.start(), m.end())
        # blank the body of an inactive branch up to the next directive
        if not active():
            nxt = _COND_RE.search(code, m.end())
            blank(m.end(), nxt.start() if nxt else len(code))
    return "".join(chars), unknown


# ==========================================================================
# C type layout engine
# ==========================================================================
#
# Target ABI is the N64's 32-bit big-endian MIPS: pointers are 4 bytes,
# ``long`` is 4, ``long long`` is 8 and forces 8-byte alignment.  Every size
# this engine computes is cross-checked against the corpus offset anchors, so
# a wrong layout shows up as a disagreement rather than a quiet bad number.

BASE_TYPES = {
    "char": (1, 1),
    "signed char": (1, 1),
    "unsigned char": (1, 1),
    "short": (2, 2),
    "short int": (2, 2),
    "signed short": (2, 2),
    "unsigned short": (2, 2),
    "unsigned short int": (2, 2),
    "int": (4, 4),
    "signed": (4, 4),
    "signed int": (4, 4),
    "unsigned": (4, 4),
    "unsigned int": (4, 4),
    "long": (4, 4),
    "long int": (4, 4),
    "unsigned long": (4, 4),
    "unsigned long int": (4, 4),
    "long long": (8, 8),
    "long long int": (8, 8),
    "signed long long": (8, 8),
    "unsigned long long": (8, 8),
    "unsigned long long int": (8, 8),
    "float": (4, 4),
    "double": (8, 8),
    "_Bool": (1, 1),
}

POINTER_SIZE = 4
POINTER_ALIGN = 4

_TYPE_QUALIFIERS = ("const", "volatile", "static", "register", "__restrict", "restrict")

_IDENT = r"[A-Za-z_][A-Za-z0-9_]*"


class Record(object):
    """A parsed struct/union body awaiting layout."""

    __slots__ = ("kind", "name", "body", "origin")

    def __init__(self, kind, name, body, origin):
        self.kind = kind  # "struct" | "union"
        self.name = name
        self.body = body
        self.origin = origin


class TypeTable(object):
    """Struct/union/typedef/constant index over the decomp headers."""

    def __init__(self):
        self.records = {}  # "struct Foo" / "union Foo" -> Record
        self.typedefs = {}  # name -> declarator string
        self.constants = {}  # name -> int
        self.func_macros = {}  # name -> (params, body)
        self._cache = {}
        self._in_progress = set()
        self._deferred_enums = []
        self.requested = set()
        self.conflicts = []

    # -- ingestion ---------------------------------------------------------

    def load_dirs(self, dirs):
        for root_dir in dirs:
            if not os.path.isdir(root_dir):
                continue
            for dirpath, _dirnames, filenames in os.walk(root_dir):
                for fn in sorted(filenames):
                    if fn.endswith((".h", ".inc.h")):
                        self.load_header(os.path.join(dirpath, fn))
        # An enum entry may name a constant defined in a header loaded later,
        # which stops that enum's scan.  Re-run the stalled ones until nothing
        # more resolves, so an ordering accident never leaves a constant --
        # and therefore an array bound -- silently missing.
        for _pass in range(4):
            deferred, self._deferred_enums = self._deferred_enums, []
            before = len(self.constants)
            for body in deferred:
                self._scan_enum_body(body)
            if len(self.constants) == before and not self._deferred_enums:
                break

    def load_header(self, path):
        try:
            with open(path, "r", encoding="utf-8", errors="replace") as fh:
                raw = fh.read()
        except OSError:
            return
        try:
            code, _comments = split_comments(raw)
        except Refusal:
            return
        rel = os.path.relpath(path, DECOMP_ROOT)
        joined = code.replace("\\\n", " ")
        self._scan_constants(joined, rel)
        self._scan_macros(joined)
        self._scan_records(code, rel)

    def _scan_macros(self, code):
        """Index function-like ``#define``s so a macro row can be word-counted."""
        for m in re.finditer(r"^[ \t]*#[ \t]*define[ \t]+(" + _IDENT + r")\(([^)]*)\)[ \t]*(.*)$",
                             code, re.M):
            name, params, body = m.group(1), m.group(2), m.group(3).strip()
            if not body:
                continue
            params = [p.strip() for p in params.split(",") if p.strip()]
            if any(p == "..." for p in params):
                continue  # variadic: not word-countable without full expansion
            self.func_macros.setdefault(name, (params, body))

    def _scan_constants(self, code, origin):
        for m in re.finditer(r"^[ \t]*#[ \t]*define[ \t]+(" + _IDENT + r")[ \t]+([^\n]+)$",
                             code, re.M):
            name, value = m.group(1), m.group(2).strip()
            if "(" in name:
                continue
            val = self._try_int(value)
            if val is not None:
                self.constants.setdefault(name, val)
        # enum bodies: enum [tag] { A, B = 3, C };  (the tag may sit on its
        # own line, so newlines are allowed before the brace)
        for m in re.finditer(r"\benum\b[ \t\n]*(?:" + _IDENT + r")?[ \t\n]*\{", code):
            body = _brace_body(code, m.end() - 1)
            if body is not None:
                self._scan_enum_body(body)

    def _scan_enum_body(self, body):
        """Record an enum's constants, stopping at the first value it cannot
        evaluate.  Continuing past one would shift every later counter and
        publish wrong array bounds -- a silent failure, so it stops instead."""
        counter = 0
        for item in _split_top(body, ","):
            item = item.strip()
            if not item or item.startswith("#"):
                continue
            if "=" in item:
                name, _, expr = item.partition("=")
                val = self._try_int(expr.strip())
                if val is None:
                    self._deferred_enums.append(body)
                    return
                counter = val
                name = name.strip()
            else:
                name = item
            if re.match(r"^" + _IDENT + r"$", name):
                self.constants.setdefault(name, counter)
            counter += 1

    # -- macro word counting ----------------------------------------------

    def macro_words(self, call, depth=0):
        """How many array elements a macro row contributes.

        ``ftMotionCommandWait(3)``           -> 1 word
        ``ftMotionCommandSubroutine(addr)``  -> 2 words (S1 opcode + address)

        The row is expanded using only macros this table knows.  Anything left
        unexpanded is a call of the form ``f(...)``, whose commas are inside
        parentheses and so cannot change the top-level count.  An unknown
        outermost macro raises rather than being assumed to be one word.
        """
        expanded = self._expand(call, 0)
        return len(_split_top(expanded, ","))

    _CALL_RE = re.compile(r"(?<![A-Za-z0-9_])(" + _IDENT + r")\s*\(")

    def _expand(self, text, depth):
        if depth > 8:
            raise Refusal("macro expansion too deep in %r" % text[:60])
        pos = 0
        while True:
            m = self._CALL_RE.search(text, pos)
            if not m:
                return text
            name = m.group(1)
            open_paren = m.end() - 1
            if name not in self.func_macros:
                # Not ours: its commas stay inside its parentheses, so it
                # cannot affect the top-level word count.
                pos = open_paren + 1
                continue
            close = _matching_paren(text, open_paren)
            if close is None:
                raise Refusal("unbalanced macro call %r" % text[:60])
            params, body = self.func_macros[name]
            args = [a.strip() for a in _split_top(text[open_paren + 1:close], ",")]
            if args == [""] and not params:
                args = []
            if len(args) != len(params):
                raise Refusal("macro %s takes %d args, called with %d"
                              % (name, len(params), len(args)))
            out = body
            for pname, aval in zip(params, args):
                out = re.sub(r"(?<![A-Za-z0-9_])" + re.escape(pname) + r"(?![A-Za-z0-9_])",
                             "(" + aval + ")", out)
            out = self._expand(out, depth + 1)
            text = text[:m.start()] + out + text[close + 1:]
            # Skip past the already-expanded text so a self-referential macro
            # cannot spin forever.
            pos = m.start() + len(out)

    def _scan_records(self, code, origin):
        # typedef struct/union [tag] { ... } Name [, *Alias];
        for m in re.finditer(r"\btypedef[ \t\n]+(struct|union)\b[ \t\n]*(" + _IDENT + r")?[ \t\n]*\{",
                             code):
            kind, tag = m.group(1), m.group(2)
            body = _brace_body(code, m.end() - 1)
            if body is None:
                continue
            close = code.index("}", m.end() - 1 + len(body) + 1)
            tail = code[close + 1:code.find(";", close)] if code.find(";", close) > 0 else ""
            names = [t.strip() for t in tail.split(",")]
            rec = Record(kind, tag, body, origin)
            if tag:
                self._put_record("%s %s" % (kind, tag), rec)
            for nm in names:
                nm = nm.strip()
                if re.match(r"^" + _IDENT + r"$", nm):
                    self._put_record("%s %s" % (kind, nm or tag), rec)
                    self.typedefs.setdefault(nm, "%s %s" % (kind, nm))
                elif nm.startswith("*"):
                    ptr = nm.lstrip("*").strip()
                    if re.match(r"^" + _IDENT + r"$", ptr):
                        self.typedefs.setdefault(ptr, "void *")

        # struct/union Name { ... };
        for m in re.finditer(r"(?<![A-Za-z0-9_])(struct|union)[ \t\n]+(" + _IDENT + r")[ \t\n]*\{",
                             code):
            if _preceded_by_typedef(code, m.start()):
                continue
            kind, tag = m.group(1), m.group(2)
            body = _brace_body(code, m.end() - 1)
            if body is None:
                continue
            self._put_record("%s %s" % (kind, tag), Record(kind, tag, body, origin))

        # typedef <existing> Name;   (no brace body)
        for m in re.finditer(r"\btypedef[ \t\n]+([^;{}()]+?)[ \t\n]*(" + _IDENT + r")[ \t]*;", code):
            base, name = m.group(1).strip(), m.group(2)
            if not base or base.endswith(","):
                continue
            self.typedefs.setdefault(name, base)

        # typedef <ret> (*Name)(args);  -> function pointer
        for m in re.finditer(r"\btypedef[^;]*\(\s*\*\s*(" + _IDENT + r")\s*\)\s*\([^;]*\)\s*;", code):
            self.typedefs.setdefault(m.group(1), "void *")

    def _put_record(self, key, rec):
        prev = self.records.get(key)
        if prev is None:
            self.records[key] = rec
            return
        if _normalise_ws(prev.body) != _normalise_ws(rec.body):
            self.conflicts.append((key, prev.origin, rec.origin))

    def _try_int(self, expr):
        """Evaluate if it is an integer expression, else None.

        The filtering is left to ``eval_const``, which resolves identifiers
        against the constant table and refuses anything else.  An earlier
        character-class pre-filter here rejected every mixed-case identifier,
        which quietly stalled the ``FTKind`` enum and lost ``nFTKindEnumCount``
        -- and with it an array bound.
        """
        expr = expr.strip()
        expr = re.sub(r"/\*.*?\*/", " ", expr)
        if not expr or '"' in expr or "'" in expr or "{" in expr:
            return None
        try:
            return self.eval_const(expr)
        except Refusal:
            return None

    # -- constant expressions ---------------------------------------------

    def eval_const(self, expr):
        """Evaluate an integer constant expression, refusing anything unusual."""
        expr = expr.strip()
        if not expr:
            raise Refusal("empty constant expression")
        expr = re.sub(r"\bU?L{0,2}\b(?=\s*(?:[)\]+\-*/|&^]|$))", "", expr)
        tokens = re.findall(r"0[xX][0-9A-Fa-f]+|\d+|" + _IDENT + r"|<<|>>|[()+\-*/|&^~]", expr)
        if "".join(tokens) != re.sub(r"\s+", "", expr):
            raise Refusal("unparsable constant expression %r" % expr)
        parts = []
        for tok in tokens:
            if re.match(r"^" + _IDENT + r"$", tok):
                if tok not in self.constants:
                    raise Refusal("unknown constant %r in %r" % (tok, expr))
                parts.append(str(self.constants[tok]))
            else:
                parts.append(tok)
        safe = "".join(parts)
        if not re.match(r"^[\s0-9xXa-fA-F()+\-*/|&^~<>]+$", safe):
            raise Refusal("unsafe constant expression %r" % expr)
        try:
            value = eval(safe, {"__builtins__": {}}, {})  # noqa: S307 - digits/operators only
        except Exception as exc:  # pragma: no cover - defensive
            raise Refusal("cannot evaluate %r (%s)" % (expr, exc))
        if not isinstance(value, int):
            raise Refusal("non-integer constant expression %r" % expr)
        return value

    # -- sizeof / alignof --------------------------------------------------

    def sizeof(self, type_name):
        return self.layout(type_name)[0]

    def layout(self, type_name):
        """Return (size, align) for a type name, refusing what it cannot lay out."""
        key = _normalise_ws(type_name)
        self.requested.add(key)
        if key in self._cache:
            return self._cache[key]
        if key in self._in_progress:
            raise Refusal("recursive type %r" % type_name)
        self._in_progress.add(key)
        try:
            result = self._layout_uncached(key)
        finally:
            self._in_progress.discard(key)
        self._cache[key] = result
        return result

    def _layout_uncached(self, name):
        name = _strip_qualifiers(name)
        if name.endswith("*"):
            return (POINTER_SIZE, POINTER_ALIGN)
        if name in BASE_TYPES:
            return BASE_TYPES[name]
        if name.startswith("struct ") or name.startswith("union "):
            rec = self.records.get(name)
            if rec is None:
                raise Refusal("no definition for %r" % name)
            return self._layout_record(rec)
        if name in self.typedefs:
            return self.layout(self.typedefs[name])
        for kind in ("struct", "union"):
            if "%s %s" % (kind, name) in self.records:
                return self._layout_record(self.records["%s %s" % (kind, name)])
        raise Refusal("unknown type %r" % name)

    def _layout_record(self, rec):
        if rec.kind == "union":
            size = 0
            align = 1
            for field in _split_fields(rec.body):
                for msize, malign, _bits in self._field_units(field):
                    size = max(size, msize)
                    align = max(align, malign)
            return (_round_up(size, align), align)

        # struct: sequential layout with bitfield packing
        bit_pos = 0
        align = 1
        for field in _split_fields(rec.body):
            for msize, malign, bits in self._field_units(field):
                align = max(align, malign)
                if bits is None:
                    byte_pos = _round_up((bit_pos + 7) // 8, malign)
                    bit_pos = byte_pos * 8 + msize * 8
                else:
                    unit_bits = msize * 8
                    if bits == 0:
                        bit_pos = _round_up(bit_pos, unit_bits)
                        continue
                    if bits > unit_bits:
                        raise Refusal("bitfield wider than its container")
                    if (bit_pos % unit_bits) + bits > unit_bits:
                        bit_pos = _round_up(bit_pos, unit_bits)
                    bit_pos += bits
        size = _round_up((bit_pos + 7) // 8, align)
        return (size, align)

    def _field_units(self, field):
        """Yield (size, align, bitwidth_or_None) for every declarator in a field."""
        field = field.strip()
        if not field:
            return
        if field.startswith("#"):
            raise Refusal("preprocessor conditional inside a struct body: %r" % field[:60])
        if re.match(r"^(?:typedef|friend|public|private|protected)\b", field):
            raise Refusal("unsupported member form %r" % field[:60])
        if "(" in field and "*" in field and ")" in field:
            # function pointer member
            if re.search(r"\(\s*\*+\s*" + _IDENT + r"?\s*\)\s*\(", field):
                yield (POINTER_SIZE, POINTER_ALIGN, None)
                return

        m = re.match(r"^(struct|union)\b[ \t\n]*(" + _IDENT + r")?[ \t\n]*\{", field)
        if m:
            body = _brace_body(field, field.index("{", m.end() - 1))
            if body is None:
                raise Refusal("unterminated inline record member")
            rec = Record(m.group(1), m.group(2), body, "<inline>")
            size, align = self._layout_record(rec)
            close = field.rindex("}")
            tail = field[close + 1:].strip()
            if not tail:
                # anonymous member: contributes its own layout
                yield (size, align, None)
                return
            for decl in _split_top(tail, ","):
                decl = decl.strip()
                if not decl:
                    continue
                yield self._declarator_units(decl, None, inline=(size, align))
            return

        base, declarators = _split_base_and_declarators(field)
        if base is None:
            raise Refusal("unparsable struct member %r" % field[:80])
        for decl in declarators:
            yield self._declarator_units(decl, base)

    def _declarator_units(self, decl, base, inline=None):
        """Lay out one declarator.  ``base`` is a type name; ``inline`` an
        already-computed ``(size, align)`` for an inline struct/union body."""
        decl = decl.strip()
        bits = None
        if ":" in decl:
            name_part, _, bit_part = decl.partition(":")
            bits = self.eval_const(bit_part)
            decl = name_part.strip()
        stars = 0
        if decl.startswith("*"):
            stars = len(decl) - len(decl.lstrip("*"))
            decl = decl.lstrip("*").strip()
        if stars:
            # A pointer declarator never needs its pointee laid out, so
            # ``void *p`` and ``struct Opaque *p`` are both fine.
            base_size, base_align = POINTER_SIZE, POINTER_ALIGN
        elif inline is not None:
            base_size, base_align = inline
        else:
            base_size, base_align = self.layout(base)
        count = 1
        for dim in re.findall(r"\[([^\]]*)\]", decl):
            dim = dim.strip()
            if not dim:
                raise Refusal("flexible array member in a sized struct")
            count *= self.eval_const(dim)
        if bits is not None and count != 1:
            raise Refusal("array bitfield %r" % decl)
        return (base_size * count, base_align, bits)


def _preceded_by_typedef(code, pos):
    head = code[max(0, pos - 40):pos]
    return bool(re.search(r"\btypedef[ \t\n]*$", head))


def _normalise_ws(text):
    return " ".join(text.split())


def _round_up(value, align):
    if align <= 1:
        return value
    return ((value + align - 1) // align) * align


def _strip_qualifiers(name):
    parts = [p for p in name.split() if p not in _TYPE_QUALIFIERS]
    out = " ".join(parts)
    out = re.sub(r"\s*\*\s*", "*", out)
    if out.endswith("*"):
        return out.rstrip("*") + "*"
    return out


def _matching_paren(text, open_index):
    depth = 0
    for i in range(open_index, len(text)):
        if text[i] == "(":
            depth += 1
        elif text[i] == ")":
            depth -= 1
            if depth == 0:
                return i
    return None


def _brace_body(text, open_index):
    """Return the text between ``text[open_index] == '{'`` and its match."""
    if open_index < 0 or open_index >= len(text) or text[open_index] != "{":
        return None
    depth = 0
    for i in range(open_index, len(text)):
        if text[i] == "{":
            depth += 1
        elif text[i] == "}":
            depth -= 1
            if depth == 0:
                return text[open_index + 1:i]
    return None


def _split_top(text, sep):
    """Split on ``sep`` at brace/paren/bracket depth zero."""
    out = []
    depth = 0
    buf = []
    for ch in text:
        if ch in "{([":
            depth += 1
        elif ch in "})]":
            depth -= 1
        if ch == sep and depth == 0:
            out.append("".join(buf))
            buf = []
        else:
            buf.append(ch)
    out.append("".join(buf))
    return out


def _split_fields(body):
    """Split a struct/union body into member declarations."""
    fields = []
    depth = 0
    buf = []
    for ch in body:
        if ch in "{([":
            depth += 1
        elif ch in "})]":
            depth -= 1
        if ch == ";" and depth == 0:
            fields.append("".join(buf))
            buf = []
        else:
            buf.append(ch)
    tail = "".join(buf).strip()
    if tail:
        # A trailing non-empty fragment means the body did not end on ';'.
        if not tail.startswith("#"):
            fields.append(tail)
        else:
            fields.append(tail)
    return [f for f in (x.strip() for x in fields) if f]


def _split_base_and_declarators(field):
    """Split ``u16 a, *b[2] : 3`` into ("u16", ["a", "*b[2] : 3"])."""
    field = _normalise_ws(field)
    m = re.match(r"^((?:" + _IDENT + r"[ \t]+)*" + _IDENT + r")\b(.*)$", field)
    if not m:
        return None, []
    words = m.group(1).split()
    rest = m.group(2)
    # Peel trailing words back into the declarator until the head is a type.
    while words:
        base = " ".join(words)
        if base in BASE_TYPES or _looks_like_type(base):
            declarators = _split_top(rest, ",")
            declarators = [d.strip() for d in declarators if d.strip()]
            if not declarators:
                return None, []
            return base, declarators
        rest = words.pop() + " " + rest
    return None, []


def _looks_like_type(base):
    if base in BASE_TYPES:
        return True
    words = base.split()
    if len(words) == 1:
        return True
    if words[0] in ("struct", "union", "enum") and len(words) == 2:
        return True
    return False


# ==========================================================================
# relocData .c parsing
# ==========================================================================

PAD_RE = re.compile(r"^PAD\s*\(\s*([^)]+)\s*\)$")
FILE_SIZE_RE = re.compile(r"File size:\s*(?:US\s+)?(\d+)\s*bytes")
ANCHOR_RE = re.compile(r"@\s*(0[xX][0-9A-Fa-f]+)")
ANCHOR_BYTES_RE = re.compile(r"\((?:.*?,\s*)?(\d+)\s*bytes")
MAIN_HEADER_RE = re.compile(r"@\s*(0[xX][0-9A-Fa-f]+)\s*,\s*(\d+)\s*bytes")

# The type group is non-greedy and must be separated from the declarator by
# whitespace or a '*'.  A greedy version silently eats the leading run of a
# symbol name (``u32 dKirbyModel_gap_0x31CC_sub_0`` + name ``x8``), which is
# exactly the class of quiet mis-parse this tool exists to refuse.
DECL_RE = re.compile(
    r"^(?P<extern>extern\s+)?(?P<static>static\s+)?"
    r"(?P<type>(?:" + _IDENT + r"\s+)*?" + _IDENT + r")"
    r"(?:\s+|\s*(?=\*))"
    r"(?P<stars>\**)\s*"
    r"(?P<name>" + _IDENT + r")\s*"
    r"(?P<dims>(?:\[[^\]]*\]\s*)*)"
    r"(?P<init>=\s*.*)?$",
    re.S,
)


class ObjectRow(object):
    __slots__ = (
        "symbol", "type_name", "pointer_depth", "count", "elem_size", "size",
        "offset", "offset_source", "line", "file_id", "file_name", "is_pad",
        "anchor_offset", "anchor_size", "name_offset", "walk_offset",
        "size_source", "notes", "init_text", "head_comment",
    )

    def __init__(self, **kw):
        for slot in self.__slots__:
            setattr(self, slot, kw.get(slot))
        if self.notes is None:
            self.notes = []

    def as_dict(self):
        return OrderedDict(
            symbol=self.symbol,
            type=self.type_name,
            pointer_depth=self.pointer_depth,
            count=self.count,
            elem_size=self.elem_size,
            size=self.size,
            size_source=self.size_source,
            offset=self.offset,
            offset_source=self.offset_source,
            anchor_offset=self.anchor_offset,
            name_offset=self.name_offset,
            walk_offset=self.walk_offset,
            file_id=self.file_id,
            file=self.file_name,
            line=self.line,
            is_pad=self.is_pad,
            notes=list(self.notes),
        )


class RefusedRow(object):
    __slots__ = ("file_name", "line", "form", "reason", "text")

    def __init__(self, file_name, line, form, reason, text):
        self.file_name = file_name
        self.line = line
        self.form = form
        self.reason = reason
        self.text = text

    def as_dict(self):
        return OrderedDict(file=self.file_name, line=self.line, form=self.form,
                           reason=self.reason, text=self.text)


class Disagreement(object):
    __slots__ = ("file_name", "symbol", "line", "kind", "detail")

    def __init__(self, file_name, symbol, line, kind, detail):
        self.file_name = file_name
        self.symbol = symbol
        self.line = line
        self.kind = kind
        self.detail = detail

    def as_dict(self):
        return OrderedDict(file=self.file_name, symbol=self.symbol, line=self.line,
                           kind=self.kind, detail=self.detail)


class ParsedFile(object):
    def __init__(self, file_id, file_name, path):
        self.file_id = file_id
        self.file_name = file_name
        self.path = path
        self.objects = []
        self.externs = []       # (type, symbol, line)
        self.directives = 0
        self.refused = []
        self.disagreements = []
        self.declared_size = None
        self.statement_count = 0
        self.region = None
        self.manifest = None

    @property
    def accounted_bytes(self):
        return sum(o.size for o in self.objects if o.size is not None)

    @property
    def end_offset(self):
        end = 0
        for o in self.objects:
            if o.offset is not None and o.size is not None:
                end = max(end, o.offset + o.size)
        return end


# A generated symbol carries an offset only in these shapes.  Anything else --
# notably ``dKirbyModel_Joint_0x0020_post_palettes``, where the hex names the
# PARENT block and the object itself sits at parent+0x78 -- yields no offset,
# because reading one out of it would invent a wrong anchor.
#
# ``_post`` is deliberately NOT a tag: ``gap_0x31CC_sub_0x41C_post`` is the
# "Raw tail after 1 DL(s) @ 0x3700" that follows the display list at 0x35E8,
# so its hex names the sibling it trails, not itself.
_NAME_TAGS = r"(?:Vtx|DisplayList)"
_NAME_PLAIN_RE = re.compile(r"_0[xX]([0-9A-Fa-f]+)$")
_NAME_TAGGED_RE = re.compile(r"_0[xX]([0-9A-Fa-f]+)_" + _NAME_TAGS + r"$")
_NAME_SUB_RE = re.compile(
    r"_0[xX]([0-9A-Fa-f]+)(?:_[A-Za-z][A-Za-z0-9]*)?_sub_0[xX]([0-9A-Fa-f]+)$")


def name_embedded_offset(symbol):
    """Recover the offset embedded in a generated symbol name.

    ``dKirbyModel_Vtx_0x04B0_Vtx``             -> 0x04B0
    ``dKirbyModel_Tex_0x1A8A0``                -> 0x1A8A0
    ``dKirbyModel_gap_0x1A2FC_sub_0x57C``      -> 0x1A2FC + 0x57C = 0x1A878
    ``dKirbyModel_Joint_0x0020_post_sub_0x90`` -> 0x0020 + 0x90   = 0xB0
    ``dKirbyModel_Joint_0x0020_post_palettes`` -> (None) ambiguous

    Returns ``(offset, rule)``; ``(None, None)`` when the name says nothing
    this tool is willing to trust.
    """
    m = _NAME_SUB_RE.search(symbol)
    if m:
        return int(m.group(1), 16) + int(m.group(2), 16), "name+sub"
    m = _NAME_TAGGED_RE.search(symbol)
    if m:
        return int(m.group(1), 16), "name"
    m = _NAME_PLAIN_RE.search(symbol)
    if m:
        return int(m.group(1), 16), "name"
    return None, None


def _reconcile(anchor_off, name_off, walk_off):
    """Pick an offset from the three sources and return every candidate.

    Priority is anchor > walk > name.  The walk outranks the symbol name
    because a generated label's hex is not always absolute: the palette
    ``dKirbyModel_palette_0x1E7C`` sits at 0x1C178, its hex being an offset
    into the enclosing ``gap_0x1A2FC`` block.  The name is therefore kept as a
    cross-check and reported when it dissents, never used to move an object
    that the anchors and the walk agree about.
    """
    candidates = []
    if anchor_off is not None:
        candidates.append(("anchor", anchor_off))
    if walk_off is not None:
        candidates.append(("walk", walk_off))
    if name_off is not None:
        candidates.append(("name", name_off))
    if not candidates:
        return None, None, candidates
    return candidates[0][1], candidates[0][0], candidates


def _record_offset_disagreement(pf, row, candidates):
    """Report -- never resolve -- a conflict between the offset sources."""
    seen = {}
    for src, value in candidates:
        seen.setdefault(value, []).append(src)
    if len(seen) < 2:
        return
    structural = {v for v, srcs in seen.items() if set(srcs) & {"anchor", "walk"}}
    kind = "offset:anchor-vs-walk" if len(structural) > 1 else "offset:name"
    pf.disagreements.append(Disagreement(
        pf.file_name, row.symbol, row.line, kind,
        "; ".join("%s=0x%X" % ("/".join(s), v) for v, s in sorted(seen.items()))))


def _resolve_block_sizes(pf):
    """Decide whether each anchor's byte count matches the block it names.

    An anchor comment describes a *block* of the original file, and the
    generator may have split that block into several declarations: a display
    list plus the 4-byte chain pointer it was extracted from
    (``..._sub_0x6D24`` 880 B + ``..._post`` 4 B against an 884-byte anchor),
    or a palette plus its trailing ``PAD(8)`` (32 + 8 against 40).  So the
    anchor size is compared against everything from the anchored object up to
    the next anchored object -- not against that one declaration alone.
    Anything left over is a real disagreement.
    """
    objs = pf.objects
    for i, row in enumerate(objs):
        if row.anchor_size is None or row.offset is None:
            continue
        region_end = row.offset + row.anchor_size
        total = 0
        members = 0
        for j in range(i, len(objs)):
            o = objs[j]
            if o.size is None or o.offset is None:
                break
            if o.offset >= region_end:
                break
            total += o.size
            members += 1
            if o.offset + o.size >= region_end:
                break
        if total == row.anchor_size:
            if members > 1:
                row.notes.append("anchor region spans %d declarations" % members)
            continue
        pf.disagreements.append(Disagreement(
            pf.file_name, row.symbol, row.line, "size",
            "anchor region 0x%X..0x%X (%d bytes): %d declaration(s) cover %d, "
            "%d byte(s) undeclared"
            % (row.offset, region_end, row.anchor_size, members, total,
               row.anchor_size - total)))


def _top_level_statements(code):
    """Yield (text, start_index) for statements at brace depth zero.

    Preprocessor lines are yielded whole so ``#include`` at file scope is
    classified, while ``#include`` inside an initialiser stays part of it.
    """
    n = len(code)
    i = 0
    depth = 0
    start = 0
    buf = []
    while i < n:
        ch = code[i]
        if depth == 0 and ch == "#" and (i == 0 or code[i - 1] == "\n" or
                                         code[:i].rsplit("\n", 1)[-1].strip() == ""):
            j = code.find("\n", i)
            if j < 0:
                j = n
            directive = code[i:j]
            if "".join(buf).strip():
                # a directive interrupting a top-level statement
                yield ("".join(buf), start)
                buf = []
            yield (directive, i)
            i = j
            start = i
            continue
        if ch == "{":
            depth += 1
        elif ch == "}":
            depth -= 1
        if ch == ";" and depth == 0:
            text = "".join(buf)
            if text.strip():
                yield (text, start)
            buf = []
            start = i + 1
        else:
            if not buf and not ch.strip():
                start = i + 1
            else:
                buf.append(ch)
        i += 1
    if "".join(buf).strip():
        yield ("".join(buf), start)


def _count_initializer_elements(init, types):
    """Count top-level elements of an unbounded array initialiser.

    Refuses rather than guessing whenever the row shape is not decidable:
    ``#if`` region alternatives, mixed brace/scalar rows, included bodies with
    no bound, and macro rows whose ``#define`` this tool never found.  A macro
    row whose definition IS known is expanded and word-counted, because such a
    row is not always one element -- ``ftMotionCommandSubroutine(addr)``
    expands to an opcode word plus an address word.
    """
    body = _brace_body(init, init.index("{"))
    if body is None:
        raise Refusal("unterminated initialiser")
    if re.search(r"^\s*#\s*(if|ifdef|ifndef|else|elif)\b", body, re.M):
        raise Refusal("region conditional inside an unbounded array initialiser")
    items = [it.strip() for it in _split_top(body, ",")]
    items = [it for it in items if it]
    if not items:
        raise Refusal("empty initialiser for an unbounded array")
    braced = [it for it in items if it.startswith("{")]
    if braced and len(braced) != len(items):
        raise Refusal("mixed brace/scalar rows in an unbounded array initialiser")
    if braced:
        return len(items)
    total = 0
    for it in items:
        if "#include" in it:
            raise Refusal("included body with no array bound")
        m = re.match(r"^(" + _IDENT + r")\s*\(", it)
        if m:
            if m.group(1) not in types.func_macros:
                raise Refusal("macro row %r has no #define this tool could find"
                              % m.group(1))
            total += types.macro_words(it)
        else:
            total += 1
    return total


def parse_reloc_c(path, file_id, types, region="US"):
    with open(path, "r", encoding="utf-8", errors="replace") as fh:
        raw = fh.read()
    code, comments = split_comments(raw)
    code, unknown_conds = blank_inactive_regions(code, region)
    pf = ParsedFile(file_id, os.path.basename(path), path)
    pf.region = region
    for cline, ctext in unknown_conds:
        pf.refused.append(RefusedRow(
            os.path.basename(path), cline, "unhandled preprocessor condition",
            "only defined(REGION_JP) / defined(REGION_US) are understood", ctext))

    for c in comments:
        m = FILE_SIZE_RE.search(c.text)
        if m:
            pf.declared_size = int(m.group(1))
            break

    # index comments by source position so a statement can claim the anchor
    # comment block that precedes it
    comment_positions = sorted((c.index, c) for c in comments)
    comment_cursor = 0
    pending_anchor = None   # (offset, size_hint, line)
    pending_comment = None  # the comment text the anchor came from, if any

    walk = 0
    walk_valid = True

    def refuse(line, form, reason, text):
        """Record a construct this parser declines to interpret.

        A refusal also drops any pending offset anchor and invalidates the
        walk: an anchor that survives an unparsed declaration silently lands
        on the NEXT object, turning one refusal into a drift of every offset
        after it.
        """
        nonlocal pending_anchor, pending_comment, walk_valid
        pf.refused.append(RefusedRow(pf.file_name, line, form, reason, text))
        pending_anchor = None
        pending_comment = None
        walk_valid = False

    for text, start in _top_level_statements(code):
        pf.statement_count += 1
        line = line_of(code, start)

        # absorb every comment that appears before this statement
        while comment_cursor < len(comment_positions) and comment_positions[comment_cursor][0] < start:
            c = comment_positions[comment_cursor][1]
            comment_cursor += 1
            m = MAIN_HEADER_RE.search(c.text)
            if m:
                pending_anchor = (int(m.group(1), 16), int(m.group(2)), c.line)
                pending_comment = c.text
                continue
            m = ANCHOR_RE.search(c.text)
            if m:
                size_hint = None
                ms = ANCHOR_BYTES_RE.search(c.text)
                if ms:
                    size_hint = int(ms.group(1))
                pending_anchor = (int(m.group(1), 16), size_hint, c.line)
                pending_comment = c.text

        stmt = _normalise_ws(text)
        if not stmt:
            continue

        if stmt.startswith("#"):
            pf.directives += 1
            continue

        m = PAD_RE.match(stmt)
        if m:
            try:
                nbytes = types.eval_const(m.group(1))
            except Refusal as exc:
                refuse(line, "PAD()", str(exc), stmt[:120])
                continue
            # A PAD is a real ``static u8 [n]`` in .data, and an anchor
            # comment can land on it -- ``/* Per-joint dispatch table @ 0x1918
            # ... */ PAD(8);`` -- so it takes part in the reconciliation like
            # any other object.  Letting the anchor skip past it slides the
            # rest of the file by the pad width.
            anchor_off = anchor_size = None
            if pending_anchor is not None:
                anchor_off, anchor_size, _aline = pending_anchor
                pending_anchor = None
            head_comment = pending_comment
            pending_comment = None
            offset, offset_source, cands = _reconcile(anchor_off, None,
                                                      walk if walk_valid else None)
            row = ObjectRow(symbol="_relocdata_pad_%d" % line, type_name="u8",
                            pointer_depth=0, count=nbytes, elem_size=1, size=nbytes,
                            size_source="PAD", file_id=file_id, file_name=pf.file_name,
                            line=line, is_pad=True, walk_offset=walk if walk_valid else None,
                            anchor_offset=anchor_off, anchor_size=anchor_size,
                            offset=offset, offset_source=offset_source, notes=["pad"],
                            init_text=None, head_comment=head_comment)
            _record_offset_disagreement(pf, row, cands)
            pf.objects.append(row)
            if offset is not None:
                walk = offset + nbytes
                walk_valid = True
            elif walk_valid:
                walk += nbytes
            continue

        m = DECL_RE.match(stmt)
        if not m:
            refuse(line, "unrecognised statement",
                   "does not match a C declaration", stmt[:160])
            continue

        type_name = _normalise_ws(m.group("type"))
        stars = m.group("stars") or ""
        name = m.group("name")
        dims = m.group("dims") or ""
        init = m.group("init")
        is_extern = bool(m.group("extern"))

        if type_name.startswith("extern "):
            type_name = type_name[len("extern "):]
            is_extern = True

        if is_extern:
            pf.externs.append((type_name + stars, name, line))
            continue

        if init is None:
            refuse(line, "tentative definition",
                   "declaration without an initialiser", stmt[:160])
            continue

        pointer_depth = len(stars)
        # element size
        try:
            if pointer_depth:
                elem_size = POINTER_SIZE
            else:
                elem_size = types.sizeof(type_name)
        except Refusal as exc:
            refuse(line, "unlayoutable type %r" % type_name, str(exc), stmt[:160])
            continue

        # element count
        dim_list = re.findall(r"\[([^\]]*)\]", dims)
        count = None
        count_source = None
        if not dim_list:
            count = 1
            count_source = "scalar"
        else:
            count = 1
            unbounded = False
            for d in dim_list:
                d = d.strip()
                if not d:
                    unbounded = True
                    continue
                try:
                    count *= types.eval_const(d)
                except Refusal as exc:
                    refuse(line, "unevaluable array bound", str(exc), stmt[:160])
                    count = None
                    break
            if count is None:
                continue
            if unbounded:
                try:
                    count *= _count_initializer_elements(init, types)
                    count_source = "counted"
                except (Refusal, ValueError) as exc:
                    refuse(line, "unbounded array with an uncountable initialiser",
                           str(exc), stmt[:160])
                    continue
            else:
                count_source = "bound"

        computed = elem_size * count

        anchor_off = anchor_size = None
        if pending_anchor is not None:
            anchor_off, anchor_size, _aline = pending_anchor
            pending_anchor = None
        head_comment = pending_comment
        pending_comment = None

        name_off, name_rule = name_embedded_offset(name)

        # -- reconcile the three offset sources ---------------------------
        offset, offset_source, candidates = _reconcile(
            anchor_off, name_off, walk if walk_valid else None)

        row = ObjectRow(symbol=name, type_name=type_name, pointer_depth=pointer_depth,
                        count=count, elem_size=elem_size, size=computed,
                        size_source="count*sizeof(%s)" % type_name if not pointer_depth
                        else "count*sizeof(ptr)",
                        offset=offset, offset_source=offset_source, line=line,
                        file_id=file_id, file_name=pf.file_name, is_pad=False,
                        anchor_offset=anchor_off, anchor_size=anchor_size,
                        name_offset=name_off, walk_offset=walk if walk_valid else None,
                        notes=[count_source], init_text=init, head_comment=head_comment)

        # disagreements are reported, never silently resolved
        _record_offset_disagreement(pf, row, candidates)
        if name_rule:
            row.notes.append(name_rule)
        pf.objects.append(row)

        # advance the walk from the most trusted offset available
        if offset is not None:
            walk = offset + computed
            walk_valid = True
        elif walk_valid:
            walk += computed

    # an anchor's byte count names a block, which may be several declarations
    _resolve_block_sizes(pf)

    return pf


def check_file_tail(pf, manifest_bytes):
    """Compare where the declarations end against the file's real size.

    The size comes from the file's own ``/* File size: N bytes */`` header
    when it has one, else from the manifest.  A shortfall is trailing padding
    the ``.c`` never declares; an overrun means the walk is wrong.
    """
    size = pf.declared_size if pf.declared_size is not None else manifest_bytes
    if size is None:
        return
    source = "the file header" if pf.declared_size is not None else "the manifest"
    end = pf.end_offset
    if end < size:
        pf.disagreements.append(Disagreement(
            pf.file_name, "<file tail>", 0, "tail",
            "declarations end at 0x%X; %s says %d bytes (0x%X): %d undeclared"
            % (end, source, size, size, size - end)))
    elif end > size:
        pf.disagreements.append(Disagreement(
            pf.file_name, "<file tail>", 0, "tail",
            "declarations end at 0x%X, PAST the %d bytes (0x%X) %s reports"
            % (end, size, size, source)))


# ==========================================================================
# .reloc sidecar parsing
# ==========================================================================

RELOC_LINE_RE = re.compile(
    r"^(?P<type>extern|intern)\s+(?P<ptr>\S+)\s+(?P<target>\S+)\s*(?:#\s*(?P<comment>.*))?$"
)
RELOC_FILE_RE = re.compile(r"->\s*file\s+(\d+)\s*\(([^)]*)\)")


class Edge(object):
    __slots__ = ("kind", "ptr_symbol", "ptr_delta", "target_symbol", "target_delta",
                 "target_offset", "donor_file_id", "donor_file_name", "line")

    def __init__(self, **kw):
        for slot in self.__slots__:
            setattr(self, slot, kw.get(slot))

    def key(self):
        return (self.kind, self.ptr_symbol, self.ptr_delta, self.target_symbol,
                self.target_delta, self.target_offset, self.donor_file_id)

    def as_dict(self):
        return OrderedDict(kind=self.kind, ptr=self.ptr_symbol, ptr_delta=self.ptr_delta,
                           target=self.target_symbol, target_delta=self.target_delta,
                           target_offset=self.target_offset,
                           donor_file_id=self.donor_file_id,
                           donor_file=self.donor_file_name, line=self.line)


def _split_label(label):
    """``Symbol+0x8`` -> ("Symbol", 8); ``0x1CF60`` -> (None, 0x1CF60)."""
    if "+" in label:
        sym, _, delta = label.partition("+")
        return sym, int(delta, 16 if delta.lower().startswith("0x") else 10)
    if re.match(r"^0[xX][0-9A-Fa-f]+$", label):
        return None, int(label, 16)
    return label, 0


class ParsedReloc(object):
    def __init__(self, file_id, file_name, path):
        self.file_id = file_id
        self.file_name = file_name
        self.path = path
        self.edges = []
        self.duplicates = 0
        self.refused = []

    @property
    def unique_edges(self):
        seen = set()
        out = []
        for e in self.edges:
            k = e.key()
            if k in seen:
                continue
            seen.add(k)
            out.append(e)
        return out


def parse_reloc_sidecar(path, file_id):
    pr = ParsedReloc(file_id, os.path.basename(path), path)
    seen = set()
    with open(path, "r", encoding="utf-8", errors="replace") as fh:
        for lineno, raw in enumerate(fh, 1):
            line = raw.strip()
            if not line or line.startswith("#"):
                continue
            m = RELOC_LINE_RE.match(line)
            if not m:
                pr.refused.append(RefusedRow(pr.file_name, lineno, "reloc row",
                                             "does not match <type> <ptr> <target>", line[:160]))
                continue
            kind = m.group("type")
            ptr_sym, ptr_delta = _split_label(m.group("ptr"))
            tgt_sym, tgt_delta = _split_label(m.group("target"))
            donor_id = donor_name = None
            comment = m.group("comment") or ""
            fm = RELOC_FILE_RE.search(comment)
            if fm:
                donor_id, donor_name = int(fm.group(1)), fm.group(2)
            elif kind == "extern":
                pr.refused.append(RefusedRow(pr.file_name, lineno, "extern reloc row",
                                             "no '# -> file N (Name)' donor suffix", line[:160]))
                continue
            edge = Edge(kind=kind, ptr_symbol=ptr_sym, ptr_delta=ptr_delta,
                        target_symbol=tgt_sym,
                        target_delta=tgt_delta if tgt_sym else None,
                        target_offset=None if tgt_sym else tgt_delta,
                        donor_file_id=donor_id, donor_file_name=donor_name, line=lineno)
            k = edge.key()
            if k in seen:
                pr.duplicates += 1
            seen.add(k)
            pr.edges.append(edge)
    return pr


# ==========================================================================
# Closure assembly
# ==========================================================================


class OffsetMap(object):
    """Resolve ``Symbol[+0xN]`` and bare file offsets to an indexed object.

    The ``.reloc`` sidecars still name blocks the generated ``.c`` has since
    split up -- ``dKirbyModel_Joint_0x0020_post`` is nine declarations now --
    so a symbol lookup alone loses those rows.  Resolving through the file
    offset instead catches them, and whatever is left over is reported rather
    than dropped.
    """

    def __init__(self, parsed_file):
        self.by_symbol = {}
        self.spans = []  # (start, end, symbol)
        for o in parsed_file.objects:
            if o.offset is None or o.size is None:
                continue
            self.by_symbol.setdefault(o.symbol, o.offset)
            self.spans.append((o.offset, o.offset + o.size, o.symbol))
        self.spans.sort()

    def offset_of(self, symbol, delta):
        base = self.by_symbol.get(symbol)
        if base is None:
            base, _rule = name_embedded_offset(symbol)
        if base is None:
            return None
        return base + delta

    def owner(self, offset):
        if offset is None:
            return None
        lo, hi = 0, len(self.spans) - 1
        while lo <= hi:
            mid = (lo + hi) // 2
            start, end, sym = self.spans[mid]
            if offset < start:
                hi = mid - 1
            elif offset >= end:
                lo = mid + 1
            else:
                return sym
        return None


def join_edges(idx):
    """Attach every ``.reloc`` edge to the objects it connects.

    Returns a counter dict; unresolved endpoints are listed so no edge is
    quietly lost between the sidecar and the index.
    """
    maps = {pf.file_id: OffsetMap(pf) for pf in idx.files}
    stats = Counter()
    unresolved = Counter()
    for pr in idx.relocs:
        src = maps.get(pr.file_id)
        for e in pr.unique_edges:
            stats["edges"] += 1
            ptr_off = src.offset_of(e.ptr_symbol, e.ptr_delta) if src else None
            owner = src.owner(ptr_off) if src else None
            if owner is None:
                stats["ptr_unresolved"] += 1
                unresolved["ptr:%s" % e.ptr_symbol] += 1
            else:
                stats["ptr_resolved"] += 1
            if e.kind == "intern":
                tgt_off = (e.target_offset if e.target_symbol is None
                           else (src.offset_of(e.target_symbol, e.target_delta or 0)
                                 if src else None))
                tgt_owner = src.owner(tgt_off) if src else None
            else:
                dmap = maps.get(e.donor_file_id)
                tgt_off = e.target_offset
                if tgt_off is None and e.target_symbol and dmap:
                    tgt_off = dmap.offset_of(e.target_symbol, e.target_delta or 0)
                tgt_owner = dmap.owner(tgt_off) if dmap else None
                if dmap is None:
                    stats["target_donor_outside_closure"] += 1
            if tgt_owner is None:
                stats["target_unresolved"] += 1
                unresolved["target:%s" % (e.target_symbol or "0x%X" % (tgt_off or 0))] += 1
            else:
                stats["target_resolved"] += 1
    return stats, unresolved


def load_manifest():
    with open(MANIFEST_PATH, "r", encoding="utf-8") as fh:
        return json.load(fh)


def fighter_entry(manifest, fighter):
    for f in manifest["fighters"]:
        if f["fighter"].lower() == fighter.lower():
            return f
    names = ", ".join(f["fighter"] for f in manifest["fighters"])
    raise SystemExit("unknown fighter %r; manifest has: %s" % (fighter, names))


_RELOCDATA_LISTING = []


def _relocdata_listing():
    if not _RELOCDATA_LISTING:
        _RELOCDATA_LISTING.extend(sorted(os.listdir(RELOCDATA_DIR)))
    return _RELOCDATA_LISTING


def relocdata_paths(file_id):
    """Resolve ``<id>_*.c`` / ``<id>_*.reloc``.

    The manifest's ``path`` names some files ``MiscDataNNN``; on disk they
    carry their real names (``201_FTCommonMoveset.c``), so resolve by id.
    """
    c_path = reloc_path = None
    prefix = "%d_" % file_id
    for fn in _relocdata_listing():
        if not fn.startswith(prefix):
            continue
        if fn.endswith(".c") and c_path is None:
            c_path = os.path.join(RELOCDATA_DIR, fn)
        elif fn.endswith(".reloc") and ".jp." not in fn and reloc_path is None:
            reloc_path = os.path.join(RELOCDATA_DIR, fn)
    return c_path, reloc_path


class ClosureIndex(object):
    def __init__(self, fighter):
        self.fighter = fighter
        self.files = []          # ParsedFile
        self.relocs = []         # ParsedReloc
        self.missing = []        # (id, path, data_bytes, reason)
        self.manifest_files = [] # closure entries

    @property
    def objects(self):
        for pf in self.files:
            for o in pf.objects:
                yield o

    @property
    def refused(self):
        out = list()
        for pf in self.files:
            out.extend(pf.refused)
        for pr in self.relocs:
            out.extend(pr.refused)
        return out

    @property
    def disagreements(self):
        out = []
        for pf in self.files:
            out.extend(pf.disagreements)
        return out


def index_closure(fighter, types, only_file=None, region="US"):
    idx = ClosureIndex(fighter)
    manifest = load_manifest()
    entry = fighter_entry(manifest, fighter)
    closure = entry["core_extern_closure"]
    idx.manifest_files = closure

    for member in closure:
        file_id = member["id"]
        c_path, reloc_path = relocdata_paths(file_id)
        if only_file and (not c_path or os.path.basename(c_path) != only_file):
            continue
        if c_path is None:
            idx.missing.append((file_id, member["path"], member["data_bytes"],
                                "no <id>_*.c in decomp/.../relocData"))
            continue
        pf = parse_reloc_c(c_path, file_id, types, region=region)
        pf.manifest = member
        check_file_tail(pf, member["data_bytes"])
        idx.files.append(pf)
        if reloc_path is not None:
            idx.relocs.append(parse_reloc_sidecar(reloc_path, file_id))
    return idx, entry


# ==========================================================================
# Reporting
# ==========================================================================


def pct(part, whole):
    if not whole:
        return 0.0
    return 100.0 * part / whole


def report(idx, entry, args, types):
    out = []
    w = out.append

    closure = entry["core_extern_closure"]
    closure_data = sum(m["data_bytes"] for m in closure)
    closure_disk = sum(m["bytes"] for m in closure)

    w("=" * 78)
    w("Semantic pack estimator, stage 1 -- typed object index")
    w("Fighter: %s   closure: %d files" % (entry["fighter"], len(closure)))
    w("=" * 78)
    w("")

    w("--- closure, per file -------------------------------------------------")
    w("%-6s %-24s %10s %10s %7s %8s %8s %7s" %
      ("id", "file", "data_B", "indexed_B", "short_B", "objs", "externs", "cover%"))
    total_indexed = 0
    total_data = 0
    parsed_ids = set()
    for pf in idx.files:
        member = pf.manifest
        indexed = pf.accounted_bytes
        total_indexed += indexed
        total_data += member["data_bytes"]
        parsed_ids.add(pf.file_id)
        w("%-6d %-24s %10d %10d %7d %8d %8d %6.2f%%" %
          (pf.file_id, pf.file_name, member["data_bytes"], indexed,
           member["data_bytes"] - indexed, len(pf.objects), len(pf.externs),
           pct(indexed, member["data_bytes"])))
    for file_id, path, data_bytes, reason in idx.missing:
        w("%-6d %-24s %10d %10s %7s %8s %8s %6s   (%s)" %
          (file_id, os.path.basename(path), data_bytes, "-", "-", "-", "-", "-", reason))
    w("")

    w("--- coverage ----------------------------------------------------------")
    w("closure data bytes (manifest)        : %10d" % closure_data)
    w("closure on-disk bytes (o2r included) : %10d" % closure_disk)
    w("indexed object bytes                 : %10d" % total_indexed)
    w("undeclared (file padding, refusals)  : %10d" % (closure_data - total_indexed))
    w("COVERAGE over the whole closure      : %9.3f%%" % pct(total_indexed, closure_data))
    if idx.missing:
        unreachable = sum(d for _i, _p, d, _r in idx.missing)
        w("of which unreachable (no typed .c)   : %10d bytes (%.2f%% of closure)"
          % (unreachable, pct(unreachable, closure_data)))
        w("COVERAGE over files that have a .c   : %9.2f%%"
          % pct(total_indexed, closure_data - unreachable))
    w("")

    # the two figures the spec pins
    model = next((pf for pf in idx.files if pf.file_name.endswith("Model.c")
                  and entry["fighter"] in pf.file_name), None)
    main = next((pf for pf in idx.files if pf.file_name.endswith("Main.c")
                 and entry["fighter"] in pf.file_name), None)
    w("--- against the spec's verified figures --------------------------------")
    if main is not None:
        m = main.manifest
        w("%s: manifest alloc_bytes=%d  data_bytes=%d  disk=%d"
          % (main.file_name, m["alloc_bytes"], m["data_bytes"], m["bytes"]))
        w("  indexed %d B of its own %d data bytes (%.3f%%, %d undeclared)"
          % (main.accounted_bytes, m["data_bytes"],
             pct(main.accounted_bytes, m["data_bytes"]),
             m["data_bytes"] - main.accounted_bytes))
        w("  NOTE: alloc_bytes on the Main slot is the WHOLE CLOSURE's data total")
        w("        (sum of the %d members' data_bytes = %d), not this file's size."
          % (len(closure), closure_data))
    if model is not None:
        m = model.manifest
        w("%s: disk=%d  member data=%d  (delta %d = o2r header)"
          % (model.file_name, m["bytes"], m["data_bytes"], m["bytes"] - m["data_bytes"]))
        w("  indexed %d B of %d (%.3f%%, %d byte(s) undeclared)"
          % (model.accounted_bytes, m["data_bytes"],
             pct(model.accounted_bytes, m["data_bytes"]),
             m["data_bytes"] - model.accounted_bytes))
    w("")

    w("--- object index, by declared type ------------------------------------")
    hist = Counter()
    bytes_by_type = Counter()
    for o in idx.objects:
        key = o.type_name + "*" * (o.pointer_depth or 0)
        hist[key] += 1
        bytes_by_type[key] += o.size or 0
    w("%-24s %8s %12s %7s" % ("type", "objects", "bytes", "share"))
    for key, cnt in sorted(hist.items(), key=lambda kv: -bytes_by_type[kv[0]]):
        w("%-24s %8d %12d %6.2f%%" %
          (key, cnt, bytes_by_type[key], pct(bytes_by_type[key], total_indexed)))
    ext_hist = Counter()
    for pf in idx.files:
        for tname, _sym, _line in pf.externs:
            ext_hist[tname] += 1
    w("")
    w("extern declarations (edges, not objects): %d over %d files"
      % (sum(ext_hist.values()), len(idx.files)))
    w("")

    w("--- .reloc edges ------------------------------------------------------")
    tot_rows = tot_unique = tot_dup = 0
    intern = extern = 0
    donors = Counter()
    for pr in idx.relocs:
        uniq = pr.unique_edges
        tot_rows += len(pr.edges)
        tot_unique += len(uniq)
        tot_dup += pr.duplicates
        for e in uniq:
            if e.kind == "intern":
                intern += 1
            else:
                extern += 1
                donors[(e.donor_file_id, e.donor_file_name)] += 1
    w("rows parsed: %d   unique: %d   duplicate rows dropped: %d"
      % (tot_rows, tot_unique, tot_dup))
    w("intern (intra-file): %d    extern (cross-file): %d" % (intern, extern))
    jstats, junres = join_edges(idx)
    if jstats["edges"]:
        w("joined to the object index: patch sites %d/%d, targets %d/%d"
          % (jstats["ptr_resolved"], jstats["edges"],
             jstats["target_resolved"], jstats["edges"]))
        if jstats["target_donor_outside_closure"]:
            w("  %d target(s) live in a donor file outside this closure"
              % jstats["target_donor_outside_closure"])
        if junres:
            w("  unresolved endpoints (%d distinct):" % len(junres))
            for label, cnt in junres.most_common(None if args.all else 12):
                w("    %-62s x%d" % (label, cnt))
            if not args.all and len(junres) > 12:
                w("    ... %d more (use --all)" % (len(junres) - 12))
    if donors:
        w("cross-file donors:")
        for (fid, fname), cnt in donors.most_common():
            inside = " (in closure)" if fid in {m["id"] for m in closure} else " (OUT OF CLOSURE)"
            w("  file %-5s %-28s %5d edges%s" % (fid, fname, cnt, inside))
    w("")

    w("--- offset disagreements ----------------------------------------------")
    dis = idx.disagreements
    if not dis:
        w("none: every anchor, name-embedded offset and walk step agreed.")
    else:
        by_kind = Counter(d.kind for d in dis)
        w("%d disagreement(s), by kind:" % len(dis))
        for kind, cnt in by_kind.most_common():
            w("  %-26s %d" % (kind, cnt))
        w("")
        limit = None if args.all else 40
        for kind, _cnt in by_kind.most_common():
            w("  [%s]" % kind)
            rows = [d for d in dis if d.kind == kind]
            for d in rows[:limit]:
                w("    %-22s %-44s line %-6s %s"
                  % (d.file_name, d.symbol, d.line, d.detail))
            if limit and len(rows) > limit:
                w("    ... %d more (use --all)" % (len(rows) - limit))
    w("")

    w("--- refused declaration forms -----------------------------------------")
    refused = idx.refused
    if not refused:
        w("none: every top-level statement was classified.")
    else:
        by_form = Counter(r.form for r in refused)
        w("%d refusal(s) across %d distinct forms:" % (len(refused), len(by_form)))
        for form, cnt in by_form.most_common():
            w("  %-52s x%d" % (form, cnt))
        w("")
        w("  first occurrences:")
        seen = set()
        for r in refused:
            if r.form in seen:
                continue
            seen.add(r.form)
            w("    %s:%d  %s" % (r.file_name, r.line, r.reason))
            w("      %s" % r.text)
    w("")

    used = [c for c in types.conflicts if c[0] in types.requested
            or c[0].split(" ", 1)[-1] in types.requested]
    if used:
        w("--- type definition conflicts affecting a USED type --------------------")
        for key, a, b in used:
            w("  %-40s %s vs %s" % (key, a, b))
        w("")

    w("--- verdict -----------------------------------------------------------")
    coverage = pct(total_indexed, closure_data)
    w("closure coverage: %.3f%%   refusals: %d   offset disagreements: %d"
      % (coverage, len(refused), len(dis)))
    return "\n".join(out)


def build_json(idx, entry):
    closure = entry["core_extern_closure"]
    doc = OrderedDict()
    doc["schema"] = "smash64ds.pack_estimator.object_index.v1"
    doc["stage"] = 1
    doc["fighter"] = entry["fighter"]
    doc["spec"] = "docs/p2/P2-2-pack-estimator.md"
    doc["closure"] = closure
    doc["closure_data_bytes"] = sum(m["data_bytes"] for m in closure)
    doc["indexed_bytes"] = sum(pf.accounted_bytes for pf in idx.files)
    doc["files"] = []
    for pf in idx.files:
        doc["files"].append(OrderedDict(
            id=pf.file_id,
            file=pf.file_name,
            declared_size=pf.declared_size,
            manifest_data_bytes=pf.manifest["data_bytes"],
            statements=pf.statement_count,
            objects=[o.as_dict() for o in pf.objects],
            externs=[OrderedDict(type=t, symbol=s, line=l) for t, s, l in pf.externs],
            refused=[r.as_dict() for r in pf.refused],
            disagreements=[d.as_dict() for d in pf.disagreements],
        ))
    doc["reloc"] = []
    for pr in idx.relocs:
        doc["reloc"].append(OrderedDict(
            id=pr.file_id,
            file=pr.file_name,
            rows=len(pr.edges),
            duplicates=pr.duplicates,
            edges=[e.as_dict() for e in pr.unique_edges],
            refused=[r.as_dict() for r in pr.refused],
        ))
    doc["missing_files"] = [OrderedDict(id=i, path=p, data_bytes=d, reason=r)
                            for i, p, d, r in idx.missing]
    return doc


# ==========================================================================
# Layout dump: show the sizes the engine derived, for eyeball verification
# ==========================================================================

def layout_dump(types, names):
    """Print sizeof/alignof for the types the corpus declares objects of.

    These are not asserted constants: they are computed from the read-only
    decomp headers, and every one of them is cross-checked object by object
    against the file anchors during the run.  The dump exists so a wrong
    layout shows up as a single obvious line rather than a wall of offset
    disagreements.
    """
    lines = ["--- struct layout engine, sizes derived from the decomp headers -------"]
    for n in sorted(names):
        if n == "void":
            lines.append("  %-26s (declared only as void*: %d bytes)"
                         % (n, POINTER_SIZE))
            continue
        try:
            size, align = types.layout(n)
        except Refusal as exc:
            lines.append("  %-26s REFUSED: %s" % (n, exc))
            continue
        lines.append("  %-26s size=%-6d (0x%-5X) align=%d" % (n, size, size, align))
    return "\n".join(lines)


# ==========================================================================
# Stage 2: the disposition ledger
# ==========================================================================
#
# Specification: ``docs/p2/P2-2-pack-estimator.md`` ("The disposition table
# this corpus needs") plus the two audited corrections from
# ``docs/reviews/Independent_Review_P2_Residency_and_Four_Fighter_Plans.md``
# section 2.2, applied verbatim:
#
#   * hurtbox defaults are NOT setup-only -- ``ftParamResetFighterDamageCollsAll``
#     restores ``fp->attr->damage_coll_descs`` from animation-event processing,
#     so ``FTAttributes`` (their owner) is retained whole, never consumed-then-
#     dropped;
#   * "low detail" compiles the EFFECTIVE selection including the null-entry
#     fallback to high-detail common parts -- no disposition below filters
#     source atoms by a high/low label, so both detail variants of every
#     retained-whole class stay in the pack by construction.
#
# Every object receives exactly one disposition.  An object no rule covers is a
# STOP -- recorded, printed, and never guessed.  A STOP anywhere invalidates
# the size verdict per the spec's gate table.
#
# Byte classes (per fighter, per costume):
#   retained     source bytes kept resident in the main-RAM pack
#   removable    source bytes the pack does not carry
#   replacement  NEW bytes the pack carries instead (native image census is
#                the measured RESIDENT term; compact records are estimates)
#   unresolved   bytes whose membership/translation is NOT proven today.  They
#                are charged into W at the conservative worst case AND listed
#                here -- an unknown must never silently become zero.
#
# Motion files keep their own lifetime (review section 6.5), so their streams
# are reported as Profile A (resident pack excludes them; today's per-instance
# figatree acquisition is unchanged) and Profile B (resident compact bank).

# SSB64 ships four costumes per fighter.  The stock-icon LUT arity in every
# Main file confirms the number structurally.
COSTUME_COUNT = 4

# Verified constants from the review (section 6.1); do not re-derive.
F_NEW_BASE = 208372          # 72,148 + 173,088 - 36,864
FLOOR_BYTES = 32768          # required general-heap floor
W_CEILING = F_NEW_BASE - FLOOR_BYTES   # 175,604 optimistic pack allowance
GREEN_BAND = 150 * 1024      # provisional GREEN ceiling (spec table)

# Compact replacement record sizes.  These are ESTIMATES, labelled as such in
# every report row; source bytes are the measured side.
REPL_MATERIAL_RECORD = 12    # MObjSub -> compact DS material record
REPL_NATIVE_ROOT_ID = 8      # retained native root id per replaced body DL
REPL_PTR_REF = 2             # pointer -> u16 section-relative ref (review 8.6)
REPL_BANK_HANDLE = 8         # VRAM texture handle + format record per bank
REPL_PACK_HEADER = 64        # per-kind pack header + manifest skeleton

# Build-config flags that gate members of the generated native image header.
# The owner-played configuration is the hwtri family (Makefile overrides
# NDS_R2_FIGHTER_HW_LIGHT := 1 there); the base default is 0.
NATIVE_IMAGE_FLAGS_HWTRI = {
    "NDS_R2_FIGHTER_HW_LIGHT": 1,
    "NDS_RENDERER_M2_DETAILED_LEDGER": 0,
    "NDS_TASK56_FIGHTER_PRIMITIVES": 0,
}
NATIVE_IMAGE_FLAGS_BASE = dict(NATIVE_IMAGE_FLAGS_HWTRI,
                               NDS_R2_FIGHTER_HW_LIGHT=0)

NATIVE_IMAGE_PATH = os.path.join(REPO_ROOT, "include", "nds", "generated",
                                 "nds_native_fighter_image.generated.h")

# ARM9 (32-bit little-endian) sizes/aligns of the image element types from
# include/nds/nds_native_fighter_tables.h.  Byte-exact hand layout, checked
# against the header's own member order.
_NATIVE_ELEM_LAYOUT = {
    "NDSNativeStateDelta": (12, 4),
    "NDSNativeVertexAction": (12, 4),
    "NDSNativeDenseVertex": (12, 4),
    "NDSNativeRun": (8, 4),
    "NDSNativeEpoch": (16, 4),
    "u8": (1, 1),
    "u16": (2, 2),
    "u32": (4, 4),
}

_IMG_STRUCT_RE = re.compile(
    r"typedef struct NDSNative(\w+?)(High|Low)Image\s*\{([^}]*)\}", re.S)
_IMG_MEMBER_RE = re.compile(r"^(\w+)\s+\w+\[(\d+)\];")
_IMG_GUARD_RE = re.compile(r"^#\s*(if|endif)\b\s*(.*)$")

# Guards the census evaluator understands.  Anything else is a Refusal.
_KNOWN_GUARDS = {
    "!NDS_R2_FIGHTER_HW_LIGHT || NDS_RENDERER_M2_DETAILED_LEDGER",
    "NDS_TASK56_FIGHTER_PRIMITIVES == 1",
    "NDS_TASK56_FIGHTER_PRIMITIVES == 2",
}


def _eval_image_guard(expr, flags):
    expr = expr.strip()
    if expr not in _KNOWN_GUARDS:
        raise Refusal("unhandled image member guard %r" % expr)
    if expr.startswith("!NDS_R2_FIGHTER_HW_LIGHT"):
        return (not flags["NDS_R2_FIGHTER_HW_LIGHT"]
                or flags["NDS_RENDERER_M2_DETAILED_LEDGER"])
    value = int(expr.rsplit("==", 1)[1])
    return flags["NDS_TASK56_FIGHTER_PRIMITIVES"] == value


def parse_native_image_census(flags=None):
    """Per-fighter {High, Low} native image byte sizes from the generated header.

    These are the RESIDENT replacement bytes review section 2.4 charges to W.
    Mario and Fox have no image slot (their owners are the P1-era linked
    tables inside the measured ARM9 baseline) and are absent from the result.
    """
    flags = NATIVE_IMAGE_FLAGS_HWTRI if flags is None else flags
    with open(NATIVE_IMAGE_PATH, "r", encoding="utf-8") as fh:
        text = fh.read()
    census = {}
    for m in _IMG_STRUCT_RE.finditer(text):
        fighter, detail, body = m.group(1), m.group(2), m.group(3)
        offset = 0
        align = 1
        guard_stack = []
        for line in body.splitlines():
            gm = _IMG_GUARD_RE.match(line.strip())
            if gm:
                if gm.group(1) == "if":
                    guard_stack.append(gm.group(2))
                else:
                    if not guard_stack:
                        raise Refusal("unbalanced #if/#endif in image struct")
                    guard_stack.pop()
                continue
            if guard_stack and not all(_eval_image_guard(g, flags)
                                       for g in guard_stack):
                continue
            mm = _IMG_MEMBER_RE.match(line.strip())
            if not mm:
                continue
            elem = mm.group(1)
            if elem not in _NATIVE_ELEM_LAYOUT:
                raise Refusal("unknown image element type %r" % elem)
            size, ealign = _NATIVE_ELEM_LAYOUT[elem]
            offset = _round_up(offset, ealign)
            offset += size * int(mm.group(2))
            align = max(align, ealign)
        if guard_stack:
            raise Refusal("unterminated #if in image struct for %s" % fighter)
        census.setdefault(fighter, {})[detail] = _round_up(offset, align)
    return census


# --------------------------------------------------------------------------
# Initializer evidence
# --------------------------------------------------------------------------

_NUM_WORD_RE = re.compile(r"^-?0[xX][0-9A-Fa-f]+[uUlL]*$|^-?\d+[uUlL]*$")
_MACRO_WORD_RE = re.compile(r"^([A-Za-z_][A-Za-z0-9_]*)\s*\(")


class Evidence(object):
    """What an object's initializer says about its contents."""

    __slots__ = ("kind", "numeric", "symbolish", "macros", "includes")

    def __init__(self, kind, numeric, symbolish, macros, includes):
        self.kind = kind        # numeric | macro | symbol | mixed | none
        self.numeric = numeric
        self.symbolish = symbolish
        self.macros = macros
        self.includes = includes


def initializer_evidence(init_text):
    if not init_text:
        return Evidence("none", 0, 0, set(), set())
    brace = init_text.find("{")
    if brace < 0:
        return Evidence("none", 0, 0, set(), set())
    body = _brace_body(init_text, brace)
    if body is None:
        return Evidence("none", 0, 0, set(), set())
    includes = set(re.findall(r"#include\s*<([^>]+)>", body))
    numeric = symbolish = 0
    macros = set()

    def count(word):
        nonlocal numeric, symbolish
        word = word.strip()
        if not word:
            return
        if _NUM_WORD_RE.match(word):
            numeric += 1
        else:
            symbolish += 1

    for item in _split_top(body, ","):
        item = item.strip()
        if not item:
            continue
        if item.startswith("{"):
            for sub in _split_top(item[1:-1], ","):
                count(sub)
            continue
        mc = _MACRO_WORD_RE.match(item)
        if mc:
            macros.add(mc.group(1))
            continue
        count(item)
    if includes and numeric == 0 and symbolish == 0:
        return Evidence("symbol", 0, 0, macros, includes)
    if macros and numeric == 0 and symbolish == 0:
        return Evidence("macro", 0, 0, macros, includes)
    if symbolish == 0:
        return Evidence("numeric", numeric, symbolish, macros, includes)
    if numeric == 0:
        return Evidence("symbol", numeric, symbolish, macros, includes)
    return Evidence("mixed", numeric, symbolish, macros, includes)


# --------------------------------------------------------------------------
# Dispositions
# --------------------------------------------------------------------------

# Exactly one per object.  Names follow the spec's table; the two corrections
# are noted on the rows they fix.
DISPOSITIONS = (
    "NATIVE_REPLACE_BODY",   # Vtx/Gfx of the fighter's own Model file
    "NATIVE_REPLACE_WEAPON", # Vtx/Gfx of donor/special files: replacement cost
                             # unmeasured -> charged at raw bytes, unresolved
    "MATERIAL_RECORD",       # MObjSub -> compact DS material record
    "RETAINED_JOINT_TREE",   # DObjDesc, both details (correction 2)
    "RETAINED_SEMANTIC",     # FT*/WP*/IT* tables incl. FTAttributes
                             # (correction 1: hurtbox defaults live here),
                             # FTModelPart rows incl. copy-hat rows
    "CONSERVATIVE_RETAIN",   # pointer-free numeric data, consumer unproven
    "EVENT_STREAM_RETAIN",   # AObjEvent programs outside the motion files
    "MOTION_STREAM",         # ftMotionCommand / motion event words
    "TEXEL_BANK",            # u8 texel bank, costume membership unresolved
    "PALETTE_BANK",          # u16 palette bank, ditto
    "PTR_TABLE",             # pointer arrays -> u16 refs
    "SCENE_SPLIT",           # Sprite/Bitmap -> CSS/Results pack
    "STAGE_SPLIT",           # stage-sector data -> stage owner, not W
    "SETUP_TRANSIENT",       # DObjDLLink scaffolding, consumed then dropped
    "PADDING_DROP",          # PAD()
    "STOP",                  # no disposition; verdict invalid until resolved
)

_TEX_INC_RE = re.compile(r"\.tex\.inc\.c")
_PAL_INC_RE = re.compile(r"\.palette\.inc\.c")
_STOCK_LUT_RE = re.compile(r"_stock_luts?$")

# File roles, decided by closure-member basename.
def _file_role(file_name, fighter):
    base = os.path.basename(file_name)
    if "StageSector" in base:
        return "stage"
    if base.endswith("%sModel.c" % fighter):
        return "body_model"
    if base.endswith("Model.c") or "SecondaryImage" in base:
        return "donor_model"
    if "Motion" in base or "Moveset" in base:
        return "motion"
    return "other"


class Assigned(object):
    __slots__ = ("row", "disposition", "reason", "evidence", "role",
                 "costume_index")

    def __init__(self, row, disposition, reason, evidence, role,
                 costume_index=None):
        self.row = row
        self.disposition = disposition
        self.reason = reason
        self.evidence = evidence
        self.role = role
        self.costume_index = costume_index  # resolved palette bank only


def _is_bank(row, ev):
    """Texel/palette bank evidence for a u8/u16 payload object."""
    if ev.includes:
        if any(_TEX_INC_RE.search(i) for i in ev.includes):
            return "texel"
        if any(_PAL_INC_RE.search(i) for i in ev.includes):
            return "palette"
    text = (row.head_comment or "") + " " + row.symbol
    if re.search(r"@tex\b|_Tex\b|_tex\b|tex\.inc", text):
        return "texel"
    if re.search(r"@pal\b|palette|\.palette\.inc", text):
        return "palette"
    return None


def classify_object(row, role, fighter, stock_lut_targets=None):
    """One disposition per object, or STOP.  Never a guess."""
    ev = initializer_evidence(row.init_text)
    lut = stock_lut_targets or {}

    if row.is_pad:
        return Assigned(row, "PADDING_DROP", "file padding", ev, role)

    if role == "stage":
        return Assigned(row, "STAGE_SPLIT",
                        "stage-sector data is owned by the stage pack, "
                        "not the fighter pack", ev, role)

    if row.pointer_depth:
        return Assigned(row, "PTR_TABLE",
                        "relocation pointer array -> u16 refs", ev, role)

    t = row.type_name
    if t in ("Vtx", "Gfx"):
        if role == "body_model":
            return Assigned(row, "NATIVE_REPLACE_BODY",
                            "replaced by the native owner; native root id "
                            "retained", ev, role)
        return Assigned(row, "NATIVE_REPLACE_WEAPON",
                        "donor/special geometry; no native image census "
                        "exists -> charged at raw bytes, UNRESOLVED", ev, role)

    if t == "MObjSub":
        return Assigned(row, "MATERIAL_RECORD",
                        "translated to a compact DS material record", ev, role)

    if t == "DObjDesc":
        return Assigned(row, "RETAINED_JOINT_TREE",
                        "joint tree retained whole at BOTH details; effective "
                        "low-detail selection (null-entry fallback included) "
                        "is a subset by construction", ev, role)

    if t in ("Sprite", "Bitmap"):
        return Assigned(row, "SCENE_SPLIT",
                        "CSS/Results presentation; battle keeps only a tiny "
                        "stock-icon handle", ev, role)

    if t == "DObjDLLink":
        return Assigned(row, "SETUP_TRANSIENT",
                        "display-list link scaffolding consumed into "
                        "per-instance state, then dropped", ev, role)

    if t == "ftMotionCommand" or (t in ("u32", "u16") and role == "motion"
                                  and ev.kind == "macro"):
        return Assigned(row, "MOTION_STREAM",
                        "motion/event words; Profile A keeps today's "
                        "per-instance acquisition, Profile B retains a "
                        "compact bank", ev, role)

    if (t.startswith("FT") or t in ("WPAttributes", "ITAttributes", "Vec3f",
                                    "Vec2h")):
        if t == "FTAttributes":
            reason = ("retained whole; hurtbox defaults are NOT setup-only: "
                      "ftParamResetFighterDamageCollsAll restores "
                      "damage_coll_descs during gameplay")
        elif t == "FTModelPart":
            reason = ("retained whole, including copy-hat rows "
                      "(modelparts_desc_0x39C -> Link boomerang etc.)")
        else:
            reason = "semantic fighter table retained whole"
        return Assigned(row, "RETAINED_SEMANTIC", reason, ev, role)

    if t.startswith("AObjEvent") and ev.kind in ("macro", "numeric"):
        return Assigned(row, "EVENT_STREAM_RETAIN",
                        "animation event program (shield/special/matanim)",
                        ev, role)

    if t in ("u8", "u16"):
        bank = _is_bank(row, ev)
        if bank:
            if "FTEmblem" in row.symbol:
                return Assigned(row, "SCENE_SPLIT",
                                "emblem texel belongs to the Results/CSS "
                                "scene pack, not the battle pack", ev, role)
            key = (row.file_id, row.symbol)
            if key in lut:
                return Assigned(row, "PALETTE_BANK",
                                "stock-icon palette; costume membership "
                                "RESOLVED via the 4-entry stock LUT",
                                ev, role, costume_index=lut[key])
            if "Stock" in row.symbol:
                # the battle stock icon is shared by every costume; only its
                # palette rotates (the stock LUT above)
                return Assigned(row,
                                "TEXEL_BANK" if bank == "texel" else
                                "PALETTE_BANK",
                                "battle stock-icon texel; membership common "
                                "to all costumes (selected for every one), "
                                "VRAM-side", ev, role, costume_index=-1)
            return Assigned(row,
                            "TEXEL_BANK" if bank == "texel" else "PALETTE_BANK",
                            "costume membership not proven from the corpus "
                            "today; charged retained (worst case) and listed "
                            "as unresolved", ev, role)
        if ev.kind in ("numeric", "mixed"):
            return Assigned(row, "CONSERVATIVE_RETAIN",
                            "pointer-free scalar data; consumers unproven, "
                            "retained whole", ev, role)
        return Assigned(row, "STOP",
                        "u8/u16 payload with unrecognized content "
                        "(neither bank evidence nor numeric data)", ev, role)

    if t in ("u32", "u16"):
        # a bare scalar (``u32 sym = 0;``) is a single word; the corpus uses
        # that shape for trailing event End opcodes
        scalar = ev.kind == "none" and row.init_text is not None and bool(
            _NUM_WORD_RE.match(row.init_text.lstrip("=").strip().rstrip(";").strip()))
        if ev.kind == "macro":
            return Assigned(row, "EVENT_STREAM_RETAIN",
                            "animation event program typed %s" % t, ev, role)
        if ev.kind in ("numeric", "symbol", "mixed") or scalar:
            if scalar:
                return Assigned(row, "EVENT_STREAM_RETAIN",
                                "scalar event End opcode word", ev, role)
            return Assigned(row, "CONSERVATIVE_RETAIN",
                            "pointer-free or self-contained scalar table; "
                            "retained whole (pointer words become u16 refs)",
                            ev, role)
        return Assigned(row, "STOP", "unclassifiable %s payload" % t, ev, role)

    if ev.kind in ("numeric", "mixed"):
        return Assigned(row, "CONSERVATIVE_RETAIN",
                        "unknown non-pointer type with scalar initializer; "
                        "retained whole", ev, role)

    return Assigned(row, "STOP",
                    "no disposition rule covers type %r with %s initializer"
                    % (t, ev.kind), ev, role)


def _stock_lut_targets(idx):
    """Resolve the 4-entry stock LUT to per-costume palette bank objects.

    The LUT pointer array is the one place the corpus itself binds costume
    index -> palette bank, so its targets are the only banks whose costume
    membership is resolved today.
    """
    maps = {pf.file_id: OffsetMap(pf) for pf in idx.files}
    by_key = {(o.file_id, o.symbol): o for o in idx.objects}
    targets = {}
    luts = {o.symbol: o for o in idx.objects
            if o.pointer_depth and _STOCK_LUT_RE.search(o.symbol)}
    if not luts:
        return targets
    for pr in idx.relocs:
        for e in pr.unique_edges:
            if e.ptr_symbol not in luts:
                continue
            index = e.ptr_delta // 4
            if e.kind == "intern":
                donor_id = pr.file_id
                src = maps.get(pr.file_id)
                off = (e.target_offset if e.target_symbol is None
                       else (src.offset_of(e.target_symbol,
                                           e.target_delta or 0)
                             if src else None))
            else:
                donor_id = e.donor_file_id
                dmap = maps.get(donor_id)
                off = e.target_offset
                if off is None and e.target_symbol and dmap:
                    off = dmap.offset_of(e.target_symbol, e.target_delta or 0)
            if off is None or donor_id not in maps:
                continue
            owner = maps[donor_id].owner(off)
            if owner is not None and index < COSTUME_COUNT:
                if (donor_id, owner) in by_key:
                    targets[(donor_id, owner)] = index
    return targets


class FighterLedger(object):
    """Stage-2 ledger for one fighter: dispositions, bytes, costumes, stops."""

    def __init__(self, fighter, idx, entry, census, flags_name):
        self.fighter = fighter
        self.idx = idx
        self.entry = entry
        self.census = census.get(fighter, {}) if census else {}
        self.flags_name = flags_name
        self.has_image_slot = fighter in census
        self.assignments = []
        self.stops = []
        self.stock_lut = _stock_lut_targets(idx)

        for pf in idx.files:
            role = _file_role(pf.file_name, fighter)
            for o in pf.objects:
                a = classify_object(o, role, fighter, self.stock_lut)
                self.assignments.append(a)
                if a.disposition == "STOP":
                    self.stops.append(a)

        # motion profile bytes
        self.motion_bytes = sum(a.row.size for a in self.assignments
                                if a.disposition == "MOTION_STREAM")

    # -- per-class aggregation --------------------------------------------

    def class_rows(self):
        rows = OrderedDict()
        for a in self.assignments:
            r = rows.setdefault(a.disposition, OrderedDict(
                disposition=a.disposition, objects=0, source_bytes=0,
                main_ram_bytes=0, replacement_bytes=0))
            r["objects"] += 1
            r["source_bytes"] += a.row.size
            d = a.disposition
            if d in ("RETAINED_JOINT_TREE", "RETAINED_SEMANTIC",
                     "CONSERVATIVE_RETAIN", "EVENT_STREAM_RETAIN"):
                r["main_ram_bytes"] += a.row.size
                # pointer words inside retained tables become u16 refs
                # (joint trees carry their per-joint DL links the same way)
                r["replacement_bytes"] += a.evidence.symbolish * REPL_PTR_REF
            elif d == "MOTION_STREAM":
                pass  # own profile line
            elif d == "NATIVE_REPLACE_BODY":
                # one native root id per display list, not per Gfx word
                r["replacement_bytes"] += (REPL_NATIVE_ROOT_ID
                                           if a.row.type_name == "Gfx" else 0)
            elif d == "NATIVE_REPLACE_WEAPON":
                r["replacement_bytes"] += a.row.size  # unresolved upper bound
            elif d == "MATERIAL_RECORD":
                r["replacement_bytes"] += REPL_MATERIAL_RECORD * a.row.count
            elif d == "PTR_TABLE":
                r["replacement_bytes"] += REPL_PTR_REF * a.row.count
            elif d in ("TEXEL_BANK", "PALETTE_BANK"):
                if a.costume_index is None:
                    r["main_ram_bytes"] += a.row.size  # worst case
                r["replacement_bytes"] += REPL_BANK_HANDLE
            elif d == "SCENE_SPLIT":
                r["replacement_bytes"] += REPL_BANK_HANDLE  # stock handle
            # PADDING_DROP / SETUP_TRANSIENT / STAGE_SPLIT: nothing carried
        return list(rows.values())

    # -- totals -----------------------------------------------------------

    def totals(self):
        retained = removable = replacement = unresolved_member = 0
        weapon_native = 0
        for r in self.class_rows():
            d = r["disposition"]
            if d == "MOTION_STREAM":
                continue
            retained += r["main_ram_bytes"]
            replacement += r["replacement_bytes"]
            if d in ("PADDING_DROP", "SETUP_TRANSIENT", "SCENE_SPLIT",
                     "STAGE_SPLIT", "PTR_TABLE", "MATERIAL_RECORD",
                     "NATIVE_REPLACE_BODY"):
                removable += r["source_bytes"]
            elif d in ("TEXEL_BANK", "PALETTE_BANK"):
                # unresolved banks stay in main RAM (counted there); resolved
                # ones leave main RAM for a VRAM handle
                removable += r["source_bytes"] - r["main_ram_bytes"]
            elif d == "NATIVE_REPLACE_WEAPON":
                removable += r["source_bytes"]
                weapon_native += r["source_bytes"]
        # native image census: RESIDENT, charged once per kind
        if self.has_image_slot:
            census_both = self.census.get("High", 0) + self.census.get("Low", 0)
            census_low = self.census.get("Low", 0)
        else:
            census_both = census_low = 0
        replacement += census_both + REPL_PACK_HEADER
        unresolved_member = sum(
            r["main_ram_bytes"] for r in self.class_rows()
            if r["disposition"] in ("TEXEL_BANK", "PALETTE_BANK"))
        stop_bytes = sum(r["source_bytes"] for r in self.class_rows()
                         if r["disposition"] == "STOP")
        bank_count = sum(r["objects"] for r in self.class_rows()
                         if r["disposition"] in ("TEXEL_BANK", "PALETTE_BANK"))
        # optimistic bound: every unresolved bank resolves to
        # "selected costume -> VRAM", leaving only handles in main RAM
        retained_vram = retained - unresolved_member
        w_worst = retained + replacement
        w_vram = retained_vram + replacement
        return OrderedDict(
            indexed_bytes=sum(o.size for o in self.idx.objects),
            retained=retained,
            removable=removable,
            replacement=replacement,
            unresolved_membership=unresolved_member,
            unresolved_weapon_native=weapon_native,
            bank_count=bank_count,
            native_census_both=int(census_both),
            native_census_low=int(census_low),
            native_owner_static=not self.has_image_slot,
            w_profile_a_worst=w_worst,
            w_profile_a_vram=w_vram,
            w_profile_b_worst=w_worst + self.motion_bytes,
            w_profile_b_vram=w_vram + self.motion_bytes,
            motion_bytes=self.motion_bytes,
            stop_count=len(self.stops),
            stop_bytes=stop_bytes,
            # every indexed byte is accounted for exactly once:
            # retained + removable + motion + stop == indexed
            reconciles=(retained + removable + self.motion_bytes + stop_bytes
                        == sum(o.size for o in self.idx.objects)),
        )

    def costume_rows(self):
        """Per-costume ledger: only resolved costume atoms differ today."""
        base = self.totals()
        rows = []
        resolved = [a for a in self.assignments if a.costume_index is not None]
        for c in range(COSTUME_COUNT):
            t = dict(base)
            sel = [a for a in resolved
                   if a.costume_index in (c, -1)]
            t["costume"] = c
            t["resolved_banks_selected"] = len(sel)
            t["resolved_banks_vram_bytes"] = sum(a.row.size for a in sel)
            t["w_profile_a_worst"] = base["w_profile_a_worst"]
            rows.append(t)
        return rows

    # -- atom maps for set enumeration ------------------------------------

    def atom_keep_bytes(self):
        """{(file_id, symbol): (worst_main_ram, vram_main_ram, motion)}."""
        atoms = {}
        for a in self.assignments:
            key = (a.row.file_id, a.row.symbol)
            d = a.disposition
            size = a.row.size
            if d in ("RETAINED_JOINT_TREE", "RETAINED_SEMANTIC",
                     "CONSERVATIVE_RETAIN", "EVENT_STREAM_RETAIN"):
                keep = size
                extra = a.evidence.symbolish * REPL_PTR_REF
                atoms[key] = (keep + extra, keep + extra, 0)
            elif d in ("TEXEL_BANK", "PALETTE_BANK"):
                if a.costume_index is None:
                    atoms[key] = (size + REPL_BANK_HANDLE,
                                  REPL_BANK_HANDLE, 0)
                else:
                    atoms[key] = (REPL_BANK_HANDLE, REPL_BANK_HANDLE, 0)
            elif d == "MOTION_STREAM":
                atoms[key] = (0, 0, size)
            elif d == "NATIVE_REPLACE_WEAPON":
                atoms[key] = (size, size, 0)
            elif d == "MATERIAL_RECORD":
                atoms[key] = (REPL_MATERIAL_RECORD * a.row.count,) * 2 + (0,)
            elif d == "PTR_TABLE":
                atoms[key] = (REPL_PTR_REF * a.row.count,) * 2 + (0,)
            elif d == "NATIVE_REPLACE_BODY":
                # one native root id per display list, not per Gfx word
                if a.row.type_name == "Gfx":
                    atoms[key] = (REPL_NATIVE_ROOT_ID, REPL_NATIVE_ROOT_ID, 0)
                else:
                    atoms[key] = (0, 0, 0)
            elif d == "SCENE_SPLIT":
                atoms[key] = (REPL_BANK_HANDLE,) * 2 + (0,)
            else:  # PADDING_DROP, SETUP_TRANSIENT, STAGE_SPLIT, STOP
                atoms[key] = (0, 0, 0)
        return atoms

    def per_kind_fixed(self):
        t = self.totals()
        return (t["native_census_both"] + REPL_PACK_HEADER)


def build_fighter_ledgers(fighters, types, flags_name="hwtri", census=None):
    if census is None:
        census = parse_native_image_census()
    ledgers = OrderedDict()
    for f in fighters:
        idx, entry = index_closure(f, types)
        ledgers[f] = FighterLedger(f, idx, entry, census, flags_name)
    return ledgers


# --------------------------------------------------------------------------
# Set enumeration and verdict
# --------------------------------------------------------------------------

def _combinations(n, r):
    import itertools
    return itertools.combinations(range(n), r)


def enumerate_sets(ledgers, profile="a_worst"):
    """All one-through-four-kind sets over the closable fighters.

    Atoms are canonical by (file_id, symbol), so a donor file named by two
    fighters is counted once (review section 4.2).  Returns
    (count_target, sets) where sets is [(fighter_tuple, W)] sorted by W desc.
    """
    names = list(ledgers)
    atoms = {f: ledgers[f].atom_keep_bytes() for f in names}
    fixed = {f: ledgers[f].per_kind_fixed() for f in names}
    stop_any = any(l.stops for l in ledgers.values())

    def set_w(kinds, use_vram, with_motion):
        # A shared atom (e.g. 338_YoshiModel is both Yoshi's body model and
        # Kirby's copy-hat donor) gets one representation at runtime; the
        # conservative union charges each atom the element-wise MAX across
        # the kinds that name it, so the result never depends on kind order.
        merged = {}
        for f in kinds:
            for k, v in atoms[f].items():
                cur = merged.get(k)
                if cur is None:
                    merged[k] = v
                else:
                    merged[k] = tuple(max(a, b) for a, b in zip(cur, v))
        idx = 1 if use_vram else 0
        total = sum(v[idx] for v in merged.values())
        if with_motion:
            total += sum(v[2] for v in merged.values())
        total += sum(fixed[f] for f in kinds)
        return total

    out = []
    n = len(names)
    for r in range(1, min(4, n) + 1):
        for combo in _combinations(n, r):
            kinds = tuple(names[i] for i in combo)
            out.append((kinds, set_w(kinds, False, profile == "b_worst")))
    out.sort(key=lambda kv: -kv[1])
    target = sum(_nCr(n, r) for r in range(1, min(4, n) + 1))
    return target, out, stop_any


def _nCr(n, r):
    import math
    return math.comb(n, r)


def verdict_for(w_worst, w_vram, stop_count):
    """RED/YELLOW/GREEN/UNKNOWN/STOP against the review's gate table."""
    if stop_count:
        return "STOP", "unclassified objects exist; no size verdict is valid"
    if w_worst <= GREEN_BAND:
        return "GREEN", "worst pack <= 150 KiB; build the runtime proof"
    if w_vram > W_CEILING:
        return "RED", ("even the optimistic bound exceeds 175,604 - "
                       "D_other - D_binder")
    if w_worst <= W_CEILING:
        return "YELLOW", ("fits the optimistic cap only; depends on explicit "
                          "secondary recovery and on unresolved costume "
                          "membership resolving favorably")
    return "UNKNOWN", ("the unresolved band straddles the ceiling: "
                       "W_vram <= cap < W_worst; resolve bank membership "
                       "before trusting either side")


def ledger_report(ledgers, census, flags_name):
    out = []
    w = out.append
    w("=" * 78)
    w("Semantic pack estimator, stage 2 -- disposition ledger")
    w("native image census flags: %s (owner-played hwtri configuration)"
      % flags_name)
    w("=" * 78)
    w("")
    w("Verified constants: F_new = 208,372 - W - D_other - D_binder;")
    w("                      W <= 175,604 - D_other - D_binder.")
    w("D_other and D_binder remain named unknowns (measured by the four-slot")
    w("skeleton build the spec requires; this tool never zeroes them).")
    w("")

    total_stops = 0
    for f, led in ledgers.items():
        t = led.totals()
        total_stops += t["stop_count"]
        w("--- %s ----------------------------------------------------" % f)
        w("indexed %d B over %d closure files" % (t["indexed_bytes"],
                                                  len(led.idx.files)))
        w("%-24s %8s %12s %12s %12s" %
          ("disposition", "objects", "source_B", "main_ram_B", "repl_B"))
        for r in led.class_rows():
            w("%-24s %8d %12d %12d %12d" %
              (r["disposition"], r["objects"], r["source_bytes"],
               r["main_ram_bytes"], r["replacement_bytes"]))
        w("")
        w("retained      : %10d   (kept in the main-RAM pack)" % t["retained"])
        w("removable     : %10d   (not carried by the pack)" % t["removable"])
        w("replacement   : %10d   (compact records + native census + header)"
          % t["replacement"])
        w("  of which native image census (RESIDENT): %d  [%s]"
          % (t["native_census_both"],
             "high+low, both until the low-only invariant lands"
             if led.has_image_slot else
             "no image slot: owner is linked into the measured ARM9 "
             "baseline (not charged here; do not double-count)"))
        w("unresolved    : %10d   costume membership of texel/palette banks"
          % t["unresolved_membership"])
        w("unresolved    : %10d   weapon/donor native translation, charged at"
          % t["unresolved_weapon_native"])
        w("                 raw bytes (upper bound; never zeroed)")
        w("motion        : %10d   (Profile A excludes / Profile B retains)"
          % t["motion_bytes"])
        w("W profile A   : worst %d   vram-bound %d"
          % (t["w_profile_a_worst"], t["w_profile_a_vram"]))
        w("W profile B   : worst %d   vram-bound %d"
          % (t["w_profile_b_worst"], t["w_profile_b_vram"]))
        w("reconciliation: retained + removable + motion + stop == indexed : "
          "%s (%d + %d + %d + %d == %d)"
          % ("OK" if t["reconciles"] else "FAIL", t["retained"],
             t["removable"], t["motion_bytes"], t["stop_bytes"],
             t["indexed_bytes"]))
        crows = led.costume_rows()
        w("per costume   : " + "; ".join(
            "c%d W=%d (selected stock banks VRAM %d B)"
            % (c["costume"], c["w_profile_a_worst"],
               c["resolved_banks_vram_bytes"]) for c in crows))
        if t["stop_count"]:
            w("STOPS: %d" % t["stop_count"])
            for a in led.stops:
                w("  %s:%d %s (%s): %s" % (a.row.file_name, a.row.line,
                                           a.row.symbol, a.row.type_name,
                                           a.reason))
        w("")

    # ---- set enumeration -------------------------------------------------
    target, sets, stop_any = enumerate_sets(ledgers, profile="a_worst")
    worst_kinds, worst_w = sets[0]
    four = [s for s in sets if len(s[0]) == 4]
    four_kinds, four_w = four[0] if four else (None, None)
    w("--- set enumeration ---------------------------------------------------")
    w("closable fighters today: %d -> %d one-through-four-kind sets "
      "(12 fighters would give 793)" % (len(ledgers), len(sets)))
    worst_vram, worst_vram_kinds = enumerate_sets_vram_worst(ledgers)
    v, reason = verdict_for(worst_w, worst_vram, total_stops)
    w("worst set (any size)   : %s  W_A_worst = %d B"
      % ("+".join(worst_kinds), worst_w))
    w("worst set under the vram bound: %s  W = %d B"
      % ("+".join(worst_vram_kinds), worst_vram))
    if four_kinds:
        w("worst exactly-four set : %s  W_A_worst = %d B"
          % ("+".join(four_kinds), four_w))
    w("worst costume combination: all four costumes of every kind present")
    w("  (mirrors); only stock palettes are costume-resolved today, so the")
    w("  per-costume spread is %d B of VRAM, invisible to main-RAM W"
      % max(abs(c1["resolved_banks_vram_bytes"] - c2["resolved_banks_vram_bytes"])
           for f, led in ledgers.items()
           for c1 in led.costume_rows() for c2 in led.costume_rows()))
    w("")
    w("--- verdict ------------------------------------------------------------")
    band, band_reason = verdict_for(worst_w, worst_vram, 0)
    w("verdict: %s -- %s" % (v, reason))
    if v == "STOP":
        w("provisional band if every STOP were resolved: %s -- %s"
          % (band, band_reason))
    w("F_new = 208,372 - %d - D_other - D_binder" % worst_w)
    w("32 KiB floor holds iff D_other + D_binder <= %d" % (W_CEILING - worst_w))
    if v == "STOP":
        w("resolve every STOP above; the byte figures stay provisional.")
    return "\n".join(out)


def enumerate_sets_vram_worst(ledgers):
    """(max W under the vram bound, the set that achieves it)."""
    atoms = {f: ledgers[f].atom_keep_bytes() for f in ledgers}
    fixed = {f: ledgers[f].per_kind_fixed() for f in ledgers}
    names = list(ledgers)
    best = None
    best_kinds = None
    import itertools
    for r in range(1, min(4, len(names)) + 1):
        for combo in itertools.combinations(range(len(names)), r):
            merged = {}
            for i in combo:
                f = names[i]
                for k, val in atoms[f].items():
                    cur = merged.get(k)
                    if cur is None:
                        merged[k] = val
                    else:
                        merged[k] = tuple(max(a, b) for a, b in zip(cur, val))
            total = sum(v[1] for v in merged.values())
            total += sum(fixed[names[i]] for i in combo)
            if best is None or total > best:
                best = total
                best_kinds = tuple(names[i] for i in combo)
    return best, best_kinds


def build_ledger_json(ledgers, census, flags_name):
    doc = OrderedDict()
    doc["schema"] = "smash64ds.pack_estimator.disposition_ledger.v1"
    doc["stage"] = 2
    doc["spec"] = "docs/p2/P2-2-pack-estimator.md"
    doc["constants"] = OrderedDict(
        f_new_base=F_NEW_BASE, floor_bytes=FLOOR_BYTES,
        w_ceiling=W_CEILING, green_band=GREEN_BAND,
        costume_count=COSTUME_COUNT)
    doc["native_image_flags"] = flags_name
    doc["native_image_census"] = OrderedDict(
        (f, OrderedDict(high=census[f].get("High"), low=census[f].get("Low")))
        for f in sorted(census))
    doc["fighters"] = []
    for f, led in ledgers.items():
        doc["fighters"].append(OrderedDict(
            fighter=f,
            closure_files=len(led.idx.files),
            classes=led.class_rows(),
            totals=led.totals(),
            costumes=led.costume_rows(),
            stops=[OrderedDict(file=a.row.file_name, line=a.row.line,
                               symbol=a.row.symbol, type=a.row.type_name,
                               reason=a.reason) for a in led.stops],
        ))
    target, sets, stop_any = enumerate_sets(ledgers)
    worst_vram, worst_vram_kinds = enumerate_sets_vram_worst(ledgers)
    four_sets = [s for s in sets if len(s[0]) == 4]
    doc["set_enumeration"] = OrderedDict(
        closable_fighters=list(ledgers),
        sets_expected=target, sets_computed=len(sets),
        worst_set=list(sets[0][0]), worst_set_w_a_worst=sets[0][1],
        worst_set_vram_bound=worst_vram,
        worst_vram_bound_set=list(worst_vram_kinds),
        worst_four_set=list(four_sets[0][0]) if four_sets else None,
        worst_four_set_w=four_sets[0][1] if four_sets else None)
    worst_w = sets[0][1]
    v, reason = verdict_for(worst_w, worst_vram,
                            sum(l.totals()["stop_count"] for l in
                                ledgers.values()))
    doc["verdict"] = OrderedDict(
        verdict=v, reason=reason, worst_w_a_worst=worst_w,
        f_new="208372 - %d - D_other - D_binder" % worst_w,
        d_other_plus_d_binder_allowance=W_CEILING - worst_w)
    return doc


# ==========================================================================
# main
# ==========================================================================


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--fighter", default=None,
                    help="stage 1: fighter whose relocData closure to index "
                         "(default: Kirby).  Stage 2 (--ledger): restrict the "
                         "ledger to one fighter instead of every closable one")
    ap.add_argument("--file", default=None,
                    help="restrict to one relocData .c basename, e.g. 328_KirbyModel.c")
    ap.add_argument("--ledger", action="store_true",
                    help="stage 2: build the disposition ledger, enumerate "
                         "the one-through-four-kind sets, and print the "
                         "go/no-go verdict")
    ap.add_argument("--native-flags", default="hwtri",
                    choices=("hwtri", "base"),
                    help="generated native image census configuration "
                         "(default: hwtri, the owner-played family)")
    ap.add_argument("--json", default=None, help="write the full object index here")
    ap.add_argument("--all", action="store_true", help="do not truncate long lists")
    ap.add_argument("--region", default="US", choices=("US", "JP"),
                    help="which region's branches to compile (default: US, "
                         "matching the manifest's data_bytes)")
    ap.add_argument("--strict", action="store_true",
                    help="exit non-zero if anything was refused or disagreed")
    ap.add_argument("--no-layout-dump", action="store_true",
                    help="omit the sizeof/alignof table for the types used")
    args = ap.parse_args(argv)

    if not os.path.isdir(RELOCDATA_DIR):
        raise SystemExit("relocData not found at %s (fetch the BattleShip reference first)"
                         % RELOCDATA_DIR)
    if not os.path.isfile(MANIFEST_PATH):
        raise SystemExit("missing %s" % MANIFEST_PATH)

    types = TypeTable()
    types.load_dirs(HEADER_DIRS)

    if args.ledger:
        if args.file:
            raise SystemExit("--file is a stage-1 restriction; it cannot "
                             "combine with --ledger")
        manifest = load_manifest()
        fighters = ([args.fighter] if args.fighter
                    else [f["fighter"] for f in manifest["fighters"]])
        census = parse_native_image_census()
        ledgers = build_fighter_ledgers(fighters, types, args.native_flags,
                                        census)
        print(ledger_report(ledgers, census, args.native_flags))
        if args.json:
            with open(args.json, "w", encoding="utf-8") as fh:
                json.dump(build_ledger_json(ledgers, census, args.native_flags),
                          fh, indent=1)
            print("")
            print("wrote %s" % args.json)
        if args.strict and any(l.idx.refused or l.idx.disagreements or l.stops
                               for l in ledgers.values()):
            return 1
        return 0

    fighter = args.fighter or "Kirby"
    idx, entry = index_closure(fighter, types, only_file=args.file,
                               region=args.region)
    if not args.no_layout_dump:
        declared = {o.type_name for o in idx.objects}
        print(layout_dump(types, declared))
        print("")
    print(report(idx, entry, args, types))

    if args.json:
        with open(args.json, "w", encoding="utf-8") as fh:
            json.dump(build_json(idx, entry), fh, indent=1)
        print("")
        print("wrote %s" % args.json)

    if args.strict and (idx.refused or idx.disagreements):
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
