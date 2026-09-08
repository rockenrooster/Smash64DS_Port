/* Host execution harness for src/nds/nds_native_wallpaper.c.
 *
 * Compiled with the REAL runtime TU plus these host stand-ins: the
 * platform overlay/commit/affine-queue trio (validation copied from
 * src/nds/nds_platform.c), the one-open NitroFS stream backed by host
 * files ("nitro:/..." remapped under argv[1]), and dmaFillHalfWords
 * writing into a page-guarded 256x192 VRAM stand-in: any store past the
 * layer lands in a PAGE_NOACCESS page and kills the process, so a clean
 * exit proves no guard overwrite.
 *
 * Usage: harness <nitro-root> <command> <dump-path> <asset-id> <offset>
 *                 <origin-x> <origin-y> <scale-x-q16> <scale-y-q16> <pal>
 *   command: draw | draw_twice | epoch_cycle | palette_cycle | invalidate_cycle
 *   pal: 0 none, 1 palette A, 2 palette B (palette_cycle forces A,A,B)
 * Prints `tag.key=value` lines the python oracle parses; writes the raw
 * 256x192 halfword layer to <dump-path>. Exit 0 always (results are in
 * the lines); exit 2 on misuse. Palette entries all carry bit 15 because
 * the runtime rejects any expanded texel without it.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#include <unistd.h>
#endif

#include "nds.h"

#include <nds/nds_native_wallpaper.h>
#include <nds/nds_platform.h>
#include <nds/nds_reloc_assets.h>

#define VRAM_HALFWORDS (256u * 192u)

static u16 *sVram;
static u32 sEpoch = 1u;
static s32 sQueued[4];
static u32 sQueuedValid;
static u32 sOpens;
static u32 sReads;
static u16 sPaletteA[16];
static u16 sPaletteB[16];
static char sRoot[1024];

extern volatile u32 gNdsNativeWallpaperLoadCount;
extern volatile u32 gNdsNativeWallpaperReuseCount;
extern volatile u32 gNdsNativeWallpaperReadFailureCount;

static void *hostGuardAlloc(size_t bytes)
{
#ifdef _WIN32
    SYSTEM_INFO info;
    DWORD page;
    size_t pages;
    char *base;
    char *buffer;

    GetSystemInfo(&info);
    page = info.dwPageSize;
    pages = (bytes + page - 1u) / page;
    base = (char *)VirtualAlloc(NULL, (pages + 2u) * page,
                                MEM_RESERVE, PAGE_NOACCESS);
    if (base == NULL)
    {
        abort();
    }
    buffer = base + page;
    if (VirtualAlloc(buffer, pages * page, MEM_COMMIT, PAGE_READWRITE) == NULL)
    {
        abort();
    }
    return buffer;
#else
    long page = sysconf(_SC_PAGESIZE);
    size_t pages = (bytes + (size_t)page - 1u) / (size_t)page;
    char *base = (char *)mmap(NULL, (pages + 2u) * (size_t)page, PROT_NONE,
                              MAP_PRIVATE | MAP_ANON, -1, 0);
    if (base == MAP_FAILED)
    {
        abort();
    }
    if (mprotect(base + page, pages * (size_t)page,
                 PROT_READ | PROT_WRITE) != 0)
    {
        abort();
    }
    return base + page;
#endif
}

void dmaFillHalfWords(u16 value, void *dest, u32 size)
{
    volatile u16 *out = (volatile u16 *)dest;
    u32 i;

    if ((dest != (void *)sVram) || ((size / 2u) > VRAM_HALFWORDS))
    {
        abort();
    }
    for (i = 0u; i < size / 2u; i++)
    {
        out[i] = value;
    }
}

u16 *ndsPlatformGetOriginalSpriteOverlayLayer(s32 is_foreground,
                                              u32 *out_pitch,
                                              u32 *out_width,
                                              u32 *out_height,
                                              u32 *out_epoch)
{
    (void)is_foreground;
    if (out_pitch != NULL) { *out_pitch = 256u; }
    if (out_width != NULL) { *out_width = 256u; }
    if (out_height != NULL) { *out_height = 192u; }
    if (out_epoch != NULL) { *out_epoch = sEpoch; }
    return sVram;
}

u32 ndsPlatformCommitOriginalSpriteFinalLayer(s32 is_foreground,
                                              u32 pixel_write_count)
{
    (void)is_foreground;
    if (pixel_write_count > VRAM_HALFWORDS)
    {
        return 0u;
    }
    sEpoch++;
    return sEpoch;
}

u32 ndsPlatformQueueNativeWallpaperAffine(s32 pa, s32 pd, s32 dx, s32 dy)
{
    if ((pa <= 0) || (pa > 32767) || (pd <= 0) || (pd > 32767) ||
        (dx < -134217728) || (dx > 134217727) ||
        (dy < -134217728) || (dy > 134217727))
    {
        return FALSE;
    }
    sQueued[0] = pa;
    sQueued[1] = pd;
    sQueued[2] = dx;
    sQueued[3] = dy;
    sQueuedValid = TRUE;
    return TRUE;
}

static void hostResolvePath(const char *path, char *out, size_t out_size)
{
    const char *tail = path;

    if (strncmp(path, "nitro:/", 7) == 0)
    {
        tail = path + 7;
    }
    snprintf(out, out_size, "%s/%s", sRoot, tail);
}

s32 ndsRelocAssetStreamOpen(NdsRelocAssetStream *stream, const char *path)
{
    char resolved[2048];
    FILE *file;

    sOpens++;
    if ((stream == NULL) || (path == NULL))
    {
        return FALSE;
    }
    stream->file = NULL;
    hostResolvePath(path, resolved, sizeof(resolved));
    file = fopen(resolved, "rb");
    if (file == NULL)
    {
        return FALSE;
    }
    stream->file = (void *)file;
    return TRUE;
}

s32 ndsRelocAssetStreamRead(NdsRelocAssetStream *stream, u32 offset, void *dst,
                            u32 bytes)
{
    FILE *file;

    if ((stream == NULL) || (stream->file == NULL) || (dst == NULL) ||
        (bytes == 0u))
    {
        return FALSE;
    }
    file = (FILE *)stream->file;
    if (fseek(file, (long)offset, SEEK_SET) != 0)
    {
        return FALSE;
    }
    if (fread(dst, 1u, (size_t)bytes, file) != (size_t)bytes)
    {
        return FALSE;
    }
    sReads++;
    return TRUE;
}

void ndsRelocAssetStreamClose(NdsRelocAssetStream *stream)
{
    if ((stream == NULL) || (stream->file == NULL))
    {
        return;
    }
    fclose((FILE *)stream->file);
    stream->file = NULL;
}

static void reportDraw(const char *tag, s32 result)
{
    printf("%s.result=%d\n", tag, (int)result);
    printf("%s.load=%u\n", tag, (unsigned)gNdsNativeWallpaperLoadCount);
    printf("%s.reuse=%u\n", tag, (unsigned)gNdsNativeWallpaperReuseCount);
    printf("%s.fail=%u\n", tag,
           (unsigned)gNdsNativeWallpaperReadFailureCount);
    printf("%s.opens=%u\n", tag, (unsigned)sOpens);
    printf("%s.reads=%u\n", tag, (unsigned)sReads);
    printf("%s.qpa=%d\n", tag, (int)sQueued[0]);
    printf("%s.qpd=%d\n", tag, (int)sQueued[1]);
    printf("%s.qdx=%d\n", tag, (int)sQueued[2]);
    printf("%s.qdy=%d\n", tag, (int)sQueued[3]);
    printf("%s.queued=%u\n", tag, (unsigned)sQueuedValid);
}

static s32 doDraw(const char *tag, u32 asset_id, u32 offset,
                  s32 origin_x, s32 origin_y, u32 scale_x, u32 scale_y,
                  const u16 *palette)
{
    s32 result;

    sQueuedValid = 0u;
    result = ndsNativeWallpaperDraw(asset_id, offset, origin_x, origin_y,
                                    scale_x, scale_y, palette);
    reportDraw(tag, result);
    return result;
}

static void dumpVram(const char *path)
{
    FILE *file = fopen(path, "wb");

    if (file == NULL)
    {
        fprintf(stderr, "dump open failed: %s\n", path);
        exit(2);
    }
    fwrite(sVram, sizeof(u16), VRAM_HALFWORDS, file);
    fclose(file);
}

static void usage(void)
{
    fprintf(stderr, "usage: harness <nitro-root> <command> <dump-path> "
                    "<asset-id> <offset> <origin-x> <origin-y> "
                    "<scale-x-q16> <scale-y-q16> <pal>\n");
}

int main(int argc, char **argv)
{
    const char *command;
    const char *dumpPath;
    u32 asset_id;
    u32 offset;
    u32 scale_x;
    u32 scale_y;
    s32 origin_x;
    s32 origin_y;
    const u16 *palette;
    u32 i;

    if (argc != 11)
    {
        usage();
        return 2;
    }
    snprintf(sRoot, sizeof(sRoot), "%s", argv[1]);
    command = argv[2];
    dumpPath = argv[3];
    asset_id = (u32)strtoul(argv[4], NULL, 0);
    offset = (u32)strtoul(argv[5], NULL, 0);
    origin_x = (s32)strtol(argv[6], NULL, 0);
    origin_y = (s32)strtol(argv[7], NULL, 0);
    scale_x = (u32)strtoul(argv[8], NULL, 0);
    scale_y = (u32)strtoul(argv[9], NULL, 0);
    switch (strtol(argv[10], NULL, 0))
    {
    case 1:
        palette = sPaletteA;
        break;
    case 2:
        palette = sPaletteB;
        break;
    default:
        palette = NULL;
        break;
    }

    for (i = 0u; i < 16u; i++)
    {
        sPaletteA[i] = (u16)(0x8000u | ((u16)i << 10) | ((u16)i << 5) |
                             (u16)i);
        sPaletteB[i] = (u16)(0x8000u | ((u16)(15u - i) << 10) |
                             ((u16)((i * 2u) & 31u) << 5) | (u16)(15u - i));
    }
    sVram = (u16 *)hostGuardAlloc((size_t)VRAM_HALFWORDS * sizeof(u16));

    if (strcmp(command, "draw") == 0)
    {
        (void)doDraw("d1", asset_id, offset, origin_x, origin_y,
                     scale_x, scale_y, palette);
    }
    else if (strcmp(command, "draw_twice") == 0)
    {
        (void)doDraw("d1", asset_id, offset, origin_x, origin_y,
                     scale_x, scale_y, palette);
        (void)doDraw("d2", asset_id, offset, origin_x, origin_y,
                     scale_x, scale_y, palette);
    }
    else if (strcmp(command, "epoch_cycle") == 0)
    {
        (void)doDraw("d1", asset_id, offset, origin_x, origin_y,
                     scale_x, scale_y, palette);
        sEpoch++;
        (void)doDraw("d2", asset_id, offset, origin_x, origin_y,
                     scale_x, scale_y, palette);
    }
    else if (strcmp(command, "palette_cycle") == 0)
    {
        (void)doDraw("d1", asset_id, offset, origin_x, origin_y,
                     scale_x, scale_y, sPaletteA);
        (void)doDraw("d2", asset_id, offset, origin_x, origin_y,
                     scale_x, scale_y, sPaletteA);
        (void)doDraw("d3", asset_id, offset, origin_x, origin_y,
                     scale_x, scale_y, sPaletteB);
    }
    else if (strcmp(command, "invalidate_cycle") == 0)
    {
        (void)doDraw("d1", asset_id, offset, origin_x, origin_y,
                     scale_x, scale_y, palette);
        sQueuedValid = 0u;
        ndsNativeWallpaperInvalidate();
        printf("inv.qpa=%d\n", (int)sQueued[0]);
        printf("inv.qpd=%d\n", (int)sQueued[1]);
        printf("inv.qdx=%d\n", (int)sQueued[2]);
        printf("inv.qdy=%d\n", (int)sQueued[3]);
        printf("inv.queued=%u\n", (unsigned)sQueuedValid);
        (void)doDraw("d2", asset_id, offset, origin_x, origin_y,
                     scale_x, scale_y, palette);
    }
    else
    {
        fprintf(stderr, "unknown command %s\n", command);
        usage();
        return 2;
    }

    dumpVram(dumpPath);
    return 0;
}
