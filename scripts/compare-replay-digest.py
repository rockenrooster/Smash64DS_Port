#!/usr/bin/env python3
"""Compare the gameplay replay digest of two four-CPU tick-HUD runs.

P2-2p8 Phase 0 (docs/p2/FOUR_FIGHTER_30FPS_ARCHITECTURE.md section 5). Each run's
rows CSV (scripts/verify-p2-four-fighter-stress.ps1 -RowsCsv) carries two
digest columns per presented frame: DGSA after the undrawn logic tick and DGSB
after the drawn one (src/port/nds_replay_digest.c). A candidate and its control
played from the same deterministic boot must agree on every frame both runs
sampled. Exit 0 = identical, 1 = diverged, 2 = the comparison could not be made.

Usage: compare-replay-digest.py CONTROL.csv CANDIDATE.csv [--json OUT.json]
"""
import argparse
import csv
import json
import sys

COLUMNS = ('DGSA', 'DGSB')


def load(path):
    with open(path, newline='') as fh:
        reader = csv.DictReader(fh)
        missing = [c for c in COLUMNS if c not in (reader.fieldnames or [])]
        if missing:
            raise SystemExit(f'{path}: no {"/".join(missing)} column; '
                             'the ROM predates the replay digest or the sampler list is stale')
        return {int(row['frame']): (int(row['DGSA']), int(row['DGSB'])) for row in reader}


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument('control')
    parser.add_argument('candidate')
    parser.add_argument('--json', default='')
    args = parser.parse_args()

    control = load(args.control)
    candidate = load(args.candidate)
    shared = sorted(set(control) & set(candidate))
    result = {
        'control': args.control,
        'candidate': args.candidate,
        'controlFrames': len(control),
        'candidateFrames': len(candidate),
        'sharedFrames': len(shared),
        'zeroDigestFrames': sum(1 for f in shared if control[f] == (0, 0)),
        'diverged': 0,
        'firstDivergence': None,
    }
    if not shared:
        print('no frames in common', file=sys.stderr)
        return 2
    if result['zeroDigestFrames'] == len(shared):
        print('every shared digest is zero: the digest never ran', file=sys.stderr)
        return 2
    for frame in shared:
        if control[frame] != candidate[frame]:
            result['diverged'] += 1
            if result['firstDivergence'] is None:
                tick = 'A' if control[frame][0] != candidate[frame][0] else 'B'
                result['firstDivergence'] = {
                    'frame': frame,
                    'tick': tick,
                    'control': [f'{v:08x}' for v in control[frame]],
                    'candidate': [f'{v:08x}' for v in candidate[frame]],
                }
    if args.json:
        with open(args.json, 'w') as fh:
            json.dump(result, fh, indent=1)
    if result['diverged']:
        first = result['firstDivergence']
        print(f"DIVERGED on {result['diverged']} of {len(shared)} shared frames; "
              f"first at presented frame {first['frame']} tick {first['tick']}: "
              f"control {first['control']} candidate {first['candidate']}")
        return 1
    print(f'IDENTICAL over {len(shared)} shared frames '
          f"(control {len(control)}, candidate {len(candidate)})")
    return 0


if __name__ == '__main__':
    sys.exit(main())
