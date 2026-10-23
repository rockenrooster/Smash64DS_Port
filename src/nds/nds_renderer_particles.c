/* Native renderer, particles: the atlas and its variants, the KO pillar and
 * environment palettes, the particle camera and quads, and the packeted
 * particle FIFO. A slice of nds_renderer.c. */

/* R2-07 particle draw path -- the upload half.
 *
 * ONE atlas, one GL name, and therefore ONE bind for every particle in a
 * frame. That is the whole reason it is an atlas: GL names are the binding
 * constraint (the battle's source/static corpus already owns a measured cache
 * partition), and a per-frame scheme would break the triangle batch on every
 * texture change. The generator's measured-safe 64x64 allocation fits the existing
 * scratch and leaves the stage texture resolver on its prepared path.
 *
 * Pinned like the static set, so the cache's LRU cannot evict it mid-match. It
 * is allocated AFTER them and therefore lands above their VRAM span, which is
 * why this does not disturb the static set's first/end address assertions. */
volatile u32 gNdsRendererParticleAtlasPrepareCount;
volatile u32 gNdsRendererParticleAtlasFailCount;
volatile u32 gNdsRendererParticleAtlasBytes;
__attribute__((used)) volatile u32 gNdsRendererHealSparkleCoverageSubmitCount;
static u32 sNdsRendererHealSparkleCellKey[2];
static u8 sNdsRendererHealSparkleSheet[2];
static u32 sNdsRendererHealSparkleCellValidMask;
volatile u32 gNdsRendererWhispyNativePrepareCount;
volatile u32 gNdsRendererWhispyNativeFailCount;
volatile u32 gNdsRendererWhispyNativeBytes;
volatile u32 gNdsRendererFoxBlasterGlowPrepareCount;
volatile u32 gNdsRendererFoxBlasterGlowFailCount;
volatile u32 gNdsRendererFoxBlasterGlowBytes;
/* BUGS.md "Fox's pistol model is missing". DrawCount is the engagement proof:
 * the model-part STATE half already rises (gNdsFighterModelPartOnCount), so a
 * zero here means the overlay was skipped, not that the gun was never asked
 * for -- exactly the distinction this row took three cycles to make. */
volatile u32 gNdsRendererFoxGunPrepareCount;
volatile u32 gNdsRendererFoxGunFailCount;
volatile u32 gNdsRendererFoxGunBytes;
volatile u32 gNdsRendererFoxGunDrawCount;
volatile u32 gNdsRendererFoxGunTriangleCount;

/* Bumped by every writer of the particle sheet, variant and binding tables
 * (the particle lookup memos). */
static u32 sNdsParticleMaterialGen = 1u;

/* Sheets already uploaded when a later one fails are direct scene-owned GL
 * names. Reclaim those names here so a partial prepare cannot leak texture or
 * palette VRAM for the rest of the scene. */
NDS_RENDERER_COLD static void ndsRendererParticleAtlasReleaseSheets(void)
{
    u32 sheet;

    sNdsParticleMaterialGen++;
    for (sheet = 0u; sheet < NDS_PARTICLE_QUAD_ATLAS_SHEETS; sheet++)
    {
        if (sNdsRendererParticleAtlasName[sheet] != 0)
        {
            if (sNdsRendererHardwareBoundTextureName ==
                (u32)sNdsRendererParticleAtlasName[sheet])
            {
                sNdsRendererHardwareBoundTextureName = 0u;
            }
            ndsRendererHardwareFencedGlDeleteTextures(
                1, &sNdsRendererParticleAtlasName[sheet]);
            sNdsRendererParticleAtlasName[sheet] = 0;
        }
        memset(&sNdsRendererParticleAtlasBinding[sheet], 0,
               sizeof(sNdsRendererParticleAtlasBinding[sheet]));
    }
}

#if NDS_VRAM_CENSUS_LIVE
/* P8 loop V0, census lab builds only: gNdsLabParticleReserveSheets (poked
 * before the match) more 8,192-byte texture allocations, taken right after
 * the atlas sheets -- the size and the moment more sheets would take -- so
 * one ROM measures how many more texture VRAM holds before stage or fighter
 * textures are refused. Released with the atlas; Taken says how many were.
 * With gNdsLabParticleReserveAfterCountdown they are taken instead when the
 * countdown's atlases go (battleship_ifcommon.c), evicting texture cache
 * entries to make room as the interface atlases' prepare does. */
#define NDS_LAB_PARTICLE_RESERVE_MAX 24u
volatile u32 gNdsLabParticleReserveSheets __attribute__((used));
volatile u32 gNdsLabParticleReserveAfterCountdown __attribute__((used));
volatile u32 gNdsLabParticleReserveTaken __attribute__((used));
static int sNdsLabParticleReserveName[NDS_LAB_PARTICLE_RESERVE_MAX];

static void ndsLabParticleReserveRelease(void)
{
    u32 i;

    for (i = 0u; i < NDS_LAB_PARTICLE_RESERVE_MAX; i++)
    {
        if (sNdsLabParticleReserveName[i] != 0)
        {
            if (sNdsRendererHardwareBoundTextureName ==
                (u32)sNdsLabParticleReserveName[i])
            {
                sNdsRendererHardwareBoundTextureName = 0u;
            }
            ndsRendererHardwareFencedGlDeleteTextures(
                1, &sNdsLabParticleReserveName[i]);
            sNdsLabParticleReserveName[i] = 0;
        }
    }
}

static void ndsLabParticleReserveTake(int size_x, int size_y, s32 evict)
{
    u32 i;

    gNdsLabParticleReserveTaken = 0u;
    for (i = 0u; (i < gNdsLabParticleReserveSheets) &&
                 (i < NDS_LAB_PARTICLE_RESERVE_MAX); i++)
    {
        int name = 0;
        u32 attempts = 0u;

        if (ndsRendererHardwareFencedGlGenTextures(1, &name) == 0)
        {
            return;
        }
        sNdsLabParticleReserveName[i] = name;
        ndsRendererHardwareBindTextureName(NULL, (u32)name);
        while (ndsRendererHardwareFencedGlTexImage2D(
                   GL_TEXTURE_2D, 0, GL_RGB32_A3, size_x, size_y, 0,
                   TEXGEN_TEXCOORD, sNdsRendererHardwareTextureScratch) == 0)
        {
            if ((evict == FALSE) ||
                (++attempts >= NDS_RENDERER_HW_TEXTURE_CACHE_COUNT) ||
                (ndsRendererHardwareEvictTexture(NULL) == FALSE))
            {
                return;
            }
            ndsRendererHardwareBindTextureName(NULL, (u32)name);
        }
        gNdsLabParticleReserveTaken = i + 1u;
    }
}

void ndsLabParticleReserveAfterCountdown(void)
{
    int size_x;
    int size_y;

    if ((gNdsLabParticleReserveAfterCountdown == 0u) ||
        (sNdsRendererParticleAtlasPrepared == 0u) ||
        (ndsRendererHardwareTextureSizeEnum(
             NDS_PARTICLE_QUAD_ATLAS_WIDTH, &size_x) == FALSE) ||
        (ndsRendererHardwareTextureSizeEnum(
             NDS_PARTICLE_QUAD_ATLAS_HEIGHT, &size_y) == FALSE))
    {
        return;
    }
    ndsLabParticleReserveRelease();
    ndsRendererHardwareEndBatch();
    ndsLabParticleReserveTake(size_x, size_y, TRUE);
}
#endif

/* Three source textures, three hardware-native encodings selected AOT by the
 * generator: A5I3 for IA16 dust, A3I5 for I8 air, and lossless PAL16 for leaf
 * frame zero. Upload happens once at scene prepare; the hot path receives only
 * GL names and full-texture UVs. */
NDS_RENDERER_COLD static s32 ndsRendererHardwarePrepareWhispyNativeTextures(void)
{
    FILE *file = NULL;
    u32 texture;

    gNdsRendererWhispyNativePrepareCount++;
    file = ndsRendererHardwareFencedTextureFopen(
        NDS_WHISPY_NATIVE_ASSET_PATH, "rb");
    if (file == NULL)
    {
        goto fail;
    }
    for (texture = 0u; texture < NDS_WHISPY_NATIVE_TEXTURE_COUNT; texture++)
    {
        int name = 0;
        u32 format;
        u32 width;
        u32 height;
        u32 texel_offset;
        u32 texel_bytes;
        u32 palette_offset;
        u32 palette_entries;
        int gl_format;
        int params = TEXGEN_TEXCOORD;
        int size_x;
        int size_y;
        int palette_format = -1;

        switch (texture)
        {
        case 0u:
            format = NDS_WHISPY_NATIVE_TEXTURE_0_FORMAT;
            width = NDS_WHISPY_NATIVE_TEXTURE_0_WIDTH;
            height = NDS_WHISPY_NATIVE_TEXTURE_0_HEIGHT;
            texel_offset = NDS_WHISPY_NATIVE_TEXTURE_0_TEXEL_OFFSET;
            texel_bytes = NDS_WHISPY_NATIVE_TEXTURE_0_TEXEL_BYTES;
            palette_offset = NDS_WHISPY_NATIVE_TEXTURE_0_PALETTE_OFFSET;
            palette_entries = NDS_WHISPY_NATIVE_TEXTURE_0_PALETTE_ENTRIES;
            break;
        case 1u:
            format = NDS_WHISPY_NATIVE_TEXTURE_1_FORMAT;
            width = NDS_WHISPY_NATIVE_TEXTURE_1_WIDTH;
            height = NDS_WHISPY_NATIVE_TEXTURE_1_HEIGHT;
            texel_offset = NDS_WHISPY_NATIVE_TEXTURE_1_TEXEL_OFFSET;
            texel_bytes = NDS_WHISPY_NATIVE_TEXTURE_1_TEXEL_BYTES;
            palette_offset = NDS_WHISPY_NATIVE_TEXTURE_1_PALETTE_OFFSET;
            palette_entries = NDS_WHISPY_NATIVE_TEXTURE_1_PALETTE_ENTRIES;
            break;
        default:
            format = NDS_WHISPY_NATIVE_TEXTURE_2_FORMAT;
            width = NDS_WHISPY_NATIVE_TEXTURE_2_WIDTH;
            height = NDS_WHISPY_NATIVE_TEXTURE_2_HEIGHT;
            texel_offset = NDS_WHISPY_NATIVE_TEXTURE_2_TEXEL_OFFSET;
            texel_bytes = NDS_WHISPY_NATIVE_TEXTURE_2_TEXEL_BYTES;
            palette_offset = NDS_WHISPY_NATIVE_TEXTURE_2_PALETTE_OFFSET;
            palette_entries = NDS_WHISPY_NATIVE_TEXTURE_2_PALETTE_ENTRIES;
            break;
        }
        NDS_RENDERER_CAPACITY_HIGH_WATER(
            gNdsRendererTextureScratchWhispyBytesHighWater, texel_bytes);
        if ((texel_bytes > sizeof(sNdsRendererHardwareTextureScratch)) ||
            (palette_entries > NDS_PARTICLE_QUAD_PALETTE_ENTRIES) ||
            (ndsRendererHardwareTextureSizeEnum(width, &size_x) == FALSE) ||
            (ndsRendererHardwareTextureSizeEnum(height, &size_y) == FALSE) ||
            (ndsRendererHardwareFencedTextureFseek(
                 file, (long)texel_offset, SEEK_SET) != 0) ||
            (ndsRendererHardwareFencedTextureFread(
                 sNdsRendererHardwareTextureScratch, 1, texel_bytes, file) !=
             texel_bytes) ||
            (ndsRendererHardwareFencedTextureFseek(
                 file, (long)palette_offset, SEEK_SET) != 0) ||
            (ndsRendererHardwareFencedTextureFread(
                 sNdsRendererParticleAtlasPalette, sizeof(u16),
                 palette_entries, file) != palette_entries))
        {
            goto fail;
        }
        if (format == NDS_PARTICLE_FORMAT_A5I3)
        {
            gl_format = GL_RGB8_A5;
        }
        else if (format == NDS_PARTICLE_FORMAT_A3I5)
        {
            gl_format = GL_RGB32_A3;
        }
        else if (format == NDS_PARTICLE_FORMAT_PAL16)
        {
            gl_format = GL_RGB16;
            params |= GL_TEXTURE_COLOR0_TRANSPARENT;
        }
        else
        {
            goto fail;
        }

        /* Immutable AOT owners have their own lifetime/name table already. Do
         * not consume a dynamic material-cache entry merely to hold that GL
         * name: the four-player texture demand needs those entries for the
         * source fighter materials touched in the current frame. */
        if (ndsRendererHardwareFencedGlGenTextures(1, &name) == 0)
        {
            goto fail;
        }
        sNdsRendererWhispyNativeName[texture] = name;
        ndsRendererHardwareBindTextureName(NULL, (u32)name);
        if (ndsRendererHardwareFencedGlTexImage2D(
                GL_TEXTURE_2D, 0, gl_format, size_x, size_y, 0, params,
                sNdsRendererHardwareTextureScratch) == 0)
        {
            goto fail;
        }
        glColorTableEXT(GL_TEXTURE_2D, 0, (int)palette_entries, 0, 0,
                        sNdsRendererParticleAtlasPalette);
        /* GFX_PAL_FORMAT is a write-only GX command port, not readable state.
         * Ask libnds for the palette object's encoded PLTT_BASE word so the
         * direct binder and packet stream reproduce glBindTexture exactly. */
        glGetColorTableParameterEXT(
            GL_TEXTURE_2D, GL_COLOR_TABLE_FORMAT_EXT, &palette_format);
        if (palette_format < 0)
        {
            goto fail;
        }
        sNdsRendererWhispyNativeBinding[texture].texture_name =
            (u32)name;
        sNdsRendererWhispyNativeBinding[texture].texture_format =
            (u32)glGetTexParameter();
        sNdsRendererWhispyNativeBinding[texture].palette_format =
            (u32)palette_format;
        sNdsRendererWhispyNativeBinding[texture].palette_name =
            (s32)glGlob->activePalette;
        sNdsRendererWhispyNativeBinding[texture].valid = TRUE;
    }
    if (ndsRendererHardwareFencedTextureFclose(file) != 0)
    {
        file = NULL;
        goto fail;
    }
    gNdsRendererWhispyNativeBytes = NDS_WHISPY_NATIVE_ASSET_BYTES;
    return TRUE;

fail:
    if (file != NULL)
    {
        (void)ndsRendererHardwareFencedTextureFclose(file);
    }
    gNdsRendererWhispyNativeFailCount++;
    return FALSE;
}

/* EFCommon texture 27 is already an exact DS PAL16 payload in the generated
 * particle asset: 16x8 CI4 + fourteen RGBA5551 colours becomes 64 texel bytes
 * plus 28 palette bytes with zero model error. Script 0x62 immediately issues
 * MASKT, making that stored image the TOP HALF of a 16x16 disc. Give it its
 * own immutable GL name so the DS can do the same reflection in TEXIMAGE_PARAM
 * (WRAP_T|FLIP_T); an atlas cell cannot wrap without sampling its neighbours.
 */
