/* Native renderer, production runs: dense shade words, R2 epoch shading and tint
 * batches, the texture memo and UV tables, texgen, run preparation and emission.
 * A slice of nds_renderer.c. */

static void ndsRendererNativeBuildDenseShadeWords(
    const NDSNativeDenseVertex *vertices, u32 vertex_count,
    const NDSNativeRun *runs, u32 run_count,
    const u16 *run_first_unique, const u8 *run_unique_count,
    const u16 *run_unique_dense, u32 *dense_words)
{
    u32 index;

    for (index = 0u; index < vertex_count; index++)
    {
        u32 rgba = vertices[index].rgba;
        s32 nx = ndsRendererR2NormalComponent((s32)(s8)(rgba >> 24));
        s32 ny = ndsRendererR2NormalComponent((s32)(s8)(rgba >> 16));
        s32 nz = ndsRendererR2NormalComponent((s32)(s8)(rgba >> 8));

        dense_words[index] =
            NDS_NORMAL_PACK((int)nx, (int)ny, (int)nz);
    }
    for (index = 0u; index < run_count; index++)
    {
        const NDSNativeRun *run = &runs[index];
        u32 unique_first;
        u32 unique_count;
        u32 unique_index;

        if (ndsRendererNativeRunUsesVertexColor(run) == FALSE)
        {
            continue;
        }
        unique_first = run_first_unique[index];
        unique_count = run_unique_count[index];
        for (unique_index = 0u; unique_index < unique_count; unique_index++)
        {
            u32 dense_id = run_unique_dense[unique_first + unique_index];
            u32 rgba = vertices[dense_id].rgba;

            dense_words[dense_id] =
                RGB15((u8)((rgba >> 27) & 0x1fu),
                      (u8)((rgba >> 19) & 0x1fu),
                      (u8)((rgba >> 11) & 0x1fu));
        }
    }
}

NDS_RENDERER_COLD static void __attribute__((noinline)) ndsRendererR2BuildDenseNormals(void)
{
    ndsRendererNativeBuildDenseShadeWords(
        sNdsNativeFighterActiveTables->dense_vertices,
        sNdsNativeFighterActiveTables->dense_count,
        sNdsNativeFighterActiveTables->runs,
        sNdsNativeFighterActiveTables->run_count,
        sNdsNativeFighterActiveTables->run_first_unique,
        sNdsNativeFighterActiveTables->run_unique_count,
        sNdsNativeFighterActiveTables->run_unique_dense,
        sNdsNativeFighterActiveDenseNormals);
    *sNdsNativeFighterActiveDenseNormalsBuilt = 1u;
}

/* One channel of the material term, reproducing the software path's arithmetic
 * so the only intended difference is that the engine evaluates the dot product.
 * `base` is the source light colour byte; `material` is the epoch's material
 * byte when the policy uses one. */
static inline u32 ndsRendererR2MaterialChannel(
    u32 base, u32 material, u32 use_material)
{
    if (use_material != 0u)
    {
        return ndsRendererHardwareScaleMaterialChannel5(base, material);
    }
    return (base >> 3) & 0x1fu;
}

/* Cleared per owner execute; set once the light vector has been written. */
static u8 sNdsLightVectorWritten;


/* The resident tint tile this epoch's untextured runs modulate by, or 0 for the
 * folded-material path. Written once per epoch by
 * ndsRendererNativeShadeProductionActions (via ndsRendererR2ResolveEpochShade)
 * and read by the run prepare. Deliberately OUTSIDE the
 * NDS_UNLIT_VERTEX_EPOCH guard that sNdsEpochUnlitVertexColor above sits
 * inside -- that flag is 0 in every shipping config, and both of this
 * variable's users are unconditional. */
static u32 sNdsEpochTintTexture;
/* P2-2p8 Phase 1 slice 1: the 24-bit colour that chose the tile above. */
static u32 sNdsEpochTintRgb;
/* Engagement proof. The ambient-only bisect showed the material path working
 * end to end while the diffuse term stayed zero, which leaves only the light
 * vector -- and "the write never ran" and "the write ran and did not stick" are
 * different bugs. */
u32 gNdsLightVectorWrites;

/* ---------------------------------------------------------------------------
 * K02/P04/J01. The facing-dependent shade seam, and what actually varies.
 *
 * REFUTED 2026-09-21 -- the standing "negative-determinant modelview" reading.
 * A fighter's facing is NOT a mirror. ftmain.c:4477 writes
 * `fp->joints[nFTPartsJointTopN]->rotate.vec.f.y = fp->lr * 90 degrees`, a pure
 * rotation, and no `lr`-driven negative scale exists anywhere in decomp/ or
 * src/import/. Both facings therefore hand the geometry engine a
 * POSITIVE-determinant vector matrix, so no normal is turned inward by it and
 * no diffuse term collapses for that reason. The witness below measures the
 * determinant in hardware anyway, because a source read is not an oracle for
 * what the engine holds.
 *
 * What IS facing-dependent is the only facing-dependent quantity left in this
 * pipeline: the per-vertex diffuse LEVEL, i.e. the light/normal dot. Write both
 * paths out for the same matrix M (the 3x3 the engine applies to GFX_NORMAL)
 * and the same source light L:
 *
 *   software  ndsRendererHardwarePrepareLitDirection + LitDiffuseNumer
 *             level = dot(N, M L) / |M L|     -- L renormalised AFTER transform
 *   hardware  this function + the engine
 *             level = dot(N, M L) / |L|       -- L normalised BEFORE it, and the
 *                                                engine renormalises nothing
 *
 *       =>    level_hw / level_sw  =  |M L| / |L|
 *
 * (The two agree on the dot itself: the engine forms dot(Lstored, N x M), and
 * dot(u, N x M) == dot(N, M u) identically, which is exactly what
 * PrepareLitDirection's `t_i = sum_j m[i][j] L_j` computes. The sign flip is
 * the DS's max(0, -L.N) convention, already handled by the negation below.)
 *
 * That ratio is 1 exactly when the 3x3 is orthonormal -- the assumption the
 * comment below states as settled fact ("the joint modelviews are rigid"). It
 * is not generally true here: renderer_adapter_matrix.c:2684 builds a joint
 * local through syMatrixTraRotRpyRSca whenever a DObj scale is not 1.0, and
 * renderer_adapter_matrix.c:2820 records the SOURCE itself measuring joint
 * world row-0 lengths up to 9.33. With an ANISOTROPIC scale in the chain the
 * ratio depends on where L points in the model frame, and the facing rotation
 * is precisely what moves L in that frame. An ISOTROPIC scale above 1 is
 * facing-dependent too, by saturation: the set of normals whose level clips at
 * 1.0 rotates with the fighter.
 *
 * Either shape reads as a FACE/BODY seam rather than a uniform brightness
 * change, because ndsRendererR2ClampDiffuseToMaterial caps the use_material run
 * at prim - ambient while the no-material / white-prim run still climbs to 31.
 * The two runs saturate at different levels, so an error in the LEVEL surfaces
 * as a boundary between them -- which is why the clamp repair fixed one facing
 * and not the other without the clamp itself being wrong.
 *
 * So measure the ratio instead of guessing it. The DS publishes the live vector
 * matrix at MATRIX_READ_VECTOR (nine 20.12 words, coherent only while GXSTAT
 * bit 27 reports the geometry engine idle) and that is the exact 3x3 every
 * GFX_NORMAL is multiplied by. */
#ifndef NDS_LIGHT_VECTOR_MATRIX
/* 1 = sample the live vector matrix at the light-vector write and publish the
 *     witness. Costs one bounded FIFO-drain wait plus nine register reads per
 *     owner execute (2/frame; E16a measured one light write per execute).
 * 0 = r25 arithmetic and r25 cost exactly: no read, no drain, no witness.
 *
 * DEFAULT 0, AND THE REASON IS THE ARENA, NOT THE ANALYSIS. This witness has
 * already done its job: read on 2026-09-21 against a live Pikachu-vs-Fox match
 * it returned row_norm 4907 (a rigid chain reads 4096), det_20p12 +7048 and 92
 * of 100 writes SHRINKING -- which confirmed the mechanism, refuted the
 * negative-determinant reading in hardware, and showed the stretch repair
 * addresses the minority case. None of that needs re-taking.
 *
 * What it costs is measured too: the whole 09-21 batch moved the shipping
 * arena from 941,568 to 933,376, and the tightest recorded roster
 * (Pikachu/Fox) had only 7,556 bytes free at GO. Instrumentation that eats the
 * headroom of the very rows it is instrumenting is a bad trade -- and
 * `malloc.c:30` is `while (TRUE);`, so overrunning that arena hangs the console
 * rather than declining. Turn this back on in a lab build when the follow-up
 * repair needs re-measuring; do not ship it on. */
#define NDS_LIGHT_VECTOR_MATRIX 0
#endif
#ifndef NDS_LIGHT_VECTOR_STRETCH_FIX
/* THE REPAIR, split from the witness so the measurement can be taken without
 * it. 1 divides the stored light by |M L| instead of |L|, which is the exact
 * software normalisation moved to the only place the hardware path can express
 * it. Algebraically IDENTICAL to 0 whenever the 3x3 is orthonormal, because
 * |M L| is then (1 << FRAC) * |L| -- a rigid chain is unchanged bit for bit,
 * so this cannot regress a fighter whose chain was already rigid. Requires
 * NDS_LIGHT_VECTOR_MATRIX; fails closed to the r25 divisor whenever the
 * sample is unusable.
 *
 * DEFAULT 0 UNTIL THE PACKET TWIN IS RECONCILED, and that is an integration
 * decision, not a doubt about the analysis. ndsFighterPacketLightWord (:9530)
 * patches the light word of a REPLAYED packet and cannot take this correction:
 * it runs outside the draw with no joint matrix loaded, and the divisor |M L|
 * is a function of the pose, so neither a stored value nor a sample taken at
 * patch time is the matrix the live write used. With the repair on and the
 * packet path live -- which it is in the shipping profile -- a fighter whose
 * chain actually stretches would shade correctly on prepared frames and as r25
 * on replayed ones. Most frames replay, so the visible result is a FLICKER
 * between two shades: a new defect, and a louder one than the steady seam
 * under repair.
 *
 * The witness above stays ON and is the point of this build. One playtest
 * reading row_norm, det_20p12 and stretch_20p12 facing left, facing right and
 * in ledge-balance decides whether this mechanism is even live. If row_norm is
 * 4096 on both facings the mechanism is dead and the residual is elsewhere; if
 * the stretch differs with facing, land the repair together with a twin that
 * agrees, not before it. Turning this on alone would ship an unproven repair
 * with a known divergence. */
#define NDS_LIGHT_VECTOR_STRETCH_FIX 0
#endif

/* Published per captured fighter by src/port/renderer_adapter_fighter.c, which
 * defines them unconditionally so these externs resolve in every build. The
 * renderer has no FTStruct, and no shade sample can be read at all without
 * knowing which facing produced it -- that is the whole point of r25. */
extern volatile s32 gNdsFighterFacingLr;
extern volatile u32 gNdsFighterFacingSlot;

#if NDS_LIGHT_VECTOR_MATRIX
/* GXSTAT bit 27 is the geometry-engine busy flag. The matrix read ports are
 * only coherent while it is clear, and an unbounded spin inside the draw is a
 * hang, so the wait is bounded and a missed sample degrades to the r25
 * arithmetic rather than to a wrong light. */
#define NDS_LIGHT_MATRIX_BUSY_BIT (1u << 27)
/* ~16k volatile MMIO polls. The FIFO drains in far less than that even full,
 * and the cap is what keeps a pathological never-idle engine to a few
 * milliseconds instead of a multi-frame hang. */
#define NDS_LIGHT_MATRIX_IDLE_SPINS 0x4000u
#define NDS_LIGHT_MATRIX_WITNESS_SLOTS 4u

typedef struct NDSLightMatrixWitness
{
    s32 vector_matrix[9];   /* MATRIX_READ_VECTOR, 20.12, row major m[i][j]   */
    s32 light_dir[3];       /* stats->light_dir_*, source units               */
    s32 row_norm[3];        /* |row i| in 20.12; 4096 == rigid                */
    s32 det_20p12;          /* det(3x3) in 20.12; +4096 == plain rotation     */
    s32 light_len;          /* |L| in 20.12 * source units                    */
    s32 stretched_len;      /* |M L| in 20.12 * source units                  */
    s32 stretch_20p12;      /* |M L| / |L| in 20.12; 4096 == no correction    */
    s32 facing_lr;          /* fp->lr at capture: -1 left, 0 centre, +1 right */
    u32 facing_slot;        /* fp->nds_slot                                   */
    u32 idle_ok;            /* 0 = engine never idled; the words are stale    */
} NDSLightMatrixWitness;

/* EVENT-LOCAL, not a latch. One slot per fighter owner slot, every field
 * overwritten on every owner execute that writes a light vector, so a value
 * cannot survive the fighter that produced it. The three counters exist only
 * to prove the sample is live -- read them as deltas, never as flags. This
 * repo has been burned by sticky diagnostic latches often enough that the rule
 * is worth restating in the code.
 *
 * COVERAGE, because a counter that only counts what it saw is how a green run
 * gets over-read: a REPLAYED fighter submits the recorded packet and never
 * enters ndsRendererR2WriteLightVector, so it contributes no sample at all.
 * Compare gNdsLightMatrixWitnessWrites against gNdsLightVectorWrites (they
 * should track) and both against the number of fighters actually drawing
 * before concluding anything from a slot that did not move. */
volatile NDSLightMatrixWitness
    gNdsLightMatrixWitness[NDS_LIGHT_MATRIX_WITNESS_SLOTS];
