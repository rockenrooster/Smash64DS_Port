#!/usr/bin/env python3
"""Host C execution of the real ftManagerMakeFighter image detail gate.

Measured result: this suite extracts ftManagerMakeFighter verbatim from
src/import/battleship_ftmanager.c with source_test_helpers.function,
compiles that exact body on the host with gcc or clang against lightweight
FTDesc, GObj, base factory, ensure and verify recorder, scene data, and enum
stubs, then runs the compiled body. No ROM, no emulator, and no live C edits
take place. All build artifacts live in a temp dir. All logs are capped.

Source references checked here:
  * src/import/battleship_ftmanager.c holds ftManagerMakeFighter.
  * include/ft/fighter.h defines nFTPlayerKindDemo and nFTPartsDetailLow.
  * include/sc/scene.h defines nSCKind1PGamePlayers and nSCKind1PIntro.
  * include/nds/nds_renderer.h declares ndsRendererNativeEnsureOwnerImage
    and ndsRendererNativeVerifyOwnerImage.
  * Makefile carries the NDS_NATIVE_OWNER_IMAGE_VERIFY default of 0.
  * decomp sc1pintro.c:998 selects LOW for some team opponents, while the 1P
    CSS maker preserves the default HIGH detail.

Scope actually proven here (host cap: 2 compiler runs, 2 executions):
  * the compiled body loads only detail 0 for CSS HIGH Demo, only detail 1
    for intro LOW Demo, and only detail 0 for intro HIGH Demo.
  * the compiled body loads both details for battle human, battle CPU, and
    other Demo scenes, and loads nothing for a null descriptor.
  * with NDS_NATIVE_OWNER_IMAGE_VERIFY set to 1, verify calls equal ensure
    calls per case; with the flag set to 0, no verify calls happen while
    ensure behavior stays the same.

Proposal, not measured here: real NitroFS image bytes, real renderer
binding, and real CSS, intro, or battle runtime still belong to the ROM and
emulator arms.

NOT proven here: anything outside the extracted wrapper, including the slot
chain for fkinds other than the single mapped test kind, and production flag
combinations beyond the two compiled here.

Run:
    python -m pytest scripts/fighters/test_preview_image_detail.py -q
"""

from __future__ import annotations

import os
import re
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(SCRIPT_DIR, "..", "menus"))
from source_test_helpers import function  # noqa: E402

ROOT = Path(__file__).resolve().parents[2]
SRC = (ROOT / "src" / "import" / "battleship_ftmanager.c").read_text(
    encoding="utf-8")
FIGHTER_H = (ROOT / "include" / "ft" / "fighter.h").read_text(
    encoding="utf-8")
SCENE_H = (ROOT / "include" / "sc" / "scene.h").read_text(encoding="utf-8")
RENDERER_H = (ROOT / "include" / "nds" / "nds_renderer.h").read_text(
    encoding="utf-8")
GENERATED_H = (
    ROOT / "include" / "nds" / "generated" /
    "nds_native_fighter_image.generated.h").read_text(encoding="utf-8")
MAKEFILE = (ROOT / "Makefile").read_text(encoding="utf-8")
INTRO_C = (ROOT / "decomp" / "BattleShip-main" / "decomp" / "src" / "sc" /
           "sc1pmode" / "sc1pintro.c").read_text(encoding="utf-8")
CSS_C = (ROOT / "decomp" / "BattleShip-main" / "decomp" / "src" / "mn" /
         "mnplayers" / "mnplayers1pgame.c").read_text(encoding="utf-8")

LOG_CAP = 2000


def bounded(text: str, limit: int = LOG_CAP) -> str:
    text = text or ""
    return text if len(text) <= limit else text[:limit] + "... [truncated]"


def pin(pattern: str, text: str, label: str) -> None:
    if not re.search(pattern, text, re.S):
        raise AssertionError(f"reference drifted, update test: {label}")


def extract_wrapper() -> str:
    fn = function(SRC, "ftManagerMakeFighter")
    for needle in ("ndsRendererNativeEnsureOwnerImage",
                   "ndsRendererNativeVerifyOwnerImage",
                   "nSCKind1PGamePlayers",
                   "nSCKind1PIntro",
                   "nFTPlayerKindDemo",
                   "nFTPartsDetailLow",
                   "ndsBaseFTManagerMakeFighter"):
        if needle not in fn:
            raise AssertionError(f"extracted wrapper lost: {needle}")
    return fn


