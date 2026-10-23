/* The stage's draw-all loop: the camera and display walk over the stage,
 * actor, weapon, item and effect links, the native owners each one
 * submits through, and the hardware frame. A slice of reloc_backend.c. */

s32 ndsFighterMarioFoxStageGCDrawAllLoopProofActive(void)
{
    return ((ndsFighterMarioFoxStageGCDrawAllLoopProofEnabled() != FALSE) &&
            (gNdsStageGCDrawAllLoopPrepared != 0u)) ? TRUE : FALSE;
}

static GObj *sNdsStageGCDrawAllLoopCurrentCameraGObj;
static GObj *sNdsStageGCDrawAllLoopCurrentDisplayGObj;
static s32 sNdsStageGCDrawAllLoopCurrentDisplayLinkID;
static sb32 sNdsStageGCDrawAllLoopHardwareSubmitActive;
static sb32 sNdsStageGCDrawAllLoopNativeStageArmed;
static u32 sNdsStageGCDrawAllLoopHardwareSubmitCount;
extern void ndsRendererAdapterResetDepthDiagnostics(void);

/* TRUE while a stage ACTOR (acid, Arwing, gate, scale, barrel on its stage
 * route) is being scanned. The link-6 rule below models the map layers --
 * only the main geometry layer keeps the source's Z state, the sky and
 * foreground layers draw as no-Z painter passes -- but an actor GObj is not a
 * layer: scVSBattleFuncLights hands it G_ZBUFFER and its own list decides
 * (the Zebes acid, link 12, renders AA_ZB_XLU_SURF with Z_CMP set and never
 * clears G_ZBUFFER). Stripping the bit classified the acid as no-Z, which
 * the renderer paints in the FOREGROUND band after the stage's first source-Z
 * triangle, so the pool covered the cliff faces it should sit behind
 * (owner's "acid looks wrong / players submerged", probe zebes-c1/ap2,
 * 2026-09-07). */
static sb32 sNdsStageGCDrawAllLoopActorKeepsZBuffer;

static u32 ndsStageGCDrawAllLoopInitialGeometryMode(void)
{
    /* scVSBattleFuncLights establishes this before every battle display proc.
     * Individual source lists remain free to clear and restore it. */
    u32 mode = NDS_RENDERER_GEOM_RESET_MODE | NDS_RENDERER_GEOM_LIGHTING;

    return ((sNdsStageGCDrawAllLoopCurrentDisplayLinkID == 6) ||
            (sNdsStageGCDrawAllLoopActorKeepsZBuffer != FALSE)) ?
        mode : (mode & ~NDS_RENDERER_GEOM_ZBUFFER);
}

/* Weapons draw on link 14; link 11, the items', carries P4 Sonic's spring:
 * Remix's change_spring_room_ moves it there so it sorts with the items
 * (nds_p4_sonic.c), and admitting only 14 left it undrawn (owner,
 * 2026-10-12: "Jump Pad Spring not visible"). The camera captures 11 in the
 * same pass as 14's neighbours; the GObj-kind test keeps the items out. */
static sb32 ndsStageGCDrawAllLoopIsWeaponDisplay(GObj *gobj, s32 link_id)
{
    return ((gobj != NULL) &&
            (gobj->id == nGCCommonKindWeapon) &&
            (gobj->dl_link_id == link_id) &&
            ((link_id == 14) || (link_id == 11))) ? TRUE : FALSE;
}

#define NDS_ITEM_DISPLAY_LINK 11
/* Items the source moves onto the effects link after making them: the Poke
 * Ball's Hitmonlee (itsawamura.c, gcMoveGObjDLHead(item_gobj, 18, ...)) and
 * Clefairy's Metronome copies of Hitmonlee and Starmie (itpippi.c, the same
 * call). Admitting only link 11 refused all three, which is why Hitmonlee was
 * "made, never reaches the adapter" while its two display lists sat baked and
 * unused. The camera already captures link 18 for the effects; the GObj-kind
 * check keeps that link's effect traffic out of the item submit. */
#define NDS_ITEM_EFFECT_DISPLAY_LINK 18

static sb32 ndsStageGCDrawAllLoopIsItemDisplay(GObj *gobj, s32 link_id)
{
    return ((gobj != NULL) &&
            (gobj->id == nGCCommonKindItem) &&
            (gobj->dl_link_id == link_id) &&
            ((link_id == NDS_ITEM_DISPLAY_LINK) ||
             (link_id == NDS_ITEM_EFFECT_DISPLAY_LINK))) ? TRUE : FALSE;
}

/* THIS PREDICATE IS WHAT "THE BATTLE HARDWARE PATH DOES NOT CONSUME SOURCE
 * EFFECT DL LINKS" ACTUALLY MEANS. The port wrote that sentence down twice --
 * Makefile:1401 and battleship_efmanager.c:1240 -- and neither said where it
 * lived, so three cycles looked for it in the particle atlas, in the camera's
 * capture passes and in the DObj tree walk. It is here, and it is one call:
 *
 *   ndsEFManagerIsVisualEffectGObj (efmanager.c:969) returns TRUE only when
 *   dobj->dl equals one of sNdsVisualTemplates[i].display_list.
 *
 * That is a pointer comparison against the PROCEDURAL stand-in templates. A
 * source-made effect -- efManagerMakeEffectForce loading a real DObjDesc, which
 * is what NDS_R2_SOURCE_EFFECTS_FULL switches the shield, rebirth halo, Fox
 * reflector and impact wave over to -- carries the ROM asset's display list and
 * can never match. So turning the flag on removed each stand-in and put nothing
 * on screen in its place: not because the geometry was wrong, not because the
 * camera dropped the link, but because this gate admits only the things the
 * flag replaces.
 *
 * A source model is admitted on its own terms now: the same link-18 effect
 * identity, plus a DObj carrying a display list, which is exactly what the
 * submit below already requires before it will draw anything. */
/* THE LINK IS THE WHOLE ANSWER, and the codebase was carrying its own control
 * the entire time. Every EFDesc names the display link it draws on
 * (efmanager.c, field 2 of the desc):
 *
 *     dEFManagerShieldEffectDesc        :460   link 15
 *     dEFManagerFoxReflectorEffectDesc  :420   link 15
 *     dEFManagerRebirthHaloEffectDesc   :1648  link 10
 *     dEFManagerImpactWaveEffectDesc    :201   link 10
 *     dEFManagerDeadExplodeEffectDesc   :850   link 18   <- the KO burst
 *
 * The first four are the four rows that have never drawn. The fifth is the KO
 * burst, which BUGS.md records as working "unconditionally" through exactly the
 * same source-model route. The only thing that differs between them is this
 * field, and this predicate used to require 18.
 *
 * So "the battle hardware path does not consume source effect DL links" was a
 * LINK COVERAGE GAP: the effect submit accepted one link, the procedural
 * stand-ins were hard-wired onto it (ndsEFManagerMakeVisualEffect passes 18 to
 * gcAddGObjDisplay), and the source models that live on 10 and 15 were never
 * offered to the hardware at all. Not the atlas, not the camera -- the camera
 * captures 10 in pass 3 and 15 in pass 4 -- and not the tree walk.
 *
 * Fox's entry Arwing adds the second half of the source contract: its EFDesc
 * starts on link 10, then efManagerSortZNeg moves the SAME effect GObj to link
 * 2 or 20 every update according to the animated model Z.  gmCameraDefaultProcDisplay
 * explicitly captures link 2 in its first pass and link 20 in its final pass.
 * Restricting a source effect to its descriptor's initial link therefore makes
 * the Arwing disappear as soon as the source performs its normal depth sort.
 * Admit those two source-owned sorted-effect links as well; the GObj-kind and
 * DObj-geometry checks below still prevent stage/ground traffic sharing link 2
 * from entering the effect submitter.
 *
 * Behind the second gate sits ndsEFManagerIsVisualEffectGObj (efmanager.c:969),
 * which returns TRUE only when dobj->dl matches a procedural template pointer.
 * A source model carries the ROM asset's list and can never match it, so both
 * gates had to open for any of these to draw. */
#define NDS_EFFECT_DISPLAY_LINK_TEMPLATE 18

static sb32 ndsStageGCDrawAllLoopIsEffectDisplay(GObj *gobj, s32 link_id)
{
    if ((gobj == NULL) || (gobj->id != nGCCommonKindEffect) ||
        (gobj->dl_link_id != link_id))
    {
        return FALSE;
    }
    if ((link_id == NDS_EFFECT_DISPLAY_LINK_TEMPLATE) &&
        (ndsEFManagerIsVisualEffectGObj(gobj) != FALSE))
    {
        return TRUE;
    }
    /* 10 and 15 carry ordinary source effect models; 2 and 20 are the source's
     * dynamic Z-sort destinations (Fox entry Arwing is the measured owner); 18
     * also carries the KO burst when it is not a procedural template. A display
     * list is required because the submit below refuses to draw without one. */
    /* 4 carries the stage background actors (ef/efground.c, imported whole
     * 2026-09-05 as battleship_efground.c): all 24 EFGroundDescs install
     * gcDrawDObjTreeForGObj on link 4 (efground.c:1406), and each is a
     * source model tree -- Lakitu, the Sector Z rocket and ship, the Jungle
     * bird, Ridley, the Yoster clouds, Bronto Burt and Dedede -- so the same
     * child-or-list discriminator below admits it. */
    if ((link_id == 2) || (link_id == 4) || (link_id == 10) ||
        (link_id == 15) || (link_id == 20) ||
        (link_id == NDS_EFFECT_DISPLAY_LINK_TEMPLATE))
    {
        DObj *dobj = DObjGetStruct(gobj);

        /* A CHILD COUNTS. Requiring geometry on the ROOT excluded every
         * tree-shaped source model, which is to say three of the four rows this
         * gate exists to admit. gcSetupCustomDObjs (objanim.c:2413) builds the
         * id==0 entry with gcAddDObjForGObj(gobj, dobjdesc->dl), and node0's dl
         * is NULL by construction in all three descs -- the root is
         * transform-only and the display list hangs off the child. So
         * dobj->dv != NULL was only ever true for the impact wave, whose EFDesc
         * omits flag 0x4 and therefore gets one DObj holding the list directly.
         * Measured: every one of the 72 admitted draws in a 901-frame flag-on
         * run was an impact wave, and the shield was created at frame 576 and
         * never reached the walker at all.
         *
         * A child pointer is the cheap discriminator and needs no walk: the
         * submit already recurses the tree and already declines nodes with no
         * list, so admitting a transform-only root costs one traversal of a
         * two- or three-node tree and cannot draw anything spurious. */
        if ((dobj != NULL) &&
            ((dobj->dv != NULL) || (dobj->child != NULL)))
        {
            NDS_DIAG(gNdsEffectRendererSourceModelAdmitCount++);
            return TRUE;
        }
    }
    return FALSE;
}

static void ndsStageGCDrawAllLoopRecordWeaponCapture(GObj *gobj,
                                                      s32 link_id)
{
    WPStruct *wp;

    if (ndsStageGCDrawAllLoopIsWeaponDisplay(gobj, link_id) == FALSE)
    {
        return;
    }
    NDS_DIAG(gNdsWeaponRendererCaptureCount++);
    wp = gobj->user_data.p;
    if ((wp != NULL) && (wp->kind >= 0) && (wp->kind < 32))
    {
        gNdsWeaponRendererKindMask |= 1u << (u32)wp->kind;
    }
}

static void ndsStageGCDrawAllLoopRecordEffectCapture(GObj *gobj,
                                                      s32 link_id)
{
    if (ndsStageGCDrawAllLoopIsEffectDisplay(gobj, link_id) != FALSE)
    {
        NDS_DIAG(gNdsEffectRendererCaptureCount++);
    }
}

static void ndsStageGCDrawAllLoopRecordItemCapture(GObj *gobj, s32 link_id)
{
    ITStruct *ip;

    if (ndsStageGCDrawAllLoopIsItemDisplay(gobj, link_id) == FALSE)
    {
        return;
    }
    NDS_DIAG(gNdsItemRendererCaptureCount++);
    ip = gobj->user_data.p;
    if ((ip != NULL) && (ip->kind >= 0) && (ip->kind < 32))
    {
        NDS_DIAG(gNdsItemRendererKindMask |= 1u << (u32)ip->kind);
    }
}

/* MARIO'S FIREBALL AS ONE CAMERA-FACING QUAD, the shield experiment applied to
 * a projectile. Same shape of change and the same reason it is worth making:
 * the source model is a SINGLE flat quad behind a display list, so both routes
 * put the same four vertices on screen and only the submit differs.
 *
 * Decoded from relocData file 297 on 2026-08-06 rather than assumed:
 *
 *   Vtx[0..3]  pos (0, +-150, +-150), st 0..512 in S10.5 = exactly 0..16
 *   Tex        CI4 16x16, indices 0..12 used, only index 0 transparent
 *   WPDesc     main matrix kind nGCMatrixKindTra (translation ONLY, so there
 *              is no joint scale to recover the way the shield's 0x2C had),
 *              secondary 0x47 = the MVP-recalc billboard
 *              (reloc_backend_renderer_dl.c:1853, "replace all three
 *              orientation rows ... the translation row stays live")
 *
 * A billboard whose orientation is discarded and whose translation survives is
 * precisely what ndsParticleDrawOwnTextureQuad draws, so the quad reproduces
 * the transform rather than approximating it.
 *
 * MEASURED FIRST, on the whole-match instrument (ROM E61D608B, 2026-08-06):
 * one live fireball costs 64,700 ticks/frame at P50 -- SRC 26,752 and MISC
 * 33,088 -- against 129,468 ticks of headroom under the two-VBlank boundary.
 * Two of them therefore consume all of it and the frame steps to three
 * VBlanks, which is the 30 -> 20 FPS collapse the owner reported. This flag
 * addresses the MISC half only; the SRC half is wpMapTestAll running full
 * stage collision for the projectile every frame and is a separate seam.
 *
 * The fireball census at the bottom of the submit below (FireballSubmitCount,
 * FireballTriangleCount, FireballCustom47*) counts the GENERIC route and
 * therefore reads zero with this flag on. That is correct, not a regression:
 * the counters here are its replacement. */
/* BGR555, NOT 0xRRGGBB -- red bits 0-4, green 5-9, blue 10-14. Duplicated from
 * battleship_efmanager.c deliberately rather than shared: a two-line packing
 * macro promoted to a header would be a speculative abstraction, and the
 * comment there records that getting this wrong shipped a black shield. */
