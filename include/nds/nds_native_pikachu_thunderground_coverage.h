/* Pikachu's GROUND Thunder Jolt: the dedicated soft-coverage texture owner.
 *
 * WHAT THIS EXISTS FOR.  The six ground segments bind through
 * `ndsRendererHardwareBindTexture`, which resolves every source texel into
 * RGB5A1 and then packs the result into GL_RGB16.  That representation has ONE
 * alpha bit.  The source images are 32x32 IA8 -- four bits of intensity and
 * four bits of ALPHA -- under
 * `rgb = (PRIMITIVE - ENVIRONMENT) * TEXEL0 + ENVIRONMENT, alpha = TEXEL0_A`,
 * so the alpha nibble IS the coverage.  `ndsRendererHardwareConvertIA` sets the
 * single alpha bit for any non-zero nibble, which promotes 2,225 of the 3,072
 * partially covered texels to fully opaque and draws the hard rim the owner
 * reported.
 *
 * WHY IT IS NOT A WIDENING OF THE SHARED CONVERTER.  The shared converter
 * already has a graded-coverage arm, but it is gated on
 * `format == NDS_RENDERER_HW_TEXTURE_FMT_I16`: an I tile whose intensity IS its
 * alpha.  This tile is IA, where the two nibbles vary independently, so the
 * arm's "one lane is the coverage" premise does not hold here and loosening its
 * gate would change every IA surface in the game.  This owner converts its own
 * three images instead, exactly as the rebirth-halo beam owns its own name.
 *
 * WHY GL_RGB8_A5.  Measured over the three images, composited over black and
 * over white against the exact N64 result at the DS's own RGB555 ceiling:
 * PAL16 + 1 alpha bit is max 238/255 error, GL_RGB32_A3 (32 colours, 8 alpha
 * levels) is 18/255, and GL_RGB8_A5 (8 colours, 32 alpha levels) is 8/255.
 * The intensity histogram is 58% zero while the alpha histogram is spread over
 * every level, so coverage precision is worth far more here than palette width
 * -- and five alpha bits carry all sixteen source levels EXACTLY, because
 * `(n * 0x11) >> 3` is injective into 0..31.  A3I5 would additionally erase the
 * faintest level outright.  The generator owns the palette and both LUTs.
 *
 * COST: NEGATIVE, MEASURED.  The shipping path does not reach GL_RGB16 at all.
 * `ndsRendererHardwarePackResolvedPal16` needs sixteen entries or fewer, and
 * the resolved image has SEVENTEEN -- all sixteen endpoint-lerp colours plus
 * the transparent one -- so every one of the three images uploads as direct
 * colour at two bytes a texel, 2,048 bytes each.  A5I3 is one byte a texel:
 * 1,024 plus a 16-byte palette.  Three images resident is 3,120 bytes instead
 * of 6,144, so this returns about 3,024 bytes of texture VRAM and three
 * generic cache entries with it.  These names are pinned for the scene rather
 * than evictable, which is the one way the trade can go the other way -- a
 * match that never drew the jolt used to pay nothing and now pays nothing
 * either, because nothing is prepared until the first ground segment draws.
 * Conversion is one pass of 1,024 texels per miss, and a miss only happens
 * when the live image, the endpoints, the tile contract, the scene texture
 * VRAM reset or the taskman heap generation moves.
 *
 * FAIL-OPEN, NOT FAIL-DARK.  Every refusal here falls back to the generic bind
 * the owner used before, so the worst case is the shipping hard edge rather
 * than a dropped segment.  `gNdsThunderGroundCoverageFallbackCount` and
 * `gNdsThunderGroundCoverageRejectStep` say when and why.
 */
#ifndef NDS_NATIVE_PIKACHU_THUNDERGROUND_COVERAGE_H
#define NDS_NATIVE_PIKACHU_THUNDERGROUND_COVERAGE_H

#include <nds/generated/nds_native_pikachu_thunderground.generated.h>

/* One GL name per live source image; the three alternate under the MObj's
 * material animation, so a single name would re-upload every few updates. */
#define NDS_NATIVE_THUNDERGROUND_COVERAGE_SLOTS \
    NDS_NATIVE_THUNDERGROUND_IMAGE_COUNT

/* gNdsThunderGroundCoverageRejectStep values.  Zero means "no refusal yet". */
#define NDS_THUNDERGROUND_COVERAGE_REJECT_NONE 0u
#define NDS_THUNDERGROUND_COVERAGE_REJECT_ARGS 1u
#define NDS_THUNDERGROUND_COVERAGE_REJECT_NO_IMAGE 2u
#define NDS_THUNDERGROUND_COVERAGE_REJECT_IMAGE_WORD 3u
#define NDS_THUNDERGROUND_COVERAGE_REJECT_TILE 4u
#define NDS_THUNDERGROUND_COVERAGE_REJECT_COMBINE 5u
#define NDS_THUNDERGROUND_COVERAGE_REJECT_ENDPOINTS 6u
#define NDS_THUNDERGROUND_COVERAGE_REJECT_SOURCE_PTR 7u
#define NDS_THUNDERGROUND_COVERAGE_REJECT_UPLOAD 8u

/* Scene-teardown release. Texture VRAM is a scene-owned resource and
 * glResetTextures invalidates every name, so anything holding one has to let
 * go; these three names are not texture-cache entries, so the cache teardown
 * loop does not reach them. Safe to call twice and safe to call before any
 * name exists. */
void ndsNativeThunderGroundReleaseCoverageTextures(void);

extern volatile u32 gNdsThunderGroundCoveragePrepareCount;
extern volatile u32 gNdsThunderGroundCoverageBindCount;
extern volatile u32 gNdsThunderGroundCoverageHitCount;
extern volatile u32 gNdsThunderGroundCoverageFallbackCount;
extern volatile u32 gNdsThunderGroundCoverageRejectStep;
extern volatile u32 gNdsThunderGroundCoverageImageMask;
extern volatile u32 gNdsThunderGroundCoverageVramBytes;

#endif