volatile u32 gNdsLightMatrixWitnessWrites;
volatile u32 gNdsLightMatrixIdleTimeouts;
__attribute__((used)) volatile u32 gNdsLightMatrixStretchApplied;
/* |M L| < |L|: the chain SHRINKS the light. The hardware diffuse is then
 * darker than the source's and no unit-bounded light vector can undo it, so
 * the repair declines rather than wrapping the packed components. A non-zero
 * delta here is the evidence that the remaining error needs the diffuse COLOUR
 * scaled instead, which is a different lever and a different task card. */
__attribute__((used)) volatile u32 gNdsLightMatrixStretchDeclined;

/* Returns |M L| in 20.12 * source units, or 0 when the sample is unusable --
 * which is the caller's signal to keep the r25 divisor. noinline and outside
 * .itcm.native_fighter for the same reason as its caller: the region is full. */
static s32 __attribute__((noinline)) ndsRendererR2SampleVectorMatrix(
    s32 lx, s32 ly, s32 lz, s32 light_len_q12)
{
    volatile NDSLightMatrixWitness *witness;
    s32 m[9];
    s64 t[3];
    s64 length_squared;
    s64 det;
    u32 slot;
    u32 spins = 0u;
    u32 idle = 1u;
    u32 i;
    s32 stretched = 0;

    while (((u32)GFX_STATUS & NDS_LIGHT_MATRIX_BUSY_BIT) != 0u)
    {
        spins++;
        if (spins >= NDS_LIGHT_MATRIX_IDLE_SPINS)
        {
            idle = 0u;
            NDS_DIAG(gNdsLightMatrixIdleTimeouts++);
            break;
        }
    }
    for (i = 0u; i < 9u; i++)
    {
        m[i] = (s32)MATRIX_READ_VECTOR[i];
    }
    /* Byte for byte ndsRendererHardwarePrepareLitDirection's transform, so the
     * witness and the software oracle cannot drift on what "transformed" means:
     *     t_i = sum_j m[i][j] * L_j  */
    for (i = 0u; i < 3u; i++)
    {
        t[i] = ((s64)m[(i * 3u) + 0u] * lx) +
               ((s64)m[(i * 3u) + 1u] * ly) +
               ((s64)m[(i * 3u) + 2u] * lz);
    }
    length_squared = (t[0] * t[0]) + (t[1] * t[1]) + (t[2] * t[2]);
    if (length_squared > 0)
    {
        stretched = (s32)ndsHwMathSqrt64((u64)length_squared);
    }

    slot = gNdsFighterFacingSlot;
    if (slot >= NDS_LIGHT_MATRIX_WITNESS_SLOTS)
    {
        slot = NDS_LIGHT_MATRIX_WITNESS_SLOTS - 1u;
    }
    witness = &gNdsLightMatrixWitness[slot];
    for (i = 0u; i < 9u; i++)
    {
        witness->vector_matrix[i] = m[i];
    }
    for (i = 0u; i < 3u; i++)
    {
        s64 row_squared =
            ((s64)m[(i * 3u) + 0u] * m[(i * 3u) + 0u]) +
            ((s64)m[(i * 3u) + 1u] * m[(i * 3u) + 1u]) +
            ((s64)m[(i * 3u) + 2u] * m[(i * 3u) + 2u]);

        witness->row_norm[i] = (row_squared > 0) ?
            (s32)ndsHwMathSqrt64((u64)row_squared) : 0;
    }
    det = ((s64)m[0] * (((s64)m[4] * m[8]) - ((s64)m[5] * m[7]))) -
          ((s64)m[1] * (((s64)m[3] * m[8]) - ((s64)m[5] * m[6]))) +
          ((s64)m[2] * (((s64)m[3] * m[7]) - ((s64)m[4] * m[6])));
    /* Three 20.12 factors make the raw determinant 20.36; shift back to 20.12
     * so +4096 reads as "plain rotation" and any negative value settles the
     * mirror claim on hardware. */
    witness->det_20p12 = (s32)(det >> (2 * NDS_RENDERER_DS_MTX_FRAC_BITS));
    witness->light_dir[0] = lx;
    witness->light_dir[1] = ly;
    witness->light_dir[2] = lz;
    witness->light_len = light_len_q12;
    witness->stretched_len = stretched;
    /* Both operands are already 20.12, so one more FRAC in the numerator puts
     * the ratio itself in 20.12: 4096 reads as "no correction needed". */
    witness->stretch_20p12 = (light_len_q12 > 0) ?
        (s32)ndsHwMathDiv64((s64)stretched << NDS_RENDERER_DS_MTX_FRAC_BITS,
                              light_len_q12) : 0;
    witness->facing_lr = gNdsFighterFacingLr;
    witness->facing_slot = gNdsFighterFacingSlot;
    witness->idle_ok = idle;
    NDS_DIAG(gNdsLightMatrixWitnessWrites++);

    return (idle != 0u) ? stretched : 0;
}
#endif /* NDS_LIGHT_VECTOR_MATRIX */

/* GFX_LIGHT_VECTOR stores the vector transformed by the vector matrix at write
 * time, so it has to be written under an identity vector matrix. The owner
 * preamble looked like the place for that -- it already loads identity before
 * the first root -- but stats->light_dir_* is only populated by the epoch state
 * deltas, so at preamble time it is still zero. A zero light vector makes every
 * dot product zero, which is exactly the black-silhouette failure the first
 * build showed and the diffuse-only diagnostic confirmed.
 *
 * So it is written here instead, the first time an epoch actually has a
 * direction, bracketed by push/identity/pop so the root's matrices survive.
 * E16a measured the direction changing 0 times in 22,296 epochs, so this runs
 * once per execute and the bracket is not on any hot path.
 *
 * The vector is normalised because the hardware normalises nothing: the software
 * path rescales the transformed light to length 127 before the dot, and IF the
 * joint modelviews are rigid, normalising before the transform is the same
 * thing. It is negated because the hardware's diffuse term is max(0, -L.N)
 * where the software clamps a positive L.N.
 *
 * That "if" used to read as settled fact. It is the K02/P04/J01 assumption --
 * see the block above NDS_LIGHT_VECTOR_MATRIX: normalising before the
 * transform and after it differ by exactly |M L| / |L|, and this function now
 * measures that factor rather than asserting it is 1.
 *
 * Deliberately outside .itcm.native_fighter and noinline -- the shade function
 * is ITCM-resident and full, and the region overflowed once already this task. */
static void __attribute__((noinline)) ndsRendererR2WriteLightVector(
    const NDSRendererStats *stats)
{
    s32 x = stats->light_dir_x;
    s32 y = stats->light_dir_y;
    s32 z = stats->light_dir_z;
    s64 length_squared = ((s64)x * x) + ((s64)y * y) + ((s64)z * z);
    s32 nx = 0;
    s32 ny = 0;
    s32 nz = 0;
    s32 light_len = (length_squared > 0) ?
        (s32)ndsHwMathSqrt64((u64)length_squared) : 0;
#if NDS_LIGHT_VECTOR_MATRIX
    s32 stretched_len;
    /* |L| in 20.12, NOT `light_len << FRAC`.
     *
     * check-light-stretch.py caught this: ndsHwMathSqrt64 truncates, and
     * the shipped light is about 100 units long, so `light_len` is up to ~1%
     * short of |L| -- r25 has been storing a light vector ~1% longer than unit
     * ever since, harmlessly. Comparing the 20.12 |M L| against that truncated
     * value would read a rigid chain as a 1% stretch and "correct" every
     * fighter in the game by two LSB. sqrt(n << 24) == sqrt(n) << 12, so this
     * costs one more hardware sqrt and removes the whole class. */
    s32 light_len_q12 = (length_squared > 0) ?
        (s32)ndsHwMathSqrt64((u64)length_squared <<
                               (2 * NDS_RENDERER_DS_MTX_FRAC_BITS)) : 0;
#endif

    /* Moved ahead of the arithmetic: the vector-matrix sample below needs the
     * primitive closed before it waits on the geometry engine, and ending the
     * batch earlier changes nothing else -- no FIFO word between here and the
     * bracket depends on a batch being open. */
    ndsRendererHardwareEndBatch();
#if NDS_LIGHT_VECTOR_MATRIX
    /* Before the identity bracket on purpose: the sample must be the LIVE
     * root/joint vector matrix this execute's normals will be multiplied by,
     * not the identity the light vector is written under. */
    stretched_len = ndsRendererR2SampleVectorMatrix(x, y, z, light_len_q12);
    /* Witness-only build still CLASSIFIES the sample below -- Applied and
     * Declined are the measurement -- and only the divisor is left alone. */
#endif
    /* The DS hardware square-root and division units, not sqrtf. Partly because
     * this runs twice a frame and they are free here, and partly because
     * check-gbi-decode-fixtures asserts the renderer holds exactly one sqrtf
     * site, isolated inside ndsRendererHardwarePrepareLitDirection -- a second
     * one is how the software light normalization creeps back in. */
    if (light_len > 0)
    {
        u32 length = (u32)light_len;
        s64 numerator_scale = 511;

#if NDS_LIGHT_VECTOR_MATRIX
        /* Divide by |M L| instead of |L| -- the software path's normalisation
         * point, expressed where the hardware path can hold it. The extra
         * 1 << FRAC in the numerator cancels the 20.12 the matrix carries, so
         * an orthonormal 3x3 reduces this to `-x * 511 / |L|` exactly.
         *
         * ONLY WHEN THE CHAIN STRETCHES, never when it shrinks. The stored
         * vector is 10-bit signed and NDS_NORMAL_PACK masks rather than
         * saturates, so a component past 511 does not clip, it WRAPS -- and a
         * wrapped component is a light pointing somewhere else entirely, which
         * is a far louder defect than the one under repair. |M L| >= |L| can
         * only shrink the stored vector, so it is unconditionally safe; the
         * shrinking case (an under-bright hardware diffuse) cannot be
         * expressed by a unit-bounded light vector at all and is counted here
         * for the witness rather than approximated.
         *
         * AND ONLY OUTSIDE A DEADBAND. A rigid chain is the overwhelmingly
         * common case and must come out of this function bit for bit as r25
         * left it -- this path is reached by every lit fighter in the game and
         * a two-LSB drift on all of them is exactly the "repair validated
         * against one user of a shared path" failure. The 1/32 band is chosen
         * to clear the 20.12 matrix rounding with room to spare while still
         * catching any stretch large enough to move a visible shade. */
        if ((light_len_q12 > 0) &&
            (stretched_len >
             (light_len_q12 + (light_len_q12 >> 5))))
        {
            /* COUNT UNDER THE WITNESS, DIVIDE ONLY UNDER THE REPAIR.
             *
             * The classification is the measurement this build exists to
             * take: a non-zero Applied says the chain really does stretch and
             * the mechanism is live, which is the fact that decides whether
             * the repair is worth reconciling with the packet twin. Counting
             * it only when the repair is enabled made the question
             * unanswerable without shipping the answer -- and left both
             * counters unwritten, which is how --gc-sections collected them
             * out of the r30 ELF entirely. */
            NDS_DIAG(gNdsLightMatrixStretchApplied++);
#if NDS_LIGHT_VECTOR_STRETCH_FIX
            length = (u32)stretched_len;
            numerator_scale =
                (s64)511 << NDS_RENDERER_DS_MTX_FRAC_BITS;
#endif
        }
        else if ((light_len_q12 > 0) && (stretched_len > 0) &&
                 (stretched_len <
                  (light_len_q12 - (light_len_q12 >> 5))))
        {
            NDS_DIAG(gNdsLightMatrixStretchDeclined++);
        }
#endif
        if (length != 0u)
        {
            nx = (s32)ndsHwMathDiv64(-(s64)x * numerator_scale,
                                       (s32)length);
            ny = (s32)ndsHwMathDiv64(-(s64)y * numerator_scale,
                                       (s32)length);
            nz = (s32)ndsHwMathDiv64(-(s64)z * numerator_scale,
                                       (s32)length);
        }
    }

    ndsRendererHardwareSetMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    GFX_LIGHT_VECTOR = NDS_NORMAL_PACK((int)nx, (int)ny, (int)nz);
    glPopMatrix(1);
    NDS_DIAG(gNdsLightVectorWrites++);
    sNdsLightVectorWritten = 1u;
}

/* P2-2p8 2026-09-27 ITCM swap: out (noinline, so no ITCM caller pulls it
 * back in) for ndsStageGxDraw; census rent under ~1,300 cycles a byte. */
static u16 __attribute__((noinline)) ndsRendererR2MaterialColor15(
    u32 light_color, u32 material_color, u32 use_material, u32 color_modulate)
{
    u32 r = ndsRendererR2MaterialChannel(
        (light_color >> 24) & 0xffu, (material_color >> 24) & 0xffu,
        use_material);
    u32 g = ndsRendererR2MaterialChannel(
        (light_color >> 16) & 0xffu, (material_color >> 16) & 0xffu,
        use_material);
    u32 b = ndsRendererR2MaterialChannel(
        (light_color >> 8) & 0xffu, (material_color >> 8) & 0xffu,
        use_material);

    return ndsRendererHardwareModulatePackedColor(
        RGB15(r, g, b), color_modulate);
}

volatile u32 gNdsShadeClampAppliedCount;

/* Forward: the shade resolver and the two tint-run helpers live outside the
 * fighter ITCM section, so the ITCM callers below carry only a call each; see
 * their definitions. */
static u32 ndsRendererR2ResolveEpochShade(
    u32 light1, u32 light2, u32 material_color, u32 use_material,
    u32 color_modulate, u32 textured);
static void ndsRendererR2BindTintTile(NDSRendererStats *stats, u32 tint);
static void ndsRendererR2OpenTintBatch(
    NDSRendererStats *stats, u32 tint, u32 poly_fmt, u32 matrix_generation);