#define NDS_R2_FIREBALL_BGR555(r, g, b) \
    ((((u32)(r) >> 3) & 31u) | ((((u32)(g) >> 3) & 31u) << 5) | \
     ((((u32)(b) >> 3) & 31u) << 10))

/* Engagement pair, same contract as the shield's. Draw climbing with Fallback
 * at 0 is the quad route working; Fallback climbing means the quad is refused
 * every frame and the display list is still carrying the picture -- which looks
 * identical on screen and costs MORE than the default. A measurement taken
 * without reading these would be meaningless. */
volatile u32 gNdsFireballQuadDrawCount;
volatile u32 gNdsFireballQuadFallbackCount;

static u32 sNdsFireballTextureName[NDS_FIREBALL_PALETTE_COUNT];

static s32 ndsFireballTexelFill(u8 *pixels, u32 bytes, void *user_data)
{
    (void)user_data;
    if ((pixels == NULL) || (bytes < NDS_FIREBALL_TEX_BYTES))
    {
        return FALSE;
    }
    memcpy(pixels, gNdsFireballTexels, NDS_FIREBALL_TEX_BYTES);
    return TRUE;
}

/* NOTHING IS COMPUTED HERE, and unlike the shield nothing was quantised in the
 * generator either: CI4 with an RGBA5551 TLUT whose entry 0 has alpha 0 IS
 * GL_RGB16 + COLOR0_TRANSPARENT, so preparing the fireball is an upload of the
 * source's own texels and a pointer to the source's own palette. */
static u32 ndsFireballTexture(s32 palette_index)
{
    u32 slot = ((u32)palette_index < NDS_FIREBALL_PALETTE_COUNT)
        ? (u32)palette_index : 0u;

    if (sNdsFireballTextureName[slot] != 0u)
    {
        return sNdsFireballTextureName[slot];
    }
    if (ndsRendererHardwarePrepareIFCommonPal16Atlas(
            NDS_FIREBALL_TEX_WIDTH, NDS_FIREBALL_TEX_HEIGHT,
            gNdsFireballPalettes[slot], ndsFireballTexelFill, NULL,
            &sNdsFireballTextureName[slot]) == FALSE)
    {
        sNdsFireballTextureName[slot] = 0u;
    }
    return sNdsFireballTextureName[slot];
}

/* Give last match's names back before this one allocates -- START at the
 * results screen restarts the match, and a name held across restarts would
 * grow the texture cache by one per fireball palette every time. Called from
 * efManagerInitEffects beside the shield's identical release. */
NDS_COLD void ndsWeaponReleaseBakedTextures(void)
{
    u32 slot;

    for (slot = 0u; slot < ARRAY_COUNT(sNdsFireballTextureName); slot++)
    {
        if (sNdsFireballTextureName[slot] != 0u)
        {
            ndsRendererHardwareReleaseIFCommonCloudAtlas(
                &sNdsFireballTextureName[slot]);
            sNdsFireballTextureName[slot] = 0u;
        }
    }
    gNdsFireballQuadDrawCount = 0u;
    gNdsFireballQuadFallbackCount = 0u;
}

static sb32 ndsStageGCDrawAllLoopDrawFireballQuad(DObj *root, WPStruct *wp)
{
    u32 texture_name;

    if ((root == NULL) || (wp == NULL))
    {
        return FALSE;
    }
    /* weapon_vars.fireball.index is the OWNER, not a frame: 0 Mario, 1 Luigi,
     * and wpmariofireball.c:189 writes the MObj palette_id from the same index.
     * So it selects the palette here for exactly the reason it does there. */
    texture_name = ndsFireballTexture(wp->weapon_vars.fireball.index);
    if (texture_name == 0u)
    {
        return FALSE;
    }
    return ndsParticleDrawOwnTextureQuad(
        texture_name, NDS_FIREBALL_TEX_WIDTH, NDS_FIREBALL_TEX_HEIGHT,
        &root->translate.vec.f, NDS_FIREBALL_QUAD_HALF_EXTENT,
        NDS_R2_FIREBALL_BGR555(0xff, 0xff, 0xff), 0xffu, 0.0F,
        /* THE SOURCE SPIN, not an approximation: the fireball's own update
         * adds 20 deg/frame to rotate.vec.f.x (wpmariofireball.c:98), and the
         * 0x47 orientation is RotRpyR(rotate.x, rotate.y, 0) -- the
         * interpreter path visibly rolls the quad as it flies. The quad
         * submit rolls the camera-facing basis by the SAME angle, so the spin
         * matches it frame for frame instead of being a static billboard.
         *
         * THE FACING DEPENDENCE IS A TEXTURE MIRROR, carried by rotate.vec.f.y:
         * wpMainVelSetModelPitch (wpmain.c:52-57) sets rotate.y = +90 deg
         * when vel.x >= 0 (facing right) and -90 deg facing left, at spawn
         * and again on every rebound (wpmariofireball.c:191/:118). In the
         * 0x47 matrix the +90 deg pitch maps the quad's local Z axis to
         * -screen-X and the -90 deg pitch to +screen-X -- a horizontal
         * mirror of the texture -- and the apparent roll direction reverses
         * with it. The mirror is VISIBLE on the flame streaks, so it is
         * applied to the basis here and the roll stays +rotate.x exactly as
         * the source writes it. */
        root->rotate.vec.f.x,
        (root->rotate.vec.f.y >= 0.0F) ? TRUE : FALSE);
}

/* Engagement/fallback pair for the lab arm. A rising draw count with zero
 * fallback proves the exact horizontal source contract reached direct GX;
 * fallback preserves the restored source display after a hop or reflection. */
volatile u32 gNdsFoxBlasterQuadDrawCount;
volatile u32 gNdsFoxBlasterQuadFallbackCount;

static sb32 ndsStageGCDrawAllLoopDrawFoxBlasterQuad(DObj *root, WPStruct *wp,
                                                     GObj *camera_gobj)
{
    s32 facing;

    if ((root == NULL) || (wp == NULL) || (camera_gobj == NULL) ||
        (root->rotate.vec.f.x != 0.0F) ||
        (root->rotate.vec.f.y != 0.0F) ||
        (root->scale.vec.f.y != 1.0F) ||
        (root->scale.vec.f.z != 1.0F) ||
        (wp->physics.vel_air.y != 0.0F))
    {
        return FALSE;
    }
    /* Source spawn writes exactly +/-160 and zero Y. A hop rotates that vector
     * and a reflector can redirect it, so admitting only these two bit-exact
     * values makes the fast path the natural Fox shot and nothing broader. */
    if (wp->physics.vel_air.x == 160.0F)
    {
        facing = 1;
    }
    else if (wp->physics.vel_air.x == -160.0F)
    {
        facing = -1;
    }
    else
    {
        return FALSE;
    }
    if (ndsRendererAdapterSetWorldQuadCamera(camera_gobj) == FALSE)
    {
        return FALSE;
    }
    return ndsRendererSubmitFoxBlasterQuad(
        &root->translate.vec.f, root->scale.vec.f.x,
        root->scale.vec.f.y, facing);
}

static void ndsStageGCDrawAllLoopSubmitWeaponDObj(GObj *weapon_gobj,
                                                  u32 callback_kind)
{
    DObj *root;
    WPStruct *wp;
    u32 triangle_before;
    u32 texture_ready_before;
    u32 texture_reject_before;
    u32 triangle_delta;
    u32 texture_ready_delta;
    u32 texture_reject_delta;
    u32 custom47_applied_before;
    u32 custom47_reject_before;
    u32 custom47_translation_mismatch_before;
    u32 custom47_applied_delta;
    u32 initial_geometry_mode;
    u32 x_bits;
    u32 y_bits;

    if ((weapon_gobj == NULL) ||
        (weapon_gobj != sNdsStageGCDrawAllLoopCurrentDisplayGObj) ||
        (ndsStageGCDrawAllLoopIsWeaponDisplay(
             weapon_gobj,
             sNdsStageGCDrawAllLoopCurrentDisplayLinkID) == FALSE))
    {
        return;
    }

    NDS_DIAG(gNdsWeaponRendererDObjDrawCount++);
    gNdsWeaponRendererCallbackKind = callback_kind;
    root = DObjGetStruct(weapon_gobj);
    if ((root == NULL) || ((root->dv == NULL) && (root->child == NULL)) ||
        (sNdsStageGCDrawAllLoopCurrentCameraGObj == NULL) ||
        ((callback_kind != NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_DLLINKS) &&
         (callback_kind != NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_DLHEAD1) &&
         (callback_kind != NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_TREE) &&
         (callback_kind !=
             NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_TREE_DLLINKS)))
    {
        gNdsWeaponRendererRejectedDrawCount++;
        return;
    }

    x_bits = ndsFloatBits(root->translate.vec.f.x);
    y_bits = ndsFloatBits(root->translate.vec.f.y);
    if ((gNdsWeaponRendererSubmitCount != 0u) &&
        ((x_bits != gNdsWeaponRendererLastXBits) ||
         (y_bits != gNdsWeaponRendererLastYBits)))
    {
        NDS_DIAG(gNdsWeaponRendererMovingDrawCount++);
    }
    gNdsWeaponRendererLastXBits = x_bits;
    gNdsWeaponRendererLastYBits = y_bits;

    triangle_before = gNdsStageGCDrawAllLoopHardwareTriangleCount;
    texture_ready_before =
        gNdsStageGCDrawAllLoopHardwareTextureReadyCount;
    texture_reject_before =
        gNdsStageGCDrawAllLoopHardwareTextureRejectCount;
    custom47_applied_before =
        gNdsRendererAdapterCustom47AppliedCount;
    custom47_reject_before =
        gNdsRendererAdapterCustom47RejectCount;
    custom47_translation_mismatch_before =
        gNdsRendererAdapterCustom47TranslationMismatchCount;
    {
        WPStruct *blaster_wp = weapon_gobj->user_data.p;

        if ((blaster_wp != NULL) && (blaster_wp->kind == nWPKindBlaster))
        {
            if (ndsStageGCDrawAllLoopDrawFoxBlasterQuad(
                    root, blaster_wp,
                    sNdsStageGCDrawAllLoopCurrentCameraGObj) != FALSE)
            {
                NDS_DIAG(gNdsFoxBlasterQuadDrawCount++);
                gNdsWeaponRendererSubmitCount++;
                gNdsWeaponRendererVisibleDrawCount++;
                gNdsWeaponRendererTriangleCount += 2u;
                gNdsStageGCDrawAllLoopHardwareTriangleCount += 2u;
                sNdsStageGCDrawAllLoopHardwareSubmitCount++;
                gNdsStageGCDrawAllLoopHardwareSubmitCount =
                    sNdsStageGCDrawAllLoopHardwareSubmitCount;
                return;
            }
            NDS_DIAG(gNdsFoxBlasterQuadFallbackCount++);
        }
    }
    /* Before the tree walk, not inside it: the whole point is to spend neither
     * the walk nor the display list. Falls through to the generic route on any
     * refusal -- a missing texture name or a camera the quad path will not
     * accept -- so a failure here is a slow fireball, never an absent one. */
    {
        WPStruct *fireball_wp = weapon_gobj->user_data.p;

        if ((fireball_wp != NULL) && (fireball_wp->kind == nWPKindFireball))
        {
            if (ndsStageGCDrawAllLoopDrawFireballQuad(root, fireball_wp) !=
                FALSE)
            {
                NDS_DIAG(gNdsFireballQuadDrawCount++);
                sNdsStageGCDrawAllLoopHardwareSubmitCount++;
                gNdsStageGCDrawAllLoopHardwareSubmitCount =
                    sNdsStageGCDrawAllLoopHardwareSubmitCount;
                return;
            }
            NDS_DIAG(gNdsFireballQuadFallbackCount++);
        }
    }
    initial_geometry_mode = ndsStageGCDrawAllLoopInitialGeometryMode();
    if ((initial_geometry_mode & NDS_RENDERER_GEOM_ZBUFFER) == 0u)
    {
        /* BattleShip wpDisplayDrawNormal clears Z before link-14 weapons. */
        NDS_DIAG(gNdsWeaponRendererNoZCount++);
    }

    ndsRendererAdapterBeginStageTraversal();
    if ((callback_kind == NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_TREE) ||
        (callback_kind == NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_TREE_DLLINKS))
    {
        /* BattleShip's tree callbacks recurse through children. Keep the
         * shared stage entry single-node (measured hot-path requirement), but
         * do not flatten a weapon tree into a transform-only root. */
        ndsRendererAdapterSubmitWeaponDObjTree(
            root, callback_kind, sNdsStageGCDrawAllLoopCurrentCameraGObj,
            initial_geometry_mode);
    }
    else
    {
        ndsRendererAdapterSubmitStageDObj(
            root, callback_kind, sNdsStageGCDrawAllLoopCurrentCameraGObj,
            initial_geometry_mode);
    }
    ndsRendererAdapterEndStageTraversal();

    triangle_delta =
        gNdsStageGCDrawAllLoopHardwareTriangleCount - triangle_before;
    texture_ready_delta =
        gNdsStageGCDrawAllLoopHardwareTextureReadyCount -
        texture_ready_before;
    texture_reject_delta =
        gNdsStageGCDrawAllLoopHardwareTextureRejectCount -
        texture_reject_before;
    custom47_applied_delta =
        gNdsRendererAdapterCustom47AppliedCount - custom47_applied_before;
    gNdsWeaponRendererTriangleCount += triangle_delta;
    NDS_DIAG(gNdsWeaponRendererTextureReadyCount += texture_ready_delta);
    NDS_DIAG(gNdsWeaponRendererTextureRejectCount += texture_reject_delta);

    if (triangle_delta == 0u)
    {
        gNdsWeaponRendererRejectedDrawCount++;
        return;
    }

    gNdsWeaponRendererSubmitCount++;
    if (texture_reject_delta == 0u)
    {
        gNdsWeaponRendererVisibleDrawCount++;
    }
    wp = weapon_gobj->user_data.p;
    if ((wp != NULL) && (wp->kind == nWPKindFireball))
    {
        if (gNdsWeaponRendererFireballSubmitCount == 0u)
        {
            gNdsWeaponRendererFireballFirstXBits = x_bits;
            gNdsWeaponRendererFireballFirstYBits = y_bits;
        }
        gNdsWeaponRendererFireballLastXBits = x_bits;
        gNdsWeaponRendererFireballLastYBits = y_bits;
        gNdsWeaponRendererFireballSubmitCount++;
        NDS_DIAG(gNdsWeaponRendererFireballTriangleCount += triangle_delta);
        gNdsWeaponRendererFireballCustom47AppliedCount +=
            custom47_applied_delta;
        if ((custom47_applied_delta != 1u) ||
            (gNdsRendererAdapterCustom47RejectCount !=
                custom47_reject_before) ||
            (gNdsRendererAdapterCustom47TranslationMismatchCount !=
                custom47_translation_mismatch_before))
        {
        }
        if ((texture_ready_delta != 0u) && (texture_reject_delta == 0u))
        {
            NDS_DIAG(gNdsWeaponRendererFireballVisibleDrawCount++);
        }
    }
    else if ((wp != NULL) && (wp->kind == nWPKindBlaster))
    {
        NDS_DIAG(gNdsWeaponRendererBlasterSubmitCount++);
        NDS_DIAG(gNdsWeaponRendererBlasterTriangleCount += triangle_delta);
        if (texture_reject_delta == 0u)
        {
            NDS_DIAG(gNdsWeaponRendererBlasterVisibleDrawCount++);
        }
    }
    sNdsStageGCDrawAllLoopHardwareSubmitCount++;
    gNdsStageGCDrawAllLoopHardwareSubmitCount =
        sNdsStageGCDrawAllLoopHardwareSubmitCount;
}