HARNESS_HEAD = r'''
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef uint8_t u8;
typedef uint32_t u32;
typedef int32_t s32;
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

typedef struct GObj { int _opaque; } GObj;
typedef struct FTDesc { s32 fkind; s32 pkind; u8 detail; } FTDesc;

/* Test declarations with actual port enum names. FTKind order mirrors
 * include/ft/fighter.h so the mapped test kind resolves to its slot. */
enum {
    nFTKindPlayableStart = 0,
    nFTKindMario = nFTKindPlayableStart,
    nFTKindFox,
    nFTKindDonkey,
    nFTKindSamus,
    nFTKindLuigi,
    nFTKindLink,
    nFTKindYoshi,
    nFTKindCaptain,
    nFTKindKirby,
    nFTKindPikachu,
    nFTKindPurin,
    nFTKindNess,
    nFTKindBoss,
    nFTKindMMario,
    nFTKindNStart,
    nFTKindNMario = nFTKindNStart,
    nFTKindNFox,
    nFTKindNDonkey,
    nFTKindNSamus,
    nFTKindNLuigi,
    nFTKindNLink,
    nFTKindNYoshi,
    nFTKindNCaptain,
    nFTKindNKirby,
    nFTKindNPikachu,
    nFTKindNPurin,
    nFTKindNNess,
    nFTKindGDonkey,
    nFTKindNull
};
enum {
    nFTPlayerKindMan = 0,
    nFTPlayerKindCom,
    nFTPlayerKindNot,
    nFTPlayerKindDemo,
    nFTPlayerKindKey,
    nFTPlayerKindGameKey
};
enum {
    nFTPartsDetailNone = 0,
    nFTPartsDetailStart = 1,
    nFTPartsDetailHigh = nFTPartsDetailStart,
    nFTPartsDetailLow = 2
};
/* Test scene values are distinct test-local ids under real port names. */
enum {
    nSCKind1PGamePlayers = 101,
    nSCKind1PIntro = 102,
    nSCKindVSBattle = 103,
    nSCKindVSResults = 104
};
typedef struct { u8 scene_curr; u8 scene_prev; } SCCommonData;
static SCCommonData gSCManagerSceneData;

/* Enable every fkind branch so the mapped test kind always resolves. */
#define NDS_P2_LUIGI 1
#define NDS_P2_DONKEY 1
#define NDS_P2_CAPTAIN 1
#define NDS_P2_SAMUS 1
#define NDS_P2_LINK 1
#define NDS_P2_PIKACHU 1
#define NDS_P2_YOSHI 1
#define NDS_P2_NESS 1
#define NDS_P2_PURIN 1
#define NDS_P2_KIRBY 1
#define NDS_P2_GDONKEY 1
#define NDS_P2_MMARIO 1
#define NDS_P2_NMARIO 1
#define NDS_P2_NFOX 1
#define NDS_P2_NDONKEY 1
#define NDS_P2_NSAMUS 1
#define NDS_P2_NLUIGI 1
#define NDS_P2_NLINK 1
#define NDS_P2_NYOSHI 1
#define NDS_P2_NCAPTAIN 1
#define NDS_P2_NKIRBY 1
#define NDS_P2_NPIKACHU 1
#define NDS_P2_NPURIN 1
#define NDS_P2_NNESS 1
#define NDS_P2_1P_GAME 1

/* Slot ids mirror the generated header values. */
#define NDS_NATIVE_IMAGE_SLOT_LUIGI 0u
#define NDS_NATIVE_IMAGE_SLOT_DONKEY 1u
#define NDS_NATIVE_IMAGE_SLOT_CAPTAIN 2u
#define NDS_NATIVE_IMAGE_SLOT_SAMUS 3u
#define NDS_NATIVE_IMAGE_SLOT_LINK 4u
#define NDS_NATIVE_IMAGE_SLOT_PIKACHU 5u
#define NDS_NATIVE_IMAGE_SLOT_YOSHI 6u
#define NDS_NATIVE_IMAGE_SLOT_NESS 7u
#define NDS_NATIVE_IMAGE_SLOT_PURIN 8u
#define NDS_NATIVE_IMAGE_SLOT_KIRBY 9u
#define NDS_NATIVE_IMAGE_SLOT_MMARIO 10u
#define NDS_NATIVE_IMAGE_SLOT_NMARIO 11u
#define NDS_NATIVE_IMAGE_SLOT_NFOX 12u
#define NDS_NATIVE_IMAGE_SLOT_NDONKEY 13u
#define NDS_NATIVE_IMAGE_SLOT_NSAMUS 14u
#define NDS_NATIVE_IMAGE_SLOT_NLINK 15u
#define NDS_NATIVE_IMAGE_SLOT_NYOSHI 16u
#define NDS_NATIVE_IMAGE_SLOT_NCAPTAIN 17u
#define NDS_NATIVE_IMAGE_SLOT_NKIRBY 18u
#define NDS_NATIVE_IMAGE_SLOT_NPIKACHU 19u
#define NDS_NATIVE_IMAGE_SLOT_NPURIN 20u
#define NDS_NATIVE_IMAGE_SLOT_NNESS 21u
#define NDS_NATIVE_IMAGE_SLOT_BOSS 22u
#define NDS_NATIVE_IMAGE_OWNER_SLOTS 23u

/* Call-recording stubs for the image seams. */
static u32 g_ensure[8];
static u32 g_nensure;
static u32 g_verify[8];
static u32 g_nverify;

s32 ndsRendererNativeEnsureOwnerImage(u32 owner_slot, u32 use_low_detail)
{
    (void)owner_slot;
    if (g_nensure < 8u) { g_ensure[g_nensure] = use_low_detail; }
    g_nensure++;
    return TRUE;
}

s32 ndsRendererNativeVerifyOwnerImage(u32 owner_slot, u32 use_low_detail)
{
    (void)owner_slot;
    if (g_nverify < 8u) { g_verify[g_nverify] = use_low_detail; }
    g_nverify++;
    return TRUE;
}

static GObj s_base_gobj;
GObj *ndsBaseFTManagerMakeFighter(FTDesc *desc)
{
    (void)desc;
    return &s_base_gobj;
}

GObj *ftManagerMakeFighter(FTDesc *desc);
'''

