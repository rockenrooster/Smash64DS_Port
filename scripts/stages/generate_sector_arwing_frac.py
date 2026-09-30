#!/usr/bin/env python3
"""Sector Z Arwing flight table: syInterpGetFracFrame results by (segment, t).

The Arwing flies one of eight authored patterns (grsector.c
dGRSectorArwingSectorDescs), each started from frame 0 by grSectorArwingAddAnim,
so every flight of a pattern makes the same (segment, t) calls in the same
order. A NDS_INTERP_FRAC_CAPTURE lab ROM records each call's segment key (the
memo's h1/h2 over kind, point count, segment id, length, the segment's two
keyframes and five quartic coefficients), t and result; this script merges one
or more capture dumps into the table the runtime consults before computing.

A table entry is only ever used when the runtime's own (h1, h2, t) matches it
bit for bit, and every entry is the result the source computed for exactly
that input on the device, so the table cannot change a result; a call it does
not hold is computed as before.

Inputs: capture dumps (4096 x 4 little-endian u32: h1, h2, t_bits, frac_bits)
with their gdb logs (FLIGHT_START / FLIGHT_END / DONE lines give the count).

Output format (little-endian u32 words):
  0  magic 'ARWF' (0x46575241)
  1  version (1)
  2  segment count S
  3  entry count N
  then S x {h1, h2, first_entry, entry_count}
  then N x {t_bits, frac_bits}, each segment's entries sorted by t_bits
"""
import argparse
import pathlib
import re
import struct
import sys

MAGIC = 0x46575241
VERSION = 1
CAPTURE_MAX = 4096


def load_capture(dump_path, log_path):
    raw = pathlib.Path(dump_path).read_bytes()
    words = struct.unpack('<%dI' % (len(raw) // 4), raw)
    count = None
    for line in pathlib.Path(log_path).read_text(errors='replace').splitlines():
        m = re.search(r'(?:FLIGHT_END \d+|DONE) count=(\d+)', line)
        if m:
            count = int(m.group(1))
    if count is None:
        raise SystemExit(f'{log_path}: no FLIGHT_END/DONE count')
    capacity = len(words) // 4
    if count > capacity:
        # Every slot the buffer holds is still a real call; the calls past it
        # are simply absent from the table and are computed at run time.
        print(f'{log_path}: {count} calls, buffer holds {capacity}; '
              f'{count - capacity} left out')
        count = capacity
    return [tuple(words[i * 4:i * 4 + 4]) for i in range(count)]


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument('--capture', nargs=2, action='append', metavar=('DUMP', 'LOG'),
                    required=True)
    ap.add_argument('--out', required=True)
    args = ap.parse_args()

    table = {}
    conflicts = 0
    calls = 0
    stale = 0
    for dump, log in args.capture:
        for h1, h2, t_bits, frac_bits in load_capture(dump, log):
            if (h2 & 1) == 0:
                # The runtime forces h2 odd, so an even h2 is a slot the debugger
                # read before its cache line reached memory (still the .bss
                # zero); it names no call.
                stale += 1
                continue
            calls += 1
            key = (h1, h2, t_bits)
            if key in table and table[key] != frac_bits:
                conflicts += 1
            table[key] = frac_bits
    if conflicts:
        # The same input with two results would mean the capture is not the
        # deterministic source function it is taken to be.
        raise SystemExit(f'{conflicts} conflicting results for one (segment, t)')

    segments = {}
    for (h1, h2, t_bits), frac_bits in table.items():
        segments.setdefault((h1, h2), []).append((t_bits, frac_bits))
    seg_rows = []
    entries = []
    for (h1, h2) in sorted(segments):
        rows = sorted(segments[(h1, h2)])
        seg_rows.append((h1, h2, len(entries), len(rows)))
        entries.extend(rows)

    out = bytearray(struct.pack('<4I', MAGIC, VERSION, len(seg_rows), len(entries)))
    for row in seg_rows:
        out += struct.pack('<4I', *row)
    for t_bits, frac_bits in entries:
        out += struct.pack('<2I', t_bits, frac_bits)
    path = pathlib.Path(args.out)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(bytes(out))
    print(f'{calls} captured calls ({stale} unwritten slots skipped) -> '
          f'{len(entries)} entries in {len(seg_rows)} segments, {len(out)} bytes -> {path}')
    return 0


if __name__ == '__main__':
    sys.exit(main())