extern void wpDisplayDLHead1(GObj *weapon_gobj);

/*
 * Phase-2 weapon NDL owners. These are intentionally narrower than the
 * generic weapon renderer: both admitted kinds already have direct native
 * quads and both are single-DObj link-14 source weapons.
 *
 * Binding checks allocation-lifetime identity (kind + exact display callback).
 * Dispatch rechecks wpDisplayMain's live visibility branch because display
 * mode and attack state are runtime state. Collision/hitbox debug modes fall
 * through to the original source proc instead of being consumed here.
 */
u32 ndsStageGCDrawAllLoopNdlWeaponKind(void *display_gobj_ptr)
{
    GObj *weapon_gobj = display_gobj_ptr;
    WPStruct *wp;
    DObj *root;

    if ((weapon_gobj == NULL) ||
        (weapon_gobj->id != nGCCommonKindWeapon) ||
        (weapon_gobj->dl_link_id != 14))
    {
        return NDS_P2_NDL_KIND_COUNT;
    }
    wp = wpGetStruct(weapon_gobj);
    root = DObjGetStruct(weapon_gobj);
    if ((wp == NULL) || (root == NULL) ||
        ((root->dv == NULL) && (root->child == NULL)))
    {
        return NDS_P2_NDL_KIND_COUNT;
    }
    if ((wp->kind == nWPKindFireball) &&
        (weapon_gobj->proc_display == wpDisplayDLHead1))
    {
        return NDS_P2_NDL_KIND_WP_FIREBALL;
    }
    if (ndsFoxBlasterIsNativeDisplayGObj(weapon_gobj) != FALSE)
    {
        return NDS_P2_NDL_KIND_WP_BLASTER;
    }
    return NDS_P2_NDL_KIND_COUNT;
}

static sb32 ndsStageGCDrawAllLoopNdlWeaponModelVisible(const WPStruct *wp)
{
    if ((wp == NULL) || (wp->display_mode == nDBDisplayModeMapCollision))
    {
        return FALSE;
    }
    return ((wp->display_mode == nDBDisplayModeMaster) ||
            (wp->attack_coll.attack_state == nGMAttackStateOff)) ? TRUE : FALSE;
}

s32 ndsStageGCDrawAllLoopSubmitNdlWeapon(void *camera_gobj_ptr,
                                         void *display_gobj_ptr,
                                         u32 kind)
{
    GObj *camera_gobj = camera_gobj_ptr;
    GObj *weapon_gobj = display_gobj_ptr;
    WPStruct *wp;
    DObj *root;
    u32 x_bits;
    u32 y_bits;
    u32 submit_count_before;
    sb32 handled = FALSE;

    if ((camera_gobj == NULL) || (weapon_gobj == NULL) ||
        (weapon_gobj->id != nGCCommonKindWeapon) ||
        (weapon_gobj->dl_link_id != 14) ||
        (gGCCurrentCamera != camera_gobj))
    {
        return FALSE;
    }
    wp = wpGetStruct(weapon_gobj);
    root = DObjGetStruct(weapon_gobj);
    if ((wp == NULL) || (root == NULL) ||
        (ndsStageGCDrawAllLoopNdlWeaponModelVisible(wp) == FALSE))
    {
        return FALSE;
    }
    submit_count_before = gNdsWeaponRendererSubmitCount;

    switch (kind)
    {
    case NDS_P2_NDL_KIND_WP_FIREBALL:
        if ((wp->kind == nWPKindFireball) &&
            (weapon_gobj->proc_display == wpDisplayDLHead1) &&
            (ndsStageGCDrawAllLoopDrawFireballQuad(root, wp) != FALSE))
        {
            NDS_DIAG(gNdsFireballQuadDrawCount++);
            sNdsStageGCDrawAllLoopHardwareSubmitCount++;
            gNdsStageGCDrawAllLoopHardwareSubmitCount =
                sNdsStageGCDrawAllLoopHardwareSubmitCount;
            handled = TRUE;
        }
        break;
    case NDS_P2_NDL_KIND_WP_BLASTER:
        if ((ndsFoxBlasterIsNativeDisplayGObj(weapon_gobj) != FALSE) &&
            (ndsStageGCDrawAllLoopDrawFoxBlasterQuad(
                 root, wp, camera_gobj) != FALSE))
        {
            NDS_DIAG(gNdsFoxBlasterQuadDrawCount++);
            gNdsWeaponRendererSubmitCount++;
            gNdsWeaponRendererVisibleDrawCount++;
            gNdsWeaponRendererTriangleCount += 2u;
            gNdsStageGCDrawAllLoopHardwareTriangleCount += 2u;
            sNdsStageGCDrawAllLoopHardwareSubmitCount++;
            gNdsStageGCDrawAllLoopHardwareSubmitCount =
                sNdsStageGCDrawAllLoopHardwareSubmitCount;
            handled = TRUE;
        }
        break;
    default:
        break;
    }
    if (handled == FALSE)
    {
        return FALSE;
    }

    /* NDL runs before RecordCapturedDisplay and bypasses gcDrawDObjDLHead1.
     * Preserve their proof accounting without replaying either traversal. */
    gNdsStageGCDrawAllLoopCapturedDisplayCount++;
    NDS_DIAG(gNdsStageGCDrawAllLoopNonStageCaptureCount++);
    NDS_DIAG(gNdsWeaponRendererCaptureCount++);
    if ((wp->kind >= 0) && (wp->kind < 32))
    {
        gNdsWeaponRendererKindMask |= 1u << (u32)wp->kind;
    }
    NDS_DIAG(gNdsWeaponRendererDObjDrawCount++);
    gNdsWeaponRendererCallbackKind =
        NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_DLHEAD1;

    x_bits = ndsFloatBits(root->translate.vec.f.x);
    y_bits = ndsFloatBits(root->translate.vec.f.y);
    if ((submit_count_before != 0u) &&
        ((x_bits != gNdsWeaponRendererLastXBits) ||
         (y_bits != gNdsWeaponRendererLastYBits)))
    {
        NDS_DIAG(gNdsWeaponRendererMovingDrawCount++);
    }
    gNdsWeaponRendererLastXBits = x_bits;
    gNdsWeaponRendererLastYBits = y_bits;
    return TRUE;
}

/* First generic item hardware owner.  BattleShip draws items on link 11 using
 * gcDrawDObjTree* just like stage/effects, but until LinkBomb there was no DS
 * consumer for that GObj kind.  Keep it separate from the effect submit: item
 * ColAnim owns per-head render state and held items can carry custom matrix
 * kind 0x52, neither of which is effect-layer state. */
static void ndsStageGCDrawAllLoopSubmitItemDObj(GObj *item_gobj,
                                                u32 callback_kind)
{
    DObj *root;
    ITStruct *ip;
    u32 triangle_before;
    u32 texture_ready_before;
    u32 texture_reject_before;
    u32 triangle_delta;
    u32 texture_ready_delta;
    u32 texture_reject_delta;
    u32 initial_geometry_mode;

    if ((item_gobj == NULL) ||
        (item_gobj != sNdsStageGCDrawAllLoopCurrentDisplayGObj) ||
        (ndsStageGCDrawAllLoopIsItemDisplay(
             item_gobj,
             sNdsStageGCDrawAllLoopCurrentDisplayLinkID) == FALSE))
    {
        return;
    }
    NDS_DIAG(gNdsItemRendererDObjDrawCount++);
    root = DObjGetStruct(item_gobj);
    ip = item_gobj->user_data.p;
    if ((root == NULL) || (ip == NULL) ||
        ((root->dv == NULL) && (root->child == NULL)) ||
        (sNdsStageGCDrawAllLoopCurrentCameraGObj == NULL) ||
        ((callback_kind != NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_TREE) &&
         (callback_kind != NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_TREE_DLLINKS)))
    {
        gNdsItemRendererRejectedDrawCount++;
        return;
    }

    triangle_before = gNdsStageGCDrawAllLoopHardwareTriangleCount;
    texture_ready_before =
        gNdsStageGCDrawAllLoopHardwareTextureReadyCount;
    texture_reject_before =
        gNdsStageGCDrawAllLoopHardwareTextureRejectCount;
#if defined(NDS_LAB_FOURCPU_SWEEP) && NDS_LAB_FOURCPU_SWEEP
    extern volatile u32 gNdsLabItemAcc[16];
    u32 lab_item_start = cpuGetTiming();
    u32 lab_item_mark = lab_item_start;
    u32 lab_item_now;
#define NDS_LAB_ITEM_SPLIT(i)                                              \
    do                                                                     \
    {                                                                      \
        lab_item_now = cpuGetTiming();                                     \
        gNdsLabItemAcc[(i)] += lab_item_now - lab_item_mark;               \
        lab_item_mark = lab_item_now;                                      \
    } while (0)
#else
#define NDS_LAB_ITEM_SPLIT(i) ((void)0)
#endif
    ndsRendererAdapterCaptureItemDisplayProcState();
    NDS_LAB_ITEM_SPLIT(1);

    /* Source itDisplayColAnimOPA/XLU uses Z-buffered render modes. The shared
     * stage helper clears Z outside battle link 6 because weapons intentionally
     * do so; items are the opposite and opt it back in here. */
    initial_geometry_mode = ndsStageGCDrawAllLoopInitialGeometryMode() |
                            NDS_RENDERER_GEOM_ZBUFFER;
    /* The stage traversal, the tree submit and their replay
     * (ndsRendererAdapterSubmitItemDObjTreeReplay). */
    ndsRendererAdapterSubmitItemDObjTreeReplay(
        root, callback_kind, sNdsStageGCDrawAllLoopCurrentCameraGObj,
        initial_geometry_mode, (u32)ip->kind);
    NDS_LAB_ITEM_SPLIT(2);
#if defined(NDS_LAB_FOURCPU_SWEEP) && NDS_LAB_FOURCPU_SWEEP
    NDS_DIAG(gNdsLabItemAcc[0] += lab_item_mark - lab_item_start);
    NDS_DIAG(gNdsLabItemAcc[15]++);
#endif
#undef NDS_LAB_ITEM_SPLIT

    triangle_delta =
        gNdsStageGCDrawAllLoopHardwareTriangleCount - triangle_before;
    texture_ready_delta =
        gNdsStageGCDrawAllLoopHardwareTextureReadyCount -
        texture_ready_before;
    texture_reject_delta =
        gNdsStageGCDrawAllLoopHardwareTextureRejectCount -
        texture_reject_before;
    NDS_DIAG(gNdsItemRendererTriangleCount += triangle_delta);
    NDS_DIAG(gNdsItemRendererTextureReadyCount += texture_ready_delta);
    NDS_DIAG(gNdsItemRendererTextureRejectCount += texture_reject_delta);
    if (triangle_delta == 0u)
    {
        gNdsItemRendererRejectedDrawCount++;
        return;
    }
    NDS_DIAG(gNdsItemRendererSubmitCount++);
    if (texture_reject_delta == 0u)
    {
        gNdsItemRendererVisibleDrawCount++;
    }
    sNdsStageGCDrawAllLoopHardwareSubmitCount++;
    NDS_DIAG(gNdsStageGCDrawAllLoopHardwareSubmitCount =
        sNdsStageGCDrawAllLoopHardwareSubmitCount);
}

/* bit0 TREE, bit1 TREE_DLLINKS, bit2 DLLINKS, bit3 DLHEAD0, bit4 DLHEAD1,
 * bit31 anything else. Only bit0 is accepted by the submit below, so a reject
 * mask of 0x02 names TREE_DLLINKS as the kind the source models arrive with. */
static u32 ndsStageGCDrawAllLoopCallbackKindBit(u32 kind)
{
    switch (kind)
    {
    case NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_TREE:         return 1u << 0;
    case NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_TREE_DLLINKS: return 1u << 1;
    case NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_DLLINKS:      return 1u << 2;
    case NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_DLHEAD0:      return 1u << 3;
    case NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_DLHEAD1:      return 1u << 4;
    default:                                               return 1u << 31;
    }
}

/* MEASURED, not assumed: with the flag on, the source models reach the submit
 * as DLHEAD0 and NOTHING ELSE -- the probe's observed-kind mask reads 0x8 with
 * no other bit set, and the reject mask reads the same 0x8. The submit accepted
 * DOBJ_TREE alone, so all six frames were refused before the tree walk ever ran
 * (nodes=0). ndsRendererAdapterSubmitStageDObjNode already handles DLHEAD0 in
 * the same switch arm as DOBJ_TREE, so nothing downstream needed teaching. */
