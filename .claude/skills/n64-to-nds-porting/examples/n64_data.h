#ifndef PORT_N64_DATA_H
#define PORT_N64_DATA_H

/* Original portable integration helpers. Not a ROM, GBI, or relocation parser.
 * Input byte arrays must already be normalized to canonical N64 big-endian order.
 * Typed results are host values, not bytes ready to memcpy into VRAM.
 */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static inline bool port_span_ok(size_t length, size_t offset, size_t count)
{
    return offset <= length && count <= length - offset;
}

/* Unchecked leaf readers: caller has validated the containing record once. */
static inline uint16_t port_be16(const uint8_t *p)
{
    return (uint16_t)(((uint32_t)p[0] << 8) | p[1]);
}

static inline uint32_t port_be32(const uint8_t *p)
{
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

static inline int16_t port_be_s16(const uint8_t *p)
{
    const uint32_t u = port_be16(p);
    const int32_t v = u < 32768u ? (int32_t)u : (int32_t)u - 65536;
    return (int16_t)v;
}

static inline int32_t port_be_s32(const uint8_t *p)
{
    const uint32_t u = port_be32(p);
    /* Avoid an out-of-range unsigned-to-signed conversion. */
    return u <= INT32_MAX ? (int32_t)u : -1 - (int32_t)(UINT32_MAX - u);
}

typedef struct PortSourceSpan {
    const uint8_t *bytes;
    size_t size;
} PortSourceSpan;

/* Strict NORMALIZED segmented address contract: upper byte is exactly 0..15.
 * Original microcode can mask upper bits or interpret addresses differently.
 * Normalize using the actual source dialect before calling this function.
 * Never use this as a universal KSEG/physical/ROM/relocation resolver.
 * NULL and zero-size spans are rejected; no pointer is published on failure.
 */
static inline bool port_resolve_segment(const PortSourceSpan segments[16],
                                       uint32_t address, size_t count,
                                       const uint8_t **out)
{
    const uint32_t segment = address >> 24;
    const size_t offset = (size_t)(address & UINT32_C(0x00ffffff));
    if (segments == NULL || out == NULL || segment >= 16u || count == 0u)
        return false;
    const PortSourceSpan *s = &segments[segment];
    if (s->bytes == NULL || !port_span_ok(s->size, offset, count))
        return false;
    *out = s->bytes + offset;
    return true;
}

/* Standard libultra Mtx: 16 integer halfwords, then 16 fractional halfwords.
 * Result is one signed Q16.16 element in SOURCE storage order. This does not
 * change row/column convention, handedness, multiplication order, or scale.
 */
static inline bool port_n64_mtx_element(const uint8_t *bytes, size_t size,
                                       unsigned element, int32_t *out)
{
    if (bytes == NULL || out == NULL || size < 64u || element >= 16u)
        return false;
    const size_t off = (size_t)element * 2u;
    const int32_t integer = port_be_s16(bytes + off);
    const int32_t fraction = (int32_t)port_be16(bytes + 32u + off);
    *out = integer * INT32_C(65536) + fraction;
    return true;
}

/* Canonical N64 RGBA5551 -> DS direct-color R5/G5/B5/A1 bit positions.
 * Neither a byte swap nor conversion to a palette entry. Endianness has already
 * been decoded by port_be16. The DS word must be written using its video owner.
 */
static inline uint16_t port_rgba5551_to_ds(uint16_t rgba)
{
    const uint32_t r = (uint32_t)rgba >> 11;
    const uint32_t g = ((uint32_t)rgba >> 6) & 31u;
    const uint32_t b = ((uint32_t)rgba >> 1) & 31u;
    const uint32_t a = (uint32_t)rgba & 1u;
    return (uint16_t)(r | (g << 5) | (b << 10) | (a << 15));
}

/* Raw VTX_16/TEXCOORD parameter packing only, not a complete GX command list. */
static inline uint32_t port_pack_s16_pair(int16_t low, int16_t high)
{
    return (uint32_t)(uint16_t)low | ((uint32_t)(uint16_t)high << 16);
}
#endif
