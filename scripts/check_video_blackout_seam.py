#!/usr/bin/env python3
"""Host checks for the DS video blackout seam, against the REAL TUs.

Compiles src/port/video_blackout.c and src/import/battleship_sys_video.c
(which itself compiles the decomp BattleShip sys/video.c) on the host and
exercises the actual functions -- no extracted copies:

Build A (host ABI, no ARM9): latch semantics + the shared-seam mirror.
  - ndsVideoSetBlackout normalizes any nonzero to TRUE and 0 to FALSE.
  - syVideoSetFlags(SYVIDEO_FLAG_BLACKOUT) latches, NOBLACKOUT restores, and
    a combined word resolves NOBLACKOUT -- the same order the source
    scheduler applies (decomp sys/scheduler.c:456-463, NOBLACKOUT second).
  - syVideoInit mirrors the setup flags: a NOBLACKOUT setup restores, a setup
    with no blackout bits leaves the latch untouched (mirror, not clear).
  - The base decomp TU is live under the wrapper: syVideoGetFillColor follows
    gSYVideoColorDepth, and syVideoApplySettingsNoBlock rides flags out on a
    task message (osSendMesg stub counts sends; task.flags carries the
    latched flags), which is exactly how source defers VI application.

Build B (-DARM9 with a fake nds/arm9/video.h routing REG_MASTER_BRIGHT onto
plain storage): the deferred hardware commit.
  - ndsVideoSetBlackout writes NO registers (source applies VI flags at a
    scheduler frame boundary, never at the setFlags call site).
  - ndsVideoBlackoutCommit writes 0x8010 (fade-down, level 16) to BOTH
    master-brightness registers when dirty, 0x0000 on restore.
  - A clean commit touches nothing; one dirty flag produces exactly one
    write no matter how many sets preceded it.

Usage:
    python scripts/check_video_blackout_seam.py [--verbose]
"""

import argparse
import os
import shutil
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC_BLACKOUT = os.path.join(ROOT, "src", "port", "video_blackout.c")
SRC_SYS_VIDEO = os.path.join(ROOT, "src", "import", "battleship_sys_video.c")
INCLUDES = [
    "-I", os.path.join(ROOT, "include"),
    "-I", os.path.join(ROOT, "decomp", "BattleShip-main", "decomp", "src"),
    "-I", os.path.join(ROOT, "decomp", "BattleShip-main", "decomp", "include"),
]

