/* Native renderer, fighter texture VRAM: the battle admission of every texture
 * a fighter can show, its VRAM regions and carved records, and the battle
 * exit release. A slice of nds_renderer.c. */

/* ---------------------------------------------------------------------------
 * P2-2p8 Phase 1 slice 2b: fighter texture admission and battle VRAM regions
 * (include/nds/renderer_fighter_lean.h, gNdsFtrLeanAdmit).
 *
 * The table is scripts/fighters/generate_nds_fighter_admission.py's NitroFS
 * payload: per kind x detail, every texture the fighter can show, as the RDP
 * texture state the draw presents at the textured triangle (image / TLUT as
 * asset id + SOURCE offset, the exact SETTIMG / SETTILE / LOAD* / SETTILESIZE
 * / TEXTURE words, combine, othermode, prim, env). Admission replays each
 * record through the renderer's own recorders into a fresh NDSRendererStats
 * and hands it to the cache's own resolve/convert/upload path, so the key is
 * built by the same code the draw uses; the entry is then marked `admitted`
 * (exempt from eviction and refresh for the battle).
 *
 * Word 2 first takes bank D for textures when the battle leaves BG3 empty
 * (ndsPlatformVramBg3Empty: no opaque BG3 pixel). The admission allocates
 * first-fit, so it packs into what A+B still has free and spills the rest
 * into D; the first frame end after it locks A+B (ndsFtrLeanAdmitLockRegions)
 * and D alone allocates and frees for the rest of the battle. Battle exit
 * releases D's cache entries, locks D and hands it back to BG3.
 * ------------------------------------------------------------------------- */
#include <nds/generated/nds_fighter_admission.generated.h>

#ifndef NDS_FTR_ADMIT_LAB
#define NDS_FTR_ADMIT_LAB(stmt) ((void)0)
#endif

const void *ndsRelocNativeAssetAddress(const void *base, u32 offset);
u32 ndsPlatformVramBg3Empty(void);
s32 ndsPlatformVramTakeBankD(void);
void ndsPlatformVramReturnBankD(void);

/* libnds keeps every texture name's record and every VRAM block split in
 * newlib malloc, from the libc runtime reserve the taskman arena leaves
 * (diagnostics_taskman_heap.c: 0xA000, 0x9100 on the all-content shell), and
 * its block allocator does not check malloc (a NULL store aborts). Admission
 * stops -- the rest stays on-demand, as today -- before the top chunk falls
 * under this floor, which keeps the reserve's measured in-match need. */
#define NDS_FTR_ADMIT_LIBC_FLOOR 0x4000u
typedef struct NDSFtrAdmitMallinfo
{
    u32 arena;
    u32 ordblks;
    u32 smblks;
    u32 hblks;
    u32 hblkhd;
    u32 usmblks;
    u32 fsmblks;
    u32 uordblks;
    u32 fordblks;
    u32 keepcost;
} NDSFtrAdmitMallinfo;
extern NDSFtrAdmitMallinfo mallinfo(void);

/* Dynamic cache slots the admission leaves to the draws' non-admitted
 * same-frame working set: the four-CPU stress measures 55 without admission
 * (gNdsFtrAdmitDynTouchedHigh, slice 2b), so admission may hold at most
 * DYNAMIC_COUNT - 56 entries. */
#define NDS_FTR_ADMIT_SLOT_RESERVE 56u

/* P2-2p8 Phase 1 slice 7: the admission, its VRAM carve and the battle-exit
 * release run while a battle loads or ends, never inside a presented battle
 * frame, so they are size-optimized like the menu translation units (Makefile,
 * `battleship_mn%.o ... CFLAGS += -Os`): every text byte is arena, and
 * making lean the default (with Ness's yo-yo program) had to cost no static
 * RAM. The draw path's own resolve / convert / upload that the admission
 * replays through, and the per-draw carve upload inside it, are untouched. */
#define NDS_FTR_ADMIT_COLD_CODE __attribute__((cold, optimize("Os")))

/* Records read from the NitroFS table per stream read. Slice 7: 8 -> 1
 * (112 B of BSS instead of 896) -- the rest of the static-RAM offset for
 * Ness's yo-yo and forward-smash programs. The four-CPU admission reads
 * 689 records while the battle loads; at 8 a read the table reads took
 * 1.35M of its 25.5M ticks, at 4 1.43M -- the cost is bytes, not calls --
 * and none of it is in a presented frame. */
#define NDS_FTR_ADMIT_CHUNK 1u
#define NDS_FTR_ADMIT_KIRBY 8u
#define NDS_FTR_ADMIT_D_LO 0x06860000u
#define NDS_FTR_ADMIT_D_HI 0x06880000u
#define NDS_FTR_ADMIT_INDEX_WORDS (4u + 4u * NDS_FIGHTER_ADMISSION_KINDS)

volatile u32 gNdsFtrLeanAdmitFail __attribute__((used));
volatile u32 gNdsFtrLeanAdmitFailFirst[4] __attribute__((used));

static u32 sNdsFtrAdmitRegions;
static u32 sNdsFtrAdmitLocked;
static int sNdsFtrAdmitSavedLock;
static u32 sNdsFtrAdmitIndex[NDS_FTR_ADMIT_INDEX_WORDS];
static u32 sNdsFtrAdmitChunk[NDS_FTR_ADMIT_CHUNK *
                             NDS_FIGHTER_ADMISSION_RECORD_WORDS];
static const NDSRendererConfig sNdsFtrAdmitConfig = {
    .texture_data_layout = NDS_RENDERER_TEXTURE_DATA_O2R_WORD_SWAPPED
};

/* ---------------------------------------------------------------------------
 * P2-2p8 Phase 1 slice 2c: carved residency.
 *
 * libnds keeps a malloc'd record for every texture name and palette, and a
 * block node for every VRAM split, in the libc reserve beside the taskman
 * arena; slice 2b's admission hit that reserve's floor after 15 records. The
 * admission now reserves VRAM in a few large blocks straight from libnds's own
 * allocators -- vramBlock_allocateBlock, the search glTexImage2D and
 * glColorTableEXT make, without a texture or palette record -- and carves every
 * admitted texture and palette out of them with a bump pointer, writing the
 * texels through the LCD mapping as glTexImage2D does. libc is touched once
 * per reservation (a handful per battle), never per texture.
 *
 * A carved entry is named NDS_FTR_CARVE_NAME_BASE | slot and keeps its own
 * TEXIMAGE_PARAM (params) and PLTT_BASE (carve_pltt) words. Its bind
 * (ndsRendererHardwareBindCarvedState) writes them and points libnds's active
 * texture and palette at one "carrier" record whose words it rewrites, so
 * glTexParameter, glGetTexParameter and glGetColorTableParameterEXT -- the
 * packet recorder's view of the bind -- see exactly what they would for a
 * real name. A texture region is returned to libnds when the last entry
 * carved in it is released (entries stop being admitted at the battle exit
 * and are then evicted like any other); the palette regions and the carrier
 * go with the last carved entry, or at the next scene reset
 * (ndsFtrLeanAdmitCarveRetire).
 * ------------------------------------------------------------------------- */
u8 *vramBlock_examineSpecial(s_vramBlock *mb, u8 *addr, u32 size, u8 align);
u32 vramBlock_allocateSpecial(s_vramBlock *mb, u8 *addr, u32 size);
u32 vramBlock_allocateBlock(s_vramBlock *mb, u32 size, u8 align);
u32 vramBlock_deallocateBlock(s_vramBlock *mb, u32 index);
u8 *vramBlock_getAddr(s_vramBlock *mb, u32 index);

#define NDS_FTR_CARVE_REGIONS 16u
#define NDS_FTR_CARVE_TEX 0u
#define NDS_FTR_CARVE_PAL 1u
/* Hole filling takes free runs down to these sizes; smaller holes stay with
 * libnds. A texture that fits no region reserves a D chunk of this size. */
#define NDS_FTR_CARVE_TEX_FILL_MAX 0x10000u
#define NDS_FTR_CARVE_TEX_FILL_MIN 0x800u
#define NDS_FTR_CARVE_PAL_FILL_MAX 0x1000u
#define NDS_FTR_CARVE_PAL_FILL_MIN 0x100u
#define NDS_FTR_CARVE_TEX_CHUNK 0x4000u
#define NDS_FTR_CARVE_PAL_CHUNK 0x400u

typedef struct NDSFtrCarveRegion
{
    u32 lcd;     /* region start, LCD address */
    u32 size;    /* bytes reserved (0: retired by the shrink) */
    u32 used;    /* bump offset */
    u32 index;   /* libnds vramBlock index of the reservation (0: returned) */
    u32 live;    /* carved textures still resident in it */
} NDSFtrCarveRegion;

