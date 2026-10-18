#!/usr/bin/env python3
"""P5 stage registry (docs/P5/New_Stages.md T1) and the 6x3 stage select's
spec (T6, shared plan 02).

`scripts/p5/stages.json` holds one row per P5 stage: the port's decisions
(gkind, music policy, item rule, camera routing, P7 unlock id, whether the
stage has landed). Everything the staged Remix build already knows -- the
select icon, the preview zoom, the training wallpaper parent, the map file
and header offset -- is read from it here, from the assembled tables
(Stages.asm `icon_offset_table`, `zoom_table`, `background_table`,
`stage_file_table`, `class_table`), never copied by hand.

    p5_stages.py --header <out.h>
        the C rows every shared site reads (include/nds/nds_p5.h)
    p5_stages.py --sss <dir> --staging <remix staging>
        sss.json for the UI kit bake (scripts/menus/generate_mn_ui_kit.py,
        NDS_P5_SSS), nds_p5_sss.generated.inc for the select's shell, and
        the donor's icon and logo files (o2r/0a04, o2r/0014). Everything under
        <dir> derives from the user's ROM: a build directory only.
    p5_stages.py --list
        "name:gkind" per row, for make
"""
from __future__ import annotations

import argparse
import json
import re
import struct
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
REGISTRY = HERE / "stages.json"
sys.path.insert(0, str(HERE.parent / "p4"))

ICON_FILE = 0xA04   # File.STAGE_ICONS: 40x30 icons, Stages.asm icon_offset_table
LOGO_FILE = 0x14    # series logos (Remix-extended FTEmblemSprites)

# The grid, in the source's 320x240 frame (owner decision 1, 2026-10-12):
# six columns by three rows of 48x36 cells on a 50x38 pitch, the source's own
# cell and gutter (mnMapsMakeIcons :540/:546), from x 10 so the six span the
# width. Remix's 40x30 icons draw at 1.2 to fill a cell.
GRID = {"columns": 6, "rows": 3, "x": 10, "y": 16, "pitch_x": 50, "pitch_y": 38,
        "icon_w": 48, "icon_h": 36}
# The lower block (the STAGE SELECT label and band, the wooden plaque, the
# name plate and the preview panel) moves down by the grid's growth: the
# vanilla second row ends at y 104 (68 + 36), this grid's third at 128.
LOWER_DY = 8

# Row-major (owner decision 1): Remix's page 1 with the three later-page
# stages in the cells of stages P5 does not carry, a spare locked cell and
# RANDOM. Vanilla tokens name the source's gkind; P5 tokens name registry rows.
LAYOUT = (
    "CASTLE", "JUNGLE", "HYRULE", "ZEBES", "INISHIE", "META_CRYSTAL",
    "YOSTER", "PUPUPU", "SECTOR", "YAMABUKI", "DUEL_ZONE", "FINAL_DESTINATION",
    "BOWSERS_STADIUM", "FIRST_DESTINATION", "HYRULE_TEMPLE", "DINOSAUR_LAND", "SPARE", "RANDOM",
)
RANDOM_GKIND = 0xDE   # mnMapsGetGroundKind's own spelling of RANDOM
NONE_GKIND = 0xFF     # the spare cell: never a ground

# The nine VS stages: gkind (= Remix id), and the source's name and emblem
# sprites (mnMapsMakeName :581, mnMapsMakeEmblem :784, gkind order) with the
# emblem's offset (mnMapsSetLogoPosition :752).
VANILLA = {
    "CASTLE": (0, "llMNMapsPeachsCastleTextSprite", "llFTEmblemSpritesMarioSprite", (3, 19)),
    "SECTOR": (1, "llMNMapsSectorZTextSprite", "llFTEmblemSpritesFoxSprite", (3, 19)),
    "JUNGLE": (2, "llMNMapsCongoJungleTextSprite", "llFTEmblemSpritesDonkeySprite", (3, 20)),
    "ZEBES": (3, "llMNMapsPlanetZebesTextSprite", "llFTEmblemSpritesMetroidSprite", (2, 20)),
    "HYRULE": (4, "llMNMapsHyruleCastleTextSprite", "llFTEmblemSpritesZeldaSprite", (3, 17)),
    "YOSTER": (5, "llMNMapsYoshisIslandTextSprite", "llFTEmblemSpritesYoshiSprite", (-1, 19)),
    "PUPUPU": (6, "llMNMapsDreamLandTextSprite", "llFTEmblemSpritesKirbySprite", (1, 20)),
    "YAMABUKI": (7, "llMNMapsSaffronCityTextSprite", "llFTEmblemSpritesPMonstersSprite", (1, 20)),
    "INISHIE": (8, "llMNMapsMushroomKingdomTextSprite", "llFTEmblemSpritesMarioSprite", (3, 19)),
}


