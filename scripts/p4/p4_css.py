#!/usr/bin/env python3
"""P4 character select: the 30-cell grid and its donor art, for the UI kit bake.

The P4 roster is the 12 originals plus the 19 Remix selections
(docs/P4/New_Characters.md, and Mad Piano at the owner's request,
2026-10-11). The select screen uses Smash Remix's own layout
(src/CharacterSelect.asm): a grid of 24-pixel cells whose 32x32 portraits
from CHARACTER_PORTRAITS (file 0xA05) draw at scale 0.8125, with the
originals in the same 6x2 block they hold in the vanilla grid. Remix's six
selections that are not P4 roster (Marina, Dr. Mario, Young Link, Goemon,
Conker, Mewtwo) give their cells to the six P4 selections Remix's main grid
does not carry (Metal Mario, Roy, Lanky, Meta Knight, MRGAW, Snake), each
next to its series where the grid allows. Mad Piano, on Remix's bonus
bookend there, has a cell of his own: an eleventh column, its two spare
cells the NONE plate (owner, 2026-10-11), the grid centred where Remix's
ten columns sit. The P6 fighters take those two spare cells (Palutena row 1,
Ike row 2); an authored content shows its base's portrait, name, logo and
call until its own (docs/P6/P6_PLAN.md A9; scripts/p6/p6_present.py appends
them, the call is its announcer cue). Metal Mario's cell selects the source MMario kind itself
(KINDS): Remix's portrait, name and logo, and the source's call.

Writes, under --out (a build directory: everything here is derived from the
user's ROM):
  o2r/0a05 o2r/0011 o2r/0014   the donor's portrait, name and logo files
  css.json                     grid cells and per-fighter rows the bake and the
                               runtime tables read
"""
from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import remix_rom as R  # noqa: E402

PORTRAIT_FILE = 0xA05   # File.CHARACTER_PORTRAITS
NAME_FILE = 0x11        # name textures (Remix-extended MNPlayersCommon)
LOGO_FILE = 0x14        # series logos (Remix-extended FTEmblemSprites)
FLASH_STRIDE = 0x860    # CharacterSelect.asm: the white flash follows each portrait

# The grid, in the source's 320x240 frame. Remix's (CharacterSelect.asm) is
# 24-unit cells at x 39 with portraits at 0.8125 (PORTRAIT_SCALE); the DS
# grid spans the screen's width instead (owner, 2026-10-11: "lots of extra
# room"): eleven 29-unit cells from x 0, portraits at 15/16, so a cell's
# right/bottom unit lies under its neighbour's as Remix's two do.
GRID = {"columns": 11, "rows": 3, "pitch": 29, "x": 0, "y": 30,
        "scale_num": 15, "scale_den": 16}
# The rest of the screen around it (same owner request): the mode label and
# BACK move 18 units up to the top edge; the four player cards, everything on
# them and their 3D previews scale by 80/69 and sit at an 80-unit pitch from
# x 2, so they fill the width and their centres land on the source's preview
# roots (840 world units apart, 80 source units); the scaled 92-unit card
# ends at the bottom edge (y 133 + 107); PRESS START, which sat under the
# cards at y 219, moves into the band between the grid and the cards.
SCREEN = {"header_dy": -18, "card_pitch": 80, "card_x": 2, "card_y": 133,
          "card_k": [80, 69], "press_y": 122}

