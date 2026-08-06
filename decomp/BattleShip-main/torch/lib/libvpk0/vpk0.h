#ifndef VPK0_H
#define VPK0_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Read the decompressed size from a VPK0-compressed buffer.
 *
 * @param src       Pointer to VPK0 data (must start with "vpk0" magic).
 * @param src_size  Size of the source buffer in bytes.
 * @return          Decompressed size in bytes, or 0 on error.
 */
uint32_t vpk0_decoded_size(const uint8_t *src, size_t src_size);

/**
 * Decode a VPK0-compressed buffer in memory.
 *
 * @param src       Pointer to VPK0 data (must start with "vpk0" magic).
 * @param src_size  Size of the source buffer in bytes.
 * @param dst       Output buffer (must be at least vpk0_decoded_size() bytes).
 * @param dst_size  Size of the output buffer.
 * @return          Number of bytes written to dst, or 0 on error.
 */
uint32_t vpk0_decode(const uint8_t *src, size_t src_size,
                     uint8_t *dst, size_t dst_size);

#ifdef __cplusplus
}
#endif

#endif /* VPK0_H */
