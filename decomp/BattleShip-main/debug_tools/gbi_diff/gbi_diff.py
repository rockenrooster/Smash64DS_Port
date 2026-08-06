#!/usr/bin/env python3
"""
gbi_diff.py — Compare GBI display list traces between the port and emulator.

Parses .gbi trace files produced by:
  - Port-side gbi_trace system (debug_traces/port_trace.gbi)
  - Mupen64Plus trace plugin   (emu_trace.gbi)

Aligns commands by frame using LCS-based matching (difflib.SequenceMatcher),
produces a structured diff highlighting only the real divergences.

A single insertion/deletion in either trace does NOT cascade into thousands
of false-positive "mismatches" on every following command — the matcher
re-syncs at the next pair of identical commands and reports the missing/
extra command as "extra in port" or "extra in emu".

Usage:
    python gbi_diff.py <port_trace.gbi> <emu_trace.gbi> [options]

Options:
    --frame N          Only diff frame N (default: all frames)
    --frame-range A-B  Diff frames A through B inclusive
    --context N        Show N preceding commands around each divergence (default: 3)
    --summary          Only show per-frame summary counts, not individual diffs
    --ignore-addresses Don't flag w1 differences in G_DL/G_VTX/G_MTX/G_SETTIMG etc.
                       (essential — port uses 32-bit PORT_RESOLVE tokens for what
                       the emu trace shows as full N64 RDRAM 0x80XXXXXX addresses)
    --output FILE      Write diff to file instead of stdout

Performance:
  --frame N and --frame-range A-B activate a parse-time filter that skips
  non-target frames entirely.  Without this a single-frame query against a
  3.7 GB emu_trace.gbi takes ~100 s; with it, ~16 s.

Divergence taxonomy (after LCS alignment):
  opcode mismatches  - paired cmds with different opcodes
  w0 mismatches      - paired cmds with same opcode, different w0
  w1 mismatches      - paired cmds with same opcode/w0, different w1
                       (suppressed for address opcodes when --ignore-addresses)
  extra in port      - port cmd with no match in emu
  extra in emu       - emu cmd with no match in port
"""
import argparse
import difflib
import re
import sys
from dataclasses import dataclass
from typing import Optional


# ============================================================================
#  Data structures
# ============================================================================

@dataclass
class GbiCommand:
    """A single parsed GBI command."""
    index: int          # [NNNN] command index within frame
    depth: int          # d=N display list call depth
    opcode: str         # G_VTX, G_TRI1, etc.
    w0: str             # 8-hex-digit w0
    w1: str             # 8-hex-digit w1
    params: str         # Decoded parameter string
    w1_64: str = ""     # Optional 64-bit w1 from port (for info only)
    raw: str = ""       # Full original line

    @property
    def opcode_and_params(self) -> str:
        return f"{self.opcode} {self.params}".strip()


@dataclass
class Frame:
    """A single frame's worth of commands."""
    number: int
    commands: list  # list[GbiCommand]
    total_cmds: int  # reported count from END FRAME line


# ============================================================================
#  Parser
# ============================================================================

# Matches: [0042] d=1 G_VTX              w0=01020040 w1=06001234  n=2 v0=0 addr=06001234
CMD_RE = re.compile(
    r'\[(\d+)\]\s+d=(\d+)\s+'       # index, depth
    r'(\S+)\s+'                       # opcode
    r'w0=([0-9A-Fa-f]+)\s+'          # w0
    r'w1=([0-9A-Fa-f]+)'             # w1
    r'(?:\s+(.*))?'                   # optional decoded params
)

# Matches the optional (w1_64=...) suffix on port traces
W1_64_RE = re.compile(r'\(w1_64=([0-9A-Fa-f]+)\)')

FRAME_START_RE = re.compile(r'^=== FRAME (\d+) ===$')
FRAME_END_RE = re.compile(r'^=== END FRAME (\d+)\s*(?:—|-)\s*(\d+) commands ===$')


