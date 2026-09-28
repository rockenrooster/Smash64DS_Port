#!/usr/bin/env python3
"""lane3_r1_pp.py -- a tiny conditional-directive evaluator (no compiler involved).

It answers two questions about lane3_r1_p1safe_sources.patch without building anything:

  equal   <orig.c> <patched.c> <nds_build_config.h>
          With NDS_FIGHTER_LEGACY_EXEC=1 the patched file must expose exactly the same
          active source lines as the original under that target's build config
          (blank lines ignored).  Equal text means the same preprocessed token stream,
          so a P1-family target compiles to the same object.
  grep    <patched.c> <nds_build_config.h> <flag 0|1> <regex> [lo hi]
          Lists the ACTIVE lines matching <regex> (original line numbers) with the flag
          set as given, optionally within [lo, hi] -- e.g. every remaining call to a
          retired entry point in the P2 configuration.
  guard   <nds_renderer.h> <nds_build_config.h> <flag>
          Evaluates the new #error conditions of the header patch for that config.

Only #if/#ifdef/#ifndef/#elif/#else/#endif/#error are interpreted; other directives are
skipped.  Macro values come from the build config's `#define NAME VALUE` lines plus the
integer #defines of nds_renderer.h; an unknown identifier evaluates to 0 like cpp and is
reported so a missing input cannot hide.
"""
import re
import sys

RE_DEF = re.compile(r'^\s*#\s*define\s+(\w+)(?:\s+(.*))?$')


def load_macros(paths):
    macros = {}
    for p in paths:
        for raw in open(p, errors='replace'):
            m = RE_DEF.match(raw.rstrip('\n').rstrip('\r'))
            if not m or '(' in m.group(1):
                continue
            v = (m.group(2) or '1').split('/*')[0].strip()
            macros.setdefault(m.group(1), v)
    return macros


def to_int(tok, macros, unknown, depth=0):
    if depth > 20:
        return 0
    tok = tok.strip()
    m = re.fullmatch(r'(0[xX][0-9a-fA-F]+|\d+)[uUlL]*', tok)
    if m:
        return int(m.group(1), 0)
    if re.fullmatch(r'\w+', tok):
        if tok in macros:
            return to_int(macros[tok], macros, unknown, depth + 1)
        unknown.add(tok)
        return 0
    return eval_expr(tok, macros, unknown, depth + 1)


def eval_expr(expr, macros, unknown, depth=0):
    expr = re.sub(r'/\*.*?\*/', ' ', expr)
    expr = re.sub(r'//.*$', '', expr)
    expr = re.sub(r'defined\s*\(\s*(\w+)\s*\)', lambda m: '1' if m.group(1) in macros else '0', expr)
    expr = re.sub(r'defined\s+(\w+)', lambda m: '1' if m.group(1) in macros else '0', expr)

    def sub_id(m):
        name = m.group(0)
        if re.fullmatch(r'0[xX][0-9a-fA-F]+[uUlL]*|\d+[uUlL]*', name):
            return str(int(re.sub(r'[uUlL]+$', '', name), 0))
        return str(to_int(name, macros, unknown, depth + 1))

    expr = re.sub(r'\b(?:0[xX][0-9a-fA-F]+|\d+|[A-Za-z_]\w*)[uUlL]*\b', sub_id, expr)
    expr = expr.replace('&&', ' and ').replace('||', ' or ')
    expr = re.sub(r'!(?!=)', ' not ', expr)
    try:
        return int(eval(expr, {'__builtins__': {}}, {}))
    except Exception as exc:  # pragma: no cover
        raise SystemExit('cannot evaluate #if expression %r (%s)' % (expr, exc))


def join_continuations(lines):
    out = []
    i = 0
    while i < len(lines):
        ln = lines[i].rstrip('\n').rstrip('\r')
        start = i
        while ln.endswith('\\') and i + 1 < len(lines):
            i += 1
            ln = ln[:-1] + ' ' + lines[i].rstrip('\n').rstrip('\r').lstrip()
        out.append((start + 1, ln))
        i += 1
    return out


