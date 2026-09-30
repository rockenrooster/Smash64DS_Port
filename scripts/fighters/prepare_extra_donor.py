#!/usr/bin/env python3
"""Resolve the pinned Meta Knight EXTRA donor in an isolated builds directory.

preflight reads and validates inputs; stage copies tracked input bytes; resolve
runs the pinned appender and bundled assembler in that copy. No command writes
to either reference checkout, and no dependency is installed globally. Outputs
remain donor data, not an admitted DS fighter or executable runtime backend.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
from typing import Any


ROOT = Path(__file__).resolve().parents[2]
ROM_SIZE = 16_777_216
ROM_SHA1 = "e2929e10fccc0aa84e5776227e798abc07cedabf"
ROM_MAGIC = bytes.fromhex("80371240")
CHARACTER = "MetaKnight"
# Import closure of character_appender.py: its core, stage, audio and SSB modules
# use these packages plus the standard library. GUI/editor entry points using
# pandas/PySide6 are outside this command's closure.
APPENDER_PACKAGES = ("lineinfile", "pillow", "pyyaml")
MANIFEST_NAME = "donor-manifest.json"


class AdmissionError(RuntimeError):
    pass


def run(argv: list[str], cwd: Path | None = None) -> str:
    result = subprocess.run(argv, cwd=cwd, capture_output=True, text=True,
                            encoding="utf-8", errors="replace", check=False)
    if result.returncode:
        raise AdmissionError(f"command failed ({result.returncode}): {argv!r}\n"
                             f"{result.stdout}{result.stderr}")
    return result.stdout.strip()


def git_path(value: str) -> Path:
    # MSYS Git can return drive paths or mount aliases such as /home/<user>.
    if os.name == "nt" and value.startswith("/"):
        converter = shutil.which("cygpath")
        if converter:
            value = run([converter, "-w", value])
        elif re.match(r"^/[A-Za-z]/", value):
            value = value[1].upper() + ":/" + value[3:]
        else:
            raise AdmissionError(f"cannot translate Git POSIX path without cygpath: {value}")
    return Path(value).resolve()


def digest(path: Path, algorithm: str = "sha256") -> str:
    hasher = hashlib.new(algorithm)
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            hasher.update(chunk)
    return hasher.hexdigest()


def file_record(path: Path, relative: str | None = None) -> dict[str, Any]:
    if not path.is_file() or path.is_symlink():
        raise AdmissionError(f"missing or symlinked input file: {path}")
    return {"path": relative if relative is not None else str(path),
            "size": path.stat().st_size, "sha256": digest(path)}


def destination(path: Path, root: Path = ROOT) -> Path:
    candidate = path.resolve()
    boundary = (root / "builds").resolve()
    if candidate == boundary or not candidate.is_relative_to(boundary):
        raise AdmissionError(f"destination must be a child of {boundary}: {candidate}")
    # resolve() follows existing junctions/symlinks, preventing a builds alias
    # from sending staging or cleanup outside this checkout's builds directory.
    if not boundary.is_relative_to(root.resolve()):
        raise AdmissionError(f"builds directory escapes workspace: {boundary}")
    if any(part.lower() == "decomp" for part in candidate.relative_to(root).parts):
        raise AdmissionError(f"staging in decomp is forbidden: {candidate}")
    return candidate


def verify_repository(root: Path, expected_pin: str) -> dict[str, Any]:
    root = root.resolve()
    actual_top = git_path(run(["git", "-C", str(root), "rev-parse", "--show-toplevel"]))
    if actual_top != root:
        raise AdmissionError(f"not an initialized repository root: {root}; Git found {actual_top}")
    actual_pin = run(["git", "-C", str(root), "rev-parse", "HEAD"])
    if actual_pin != expected_pin:
        raise AdmissionError(f"wrong source pin for {root}: {actual_pin}; expected {expected_pin}")
    dirty = run(["git", "-C", str(root), "status", "--porcelain=v1",
                 "--untracked-files=all", "--ignore-submodules=none"])
    if dirty:
        raise AdmissionError(f"source checkout is not clean: {root}\n{dirty}")
    entries = run(["git", "-C", str(root), "ls-files", "--stage", "-z"])
    files = []
    gitlinks = []
    for entry in entries.split("\0"):
        if not entry:
            continue
        metadata, relative = entry.split("\t", 1)
        mode, blob, stage = metadata.split()
        if stage != "0":
            raise AdmissionError(f"unmerged input: {root / relative}")
        if mode == "160000":
            gitlinks.append({"path": relative, "commit": blob})
            continue
        if mode not in ("100644", "100755"):
            raise AdmissionError(f"unsupported source mode {mode}: {root / relative}")
        relative_path = Path(relative)
        if relative_path.is_absolute() or ".." in relative_path.parts:
            raise AdmissionError(f"escaping tracked input: {relative}")
        files.append(file_record(root / relative, relative))
    if not files:
        raise AdmissionError(f"empty source inventory: {root}")
    return {"root": str(root), "commit": actual_pin, "clean": True,
            "files": files, "gitlinks": gitlinks}


def verify_rom(path: Path) -> dict[str, Any]:
    record = file_record(path.resolve())
    if record["size"] != ROM_SIZE:
        raise AdmissionError(f"wrong source ROM size: {record['size']}; expected {ROM_SIZE}")
    with path.open("rb") as stream:
        if stream.read(4) != ROM_MAGIC:
            raise AdmissionError("source ROM must be the big-endian NTSC-U v1.0 dump")
    record["sha1"] = digest(path, "sha1")
    if record["sha1"] != ROM_SHA1:
        raise AdmissionError(f"wrong source ROM SHA-1: {record['sha1']}; expected {ROM_SHA1}")
    return record


def verify_character(extra: Path) -> list[dict[str, Any]]:
    folder = extra / "extra_characters" / CHARACTER
    names = ("config.yaml", "main.asm", "MetaKnightSpecial.asm", "CPU.asm",
             "main.bin", "character.bin", "main_reqlist.txt", "portrait.png",
             "portrait_flash.png", "nameplate.png", "nameplate_singleplayer.png",
             "victory_theme.bin", "transparency.txt")
    records = [file_record(folder / name, f"extra_characters/{CHARACTER}/{name}")
               for name in names]
    main = "\n".join(line.split("//", 1)[0] for line in
                     (folder / "main.asm").read_text(encoding="utf-8").splitlines())
    animations = {p.stem.upper(): p for p in (folder / "animations").glob("*.bin")}
    for animation in sorted(set(re.findall(r"\bFile\.METAKNIGHT_ANIM_(\w+)", main))):
        if animation not in animations:
            raise AdmissionError(f"missing Meta Knight animation: {animation}")
    for subdir in ("animations", "moveset", "sounds"):
        payloads = sorted(p for p in (folder / subdir).iterdir() if p.is_file())
        if not payloads:
            raise AdmissionError(f"empty Meta Knight asset directory: {folder / subdir}")
        records.extend(file_record(p, p.relative_to(extra).as_posix()) for p in payloads)
    for resource in re.findall(r'\binsert\s+\w+\s*,\s*"([^"]+)"', main):
        file_record(folder / resource)
    return records


def locked_requirements(lock_path: Path) -> str:
    lock = json.loads(lock_path.read_text(encoding="utf-8"))
    lines = []
    for package in APPENDER_PACKAGES:
        row = lock["default"].get(package)
        if not row or not re.fullmatch(r"==[^\s]+", row.get("version", "")):
            raise AdmissionError(f"missing exact package pin in Pipfile.lock: {package}")
        hashes = row.get("hashes", [])
        if not hashes or any(not re.fullmatch(r"sha256:[a-f0-9]{64}", h) for h in hashes):
            raise AdmissionError(f"missing valid package hashes in Pipfile.lock: {package}")
        lines.append(package + row["version"] + " " +
                     " ".join("--hash=" + h for h in hashes))
    return "\n".join(lines) + "\n"


def python_info(executable: str) -> dict[str, Any]:
    code = ("import json,sys,tkinter; print(json.dumps({'executable':sys.executable,"
            "'version':list(sys.version_info[:3])}))")
    info = json.loads(run([executable, "-c", code]))
    if tuple(info["version"]) < (3, 12):
        raise AdmissionError("pinned EXTRA appender requires Python >= 3.12 (PEP 701 f-strings)")
    info["binary"] = file_record(Path(info["executable"]))
    return info


def preflight(args: argparse.Namespace) -> dict[str, Any]:
    target = destination(args.dest)
    lock_path = ROOT / "docs/P4/source-lock.json"
    lock = json.loads(lock_path.read_text(encoding="utf-8"))
    pins = {row["path"]: row["commit"] for row in lock["submodules"]}
    extra = verify_repository(args.extra_root, pins["decomp/smashremix-plus-extra"])
    remix = verify_repository(args.remix_root, pins["decomp/smashremix"])
    nested = next((row for row in extra["gitlinks"] if row["path"] == "smashremix"), None)
    if not nested or nested["commit"] != remix["commit"] or nested["commit"] != pins[
            "decomp/smashremix-plus-extra/smashremix"]:
        raise AdmissionError("EXTRA nested Remix gitlink does not match the locked direct Remix")
    extra_root, remix_root = Path(extra["root"]), Path(remix["root"])
    for reference in (extra_root, remix_root):
        if target.is_relative_to(reference) or reference.is_relative_to(target):
            raise AdmissionError(f"staging destination overlaps a reference checkout: {reference}")
    character = verify_character(extra_root)
    tools = [file_record(remix_root / rel, rel) for rel in (
        "original.xdelta", "xdelta.exe", "assembler/bass.exe", "assembler/rn64crc.exe")]
    requirements = locked_requirements(extra_root / "Pipfile.lock")
    rom = verify_rom(args.rom) if args.rom else None
    return {"schema": "p4-extra-resolved-donor-v1", "phase": "preflight",
            "character": CHARACTER, "destination": str(target),
            "source_lock": file_record(lock_path), "sources": {"extra": extra, "remix": remix},
            "character_inputs": character, "tools": tools,
            "pipfile_lock": file_record(extra_root / "Pipfile.lock"),
            "requirements": requirements, "appender_packages": list(APPENDER_PACKAGES),
            "python": python_info(args.python), "rom": rom, "commands": [], "outputs": [],
            "acceptance": "donor resolution only; native admission and playability unverified"}


def save_manifest(path: Path, manifest: dict[str, Any]) -> None:
    path.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")


def copy_source(source: dict[str, Any], target: Path) -> None:
    root = Path(source["root"])
    for record in source["files"]:
        original = root / record["path"]
        if file_record(original, record["path"]) != record:
            raise AdmissionError(f"source changed after preflight: {original}")
        copied = target / record["path"]
        copied.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(original, copied)
        if file_record(copied, record["path"]) != record:
            raise AdmissionError(f"staged input differs from reference: {copied}")


def stage(manifest: dict[str, Any]) -> Path:
    if manifest["rom"] is None:
        raise AdmissionError("stage requires --rom; preflight without ROM remains read-only")
    target = destination(Path(manifest["destination"]))
    if target.exists() and any(target.iterdir()):
        raise AdmissionError(f"refusing to overwrite nonempty staging directory: {target}")
    target.mkdir(parents=True, exist_ok=True)
    donor = target / "extra"
    copy_source(manifest["sources"]["extra"], donor)
    copy_source(manifest["sources"]["remix"], donor / "smashremix")
    # EXTRA expects source folders even when --single_character suppresses stages.
    (donor / "extra_stages").mkdir(exist_ok=True)
    rom = Path(manifest["rom"]["path"])
    if verify_rom(rom) != manifest["rom"]:
        raise AdmissionError("source ROM changed after preflight")
    staged_rom = donor / "smashremix/roms/ssb.rom"
    shutil.copyfile(rom, staged_rom)
    if digest(staged_rom) != manifest["rom"]["sha256"]:
        raise AdmissionError("staged ROM differs from source")
    (target / "appender-requirements.txt").write_text(manifest["requirements"], encoding="utf-8")
    manifest["phase"] = "staged"
    manifest["adaptations"] = []
    save_manifest(target / MANIFEST_NAME, manifest)
    return target


def execute(argv: list[str], cwd: Path, manifest: dict[str, Any], target: Path,
            label: str) -> None:
    log = target / f"{label}-{len(manifest['commands']):02d}.log"
    command = {"label": label, "argv": argv, "cwd": str(cwd), "log": str(log), "status": "running"}
    manifest["commands"].append(command)
    save_manifest(target / MANIFEST_NAME, manifest)
    env = os.environ.copy()
    env["PYTHONNOUSERSITE"] = "1"
    env["PYTHONDONTWRITEBYTECODE"] = "1"
    with log.open("w", encoding="utf-8") as stream:
        result = subprocess.run(argv, cwd=cwd, stdout=stream, stderr=subprocess.STDOUT,
                                env=env, check=False)
    command.update(status="passed" if result.returncode == 0 else "failed",
                   returncode=result.returncode, log_sha256=digest(log))
    save_manifest(target / MANIFEST_NAME, manifest)
    if result.returncode:
        tail = "\n".join(log.read_text(encoding="utf-8", errors="replace").splitlines()[-20:])
        raise AdmissionError(f"{label} failed ({result.returncode}); see {log}\n{tail}")


def compare_review_payload(reference: bytes, review: bytes) -> dict[str, Any]:
    # Appended data changes CRC words, not the rest of the original image.
    if len(review) <= len(reference) or review[:0x10] != reference[:0x10] or review[
            0x18:len(reference)] != reference[0x18:]:
        raise AdmissionError("review export modified the existing donor payload")
    return {"reference_bytes": len(reference), "review_bytes": len(review),
            "excluded_header_span": [0x10, 0x18], "existing_payload_identical": True}


def resolve(current: dict[str, Any]) -> Path:
    target = destination(Path(current["destination"]))
    manifest_path = target / MANIFEST_NAME
    if not manifest_path.is_file():
        raise AdmissionError("resolve requires an existing successful --phase stage")
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    if manifest["phase"] != "staged":
        raise AdmissionError(f"resolve requires fresh staged inputs, found {manifest['phase']}; use a new destination")
    for field in ("sources", "source_lock", "pipfile_lock", "rom", "python", "requirements"):
        if current[field] != manifest[field]:
            raise AdmissionError(f"inputs changed since stage: {field}")
    donor = target / "extra"
    for name, location in (("extra", donor), ("remix", donor / "smashremix")):
        for record in manifest["sources"][name]["files"]:
            if file_record(location / record["path"], record["path"]) != record:
                raise AdmissionError(f"staged source changed before resolve: {location / record['path']}")
    if digest(donor / "smashremix/roms/ssb.rom") != manifest["rom"]["sha256"]:
        raise AdmissionError("staged ROM changed before resolve")
    if os.name != "nt":
        raise AdmissionError("the pinned bundled .exe donor tools require Windows for this recipe")
    manifest["phase"] = "resolving"
    save_manifest(manifest_path, manifest)
    python = manifest["python"]["executable"]
    venv = target / ".venv"
    execute([python, "-m", "venv", str(venv)], target, manifest, target, "venv")
    staged_python = venv / "Scripts/python.exe"
    execute([str(staged_python), "-m", "pip", "--isolated", "install", "--require-hashes",
             "--only-binary=:all:", "--no-deps", "-r", str(target / "appender-requirements.txt")],
            target, manifest, target, "dependencies")
    # Reconstruct original.z64 explicitly with safe argv. The appender's own
    # fallback uses an unquoted command string and reports xdelta failure as OK.
    execute([str(donor / "smashremix/xdelta.exe"), "-d", "-f", "-s",
             str(donor / "smashremix/roms/ssb.rom"), str(donor / "smashremix/original.xdelta"),
             str(donor / "smashremix/roms/original.z64")], donor, manifest, target, "xdelta")
    execute([str(staged_python), "character_appender.py", "--single_character", CHARACTER],
            donor, manifest, target, "appender")
    execute([str(donor / "smashremix/assembler/bass.exe"), "-o", "ssb64asm_extra.z64",
             "main.asm", "-sym", "logfile.log"], donor, manifest, target, "assembler")
    execute([str(donor / "smashremix/assembler/rn64crc.exe"), "-u", "ssb64asm_extra.z64"],
            donor, manifest, target, "crc")
    return complete_review(target, manifest, preserve_reference=True)


def complete_review(target: Path, manifest: dict[str, Any],
                    preserve_reference: bool) -> Path:
    donor = target / "extra"
    manifest_path = target / MANIFEST_NAME
    python = manifest["python"]["executable"]
    # Preserve the original donor build before adding the review-only export.
    # Bass symbols omit origin constants; the appended typed table carries them.
    import extra_resolved_actions as review_export
    reference_main = donor / "main.reference.asm"
    if preserve_reference:
        shutil.copyfile(donor / "main.asm", reference_main)
        shutil.copyfile(donor / "ssb64asm_extra.z64", donor / "ssb64asm_extra_reference.z64")
        shutil.copyfile(donor / "logfile.log", donor / "reference-symbols.log")
    include = review_export.install_review_export(donor, ROOT)
    manifest["adaptations"].append({"kind": "review_export_append",
                                     "producer": file_record(Path(review_export.__file__)),
                                     "include": file_record(include),
                                     "main_before": file_record(reference_main),
                                     "main_after": file_record(donor / "main.asm"),
                                     "purpose": "typed offsets only; gameplay payload unchanged"})
    save_manifest(manifest_path, manifest)
    execute([str(donor / "smashremix/assembler/bass.exe"), "-o", "ssb64asm_extra_review.z64",
             "main.asm", "-sym", "review-symbols.log"], donor, manifest, target, "review-assembler")
    execute([str(donor / "smashremix/assembler/rn64crc.exe"), "-u", "ssb64asm_extra_review.z64"],
            donor, manifest, target, "review-crc")
    reference = (donor / "ssb64asm_extra_reference.z64").read_bytes()
    review = (donor / "ssb64asm_extra_review.z64").read_bytes()
    # CRC/header words may differ after appending; every other existing byte
    # must be identical before this review image can serve as table evidence.
    manifest["review_payload_comparison"] = compare_review_payload(reference, review)
    execute([python, str(ROOT / "scripts/fighters/extra_resolved_actions.py"),
             "--rom", str(donor / "ssb64asm_extra_review.z64"),
             "--symbols", str(donor / "review-symbols.log"),
             "--output", str(target / "resolved-actions.json")],
            target, manifest, target, "review-extract")
    outputs = [donor / rel for rel in (
        "smashremix/roms/original.z64", "smashremix/roms/original_extra.z64",
        "ssb64asm_extra.z64", "logfile.log", "main.asm", "main.reference.asm",
        "ssb64asm_extra_reference.z64", "reference-symbols.log",
        "ssb64asm_extra_review.z64", "review-symbols.log")]
    outputs.extend((include, target / "resolved-actions.json"))
    # Appender metadata, edited source, and sound-substituted motions/events are
    # part of the resolved input, alongside both ROM roles and assembler symbols.
    for subdir in ("build", "src"):
        outputs.extend(sorted(p for p in (donor / subdir).rglob("*") if p.is_file()))
    manifest["outputs"] = [file_record(p, p.relative_to(target).as_posix()) for p in outputs]
    manifest["output_roles"] = {"assets": "extra/smashremix/roms/original_extra.z64",
                                "actions_scripts": "extra/ssb64asm_extra_reference.z64",
                                "symbols": "extra/reference-symbols.log",
                                "review_tables": "extra/ssb64asm_extra_review.z64",
                                "resolved_actions": "resolved-actions.json"}
    manifest["phase"] = "resolved"
    save_manifest(manifest_path, manifest)
    return target


def resume_review(current: dict[str, Any]) -> Path:
    """Repair only a failed review producer, preserving the completed donor."""
    target = destination(Path(current["destination"]))
    manifest = json.loads((target / MANIFEST_NAME).read_text(encoding="utf-8"))
    for field in ("sources", "source_lock", "pipfile_lock", "rom", "python", "requirements"):
        if current[field] != manifest[field]:
            raise AdmissionError(f"inputs changed since donor resolution: {field}")
    commands = manifest.get("commands", [])
    if (manifest["phase"] != "resolving" or not commands or
            commands[-1].get("status") != "failed" or
            commands[-1].get("label", Path(commands[-1]["log"]).stem) != "review-assembler"):
        raise AdmissionError("resume-review requires a recorded failed review assembler")
    donor = target / "extra"
    reference = donor / "ssb64asm_extra_reference.z64"
    if digest(reference) != digest(donor / "ssb64asm_extra.z64"):
        raise AdmissionError("preserved reference differs from completed donor output")
    if not any(command.get("label", Path(command["log"]).stem) == "crc" and
               command["status"] == "passed" for command in commands):
        raise AdmissionError("reference donor has no completed CRC command")
    for index, command in enumerate(commands):
        if any(other["log"] == command["log"] for other in commands[index + 1:]):
            if command["status"] != "failed":
                raise AdmissionError("a successful donor log was overwritten")
            command["log_state"] = "overwritten by prior review retry; failure not reused as proof"
            continue
        if digest(Path(command["log"])) != command["log_sha256"]:
            raise AdmissionError("recorded donor command log changed")
    adaptation = manifest["adaptations"][0]
    if file_record(donor / "main.reference.asm") != adaptation["main_before"]:
        raise AdmissionError("preserved reference assembly changed")
    manifest["resume_reason"] = "repair review metadata producer; reference donor remains frozen"
    return complete_review(target, manifest, preserve_reference=False)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--phase", choices=("preflight", "stage", "resolve", "resume-review"), default="preflight")
    parser.add_argument("--extra-root", type=Path, required=True)
    parser.add_argument("--remix-root", type=Path, required=True)
    parser.add_argument("--rom", type=Path, help="owner-provided NTSC-U v1.0 big-endian ROM")
    parser.add_argument("--dest", type=Path, default=ROOT / "builds/p4/meta-knight-donor")
    parser.add_argument("--python", default=sys.executable, help="Python >=3.12 with tkinter")
    args = parser.parse_args(argv)
    try:
        manifest = preflight(args)
        if args.phase == "preflight":
            print(json.dumps({"phase": "preflight", "character": CHARACTER,
                              "destination": manifest["destination"],
                              "sources": {key: {k: value[k] for k in ("root", "commit", "clean")}
                                          for key, value in manifest["sources"].items()},
                              "input_files": sum(len(s["files"]) for s in manifest["sources"].values()),
                              "python": manifest["python"], "rom": manifest["rom"],
                              "appender_packages": list(APPENDER_PACKAGES),
                              "ready_to_stage": manifest["rom"] is not None}, indent=2))
        else:
            target = (stage(manifest) if args.phase == "stage" else
                      resume_review(manifest) if args.phase == "resume-review" else resolve(manifest))
            print(f"{args.phase} passed: {target / MANIFEST_NAME}")
        return 0
    except (AdmissionError, OSError, ValueError, KeyError) as error:
        print(f"EXTRA donor admission failed: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
