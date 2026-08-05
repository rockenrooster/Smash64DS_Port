/* DS storage for the SSB64 software Z-buffer symbol.
 *
 * The original is compiled straight out of decomp:
 *
 *     u16 gSYZBuffer[(320 * 240) - (((320 * 240) - (320 * 230)) * sizeof(u16))]
 *
 * 70,400 halfwords = 140,800 bytes. That is an N64 *software* depth buffer --
 * the RDP rasterized depth into RDRAM. The DS rasterizes depth in hardware into
 * VRAM, so no DS path can write here. Cycle 84 MEASURED that rather than
 * inferring it from a failed search, because "no dereference found" is an
 * absence and absences are the weakest evidence available.
 *
 * The measurement, whole-match gate-arm control, frames 600-607, DLDI on
 * (artifacts/performance/2026-08-05_c84-zbuffer-liveness.json):
 *
 *   - gSYZBuffer sampled at 9 points across all 70,400 halfwords: every one 0,
 *     i.e. still untouched .bss deep into gameplay.
 *   - The -6,400 border region sampled at 3 points: every one 1.
 *   - CONTROL, same run, same instant: gSYFramebufferSets[0][0][0],
 *     [1][115][160] and [2][0][0] all read 1 -- the clear loop's
 *     GPACK_RGBA5551(0,0,0,1). The probe demonstrably sees writes (0 -> 1) in
 *     the neighbouring bytes and sees none here.
 *
 * WHY THE BORDER SAMPLES ARE THE INTERESTING ONES. SYVIDEO_ZBUFFER_START
 * (sys/video.h) resolves to `gSYZBuffer - 6400`: the original array is
 * deliberately 6,400 bytes short at the front and the start pointer backs up
 * into the preceding allocation to complete a 320x230 buffer. In this build
 * gSYFramebufferSets ends at 0x02211cd0, which is exactly gSYZBuffer's address,
 * so those 6,400 bytes ARE the tail of gSYFramebufferSets[2] -- a live buffer
 * that feeds the VS Results photo wipe. They still hold the clear value, so
 * nothing writes through the start pointer either.
 *
 * Every consumer is a store or a compare, none a dereference:
 * SYVIDEO_ZBUFFER_START computes the pointer; video_bootstrap.c:19 stores
 * &gSYZBuffer into SYVideoSetup.zbuffer; syVideoInit stores that into
 * gSYVideoZBuffer; the imported scene files store SYVIDEO_ZBUFFER_START(...)
 * into their own setups; and gSYVideoZBuffer's ONLY consumer is the equality
 * check at video_bootstrap.c:34, which needs the address, not the storage.
 *
 * So the symbol has to keep existing and keep its address taken, but it does
 * not have to keep 140,800 bytes. It is reduced to the one quantity the code's
 * own pointer arithmetic names -- the 320x10 border, 6,400 bytes -- which frees
 * 134,400 bytes of ARM9 main RAM. This is a size change only: no consumer reads
 * or writes the storage, and the bootstrap equality check is unaffected because
 * the address still exists.
 *
 * If a future change ever makes something rasterize into this buffer on DS,
 * restore the full extent here and in the matching extern in include/sys/video.h
 * TOGETHER -- they must agree, and nothing takes sizeof(gSYZBuffer), which is
 * what makes the reduction safe.
 */
#include <ssb_types.h>

u16 gSYZBuffer[320 * 10];
