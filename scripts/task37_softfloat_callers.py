#!/usr/bin/env python3
"""Attribute the soft-float library bill to the functions that actually call it.

The ARM946E-S has no FPU, so every `float` operation is a libgcc call costing
20-30 Thumb instructions. In the cycle-117 whole-match profile that library is
the single largest addressable class in the build -- larger than any one
symbol -- but a PC sampler charges all of it to `__aeabi_fadd`, which tells you
nothing about which code to convert to fixed point.

This makes the attribution exact rather than estimated. Every `bl __aeabi_fadd`
site is itself an instruction with its own PC, so the profiler's execution count
AT THAT PC is the exact number of times that call site ran -- not a sample, not
a static count, not a guess from a loop bound. Summing the sites inside one
function gives that function's exact call count; dividing by the total gives its
exact share of the library's measured cycles.

Two things this deliberately does NOT do:

  * It does not credit a caller with its callees' float. `ftMainPlayAnim` calling
    something that adds floats shows up under the callee, where the code to
    change lives. Read it as a work list, not as a subsystem budget
    (`docs/HANDOFF.md` has the standing warning about confusing the two).
  * It does not estimate cycles-per-call from the library's disassembly. The
    per-call cost is measured: the callee's own total cycles divided by its own
    total call count, so ITCM residency, operand-dependent early-outs and
    denormal paths are all already in the number.

**`scripts/census-softfloat-callers.ps1` asks the same question a different way,
and both are worth keeping.** That one (Task 92 E0) sets a GDB breakpoint on each
helper's exact entry address and reads `lr` for the caller, over a live 90-second
sample. It is the only method that can attribute an INDIRECT call, and it needs
no prior profile. This one is exact rather than sampled -- it counts every
executed call site rather than the ones a sample happened to catch -- covers all
eighteen helpers instead of the two that script defaults to, perturbs nothing,
and needs no emulator run at all, just a profile CSV that already exists. Prefer
this for ranking; reach for the PS1 when there is no profile for the build in
hand, or when a caller is reached through a function pointer.

They agree across a change, which is the useful check: the PS1's cycle-92 reading
had `gcPlayDObjAnimJoint` at **58% of the whole soft-float class**; Requirement 4
converted that function to fixed point, and this attribution no longer finds it
among the float callers at all.

**Read the result against the conversion freeze before proposing anything.**
`census-softfloat-callers.ps1` records it: float in `gmcollision`, `mp*`,
`ftMain*` and `ftComputer` is frozen by the Task 9 state hash and by
`PROJECT_GOAL.md`'s mechanical-equivalence contract, so it **cannot be converted
to fixed point whatever it costs**. Renderer-side float gates on the fidelity
budget instead, which is the owner's call. A high row in this table is therefore
not automatically a lever -- for frozen code the only moves are exact ones
(memoise the answer, cut the call count, delete redundant work), which is what
slices 35-37 did for map collision to bank -10,752 without touching a numeric.

Usage (needs devkitARM on PATH for objdump/nm):

    python scripts/task37_softfloat_callers.py <profile.csv> --elf <elf>
        [--frames N]     divide the totals into ticks per presented frame
        [--top N]        rows to print (default 40)
        [--callee SYM]   restrict to one library routine
"""

from __future__ import annotations

import argparse
import collections
import json
import os
import pathlib
import re
import shutil
import subprocess
import sys

TICKS_PER_CYCLE = 0.4993  # docs/HANDOFF.md: measured, not derived from FTR

# The libgcc/AEABI soft-float surface. Aliases are resolved through the symbol
# table, so listing the canonical AEABI name is enough for the common ones, but
# the raw libgcc names appear in the disassembly too when a TU was built without
# the AEABI aliases.
SOFTFLOAT = re.compile(
    r"^(__aeabi_(f(add|sub|mul|div|rsub|cmp(eq|lt|le|ge|gt|un)|2iz|2uiz)"
    r"|i2f|ui2f|l2f|ul2f|d(add|sub|mul|div|cmp\w+)|cd\w+|i2d|f2d|d2f|d2iz)"
    r"|__(add|sub|mul|div)sf3|__floatsisf|__floatunsisf|__fixsfsi|__fixunssfsi"
    r"|__(eq|ne|lt|le|gt|ge|cmp|unord)sf2"
    r"|__(add|sub|mul|div)df3|__extendsfdf2|__truncdfsf2|__floatsidf"
    r"|__fixdfsi|__(eq|ne|lt|le|gt|ge|cmp|unord)df2)$")


def tool(name):
    for cand in ("arm-none-eabi-" + name, name):
        found = shutil.which(cand)
        if found:
            return found
    dka = os.environ.get("DEVKITARM")
    if dka:
        cand = pathlib.Path(dka) / "bin" / ("arm-none-eabi-" + name)
        for suffix in ("", ".exe"):
            if cand.with_suffix(suffix).exists():
                return str(cand.with_suffix(suffix))
    sys.exit("FAIL: could not run arm-none-eabi-%s -- put devkitARM on PATH "
             "or set DEVKITARM" % name)


