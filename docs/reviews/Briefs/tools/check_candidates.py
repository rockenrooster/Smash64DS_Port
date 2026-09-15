#!/usr/bin/env python3
"""Read-only candidate applicability check. Never applies patches or changes Git state."""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys

BRIEFS = Path(__file__).resolve().parents[1]


def git(repo: Path, *args: str) -> subprocess.CompletedProcess[str]:
    return subprocess.run(["git", "-C", str(repo), *args], text=True,
                          stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=False)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--repo", type=Path, default=Path.cwd())
    ap.add_argument("--candidate", action="append", default=[],
                    help="Full ID or unique prefix, e.g. R01; repeat to check several.")
    args = ap.parse_args()
    repo = args.repo.resolve()
    manifest = json.loads((BRIEFS / "research/candidate_preimages.json").read_text())
    head = git(repo, "rev-parse", "HEAD")
    if head.returncode:
        print(head.stderr.strip() or "Not a Git checkout", file=sys.stderr)
        return 2
    print(f"Inspected base: {manifest['base_commit']}\nActual HEAD:   {head.stdout.strip()}")
    if head.stdout.strip() != manifest["base_commit"]:
        print("NOTE: different HEAD; check individual contexts and revalidate assumptions.")
    selected = manifest["candidates"]
    if args.candidate:
        chosen = []
        for requested in args.candidate:
            matches = [p for p in selected if p["id"] == requested or p["id"].startswith(requested + "_")]
            if len(matches) != 1:
                ap.error(f"{requested!r} does not identify exactly one candidate")
            if matches[0] not in chosen:
                chosen.append(matches[0])
        selected = chosen
    blocked = False
    for item in selected:
        print(f"\n{item['id']} [{item['status']}]")
        patch = BRIEFS / item["path"]
        expected_hash = item.get("sha256")
        if expected_hash and hashlib.sha256(patch.read_bytes()).hexdigest() != expected_hash:
            print("  BLOCKED: patch hash differs from manifest")
            blocked = True
            continue
        states = []
        for f in item["fragments"]:
            path = repo / f["path"]
            if not path.is_file():
                print(f"  MISSING {f['path']}")
                states.append("missing")
                continue
            text = path.read_text(encoding="utf-8-sig")
            old = text.count(f["before"])
            new = text.count(f["after"])
            if new == 1:
                state = "already-present"
            elif old == 1:
                state = "preimage-matches"
            else:
                state = f"needs-rebase(old={old}, new={new})"
            states.append(state)
            print(f"  {state}: {f['path']} (old line hint {f['line_hint']})")
        touched = sorted({f["path"] for f in item["fragments"]})
        dirty = git(repo, "status", "--short", "--", *touched)
        if dirty.stdout.strip():
            print("  Preserve these local changes:\n" + dirty.stdout.rstrip())
        if all(s == "already-present" for s in states):
            print("  ALREADY PRESENT: do not apply again; this does not prove correctness.")
            continue
        if not all(s == "preimage-matches" for s in states):
            print("  BLOCKED: mixed or changed contexts; rebase/review, never force apply.")
            blocked = True
            continue
        result = git(repo, "apply", "--check", "--whitespace=error", str(patch))
        if result.returncode:
            print("  PATCH CHECK FAILED:\n" + result.stderr.rstrip())
            blocked = True
        else:
            print("  PATCH CHECK PASS: syntactic applicability only; no files changed.")
    return 1 if blocked else 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, ValueError) as exc:
        print(f"check_candidates: {exc}", file=sys.stderr)
        raise SystemExit(2)
