"""Cheap LLM tier of the matching swarm: Haiku on template near-misses.

A "near-miss" is a function the zero-token template tier recognised the *shape*
of, but whose auto-generated C didn't byte-match (wrong field type, arg variant,
etc.). Those are the cheapest possible LLM targets: the model gets the target
disassembly plus a close draft and just has to nudge it.

Design for minimum tokens:
  * raw Messages API over urllib -- no SDK, no agent overhead
  * a single STATIC system prompt (instructions + a few examples), marked with
    cache_control so it's billed once and read cheap on every subsequent call
  * no chain-of-thought: "output only the C function"
  * the deterministic compile + relocation-aware byte-diff (match.py) is the
    judge -- the model is a pure asm->C transformer, nothing else
  * 1 retry max, feeding back the diff; then give up (queue for escalation)

The model never decides anything but the C. Everything else is free code.

Usage:
    python tools/cascade.py --limit 25            # measure on 25 near-misses
    python tools/cascade.py --limit 25 --apply     # also record wins + src
"""
import argparse
import json
import pathlib
import re
import sys
import tempfile
import urllib.request

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import match as M
import swarm as S

API_URL = "https://api.anthropic.com/v1/messages"
DEFAULT_MODEL = "claude-haiku-4-5-20251001"
GHIDRA_OUT = M.REPO / "ghidra_out"


def ghidra_draft(addr):
    """Read the Ghidra C draft for a function address, if one was dumped."""
    p = GHIDRA_OUT / f"0x{addr:08x}.c"
    if p.is_file():
        txt = p.read_text(errors="ignore").strip()
        return txt or None
    return None

SYSTEM = """You reconstruct C source for a matching decompilation of a Nintendo DS \
game. Given a function's ARM disassembly, output C that, compiled with mwccarm \
1.2/sp2p3 at flags `-O4,p -enum int -lang c99 -char signed -interworking -proc \
arm946e -gccext,on -msgstyle gcc`, produces BYTE-IDENTICAL code.

Rules:
- Output ONLY the C function inside one ```c code block. No prose, no comments.
- The function name must be EXACTLY the name given. Keep types minimal (int, \
char, short, their unsigned forms, and pointers).
- Valid C99 ONLY -- never C++. No `&` reference params, no classes, no `::`, no \
methods. A name like `_ZN5ActorFooEv` is just a C identifier; declare it `void \
_ZN5ActorFooEv(SomeType *p)` with a pointer, not a reference.
- The code must COMPILE STANDALONE. If it references another function or a \
global, declare it first with `extern`. Define any struct you dereference.
- Leaf-function ABI: integer/pointer args arrive in r0, r1, r2, r3 in order; the \
return value goes in r0.

Examples:

ARM:
  bx lr
C:
```c
void NAME(void) {}
```

ARM:
  ldrh r0, [r0, #0x10]
  bx lr
C:
```c
int NAME(unsigned short *p) { return p[8]; }
```

ARM:
  add r0, r0, r1
  bx lr
C:
```c
int NAME(int a, int b) { return a + b; }
```"""


def disasm_text(b):
    return "\n".join(f"  {i.mnemonic} {i.op_str}".rstrip() for i in S.md.disasm(b, 0))


def diff_text(target, cand_code):
    """Compact word-by-word diff: only the differing offsets."""
    lines = []
    n = min(len(target), len(cand_code))
    for i in range(0, n, 4):
        tw, cw = target[i:i + 4], cand_code[i:i + 4]
        if tw != cw:
            ti = next(S.md.disasm(tw, 0), None)
            ci = next(S.md.disasm(cw, 0), None)
            ts = f"{ti.mnemonic} {ti.op_str}".strip() if ti else tw.hex()
            cs = f"{ci.mnemonic} {ci.op_str}".strip() if ci else cw.hex()
            lines.append(f"  +0x{i:02x}: want `{ts}`  got `{cs}`")
    if len(target) != len(cand_code):
        lines.append(f"  size: want 0x{len(target):x} got 0x{len(cand_code):x}")
    return "\n".join(lines) or "  (sizes/relocs only)"