# The DS P4 grid, row-major. Originals keep Remix's (and the vanilla grid's)
# cells; each P4 replacement names the Remix cell it takes.
LAYOUT = (
    "MRGAW", "PIANO", "METAL", "LUIGI", "MARIO", "DONKEY", "LINK", "SAMUS", "CAPTAIN", "GND", "SONIC",
    "PALUTENA", "DEDEDE", "METAKNIGHT", "NESS", "YOSHI", "KIRBY", "FOX", "PIKACHU", "JIGGLYPUFF", "FALCO", "SHEIK",
    "IKE", "SNAKE", "CRASH", "WARIO", "PEACH", "BOWSER", "WOLF", "LANKY", "ROY", "MARTH", "RANDOM",
)
# The random chooser (owner 2026-10-17: Banjo & Kazooie's cell, the content
# retired, "a random chooser like SSS has"): Remix's RANDOM portrait; a token
# dropped on it takes a random selection among the unlocked cells
# (src/nds/nds_menu_shell_css.c, NDS_P4_SEL_RANDOM).
RANDOM = "RANDOM"
# Original cast: Remix constant name -> fkind.
ORIGINALS = {"MARIO": 0, "FOX": 1, "DONKEY": 2, "SAMUS": 3, "LUIGI": 4, "LINK": 5,
             "YOSHI": 6, "CAPTAIN": 7, "KIRBY": 8, "PIKACHU": 9, "JIGGLYPUFF": 10, "NESS": 11}
# Source kinds the P4 roster makes selectable as themselves (selection id =
# fkind, content 0): Metal Mario is the existing MMario kind. Remix gives the
# vanilla cast no add_to_css row, so its gate art and call are its rows in
# CharacterSelect.asm's per-character tables (name_texture_table,
# series_logo_offset_table, fgm_table): Metal Mario's name, the Mario Bros.
# logo, the announcer's "Metal Mario" (FGM.asm names.METAL_MARIO).
KINDS = {"METAL": {"fkind": 13, "name": "METAL", "logo": "MARIO_BROS", "fgm": "METAL_MARIO"}}
# P4 selections: token (Remix constant name, or an authored content's NAME)
# -> DS content id, from the content registry (scripts/p4/contents.json); a
# selection without a compiled content draws its portrait locked. ART names
# the Remix selection whose portrait, name, logo and call a token shows: its
# own, or an authored content's base's.
sys.path.insert(0, str(Path(__file__).resolve().parent))
import p4_contents  # noqa: E402

_REGISTRY = p4_contents.load_registry()
CONTENT = {p4_contents.token(row): row["id"] for row in _REGISTRY}
ART = {p4_contents.token(row): p4_contents.source_row(row, _REGISTRY)["remix"] for row in _REGISTRY}


class CssError(SystemExit):
    pass


def asm_scope_constants(text: str, scope: str) -> dict[str, int]:
    """`constant NAME(expr)` rows inside `scope <scope> {`, expr a hex sum."""
    m = re.search(r"scope\s+" + re.escape(scope) + r"\s*:?\s*\{", text)
    if m is None:
        raise CssError(f"scope {scope} not found")
    depth, i = 1, m.end()
    while depth and i < len(text):
        depth += {"{": 1, "}": -1}.get(text[i], 0)
        i += 1
    body = text[m.end():i]
    out = {}
    for name, expr in re.findall(r"constant\s+(\w+)\(([^)]*)\)", body):
        try:
            out[name] = sum(int(t.strip(), 0) for t in expr.split("+"))
        except ValueError:
            continue
    return out


def add_to_css_rows(text: str) -> dict[str, dict]:
    """add_to_css rows. EXTRA's appended characters give the announcer FGM and
    the name texture as numbers (`0x0609`, `0x00028970 + 0x10`) where Remix's
    own rows name constants."""
    rows = {}
    pat = re.compile(r"add_to_css\(Character\.id\.(\w+),\s*([\w.]+),\s*([\d.]+),\s*(0x[0-9A-Fa-f]+),"
                     r"\s*(\w+),\s*(name_texture\.\w+|0x[0-9A-Fa-f]+(?:\s*\+\s*0x[0-9A-Fa-f]+)?),"
                     r"\s*portrait_offsets\.(\w+),\s*(-?\w+)\)")
    for m in pat.finditer(text):
        rows[m.group(1)] = {"fgm": m.group(2), "circle": float(m.group(3)),
                            "action": int(m.group(4), 16), "logo": m.group(5),
                            "name": m.group(6), "portrait": m.group(7)}
    return rows


def hex_sum(expr: str) -> int:
    return sum(int(t.strip(), 0) for t in expr.split("+"))