NDS_RENDERER_COLD static s32 ndsRendererHardwarePrepareFoxBlasterGlowTexture(void)
{
    const NDSParticleTexture *texture =
        &gNdsParticleTextures[27u];
    FILE *file = NULL;
    int name = 0;
    u32 texel_bytes;
    int palette_format = -1;
    int size_x;
    int size_y;
    int params = TEXGEN_TEXCOORD | GL_TEXTURE_COLOR0_TRANSPARENT |
                 GL_TEXTURE_WRAP_T | GL_TEXTURE_FLIP_T;

    gNdsRendererFoxBlasterGlowPrepareCount++;
    texel_bytes = ((u32)texture->width * (u32)texture->height) >> 1;
    NDS_RENDERER_CAPACITY_HIGH_WATER(
        gNdsRendererTextureScratchFoxGlowBytesHighWater, texel_bytes);
    if ((texture->width != 16u) || (texture->height != 8u) ||
        (texture->ds_format != NDS_PARTICLE_FORMAT_PAL16) ||
        (texture->palette_entries != 14u) ||
        (texture->data_offset == NDS_PARTICLE_TEXTURE_UNPACKED) ||
        (texture->palette_offset == NDS_PARTICLE_TEXTURE_UNPACKED) ||
        (texel_bytes != 64u) ||
        (texel_bytes > sizeof(sNdsRendererHardwareTextureScratch)) ||
        ((u32)texture->palette_entries >
         NDS_PARTICLE_QUAD_PALETTE_ENTRIES) ||
        (ndsRendererHardwareTextureSizeEnum(texture->width, &size_x) ==
         FALSE) ||
        (ndsRendererHardwareTextureSizeEnum(texture->height, &size_y) ==
         FALSE))
    {
        goto fail;
    }
    file = ndsRendererHardwareFencedTextureFopen(
        NDS_PARTICLE_TEXTURE_ASSET_PATH, "rb");
    if ((file == NULL) ||
        (ndsRendererHardwareFencedTextureFseek(
             file, (long)texture->data_offset, SEEK_SET) != 0) ||
        (ndsRendererHardwareFencedTextureFread(
             sNdsRendererHardwareTextureScratch, 1, texel_bytes, file) !=
         texel_bytes) ||
        (ndsRendererHardwareFencedTextureFseek(
             file,
             (long)(NDS_PARTICLE_PALETTE_ASSET_OFFSET +
                    ((u32)texture->palette_offset * sizeof(u16))),
             SEEK_SET) != 0) ||
        (ndsRendererHardwareFencedTextureFread(
             sNdsRendererParticleAtlasPalette, sizeof(u16),
             texture->palette_entries, file) != texture->palette_entries))
    {
        goto fail;
    }
    if (ndsRendererHardwareFencedGlGenTextures(1, &name) == 0)
    {
        goto fail;
    }
    sNdsRendererFoxBlasterGlowName = name;
    ndsRendererHardwareBindTextureName(NULL, (u32)name);
    if (ndsRendererHardwareFencedGlTexImage2D(
            GL_TEXTURE_2D, 0, GL_RGB16, size_x, size_y, 0, params,
            sNdsRendererHardwareTextureScratch) == 0)
    {
        goto fail;
    }
    glColorTableEXT(GL_TEXTURE_2D, 0, texture->palette_entries, 0, 0,
                    sNdsRendererParticleAtlasPalette);
    glGetColorTableParameterEXT(
        GL_TEXTURE_2D, GL_COLOR_TABLE_FORMAT_EXT, &palette_format);
    if (palette_format < 0)
    {
        goto fail;
    }
    sNdsRendererWhispyNativeBinding[
        NDS_WHISPY_NATIVE_TEXTURE_COUNT].texture_name = (u32)name;
    sNdsRendererWhispyNativeBinding[
        NDS_WHISPY_NATIVE_TEXTURE_COUNT].texture_format =
            (u32)glGetTexParameter();
    sNdsRendererWhispyNativeBinding[
        NDS_WHISPY_NATIVE_TEXTURE_COUNT].palette_format =
        (u32)palette_format;
    sNdsRendererWhispyNativeBinding[
        NDS_WHISPY_NATIVE_TEXTURE_COUNT].palette_name =
        (s32)glGlob->activePalette;
    sNdsRendererWhispyNativeBinding[
        NDS_WHISPY_NATIVE_TEXTURE_COUNT].valid = TRUE;
    if (ndsRendererHardwareFencedTextureFclose(file) != 0)
    {
        file = NULL;
        goto fail;
    }
    file = NULL;
    gNdsRendererFoxBlasterGlowBytes =
        texel_bytes + ((u32)texture->palette_entries * sizeof(u16));
    return TRUE;

fail:
    if (file != NULL)
    {
        (void)ndsRendererHardwareFencedTextureFclose(file);
    }
    if (sNdsRendererFoxBlasterGlowName != 0)
    {
        ndsRendererHardwareFencedGlDeleteTextures(
            1, &sNdsRendererFoxBlasterGlowName);
    }
    sNdsRendererFoxBlasterGlowName = 0;
    memset(&sNdsRendererWhispyNativeBinding[
               NDS_WHISPY_NATIVE_TEXTURE_COUNT], 0,
           sizeof(sNdsRendererWhispyNativeBinding[0]));
    gNdsRendererFoxBlasterGlowBytes = 0u;
    gNdsRendererFoxBlasterGlowFailCount++;
    return FALSE;
}

/* KO BLAST PILLAR ENVIRONMENT VARIANTS.
 *
 * The source KO leaf scripts (efcommon 46/48/49/50/54: texture 10/11/13/14/18)
 * all write ENVCOLOR, so their pixel is (PRIM - ENV) * TEXEL + ENV
 * (lbparticle.c:2053-2065). The quad sheet carries shape only -- every cell is
 * near-white and takes its colour from the one vertex colour -- so no single
 * vertex tint can be both ends of that ramp, and dropping ENV collapses the
 * pillar to thin streaks. Bake the lerp into the palette instead, reusing
 * ndsRendererHardwareBlendPrimEnvTexel0 (forward-declared; defined below),
 * and leave the vertex white with the alpha path untouched.
 *
 * A variant is keyed on (sheet, prim, env): prim ramps through a handful of
 * values per burst (script 46: 0xFFF94A -> 0xFFFF97) while env is constant per
 * script (script 46: 0x42FF21; script 54: 0xFF9305), so eight round-robin
 * entries cover a live burst and a miss only rebakes 32 entries plus a 64-byte
 * palette upload. Names are palette-only and share the sheet image through
 * glAssignColorTable -- no image re-upload, same mechanism as the entry-effect
 * KO palettes (nds_renderer_native_common.c) -- so each entry costs 64 bytes
 * of palette RAM and one GL name, 512 bytes total, and nothing from the atlas
 * budget (31,872 of 32,768 bytes; 896 free).
 *
 * Alpha is excluded from the key (and from the bake, which keeps the sheet
 * coverage bit): the 8-bit source alpha still becomes POLYGON_ATTR per quad,
 * exactly as the legacy path does.
 *
 * P2-2p8 (2026-10-05): DEFERRED REWRITES. Eight entries did not cover a burst:
 * the gate's KO frames carry up to 13 distinct (sheet, prim, env) keys a frame
 * (particles of different ages sit at different points of the prim ramp), so
 * the round robin rebaked 124 times in 19 frames, and each rebake was a
 * glColorTableEXT on a live name -- libnds frees the palette block, allocates
 * a new one (a walk of ~300 vramBlock nodes) and maps banks F/G to LCD
 * mid-frame, ~11.7K cycles. Worse, it rewrote palettes the frame on screen
 * was still rendering from (the previous frame's KO quads), and unmapped every
 * palette from the 3D engine while it rendered.
 *
 * Now each of 16 entries allocates its palette once and keeps it. A miss
 * takes the least recently used entry that NO draw of the frame being built
 * references, bakes into that entry's staging copy and queues it; the copy
 * lands in the palette at the VBlank that swaps the frame in
 * (ndsRendererParticleEnvVariantCommit, called from the platform's
 * scheduled-VBlank wait), when the frame it replaces has stopped rendering
 * and the next has not started. 16 entries: the burst's 44 distinct keys bake
 * 56 times and no frame runs out (the trace replay, 2026-10-05). 1,024 bytes
 * of palette RAM. */
#define NDS_PARTICLE_ENV_VARIANT_COUNT 16u
#define NDS_PARTICLE_ENV_VARIANT_WHITE 0x7FFFu
typedef struct NDSParticleEnvVariant
{
    u32 valid;
    u32 sheet;
    u32 prim;
    u32 env;
    int name;
    u32 palette_format;
    s32 palette_name;
    /* The palette's LCD address (NULL: never rewritten in place), the banks
     * it spans (bit 0 E, 1 F, 2 G), the last epoch that referenced it, and a
     * refused allocation (palette RAM full) that is not retried. */
    u32 *vram;
    u32 banks;
    u32 epoch;
    u32 refused;
} NDSParticleEnvVariant;
static NDSParticleEnvVariant
    sNdsParticleEnvVariants[NDS_PARTICLE_ENV_VARIANT_COUNT];
/* One epoch per presented frame; entries start at 0, never the current one. */
static u32 sNdsParticleEnvVariantEpoch = 1u;
static u32 sNdsParticleEnvVariantPending;
static u32 sNdsParticleEnvVariantPendingBanks;
/* gNdsRendererSceneTextureVramResetCount at the queue: a reset since then
 * retired the palettes the queue targets. */
static u32 sNdsParticleEnvVariantPendingReset;
static u32 sNdsParticleEnvVariantStage[NDS_PARTICLE_ENV_VARIANT_COUNT]
                                      [NDS_PARTICLE_QUAD_PALETTE_ENTRIES / 2u];
volatile u32 gNdsParticleEnvVariantCommits;
/* P2-2p8 (2026-10-05): a burst's particles share one sheet, prim and env, and
 * each quad searched the sheet names and the variant table twice (the variant
 * for its colours, then the packet binding for that variant) to find the
 * answer the previous quad found. Both lookups are pure reads of the sheet,
 * variant and binding tables; every writer of those tables bumps this
 * generation, and a one-entry memo of each lookup holds only while it is
 * unchanged. */
static u32 sNdsParticleEnvMemoGen;
static u32 sNdsParticleEnvMemoAtlas;
static u32 sNdsParticleEnvMemoPrim;
static u32 sNdsParticleEnvMemoEnv;
static u32 sNdsParticleEnvMemoName;
/* Palette-only names holding each sheet's ORIGINAL palette, created the first
 * time that sheet bakes a variant. They exist because glAssignColorTable
 * copies the palette of the NAMED texture onto the BOUND one, so assigning a
 * variant onto a sheet overwrites the sheet's own palette pointer, and
 * assigning the sheet from itself afterwards is a no-op -- every later
 * particle on that sheet would keep the KO ramp. 64 bytes of palette RAM per
 * sheet that ever uses a variant, and none for a sheet that never does. */
static int sNdsParticleBasePaletteName[NDS_PARTICLE_QUAD_ATLAS_SHEETS];
static u32
    sNdsParticleBasePaletteFormat[NDS_PARTICLE_QUAD_ATLAS_SHEETS];
static s32
    sNdsParticleBasePaletteActive[NDS_PARTICLE_QUAD_ATLAS_SHEETS];
/* Any GL resource bind/upload performed while a particle primitive is open
 * invalidates the hardware material state that buffered vertices would have
 * inherited.  Resource writers flush first, then set this bit so the next
 * packet run emits TEXIMAGE_PARAM + PLTT_BASE even if the logical sheet key
 * did not change. */
static u32 sNdsRendererParticlePacketStateDirty;
static void ndsRendererFlushWhispyNativePacket(void);
volatile u32 gNdsParticleEnvVariantBakeCount;
volatile u32 gNdsParticleEnvVariantHitCount;
/* ENVCOLOR was set and no variant could be produced, so the quad drew with the
 * primitive tint alone -- the very defect this cache exists to remove. Not a
 * silent fallback: this is the recorded evidence that it happened. */
volatile u32 gNdsParticleEnvVariantFallbackCount;
volatile u32 gNdsParticleQuadPaletteBreaks;

static u16 ndsRendererHardwareBlendPrimEnvTexel0(u16 texel0,
                                                 u32 primitive,
                                                 u32 environment);

static u32 ndsRendererParticleSheetForName(u32 atlas_name)
{
    u32 sheet;

    for (sheet = 0u; sheet < NDS_PARTICLE_QUAD_ATLAS_SHEETS; sheet++)
    {
        if (atlas_name == (u32)sNdsRendererParticleAtlasName[sheet])
        {
            return sheet;
        }
    }
    return NDS_PARTICLE_QUAD_ATLAS_SHEETS;
}

/* The entry a deferred miss may take: one never allocated, else the least
 * recently referenced entry no draw of the frame being built references
 * (an older epoch), so the queued rewrite cannot change a palette this
 * frame's queued draws already point at. NDS_PARTICLE_ENV_VARIANT_COUNT when
 * every entry is in this frame's use. */
static u32 ndsRendererParticleEnvVariantVictim(void)
{
    const u32 epoch = sNdsParticleEnvVariantEpoch;
    u32 best = NDS_PARTICLE_ENV_VARIANT_COUNT;
    u32 best_age = 0u;
    u32 slot;

    for (slot = 0u; slot < NDS_PARTICLE_ENV_VARIANT_COUNT; slot++)
    {
        const NDSParticleEnvVariant *variant = &sNdsParticleEnvVariants[slot];
        u32 age;

        if (variant->refused != 0u)
        {
            continue;
        }
        if (variant->valid == 0u)
        {
            return slot;
        }
        if (variant->vram == NULL)
        {
            continue;
        }
        age = epoch - variant->epoch;
        if (age > best_age)
        {
            best_age = age;
            best = slot;
        }
    }
    return best;
}

/* Palette-only GL name for (sheet, prim, env), or 0 to stay on the legacy
 * vertex-tint path. 0 also covers non-sheet images (Hyrule native names),
 * which keep today's behaviour rather than failing closed. */