static NDSFtrCarveRegion sNdsFtrCarveRegion[2][NDS_FTR_CARVE_REGIONS];
static u32 sNdsFtrCarveRegionCount[2];
static u32 sNdsFtrCarveLazyTex;
static u32 sNdsFtrCarveLive;
static u32 sNdsFtrCarveRetiring;
static int sNdsFtrCarveCarrier;
static gl_texture_data *sNdsFtrCarveCarrierTex;
static int sNdsFtrCarveCarrierPal;
static gl_palette_data *sNdsFtrCarveCarrierPalData;

#if NDS_VRAM_CENSUS_LIVE
typedef struct NDSFtrCarveLab
{
    u32 opens;
    u32 carrier_fail;
    u32 regions[2];
    u32 reserved[2];     /* bytes reserved (peak) */
    u32 used[2];         /* bytes carved */
    u32 carved[2];       /* textures / palettes carved */
    u32 fails[2];        /* carve refusals: no region fits */
    u32 lazy;            /* D chunks reserved during the admission */
    u32 shrink_freed[2]; /* bytes the end-of-admission shrink returned */
    u32 retires;
    u32 retire_regions;
    u32 region_returns;  /* texture regions returned when their last entry went */
    u32 exit_released;   /* carved entries released at battle exit */
    u32 libc_before;     /* libc top chunk at open / after the admission */
    u32 libc_after;
    /* the lab identity census (gNdsFtrAdmitIdentCensus) */
    u32 census_runs;
    u32 census_records;
    u32 census_keys;     /* distinct identities */
    u32 census_dups;     /* records that repeat an identity with the same key */
    u32 census_skipped;  /* sources this battle cannot address */
    u32 census_noprobe;  /* records the resolve path refused before its key */
    u32 census_collisions;
    u32 census_overflow;
    u32 census_ticks;
    u32 census_first[4]; /* kind << 8 | detail, record, other record, id_a */
} NDSFtrCarveLab;
NDSFtrCarveLab gNdsFtrCarveLab __attribute__((used, aligned(32)));

static void ndsFtrCarveLabPublish(void)
{
    DC_FlushRange(&gNdsFtrCarveLab, sizeof(gNdsFtrCarveLab));
}
#endif

/* ndsRendererHardwareBindTextureState's carved half. */
static void ndsRendererHardwareBindCarvedState(u32 name)
{
    const NDSRendererHardwareTextureCacheEntry *entry;
    u32 slot = name & 0xFFu;

    if ((slot >= NDS_RENDERER_HW_TEXTURE_CACHE_COUNT) ||
        (sNdsFtrCarveCarrierTex == NULL))
    {
        GFX_TEX_FORMAT = 0u;
        GFX_PAL_FORMAT = 0u;
        glGlobalData.activeTexture = 0;
        glGlobalData.activePalette = 0;
        return;
    }
    entry = &sNdsRendererHardwareTextureCache[slot];
    sNdsFtrCarveCarrierTex->texFormat = entry->params;
    GFX_TEX_FORMAT = entry->params;
    glGlobalData.activeTexture = sNdsFtrCarveCarrier;
    if (((entry->carve_flags & NDS_FTR_CARVE_F_PALETTE) != 0u) &&
        (sNdsFtrCarveCarrierPalData != NULL))
    {
        sNdsFtrCarveCarrierPalData->addr = entry->carve_pltt;
        GFX_PAL_FORMAT = entry->carve_pltt;
        glGlobalData.activePalette = sNdsFtrCarveCarrierPal;
    }
    else
    {
        GFX_PAL_FORMAT = 0u;
        glGlobalData.activePalette = 0;
    }
}

/* The carrier: one real libnds name with one 8-colour palette, whose record
 * words a carved bind rewrites. */
static s32 ndsFtrCarveCarrierOpen(void)
{
    static const u16 kCarrierPalette[8] = { 0u };
    int name = 0;
    gl_texture_data *tex;

    if (sNdsFtrCarveCarrier != 0)
    {
        return TRUE;
    }
    if ((ndsRendererHardwareFencedGlGenTextures(1, &name) == 0) ||
        (name <= 0))
    {
        return FALSE;
    }
    ndsRendererHardwareEndBatch();
    ndsRendererHardwareBindTextureState(name);
    glColorTableEXT(GL_TEXTURE_2D, 0, 8, 0, 0, kCarrierPalette);
    tex = (gl_texture_data *)DynamicArrayGet(&glGlobalData.texturePtrs,
                                             (unsigned int)name);
    sNdsRendererHardwareBoundTextureName = 0u;
    sNdsRendererHardwareActiveTextureEntry = NULL;
    if ((tex == NULL) || (tex->palIndex <= 0))
    {
        ndsRendererHardwareFencedGlDeleteTextures(1, &name);
        return FALSE;
    }
    sNdsFtrCarveCarrierPalData = (gl_palette_data *)DynamicArrayGet(
        &glGlobalData.palettePtrs, (unsigned int)tex->palIndex);
    if (sNdsFtrCarveCarrierPalData == NULL)
    {
        ndsRendererHardwareFencedGlDeleteTextures(1, &name);
        return FALSE;
    }
    sNdsFtrCarveCarrier = name;
    sNdsFtrCarveCarrierTex = tex;
    sNdsFtrCarveCarrierPal = tex->palIndex;
    return TRUE;
}

static void ndsFtrCarveCarrierClose(void)
{
    if (sNdsFtrCarveCarrier != 0)
    {
        ndsRendererHardwareEndBatch();
        ndsRendererHardwareFencedGlDeleteTextures(1, &sNdsFtrCarveCarrier);
    }
    sNdsFtrCarveCarrier = 0;
    sNdsFtrCarveCarrierTex = NULL;
    sNdsFtrCarveCarrierPal = 0;
    sNdsFtrCarveCarrierPalData = NULL;
    sNdsRendererHardwareBoundTextureName = 0u;
    sNdsRendererHardwareActiveTextureEntry = NULL;
}

/* One reservation: libnds's own first-fit search over the allocator's usable,
 * unlocked banks, 16-byte aligned. */
static s32 ndsFtrCarveReserve(u32 which, u32 bytes)
{
    s_vramBlock *mb = glGlobalData.vramBlocks[which];
    NDSFtrCarveRegion *region;
    u32 index;
    u8 *addr;

    if ((mb == NULL) || (bytes == 0u) ||
        (sNdsFtrCarveRegionCount[which] >= NDS_FTR_CARVE_REGIONS))
    {
        return FALSE;
    }
    bytes = (bytes + 15u) & ~15u;
    index = vramBlock_allocateBlock(mb, bytes, 4u);
    if (index == 0u)
    {
        return FALSE;
    }
    addr = vramBlock_getAddr(mb, index);
    if (addr == NULL)
    {
        (void)vramBlock_deallocateBlock(mb, index);
        return FALSE;
    }
    region = &sNdsFtrCarveRegion[which][sNdsFtrCarveRegionCount[which]++];
    region->lcd = (u32)(uintptr_t)addr;
    region->size = bytes;
    region->used = 0u;
    region->index = index;
    region->live = 0u;
    NDS_FTR_ADMIT_LAB((gNdsFtrCarveLab.regions[which]++,
                       gNdsFtrCarveLab.reserved[which] += bytes));
    return TRUE;
}

/* Take every free run down to `smallest`, largest pieces first. */
static void ndsFtrCarveFill(u32 which, u32 largest, u32 smallest)
{
    u32 size;

    for (size = largest; size >= smallest; size >>= 1)
    {
        while (ndsFtrCarveReserve(which, size) != FALSE)
        {
        }
    }
}

static s32 ndsFtrCarveTake(u32 which, u32 bytes, u32 align, u32 *lcd,
                           u32 *region_index)
{
    u32 i;

    for (i = 0u; i < sNdsFtrCarveRegionCount[which]; i++)
    {
        NDSFtrCarveRegion *region = &sNdsFtrCarveRegion[which][i];
        u32 at = (region->lcd + region->used + (align - 1u)) & ~(align - 1u);

        if ((region->index != 0u) &&
            (at + bytes <= region->lcd + region->size))
        {
            region->used = at + bytes - region->lcd;
            *lcd = at;
            *region_index = i;
            return TRUE;
        }
    }
    return FALSE;
}

static s32 ndsFtrCarveAlloc(u32 which, u32 bytes, u32 align, u32 *lcd,
                            u32 *region_index)
{
    u32 chunk = (which == NDS_FTR_CARVE_TEX) ? NDS_FTR_CARVE_TEX_CHUNK :
        NDS_FTR_CARVE_PAL_CHUNK;

    if (ndsFtrCarveTake(which, bytes, align, lcd, region_index) != FALSE)
    {
        return TRUE;
    }
    if ((which == NDS_FTR_CARVE_TEX) && (sNdsFtrCarveLazyTex == 0u))
    {
        return FALSE;
    }
    if (((bytes < chunk) && (ndsFtrCarveReserve(which, chunk) != FALSE)) ||
        (ndsFtrCarveReserve(which, bytes + align) != FALSE))
    {
        NDS_FTR_ADMIT_LAB(if (which == NDS_FTR_CARVE_TEX)
                          { gNdsFtrCarveLab.lazy++; });
        return ndsFtrCarveTake(which, bytes, align, lcd, region_index);
    }
    return FALSE;
}

