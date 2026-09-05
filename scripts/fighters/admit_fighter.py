#!/usr/bin/env python3
"""Admit a P2-3 fighter's port boilerplate from the BattleShip source.

The P2-3 pipeline landed Luigi..Yoshi by hand-written patch scripts that each
appended one fighter's block after the previous fighter's at ~60 sites across
the port. This script is those scripts made generic: it derives the per-fighter
data from the decomp (enums, status-table callbacks, gmsound ordinals, weapon
vars, effect descs, stock-icon offsets, CSS symbols, selected clip) plus a
small hand-checked spec (kind, slots, weapon-attribute pins, entry arm, TUs),
and inserts every block after the previous fighter's block in the chain. It
does NOT write the fighter's translation units, compat shims, native-owner
tables or audio pins -- those remain the source-reading part of a row.

Usage:
  python scripts/fighters/admit_fighter.py --repo-root . --fighter ness [--dry-run]
  python scripts/fighters/admit_fighter.py --repo-root . --fighter ness --audio-table
"""
from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
import re
import sys
from pathlib import Path

# ---------------------------------------------------------------------------
# Specs. Kinds are BattleShip fttypes.h ordinals. `prev` names the fighter
# whose blocks this one is inserted after; the chain must be admitted in order.
# Stock icon layouts come from <N>_<Name>Main.c `d<Name>Main_stock_luts` and the
# model's palette symbol offsets; the 88-byte CI4 texture precedes the first
# LUT. Weapon pins are (define suffix, file symbol suffix, asset field, offset,
# attack0_y, attack1_y, top, center, bottom, width) read from the relocData
# WPAttributes initializers.
# ---------------------------------------------------------------------------
SPECS: dict[str, dict] = {
    "yoshi": dict(
        Name="Yoshi", kind=6, token="YOSHI", owner_slot=8, image_slot=6,
        css_portrait="llMNPlayersPortraitsYoshiSprite",
        css_emblem="llFTEmblemSpritesYoshiSprite",
        css_name="llMNPlayersCommonYoshiTextSprite",
        model="YoshiModel", model_file_id=0x152, main_file_id=0xf7,
        selected_file=444, tus=["battleship_yoshi.c", "battleship_yoshi_weapons.c",
                                "battleship_ftcommon_captureyoshi.c"],
        stock=dict(sprite=0xAAA8, texture=0xA958,
                   palettes=[0xA9B0, 0xA9D8, 0xAA00, 0xAA28, 0xAA50, 0xAA78]),
        prev="pikachu",
    ),
    "ness": dict(
        Name="Ness", kind=11, token="NESS", owner_slot=9, image_slot=7,
        css_portrait="llMNPlayersPortraitsNessSprite",
        css_emblem="llFTEmblemSpritesMotherSprite",
        css_name="llMNPlayersCommonNessTextSprite",
        model="NessModel", model_file_id=0x14f, main_file_id=0xef,
        attr_offset=0x5bc, selected_file=437,
        tus=["battleship_ness.c", "battleship_ness_weapons.c", "battleship_ness_items.c"],
        # 239_NessMain.c dNessMain_stock_luts: palette_0xC0E0 then
        # gap_0xC100_sub_0x8/0x30/0x58; reloc_data_symbols llNessModelStockSprite.
        stock=dict(sprite=0xC188, texture=0xC088,
                   palettes=[0xC0E0, 0xC108, 0xC130, 0xC158]),
        # 239_NessMain.c @0x0C PK Thunder head (attack 0, map 100/0/-100/100),
        # @0x40 PK Thunder trail (attack_offsets {0,-100,0}, map 50/0/-50/50);
        # 240_NessSpecial1.c @0x00 PK Fire spark (attack 0, map 10/0/-10/10).
        weapon_pins=[
            ("NESS_MAIN_PK_THUNDER", "NESS_MAIN", 0x0c, 0, 0, 100, 0, -100, 100),
            ("NESS_MAIN_PK_THUNDER_TRAIL", "NESS_MAIN", 0x40, -100, 0, 50, 0, -50, 50),
            ("NESS_SPECIAL1_PK_FIRE", "NESS_SPECIAL1", 0x00, 0, 0, 10, 0, -10, 10),
        ],
        extra_assets=[("NESS_SPECIAL1", 0xf0)],
        effect_descs=[], effect_files=[],
        entry_statuses=("nFTNessStatusAppearRStart", "nFTNessStatusAppearLStart"),
        entry_effect=None,
        entry_note=("BattleShip ftcommonentry.c:24. Ness is the second two-status "
                    "entry ladder (AppearRStart/AppearLStart -> Wait -> End)."),
        stubs=[], prev="yoshi",
    ),
    "purin": dict(
        Name="Purin", kind=10, token="PURIN", owner_slot=10, image_slot=8,
        css_portrait="llMNPlayersPortraitsPurinSprite",
        css_emblem="llFTEmblemSpritesPMonstersSprite",
        css_name="llMNPlayersCommonJigglypuffTextSprite",
        model="PurinModel", model_file_id=0x14a, main_file_id=0xe9,
        attr_offset=0x474, selected_file=470,
        tus=["battleship_purin.c"],
        # 233_PurinMain.c dPurinMain_stock_luts: palette_0x7AE0 then
        # gap_0x7B00_sub_0x8/0x30/0x58/0x80; llPurinModelStockSprite = 0x7bb0.
        stock=dict(sprite=0x7BB0, texture=0x7A88,
                   palettes=[0x7AE0, 0x7B08, 0x7B30, 0x7B58, 0x7B80]),
        weapon_pins=[], extra_assets=[],
        effect_descs=["dEFManagerPurinSingEffectDesc"],
        effect_files=[("PurinSpecial2", "llPurinSpecial2FileID",
                       "dEFManagerPurinSingEffectDesc: Sing's note ring (efmanager.c:824)")],
        entry_statuses=("nFTPurinStatusAppearR", "nFTPurinStatusAppearL"),
        entry_effect=None,
        entry_note=("BattleShip ftcommonentry.c:23,226-229. Jigglypuff shares "
                    "Pikachu's Master Ball entry: the ball itself "
                    "(efManagerMBallThrownMakeEffect) draws from ITCommonData, "
                    "not linked until P2-5, so it is the same recorded delta; "
                    "the rays still spawn from the script's flag1."),
        stubs=["ftCommonSleepProcUpdate"], prev="ness",
    ),
    "kirby": dict(
        Name="Kirby", kind=8, token="KIRBY", owner_slot=11, image_slot=9,
        css_portrait="llMNPlayersPortraitsKirbySprite",
        css_emblem="llFTEmblemSpritesKirbySprite",
        css_name="llMNPlayersCommonKirbyTextSprite",
        model="KirbyModel", model_file_id=0x148, main_file_id=0xe5,
        attr_offset=0x808, selected_file=418,
        tus=["battleship_kirby.c", "battleship_kirby_copy.c",
             "battleship_kirby_weapons.c", "battleship_ftcommon_capturekirby.c"],
        # 229_KirbyMain.c dKirbyMain_stock_luts: palette_0x1D510 then
        # gap_0x1D530_sub_0x8/0x30/0x58/0x80; llKirbyModelStockSprite = 0x1d5e0.
        stock=dict(sprite=0x1D5E0, texture=0x1D4B8,
                   palettes=[0x1D510, 0x1D538, 0x1D560, 0x1D588, 0x1D5B0]),
        # 229_KirbyMain.c @0x08 Final Cutter beam (attack 0, map 220/0/-220/50).
        weapon_pins=[
            ("KIRBY_MAIN_CUTTER", "KIRBY_MAIN", 0x08, 0, 0, 220, 0, -220, 50),
        ],
        extra_assets=[],
        effect_descs=["dEFManagerVulcanJabEffectDesc",
                      "dEFManagerKirbyCutterUpEffectDesc",
                      "dEFManagerKirbyCutterDownEffectDesc",
                      "dEFManagerKirbyCutterDrawEffectDesc",
                      "dEFManagerKirbyCutterTrailEffectDesc",
                      "dEFManagerKirbyEntryStarEffectDesc"],
        effect_files=[("KirbySpecial2", "llKirbySpecial2FileID",
                       "Vulcan Jab, the four Final Cutter descs and the entry star "
                       "(efmanager.c:704,896..986,1226)")],
        entry_statuses=("nFTKirbyStatusAppearR", "nFTKirbyStatusAppearL"),
        entry_effect="efManagerKirbyEntryStarMakeEffect(&fp->entry_pos, fp->status_vars.common.entry.lr);",
        entry_effect_decl="GObj *efManagerKirbyEntryStarMakeEffect(Vec3f *pos, s32 lr);",
        entry_note="BattleShip ftcommonentry.c:21,222-224. Kirby rides in on the warp star from KirbySpecial2.",
        stubs=["ftCommonCaptureKirbyProcPhysics", "ftCommonCaptureWaitKirbyProcInterrupt",
               "ftCommonCaptureWaitKirbyProcMap", "ftCommonThrownKirbyStarProcUpdate",
               "ftCommonThrownKirbyStarProcPhysics",
               "ftCommonThrownCopyStarProcUpdate", "ftCommonThrownCopyStarProcPhysics"],
        prev="purin",
    ),
    # P2-6 variants. A variant kind is never selectable: no CSS portrait,
    # emblem, name or stock icons, no selected demo, no weapon pins. It shares
    # its base kind's special status table verbatim (ftmain.c:78-107 puts the
    # Mario table in the MMario slot and the Donkey table in the GDonkey slot),
    # so it owns no motion/status enums of its own. `entry_statuses` reuses the
    # base pair verbatim (ftcommonentry.c table :26/:39 + effect switch
    # :194-207); the single logical entry is that reused pair. `reuse_owner`
    # selects the model path: True reuses the base owner's native packet
    # (GDonkey reuses DonkeyModel 0x13d, only Main 0xd7 is own), False owns a
    # native packet from its own model file (MMario owns MMarioModel 0x12c).
    # `base` names the fighter whose status tables/owner are shared.
    "gdonkey": dict(
        Name="GDonkey", kind=26, token="GDONKEY", owner_slot=3, image_slot=1,
        variant=True, base="Donkey", base_kind=2, reuse_owner=True,
        model="DonkeyModel", model_file_id=0x13d, main_file_id=0xd7,
        attr_offset=0x3c8,
        tus=["battleship_gdonkey.c"],
        stock=None,
        weapon_pins=[], extra_assets=[],
        effect_descs=[], effect_files=[],
        entry_statuses=("nFTDonkeyStatusAppearR", "nFTDonkeyStatusAppearL"),
        entry_effect="efManagerDonkeyEntryTaruMakeEffect(&fp->entry_pos);",
        entry_effect_decl=None,
        entry_note=("BattleShip ftcommonentry.c:15,204-207. GDonkey reuses the "
                    "Donkey AppearR/AppearL pair and the Special2 barrel entry."),
        stubs=[], prev="kirby",
    ),
    "mmario": dict(
        Name="MMario", kind=13, token="MMARIO", owner_slot=12, image_slot=10,
        variant=True, base="Mario", base_kind=0, reuse_owner=False,
        model="MMarioModel", model_file_id=0x12c, main_file_id=0xce,
        mainmotion_file_id=0xcd, attr_offset=0x2a8,
        tus=["battleship_mmario.c"],
        stock=None,
        weapon_pins=[], extra_assets=[],
        effect_descs=[], effect_files=[],
        entry_statuses=("nFTMarioStatusAppearR", "nFTMarioStatusAppearL"),
        entry_effect="efManagerMarioEntryDokanMakeEffect(&fp->entry_pos, fp->fkind);",
        entry_effect_decl=None,
        entry_note=("BattleShip ftcommonentry.c:26,194-197. MMario reuses the "
                    "Mario AppearR/AppearL pair and the Dokan pipe entry."),
        stubs=[], prev="gdonkey",
    ),
    # P2-6 Fighting Polygon Team. Twelve variant kinds (14-25, nFTKindNStart
    # plus offset, ftdef.h:1110-1123) admitted in kind order. Each owns its
    # Main (file IDs 0xcf,0xd3,0xd6,0xdb,0xdf,0xe3,0xf8,0xed,0xe7,0xf5,0xea,
    # 0xf1) and -- except NLuigi, which reuses NMarioModel (ftdata.c:3156, no
    # llNLuigiModel exists) -- its own low-poly Model (0x12d,0x12f,0x134,
    # 0x135,0x136,0x130,0x137,0x131,0x133,0x132,0x138), and reuses the base
    # MainMotion and ShieldPose with all specials zero (ftdata.c:839-871 and
    # the same rows in every dFTN*Data). NPikachu is the one exception to the
    # zero-specials row: it keeps &llPikachuSpecial2FileID (ftdata.c:6142).
    # Every N Main zeroes specials, catch and voice (207_NMarioMain.c:201-208,
    # 211_NFoxMain.c:215-222, 214_NDonkeyMain.c:198-205,
    # 219_NSamusMain.c:221-228, 223_NLuigiMain.c:201-208,
    # 227_NLinkMain.c:209-216, 248_NYoshiMain.c:205ff,
    # 237_NCaptainMain.c:199-206, 231_NKirbyMain.c:203-210,
    # 245_NPikachuMain.c:208ff, 234_NPurinMain.c:204-211,
    # 241_NNessMain.c:215-222). Status tables are shared verbatim per base
    # (ftmain.c:78-107). Entry is source EntryNull for all twelve
    # (ftcommonentry.c:194-207 carries no N arm), so entry_statuses is None
    # and the admitter adds no entry arm. All twelve share one doc-only TU:
    # the ftn*/ftn*.c data-pointer files already ride
    # battleship_ftchar_data_slots.c.
    "nmario": dict(
        Name="NMario", kind=14, token="NMARIO", owner_slot=13, image_slot=11,
        variant=True, base="Mario", base_kind=0, reuse_owner=False,
        model="NMarioModel", model_file_id=0x12d, main_file_id=0xcf,
        attr_offset=0x298,
        tus=["battleship_ftn_polygons.c"],
        stock=None,
        weapon_pins=[], extra_assets=[],
        effect_descs=[], effect_files=[],
        entry_statuses=None,
        entry_effect=None,
        entry_note=("BattleShip ftcommonentry.c:194-207. Polygon kinds take "
                    "EntryNull (no N arm); the port else already yields it."),
        stubs=[], prev="mmario",
    ),
    "nfox": dict(
        Name="NFox", kind=15, token="NFOX", owner_slot=14, image_slot=12,
        variant=True, base="Fox", base_kind=1, reuse_owner=False,
        model="NFoxModel", model_file_id=0x12f, main_file_id=0xd3,
        attr_offset=0x2a4,
        tus=["battleship_ftn_polygons.c"],
        stock=None,
        weapon_pins=[], extra_assets=[],
        effect_descs=[], effect_files=[],
        entry_statuses=None,
        entry_effect=None,
        entry_note=("BattleShip ftcommonentry.c:194-207. Polygon kinds take "
                    "EntryNull (no N arm); the port else already yields it."),
        stubs=[], prev="nmario",
    ),
    "ndonkey": dict(
        Name="NDonkey", kind=16, token="NDONKEY", owner_slot=15, image_slot=13,
        variant=True, base="Donkey", base_kind=2, reuse_owner=False,
        model="NDonkeyModel", model_file_id=0x134, main_file_id=0xd6,
        attr_offset=0x298,
        tus=["battleship_ftn_polygons.c"],
        stock=None,
        weapon_pins=[], extra_assets=[],
        effect_descs=[], effect_files=[],
        entry_statuses=None,
        entry_effect=None,
        entry_note=("BattleShip ftcommonentry.c:194-207. Polygon kinds take "
                    "EntryNull (no N arm); the port else already yields it."),
        stubs=[], prev="nfox",
    ),
    "nsamus": dict(
        Name="NSamus", kind=17, token="NSAMUS", owner_slot=16, image_slot=14,
        variant=True, base="Samus", base_kind=3, reuse_owner=False,
        model="NSamusModel", model_file_id=0x135, main_file_id=0xdb,
        attr_offset=0x3bc,
        tus=["battleship_ftn_polygons.c"],
        stock=None,
        weapon_pins=[], extra_assets=[],
        effect_descs=[], effect_files=[],
        entry_statuses=None,
        entry_effect=None,
        entry_note=("BattleShip ftcommonentry.c:194-207. Polygon kinds take "
                    "EntryNull (no N arm); the port else already yields it."),
        stubs=[], prev="ndonkey",
    ),
    "nluigi": dict(
        Name="NLuigi", kind=18, token="NLUIGI", owner_slot=13, image_slot=11,
        variant=True, base="Luigi", base_kind=4, reuse_owner=True,
        reuse_from="nmario",
        model="NMarioModel", model_file_id=0x12d, main_file_id=0xdf,
        attr_offset=0x298,
        tus=["battleship_ftn_polygons.c"],
        stock=None,
        weapon_pins=[], extra_assets=[],
        effect_descs=[], effect_files=[],
        entry_statuses=None,
        entry_effect=None,
        entry_note=("BattleShip ftcommonentry.c:194-207. Polygon kinds take "
                    "EntryNull (no N arm); the port else already yields it."),
        stubs=[], prev="nsamus",
    ),
    "nlink": dict(
        Name="NLink", kind=19, token="NLINK", owner_slot=17, image_slot=15,
        variant=True, base="Link", base_kind=5, reuse_owner=False,
        model="NLinkModel", model_file_id=0x136, main_file_id=0xe3,
        attr_offset=0x2d8,
        tus=["battleship_ftn_polygons.c"],
        stock=None,
        weapon_pins=[], extra_assets=[],
        effect_descs=[], effect_files=[],
        entry_statuses=None,
        entry_effect=None,
        entry_note=("BattleShip ftcommonentry.c:194-207. Polygon kinds take "
                    "EntryNull (no N arm); the port else already yields it."),
        stubs=[], prev="nluigi",
    ),
    "nyoshi": dict(
        Name="NYoshi", kind=20, token="NYOSHI", owner_slot=18, image_slot=16,
        variant=True, base="Yoshi", base_kind=6, reuse_owner=False,
        model="NYoshiModel", model_file_id=0x130, main_file_id=0xf8,
        attr_offset=0x2b8,
        tus=["battleship_ftn_polygons.c"],
        stock=None,
        weapon_pins=[], extra_assets=[],
        effect_descs=[], effect_files=[],
        entry_statuses=None,
        entry_effect=None,
        entry_note=("BattleShip ftcommonentry.c:194-207. Polygon kinds take "
                    "EntryNull (no N arm); the port else already yields it."),
        stubs=[], prev="nlink",
    ),
    "ncaptain": dict(
        Name="NCaptain", kind=21, token="NCAPTAIN", owner_slot=19, image_slot=17,
        variant=True, base="Captain", base_kind=7, reuse_owner=False,
        model="NCaptainModel", model_file_id=0x137, main_file_id=0xed,
        attr_offset=0x29c,
        tus=["battleship_ftn_polygons.c"],
        stock=None,
        weapon_pins=[], extra_assets=[],
        effect_descs=[], effect_files=[],
        entry_statuses=None,
        entry_effect=None,
        entry_note=("BattleShip ftcommonentry.c:194-207. Polygon kinds take "
                    "EntryNull (no N arm); the port else already yields it."),
        stubs=[], prev="nyoshi",
    ),
    "nkirby": dict(
        Name="NKirby", kind=22, token="NKIRBY", owner_slot=20, image_slot=18,
        variant=True, base="Kirby", base_kind=8, reuse_owner=False,
        model="NKirbyModel", model_file_id=0x131, main_file_id=0xe7,
        attr_offset=0x2c0,
        tus=["battleship_ftn_polygons.c"],
        stock=None,
        weapon_pins=[], extra_assets=[],
        effect_descs=[], effect_files=[],
        entry_statuses=None,
        entry_effect=None,
        entry_note=("BattleShip ftcommonentry.c:194-207. Polygon kinds take "
                    "EntryNull (no N arm); the port else already yields it."),
        stubs=[], prev="ncaptain",
    ),
    "npikachu": dict(
        Name="NPikachu", kind=23, token="NPIKACHU", owner_slot=21, image_slot=19,
        variant=True, base="Pikachu", base_kind=9, reuse_owner=False,
        model="NPikachuModel", model_file_id=0x133, main_file_id=0xf5,
        attr_offset=0x2a8,
        tus=["battleship_ftn_polygons.c"],
        stock=None,
        weapon_pins=[], extra_assets=[],
        effect_descs=[], effect_files=[],
        entry_statuses=None,
        entry_effect=None,
        entry_note=("BattleShip ftcommonentry.c:194-207. Polygon kinds take "
                    "EntryNull (no N arm); the port else already yields it."),
        stubs=[], prev="nkirby",
    ),
    "npurin": dict(
        Name="NPurin", kind=24, token="NPURIN", owner_slot=22, image_slot=20,
        variant=True, base="Purin", base_kind=10, reuse_owner=False,
        model="NPurinModel", model_file_id=0x132, main_file_id=0xea,
        attr_offset=0x2a0,
        tus=["battleship_ftn_polygons.c"],
        stock=None,
        weapon_pins=[], extra_assets=[],
        effect_descs=[], effect_files=[],
        entry_statuses=None,
        entry_effect=None,
        entry_note=("BattleShip ftcommonentry.c:194-207. Polygon kinds take "
                    "EntryNull (no N arm); the port else already yields it."),
        stubs=[], prev="npikachu",
    ),
    "nness": dict(
        Name="NNess", kind=25, token="NNESS", owner_slot=23, image_slot=21,
        variant=True, base="Ness", base_kind=11, reuse_owner=False,
        model="NNessModel", model_file_id=0x138, main_file_id=0xf1,
        attr_offset=0x2f0,
        tus=["battleship_ftn_polygons.c"],
        stock=None,
        weapon_pins=[], extra_assets=[],
        effect_descs=[], effect_files=[],
        entry_statuses=None,
        entry_effect=None,
        entry_note=("BattleShip ftcommonentry.c:194-207. Polygon kinds take "
                    "EntryNull (no N arm); the port else already yields it."),
        stubs=[], prev="npurin",
    ),
}
CHAIN = ["pikachu", "yoshi", "ness", "purin", "kirby", "gdonkey", "mmario",
         "nmario", "nfox", "ndonkey", "nsamus", "nluigi", "nlink", "nyoshi",
         "ncaptain", "nkirby", "npikachu", "npurin", "nness"]
