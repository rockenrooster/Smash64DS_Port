/* Relocation asset loading, the normalizers: AObj16 scripts, fighter attributes,
 * ground maps, MObjSubs and the other mixed-width records the blanket u32 swap
 * gets wrong. A slice of reloc_backend.c. */

/* Task 85, same reasoning as ndsRelocReadNative32 above. */
static u16 ndsRelocReadNative16(const void *addr)
{
    u16 value;

    if ((((uintptr_t)addr) & 1u) == 0u)
    {
        return *(const u16 *)addr;
    }
    memcpy(&value, addr, sizeof(value));
    return value;
}

static void ndsRelocWriteNative16(void *addr, u16 value)
{
    if ((((uintptr_t)addr) & 1u) == 0u)
    {
        *(u16 *)addr = value;
        return;
    }
    memcpy(addr, &value, sizeof(value));
}

static u16 ndsRelocAObj16EncodeForNativeBitfields(u16 source)
{
    u16 opcode = (u16)((source >> 11) & 0x1fu);
    u16 flags = (u16)((source >> 1) & 0x3ffu);
    u16 toggle = (u16)(source & 1u);

    return (u16)(opcode | (flags << 5) | (toggle << 15));
}

static u32 ndsRelocAObj16FlagCount(u16 flags)
{
    u32 count = 0;

    while (flags != 0u)
    {
        if ((flags & 1u) != 0u)
        {
            count++;
        }
        flags = (u16)(flags >> 1);
    }
    return count;
}

static u32 ndsRelocAObj16CommandWords(u16 opcode, u16 flags, u16 toggle)
{
    u32 words = 1u;
    u32 flagged = ndsRelocAObj16FlagCount(flags);

    if (toggle != 0u)
    {
        words++;
    }

    switch (opcode)
    {
    case nGCAnimEvent16SetValBlock:
    case nGCAnimEvent16SetVal:
    case nGCAnimEvent16SetVal0RateBlock:
    case nGCAnimEvent16SetVal0Rate:
    case nGCAnimEvent16SetValAfterBlock:
    case nGCAnimEvent16SetValAfter:
        words += flagged;
        break;

    case nGCAnimEvent16SetValRateBlock:
    case nGCAnimEvent16SetValRate:
        words += flagged * 2u;
        break;

    case nGCAnimEvent16SetTargetRate:
        words += flagged;
        break;

    case nGCAnimEvent16Loop:
    case nGCAnimEvent16SetTranslateInterp:
        words = 2u;
        break;

    default:
        break;
    }
    return words;
}

/* The spline figatrees' SYInterpDesc header word, source-derived.
 *
 * On disk (big endian) word 0 of the descriptor is {u8 kind; u8 pad;
 * s16 points_num}, i.e. bytes [kind, pad, pn_hi, pn_lo]. The common word
 * byte-swap that is correct for every full-width field -- the f32 unk04 and
 * length and the three pointers the internal fixups patch -- reverses word 0
 * to [pn_lo, pn_hi, pad, kind]. The ARM LE struct the port compiles from the
 * ORIGINAL decomp header (src/import/battleship_sys_interp.c includes
 * decomp BattleShip src/sys/interp.c, whose SYInterpDesc is {u8 kind;
 * s16 points_num; ...}) then reads kind = pn_lo and points_num =
 * (pad << 8) | kind: Samus RollB evaluates as kind 5 / points_num 0x0200,
 * a kind no switch in interp.c names, so syInterpGetFracFrame returns an
 * UNINITIALIZED frac_frame and the roll translates on garbage. This is the
 * 2026-09-06 four-CPU frame-447 crash family: the desc is the one word in
 * the interpolation block that is neither f32 nor pointer, and a blanket
 * u32 swap cannot be right for both widths.
 *
 * Native layout the struct needs: [kind, pad, pn_lo, pn_hi], i.e.
 * kind | (pad << 8) | (pn_lo << 16) | (pn_hi << 24) from the swapped word.
 * Byte-for-byte invertible, applied exactly once under
 * format_fixups_applied. The transform rides pipeline 4c's exactly-once
 * precondition: like the command re-encode it is not idempotent, and the
 * corpus invariant that no two walked scripts share bytes (enforced at pack
 * time by the generator's table-slot check) is what guarantees one TraI pass
 * per descriptor. The bank's only TraI bearers are FTSamusAnim060/061, one
 * command and one desc each. Shared with the event32 stage normalizer
 * (src/import/battleship_sys_objanim.c), which fixes the Sector Z asset 0x99
 * TraI descriptors under its own ledger; the transform itself is unchanged. */
u32 ndsRelocSYInterpDescHeaderNative(u32 swapped)
{
    return ((swapped >> 24) |
            (((swapped >> 16) & 0xffu) << 8) |
            ((swapped & 0xffu) << 16) |
            (((swapped >> 8) & 0xffu) << 24));
}

/* TraI references whose descriptor did not land inside the interpolation
 * block [table_bytes, script_bytes). Never a silent accept: a refusal leaves
 * every payload byte exactly as the common passes produced it and the counters
 * name the asset, because a layout this function does not understand is a
 * question, not a heuristic patch. The header lane swap and the command
 * re-encode are both non-idempotent, so a partial write would make a RAW cache
 * entry non-raw and the next pass would normalize twice. */
__attribute__((used)) volatile u32 gNdsRelocSYInterpDescFixCount;
__attribute__((used)) volatile u32 gNdsRelocSYInterpDescUnresolvedCount;
__attribute__((used)) volatile u32 gNdsRelocSYInterpDescUnresolvedFirstAsset;

/* Read-only mirror of the script walk below. Returns the same unresolved count
 * the mutating pass would produce, plus one for a step that does not tile
 * (which the mutating pass used to truncate silently after already writing a
 * prefix). A descriptor named twice in one script is also a refusal: the
 * header fix is not idempotent, so fixing twice corrupts and refusing keeps
 * the payload pristine. Writes nothing, touches no counters. */
static u32 ndsRelocValidateAObj16Script(const u16 *script, u32 word_count,
                                        const u8 *data, u32 interp_start,
                                        u32 interp_end)
{
    u32 index = 0;
    u32 guard = word_count;
    u32 unresolved = 0u;

    while ((index < word_count) && (guard != 0u))
    {
        u16 source = script[index];
        u16 opcode = (u16)((source >> 11) & 0x1fu);
        u16 flags = (u16)((source >> 1) & 0x3ffu);
        u16 toggle = (u16)(source & 1u);
        u32 step = ndsRelocAObj16CommandWords(opcode, flags, toggle);

        if (opcode == nGCAnimEvent16End)
        {
            return unresolved;
        }
        if (opcode == nGCAnimEvent16SetTranslateInterp)
        {
            if ((index + 1u) < word_count)
            {
                const u8 *payload = (const u8 *)&script[index + 1u];
                s32 rel = (s32)(s16)script[index + 1u];
                u32 desc_off = (u32)((uintptr_t)payload - (uintptr_t)data) +
                               ((u32)(rel / 2) * 2u);

                if ((desc_off >= interp_start) &&
                    (desc_off <= interp_end) &&
                    (24u <= (interp_end - desc_off)))
                {
                    u32 j = 0u;
                    u32 jguard = word_count;

                    while ((j < index) && (jguard != 0u))
                    {
                        u16 jsource = script[j];
                        u16 jop = (u16)((jsource >> 11) & 0x1fu);
                        u16 jflags = (u16)((jsource >> 1) & 0x3ffu);
                        u16 jtoggle = (u16)(jsource & 1u);
                        u32 jstep =
                            ndsRelocAObj16CommandWords(jop, jflags, jtoggle);

                        if (jop == nGCAnimEvent16End)
                        {
                            break;
                        }
                        if ((jop == nGCAnimEvent16SetTranslateInterp) &&
                            ((j + 1u) < word_count))
                        {
                            const u8 *jpayload =
                                (const u8 *)&script[j + 1u];
                            s32 jrel = (s32)(s16)script[j + 1u];
                            u32 jdesc =
                                (u32)((uintptr_t)jpayload -
                                      (uintptr_t)data) +
                                ((u32)(jrel / 2) * 2u);

                            if (jdesc == desc_off)
                            {
                                unresolved++;
                                break;
                            }
                        }
                        if ((jstep == 0u) || ((j + jstep) > word_count))
                        {
                            break;
                        }
                        j += jstep;
                        jguard--;
                    }
                }
                else
                {
                    unresolved++;
                }
            }
        }
        if ((step == 0u) || ((index + step) > word_count))
        {
            return unresolved + 1u;
        }
        index += step;
        guard--;
    }
    return unresolved;
}

/* Returns the number of SetTranslateInterp commands whose descriptor did not
 * land in [interp_start, interp_end); 0 when every desc was fixed. Atomic: the
 * walk is validated read-only first, so a non-zero return leaves every script
 * and descriptor byte untouched. */
static u32 ndsRelocNormalizeAObj16Script(u16 *script, u32 word_count,
                                         u8 *data, u32 interp_start,
                                         u32 interp_end)
{
    u32 index = 0;
    u32 guard = word_count;
    u32 unresolved = 0u;
    u32 valid;

    valid = ndsRelocValidateAObj16Script(script, word_count, data,
                                         interp_start, interp_end);
    if (valid != 0u)
    {
        return valid;
    }

    while ((index < word_count) && (guard != 0u))
    {
        u16 source = script[index];
        u16 opcode = (u16)((source >> 11) & 0x1fu);
        u16 flags = (u16)((source >> 1) & 0x3ffu);
        u16 toggle = (u16)(source & 1u);
        u32 step = ndsRelocAObj16CommandWords(opcode, flags, toggle);

        script[index] = ndsRelocAObj16EncodeForNativeBitfields(source);
        if (opcode == nGCAnimEvent16End)
        {
            return unresolved;
        }
        if (opcode == nGCAnimEvent16SetTranslateInterp)
        {
            /* The s16 one word after the command is the offset every parser
             * of this format adds to its own address (`interpolate =
             * event16 + (event16->s / 2)`, sys/objanim.c and the pose
             * engine alike), so the walk that already re-encodes the command
             * is also the one place that knows where the descriptor lives. */
            if ((index + 1u) < word_count)
            {
                const u8 *payload = (const u8 *)&script[index + 1u];
                s32 rel = (s32)(s16)script[index + 1u];
                u32 desc_off = (u32)((uintptr_t)payload - (uintptr_t)data) +
                               ((u32)(rel / 2) * 2u);

                if ((desc_off >= interp_start) &&
                    (desc_off <= interp_end) &&
                    (24u <= (interp_end - desc_off)))
                {
                    ndsRelocWriteNative32(
                        data + desc_off,
                        ndsRelocSYInterpDescHeaderNative(
                            ndsRelocReadNative32(data + desc_off)));
                    gNdsRelocSYInterpDescFixCount++;
                }
                else
                {
                    unresolved++;
                }
            }
        }
        if ((step == 0u) || ((index + step) > word_count))
        {
            return unresolved;
        }
        index += step;
        guard--;
    }
    return unresolved;
}


static s32 ndsRelocNormalizeFighterAObj16File(NDSRelocLoadedFile *loaded)
{
    uintptr_t base;
    uintptr_t table_bytes;
    uintptr_t script_bytes;
    u32 i;

    if ((loaded == NULL) || (loaded->data == NULL) ||
        (ndsRelocIsFighterAObj16Asset(loaded->asset_id) == FALSE))
    {
        return TRUE;
    }
    if (loaded->format_fixups_applied != FALSE)
    {
        return TRUE;
    }
    /* K0 line 5, "AObj16 file normalization". Marked past the two early
     * returns, so this counts normalization that actually walks the payload --
     * a prebaked cache entry that claims the transform is correctly not a
     * normalization. */
    NDS_K0_MARK(gNdsK0AfterGoNormalizes, loaded->asset_id);

    base = (uintptr_t)loaded->data;
    table_bytes = 0u;
    script_bytes = loaded->data_size;
    /* Fighter AObj16 files begin with the joint-script pointer array.  Stop at
     * its first non-pointer/non-NULL word instead of searching the entire
     * prefix for the smallest pointer.
     *
     * Samus RollB (1014_FTSamusAnimRollB.c) is the case that makes this a
     * correctness rule rather than a nicer parser.  Its 0x00..0x5f top-level
     * pointer table is followed by interpolation keyframes + an SYInterpDesc
     * and a SECOND pointer array at 0x120; the actual AObj16 joint scripts do
     * not begin until 0x130.  The old minimum-pointer scan wandered through the
     * interpolation block, found that second array's pointer back to 0x60, and
     * then lane-swapped every u32 from 0x60 onward as though it were AObj16.
     * That half-swapped already-relocated pointers (0x022ed664 -> 0xd664022e)
     * and crashed syInterpGetFracFrame when RollB evaluated TraI.
     *
     * Keep the two boundaries semantically distinct: table_bytes is the
     * contiguous source pointer array, while script_bytes is the earliest joint
     * script it names.  They may be equal when the first script follows the
     * pointer table immediately (1099_FTSamusAnimBomb is the source example).
     * When script_bytes is greater, the bytes between them are 32-bit
     * interpolation data and keep the common word-byte-swap/internal-reloc
     * form -- with ONE exception, applied by the script walk below: word 0 of
     * each SYInterpDesc the scripts' SetTranslateInterp commands name is the
     * mixed u8/s16 header `ndsRelocSYInterpDescHeaderNative` exists for. */
    for (i = 0; (((i + 1u) * sizeof(u32)) <= loaded->data_size); i++)
    {
        uintptr_t value =
            (uintptr_t)ndsRelocReadNative32((u8 *)loaded->data +
                                            (i * sizeof(u32)));

        if (value == 0u)
        {
            table_bytes += sizeof(u32);
            continue;
        }
        if ((value >= base) && ((value - base) < loaded->data_size))
        {
            uintptr_t offset = value - base;

            table_bytes += sizeof(u32);
            if (offset < script_bytes)
            {
                script_bytes = offset;
            }
            continue;
        }
        break;
    }
    if ((table_bytes == 0u) || (table_bytes >= loaded->data_size) ||
        (script_bytes < table_bytes) ||
        (script_bytes >= loaded->data_size) ||
        ((table_bytes % sizeof(u32)) != 0u) ||
        ((script_bytes % sizeof(u32)) != 0u))
    {
        ndsRelocRecordExternalFixupFail(loaded->asset_id);
        return FALSE;
    }

    for (i = (u32)script_bytes; (i + sizeof(u32)) <= loaded->data_size;
         i += sizeof(u32))
    {
        u16 first = ndsRelocReadNative16((u8 *)loaded->data + i);
        u16 second = ndsRelocReadNative16((u8 *)loaded->data + i + sizeof(u16));

        ndsRelocWriteNative16((u8 *)loaded->data + i, second);
        ndsRelocWriteNative16((u8 *)loaded->data + i + sizeof(u16), first);
    }

    /* Atomic refusal across the whole file. The lane swap above is reversible
     * (it is its own inverse) and the script validator is read-only, so a bad
     * bound, a non-tiling step, or a repeated descriptor in ANY script restores
     * the pre-AObj16 script bytes and returns FALSE with no command or header
     * word written. That keeps a declined prebake payload pristine, so the RAW
     * entry the cache stores normalizes exactly once later instead of twice.
     * No extra buffer: two bounded walks, the second recomputing the same
     * successor bounds. */
    {
        u32 total_unresolved = 0u;

        for (i = 0; (i * sizeof(u32)) < table_bytes; i++)
        {
            uintptr_t value =
                (uintptr_t)ndsRelocReadNative32((u8 *)loaded->data +
                                                 (i * sizeof(u32)));

            if ((value >= (base + table_bytes)) &&
                ((value - base) < (uintptr_t)loaded->data_size))
            {
                u32 script_offset = (u32)(value - base);
                uintptr_t script_end = base + loaded->data_size;
                u32 j;
                u32 word_count;

                for (j = 0; (j * sizeof(u32)) < table_bytes; j++)
                {
                    uintptr_t next =
                        (uintptr_t)ndsRelocReadNative32((u8 *)loaded->data +
                                                         (j * sizeof(u32)));

                    if ((next > value) && (next < script_end))
                    {
                        script_end = next;
                    }
                }
                word_count = (u32)((script_end - value) / sizeof(u16));
                total_unresolved += ndsRelocValidateAObj16Script(
                    (const u16 *)((const u8 *)loaded->data + script_offset),
                    word_count, (const u8 *)loaded->data, (u32)table_bytes,
                    (u32)script_bytes);
            }
        }
        if (total_unresolved != 0u)
        {
            u32 k;

            for (k = (u32)script_bytes;
                 (k + sizeof(u32)) <= loaded->data_size;
                 k += sizeof(u32))
            {
                u16 first = ndsRelocReadNative16((u8 *)loaded->data + k);
                u16 second = ndsRelocReadNative16((u8 *)loaded->data + k +
                                                  sizeof(u16));

                ndsRelocWriteNative16((u8 *)loaded->data + k, second);
                ndsRelocWriteNative16((u8 *)loaded->data + k + sizeof(u16),
                                      first);
            }
            gNdsRelocSYInterpDescUnresolvedCount += total_unresolved;
            if (gNdsRelocSYInterpDescUnresolvedFirstAsset == 0u)
            {
                gNdsRelocSYInterpDescUnresolvedFirstAsset = loaded->asset_id;
                DC_FlushAll();
            }
            return FALSE;
        }
    }

    for (i = 0; (i * sizeof(u32)) < table_bytes; i++)
    {
        uintptr_t value =
            (uintptr_t)ndsRelocReadNative32((u8 *)loaded->data +
                                             (i * sizeof(u32)));

        if ((value >= (base + table_bytes)) &&
            ((value - base) < (uintptr_t)loaded->data_size))
        {
            u32 script_offset = (u32)(value - base);
            uintptr_t script_end = base + loaded->data_size;
            u32 j;
            u32 word_count;
            u32 unresolved;

            for (j = 0; (j * sizeof(u32)) < table_bytes; j++)
            {
                uintptr_t next =
                    (uintptr_t)ndsRelocReadNative32((u8 *)loaded->data +
                                                     (j * sizeof(u32)));

                if ((next > value) && (next < script_end))
                {
                    script_end = next;
                }
            }
            word_count = (u32)((script_end - value) / sizeof(u16));

            unresolved = ndsRelocNormalizeAObj16Script(
                (u16 *)((u8 *)loaded->data + script_offset),
                word_count, (u8 *)loaded->data, (u32)table_bytes,
                (u32)script_bytes);
            if (unresolved != 0u)
            {
                gNdsRelocSYInterpDescUnresolvedCount += unresolved;
                if (gNdsRelocSYInterpDescUnresolvedFirstAsset == 0u)
                {
                    gNdsRelocSYInterpDescUnresolvedFirstAsset =
                        loaded->asset_id;
                    DC_FlushAll();
                }
            }
        }
    }

    loaded->format_fixups_applied = TRUE;
    return TRUE;
}

/* The O2R fighter files preserve the N64's big-endian mixed-width layout.
 * The common relocation pass first byte-swaps each u32, which is correct for
 * floats, pointers, and integers but reverses the two u16 lanes within these
 * exact FTAttributes words.  BattleShip reads all six words directly. */
_Static_assert(offsetof(FTAttributes, dead_fgm_ids) == 0xb4u,
               "FTAttributes dead FGM layout changed");
_Static_assert(offsetof(FTAttributes, deadup_sfx) == 0xb8u,
               "FTAttributes dead-up FGM layout changed");
_Static_assert(offsetof(FTAttributes, damage_sfx) == 0xbau,
               "FTAttributes damage FGM layout changed");
