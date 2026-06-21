#!/usr/bin/env python3
"""Summarize Smash64DS renderer verifier/log lines.

Usage:
    python scripts/gbi_trace_summary.py verify-output.txt

This is intentionally loose: it scans text for the current OR* renderer markers
and prints only the lines likely to matter during renderer work.
"""
from __future__ import annotations
import re
import sys
from pathlib import Path

PATTERNS = [
    re.compile(r"\bORM[BPEDX]\b", re.I),
    re.compile(r"\bORTX\b", re.I),
    re.compile(r"\bORDP\b", re.I),
    re.compile(r"display[- ]list|\bG_DL\b|\bG_VTX\b|\bTRI", re.I),
    re.compile(r"texture|palette|tlut|mobj|sprite", re.I),
    re.compile(r"blocker|unsupported|opcode|commands|vertices|triangles", re.I),
]

def interesting(line: str) -> bool:
    return any(p.search(line) for p in PATTERNS)

def main(argv: list[str]) -> int:
    if len(argv) < 2:
        print(__doc__.strip())
        return 2
    path = Path(argv[1])
    if not path.exists():
        print(f"missing: {path}", file=sys.stderr)
        return 1
    for line in path.read_text(errors="replace").splitlines():
        if interesting(line):
            print(line)
    return 0

if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
