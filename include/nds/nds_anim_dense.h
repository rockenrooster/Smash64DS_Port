#ifndef NDS_ANIM_DENSE_H
#define NDS_ANIM_DENSE_H

#include <PR/ultratypes.h>

/* Slice 32. The dense fighter-animation track: what replaces walking a list of
 * 36-byte `AObj` nodes by pointer.
 *
 * WHY, in one measurement. `ldrb r5,[r4,#5]` -- `aobj->kind` -- is 14,616,804
 * cycles, 21.5% of `gcPlayDObjAnimJoint`, at 25.64 cyc/insn over 570,065
 * executions; with `aobj->next` the bare walk is 25.3% of the largest animation
 * symbol in the build. 335 nodes a frame at 36 bytes is 12,060 bytes through a
 * 4 KB D-cache. Cycle 109 already made the pool contiguous and the cost did not
 * move, exactly as that change's own comment predicted -- contiguity buys the
 * second cache line per node and cannot buy residency. Only a smaller node can.
 * At 20 bytes the same frame streams 6,700 bytes: 1.64x the D-cache instead of
 * 2.94x. It also FREES 8,192 bytes of pool, so unlike step 9's load-time bake
 * this trades no RAM at all.
 *
 * FIGHTER JOINTS ONLY. `gcPlayDObjAnimJoint` also serves stage and item DObjs,
 * whose AObjs are shared with other writers; those keep the list. This is the
 * specialization `PROJECT_GOAL.md` asks for, not a global struct change, and
 * `AObj` itself is decomp-mirrored and not ours to repack in any case.
 *
 * THE FIELD WIDTHS ARE MEASURED, NOT CHOSEN. Every one was checked against all
 * 145,873 records the emitter produces from the shipped bank:
 *
 *   value_base/value_target/rate_target  the AUTHORED s16 disk words. The
 *     runtime value is `word * 2^-k` with `k` a per-track constant
 *     (`sNdsR2AnimFracShift`), so keeping the word is EXACT. Keeping the scaled
 *     Q12 value instead would need 24 bits for translation, and re-quantising
 *     to fit s16 would have thrown away fractional bits of every value.
 *   length          s16 Q7.  Measured -185..0 over the whole bank; exact.
 *   rate_base       s32 Q16. Authored for the cubic families, but SetVal{,Block}
 *     COMPUTES it: 753 records are fractional over -614.25..1064.80, and s16 Q4
 *     -- the only Q that fits that magnitude -- carries 81% relative error on
 *     the small values. Q16 is `NDS_R2_AQ_RF`, what the runtime already stores.
 *   length_invert   s32. Q30 for Cubic/Linear, a plain FRAME COUNT for Step --
 *     the field's double meaning is the original's, not ours. Sized s16 Q8 from
 *     its 0..64 range first, and the round-trip rejected that immediately: for a
 *     Cubic it is `1.0 / payload`, a reciprocal, and 1/17 at Q8 is off by 0.4%.
 *     Range was the wrong question for a rate. Q30 is `NDS_R2_AQ_IF`.
 *
 * `scripts/check_ftanim_dense_layout.py` asserts this struct and the emitter's
 * Python `RECORD` agree on size, offsets and Q constants. They are two halves of
 * one format in two languages, and a silent disagreement would show up as one
 * joint of one animation being subtly wrong -- invisible to a screenshot and to
 * every geometry counter. */

#define NDS_ANIM_DENSE_LENGTH_Q 7
#define NDS_ANIM_DENSE_RATE_BASE_Q 16     /* == NDS_R2_AQ_RF */
#define NDS_ANIM_DENSE_LENGTH_INVERT_Q 30 /* == NDS_R2_AQ_IF */

/* kind in the high nibble, track index in the low nibble. Both fit: the kinds
 * are 0..7 and the joint tracks are 0..9. */
#define NDS_ANIM_DENSE_TRACK_MASK 0x0fu
#define NDS_ANIM_DENSE_KIND_SHIFT 4

/* ONE record serves the ROM bank and the live track, which is why `cmd_index`
 * sits here rather than in a separate bank-only struct. The live form has three
 * spare bytes at this offset anyway, so carrying the command index costs
 * nothing and lets the runtime copy a bank record into a live slot with a plain
 * 20-byte copy -- no repacking, no field-by-field marshalling on the path this
 * whole slice exists to make cheaper. `cmd_index` is meaningless once live and
 * the player never reads it.
 *
 * The two-struct version of this was the first draft, and
 * `check_ftanim_dense_layout.py` rejected it on its first run for exactly the
 * reason the checker exists: the emitter packed nine scalars and the header
 * declared ten. */
typedef struct NDSAnimDenseTrack
{
    s32 rate_base;      /* Q16 */
    s32 length_invert;  /* Q30, or a frame count when the kind is Step */
    s16 value_base;     /* authored s16 disk word */
    s16 value_target;   /* authored s16 disk word */
    s16 rate_target;    /* authored s16 disk word */
    s16 length;         /* Q7 */
    u8 kind_track;
    u8 cmd_index;       /* bank only: which command of the script writes this */
    /* One u16 rather than `u8 pad[2]` so the C members and the emitter's
     * `struct.Struct` codes are the same sequence, and the layout checker can
     * compare them strictly instead of carrying a special case for the padding
     * -- a tolerance in a checker is a place for a real difference to hide. */
    u16 pad;
} NDSAnimDenseTrack;

/* The two s32 fields set the stride; dropping a byte elsewhere buys nothing. */
_Static_assert(sizeof(NDSAnimDenseTrack) == 20,
               "dense track stride changed -- re-run "
               "scripts/check_ftanim_dense_layout.py and the emitter together");
_Static_assert(offsetof(NDSAnimDenseTrack, rate_base) == 0,
               "dense track rate_base moved");
_Static_assert(offsetof(NDSAnimDenseTrack, length_invert) == 4,
               "dense track length_invert moved");
_Static_assert(offsetof(NDSAnimDenseTrack, value_base) == 8,
               "dense track value_base moved");
_Static_assert(offsetof(NDSAnimDenseTrack, value_target) == 10,
               "dense track value_target moved");
_Static_assert(offsetof(NDSAnimDenseTrack, rate_target) == 12,
               "dense track rate_target moved");
_Static_assert(offsetof(NDSAnimDenseTrack, length) == 14,
               "dense track length moved");
_Static_assert(offsetof(NDSAnimDenseTrack, kind_track) == 16,
               "dense track kind_track moved");
_Static_assert(offsetof(NDSAnimDenseTrack, cmd_index) == 17,
               "dense track cmd_index moved");

#endif /* NDS_ANIM_DENSE_H */