HOST_MAIN = r'''
static int s_failures = 0;

static void reset_calls(void)
{
    g_nensure = 0u;
    g_nverify = 0u;
    memset(g_ensure, 0, sizeof(g_ensure));
    memset(g_verify, 0, sizeof(g_verify));
}

/* Independent expectations: each wanted list is a literal per case, never
 * derived from the wrapper text. */
static void check(const char *name, u32 scene, s32 pkind, int detail,
                  int is_null, u32 nwant, const u32 *want)
{
    u32 i;
    int ok;
    FTDesc d;
    FTDesc *p;
    reset_calls();
    gSCManagerSceneData.scene_curr = (u8)scene;
    memset(&d, 0, sizeof(d));
    d.fkind = nFTKindMMario;
    d.pkind = pkind;
    d.detail = (u8)detail;
    p = is_null ? (FTDesc *)0 : &d;
    (void)ftManagerMakeFighter(p);
    ok = (g_nensure == nwant);
    for (i = 0u; i < nwant && i < 8u; i++) {
        if (g_ensure[i] != want[i]) { ok = 0; }
    }
#if NDS_NATIVE_OWNER_IMAGE_VERIFY
    if (g_nverify != g_nensure) { ok = 0; }
    for (i = 0u; i < g_nensure && i < 8u; i++) {
        if (g_verify[i] != g_ensure[i]) { ok = 0; }
    }
#else
    if (g_nverify != 0u) { ok = 0; }
#endif
    printf("CASE %s: %s nensure=%u nverify=%u\n",
           name, ok ? "PASS" : "FAIL",
           (unsigned)g_nensure, (unsigned)g_nverify);
    if (!ok) { s_failures++; }
}

int main(void)
{
    static const u32 only0[] = { 0u };
    static const u32 only1[] = { 1u };
    static const u32 both[] = { 0u, 1u };
    check("css_high_demo", nSCKind1PGamePlayers, nFTPlayerKindDemo,
          nFTPartsDetailHigh, 0, 1u, only0);
    check("intro_low_demo", nSCKind1PIntro, nFTPlayerKindDemo,
          nFTPartsDetailLow, 0, 1u, only1);
    check("intro_high_demo", nSCKind1PIntro, nFTPlayerKindDemo,
          nFTPartsDetailHigh, 0, 1u, only0);
    check("battle_human", nSCKindVSBattle, nFTPlayerKindMan,
          nFTPartsDetailHigh, 0, 2u, both);
    check("battle_cpu", nSCKindVSBattle, nFTPlayerKindCom,
          nFTPartsDetailLow, 0, 2u, both);
    check("other_demo", nSCKindVSResults, nFTPlayerKindDemo,
          nFTPartsDetailHigh, 0, 2u, both);
    check("null_desc", nSCKind1PGamePlayers, nFTPlayerKindDemo,
          nFTPartsDetailHigh, 1, 0u, both);
    if (s_failures == 0) { printf("ALL PASS\n"); return 0; }
    printf("%d FAILURES\n", s_failures);
    return 1;
}
'''


def pick_compiler() -> str:
    for name in ("gcc", "clang", "cc"):
        found = shutil.which(name)
        if found:
            return found
    raise AssertionError("no host C compiler found (gcc, clang, or cc)")


