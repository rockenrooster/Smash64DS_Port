#!/usr/bin/env python3
"""Compare the gameplay replay digest of two four-CPU tick-HUD runs.

P2-2p8 Phase 0 (docs/p2/FOUR_FIGHTER_30FPS_ARCHITECTURE.md section 5). Each run's
rows CSV (scripts/verify-p2-four-fighter-stress.ps1 -RowsCsv) carries two
digest columns per presented frame: DGSA after the undrawn logic tick and DGSB
after the drawn one (src/port/nds_replay_digest.c). A candidate and its control
played from the same deterministic boot must agree on every frame both runs
sampled. Exit 0 = identical, 1 = diverged, 2 = the comparison could not be made.

Usage: compare-replay-digest.py CONTROL.csv CANDIDATE.csv [--json OUT.json]

For matched ring-dump windows with skewed presented-frame labels, --sequence
compares every recorded pair in file order. It requires equal lengths and never
skips or realigns rows. The sampler's population/coherence checks still apply.

--resync N (with --sequence) allows ONE realignment: when the runs diverge at
sample i, it accepts a single offset k (0 < |k| <= N) only if every later
overlapping sample matches at that offset. This is the case where a pre-GO load
wait ends one presented frame earlier or later: the same tick states, presented
one frame apart. It is reported as such, never as plain IDENTICAL.
Within a resync only, a digest word of 0 counts as unrecorded (a ring-stop seam
sample can carry DGSB 0); the count of such samples is reported. Also within a
resync only, a sample whose DGSA agrees but whose DGSB differs is accepted when
the NEXT sample agrees in full: the drawn tick's state is what the next undrawn
tick starts from, so a real difference there cannot heal one tick later, while
a DGSB read across one of the sampler's ring stops does exactly that (seen
2026-09-27 on a one-frame-shifted run, every 384 samples). Those are reported
separately as seam tick-B words.
"""
import argparse
import csv
import json
import sys

COLUMNS = ('DGSA', 'DGSB')


def load(path, sequence=False, labels=None):
    with open(path, newline='') as fh:
        reader = csv.DictReader(fh)
        missing = [c for c in COLUMNS if c not in (reader.fieldnames or [])]
        if missing:
            raise SystemExit(f'{path}: no {"/".join(missing)} column; '
                             'the ROM predates the replay digest or the sampler list is stale')
        rows = list(reader)
        if labels is not None:
            labels.extend(int(row['frame']) for row in rows)
        return {i if sequence else int(row['frame']):
                (int(row['DGSA']), int(row['DGSB']))
                for i, row in enumerate(rows)}




def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument('control')
    parser.add_argument('candidate')
    parser.add_argument('--json', default='')
    parser.add_argument('--sequence', action='store_true',
                        help='compare equal-length matched windows in recorded order; no row skipping')
    parser.add_argument('--resync', type=int, default=0,
                        help='with --sequence: accept one shift of up to N samples '
                             'after the first divergence if every later sample matches')
    args = parser.parse_args()

    control_labels = []
    candidate_labels = []
    control = load(args.control, args.sequence, control_labels)
    candidate = load(args.candidate, args.sequence, candidate_labels)
    if args.sequence and len(control) != len(candidate):
        print('sequence comparison requires equal sample counts', file=sys.stderr)
        return 2
    identity = 'sample' if args.sequence else 'frame'
    unit = 'recorded samples' if args.sequence else 'shared frames'
    shared = sorted(set(control) & set(candidate))
    result = {
        'control': args.control,
        'candidate': args.candidate,
        'comparison': 'sequence' if args.sequence else 'frame',
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
                    identity: frame,
                    'tick': tick,
                    'control': [f'{v:08x}' for v in control[frame]],
                    'candidate': [f'{v:08x}' for v in candidate[frame]],
                }
    if result['diverged'] and args.sequence and args.resync > 0:
        first = result['firstDivergence']['sample']
        count = len(shared)
        for k in sorted(range(-args.resync, args.resync + 1), key=abs):
            if k == 0:
                continue
            pairs = [(j + k, j) for j in range(first, count)
                     if 0 <= j + k < count]
            if len(pairs) < count - first - abs(k):
                continue
            unrecorded = 0
            seam_b = 0
            ok = True
            for c, d in pairs:
                if control[c] == candidate[d]:
                    continue
                if all((x == y) or (x == 0) or (y == 0)
                       for x, y in zip(control[c], candidate[d])):
                    unrecorded += 1
                    continue
                if ((control[c][0] == candidate[d][0]) and
                        ((c + 1) in control) and ((d + 1) in candidate) and
                        (control[c + 1] == candidate[d + 1])):
                    seam_b += 1
                    continue
                ok = False
                break
            if ok:
                result['resync'] = {'sample': first, 'shift': k,
                                    'matched': len(pairs),
                                    'unrecordedWords': unrecorded,
                                    'seamTickBWords': seam_b}
                break
    if args.json:
        with open(args.json, 'w') as fh:
            json.dump(result, fh, indent=1)
    if result.get('resync'):
        r = result['resync']
        print(f"IDENTICAL AFTER ONE RESYNC: from sample {r['sample']} candidate "
              f"sample j equals control sample j{r['shift']:+d} for all "
              f"{r['matched']} later samples ({r['unrecordedWords']} with an "
              f"unrecorded zero word, {r['seamTickBWords']} seam tick-B "
              f"words; control {len(control)}, candidate {len(candidate)})")
        return 0
    if result['diverged']:
        first = result['firstDivergence']
        print(f"DIVERGED on {result['diverged']} of {len(shared)} {unit}; "
              f"first at {identity} {first[identity]} tick {first['tick']}: "
              f"control {first['control']} candidate {first['candidate']}")
        return 1
    print(f'IDENTICAL over {len(shared)} {unit} '
          f"(control {len(control)}, candidate {len(candidate)})")
    return 0


if __name__ == '__main__':
    sys.exit(main())