/* libnds's glColorTableEXT offset: the bank's own offset, plus its palette
 * slot for F and G (VRAMCNT OFS: slot 0, 1, 4 or 5 x 16 KB). */
static u32 ndsFtrCarvePaletteOffset(u32 lcd)
{
    const volatile u8 *cnt = (const volatile u8 *)0x04000244u;
    u32 base;
    u32 ofs;

    if (lcd >= 0x06894000u)
    {
        base = 0x06894000u;
        ofs = (cnt[2] >> 3) & 3u;
    }
    else if (lcd >= 0x06890000u)
    {
        base = 0x06890000u;
        ofs = (cnt[1] >> 3) & 3u;
    }
    else
    {
        return lcd - 0x06880000u;
    }
    return (lcd - base) + (((ofs & 1u) + ((ofs & 2u) << 1)) << 14);
}

/* CPU-copy into VRAM through the LCD mapping of exactly the banks the range
 * covers (glTexImage2D / glColorTableEXT map A-D / E-G the same way). */
static void ndsFtrCarveCopy(u32 lcd, const void *src, u32 bytes, u32 halfwords)
{
    volatile u8 *cnt;
    u8 saved[4];
    u32 first;
    u32 last;
    u32 b;

    if (lcd >= 0x06880000u)
    {
        /* E 0x06880000 (64 KB), F 0x06890000, G 0x06894000 (16 KB each) */
        cnt = (volatile u8 *)0x04000244u;
        first = (lcd >= 0x06894000u) ? 2u : ((lcd >= 0x06890000u) ? 1u : 0u);
        last = ((lcd + bytes - 1u) >= 0x06894000u) ? 2u :
            (((lcd + bytes - 1u) >= 0x06890000u) ? 1u : 0u);
    }
    else
    {
        cnt = (volatile u8 *)0x04000240u;
        first = (lcd - 0x06800000u) >> 17;
        last = (lcd + bytes - 1u - 0x06800000u) >> 17;
    }
    for (b = first; b <= last; b++)
    {
        saved[b] = cnt[b];
        cnt[b] = 0x80u;
    }
    if (halfwords != 0u)
    {
        swiCopy(src, (void *)(uintptr_t)lcd, (int)((bytes >> 1) | COPY_MODE_HWORD));
    }
    else
    {
        swiCopy(src, (void *)(uintptr_t)lcd, (int)((bytes >> 2) | COPY_MODE_WORD));
    }
    for (b = first; b <= last; b++)
    {
        cnt[b] = saved[b];
    }
}

/* The resolve path's upload, carved: the same words glTexImage2D and
 * glColorTableEXT would have produced, in VRAM the admission owns. */
static s32 ndsFtrCarveUpload(NDSRendererHardwareTextureCacheEntry *entry,
                             u32 type, u32 size_x, u32 size_y, u32 params,
                             const void *texels, u32 palette_entries,
                             const u16 *palette)
{
    /* bits per texel by GL_TEXTURE_TYPE_ENUM (GL_RGB is uploaded as GL_RGBA) */
    static const u8 kBits[8] = { 0u, 8u, 2u, 4u, 8u, 0u, 8u, 16u };
    u32 hw_type = (type == (u32)GL_RGB) ? (u32)GL_RGBA : type;
    u32 bits = (hw_type < 8u) ? kBits[hw_type] : 0u;
    u32 bytes;
    u32 lcd = 0u;
    u32 pal_lcd = 0u;
    u32 pltt = 0u;
    u32 word;
    u32 slot;
    u32 region = 0u;
    u32 pal_region = 0u;

    if ((entry == NULL) || (texels == NULL) || (bits == 0u) ||
        (size_x > 7u) || (size_y > 7u) || (palette_entries > 256u) ||
        ((palette_entries != 0u) && (palette == NULL)))
    {
        return FALSE;
    }
    bytes = ((1u << (size_x + size_y + 6u)) * bits) >> 3;
    if (ndsFtrCarveAlloc(NDS_FTR_CARVE_TEX, bytes, 8u, &lcd, &region) == FALSE)
    {
        NDS_FTR_ADMIT_LAB(gNdsFtrCarveLab.fails[NDS_FTR_CARVE_TEX]++);
        return FALSE;
    }
    if (palette_entries != 0u)
    {
        u32 pal_bytes = palette_entries * 2u;
        u32 rgb4 = (hw_type == (u32)GL_RGB4) ? 1u : 0u;

        if (ndsFtrCarveAlloc(NDS_FTR_CARVE_PAL, pal_bytes, rgb4 ? 8u : 16u,
                             &pal_lcd, &pal_region) == FALSE)
        {
            NDS_FTR_ADMIT_LAB(gNdsFtrCarveLab.fails[NDS_FTR_CARVE_PAL]++);
            return FALSE;
        }
        ndsFtrCarveCopy(pal_lcd, palette, pal_bytes, 1u);
        pltt = ndsFtrCarvePaletteOffset(pal_lcd) >> (rgb4 ? 3u : 4u);
        NDS_FTR_ADMIT_LAB((gNdsFtrCarveLab.carved[NDS_FTR_CARVE_PAL]++,
                           gNdsFtrCarveLab.used[NDS_FTR_CARVE_PAL] += pal_bytes));
    }
    ndsFtrCarveCopy(lcd, texels, bytes, 0u);
    word = (size_x << 20) | (size_y << 23) | (hw_type << 26) |
        ((lcd >> 3) & 0xFFFFu);
    word = (word & 0x1FF0FFFFu) | params;
    slot = (u32)(entry - sNdsRendererHardwareTextureCache);
    entry->name = (int)(NDS_FTR_CARVE_NAME_BASE | slot);
    entry->params = (word & ~NDS_RENDERER_TEXTURE_PARAM_MUTABLE_MASK) |
        (params & NDS_RENDERER_TEXTURE_PARAM_MUTABLE_MASK);
    entry->carve_pltt = (u16)pltt;
    entry->carve_flags = (u8)(NDS_FTR_CARVE_F_CARVED |
        ((palette_entries != 0u) ? NDS_FTR_CARVE_F_PALETTE : 0u));
    entry->carve_region = (u8)region;
    sNdsFtrCarveRegion[NDS_FTR_CARVE_TEX][region].live++;
    sNdsFtrCarveLive++;
    (void)pal_region;
    NDS_FTR_ADMIT_LAB((gNdsFtrCarveLab.carved[NDS_FTR_CARVE_TEX]++,
                       gNdsFtrCarveLab.used[NDS_FTR_CARVE_TEX] += bytes));
    return TRUE;
}

/* Admission start: the carrier, then every free texture run of A+B (D is
 * held back so the holes the lock is about to strand are used first; D is
 * chunked on demand) and every free palette run of F+G. */
static s32 NDS_FTR_ADMIT_COLD_CODE ndsFtrCarveOpen(u32 with_d)
{
    int saved_lock = glGlobalData.vramLock[0];

    NDS_FTR_ADMIT_LAB((gNdsFtrCarveLab.opens++,
                       gNdsFtrCarveLab.libc_before = mallinfo().keepcost));
    if (ndsFtrCarveCarrierOpen() == FALSE)
    {
        NDS_FTR_ADMIT_LAB(gNdsFtrCarveLab.carrier_fail++);
        return FALSE;
    }
    if (with_d != 0u)
    {
        glLockVRAMBank((u16 *)VRAM_D);
    }
    ndsFtrCarveFill(NDS_FTR_CARVE_TEX, NDS_FTR_CARVE_TEX_FILL_MAX,
                    NDS_FTR_CARVE_TEX_FILL_MIN);
    glGlobalData.vramLock[0] = saved_lock;
    ndsFtrCarveFill(NDS_FTR_CARVE_PAL, NDS_FTR_CARVE_PAL_FILL_MAX,
                    NDS_FTR_CARVE_PAL_FILL_MIN);
    sNdsFtrCarveLazyTex = with_d;
    sNdsFtrCarveActive = 1u;
    return TRUE;
}

/* Admission end: give back what was not carved. A region keeps exactly its
 * used bytes (released, then re-reserved at the same address, which is free
 * and large enough by construction). */
