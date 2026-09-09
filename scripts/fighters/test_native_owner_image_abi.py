#!/usr/bin/env python3
"""Host test for the actual native owner-image loader ABI-v3 guard.

Covers src/nds/nds_renderer_assets.c::ndsRendererNativeEnsureOwnerImage and
its BindOwnerImage seam: valid tag binds, wrong/missing tag rejects with zero
binds even when the payload length matches (stale untagged v1 discriminator),
short reads reject, exact-read semantics (read-size not file-size), current
generation reuse does no payload I/O, stale generation reloads, and a failed
reload never exposes the stale owner. Also validates the 46 generated payloads
and their shared header prefix independently from the generator module.

Strategy: extract the real EnsureOwnerImage body from the C file and compile
it on the host with mocked stream/arena/bind APIs. No make, ROM, emulator, or
shared generated writes; all build artifacts live in a temp dir. Logs bounded.
"""
from __future__ import annotations

import os
import re
import shutil
import struct
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

HERE = Path(__file__).resolve().parent
def _repo() -> Path:
    for p in (HERE, *HERE.parents):
        if (p / "src" / "nds" / "nds_renderer_assets.c").is_file():
            return p
    return HERE.parents[1]
REPO = _repo()
LOADER_C = REPO / "src" / "nds" / "nds_renderer_assets.c"
HEADER_H = REPO / "include" / "nds" / "generated" / "nds_native_fighter_image.generated.h"
IMAGE_GLOB = "nds_native_fighter_*.image.c"
IMAGE_DIR = REPO / "src" / "nds" / "generated"
EXPECTED_TAG = 0x334F444E
EXPECT_BYTES = 64
LOG_CAP = 8000


def _read(p: Path) -> str:
    return p.read_text(encoding="utf-8")


def loader_tag() -> int:
    m = re.search(r"#define\s+NDS_NATIVE_OWNER_IMAGE_ABI_TAG\s+(0x[0-9a-fA-F]+)u?",
                  _read(LOADER_C))
    assert m, "loader ABI tag define missing"
    return int(m.group(1), 16)


def extract_loader_fn() -> str:
    src = _read(LOADER_C)
    key = "s32 ndsRendererNativeEnsureOwnerImage(u32 owner_slot, u32 use_low_detail)"
    i = src.find(key)
    assert i >= 0, "EnsureOwnerImage not found"
    brace = src.find("{", i)
    assert brace > 0
    depth = 0
    for j in range(brace, len(src)):
        if src[j] == "{":
            depth += 1
        elif src[j] == "}":
            depth -= 1
            if depth == 0:
                fn = src[i:j + 1]
                break
    else:
        raise AssertionError("unbalanced braces in loader")
    assert "*(const u32 *)buffer != (u32)NDS_NATIVE_OWNER_IMAGE_ABI_TAG" in fn, \
        "tag guard missing from loader"
    assert "ndsRendererNativeBindOwnerImage(owner_slot, use_low_detail, buffer)" in fn, \
        "bind-after-tag order missing from loader"
    assert "heap_generation == gNdsTaskmanHeapGeneration" in fn, \
        "generation reuse guard missing from loader"
    return fn


def strip_tag_guard(fn: str) -> str:
    pat = re.compile(
        r"/\* Same-size stale images.*?\n.*?\*/\s*"
        r"if\s*\(\s*\*\(const u32 \*\)buffer\s*!=\s*\(u32\)NDS_NATIVE_OWNER_IMAGE_ABI_TAG\s*\)"
        r"\s*\{[^}]*return FALSE;[^}]*\}", re.DOTALL)
    stripped, n = pat.subn("/* negative control: tag guard removed */", fn, count=1)
    if n == 0:  # fallback: neutralize just the condition
        stripped = fn.replace(
            "*(const u32 *)buffer != (u32)NDS_NATIVE_OWNER_IMAGE_ABI_TAG",
            "0 /* negative control: guard removed */")
    assert stripped != fn, "could not strip tag guard for negative control"
    assert "NDS_NATIVE_OWNER_IMAGE_ABI_TAG" not in stripped.split(
        "negative control")[1] or "0 /* negative" in stripped, \
        "guard strip failed"
    return stripped


