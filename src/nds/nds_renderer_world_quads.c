/* Native renderer, world quads: Whispy's packeted quads, the staff roll's names,
 * Fox's blaster beam and gun, the afterimage trail and the debug collision
 * diamond. A slice of nds_renderer.c. */

/* When the last commands in the pass are packeted Whispy quads, put the
 * balancing matrix pop behind those vertices in that same stream.  Returning
 * FALSE tells the ordinary end path that an immediate/generic draw followed
 * the packet and therefore still owns the pop. */
static NDS_RENDERER_FAST_RUN_CODE sb32
ndsRendererFinishWhispyNativePacket(void)
{
    u32 *out;

    if (sNdsRendererWhispyPacket.word_count == 0u)
    {
        return FALSE;
    }
    out = ndsRendererWhispyPacketReserve(2u);
    if (out == NULL)
    {
        return FALSE;
    }
    out[0] = FIFO_COMMAND_PACK(
        REG2ID(MATRIX_POP), FIFO_NOP, FIFO_NOP, FIFO_NOP);
    out[1] = 1u;
    ndsRendererFlushWhispyNativePacket();
    return TRUE;
}

/* ARM946E-S has no FPU. GCC lowers even `value * 4096.0F` followed by an int
 * cast to two libgcc calls, so the first fixed submit still paid eight soft-
 * float calls per quad. Decode the finite IEEE-754 value directly and shift
 * its mantissa into the requested binary fixed domain. The result is the same
 * truncation toward zero as a C cast; failure is the generic-fallback seam. */
static inline sb32 ndsRendererWhispyFloatToFixed(f32 value,
                                                  u32 fraction_bits,
                                                  s32 *fixed)
{
    return ndsRendererParticleFloatToFixed(value, fraction_bits, fixed);
}

s32 ndsRendererParticlePositionToQ12(const Vec3f *pos,
                                     s32 fixed_center_q12[3])
{
    if ((pos == NULL) || (fixed_center_q12 == NULL) ||
        (ndsRendererWhispyFloatToFixed(
             pos->x, NDS_RENDERER_WHISPY_COORD_SHIFT,
             &fixed_center_q12[0]) == FALSE) ||
        (ndsRendererWhispyFloatToFixed(
             pos->y, NDS_RENDERER_WHISPY_COORD_SHIFT,
             &fixed_center_q12[1]) == FALSE) ||
        (ndsRendererWhispyFloatToFixed(
             pos->z, NDS_RENDERER_WHISPY_COORD_SHIFT,
             &fixed_center_q12[2]) == FALSE))
    {
        return FALSE;
    }
    return TRUE;
}

static sb32 ndsRendererWhispyBasisComponent(f32 value, s32 *fixed)
{
    if ((ndsRendererWhispyFloatToFixed(
             value, NDS_RENDERER_WHISPY_BASIS_SHIFT, fixed) == FALSE) ||
        (*fixed < -(s32)(1u << NDS_RENDERER_WHISPY_BASIS_SHIFT)) ||
        (*fixed > (s32)(1u << NDS_RENDERER_WHISPY_BASIS_SHIFT)))
    {
        return FALSE;
    }
    return TRUE;
}

void ndsRendererSetWhispyNativeBasis(const Vec3f *right, const Vec3f *up)
{
    sNdsRendererWhispyBasisValid = FALSE;
    sNdsRendererWhispyLeanBindingMask = 0u;
    sNdsRendererWhispyLegCacheValid = FALSE;
    if ((right == NULL) || (up == NULL) ||
        (ndsRendererWhispyBasisComponent(
             right->x, &sNdsRendererWhispyRight[0]) == FALSE) ||
        (ndsRendererWhispyBasisComponent(
             right->y, &sNdsRendererWhispyRight[1]) == FALSE) ||
        (ndsRendererWhispyBasisComponent(
             right->z, &sNdsRendererWhispyRight[2]) == FALSE) ||
        (ndsRendererWhispyBasisComponent(
             up->x, &sNdsRendererWhispyUp[0]) == FALSE) ||
        (ndsRendererWhispyBasisComponent(
             up->y, &sNdsRendererWhispyUp[1]) == FALSE) ||
        (ndsRendererWhispyBasisComponent(
             up->z, &sNdsRendererWhispyUp[2]) == FALSE))
    {
        return;
    }
    sNdsRendererWhispyBasisValid = TRUE;
}

/* Mirror ndsRendererSubmitParticleQuad's state transitions without entering
 * its float corner builder. Keeping these exact transitions preserves source
 * draw order, per-particle alpha, texture changes, matrix ownership, and the
 * no-glEnd FIFO contract. */
static NDS_RENDERER_FAST_RUN_CODE void
ndsRendererPrepareWhispyQuadState(u32 texture_name, u32 poly_alpha,
                                  u32 texture_slot, u32 submit_route)
{
    if ((sNdsRendererParticleQuadOpen != 0u) &&
        ((poly_alpha != sNdsRendererParticleQuadAlpha) ||
         (texture_name != sNdsRendererParticleQuadTexture)))
    {
        if (texture_name != sNdsRendererParticleQuadTexture)
        {
            if (submit_route >= 3u)
            {
                (void)ndsRendererBindWhispyNativeTextureFast(
                    texture_name, texture_slot);
            }
            else
            {
                ndsRendererHardwareBindTextureName(NULL, texture_name);
            }
            sNdsRendererParticleQuadTexture = texture_name;
            gNdsParticleQuadSheetBreaks++;
        }
        ndsRendererHardwareSetPolyFmt(
            POLY_ALPHA(poly_alpha) | POLY_CULL_NONE | POLY_ID(0));
        if (poly_alpha != sNdsRendererParticleQuadAlpha)
        {
            gNdsParticleQuadAlphaBreaks++;
        }
        sNdsRendererParticleQuadAlpha = poly_alpha;
        glBegin(GL_QUAD);
    }
    if (sNdsRendererParticleQuadOpen == 0u)
    {
        gNdsParticleMatrixModeSeen |=
            1u << (sNdsRendererHardwareMatrixMode & 7u);
        gNdsParticleMatrixLoadedSeen = sNdsRendererHardwareMatrixLoaded;
        NDS_DIAG(gNdsParticleBatchOpens++);
        ndsRendererHardwareEndBatch();
        if (sNdsRendererParticleCameraValid != FALSE)
        {
            NDSRendererMatrix20p12 scaled_modelview;
            m4x4 projection_hw;
            m4x4 modelview_hw;
            u32 col;

            if (sNdsRendererParticleViewSpace != FALSE)
            {
                ndsRendererMatrixIdentity20p12(&scaled_modelview, 1 << 12);
            }
            else
            {
                ndsRendererMatrixCopy20p12(
                    &scaled_modelview, &sNdsRendererParticleModelview);
            }
            for (col = 0u; col < 4u; col++)
            {
                scaled_modelview.m[3][col] =
                    ndsRendererRoundShiftS32Signed(
                        scaled_modelview.m[3][col],
                        NDS_RENDERER_HW_WORLD_UNIT_SHIFT);
            }
            ndsRendererCopyMtx20p12ToM4x4(
                &sNdsRendererParticleProjection, &projection_hw);
            ndsRendererHardwareSetMatrixMode(GL_PROJECTION);
            glLoadMatrix4x4(&projection_hw);
            ndsRendererCopyMtx20p12ToM4x4(
                &scaled_modelview, &modelview_hw);
            ndsRendererHardwareSetMatrixMode(GL_MODELVIEW);
            glLoadMatrix4x4(&modelview_hw);
            sNdsRendererHardwareMatrixLoaded = FALSE;
            sNdsRendererHardwareMatrixMode = NDS_RENDERER_HW_MATRIX_MODE_NONE;
            gNdsParticleCameraLoads++;
            gNdsParticleCamTx = scaled_modelview.m[3][0];
            gNdsParticleCamTy = scaled_modelview.m[3][1];
            gNdsParticleCamTz = scaled_modelview.m[3][2];
        }
        sNdsRendererParticleScaleShift = 0u;
        glPushMatrix();
        glEnable(GL_TEXTURE_2D);
        if (submit_route >= 3u)
        {
            (void)ndsRendererBindWhispyNativeTextureFast(
                texture_name, texture_slot);
        }
        else
        {
            ndsRendererHardwareBindTextureName(NULL, texture_name);
        }
        sNdsRendererParticleQuadTexture = texture_name;
        ndsRendererHardwareSetPolyFmt(
            POLY_ALPHA(poly_alpha) | POLY_CULL_NONE | POLY_ID(0));
        sNdsRendererParticleQuadAlpha = poly_alpha;
        glBegin(GL_QUAD);
        sNdsRendererParticleQuadOpen = TRUE;
    }
}