def symbol_ranges(elf):
    """{name: (start, size)} for every sized FUNC symbol."""
    out = {}
    text = subprocess.run([tool("nm"), "-S", "--defined-only", str(elf)],
                          capture_output=True, text=True, check=True).stdout
    for line in text.splitlines():
        parts = line.split()
        if len(parts) == 4 and parts[2] in ("t", "T"):
            addr, size, _t, name = parts
            out[name] = (int(addr, 16), int(size, 16))
    return out


def disassemble(elf):
    return subprocess.run([tool("objdump"), "-d", "--no-show-raw-insn",
                           str(elf)], capture_output=True, text=True,
                          check=True).stdout


FUNC_LINE = re.compile(r"^([0-9a-f]{8}) <([^>]+)>:$")
INSN_LINE = re.compile(r"^\s*([0-9a-f]+):\s+(\S+)\s*(.*)$")
CALL_TARGET = re.compile(r"<([^>+]+)(?:\+0x[0-9a-f]+)?>")


def call_sites(disasm):
    """[(pc, caller, callee)] for every branch-with-link into a float routine.

    Tail calls (`b`/`bx` into the library) count too: they execute the routine
    just as a `bl` does, and animation code compiled -Os produces plenty of
    them. A tail call's PC still gets sampled, so the count stays exact.
    """
    sites = []
    caller = None
    for line in disasm.splitlines():
        m = FUNC_LINE.match(line)
        if m:
            caller = m.group(2)
            continue
        m = INSN_LINE.match(line)
        if not m or caller is None:
            continue
        pc, mnem, rest = int(m.group(1), 16), m.group(2), m.group(3)
        if mnem.split(".")[0] not in ("bl", "blx", "b", "bx"):
            continue
        t = CALL_TARGET.search(rest)
        if not t:
            continue
        callee = t.group(1)
        if callee == caller or not SOFTFLOAT.match(callee):
            continue
        sites.append((pc, caller, callee))
    return sites