_Static_assert(offsetof(FTAttributes, smash_sfx) == 0xbcu,
               "FTAttributes smash FGM layout changed");
_Static_assert(offsetof(FTAttributes, item_pickup) == 0xc4u,
               "FTAttributes smash padding layout changed");
_Static_assert(offsetof(FTAttributes, itemthrow_vel_scale) == 0xe4u,
               "FTAttributes item-throw layout changed");
_Static_assert(offsetof(FTAttributes, itemthrow_damage_scale) == 0xe6u,
               "FTAttributes item-throw damage layout changed");
_Static_assert(offsetof(FTAttributes, heavyget_sfx) == 0xe8u,
               "FTAttributes heavy-get FGM layout changed");
_Static_assert(offsetof(FTAttributes, halo_size) == 0xecu,
               "FTAttributes heavy-get padding layout changed");

static void ndsRelocSwapNativeU16WordLanes(void *word)
{
    u8 *bytes = word;
    u16 first = ndsRelocReadNative16(bytes);
    u16 second = ndsRelocReadNative16(bytes + sizeof(u16));

    ndsRelocWriteNative16(bytes, second);
    ndsRelocWriteNative16(bytes + sizeof(u16), first);
}

/* KirbyMainMotion begins with BattleShip's FTKirbyCopy[27] table
 * (relocData/228_KirbyMainMotion.c:132-160).  Its first word is the mixed-width
 * pair { u16 copy_id; s16 copy_modelpart_id }.  The common O2R u32 byte swap is
 * correct for the following f32/s32 words but reverses those two 16-bit lanes:
 * the source no-copy Kirby row { nFTKindKirby, 0, ... } consequently arrived as
 * { 0, 8, ... }, and ftManagerMakeFighter installed modelpart 8 on joint 6.
 * That is the native-render failure seen naturally at Kirby AppearL.
 *
 * Validate every transformed row against the source table BEFORE mutating any
 * byte, then swap exactly the one mixed word per row.  Full-width effect_scale
 * and star_damage stay in the common word-swapped form and are deliberately
 * untouched.  This asset is not an AObj16 bank or an FTAttributes file, so its
 * format_fixups_applied bit belongs exclusively to this exactly-once pass. */
_Static_assert(sizeof(FTKirbyCopy) == 12u,
               "FTKirbyCopy source row size changed");
_Static_assert(offsetof(FTKirbyCopy, copy_id) == 0u,
               "FTKirbyCopy copy_id layout changed");
_Static_assert(offsetof(FTKirbyCopy, copy_modelpart_id) == 2u,
               "FTKirbyCopy modelpart layout changed");
_Static_assert(offsetof(FTKirbyCopy, effect_scale) == 4u,
               "FTKirbyCopy effect scale layout changed");
_Static_assert(offsetof(FTKirbyCopy, star_damage) == 8u,
               "FTKirbyCopy star damage layout changed");

static s32 ndsRelocNormalizeKirbyMainMotionCopyTable(
    NDSRelocLoadedFile *loaded)
{
    static const u8 source_copy_id[27] = {
        nFTKindMario, nFTKindFox, nFTKindDonkey, nFTKindSamus,
        nFTKindLuigi, nFTKindLink, nFTKindYoshi, nFTKindCaptain,
        nFTKindKirby, nFTKindPikachu, nFTKindPurin, nFTKindNess,
        nFTKindKirby, nFTKindKirby, nFTKindKirby, nFTKindKirby,
        nFTKindKirby, nFTKindKirby, nFTKindKirby, nFTKindKirby,
        nFTKindKirby, nFTKindKirby, nFTKindKirby, nFTKindKirby,
        nFTKindKirby, nFTKindKirby, nFTKindDonkey
    };
    static const s8 source_modelpart_id[27] = {
        12, 7, 4, 8, 11, 10, 5, 9, 0, 6, 3, 13,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4
    };
    u32 i;

    if ((loaded == NULL) || (loaded->data == NULL) ||
        (loaded->asset_id != NDS_RELOC_ASSET_KIRBY_MAIN_MOTION))
    {
        return TRUE;
    }
    if (loaded->format_fixups_applied != FALSE)
    {
        return TRUE;
    }
    if (ndsRelocRangeInLoadedFile(
            loaded, 0u, ARRAY_COUNT(source_copy_id) * sizeof(FTKirbyCopy)) ==
        FALSE)
    {
        ndsRelocRecordExternalFixupFail(loaded->asset_id);
        return FALSE;
    }

    /* Preflight the post-swap values so a source/layout mismatch writes no
     * partial table.  Before repair, lane 0 contains source modelpart and lane
     * 1 contains source copy kind. */
    for (i = 0u; i < ARRAY_COUNT(source_copy_id); i++)
    {
        const u8 *row = (const u8 *)loaded->data +
                        (i * sizeof(FTKirbyCopy));
        u16 lane0 = ndsRelocReadNative16(
            row + offsetof(FTKirbyCopy, copy_id));
        u16 lane1 = ndsRelocReadNative16(
            row + offsetof(FTKirbyCopy, copy_modelpart_id));

        if ((lane1 != (u16)source_copy_id[i]) ||
            ((s16)lane0 != (s16)source_modelpart_id[i]))
        {
            ndsRelocRecordExternalFixupFail(loaded->asset_id);
            return FALSE;
        }
    }
    for (i = 0u; i < ARRAY_COUNT(source_copy_id); i++)
    {
        u8 *row = (u8 *)loaded->data + (i * sizeof(FTKirbyCopy));

        ndsRelocSwapNativeU16WordLanes(
            row + offsetof(FTKirbyCopy, copy_id));
    }
    loaded->format_fixups_applied = TRUE;
    return TRUE;
}

static s32 ndsRelocFighterAttributesMatchSource(
    u32 asset_id, const FTAttributes *attr)
{

    if (attr == NULL)
    {
        return FALSE;
    }
    if (asset_id == NDS_RELOC_ASSET_MARIO_MAIN)
    {
        return
            (attr->dead_fgm_ids[0] == nSYAudioVoiceMarioDead) &&
            (attr->dead_fgm_ids[1] == nSYAudioFGMMarioDeadSlam) &&
            (attr->deadup_sfx == nSYAudioVoiceMarioDeadUp) &&
            (attr->damage_sfx == nSYAudioVoiceMarioDamage) &&
            (attr->smash_sfx[0] == nSYAudioVoiceMarioSmash1) &&
            (attr->smash_sfx[1] == nSYAudioVoiceMarioSmash2) &&
            (attr->smash_sfx[2] == nSYAudioVoiceMarioSmash3) &&
            (attr->itemthrow_vel_scale == 0x64u) &&
            (attr->itemthrow_damage_scale == 0x64u) &&
            (attr->heavyget_sfx == nSYAudioVoiceMarioHeavyGet);
    }
    if (asset_id == NDS_RELOC_ASSET_FOX_MAIN)
    {
        return
            (attr->dead_fgm_ids[0] == nSYAudioVoiceFoxDead) &&
            (attr->dead_fgm_ids[1] == nSYAudioFGMFoxDeadSlam) &&
            (attr->deadup_sfx == nSYAudioVoiceFoxDeadUp) &&
            (attr->damage_sfx == nSYAudioVoiceFoxDamage) &&
            (attr->smash_sfx[0] == nSYAudioVoiceFoxSmash3) &&
            (attr->smash_sfx[1] == nSYAudioVoiceFoxSmash1) &&
            (attr->smash_sfx[2] == nSYAudioVoiceFoxSmash2) &&
            (attr->itemthrow_vel_scale == 0x64u) &&
            (attr->itemthrow_damage_scale == 0x64u) &&
            (attr->heavyget_sfx == nSYAudioFGMVoiceEnd);
    }
#if NDS_P2_LUIGI
    if (asset_id == NDS_RELOC_ASSET_LUIGI_MAIN)
    {
        /* 221_LuigiMain.c:331-338, ordinals from gmsound.h's REGION_US arm
         * (`nSYAudioVoiceLuigiDead` 427, DeadUp 420, Damage 422,
         * Smash1..3 416/417/418, HeavyGet 426).  Numeric for the same reason
         * Donkey's and Captain's are.
         *
         * NOTE Luigi's dead-slam is MARIO's -- the source spells
         * dead_fgm_ids[1] `nSYAudioFGMMarioDeadSlam` (292), not a Luigi id, so
         * a fighter-name substitution would have been wrong here even though
         * his smash triple IS the ordinary Smash1..3 that Captain's is not. */
        return
            (attr->dead_fgm_ids[0] == 427u) &&
            (attr->dead_fgm_ids[1] == (u16)nSYAudioFGMMarioDeadSlam) &&
            (attr->deadup_sfx == 420u) &&
            (attr->damage_sfx == 422u) &&
            (attr->smash_sfx[0] == 416u) &&
            (attr->smash_sfx[1] == 417u) &&
            (attr->smash_sfx[2] == 418u) &&
            (attr->itemthrow_vel_scale == 0x64u) &&
            (attr->itemthrow_damage_scale == 0x64u) &&
            (attr->heavyget_sfx == 426u);
    }
#endif
#if NDS_P2_DONKEY
    if (asset_id == NDS_RELOC_ASSET_DONKEY_MAIN)
    {
        /* BattleShip US gmsound.h: Donkey dead-slam is 0x11f and the voice
         * run Smash1..Dead2 is 326..336.  Keep the validation numeric here:
         * the port's compact gmsound shadow intentionally names only sounds
         * that production code calls directly, while these are source data
         * values used to prove the mixed-u16 FTAttributes lanes were restored. */
        return
            (attr->dead_fgm_ids[0] == 336u) &&
            (attr->dead_fgm_ids[1] == 0x11fu) &&
            (attr->deadup_sfx == 330u) &&
            (attr->damage_sfx == 332u) &&
            (attr->smash_sfx[0] == 326u) &&
            (attr->smash_sfx[1] == 327u) &&
            (attr->smash_sfx[2] == 328u) &&
            (attr->itemthrow_vel_scale == 0x64u) &&
            (attr->itemthrow_damage_scale == 0x64u) &&
            (attr->heavyget_sfx == 334u);
    }
#endif
#if NDS_P2_CAPTAIN
    if (asset_id == NDS_RELOC_ASSET_CAPTAIN_MAIN)
    {
        /* 236_CaptainMain.c:330-337 names these; the ordinals are gmsound.h's
         * REGION_US arm (`nSYAudioVoiceCaptainDead` 355,
         * `nSYAudioFGMCaptainDeadSlam` 0x120, DeadUp 349, Damage 351,
         * Smash3/Smash2/JumpAerial 341/340/353, HeavyGet 354). Numeric for the
         * same reason Donkey's are: the port's gmsound shadow deliberately
         * names only sounds production code calls directly, while these are
         * source DATA proving the mixed-u16 lanes were restored.
         *
         * NOTE Falcon's smash triple is NOT Smash1..3 -- the source gives him
         * { Smash3, Smash2, JumpAerial }. */
        return
            (attr->dead_fgm_ids[0] == 355u) &&
            (attr->dead_fgm_ids[1] == 0x120u) &&
            (attr->deadup_sfx == 349u) &&
            (attr->damage_sfx == 351u) &&
            (attr->smash_sfx[0] == 341u) &&
            (attr->smash_sfx[1] == 340u) &&
            (attr->smash_sfx[2] == 353u) &&
            (attr->itemthrow_vel_scale == 0x64u) &&
            (attr->itemthrow_damage_scale == 0x64u) &&
            (attr->heavyget_sfx == 354u);
    }
#endif
    if (asset_id == NDS_RELOC_ASSET_SAMUS_MAIN)
    {
        /* 217_SamusMain.c:354-362. The US voice block is contiguous from
         * Smash1=573 through Dead=582; Samus dead-slam is source FGM 0x128. */
        return
            (attr->dead_fgm_ids[0] == 582u) &&
            (attr->dead_fgm_ids[1] == 0x128u) &&
            (attr->deadup_sfx == 576u) &&
            (attr->damage_sfx == 581u) &&
            (attr->smash_sfx[0] == 573u) &&
            (attr->smash_sfx[1] == 574u) &&
            (attr->smash_sfx[2] == 575u) &&
            (attr->itemthrow_vel_scale == 0x64u) &&
            (attr->itemthrow_damage_scale == 0x64u) &&
            (attr->heavyget_sfx == nSYAudioFGMVoiceEnd);
    }
#if NDS_P2_LINK
    if (asset_id == NDS_RELOC_ASSET_LINK_MAIN)
    {
        /* 225_LinkMain.c:522-529. Link's US voice run starts at Smash1=401;
         * DeadSlam is the source FGM 0x123. */
        return
            (attr->dead_fgm_ids[0] == 413u) &&
            (attr->dead_fgm_ids[1] == 0x123u) &&
            (attr->deadup_sfx == 405u) &&
            (attr->damage_sfx == 407u) &&
            (attr->smash_sfx[0] == 401u) &&
            (attr->smash_sfx[1] == 402u) &&
            (attr->smash_sfx[2] == 403u) &&
            (attr->itemthrow_vel_scale == 0x64u) &&
            (attr->itemthrow_damage_scale == 0x64u) &&
            (attr->heavyget_sfx == 411u);
    }
#endif
    if (asset_id == NDS_RELOC_ASSET_PIKACHU_MAIN)
    {
        /* 243_PikachuMain.c dead_fgm_ids..heavyget_sfx. BattleShip gmsound.h:
         * Pikachu's voice run is Appeal=536..FuraSleep=551 (Dead 550, DeadUp
         * 542, Damage 544, Smash 537..539, HeavyGet 548); DeadSlam is the
         * source FGM 0x126. */
        return
            (attr->dead_fgm_ids[0] == 550u) &&
            (attr->dead_fgm_ids[1] == 0x126u) &&
            (attr->deadup_sfx == 542u) &&
            (attr->damage_sfx == 544u) &&
            (attr->smash_sfx[0] == 537u) &&
            (attr->smash_sfx[1] == 538u) &&
            (attr->smash_sfx[2] == 539u) &&
            (attr->itemthrow_vel_scale == 0x64u) &&
            (attr->itemthrow_damage_scale == 0x64u) &&
            (attr->heavyget_sfx == 548u);
    }
    if (asset_id == NDS_RELOC_ASSET_YOSHI_MAIN)
    {
        /* 247_YoshiMain.c dYoshiMain_attr dead_fgm_ids..heavyget_sfx.
         * BattleShip gmsound.h (REGION_US): Yoshi's voice run is
         * Appeal=583..UnkVocalize=602 (Dead 595, DeadUp 588, Damage 590,
         * Smash 584..586, HeavyGet 593); DeadSlam is the source FGM 297. */
        return
            (attr->dead_fgm_ids[0] == 595u) &&
            (attr->dead_fgm_ids[1] == 297u) &&
            (attr->deadup_sfx == 588u) &&
            (attr->damage_sfx == 590u) &&
            (attr->smash_sfx[0] == 584u) &&
            (attr->smash_sfx[1] == 585u) &&
            (attr->smash_sfx[2] == 586u) &&
            (attr->itemthrow_vel_scale == 0x64u) &&
            (attr->itemthrow_damage_scale == 0x64u) &&
            (attr->heavyget_sfx == 593u);
    }
    if (asset_id == NDS_RELOC_ASSET_NESS_MAIN)
    {
        /* NessMain.c dNessMain_attr dead_fgm_ids..heavyget_sfx, BattleShip
         * gmsound.h (REGION_US) ordinals, derived by admit_fighter.py. */
        return
            (attr->dead_fgm_ids[0] == 457u) &&
            (attr->dead_fgm_ids[1] == 293u) &&
            (attr->deadup_sfx == 447u) &&
            (attr->damage_sfx == 449u) &&
            (attr->smash_sfx[0] == 443u) &&
            (attr->smash_sfx[1] == 444u) &&
            (attr->smash_sfx[2] == 445u) &&
            (attr->itemthrow_vel_scale == 0x64u) &&
            (attr->itemthrow_damage_scale == 0x64u) &&
            (attr->heavyget_sfx == 451u);
    }
    if (asset_id == NDS_RELOC_ASSET_PURIN_MAIN)
    {
        /* PurinMain.c dPurinMain_attr dead_fgm_ids..heavyget_sfx, BattleShip
         * gmsound.h (REGION_US) ordinals, derived by admit_fighter.py. */
        return
            (attr->dead_fgm_ids[0] == 564u) &&
            (attr->dead_fgm_ids[1] == 295u) &&
            (attr->deadup_sfx == 562u) &&
            (attr->damage_sfx == 564u) &&
            (attr->smash_sfx[0] == 558u) &&
            (attr->smash_sfx[1] == 559u) &&
            (attr->smash_sfx[2] == 560u) &&
            (attr->itemthrow_vel_scale == 0x64u) &&
            /* nSYAudioFGMVoiceEnd, and Jigglypuff is the only fighter whose
             * heavyget_sfx is that terminator rather than a real voice. The
             * value is region-conditional, 0x2B7 under REGION_US and 0x29D
             * otherwise, and this pin was generated as the second one because
             * the generator's enum walk read both arms and kept the last.
             * This ROM compiles with -DREGION_US. */
            (attr->itemthrow_damage_scale == 0x64u) &&
            (attr->heavyget_sfx == 695u);
    }
    if (asset_id == NDS_RELOC_ASSET_KIRBY_MAIN)
    {
        /* KirbyMain.c dKirbyMain_attr dead_fgm_ids..heavyget_sfx, BattleShip
         * gmsound.h (REGION_US) ordinals, derived by admit_fighter.py. */
        return
            (attr->dead_fgm_ids[0] == 396u) &&
            (attr->dead_fgm_ids[1] == 290u) &&
            (attr->deadup_sfx == 388u) &&
            (attr->damage_sfx == 390u) &&
            (attr->smash_sfx[0] == 378u) &&
            (attr->smash_sfx[1] == 379u) &&
            (attr->smash_sfx[2] == 380u) &&
            (attr->itemthrow_vel_scale == 0x64u) &&
            (attr->itemthrow_damage_scale == 0x64u) &&
            (attr->heavyget_sfx == 393u);
    }
#if NDS_P2_GDONKEY
    if (asset_id == NDS_RELOC_ASSET_GDONKEY_MAIN)
    {
        /* GDonkeyMain.c dGDonkeyMain_attr dead_fgm_ids..heavyget_sfx, BattleShip
         * gmsound.h (REGION_US) ordinals, derived by admit_fighter.py. */
        return
            (attr->dead_fgm_ids[0] == 336u) &&
            (attr->dead_fgm_ids[1] == 287u) &&
            (attr->deadup_sfx == 330u) &&
            (attr->damage_sfx == 332u) &&
            (attr->smash_sfx[0] == 326u) &&
            (attr->smash_sfx[1] == 327u) &&
            (attr->smash_sfx[2] == 328u) &&
            (attr->itemthrow_vel_scale == 0x64u) &&
            (attr->itemthrow_damage_scale == 0x64u) &&
            (attr->heavyget_sfx == 334u);
    }
#endif
#if NDS_P2_1P_GAME
    if (asset_id == NDS_RELOC_ASSET_BOSS_MAIN)
    {
        /* 250_BossMain.c:107-114: no voice anywhere, item scales 100. */
        return
            (attr->dead_fgm_ids[0] == nSYAudioFGMVoiceEnd) &&
            (attr->dead_fgm_ids[1] == nSYAudioFGMVoiceEnd) &&
            (attr->deadup_sfx == nSYAudioFGMVoiceEnd) &&
            (attr->damage_sfx == nSYAudioFGMVoiceEnd) &&
            (attr->smash_sfx[0] == nSYAudioFGMVoiceEnd) &&
            (attr->smash_sfx[1] == nSYAudioFGMVoiceEnd) &&
            (attr->smash_sfx[2] == nSYAudioFGMVoiceEnd) &&
            (attr->itemthrow_vel_scale == 0x64u) &&
            (attr->itemthrow_damage_scale == 0x64u) &&
            (attr->heavyget_sfx == nSYAudioFGMVoiceEnd);
    }
#endif
#if NDS_P2_MMARIO
    if (asset_id == NDS_RELOC_ASSET_MMARIO_MAIN)
    {
        /* MMarioMain.c dMMarioMain_attr dead_fgm_ids..heavyget_sfx, BattleShip
         * gmsound.h (REGION_US) ordinals, derived by admit_fighter.py. */
        return
            (attr->dead_fgm_ids[0] == 695u) &&
            (attr->dead_fgm_ids[1] == 695u) &&
            (attr->deadup_sfx == 695u) &&
            (attr->damage_sfx == 695u) &&
            (attr->smash_sfx[0] == 695u) &&
            (attr->smash_sfx[1] == 695u) &&
            (attr->smash_sfx[2] == 695u) &&
            (attr->itemthrow_vel_scale == 0x64u) &&
            (attr->itemthrow_damage_scale == 0x64u) &&
            (attr->heavyget_sfx == 695u);
    }
#endif
#if NDS_P2_NMARIO
    if (asset_id == NDS_RELOC_ASSET_NMARIO_MAIN)
    {
        /* NMarioMain.c dNMarioMain_attr dead_fgm_ids..heavyget_sfx, BattleShip
         * gmsound.h (REGION_US) ordinals, derived by admit_fighter.py. */
        return
            (attr->dead_fgm_ids[0] == 695u) &&
            (attr->dead_fgm_ids[1] == 695u) &&
            (attr->deadup_sfx == 695u) &&
            (attr->damage_sfx == 695u) &&
            (attr->smash_sfx[0] == 695u) &&
            (attr->smash_sfx[1] == 695u) &&
            (attr->smash_sfx[2] == 695u) &&
            (attr->itemthrow_vel_scale == 0x64u) &&
            (attr->itemthrow_damage_scale == 0x64u) &&
            (attr->heavyget_sfx == 695u);
    }
#endif
#if NDS_P2_NFOX
    if (asset_id == NDS_RELOC_ASSET_NFOX_MAIN)
    {
        /* NFoxMain.c dNFoxMain_attr dead_fgm_ids..heavyget_sfx, BattleShip
         * gmsound.h (REGION_US) ordinals, derived by admit_fighter.py. */
        return
            (attr->dead_fgm_ids[0] == 695u) &&
            (attr->dead_fgm_ids[1] == 695u) &&
            (attr->deadup_sfx == 695u) &&
            (attr->damage_sfx == 695u) &&
            (attr->smash_sfx[0] == 695u) &&
            (attr->smash_sfx[1] == 695u) &&
            (attr->smash_sfx[2] == 695u) &&
            (attr->itemthrow_vel_scale == 0x64u) &&
            (attr->itemthrow_damage_scale == 0x64u) &&
            (attr->heavyget_sfx == 695u);
    }
#endif
#if NDS_P2_NDONKEY
    if (asset_id == NDS_RELOC_ASSET_NDONKEY_MAIN)
    {
        /* NDonkeyMain.c dNDonkeyMain_attr dead_fgm_ids..heavyget_sfx, BattleShip
         * gmsound.h (REGION_US) ordinals, derived by admit_fighter.py. */
        return
            (attr->dead_fgm_ids[0] == 695u) &&
            (attr->dead_fgm_ids[1] == 695u) &&
            (attr->deadup_sfx == 695u) &&
            (attr->damage_sfx == 695u) &&
            (attr->smash_sfx[0] == 695u) &&
            (attr->smash_sfx[1] == 695u) &&
            (attr->smash_sfx[2] == 695u) &&
            (attr->itemthrow_vel_scale == 0x64u) &&
            (attr->itemthrow_damage_scale == 0x64u) &&
            (attr->heavyget_sfx == 695u);
    }
#endif
#if NDS_P2_NSAMUS
    if (asset_id == NDS_RELOC_ASSET_NSAMUS_MAIN)
    {
        /* NSamusMain.c dNSamusMain_attr dead_fgm_ids..heavyget_sfx, BattleShip
         * gmsound.h (REGION_US) ordinals, derived by admit_fighter.py. */
        return
            (attr->dead_fgm_ids[0] == 695u) &&
            (attr->dead_fgm_ids[1] == 695u) &&
            (attr->deadup_sfx == 695u) &&
            (attr->damage_sfx == 695u) &&
            (attr->smash_sfx[0] == 695u) &&
            (attr->smash_sfx[1] == 695u) &&
            (attr->smash_sfx[2] == 695u) &&
            (attr->itemthrow_vel_scale == 0x64u) &&
            (attr->itemthrow_damage_scale == 0x64u) &&
            (attr->heavyget_sfx == 695u);
    }
#endif
#if NDS_P2_NLUIGI
    if (asset_id == NDS_RELOC_ASSET_NLUIGI_MAIN)
    {
        /* NLuigiMain.c dNLuigiMain_attr dead_fgm_ids..heavyget_sfx, BattleShip
         * gmsound.h (REGION_US) ordinals, derived by admit_fighter.py. */
        return
            (attr->dead_fgm_ids[0] == 695u) &&
            (attr->dead_fgm_ids[1] == 695u) &&
            (attr->deadup_sfx == 695u) &&
            (attr->damage_sfx == 695u) &&
            (attr->smash_sfx[0] == 695u) &&
            (attr->smash_sfx[1] == 695u) &&
            (attr->smash_sfx[2] == 695u) &&
            (attr->itemthrow_vel_scale == 0x64u) &&
            (attr->itemthrow_damage_scale == 0x64u) &&
            (attr->heavyget_sfx == 695u);
    }
#endif
#if NDS_P2_NLINK
    if (asset_id == NDS_RELOC_ASSET_NLINK_MAIN)
    {
        /* NLinkMain.c dNLinkMain_attr dead_fgm_ids..heavyget_sfx, BattleShip
         * gmsound.h (REGION_US) ordinals, derived by admit_fighter.py. */
        return
            (attr->dead_fgm_ids[0] == 695u) &&
            (attr->dead_fgm_ids[1] == 695u) &&
            (attr->deadup_sfx == 695u) &&
            (attr->damage_sfx == 695u) &&
            (attr->smash_sfx[0] == 695u) &&
            (attr->smash_sfx[1] == 695u) &&
            (attr->smash_sfx[2] == 695u) &&
            (attr->itemthrow_vel_scale == 0x64u) &&
            (attr->itemthrow_damage_scale == 0x64u) &&
            (attr->heavyget_sfx == 695u);
    }
#endif
#if NDS_P2_NYOSHI
    if (asset_id == NDS_RELOC_ASSET_NYOSHI_MAIN)
    {
        /* NYoshiMain.c dNYoshiMain_attr dead_fgm_ids..heavyget_sfx, BattleShip
         * gmsound.h (REGION_US) ordinals, derived by admit_fighter.py. */
        return
            (attr->dead_fgm_ids[0] == 695u) &&
            (attr->dead_fgm_ids[1] == 695u) &&
            (attr->deadup_sfx == 695u) &&
            (attr->damage_sfx == 695u) &&
            (attr->smash_sfx[0] == 695u) &&
            (attr->smash_sfx[1] == 695u) &&
            (attr->smash_sfx[2] == 695u) &&
            (attr->itemthrow_vel_scale == 0x64u) &&
            (attr->itemthrow_damage_scale == 0x64u) &&
            (attr->heavyget_sfx == 695u);
    }
#endif
#if NDS_P2_NCAPTAIN
    if (asset_id == NDS_RELOC_ASSET_NCAPTAIN_MAIN)
    {
        /* NCaptainMain.c dNCaptainMain_attr dead_fgm_ids..heavyget_sfx, BattleShip
         * gmsound.h (REGION_US) ordinals, derived by admit_fighter.py. */
        return
            (attr->dead_fgm_ids[0] == 695u) &&
            (attr->dead_fgm_ids[1] == 695u) &&
            (attr->deadup_sfx == 695u) &&
            (attr->damage_sfx == 695u) &&
            (attr->smash_sfx[0] == 695u) &&
            (attr->smash_sfx[1] == 695u) &&
            (attr->smash_sfx[2] == 695u) &&
            (attr->itemthrow_vel_scale == 0x64u) &&
            (attr->itemthrow_damage_scale == 0x64u) &&
            (attr->heavyget_sfx == 695u);
    }
#endif
#if NDS_P2_NKIRBY
    if (asset_id == NDS_RELOC_ASSET_NKIRBY_MAIN)
    {
        /* NKirbyMain.c dNKirbyMain_attr dead_fgm_ids..heavyget_sfx, BattleShip
         * gmsound.h (REGION_US) ordinals, derived by admit_fighter.py. */
        return
            (attr->dead_fgm_ids[0] == 695u) &&
            (attr->dead_fgm_ids[1] == 695u) &&
            (attr->deadup_sfx == 695u) &&
            (attr->damage_sfx == 695u) &&
            (attr->smash_sfx[0] == 695u) &&
            (attr->smash_sfx[1] == 695u) &&
            (attr->smash_sfx[2] == 695u) &&
            (attr->itemthrow_vel_scale == 0x64u) &&
            (attr->itemthrow_damage_scale == 0x64u) &&
            (attr->heavyget_sfx == 695u);
    }
#endif
#if NDS_P2_NPIKACHU
    if (asset_id == NDS_RELOC_ASSET_NPIKACHU_MAIN)
    {
        /* NPikachuMain.c dNPikachuMain_attr dead_fgm_ids..heavyget_sfx, BattleShip
         * gmsound.h (REGION_US) ordinals, derived by admit_fighter.py. */
        return
            (attr->dead_fgm_ids[0] == 695u) &&
            (attr->dead_fgm_ids[1] == 695u) &&
            (attr->deadup_sfx == 695u) &&
            (attr->damage_sfx == 695u) &&
            (attr->smash_sfx[0] == 695u) &&
            (attr->smash_sfx[1] == 695u) &&
            (attr->smash_sfx[2] == 695u) &&
            (attr->itemthrow_vel_scale == 0x64u) &&
            (attr->itemthrow_damage_scale == 0x64u) &&
            (attr->heavyget_sfx == 695u);
    }
#endif
#if NDS_P2_NPURIN
    if (asset_id == NDS_RELOC_ASSET_NPURIN_MAIN)
    {
        /* NPurinMain.c dNPurinMain_attr dead_fgm_ids..heavyget_sfx, BattleShip
         * gmsound.h (REGION_US) ordinals, derived by admit_fighter.py. */
        return
            (attr->dead_fgm_ids[0] == 695u) &&
            (attr->dead_fgm_ids[1] == 695u) &&
            (attr->deadup_sfx == 695u) &&
            (attr->damage_sfx == 695u) &&
            (attr->smash_sfx[0] == 695u) &&
            (attr->smash_sfx[1] == 695u) &&
            (attr->smash_sfx[2] == 695u) &&
            (attr->itemthrow_vel_scale == 0x64u) &&
            (attr->itemthrow_damage_scale == 0x64u) &&
            (attr->heavyget_sfx == 695u);
    }
#endif
#if NDS_P2_NNESS
    if (asset_id == NDS_RELOC_ASSET_NNESS_MAIN)
    {
        /* NNessMain.c dNNessMain_attr dead_fgm_ids..heavyget_sfx, BattleShip
         * gmsound.h (REGION_US) ordinals, derived by admit_fighter.py. */
        return
            (attr->dead_fgm_ids[0] == 695u) &&
            (attr->dead_fgm_ids[1] == 695u) &&
            (attr->deadup_sfx == 695u) &&
            (attr->damage_sfx == 695u) &&
            (attr->smash_sfx[0] == 695u) &&
            (attr->smash_sfx[1] == 695u) &&
            (attr->smash_sfx[2] == 695u) &&
            (attr->itemthrow_vel_scale == 0x64u) &&
            (attr->itemthrow_damage_scale == 0x64u) &&
            (attr->heavyget_sfx == 695u);
    }
#endif
    return FALSE;
}

