#!/usr/bin/env python3
"""Source-qualified mixed-width FTAttributes and fighter Sprite metadata.

The ARM32 compiler evaluates the actual mirrored types. Values and resource
identities come from the frozen big-endian native RELO payloads and their
recorded donor provenance. No parent fighter values, numeric offset guesses,
runtime pointer values, or generated payload edits are used.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import struct
import subprocess
import sys

from extra_native_asset_adapter import load_o2r

ROOT = Path(__file__).resolve().parents[2]
ATTR_FIELDS = (
    ("DEAD_FGM_0", "dead_fgm_ids[0]"), ("DEAD_FGM_1", "dead_fgm_ids[1]"),
    ("DEADUP_SFX", "deadup_sfx"), ("DAMAGE_SFX", "damage_sfx"),
    ("SMASH_SFX_0", "smash_sfx[0]"), ("SMASH_SFX_1", "smash_sfx[1]"),
    ("SMASH_SFX_2", "smash_sfx[2]"),
    ("ITEMTHROW_VEL_SCALE", "itemthrow_vel_scale"),
    ("ITEMTHROW_DAMAGE_SCALE", "itemthrow_damage_scale"),
    ("HEAVYGET_SFX", "heavyget_sfx"))
SPRITE_FIELDS = (
    "x", "y", "width", "height", "scalex", "scaley", "expx", "expy",
    "attr", "zdepth", "red", "green", "blue", "alpha", "startTLUT", "nTLUT",
    "LUT", "istart", "istep", "nbitmaps", "ndisplist", "bmheight", "bmHreal",
    "bmfmt", "bmsiz", "bitmap", "rsp_dl", "rsp_dl_next", "frac_s", "frac_t")
BITMAP_FIELDS = ("width", "width_img", "s", "t", "buf", "actualHeight", "LUToffset")


class MetadataError(ValueError):
    pass


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def record(path: Path) -> dict:
    return {"path": str(path.resolve()), "sha256": sha(path.read_bytes()),
            "size": path.stat().st_size}


def query_fields() -> list[tuple[str, str]]:
    rows = [("POINTER_SIZE", "sizeof(void *)"), ("U16_SIZE", "sizeof(u16)"),
            ("FTATTR_SIZE", "sizeof(FTAttributes)"), ("FTSPRITES_SIZE", "sizeof(FTSprites)"),
            ("SPRITE_SIZE", "sizeof(Sprite)"), ("BITMAP_SIZE", "sizeof(Bitmap)"),
            ("HURTBOX_DESC_SIZE", "sizeof(FTDamageCollDesc)"), ("MOBJSUB_SIZE", "sizeof(MObjSub)"),
            ("ATTR_SPRITES_OFFSET", "offsetof(FTAttributes, sprites)")]
    for name, member in ATTR_FIELDS:
        rows.extend((("ATTR_" + name + "_OFFSET", f"offsetof(FTAttributes, {member})"),
                     ("ATTR_" + name + "_SIZE", f"sizeof(((FTAttributes *)0)->{member})")))
    for member in ("stock_sprite", "stock_luts", "emblem"):
        rows.append(("FTSPRITES_" + member.upper() + "_OFFSET", f"offsetof(FTSprites, {member})"))
    for type_name, members in (("Sprite", SPRITE_FIELDS), ("Bitmap", BITMAP_FIELDS)):
        for member in members:
            prefix = type_name.upper() + "_" + member.upper()
            rows.extend(((prefix + "_OFFSET", f"offsetof({type_name}, {member})"),
                         (prefix + "_SIZE", f"sizeof((({type_name} *)0)->{member})")))
    return rows


def parse_layout_assembly(text: str, names: list[str]) -> dict[str, int]:
    labels = list(re.finditer(r"^nds_meta_layout:\s*$", text, re.MULTILINE))
    if len(labels) != 1:
        raise MetadataError("compiler did not emit one layout object")
    body = text[labels[0].end():]
    words = []
    for line in body.splitlines():
        if line.lstrip().startswith((".ident", ".section", ".global", ".type")):
            break
        match = re.fullmatch(r"\s*\.word\s+([0-9]+|0x[0-9a-fA-F]+)\s*", line)
        if match:
            words.append(int(match[1], 0))
        elif re.match(r"\s*\.(?:space|zero)\s+", line):
            match = re.fullmatch(r"\s*\.(?:space|zero)\s+(\d+)\s*", line)
            if match is None or int(match[1]) % 4:
                raise MetadataError("unsupported compiler zero-fill directive")
            words.extend([0] * (int(match[1]) // 4))
        elif line.strip() and not line.lstrip().startswith((".size", ".align")):
            raise MetadataError(f"unsupported compiler layout directive: {line!r}")
    if len(words) != len(names):
        raise MetadataError(f"compiler layout count {len(words)} != query count {len(names)}")
    result = dict(zip(names, words))
    if result.get("POINTER_SIZE") != 4 or result.get("U16_SIZE") != 2:
        raise MetadataError("layout oracle is not the ARM32 pointer/u16 ABI")
    return result


def compiler_layout(compiler: Path, decomp_root: Path, libnds_include: Path) -> tuple[dict, dict]:
    queries = query_fields()
    source = ("#include <ft/fighter.h>\n#include <PR/sp.h>\n"
              "_Static_assert(sizeof(void *) == 4, \"ARM32 required\");\n"
              "const unsigned nds_meta_layout[] = {\n" +
              ",\n".join(expr for _, expr in queries) + "\n};\n")
    argv = [str(compiler.resolve()), "-std=gnu11", "-march=armv5te", "-mtune=arm946e-s",
            "-mthumb", "-DARM9", "-D_LANGUAGE_C", "-DSSB64_TARGET_NDS", "-DREGION_US",
            "-DNDS_P4_METAKNIGHT=1", "-I" + str(ROOT / "include"),
            "-I" + str(decomp_root / "src"), "-I" + str(decomp_root / "src/sys"),
            "-I" + str(libnds_include), "-S", "-H", "-x", "c", "-o", "-", "-"]
    process = subprocess.run(argv, input=source, text=True, capture_output=True, check=False)
    if process.returncode:
        raise MetadataError("ARM32 layout oracle failed:\n" + process.stderr[-8000:])
    layout = parse_layout_assembly(process.stdout, [name for name, _ in queries])
    dependencies = set()
    for line in process.stderr.splitlines():
        match = re.fullmatch(r"\.+ (.+)", line)
        if match:
            path = Path(match[1]).resolve()
            if not path.is_file():
                raise MetadataError(f"compiler header provenance is unresolved: {path}")
            dependencies.add(path)
    if ROOT / "include/ft/fighter.h" not in dependencies or ROOT / "include/PR/sp.h" not in dependencies:
        raise MetadataError("compiler did not consume the project's mirrored fighter/Sprite headers")
    version = subprocess.run([str(compiler), "--version"], text=True, capture_output=True, check=True).stdout
    return layout, {"compiler": record(compiler), "version": version.splitlines()[0],
                    "argv": argv, "query_sha256": sha(source.encode()),
                    "assembly_sha256": sha(process.stdout.encode()),
                    "headers": [record(p) for p in sorted(dependencies)]}


def validate_provenance(bindings: dict, donor: dict, source_lock: dict) -> dict:
    if bindings.get("character") != "MetaKnight" or bindings.get("schema") != \
            "smash64ds.p4-native-runtime-bindings.v1":
        raise MetadataError("not Meta Knight native runtime bindings")
    if donor.get("phase") != "resolved" or donor.get("character") != "MetaKnight":
        raise MetadataError("donor resolution is incomplete or has the wrong character")
    pins = {row["path"]: row["commit"] for row in source_lock["submodules"]}
    expected = {"extra": pins["decomp/smashremix-plus-extra"], "remix": pins["decomp/smashremix"]}
    for name, pin in expected.items():
        if donor["sources"][name]["commit"] != pin or not donor["sources"][name]["clean"]:
            raise MetadataError(f"wrong or unclean frozen donor source pin: {name}")
    if pins["decomp/smashremix-plus-extra/smashremix"] != expected["remix"]:
        raise MetadataError("nested/direct source-lock pins differ")
    for binding_name, role in (("source_rom_sha256", "review_tables"),
                               ("source_files_rom_sha256", "assets")):
        path = donor["output_roles"][role]
        rows = [row for row in donor["outputs"] if row["path"] == path]
        if len(rows) != 1 or rows[0]["sha256"] != bindings[binding_name]:
            raise MetadataError(f"native bindings/frozen donor identity differs: {role}")
    return {"source_pins": expected, "source_rom_sha256": bindings["source_rom_sha256"],
            "source_files_rom_sha256": bindings["source_files_rom_sha256"]}


class VerifiedResources:
    def __init__(self, bindings: dict, assets_root: Path):
        self.root = assets_root.resolve()
        self.rows = {row["native_file_id"]: row for row in bindings["assets"]}
        if len(self.rows) != len(bindings["assets"]):
            raise MetadataError("duplicate emitted resource identities")
        self.cache = {}
        self.records = {}

    def get(self, asset_id: int):
        if asset_id in self.cache:
            return self.cache[asset_id]
        row = self.rows.get(asset_id)
        if row is None:
            raise MetadataError(f"missing required native resource {asset_id}")
        path = (self.root / row["path"]).resolve()
        if not path.is_relative_to(self.root) or not path.is_file() or path.is_symlink():
            raise MetadataError(f"escaping/missing native resource: {path}")
        if sha(path.read_bytes()) != row["container_sha256"]:
            raise MetadataError(f"native resource hash changed: {path}")
        found_id, resource = load_o2r(path)
        if found_id != asset_id or len(resource.payload) != row["size"]:
            raise MetadataError(f"native resource identity/size changed: {path}")
        self.cache[asset_id] = resource
        self.records[asset_id] = {**record(path), "asset_id": asset_id,
                                  "payload_sha256": sha(resource.payload)}
        return resource

    def pointer(self, owner, slot: int, label: str):
        ptr = owner.pointer(slot)
        if ptr is None:
            raise MetadataError(f"{label}: required source pointer is null")
        if ptr.resource is not None:
            asset_id = int(ptr.resource)
        elif ptr.dependency is not None and ptr.dependency.kind == "donor_file":
            asset_id = ptr.dependency.file_id
        else:
            raise MetadataError(f"{label}: unclassified pointer domain")
        resource = self.get(asset_id)
        if not 0 <= ptr.offset < len(resource.payload):
            raise MetadataError(f"{label}: source target out of bounds")
        return resource, ptr.offset


def span(payload: bytes, at: int, size: int, label: str) -> bytes:
    if at < 0 or size < 0 or at > len(payload) or size > len(payload) - at:
        raise MetadataError(f"{label}: source span out of bounds")
    return payload[at:at + size]


def attribute_fields(payload: bytes, at: int, layout: dict) -> list[dict]:
    if at < 0 or at % 4:
        raise MetadataError("FTAttributes offset is negative/unaligned")
    span(payload, at, layout["FTATTR_SIZE"], "FTAttributes")
    fields = []
    occupied = set()
    for name, member in ATTR_FIELDS:
        offset, size = layout["ATTR_" + name + "_OFFSET"], layout["ATTR_" + name + "_SIZE"]
        if size != 2 or offset % 2 or offset + size > layout["FTATTR_SIZE"]:
            raise MetadataError(f"{member}: unsupported mixed field layout")
        if occupied.intersection(range(offset, offset + size)):
            raise MetadataError("overlapping FTAttributes mixed fields")
        occupied.update(range(offset, offset + size))
        value = struct.unpack(">H", span(payload, at + offset, size, member))[0]
        fields.append({"name": name, "member": member, "relative_offset": offset,
                       "payload_offset": at + offset, "width": size,
                       "source_type": "u16_be", "expected_native_value": value,
                       "normalization": "restore u16 lanes after the owning word byte swap"})
    return fields


def sprite_metadata(resources: VerifiedResources, owner, at: int, layout: dict, role: str) -> dict:
    span(owner.payload, at, layout["SPRITE_SIZE"], role + " Sprite")
    fields = {}
    for member in SPRITE_FIELDS:
        prefix = "SPRITE_" + member.upper()
        offset, size = layout[prefix + "_OFFSET"], layout[prefix + "_SIZE"]
        raw = span(owner.payload, at + offset, size, member)
        if member in ("LUT", "bitmap", "rsp_dl", "rsp_dl_next"):
            continue
        if member in ("scalex", "scaley"):
            fields[member] = {"binary32_bits": int.from_bytes(raw, "big")}
        else:
            fields[member] = int.from_bytes(raw, "big", signed=(size == 2 and member != "attr"))
    if (fields["width"] <= 0 or fields["height"] <= 0 or fields["nbitmaps"] <= 0 or
            fields["bmfmt"] not in range(5) or fields["bmsiz"] not in range(4)):
        raise MetadataError(f"{role} Sprite: invalid source geometry/format/count")
    bitmap_owner, bitmap_at = resources.pointer(owner, at + layout["SPRITE_BITMAP_OFFSET"], role + " Bitmap")
    span(bitmap_owner.payload, bitmap_at, fields["nbitmaps"] * layout["BITMAP_SIZE"], role + " Bitmap table")
    bitmaps = []
    for index in range(fields["nbitmaps"]):
        offset = bitmap_at + index * layout["BITMAP_SIZE"]
        row = {}
        for member in BITMAP_FIELDS:
            if member == "buf":
                continue
            prefix = "BITMAP_" + member.upper()
            raw = span(bitmap_owner.payload, offset + layout[prefix + "_OFFSET"],
                       layout[prefix + "_SIZE"], role + " Bitmap." + member)
            row[member] = int.from_bytes(raw, "big", signed=True)
        if row["width_img"] <= 0 or row["actualHeight"] <= 0:
            raise MetadataError(f"{role} Bitmap: invalid source stride/height")
        pixels, pixels_at = resources.pointer(bitmap_owner, offset + layout["BITMAP_BUF_OFFSET"], role + " pixels")
        pixels_bytes = (row["width_img"] * row["actualHeight"] * (4 << fields["bmsiz"]) + 7) // 8
        span(pixels.payload, pixels_at, pixels_bytes, role + " pixels")
        bitmaps.append({"offset": offset, "fields": row, "pixels_asset_id": int(pixels.name),
                        "pixels_offset": pixels_at, "pixels_bytes": pixels_bytes})
    return {"role": role, "asset_id": int(owner.name), "offset": at,
            "bytes": layout["SPRITE_SIZE"], "fields": fields,
            "bitmap_asset_id": int(bitmap_owner.name), "bitmap_offset": bitmap_at,
            "bitmap_stride": layout["BITMAP_SIZE"], "bitmaps": bitmaps}


def build_metadata(bindings: dict, resources: VerifiedResources, layout: dict) -> dict:
    mains = [row for row in bindings["assets"] if row["role"] == "MAIN"]
    if len(mains) != 1 or bindings["core_files"][0]["native_file_id"] != mains[0]["native_file_id"]:
        raise MetadataError("native Main identity is ambiguous or differs from core descriptor")
    main = resources.get(mains[0]["native_file_id"])
    at = bindings["attribute_offset"]
    fields = attribute_fields(main.payload, at, layout)
    sprites_owner, sprites_at = resources.pointer(main, at + layout["ATTR_SPRITES_OFFSET"], "FTSprites")
    span(sprites_owner.payload, sprites_at, layout["FTSPRITES_SIZE"], "FTSprites")
    sprites = []
    for role, member in (("STOCK", "stock_sprite"), ("EMBLEM", "emblem")):
        owner, pos = resources.pointer(sprites_owner, sprites_at + layout[
            "FTSPRITES_" + member.upper() + "_OFFSET"], role + " Sprite")
        sprites.append(sprite_metadata(resources, owner, pos, layout, role))
    return {"schema": "smash64ds.meta-mixed-attribute-metadata.v1", "character": "MetaKnight",
            "address_domain": "native asset ID and payload offset; no loaded pointer values",
            "attribute_asset_id": int(main.name), "attribute_offset": at,
            "attribute_fields": fields,
            "attribute_lane_swap_words": sorted({row["relative_offset"] & ~3 for row in fields}),
            "ftsprites": {"asset_id": int(sprites_owner.name), "offset": sprites_at,
                          "bytes": layout["FTSPRITES_SIZE"]},
            "sprites": sprites, "target_layout": layout,
            "resource_inputs": [resources.records[key] for key in sorted(resources.records)]}


def render_header(data: dict) -> str:
    lines = ["/* Generated by generate_meta_attribute_metadata.py; do not edit. */",
             "#ifndef NDS_METAKNIGHT_ATTRIBUTE_METADATA_GENERATED_H",
             "#define NDS_METAKNIGHT_ATTRIBUTE_METADATA_GENERATED_H", "",
             f"#define NDS_META_ATTRIBUTE_ASSET_ID {data['attribute_asset_id']}u",
             f"#define NDS_META_ATTRIBUTE_OFFSET {data['attribute_offset']}u"]
    provenance = data.get("provenance")
    if provenance is not None:
        main = next(row for row in data["resource_inputs"] if row["asset_id"] == data["attribute_asset_id"])
        lines += [f"/* EXTRA pin: {provenance['source_pins']['extra']} */",
                  f"/* Main container SHA256: {main['sha256']} */",
                  f"/* Main source payload SHA256: {main['payload_sha256']} */",
                  f"/* ARM32 layout assembly SHA256: {provenance['layout_oracle']['assembly_sha256']} */"]
    for row in data["attribute_fields"]:
        lines += [f"#define NDS_META_ATTR_{row['name']}_OFFSET {row['relative_offset']}u",
                  f"#define NDS_META_ATTR_{row['name']}_EXPECTED {row['expected_native_value']}u"]
    lines += [f"#define NDS_META_FTSPRITES_ASSET_ID {data['ftsprites']['asset_id']}u",
              f"#define NDS_META_FTSPRITES_OFFSET {data['ftsprites']['offset']}u", ""]
    for row in data["sprites"]:
        for name, value in (("ASSET_ID", row["asset_id"]), ("OFFSET", row["offset"]),
                            ("WIDTH", row["fields"]["width"]), ("HEIGHT", row["fields"]["height"]),
                            ("NBITMAPS", row["fields"]["nbitmaps"]), ("BMFMT", row["fields"]["bmfmt"]),
                            ("BMSIZ", row["fields"]["bmsiz"]), ("BITMAP_ASSET_ID", row["bitmap_asset_id"]),
                            ("BITMAP_OFFSET", row["bitmap_offset"])):
            lines.append(f"#define NDS_META_{row['role']}_SPRITE_{name} {value}u")
    lines += ["", "/* X(asset_id, sprite_offset, width, height, count, fmt, siz, bitmap_asset_id, bitmap_offset). */",
              "#define NDS_META_SPRITES(X) \\"]
    for index, row in enumerate(data["sprites"]):
        fields = row["fields"]
        values = (row["asset_id"], row["offset"], fields["width"], fields["height"], fields["nbitmaps"],
                  fields["bmfmt"], fields["bmsiz"], row["bitmap_asset_id"], row["bitmap_offset"])
        lines.append("    X(" + ", ".join(f"{value}u" for value in values) + ")" +
                     (" \\" if index + 1 < len(data["sprites"]) else ""))
    lines += ["", "#endif", ""]
    return "\n".join(lines)


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--bindings", type=Path, default=ROOT / "builds/p4/meta-knight-native/native-runtime-bindings.json")
    parser.add_argument("--donor-manifest", type=Path, default=ROOT / "builds/p4/meta-knight-donor/donor-manifest.json")
    parser.add_argument("--assets-root", type=Path, default=ROOT / "builds/p4/meta-knight-native")
    parser.add_argument("--compiler", type=Path, default=Path(shutil.which("arm-none-eabi-gcc") or
                                                            "C:/devkitPro/devkitARM/bin/arm-none-eabi-gcc.exe"))
    parser.add_argument("--decomp-root", type=Path, default=ROOT / "decomp/BattleShip-main/decomp")
    parser.add_argument("--libnds-include", type=Path, default=Path("C:/devkitPro/libnds/include"))
    parser.add_argument("--output-header", type=Path)
    parser.add_argument("--output-json", type=Path)
    parser.add_argument("--verify-only", action="store_true")
    args = parser.parse_args(argv)
    if not args.verify_only and not (args.output_header and args.output_json):
        parser.error("provide --output-header and --output-json, or --verify-only")
    try:
        bindings = json.loads(args.bindings.read_text(encoding="utf-8"))
        donor = json.loads(args.donor_manifest.read_text(encoding="utf-8"))
        lock_path = ROOT / "docs/P4/source-lock.json"
        provenance = validate_provenance(bindings, donor, json.loads(lock_path.read_text(encoding="utf-8")))
        if donor["source_lock"]["sha256"] != sha(lock_path.read_bytes()):
            raise MetadataError("source-lock bytes changed since donor resolution")
        layout, oracle = compiler_layout(args.compiler, args.decomp_root, args.libnds_include)
        data = build_metadata(bindings, VerifiedResources(bindings, args.assets_root), layout)
        data["provenance"] = {**provenance, "bindings": record(args.bindings),
                              "donor_manifest": record(args.donor_manifest), "source_lock": record(lock_path),
                              "layout_oracle": oracle, "producer": record(Path(__file__))}
        if not args.verify_only:
            if args.output_header.suffix != ".h" or args.output_json.suffix != ".json" or \
                    args.output_header.resolve() == args.output_json.resolve():
                raise MetadataError("metadata requires distinct .h and .json output paths")
            for path in (args.output_header, args.output_json):
                resolved = path.resolve()
                if not resolved.is_relative_to(ROOT / "builds") and not resolved.is_relative_to(ROOT / "include/nds/generated"):
                    raise MetadataError("metadata outputs must be in this checkout's builds/ or include/nds/generated/")
                if resolved in {args.bindings.resolve(), args.donor_manifest.resolve(), lock_path.resolve()}:
                    raise MetadataError("metadata output would overwrite a required provenance input")
                resolved.parent.mkdir(parents=True, exist_ok=True)
            args.output_header.write_text(render_header(data), encoding="utf-8", newline="\n")
            args.output_json.write_text(json.dumps(data, indent=2, sort_keys=True) + "\n", encoding="utf-8")
        print(json.dumps({"fields": {row["name"]: row["expected_native_value"] for row in data["attribute_fields"]},
                          "sprites": [{key: row[key] for key in ("role", "asset_id", "offset", "bitmap_asset_id", "bitmap_offset")}
                                      for row in data["sprites"]]}, indent=2))
    except (MetadataError, OSError, ValueError, KeyError) as error:
        parser.exit(1, f"Meta mixed metadata refused: {error}\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
