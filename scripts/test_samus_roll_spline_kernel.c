/* Host proof kernel for the Samus roll spline fix (2026-09-06).
 *
 * Executes the ORIGINAL decomp syInterpCubic -- the same translation unit the
 * DS build links via src/import/battleship_sys_interp.c -- against an
 * ARM-layout figatree image produced by the loader simulation in
 * scripts/test_samus_roll_spline.py.
 *
 * The host cannot simply cast the image to SYInterpDesc: pointers are 8 bytes
 * here and 4 on ARM9, so this kernel reads the ARM field offsets explicitly
 * (8/12/16/20, asserted for the REAL target by the devkitARM layout probe the
 * python driver compiles) and rebuilds a host desc whose pointers reference
 * the image buffer. The math that then runs is the original code, unmodified.
 *
 * Usage: kernel <image.bin> <desc_off> <t0> <t1> ...
 * Output:
 *   H kind points_num unk04_bits length_bits points_off keyframes_off
 *     quartics_off ptrs_ok
 *   one line per t: "<i> <x_bits> <y_bits> <z_bits>"
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../decomp/BattleShip-main/decomp/src/sys/interp.c"

/* ARM9 field offsets of SYInterpDesc; see scripts/test_samus_roll_spline.py,
 * which proves them on the target toolchain before this kernel is trusted. */
#define ARM_OFF_POINTS 8u
#define ARM_OFF_LENGTH 12u
#define ARM_OFF_KEYFRAMES 16u
#define ARM_OFF_QUARTICS 20u

static u32 rd32(const u8 *img, u32 off)
{
    u32 v;

    memcpy(&v, img + off, sizeof(v));
    return v;
}

int main(int argc, char **argv)
{
    FILE *file;
    long file_bytes;
    u8 *img;
    u32 desc_off;
    SYInterpDesc desc;
    Vec3f out;
    u32 points_off, keyframes_off, quartics_off;
    u32 ptrs_ok;
    int i;

    if (argc < 4)
    {
        fprintf(stderr, "usage: %s <image> <desc_off> <t...>\n", argv[0]);
        return 2;
    }
    file = fopen(argv[1], "rb");
    if (file == NULL)
    {
        perror(argv[1]);
        return 2;
    }
    fseek(file, 0, SEEK_END);
    file_bytes = ftell(file);
    fseek(file, 0, SEEK_SET);
    img = (u8 *)malloc((size_t)file_bytes);
    if (img == NULL)
    {
        return 2;
    }
    if (fread(img, 1u, (size_t)file_bytes, file) != (size_t)file_bytes)
    {
        return 2;
    }
    fclose(file);

    desc_off = (u32)strtoul(argv[2], NULL, 0);
    points_off = rd32(img, desc_off + ARM_OFF_POINTS);
    keyframes_off = rd32(img, desc_off + ARM_OFF_KEYFRAMES);
    quartics_off = rd32(img, desc_off + ARM_OFF_QUARTICS);

    ptrs_ok = ((points_off < (u32)file_bytes) &&
               (keyframes_off < (u32)file_bytes) &&
               (quartics_off < (u32)file_bytes) &&
               (((uintptr_t)(img + points_off) & 3u) == 0u)) ? 1u : 0u;

    memset(&desc, 0, sizeof(desc));
    desc.kind = img[desc_off];
    memcpy(&desc.points_num, img + desc_off + 2u, sizeof(s16));
    memcpy(&desc.unk04, img + desc_off + 4u, sizeof(f32));
    desc.points = (Vec3f *)(img + points_off);
    memcpy(&desc.length, img + desc_off + ARM_OFF_LENGTH, sizeof(f32));
    desc.keyframes = (f32 *)(img + keyframes_off);
    desc.quartics = (f32 *)(img + quartics_off);

    printf("H %u %d %08x %08x %08x %08x %08x %u\n",
           (unsigned)desc.kind, (int)desc.points_num,
           rd32(img, desc_off + 4u), rd32(img, desc_off + ARM_OFF_LENGTH),
           points_off, keyframes_off, quartics_off, (unsigned)ptrs_ok);
    if (ptrs_ok == 0u)
    {
        return 1;
    }

    for (i = 3; i < argc; i++)
    {
        f32 t = (f32)strtod(argv[i], NULL);

        memset(&out, 0, sizeof(out));
        syInterpCubic(&out, &desc, t);
        printf("%d %08x %08x %08x\n", i - 3,
               (unsigned)rd32((const u8 *)&out.x, 0u),
               (unsigned)rd32((const u8 *)&out.y, 0u),
               (unsigned)rd32((const u8 *)&out.z, 0u));
    }
    free(img);
    return 0;
}