def announcer_ids(fgm_text: str) -> dict[str, int]:
    return asm_scope_constants(fgm_text, "names")


def write_c_tables(path: Path, cells: list[dict], fighters: list[dict]) -> None:
    """The runtime half of the grid (src/nds/nds_menu_shell_css.c, P4 arm)."""
    def sel(cell: dict) -> str:
        if cell.get("random"):
            return "NDS_P4_SEL_RANDOM"
        if cell["fkind"] is not None:
            return f"{cell['fkind']}u"
        if cell["locked"]:
            return "(u8)nFTKindNull"
        return f"NDS_P4_SEL_BASE + {cell['content']}u"

    gate_index = {f["content"]: i for i, f in enumerate(fighters) if f["content"]}
    kinds = [(i, f) for i, f in enumerate(fighters) if not f["content"]]
    last = fighters[-1]["token"] if fighters else None
    lines = [
        "/* Generated by scripts/p4/p4_css.py from the staged Remix donor build",
        " * (CharacterSelect.asm). Build output only, never tracked. */",
        f"#define NDS_P4_CSS_CELLS {len(cells)}u",
        f"#define NDS_P4_CSS_COLUMNS {GRID['columns']}",
        f"#define NDS_P4_CSS_ROWS {GRID['rows']}",
        f"#define NDS_P4_CSS_PITCH {GRID['pitch']}",
        f"#define NDS_P4_CSS_X {GRID['x']}",
        f"#define NDS_P4_CSS_Y {GRID['y']}",
        '#include "nds_p4_css_screen.generated.h"',
        "/* Each cell's selection: an original's fkind, NDS_P4_SEL_BASE + content,",
        " * NDS_P4_SEL_RANDOM for the random chooser, or nFTKindNull for a",
        " * selection this build does not compile. */",
        "static const u8 kNdsP4CssCellSel[NDS_P4_CSS_CELLS] = {",
    ]
    for row in range(GRID["rows"]):
        chunk = cells[row * GRID["columns"]:(row + 1) * GRID["columns"]]
        lines.append("    " + ", ".join(sel(c) for c in chunk) + ",")
    lines += ["};",
              "/* Remix selections with gate art, after the originals' twelve, in bake",
              " * order (generate_mn_ui_kit.py CSS_FIGHTER_TOKEN); 0xff = none. */",
              f"#define NDS_P4_CSS_GATE_FIGHTERS {len(fighters)}u",
              "static const u8 kNdsP4CssGateIndex[NDS_P4_CONTENT_LIMIT] = {"]
    lines += [f"    [{c}] = {i}u," for c, i in sorted(gate_index.items())]
    lines += ["};",
              "/* add_to_css: the announcer's name call when the token lands. */",
              "static const u16 kNdsP4CssAnnounceFgm[NDS_P4_CONTENT_LIMIT] = {"]
    lines += [f"    [{f['content']}] = {f['announce_fgm']:#x}u," for f in fighters if f["content"]]
    lines += ["};"]
    # A selectable source kind (KINDS): its gate art's index and its call.
    for i, f in kinds:
        lines += [f"#define NDS_P4_CSS_GATE_KIND_{f['fkind']} {i}u",
                  f"#define NDS_P4_CSS_ANNOUNCE_KIND_{f['fkind']} {f['announce_fgm']}u"]
    if last is not None:
        lines += [f"#define NDS_P4_CSS_GATE_LAST_FFA NDS_MN_UI_KIT_SURFACE_CSS_GATE_3_HOLD_{last}",
                  f"#define NDS_P4_CSS_GATE_LAST_TEAM NDS_MN_UI_KIT_SURFACE_CSS_GATE_TEAM_GREEN_3_HOLD_{last}"]
    lines.append("")
    path.write_text("\n".join(lines), encoding="utf-8", newline="\n")
    # The screen offsets alone, for the select's preview code
    # (battleship_mnplayersvs.c), which takes none of the tables.
    path.with_name("nds_p4_css_screen.generated.h").write_text("\n".join([
        "/* Generated by scripts/p4/p4_css.py (SCREEN). Build output only. */",
        "#ifndef NDS_P4_CSS_SCREEN_GENERATED_H",
        "#define NDS_P4_CSS_SCREEN_GENERATED_H",
        f"#define NDS_P4_CSS_HEADER_DY ({SCREEN['header_dy']})",
        f"#define NDS_P4_CSS_CARD_PITCH {SCREEN['card_pitch']}",
        f"#define NDS_P4_CSS_CARD_X {SCREEN['card_x']}",
        f"#define NDS_P4_CSS_CARD_Y {SCREEN['card_y']}",
        f"#define NDS_P4_CSS_CARD_KN {SCREEN['card_k'][0]}",
        f"#define NDS_P4_CSS_CARD_KD {SCREEN['card_k'][1]}",
        f"#define NDS_P4_CSS_PRESS_Y {SCREEN['press_y']}",
        "/* add_to_css's action: each content's Selected demo status, as "
        "Remix's",
        " * selection_action_table holds it (0x0001000N: nFTDemoStatusWin "
        "N), which",
        " * is not always its parent's. */",
        "#define NDS_P4_CSS_SELECTED_ROWS { [0] = 0u" + "".join(
            f", [{f['content']}] = {f['action'] & 0xff}u"
            for f in fighters
            if f["content"] and (f["action"] is not None)) + " }",
        "#endif", ""]), encoding="utf-8", newline="\n")


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--staging", type=Path, required=True)
    ap.add_argument("--out", type=Path, required=True)
    ap.add_argument("--contents", default="",
                    help="tokens of the contents this build compiles (Remix names, or an authored "
                         "content's NAME), comma or space separated (e.g. FALCO IKE)")
    args = ap.parse_args()

    src = args.staging / "src"
    css_text = (src / "CharacterSelect.asm").read_text(encoding="utf-8", errors="replace")
    fgm_text = (src / "FGM.asm").read_text(encoding="utf-8", errors="replace")
    portraits = asm_scope_constants(css_text, "portrait_offsets")
    names = asm_scope_constants(css_text, "name_texture")
    logo_offsets = asm_scope_constants(css_text, "offset")
    logo_pos = asm_scope_constants(css_text, "position")
    rows = add_to_css_rows(css_text)
    announce = announcer_ids(fgm_text)
    built = {c.upper() for c in re.split(r"[,\s]+", args.contents) if c}
    unknown = built - set(CONTENT)
    if unknown:
        raise CssError(f"no content id for {sorted(unknown)}")

    rom = R.Rom(args.staging / "ssb64asm.z64")
    o2r = args.out / "o2r"
    o2r.mkdir(parents=True, exist_ok=True)
    for fid in (PORTRAIT_FILE, NAME_FILE, LOGO_FILE):
        (o2r / f"{fid:04x}").write_bytes(rom.o2r(fid))
    # P6 A9: an authored content's own portrait, name and logo, appended to
    # the copies (scripts/p6/p6_present.py); what it has none of stays its
    # base's.
    sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "p6"))
    import p6_audio  # noqa: E402
    import p6_present  # noqa: E402
    present = {}
    donor = {"portraits": o2r / f"{PORTRAIT_FILE:04x}", "names": o2r / f"{NAME_FILE:04x}",
             "logos": o2r / f"{LOGO_FILE:04x}"}
    for reg in _REGISTRY:
        token = p4_contents.token(reg)
        if "base" in reg and token in built:
            present[token] = p6_present.apply(reg["name"], donor, portraits, names, logo_offsets, ART[token])
            call = p6_audio.cue_ids(reg["name"]).get("announcer")
            if call is not None:
                present[token]["announce_fgm"] = call

    cells = []
    for index, token in enumerate(LAYOUT):
        row, col = divmod(index, GRID["columns"])
        cell = {"cell": index, "row": row, "col": col, "token": token,
                "x": GRID["x"] + col * GRID["pitch"], "y": GRID["y"] + row * GRID["pitch"]}
        if token == RANDOM:
            cell.update(fkind=None, content=0, locked=False, random=True)
        elif token in ORIGINALS:
            cell.update(fkind=ORIGINALS[token], content=0, locked=False)
        elif token in KINDS:
            cell.update(fkind=KINDS[token]["fkind"], content=0, locked=False)
        else:
            content = CONTENT.get(token, 0)
            cell.update(fkind=None, content=content, locked=token not in built)
        # A selection this build does not compile shows Remix's NONE plate (the
        # question mark).
        key = ART.get(token, token)
        key = key if key in portraits else None
        if token == RANDOM:
            key = "RANDOM"
        if key is None or cell["locked"]:
            key = "NONE"
        cell["portrait"] = portraits[key]
        if not cell["locked"] and "portrait" in present.get(token, {}):
            cell["portrait"] = present[token]["portrait"]
        # The random chooser never flashes (the token moves to the cell it
        # picks, which does): its "flash" is its portrait.
        cell["flash"] = cell["portrait"] if cell.get("random") else cell["portrait"] + FLASH_STRIDE
        cells.append(cell)

    fighters = []
    for token in sorted(built, key=lambda t: CONTENT[t]):
        row = rows.get(ART[token])
        if row is None:
            raise CssError(f"{token}: no add_to_css row for {ART[token]}")
        if row["fgm"].startswith("0x"):
            fgm = int(row["fgm"], 16)
        else:
            fgm_name = row["fgm"].rsplit(".", 1)[-1]
            if fgm_name not in announce:
                raise CssError(f"{token}: announcer FGM {row['fgm']} not found")
            fgm = announce[fgm_name]
        name_sprite = (names[row["name"].split(".", 1)[1]] if row["name"].startswith("name_texture.")
                       else hex_sum(row["name"]))
        logo = row["logo"]
        art = present.get(token, {})
        logo = art.get("logo", logo)
        fighters.append({
            "token": token, "content": CONTENT[token],
            "name_sprite": art.get("name_sprite", name_sprite),
            "logo": logo, "logo_sprite": art.get("logo_sprite", logo_offsets[logo]),
            "logo_x": logo_pos.get("X_" + logo, 0), "logo_y": logo_pos.get("Y_" + logo, 0),
            "announce_fgm": art.get("announce_fgm", fgm), "circle": row["circle"], "action": row["action"],
        })
    # The selectable source kinds' gate art follows the contents' in the bake.
    for token, kind in KINDS.items():
        if token not in LAYOUT:
            continue
        if kind["fgm"] not in announce:
            raise CssError(f"{token}: announcer FGM names.{kind['fgm']} not found")
        logo = kind["logo"]
        fighters.append({
            "token": token, "content": 0, "fkind": kind["fkind"],
            "name_sprite": names[kind["name"]],
            "logo": logo, "logo_sprite": logo_offsets[logo],
            "logo_x": logo_pos.get("X_" + logo, 0), "logo_y": logo_pos.get("Y_" + logo, 0),
            "announce_fgm": announce[kind["fgm"]], "circle": None, "action": None,
        })

    spec = {"schema": "p4-css-v1", "grid": GRID, "screen": SCREEN, "cells": cells, "fighters": fighters,
            # P7 U7: a save-locked cell draws Remix's NONE plate at run time.
            "none_portrait": portraits["NONE"],
            "files": {"portraits": str(o2r / f"{PORTRAIT_FILE:04x}"),
                      "names": str(o2r / f"{NAME_FILE:04x}"),
                      "logos": str(o2r / f"{LOGO_FILE:04x}")}}
    (args.out / "css.json").write_text(json.dumps(spec, indent=1), encoding="utf-8", newline="\n")
    write_c_tables(args.out / "nds_p4_css.generated.inc", cells, fighters)
    print(f"p4_css: {len(cells)} cells, {len(fighters)} P4 fighters -> {args.out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