static s32 ndsRendererWhispyAbsS32(s32 value)
{
    return (value < 0) ? -value : value;
}

static s32 ndsRendererWhispyTruncShiftS32(s32 value, u32 shift)
{
    if (value < 0)
    {
        return -(s32)(((u32)-value) >> shift);
    }
    return (s32)((u32)value >> shift);
}

static v16 ndsRendererWhispyCoordToV16(s32 coord_q12, u32 scale_shift)
{
    s32 value = ndsRendererWhispyTruncShiftS32(
        coord_q12,
        (NDS_RENDERER_WHISPY_COORD_SHIFT - 4u) + scale_shift);

    if (value > 32767)
    {
        gNdsParticleWorldClampCount++;
        return (v16)32767;
    }
    if (value < -32768)
    {
        gNdsParticleWorldClampCount++;
        return (v16)-32768;
    }
    return (v16)value;
}

static inline __attribute__((always_inline)) v16
ndsRendererWhispyCoordToV16Unchecked(s32 coord_q12, u32 scale_shift)
{
    return (v16)ndsRendererWhispyTruncShiftS32(
        coord_q12,
        (NDS_RENDERER_WHISPY_COORD_SHIFT - 4u) + scale_shift);
}

NDS_RENDERER_FAST_RUN_CODE
s32 ndsRendererSubmitWhispyNativeQuad(u32 texture_name,
                                      u32 texture_slot,
                                      const Vec3f *pos, f32 size,
                                      const s32 fixed_center_q12[3],
                                      s32 fixed_size_q8,
                                      u32 color, u8 alpha,
                                      u32 mirror_mask,
                                      u32 texture_w, u32 texture_h,
                                      sb32 packet)
{
    NDS_FIGHTER_PACKET_DMA_WAIT();
    s32 center[3];
    s32 right[3];
    s32 up[3];
    v16 packed_vertex[4][3];
    s32 extent;
    s32 size_q8;
    u32 axis;
    u32 corner;
    u32 needed = 0u;
    u32 coordinates_safe = FALSE;
    u32 poly_alpha;

    /* These bounds make every 32-bit basis product total. Whispy's source
     * sizes top out at 350; an altered script or camera uses the generic submit
     * instead of wrapping this closed representation. */
    if ((sNdsRendererWhispyBasisValid == FALSE) ||
        (texture_name == 0u) ||
        ((fixed_center_q12 == NULL) && (pos == NULL)))
    {
        return -1;
    }
    if (alpha == 0u)
    {
        return 0;
    }
    poly_alpha = ((u32)alpha >> 3) & 31u;
    if (poly_alpha == 0u) { poly_alpha = 1u; }
    if (packet != FALSE)
    {
        u32 binding_bit;

        if (texture_slot >= NDS_WHISPY_NATIVE_TEXTURE_COUNT)
        {
            gNdsWhispyAOTTier4PacketFallbacks++;
            return -1;
        }
        binding_bit = 1u << texture_slot;
        if (((sNdsRendererWhispyLeanBindingMask & binding_bit) == 0u) &&
            (ndsRendererWhispyNativeBindingFor(
                 texture_name, texture_slot) == NULL))
        {
            gNdsWhispyAOTTier4PacketFallbacks++;
            return -1;
        }
        sNdsRendererWhispyLeanBindingMask |= binding_bit;
    }
    else if (ndsRendererWhispyNativeBindingFor(
                 texture_name, texture_slot) == NULL)
    {
        return -1;
    }
    if (packet == FALSE)
    {
        /* An immediate-mode quad (the Fox blaster glow) must not let an
         * older packet cross it. */
        ndsRendererFlushWhispyNativePacket();
    }
    else if (sNdsRendererWhispyPacket.word_count == 0u)
    {
        sNdsRendererWhispyPacket.lean_counters = TRUE;
    }

    if (fixed_center_q12 != NULL)
    {
        center[0] = fixed_center_q12[0];
        center[1] = fixed_center_q12[1];
        center[2] = fixed_center_q12[2];
        size_q8 = fixed_size_q8;
    }
    else if ((ndsRendererWhispyFloatToFixed(
                  pos->x, NDS_RENDERER_WHISPY_COORD_SHIFT,
                  &center[0]) == FALSE) ||
             (ndsRendererWhispyFloatToFixed(
                  pos->y, NDS_RENDERER_WHISPY_COORD_SHIFT,
                  &center[1]) == FALSE) ||
             (ndsRendererWhispyFloatToFixed(
                  pos->z, NDS_RENDERER_WHISPY_COORD_SHIFT,
                  &center[2]) == FALSE) ||
             (ndsRendererWhispyFloatToFixed(
                  size, NDS_RENDERER_WHISPY_SIZE_SHIFT,
                  &size_q8) == FALSE))
    {
        return -1;
    }
    if ((size_q8 <= 0) ||
        (size_q8 >= (s32)(512u << NDS_RENDERER_WHISPY_SIZE_SHIFT)) ||
        (center[0] < -(s32)(131072u << NDS_RENDERER_WHISPY_COORD_SHIFT)) ||
        (center[0] >  (s32)(131072u << NDS_RENDERER_WHISPY_COORD_SHIFT)) ||
        (center[1] < -(s32)(131072u << NDS_RENDERER_WHISPY_COORD_SHIFT)) ||
        (center[1] >  (s32)(131072u << NDS_RENDERER_WHISPY_COORD_SHIFT)) ||
        (center[2] < -(s32)(131072u << NDS_RENDERER_WHISPY_COORD_SHIFT)) ||
        (center[2] >  (s32)(131072u << NDS_RENDERER_WHISPY_COORD_SHIFT)))
    {
        return -1;
    }
    if (ndsRendererParticleTransformCenter(
            center, NDS_RENDERER_WHISPY_COORD_SHIFT) == FALSE)
    {
        return -1;
    }
    if (sNdsRendererParticleViewSpace != FALSE)
    {
        right[0] = ((mirror_mask & 1u) != 0u) ? -size_q8 * 16 : size_q8 * 16;
        right[1] = right[2] = 0;
        up[1] = ((mirror_mask & 2u) != 0u) ? -size_q8 * 16 : size_q8 * 16;
        up[0] = up[2] = 0;
    }
    else if ((packet != FALSE) &&
        (sNdsRendererWhispyLegCacheValid != FALSE) &&
        (sNdsRendererWhispyLegCacheSizeQ8 == size_q8) &&
        (sNdsRendererWhispyLegCacheMirrorMask == (mirror_mask & 3u)))
    {
        for (axis = 0u; axis < 3u; axis++)
        {
            right[axis] = sNdsRendererWhispyLegCacheRight[axis];
            up[axis] = sNdsRendererWhispyLegCacheUp[axis];
        }
    }
    else
    {
        s32 leg_size_q8 = size_q8;

        if ((mirror_mask & 1u) != 0u) { leg_size_q8 = -leg_size_q8; }
        for (axis = 0u; axis < 3u; axis++)
        {
            right[axis] = (sNdsRendererWhispyRight[axis] * leg_size_q8) /
                          (s32)(1u << NDS_RENDERER_WHISPY_LEG_SHIFT);
        }
        if ((mirror_mask & 1u) != 0u) { leg_size_q8 = -leg_size_q8; }
        if ((mirror_mask & 2u) != 0u) { leg_size_q8 = -leg_size_q8; }
        for (axis = 0u; axis < 3u; axis++)
        {
            up[axis] = (sNdsRendererWhispyUp[axis] * leg_size_q8) /
                       (s32)(1u << NDS_RENDERER_WHISPY_LEG_SHIFT);
        }
        if (packet != FALSE)
        {
            sNdsRendererWhispyLegCacheSizeQ8 = size_q8;
            sNdsRendererWhispyLegCacheMirrorMask = mirror_mask & 3u;
            for (axis = 0u; axis < 3u; axis++)
            {
                sNdsRendererWhispyLegCacheRight[axis] = right[axis];
                sNdsRendererWhispyLegCacheUp[axis] = up[axis];
            }
            sNdsRendererWhispyLegCacheValid = TRUE;
        }
    }

    extent = ndsRendererWhispyAbsS32(center[0]) +
             ndsRendererWhispyAbsS32(right[0]) +
             ndsRendererWhispyAbsS32(up[0]);
    for (axis = 1u; axis < 3u; axis++)
    {
        s32 candidate = ndsRendererWhispyAbsS32(center[axis]) +
                        ndsRendererWhispyAbsS32(right[axis]) +
                        ndsRendererWhispyAbsS32(up[axis]);
        if (candidate > extent) { extent = candidate; }
    }
    if (packet != FALSE)
    {
        needed = sNdsRendererParticleScaleShift;
    }
    while ((needed < NDS_RENDERER_PARTICLE_MAX_SCALE_SHIFT) &&
           ((extent / (s32)(1u <<
               ((NDS_RENDERER_WHISPY_COORD_SHIFT - 4u) + needed))) > 32767))
    {
        needed++;
    }
    if (packet != FALSE)
    {
        coordinates_safe =
            ((extent / (s32)(1u <<
                ((NDS_RENDERER_WHISPY_COORD_SHIFT - 4u) + needed))) <=
             32767) ? TRUE : FALSE;
    }

    if (packet != FALSE)
    {
        if (sNdsRendererParticleQuadOpen == 0u)
        {
            /* Establish the camera/matrix stack once. The first material bind
             * uses the exact direct-register transition; every later
             * state/quad command in this contiguous run goes through DMA. */
            ndsRendererPrepareWhispyQuadState(
                texture_name, poly_alpha, texture_slot, 3u);
        }
    }
    else
    {
        ndsRendererPrepareWhispyQuadState(
            texture_name, poly_alpha, texture_slot, 3u);
    }
    if (needed > sNdsRendererParticleScaleShift)
    {
        if (packet != FALSE)
        {
            if (ndsRendererAppendWhispyPacketScale(needed) == FALSE)
            {
                return -1;
            }
        }
        else
        {
            s32 factor = inttof32(1 << needed);

            glPopMatrix(1);
            glPushMatrix();
            glScalef32(factor, factor, factor);
        }
        sNdsRendererParticleScaleShift = needed;
        gNdsParticleScaleEscalations++;
        if (needed > gNdsParticleScaleShiftMax)
        {
            gNdsParticleScaleShiftMax = needed;
        }
    }
    if (packet != FALSE)
    {
        if (ndsRendererAppendWhispyPacketState(
                texture_slot, poly_alpha) == FALSE)
        {
            return -1;
        }
        if (coordinates_safe != FALSE)
        {
            for (corner = 0u; corner < 4u; corner++)
            {
                s32 right_sign =
                    ((corner == 0u) || (corner == 3u)) ? -1 : 1;
                s32 up_sign = (corner < 2u) ? -1 : 1;
                u32 whispy_shift = sNdsRendererParticleScaleShift;
                u32 component;

                for (component = 0u; component < 3u; component++)
                {
                    packed_vertex[corner][component] =
                        ndsRendererWhispyCoordToV16Unchecked(
                            center[component] +
                            (right_sign * right[component]) +
                            (up_sign * up[component]), whispy_shift);
                }
            }
        }
        else
        {
            for (corner = 0u; corner < 4u; corner++)
            {
                s32 right_sign =
                    ((corner == 0u) || (corner == 3u)) ? -1 : 1;
                s32 up_sign = (corner < 2u) ? -1 : 1;
                u32 whispy_shift = sNdsRendererParticleScaleShift;
                u32 component;

                for (component = 0u; component < 3u; component++)
                {
                    packed_vertex[corner][component] =
                        ndsRendererWhispyCoordToV16(
                            center[component] +
                            (right_sign * right[component]) +
                            (up_sign * up[component]), whispy_shift);
                }
            }
        }
        if (ndsRendererAppendWhispyPacketQuad(
                color, packed_vertex, texture_w, texture_h,
                texture_slot, poly_alpha) == FALSE)
        {
            return -1;
        }
    }
    else
    {
        glColor((rgb)color);

        /* Four fixed corners, still in source draw order. A small integer loop
         * is materially cheaper in ARM9 code/RAM than cloning the diagnostic-
         * aware GX wrappers four times. */
        for (corner = 0u; corner < 4u; corner++)
        {
            s32 right_sign =
                ((corner == 0u) || (corner == 3u)) ? -1 : 1;
            s32 up_sign = (corner < 2u) ? -1 : 1;
            u32 texel_s =
                ((corner == 0u) || (corner == 3u)) ? 0u : texture_w;
            u32 texel_t = (corner < 2u) ? texture_h : 0u;
            u32 whispy_shift = sNdsRendererParticleScaleShift;

            glTexCoord2t16((t16)(texel_s << 4), (t16)(texel_t << 4));
            glVertex3v16(
                ndsRendererWhispyCoordToV16(
                    center[0] + (right_sign * right[0]) +
                        (up_sign * up[0]), whispy_shift),
                ndsRendererWhispyCoordToV16(
                    center[1] + (right_sign * right[1]) +
                        (up_sign * up[1]), whispy_shift),
                ndsRendererWhispyCoordToV16(
                    center[2] + (right_sign * right[2]) +
                        (up_sign * up[2]), whispy_shift));
        }
    }
    return 1;
}

