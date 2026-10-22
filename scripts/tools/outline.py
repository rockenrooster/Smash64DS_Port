"""Outline a C source for splitting: every top-level definition (function or
data) with its first line, size and the preprocessor depth at that point,
plus column-0 comment banners. Prints prefix groups of consecutive
definitions to show topic runs.

usage: python outline.py FILE [--all]
"""
import re
import sys

path = sys.argv[1]
show_all = '--all' in sys.argv
L = open(path, encoding='utf-8', errors='replace').read().replace('\r\n', '\n').split('\n')
SIG = re.compile(r'^(?!(?:if|for|while|switch|return|else|do)\b)[A-Za-z_][\w\s\*\(\),]*?\b([A-Za-z_]\w*)\s*(\(|\[|=|;)')
items = []
pp = 0
i = 0
n = len(L)
while i < n:
    line = L[i]
    m = re.match(r'^\s*#\s*(if|ifdef|ifndef|endif)\b', line)
    if m:
        pp += -1 if m.group(1) == 'endif' else 1
        i += 1
        continue
    if line.startswith('/*') and (i == 0 or not L[i - 1].strip()):
        j = i
        while j < n and '*/' not in L[j]:
            j += 1
        text = ' '.join(x.strip(' /*') for x in L[i:j + 1]).strip()
        if j - i >= 2 or len(text) > 60:
            items.append(('banner', i + 1, j - i + 1, text[:110], pp))
        i = j + 1
        continue
    if line and not line[0].isspace() and line[0] not in '#/}{' and not line.startswith(('typedef', 'extern')):
        m = SIG.match(line)
        if m:
            name = m.group(1)
            # extent: to the matching close brace or ';' at depth 0
            depth = 0
            seen = False
            j = i
            while j < n:
                code = re.sub(r'"(\\.|[^"\\])*"|\'(\\.|[^\'\\])*\'|//.*$', '', L[j])
                depth += code.count('{') - code.count('}')
                if '{' in code:
                    seen = True
                if (seen and depth <= 0) or (not seen and code.rstrip().endswith(';') and depth <= 0):
                    break
                j += 1
            items.append(('def', i + 1, j - i + 1, name, pp))
            i = j + 1
            continue
    i += 1


def stem(name):
    m = re.match(r'(?:g|s)?Nds(?:Renderer)?|nds(?:Renderer)?', name)
    rest = name[m.end():] if m else name
    words = re.findall(r'[A-Z][a-z0-9]*|[A-Z]+(?![a-z])', rest)
    return ''.join(words[:2])


groups = []
for it in items:
    if it[0] == 'banner':
        groups.append(['banner', it[1], it[1], it[3], 1, 0])
        continue
    s = stem(it[3])
    if groups and groups[-1][0] == 'def' and groups[-1][3] == s:
        g = groups[-1]
        g[2] = it[1] + it[2] - 1
        g[4] += 1
    else:
        groups.append(['def', it[1], it[1] + it[2] - 1, s, 1, it[4]])
print(f'{path}: {n} lines, {sum(1 for it in items if it[0] == "def")} definitions')
for g in groups:
    if g[0] == 'banner':
        if show_all:
            print(f'      {g[1]:6d} /* {g[3]}')
    elif g[4] >= 3 or g[2] - g[1] > 150 or show_all:
        print(f'{g[1]:6d}-{g[2]:6d} {g[2] - g[1] + 1:6d} lines {g[4]:3d} defs  {g[3]}{"  (pp " + str(g[5]) + ")" if g[5] else ""}')