KIND_NAMES = ["Mario", "Fox", "Donkey", "Samus", "Luigi", "Link", "Yoshi",
              "Captain", "Kirby", "Pikachu", "Purin", "Ness"]
OPT_IN_FLAGS = ["NDS_P2_LINK", "NDS_P2_PIKACHU", "NDS_P2_YOSHI"]  # CSS mask arms carry these conditionally


class Tree:
    """A repo-relative file cache with count-checked replace/insert."""

    def __init__(self, root: Path, dry: bool):
        self.root, self.dry, self.files = root, dry, {}

    def text(self, rel: str) -> str:
        if rel not in self.files:
            self.files[rel] = (self.root / rel).read_text(encoding="utf-8", newline="")
        return self.files[rel]

    def nl(self, rel: str) -> str:
        return "\r\n" if "\r\n" in self.text(rel)[:4000] else "\n"

    def replace(self, rel: str, old: str, new: str, count: int = 1) -> None:
        s = self.text(rel)
        nl = self.nl(rel)
        o, n = old.replace("\n", nl), new.replace("\n", nl)
        if s.count(o) == 0 and s.count(n) >= 1:
            print(f"  = {rel}: already applied ({old.strip()[:50]!r})")
            return
        if s.count(o) != count:
            raise SystemExit(f"{rel}: anchor found {s.count(o)}x, want {count}: {old[:120]!r}")
        self.files[rel] = s.replace(o, n)
        print(f"  + {rel}: {old.strip().splitlines()[0][:60]!r}")

    def insert_after_block(self, rel: str, flag: str, hint: str, block: str) -> None:
        """Insert `block` after the `#if <flag>` ... `#endif` block containing `hint`."""
        s = self.text(rel)
        nl = self.nl(rel)
        if block.replace("\n", nl) in s:
            print(f"  = {rel}: block already present ({hint[:40]!r})")
            return
        lines = s.split(nl)
        starts = [i for i, l in enumerate(lines)
                  if l.startswith(f"#if {flag}") and (l.strip() == f"#if {flag}" or l.startswith(f"#if {flag} "))]
        found = None
        for i in starts:
            depth, j = 0, i
            while j < len(lines):
                t = lines[j].strip()
                if t.startswith("#if"):
                    depth += 1
                elif t.startswith("#endif"):
                    depth -= 1
                    if depth == 0:
                        break
                j += 1
            body = nl.join(lines[i:j + 1])
            if hint in body:
                if found is not None:
                    raise SystemExit(f"{rel}: hint {hint!r} matches two {flag} blocks")
                found = j
        if found is None:
            raise SystemExit(f"{rel}: no `#if {flag}` block containing {hint!r}")
        lines[found + 1:found + 1] = block.rstrip("\n").split("\n")
        self.files[rel] = nl.join(lines)
        print(f"  + {rel}: after #if {flag} block ({hint[:40]!r})")

    def write_new(self, rel: str, content: str) -> None:
        self.files[rel] = content
        print(f"  + {rel}: new file")

    def flush(self) -> None:
        for rel, s in self.files.items():
            path = self.root / rel
            if self.dry:
                continue
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(s, encoding="utf-8", newline="")
        print("dry run, nothing written" if self.dry else f"wrote {len(self.files)} files")


