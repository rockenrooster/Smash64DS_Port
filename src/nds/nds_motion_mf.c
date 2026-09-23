/* P2-2p8 Phase 3: MF1 compact motion format -- table expansion and the clip
 * decoder. See include/nds/nds_motion_mf.h for the format contract and
 * scripts/motion/mf_emit.py for the encoder; scripts/motion/check_mf_pack.py
 * compiles this file for the host and proves every clip byte-exact against
 * the BPS1 bytes the pack producer writes.
 *
 * The decode mirrors the pose parser's view of a clip
 * (src/nds/nds_ft_pose.c:709-980): per run, command words until End or Loop,
 * each followed by its payload and per-track values; then the run's tail
 * (alignment fill or source padding) so the output is the exact BPS1 image. */

#include <stdint.h>

#include <nds/nds_motion_mf.h>

#ifndef NDS_MF_CODE
#if defined(NDS_MF_HOST)
#define NDS_MF_CODE
#else
#define NDS_MF_CODE __attribute__((target("arm")))
#endif
#endif

#define MF_E_BLOB (-1)
#define MF_E_STORAGE (-2)
#define MF_E_BAD_CODE (-3)
#define MF_E_OVERFLOW (-4)
#define MF_E_STRUCT (-5)
#define MF_E_OVERRUN (-6)

#define MF_BAD_SYM 0x7FFFFFFF
#define MF_MAX_SLOTS 64

#define MF_OP_END 0u
#define MF_OP_INTERP 12u
#define MF_OP_LOOP 13u
#define MF_OP_SET_TARGET_RATE 6u

#define MF_BLOB_HEADER_BYTES 12u
#define MF_BLOB_RECIP_COUNT 256u
#define MF_TABLE_HEADER_BYTES 30u /* cls u8, ctx u8, nsym u16, nesc u16, cnt u16[12] */