void ndsRendererEndParticleQuads(void)
{
    NDS_FIGHTER_PACKET_DMA_WAIT();
    u32 whispy_packet_popped = FALSE;
    sNdsRendererParticleViewSpace = FALSE;

    /* Route 4 leaves the final source-ordered packet in cached RAM until this
     * natural pass boundary. The balancing pop rides after its last vertex. */
    whispy_packet_popped = ndsRendererFinishWhispyNativePacket();
    if (sNdsRendererParticleQuadOpen != 0u)
    {
        /* NO glEnd(). Task 29 removed libnds's dummy glEnd FIFO writes from
         * this renderer and check-gbi-decode-fixtures.ps1 pins the count at
         * one, because a primitive group ends when the NEXT glBegin starts --
         * an extra FIFO write desynchronises the command stream instead of
         * closing anything. The first build of this emitter called it and hung
         * the ROM with GXSTAT=0e008900, geometry-engine-busy forever.
         *
         * So the group is left open exactly as ndsRendererHardwareEndBatch
         * leaves its own, and the tracker is told nothing is open so a later
         * reuse check cannot match stale state. */
        sNdsRendererParticleQuadOpen = FALSE;
        sNdsRendererParticleQuadAlpha = 0u;
        sNdsRendererParticleQuadTexture = 0u;
        sNdsRendererParticleQuadPalette = 0u;
        ndsRendererHardwareEndBatch();
        /* AFTER EndBatch, not before. The comment above is explicit that this
         * group is left open exactly as EndBatch leaves its own and that an
         * extra FIFO write here hung the ROM at GXSTAT=0e008900; popping the
         * matrix before the batch is flushed would retroactively move vertices
         * already queued against it. Pairs with the unconditional push at batch
         * open, at whatever shift the pass escalated to. */
        if (whispy_packet_popped == FALSE)
        {
            glPopMatrix(1);
        }
        sNdsRendererParticleScaleShift = 0u;
        /* The bound-texture name is LEFT ALONE. Clearing it here forced the
         * next binder to re-issue every frame, and the stage's prepared-run
         * reuse rides on that state: rebuilds went 2 -> 197 a match purely
         * from this line. The tracker already knows the atlas is bound, and
         * the next real bind compares against it correctly. */
        sNdsRendererHardwareActiveTextureEntry = NULL;
    }
    sNdsRendererWhispyBasisValid = FALSE;
}