static void ndsRelocNormalizeWeaponAttributes(WPAttributes *attr);
static s32 ndsRelocMarkWeaponAttributesNormalized(
    const NDSRelocLoadedFile *loaded, u32 attr_offset);
static s32 ndsRelocNormalizePikachuWeaponAttributes(
    NDSRelocLoadedFile *loaded, u32 attr_offset, s16 attack0_y, s16 attack1_y,
    s16 map_top, s16 map_center, s16 map_bottom, s16 map_width);

static s32 ndsRelocNormalizeFighterCommonPartFlags(
    NDSRelocLoadedFile *loaded, FTCommonPartContainer *common)
{
    u32 detail;

    if (common == NULL)
    {
        return TRUE;
    }
    if (((uintptr_t)common < (uintptr_t)loaded->data) ||
        (ndsRelocRangeInLoadedFile(loaded,
            (u32)((uintptr_t)common - (uintptr_t)loaded->data),
            sizeof(*common)) == FALSE))
    {
        ndsRelocRecordExternalFixupFail(loaded->asset_id);
        return FALSE;
    }
    /* FTCommonPart ends in a byte followed by three padding bytes. The
     * file's u32 swap moved that byte to the high lane. Restore it before
     * ftManager copies the dispatch flags into every live FTParts: Yoshi
     * requires case 1 (pre/post display-list pairs), not case 0. */
    for (detail = 0u; detail < ARRAY_COUNT(common->commonparts); detail++)
    {
        FTCommonPart *part = &common->commonparts[detail];
        part->flags = (u8)(ndsRelocReadNative32(&part->flags) >> 24);
    }
    return TRUE;
}

static void ndsRelocReverseWordBytes(void *base, u32 begin, u32 end);

