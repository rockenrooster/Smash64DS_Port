#!/usr/bin/env python3
"""P6 A13: no 3DS-, Ultimate- or Brawl-derived data in a tracked file.

The P6 art pipeline reads local rips (decomp/alt_assets/Fighters/Super Smash
Bros 3DS|Ultimate|brawl, plan D10) and writes kit-derived outputs under
assets/p6-art/ (the hand-run chain: look, gate, costumes, portraits) and
builds/p6-art/<name>/anims/ (make's clips). Only recipes, generators and
authored values are committed.
Checks, over `git ls-files` (decomp/ is the read-only third-party tree and is
left out of the content checks):

1. tracked: no tracked file inside a rip folder, under builds/ or under
   assets/p6-art/;
2. audio: no tracked audio file (wav, mp3, ogg, flac, aif, aiff) outside
   decomp/;
3. images: no tracked image (png, bmp, tga, gif, jpg, jpeg) under art/p6,
   scripts/p6 or docs/P6 unless ALLOWED_IMAGES lists it as authored;
4. copies: no tracked file outside decomp/ byte-identical to a file in a rip
   folder, in assets/p6-art/ or in builds/p6-art/ (checked when those
   folders are present);
5. dumps: no array of more than DUMP_NUMBERS numbers, and no string of more
   than DUMP_CHARS characters, in a tracked JSON recipe under art/p6 or
   scripts/p6 (recipes hold anchors, offsets, colours and tile boxes, never
   texels, vertex pools or encoded images);
6. ignores: .gitignore keeps the three rip folders, builds/ and assets/p6-art/
   out (an /assets/ rule counts);
7. fresh: each authored content's export (builds/p4-staging/export-x/<name>,
   when built) was written from the current spec, motion and menu scripts
   and audio recipe, and from the current files its spec names (main
   excepted: the author rewrites it).

    check_p6_art.py            run the checks on the repository
    check_p6_art.py --self-test  plant a fault for each check and expect red
"""
from __future__ import annotations

import argparse
import hashlib
import json
import subprocess
import sys
import tempfile
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
RIPS = ("decomp/alt_assets/Fighters/Super Smash Bros 3DS",
        "decomp/alt_assets/Fighters/Super Smash Bros Ultimate",
        "decomp/alt_assets/Fighters/Super Smash Bros brawl")
LOCAL_ART = ("assets/p6-art", "builds/p6-art")
AUDIO = (".wav", ".mp3", ".ogg", ".flac", ".aif", ".aiff")
IMAGES = (".png", ".bmp", ".tga", ".gif", ".jpg", ".jpeg")
IMAGE_SCOPES = ("art/p6/", "scripts/p6/", "docs/P6/")
RECIPE_SCOPES = ("art/p6/", "scripts/p6/")
# Authored images a P6 recipe may commit (owner D10: authored art is public).
ALLOWED_IMAGES: set[str] = set()
DUMP_NUMBERS = 4096
DUMP_CHARS = 4096


def tracked(repo: Path) -> list[str]:
    git = "C:/Program Files/Git/cmd/git.exe" if Path("C:/Program Files/Git/cmd/git.exe").exists() else "git"
    out = subprocess.run([git, "-C", str(repo), "ls-files", "-z"], check=True,
                         capture_output=True).stdout
    return [p for p in out.decode("utf-8").split("\0") if p]


def check_tracked(files: list[str]) -> list[str]:
    bad = []
    for f in files:
        if f.startswith(("builds/", "assets/p6-art/")) or any(f.startswith(r + "/") for r in RIPS):
            bad.append(f"tracked: {f} is inside a rip folder, builds/ or assets/p6-art/")
    return bad


def check_audio(files: list[str]) -> list[str]:
    return [f"audio: {f} is a tracked audio file" for f in files
            if not f.startswith("decomp/") and f.lower().endswith(AUDIO)]