static void NDS_FTR_ADMIT_COLD_CODE ndsFtrCarveShrink(u32 which)
{
    s_vramBlock *mb = glGlobalData.vramBlocks[which];
    u32 i;

    for (i = 0u; i < sNdsFtrCarveRegionCount[which]; i++)
    {
        NDSFtrCarveRegion *region = &sNdsFtrCarveRegion[which][i];
        u32 keep = (region->used + 15u) & ~15u;
        u32 index = 0u;

        if ((which == NDS_FTR_CARVE_TEX) && (region->live == 0u))
        {
            keep = 0u;
        }
        if ((region->index == 0u) || (keep >= region->size))
        {
            continue;
        }
        (void)vramBlock_deallocateBlock(mb, region->index);
        NDS_FTR_ADMIT_LAB(gNdsFtrCarveLab.shrink_freed[which] +=
                              region->size - keep);
        if (keep == 0u)
        {
            region->index = 0u;
            region->size = 0u;
            continue;
        }
        if (vramBlock_examineSpecial(mb, (u8 *)(uintptr_t)region->lcd, keep,
                                     4u) == (u8 *)(uintptr_t)region->lcd)
        {
            index = vramBlock_allocateSpecial(mb, (u8 *)(uintptr_t)region->lcd,
                                              keep);
        }
        if (index == 0u)
        {
            /* Cannot happen (the run was just freed); keep the whole region
             * rather than leave carved texels unowned. */
            NDS_FTR_ADMIT_LAB(gNdsFtrCarveLab.shrink_freed[which] -=
                                  region->size - keep);
            if (vramBlock_examineSpecial(mb, (u8 *)(uintptr_t)region->lcd,
                                         region->size, 4u) ==
                (u8 *)(uintptr_t)region->lcd)
            {
                index = vramBlock_allocateSpecial(
                    mb, (u8 *)(uintptr_t)region->lcd, region->size);
            }
        }
        else
        {
            region->size = keep;
        }
        region->index = index;
    }
}

static void NDS_FTR_ADMIT_COLD_CODE ndsFtrCarveClose(void)
{
    if (sNdsFtrCarveActive == 0u)
    {
        return;
    }
    sNdsFtrCarveActive = 0u;
    sNdsFtrCarveLazyTex = 0u;
    ndsFtrCarveShrink(NDS_FTR_CARVE_TEX);
    ndsFtrCarveShrink(NDS_FTR_CARVE_PAL);
    NDS_FTR_ADMIT_LAB(gNdsFtrCarveLab.libc_after = mallinfo().keepcost);
}

/* Everything carved goes: every carved entry still resident, every region
 * and the carrier. The last carved entry's release calls this; so does the
 * scene reset (before glResetTextures rebuilds the allocators) and the
 * admission when it carved nothing. */
void NDS_FTR_ADMIT_COLD_CODE ndsFtrLeanAdmitCarveRetire(void)
{
    u32 which;
    u32 i;

    if ((sNdsFtrCarveRetiring != 0u) ||
        ((sNdsFtrCarveCarrier == 0) && (sNdsFtrCarveRegionCount[0] == 0u) &&
         (sNdsFtrCarveRegionCount[1] == 0u)))
    {
        return;
    }
    sNdsFtrCarveRetiring = 1u;
    for (i = 0u; i < NDS_RENDERER_HW_TEXTURE_CACHE_COUNT; i++)
    {
        if ((sNdsRendererHardwareTextureCache[i].carve_flags &
             NDS_FTR_CARVE_F_CARVED) != 0u)
        {
            (void)ndsRendererHardwareReleaseTexture(
                &sNdsRendererHardwareTextureCache[i]);
        }
    }
    for (which = 0u; which < 2u; which++)
    {
        s_vramBlock *mb = glGlobalData.vramBlocks[which];

        for (i = 0u; i < sNdsFtrCarveRegionCount[which]; i++)
        {
            if ((mb != NULL) && (sNdsFtrCarveRegion[which][i].index != 0u))
            {
                (void)vramBlock_deallocateBlock(
                    mb, sNdsFtrCarveRegion[which][i].index);
                NDS_FTR_ADMIT_LAB(gNdsFtrCarveLab.retire_regions++);
            }
        }
        sNdsFtrCarveRegionCount[which] = 0u;
    }
    ndsFtrCarveCarrierClose();
    sNdsFtrCarveActive = 0u;
    sNdsFtrCarveLazyTex = 0u;
    sNdsFtrCarveLive = 0u;
    sNdsFtrCarveRetiring = 0u;
    NDS_FTR_ADMIT_LAB(gNdsFtrCarveLab.retires++);
#if NDS_VRAM_CENSUS_LIVE
    ndsFtrCarveLabPublish();
#endif
}

/* ndsRendererHardwareReleaseTexture, for a carved entry (before it is zeroed):
 * its region goes back to libnds with its last entry, everything else with
 * the last carved entry. Never while the admission is still carving. */
static void ndsFtrCarveNoteRelease(
    const NDSRendererHardwareTextureCacheEntry *entry)
{
    NDSFtrCarveRegion *region;

    if ((sNdsFtrCarveRetiring != 0u) ||
        ((entry->carve_flags & NDS_FTR_CARVE_F_CARVED) == 0u))
    {
        return;
    }
    if (sNdsFtrCarveLive != 0u)
    {
        sNdsFtrCarveLive--;
    }
    if (entry->carve_region < sNdsFtrCarveRegionCount[NDS_FTR_CARVE_TEX])
    {
        region = &sNdsFtrCarveRegion[NDS_FTR_CARVE_TEX][entry->carve_region];
        if (region->live != 0u)
        {
            region->live--;
        }
        if ((region->live == 0u) && (region->index != 0u) &&
            (sNdsFtrCarveActive == 0u))
        {
            (void)vramBlock_deallocateBlock(
                glGlobalData.vramBlocks[NDS_FTR_CARVE_TEX], region->index);
            region->index = 0u;
            region->size = 0u;
            NDS_FTR_ADMIT_LAB(gNdsFtrCarveLab.region_returns++);
        }
    }
    if ((sNdsFtrCarveLive == 0u) && (sNdsFtrCarveActive == 0u))
    {
        ndsFtrLeanAdmitCarveRetire();
    }
}

/* An entry's VRAM address (LCD) for the lab's A+B / D accounting. */
static u32 ndsFtrAdmitEntryAddress(
    const NDSRendererHardwareTextureCacheEntry *entry);

static void NDS_FTR_ADMIT_COLD_CODE
ndsFtrAdmitLatch(u32 fkind, u32 detail, u32 reason, u32 index,
                 u32 image, u32 reject)
{
    if (gNdsFtrLeanAdmitFail == 0u)
    {
        gNdsFtrLeanAdmitFailFirst[0] = (fkind << 16) | (detail << 8) | reason;
        gNdsFtrLeanAdmitFailFirst[1] = index;
        gNdsFtrLeanAdmitFailFirst[2] = image;
        gNdsFtrLeanAdmitFailFirst[3] = reject;
    }
    gNdsFtrLeanAdmitFail++;
}

static const u32 *sNdsFtrAdmitBaseAsset;
static const void *const *sNdsFtrAdmitBaseData;
static u32 sNdsFtrAdmitBaseCount;

/* A record's SOURCE offset -> the live pointer the draw would carry: the
 * fighter's own loaded file (the adapter's base table, from FTData), else the
 * reloc backend's current-scene view. */
/* *why on NULL: 1 = the file's compact battle pack omitted this SOURCE span
 * (the pack keeps exactly the spans battle consumes, so battle cannot draw
 * it either); 2 = the file is not loaded in this battle (a part that lives in
 * another kind's file -- Kirby's copy props -- whose kind is absent);
 * 0 = a real failure. */
static const void *ndsFtrAdmitAddress(u32 asset, u32 offset, u32 *why)
{
    const void *data = NULL;
    const void *mapped;
    u32 size = 0u;
    u32 i;

    for (i = 0u; i < sNdsFtrAdmitBaseCount; i++)
    {
        if ((sNdsFtrAdmitBaseAsset[i] == asset) &&
            (sNdsFtrAdmitBaseData[i] != NULL))
        {
            data = sNdsFtrAdmitBaseData[i];
            break;
        }
    }
    if (data == NULL)
    {
        if ((ndsRelocGetLoadedAssetView(asset, &data, &size) == FALSE) ||
            (data == NULL))
        {
            *why = 2u;
            return NULL;
        }
        if (offset >= size)
        {
            return NULL;
        }
    }
    mapped = ndsRelocNativeAssetAddress(data, offset);
    if (mapped == NULL)
    {
        *why = 1u;
    }
    return mapped;
}

static u32 ndsFtrAdmitNameAddress(int name)
{
    const DynamicArray *textures = &glGlobalData.texturePtrs;
    const gl_texture_data *tex;

    if ((name <= 0) || ((u32)name >= textures->cur_size))
    {
        return 0u;
    }
    tex = (const gl_texture_data *)textures->data[name];
    return (tex != NULL) ? (u32)(uintptr_t)tex->vramAddr : 0u;
}

static u32 ndsFtrAdmitEntryAddress(
    const NDSRendererHardwareTextureCacheEntry *entry)
{
    if ((entry->carve_flags & NDS_FTR_CARVE_F_CARVED) != 0u)
    {
        return 0x06800000u + ((entry->params & 0xFFFFu) << 3);
    }
    return ndsFtrAdmitNameAddress(entry->name);
}

/* One record: the draw's texture state, replayed; then the cache resolves
 * (and, when it is not resident yet, converts and uploads) it. Returns 1
 * resolved, 0 refused by the cache, -1 source not addressable, -2 source span
 * omitted by the file's compact battle pack, -3 source file not loaded in this
 * battle (-2 and -3 cannot be drawn in this battle: skipped, not failed). */