/* OWNER 2026-09-21: Kirby's face and body are still two colours, and HOLDING
 * NEUTRAL B fixes it. A static arithmetic error cannot be cured by holding a
 * button, so the clamp fold below is not the reported defect -- something a
 * status change clears is stale cached state. The owner's question is the one
 * to answer: is the FACE unlit, or the BODY?
 *
 * A breakpoint cannot answer it. This decision is made once per material epoch
 * per fighter per frame in ITCM, and attaching gdb there wanders the CPU to
 * 0xfffffffc within a minute (measured). So record it instead: a ring of the
 * last 16 epochs, read once from a cheap breakpoint, then run twice -- with and
 * without B held -- and diff.
 *
 * `lit` is the geometry-engine lighting decision, `unlit_vertex` is the E48/E49
 * raw-vertex-colour override, `material` is the colour the fold was given, and
 * `shade` is the diffuse|ambient<<16 actually written. Two adjacent runs
 * disagreeing on `lit` or `unlit_vertex` look exactly like two materials and
 * are immune to any diffuse/ambient correction. */
#define NDS_SHADE_WITNESS_SLOTS 16u
typedef struct NDSShadeWitness
{
    u32 material;
    u32 shade;
    u8 lit;
    u8 unlit_vertex;
    u8 use_material;
    /* r25 addition. The owner's report is FACING-dependent -- fixed left, wrong
     * right, fixed right in ledge-balance -- so a shade sample that does not
     * carry its facing cannot be compared against another one. `fp->lr` verbatim
     * from the source (-1 left, 0 centre, +1 right), narrowed to the byte the
     * struct already wasted on padding, so the ring costs no extra bytes.
     * Overwritten every epoch: event-local, never a latch. */
    s8 facing_lr;
} NDSShadeWitness;
volatile NDSShadeWitness gNdsShadeWitness[NDS_SHADE_WITNESS_SLOTS];
volatile u32 gNdsShadeWitnessWrites;

static void ndsRendererR2RecordShadeWitness(u32 lit, u32 unlit_vertex,
                                            u32 use_material, u32 material)
{
    u32 slot = gNdsShadeWitnessWrites % NDS_SHADE_WITNESS_SLOTS;

    gNdsShadeWitness[slot].material = material;
    gNdsShadeWitness[slot].shade = 0u;
    gNdsShadeWitness[slot].lit = (u8)(lit != 0u);
    gNdsShadeWitness[slot].unlit_vertex = (u8)(unlit_vertex != 0u);
    gNdsShadeWitness[slot].use_material = (u8)(use_material != 0u);
    gNdsShadeWitness[slot].facing_lr = (s8)gNdsFighterFacingLr;
    gNdsShadeWitnessWrites++;
}

/* CLAMP ORDER. The N64 clamps the shade sum to eight bits and modulates by the
 * primitive colour AFTERWARDS -- sys/objdisplay.c's material plus the RDP
 * combiner give pixel = clamp8(light2 + light1 * dot) * prim / 255. The DS
 * geometry engine has no inner clamp; it saturates once, at the end, after the
 * material fold. So folding prim into diffuse and ambient computes
 * clamp5(light2 * prim/255 + light1 * prim/255 * dot), and wherever the source
 * sum would have saturated the port overshoots and washes toward white instead
 * of settling on prim.
 *
 * That is BUGS.md's "face color is slightly different from body color" for
 * Pikachu, Kirby and Jigglypuff, all three at once. The TEXTURED face runs take
 * their tint from the palette through the DS texture modulate, which IS a
 * post-shade multiply, so they are unaffected; only the untextured tinted body
 * diverges. Measured across three fighters and both details, 32 to 132 of each
 * few hundred channel samples diverge, worst at high dot. The owner's reading --
 * that Kirby's pink face looks more correct than his body -- is the right way
 * round: the face is correct and the body is not.
 *
 * The engine cannot express the inner clamp, so hold the one point that is
 * unambiguous: a fully lit vertex must land on prim, never on white. Capping
 * diffuse at prim - ambient does exactly that, per channel, and leaves the
 * unlit end untouched. The mid-tone ramp then reaches prim at dot = 1 instead
 * of at the source's earlier saturation point. This is a deliberate fidelity
 * trade under PROJECT_GOAL's render-side policy: not pixel-exact, and a
 * strictly smaller error than losing the tint.
 *
 * Only the prim-folded case is touched. With use_material clear the source has
 * no prim multiply either, so that path already agrees and is left alone. */
static inline u32 ndsRendererR2ClampDiffuseChannel(
    u32 diffuse, u32 ambient, u32 prim, u32 shift)
{
    u32 d = (diffuse >> shift) & 0x1fu;
    u32 a = (ambient >> shift) & 0x1fu;
    u32 p = (prim >> shift) & 0x1fu;
    u32 headroom = (p > a) ? (p - a) : 0u;

    return (d > headroom) ? headroom : d;
}

/* P2-2p8 2026-09-27 ITCM swap: out (noinline, so no ITCM caller pulls it
 * back in) for ndsStageGxDraw; census rent under ~1,300 cycles a byte. */
static u16 __attribute__((noinline)) ndsRendererR2ClampDiffuseToMaterial(
    u32 diffuse, u32 ambient, u32 material_color, u32 use_material,
    u32 color_modulate)
{
    u32 prim;
    u32 r;
    u32 g;
    u32 b;
    u16 capped;

    if (use_material == 0u)
    {
        return (u16)diffuse;
    }
    /* A WHITE PRIM IS THE IDENTITY FOLD, SO IT MUST NOT BE CAPPED.
     *
     * With prim = 0xffffff the source's material path is
     * clamp8(light2 + light1 * dot) * 255/255 -- literally the no-material
     * path. The two agree in the source, and they agreed in the port too,
     * because folding white changes nothing. Capping diffuse at prim - ambient
     * broke that agreement: measured on Kirby, a no-material epoch wrote
     * diffuse (31,31,31) while a white-prim epoch beside it wrote (22,22,22)
     * against the same (9,9,9) ambient. Both reach 31 at full light, but they
     * climb on different curves, and that seam between two runs on one fighter
     * is the owner's "face and body are two distinct colours". Holding neutral
     * B appeared to cure it only because changing model parts moves the seam
     * off that boundary.
     *
     * So cap only where the fold actually darkens. The tinted case the cap was
     * written for -- Pikachu 0xFFD933, Kirby 0x00FF5A, Purin 0xFFCDD8 -- is
     * unaffected and still lands on prim at full light. */
    if (((material_color >> 8) & 0x00ffffffu) == 0x00ffffffu)
    {
        return (u16)diffuse;
    }
    /* Full white light through the same fold IS the modulated prim colour, so
     * the cap comes from the identical arithmetic, not a second copy of it. */
    prim = ndsRendererR2MaterialColor15(0xffffffffu, material_color,
                                        use_material, color_modulate);
    r = ndsRendererR2ClampDiffuseChannel(diffuse, ambient, prim, 0u);
    g = ndsRendererR2ClampDiffuseChannel(diffuse, ambient, prim, 5u);
    b = ndsRendererR2ClampDiffuseChannel(diffuse, ambient, prim, 10u);
    capped = (u16)RGB15(r, g, b);
    if (capped != (u16)diffuse)
    {
        NDS_DIAG(gNdsShadeClampAppliedCount++);
    }
    return capped;
}

/* THE PRIM MULTIPLY BELONGS AFTER THE SHADE, NOT INSIDE IT.
 *
 * An untextured material epoch with a non-white prim takes the tint route when
 * a tile for its prim is resident: diffuse/ambient carry the RAW light colours
 * and the run modulates by a tile whose texels are all prim, which is
 * clamp31(RGB5(l2) + RGB5(l1)*dot) * prim -- the source's clamp-then-multiply
 * order, and the same arithmetic the textured face beside it already gets. A
 * colour with no tile yet (its first frame, or a full table) keeps the capped
 * fold; ndsRendererR2FighterTintLookup has queued it for the frame boundary.
 *
 * Textured epochs never tint: only the run prepare's untextured branch puts
 * prim back, so a textured epoch that took raw light here would lose prim.
 *
 * The colour modulate (hurt flash, a lerp toward a flash colour) is applied to
 * the raw light like any other shade, so a flashing tinted body lerps toward
 * flash x prim -- the same thing the textured face does (flash x texel) --
 * rather than declining and switching routes mid-flash.
 *
 * OUT OF ITCM ON PURPOSE: once per epoch, and the fighter ITCM section has no
 * room for the body (an inlined first version overflowed it by 1,416 bytes).
 *
 * Returns diffuse | (ambient << 16) and publishes sNdsEpochTintTexture. */
static u32 __attribute__((noinline)) ndsRendererR2ResolveEpochShade(
    u32 light1, u32 light2, u32 material_color, u32 use_material,
    u32 color_modulate, u32 textured)
{
    u32 tint = 0u;
    u32 diffuse;
    u32 ambient;

    if ((use_material != 0u) && (textured == 0u) &&
        (((material_color >> 8) & 0x00ffffffu) != 0x00ffffffu))
    {
        tint = ndsRendererR2FighterTintLookup(material_color);
    }
    sNdsEpochTintTexture = tint;
    sNdsEpochTintRgb = (material_color >> 8) & 0x00ffffffu;
    if (tint != 0u)
    {
        diffuse = ndsRendererR2MaterialColor15(light1, 0u, 0u, color_modulate);
        ambient = ndsRendererR2MaterialColor15(light2, 0u, 0u, color_modulate);
    }
    else
    {
        diffuse = ndsRendererR2MaterialColor15(
            light1, material_color, use_material, color_modulate);
        ambient = ndsRendererR2MaterialColor15(
            light2, material_color, use_material, color_modulate);
        diffuse = ndsRendererR2ClampDiffuseToMaterial(
            diffuse, ambient, material_color, use_material, color_modulate);
    }
    /* Composed with the same expression the replay twin uses, deliberately:
     * check-shade-twin.py finds the derivation sites by that shape. */
    return diffuse | (ambient << 16);
}

/* Bind the run's tint tile BEFORE the packet's prepare hook, so the hook
 * records the tile's TEXIMAGE_PARAM/PAL_FORMAT as this run's texture exactly as
 * it records any cache texture -- and, finding no cache entry active, marks
 * the packet needs_fence. Replay then binds the tile itself; nothing about the
 * tint has to be re-derived except the raw-light shade words. */
static void __attribute__((noinline)) ndsRendererR2BindTintTile(
    NDSRendererStats *stats, u32 tint)
{
    ndsRendererHardwareBindForeignTextureName(stats, tint);
}

/* The tinted run's batch, and the one texcoord that serves every vertex of it:
 * the DS texcoord register is sticky and every texel of the tile is prim, so a
 * single GFX_TEX_COORD at the tile centre (1/16-texel units) covers the run
 * under either wrap mode and the untextured vertex emitter stays untouched.
 * The packet records the same word after its BEGIN, so replay matches. */
static void __attribute__((noinline)) ndsRendererR2OpenTintBatch(
    NDSRendererStats *stats, u32 tint, u32 poly_fmt, u32 matrix_generation)
{
    u32 word = (u32)TEXTURE_PACK((NDS_FIGHTER_TINT_DIM / 2u) << 4,
                                 (NDS_FIGHTER_TINT_DIM / 2u) << 4);

    ndsRendererNativeBeginDirectBatch(stats, TRUE, tint, poly_fmt,
                                      matrix_generation);
    GFX_TEX_COORD = word;
}


/* R2-03 E28. Once E16 moved the fighter's diffuse term onto the geometry
 * engine, the software light direction and the shade LUT became dead: both are
 * read only inside the per-dense-vertex loop that `hardware_lit` skips. The
 * exception is the M2 detailed ledger, which reads
 * state->prepared_light_direction after the shade returns, so it keeps the
 * software preparation.
 *
 * NDS_FIGHTER_SHADE_PROOF hashes `prepared_direction`; with the skip active
 * that field is NULL, so the proof is only comparable against another skipped
 * build. It is a lab flag and never ships with this one.
 *
 * The #define itself now lives above ndsRendererNativeBindProductionRoot,
 * because that function's matrix copies are gated on it. */

