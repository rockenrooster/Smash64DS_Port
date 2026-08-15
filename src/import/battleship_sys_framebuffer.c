/* DS storage for the SSB64 software framebuffer sets.
 *
 * Reduced from decomp's [3][230][320] (441,600 B) to [2] (294,400 B) and then
 * to [1][231][320] (147,840 B), freeing 147,200 + 146,560 = 293,760 bytes of
 * ARM9 main RAM in total. 231 rows is the whole-row extent of the VS Results
 * photo wipe's read range, re-derived from the wipe's own COMPILED literals
 * (base+147,220, step -640, 220 rows, 600 bytes/row -> base+7,060..+147,819).
 * The full derivation, the linked-ELF reader set, and the addressing contracts
 * that were checked first live on the extern in include/sys/video.h. The two
 * MUST be kept in step.
 *
 * aligned(4) is a contract, not decoration: the wipe reads this storage with
 * 32-bit `ldr` at base + 147,220 + 4k, and 147,220 % 4 == 0, so a 2-aligned
 * base (the natural alignment of u16) would rotate every load.
 */
#include <PR/ultratypes.h>

u16 gSYFramebufferSets[1][231][320] __attribute__((aligned(4)));