/* A frame whose only geometry is world quads (the staff roll's names) is still
 * a GX frame: the platform flushes it, and shows BG0, only for a submitted one.
 * Every other quad shares a frame with a fighter or stage that marks it. */
void ndsRendererHardwareNoteQuadFrame(void)
{
    sNdsRendererHardwareSubmitted = TRUE;
}

/* ARM946E-S has no FPU. The beam's source DObj is already a closed four-vertex
 * contract, so paying __aeabi_fmul/__aeabi_f2iz for every coordinate would
 * throw away most of the native-submit gain. Decode finite IEEE-754 directly
 * into Q12, with the same truncation toward zero as a C cast. */
static sb32 ndsRendererFoxBlasterFloatToQ12(f32 value, s32 *fixed)
{
    union { f32 f; u32 u; } bits;
    u32 exponent;
    u32 mantissa;
    u32 magnitude;
    u32 limit;
    u32 sign;
    s32 shift;

    bits.f = value;
    sign = bits.u >> 31;
    exponent = (bits.u >> 23) & 0xffu;
    mantissa = bits.u & 0x7fffffu;
    if ((fixed == NULL) || (exponent == 0xffu))
    {
        return FALSE;
    }
    if (exponent == 0u)
    {
        if (mantissa == 0u)
        {
            *fixed = 0;
            return TRUE;
        }
        shift = -126 - 23 + 12;
    }
    else
    {
        mantissa |= 0x800000u;
        shift = (s32)exponent - 127 - 23 + 12;
    }
    limit = (sign != 0u) ? 0x80000000u : 0x7fffffffu;
    if (shift >= 0)
    {
        if ((shift >= 32) || (mantissa > (limit >> (u32)shift)))
        {
            return FALSE;
        }
        magnitude = mantissa << (u32)shift;
    }
    else if (shift <= -32)
    {
        magnitude = 0u;
    }
    else
    {
        magnitude = mantissa >> (u32)-shift;
    }
    *fixed = (sign != 0u) ?
        ((magnitude == 0x80000000u) ? INT_MIN : -(s32)magnitude) :
        (s32)magnitude;
    return TRUE;
}

static u32 ndsRendererFoxBlasterAbsQ12(s32 value)
{
    return (value < 0) ? (u32)(-(s64)value) : (u32)value;
}

static v16 ndsRendererFoxBlasterQ12ToV16(s32 value, u32 scale_shift)
{
    u32 shift = 8u + scale_shift;

    if (value < 0)
    {
        return (v16)-(s32)((u32)(-(s64)value) >> shift);
    }
    return (v16)((u32)value >> shift);
}

/* relocData 316, decoded from the source bytes:
 *
 *   (  0,  24, 0), (  0, -26, 0),
 *   (-30, -26, 0), (-30,  24, 0), RGBA (219,0,134,0)
 *
 * Its combine mode uses shade RGB with full combiner alpha, texture disabled.
 * The DObj owns TraRotRpyRSca; the admitted path is exactly rotate.z 0 or pi,
 * selected from the source velocity by the caller. No texture, DL decode,
 * Vtx staging, matrix build, trig, or software-float multiply remains here. */