def check_images(files: list[str]) -> list[str]:
    return [f"images: {f} is a tracked image (not in ALLOWED_IMAGES)" for f in files
            if f.startswith(IMAGE_SCOPES) and f.lower().endswith(IMAGES) and f not in ALLOWED_IMAGES]


def digests(root: Path) -> dict[str, str]:
    out = {}
    if root.is_dir():
        for p in root.rglob("*"):
            if p.is_file():
                out[hashlib.sha256(p.read_bytes()).hexdigest()] = str(p.relative_to(root.parent))
    return out


def check_copies(repo: Path, files: list[str], derived: dict[str, str]) -> list[str]:
    if not derived:
        return []
    bad = []
    for f in files:
        if f.startswith("decomp/"):
            continue
        p = repo / f
        if p.is_file() and p.stat().st_size > 0:
            h = hashlib.sha256(p.read_bytes()).hexdigest()
            if h in derived:
                bad.append(f"copies: {f} is byte-identical to {derived[h]}")
    return bad


def _walk(value, path: str, bad: list[str], name: str) -> None:
    if isinstance(value, dict):
        for k, v in value.items():
            _walk(v, f"{path}.{k}", bad, name)
    elif isinstance(value, list):
        numbers = sum(1 for v in value if isinstance(v, (int, float)) and not isinstance(v, bool))
        nested = sum(len(v) for v in value if isinstance(v, list)
                     and all(isinstance(x, (int, float)) for x in v))
        if numbers + nested > DUMP_NUMBERS:
            bad.append(f"dumps: {name}{path} holds {numbers + nested} numbers")
        for i, v in enumerate(value):
            _walk(v, f"{path}[{i}]", bad, name)
    elif isinstance(value, str) and len(value) > DUMP_CHARS:
        bad.append(f"dumps: {name}{path} is a {len(value)}-character string")


def check_dumps(repo: Path, files: list[str]) -> list[str]:
    bad: list[str] = []
    for f in files:
        if f.startswith(RECIPE_SCOPES) and f.endswith(".json"):
            try:
                data = json.loads((repo / f).read_text(encoding="utf-8"))
            except (OSError, ValueError) as e:
                bad.append(f"dumps: {f} unreadable ({e})")
                continue
            _walk(data, "", bad, f)
    return bad


def check_ignores(repo: Path) -> list[str]:
    text = (repo / ".gitignore").read_text(encoding="utf-8") if (repo / ".gitignore").exists() else ""
    rules = {ln.strip().rstrip("/") for ln in text.splitlines()}
    bad = []
    for r in RIPS:
        if "/" + r not in rules and r not in rules:
            bad.append(f"ignores: .gitignore does not keep {r} out")
    for keep in (("builds",), ("assets", "assets/p6-art")):
        if not ({r for k in keep for r in (k, "/" + k)} & rules):
            bad.append(f"ignores: .gitignore does not keep {keep[-1]}/ out")
    return bad


def _sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def check_fresh(repo: Path, export_root: Path) -> list[str]:
    bad = []
    for spec in sorted((repo / "scripts/p6").glob("*/spec.json")):
        name = spec.parent.name
        resolved = export_root / name / "resolved.json"
        if not resolved.exists():
            continue
        a = json.loads(resolved.read_text(encoding="utf-8")).get("authored") or {}
        if a.get("spec_sha256") != _sha(spec):
            bad.append(f"fresh: {name}'s export predates scripts/p6/{name}/spec.json")
            continue
        cfg = json.loads(spec.read_text(encoding="utf-8"))
        for table, key in (("motions", "scripts_sha256"), ("menu_motions", "menu_scripts_sha256")):
            for index, entry in (cfg.get(table) or {}).items():
                if "script" in entry:
                    want = (a.get(key) or {}).get(str(int(index, 0)))
                    if want != _sha(repo / entry["script"]):
                        bad.append(f"fresh: {name}'s export predates {entry['script']}")
        audio = repo / "scripts/p6" / name / "audio.json"
        if audio.exists() and a.get("audio_sha256") not in (None, _sha(audio)):
            bad.append(f"fresh: {name}'s export predates its audio.json")
        for role, entry in (cfg.get("files") or {}).items():
            src = repo / entry["from"]
            want = (a.get("files_sha256") or {}).get(role)
            if role != "main" and src.exists() and want is not None and want != _sha(src):
                bad.append(f"fresh: {name}'s export predates {entry['from']}")
    return bad


