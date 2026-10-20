"""Owner criterion B (2026-10-20): motion reads after GO never push a frame
past two VBlanks. Reads a gate ring CSV with the STRD column (this frame's
synchronous storage wait, cpuGetTiming ticks).

A presented frame lasts ALL ticks (wall, VBlank-quantized). It is past two
VBlanks when ALL > 2.5 VBlanks (three or more). It was PUSHED there by its
reads when, without them, it would have fit: ALL - WAIT - STRD (the frame's
busy time less the reads) within two VBlanks' budget. WORK-based figures are
printed beside for reference.

usage: python scripts/p7/check_motion_read_push.py <ring csv> [<ring csv> ...]
(the CSVs scripts/sample-tick-hud-buckets.ps1 writes from a gate ROM built after
2026-10-20). Exit 1 if any frame was pushed."""
import csv
import sys

VB = 560190  # cpuGetTiming ticks a VBlank (BUS_CLOCK / 59.8261)


def main(paths):
    worst = 0
    for path in paths:
        rows = list(csv.DictReader(open(path)))[1:]
        if not rows or 'STRD' not in rows[0]:
            print(f'{path}: no STRD column')
            continue
        n = len(rows)
        reads = [r for r in rows if int(r['STRD']) > 0]
        late = [r for r in rows if int(r['ALL']) > 2.5 * VB]
        late_read = [r for r in late if int(r['STRD']) > 0]
        pushed = [r for r in late_read
                  if int(r['ALL']) - int(r['WAIT']) - int(r['STRD']) <= 2 * VB]
        work_pushed = [r for r in rows if int(r['WORK']) > 2 * VB >= int(r['WORK']) - int(r['STRD'])]
        strd = sorted(int(r['STRD']) for r in reads)
        print(f'{path.split("/")[-1].split(chr(92))[-1]}: frames {n}; with a read {len(reads)} '
              f'(median {strd[len(strd) // 2] if strd else 0:,}, max {max(strd) if strd else 0:,} ticks); '
              f'past two VBlanks {len(late)} ({100.0 * (n - len(late)) / n:.1f}% within), of them with a read '
              f'{len(late_read)}, pushed by the read {len(pushed)}; [WORK view: {len(work_pushed)}]')
        for r in pushed[:6]:
            print(f"   frame {r['frame']}: ALL {int(r['ALL']):,} WAIT {int(r['WAIT']):,} "
                  f"WORK {int(r['WORK']):,} read {int(r['STRD']):,}")
        worst = max(worst, len(pushed))
    return 1 if worst else 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