static s32 ndsRelocNormalizeFighterAttributesFile(
    NDSRelocLoadedFile *loaded)
{
    u32 attr_offset;
    u8 *attr_bytes;
    FTAttributes *attr;
    WPAttributes *samus_bomb_attr;
#if NDS_P2_LINK
    WPAttributes *link_spin_attack_attr;
#endif

    if ((loaded == NULL) || (loaded->data == NULL))
    {
        return FALSE;
    }
    if (loaded->asset_id == NDS_RELOC_ASSET_MARIO_MAIN)
    {
        attr_offset = NDS_RELOC_SYMBOL_MARIO_MAIN_ATTRIBUTES;
    }
    else if (loaded->asset_id == NDS_RELOC_ASSET_FOX_MAIN)
    {
        attr_offset = NDS_RELOC_SYMBOL_FOX_MAIN_ATTRIBUTES;
    }
#if NDS_P2_LUIGI
    else if (loaded->asset_id == NDS_RELOC_ASSET_LUIGI_MAIN)
    {
        attr_offset = NDS_RELOC_SYMBOL_LUIGI_MAIN_ATTRIBUTES;
    }
#endif
#if NDS_P2_DONKEY
    else if (loaded->asset_id == NDS_RELOC_ASSET_DONKEY_MAIN)
    {
        attr_offset = NDS_RELOC_SYMBOL_DONKEY_MAIN_ATTRIBUTES;
    }
#endif
#if NDS_P2_CAPTAIN
    else if (loaded->asset_id == NDS_RELOC_ASSET_CAPTAIN_MAIN)
    {
        attr_offset = NDS_RELOC_SYMBOL_CAPTAIN_MAIN_ATTRIBUTES;
    }
#endif
    else if (loaded->asset_id == NDS_RELOC_ASSET_SAMUS_MAIN)
    {
        attr_offset = NDS_RELOC_SYMBOL_SAMUS_MAIN_ATTRIBUTES;
    }
#if NDS_P2_LINK
    else if (loaded->asset_id == NDS_RELOC_ASSET_LINK_MAIN)
    {
        attr_offset = NDS_RELOC_SYMBOL_LINK_MAIN_ATTRIBUTES;
    }
#endif
    else if (loaded->asset_id == NDS_RELOC_ASSET_PIKACHU_MAIN)
    {
        attr_offset = NDS_RELOC_SYMBOL_PIKACHU_MAIN_ATTRIBUTES;
    }
    else if (loaded->asset_id == NDS_RELOC_ASSET_YOSHI_MAIN)
    {
        attr_offset = NDS_RELOC_SYMBOL_YOSHI_MAIN_ATTRIBUTES;
    }
    else if (loaded->asset_id == NDS_RELOC_ASSET_NESS_MAIN)
    {
        attr_offset = NDS_RELOC_SYMBOL_NESS_MAIN_ATTRIBUTES;
    }
    else if (loaded->asset_id == NDS_RELOC_ASSET_PURIN_MAIN)
    {
        attr_offset = NDS_RELOC_SYMBOL_PURIN_MAIN_ATTRIBUTES;
    }
    else if (loaded->asset_id == NDS_RELOC_ASSET_KIRBY_MAIN)
    {
        attr_offset = NDS_RELOC_SYMBOL_KIRBY_MAIN_ATTRIBUTES;
    }
#if NDS_P2_GDONKEY
    else if (loaded->asset_id == NDS_RELOC_ASSET_GDONKEY_MAIN)
    {
        attr_offset = NDS_RELOC_SYMBOL_GDONKEY_MAIN_ATTRIBUTES;
    }
#endif
#if NDS_P2_MMARIO
    else if (loaded->asset_id == NDS_RELOC_ASSET_MMARIO_MAIN)
    {
        attr_offset = NDS_RELOC_SYMBOL_MMARIO_MAIN_ATTRIBUTES;
    }
#endif
#if NDS_P2_1P_GAME
    /* Master Hand. Without this the commonparts flags kept the u32 swap's
     * lane: 0 instead of 250_BossMain.c's 0x01, so the pre/post display-list
     * pairs (dBossModel_JointTree's 23 pair bases) were drawn as plain lists
     * and the native owner, which welds them, saw 23 roots for its 18 --
     * Master Hand was never drawn. */
    else if (loaded->asset_id == NDS_RELOC_ASSET_BOSS_MAIN)
    {
        attr_offset = NDS_RELOC_SYMBOL_BOSS_MAIN_ATTRIBUTES;
    }
#endif
#if NDS_P2_NMARIO
    else if (loaded->asset_id == NDS_RELOC_ASSET_NMARIO_MAIN)
    {
        attr_offset = NDS_RELOC_SYMBOL_NMARIO_MAIN_ATTRIBUTES;
    }
#endif
#if NDS_P2_NFOX
    else if (loaded->asset_id == NDS_RELOC_ASSET_NFOX_MAIN)
    {
        attr_offset = NDS_RELOC_SYMBOL_NFOX_MAIN_ATTRIBUTES;
    }
#endif
#if NDS_P2_NDONKEY
    else if (loaded->asset_id == NDS_RELOC_ASSET_NDONKEY_MAIN)
    {
        attr_offset = NDS_RELOC_SYMBOL_NDONKEY_MAIN_ATTRIBUTES;
    }
#endif
#if NDS_P2_NSAMUS
    else if (loaded->asset_id == NDS_RELOC_ASSET_NSAMUS_MAIN)
    {
        attr_offset = NDS_RELOC_SYMBOL_NSAMUS_MAIN_ATTRIBUTES;
    }
#endif
#if NDS_P2_NLUIGI
    else if (loaded->asset_id == NDS_RELOC_ASSET_NLUIGI_MAIN)
    {
        attr_offset = NDS_RELOC_SYMBOL_NLUIGI_MAIN_ATTRIBUTES;
    }
#endif
#if NDS_P2_NLINK
    else if (loaded->asset_id == NDS_RELOC_ASSET_NLINK_MAIN)
    {
        attr_offset = NDS_RELOC_SYMBOL_NLINK_MAIN_ATTRIBUTES;
    }
#endif
#if NDS_P2_NYOSHI
    else if (loaded->asset_id == NDS_RELOC_ASSET_NYOSHI_MAIN)
    {
        attr_offset = NDS_RELOC_SYMBOL_NYOSHI_MAIN_ATTRIBUTES;
    }
#endif
#if NDS_P2_NCAPTAIN
    else if (loaded->asset_id == NDS_RELOC_ASSET_NCAPTAIN_MAIN)
    {
        attr_offset = NDS_RELOC_SYMBOL_NCAPTAIN_MAIN_ATTRIBUTES;
    }
#endif
#if NDS_P2_NKIRBY
    else if (loaded->asset_id == NDS_RELOC_ASSET_NKIRBY_MAIN)
    {
        attr_offset = NDS_RELOC_SYMBOL_NKIRBY_MAIN_ATTRIBUTES;
    }
#endif
#if NDS_P2_NPIKACHU
    else if (loaded->asset_id == NDS_RELOC_ASSET_NPIKACHU_MAIN)
    {
        attr_offset = NDS_RELOC_SYMBOL_NPIKACHU_MAIN_ATTRIBUTES;
    }
#endif
#if NDS_P2_NPURIN
    else if (loaded->asset_id == NDS_RELOC_ASSET_NPURIN_MAIN)
    {
        attr_offset = NDS_RELOC_SYMBOL_NPURIN_MAIN_ATTRIBUTES;
    }
#endif
#if NDS_P2_NNESS
    else if (loaded->asset_id == NDS_RELOC_ASSET_NNESS_MAIN)
    {
        attr_offset = NDS_RELOC_SYMBOL_NNESS_MAIN_ATTRIBUTES;
    }
#endif
    /* P4 donor main files: same FTAttributes layout at the donor's offset. */
    else if ((attr_offset = ndsP4MainAttributesOffset(loaded->asset_id)) != 0u)
    {
    }
    else
    {
        return TRUE;
    }
    if (ndsRelocRangeInLoadedFile(
            loaded, attr_offset, sizeof(FTAttributes)) == FALSE)
    {
        ndsRelocRecordExternalFixupFail(loaded->asset_id);
        return FALSE;
    }

    attr_bytes = (u8 *)loaded->data + attr_offset;
    attr = (FTAttributes *)attr_bytes;
    if (loaded->format_fixups_applied == FALSE)
    {
        if (ndsRelocNormalizeFighterCommonPartFlags(
                loaded, attr->commonparts_container) == FALSE)
        {
            return FALSE;
        }
        /* SamusMain is unusual: its first 0x4c bytes are also Bomb's
         * WPAttributes (llSamusMainBombWeaponAttributes = 0x0c), while the
         * fighter FTAttributes live at 0x610. The generic word swap reverses
         * the two s16 lanes in BOTH mixed-width structs. Normalize Bomb before
         * this file-wide flag is set, or the weapon pass immediately after
         * this one will correctly assume the file is already converted and
         * leave the source { 75, 0, -75, 75 } collision box swapped. */
        if (loaded->asset_id == NDS_RELOC_ASSET_SAMUS_MAIN)
        {
            if (ndsRelocRangeInLoadedFile(
                    loaded,
                    NDS_RELOC_SYMBOL_SAMUS_MAIN_BOMB_WEAPON_ATTRIBUTES,
                    sizeof(WPAttributes)) == FALSE)
            {
                ndsRelocRecordExternalFixupFail(loaded->asset_id);
                return FALSE;
            }
            samus_bomb_attr = (WPAttributes *)((u8 *)loaded->data +
                NDS_RELOC_SYMBOL_SAMUS_MAIN_BOMB_WEAPON_ATTRIBUTES);
            ndsRelocNormalizeWeaponAttributes(samus_bomb_attr);
            ndsRelocMarkWeaponAttributesNormalized(
                loaded,
                NDS_RELOC_SYMBOL_SAMUS_MAIN_BOMB_WEAPON_ATTRIBUTES);
        }
#if NDS_P2_LINK
        /* LinkMain shares SamusMain's mixed-struct layout: the file-global
         * format flag covers Link's FTAttributes at 0x708 and grounded Spin
         * Attack's WPAttributes at 0x0c. Normalize both before setting it. */
        if (loaded->asset_id == NDS_RELOC_ASSET_LINK_MAIN)
        {
            if (ndsRelocRangeInLoadedFile(
                    loaded,
                    NDS_RELOC_SYMBOL_LINK_MAIN_SPIN_ATTACK_WEAPON_ATTRIBUTES,
                    sizeof(WPAttributes)) == FALSE)
            {
                ndsRelocRecordExternalFixupFail(loaded->asset_id);
                return FALSE;
            }
            link_spin_attack_attr = (WPAttributes *)((u8 *)loaded->data +
                NDS_RELOC_SYMBOL_LINK_MAIN_SPIN_ATTACK_WEAPON_ATTRIBUTES);
            ndsRelocNormalizeWeaponAttributes(link_spin_attack_attr);
            ndsRelocMarkWeaponAttributesNormalized(
                loaded,
                NDS_RELOC_SYMBOL_LINK_MAIN_SPIN_ATTACK_WEAPON_ATTRIBUTES);
        }
#endif
        /* PikachuMain has the same shape: Thunder's head (0x0c) and trail
         * (0x40) WPAttributes overlap the file-handle words ahead of the
         * fighter FTAttributes at 0x41c. Normalize and pin both here, before
         * the file-wide flag is set, for the same reason as Samus's Bomb. */
        if ((loaded->asset_id == NDS_RELOC_ASSET_PIKACHU_MAIN) &&
            (ndsRelocNormalizePikachuWeaponAttributes(
                 loaded,
                 NDS_RELOC_SYMBOL_PIKACHU_MAIN_THUNDER_HEAD_WEAPON_ATTRIBUTES,
                 225, 0, 100, 0, -100, 50) == FALSE ||
             ndsRelocNormalizePikachuWeaponAttributes(
                 loaded,
                 NDS_RELOC_SYMBOL_PIKACHU_MAIN_THUNDER_TRAIL_WEAPON_ATTRIBUTES,
                 240, -240, 225, 0, -225, 75) == FALSE))
        {
            ndsRelocRecordExternalFixupFail(loaded->asset_id);
            return FALSE;
        }
        /* YoshiMain has the same shape: Egg Throw's (0x0c) and the stars'
         * (0x40) WPAttributes overlap the file-handle words ahead of the
         * fighter FTAttributes at 0x47c. Both carry zero attack offsets; the
         * map boxes are the source words (egg 150/0/-150/150, star
         * 100/0/-100/96). Normalize and pin them here, before the file-wide
         * flag is set, for the same reason as Samus's Bomb. */
        if ((loaded->asset_id == NDS_RELOC_ASSET_YOSHI_MAIN) &&
            (ndsRelocNormalizePikachuWeaponAttributes(
                 loaded,
                 NDS_RELOC_SYMBOL_YOSHI_MAIN_EGG_THROW_WEAPON_ATTRIBUTES,
                 0, 0, 150, 0, -150, 150) == FALSE ||
             ndsRelocNormalizePikachuWeaponAttributes(
                 loaded,
                 NDS_RELOC_SYMBOL_YOSHI_MAIN_STAR_WEAPON_ATTRIBUTES,
                 0, 0, 100, 0, -100, 96) == FALSE))
        {
            ndsRelocRecordExternalFixupFail(loaded->asset_id);
            return FALSE;
        }
        /* Kirby: source WPAttributes overlapping the file-handle words, normalized
         * and pinned before the file-wide flag is set (relocData initializers,
         * admit_fighter.py). */
        if ((loaded->asset_id == NDS_RELOC_ASSET_KIRBY_MAIN) &&
            (ndsRelocNormalizePikachuWeaponAttributes(
                 loaded,
                 NDS_RELOC_SYMBOL_KIRBY_MAIN_CUTTER_WEAPON_ATTRIBUTES,
                 0, 0, 220, 0, -220, 50) == FALSE))
        {
            ndsRelocRecordExternalFixupFail(loaded->asset_id);
            return FALSE;
        }
        /* Ness: source WPAttributes overlapping the file-handle words, normalized
         * and pinned before the file-wide flag is set (relocData initializers,
         * admit_fighter.py). */
        if ((loaded->asset_id == NDS_RELOC_ASSET_NESS_MAIN) &&
            (ndsRelocNormalizePikachuWeaponAttributes(
                 loaded,
                 NDS_RELOC_SYMBOL_NESS_MAIN_PK_THUNDER_WEAPON_ATTRIBUTES,
                 0, 0, 100, 0, -100, 100) == FALSE ||
             ndsRelocNormalizePikachuWeaponAttributes(
                 loaded,
                 NDS_RELOC_SYMBOL_NESS_MAIN_PK_THUNDER_TRAIL_WEAPON_ATTRIBUTES,
                 -100, 0, 50, 0, -50, 50) == FALSE))
        {
            ndsRelocRecordExternalFixupFail(loaded->asset_id);
            return FALSE;
        }
        /* Ness: source WPAttributes overlapping the file-handle words, normalized
         * and pinned before the file-wide flag is set (relocData initializers,
         * admit_fighter.py). */
        if ((loaded->asset_id == NDS_RELOC_ASSET_NESS_SPECIAL1) &&
            (ndsRelocNormalizePikachuWeaponAttributes(
                 loaded,
                 NDS_RELOC_SYMBOL_NESS_SPECIAL1_PK_FIRE_WEAPON_ATTRIBUTES,
                 0, 0, 10, 0, -10, 10) == FALSE))
        {
            ndsRelocRecordExternalFixupFail(loaded->asset_id);
            return FALSE;
        }
        ndsRelocSwapNativeU16WordLanes(
            attr_bytes + offsetof(FTAttributes, dead_fgm_ids));
        ndsRelocSwapNativeU16WordLanes(
            attr_bytes + offsetof(FTAttributes, deadup_sfx));
        ndsRelocSwapNativeU16WordLanes(
            attr_bytes + offsetof(FTAttributes, smash_sfx));
        ndsRelocSwapNativeU16WordLanes(
            attr_bytes + offsetof(FTAttributes, smash_sfx) +
            (2u * sizeof(u16)));
        ndsRelocSwapNativeU16WordLanes(
            attr_bytes + offsetof(FTAttributes, itemthrow_vel_scale));
        ndsRelocSwapNativeU16WordLanes(
            attr_bytes + offsetof(FTAttributes, heavyget_sfx));
        /* shade_color[3] and fog_color are packed u8 RGBA words, so the
         * blanket swap had them as ABGR; the shade reader
         * (reloc_backend_compat_shims.c, ftParamsSetupShadeColors) took the
         * lanes raw (byte-lane audit, 2026-09-07). */
        _Static_assert((offsetof(FTAttributes, shade_color) %
                        sizeof(u32)) == 0u,
                       "FTAttributes shade_color must start a word");
        _Static_assert((offsetof(FTAttributes, fog_color) -
                        offsetof(FTAttributes, shade_color)) == 12u,
                       "FTAttributes colour run must be four words");
        ndsRelocReverseWordBytes(
            attr_bytes, (u32)offsetof(FTAttributes, shade_color),
            (u32)offsetof(FTAttributes, fog_color) + 4u);
        loaded->format_fixups_applied = TRUE;
    }
    if (
        /* No BattleShip source table describes a donor; the generator's
         * round-trip check is its source proof. */
        (ndsP4MainAttributesOffset(loaded->asset_id) == 0u) &&
        (ndsRelocFighterAttributesMatchSource(loaded->asset_id, attr) == FALSE))
    {
        ndsRelocRecordExternalFixupFail(loaded->asset_id);
        return FALSE;
    }
    return TRUE;
}

static size_t ndsRelocAssetAllocSize(u32 asset_id);
static s32 ndsRelocFinalizeLoadedFile(NDSRelocLoadedFile *loaded);
static s32 ndsRelocNormalizeBattleInterfaceSprites(
    NDSRelocLoadedFile *loaded);
static size_t ndsRelocExternTreeAllocSize(u32 asset_id, u32 *seen,
                                          u32 *seen_count);
static NDSRelocLoadedFile *ndsRelocLoadExternTreeAsset(u32 asset_id,
                                                       uintptr_t *heap_ptr);

static void *ndsRelocStaticBufferForAsset(u32 asset_id, size_t asset_size)
{
    u8 *store;

    if ((asset_id == NDS_RELOC_ASSET_STAGE_CASTLE) &&
        (asset_size <= NDS_RELOC_STAGE_CASTLE_STATIC_SIZE))
    {
        return ndsRelocSceneFileBuffer();
    }
    if ((asset_id == NDS_RELOC_ASSET_EXTERN_DATA_BANK_113) &&
        (asset_size <= NDS_RELOC_EXTERN_DATA_BANK_113_STATIC_SIZE) &&
        ((NDS_RELOC_STAGE_CASTLE_STATIC_SIZE +
          NDS_RELOC_EXTERN_DATA_BANK_113_STATIC_SIZE) <=
         NDS_RELOC_SCENE_FILE_BUFFER_SIZE))
    {
        store = ndsRelocSceneFileBuffer();
        return (store != NULL) ?
            &store[NDS_RELOC_STAGE_CASTLE_STATIC_SIZE] : NULL;
    }
    return ndsFrontendOverlayTryAlloc(asset_size, NDS_RELOC_ALIGN_BYTES);
}

/* Counts heap allocations DECLINED rather than allowed to spin. Any non-zero
 * value means the general heap ran out during play and the ROM survived it --
 * before the guards at the two loaders below, that was a hard freeze with no
 * error and no recovery. A rising count in a soak is the heap budget getting
 * tight, not a bug in itself. Defined here because both guarded sites are
 * above the rest of this file's counter block. */
volatile u32 gNdsRelocHeapDeclineCount;

static u32 sNdsRelocEnsureKeepFree;

static NDSRelocLoadedFile *ndsRelocEnsureLoadedAsset(u32 asset_id)
{
    NDSRelocLoadedFile *loaded;
    size_t asset_size;
    NDSRelocAssetHeader header;
    void *heap;

    ndsRelocPrepareSceneCache();
    loaded = ndsRelocFindLoadedFileByAsset(asset_id);

    if (loaded != NULL)
    {
        if (ndsRelocFinalizeLoadedFile(loaded) == FALSE)
        {
            return NULL;
        }
        return loaded;
    }

    asset_size = ndsRelocAssetAllocSize(asset_id);
    if (asset_size == 0)
    {
        ndsRelocRecordExternalFixupFail(asset_id);
        return NULL;
    }

    heap = ndsRelocStaticBufferForAsset(asset_id, asset_size);
    if (heap == NULL)
    {
        /* Ask BEFORE allocating. syTaskmanMalloc cannot return NULL -- on
         * exhaustion syMallocSet spins in `while (TRUE);` (decomp
         * src/sys/malloc.c:30) -- so the NULL test below it has always been
         * dead code and this call is a freeze, not a failed load. Same defect
         * the anim cache was moved off the heap for on 2026-07-29; that fix
         * covered ndsR2AnimCacheStore and left the two loaders that reach the
         * heap on their own. See the arena's own use of this guard at :6078. */
        if (ndsSyMallocWouldFit(&gSYTaskmanGeneralHeap, asset_size
                                + sNdsRelocEnsureKeepFree
                                , 0x10) == FALSE)
        {
            gNdsRelocHeapDeclineCount++;
            ndsRelocRecordExternalFixupFail(asset_id);
            return NULL;
        }
        heap = syTaskmanMalloc(asset_size, 0x10);
    }
    if (heap == NULL)
    {
        ndsRelocRecordExternalFixupFail(asset_id);
        return NULL;
    }
    if (ndsRelocAssetLoadData(asset_id, heap, asset_size, &header) == FALSE)
    {
        ndsRelocRecordExternalFixupFail(asset_id);
        return NULL;
    }

    loaded = ndsRelocRegisterLoadedFile(asset_id, 0, heap, &header);
    if (loaded == NULL)
    {
        ndsRelocRecordExternalFixupFail(asset_id);
        return NULL;
    }
    if (ndsRelocApplyWordByteSwap(loaded) == FALSE)
    {
        ndsRelocRecordExternalFixupFail(asset_id);
        return NULL;
    }
    if (ndsRelocFinalizeLoadedFile(loaded) == FALSE)
    {
        return NULL;
    }
    return loaded;
}

static s32 ndsRelocNativeEntryOwnsDependency(u32 owner_asset, u32 dep_asset)
{
    if (ndsP4LabSkipsDependency(owner_asset, dep_asset) != FALSE)
    {
        return TRUE;
    }
    /* A content's entry article whose images its native packets carry (the
     * Wolfen's four texture pointers into bank 109, Fox's Arwing's case). */
    if (ndsP4NativeOwnsDependency(owner_asset, dep_asset) != FALSE)
    {
        return TRUE;
    }
    return ((gNdsSceneManagerCurrIsBattle != 0u) &&
            (owner_asset == NDS_RELOC_ASSET_FOX_SPECIAL3) &&
            (dep_asset == NDS_RELOC_ASSET_EXTERN_DATA_BANK_109)) ? TRUE : FALSE;
}

static s32 ndsRelocResolveNativeEntryExternalFixup(
    u32 owner_asset, u32 dep_asset, u32 slot_offset, u32 target_offset,
    void **resolved)
{
    if (ndsRelocNativeEntryOwnsDependency(owner_asset, dep_asset) == FALSE)
    {
        return 0;
    }
    /* Lab builds: a donor special file a stand-in replaces is never loaded;
     * the main's header words that named it read NULL, like the slots
     * aimed past a file (gNdsP4RelocOutOfRangeSlots). */
    if (ndsP4LabSkipsDependency(owner_asset, dep_asset) != FALSE)
    {
        (void)slot_offset;
        (void)target_offset;
        gNdsP4RelocOutOfRangeSlots++;
        *resolved = NULL;
        return 1;
    }
    if (ndsP4NativeOwnsDependency(owner_asset, dep_asset) != FALSE)
    {
        if (ndsP4NativeBakedRef(owner_asset, dep_asset, slot_offset,
                                target_offset) == FALSE)
        {
            return -1;
        }
        *resolved = NULL;
        return 1;
    }
    /* The three SETTIMG pointers all name bank 109's texture at 0x19f8.
     * The native Arwing packet already contains its converted pixels. Keep
     * FoxSpecial3's live DObjDesc/AnimJoint, but do not load the 47,120-byte
     * bank solely for these baked texture references. NULL makes accidental
     * raw consumption fail closed; native submission uses its baked slots. */
    if (target_offset != 0x19f8u)
    {
        return -1;
    }
    switch (slot_offset)
    {
    case 0x2974u:
    case 0x2ab4u:
    case 0x2b64u:
        *resolved = NULL;
        return 1;
    default:
        return -1;
    }
}