NDS_RENDERER_FAST_RUN_CODE
s32 ndsRendererSubmitFoxBlasterQuad(const Vec3f *translate,
                                    f32 scale_x, f32 scale_y,
                                    s32 facing)
{
    NDS_FIGHTER_PACKET_DMA_WAIT();
    s32 tx;
    s32 ty;
    s32 tz;
    s32 sx;
    s32 sy;
    s32 x0;
    s32 x1;
    s32 y0;
    s32 y1;
    s32 span_x;
    s32 span_y0;
    s32 span_y1;
    v16 vx0;
    v16 vx1;
    v16 vy0;
    v16 vy1;
    v16 vz;
    u32 max_q12;
    u32 scale_shift = 0u;

    if ((translate == NULL) || ((facing != 1) && (facing != -1)) ||
        (ndsRendererFoxBlasterFloatToQ12(translate->x, &tx) == FALSE) ||
        (ndsRendererFoxBlasterFloatToQ12(translate->y, &ty) == FALSE) ||
        (ndsRendererFoxBlasterFloatToQ12(translate->z, &tz) == FALSE) ||
        (ndsRendererFoxBlasterFloatToQ12(scale_x, &sx) == FALSE) ||
        (ndsRendererFoxBlasterFloatToQ12(scale_y, &sy) == FALSE) ||
        (sx <= 0) || (sy <= 0) || (sx > (INT_MAX / 30)) ||
        (sy > (INT_MAX / 26)))
    {
        return FALSE;
    }
    /* OWNER DECISION 2026-08-13 -- the approved draw-only bore offset, and this
     * is the whole of it for the beam. `translate` is const and is the weapon
     * DObj's own world position: it is not written here, so spawn, attack_pos,
     * hitbox and collision are untouched by construction.
     *
     * The raise lands on the DECODED translation, before span_x/span_y0/span_y1
     * apply the source scale to the quad's four source vertices. scale.x runs
     * 1.0 -> 53.33 over a shot's first ten ticks; folding the offset in later
     * would stretch the raise by that same 53x as the beam grew. */
    if (ty > (INT_MAX - NDS_FOX_BLASTER_BORE_OFFSET_Y_Q12))
    {
        return FALSE;
    }
    ty += NDS_FOX_BLASTER_BORE_OFFSET_Y_Q12;
    span_x = 30 * sx;
    span_y0 = 24 * sy;
    span_y1 = 26 * sy;
    if (facing > 0)
    {
        if ((tx < (INT_MIN + span_x)) ||
            (ty > (INT_MAX - span_y0)) ||
            (ty < (INT_MIN + span_y1)))
        {
            return FALSE;
        }
        x0 = tx;
        x1 = tx - span_x;
        y0 = ty + span_y0;
        y1 = ty - span_y1;
    }
    else
    {
        /* Rz(pi): local -X becomes world +X and local Y changes sign. */
        if ((tx > (INT_MAX - span_x)) ||
            (ty < (INT_MIN + span_y0)) ||
            (ty > (INT_MAX - span_y1)))
        {
            return FALSE;
        }
        x0 = tx;
        x1 = tx + span_x;
        y0 = ty - span_y0;
        y1 = ty + span_y1;
    }
    max_q12 = ndsRendererFoxBlasterAbsQ12(x0);
#define NDS_FOX_BLASTER_MAX_Q12(value) do { \
    u32 magnitude__ = ndsRendererFoxBlasterAbsQ12((value)); \
    if (magnitude__ > max_q12) { max_q12 = magnitude__; } \
} while (0)
    NDS_FOX_BLASTER_MAX_Q12(x1);
    NDS_FOX_BLASTER_MAX_Q12(y0);
    NDS_FOX_BLASTER_MAX_Q12(y1);
    NDS_FOX_BLASTER_MAX_Q12(tz);
#undef NDS_FOX_BLASTER_MAX_Q12
    while ((scale_shift < NDS_RENDERER_PARTICLE_MAX_SCALE_SHIFT) &&
           ((max_q12 >> (8u + scale_shift)) > 32767u))
    {
        scale_shift++;
    }
    if ((max_q12 >> (8u + scale_shift)) > 32767u)
    {
        return FALSE;
    }

    ndsRendererFlushWhispyNativePacket();
    /* A direct weapon display cannot borrow an open particle primitive. Flush
     * it first if ordering ever places one here, then own one balanced matrix
     * push for this source quad. */
    ndsRendererEndParticleQuads();
    ndsRendererHardwareEndBatch();
    if (ndsRendererLoadFoxBlasterNoZCameraMatrices() == FALSE)
    {
        return FALSE;
    }
    glPushMatrix();
    if (scale_shift != 0u)
    {
        s32 factor = inttof32(1 << scale_shift);
        glScalef32(factor, factor, factor);
    }
    /* Match the interpreted renderer's GL_NOTEXTURE TEXIMAGE_PARAM contract.
     * Clearing DISP3DCNT's global texture bit is not the same GX material
     * state and visibly shifts this thin saturated beam on DS. The shared
     * no-texture name carries no texel payload and is normally already hot. */
    glEnable(GL_TEXTURE_2D);
    ndsRendererHardwareBindNoTexture(NULL);
    /* The generic source renderer derives ID 1 from this list's single
     * combine command. Keep that exact ID: the spawn glow uses particle ID 0,
     * and collapsing both to 0 changes their DS translucent-overlap result. */
    ndsRendererHardwareSetPolyFmt(
        POLY_ALPHA(31) | POLY_CULL_NONE | POLY_ID(1));
    vx0 = ndsRendererFoxBlasterQ12ToV16(x0, scale_shift);
    vx1 = ndsRendererFoxBlasterQ12ToV16(x1, scale_shift);
    vy0 = ndsRendererFoxBlasterQ12ToV16(y0, scale_shift);
    vy1 = ndsRendererFoxBlasterQ12ToV16(y1, scale_shift);
    vz = ndsRendererFoxBlasterQ12ToV16(tz, scale_shift);
    /* The source quadrangle decodes to triangles 0/1/2 and 0/2/3. A DS
     * GL_QUAD covers the same mathematical area but rasterizes this five-pixel
     * beam differently, so retain the source triangle split exactly. */
    glBegin(GL_TRIANGLES);
    /* GX BEGIN establishes the primitive's vertex state. Match the generic
     * path and write COLOR after it; writing COLOR first inherits the prior
     * primitive's latched color on hardware. */
    glColor(RGB15(27, 0, 16));
    glVertex3v16(vx0, vy0, vz);
    glVertex3v16(vx0, vy1, vz);
    glVertex3v16(vx1, vy1, vz);
    glVertex3v16(vx0, vy0, vz);
    glVertex3v16(vx1, vy1, vz);
    glVertex3v16(vx1, vy0, vz);
    ndsRendererHardwareEndBatch();
    glPopMatrix(1);
    sNdsRendererHardwareMatrixLoaded = FALSE;
    sNdsRendererHardwareMatrixMode = NDS_RENDERER_HW_MATRIX_MODE_NONE;
    return TRUE;
}

/* Fox's blaster geometry, uploaded once. Unlike the glow this reads no asset
 * file: nds_fox_gun.c carries the source's own CI4 32x16 and its 16 RGBA5551
 * colours in the DS channel order, so the "conversion" is a memcpy into VRAM.
 * Its own immutable GL name is scene-owned directly, because a 22-triangle
 * part that appears and disappears with a motion script must not consume a
 * dynamic material-cache slot against the fighter's own materials. */
