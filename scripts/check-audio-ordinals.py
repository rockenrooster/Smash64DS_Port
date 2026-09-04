#!/usr/bin/env python3
"""Check every audio ordinal in include/gm/gmsound.h against the decomp enum.

BattleShip's gmsound.h declares its sound IDs positionally: a long enum with no
explicit values, where a cue's ordinal is its index.  This port cannot copy that
enum -- it carries only the cues landed content actually asks for -- so each one
is written out by hand as `name = <number>`.

That number is derived by counting enum members in a file of about nine hundred
of them, and a wrong count is silent: the ROM plays a different sound, or none,
and nothing anywhere says so.  The count also has to skip the JP arm of every
`#if defined(REGION_US)` block, because this port builds -DREGION_US.

So count them here instead.  This script parses the decomp enum, resolves every
member's ordinal the way the compiler would, and asserts the port agrees on
every name it declares.  It is the same reasoning as
scripts/items/check-item-import-fidelity.py: the question is mechanical, so ask
it mechanically rather than trusting that it was answered carefully.

Exit status is non-zero if any ordinal disagrees or any name is unknown to the
decomp, so this can gate a batch.
"""
import io
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DECOMP = os.path.join(ROOT, 'decomp', 'BattleShip-main', 'decomp', 'src', 'gm',
                      'gmsound.h')
PORT = os.path.join(ROOT, 'include', 'gm', 'gmsound.h')

MEMBER = re.compile(r'^(n[A-Za-z0-9_]+)\s*(?:=\s*([0-9xXa-fA-F]+))?\s*,')
PORT_MEMBER = re.compile(r'^(nSYAudio[A-Za-z0-9_]+)\s*=\s*(\d+)\s*,')


def read(path):
    return io.open(path, encoding='utf-8', errors='replace').read()


def decomp_ordinals():
    """Every enum member in the decomp header, mapped to its value.

    The two things that make this more than a line count: an explicit `= N`
    re-bases everything after it, and a member is only counted if this build's
    region selects it.  This port is -DREGION_US, so the #else arm of a
    REGION_US block is skipped -- its members do not exist here and would
    otherwise shift every ordinal after them.
    """
    value = None
    ordinals = {}
    # One entry per open #if: True emit, False skip, None unrelated (emit).
    conditions = []

    for line in read(DECOMP).splitlines():
        text = line.strip()
        if text.startswith('enum') or text == '{':
            value = None
        if text.startswith('#if'):
            if 'REGION_US' in text and not text.startswith('#ifndef'):
                conditions.append(True)
            else:
                conditions.append(None)
            continue
        if text.startswith('#else'):
            if conditions and conditions[-1] is not None:
                conditions[-1] = not conditions[-1]
            continue
        if text.startswith('#endif'):
            if conditions:
                conditions.pop()
            continue
        if any(c is False for c in conditions):
            continue
        match = MEMBER.match(text)
        if match is None:
            continue
        if match.group(2) is not None:
            value = int(match.group(2), 0)
        else:
            value = 0 if value is None else value + 1
        ordinals[match.group(1)] = value
    return ordinals


def main():
    ordinals = decomp_ordinals()
    if len(ordinals) < 500:
        print('parsed only %d enum members from %s -- the header moved or its '
              'shape changed, and a partial parse would pass everything'
              % (len(ordinals), os.path.relpath(DECOMP, ROOT)))
        return 1

    failures = 0
    checked = 0
    for number, line in enumerate(read(PORT).splitlines(), 1):
        match = PORT_MEMBER.match(line.strip())
        if match is None:
            continue
        name, declared = match.group(1), int(match.group(2))
        expected = ordinals.get(name)
        if expected is None:
            print('%s:%d: %s is not an enum member of the decomp gmsound.h'
                  % (os.path.relpath(PORT, ROOT), number, name))
            failures += 1
        elif expected != declared:
            print('%s:%d: %s is %d here, but the decomp enum puts it at %d'
                  % (os.path.relpath(PORT, ROOT), number, name, declared,
                     expected))
            failures += 1
        else:
            checked += 1

    print('%d audio ordinals verified against the decomp enum' % checked)
    if failures:
        print('%d audio ordinal problem(s)' % failures)
        return 1
    print('audio ordinals clean')
    return 0


if __name__ == '__main__':
    sys.exit(main())
