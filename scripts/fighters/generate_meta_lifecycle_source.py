#!/usr/bin/env python3
"""Produce bounded Meta Knight lifecycle seams from frozen source evidence.

Only the named kind-indexed consumers change. Source references stay read-only;
the wrappers include these copies exclusively in the admitted P4 configuration.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import re
import struct

from extra_resolved_actions import parse_symbols

ROOT = Path(__file__).resolve().parents[2]


class LifecycleError(ValueError):
    pass


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def replace_exact(text: str, old: str, new: str, count: int, label: str) -> str:
    if text.count(old) != count:
        raise LifecycleError(f"{label}: expected {count} source occurrences, found {text.count(old)}")
    return text.replace(old, new)


def transform(relative: str, text: str) -> str:
    text = text.replace("\r\n", "\n")
    if relative.endswith("ftkirbyspecialn.c"):
        return replace_exact(text, "copy[victim_fp->fkind].copy_id",
                             "ndsMetaKirbyVictimCopyID(victim_fp, copy)", 1, relative)
    if relative.endswith("ftcommoncapturekirby.c"):
        return replace_exact(text, "copy[fp->fkind].star_damage",
                             "ndsMetaKirbyVictimStarDamage(fp, copy)", 1, relative)
    if relative.endswith("ftpublic.c"):
        return replace_exact(text, "dFTCommonDataPublicFighterCallFGMs[ftGetStruct(fighter_gobj)->fkind]",
                             "ndsMetaPublicChantFGM(ftGetStruct(fighter_gobj))", 1, relative)
    if relative.endswith("efmanager.c"):
        text = replace_exact(text, "copy[fp->fkind].effect_scale",
                             "ndsMetaKirbyVictimStarScale(fp, copy)", 3, relative)
        text = replace_exact(text, "copy[ftGetStruct(fighter_gobj)->fkind].effect_scale",
                             "ndsMetaKirbyVictimStarScale(ftGetStruct(fighter_gobj), copy)", 1, relative)
        return replace_exact(text, "dFTCommonYoshiEggDamageCollDescs[fp->fkind].effect_size",
                             "dFTCommonYoshiEggDamageCollDescs[ndsP4GetThrownScriptColumn(fp->fkind)].effect_size",
                             1, relative + ": inherited egg effect size")
    if relative.endswith("ftcommoncapturecaptain.c"):
        return replace_exact(text, "offset_add[capture_fp->fkind]",
                             "offset_add[ndsP4GetThrownScriptColumn(capture_fp->fkind)]",
                             2, relative + ": inherited Falcon Dive victim offset")
    if relative.endswith("ftcommoncaptureyoshi.c"):
        return replace_exact(text, "dFTCommonYoshiEggDamageCollDescs[fp->fkind]",
                             "dFTCommonYoshiEggDamageCollDescs[ndsP4GetThrownScriptColumn(fp->fkind)]",
                             1, relative + ": inherited egg victim hurtbox")
    if relative.endswith("ftcommondownwaitbounce.c"):
        return replace_exact(text, "dFTCommonDataDownBounceSFX[fp->fkind]",
                             "ndsMetaDownBounceFGM(fp)", 1, relative + ": own resolved bounce cue")
    if relative.endswith("ftcommonthrow.c"):
        return replace_exact(text, "this_fp->attr->thrown_status[catch_fp->fkind]",
                             "this_fp->attr->thrown_status[ndsP4GetThrownScriptColumn(catch_fp->fkind)]",
                             2, relative + ": inherited ordinary throw victim row")
    if relative.endswith("mnvsresults.c"):
        replacements = (
            ("announce_names[mnVSResultsGetFighterKind(mnVSResultsGetWinPlayer())]",
             "ndsMetaVSResultAnnouncer(mnVSResultsGetFighterKind(mnVSResultsGetWinPlayer()), announce_names)", 1),
            ("dobjdescs[win_fkind]", "dobjdescs[ndsMetaVSResultSeriesKind(win_fkind)]", 1),
            ("mobjsubs[win_fkind]", "mobjsubs[ndsMetaVSResultSeriesKind(win_fkind)]", 1),
            ("matanim_joints[win_fkind]", "matanim_joints[ndsMetaVSResultSeriesKind(win_fkind)]", 1),
            ("dSCSubsysFighterScales[fkind]", "ndsMetaVSResultFighterScale(fkind)", 3),
            ("x_fkinds[winner]", "ndsMetaVSResultWinsX(winner, x_fkinds)", 2),
            ("names[fkind]", "ndsMetaVSResultName(fkind, names)", 1),
            ("pos_x[fkind]", "ndsMetaVSResultNameX(fkind, pos_x)", 1),
            ("scales[fkind]", "ndsMetaVSResultNameScale(fkind, scales)", 1))
        for old, new, count in replacements:
            text = replace_exact(text, old, new, count, relative + ": " + old)
        # Pair fields must be replaced before the complete record expression.
        for member, helper in (("ko_count", "ndsMetaVSRecordKO"),
                               ("player_count_tallies", "ndsMetaVSRecordPlayerTally"),
                               ("played_against", "ndsMetaVSRecordPlayedAgainst")):
            old = f"gSCManagerBackupData.vs_records[this_fkind].{member}[vs_fkind]"
            count = 3 if member == "ko_count" else 1
            text = replace_exact(text, old, f"(*{helper}(this_fkind, vs_fkind))", count, relative + ": " + member)
        text = replace_exact(text, "gSCManagerBackupData.vs_records[this_fkind]",
                             "(*ndsMetaVSRecord(this_fkind))", 14, relative + ": aggregate record")
        text = replace_exact(text, "void mnVSResultsPlayWinBGM(void)\n{",
                             "void mnVSResultsPlayWinBGM(void)\n{\n"
                             "\tif (mnVSResultsGetFighterKind(mnVSResultsGetWinPlayer()) == "
                             "(s32)NDS_P4_RUNTIME_METAKNIGHT)\n\t{\n"
                             "\t\tndsP4MetaKnightPlayVictoryBGM();\n\t\treturn;\n\t}\n", 1,
                             relative + ": own victory BGM")
        return text
    raise LifecycleError(f"unowned lifecycle source: {relative}")


def frozen_output(manifest: dict, role: str) -> tuple[Path, bytes]:
    target = manifest["output_roles"][role]
    rows = [row for row in manifest["outputs"] if row["path"] == target]
    if len(rows) != 1:
        raise LifecycleError(f"ambiguous frozen donor output {role}")
    path = Path(manifest["destination"]) / target
    data = path.read_bytes()
    if sha(data) != rows[0]["sha256"]:
        raise LifecycleError(f"frozen donor output changed: {path}")
    return path, data


def inherited_victim_metadata(rom: bytes, labels: dict[str, int], donor_id: int,
                              character_source: str) -> dict:
    """Qualify only the existing donor's explicit parent-owned lookup domains."""
    source = "\n".join(line.split("//", 1)[0] for line in character_source.splitlines())
    contracts = {
        "f_thrown_action": "add_to_id_table(f_thrown_action, id.{name}, id.{parent})",
        "b_thrown_action": "add_to_id_table(b_thrown_action, id.{name}, id.{parent})",
        "falcon_dive_id": "add_to_id_table(falcon_dive_id, id.{name}, id.{parent})",
        "yoshi_egg": "add_to_table(yoshi_egg, id.{name}, id.{parent}, 0x1C)",
        "down_bound_fgm": "add_to_table(down_bound_fgm, id.{name}, id.{parent}, 0x2)"}
    for name, contract in contracts.items():
        if source.count(contract) != 1:
            raise LifecycleError(f"{name}: source inheritance contract changed")

    def linked_row(name: str, index: int, width: int) -> tuple[int, bytes]:
        symbol = "Character." + name + ".table"
        if symbol not in labels:
            raise LifecycleError(f"missing linked inheritance table: {symbol}")
        address = labels[symbol] + index * width
        at = address - 0x80400000 + 0x02C00000
        if address < 0x80400000 or at < 0x02C00000 or at + width > len(rom):
            raise LifecycleError(f"{name}: unclassified/out-of-bounds linked row")
        return at, rom[at:at + width]

    lookup_kind = 10  # The exact JIGGLYPUFF parent ID declared by this pinned input.
    aliases = []
    for name in ("f_thrown_action", "b_thrown_action", "falcon_dive_id"):
        at, data = linked_row(name, donor_id, 4)
        value = struct.unpack(">I", data)[0]
        if value != lookup_kind:
            raise LifecycleError(f"{name}: Meta lookup is not the explicitly inherited JIGGLYPUFF column")
        aliases.append({"table": name, "rom_offset": at, "width": 4,
                        "value": value, "inheritance_contract": contracts[name]})
    egg_at, egg = linked_row("yoshi_egg", donor_id, 28)
    parent_egg_at, parent_egg = linked_row("yoshi_egg", lookup_kind, 28)
    if egg != parent_egg:
        raise LifecycleError("Meta Yoshi egg row differs from the explicitly inherited parent row")
    bounce_at, bounce = linked_row("down_bound_fgm", donor_id, 2)
    parent_bounce_at, parent_bounce = linked_row("down_bound_fgm", lookup_kind, 2)
    bounce_fgm = struct.unpack(">H", bounce)[0]
    if bounce != parent_bounce or bounce_fgm != 306:
        raise LifecycleError("Meta down-bounce cue changed from the qualified linked source cue 306")
    return {"lookup_kind": lookup_kind, "aliases": aliases,
            "yoshi_egg": {"rom_offset": egg_at, "parent_rom_offset": parent_egg_at,
                          "bytes_big_endian": egg.hex(), "width": 28,
                          "inheritance_contract": contracts["yoshi_egg"]},
            "down_bounce": {"rom_offset": bounce_at, "parent_rom_offset": parent_bounce_at,
                            "fgm": bounce_fgm, "width": 2,
                            "inheritance_contract": contracts["down_bound_fgm"]},
            "source_character_asm_sha256": sha(character_source.encode())}