# A vanilla 1P arena's preview panel is its own wallpaper (mnMapsMakePreview-
# Wallpaper draws the map header's), and the o2r containers that hold those
# files are named for other stages: the file ids are what
# scripts/stages/generate_native_wallpapers.py checks (0x60 Final Destination,
# 0x61 Duel Zone, 0x62 Meta Crystal), at the Sprite offset every stage
# wallpaper file shares.
VENUE_WALLPAPER = {
    "metal": "StageLastWallpaper",
    "zako": "StageInishieWallpaper",
    "last": "StageYamabukiWallpaper",
}
WALLPAPER_SPRITE = 0x26C88


class StageError(SystemExit):
    pass


def write_if_changed(path: Path, data) -> None:
    """Rewrite a generated file only when its bytes change, so make's
    dependents (the UI kit bake, a whole recompile) stay put on a no-op run."""
    raw = data.encode("utf-8") if isinstance(data, str) else bytes(data)
    if path.exists() and path.read_bytes() == raw:
        return
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(raw)


def load_registry() -> list[dict]:
    rows = json.loads(REGISTRY.read_text(encoding="utf-8"))["stages"]
    seen_g, seen_u = set(), set()
    for row in rows:
        if row["gkind"] in seen_g or row["unlock_id"] in seen_u:
            raise StageError(f"stages.json: duplicate gkind or unlock id at {row['name']}")
        seen_g.add(row["gkind"])
        seen_u.add(row["unlock_id"])
        if row["token"] not in LAYOUT:
            raise StageError(f"stages.json: {row['token']} has no select cell")
    return rows


def asm_scope_constants(text: str, scope: str) -> dict[str, int]:
    """`constant NAME(expr)` rows inside `scope <scope> {`, expr a hex sum."""
    m = re.search(r"scope\s+" + re.escape(scope) + r"\s*:?\s*\{", text)
    if m is None:
        raise StageError(f"scope {scope} not found")
    depth, i = 1, m.end()
    while depth and i < len(text):
        depth += {"{": 1, "}": -1}.get(text[i], 0)
        i += 1
    out = {}
    for name, expr in re.findall(r"constant\s+(\w+)\(([^)]*)\)", text[m.end():i]):
        try:
            out[name] = sum(int(t.strip(), 0) for t in expr.split("+"))
        except ValueError:
            continue
    return out


def emit_header(path: Path, rows: list[dict]) -> None:
    lines = [
        "/* Generated by scripts/p5/p5_stages.py from scripts/p5/stages.json. Do not edit. */",
        "#ifndef NDS_P5_STAGES_GENERATED_H",
        "#define NDS_P5_STAGES_GENERATED_H",
        "",
        "/* X(gkind, TOKEN, unlock_id, landed, camera_inishie, items_none, bgm0, bgm1,",
        " *   bgm_switch_frame) -- bgm0 -1: the song the stage's own row names (T7). */",
        "#define NDS_P5_STAGE_ROWS(X) \\",
    ]
    for row in rows:
        bgm = row.get("bgm") or []
        b0 = bgm[0] if bgm else -1
        b1 = bgm[1] if len(bgm) > 1 else -1
        lines.append(
            f"    X({row['gkind']}, {row['token']}, {row['unlock_id']}, "
            f"{1 if row['landed'] else 0}, {1 if row['camera'] == 'inishie' else 0}, "
            f"{1 if row['items'] == 'none' else 0}, {b0}, {b1}, "
            f"{row.get('bgm_switch_frame', 0)}) \\")
    lines += ["    /* end */", "",
              f"#define NDS_P5_STAGE_COUNT {len(rows)}u",
              f"#define NDS_P5_GKIND_MAX {max(r['gkind'] for r in rows)}u", "",
              "#endif", ""]
    text = "\n".join(lines)
    write_if_changed(path, text)


def remix_tables(staging: Path):
    import remix_rom as R

    rom = R.Rom(staging / "ssb64asm.z64")
    sym, _ = R.load_symbols(staging / "logfile.log")

    def table(name: str) -> int:
        key = "Stages." + name
        if key not in sym:
            raise StageError(f"{key} not in {staging / 'logfile.log'}")
        return sym[key]

    def u32(name: str, i: int) -> int:
        return rom.u32_ram(table(name) + 4 * i)

    def u8(name: str, i: int) -> int:
        return rom.read_ram(table(name) + i, 1)[0]

    def f32(name: str, i: int) -> float:
        return struct.unpack(">f", rom.read_ram(table(name) + 4 * i, 4))[0]

    return rom, u32, u8, f32, table