def collect_near_misses(lo, hi, scan, want):
    """Scan templated functions; keep those whose draft compiled but didn't match."""
    out = []
    for t in S.enumerate_targets(lo, hi, "arm", scan):
        tgt = M.target_bytes(t["addr"], t["size"])
        ins = list(S.md.disasm(tgt, 0))
        if S.is_thunk(ins):
            continue  # tail-call veneer: not single-function matchable
        cand = None
        for rule in S.RULES:
            cand = rule(t["name"], ins, tgt)
            if cand:
                break
        if not cand:
            continue
        c_source, _ = cand
        with tempfile.TemporaryDirectory() as td:
            cf = pathlib.Path(td) / "c.c"
            cf.write_text(c_source)
            obj = M.compile_c(cf, M.CANONICAL, M.DEFAULT_FLAGS)
        if obj is None:
            continue
        code, relocs = M.extract_func(obj, t["name"])
        if code is None:
            continue
        ok, _ = M.compare(tgt, code, relocs, verbose=False)
        if not ok:  # near-miss
            draft = code if code is not None else b""
            out.append({**t, "tgt": tgt, "draft_c": c_source, "draft_diff": diff_text(tgt, draft)})
            if len(out) >= want:
                break
    return out


def collect_no_template(lo, hi, scan, want):
    """Functions no template recognised (and not thunks): the real LLM tier."""
    out = []
    for t in S.enumerate_targets(lo, hi, "arm", scan):
        tgt = M.target_bytes(t["addr"], t["size"])
        ins = list(S.md.disasm(tgt, 0))
        if S.is_thunk(ins):
            continue
        if any(rule(t["name"], ins, tgt) for rule in S.RULES):
            continue  # a template handles it; not for the LLM
        out.append({**t, "tgt": tgt})
        if len(out) >= want:
            break
    return out


def call_haiku(api_key, model, user_text):
    body = {
        "model": model,
        "max_tokens": 512,
        "system": [{"type": "text", "text": SYSTEM,
                    "cache_control": {"type": "ephemeral"}}],
        "messages": [{"role": "user", "content": user_text}],
    }
    req = urllib.request.Request(
        API_URL, data=json.dumps(body).encode(),
        headers={"x-api-key": api_key, "anthropic-version": "2023-06-01",
                 "content-type": "application/json"})
    with urllib.request.urlopen(req, timeout=60) as r:
        data = json.loads(r.read())
    text = "".join(b.get("text", "") for b in data.get("content", []))
    return text, data.get("usage", {})


def extract_c(text):
    m = re.search(r"```(?:c)?\s*(.*?)```", text, re.S)
    return (m.group(1) if m else text).strip()