/* Values per selected track: 1 for ops 2,3,6,7,8,9,10; 2 for 4,5. */
static const u8 sMfPerTrack[32] = {
    0, 0, 1, 1, 2, 2, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

/* PAY context: 1 for the block ops (1, 2, 4, 7, 9). */
static const u8 sMfBlockOp[32] = {
    0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

/* Track group per track (rotation, TraI, translation, scale). */
static const u8 sMfTrackGroup[10] = { 0, 0, 0, 1, 2, 2, 2, 3, 3, 3 };

static u32 mfRead16(const u8 *p)
{
    return (u32)p[0] | ((u32)p[1] << 8);
}

static u32 mfRead32(const u8 *p)
{
    return (u32)p[0] | ((u32)p[1] << 8) | ((u32)p[2] << 16) | ((u32)p[3] << 24);
}

static s32 mfS16(s32 v)
{
    return ((v + 0x8000) & 0xFFFF) - 0x8000;
}

/* Walk the blob. With storage == NULL only sizes it; returns the storage
 * bytes needed, or 0 when the blob is malformed. */
static u32 mfWalkTables(const u8 *blob, u32 blob_bytes, void *storage,
                        NdsMfTables *out)
{
    u32 nwords, nsucc, ntables, pos, i, total_syms, off, bytes;
    const u8 *succ_base;
    NdsMfTable *tabs = NULL;
    s32 *syms = NULL;
    u16 *succ_off = NULL;
    u16 *succ_len = NULL;

    if ((blob == NULL) || (blob_bytes < (MF_BLOB_HEADER_BYTES + 2u * MF_BLOB_RECIP_COUNT)) ||
        ((((uintptr_t)blob) & 1u) != 0u) ||
        (mfRead32(blob) != NDS_MF_TABLES_MAGIC))
    {
        return 0;
    }
    nwords = mfRead16(blob + 4);
    nsucc = mfRead16(blob + 6);
    ntables = mfRead16(blob + 8);
    pos = MF_BLOB_HEADER_BYTES + 2u * MF_BLOB_RECIP_COUNT + 2u * nwords;
    if ((nwords == 0u) || (pos > blob_bytes))
    {
        return 0;
    }
    /* Successor lists: (ctx, len, word index x len). */
    succ_base = blob + pos;
    for (i = 0; i < nsucc; i++)
    {
        u32 ctx, len;

        if ((pos + 4u) > blob_bytes)
        {
            return 0;
        }
        ctx = mfRead16(blob + pos);
        len = mfRead16(blob + pos + 2);
        if ((ctx > nwords) || (len == 0u) || ((pos + 4u + 2u * len) > blob_bytes))
        {
            return 0;
        }
        pos += 4u + 2u * len;
    }
    /* Tables: sizes first. */
    off = pos;
    total_syms = 0;
    for (i = 0; i < ntables; i++)
    {
        u32 nsym, nesc;

        if ((off + MF_TABLE_HEADER_BYTES) > blob_bytes)
        {
            return 0;
        }
        nsym = mfRead16(blob + off + 2);
        nesc = mfRead16(blob + off + 4);
        off += MF_TABLE_HEADER_BYTES + 2u * nsym + 2u * nesc;
        if ((off > blob_bytes) || (nsym == 0u) || (nsym > (1u << NDS_MF_MAX_CODE_BITS)))
        {
            return 0;
        }
        total_syms += nsym;
    }
    bytes = (u32)sizeof(NdsMfTable) * ntables + 4u * total_syms +
            2u * 2u * (nwords + 1u);
    bytes = (bytes + 3u) & ~3u;
    if (storage == NULL)
    {
        return bytes;
    }

    tabs = (NdsMfTable *)storage;
    syms = (s32 *)(tabs + ntables);
    succ_off = (u16 *)(syms + total_syms);
    succ_len = succ_off + (nwords + 1u);
    for (i = 0; i <= nwords; i++)
    {
        succ_off[i] = 0;
        succ_len[i] = 0;
    }
    {
        u32 spos = 0;
        const u8 *p = succ_base;

        for (i = 0; i < nsucc; i++)
        {
            u32 ctx = mfRead16(p);
            u32 len = mfRead16(p + 2);

            succ_off[ctx] = (u16)(spos + 2u);
            succ_len[ctx] = (u16)len;
            spos += 2u + len;
            p += 4u + 2u * len;
        }
    }
    out->words = (const u16 *)(blob + MF_BLOB_HEADER_BYTES + 2u * MF_BLOB_RECIP_COUNT);
    out->recip15 = (const u16 *)(blob + MF_BLOB_HEADER_BYTES);
    out->succ_words = (const u16 *)succ_base;
    out->succ_off = succ_off;
    out->succ_len = succ_len;
    out->nwords = (u16)nwords;
    out->ntables = (u16)ntables;
    {
        u32 c, x;

        for (c = 0; c < NDS_MF_CLASS_COUNT; c++)
        {
            for (x = 0; x < NDS_MF_CTX_COUNT; x++)
            {
                out->table[c][x] = NULL;
            }
        }
    }

    off = pos;
    for (i = 0; i < ntables; i++)
    {
        NdsMfTable *t = &tabs[i];
        const u8 *h = blob + off;
        u32 cls = h[0];
        u32 ctx = h[1];
        u32 nsym = mfRead16(h + 2);
        u32 nesc = mfRead16(h + 4);
        const u8 *sym = h + MF_TABLE_HEADER_BYTES;
        const u8 *esc = sym + 2u * nsym;
        u32 L, j, code, idx;

        if ((cls >= NDS_MF_CLASS_COUNT) || (ctx >= NDS_MF_CTX_COUNT) ||
            (out->table[cls][ctx] != NULL))
        {
            return 0;
        }
        for (j = 0; j < nsym; j++)
        {
            syms[j] = (s32)(s16)mfRead16(sym + 2u * j);
        }
        for (j = 0; j < nesc; j++)
        {
            u32 at = mfRead16(esc + 2u * j);

            if ((at >= nsym) || (syms[at] < 0) || (syms[at] > 16))
            {
                return 0;
            }
            syms[at] = NDS_MF_ESC | syms[at];
        }
        t->syms = syms;
        code = 0;
        idx = 0;
        t->first_code[0] = 0;
        t->first_index[0] = 0;
        t->count[0] = 0;
        for (L = 1; L <= NDS_MF_MAX_CODE_BITS; L++)
        {
            u32 n = mfRead16(h + 6u + 2u * (L - 1u));

            t->first_code[L] = (u16)code;
            t->first_index[L] = (u16)idx;
            t->count[L] = (u16)n;
            if ((code + n) > (1u << L))
            {
                return 0; /* over-subscribed */
            }
            code = (code + n) << 1;
            idx += n;
        }
        if (idx != nsym)
        {
            return 0;
        }
        for (j = 0; j < (1u << NDS_MF_LUT_BITS); j++)
        {
            t->lut[j] = 0;
        }
        for (L = 1; L <= NDS_MF_LUT_BITS; L++)
        {
            for (j = 0; j < t->count[L]; j++)
            {
                u32 base = ((u32)t->first_code[L] + j) << (NDS_MF_LUT_BITS - L);
                u32 fill = 1u << (NDS_MF_LUT_BITS - L);
                u32 e = (((u32)t->first_index[L] + j) << 4) | L;
                u32 r;

                for (r = 0; r < fill; r++)
                {
                    t->lut[base + r] = (u16)e;
                }
            }
        }
        out->table[cls][ctx] = t;
        syms += nsym;
        off += MF_TABLE_HEADER_BYTES + 2u * nsym + 2u * nesc;
    }
    return bytes;
}

u32 ndsMfTablesStorageBytes(const u8 *blob, u32 blob_bytes)
{
    return mfWalkTables(blob, blob_bytes, NULL, NULL);
}

s32 ndsMfExpandTables(const u8 *blob, u32 blob_bytes, void *storage,
                      u32 storage_bytes, NdsMfTables *out)
{
    u32 need = mfWalkTables(blob, blob_bytes, NULL, NULL);

    if (need == 0u)
    {
        return MF_E_BLOB;
    }
    if ((storage == NULL) || (out == NULL) || (storage_bytes < need) ||
        ((((uintptr_t)storage) & 3u) != 0u))
    {
        return MF_E_STORAGE;
    }
    return (mfWalkTables(blob, blob_bytes, storage, out) == need) ? 0 : MF_E_BLOB;
}

/* ------------------------------------------------------------------ decode */

typedef struct MfBits
{
    const u8 *data;
    u32 nbytes;
    u32 next;   /* next byte to load */
    u32 acc;    /* left-aligned: bit 31 is the next bit */
    s32 avail;  /* valid bits in acc */
    u32 used;   /* bits consumed */
} MfBits;

static inline void mfRefill(MfBits *b)
{
    while (b->avail <= 24)
    {
        u32 byte = (b->next < b->nbytes) ? b->data[b->next] : 0u;

        b->next++;
        b->acc |= byte << (24 - b->avail);
        b->avail += 8;
    }
}

static inline u32 mfBits(MfBits *b, u32 k)
{
    u32 v;

    if (k == 0u)
    {
        return 0;
    }
    mfRefill(b);
    v = b->acc >> (32u - k);
    b->acc <<= k;
    b->avail -= (s32)k;
    b->used += k;
    return v;
}

static inline s32 mfSym(MfBits *b, const NdsMfTable *t)
{
    u32 e, L;

    mfRefill(b);
    e = t->lut[b->acc >> (32u - NDS_MF_LUT_BITS)];
    if (e != 0u)
    {
        L = e & 15u;
        b->acc <<= L;
        b->avail -= (s32)L;
        b->used += L;
        return t->syms[e >> 4];
    }
    for (L = NDS_MF_LUT_BITS + 1u; L <= NDS_MF_MAX_CODE_BITS; L++)
    {
        u32 off = (b->acc >> (32u - L)) - (u32)t->first_code[L];

        if (off < (u32)t->count[L])
        {
            b->acc <<= L;
            b->avail -= (s32)L;
            b->used += L;
            return t->syms[t->first_index[L] + off];
        }
    }
    return MF_BAD_SYM;
}

/* One value of a class/context; *err latches the first failure. */
static inline s32 mfValue(MfBits *b, const NdsMfTable *t, s32 *err)
{
    s32 s;
    u32 k, bits;

    if (t == NULL)
    {
        *err = MF_E_STRUCT;
        return 0;
    }
    s = mfSym(b, t);
    if (s == MF_BAD_SYM)
    {
        *err = MF_E_BAD_CODE;
        return 0;
    }
    if (s < NDS_MF_ESC)
    {
        return s;
    }
    k = (u32)s & 31u;
    if (k == 0u)
    {
        return 0;
    }
    bits = mfBits(b, k);
    return ((bits >> (k - 1u)) != 0u) ? (s32)bits : (s32)bits - (s32)(1u << k) + 1;
}

NDS_MF_CODE s32 ndsMfDecodeClip(const NdsMfTables *T, const u8 *stream,
                                u32 stream_bytes, u8 *out, u32 out_cap,
                                u32 *bits_used)
{
    MfBits b;
    s32 err = 0;
    s32 nslot, next = 0, max_run = -1, i;
    u8 slot_used[MF_MAX_SLOTS];
    u8 slot_run[MF_MAX_SLOTS];
    u32 run_off[MF_MAX_SLOTS];
    u32 pos, r;
    const NdsMfTable *t_slot, *t_crank, *t_crank0, *t_tail, *t_rt6;

#define MF_EMIT16(v)                                  \
    do {                                              \
        u32 mf_v = (u32)(v);                          \
        if ((pos + 2u) > out_cap) { err = MF_E_OVERFLOW; goto done; } \
        out[pos] = (u8)mf_v;                          \
        out[pos + 1u] = (u8)(mf_v >> 8);              \
        pos += 2u;                                    \
    } while (0)

    b.data = stream;
    b.nbytes = stream_bytes;
    b.next = 0;
    b.acc = 0;
    b.avail = 0;
    b.used = 0;
    pos = 0;
    t_slot = T->table[nNdsMfClassSlot][0];
    t_crank = T->table[nNdsMfClassCRank][0];
    t_crank0 = T->table[nNdsMfClassCRank0][0];
    t_tail = T->table[nNdsMfClassTail][0];
    t_rt6 = T->table[nNdsMfClassRT6][0];

    nslot = mfValue(&b, T->table[nNdsMfClassNSlot][0], &err);
    if (err != 0)
    {
        goto done;
    }
    if ((nslot < 0) || (nslot > MF_MAX_SLOTS))
    {
        err = MF_E_STRUCT;
        goto done;
    }
    if ((u32)(4 * nslot) > out_cap)
    {
        err = MF_E_OVERFLOW;
        goto done;
    }
    for (i = 0; i < nslot; i++)
    {
        s32 s = mfValue(&b, t_slot, &err);
        s32 j;

        if (err != 0)
        {
            goto done;
        }
        if (s == 0)
        {
            slot_used[i] = 0;
            continue;
        }
        if (s == 1)
        {
            j = next++;
        }
        else if (s > 1)
        {
            j = next - (s - 1);
        }
        else
        {
            j = next - s;
            next = j + 1;
        }
        if ((j < 0) || (j >= MF_MAX_SLOTS))
        {
            err = MF_E_STRUCT;
            goto done;
        }
        slot_used[i] = 1;
        slot_run[i] = (u8)j;
        if (j > max_run)
        {
            max_run = j;
        }
    }
    pos = 4u * (u32)nslot;

    for (r = 0; (s32)r <= max_run; r++)
    {
        u32 prev_ctx = 0; /* 0 = run start, i + 1 = word index i */
        s32 last_v[10], last_r[10];
        u32 seen = 0;
        s32 tail;
        u32 t;

        for (t = 0; t < 10; t++)
        {
            last_v[t] = 0;
            last_r[t] = 0;
        }
        run_off[r] = pos;
        for (;;)
        {
            u32 widx, w, op, flags, per;
            u32 p = 0;
            s32 rank = -1;

            if (T->succ_len[prev_ctx] != 0u)
            {
                rank = mfValue(&b, t_crank, &err);
                if (err != 0)
                {
                    goto done;
                }
                if (rank >= (s32)T->succ_len[prev_ctx])
                {
                    err = MF_E_STRUCT;
                    goto done;
                }
            }
            if (rank >= 0)
            {
                widx = T->succ_words[T->succ_off[prev_ctx] + (u32)rank];
            }
            else
            {
                s32 v = mfValue(&b, t_crank0, &err);

                if (err != 0)
                {
                    goto done;
                }
                if ((v < 0) || (v >= (s32)T->nwords))
                {
                    err = MF_E_STRUCT;
                    goto done;
                }
                widx = (u32)v;
            }
            w = T->words[widx];
            prev_ctx = widx + 1u;
            MF_EMIT16(w);
            op = w & 0x1Fu;
            flags = (w >> 5) & 0x3FFu;
            if ((op == MF_OP_LOOP) || (op == MF_OP_INTERP))
            {
                MF_EMIT16(mfBits(&b, 16));
                if (op == MF_OP_LOOP)
                {
                    break;
                }
                continue;
            }
            if (op == MF_OP_END)
            {
                break;
            }
            if ((w >> 15) != 0u)
            {
                p = (u32)mfValue(&b, T->table[nNdsMfClassPay][sMfBlockOp[op]], &err) & 0xFFFFu;
                if (err != 0)
                {
                    goto done;
                }
                MF_EMIT16(p);
            }
            per = sMfPerTrack[op];
            if (per == 0u)
            {
                continue;
            }
            for (t = 0; t < 10; t++)
            {
                s32 d, v;
                u32 tg;

                if (((flags >> t) & 1u) == 0u)
                {
                    continue;
                }
                if (op == MF_OP_SET_TARGET_RATE)
                {
                    v = mfS16(last_r[t] + mfValue(&b, t_rt6, &err));
                    if (err != 0)
                    {
                        goto done;
                    }
                    MF_EMIT16((u32)v & 0xFFFFu);
                    last_r[t] = v;
                    continue;
                }
                tg = sMfTrackGroup[t];
                d = mfValue(&b, T->table[nNdsMfClassVD][(tg << 1) | (((seen >> t) & 1u) ^ 1u)], &err);
                if (err != 0)
                {
                    goto done;
                }
                v = mfS16(last_v[t] + d);
                MF_EMIT16((u32)v & 0xFFFFu);
                last_v[t] = v;
                seen |= 1u << t;
                if (per == 2u)
                {
                    s32 pred = 0;
                    s32 rate;

                    if ((p != 0u) && (p <= 255u))
                    {
                        pred = (mfS16(d) * (s32)T->recip15[p]) >> 15;
                    }
                    rate = mfS16(pred + mfValue(&b, T->table[nNdsMfClassRT][tg], &err));
                    if (err != 0)
                    {
                        goto done;
                    }
                    MF_EMIT16((u32)rate & 0xFFFFu);
                    last_r[t] = rate;
                }
                else
                {
                    last_r[t] = 0;
                }
            }
        }
        tail = mfValue(&b, t_tail, &err);
        if (err != 0)
        {
            goto done;
        }
        if (tail >= 0)
        {
            s32 n;

            for (n = 0; n < tail; n++)
            {
                MF_EMIT16(0);
            }
        }
        else
        {
            s32 n;

            for (n = 0; n < (-tail - 1); n++)
            {
                MF_EMIT16(mfBits(&b, 16));
            }
        }
    }
    for (i = 0; i < nslot; i++)
    {
        u32 word = (slot_used[i] == 0u) ? 0u : run_off[slot_run[i]];

        out[4 * i + 0] = (u8)word;
        out[4 * i + 1] = (u8)(word >> 8);
        out[4 * i + 2] = (u8)(word >> 16);
        out[4 * i + 3] = (u8)(word >> 24);
    }
    if (b.used > (8u * stream_bytes))
    {
        err = MF_E_OVERRUN;
    }
done:
#undef MF_EMIT16
    if (bits_used != NULL)
    {
        *bits_used = b.used;
    }
    return (err != 0) ? err : (s32)pos;
}