# ---------------------------------------------------------------------------
# Decomp derivations
# ---------------------------------------------------------------------------
def decomp(root: Path) -> Path:
    return root / "decomp/BattleShip-main/decomp/src"


def enum_blocks(root: Path, name: str) -> tuple[str, str]:
    hdr = (decomp(root) / f"ft/ftchar/ft{name.lower()}/ft{name.lower()}.h").read_text(
        encoding="utf-8", errors="replace")

    def grab(kind: str) -> str:
        m = re.search(rf"typedef enum ft{name}{kind}\s*\{{(.*?)\}}\s*ft{name}{kind};", hdr, re.S)
        if not m:
            raise SystemExit(f"{name}: no ft{name}{kind} enum")
        out = []
        for line in m.group(1).splitlines():
            t = line.split("//")[0].strip().rstrip(",")
            if not t or t.startswith("/*") or t.startswith("*"):
                continue
            out.append(f"    {t},")
        return "\n".join(out)
    return grab("Motion"), grab("Status")


def status_callbacks(root: Path, name: str, port_header: str) -> list[str]:
    table = (decomp(root) / f"ft/ftchar/ft{name.lower()}/ft{name.lower()}status.h").read_text(
        encoding="utf-8", errors="replace")
    have = set(re.findall(r"\b(\w+)\s*\(GObj \*fighter_gobj\)", port_header))
    names = []
    for m in re.finditer(rf"\b(ft{name}\w+)\b", table):
        n = m.group(1)
        if n not in names and n not in have and not n.startswith(f"nFT{name}"):
            names.append(n)
    return names


def sound_ordinals(root: Path) -> dict[str, int]:
    lines = (decomp(root) / "gm/gmsound.h").read_text(encoding="utf-8", errors="replace").splitlines()
    ids: dict[str, int] = {}
    val = -1
    in_enum = False
    # The enum is region-conditional in at least one place, and this walk used
    # to ignore preprocessor lines entirely -- so both arms of
    #   #if defined(REGION_US) ... #else ... #endif
    # were processed in order and the LAST one won. That silently gave the
    # non-US value for nSYAudioFGMVoiceEnd, 0x29D instead of 0x2B7, and
    # Jigglypuff is the one fighter whose heavyget_sfx is that terminator. The
    # attribute validator generated from it therefore rejected PurinMain on
    # every boot, which aborted his battle before the first frame (P2-3f50).
    # The port compiles with -DREGION_US, so take that arm.
    skip_depth = 0      # nesting inside a conditional whose arm we are dropping
    us_arm: list[bool] = []   # per open conditional: is this a REGION_US test
    for line in lines:
        if "typedef enum gmFGMVoiceID" in line:
            in_enum, val = True, -1
            continue
        if not in_enum:
            continue
        stripped = line.strip()
        if stripped.startswith("#"):
            directive = stripped[1:].strip()
            if directive.startswith(("if", "ifdef", "ifndef")):
                is_us = "REGION_US" in directive and not directive.startswith("ifndef")
                us_arm.append(is_us)
                if skip_depth == 0 and not is_us:
                    pass          # unknown condition: keep the old behaviour
            elif directive.startswith("else"):
                if us_arm and us_arm[-1] and skip_depth == 0:
                    skip_depth = 1
                elif skip_depth == 1 and us_arm and us_arm[-1]:
                    skip_depth = 0
            elif directive.startswith("endif"):
                if us_arm:
                    us_arm.pop()
                skip_depth = 0
            continue
        if skip_depth:
            continue
        t = line.split("//")[0].strip().rstrip(",")
        if t.startswith("}"):
            break
        if not t.startswith("nSYAudio"):
            continue
        if "=" in t:
            n, v = [x.strip() for x in t.split("=")]
            val = int(v, 0) if v[0].isdigit() else ids[v]
        else:
            n, val = t, val + 1
        ids[n] = val
    return ids


def sound_reach(root: Path, name: str) -> list[str]:
    """Every nSYAudio* name the fighter's motion scripts, TUs, attributes,
    articles and CSS clip reference, plus its own announcer/crowd lines."""
    d = decomp(root)
    texts = []
    reloc = d / "relocData"
    for f in reloc.iterdir():
        if re.match(rf"\d+_{name}Main(Motion)?\.c$", f.name) or \
           re.match(rf"\d+_FT{name}AnimSelected\.c$", f.name) or \
           re.match(rf"\d+_{name}Special\d\.c$", f.name):
            texts.append(f.read_text(encoding="utf-8", errors="replace"))
    for sub in (f"ft/ftchar/ft{name.lower()}", f"wp/wp{name.lower()}"):
        p = d / sub
        if p.is_dir():
            for f in p.glob("*.c"):
                texts.append(f.read_text(encoding="utf-8", errors="replace"))
    for f in (d / "ft/ftcommon").glob(f"ftcommon*{name.lower()}*.c"):
        texts.append(f.read_text(encoding="utf-8", errors="replace"))
    names = set()
    for t in texts:
        names.update(re.findall(r"\bnSYAudio(?:FGM|Voice)\w+", t))
    ords = sound_ordinals(root)
    names.update(n for n in ords if re.match(rf"nSYAudio(FGM|Voice){name}[A-Z]", n))
    for n in (f"nSYAudioVoiceAnnounce{name}", f"nSYAudioVoicePublic{name}"):
        if n in ords:
            names.add(n)
    names.discard("nSYAudioFGMVoiceEnd")
    return sorted(names, key=lambda n: ords.get(n, 1 << 20))


def weapon_var_structs(root: Path, name: str) -> tuple[list[str], list[tuple[str, str]]]:
    vars_h = (decomp(root) / "wp/wpvars.h").read_text(encoding="utf-8", errors="replace")
    types_h = (decomp(root) / "wp/wptypes.h").read_text(encoding="utf-8", errors="replace")
    structs = re.findall(rf"typedef struct _?(wp{name}WeaponVars\w+)\s*\{{(.*?)\}}\s*\1;", vars_h, re.S)
    members = re.findall(rf"^\s*(wp{name}WeaponVars\w+)\s+(\w+);", types_h, re.M)
    return [f"typedef struct {n} {{{body}}} {n};" for n, body in structs], members


def attr_pin_values(root: Path, name: str, ords: dict[str, int]) -> dict[str, object]:
    src = next((decomp(root) / "relocData").glob(f"*_{name}Main.c")).read_text(encoding="utf-8", errors="replace")
    block = src[src.index(f"FTAttributes d{name}Main_attr = {{"):]
    block = block[:block.index("};")]

    def one(field: str) -> str:
        m = re.search(rf"^\s*(.+?),\s*/\* {field} \*/", block, re.M)
        if not m:
            raise SystemExit(f"{name}: attr field {field} not found")
        return m.group(1).strip()

    def val(tok: str) -> int:
        tok = tok.strip()
        return int(tok, 0) if tok[0].isdigit() else ords[tok]
    dead = re.findall(r"nSYAudio\w+|0x[0-9A-Fa-f]+|\d+", one("dead_fgm_ids"))
    smash = re.findall(r"nSYAudio\w+|0x[0-9A-Fa-f]+|\d+", one("smash_sfx"))
    return dict(dead=[val(x) for x in dead], deadup=val(one("deadup_sfx")),
                damage=val(one("damage_sfx")), smash=[val(x) for x in smash],
                vel=val(one("itemthrow_vel_scale")), dmg=val(one("itemthrow_damage_scale")),
                heavyget=val(one("heavyget_sfx")))