def source_metadata(donor_path: Path, actions_path: Path) -> dict:
    donor = json.loads(donor_path.read_text())
    actions_data = actions_path.read_bytes()
    actions = json.loads(actions_data)
    if donor["phase"] != "resolved" or donor["sources"]["extra"]["commit"] != \
            "96621afea26a83305abaf81add07dcf5a9c5fe3e":
        raise LifecycleError("Meta lifecycle requires the resolved pinned EXTRA donor")
    lock = json.loads((ROOT / "docs/P4/source-lock.json").read_text())
    pins = {row["path"]: row["commit"] for row in lock["submodules"]}
    for component, relative in (("extra", "decomp/smashremix-plus-extra"), ("remix", "decomp/smashremix")):
        if donor["sources"][component]["commit"] != pins[relative] or not donor["sources"][component]["clean"]:
            raise LifecycleError("source lock differs from frozen lifecycle donor")
    rom_path, rom = frozen_output(donor, "review_tables")
    if actions["character"] != "MetaKnight" or actions["rom_sha256"] != sha(rom):
        raise LifecycleError("resolved action identity differs from frozen donor")
    symbols_path = Path(donor["destination"]) / "extra/review-symbols.log"
    symbols_data = symbols_path.read_bytes()
    if actions["symbols_sha256"] != sha(symbols_data):
        raise LifecycleError("resolved symbols differ from frozen donor")
    symbols = parse_symbols(symbols_data.decode())
    labels = {label: address for address, labels in symbols.items() for label in labels}
    if "Character.menu_zoom.table" not in labels:
        raise LifecycleError("missing linked menu zoom table symbol")
    # This donor segment mapping is source main.asm's origin/base contract,
    # also used by extra_native_asset_adapter.EventCompiler. Reject other domains.
    address = labels["Character.menu_zoom.table"] + actions["donor_character_id"] * 4
    if not 0x80400000 <= address < 0x80400000 + len(rom) - 0x02C00000:
        raise LifecycleError("menu zoom has an unclassified donor address")
    rom_offset = address - 0x80400000 + 0x02C00000
    scale_bits = struct.unpack_from(">I", rom, rom_offset)[0]
    scale = struct.unpack_from(">f", rom, rom_offset)[0]
    if not 0.0 < scale < 100.0:
        raise LifecycleError("invalid source menu/results zoom")
    if "Character.electric_hit.table" not in labels:
        raise LifecycleError("missing linked electric colanim family table")
    electric_address = labels["Character.electric_hit.table"] + actions["donor_character_id"] * 4
    if not 0x80400000 <= electric_address < 0x80400000 + len(rom) - 0x02C00000:
        raise LifecycleError("electric family has an unclassified donor address")
    electric_offset = electric_address - 0x80400000 + 0x02C00000
    electric_family = struct.unpack_from(">I", rom, electric_offset)[0]
    if electric_family != 0x18:
        raise LifecycleError("Meta electric program family changed; review its reached skeleton/art states")
    hook = next(row for row in actions["character_hooks"] if row["name"] == "kirby_inhale_struct")
    hook_bytes = bytes.fromhex(hook["bytes_big_endian"])
    if rom[hook["rom_offset"]:hook["rom_offset"] + 12] != hook_bytes:
        raise LifecycleError("inhale hook disagrees with frozen source bytes")
    copy_id, hat_id, star_scale, star_damage = struct.unpack(">HHII", hook_bytes)
    if copy_id != 8 or hat_id != 0:
        raise LifecycleError("Meta Knight no-copy policy changed; review required")
    source_root = Path(donor["destination"]) / "extra"
    originals = {row["path"]: row for row in donor["sources"]["extra"]["files"]}
    evidence = {}
    for name in ("config.yaml", "main.asm"):
        relative = "extra_characters/MetaKnight/" + name
        data = (source_root / relative).read_bytes()
        if sha(data) != originals[relative]["sha256"]:
            raise LifecycleError(f"frozen original Meta source changed: {name}")
        evidence[name] = {"sha256": sha(data), "text": data.decode()}
    config = evidence["config.yaml"]["text"]
    if re.search(r"^\s+base_character:\s+JIGGLYPUFF\s*$", config, re.MULTILINE) is None:
        raise LifecycleError("Meta base-character inheritance changed")
    character_path = source_root / "smashremix/src/Character.asm"
    character_data = character_path.read_bytes()
    remix_inputs = {row["path"]: row for row in donor["sources"]["remix"]["files"]}
    if sha(character_data) != remix_inputs["src/Character.asm"]["sha256"]:
        raise LifecycleError("frozen Character.asm inheritance source changed")
    victim_metadata = inherited_victim_metadata(rom, labels, actions["donor_character_id"],
                                                character_data.decode())
    results_match = re.search(r"^results:\s*\n((?:^[ \t]+.*\n)+)", config, re.MULTILINE)
    if results_match is None:
        raise LifecycleError("missing explicit Meta results configuration")
    results = {}
    for field in ("name", "name_x", "name_scale", "wins_x"):
        match = re.search(rf"^\s+{field}:\s*([^#\r\n]+)", results_match[1], re.MULTILINE)
        if match is None:
            raise LifecycleError(f"missing explicit results field {field}")
        value = match[1].strip().strip('"')
        results[field] = value.upper() if field == "name" else float(value)
    costume_match = re.search(r"Character\.set_default_costumes\(Character\.id\.METAKNIGHT,\s*([^)]*)\)",
                               evidence["main.asm"]["text"])
    if costume_match is None:
        raise LifecycleError("missing explicit Meta costume assignment")
    costumes = [int(value.strip(), 0) for value in costume_match[1].split(",")]
    if len(costumes) != 7 or any(not 0 <= value < 6 for value in costumes):
        raise LifecycleError("Meta costume domain changed")
    return {"extra_pin": donor["sources"]["extra"]["commit"], "rom_sha256": sha(rom),
            "actions_sha256": sha(actions_data), "symbols_sha256": sha(symbols_data),
            "model_scale_bits": scale_bits, "model_scale_rom_offset": rom_offset,
            "electric_family": electric_family, "electric_family_rom_offset": electric_offset,
            "copy_id": copy_id, "hat_id": hat_id, "star_scale_bits": star_scale,
            "star_damage": star_damage, "results": results, "costumes": costumes,
            "victim_lookup": victim_metadata,
            "source_inputs": {key: row["sha256"] for key, row in evidence.items()}}


