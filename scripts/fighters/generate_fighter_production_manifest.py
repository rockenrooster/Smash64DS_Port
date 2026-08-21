#!/usr/bin/env python3
"""Build the P2-3 fighter-production inventory from BattleShip source data.

This is deliberately a *source reader*, not another hand-maintained fighter
table.  The BattleShip FTData initializer owns each fighter's core file graph,
FTMotionDesc owns the animation references, reloc_data_symbols.us.txt owns
named file IDs where the decomp has them, and the O2R resource headers own the
actual file IDs/extern dependency graph.

One decomp wart needs an explicit recovery rule: semantic FTMotionDesc aliases
such as llFTLuigiAnimDashFileID were introduced by BattleShip's refactor tool,
but many of those aliases are still STUBBED in reloc_data.us.h.  The original
numeric binding was not lost.  BattleShip's generated relocData source filename
binds semantic name to resource number (for example
`1104_FTLuigiAnimDash.c`), while `ll_1104_FileID` in the US symbol table binds
that resource number to the O2R file ID.  Joining those two source-owned facts
recovers the semantic ID without guessing.  Existing port values are recorded
beside the source-derived value so historical hand-recovered aliases can be
audited without silently becoming the production pipeline's source of truth.

The generated JSON is a build/tooling manifest.  It does not make a fighter
selectable by itself; later P2-3 slices consume it to stage files, generate
native owners, and build per-fighter acceptance inventories.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import struct
from collections import Counter
from pathlib import Path
from typing import Iterable


BOOTSTRAP_FIGHTERS = ("Mario", "Fox", "Luigi")
CORE_SLOT_NAMES = (
    "main",
    "mainmotion",
    "submotion",
    "model",
    "shieldpose",
    "special1",
    "special2",
    "special3",
    "special4",
)
ITEM_NAME_RE = re.compile(
    r"(?:Item|Sword|Bat|Harisen|StarRod|FireFlower|Hammer)", re.IGNORECASE
)


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def strip_line_comments(text: str) -> str:
    return re.sub(r"//[^\n]*", "", text)


def split_top_level_csv(text: str) -> list[str]:
    values: list[str] = []
    current: list[str] = []
    depth = 0
    for ch in text:
        if ch in "([{":
            depth += 1
        elif ch in ")]}":
            depth -= 1
            if depth < 0:
                raise ValueError("unbalanced initializer while parsing FTData")
        if ch == "," and depth == 0:
            values.append("".join(current).strip())
            current = []
        else:
            current.append(ch)
    if "".join(current).strip():
        values.append("".join(current).strip())
    if depth != 0:
        raise ValueError("unterminated initializer while parsing FTData")
    return values


def find_initializer(text: str, declaration: str) -> str:
    match = re.search(
        rf"{re.escape(declaration)}\s*=\s*\{{(.*?)\n\}};", text, re.DOTALL
    )
    if match is None:
        raise ValueError(f"source initializer not found: {declaration}")
    return match.group(1)


def load_named_symbols(path: Path) -> dict[str, int]:
    result: dict[str, int] = {}
    pattern = re.compile(r"^(\w+)\s*=\s*(0x[0-9A-Fa-f]+);", re.MULTILINE)
    for name, value in pattern.findall(path.read_text(encoding="utf-8")):
        result[name] = int(value, 16)
    return result


def load_port_symbol_values(path: Path) -> dict[str, int]:
    result: dict[str, int] = {}
    pattern = re.compile(
        r"^uintptr_t\s+(\w+)\s*=\s*(0x[0-9A-Fa-f]+|0)u;", re.MULTILINE
    )
    for name, value in pattern.findall(path.read_text(encoding="utf-8")):
        result[name] = int(value, 0)
    return result


def load_relocdata_semantic_ids(
    relocdata_root: Path,
    named_symbols: dict[str, int],
) -> dict[str, int]:
    """Recover semantic `ll<Name>FileID` values from generated source names.

    A file named `1104_FTLuigiAnimDash.c` is BattleShip's own declaration that
    reloc resource 1104 is `FTLuigiAnimDash`; the US symbol table independently
    gives `ll_1104_FileID = 0x450`.  Keeping the join here, in tooling, avoids
    baking another hand-maintained alias table into the DS runtime.
    """
    result: dict[str, int] = {}
    pattern = re.compile(r"^(\d+)_([A-Za-z0-9_]+)\.c$")
    for path in relocdata_root.glob("*.c"):
        match = pattern.match(path.name)
        if match is None:
            continue
        resource_number, semantic = match.groups()
        numeric_symbol = f"ll_{resource_number}_FileID"
        symbol = f"ll{semantic}FileID"
        if symbol in named_symbols:
            file_id = named_symbols[symbol]
        elif numeric_symbol in named_symbols:
            file_id = named_symbols[numeric_symbol]
        else:
            continue
        previous = result.get(symbol)
        if previous is not None and previous != file_id:
            raise ValueError(
                f"semantic reloc alias {symbol} maps to both "
                f"0x{previous:x} and 0x{file_id:x}"
            )
        result[symbol] = file_id
    return result


def read_o2r_record(path: Path, root: Path) -> dict[str, object] | None:
    raw = path.read_bytes()
    if len(raw) < 0x4C or raw[4:8] != b"OLER":
        return None
    file_id = struct.unpack_from("<I", raw, 0x40)[0]
    extern_count = struct.unpack_from("<I", raw, 0x48)[0]
    extern_end = 0x4C + extern_count * 2
    if extern_end > len(raw):
        raise ValueError(f"O2R extern table escapes file: {path}")
    extern_ids = list(struct.unpack_from(f"<{extern_count}H", raw, 0x4C)) \
        if extern_count else []
    return {
        "file_id": file_id,
        "path": path.relative_to(root).as_posix(),
        "bytes": len(raw),
        "sha256": hashlib.sha256(raw).hexdigest(),
        "extern_ids": extern_ids,
    }


def scan_o2r(root: Path) -> tuple[dict[int, dict[str, object]], dict[str, dict[str, object]]]:
    by_id: dict[int, dict[str, object]] = {}
    by_path: dict[str, dict[str, object]] = {}
    for path in root.rglob("*"):
        if not path.is_file():
            continue
        record = read_o2r_record(path, root)
        if record is None:
            continue
        file_id = int(record["file_id"])
        rel = str(record["path"])
        if file_id in by_id:
            raise ValueError(
                f"duplicate O2R file id 0x{file_id:x}: {by_id[file_id]['path']} / {rel}"
            )
        by_id[file_id] = record
        by_path[rel] = record
    return by_id, by_path


def symbol_from_value(value: str) -> str | None:
    value = value.strip()
    if value in ("0", "0x0", "0x00000000", "NULL"):
        return None
    match = re.fullmatch(r"&([A-Za-z0-9_]+FileID)", value)
    if match is None:
        raise ValueError(f"FTData file slot is not a file-id symbol or zero: {value}")
    return match.group(1)


def motion_symbols(ftdata_text: str, fighter: str) -> list[str]:
    block = find_initializer(ftdata_text, f"FTMotionDesc dFT{fighter}MotionDescs[]")
    return re.findall(r"&([A-Za-z0-9_]+FileID)", block)


def local_animation_alias_map(
    fighter: str,
    motion_refs: list[str],
    by_id: dict[int, dict[str, object]],
    semantic_ids: dict[str, int],
) -> dict[str, int]:
    prefix = f"llFT{fighter}Anim"
    aliases: list[str] = []
    for symbol in motion_refs:
        if symbol.startswith(prefix) and symbol not in aliases:
            aliases.append(symbol)

    local_o2r = {
        file_id: record
        for file_id, record in by_id.items()
        if str(record["path"]).startswith(f"reloc_animations/FT{fighter}Anim")
    }
    if len(aliases) != len(local_o2r):
        raise ValueError(
            f"{fighter}: source-local animation aliases ({len(aliases)}) != "
            f"O2R files ({len(local_o2r)}); semantic source recovery is not proven"
        )

    result: dict[str, int] = {}
    for alias in aliases:
        file_id = semantic_ids.get(alias)
        if file_id is None:
            raise ValueError(
                f"{fighter}: no generated relocData source binding for {alias}"
            )
        if file_id not in local_o2r:
            raise ValueError(
                f"{fighter}: {alias} -> 0x{file_id:x}, but that ID is not a "
                f"local FT{fighter}Anim O2R"
            )
        result[alias] = file_id
    return result


def resolve_symbol(
    symbol: str,
    named_symbols: dict[str, int],
    port_symbols: dict[str, int],
    semantic_ids: dict[str, int],
) -> int:
    if symbol in named_symbols:
        return named_symbols[symbol]
    if symbol in semantic_ids:
        return semantic_ids[symbol]
    value = port_symbols.get(symbol, 0)
    if value != 0:
        return value
    raise ValueError(f"unresolved file-id symbol: {symbol}")


def extern_closure(
    roots: Iterable[int], by_id: dict[int, dict[str, object]]
) -> list[int]:
    ordered: list[int] = []
    seen: set[int] = set()

    def visit(file_id: int) -> None:
        if file_id in seen:
            return
        seen.add(file_id)
        record = by_id.get(file_id)
        if record is None:
            raise ValueError(f"O2R dependency file id 0x{file_id:x} is missing")
        ordered.append(file_id)
        for dep in record["extern_ids"]:
            visit(int(dep))

    for file_id in roots:
        visit(file_id)
    return ordered


def asset_summary(file_id: int, by_id: dict[int, dict[str, object]]) -> dict[str, object]:
    record = by_id[file_id]
    return {
        "id": file_id,
        "id_hex": f"0x{file_id:x}",
        "path": record["path"],
        "bytes": record["bytes"],
        "sha256": record["sha256"],
    }


def build_fighter_manifest(
    fighter: str,
    ftdata_text: str,
    named_symbols: dict[str, int],
    port_symbols: dict[str, int],
    semantic_ids: dict[str, int],
    by_id: dict[int, dict[str, object]],
) -> dict[str, object]:
    data_block = find_initializer(ftdata_text, f"FTData dFT{fighter}Data")
    data_values = split_top_level_csv(strip_line_comments(data_block))
    if len(data_values) != 30:
        raise ValueError(f"{fighter}: FTData field count changed: {len(data_values)} != 30")

    refs = motion_symbols(ftdata_text, fighter)
    recovered = local_animation_alias_map(fighter, refs, by_id, semantic_ids)

    core: list[dict[str, object]] = []
    core_ids: list[int] = []
    for slot, value in zip(CORE_SLOT_NAMES, data_values[:9]):
        symbol = symbol_from_value(value)
        if symbol is None:
            core.append({"slot": slot, "symbol": None, "asset": None})
            continue
        file_id = resolve_symbol(symbol, named_symbols, port_symbols, semantic_ids)
        if file_id not in by_id:
            raise ValueError(f"{fighter}: core {slot} id 0x{file_id:x} has no O2R")
        core_ids.append(file_id)
        core.append({
            "slot": slot,
            "symbol": symbol,
            "asset": asset_summary(file_id, by_id),
        })

    use_counts = Counter(refs)
    unique_motion_symbols = list(dict.fromkeys(refs))
    motion_files: list[dict[str, object]] = []
    for symbol in unique_motion_symbols:
        file_id = resolve_symbol(symbol, named_symbols, port_symbols, semantic_ids)
        if file_id not in by_id:
            raise ValueError(
                f"{fighter}: motion symbol {symbol} resolves to missing O2R 0x{file_id:x}"
            )
        motion_files.append({
            "symbol": symbol,
            "uses": use_counts[symbol],
            "item_related": bool(ITEM_NAME_RE.search(symbol)),
            "local": symbol in recovered,
            "asset": asset_summary(file_id, by_id),
        })

    local_aliases = [
        {
            "symbol": symbol,
            "asset": asset_summary(file_id, by_id),
            "port_value": port_symbols.get(symbol, 0),
            "port_value_hex": f"0x{port_symbols.get(symbol, 0):x}",
            "port_matches_source": port_symbols.get(symbol, 0) == file_id,
            "was_stubbed_in_port": port_symbols.get(symbol, 0) == 0,
        }
        for symbol, file_id in recovered.items()
    ]
    core_closure = extern_closure(core_ids, by_id)
    motion_ids = [int(row["asset"]["id"]) for row in motion_files]
    nitrofs_ids = list(dict.fromkeys(core_closure + motion_ids))

    return {
        "fighter": fighter,
        "ftdata_symbol": f"dFT{fighter}Data",
        "motion_symbol": f"dFT{fighter}MotionDescs",
        "core": core,
        "core_extern_closure": [asset_summary(i, by_id) for i in core_closure],
        "local_animation_aliases": local_aliases,
        "motion_files": motion_files,
        "item_motion_file_count": sum(1 for row in motion_files if row["item_related"]),
        "nitrofs_file_count": len(nitrofs_ids),
        "nitrofs_files": [asset_summary(i, by_id) for i in nitrofs_ids],
    }


def parse_mariofox_makefile_files(makefile_text: str) -> set[str]:
    match = re.search(
        r"NDS_MARIOFOX_FIGHTER_RELOC_FILES\s*:=\s*\\\n(.*?)(?=\n\S)",
        makefile_text,
        re.DOTALL,
    )
    if match is None:
        raise ValueError("Makefile NDS_MARIOFOX_FIGHTER_RELOC_FILES block not found")
    return set(re.findall(r"\b(reloc_[A-Za-z0-9_./]+)", match.group(1)))


def verify_mariofox_makefile(
    repo_root: Path,
    fighters: list[dict[str, object]],
    by_id: dict[int, dict[str, object]],
) -> dict[str, object]:
    by_name = {str(row["fighter"]): row for row in fighters}
    current = parse_mariofox_makefile_files((repo_root / "Makefile").read_text(encoding="utf-8"))
    expected_ids = [0xA3]  # BattleShip FTManagerCommon, source manager prerequisite.
    for name in ("Mario", "Fox"):
        row = by_name[name]
        expected_ids.extend(int(a["id"]) for a in row["core_extern_closure"])
        expected_ids.extend(int(a["asset"]["id"]) for a in row["motion_files"])
    expected = {str(by_id[i]["path"]) for i in dict.fromkeys(expected_ids)}
    missing = sorted(expected - current)
    extra = sorted(current - expected)
    if missing or extra:
        raise ValueError(
            "source-derived Mario/Fox file set drifted from shipping Makefile: "
            f"missing={missing}, extra={extra}"
        )
    return {
        "shipping_makefile_file_count": len(current),
        "source_derived_file_count": len(expected),
        "exact_set_match": True,
    }


def build_manifest(repo_root: Path) -> dict[str, object]:
    ftdata_path = repo_root / "decomp/BattleShip-main/decomp/src/ft/ftdata.c"
    symbols_path = repo_root / "decomp/BattleShip-main/tools/reloc_data_symbols.us.txt"
    port_symbols_path = repo_root / "src/port/reloc_backend_ftdata_symbols.c"
    o2r_root = repo_root / "decomp/BattleShip-main/BattleShip_o2r"
    relocdata_root = repo_root / "decomp/BattleShip-main/decomp/src/relocData"

    ftdata_text = ftdata_path.read_text(encoding="utf-8")
    named_symbols = load_named_symbols(symbols_path)
    port_symbols = load_port_symbol_values(port_symbols_path)
    semantic_ids = load_relocdata_semantic_ids(relocdata_root, named_symbols)
    by_id, _by_path = scan_o2r(o2r_root)

    fighters = [
        build_fighter_manifest(
            fighter, ftdata_text, named_symbols, port_symbols, semantic_ids, by_id
        )
        for fighter in BOOTSTRAP_FIGHTERS
    ]

    # The local-file census is a structural guard.  The source-name join is the
    # authority; existing port aliases are audit evidence only because several
    # old fenced-special mappings predate this production pipeline.
    mario_local = len(fighters[0]["local_animation_aliases"])
    fox_local = len(fighters[1]["local_animation_aliases"])
    if (mario_local, fox_local) != (143, 158):
        raise ValueError(
            f"Mario/Fox alias proof census changed: {(mario_local, fox_local)}"
        )
    luigi = fighters[2]
    if len(luigi["local_animation_aliases"]) != 12:
        raise ValueError("Luigi local animation census changed from the source 12")
    if not all(row["was_stubbed_in_port"] for row in luigi["local_animation_aliases"]):
        raise ValueError("Luigi bootstrap expected all 12 local aliases to be unresolved")

    return {
        "schema": "smash64ds.p2-fighter-production-manifest.v1",
        "generated_by": "scripts/fighters/generate_fighter_production_manifest.py",
        "source": {
            "ftdata": str(ftdata_path.relative_to(repo_root)).replace("\\", "/"),
            "ftdata_sha256": sha256(ftdata_path),
            "reloc_symbols": str(symbols_path.relative_to(repo_root)).replace("\\", "/"),
            "reloc_symbols_sha256": sha256(symbols_path),
            "o2r_root": str(o2r_root.relative_to(repo_root)).replace("\\", "/"),
        },
        "alias_recovery_proof": {
            "rule": "generated relocData semantic filename + numeric US FileID",
            "mario_local_aliases": mario_local,
            "mario_existing_port_drift": sum(
                1 for row in fighters[0]["local_animation_aliases"]
                if (not row["was_stubbed_in_port"]) and (not row["port_matches_source"])
            ),
            "fox_local_aliases": fox_local,
            "fox_existing_port_drift": sum(
                1 for row in fighters[1]["local_animation_aliases"]
                if (not row["was_stubbed_in_port"]) and (not row["port_matches_source"])
            ),
            "luigi_aliases_recovered": len(luigi["local_animation_aliases"]),
        },
        "mariofox_shipping_file_set": verify_mariofox_makefile(
            repo_root, fighters, by_id
        ),
        "fighters": fighters,
    }


def render_json(manifest: dict[str, object]) -> str:
    return json.dumps(manifest, indent=2, sort_keys=False) + "\n"


def render_make_fragment(manifest: dict[str, object]) -> str:
    """Render only the *incremental* NitroFS files a fighter adds.

    Mario/Fox remain the P2-2 shipping baseline.  P2-3 fighters are layered on
    top of that set so a fighter's ROM cost is reviewable and shared resources
    (Luigi deliberately reuses Mario shield/special/item animation files) are
    not copied into a second hand-maintained list.
    """
    fighters = {
        str(row["fighter"]): row for row in manifest["fighters"]
    }
    baseline = {
        str(asset["path"])
        for fighter in (fighters["Mario"], fighters["Fox"])
        for asset in fighter["nitrofs_files"]
    }
    lines = [
        "# Auto-generated by scripts/fighters/generate_fighter_production_manifest.py.",
        "# Source of truth: BattleShip FTData + generated relocData + US O2R headers.",
        "# Do not edit by hand; run `make p2-fighter-production-manifest`.",
        "",
    ]
    for name in BOOTSTRAP_FIGHTERS[2:]:
        row = fighters[name]
        added = [
            str(asset["path"])
            for asset in row["nitrofs_files"]
            if str(asset["path"]) not in baseline
        ]
        variable = f"NDS_P2_{name.upper()}_FIGHTER_RELOC_FILES"
        if not added:
            lines.append(f"{variable} :=")
            lines.append("")
            continue
        lines.append(f"{variable} := \\")
        for index, path in enumerate(added):
            suffix = " \\" if index + 1 < len(added) else ""
            lines.append(f"\t{path}{suffix}")
        lines.append("")
    return "\n".join(lines)


def render_runtime_header(manifest: dict[str, object]) -> str:
    """Emit the narrow source-derived runtime catalog for admitted fighters.

    The C runtime receives *symbol address -> source O2R id* rows because that
    is the ABI BattleShip's relocation calls use: call sites pass `&ll...FileID`,
    not the integer stored in that variable.  Keeping those rows generated is
    what lets still-stubbed decomp aliases (Luigi has twelve) work without
    hand-patching `reloc_backend_ftdata_symbols.c`.
    """
    fighters = {
        str(row["fighter"]): row for row in manifest["fighters"]
    }
    baseline_paths = {
        str(asset["path"])
        for fighter in (fighters["Mario"], fighters["Fox"])
        for asset in fighter["nitrofs_files"]
    }
    lines = [
        "/* Auto-generated by scripts/fighters/generate_fighter_production_manifest.py. */",
        "/* Source: BattleShip FTData + generated relocData + US O2R headers. */",
        "/* Do not edit by hand; run `make p2-fighter-production-manifest`. */",
        "#ifndef SSB64_NDS_FIGHTER_PRODUCTION_GENERATED_H",
        "#define SSB64_NDS_FIGHTER_PRODUCTION_GENERATED_H",
        "",
    ]
    for name in BOOTSTRAP_FIGHTERS[2:]:
        fighter = fighters[name]
        prefix = f"NDS_P2_{name.upper()}"
        incremental_paths = {
            str(asset["path"])
            for asset in fighter["nitrofs_files"]
            if str(asset["path"]) not in baseline_paths
        }
        unique_core = [
            row for row in fighter["core"]
            if row["asset"] is not None
            and str(row["asset"]["path"]) in incremental_paths
        ]
        local_aliases = list(fighter["local_animation_aliases"])
        if not local_aliases:
            raise ValueError(f"{name}: runtime catalog requires local animation aliases")

        first_id = min(int(row["asset"]["id"]) for row in local_aliases)
        last_id = max(int(row["asset"]["id"]) for row in local_aliases)
        lines.extend([
            f"#define {prefix}_ANIM_FIRST 0x{first_id:x}u",
            f"#define {prefix}_ANIM_LAST 0x{last_id:x}u",
            f"#define {prefix}_ANIM_COUNT {len(local_aliases)}u",
            "",
        ])

        def append_rows(macro_name: str, rows: list[tuple[str, int, str]]) -> None:
            if not rows:
                lines.append(f"#define {macro_name}(X)")
                lines.append("")
                return
            lines.append(f"#define {macro_name}(X) \\")
            for index, (symbol, file_id, path) in enumerate(rows):
                suffix = " \\" if index + 1 < len(rows) else ""
                nitro_path = f"nitro:/reloc/{path}"
                lines.append(
                    f"    X({symbol}, 0x{file_id:x}u, \"{nitro_path}\"){suffix}"
                )
            lines.append("")

        core_rows = [
            (
                str(row["symbol"]),
                int(row["asset"]["id"]),
                str(row["asset"]["path"]),
            )
            for row in unique_core
        ]
        anim_rows = [
            (
                str(row["symbol"]),
                int(row["asset"]["id"]),
                str(row["asset"]["path"]),
            )
            for row in local_aliases
        ]
        append_rows(f"{prefix}_CORE_ASSET_ROWS", core_rows)
        append_rows(f"{prefix}_ANIM_ASSET_ROWS", anim_rows)

    lines.extend([
        "#endif",
        "",
    ])
    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--repo-root", type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument(
        "--output",
        type=Path,
        default=Path("scripts/fighters/fighter_production_manifest.json"),
    )
    parser.add_argument(
        "--make-output",
        type=Path,
        default=Path("scripts/fighters/fighter_production_files.mk"),
    )
    parser.add_argument(
        "--runtime-header-output",
        type=Path,
        default=Path("include/nds/generated/nds_fighter_production.generated.h"),
    )
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()

    repo_root = args.repo_root.resolve()
    output = args.output
    if not output.is_absolute():
        output = repo_root / output
    make_output = args.make_output
    if not make_output.is_absolute():
        make_output = repo_root / make_output
    runtime_header_output = args.runtime_header_output
    if not runtime_header_output.is_absolute():
        runtime_header_output = repo_root / runtime_header_output

    manifest = build_manifest(repo_root)
    rendered = render_json(manifest)
    rendered_make = render_make_fragment(manifest)
    rendered_runtime_header = render_runtime_header(manifest)
    if args.check:
        if not output.exists():
            raise SystemExit(f"fighter production manifest missing: {output}")
        current = output.read_text(encoding="utf-8")
        if current != rendered:
            raise SystemExit(
                "fighter production manifest is stale; rerun "
                "scripts/fighters/generate_fighter_production_manifest.py"
            )
        if not make_output.exists():
            raise SystemExit(f"fighter production make fragment missing: {make_output}")
        current_make = make_output.read_text(encoding="utf-8")
        if current_make != rendered_make:
            raise SystemExit(
                "fighter production make fragment is stale; rerun "
                "scripts/fighters/generate_fighter_production_manifest.py"
            )
        if not runtime_header_output.exists():
            raise SystemExit(
                f"fighter production runtime header missing: {runtime_header_output}"
            )
        current_runtime_header = runtime_header_output.read_text(encoding="utf-8")
        if current_runtime_header != rendered_runtime_header:
            raise SystemExit(
                "fighter production runtime header is stale; rerun "
                "scripts/fighters/generate_fighter_production_manifest.py"
            )
        print(f"fighter production manifest check passed: {output}")
        print(f"fighter production make-fragment check passed: {make_output}")
        print(f"fighter production runtime-header check passed: {runtime_header_output}")
        return 0

    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(rendered, encoding="utf-8", newline="\n")
    make_output.parent.mkdir(parents=True, exist_ok=True)
    make_output.write_text(rendered_make, encoding="utf-8", newline="\n")
    runtime_header_output.parent.mkdir(parents=True, exist_ok=True)
    runtime_header_output.write_text(
        rendered_runtime_header, encoding="utf-8", newline="\n"
    )
    print(f"wrote fighter production manifest: {output}")
    print(f"wrote fighter production make fragment: {make_output}")
    print(f"wrote fighter production runtime header: {runtime_header_output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