def run(repo: Path, files: list[str] | None = None, derived: dict[str, str] | None = None) -> list[str]:
    files = tracked(repo) if files is None else files
    if derived is None:
        derived = {}
        for r in RIPS + LOCAL_ART:
            derived.update(digests(repo / r))
    return (check_tracked(files) + check_audio(files) + check_images(files)
            + check_copies(repo, files, derived) + check_dumps(repo, files) + check_ignores(repo)
            + check_fresh(repo, repo / "builds/p4-staging/export-x"))


def self_test() -> int:
    """Each check red on a planted fault, green without it."""
    failures = []
    with tempfile.TemporaryDirectory() as tmp:
        repo = Path(tmp)
        (repo / ".gitignore").write_text("/builds/\n/assets/\n" + "\n".join("/" + r + "/" for r in RIPS) + "\n",
                                         encoding="utf-8")
        (repo / "art/p6/x").mkdir(parents=True)
        (repo / "art/p6/x/recipe.json").write_text('{"anchors": [1, 2, 3]}', encoding="utf-8")
        clean = [".gitignore", "art/p6/x/recipe.json"]
        if run(repo, clean, {}):
            failures.append(f"clean tree reported {run(repo, clean, {})}")
        planted = {
            "tracked": (clean + [RIPS[0] + "/Palutena/gate/gate.png"], {}),
            "audio": (clean + ["scripts/p6/x.wav"], {}),
            "images": (clean + ["art/p6/x/face.png"], {}),
        }
        for check, (files, derived) in planted.items():
            got = run(repo, files, derived)
            if not any(m.startswith(check + ":") for m in got):
                failures.append(f"{check}: planted fault not reported ({got})")
        blob = b"kit texels"
        (repo / "art/p6/x/copy.bin").write_bytes(blob)
        got = run(repo, clean + ["art/p6/x/copy.bin"], {hashlib.sha256(blob).hexdigest(): "kit/face.png"})
        if not any(m.startswith("copies:") for m in got):
            failures.append(f"copies: planted fault not reported ({got})")
        (repo / "art/p6/x/dump.json").write_text(json.dumps({"vertices": list(range(DUMP_NUMBERS + 1))}),
                                                 encoding="utf-8")
        got = run(repo, clean + ["art/p6/x/dump.json"], {})
        if not any(m.startswith("dumps:") for m in got):
            failures.append(f"dumps: planted fault not reported ({got})")
        (repo / ".gitignore").write_text("/builds/\n", encoding="utf-8")
        got = run(repo, clean, {})
        if not any(m.startswith("ignores:") for m in got):
            failures.append(f"ignores: planted fault not reported ({got})")
        spec = repo / "scripts/p6/x/spec.json"
        spec.parent.mkdir(parents=True)
        spec.write_text("{}", encoding="utf-8")
        export = repo / "builds/p4-staging/export-x/x"
        export.mkdir(parents=True)
        (export / "resolved.json").write_text(json.dumps({"authored": {"spec_sha256": "stale"}}),
                                              encoding="utf-8")
        got = check_fresh(repo, repo / "builds/p4-staging/export-x")
        if not any(m.startswith("fresh:") for m in got):
            failures.append(f"fresh: planted fault not reported ({got})")
    for f in failures:
        print("SELF-TEST FAIL", f)
    print(f"check_p6_art self-test: {7 - len(failures)} of 7 checks red on their planted fault")
    return 1 if failures else 0


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--self-test", action="store_true")
    args = ap.parse_args()
    if args.self_test:
        return self_test()
    problems = run(REPO)
    for p in problems:
        print(p)
    print(f"check_p6_art: {len(problems)} problem(s)")
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
