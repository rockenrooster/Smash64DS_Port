"""Execute the actual admission/binding C with storage and DS services mocked.

The image ABI/geometry has separate producer tests. These fixtures exercise the
new lifetime boundary, shared mutable-table identity, and source copy-set rule.
"""
from pathlib import Path
import re
import shutil
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[2]


def function(source, name):
    match = re.search(r"^.*\b" + name + r"\([^;]*?\)\s*\{", source, re.M)
    assert match, name
    start = match.start()
    brace = source.index("{", match.start())
    depth = 1
    end = brace + 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[start:end]


def run_c(source):
    compiler = next((shutil.which(c) for c in ("gcc", "clang", "cc")
                     if shutil.which(c)), None)
    assert compiler, "A host C compiler is required"
    with tempfile.TemporaryDirectory(prefix="kirby-match-", dir=ROOT / "builds") as tmp:
        cpath = Path(tmp) / "fixture.c"
        binary = Path(tmp) / "fixture.exe"
        cpath.write_text(source)
        result = subprocess.run([compiler, "-std=c11", "-O2", "-Wall", "-Werror",
                                 str(cpath), "-o", str(binary)],
                                capture_output=True, text=True)
        assert result.returncode == 0, result.stderr
        result = subprocess.run([str(binary)], capture_output=True, text=True)
        assert result.returncode == 0, result.stdout + result.stderr


COMMON = r'''
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
typedef uint8_t u8;
typedef uint32_t u32;
typedef int32_t s32;
enum { FALSE = 0, TRUE = 1 };
'''