static sb32 ndsStageGCDrawAllLoopEffectKindAccepted(u32 callback_kind)
{
    if (callback_kind == NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_TREE)
    {
        return TRUE;
    }
    if (callback_kind == NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_DLHEAD0)
    {
        return TRUE;
    }
    /* Falcon Punch's source proc_display is lbCommonDObjScaleXProcDisplay.
     * Its descriptor owns one DObj only, so on N64 that callback submits that
     * exact node through display-list head 1. The Captain-only bridge in
     * battleship_efmanager.c records it as DLHEAD1; admit that source draw kind
     * here instead of rewriting the effect as an opaque/tree draw. */
    if (callback_kind == NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_DLHEAD1)
    {
        return TRUE;
    }
    /* TREE_DLLINKS ARRIVES TOO, once the transform-only-root rule above stops
     * discarding tree-shaped effects. Measured the moment that landed: the
     * observed-kind mask went 0x8 -> 0xa and the reject mask read 0x2, which is
     * exactly this kind and nothing else. It is the rebirth halo, whose EFDesc
     * names gcDrawDObjTreeDLLinksForGObj as its proc_display, so a DL-links
     * tree is the correct shape rather than a surprise.
     * ndsRendererAdapterSubmitStageDObjNode already walks it in the DLLINKS
     * arm, so as with DLHEAD0 nothing downstream needed teaching. */
    if (callback_kind == NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_TREE_DLLINKS)
    {
        return TRUE;
    }
    /* efManagerPikachuThunderTrailProcDisplay and
     * efManagerNessPKThunderTrailProcDisplay (efmanager.c:4496, :5018) draw
     * their one DObj through gcDrawDObjDLLinksForGObj. Refusing the kind
     * dropped both bolt tails as counted-but-silent rejected draws. */
    if (callback_kind == NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_DLLINKS)
    {
        return TRUE;
    }
    return FALSE;
}

static void ndsStageGCDrawAllLoopSubmitEffectDObj(GObj *effect_gobj,
                                                  u32 callback_kind)
{
    DObj *root;
    u32 triangle_before;
    u32 texture_ready_before;
    u32 texture_reject_before;
    u32 triangle_delta;
    u32 texture_ready_delta;
    u32 texture_reject_delta;

    if ((effect_gobj == NULL) ||
        (effect_gobj != sNdsStageGCDrawAllLoopCurrentDisplayGObj) ||
        (ndsStageGCDrawAllLoopIsEffectDisplay(
             effect_gobj,
             sNdsStageGCDrawAllLoopCurrentDisplayLinkID) == FALSE))
    {
        return;
    }
    NDS_DIAG(gNdsEffectRendererDObjDrawCount++);
    /* WHICH KIND ARRIVED, because the guard below has four clauses and a single
     * reject counter cannot say which one fired. The source-model probe reads
     * dobjdraw=6 reject=6 nodes=0 -- refused before the walk -- and dv is
     * already established by the admission gate while the camera is non-NULL
     * for the link-18 stand-ins drawing beside it, so callback_kind is the
     * remaining clause. A MASK, not a last-value: several kinds reach here per
     * frame and the last to arrive is not necessarily the refused one.
     *
     * The kinds are FOURCCs (DOBJ_TREE is 0x44545245, "DTRE"), NOT small enums,
     * so `1u << kind` is undefined and `kind < 32` is never true -- an obvious
     * shift-mask here would have stayed 0 and read as "nothing arrives". */
    gNdsEffectRendererCallbackKindMask |=
        ndsStageGCDrawAllLoopCallbackKindBit(callback_kind);
    root = DObjGetStruct(effect_gobj);
    /* Same transform-only-root rule as the admission gate above, and it has to
     * be repeated here or the gate's work is undone one call later: with only
     * the gate relaxed this path admitted 468 and rejected all 219 draws while
     * the tree walker still ran 6 times, because a source desc tree's id==0
     * root carries no display list. A root with a child is a tree; the walk
     * below declines individual nodes that have nothing to draw. */
    if ((root == NULL) ||
        ((root->dv == NULL) && (root->child == NULL)) ||
        (sNdsStageGCDrawAllLoopCurrentCameraGObj == NULL) ||
        (ndsStageGCDrawAllLoopEffectKindAccepted(callback_kind) == FALSE))
    {
        gNdsEffectRendererRejectedKindMask |=
            ndsStageGCDrawAllLoopCallbackKindBit(callback_kind);
        gNdsEffectRendererRejectedDrawCount++;
        return;
    }
    /* The efground actors (Castle's Lakitu, Dream Land's Bronto Burt and
     * Dedede, the other stages' flybys) draw through the walk below: their
     * roots are baked effect roots (generate_nds_native_item_baked.py). */
    /* THE DObj FLAGS, because the drawable test is ASYMMETRIC by kind
     * (ndsRendererAdapterStageDObjDrawable, reloc_backend_renderer_dl.c:5886):
     *
     *   DOBJ_TREE / TREE_DLLINKS -> (flags & DOBJ_FLAG_NOTEXTURE) == 0
     *   DLLINKS / DLHEAD0 / DLHEAD1 -> flags == DOBJ_FLAG_NONE
     *
     * Source models arrive as DLHEAD0, so ANY flag at all makes them
     * undrawable and the walk emits nothing -- which is exactly the tris=0
     * this path now reports. Record the flags rather than assume that: it is
     * one OR and it turns the next cycle into a single run. */
    gNdsEffectRendererDObjFlagsMask |= (u32)root->flags;
    /* WHICH FIELD HOLDS THE GEOMETRY. bit0 dl, bit1 dl_link, bit2 dv.
     * The flags hypothesis above was REFUTED by its own counter (dobjflags read
     * 0x0, so the drawable test passes), which is why this one is measured in
     * the same build that acts on it rather than after it. */
    gNdsEffectRendererDObjFieldMask |=
        ((root->dl != NULL) ? 1u : 0u) |
        ((root->dl_link != NULL) ? 2u : 0u) |
        ((root->dv != NULL) ? 4u : 0u);
    triangle_before = gNdsStageGCDrawAllLoopHardwareTriangleCount;
    texture_ready_before =
        gNdsStageGCDrawAllLoopHardwareTextureReadyCount;
    texture_reject_before =
        gNdsStageGCDrawAllLoopHardwareTextureRejectCount;
    /* Consume the colour this effect's own proc_display just emitted, before
     * the tree submit inherits the previous list's persistent RDP state. */
    ndsRendererAdapterCaptureDisplayProcColors();
    ndsRendererAdapterBeginStageTraversal();
    /* The effect path, and the only caller that walks the tree. An effect
     * EFDesc is a model -- shield, rebirth halo, Fox reflector, impact wave are
     * all multi-node.
     *
     * TWO HYPOTHESES FOR THE REMAINING tris=0 WERE TESTED HERE AND BOTH DIED,
     * which is why the kind is passed through unchanged:
     *
     *   "a DObj flag makes it undrawable" -- the DLHEAD0 drawable rule is the
     *     strict `flags == DOBJ_FLAG_NONE` (renderer_dl.c:5886). REFUTED:
     *     dobjflags reads 0x0, so the node IS drawable.
     *   "the geometry is in dl_link[], which only the *_DLLINKS kinds read, and
     *     DLHEAD0 submits `dl` while guarding on `dv`" -- REFUTED: the field
     *     mask reads 0x7, so dl, dl_link and dv are ALL non-null, and routing
     *     DLHEAD0 to TREE_DLLINKS left tris at 0 exactly as before.
     *
     * So the node is drawable, the walk reaches it, and every candidate
     * geometry pointer is populated -- and ndsRendererAdapterSubmitStageDL
     * still emits nothing from it. texready and texreject are both 0, so it
     * does not even reach texture handling. That is the next seam and it is
     * inside SubmitStageDL, not here. */
    ndsRendererAdapterSubmitEffectDObjTree(
        root,
        callback_kind,
        sNdsStageGCDrawAllLoopCurrentCameraGObj,
        ndsStageGCDrawAllLoopInitialGeometryMode() |
            NDS_RENDERER_GEOM_ZBUFFER);
    ndsRendererAdapterEndStageTraversal();
    triangle_delta =
        gNdsStageGCDrawAllLoopHardwareTriangleCount - triangle_before;
    texture_ready_delta =
        gNdsStageGCDrawAllLoopHardwareTextureReadyCount -
        texture_ready_before;
    texture_reject_delta =
        gNdsStageGCDrawAllLoopHardwareTextureRejectCount -
        texture_reject_before;
    NDS_DIAG(gNdsEffectRendererTriangleCount += triangle_delta);
    NDS_DIAG(gNdsEffectRendererTextureReadyCount += texture_ready_delta);
    NDS_DIAG(gNdsEffectRendererTextureRejectCount += texture_reject_delta);
    if (triangle_delta == 0u)
    {
        gNdsEffectRendererRejectedDrawCount++;
        return;
    }
    /* "A LINK-15 SOURCE EFFECT DREW TRIANGLES THIS FRAME", which is the arming
     * signal a shield capture needs and which no existing counter provided.
     * probe-shield-vfx.ps1 could only arm on gNdsTask39FxShieldDrawCount, which
     * belonged to the procedural stand-in and died with it, or on
     * SourceModelAdmitCount, which
     * counts every source admit and is dominated by the impact wave -- so the
     * 2026-08-03 captures armed on a wave at frame 343 and shot an empty frame.
     * Link 15 carries the shield and Fox's reflector; the wave and the rebirth
     * halo are link 10, so this separates them. Counted only past the
     * triangle_delta test, so it means DREW and not merely "was submitted". */
    if ((effect_gobj != NULL) && (effect_gobj->dl_link_id == 15))
    {
        NDS_DIAG(gNdsEffectRendererLink15DrawCount++);
    }
    NDS_DIAG(gNdsEffectRendererSubmitCount++);
    sNdsStageGCDrawAllLoopHardwareSubmitCount++;
    gNdsStageGCDrawAllLoopHardwareSubmitCount =
        sNdsStageGCDrawAllLoopHardwareSubmitCount;
}

void ndsStageGCDrawAllLoopRecordNdlEffectSubmit(
    u32 triangles, u32 texture_ready, u32 texture_reject)
{
    /* NDL enters before RecordCapturedDisplay and therefore bypasses the
     * ordinary effect callback that maintains these proof counters. Keep the
     * accounting contract identical without recreating its classification/tree
     * work. */
    NDS_DIAG(gNdsEffectRendererCaptureCount++);
    NDS_DIAG(gNdsEffectRendererDObjDrawCount++);
    NDS_DIAG(gNdsEffectRendererTriangleCount += triangles);
    NDS_DIAG(gNdsEffectRendererTextureReadyCount += texture_ready);
    NDS_DIAG(gNdsEffectRendererTextureRejectCount += texture_reject);
    NDS_DIAG(gNdsEffectRendererSubmitCount++);
    sNdsStageGCDrawAllLoopHardwareSubmitCount++;
    gNdsStageGCDrawAllLoopHardwareSubmitCount =
        sNdsStageGCDrawAllLoopHardwareSubmitCount;
}

void ndsStageGCDrawAllLoopRecordNdlItemSubmit(
    void *item_gobj_ptr, u32 triangles, u32 texture_ready, u32 texture_reject)
{
    GObj *item_gobj = item_gobj_ptr;
    ITStruct *ip = (item_gobj != NULL) ? item_gobj->user_data.p : NULL;

    /* NDL consumes the item proc before RecordCapturedDisplay, so retain the
     * same proof census that the ordinary camera path would have recorded. */
    NDS_DIAG(gNdsItemRendererCaptureCount++);
    if ((ip != NULL) && (ip->kind >= 0) && (ip->kind < 32))
    {
        gNdsItemRendererKindMask |= 1u << (u32)ip->kind;
    }
    /* Invisible held items execute no DObj draw in the source callback. */
    if (triangles == 0u)
    {
        return;
    }
    NDS_DIAG(gNdsItemRendererDObjDrawCount++);
    gNdsItemRendererTriangleCount += triangles;
    NDS_DIAG(gNdsItemRendererTextureReadyCount += texture_ready);
    NDS_DIAG(gNdsItemRendererTextureRejectCount += texture_reject);
    gNdsItemRendererSubmitCount++;
    if (texture_reject == 0u)
    {
        gNdsItemRendererVisibleDrawCount++;
    }
    sNdsStageGCDrawAllLoopHardwareSubmitCount++;
    gNdsStageGCDrawAllLoopHardwareSubmitCount =
        sNdsStageGCDrawAllLoopHardwareSubmitCount;
}

static sb32 ndsStageGCDrawAllLoopClassifyGObj(GObj *gobj, u32 *mask,
                                              sb32 *is_layer)
{
    u32 i;

    if (mask != NULL)
    {
        *mask = 0u;
    }
    if (is_layer != NULL)
    {
        *is_layer = FALSE;
    }
    if (gobj == NULL)
    {
        return FALSE;
    }
    for (i = 0u; i < ARRAY_COUNT(gGRCommonLayerGObjs); i++)
    {
        if (gobj == gGRCommonLayerGObjs[i])
        {
            if (mask != NULL)
            {
                *mask = 1u << i;
            }
            if (is_layer != NULL)
            {
                *is_layer = TRUE;
            }
            return TRUE;
        }
    }
    /* gGRCommonStruct is a UNION of every stage's ground vars: Dream Land's
     * map_gobj[4] sits at bytes 4..19, exactly where Zebes/Sector keep
     * map_gobj (the acid, the Arwing), Jungle keeps tarucann_gobj and Saffron
     * keeps gate_gobj (byte 12). Read unconditionally, this loop classified
     * every one of those actors as a Dream Land map piece and sent it down the
     * layer scan, so the native barrel arm and the ground-actor arm below
     * never saw them (probe: `ground_actor calls=0`, `gatebt2 rec=30`,
     * 2026-09-07). Dream Land is the only kind whose vars carry these. */
    if ((gSCManagerBattleState == NULL) ||
        (gSCManagerBattleState->gkind != nGRKindPupupu))
    {
        return FALSE;
    }
    for (i = 0u; i < ARRAY_COUNT(gGRCommonStruct.pupupu.map_gobj); i++)
    {
        if (gobj == gGRCommonStruct.pupupu.map_gobj[i])
        {
            if (mask != NULL)
            {
                *mask = 1u << i;
            }
            return TRUE;
        }
    }
    return FALSE;
}

static sb32 ndsStageGCDrawAllLoopIsSelectedFighter(GObj *gobj)
{
    FTStruct *fp;

    if ((gobj == NULL) || (gobj->id != nGCCommonKindFighter))
    {
        return FALSE;
    }
    fp = gobj->user_data.p;
    if (ndsFighterStructIsPoolPointer(fp) != FALSE)
    {
        return TRUE;
    }
    fp = ftGetStruct(gobj);
    return (ndsFighterStructIsPoolPointer(fp) != FALSE) ? TRUE : FALSE;
}

