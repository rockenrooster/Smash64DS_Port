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

import generate_nds_native_owners as native_owner


# nds_reloc_assets.c owns these two always-compiled resolvers; a P2 fighter that
# shares their files needs no segment of his own.
MARIO_ANIM_FIRST = 0x1f3
MARIO_ANIM_LAST = 0x281
FOX_ANIM_FIRST = 0x282
FOX_ANIM_LAST = 0x31f

BOOTSTRAP_FIGHTERS = (
    "Mario", "Fox", "Luigi", "Donkey", "Captain", "Samus", "Link", "Pikachu",
    "Yoshi", "Ness", "Purin", "Kirby",
)
# P2-6 variant kinds are never selectable and own no FTData/motion rows here;
# they stage only their own reloc roots plus whatever those roots' O2R extern
# tables reach. GDonkey reuses the Donkey owner packet, so only his Main is
# his own (admit_fighter.py); MMario owns Main/MainMotion/Model. File IDs from
# the O2R headers themselves (also pinned in reloc_backend_ftdata_symbols.c:
# llGDonkeyMain 0xd7, llMMarioMain 0xce/MainMotion 0xcd/Model 0x12c).
VARIANT_RELOC_ROOTS = {
    "GDonkey": (0xD7,),
    "MMario": (0xCE, 0xCD, 0x12C),
}
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


def strip_c_comments(text: str) -> str:
    """Strip both C comment forms before initializer tokenization.

    The production manifest originally only needed line comments because
    FTData/FTMotionDesc use them.  WPAttributes carries block comments between
    fields, and leaving those attached to the next token makes a source-derived
    behavior contract needlessly depend on comment wording.
    """
    return re.sub(r"/\*.*?\*/", "", strip_line_comments(text), flags=re.DOTALL)


def select_region_us(text: str) -> str:
    """Resolve the simple US/JP branches present in fighter data initializers."""
    text = re.sub(
        r"#if\s+defined\s*\(REGION_US\)\s*\n(.*?)#else\s*\n(.*?)#endif",
        lambda match: match.group(1),
        text,
        flags=re.DOTALL,
    )
    text = re.sub(
        r"#if\s+defined\s*\(REGION_JP\)\s*\n(.*?)#else\s*\n(.*?)#endif",
        lambda match: match.group(2),
        text,
        flags=re.DOTALL,
    )
    return text


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


def parse_enum_names(text: str, enum_name: str) -> list[str]:
    match = re.search(
        rf"typedef\s+enum\s+{re.escape(enum_name)}\s*\{{(.*?)\}}\s*"
        rf"{re.escape(enum_name)}\s*;",
        text,
        re.DOTALL,
    )
    if match is None:
        raise ValueError(f"enum not found: {enum_name}")
    names: list[str] = []
    for token in split_top_level_csv(strip_c_comments(match.group(1))):
        name = token.split("=", 1)[0].strip()
        if not re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", name):
            raise ValueError(f"unexpected {enum_name} enumerator: {token}")
        names.append(name)
    return names


def build_special_status_contract(repo_root: Path, fighter: str) -> dict[str, object]:
    """Read a fighter's source special-status table into an audit contract.

    P2-3 variants often share state-machine callbacks but not motion/event data.
    Recording the actual status descriptors makes that distinction explicit and
    keeps runtime proofs anchored to BattleShip instead of to hand-maintained
    assumptions about which fighter should have a bespoke implementation.
    """
    stem = fighter.lower()
    char_root = repo_root / f"decomp/BattleShip-main/decomp/src/ft/ftchar/ft{stem}"
    status_path = char_root / f"ft{stem}status.h"
    header_path = char_root / f"ft{stem}.h"
    status_text = status_path.read_text(encoding="utf-8")
    header_text = header_path.read_text(encoding="utf-8")
    declaration = f"FTStatusDesc dFT{fighter}SpecialStatusDescs[/* */]"
    block = find_initializer(status_text, declaration)
    entries = split_top_level_csv(strip_c_comments(block))
    names = parse_enum_names(header_text, f"ft{fighter}Status")

    # Fox has two enum-only sentinels outside the concrete descriptor table.
    # The first descriptor is always nFTCommonStatusSpecialStart; preserve the
    # source table length rather than inventing rows for enum values with no
    # FTStatusDesc storage.
    if len(names) < len(entries):
        raise ValueError(
            f"{fighter}: status enum shorter than descriptor table: "
            f"{len(names)} < {len(entries)}"
        )
    names = names[:len(entries)]

    rows: list[dict[str, object]] = []
    for name, entry in zip(names, entries):
        value = entry.strip()
        if not (value.startswith("{") and value.endswith("}")):
            raise ValueError(f"{fighter}: malformed status descriptor for {name}")
        fields = split_top_level_csv(value[1:-1])
        if len(fields) != 11:
            raise ValueError(
                f"{fighter}: {name} FTStatusDesc field count changed: "
                f"{len(fields)} != 11"
            )
        rows.append({
            "status": name,
            "motion": fields[0].strip(),
            "motion_attack_id": fields[1].strip(),
            "kinetics": fields[4].strip(),
            "is_projectile": fields[5].strip(),
            "status_attack_id": fields[6].strip(),
            "callbacks": {
                "update": fields[7].strip(),
                "interrupt": fields[8].strip(),
                "physics": fields[9].strip(),
                "map": fields[10].strip(),
            },
        })
    return {
        "source": status_path.relative_to(repo_root).as_posix(),
        "source_sha256": sha256(status_path),
        "descriptor_count": len(rows),
        "descriptors": rows,
    }


