/**
 * VPK0 decompressor — standalone, in-memory version.
 *
 * Ported from the SSB64 decompilation (src/sys/dma.c: syDmaDecodeVpk0).
 *
 * VPK0 format:
 *   [4 bytes]  Magic "vpk0" (0x76706B30)
 *   [4 bytes]  Decompressed size (big-endian u32)
 *   [1 byte]   Sample method (0 = one offset tree, non-zero = two)
 *   [variable] Huffman tree for offsets (bitstream)
 *   [variable] Huffman tree for lengths (bitstream)
 *   [variable] Compressed data stream
 *
 * The decompression uses two Huffman trees (offsets and lengths) to decode
 * LZ77-style back-references interleaved with literal bytes.
 */

#include "vpk0.h"
#include <string.h>

/* ------------------------------------------------------------------ */
/*  Bitstream reader operating on a big-endian byte buffer             */
/* ------------------------------------------------------------------ */

typedef struct {
    const uint8_t *data;
    size_t         size;   /* total bytes in buffer  */
    size_t         pos;    /* byte read cursor       */
    uint32_t       bits;   /* accumulator            */
    int            avail;  /* valid bits in 'bits'   */
} BitStream;

static void bs_init(BitStream *bs, const uint8_t *data, size_t size) {
    bs->data  = data;
    bs->size  = size;
    bs->pos   = 0;
    bs->bits  = 0;
    bs->avail = 0;
}

/* Ensure at least `need` bits are available (max 25 safe with u32). */
static int bs_refill(BitStream *bs, int need) {
    while (bs->avail < need) {
        if (bs->pos >= bs->size) {
            return -1; /* out of data */
        }
        bs->bits = (bs->bits << 8) | bs->data[bs->pos++];
        bs->avail += 8;
    }
    return 0;
}

/* Read exactly `n` bits (1..25) and return them in the low bits. */
static uint32_t bs_read(BitStream *bs, int n) {
    bs_refill(bs, n);
    bs->avail -= n;
    return (bs->bits >> bs->avail) & ((1u << n) - 1u);
}

/* ------------------------------------------------------------------ */
/*  Huffman tree                                                       */
/* ------------------------------------------------------------------ */

typedef struct HuffNode {
    struct HuffNode *left;
    struct HuffNode *right;
    int              value;
} HuffNode;

#define MAX_HUFF_NODES 64

/**
 * Build a Huffman tree from the bitstream.
 *
 * The tree is serialised in prefix notation:
 *   bit 0 → leaf:  followed by 8-bit value
 *   bit 1 → node:  combine top two items on the stack
 *
 * The tree ends when a '1' bit (node) is read with fewer than 2 items
 * on the stack.
 *
 * Returns the root node, or NULL on error.
 */
static HuffNode *build_tree(BitStream *bs, HuffNode *pool, int *pool_idx) {
    HuffNode *stack[20];
    int sp = 0;

    stack[0] = NULL;

    for (;;) {
        uint32_t bit = bs_read(bs, 1);

        if (bit != 0 && sp < 2) {
            /* End of tree */
            break;
        }

        if (*pool_idx >= MAX_HUFF_NODES) {
            return NULL; /* pool exhausted */
        }

        HuffNode *node = &pool[(*pool_idx)++];
        node->left  = NULL;
        node->right = NULL;
        node->value = 0;

        if (bit != 0) {
            /* Internal node: pop two children */
            node->left  = stack[sp - 2];
            node->right = stack[sp - 1];
            stack[sp - 2] = node;
            sp--;
        } else {
            /* Leaf: read 8-bit value */
            node->value = (int)bs_read(bs, 8);
            stack[sp++] = node;
        }
    }

    return stack[0];
}

/** Walk the tree one bit at a time until we reach a leaf. */
static int tree_decode(BitStream *bs, const HuffNode *root) {
    const HuffNode *n = root;

    while (n->left != NULL) {
        uint32_t bit = bs_read(bs, 1);
        n = bit ? n->right : n->left;
    }
    return n->value;
}

/* ------------------------------------------------------------------ */
/*  Public API                                                         */
/* ------------------------------------------------------------------ */

uint32_t vpk0_decoded_size(const uint8_t *src, size_t src_size) {
    if (!src || src_size < 8) {
        return 0;
    }
    /* Check magic: "vpk0" = 0x76 0x70 0x6B 0x30 */
    if (src[0] != 0x76 || src[1] != 0x70 || src[2] != 0x6B || src[3] != 0x30) {
        return 0;
    }
    /* Decompressed size is a big-endian u32 at offset 4 */
    return ((uint32_t)src[4] << 24) |
           ((uint32_t)src[5] << 16) |
           ((uint32_t)src[6] <<  8) |
           ((uint32_t)src[7]);
}

uint32_t vpk0_decode(const uint8_t *src, size_t src_size,
                     uint8_t *dst, size_t dst_size) {
    if (!src || !dst || src_size < 9) {
        return 0;
    }

    uint32_t dec_size = vpk0_decoded_size(src, src_size);
    if (dec_size == 0 || dec_size > dst_size) {
        return 0;
    }

    BitStream bs;
    /* Skip past the 4-byte magic; the bitstream reader will consume
     * the 4-byte decompressed size and 1-byte sample method below. */
    bs_init(&bs, src + 4, src_size - 4);

    /* Read decompressed size (already parsed, but we must advance the stream). */
    bs_read(&bs, 16); /* high half */
    bs_read(&bs, 16); /* low half  */

    /* Read sample method byte */
    uint32_t sample_method = bs_read(&bs, 8);

    /* Build Huffman trees from the bitstream */
    HuffNode node_pool[MAX_HUFF_NODES];
    int pool_idx = 0;

    HuffNode *offsets_tree = build_tree(&bs, node_pool, &pool_idx);
    HuffNode *lengths_tree = build_tree(&bs, node_pool, &pool_idx);

    if (!offsets_tree || !lengths_tree) {
        return 0;
    }

    /* Decompress */
    uint8_t *out     = dst;
    uint8_t *out_end = dst + dec_size;

    while (out < out_end) {
        uint32_t flag = bs_read(&bs, 1);

        if (flag == 0) {
            /* Literal byte */
            *out++ = (uint8_t)bs_read(&bs, 8);
        } else {
            /* LZ77 back-reference */
            uint8_t *copy_src;

            if (sample_method != 0) {
                /* Two-sample offset mode */
                int sub_offset = 0;

                /* First offset tree decode → number of extra bits */
                int extra_bits = tree_decode(&bs, offsets_tree);
                int value = (int)bs_read(&bs, extra_bits);

                if (value <= 2) {
                    sub_offset = value + 1;

                    /* Second offset tree decode */
                    int extra_bits2 = tree_decode(&bs, offsets_tree);
                    value = (int)bs_read(&bs, extra_bits2);
                }
                copy_src = out - value * 4 - sub_offset + 8;
            } else {
                /* One-sample offset mode */
                int extra_bits = tree_decode(&bs, offsets_tree);
                int value = (int)bs_read(&bs, extra_bits);
                copy_src = out - value;
            }

            /* Decode copy length */
            int len_bits = tree_decode(&bs, lengths_tree);
            int length   = (int)bs_read(&bs, len_bits);

            while (length-- > 0) {
                *out++ = *copy_src++;
            }
        }
    }

    return dec_size;
}