static s32 NDS_RENDERER_NATIVE_FIGHTER_CODE
ndsRendererNativeShadeProductionActions(
    const NDSNativeEpoch *epoch,
    u32 epoch_policy,
    u32 packet_mode,
    NDSRendererStats *stats,
    NDSRendererTraversalState *state)
{
    const NDSNativeDirectPolicy *policy =
        &sNdsNativeFighterDirectPolicies[
            epoch_policy & NDS_NATIVE_DIRECT_POLICY_FAMILY_MASK];
    const NDSRendererHardwareLightDirection *prepared_direction = NULL;
    const u32 *shade_lut = NULL;
    /* R2-03 E47. The native owner baked both of these; the generic path derives
     * both from `stats` per draw, and on hitlag frames they disagree.
     *
     * `ndsRendererHardwareColorSource` picks `env_color` whenever the combiner
     * outputs ENVIRONMENT and `prim_color` only otherwise, and returns 0 when
     * the epoch has no combiner at all. The native owner used `prim_color`
     * unconditionally. `ndsRendererHardwareUseMaterialColor` is likewise a
     * predicate over the live combiner -- for a lit-shade combine it reduces to
     * `UsesLitPrimitiveModulate(stats)` -- while the policy flag is fixed at
     * generation time.
     *
     * E34 measured `prim_color`/`env_color` as the only per-epoch state that
     * varies at runtime, and Task 39's hurt flash is what varies them, so
     * hitlag frames are exactly where a baked answer goes wrong: the struck
     * fighter came out dark maroon where the generic path draws light grey
     * (E32, `artifacts/visibility/e32-compare-480.png`). E41 had already
     * excluded the fold arithmetic and E16's hardware lighting by three-way
     * capture, and E36 excluded `color_modulate`.
     *
     * This runs once per epoch (46.4/frame), not per vertex, so deriving it is
     * not on any hot path. A rendering-correctness fix owed regardless of E32 --
     * E32 only decides whether these frames reach the native owner at all. */
    u32 use_material =
        policy->vertex_flags & NDS_RENDERER_VERTEX_CONTEXT_USE_MATERIAL;
    u32 material_color = (use_material != 0u) ? stats->prim_color : 0u;
    u32 action_offset;
    /* Identical to `prepared_direction != NULL` in every build that prepares
     * the direction, and still correct in the build that skips it. */
    u32 epoch_lit = FALSE;
    u32 hardware_lit = FALSE;

    /* Cleared for EVERY epoch, not only the lit ones: the run prepare reads it
     * unconditionally, and an epoch that never reaches the shade write below
     * must not inherit the previous epoch's tile. */
    sNdsEpochTintTexture = 0u;
    if (epoch->action_count == 0u)
    {
        return TRUE;
    }
    ndsRendererNativeSourceBoundary(state);
    if (((stats->geometry_mode & NDS_RENDERER_GEOM_LIGHTING) != 0u) &&
        ((stats->light_dir_mask & NDS_RENDERER_LIGHT_DIR_1_MASK) != 0u))
    {
        epoch_lit = TRUE;
    }

    ndsRendererR2RecordShadeWitness(
        epoch_lit,
        0u,
        use_material, material_color);
    /* R2-03 E16. The per-dense-vertex shading below becomes one register write:
     * the engine evaluates the dot product per vertex from GFX_NORMAL.
     *
     * This sets a flag rather than returning, because the action walk below is
     * not only shading -- it also advances stats->vertex_count and
     * gNdsRendererProfileSourceVertexLoadCount. An earlier revision returned
     * here and drew identical geometry while leaving that load count at zero,
     * which the Boundary harness caught as the complete-stage owner having
     * entered the generic transform path. Only the inner loop is skipped. */
    if ((epoch_lit != FALSE)
        )
    {
        u32 diffuse;
        u32 ambient;

        if (sNdsLightVectorWritten == 0u)
        {
            ndsRendererR2WriteLightVector(stats);
        }
        u32 shade = ndsRendererR2ResolveEpochShade(
            stats->light_color_1, stats->light_color_2, material_color,
            use_material, state->color_modulate, policy->textured);
        u32 tinted = (sNdsEpochTintTexture != 0u) ? 1u : 0u;

        diffuse = shade & 0xffffu;
        ambient = shade >> 16;

        gNdsShadeWitness[(gNdsShadeWitnessWrites - 1u) %
                           NDS_SHADE_WITNESS_SLOTS].shade =
            diffuse | (ambient << 16);
        ndsRendererHardwareWriteDiffuseAmbient(diffuse | (ambient << 16));
        (void)tinted;
        hardware_lit = TRUE;
    }
    for (action_offset = 0u;
         action_offset < epoch->action_count;
         action_offset++)
    {
        u32 action_index = epoch->first_action + action_offset;
        const NDSNativeVertexAction *action =
            &sNdsNativeFighterActiveTables->vertex_actions[action_index];

        if (action->kind == NDS_NATIVE_VERTEX_BLOCK)
        {
            u32 vertex_end = (u32)action->index + (u32)action->count;

            if (vertex_end > stats->vertex_count)
            {
                stats->vertex_count = vertex_end;
            }
            if (packet_mode == 0u)
            {
                sNdsRendererHardwareSourceVertexLoadCount += action->count;
            }
        }
        /* R2-03 E29 / P2-2. The software shade span has no consumer in the
         * hardware-light owner. Do not even retain its generated Low-detail
         * alias/span tables in this configuration; the action itself remains
         * because source vertex-count accounting is part of the contract. */
        (void)hardware_lit;
    }
    return TRUE;
}

/* sNdsNativeFighterRuns[67]. Shared by the E5 proof and the E12 memo, so it
 * lives outside both flags' guards. */
#define NDS_RUN_MEMO_MAX 67u

/* E12 counters live outside the flag so a build with the memo off reads five
 * honest zeros instead of whatever the linker left at those addresses. Two
 * probes this cycle read an ARM opcode (0xEA80003B) out of a garbage-collected
 * counter and had to be re-run; the verifier reads these, so it cannot afford
 * that ambiguity. Only the increments are conditional.
 *
 * `volatile` is not enough. It stops the compiler from folding accesses, not
 * the linker from dropping an object nothing references -- and at MEMO=1
 * nothing writes VerifyFail, because only the level-2 verify does. `retain`
 * emits SHF_GNU_RETAIN so --gc-sections keeps it and a zero reads as zero. */
#define NDS_TEXMEMO_COUNTER __attribute__((used, retain))
volatile u32 NDS_TEXMEMO_COUNTER gNdsTexMemoHitCount;
volatile u32 NDS_TEXMEMO_COUNTER gNdsTexMemoMissCount;
volatile u32 NDS_TEXMEMO_COUNTER gNdsTexMemoFillCount;
volatile u32 NDS_TEXMEMO_COUNTER gNdsTexMemoStaleCount;
volatile u32 NDS_TEXMEMO_COUNTER gNdsTexMemoVerifyFail;
/* Player-slot memo columns dropped because the fighter's material identity
 * moved (ndsRendererR2RunTextureMemoFence). */
volatile u32 NDS_TEXMEMO_COUNTER gNdsTexMemoFenceCount;

/* E12. E5 proved every field below is invariant in run_index over a whole
 * canonical match, and E11 measured what recomputing them costs: 45.3 of 60.9
 * calls take the full texture prepare at 1,013 ticks, because the caller resets
 * texture_prepare_valid per DObj. The work skipped is SyncTextureTile, the
 * ~30-field key build, its hash, and the cache lookup.
 *
 * What is NOT skipped, because the resolver's cache-hit tail has live effects
 * the frame depends on: refreshing last_used_frame (this is the eviction LRU --
 * dropping it would let the entry the memo points at be reclaimed), the name and
 * param binds, the active-entry pointer, the pinned static-texture hit, and the
 * four stats fields.
 *
 * E5 deliberately excluded resolved->entry from its STABLE hash: "a pointer into
 * the hardware texture cache, which rotates for reasons unrelated to what is
 * drawn". So identity being stable is not residency being stable, and the memo
 * revalidates before trusting itself using readiness, name, and the cache key
 * generation. The generation is the identity fence: libnds may recycle a
 * deleted GL name for different texels in the same slot. A stale entry falls
 * through to the full path and refills. */
typedef struct NDSRunTextureMemo
{
    /* Slot index, not a pointer: sNdsRendererHardwareActiveTextureEntry is a
     * const pointer and the memo must refresh last_used_frame, so an index into
     * the cache array is both writable and cheaper to validate than a cast. */
    u32 entry_generation;
    /* `run_index` identifies immutable generated geometry, not a live fighter.
     * Mario/Mario (or Fox/Fox) therefore executes the same run indices twice in
     * one frame.  The full texture resolver keys costume-sensitive bakes on live
     * PRIM/ENV state; without the owner key below, the second fighter could reuse
     * the first fighter's baked texture merely because the cache entry was still
     * resident.  The key also separates High/Low generated programs and later
     * roster owners.  Texture-cache `key_generation` below is the scene/VRAM
     * lifetime fence, so duplicating taskman generation here would add a hot
     * compare without adding ownership information. */
    u32 owner_key;
    u16 name;
    u16 width;
    u16 height;
    u16 scale_s;
    u16 scale_t;
    u16 origin_s;
    u16 origin_t;
    s16 offset;
    u8 slot_plus1;
    u8 format;
    u8 valid;
    u8 run;         /* the run this row holds (rows are run_index mod 67) */
} NDSRunTextureMemo;
_Static_assert(sizeof(NDSRunTextureMemo) == 28u,
               "run texture memo must retain the compact DS field layout");

/* One live fighter instance per source player slot.  A single row per run was
 * correct for one fighter but pathological for multiplayer: slot 1 replaced
 * slot 0's owner_key, slot 2 replaced slot 1's, and so on, so the next frame
 * every fighter missed and rebuilt the same resident texture identity again.
 *
 * texture_memo_owner_key already carries the source player slot in bits 10:9
 * (renderer_adapter_fighter.c) and retains owner/detail/costume/shade in the
 * row itself.  Indexing by those two slot bits therefore gives the four live
 * instances independent residency records without weakening any identity
 * check. */
#define NDS_RUN_TEXMEMO_PLAYER_COUNT 4u
static NDSRunTextureMemo
    sNdsRunTextureMemo[NDS_RUN_MEMO_MAX]
                          [NDS_RUN_TEXMEMO_PLAYER_COUNT];

/* Rows by run index modulo 67, tagged with the run. A P4 content's owner
 * image appends its model parts' runs after the canonical ones (Banjo's runs
 * reach 160), so a draw's runs reached past the 67 rows and every one of
 * those took the full resolver at every materialization; a list draws ~60,
 * and runs sharing a residue only take turns. Run indices fit the tag byte
 * (the generator's u8 run counts); a larger one is never memoized. */
#define NDS_RUN_TEXMEMO_RUN_MAX 0xffu
static inline NDSRunTextureMemo *ndsRendererR2RunTextureMemoFor(
    u32 run_index, u32 owner_key)
{
    u32 row = run_index;

    while (row >= NDS_RUN_MEMO_MAX)
    {
        row -= NDS_RUN_MEMO_MAX;
    }
    return &sNdsRunTextureMemo[row][(owner_key >> 9) & 3u];
}

/* THE MEMO IS FENCED ON THE FIGHTER'S LIVE MATERIAL STATE.
 *
 * E5 found every memoised field invariant in run_index over its canonical
 * match. That held only while texture parts never reached a material: until
 * r51 the texture-part container was read off the wrong byte lane, so most
 * fighters' expressions went nowhere. With blinks and damage faces live, an
 * eye run's image changes while its owner key, cache entry and generation do
 * not, and the memo kept binding whatever the run showed when it was filled.
 * A CSS preview is first drawn mid-blink more often than not, so on r51 every
 * fighter with eyes sat half or fully shut in the CSS (owner), while the MObjs
 * read id 0 (traced).
 *
 * The adapter already folds every selected MObj's texture ids, palette id,
 * fraction and colours into sNdsFighterPacketMaterialIdentity for the packet
 * key. When that word moves for a player slot, the slot's rows are dropped and
 * the next draw refills them from the full resolver: one full resolve per
 * material change, not per frame. */
static u32 sNdsRunTextureMemoIdentity[NDS_RUN_TEXMEMO_PLAYER_COUNT];

void ndsRendererR2RunTextureMemoFence(u32 owner_key, u32 material_identity)
{
    u32 slot = (owner_key >> 9) & 3u;
    u32 run_index;

    if (sNdsRunTextureMemoIdentity[slot] == material_identity)
    {
        return;
    }
    sNdsRunTextureMemoIdentity[slot] = material_identity;
    for (run_index = 0u; run_index < NDS_RUN_MEMO_MAX; run_index++)
    {
        sNdsRunTextureMemo[run_index][slot].valid = 0u;
    }
    gNdsTexMemoFenceCount++;
}

/* The cheap tail of the resolver's cache-hit path, replayed from the memo. */
static s32 __attribute__((noinline)) ndsRendererR2RunTextureMemoApply(
    u32 run_index,
    NDSRendererStats *stats,
    u32 *texture_name,
    u32 *scale_s,
    u32 *scale_t,
    u32 *origin_s,
    u32 *origin_t,
    s32 *offset)
{
    NDSRunTextureMemo *memo;
    NDSRendererHardwareTextureCacheEntry *entry;

    if (run_index > NDS_RUN_TEXMEMO_RUN_MAX)
    {
        return FALSE;
    }
    memo = ndsRendererR2RunTextureMemoFor(
        run_index, sNdsNativeFighterOwnerExecution.texture_memo_owner_key);
    if ((memo->valid == 0u) || (memo->run != (u8)run_index))
    {
        gNdsTexMemoMissCount++;
        return FALSE;
    }
    if (memo->owner_key !=
        sNdsNativeFighterOwnerExecution.texture_memo_owner_key)
    {
        /* This is an identity miss, not a stale hardware-cache entry.  Leave
         * the resident texture untouched and let the source-equivalent full
         * resolver below choose the live instance's key, then refill. */
        gNdsTexMemoMissCount++;
        return FALSE;
    }
    entry = &sNdsRendererHardwareTextureCache[memo->slot_plus1 - 1u];
    if ((entry->ready == FALSE) || ((u32)entry->name != memo->name) ||
        (entry->key_generation != memo->entry_generation))
    {
        memo->valid = 0u;
        gNdsTexMemoStaleCount++;
        return FALSE;
    }

    entry->last_used_frame = sNdsRendererHardwareFrameSerial + 1u;
    if (entry->pinned != 0u)
    {
        ndsRendererHardwareBindTextureName(stats, memo->name);
        ndsRendererHardwareApplyTextureParams(entry->params);
        sNdsRendererHardwareActiveTextureEntry = entry;
        ndsRendererHardwareRecordBattleStaticTextureHit(entry);
    }
    else if (sNdsRendererHardwareActiveTextureEntry != entry)
    {
        ndsRendererHardwareBindTextureName(stats, memo->name);
        ndsRendererHardwareApplyTextureParams(entry->params);
        sNdsRendererHardwareActiveTextureEntry = entry;
    }
    stats->hardware_texture_ready_count++;
    stats->hardware_texture_format = memo->format;
    stats->hardware_texture_width = memo->width;
    stats->hardware_texture_height = memo->height;

    *texture_name = memo->name;
    *scale_s = memo->scale_s;
    *scale_t = memo->scale_t;
    *origin_s = memo->origin_s;
    *origin_t = memo->origin_t;
    *offset = memo->offset;
    gNdsTexMemoHitCount++;
    return TRUE;
}

