#!/usr/bin/env python3
"""P4 source adapter, stage 0: build the pinned Remix reference ROM in staging.

decomp/smashremix is read-only, and the donor build writes files, so this
exports the pinned tree (git archive of the submodule HEAD, which must equal
the source lock) into a fresh directory under builds/, applies the donor's own
xdelta to the user's vanilla ROM and assembles main.asm with the donor's own
bass, exactly as patch.bat / "xdelta - apply original.bat" do, adding only
bass's `-sym` symbol log. The reference payload is unchanged by that flag.

Outputs stay local: every ROM-derived file is ignored (/builds/) and must
never be committed. staging-manifest.json records the input and output hashes;
a second staging of the same pins must reproduce them bit-for-bit.
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


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--rom", type=Path, required=True, help="vanilla SSB64 US z64")
    ap.add_argument("--dest", type=Path, required=True, help="fresh directory under builds/")
    args = ap.parse_args()

    lock = json.loads(LOCK.read_text(encoding="utf-8"))
    pin = next(s["commit"] for s in lock["submodules"] if s["path"] == "decomp/smashremix")
    head = subprocess.run([GIT, "-C", str(REMIX), "rev-parse", "HEAD"], capture_output=True,
                          text=True, check=True).stdout.strip()
    if head != pin:
        raise SystemExit(f"decomp/smashremix HEAD {head} != source-lock pin {pin}")
    dirty = subprocess.run([GIT, "-C", str(REMIX), "status", "--porcelain"], capture_output=True,
                           text=True, check=True).stdout.strip()
    if dirty:
        raise SystemExit("decomp/smashremix is not clean")
    rom_sha1 = sha(args.rom, "sha1")
    if rom_sha1 != VANILLA_US_SHA1:
        raise SystemExit(f"{args.rom}: SHA1 {rom_sha1} is not the supported US v1.0 z64 image")

    dest = args.dest.resolve()
    if dest.exists() and any(dest.iterdir()):
        raise SystemExit(f"{dest} is not empty; staging is disposable, use a fresh directory")
    dest.mkdir(parents=True, exist_ok=True)
    archive = subprocess.run([GIT, "-C", str(REMIX), "archive", "--format=tar", "HEAD"],
                             capture_output=True, check=True).stdout
    tar = dest / "_tree.tar"
    tar.write_bytes(archive)
    run(["tar", "-xf", tar.name], dest)
    tar.unlink()
    shutil.copyfile(args.rom, dest / "roms" / "ssb.rom")
    run([str(dest / "xdelta.exe"), "-d", "-f", "-s", "roms/ssb.rom", "original.xdelta",
         "roms/original.z64"], dest)
    run([str(dest / "assembler" / "bass.exe"), "-o", "ssb64asm.z64", "main.asm", "-sym",
         "logfile.log"], dest, dest / "bass.out")

    files = ["roms/ssb.rom", "original.xdelta", "xdelta.exe", "assembler/bass.exe",
             "roms/original.z64", "ssb64asm.z64", "logfile.log"]
    manifest = {
        "schema": "p4-remix-staging-v1",
        "remix_commit": pin,
        "outputs": {
            "assets": "roms/original.z64 (xdelta-patched files; reloc table grown in place)",
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
