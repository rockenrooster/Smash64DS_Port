#!/usr/bin/env python3
"""lane3_spans.py -- source line spans of the functions/objects in a lane3_members csv.

Usage: lane3_spans.py <members.csv> <repo-root> [--min N] [--kind text|bss|rodata|data] [--out spans.csv]
For each member (size >= min) it opens srcfile, starts at the DWARF line, and brace-matches to the
end of the function (text) or to the terminating ';' of the definition (objects), skipping
strings/comments/preprocessor lines.  Prints  file:start-end  name  bytes  and per-file line totals.
Unity-build note: a .c file that is #included keeps its own line numbers, which is what DWARF reports.
"""
import collections
import csv
import os
import re
import sys


def strip_noise(line):
    out = []
    i = 0
    n = len(line)
    in_s = None
    while i < n:
        c = line[i]
        if in_s:
            if c == '\\':
                i += 2
                continue
            if c == in_s:
                in_s = None
            i += 1
            continue
        if c in '"\'':
            in_s = c
            i += 1
            continue
        if c == '/' and i + 1 < n and line[i + 1] == '/':
            break
        out.append(c)
        i += 1
    return ''.join(out)


def span(lines, start, is_func):
    depth = 0
    seen_brace = False
    in_comment = False
    i = start - 1
    while i < len(lines):
        ln = lines[i]
        # crude block-comment handling
        s = ln
        if in_comment:
            if '*/' in s:
                s = s.split('*/', 1)[1]
                in_comment = False
            else:
                i += 1
                continue
        while '/*' in s:
            pre, post = s.split('/*', 1)
            if '*/' in post:
                s = pre + post.split('*/', 1)[1]
            else:
                s = pre
                in_comment = True
                break
        if not s.lstrip().startswith('#'):
            s = strip_noise(s)
            for c in s:
                if c == '{':
                    depth += 1
                    seen_brace = True
                elif c == '}':
                    depth -= 1
                    if seen_brace and depth == 0 and is_func:
                        return i + 1
            if not is_func and s.rstrip().endswith(';') and depth == 0:
                return i + 1
        i += 1
    return None


def main():
    a = sys.argv
    root = a[2]
    lo = int(a[a.index('--min') + 1]) if '--min' in a else 100
    kind = a[a.index('--kind') + 1] if '--kind' in a else None
    cache = {}
    tot = collections.Counter()
    rows = []
    for r in csv.DictReader(open(a[1], newline='')):
        if int(r['size']) < lo or (kind and r['kind'] != kind):
            continue
        f = r['srcfile']
        if f in ('?',) or not r['line'] or r['line'] == '0':
            continue
        path = os.path.join(root, f)
        if path not in cache:
            try:
                cache[path] = open(path, encoding='utf-8', errors='replace').read().split('\n')
            except Exception:
                cache[path] = None
        lines = cache[path]
        if lines is None:
            continue
        start = int(r['line'])
        end = span(lines, start, r['kind'] == 'text')
        rows.append((f, start, end, r['name'], int(r['size']), r['kind']))
        if end:
            tot[f] += end - start + 1
    rows.sort(key=lambda x: (x[0], x[1]))
    for f, s, e, n, sz, k in rows:
        print('%-58s %6d-%-6s %-58s %6d %s' % (f, s, e if e else '?', n[:58], sz, k))
    print('--- source lines per file (matched spans):')
    for f, c in tot.most_common():
        print('  %-60s %6d' % (f, c))
    if '--out' in a:
        with open(a[a.index('--out') + 1], 'w', newline='') as fh:
            w = csv.writer(fh)
            w.writerow(['file', 'start', 'end', 'name', 'bytes', 'kind'])
            for row in rows:
                w.writerow(row)


if __name__ == '__main__':
    main()
