#!/usr/bin/env python3
"""Check imported item translation units against the decomp source.

Three questions, all mechanical, all cheap, and all otherwise answered by
reading a few thousand lines of agent-written C by eye:

  1. Does every reloc offset a TU owns match decomp/BattleShip-main/include/
     reloc_data.us.h?  A wrong offset is not a compile error and not a link
     error; it is a wild pointer that reaches a load.  Peach's Castle and
     Planet Zebes both died that way.

  2. Does every numeric literal in the TU appear somewhere in the decomp source
     it claims to be a verbatim adaptation of?  "No invented constants" is the
     standing rule for these imports and it is checkable rather than promised.

  3. Does include/it/item.h define any tuning macro twice?  itvars.h wraps a
     handful of them in #if defined(REGION_US), and when that block was
     transcribed the guard was dropped: ITPKFIRE_GRAVITY and ITPKFIRE_TVEL
     landed as both the US pair and the JP pair, back to back, and the JP
     values won every redefinition.  Nothing complains -- it is a legal
     redefinition warning at most, and it changed PK Fire's gravity.

Question 2 is deliberately loose: it proves a literal EXISTS in the source, not
that it is used in the right place.  It catches an invented number, which is the
failure that matters here, and it does not pretend to catch a transposed one.

Exit status is non-zero if anything fails, so this can gate a batch.
"""
import glob
import io
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
DECOMP = os.path.join(ROOT, 'decomp', 'BattleShip-main')
AUTHORITY = os.path.join(DECOMP, 'include', 'reloc_data.us.h')
SRC = os.path.join(DECOMP, 'decomp', 'src', 'it')

# Literals too common to carry information; flagging them would bury the signal.
TRIVIAL = {0.0, 1.0, 2.0, 3.0, 4.0, 8.0, 10.0, 16.0, 32.0, 100.0, 255.0}
NUM = re.compile(r'(?<![\w.])(?:0[xX][0-9a-fA-F]+|\d+\.\d+[Ff]?|\d+)(?![\w.])')


ITEM_HEADER = os.path.join(ROOT, 'include', 'it', 'item.h')

# The one legitimate reason to see a name twice: an include guard's own #define
# sits beside no second definition, and a #ifndef X / #define X pair is the
# ordinary way to make a macro overridable.
GUARDED = re.compile(r'^\s*#\s*(?:ifndef|if\s+!\s*defined)\b')
DEFINE = re.compile(r'^\s*#\s*define\s+([A-Za-z_]\w*)')


def read(path):
    return io.open(path, encoding='utf-8', errors='replace').read()


ITEM_SWITCH_SOURCE = os.path.join(SRC, 'mnvsitemswitch.c')
SWITCH_ROWS_PORT = os.path.join(ROOT, 'src', 'port', 'nds_match_config.c')


def check_item_switch_row_order():
    """The Item Switch screen's fifteen rows, in the source's order.

    The port carries this table twice removed from its source -- a different
    file, a different type, and the leading appearance-rate entry dropped -- so
    a transposed or missing kind is invisible to the compiler and shows up as
    the wrong item toggling the wrong row.  It is a list of names, so compare
    the lists.
    """
    decomp_source = read(os.path.join(
        os.path.dirname(SRC), 'mn', 'mnvsmode', 'mnvsitemswitch.c'))
    match = re.search(r'dMNVSItemSwitchTogglesItemKinds\[[^\]]*\]\s*=\s*\{'
                      r'(.*?)\}', decomp_source, re.S)
    if match is None:
        print('could not find dMNVSItemSwitchTogglesItemKinds in the decomp '
              'item switch screen -- the source moved')
        return 1
    # Entry 0 is the appearance rate, not a kind; the port's table starts at 1.
    expected = re.findall(r'nITKind\w+', match.group(1))

    port = read(SWITCH_ROWS_PORT)
    match = re.search(r'kNdsItemSwitchToggleKinds\[[^\]]*\]\s*=\s*\{(.*?)\}',
                      port, re.S)
    if match is None:
        print('%s: kNdsItemSwitchToggleKinds is missing'
              % os.path.relpath(SWITCH_ROWS_PORT, ROOT))
        return 1
    actual = re.findall(r'nITKind\w+', match.group(1))

    if actual != expected:
        print('%s: the item switch rows do not match the source. source %s, '
              'port %s' % (os.path.relpath(SWITCH_ROWS_PORT, ROOT),
                           ', '.join(expected), ', '.join(actual)))
        return 1
    return 0


GLOB_CLOSES_COMMENT = re.compile(r'\w\*/')