static u32 ndsRendererParticleEnvVariant(u32 atlas_name, u32 prim_bgr555,
                                         u32 env_word)
{
    u32 sheet;
    u32 prim_key;
    u32 env_key;
    u32 slot;
    u32 entry;
    u32 prim_word;
    u32 r5;
    u32 g5;
    u32 b5;
    u16 *bake;
    NDSParticleEnvVariant *variant;
    int name = 0;
    int palette_width = 0;

    sheet = ndsRendererParticleSheetForName(atlas_name);
    if (sheet >= NDS_PARTICLE_QUAD_ATLAS_SHEETS)
    {
        return 0u;
    }
    prim_key = prim_bgr555 & 0x7FFFu;
    env_key = env_word & 0xFFFFFF00u;
    for (slot = 0u; slot < NDS_PARTICLE_ENV_VARIANT_COUNT; slot++)
    {
        if ((sNdsParticleEnvVariants[slot].valid != 0u) &&
            (sNdsParticleEnvVariants[slot].sheet == sheet) &&
            (sNdsParticleEnvVariants[slot].prim == prim_key) &&
            (sNdsParticleEnvVariants[slot].env == env_key))
        {
            NDS_DIAG(gNdsParticleEnvVariantHitCount++);
            sNdsParticleEnvVariants[slot].epoch = sNdsParticleEnvVariantEpoch;
            return (u32)sNdsParticleEnvVariants[slot].name;
        }
    }
    slot = ndsRendererParticleEnvVariantVictim();
    if (slot >= NDS_PARTICLE_ENV_VARIANT_COUNT)
    {
        return 0u;
    }
    variant = &sNdsParticleEnvVariants[slot];
    /* A miss rewrites the tables below (bake, base palette, failures). */
    sNdsParticleMaterialGen++;
    /* 5-to-8 expansion keeps the top five bits identical to the source
     * channels, which is all ndsRendererHardwareBlendPrimEnvTexel0 reads, so
     * rebuilding the word from BGR555 is exact rather than lossy. */
    r5 = prim_key & 31u;
    g5 = (prim_key >> 5) & 31u;
    b5 = (prim_key >> 10) & 31u;
    prim_word = (((r5 << 3) | (r5 >> 2)) << 24) |
                (((g5 << 3) | (g5 >> 2)) << 16) |
                (((b5 << 3) | (b5 >> 2)) << 8) | 0xFFu;
    bake = (u16 *)sNdsParticleEnvVariantStage[slot];
    for (entry = 0u; entry < NDS_PARTICLE_QUAD_PALETTE_ENTRIES; entry++)
    {
        bake[entry] = ndsRendererHardwareBlendPrimEnvTexel0(
            sNdsRendererParticleAtlasPalette[
                (sheet * NDS_PARTICLE_QUAD_PALETTE_ENTRIES) + entry],
            prim_word, env_key);
    }
    if (variant->valid != 0u)
    {
        /* Queued: the palette changes at the VBlank that swaps this frame
         * in. No GL call, so an open packet keeps its material state. */
        sNdsParticleEnvVariantPending |= 1u << slot;
        sNdsParticleEnvVariantPendingBanks |= variant->banks;
        sNdsParticleEnvVariantPendingReset =
            gNdsRendererSceneTextureVramResetCount;
        variant->sheet = sheet;
        variant->prim = prim_key;
        variant->env = env_key;
        variant->epoch = sNdsParticleEnvVariantEpoch;
        sNdsParticleMaterialGen++;
        gNdsParticleEnvVariantBakeCount++;
        return (u32)variant->name;
    }
    /* Before the first variant is ever assigned onto this sheet, capture the
     * sheet's own palette under a name of its own. Without this there is no
     * way back to it. */
    if (sNdsParticleBasePaletteName[sheet] == 0)
    {
        int base_name = 0;
        int base_palette_format = -1;

        /* glGen/bind/upload below changes live GX/libnds texture state.  A
         * packet already carrying older vertices must reach GX first. */
        ndsRendererFlushWhispyNativePacket();

        if (ndsRendererHardwareFencedGlGenTextures(1, &base_name) == 0)
        {
            return 0u;
        }
        ndsRendererHardwareBindTextureName(NULL, (u32)base_name);
        glColorTableEXT(GL_TEXTURE_2D, 0, NDS_PARTICLE_QUAD_PALETTE_ENTRIES,
                        0, 0,
                        &sNdsRendererParticleAtlasPalette[
                            sheet * NDS_PARTICLE_QUAD_PALETTE_ENTRIES]);
        glGetColorTableParameterEXT(GL_TEXTURE_2D, GL_COLOR_TABLE_WIDTH_EXT,
                                    &palette_width);
        if (palette_width != (int)NDS_PARTICLE_QUAD_PALETTE_ENTRIES)
        {
            ndsRendererHardwareFencedGlDeleteTextures(1, &base_name);
            sNdsRendererHardwareBoundTextureName = 0u;
            return 0u;
        }
        glGetColorTableParameterEXT(
            GL_TEXTURE_2D, GL_COLOR_TABLE_FORMAT_EXT, &base_palette_format);
        if (base_palette_format < 0)
        {
            ndsRendererHardwareFencedGlDeleteTextures(1, &base_name);
            sNdsRendererHardwareBoundTextureName = 0u;
            return 0u;
        }
        sNdsParticleBasePaletteFormat[sheet] = (u32)base_palette_format;
        sNdsParticleBasePaletteActive[sheet] = (s32)glGlob->activePalette;
        sNdsRendererParticlePacketStateDirty = TRUE;
        sNdsParticleBasePaletteName[sheet] = base_name;
    }
    if (variant->valid == 0u)
    {
        ndsRendererFlushWhispyNativePacket();
        if (ndsRendererHardwareFencedGlGenTextures(1, &name) == 0)
        {
            return 0u;
        }
        variant->name = name;
        variant->valid = TRUE;
    }
    else
    {
        /* The palette object is about to be rewritten.  No queued GX command
         * may retain its old PLTT_BASE contents past this point. */
        ndsRendererFlushWhispyNativePacket();
    }
    ndsRendererHardwareBindTextureName(NULL, (u32)variant->name);
    glColorTableEXT(GL_TEXTURE_2D, 0, NDS_PARTICLE_QUAD_PALETTE_ENTRIES, 0, 0,
                    bake);
    glGetColorTableParameterEXT(GL_TEXTURE_2D, GL_COLOR_TABLE_WIDTH_EXT,
                                &palette_width);
    if (palette_width != (int)NDS_PARTICLE_QUAD_PALETTE_ENTRIES)
    {
        ndsRendererHardwareFencedGlDeleteTextures(1, &variant->name);
        variant->name = 0;
        variant->valid = FALSE;
        variant->refused = 1u;
        sNdsRendererHardwareBoundTextureName = 0u;
        return 0u;
    }
    {
        int variant_palette_format = -1;

        glGetColorTableParameterEXT(
            GL_TEXTURE_2D, GL_COLOR_TABLE_FORMAT_EXT,
            &variant_palette_format);
        if (variant_palette_format < 0)
        {
            ndsRendererHardwareFencedGlDeleteTextures(1, &variant->name);
            variant->name = 0;
            variant->valid = FALSE;
            variant->refused = 1u;
            sNdsRendererHardwareBoundTextureName = 0u;
            return 0u;
        }
        variant->palette_format = (u32)variant_palette_format;
        variant->palette_name = (s32)glGlob->activePalette;
        sNdsRendererParticlePacketStateDirty = TRUE;
    }
    /* Where the palette lives, so later misses rewrite it in place. */
    variant->vram = NULL;
    variant->banks = 0u;
    {
        const gl_palette_data *palette = (const gl_palette_data *)
            DynamicArrayGet(&glGlob->palettePtrs,
                            (unsigned int)glGlob->activePalette);

        if ((palette != NULL) &&
            (((uintptr_t)palette->vramAddr & 3u) == 0u))
        {
            variant->banks = ndsRendererPaletteBanksFor(
                palette->vramAddr,
                NDS_PARTICLE_QUAD_PALETTE_ENTRIES * sizeof(u16));
            if (variant->banks != 0u)
            {
                variant->vram = (u32 *)palette->vramAddr;
            }
        }
    }
    variant->sheet = sheet;
    variant->prim = prim_key;
    variant->env = env_key;
    variant->epoch = sNdsParticleEnvVariantEpoch;
    sNdsParticleMaterialGen++;
    gNdsParticleEnvVariantBakeCount++;
    return (u32)variant->name;
}

/* The VBlank that swaps the built frame in (ndsRendererCommitDeferredPalettes,
 * from the platform's scheduled wait after its first retrace, the one after
 * the flush): the frame that referenced the old contents has stopped
 * rendering and the new one starts at line 214, so the queued palettes land
 * now, with banks E-G mapped to the CPU only while the 3D engine reads none
 * of them. Every epoch ends here, which also drops the lookup memos -- a memo
 * hit does not restamp its entry. */
static void ndsRendererParticleEnvVariantCommit(void)
{
    u32 pending = sNdsParticleEnvVariantPending;
    u32 banks;
    u32 saved;

    sNdsParticleEnvVariantEpoch++;
    sNdsParticleMaterialGen++;
    if (pending == 0u)
    {
        return;
    }
    banks = sNdsParticleEnvVariantPendingBanks;
    sNdsParticleEnvVariantPending = 0u;
    sNdsParticleEnvVariantPendingBanks = 0u;
    if (sNdsParticleEnvVariantPendingReset !=
        gNdsRendererSceneTextureVramResetCount)
    {
        return;
    }
    saved = ndsRendererPaletteBanksToLcd(banks);
    do
    {
        const u32 slot = (u32)__builtin_ctz(pending);
        const u32 *src = sNdsParticleEnvVariantStage[slot];
        u32 *dst = sNdsParticleEnvVariants[slot].vram;
        u32 i;

        pending &= pending - 1u;
        for (i = 0u; i < (NDS_PARTICLE_QUAD_PALETTE_ENTRIES / 2u); i++)
        {
            dst[i] = src[i];
        }
    } while (pending != 0u);
    ndsRendererPaletteBanksRestore(banks, saved);
    NDS_DIAG(gNdsParticleEnvVariantCommits++);
}

NDS_RENDERER_COLD static void ndsRendererParticleEnvVariantDiscard(void)
{
    u32 slot;

    sNdsParticleMaterialGen++;
    for (slot = 0u; slot < NDS_PARTICLE_ENV_VARIANT_COUNT; slot++)
    {
        if (sNdsParticleEnvVariants[slot].valid != 0u)
        {
            if (sNdsRendererHardwareBoundTextureName ==
                (u32)sNdsParticleEnvVariants[slot].name)
            {
                sNdsRendererHardwareBoundTextureName = 0u;
            }
            ndsRendererHardwareFencedGlDeleteTextures(
                1, &sNdsParticleEnvVariants[slot].name);
            sNdsParticleEnvVariants[slot].name = 0;
            sNdsParticleEnvVariants[slot].valid = FALSE;
            sNdsParticleEnvVariants[slot].palette_format = 0u;
            sNdsParticleEnvVariants[slot].palette_name = 0;
        }
        sNdsParticleEnvVariants[slot].vram = NULL;
        sNdsParticleEnvVariants[slot].banks = 0u;
        sNdsParticleEnvVariants[slot].epoch = 0u;
        sNdsParticleEnvVariants[slot].refused = 0u;
    }
    /* A queued rewrite targets a palette that no longer exists. */
    sNdsParticleEnvVariantPending = 0u;
    sNdsParticleEnvVariantPendingBanks = 0u;
    for (slot = 0u; slot < NDS_PARTICLE_QUAD_ATLAS_SHEETS; slot++)
    {
        if (sNdsParticleBasePaletteName[slot] != 0)
        {
            if (sNdsRendererHardwareBoundTextureName ==
                (u32)sNdsParticleBasePaletteName[slot])
            {
                sNdsRendererHardwareBoundTextureName = 0u;
            }
            ndsRendererHardwareFencedGlDeleteTextures(
                1, &sNdsParticleBasePaletteName[slot]);
            sNdsParticleBasePaletteName[slot] = 0;
            sNdsParticleBasePaletteFormat[slot] = 0u;
            sNdsParticleBasePaletteActive[slot] = 0;
        }
    }
    sNdsRendererParticlePacketStateDirty = TRUE;
}


/* One sheet from the open asset into its own GL name, with its palette
 * and the binding words the packet path switches sheets with. FALSE
 * leaves the name in sNdsRendererParticleAtlasName for the release. */
static NDS_RENDERER_COLD s32 ndsRendererParticleAtlasUploadSheet(
    FILE *file, u32 sheet, int size_x, int size_y)
{
    int name = 0;
    int palette_format = -1;

    if ((ndsRendererHardwareFencedTextureFseek(
             file, (long)(sheet * NDS_PARTICLE_QUAD_SHEET_BYTES),
             SEEK_SET) != 0) ||
        (ndsRendererHardwareFencedTextureFread(
             sNdsRendererHardwareTextureScratch, 1,
             NDS_PARTICLE_QUAD_SHEET_BYTES, file) !=
         NDS_PARTICLE_QUAD_SHEET_BYTES))
    {
        return FALSE;
    }

    if (ndsRendererHardwareFencedGlGenTextures(1, &name) == 0)
    {
        return FALSE;
    }
    sNdsRendererParticleAtlasName[sheet] = name;
    ndsRendererHardwareBindTextureName(NULL, (u32)name);
    /* A3I5, one byte per texel: 32 palette entries and 8 alpha levels.
     *
     * Not GL_RGBA, for the reason that has not changed -- RGB555+A1 gives
     * particles a SINGLE alpha bit, so every soft-edged sprite draws as a
     * hard blob, and it costs two bytes a texel besides.
     *
     * A5I3 (8 entries, 32 alpha levels) held this slot until 2026-08-03 and
     * its reasoning was sound while the sheet carried SHAPE and the batch
     * supplied COLOUR through glColor per quad, exactly as the RDP's prim
     * colour did. Fox's reflector is the asset that does not fit that
     * division: it is two flat tones with no shape at all, so eight shared
     * entries could not hold its specific blues alongside every other
     * effect's greys, and the owner filed it as the wrong asset. Colour has
     * no other source here; alpha resolution does -- 8 levels is four times
     * what the RGB555+A1 failure had, spread over cells that are now at
     * their source size instead of halved. */
    if (ndsRendererHardwareFencedGlTexImage2D(
            GL_TEXTURE_2D, 0, GL_RGB32_A3, size_x, size_y, 0,
            TEXGEN_TEXCOORD, sNdsRendererHardwareTextureScratch) == 0)
    {
        return FALSE;
    }
    /* PER SHEET, INSIDE THE LOOP, AND THAT PLACEMENT IS THE WHOLE POINT.
     * glColorTableEXT attaches the palette to the texture name that is
     * BOUND RIGHT NOW, so one call outside this loop would leave three of
     * the four sheets indexing whatever palette memory happened to follow.
     * Four calls means four 64-byte blocks in VRAM F/G -- 256 bytes, and
     * not the allocator that was ever refusing.
     *
     * EACH SHEET NOW GETS ITS OWN COLOURS, which is a change to the ASSET
     * and not to this loop's cost: until 2026-08-14 all four calls handed
     * over the same 32 entries, so the atlas paid for 128 palette slots and
     * used 32. Making the generator emit one table per sheet is what let
     * the packer seat shield-break texture 4 and side-KO textures 11 and 14
     * without the extra texels pulling the shared k-means centres off every
     * texture already on the sheet -- measured, that sharing cost 29 of 31
     * admitted textures accuracy, and per-sheet tables instead make 31 of
     * 36 strictly better than the old build with none worse.
     *
     * Read separately above, so no byte-offset arithmetic into a u16 buffer
     * -- which is what used to hand the hardware texels as a palette when
     * the offset was applied through the wrong pointer type. */
    glColorTableEXT(
        GL_TEXTURE_2D, 0, NDS_PARTICLE_QUAD_PALETTE_ENTRIES, 0, 0,
        &sNdsRendererParticleAtlasPalette[
            sheet * NDS_PARTICLE_QUAD_PALETTE_ENTRIES]);
    glGetColorTableParameterEXT(
        GL_TEXTURE_2D, GL_COLOR_TABLE_FORMAT_EXT, &palette_format);
    if (palette_format < 0)
    {
        return FALSE;
    }
    sNdsRendererParticleAtlasBinding[sheet].texture_name = (u32)name;
    sNdsRendererParticleAtlasBinding[sheet].texture_format =
        (u32)glGetTexParameter();
    sNdsRendererParticleAtlasBinding[sheet].palette_format =
        (u32)palette_format;
    sNdsRendererParticleAtlasBinding[sheet].palette_name =
        (s32)glGlob->activePalette;
    sNdsRendererParticleAtlasBinding[sheet].valid = TRUE;
    return TRUE;
}