static void __attribute__((noinline)) ndsRendererR2RunTextureMemoFill(
    u32 run_index,
    const NDSRendererStats *stats,
    u32 texture_name,
    u32 scale_s,
    u32 scale_t,
    u32 origin_s,
    u32 origin_t,
    s32 offset)
{
    NDSRunTextureMemo *memo;
    const NDSRendererHardwareTextureCacheEntry *entry =
        sNdsRendererHardwareActiveTextureEntry;
    u32 slot;

    /* Only bake an entry the resolver actually landed on. A run whose bind took
     * the stage-site shortcut or the no-texture path leaves no active entry, and
     * baking one from a neighbouring run's state is exactly the class of bug the
     * revalidation above cannot catch. */
    if ((run_index > NDS_RUN_TEXMEMO_RUN_MAX) || (entry == NULL) ||
        (entry->ready == FALSE) || ((u32)entry->name != texture_name) ||
        (texture_name == 0u))
    {
        return;
    }
    slot = (u32)(entry - sNdsRendererHardwareTextureCache);
    if (slot >= NDS_RENDERER_HW_TEXTURE_CACHE_COUNT)
    {
        return;
    }
    /* Keeps the level-2 counter linked at level 1. `volatile` stops the
     * compiler folding this away and `retain` was accepted but did not survive
     * this linker, so the symbol needs a genuine reference or --gc-sections
     * drops it -- and the verifier then reads whatever sits at its address
     * (0xEA80003D, an ARM opcode, on the run that caught this). Nine fills a
     * match, so the load/store is free. */
    gNdsTexMemoVerifyFail = gNdsTexMemoVerifyFail;
    memo = ndsRendererR2RunTextureMemoFor(
        run_index, sNdsNativeFighterOwnerExecution.texture_memo_owner_key);
    /* Preserve the full resolver for values outside the compact memo's
     * representation. This only declines caching; it never truncates a draw.
     * Texture parameters already live in the generation-checked cache entry. */
    if ((slot >= 255u) || (texture_name > 0xffffu) ||
        (stats->hardware_texture_format > 0xffu) ||
        (stats->hardware_texture_width > 0xffffu) ||
        (stats->hardware_texture_height > 0xffffu) ||
        (scale_s > 0xffffu) || (scale_t > 0xffffu) ||
        (origin_s > 0xffffu) || (origin_t > 0xffffu) ||
        (offset < -32768) || (offset > 32767))
    {
        memo->valid = 0u;
        return;
    }
    memo->slot_plus1 = slot + 1u;
    memo->name = texture_name;
    memo->entry_generation = entry->key_generation;
    memo->format = stats->hardware_texture_format;
    memo->width = stats->hardware_texture_width;
    memo->height = stats->hardware_texture_height;
    memo->scale_s = scale_s;
    memo->scale_t = scale_t;
    memo->origin_s = origin_s;
    memo->origin_t = origin_t;
    memo->offset = offset;
    memo->owner_key = sNdsNativeFighterOwnerExecution.texture_memo_owner_key;
    memo->run = (u8)run_index;
    memo->valid = 1u;
    gNdsTexMemoFillCount++;
}


/* Cycle 110, Requirement 3: the prepared dense UVs are IMMUTABLE fighter state,
 * not per-frame work.
 *
 * Whole-match proof at NDS_FIGHTER_RUN_PROOF=2: the UV loop performed
 * **246,736 writes** and produced **106 distinct dense vertices**, with
 * `gNdsUvChangeCount == 0` — not one of those writes ever changed a value —
 * and `gNdsUvOutOfRange == 0`, so the proof array covered every id. 154 writes
 * a frame to re-derive a constant.
 *
 * `dense->s/t` is baked asset data and these five are the entire remaining
 * dependency, so recording the ones that produced a run's current contents says
 * exactly when the loop would rewrite what is already there. Compared, not
 * hashed: five compares are cheaper than five multiplies and leave no collision
 * to argue about. 1,340 bytes of bss for 67 runs, and nothing new is cached —
 * `sNdsNativeFighterPreparedDense` already existed and already held this answer.
 *
 * What a per-run key cannot see is two runs sharing a dense id with different
 * inputs. That is a property of the BAKED `sNdsNativeFighterRunUniqueDense`
 * table, not of the frame, and `gNdsUvChangeCount` is its detector. */
typedef struct NDSNativeFighterRunUvInputs
{
    u32 scale_s;
    u32 scale_t;
    u32 origin_s;
    u32 origin_t;
    s32 offset;
    /* High and low programs reuse run indices but do not reuse dense IDs.  The
     * source can cross that detail boundary between a 2-player and 3+ player
     * scene without changing the texture metrics, so table identity is part of
     * the memo key. */
    const NDSNativeFighterRuntimeTables *tables;
    /* The arena fence, same key the fighter material block uses. A restart
     * rewinds the taskman heap and could reload a different dense table behind
     * the same run index with the same texture metrics; without this the stamp
     * would skip on the strength of metrics that no longer describe the
     * vertices. One compare, and the P1 milestone restarts from Results. */
    u32 heap_generation;
} NDSNativeFighterRunUvInputs;

/* P2-2p8: four-player battle alternates independent runtime table images
 * through this one renderer. A single run-index stamp made each table evict
 * the previous table even though their prepared_dense arrays are independent,
 * forcing the same immutable UVs to be rebuilt every frame.
 *
 * Key this residency by the storage that actually owns the prepared result,
 * not by player slot. Mirrors intentionally share one runtime table and one
 * prepared_dense array; giving them separate player stamps would allow player
 * A to skip after player B overwrote that shared array. Table identity avoids
 * that stale-hit class: equal tables compare the full UV inputs, while distinct
 * tables retain independent stamps. Four ways cover the maximum four distinct
 * fighter table images alive in a match. Auxiliary namespaces (Kirby copy-hat
 * and Link boomerang) may evict one row temporarily; that is only a performance
 * miss, because eviction forces a rebuild and can never authorize a stale skip.
 */
#define NDS_RUN_UVMEMO_WAYS 4u
static NDSNativeFighterRunUvInputs
    sNdsNativeFighterRunUvInputs[NDS_RUN_MEMO_MAX]
                                      [NDS_RUN_UVMEMO_WAYS];
static u8 sNdsNativeFighterRunUvVictim[NDS_RUN_MEMO_MAX];
/* Rows dropped because the image they described was bound or unbound. A CSS
 * browse that revisits fighters must move this; a match must not, except at a
 * Kirby copy. */
volatile u32 gNdsNativeFighterRunUvForgotten;

/* THE MEMO VOUCHES FOR BYTES IN A RELOADABLE IMAGE, SO A RELOAD MUST FORGET IT.
 *
 * `prepared_dense` -- where ndsRendererNativeRebuildProductionRunUv writes the
 * s/t this memo vouches for -- lives INSIDE the owner image
 * (ndsRendererNativeBindOwnerImage: "Image RAM owns mutable prepared-dense
 * scratch"), and generate_nds_native_owner_images.py ships it with positions
 * only, so every freshly loaded image carries s = t = 0. The memo's key is the
 * tables STRUCT -- static, one per owner and detail -- plus
 * gNdsTaskmanHeapGeneration. A CSS preview released and loaded again (hover
 * away from a fighter and back) changes neither: same struct, same arena
 * generation, same texture metrics. The first draw after the reload therefore
 * hit this memo, skipped the rebuild, and sampled every textured vertex of the
 * run at texel (0,0) -- Link gray, an eye with no pupil, a face the wrong
 * colour, and more of it the longer the CSS was browsed, because each revisit
 * adds a fighter and the 4-way rows age out one run at a time (which is how ONE
 * eye goes blank while the other survives). A Kirby copy hat reloaded into its
 * fixed per-slot struct mid-battle has the same hole.
 *
 * NDS_IMG_BIND calls this on every bind and unbind. Clearing `tables` returns
 * the row to the free pool ReserveMiss already prefers, and a NULL row can
 * never match a live lookup, so the next draw of that owner rebuilds. Cold:
 * 67 x 4 compares per image load, never per frame. */
static void ndsRendererNativeForgetFighterRunUvTables(const void *tables)
{
    u32 run_index;
    u32 way;

    if (tables == NULL)
    {
        return;
    }
    for (run_index = 0u; run_index < NDS_RUN_MEMO_MAX; run_index++)
    {
        for (way = 0u; way < NDS_RUN_UVMEMO_WAYS; way++)
        {
            NDSNativeFighterRunUvInputs *uv =
                &sNdsNativeFighterRunUvInputs[run_index][way];

            if ((const void *)uv->tables == tables)
            {
                uv->tables = NULL;
                NDS_DIAG(gNdsNativeFighterRunUvForgotten++);
            }
        }
    }
}

static inline __attribute__((always_inline))
NDSNativeFighterRunUvInputs *ndsRendererNativeFighterRunUvLookup(
    u32 run_index)
{
    u32 way;

    if (run_index >= NDS_RUN_MEMO_MAX)
    {
        return NULL;
    }
    for (way = 0u; way < NDS_RUN_UVMEMO_WAYS; way++)
    {
        NDSNativeFighterRunUvInputs *uv =
            &sNdsNativeFighterRunUvInputs[run_index][way];

        if (uv->tables == sNdsNativeFighterActiveTables)
        {
            return uv;
        }
    }
    return NULL;
}

static NDSNativeFighterRunUvInputs *NDS_RENDERER_NATIVE_FIGHTER_MAIN_CODE
ndsRendererNativeFighterRunUvReserveMiss(
    u32 run_index)
{
    NDSNativeFighterRunUvInputs *uv;
    u32 way;

    if (run_index >= NDS_RUN_MEMO_MAX)
    {
        return NULL;
    }
    /* Called only after the hot lookup has proved this table absent.  Do not
     * repeat that six-way search here: besides doing duplicate work on a miss,
     * inlining both searches overflowed the 32 KiB native-fighter ITCM by 88 B.
     * Miss allocation is cold setup work, so keep it in main RAM. */
    for (way = 0u; way < NDS_RUN_UVMEMO_WAYS; way++)
    {
        uv = &sNdsNativeFighterRunUvInputs[run_index][way];
        if (uv->tables == NULL)
        {
            return uv;
        }
    }
    way = (u32)sNdsNativeFighterRunUvVictim[run_index];
    if (way >= NDS_RUN_UVMEMO_WAYS)
    {
        way = 0u;
    }
    sNdsNativeFighterRunUvVictim[run_index] =
        (u8)(((way + 1u) < NDS_RUN_UVMEMO_WAYS) ? (way + 1u) : 0u);
    return &sNdsNativeFighterRunUvInputs[run_index][way];
}
#if NDS_TICK_HUD
/* Engagement proof. Skip should be ~99.96% of calls; a Build rate above the
 * 106 first-fills means the invariant above stopped holding. */
volatile u32 gNdsRunUvSkip;
volatile u32 gNdsRunUvBuild;
#endif

#if NDS_LINK
typedef struct NDSNativeTexgenDirectionQ15
{
    s32 x;
    s32 y;
    s32 z;
} NDSNativeTexgenDirectionQ15;

/* P2-3f31. BattleShip Fast3D's G_TEXTURE_GEN path transforms each LookAt
 * direction by the current modelview, normalizes it, dots the source s8 vertex
 * normal against that direction, clamps the dot to [-1, 1], then evaluates
 *
 *     tc = ((dot + 1) / 4) * gSPTexture.scale
 *
 * for regular (non-LINEAR) texgen. Link is the first production owner to make
 * that state observable: one epoch in each detail level.
 *
 * Keep the transformed direction at Q15 rather than quantizing it back to s8.
 * The source uses float coefficients after normalization, while the DS source
 * modelview is already exact Q20.12 fixed point. This keeps all available DS
 * precision and uses the ARM9 hardware sqrt/div unit already owned by this
 * renderer. */
static s32 ndsRendererNativePrepareTexgenDirectionQ15(
    const Light *look_at,
    const NDSRendererMatrix20p12 *modelview,
    NDSNativeTexgenDirectionQ15 *out)
{
    s32 dx;
    s32 dy;
    s32 dz;
    s64 transformed_x;
    s64 transformed_y;
    s64 transformed_z;
    u64 length_squared;
    u32 length;

    if ((look_at == NULL) || (modelview == NULL) || (out == NULL))
    {
        return FALSE;
    }
    dx = (s32)look_at->l.dir[0];
    dy = (s32)look_at->l.dir[1];
    dz = (s32)look_at->l.dir[2];
    transformed_x =
        (s64)dx * modelview->m[0][0] +
        (s64)dy * modelview->m[0][1] +
        (s64)dz * modelview->m[0][2];
    transformed_y =
        (s64)dx * modelview->m[1][0] +
        (s64)dy * modelview->m[1][1] +
        (s64)dz * modelview->m[1][2];
    transformed_z =
        (s64)dx * modelview->m[2][0] +
        (s64)dy * modelview->m[2][1] +
        (s64)dz * modelview->m[2][2];
    length_squared =
        (u64)(transformed_x * transformed_x) +
        (u64)(transformed_y * transformed_y) +
        (u64)(transformed_z * transformed_z);
    if (length_squared == 0u)
    {
        /* A joint scaled to zero (Link's 1P entry) leaves the LookAt no
         * direction in its space. Its vertices collapse onto one point, so the
         * run draws nothing whatever its texcoords; the RSP normalises the zero
         * vector to zero instead of failing the list, and so does this. It
         * used to reject the whole owner for those frames. */
        out->x = 0;
        out->y = 0;
        out->z = 0;
        return TRUE;
    }
    length = ndsHwMathSqrt64(length_squared);
    if ((length == 0u) || (length > (u32)INT_MAX))
    {
        return FALSE;
    }
    out->x = ndsHwMathDiv64(transformed_x * 32767, (s32)length);
    out->y = ndsHwMathDiv64(transformed_y * 32767, (s32)length);
    out->z = ndsHwMathDiv64(transformed_z * 32767, (s32)length);
    return TRUE;
}