# ---------------------------------------------------------------------------
# Patch sites
# ---------------------------------------------------------------------------
def admit(root: Path, key: str, dry: bool) -> None:
    S = SPECS[key]
    P = SPECS[S["prev"]]
    N, U, L = S["Name"], S["token"], key
    PN, PU, PL = P["Name"], P["token"], S["prev"]
    F, PF = f"NDS_P2_{U}", f"NDS_P2_{PU}"
    T = Tree(root, dry)
    ords = sound_ordinals(root)
    is_variant = bool(S.get("variant", False))
    reuse_owner = bool(S.get("reuse_owner", False))
    base_name = str(S.get("base", PN))
    # Reused owners usually name the base fighter (GDonkey reuses Donkey), but
    # NLuigi reuses its twin NMario's packet: status base Luigi, owner base
    # NMario (ftdata.c:3156 points at llNMarioModelFileID; no llNLuigiModel
    # exists). `reuse_from` names the spec whose packet is reused.
    reuse_key = str(S.get("reuse_from", base_name.lower())) if reuse_owner else ""
    reuse_spec = SPECS[reuse_key] if (reuse_owner and reuse_key in SPECS) else None
    reuse_owner_name = str(reuse_spec["Name"]) if reuse_spec is not None else base_name
    reuse_model = str(reuse_spec["model"]) if reuse_spec is not None else str(S["model"])
    reuse_token = str(reuse_spec["token"]) if reuse_spec is not None else base_name.upper()
    # Own-model admissions anchor their generated owner rows on the nearest
    # prev that actually owns a packet (reuse variants add none). GDonkey
    # reuses Donkey, so MMario anchors on Kirby.
    _oak = S["prev"]
    while SPECS[_oak].get("variant") and SPECS[_oak].get("reuse_owner"):
        _oak = SPECS[_oak]["prev"]
    OPN, OPU, OPL = SPECS[_oak]["Name"], SPECS[_oak]["token"], _oak
    OPF = f"NDS_P2_{OPU}"

    # ---- architecture allowlist -------------------------------------------
    # Every selectable fighter brings a decomp status header under
    # include/ft/ftchar/, and check-architecture.ps1 forbids decomp includes
    # outside src/import unless the file is listed. Variants share their base
    # kind's status table verbatim (ftmain.c:78-107) and bring no decomp status
    # header of their own, so they need no allowlist row.
    if not is_variant:
        T.replace("scripts/check-architecture.ps1",
                  f"    'include/ft/ftchar/ft{PL}/ft{PL}status.h',\n",
                  f"    'include/ft/ftchar/ft{PL}/ft{PL}status.h',\n"
                  f"    'include/ft/ftchar/ft{L}/ft{L}status.h',\n")

    # ---- Makefile ----------------------------------------------------------
    T.replace("Makefile", f"NDS_P2_{PU} ?= 0\n",
              f"NDS_P2_{PU} ?= 0\n"
              f"# P2-3 fighter: {N} stays opt-in until his source specials, articles, native\n"
              f"# owner, CSS/audio surfaces and runtime proofs are admitted (admit_fighter.py).\n"
              f"{F} ?= 0\n")
    if not is_variant:
        T.replace("Makefile",
                  f"ifeq ($(NDS_P2_{PU}),1)\nNDS_P2_FIGHTER_RELOC_FILES += $(NDS_P2_{PU}_FIGHTER_RELOC_FILES)\nendif\n",
                  f"ifeq ($(NDS_P2_{PU}),1)\nNDS_P2_FIGHTER_RELOC_FILES += $(NDS_P2_{PU}_FIGHTER_RELOC_FILES)\nendif\n"
                  f"ifeq ($({F}),1)\nNDS_P2_FIGHTER_RELOC_FILES += $({F}_FIGHTER_RELOC_FILES)\nendif\n")
    else:
        # Variants stage only their own Main (+ own MainMotion/Model when they
        # own one); reused base files already ride the base fighter's list.
        # The per-variant reloc list lives with the production manifest; until
        # it does, this aggregation expands empty and the missing Main is the
        # first thing a link/run reveals (reported at admission).
        T.replace("Makefile",
                  f"ifeq ($(NDS_P2_{PU}),1)\nNDS_P2_FIGHTER_RELOC_FILES += $(NDS_P2_{PU}_FIGHTER_RELOC_FILES)\nendif\n",
                  f"ifeq ($(NDS_P2_{PU}),1)\nNDS_P2_FIGHTER_RELOC_FILES += $(NDS_P2_{PU}_FIGHTER_RELOC_FILES)\nendif\n"
                  f"ifeq ($({F}),1)\nNDS_P2_FIGHTER_RELOC_FILES += $({F}_FIGHTER_RELOC_FILES)\nendif\n")
    if (not is_variant) or (not reuse_owner):
        T.replace("Makefile",
                  f"NDS_NATIVE_OWNER_IMAGE_{OPU} = $(if $(filter 1,$(NDS_NATIVE_OWNER_IMAGE)),$(NDS_P2_{OPU}),0)\n",
                  f"NDS_NATIVE_OWNER_IMAGE_{OPU} = $(if $(filter 1,$(NDS_NATIVE_OWNER_IMAGE)),$(NDS_P2_{OPU}),0)\n"
                  f"NDS_NATIVE_OWNER_IMAGE_{U} = $(if $(filter 1,$(NDS_NATIVE_OWNER_IMAGE)),$({F}),0)\n")
        T.replace("Makefile",
                  f"ifeq ($(NDS_P2_{OPU}),1)\nNDS_NATIVE_IMAGE_OWNERS += {OPL}\nendif\n",
                  f"ifeq ($(NDS_P2_{OPU}),1)\nNDS_NATIVE_IMAGE_OWNERS += {OPL}\nendif\n"
                  f"ifeq ($({F}),1)\nNDS_NATIVE_IMAGE_OWNERS += {L}\nendif\n")
    T.replace("Makefile",
              f"\t\techo '#define NDS_P2_{PU} $(NDS_P2_{PU})'; \\\n",
              f"\t\techo '#define NDS_P2_{PU} $(NDS_P2_{PU})'; \\\n"
              f"\t\techo '#define {F} $({F})'; \\\n")
    if (not is_variant) or (not reuse_owner):
        T.replace("Makefile",
                  f"\t\techo '#define NDS_NATIVE_OWNER_IMAGE_{OPU} $(NDS_NATIVE_OWNER_IMAGE_{OPU})'; \\\n",
                  f"\t\techo '#define NDS_NATIVE_OWNER_IMAGE_{OPU} $(NDS_NATIVE_OWNER_IMAGE_{OPU})'; \\\n"
                  f"\t\techo '#define NDS_NATIVE_OWNER_IMAGE_{U} $(NDS_NATIVE_OWNER_IMAGE_{U})'; \\\n")
    tus = " ".join(S["tus"])
    prev_cfiles = re.search(rf"ifeq \(\$\(NDS_P2_{PU}\),1\)\n(?:#[^\n]*\n)*CFILES \+= [^\n]*(?:\\\n[^\n]*)*\nendif\n",
                            T.text("Makefile").replace("\r\n", "\n"))
    if not prev_cfiles:
        raise SystemExit(f"Makefile: no CFILES block for {PU}")
    T.replace("Makefile", prev_cfiles.group(0),
              prev_cfiles.group(0) + f"ifeq ($({F}),1)\n# BattleShip owns {N}'s specials and articles verbatim (admit_fighter.py).\nCFILES += {tus}\nendif\n")

    # ---- reloc_backend_assets.c -------------------------------------------
    rb = "src/port/reloc_backend_assets.c"
    pins = "".join(
        f"#define NDS_RELOC_SYMBOL_{d}_WEAPON_ATTRIBUTES 0x{off:x}u\n"
        for d, _a, off, *_ in S.get("weapon_pins", []))
    extra = "".join(f"#define NDS_RELOC_ASSET_{a} 0x{i:x}u\n" for a, i in S.get("extra_assets", []))
    T.insert_after_block(rb, PF, f"#define NDS_RELOC_ASSET_{PU}_MAIN ", f"""#if {F}
/* fighter_production_manifest.json: dFT{N}Data field 24 puts his FTAttributes at
 * 0x{S['attr_offset']:x}; ll{N}MainFileID is 0x{S['main_file_id']:x} in the US relocation symbol table. */
#define NDS_RELOC_SYMBOL_{U}_MAIN_ATTRIBUTES 0x{S['attr_offset']:x}u
#define NDS_RELOC_ASSET_{U}_MAIN 0x{S['main_file_id']:x}u
{extra}{pins}#endif
""")
    chain_old = T.text(rb).replace("\r\n", "\n")
    m = re.search(r"#if NDS_P2_LUIGI \|\| NDS_P2_DONKEY \|\| NDS_P2_CAPTAIN \|\| NDS_P2_SAMUS \|\| NDS_P2_LINK \|\| NDS_P2_PIKACHU \|\| NDS_P2_YOSHI(?: \|\| NDS_P2_\w+)*\n", chain_old)
    if not m:
        raise SystemExit(f"{rb}: no combined admission condition")
    combined = m.group(0)
    if F not in combined:
        T.replace(rb, combined, combined.rstrip("\n") + f" || {F}\n", chain_old.count(combined))
    # Variants own no generated ANIM/CORE/PAYLOAD/ALLOC rows; they keep only the
    # FTAttributes offset arm (field 24) and the validator below. The rows need
    # NDS_P2_<VARIANT>_* macros the manifest does not emit yet.
    variant_asset_rows = (
        (f"attr_offset = NDS_RELOC_SYMBOL_{PU}_MAIN_ATTRIBUTES;", f"#if {F}\n    else if (loaded->asset_id == NDS_RELOC_ASSET_{U}_MAIN)\n    {{\n        attr_offset = NDS_RELOC_SYMBOL_{U}_MAIN_ATTRIBUTES;\n    }}\n#endif\n"),
    )
    full_asset_rows = (
        (f"(asset_id >= NDS_P2_{PU}_ANIM_FIRST) &&", f"#if {F}\n    if ((asset_id >= NDS_P2_{U}_ANIM_FIRST) &&\n        (asset_id <= NDS_P2_{U}_ANIM_LAST))\n    {{\n        return TRUE;\n    }}\n#endif\n"),
        (f"NDS_P2_{PU}_ANIM_ASSET_ROWS(NDS_P2_FIGHTER_ANIM_TOKEN_ROW)", f"#if {F}\n    NDS_P2_{U}_ANIM_ASSET_ROWS(NDS_P2_FIGHTER_ANIM_TOKEN_ROW)\n#endif\n"),
        (f"(token >= NDS_P2_{PU}_ANIM_FIRST)", f"#if {F}\n    if ((token >= NDS_P2_{U}_ANIM_FIRST) &&\n        (token <= NDS_P2_{U}_ANIM_LAST))\n    {{\n        return token;\n    }}\n#endif\n"),
        (f"NDS_P2_{PU}_CORE_ASSET_ROWS(NDS_P2_FIGHTER_TOKEN_ROW)", f"#if {F}\n    NDS_P2_{U}_CORE_ASSET_ROWS(NDS_P2_FIGHTER_TOKEN_ROW)\n    NDS_P2_{U}_DEPENDENCY_ASSET_ROWS(NDS_P2_FIGHTER_DEPENDENCY_TOKEN_ROW)\n#endif\n"),
        (f"NDS_P2_{PU}_CORE_ASSET_ROWS(NDS_P2_FIGHTER_ASSET_TEST)", f"#if {F}\n    NDS_P2_{U}_CORE_ASSET_ROWS(NDS_P2_FIGHTER_ASSET_TEST)\n    NDS_P2_{U}_DEPENDENCY_ASSET_ROWS(NDS_P2_FIGHTER_DEPENDENCY_TEST)\n#endif\n"),
        (f"NDS_P2_{PU}_AOBJ32_ASSET_ROWS(NDS_P2_FIGHTER_AOBJ32_TEST)", f"#if {F}\n    NDS_P2_{U}_AOBJ32_ASSET_ROWS(NDS_P2_FIGHTER_AOBJ32_TEST)\n#endif\n"),
        (f"attr_offset = NDS_RELOC_SYMBOL_{PU}_MAIN_ATTRIBUTES;", f"#if {F}\n    else if (loaded->asset_id == NDS_RELOC_ASSET_{U}_MAIN)\n    {{\n        attr_offset = NDS_RELOC_SYMBOL_{U}_MAIN_ATTRIBUTES;\n    }}\n#endif\n"),
        (f"sNdsP2{PN}PayloadSizes[] =", f"#if {F}\nstatic const NDSP2FighterPayloadSizeRow sNdsP2{N}PayloadSizes[] =\n{{\n    NDS_P2_{U}_PAYLOAD_SIZE_ROWS(NDS_P2_PAYLOAD_SIZE_ROW)\n}};\n#endif\n"),
        (f"        sNdsP2{PN}PayloadSizes,", f"#if {F}\n    size = ndsRelocP2FindGeneratedPayloadSize(\n        sNdsP2{N}PayloadSizes,\n        sizeof(sNdsP2{N}PayloadSizes) / sizeof(sNdsP2{N}PayloadSizes[0]),\n        asset_id);\n    if (size != 0u) return size;\n#endif\n"),
        (f"sNdsP2{PN}AllocSizes[] =", f"#if {F}\nstatic const NDSP2FighterAllocSizeRow sNdsP2{N}AllocSizes[] =\n{{\n    NDS_P2_{U}_ALLOC_SIZE_ROWS(NDS_P2_ALLOC_SIZE_ROW)\n}};\n#endif\n"),
        (f"        sNdsP2{PN}AllocSizes,", f"#if {F}\n    size = ndsRelocP2FindGeneratedAllocSize(\n        sNdsP2{N}AllocSizes,\n        sizeof(sNdsP2{N}AllocSizes) / sizeof(sNdsP2{N}AllocSizes[0]),\n        asset_id);\n    if (size != 0u)\n    {{\n        return size;\n    }}\n#endif\n"),
    )
    for hint, block in (variant_asset_rows if is_variant else full_asset_rows):
        T.insert_after_block(rb, PF, hint, block)
    a = attr_pin_values(root, N, ords)
    T.insert_after_block(rb, PF, f"if (asset_id == NDS_RELOC_ASSET_{PU}_MAIN)", f"""#if {F}
    if (asset_id == NDS_RELOC_ASSET_{U}_MAIN)
    {{
        /* {N}Main.c d{N}Main_attr dead_fgm_ids..heavyget_sfx, BattleShip
         * gmsound.h (REGION_US) ordinals, derived by admit_fighter.py. */
        return
            (attr->dead_fgm_ids[0] == {a['dead'][0]}u) &&
            (attr->dead_fgm_ids[1] == {a['dead'][1]}u) &&
            (attr->deadup_sfx == {a['deadup']}u) &&
            (attr->damage_sfx == {a['damage']}u) &&
            (attr->smash_sfx[0] == {a['smash'][0]}u) &&
            (attr->smash_sfx[1] == {a['smash'][1]}u) &&
            (attr->smash_sfx[2] == {a['smash'][2]}u) &&
            (attr->itemthrow_vel_scale == 0x{a['vel']:x}u) &&
            (attr->itemthrow_damage_scale == 0x{a['dmg']:x}u) &&
            (attr->heavyget_sfx == {a['heavyget']}u);
    }}
#endif
""")
    if S.get("weapon_pins"):
        guard_old = re.search(r"#if NDS_P2_PIKACHU \|\| NDS_P2_YOSHI(?: \|\| NDS_P2_\w+)*\n", T.text(rb).replace("\r\n", "\n")).group(0)
        if F not in guard_old:
            T.replace(rb, guard_old, guard_old.rstrip("\n") + f" || {F}\n", 2)
        by_asset: dict[str, list] = {}
        for d, asset, off, a0, a1, top, cen, bot, wid in S["weapon_pins"]:
            by_asset.setdefault(asset, []).append((d, a0, a1, top, cen, bot, wid))
        arms = ""
        for asset, rows in by_asset.items():
            conds = " ||\n             ".join(
                f"ndsRelocNormalizePikachuWeaponAttributes(\n                 loaded,\n                 NDS_RELOC_SYMBOL_{d}_WEAPON_ATTRIBUTES,\n                 {a0}, {a1}, {top}, {cen}, {bot}, {wid}) == FALSE"
                for d, a0, a1, top, cen, bot, wid in rows)
            arms += f"""#if {F}
        /* {N}: source WPAttributes overlapping the file-handle words, normalized
         * and pinned before the file-wide flag is set (relocData initializers,
         * admit_fighter.py). */
        if ((loaded->asset_id == NDS_RELOC_ASSET_{asset}) &&
            ({conds}))
        {{
            ndsRelocRecordExternalFixupFail(loaded->asset_id);
            return FALSE;
        }}
#endif
"""
        T.insert_after_block(rb, "NDS_P2_YOSHI", "NDS_RELOC_SYMBOL_YOSHI_MAIN_STAR_WEAPON_ATTRIBUTES,\n                 0, 0, 100, 0, -100, 96", arms)

    # ---- nds_reloc_assets.c ------------------------------------------------
    # Variants own no generated CORE_ASSET rows (the production manifest tracks
    # only the 12 selectable fighters); their Main files ride the reloc loader
    # through the FTData file IDs already live in reloc_backend_ftdata_symbols.c
    # (llGDonkeyMain 0xd7, llMMarioMain 0xce/MainMotion 0xcd/Model 0x12c).
    # Skipping here is deliberate: adding the rows is the first thing a
    # manifest extension reveals, reported at admission.
    if not is_variant:
        ra = "src/nds/nds_reloc_assets.c"
        txt = T.text(ra).replace("\r\n", "\n")
        m = re.search(r"#if NDS_P2_LUIGI \|\| NDS_P2_DONKEY \|\| NDS_P2_CAPTAIN \|\| NDS_P2_SAMUS \|\| NDS_P2_LINK \|\| NDS_P2_PIKACHU \|\| NDS_P2_YOSHI(?: \|\| NDS_P2_\w+)*\n", txt)
        if m and F not in m.group(0):
            T.replace(ra, m.group(0), m.group(0).rstrip("\n") + f" || {F}\n", txt.count(m.group(0)))
        T.insert_after_block(ra, PF, f"NDS_P2_{PU}_CORE_ASSET_ROWS(NDS_P2_FIGHTER_ASSET_ENTRY)",
                             f"#if {F}\n    NDS_P2_{U}_CORE_ASSET_ROWS(NDS_P2_FIGHTER_ASSET_ENTRY)\n    NDS_P2_{U}_DEPENDENCY_ASSET_ROWS(NDS_P2_FIGHTER_DEPENDENCY_ENTRY)\n#endif\n")
    # The animation path table is generated: nds_reloc_assets.c walks every
    # enabled fighter's NDS_P2_<X>_ANIM_SEGMENTS rows, so admitting a fighter
    # needs no hand-inserted resolver arm. Only the #if list above gates it.

    # ---- fighter.h / callbacks / status header ------------------------------
    if not is_variant:
        fh = "include/ft/fighter.h"
        motion, status = enum_blocks(root, N)
        prev_motion, prev_status = enum_blocks(root, PN)
        prev_motion_last = prev_motion.strip().splitlines()[-1].strip()
        prev_status_last = prev_status.strip().splitlines()[-1].strip()
        T.replace(fh, f"    {prev_motion_last}\n    nFTMarioStatusAttack13 = nFTCommonStatusSpecialStart,\n",
                  f"    {prev_motion_last}\n    /* BattleShip ft{L}.h ft{N}Motion, admit_fighter.py. */\n{motion}\n    nFTMarioStatusAttack13 = nFTCommonStatusSpecialStart,\n")
        T.replace(fh, f"    {prev_status_last}\n    nFTKirbyStatusAttack100Start = nFTCommonStatusSpecialStart,\n",
                  f"    {prev_status_last}\n    /* BattleShip ft{L}.h ft{N}Status, admit_fighter.py. */\n{status}\n    nFTKirbyStatusAttack100Start = nFTCommonStatusSpecialStart,\n")
        cb = "include/ft/ftstatus_callbacks.h"
        names = status_callbacks(root, N, T.text(cb))
        decls = "".join(f"void {n}(GObj *fighter_gobj);\n" for n in names)
        T.replace(cb, f"/* BattleShip ft{PL}status.h callbacks.",
                  f"/* BattleShip ft{L}status.h callbacks. {N}'s source descriptor table is\n * promoted wholesale under {F}; only the names that table references\n * belong here (admit_fighter.py). */\n{decls}/* BattleShip ft{PL}status.h callbacks.")
        T.write_new(f"include/ft/ftchar/ft{L}/ft{L}status.h", f"""#ifndef _FT{U}_STATUS_H_
#if NDS_IMPORT_BATTLESHIP_FTMANAGER && {F}
/* P2-3 {N}. The project fighter header mirrors {N}'s enum and status-var ABI;
 * publish BattleShip's exact table once his runtime is admitted. Keep the
 * compatibility table for builds where {N} is inactive. */
#include <ft/ftstatus_callbacks.h>
#include "../../../../decomp/BattleShip-main/decomp/src/ft/ftchar/ft{L}/ft{L}status.h"
#else
#define _FT{U}_STATUS_H_

#include <ft/ftchar/ftcompat_status.h>

FTStatusDesc dFT{N}SpecialStatusDescs[] = {{
    NDS_FT_STATUS_STUB16
}};

#endif

#endif
""")
    else:
        # Variants share the base table verbatim (ftmain.c:78-107): the Mario
        # table sits in the MMario slot, the Donkey table in the GDonkey slot.
        # No motion/status enums, no callbacks, no new descriptor table. This
        # header only documents the sharing for builds that include the kind.
        T.write_new(f"include/ft/ftchar/ft{L}/ft{L}status.h", f"""#ifndef _FT{U}_STATUS_H_
#define _FT{U}_STATUS_H_
/* P2-6 variant {N}: shares {base_name}'s special status table verbatim
 * (BattleShip ftmain.c:78-107 dFTMainSpecialStatusDescs). No motion/status
 * enums of its own; the kind slot reuses dFT{base_name}SpecialStatusDescs. */
#include <ft/ftchar/ft{base_name.lower()}/ft{base_name.lower()}status.h>
#endif
""")

    # ---- gmsound.h ----------------------------------------------------------
    gm = "include/gm/gmsound.h"
    port_gm = T.text(gm)
    missing = [n for n in sound_reach(root, N) if re.search(rf"\b{n}\b", port_gm) is None]
    if missing:
        ids = "".join(f"    {n} = {ords[n]},\n" for n in missing)
        T.replace(gm, "} gmFGMID;\n",
                  f"    /* P2-3 {N}: every id his sources reach that the port lacked (BattleShip\n     * gmsound.h REGION_US ordinals, admit_fighter.py). */\n{ids}}} gmFGMID;\n")
        # the previous last member needs a trailing comma
        s = T.text(gm)
        nl = T.nl(gm)
        s = re.sub(r"(\w+ = \d+)(\r?\n    /\* P2-3 " + N + ":)", r"\1," + r"\2", s, count=1)
        T.files[gm] = s

    # ---- weapon.h -----------------------------------------------------------
    # Variants own no wp/ weapon vars (MMario reuses Mario specials, GDonkey
    # reuses Donkey Special2); skip the struct promotion entirely.
    if not is_variant:
        structs, members = weapon_var_structs(root, N)
        wh = "include/wp/weapon.h"
        for typedef, (tname, mname) in zip(structs, members):
            if re.search(rf"\b{tname}\b", T.text(wh)) or re.search(rf"\}} {mname};", T.text(wh)):
                continue
            T.replace(wh, "typedef struct wpYoshiWeaponVarsEggThrow {",
                      f"/* BattleShip wpvars.h, admit_fighter.py. */\n{typedef}\n\ntypedef struct wpYoshiWeaponVarsEggThrow {{")
            T.replace(wh, "        wpYoshiWeaponVarsEggThrow egg_throw;\n",
                      f"        wpYoshiWeaponVarsEggThrow egg_throw;\n        {tname} {mname};\n")

    # ---- efmanager ----------------------------------------------------------
    ef = "src/import/battleship_efmanager.c"
    if S["effect_descs"]:
        # Anchor on the last fighter in the chain that carries effect descs
        # (a fighter without any, like Ness, leaves no efmanager block).
        ek = S["prev"]
        while ek != "yoshi" and not SPECS[ek].get("effect_descs"):
            ek = SPECS[ek]["prev"]
        EN, EU, EF = SPECS[ek]["Name"], SPECS[ek]["token"], f"NDS_P2_{SPECS[ek]['token']}"
        hooks = "".join(f"""    if (file_head == &gFTData{sym})
    {{
        /* {note}. */
        return ndsRelocGetLoadedFileSize(&{fid});
    }}
""" for sym, fid, note in S["effect_files"])
        T.insert_after_block(ef, EF, f"if (file_head == &gFTData{EN}", f"#if {F}\n{hooks}#endif\n")
        xs = " \\\n".join(f"    X({d})" for d in S["effect_descs"])
        T.replace(ef, f"#else\n#define NDS_EF_ROSTER_DESCS_{EU}(X)\n#endif\n",
                  f"#else\n#define NDS_EF_ROSTER_DESCS_{EU}(X)\n#endif\n#if {F}\n/* {N}'s fighter-file descs carry &ll{N}* linker symbols (admit_fighter.py). */\n#define NDS_EF_ROSTER_DESCS_{U}(X) \\\n{xs}\n#else\n#define NDS_EF_ROSTER_DESCS_{U}(X)\n#endif\n")
        T.replace(ef, f"    NDS_EF_ROSTER_DESCS_{EU}(X)\n", f"    NDS_EF_ROSTER_DESCS_{EU}(X) \\\n    NDS_EF_ROSTER_DESCS_{U}(X)\n")
        cur = re.search(r"#define NDS_EF_DEFERRED_MAX (\d+)u", T.text(ef)).group(1)
        T.replace(ef, f"#define NDS_EF_DEFERRED_MAX {cur}u", f"#define NDS_EF_DEFERRED_MAX {int(cur) + len(S['effect_descs'])}u")

    # ---- entry seam ---------------------------------------------------------
    # Polygon variants take source EntryNull (ftcommonentry.c:194-207 carries
    # no N arm; the port else already yields it), so entry_statuses None adds
    # no arm. GDonkey/MMario reuse their base pair verbatim.
    en = "src/import/battleship_ftcommon_entry.c"
    if S.get("entry_effect_decl"):
        T.replace(en, f"#if NDS_P2_YOSHI\nGObj *efManagerYoshiEntryEggMakeEffect(Vec3f *pos);\n#endif\n",
                  f"#if NDS_P2_YOSHI\nGObj *efManagerYoshiEntryEggMakeEffect(Vec3f *pos);\n#endif\n#if {F}\n{S['entry_effect_decl']}\n#endif\n")
    if S.get("entry_statuses") is not None:
        r_, l_ = S["entry_statuses"]
        eff = f"        {S['entry_effect']}\n" if S.get("entry_effect") else ""
        T.insert_after_block(en, PF, f"fp->fkind == nFTKind{PN}", f"""#if {F}
    else if (fp->fkind == nFTKind{N})
    {{
        /* {S['entry_note']} */
        status_id = (entry_id == 0) ? {r_} : {l_};
{eff}    }}
#endif
""")

    # ---- mnplayersvs --------------------------------------------------------
    # Variants are never on the CSS: stage only the loader arm. Preview and
    # image arms belong to selectable fighters.
    mv = "src/import/battleship_mnplayersvs.c"
    T.insert_after_block(mv, PF, f"ftManagerSetupFilesAllKind(nFTKind{PN});", f"#if {F}\n    ftManagerSetupFilesAllKind(nFTKind{N});\n#endif\n")
    if not is_variant:
        T.insert_after_block(mv, PF, f"&& (fkind != nFTKind{PN})", f"#if {F}\n        && (fkind != nFTKind{N})\n#endif\n")
        T.insert_after_block(mv, PF, f"ndsMNPlayersVSPreviewPrepareResidentKind(nFTKind{PN});", f"#if {F}\n    (void)ndsMNPlayersVSPreviewPrepareResidentKind(nFTKind{N});\n#endif\n")
        T.insert_after_block(mv, PF, f"NDS_NATIVE_IMAGE_SLOT_{PU}, 1u) == FALSE))", f"""#if {F}
    if (fkind == nFTKind{N})
    {{
        if ((ndsRendererNativeEnsureOwnerImage(
                 NDS_NATIVE_IMAGE_SLOT_{U}, 0u) == FALSE) ||
            (ndsRendererNativeEnsureOwnerImage(
                 NDS_NATIVE_IMAGE_SLOT_{U}, 1u) == FALSE))
        {{
            gNdsPlayersVSPreviewResidentOwnerFailMask |= kind_bit;
            return FALSE;
        }}
        gNdsPlayersVSPreviewResidentReadyMask |= kind_bit;
        return TRUE;
    }}
#endif
""")

    # ---- nds_match_config ---------------------------------------------------
    mc = "src/port/nds_match_config.c"
    T.insert_after_block(mc, f"NDS_P2_PROOF_FIGHTER0 == {P['kind']} && !{PF}", f'#error "{PN} proof fighter requires {PF}=1"', f"""#if NDS_P2_PROOF_FIGHTER0 == {S['kind']} && !{F}
#error "{N} proof fighter requires {F}=1"
#endif
""")
    adm = re.search(r"#define NDS_P2_KIND_ADMITTED\(k\) \\\n(?:.*\\\n)*.*\)\)\n", T.text(mc).replace("\r\n", "\n")).group(0)
    if F not in adm:
        T.replace(mc, adm, adm.rstrip("\n").rstrip(")") + f" || \\\n     ((k) == {S['kind']} && {F}))\n")

    # ---- inactive stubs -----------------------------------------------------
    st = "src/import/battleship_ftstatus_inactive_stubs.c"
    for stub in S.get("stubs", []):
        line = f"NDS_INACTIVE_STATUS_STUB({stub})\n"
        if f"#if !{F}\n{line}" in T.text(st).replace("\r\n", "\n"):
            continue
        T.replace(st, line, f"#if !{F}\n{line}#endif\n")

    # ---- shell: HUD ---------------------------------------------------------
    # Variants are never selectable: no CSS portrait/emblem/name, no stock
    # icons. Skip the whole HUD block (S["stock"] is None).
    if not is_variant:
        hud_py = "scripts/menus/generate_battle_hud.py"
        st_ = S["stock"]
        pals = ", ".join(f"0x{p:X}" for p in st_["palettes"])
        T.replace(hud_py, f'    "{P["css_portrait"]}",\n]\n', f'    "{P["css_portrait"]}",\n    "{S["css_portrait"]}",\n]\n')
        T.replace(hud_py, f'    "{PU}": {{\n', f'    # {N}: stock texture 88 bytes before the first LUT, LUTs from d{N}Main_stock_luts,\n    # sprite ll{N}ModelStockSprite (admit_fighter.py).\n    "{U}": {{\n        "file": "{S["model"]}",\n        "sprite": 0x{st_["sprite"]:X},\n        "texture": 0x{st_["texture"]:X},\n        "palettes": [{pals}],\n    }},\n    "{PU}": {{\n')
        T.replace(hud_py, f'    {PL}_gfx, {PL}_palettes = stock_asset(', f'    {L}_gfx, {L}_palettes = stock_asset(ui, repo_root, MODEL_STOCK["{U}"])\n    {PL}_gfx, {PL}_palettes = stock_asset(')
        gfx_list = re.search(r"c_array_u8\(\"kNdsBattleHudStockGfx\", \[\n(.*?)\]\)", T.text(hud_py).replace("\r\n", "\n"), re.S).group(1)
        if f"{L}_gfx" not in gfx_list:
            T.replace(hud_py, gfx_list, gfx_list.rstrip("\n").rstrip() + f", {L}_gfx\n")
        T.replace(hud_py, f'    lines += c_array_u16("kNdsBattleHud{PN}StockPalette", {PL}_palettes)\n    lines += [""]\n',
                  f'    lines += c_array_u16("kNdsBattleHud{PN}StockPalette", {PL}_palettes)\n    lines += [""]\n    lines += c_array_u16("kNdsBattleHud{N}StockPalette", {L}_palettes)\n    lines += [""]\n')
        hud_c = "src/nds/nds_battle_hud.c"
        T.replace(hud_c, f"        source = kNdsBattleHud{PN}StockPalette[costume];\n    }}\n    else\n    {{\n",
                  f"        source = kNdsBattleHud{PN}StockPalette[costume];\n    }}\n    else if (fkind == (u32)nFTKind{N})\n    {{\n        /* {N}Main.c stock_luts names {len(st_['palettes'])} source LUTs. */\n        if (costume >= {len(st_['palettes'])}u) costume = 0u;\n        source = kNdsBattleHud{N}StockPalette[costume];\n    }}\n    else\n    {{\n")
        T.replace(hud_c, f"    else if (fkind == (u32)nFTKind{PN}) owner = {P['owner_slot']}u;\n    else return;\n",
                  f"    else if (fkind == (u32)nFTKind{PN}) owner = {P['owner_slot']}u;\n    else if (fkind == (u32)nFTKind{N}) owner = {S['owner_slot']}u;\n    else return;\n", 2)

    # ---- shell: CSS bake ----------------------------------------------------
    # Variants are never selectable: no CSS slot, no gate, no selected demo.
    if not is_variant:
        kit = "scripts/menus/generate_mn_ui_kit.py"
        kt = T.text(kit)
        for var in ("CSS_BUILT_FKIND", "CSS_INPROGRESS_FKIND"):
            m = re.search(rf"{var} = \(([^)]*)\)", kt)
            if str(S["kind"]) not in [x.strip() for x in m.group(1).split(",")]:
                T.replace(kit, m.group(0), f"{var} = ({m.group(1).rstrip()}, {S['kind']})")
        T.replace(kit, f'    {P["kind"]}: "{P["css_portrait"]}",\n}}\n', f'    {P["kind"]}: "{P["css_portrait"]}",\n    {S["kind"]}: "{S["css_portrait"]}",\n}}\n')
        T.replace(kit, f'                     "{P["css_emblem"]}")\n', f'                     "{P["css_emblem"]}",\n                     "{S["css_emblem"]}")\n')
        T.replace(kit, f'                   "{P["css_name"]}")\n', f'                   "{P["css_name"]}",\n                   "{S["css_name"]}")\n')
        tok = re.search(r'CSS_FIGHTER_TOKEN = \(([^)]*)\)', T.text(kit), re.S).group(0)
        if f'"{U}"' not in tok:
            T.replace(kit, tok, tok.rstrip(")").rstrip() + f',\n                     "{U}")')
        aud = "scripts/menus/audit_mn_screen_coverage.py"
        gt = re.search(r'_CSS_GATE_FIGHTERS = \(([^)]*)\)', T.text(aud), re.S).group(0)
        if f'"{U}"' not in gt:
            T.replace(aud, gt, gt.rstrip(")").rstrip() + f', "{U}")')
        al = "scripts/menus/mn_screen_coverage_allowlist.json"
        for pat, sym in ((r'llMNPlayersPortraits\(([^)]*)\)Sprite', S["css_portrait"][len("llMNPlayersPortraits"):-len("Sprite")]),
                         (r'llMNPlayersCommon\(([^)]*)\)TextSprite', S["css_name"][len("llMNPlayersCommon"):-len("TextSprite")]),
                         (r'llFTEmblemSprites\(([^)]*)\)Sprite', S["css_emblem"][len("llFTEmblemSprites"):-len("Sprite")])):
            m = re.search(pat, T.text(al))
            if m:
                alts = m.group(1).split("|")
                if sym in alts:
                    alts.remove(sym)
                    T.replace(al, m.group(0), m.group(0).replace("(" + m.group(1) + ")", "(" + "|".join(alts) + ")"))
        css_c = "src/nds/nds_menu_shell_css.c"
        terms = ["(NDS_P2_LINK ? LBBACKUP_MASK_FIGHTER(nFTKindLink) : 0u)",
                 "(NDS_P2_PIKACHU ? LBBACKUP_MASK_FIGHTER(nFTKindPikachu) : 0u)",
                 "(NDS_P2_YOSHI ? LBBACKUP_MASK_FIGHTER(nFTKindYoshi) : 0u)"]
        for k in CHAIN[CHAIN.index("ness"):CHAIN.index(key)]:
            if SPECS[k].get("variant"):
                continue
            terms.append(f"(NDS_P2_{SPECS[k]['token']} ? LBBACKUP_MASK_FIGHTER(nFTKind{SPECS[k]['Name']}) : 0u)")
        joined = " | \\\n     ".join(terms)
        T.replace(css_c, f"#if {PF}\n/* ", f"""#if {F}
    /* {N} lands behind his own flag; every earlier opt-in fighter rides its own
     * flag in this arm (admit_fighter.py). */
    #define NDS_CSS_FIGHTER_MASK \\
        (LBBACKUP_MASK_FIGHTER(nFTKindMario) | LBBACKUP_MASK_FIGHTER(nFTKindFox) | \\
         LBBACKUP_MASK_FIGHTER(nFTKindLuigi) | \\
         LBBACKUP_MASK_FIGHTER(nFTKindDonkey) | \\
         LBBACKUP_MASK_FIGHTER(nFTKindCaptain) | \\
         LBBACKUP_MASK_FIGHTER(nFTKindSamus) | \\
         {joined} | \\
         LBBACKUP_MASK_FIGHTER(nFTKind{N}))
    #elif {PF}
    /* """)
        gate = int(re.search(r"#define NDS_CSS_GATE_FIGHTERS (\d+)u", T.text(css_c)).group(1))
        T.replace(css_c, f"#define NDS_CSS_GATE_FIGHTERS {gate}u", f"#define NDS_CSS_GATE_FIGHTERS {gate + 1}u")
        T.replace(css_c, f"_Static_assert(NDS_MN_UI_KIT_SURFACE_CSS_GATE_3_HOLD_{PU} ==", f"_Static_assert(NDS_MN_UI_KIT_SURFACE_CSS_GATE_3_HOLD_{U} ==")
        T.replace(css_c, f"_Static_assert(NDS_MN_UI_KIT_SURFACE_CSS_GATE_TEAM_GREEN_3_HOLD_{PU} ==", f"_Static_assert(NDS_MN_UI_KIT_SURFACE_CSS_GATE_TEAM_GREEN_3_HOLD_{U} ==")
        T.insert_after_block(css_c, PF, f"fkind == (u32)nFTKind{PN})\n    {{\n        fighter = ", f"#if {F}\n    else if (fkind == (u32)nFTKind{N})\n    {{\n        fighter = {gate}u;\n    }}\n#endif\n")
        sc = "src/import/battleship_scsubsysdata_ft.c"
        sel = S["selected_file"]
        T.insert_after_block(sc, PF, f"relocData/{P['selected_file']}_FT{PN}AnimSelected.c",
                             f'#if {F}\n/* scsubsysdata{L}.c maps {N}\'s Selected demo to file {sel} with the same\n * demo-only clip contract as the fighters above. */\n#include "../../decomp/BattleShip-main/decomp/src/relocData/{sel}_FT{N}AnimSelected.c"\n#endif\n')
        T.insert_after_block(sc, PF, f"return sizeof(dFT{PN}AnimSelected_joints);",
                             f"#if {F}\n    if (file_id == &llFT{N}AnimSelectedFileID)\n    {{\n        return sizeof(dFT{N}AnimSelected_joints);\n    }}\n#endif\n")
        T.insert_after_block(sc, PF, f"source = dFT{PN}AnimSelected_joints;",
                             f"#if {F}\n    else if (file_id == &llFT{N}AnimSelectedFileID)\n    {{\n        source = dFT{N}AnimSelected_joints;\n        size = sizeof(dFT{N}AnimSelected_joints);\n    }}\n#endif\n")

    # ---- owner generator tables --------------------------------------------
    # Reused owners (GDonkey -> DonkeyModel) need no generator row: the base
    # packet already ships. Own-model variants (MMario -> MMarioModel 0x12c)
    # generate exactly like any fighter, with their own textures; no metal
    # shader is invented (see battleship_mmario.c doc).
    if is_variant and reuse_owner:
        pass
    else:
        og = "scripts/fighters/generate_nds_native_owners.py"
        sha = hashlib.sha256((root / "decomp/BattleShip-main/BattleShip_o2r/reloc_fighters_main" / S["model"]).read_bytes()).hexdigest()
        # P2_RUNTIME_OWNERS drives the full export and needs OWNER_JOINT_TREES /
        # SETUP_PARTS / cross-slot pins the variant does not pin yet (KeyError
        # 'mmario' is the first thing generation reveals). Stage only the O2R
        # identity row here; the runtime row lands with those pins.
        _opm = SPECS[OPL]["model"]
        _opmid = SPECS[OPL]["model_file_id"]
        T.replace(og, f'    "{OPL}": (\n        Path("decomp/BattleShip-main/BattleShip_o2r"\n             "/reloc_fighters_main/{_opm}"),\n        0x{_opmid:04x},\n',
                  f'    "{L}": (\n        Path("decomp/BattleShip-main/BattleShip_o2r"\n             "/reloc_fighters_main/{S["model"]}"),\n        0x{S["model_file_id"]:04x},\n        "{sha}",\n    ),\n    "{OPL}": (\n        Path("decomp/BattleShip-main/BattleShip_o2r"\n             "/reloc_fighters_main/{_opm}"),\n        0x{_opmid:04x},\n')
        oi = "scripts/fighters/generate_nds_native_owner_images.py"
        io = re.search(r'P2_IMAGE_OWNERS = \(([^)]*)\)', T.text(oi), re.S).group(0)
        if f'"{L}"' not in io:
            T.replace(oi, io, io.rstrip(")").rstrip() + f', "{L}")')

    # ---- owner runtime seams ------------------------------------------------
    # Reused owners map the variant kind onto the base packet and stop: no new
    # profile, no new model-id arm, no new runtime tables. Own-model variants
    # take the full selectable path.
    af = "src/port/renderer_adapter_fighter.c"
    if is_variant and reuse_owner:
        T.insert_after_block(af, PF, f"*owner_slot = {P['owner_slot']}u;", f"#if {F}\n    if (fp->fkind == nFTKind{N})\n    {{\n        /* P2-6 variant: {N} reuses the {reuse_owner_name} owner packet verbatim\n         * (BattleShip {reuse_model} 0x{S['model_file_id']:x}, admit_fighter.py). */\n        *owner_slot = {S['owner_slot']}u;\n        return TRUE;\n    }}\n#endif\n")
        fm = "src/import/battleship_ftmanager.c"
    else:
        # Own-model variants anchor generated rows on the nearest packet owner
        # (Kirby for MMario, since GDonkey reuses and adds none).
        T.replace("include/nds/nds_renderer.h", f"#if {OPF}\n    NDS_RENDERER_PROFILE_OWNER_{OPU},\n#endif\n",
                  f"#if {OPF}\n    NDS_RENDERER_PROFILE_OWNER_{OPU},\n#endif\n#if {F}\n    NDS_RENDERER_PROFILE_OWNER_{U},\n#endif\n")
        T.insert_after_block(af, PF, f"*owner_slot = {P['owner_slot']}u;", f"#if {F}\n    if (fp->fkind == nFTKind{N})\n    {{\n        *owner_slot = {S['owner_slot']}u;\n        return TRUE;\n    }}\n#endif\n")
        T.insert_after_block(af, OPF, f"return 0x{SPECS[OPL]['model_file_id']:x}u; /* ll{OPN}ModelFileID", f"#if {F}\n    if (owner_slot == {S['owner_slot']}u)\n    {{\n        return 0x{S['model_file_id']:x}u; /* ll{N}ModelFileID, BattleShip dFT{N}Data */\n    }}\n#endif\n")
        T.insert_after_block(af, OPF, f"return NDS_RENDERER_PROFILE_OWNER_{OPU};", f"#if {F}\n    if (owner_slot == {S['owner_slot']}u)\n    {{\n        return NDS_RENDERER_PROFILE_OWNER_{U};\n    }}\n#endif\n")
        fm = "src/import/battleship_ftmanager.c"
    txt = T.text(fm).replace("\r\n", "\n")
    m = re.search(r"#if NDS_P2_LUIGI \|\| NDS_P2_DONKEY \|\| NDS_P2_CAPTAIN \|\| NDS_P2_SAMUS \|\| NDS_P2_LINK \|\| NDS_P2_PIKACHU \|\| NDS_P2_YOSHI(?: \|\| NDS_P2_\w+)*\n", txt)
    if m and F not in m.group(0):
        T.replace(fm, m.group(0), m.group(0).rstrip("\n") + f" || {F}\n", txt.count(m.group(0)))
    if is_variant and reuse_owner:
        T.insert_after_block(fm, PF, f"image_slot = NDS_NATIVE_IMAGE_SLOT_{PU};", f"#if {F}\n        if (desc->fkind == nFTKind{N})\n        {{\n            /* Reuses the {reuse_owner_name} image packet. */\n            image_slot = NDS_NATIVE_IMAGE_SLOT_{reuse_token};\n        }}\n#endif\n")
    else:
        _fmpf, _fmpu = (OPF, OPU) if is_variant else (PF, PU)
        T.insert_after_block(fm, _fmpf, f"image_slot = NDS_NATIVE_IMAGE_SLOT_{_fmpu};", f"#if {F}\n        if (desc->fkind == nFTKind{N})\n        {{\n            image_slot = NDS_NATIVE_IMAGE_SLOT_{U};\n        }}\n#endif\n")
    rs = "src/nds/nds_renderer_assets.c"
    if is_variant and reuse_owner:
        T.flush()
        return
    txt = T.text(rs).replace("\r\n", "\n")
    m = re.search(r"#if NDS_P2_LUIGI \|\| NDS_P2_DONKEY \|\| NDS_P2_CAPTAIN \|\| NDS_P2_SAMUS \|\| NDS_P2_LINK \|\| NDS_P2_PIKACHU \|\| NDS_P2_YOSHI(?: \|\| NDS_P2_\w+)*\n", txt)
    if m and F not in m.group(0):
        T.replace(rs, m.group(0), m.group(0).rstrip("\n") + f" || {F}\n", txt.count(m.group(0)))
    APF, APU, APN, APL = (OPF, OPU, OPN, OPL) if is_variant else (PF, PU, PN, PL)
    prev_tables = re.search(rf"#if {APF}\n#if NDS_NATIVE_OWNER_IMAGE_{APU}\nstatic NDSNativeFighterRuntimeTables sNdsNative{APN}FighterHighTables;.*?NDS_NATIVE_{APU}_MODEL_DATA_SIZE\);\n#endif\n", txt, re.S)
    if not prev_tables:
        raise SystemExit(f"{rs}: no owner tables block for {APN}")
    block = prev_tables.group(0).replace(APF, F).replace(f"NDS_NATIVE_OWNER_IMAGE_{APU}", f"NDS_NATIVE_OWNER_IMAGE_{U}").replace(f"NDS_NATIVE_{APU}_MODEL", f"NDS_NATIVE_{U}_MODEL").replace(f"sNdsNative{APN}", f"sNdsNative{N}")
    T.replace(rs, prev_tables.group(0), prev_tables.group(0) + "\n" + block)
    for hint, blk in (
        (f'"nitro:/fighters/{APL}_high.bin"', f'#if {F}\n    if (owner_slot == NDS_NATIVE_IMAGE_SLOT_{U})\n    {{\n        return (use_low_detail != 0u) ? "nitro:/fighters/{L}_low.bin" :\n                                        "nitro:/fighters/{L}_high.bin";\n    }}\n#endif\n'),
        (f"(u32)sizeof(NDSNative{APN}HighImage);", f"#if {F}\n    if (owner_slot == NDS_NATIVE_IMAGE_SLOT_{U})\n    {{\n        return (use_low_detail != 0u) ?\n            (u32)sizeof(NDSNative{N}LowImage) :\n            (u32)sizeof(NDSNative{N}HighImage);\n    }}\n#endif\n"),
        (f"&sNdsNative{APN}LowOwner : &sNdsNative{APN}HighOwner;", f"#if {F}\n    if (slot == {S['owner_slot']}u)\n    {{\n        return (use_low_detail != 0u) ?\n            &sNdsNative{N}LowOwner : &sNdsNative{N}HighOwner;\n    }}\n#endif\n"),
    ):
        T.insert_after_block(rs, APF, hint, blk)
    verify_prev = re.search(rf"#if {APF} && !NDS_NATIVE_OWNER_IMAGE_{APU}\n.*?\n#endif\n", txt, re.S).group(0)
    T.replace(rs, verify_prev, verify_prev + verify_prev.replace(APF, F).replace(APU, U).replace(APN, N))
    nc = "src/nds/nds_renderer_native_common.c"
    T.insert_after_block(nc, APF, f"sNdsNative{APN}FighterDenseNormalsLow[", f"#if {F}\nstatic u32 sNdsNative{N}FighterDenseNormals[\n    NDS_NATIVE_IMAGE_{U}_HIGH_DENSE_VERTICES_COUNT];\nstatic u32 sNdsNative{N}FighterDenseNormalsLow[\n    NDS_NATIVE_IMAGE_{U}_LOW_DENSE_VERTICES_COUNT];\n#endif\n")
    T.insert_after_block(nc, APF, f"static u8 sNdsNative{APN}FighterDenseNormalsBuiltLow;", f"#if {F}\nstatic u8 sNdsNative{N}FighterDenseNormalsBuilt;\nstatic u8 sNdsNative{N}FighterDenseNormalsBuiltLow;\n#endif\n")
    _ap_slot = SPECS[APL]["owner_slot"]
    if is_variant:
        # Current file carries the P2-3f49 image branch inside the active
        # mapping, so the old replace anchor no longer matches. Insert the new
        # slot arm after the anchor block instead; same runtime effect.
        T.insert_after_block(nc, APF, "sNdsNativeImageDenseNormalsReady;", f"""#if {F}
    if (slot == {S['owner_slot']}u)
    {{
#if NDS_NATIVE_OWNER_IMAGE_{U}
        sNdsNativeFighterActiveDenseNormals =
            (u32 *)sNdsNativeFighterActiveTables->dense_normals;
        sNdsNativeFighterActiveDenseNormalsBuilt =
            &sNdsNativeImageDenseNormalsReady;
#else
        if (use_low_detail != 0u)
        {{
            sNdsNativeFighterActiveDenseNormals =
                sNdsNative{N}FighterDenseNormalsLow;
            sNdsNativeFighterActiveDenseNormalsBuilt =
                &sNdsNative{N}FighterDenseNormalsBuiltLow;
        }}
        else
        {{
            sNdsNativeFighterActiveDenseNormals =
                sNdsNative{N}FighterDenseNormals;
            sNdsNativeFighterActiveDenseNormalsBuilt =
                &sNdsNative{N}FighterDenseNormalsBuilt;
        }}
#endif
        return TRUE;
    }}
#endif
""")
    else:
        T.replace(nc, f"#if {APF}\n    if (slot == {_ap_slot}u)\n    {{\n        if (use_low_detail != 0u)\n        {{\n            sNdsNativeFighterActiveDenseNormals =\n                sNdsNative{APN}FighterDenseNormalsLow;",
                  f"#if {F}\n    if (slot == {S['owner_slot']}u)\n    {{\n        if (use_low_detail != 0u)\n        {{\n            sNdsNativeFighterActiveDenseNormals =\n                sNdsNative{N}FighterDenseNormalsLow;\n            sNdsNativeFighterActiveDenseNormalsBuilt =\n                &sNdsNative{N}FighterDenseNormalsBuiltLow;\n        }}\n        else\n        {{\n            sNdsNativeFighterActiveDenseNormals =\n                sNdsNative{N}FighterDenseNormals;\n            sNdsNativeFighterActiveDenseNormalsBuilt =\n                &sNdsNative{N}FighterDenseNormalsBuilt;\n        }}\n        return TRUE;\n    }}\n#endif\n#if {APF}\n    if (slot == {_ap_slot}u)\n    {{\n        if (use_low_detail != 0u)\n        {{\n            sNdsNativeFighterActiveDenseNormals =\n                sNdsNative{APN}FighterDenseNormalsLow;")
    for hint, blk in (
        (f"tables->roots = sNdsNative{APN}Roots;", f"#if {F}\n    else if (slot == {S['owner_slot']}u)\n    {{\n        tables->roots = sNdsNative{N}Roots;\n        tables->schedule = sNdsNative{N}JointSchedule;\n        tables->binding_joints = sNdsNative{N}BindingJoints;\n        tables->cross_slots = sNdsNative{N}CrossPaletteSlots;\n        tables->root_count = sizeof(sNdsNative{N}Roots) /\n            sizeof(sNdsNative{N}Roots[0]);\n        tables->joint_count = sizeof(sNdsNative{N}JointSchedule) /\n            sizeof(sNdsNative{N}JointSchedule[0]);\n    }}\n#endif\n"),
        (f"return sNdsNative{APN}BindingParents;", f"#if {F}\n    if (slot == {S['owner_slot']}u)\n    {{\n        *count = (u32)(sizeof(sNdsNative{N}BindingParents) /\n                       sizeof(sNdsNative{N}BindingParents[0]));\n        return sNdsNative{N}BindingParents;\n    }}\n#endif\n"),
        (f"return sNdsNative{APN}CrossPaletteSlots;", f"#if {F}\n    if (slot == {S['owner_slot']}u)\n    {{\n        *count = (u32)(sizeof(sNdsNative{N}CrossPaletteSlots) /\n                       sizeof(sNdsNative{N}CrossPaletteSlots[0]));\n        return sNdsNative{N}CrossPaletteSlots;\n    }}\n#endif\n"),
    ):
        T.insert_after_block(nc, APF, hint, blk)

    T.flush()