NDS_RENDERER_COLD s32 ndsRendererHardwarePrepareParticleAtlas(void)
{
    FILE *file = NULL;
    int size_x;
    int size_y;
    u32 sheet;
    u32 row_index;

    if (sNdsRendererParticleAtlasPrepared != 0u)
    {
#if NDS_P2_STAGE_HYRULE
        if (gNdsParticleBankHyruleID != 0xffu)
        {
            return ndsRendererHardwarePrepareHyruleTextures();
        }
#endif
        return TRUE;
    }
    sNdsParticleMaterialGen++;
    gNdsRendererParticleAtlasPrepareCount++;
    /* ONE SHEET AT A TIME against the scratch. The sheet size is fixed at the
     * 8,192-byte allocation that has never been refused and the SHEET COUNT is
     * what grew, so this reads and uploads NDS_PARTICLE_QUAD_ATLAS_SHEETS times
     * rather than asking the allocator for one block four times the size --
     * which is the request that broke stage texture resolves at 16,384 and at
     * 32,768. The palette is shared and gets its own small read below. */
    NDS_RENDERER_CAPACITY_HIGH_WATER(
        gNdsRendererTextureScratchParticleAtlasBytesHighWater,
        NDS_PARTICLE_QUAD_SHEET_BYTES);
    if ((NDS_PARTICLE_QUAD_SHEET_BYTES >
          sizeof(sNdsRendererHardwareTextureScratch)) ||
        (ndsRendererHardwareTextureSizeEnum(
             NDS_PARTICLE_QUAD_ATLAS_WIDTH, &size_x) == FALSE) ||
        (ndsRendererHardwareTextureSizeEnum(
             NDS_PARTICLE_QUAD_ATLAS_HEIGHT, &size_y) == FALSE))
    {
        goto fail;
    }
    file = ndsRendererHardwareFencedTextureFopen(
        NDS_PARTICLE_QUAD_ASSET_PATH, "rb");
    if (file == NULL)
    {
        goto fail;
    }
    if ((ndsRendererHardwareFencedTextureFseek(
             file, (long)NDS_PARTICLE_QUAD_PALETTE_OFFSET, SEEK_SET) != 0) ||
        (ndsRendererHardwareFencedTextureFread(
             sNdsRendererParticleAtlasPalette, 1,
             sizeof(sNdsRendererParticleAtlasPalette), file) !=
         sizeof(sNdsRendererParticleAtlasPalette)))
    {
        goto fail;
    }

    for (sheet = 0u; sheet < NDS_PARTICLE_QUAD_BASE_SHEETS; sheet++)
    {
        if (ndsRendererParticleAtlasUploadSheet(file, sheet, size_x,
                                                size_y) == FALSE)
        {
            goto fail;
        }
    }
#if NDS_VRAM_CENSUS_LIVE
    if (gNdsLabParticleReserveAfterCountdown == 0u)
    {
        ndsLabParticleReserveTake(size_x, size_y, FALSE);
    }
#endif

    if (ndsRendererHardwareFencedTextureFclose(file) != 0)
    {
        file = NULL;
        goto fail;
    }
    file = NULL;
    if (ndsRendererHardwarePrepareWhispyNativeTextures() == FALSE)
    {
        goto fail;
    }
    if (ndsRendererHardwarePrepareFoxBlasterGlowTexture() == FALSE)
    {
        goto fail;
    }
#if NDS_P2_STAGE_HYRULE
    if ((gNdsParticleBankHyruleID != 0xffu) &&
        (ndsRendererHardwarePrepareHyruleTextures() == FALSE))
    {
        goto fail;
    }
#endif
    {
        u32 heal_frame_mask = 0u;

        sNdsRendererHealSparkleCellValidMask = 0u;
        for (row_index = 0u; row_index < NDS_PARTICLE_QUAD_FRAME_COUNT;
             row_index++)
        {
            const NDSParticleQuadFrame *row = &gNdsParticleQuadFrames[row_index];

            if ((row->texture_id == 7u) && (row->frame < 2u) &&
                (row->width == 8u) && (row->height == 8u))
            {
                sNdsRendererHealSparkleCellKey[row->frame] =
                    (u32)row->x | ((u32)row->y << 8) |
                    ((u32)row->width << 16) | ((u32)row->height << 24);
                sNdsRendererHealSparkleSheet[row->frame] = row->sheet;
                heal_frame_mask |= 1u << row->frame;
            }
        }
#if NDS_PARTICLE_HEAL_SPARKLE_COVERAGE_REDUCTION
        if (heal_frame_mask == 0x3u)
        {
            sNdsRendererHealSparkleCellValidMask = heal_frame_mask;
        }
#endif
    }
    sNdsRendererParticleAtlasPrepared = TRUE;
    gNdsParticleQuadResidentSheets = NDS_PARTICLE_QUAD_BASE_SHEETS;
    gNdsRendererParticleAtlasBytes = NDS_PARTICLE_QUAD_ASSET_BYTES;
    sNdsRendererHardwareActiveTextureEntry = NULL;
    return TRUE;

fail:
    if (file != NULL)
    {
        (void)ndsRendererHardwareFencedTextureFclose(file);
    }
    ndsRendererParticleAtlasReleaseSheets();
    ndsRendererHardwareDiscardParticleAtlas();
    gNdsRendererParticleAtlasFailCount++;
    return FALSE;
}

/* P8 LOOP V: THE ANIMATION SHEETS, ONE A FRAME AFTER THE COUNTDOWN.
 *
 * The sheets past NDS_PARTICLE_QUAD_BASE_SHEETS hold the frames the base
 * cells lack (generate_nds_particle_banks.py build_animation_sheets). Texture
 * VRAM cannot hold them at battle prepare: the 2026-10-22 census refused stage
 * textures on every measured stage but Dream Land with all of them resident
 * beside the countdown's atlases. Once the countdown has released those
 * 57,344 bytes (ndsIFCommonCountdownProcUpdate), the frame-end service
 * (ndsRendererHardwareConsumeSubmittedFrame) uploads one sheet a frame -- one
 * 8 KiB read and the same allocation the base sheets make -- and each sheet's
 * rows join the lookup as it lands (gNdsParticleQuadResidentSheets; until
 * then ndsParticleQuadFrameFor skips them and the nearest earlier resident
 * frame draws, as before the animation sheets existed). The countdown's
 * atlases sit in banks A+B, which the battle has locked by then, so each
 * upload opens their holes (ndsFtrLeanAdmitOpenHoles) -- without that only
 * what bank D had free took sheets (4 of 17 on Peach's Castle). A refused
 * allocation ends the service with the sheets that fit
 * (gNdsParticleAnimSheetFailCount): nothing is evicted to make room. */
volatile u32 gNdsParticleQuadResidentSheets;
volatile u32 gNdsParticleAnimSheetFailCount;
static u32 sNdsParticleAnimNextSheet;
static FILE *sNdsParticleAnimFile;

NDS_RENDERER_COLD static void ndsRendererParticleAnimationStop(void)
{
    if (sNdsParticleAnimFile != NULL)
    {
        (void)ndsRendererHardwareFencedTextureFclose(sNdsParticleAnimFile);
        sNdsParticleAnimFile = NULL;
    }
    sNdsParticleAnimNextSheet = 0u;
}

void ndsRendererParticleAnimationStart(void)
{
    if ((sNdsRendererParticleAtlasPrepared != 0u) &&
        (sNdsParticleAnimNextSheet == 0u) &&
        (gNdsParticleQuadResidentSheets == NDS_PARTICLE_QUAD_BASE_SHEETS) &&
        (NDS_PARTICLE_QUAD_ATLAS_SHEETS > NDS_PARTICLE_QUAD_BASE_SHEETS))
    {
        sNdsParticleAnimNextSheet = NDS_PARTICLE_QUAD_BASE_SHEETS;
    }
}

static NDS_RENDERER_COLD void ndsRendererParticleAnimationPumpSheet(void)
{
    int size_x;
    int size_y;
    const u32 sheet = sNdsParticleAnimNextSheet;
    s32 uploaded = FALSE;

    if ((sNdsRendererParticleAtlasPrepared == 0u) ||
        (sheet >= NDS_PARTICLE_QUAD_ATLAS_SHEETS) ||
        (ndsRendererHardwareTextureSizeEnum(
             NDS_PARTICLE_QUAD_ATLAS_WIDTH, &size_x) == FALSE) ||
        (ndsRendererHardwareTextureSizeEnum(
             NDS_PARTICLE_QUAD_ATLAS_HEIGHT, &size_y) == FALSE))
    {
        ndsRendererParticleAnimationStop();
        return;
    }
    if (sNdsParticleAnimFile == NULL)
    {
        sNdsParticleAnimFile = ndsRendererHardwareFencedTextureFopen(
            NDS_PARTICLE_QUAD_ASSET_PATH, "rb");
    }
    if (sNdsParticleAnimFile != NULL)
    {
        /* A+B are locked for the rest of the battle once its setup uploads
         * are done (ndsFtrLeanAdmitLockRegions), and the countdown's atlases
         * were freed THERE: open them for this upload, as the cache's last
         * resort does. First fit takes those holes before bank D, which the
         * cache keeps for its own churn. */
        const s32 holes = ndsFtrLeanAdmitOpenHoles();

        uploaded = ndsRendererParticleAtlasUploadSheet(
            sNdsParticleAnimFile, sheet, size_x, size_y);
        if (holes != FALSE)
        {
            ndsFtrLeanAdmitCloseHoles();
        }
    }
    if (uploaded == FALSE)
    {
        /* The refused name goes now, not with the atlas: nothing reads a
         * sheet the lookup never admitted. */
        if (sNdsRendererParticleAtlasName[sheet] != 0)
        {
            if (sNdsRendererHardwareBoundTextureName ==
                (u32)sNdsRendererParticleAtlasName[sheet])
            {
                sNdsRendererHardwareBoundTextureName = 0u;
            }
            ndsRendererHardwareFencedGlDeleteTextures(
                1, &sNdsRendererParticleAtlasName[sheet]);
            sNdsRendererParticleAtlasName[sheet] = 0;
        }
        memset(&sNdsRendererParticleAtlasBinding[sheet], 0,
               sizeof(sNdsRendererParticleAtlasBinding[sheet]));
        gNdsParticleAnimSheetFailCount++;
        ndsRendererParticleAnimationStop();
        return;
    }
    gNdsParticleQuadResidentSheets = sheet + 1u;
    sNdsParticleMaterialGen++;
    sNdsRendererParticlePacketStateDirty = TRUE;
    if ((sheet + 1u) < NDS_PARTICLE_QUAD_ATLAS_SHEETS)
    {
        sNdsParticleAnimNextSheet = sheet + 1u;
    }
    else
    {
        ndsRendererParticleAnimationStop();
    }
}

/* The frame end's whole cost when no animation sheet is pending. */
static inline void ndsRendererParticleAnimationPump(void)
{
    if (sNdsParticleAnimNextSheet != 0u)
    {
        ndsRendererParticleAnimationPumpSheet();
    }
}

/* 0 is "no atlas", so a sheet index the table never emits fails closed rather
 * than binding sheet 0's texels under another sheet's coordinates. */
u32 ndsRendererHardwareParticleAtlasNameForSheet(u32 sheet)
{
    if ((sNdsRendererParticleAtlasPrepared == 0u) ||
        (sheet >= NDS_PARTICLE_QUAD_ATLAS_SHEETS))
    {
        return 0u;
    }
    return (u32)sNdsRendererParticleAtlasName[sheet];
}

u32 ndsRendererHardwareParticleAtlasName(void)
{
    return ndsRendererHardwareParticleAtlasNameForSheet(0u);
}

u32 ndsRendererHardwareFoxBlasterGlowName(void)
{
    if (sNdsRendererParticleAtlasPrepared != 0u)
    {
        return (u32)sNdsRendererFoxBlasterGlowName;
    }
    return 0u;
}

u32 ndsRendererHardwareWhispyNativeName(u32 texture_id)
{
    if ((sNdsRendererParticleAtlasPrepared != 0u) &&
        (texture_id < NDS_WHISPY_NATIVE_TEXTURE_COUNT))
    {
        return (u32)sNdsRendererWhispyNativeName[texture_id];
    }
    return 0u;
}

NDS_RENDERER_COLD void ndsRendererHardwareDiscardParticleAtlas(void)
{
    u32 sheet;

    ndsRendererParticleAnimationStop();
    gNdsParticleQuadResidentSheets = 0u;
    ndsRendererParticleAtlasReleaseSheets();
#if NDS_VRAM_CENSUS_LIVE
    ndsLabParticleReserveRelease();
#endif
    for (sheet = 0u; sheet < NDS_WHISPY_NATIVE_TEXTURE_COUNT; sheet++)
    {
        if (sNdsRendererWhispyNativeName[sheet] != 0)
        {
            if (sNdsRendererHardwareBoundTextureName ==
                (u32)sNdsRendererWhispyNativeName[sheet])
            {
                sNdsRendererHardwareBoundTextureName = 0u;
            }
            ndsRendererHardwareFencedGlDeleteTextures(
                1, &sNdsRendererWhispyNativeName[sheet]);
        }
        sNdsRendererWhispyNativeName[sheet] = 0;
        sNdsRendererWhispyNativeBinding[sheet].texture_name = 0u;
        sNdsRendererWhispyNativeBinding[sheet].texture_format = 0u;
        sNdsRendererWhispyNativeBinding[sheet].palette_format = 0u;
        sNdsRendererWhispyNativeBinding[sheet].palette_name = 0;
        sNdsRendererWhispyNativeBinding[sheet].valid = FALSE;
    }
    if (sNdsRendererFoxBlasterGlowName != 0)
    {
        if (sNdsRendererHardwareBoundTextureName ==
            (u32)sNdsRendererFoxBlasterGlowName)
        {
            sNdsRendererHardwareBoundTextureName = 0u;
        }
        ndsRendererHardwareFencedGlDeleteTextures(
            1, &sNdsRendererFoxBlasterGlowName);
    }
    sNdsRendererFoxBlasterGlowName = 0;
    memset(&sNdsRendererWhispyNativeBinding[
               NDS_WHISPY_NATIVE_TEXTURE_COUNT], 0,
           sizeof(sNdsRendererWhispyNativeBinding[0]));
#if NDS_P2_STAGE_HYRULE
    /* The atlas tail owns these names, so every atlas discard drops them for
     * the same reason whispy and fox glow release above: glResetTextures (and
     * the prepare failure path) must not find them still held. Idempotent --
     * the release the failure path already ran is a second no-op here. */
    ndsRendererHardwareReleaseHyruleTextures();
#endif
    /* Palette-only ENV variant names hold no image, but their GL names must
     * not outlive the sheets they were baked against. */
    ndsRendererParticleEnvVariantDiscard();
    sNdsRendererParticleAtlasPrepared = FALSE;
    gNdsRendererParticleAtlasBytes = 0u;
    sNdsRendererHealSparkleCellValidMask = 0u;
    gNdsRendererWhispyNativeBytes = 0u;
    gNdsRendererFoxBlasterGlowBytes = 0u;
}

/* BUGS.md "VFX get x flattened around stage edges ... why is there a limit
 * anyways???" and "Star KO twinkle not playing in correct spot".
 *
 * THE LIMIT IS v16, AND IT IS ONE BUG WEARING TWO FACES. At the stock x16
 * factor the reach is 32767/16 = 2047.9 world units. Every vertex past that
 * SATURATES on the rail, so a quad straddling it has its far corners pinned to
 * the same coordinate and collapses along that axis -- the owner's "x flattened
 * around stage edges", measured at 1,118 of 5,590 quads a match on Whispy's
 * wind alone, because the live camera reaches 3,148. The same rail is why the
 * Star KO twinkle draws in the wrong place: ftcommondead.c recedes the dying
 * fighter to z = -14,999 and the sparkle spawns AT it, so the fighter (drawn
 * through the ordinary matrix path, which has 20.12 of range) recedes correctly
 * while its twinkle stops dead at -2,047.9 and hangs near the camera. The
 * fighter's own draw never had this limit; only the particle pass did.
 *
 * The range has to come from SCALE, since v16 is 16 bits and no encoding buys
 * it back. A FIXED coarser factor was the previous shape of this and it is the
 * wrong trade: it charges every ordinary hit spark a permanent loss of sub-unit
 * resolution to pay for the one effect a match that leaves the stage.
 *
 * So the factor is chosen per BATCH and escalates only when a quad actually
 * needs it. Each quad's furthest corner picks the smallest shift that keeps it
 * off the rail; if that is coarser than what the batch is running, the pass
 * swaps its compensating modelview scale and continues. Vertices already queued
 * were transformed when they were submitted and are unaffected. An ordinary
 * frame never escalates and keeps the full x16 precision it has always had; a
 * Star KO frame escalates once and spends 0.5 world units of precision on
 * particles that are 15,000 units away. Monotonic within a frame, reset with
 * the batch, so the choice is deterministic rather than order-dependent noise.
 *
 * NDS_RENDERER_PARTICLE_MAX_SCALE_SHIFT 4 reaches +/-32,767 world units, past
 * every blast zone and past the DeadUpStar recession. */
