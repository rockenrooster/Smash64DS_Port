/* DS storage for the SSB64 software framebuffer sets.
 *
 * Reduced from decomp's [3][230][320] (441,600 B) to [2] (294,400 B), freeing
 * 147,200 bytes of ARM9 main RAM. The extent is dictated by the VS Results
 * photo wipe's read range, and the full derivation plus the addressing
 * contracts that were checked first live on the extern in include/sys/video.h.
 * The two MUST be kept in step.
 */
#include <PR/ultratypes.h>

u16 gSYFramebufferSets[2][230][320];