static s32 ndsFtrAdmitApply(NDSRendererStats *stats, const u32 *w,
                            NDSRendererHardwareResolvedTexture *resolved)
{
    u32 flags = w[0];
    u32 why = 0u;
    const void *image = ndsFtrAdmitAddress(w[2] & 0xffffu, w[3], &why);
    const void *tlut = NULL;

    if (image == NULL)
    {
        return (why == 1u) ? -2 : (why == 2u) ? -3 : -1;
    }
    if ((flags & NDS_FIGHTER_ADMISSION_F_HAS_TLUT) != 0u)
    {
        tlut = ndsFtrAdmitAddress(w[2] >> 16, w[4], &why);
        if (tlut == NULL)
        {
            return (why == 1u) ? -2 : (why == 2u) ? -3 : -1;
        }
    }
    ndsRendererInitStats(stats);
    if (tlut != NULL)
    {
        ndsRendererRecordSetImage(stats, w[10], (u32)(uintptr_t)tlut);
        ndsRendererRecordSetTile(stats, w[11], w[12]);
        ndsRendererRecordLoadTlut(stats, w[13]);
    }
    ndsRendererRecordSetImage(stats, w[5], (u32)(uintptr_t)image);
    ndsRendererRecordSetTile(stats, w[6], w[7]);
    if ((flags & NDS_FIGHTER_ADMISSION_F_LOAD_TILE) != 0u)
    {
        ndsRendererRecordLoadTile(stats, w[8], w[9]);
    }
    else
    {
        ndsRendererRecordLoadBlock(stats, w[8], w[9]);
    }
    ndsRendererRecordSetTile(stats, w[14], w[15]);
    ndsRendererRecordSetTileSize(stats, w[16], w[17]);
    ndsRendererRecordTextureState(stats, w[18], w[19]);
    ndsRendererRecordSetCombine(stats, w[20], w[21]);
    ndsRendererRecordOtherMode(
        stats, NDS_RENDERER_OP_RDPSETOTHERMODE,
        (NDS_RENDERER_OP_RDPSETOTHERMODE << 24) | (w[22] & 0x00ffffffu), w[23]);
    if ((flags & NDS_FIGHTER_ADMISSION_F_HAS_PRIM) != 0u)
    {
        stats->prim_color = w[25];
        stats->prim_min_level = (w[24] >> 8) & 0xffu;
        stats->prim_lod_fraction = w[24] & 0xffu;
    }
    if ((flags & NDS_FIGHTER_ADMISSION_F_HAS_ENV) != 0u)
    {
        stats->env_color = w[26];
    }
    /* The draw path proper (resolve, convert, upload, bind: `resolved` NULL),
     * then the resident lookup the hierarchy preflight uses, for the entry. */
    if (ndsRendererHardwareResolveOrBindTexture(stats, &sNdsFtrAdmitConfig,
                                                NULL, NULL, FALSE) == FALSE)
    {
        return 0;
    }
    return (ndsRendererHardwareResolveResidentTexture(
                stats, &sNdsFtrAdmitConfig, NULL, resolved) != FALSE) ? 1 : 0;
}

#if NDS_VRAM_CENSUS_LIVE
static void ndsFtrAdmitLabFree(u32 *free_bytes, u32 *largest)
{
    u32 blocks[11];

    ndsVramCensusBlocks(glGlobalData.vramBlocks[0], 0u, blocks);
    *free_bytes = blocks[8];
    *largest = blocks[9];
}
#endif

/* Take bank D for textures (word 2), once per battle, keyed on the property
 * that makes it safe: the battle has left BG3 empty. A, B and D all allocate
 * until ndsFtrLeanAdmitLockRegions, so the admission (first-fit) packs into
 * what A+B still has free before it spills into D. */
static void ndsFtrAdmitEnterRegions(void)
{
    if (sNdsFtrAdmitRegions != 0u)
    {
        return;
    }
    NDS_FTR_ADMIT_LAB(ndsFtrAdmitLabFree(&gNdsFtrAdmitLab.free_before,
                                         &gNdsFtrAdmitLab.largest_before));
    if ((ndsPlatformVramBg3Empty() == FALSE) ||
        (ndsPlatformVramTakeBankD() == FALSE))
    {
        NDS_FTR_ADMIT_LAB(gNdsFtrAdmitLab.bg3_not_empty++);
        return;
    }
    sNdsFtrAdmitSavedLock = glGlobalData.vramLock[0];
    NDS_FTR_ADMIT_LAB(gNdsFtrAdmitLab.lock_before = (u32)sNdsFtrAdmitSavedLock);
    glUnlockVRAMBank((u16 *)VRAM_D);
    sNdsFtrAdmitRegions = 1u;
    sNdsFtrAdmitLocked = 0u;
    NDS_FTR_ADMIT_LAB(gNdsFtrAdmitLab.regions = 1u);
}

/* The battle's setup uploads are done (first frame end after the admission):
 * A+B hold the scene set and the admitted fighters for the rest of the
 * battle, and D alone allocates and frees. */
void ndsFtrLeanAdmitLockRegions(void)
{
    if ((sNdsFtrAdmitRegions == 0u) || (sNdsFtrAdmitLocked != 0u))
    {
        return;
    }
    glLockVRAMBank((u16 *)VRAM_A);
    glLockVRAMBank((u16 *)VRAM_B);
    sNdsFtrAdmitLocked = 1u;
    NDS_FTR_ADMIT_LAB((gNdsFtrAdmitLab.locked = 1u,
                       gNdsFtrAdmitLab.lock_frame = gNdsRendererProfileFrameCount,
                       gNdsFtrAdmitLab.lock_after =
                           (u32)glGlobalData.vramLock[0]));
}

/* D IS FULL AND THE CACHE HAS NOTHING TO GIVE (2026-10-18). After the lock D
 * alone serves the battle's uploads, and its bytes can all be held by what
 * the cache may not evict: the admission's D chunks and the names other
 * owners keep (effects, weapons). Four heavy contents on Hyrule Castle left D
 * 128 B and A+B 28.5 KB of free holes, and a Bob-omb's bind failed every
 * frame it was out (2 native failure rows a match). The cache's upload takes
 * those holes once before it refuses: A+B unlocked for that one upload, the
 * lock word restored after; what lands there is an ordinary entry, evicted
 * and freed as any other. FALSE when A+B were not locked by this battle. */
static int sNdsFtrAdmitHolesSavedLock;

s32 ndsFtrLeanAdmitOpenHoles(void)
{
    if ((sNdsFtrAdmitRegions == 0u) || (sNdsFtrAdmitLocked == 0u))
    {
        return FALSE;
    }
    sNdsFtrAdmitHolesSavedLock = glGlobalData.vramLock[0];
    glUnlockVRAMBank((u16 *)VRAM_A);
    glUnlockVRAMBank((u16 *)VRAM_B);
    gNdsFtrAdmitHoleUploads++;
    return TRUE;
}

void ndsFtrLeanAdmitCloseHoles(void)
{
    glGlobalData.vramLock[0] = sNdsFtrAdmitHolesSavedLock;
}

#if NDS_VRAM_CENSUS_LIVE
/* Lab word: 1 = before admitting, prove the identities of the admission table
 * collision-free for every kind in the match -- both details, every costume,
 * every hat. Each record's key is built by the resolve path itself (the key
 * probe returns right after the key); a record whose identity repeats an
 * earlier record's is rebuilt and compared word for word, so a collision is
 * an exact finding, not a fingerprint's. The rows live in the texture scratch
 * buffer, which nothing converts into while the probe is set. */
#if defined(__arm__)
volatile u32 gNdsFtrAdmitIdentCensus
    __attribute__((used, section(".dtcm.bss"), aligned(4)));
#else
volatile u32 gNdsFtrAdmitIdentCensus;
#endif

static s32 ndsFtrAdmitProbeRecord(NdsRelocAssetStream *stream,
                                  NDSRendererStats *scratch, u32 record,
                                  NDSRendererHardwareTextureKey *key,
                                  s32 *result)
{
    u32 w[NDS_FIGHTER_ADMISSION_RECORD_WORDS];
    NDSRendererHardwareResolvedTexture resolved;

    if (ndsRelocAssetStreamRead(
            stream,
            NDS_FIGHTER_ADMISSION_HEADER_BYTES +
                record * NDS_FIGHTER_ADMISSION_RECORD_WORDS * 4u,
            w, sizeof(w)) == FALSE)
    {
        return FALSE;
    }
    memset(key, 0, sizeof(*key));
    sNdsFtrAdmitKeyProbe = key;
    *result = ndsFtrAdmitApply(scratch, w, &resolved);
    sNdsFtrAdmitKeyProbe = NULL;
    return TRUE;
}