#if NDS_P2_STAGE_JUNGLE
/* P2-4 native stage actor: Congo Jungle barrel cannon. The imported Jungle TU
 * owns the GObj pointer (gGRCommonStruct.jungle.tarucann_gobj, written once
 * by grJungleMakeTaruCann); this route recognises the barrel by that pointer
 * plus its ground-kind/link identity, so no static stage packet or steady
 * path is touched. The source procs (gcPlayAnimAll at priority 5 plus
 * grJungleTaruCannProcUpdate at 4: translate/rotate.z on the root, Default /
 * Fill / Shoot joints on the child) already ran before display, so the live
 * DObj tree this admits carries the stepped AnimJoint pose. */
volatile u32 gNdsStageGCDrawAllLoopActorDisplayCallbackCount;
volatile u32 gNdsStageGCDrawAllLoopActorTriangleCount;
volatile u32 gNdsStageGCDrawAllLoopActorRejectCount;

static sb32 ndsStageGCDrawAllLoopIsTaruCann(GObj *gobj)
{
    GObj *tarucann_gobj;

    if ((gobj == NULL) || (gobj->id != nGCCommonKindGround))
    {
        return FALSE;
    }
    if ((gSCManagerBattleState == NULL) ||
        (gSCManagerBattleState->gkind != nGRKindJungle))
    {
        return FALSE;
    }
    tarucann_gobj = (GObj *)ndsGRJungleTaruCannGObj();
    if ((tarucann_gobj == NULL) || (gobj != tarucann_gobj))
    {
        return FALSE;
    }
    return (gobj->dl_link_id == 6u) ? TRUE : FALSE;
}
#endif

/* YOSTER-CLOUDS: per-cloud recognition by the imported Yoster TU's GObj
 * pointer (written once by grYosterInitAll) plus ground-kind/link
 * identity, so no static stage packet or steady path is touched. The
 * source procs (gcPlayAnimAll plus grYosterProcUpdate: yakumono pose on
 * the root, Solid/Evaporate prim scripts on the drawables) already ran
 * before display, so the admitted tree carries the live pose + alpha. */
#if NDS_P2_STAGE_YOSTER
volatile u32 gNdsStageGCDrawAllLoopYosterCloudDisplayCallbackCount;
volatile u32 gNdsStageGCDrawAllLoopYosterCloudTriangleCount;
volatile u32 gNdsStageGCDrawAllLoopYosterCloudRejectCount;

static sb32 ndsStageGCDrawAllLoopIsYosterCloud(GObj *gobj, u32 *index_out)
{
    u32 i;

    if ((gobj == NULL) || (gobj->id != nGCCommonKindGround))
    {
        return FALSE;
    }
    if ((gSCManagerBattleState == NULL) ||
        (gSCManagerBattleState->gkind != nGRKindYoster))
    {
        return FALSE;
    }
    for (i = 0u; i < 3u; i++)
    {
        if ((gobj == (GObj *)ndsGRYosterCloudGObj(i)) &&
            (gobj->dl_link_id == 6u))
        {
            if (index_out != NULL)
            {
                *index_out = i;
            }
            return TRUE;
        }
    }
    return FALSE;
}

static sb32 ndsStageGCDrawAllLoopSubmitYosterCloudDObjForCamera(
    GObj *cloud_gobj, GObj *camera_gobj, u32 callback_kind,
    u32 initial_geometry_mode, sb32 record_reject)
{
    DObj *root;
    CObj *cobj;
    u32 triangle_delta;
    NDSRendererStats stats;
    sb32 submitted;

    if ((cloud_gobj == NULL) ||
        (ndsStageGCDrawAllLoopIsYosterCloud(cloud_gobj, NULL) == FALSE))
    {
        return FALSE;
    }
    root = DObjGetStruct(cloud_gobj);
    if ((root == NULL) ||
        (camera_gobj == NULL) ||
        (callback_kind != NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_TREE))
    {
        return FALSE;
    }
    cobj = CObjGetStruct(camera_gobj);
    ndsRendererInitStats(&stats);
    submitted = ndsRendererAdapterSubmitNativeYosterCloud(root, cobj,
        initial_geometry_mode, &stats);
    if (submitted == FALSE)
    {
        if (record_reject != FALSE)
        {
            gNdsStageGCDrawAllLoopHardwareTextureBindCount +=
                stats.hardware_texture_bind_count;
            gNdsStageGCDrawAllLoopHardwareTextureUploadCount +=
                stats.hardware_texture_upload_count;
            NDS_DIAG(gNdsStageGCDrawAllLoopYosterCloudRejectCount++);
        }
        return FALSE;
    }
    NDS_DIAG(gNdsStageGCDrawAllLoopHardwareTextureBindCount += stats.hardware_texture_bind_count);
    NDS_DIAG(gNdsStageGCDrawAllLoopHardwareTextureUploadCount += stats.hardware_texture_upload_count);
    triangle_delta = stats.hardware_triangle_count;
    gNdsStageGCDrawAllLoopHardwareTriangleCount += triangle_delta;
    gNdsStageGCDrawAllLoopHardwareZBufferTriangleCount +=
        stats.hardware_zbuffer_triangle_count;
    NDS_DIAG(gNdsStageGCDrawAllLoopYosterCloudTriangleCount += triangle_delta);
    if (triangle_delta == 0u)
    {
        return TRUE;
    }
    sNdsStageGCDrawAllLoopHardwareSubmitCount++;
    gNdsStageGCDrawAllLoopHardwareSubmitCount =
        sNdsStageGCDrawAllLoopHardwareSubmitCount;
    return TRUE;
}

static void ndsStageGCDrawAllLoopSubmitYosterCloudDObj(GObj *cloud_gobj,
    u32 callback_kind)
{
    if ((cloud_gobj == NULL) ||
        (cloud_gobj != sNdsStageGCDrawAllLoopCurrentDisplayGObj))
    {
        return;
    }
    (void)ndsStageGCDrawAllLoopSubmitYosterCloudDObjForCamera(
        cloud_gobj, sNdsStageGCDrawAllLoopCurrentCameraGObj, callback_kind,
        ndsStageGCDrawAllLoopInitialGeometryMode(), TRUE);
}
#endif


#if NDS_P2_STAGE_JUNGLE
static sb32 ndsStageGCDrawAllLoopSubmitTaruCannDObjForCamera(
    GObj *tarucann_gobj, GObj *camera_gobj, u32 callback_kind,
    u32 initial_geometry_mode, sb32 record_reject)
{
    DObj *root;
    CObj *cobj;
    u32 triangle_delta;
    NDSRendererStats stats;
    sb32 submitted;

    if ((tarucann_gobj == NULL) ||
        (ndsStageGCDrawAllLoopIsTaruCann(tarucann_gobj) == FALSE))
    {
        return FALSE;
    }
    root = DObjGetStruct(tarucann_gobj);
    if ((root == NULL) || (root->child == NULL) ||
        (camera_gobj == NULL) ||
        (callback_kind != NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_TREE))
    {
        return FALSE;
    }
    cobj = CObjGetStruct(camera_gobj);
    ndsRendererInitStats(&stats);
    submitted = ndsRendererAdapterSubmitNativeTaruCann(root, cobj,
        initial_geometry_mode, &stats);
    /* A failed texture upload may already have bound or evicted cache entries.
     * Keep that owner's telemetry even when the actor emitted no triangles. */
    if (submitted == FALSE)
    {
        if (record_reject != FALSE)
        {
            gNdsStageGCDrawAllLoopHardwareTextureBindCount +=
                stats.hardware_texture_bind_count;
            gNdsStageGCDrawAllLoopHardwareTextureUploadCount +=
                stats.hardware_texture_upload_count;
            gNdsStageGCDrawAllLoopHardwareTextureReadyCount +=
                stats.hardware_texture_ready_count;
            gNdsStageGCDrawAllLoopHardwareTextureRejectCount +=
                stats.hardware_texture_reject_count;
            NDS_DIAG(gNdsStageGCDrawAllLoopActorRejectCount++);
        }
        return FALSE;
    }
    NDS_DIAG(gNdsStageGCDrawAllLoopHardwareTextureBindCount += stats.hardware_texture_bind_count);
    NDS_DIAG(gNdsStageGCDrawAllLoopHardwareTextureUploadCount += stats.hardware_texture_upload_count);
    gNdsStageGCDrawAllLoopHardwareTextureReadyCount += stats.hardware_texture_ready_count;
    gNdsStageGCDrawAllLoopHardwareTextureRejectCount += stats.hardware_texture_reject_count;
    triangle_delta = stats.hardware_triangle_count;
    gNdsStageGCDrawAllLoopHardwareTriangleCount += triangle_delta;
    gNdsStageGCDrawAllLoopHardwareZBufferTriangleCount +=
        stats.hardware_zbuffer_triangle_count;
    NDS_DIAG(gNdsStageGCDrawAllLoopActorTriangleCount += triangle_delta);
    if (triangle_delta == 0u)
    {
        return TRUE;
    }
    sNdsStageGCDrawAllLoopHardwareSubmitCount++;
    gNdsStageGCDrawAllLoopHardwareSubmitCount =
        sNdsStageGCDrawAllLoopHardwareSubmitCount;
    return TRUE;
}

static void ndsStageGCDrawAllLoopSubmitTaruCannDObj(GObj *tarucann_gobj,
                                                   u32 callback_kind)
{
    if ((tarucann_gobj == NULL) ||
        (tarucann_gobj != sNdsStageGCDrawAllLoopCurrentDisplayGObj))
    {
        return;
    }
    (void)ndsStageGCDrawAllLoopSubmitTaruCannDObjForCamera(
        tarucann_gobj, sNdsStageGCDrawAllLoopCurrentCameraGObj, callback_kind,
        ndsStageGCDrawAllLoopInitialGeometryMode(), TRUE);
}
#endif

u32 ndsStageGCDrawAllLoopNdlGroundKind(void *display_gobj)
{
    GObj *gobj = (GObj *)display_gobj;

    if ((gobj == NULL) || (gobj->proc_display != gcDrawDObjTreeForGObj))
    {
        return NDS_P2_NDL_KIND_COUNT;
    }
#if NDS_P2_STAGE_YOSTER
    if (ndsStageGCDrawAllLoopIsYosterCloud(gobj, NULL) != FALSE)
    {
        return NDS_P2_NDL_KIND_GR_YOSTER_CLOUD;
    }
#endif
#if NDS_P2_STAGE_JUNGLE
    if (ndsStageGCDrawAllLoopIsTaruCann(gobj) != FALSE)
    {
        return NDS_P2_NDL_KIND_GR_TARUCANN;
    }
#endif
    return NDS_P2_NDL_KIND_COUNT;
}

s32 ndsStageGCDrawAllLoopSubmitNdlGround(void *camera_gobj,
                                          void *display_gobj,
                                          u32 kind)
{
    GObj *camera = (GObj *)camera_gobj;
    GObj *display = (GObj *)display_gobj;
    const u32 initial_geometry_mode =
        NDS_RENDERER_GEOM_RESET_MODE | NDS_RENDERER_GEOM_LIGHTING;
    sb32 handled = FALSE;

    if ((camera == NULL) || (display == NULL) || (display->dl_link_id != 6u))
    {
        return FALSE;
    }
    switch (kind)
    {
#if NDS_P2_STAGE_YOSTER
    case NDS_P2_NDL_KIND_GR_YOSTER_CLOUD:
        handled = ndsStageGCDrawAllLoopSubmitYosterCloudDObjForCamera(
            display, camera, NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_TREE,
            initial_geometry_mode, FALSE);
        if (handled != FALSE)
        {
            NDS_DIAG(gNdsStageGCDrawAllLoopYosterCloudDisplayCallbackCount++);
        }
        break;
#endif
#if NDS_P2_STAGE_JUNGLE
    case NDS_P2_NDL_KIND_GR_TARUCANN:
        handled = ndsStageGCDrawAllLoopSubmitTaruCannDObjForCamera(
            display, camera, NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_TREE,
            initial_geometry_mode, FALSE);
        if (handled != FALSE)
        {
            NDS_DIAG(gNdsStageGCDrawAllLoopActorDisplayCallbackCount++);
        }
        break;
#endif
    default:
        return FALSE;
    }
    if (handled != FALSE)
    {
        /* RecordCapturedDisplay is skipped by NDL.  Preserve its generic
         * accepted-display count; these ground actors are classification
         * rejects, so RecordDObjDraw has no generic callback counter to copy. */
        gNdsStageGCDrawAllLoopCapturedDisplayCount++;
    }
    return handled;
}

/* GROUND-ACTORS: generic fallback for the Ground-kind stage actors the
 * classify branch rejects. Source: grzebes.c:80-83 (acid map_gobj, Ground
 * kind, gcDrawDObjTreeDLLinksForGObj on link 12), grsector.c:1098-1102
 * (Arwing map_gobj, Ground kind, TreeDLLinks on link 6),
 * gryamabuki.c:250-252 (gate_gobj, Ground kind, TreeDLLinks on link 6),
 * grinishie.c:356-370 (string tree with gcDrawDObjTreeForGObj plus two
 * platform GObjs with gcDrawDObjDLHead0, all Ground kind on link 6). Each
 * recognizer follows IsTaruCann: ground kind, stage gkind, pointer equality
 * with the imported TU's GObj, link id. The one submit routes the admitted
 * tree through the item hardware DL submission, passing the source callback
 * kind through so TREE walks dv lists, TREE_DLLINKS walks each DObj's
 * dl_link array, and DLHEAD0 draws only head 0. */
#if NDS_P2_STAGE_ZEBES || NDS_P2_STAGE_SECTOR || NDS_P2_STAGE_YAMABUKI || \
    NDS_P2_STAGE_INISHIE
volatile u32 gNdsStageGCDrawAllLoopGroundActorSubmitCount;
volatile u32 gNdsStageGCDrawAllLoopGroundActorRejectCount;
volatile u32 gNdsStageGCDrawAllLoopGroundActorCallCount;
volatile u32 gNdsStageGCDrawAllLoopGroundActorGuardCount;
volatile u32 gNdsStageGCDrawAllLoopGroundActorLastGObj;
volatile u32 gNdsStageGCDrawAllLoopGroundActorLastId;
volatile u32 gNdsStageGCDrawAllLoopGroundActorLastLink;
volatile u32 gNdsStageGCDrawAllLoopGateFailStep;
volatile u32 gNdsStageGCDrawAllLoopGateSeenCount;
/* Saffron gate motion witness; see the sample site for what each value means.
 * TraZ is stored as a signed integer reinterpreted through u32 so a debugger
 * reads it without a float decode: 330 open, 0 closed. */
