"""Slice 6: host lean-list sizes for every VS kind (fighter_list_proof.list_words).

Usage: listsizes6.py [kind ...]   (default: all twelve VS kinds)
Prints per kind x detail: canonical lean / recorded words and every root
program's lean words, against the entry capacity (3,460 words) and the whole
slot region less one header (7,880 words).
"""
import json
import sys
from pathlib import Path

ROOT = Path(r'D:\Stuff\DevFolder\Smash64DS_Port')
sys.path.insert(0, str(ROOT / 'scripts' / 'fighters'))
sys.path.insert(0, str(ROOT / 'scripts'))
import fighter_list_proof as flp  # noqa: E402

KINDS = ('mario', 'fox', 'luigi', 'donkey', 'captain', 'samus', 'link',
         'pikachu', 'yoshi', 'ness', 'purin', 'kirby')
ENTRY = flp.ENTRY_CAPACITY_WORDS
WIDE = 8840 - 960


def main():
    kinds = sys.argv[1:] or KINDS
    out = {}
    for kind in kinds:
        for detail in ('low', 'high'):
            try:
                variants = flp.list_words(kind, detail)
            except Exception as exc:  # report and continue
                print('%-8s %-4s ERROR %s' % (kind, detail, exc))
                out['%s_%s' % (kind, detail)] = {'error': str(exc)}
                continue
            out['%s_%s' % (kind, detail)] = {k: list(v[:2]) for k, v in variants.items()}
            canon = variants['canonical']
            worst_name, worst = max(variants.items(), key=lambda kv: kv[1][0])
            print('%-8s %-4s canonical lean %5d rec %5d | %2d programs, worst %-18s %5d | %s' % (
                kind, detail, canon[0], canon[1], len(variants) - 1, worst_name, worst[0],
                'fits entry' if worst[0] <= ENTRY else
                ('needs WIDE (%d)' % WIDE if worst[0] <= WIDE else 'EXCEEDS WIDE')))
            for name, v in variants.items():
                if name != 'canonical' and v[0] > ENTRY:
                    print('         over entry: %s %d' % (name, v[0]))
    (Path(__file__).resolve().parent.parent / 'list-sizes.json').write_text(json.dumps(out, indent=1))


if __name__ == '__main__':
    main()