def parse_float_literal(value: str) -> float:
    token = value.strip()
    if token.endswith(("F", "f")):
        token = token[:-1]
    return float(token)


def parse_degree_expression(value: str) -> float:
    match = re.fullmatch(r"F_CLC_DTOR32\(([-+0-9.eEfF]+)\)", value.strip())
    if match is None:
        raise ValueError(f"expected F_CLC_DTOR32 degree expression: {value}")
    return parse_float_literal(match.group(1))


def build_luigi_fireball_contract(repo_root: Path) -> dict[str, object]:
    """Derive Luigi's shared-fireball variant directly from US source data."""
    weapon_path = repo_root / "decomp/BattleShip-main/decomp/src/wp/wpmario/wpmariofireball.c"
    attr_path = repo_root / "decomp/BattleShip-main/decomp/src/relocData/222_LuigiSpecial1.c"

    weapon_text = select_region_us(weapon_path.read_text(encoding="utf-8"))
    weapon_block = find_initializer(
        weapon_text,
        "wpMarioFireballAttributes dWPMarioFireballWeaponAttributes[/* */]",
    )
    variants = split_top_level_csv(strip_c_comments(weapon_block))
    if len(variants) != 2:
        raise ValueError(f"shared fireball variant count changed: {len(variants)} != 2")
    luigi_entry = variants[1].strip()
    if not (luigi_entry.startswith("{") and luigi_entry.endswith("}")):
        raise ValueError("Luigi fireball variant initializer is malformed")
    fields = split_top_level_csv(luigi_entry[1:-1])
    if len(fields) != 12:
        raise ValueError(f"Luigi fireball variant field count changed: {len(fields)} != 12")

    attr_text = select_region_us(attr_path.read_text(encoding="utf-8"))
    attr_block = find_initializer(
        attr_text, "WPAttributes dLuigiSpecial1_Fireball_WeaponAttributes"
    )
    attr_fields = split_top_level_csv(strip_c_comments(attr_block))
    if len(attr_fields) != 29:
        raise ValueError(
            f"Luigi fireball WPAttributes field count changed: {len(attr_fields)} != 29"
        )

    return {
        "source": weapon_path.relative_to(repo_root).as_posix(),
        "source_sha256": sha256(weapon_path),
        "variant_index": 1,
        "lifetime": int(fields[0], 0),
        "terminal_velocity": parse_float_literal(fields[1]),
        "map_collision_damage": parse_float_literal(fields[2]),
        "gravity": parse_float_literal(fields[3]),
        "collision_rebound": parse_float_literal(fields[4]),
        "rotate_degrees_per_tick": parse_degree_expression(fields[5]),
        "ground_launch_degrees": parse_degree_expression(fields[6]),
        "air_launch_degrees": parse_degree_expression(fields[7]),
        "base_velocity": parse_float_literal(fields[8]),
        "fighter_special_file": fields[9].strip(),
        "weapon_attributes_symbol": fields[10].strip(),
        "animation_start_frame": parse_float_literal(fields[11]),
        "map_callback": "wpMarioFireballProcMap",
        "map_rebound_shared_with_mario": True,
        "hitbox_source": attr_path.relative_to(repo_root).as_posix(),
        "hitbox_source_sha256": sha256(attr_path),
        "hitbox": {
            "map_top": int(attr_fields[5], 0),
            "map_center": int(attr_fields[6], 0),
            "map_bottom": int(attr_fields[7], 0),
            "map_width": int(attr_fields[8], 0),
            "size": int(attr_fields[9], 0),
            "angle": int(attr_fields[10], 0),
            "knockback_scale": int(attr_fields[11], 0),
            "damage": int(attr_fields[12], 0),
            "element": int(attr_fields[13], 0),
            "shield_damage": int(attr_fields[15], 0),
            "sfx": int(attr_fields[18], 0),
            "priority": int(attr_fields[19], 0),
            "knockback_base": int(attr_fields[28], 0),
        },
    }


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
    if extern_end + 4 > len(raw):
        raise ValueError(f"O2R extern table escapes file: {path}")
    extern_ids = list(struct.unpack_from(f"<{extern_count}H", raw, 0x4C)) \
        if extern_count else []
    data_size = struct.unpack_from("<I", raw, extern_end)[0]
    data_start = extern_end + 4
    if data_start + data_size != len(raw):
        raise ValueError(
            f"O2R payload size disagrees with file length: {path}: "
            f"0x{data_start:x}+0x{data_size:x} != 0x{len(raw):x}"
        )
    return {
        "file_id": file_id,
        "path": path.relative_to(root).as_posix(),
        "bytes": len(raw),
        "data_bytes": data_size,
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


def motion_desc_rows(text: str, declaration: str) -> list[tuple[str | None, bool]]:
    """Parse one source FTMotionDesc array in exact initializer order.

    BattleShip mixes ordinary braced triples with a few unbraced scalar
    placeholder triples.  `ftManagerSetupFileSize` still counts every one of
    those entries, so a source-equivalent AOT census must preserve them rather
    than counting only `&ll...FileID` references.

    Returns `(file_id_symbol, is_shieldpose)` for each descriptor.  A placeholder
    has `file_id_symbol=None`; shield-pose descriptors are present in the table
    but source deliberately excludes them from the figatree heap-size census.
    """
    clean = strip_c_comments(find_initializer(select_region_us(text), declaration))
    items = split_top_level_csv(clean)
    rows: list[tuple[str | None, bool]] = []
    index = 0

    while index < len(items):
        item = items[index].strip()
        if item.startswith("{") and item.endswith("}"):
            fields = split_top_level_csv(item[1:-1])
            index += 1
        else:
            fields = items[index:index + 3]
            index += 3
        if len(fields) != 3:
            raise ValueError(
                f"{declaration}: malformed FTMotionDesc initializer: {fields}"
            )

        symbol_match = re.fullmatch(
            r"&([A-Za-z0-9_]+FileID)", fields[0].strip()
        )
        symbol = symbol_match.group(1) if symbol_match is not None else None
        flags = fields[2].strip()
        is_shieldpose = "FTANIM_FLAG_SHIELDPOSE" in flags
        if not is_shieldpose:
            numeric = re.fullmatch(r"(0x[0-9A-Fa-f]+|[0-9]+)[uUlL]*", flags)
            if numeric is not None:
                is_shieldpose = (int(numeric.group(1), 0) & 0x2) != 0
        rows.append((symbol, is_shieldpose))

    return rows


def build_ftmanager_file_size_census(
    repo_root: Path,
    ftdata_text: str,
    named_symbols: dict[str, int],
    port_symbols: dict[str, int],
    semantic_ids: dict[str, int],
    by_id: dict[int, dict[str, object]],
) -> list[dict[str, object]]:
    """Reproduce BattleShip ftManagerSetupFileSize entirely at build time.

    The N64 source asks its in-ROM reloc table for immutable transitive file
    sizes.  Doing the same census through DS pointer-token classification and
    NitroFS metadata is a port artifact, and becomes especially expensive once
    the complete source roster is present.  Derive the same three FTFileSize
    fields from the pinned US O2Rs and the exact source motion/submotion tables.
    """
    ftdata_us = select_region_us(ftdata_text)
    roster_block = find_initializer(
        ftdata_us, "FTData *dFTManagerDataFiles[nFTKindEnumCount + 1]"
    )
    roster = re.findall(r"&dFT([A-Za-z0-9]+)Data", roster_block)
    if len(roster) != 27:
        raise ValueError(f"ftManager roster census changed: {len(roster)} != 27")

    def largest_alloc(rows: list[tuple[str | None, bool]]) -> int:
        largest = 0
        for symbol, is_shieldpose in rows:
            if symbol is None or is_shieldpose:
                continue
            file_id = resolve_symbol(symbol, named_symbols, port_symbols, semantic_ids)
            largest = max(largest, extern_alloc_size(file_id, by_id))
        return largest

    result: list[dict[str, object]] = []
    for kind, fighter in enumerate(roster):
        data_values = split_top_level_csv(
            strip_c_comments(find_initializer(ftdata_us, f"FTData dFT{fighter}Data"))
        )
        if len(data_values) != 30:
            raise ValueError(
                f"{fighter}: FTData field count changed: {len(data_values)} != 30"
            )
        main_symbol = symbol_from_value(data_values[0])
        if main_symbol is None:
            raise ValueError(f"{fighter}: source FTData main file is absent")
        main_id = resolve_symbol(
            main_symbol, named_symbols, port_symbols, semantic_ids
        )
        mainmotion_count = int(data_values[27], 0)
        main_rows = motion_desc_rows(
            ftdata_us, f"FTMotionDesc dFT{fighter}MotionDescs[]"
        )
        if len(main_rows) != mainmotion_count:
            raise ValueError(
                f"{fighter}: main motion census {len(main_rows)} != "
                f"FTData count {mainmotion_count}"
            )

        submotion_path = (
            repo_root
            / "decomp/BattleShip-main/decomp/src/sc/scsubsys"
            / f"scsubsysdata{fighter.lower()}.c"
        )
        if not submotion_path.is_file():
            raise ValueError(f"{fighter}: missing submotion source {submotion_path}")
        submotion_text = submotion_path.read_text(encoding="utf-8")
        sub_rows = motion_desc_rows(
            submotion_text, f"FTMotionDesc dFT{fighter}SubMotionDescs[]"
        )

        result.append(
            {
                "kind": kind,
                "fighter": fighter,
                "main": extern_alloc_size(main_id, by_id),
                "mainmotion_largest_anim": largest_alloc(main_rows),
                "submotion_largest_anim": largest_alloc(sub_rows),
                "mainmotion_count": mainmotion_count,
                "submotion_count": len(sub_rows),
                "submotion_source": str(submotion_path.relative_to(repo_root)).replace(
                    "\\", "/"
                ),
            }
        )
    return result


def motion_animjoint_symbols(ftdata_text: str, fighter: str) -> list[str]:
    """Return animation files whose source motion descriptor is AObjEvent32.

    BattleShip does not encode the animation parser kind in the reloc file
    header.  `ftMainSetStatus` selects it from `FTANIM_FLAG_ANIMJOINT`: flagged
    motions go through `lbCommonAddFighterPartsFigatree`/`gcParseDObjAnimJoint`
    (AObjEvent32), while ordinary fighter motions go through
    `ftAnimParseDObjFigatree` (AObjEvent16).  That distinction therefore has to
    follow the *source FTMotionDesc table*, not an ID range guessed in the DS
    loader.

    Keep the result ordered by first source use, matching ``motion_symbols``.
    A file is Event32 if any source descriptor that references it carries the
    flag; sharing one file between flagged descriptors is common for appear
    pairs on variants.
    """
    block = find_initializer(ftdata_text, f"FTMotionDesc dFT{fighter}MotionDescs[]")
    result: list[str] = []

    # Concrete descriptors are brace-enclosed triples.  The table also has a
    # few scalar zero/0x80000000 placeholder triples; those intentionally have
    # no file-id symbol and cannot contribute an Event32 asset.
    for match in re.finditer(r"\{([^{}]*)\}", strip_c_comments(block), re.DOTALL):
        fields = split_top_level_csv(match.group(1))
        if len(fields) != 3:
            raise ValueError(
                f"{fighter}: malformed FTMotionDesc initializer: {match.group(0)}"
            )
        symbol_match = re.fullmatch(r"&([A-Za-z0-9_]+FileID)", fields[0].strip())
        if symbol_match is None:
            continue
        if re.search(r"\bFTANIM_FLAG_ANIMJOINT\b", fields[2]) is None:
            continue
        symbol = symbol_match.group(1)
        if symbol not in result:
            result.append(symbol)
    return result


# The checked O2R corpus names each fighter's motion-table animations
# `FT<Name>Anim###` in one contiguous id run -- except Jigglypuff, whose 67
# battle animations (source ids 1445..1511, relocData `*_FTPurinAnim*.c`) the
# extraction split into `FTKirbyCopyAnim000..058` (1445..1503) and
# `FTPurinAnim000..007` (1504..1511). The corpus is read-only reference, so the
# manifest carries the stems the corpus actually uses and the runtime catalog
# emits a second path segment when a fighter needs one.
O2R_ANIM_STEMS: dict[str, tuple[str, ...]] = {
    "Purin": ("FTKirbyCopyAnim", "FTPurinAnim"),
}


def o2r_anim_stems(fighter: str) -> tuple[str, ...]:
    return O2R_ANIM_STEMS.get(fighter, (f"FT{fighter}Anim",))


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
        if any(str(record["path"]).startswith(f"reloc_animations/{stem}")
               for stem in o2r_anim_stems(fighter))
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


def extern_alloc_size(
    root: int, by_id: dict[int, dict[str, object]]
) -> int:
    """Mirror BattleShip/lbreloc's transitive allocation size offline.

    The N64 manager asks this question while sizing every fighter animation.
    On DS the answer is immutable ROM metadata, so publishing it AOT avoids a
    NitroFS directory/header walk for every motion while preserving the exact
    source allocation contract (16-byte aligned payloads, dependencies counted
    once per root).
    """
    seen: set[int] = set()

    def visit(file_id: int) -> int:
        if file_id in seen:
            return 0
        seen.add(file_id)
        record = by_id.get(file_id)
        if record is None:
            raise ValueError(f"O2R dependency file id 0x{file_id:x} is missing")
        total = (int(record["data_bytes"]) + 0xF) & ~0xF
        for dep in record["extern_ids"]:
            total = (total + 0xF) & ~0xF
            total += visit(int(dep))
        return total

    return visit(root)


def asset_summary(file_id: int, by_id: dict[int, dict[str, object]]) -> dict[str, object]:
    record = by_id[file_id]
    return {
        "id": file_id,
        "id_hex": f"0x{file_id:x}",
        "path": record["path"],
        "bytes": record["bytes"],
        "data_bytes": record["data_bytes"],
        "alloc_bytes": extern_alloc_size(file_id, by_id),
        "sha256": record["sha256"],
    }


def build_variant_closure(
    fighter: str,
    roots: tuple[int, ...],
    by_id: dict[int, dict[str, object]],
) -> dict[str, object]:
    """Stage one variant kind's reloc closure from O2R extern tables.

    Unlike build_fighter_manifest this reads no FTData: variants share their
    base kind's motion/status tables verbatim and are never selectable. The
    roots above are the only hand-pinned input; every further file comes from
    the O2R headers' own extern tables via extern_closure.
    """
    for file_id in roots:
        if file_id not in by_id:
            raise ValueError(f"{fighter}: variant root 0x{file_id:x} has no O2R")
    closure = extern_closure(roots, by_id)
    return {
        "fighter": fighter,
        "roots": [
            {"id": file_id, "id_hex": f"0x{file_id:x}",
             "path": str(by_id[file_id]["path"])}
            for file_id in roots
        ],
        "closure": [asset_summary(i, by_id) for i in closure],
        "nitrofs_files": [str(by_id[i]["path"]) for i in closure],
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
    event32_refs = motion_animjoint_symbols(ftdata_text, fighter)
    if not set(event32_refs).issubset(refs):
        raise ValueError(f"{fighter}: Event32 motion census escaped the motion table")
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

    event32_motion_files: list[dict[str, object]] = []
    for symbol in event32_refs:
        file_id = resolve_symbol(symbol, named_symbols, port_symbols, semantic_ids)
        event32_motion_files.append({
            "symbol": symbol,
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
        # FTData field 24 is the fighter's FTAttributes offset inside its Main
        # file.  It is the only source for the port's
        # NDS_RELOC_SYMBOL_<F>_MAIN_ATTRIBUTES, and the input to the mixed-u16
        # lane normalizer guarded below.
        "attributes_offset": int(data_values[24], 0),
        "core": core,
        "core_extern_closure": [asset_summary(i, by_id) for i in core_closure],
        "local_animation_aliases": local_aliases,
        "motion_files": motion_files,
        "event32_motion_file_count": len(event32_motion_files),
        "event32_motion_files": event32_motion_files,
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


def verify_attributes_normalizer_coverage(
    repo_root: Path, fighters: list[dict[str, object]]
) -> None:
    """Every landed fighter needs an FTAttributes mixed-u16 normalizer arm.

    The O2R payload is big-endian, and the loader's blanket u32 byte swap
    reverses the two u16 lanes inside the six FTAttributes words that hold
    dead/deadup/damage/smash/item-throw/heavy-get FGM ids.  A fighter with no
    arm in `ndsRelocNormalizeFighterAttributesFile` keeps the reversed lanes:
    his damage voice is his star-KO voice, one of his three smash-voice slots
    is FGM id 0, and his heavy-get id is 0 too.

    Luigi shipped for weeks without one, found only as a side effect of
    Captain Falcon's landing (board row P2-3f8).  Nothing checked, so nothing
    said.  This is the check.
    """
    backend = (repo_root / "src/port/reloc_backend_assets.c").read_text(
        encoding="utf-8"
    )
    missing: list[str] = []
    for row in fighters:
        name = str(row["fighter"])
        upper = name.upper()
        offset = int(row["attributes_offset"])
        define = f"#define NDS_RELOC_SYMBOL_{upper}_MAIN_ATTRIBUTES 0x{offset:x}u"
        if define not in backend:
            missing.append(
                f"{name}: reloc_backend_assets.c is missing `{define}` "
                f"(ftdata.c FTData field 24 = 0x{offset:x})"
            )
        if f"NDS_RELOC_ASSET_{upper}_MAIN" not in backend:
            missing.append(
                f"{name}: reloc_backend_assets.c names no "
                f"NDS_RELOC_ASSET_{upper}_MAIN, so "
                "ndsRelocNormalizeFighterAttributesFile cannot reach him"
            )
    if missing:
        raise ValueError(
            "FTAttributes normalizer coverage gap:\n  " + "\n  ".join(missing)
        )


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
    ftmanager_file_size_census = build_ftmanager_file_size_census(
        repo_root, ftdata_text, named_symbols, port_symbols, semantic_ids, by_id
    )

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
    donkey = fighters[3]
    if len(donkey["local_animation_aliases"]) != 153:
        raise ValueError("Donkey local animation census changed from the source 153")
    if not all(row["was_stubbed_in_port"] for row in donkey["local_animation_aliases"]):
        raise ValueError("Donkey bootstrap expected all 153 local aliases to be unresolved")

    verify_attributes_normalizer_coverage(repo_root, fighters)

    # Native model admission is sourced from the same O2R display-list decoder
    # that produced Mario/Fox's shipping AOT owner.  Keep this attached to the
    # fighter manifest so the next runtime slice consumes an already-reviewed
    # high/low topology instead of rediscovering or hand-copying it in C.
    for fighter in fighters:
        owner_name = str(fighter["fighter"]).lower()
        if owner_name in native_owner.P2_OWNER_MODEL_CENSUS:
            fighter["native_model"] = native_owner.build_p2_owner_model_inventory(
                repo_root, owner_name
            )
        fighter["special_status_contract"] = build_special_status_contract(
            repo_root, str(fighter["fighter"])
        )

    # Luigi is the first P2-3 variant fighter.  BattleShip deliberately points
    # his N/Hi/Lw status descriptors at Mario's state-machine callbacks; the
    # authored differences live in Luigi motion/event data and the fireball's
    # index-1 weapon data.  Derive that relationship instead of forking three
    # DS implementations merely because the character name changed.
    mario_statuses = fighters[0]["special_status_contract"]["descriptors"]
    luigi_statuses = luigi["special_status_contract"]["descriptors"]
    mario_specials = mario_statuses[-6:]
    luigi_specials = luigi_statuses[-6:]
    shared_callbacks = all(
        mario_row["callbacks"] == luigi_row["callbacks"]
        for mario_row, luigi_row in zip(mario_specials, luigi_specials)
    )
    if not shared_callbacks:
        raise ValueError(
            "Luigi N/Hi/Lw callbacks no longer match Mario; P2-3 variant "
            "runtime assumptions must be re-reviewed against BattleShip"
        )
    luigi["variant_contract"] = {
        "shares_mario_special_callbacks": True,
        "shared_status_pairs": [
            {
                "luigi": luigi_row["status"],
                "mario": mario_row["status"],
                "callbacks": luigi_row["callbacks"],
            }
            for mario_row, luigi_row in zip(mario_specials, luigi_specials)
        ],
        "fireball": build_luigi_fireball_contract(repo_root),
    }

    # Variant kinds stage their own reloc roots only; the twelve selectable
    # rows above are untouched by this block.
    variant_closures = {
        name: build_variant_closure(name, roots, by_id)
        for name, roots in VARIANT_RELOC_ROOTS.items()
    }
    gdonkey_ids = {int(a["id"]) for a in variant_closures["GDonkey"]["closure"]}
    if 0xD7 not in gdonkey_ids:
        raise ValueError("GDonkey closure lost its own GDonkeyMain 0xd7")
    mmario_ids = {int(a["id"]) for a in variant_closures["MMario"]["closure"]}
    if not {0xCE, 0xCD, 0x12C} <= mmario_ids:
        raise ValueError("MMario closure lost its own Main/MainMotion/Model")

    return {
        "schema": "smash64ds.p2-fighter-production-manifest.v2",
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
            "donkey_aliases_recovered": len(donkey["local_animation_aliases"]),
        },
        "mariofox_shipping_file_set": verify_mariofox_makefile(
            repo_root, fighters, by_id
        ),
        "ftmanager_file_size_census": ftmanager_file_size_census,
        "fighters": fighters,
        "variant_closures": variant_closures,
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
    # Variant kinds aggregate only under their own NDS_P2_<VARIANT> flag
    # (admit_fighter.py); reused base files already ride the base list, so
    # these are incremental over the Mario/Fox baseline like every other row.
    for name in VARIANT_RELOC_ROOTS:
        row = manifest["variant_closures"][name]
        added = [
            path for path in row["nitrofs_files"]
            if path not in baseline
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

    file_size_census = list(manifest["ftmanager_file_size_census"])
    if len(file_size_census) != 27:
        raise ValueError(
            f"ftManager runtime file-size census changed: {len(file_size_census)} != 27"
        )
    lines.append(
        f"#define NDS_FTMANAGER_FILE_SIZE_CENSUS_COUNT {len(file_size_census)}u"
    )
    lines.append("#define NDS_FTMANAGER_FILE_SIZE_CENSUS_ROWS(X) \\")
    for index, row in enumerate(file_size_census):
        suffix = " \\" if index + 1 < len(file_size_census) else ""
        lines.append(
            "    X({kind}u, {main}u, {mainmotion}u, {submotion}u)"
            " /* {fighter} */{suffix}".format(
                kind=int(row["kind"]),
                main=int(row["main"]),
                mainmotion=int(row["mainmotion_largest_anim"]),
                submotion=int(row["submotion_largest_anim"]),
                suffix=suffix,
                fighter=row["fighter"],
            )
        )
    lines.append("")

    # BattleShip's global fighter-size census visits Mario/Fox too, not only the
    # incremental P2-3 rows emitted below.  Publish AOT size metadata by
    # admission group, though: a canonical Mario/Fox build must not pay linked
    # bytes for DK merely because the generator knows DK exists.  The runtime
    # selects the incremental macros under the same NDS_P2_* flags as the asset
    # and native-owner rows.
    published_alloc_ids: set[int] = set()
    published_payload_ids: set[int] = set()

    def append_alloc_rows(macro_name: str, names: tuple[str, ...]) -> None:
        # ftManagerSetupFileSize asks only for a fighter's main file and the
        # animation file referenced by each motion descriptor.  Models,
        # shield/special files and main-file dependency closure are loaded later
        # but are not part of this census; publishing them here only grew the DS
        # lookup with answers the source never asks at this seam.
        alloc_by_id: dict[int, int] = {}
        for fighter_name in names:
            fighter = fighters[fighter_name]
            main = next(row for row in fighter["core"] if row["slot"] == "main")
            assets = [main["asset"]] + [row["asset"] for row in fighter["motion_files"]]
            for asset in assets:
                if asset is None:
                    raise ValueError(f"{fighter_name}: fighter-size source asset is absent")
                file_id = int(asset["id"])
                if file_id in published_alloc_ids:
                    continue
                alloc_bytes = int(asset["alloc_bytes"])
                if (alloc_bytes & 0xF) != 0 or (alloc_bytes >> 4) > 0xFFFF:
                    raise ValueError(
                        f"asset 0x{file_id:x} allocation size cannot use u16/16 AOT form: "
                        f"{alloc_bytes}"
                    )
                previous = alloc_by_id.get(file_id)
                if previous is not None and previous != alloc_bytes:
                    raise ValueError(
                        f"asset 0x{file_id:x} has conflicting allocation sizes: "
                        f"{previous} / {alloc_bytes}"
                    )
                alloc_by_id[file_id] = alloc_bytes
        rows = sorted(alloc_by_id.items())
        published_alloc_ids.update(alloc_by_id)
        if not rows:
            lines.append(f"#define {macro_name}(X)")
            lines.append("")
            return
        lines.append(f"#define {macro_name}(X) \\")
        for index, (file_id, alloc_bytes) in enumerate(rows):
            suffix = " \\" if index + 1 < len(rows) else ""
            lines.append(f"    X(0x{file_id:x}u, {alloc_bytes}u){suffix}")
        lines.append("")

    append_alloc_rows("NDS_P2_BASE_FIGHTER_ALLOC_SIZE_ROWS", ("Mario", "Fox"))
    for name in BOOTSTRAP_FIGHTERS[2:]:
        append_alloc_rows(f"NDS_P2_{name.upper()}_ALLOC_SIZE_ROWS", (name,))

    def append_payload_rows(macro_name: str, names: tuple[str, ...]) -> None:
        """Publish exact per-file payload sizes for fighter-main extern trees.

        BattleShip's ROM reloc table supplies this metadata without file I/O.
        On DS the imported manager otherwise has to open each O2R just to learn
        how far to advance the caller-provided extern-tree heap before opening
        the same path again for the payload.  The manifest has already parsed
        and hash-pinned those headers, so emit the same immutable answer AOT.

        Keep this narrower than ``nitrofs_files``: animation motions have their
        own one-open acquisition path.  This table exists for the core extern
        closure loaded by ``ftManagerSetupFilesMainKind`` / ``SetupFilesKind``.
        """
        payload_by_id: dict[int, int] = {}
        for fighter_name in names:
            fighter = fighters[fighter_name]
            for asset in fighter["core_extern_closure"]:
                file_id = int(asset["id"])
                if file_id in published_payload_ids:
                    continue
                data_bytes = int(asset["data_bytes"])
                if (data_bytes & 0xF) != 0 or (data_bytes >> 4) > 0xFFFF:
                    raise ValueError(
                        f"asset 0x{file_id:x} payload size cannot use u16/16 AOT form: "
                        f"{data_bytes}"
                    )
                previous = payload_by_id.get(file_id)
                if previous is not None and previous != data_bytes:
                    raise ValueError(
                        f"asset 0x{file_id:x} has conflicting payload sizes: "
                        f"{previous} / {data_bytes}"
                    )
                payload_by_id[file_id] = data_bytes
        rows = sorted(payload_by_id.items())
        published_payload_ids.update(payload_by_id)
        if not rows:
            lines.append(f"#define {macro_name}(X)")
            lines.append("")
            return
        lines.append(f"#define {macro_name}(X) \\")
        for index, (file_id, data_bytes) in enumerate(rows):
            suffix = " \\" if index + 1 < len(rows) else ""
            lines.append(f"    X(0x{file_id:x}u, {data_bytes}u){suffix}")
        lines.append("")

    append_payload_rows(
        "NDS_P2_BASE_FIGHTER_PAYLOAD_SIZE_ROWS", ("Mario", "Fox")
    )
    for name in BOOTSTRAP_FIGHTERS[2:]:
        append_payload_rows(
            f"NDS_P2_{name.upper()}_PAYLOAD_SIZE_ROWS", (name,)
        )

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

        # Token rows follow the MOTION TABLE, not the local-alias census.
        # Every animation file the motion table references needs a
        # symbol-address -> numeric-id row so the runtime token table can
        # answer its &ll...FileID token even when the file's owner is not
        # in the build (Purin borrows 77 Kirby files, Kirby borrows 2 Purin
        # files). Local-only rows left those borrowed tokens unresolvable
        # and the force loader silently replayed the previous motion.
        # Mario/Fox ids are excluded: their resolvers are always compiled.
        anim_rows: list[tuple[str, int, str]] = []
        for row in sorted(fighter["motion_files"],
                          key=lambda r: int(r["asset"]["id"])):
            file_id = int(row["asset"]["id"])
            if (MARIO_ANIM_FIRST <= file_id <= MARIO_ANIM_LAST) or \
                    (FOX_ANIM_FIRST <= file_id <= FOX_ANIM_LAST):
                continue
            anim_rows.append(
                (
                    str(row["symbol"]),
                    file_id,
                    str(row["asset"]["path"]),
                )
            )

        first_id = min(int(row["asset"]["id"]) for row in local_aliases)
        last_id = max(int(row["asset"]["id"]) for row in local_aliases)
        # The runtime builds `nitro:/.../<stem><id - stem_zero_id>`. A fighter's
        # MOTION TABLE -- not just his own local aliases -- decides which files
        # he must be able to open, and the corpus freely puts those under
        # another fighter's stem: Luigi takes 131 from FTMarioAnim, Purin 77
        # from FTKirbyAnim, Kirby 2 from FTKirbyCopyAnim (measured 2026-09-02).
        # Emitting only his own stem left Purin unable to resolve 77 animations
        # unless NDS_P2_KIRBY happened to be in the build too, and a Purin-only
        # ROM never presented a battle frame. So every segment he needs is
        # emitted here. Mario's and Fox's ranges are excluded because their
        # resolvers are always compiled and carry the three odd unnumbered
        # names (FTMarioAnimWait, ...DownBounceD, ...DownStandD) besides.
        known_stems = {
            m.group("stem")
            for other in manifest["fighters"]
            for asset_row in other["motion_files"]
            for m in [re.match(r"reloc_animations/(?P<stem>.+?)\d{3}$",
                                str(asset_row["asset"]["path"]))]
            if m is not None
        }
        segments: list[list] = []
        for row in sorted(fighter["motion_files"],
                          key=lambda r: int(r["asset"]["id"])):
            file_id = int(row["asset"]["id"])
            if (MARIO_ANIM_FIRST <= file_id <= MARIO_ANIM_LAST) or \
                    (FOX_ANIM_FIRST <= file_id <= FOX_ANIM_LAST):
                continue
            path = str(row["asset"]["path"])
            # The stem is read off the path itself rather than from a
            # per-fighter allowlist: a fighter legitimately references files
            # under other fighters' stems, and the corpus name is the truth.
            match = re.match(r"reloc_animations/(?P<stem>.+?)(?P<index>\d{3})$",
                             path)
            if match is None:
                raise ValueError(
                    f"{name}: {path} is not a numbered AOT animation path")
            stem = match.group("stem")
            if stem not in known_stems:
                raise ValueError(f"{name}: {path} is under no known O2R stem")
            zero_id = file_id - int(match.group("index"))
            # Merge on (stem, zero_id), not on contiguity: a fighter's ids
            # are sparse within a stem, and widening a row over an id he
            # never asks for cannot change how a used id resolves.
            if segments and (segments[-1][2] == stem) and \
                    (segments[-1][3] == zero_id):
                segments[-1][1] = file_id
            else:
                segments.append([file_id, file_id, stem, zero_id])
        if not segments:
            raise ValueError(f"{name}: no resolvable animation segments")
        lines.extend([
            f"#define {prefix}_ANIM_FIRST 0x{first_id:x}u",
            f"#define {prefix}_ANIM_LAST 0x{last_id:x}u",
            f"#define {prefix}_ANIM_COUNT {len(anim_rows)}u",
            f"#define {prefix}_ANIM_SEGMENT_COUNT {len(segments)}u",
            f"#define {prefix}_ANIM_SEGMENTS(X) \\",
        ])
        for k, (seg_first, seg_last, stem, zero_id) in enumerate(segments):
            cont = " \\" if k + 1 < len(segments) else ""
            lines.append(
                f"    X(0x{seg_first:x}u, 0x{seg_last:x}u, "
                f"\"{stem}\", 0x{zero_id:x}u){cont}")
        lines.append("")

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

        def append_dependency_rows(
            macro_name: str, rows: list[tuple[int, str]]
        ) -> None:
            """Emit numeric-only O2R dependencies.

            Core/animation rows have an addressable ``ll...FileID`` symbol because
            BattleShip passes those symbols to lbReloc directly.  Some O2R-only
            dependencies do not: DK's DkIcon, for example, is a numeric reloc
            macro in BattleShip and is reached only through DonkeyMain's external
            relocation table.  It still needs a NitroFS/runtime catalog row, but
            manufacturing an addressable symbol for it would change the source
            ABI rather than adapting it.
            """
            if not rows:
                lines.append(f"#define {macro_name}(X)")
                lines.append("")
                return
            lines.append(f"#define {macro_name}(X) \\")
            for index, (file_id, path) in enumerate(rows):
                suffix = " \\" if index + 1 < len(rows) else ""
                nitro_path = f"nitro:/reloc/{path}"
                lines.append(
                    f"    X(0x{file_id:x}u, \"{nitro_path}\"){suffix}"
                )
            lines.append("")

        def append_symbol_id_rows(
            macro_name: str, rows: list[tuple[str, int]]
        ) -> None:
            """Emit a source-symbol/id classifier table without a runtime path.

            Parser kind is a property of the source FTMotionDesc flag, not of
            NitroFS routing.  Keeping the semantic symbol beside the numeric
            O2R id makes the generated classifier auditable without carrying a
            path string into the C translation unit that consumes it.
            """
            if not rows:
                lines.append(f"#define {macro_name}(X)")
                lines.append("")
                return
            lines.append(f"#define {macro_name}(X) \\")
            for index, (symbol, file_id) in enumerate(rows):
                suffix = " \\" if index + 1 < len(rows) else ""
                lines.append(f"    X({symbol}, 0x{file_id:x}u){suffix}")
            lines.append("")

        core_rows = [
            (
                str(row["symbol"]),
                int(row["asset"]["id"]),
                str(row["asset"]["path"]),
            )
            for row in unique_core
        ]
        addressable_paths = {
            path for _, _, path in core_rows
        } | {
            path for _, _, path in anim_rows
        }
        dependency_rows = [
            (int(asset["id"]), str(asset["path"]))
            for asset in fighter["nitrofs_files"]
            if str(asset["path"]) in incremental_paths
            and str(asset["path"]) not in addressable_paths
        ]
        # Build-time coverage rule: every motion-table animation file
        # outside the always-compiled Mario/Fox banks must carry a token
        # row above. Runs on every generation and under --check.
        expected_tokens = {
            (str(row["symbol"]), int(row["asset"]["id"]))
            for row in fighter["motion_files"]
            if not (
                (MARIO_ANIM_FIRST <= int(row["asset"]["id"]) <= MARIO_ANIM_LAST)
                or (FOX_ANIM_FIRST <= int(row["asset"]["id"]) <= FOX_ANIM_LAST)
            )
        }
        actual_tokens = {(symbol, file_id) for symbol, file_id, _ in anim_rows}
        missing_tokens = sorted(expected_tokens - actual_tokens)
        if missing_tokens:
            raise ValueError(
                f"{name}: {len(missing_tokens)} motion animation files have "
                f"no token row (e.g. {missing_tokens[:3]})"
            )
        append_rows(f"{prefix}_CORE_ASSET_ROWS", core_rows)
        append_rows(f"{prefix}_ANIM_ASSET_ROWS", anim_rows)
        append_dependency_rows(f"{prefix}_DEPENDENCY_ASSET_ROWS", dependency_rows)
        event32_rows: list[tuple[str, int]] = []
        seen_event32_ids: set[int] = set()
        for event32 in fighter["event32_motion_files"]:
            file_id = int(event32["asset"]["id"])
            if file_id in seen_event32_ids:
                continue
            seen_event32_ids.add(file_id)
            event32_rows.append((str(event32["symbol"]), file_id))
        append_symbol_id_rows(f"{prefix}_AOBJ32_ASSET_ROWS", event32_rows)

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
