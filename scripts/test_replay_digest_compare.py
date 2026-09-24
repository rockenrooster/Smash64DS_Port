"""Run: python scripts/test_replay_digest_compare.py (stdlib only)."""
import csv
from pathlib import Path
import subprocess
import sys
import tempfile


def main():
    tool = Path(__file__).with_name('compare-replay-digest.py')
    with tempfile.TemporaryDirectory() as tmp:
        control, candidate = (Path(tmp) / name for name in ('a.csv', 'b.csv'))

        def write(path, rows):
            with path.open('w', newline='') as stream:
                writer = csv.writer(stream)
                writer.writerow(('frame', 'DGSA', 'DGSB'))
                writer.writerows(rows)

        def check(rows, expected, sequence=True):
            write(candidate, rows)
            result = subprocess.run(
                [sys.executable, str(tool), str(control), str(candidate)] +
                (['--sequence'] if sequence else []), capture_output=True, text=True)
            assert result.returncode == expected, result.stdout + result.stderr

        write(control, [(2, 10, 11), (3, 12, 13), (4, 14, 15)])
        check([(2, 10, 11), (3, 12, 13), (4, 14, 15)], 0, sequence=False)
        check([(2, 10, 11), (2, 12, 13), (3, 14, 15)], 0)  # skewed labels
        check([(2, 10, 11), (2, 99, 13), (3, 14, 15)], 1)  # real mutation
        check([(2, 10, 11), (3, 14, 15)], 2)  # missing sample
        check([(2, 12, 13), (3, 14, 15), (4, 16, 17)], 1)  # no resync
        write(control, [(2, 0, 0)])
        check([(2, 0, 0)], 2)  # unengaged digest
        write(control, [])
        check([], 2)
    print('REPLAY_DIGEST_COMPARE_OK: labels, mutations, gaps, engagement')


if __name__ == '__main__':
    main()
