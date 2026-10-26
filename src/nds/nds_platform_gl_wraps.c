/* The libnds texture-upload wraps of nds_platform.c's translation unit (it
 * includes this file in place, between the scene-transition hold and the 3D
 * viewport): every glTexImage2D / glColorTableEXT / glColorSubTableEXT /
 * glDeleteTextures the game makes ends a held 3D frame first (-Wl,--wrap,
 * mk/toolchain.mk), waits out a DMA0 copy, refuses an upload whose VRAM
 * block split libnds's unchecked malloc would fail under the libc floor,
 * and keeps P8 V9's texel aliases (ndsPlatformGlTextureAlias: a name that
 * shows a holder's block; a deleted holder hands its block to an alias).
 * Uses the hold state and helpers defined above its include. */

/* Texture writes the next scene makes while a frame with 3D is held would
 * overwrite that frame's texels under it: the link wraps libnds's uploads
 * (-Wl,--wrap, Makefile) to end the hold first. */
int __real_glTexImage2D(int target, int empty1, GL_TEXTURE_TYPE_ENUM type,
                        int sizeX, int sizeY, int empty2, int param,
                        const void *texture);
void __real_glColorTableEXT(int target, int empty1, u16 width, int empty2,
                            int empty3, const u16 *table);
void __real_glColorSubTableEXT(int target, int start, int count, int empty1,
                               int empty2, const u16 *data);
int __real_glDeleteTextures(int n, int *names);

/* P8 V9: IDENTICAL TEXELS UPLOADED ONCE. A name made by
 * ndsPlatformGlTextureAlias shows another name's texels: its libnds record
 * takes the holder's VRAM address and size with no block of its own
 * (texIndex 0), so deleting it frees nothing, while its palette and texture
 * parameters stay its own. A holder with aliases has its bit here; before it
 * is deleted (the glDeleteTextures wrap) or re-uploaded into a block of its
 * own (the glTexImage2D wrap), the block passes to one of its aliases. Only
 * textures nothing rewrites in VRAM after their upload may be shared: not the
 * texture cache's refreshed entries, the particle sheets or the carved
 * fighter images. */
#define NDS_GL_ALIAS_NAME_MAX 512u
static u32 sNdsGlAliasHolders[NDS_GL_ALIAS_NAME_MAX / 32u];
volatile u32 gNdsGlAliasMade;
volatile u32 gNdsGlAliasHandOvers;

static gl_texture_data *ndsPlatformGlTextureRecord(int name)
{
    if ((name <= 0) ||
        ((unsigned)name >= glGlobalData.texturePtrs.cur_size))
    {
        return NULL;
    }
    return (gl_texture_data *)glGlobalData.texturePtrs.data[name];
}

void ndsPlatformGlTextureAliasReset(void)
{
    memset(sNdsGlAliasHolders, 0, sizeof(sNdsGlAliasHolders));
}

/* `name`'s block, if aliases show it, to one of them. */
static NDS_COLD void ndsPlatformGlTextureAliasHandOver(int name)
{
    gl_texture_data *holder;
    gl_texture_data *heir = NULL;
    u32 heir_name = 0u;
    u32 others = 0u;
    u32 bit;
    u32 i;

    if ((name <= 0) || ((u32)name >= NDS_GL_ALIAS_NAME_MAX))
    {
        return;
    }
    bit = 1u << ((u32)name & 31u);
    if ((sNdsGlAliasHolders[(u32)name >> 5] & bit) == 0u)
    {
        return;
    }
    sNdsGlAliasHolders[(u32)name >> 5] &= ~bit;
    holder = ndsPlatformGlTextureRecord(name);
    if ((holder == NULL) || (holder->texIndex == 0u))
    {
        return;
    }
    for (i = 1u; i < glGlobalData.texturePtrs.cur_size; i++)
    {
        gl_texture_data *tex =
            (gl_texture_data *)glGlobalData.texturePtrs.data[i];

        if ((tex == NULL) || (tex == holder) || (tex->texIndex != 0u) ||
            (tex->vramAddr != holder->vramAddr))
        {
            continue;
        }
        if (heir == NULL)
        {
            heir = tex;
            heir_name = i;
        }
        else
        {
            others++;
        }
    }
    if (heir == NULL)
    {
        return;
    }
    heir->texIndex = holder->texIndex;
    holder->texIndex = 0u;
    holder->vramAddr = NULL;
    /* Aliases are made only under NDS_GL_ALIAS_NAME_MAX, so the heir always
     * has a bit for the others still showing its block. */
    if (others != 0u)
    {
        sNdsGlAliasHolders[heir_name >> 5] |= 1u << (heir_name & 31u);
    }
    NDS_DIAG(gNdsGlAliasHandOvers++);
}

