/*
 * Compile-only import of BattleShip ft/ftanim.c.
 *
 * The public ftAnim* entry points still resolve to the current DS seams until
 * the fighter-data/status-manager slice can graduate naturally.
 */
#include <ft/fighter.h>
#include <sys/objman.h>

#ifndef AObjAnimAdvance
#define AObjAnimAdvance(script) ((script)++)
#endif
#ifndef DObjGetStruct
#define DObjGetStruct(gobj) ((DObj *)((gobj)->obj))
#endif
void gcPlayDObjAnimJoint(DObj *dobj);
void gcParseMObjMatAnimJoint(MObj *mobj);
void gcPlayMObjMatAnim(MObj *mobj);

#define ftAnimGetTargetValue battleship_ftAnimGetTargetValue
#define ftAnimParseDObjFigatree battleship_ftAnimParseDObjFigatree
#define func_ovl2_800ECCA4 battleship_func_ovl2_800ECCA4
#include <battleship_overlay/src/ft/ftanim.c>
#undef ftAnimGetTargetValue
#undef ftAnimParseDObjFigatree
#undef func_ovl2_800ECCA4

/* ------------------------------------------------------------------------- *
 * R2-07 cycle 109 - the figatree parser without the soft float.
 *
 * `ftAnimParseDObjFigatree` is the #1 soft-float caller in the build:
 * 24,124,071 cycles inclusive, of which 7,931,031 is soft float. The shipped
 * ARM has 45 `bl __aeabi_*` sites in this ONE function (12 i2f, 11 fadd, 6
 * fsub, 6 fmul, 5 fcmpeq, 3 fdiv, 1 fcmple, 1 fcmpgt), and the three fdiv alone
 * are 1,494,619 cycles at 109.4 a call - the most expensive helper in the build
 * by 3x. `ftAnimGetTargetValue` does not appear as a symbol at all because it is
 * inlined into every call site, which is where most of the i2f and fmul live.
 *
 * This is a VERBATIM transcription of the decomp body with only the arithmetic
 * swapped. It is not a redesign: every `AObjAnimAdvance` is preserved in count
 * and order (it is `p++`, used more than once per expression and conditionally,
 * so the advance count is load-bearing), every branch sits in the same place,
 * and the deliberately odd parts - `track <= ARRAY_COUNT(track_aobjs)` at the
 * gather, `anim_frame` written from `anim_wait` on both exit paths - are
 * reproduced exactly rather than corrected.
 *
 * Why it lives HERE and not in the shim: this TU already compiles the decomp
 * file, so every type, enum and helper is in scope. It cannot simply redefine
 * the name - the `#define` above renames the definition and its internal call
 * sites TOGETHER - so the port body takes its own name and
 * `reloc_backend_compat_shims.c`'s `ftAnimParseDObjFigatree` (the one
 * `ftParamUpdateAnimKeys` actually calls) selects it. That is the same
 * interposition `battleship_sys_objanim.c` uses for `gcPlayDObjAnimJoint`.
 *
 * WHAT IS AND IS NOT BIT-IDENTICAL, stated rather than implied:
 *
 *   - `1.0F / payload` (two sites): bit-identical. `payload` is a u16 frame
 *     count (`relocdata_types.h`: the payload word follows the command word),
 *     and this build passes no `-ffast-math`, so the compile-time `1.0f/(f32)n`
 *     initialisers below are correctly rounded - the same value the runtime
 *     divide produces, for every n the table covers.
 *   - `ftAnimGetTargetValue`: bit-identical on the six power-of-two fracs. An
 *     s16 carries at most 16 significant bits against f32's 24, so the
 *     conversion is exact with no rounding decision at all, and scaling by 2^-k
 *     is an exponent subtraction that cannot underflow at these ranges (worst
 *     case k=13 with |arg|=1 leaves exponent 114).
 *   - `(value_target - value_base) / payload` (one site, Linear): the divide is
 *     KEPT. x/n and x*(1/n) round differently, and this feeds `rate_base`, which
 *     the cubic amplifies by `length`. Linear is 1.7% of nodes, so buying a
 *     1-ulp risk there would trade real fidelity for nothing measurable.
 *   - The comparisons go through `nds_fcmp.h`, exact by that header's own
 *     exhaustive proof. Equality against the `F32_MIN`-derived sentinels is a
 *     bit-pattern compare (IEEE gives every non-zero value a unique
 *     representation); that header's positive-constant restriction binds the
 *     ORDERED predicates, not `EQ`/`NE`.
 *
 * Cycle 116, Requirement 4, changes what those statements are ABOUT. The float
 * arms above are still here and still exact, but they are now the `q == 0` arm:
 * the shipped parser writes the AObj in fixed point and the shipped player
 * reads it there. What that does and does not preserve is stated in
 * `include/nds/nds_anim_fixed.h` and MEASURED by
 * `scripts/check_r2_cubic_error_bound.py`, which drives this parser's own
 * arithmetic end to end against the decomp float reference rather than
 * bounding the evaluator alone.
 * ------------------------------------------------------------------------- */

