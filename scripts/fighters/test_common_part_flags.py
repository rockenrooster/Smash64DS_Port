"""Host tests for ndsRelocNormalizeFighterCommonPartFlags (actual C)."""
import shutil
import struct
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts" / "menus"))
from source_test_helpers import function

SOURCE = ROOT / "src" / "port" / "reloc_backend_assets.c"
O2R = ROOT / "decomp" / "BattleShip-main" / "BattleShip_o2r" / "reloc_fighters_main" / "YoshiMain"

PRE = r'''#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32; typedef int32_t s32;
#define TRUE 1
#define FALSE 0
#define ARRAY_COUNT(a) (sizeof(a) / sizeof((a)[0]))
typedef struct { u32 w0; u32 w1; u32 w2; u8 flags; u8 pad[3]; } FTCommonPart;
typedef struct { FTCommonPart commonparts[2]; } FTCommonPartContainer;
typedef struct { void *data; u32 data_size; u32 asset_id; u8 format_fixups_applied; } NDSRelocLoadedFile;
static u32 g_fail;
static void ndsRelocRecordExternalFixupFail(u32 id) { (void)id; ++g_fail; }
static u32 ndsRelocReadNative32(const void *a) { u32 v; memcpy(&v, a, 4); return v; }
static s32 ndsRelocRangeInLoadedFile(const NDSRelocLoadedFile *l, uintptr_t off, size_t sz) {
if (!l || !l->data) { return FALSE; } if (off > l->data_size) { return FALSE; }
if (sz > (size_t)(l->data_size - off)) { return FALSE; } return TRUE; }
'''


def yoshi_container_bytes():
    blob = O2R.read_bytes()
    externs = struct.unpack_from("<I", blob, 0x48)[0]
    size_off = 0x4C + externs * 2
    size = struct.unpack_from("<I", blob, size_off)[0]
    payload = blob[size_off + 4:]
    assert len(payload) == size
    raw = payload[0x19C:0x19C + 32]
    assert len(raw) == 32
    assert payload[0x1A8] == 0x01 and payload[0x1B8] == 0x01
    assert raw[12] == 0x01 and raw[28] == 0x01
    return raw


def build_program():
    helper = function(SOURCE.read_text(), "ndsRelocNormalizeFighterCommonPartFlags")
    raw = yoshi_container_bytes()
    vals = ",".join(f"0x{b:02x}" for b in raw)
    body = r'''
static void swap_words(u8 *p, size_t n) { size_t i; for (i = 0; i < n; i += 4) {
u8 a = p[i], b = p[i+1], c = p[i+2], d = p[i+3]; p[i] = d; p[i+1] = c; p[i+2] = b; p[i+3] = a; } }
#define CHECK(x) do { if (!(x)) { printf("fail line %d: %s\n", __LINE__, #x); return 1; } } while (0)
static const u8 kRaw[32] = {''' + vals + r'''};
int main(void) {
CHECK(sizeof(FTCommonPartContainer) == 32u); CHECK(sizeof(FTCommonPart) == 16u);
static u8 file[64]; NDSRelocLoadedFile f = { file, sizeof(file), 0xF7u, FALSE };
FTCommonPartContainer *c = (FTCommonPartContainer *)(file + 16); u8 snap[64]; u32 w[6]; int v;
memcpy(c, kRaw, 32); swap_words((u8 *)c, 32);
memcpy(w, c, 12); memcpy(w + 3, (u8 *)c + 16, 12);
g_fail = 0; CHECK(ndsRelocNormalizeFighterCommonPartFlags(&f, c) == TRUE);
CHECK(c->commonparts[0].flags == 0x01 && c->commonparts[1].flags == 0x01);
CHECK(!memcmp(&c->commonparts[0], w, 12) && !memcmp(&c->commonparts[1], w + 3, 12));
CHECK(g_fail == 0u);
for (v = 0; v < 256; ++v) { memcpy(c, kRaw, 32); c->commonparts[0].flags = (u8)v;
c->commonparts[1].flags = (u8)(255 - v); swap_words((u8 *)c, 32);
memcpy(w, c, 12); memcpy(w + 3, (u8 *)c + 16, 12);
CHECK(ndsRelocNormalizeFighterCommonPartFlags(&f, c) == TRUE);
CHECK(c->commonparts[0].flags == (u8)v && c->commonparts[1].flags == (u8)(255 - v));
CHECK(!memcmp(&c->commonparts[0], w, 12) && !memcmp(&c->commonparts[1], w + 3, 12)); }
CHECK(ndsRelocNormalizeFighterCommonPartFlags(&f, NULL) == TRUE);
memset(c, 0, 32); swap_words((u8 *)c, 32);
CHECK(ndsRelocNormalizeFighterCommonPartFlags(&f, c) == TRUE);
CHECK(c->commonparts[0].flags == 0 && c->commonparts[1].flags == 0);
memcpy(c, kRaw, 32); memcpy(snap, file, sizeof(file)); g_fail = 0;
CHECK(ndsRelocNormalizeFighterCommonPartFlags(&f, (FTCommonPartContainer *)(file + 64)) == FALSE);
CHECK(g_fail == 1u && !memcmp(snap, file, sizeof(file)));
f.data_size = 16u + 31u; memcpy(snap, file, sizeof(file)); g_fail = 0;
CHECK(ndsRelocNormalizeFighterCommonPartFlags(&f, c) == FALSE);
CHECK(g_fail == 1u && !memcmp(snap, file, sizeof(file)));
printf("ok flags=01 words_kept sweep=256 null_ok oob_ok trunc_ok\n"); return 0; }
'''
    return PRE + helper + body


def test_common_part_flags_against_actual_c():
    cc = shutil.which("gcc") or shutil.which("clang")
    assert cc is not None
    with tempfile.TemporaryDirectory() as tmp:
        d = Path(tmp)
        src, exe = d / "flags.c", d / "flags.exe"
        src.write_text(build_program())
        done = subprocess.run([cc, "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
                               str(src), "-o", str(exe)], capture_output=True, text=True)
        assert done.returncode == 0, done.stderr
        done = subprocess.run([str(exe)], capture_output=True, text=True)
        assert done.returncode == 0, done.stdout + done.stderr
        assert "sweep=256" in done.stdout


def test_caller_runs_once_before_flag_set():
    body = function(SOURCE.read_text(), "ndsRelocNormalizeFighterAttributesFile")
    gate = body.index("if (loaded->format_fixups_applied == FALSE)")
    call = body.index("ndsRelocNormalizeFighterCommonPartFlags")
    done = body.index("loaded->format_fixups_applied = TRUE;")
    assert gate < call < done