def audio_bank(root: Path, key: str, dry: bool) -> None:
    """Add the fighter's bank to the FGM pack renderer, the runtime case table
    and the checker's expected-id list. The selector pin starts as zeros; run
    the renderer, paste the digest it reports, render again, then pin the
    checker/header bytes (the same two-pass flow every landed bank used)."""
    S = SPECS[key]
    N, U, L = S["Name"], S["token"], key
    P = SPECS[S["prev"]]
    PN, PU = P["Name"], P["token"]
    T = Tree(root, dry)
    ords = sound_ordinals(root)
    names = sound_reach(root, N)
    pack = json.loads((root / "assets/audio/fgm_phase_pack_ima.json").read_text(encoding="utf-8"))
    have = {int(e["id"]) for e in pack["entries"]}
    own = [n for n in names if re.match(rf"nSYAudio(FGM|Voice){N}[A-Z]", n) or n in (f"nSYAudioVoiceAnnounce{N}", f"nSYAudioVoicePublic{N}")]
    shared = [n for n in names if n not in own and ords[n] not in have]
    table = own + shared
    # bare-fork roots render their fork target
    spec = importlib_load(root / "scripts/sfx/render-audio-fgm-phase-pack.py", "rfp")
    tools = root / "decomp/BattleShip-main/decomp/tools"
    ef = spec.load_module(tools / "extract_fgm.py", "extract_fgm")
    raw = spec.read_o2r_payload(root / "decomp/BattleShip-main/BattleShip_o2r/audio/fgm_ucd")[1]
    ucd = ef.decode_fgm_ucd(raw)
    programs = {}
    for n in table:
        prog = ucd["entries"][ords[n]]["program"]
        ops = [op[0] for op in prog]
        if "note" not in ops and "fork_voice" in ops:
            programs[ords[n]] = int(next(op[1] for op in prog if op[0] == "fork_voice"))
    rs = "scripts/sfx/render-audio-fgm-phase-pack.py"
    tuple_lines = "".join(f'    ({ords[n]}, "{n}"),\n' for n in table)
    prog_lines = "".join(f"    {k}: {v},   # bare fork -> its voiced program\n" for k, v in programs.items())
    T.replace(rs, f"{PU}_SELECTOR_SHA256 = (\n", f"""# P2-3 {N}'s bank (admit_fighter.py): gmsound.h's complete nSYAudio{{FGM,Voice}}{N}*
# run, his announcer/crowd lines, and the shared cues his motion scripts, TUs
# and attributes reach that no earlier bank packed ({', '.join(str(ords[n]) for n in shared) or 'none'}).
{U}_AUDIO = (
{tuple_lines})
{U}_RENDER_PROGRAMS = {{
{prog_lines}}}
{U}_SELECTOR_SHA256 = (
    "0000000000000000000000000000000000000000000000000000000000000000")

{PU}_SELECTOR_SHA256 = (
""")
    T.replace(rs, f"    *(fgm_id for fgm_id, _name in {PU}_AUDIO),\n)\n",
              f"    *(fgm_id for fgm_id, _name in {PU}_AUDIO),\n    *(fgm_id for fgm_id, _name in {U}_AUDIO),\n)\n")
    T.replace(rs, f"    *(fgm_id for fgm_id, _name in {PU}_AUDIO),\n    18, 365,\n",
              f"    *(fgm_id for fgm_id, _name in {PU}_AUDIO),\n    *(fgm_id for fgm_id, _name in {U}_AUDIO),\n    18, 365,\n")
    T.replace(rs, f'            ("{S["prev"]}", {PU}_AUDIO, {PU}_RENDER_PROGRAMS,\n             {PU}_SELECTOR_SHA256)):\n',
              f'            ("{S["prev"]}", {PU}_AUDIO, {PU}_RENDER_PROGRAMS,\n             {PU}_SELECTOR_SHA256),\n            ("{L}", {U}_AUDIO, {U}_RENDER_PROGRAMS,\n             {U}_SELECTOR_SHA256)):\n')
    fc = "src/nds/nds_audio_fgm.c"
    cases = "".join(f"    case {n}:\n" for n in table)
    T.replace(fc, f"    case nSYAudioVoicePublic{PN}:\n",
              f"    case nSYAudioVoicePublic{PN}:\n    /* P2-3 {N}'s bank (admit_fighter.py). */\n{cases}")
    ck = "scripts/check-audio-fgm-phase-pack.ps1"
    ids = [ords[n] for n in table]
    rows = [", ".join(str(i) for i in ids[k:k + 12]) for k in range(0, len(ids), 12)]
    last = re.search(r"(    [\d, ]+)\)\n\$actualIDs", T.text(ck).replace("\r\n", "\n"))
    if not last:
        raise SystemExit(f"{ck}: expected-id tail not found")
    T.replace(ck, last.group(0), last.group(1) + ",\n    # P2-3 " + N + " bank (admit_fighter.py).\n" + ",\n".join("    " + r for r in rows) + ")\n$actualIDs")
    T.flush()
    print(f"{N}: {len(own)} own + {len(shared)} shared cues; bare forks: {programs}")