NDS_COLD s32 ndsPlatformGlTextureAlias(int name, int source, int type,
                                       int size_x, int size_y, int params)
{
    gl_texture_data *tex = ndsPlatformGlTextureRecord(name);
    gl_texture_data *holder = ndsPlatformGlTextureRecord(source);
    u32 holder_name = (u32)source;
    u32 i;

    if ((tex == NULL) || (holder == NULL) || (name == source) ||
        (glGlobalData.activeTexture != name) ||
        ((u32)name >= NDS_GL_ALIAS_NAME_MAX) ||
        (tex->texIndex != 0u) || (holder->vramAddr == NULL) ||
        (type == GL_COMPRESSED) ||
        (((holder->texFormat >> 20) & 7u) != (u32)size_x) ||
        (((holder->texFormat >> 23) & 7u) != (u32)size_y) ||
        (((holder->texFormat >> 26) & 7u) != (u32)type))
    {
        return FALSE;
    }
    if (holder->texIndex == 0u)
    {
        /* `source` is an alias itself: find the name holding the block. */
        const void *block = holder->vramAddr;

        holder = NULL;
        for (i = 1u; i < glGlobalData.texturePtrs.cur_size; i++)
        {
            gl_texture_data *other =
                (gl_texture_data *)glGlobalData.texturePtrs.data[i];

            if ((other != NULL) && (other->texIndex != 0u) &&
                (other->vramAddr == block))
            {
                holder = other;
                holder_name = i;
                break;
            }
        }
    }
    if ((holder == NULL) || (holder_name >= NDS_GL_ALIAS_NAME_MAX))
    {
        return FALSE;
    }
    tex->vramAddr = holder->vramAddr;
    tex->texSize = holder->texSize;
    tex->texIndexExt = 0u;
    tex->texFormat = ((u32)size_x << 20) | ((u32)size_y << 23) |
                     ((u32)type << 26) | (holder->texFormat & 0xffffu);
    glTexParameter(GL_TEXTURE_2D, params);
    sNdsGlAliasHolders[holder_name >> 5] |= 1u << (holder_name & 31u);
    NDS_DIAG(gNdsGlAliasMade++);
    return TRUE;
}

/* libnds splits a VRAM block with a malloc it never checks (videoGL.c,
 * vramBlock__allocateBlock), one ~28-byte node per texture and per palette.
 * The campaign walk faulted there in the Giant DK battle (2026-10-02): the
 * palette manager held 303 blocks and libc's top chunk was down to 624 B.
 * An upload refused here is glTexImage2D's 0, the answer the texture cache
 * already takes when VRAM is full: it evicts its oldest texture, whose blocks
 * go back to libc, and retries. The margin keeps libc's other users (stdio
 * buffers, storage calls) clear of the edge. A palette refused leaves the
 * texture's previous palette, not a fault. */
#define NDS_PLATFORM_GL_LIBC_ROOM 4096u
__attribute__((used)) volatile u32 gNdsPlatformGlLibcRefusals;

/* libnds's texture upload copies on DMA 0 (glTexImage2D programs DMA0SAD..
 * DMA0CNT), the channel the geometry FIFO's transfers run on, and the
 * renderer leaves a FIFO transfer running for the next writer to wait on.
 * Started meanwhile, the copy rewrote that transfer mid-list: the engine
 * waited on a command's missing parameters and the next CPU write to the
 * full FIFO stalled the ARM9 on the bus, the freeze watchdog with it
 * (2026-10-18, owner hardware: a silent host freeze, music still playing).
 * Every upload and palette load waits for the transfer first (bounded:
 * NDS_GX_DMA0_WAIT clears a stall that outlasts four frames). */
#define NDS_PLATFORM_GL_DMA0_SITE 9u

int __wrap_glTexImage2D(int target, int empty1, GL_TEXTURE_TYPE_ENUM type,
                        int sizeX, int sizeY, int empty2, int param,
                        const void *texture)
{
    ndsPlatformTransitionThawIf3DShown();
    if (ndsTaskmanLibcHasRoom(NDS_PLATFORM_GL_LIBC_ROOM) == FALSE)
    {
        NDS_DIAG(gNdsPlatformGlLibcRefusals++);
        return 0;
    }
    /* New texels into a block aliases show: they keep the old ones (V9). */
    ndsPlatformGlTextureAliasHandOver(glGlobalData.activeTexture);
    NDS_GX_DMA0_WAIT(NDS_PLATFORM_GL_DMA0_SITE);
    return __real_glTexImage2D(target, empty1, type, sizeX, sizeY, empty2,
                               param, texture);
}

/* A deleted holder's block passes to an alias first (V9). */
int __wrap_glDeleteTextures(int n, int *names)
{
    int i;

    for (i = 0; i < n; i++)
    {
        ndsPlatformGlTextureAliasHandOver(names[i]);
    }
    return __real_glDeleteTextures(n, names);
}

void __wrap_glColorTableEXT(int target, int empty1, u16 width, int empty2,
                            int empty3, const u16 *table)
{
    ndsPlatformTransitionThawIf3DShown();
    /* A zero width (with no table) frees the palette: never refused. */
    if ((width != 0u) && (table != NULL) &&
        (ndsTaskmanLibcHasRoom(NDS_PLATFORM_GL_LIBC_ROOM) == FALSE))
    {
        NDS_DIAG(gNdsPlatformGlLibcRefusals++);
        return;
    }
    NDS_GX_DMA0_WAIT(NDS_PLATFORM_GL_DMA0_SITE);
    __real_glColorTableEXT(target, empty1, width, empty2, empty3, table);
}

void __wrap_glColorSubTableEXT(int target, int start, int count, int empty1,
                               int empty2, const u16 *data)
{
    ndsPlatformTransitionThawIf3DShown();
    NDS_GX_DMA0_WAIT(NDS_PLATFORM_GL_DMA0_SITE);
    __real_glColorSubTableEXT(target, start, count, empty1, empty2, data);
}