static int sNdsRendererFoxGunName;
static int sNdsRendererFoxGunPaletteFormat = -1;

static void ndsRendererHardwareResetFoxGunTextureState(void)
{
    sNdsRendererFoxGunName = 0;
    sNdsRendererFoxGunPaletteFormat = -1;
    gNdsRendererFoxGunBytes = 0u;
}

static void ndsRendererHardwareReleaseFoxGunTexture(void)
{
    if (sNdsRendererFoxGunName != 0)
    {
        if (sNdsRendererHardwareBoundTextureName ==
            (u32)sNdsRendererFoxGunName)
        {
            sNdsRendererHardwareBoundTextureName = 0u;
        }
        ndsRendererHardwareFencedGlDeleteTextures(
            1, &sNdsRendererFoxGunName);
    }
    ndsRendererHardwareResetFoxGunTextureState();
}

NDS_RENDERER_COLD s32 ndsRendererHardwarePrepareFoxGunTexture(void)
{
    const u8 *texels;
    const u16 *palette;
    u32 texel_bytes = 0u;
    u32 palette_entries = 0u;
    int size_x;
    int size_y;
    int name = 0;
    /* No WRAP_S/WRAP_T: MiscData315's G_SETTILE (command 10) sets cmS and cmT
     * to G_TX_CLAMP, and libnds spells clamp as the absence of those bits. It
     * matters at the seam -- the baked texcoords touch the exact far edge
     * (s=512 of 512, t=1024 of 1024), which repeat wraps back to texel 0. */
    int params = TEXGEN_TEXCOORD;

    if (sNdsRendererFoxGunName != 0)
    {
        return TRUE;
    }
    gNdsRendererFoxGunPrepareCount++;
    texels = ndsFoxGunTexels(&texel_bytes);
    palette = ndsFoxGunPalette(&palette_entries);
    if ((texels == NULL) || (palette == NULL) ||
        (texel_bytes != ((NDS_FOX_GUN_TEXTURE_WIDTH *
                          NDS_FOX_GUN_TEXTURE_HEIGHT) >> 1)) ||
        (palette_entries != 16u) ||
        (ndsRendererHardwareTextureSizeEnum(
             NDS_FOX_GUN_TEXTURE_WIDTH, &size_x) == FALSE) ||
        (ndsRendererHardwareTextureSizeEnum(
             NDS_FOX_GUN_TEXTURE_HEIGHT, &size_y) == FALSE))
    {
        goto fail;
    }
    if (ndsRendererHardwareFencedGlGenTextures(1, &name) == 0)
    {
        goto fail;
    }
    sNdsRendererFoxGunName = name;
    ndsRendererHardwareBindTextureName(NULL, (u32)name);
    if (ndsRendererHardwareFencedGlTexImage2D(
            GL_TEXTURE_2D, 0, GL_RGB16, size_x, size_y, 0, params,
            texels) == 0)
    {
        goto fail;
    }
    glColorTableEXT(GL_TEXTURE_2D, 0, (int)palette_entries, 0, 0, palette);
    glGetColorTableParameterEXT(
        GL_TEXTURE_2D, GL_COLOR_TABLE_FORMAT_EXT,
        &sNdsRendererFoxGunPaletteFormat);
    if (sNdsRendererFoxGunPaletteFormat < 0)
    {
        goto fail;
    }
    gNdsRendererFoxGunBytes =
        texel_bytes + (palette_entries * sizeof(u16));
    return TRUE;

fail:
    ndsRendererHardwareReleaseFoxGunTexture();
    gNdsRendererFoxGunFailCount++;
    return FALSE;
}

/* Submit the blaster.
 *
 * `composed` is the FINAL matrix, built by the caller exactly the way a fighter
 * root's is: joint 17's world matrix, times the frame camera modelview, times
 * the projection.
 *
 * THE MATRIX GOES THROUGH ndsRendererLoadHardwareRawComposedMatrix, NOT
 * glLoadMatrix4x4. Loading it by hand is what made this row look fixed while the
 * gun was invisible, and it got two separate things wrong:
 *
 *   * The world-unit half of the encoding. Vertices are submitted as
 *     `source << (12 - NDS_RENDERER_HW_WORLD_UNIT_SHIFT)` -- the x16 below --
 *     and the composed matrix's complete homogeneous row 3 must be divided by
 *     the same 256 (ndsRendererBuildRawHardwareMatrix). Applying one half and
 *     not the other renders the mesh at 1/256 scale, correctly placed and
 *     correctly oriented and far too small to rasterize. MEASURED on
 *     build-c128-foxgun with no rebuild: the 44 corners spanned 0.036 x 0.032
 *     px around (143.97, 47.90), 44/44 inside the viewport, 0 behind the
 *     camera. The same captured matrix with the shift gives 9.245 x 8.191 px.
 *     Evidence: artifacts/verification/2026-08-12_fox-gun-matrix.txt, replayed
 *     by scripts/fox_gun_screen_bounds.py.
 *   * GL_PROJECTION. A composed MVP in GL_MODELVIEW is only right when the
 *     projection is identity, and it is NOT identity here -- this target builds
 *     NDS_R2_FIGHTER_HW_MTX=1, so the fighter that draws immediately before
 *     leaves the camera projection loaded. The pair loader sets identity.
 *
 * This runs immediately after the fighter's own production run, so it inherits
 * a live GX context and owns only what it changes: one matrix pair, one texture
 * bind, one poly format, one colour, one batch. It never touches the fighter's
 * baked stream, which is the whole point of an overlay.
 */