HARNESS_PREAMBLE = r"""
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
typedef uint32_t u32; typedef int32_t s32;
#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif
#ifndef NULL
#define NULL ((void*)0)
#endif
#define NDS_NATIVE_IMAGE_OWNER_SLOTS 1u
#define NDS_NATIVE_IMAGE_DETAILS 2u
#ifndef NDS_NATIVE_OWNER_IMAGE_ABI_TAG
#define NDS_NATIVE_OWNER_IMAGE_ABI_TAG __ABI_TAG__u
#endif
typedef struct NdsRelocAssetStream { void *file; } NdsRelocAssetStream;
typedef struct NDSNativeOwnerImageSlot {
    const void *base; u32 heap_generation; u32 bytes;
} NDSNativeOwnerImageSlot;
static NDSNativeOwnerImageSlot sNdsNativeOwnerImage[1][2];
static u32 gNdsTaskmanHeapGeneration = 1u;
static volatile u32 gNdsNativeOwnerImageLoadCount;
static volatile u32 gNdsNativeOwnerImageFailCount;
static volatile u32 gNdsNativeOwnerImageBytes;
/* mock controls */
static uint8_t mock_file[256]; static u32 mock_file_len;
static int mock_open_fail; static int mock_malloc_fail;
static u32 mock_bytes = __EXPECT_BYTES__u;
static u32 mock_open_count, mock_read_count, mock_close_count, mock_malloc_count;
static u32 bind_count; static const void *last_bind_base;
static void *allocs[64]; static u32 nallocs;
void *syTaskmanMalloc(size_t size, u32 align) {
    (void)align; mock_malloc_count++;
    if (mock_malloc_fail) return NULL;
    if (nallocs < 64) { allocs[nallocs++] = malloc(size ? size : 1); return allocs[nallocs-1]; }
    return malloc(size ? size : 1);
}
s32 ndsRelocAssetStreamOpen(NdsRelocAssetStream *s, const char *p) {
    mock_open_count++;
    if (!s || !p || mock_open_fail) return FALSE;
    s->file = (void*)0x11; return TRUE;
}
s32 ndsRelocAssetStreamRead(NdsRelocAssetStream *s, u32 off, void *dst, u32 n) {
    mock_read_count++;
    if (!s || !s->file || !dst || !n) return FALSE;
    if (off + n > mock_file_len) return FALSE; /* exact-read: short file fails */
    memcpy(dst, mock_file + off, n); return TRUE;
}
void ndsRelocAssetStreamClose(NdsRelocAssetStream *s) {
    mock_close_count++; if (s) s->file = NULL;
}
static const char *ndsRendererNativeOwnerImagePath(u32 slot, u32 det) {
    (void)slot; (void)det; return "mock:/owner.bin";
}
static u32 ndsRendererNativeOwnerImageBytes(u32 slot, u32 det) {
    (void)slot; (void)det; return mock_bytes;
}
static void ndsRendererNativeBindOwnerImage(u32 slot, u32 det, const void *base) {
    (void)slot; (void)det; bind_count++; last_bind_base = base;
}
s32 ndsRendererNativeEnsureOwnerImage(u32 owner_slot, u32 use_low_detail);
"""

