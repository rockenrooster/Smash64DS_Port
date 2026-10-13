#!/usr/bin/env python3
"""P6 authored export: a base content's export plus an authored content's spec.

An authored content (scripts/p4/contents.json `base`, docs/P6/P6_PLAN.md A7)
has no Remix donor to export. Its export directory is written here, in the
layout scripts/p4/remix_export.py writes (resolved.json, events.json,
summary.txt), from its base content's export and its committed spec
(scripts/p6/<name>/spec.json). Every P4 rule after the export then runs on it
unchanged.

The base's data stays the base's: its files, clips, scripts and tables keep
their ids, so the authored content shares them with the base in a match. The
export gains the content's identity (`content`, `base`) and, from the spec,
what the authored content changes. An empty spec, or none, reproduces the base
export but for identity (F4): the A1 null clone.

    p6_author.py --base-export builds/p4-staging/export-x/roy --content ike \\
        --out builds/p4-staging/export-x/ike [--spec scripts/p6/ike/spec.json]

The output derives from the user's ROM through the base export: build output
only, never tracked.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent / "p4"))
import p4_contents  # noqa: E402

# Spec keys the author applies (A7 adds file substitutions, appended motions
# and statuses, kind-table rows, attribute deltas, sword trails and colour
# animations). Identity keys only describe the spec.
SPEC_IDENTITY = {"schema", "content", "comment"}
SPEC_SCHEMA = "p6-spec-v1"


class AuthorError(SystemExit):
    pass


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def load_spec(path: Path | None, name: str) -> tuple[dict, str | None]:
    """The content's spec ({} when it has none) and its digest."""
    if path is None or not path.exists():
        return {}, None
    spec = json.loads(path.read_text(encoding="utf-8"))
    if spec.get("schema") != SPEC_SCHEMA:
        raise AuthorError(f"{path}: schema {spec.get('schema')!r}, expected {SPEC_SCHEMA}")
    if spec.get("content", name) != name:
        raise AuthorError(f"{path}: spec of {spec['content']!r}, not {name!r}")
    unknown = sorted(set(spec) - SPEC_IDENTITY)
    if unknown:
        raise AuthorError(f"{path}: spec keys {unknown} need the authored export (A7)")
    return spec, sha256(path)


def author(base_export: Path, name: str, spec_path: Path | None, out: Path) -> dict:
    registry = p4_contents.load_registry()
    row = p4_contents.row_of(name, registry)
    if "base" not in row:
        raise AuthorError(f"{name}: not an authored content (no base in contents.json)")
    base = p4_contents.row_of(row["base"], registry)
    resolved_path = base_export / "resolved.json"
    if not resolved_path.exists():
        raise AuthorError(f"{resolved_path}: no export of the base {base['name']}")
    resolved = json.loads(resolved_path.read_text(encoding="utf-8"))
    if resolved.get("fighter") != base["remix"] or "content" in resolved:
        raise AuthorError(f"{base_export}: not {base['name']}'s Remix export")
    spec, spec_sha = load_spec(spec_path, name)

    resolved["content"] = name
    resolved["base"] = base["name"]
    resolved["authored"] = {
        "base_resolved_sha256": sha256(resolved_path),
        "base_events_sha256": sha256(base_export / "events.json"),
        "spec_sha256": spec_sha,
    }
    out.mkdir(parents=True, exist_ok=True)
    (out / "events.json").write_bytes((base_export / "events.json").read_bytes())
    summary = base_export / "summary.txt"
    text = summary.read_text(encoding="utf-8") if summary.exists() else ""
    (out / "summary.txt").write_text(
        f"authored content {name} (P6): base {base['name']}'s export, "
        f"spec {'none' if spec_sha is None else spec_sha[:12]}\n" + text,
        encoding="utf-8", newline="\n")
    # resolved.json last: the Makefile's target, so a stopped run leaves none.
    (out / "resolved.json").write_text(json.dumps(resolved, indent=1) + "\n",
                                       encoding="utf-8", newline="\n")
    return {"content": name, "base": base["name"], "spec": spec_sha, "keys": sorted(spec)}


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--base-export", type=Path, required=True,
                    help="the base content's remix_export.py directory")
    ap.add_argument("--content", required=True, help="the authored content's registry name")
    ap.add_argument("--spec", type=Path, help="its spec (default scripts/p6/<name>/spec.json)")
    ap.add_argument("--out", type=Path, required=True)
    args = ap.parse_args()
    spec = args.spec if args.spec is not None else HERE / args.content / "spec.json"
    done = author(args.base_export, args.content, spec, args.out)
    print(f"p6 author: {done['content']} from {done['base']}, spec "
          f"{'none' if done['spec'] is None else done['spec'][:12]} -> {args.out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