static void ndsFtrAdmitIdentCensus(NdsRelocAssetStream *stream,
                                   NDSRendererStats *scratch, u32 present)
{
    u32 (*rows)[4] = (u32 (*)[4])(void *)sNdsRendererHardwareTextureScratch;
    u32 cap = (u32)(sizeof(sNdsRendererHardwareTextureScratch) /
                    (4u * sizeof(u32)));
    NDSRendererHardwareTextureKey key;
    NDSRendererHardwareTextureKey other;
    u32 start = cpuGetTiming();
    u32 n = 0u;
    u32 kind;
    u32 d;

    gNdsFtrCarveLab.census_runs++;
    for (kind = 0u; kind < NDS_FIGHTER_ADMISSION_KINDS; kind++)
    {
        if ((present & (1u << kind)) == 0u)
        {
            continue;
        }
        for (d = 0u; d < 2u; d++)
        {
            u32 first = sNdsFtrAdmitIndex[4u + (kind * 2u + d) * 2u];
            u32 count = sNdsFtrAdmitIndex[5u + (kind * 2u + d) * 2u];
            u32 r;

            for (r = 0u; r < count; r++)
            {
                s32 result = 0;
                u32 id_a;
                u32 id_b;
                u32 j;

                gNdsFtrCarveLab.census_records++;
                if ((ndsFtrAdmitProbeRecord(stream, scratch, first + r, &key,
                                            &result) == FALSE) ||
                    (result < 0))
                {
                    gNdsFtrCarveLab.census_skipped++;
                    continue;
                }
                if (key.image == 0u)
                {
                    gNdsFtrCarveLab.census_noprobe++;
                    continue;
                }
                ndsRendererHardwareTextureIdentOf(&key, &id_a, &id_b);
                for (j = 0u; j < n; j++)
                {
                    if ((rows[j][0] == id_a) && (rows[j][1] == id_b) &&
                        (rows[j][2] == key.image))
                    {
                        break;
                    }
                }
                if (j < n)
                {
                    s32 other_result = 0;

                    if ((ndsFtrAdmitProbeRecord(stream, scratch, rows[j][3],
                                                &other, &other_result) !=
                         FALSE) &&
                        (memcmp(&key, &other, sizeof(key)) == 0))
                    {
                        gNdsFtrCarveLab.census_dups++;
                    }
                    else
                    {
                        if (gNdsFtrCarveLab.census_collisions == 0u)
                        {
                            gNdsFtrCarveLab.census_first[0] = (kind << 8) | d;
                            gNdsFtrCarveLab.census_first[1] = first + r;
                            gNdsFtrCarveLab.census_first[2] = rows[j][3];
                            gNdsFtrCarveLab.census_first[3] = id_a;
                        }
                        gNdsFtrCarveLab.census_collisions++;
                    }
                    continue;
                }
                if (n >= cap)
                {
                    gNdsFtrCarveLab.census_overflow++;
                    continue;
                }
                rows[n][0] = id_a;
                rows[n][1] = id_b;
                rows[n][2] = key.image;
                rows[n][3] = first + r;
                n++;
            }
        }
    }
    gNdsFtrCarveLab.census_keys += n;
    gNdsFtrCarveLab.census_ticks += cpuGetTiming() - start;
}
#endif

/* One admission pass: the stops and counts every record shares. */
typedef struct NDSFtrAdmitPass
{
    NDSRendererStats *scratch;
    u32 present;
    u32 libc_stop;
    u32 slot_stop;
    u32 admitted_total;
    u32 admitted;
    u32 bytes;          /* texel bytes of the entries admitted */
} NDSFtrAdmitPass;

/* TEXIMAGE_PARAM's texel bytes (entry->params). */
static u32 ndsFtrAdmitParamsBytes(u32 params)
{
    static const u8 bits_per_texel[8] = { 0u, 8u, 2u, 4u, 8u, 2u, 8u, 16u };

    return ((8u << ((params >> 20) & 7u)) * (8u << ((params >> 23) & 7u)) *
            (u32)bits_per_texel[(params >> 26) & 7u]) / 8u;
}

/* Record `index` (`w`) of fighter `i`'s list (kind, detail d, costume):
 * skipped (another kind's hat, another costume, a stop), or replayed and the
 * entry it resolves admitted. */
static void NDS_FTR_ADMIT_COLD_CODE
ndsFtrAdmitRecord(NDSFtrAdmitPass *pass, const u32 *w, u32 i, u32 kind,
                  u32 d, u32 costume, u32 index)
{
    NDSRendererHardwareResolvedTexture resolved;
    s32 result;

    NDS_FTR_ADMIT_LAB(gNdsFtrAdmitLab.records++);
    if ((w[0] & NDS_FIGHTER_ADMISSION_F_HAT) != 0u)
    {
        u32 hat = (w[0] >> 8) & 0xffu;

        if ((kind != NDS_FTR_ADMIT_KIRBY) ||
            (hat == NDS_FTR_ADMIT_KIRBY) || (hat >= 32u) ||
            ((pass->present & (1u << hat)) == 0u))
        {
            NDS_FTR_ADMIT_LAB(gNdsFtrAdmitLab.skipped_hat++);
            return;
        }
    }
    if ((w[1] & (1u << (costume & 7u))) == 0u)
    {
        NDS_FTR_ADMIT_LAB(gNdsFtrAdmitLab.skipped_costume++);
        return;
    }
    if (pass->libc_stop != 0u)
    {
        NDS_FTR_ADMIT_LAB(gNdsFtrAdmitLab.libc_skipped++);
        return;
    }
    if (pass->admitted_total + NDS_FTR_ADMIT_SLOT_RESERVE >=
        NDS_RENDERER_HW_TEXTURE_DYNAMIC_COUNT)
    {
        if (pass->slot_stop == 0u)
        {
            pass->slot_stop = 1u;
            ndsFtrAdmitLatch(kind, d, nNDSFtrLeanAdmitFailSlots, index,
                             pass->admitted_total, 0u);
        }
        NDS_FTR_ADMIT_LAB(gNdsFtrAdmitLab.slot_skipped++);
        return;
    }
    {
        u32 top = mallinfo().keepcost;

        NDS_FTR_ADMIT_LAB(
            if ((gNdsFtrAdmitLab.libc_top_min == 0u) ||
                (top < gNdsFtrAdmitLab.libc_top_min))
            {
                gNdsFtrAdmitLab.libc_top_min = top;
            });
        if (top < NDS_FTR_ADMIT_LIBC_FLOOR)
        {
            pass->libc_stop = 1u;
            NDS_FTR_ADMIT_LAB(gNdsFtrAdmitLab.libc_skipped++);
            ndsFtrAdmitLatch(kind, d, nNDSFtrLeanAdmitFailLibc, index, top,
                             0u);
            return;
        }
    }
#if NDS_VRAM_CENSUS_LIVE
    gNdsFtrAdmitLastReject = 0u;
    {
        u32 apply_start = cpuGetTiming();

        result = ndsFtrAdmitApply(pass->scratch, w, &resolved);
        gNdsFtrAdmitLab.apply_ticks += cpuGetTiming() - apply_start;
    }
#else
    result = ndsFtrAdmitApply(pass->scratch, w, &resolved);
#endif
    if (result == -2)
    {
        NDS_FTR_ADMIT_LAB(gNdsFtrAdmitLab.pruned++);
        return;
    }
    if (result == -3)
    {
        NDS_FTR_ADMIT_LAB(gNdsFtrAdmitLab.absent++);
        return;
    }
    if (result < 0)
    {
        NDS_FTR_ADMIT_LAB(gNdsFtrAdmitLab.asset_fails++);
        ndsFtrAdmitLatch(kind, d, nNDSFtrLeanAdmitFailAsset, index, w[3],
                         w[2]);
        return;
    }
    if ((result == 0) || (resolved.entry == NULL))
    {
#if NDS_VRAM_CENSUS_LIVE
        gNdsFtrAdmitLab.reject_mask |= gNdsFtrAdmitLastReject;
        if (gNdsFtrAdmitLab.reject_first[0] == 0u)
        {
            gNdsFtrAdmitLab.reject_first[0] =
                gNdsFtrAdmitLastReject | ((result == 0) ? 0u : 0x80000000u);
            gNdsFtrAdmitLab.reject_first[1] = index;
            gNdsFtrAdmitLab.reject_first[2] = w[14];
            gNdsFtrAdmitLab.reject_first[3] = w[15];
            gNdsFtrAdmitLab.reject_first[4] = w[16];
            gNdsFtrAdmitLab.reject_first[5] = w[17];
        }
#endif
        NDS_FTR_ADMIT_LAB(gNdsFtrAdmitLab.fails++);
        ndsFtrAdmitLatch(kind, d, nNDSFtrLeanAdmitFailResolve, index,
                         ((w[2] & 0xfffu) << 20) | (w[3] & 0xfffffu),
#if NDS_FTR_LEAN_LAB
                         gNdsFtrLean.reject_mask
#else
                         0u
#endif
                         );
        return;
    }
    NDS_FTR_ADMIT_LAB((gNdsFtrAdmitLab.applied++,
                       gNdsFtrAdmitLab.per_fighter_applied[i & 3u]++));
    if (resolved.entry->admitted == 0u)
    {
        resolved.entry->admitted = 1u;
        pass->admitted++;
        pass->admitted_total++;
        pass->bytes += ndsFtrAdmitParamsBytes(resolved.entry->params);
#if NDS_VRAM_CENSUS_LIVE
        {
            u32 addr = ndsFtrAdmitEntryAddress(resolved.entry);
            u32 bytes = ndsVramCensusParamsBytes(resolved.entry->params);

            gNdsFtrAdmitLab.entries_admitted++;
            gNdsFtrAdmitLab.bytes_admitted += bytes;
            gNdsFtrAdmitLab.per_fighter_bytes[i & 3u] += bytes;
            if ((addr >= NDS_FTR_ADMIT_D_LO) && (addr < NDS_FTR_ADMIT_D_HI))
            {
                gNdsFtrAdmitLab.bytes_in_d += bytes;
            }
            else
            {
                gNdsFtrAdmitLab.bytes_in_ab += bytes;
            }
        }
#endif
    }
}