HARNESS_MAIN = r"""
static void set_file_u32_first(u32 first, u32 total, u32 fill) {
    memcpy(mock_file, &first, 4);
    memset(mock_file + 4, (int)fill, total > 4 ? total - 4 : 0);
    mock_file_len = total;
}
static void reset_state(u32 gen) {
    memset(sNdsNativeOwnerImage, 0, sizeof(sNdsNativeOwnerImage));
    gNdsTaskmanHeapGeneration = gen;
    bind_count = 0; last_bind_base = NULL;
    mock_open_count = mock_read_count = mock_close_count = mock_malloc_count = 0;
    mock_open_fail = 0; mock_malloc_fail = 0; mock_bytes = __EXPECT_BYTES__u;
    gNdsNativeOwnerImageLoadCount = gNdsNativeOwnerImageFailCount = gNdsNativeOwnerImageBytes = 0;
}
static int fails = 0;
#define CHECK(name_, cond_) do { \
    printf("CASE %s: %s\n", name_, (cond_) ? "PASS" : "FAIL"); \
    if (!(cond_)) fails++; } while (0)
int main(void) {
    s32 r; u32 o0, r0, m0, b0; const void *base0;
    /* 1 valid tag binds */
    reset_state(1);
    { u32 tag = (u32)NDS_NATIVE_OWNER_IMAGE_ABI_TAG; set_file_u32_first(tag, __EXPECT_BYTES__, 0xA5); }
    r = ndsRendererNativeEnsureOwnerImage(0u, 0u);
    CHECK("valid_tag_bind", r == TRUE && bind_count == 1 && sNdsNativeOwnerImage[0][0].base != NULL
        && sNdsNativeOwnerImage[0][0].heap_generation == 1u
        && *(const u32*)sNdsNativeOwnerImage[0][0].base == (u32)NDS_NATIVE_OWNER_IMAGE_ABI_TAG);
    /* 2 wrong tag, same length: reject before bind */
    reset_state(2);
    set_file_u32_first(0xDEADBEEFu, __EXPECT_BYTES__, 0xA5);
    r = ndsRendererNativeEnsureOwnerImage(0u, 0u);
    CHECK("wrong_tag_reject", r == FALSE && bind_count == 0 && sNdsNativeOwnerImage[0][0].base == NULL);
    /* 3 missing (zero) tag, same length */
    reset_state(3);
    set_file_u32_first(0u, __EXPECT_BYTES__, 0xA5);
    r = ndsRendererNativeEnsureOwnerImage(0u, 0u);
    CHECK("zero_tag_reject", r == FALSE && bind_count == 0 && sNdsNativeOwnerImage[0][0].base == NULL);
    /* 4 short read */
    reset_state(4);
    { u32 tag = (u32)NDS_NATIVE_OWNER_IMAGE_ABI_TAG; set_file_u32_first(tag, __EXPECT_BYTES__ - 1u, 0xA5); }
    r = ndsRendererNativeEnsureOwnerImage(0u, 0u);
    CHECK("short_read_reject", r == FALSE && bind_count == 0 && sNdsNativeOwnerImage[0][0].base == NULL);
    /* 5 long file: exact-read binds (read-size, not file-size) */
    reset_state(5);
    { u32 tag = (u32)NDS_NATIVE_OWNER_IMAGE_ABI_TAG; set_file_u32_first(tag, __EXPECT_BYTES__ + 64u, 0xA5); }
    r = ndsRendererNativeEnsureOwnerImage(0u, 0u);
    CHECK("long_file_binds_read_size", r == TRUE && bind_count == 1);
    printf("NOTE size_check_is_read_size_not_file_size: trailing_bytes_ignored=1\n");
    /* 6 reuse current generation: no payload I/O, no rebind */
    o0 = mock_open_count; r0 = mock_read_count; m0 = mock_malloc_count; b0 = bind_count;
    r = ndsRendererNativeEnsureOwnerImage(0u, 0u);
    CHECK("reuse_no_io", r == TRUE && mock_open_count == o0 && mock_read_count == r0
        && mock_malloc_count == m0 && bind_count == b0);
    /* 7 stale generation reloads */
    base0 = sNdsNativeOwnerImage[0][0].base;
    gNdsTaskmanHeapGeneration = 6u;
    { u32 tag = (u32)NDS_NATIVE_OWNER_IMAGE_ABI_TAG; set_file_u32_first(tag, __EXPECT_BYTES__, 0x5A); }
    o0 = mock_open_count; b0 = bind_count;
    r = ndsRendererNativeEnsureOwnerImage(0u, 0u);
    CHECK("stale_reload", r == TRUE && bind_count == b0 + 1 && mock_open_count == o0 + 1
        && sNdsNativeOwnerImage[0][0].heap_generation == 6u
        && sNdsNativeOwnerImage[0][0].base != base0);
    /* 8 failed reload does not expose stale owner */
    base0 = sNdsNativeOwnerImage[0][0].base; b0 = bind_count;
    gNdsTaskmanHeapGeneration = 7u;
    set_file_u32_first(0xDEADBEEFu, __EXPECT_BYTES__, 0xA5);
    o0 = mock_open_count;
    r = ndsRendererNativeEnsureOwnerImage(0u, 0u);
    CHECK("failed_reload_hidden", r == FALSE && bind_count == b0
        && sNdsNativeOwnerImage[0][0].base == base0
        && sNdsNativeOwnerImage[0][0].heap_generation == 6u);
    r = ndsRendererNativeEnsureOwnerImage(0u, 0u); /* still stale: must retry I/O, not claim resident */
    CHECK("stale_still_retries", r == FALSE && mock_open_count == o0 + 2 && bind_count == b0);
    { u32 tag = (u32)NDS_NATIVE_OWNER_IMAGE_ABI_TAG; set_file_u32_first(tag, __EXPECT_BYTES__, 0x5A); }
    r = ndsRendererNativeEnsureOwnerImage(0u, 0u);
    CHECK("recover_after_fail", r == TRUE && bind_count == b0 + 1
        && sNdsNativeOwnerImage[0][0].heap_generation == 7u);
#ifdef NEGATIVE_CONTROL_ONLY_WRONG_TAG
    (void)o0; (void)r0; (void)m0;
#else
    (void)0;
#endif
    printf("DONE fails=%d\n", fails);
    return fails ? 1 : 0;
}
"""