#include <nds/nds_fcmp.h>
#include <nds/nds_anim_fixed.h>

/* Requirement 4 -- this parser is the WRITER half. The evaluator half is
 * `ndsR2AnimValueQ` in `src/import/battleship_sys_objanim.c`; the two must move
 * together, because the AObj carries state across segments (`value_base =
 * value_target`, `rate_base = rate_target`) and a half-converted list is a
 * float bit pattern read as a Q integer.
 *
 * Route bit 3 selects it on the same binary. Compiled out (the default, and
 * every published ROM) `NDS_R2_ANIM_CUT_ON` folds to a constant 1, GCC
 * dead-codes the float arms, and there is no per-event test left. */
#ifndef NDS_R2_ANIM_CUT_ROUTE
#define NDS_R2_ANIM_CUT_ROUTE 0
#endif
#if NDS_R2_ANIM_CUT_ROUTE
extern volatile u32 gNdsR2AnimCutRoute;
#define NDS_R2_ANIM_CUT_ON(bit) ((gNdsR2AnimCutRoute & (bit)) != 0u)
#else
#define NDS_R2_ANIM_CUT_ON(bit) (1)
#endif

/* 1/n for n in [1,255], built by the compiler. Entry 0 is a placeholder: every
 * reader is guarded by a payload-not-zero test, and `1.0f/0.0f` in a constant
 * initialiser is a diagnostic. 1 KiB of .rodata against 34,816 bytes of proven
 * static headroom - priced before writing, as the RAM rule requires. */
#define NDS_R2R1(n)   (1.0f / (f32)(n))
#define NDS_R2R4(n)   NDS_R2R1((n)), NDS_R2R1((n) + 1), \
                      NDS_R2R1((n) + 2), NDS_R2R1((n) + 3)
#define NDS_R2R16(n)  NDS_R2R4((n)), NDS_R2R4((n) + 4), \
                      NDS_R2R4((n) + 8), NDS_R2R4((n) + 12)
#define NDS_R2R64(n)  NDS_R2R16((n)), NDS_R2R16((n) + 16), \
                      NDS_R2R16((n) + 32), NDS_R2R16((n) + 48)
#define NDS_R2_RECIP_COUNT 256u

static const f32 sNdsR2Recip[NDS_R2_RECIP_COUNT] = {
    0.0f, NDS_R2R1(1), NDS_R2R1(2), NDS_R2R1(3),
    NDS_R2R4(4), NDS_R2R4(8), NDS_R2R16(12), NDS_R2R4(28),
    NDS_R2R64(32), NDS_R2R64(96), NDS_R2R64(160), NDS_R2R16(224),
    NDS_R2R16(240)
};

volatile u32 gNdsR2FtAnimRecipHits;
volatile u32 gNdsR2FtAnimRecipMisses;
volatile u32 gNdsR2FtAnimParseCalls;
/* Slice 32 step 11. The parse RESUMES a script rather than re-parsing it, and
 * returns before the table walk whenever the animation has not advanced to a
 * new command. So its cost splits in two, and only one half is reducible:
 *
 *   EarlyOut -- `anim_wait -= anim_speed; anim_frame += anim_speed;` plus a
 *     GObj store and two sentinel compares. This IS the animation clock. No
 *     baked track, dense or otherwise, can remove it.
 *   Stepped  -- the table walk and the event loop. This is what AOT deletes.
 *
 * The ratio decides whether slice 32 is worth building at all, so it is
 * measured before the player is written rather than after. */
volatile u32 gNdsR2FtAnimParseEarlyOut;
volatile u32 gNdsR2FtAnimParseStepped;

static inline f32 ndsR2BitsToF32(u32 bits)
{
    f32 v;

    __builtin_memcpy(&v, &bits, sizeof(v));
    return v;
}

/* Exactly (f32)v for v < 2^16: at most 16 significant bits against f32's 24, so
 * there is no rounding decision to make. */
static inline f32 ndsR2U16ToF32(u32 v)
{
    u32 shift;

    if (v == 0u)
    {
        return 0.0f;
    }
    shift = (u32)__builtin_clz(v);
    return ndsR2BitsToF32((((127u + 31u) - shift) << 23) |
                          (((v << shift) & 0x7fffffffu) >> 8));
}

/* `1.0F / payload`, a table hit whenever the frame count is in range. */
static inline f32 ndsR2Recip(u32 n)
{
    if (n < NDS_R2_RECIP_COUNT)
    {
        gNdsR2FtAnimRecipHits++;
        return sNdsR2Recip[n];
    }
    gNdsR2FtAnimRecipMisses++;
    return 1.0f / (f32)n;
}

