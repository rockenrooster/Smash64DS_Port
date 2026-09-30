"""Execute production CSS hit tests and falsify source/layout/stock drift.

These host fixtures qualify UI producers only; natural-input DS CSS, preview,
announcer, match HUD and scene-lifetime acceptance remains due.
"""

from __future__ import annotations

import importlib.util
import os
from pathlib import Path
import re
import shutil
import struct
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

from source_test_helpers import braced, function
import p4_meta_knight_ui as meta

ROOT = Path(__file__).resolve().parents[2]
CSS = (ROOT / "src/nds/nds_menu_shell_css.c").read_text(encoding="utf-8")


def load_module(name, filename):
    spec = importlib.util.spec_from_file_location(name, ROOT / "scripts/menus" / filename)
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


def load_ui(enabled):
    with patch.dict(os.environ, {"NDS_P4_METAKNIGHT": str(int(enabled))}):
        return load_module(f"meta_ui_{int(enabled)}", "generate_mn_ui_kit.py")


class MetaKnightCssTests(unittest.TestCase):
    def test_full_cells_fit_frame_and_preserve_one_player(self):
        for enabled in (False, True):
            ui = load_ui(enabled)
            left = 2 if enabled else 25
            for portrait in range(12 + int(enabled)):
                column, row = (6, 0) if portrait == 12 else (portrait % 6, portrait // 6)
                expected = (left + column * 45, 36 + row * 43)
                self.assertEqual(ui.css_portrait_pos(portrait), expected)
                x, y = expected
                self.assertLessEqual(x + 45, 320)
                self.assertLessEqual(y + 43, 126)
            with self.assertRaises(ui.ConvertError):
                ui.css_portrait_pos(12 + int(enabled))
            for portrait in range(12):
                self.assertEqual(ui.onep_css_portrait_pos(portrait),
                                 (25 + (portrait % 6) * 45, 36 + (portrait // 6) * 43))

    def test_gate_and_flash_use_meta_assets_and_kirby_emblem(self):
        ui = load_ui(True)
        self.assertEqual(len(ui.CSS_GATE_STATES), 42)
        self.assertEqual(ui.CSS_FIGHTER_TOKEN[-1], "METAKNIGHT")
        self.assertEqual(ui.CSS_EMBLEM_SYMBOL[-1], "llFTEmblemSpritesKirbySprite")
        gate = ui.css_gate(0, "MAN_METAKNIGHT", "GateMan%dPLUT", False,
                           12, ui.CSS_TINT_MAN, True)
        name = [part for part in gate.parts if part.source_png == "nameplate.png"]
        self.assertEqual(len(name), 1)
        self.assertEqual(name[0].dest, (53, 13))
        regular = [p.source_png for p in ui.css_screen_parts() if p.source_png]
        flashed = [p.source_png for p in ui.css_screen_parts(12) if p.source_png]
        self.assertEqual(regular, ["portrait.png"])
        self.assertEqual(flashed, ["portrait_flash.png"])
        self.assertTrue(any(s.token == "CSS_FLASH_METAKNIGHT_ON_READY1"
                            for s in ui.SURFACE_SOURCES))
        legacy = load_ui(False)
        self.assertEqual(len(legacy.CSS_GATE_STATES), 39)
        self.assertFalse(any(p.source_png for p in legacy.css_screen_parts()))

    def test_real_c_centers_hit_tests_save_and_sentinels(self):
        cc = shutil.which("gcc") or shutil.which("clang")
        if cc is None:
            self.skipTest("host C compiler unavailable")
        names = ("ndsMenuShellCssSaveLocked", "ndsMenuShellCssFighterLocked",
                 "ndsMenuShellCssPortraitX", "ndsMenuShellCssPortraitY",
                 "ndsMenuShellCssCenterPuck", "ndsMenuShellCssPuckFighterKind",
                 "ndsMenuShellCssGateState")
        functions = "\n".join(function(CSS, name) for name in names)
        arrays = "\n".join(braced(CSS, rf"static const u8 {name}\b", True)
                            for name in ("kNdsCssPortraitFighter", "kNdsCssFighterPortrait"))
        definitions = "\n".join(re.findall(
            r"^#define NDS_CSS_(?:LEGACY_PORTRAITS|PORTRAITS|GRID_COLUMNS|GRID_LEFT|PUCK_HOME_X|PUCK_HOME_Y|GATE_\w+)\b[^\n]*",
            CSS, re.M))
        mask = CSS[CSS.index("#if NDS_P2_KIRBY"):CSS.index("static u8 sCssPkind")]
        roster = (ROOT / "include/nds/nds_p4_roster.h").read_text()
        roster = roster.replace("#include <PR/ultratypes.h>", "")
        fixture = r'''
#include <stdint.h>
#include <stdio.h>
typedef uint8_t u8;
typedef uint32_t u32;
typedef int32_t s32;
typedef int16_t s16;
enum { FALSE, TRUE };
enum { nFTKindMario, nFTKindFox, nFTKindDonkey, nFTKindSamus, nFTKindLuigi,
       nFTKindLink, nFTKindYoshi, nFTKindCaptain, nFTKindKirby, nFTKindPikachu,
       nFTKindPurin, nFTKindNess, nFTKindNull=28 };
#define NDS_DEV_SCENE_HARNESS_NORMAL 0u
#define LBBACKUP_MASK_FIGHTER(k) (1u << (k))
static u32 gNdsSceneHarnessMode;
static struct { u32 fighter_mask; } gSCManagerBackupData;
static s16 sCssPuckX[4], sCssPuckY[4];
static u8 sCssPkind[4], sCssFkind[4], sCssSelected[4];
enum { nFTPlayerKindMan, nFTPlayerKindCom, nFTPlayerKindNot };
'''
        flags = "\n".join(f"#define NDS_P2_{name} 1" for name in
                            ("KIRBY", "PURIN", "NESS", "YOSHI", "PIKACHU", "LINK",
                             "LUIGI", "DONKEY", "CAPTAIN", "SAMUS"))
        checks = r'''
#define CHECK(c) do { if (!(c)) { fprintf(stderr,"failed line %d\n",__LINE__); return 1; } } while(0)
int main(void) {
    unsigned kind;
    const unsigned expected_portraits[12] = {1,9,2,4,0,3,7,5,8,10,11,6};
    gSCManagerBackupData.fighter_mask=0xfffu;
    for(kind=0;kind<12;kind++) {
        unsigned p=expected_portraits[kind];
        int left=NDS_P4_METAKNIGHT ? 2 : 25;
        ndsMenuShellCssCenterPuck(0,kind);
        CHECK(sCssPuckX[0] == left + (int)(p%6)*45 + 11);
        CHECK(sCssPuckY[0] == 36 + (int)(p/6)*43 + 10);
        CHECK(ndsMenuShellCssPuckFighterKind(0)==kind);
    }
    ndsMenuShellCssCenterPuck(0,29);
#if NDS_P4_METAKNIGHT
    CHECK(sCssPuckX[0]==283 && sCssPuckY[0]==46);
    CHECK(ndsMenuShellCssPuckFighterKind(0)==29);
    CHECK(ndsRosterSelectionIndex(29)==12);
    CHECK(ndsRosterRuntimeKind(12)==29);
    sCssFkind[0]=29;
    sCssPkind[0]=nFTPlayerKindMan; CHECK(ndsMenuShellCssGateState(0)==15);
    sCssPkind[0]=nFTPlayerKindCom; sCssSelected[0]=1;
    CHECK(ndsMenuShellCssGateState(0)==28);
    sCssSelected[0]=0; CHECK(ndsMenuShellCssGateState(0)==41);
    sCssPkind[0]=nFTPlayerKindNot; CHECK(ndsMenuShellCssGateState(0)==0);
    gSCManagerBackupData.fighter_mask=0;
    CHECK(ndsMenuShellCssSaveLocked(29)==FALSE);
    CHECK(ndsMenuShellCssFighterLocked(29)==FALSE);
    sCssPuckY[0]=89; CHECK(ndsMenuShellCssPuckFighterKind(0)==28);
#else
    CHECK(sCssPuckX[0]==51 && sCssPuckY[0]==161);
    CHECK(ndsMenuShellCssFighterLocked(29)==TRUE);
    CHECK(ndsRosterRuntimeKind(12)==28);
    sCssFkind[0]=29; sCssPkind[0]=nFTPlayerKindMan;
    CHECK(ndsMenuShellCssGateState(0)==1);
#endif
    CHECK(ndsMenuShellCssFighterLocked(27)==TRUE);
    CHECK(ndsMenuShellCssFighterLocked(28)==TRUE);
    CHECK(ndsMenuShellCssFighterLocked(0xffffffffu)==TRUE);
    gSCManagerBackupData.fighter_mask=0;
    CHECK(ndsMenuShellCssFighterLocked(nFTKindLuigi)==TRUE);
    CHECK(ndsMenuShellCssFighterLocked(nFTKindMario)==FALSE);
    gSCManagerBackupData.fighter_mask=1u << nFTKindLuigi;
    CHECK(ndsMenuShellCssFighterLocked(nFTKindLuigi)==FALSE);
    gSCManagerBackupData.fighter_mask=0xfffu;
    sCssPuckX[0]=(s16)NDS_CSS_GRID_LEFT-14; sCssPuckY[0]=46;
    CHECK(ndsMenuShellCssPuckFighterKind(0)==28);
    sCssPuckX[0]=(s16)(NDS_CSS_GRID_LEFT+NDS_CSS_GRID_COLUMNS*45)-13;
    CHECK(ndsMenuShellCssPuckFighterKind(0)==28);
    sCssPuckX[0]=(s16)NDS_CSS_GRID_LEFT; sCssPuckY[0]=23;
    CHECK(ndsMenuShellCssPuckFighterKind(0)==28);
    sCssPuckY[0]=110; CHECK(ndsMenuShellCssPuckFighterKind(0)==28);
    return 0;
}
'''
        with tempfile.TemporaryDirectory() as temporary:
            tmp = Path(temporary)
            for enabled in (0, 1):
                source = f"#define NDS_P4_METAKNIGHT {enabled}\n" + fixture + flags + "\n" + roster
                source += "\n" + definitions + "\n" + mask + arrays + "\n" + functions + checks
                path, executable = tmp / "css.c", tmp / "css.exe"
                path.write_text(source)
                build = subprocess.run([cc, "-std=c11", "-Wall", "-Wextra", "-Werror",
                                        str(path), "-o", str(executable)], capture_output=True, text=True)
                self.assertEqual(build.returncode, 0, build.stderr)
                result = subprocess.run([str(executable)], capture_output=True, text=True)
                self.assertEqual(result.returncode, 0, result.stderr)


class MetaKnightSourceTests(unittest.TestCase):
    def setUp(self):
        if not (meta.source_dir(ROOT) / "portrait.png").exists():
            self.skipTest("source-qualified EXTRA corpus unavailable; set META_KNIGHT_SOURCE_DIR")

    def test_exact_images_and_distinct_flash(self):
        regular = meta.load_image(ROOT, "portrait.png")
        flashed = meta.load_image(ROOT, "portrait_flash.png")
        self.assertNotEqual(regular, flashed)
        self.assertEqual((len(regular[0]), len(regular)), (32, 32))
        name = meta.load_image(ROOT, "nameplate.png")
        self.assertEqual((len(name[0]), len(name)), (72, 16))
        self.assertTrue(any(a for row in name for _, _, _, a in row))

    def test_changed_or_unqualified_art_fails(self):
        with self.assertRaises(ValueError):
            meta.load_image(ROOT, "../portrait.png")
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "portrait.png"
            path.write_bytes((meta.source_dir(ROOT) / "portrait.png").read_bytes() + b"drift")
            with patch.dict(os.environ, {"META_KNIGHT_SOURCE_DIR": temporary}):
                with self.assertRaisesRegex(ValueError, "source drift"):
                    meta.load_image(ROOT, "portrait.png")

    def test_exact_stock_and_six_palette_conversion(self):
        texture, palettes = meta.load_stock(ROOT)
        ui = load_ui(True)
        hud = load_module("meta_hud_test", "generate_battle_hud.py")
        gfx, ds_palettes = hud.stock_cells(ui, texture, palettes)
        self.assertEqual((len(texture), len(gfx), len(ds_palettes)), (80, 32, 6))
        self.assertEqual(len(set(tuple(p) for p in ds_palettes)), 6)
        # Independent CI4 index oracle: undo the donor odd-row qword exchange,
        # then nearest sample 8x10 -> 6x8. Unused OBJ columns remain transparent.
        source = []
        for y in range(10):
            row = texture[y * 8:(y + 1) * 8]
            if y & 1:
                row = row[4:] + row[:4]
            source.append([value for byte in row for value in (byte >> 4, byte & 15)][:8])
        actual = [value for byte in gfx for value in (byte & 15, byte >> 4)]
        expected = [source[y * 10 // 8][x * 8 // 6] if x < 6 else 0
                    for y in range(8) for x in range(8)]
        self.assertEqual(actual, expected)
        for source_palette, palette in zip(palettes, ds_palettes):
            for index in range(1, 16):
                n64 = struct.unpack_from(">H", source_palette, index * 2)[0]
                self.assertEqual(palette[index] & 0x7fff,
                                 ((n64 >> 11) & 31) | (((n64 >> 6) & 31) << 5) |
                                 (((n64 >> 1) & 31) << 10))
        with self.assertRaises(hud.BakeError):
            hud.stock_cells(ui, texture[:-1], palettes)


if __name__ == "__main__":
    unittest.main()