def _f32_unit(bits: int) -> int:
    """CharacterSelect.asm `position` holds float32 bit patterns (X_SMASH is
    0x3F800000, 1.0); the bake places whole source units, half up (SMASH's
    19.5 -> 20)."""
    value = struct.unpack(">f", struct.pack(">I", bits & 0xFFFFFFFF))[0]
    return int(value + 0.5) if value >= 0 else -int(-value + 0.5)


def emit_sss(out: Path, staging: Path, rows: list[dict], stages_root: Path | None = None) -> None:
    rom, u32, u8, f32, table = remix_tables(staging)
    css_text = (staging / "src" / "CharacterSelect.asm").read_text(encoding="utf-8", errors="replace")
    logo_offsets = asm_scope_constants(css_text, "offset")
    logo_pos = asm_scope_constants(css_text, "position")
    by_token = {row["token"]: row for row in rows}

    o2r = out / "o2r"
    o2r.mkdir(parents=True, exist_ok=True)
    for fid in (ICON_FILE, LOGO_FILE):
        write_if_changed(o2r / f"{fid:04x}", rom.o2r(fid))

    cells = []
    for index, token in enumerate(LAYOUT):
        row_i, col = divmod(index, GRID["columns"])
        cell = {"cell": index, "row": row_i, "col": col, "token": token,
                "x": GRID["x"] + col * GRID["pitch_x"], "y": GRID["y"] + row_i * GRID["pitch_y"]}
        if token == "RANDOM":
            cell.update(gkind=RANDOM_GKIND, icon=u32("icon_offset_random", 0), locked=False)
        elif token == "SPARE":
            cell.update(gkind=NONE_GKIND, icon=None, locked=True)
        elif token in VANILLA:
            gkind, name_sym, emblem_sym, emblem_pos = VANILLA[token]
            cell.update(gkind=gkind, remix_id=gkind, icon=u32("icon_offset_table", gkind),
                        name_symbol=name_sym, emblem_symbol=emblem_sym,
                        emblem_dx=emblem_pos[0], emblem_dy=emblem_pos[1], locked=False)
        else:
            row = by_token[token]
            rid = row["remix_id"]
            if u8("class_table", rid) != 0:
                raise StageError(f"{token}: Remix class {u8('class_table', rid)} is not BATTLE")
            logo = row["logo"]
            if logo not in logo_offsets:
                raise StageError(f"{token}: no series logo {logo} in CharacterSelect.asm")
            cell.update(gkind=row["gkind"], remix_id=rid, icon=u32("icon_offset_table", rid),
                        name_text=row["title"].upper(),
                        logo=logo, logo_sprite=logo_offsets[logo],
                        logo_dx=_f32_unit(logo_pos.get("X_" + logo, 0)),
                        logo_dy=_f32_unit(logo_pos.get("Y_" + logo, 0)),
                        # P7 S0 (A10): a scaled stage's preview zooms out
                        # by its scale, so it still fits its frame.
                        zoom=round(f32("zoom_table", rid) / float(row.get("owner_scale", 1.0)), 4),
                        training_parent=u8("background_table", rid),
                        map_file=u32("stage_file_table", rid * 2),
                        map_offset=u32("stage_file_table", rid * 2 + 1),
                        locked=not row["landed"])
        cells.append(cell)

    # The preview panels the bake can draw: a landed vanilla arena's own
    # wallpaper (a Remix stage's joins with its export, T3).
    previews = []
    for row in rows:
        if row["landed"] and row["source"] == "vanilla":
            previews.append({"gkind": row["gkind"], "token": row["token"],
                             "container": VENUE_WALLPAPER[row["descriptor"]],
                             "offset": WALLPAPER_SPRITE})
        elif row["landed"]:
            if stages_root is None:
                raise StageError(f"{row['name']}: a landed Remix stage needs --stages-root")
            stage = json.loads((stages_root / row["name"] / "stage.json").read_text(encoding="utf-8"))
            fid, off = stage["wallpaper"]
            previews.append({"gkind": row["gkind"], "token": row["token"],
                             "container": "p4:" + str((stages_root / row["name"] / "o2r" / f"{fid:04x}").resolve()),
                             "offset": off})
    spec = {"schema": "p5-sss-v1", "grid": GRID, "lower_dy": LOWER_DY, "cells": cells,
            "previews": previews,
            "files": {"icons": str(o2r / f"{ICON_FILE:04x}"), "logos": str(o2r / f"{LOGO_FILE:04x}")}}
    write_if_changed(out / "sss.json", json.dumps(spec, indent=1) + "\n")
    write_shell_inc(out / "nds_p5_sss.generated.inc", cells, previews)
    write_preview_header(out / "nds_p5_preview.generated.h", cells)
    print(f"p5_stages: {len(cells)} select cells -> {out}")