/* `ftAnimGetTargetValue` with the i2f and the fmul replaced by an exponent
 * subtraction on the six power-of-two fracs. The shift per id is the decomp's
 * own table read as a power of two: 1/512, 1/4, 1/4096 for values, then 1/512,
 * 1/32, 1/8192 for rates. Ids 3 and 7 (TraI) are `1/16384 - 3e-12`, which is
 * not a power of two, so they keep the original expression - there is no exact
 * shift for them and TraI is one track of ten. */
static const u8 sNdsR2AnimFracShift[8] = { 9u, 2u, 12u, 0u, 9u, 5u, 13u, 0u };

static f32 ndsR2AnimTargetValue(s16 arg, s32 track, sb32 value_or_step, u32 q)
{
    s32 id;
    u32 mag;
    u32 shift;
    u32 k;

    switch (track)
    {
    case nGCAnimTrackRotX:
    case nGCAnimTrackRotY:
    case nGCAnimTrackRotZ:
        id = 0;
        break;

    case nGCAnimTrackTraX:
    case nGCAnimTrackTraY:
    case nGCAnimTrackTraZ:
        id = 1;
        break;

    case nGCAnimTrackScaX:
    case nGCAnimTrackScaY:
    case nGCAnimTrackScaZ:
        id = 2;
        break;

    case nGCAnimTrackTraI:
        id = 3;
        break;

    default:
        /* The decomp leaves `id` uninitialised here. Every caller passes a
         * joint track, so the arm is unreachable; pinning it to 0 makes the
         * unreachable case defined instead of whatever was on the stack. */
        id = 0;
        break;
    }
    if (value_or_step != 0)
    {
        id += 4;
    }
    if ((id == 3) || (id == 7))
    {
        /* TraI's scale is not a power of two, so the Q form goes through the
         * float expression and the same converter the shipped cubic uses --
         * which makes it bit-identical rather than merely close. One track of
         * ten, and only on the rare parse event. */
        f32 trai = (f32)arg * (1.0F / 16384.0F - (3.0F / 1000000000000.0F));

        return (q != 0u) ?
            ndsR2AQStore(ndsR2F32ToFixed(trai, NDS_R2_AQ_VF)) : trai;
    }
    if (q != 0u)
    {
        /* `arg * 2^-k` quantised to Q12 is `arg << (12-k)`, and the shipped
         * cubic already quantises the f32 to exactly that. Zero needs no
         * special case here -- it shifts to zero either way. */
        return ndsR2AQStore(ndsR2AnimArgToQ(
            (s32)arg, NDS_R2_AQ_VF - (s32)sNdsR2AnimFracShift[id]));
    }
    if (arg == 0)
    {
        return 0.0f;
    }
    mag = (arg < 0) ? (u32)(-(s32)arg) : (u32)arg;
    shift = (u32)__builtin_clz(mag);
    k = sNdsR2AnimFracShift[id];
    /* mag = 1.m * 2^(31-shift), so mag * 2^-k = 1.m * 2^(31-shift-k). */
    return ndsR2BitsToF32(
        ((arg < 0) ? 0x80000000u : 0u) |
        ((((127u + 31u) - shift) - k) << 23) |
        (((mag << shift) & 0x7fffffffu) >> 8));
}

/* Requirement 4 helpers. Each takes the hoisted `q` and returns the WORD to
 * store, so the twelve write sites below stay one body rather than two. */

/* `1/payload` as the Q30 reciprocal the Q cubic multiplies `length` by. Q30 is
 * the widest that holds 1/1 without saturating, and it is derived from the same
 * compile-time-folded table the float path uses, so the two agree to the f32's
 * own precision. */
static inline f32 ndsR2AnimRecipSlot(u32 n, u32 q)
{
    f32 r = ndsR2Recip(n);

    return (q != 0u) ? ndsR2AQStore(ndsR2F32ToFixed(r, NDS_R2_AQ_IF)) : r;
}

/* A u16 frame count as `length`/Step's `length_invert` carries it. */
static inline f32 ndsR2AnimFramesSlot(u32 n, f32 as_float, u32 q)
{
    return (q != 0u) ? ndsR2AQStore((s32)n << NDS_R2_AQ_LF) : as_float;
}

/* `-anim_wait - anim_speed`, the segment's starting phase. Loop-invariant
 * across the flag scan, and two soft-float operations, so the Q form converts
 * it once per event rather than once per track. */
static inline f32 ndsR2AnimSegmentStart(const DObj *root_dobj, u32 q)
{
    f32 v = -root_dobj->anim_wait - root_dobj->anim_speed;

    return (q != 0u) ? ndsR2AQStore(ndsR2F32ToFixed(v, NDS_R2_AQ_LF)) : v;
}