/* G_TEXTURE_GEN_LINEAR maps the angle rather than its cosine: F3DEX2 computes
 * 0.5 + asin(d)/pi, i.e. acos(-d)/pi, over the same [0, 1] range the spherical
 * (d + 1)/2 covers. Q16 samples at d = i/128 - 1, interpolated linearly. */
static const u16 sNdsNativeTexgenLinearAcos[257] =
{
        0u,  2609u,  3692u,  4525u,  5229u,  5850u,  6412u,  6931u,
     7414u,  7869u,  8300u,  8711u,  9105u,  9483u,  9848u, 10200u,
    10542u, 10874u, 11197u, 11512u, 11819u, 12119u, 12413u, 12701u,
    12983u, 13260u, 13532u, 13799u, 14063u, 14322u, 14577u, 14828u,
    15077u, 15321u, 15563u, 15802u, 16037u, 16270u, 16501u, 16729u,
    16954u, 17178u, 17399u, 17618u, 17835u, 18050u, 18263u, 18474u,
    18684u, 18892u, 19098u, 19303u, 19506u, 19708u, 19909u, 20108u,
    20305u, 20502u, 20697u, 20891u, 21084u, 21276u, 21467u, 21656u,
    21845u, 22033u, 22219u, 22405u, 22590u, 22774u, 22958u, 23140u,
    23322u, 23502u, 23683u, 23862u, 24041u, 24219u, 24396u, 24573u,
    24749u, 24924u, 25099u, 25274u, 25447u, 25621u, 25793u, 25966u,
    26138u, 26309u, 26480u, 26650u, 26820u, 26990u, 27159u, 27328u,
    27496u, 27665u, 27832u, 28000u, 28167u, 28334u, 28501u, 28667u,
    28833u, 28999u, 29164u, 29330u, 29495u, 29660u, 29824u, 29989u,
    30153u, 30317u, 30481u, 30645u, 30809u, 30973u, 31136u, 31300u,
    31463u, 31626u, 31789u, 31952u, 32116u, 32279u, 32442u, 32605u,
    32768u, 32930u, 33093u, 33256u, 33419u, 33583u, 33746u, 33909u,
    34072u, 34235u, 34399u, 34562u, 34726u, 34890u, 35054u, 35218u,
    35382u, 35546u, 35711u, 35875u, 36040u, 36205u, 36371u, 36536u,
    36702u, 36868u, 37034u, 37201u, 37368u, 37535u, 37703u, 37870u,
    38039u, 38207u, 38376u, 38545u, 38715u, 38885u, 39055u, 39226u,
    39397u, 39569u, 39742u, 39914u, 40088u, 40261u, 40436u, 40611u,
    40786u, 40962u, 41139u, 41316u, 41494u, 41673u, 41852u, 42033u,
    42213u, 42395u, 42577u, 42761u, 42945u, 43130u, 43316u, 43502u,
    43690u, 43879u, 44068u, 44259u, 44451u, 44644u, 44838u, 45033u,
    45230u, 45427u, 45626u, 45827u, 46029u, 46232u, 46437u, 46643u,
    46851u, 47061u, 47272u, 47485u, 47700u, 47917u, 48136u, 48357u,
    48581u, 48806u, 49034u, 49265u, 49498u, 49733u, 49972u, 50214u,
    50458u, 50707u, 50958u, 51213u, 51472u, 51736u, 52003u, 52275u,
    52552u, 52834u, 53122u, 53416u, 53716u, 54023u, 54338u, 54661u,
    54993u, 55335u, 55687u, 56052u, 56430u, 56824u, 57235u, 57666u,
    58121u, 58604u, 59123u, 59685u, 60306u, 61010u, 61843u, 62926u,
    65535u,
};

/* P2-6 (2026-10-01): the two divisions below were two hardware 64/32 divides
 * a texgen vertex (330K cycles a frame of Polygon Team's polygons, 125K of
 * the Race's, 1p-pf04-st12 / 1p-pf03-st11). Both divisors are fixed
 * (2 * unit, and 8 * unit times the run's texture scale), so each is a
 * multiply by a 2^40 reciprocal; the scale's is scale * floor(2^48 / 8unit)
 * >> 8, no division at all. The dividend (dot + unit) is under 2^24, every
 * product under 2^64, and a quotient falls one 1/16 texel short only within
 * 2^-16 of a texel edge (host-checked over the whole scale range). */
#define NDS_NATIVE_TEXGEN_UNIT ((u32)127u * 32767u)
#define NDS_NATIVE_TEXGEN_SCALE_RECIP_Q48 \
    (((u64)1u << 48) / (8u * NDS_NATIVE_TEXGEN_UNIT))

static s32 ndsRendererNativeTexgenCoord(
    s32 nx,
    s32 ny,
    s32 nz,
    const NDSNativeTexgenDirectionQ15 *direction,
    u32 texture_scale,
    u32 linear)
{
    const s64 unit = (s64)NDS_NATIVE_TEXGEN_UNIT;
    s64 dot =
        (s64)nx * direction->x +
        (s64)ny * direction->y +
        (s64)nz * direction->z;

    if (dot > unit)
    {
        dot = unit;
    }
    else if (dot < -unit)
    {
        dot = -unit;
    }
    if (linear != 0u)
    {
        /* Q8.8 table position of d in [-1, 1]; the result is the spherical
         * expression below with (d + 1)/2 replaced by acos(-d)/pi. The
         * reciprocal is 2^40 * 65536 / (2 * unit). */
        u32 position = (u32)(((u64)(dot + unit) *
                              (u64)(((u64)1u << 56) /
                                    (2u * NDS_NATIVE_TEXGEN_UNIT))) >> 40);
        u32 index = position >> 8;
        u32 low;
        u32 high;
        u32 angle;

        if (index >= 256u)
        {
            angle = sNdsNativeTexgenLinearAcos[256];
        }
        else
        {
            low = sNdsNativeTexgenLinearAcos[index];
            high = sNdsNativeTexgenLinearAcos[index + 1u];
            angle = low + (((high - low) * (position & 0xffu)) >> 8);
        }
        return (s32)(((u64)angle * (texture_scale & 0xffffu)) >> 18);
    }
    /* Fast3D produces an N64 s/t value first and the DS FIFO uses 1/16-texel
     * coordinates where N64 source tc is 1/32 texel. The existing ordinary UV
     * path's >>17 is exactly (source * scale >>16) / 2. Since dot+unit is
     * non-negative, folding those two truncations is exactly division by
     * 8*unit here. */
    return (s32)(((u64)(dot + unit) *
                  (((u64)(texture_scale & 0xffffu) *
                    NDS_NATIVE_TEXGEN_SCALE_RECIP_Q48) >> 8)) >> 40);
}
#endif

/* The production UV memo miss is a setup/invalidated-data path, not steady
 * state. c200 executes the equality/validity guard but no instruction in this
 * rebuild body during the 1,600-frame gate window. Keep the exact writes and
 * proof hooks, but do not reserve ITCM for a branch the hot path does not take. */
static s32 __attribute__((noinline, cold))
ndsRendererNativeRebuildProductionRunUv(
    u32 run_index,
    u32 unique_first,
    u32 unique_count,
    const NDSRendererStats *stats,
    NDSRendererTraversalState *state,
    NDSNativeFighterRunUvInputs *uv)
{
    u32 unique_offset;
#if NDS_LINK
    u32 use_texgen =
        ((stats != NULL) &&
         ((stats->geometry_mode & NDS_RENDERER_GEOM_TEXTURE_GEN) != 0u)) ?
            TRUE : FALSE;
    u32 texgen_linear =
        ((use_texgen != FALSE) &&
         ((stats->geometry_mode & NDS_RENDERER_GEOM_TEXTURE_GEN_LINEAR) !=
          0u)) ? TRUE : FALSE;
    NDSNativeTexgenDirectionQ15 lookat_x;
    NDSNativeTexgenDirectionQ15 lookat_y;

    if (use_texgen != FALSE)
    {
        const LookAt *look_at = ndsRendererAdapterCurrentLookAt();

        if ((look_at == NULL) || (state == NULL) ||
            (state->modelview_valid == 0u) ||
            (ndsRendererNativePrepareTexgenDirectionQ15(
                 &look_at->l[0], &state->modelview,
                 &lookat_x) == FALSE) ||
            (ndsRendererNativePrepareTexgenDirectionQ15(
                 &look_at->l[1], &state->modelview,
                 &lookat_y) == FALSE))
        {
            return FALSE;
        }
    }
#endif

    for (unique_offset = 0u;
         unique_offset < unique_count;
         unique_offset++)
    {
        u32 dense_id = sNdsNativeFighterActiveTables->run_unique_dense[
            unique_first + unique_offset];
        const NDSNativeDenseVertex *dense =
            &sNdsNativeFighterActiveTables->dense_vertices[dense_id];
        NDSNativePreparedDenseVertex *prepared =
            &sNdsNativeFighterActiveTables->prepared_dense[dense_id];
#if NDS_LINK
        s32 scaled_s;
        s32 scaled_t;

        if (use_texgen != FALSE)
        {
            u32 rgba = dense->rgba;
            s32 nx = (s32)(s8)(rgba >> 24);
            s32 ny = (s32)(s8)(rgba >> 16);
            s32 nz = (s32)(s8)(rgba >> 8);

            scaled_s = ndsRendererNativeTexgenCoord(
                nx, ny, nz, &lookat_x, state->texture_prepare_scale_s,
                texgen_linear);
            scaled_t = ndsRendererNativeTexgenCoord(
                nx, ny, nz, &lookat_y, state->texture_prepare_scale_t,
                texgen_linear);
        }
        else
        {
            scaled_s =
                ((s32)dense->s *
                 (s32)state->texture_prepare_scale_s) >> 17;
            scaled_t =
                ((s32)dense->t *
                 (s32)state->texture_prepare_scale_t) >> 17;
        }
#else
        s32 scaled_s =
            ((s32)dense->s *
             (s32)state->texture_prepare_scale_s) >> 17;
        s32 scaled_t =
            ((s32)dense->t *
             (s32)state->texture_prepare_scale_t) >> 17;
#endif

        prepared->s = (s16)(
            scaled_s -
            ((s32)state->texture_prepare_origin_s << 2) +
            state->texture_prepare_offset);
        prepared->t = (s16)(
            scaled_t -
            ((s32)state->texture_prepare_origin_t << 2) +
            state->texture_prepare_offset);
    }
    if (uv != NULL)
    {
        uv->scale_s = state->texture_prepare_scale_s;
        uv->scale_t = state->texture_prepare_scale_t;
        uv->origin_s = state->texture_prepare_origin_s;
        uv->origin_t = state->texture_prepare_origin_t;
        uv->offset = state->texture_prepare_offset;
        uv->tables = sNdsNativeFighterActiveTables;
        uv->heap_generation = gNdsTaskmanHeapGeneration;
    }
#if NDS_TICK_HUD
    NDS_DIAG(gNdsRunUvBuild++);
#endif
    return TRUE;
}