def importlib_load(path: Path, name: str):
    spec = importlib.util.spec_from_file_location(name, path)
    mod = importlib.util.module_from_spec(spec)
    sys.modules[name] = mod
    spec.loader.exec_module(mod)
    return mod


def audio_table(root: Path, key: str) -> None:
    """Print the fighter's bank table and bare-fork render programs."""
    S = SPECS[key]
    N = S["Name"]
    ords = sound_ordinals(root)
    names = sound_reach(root, N)
    pack = json.loads((root / "assets/audio/fgm_phase_pack_ima.json").read_text(encoding="utf-8"))
    have = {int(e["id"]) for e in pack["entries"]}
    own = [n for n in names if re.match(rf"nSYAudio(FGM|Voice){N}[A-Z]", n) or n in (f"nSYAudioVoiceAnnounce{N}", f"nSYAudioVoicePublic{N}")]
    shared = [n for n in names if n not in own and ords[n] not in have]
    print(f"{N}: {len(own)} own cues, {len(shared)} shared cues not in the pack")
    for n in own + shared:
        print(f"    ({ords[n]}, \"{n}\"),")
    print("shared:", ", ".join(f"{n}={ords[n]}" for n in shared))


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--repo-root", type=Path, default=Path("."))
    ap.add_argument("--fighter", required=True, choices=[k for k in SPECS if k != "yoshi"])
    ap.add_argument("--dry-run", action="store_true")
    ap.add_argument("--audio-table", action="store_true")
    ap.add_argument("--audio", action="store_true",
                    help="add the fighter's bank to the FGM renderer, runtime table and checker")
    a = ap.parse_args()
    if a.audio_table:
        audio_table(a.repo_root.resolve(), a.fighter)
        return 0
    if a.audio:
        audio_bank(a.repo_root.resolve(), a.fighter, a.dry_run)
        return 0
    admit(a.repo_root.resolve(), a.fighter, a.dry_run)
    return 0


if __name__ == "__main__":
    sys.exit(main())
