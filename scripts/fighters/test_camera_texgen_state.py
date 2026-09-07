#!/usr/bin/env python3
"""Host-execute ndsRendererAdapterCurrentLookAt from src/port/renderer_adapter_matrix.c.

Measured result: the suite compiles the production camera-selected LookAt
provider verbatim and runs it against stub cameras. An ordinary camera with
kinds 3 and 6, a null camera, and null XObj entries all return the reset
directions parsed from Interpreter::SpReset. A camera carrying kind 0x4c
returns the live battle LookAt pointer. A battle to ordinary to battle
sequence returns fresh battle data with no stale leak, and the reset bytes
stay unchanged after battle traffic.

Scope actually proven here:
  * extracted, unmodified via source_test_helpers.function:
    ndsRendererAdapterCurrentLookAt (camera-selected LookAt provider).
  * expected reset axes parsed from
    decomp/BattleShip-main/libultraship/src/fast/interpreter.cpp SpReset,
    not copied from the production initializer.
  * stubbed seams (NOT claimed as coverage): LookAt, Light, CObj, GObj, and
    XObj layouts reduced to the members the provider reads, the
    CObjGetStruct macro, the gGCCurrentCamera global, and the
    ndsR2CameraCurrentLookAt battle getter. No hardware behavior is claimed.
  * pinned integration text in src/nds/nds_renderer_native_common.c proving
    the texgen UV rebuild calls the camera-selected getter.

NOT proven here: real CSS or battle rendering, cadence, or emulator output.
Those belong to Main's serialization and measurement.

Run:
    python -m pytest scripts/fighters/test_camera_texgen_state.py -q
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
ADAPTER = (ROOT / "src/port/renderer_adapter_matrix.c").read_text(encoding="utf-8")
RENDERER = (ROOT / "src/nds/nds_renderer_native_common.c").read_text(encoding="utf-8")
INTERP = (
    ROOT / "decomp/BattleShip-main/libultraship/src/fast/interpreter.cpp"
).read_text(encoding="utf-8")

MAX_LOG = 4000


def bounded(text, limit=MAX_LOG):
    return text if len(text) <= limit else text[:limit] + "\n...[truncated]"


def pin(pattern, text, label):
    if not re.search(pattern, text, re.S):
        raise AssertionError(f"reference drifted, update test: {label}")
    return True


# Integration seams this suite relies on (fail loudly on drift).
pin(
    r"const LookAt \*ndsRendererAdapterCurrentLookAt\(void\)",
    ADAPTER,
    "provider declaration",
)
pin(
    r"#define NDS_RENDERER_ADAPTER_GM_CAMERA_MTX_KIND 0x4Cu",
    ADAPTER,
    "battle camera kind",
)
pin(
    r"return ndsR2CameraCurrentLookAt\(\);",
    ADAPTER,
    "battle leg returns live battle data",
)
pin(
    r"const LookAt \*look_at = ndsRendererAdapterCurrentLookAt\(\);",
    RENDERER,
    "UV rebuild uses camera-selected getter",
)


def parse_sp_reset_axes():
    """Parse the six SpReset direction assignments from interpreter source."""
    axes = {}
    for index in (0, 1):
        for axis in (0, 1, 2):
            match = re.search(
                rf"mRsp->lookat\[{index}\]\.dir\[{axis}\] = (-?\d+);",
                INTERP,
            )
            if match is None:
                raise AssertionError(
                    f"SpReset assignment missing: lookat[{index}].dir[{axis}]"
                )
            axes[(index, axis)] = int(match.group(1))
    first = (axes[(0, 0)], axes[(0, 1)], axes[(0, 2)])
    second = (axes[(1, 0)], axes[(1, 1)], axes[(1, 2)])
    return first, second


def production_kind_value():
    match = re.search(
        r"#define NDS_RENDERER_ADAPTER_GM_CAMERA_MTX_KIND (0x[0-9A-Fa-f]+|\d+)u?",
        ADAPTER,
    )
    if match is None:
        raise AssertionError("production battle kind define missing")
    return int(match.group(1), 0)


def extract_real():
    body = function(ADAPTER, "ndsRendererAdapterCurrentLookAt")
    for needle in (
        "gGCCurrentCamera",
        "CObjGetStruct",
        "ndsR2CameraCurrentLookAt",
        "NDS_RENDERER_ADAPTER_GM_CAMERA_MTX_KIND",
        "reset_look_at",
    ):
        if needle not in body:
            raise AssertionError(f"extracted provider lost seam: {needle}")
    return body


def build_source(first, second, kind_value):
    head = r'''
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef int8_t s8;
typedef uint8_t u8;
typedef uint32_t u32;

typedef struct {
    u8 col[3];
    u8 pad1;
    u8 colc[3];
    u8 pad2;
    s8 dir[3];
    u8 pad3;
} Light_t;

typedef union {
    Light_t l;
    long long int force_structure_alignment;
} Light;

typedef struct {
    Light l[2];
} LookAt;

typedef struct XObj {
    struct XObj *next;
    u8 kind;
    u8 unk05;
} XObj;

typedef struct CObj {
    int xobjs_num;
    XObj *xobjs[2];
} CObj;

typedef struct GObj {
    void *obj;
} GObj;

#define CObjGetStruct(gobj) ((CObj *)((gobj)->obj))

static LookAt gBattleLookAt;
GObj *gGCCurrentCamera = ((void *)0);

const LookAt *ndsR2CameraCurrentLookAt(void)
{
    return &gBattleLookAt;
}

#define NDS_RENDERER_ADAPTER_GM_CAMERA_MTX_KIND __KIND__
#define EXPECT_L0X __L0X__
#define EXPECT_L0Y __L0Y__
#define EXPECT_L0Z __L0Z__
#define EXPECT_L1X __L1X__
#define EXPECT_L1Y __L1Y__
#define EXPECT_L1Z __L1Z__
'''
    head = (
        head.replace("__KIND__", f"({kind_value}u)")
        .replace("__L0X__", str(first[0]))
        .replace("__L0Y__", str(first[1]))
        .replace("__L0Z__", str(first[2]))
        .replace("__L1X__", str(second[0]))
        .replace("__L1Y__", str(second[1]))
        .replace("__L1Z__", str(second[2]))
    )
    main = r'''
static int sFailures = 0;
#define CHECK(cond, ...) do { \
    if (!(cond)) { printf("FAIL %d: ", __LINE__); printf(__VA_ARGS__); \
        printf("\n"); sFailures++; } \
} while (0)

static void check_reset(const LookAt *got, const char *label)
{
    CHECK(got != ((void *)0), "%s null pointer", label);
    if (got == ((void *)0)) {
        return;
    }
    CHECK(got->l[0].l.dir[0] == EXPECT_L0X, "%s l0x %d", label,
        (int)got->l[0].l.dir[0]);
    CHECK(got->l[0].l.dir[1] == EXPECT_L0Y, "%s l0y %d", label,
        (int)got->l[0].l.dir[1]);
    CHECK(got->l[0].l.dir[2] == EXPECT_L0Z, "%s l0z %d", label,
        (int)got->l[0].l.dir[2]);
    CHECK(got->l[1].l.dir[0] == EXPECT_L1X, "%s l1x %d", label,
        (int)got->l[1].l.dir[0]);
    CHECK(got->l[1].l.dir[1] == EXPECT_L1Y, "%s l1y %d", label,
        (int)got->l[1].l.dir[1]);
    CHECK(got->l[1].l.dir[2] == EXPECT_L1Z, "%s l1z %d", label,
        (int)got->l[1].l.dir[2]);
}

int main(void)
{
    GObj camera_gobj;
    CObj cobj;
    XObj x0;
    XObj x1;
    const LookAt *got;
    const LookAt *reset_seen;

    memset(&camera_gobj, 0, sizeof(camera_gobj));
    memset(&cobj, 0, sizeof(cobj));
    memset(&x0, 0, sizeof(x0));
    memset(&x1, 0, sizeof(x1));

    /* Distinctive live battle vectors, far from any reset byte. */
    gBattleLookAt.l[0].l.dir[0] = 10;
    gBattleLookAt.l[0].l.dir[1] = 20;
    gBattleLookAt.l[0].l.dir[2] = 30;
    gBattleLookAt.l[1].l.dir[0] = 40;
    gBattleLookAt.l[1].l.dir[1] = 50;
    gBattleLookAt.l[1].l.dir[2] = 60;

    /* A. Null camera returns the SpReset default. */
    gGCCurrentCamera = ((void *)0);
    check_reset(ndsRendererAdapterCurrentLookAt(), "null camera");

    /* B. Camera GObj with null CObj returns the default. */
    camera_gobj.obj = ((void *)0);
    gGCCurrentCamera = &camera_gobj;
    check_reset(ndsRendererAdapterCurrentLookAt(), "null cobj");

    /* C. Ordinary CSS-style camera (kinds 3 and 6) returns the default. */
    cobj.xobjs_num = 2;
    x0.kind = 3;
    x1.kind = 6;
    cobj.xobjs[0] = &x0;
    cobj.xobjs[1] = &x1;
    camera_gobj.obj = &cobj;
    gGCCurrentCamera = &camera_gobj;
    check_reset(ndsRendererAdapterCurrentLookAt(), "ordinary 3/6");

    /* D. Zero XObjs returns the default. */
    cobj.xobjs_num = 0;
    cobj.xobjs[0] = &x0;
    cobj.xobjs[1] = &x1;
    check_reset(ndsRendererAdapterCurrentLookAt(), "zero xobjs");

    /* E. Null XObj entries are safe and return the default. */
    cobj.xobjs_num = 2;
    cobj.xobjs[0] = ((void *)0);
    cobj.xobjs[1] = ((void *)0);
    check_reset(ndsRendererAdapterCurrentLookAt(), "null xobjs");

    /* F. Non-battle custom kind 0x4B still returns the default. */
    x0.kind = 0x4B;
    cobj.xobjs[0] = &x0;
    cobj.xobjs[1] = ((void *)0);
    check_reset(ndsRendererAdapterCurrentLookAt(), "kind 0x4B");

    /* G. Battle camera with kind 0x4C in slot 0 returns live data. */
    x0.kind = (u8)NDS_RENDERER_ADAPTER_GM_CAMERA_MTX_KIND;
    cobj.xobjs[0] = &x0;
    cobj.xobjs[1] = ((void *)0);
    got = ndsRendererAdapterCurrentLookAt();
    CHECK(got == &gBattleLookAt, "battle slot0 pointer");
    CHECK(got->l[0].l.dir[0] == 10, "battle slot0 l0x");
    CHECK(got->l[1].l.dir[2] == 60, "battle slot0 l1z");

    /* H. Battle kind in slot 1 also returns live data. */
    x0.kind = 6;
    x1.kind = (u8)NDS_RENDERER_ADAPTER_GM_CAMERA_MTX_KIND;
    cobj.xobjs[0] = &x0;
    cobj.xobjs[1] = &x1;
    got = ndsRendererAdapterCurrentLookAt();
    CHECK(got == &gBattleLookAt, "battle slot1 pointer");

    /* I. Null in one slot does not hide a battle kind in the other. */
    cobj.xobjs[0] = ((void *)0);
    cobj.xobjs[1] = &x1;
    got = ndsRendererAdapterCurrentLookAt();
    CHECK(got == &gBattleLookAt, "battle beside null");

    /* J. Battle, ordinary, battle: no stale leak in either direction. */
    x0.kind = (u8)NDS_RENDERER_ADAPTER_GM_CAMERA_MTX_KIND;
    cobj.xobjs[0] = &x0;
    cobj.xobjs[1] = ((void *)0);
    got = ndsRendererAdapterCurrentLookAt();
    CHECK(got == &gBattleLookAt, "re Battle pointer");
    CHECK(got->l[0].l.dir[1] == 20, "re Battle value");
    x0.kind = 6;
    x1.kind = 3;
    cobj.xobjs[0] = &x0;
    cobj.xobjs[1] = &x1;
    reset_seen = ndsRendererAdapterCurrentLookAt();
    check_reset(reset_seen, "ordinary after battle");
    gBattleLookAt.l[0].l.dir[0] = 11;
    gBattleLookAt.l[0].l.dir[1] = 21;
    gBattleLookAt.l[0].l.dir[2] = 31;
    gBattleLookAt.l[1].l.dir[0] = 41;
    gBattleLookAt.l[1].l.dir[1] = 51;
    gBattleLookAt.l[1].l.dir[2] = 61;
    x0.kind = (u8)NDS_RENDERER_ADAPTER_GM_CAMERA_MTX_KIND;
    cobj.xobjs[0] = &x0;
    cobj.xobjs[1] = ((void *)0);
    got = ndsRendererAdapterCurrentLookAt();
    CHECK(got == &gBattleLookAt, "battle after ordinary pointer");
    CHECK(got->l[0].l.dir[0] == 11, "fresh battle l0x %d",
        (int)got->l[0].l.dir[0]);
    CHECK(got->l[1].l.dir[2] == 61, "fresh battle l1z %d",
        (int)got->l[1].l.dir[2]);
    CHECK(got != reset_seen, "battle and reset are distinct objects");

    /* K. Reset bytes are unchanged by battle traffic. */
    x0.kind = 6;
    cobj.xobjs[0] = &x0;
    cobj.xobjs[1] = ((void *)0);
    check_reset(ndsRendererAdapterCurrentLookAt(), "reset after battle");

    if (sFailures == 0) {
        printf("ALL PASS\n");
        return 0;
    }
    printf("%d FAILURES\n", sFailures);
    return 1;
}
'''
    return head + extract_real() + main


class CameraTexgenStateTest(unittest.TestCase):
    def test_sp_reset_source_parses_to_two_axes(self):
        first, second = parse_sp_reset_axes()
        self.assertEqual(first, (0, 127, 0), "SpReset lookat[0]")
        self.assertEqual(second, (127, 0, 0), "SpReset lookat[1]")

    def test_production_battle_kind_is_0x4c(self):
        self.assertEqual(production_kind_value(), 0x4C, "battle kind")

    def test_uv_caller_uses_camera_selected_getter(self):
        self.assertIn(
            "const LookAt *look_at = ndsRendererAdapterCurrentLookAt();",
            RENDERER,
            "UV caller getter",
        )
        window = RENDERER[
            RENDERER.index(
                "const LookAt *look_at = ndsRendererAdapterCurrentLookAt();"
            ) - 800:
            RENDERER.index(
                "const LookAt *look_at = ndsRendererAdapterCurrentLookAt();"
            ) + 800
        ]
        self.assertIn("use_texgen", window, "texgen gate context")
        self.assertIn("look_at->l[0]", window, "LookAt leg context")

    def test_extracted_provider_keeps_both_legs(self):
        body = extract_real()
        self.assertIn("reset_look_at", body, "default leg")
        self.assertIn("ndsR2CameraCurrentLookAt", body, "battle leg")

    def test_provider_against_stub_cameras(self):
        first, second = parse_sp_reset_axes()
        source_text = build_source(first, second, production_kind_value())
        compiler = next(
            (c for c in ("clang", "gcc", "cc") if shutil.which(c)),
            None,
        )
        self.assertIsNotNone(compiler, "Host C compiler required (clang/gcc/cc)")
        with tempfile.TemporaryDirectory() as directory:
            directory_path = Path(directory)
            source = directory_path / "camera_texgen_real.c"
            program = directory_path / "camera_texgen_real.exe"
            source.write_text(source_text, encoding="utf-8")
            built = subprocess.run(
                [
                    compiler,
                    "-std=c11",
                    "-Wall",
                    "-Wextra",
                    "-Werror",
                    str(source),
                    "-o",
                    str(program),
                ],
                capture_output=True,
            )
            self.assertEqual(
                built.returncode,
                0,
                f"host build failed:\n{bounded(built.stderr.decode('utf-8', 'replace'))}",
            )
            ran = subprocess.run(
                [str(program)],
                capture_output=True,
                timeout=60,
                cwd=directory,
            )
            out = ran.stdout.decode("utf-8", "replace")
            err = ran.stderr.decode("utf-8", "replace")
            self.assertEqual(
                ran.returncode,
                0,
                f"host run failed (stderr):\n{bounded(err)}\n"
                f"(stdout):\n{bounded(out)}",
            )
            self.assertIn("ALL PASS", out, f"missing ALL PASS:\n{bounded(out)}")


if __name__ == "__main__":
    unittest.main()
