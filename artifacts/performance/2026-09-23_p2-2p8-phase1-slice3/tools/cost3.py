"""Per-draw lean cost per kind, as a markdown table, for one or more slice 3
run JSONs (run-s3.ps1 output).

Usage: cost3.py ARM [ARM...]
Per lean draw: guard, kernel, patch, submit (the four parts the brief names),
their sum, and the per-draw bookkeeping (book_ticks, all kinds together).
Kernel/joint divides the kind's kernel ticks by the joints it composed.
"""
import json
import os
import sys

KINDS = ['DK', 'Samus', 'Link', 'Kirby']
HERE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def load(arm):
    path = os.path.join(HERE, arm + '.json')
    d = json.load(open(path))
    return d, {e['name']: e['value'] for e in d['extras']}


def g(ex, name):
    return ex.get('gNdsFtrLean.' + name, 0)


def main():
    print('| arm | kind | lean draws | guard | kernel | patch | submit | sum | kernel/joint | slow joints |')
    print('|---|---|---:|---:|---:|---:|---:|---:|---:|---:|')
    for arm in sys.argv[1:]:
        d, ex = load(arm)
        if 'gNdsFtrLean.k_draws[0]' not in ex:
            # Slice 1 ROM (b0): Samus only, global counters; its submit
            # includes the renderer-stats bookkeeping.
            dr = g(ex, 'draws')
            parts = [g(ex, '%s_ticks' % p) / dr
                     for p in ('guard', 'kernel', 'patch', 'submit')]
            joints = g(ex, 'kernel_joints')
            print('| %s | Samus (slice 1) | %s | %s | %s | %s | %s | %s | %s | %.1f%% |' % (
                arm, '{:,}'.format(dr), *['{:,.0f}'.format(v) for v in parts],
                '{:,.0f}'.format(sum(parts)),
                '{:,.0f}'.format(g(ex, 'kernel_ticks') / joints),
                100.0 * g(ex, 'kernel_slow_joints') / joints))
            continue
        for k, name in enumerate(KINDS):
            dr = g(ex, 'k_draws[%d]' % k)
            if not dr:
                continue
            parts = [g(ex, 'k_%s_ticks[%d]' % (p, k)) / dr
                     for p in ('guard', 'kernel', 'patch', 'submit')]
            joints = g(ex, 'k_kernel_joints[%d]' % k)
            classes = [g(ex, 'k_kernel_class[%d][%d]' % (k, i)) for i in range(8)]
            # fast 0, nolocal 1 are integer/no-local; scale 2 / warm 3 are fast
            # unless the arm sends them to the builder (slow bit 1); lock 4,
            # convert 5, xobj 6, nogobj 7 always take the builder.
            slow = sum(classes[4:])
            if (ex.get('gNdsFtrLeanSlow') or 0) & 1:
                slow += classes[2] + classes[3]
            print('| %s | %s | %s | %s | %s | %s | %s | %s | %s | %s |' % (
                arm, name, '{:,}'.format(dr),
                *['{:,.0f}'.format(v) for v in parts],
                '{:,.0f}'.format(sum(parts)),
                '{:,.0f}'.format(g(ex, 'k_kernel_ticks[%d]' % k) / joints) if joints else '-',
                '%.1f%%' % (100.0 * slow / joints) if joints else '-'))
        dn = g(ex, 'draws')
        if dn:
            print('| %s | book (all kinds) | %s | | | | | %s | | |' % (
                arm, '{:,}'.format(dn), '{:,.0f}'.format(g(ex, 'book_ticks') / dn)))


if __name__ == '__main__':
    main()