def test_match_bank_sharing_failure_and_scene_generation():
    source = (ROOT / "src/nds/nds_renderer_assets.c").read_text()
    start = source.index("typedef struct NDSNativeKirbyHatImageSlot")
    declarations = source[start:source.index("\n#endif", start)]
    fixture = COMMON + r'''
#define NDS_NATIVE_KIRBY_HAT_BATTLE_SLOTS 4
#define NDS_NATIVE_IMAGE_DETAILS 2
#define NDS_NATIVE_KIRBY_HAT_MIN_MODELPART 3
#define NDS_NATIVE_KIRBY_HAT_MAX_MODELPART 13
#define NDS_NATIVE_KIRBY_HAT_MAX_HIGH_BYTES 96
#define NDS_NATIVE_KIRBY_HAT_MAX_LOW_BYTES 64
#define NDS_NATIVE_KIRBY_HAT_MAX_BYTES 96
#define NDS_NATIVE_OWNER_IMAGE_ABI_TAG 0x1234abcd
typedef struct { u32 *prepared_dense; } NDSNativeFighterRuntimeTables;
typedef struct { u32 root_offset; } NDSNativeRoot;
typedef struct { unsigned id; } NdsRelocAssetStream;
static u32 gNdsTaskmanHeapGeneration = 1;
''' + declarations + r'''
static _Alignas(16) u8 arena[32768];
static unsigned cursor, allocations, opens, reads, closes, bindings;
static unsigned io_disabled, reject_alloc, reject_open, reject_read, bad_abi, bad_bind;
static char paths[28][1];
static void *ndsSceneAssetTryAlloc(size_t bytes, u32 alignment, size_t keep) {
    assert(!io_disabled && alignment == 16 && keep == 25600);
    if (reject_alloc) return NULL;
    assert(cursor + bytes <= sizeof(arena));
    void *p = arena + cursor;
    cursor += bytes;
    ++allocations;
    return p;
}
static void *ndsBattleIdleScratchAlloc(size_t bytes, u32 alignment) {
    return ndsSceneAssetTryAlloc((bytes + 15) & ~15u, alignment, 25600);
}
static void *syTaskmanMalloc(size_t bytes, u32 alignment) {
    return ndsBattleIdleScratchAlloc(bytes, alignment);
}
static const char *ndsRendererNativeKirbyHatImagePath(u32 part, u32 detail) {
    return paths[part * 2 + detail];
}
static u32 ndsRendererNativeKirbyHatImageBytes(u32 part, u32 detail) {
    (void)part;
    return detail ? 60 : 92;
}
static int ndsRelocAssetStreamOpen(NdsRelocAssetStream *s, const char *path) {
    assert(!io_disabled);
    ++opens;
    s->id = (unsigned)(path - paths[0]);
    return !reject_open;
}
static int ndsRelocAssetStreamRead(NdsRelocAssetStream *s, u32 off, void *p, u32 bytes) {
    assert(!io_disabled && off == 0 && s->id < 28);
    ++reads;
    memset(p, 0, bytes);
    *(u32 *)p = bad_abi ? 0 : NDS_NATIVE_OWNER_IMAGE_ABI_TAG;
    return !reject_read;
}
static void ndsRelocAssetStreamClose(NdsRelocAssetStream *s) { (void)s; ++closes; }
static s32 ndsRendererNativeBindKirbyHatImage(NDSNativeKirbyHatImageSlot *entry,
                                            u32 part, u32 detail, const void *base) {
    (void)detail;
    ++bindings;
    entry->tables.prepared_dense = (u32 *)base + 1;
    entry->root.root_offset = part * 16;
    return !bad_bind;
}
''' + function(source, "ndsRendererNativePrepareKirbyHatMatch") + "\n" + function(
        source, "ndsRendererNativeEnsureKirbyCopyHat") + r'''
static void next_scene(void) {
    ++gNdsTaskmanHeapGeneration;
    cursor = 0;
    io_disabled = reject_alloc = reject_open = reject_read = bad_abi = bad_bind = 0;
    ndsRendererNativeBeginKirbyHatMatch();
}
int main(void) {
    u32 mask = (1u << 6) | (1u << 9) | (1u << 10);
    assert(!ndsRendererNativePrepareKirbyHatMatch(mask, mask)); // begin required
    ndsRendererNativeBeginKirbyHatMatch();
    assert(!ndsRendererNativePrepareKirbyHatMatch(1, 0));
    assert(!ndsRendererNativePrepareKirbyHatMatch(mask, 1u << 7));
    assert(ndsRendererNativePrepareKirbyHatMatch(mask, mask));
    assert(opens == 6 && reads == 6 && closes == 6 && allocations == 1);
    assert(sNdsNativeKirbyHatMatchCount == 6 && bindings == 6);
    assert(cursor == ((6 * sizeof(NDSNativeKirbyHatImageSlot) + 15) & ~15u) + 3 * (96 + 64));
    assert(cursor == gNdsNativeKirbyHatMatchRequiredBytes);
    io_disabled = 1;
    assert(ndsRendererNativePrepareKirbyHatMatch(mask, mask)); // no second allocation
    assert(!ndsRendererNativePrepareKirbyHatMatch(mask | (1u << 7), mask));
    for (u32 slot = 0; slot < 4; ++slot) {
        for (u32 detail = 0; detail < 2; ++detail) {
            assert(ndsRendererNativeEnsureKirbyCopyHat(slot, 6, detail));
            // Same mutable image MUST have the same table identity for UV stamps.
            assert(ndsKirbyHatSlot(slot, detail) == ndsKirbyHatSlot(0, detail));
        }
    }
    assert(ndsKirbyHatSlot(0, 0) != ndsKirbyHatSlot(0, 1));
    assert(ndsRendererNativeEnsureKirbyCopyHat(0, 10, 0));
    assert(ndsKirbyHatSlot(0, 0) != ndsKirbyHatSlot(1, 0));
    assert(!ndsRendererNativeEnsureKirbyCopyHat(0, 7, 0)); // no lazy load
    assert(!ndsRendererNativeEnsureKirbyCopyHat(4, 6, 0));
    assert(!ndsRendererNativeEnsureKirbyCopyHat(0, 6, 2));
    assert(opens == 6 && allocations == 1);
    // A following menu cannot return an arena pointer from the retired match.
    ++gNdsTaskmanHeapGeneration;
    memset(arena, 0xdd, sizeof(arena));
    assert(ndsKirbyHatSlot(0, 0) == &sNdsNativeKirbyHatWorkingImages[0][0]);
    io_disabled = 0;
    cursor = 0;
    assert(ndsRendererNativeEnsureKirbyCopyHat(0, 7, 0)); // bounded preview path
    assert(ndsKirbyHatSlot(0, 0)->valid);
    next_scene();
    assert(!ndsRendererNativeEnsureKirbyCopyHat(0, 7, 0)); // not admitted yet
    assert(ndsRendererNativePrepareKirbyHatMatch(0, 0));
    assert(sNdsNativeKirbyHatMatchCount == 0 && cursor == 0);
    assert(!ndsRendererNativeEnsureKirbyCopyHat(0, 7, 0));
    next_scene();
    assert(ndsRendererNativePrepareKirbyHatMatch(1u << 7, 0));
    assert(ndsRendererNativeEnsureKirbyCopyHat(0, 7, 0));
    assert(!ndsRendererNativeEnsureKirbyCopyHat(0, 7, 1));
    // Allocation/read/corrupt-image failures publish no partially ready bank.
    for (unsigned failure = 0; failure < 5; ++failure) {
        next_scene();
        switch (failure) {
        case 0: reject_alloc = 1; break;
        case 1: reject_open = 1; break;
        case 2: reject_read = 1; break;
        case 3: bad_abi = 1; break;
        case 4: bad_bind = 1; break;
        }
        assert(!ndsRendererNativePrepareKirbyHatMatch(mask, mask));
        assert(!sNdsNativeKirbyHatMatchReady && !sNdsNativeKirbyHatMatchCount);
        assert(!ndsRendererNativeEnsureKirbyCopyHat(0, 6, 0));
        unsigned before = allocations;
        assert(!ndsRendererNativePrepareKirbyHatMatch(mask, mask));
        assert(allocations == before);
    }
    return 0;
}
'''
    run_c(fixture)


