#!/usr/bin/env python3
"""Reject reference graphics code in ARM9 link inputs before ROM packaging.

This checks linked symbols AND compiler dependency inputs. Removing the rejected
code and completing its native callers remain separate implementation work.
"""
import argparse
import re
import shlex
import struct
from pathlib import Path

FORBIDDEN = frozenset((
    "ndsRendererScanList", "ndsRendererScanColdCommand",
    "ndsRendererScanColdStateOpcode", "ndsRendererScanDisplayList",
    "ndsRendererExecuteDisplayList", "ndsRendererExecuteDisplayListWithVertexCache",
    "ndsRendererApplyVertexCommand", "ndsRendererExecuteTriangleCommand",
    "ndsRendererExecuteDirectRawRemainder", "ndsRendererExecuteFastRawCurrentRun",
    "ndsRendererDirectRawFindPlan", "ndsRendererFastRawFallbackCommand",
    "ndsDrawSObjIntoPreview", "ndsFighterDLDrawTriangle",
))
# These are the exact SDK archive leaves extracted by the existing Task 9/37
# rules, not project translation units. Their archive/code pins remain enforced
# by those rules and verifiers; every project executable input needs its .d file.
SDK_OBJECTS = frozenset(stem + suffix for stem in (
    "_arm_addsubsf3", "_arm_muldivsf3", "_arm_cmpsf2", "_arm_unordsf2",
    "_arm_fixsfsi", "_arm_fixunssfsi", "libc_a-memset", "libc_a-memcpy",
    "libc_a-memcpy-stub", "libc_a-memcmp", "libm_a-ef_sqrt",
) for suffix in (".itcm.o", ".mainram.o"))


def resolve_path(value, base):
    if re.match(r"^/[A-Za-z]/", value):
        value = value[1] + ":" + value[2:]
    path = Path(value)
    return (path if path.is_absolute() else base / path).resolve()


def elf_symbols(path, expected_type=None):
    data = path.read_bytes()
    if data[:6] != b"\x7fELF\x01\x01" or len(data) < 52:
        raise ValueError(f"not an ELF32 little-endian input: {path}")
    if struct.unpack_from("<H", data, 18)[0] != 40:
        raise ValueError(f"not an ARM input: {path}")
    if expected_type is not None and struct.unpack_from("<H", data, 16)[0] != expected_type:
        raise ValueError(f"wrong ELF type (expected {expected_type}): {path}")
    offset = struct.unpack_from("<I", data, 32)[0]
    size, count = struct.unpack_from("<HH", data, 46)
    if size < 40 or offset + size * count > len(data):
        raise ValueError(f"invalid ELF section table: {path}")
    sections = [struct.unpack_from("<10I", data, offset + size * i) for i in range(count)]
    names = []
    executable = any(row[2] & 4 and row[5] for row in sections)
    for row in sections:
        if row[1] != 2:
            continue
        strings = sections[row[6]]
        table = data[strings[4]:strings[4] + strings[5]]
        for pos in range(row[4], row[4] + row[5], row[9] or 16):
            name, _, _, _, _, section = struct.unpack_from("<IIIBBH", data, pos)
            if section and name < len(table):
                names.append(table[name:table.find(b"\0", name)].decode("utf-8", "replace"))
    if not names:
        raise ValueError(f"missing auditable symbol table: {path}")
    return names, executable


def dependency_inputs(path, base):
    text = path.read_text(encoding="utf-8").replace("\\\n", " ")
    # GCC accepts Windows paths with literal backslashes as well as MSYS paths.
    # Preserve Make escapes (spaces/#/$), but do not let shlex eat separators.
    text = re.sub(r"\\(?![\s\\#$])", "/", text)
    first_rule = text.split("\n\n", 1)[0]
    match = re.search(r":\s", first_rule)
    if not match:
        raise ValueError(f"invalid compiler dependency file: {path}")
    return [resolve_path(token, base) for token in shlex.split(first_rule[match.end():])
            if not token.endswith(":")]


def forbidden_definitions(path):
    text = path.read_text(encoding="utf-8", errors="replace")
    # Preserve line positions; comments and literals cannot define functions.
    text = re.sub(r'/\*.*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'',
                  lambda m: "\n" * m[0].count("\n"), text, flags=re.S)
    found = []
    for name in FORBIDDEN:
        for match in re.finditer(r"\b" + re.escape(name) + r"\s*\(", text):
            pos, depth = match.end(), 1
            while pos < len(text) and depth:
                depth += (text[pos] == "(") - (text[pos] == ")")
                pos += 1
            if not depth and text[pos:].lstrip().startswith("{"):
                found.append(name)
    return found


def audit(elf, objects, build_dir):
    failures, seen = [], set()
    if not objects:
        return ["empty actual link-input list"]
    for path, elf_type in [(elf, 2), *((obj, 1) for obj in objects)]:
        names, executable = elf_symbols(path, elf_type)
        for name in names:
            if name.split(".", 1)[0] in FORBIDDEN:
                failures.append(f"forbidden linked definition: {path.name}: {name}")
        if elf_type == 2:
            continue
        dep = path.with_suffix(".d")
        if not dep.exists():
            if executable and path.name not in SDK_OBJECTS:
                failures.append(f"executable input lacks compiler dependencies: {path}")
            continue
        for source in dependency_inputs(dep, build_dir):
            if source in seen:
                continue
            seen.add(source)
            if not source.exists():
                failures.append(f"missing build input: {source}")
                continue
            normalized = source.as_posix().lower()
            if "/host/graphics_reference/" in normalized:
                failures.append(f"host graphics implementation in ROM inputs: {source}")
            if source.suffix.lower() in (".c", ".cpp", ".h", ".inc"):
                for name in forbidden_definitions(source):
                    failures.append(f"forbidden implementation in compiler inputs: {source}: {name}")
    return sorted(set(failures))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--elf", required=True, type=Path)
    parser.add_argument("--objects-list", required=True, type=Path)
    parser.add_argument("--build-dir", required=True, type=Path)
    args = parser.parse_args()
    try:
        objects = [resolve_path(value, args.build_dir) for value in
                   shlex.split(args.objects_list.read_text(encoding="utf-8"))]
        failures = audit(args.elf.resolve(), objects, args.build_dir.resolve())
    except (OSError, ValueError, struct.error, IndexError) as error:
        failures = [str(error)]
    for failure in failures:
        print("NATIVE_ONLY_REJECT: " + failure)
    if failures:
        return 1
    print(f"NATIVE_ONLY_PASS: {args.elf.name}, {len(objects)} actual link inputs")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
