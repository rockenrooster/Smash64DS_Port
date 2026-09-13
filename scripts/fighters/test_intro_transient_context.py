#!/usr/bin/env python3
"""Host-execute the 1P Intro transient ownership gate from
src/port/renderer_adapter_fighter.c.

Measured result: the suite compiles the production bind-change predicate
verbatim and runs the 21-actor ownership protocol against stub GObjs: every
distinct source GObj forces scratch invalidation, repeats of the bound GObj
do not, and NULL never invalidates. String pins keep the four battle-cache
arrays at four entries and pin the transient integration seams (contract
fallback, memo disarm, plan force-miss plus no-bake, scratch reset, and the
Intro draw wrapper) so drift fails loudly instead of silently aliasing.

Scope actually proven here:
  * extracted, unmodified via source_test_helpers.function:
    ndsIntroTransientNeedsBindChange (pointer-identity ownership test).
  * stubbed seams (NOT claimed as coverage): GObj layout, sb32/TRUE/FALSE,
    the scratch plan/memo/packet caches, native production, camera and pose
    paths, and real Intro runtime. No hardware behavior is claimed.
  * pinned integration text in src/port/renderer_adapter_fighter.c and
    src/import/battleship_sc1pintro.c (presence checks only).

NOT proven here: real Intro rendering, cadence, or emulator output. Those
belong to Main's serialization and measurement.

Run:
    python -m pytest scripts/fighters/test_intro_transient_context.py -q
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
sys.path.insert(0, SCRIPT_DIR)
sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "scripts" / "menus"))
from source_test_helpers import function  # noqa: E402

ROOT = Path(__file__).resolve().parents[2]
ADAPTER = (ROOT / "src/port/renderer_adapter_fighter.c").read_text(
    encoding="utf-8")
INTRO_TU = (ROOT / "src/import/battleship_sc1pintro.c").read_text(
    encoding="utf-8")

MAX_LOG = 4000


def bounded(text, limit=MAX_LOG):
    return text if len(text) <= limit else text[:limit] + "\n...[truncated]"


def pin(pattern, text, label):
    if not re.search(pattern, text, re.S):
        raise AssertionError(f"reference drifted, update test: {label}")
    return True


# Integration seams this suite relies on (fail loudly on drift).
pin(r"static sb32 ndsIntroTransientNeedsBindChange\(GObj \*bound, GObj \*incoming\)",
    ADAPTER, "bind predicate")
pin(r"\(void\)ndsFighterIntroTransientSubmit\(fighter_gobj\);", ADAPTER,
    "contract fallback")
pin(r"\(sNdsIntroTransientActive == FALSE\) &&\s*\(fp->camera_mode",
    ADAPTER, "memo disarm")
pin(r"native_owner_plan_hit = \(\(sNdsIntroTransientActive != FALSE\)",
    ADAPTER, "plan force-miss")
pin(r"\(sNdsIntroTransientActive == FALSE\) &&\s*\(slot < GMCOMMON_PLAYERS_MAX\)",
    ADAPTER, "plan no-bake")
pin(r"sNdsIntroTransientBoundGObj = NULL;", ADAPTER, "reset clears bound")
pin(r"NDS_INTRO_TRANSIENT_SCRATCH_SLOT\]\.valid = 0u;", ADAPTER,
    "reset clears scratch")
pin(r"#define NDS_INTRO_TRANSIENT_SCRATCH_SLOT 0u", ADAPTER, "scratch slot")
pin(r"sNdsFighterDrawPlan\[GMCOMMON_PLAYERS_MAX\]", ADAPTER,
    "draw plan stays four entries")
pin(r"sNdsFtrDrawMemo\[GMCOMMON_PLAYERS_MAX\]", ADAPTER,
    "draw memo stays four entries")
pin(r"sNdsFighterDisplayContractLastFrame\[GMCOMMON_PLAYERS_MAX\]",
    ADAPTER, "dedup stays four entries")
pin(r"#define scManagerFuncDraw ndsSC1PIntroDraw", INTRO_TU, "draw wrapper")
pin(r"gGCCommonLinks\[nGCCommonLinkIDFighter\]", INTRO_TU, "visibility walk")
pin(r"ndsPlatformSet3DViewportSource\(10, 10, 310, 230\)", INTRO_TU,
    "fighter viewport")
pin(r"ndsFighterIntroTransientReset\(\);", INTRO_TU, "startscene reset")


def extract_real():
    body = function(ADAPTER, "ndsIntroTransientNeedsBindChange")
    if "bound != incoming" not in body:
        raise AssertionError("extracted bind predicate lost identity compare")
    if "incoming == NULL" not in body:
        raise AssertionError("extracted bind predicate lost NULL guard")
    return body


HARNESS_HEAD = r'''
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef int32_t sb32;
typedef struct GObj { int tag; } GObj;
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif
#ifndef NULL
#define NULL ((void *)0)
#endif
'''

HOST_MAIN = r'''
static int sFailures = 0;
#define CHECK(cond, ...) do { \
    if (!(cond)) { printf("FAIL %d: ", __LINE__); printf(__VA_ARGS__); \
        printf("\n"); sFailures++; } \
} while (0)

/* Mirror the production protocol: invalidate scratch exactly when the
 * predicate fires, then draw through the bound GObj. Counts prove every
 * one of the 21 stage-maximum actors draws with fresh state. */
static int sInvalidates = 0;
static int sDraws = 0;
static GObj *sBound = NULL;
static void submit(GObj *gobj)
{
    if (ndsIntroTransientNeedsBindChange(sBound, gobj) != FALSE) {
        sInvalidates++;
        sBound = gobj;
    }
    if (gobj != NULL) {
        sDraws++;
    }
}

int main(void)
{
    GObj actors[21];
    int i;
    for (i = 0; i < 21; i++) {
        actors[i].tag = i;
    }
    /* A. NULL never binds and never invalidates. */
    submit(NULL);
    CHECK(sInvalidates == 0, "null invalidates");
    CHECK(sDraws == 0, "null draws");
    CHECK(sBound == NULL, "null binds");
    /* B. Stage-maximum run: 21 distinct actors invalidate 21 times. */
    for (i = 0; i < 21; i++) {
        submit(&actors[i]);
    }
    CHECK(sInvalidates == 21, "21 actors invalidate %d", sInvalidates);
    CHECK(sDraws == 21, "21 actors draw %d", sDraws);
    /* C. Repeats of the bound actor reuse state without invalidation. */
    for (i = 0; i < 5; i++) {
        submit(&actors[20]);
    }
    CHECK(sInvalidates == 21, "repeat invalidates %d", sInvalidates);
    CHECK(sDraws == 26, "repeat draws %d", sDraws);
    /* D. Same-kind interleaving (distinct pointers) always invalidates:
     * Yoshi/Kirby teams and Zako must never share scratch state. */
    sBound = NULL; sInvalidates = 0; sDraws = 0;
    for (i = 0; i < 6; i++) {
        submit(&actors[i % 2]);
    }
    CHECK(sInvalidates == 6, "interleave invalidates %d", sInvalidates);
    CHECK(sDraws == 6, "interleave draws %d", sDraws);
    /* E. Direct predicate unit checks. */
    CHECK(ndsIntroTransientNeedsBindChange(NULL, &actors[0]) != FALSE,
          "unbound binds");
    CHECK(ndsIntroTransientNeedsBindChange(&actors[0], &actors[0]) == FALSE,
          "same actor");
    CHECK(ndsIntroTransientNeedsBindChange(&actors[0], NULL) == FALSE,
          "null incoming");
    CHECK(ndsIntroTransientNeedsBindChange(&actors[0], &actors[1]) != FALSE,
          "distinct actors");
    if (sFailures == 0) {
        printf("ALL PASS\n");
        return 0;
    }
    printf("%d FAILURES\n", sFailures);
    return 1;
}
'''


def build_source():
    return (HARNESS_HEAD + extract_real() + HOST_MAIN)


class IntroTransientContextTest(unittest.TestCase):
    def test_full_submit_preserves_capture_playback_and_actor_identity(self):
        code = r'''
#include <assert.h>
#include <stddef.h>
typedef unsigned u32; typedef int sb32;
#define TRUE 1
#define FALSE 0
#define NDS_R2_FIGHTER_NO_ORACLE 1
#define NDS_RENDERER_PROFILE_LEVEL 0
#define NDS_INTRO_TRANSIENT_SCRATCH_SLOT 0u
#define nSCKind1PIntro 14
typedef struct GObj GObj;
typedef struct FTStruct { GObj *fighter_gobj; u32 fkind; } FTStruct;
struct GObj { FTStruct *fp; };
static u32 gNdsSceneManagerCurrKind = 14;
static GObj *sNdsIntroTransientBoundGObj, *captured;
static sb32 sNdsIntroTransientActive, sNdsFighterDisplayContractPlayback;
static u32 gNdsIntroTransientSubmitCount, gNdsIntroTransientDrawCount;
static u32 gNdsIntroTransientBindChanges, gNdsIntroTransientOwnerRejectCount;
static u32 gNdsFighterMarioFoxDLAllDrawCount, invalidations, oracle, empty;
static struct { u32 event_count; } sNdsFighterDisplayContract;
static FTStruct *ftGetStruct(GObj *g) { return g->fp; }
static sb32 ndsFighterGetNativeOwnerSlot(FTStruct *fp, u32 *slot) {
    *slot = fp->fkind; return fp->fkind < 12;
}
static void ndsFighterIntroTransientInvalidateScratch(void) { invalidations++; }
static void ndsFighterDisplayContractCapture(GObj *g) {
    assert(sNdsIntroTransientActive); captured = g;
    sNdsFighterDisplayContract.event_count = empty ? 0 : 1;
}
static u32 ndsRendererHardwareNoOracleEnabled(void) { return oracle; }
static void ndsRendererHardwareSetNoOracle(u32 value) { oracle = value; }
static void ndsFighterMarioFoxDLAllDrawForSlot(u32 slot, FTStruct *fp,
                                              void *pixels, u32 pitch) {
    assert(slot == 0 && pixels == NULL && pitch == 0);
    assert(sNdsIntroTransientActive && sNdsFighterDisplayContractPlayback);
    assert(oracle && captured->fp == fp);
    gNdsFighterMarioFoxDLAllDrawCount++;
}
'''
        code += '\n' + function(ADAPTER, 'ndsIntroTransientNeedsBindChange')
        code += '\n' + function(ADAPTER, 'ndsFighterIntroTransientSubmit')
        code += r'''
int main(void) {
    GObj actors[21]; FTStruct fighters[21];
    for (u32 i = 0; i < 21; i++) {
        actors[i].fp = &fighters[i]; fighters[i].fighter_gobj = &actors[i];
        fighters[i].fkind = 6; oracle = i & 1;
        assert(ndsFighterIntroTransientSubmit(&actors[i]));
        assert(!sNdsIntroTransientActive && !sNdsFighterDisplayContractPlayback);
        assert(oracle == (i & 1));
    }
    assert(invalidations == 21 && gNdsIntroTransientDrawCount == 21);
    assert(ndsFighterIntroTransientSubmit(&actors[20]));
    assert(invalidations == 21 && gNdsIntroTransientDrawCount == 22);
    empty = 1; assert(!ndsFighterIntroTransientSubmit(&actors[0]));
    assert(!sNdsIntroTransientActive && !sNdsFighterDisplayContractPlayback);
    assert(!ndsFighterIntroTransientSubmit(NULL));
    gNdsSceneManagerCurrKind = 52;
    assert(!ndsFighterIntroTransientSubmit(&actors[0]));
    return 0;
}
'''
        with tempfile.TemporaryDirectory() as directory:
            c = Path(directory) / 'submit.c'
            exe = c.with_suffix('.exe')
            c.write_text(code)
            result = subprocess.run([shutil.which('gcc'), '-std=c11', str(c),
                                     '-o', str(exe)], capture_output=True)
            self.assertEqual(result.returncode, 0, result.stderr.decode())
            subprocess.run([str(exe)], check=True, capture_output=True)

    MAX_LOG = MAX_LOG

    def compile_and_run(self, source_text, tag, compiler):
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory) / f"intro_transient_{tag}.c"
            program = Path(directory) / f"intro_transient_{tag}.exe"
            source.write_text(source_text, encoding="utf-8")
            built = subprocess.run(
                [compiler, "-std=c11", "-Wall", "-Wextra", "-Werror",
                 str(source), "-o", str(program)],
                capture_output=True)
            self.assertEqual(
                built.returncode, 0,
                f"host build failed:\n{bounded(built.stderr.decode('utf-8', 'replace'))}")
            ran = subprocess.run([str(program)], capture_output=True,
                                 timeout=60, cwd=directory)
            out = ran.stdout.decode("utf-8", "replace")
            err = ran.stderr.decode("utf-8", "replace")
            self.assertEqual(
                ran.returncode, 0,
                f"host run failed (stderr):\n{bounded(err)}\n"
                f"(stdout):\n{bounded(out)}")
            self.assertIn("ALL PASS", out,
                          f"missing ALL PASS:\n{bounded(out)}")

    def pick_compiler(self):
        compiler = next((c for c in ("clang", "gcc", "cc")
                         if shutil.which(c)), None)
        self.assertIsNotNone(compiler,
                             "Host C compiler required (clang/gcc/cc)")
        return compiler

    def test_bind_protocol_against_stubs(self):
        self.compile_and_run(build_source(), "real", self.pick_compiler())

    def test_scratch_slot_within_instance_range(self):
        """Scratch index must address the existing four-entry caches."""
        match = re.search(
            r"#define NDS_INTRO_TRANSIENT_SCRATCH_SLOT (\d+)u", ADAPTER)
        self.assertIsNotNone(match, "scratch slot define")
        slot = int(match.group(1))
        self.assertGreaterEqual(slot, 0, "scratch slot")
        self.assertLess(slot, 4, "scratch slot inside four-entry caches")

    def test_no_twenty_one_entry_caches(self):
        """Battle caches stay four entries; only one transient context."""
        for needle in ("sNdsFighterStatusGeneration[GMCOMMON_PLAYERS_MAX]",
                       "sNdsFtrContractCensusPrev[GMCOMMON_PLAYERS_MAX]"):
            self.assertIn(needle, ADAPTER, needle)
        self.assertNotIn("[21]", ADAPTER, "no 21-entry cache")
        self.assertNotIn("GMCOMMON_PLAYERS_MAX +", ADAPTER,
                         "no grown cache")


if __name__ == "__main__":
    unittest.main()