#define NDS_RENDERER_PARTICLE_MAX_SCALE_SHIFT 4u

/* World coordinate -> v16 at the batch's current scale. The scene's modelview
 * carries a NDS_RENDERER_HW_WORLD_UNIT_SHIFT (=8) scale, so at shift 0 one
 * world unit is 2^(12-8) = 16 in vertex space -- the same relation
 * ndsRendererHardwareCoordToV16 applies to integer source vertices, restated
 * for the f32 the particle interpreter holds. Getting this wrong does not
 * fail; it draws the effects at 1/256 or 256x scale, which is why it is
 * spelled out rather than folded into a literal. */
#define NDS_RENDERER_PARTICLE_UNIT_SHIFT NDS_RENDERER_HW_WORLD_UNIT_SHIFT
#define NDS_RENDERER_PARTICLE_COORD_SHIFT 8u
#define NDS_RENDERER_PARTICLE_BASIS_SHIFT 13u
/* Q8 conversion truncates center, basis*size and the final leg independently.
 * Bias the range-only extent by 1/4 world unit so quantization can never make a
 * near-rail quad select a finer batch scale solely because the fixed estimate
 * rounded down. The submitted vertices are not biased. */
#define NDS_RENDERER_PARTICLE_EXTENT_GUARD_Q8 64u

/* Counted, not assumed: Clamp must read 0 on a fixed build. It is the direct
 * measurement of the reported symptom, so a non-zero value means a quad was
 * still drawn on the rail. */
volatile u32 gNdsParticleWorldClampCount;
volatile u32 gNdsParticleScaleEscalations;
volatile u32 gNdsParticleScaleShiftMax;

static u32 sNdsRendererParticleScaleShift;

/* A world magnitude past this cannot be on screen under any camera this game
 * builds, and converting it would overflow the s32 below -- which is undefined
 * rather than merely wrong. Saturating here keeps the conversion total. */
#define NDS_RENDERER_PARTICLE_WORLD_LIMIT 131072.0F

/* ARM946E-S has no FPU. Decode a finite IEEE-754 source value directly into the
 * requested binary fixed domain so render-only conversion does not call the
 * libgcc float helpers. This preserves the old C-cast truncation toward zero. */
static sb32 ndsRendererParticleFloatToFixed(f32 value, u32 fraction_bits,
                                            s32 *fixed)
{
    union
    {
        f32 f;
        u32 u;
    } bits;
    u32 exponent;
    u32 mantissa;
    u32 magnitude;
    u32 limit;
    u32 sign;
    s32 shift;

    bits.f = value;
    sign = bits.u >> 31;
    exponent = (bits.u >> 23) & 0xFFu;
    mantissa = bits.u & 0x7FFFFFu;
    if ((fixed == NULL) || (exponent == 0xFFu))
    {
        return FALSE;
    }
    if (exponent == 0u)
    {
        if (mantissa == 0u)
        {
            *fixed = 0;
            return TRUE;
        }
        shift = -126 - 23 + (s32)fraction_bits;
    }
    else
    {
        mantissa |= 0x800000u;
        shift = (s32)exponent - 127 - 23 + (s32)fraction_bits;
    }
    limit = (sign != 0u) ? 0x80000000u : 0x7FFFFFFFu;
    if (shift >= 0)
    {
        if ((shift >= 32) || (mantissa > (limit >> (u32)shift)))
        {
            return FALSE;
        }
        magnitude = mantissa << (u32)shift;
    }
    else if (shift <= -32)
    {
        magnitude = 0u;
    }
    else
    {
        magnitude = mantissa >> (u32)-shift;
    }
    if (sign != 0u)
    {
        *fixed = (magnitude == 0x80000000u) ? INT_MIN : -(s32)magnitude;
    }
    else
    {
        *fixed = (s32)magnitude;
    }
    return TRUE;
}

static u32 ndsRendererParticleAbsQ8(s32 value)
{
    return (value < 0) ? (u32)(-(s64)value) : (u32)value;
}

static s32 ndsRendererParticleTruncShiftS32(s32 value, u32 shift)
{
    if (value < 0)
    {
        return -(s32)((u32)(-(s64)value) >> shift);
    }
    return (s32)((u32)value >> shift);
}

/* The submitted GX vertex ultimately carries world*16. Q8 center/size retains
 * sixteen times more fractional precision than that final representation, and
 * Q13 basis keeps sub-pixel camera-axis error bounded. One signed wide multiply
 * plus a power-of-two shift replaces the old soft-float multiply/cast chain;
 * saturation keeps pathological transformed inputs total without a divide. */
static s32 ndsRendererParticleScaleBasisQ8(s32 basis_q13, s32 size_q8)
{
    s64 product = (s64)basis_q13 * (s64)size_q8;
    u64 magnitude;

    if (product < 0)
    {
        magnitude = ((u64)-product) >> NDS_RENDERER_PARTICLE_BASIS_SHIFT;
        if (magnitude >= 0x80000000u)
        {
            return INT_MIN;
        }
        return -(s32)magnitude;
    }
    magnitude = (u64)product >> NDS_RENDERER_PARTICLE_BASIS_SHIFT;
    if (magnitude > 0x7fffffffu)
    {
        return INT_MAX;
    }
    return (s32)magnitude;
}

/* Q8 world coordinate -> v16 under the same world*16 convention as the old
 * float helper. Keep the final rail counter/behavior exactly at the vertex
 * boundary. */
static inline __attribute__((always_inline)) v16
ndsRendererParticleQ8ToV16(s32 value_q8, u32 shift)
{
    s32 scaled = ndsRendererParticleTruncShiftS32(
        value_q8, (NDS_RENDERER_PARTICLE_COORD_SHIFT - 4u) + shift);

    if (scaled > 32767) { NDS_DIAG(gNdsParticleWorldClampCount++); return (v16)32767; }
    if (scaled < -32768) { NDS_DIAG(gNdsParticleWorldClampCount++); return (v16)-32768; }
    return (v16)scaled;
}

static u32 ndsRendererParticleScaleShiftForQ8(u32 extent_q8)
{
    u32 shift = 0u;

    while ((shift < NDS_RENDERER_PARTICLE_MAX_SCALE_SHIFT) &&
           ((extent_q8 >>
             ((NDS_RENDERER_PARTICLE_COORD_SHIFT - 4u) + shift)) > 32767u))
    {
        shift++;
    }
    return shift;
}

static s32 ndsRendererParticleWorldFixed(f32 value)
{
    if (value > NDS_RENDERER_PARTICLE_WORLD_LIMIT)
    {
        value = NDS_RENDERER_PARTICLE_WORLD_LIMIT;
    }
    else if (value < -NDS_RENDERER_PARTICLE_WORLD_LIMIT)
    {
        value = -NDS_RENDERER_PARTICLE_WORLD_LIMIT;
    }
    return (s32)(value * (f32)(1 << (12u -
                                     NDS_RENDERER_PARTICLE_UNIT_SHIFT)));
}

static v16 ndsRendererParticleWorldToV16(f32 value, u32 shift)
{
    s32 scaled = ndsRendererParticleWorldFixed(value) / (s32)(1 << shift);

    if (scaled > 32767) { gNdsParticleWorldClampCount++; return (v16)32767; }
    if (scaled < -32768) { gNdsParticleWorldClampCount++; return (v16)-32768; }
    return (v16)scaled;
}

static u32 sNdsRendererParticleQuadOpen;
/* The POLY_ALPHA currently latched into the open group, so a quad only pays for
 * a new group when its alpha actually differs. Reset with the batch. */
static u32 sNdsRendererParticleQuadAlpha;
/* How many times a frame's particle pass had to start a new primitive group
 * because the alpha bucket moved. Engagement proof for the per-particle fade:
 * 0 with particles drawing means every live particle shared one alpha, which is
 * what the bug looked like. */
volatile u32 gNdsParticleQuadAlphaBreaks;
/* Companion to the above for the four-sheet atlas: how many times the pass had
 * to rebind because the next quad's cell was packed on a different sheet. */
volatile u32 gNdsParticleQuadSheetBreaks;
/* The sheet currently bound into the open group, so a quad only pays for a
 * rebind when its sheet actually differs. Reset with the batch. */
static u32 sNdsRendererParticleQuadTexture;
/* The palette-only variant name currently assigned into the open group, or 0
 * for the sheet's own palette. A change re-issues the group in the common
 * block below (both batch routes); legacy quads never leave 0 and therefore
 * issue no new GL calls. Reset with the batch. */
static u32 sNdsRendererParticleQuadPalette;

/* gNdsParticleMatrixModeSeen records the mode the batch INHERITED, latched
 * before the camera load replaces it -- so it stays 0x12 by design and is the
 * evidence of what the bug was, not a pass/fail. The regression guard is
 * gNdsParticleCameraLoads == gNdsParticleBatchOpens: every batch must load its
 * own camera. gNdsParticleCamT* samples the loaded translation so a silently
 * empty matrix cannot masquerade as a working one. */
volatile u32 gNdsParticleMatrixModeSeen;
volatile u32 gNdsParticleMatrixLoadedSeen;
volatile u32 gNdsParticleBatchOpens;
volatile u32 gNdsParticleCameraLoads;
volatile s32 gNdsParticleCamTx;
volatile s32 gNdsParticleCamTy;
volatile s32 gNdsParticleCamTz;

/* THE PARTICLE PASS HAD NO MATRIX OF ITS OWN.
 *
 * It never loaded one, so every quad rendered under whatever the previously
 * drawn object happened to leave in the hardware. Measured over a match:
 * gNdsParticleMatrixModeSeen = 0x12, i.e. bits 1 and 4 -- PROJECTED_IDENTITY
 * and STAGE_HW_COMPOSE -- across 599 batch opens, varying frame to frame.
 *
 * That is the whole family of "effects are in the wrong place" reports at once.
 * Under PROJECTED_IDENTITY the modelview is identity, so world coordinates are
 * read as view coordinates and the effect renders at the eye: the owner's "the
 * KO effect plays too close to the camera instead of at the fighters' z depth".
 * Under STAGE_HW_COMPOSE it inherits a stage segment's local matrix and lands
 * wherever that segment is: "stray VFX across the stage", and Whispy's wind
 * appearing far from the tree even though its emitter measures source-exact.
 *
 * The positions were never wrong -- the space they were drawn in was. So the
 * game hands the pass its own camera and the batch loads it, exactly as any
 * other root does. Set from lbParticleDrawTextures, which has the game headers
 * this file deliberately does not. */
/* Defined below with the rest of the matrix plumbing; the particle batch is
 * the one caller that sits above it in the file. */
static void ndsRendererCopyMtx20p12ToM4x4(
    const NDSRendererMatrix20p12 *src, m4x4 *dst);
/* Declared here for ndsRendererSubmitFoxGun, which sits above the definition.
 * It is the ONLY correct way to hand this renderer a CPU-composed MVP: it
 * forces GL_PROJECTION to identity and divides the composed homogeneous row by
 * NDS_RENDERER_HW_WORLD_UNIT_SHIFT, which is the other half of the x16 vertex
 * encoding. A submit that loads the MVP itself gets neither. */
static void ndsRendererLoadHardwareRawComposedMatrix(
    const NDSRendererMatrix20p12 *composed, u32 generation);

static NDSRendererMatrix20p12 sNdsRendererParticleProjection;
static NDSRendererMatrix20p12 sNdsRendererParticleModelview;
static u32 sNdsRendererParticleCameraValid;
static u32 sNdsRendererParticleViewSpace;
/* Owner BUGS.md (r71): attack VFX lost against Master Hand as Kirby, and
 * VFX that sometimes fail to draw on the Fox and Master Hand stages. The
 * battle's particle displays are efDisplayZPerspCLDProcDisplay on DL links
 * 18 and 15 (G_RM_CLD_SURF) and efDisplayZPerspXLUProcDisplay on 25
 * (G_RM_XLU_SURF), efdisplay.c:37-62: neither compares Z, so a hit spark
 * whose centre lies inside Master Hand's palm, or behind a stage part, still
 * paints over it. Depth-tested here, the same spark was hidden. The DS
 * cannot turn its depth test off per polygon, so such a pass keeps its
 * camera's X/Y/W and replaces clip Z with the renderer's next foreground
 * painter depth, as the Fox blaster's no-Z list does. Only
 * efDisplayZPerspAAXLUProcDisplay's link 10 (G_RM_AA_ZB_XLU_SURF) keeps the
 * depth test. */
static u32 sNdsRendererParticleNoDepth;
volatile u32 gNdsParticleNoDepthLoads;
static s32 ndsRendererHardwareNextProjectedDepth(void);
static void ndsRendererHardwareEnterProjectedForeground(void);

void ndsRendererSetParticleNoDepth(u32 no_depth)
{
    sNdsRendererParticleNoDepth = (no_depth != 0u) ? TRUE : FALSE;
}
volatile u32 gNdsParticleViewPasses;
volatile u32 gNdsParticleViewCenters;
volatile u32 gNdsParticleViewRejects;

#include <nds/nds_particle_view.h>

void NDS_FIGHTER_PACKET_EVICT(NDS_R2_ITCM_PACK2_CODE)
ndsRendererSetParticleCamera(const NDSRendererMatrix20p12 *projection,
                                  const NDSRendererMatrix20p12 *modelview)
{
    if ((projection == NULL) || (modelview == NULL))
    {
        sNdsRendererParticleCameraValid = FALSE;
        return;
    }
    ndsRendererMatrixCopy20p12(&sNdsRendererParticleProjection, projection);
    ndsRendererMatrixCopy20p12(&sNdsRendererParticleModelview, modelview);
    sNdsRendererParticleCameraValid = TRUE;
}

s32 ndsRendererBeginParticleViewPass(
    const NDSRendererMatrix20p12 *projection,
    const NDSRendererMatrix20p12 *modelview)
{
    if ((projection == NULL) || (modelview == NULL) ||
        (modelview->m[0][3] != 0) || (modelview->m[1][3] != 0) ||
        (modelview->m[2][3] != 0) || (modelview->m[3][3] != (1 << 12)) ||
        (sNdsRendererParticleQuadOpen != FALSE))
    {
        gNdsParticleViewRejects++;
        return FALSE;
    }
    ndsRendererSetParticleCamera(projection, modelview);
    sNdsRendererParticleViewSpace = TRUE;
    NDS_DIAG(gNdsParticleViewPasses++);
    return TRUE;
}

static s32 ndsRendererParticleTransformCenter(s32 center[3], u32 fraction_bits)
{
    if (sNdsRendererParticleViewSpace == FALSE) { return TRUE; }
    if (!ndsParticleViewCenter(sNdsRendererParticleModelview.m,
                               center, fraction_bits))
    {
        gNdsParticleViewRejects++;
        return FALSE;
    }
    NDS_DIAG(gNdsParticleViewCenters++);
    return TRUE;
}

static s32 ndsRendererHardwareNextProjectedDepth(void);
static void ndsRendererHardwareBindNoTexture(NDSRendererStats *stats);

/* Fox's source list is a projected no-Z weapon. Preserve the cached camera's
 * X/Y/W transform and replace only clip Z with the renderer's painter depth,
 * exactly like the promoted split-matrix Rebirth Halo path. Loading the raw
 * world camera instead leaves the translucent muzzle glow in front of the
 * beam even though the source renderer submits the beam over it. */