HARNESS_A = r"""
#include <stdio.h>
#include <sys/video.h>

/* Link stubs for the base decomp TU's scheduler/libultra externals. */
static int sSends;

void func_80000970(SYTaskInfo *info) { (void)info; }
s32 osSendMesg(OSMesgQueue *mq, OSMesg msg, s32 flags)
{
    (void)mq; (void)msg; (void)flags;
    sSends++;
    return 0;
}
OSMesgQueue gSYSchedulerTaskMesgQueue;

static int sFails;
#define CHECK(cond, why)                                        \
    do {                                                        \
        if (!(cond)) {                                          \
            printf("FAIL: %s\n", why);                          \
            sFails++;                                           \
        }                                                       \
    } while (0)

int main(void)
{
    SYVideoSetup setup;
    SYTaskVi task;

    /* Normalization: any nonzero latches TRUE, 0 latches FALSE. */
    ndsVideoSetBlackout(2);
    CHECK(ndsVideoGetBlackout() == TRUE, "set(2) must normalize to TRUE");
    ndsVideoSetBlackout(0);
    CHECK(ndsVideoGetBlackout() == FALSE, "set(0) must latch FALSE");

    /* Endings latch, then explicit restore (source two-step). */
    syVideoSetFlags(SYVIDEO_FLAG_BLACKOUT);
    CHECK(ndsVideoGetBlackout() == TRUE, "setFlags(BLACKOUT) latches");
    syVideoSetFlags(SYVIDEO_FLAG_NOBLACKOUT);
    CHECK(ndsVideoGetBlackout() == FALSE, "setFlags(NOBLACKOUT) restores");

    /* Same-word precedence: scheduler.c:456-463 applies NOBLACKOUT second. */
    syVideoSetFlags(SYVIDEO_FLAG_BLACKOUT | SYVIDEO_FLAG_NOBLACKOUT);
    CHECK(ndsVideoGetBlackout() == FALSE,
          "combined word must resolve NOBLACKOUT");

    /* syVideoInit mirrors the setup flags; it does not clear unconditionally. */
    ndsVideoSetBlackout(TRUE);
    setup.framebuffers[0] = setup.framebuffers[1] =
        setup.framebuffers[2] = (void *)0;
    setup.zbuffer = NULL;
    setup.width = 320;
    setup.height = 240;
    setup.flags = SYVIDEO_FLAG_NOBLACKOUT | SYVIDEO_FLAG_COLORDEPTH16;
    syVideoInit(&setup);
    CHECK(ndsVideoGetBlackout() == FALSE,
          "init must mirror the setup's NOBLACKOUT");

    ndsVideoSetBlackout(TRUE);
    setup.flags = SYVIDEO_FLAG_COLORDEPTH16;
    syVideoInit(&setup);
    CHECK(ndsVideoGetBlackout() == TRUE,
          "init with no blackout bits must leave the latch alone");

    /* The base decomp TU is the code under the wrapper, not a stub: color
     * depth follows the setup/COLORDEPTH flags through the real TU. */
    CHECK(syVideoGetFillColor(0xF8FCFEFFu) == 0xFFFFFFFFu,
          "fill color must pack RGBA5551 duplicated for 16b depth");
    syVideoSetFlags(SYVIDEO_FLAG_COLORDEPTH32);
    CHECK(syVideoGetFillColor(0x12345678u) == 0x12345678u,
          "32b depth must return the color unmodified");
    syVideoSetFlags(SYVIDEO_FLAG_COLORDEPTH16);

    /* Flags ride a task message out of ApplySettingsNoBlock -- the source
     * scheduling the deferred commit mirrors. A fresh syVideoInit opens a
     * clean accumulation window; setFlags ORs into it (video.c:82), never
     * replaces. */
    setup.flags = SYVIDEO_FLAG_COLORDEPTH16;
    syVideoInit(&setup);
    sSends = 0;
    syVideoSetFlags(SYVIDEO_FLAG_GAMMA);
    syVideoSetFlags(SYVIDEO_FLAG_SERRATE | SYVIDEO_FLAG_NOSERRATE);
    syVideoApplySettingsNoBlock(&task);
    CHECK(sSends == 1, "settings change must send exactly one VI task");
    CHECK(task.flags == (SYVIDEO_FLAG_GAMMA | SYVIDEO_FLAG_SERRATE |
                         SYVIDEO_FLAG_NOSERRATE),
          "VI task must carry the accumulated flags verbatim");
    sSends = 0;
    syVideoApplySettingsNoBlock(&task);
    CHECK(sSends == 0, "no change must send no task");

    /* Commit is callable and idempotent in host ABI. */
    ndsVideoSetBlackout(TRUE);
    ndsVideoBlackoutCommit();
    ndsVideoBlackoutCommit();
    CHECK(ndsVideoGetBlackout() == TRUE, "commit must not change the latch");

    printf("build A fails %d\n", sFails);
    return sFails ? 1 : 0;
}
"""

HARNESS_B = r"""
#include <stdio.h>
#include <sys/video.h>
/* No <stdint.h> here: the system header pulls in <stddef.h>, which the
 * decomp include root replaces with its own that bootstraps PR/ultratypes
 * before any stdint type exists. Nothing in this harness needs it. */

unsigned short fake_master_bright_main;
unsigned short fake_master_bright_sub;

static int sFails;
#define CHECK(cond, why)                                        \
    do {                                                        \
        if (!(cond)) {                                          \
            printf("FAIL: %s\n", why);                          \
            sFails++;                                           \
        }                                                       \
    } while (0)

int main(void)
{
    CHECK(fake_master_bright_main == 0 && fake_master_bright_sub == 0,
          "registers must start untouched");

    /* Deferral: the set path must not touch the registers at all. */
    ndsVideoSetBlackout(TRUE);
    CHECK(fake_master_bright_main == 0 && fake_master_bright_sub == 0,
          "set must NOT write registers mid-frame");
    CHECK(ndsVideoGetBlackout() == TRUE, "set must latch TRUE");

    /* Commit applies fade-down level 16 to BOTH engines. */
    ndsVideoBlackoutCommit();
    CHECK(fake_master_bright_main == 0x8010 &&
          fake_master_bright_sub == 0x8010,
          "commit must write 0x8010 to both master-brightness registers");

    /* Restore writes zero. */
    ndsVideoSetBlackout(FALSE);
    ndsVideoBlackoutCommit();
    CHECK(fake_master_bright_main == 0 && fake_master_bright_sub == 0,
          "restore commit must clear both registers");

    /* Clean commit touches nothing. */
    fake_master_bright_main = 0x1234;
    fake_master_bright_sub = 0x5678;
    ndsVideoBlackoutCommit();
    CHECK(fake_master_bright_main == 0x1234 &&
          fake_master_bright_sub == 0x5678,
          "clean commit must not rewrite the registers");

    /* One dirty flag -> one write, however many sets preceded it. */
    ndsVideoSetBlackout(TRUE);
    ndsVideoSetBlackout(TRUE);
    ndsVideoBlackoutCommit();
    ndsVideoBlackoutCommit();
    CHECK(fake_master_bright_main == 0x8010 && fake_master_bright_sub == 0x8010,
          "double set must still commit exactly one value");

    printf("build B fails %d\n", sFails);
    return sFails ? 1 : 0;
}
"""

