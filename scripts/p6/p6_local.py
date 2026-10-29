#!/usr/bin/env python3
"""P6: which authored contents (Ike, Palutena) this checkout can build.

Their look, gate, clips and sounds derive from the owner's local rips and
are never tracked (plan D1); the owner keeps both fighters on their own PC
(2026-10-29), and a checkout without those stored files builds every other
content. The build tools (scripts/tools/common.ps1 Select-S64ContentFlags)
enable an authored content only when this reports it ready; make itself
still fails a content that is enabled without its inputs.

A content is ready when every stored file its tracked recipes name is
present and, where the recipe records a digest, matches it: the spec's own
files (look, gate) and its skirt scene, each frozen clip and skirt guide,
and each frozen cue. A clip or cue that would still convert from a rip
counts as missing.

    p6_local.py --ready        the ready contents, one registry name a line
    p6_local.py --not-ready    the others
    p6_local.py --why          what each content lacks
"""
from __future__ import annotations

import argparse
import hashlib
import json
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[1]
sys.path.insert(0, str(REPO / "scripts" / "p4"))


def authored() -> list[str]:
    import p4_contents
    return [row["name"] for row in p4_contents.load_registry() if "base" in row]


def missing(name: str) -> list[str]:
    """What the content lacks, empty when it is ready."""
    out = []

    def stored(rel: str, sha: str | None = None) -> None:
        path = REPO / rel
        if not path.is_file():
            out.append(f"{rel}: absent")
        elif sha is not None and hashlib.sha256(path.read_bytes()).hexdigest() != sha:
            out.append(f"{rel}: not the stored file its recipe names")

    spec_path = REPO / "scripts" / "p6" / name / "spec.json"
    spec = json.loads(spec_path.read_text(encoding="utf-8")) if spec_path.exists() else {}
    for entry in (spec.get("files") or {}).values():
        stored(entry["from"])
    if (spec.get("skirt") or {}).get("scene"):
        stored(spec["skirt"]["scene"])
    for recipe_path in sorted((REPO / "art" / "p6" / name / "anims").glob("*.json")):
        recipe = json.loads(recipe_path.read_text(encoding="utf-8"))
        frozen = recipe.get("frozen")
        if frozen:
            stored(frozen["clip"], frozen["sha256"])
            if "skirt_guide" in frozen:
                stored(frozen["skirt_guide"], frozen["skirt_guide_sha256"])
        elif "ultimate" in recipe:
            out.append(f"{recipe_path.relative_to(REPO).as_posix()}: converts from Ultimate (not frozen)")
    audio_path = REPO / "scripts" / "p6" / name / "audio.json"
    if audio_path.exists():
        audio = json.loads(audio_path.read_text(encoding="utf-8"))
        for row in audio["cues"].values():
            if "frozen" in row:
                stored(row["frozen"]["pcm"], row["frozen"]["sha256"])
            else:
                stored(f"{audio['source']}/{row['file']}")
    return out


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    mode = ap.add_mutually_exclusive_group(required=True)
    mode.add_argument("--ready", action="store_true")
    mode.add_argument("--not-ready", action="store_true")
    mode.add_argument("--why", action="store_true")
    args = ap.parse_args()
    for name in authored():
        lacks = missing(name)
        if args.ready and not lacks:
            print(name)
        elif args.not_ready and lacks:
            print(name)
        elif args.why:
            print(f"{name}: {'ready' if not lacks else f'{len(lacks)} missing'}")
            for line in lacks[:10]:
                print(f"  {line}")
            if len(lacks) > 10:
                print(f"  ... {len(lacks) - 10} more")
    return 0


if __name__ == "__main__":
    sys.exit(main())