def parse_trace(filepath: str,
                want_frames: Optional[set] = None) -> dict[int, Frame]:
    """Parse a .gbi trace file into a dict of frame_number -> Frame.

    If `want_frames` is supplied, only frames whose number is in the set
    are kept and the parser skips line work entirely for other frames.
    For multi-GB emu traces this brings single-frame queries from
    ~100s to ~5s by avoiding regex matching on ~6M unrelated cmd lines.
    """
    frames: dict[int, Frame] = {}
    current_frame: Optional[int] = None
    current_cmds: list[GbiCommand] = []
    keep_current = True  # whether the current frame is in want_frames

    with open(filepath, 'r', encoding='utf-8') as f:
        for line in f:
            line = line.rstrip('\n')

            # Skip comments and blank lines
            if not line or line.startswith('#'):
                continue

            # Frame start
            m = FRAME_START_RE.match(line)
            if m:
                current_frame = int(m.group(1))
                current_cmds = []
                keep_current = (want_frames is None or
                                current_frame in want_frames)
                continue

            # Frame end
            m = FRAME_END_RE.match(line)
            if m:
                frame_num = int(m.group(1))
                total = int(m.group(2))
                if keep_current:
                    frames[frame_num] = Frame(
                        number=frame_num,
                        commands=current_cmds,
                        total_cmds=total,
                    )
                current_frame = None
                continue

            # Command line — skip the regex work entirely for filtered frames
            if current_frame is not None and keep_current:
                m = CMD_RE.match(line)
                if m:
                    w1_64 = ""
                    m2 = W1_64_RE.search(line)
                    if m2:
                        w1_64 = m2.group(1)

                    cmd = GbiCommand(
                        index=int(m.group(1)),
                        depth=int(m.group(2)),
                        opcode=m.group(3),
                        w0=m.group(4).upper(),
                        w1=m.group(5).upper(),
                        params=m.group(6).strip() if m.group(6) else "",
                        w1_64=w1_64,
                        raw=line,
                    )
                    # Skip zero-NOOP padding: the M64P rsp_trace walker reads
                    # past G_ENDDL into uninitialized RDRAM, producing thousands
                    # of G_NOOP w0=0 w1=0 entries. The port never emits these.
                    if (cmd.opcode == "G_NOOP" and
                            cmd.w0 == "00000000" and cmd.w1 == "00000000"):
                        continue
                    # Skip the RDPHALF_1 / RDPHALF_2 trailers that follow a
                    # G_TEXRECT / G_TEXRECTFLIP in the emu trace. The port's
                    # Fast3D interpreter folds those into the TEXRECT entry.
                    if current_cmds and cmd.opcode in ("G_RDPHALF_1", "G_RDPHALF_2"):
                        prev = current_cmds[-1]
                        if prev.opcode in ("G_TEXRECT", "G_TEXRECTFLIP", "G_RDPHALF_1"):
                            continue
                    current_cmds.append(cmd)

    return frames


# ============================================================================
#  Comparison logic
# ============================================================================

# Opcodes where w1 is an address (not comparable between port and emu)
ADDRESS_OPCODES = {
    'G_VTX', 'G_DL', 'G_MTX', 'G_SETTIMG', 'G_SETCIMG', 'G_SETZIMG',
    'G_MOVEMEM', 'G_MOVEWORD', 'G_LOAD_UCODE', 'G_BRANCH_Z', 'G_DMA_IO',
}


@dataclass
class Divergence:
    """A single command divergence between port and emu.

    After LCS-based alignment, port_cmd and emu_cmd may correspond to
    different physical positions in their respective traces.  We store
    the parser-list position on each side (port_pos / emu_pos) so the
    output can show context cheaply without an O(N) lookup.
    `cmd_index` is the *port-side* index when present, else the emu
    index, used only for display ordering.
    """
    cmd_index: int
    port_cmd: Optional[GbiCommand]
    emu_cmd: Optional[GbiCommand]
    reason: str  # What diverged: opcode, w0, w1, extra in port, extra in emu
    port_pos: Optional[int] = None
    emu_pos: Optional[int] = None