static s32 ndsRelocApplyExternalPointerFixups(NDSRelocLoadedFile *loaded)
{
    u16 reloc_extern;
    u32 guard;
    u32 extern_index = 0;

    if ((loaded == NULL) || (loaded->data == NULL))
    {
        return FALSE;
    }
    if (loaded->external_fixups_applied != FALSE)
    {
        return TRUE;
    }
    if (loaded->reloc_extern_offset == 0xffffu)
    {
        loaded->external_fixups_applied = TRUE;
        return TRUE;
    }
    if (loaded->extern_count == 0u)
    {
        loaded->external_fixup_fail_count++;
        ndsRelocRecordExternalFixupFail(loaded->asset_id);
        return FALSE;
    }

    reloc_extern = loaded->reloc_extern_offset;
    guard = (loaded->data_size / sizeof(u32)) + 1u;

    while (reloc_extern != 0xffffu)
    {
        uintptr_t slot_offset = (uintptr_t)reloc_extern * sizeof(u32);
        u32 reloc_word;
        u16 next_reloc;
        u16 target_words;
        uintptr_t target_offset;
        u32 dep_asset_id;
        NDSRelocLoadedFile *dep;
        void *native_dep = NULL;
        s32 native_dep_result;

        if ((guard == 0) ||
            ((slot_offset + sizeof(u32)) > loaded->data_size) ||
            (extern_index >= loaded->extern_count))
        {
            loaded->external_fixup_fail_count++;
            if (gNdsRelocExternalFixupFailCount == 0u)
            {
                gNdsRelocExternalFixupFailIndex = extern_index;
                gNdsRelocExternalFixupFailSlot = reloc_extern;
                gNdsRelocExternalFixupFailDeclared = loaded->extern_count;
                gNdsRelocExternalFixupFailDataSize =
                    (u32)loaded->data_size;
            }
            ndsRelocRecordExternalFixupFail(loaded->asset_id);
            return FALSE;
        }
        guard--;

        reloc_word = ndsRelocReadNative32((u8 *)loaded->data + slot_offset);
        next_reloc = (u16)(reloc_word >> 16);
        target_words = (u16)(reloc_word & 0xffffu);
        target_offset = (uintptr_t)target_words * sizeof(u32);
        dep_asset_id = ndsRelocAssetIDForToken(loaded->extern_file_ids[extern_index++]);

        /* P2-2 ShieldPose residency: the four current capacity-worst base
         * fighter Main files have exactly nine source fixups into their
         * ShieldPose file (DObjDesc + eight angle tables). The generated native
         * package owns those targets, so resolve them before recursively
         * loading a raw ShieldPose file that the native guard path never reads.
         * A recognized pair with an unknown target is a contract failure, not
         * permission to mix compact handles and raw Event32 pointers. */
        native_dep_result = ndsRelocResolveNativeEntryExternalFixup(
            loaded->asset_id, dep_asset_id, (u32)slot_offset,
            (u32)target_offset, &native_dep);
        if (native_dep_result == 0)
        {
            native_dep_result = ndsShieldPoseResolveExternalFixup(
                loaded->asset_id, dep_asset_id, (u32)target_offset,
                &native_dep);
        }
        if (native_dep_result != 0)
        {
            if (native_dep_result < 0)
            {
                loaded->external_fixup_fail_count++;
                if (gNdsRelocExternalFixupFailCount == 0u)
                {
                    gNdsRelocExternalFixupFailFirstDep = dep_asset_id;
                }
                ndsRelocRecordExternalFixupFail(loaded->asset_id);
                return FALSE;
            }
            ndsRelocWriteNativePointer((u8 *)loaded->data + slot_offset,
                                       native_dep);
            loaded->external_fixup_count++;
            ndsRelocRecordExternalFixupSuccess(loaded->asset_id, dep_asset_id);
            reloc_extern = next_reloc;
            continue;
        }

        dep = ndsRelocEnsureLoadedAsset(dep_asset_id);
        /* Remix kept the parent's header offsets into small donor files:
         * the N64 relocates such a slot unchecked, so treat it like the
         * internal case above (a NULL that nothing reads). */
        if ((dep != NULL) && (target_offset >= dep->data_size) &&
            (ndsP4RelocAssetPath(loaded->asset_id) != NULL))
        {
            gNdsP4RelocOutOfRangeSlots++;
            ndsRelocWriteNativePointer((u8 *)loaded->data + slot_offset, NULL);
            loaded->external_fixup_count++;
            reloc_extern = next_reloc;
            continue;
        }
        /* A compact polygon model's display lists are identity cells: a
         * main's model-part slot aimed at one lands on its cell. */
        if ((dep == NULL) || (target_offset >= dep->data_size) ||
            (ndsRelocLoadedFileTarget(dep, (u32)target_offset) == NULL))
        {
            loaded->external_fixup_fail_count++;
            if (gNdsRelocExternalFixupFailCount == 0u)
            {
                gNdsRelocExternalFixupFailFirstDep = dep_asset_id;
            }
            ndsRelocRecordExternalFixupFail(loaded->asset_id);
            return FALSE;
        }

        ndsRelocWriteNativePointer((u8 *)loaded->data + slot_offset,
                                   ndsRelocLoadedFileTarget(
                                       dep, (u32)target_offset));

        loaded->external_fixup_count++;
        ndsRelocRecordExternalFixupSuccess(loaded->asset_id, dep->asset_id);

        reloc_extern = next_reloc;
    }

    loaded->external_fixups_applied = TRUE;
    return TRUE;
}


static s32 ndsRelocFinalizeLoadedFile(NDSRelocLoadedFile *loaded)
{

    if (loaded == NULL)
    {
        return FALSE;
    }
    if (loaded->fixups_applying != FALSE)
    {
        return TRUE;
    }

    /* Task 75 E0: counted here rather than at the open, because this is the one
     * point every load path funnels through exactly once per file -- the early
     * return above is the re-entrant case and must not be counted twice. */
    NDS_TASK75_MARK_ASSET_LOAD();
    /* K0 line 4, "animation-file relocation / fixups". This function is the
     * whole fixup chain: internal pointers, AObj16, attributes, weapon
     * attributes, external pointers and the sprite pass. */
    NDS_K0_MARK(gNdsK0AfterGoRelocs, loaded->asset_id);
    loaded->fixups_applying = TRUE;
    if ((ndsRelocApplyInternalPointerFixups(loaded) == FALSE) ||
        (ndsRelocNormalizeFighterAObj16File(loaded) == FALSE) ||
        (ndsRelocNormalizeKirbyMainMotionCopyTable(loaded) == FALSE) ||
        (ndsRelocNormalizeFighterAttributesFile(loaded) == FALSE) ||
        (ndsRelocApplyExternalPointerFixups(loaded) == FALSE))
    {
        loaded->fixups_applying = FALSE;
        return FALSE;
    }
    loaded->fixups_applying = FALSE;

    if (ndsRelocNormalizeBattleInterfaceSprites(loaded) == FALSE)
    {
        return FALSE;
    }

    if (ndsPupupuStageAssetBit(loaded->asset_id) != 0u)
    {
        gNdsStagePupupuRelocAssetMask |=
            ndsPupupuStageAssetBit(loaded->asset_id);
        gNdsStagePupupuRelocDependencyMask |=
            ndsPupupuStageAssetBit(loaded->asset_id);
#if defined(NDS_P2_COMPACT_GROUND_MAPS) && NDS_P2_COMPACT_GROUND_MAPS
        /* The compact Pupupu map carries the wallpaper stub itself, so
         * StageDreamLand (bit 1) is no longer a file of its tree. */
        if (ndsRelocCompactGroundMapSprite(loaded) != NULL)
        {
            gNdsStagePupupuRelocAssetMask |=
                ndsPupupuStageAssetBit(NDS_RELOC_ASSET_STAGE_DREAM_LAND);
            gNdsStagePupupuRelocDependencyMask |=
                ndsPupupuStageAssetBit(NDS_RELOC_ASSET_STAGE_DREAM_LAND);
        }
#endif
        if ((gNdsStagePupupuRelocAssetMask & 0x1fu) == 0x1fu)
        {
            gNdsStagePupupuRelocResult = NDS_STAGE_PUPUPU_RELOC_PASS;
        }
    }
    if (ndsFighterMarioFoxAssetBit(loaded->asset_id) != 0u)
    {
        gNdsFighterMarioFoxRelocAssetMask |=
            ndsFighterMarioFoxAssetBit(loaded->asset_id);
        gNdsFighterMarioFoxRelocDependencyMask |=
            ndsFighterMarioFoxAssetBit(loaded->asset_id);
        if ((gNdsFighterMarioFoxRelocAssetMask &
             NDS_FIGHTER_MARIOFOX_FILE_MASK) ==
            NDS_FIGHTER_MARIOFOX_FILE_MASK)
        {
            gNdsFighterMarioFoxRelocResult =
                NDS_FIGHTER_MARIOFOX_RELOC_PASS;
            gNdsFighterMarioFoxSetupMask |=
                NDS_FIGHTER_MARIOFOX_SETUP_FILES;
        }
    }
    return TRUE;
}

static void ndsRelocSwapS16Pair(s16 *a, s16 *b)
{
    s16 tmp = *a;

    *a = *b;
    *b = tmp;
}

/* Undo the loader's word swap over a run of s16 that carries no other type.
 *
 * ndsRelocApplyWordByteSwap byte-swaps every 32-bit word of a reloc file. That
 * is right for u32, f32 and pointers, and it EXCHANGES THE POSITIONS of any two
 * s16 sharing a word. Every other struct in this file undoes that with explicit
 * ndsRelocSwapS16Pair calls (Sprite, Bitmap, MObjSub); this is the same
 * operation expressed over a byte range, because MPGroundData's second s16 run
 * is long and its pairing is not obvious from the field names. */
static void ndsRelocSwapWordS16Halves(void *base, u32 begin, u32 end)
{
    u32 offset;

    for (offset = begin; offset < end; offset += (u32)sizeof(u32))
    {
        s16 *pair = (s16 *)((u8 *)base + offset);

        ndsRelocSwapS16Pair(&pair[0], &pair[1]);
    }
}

/* The u8 twin of the s16 helper: reverse the four bytes of every word in
 * [begin, end), which is the exact inverse of the loader's blanket u32 swap
 * for byte-lane fields (u8 flags, packed RGB). */
static void ndsRelocReverseWordBytes(void *base, u32 begin, u32 end)
{
    u32 offset;

    for (offset = begin; offset < end; offset += (u32)sizeof(u32))
    {
        u8 *b = (u8 *)base + offset;
        u8 t0 = b[0];
        u8 t1 = b[1];

        b[0] = b[3];
        b[1] = b[2];
        b[2] = t1;
        b[3] = t0;
    }
}

/* WPAttributes s16 runs are scrambled by the loader's blanket u32 word swap,
 * exactly the way the fighter attributes' u16 pairs were: each u32 word in the
 * O2R payload holds TWO s16 lanes, and the byte swap reverses their order, so
 * a plain C struct read (which wpManagerMakeWeapon does at wpmanager.c:285-288
 * and itmanager.c does for items) sees every s16 pair swapped. The fireball is
 * the visible casualty -- map_coll {top 50, center 0, bottom -50, width 50}
 * (204_MarioSpecial1.c:33) reads as {0, 50, +50, -50}, so the floor clamp
 * rests the weapon translate at floor_y - 50 instead of floor_y + 50 and the
 * quad sinks a full collision box into the ground before bouncing.
 *
 * The runs that need un-swapping are the ONLY s16-carrying fields in the
 * struct, both 4-aligned, both pure-s16: attack_offsets[2] (6 s16, 12 bytes at
 * offset 0x10) and map_coll top/center/bottom/width (4 s16 at 0x1C). The
 * bitfield words from 0x24 on are single-u32 and survive the word swap
 * correctly -- swapping THEIR halves would corrupt size/angle/damage, which
 * is why the range stops at map_coll_width. */
static void ndsRelocNormalizeWeaponAttributes(WPAttributes *attr)
{
    _Static_assert((offsetof(WPAttributes, attack_offsets) %
                    sizeof(u32)) == 0u,
                   "WPAttributes attack_offsets must start on a word");
    _Static_assert((offsetof(WPAttributes, map_coll_width) -
                    offsetof(WPAttributes, attack_offsets)) == 18u,
                   "WPAttributes s16 run must be attack_offsets[2] plus "
                   "map_coll top/center/bottom/width");

    if (attr == NULL)
    {
        return;
    }
    ndsRelocSwapWordS16Halves(
        attr,
        (u32)offsetof(WPAttributes, attack_offsets),
        (u32)offsetof(WPAttributes, map_coll_width) + (u32)sizeof(s16));
}

/* Engagement + integrity check after the swap. The map_coll box is the
 * gameplay-visible value -- a wrong box makes projectiles sink into floors or
 * float over them -- so it is pinned to the source's literal values
 * (204_MarioSpecial1.c:33 for the fireball, 210_FoxSpecial1.c:27 for the
 * blaster) rather than guessed. attack_offsets are all zero for the P1 weapons
 * (fireball, blaster), so they are pinned too: a weapon whose offsets are
 * non-zero will fail here until this table grows, which is the intended
 * fail-closed behaviour for an asset this loader has never examined. */
static s32 ndsRelocWeaponAttributesMatchSource(u32 asset_id,
                                               const WPAttributes *attr)
{
    s32 i;

    if (attr == NULL)
    {
        return FALSE;
    }
    for (i = 0; i < 2; i++)
    {
        if ((attr->attack_offsets[i].x != 0) ||
            (attr->attack_offsets[i].y != 0) ||
            (attr->attack_offsets[i].z != 0))
        {
            return FALSE;
        }
    }
    if (asset_id == NDS_RELOC_ASSET_MARIO_SPECIAL1)
    {
        return (attr->map_coll_top == 50) &&
               (attr->map_coll_center == 0) &&
               (attr->map_coll_bottom == -50) &&
               (attr->map_coll_width == 50);
    }
    if (asset_id == NDS_RELOC_ASSET_FOX_SPECIAL1)
    {
        return (attr->map_coll_top == 10) &&
               (attr->map_coll_center == 0) &&
               (attr->map_coll_bottom == -10) &&
               (attr->map_coll_width == 10);
    }
    if (asset_id == NDS_RELOC_ASSET_SAMUS_MAIN)
    {
        /* 217_SamusMain.c bytes 0x28..0x2f, viewed from Bomb's 0x0c base. */
        return (attr->map_coll_top == 75) &&
               (attr->map_coll_center == 0) &&
               (attr->map_coll_bottom == -75) &&
               (attr->map_coll_width == 75);
    }
    if (asset_id == NDS_RELOC_ASSET_SAMUS_SPECIAL1)
    {
        /* 218_SamusSpecial1.c: Charge Shot's source collision extents are
         * level-dependent and installed by wpSamusChargeShotLaunch; the base
         * WPAttributes carries a zero map box. */
        return (attr->map_coll_top == 0) &&
               (attr->map_coll_center == 0) &&
               (attr->map_coll_bottom == 0) &&
               (attr->map_coll_width == 0);
    }
#if NDS_P2_LINK
    if ((asset_id == NDS_RELOC_ASSET_LINK_MAIN) ||
        (asset_id == NDS_RELOC_ASSET_LINK_SPECIAL1))
    {
        /* LinkMain Spin Attack and LinkSpecial1 Boomerang both carry the
         * source {150, 0, -150, 150} map box and zero attack offsets. */
        return (attr->map_coll_top == 150) &&
               (attr->map_coll_center == 0) &&
               (attr->map_coll_bottom == -150) &&
               (attr->map_coll_width == 150);
    }
#endif
    return TRUE;
}

/* Record one load-time WPAttributes normalization so the make seam below
 * does not swap it back. Every fighter-file weapon the loader still converts
 * (Samus/Link mains inline above, Pikachu/Yoshi/Kirby/Ness through the
 * helper below) funnels through here. */
static s32 ndsRelocMarkWeaponAttributesNormalized(
    const NDSRelocLoadedFile *loaded, u32 attr_offset)
{
    u32 i;

    if (loaded == NULL)
    {
        return FALSE;
    }
    for (i = 0u; i < sNdsRelocNormalizedWeaponAttrCount; i++)
    {
        if ((sNdsRelocNormalizedWeaponAttrs[i].asset_id ==
             loaded->asset_id) &&
            (sNdsRelocNormalizedWeaponAttrs[i].offset == attr_offset) &&
            (sNdsRelocNormalizedWeaponAttrs[i].owner_generation ==
             loaded->owner_generation))
        {
            return TRUE;
        }
    }
    if (sNdsRelocNormalizedWeaponAttrCount >=
        NDS_RELOC_NORMALIZED_WEAPON_ATTR_CAPACITY)
    {
        ndsRelocRecordExternalFixupFail(loaded->asset_id);
        return FALSE;
    }
    sNdsRelocNormalizedWeaponAttrs[sNdsRelocNormalizedWeaponAttrCount]
        .asset_id = loaded->asset_id;
    sNdsRelocNormalizedWeaponAttrs[sNdsRelocNormalizedWeaponAttrCount].offset =
        attr_offset;
    sNdsRelocNormalizedWeaponAttrs[sNdsRelocNormalizedWeaponAttrCount]
        .owner_generation = loaded->owner_generation;
    sNdsRelocNormalizedWeaponAttrCount++;
    return TRUE;
}

/* The single WPAttributes seam. Every weapon in the game is made by
 * wpManagerMakeWeapon (decomp src/wp/wpmanager.c:87), which reads the s16
 * run at :196 and :285-288, so normalizing the descriptor's struct here, in
 * place, covers every weapon-owning file at once -- including the ones no
 * per-file table ever listed. Pointers outside any loaded file are
 * statically linked descriptors and are left alone. Exactly-once per
 * resident struct comes from the record above: a second make finds its
 * (asset id, offset, generation) entry and returns without touching the
 * bytes, so the swap can never run twice on the same struct. The
 * source-literal pins stay live for the single-attr files the old table
 * checked. */
void ndsRelocEnsureWeaponAttributesNormalized(void *attr_ptr)
{
    NDSRelocLoadedFile *loaded;
    WPAttributes *attr = (WPAttributes *)attr_ptr;
    u32 attr_offset;
    u32 i;

    if (attr == NULL)
    {
        return;
    }
    loaded = ndsRelocFindLoadedFileContaining(attr, sizeof(WPAttributes));
    if (loaded == NULL)
    {
        return;
    }
    attr_offset = (u32)((const u8 *)attr - (const u8 *)loaded->data);
    for (i = 0u; i < sNdsRelocNormalizedWeaponAttrCount; i++)
    {
        if ((sNdsRelocNormalizedWeaponAttrs[i].asset_id ==
             loaded->asset_id) &&
            (sNdsRelocNormalizedWeaponAttrs[i].offset == attr_offset) &&
            (sNdsRelocNormalizedWeaponAttrs[i].owner_generation ==
             loaded->owner_generation))
        {
            return;
        }
    }
    /* Record first: with the record full the swap must not run, or every
     * later make would toggle the struct between corrupt and correct. */
    if (ndsRelocMarkWeaponAttributesNormalized(loaded, attr_offset) == FALSE)
    {
        return;
    }
    ndsRelocNormalizeWeaponAttributes(attr);
    if ((((loaded->asset_id == NDS_RELOC_ASSET_MARIO_SPECIAL1) ||
          (loaded->asset_id == NDS_RELOC_ASSET_FOX_SPECIAL1))
         || (loaded->asset_id == NDS_RELOC_ASSET_SAMUS_SPECIAL1)
#if NDS_P2_LINK
         || (loaded->asset_id == NDS_RELOC_ASSET_LINK_SPECIAL1)
#endif
        ) &&
        (ndsRelocWeaponAttributesMatchSource(loaded->asset_id, attr) == FALSE))
    {
        ndsRelocRecordExternalFixupFail(loaded->asset_id);
    }
}

/* One Pikachu WPAttributes: normalize its s16 run (once per file, the caller
 * owns the file-wide flag ordering) and pin the source literals that decide
 * gameplay -- the two attack offsets the hits ride on and the map-collision
 * box (243_PikachuMain.c bytes 0x1c/0x50: head (0,225,0)/(0,0,0), trail
 * (0,240,0)/(0,-240,0)).
 * The generic ndsRelocWeaponAttributesMatchSource deliberately refuses any
 * non-zero attack offset it has not examined; Pikachu is the first landed
 * owner whose weapons carry them (243_PikachuMain.c bytes 0x1c/0x50,
 * 244_PikachuSpecial1.c:38/:76), so each struct is pinned here instead. */