__attribute__((used)) volatile u32 gNdsYamabukiGateChildTraZ;
__attribute__((used)) volatile u32 gNdsYamabukiGateChildTraZMin;
__attribute__((used)) volatile u32 gNdsYamabukiGateChildTraZMax;
__attribute__((used)) volatile u32 gNdsYamabukiGateChildTraZSamples;
__attribute__((used)) volatile u32 gNdsYamabukiGateStatus;
/* gGRCommonStruct.yamabuki.gate_gobj as the draw loop sees it, plus how many
 * Saffron ground objects were offered. GroundSeen > 0 with GateGObjPtr == 0
 * means the gate maker was refused and there is no door object at all. */
__attribute__((used)) volatile u32 gNdsYamabukiGateGObjPtr;
__attribute__((used)) volatile u32 gNdsYamabukiGroundSeenCount;
static void ndsStageGCDrawAllLoopScanDObjs(GObj *gobj, u32 owner_mask,
                                           sb32 is_layer, u32 kind,
                                           u32 callback_kind);
#endif
#if NDS_P2_STAGE_ZEBES
static sb32 ndsStageGCDrawAllLoopIsZebesAcid(GObj *gobj)
{
    GObj *acid_gobj;

    if ((gobj == NULL) || (gobj->id != nGCCommonKindGround))
    {
        return FALSE;
    }
    if ((gSCManagerBattleState == NULL) ||
        (gSCManagerBattleState->gkind != nGRKindZebes))
    {
        return FALSE;
    }
    acid_gobj = (GObj *)ndsGRZebesAcidGObj();
    if ((acid_gobj == NULL) || (gobj != acid_gobj))
    {
        return FALSE;
    }
    return (gobj->dl_link_id == 12u) ? TRUE : FALSE;
}
#endif
#if NDS_P2_STAGE_SECTOR
static sb32 ndsStageGCDrawAllLoopIsSectorArwing(GObj *gobj)
{
    GObj *arwing_gobj;

    if ((gobj == NULL) || (gobj->id != nGCCommonKindGround))
    {
        return FALSE;
    }
    if ((gSCManagerBattleState == NULL) ||
        (gSCManagerBattleState->gkind != nGRKindSector))
    {
        return FALSE;
    }
    arwing_gobj = (GObj *)ndsGRSectorArwingGObj();
    if ((arwing_gobj == NULL) || (gobj != arwing_gobj))
    {
        return FALSE;
    }
    return (gobj->dl_link_id == 6u) ? TRUE : FALSE;
}
#endif
#if NDS_P2_STAGE_YAMABUKI
static sb32 ndsStageGCDrawAllLoopIsYamabukiGate(GObj *gobj)
{
    GObj *gate_gobj;

    if ((gobj == NULL) || (gobj->id != nGCCommonKindGround))
    {
        return FALSE;
    }
    if ((gSCManagerBattleState == NULL) ||
        (gSCManagerBattleState->gkind != nGRKindYamabuki))
    {
        return FALSE;
    }
    gate_gobj = (GObj *)ndsGRYamabukiGateGObj();
    /* Recorded BEFORE the early return, and for every Saffron ground object,
     * so a zero here means something different from a zero in the counters
     * below. grYamabukiMakeGate stores this pointer once
     * (gGRCommonStruct.yamabuki.gate_gobj); if gcMakeGObjSPAfter refused the
     * maker it is NULL, gcAddAnimJointAll(NULL, ...) installs nothing, the
     * gate's DObjs never exist, and the doorway is a permanent hole -- which
     * looks exactly like a door stuck open. The 2026-09-07 probe's
     * `ground_actor submit=0 reject=0` on this stage is consistent with that
     * and with three other things, which is why it needs its own witness
     * rather than an inference. */
    gNdsYamabukiGateGObjPtr = (u32)(uintptr_t)gate_gobj;
    NDS_DIAG(gNdsYamabukiGroundSeenCount++);
    if ((gate_gobj == NULL) || (gobj != gate_gobj))
    {
        return FALSE;
    }
    /* Witness for the arm that never fired on Saffron (probe `ground_actor
     * submit=0 reject=0`, 2026-09-07): 1 = wrong link, 0 = recognised. */
    gNdsStageGCDrawAllLoopGateFailStep = (gobj->dl_link_id == 6u) ? 0u : 1u;
    NDS_DIAG(gNdsStageGCDrawAllLoopGateSeenCount++);
    /* DOES THE DOOR ACTUALLY MOVE? The owner reports it permanently open, and
     * the two surviving explanations are indistinguishable from outside: a
     * state machine stuck in Open, or an animation installed and never
     * advanced. Frame 0 of the CLOSE script is itself the open pose --
     * 160_StageYamabukiFile4.c:158 sets TraZ = 330 before ramping to 0 -- and
     * grYamabukiMakeGate installs exactly that script at setup, so a gate
     * whose joints are never played sits at 330 for the whole match.
     *
     * Sample the first animated child's TraZ here, where the gate is already
     * identified and the cost is one load per draw. Pinned at 330 means the
     * joints are not being advanced; sweeping 330 to 0 and back means they are
     * and the fault is upstream in the state machine; pinned at 0 means the
     * door is closed and the owner is describing different geometry. `Min` and
     * `Max` are the whole point -- a single sample cannot tell a frozen value
     * from one caught mid-cycle. */
    {
        DObj *root = DObjGetStruct(gobj);
        DObj *child = (root != NULL) ? root->child : NULL;

        if (child != NULL)
        {
            s32 traz = (s32)child->translate.vec.f.z;

            gNdsYamabukiGateChildTraZ = (u32)traz;
            if (gNdsYamabukiGateChildTraZSamples == 0u)
            {
                gNdsYamabukiGateChildTraZMin = (u32)traz;
                gNdsYamabukiGateChildTraZMax = (u32)traz;
            }
            else
            {
                if (traz < (s32)gNdsYamabukiGateChildTraZMin)
                {
                    gNdsYamabukiGateChildTraZMin = (u32)traz;
                }
                if (traz > (s32)gNdsYamabukiGateChildTraZMax)
                {
                    gNdsYamabukiGateChildTraZMax = (u32)traz;
                }
            }
            gNdsYamabukiGateChildTraZSamples++;
        }
        gNdsYamabukiGateStatus = (u32)gGRCommonStruct.yamabuki.gate_status;
    }
    return (gobj->dl_link_id == 6u) ? TRUE : FALSE;
}
#endif
#if NDS_P2_STAGE_INISHIE
static sb32 ndsStageGCDrawAllLoopIsInishieScale(GObj *gobj)
{
    u32 i;

    if ((gobj == NULL) || (gobj->id != nGCCommonKindGround))
    {
        return FALSE;
    }
    if ((gSCManagerBattleState == NULL) ||
        (gSCManagerBattleState->gkind != nGRKindInishie))
    {
        return FALSE;
    }
    if (gobj->dl_link_id != 6u)
    {
        return FALSE;
    }
    /* The source keeps no scale GObj: the string tree owns one Ground GObj
     * (grinishie.c:356) and each platform owns one more (:369-370). Both are
     * recovered from their stored DObjs' parent_gobj by the imported TU. */
    for (i = 0u; i < 2u; i++)
    {
        if ((gobj == (GObj *)ndsGRInishieScaleStringGObj(i)) ||
            (gobj == (GObj *)ndsGRInishieScalePlatformGObj(i)))
        {
            return TRUE;
        }
    }
    return FALSE;
}
#endif
#if NDS_P2_STAGE_ZEBES || NDS_P2_STAGE_SECTOR || NDS_P2_STAGE_YAMABUKI || \
    NDS_P2_STAGE_INISHIE
static void ndsStageGCDrawAllLoopSubmitGroundActorDObj(GObj *actor_gobj,
                                                       u32 callback_kind)
{
    DObj *root;
    u32 triangle_before;
    u32 triangle_delta;
    u32 initial_geometry_mode;
    sb32 is_actor = FALSE;
#if NDS_P2_STAGE_SECTOR
    sb32 is_sector_arwing = FALSE;
    u32 two_pass_dobjs = 0u;
#endif

    NDS_DIAG(gNdsStageGCDrawAllLoopGroundActorCallCount++);
    if ((actor_gobj == NULL) ||
        (actor_gobj != sNdsStageGCDrawAllLoopCurrentDisplayGObj))
    {
        NDS_DIAG(gNdsStageGCDrawAllLoopGroundActorGuardCount++);
        return;
    }
    gNdsStageGCDrawAllLoopGroundActorLastGObj = (u32)(uintptr_t)actor_gobj;
    gNdsStageGCDrawAllLoopGroundActorLastId = actor_gobj->id;
    gNdsStageGCDrawAllLoopGroundActorLastLink = actor_gobj->dl_link_id;
#if NDS_P2_STAGE_ZEBES
    if (ndsStageGCDrawAllLoopIsZebesAcid(actor_gobj) != FALSE)
    {
        is_actor = TRUE;
    }
#endif
#if NDS_P2_STAGE_SECTOR
    if (ndsStageGCDrawAllLoopIsSectorArwing(actor_gobj) != FALSE)
    {
        is_actor = TRUE;
        is_sector_arwing = TRUE;
    }
#endif
#if NDS_P2_STAGE_YAMABUKI
    if (ndsStageGCDrawAllLoopIsYamabukiGate(actor_gobj) != FALSE)
    {
        is_actor = TRUE;
    }
#endif
#if NDS_P2_STAGE_INISHIE
    if (ndsStageGCDrawAllLoopIsInishieScale(actor_gobj) != FALSE)
    {
        is_actor = TRUE;
    }
#endif
    if (is_actor == FALSE)
    {
        return;
    }
    root = DObjGetStruct(actor_gobj);
    if ((root == NULL) ||
        ((root->dv == NULL) && (root->child == NULL)) ||
        (sNdsStageGCDrawAllLoopCurrentCameraGObj == NULL) ||
        ((callback_kind != NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_TREE) &&
         (callback_kind !=
             NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_TREE_DLLINKS) &&
         (callback_kind != NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_DLHEAD0)))
    {
        NDS_DIAG(gNdsStageGCDrawAllLoopGroundActorRejectCount++);
        return;
    }
    triangle_before = gNdsStageGCDrawAllLoopHardwareTriangleCount;
    (void)initial_geometry_mode;
    /* The same per-DObj stage submit the layer scan uses. Until 2026-09-07
     * these actors reached that scan by accident (the ground-vars union
     * aliased them onto Dream Land's map GObjs) and drew correctly; the item
     * tree submit this arm used to call drew the Zebes acid out of place and
     * cost ~5 FPS on the same frame, so the proven stage route stays. */
    ndsRendererAdapterBeginStageTraversal();
    sNdsStageGCDrawAllLoopActorKeepsZBuffer = TRUE;
#if NDS_P2_STAGE_SECTOR
    /* Sector Z's Arwing: the scan's lists in two passes, matrices first
     * (ndsRendererAdapterSubmitArwingTwoPass). The scan's own bookkeeping for
     * an actor (owner mask 0) is the submit count and the kind bit. */
    if ((is_sector_arwing != FALSE) &&
        (callback_kind == NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_TREE_DLLINKS) &&
        (sNdsStageGCDrawAllLoopHardwareSubmitActive != FALSE) &&
        (ndsRendererAdapterSubmitArwingTwoPass(
             root, sNdsStageGCDrawAllLoopCurrentCameraGObj,
             ndsStageGCDrawAllLoopInitialGeometryMode(),
             &two_pass_dobjs) != FALSE))
    {
        sNdsStageGCDrawAllLoopHardwareSubmitCount += two_pass_dobjs;
        gNdsStageGCDrawAllLoopHardwareSubmitCount =
            sNdsStageGCDrawAllLoopHardwareSubmitCount;
        gNdsStageGCDrawAllLoopDObjDrawKindMask |=
            1u << ((callback_kind < 32u) ? callback_kind : 31u);
    }
    else
#endif
    {
        ndsStageGCDrawAllLoopScanDObjs(actor_gobj, 0u, FALSE,
                                       (callback_kind < 32u) ? callback_kind : 31u,
                                       callback_kind);
    }
    sNdsStageGCDrawAllLoopActorKeepsZBuffer = FALSE;
    ndsRendererAdapterEndStageTraversal();
    triangle_delta =
        gNdsStageGCDrawAllLoopHardwareTriangleCount - triangle_before;
    if (triangle_delta == 0u)
    {
        NDS_DIAG(gNdsStageGCDrawAllLoopGroundActorRejectCount++);
        /* A recognised ground actor that scanned its whole tree and emitted
         * nothing is a successful empty draw, which the native-only contract
         * forbids outright (docs/reviews/NATIVE_ONLY_IMPLEMENTATION_GOAL.md:
         * a required unsupported state is a failure, not an empty draw). The
         * counter alone said how often it happened and never which actor, so
         * Saffron's gate and the Sector Arwing were indistinguishable in it. */
        ndsRendererRecordNativeFailure(
            NDS_NATIVE_FAILURE_STAGE, (u32)gSCManagerSceneData.scene_curr,
            (u32)gSCManagerBattleState->gkind,
            (u32)actor_gobj->dl_link_id, (u32)(uintptr_t)actor_gobj, 0u,
            NDS_NATIVE_FAILURE_REJECTED_PROGRAM);
        return;
    }
    NDS_DIAG(gNdsStageGCDrawAllLoopGroundActorSubmitCount++);
}
#endif

