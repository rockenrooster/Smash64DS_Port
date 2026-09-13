/* Host execution harness for src/nds/nds_native_stage_blob.c.
 * Compiled with the REAL loader TU plus these host stand-ins:
 * scene arena (syTaskmanMalloc), heap generation key, and the one-open
 * NitroFS stream backed by host files ("nitro:/..." remapped under argv[1]).
 *
 * Usage: harness <blob-root> <once|twice|regen> <gkind>
 * Prints `key=value` lines the python oracle parses. One concise line per
 * check; no dumps beyond the asserted fields. Exit 0 always (results are
 * in the lines); exit 2 on harness misuse.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nds_build_config.h"

#include <sys/taskman.h>

#include <nds/nds_startup.h>
#include <nds/nds_reloc_assets.h>
#include <nds/nds_native_stage_blob.h>

volatile u32 gNdsTaskmanHeapGeneration = 1u;

#define HOST_ARENA_BYTES (128u * 1024u)
static u8 sHostArena[HOST_ARENA_BYTES];
static u32 sHostArenaUsed = 0u;
static u32 sHostMallocCount = 0u;
static const u8 *sHostLastBase = NULL;
static u32 sHostLastSize = 0u;

/* The loader arms the renderer's warm-upload flag after a successful blob
 * load (nds_native_stage_blob.c); the owner that consumes it lives in
 * nds_renderer_native_owners.c, outside this host harness. */
volatile u32 gNdsNativeStageWarmUploads = 0u;

void *syTaskmanMalloc(size_t size, u32 align)
{
    u32 granule = (align < 4u) ? 4u : align;
    u32 base = (sHostArenaUsed + granule - 1u) & ~(granule - 1u);

    if (size == 0u)
    {
        return NULL;
    }
    if (base + (u32)size > HOST_ARENA_BYTES)
    {
        return NULL;
    }
    sHostArenaUsed = base + (u32)size;
    sHostMallocCount++;
    sHostLastBase = &sHostArena[base];
    sHostLastSize = (u32)size;
    return (void *)&sHostArena[base];
}

static char sHostRoot[1024];

static void hostResolvePath(const char *path, char *out, size_t out_size)
{
    const char *tail = path;

    if (strncmp(path, "nitro:/", 7) == 0)
    {
        tail = path + 7;
    }
    snprintf(out, out_size, "%s/%s", sHostRoot, tail);
}

