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
separately as seam tick-B words. The same seam shape with NO shift (every
differing sample has equal DGSA and a fully equal next sample; seen 2026-09-27
at three of four ring stops of an unshifted run) is reported as IDENTICAL WITH
N SEAM TICK-B WORDS, and only when --resync is given.

--ticks N, when the frame comparison diverges, compares the runs' tick
streams instead: each run's digest words in frame order (DGSA, DGSB), a word
equal to the one before it counted once (a row samples the same state again
while no new tick ran: the load waits, and the frames whose two words agree)
and zero words dropped. Merged in order, the words must match except for gaps
of at most N words that one run recorded and the other did not (its sampler
missed those frames), with at most N words left over at either end. A change in presentation timing
(a different ROM image, the same game code: frames presented a VBlank earlier
or later) relabels frames without changing one tick, and this sees through it;
a real divergence changes every later tick, which no offset can hide. Reported
as IDENTICAL TICK STREAM, never as plain IDENTICAL.
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


def tick_stream(samples):
    """The digest words in frame order (DGSA then DGSB), a word repeated next
    to itself and zero words dropped (see --ticks)."""
    stream = []
    for frame in sorted(samples):
        for word in samples[frame]:
            if word == 0 or (stream and stream[-1] == word):
                continue
            stream.append(word)
    return stream


def compare_ticks(control, candidate, limit):
    """Merge the two tick streams in order: equal words match; a run of words
    in one stream only (samples the other run did not record) is a gap of at
    most `limit` words; a word in each stream that the other never reaches is
    a divergence. Returns (matched, control gap words, candidate gap words,
    longest gap) or None on a divergence."""
    c, d = tick_stream(control), tick_stream(candidate)

    def ahead(stream, start, word):
        for index in range(start, min(len(stream), start + limit + 1)):
            if stream[index] == word:
                return index
        return None

    i = j = matched = only_c = only_d = longest = 0
    while i < len(c) and j < len(d):
        if c[i] == d[j]:
            matched += 1
            i += 1
            j += 1
            continue
        k = ahead(c, i, d[j])
        m = ahead(d, j, c[i])
        if k is None and m is None:
            return None
        if m is None or (k is not None and k - i <= m - j):
            only_c += k - i
            longest = max(longest, k - i)
            i = k
        else:
            only_d += m - j
            longest = max(longest, m - j)
            j = m
    if (len(c) - i > limit) or (len(d) - j > limit) or matched < limit:
        return None
    return matched, only_c, only_d, longest


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
    parser.add_argument('--ticks', type=int, default=0,
                        help='when frames diverge, compare the tick streams at one '
                             'offset of up to N words (presentation timing changes)')
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
        seam_only = 0
        for frame in shared:
            if control[frame] == candidate[frame]:
                continue
            if ((control[frame][0] == candidate[frame][0]) and
                    ((frame + 1) in control) and ((frame + 1) in candidate) and
                    (control[frame + 1] == candidate[frame + 1])):
                seam_only += 1
                continue
            seam_only = -1
            break
        if seam_only > 0:
            result['seamOnly'] = seam_only
    if (result['diverged'] and args.sequence and args.resync > 0 and
            not result.get('seamOnly')):
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
    if (result['diverged'] and not args.sequence and args.ticks > 0):
        ticks = compare_ticks(control, candidate, args.ticks)
        if ticks is not None:
            result['ticks'] = {'matched': ticks[0], 'controlOnly': ticks[1],
                               'candidateOnly': ticks[2], 'longestGap': ticks[3]}
    if args.json:
        with open(args.json, 'w') as fh:
            json.dump(result, fh, indent=1)
    if result.get('ticks'):
        t = result['ticks']
        print(f"IDENTICAL TICK STREAM over {t['matched']} digest words in order "
              f"({t['controlOnly']} control-only and {t['candidateOnly']} "
              f"candidate-only words in gaps of at most {t['longestGap']}; frame "
              f"labels differ on {result['diverged']} of {len(shared)} shared "
              f"frames: presentation timing)")
        return 0
    if result.get('seamOnly'):
        print(f"IDENTICAL WITH {result['seamOnly']} SEAM TICK-B WORDS over "
              f"{len(shared)} {unit} (DGSA equal and the next sample equal at "
              f"each; control {len(control)}, candidate {len(candidate)})")
        return 0
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