/* P4 (2026-10-13): the contents' records (scripts/p4/p4_admission.py) -- the
 * cast's record format under a header naming the content ids it indexes, then
 * (first, count) per content id x detail. A content was noted under its
 * parent kind, whose records name files its battle never loads, so its own
 * textures converted on first sight mid-match (four Crashes: 66 uploads after
 * GO, ~100K cycles each, inside the materializations of the frames that
 * first showed them). */
#define NDS_FTR_ADMIT_P4_MAGIC 0x34504441u /* 'ADP4' */
#define NDS_FTR_ADMIT_P4_VERSION 1u
/* The contents' texel bytes: a costume's set is 3-20 KB (Mr. Game & Watch
 * 45-59 KB), the cast's about 18 KB a fighter, and after the admission bank
 * D alone serves the battle's own uploads (effects, items, the stage's
 * animated textures). */
#define NDS_FTR_ADMIT_P4_BYTES 0x10000u

/* Every P4 fighter's list, one record of each in turn: a content's list is a
 * superset (every face of every costume), and four of them outgrow the slot
 * budget, which in fighter order would leave the last fighters none. */
static void NDS_FTR_ADMIT_COLD_CODE
ndsFtrAdmitP4Pass(NDSFtrAdmitPass *pass, const u32 *fkind, const u32 *costume,
                  const u32 *detail, const u32 *player, const u32 *content,
                  u32 count)
{
    NdsRelocAssetStream stream = { NULL };
    u32 head[4];
    u32 first[NDS_FTR_LEAN_ADMIT_FIGHTERS];
    u32 n[NDS_FTR_LEAN_ADMIT_FIGHTERS];
    u32 records_at;
    u32 longest = 0u;
    u32 budget;
    u32 r;
    u32 i;

    for (i = 0u; (i < count) && (content[i] == 0u); i++)
    {
    }
    if ((i == count) || (count > NDS_FTR_LEAN_ADMIT_FIGHTERS))
    {
        return;
    }
    if (ndsRelocAssetStreamOpen(&stream, "nitro:/fighters/p4_admission.bin") ==
        FALSE)
    {
        ndsFtrAdmitLatch(0u, 0u, nNDSFtrLeanAdmitFailOpen, 0u, 0u, 0u);
        return;
    }
    if ((ndsRelocAssetStreamRead(&stream, 0u, head, sizeof(head)) == FALSE) ||
        (head[0] != NDS_FTR_ADMIT_P4_MAGIC) ||
        (head[1] != NDS_FTR_ADMIT_P4_VERSION) ||
        (head[3] != NDS_FIGHTER_ADMISSION_RECORD_WORDS))
    {
        ndsRelocAssetStreamClose(&stream);
        ndsFtrAdmitLatch(0u, 0u, nNDSFtrLeanAdmitFailFormat, 0u, 0u, 0u);
        return;
    }
    records_at = sizeof(head) + head[2] * 2u * 2u * sizeof(u32);
    for (i = 0u; i < count; i++)
    {
        u32 row[2] = { 0u, 0u };
        u32 d = (detail[i] == 2u) ? 1u : 0u;

        if ((content[i] != 0u) && (content[i] < head[2]) &&
            (ndsRelocAssetStreamRead(
                 &stream,
                 sizeof(head) + (content[i] * 2u + d) * 2u * sizeof(u32),
                 row, sizeof(row)) == FALSE))
        {
            row[1] = 0u;
        }
        first[i] = row[0];
        n[i] = (content[i] != 0u) ? row[1] : 0u;
        longest = (n[i] > longest) ? n[i] : longest;
    }
    budget = pass->bytes + NDS_FTR_ADMIT_P4_BYTES;
    for (r = 0u; (r < longest) && (pass->slot_stop == 0u) &&
                 (pass->libc_stop == 0u) && (pass->bytes < budget); r++)
    {
        for (i = 0u; i < count; i++)
        {
            const u32 d = (detail[i] == 2u) ? 1u : 0u;

            if (r >= n[i])
            {
                continue;
            }
#if NDS_VRAM_CENSUS_LIVE
            gNdsVramCensusDrawSlotPlus1 = (player[i] & 3u) + 1u;
#else
            (void)player;
#endif
            if (ndsRelocAssetStreamRead(
                    &stream,
                    records_at + (first[i] + r) *
                                     NDS_FIGHTER_ADMISSION_RECORD_WORDS * 4u,
                    sNdsFtrAdmitChunk,
                    NDS_FIGHTER_ADMISSION_RECORD_WORDS * 4u) == FALSE)
            {
                ndsFtrAdmitLatch(fkind[i], d, nNDSFtrLeanAdmitFailRead, r, 0u,
                                 0u);
                n[i] = 0u;
                continue;
            }
            ndsFtrAdmitRecord(pass, sNdsFtrAdmitChunk, i, fkind[i], d,
                              costume[i], first[i] + r);
        }
    }
    ndsRelocAssetStreamClose(&stream);
}