/* Whichever representation the AObj is in, advance `length` by `payload`. */
static inline void ndsR2AnimAddLength(AObj *a, u32 n, f32 as_float, u32 q)
{
    if (q != 0u)
    {
        a->length = ndsR2AQStore(ndsR2AQLoad(a->length) +
            ((s32)n << NDS_R2_AQ_LF));
    }
    else
    {
        a->length += as_float;
    }
}

/* Bring one AObj into Q form. Called wherever this parser is about to write a Q
 * kind onto it, which is the only place the conversion can be made safe: the
 * arms carry state forward (`value_base = value_target`), so a promotion that
 * skipped this would copy an f32 bit pattern into a Q slot.
 *
 * Cost in the steady state is one byte compare -- after the first event on a
 * DObj every one of its AObjs is already Q.
 *
 * That compare is why the conversion is out of line. The one-byte test was true
 * for 90.3 calls a frame and the whole function cost 32 cycles a call, of which
 * the profile charges **19 to `push`/`pop` alone -- 59.1% of its 2,869
 * cyc/frame**, saving registers only the conversion below ever touches.
 * ARMv5TE Thumb-1 has no conditional execution, so GCC cannot shrink-wrap a
 * prologue the tail needs past an early return; splitting the tail is the only
 * way to stop paying for it. The steady-state path is now three instructions at
 * the call site and the conversion is unchanged. */
static void __attribute__((noinline))
ndsR2AnimAObjToQConvert(AObj *a, s32 kind);

static inline void ndsR2AnimAObjToQ(AObj *a)
{
    s32 kind = (s32)a->kind;

    if (kind >= (s32)NDS_R2_AQ_KIND_BASE)
    {
        return;
    }
    ndsR2AnimAObjToQConvert(a, kind);
}

static void __attribute__((noinline))
ndsR2AnimAObjToQConvert(AObj *a, s32 kind)
{
    if (kind == nGCAnimKindNone)
    {
        /* `gcAddAObjForDObj` leaves every value at 0.0F -- the same word in both
         * formats -- and `length_invert` at 1.0F, which is NOT. It is read: a
         * Cubic event with a zero payload does not overwrite it. */
        a->length_invert = ndsR2AQStore(1 << NDS_R2_AQ_IF);
        return;
    }
    if (kind > nGCAnimKindCubic)
    {
        return;     /* nGCAnimKindSpecial: declared in objdef.h, never written */
    }
    a->length_invert = ndsR2AQStore(ndsR2F32ToFixed(a->length_invert,
        (kind == nGCAnimKindStep) ? NDS_R2_AQ_LF : NDS_R2_AQ_IF));
    a->length = ndsR2AQStore(ndsR2F32ToFixed(a->length, NDS_R2_AQ_LF));
    a->value_base = ndsR2AQStore(ndsR2F32ToFixed(a->value_base, NDS_R2_AQ_VF));
    a->value_target =
        ndsR2AQStore(ndsR2F32ToFixed(a->value_target, NDS_R2_AQ_VF));
    a->rate_base = ndsR2AQStore(ndsR2F32ToFixed(a->rate_base, NDS_R2_AQ_VF));
    a->rate_target =
        ndsR2AQStore(ndsR2F32ToFixed(a->rate_target, NDS_R2_AQ_VF));
    a->kind = (u8)(kind + ((s32)NDS_R2_AQ_KIND_BASE - nGCAnimKindStep));
}

/* The two script-exhausted exits run the same tail loop over the AObj list.
 * They are one function here for the reason the payload macro below is one
 * macro: textually identical sites must not be able to drift. `anim_speed +
 * anim_wait` is hoisted out of the loop, which is pure loop-invariant motion --
 * neither field is written inside it -- so the float arm is bit-identical. */
static void ndsR2AnimAdvanceTail(DObj *root_dobj, u32 q)
{
    f32 tail = root_dobj->anim_speed + root_dobj->anim_wait;
    s32 tail_q = (q != 0u) ? ndsR2F32ToFixed(tail, NDS_R2_AQ_LF) : 0;
    AObj *a = root_dobj->aobj;

    while (a != NULL)
    {
        if (a->kind != nGCAnimKindNone)
        {
            if (a->kind >= NDS_R2_AQ_KIND_BASE)
            {
                a->length = ndsR2AQStore(ndsR2AQLoad(a->length) + tail_q);
            }
            else
            {
                a->length += tail;
            }
        }
        a = a->next;
    }
}

/* The six payload reads are textually identical in the decomp, so they are one
 * macro here and a slip cannot differ between sites. `AObjAnimAdvance` is `p++`,
 * and the second advance happens ONLY when the toggle bit is set - that
 * asymmetry is the format (a payload word follows the command only when the
 * toggle says so), not an accident. */
