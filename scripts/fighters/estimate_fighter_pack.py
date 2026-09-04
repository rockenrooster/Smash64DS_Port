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
        "size_source", "notes",
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

    walk = 0
    walk_valid = True

    def refuse(line, form, reason, text):
        """Record a construct this parser declines to interpret.

        A refusal also drops any pending offset anchor and invalidates the
        walk: an anchor that survives an unparsed declaration silently lands
        on the NEXT object, turning one refusal into a drift of every offset
        after it.
        """
        nonlocal pending_anchor, walk_valid
        pf.refused.append(RefusedRow(pf.file_name, line, form, reason, text))
        pending_anchor = None
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
                continue
            m = ANCHOR_RE.search(c.text)
            if m:
                size_hint = None
                ms = ANCHOR_BYTES_RE.search(c.text)
                if ms:
                    size_hint = int(ms.group(1))
                pending_anchor = (int(m.group(1), 16), size_hint, c.line)

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
            offset, offset_source, cands = _reconcile(anchor_off, None,
                                                      walk if walk_valid else None)
            row = ObjectRow(symbol="_relocdata_pad_%d" % line, type_name="u8",
                            pointer_depth=0, count=nbytes, elem_size=1, size=nbytes,
                            size_source="PAD", file_id=file_id, file_name=pf.file_name,
                            line=line, is_pad=True, walk_offset=walk if walk_valid else None,
                            anchor_offset=anchor_off, anchor_size=anchor_size,
                            offset=offset, offset_source=offset_source, notes=["pad"])
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
                        notes=[count_source])

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
# main
# ==========================================================================


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--fighter", default="Kirby",
                    help="fighter whose relocData closure to index (default: Kirby)")
    ap.add_argument("--file", default=None,
                    help="restrict to one relocData .c basename, e.g. 328_KirbyModel.c")
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

    idx, entry = index_closure(args.fighter, types, only_file=args.file,
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