s32 ndsRelocAssetStreamOpen(NdsRelocAssetStream *stream, const char *path)
{
    char resolved[2048];
    FILE *file;

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

static void dumpTablePtr(const char *prefix, unsigned index, const void *ptr)
{
    if (ptr == NULL)
    {
        printf("%s.t%u=null\n", prefix, index);
        return;
    }
    printf("%s.t%u=%ld\n", prefix, index,
           (long)((const u8 *)ptr - (const u8 *)sHostArena));
}

static void dumpPeek(const char *prefix, unsigned index, const void *ptr)
{
    unsigned i;
    const u8 *bytes = (const u8 *)ptr;
    size_t avail;
    unsigned peek;

    if (ptr == NULL)
    {
        return;
    }
    if ((sHostLastBase == NULL) || (bytes < sHostLastBase))
    {
        return;
    }
    avail = (size_t)((sHostLastBase + sHostLastSize) - bytes);
    /* Only body bytes are deterministic; anything past the slab is arena
     * garbage, so cap the peek at the resident slab. */
    peek = (avail < 16u) ? (unsigned)avail : 16u;
    if (peek == 0u)
    {
        return;
    }
    printf("%s.peek%u=", prefix, index);
    for (i = 0u; i < peek; i++)
    {
        printf("%02x", bytes[i]);
    }
    printf("\n");
}

static void dumpAttempt(const char *prefix, s32 result)
{
    const NDSNativeStageBlobPacket *packet = ndsNativeStageBlobPacket();
    unsigned i;

    printf("%s.result=%d\n", prefix, (int)result);
    printf("%s.null=%d\n", prefix, (packet == NULL) ? 1 : 0);
    printf("%s.load_count=%lu\n", prefix,
           (unsigned long)gNdsNativeStageBlobLoadCount);
    printf("%s.bytes=%lu\n", prefix,
           (unsigned long)gNdsNativeStageBlobBytesLoaded);
    printf("%s.hash=%lu\n", prefix,
           (unsigned long)gNdsNativeStageBlobHashMismatchCount);
    printf("%s.read=%lu\n", prefix,
           (unsigned long)gNdsNativeStageBlobReadFailCount);
    printf("%s.active=%lu\n", prefix,
           (unsigned long)gNdsNativeStageBlobActiveGKind);
    printf("%s.generation=%lu\n", prefix,
           (unsigned long)gNdsTaskmanHeapGeneration);
    printf("%s.mallocs=%lu\n", prefix, (unsigned long)sHostMallocCount);
    if (sHostLastBase != NULL)
    {
        printf("%s.slab=%ld:%lu\n", prefix,
               (long)(sHostLastBase - sHostArena),
               (unsigned long)sHostLastSize);
    }
    else
    {
        printf("%s.slab=none\n", prefix);
    }
    if (packet == NULL)
    {
        return;
    }
    printf("%s.mirror=%ld\n", prefix,
           (long)((const u8 *)packet - (const u8 *)sHostArena));
    printf("%s.packet_gkind=%u\n", prefix, packet->gkind);
    printf("%s.has_seg0=%u\n", prefix, packet->has_generated_segment0);
    printf("%s.rigid=%llu\n", prefix,
           (unsigned long long)packet->rigid_binding_mask);
    printf("%s.camera=%llu\n", prefix,
           (unsigned long long)packet->camera_binding_mask);
    printf("%s.c0=%u\n", prefix, packet->asset_count);
    printf("%s.c1=%u\n", prefix, packet->segment_count);
    printf("%s.c2=%u\n", prefix, packet->dobj_count);
    printf("%s.c3=%u\n", prefix, packet->binding_count);
    printf("%s.c4=%u\n", prefix, packet->run_count);
    printf("%s.c5=%u\n", prefix, packet->epoch_count);
    printf("%s.c6=%u\n", prefix, packet->material_event_count);
    printf("%s.c7=%u\n", prefix, packet->policy_count);
    printf("%s.c8=%u\n", prefix, packet->state_delta_count);
    printf("%s.c9=%u\n", prefix, packet->state_sequence_count);
    printf("%s.c10=%u\n", prefix, packet->state_span_count);
    printf("%s.c11=%u\n", prefix, packet->dense_vertex_count);
    printf("%s.c12=%u\n", prefix, packet->corner_count);
    printf("%s.c13=%u\n", prefix, packet->triangle_count);
    printf("%s.c14=%u\n", prefix, packet->source_command_count);
    printf("%s.c15=%u\n", prefix, packet->source_vertex_count);
    printf("%s.c16=%u\n", prefix, packet->vertex_command_count);
    printf("%s.c17=%u\n", prefix, packet->triangle_command_count);
    printf("%s.c18=%u\n", prefix, packet->baked_world_count);
    printf("%s.c19=%u\n", prefix, packet->raw_triangles);
    printf("%s.c20=%u\n", prefix, packet->projected_no_z_triangles);
    printf("%s.c21=%u\n", prefix, packet->projected_range_triangles);
    printf("%s.c22=%u\n", prefix, packet->cross_run_count);
    printf("%s.c23=%u\n", prefix, packet->cross_triangle_count);
    printf("%s.c24=%u\n", prefix, packet->cross_corner_count);
    {
        const void *tables[16] = {
            packet->assets, packet->segments, packet->dobjs,
            packet->bindings, packet->runs, packet->vertices,
            packet->corners, packet->epochs, packet->materials,
            packet->policies, packet->state_deltas, packet->state_sequence,
            packet->state_spans,
#if NDS_TASK51_STAGE_NATIVE
            packet->baked_world,
#else
            NULL,
#endif
            packet->binding_dobjs, packet->binding_heads
        };

        for (i = 0u; i < 16u; i++)
        {
            dumpTablePtr(prefix, i, tables[i]);
            dumpPeek(prefix, i, tables[i]);
        }
    }
    if ((packet->binding_dobjs != NULL) && (packet->binding_count > 0u) &&
        (packet->binding_count <= 64u))
    {
        unsigned n = packet->binding_count;

        printf("%s.dobjs=", prefix);
        for (i = 0u; i < n; i++)
        {
            printf("%s%u", (i == 0u) ? "" : ",", packet->binding_dobjs[i]);
        }
        printf("\n");
    }
    if ((packet->binding_heads != NULL) && (packet->binding_count > 0u) &&
        (packet->binding_count <= 64u))
    {
        unsigned n = packet->binding_count;

        printf("%s.heads=", prefix);
        for (i = 0u; i < n; i++)
        {
            printf("%s%u", (i == 0u) ? "" : ",", packet->binding_heads[i]);
        }
        printf("\n");
    }
}

int main(int argc, char **argv)
{
    const char *command;
    u32 gkind;
    s32 result;

    if (argc != 4)
    {
        fprintf(stderr, "usage: harness <blob-root> <once|twice|regen> <gkind>\n");
        return 2;
    }
    snprintf(sHostRoot, sizeof(sHostRoot), "%s", argv[1]);
    command = argv[2];
    gkind = (u32)strtoul(argv[3], NULL, 0);
    if (strcmp(command, "once") == 0)
    {
        result = ndsNativeStageBlobLoad(gkind);
        dumpAttempt("a", result);
    }
    else if (strcmp(command, "twice") == 0)
    {
        result = ndsNativeStageBlobLoad(gkind);
        dumpAttempt("first", result);
        result = ndsNativeStageBlobLoad(gkind);
        dumpAttempt("second", result);
    }
    else if (strcmp(command, "regen") == 0)
    {
        const NDSNativeStageBlobPacket *before;
        const NDSNativeStageBlobPacket *after_rewind;

        result = ndsNativeStageBlobLoad(gkind);
        dumpAttempt("cold", result);
        before = ndsNativeStageBlobPacket();
        printf("regen.cached_mirror=%ld\n",
               (before == NULL) ? -1L :
               (long)((const u8 *)before - (const u8 *)sHostArena));
        gNdsTaskmanHeapGeneration++;
        after_rewind = ndsNativeStageBlobPacket();
        printf("regen.after_rewind_null=%d\n", (after_rewind == NULL) ? 1 : 0);
        result = ndsNativeStageBlobLoad(gkind);
        dumpAttempt("reloaded", result);
    }
    else
    {
        fprintf(stderr, "unknown command %s\n", command);
        return 2;
    }
    return 0;
}