def check_no_glob_closed_comments():
    """A macro glob written in a block comment closes the comment.

    Every one of these TUs opens with a provenance comment, and the natural way
    to write "the ITNYARS_ and ITMONSTER_ tuning" is ITNYARS_*/ITMONSTER_* --
    which contains */ and ends the comment on the spot.  Everything below,
    including the whole extern block, then parses as code and the errors point
    at the declarations rather than at the comment.

    It has cost a build round-trip three times: itstarrod's syUtils*/syVector*,
    then five monster files at once.  The signature is exact -- a word
    character immediately followed by */ -- and it never occurs in code, since
    a real comment terminator is preceded by a space or a star.
    """
    failures = 0
    for path in sorted(glob.glob(os.path.join(ROOT, 'src', 'import',
                                              'battleship_item_*.c'))):
        for number, line in enumerate(read(path).splitlines(), 1):
            if GLOB_CLOSES_COMMENT.search(line):
                print('%s:%d: a macro glob ends this line with */, which '
                      'closes the block comment early. Write the names out '
                      '("ITNYARS_ and ITMONSTER_") instead of globbing them.'
                      % (os.path.relpath(path, ROOT), number))
                failures += 1
    return failures


def check_no_redefined_macros():
    """A tuning macro defined twice keeps the LAST value, silently.

    This is not a style check.  decomp/src/it/itvars.h selects between US and
    JP tuning with #if defined(REGION_US); a transcription that drops the guard
    lands both arms and the JP values win, which is a gameplay change with no
    diagnostic attached to it.
    """
    lines = read(ITEM_HEADER).splitlines()
    seen = {}
    failures = 0

    for i, line in enumerate(lines):
        match = DEFINE.match(line)
        if match is None:
            continue
        name = match.group(1)
        if i > 0 and GUARDED.match(lines[i - 1]):
            continue
        if name in seen:
            print('%s:%d: %s was already defined at line %d -- the later '
                  'value wins silently. If the decomp guards these with '
                  '#if defined(REGION_US), keep only the US arm.'
                  % (os.path.relpath(ITEM_HEADER, ROOT), i + 1, name,
                     seen[name] + 1))
            failures += 1
        else:
            seen[name] = i
    return failures


def literals(text):
    text = re.sub(r'/\*.*?\*/', '', text, flags=re.S)
    text = re.sub(r'//[^\n]*', '', text)
    out = []
    for m in NUM.finditer(text):
        tok = m.group(0)
        try:
            if tok.lower().startswith('0x'):
                out.append((tok, float(int(tok, 16))))
            else:
                out.append((tok, float(tok.rstrip('fF'))))
        except ValueError:
            pass
    return out


def source_for(port_path):
    """The decomp TU a port file claims to adapt, from its own header comment."""
    head = read(port_path)[:4000]
    m = re.search(r'src/it/(\w+)/(\w+\.c)', head)
    if m:
        cand = os.path.join(SRC, m.group(1), m.group(2))
        if os.path.exists(cand):
            return cand
    return None


def main():
    authority = {}
    for m in re.finditer(r'#define\s+(ll\w+)\s+\(\(intptr_t\)(0[xX][0-9a-fA-F]+|\d+)\)',
                         read(AUTHORITY)):
        authority[m.group(1)] = int(m.group(2), 0)

    # Shared tables every item may legitimately draw a constant from.
    shared = ''
    for extra in ('itvars.h', 'itmain.c', 'itmanager.c', 'itmap.c', 'itdef.h'):
        path = os.path.join(SRC, extra)
        if os.path.exists(path):
            shared += read(path)
    shared += read(AUTHORITY)

    failures = 0
    checked_offsets = 0
    for path in sorted(glob.glob(os.path.join(ROOT, 'src', 'import',
                                              'battleship_item_*.c'))):
        name = os.path.basename(path)
        text = read(path)

        for m in re.finditer(r'uintptr_t\s+(ll\w+)\s*=\s*(0[xX][0-9a-fA-F]+|\d+)u?\s*;',
                             text):
            sym, val = m.group(1), int(m.group(2), 0)
            if sym not in authority:
                print('%s: %s is not in reloc_data.us.h' % (name, sym))
                failures += 1
            elif authority[sym] != val:
                print('%s: %s is %#x, authority says %#x'
                      % (name, sym, val, authority[sym]))
                failures += 1
            else:
                checked_offsets += 1

        src = source_for(path)
        if src is None:
            continue
        known = set(v for _, v in literals(read(src) + shared))
        untraced = sorted({t for t, v in literals(text)
                           if v not in known and v not in TRIVIAL})
        if untraced:
            print('%s: literals not found in %s: %s'
                  % (name, os.path.basename(src), ', '.join(untraced[:12])))
            failures += 1

    failures += check_no_glob_closed_comments()
    failures += check_no_redefined_macros()
    failures += check_item_switch_row_order()

    print('%d reloc offsets verified against reloc_data.us.h' % checked_offsets)
    if failures:
        print('%d item import problem(s)' % failures)
        return 1
    print('item imports clean')
    return 0


if __name__ == '__main__':
    sys.exit(main())