def test_source_roster_copy_closure():
    source = (ROOT / "src/port/nds_kirby_hat_residency.c").read_text()
    fixture = COMMON + r'''
#include <setjmp.h>
#define NDS_P2_KIRBY 1
enum { GMCOMMON_PLAYERS_MAX = 4, nFTPlayerKindNot = 2, nFTKindKirby = 8,
       nFTKindEnumCount = 27 };
typedef struct { int16_t copy_id, copy_modelpart_id; float scale; int damage; } FTKirbyCopy;
static FTKirbyCopy copy_table[27];
static void *gFTDataKirbyMainMotion = copy_table;
#define lbRelocGetFileData(type, file, symbol) ((type)(file))
static struct { struct { int pkind, fkind; } players[4]; int pl_count, cp_count; } battle;
#define gSCManagerBattleState (&battle)
static u32 gNdsKirbyHatRequiredHighMask, gNdsKirbyHatRequiredLowMask;
static jmp_buf failure;
static unsigned reason, admitted_high, admitted_low, refuse;
static void ndsKirbyHatResidencyHalt(u32 value) __attribute__((noreturn));
static void ndsKirbyHatResidencyHalt(u32 value) { reason = value; longjmp(failure, 1); }
static int ndsRendererNativePrepareKirbyHatMatch(u32 high, u32 low) {
    admitted_high = high; admitted_low = low; return !refuse;
}
''' + function(source, "ndsKirbyHatPrepareMatch") + r'''
int main(void) {
    const int parts[27] = {12,7,4,8,11,10,5,9,0,6,3,13,0,0,0,0,0,0,0,0,0,0,0,0,0,0,4};
    for (unsigned i = 0; i < 27; ++i) {
        copy_table[i].copy_id = (i < 12) ? i : ((i == 26) ? 2 : 8);
        copy_table[i].copy_modelpart_id = parts[i];
    }
    battle.cp_count = 4;
    // Every ordered four-slot roster, including mirrors and multiple Kirbys.
    for (unsigned roster = 0; roster < 12*12*12*12; ++roster) {
        unsigned r = roster, mask = 0, kirby = 0;
        for (unsigned slot = 0; slot < 4; ++slot) {
            unsigned kind = r % 12; r /= 12;
            battle.players[slot].fkind = kind;
            kirby |= kind == 8;
            if (kind != 8) mask |= 1u << parts[kind];
        }
        ndsKirbyHatPrepareMatch();
        assert(admitted_high == (kirby ? mask : 0));
        assert(admitted_low == admitted_high);
    }
    battle.cp_count = 2;
    battle.players[0].fkind = 8;
    battle.players[1].fkind = 1;
    battle.players[2].pkind = battle.players[3].pkind = nFTPlayerKindNot;
    ndsKirbyHatPrepareMatch();
    assert(admitted_high == (1u << 7) && admitted_low == 0);
    battle.players[1].fkind = 26; // Giant DK aliases the normal DK copy row.
    ndsKirbyHatPrepareMatch();
    assert(admitted_high == (1u << 4));
    copy_table[26].copy_modelpart_id = 13; // Donor row is not the acquired hat row.
    ndsKirbyHatPrepareMatch();
    assert(admitted_high == (1u << 4));
    for (unsigned invalid = 0; invalid < 5; ++invalid) {
        battle.players[1].fkind = 1;
        gFTDataKirbyMainMotion = copy_table;
        copy_table[1].copy_id = 1;
        copy_table[1].copy_modelpart_id = 7;
        refuse = 0;
        switch (invalid) {
        case 0: gFTDataKirbyMainMotion = NULL; break;
        case 1: battle.players[1].fkind = 27; break;
        case 2: copy_table[1].copy_id = 27; break;
        case 3: copy_table[1].copy_modelpart_id = 2; break;
        case 4: refuse = 1; break;
        }
        if (setjmp(failure) == 0) { ndsKirbyHatPrepareMatch(); assert(0); }
        assert(reason == ((invalid == 0) ? 1 : (invalid < 3) ? 2 : invalid));
    }
    return 0;
}
'''
    run_c(fixture)
