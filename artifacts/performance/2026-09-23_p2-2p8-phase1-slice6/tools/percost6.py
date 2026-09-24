"""Slice 6: route 1 lean draw cost per kind (ticks a draw), from the lab counters.

Usage: percost6.py PREFIX [PREFIX...]   (reads PREFIX-route1.json)
Per row: draws, then head / guard / kernel / patch / submit ticks a draw and
their sum; the guard window holds the event path, whose share is also shown on
its own (event: amortised over all draws, already inside guard). The tick-HUD
census span before the submit (book) has no per-row counter and is left out.
"""
import json
import os
import sys

HERE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OWNERS = ['Mario', 'Fox', 'Luigi', 'Donkey', 'Captain', 'Samus', 'Link',
          'Pikachu', 'Yoshi', 'Ness', 'Purin', 'Kirby']
PARTS = ('head', 'guard', 'kernel', 'patch', 'submit')


def main():
    print('| roster | kind | detail | draws | ' + ' | '.join(PARTS) + ' | sum | of guard: event |')
    print('|---|---|---|---:|' + '---:|' * (len(PARTS) + 2))
    for pre in sys.argv[1:]:
        p = os.path.join(HERE, pre + '-route1.json')
        if not os.path.exists(p):
            continue
        ex = {e['name']: e['value'] for e in json.load(open(p))['extras']}

        def g(n):
            return ex.get('gNdsFtrLean.' + n, 0) or 0
        for k in range(4):
            own = g('k_owner[%d]' % k)
            dr = g('k_draws[%d]' % k)
            if not own or not dr:
                continue
            cells = [g('k_%s_ticks[%d]' % (part, k)) / dr for part in PARTS]
            detail = 'high' if g('k_high_draws[%d]' % k) * 2 > dr else 'low'
            print('| %s | %s | %s | %s | %s | %s | %s |' % (
                pre, OWNERS[own - 1], detail, '{:,}'.format(dr),
                ' | '.join('{:,.0f}'.format(c) for c in cells), '{:,.0f}'.format(sum(cells)),
                '{:,.0f}'.format(g('k_event_ticks[%d]' % k) / dr)))


if __name__ == '__main__':
    main()
