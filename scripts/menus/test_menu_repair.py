#!/usr/bin/env python3
"""Compile small, actual C function extractions; this is NOT a ROM/emulator test.

Usage: python test_menu_repair.py --repo /path/to/Smash64DS_Port [--cc cc]
Needs a host C11 compiler. Does not modify the checkout or build DS assets.
The test extracts the edited functions and the actual scene-stub guard block
from the supplied checkout. OS, video, allocation and surface I/O are test doubles.
"""
from __future__ import annotations
import argparse
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile


def function(text: str, name: str) -> str | None:
    # These deliberately small target functions have no braces inside literals.
    pattern = re.compile(r"(?m)^(?:static\s+)?void\s+" + re.escape(name) + r"\s*\([^;{}]*\)\s*\{")
    match = pattern.search(text)
    if match is None:
        return None
    opening = text.index("{", match.start())
    depth = 0
    for index in range(opening, len(text)):
        depth += (text[index] == "{") - (text[index] == "}")
        if depth == 0:
            return text[match.start():index + 1]
    raise ValueError(f"Unclosed function: {name}")


ENTRY_PREAMBLE = r'''
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
typedef uint16_t u16;
typedef struct { void *arena_start; size_t arena_size; } SceneSetup;
typedef struct { SceneSetup scene_setup; void (*func_start)(void); unsigned marker; } SYTaskmanSetup;
typedef struct { void *zbuffer; } SYVideoSetup;
unsigned starts, videos, parked, scene_curr, seen_scene;
unsigned char arena[4096];
u16 zbuffer;
void original_start(void) { assert(!"source func_start must not execute"); }
SYTaskmanSetup dMNVSModeTaskmanSetup = {{NULL, 17}, original_start, 0x12345678u};
SYVideoSetup dMNVSModeVideoSetup;
#define SYVIDEO_ZBUFFER_START(w,h,x,y,t) (&zbuffer)
void *ndsTaskmanArenaStart(void) { return arena; }
size_t ndsTaskmanArenaSize(void) { return sizeof(arena); }
void syVideoInit(SYVideoSetup *v) {
    assert(v == &dMNVSModeVideoSetup && v->zbuffer == &zbuffer); ++videos;
}
void syTaskmanStartTask(SYTaskmanSetup *s) {
    assert(s->scene_setup.arena_start == arena);
    assert(s->scene_setup.arena_size == sizeof(arena));
    assert(s->func_start == NULL && s->marker == 0x12345678u);
    assert(dMNVSModeTaskmanSetup.scene_setup.arena_start == NULL);
    assert(dMNVSModeTaskmanSetup.scene_setup.arena_size == 17);
    assert(dMNVSModeTaskmanSetup.func_start == original_start);
    seen_scene = scene_curr; ++starts;
}
#define NDS_SCENE_STUB(name) void name(void) { ++parked; }
'''
ENTRY_MAIN = r'''
int main(void) {
#if NDS_P2_MENU_SHELL
    void (*routes[])(void) = {mnModeSelectStartScene, mnVSOptionsStartScene, mnVSItemSwitchStartScene};
    /* Re-entry and source setup immutability, not a memory-leak measurement. */
    for (unsigned lap = 0; lap < 4; ++lap) {
        for (unsigned r = 0; r < 3; ++r) {
            scene_curr = 100 + r;
            unsigned before = starts;
            routes[r]();
            assert(starts == before + 1);
            assert(videos == starts && parked == 0);
            assert(scene_curr == 100 + r && seen_scene == scene_curr);
        }
    }
#else
    mnVSOptionsStartScene(); mnVSItemSwitchStartScene();
    assert(parked == 2 && starts == 0 && videos == 0);
#endif
    puts("entry-point assertions passed"); return 0;
}
'''
ROW_PREAMBLE = r'''
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
typedef uint32_t u32;
typedef unsigned NdsUiKitSurfaceId;
#define NDS_MENU_VSOPTIONS_ROWS 5u
#define FALSE 0
NdsUiKitSurfaceId sMenuVsOptionsRowSurface[5], wanted[5];
u32 gNdsMenuShellVsOptionsBlitCount, attempts, fail_at;
NdsUiKitSurfaceId ndsMenuShellVsOptionsWantSurface(u32 row) { return wanted[row]; }
int ndsUiKitBlitSurfaces(const NdsUiKitSurfaceId *surface, u32 count) {
    assert(surface != NULL && count == 1); ++attempts;
    return attempts != fail_at;
}
'''
ROW_MAIN = r'''
int main(int argc, char **argv) {
    assert(argc == 2);
    for (u32 i = 0; i < 5; ++i) wanted[i] = 10 + i;
    switch (atoi(argv[1])) {
    case 0: /* Zero budget must not write. */
        ndsMenuShellVsOptionsSyncRows(0);
        assert(attempts == 0 && gNdsMenuShellVsOptionsBlitCount == 0); break;
    case 1: /* One budget must not write two rows. */
        ndsMenuShellVsOptionsSyncRows(1);
        assert(attempts == 1 && gNdsMenuShellVsOptionsBlitCount == 1);
        assert(sMenuVsOptionsRowSurface[0] == wanted[0] && sMenuVsOptionsRowSurface[1] == 0); break;
    case 2: /* Entry can populate all rows. */
        ndsMenuShellVsOptionsSyncRows(5);
        assert(attempts == 5 && gNdsMenuShellVsOptionsBlitCount == 5); break;
    case 3: /* Idle does not re-blit. */
        for (u32 i = 0; i < 5; ++i) sMenuVsOptionsRowSurface[i] = wanted[i];
        ndsMenuShellVsOptionsSyncRows(5); assert(attempts == 0); break;
    case 4: /* Failed I/O must not mark content drawn; next call retries. */
        fail_at = 1; ndsMenuShellVsOptionsSyncRows(5);
        assert(attempts == 1 && gNdsMenuShellVsOptionsBlitCount == 0);
        for (u32 i = 0; i < 5; ++i) assert(sMenuVsOptionsRowSurface[i] == 0);
        fail_at = 0; ndsMenuShellVsOptionsSyncRows(5);
        assert(attempts == 6 && gNdsMenuShellVsOptionsBlitCount == 5); break;
    case 5: /* Two dirty rows: deselect old and highlight new. */
        for (u32 i = 0; i < 5; ++i) sMenuVsOptionsRowSurface[i] = wanted[i];
        ++wanted[1]; ++wanted[3]; ndsMenuShellVsOptionsSyncRows(5);
        assert(attempts == 2 && gNdsMenuShellVsOptionsBlitCount == 2); break;
    case 6: /* Failure after one success preserves exactly that success. */
        fail_at = 2; ndsMenuShellVsOptionsSyncRows(5);
        assert(attempts == 2 && gNdsMenuShellVsOptionsBlitCount == 1);
        assert(sMenuVsOptionsRowSurface[0] == wanted[0] && sMenuVsOptionsRowSurface[1] == 0);
        fail_at = 0; ndsMenuShellVsOptionsSyncRows(5);
        assert(gNdsMenuShellVsOptionsBlitCount == 5 && attempts == 6); break;
    default: assert(!"unknown case");
    }
    puts("row assertions passed"); return 0;
}
'''


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo", required=True, type=Path)
    parser.add_argument("--cc", default=os.environ.get("CC", "cc"))
    args = parser.parse_args(argv)
    compiler = shutil.which(args.cc)
    if compiler is None:
        parser.error(f"Host compiler not found: {args.cc}; supply --cc gcc or clang")
    router = (args.repo / "src/nds/nds_menu_shell_router.c").read_text(encoding="utf-8")
    backend = (args.repo / "src/port/title_backend.c").read_text(encoding="utf-8")
    options = (args.repo / "src/nds/nds_menu_shell_vsoptions.c").read_text(encoding="utf-8")
    stub_start = backend.index("NDS_SCENE_STUB(mnUnusedFightersStartScene)")
    stub_end = backend.index("#if !NDS_P2_1P_GAME", stub_start)
    stubs = backend[stub_start:stub_end]  # Keeps the actual shell-off guard.
    names = ["ndsMenuShellStartNative2DScene", "mnModeSelectStartScene",
             "mnVSOptionsStartScene", "mnVSItemSwitchStartScene"]
    bodies = [body for name in names if (body := function(router, name)) is not None]
    rows = function(options, "ndsMenuShellVsOptionsSyncRows")
    if not bodies or rows is None:
        raise ValueError("Expected menu function definitions not found; inspect the new source layout.")
    entry = ENTRY_PREAMBLE + "\n#if NDS_P2_MENU_SHELL\n" + "\n".join(bodies) + "\n#endif\n" + stubs + ENTRY_MAIN
    row_code = ROW_PREAMBLE + rows + ROW_MAIN
    results: list[tuple[str, bool]] = []
    with tempfile.TemporaryDirectory(prefix="smash-menu-test-") as temp:
        path = Path(temp)
        for label, code, defines, cases in (
            ("shell-on-entry", entry, ["-DNDS_P2_MENU_SHELL=1", "-DNDS_P2_1P_GAME=0"], [None]),
            ("shell-off-stubs", entry, ["-DNDS_P2_MENU_SHELL=0", "-DNDS_P2_1P_GAME=0"], [None]),
            ("row-budget", row_code, [], list(range(7))),
        ):
            c_file, executable = path / f"{label}.c", path / (label + (".exe" if os.name == "nt" else ""))
            c_file.write_text(code, encoding="utf-8")
            built = subprocess.run([compiler, "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror", *defines, str(c_file), "-o", str(executable)], capture_output=True, text=True)
            if built.returncode:
                print(f"FAIL {label}: compile\n{built.stderr}")
                results.append((label, False)); continue
            for case in cases:
                name = label if case is None else f"{label}/{case}"
                ran = subprocess.run([str(executable), *([] if case is None else [str(case)])], capture_output=True, text=True)
                good = ran.returncode == 0
                results.append((name, good))
                print(f"{'PASS' if good else 'FAIL'} {name}")
                if not good:
                    print(ran.stderr.strip())
    failures = sum(not passed for _, passed in results)
    print(f"{len(results) - failures}/{len(results)} host cases passed. ROM/runtime verification is separate.")
    return int(failures != 0)


def test_repository_menu_repairs() -> None:
    """Include the bundled extracted-C cases in the repository pytest suite."""
    compiler = shutil.which("gcc") or shutil.which("clang")
    assert compiler is not None, "A host C11 compiler is required"
    assert main(["--repo", str(Path(__file__).resolve().parents[2]),
                 "--cc", compiler]) == 0


if __name__ == "__main__":
    raise SystemExit(main())
