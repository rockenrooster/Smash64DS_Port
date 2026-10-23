"""Every tracked scripts/**/test_*.py under pytest, one process a file, N at a
time, from the repository root: a per-file line (passed, failed, errors, the
failing ids) and a TOTAL line. Exit 0 only when no file has a failure, an
error or a timeout.

usage: python scripts/tools/run_host_tests.py OUT_TXT [--workers N] [--timeout S]
"""
import argparse
import concurrent.futures as cf
import re
import subprocess
import sys
import time
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]


def tracked_tests():
    listed = subprocess.run(['git', 'ls-files', 'scripts/*test_*.py', 'scripts/**/test_*.py'],
                            cwd=REPO, capture_output=True, text=True, check=True).stdout
    return sorted(set(listed.split()))


def run(path, limit):
    t0 = time.time()
    try:
        r = subprocess.run([sys.executable, '-m', 'pytest', '-q', '-p', 'no:cacheprovider',
                            '--tb=no', '-rfE', path],
                           cwd=REPO, capture_output=True, text=True, timeout=limit)
        text = r.stdout + r.stderr
        timed_out = False
    except subprocess.TimeoutExpired:
        text, timed_out = '', True
    counts = {k: int(v) for v, k in re.findall(r'(\d+) (passed|failed|errors?|skipped)', text)}
    bad = re.findall(r'^(?:FAILED|ERROR) (\S+)', text, re.M)
    errors = counts.get('error', 0) + counts.get('errors', 0)
    # A file pytest could not even collect reports neither passes nor fails.
    if not timed_out and not counts and 'no tests ran' not in text:
        errors = max(errors, 1)
        bad = bad or ['(no result: ' + (text.strip().splitlines() or ['empty output'])[-1][:120] + ')']
    return path, counts.get('passed', 0), counts.get('failed', 0), errors, time.time() - t0, timed_out, bad


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('out', type=Path)
    ap.add_argument('--workers', type=int, default=6)
    ap.add_argument('--timeout', type=int, default=900)
    a = ap.parse_args()
    files = tracked_tests()
    with cf.ThreadPoolExecutor(a.workers) as ex:
        rows = sorted(ex.map(lambda f: run(f, a.timeout), files))
    problems = 0
    lines = []
    for f, p, fl, e, dt, to, bad in rows:
        flag = '\tTIMEOUT' if to else ''
        lines.append(f'{f}\tpass={p}\tfail={fl}\terr={e}\t{dt:.0f}s{flag}')
        lines.extend(f'  {b}' for b in bad)
        problems += 1 if (fl or e or to) else 0
    tot = [sum(r[i] for r in rows) for i in (1, 2, 3)]
    total = (f'TOTAL files={len(rows)} pass={tot[0]} fail={tot[1]} err={tot[2]} '
             f'files_with_problems={problems}')
    lines.append(total)
    a.out.parent.mkdir(parents=True, exist_ok=True)
    a.out.write_text('\n'.join(lines) + '\n', encoding='utf-8')
    print(total)
    return 1 if problems else 0


if __name__ == '__main__':
    raise SystemExit(main())