def header(metadata: dict) -> str:
    result = metadata["results"]
    return "\n".join([
        "/* Generated by generate_meta_lifecycle_source.py; do not edit. */",
        f"/* EXTRA pin {metadata['extra_pin']}; resolved ROM {metadata['rom_sha256']}. */",
        "#ifndef NDS_META_LIFECYCLE_GENERATED_H", "#define NDS_META_LIFECYCLE_GENERATED_H",
        f"#define NDS_META_RESULTS_MODEL_SCALE_BITS 0x{metadata['model_scale_bits']:08x}u",
        f"#define NDS_META_ELECTRIC_COLANIM_FAMILY {metadata['electric_family']}u",
        f"#define NDS_META_INHERITED_VICTIM_LOOKUP_KIND {metadata['victim_lookup']['lookup_kind']}u",
        f"#define NDS_META_DOWN_BOUNCE_FGM {metadata['victim_lookup']['down_bounce']['fgm']}u",
        f"#define NDS_META_KIRBY_COPY_ID {metadata['copy_id']}u",
        f"#define NDS_META_KIRBY_HAT_ID {metadata['hat_id']}u",
        f"#define NDS_META_KIRBY_STAR_SCALE_BITS 0x{metadata['star_scale_bits']:08x}u",
        f"#define NDS_META_KIRBY_STAR_DAMAGE {metadata['star_damage']}u",
        f"#define NDS_META_RESULTS_NAME {json.dumps(result['name'])}",
        f"#define NDS_META_RESULTS_NAME_X {result['name_x']:.8f}F",
        f"#define NDS_META_RESULTS_NAME_SCALE {result['name_scale']:.8f}F",
        f"#define NDS_META_RESULTS_WINS_X {result['wins_x']:.8f}F",
        f"#define NDS_META_COSTUME_ROYAL {{ {', '.join(str(v) + 'u' for v in metadata['costumes'][:4])} }}",
        f"#define NDS_META_COSTUME_TEAM {{ {', '.join(str(v) + 'u' for v in metadata['costumes'][4:])} }}",
        "#endif", ""])


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--donor-manifest", type=Path, default=ROOT / "builds/p4/meta-knight-donor/donor-manifest.json")
    parser.add_argument("--actions", type=Path, default=ROOT / "builds/p4/meta-knight-donor/resolved-actions.json")
    parser.add_argument("--decomp-root", type=Path, default=ROOT / "decomp/BattleShip-main/decomp/src")
    parser.add_argument("--output-root", type=Path, default=ROOT / "builds/p4/meta-knight-lifecycle")
    args = parser.parse_args(argv)
    try:
        output = args.output_root.resolve()
        if not output.is_relative_to(ROOT / "builds") or "decomp" in output.parts:
            raise LifecycleError("lifecycle source output must be in this checkout's builds/")
        metadata = source_metadata(args.donor_manifest, args.actions)
        source_rows = []
        copies = {}
        for relative in ("ft/ftchar/ftkirby/ftkirbyspecialn.c", "ft/ftcommon/ftcommoncapturekirby.c",
                         "ft/ftpublic.c", "mn/mnvsmode/mnvsresults.c", "ef/efmanager.c",
                         "ft/ftcommon/ftcommoncapturecaptain.c", "ft/ftcommon/ftcommoncaptureyoshi.c",
                         "ft/ftcommon/ftcommondownwaitbounce.c", "ft/ftcommon/ftcommonthrow.c"):
            path = args.decomp_root / relative
            data = path.read_bytes()
            text = transform(relative, data.decode())
            copies[path.name] = text
            source_rows.append({"path": str(path.resolve()), "sha256": sha(data),
                                "output": path.name, "output_sha256": sha(text.encode())})
        # All transformations qualify before any output is published.
        output.mkdir(parents=True, exist_ok=True)
        for name, text in copies.items():
            (output / name).write_text(text, encoding="utf-8", newline="\n")
        (output / "nds_meta_lifecycle.generated.h").write_text(header(metadata), encoding="utf-8", newline="\n")
        metadata["source_copies"] = source_rows
        (output / "lifecycle-source-manifest.json").write_text(json.dumps(metadata, indent=2) + "\n", encoding="utf-8")
        print(f"generated {len(copies)} bounded lifecycle source seams at {output}")
    except (LifecycleError, OSError, ValueError, KeyError) as error:
        parser.exit(1, f"Meta lifecycle producer refused: {error}\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