def oracle_ok(c_source, name, target):
    with tempfile.TemporaryDirectory() as td:
        cf = pathlib.Path(td) / "c.c"
        cf.write_text(c_source)
        obj = M.compile_c(cf, M.CANONICAL, M.DEFAULT_FLAGS)
    if obj is None:
        return False, None
    code, relocs = M.extract_func(obj, name)
    if code is None:
        return False, None
    ok, _ = M.compare(target, code, relocs, verbose=False)
    return ok, code


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--min", type=lambda x: int(x, 0), default=0x4)
    ap.add_argument("--max", type=lambda x: int(x, 0), default=0x80)
    ap.add_argument("--scan", type=int, default=0, help="how many funcs to scan for near-misses (0=all)")
    ap.add_argument("--scaffold", default="none", choices=["none", "ghidra"],
                    help="ghidra: include the Ghidra decompiler draft in the prompt")
    ap.add_argument("--pool", default="near-miss", choices=["near-miss", "no-template"],
                    help="near-miss: template recognised the shape but C was wrong; "
                         "no-template: no rule fired (the real LLM tier)")
    ap.add_argument("--limit", type=int, default=25, help="max functions to send to the model")
    ap.add_argument("--retries", type=int, default=1)
    ap.add_argument("--model", default=DEFAULT_MODEL)
    ap.add_argument("--apply", action="store_true")
    ap.add_argument("--verbose", action="store_true")
    args = ap.parse_args()

    import os
    api_key = os.environ.get("ANTHROPIC_API_KEY")
    if not api_key:
        sys.exit("ANTHROPIC_API_KEY not set")

    print(f"collecting up to {args.limit} {args.pool} functions...")
    if args.pool == "no-template":
        nm = collect_no_template(args.min, args.max, args.scan, args.limit)
    else:
        nm = collect_near_misses(args.min, args.max, args.scan, args.limit)
    if args.scaffold == "ghidra":
        have = 0
        for t in nm:
            t["ghidra"] = ghidra_draft(t["addr"])
            have += t["ghidra"] is not None
        print(f"ghidra drafts available for {have}/{len(nm)}")
    print(f"got {len(nm)}; sending to {args.model}\n")

    tok = {"in": 0, "out": 0, "cache_read": 0, "cache_write": 0}
    wins = []
    calls = 0
    for t in nm:
        prompt = (f"Function name: {t['name']}\n\n"
                  f"Target ARM disassembly (reproduce exactly):\n{disasm_text(t['tgt'])}\n\n")
        if t.get("ghidra"):
            prompt += ("A decompiler (Ghidra) draft of this function -- approximate, "
                       "NOT byte-matching; use it for structure/types/callee names, then "
                       f"rewrite to match the disassembly exactly:\n```c\n{t['ghidra']}\n```\n\n")
        if t.get("draft_c"):
            prompt += (f"A draft that compiled but differed:\n```c\n{t['draft_c'].strip()}\n```\n"
                       f"Differences:\n{t['draft_diff']}\n\n")
        prompt += "Return only the C function."
        matched_c = None
        for attempt in range(args.retries + 1):
            try:
                text, usage = call_haiku(api_key, args.model, prompt)
            except Exception as e:
                print(f"  ! {t['name']}: API error {e}")
                break
            calls += 1
            tok["in"] += usage.get("input_tokens", 0)
            tok["out"] += usage.get("output_tokens", 0)
            tok["cache_read"] += usage.get("cache_read_input_tokens", 0)
            tok["cache_write"] += usage.get("cache_creation_input_tokens", 0)
            c_source = extract_c(text)
            ok, code = oracle_ok(c_source, t["name"], t["tgt"])
            if ok:
                matched_c = c_source
                break
            # feed the new diff back for the retry
            prompt = (f"Function name: {t['name']}\n\n"
                      f"Target ARM disassembly (reproduce exactly):\n{disasm_text(t['tgt'])}\n\n"
                      f"Your last attempt still differed:\n```c\n{c_source}\n```\n"
                      f"Differences:\n{diff_text(t['tgt'], code) if code else '  (did not compile / symbol missing)'}\n\n"
                      f"Return only the corrected C function.")
        if matched_c:
            wins.append((t, matched_c))
            mark = "MATCH"
        else:
            mark = "miss"
        if args.verbose or matched_c:
            print(f"  {mark} {t['name']} @ 0x{t['addr']:08x}")

    written = 0
    if args.apply and wins:
        import ledger as L
        for t, c_source in wins:
            body = S.pretty(c_source) if " { " in c_source else c_source
            if L.bank({"addr": t["addr"], "name": t["name"], "size": t["size"],
                       "module": "arm9", "versions": [f"haiku:{args.model}"]},
                      body) == "banked":
                written += 1

    print("\n" + "=" * 52)
    matched = len(wins)
    print(f"near-misses sent: {len(nm)}   API calls: {calls}   MATCHED: {matched}")
    print(f"tokens  in={tok['in']}  out={tok['out']}  "
          f"cache_read={tok['cache_read']}  cache_write={tok['cache_write']}")
    total_in = tok["in"] + tok["cache_read"] + tok["cache_write"]
    if matched:
        print(f"per match:  ~{total_in // matched} input tok  +  {tok['out'] // matched} output tok")
    if args.apply:
        print(f"applied: banked {written}/{matched} to matched.jsonl + src/*.c")
    elif matched:
        print("\n(dry-run: re-run with --apply to record)")


if __name__ == "__main__":
    main()