static inline __attribute__((always_inline)) s32
ndsRendererNativePrepareProductionRunCore(
    u32 run_index,
    u32 epoch_policy,
    u32 packet_mode,
    const NDSRendererConfig *config,
    NDSRendererStats *stats,
    NDSRendererTraversalState *state,
    NDSNativeHierarchyPreparedRun *hierarchy_run)
{
    const NDSNativeDirectPolicy *policy;
    const NDSNativeRun *run;
    const NDSRendererTileState *render_tile;
    u32 family = epoch_policy & NDS_NATIVE_DIRECT_POLICY_FAMILY_MASK;
    u32 expected_geometry_cull =
        ((epoch_policy & NDS_NATIVE_DIRECT_POLICY_CULL_NONE) != 0u) ?
            0u : NDS_RENDERER_GEOM_CULL_BACK;
    u32 expected_poly_cull =
        ((epoch_policy & NDS_NATIVE_DIRECT_POLICY_CULL_NONE) != 0u) ?
            POLY_CULL_NONE : POLY_CULL_BACK;
    u32 geometry_cull;
    u32 run_unlit;
    u32 run_poly_alpha;
    u32 run_poly_light = 0u;
    u32 material_color;
    u32 use_texture;
    u32 texture_scale_s = 0u;
    u32 texture_scale_t = 0u;
    u32 texture_origin_s = 0u;
    u32 texture_origin_t = 0u;
    u32 unique_first;
    u32 unique_count;
    s32 texture_offset = 0;
    NDSRendererHardwareResolvedTexture resolved_texture;

    memset(&resolved_texture, 0, sizeof(resolved_texture));

    if ((config == NULL) || (stats == NULL) || (state == NULL) ||
        (run_index >= sNdsNativeFighterActiveTables->run_count) ||
        (family >= (sizeof(sNdsNativeFighterDirectPolicies) /
                    sizeof(sNdsNativeFighterDirectPolicies[0]))))
    {
        return ndsRendererNativeDirectReject(stats);
    }
    run = &sNdsNativeFighterActiveTables->runs[run_index];
    run_unlit = ndsRendererNativeRunUsesVertexColor(run);
    run_poly_alpha = (run_unlit != FALSE) ?
        ndsRendererNativeRunPolyAlpha(run) : 31u;
    if (run_unlit == FALSE)
    {
        run_poly_light = POLY_FORMAT_LIGHT0;
    }
    policy = &sNdsNativeFighterDirectPolicies[family];
    geometry_cull = stats->geometry_mode &
        (NDS_RENDERER_GEOM_CULL_FRONT | NDS_RENDERER_GEOM_CULL_BACK);
    if (((stats->geometry_mode & NDS_RENDERER_GEOM_ZBUFFER) == 0u) ||
        ((run_unlit == FALSE) &&
         ((stats->geometry_mode & NDS_RENDERER_GEOM_LIGHTING) == 0u)) ||
        ((run_unlit != FALSE) &&
         ((stats->geometry_mode & NDS_RENDERER_GEOM_LIGHTING) != 0u)) ||
#if NDS_LINK
        /* G_TEXTURE_GEN_LINEAR only selects the texgen mapping (the polygon
         * team's Link, Captain and Ness parts); without G_TEXTURE_GEN the RSP
         * ignores it. */
        ((stats->geometry_mode & NDS_RENDERER_GEOM_FOG) != 0u) ||
        ((((stats->geometry_mode & NDS_RENDERER_GEOM_TEXTURE_GEN) != 0u) &&
          ((policy->textured == 0u) || (state->modelview_valid == 0u)))) ||
#else
        ((stats->geometry_mode &
          (NDS_RENDERER_GEOM_FOG |
           NDS_RENDERER_GEOM_TEXTURE_GEN_LINEAR)) != 0u) ||
        ((stats->geometry_mode & NDS_RENDERER_GEOM_TEXTURE_GEN) != 0u) ||
#endif
        (geometry_cull != expected_geometry_cull) ||
        ((((stats->othermode_l & NDS_RENDERER_ALPHA_COMPARE_MASK) != 0u) &&
          ((stats->othermode_l & NDS_RENDERER_ALPHA_COMPARE_MASK) !=
           NDS_RENDERER_ALPHA_COMPARE_THRESHOLD))) ||
        ((stats->othermode_l & NDS_RENDERER_ZMODE_MASK) ==
         NDS_RENDERER_ZMODE_DEC) ||
        ((stats->othermode_l & NDS_RENDERER_ZSOURCE_MASK) != 0u) ||
        (stats->texture_combine_w0 != policy->combine_w0) ||
        (stats->texture_combine_w1 != policy->combine_w1) ||
        /* Families 4/5 are the source-exact Ness 0x6760 PASS2 combines
         * emitted by the generator: TEXEL0*SHADE,G_CC_PASS2 and
         * SHADE,G_CC_PASS2. G_CC_PASS2's second cycle is COMBINED passthrough;
         * ENVIRONMENT is not an input, so a non-white env_color cannot
         * modulate either family. The older families keep the white-ENV gate. */
        ((family != NDS_NATIVE_DIRECT_POLICY_LIT_ONLY) &&
         (family != NDS_NATIVE_DIRECT_POLICY_TEXTURED_PASS2) &&
         (family != NDS_NATIVE_DIRECT_POLICY_LIT_PASS2) &&
         (stats->env_color != 0xffffffffu)) ||
        (state->matrix_valid == 0u) ||
        (state->matrix_generation == 0u))
    {
        return ndsRendererNativeDirectReject(stats);
    }

    material_color =
        ((policy->vertex_flags &
          NDS_RENDERER_VERTEX_CONTEXT_USE_MATERIAL) != 0u) ?
            stats->prim_color : 0u;
    state->texture_prepare_source_zbuffered = TRUE;
    state->texture_prepare_decal_depth = FALSE;
    state->texture_prepare_prim_depth = FALSE;
    state->texture_prepare_material_color = material_color;
    state->texture_prepare_vertex_flags = policy->vertex_flags;
    if (state->texture_prepare_valid == 0u)
    {
        u32 texture_name = 0u;

        use_texture = FALSE;
        if ((policy->textured != 0u) && (hierarchy_run == NULL) &&
            (ndsRendererR2RunTextureMemoApply(
                 run_index, stats, &texture_name,
                 &texture_scale_s, &texture_scale_t,
                 &texture_origin_s, &texture_origin_t,
                 &texture_offset) != FALSE))
        {
            use_texture = TRUE;
        }
        else
        if (policy->textured != 0u)
        {
            u32 render_tile_index =
                ((stats->texture_state_flags &
                  NDS_RENDERER_TEXTURE_STATE_SEEN) != 0u) ?
                    (stats->texture_tile & 0x7u) :
                    NDS_RENDERER_RENDER_TILE;
            u32 implicit_texture_on = FALSE;
            u32 required_texture_mask =
                NDS_RENDERER_TEXTURE_SETTIMG |
                NDS_RENDERER_TEXTURE_SETTILE |
                NDS_RENDERER_TEXTURE_SETTILESIZE;

            render_tile = &stats->texture_tiles[render_tile_index];
            if ((stats->texture_state_flags &
                 NDS_RENDERER_TEXTURE_STATE_ON) == 0u)
            {
                implicit_texture_on =
                    (((stats->texture_mask & required_texture_mask) ==
                      required_texture_mask) &&
                     ((stats->texture_mask &
                       (NDS_RENDERER_TEXTURE_LOADBLOCK |
                        NDS_RENDERER_TEXTURE_LOADTILE)) != 0u) &&
                     (stats->texture_image != 0u) &&
                     (stats->texture_load_texels != 0u) &&
                     (render_tile->set_seen != 0u) &&
                     (render_tile->size_seen != 0u) &&
                     (render_tile->line != 0u) &&
                     (render_tile->width != 0u) &&
                     (render_tile->height != 0u)) ? TRUE : FALSE;
            }
            use_texture = (hierarchy_run != NULL) ?
                ndsRendererHardwareResolveResidentTexture(
                    stats, config, state, &resolved_texture) :
                ndsRendererHardwareBindTexture(stats, config, state);
            if (use_texture == FALSE)
            {
                return ndsRendererNativeDirectReject(stats);
            }
            texture_name = (hierarchy_run != NULL) ?
                resolved_texture.name : sNdsRendererHardwareBoundTextureName;
            texture_scale_s = stats->texture_scale_s;
            texture_scale_t = stats->texture_scale_t;
            if (implicit_texture_on != FALSE)
            {
                if ((stats->texture_state_flags &
                     NDS_RENDERER_TEXTURE_STATE_SCALE_S) == 0u)
                {
                    texture_scale_s =
                        NDS_RENDERER_HW_IMPLICIT_TEXTURE_SCALE;
                }
                if ((stats->texture_state_flags &
                     NDS_RENDERER_TEXTURE_STATE_SCALE_T) == 0u)
                {
                    texture_scale_t =
                        NDS_RENDERER_HW_IMPLICIT_TEXTURE_SCALE;
                }
            }
            texture_origin_s = render_tile->uls;
            texture_origin_t = render_tile->ult;
            texture_offset =
                ((stats->othermode_h & NDS_RENDERER_TEXTFILT_MASK) !=
                 NDS_RENDERER_TF_POINT) ?
                    NDS_RENDERER_TEXCOORD_FILTER_OFFSET : 0;
            if (hierarchy_run == NULL)
            {
                ndsRendererR2RunTextureMemoFill(
                    run_index, stats, texture_name,
                    texture_scale_s, texture_scale_t,
                    texture_origin_s, texture_origin_t, texture_offset);
            }
        }
        state->texture_prepare_valid = TRUE;
        state->texture_prepare_enabled = use_texture;
        state->texture_prepare_name = texture_name;
        state->texture_prepare_alpha_constant = TRUE;
        state->texture_prepare_poly_alpha = run_poly_alpha;
        state->texture_prepare_poly_fmt =
            expected_poly_cull | POLY_ALPHA(run_poly_alpha) | run_poly_light |
            POLY_ID(stats->texture_combine_count &
                    NDS_RENDERER_POLY_ID_MASK);
        state->texture_prepare_scale_s = texture_scale_s;
        state->texture_prepare_scale_t = texture_scale_t;
        state->texture_prepare_origin_s = texture_origin_s;
        state->texture_prepare_origin_t = texture_origin_t;
        state->texture_prepare_offset = texture_offset;
        if (packet_mode == 0u)
        {
            ndsRendererProfileRecordTexturePrepare();
        }
    }
    else
    {
        use_texture = state->texture_prepare_enabled;
        if ((use_texture != FALSE) != (policy->textured != 0u))
        {
            return ndsRendererNativeDirectReject(stats);
        }
        /* The cull bits belong to THIS epoch's policy, not to whichever epoch
         * last did a full prepare. Everything else in texture_prepare_poly_fmt
         * is a function of state the validity flag already covers; the cull is
         * not, because it is read from `epoch_policy` two lines into this
         * function while the flag is keyed on source geometry bits.
         *
         * P2-3r17 measured that this cannot currently fire: a cull change
         * arrives as NDS_NATIVE_STATE_GEOMETRY, which invalidates the prepare,
         * and every owner's live geometry mode is validated against the same
         * policy above -- Donkey High has one CULL_NONE epoch (22) and Mario
         * High none. It is repaired anyway because "unreachable" here is a
         * property of today's generated tables, not of this function, and one
         * mask/or per run against ~53 runs a frame is not a price worth
         * leaving a latent wrong-attribute path for. It is NOT the P2-3r17
         * seam: the owner falsified the whole cull family by observing that
         * POLY_CULL_NONE fills the holes' colour in without closing them. */
        state->texture_prepare_poly_alpha = run_poly_alpha;
        state->texture_prepare_poly_fmt =
            (state->texture_prepare_poly_fmt &
             ~((u32)POLY_CULL_NONE | (u32)POLY_ALPHA(31u) |
               (u32)POLY_FORMAT_LIGHT0)) |
            expected_poly_cull | POLY_ALPHA(run_poly_alpha) | run_poly_light;
        if (packet_mode == 0u)
        {
            ndsRendererProfileRecordTexturePrepareReuse();
        }
    }
    /* Packet prepare is a per-run boundary. Fresh and reused texture state
     * both arrive here after NDO6 has applied this run's alpha/LIGHT0 bits, so
     * one shared hook records the final TEXIMAGE_PARAM/POLYGON_ATTR/BEGIN and
     * avoids duplicating the recording branch in scarce fighter ITCM. */
    /* An untextured run of a tinted epoch draws through its tile. Bound here,
     * before the hook, so the packet records the tile as this run's texture
     * (see ndsRendererR2BindTintTile); the hierarchy preflight (mode 7, not a
     * shipping target) keeps the fold. */
    u32 tint = ((policy->textured == 0u) && (hierarchy_run == NULL) &&
                (packet_mode == 0u)) ? sNdsEpochTintTexture : 0u;

    if (tint != 0u)
    {
        ndsRendererR2BindTintTile(stats, tint);
    }
    if ((hierarchy_run != NULL) && (policy->textured != 0u) &&
        (resolved_texture.entry == NULL))
    {
        if (ndsRendererHardwareResolveResidentTexture(
                stats, config, state, &resolved_texture) == FALSE)
        {
            return ndsRendererNativeDirectReject(stats);
        }
        state->texture_prepare_name = resolved_texture.name;
    }

    unique_first = sNdsNativeFighterActiveTables->run_first_unique[run_index];
    unique_count = sNdsNativeFighterActiveTables->run_unique_count[run_index];
    /* Hierarchy preflight records immutable UV policy only; commit evaluates
     * the live dense UVs once.  Mode-8 keeps its original immediate path. */
    if ((policy->textured != 0u) && (hierarchy_run == NULL))
    {
        /* See sNdsNativeFighterRunUvInputs: these five plus baked vertex data
         * are the whole dependency, so equal inputs mean the loop below would
         * write back exactly what is already there. */
        NDSNativeFighterRunUvInputs *uv =
            ndsRendererNativeFighterRunUvLookup(run_index);

        if ((stats->geometry_mode & NDS_RENDERER_GEOM_TEXTURE_GEN) != 0u)
        {
            /* Texgen depends on this root's live modelview and the camera
             * LookAt vectors. It is not a run-index memo unless those become
             * part of the key. Link's texgen run owns all 28 of its dense IDs
             * in both detail levels, so rebuilding it cannot poison a normal-UV
             * run's prepared storage. */
            if (ndsRendererNativeRebuildProductionRunUv(
                    run_index, unique_first, unique_count,
                    stats, state, NULL) == FALSE)
            {
                return ndsRendererNativeDirectReject(stats);
            }
            if (uv != NULL)
            {
                /* Texgen writes this table's shared prepared_dense storage from
                 * live modelview/LookAt state. Any ordinary-UV stamp for the
                 * same table is stale until it rebuilds. */
                uv->tables = NULL;
            }
        }
        else if ((uv != NULL) &&
            (uv->scale_s == state->texture_prepare_scale_s) &&
            (uv->scale_t == state->texture_prepare_scale_t) &&
            (uv->origin_s == state->texture_prepare_origin_s) &&
            (uv->origin_t == state->texture_prepare_origin_t) &&
            (uv->offset == state->texture_prepare_offset) &&
            (uv->tables == sNdsNativeFighterActiveTables) &&
            (uv->heap_generation == gNdsTaskmanHeapGeneration))
        {
#if NDS_TICK_HUD
            NDS_DIAG(gNdsRunUvSkip++);
#endif
        }
        else
        {
            if (uv == NULL)
            {
                uv = ndsRendererNativeFighterRunUvReserveMiss(run_index);
            }
            if (ndsRendererNativeRebuildProductionRunUv(
                    run_index, unique_first, unique_count,
                    stats, state, uv) == FALSE)
            {
                return ndsRendererNativeDirectReject(stats);
            }
        }
    }

    if (hierarchy_run != NULL)
    {
        hierarchy_run->texture_entry = resolved_texture.entry;
        hierarchy_run->texture_name = state->texture_prepare_name;
        hierarchy_run->texture_params = resolved_texture.params;
        hierarchy_run->texture_format = resolved_texture.format;
        hierarchy_run->texture_width = resolved_texture.width;
        hierarchy_run->texture_height = resolved_texture.height;
        hierarchy_run->poly_fmt = state->texture_prepare_poly_fmt;
        hierarchy_run->scale_s = state->texture_prepare_scale_s;
        hierarchy_run->scale_t = state->texture_prepare_scale_t;
        hierarchy_run->origin_s = state->texture_prepare_origin_s;
        hierarchy_run->origin_t = state->texture_prepare_origin_t;
        hierarchy_run->texture_offset = state->texture_prepare_offset;
        hierarchy_run->vertex_flags = state->texture_prepare_vertex_flags;
        hierarchy_run->textured = state->texture_prepare_enabled;
    }
    else if (packet_mode == 0u)
    {
        if (tint != 0u)
        {
            ndsRendererR2OpenTintBatch(stats, tint,
                                       state->texture_prepare_poly_fmt,
                                       state->matrix_generation);
            /* THE BIND HAS TO BE DECLARED, OR IT STEALS THE NEXT RUN'S TEXTURE.
             * ndsRendererHardwareBindTexture runs only inside the
             * `texture_prepare_valid == 0` branch, so a run REUSING its
             * prepare never re-binds and trusts that the texture the tracker
             * names is still on the hardware. The tile made that false: make
             * the next run re-prepare, which re-binds its own texture. */
            NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
        }
        else
        {
            ndsRendererNativeBeginDirectBatch(
                stats, policy->textured, state->texture_prepare_name,
                state->texture_prepare_poly_fmt, state->matrix_generation);
        }
    }
    return TRUE;
}