def cmd_key(cmd: GbiCommand, ignore_addresses: bool) -> tuple:
    """Build a hashable key used by SequenceMatcher to align cmds.

    Two cmds with the same key are treated as identical for alignment
    purposes.  We deliberately exclude depth and decoded params: depth
    can drift on the emu side because the rsp_trace plugin walks past
    G_ENDDL into garbage, and params are derived from w0/w1 anyway.
    """
    if ignore_addresses and cmd.opcode in ADDRESS_OPCODES:
        return (cmd.opcode, cmd.w0, "*")
    return (cmd.opcode, cmd.w0, cmd.w1)


def _classify_replace_reason(port_cmd: GbiCommand, emu_cmd: GbiCommand) -> str:
    """Build a reason string for two cmds at the same logical slot."""
    if port_cmd.opcode != emu_cmd.opcode:
        return f"opcode: {port_cmd.opcode} vs {emu_cmd.opcode}"
    if port_cmd.w0 != emu_cmd.w0:
        return f"w0: {port_cmd.w0} vs {emu_cmd.w0}"
    return f"w1: {port_cmd.w1} vs {emu_cmd.w1}"


def diff_frame(port_frame: Frame, emu_frame: Frame,
               ignore_addresses: bool) -> list[Divergence]:
    """Diff two frames using LCS-based alignment.

    Uses difflib.SequenceMatcher so a single insertion or deletion in
    one trace doesn't cascade into thousands of false-positive mismatches
    on every subsequent command.  The matcher walks both sequences and
    re-syncs at the next pair of identical commands after any divergence.
    """
    port_cmds = port_frame.commands
    emu_cmds = emu_frame.commands

    port_keys = [cmd_key(c, ignore_addresses) for c in port_cmds]
    emu_keys = [cmd_key(c, ignore_addresses) for c in emu_cmds]

    # autojunk=False keeps SequenceMatcher from heuristically dropping
    # frequent items (e.g. G_RDPPIPESYNC), which would corrupt alignment.
    matcher = difflib.SequenceMatcher(a=port_keys, b=emu_keys, autojunk=False)

    divergences: list[Divergence] = []

    for tag, i1, i2, j1, j2 in matcher.get_opcodes():
        if tag == "equal":
            continue

        if tag == "replace":
            # Items at logically-corresponding slots that differ.  The
            # spans (i2-i1) and (j2-j1) need not be equal — pair them up
            # element-by-element and emit any leftovers as inserts/deletes.
            n_port = i2 - i1
            n_emu = j2 - j1
            n_pair = min(n_port, n_emu)
            for k in range(n_pair):
                pi = i1 + k
                ej = j1 + k
                pc = port_cmds[pi]
                ec = emu_cmds[ej]
                divergences.append(Divergence(
                    cmd_index=pc.index,
                    port_cmd=pc,
                    emu_cmd=ec,
                    reason=_classify_replace_reason(pc, ec),
                    port_pos=pi,
                    emu_pos=ej,
                ))
            for k in range(n_pair, n_port):
                pi = i1 + k
                pc = port_cmds[pi]
                divergences.append(Divergence(
                    cmd_index=pc.index,
                    port_cmd=pc,
                    emu_cmd=None,
                    reason="extra in port",
                    port_pos=pi,
                ))
            for k in range(n_pair, n_emu):
                ej = j1 + k
                ec = emu_cmds[ej]
                divergences.append(Divergence(
                    cmd_index=ec.index,
                    port_cmd=None,
                    emu_cmd=ec,
                    reason="extra in emu",
                    emu_pos=ej,
                ))
            continue

        if tag == "delete":
            # Cmds present in port but absent in emu.
            for k in range(i1, i2):
                pc = port_cmds[k]
                divergences.append(Divergence(
                    cmd_index=pc.index,
                    port_cmd=pc,
                    emu_cmd=None,
                    reason="extra in port",
                    port_pos=k,
                ))
            continue

        if tag == "insert":
            # Cmds present in emu but absent in port.
            for k in range(j1, j2):
                ec = emu_cmds[k]
                divergences.append(Divergence(
                    cmd_index=ec.index,
                    port_cmd=None,
                    emu_cmd=ec,
                    reason="extra in emu",
                    emu_pos=k,
                ))
            continue

    return divergences