u32 NDS_FTR_ADMIT_COLD_CODE
ndsFtrLeanAdmitRun(NDSRendererStats *scratch, const u32 *fkind,
                   const u32 *costume, const u32 *detail,
                   const u32 *player, const u32 *content, u32 count,
                   const u32 *base_asset, const void *const *base_data,
                   u32 base_count, u32 word)
{
    NdsRelocAssetStream stream = { NULL };
    NDSFtrAdmitPass pass;
    u32 start = cpuGetTiming();
    u32 i;

    if ((scratch == NULL) || (word == 0u))
    {
        return 0u;
    }
    /* Bank D goes to the textures before the first frame whatever the
     * battle's fighters are: one with no admissible kind (Metal Mario
     * alone) admits nothing, but its stage still needs D. */
    if (word >= NDS_FTR_LEAN_ADMIT_REGIONS)
    {
        ndsFtrAdmitEnterRegions();
    }
    if (count == 0u)
    {
        return 0u;
    }
    memset(&pass, 0, sizeof(pass));
    pass.scratch = scratch;
    NDS_FTR_ADMIT_LAB((gNdsFtrAdmitLab.runs++, gNdsFtrAdmitLab.word = word,
                       gNdsFtrAdmitLab.frame = gNdsRendererProfileFrameCount,
                       gNdsFtrAdmitLab.bases = base_count));
    sNdsFtrAdmitBaseAsset = base_asset;
    sNdsFtrAdmitBaseData = base_data;
    sNdsFtrAdmitBaseCount = ((base_asset != NULL) && (base_data != NULL)) ?
        base_count : 0u;
    for (i = 0u; i < count; i++)
    {
        pass.present |= 1u << (fkind[i] & 31u);
    }
    if (ndsRelocAssetStreamOpen(&stream, "nitro:/fighters/admission.bin") ==
        FALSE)
    {
        sNdsFtrAdmitBaseCount = 0u;
        ndsFtrAdmitLatch(0u, 0u, nNDSFtrLeanAdmitFailOpen, 0u, 0u, 0u);
        return 0u;
    }
    if ((ndsRelocAssetStreamRead(&stream, 0u, sNdsFtrAdmitIndex,
                                 sizeof(sNdsFtrAdmitIndex)) == FALSE) ||
        (sNdsFtrAdmitIndex[0] != NDS_FIGHTER_ADMISSION_MAGIC) ||
        (sNdsFtrAdmitIndex[1] != NDS_FIGHTER_ADMISSION_VERSION) ||
        (sNdsFtrAdmitIndex[2] != NDS_FIGHTER_ADMISSION_KINDS) ||
        (sNdsFtrAdmitIndex[3] != NDS_FIGHTER_ADMISSION_RECORD_WORDS))
    {
        ndsRelocAssetStreamClose(&stream);
        sNdsFtrAdmitBaseCount = 0u;
        ndsFtrAdmitLatch(0u, 0u, nNDSFtrLeanAdmitFailFormat, 0u, 0u, 0u);
        return 0u;
    }
#if NDS_VRAM_CENSUS_LIVE
    if (gNdsFtrAdmitIdentCensus != 0u)
    {
        ndsFtrAdmitIdentCensus(&stream, scratch, pass.present);
    }
#endif
    /* Slice 2c: carve (no libnds record, no libc per texture). Without the
     * carrier the admission falls back to slice 2b's libnds uploads, which
     * the libc floor still guards. */
    (void)ndsFtrCarveOpen((sNdsFtrAdmitRegions != 0u) ? 1u : 0u);
    for (i = 0u; i < count; i++)
    {
        u32 kind = fkind[i];
        /* FTPartsLevelDetail: High 1, Low 2 (ftdef.h) */
        u32 d = (detail[i] == 2u) ? 1u : 0u;
        u32 first;
        u32 n;
        u32 r;

        /* A content's records are its own (ndsFtrAdmitP4Pass). */
        if ((kind >= NDS_FIGHTER_ADMISSION_KINDS) || (content[i] != 0u))
        {
            continue;
        }
        first = sNdsFtrAdmitIndex[4u + (kind * 2u + d) * 2u];
        n = sNdsFtrAdmitIndex[5u + (kind * 2u + d) * 2u];
#if NDS_VRAM_CENSUS_LIVE
        /* Census attribution: these uploads belong to this fighter's slot. */
        gNdsVramCensusDrawSlotPlus1 = (player[i] & 3u) + 1u;
#else
        (void)player;
#endif
        for (r = 0u; r < n; r += NDS_FTR_ADMIT_CHUNK)
        {
            u32 m = ((n - r) < NDS_FTR_ADMIT_CHUNK) ? (n - r) :
                NDS_FTR_ADMIT_CHUNK;
            u32 k;

#if NDS_VRAM_CENSUS_LIVE
            u32 read_start = cpuGetTiming();
            s32 read_ok = ndsRelocAssetStreamRead(
                &stream,
                NDS_FIGHTER_ADMISSION_HEADER_BYTES +
                    (first + r) * NDS_FIGHTER_ADMISSION_RECORD_WORDS * 4u,
                sNdsFtrAdmitChunk,
                m * NDS_FIGHTER_ADMISSION_RECORD_WORDS * 4u);

            gNdsFtrAdmitLab.read_ticks += cpuGetTiming() - read_start;
            if (read_ok == FALSE)
#else
            if (ndsRelocAssetStreamRead(
                    &stream,
                    NDS_FIGHTER_ADMISSION_HEADER_BYTES +
                        (first + r) * NDS_FIGHTER_ADMISSION_RECORD_WORDS * 4u,
                    sNdsFtrAdmitChunk,
                    m * NDS_FIGHTER_ADMISSION_RECORD_WORDS * 4u) == FALSE)
#endif
            {
                ndsFtrAdmitLatch(kind, d, nNDSFtrLeanAdmitFailRead, r, 0u, 0u);
                break;
            }
            for (k = 0u; k < m; k++)
            {
                ndsFtrAdmitRecord(
                    &pass,
                    &sNdsFtrAdmitChunk[k * NDS_FIGHTER_ADMISSION_RECORD_WORDS],
                    i, kind, d, costume[i], first + r + k);
            }
        }
    }
    ndsRelocAssetStreamClose(&stream);
    ndsFtrAdmitP4Pass(&pass, fkind, costume, detail, player, content, count);
    ndsFtrCarveClose();
    if (sNdsFtrCarveLive == 0u)
    {
        ndsFtrLeanAdmitCarveRetire();
    }
    sNdsFtrAdmitBaseCount = 0u;
#if NDS_VRAM_CENSUS_LIVE
    gNdsVramCensusDrawSlotPlus1 = 0u;
    ndsFtrAdmitLabFree(&gNdsFtrAdmitLab.free_after,
                       &gNdsFtrAdmitLab.largest_after);
    gNdsFtrAdmitLab.ticks += cpuGetTiming() - start;
    gNdsFtrAdmitLab.done = 1u;
    DC_FlushRange(&gNdsFtrAdmitLab, sizeof(gNdsFtrAdmitLab));
#else
    (void)start;
#endif
    return pass.admitted;
}

/* Battle exit: admitted textures stop being special, and bank D goes back to
 * BG3 before any later scene can use it.
 *  - Every cache entry living in D is released, so the next scene's draws
 *    re-resolve into A+B instead of binding texels that are about to go.
 *  - D is locked and A+B unlocked (the lock word the battle found, plus D),
 *    so nothing new is allocated in D; its texels stay intact, which keeps
 *    the final battle frame -- still on screen during the hand-off, and
 *    still reading D -- correct.
 *  - The platform remaps D to BG3 after the next scene's first 3D frame is
 *    displayed, or at that scene's first BG3 write or clear.
 * Names outside the cache that still live in D (weapons, effects created
 * after the lock) are left to their owners and to the next scene reset's
 * glResetTextures; deleting them here would let libnds hand the same name to
 * someone else while the old owner still holds it. The one owner that
 * outlives the battle is the renderer's own fighter tint set, which VS Results
 * and the character select draw with: its tiles in D go here (slice 7,
 * ndsR2FighterTintReleaseRange). */
void NDS_FTR_ADMIT_COLD_CODE ndsFtrLeanAdmitBattleExit(void)
{
    u32 i;

    for (i = 0u; i < NDS_RENDERER_HW_TEXTURE_CACHE_COUNT; i++)
    {
        sNdsRendererHardwareTextureCache[i].admitted = 0u;
    }
    NDS_FTR_ADMIT_LAB((gNdsFtrAdmitLab.exit_runs++, gNdsFtrAdmitLab.done = 0u));
    if (sNdsFtrAdmitRegions == 0u)
    {
        return;
    }
    /* Slice 2c: D is locked before anything in it is released, so a carve
     * region that goes back to libnds with its last entry cannot be handed
     * out again while D still holds the final frame's texels. */
    glGlobalData.vramLock[0] = sNdsFtrAdmitSavedLock;
    glLockVRAMBank((u16 *)VRAM_D);
    for (i = 0u; i < NDS_RENDERER_HW_TEXTURE_CACHE_COUNT; i++)
    {
        NDSRendererHardwareTextureCacheEntry *entry =
            &sNdsRendererHardwareTextureCache[i];
        u32 addr = ndsFtrAdmitEntryAddress(entry);

        if ((addr >= NDS_FTR_ADMIT_D_LO) && (addr < NDS_FTR_ADMIT_D_HI))
        {
            NDS_FTR_ADMIT_LAB(if ((entry->carve_flags &
                                   NDS_FTR_CARVE_F_CARVED) != 0u)
                              { gNdsFtrCarveLab.exit_released++; });
            (void)ndsRendererHardwareReleaseTexture(entry);
            NDS_FTR_ADMIT_LAB(gNdsFtrAdmitLab.exit_released++);
        }
    }
    {
        u32 tints = ndsR2FighterTintReleaseRange(NDS_FTR_ADMIT_D_LO,
                                                 NDS_FTR_ADMIT_D_HI);

        NDS_FTR_ADMIT_LAB(gNdsFtrAdmitLab.exit_tints += tints);
        (void)tints;
    }
#if NDS_VRAM_CENSUS_LIVE
    {
        const DynamicArray *textures = &glGlobalData.texturePtrs;
        int name;

        for (name = 1; (u32)name < textures->cur_size; name++)
        {
            u32 addr = ndsFtrAdmitNameAddress(name);

            if ((addr >= NDS_FTR_ADMIT_D_LO) && (addr < NDS_FTR_ADMIT_D_HI))
            {
                gNdsFtrAdmitLab.exit_orphans++;
            }
        }
    }
#endif
    sNdsRendererHardwareBoundTextureName = 0u;
    sNdsRendererHardwareActiveTextureEntry = NULL;
    ndsPlatformVramReturnBankD();
    sNdsFtrAdmitRegions = 0u;
    sNdsFtrAdmitLocked = 0u;
    NDS_FTR_ADMIT_LAB((gNdsFtrAdmitLab.regions = 0u,
                       gNdsFtrAdmitLab.locked = 0u));
#if NDS_VRAM_CENSUS_LIVE
    /* The exit is read from the Results screen (gdb reads memory). */
    DC_FlushRange(&gNdsFtrAdmitLab, sizeof(gNdsFtrAdmitLab));
    ndsFtrCarveLabPublish();
#endif
}

static void NDS_FTR_ADMIT_COLD_CODE ndsFtrAdmitSceneResetGuard(void)
{
    if (sNdsFtrAdmitRegions != 0u)
    {
        NDS_FTR_ADMIT_LAB(gNdsFtrAdmitLab.exit_safety++);
        ndsFtrLeanAdmitBattleExit();
    }
    /* Slice 2c: glResetTextures is about to rebuild both allocators; carved
     * entries, regions and the carrier go first, while their indices mean
     * something. */
    ndsFtrLeanAdmitCarveRetire();
}