#define NDS_R2_FTANIM_PAYLOAD()                                              \
    (payload_u =                                                             \
        (AObjAnimAdvance(root_dobj->anim_joint.event16)->command.toggle)      \
            ? AObjAnimAdvance(root_dobj->anim_joint.event16)->u              \
            : 0u,                                                            \
     payload = ndsR2U16ToF32(payload_u))

#define NDS_R2_FTANIM_TARGET(vos)                                            \
    ndsR2AnimTargetValue(                                                    \
        AObjAnimAdvance(root_dobj->anim_joint.event16)->s,                   \
        i + nGCAnimTrackJointStart, (vos), q)

/* Every arm that writes a kind calls this first, so migrating here covers every
 * promotion into Q form -- including the AObj this call has just created, whose
 * `length_invert` the constructor set to 1.0F. */
/* Requirement 4, next slice: build `track_aobjs[]` ON FIRST USE.
 *
 * The eager version cleared the table and walked the DObj's whole `aobj` chain
 * before the event loop -- about ten instructions over about ten 36-byte nodes,
 * reached by pointer. That is ~100 instructions against the 95.7 the profile
 * measures for the whole call, at 3.16 cyc/insn in a tier running 42.3% memory
 * stall: the walk IS the call, 200,231 times a match, rebuilding a table that
 * is invariant while the list is unchanged.
 *
 * A cross-call cache is the obvious move and the wrong one -- cycle 117 lost
 * two slices to caches in the neighbouring collision code and one failure is
 * still unexplained. This has NO cross-call state: the table is built at most
 * once per call, and only on calls that actually read it. The
 * `anim_joint.event16 == NULL` exit never does.
 *
 * Every read is covered. An audit of the 41 `track_aobjs[i]` reads against the
 * 7 `NDS_R2_FTANIM_ENSURE()` sites found exactly one block reading without it,
 * `nGCAnimEvent16SetTranslateInterp`, which the source already flags as "the
 * only creation site outside NDS_R2_FTANIM_ENSURE". Both trigger the build.
 * Missing a site would NOT crash -- ENSURE allocates on NULL -- it would
 * silently create a duplicate AObj, which is why the audit came first.
 *
 * Behind route bit 16 because it is not bit-identical: the walk also MIGRATES
 * each joint node to Q form, so a node the script never writes migrates later
 * (or not at all) and is played by the float arm meanwhile. Both arms are
 * correct -- `gcPlayDObjAnimJoint` and `ndsR2AnimAdvanceTail` each dispatch on
 * `kind >= NDS_R2_AQ_KIND_BASE` -- and the difference is inside the Q bound
 * already proven for Requirement 4, but it is a difference, so it gets an A/B
 * on one binary rather than an assertion. */
static void ndsR2AnimBuildTrackTable(DObj *root_dobj, AObj **track_aobjs,
                                     s32 count, u32 q)
{
    AObj *current_aobj = root_dobj->aobj;
    s32 k;

    for (k = 0; k < count; k++)
    {
        track_aobjs[k] = NULL;
    }
    while (current_aobj != NULL)
    {
        if ((current_aobj->track >= nGCAnimTrackJointStart) &&
            (current_aobj->track <= count))
        {
            track_aobjs[current_aobj->track - nGCAnimTrackJointStart] =
                current_aobj;
            if (q != 0u)
            {
                ndsR2AnimAObjToQ(current_aobj);
            }
        }
        current_aobj = current_aobj->next;
    }
}

#define NDS_R2_FTANIM_TRACKS()                                               \
    do {                                                                     \
        if (tracks_built == 0u)                                                  \
        {                                                                    \
            ndsR2AnimBuildTrackTable(root_dobj, track_aobjs,                 \
                                     (s32)ARRAY_COUNT(track_aobjs), q);      \
            tracks_built = 1u;                                                   \
        }                                                                    \
    } while (0)

#define NDS_R2_FTANIM_ENSURE()                                               \
    do {                                                                     \
        NDS_R2_FTANIM_TRACKS();                                      \
        if (track_aobjs[i] == NULL)                                          \
        {                                                                    \
            track_aobjs[i] =                                                 \
                gcAddAObjForDObj(root_dobj, i + nGCAnimTrackJointStart);     \
        }                                                                    \
        if (q != 0u)                                                         \
        {                                                                    \
            ndsR2AnimAObjToQ(track_aobjs[i]);                                \
        }                                                                    \
    } while (0)