def scan(csv_path, want_pc):
    """Sum `instructions` and `total_cycles` per PC, for PCs we care about.

    The CSV carries one row per (region, pc); a PC executed from two regions is
    two rows and both count. Parsed by hand rather than through `csv` because
    this file is 54.7M rows.
    """
    insns = collections.Counter()
    cycles = collections.Counter()
    with open(csv_path, "r", buffering=1 << 20) as fh:
        fh.readline()
        for line in fh:
            # region,pc,mode,opcode,instructions,total_cycles,...
            f = line.split(",", 6)
            if len(f) < 6:
                continue
            pc = int(f[1], 16)
            if pc not in want_pc:
                continue
            insns[pc] += int(f[4])
            cycles[pc] += int(f[5])
    return insns, cycles


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("profile")
    ap.add_argument("--elf", required=True)
    ap.add_argument("--frames", type=int, default=0)
    ap.add_argument("--top", type=int, default=40)
    ap.add_argument("--callee", default=None)
    ap.add_argument("--cache", default=None,
                    help="JSON scratch file for the CSV scan")
    args = ap.parse_args()

    syms = symbol_ranges(pathlib.Path(args.elf))
    sites = call_sites(disassemble(pathlib.Path(args.elf)))
    if args.callee:
        sites = [s for s in sites if s[2] == args.callee]
    if not sites:
        print("FAIL: no soft-float call sites found. Wrong ELF, or the build "
              "genuinely has no float left -- check with `nm | grep aeabi_f`.")
        return 1

    # Every PC inside a called routine, so its measured total is exact rather
    # than carried over from a separate census run that may have a different
    # window.
    callees = sorted({c for _p, _f, c in sites})
    body_pc = {}
    for name in callees:
        if name not in syms:
            continue
        start, size = syms[name]
        for pc in range(start & ~1, (start & ~1) + size, 2):
            body_pc[pc] = name

    want = set(p for p, _f, _c in sites) | set(body_pc)
    # The scan is minutes over 54.7M rows and its result is a pure function of
    # (profile, ELF). Cache it so re-ranking is instant -- the analysis above it
    # got rewritten twice on the first day this ran.
    cache = pathlib.Path(args.cache) if args.cache else None
    if cache and cache.exists():
        blob = json.loads(cache.read_text())
        if blob.get("profile") == str(args.profile) and blob.get("n") == len(want):
            insns = collections.Counter({int(k): v
                                         for k, v in blob["insns"].items()})
            cycles = collections.Counter({int(k): v
                                          for k, v in blob["cycles"].items()})
        else:
            cache = None
    if not (cache and cache.exists()):
        insns, cycles = scan(args.profile, want)
        if args.cache:
            pathlib.Path(args.cache).write_text(json.dumps({
                "profile": str(args.profile), "n": len(want),
                "insns": {str(k): v for k, v in insns.items()},
                "cycles": {str(k): v for k, v in cycles.items()}}))

    # Measured cost of one call, per routine.
    calls = collections.Counter()
    for pc, _caller, callee in sites:
        calls[callee] += insns.get(pc, 0)
    body_cycles = collections.Counter()
    for pc, name in body_pc.items():
        body_cycles[name] += cycles.get(pc, 0)

    # libgcc FALLS THROUGH. `__aeabi_fsub` is four bytes -- flip the sign of the
    # second operand -- and then execution runs straight off its end into
    # `__aeabi_fadd`'s body, with no branch. So fsub's own range measures ~1
    # cycle a call while its real work is charged to fadd, which overcharges
    # every fadd caller and undercharges every fsub caller. On the c117 profile
    # that is 1.39M calls misfiled, and it reorders the table: the collision
    # kernels are fsub-heavy and the matrix kernels are fmul/fadd-heavy.
    #
    # Detected, not hardcoded: a routine whose body is adjacent to the next
    # float routine and costs under four cycles a call cannot be doing the work.
    # Pool it forward and say so.
    order = sorted((syms[n][0] & ~1, n) for n in callees if n in syms)
    pool = {}
    for idx, (start, name) in enumerate(order):
        n = calls.get(name, 0)
        if not n or idx + 1 >= len(order):
            continue
        if (body_cycles.get(name, 0) / n) >= 4.0:
            continue
        nxt_start, nxt = order[idx + 1]
        if start + syms[name][1] == nxt_start:
            pool[name] = nxt
    for src, dst in pool.items():
        while dst in pool:
            dst = pool[dst]
        body_cycles[dst] += body_cycles.pop(src, 0)
        calls[dst] += calls.get(src, 0)
        print("pooled         %s falls through into %s (%s calls)"
              % (src, dst, "{:,}".format(calls.get(src, 0))))
        calls[src] = 0

    per_call = {}
    for name in callees:
        tgt = name
        while tgt in pool:
            tgt = pool[tgt]
        n = calls.get(tgt, 0)
        per_call[name] = (body_cycles.get(tgt, 0) / n) if n else 0.0

    # Attribute each routine's measured cycles to callers by exact call count.
    by_caller = collections.Counter()
    caller_calls = collections.Counter()
    caller_sites = collections.Counter()
    detail = collections.defaultdict(collections.Counter)
    for pc, caller, callee in sites:
        n = insns.get(pc, 0)
        if not n:
            continue
        by_caller[caller] += n * per_call[callee]
        caller_calls[caller] += n
        caller_sites[caller] += 1
        detail[caller][callee] += n

    total_lib = sum(body_cycles.values())
    total_calls = sum(calls.values())
    attributed = sum(by_caller.values())

    def fr(cyc):
        if not args.frames:
            return ""
        return "%10.0f" % (cyc * TICKS_PER_CYCLE / args.frames)

    print("elf            %s" % args.elf)
    print("call sites     %d static, %d executed, in %d callers"
          % (len(sites), sum(1 for p, _f, _c in sites if insns.get(p)),
             len(by_caller)))
    print("library total  %s cycles over %s calls"
          % ("{:,}".format(total_lib), "{:,}".format(total_calls)))
    if args.frames:
        print("               %s ticks per presented frame over %d frames"
              % ("{:,.0f}".format(total_lib * TICKS_PER_CYCLE / args.frames),
                 args.frames))
    print("attributed     %.1f%% (the rest is call sites never executed)"
          % (100.0 * attributed / total_lib if total_lib else 0.0))

    print("\n== routines, and what one call actually costs ==")
    print("%-22s %14s %14s %9s" % ("routine", "cycles", "calls", "cyc/call"))
    print("-" * 62)
    for name in sorted(callees, key=lambda n: -body_cycles.get(n, 0)):
        if not body_cycles.get(name):
            continue
        print("%-22s %14s %14s %9.1f"
              % (name, "{:,}".format(body_cycles[name]),
                 "{:,}".format(calls.get(name, 0)), per_call[name]))

    hdr = "%14s %6s %14s %6s %s" % ("cycles", "%lib", "calls", "sites",
                                    "caller")
    if args.frames:
        hdr = "%10s " % "tk/fr" + hdr
    print("\n== who pays it ==")
    print(hdr)
    print("-" * (len(hdr) + 30))
    for caller, cyc in by_caller.most_common(args.top):
        row = "%14s %5.1f%% %14s %6d %s" % (
            "{:,.0f}".format(cyc), 100.0 * cyc / total_lib,
            "{:,}".format(caller_calls[caller]), caller_sites[caller], caller)
        if args.frames:
            row = fr(cyc) + " " + row
        print(row)
        mix = ", ".join("%s x%s" % (c.replace("__aeabi_", ""),
                                    "{:,}".format(n))
                        for c, n in detail[caller].most_common(4))
        print("%s   %s" % (" " * (11 if args.frames else 0), mix))

    print("\nA caller's row is the float IT executes, not its subtree. Convert "
          "the top rows to fixed point and the library shrinks by their %lib.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