FAKE_NDS_VIDEO_H = r"""
/* Host-test stand-in for libnds <nds/arm9/video.h>: routes the two
 * master-brightness register macros onto plain storage so the ARM9 write
 * path of src/port/video_blackout.c is observable off hardware. */
#ifndef FAKE_NDS_ARM9_VIDEO_H
#define FAKE_NDS_ARM9_VIDEO_H

#ifndef FAKE_NDS_ARM9_VIDEO_C
extern unsigned short fake_master_bright_main;
extern unsigned short fake_master_bright_sub;
#endif

#define REG_MASTER_BRIGHT     (fake_master_bright_main)
#define REG_MASTER_BRIGHT_SUB (fake_master_bright_sub)

#endif
"""


def run_build(name, compiler, workdir, csrc, sources, extra_flags, log):
    exe = os.path.join(workdir, name + ".exe")
    build = subprocess.run(
        [compiler, "-fmax-errors=8", "-O1"] + extra_flags + INCLUDES
        + ["-o", exe, csrc] + sources,
        capture_output=True, text=True,
    )
    if build.returncode != 0:
        log("build %s FAILED:" % name)
        log(build.stdout)
        log(build.stderr[:4000])
        return None
    return subprocess.run([exe], capture_output=True, text=True)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--verbose", action="store_true")
    args = parser.parse_args()

    lines = []

    def flush_log():
        for text in lines[-40:]:
            sys.stdout.write(text + "\n")

    log = lines.append

    compiler = None
    for candidate in ("gcc", "cc", "clang"):
        found = shutil.which(candidate)
        if found:
            compiler = found
            break
    if compiler is None:
        sys.exit("FAIL: no host C compiler (gcc/cc/clang) on PATH.")

    for path in (SRC_BLACKOUT, SRC_SYS_VIDEO):
        if not os.path.isfile(path):
            sys.exit("FAIL: missing seam source: %s" % path)

    workdir = tempfile.mkdtemp(prefix="blackout-seam-")
    try:
        # Build A: shared-seam logic against both real TUs.
        harness_a = os.path.join(workdir, "harness_a.c")
        with open(harness_a, "w", encoding="utf-8") as handle:
            handle.write(HARNESS_A)
        run_a = run_build(
            "a", compiler, workdir, harness_a,
            [SRC_SYS_VIDEO, SRC_BLACKOUT],
            # The decomp TU expects NULL from the platform's include set.
            ["-DNULL=((void*)0)"], log,
        )
        if run_a is None:
            flush_log()
            print("RED: build A (shared seam) failed to compile.")
            return 1

        # Build B: ARM9 write path through a fake libnds register header.
        fakeinc = os.path.join(workdir, "fakeinc", "nds", "arm9")
        os.makedirs(fakeinc)
        with open(os.path.join(fakeinc, "video.h"), "w",
                  encoding="utf-8") as handle:
            handle.write(FAKE_NDS_VIDEO_H)
        harness_b = os.path.join(workdir, "harness_b.c")
        with open(harness_b, "w", encoding="utf-8") as handle:
            handle.write("#define FAKE_NDS_ARM9_VIDEO_C\n")
            handle.write(HARNESS_B)
        run_b = run_build(
            "b", compiler, workdir, harness_b,
            [SRC_BLACKOUT],
            ["-DARM9", "-I" + os.path.join(workdir, "fakeinc")], log,
        )
        if run_b is None:
            flush_log()
            print("RED: build B (ARM9 fake registers) failed to compile.")
            return 1
    finally:
        shutil.rmtree(workdir, ignore_errors=True)

    print("DS video blackout seam -- real TUs, host compiled")
    for run in (run_a, run_b):
        print("  " + run.stdout.strip().replace("\n", "\n  "))
        if run.returncode != 0:
            print("RED: %s" % run.stdout.strip().splitlines()[0])
            return 1
    print("GREEN: latch semantics, source-flag precedence, setup mirroring, "
          "and the deferred VBlank commit all hold.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