def preview_lift() -> float:
    """World units the preview camera rises so its image moves LOWER_DY rows
    down: mnMapsMakePreviewCamera's fovy and Peach's Castle's camera
    (mnMapsSetPreviewCameraPosition), over the source's 220-row viewport."""
    import math
    eye, at = (-3000.0, 3000.0, 9000.0), (1700.0, 1800.0, 0.0)
    depth = math.dist(eye, at)
    rows_world = 2.0 * depth * math.tan(math.radians(32.588150) / 2.0) / 220.0
    return LOWER_DY * rows_world


def write_preview_header(path: Path, cells: list[dict]) -> None:
    """The select's 3D preview rows for battleship_mnmaps.c: each P5 stage's
    model scale, Remix's own `zoom_table` (set_preview_position_)."""
    rows = [c for c in cells if "zoom" in c]
    lines = [
        "/* Generated by scripts/p5/p5_stages.py --sss. Do not edit. */",
        "#ifndef NDS_P5_PREVIEW_GENERATED_H",
        "#define NDS_P5_PREVIEW_GENERATED_H",
        "/* The select's panel moves down LOWER_DY source rows; the preview's",
        " * camera lifts by the world height of that many rows at its target's",
        " * depth (fovy 32.588150, eye (-3000, 3000, 9000) to Peach's Castle's",
        " * target (1700, 1800, 0), a 220-row viewport). */",
        f"#define NDS_P5_PREVIEW_DY {LOWER_DY}",
        f"#define NDS_P5_PREVIEW_LIFT {preview_lift():.1f}F",
        "/* X(gkind, zoom) */",
        "#define NDS_P5_PREVIEW_ROWS(X) \\",
    ]
    lines += [f"    X({c['gkind']}, {c['zoom']!r}F) \\" for c in rows]
    lines += ["    /* end */", "#endif", ""]
    write_if_changed(path, "\n".join(lines))


def write_shell_inc(path: Path, cells: list[dict], previews: list[dict]) -> None:
    lines = [
        "/* Generated by scripts/p5/p5_stages.py --sss. Do not edit. */",
        f"#define NDS_SSS_SLOTS {len(cells)}u",
        f"#define NDS_SSS_ROW {GRID['columns']}u",
        f"#define NDS_SSS_ICON_X0 {GRID['x']}",
        f"#define NDS_SSS_ICON_PITCH {GRID['pitch_x']}",
        f"#define NDS_SSS_ICON_Y0 {GRID['y']}",
        f"#define NDS_SSS_ICON_PITCH_Y {GRID['pitch_y']}",
        f"#define NDS_SSS_LOWER_DY {LOWER_DY}",
        "/* The cell's ground (0xDE RANDOM, 0xFF the spare), row-major. */",
        "static const u8 kNdsSssSlotGkind[NDS_SSS_SLOTS] = {",
    ]
    lines += [f"    {c['gkind']}u, /* {c['token']} */" for c in cells]
    lines += ["};",
              "/* Each cell's plaque copy on BG3 (generate_mn_ui_kit.py sss_plaque_front). */",
              "static const NdsUiKitSurfaceId kNdsSssPlaqueFrontSurface[NDS_SSS_SLOTS] = {"]
    lines += [f"    NDS_MN_UI_KIT_SURFACE_SSS_PLAQUE_FRONT_{c['cell']}," for c in cells]
    lines += ["};",
              "/* The P5 stages whose preview panel the kit bakes: X(gkind, TOKEN). */",
              "#define NDS_SSS_P5_PREVIEW_ROWS(X) \\"]
    lines += [f"    X({p['gkind']}, {p['token']}) \\" for p in previews]
    lines += ["    /* end */", ""]
    write_if_changed(path, "\n".join(lines))


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--header", type=Path)
    ap.add_argument("--sss", type=Path)
    ap.add_argument("--staging", type=Path)
    ap.add_argument("--stages-root", type=Path,
                    help="scripts/p5/stage_export.py's --out, for landed Remix stages")
    ap.add_argument("--list", action="store_true")
    ap.add_argument("--remix-landed", action="store_true")
    args = ap.parse_args()
    rows = load_registry()
    if args.list:
        print(" ".join(f"{r['name']}:{r['gkind']}" for r in rows))
        return 0
    if args.remix_landed:
        # "name:c_name" per landed Remix stage, for the NitroFS staging rule.
        print(" ".join(f"{r['name']}:{r['c_name']}" for r in rows
                       if r["source"] == "remix" and r["landed"]))
        return 0
    if args.header is not None:
        emit_header(args.header, rows)
    if args.sss is not None:
        if args.staging is None:
            raise StageError("--sss needs --staging")
        emit_sss(args.sss, args.staging, rows, args.stages_root)
    return 0


if __name__ == "__main__":
    sys.exit(main())