static s32 ndsRelocNormalizePikachuWeaponAttributes(
    NDSRelocLoadedFile *loaded, u32 attr_offset, s16 attack0_y, s16 attack1_y,
    s16 map_top, s16 map_center, s16 map_bottom, s16 map_width)
{
    WPAttributes *attr;

    if (ndsRelocRangeInLoadedFile(
            loaded, attr_offset, sizeof(WPAttributes)) == FALSE)
    {
        return FALSE;
    }
    attr = (WPAttributes *)((u8 *)loaded->data + attr_offset);
    if (loaded->format_fixups_applied == FALSE)
    {
        ndsRelocNormalizeWeaponAttributes(attr);
        ndsRelocMarkWeaponAttributesNormalized(loaded, attr_offset);
    }
    return (attr->attack_offsets[0].x == 0) &&
           (attr->attack_offsets[0].y == attack0_y) &&
           (attr->attack_offsets[0].z == 0) &&
           (attr->attack_offsets[1].x == 0) &&
           (attr->attack_offsets[1].y == attack1_y) &&
           (attr->attack_offsets[1].z == 0) &&
           (attr->map_coll_top == map_top) &&
           (attr->map_coll_center == map_center) &&
           (attr->map_coll_bottom == map_bottom) &&
           (attr->map_coll_width == map_width);
}

/* MPGroundData has TWO s16-only runs, both 4-aligned: the camera/map bounds
 * ahead of bgm_id, and everything from alt_warning to the end of the struct.
 *
 * This used to be a `if (min > max) swap` heuristic over eight named pairs, and
 * it was wrong twice over.
 *
 * It never touched `alt_warning` at all -- alt_warning is the FIRST s16 of the
 * second run, so the word swap left it holding camera_bound_team_top. Dream
 * Land therefore read +3500 where 255_GRPupupuMap.c:59 says -2900. FGM 153
 * AltitudeWarn fires on `pos_prev.y >= alt_warning && topn.y < alt_warning`
 * (decomp ftmain.c:1817) -- the bottom blast-zone whistle -- so at +3500 it
 * played every time a fighter fell back down through y = 3500, which is exactly
 * what a big upward knockback does. That is the owner's "FGM 153 AltitudeWarn
 * plays at wrong trigger", and the trigger was never the defect: the threshold
 * was.
 *
 * And because alt_warning shifts the rest of the run by one s16, the team
 * fields do not pair top-with-bottom inside a word -- alt_warning pairs with
 * team_top, team_bottom with team_right, team_left with map_team_top, and so
 * on. The heuristic was comparing fields that never shared a word, so every
 * team bound was left scrambled too.
 *
 * Swapping the halves of each word is the exact inverse and needs no heuristic.
 * The static asserts pin the two runs; if a field is ever added between them
 * the build fails rather than silently reverting to the old symptom. */
static void ndsRelocNormalizeGroundDataBounds(MPGroundData *ground_data)
{
    _Static_assert((offsetof(MPGroundData, camera_bound_top) %
                    sizeof(u32)) == 0u,
                   "MPGroundData bounds run must start on a word");
    _Static_assert((offsetof(MPGroundData, bgm_id) -
                    offsetof(MPGroundData, camera_bound_top)) == 16u,
                   "MPGroundData bounds run must be eight s16");
    _Static_assert((offsetof(MPGroundData, alt_warning) %
                    sizeof(u32)) == 0u,
                   "MPGroundData alt_warning run must start on a word");
    _Static_assert((sizeof(MPGroundData) -
                    offsetof(MPGroundData, alt_warning)) == 32u,
                   "MPGroundData alt_warning run must reach the struct end");

    if (ground_data == NULL)
    {
        return;
    }
    ndsRelocSwapWordS16Halves(ground_data,
                              (u32)offsetof(MPGroundData, camera_bound_top),
                              (u32)offsetof(MPGroundData, bgm_id));
    ndsRelocSwapWordS16Halves(ground_data,
                              (u32)offsetof(MPGroundData, alt_warning),
                              (u32)sizeof(MPGroundData));
    /* The header's u8 fields share their words with padding and each other,
     * so the blanket swap leaves them in the wrong lanes too. layer_mask
     * (bit N = layer N draws through the Sec callback, grdisplay.c:193) read
     * 0 on every stage because its byte landed at +3: every Sec layer ran the
     * Pri callback, which is why the native stage adapter's proc match
     * declined Sector Z, Planet Zebes, Hyrule, Saffron City and Mushroom
     * Kingdom while the mask-0 stages admitted (2026-09-07). fog_color,
     * fog_alpha and the four emblem colours are the same shape; the
     * wallpaper pointer between them is a whole word and is skipped.
     *
     * `unused` is a fifth emblem colour in all but name: a CPU's damage-meter
     * emblem reads emblem_colors[GMCOMMON_PLAYERS_MAX] (ifcommon.c:946-950,
     * color 4 = nSCBattlePlayerColorCP), its first three bytes in N64 order --
     * 0xDCDCDC00 on Dream Land is light grey. Nothing reads it as an s32, so
     * it stays in N64 byte order with the colours; native order made the CPU
     * emblem (0x00, 0xDC, 0xDC), cyan. */
    _Static_assert((offsetof(MPGroundData, layer_mask) % sizeof(u32)) == 0u,
                   "MPGroundData layer_mask must start a word");
    _Static_assert((offsetof(MPGroundData, fog_color) % sizeof(u32)) == 0u,
                   "MPGroundData fog_color must start a word");
    _Static_assert((offsetof(MPGroundData, unused) -
                    offsetof(MPGroundData, fog_color)) == 16u,
                   "MPGroundData colour run must be four words");
    ndsRelocReverseWordBytes(ground_data,
                             (u32)offsetof(MPGroundData, layer_mask),
                             (u32)offsetof(MPGroundData, layer_mask) + 4u);
    ndsRelocReverseWordBytes(ground_data,
                             (u32)offsetof(MPGroundData, fog_color),
                             (u32)offsetof(MPGroundData, unused) + 4u);
}

static void ndsRelocNormalizeGroundMapHeader(NDSRelocLoadedFile *loaded,
                                             u32 offset)
{
    if ((loaded == NULL) ||
        (loaded->data == NULL) ||
        ((offset + sizeof(MPGroundData)) > loaded->data_size))
    {
        return;
    }
    ndsRelocNormalizeGroundDataBounds(
        (MPGroundData *)((u8 *)loaded->data + offset));
}

/* EVERY stage's MPGroundData, not three of them.
 *
 * This was three hardcoded arms -- Dream Land, Hyrule Castle, Mushroom
 * Kingdom -- so every other stage's camera bounds stayed in their ROM word
 * order. gmCameraSetBoundsPosition (gm/gmcamera.c:102) is a bare `while
 * (TRUE)` that clamps the camera against those bounds and leaves only when
 * the position is inside them, so a left bound sitting past its right bound
 * oscillates forever: no exception, VBlank alive, first battle frame, and
 * stage-specific. That is what froze Peach's Castle, and Yoshi's Island,
 * Congo Jungle, Planet Zebes, Saffron City and Sector Z would each have
 * frozen the same way in turn.
 *
 * The offset was never the variable. All nine map files carry their header at
 * 0x14 (decomp reloc_data.us.h:3845, 3898, 3914, 3920, 3932, 3943, 3969,
 * 3998, 4030), so the list of assets is the whole content of this function
 * and a table is the honest shape for it. */
/* One row per ground the game has (decomp mp/mpcollision.c:26-67 order),
 * each with ITS OWN header offset: the nine VS maps carry the MPGroundData
 * at 0x14, the 1P arenas too, but Explain, Race, Final Destination and the
 * 24 bonus boards carry it at 0x0 (tools/reloc_data_symbols.us.txt, the
 * ll*MapMapHeader rows). Numeric ids, so a map not yet staged still has
 * its row and lands by staging alone. */
static const struct {
    u32 asset_id;
    u32 header;
} sNdsRelocGroundMapAssets[] = {
    { 0x103, 0x14 }, /* nGRKindCastle */
    { 0x106, 0x14 }, /* nGRKindSector */
    { 0x105, 0x14 }, /* nGRKindJungle */
    { 0x101, 0x14 }, /* nGRKindZebes */
    { 0x109, 0x14 }, /* nGRKindHyrule */
    { 0x107, 0x14 }, /* nGRKindYoster */
    { 0xff, 0x14 }, /* nGRKindPupupu */
    { 0x108, 0x14 }, /* nGRKindYamabuki */
    { 0x104, 0x14 }, /* nGRKindInishie */
    { 0x100, 0x14 }, /* nGRKindPupupuSmall */
    { 0x102, 0x14 }, /* nGRKindPupupuNew */
    { 0x10b, 0x0 }, /* nGRKindExplain */
    { 0x10e, 0x14 }, /* nGRKindYosterSmall */
    { 0x10d, 0x14 }, /* nGRKindMetal */
    { 0x10c, 0x14 }, /* nGRKindZako */
    { 0x127, 0x0 }, /* nGRKindBonus3 */
    { 0x10a, 0x0 }, /* nGRKindLast */
    { 0x10f, 0x0 }, /* nGRKindBonus1Mario */
    { 0x110, 0x0 }, /* nGRKindBonus1Fox */
    { 0x111, 0x0 }, /* nGRKindBonus1Donkey */
    { 0x112, 0x0 }, /* nGRKindBonus1Samus */
    { 0x113, 0x0 }, /* nGRKindBonus1Luigi */
    { 0x114, 0x0 }, /* nGRKindBonus1Link */
    { 0x115, 0x0 }, /* nGRKindBonus1Yoshi */
    { 0x116, 0x0 }, /* nGRKindBonus1Captain */
    { 0x117, 0x0 }, /* nGRKindBonus1Kirby */
    { 0x118, 0x0 }, /* nGRKindBonus1Pikachu */
    { 0x119, 0x0 }, /* nGRKindBonus1Purin */
    { 0x11a, 0x0 }, /* nGRKindBonus1Ness */
    { 0x11b, 0x0 }, /* nGRKindBonus2Mario */
    { 0x11c, 0x0 }, /* nGRKindBonus2Fox */
    { 0x11d, 0x0 }, /* nGRKindBonus2Donkey */
    { 0x11e, 0x0 }, /* nGRKindBonus2Samus */
    { 0x11f, 0x0 }, /* nGRKindBonus2Luigi */
    { 0x120, 0x0 }, /* nGRKindBonus2Link */
    { 0x121, 0x0 }, /* nGRKindBonus2Yoshi */
    { 0x122, 0x0 }, /* nGRKindBonus2Captain */
    { 0x123, 0x0 }, /* nGRKindBonus2Kirby */
    { 0x124, 0x0 }, /* nGRKindBonus2Pikachu */
    { 0x125, 0x0 }, /* nGRKindBonus2Purin */
    { 0x126, 0x0 }, /* nGRKindBonus2Ness */
#if NDS_P5
    /* P5: the Remix stages' maps (scripts/p5/stage_generate.py). Without its
     * row First Destination's header stayed in ROM word order: layer_mask
     * read 0, so the stage adapter declined its Sec layer and drew nothing,
     * and the scrambled blast-zone bounds KO'd every fighter on landing. */
#define NDS_P5_GROUND_MAP_ROW(g, cname, name, map_file, map_offset) \
    { (map_file), (map_offset) },
    NDS_P5_NATIVE_STAGE_ROWS(NDS_P5_GROUND_MAP_ROW)
#undef NDS_P5_GROUND_MAP_ROW
#endif
};

/* The witness this cost four emulator runs for want of. A stage whose bounds
 * come out inconsistent still freezes inside the source's own clamp, which no
 * port-side guard may refuse; what it must not do is freeze SILENTLY. */
__attribute__((used)) volatile u32 gNdsRelocGroundBoundsCount;
__attribute__((used)) volatile u32 gNdsRelocGroundBoundsRejectCount;
__attribute__((used)) volatile u32 gNdsRelocGroundBoundsRejectAsset;
__attribute__((used)) volatile s32 gNdsRelocGroundBoundsLastLeft;
__attribute__((used)) volatile s32 gNdsRelocGroundBoundsLastRight;
__attribute__((used)) volatile s32 gNdsRelocGroundBoundsLastBottom;
__attribute__((used)) volatile s32 gNdsRelocGroundBoundsLastTop;

static void ndsRelocNormalizeGroundMapAsset(NDSRelocLoadedFile *loaded)
{
    u32 i;

    if (loaded == NULL)
    {
        return;
    }
    for (i = 0u; i < ARRAY_COUNT(sNdsRelocGroundMapAssets); i++)
    {
        const MPGroundData *ground_data;

        if (loaded->asset_id != sNdsRelocGroundMapAssets[i].asset_id)
        {
            continue;
        }
        ndsRelocNormalizeGroundMapHeader(loaded,
                                         sNdsRelocGroundMapAssets[i].header);
        if ((sNdsRelocGroundMapAssets[i].header + sizeof(MPGroundData)) >
            loaded->data_size)
        {
            return;
        }
        ground_data = (const MPGroundData *)((const u8 *)loaded->data +
                                             sNdsRelocGroundMapAssets[i].header);
        NDS_DIAG(gNdsRelocGroundBoundsCount++);
        gNdsRelocGroundBoundsLastLeft = ground_data->camera_bound_left;
        gNdsRelocGroundBoundsLastRight = ground_data->camera_bound_right;
        gNdsRelocGroundBoundsLastBottom = ground_data->camera_bound_bottom;
        gNdsRelocGroundBoundsLastTop = ground_data->camera_bound_top;
        if ((ground_data->camera_bound_left >=
             ground_data->camera_bound_right) ||
            (ground_data->camera_bound_bottom >=
             ground_data->camera_bound_top))
        {
            NDS_DIAG(gNdsRelocGroundBoundsRejectCount++);
            gNdsRelocGroundBoundsRejectAsset = loaded->asset_id;
        }
        return;
    }
}

static void ndsRelocSwapSpriteAttrZDepth(Sprite *sprite)
{
    u16 attr = sprite->attr;

    sprite->attr = (u16)sprite->zdepth;
    sprite->zdepth = (s16)attr;
}

static void ndsRelocReverseSpriteColorBytes(Sprite *sprite)
{
    u8 red = sprite->red;
    u8 green = sprite->green;
    u8 blue = sprite->blue;
    u8 alpha = sprite->alpha;

    sprite->red = alpha;
    sprite->green = blue;
    sprite->blue = green;
    sprite->alpha = red;
}

static void ndsRelocNormalizeSpriteHeaderFields(Sprite *sprite, u8 bmfmt,
                                                u8 bmsiz)
{
    ndsRelocSwapS16Pair(&sprite->x, &sprite->y);
    ndsRelocSwapS16Pair(&sprite->width, &sprite->height);
    ndsRelocSwapS16Pair(&sprite->expx, &sprite->expy);
    ndsRelocSwapSpriteAttrZDepth(sprite);
    ndsRelocReverseSpriteColorBytes(sprite);
    ndsRelocSwapS16Pair(&sprite->startTLUT, &sprite->nTLUT);
    ndsRelocSwapS16Pair(&sprite->istart, &sprite->istep);
    ndsRelocSwapS16Pair(&sprite->nbitmaps, &sprite->ndisplist);
    ndsRelocSwapS16Pair(&sprite->bmheight, &sprite->bmHreal);

    /* The blanket u32 endian pass shifts these 8-bit format fields into the
     * padding before the bitmap pointer. Keep correction tied to known Sprite
     * manifests rather than guessing across every relocated resource. */
    sprite->bmfmt = bmfmt;
    sprite->bmsiz = bmsiz;
}

static s32 ndsRelocNormalizeSpriteBitmapTable(NDSRelocLoadedFile *loaded,
                                               Sprite *sprite,
                                               u32 bitmap_count)
{
    Bitmap *bitmap;
    u32 i;

    bitmap = sprite->bitmap;
    if (ndsRelocPointerRangeInLoadedFile(
            loaded, bitmap, sizeof(Bitmap) * bitmap_count) == FALSE)
    {
        return FALSE;
    }

    for (i = 0; i < bitmap_count; i++)
    {
        ndsRelocSwapS16Pair(&bitmap[i].width, &bitmap[i].width_img);
        ndsRelocSwapS16Pair(&bitmap[i].s, &bitmap[i].t);
        ndsRelocSwapS16Pair(&bitmap[i].actualHeight, &bitmap[i].LUToffset);
    }
    return TRUE;
}

/* One manifest row against a loaded file: TRUE when it does not apply or
 * was normalized (or already native), FALSE on a refused signature. */
static s32 ndsRelocNormalizeSpriteDesc(
    NDSRelocLoadedFile *loaded, const NDSRelocSpriteNormalizeDesc *desc)
{
    Sprite *sprite;
    u32 display_list_words;
    u32 sprite_offset = desc->offset;

    if (desc->asset_id != loaded->asset_id)
    {
        return TRUE;
    }
#if (NDS_P2_1P_GAME || NDS_P2_MENU_SHELL) || NDS_P2_COMPACT_BATTLE_FIGHTERS
    if ((loaded->reserved[0] != 0u) &&
        (ndsPreviewFileOffset(loaded, desc->offset, sizeof(Sprite),
                              &sprite_offset) == FALSE))
    {
        return TRUE; /* This descriptor belongs to an omitted source span. */
    }
#endif
    /* Through Sprite.rsp_dl_next: nothing here reads or swaps frac_s/frac_t,
     * and Smash Remix packs some emblem Sprites at the very end of a file
     * without that last word (Ganondorf's model 0x8B0: 64 of 68 bytes). */
    if (ndsRelocRangeInLoadedFile(loaded, sprite_offset,
                                  offsetof(Sprite, frac_s)) == FALSE)
    {
        return FALSE;
    }

    sprite = (Sprite *)((u8 *)loaded->data + sprite_offset);
    /* libultra generates one sprite's display list as a fixed 24-word frame
     * plus 12 words per bitmap, so `ndisplist` is DERIVABLE and does not need
     * a per-offset table. This used to be `36` with three hardcoded
     * exceptions (84 for 0x4d78, 96 for 0xa730, 48 for 0xc370); read out of
     * the extracted asset, all six known sprites fit `12n + 24` exactly --
     * 1 bitmap 36, 2 -> 48, 3 -> 60, 4 -> 72, 5 -> 84, 6 -> 96. Keeping the
     * table would have meant a fourth, fifth and sixth exception for the
     * GAME SET / TIME UP letters below, each of which is a 2-, 3- or
     * 4-bitmap sprite. The check this feeds is still fail-closed: a sprite
     * whose raw `ndisplist` disagrees with the formula is refused, not
     * normalized. */
    display_list_words = (12u * desc->bitmap_count) + 24u;
    if (((u32)(u16)sprite->width == desc->width) &&
        ((u32)(u16)sprite->height == desc->height) &&
        ((u32)(u16)sprite->nbitmaps == desc->bitmap_count) &&
        (sprite->bmfmt == desc->bmfmt) &&
        (sprite->bmsiz == desc->bmsiz))
    {
        return TRUE;
    }

    /* The blanket u32 endian pass exchanges each adjacent halfword.
     * Validate that exact raw signature, including libultra's 36-word
     * generated display list, before touching a known manifest entry. */
    if (((u32)(u16)sprite->width != desc->height) ||
        ((u32)(u16)sprite->height != desc->width) ||
        ((u32)(u16)sprite->nbitmaps != display_list_words) ||
        ((u32)(u16)sprite->ndisplist != desc->bitmap_count) ||
        (ndsRelocPointerRangeInLoadedFile(
            loaded, sprite->bitmap,
            sizeof(Bitmap) * desc->bitmap_count) == FALSE))
    {
        return FALSE;
    }

    ndsRelocNormalizeSpriteHeaderFields(sprite, desc->bmfmt,
                                        desc->bmsiz);
    if (((u32)(u16)sprite->width != desc->width) ||
        ((u32)(u16)sprite->height != desc->height) ||
        ((u32)(u16)sprite->nbitmaps != desc->bitmap_count) ||
        (ndsRelocNormalizeSpriteBitmapTable(
            loaded, sprite, desc->bitmap_count) == FALSE))
    {
        return FALSE;
    }
    return TRUE;
}