def compile_and_run(wrapper: str, verify: int, workdir: Path,
                    compiler: str) -> str:
    tag = f"verify{verify}"
    source = workdir / f"preview_detail_{tag}.c"
    program = workdir / (f"preview_detail_{tag}.exe"
                         if os.name == "nt" else f"preview_detail_{tag}")
    source.write_text(HARNESS_HEAD + "\n" + wrapper + "\n" + HOST_MAIN + "\n",
                      encoding="utf-8")
    built = subprocess.run(
        [compiler, "-std=c99", "-O1", "-w",
         f"-DNDS_NATIVE_OWNER_IMAGE_VERIFY={verify}",
         str(source), "-o", str(program)],
        capture_output=True, text=True, timeout=120)
    if built.returncode != 0:
        raise AssertionError(
            f"host build failed ({tag}):\n"
            f"{bounded((built.stderr or '') + (built.stdout or ''))}")
    ran = subprocess.run([str(program)], capture_output=True, text=True,
                         timeout=60)
    out = ran.stdout or ""
    if ran.returncode != 0:
        raise AssertionError(
            f"host run failed ({tag}):\n{bounded(out + (ran.stderr or ''))}")
    if "ALL PASS" not in out:
        raise AssertionError(
            f"missing ALL PASS ({tag}):\n{bounded(out)}")
    return out


class PreviewImageDetailTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.wrapper = extract_wrapper()
        cls.compiler = pick_compiler()
        cls.workdir = Path(tempfile.mkdtemp(prefix="preview-detail-"))
        cls.addClassCleanup(shutil.rmtree, cls.workdir, True)
        # Concrete cap: exactly 2 compiler runs and 2 executions.
        cls.out_on = compile_and_run(cls.wrapper, 1, cls.workdir,
                                     cls.compiler)
        cls.out_off = compile_and_run(cls.wrapper, 0, cls.workdir,
                                      cls.compiler)

    def test_guard_symbols_are_actual_port_names(self):
        """Wrapper guards use enum names defined by the port headers."""
        for symbol in ("nFTPlayerKindDemo", "nFTPartsDetailLow"):
            self.assertIn(symbol, FIGHTER_H, symbol)
            self.assertIn(symbol, self.wrapper, symbol)
        for symbol in ("nSCKind1PGamePlayers", "nSCKind1PIntro"):
            self.assertIn(symbol, SCENE_H, symbol)
            self.assertIn(symbol, self.wrapper, symbol)
        for symbol in ("ndsRendererNativeEnsureOwnerImage",
                       "ndsRendererNativeVerifyOwnerImage"):
            self.assertIn(symbol, RENDERER_H, symbol)
            self.assertIn(symbol, self.wrapper, symbol)
        self.assertIn("NDS_NATIVE_IMAGE_OWNER_SLOTS", GENERATED_H)
        self.assertIn("NDS_NATIVE_IMAGE_OWNER_SLOTS", self.wrapper)
        self.assertIn("NDS_NATIVE_OWNER_IMAGE_VERIFY", RENDERER_H)
        self.assertIn("NDS_NATIVE_OWNER_IMAGE_VERIFY", SRC)

    def test_source_context_intro_low_and_css_default(self):
        """Intro LOW override exists while CSS keeps the default detail."""
        pin(r"desc\.detail\s*=\s*nFTPartsDetailLow\s*;", INTRO_C,
            "sc1pintro.c LOW assignment")
        pin(r"void\s+mnPlayers1PGameMakeFighter", CSS_C, "CSS maker present")

    def test_compiled_gated_demo_loads_single_detail(self):
        """Compiled body loads one image for gated Demo actors."""
        for case in ("css_high_demo", "intro_low_demo", "intro_high_demo"):
            self.assertIn(f"CASE {case}: PASS", self.out_on,
                          bounded(self.out_on))

    def test_compiled_others_load_both_and_null_loads_none(self):
        """Compiled body loads both elsewhere and none for null."""
        for case in ("battle_human", "battle_cpu", "other_demo",
                     "null_desc"):
            self.assertIn(f"CASE {case}: PASS", self.out_on,
                          bounded(self.out_on))

    def test_compiled_verify_flag_on_and_off(self):
        """Verify matches ensure when on and stays silent when off."""
        for case in ("css_high_demo", "intro_low_demo", "intro_high_demo",
                     "battle_human", "battle_cpu", "other_demo",
                     "null_desc"):
            self.assertIn(f"CASE {case}: PASS", self.out_off,
                          bounded(self.out_off))
        self.assertIn("nensure=1 nverify=1", self.out_on)
        self.assertIn("nensure=2 nverify=2", self.out_on)
        self.assertIn("nensure=1 nverify=0", self.out_off)
        self.assertIn("nensure=2 nverify=0", self.out_off)
        self.assertIn("nensure=0 nverify=0", self.out_off)
        pin(r"NDS_NATIVE_OWNER_IMAGE_VERIFY\s*\?=\s*0", MAKEFILE,
            "verify Makefile default")


if __name__ == "__main__":
    unittest.main(verbosity=2)