static void ndsStageGCDrawAllLoopScanDObjs(GObj *gobj, u32 owner_mask,
                                           sb32 is_layer, u32 kind,
                                           u32 callback_kind)
{
    DObj *stack[128];
    u32 stack_count = 0u;
    u32 scanned = 0u;
    DObj *root;

    if (gobj == NULL)
    {
        return;
    }
    root = DObjGetStruct(gobj);
    if (root == NULL)
    {
        return;
    }
    stack[stack_count++] = root;
    while ((stack_count != 0u) && (scanned < ARRAY_COUNT(stack)))
    {
        DObj *dobj = stack[--stack_count];

        if (dobj == NULL)
        {
            continue;
        }
        scanned++;
        /* objdisplay.c gcDrawDObjTree / gcDrawDObjTreeDLLinks (:1563, :1707):
         * a HIDDEN DObj draws nothing and neither does its subtree; its
         * siblings still draw. The Sector Z Arwing shows one of its DObjs 7
         * and 9 and hides the other (grsector.c:457-467); the scan drew the
         * hidden one too, an orange shape the owner read as a debug overlay
         * of the platform (r58). */
        if ((dobj->flags & DOBJ_FLAG_HIDDEN) != 0)
        {
            if ((dobj->sib_next != NULL) && (stack_count < ARRAY_COUNT(stack)))
            {
                stack[stack_count++] = dobj->sib_next;
            }
            continue;
        }
        if (is_layer != FALSE)
        {
            gNdsStageGCDrawAllLoopLayerDObjMask |= owner_mask;
            if (dobj->dv != NULL)
            {
                gNdsStageGCDrawAllLoopLayerDLReadyMask |= owner_mask;
            }
            if (dobj->mobj != NULL)
            {
                gNdsStageGCDrawAllLoopLayerMObjMask |= owner_mask;
            }
        }
        else
        {
            gNdsStageGCDrawAllLoopMapDObjMask |= owner_mask;
            if (dobj->dv != NULL)
            {
                gNdsStageGCDrawAllLoopMapDLReadyMask |= owner_mask;
            }
            if (dobj->mobj != NULL)
            {
                gNdsStageGCDrawAllLoopMapMObjMask |= owner_mask;
            }
        }
        /* DOBJ_FLAG_NOTEXTURE: the source skips this DObj's own lists and
         * still draws its children (objdisplay.c:1568, :1714). */
        if ((dobj->dv != NULL) &&
            ((dobj->flags & DOBJ_FLAG_NOTEXTURE) == 0) &&
            (sNdsStageGCDrawAllLoopHardwareSubmitActive != FALSE) &&
            ((callback_kind == NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_TREE) ||
             (callback_kind ==
                 NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_TREE_DLLINKS) ||
             (((callback_kind ==
                    NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_DLLINKS) ||
               (callback_kind ==
                    NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_DLHEAD0) ||
               (callback_kind ==
                    NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_DLHEAD1)) &&
              (dobj == root))))
        {
            ndsRendererAdapterSubmitStageDObj(
                dobj,
                callback_kind,
                sNdsStageGCDrawAllLoopCurrentCameraGObj,
                ndsStageGCDrawAllLoopInitialGeometryMode());
            sNdsStageGCDrawAllLoopHardwareSubmitCount++;
            gNdsStageGCDrawAllLoopHardwareSubmitCount =
                sNdsStageGCDrawAllLoopHardwareSubmitCount;
        }
        if ((dobj->sib_next != NULL) && (stack_count < ARRAY_COUNT(stack)))
        {
            stack[stack_count++] = dobj->sib_next;
        }
        if ((dobj->child != NULL) && (stack_count < ARRAY_COUNT(stack)))
        {
            stack[stack_count++] = dobj->child;
        }
    }
    gNdsStageGCDrawAllLoopDObjDrawKindMask |= 1u << kind;
}

void ndsStageGCDrawAllLoopRecordCameraCallback(void)
{
    if ((ndsFighterMarioFoxStageGCDrawAllLoopProofActive() == FALSE) ||
        (gNdsSceneManagerCurrIsBattle == 0u) ||
        ((sNdsFighterGCDrawAllLoopDisplayActive == FALSE) &&
         (sNdsStageGCDrawAllLoopHardwareSubmitActive == FALSE)))
    {
        return;
    }
    gNdsStageGCDrawAllLoopCameraCallbackCount++;
}


#if NDS_P2_1P_GAME
/* Board the Platforms: the platforms hung under a layer the native packet just
 * drew (ndsRendererAdapterIsForeignStageSubtree) draw here, after it, as the
 * source's gcDrawDObjTreeDLLinks reaches them inside the same layer. Each
 * platform DObj goes through the per-DObj path, whose baked roots own the
 * Bonus2Common lists. Not ITCM: one board's handful of platforms. */
static void ndsStageGCDrawAllLoopSubmitForeignSubtrees(GObj *display)
{
    DObj *stack[64];
    u32 stack_count = 0u;
    sb32 began = FALSE;
    DObj *root;

    /* A Board the Platforms board, the source's or a content's (P7 C2). */
    if ((display == NULL) || (gSCManagerBattleState == NULL) ||
        (ndsGrBonusClass(gSCManagerBattleState->gkind) != 2u) ||
        (sNdsStageGCDrawAllLoopHardwareSubmitActive == FALSE))
    {
        return;
    }
    root = DObjGetStruct(display);
    if (root != NULL)
    {
        stack[stack_count++] = root;
    }
    while (stack_count != 0u)
    {
        DObj *dobj = stack[--stack_count];
        sb32 foreign = FALSE;

        if ((dobj->flags & DOBJ_FLAG_HIDDEN) == 0u)
        {
            foreign = ndsRendererAdapterIsForeignStageSubtree(dobj);
        }
        if (foreign != FALSE)
        {
            /* The platform and its descendants, never its siblings. */
            DObj *sub[16];
            u32 sub_count = 0u;

            if (began == FALSE)
            {
                ndsRendererAdapterBeginStageTraversal();
                began = TRUE;
            }
            sub[sub_count++] = dobj;
            while (sub_count != 0u)
            {
                DObj *part = sub[--sub_count];

                if ((part->flags & DOBJ_FLAG_HIDDEN) != 0u)
                {
                    if ((part != dobj) && (part->sib_next != NULL) &&
                        (sub_count < ARRAY_COUNT(sub)))
                    {
                        sub[sub_count++] = part->sib_next;
                    }
                    continue;
                }
                if ((part->dl_link != NULL) &&
                    ((part->flags & DOBJ_FLAG_NOTEXTURE) == 0u))
                {
                    ndsRendererAdapterSubmitStageDObj(
                        part, NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_TREE_DLLINKS,
                        sNdsStageGCDrawAllLoopCurrentCameraGObj,
                        ndsStageGCDrawAllLoopInitialGeometryMode());
                }
                if ((part != dobj) && (part->sib_next != NULL) &&
                    (sub_count < ARRAY_COUNT(sub)))
                {
                    sub[sub_count++] = part->sib_next;
                }
                if ((part->child != NULL) && (sub_count < ARRAY_COUNT(sub)))
                {
                    sub[sub_count++] = part->child;
                }
            }
        }
        if ((dobj->sib_next != NULL) && (stack_count < ARRAY_COUNT(stack)))
        {
            stack[stack_count++] = dobj->sib_next;
        }
        if ((foreign == FALSE) && ((dobj->flags & DOBJ_FLAG_HIDDEN) == 0u) &&
            (dobj->child != NULL) && (stack_count < ARRAY_COUNT(stack)))
        {
            stack[stack_count++] = dobj->child;
        }
    }
    if (began != FALSE)
    {
        ndsRendererAdapterEndStageTraversal();
    }
}
#endif

s32 NDS_R2_ITCM_PACK2_CODE
ndsStageGCDrawAllLoopRecordCapturedDisplay(void *camera_gobj,
                                           void *display_gobj,
                                           s32 link_id)
{
    GObj *display = display_gobj;
    u32 mask;
    sb32 is_layer;

    if ((ndsFighterMarioFoxStageGCDrawAllLoopProofActive() == FALSE) ||
        (gNdsSceneManagerCurrIsBattle == 0u) ||
        ((sNdsFighterGCDrawAllLoopDisplayActive == FALSE) &&
         (sNdsStageGCDrawAllLoopHardwareSubmitActive == FALSE)))
    {
        return FALSE;
    }
    gNdsStageGCDrawAllLoopCapturedDisplayCount++;
    sNdsStageGCDrawAllLoopCurrentCameraGObj = camera_gobj;
    sNdsStageGCDrawAllLoopCurrentDisplayGObj = display;
    sNdsStageGCDrawAllLoopCurrentDisplayLinkID = link_id;
    /* WHERE THE PROC'S OWN RDP STATE STARTS. This hook runs immediately before
     * `current_gobj->proc_display(current_gobj)` (opening_movie_backend.c:4387),
     * so the four heads here bound exactly the span that proc is about to emit.
     * A source effect's colour lives in that span and nowhere else --
     * efManagerShieldProcDisplay writes prim/env into gSYTaskmanDLHeads[1] and
     * then calls gcDrawDObjTreeDLLinksForGObj -- so the effect submit reads it
     * back from here instead of any effect-specific field. */
    ndsRendererAdapterMarkDisplayProcHeads();
    ndsStageGCDrawAllLoopRecordWeaponCapture(display, link_id);
    ndsStageGCDrawAllLoopRecordItemCapture(display, link_id);
    ndsStageGCDrawAllLoopRecordEffectCapture(display, link_id);
    if (ndsStageGCDrawAllLoopClassifyGObj(display, &mask,
                                          &is_layer) != FALSE)
    {
        if (is_layer != FALSE)
        {
            gNdsStageGCDrawAllLoopLayerCaptureMask |= mask;
        }
        else
        {
            gNdsStageGCDrawAllLoopMapCaptureMask |= mask;
        }
    }
    /* P2-2p8 (2026-10-04): the rest of this chain only counts which kind of
     * GObj a non-stage display was -- no code reads those counters -- and it
     * ran five recognisers (two of them DObj-tree walks) for every display
     * GObj of every camera: ~8K cycles a frame in the late-match lab
     * profile. Lab builds keep the census. */
#if defined(NDS_LAB_FOURCPU_SWEEP) && NDS_LAB_FOURCPU_SWEEP
    else if (ndsStageGCDrawAllLoopIsSelectedFighter(display) != FALSE)
    {
        NDS_DIAG(gNdsStageGCDrawAllLoopFighterDisplayCallbackCount++);
    }
#if NDS_P2_STAGE_JUNGLE
    else if (ndsStageGCDrawAllLoopIsTaruCann(display) != FALSE)
    {
        NDS_DIAG(gNdsStageGCDrawAllLoopActorDisplayCallbackCount++);
    }
#endif
#if NDS_P2_STAGE_YOSTER
    else if (ndsStageGCDrawAllLoopIsYosterCloud(display, NULL) != FALSE)
    {
        NDS_DIAG(gNdsStageGCDrawAllLoopYosterCloudDisplayCallbackCount++);
    }
#endif
    else
    {
        NDS_DIAG(gNdsStageGCDrawAllLoopNonStageCaptureCount++);
    }
#endif
    if (sNdsStageGCDrawAllLoopNativeStageArmed != FALSE)
    {
        /* A9: every display GObj of the stage camera passes through here, but
         * only the stage's own segments commit. The tick HUD's STG span is
         * taken inside ndsRendererAdapterCommitNativeStageDisplay, on the
         * matched segment only, instead of two clock reads per display. */
#if NDS_P2_1P_GAME
        {
            s32 handled = ndsRendererAdapterCommitNativeStageDisplay(
                display, link_id);

            if (handled != FALSE)
            {
                ndsStageGCDrawAllLoopSubmitForeignSubtrees(display);
            }
            return handled;
        }
#else
        return ndsRendererAdapterCommitNativeStageDisplay(display, link_id);
#endif
    }
    return FALSE;
}

/* R01-B. THE RESULTS WINNER-SERIES EMBLEM OWNER.
 *
 * `mnVSResultsMakeEmblem` (decomp mnvsresults.c:615) registers the emblem's
 * DObj tree with `gcAddGObjDisplay(gobj, gcDrawDObjTreeForGObj, 33, ...)`.
 * On this target that display callback reaches NO geometry, at three
 * independent points, every one of them a battle gate:
 *
 *   1. `ndsStageGCDrawAllLoopRecordCapturedDisplay` (:14829) returns FALSE on
 *      `gNdsSceneManagerCurrIsBattle == 0`, and `nSCKindVSResults` carries no
 *      NDS_SCENE_FLAG_BATTLE (nds_scene_manager.c:42), so the display is
 *      never claimed and the source proc runs instead.
 *   2. That source proc is the port's `gcDrawDObjTreeForGObj`
 *      (opening_movie_backend.c:1804), whose whole body is two recorders.
 *      The first, `ndsStageGCDrawAllLoopRecordDObjDraw`, hits the SAME
 *      IsBattle gate below (:14954) and only bumps UnexpectedSceneCount.
 *   3. The second, `ndsOpeningRoomRecordDObjDraw`
 *      (opening_movie_backend.c:1521), returns unless the scene is
 *      nSCKindOpeningRoom.
 *
 * So the emblem had no path at all -- not a missing program, a missing
 * route. The Results FIGHTERS draw only because they take a different one:
 * `ftDisplayMainProcDisplay` is port-owned and calls
 * `ndsFighterDisplayContractSubmit` directly, with its own owner bracket and
 * no IsBattle gate (reloc_backend_fighter_display_seam.c:86).
 *
 * WHY AN OWNER AND NOT A WIDER GATE. Opening the IsBattle guard for
 * nSCKindVSResults would admit EVERY Results DObj-tree GObj into the battle
 * stage loop -- the transition camera's tree, the wallpaper, whatever a later
 * row adds -- and then run each one through ClassifyGObj, the weapon/item/
 * effect captures and `ndsRendererAdapterCommitNativeStageDisplay`, all of
 * which are battle-stage machinery keyed to `sNdsStageGCDrawAllLoopNativeStage
 * Armed` and `sNdsStageGCDrawAllLoopHardwareSubmitActive` that this scene
 * never arms. It would also charge that per-GObj work to a scene already
 * outside its cadence budget. This owner claims exactly one GObj, by pointer
 * identity, and costs one compare per captured Results display otherwise.
 *
 * The tree walk mirrors `ndsStageGCDrawAllLoopScanDObjs` (:14688) but reads
 * the camera from the capture site instead of the battle loop's static, which
 * this scene never writes. */
volatile u32 gNdsVSResultsEmblemCaptureCount;
volatile u32 gNdsVSResultsEmblemSubmitCount;
volatile u32 gNdsVSResultsEmblemRejectCount;
volatile u32 gNdsVSResultsEmblemTriangleCount;

extern void *ndsVSResultsEmblemGObj(void);

