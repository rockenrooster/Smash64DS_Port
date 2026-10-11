#!/usr/bin/env python3
"""P4 source adapter, stage 0 for EXTRA (S12): build the pinned Remix EXTRA
reference ROM with the source lock's selected EXTRA characters in staging.

The EXTRA tree (decomp/smashremix-plus-extra) nests Remix at the same commit
as decomp/smashremix. Its gitlink is not checked out, so the nested tree comes
from decomp/smashremix's own HEAD once both pins are checked. Nothing under
decomp/ is written: both trees are exported (git archive) into a fresh
directory under builds/, which keeps only the lock's selected_extra_folders of
extra_characters/ and none of extra_stages/ (the appender then numbers only
their files, past Remix's), applies the donor's xdelta to the user's vanilla
ROM, runs EXTRA's own appender (character_appender.py, in an interpreter with
its Pipfile's packages: --python) and assembles main.asm with Remix's bass as
patch_extra.bat does, adding only bass's `-sym` log. The output is named like
the main staging's (ssb64asm.z64, logfile.log, src/, roms/ssb.rom and
roms/original.z64), so remix_export.py and the generators read either.

Outputs stay local: every ROM-derived file is ignored (/builds/) and must
never be committed. staging-manifest.json records the input and output hashes.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import shutil
import subprocess
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
EXTRA = REPO / "decomp" / "smashremix-plus-extra"
REMIX = REPO / "decomp" / "smashremix"
LOCK = REPO / "docs" / "P4" / "source-lock.json"
VANILLA_US_SHA1 = "e2929e10fccc0aa84e5776227e798abc07cedabf"
GIT = "git"


def sha(path: Path, algo: str) -> str:
    h = hashlib.new(algo)
    with open(path, "rb") as fh:
        for chunk in iter(lambda: fh.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def run(argv: list[str], cwd: Path, log: Path | None = None) -> None:
    out = subprocess.run(argv, cwd=cwd, capture_output=True)
    if log is not None:
        log.write_bytes(out.stdout + out.stderr)
    if out.returncode != 0:
        sys.stderr.write((out.stdout + out.stderr).decode(errors="replace")[-4000:])
        raise SystemExit(f"failed ({out.returncode}): {' '.join(argv)}")


def git(repo: Path, *args: str) -> str:
    return subprocess.run([GIT, "-C", str(repo), *args], capture_output=True, text=True,
                          check=True).stdout.strip()


def export_tree(repo: Path, dest: Path) -> None:
    dest.mkdir(parents=True, exist_ok=True)
    tar = dest / "_tree.tar"
    tar.write_bytes(subprocess.run([GIT, "-C", str(repo), "archive", "--format=tar", "HEAD"],
                                   capture_output=True, check=True).stdout)
    run(["tar", "-xf", tar.name], dest)
    tar.unlink()


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--rom", type=Path, required=True, help="vanilla SSB64 US z64")
    ap.add_argument("--dest", type=Path, required=True, help="fresh directory under builds/")
    ap.add_argument("--python", default=sys.executable,
                    help="interpreter with EXTRA's Pipfile packages (pyyaml, lineinfile, pillow, pandas)")
    args = ap.parse_args()

    lock = json.loads(LOCK.read_text(encoding="utf-8"))
    pins = {s["path"]: s["commit"] for s in lock["submodules"]}
    selected = list(lock["selected_extra_folders"])
    outer_pin = pins["decomp/smashremix-plus-extra"]
    nested_pin = pins["decomp/smashremix-plus-extra/smashremix"]
    if git(EXTRA, "rev-parse", "HEAD") != outer_pin:
        raise SystemExit(f"decomp/smashremix-plus-extra HEAD != source-lock pin {outer_pin}")
    gitlink = git(EXTRA, "ls-tree", "HEAD", "smashremix").split()
    if len(gitlink) < 3 or gitlink[1] != "commit" or gitlink[2] != nested_pin:
        raise SystemExit(f"EXTRA's smashremix gitlink {gitlink} != source-lock pin {nested_pin}")
    if git(REMIX, "rev-parse", "HEAD") != nested_pin:
        raise SystemExit(f"decomp/smashremix HEAD is not EXTRA's nested pin {nested_pin}")
    for repo in (EXTRA, REMIX):
        if git(repo, "status", "--porcelain"):
            raise SystemExit(f"{repo} is not clean")
    rom_sha1 = sha(args.rom, "sha1")
    if rom_sha1 != VANILLA_US_SHA1:
        raise SystemExit(f"{args.rom}: SHA1 {rom_sha1} is not the supported US v1.0 z64 image")

    dest = args.dest.resolve()
    if dest.exists() and any(dest.iterdir()):
        raise SystemExit(f"{dest} is not empty; staging is disposable, use a fresh directory")
    export_tree(EXTRA, dest)
    nested = dest / "smashremix"
    if nested.exists():
        shutil.rmtree(nested)
    export_tree(REMIX, nested)

    for folder in sorted((dest / "extra_characters").iterdir()):
        if folder.is_dir() and folder.name not in selected:
            shutil.rmtree(folder)
    missing = [name for name in selected if not (dest / "extra_characters" / name).is_dir()]
    if missing:
        raise SystemExit(f"selected EXTRA folders not in the tree: {missing}")
    for folder in sorted((dest / "extra_stages").iterdir()):
        if folder.is_dir():
            shutil.rmtree(folder)

    shutil.copyfile(args.rom, nested / "roms" / "ssb.rom")
    run([str(nested / "xdelta.exe"), "-d", "-f", "-s", "roms/ssb.rom", "original.xdelta",
         "roms/original.z64"], nested)
    # The generators read the vanilla image and Remix's patched one from
    # <staging>/roms, as in the main staging.
    (dest / "roms").mkdir(exist_ok=True)
    for name in ("ssb.rom", "original.z64"):
        shutil.copyfile(nested / "roms" / name, dest / "roms" / name)
    run([args.python, "character_appender.py"], dest, dest / "appender.out")
    run([str(nested / "assembler" / "bass.exe"), "-o", "ssb64asm.z64", "main.asm", "-sym",
         "logfile.log"], dest, dest / "bass.out")

    files = ["smashremix/roms/ssb.rom", "smashremix/original.xdelta", "smashremix/xdelta.exe",
             "smashremix/assembler/bass.exe", "smashremix/roms/original.z64",
             "smashremix/roms/original_extra.z64", "ssb64asm.z64", "logfile.log"]
    manifest = {
        "schema": "p4-extra-staging-v1",
        "extra_commit": outer_pin,
        "remix_commit": nested_pin,
        "characters": selected,
        "outputs": {
            "assets": "smashremix/roms/original_extra.z64 (Remix's files plus the appended ones)",
            "actions": "ssb64asm.z64 + logfile.log (assembled descriptors, tables, inserted scripts)",
        },
        "sha256": {f: sha(dest / f, "sha256") for f in files},
        "sha1": {f: sha(dest / f, "sha1") for f in files},
    }
    (dest / "staging-manifest.json").write_text(json.dumps(manifest, indent=1), encoding="utf-8")
    print(json.dumps(manifest["sha1"], indent=1))
    return 0


if __name__ == "__main__":
    sys.exit(main())