void ndsR2FtAnimParseDObjFigatree(DObj *root_dobj)
{
    AObj *track_aobjs[nGCAnimTrackJointEnd - nGCAnimTrackJointStart + 1];
    AObj *current_aobj;
    f32 payload;
    u32 payload_u;
    s32 i;
    u16 command_kind;
    u16 flags;
    u32 events = 0;
    /* Requirement 4. Read once per call: with the route compiled out this folds
     * to 1 and every float arm below dead-codes away.
     *
     * Gated on NDS_R2_CUBIC_FIXED because that flag is what compiles the Q
     * PLAYER in. Without it `gcPlayDObjAnimJoint` is the decomp's own, whose
     * switch has no case for a Q kind -- so writing one here would silently
     * stop every joint animating, in exactly the configuration (bare `make`,
     * the P2 ROM) that nobody measures. */
#if NDS_R2_CUBIC_FIXED
    const u32 q = NDS_R2_ANIM_CUT_ON(8u) ? 1u : 0u;
#else
    const u32 q = 0u;
#endif
    const u8 kind_step = (u8)(q ? NDS_R2_AQ_KIND_STEP : nGCAnimKindStep);
    const u8 kind_linear = (u8)(q ? NDS_R2_AQ_KIND_LINEAR : nGCAnimKindLinear);
    const u8 kind_cubic = (u8)(q ? NDS_R2_AQ_KIND_CUBIC : nGCAnimKindCubic);
    f32 len_new = 0.0F;
    /* Route bit 16 defers the table build to first use; 0 keeps the eager walk,
     * so both arms live in one binary and `-SetGlobals` picks between them. */
    const u32 lazy_tracks = NDS_R2_ANIM_CUT_ON(16u) ? 1u : 0u;
    u32 tracks_built = 0u;

    gNdsR2FtAnimParseCalls++;

    if (NDS_FCMP_NE_C(root_dobj->anim_wait, AOBJ_ANIM_NULL))
    {
        if (NDS_FCMP_EQ_C(root_dobj->anim_wait, AOBJ_ANIM_CHANGED))
        {
            root_dobj->anim_wait = -root_dobj->anim_frame;
        }
        else
        {
            root_dobj->anim_wait -= root_dobj->anim_speed;
            root_dobj->anim_frame += root_dobj->anim_speed;
            root_dobj->parent_gobj->anim_frame = root_dobj->anim_frame;

            if (NDS_FCMP_GT0(root_dobj->anim_wait))
            {
                gNdsR2FtAnimParseEarlyOut++;
                return;
            }
        }
        gNdsR2FtAnimParseStepped++;
        /* Migrate the ones this parser owns, not the whole list: an AObj
         * outside the joint range belongs to another writer and its float kind
         * is the right one for it. Route bit 16 moves this to first use. */
        if (lazy_tracks == 0u)
        {
            NDS_R2_FTANIM_TRACKS();
        }
        do
        {
            if (root_dobj->anim_joint.event16 == NULL)
            {
                ndsR2AnimAdvanceTail(root_dobj, q);
                root_dobj->anim_frame = root_dobj->anim_wait;
                root_dobj->parent_gobj->anim_frame = root_dobj->anim_wait;
                root_dobj->anim_wait = AOBJ_ANIM_END;

                return;
            }
            command_kind = root_dobj->anim_joint.event16->command.opcode;

            switch (command_kind)
            {
            case nGCAnimEvent16SetVal0RateBlock:
            case nGCAnimEvent16SetVal0Rate:
                flags = root_dobj->anim_joint.event16->command.flags;
                NDS_R2_FTANIM_PAYLOAD();
                len_new = ndsR2AnimSegmentStart(root_dobj, q);

                for (i = 0; i < (s32)ARRAY_COUNT(track_aobjs);
                     i++, flags = flags >> 1)
                {
                    if (!(flags))
                    {
                        break;
                    }
                    if (flags & 1)
                    {
                        NDS_R2_FTANIM_ENSURE();
                        track_aobjs[i]->value_base =
                            track_aobjs[i]->value_target;
                        track_aobjs[i]->value_target = NDS_R2_FTANIM_TARGET(0);

                        track_aobjs[i]->rate_base = track_aobjs[i]->rate_target;
                        track_aobjs[i]->rate_target = 0.0F;

                        track_aobjs[i]->kind = kind_cubic;

                        if (payload_u != 0u)
                        {
                            track_aobjs[i]->length_invert =
                                ndsR2AnimRecipSlot(payload_u, q);
                        }
                        track_aobjs[i]->length = len_new;
                    }
                }
                if (command_kind == nGCAnimEvent16SetVal0RateBlock)
                {
                    root_dobj->anim_wait += payload;
                }
                break;

            case nGCAnimEvent16SetValBlock:
            case nGCAnimEvent16SetVal:
                flags = root_dobj->anim_joint.event16->command.flags;
                NDS_R2_FTANIM_PAYLOAD();

                for (i = 0; i < (s32)ARRAY_COUNT(track_aobjs);
                     i++, flags = flags >> 1)
                {
                    if (!(flags))
                    {
                        break;
                    }
                    if (flags & 1)
                    {
                        NDS_R2_FTANIM_ENSURE();
                        track_aobjs[i]->value_base =
                            track_aobjs[i]->value_target;
                        track_aobjs[i]->value_target = NDS_R2_FTANIM_TARGET(0);

                        track_aobjs[i]->kind = kind_linear;

                        if (payload_u != 0u)
                        {
                            /* The float arm's divide STAYS a divide: x*(1/n)
                             * rounds differently and this feeds a rate the
                             * cubic amplifies by `length`. The Q arm divides
                             * two Q12 integers, rounding the magnitude to
                             * nearest -- `__aeabi_idiv` where the float arm
                             * pays `__aeabi_fdiv`, the most expensive helper
                             * in the build at 109.4 cycles a call. */
                            if (q != 0u)
                            {
                                s32 d =
                                    (ndsR2AQLoad(track_aobjs[i]->value_target) -
                                     ndsR2AQLoad(track_aobjs[i]->value_base))
                                        << (NDS_R2_AQ_RF - NDS_R2_AQ_VF);
                                u32 h = payload_u >> 1;
                                s32 r = (d < 0) ?
                                    -(s32)(((u32)(-d) + h) / payload_u) :
                                    (s32)(((u32)d + h) / payload_u);

                                track_aobjs[i]->rate_base = ndsR2AQStore(r);
                            }
                            else
                            {
                                track_aobjs[i]->rate_base =
                                    (track_aobjs[i]->value_target -
                                     track_aobjs[i]->value_base) / payload;
                            }
                        }
                        track_aobjs[i]->length = len_new;
                        track_aobjs[i]->rate_target = 0.0F;
                    }
                }
                if (command_kind == nGCAnimEvent16SetValBlock)
                {
                    root_dobj->anim_wait += payload;
                }
                break;

            case nGCAnimEvent16SetValRateBlock:
            case nGCAnimEvent16SetValRate:
                flags = root_dobj->anim_joint.event16->command.flags;
                NDS_R2_FTANIM_PAYLOAD();

                for (i = 0; i < (s32)ARRAY_COUNT(track_aobjs);
                     i++, flags = flags >> 1)
                {
                    if (!(flags))
                    {
                        break;
                    }
                    if (flags & 1)
                    {
                        NDS_R2_FTANIM_ENSURE();
                        track_aobjs[i]->value_base =
                            track_aobjs[i]->value_target;
                        track_aobjs[i]->value_target = NDS_R2_FTANIM_TARGET(0);

                        track_aobjs[i]->rate_base = track_aobjs[i]->rate_target;
                        track_aobjs[i]->rate_target = NDS_R2_FTANIM_TARGET(1);

                        track_aobjs[i]->kind = kind_cubic;

                        if (payload_u != 0u)
                        {
                            track_aobjs[i]->length_invert =
                                ndsR2AnimRecipSlot(payload_u, q);
                        }
                        track_aobjs[i]->length = len_new;
                    }
                }
                if (command_kind == nGCAnimEvent16SetValRateBlock)
                {
                    root_dobj->anim_wait += payload;
                }
                break;

            case nGCAnimEvent16SetTargetRate:
                flags = root_dobj->anim_joint.event16->command.flags;

                NDS_R2_FTANIM_PAYLOAD();

                for (i = 0; i < (s32)ARRAY_COUNT(track_aobjs);
                     i++, flags = flags >> 1)
                {
                    if (!(flags))
                    {
                        break;
                    }
                    if (flags & 1)
                    {
                        NDS_R2_FTANIM_ENSURE();
                        track_aobjs[i]->rate_target = NDS_R2_FTANIM_TARGET(1);
                    }
                }
                break;

            case nGCAnimEvent16Block:
                if (AObjAnimAdvance(
                        root_dobj->anim_joint.event16)->command.toggle)
                {
                    root_dobj->anim_wait += ndsR2U16ToF32(
                        AObjAnimAdvance(root_dobj->anim_joint.event16)->u);
                }
                break;

            case nGCAnimEvent16SetValAfterBlock:
            case nGCAnimEvent16SetValAfter:
                flags = root_dobj->anim_joint.event16->command.flags;
                NDS_R2_FTANIM_PAYLOAD();

                for (i = 0; i < (s32)ARRAY_COUNT(track_aobjs);
                     i++, flags = flags >> 1)
                {
                    if (!(flags))
                    {
                        break;
                    }
                    if (flags & 1)
                    {
                        NDS_R2_FTANIM_ENSURE();
                        track_aobjs[i]->value_base =
                            track_aobjs[i]->value_target;
                        track_aobjs[i]->value_target = NDS_R2_FTANIM_TARGET(0);

                        track_aobjs[i]->kind = kind_step;

                        /* Step's `length_invert` holds a FRAME COUNT, not a
                         * reciprocal -- the original's own double meaning for
                         * the field, so the Q form keeps it in `length`'s
                         * scale and the compare stays a compare. */
                        track_aobjs[i]->length_invert =
                            ndsR2AnimFramesSlot(payload_u, payload, q);
                        track_aobjs[i]->length = len_new;

                        track_aobjs[i]->rate_target = 0.0F;
                    }
                }
                if (command_kind == nGCAnimEvent16SetValAfterBlock)
                {
                    root_dobj->anim_wait += payload;
                }
                break;

            case nGCAnimEvent16Loop:
                AObjAnimAdvance(root_dobj->anim_joint.event16);

                root_dobj->anim_joint.event16 +=
                    root_dobj->anim_joint.event16->s / 2;

                root_dobj->anim_frame = -root_dobj->anim_wait;
                root_dobj->parent_gobj->anim_frame = -root_dobj->anim_wait;

                if (root_dobj->is_anim_root != FALSE)
                {
                    if (root_dobj->parent_gobj->func_anim != NULL)
                    {
                        root_dobj->parent_gobj->func_anim(root_dobj, -2, 0);
                    }
                }
                break;

            case nGCAnimEvent1611:
                flags = root_dobj->anim_joint.event16->command.flags;

                NDS_R2_FTANIM_PAYLOAD();

                for (i = 0; i < (s32)ARRAY_COUNT(track_aobjs);
                     i++, flags = flags >> 1)
                {
                    if (!(flags))
                    {
                        break;
                    }
                    if (flags & 1)
                    {
                        NDS_R2_FTANIM_ENSURE();
                        ndsR2AnimAddLength(track_aobjs[i], payload_u, payload,
                                           q);
                    }
                }
                break;

            case nGCAnimEvent16SetTranslateInterp:
                /* The one block that reads the table without ENSURE, per the
                 * audit and per the comment further down. */
                NDS_R2_FTANIM_TRACKS();
                AObjAnimAdvance(root_dobj->anim_joint.event16);

                if (track_aobjs[nGCAnimTrackTraI - nGCAnimTrackJointStart] ==
                    NULL)
                {
                    track_aobjs[nGCAnimTrackTraI - nGCAnimTrackJointStart] =
                        gcAddAObjForDObj(root_dobj, nGCAnimTrackTraI);
                    /* The only creation site outside NDS_R2_FTANIM_ENSURE. */
                    if (q != 0u)
                    {
                        ndsR2AnimAObjToQ(
                            track_aobjs[nGCAnimTrackTraI -
                                nGCAnimTrackJointStart]);
                    }
                }
                track_aobjs[nGCAnimTrackTraI -
                    nGCAnimTrackJointStart]->interpolate =
                        root_dobj->anim_joint.event16 +
                            (root_dobj->anim_joint.event16->s / 2);

                AObjAnimAdvance(root_dobj->anim_joint.event16);
                break;

            case nGCAnimEvent16End:
                ndsR2AnimAdvanceTail(root_dobj, q);
                root_dobj->anim_frame = root_dobj->anim_wait;
                root_dobj->parent_gobj->anim_frame = root_dobj->anim_wait;
                root_dobj->anim_wait = AOBJ_ANIM_END;

                if (root_dobj->is_anim_root != FALSE)
                {
                    if (root_dobj->parent_gobj->func_anim != NULL)
                    {
                        root_dobj->parent_gobj->func_anim(root_dobj, -1, 0);
                    }
                }
                return;

            case nGCAnimEvent16SetFlags:
                root_dobj->flags =
                    root_dobj->anim_joint.event16->command.flags;

                if (AObjAnimAdvance(
                        root_dobj->anim_joint.event16)->command.toggle)
                {
                    root_dobj->anim_wait += ndsR2U16ToF32(
                        AObjAnimAdvance(root_dobj->anim_joint.event16)->u);
                }
                break;

            default:
                /* Same recorded fault as the patch this body replaces: an
                 * unknown opcode is a runaway script, not something to spin on.
                 * Mask bits 6 and 7 are this parser's. */
                gNdsObjAnimRunawayCount++;
                gNdsObjAnimRunawayMask |= 1u << 6u;
                gNdsObjAnimRunawayScript =
                    (u32)(uintptr_t)root_dobj->anim_joint.event16;
                gNdsObjAnimRunawayOpcode = command_kind;
                root_dobj->anim_wait = AOBJ_ANIM_NULL;
                return;
            }
            if (++events >= NDS_FTANIM_EVENT_LIMIT)
            {
                gNdsObjAnimRunawayCount++;
                gNdsObjAnimRunawayMask |= 1u << 7u;
                gNdsObjAnimRunawayScript =
                    (u32)(uintptr_t)root_dobj->anim_joint.event16;
                gNdsObjAnimRunawayOpcode = command_kind;
                root_dobj->anim_wait = AOBJ_ANIM_NULL;
                return;
            }
        }
        while (NDS_FCMP_LE0(root_dobj->anim_wait));
    }
}