static s32 ndsRendererLoadFoxBlasterNoZCameraMatrices(void)
{
    NDSRendererMatrix20p12 projection = sNdsRendererParticleProjection;
    NDSRendererMatrix20p12 scaled_modelview;
    m4x4 projection_hw;
    m4x4 modelview_hw;
    s16 projected_z;
    u32 row;

    if (sNdsRendererParticleCameraValid == FALSE)
    {
        return FALSE;
    }
    projected_z = (s16)ndsRendererHardwareNextProjectedDepth();
    for (row = 0u; row < 4u; row++)
    {
        projection.m[row][2] = (s32)ndsRendererRoundShiftS64(
            (s64)projection.m[row][3] * projected_z, 12u);
    }
    ndsRendererMatrixCopy20p12(&scaled_modelview,
                               &sNdsRendererParticleModelview);
    for (row = 0u; row < 4u; row++)
    {
        scaled_modelview.m[3][row] = ndsRendererRoundShiftS32Signed(
            scaled_modelview.m[3][row],
            NDS_RENDERER_HW_WORLD_UNIT_SHIFT);
    }
    ndsRendererCopyMtx20p12ToM4x4(&projection, &projection_hw);
    ndsRendererHardwareSetMatrixMode(GL_PROJECTION);
    glLoadMatrix4x4(&projection_hw);
    ndsRendererCopyMtx20p12ToM4x4(&scaled_modelview, &modelview_hw);
    ndsRendererHardwareSetMatrixMode(GL_MODELVIEW);
    glLoadMatrix4x4(&modelview_hw);
    sNdsRendererHardwareMatrixLoaded = FALSE;
    sNdsRendererHardwareMatrixMode = NDS_RENDERER_HW_MATRIX_MODE_NONE;
    gNdsParticleCameraLoads++;
    gNdsParticleCamTx = scaled_modelview.m[3][0];
    gNdsParticleCamTy = scaled_modelview.m[3][1];
    gNdsParticleCamTz = scaled_modelview.m[3][2];
    return TRUE;
}

static NDS_RENDERER_FAST_RUN_CODE void
ndsRendererPrepareWhispyQuadState(u32 texture_name, u32 poly_alpha,
                                  u32 texture_slot, u32 submit_route);
static void ndsRendererFlushWhispyNativePacket(void);
static s32 ndsRendererSubmitParticleQuadPacket(
    u32 texture_name, u32 env_palette_name,
    const s32 center_q8[3],
    const s32 right_leg_q8[3], const s32 up_leg_q8[3],
    u32 color, u32 poly_alpha, u32 mirror_mask,
    u32 atlas_x, u32 atlas_y, u32 atlas_w, u32 atlas_h);

/* One quad, camera-facing, in world space. `right` and `up` are the camera
 * basis the caller derived from the CObj -- computed there because that is
 * where the source's own draw reads the camera, and because it is per-frame
 * work that must not be repeated per particle.
 *
 * The batch is opened lazily on the first quad and closed by
 * ndsRendererEndParticleQuads, so the whole particle pass is ONE glBegin and
 * ONE texture bind however many particles it carries. */
static u32 sNdsParticleBasisValid;
static u32 sNdsParticleBasisBits[2];
static s32 sNdsParticleBasisQ13[2];

_Static_assert(NDS_RENDERER_PARTICLE_CENTER_SHIFT ==
               NDS_RENDERER_PARTICLE_COORD_SHIFT,
               "the fixed-centre entry takes the submitter's own Q8 centre");

s32 ndsRendererSubmitParticleQuad(u32 atlas_name, const Vec3f *pos, f32 size,
                                  u32 color, u8 alpha,
                                  u32 envcolor, u32 particle_flags,
                                  const Vec3f *right, const Vec3f *up,
                                  u32 mirror_mask,
                                  u32 atlas_x, u32 atlas_y,
                                  u32 atlas_w, u32 atlas_h)
{
    s32 center_q8[3];

    if ((pos == NULL) ||
        (ndsRendererParticleFloatToFixed(
             pos->x, NDS_RENDERER_PARTICLE_COORD_SHIFT, &center_q8[0]) == FALSE) ||
        (ndsRendererParticleFloatToFixed(
             pos->y, NDS_RENDERER_PARTICLE_COORD_SHIFT, &center_q8[1]) == FALSE) ||
        (ndsRendererParticleFloatToFixed(
             pos->z, NDS_RENDERER_PARTICLE_COORD_SHIFT, &center_q8[2]) == FALSE))
    {
        return FALSE;
    }
    return ndsRendererSubmitParticleQuadQ8(
        atlas_name, center_q8, size, color, alpha, envcolor, particle_flags,
        right, up, mirror_mask, atlas_x, atlas_y, atlas_w, atlas_h);
}

s32 ndsRendererSubmitParticleQuadQ8(u32 atlas_name, const s32 center[3],
                                    f32 size, u32 color, u8 alpha,
                                    u32 envcolor, u32 particle_flags,
                                    const Vec3f *right, const Vec3f *up,
                                    u32 mirror_mask,
                                    u32 atlas_x, u32 atlas_y,
                                    u32 atlas_w, u32 atlas_h)
{
    NDS_FIGHTER_PACKET_DMA_WAIT();
    s32 center_q8[3];
    s32 right_q13[3];
    s32 up_q13[3];
    s32 size_q8;
    s32 right_leg_q8[3];
    s32 up_leg_q8[3];
    u32 poly_alpha;
    u32 env_palette_name;
    u32 axis;

    if ((atlas_name == 0u) || (center == NULL) || (right == NULL) ||
        (up == NULL))
    {
        return FALSE;
    }
    if (ndsRendererParticleFloatToFixed(
            size, NDS_RENDERER_PARTICLE_COORD_SHIFT, &size_q8) == FALSE)
    {
        return FALSE;
    }
    center_q8[0] = center[0];
    center_q8[1] = center[1];
    center_q8[2] = center[2];
    if (sNdsRendererParticleViewSpace != FALSE)
    {
        /* A burst's quads share their two view-space axis lengths: the last
         * conversion is kept on its source bits. */
        union
        {
            f32 f;
            u32 u;
        } rx = { right->x }, uy = { up->y };

        if ((sNdsParticleBasisValid != 0u) &&
            (rx.u == sNdsParticleBasisBits[0]) &&
            (uy.u == sNdsParticleBasisBits[1]))
        {
            right_q13[0] = sNdsParticleBasisQ13[0];
            up_q13[1] = sNdsParticleBasisQ13[1];
        }
        else if ((ndsRendererParticleFloatToFixed(
                      right->x, NDS_RENDERER_PARTICLE_BASIS_SHIFT,
                      &right_q13[0]) == FALSE) ||
                 (ndsRendererParticleFloatToFixed(
                      up->y, NDS_RENDERER_PARTICLE_BASIS_SHIFT,
                      &up_q13[1]) == FALSE))
        {
            sNdsParticleBasisValid = 0u;
            return FALSE;
        }
        else
        {
            sNdsParticleBasisBits[0] = rx.u;
            sNdsParticleBasisBits[1] = uy.u;
            sNdsParticleBasisQ13[0] = right_q13[0];
            sNdsParticleBasisQ13[1] = up_q13[1];
            sNdsParticleBasisValid = 1u;
        }
        if (ndsRendererParticleTransformCenter(
                center_q8, NDS_RENDERER_PARTICLE_COORD_SHIFT) == FALSE)
        {
            return FALSE;
        }
        right_leg_q8[0] = ndsRendererParticleScaleBasisQ8(right_q13[0], size_q8);
        right_leg_q8[1] = right_leg_q8[2] = 0;
        up_leg_q8[1] = ndsRendererParticleScaleBasisQ8(up_q13[1], size_q8);
        up_leg_q8[0] = up_leg_q8[2] = 0;
    }
    else
    {
        if ((ndsRendererParticleFloatToFixed(
                 right->x, NDS_RENDERER_PARTICLE_BASIS_SHIFT, &right_q13[0]) == FALSE) ||
            (ndsRendererParticleFloatToFixed(
                 right->y, NDS_RENDERER_PARTICLE_BASIS_SHIFT, &right_q13[1]) == FALSE) ||
            (ndsRendererParticleFloatToFixed(
                 right->z, NDS_RENDERER_PARTICLE_BASIS_SHIFT, &right_q13[2]) == FALSE) ||
            (ndsRendererParticleFloatToFixed(
                 up->x, NDS_RENDERER_PARTICLE_BASIS_SHIFT, &up_q13[0]) == FALSE) ||
            (ndsRendererParticleFloatToFixed(
                 up->y, NDS_RENDERER_PARTICLE_BASIS_SHIFT, &up_q13[1]) == FALSE) ||
            (ndsRendererParticleFloatToFixed(
                 up->z, NDS_RENDERER_PARTICLE_BASIS_SHIFT, &up_q13[2]) == FALSE))
        {
            return FALSE;
        }
        for (axis = 0u; axis < 3u; axis++)
        {
            right_leg_q8[axis] =
                ndsRendererParticleScaleBasisQ8(right_q13[axis], size_q8);
            up_leg_q8[axis] =
                ndsRendererParticleScaleBasisQ8(up_q13[axis], size_q8);
        }
    }
#if NDS_PARTICLE_HEAL_SPARKLE_COVERAGE_REDUCTION
    if (sNdsRendererHealSparkleCellValidMask != 0u)
    {
        u32 cell_key = atlas_x | (atlas_y << 8) |
            (atlas_w << 16) | (atlas_h << 24);
        u32 frame;

        for (frame = 0u; frame < 2u; frame++)
        {
            if (((sNdsRendererHealSparkleCellValidMask & (1u << frame)) != 0u) &&
                (cell_key == sNdsRendererHealSparkleCellKey[frame]) &&
                (atlas_name == ndsRendererHardwareParticleAtlasNameForSheet(
                                   sNdsRendererHealSparkleSheet[frame])))
            {
                gNdsRendererHealSparkleCoverageSubmitCount++;
                break;
            }
        }
    }
#endif
    /* BUGS.md "Whispy blow VFX ... emitted objects turn flat at end of
     * lifetime".
     *
     * A source particle fades: lbparticle.c ramps pc->primcolor toward
     * target_primcolor over primcolor_target_length frames, ALPHA INCLUDED, and
     * the RDP blends with it. This path dropped that channel on the floor --
     * the caller packed r/g/b into a BGR555 word and the pass ran the whole
     * batch at POLY_ALPHA(31). So every particle stayed 100% opaque for its
     * entire life and the tail, which should dissolve, instead sat there as a
     * hard flat blob at exactly the moment the owner noticed it.
     *
     * DS vertex colour carries no alpha, so per-particle alpha has to be
     * POLYGON_ATTR's, which latches at glBegin. The batch therefore reopens
     * when the alpha BUCKET changes rather than staying one glBegin for the
     * pass; particles in a burst share a ramp, so that is far fewer than one
     * per quad, and gNdsParticleQuadAlphaBreaks measures it rather than
     * assuming it.
     *
     * 8-bit source alpha to the hardware's 5 bits, and never to 0: POLY_ALPHA(0)
     * is WIREFRAME on this hardware, not invisible. A fully transparent particle
     * is skipped outright, which is also the cheaper answer. */
    if (alpha == 0u)
    {
        return FALSE;
    }
    poly_alpha = ((u32)alpha >> 3) & 31u;
    if (poly_alpha == 0u)
    {
        poly_alpha = 1u;
    }
    /* ENVCOLOR particles bake (PRIM - ENV) * TEXEL + ENV into a palette
     * variant sharing this sheet's image (see the variant cache above) and
     * draw it with a white vertex; everything else keeps vertex `color` and
     * the sheet palette, exactly as before. A 0 return is the same legacy
     * path, so a failed bake or a non-sheet image can never draw worse than
     * today. */
    env_palette_name = 0u;
    if ((particle_flags & NDS_RENDERER_PARTICLE_QUAD_ENVCOLOR) != 0u)
    {
        const u32 prim_key = color & 0x7FFFu;
        const u32 env_key = envcolor & 0xFFFFFF00u;

        if ((sNdsParticleEnvMemoGen == sNdsParticleMaterialGen) &&
            (sNdsParticleEnvMemoAtlas == atlas_name) &&
            (sNdsParticleEnvMemoPrim == prim_key) &&
            (sNdsParticleEnvMemoEnv == env_key))
        {
            env_palette_name = sNdsParticleEnvMemoName;
        }
        else
        {
            env_palette_name = ndsRendererParticleEnvVariant(
                atlas_name, color, envcolor);
            sNdsParticleEnvMemoGen = sNdsParticleMaterialGen;
            sNdsParticleEnvMemoAtlas = atlas_name;
            sNdsParticleEnvMemoPrim = prim_key;
            sNdsParticleEnvMemoEnv = env_key;
            sNdsParticleEnvMemoName = env_palette_name;
        }
        if (env_palette_name == 0u)
        {
            gNdsParticleEnvVariantFallbackCount++;
        }
    }
    return ndsRendererSubmitParticleQuadPacket(
        atlas_name, env_palette_name,
        center_q8, right_leg_q8, up_leg_q8,
        color, poly_alpha, mirror_mask,
        atlas_x, atlas_y, atlas_w, atlas_h);
}

#define NDS_RENDERER_WHISPY_BASIS_SHIFT 14u
#define NDS_RENDERER_WHISPY_SIZE_SHIFT 8u
#define NDS_RENDERER_WHISPY_COORD_SHIFT 12u
#define NDS_RENDERER_WHISPY_LEG_SHIFT \
    (NDS_RENDERER_WHISPY_BASIS_SHIFT + NDS_RENDERER_WHISPY_SIZE_SHIFT - \
     NDS_RENDERER_WHISPY_COORD_SHIFT)

static s32 sNdsRendererWhispyRight[3];
static s32 sNdsRendererWhispyUp[3];
static u32 sNdsRendererWhispyBasisValid;
static u32 sNdsRendererWhispyLeanBindingMask;
static u32 sNdsRendererWhispyLegCacheValid;
static s32 sNdsRendererWhispyLegCacheSizeQ8;
static u32 sNdsRendererWhispyLegCacheMirrorMask;
static s32 sNdsRendererWhispyLegCacheRight[3];
static s32 sNdsRendererWhispyLegCacheUp[3];

/* Same-order command offload.  Forty-eight is the source particle-pool cap;
 * 1,024 words covers its worst case (16 words per quad plus every possible
 * texture/alpha transition) with room to spare.  Overflow still has an
 * intentional policy: publish the current packet, then continue in the empty
 * buffer.  No source list pointer or particle lifetime is ever rewritten. */
#define NDS_RENDERER_WHISPY_PACKET_WORDS 1024u
typedef struct NDSRendererWhispyPacket
{
    u32 words[NDS_RENDERER_WHISPY_PACKET_WORDS];
    u32 word_count;
    u32 final_texture_name;
    u32 final_texture_format;
    u32 final_palette_format;
    s32 final_palette_name;
    u32 final_poly_alpha;
    u32 generic_words;
    u32 lean_counters;
    u32 lean_quads;
    u32 lean_state_groups;
    u32 lean_sheet_breaks;
    u32 lean_alpha_breaks;
} NDSRendererWhispyPacket;

static NDSRendererWhispyPacket sNdsRendererWhispyPacket
    __attribute__((aligned(32)));