# ============================================================================
#  Output formatting
# ============================================================================

def format_cmd(label: str, cmd: Optional[GbiCommand]) -> str:
    """Format a command for diff output."""
    if cmd is None:
        return f"  {label}: (absent)"
    return f"  {label}: [{cmd.index:04d}] d={cmd.depth} {cmd.opcode:<18s} w0={cmd.w0} w1={cmd.w1}  {cmd.params}"


def _div_category(div: Divergence) -> str:
    """Bucket a Divergence into one of the summary categories."""
    if div.reason.startswith("opcode"):
        return "opcode"
    if div.reason.startswith("w0"):
        return "w0"
    if div.reason.startswith("w1"):
        return "w1"
    if "extra in port" in div.reason:
        return "extra_port"
    if "extra in emu" in div.reason:
        return "extra_emu"
    return "other"


def print_diff(port_frames: dict[int, Frame], emu_frames: dict[int, Frame],
               args, out):
    """Produce the full diff report."""

    # Determine frame range
    all_frames = sorted(set(port_frames.keys()) | set(emu_frames.keys()))

    if args.frame is not None:
        all_frames = [f for f in all_frames if f == args.frame]
    elif args.frame_range:
        a, b = map(int, args.frame_range.split('-'))
        all_frames = [f for f in all_frames if a <= f <= b]

    total_divs = 0
    total_frames_diffed = 0
    frames_with_divs = 0

    for fnum in all_frames:
        port_frame = port_frames.get(fnum)
        emu_frame = emu_frames.get(fnum)

        if port_frame is None:
            out.write(f"\n--- FRAME {fnum}: present in emu only ({emu_frame.total_cmds} cmds) ---\n")
            total_frames_diffed += 1
            frames_with_divs += 1
            continue

        if emu_frame is None:
            out.write(f"\n--- FRAME {fnum}: present in port only ({port_frame.total_cmds} cmds) ---\n")
            total_frames_diffed += 1
            frames_with_divs += 1
            continue

        divergences = diff_frame(port_frame, emu_frame, args.ignore_addresses)
        total_frames_diffed += 1

        if not divergences:
            if not args.summary:
                # Only note matching frames in verbose mode
                pass
            continue

        frames_with_divs += 1
        total_divs += len(divergences)

        port_count = len(port_frame.commands)
        emu_count = len(emu_frame.commands)
        # After LCS alignment "matching" is the count of paired-equal cmds.
        # Each replace divergence consumes one cmd from each side; each
        # extra-in-port consumes one port cmd; each extra-in-emu consumes
        # one emu cmd.  So matching = port_count - (replaces + extra_port).
        n_replace = sum(1 for d in divergences
                        if d.port_cmd is not None and d.emu_cmd is not None)
        n_extra_port = sum(1 for d in divergences
                           if d.port_cmd is not None and d.emu_cmd is None)
        matching = port_count - n_replace - n_extra_port

        out.write(f"\n{'='*72}\n")
        out.write(f"FRAME {fnum}: {len(divergences)} divergences "
                  f"(port={port_count} cmds, emu={emu_count} cmds, "
                  f"{matching} matching)\n")
        out.write(f"{'='*72}\n")

        if args.summary:
            buckets = {"opcode": 0, "w0": 0, "w1": 0,
                       "extra_port": 0, "extra_emu": 0, "other": 0}
            for d in divergences:
                buckets[_div_category(d)] += 1
            out.write(f"  opcode mismatches: {buckets['opcode']}\n")
            out.write(f"  w0 mismatches:     {buckets['w0']}\n")
            out.write(f"  w1 mismatches:     {buckets['w1']}\n")
            out.write(f"  extra in port:     {buckets['extra_port']}\n")
            out.write(f"  extra in emu:      {buckets['extra_emu']}\n")
            if buckets["other"]:
                out.write(f"  other:             {buckets['other']}\n")
            continue

        # Detailed divergence output with context.
        # After LCS alignment, surrounding context comes from each side's
        # own neighbors — we no longer assume port[i] and emu[i] are paired.
        for div in divergences:
            label = "DIVERGENCE"
            port_idx = div.port_cmd.index if div.port_cmd else None
            emu_idx = div.emu_cmd.index if div.emu_cmd else None
            if port_idx is not None and emu_idx is not None:
                where = f"port[{port_idx:04d}] vs emu[{emu_idx:04d}]"
            elif port_idx is not None:
                where = f"port[{port_idx:04d}] (no emu pair)"
            else:
                where = f"emu[{emu_idx:04d}] (no port pair)"
            out.write(f"\n  {label} {where} — {div.reason}\n")
            out.write(format_cmd("PORT", div.port_cmd) + "\n")
            out.write(format_cmd("EMU ", div.emu_cmd) + "\n")

            if args.context > 0:
                # Show the cmds immediately before each side of the divergence.
                # Uses parser-list positions stored on the Divergence so we
                # don't pay an O(N) lookup or get confused by duplicate cmds.
                if div.port_pos is not None and div.port_pos > 0:
                    lo = max(0, div.port_pos - args.context)
                    out.write("  port context:\n")
                    for k in range(lo, div.port_pos):
                        pc = port_frame.commands[k]
                        out.write(f"    [{pc.index:04d}] {pc.opcode:<18s} "
                                  f"w0={pc.w0} w1={pc.w1}  {pc.params}\n")
                if div.emu_pos is not None and div.emu_pos > 0:
                    lo = max(0, div.emu_pos - args.context)
                    out.write("  emu context:\n")
                    for k in range(lo, div.emu_pos):
                        ec = emu_frame.commands[k]
                        out.write(f"    [{ec.index:04d}] {ec.opcode:<18s} "
                                  f"w0={ec.w0} w1={ec.w1}  {ec.params}\n")

    # Summary
    out.write(f"\n{'='*72}\n")
    out.write(f"SUMMARY: {total_frames_diffed} frames compared, "
              f"{frames_with_divs} with divergences, "
              f"{total_divs} total divergences\n")
    out.write(f"{'='*72}\n")