NDS_COLD sb32 ndsResultsEmblemRecordCapturedDisplay(void *camera_gobj,
                                           void *display_gobj,
                                           s32 link_id)
{
    GObj *display = display_gobj;
    GObj *owned;
    DObj *stack[16];
    u32 stack_count = 0u;
    u32 scanned = 0u;
    u32 triangle_before;
    DObj *root;

    if ((display == NULL) || (camera_gobj == NULL) ||
        (gSCManagerSceneData.scene_curr != nSCKindVSResults) ||
        ((u32)link_id != NDS_NATIVE_VS_EMBLEM_RESULTS_DL_LINK))
    {
        return FALSE;
    }
    /* Pointer identity, published by the port's own wrapper around the
     * source's `mnVSResultsEmblemProcUpdate` (battleship_mnvsresults.c), and
     * cleared at scene entry. id/link alone would be a guess: the source is
     * free to put another GObj on link 33, and a stale pointer across a
     * Results re-entry is exactly the class of bug that crashed this scene
     * once already (see mnVSResultsStartScene's fighter-GObj clear). */
    owned = (GObj *)ndsVSResultsEmblemGObj();
    if ((owned == NULL) || (display != owned) ||
        (display->id != NDS_NATIVE_VS_EMBLEM_RESULTS_GOBJ_ID))
    {
        return FALSE;
    }
    NDS_DIAG(gNdsVSResultsEmblemCaptureCount++);
    root = DObjGetStruct(display);
    if ((root == NULL) || ((root->dv == NULL) && (root->child == NULL)))
    {
        NDS_DIAG(gNdsVSResultsEmblemRejectCount++);
        return FALSE;
    }

    triangle_before = gNdsStageGCDrawAllLoopHardwareTriangleCount;
    ndsRendererAdapterBeginStageTraversal();
    /* The emblem is a foreground menu object on its own camera; it must not
     * be depth-sorted against the Results fighters, which draw on a different
     * link from a different camera. Same handling the ground actors take. */
    sNdsStageGCDrawAllLoopActorKeepsZBuffer = TRUE;
    stack[stack_count++] = root;
    while ((stack_count != 0u) && (scanned < ARRAY_COUNT(stack)))
    {
        DObj *dobj = stack[--stack_count];

        if (dobj == NULL)
        {
            continue;
        }
        scanned++;
        if (dobj->dv != NULL)
        {
            ndsRendererAdapterSubmitStageDObj(
                dobj,
                NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_TREE,
                camera_gobj,
                ndsStageGCDrawAllLoopInitialGeometryMode());
        }
        if ((dobj->sib_next != NULL) && (stack_count < ARRAY_COUNT(stack)))
        {
            stack[stack_count++] = dobj->sib_next;
        }
        if ((dobj->child != NULL) && (stack_count < ARRAY_COUNT(stack)))
        {
            stack[stack_count++] = dobj->child;
        }
    }
    sNdsStageGCDrawAllLoopActorKeepsZBuffer = FALSE;
    ndsRendererAdapterEndStageTraversal();

    gNdsVSResultsEmblemTriangleCount =
        gNdsStageGCDrawAllLoopHardwareTriangleCount - triangle_before;
    if (gNdsVSResultsEmblemTriangleCount == 0u)
    {
        /* A recognised emblem that scanned its whole tree and emitted nothing
         * is a successful empty draw, which the native-only contract forbids.
         * Report it and hand the GObj back rather than claiming a draw that
         * put no pixels on the screen. */
        NDS_DIAG(gNdsVSResultsEmblemRejectCount++);
        ndsRendererRecordNativeFailure(
            NDS_NATIVE_FAILURE_STAGE, (u32)gSCManagerSceneData.scene_curr,
            (u32)display->dl_link_id, 0u, (u32)(uintptr_t)display, 0u,
            NDS_NATIVE_FAILURE_REJECTED_PROGRAM);
        return FALSE;
    }
    NDS_DIAG(gNdsVSResultsEmblemSubmitCount++);
    return TRUE;
}

void ndsStageGCDrawAllLoopRecordDObjDraw(void *gobj, u32 kind)
{
    GObj *stage_gobj = gobj;
    u32 mask;
    sb32 is_layer;
    u32 callback_kind = kind;
#if NDS_TICK_HUD
    u32 owner_start = 0u;
#endif

    if (kind >= 32u)
    {
        kind = 31u;
    }
    if (ndsFighterMarioFoxStageGCDrawAllLoopProofActive() == FALSE)
    {
        return;
    }
    if (gNdsSceneManagerCurrIsBattle == 0u)
    {
        NDS_DIAG(gNdsStageGCDrawAllLoopUnexpectedSceneCount++);
        return;
    }
    if ((sNdsFighterGCDrawAllLoopDisplayActive == FALSE)
        && (sNdsStageGCDrawAllLoopHardwareSubmitActive == FALSE)
    )
    {
        gNdsStageGCDrawAllLoopManualDisplayCallCount++;
        return;
    }
    if (ndsStageGCDrawAllLoopClassifyGObj(stage_gobj, &mask,
                                          &is_layer) == FALSE)
    {
        if (sNdsStageGCDrawAllLoopHardwareSubmitActive != FALSE)
        {
            /* R2-07 MISC split. These two submits are the projectile and
             * effect DObj halves of the MISC bucket: they run in the branch
             * where ClassifyGObj REJECTED the GObj, so they are outside the
             * STG bracket by construction and land in the residual. */
#if NDS_TICK_HUD
            u32 misc_split_mark = NDS_TICK_HUD_SPAN_CLOCK();
#endif
            /* P2-2p8 (2026-10-04): each of the three submits returns at
             * once unless the GObj is its kind (Is*Display tests gobj->id
             * first), so only the matching one is called, and only its span
             * is timed: two main-RAM calls and three clock reads fewer for
             * every DObj-tree GObj outside the stage. */
            switch (stage_gobj->id)
            {
            case nGCCommonKindWeapon:
                ndsStageGCDrawAllLoopSubmitWeaponDObj(stage_gobj,
                                                      callback_kind);
#if NDS_TICK_HUD
                gNdsMiscWeaponDrawTicks += NDS_TICK_HUD_SPAN_CLOCK() - misc_split_mark;
#endif
                break;
            case nGCCommonKindItem:
                ndsStageGCDrawAllLoopSubmitItemDObj(stage_gobj,
                                                    callback_kind);
#if NDS_TICK_HUD
                gNdsMiscItemDrawTicks += NDS_TICK_HUD_SPAN_CLOCK() - misc_split_mark;
#endif
                break;
            case nGCCommonKindEffect:
                ndsStageGCDrawAllLoopSubmitEffectDObj(stage_gobj,
                                                      callback_kind);
#if NDS_TICK_HUD
                gNdsMiscEffectDrawTicks += NDS_TICK_HUD_SPAN_CLOCK() - misc_split_mark;
#endif
                break;
            default:
                break;
            }
#if NDS_TICK_HUD && (NDS_P2_STAGE_JUNGLE || NDS_P2_STAGE_YOSTER || \
    NDS_P2_STAGE_ZEBES || NDS_P2_STAGE_SECTOR || NDS_P2_STAGE_YAMABUKI || \
    NDS_P2_STAGE_INISHIE)
            misc_split_mark = NDS_TICK_HUD_SPAN_CLOCK();
#endif
#if NDS_P2_STAGE_JUNGLE
            /* The barrel is none of weapon/item/effect (ground kind), so the
             * three submits above no-op on it; its actor commit runs here, in
             * the same ClassifyGObj-rejected branch, and lands its triangles
             * in the same hardware counters. */
            ndsStageGCDrawAllLoopSubmitTaruCannDObj(stage_gobj,
                                                    callback_kind);
#endif
#if NDS_P2_STAGE_YOSTER
            ndsStageGCDrawAllLoopSubmitYosterCloudDObj(stage_gobj,
                                                       callback_kind);
#endif
#if NDS_P2_STAGE_ZEBES || NDS_P2_STAGE_SECTOR || \
    NDS_P2_STAGE_YAMABUKI || NDS_P2_STAGE_INISHIE
            /* Ground-kind stage actors (Zebes acid, Sector Arwing, Saffron
             * gate, Inishie scales): none of weapon/item/effect kind, so the
             * three submits above no-op on them. The generic actor commit
             * runs here, in the same ClassifyGObj-rejected branch, and lands
             * its triangles in the same hardware counters. The native
             * TaruCann and cloud arms above are untouched. */
            ndsStageGCDrawAllLoopSubmitGroundActorDObj(stage_gobj,
                                                       callback_kind);
#endif
#if NDS_TICK_HUD && (NDS_P2_STAGE_JUNGLE || NDS_P2_STAGE_YOSTER || \
    NDS_P2_STAGE_ZEBES || NDS_P2_STAGE_SECTOR || NDS_P2_STAGE_YAMABUKI || \
    NDS_P2_STAGE_INISHIE)
            gNdsMiscActorDrawTicks += NDS_TICK_HUD_SPAN_CLOCK() - misc_split_mark;
#endif
        }
        return;
    }
    gNdsStageGCDrawAllLoopDObjDrawCallbackCount++;
    if (sNdsStageGCDrawAllLoopHardwareSubmitActive != FALSE)
    {
#if NDS_TICK_HUD
        owner_start = NDS_TICK_HUD_SPAN_CLOCK();
#endif
        ndsRendererAdapterBeginStageTraversal();
    }
    ndsStageGCDrawAllLoopScanDObjs(stage_gobj, mask, is_layer, kind,
                                   callback_kind);
    if (sNdsStageGCDrawAllLoopHardwareSubmitActive != FALSE)
    {
        ndsRendererAdapterEndStageTraversal();
#if NDS_TICK_HUD
        if (owner_start != 0u)
        {
            u32 owner_ticks = NDS_TICK_HUD_SPAN_CLOCK() - owner_start;

#if NDS_TICK_HUD
            gNdsTickHudStageTicks += owner_ticks;
#endif
        }
#endif
    }
}

static void ndsStageGCDrawAllLoopBeginHardwareFrame(void)
{
    Gfx scratch[2];
    Gfx *head = scratch;
    void *saved_graphics_heap_ptr = gSYTaskmanGraphicsHeap.ptr;

    /* BattleShip sys/rdp.c:112-115 invokes the registered scene-light
     * callback before drawing. DS owns the rest of the frame reset. */
    syRdpResetSettings(&head);
    gSYTaskmanGraphicsHeap.ptr = saved_graphics_heap_ptr;
}


/* THE STAGE'S TEXTURES FIRST (S6, 2026-10-23). A blob stage's images
 * upload at its owner's first prepare, the battle's sixth presented frame,
 * after every scene owner -- the countdown's atlases, the particle sheets,
 * the entry images -- has taken texture VRAM. Hyrule Castle with four
 * contents (the heavy roster) then found no room for its 34th image: every
 * prepare declined until the startup entry images retired two frames into
 * GO, and the castle, whose ground lists are lent to the heap, drew nothing
 * for the whole countdown. ndsBattlePrepareSceneTextures runs the owner's
 * prepare here once, before those owners, so the stage's images are
 * resident first; a late entry image that then finds no room is placed at
 * GO (scVSBattleFuncUpdate), before any of them can draw. A prepare that
 * declines here leaves the first frames' warm-up as it was. */
NDS_COLD void ndsBattleWarmNativeStageTextures(void)
{
    GObj *camera = ndsBattleCompatMainCameraGObj();

    if ((gSCManagerSceneData.scene_curr != nSCKindVSBattle) ||
        (camera == NULL))
    {
        return;
    }
    if (ndsRendererAdapterPrepareNativeStageOwner(camera) != FALSE)
    {
        ndsRendererAdapterFinishNativeStageOwner();
    }
}

static void ndsStageGCDrawAllLoopSubmitHardwareFrame(void)
{

    if ((sNdsStageGCDrawAllLoopHardwareSubmitCount != 0u) ||
        (gSCManagerSceneData.scene_curr != nSCKindVSBattle))
    {
        return;
    }

    ndsStageGCDrawAllLoopBeginHardwareFrame();
    sNdsStageGCDrawAllLoopHardwareSubmitActive = TRUE;
    ndsRendererAdapterResetDepthDiagnostics();
    sNdsStageGCDrawAllLoopNativeStageArmed =
        ndsRendererAdapterPrepareNativeStageOwner(
            ndsBattleCompatMainCameraGObj());
    gcDrawAll();
    if (sNdsStageGCDrawAllLoopNativeStageArmed != FALSE)
    {
        ndsRendererAdapterFinishNativeStageOwner();
        sNdsStageGCDrawAllLoopNativeStageArmed = FALSE;
    }
    ndsFighterDisplayContractSubmitStageFighters();
    sNdsStageGCDrawAllLoopHardwareSubmitActive = FALSE;
}

static void ndsStageGCDrawAllLoopPresentHardwareFrame(void)
{
#if NDS_TICK_HUD
    u32 tickhud_owner_start;
#endif

    if (gNdsSceneManagerCurrIsBattle == 0u)
    {
        return;
    }



    ndsStageGCDrawAllLoopBeginHardwareFrame();
    sNdsStageGCDrawAllLoopHardwareSubmitActive = TRUE;
    ndsRendererAdapterResetDepthDiagnostics();
    ndsRendererHardwareSetNoOracle(TRUE);
#if NDS_TICK_HUD
    tickhud_owner_start = NDS_TICK_HUD_SPAN_CLOCK();
#endif
    sNdsStageGCDrawAllLoopNativeStageArmed =
        ndsRendererAdapterPrepareNativeStageOwner(
            ndsBattleCompatMainCameraGObj());
#if NDS_TICK_HUD
    {
        u32 stage_ticks = NDS_TICK_HUD_SPAN_CLOCK() - tickhud_owner_start;

        gNdsTickHudStageTicks += stage_ticks;
    }
#endif
    gcDrawAll();
    if (sNdsStageGCDrawAllLoopNativeStageArmed != FALSE)
    {
#if NDS_TICK_HUD
        tickhud_owner_start = NDS_TICK_HUD_SPAN_CLOCK();
#endif
        ndsRendererAdapterFinishNativeStageOwner();
#if NDS_TICK_HUD
        {
            u32 stage_ticks = NDS_TICK_HUD_SPAN_CLOCK() - tickhud_owner_start;

            gNdsTickHudStageTicks += stage_ticks;
        }
#endif
        sNdsStageGCDrawAllLoopNativeStageArmed = FALSE;
    }
    ndsFighterDisplayContractSubmitStageFighters();
    ndsRendererHardwareSetNoOracle(FALSE);
    sNdsStageGCDrawAllLoopHardwareSubmitActive = FALSE;
}

void ndsFighterMarioFoxStageGCDrawAllLoopSubmitHardwareFrame(void)
{
    ndsFighterMarioFoxStageGCDrawAllLoopPrepare();
    ndsStageGCDrawAllLoopSubmitHardwareFrame();
}

void ndsFighterMarioFoxStageGCDrawAllLoopPresentHardwareFrame(void)
{
    ndsFighterMarioFoxStageGCDrawAllLoopPrepare();
    ndsStageGCDrawAllLoopPresentHardwareFrame();
}