volatile u32 gNdsWhispyAOTTier3FastBinds;
volatile u32 gNdsWhispyAOTTier3BindFallbacks;
volatile u32 gNdsWhispyAOTTier4PacketQuads;
volatile u32 gNdsWhispyAOTTier4PacketStateGroups;
volatile u32 gNdsWhispyAOTTier4PacketFlushes;
volatile u32 gNdsWhispyAOTTier4PacketWords;
volatile u32 gNdsWhispyAOTTier4PacketFallbacks;
volatile u32 gNdsParticlePacketQuads;
volatile u32 gNdsParticlePacketStateGroups;
volatile u32 gNdsParticlePacketFlushes;
volatile u32 gNdsParticlePacketWords;
volatile u32 gNdsParticlePacketFallbacks;
volatile u32 gNdsParticlePacketDynamicBinds;

static const NDSRendererWhispyNativeBinding *
ndsRendererWhispyNativeBindingFor(u32 texture_name, u32 texture_slot)
{
    const NDSRendererWhispyNativeBinding *binding;

    if (texture_slot >=
        (NDS_WHISPY_NATIVE_TEXTURE_COUNT + NDS_R2_FOX_BLASTER_GLOW_AOT))
    {
        return NULL;
    }
    binding = &sNdsRendererWhispyNativeBinding[texture_slot];
    if ((binding->valid == FALSE) ||
        (binding->texture_name == 0u) ||
        (binding->texture_name != texture_name))
    {
        return NULL;
    }
    return binding;
}

/* Exact glBindTexture state without libnds's DynamicArray lookup.  The cached
 * words and active palette/name were captured while this same immutable GL
 * object was bound at scene preparation, so both libnds's software state and
 * the DS registers advance together. */
static NDS_RENDERER_FAST_RUN_CODE sb32
ndsRendererBindWhispyNativeTextureFast(u32 texture_name, u32 texture_slot)
{
    const NDSRendererWhispyNativeBinding *binding =
        ndsRendererWhispyNativeBindingFor(texture_name, texture_slot);

    if (binding == NULL)
    {
        gNdsWhispyAOTTier3BindFallbacks++;
        ndsRendererHardwareBindTextureName(NULL, texture_name);
        return FALSE;
    }
    if (sNdsRendererHardwareBoundTextureName != texture_name)
    {
        ndsRendererHardwareEndBatch();
        GFX_TEX_FORMAT = binding->texture_format;
        GFX_PAL_FORMAT = binding->palette_format;
        glGlob->activeTexture = (int)texture_name;
        glGlob->activePalette = (int)binding->palette_name;
        sNdsRendererHardwareBoundTextureName = texture_name;
        ndsRendererHardwareInvalidateGXState(
            NDS_RENDERER_GX_STATE_TEXTURE_PARAMS);
        ndsRendererProfileRecordTextureBind();
        gNdsWhispyAOTTier3FastBinds++;
    }
    sNdsRendererHardwareActiveTextureEntry = NULL;
    return TRUE;
}

static NDS_RENDERER_FAST_RUN_CODE void
ndsRendererFlushWhispyNativePacket(void)
{
    NDSRendererWhispyPacket *packet = &sNdsRendererWhispyPacket;
    u32 word_count = packet->word_count;
    u32 generic_words = packet->generic_words;

    if (word_count == 0u)
    {
        return;
    }
    DC_FlushRange(packet->words, word_count * sizeof(u32));
    NDS_GX_DMA0_WAIT(7u);
    DMA_SRC(0) = (u32)packet->words;
    DMA_DEST(0) = (u32)&GFX_FIFO;
    DMA_CR(0) = DMA_FIFO | word_count;
    NDS_GX_DMA0_WAIT(8u);

    if (packet->final_texture_name != 0u)
    {
        glGlob->activeTexture = (int)packet->final_texture_name;
        glGlob->activePalette = (int)packet->final_palette_name;
        sNdsRendererHardwareBoundTextureName = packet->final_texture_name;
    }
    else
    {
        /* This cannot occur after admission, but invalidating both software
         * trackers makes a later ordinary bind re-establish real hardware
         * state instead of inheriting a packet whose identity was lost. */
        glGlob->activeTexture = 0;
        glGlob->activePalette = 0;
        sNdsRendererHardwareBoundTextureName = 0u;
        gNdsWhispyAOTTier4PacketFallbacks++;
    }
    ndsRendererHardwareInvalidateGXState(
        NDS_RENDERER_GX_STATE_TEXTURE_PARAMS);
    sNdsRendererGXStateShadow.poly_fmt =
        POLY_ALPHA(packet->final_poly_alpha) |
        POLY_CULL_NONE | POLY_ID(0);
    sNdsRendererGXStateShadow.valid_mask |= NDS_RENDERER_GX_STATE_POLY_FMT;
    sNdsRendererHardwareActiveTextureEntry = NULL;
    sNdsRendererParticlePacketStateDirty = FALSE;
    if (packet->lean_counters != FALSE)
    {
        NDS_DIAG(gNdsWhispyAOTTier4PacketQuads += packet->lean_quads);
        gNdsWhispyAOTTier4PacketStateGroups += packet->lean_state_groups;
        NDS_DIAG(gNdsParticleQuadSheetBreaks += packet->lean_sheet_breaks);
        gNdsParticleQuadAlphaBreaks += packet->lean_alpha_breaks;
    }
    gNdsWhispyAOTTier4PacketFlushes++;
    gNdsWhispyAOTTier4PacketWords += word_count;
    if (generic_words != 0u)
    {
        gNdsParticlePacketFlushes++;
        gNdsParticlePacketWords += generic_words;
    }
    packet->word_count = 0u;
    packet->generic_words = 0u;
    packet->lean_quads = 0u;
    packet->lean_state_groups = 0u;
    packet->lean_sheet_breaks = 0u;
    packet->lean_alpha_breaks = 0u;
}

static u32 *ndsRendererWhispyPacketReserve(u32 words);

static inline void ndsRendererParticlePacketSetFinalBinding(
    NDSRendererWhispyPacket *packet,
    u32 texture_name, u32 texture_format,
    u32 palette_format, s32 palette_name,
    u32 poly_alpha)
{
    packet->final_texture_name = texture_name;
    packet->final_texture_format = texture_format;
    packet->final_palette_format = palette_format;
    packet->final_palette_name = palette_name;
    packet->final_poly_alpha = poly_alpha;
}

/* Common raw material transition for the shared particle FIFO.  A logical
 * palette key is kept separately from PLTT_BASE: ENV variants are GL names,
 * while an ordinary atlas draw uses key 0 even though its raw palette address
 * is nonzero.  state-dirty forces both texture words after a resource upload
 * disturbed live GX state. */
static inline __attribute__((always_inline)) sb32
ndsRendererAppendParticlePacketStateRaw(
    u32 texture_name, u32 texture_format,
    u32 palette_format, s32 palette_name,
    u32 palette_key, u32 poly_alpha,
    sb32 whispy_account)
{
    NDSRendererWhispyPacket *packet = &sNdsRendererWhispyPacket;
    u32 logical_texture_changed =
        (texture_name != sNdsRendererParticleQuadTexture) ? TRUE : FALSE;
    u32 logical_palette_changed =
        (palette_key != sNdsRendererParticleQuadPalette) ? TRUE : FALSE;
    u32 texture_or_palette_changed =
        (logical_texture_changed != FALSE) ||
        (logical_palette_changed != FALSE) ||
        (sNdsRendererParticlePacketStateDirty != FALSE);
    u32 alpha_changed =
        (poly_alpha != sNdsRendererParticleQuadAlpha) ? TRUE : FALSE;
    u32 words = 0u;
    u32 *out;

    if ((texture_or_palette_changed == FALSE) && (alpha_changed == FALSE))
    {
        ndsRendererParticlePacketSetFinalBinding(
            packet, texture_name, texture_format,
            palette_format, palette_name, poly_alpha);
        return TRUE;
    }
    if ((texture_or_palette_changed != FALSE) && (alpha_changed != FALSE))
    {
        words = 5u;
        out = ndsRendererWhispyPacketReserve(words);
        if (out == NULL) { return FALSE; }
        out[0] = FIFO_COMMAND_PACK(
            FIFO_TEX_FORMAT, FIFO_PAL_FORMAT,
            FIFO_POLY_FORMAT, FIFO_BEGIN);
        out[1] = texture_format;
        out[2] = palette_format;
        out[3] = POLY_ALPHA(poly_alpha) | POLY_CULL_NONE | POLY_ID(0);
        out[4] = GL_QUAD;
    }
    else if (texture_or_palette_changed != FALSE)
    {
        words = 4u;
        out = ndsRendererWhispyPacketReserve(words);
        if (out == NULL) { return FALSE; }
        out[0] = FIFO_COMMAND_PACK(
            FIFO_TEX_FORMAT, FIFO_PAL_FORMAT, FIFO_BEGIN, FIFO_NOP);
        out[1] = texture_format;
        out[2] = palette_format;
        out[3] = GL_QUAD;
    }
    else
    {
        words = 3u;
        out = ndsRendererWhispyPacketReserve(words);
        if (out == NULL) { return FALSE; }
        out[0] = FIFO_COMMAND_PACK(
            FIFO_POLY_FORMAT, FIFO_BEGIN, FIFO_NOP, FIFO_NOP);
        out[1] = POLY_ALPHA(poly_alpha) | POLY_CULL_NONE | POLY_ID(0);
        out[2] = GL_QUAD;
    }

    if (logical_texture_changed != FALSE)
    {
        sNdsRendererParticleQuadTexture = texture_name;
        if ((packet->lean_counters != FALSE) && (whispy_account != FALSE))
        {
            packet->lean_sheet_breaks++;
        }
        else
        {
            gNdsParticleQuadSheetBreaks++;
        }
        ndsRendererProfileRecordTextureBind();
    }
    if (logical_palette_changed != FALSE)
    {
        sNdsRendererParticleQuadPalette = palette_key;
        NDS_DIAG(gNdsParticleQuadPaletteBreaks++);
    }
    if (alpha_changed != FALSE)
    {
        sNdsRendererParticleQuadAlpha = poly_alpha;
        if ((packet->lean_counters != FALSE) && (whispy_account != FALSE))
        {
            packet->lean_alpha_breaks++;
        }
        else
        {
            gNdsParticleQuadAlphaBreaks++;
        }
    }
    sNdsRendererParticlePacketStateDirty = FALSE;
    ndsRendererParticlePacketSetFinalBinding(
        packet, texture_name, texture_format,
        palette_format, palette_name, poly_alpha);
    if (whispy_account != FALSE)
    {
        if (packet->lean_counters != FALSE)
        {
            packet->lean_state_groups++;
        }
        else
        {
            gNdsWhispyAOTTier4PacketStateGroups++;
        }
    }
    else
    {
        packet->generic_words += words;
        NDS_DIAG(gNdsParticlePacketStateGroups++);
    }
    return TRUE;
}

static u32 *ndsRendererWhispyPacketReserve(u32 words)
{
    NDSRendererWhispyPacket *packet = &sNdsRendererWhispyPacket;
    u32 *out;

    if (words > NDS_RENDERER_WHISPY_PACKET_WORDS)
    {
        gNdsWhispyAOTTier4PacketFallbacks++;
        return NULL;
    }
    if (packet->word_count + words > NDS_RENDERER_WHISPY_PACKET_WORDS)
    {
        ndsRendererFlushWhispyNativePacket();
    }
    out = &packet->words[packet->word_count];
    packet->word_count += words;
    return out;
}

/* Append only the state that actually changed, followed by the glBegin that
 * latches it.  The source particle order is untouched: this is the same
 * command sequence the immediate path emits, merely packed in cached RAM. */
static inline __attribute__((always_inline)) sb32
ndsRendererAppendWhispyPacketState(u32 texture_slot, u32 poly_alpha)
{
    /* The submit proved the slot's binding before it opened the packet. */
    const NDSRendererWhispyNativeBinding *binding =
        &sNdsRendererWhispyNativeBinding[texture_slot];

    return ndsRendererAppendParticlePacketStateRaw(
        binding->texture_name, binding->texture_format,
        binding->palette_format, binding->palette_name,
        0u, poly_alpha, TRUE);
}

/* COLOR + four (TEXCOORD, VERTEX16) pairs.  There are nine commands and
 * thirteen parameter words, packed into exactly sixteen FIFO words. */
static inline __attribute__((always_inline)) sb32
ndsRendererAppendWhispyPacketQuad(u32 color,
                                  const v16 vertex[4][3],
                                  u32 texture_w, u32 texture_h,
                                  u32 texture_slot, u32 poly_alpha)
{
    u32 *out = ndsRendererWhispyPacketReserve(16u);

    if (out == NULL)
    {
        return FALSE;
    }
    out[0] = FIFO_COMMAND_PACK(
        FIFO_COLOR, FIFO_TEX_COORD, FIFO_VERTEX16, FIFO_TEX_COORD);
    out[1] = color;
    out[2] = TEXTURE_PACK((t16)0, (t16)(texture_h << 4));
    out[3] = (u32)(u16)vertex[0][0] |
             ((u32)(u16)vertex[0][1] << 16);
    out[4] = (u32)(s32)vertex[0][2];
    out[5] = TEXTURE_PACK((t16)(texture_w << 4),
                          (t16)(texture_h << 4));

    out[6] = FIFO_COMMAND_PACK(
        FIFO_VERTEX16, FIFO_TEX_COORD, FIFO_VERTEX16, FIFO_TEX_COORD);
    out[7] = (u32)(u16)vertex[1][0] |
             ((u32)(u16)vertex[1][1] << 16);
    out[8] = (u32)(s32)vertex[1][2];
    out[9] = TEXTURE_PACK((t16)(texture_w << 4), (t16)0);
    out[10] = (u32)(u16)vertex[2][0] |
              ((u32)(u16)vertex[2][1] << 16);
    out[11] = (u32)(s32)vertex[2][2];
    out[12] = TEXTURE_PACK((t16)0, (t16)0);

    out[13] = FIFO_COMMAND_PACK(
        FIFO_VERTEX16, FIFO_NOP, FIFO_NOP, FIFO_NOP);
    out[14] = (u32)(u16)vertex[3][0] |
              ((u32)(u16)vertex[3][1] << 16);
    out[15] = (u32)(s32)vertex[3][2];

    (void)texture_slot;
    sNdsRendererWhispyPacket.final_poly_alpha = poly_alpha;
    if (sNdsRendererWhispyPacket.lean_counters != FALSE)
    {
        sNdsRendererWhispyPacket.lean_quads++;
    }
    else
    {
        gNdsWhispyAOTTier4PacketQuads++;
    }
    return TRUE;
}

/* Keep a scale escalation in the same ordered FIFO stream as the vertices it
 * separates.  Flushing before every escalation made route 4 wait on DMA 32
 * extra times in the focused Whispy window even though GX can consume the
 * pop/push/scale commands in-order itself. */
static NDS_RENDERER_FAST_RUN_CODE sb32
ndsRendererAppendWhispyPacketScale(u32 scale_shift)
{
    u32 *out = ndsRendererWhispyPacketReserve(5u);
    s32 factor;

    if (out == NULL)
    {
        return FALSE;
    }
    factor = inttof32(1 << scale_shift);
    out[0] = FIFO_COMMAND_PACK(
        REG2ID(MATRIX_POP), REG2ID(MATRIX_PUSH),
        REG2ID(MATRIX_SCALE), FIFO_NOP);
    out[1] = 1u;
    out[2] = (u32)factor;
    out[3] = (u32)factor;
    out[4] = (u32)factor;
    return TRUE;
}

/* The sheet path's last answer (the particle lookup memos). */
static u32 sNdsParticleBindMemoGen;
static u32 sNdsParticleBindMemoTexture;
static u32 sNdsParticleBindMemoEnv;
static NDSRendererWhispyNativeBinding sNdsParticleBindMemo;