s32 ndsRendererSubmitFoxGun(const NDSRendererMatrix20p12 *composed)
{
    NDS_FIGHTER_PACKET_DMA_WAIT();
    const NDSFoxGunVertex *vertices;
    const u8 (*triangles)[3];
    u32 vertex_count = 0u;
    u32 triangle_count = 0u;
    u32 index;

    if (composed == NULL)
    {
        return FALSE;
    }
    if (ndsRendererHardwarePrepareFoxGunTexture() == FALSE)
    {
        return FALSE;
    }
    vertices = ndsFoxGunVertices(&vertex_count);
    triangles = ndsFoxGunTriangles(&triangle_count);
    if ((vertices == NULL) || (triangles == NULL) ||
        (vertex_count == 0u) || (triangle_count == 0u))
    {
        return FALSE;
    }

    ndsRendererHardwareEndBatch();
    ndsRendererLoadHardwareRawComposedMatrix(
        composed, ndsRendererNextMatrixGeneration());

    glEnable(GL_TEXTURE_2D);
    ndsRendererHardwareBindTextureName(NULL, (u32)sNdsRendererFoxGunName);
    /* CULL_NONE, not CULL_BACK. The source triangles wind for the N64's
     * front-face convention, and the DS's is the other one, so culling here
     * would show the gun's inside faces or nothing at all. Twenty-two triangles
     * do not pay for getting that wrong; tighten it only against a screenshot
     * pair that proves which faces the DS keeps. */
    ndsRendererHardwareSetPolyFmt(
        POLY_ALPHA(31) | POLY_CULL_NONE | POLY_ID(0));
    glBegin(GL_TRIANGLES);
    /* AFTER glBegin, and never omitted. The source combiner for this display
     * list is TEXEL0 x SHADE (G_SETCOMBINE at command 6 of MiscData315's list),
     * so the DS's MODULATE needs a latched colour; with none the gun inherits
     * whatever the fighter's last vertex left in GX and can come out black.
     * White is the honest stand-in: it shows the source palette unmodulated.
     * The source's directional term (light 0xb3b3b3 over ambient 0x808080,
     * G_MOVEWORD commands 1..4) is NOT reproduced, because hardware lighting
     * would have to rotate these normals by the vector matrix and this path
     * deliberately loads a composed MVP there -- R2-03 E16b's rule that normals
     * must never be rotated by the projection. Recorded as a fidelity delta. */
    glColor(RGB15(31, 31, 31));
    for (index = 0u; index < triangle_count; index++)
    {
        u32 corner;

        for (corner = 0u; corner < 3u; corner++)
        {
            const NDSFoxGunVertex *vertex =
                &vertices[triangles[index][corner]];

            glTexCoord2t16(
                (t16)(vertex->st[0] >> NDS_FOX_GUN_TEXCOORD_SHIFT),
                (t16)(vertex->st[1] >> NDS_FOX_GUN_TEXCOORD_SHIFT));
            glVertex3v16(
                (v16)(vertex->pos[0] * NDS_FOX_GUN_VERTEX_SCALE),
                (v16)(vertex->pos[1] * NDS_FOX_GUN_VERTEX_SCALE),
                (v16)(vertex->pos[2] * NDS_FOX_GUN_VERTEX_SCALE));
        }
    }
    ndsRendererHardwareEndBatch();
    sNdsRendererHardwareMatrixLoaded = FALSE;
    sNdsRendererHardwareMatrixMode = NDS_RENDERER_HW_MATRIX_MODE_NONE;
    gNdsRendererFoxGunDrawCount++;
    gNdsRendererFoxGunTriangleCount += triangle_count;
    return TRUE;
}

/* The afterimage trail (nds_renderer.h). The source strip
 * (ftdisplaymain.c:589-596) is one gSPVertex of `count` shaded vertices and
 * the triangles (i, i+1, i+2), (i+1, i+3, i+2) for each pair step, drawn on
 * the translucent list with combine G_CC_SHADE and culling and lighting off
 * (dFTDisplayMainAfterImageVertexDL). The DS keeps the shade colour per
 * vertex but has one alpha per polygon, so each quad is its own group at the
 * mean of its two edges' alpha; a quad whose alpha rounds to 0 is skipped
 * (DS alpha 0 draws wireframe). One polygon ID for the whole trail: where
 * the strip folds over itself the DS blends it once, the N64 twice. */
volatile u32 gNdsRendererAfterImageDrawCount;
volatile u32 gNdsRendererAfterImageRejectCount;
static void ndsRendererLoadHardwareRawComposedMatrix(
    const NDSRendererMatrix20p12 *composed, u32 generation);

#define NDS_AFTERIMAGE_POLY_ID 3u
#define NDS_AFTERIMAGE_VERTEX_SCALE (1 << (12u - NDS_RENDERER_HW_WORLD_UNIT_SHIFT))
_Static_assert(NDS_RENDERER_AFTERIMAGE_VERTEX_LIMIT * NDS_AFTERIMAGE_VERTEX_SCALE <= 32767,
               "afterimage vertices must fit a v16");
#define NDS_AFTERIMAGE_VERTEX_LIMIT NDS_RENDERER_AFTERIMAGE_VERTEX_LIMIT

s32 ndsRendererSubmitAfterImage(const NDSRendererMatrix20p12 *composed,
                                const s16 (*xyz)[3], const u8 (*rgba)[4],
                                u32 count)
{
    NDS_FIGHTER_PACKET_DMA_WAIT();
    u32 i;

    if ((composed == NULL) || (xyz == NULL) || (rgba == NULL) || (count < 4u))
    {
        return FALSE;
    }
    for (i = 0u; i < count; i++)
    {
        u32 axis;

        for (axis = 0u; axis < 3u; axis++)
        {
            if ((xyz[i][axis] > NDS_AFTERIMAGE_VERTEX_LIMIT) ||
                (xyz[i][axis] < -NDS_AFTERIMAGE_VERTEX_LIMIT))
            {
                gNdsRendererAfterImageRejectCount++;
                return FALSE;
            }
        }
    }
    ndsRendererFlushWhispyNativePacket();
    ndsRendererEndParticleQuads();
    ndsRendererHardwareEndBatch();
    ndsRendererLoadHardwareRawComposedMatrix(
        composed, ndsRendererNextMatrixGeneration());
    /* The interpreted renderer's untextured contract, as the Fox blaster's. */
    glEnable(GL_TEXTURE_2D);
    ndsRendererHardwareBindNoTexture(NULL);
    for (i = 0u; (i + 3u) < count; i += 2u)
    {
        static const u8 kCorner[6] = { 0u, 1u, 2u, 1u, 3u, 2u };
        u32 alpha = (((u32)rgba[i][3] + (u32)rgba[i + 2u][3]) / 2u) >> 3;
        u32 corner;

        if (alpha == 0u)
        {
            continue;
        }
        ndsRendererHardwareSetPolyFmt(POLY_ALPHA(alpha) | POLY_CULL_NONE |
                                      POLY_ID(NDS_AFTERIMAGE_POLY_ID));
        glBegin(GL_TRIANGLES);
        for (corner = 0u; corner < 6u; corner++)
        {
            u32 v = i + kCorner[corner];

            glColor(RGB15(rgba[v][0] >> 3, rgba[v][1] >> 3, rgba[v][2] >> 3));
            glVertex3v16((v16)(xyz[v][0] * NDS_AFTERIMAGE_VERTEX_SCALE),
                         (v16)(xyz[v][1] * NDS_AFTERIMAGE_VERTEX_SCALE),
                         (v16)(xyz[v][2] * NDS_AFTERIMAGE_VERTEX_SCALE));
        }
        ndsRendererHardwareEndBatch();
    }
    sNdsRendererHardwareMatrixLoaded = FALSE;
    sNdsRendererHardwareMatrixMode = NDS_RENDERER_HW_MATRIX_MODE_NONE;
    gNdsRendererAfterImageDrawCount++;
    return TRUE;
}

/* DEBUG-ONLY world-space collision-diamond, for tuning the fireball's
 * stage-collision box. The DS geometry engine has no line primitive, so the
 * diamond is drawn as two filled translucent triangles (a flat diamond shape)
 * through the same particle camera matrix and scale the quad batch uses, so it
 * lands exactly where the collision probes reach. Must be called while a
 * particle quad batch is OPEN -- glBegin(GL_TRIANGLE) closes the preceding quad
 * group on this hardware (no glEnd needed) and the caller's
 * ndsRendererEndParticleQuads flushes everything. The diamond is axis-aligned
 * in world space: cx/cy/cz is the fireball translate; top/center/bottom/width
 * are the MPObjectColl offsets. */