def walk(path, macros):
    """Yield (line_no, text, active) for every non-directive line, plus errors."""
    unknown = set()
    stack = []          # (parent_active, taken, active)
    active = True
    result = []
    errors = []
    for no, ln in join_continuations(open(path, errors='replace').readlines()):
        s = ln.strip()
        if s.startswith('#'):
            d = re.match(r'#\s*(\w+)\s*(.*)', s)
            if not d:
                continue
            kw, rest = d.group(1), d.group(2)
            if kw in ('if', 'ifdef', 'ifndef'):
                if kw == 'if':
                    val = active and eval_expr(rest, macros, unknown) != 0
                elif kw == 'ifdef':
                    val = active and rest.split()[0] in macros
                else:
                    val = active and rest.split()[0] not in macros
                stack.append((active, val, val))
                active = val
            elif kw == 'elif':
                parent, taken, _ = stack.pop()
                val = parent and (not taken) and eval_expr(rest, macros, unknown) != 0
                stack.append((parent, taken or val, val))
                active = val
            elif kw == 'else':
                parent, taken, _ = stack.pop()
                val = parent and not taken
                stack.append((parent, True, val))
                active = val
            elif kw == 'endif':
                parent, _, _ = stack.pop()
                active = parent
            elif kw == 'error' and active:
                errors.append((no, rest))
            elif kw == 'define' and active:
                m = RE_DEF.match(s)
                if m and '(' not in m.group(1):
                    macros[m.group(1)] = (m.group(2) or '1').split('/*')[0].strip()
            continue
        result.append((no, ln, active))
    if stack:
        raise SystemExit('%s: unbalanced conditional (%d open)' % (path, len(stack)))
    return result, errors, unknown


def active_text(path, macros):
    res, errors, unknown = walk(path, dict(macros))
    return [ln for no, ln, act in res if act and ln.strip()], errors, unknown


def main():
    cmd = sys.argv[1]
    if cmd == 'equal':
        orig, patched, cfg = sys.argv[2:5]
        m1 = load_macros([cfg])
        m2 = dict(m1)
        m2['NDS_FIGHTER_LEGACY_EXEC'] = '1'
        a, e1, u1 = active_text(orig, m1)
        b, e2, u2 = active_text(patched, m2)
        print('active lines: orig %d patched %d  errors %s/%s  unknown %d' % (len(a), len(b), e1, e2, len(u1 | u2)))
        print('IDENTICAL' if a == b else 'DIFFERENT')
        if a != b:
            import difflib
            for l in list(difflib.unified_diff(a, b, 'orig', 'patched', lineterm='', n=0))[:40]:
                print(l)
    elif cmd == 'grep':
        patched, cfg, flag, rx = sys.argv[2:6]
        lo = int(sys.argv[6]) if len(sys.argv) > 6 else 0
        hi = int(sys.argv[7]) if len(sys.argv) > 7 else 10 ** 9
        m = load_macros([cfg])
        m['NDS_FIGHTER_LEGACY_EXEC'] = flag
        res, errors, unknown = walk(patched, m)
        r = re.compile(rx)
        n = 0
        for no, ln, act in res:
            if act and lo <= no <= hi and r.search(ln):
                print('%6d  %s' % (no, ln.strip()[:150]))
                n += 1
        print('%d active matching lines (flag=%s), errors=%s' % (n, flag, errors))
    elif cmd == 'guard':
        hdr, cfg, flag = sys.argv[2:5]
        m = load_macros([cfg, hdr])
        m.pop('SSB64_NDS_RENDERER_H', None)     # load_macros reads the include guard too
        m['NDS_FIGHTER_LEGACY_EXEC'] = flag
        res, errors, unknown = walk(hdr, m)
        print('header #error hits with flag=%s:' % flag, [e for e in errors if 'FIGHTER_LEGACY' in e[1]] or 'none')
        print('unknown identifiers evaluated as 0:', sorted(u for u in unknown if u.startswith('NDS_'))[:12])
    else:
        raise SystemExit(__doc__)


if __name__ == '__main__':
    main()