static sb32 ndsRendererParticlePacketBindingFor(
    u32 texture_name, u32 env_palette_name,
    NDSRendererWhispyNativeBinding *out)
{
    u32 sheet;

    if (out == NULL)
    {
        return FALSE;
    }
    if ((sNdsParticleBindMemoGen == sNdsParticleMaterialGen) &&
        (sNdsParticleBindMemoTexture == texture_name) &&
        (sNdsParticleBindMemoEnv == env_palette_name))
    {
        *out = sNdsParticleBindMemo;
        return TRUE;
    }
    sheet = ndsRendererParticleSheetForName(texture_name);
    if (sheet < NDS_PARTICLE_QUAD_ATLAS_SHEETS)
    {
        u32 slot;

        if ((sNdsRendererParticleAtlasBinding[sheet].valid == FALSE) ||
            (sNdsRendererParticleAtlasBinding[sheet].texture_name !=
             texture_name))
        {
            return FALSE;
        }
        *out = sNdsRendererParticleAtlasBinding[sheet];
        if (env_palette_name != 0u)
        {
            for (slot = 0u; slot < NDS_PARTICLE_ENV_VARIANT_COUNT; slot++)
            {
                if ((sNdsParticleEnvVariants[slot].valid != FALSE) &&
                    ((u32)sNdsParticleEnvVariants[slot].name ==
                     env_palette_name) &&
                    (sNdsParticleEnvVariants[slot].sheet == sheet))
                {
                    out->palette_format =
                        sNdsParticleEnvVariants[slot].palette_format;
                    out->palette_name =
                        sNdsParticleEnvVariants[slot].palette_name;
                    break;
                }
            }
            if (slot == NDS_PARTICLE_ENV_VARIANT_COUNT)
            {
                return FALSE;
            }
        }
        sNdsParticleBindMemo = *out;
        sNdsParticleBindMemoTexture = texture_name;
        sNdsParticleBindMemoEnv = env_palette_name;
        sNdsParticleBindMemoGen = sNdsParticleMaterialGen;
        return TRUE;
    }

    /* Hyrule and any later scene-native particle texture is still admitted to
     * the same command stream.  It is not part of the common atlas table, so
     * capture its immutable raw binding on demand at the material boundary.
     * Flush first: glBindTexture writes GX state immediately. */
    if ((texture_name == 0u) || (env_palette_name != 0u))
    {
        return FALSE;
    }
    {
        int palette_format = -1;

        ndsRendererFlushWhispyNativePacket();
        ndsRendererHardwareBindTextureName(NULL, texture_name);
        glGetColorTableParameterEXT(
            GL_TEXTURE_2D, GL_COLOR_TABLE_FORMAT_EXT, &palette_format);
        if (palette_format < 0)
        {
            return FALSE;
        }
        out->texture_name = texture_name;
        out->texture_format = (u32)glGetTexParameter();
        out->palette_format = (u32)palette_format;
        out->palette_name = (s32)glGlob->activePalette;
        out->valid = TRUE;
        sNdsRendererParticlePacketStateDirty = TRUE;
        gNdsParticlePacketDynamicBinds++;
    }
    return TRUE;
}

static inline __attribute__((always_inline)) sb32
ndsRendererAppendParticlePacketQuad(
    u32 color, const v16 vertex[4][3],
    const u32 s_uv_q4[4], const u32 t_uv_q4[4])
{
    u32 *out = ndsRendererWhispyPacketReserve(16u);

    if (out == NULL)
    {
        return FALSE;
    }
    out[0] = FIFO_COMMAND_PACK(
        FIFO_COLOR, FIFO_TEX_COORD, FIFO_VERTEX16, FIFO_TEX_COORD);
    out[1] = color;
    out[2] = TEXTURE_PACK((t16)s_uv_q4[0], (t16)t_uv_q4[0]);
    out[3] = (u32)(u16)vertex[0][0] |
             ((u32)(u16)vertex[0][1] << 16);
    out[4] = (u32)(s32)vertex[0][2];
    out[5] = TEXTURE_PACK((t16)s_uv_q4[1], (t16)t_uv_q4[1]);

    out[6] = FIFO_COMMAND_PACK(
        FIFO_VERTEX16, FIFO_TEX_COORD, FIFO_VERTEX16, FIFO_TEX_COORD);
    out[7] = (u32)(u16)vertex[1][0] |
             ((u32)(u16)vertex[1][1] << 16);
    out[8] = (u32)(s32)vertex[1][2];
    out[9] = TEXTURE_PACK((t16)s_uv_q4[2], (t16)t_uv_q4[2]);
    out[10] = (u32)(u16)vertex[2][0] |
              ((u32)(u16)vertex[2][1] << 16);
    out[11] = (u32)(s32)vertex[2][2];
    out[12] = TEXTURE_PACK((t16)s_uv_q4[3], (t16)t_uv_q4[3]);

    out[13] = FIFO_COMMAND_PACK(
        FIFO_VERTEX16, FIFO_NOP, FIFO_NOP, FIFO_NOP);
    out[14] = (u32)(u16)vertex[3][0] |
              ((u32)(u16)vertex[3][1] << 16);
    out[15] = (u32)(s32)vertex[3][2];
    sNdsRendererWhispyPacket.generic_words += 16u;
    NDS_DIAG(gNdsParticlePacketQuads++);
    return TRUE;
}

static s32 ndsRendererSubmitParticleQuadPacket(
    u32 texture_name, u32 env_palette_name,
    const s32 center_q8[3],
    const s32 right_leg_q8[3], const s32 up_leg_q8[3],
    u32 color, u32 poly_alpha, u32 mirror_mask,
    u32 atlas_x, u32 atlas_y, u32 atlas_w, u32 atlas_h)
{
    NDSRendererWhispyNativeBinding binding;
    u32 ex;
    u32 ey;
    u32 ez;
    u32 extent_q8;
    u32 needed;
    u32 shift;

    if (ndsRendererParticlePacketBindingFor(
            texture_name, env_palette_name, &binding) == FALSE)
    {
        gNdsParticlePacketFallbacks++;
        return FALSE;
    }
    if (sNdsRendererParticleQuadOpen == 0u)
    {
        /* Establish the camera and balanced matrix stack once.  The first
         * immediate material bind is only bootstrap state; all source-ordered
         * material changes and vertices after it live in the packet. */
        ndsRendererPrepareWhispyQuadState(
            texture_name, poly_alpha, 0u, 2u);
        /* The opener records logical keys, but a preceding pass may have left
         * an ENV palette on this same texture. Emit the resolved raw binding
         * for the first run even when glBindTexture elides a same-name bind. */
        sNdsRendererParticlePacketStateDirty = TRUE;
    }

    ex = ndsRendererParticleAbsQ8(center_q8[0]) +
         ndsRendererParticleAbsQ8(right_leg_q8[0]) +
         ndsRendererParticleAbsQ8(up_leg_q8[0]);
    ey = ndsRendererParticleAbsQ8(center_q8[1]) +
         ndsRendererParticleAbsQ8(right_leg_q8[1]) +
         ndsRendererParticleAbsQ8(up_leg_q8[1]);
    ez = ndsRendererParticleAbsQ8(center_q8[2]) +
         ndsRendererParticleAbsQ8(right_leg_q8[2]) +
         ndsRendererParticleAbsQ8(up_leg_q8[2]);
    extent_q8 = (ex > ey) ? ex : ey;
    if (ez > extent_q8) { extent_q8 = ez; }
    if (extent_q8 <= (UINT_MAX - NDS_RENDERER_PARTICLE_EXTENT_GUARD_Q8))
    {
        extent_q8 += NDS_RENDERER_PARTICLE_EXTENT_GUARD_Q8;
    }
    else
    {
        extent_q8 = UINT_MAX;
    }
    needed = ndsRendererParticleScaleShiftForQ8(extent_q8);
    if (needed > sNdsRendererParticleScaleShift)
    {
        if (ndsRendererAppendWhispyPacketScale(needed) == FALSE)
        {
            gNdsParticlePacketFallbacks++;
            return FALSE;
        }
        sNdsRendererWhispyPacket.generic_words += 5u;
        sNdsRendererParticleScaleShift = needed;
        NDS_DIAG(gNdsParticleScaleEscalations++);
        if (needed > gNdsParticleScaleShiftMax)
        {
            gNdsParticleScaleShiftMax = needed;
        }
    }
    if (ndsRendererAppendParticlePacketStateRaw(
                 binding.texture_name, binding.texture_format,
                 binding.palette_format, binding.palette_name,
                 env_palette_name, poly_alpha, FALSE) == FALSE)
    {
        gNdsParticlePacketFallbacks++;
        return FALSE;
    }
    if (env_palette_name != 0u)
    {
        color = NDS_PARTICLE_ENV_VARIANT_WHITE;
    }

    shift = sNdsRendererParticleScaleShift;
    mirror_mask &= 3u;
    if (mirror_mask == 0u)
    {
        v16 vertex[4][3];
        u32 s_uv_q4[4];
        u32 t_uv_q4[4];
        u32 corner;
        u32 axis;
        const u32 view_space = sNdsRendererParticleViewSpace;
        v16 planar_x[2];
        v16 planar_y[2];
        v16 planar_z = 0;

        if (view_space != FALSE)
        {
            planar_x[0] = ndsRendererParticleQ8ToV16(
                center_q8[0] - right_leg_q8[0], shift);
            planar_x[1] = ndsRendererParticleQ8ToV16(
                center_q8[0] + right_leg_q8[0], shift);
            planar_y[0] = ndsRendererParticleQ8ToV16(
                center_q8[1] - up_leg_q8[1], shift);
            planar_y[1] = ndsRendererParticleQ8ToV16(
                center_q8[1] + up_leg_q8[1], shift);
            planar_z = ndsRendererParticleQ8ToV16(center_q8[2], shift);
        }

        for (corner = 0u; corner < 4u; corner++)
        {
            s32 sx = ((corner == 0u) || (corner == 3u)) ? -1 : 1;
            s32 sy = (corner < 2u) ? -1 : 1;

            if (view_space != FALSE)
            {
                vertex[corner][0] = planar_x[sx > 0];
                vertex[corner][1] = planar_y[sy > 0];
                vertex[corner][2] = planar_z;
            }
            else for (axis = 0u; axis < 3u; axis++)
            {
                vertex[corner][axis] = ndsRendererParticleQ8ToV16(
                    center_q8[axis] + right_leg_q8[axis] * sx +
                    up_leg_q8[axis] * sy, shift);
            }
            s_uv_q4[corner] =
                (atlas_x + (((corner == 0u) || (corner == 3u))
                                ? 0u : atlas_w)) << 4;
            t_uv_q4[corner] =
                (atlas_y + ((corner < 2u) ? atlas_h : 0u)) << 4;
        }
        if (ndsRendererAppendParticlePacketQuad(
                     color, vertex, s_uv_q4, t_uv_q4) == FALSE)
        {
            gNdsParticlePacketFallbacks++;
            return FALSE;
        }
    }
    else
    {
        s32 right_x[3] = { -right_leg_q8[0], 0, right_leg_q8[0] };
        s32 right_y[3] = { -right_leg_q8[1], 0, right_leg_q8[1] };
        s32 right_z[3] = { -right_leg_q8[2], 0, right_leg_q8[2] };
        s32 up_x[3] = { -up_leg_q8[0], 0, up_leg_q8[0] };
        s32 up_y[3] = { -up_leg_q8[1], 0, up_leg_q8[1] };
        s32 up_z[3] = { -up_leg_q8[2], 0, up_leg_q8[2] };
        v16 grid_x[3][3];
        v16 grid_y[3][3];
        v16 grid_z[3][3];
        u32 s_edge[3];
        u32 t_edge[3];
        u32 s_uv_edge[3];
        u32 t_uv_edge[3];
        u32 s_parts;
        u32 t_parts;
        u32 row;
        u32 column;
        const u32 view_space = sNdsRendererParticleViewSpace;
        v16 planar_x[3];
        v16 planar_y[3];
        v16 planar_z = 0;

        if (view_space != FALSE)
        {
            for (column = 0u; column < 3u; column++)
            {
                planar_x[column] = ndsRendererParticleQ8ToV16(
                    center_q8[0] + right_x[column], shift);
                planar_y[column] = ndsRendererParticleQ8ToV16(
                    center_q8[1] + up_y[column], shift);
            }
            planar_z = ndsRendererParticleQ8ToV16(center_q8[2], shift);
        }

        for (row = 0u; row < 3u; row++)
        {
            for (column = 0u; column < 3u; column++)
            {
                if (view_space != FALSE)
                {
                    grid_x[row][column] = planar_x[column];
                    grid_y[row][column] = planar_y[row];
                    grid_z[row][column] = planar_z;
                    continue;
                }
                grid_x[row][column] = ndsRendererParticleQ8ToV16(
                    center_q8[0] + right_x[column] + up_x[row], shift);
                grid_y[row][column] = ndsRendererParticleQ8ToV16(
                    center_q8[1] + right_y[column] + up_y[row], shift);
                grid_z[row][column] = ndsRendererParticleQ8ToV16(
                    center_q8[2] + right_z[column] + up_z[row], shift);
            }
        }
        if ((mirror_mask & 1u) != 0u)
        {
            s_parts = 2u;
            s_edge[0] = 0u; s_edge[1] = 1u; s_edge[2] = 2u;
            s_uv_edge[0] = atlas_x << 4;
            s_uv_edge[1] = ((atlas_x + atlas_w) << 4) - 1u;
            s_uv_edge[2] = atlas_x << 4;
        }
        else
        {
            s_parts = 1u;
            s_edge[0] = 0u; s_edge[1] = 2u;
            s_uv_edge[0] = atlas_x << 4;
            s_uv_edge[1] = (atlas_x + atlas_w) << 4;
        }
        if ((mirror_mask & 2u) != 0u)
        {
            t_parts = 2u;
            t_edge[0] = 0u; t_edge[1] = 1u; t_edge[2] = 2u;
            t_uv_edge[0] = atlas_y << 4;
            t_uv_edge[1] = ((atlas_y + atlas_h) << 4) - 1u;
            t_uv_edge[2] = atlas_y << 4;
        }
        else
        {
            t_parts = 1u;
            t_edge[0] = 0u; t_edge[1] = 2u;
            t_uv_edge[0] = (atlas_y + atlas_h) << 4;
            t_uv_edge[1] = atlas_y << 4;
        }
        for (row = 0u; row < t_parts; row++)
        {
            for (column = 0u; column < s_parts; column++)
            {
                u32 s0 = s_edge[column];
                u32 s1 = s_edge[column + 1u];
                u32 t0 = t_edge[row];
                u32 t1 = t_edge[row + 1u];
                v16 vertex[4][3] = {
                    { grid_x[t0][s0], grid_y[t0][s0], grid_z[t0][s0] },
                    { grid_x[t0][s1], grid_y[t0][s1], grid_z[t0][s1] },
                    { grid_x[t1][s1], grid_y[t1][s1], grid_z[t1][s1] },
                    { grid_x[t1][s0], grid_y[t1][s0], grid_z[t1][s0] }
                };
                u32 s_uv_q4[4] = {
                    s_uv_edge[column], s_uv_edge[column + 1u],
                    s_uv_edge[column + 1u], s_uv_edge[column]
                };
                u32 t_uv_q4[4] = {
                    t_uv_edge[row], t_uv_edge[row],
                    t_uv_edge[row + 1u], t_uv_edge[row + 1u]
                };

                if (ndsRendererAppendParticlePacketQuad(
                             color, vertex, s_uv_q4, t_uv_q4) == FALSE)
                {
                    gNdsParticlePacketFallbacks++;
                    return FALSE;
                }
            }
        }
    }
    return TRUE;
}