NEG_CONTROL_MAIN = r"""
static int fails = 0;
int main(void) {
    memset(sNdsNativeOwnerImage, 0, sizeof(sNdsNativeOwnerImage));
    gNdsTaskmanHeapGeneration = 9u;
    { u32 bad = 0xDEADBEEFu; memcpy(mock_file, &bad, 4);
      memset(mock_file + 4, 0xA5, __EXPECT_BYTES__ - 4); mock_file_len = __EXPECT_BYTES__; }
    s32 r = ndsRendererNativeEnsureOwnerImage(0u, 0u);
    printf("CONTROL no_guard_wrong_tag: result=%d binds=%u\n", (int)r, bind_count);
    printf("DONE fails=%d\n", (r == TRUE && bind_count == 1) ? 0 : 1);
    return (r == TRUE && bind_count == 1) ? 0 : 1;
}
"""


def _compile_and_run(fn_text: str, main_text: str, tag: int, extra_cflags=()) -> str:
    tmp = Path(tempfile.mkdtemp(prefix="owner_abi_"))
    try:
        pre = HARNESS_PREAMBLE.replace("__ABI_TAG__",
                                       f"0x{tag:08x}").replace("__EXPECT_BYTES__",
                                                                str(EXPECT_BYTES))
        main = main_text.replace("__EXPECT_BYTES__", str(EXPECT_BYTES))
        c_path = tmp / "harness.c"
        c_path.write_text(pre + "\n" + fn_text + "\n" + main + "\n",
                          encoding="utf-8")
        exe = tmp / ("harness.exe" if os.name == "nt" else "harness")
        cc = shutil.which("gcc") or shutil.which("cc") or shutil.which("clang")
        assert cc, "no host C compiler found"
        cmd = [cc, "-std=c99", "-O1", "-w", str(c_path), "-o", str(exe),
               *extra_cflags]
        p = subprocess.run(cmd, capture_output=True, text=True, timeout=120)
        assert p.returncode == 0, f"host compile failed: {(p.stderr or '')[:2000]}"
        q = subprocess.run([str(exe)], capture_output=True, text=True,
                           timeout=60)
        out = (q.stdout or "")[:LOG_CAP]
        assert q.returncode == 0, f"host harness failed:\n{out}"
        assert "DONE fails=0" in out, f"harness reports failures:\n{out}"
        return out
    finally:
        shutil.rmtree(tmp, ignore_errors=True)