void ndsRendererSubmitDebugDiamond(f32 cx, f32 cy, f32 cz,
                                   f32 top, f32 center,
                                   f32 bottom, f32 width)
{
    NDS_FIGHTER_PACKET_DMA_WAIT();
    ndsRendererFlushWhispyNativePacket();
    if (sNdsRendererParticleQuadOpen == 0u)
    {
        return;
    }
    /* Translucent green, unlit, both faces: a visible overlay that does not
     * fully occlude the fireball sprite beneath it. Disable the texture so the
     * triangles draw as flat colour. */
    glDisable(GL_TEXTURE_2D);
    ndsRendererHardwareSetPolyFmt(POLY_ALPHA(10) | POLY_CULL_NONE |
                                  POLY_ID(0));
    glColor(RGB15(0, 31, 0));
    glBegin(GL_TRIANGLE);
    /* The batch pushed a matrix scaled by the current shift; emit vertices in
     * the same world-fixed / shift units the quad submitter uses so they land
     * in the same place. */
    {
        s32 sx = ndsRendererParticleWorldToV16(cx, sNdsRendererParticleScaleShift);
        s32 sc = ndsRendererParticleWorldToV16(cy + center,
                                               sNdsRendererParticleScaleShift);
        s32 st = ndsRendererParticleWorldToV16(cy + top,
                                               sNdsRendererParticleScaleShift);
        s32 sb = ndsRendererParticleWorldToV16(cy + bottom,
                                               sNdsRendererParticleScaleShift);
        s32 sw = ndsRendererParticleWorldToV16(width,
                                               sNdsRendererParticleScaleShift);
        s32 sz = ndsRendererParticleWorldToV16(cz,
                                               sNdsRendererParticleScaleShift);
        /* Two triangles: top/right/center, center/bottom/left -- a flat diamond
         * spanning top..bottom and -width..+width. */
        glVertex3v16(sx, st, sz);
        glVertex3v16(sx + sw, sc, sz);
        glVertex3v16(sx - sw, sc, sz);

        glVertex3v16(sx - sw, sc, sz);
        glVertex3v16(sx + sw, sc, sz);
        glVertex3v16(sx, sb, sz);
    }
    /* Leave the triangle group open; ndsRendererEndParticleQuads flushes it. Do
     * NOT re-enable GL_TEXTURE_2D here -- the batch is about to close. */
}

void ndsRendererHardwareArmBattleStaticTextures(void)
{
    if ((gNdsRendererBattleStaticTextureEnabled != 0u) &&
        (sNdsRendererBattleStaticTexturePrepared != 0u) &&
        (sNdsRendererBattleStaticTextureArmed == 0u))
    {
        memset((void *)gNdsRendererBattleTextureFenceCounts, 0,
               sizeof(gNdsRendererBattleTextureFenceCounts));
        gNdsRendererBattleTextureFenceFirstClassPlus1 = 0u;
        gNdsRendererBattleTextureFenceFirstFrame = 0u;
        gNdsRendererBattleTextureFallbackFirstFrame = 0u;
        gNdsRendererBattleTextureFallbackFirstKeyHash = 0u;
        gNdsRendererBattleTextureFallbackFirstImageAsset = 0u;
        gNdsRendererBattleTextureFallbackFirstImageOffset = 0u;
        gNdsRendererBattleTextureFallbackFirstTlutAsset = 0u;
        gNdsRendererBattleTextureFallbackFirstTlutOffset = 0u;
        gNdsRendererBattleTextureFallbackFirstFormat = 0u;
        gNdsRendererBattleTextureFallbackFirstSize = 0u;
        gNdsRendererBattleTextureFallbackFirstWidth = 0u;
        gNdsRendererBattleTextureFallbackFirstHeight = 0u;
        gNdsRendererBattleTextureFallbackFirstFlags = 0u;
        gNdsRendererBattleTextureFallbackFirstCombineW0 = 0u;
        gNdsRendererBattleTextureFallbackFirstCombineW1 = 0u;
        sNdsRendererBattleStaticTextureArmed = TRUE;
        gNdsRendererBattleStaticTextureArmCount++;
    }
}

NDS_RENDERER_COLD void ndsRendererHardwareDiscardBattleStaticTextures(void)
{
    u32 was_prepared = sNdsRendererBattleStaticTexturePrepared;

    sNdsRendererBattleStaticTexturePrepared = FALSE;
    sNdsRendererBattleStaticTextureArmed = FALSE;
    ndsRendererHardwareDiscardTextureCache();
    if (was_prepared != 0u)
    {
        gNdsRendererBattleStaticTextureTeardownCount++;
    }
    sNdsRendererBattleStaticTexturePrepared = FALSE;
    sNdsRendererBattleStaticTextureArmed = FALSE;
    gNdsRendererStaticTexturePreparedNow = 0u;
}

void ndsRendererHardwareAbortBattleStaticTextures(void)
{
    /* Counted unconditionally and before anything else can early-out: this is
     * the entry point that makes the scene's textures unrecoverable, so "was it
     * called" must never depend on a build flag. See the row 6 note above. */
    gNdsRendererStageOwnerAbortCount++;
    u32 was_prepared = sNdsRendererBattleStaticTexturePrepared;

    /* This deliberately retains the armed bit through every pinned delete.
     * DiscardTextureCache records the violation and clears the bit last. */
    ndsRendererHardwareDiscardTextureCache();
    if (was_prepared != 0u)
    {
        gNdsRendererBattleStaticTextureTeardownCount++;
    }
    sNdsRendererBattleStaticTexturePrepared = FALSE;
    sNdsRendererBattleStaticTextureArmed = FALSE;
    gNdsRendererStaticTexturePreparedNow = 0u;
}

static void ndsRendererHardwareBindNoTexture(NDSRendererStats *stats)
{
    if (sNdsRendererHardwareNoTextureName == 0)
    {
        ndsRendererHardwareEndBatch();
        if (ndsRendererHardwareFencedGlGenTextures(
                1, &sNdsRendererHardwareNoTextureName) == 0)
        {
            return;
        }
        ndsRendererHardwareBindTextureState(
            sNdsRendererHardwareNoTextureName);
        sNdsRendererHardwareBoundTextureName =
            (u32)sNdsRendererHardwareNoTextureName;
        ndsRendererHardwareFencedGlTexImage2D(
            GL_TEXTURE_2D, 0, GL_NOTEXTURE, 0, 0, 0,
            TEXGEN_TEXCOORD, NULL);
    }
    else
    {
        ndsRendererHardwareBindTextureName(
            stats, (u32)sNdsRendererHardwareNoTextureName);
    }
    sNdsRendererHardwareActiveTextureEntry = NULL;
}