/* The production owner always passes hierarchy_run == NULL, while the
 * hierarchy preflight always passes a real destination.  Specialize those two
 * cases at the call site so the production copy does not keep hierarchy-only
 * branches resident in ITCM.  The source body remains shared and identical. */
static s32 NDS_RENDERER_NATIVE_FIGHTER_CODE
ndsRendererNativePrepareProductionRun(
    u32 run_index,
    u32 epoch_policy,
    u32 packet_mode,
    const NDSRendererConfig *config,
    NDSRendererStats *stats,
    NDSRendererTraversalState *state)
{
    return ndsRendererNativePrepareProductionRunCore(
        run_index, epoch_policy, packet_mode, config, stats, state, NULL);
}

static s32 NDS_RENDERER_NATIVE_FIGHTER_MAIN_CODE
ndsRendererNativePrepareHierarchyRun(
    u32 run_index,
    u32 epoch_policy,
    u32 packet_mode,
    const NDSRendererConfig *config,
    NDSRendererStats *stats,
    NDSRendererTraversalState *state,
    NDSNativeHierarchyPreparedRun *hierarchy_run)
{
    return ndsRendererNativePrepareProductionRunCore(
        run_index, epoch_policy, packet_mode, config, stats, state,
        hierarchy_run);
}



static inline void ndsRendererNativeEmitProductionShade(
    u32 run_index, u32 dense_id)
{
    const NDSNativeRun *run = &sNdsNativeFighterActiveTables->runs[run_index];
    const u8 *submit_class = &run->submit_class;

    /* This read is deliberately volatile in the per-corner emitter.  The flag
     * is run-invariant, and GCC O3 otherwise loop-unswitches it by cloning the
     * complete primitive/cross vertex loops into flagged and unflagged copies.
     * That grew .itcm.native_fighter by 284 bytes on the roster-close build.
     * One byte load per emitted corner is cheaper than evicting either hot loop
     * from ITCM; the generated table is immutable so the value itself is the
     * same NDO6 metadata byte used by admission. */
    if (((u32)*submit_class &
         NDS_NATIVE_RUN_FLAG_UNLIT_VERTEX_COLOR) != 0u)
    {
        ndsRendererHardwareWriteFighterColorWord(
            sNdsNativeFighterActiveDenseNormals[dense_id]);
        return;
    }
    ndsRendererHardwareWriteNormalWord(
        sNdsNativeFighterActiveDenseNormals[dense_id]);
}

static void NDS_RENDERER_NATIVE_FIGHTER_MAIN_CODE
ndsRendererNativeEmitProductionRawTexturedRun(
    u32 run_index,
    u32 corner_count)
{
    const u16 *corner =
        &sNdsNativeFighterActiveTables->packed_corners[
            (u32)sNdsNativeFighterActiveTables->runs[run_index].first_triangle * 3u];
    u32 remaining = corner_count;

    while (remaining-- != 0u)
    {
        u32 dense_id = *corner++;
        const NDSNativePreparedDenseVertex *prepared =
            &sNdsNativeFighterActiveTables->prepared_dense[dense_id];

        ndsRendererNativeEmitProductionShade(run_index, dense_id);
        ndsRendererHardwareWriteFighterTexCoordWord(
            (u32)(u16)prepared->s |
            ((u32)(u16)prepared->t << 16));
        ndsRendererHardwareWriteFighterVertex16Words(
            prepared->gx_xy, prepared->gx_z);
    }
}

static void NDS_RENDERER_NATIVE_FIGHTER_MAIN_CODE
ndsRendererNativeEmitProductionRawUntexturedRun(
    u32 run_index,
    u32 corner_count)
{
    const u16 *corner =
        &sNdsNativeFighterActiveTables->packed_corners[
            (u32)sNdsNativeFighterActiveTables->runs[run_index].first_triangle * 3u];
    u32 remaining = corner_count;

    while (remaining-- != 0u)
    {
        u32 dense_id = *corner++;
        const NDSNativePreparedDenseVertex *prepared =
            &sNdsNativeFighterActiveTables->prepared_dense[dense_id];

        ndsRendererNativeEmitProductionShade(run_index, dense_id);
        ndsRendererHardwareWriteFighterVertex16Words(
            prepared->gx_xy, prepared->gx_z);
    }
}

/* The one-binary A/B for the strips, same instrument as gNdsAnimCutRoute.
 * .data and aligned(32) so it owns its cache line and no neighbouring
 * counter's write-back can stamp the poke. Default 1 = strips, so an unpoked
 * ROM is the candidate; `-SetGlobals gNdsFighterStripRoute=0` is the raw
 * control, in the SAME binary at the SAME placement. Compile-time gated
 * because the route is an instrument, not a feature: at NDS_STRIP_ROUTE=0
 * the test folds to a constant and the unselected emitter is dead-coded. */
#define NDS_STRIP_ROUTE_ON() (1)


/* Emit a RAW run's triangles as DS-native primitive groups
 * (GL_TRIANGLE_STRIP + residual GL_TRIANGLES) compiled host-side by the
 * generator. 626 triangles submitted as 1,878 individual GL_TRIANGLES corners
 * become 1,014 strip corners in 163 groups -- **46.0% fewer vertex
 * submissions**, and the c115 per-PC census says a fighter corner costs ~40
 * cycles of which ~28 is the GX write itself, so the vertices ARE the cost.
 *
 * Two things about this path were wrong when it was first killed:
 *
 *  - the generator emitted 35.6% of the triangles with reversed winding, so a
 *    third of the fighter was culled away with no assert to say so. Fixed in
 *    `_stripify_run`; `check_fighter_primitive_streams.py` is the standing
 *    proof and expands every group back into oriented triangles.
 *  - this function was `cold` and `optimize("Os")` in `.main` while its raw
 *    siblings sat in ITCM at eleven instructions a corner, and it branched on
 *    `textured` once per VERTEX. A 46% vertex cut cannot survive being paid
 *    for at twice the per-vertex rate. It now has the same placement and the
 *    same inner-loop shape as the raw emitters, with the type branch hoisted
 *    to the group.
 *
 * The batch is open from PrepareProductionRun with GL_TRIANGLE, and every
 * other emitter -- the cross-matrix run, the raw runs, the next run reusing
 * this batch -- assumes the primitive type is still GL_TRIANGLE, because
 * batches are REUSED without re-issuing glBegin. So this restores GL_TRIANGLE
 * before returning rather than tracking the type globally: one FIFO word per
 * run against 53 run emissions a frame, and no invariant left for a future
 * caller to violate. */
static void NDS_RENDERER_NATIVE_FIGHTER_CODE
ndsRendererNativeEmitProductionPrimitiveGroups(
    u32 run_index,
    u32 textured)
{
    u32 g = sNdsNativeFighterActiveTables->primitive_group_first[run_index];
    u32 remaining_groups =
        sNdsNativeFighterActiveTables->primitive_group_count[run_index];
    u32 current_type = (u32)GL_TRIANGLE;

    while (remaining_groups-- != 0u)
    {
        u32 gtype = sNdsNativeFighterActiveTables->primitive_group_type[g];
        const u16 *vref = &sNdsNativeFighterActiveTables->primitive_vertices[
            sNdsNativeFighterActiveTables->primitive_group_first_vertex[g]];
        u32 remaining =
            sNdsNativeFighterActiveTables->primitive_group_vertex_count[g];

        g++;
        /* A new group needs its own BEGIN unless it is a GL_TRIANGLE group
         * following a GL_TRIANGLE group -- separate triangles concatenate
         * harmlessly, strips do NOT.
         *
         * The inherited condition was `gtype != current_type`, which skipped
         * the BEGIN between ADJACENT STRIPS and silently welded them into one
         * list: two bogus bridging triangles, and every triangle after the join
         * carrying the wrong parity, so it was culled. The tables have six
         * consecutive strip groups in the first run alone, and the owner saw it
         * immediately as missing geometry on both fighters. It cost a shipped
         * regression because `check_fighter_primitive_streams.py` expanded each
         * group INDEPENDENTLY -- it proved the data and assumed this policy.
         * The checker now models the policy; keep the two in step. */
        if ((gtype != current_type) || (gtype != (u32)GL_TRIANGLE))
        {
            glBegin((GL_GLBEGIN_ENUM)gtype);
            current_type = gtype;
        }
        if (textured != 0u)
        {
            while (remaining-- != 0u)
            {
                u32 dense_id = *vref++;
                const NDSNativePreparedDenseVertex *prepared =
                    &sNdsNativeFighterActiveTables->prepared_dense[dense_id];

                ndsRendererNativeEmitProductionShade(run_index, dense_id);
                ndsRendererHardwareWriteFighterTexCoordWord(
                    (u32)(u16)prepared->s |
                    ((u32)(u16)prepared->t << 16));
                ndsRendererHardwareWriteFighterVertex16Words(
                    prepared->gx_xy, prepared->gx_z);
            }
        }
        else
        {
            while (remaining-- != 0u)
            {
                u32 dense_id = *vref++;
                const NDSNativePreparedDenseVertex *prepared =
                    &sNdsNativeFighterActiveTables->prepared_dense[dense_id];

                ndsRendererNativeEmitProductionShade(run_index, dense_id);
                ndsRendererHardwareWriteFighterVertex16Words(
                    prepared->gx_xy, prepared->gx_z);
            }
        }
    }
    if (current_type != (u32)GL_TRIANGLE)
    {
        glBegin(GL_TRIANGLE);
    }
}

static void NDS_RENDERER_NATIVE_FIGHTER_CODE
ndsRendererNativeEmitProductionCrossRun(
    u32 run_index,
    u32 corner_count,
    u32 textured,
    u32 current_palette_slot,
    const u8 *binding_palette_slots)
{
    u32 group = sNdsNativeFighterActiveTables->primitive_group_first[run_index];
    const u16 *corner = &sNdsNativeFighterActiveTables->primitive_vertices[
        sNdsNativeFighterActiveTables->primitive_group_first_vertex[group]];
    u32 active_palette_slot = current_palette_slot;
    u32 remaining = corner_count;

    while (remaining-- != 0u)
    {
        u32 packed = *corner++;
        u32 dense_id = packed & NDS_NATIVE_DENSE_ID_MASK;
        const NDSNativeDenseVertex *dense =
            &sNdsNativeFighterActiveTables->dense_vertices[dense_id];
        const NDSNativePreparedDenseVertex *prepared =
            &sNdsNativeFighterActiveTables->prepared_dense[dense_id];
        u32 palette_slot;

        if (binding_palette_slots != NULL)
        {
            palette_slot = binding_palette_slots[dense->matrix_binding];
        }
        else
        {
            palette_slot =
                packed >> NDS_NATIVE_PACKED_CORNER_MATRIX_SHIFT;
        }
        if (palette_slot == NDS_NATIVE_GX_MATRIX_CURRENT)
        {
            palette_slot = current_palette_slot;
        }
        if (palette_slot != active_palette_slot)
        {
            glRestoreMatrix((int)palette_slot);
            active_palette_slot = palette_slot;
        }
        ndsRendererNativeEmitProductionShade(run_index, dense_id);
        if (textured != 0u)
        {
            ndsRendererHardwareWriteFighterTexCoordWord(
                (u32)(u16)prepared->s |
                ((u32)(u16)prepared->t << 16));
        }
        ndsRendererHardwareWriteFighterVertex16Words(
            prepared->gx_xy, prepared->gx_z);
    }
    if (active_palette_slot != current_palette_slot)
    {
        glRestoreMatrix((int)current_palette_slot);
    }
}

static inline void ndsRendererNativeAccountGXCrossTriangles(
    NDSRendererStats *stats,
    u32 triangle_count,
    u32 reuse_count)
{
    sNdsRendererHardwareSubmitClassCounts[
        NDS_RENDERER_HW_SUBMIT_PROJECTED_CROSS_MATRIX] += triangle_count;
#if NDS_DIAG_COUNTERS
    sNdsRendererRuntimeFrameSummary.raw_cross_matrix_count += triangle_count;
    sNdsRendererRuntimeFrameSummary.hardware_batch_reuse_count += reuse_count;
    sNdsRendererRuntimeFrameSummary.texture_prepare_reuse_count += reuse_count;
    sNdsRendererRuntimeFrameSummary.hardware_triangles += triangle_count;
    sNdsRendererRuntimeFrameSummary.hardware_vertices += triangle_count * 3u;
    if ((sNdsRendererRuntimeFrameSummary.hardware_triangles > 2048u) ||
        (sNdsRendererRuntimeFrameSummary.hardware_vertices > 6144u))
    {
        sNdsRendererRuntimeFrameSummary.hardware_over_limit = 1u;
    }
#else
    (void)reuse_count;
#endif
    stats->hardware_triangle_count += triangle_count;
    stats->hardware_vertex_count += triangle_count * 3u;
    stats->hardware_zbuffer_triangle_count += triangle_count;
    sNdsRendererHardwareSubmitted = TRUE;
}