static s32 ndsRelocNormalizeBattleInterfaceSprites(
    NDSRelocLoadedFile *loaded)
{
    u32 i;

    if (loaded == NULL)
    {
        return FALSE;
    }

    for (i = 0; i < ARRAY_COUNT(sNdsBattleInterfaceSpriteDescs); i++)
    {
        if (ndsRelocNormalizeSpriteDesc(
                loaded, &sNdsBattleInterfaceSpriteDescs[i]) == FALSE)
        {
            return FALSE;
        }
    }
    /* Each P4 content's FTSprites sprites (stock icon, emblem), generated
     * by scripts/p4/generate_p4_fighter.py from its donor files. */
    if (loaded->asset_id >= 0x854u)
    {
        u32 c;

        for (c = 1u; c < NDS_P4_CONTENT_LIMIT; c++)
        {
            const NDSP4Fighter *p4 = ndsP4Fighter(c);

            if (p4 == NULL)
            {
                continue;
            }
            for (i = 0u; i < *p4->sprite_count; i++)
            {
                const NDSP4SpriteDesc *row = &p4->sprites[i];
                NDSRelocSpriteNormalizeDesc desc;

                desc.asset_id = row->file_id;
                desc.offset = row->offset;
                desc.width = row->width;
                desc.height = row->height;
                desc.bitmap_count = row->bitmap_count;
                desc.bmfmt = row->bmfmt;
                desc.bmsiz = row->bmsiz;
                if (ndsRelocNormalizeSpriteDesc(loaded, &desc) == FALSE)
                {
                    return FALSE;
                }
            }
        }
    }
    {
        /* P7 C1: the contents' congratulations plates and intro names. */
        static const NDSRelocSpriteNormalizeDesc kOnePArt[] = {
#define NDS_RELOC_ONEP_ART_ROW(file, offset, width, height, bitmaps, fmt, siz) \
            { (file), (offset), (width), (height), (bitmaps), (fmt), (siz) },
            NDS_P4_ONEP_ART_SPRITE_ROWS(NDS_RELOC_ONEP_ART_ROW)
#undef NDS_RELOC_ONEP_ART_ROW
            { 0u, 0u, 0u, 0u, 0u, 0u, 0u }
        };

        for (i = 0u; kOnePArt[i].asset_id != 0u; i++)
        {
            if (ndsRelocNormalizeSpriteDesc(loaded, &kOnePArt[i]) == FALSE)
            {
                return FALSE;
            }
        }
    }
    if (loaded->asset_id == NDS_RELOC_ASSET_IF_COMMON_GAME_STATUS)
    {
        if (ndsIFCommonNativeOamPrepareGameStatus(
                loaded->data, loaded->data_size) == FALSE)
        {
            /* Preparation is an optimization seam.  The relocated source
             * asset remains valid and the exact BG3 SObj compositor is the
             * required fallback if OBJ capacity or conversion rejects it. */
        }
    }
    return TRUE;
}

/* Every stage wallpaper container (StageDreamLand, StageCastle, StageZebes,
 * StageJungle, StageYoshi, StagePokemon, StageSector, the mislabelled
 * StageHyrule/Yamabuki/Inishie/Last/Metal wallpaper files and the movie
 * room wallpaper Castle borrows) is the same 159,008-byte layout with its
 * Sprite at 0x26c88 (include/reloc_data.h). Until 2026-09-06 only Dream
 * Land's file was normalized here, so the other eight VS stages kept their
 * Sprite header in the swapped halfword lanes, failed the shape gate in
 * ndsSObjGetOpaqueWallpaperCache, and drew no background at all. */
static u32 ndsRelocIsStageWallpaperAsset(u32 asset_id)
{
    switch (asset_id)
    {
    case NDS_RELOC_ASSET_STAGE_DREAM_LAND:
    case NDS_RELOC_ASSET_STAGE_CASTLE:
    case NDS_RELOC_ASSET_STAGE_ZEBES:
    case NDS_RELOC_ASSET_STAGE_HYRULE_WALLPAPER:
    case NDS_RELOC_ASSET_STAGE_JUNGLE:
    case NDS_RELOC_ASSET_STAGE_YOSHI:
    case NDS_RELOC_ASSET_STAGE_POKEMON:
    case NDS_RELOC_ASSET_STAGE_SECTOR:
    case NDS_RELOC_ASSET_MV_OPENING_ROOM_WALLPAPER:
#if NDS_P2_1P_GAME
    case NDS_RELOC_ASSET_STAGE_LAST_WALLPAPER:
    case NDS_RELOC_ASSET_STAGE_METAL_WALLPAPER:
    case NDS_RELOC_ASSET_STAGE_ZAKO_WALLPAPER:
    case NDS_RELOC_ASSET_STAGE_BONUS_WALLPAPER:
#endif
        return TRUE;
    default:
        return FALSE;
    }
}

#if defined(NDS_P2_COMPACT_GROUND_MAPS) && NDS_P2_COMPACT_GROUND_MAPS
/* A compact ground map's appended Sprite and Bitmap table took the same word
 * swap the container's did, so they need the same lane fix and get the same
 * "already normalized" test. TRUE when `loaded` is a compact map at all. */
static __attribute__((noinline)) s32 ndsRelocNormalizeCompactGroundMapSprite(
    NDSRelocLoadedFile *loaded)
{
    Sprite *sprite = ndsRelocCompactGroundMapSprite(loaded);
    s32 index;

    if (sprite == NULL)
    {
        return FALSE;
    }
    if (!((sprite->width == 300) && (sprite->height == 220) &&
          (sprite->nbitmaps == 44) &&
          (sprite->bmfmt == G_IM_FMT_RGBA) &&
          (sprite->bmsiz == G_IM_SIZ_16b)))
    {
        ndsRelocNormalizeSpriteHeaderFields(sprite, G_IM_FMT_RGBA,
                                            G_IM_SIZ_16b);
        (void)ndsRelocNormalizeSpriteBitmapTable(loaded, sprite, 44u);
        gNdsRelocCompactGroundMapCount++;
    }
    index = ndsRelocCompactGroundMapIndex(loaded->asset_id);
    if ((index >= 0) &&
        (ndsRelocPointerRangeInLoadedFile(loaded, sprite->bitmap,
                                          sizeof(Bitmap)) != FALSE))
    {
        sNdsRelocCompactGroundMapActive.scene_generation =
            sNdsRelocSceneGeneration;
        sNdsRelocCompactGroundMapActive.map_asset_id = loaded->asset_id;
        sNdsRelocCompactGroundMapActive.bitmap_offset =
            (u32)((const u8 *)sprite->bitmap - (const u8 *)loaded->data);
        sNdsRelocCompactGroundMapActive.wallpaper_asset_id =
            sNdsRelocCompactGroundMaps[index].wallpaper_asset_id;
    }
    return TRUE;
}
#endif

static void ndsRelocNormalizeStageDreamLandSprite(
    NDSRelocLoadedFile *loaded)
{
    Sprite *sprite;

#if defined(NDS_P2_COMPACT_GROUND_MAPS) && NDS_P2_COMPACT_GROUND_MAPS
    if (ndsRelocNormalizeCompactGroundMapSprite(loaded) != FALSE)
    {
        return;
    }
#endif
    if ((loaded == NULL) ||
        (ndsRelocIsStageWallpaperAsset(loaded->asset_id) == FALSE) ||
        (ndsRelocRangeInLoadedFile(
            loaded, NDS_RELOC_SYMBOL_STAGE_DREAM_LAND_SPRITE,
            sizeof(Sprite)) == FALSE))
    {
        return;
    }

    sprite = (Sprite *)((u8 *)loaded->data +
                        NDS_RELOC_SYMBOL_STAGE_DREAM_LAND_SPRITE);
    if ((sprite->width == 300) && (sprite->height == 220) &&
        (sprite->nbitmaps == 44) &&
        (sprite->bmfmt == G_IM_FMT_RGBA) &&
        (sprite->bmsiz == G_IM_SIZ_16b))
    {
        return;
    }

    ndsRelocNormalizeSpriteHeaderFields(sprite, G_IM_FMT_RGBA,
                                        G_IM_SIZ_16b);
    (void)ndsRelocNormalizeSpriteBitmapTable(loaded, sprite, 44u);
}

static void ndsRelocNormalizeVSResultsSprites(NDSRelocLoadedFile *loaded)
{
    u32 i;

    if ((loaded == NULL) ||
        (loaded->asset_id != NDS_RELOC_ASSET_MN_VS_RESULTS))
    {
        return;
    }

    for (i = 0; i < ARRAY_COUNT(sNdsVSResultsSpriteDescs); i++)
    {
        const NDSVSResultsSpriteDesc *desc =
            &sNdsVSResultsSpriteDescs[i];
        Sprite *sprite;

        if (ndsRelocRangeInLoadedFile(loaded, desc->offset,
                                      sizeof(Sprite)) == FALSE)
        {
            continue;
        }
        sprite = (Sprite *)((u8 *)loaded->data + desc->offset);
        if (((u32)(u16)sprite->width == desc->width) &&
            ((u32)(u16)sprite->height == desc->height) &&
            ((u32)(u16)sprite->nbitmaps == desc->bitmap_count) &&
            (sprite->bmfmt == desc->bmfmt) &&
            (sprite->bmsiz == desc->bmsiz))
        {
            continue;
        }
        ndsRelocNormalizeSpriteHeaderFields(sprite, desc->bmfmt,
                                            desc->bmsiz);
        (void)ndsRelocNormalizeSpriteBitmapTable(loaded, sprite,
                                                 desc->bitmap_count);
    }
}

static void ndsRelocNormalizeN64LogoSprite(NDSRelocLoadedFile *loaded)
{
    Sprite *sprite;

    if ((loaded == NULL) ||
        (loaded->asset_id != NDS_RELOC_ASSET_N64_LOGO) ||
        (ndsRelocRangeInLoadedFile(loaded, NDS_RELOC_SYMBOL_N64_LOGO_SPRITE,
                                   sizeof(Sprite)) == FALSE))
    {
        return;
    }

    sprite = (Sprite *)((u8 *)loaded->data +
                        NDS_RELOC_SYMBOL_N64_LOGO_SPRITE);
    if ((sprite->width == 128) &&
        (sprite->height == 108) &&
        (sprite->nbitmaps == 8) &&
        (sprite->bmfmt == G_IM_FMT_RGBA) &&
        (sprite->bmsiz == G_IM_SIZ_16b))
    {
        return;
    }

    ndsRelocNormalizeSpriteHeaderFields(sprite, G_IM_FMT_RGBA, G_IM_SIZ_16b);
    (void)ndsRelocNormalizeSpriteBitmapTable(loaded, sprite, 8u);
}

static void ndsRelocNormalizeOpeningPortraitsSprite(
    NDSRelocLoadedFile *loaded,
    u32 offset,
    u32 expected_width,
    u32 expected_height,
    u8 bmfmt,
    u8 bmsiz)
{
    Sprite *sprite;
    u32 bitmap_count;

    if ((loaded == NULL) ||
        (ndsRelocRangeInLoadedFile(loaded, offset, sizeof(Sprite)) == FALSE))
    {
        gNdsOpeningPortraitsSpriteNormalizeFailCount++;
        return;
    }

    sprite = (Sprite *)((u8 *)loaded->data + offset);
    if (((u32)(u16)sprite->width == expected_width) &&
        ((u32)(u16)sprite->height == expected_height) &&
        (sprite->bmfmt == bmfmt) &&
        (sprite->bmsiz == bmsiz))
    {
        gNdsOpeningPortraitsSpriteNormalizeCount++;
        return;
    }

    ndsRelocNormalizeSpriteHeaderFields(sprite, bmfmt, bmsiz);
    bitmap_count = (u32)(u16)sprite->nbitmaps;
    if (((u32)(u16)sprite->width != expected_width) ||
        ((u32)(u16)sprite->height != expected_height) ||
        (bitmap_count == 0) ||
        (bitmap_count > 128u) ||
        (ndsRelocNormalizeSpriteBitmapTable(loaded, sprite,
                                            bitmap_count) == FALSE))
    {
        gNdsOpeningPortraitsSpriteNormalizeFailCount++;
        return;
    }

    gNdsOpeningPortraitsSpriteNormalizeCount++;
}

static void ndsRelocNormalizeOpeningPortraitsSprites(NDSRelocLoadedFile *loaded)
{
    if (loaded == NULL)
    {
        return;
    }

    if (loaded->asset_id == NDS_RELOC_ASSET_OPENING_PORTRAITS_SET1)
    {
        ndsRelocNormalizeOpeningPortraitsSprite(
            loaded, NDS_RELOC_SYMBOL_OPENING_PORTRAITS_SET1_SAMUS,
            NDS_OPENING_PORTRAITS_CARD_WIDTH,
            NDS_OPENING_PORTRAITS_CARD_HEIGHT, G_IM_FMT_RGBA, G_IM_SIZ_16b);
        ndsRelocNormalizeOpeningPortraitsSprite(
            loaded, NDS_RELOC_SYMBOL_OPENING_PORTRAITS_SET1_MARIO,
            NDS_OPENING_PORTRAITS_CARD_WIDTH,
            NDS_OPENING_PORTRAITS_CARD_HEIGHT, G_IM_FMT_RGBA, G_IM_SIZ_16b);
        ndsRelocNormalizeOpeningPortraitsSprite(
            loaded, NDS_RELOC_SYMBOL_OPENING_PORTRAITS_SET1_FOX,
            NDS_OPENING_PORTRAITS_CARD_WIDTH,
            NDS_OPENING_PORTRAITS_CARD_HEIGHT, G_IM_FMT_RGBA, G_IM_SIZ_16b);
        ndsRelocNormalizeOpeningPortraitsSprite(
            loaded, NDS_RELOC_SYMBOL_OPENING_PORTRAITS_SET1_PIKACHU,
            NDS_OPENING_PORTRAITS_CARD_WIDTH,
            NDS_OPENING_PORTRAITS_CARD_HEIGHT, G_IM_FMT_RGBA, G_IM_SIZ_16b);
        ndsRelocNormalizeOpeningPortraitsSprite(
            loaded, NDS_RELOC_SYMBOL_OPENING_PORTRAITS_SET1_COVER,
            NDS_OPENING_PORTRAITS_COVER_WIDTH,
            NDS_OPENING_PORTRAITS_COVER_HEIGHT, G_IM_FMT_I, G_IM_SIZ_4b);
    }
    else if (loaded->asset_id == NDS_RELOC_ASSET_OPENING_PORTRAITS_SET2)
    {
        ndsRelocNormalizeOpeningPortraitsSprite(
            loaded, NDS_RELOC_SYMBOL_OPENING_PORTRAITS_SET2_LINK,
            NDS_OPENING_PORTRAITS_CARD_WIDTH,
            NDS_OPENING_PORTRAITS_CARD_HEIGHT, G_IM_FMT_RGBA, G_IM_SIZ_16b);
        ndsRelocNormalizeOpeningPortraitsSprite(
            loaded, NDS_RELOC_SYMBOL_OPENING_PORTRAITS_SET2_KIRBY,
            NDS_OPENING_PORTRAITS_CARD_WIDTH,
            NDS_OPENING_PORTRAITS_CARD_HEIGHT, G_IM_FMT_RGBA, G_IM_SIZ_16b);
        ndsRelocNormalizeOpeningPortraitsSprite(
            loaded, NDS_RELOC_SYMBOL_OPENING_PORTRAITS_SET2_DONKEY,
            NDS_OPENING_PORTRAITS_CARD_WIDTH,
            NDS_OPENING_PORTRAITS_CARD_HEIGHT, G_IM_FMT_RGBA, G_IM_SIZ_16b);
        ndsRelocNormalizeOpeningPortraitsSprite(
            loaded, NDS_RELOC_SYMBOL_OPENING_PORTRAITS_SET2_YOSHI,
            NDS_OPENING_PORTRAITS_CARD_WIDTH,
            NDS_OPENING_PORTRAITS_CARD_HEIGHT, G_IM_FMT_RGBA, G_IM_SIZ_16b);
    }

    if (gNdsOpeningPortraitsSpriteNormalizeFailCount == 0)
    {
        gNdsOpeningPortraitsRelocResult = NDS_OPENING_PORTRAITS_RELOC_PASS;
    }
}

static void ndsRelocNormalizeIFAnnounceSprite(NDSRelocLoadedFile *loaded,
                                              u32 offset)
{
    Sprite *sprite;
    u32 bitmap_count;

    if ((loaded == NULL) ||
        (loaded->asset_id != NDS_RELOC_ASSET_IF_COMMON_ANNOUNCE) ||
        (ndsRelocRangeInLoadedFile(loaded, offset, sizeof(Sprite)) == FALSE))
    {
        NDS_DIAG(gNdsOpeningMarioSpriteNormalizeFailCount++);
        return;
    }

    sprite = (Sprite *)((u8 *)loaded->data + offset);
    if ((sprite->bmfmt == G_IM_FMT_IA) &&
        (sprite->bmsiz == G_IM_SIZ_8b) &&
        ((u32)(u16)sprite->width <= NDS_IF_ANNOUNCE_LETTER_MAX_WIDTH) &&
        ((u32)(u16)sprite->height <= NDS_IF_ANNOUNCE_LETTER_MAX_HEIGHT) &&
        ((u32)(u16)sprite->nbitmaps != 0) &&
        ((u32)(u16)sprite->nbitmaps <= 16u))
    {
        NDS_DIAG(gNdsOpeningMarioSpriteNormalizeCount++);
        return;
    }

    ndsRelocNormalizeSpriteHeaderFields(sprite, G_IM_FMT_IA, G_IM_SIZ_8b);
    bitmap_count = (u32)(u16)sprite->nbitmaps;
    if (((u32)(u16)sprite->width == 0) ||
        ((u32)(u16)sprite->height == 0) ||
        ((u32)(u16)sprite->width > NDS_IF_ANNOUNCE_LETTER_MAX_WIDTH) ||
        ((u32)(u16)sprite->height > NDS_IF_ANNOUNCE_LETTER_MAX_HEIGHT) ||
        (bitmap_count == 0) ||
        (bitmap_count > 16u) ||
        (ndsRelocNormalizeSpriteBitmapTable(loaded, sprite,
                                            bitmap_count) == FALSE))
    {
        NDS_DIAG(gNdsOpeningMarioSpriteNormalizeFailCount++);
        return;
    }

    NDS_DIAG(gNdsOpeningMarioSpriteNormalizeCount++);
}