def check_schema(tag: int) -> str:
    h = _read(HEADER_H)
    m = re.search(r"#define\s+NDS_NATIVE_OWNER_IMAGE_ABI_TAG\s+(0x[0-9a-fA-F]+)u?",
                  h)
    assert m and int(m.group(1), 16) == tag, "header tag disagrees with loader"
    structs = re.findall(r"typedef struct (\w+)\s*\{(.*?)\}\s*\w+\s*;", h, re.DOTALL)
    imgs = [s for s in structs if s[0].endswith("Image")]
    assert len(imgs) == 46, f"expected 46 image structs, saw {len(imgs)}"
    for name, body in imgs:
        first = [ln.strip() for ln in body.strip().splitlines()
                 if ln.strip()][0]
        assert first == "u32 abi_tag[1];", f"{name} prefix is not abi_tag: {first}"
        assert "u16 packed_corners[" in body, f"{name} lacks integrated u16 corners"
    files = sorted(IMAGE_DIR.glob(IMAGE_GLOB))
    assert len(files) == 46, f"expected 46 image payloads, saw {len(files)}"
    for f in files:
        t = _read(f)
        mt = re.search(r"\.abi_tag\s*=\s*\{\s*(0x[0-9a-fA-F]+)u?\s*\}", t)
        assert mt, f"{f.name} missing abi_tag initializer"
        assert int(mt.group(1), 16) == tag, f"{f.name} wrong tag"
        assert t.find(".abi_tag") < t.find(".state_deltas"), \
            f"{f.name} abi_tag not first initializer"
        assert ".fighter_image" in t, f"{f.name} missing image section"
    # integrated dense11+matrix5 same-u16: corners span both fields in one u16
    sample = _read(files[0])
    vals = [int(x, 16) for x in
            re.findall(r"0x([0-9a-fA-F]+)u",
                       sample.split(".packed_corners")[1].split("},")[0])]
    mx = max(vals)
    assert all(v <= 0xFFFF for v in vals) and mx > 0x07FF, \
        "packed corners do not show integrated 11+5 use"
    return f"schema images=46 header_structs=46 tag=0x{tag:08X} sample_max={hex(mx)}"


class OwnerImageAbiTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.tag = loader_tag()
        assert cls.tag == EXPECTED_TAG, \
            f"loader tag 0x{cls.tag:08X} != contract 0x{EXPECTED_TAG:08X}"
        cls.fn = extract_loader_fn()

    def test_schema_prefix_independent(self):
        note = check_schema(self.tag)
        print(note[:200])

    def test_loader_behavior_and_lifetime(self):
        out = _compile_and_run(self.fn, HARNESS_MAIN, self.tag)
        for case in ("valid_tag_bind", "wrong_tag_reject", "zero_tag_reject",
                     "short_read_reject", "long_file_binds_read_size",
                     "reuse_no_io", "stale_reload", "failed_reload_hidden",
                     "stale_still_retries", "recover_after_fail"):
            self.assertIn(f"CASE {case}: PASS", out)
        self.assertIn("NOTE size_check_is_read_size_not_file_size", out)

    def test_negative_control_guard_removed_accepts_wrong_tag(self):
        stripped = strip_tag_guard(self.fn)
        out = _compile_and_run(stripped, NEG_CONTROL_MAIN, self.tag)
        self.assertIn("CONTROL no_guard_wrong_tag", out)


if __name__ == "__main__":
    unittest.main(verbosity=2)