# ============================================================================
#  Main
# ============================================================================

def main():
    parser = argparse.ArgumentParser(
        description="Compare GBI display list traces between port and emulator")
    parser.add_argument("port_trace", help="Port-side trace file (.gbi)")
    parser.add_argument("emu_trace", help="Emulator-side trace file (.gbi)")
    parser.add_argument("--frame", type=int, default=None,
                        help="Only diff this specific frame number")
    parser.add_argument("--frame-range", type=str, default=None,
                        help="Diff frame range (e.g. 10-20)")
    parser.add_argument("--context", type=int, default=3,
                        help="Lines of context around divergences (default: 3)")
    parser.add_argument("--summary", action="store_true",
                        help="Only show per-frame summary counts")
    parser.add_argument("--ignore-addresses", action="store_true",
                        help="Don't flag w1 differences for address-bearing opcodes")
    parser.add_argument("--output", type=str, default=None,
                        help="Output file (default: stdout)")

    args = parser.parse_args()

    # Build a frame filter so the parser can skip irrelevant frames in
    # huge emu traces (~3.7 GB / 6M lines).  Without this a single-frame
    # query parses the entire file end-to-end.
    want_frames: Optional[set] = None
    if args.frame is not None:
        want_frames = {args.frame}
    elif args.frame_range:
        a, b = map(int, args.frame_range.split('-'))
        want_frames = set(range(a, b + 1))

    print(f"Parsing port trace: {args.port_trace}")
    port_frames = parse_trace(args.port_trace, want_frames=want_frames)
    print(f"  -> {len(port_frames)} frames parsed")

    print(f"Parsing emu trace: {args.emu_trace}")
    emu_frames = parse_trace(args.emu_trace, want_frames=want_frames)
    print(f"  -> {len(emu_frames)} frames parsed")

    out = open(args.output, 'w') if args.output else sys.stdout

    try:
        print_diff(port_frames, emu_frames, args, out)
    finally:
        if args.output and out is not sys.stdout:
            out.close()
            print(f"Diff written to {args.output}")


if __name__ == "__main__":
    main()