static void ndsRelocNormalizeIFAnnounceMarioSprites(NDSRelocLoadedFile *loaded)
{
    if ((loaded == NULL) ||
        (loaded->asset_id != NDS_RELOC_ASSET_IF_COMMON_ANNOUNCE))
    {
        return;
    }

    ndsRelocNormalizeIFAnnounceSprite(
        loaded, NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_A);
    ndsRelocNormalizeIFAnnounceSprite(
        loaded, NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_B);
    ndsRelocNormalizeIFAnnounceSprite(
        loaded, NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_C);
    ndsRelocNormalizeIFAnnounceSprite(
        loaded, NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_D);
    ndsRelocNormalizeIFAnnounceSprite(
        loaded, NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_E);
    ndsRelocNormalizeIFAnnounceSprite(
        loaded, NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_F);
    ndsRelocNormalizeIFAnnounceSprite(
        loaded, NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_G);
    ndsRelocNormalizeIFAnnounceSprite(
        loaded, NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_H);
    ndsRelocNormalizeIFAnnounceSprite(
        loaded, NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_I);
    ndsRelocNormalizeIFAnnounceSprite(
        loaded, NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_J);
    ndsRelocNormalizeIFAnnounceSprite(
        loaded, NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_K);
    ndsRelocNormalizeIFAnnounceSprite(
        loaded, NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_L);
    ndsRelocNormalizeIFAnnounceSprite(
        loaded, NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_M);
    ndsRelocNormalizeIFAnnounceSprite(
        loaded, NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_N);
    ndsRelocNormalizeIFAnnounceSprite(
        loaded, NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_O);
    ndsRelocNormalizeIFAnnounceSprite(
        loaded, NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_P);
    ndsRelocNormalizeIFAnnounceSprite(
        loaded, NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_Q);
    ndsRelocNormalizeIFAnnounceSprite(
        loaded, NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_R);
    ndsRelocNormalizeIFAnnounceSprite(
        loaded, NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_S);
    ndsRelocNormalizeIFAnnounceSprite(
        loaded, NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_T);
    ndsRelocNormalizeIFAnnounceSprite(
        loaded, NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_U);
    ndsRelocNormalizeIFAnnounceSprite(
        loaded, NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_V);
    ndsRelocNormalizeIFAnnounceSprite(
        loaded, NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_W);
    ndsRelocNormalizeIFAnnounceSprite(
        loaded, NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_X);
    ndsRelocNormalizeIFAnnounceSprite(
        loaded, NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_Y);
    ndsRelocNormalizeIFAnnounceSprite(
        loaded, NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_Z);
    ndsRelocNormalizeIFAnnounceSprite(
        loaded, NDS_RELOC_SYMBOL_IF_ANNOUNCE_PERIOD);
}

static s32 ndsRelocNormalizeTitleFireSprite(NDSRelocLoadedFile *loaded,
                                            u32 offset)
{
    Sprite *sprite;
    u32 bitmap_count;
    Bitmap *bitmap;

    if ((loaded == NULL) ||
        (loaded->asset_id != NDS_RELOC_ASSET_MN_TITLE_FIRE_ANIM) ||
        (ndsRelocRangeInLoadedFile(loaded, offset, sizeof(Sprite)) == FALSE))
    {
        gNdsTitleFireSpriteNormalizeFailCount++;
        return FALSE;
    }

    sprite = (Sprite *)((u8 *)loaded->data + offset);
    if (((u32)(u16)sprite->width == 32u) &&
        ((u32)(u16)sprite->height == 32u) &&
        ((u32)(u16)sprite->nbitmaps == 1u) &&
        (sprite->bmfmt == G_IM_FMT_RGBA) &&
        (sprite->bmsiz == G_IM_SIZ_32b))
    {
        gNdsTitleFireSpriteNormalizeCount++;
        return TRUE;
    }

    ndsRelocNormalizeSpriteHeaderFields(sprite, G_IM_FMT_RGBA, G_IM_SIZ_32b);
    bitmap_count = (u32)(u16)sprite->nbitmaps;
    if (((u32)(u16)sprite->width != 32u) ||
        ((u32)(u16)sprite->height != 32u) ||
        (bitmap_count != 1u) ||
        (ndsRelocNormalizeSpriteBitmapTable(loaded, sprite,
                                            bitmap_count) == FALSE))
    {
        gNdsTitleFireSpriteNormalizeFailCount++;
        return FALSE;
    }

    bitmap = sprite->bitmap;
    if ((bitmap == NULL) ||
        ((u32)(u16)bitmap->width != 32u) ||
        ((u32)(u16)bitmap->width_img != 32u) ||
        ((u32)(u16)bitmap->actualHeight != 32u))
    {
        gNdsTitleFireSpriteNormalizeFailCount++;
        return FALSE;
    }

    gNdsTitleFireSpriteNormalizeCount++;
    return TRUE;
}

static void ndsRelocNormalizeTitleFireSprites(NDSRelocLoadedFile *loaded)
{
    u32 i;

    if ((loaded == NULL) ||
        (loaded->asset_id != NDS_RELOC_ASSET_MN_TITLE_FIRE_ANIM))
    {
        return;
    }

    for (i = 0; i < ARRAY_COUNT(sNdsTitleFireAnimFrameSymbols); i++)
    {
        (void)ndsRelocNormalizeTitleFireSprite(
            loaded,
            sNdsTitleFireAnimFrameSymbols[i].offset);
    }
}

static void ndsRelocReverseColorPackBytes(SYColorPack *color)
{
    u8 red;
    u8 green;
    u8 blue;
    u8 alpha;

    if (color == NULL)
    {
        return;
    }

    red = color->s.r;
    green = color->s.g;
    blue = color->s.b;
    alpha = color->s.a;
    color->s.r = alpha;
    color->s.g = blue;
    color->s.b = green;
    color->s.a = red;
}

static void ndsRelocNormalizeMObjSubWordSwapped(MObjSub *mobjsub)
{
    u16 old_pad00;
    u8 old_fmt;
    u8 old_siz;
    u16 old_flags;
    u8 old_block_fmt;
    u8 old_block_siz;
    u8 old_prim_l;
    u8 old_prim_m;
    u8 old_prim_pad0;
    u8 old_prim_pad1;

    if (mobjsub == NULL)
    {
        return;
    }

    old_pad00 = mobjsub->pad00;
    old_fmt = mobjsub->fmt;
    old_siz = mobjsub->siz;
    old_flags = mobjsub->flags;
    old_block_fmt = mobjsub->block_fmt;
    old_block_siz = mobjsub->block_siz;
    old_prim_l = mobjsub->prim_l;
    old_prim_m = mobjsub->prim_m;
    old_prim_pad0 = mobjsub->prim_pad[0];
    old_prim_pad1 = mobjsub->prim_pad[1];

    mobjsub->pad00 = ((u16)old_siz << 8) | old_fmt;
    mobjsub->fmt = (u8)(old_pad00 >> 8);
    mobjsub->siz = (u8)(old_pad00 & 0xffu);
    ndsRelocSwapS16Pair((s16 *)&mobjsub->unk08, (s16 *)&mobjsub->unk0A);
    ndsRelocSwapS16Pair((s16 *)&mobjsub->unk0C, (s16 *)&mobjsub->unk0E);

    mobjsub->flags = ((u16)old_block_siz << 8) | old_block_fmt;
    mobjsub->block_fmt = (u8)(old_flags >> 8);
    mobjsub->block_siz = (u8)(old_flags & 0xffu);
    ndsRelocSwapS16Pair((s16 *)&mobjsub->block_dxt,
                        (s16 *)&mobjsub->unk36);
    ndsRelocSwapS16Pair((s16 *)&mobjsub->unk38, (s16 *)&mobjsub->unk3A);

    mobjsub->prim_l = old_prim_pad1;
    mobjsub->prim_m = old_prim_pad0;
    mobjsub->prim_pad[0] = old_prim_m;
    mobjsub->prim_pad[1] = old_prim_l;
    ndsRelocReverseColorPackBytes(&mobjsub->primcolor);
    ndsRelocReverseColorPackBytes(&mobjsub->envcolor);
    ndsRelocReverseColorPackBytes(&mobjsub->blendcolor);
    ndsRelocReverseColorPackBytes(&mobjsub->light1color);
    ndsRelocReverseColorPackBytes(&mobjsub->light2color);
}

static u32 ndsRelocEffectiveMObjSubFlags(const MObjSub *mobjsub)
{
    u32 flags;

    if (mobjsub == NULL)
    {
        return 0;
    }

    flags = mobjsub->flags;
    if (flags == MOBJ_FLAG_NONE)
    {
        return MOBJ_FLAG_TEXTURE | 0x20u | MOBJ_FLAG_ALPHA;
    }
    return flags;
}

static s32 ndsRelocMObjSubFlagsKnown(u32 flags)
{
    return (flags &
            ~(MOBJ_FLAG_ALPHA | MOBJ_FLAG_SPLIT | MOBJ_FLAG_PALETTE |
              MOBJ_FLAG_FRAC | MOBJ_FLAG_TEXTURE | MOBJ_FLAG_PRIMCOLOR |
              MOBJ_FLAG_ENVCOLOR | MOBJ_FLAG_BLENDCOLOR |
              MOBJ_FLAG_LIGHT1 | MOBJ_FLAG_LIGHT2 | 0x8u | 0x20u |
              0x40u)) == 0;
}

static s32 ndsRelocMObjSubAttachmentFieldsLookNative(
    const MObjSub *mobjsub)
{
    if (mobjsub == NULL)
    {
        return FALSE;
    }

    /* pad00 is NOT part of the contract and must not gate the copy. It is a
     * decomp name for padding (sys/objtypes.h:302) that no code in
     * decomp/src ever reads -- objdisplay.c consumes flags, fmt, siz,
     * block_fmt, block_siz, block_dxt and the colours, and nothing else --
     * and the sibling validator ndsRelocMObjSubMixedFieldsLookNative already
     * omits it. Across all 795 typed MObjSub records in the relocData corpus
     * exactly three carry a non-zero pad00 (CommonSpark 0x0004,
     * DamageFlyMDust 0x0010, ShockSmall 0x0010) and every other field of all
     * three converts correctly; requiring pad00 == 0 declined them, which is
     * the three-per-lap BAD_ASSET record p2_shell_loop was reporting on Dream
     * Land. They are hit-reaction sparks and metal dust, so the fighters were
     * losing those materials in an ordinary Mario-versus-Fox match. */
    return (ndsRelocMObjSubFlagsKnown(mobjsub->flags) != FALSE) &&
           (mobjsub->fmt <= NDS_RELOC_G_IM_FMT_MAX) &&
           (mobjsub->siz <= G_IM_SIZ_32b) &&
           (mobjsub->block_fmt <= NDS_RELOC_G_IM_FMT_MAX) &&
           (mobjsub->block_siz <= G_IM_SIZ_32b);
}

static void ndsRelocRegisterNormalizedMObjSub(
    const NDSRelocLoadedFile *loaded, const MObjSub *mobjsub)
{
    u32 i;

    if ((loaded == NULL) || (mobjsub == NULL))
    {
        return;
    }
    for (i = 0u; i < sNdsRelocNormalizedMObjSubCount; i++)
    {
        NDSRelocNormalizedMObjSub *entry =
            &sNdsRelocNormalizedMObjSubs[i];

        if ((entry->record == mobjsub) &&
            (entry->asset_id == loaded->asset_id) &&
            (entry->owner_generation == loaded->owner_generation))
        {
            return;
        }
    }
    if (sNdsRelocNormalizedMObjSubCount >=
        NDS_RELOC_NORMALIZED_MOBJ_SUB_CAPACITY)
    {
        return;
    }
    sNdsRelocNormalizedMObjSubs[sNdsRelocNormalizedMObjSubCount].record =
        mobjsub;
    sNdsRelocNormalizedMObjSubs[sNdsRelocNormalizedMObjSubCount].asset_id =
        loaded->asset_id;
    sNdsRelocNormalizedMObjSubs[sNdsRelocNormalizedMObjSubCount]
        .owner_generation = loaded->owner_generation;
    sNdsRelocNormalizedMObjSubCount++;
}

static s32 ndsRelocMObjSubAlreadyNormalized(
    const NDSRelocLoadedFile *loaded, const MObjSub *mobjsub)
{
    u32 i;

    if ((loaded == NULL) || (mobjsub == NULL))
    {
        return FALSE;
    }
    for (i = 0u; i < sNdsRelocNormalizedMObjSubCount; i++)
    {
        const NDSRelocNormalizedMObjSub *entry =
            &sNdsRelocNormalizedMObjSubs[i];

        if ((entry->record == mobjsub) &&
            (entry->asset_id == loaded->asset_id) &&
            (entry->owner_generation == loaded->owner_generation))
        {
            return TRUE;
        }
    }
    return FALSE;
}

/* BattleShip O2R payloads are blanket-swapped as u32 words. MObjSub mixes
 * u16/u8 fields inside those words, so public object attachment must restore
 * those lanes before objman.c copies the material into its live MObj. Keep
 * pointers outside loaded files native. Within a loaded file, the
 * asset/generation registry identifies records already normalized in place;
 * every other record still has the O2R word-swapped representation. */
s32 ndsRelocCopyMObjSubForAttachment(MObjSub *dst, const MObjSub *src)
{
    NDSRelocLoadedFile *loaded;

    if ((dst == NULL) || (src == NULL))
    {
        return -1;
    }

    *dst = *src;
    loaded = ndsRelocFindLoadedFileContaining(src, sizeof(*src));
    if (loaded == NULL)
    {
        return 0;
    }
    if (ndsRelocMObjSubAlreadyNormalized(loaded, src) != FALSE)
    {
        return (ndsRelocMObjSubAttachmentFieldsLookNative(dst) != FALSE) ?
            0 : -1;
    }

    ndsRelocNormalizeMObjSubWordSwapped(dst);
    return (ndsRelocMObjSubAttachmentFieldsLookNative(dst) != FALSE) ? 1 : -1;
}

static s32 ndsRelocMVCommonMObjSubFlagsLookNative(u32 flags)
{
    return (flags == MOBJ_FLAG_NONE) ||
           (flags == MOBJ_FLAG_PRIMCOLOR) ||
           (flags == (MOBJ_FLAG_PRIMCOLOR | MOBJ_FLAG_LIGHT1));
}

static void ndsRelocRecordMObjSubNormalize(
    const NDSRelocLoadedFile *loaded, const MObjSub *mobjsub)
{
    u32 flags;
    u32 effective_flags;

    if (mobjsub == NULL)
    {
        return;
    }

    flags = mobjsub->flags;
    effective_flags = ndsRelocEffectiveMObjSubFlags(mobjsub);
    if (gNdsOpeningRoomRelocMObjSubNormalizeCount == 0)
    {
        gNdsOpeningRoomRelocMObjSubFirstFlags = flags;
    }
    gNdsOpeningRoomRelocMObjSubNormalizeCount++;
    gNdsOpeningRoomRelocMObjSubSourceResult =
        NDS_OPENING_ROOM_RELOC_MOBJ_SOURCE_PASS;

    if ((effective_flags & MOBJ_FLAG_TEXTURE) != 0)
    {
        if (gNdsOpeningRoomRelocMObjSubFirstTextureOffset == 0xffffffffu)
        {
            if ((loaded != NULL) &&
                (ndsRelocPointerRangeInLoadedFile(
                     loaded, mobjsub, sizeof(*mobjsub)) != FALSE))
            {
                gNdsOpeningRoomRelocMObjSubFirstTextureOffset =
                    (u32)((const u8 *)mobjsub - (const u8 *)loaded->data);
            }
            gNdsOpeningRoomRelocMObjSubFirstTextureFlags = flags;
        }
    }

}

static s32 ndsRelocMObjSubMixedFieldsLookNative(const MObjSub *mobjsub)
{
    if (mobjsub == NULL)
    {
        return FALSE;
    }

    return (ndsRelocMVCommonMObjSubFlagsLookNative(mobjsub->flags) != FALSE) &&
           (mobjsub->fmt <= NDS_RELOC_G_IM_FMT_MAX) &&
           (mobjsub->siz <= G_IM_SIZ_32b) &&
           (mobjsub->block_fmt <= NDS_RELOC_G_IM_FMT_MAX) &&
           (mobjsub->block_siz <= G_IM_SIZ_32b);
}

static void ndsRelocNormalizeMObjSubMixedFields(NDSRelocLoadedFile *loaded,
                                                MObjSub *mobjsub)
{
    if (mobjsub == NULL)
    {
        return;
    }

    if (ndsRelocMObjSubMixedFieldsLookNative(mobjsub) != FALSE)
    {
        ndsRelocRegisterNormalizedMObjSub(loaded, mobjsub);
        ndsRelocRecordMObjSubNormalize(loaded, mobjsub);
        return;
    }

    ndsRelocNormalizeMObjSubWordSwapped(mobjsub);
    ndsRelocRegisterNormalizedMObjSub(loaded, mobjsub);
    ndsRelocRecordMObjSubNormalize(loaded, mobjsub);
}

static void ndsRelocNormalizeMObjSubTable(NDSRelocLoadedFile *loaded,
                                          u32 offset,
                                          u32 head_count)
{
    MObjSub ***p_mobjsubs;
    u32 head_index;

    if ((loaded == NULL) ||
        (loaded->asset_id != NDS_RELOC_ASSET_MV_COMMON) ||
        (ndsRelocRangeInLoadedFile(loaded, offset, sizeof(*p_mobjsubs)) ==
         FALSE))
    {
        return;
    }

    p_mobjsubs = (MObjSub ***)((u8 *)loaded->data + offset);
    for (head_index = 0; head_index < head_count; head_index++)
    {
        MObjSub **mobjsubs;
        u32 list_index;

        if (ndsRelocPointerRangeInLoadedFile(
                loaded, &p_mobjsubs[head_index], sizeof(*p_mobjsubs)) ==
            FALSE)
        {
            return;
        }

        mobjsubs = p_mobjsubs[head_index];
        if (mobjsubs == NULL)
        {
            continue;
        }
        if (ndsRelocPointerRangeInLoadedFile(
                loaded, mobjsubs, sizeof(*mobjsubs)) == FALSE)
        {
            return;
        }

        for (list_index = 0; list_index < 8u; list_index++)
        {
            MObjSub *mobjsub;

            if (ndsRelocPointerRangeInLoadedFile(
                    loaded, &mobjsubs[list_index], sizeof(*mobjsubs)) ==
                FALSE)
            {
                return;
            }

            mobjsub = mobjsubs[list_index];
            if (mobjsub == NULL)
            {
                break;
            }
            if (ndsRelocPointerRangeInLoadedFile(
                    loaded, mobjsub, sizeof(*mobjsub)) == FALSE)
            {
                return;
            }
            ndsRelocNormalizeMObjSubMixedFields(loaded, mobjsub);
        }
    }
}

static void ndsRelocNormalizeMVCommonMObjSubs(NDSRelocLoadedFile *loaded)
{
    if ((loaded == NULL) ||
        (loaded->asset_id != NDS_RELOC_ASSET_MV_COMMON))
    {
        return;
    }

    ndsRelocNormalizeMObjSubTable(
        loaded, NDS_RELOC_SYMBOL_MVCOMMON_BACKGROUND_MOBJ, 52u);
    ndsRelocNormalizeMObjSubTable(
        loaded, NDS_RELOC_SYMBOL_MVCOMMON_LOGO_MOBJ, 2u);
    ndsRelocNormalizeMObjSubTable(
        loaded, NDS_RELOC_SYMBOL_MVCOMMON_CLOSEUP_AIR_MOBJ, 4u);
    ndsRelocNormalizeMObjSubTable(
        loaded, NDS_RELOC_SYMBOL_MVCOMMON_CLOSEUP_GROUND_MOBJ, 2u);
    ndsRelocNormalizeMObjSubTable(
        loaded, NDS_RELOC_SYMBOL_MVCOMMON_DESK_GROUND_MOBJ, 8u);
    ndsRelocNormalizeMObjSubTable(loaded,
                                  NDS_RELOC_SYMBOL_MVCOMMON_SPOTLIGHT_MOBJ,
                                  2u);
}
