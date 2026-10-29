#!/usr/bin/env python3
"""P6: store the authored contents' Ultimate-derived clips and cues once
(owner 2026-10-29), so the build no longer reads the local Ultimate files.

The build converted them every time: anim.py retargeted each "ultimate"
recipe's motion (scripts/p6/ultimate.py, the owner's .nuanmb files and the
Ultimate skeleton) and p6_audio.py rendered each cue from its recording.
This converts them one more time, writes the results under
assets/p6-art/<name>/anims/ (the clip, and its skirt guide when it has one)
and assets/p6-art/<name>/audio/ (the cue's samples: 16-bit mono WAV at the
kind's rate, render_cue's output), and records each file and its sha256 in
the recipe ("frozen"), which the build reads instead (anim.py build,
p6_audio.py records). The stored files derive from the rips: local, ignored,
never tracked (plan D1); authored replacements take their place later.

Freezing needs what converting needed (the Ultimate files, ssbh_data_py,
the staging and the look scenes): run it again after editing such a
recipe's conversion block or a cue's trim, gain or pitch, or the build
keeps the stored result. --from-built stores the clips make last built
(builds/p6-art/<name>/anims) instead of converting them again: the first
freeze took the clips the play ROM was built from (2026-10-29).

    p6_freeze.py [--content palutena|ike] [--clips | --audio] [--only NAME ...] [--from-built]
    p6_freeze.py --check    stored files present and matching their digests
"""
from __future__ import annotations

import argparse
import hashlib
import json
import sys
import wave
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[1]
sys.path.insert(0, str(HERE))
sys.path.insert(0, str(REPO / "scripts" / "p4"))
CONTENTS = ("palutena", "ike")
STAGING = REPO / "builds" / "p4-staging" / "extra-96621af"


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def rel(path: Path) -> str:
    return path.relative_to(REPO).as_posix()


def write_json(path: Path, data: dict) -> None:
    # The recipes' own style (indent 1, ASCII, a final newline).
    path.write_text(json.dumps(data, indent=1) + "\n", encoding="utf-8", newline="\n")


def freeze_clips(name: str, only: set[str], staging: Path, from_built: bool) -> tuple[int, int]:
    if not from_built:
        import anim
        import remix_rom
        rom = remix_rom.Rom(staging / "ssb64asm.z64")
    out_dir = REPO / "assets" / "p6-art" / name / "anims"
    out_dir.mkdir(parents=True, exist_ok=True)
    built_dir = REPO / "builds" / "p6-art" / name / "anims"
    stored = same = 0
    for path in sorted((REPO / "art" / "p6" / name / "anims").glob("*.json")):
        recipe = json.loads(path.read_text(encoding="utf-8"))
        if "ultimate" not in recipe or (only and path.stem not in only):
            continue
        built = built_dir / f"{path.stem}.o2r"
        built_guide = built.with_suffix(".skirt.json")
        if from_built:
            if not built.exists():
                raise SystemExit(f"{rel(built)}: not built (run make first)")
            blob = built.read_bytes()
            text = built_guide.read_text(encoding="utf-8") if built_guide.exists() else None
        else:
            extras: dict = {}
            blob = anim.build({k: v for k, v in recipe.items() if k != "frozen"}, rom, extras)
            text = json.dumps(extras["skirt_guide"]) + "\n" if "skirt_guide" in extras else None
        clip = out_dir / f"{path.stem}.o2r"
        clip.write_bytes(blob)
        frozen = {"clip": rel(clip), "sha256": digest(blob)}
        guide = clip.with_suffix(".skirt.json")
        if text is not None:
            guide.write_text(text, encoding="utf-8", newline="\n")
            frozen["skirt_guide"] = rel(guide)
            frozen["skirt_guide_sha256"] = digest(text.encode("utf-8"))
        elif guide.exists():
            guide.unlink()
        recipe["frozen"] = frozen
        write_json(path, recipe)
        stored += 1
        if built.exists() and built.read_bytes() == blob:
            same += 1
        elif not from_built:
            print(f"clip {name}/{path.stem}: {len(blob)} bytes, differs from {rel(built)}")
    return stored, same


def freeze_audio(name: str, only: set[str]) -> int:
    import p6_audio
    spec_path = p6_audio.spec_path(name)
    spec = json.loads(spec_path.read_text(encoding="utf-8"))
    p6_audio.load(name)  # validated as the build validates it
    source = REPO / spec["source"]
    out_dir = REPO / "assets" / "p6-art" / name / "audio"
    out_dir.mkdir(parents=True, exist_ok=True)
    stored = 0
    for cue, row in sorted(spec["cues"].items()):
        if only and cue not in only:
            continue
        pcm, rate = p6_audio.render_cue({k: v for k, v in row.items() if k != "frozen"}, source)
        path = out_dir / f"{cue}.wav"
        with wave.open(str(path), "wb") as w:
            w.setnchannels(1)
            w.setsampwidth(2)
            w.setframerate(rate)
            w.writeframes(b"".join(int(s).to_bytes(2, "little", signed=True) for s in pcm))
        row["frozen"] = {"pcm": rel(path), "sha256": digest(path.read_bytes())}
        stored += 1
    write_json(spec_path, spec)
    print(f"audio {name}: {stored} cues stored")
    return stored


def check() -> int:
    bad = 0
    for name in CONTENTS:
        for path in sorted((REPO / "art" / "p6" / name / "anims").glob("*.json")):
            f = json.loads(path.read_text(encoding="utf-8")).get("frozen")
            for key, sha in (("clip", "sha256"), ("skirt_guide", "skirt_guide_sha256")):
                if f and key in f:
                    p = REPO / f[key]
                    if not p.exists() or digest(p.read_bytes()) != f[sha]:
                        print(f"{rel(path)}: {f[key]} missing or changed")
                        bad += 1
        spec = json.loads((REPO / "scripts" / "p6" / name / "audio.json").read_text(encoding="utf-8"))
        for cue, row in spec["cues"].items():
            f = row.get("frozen")
            if f:
                p = REPO / f["pcm"]
                if not p.exists() or digest(p.read_bytes()) != f["sha256"]:
                    print(f"{name} cue {cue}: {f['pcm']} missing or changed")
                    bad += 1
    print("p6_freeze --check:", "OK" if not bad else f"{bad} stored files missing or changed")
    return 1 if bad else 0


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--content", choices=CONTENTS, action="append")
    ap.add_argument("--clips", action="store_true", help="only the clips")
    ap.add_argument("--audio", action="store_true", help="only the cues")
    ap.add_argument("--only", nargs="+", default=[], help="recipe stems or cue names")
    ap.add_argument("--staging", type=Path, default=STAGING)
    ap.add_argument("--from-built", action="store_true", help="store make's last built clips")
    ap.add_argument("--check", action="store_true")
    args = ap.parse_args()
    if args.check:
        return check()
    both = not args.clips and not args.audio
    only = set(args.only)
    for name in args.content or CONTENTS:
        if args.clips or both:
            stored, same = freeze_clips(name, only, args.staging, args.from_built)
            print(f"clips {name}: {stored} stored, {same} identical to builds/p6-art")
        if args.audio or both:
            freeze_audio(name, only)
    return 0


if __name__ == "__main__":
    sys.exit(main())
