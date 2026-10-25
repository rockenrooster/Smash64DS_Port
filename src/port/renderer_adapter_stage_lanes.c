/* The lanes ahead of the general stage DL submit: the entry-effect and P4
 * content owners, the fast lane on the DTCM hot stack, item draw replay,
 * the routed item owners, the rebirth halo's arm and the router that tries
 * them before the body. Included in order by reloc_backend_renderer_dl.c;
 * not a translation unit of its own. */

/* One list's matrices, prepared ahead of its submit (Sector Z's Arwing in two
 * passes, ndsRendererAdapterSubmitArwingTwoPass): exactly what
 * ndsRendererAdapterPrepareInitialMatrices returned for this DObj, before the
 * identity fill. Set only around that list's submit. */
typedef struct NDSRendererAdapterEntryPrepared
{
    DObj *dobj;
    const Gfx *dl;
    u32 list_id;
    u32 prepared;
    u32 projection_valid;
    u32 modelview_valid;
    NDSRendererMatrix20p12 projection;
    NDSRendererMatrix20p12 modelview;
} NDSRendererAdapterEntryPrepared;

static const NDSRendererAdapterEntryPrepared *sNdsRendererAdapterEntryPrepared;

#if defined(NDS_LAB_FOURCPU_SWEEP) && NDS_LAB_FOURCPU_SWEEP
/* LAB ONLY (the any-stage sweep ROM): the FoxSpecial3 Arwing roots this
 * owner draws (Sector Z's hazard, Fox's entry), in the order of the root
 * switch below. Per root: calls, ticks from entry through
 * PrepareInitialMatrices, the executor's ticks, the rest (stats, config,
 * tail), triangles. A call spanning >= 2^20 ticks (the clock's 2^22
 * artifact) is left out. */
volatile u32 gNdsLabArwingRootCensus[8][5] __attribute__((used));
#define NDS_LAB_ARW_MARK(v) ((v) = cpuGetTiming())
#else
#define NDS_LAB_ARW_MARK(v) ((void)0)
#endif

/* The entry owner's inherited state, as the executor's stats see it at
 * submit; shared by ndsRendererAdapterTryNativeEntryEffect and the Arwing's
 * replay submit so the two cannot drift. */
static inline __attribute__((always_inline)) void
ndsRendererAdapterEntryEffectSeedStats(DObj *dobj, NDSRendererStats *stats)
{
    ndsRendererInitStats(stats);
    /* Light state. Both lists inherit G_LIGHTING from the battle display and
     * carry their own gSPLightColor words in the packet; what they do not
     * carry is seeded from what the display left in the RSP: the direction
     * scVSBattleFuncLights aimed, re-aimed by the fighter's own
     * ftDisplayLightsDrawReflect when it uses a light (same seed as
     * ndsRendererAdapterBeginStageTraversal), and for a group before the
     * first colour word the last colours written: the effect's own MObj
     * colours when it carries MOBJ_FLAG_LIGHT1/2, otherwise the fighter
     * material drawn before it. */
    if ((sNdsFighterDisplayCurrentLightValid != FALSE) &&
        (sNdsFighterDisplayCurrentLightCount != 0u))
    {
        stats->light_dir_x = sNdsFighterDisplayCurrentLight.l.dir[0];
        stats->light_dir_y = sNdsFighterDisplayCurrentLight.l.dir[1];
        stats->light_dir_z = sNdsFighterDisplayCurrentLight.l.dir[2];
        stats->light_dir_mask = 1u;
    }
    {
        MObj *mobj;

        for (mobj = dobj->mobj; mobj != NULL; mobj = mobj->next)
        {
            u32 flags = mobj->sub.flags;

            if ((flags & MOBJ_FLAG_LIGHT1) != 0u)
            {
                stats->light_color_1 =
                    ndsRendererAdapterPackColor(&mobj->sub.light1color);
                stats->light_color_mask |=
                    NDS_FIGHTER_DISPLAY_LIGHT_COLOR_1_MASK;
            }
            if ((flags & MOBJ_FLAG_LIGHT2) != 0u)
            {
                stats->light_color_2 =
                    ndsRendererAdapterPackColor(&mobj->sub.light2color);
                stats->light_color_mask |=
                    NDS_FIGHTER_DISPLAY_LIGHT_COLOR_2_MASK;
            }
        }
        if (gNdsFighterDisplayContractMaterialLightSeedCount != 0u)
        {
            if ((stats->light_color_mask &
                 NDS_FIGHTER_DISPLAY_LIGHT_COLOR_1_MASK) == 0u)
            {
                stats->light_color_1 =
                    gNdsFighterDisplayContractMaterialLight1;
                stats->light_color_mask |=
                    NDS_FIGHTER_DISPLAY_LIGHT_COLOR_1_MASK;
            }
            if ((stats->light_color_mask &
                 NDS_FIGHTER_DISPLAY_LIGHT_COLOR_2_MASK) == 0u)
            {
                stats->light_color_2 =
                    gNdsFighterDisplayContractMaterialLight2;
                stats->light_color_mask |=
                    NDS_FIGHTER_DISPLAY_LIGHT_COLOR_2_MASK;
            }
        }
    }
    if (sNdsRendererAdapterEffectSubmitActive != FALSE)
    {
        if ((sNdsRendererAdapterEffectColorMask & 1u) != 0u)
        {
            stats->prim_color = sNdsRendererAdapterEffectPrimColor;
        }
        if ((sNdsRendererAdapterEffectColorMask & 2u) != 0u)
        {
            stats->env_color = sNdsRendererAdapterEffectEnvColor;
        }
        if (sNdsRendererAdapterEffectOtherModeValid != FALSE)
        {
            stats->othermode_l = sNdsRendererAdapterEffectOtherModeL;
        }
        /* A list-1 draw whose proc set no mode inherits the battle camera's
         * own XLU head (gmcamera.c:1055 writes G_RM_AA_ZB_XLU_SURF into
         * gSYTaskmanDLHeads[1] before every layer), not the last effect's. */
        if ((sNdsRendererAdapterEffectOtherModeThisProc == 0u) &&
            (sNdsRendererAdapterEffectSubmitHead == 1u))
        {
            stats->othermode_l = G_RM_AA_ZB_XLU_SURF | G_RM_AA_ZB_XLU_SURF2;
        }
    }
}

static inline __attribute__((always_inline)) void
ndsRendererAdapterEntryEffectAccumulate(const NDSRendererStats *stats)
{
    gNdsStageGCDrawAllLoopHardwareTriangleCount += stats->hardware_triangle_count;
    gNdsStageGCDrawAllLoopHardwareZBufferTriangleCount +=
        stats->hardware_zbuffer_triangle_count;
    gNdsStageGCDrawAllLoopHardwareProjectedDepthTriangleCount +=
        stats->hardware_projected_depth_triangle_count;
    gNdsStageGCDrawAllLoopHardwareDecalDepthTriangleCount +=
        stats->hardware_decal_depth_triangle_count;
    gNdsStageGCDrawAllLoopHardwareTextureBindCount +=
        stats->hardware_texture_bind_count;
    gNdsStageGCDrawAllLoopHardwareTextureUploadCount +=
        stats->hardware_texture_upload_count;
    gNdsStageGCDrawAllLoopHardwareTextureReadyCount +=
        stats->hardware_texture_ready_count;
    gNdsStageGCDrawAllLoopHardwareTextureRejectCount +=
        stats->hardware_texture_reject_count;
}

/* nds_renderer_native_common.c: a P4 content article root's admission
 * (scripts/p4/p4_articles.py rows in the build's entry-effect packet). The
 * variant count carries these bits (the same values there): frame i was
 * baked with palette i; a list with no TEXID frames needs the live material. */
#define NDS_P4_VARIANT_PALETTE_FOLLOWS 0x80u
#define NDS_P4_VARIANT_MATERIAL 0x40u
/* An entry root whose entry is over: its corners are lent out, it draws
 * nothing (ndsRendererEntryP4RetireEntries). */
#define NDS_P4_VARIANT_RETIRED 0x20u
sb32 ndsRendererEntryEffectP4Admit(const void *dl, u32 *owner_asset_id,
                                   u32 *root_offset, u32 *variant_count);

static sb32 ndsRendererAdapterTryNativeEntryEffect(
    DObj *dobj, const Gfx *dl, GObj *camera_gobj, u32 initial_geometry_mode)
{
    extern void *gFTManagerCommonFile;
#if defined(NDS_LAB_FOURCPU_SWEEP) && NDS_LAB_FOURCPU_SWEEP
    u32 lab_t[5] = { 0u, 0u, 0u, 0u, 0u };
#endif
    const u8 *base = NULL;
    u32 owner_asset_id = 0u;
    u32 root_offset = 0u;
    sb32 candidate = FALSE;
    /* Zeroed where it is filled: every stage list asks here first and almost
     * none is an entry prop, so an initialiser was a memset per list (the
     * ending room's DObjs paid it on every draw). */
    NDSRendererConfig config;
    NDSRendererStats stats;
    NDSRendererMatrix20p12 projection;
    NDSRendererMatrix20p12 modelview;
    const NDSRendererMatrix20p12 *projection_ptr;
    const NDSRendererMatrix20p12 *modelview_ptr;
    /* One owner's material snapshot a call: the arms below are exclusive by
     * owner, so their snapshots share storage. As separate locals they made
     * this the DTCM hot stack's deepest frame (2,888 B; the stack 21.9% free
     * at its deepest, P8 S3). */
    union
    {
        NDSRendererNativeMaterial common;
#if NDS_LINK
        NDSRendererNativeMaterial link_special2;
        NDSRendererNativeMaterial link_spin[9];
#endif
        /* EFCommonEffects3 needs at most two live MObj snapshots per root:
         * MBallRays takes two per ray fan, ItemGetSwirl one per drawable
         * child. */
        NDSRendererNativeMaterial efcommon3[2];
    } snapshot;
    /* Every owner passes these to the native prepare; only Link's two
     * material-snapshot arms fill them, so the pair lives outside his flag. */
    const NDSRendererNativeMaterial *native_materials = NULL;
    u32 native_material_count = 0u;
    u32 native_texture_variant = 0xffffffffu;
    u32 p4_variant_count = 0u;

    if ((dobj == NULL) || (dl == NULL))
    {
        return FALSE;
    }
    NDS_LAB_ARW_MARK(lab_t[0]);

    /* A content's root first: its rows answer from a memo by list
     * (ndsRendererEntryEffectP4Admit), and no arm below can claim a list in
     * a content's file. Four Sonics' entry planes drew 60 roots a frame
     * through every arm. */
    if ((dobj->parent_gobj != NULL) &&
        ((dobj->parent_gobj->id == nGCCommonKindEffect) ||
         (dobj->parent_gobj->id == nGCCommonKindWeapon) ||
         (dobj->parent_gobj->id == nGCCommonKindItem)) &&
        (ndsRendererEntryEffectP4Admit(dl, &owner_asset_id, &root_offset,
                                       &p4_variant_count) != FALSE))
    {
        if ((p4_variant_count & NDS_P4_VARIANT_RETIRED) != 0u)
        {
            return TRUE;
        }
        base = (const u8 *)dl - root_offset;
        candidate = TRUE;
    }
    /* Exact source asset + exact generated root is the whole admission test.
     * Do not classify arbitrary effect lists by shape: this path intentionally
     * owns only entry props that the offline source bake emitted. */
    if ((candidate == FALSE) && (gEFManagerFiles[1] != NULL))
    {
        const uintptr_t address = (uintptr_t)dl;
        const uintptr_t effect_base = (uintptr_t)gEFManagerFiles[1];
        if ((address == effect_base + 0x2500u) ||
            (address == effect_base + 0x2588u) ||
            (address == effect_base + 0x2610u) ||
            (address == effect_base + 0x2698u) ||
            (address == effect_base + 0x5218u) ||
            (address == effect_base + 0x52b0u) ||
            (address == effect_base + 0x5310u) ||
            (address == effect_base + 0x31d0u) ||
            (address == effect_base + 0x3258u) ||
            (address == effect_base + 0x32e0u))
        {
            base = (const u8 *)effect_base;
            root_offset = (u32)(address - effect_base);
            owner_asset_id = 84u;
            candidate = TRUE;
        }
    }
    if ((candidate == FALSE) && (gFTManagerCommonFile != NULL) &&
        ((uintptr_t)dl == (uintptr_t)gFTManagerCommonFile + 0x0248u))
    {
        base = (const u8 *)gFTManagerCommonFile;
        root_offset = 0x0248u;
        owner_asset_id = 163u;
        candidate = TRUE;
    }
    if ((candidate == FALSE) && (gFTDataFoxSpecial2 != NULL) &&
        ((uintptr_t)dl == (uintptr_t)gFTDataFoxSpecial2 + 0x01b8u))
    {
        base = (const u8 *)gFTDataFoxSpecial2;
        root_offset = 0x01b8u;
        owner_asset_id = 346u;
        candidate = TRUE;
    }
    /* Yoshi's egg. One source list serves two states that both hide his whole
     * body on purpose: the shield (efmanager.c:490) and the egg-hatching intro
     * (:1375) name the same DObj setup field. The ordinary shield arm above
     * cannot cover him, because ftcommonguard1.c:387-397 and
     * ftcommonguard2.c:20-26 are an if/else on `fkind == nFTKindYoshi` that
     * sends him to efManagerYoshiShieldMakeEffect instead. Guarded because
     * gFTDataYoshiModel lives in ftyoshi.c, which is not built at every roster.
     * The per-frame env fade from fp->shield_health stays runtime-owned. */
    if ((candidate == FALSE) && (gFTDataYoshiModel != NULL) &&
        ((uintptr_t)dl == (uintptr_t)gFTDataYoshiModel + 0xa860u))
    {
        base = (const u8 *)gFTDataYoshiModel;
        root_offset = 0xa860u;
        owner_asset_id = 338u;
        candidate = TRUE;
    }
    if ((candidate == FALSE) && (gFTMarioFileSpecial2 != NULL) &&
        ((const u8 *)dl >= (const u8 *)gFTMarioFileSpecial2))
    {
        base = (const u8 *)gFTMarioFileSpecial2;
        root_offset = (u32)((const u8 *)dl - base);
        if ((root_offset == 0x03c0u) || (root_offset == 0x04c0u))
        {
            owner_asset_id = 356u;
            candidate = TRUE;
        }
    }
#if NDS_LUIGI
    /* dFTLuigiData points its Special2 slot at llMarioSpecial2FileID exactly
     * like Mario, but ftManager owns a separate destination pointer for each
     * fighter kind.  In a Luigi-vs-Fox match gFTMarioFileSpecial2 is therefore
     * legitimately NULL while gFTDataLuigiSpecial2 contains the same source
     * asset.  Admit that second live base explicitly; otherwise Luigi's pipe
     * falls back to the N64 interpreter even though the AOT packet is already
     * an exact bake of file 356. */
    if ((candidate == FALSE) && (gFTDataLuigiSpecial2 != NULL) &&
        ((const u8 *)dl >= (const u8 *)gFTDataLuigiSpecial2))
    {
        base = (const u8 *)gFTDataLuigiSpecial2;
        root_offset = (u32)((const u8 *)dl - base);
        if ((root_offset == 0x03c0u) || (root_offset == 0x04c0u))
        {
            owner_asset_id = 356u;
            candidate = TRUE;
        }
    }
#endif
    /* Not only Fox's entry: Sector Z's own Arwing is a GROUND object drawing
     * the very same FoxSpecial3 list (GRSectorMap pulls file 161 in as an
     * external dependency, with or without Fox in the match), so this owner is
     * its renderer too. See the texture-lifetime note in
     * battleship_scvsbattle.c. */
    if ((candidate == FALSE) && (gFTDataFoxSpecial3 != NULL) &&
        ((const u8 *)dl >= (const u8 *)gFTDataFoxSpecial3))
    {
        base = (const u8 *)gFTDataFoxSpecial3;
        root_offset = (u32)((const u8 *)dl - base);
        switch (root_offset)
        {
        case 0x1fa0u:
        case 0x2920u:
        case 0x29d0u:
        case 0x29f0u:
        case 0x2a20u:
        case 0x2868u:
        case 0x2a50u:
        case 0x2b00u:
            owner_asset_id = 161u;
            candidate = TRUE;
            break;
        default:
            break;
        }
    }
#if NDS_DONKEY
    /* BattleShip dEFManagerDonkeyEntryTaruEffectDesc uses DonkeySpecial2 and
     * gcDrawDObjTreeForGObj. Its DObjDesc at 0x07c8 contains one drawable child
     * whose exact source display-list root is 0x0620. Keep the source tree and
     * animation live; replace only that immutable N64 DL/texture work. */
    if ((candidate == FALSE) && (gFTDataDonkeySpecial2 != NULL) &&
        ((const u8 *)dl >= (const u8 *)gFTDataDonkeySpecial2))
    {
        base = (const u8 *)gFTDataDonkeySpecial2;
        root_offset = (u32)((const u8 *)dl - base);
        if (root_offset == 0x0620u)
        {
            owner_asset_id = 355u;
            candidate = TRUE;
        }
    }
#endif
    /* BattleShip dEFManagerSamusEntryPointEffectDesc owns one animated child
     * whose DObjDLLink submits two immutable source lists (links 0 and 1).
     * Keep the source DObj tree and EntryPoint AnimJoint live -- notably its
     * near-zero -> full -> near-zero Y scale that opens/closes the point -- and
     * replace only those two N64 Gfx streams with their generated DS packets. */
    if ((candidate == FALSE) && (gFTDataSamusSpecial2 != NULL) &&
        ((const u8 *)dl >= (const u8 *)gFTDataSamusSpecial2))
    {
        base = (const u8 *)gFTDataSamusSpecial2;
        root_offset = (u32)((const u8 *)dl - base);
        if ((root_offset == 0x0930u) || (root_offset == 0x0ad0u) ||
            ((root_offset == 0x02e0u) &&
             (sNdsRendererAdapterEffectSubmitActive != FALSE) &&
             (dobj->parent_gobj != NULL) &&
             (dobj->parent_gobj->id == nGCCommonKindEffect) &&
             (dobj->mobj != NULL)))
        {
            owner_asset_id = 349u;
            candidate = TRUE;
        }
    }

    if ((owner_asset_id == 349u) && (root_offset == 0x02e0u))
    {
        MObj *mobj = dobj->mobj;
        s32 texture_id_curr = -1;
        s32 texture_id_next = -1;

        /* Catch's grapple glow owns exactly one ALPHA material. Segment-E
         * supplies only the current source image; MatAnim loops TEXID 0/1.
         * Preserve that live selector while the generated owner supplies the
         * immutable tile/load/combine/geometry and preconverted texture pair. */
        bzero(&snapshot.common, sizeof(snapshot.common));
        if ((mobj == NULL) || (mobj->next != NULL) ||
            (ndsRendererAdapterBuildNativeMaterialSnapshot(
                 mobj, &snapshot.common, FALSE,
                 &texture_id_curr, &texture_id_next) == FALSE) ||
            (snapshot.common.effects !=
                 NDS_RENDERER_NATIVE_MATERIAL_CURRENT_IMAGE) ||
            (texture_id_curr < 0) || (texture_id_curr > 1) ||
            (texture_id_next < 0) || (texture_id_next > 1))
        {
            NDS_DIAG(gNdsEntryEffectNativeFallbackCount++);
            return FALSE;
        }
        native_materials = &snapshot.common;
        native_material_count = 1u;
        native_texture_variant = (u32)texture_id_curr;
    }
    /* Final Cutter's Draw/Trail/Up/Down effects are source-owned DObj trees
     * from KirbySpecial2. None has an MObj or MatAnimJoint; Draw attaches to
     * fighter joint 17, while Trail/Up/Down keep their source AnimJoints. Keep
     * those live attachments/transforms and replace only the exact immutable
     * Gfx/texture roots emitted by their source descriptors. */
    if ((candidate == FALSE) &&
        (sNdsRendererAdapterEffectSubmitActive != FALSE) &&
        (gFTDataKirbySpecial2 != NULL) &&
        (dobj->parent_gobj != NULL) &&
        (dobj->parent_gobj->id == nGCCommonKindEffect) &&
        (dobj->mobj == NULL))
    {
        const uintptr_t address = (uintptr_t)dl;
        const uintptr_t kirby_special2_base = (uintptr_t)gFTDataKirbySpecial2;

        base = (const u8 *)gFTDataKirbySpecial2;
        root_offset = ((address >= kirby_special2_base) &&
                       (address < kirby_special2_base + 0x2960u)) ?
            (u32)(address - kirby_special2_base) : 0xffffffffu;
        switch (root_offset)
        {
        case 0x27a0u: /* Draw */
        case 0x0c70u: /* Trail */
        case 0x0ce0u:
        case 0x11b0u: /* Up */
        case 0x1218u:
        case 0x1280u:
        case 0x2210u: /* Down */
        case 0x2270u:
        case 0x22d0u:
        case 0x2330u:
        case 0x1cf8u: /* EntryStar: the warp star of Kirby's intro */
            owner_asset_id = 348u;
            candidate = TRUE;
            break;
        default:
            break;
        }
    }

    /* Final Cutter's travelling weapon is a different source owner from the
     * KirbySpecial2 effects above. BattleShip dWPKirbyCutterWeaponDesc points
     * at KirbyMain+0x08 WPAttributes, whose relocated DObjDesc lives in
     * KirbyModel at 0x1D388 and submits these two immutable lists in order.
     * Keep weapon physics/collision and the live source DObj tree; specialize
     * only this exact nWPKindCutter geometry. */
    if ((candidate == FALSE) &&
        (dobj->parent_gobj != NULL) &&
        (dobj->parent_gobj->id == nGCCommonKindWeapon) &&
        (dobj->mobj == NULL))
    {
        WPStruct *cutter_wp = wpGetStruct(dobj->parent_gobj);
        NDSRelocLoadedFile *cutter_model = NULL;

        if ((cutter_wp != NULL) && (cutter_wp->kind == nWPKindCutter))
        {
            /* Compact battle fighters intentionally do not publish the raw
             * KirbyModel through gFTDataKirbyModel. The source weapon does not
             * need that slot: KirbyMain+0x08's relocated WPAttributes already
             * point its live DObjDesc into the loaded asset-328 closure. Follow
             * that authoritative relocated pointer instead of requiring an
             * unrelated global publication. */
            cutter_model = ndsRelocFindLoadedFileContaining(dl, sizeof(*dl));
            if ((cutter_model != NULL) &&
                (cutter_model->asset_id == 328u) &&
                (cutter_model->data != NULL))
            {
                root_offset = ndsRelocNativeRootOffset(cutter_model, dl);
                if ((root_offset == 0x1d238u) || (root_offset == 0x1d308u))
                {
                    base = (const u8 *)cutter_model->data;
                    owner_asset_id = 328u;
                    candidate = TRUE;
                }
            }
        }
    }
#if NDS_LINK
    /* BattleShip's Link entry wave/beam and attached grounded Spin EFFECT each
     * own a live animated DObj in LinkSpecial2. The collision weapon is a
     * different owner (LinkModel below); do not conflate its MatAnim payload
     * with weapon geometry again. */
    if ((candidate == FALSE) && (gFTDataLinkSpecial2 != NULL) &&
        ((const u8 *)dl >= (const u8 *)gFTDataLinkSpecial2))
    {
        base = (const u8 *)gFTDataLinkSpecial2;
        root_offset = (u32)((const u8 *)dl - base);
        if ((root_offset == 0x02d8u) || (root_offset == 0x0698u) ||
            (root_offset == 0x1100u))
        {
            owner_asset_id = 353u;
            candidate = TRUE;
        }
    }
    /* Grounded Spin's source WPAttributes live in LinkMain but resolve their
     * DObj/MObj/Anim/MatAnim pointers into LinkModel. The drawable child's
     * DObjDLLink at +0x118f8 submits exactly LinkModel+0x11680. Its segment-E
     * calls select the nine live MObjs built by gcDrawMObjForDObj. */
    if ((candidate == FALSE) &&
        (dobj->parent_gobj != NULL) &&
        (dobj->parent_gobj->id == nGCCommonKindWeapon))
    {
        WPStruct *spin_wp = wpGetStruct(dobj->parent_gobj);
        NDSRelocLoadedFile *spin_model = NULL;

        if ((spin_wp != NULL) && (spin_wp->kind == nWPKindSpinAttack))
        {
            /* Compact battle packs publish the WPAttributes closure, not the
             * raw gFTDataLinkModel slot. Follow the weapon's relocated source
             * root; the native material owner still requires all nine MObjs. */
            spin_model = ndsRelocFindLoadedFileContaining(dl, sizeof(*dl));
            if ((spin_model != NULL) && (spin_model->asset_id == 324u) &&
                (spin_model->data != NULL))
            {
                root_offset = ndsRelocNativeRootOffset(spin_model, dl);
                if (root_offset == 0x11680u)
                {
                    base = (const u8 *)spin_model->data;
                    owner_asset_id = 324u;
                    candidate = TRUE;
                }
            }
        }
    }
    /* Boomerang's source DObj tree and six-tick rotation loop stay live. Its
     * two drawable children submit these exact LinkSpecial3 wrapper roots.
     * Compact battle packing intentionally does not require the raw
     * gFTDataLinkSpecial3 publication, so follow the live weapon's relocated
     * DL back to its authoritative loaded file just like Final Cutter above. */
    if ((candidate == FALSE) &&
        (dobj->parent_gobj != NULL) &&
        (dobj->parent_gobj->id == nGCCommonKindWeapon))
    {
        WPStruct *boomerang_wp = wpGetStruct(dobj->parent_gobj);
        NDSRelocLoadedFile *boomerang_file = NULL;

        if ((boomerang_wp != NULL) && (boomerang_wp->kind == nWPKindBoomerang))
        {
            boomerang_file = ndsRelocFindLoadedFileContaining(dl, sizeof(*dl));
            if ((boomerang_file != NULL) &&
                (boomerang_file->asset_id == 325u) &&
                (boomerang_file->data != NULL))
            {
                root_offset = ndsRelocNativeRootOffset(boomerang_file, dl);
                if ((root_offset == 0x0458u) || (root_offset == 0x0580u))
                {
                    base = (const u8 *)boomerang_file->data;
                    owner_asset_id = 325u;
                    candidate = TRUE;
                }
            }
        }
    }
#endif
#if NDS_CAPTAIN
    /* Falcon Kick/Punch are ordinary source EFDesc GObjs, not entry-car
     * geometry. Keep their live attachment/animation/MObj state and admit only
     * the exact immutable source roots generated for this package. */
    if ((candidate == FALSE) &&
        (sNdsRendererAdapterEffectSubmitActive != FALSE) &&
        (gFTDataCaptainSpecial2 != NULL) &&
        (dobj->parent_gobj != NULL) &&
        (dobj->parent_gobj->id == nGCCommonKindEffect) &&
        (dobj->mobj != NULL) &&
        ((const u8 *)dl >= (const u8 *)gFTDataCaptainSpecial2))
    {
        base = (const u8 *)gFTDataCaptainSpecial2;
        root_offset = (u32)((const u8 *)dl - base);
        if (root_offset == 0x0a30u)
        {
            owner_asset_id = 350u;
            candidate = TRUE;
        }
    }
    if ((candidate == FALSE) &&
        (sNdsRendererAdapterEffectSubmitActive != FALSE) &&
        (gFTDataCaptainSpecial3 != NULL) &&
        (dobj->parent_gobj != NULL) &&
        (dobj->parent_gobj->id == nGCCommonKindEffect) &&
        (dobj->mobj != NULL) &&
        ((const u8 *)dl >= (const u8 *)gFTDataCaptainSpecial3))
    {
        base = (const u8 *)gFTDataCaptainSpecial3;
        root_offset = (u32)((const u8 *)dl - base);
        if (root_offset == 0x0760u)
        {
            owner_asset_id = 333u;
            candidate = TRUE;
        }
    }

    /* BattleShip dEFManagerCaptainEntryCarEffectDesc owns a live 13-node DObj
     * tree. Its 0x6200 main AnimJoint plus 0x6518/0x6598 child animations remain
     * source-owned; only the ten immutable CaptainSpecial2 Gfx roots below are
     * replaced by the exact generated DS packets. */
    if ((candidate == FALSE) && (gFTDataCaptainSpecial2 != NULL) &&
        ((const u8 *)dl >= (const u8 *)gFTDataCaptainSpecial2))
    {
        base = (const u8 *)gFTDataCaptainSpecial2;
        root_offset = (u32)((const u8 *)dl - base);
        switch (root_offset)
        {
        case 0x5690u:
        case 0x5c60u:
        case 0x5d20u:
        case 0x5d50u:
        case 0x5d80u:
        case 0x5db0u:
        case 0x5de0u:
        case 0x5e10u:
        case 0x5e40u:
        case 0x5e70u:
            owner_asset_id = 350u;
            candidate = TRUE;
            break;
        default:
            break;
        }
    }
#endif
    /* Poke Ball entry rays.  dEFManagerMBallRaysEffectDesc's 5-entry DObjDesc
     * at EFCommonEffects3+0x0628 keeps its live DObj tree, AnimJoint and
     * MatAnimJoint; only its two immutable Gfx roots are replaced.  Exact
     * source asset plus exact generated root is the whole admission test -- a
     * whole-image referrer census over all 2,132 O2R files found exactly one
     * pointer reaching each of these two roots and none from any other file,
     * so no live-kind discriminator is needed.  ftcommonentry.c:99 spawns this
     * from ftCommonAppearUpdateEffects for Pikachu and Jigglypuff, i.e. match
     * entry and every respawn, which is why it is live with items off. */
    if ((candidate == FALSE) && (gEFManagerFiles[2] != NULL) &&
        ((const u8 *)dl >= (const u8 *)gEFManagerFiles[2]))
    {
        base = (const u8 *)gEFManagerFiles[2];
        root_offset = (u32)((const u8 *)dl - base);
        if ((root_offset == 0x0440u) || (root_offset == 0x0518u) ||
            (root_offset == 0x2ef0u) || (root_offset == 0x2f80u) ||
            (root_offset == 0x3010u) || (root_offset == 0x30a0u))
        {
            owner_asset_id = 85u;
            candidate = TRUE;
            if ((root_offset == 0x0440u) || (root_offset == 0x0518u))
            {
                NDS_DIAG(gNdsMBallRaysCandidateCount++);
            }
        }
    }
    if (candidate == FALSE)
    {
        return FALSE;
    }

#if NDS_CAPTAIN
    if (((owner_asset_id == 350u) && (root_offset == 0x0a30u)) ||
        ((owner_asset_id == 333u) && (root_offset == 0x0760u)))
    {
        MObj *mobj = dobj->mobj;
        s32 texture_id_curr = -1;
        s32 texture_id_next = -1;
        s32 texture_id_max = (owner_asset_id == 350u) ? 1 : 2;
        const u32 expected_effects =
            NDS_RENDERER_NATIVE_MATERIAL_CURRENT_IMAGE |
            NDS_RENDERER_NATIVE_MATERIAL_RENDER_TILE_SIZE |
            NDS_RENDERER_NATIVE_MATERIAL_TEXTURE;

        /* Source MObj flags are 0x00A1 (ALPHA | 0x20 | TEXTURE). MatAnim
         * changes only TEXID; the root DL owns the effective combine/blend/TLUT
         * state. Snapshot without advancing IDs and select the matching AOT
         * CI4 frame so effective source alpha survives end to end. */
        bzero(&snapshot.common, sizeof(snapshot.common));
        if ((mobj == NULL) || (mobj->next != NULL) ||
            (ndsRendererAdapterBuildNativeMaterialSnapshot(
                 mobj, &snapshot.common, FALSE,
                 &texture_id_curr, &texture_id_next) == FALSE) ||
            (snapshot.common.effects != expected_effects) ||
            (texture_id_curr < 0) || (texture_id_curr > texture_id_max) ||
            (texture_id_next < 0) || (texture_id_next > texture_id_max))
        {
            NDS_DIAG(gNdsEntryEffectNativeFallbackCount++);
            return FALSE;
        }
        native_materials = &snapshot.common;
        native_material_count = 1u;
        native_texture_variant = (u32)texture_id_curr;
    }
#endif
    if ((p4_variant_count & NDS_P4_VARIANT_MATERIAL) != 0u)
    {
        /* A P4 list with no TEXID frames whose TLUT comes from its MObj's
         * segment-E branch (the eggs, the grape, the needle): the generator
         * baked palette 0, which is all their makers select; the draw takes
         * the live material. Marth's, Roy's and Banjo's entry wave and beam
         * take their colours there, as Link's do (owner 353 below). Anything
         * else declines, counted. */
        MObj *mobj = dobj->mobj;

        bzero(&snapshot.common, sizeof(snapshot.common));
        if ((mobj == NULL) || (mobj->next != NULL) ||
            (((mobj->sub.flags & MOBJ_FLAG_PALETTE) != 0u) && (mobj->palette_id != 0.0F)) ||
            (ndsRendererAdapterBuildNativeMaterialSnapshot(
                 mobj, &snapshot.common, FALSE, NULL, NULL) == FALSE))
        {
            NDS_DIAG(gNdsEntryEffectNativeFallbackCount++);
            return FALSE;
        }
        native_materials = &snapshot.common;
        native_material_count = 1u;
    }
    else if (p4_variant_count != 0u)
    {
        /* A P4 TEXID root (Falcon Punch's shape): one live MObj whose
         * MatAnim switches only TEXID among the preconverted frames; with
         * MOBJ_FLAG_PALETTE the generator baked palette 0, which is all the
         * makers ever select, or palette i into frame i where the article
         * moves palette_id with the TEXID (Sonic's spring). Anything else
         * declines, counted. */
        MObj *mobj = dobj->mobj;
        s32 texture_id_curr = -1;
        s32 texture_id_next = -1;
        sb32 palette_follows =
            ((p4_variant_count & NDS_P4_VARIANT_PALETTE_FOLLOWS) != 0u) ? TRUE : FALSE;

        p4_variant_count &= ~NDS_P4_VARIANT_PALETTE_FOLLOWS;
        bzero(&snapshot.common, sizeof(snapshot.common));
        if ((mobj == NULL) || (mobj->next != NULL) ||
            (ndsRendererAdapterBuildNativeMaterialSnapshot(
                 mobj, &snapshot.common, FALSE,
                 &texture_id_curr, &texture_id_next) == FALSE) ||
            (texture_id_curr < 0) ||
            ((u32)texture_id_curr >= p4_variant_count) ||
            (((mobj->sub.flags & MOBJ_FLAG_PALETTE) != 0u) &&
             (mobj->palette_id !=
              ((palette_follows != FALSE) ? (f32)texture_id_curr : 0.0F))))
        {
            NDS_DIAG(gNdsEntryEffectNativeFallbackCount++);
            return FALSE;
        }
        native_materials = &snapshot.common;
        native_material_count = 1u;
        native_texture_variant = (u32)texture_id_curr;
    }

#if NDS_LINK
    if (owner_asset_id == 353u)
    {
        MObj *mobj = dobj->mobj;

        /* LinkSpecial2's entry Wave, entry Beam, and attached grounded Spin
         * effect each select segment 0xE slot 0 from exactly one live MObj.
         * Keep the source MatAnim live and translate that one typed material
         * state instead of freezing its animated PRIM/light values in AOT. */
        if ((mobj == NULL) || (mobj->next != NULL) ||
            (ndsRendererAdapterBuildNativeMaterialSnapshot(
                 mobj, &snapshot.link_special2, FALSE, NULL, NULL) == FALSE))
        {
            NDS_DIAG(gNdsEntryEffectNativeFallbackCount++);
            return FALSE;
        }
        native_materials = &snapshot.link_special2;
        native_material_count = 1u;
    }
    else if (owner_asset_id == 324u)
    {
        MObj *mobj = dobj->mobj;
        u32 i;

        /* Source LinkModel has exactly nine MObjs for this DObj. Snapshot the
         * live values without advancing texture ids: every source MObj is
         * PRIM-only, and the native owner validates that invariant before GX. */
        for (i = 0u; i < 9u; i++)
        {
            if ((mobj == NULL) ||
                (ndsRendererAdapterBuildNativeMaterialSnapshot(
                     mobj, &snapshot.link_spin[i], FALSE, NULL, NULL) == FALSE))
            {
                NDS_DIAG(gNdsEntryEffectNativeFallbackCount++);
                return FALSE;
            }
            mobj = mobj->next;
        }
        if (mobj != NULL)
        {
            NDS_DIAG(gNdsEntryEffectNativeFallbackCount++);
            return FALSE;
        }
        native_materials = snapshot.link_spin;
        native_material_count = 9u;
    }
#endif

    if (owner_asset_id == 85u)
    {
        MObj *mobj = dobj->mobj;
        u32 expected_material_count;
        u32 i;
        sb32 is_mballrays =
            ((root_offset == 0x0440u) || (root_offset == 0x0518u));

        if (is_mballrays != FALSE)
        {
            expected_material_count = 2u;
        }
        else if ((root_offset == 0x2ef0u) || (root_offset == 0x2f80u) ||
                 (root_offset == 0x3010u) || (root_offset == 0x30a0u))
        {
            expected_material_count = 1u;
        }
        else
        {
            NDS_DIAG(gNdsEntryEffectNativeFallbackCount++);
            return FALSE;
        }

        /* MBallRays owns two PRIM-only MObjs per ray fan and its list calls
         * segment 0xE slot 1 then slot 0; ItemGetSwirl owns ONE per drawable
         * child.  Snapshot only that live state, without advancing texture
         * ids, and let the native owner re-validate the closed contract before
         * it touches GX. */
        for (i = 0u; i < expected_material_count; i++)
        {
            if ((mobj == NULL) ||
                (ndsRendererAdapterBuildNativeMaterialSnapshot(
                     mobj, &snapshot.efcommon3[i], FALSE, NULL, NULL) ==
                 FALSE) ||
                (snapshot.efcommon3[i].effects !=
                     NDS_RENDERER_NATIVE_MATERIAL_PRIM))
            {
                /* No fallback and NO second record: return FALSE and let the
                 * single loud NO_PROGRAM guard publish this one event. */
                if (is_mballrays != FALSE)
                {
                    NDS_DIAG(gNdsMBallRaysMaterialRejectCount++);
                }
                NDS_DIAG(gNdsEntryEffectNativeFallbackCount++);
                return FALSE;
            }
            mobj = mobj->next;
        }
        if (mobj != NULL)
        {
            if (is_mballrays != FALSE)
            {
                NDS_DIAG(gNdsMBallRaysMaterialRejectCount++);
            }
            NDS_DIAG(gNdsEntryEffectNativeFallbackCount++);
            return FALSE;
        }
        native_materials = snapshot.efcommon3;
        native_material_count = expected_material_count;
    }

    if (owner_asset_id == 84u)
    {
        u32 expected_effects = NDS_RENDERER_NATIVE_MATERIAL_PRIM;
        if ((root_offset == 0x5218u) || (root_offset == 0x5310u))
        {
            expected_effects |= NDS_RENDERER_NATIVE_MATERIAL_ENV;
        }
        if ((root_offset == 0x31d0u) || (root_offset == 0x3258u) || (root_offset == 0x32e0u))
        {
            expected_effects |= NDS_RENDERER_NATIVE_MATERIAL_LIGHT1;
        }
        /* These source roots select segment-E material slot zero. Keep that
         * material's animated fields live; unselected slots cannot affect it. */
        bzero(&snapshot.common, sizeof(snapshot.common));
        if ((dobj->mobj == NULL) ||
            (ndsRendererAdapterBuildNativeMaterialSnapshot(
                 dobj->mobj, &snapshot.common, FALSE, NULL, NULL) == FALSE) ||
            (snapshot.common.effects != expected_effects))
        {
            NDS_DIAG(gNdsEntryEffectNativeFallbackCount++);
            /* Material rejection status packs source flags in the low half
             * and prepared effects in the high half; all values come from
             * the CPU, not a stale debugger read of the task stack. */
            ndsRendererRecordNativeFailure(NDS_NATIVE_FAILURE_STAGE,
                (u32)gSCManagerSceneData.scene_curr,
                (((dobj->parent_gobj != NULL) ? dobj->parent_gobj->id : 0xffffu) << 16) | 84u,
                ((snapshot.common.effects & 0xffffu) << 16) |
                    ((dobj->mobj != NULL) ? dobj->mobj->sub.flags : 0xffffu),
                root_offset, (u32)(uintptr_t)dobj->mobj, NDS_NATIVE_FAILURE_BAD_ASSET);
            return FALSE;
        }
        native_materials = &snapshot.common;
        native_material_count = 1u;
    }
    if ((sNdsRendererAdapterEntryPrepared != NULL) &&
        (sNdsRendererAdapterEntryPrepared->dobj == dobj) &&
        (sNdsRendererAdapterEntryPrepared->dl == dl))
    {
        /* Prepared by the Arwing's first pass with this same call; the
         * identity fill below still applies. */
        projection_ptr = (sNdsRendererAdapterEntryPrepared->projection_valid != 0u) ?
            &sNdsRendererAdapterEntryPrepared->projection : NULL;
        modelview_ptr = (sNdsRendererAdapterEntryPrepared->modelview_valid != 0u) ?
            &sNdsRendererAdapterEntryPrepared->modelview : NULL;
    }
    else
    {
        ndsRendererAdapterPrepareInitialMatrices(
            dobj,
            (camera_gobj != NULL) ? CObjGetStruct(camera_gobj) :
                ((gGCCurrentCamera != NULL) ? CObjGetStruct(gGCCurrentCamera) : NULL),
            FALSE, &projection, &projection_ptr, &modelview, &modelview_ptr);
    }
    NDS_LAB_ARW_MARK(lab_t[1]);
    /* The default battle camera has one legitimate split shape where the
     * complete camera transform lives on only one side of the DS pair (the
     * world-quad bridge handles the same contract above).  The generic DL
     * interpreter tolerates that because its matrix stream starts from
     * identity; this fixed owner has no stream to do the implicit fill for it.
     * Make that identity explicit so Fox's six small Arwing glow lists stay on
     * the native path instead of falling back solely because their capture pass
     * supplies one camera half. */
    if ((projection_ptr == NULL) && (modelview_ptr != NULL))
    {
        ndsRendererAdapterMtxIdentity20p12(&projection);
        projection_ptr = &projection;
    }
    if ((modelview_ptr == NULL) && (projection_ptr != NULL))
    {
        ndsRendererAdapterMtxIdentity20p12(&modelview);
        modelview_ptr = &modelview;
    }
    if ((projection_ptr == NULL) || (modelview_ptr == NULL))
    {
        NDS_DIAG(gNdsEntryEffectNativeFallbackCount++);
        ndsStageRejectNativeRender(dobj, dl,
            NDS_NATIVE_FAILURE_REJECTED_PROGRAM, NULL);
        return FALSE;
    }

    ndsRendererAdapterEntryEffectSeedStats(dobj, &stats);
    memset(&config, 0, sizeof(config));
    config.max_depth = 4u;
    config.max_commands = 1u;
    config.max_list_commands = 1u;
    config.initial_projection = projection_ptr;
    config.initial_modelview = modelview_ptr;
    config.initial_geometry_mode = initial_geometry_mode;
    config.texture_data_layout = NDS_RENDERER_TEXTURE_DATA_O2R_WORD_SWAPPED;

    NDS_LAB_ARW_MARK(lab_t[2]);
    if (ndsRendererSubmitNativeEntryEffect(
            owner_asset_id, root_offset, native_materials,
            native_material_count, native_texture_variant,
            &config, &stats) == FALSE)
    {
        NDS_DIAG(gNdsEntryEffectNativeFallbackCount++);
        ndsStageRejectNativeRender(dobj, dl,
            NDS_NATIVE_FAILURE_REJECTED_PROGRAM, &stats);
        return FALSE;
    }
    NDS_LAB_ARW_MARK(lab_t[3]);

    ndsRendererAdapterEntryEffectAccumulate(&stats);
#if defined(NDS_LAB_FOURCPU_SWEEP) && NDS_LAB_FOURCPU_SWEEP
    NDS_LAB_ARW_MARK(lab_t[4]);
    if ((owner_asset_id == 161u) && ((lab_t[4] - lab_t[0]) < 0x100000u))
    {
        static const u16 lab_roots[8] = { 0x1fa0u, 0x2920u, 0x29d0u, 0x29f0u,
                                          0x2a20u, 0x2868u, 0x2a50u, 0x2b00u };
        u32 r;

        for (r = 0u; r < 8u; r++)
        {
            if (root_offset == lab_roots[r])
            {
                NDS_DIAG(gNdsLabArwingRootCensus[r][0]++);
                NDS_DIAG(gNdsLabArwingRootCensus[r][1] += lab_t[1] - lab_t[0]);
                NDS_DIAG(gNdsLabArwingRootCensus[r][2] += lab_t[3] - lab_t[2]);
                gNdsLabArwingRootCensus[r][3] +=
                    (lab_t[2] - lab_t[1]) + (lab_t[4] - lab_t[3]);
                NDS_DIAG(gNdsLabArwingRootCensus[r][4] += stats.hardware_triangle_count);
                break;
            }
        }
    }
#endif
    return TRUE;
}

/* A P4 content's model part drawn beside its body (Wolf's gun): its
 * generated root at the hold joint's matrices, with the fighter display's
 * reset geometry mode (the list sets its own lighting). A refusal is
 * counted as every native failure is. */
static sb32 ndsRendererAdapterSubmitP4SidecarRoot(
    DObj *joint, u32 owner_asset_id, u32 root_offset,
    const NDSRendererMatrix20p12 *projection,
    const NDSRendererMatrix20p12 *modelview)
{
    NDSRendererConfig config;
    NDSRendererStats stats;

    if (joint == NULL)
    {
        return FALSE;
    }
    ndsRendererAdapterEntryEffectSeedStats(joint, &stats);
    memset(&config, 0, sizeof(config));
    config.max_depth = 4u;
    config.max_commands = 1u;
    config.max_list_commands = 1u;
    config.initial_projection = projection;
    config.initial_modelview = modelview;
    config.initial_geometry_mode = NDS_RENDERER_GEOM_RESET_MODE;
    config.texture_data_layout = NDS_RENDERER_TEXTURE_DATA_O2R_WORD_SWAPPED;
    if (ndsRendererSubmitNativeEntryEffect(
            owner_asset_id, root_offset, NULL, 0u, 0xffffffffu,
            &config, &stats) == FALSE)
    {
        NDS_DIAG(gNdsEntryEffectNativeFallbackCount++);
        ndsRendererRecordNativeFailure(NDS_NATIVE_FAILURE_STAGE,
            (u32)gSCManagerSceneData.scene_curr, owner_asset_id, 0u,
            root_offset, 0u, NDS_NATIVE_FAILURE_REJECTED_PROGRAM);
        return FALSE;
    }
    ndsRendererAdapterEntryEffectAccumulate(&stats);
    return TRUE;
}

static sb32 ndsRendererAdapterPikachuThunder(NDSRelocLoadedFile *loaded,
    DObj *dobj, const Gfx *dl, const NDSRendererConfig *config, NDSRendererStats *stats)
{
    NDSRendererNativeMaterial material;
    NDSRendererConfig local = *config;
    NDSRendererMatrix20p12 identity;
    const void *palette = NULL, *image = NULL;
    u32 offset = ndsRelocNativeRootOffset(loaded, dl), root, role;
    if (dobj->parent_gobj == NULL) return FALSE;
    if (loaded->asset_id == NDS_NATIVE_PIKACHU_THUNDER_MODEL && offset == NDS_NATIVE_PIKACHU_THUNDER_ROOT)
    {
        root = 0u;
        if (dobj->parent_gobj->id == nGCCommonKindWeapon)
        {
            WPStruct *wp = wpGetStruct(dobj->parent_gobj);
            if (wp == NULL) return FALSE;
            if (wp->kind == nWPKindThunderHead) role = 1u;
            else if (wp->kind == nWPKindThunderTrail) role = 2u;
            else return FALSE;
        }
        else if (dobj->parent_gobj->id == nGCCommonKindEffect) role = 4u;
        else return FALSE;
    }
    else if (loaded->asset_id == NDS_NATIVE_PIKACHU_THUNDER_SPECIAL &&
             dobj->parent_gobj->id == nGCCommonKindEffect)
    {
        if (offset == NDS_NATIVE_PIKACHU_SHOCK_ROOT0) root = 1u;
        else if (offset == NDS_NATIVE_PIKACHU_SHOCK_ROOT1) root = 2u;
        else return FALSE;
        role = 8u;
    }
    else return FALSE;
    if (root == 1u)
    {
        if (dobj->mobj != NULL || loaded->data_size < 0x13a0u) return FALSE;
#if (NDS_1P_GAME || NDS_MENU_SHELL) || NDS_COMPACT_BATTLE_FIGHTERS
        palette = ndsRelocNativeAssetAddress(loaded->data, NDS_NATIVE_PIKACHU_SHOCK_PALETTE);
        image = ndsRelocNativeAssetAddress(loaded->data, NDS_NATIVE_PIKACHU_SHOCK_IMAGE);
#else
        palette = (u8 *)loaded->data + NDS_NATIVE_PIKACHU_SHOCK_PALETTE;
        image = (u8 *)loaded->data + NDS_NATIVE_PIKACHU_SHOCK_IMAGE;
#endif
    }
    else if (dobj->mobj == NULL || dobj->mobj->next != NULL ||
             !ndsRendererAdapterBuildNativeMaterialSnapshot(dobj->mobj, &material, FALSE, NULL, NULL)) return FALSE;
    ndsRendererAdapterMtxIdentity20p12(&identity);
    if (local.initial_projection == NULL) local.initial_projection = &identity;
    if (local.initial_modelview == NULL) local.initial_modelview = &identity;
    return ndsRendererSubmitNativePikachuThunder(root, role,
        (root == 1u) ? NULL : &material, palette, image, &local, stats);
}

static sb32 __attribute__((noinline)) ndsRendererAdapterSubmitDamageFlyMDust(
    DObj *dobj, const NDSRelocLoadedFile *loaded,
    const NDSRendererConfig *config, NDSRendererStats *stats)
{
    NDSRendererNativeMaterial material;
    const NDSRendererNativeMaterial *material_ptr = NULL;
    NDSRendererConfig local = *config;
    NDSRendererMatrix20p12 identity;

    /* Both source makers share this animated DObjDLLink. Preserve its current
     * image and tile state without a synthetic segment-E command stream. */
    if ((dobj->parent_gobj != NULL) &&
        (dobj->parent_gobj->id == nGCCommonKindEffect) &&
        (dobj->mobj != NULL) && (dobj->mobj->next == NULL) &&
        (ndsRendererAdapterBuildNativeMaterialSnapshot(
             dobj->mobj, &material, FALSE, NULL, NULL) != FALSE))
    {
        material_ptr = &material;
    }
    ndsRendererAdapterMtxIdentity20p12(&identity);
    if ((local.initial_projection == NULL) &&
        (local.initial_modelview != NULL)) { local.initial_projection = &identity; }
    else if ((local.initial_modelview == NULL) &&
             (local.initial_projection != NULL)) { local.initial_modelview = &identity; }
    return ndsRendererSubmitNativeDamageFlyMDust(
        loaded->data, loaded->data_size, NDS_NATIVE_DAMAGE_FLY_MDUST_ROOT,
        material_ptr, &local, stats);
}

#if defined(NDS_LAB_FOURCPU_SWEEP) && NDS_LAB_FOURCPU_SWEEP
/* LAB ONLY (the any-stage sweep ROM): which GObjs reach the generic stage DL
 * submit and what they cost. Key = GObj id | (item/weapon kind << 16) for the
 * item and weapon links (ITStruct/WPStruct kind, both at +0xC), 0xFFFF for
 * everything else. Rows: key, calls, own ticks, GX-drain wait ticks, then
 * own ticks split at the Impl's phase marks: find, seed, material, matrix,
 * config/stats, exec, tail (a call that returns before a mark leaves the
 * rest in [2] only), then PrepareInitialMatrices' camera, world, compose,
 * recalc and its uncached-fallback count (gNdsLabPimAcc). */
#define NDS_LAB_STAGE_DL_CENSUS_ROWS 24u
volatile u32 gNdsLabStageDLCensus[NDS_LAB_STAGE_DL_CENSUS_ROWS][16]
    __attribute__((used));
/* Samples whose own or wait span read >= 2^20 ticks (the clock's 2^22
 * artifact), left out of the rows. */
volatile u32 gNdsLabStageDLCensusOutliers __attribute__((used));
static u32 sNdsLabStageDLMark[6];
/* 1 = drain the geometry engine before each submit (the wait split). */
volatile u32 gNdsLabStageDLDrain __attribute__((used, section(".data"))) = 0u;
#define NDS_LAB_SDL_MARK(i) (sNdsLabStageDLMark[(i)] = cpuGetTiming())
static void ndsRendererAdapterSubmitStageDLImpl(DObj *dobj, const Gfx *dl,
                                                 GObj *camera_gobj,
                                                 u32 initial_geometry_mode);
static void ndsRendererAdapterSubmitStageDL(DObj *dobj, const Gfx *dl,
                                             GObj *camera_gobj,
                                             u32 initial_geometry_mode)
{
    u32 start = cpuGetTiming();
    u32 idle;
    u32 spins = 0u;
    GObj *owner = (dobj != NULL) ? dobj->parent_gobj : NULL;
    u32 id = (owner != NULL) ? (u32)owner->id : 0u;
    u32 kind = 0xffffu;
    u32 key;
    u32 own;
    u32 i;

    /* Drain whatever the geometry engine still holds, so [3] is the
     * wait for earlier work and [2] this submit's own cost. */
    while ((gNdsLabStageDLDrain != 0u) &&
           (((*(volatile u32 *)0x04000600u) & (1u << 27)) != 0u) && /* GXSTAT */
           (spins < 0x40000u)) { spins++; }
    sNdsLabStageDLMark[0] = sNdsLabStageDLMark[1] = 0u;
    sNdsLabStageDLMark[2] = sNdsLabStageDLMark[3] = 0u;
    sNdsLabStageDLMark[4] = sNdsLabStageDLMark[5] = 0u;
    for (i = 0u; i < 5u; i++)
    {
        gNdsLabPimAcc[i] = 0u;
    }
    idle = cpuGetTiming();
    ndsRendererAdapterSubmitStageDLImpl(dobj, dl, camera_gobj,
                                        initial_geometry_mode);
    own = cpuGetTiming() - idle;
    if ((own >= 0x100000u) || ((idle - start) >= 0x100000u))
    {
        NDS_DIAG(gNdsLabStageDLCensusOutliers++);
        return;
    }
    if ((owner != NULL) && ((id == 1012u) || (id == 1013u)) &&
        (owner->user_data.p != NULL))
    {
        kind = (u32)((const s32 *)owner->user_data.p)[3] & 0xffffu;
    }
    else if ((owner != NULL) && (id == 1011u))
    {
        /* Effects have no kind word: key them by the asset whose list this
         * is (0xfffe = in no loaded file). */
        const NDSRelocLoadedFile *effect_file =
            ndsRelocFindLoadedFileContaining(dl, sizeof(*dl));

        kind = (effect_file != NULL) ? (effect_file->asset_id & 0xffffu) :
                                       0xfffeu;
    }
    key = (id & 0xffffu) | (kind << 16);
    for (i = 0u; i < NDS_LAB_STAGE_DL_CENSUS_ROWS; i++)
    {
        if ((gNdsLabStageDLCensus[i][0] == key) ||
            (gNdsLabStageDLCensus[i][1] == 0u))
        {
            gNdsLabStageDLCensus[i][0] = key;
            gNdsLabStageDLCensus[i][1]++;
            gNdsLabStageDLCensus[i][2] += own;
            gNdsLabStageDLCensus[i][3] += idle - start;
            if ((sNdsLabStageDLMark[0] != 0u) && (sNdsLabStageDLMark[1] != 0u) &&
                (sNdsLabStageDLMark[2] != 0u) && (sNdsLabStageDLMark[3] != 0u) &&
                (sNdsLabStageDLMark[4] != 0u) && (sNdsLabStageDLMark[5] != 0u))
            {
                gNdsLabStageDLCensus[i][4] += sNdsLabStageDLMark[0] - idle;
                gNdsLabStageDLCensus[i][5] += sNdsLabStageDLMark[4] - sNdsLabStageDLMark[0];
                gNdsLabStageDLCensus[i][6] += sNdsLabStageDLMark[5] - sNdsLabStageDLMark[4];
                gNdsLabStageDLCensus[i][7] += sNdsLabStageDLMark[1] - sNdsLabStageDLMark[5];
                gNdsLabStageDLCensus[i][8] += sNdsLabStageDLMark[2] - sNdsLabStageDLMark[1];
                gNdsLabStageDLCensus[i][9] += sNdsLabStageDLMark[3] - sNdsLabStageDLMark[2];
                gNdsLabStageDLCensus[i][10] += (idle + own) - sNdsLabStageDLMark[3];
            }
            gNdsLabStageDLCensus[i][11] += gNdsLabPimAcc[0];
            gNdsLabStageDLCensus[i][12] += gNdsLabPimAcc[1];
            gNdsLabStageDLCensus[i][13] += gNdsLabPimAcc[2];
            gNdsLabStageDLCensus[i][14] += gNdsLabPimAcc[3];
            gNdsLabStageDLCensus[i][15] += gNdsLabPimAcc[4];
            break;
        }
    }
}
#else
#define ndsRendererAdapterSubmitStageDLImpl ndsRendererAdapterSubmitStageDL
#define NDS_LAB_SDL_MARK(i) ((void)0)
#endif

/* P2-2p8 (2026-09-29): entry models first, ahead of the general submit.
 * The general submit is ~29 KB with a prologue that initialises dozens of
 * per-owner locals, and it paid all of that before its first statement handed
 * an entry model to its generated owner -- eight times a frame for Sector Z's
 * Arwing. The admission now runs in this small frame and the general body is
 * entered only for what it declines; a candidate the owner refuses records
 * its failure there exactly as before. */

static void __attribute__((noinline)) ndsRendererAdapterSubmitStageDLBody(
    DObj *dobj, const Gfx *dl, GObj *camera_gobj, u32 initial_geometry_mode);

/* P2-2p8 (2026-09-29): the fast lane for owners the general body already
 * served. A Samus Charge Shot or a Beam Sword root cost ~42K / ~29K ticks a
 * draw on Dream Land (lab census), a quarter of it the body finding its
 * owner: the entry-model scan, the loaded-file lookup and ~60 candidate
 * tests, a 4 KB frame and 20 KB of Thumb code, every draw. The body now
 * records the route when that owner draws a list, and a later draw of the
 * same list goes straight to the owner with the same inputs: the same
 * PrepareInitialMatrices, config, persistent stats and item colours, the
 * same stats tail. A route holds only immutable admission facts (the loaded
 * file, its asset and data, the root); everything live -- the GObj kind, the
 * item kind, a NULL MObj, the submit context -- is tested again each draw,
 * and anything else falls to the body as before. Owners with an MObj or a
 * segment-E material are not routed. */
volatile u32 gNdsStageDLFastLaneHits;
volatile u32 gNdsStageDLFastLaneFills;

#define NDS_SDL_ROUTE_NONE 0u
#define NDS_SDL_ROUTE_CHARGE_SHOT 1u
/* Not stored: decided per draw from the effect-tree owner's latch. */
#define NDS_SDL_ROUTE_VISUAL 0xffu
/* 2 + an index into sNdsStageDLItemRoutes: the MObj-less item owners, whose
 * admission is the same everywhere -- the item submit, an Item GObj of the
 * owner's kind, no MObj -- and whose call is one of two shapes. */
#define NDS_SDL_ROUTE_ITEM 2u
/* A baked root (nds_native_item_baked.exec.inc) under a ground-display GObj:
 * Board the Platforms' platforms, ~30 lists a frame, which each paid the
 * whole body -- its owner probes, the entry-effect probe and the baked-root
 * search -- for one table lookup. The route keeps the root handle (in `root`)
 * and its material slot count (in `pad`); the MObj materials are live, so they
 * are snapshotted again on every draw, as the body does. */
#define NDS_SDL_ROUTE_BAKED 0xfeu
/* P2-6 (2026-10-01): a baked root under an Item GObj (the Race's Bob-ombs
 * and placed Bumpers): every draw went through the body (the Race: 8 body
 * submits a frame, none routed). Same inputs as the body's baked branch --
 * the item submit's head colours over the reset persistent stats -- and the
 * same off-screen exit as NDS_SDL_ROUTE_BAKED. */
#define NDS_SDL_ROUTE_BAKED_ITEM 0xfdu
/* P2-6 (2026-10-02): the GBumper's quad (the Race places them along the
 * course; Peach's Castle drops one): the body's castle bumper owner with the
 * body's admission -- an Item GObj of kind GBumper under the item submit, one
 * MObj of the owner's flags whose snapshot selects a palette image -- tested
 * again on every draw, and the snapshot taken live, as the body takes it. */
#define NDS_SDL_ROUTE_CASTLE_BUMPER 0xfcu
/* P2-6 (2026-10-02): the ending's room (owner: "End scene in the room with
 * the desk is really slow"; 3-4 VBlanks a frame). Its six GObjs (kind 0) draw
 * their DObj lists through the stage traversal, and every list paid the
 * fast-lane miss, the entry-effect probe and the whole body for one lookup in
 * the room table the ending loads. The route is NDS_SDL_ROUTE_BAKED's, owned
 * by any GObj whose root lies in that table (re-tested per draw: the table
 * leaves with the ending's heap), with the same persistent stats and
 * off-screen exit the body's baked branch reaches. */
#define NDS_SDL_ROUTE_BAKED_ROOM 0xfbu
/* P2-2p8 (2026-10-05): the Fire Flower's live root (0x4608): one MObj whose
 * snapshot selects a palette image, as the body's candidate admits it --
 * an Item GObj of kind FFlower under the item submit, the owner's MObj flags
 * -- tested again on every draw, the snapshot taken live. Its branch root
 * (0x4520, no MObj) is an ordinary item route. Each draw paid the body: two
 * lists a frame for as long as a flower is out (Jungle's sweep match, frame
 * 820 on, ~29K cycles a frame). */
#define NDS_SDL_ROUTE_FFLOWER_LIVE 0xfau
/* P2-2p8 (2026-10-05): the N Bumper's quad (ITCommonObject 0x7558, the list
 * the castle bumper draws, under an item of kind NBumper): the body's
 * admission -- the item submit, one MObj of the owner's flags, a
 * palette-image snapshot taken live -- tested again on every draw. Sector Z's
 * sweep match drew it through the body on 238 frames. */
#define NDS_SDL_ROUTE_NBUMPER 0xf9u
/* P2-2p8 (2026-10-05), the next owners a body-submit census found drawn
 * through the body every frame they live (clean sweep ROM, frames
 * 100-1,900): Sector Z's Arwing laser (226 frames; a weapon, no MObj, its
 * texture in a second file the list's relocated words name -- re-proved each
 * draw), Saffron's Hitokage and Fushigibana (one CURRENT_IMAGE material
 * snapshot, taken live), and the damage-fly dust (an effect; its MObj
 * snapshot taken live by the body's own helper, under the effect layer's
 * seeds and witnesses). Each admission is the body's, tested again on every
 * draw. Saffron's MObj-less Marumine, GLucky and Porygon are item routes. */
#define NDS_SDL_ROUTE_SECTOR_LASER 0xf8u
#define NDS_SDL_ROUTE_HITOKAGE 0xf7u
#define NDS_SDL_ROUTE_FUSHIGIBANA 0xf6u
#define NDS_SDL_ROUTE_DAMAGE_FLY_MDUST 0xf5u
/* Mushroom Kingdom's two Pakkun (file 155 root 0x0B40: one MObj of flags
 * 0x0001 whose CURRENT_IMAGE lies in the same file, the palette in file 107)
 * and its POW block (0x10D0: no MObj, the TLUT in file 107 and both images
 * in file 155, every word proved against those files): items the body drew
 * on every frame of the sweep match (3,600 and 805 body submits). */
#define NDS_SDL_ROUTE_INISHIE_PAKKUN 0xf4u
#define NDS_SDL_ROUTE_INISHIE_POWBLOCK 0xf3u
/* The star quad four owners share (ITCommonObject 0x5458: the Star Rod's two
 * weapons and Kirby's two stars, an effect): no MObj, admitted by asset and
 * root under any owner, as the body admits it; under the effect layer it
 * takes the layer's seeds and witnesses first. */
#define NDS_SDL_ROUTE_KIRBYSTAR 0xf2u
/* P2-2p8 (2026-10-05), owner playtest "Pikachu Down B / Ness Up B cause P95
 * slowdown": Thunder's bolt (its head and trail weapons, the trail effect and
 * the shock roots) and PK Thunder's head and trail weapons drew through the
 * body on every frame they live, a dozen lists a frame with four Pikachus.
 * Admission is the body's owner's own, tested again on every draw: Thunder's
 * adapter re-proves the GObj kind, the weapon kind, the root and its MObj
 * snapshot; PK Thunder's arm re-proves the weapon kind against the recorded
 * root, the trail id and the snapshot's effects. An owner that declines sends
 * the list back to the body, which decides as before. */
#define NDS_SDL_ROUTE_PIKACHU_THUNDER 0xf1u
#define NDS_SDL_ROUTE_NESS_PKTHUNDER 0xf0u
/* P2-2p8 (2026-10-07): a baked root under a Weapon GObj -- the Poke Ball
 * monsters' weapons (Meowth's coins, Onix's rocks, Blastoise's water,
 * Beedrill's swarm) and Master Hand's bullets, which draw their DObj trees
 * through ndsWPDisplayDObjTree. The body's baked branch drew them and recorded
 * no route, so every coin of a Pay Day paid the whole body (a Yoshi's Island
 * sweep match: ~4.5 body submits a frame for 150 frames). The ground display's
 * inputs: no item or effect seeds over the reset persistent stats. */
#define NDS_SDL_ROUTE_BAKED_WEAPON 0xefu
/* P2-2p8 (2026-10-07), a body-submit census of the failing stages' sweep
 * matches (clean ROM, frames 100-1,900): the Poke Ball (Yoshi's Island, 998
 * body submits), the Red and Green Shells (Mushroom Kingdom 354, Yoshi's
 * Island 93) and the Starman (Jungle 170) drew through the body on every
 * frame they live. Each admission is the body's own, tested again on every
 * draw: the item submit, an Item GObj of the owner's kind, the MObj count and
 * the live snapshots' effects. The two shells share one root, so the route
 * re-reads the kind and takes that shell's owner. */
#define NDS_SDL_ROUTE_MBALL 0xeeu
#define NDS_SDL_ROUTE_SHELL 0xedu
#define NDS_SDL_ROUTE_STAR 0xecu
/* The seed-3 census: the Bob-omb (Sector Z 151, Yoshi's Island 47; one
 * CURRENT_IMAGE MObj). */
#define NDS_SDL_ROUTE_BOMBHEI 0xebu
/* Yoshi's Island's capsules and boxes thrashed an 8-slot table (1,231 fills
 * for 1,469 hits a match): the owners are few, but a capsule alone draws three
 * roots, and its header and third root shared a slot under an address-bit
 * index. A multiplicative hash over 64 slots. */
#define NDS_SDL_ROUTES 64u
#define NDS_SDL_ROUTE_SHIFT 26u

typedef sb32 (*NDSStageDLItemSubmit)(const void *base, u32 bytes,
                                     const NDSRendererConfig *config,
                                     NDSRendererStats *stats);
typedef sb32 (*NDSStageDLItemSubmitRooted)(u32 root, const void *base,
                                           u32 bytes,
                                           const NDSRendererConfig *config,
                                           NDSRendererStats *stats);
typedef struct NDSStageDLItemRoute
{
    NDSStageDLItemSubmit submit;
    NDSStageDLItemSubmitRooted submit_rooted;
    volatile u32 *draws;
    volatile u32 *fails;
    u32 item_kind;
} NDSStageDLItemRoute;

enum
{
    nNDSStageDLItemSword,
    nNDSStageDLItemBat,
    nNDSStageDLItemCapsule,
    nNDSStageDLItemStarRod,
    nNDSStageDLItemMSBomb,
    nNDSStageDLItemBox,
    nNDSStageDLItemTaru,
    nNDSStageDLItemEgg,
    nNDSStageDLItemIwark,
    nNDSStageDLItemHammer,
    nNDSStageDLItemLGun,
    nNDSStageDLItemHarisen,
    nNDSStageDLItemHeart,
    nNDSStageDLItemFFlower,
    nNDSStageDLItemTomato,
#if NDS_STAGE_YAMABUKI
    nNDSStageDLItemMarumine,
    nNDSStageDLItemGLucky,
    nNDSStageDLItemPorygon,
#endif
    nNDSStageDLItemRouteCount
};

/* The Fire Flower's branch root: no material. */
static sb32 ndsStageDLSubmitFFlowerBranch(u32 root, const void *base,
                                          u32 bytes,
                                          const NDSRendererConfig *config,
                                          NDSRendererStats *stats)
{
    return ndsRendererSubmitNativeItemFFlower(root, base, bytes, NULL, config,
                                              stats);
}

static const NDSStageDLItemRoute sNdsStageDLItemRoutes[nNDSStageDLItemRouteCount] =
{
    [nNDSStageDLItemSword] = { NULL, ndsRendererSubmitNativeItemSword,
        &gNdsItemSwordDrawCount, &gNdsItemSwordSubmitFailCount, nITKindSword },
    [nNDSStageDLItemBat] = { NULL, ndsRendererSubmitNativeItemBat,
        &gNdsItemBatDrawCount, &gNdsItemBatSubmitFailCount, nITKindBat },
    [nNDSStageDLItemCapsule] = { NULL, ndsRendererSubmitNativeItemCapsule,
        &gNdsItemCapsuleDrawCount, &gNdsItemCapsuleSubmitFailCount,
        nITKindCapsule },
    [nNDSStageDLItemStarRod] = { NULL, ndsRendererSubmitNativeItemStarRod,
        &gNdsItemStarRodDrawCount, &gNdsItemStarRodSubmitFailCount,
        nITKindStarRod },
    [nNDSStageDLItemMSBomb] = { NULL, ndsRendererSubmitNativeItemMSBomb,
        &gNdsItemMSBombDrawCount, &gNdsItemMSBombSubmitFailCount,
        nITKindMSBomb },
    [nNDSStageDLItemBox] = { ndsRendererSubmitNativeItemBox, NULL,
        &gNdsItemBoxDrawCount, &gNdsItemBoxSubmitFailCount, nITKindBox },
    [nNDSStageDLItemTaru] = { ndsRendererSubmitNativeItemTaru, NULL,
        &gNdsItemTaruDrawCount, &gNdsItemTaruSubmitFailCount, nITKindTaru },
    [nNDSStageDLItemEgg] = { ndsRendererSubmitNativeItemEgg, NULL,
        &gNdsItemEggDrawCount, &gNdsItemEggSubmitFailCount, nITKindEgg },
    [nNDSStageDLItemIwark] = { ndsRendererSubmitNativeItemIwark, NULL,
        &gNdsItemIwarkDrawCount, &gNdsItemIwarkSubmitFailCount,
        nITKindIwark },
    [nNDSStageDLItemHammer] = { ndsRendererSubmitNativeItemHammer, NULL,
        &gNdsItemHammerDrawCount, &gNdsItemHammerSubmitFailCount,
        nITKindHammer },
    [nNDSStageDLItemLGun] = { ndsRendererSubmitNativeItemLGun, NULL,
        &gNdsItemLGunDrawCount, &gNdsItemLGunSubmitFailCount, nITKindLGun },
    [nNDSStageDLItemHarisen] = { ndsRendererSubmitNativeItemHarisen, NULL,
        &gNdsItemHarisenDrawCount, &gNdsItemHarisenSubmitFailCount,
        nITKindHarisen },
    [nNDSStageDLItemHeart] = { ndsRendererSubmitNativeItemHeart, NULL,
        &gNdsItemHeartDrawCount, &gNdsItemHeartSubmitFailCount,
        nITKindHeart },
    [nNDSStageDLItemFFlower] = { NULL, ndsStageDLSubmitFFlowerBranch,
        &gNdsItemFFlowerDrawCount, &gNdsItemFFlowerSubmitFailCount,
        nITKindFFlower },
    /* 2026-10-07: Jungle's seed-2 sweep match drew the Maxim Tomato through
     * the body 276 times (no MObj; the body's list proofs ran once, at the
     * draw that recorded the route). */
    [nNDSStageDLItemTomato] = { ndsRendererSubmitNativeItemTomato, NULL,
        &gNdsItemTomatoDrawCount, &gNdsItemTomatoSubmitFailCount,
        nITKindTomato },
#if NDS_STAGE_YAMABUKI
    [nNDSStageDLItemMarumine] = { ndsRendererSubmitNativeYamabukiMarumine,
        NULL, &gNdsYamabukiMarumineDrawCount,
        &gNdsYamabukiMarumineSubmitFailCount, nITKindMarumine },
    [nNDSStageDLItemGLucky] = { ndsRendererSubmitNativeItemGLucky, NULL,
        &gNdsYamabukiGluckyDrawCount, &gNdsYamabukiGluckySubmitFailCount,
        nITKindGLucky },
    [nNDSStageDLItemPorygon] = { ndsRendererSubmitNativeItemPorygon, NULL,
        &gNdsYamabukiPorygonDrawCount, &gNdsYamabukiPorygonSubmitFailCount,
        nITKindPorygon },
#endif
};

typedef struct NDSStageDLRoute
{
    const Gfx *dl;
    NDSRelocLoadedFile *loaded;
    const void *data;
    u32 data_size;
    u32 root;
    u16 asset_id;
    u8 route;
    u8 pad;
} NDSStageDLRoute;

static NDSStageDLRoute sNdsStageDLRoutes[NDS_SDL_ROUTES];

/* P2-6 (2026-10-02): two ways, the hashed slot and its pair. The Race's
 * bumper quad and a Bob-omb list shared a slot and evicted each other on
 * every draw (1,021 fills for 1,021 body submits in 600 frames). Returns the
 * slot holding `dl`, else the hashed one. */
static inline NDSStageDLRoute *ndsStageDLRouteSlot(const Gfx *dl)
{
    const u32 index = ((u32)(uintptr_t)dl * 2654435761u) >>
        NDS_SDL_ROUTE_SHIFT;
    NDSStageDLRoute *slot = &sNdsStageDLRoutes[index];

    if (slot->dl != dl)
    {
        NDSStageDLRoute *pair = &sNdsStageDLRoutes[index ^ 1u];

        if (pair->dl == dl)
        {
            return pair;
        }
    }
    return slot;
}

static void ndsStageDLRouteRecord(const Gfx *dl, NDSRelocLoadedFile *loaded,
                                  u32 root, u32 route)
{
    NDSStageDLRoute *slot = ndsStageDLRouteSlot(dl);

    if ((loaded == NULL) || (loaded->data == NULL))
    {
        return;
    }
    if ((slot->dl != dl) && (slot->route != NDS_SDL_ROUTE_NONE))
    {
        /* The hashed slot holds another list: take its pair. */
        slot = &sNdsStageDLRoutes[(u32)(slot - sNdsStageDLRoutes) ^ 1u];
    }
    if ((slot->dl != dl) || (slot->route != route))
    {
        NDS_DIAG(gNdsStageDLFastLaneFills++);
    }
    slot->dl = dl;
    slot->loaded = loaded;
    slot->data = loaded->data;
    slot->data_size = loaded->data_size;
    slot->root = root;
    slot->asset_id = (u16)loaded->asset_id;
    slot->route = (u8)route;
}

#include <nds/nds_item_replay.h>

/* P2-2p8 (2026-10-04): ITEM DRAW REPLAY (include/nds/nds_item_replay.h).
 *
 * The gate's late match draws only Beam Swords, one lying and one held, and
 * each root paid ~15K ticks: the stage traversal reset, the fast lane's
 * config, stats reset and seeds, the executor's generated N64 state setup and
 * the per-vertex words -- for an 11-triangle blade and a 2-triangle hilt
 * whose GX output never changes while the inputs below do not. An item draw
 * whose every list is a replayable owner records once (its emits, through
 * ndsNativeItemWave1Emit's sink) and later draws replay: the same tree walk,
 * the same matrix preparation per list (the held item's latch walk included:
 * its FTParts writes are gameplay), the recorded binds, batch states and
 * corners. The persistent stats the skipped owners would have written are
 * dead: the next traversal resets them before any reader.
 *
 * The key is held verbatim, not hashed: the item's root and kind, the
 * initial geometry mode, the traversal's light, and per list its DObj, list,
 * head, route and the head's captured colours and blend modes. A recorded
 * texture must still be resident, else the draw runs its owners and records
 * again. */
#define NDS_ITEM_REPLAY_DRAWS 2u
#define NDS_ITEM_REPLAY_ROOTS 4u
#define NDS_ITEM_REPLAY_KEY_WORDS 56u

typedef struct NDSItemReplayRoot
{
    DObj *dobj;
    const Gfx *dl;
    u8 head;
    u8 emit_first;
    u8 emit_count;
    u8 pad;
} NDSItemReplayRoot;

typedef struct NDSItemReplayDraw
{
    const DObj *root;
    u32 last_used;
    u8 valid;
    u8 root_count;
    u8 key_count;
    u8 emit_count;
    NDSItemReplayRoot roots[NDS_ITEM_REPLAY_ROOTS];
    u32 key[NDS_ITEM_REPLAY_KEY_WORDS];
    NDSItemReplayEmit emits[NDS_ITEM_REPLAY_EMITS];
    u32 pool[NDS_ITEM_REPLAY_POOL_WORDS];
} NDSItemReplayDraw;

__attribute__((used)) volatile u32 gNdsItemReplayDraws;
__attribute__((used)) volatile u32 gNdsItemReplayRecords;
__attribute__((used)) volatile u32 gNdsItemReplayRecordFailed;
__attribute__((used)) volatile u32 gNdsItemReplayNotResident;

static NDSItemReplayDraw sNdsItemReplayDraws[NDS_ITEM_REPLAY_DRAWS];
static NDSItemReplaySink sNdsItemReplaySinkState;
#if defined(NDS_LAB_FOURCPU_SWEEP) && NDS_LAB_FOURCPU_SWEEP
/* LAB: 1 = every draw that would replay runs its owners into a scratch
 * recording instead, and counts a recording that differs from the cached one
 * in any emit field or vertex word. */
volatile u32 gNdsItemReplayVerify __attribute__((used, section(".data"))) = 0u;
__attribute__((used)) volatile u32 gNdsItemReplayVerifyRuns;
__attribute__((used)) volatile u32 gNdsItemReplayVerifyFail;
static NDSItemReplayDraw sNdsItemReplayVerifyDraw;

static sb32 ndsItemReplaySameRecording(const NDSItemReplayDraw *a,
                                       const NDSItemReplayDraw *b)
{
    u32 i;

    if ((a->valid != 1u) || (a->emit_count != b->emit_count) ||
        (a->root_count != b->root_count))
    {
        return FALSE;
    }
    for (i = 0u; i < a->root_count; i++)
    {
        if ((a->roots[i].emit_first != b->roots[i].emit_first) ||
            (a->roots[i].emit_count != b->roots[i].emit_count))
        {
            return FALSE;
        }
    }
    for (i = 0u; i < a->emit_count; i++)
    {
        const NDSItemReplayEmit *x = &a->emits[i];
        const NDSItemReplayEmit *y = &b->emits[i];

        if ((x->indices != y->indices) || (x->drawn != y->drawn) ||
            (x->use_texture != y->use_texture) ||
            (x->vertex_count != y->vertex_count) ||
            (x->corner_count != y->corner_count) ||
            (x->triangle_count != y->triangle_count))
        {
            return FALSE;
        }
        if ((x->use_texture != 0u) &&
            ((x->tex_slot != y->tex_slot) || (x->tex_name != y->tex_name) ||
             (x->tex_generation != y->tex_generation) ||
             (x->tex_format != y->tex_format) ||
             (x->tex_width != y->tex_width) ||
             (x->tex_height != y->tex_height)))
        {
            return FALSE;
        }
        if ((x->drawn != 0u) &&
            ((x->poly_fmt != y->poly_fmt) ||
             (x->othermode_l != y->othermode_l) ||
             (x->blend_color != y->blend_color) ||
             (memcmp(&a->pool[x->word_first], &b->pool[y->word_first],
                     (u32)x->vertex_count * 4u * sizeof(u32)) != 0)))
        {
            return FALSE;
        }
    }
    return TRUE;
}
#endif
/* The draw being recorded and the lists it has seen, NULL outside one; and
 * whether one of its lists drew without a sink emit (the recording is then
 * dropped, not kept). */
static NDSItemReplayDraw *sNdsItemReplayRecording;
static u32 sNdsItemReplayRecordRoots;
static u32 sNdsItemReplayRecordEmpty;

/* Owners whose GX output is their ndsNativeItemWave1Emit calls and nothing
 * else: the MObj-less item routes (fixed geometry, generated setup, no
 * materials). An owner the sink cannot hold (too many emits or vertices)
 * fails its recording and keeps drawing itself.
 *
 * NOT Saffron's three gate Pokemon (owner, 2026-10-06: "Regression, pokemon
 * hazards are not visible"). Marumine, GLucky and Porygon draw through their
 * own traversal and never call the sink, so since 2026-10-04 (3911899aa71,
 * every item route admitted) their first draw recorded zero emits and every
 * later draw replayed nothing: two owner calls a monster, then invisible
 * (walk-1006e, Chansey and Electrode). */
static inline sb32 ndsItemReplayRouteOk(u32 route_kind)
{
    if ((route_kind < NDS_SDL_ROUTE_ITEM) ||
        (route_kind >= (NDS_SDL_ROUTE_ITEM + nNDSStageDLItemRouteCount)))
    {
        return FALSE;
    }
    /* The Maxim Tomato's owner draws its quad with glVertex itself, never
     * through the sink: a recording would hold zero emits. */
    if (route_kind == (NDS_SDL_ROUTE_ITEM + nNDSStageDLItemTomato))
    {
        return FALSE;
    }
#if NDS_STAGE_YAMABUKI
    if ((route_kind == (NDS_SDL_ROUTE_ITEM + nNDSStageDLItemMarumine)) ||
        (route_kind == (NDS_SDL_ROUTE_ITEM + nNDSStageDLItemGLucky)) ||
        (route_kind == (NDS_SDL_ROUTE_ITEM + nNDSStageDLItemPorygon)))
    {
        return FALSE;
    }
#endif
    return TRUE;
}

/* The fast lane's item branch, around its owner call: the list must be the
 * next one the recording's walk named, through a replayable owner. */
static sb32 ndsItemReplayRootBegin(const DObj *dobj, const Gfx *dl,
                                   u32 route_kind, u32 *emit_first)
{
    NDSItemReplayDraw *draw = sNdsItemReplayRecording;
    const u32 index = sNdsItemReplayRecordRoots;

    if (draw == NULL)
    {
        return FALSE;
    }
    if ((ndsItemReplayRouteOk(route_kind) == FALSE) ||
        (index >= draw->root_count) || (draw->roots[index].dobj != dobj) ||
        (draw->roots[index].dl != dl))
    {
        sNdsItemReplaySinkState.failed = 1u;
        return FALSE;
    }
    *emit_first = sNdsItemReplaySinkState.emit_count;
    return TRUE;
}

static void ndsItemReplayRootEnd(sb32 handled, u32 emit_first)
{
    NDSItemReplayDraw *draw = sNdsItemReplayRecording;
    const u32 index = sNdsItemReplayRecordRoots;

    if (draw == NULL)
    {
        return;
    }
    if (handled == FALSE)
    {
        sNdsItemReplaySinkState.failed = 1u;
    }
    else if (sNdsItemReplaySinkState.emit_count == emit_first)
    {
        /* Drawn without one sink emit: a baked root culled outside the view
         * (or an owner that never calls the sink). Kept, the recording would
         * replay this list as nothing even once it is back in view, so this
         * frame's recording is dropped and the next draw records again. */
        sNdsItemReplayRecordEmpty = 1u;
    }
    draw->roots[index].emit_first = (u8)emit_first;
    draw->roots[index].emit_count =
        (u8)(sNdsItemReplaySinkState.emit_count - emit_first);
    sNdsItemReplayRecordRoots = index + 1u;
}

/* The body's item-submit seeds over the reset persistent stats, with its
 * witnesses: the head's prim/env colours and blend modes, each only where the
 * head captured it. */
static inline void ndsStageDLFastItemSeeds(NDSRendererStats *render_stats)
{
    const u32 head = (sNdsRendererAdapterItemSubmitHead <
                      NDS_RENDERER_STAGE_DL_HEADS) ?
        sNdsRendererAdapterItemSubmitHead : 0u;

    gNdsItemRendererLastHead = head;
    gNdsItemRendererLastColorMask = sNdsRendererAdapterItemColorMask[head];
    gNdsItemRendererLastEnvColor = sNdsRendererAdapterItemEnvColor[head];
    gNdsItemRendererLastOtherModeL = sNdsRendererAdapterItemOtherModeL[head];
    gNdsItemRendererLastOtherModeH = sNdsRendererAdapterItemOtherModeH[head];
    if ((sNdsRendererAdapterItemColorMask[head] & 1u) != 0u)
    {
        render_stats->prim_color = sNdsRendererAdapterItemPrimColor[head];
    }
    if ((sNdsRendererAdapterItemColorMask[head] & 2u) != 0u)
    {
        render_stats->env_color = sNdsRendererAdapterItemEnvColor[head];
    }
    if (sNdsRendererAdapterItemOtherModeLValid[head] != 0u)
    {
        render_stats->othermode_l = sNdsRendererAdapterItemOtherModeL[head];
    }
    if (sNdsRendererAdapterItemOtherModeHValid[head] != 0u)
    {
        render_stats->othermode_h = sNdsRendererAdapterItemOtherModeH[head];
    }
}

/* TRUE when a routed owner drew `dl`; FALSE sends it to the body. */
static sb32 __attribute__((noinline)) ndsRendererAdapterSubmitStageDLFast(
    DObj *dobj, const Gfx *dl, GObj *camera_gobj, u32 initial_geometry_mode)
{
    NDSStageDLRoute *route = ndsStageDLRouteSlot(dl);
    NDSRelocLoadedFile *loaded = route->loaded;
    GObj *owner = dobj->parent_gobj;
    u32 route_kind = NDS_SDL_ROUTE_NONE;
#if NDS_STAGE_CASTLE
    NDSRendererNativeMaterial bumper_material;
#endif
    /* The Fire Flower's or the N Bumper's live snapshot (the Starman's
     * first; item_route_material1 its second). */
    NDSRendererNativeMaterial item_route_material;
    NDSRendererNativeMaterial item_route_material1;
#if NDS_STAGE_SECTOR
    const u8 *laser_tex_base = NULL;
#endif
#if NDS_STAGE_INISHIE
    const u8 *inishie_pal_base = NULL;
#endif
    NDSRendererNativeMaterial pkthunder_material;
    u32 pkthunder_root_index = 0u;
    u32 pkthunder_trail_color = 0u;
    /* Zeroed once a route is taken: most lists leave at the route test. */
    NDSRendererConfig config;
    NDSRendererStats *render_stats;
    NDSRendererMatrix20p12 projection;
    NDSRendererMatrix20p12 modelview;
    NDSRendererMatrix20p12 identity;
    const NDSRendererMatrix20p12 *projection_ptr;
    const NDSRendererMatrix20p12 *modelview_ptr;
    void *saved_graphics_heap_ptr;
    sb32 handled = FALSE;

    /* The procedural visual templates need no route: the effect-tree owner
     * latched the template for this GObj, and the body claims such a list
     * before any loaded-file owner could -- its list lives in the arena, in
     * no loaded file, so no earlier candidate can match it. The same tests,
     * and the arena check whose failure the body reports instead. */
    if ((sNdsRendererAdapterVisualEffectNativeActive != FALSE) &&
        (sNdsRendererAdapterEffectSubmitActive != FALSE) &&
        (sNdsRendererAdapterImpactWaveNativeActive == FALSE) &&
        (sNdsRendererAdapterRebirthHaloNativeActive == FALSE) &&
        (sNdsRendererAdapterItemSubmitActive == FALSE) &&
        (sNdsRendererAdapterStagePersistentActive != FALSE) &&
        (dobj->dl == dl) && (dobj->child == NULL) &&
        (ndsRendererHardwareNoOracleEnabled() != FALSE) &&
        (ndsRelocFindLoadedFileContaining(dl, sizeof(*dl)) == NULL) &&
        (ndsFighterDLScanRangeInTaskmanArena(dl, sizeof(*dl)) != FALSE))
    {
        route_kind = NDS_SDL_ROUTE_VISUAL;
        loaded = NULL;
    }
    if ((route_kind == NDS_SDL_ROUTE_NONE) &&
        ((route->dl != dl) || (route->route == NDS_SDL_ROUTE_NONE) ||
         (loaded == NULL) || (loaded->data != route->data) ||
         (loaded->data_size != route->data_size) ||
         (loaded->asset_id != (u32)route->asset_id) ||
         ((dobj->mobj != NULL) && (route->route != NDS_SDL_ROUTE_BAKED) &&
          (route->route != NDS_SDL_ROUTE_BAKED_ITEM) &&
          (route->route != NDS_SDL_ROUTE_BAKED_ROOM) &&
          (route->route != NDS_SDL_ROUTE_BAKED_WEAPON) &&
          (route->route != NDS_SDL_ROUTE_MBALL) &&
          (route->route != NDS_SDL_ROUTE_SHELL) &&
          (route->route != NDS_SDL_ROUTE_STAR) &&
          (route->route != NDS_SDL_ROUTE_BOMBHEI) &&
          (route->route != NDS_SDL_ROUTE_CASTLE_BUMPER) &&
          (route->route != NDS_SDL_ROUTE_FFLOWER_LIVE) &&
          (route->route != NDS_SDL_ROUTE_NBUMPER) &&
          (route->route != NDS_SDL_ROUTE_HITOKAGE) &&
          (route->route != NDS_SDL_ROUTE_FUSHIGIBANA) &&
          (route->route != NDS_SDL_ROUTE_DAMAGE_FLY_MDUST) &&
          (route->route != NDS_SDL_ROUTE_INISHIE_PAKKUN) &&
          (route->route != NDS_SDL_ROUTE_PIKACHU_THUNDER) &&
          (route->route != NDS_SDL_ROUTE_NESS_PKTHUNDER)) ||
         (owner == NULL) ||
         (sNdsRendererAdapterStagePersistentActive == FALSE) ||
         ((sNdsRendererAdapterEffectSubmitActive != FALSE) &&
          (route->route != NDS_SDL_ROUTE_DAMAGE_FLY_MDUST) &&
          (route->route != NDS_SDL_ROUTE_KIRBYSTAR) &&
          (route->route != NDS_SDL_ROUTE_PIKACHU_THUNDER)) ||
         (ndsRendererHardwareNoOracleEnabled() == FALSE)))
    {
        return FALSE;
    }
    if (route_kind == NDS_SDL_ROUTE_NONE)
    {
        route_kind = route->route;
    }
    switch (route_kind)
    {
    case NDS_SDL_ROUTE_VISUAL:
        break;
    case NDS_SDL_ROUTE_CHARGE_SHOT:
        if ((owner->id != nGCCommonKindWeapon) ||
            (sNdsRendererAdapterItemSubmitActive != FALSE))
        {
            return FALSE;
        }
        break;
#if NDS_STAGE_SECTOR
    case NDS_SDL_ROUTE_SECTOR_LASER:
    {
        /* The texture file, found from the pointer the list carries and
         * proved against both relocated words, as the body proves it. */
        const NDSRelocLoadedFile *laser_tex;

        if ((owner->id != nGCCommonKindWeapon) ||
            (sNdsRendererAdapterItemSubmitActive != FALSE) ||
            (dobj->mobj != NULL))
        {
            return FALSE;
        }
        laser_tex = ndsRelocFindLoadedFileContaining(
            (const void *)(uintptr_t)dl[14].words.w1, 1u);
        if ((laser_tex == NULL) || (laser_tex->data == NULL) ||
            (laser_tex->asset_id != NDS_NATIVE_SECTOR_LASER_TEX_ASSET) ||
            (laser_tex->data_size < NDS_NATIVE_SECTOR_LASER_TEX_END))
        {
            return FALSE;
        }
        laser_tex_base = (const u8 *)laser_tex->data;
        if ((dl[8].words.w1 != (u32)(uintptr_t)(laser_tex_base +
                 NDS_NATIVE_SECTOR_LASER_TLUT_OFFSET)) ||
            (dl[14].words.w1 != (u32)(uintptr_t)(laser_tex_base +
                 NDS_NATIVE_SECTOR_LASER_IMAGE_OFFSET)))
        {
            return FALSE;
        }
        break;
    }
#endif
    case NDS_SDL_ROUTE_DAMAGE_FLY_MDUST:
        /* The body's arm admits the list by asset and root alone; its helper
         * snapshots the MObj under an effect owner. */
        if (sNdsRendererAdapterItemSubmitActive != FALSE)
        {
            return FALSE;
        }
        break;
    case NDS_SDL_ROUTE_PIKACHU_THUNDER:
        /* The adapter re-proves the rest on every draw. */
        if (sNdsRendererAdapterItemSubmitActive != FALSE)
        {
            return FALSE;
        }
        break;
    case NDS_SDL_ROUTE_NESS_PKTHUNDER:
    {
        /* The body's candidate test (PK Thunder's head and trail are
         * NessModel weapons with one live CURRENT_IMAGE MObj). */
        WPStruct *pkthunder_wp;

        if ((owner->id != nGCCommonKindWeapon) ||
            (sNdsRendererAdapterItemSubmitActive != FALSE) ||
            (dobj->mobj == NULL) || (dobj->mobj->next != NULL))
        {
            return FALSE;
        }
        pkthunder_wp = wpGetStruct(owner);
        if ((pkthunder_wp != NULL) &&
            (pkthunder_wp->kind == nWPKindPKThunderHead) &&
            (route->root == NDS_NATIVE_NESS_PKTHUNDER_HEAD_ROOT))
        {
            pkthunder_root_index = NDS_NATIVE_NESS_PKTHUNDER_HEAD_INDEX;
        }
        else if ((pkthunder_wp != NULL) &&
                 (pkthunder_wp->kind == nWPKindPKThunderTrail) &&
                 (route->root == NDS_NATIVE_NESS_PKTHUNDER_TRAIL_ROOT) &&
                 ((u32)pkthunder_wp->weapon_vars.pkthunder_trail.trail_id <
                  NDS_NATIVE_NESS_PKTHUNDER_TRAIL_COLOR_COUNT))
        {
            pkthunder_root_index = NDS_NATIVE_NESS_PKTHUNDER_TRAIL_INDEX;
            pkthunder_trail_color =
                (u32)pkthunder_wp->weapon_vars.pkthunder_trail.trail_id;
        }
        else
        {
            return FALSE;
        }
        if ((ndsRendererAdapterBuildNativeMaterialSnapshot(
                 dobj->mobj, &pkthunder_material, FALSE, NULL, NULL) ==
             FALSE) ||
            (pkthunder_material.effects !=
                 NDS_NATIVE_NESS_PKTHUNDER_MATERIAL_EFFECTS))
        {
            return FALSE;
        }
        break;
    }
    case NDS_SDL_ROUTE_KIRBYSTAR:
        if ((sNdsRendererAdapterItemSubmitActive != FALSE) ||
            (dobj->mobj != NULL))
        {
            return FALSE;
        }
        break;
#if NDS_STAGE_INISHIE
    case NDS_SDL_ROUTE_INISHIE_PAKKUN:
    {
        const NDSRelocLoadedFile *pal;

        if ((owner->id != nGCCommonKindItem) ||
            (sNdsRendererAdapterItemSubmitActive == FALSE) ||
            (dobj->mobj == NULL) || (dobj->mobj->next != NULL) ||
            (ndsRendererAdapterMaterialFlags(dobj->mobj) != 0x0001u) ||
            (ndsRendererAdapterBuildNativeMaterialSnapshot(
                 dobj->mobj, &item_route_material, FALSE, NULL, NULL) == FALSE) ||
            (item_route_material.effects !=
                 NDS_RENDERER_NATIVE_MATERIAL_CURRENT_IMAGE) ||
            (ndsRelocFindLoadedFileContaining(
                 (const void *)(uintptr_t)item_route_material.current_image,
                 1u) != loaded))
        {
            return FALSE;
        }
        pal = ndsRelocFindLoadedFileByAsset(107u);
        if ((pal == NULL) || (pal->data_size < (0x3620u + 32u)))
        {
            return FALSE;
        }
        inishie_pal_base = (const u8 *)pal->data;
        break;
    }
    case NDS_SDL_ROUTE_INISHIE_POWBLOCK:
    {
        const NDSRelocLoadedFile *pal;
        const u8 *pow_base = (const u8 *)loaded->data;

        if ((owner->id != nGCCommonKindItem) ||
            (sNdsRendererAdapterItemSubmitActive == FALSE) ||
            (dobj->mobj != NULL))
        {
            return FALSE;
        }
        pal = ndsRelocFindLoadedFileContaining(
            (const void *)(uintptr_t)dl[8].words.w1, 1u);
        if ((pal == NULL) || (pal->data == NULL) ||
            (pal->asset_id != NDS_NATIVE_INISHIE_POWBLOCK_PAL_ASSET) ||
            (pal->data_size < NDS_NATIVE_INISHIE_POWBLOCK_TLUT_END))
        {
            return FALSE;
        }
        inishie_pal_base = (const u8 *)pal->data;
        if ((dl[8].words.w1 != (u32)(uintptr_t)(inishie_pal_base +
                 NDS_NATIVE_INISHIE_POWBLOCK_TLUT_OFFSET)) ||
            (dl[14].words.w1 != (u32)(uintptr_t)(pow_base +
                 NDS_NATIVE_INISHIE_POWBLOCK_IMAGE_A_OFFSET)) ||
            (dl[26].words.w1 != (u32)(uintptr_t)(pow_base +
                 NDS_NATIVE_INISHIE_POWBLOCK_IMAGE_B_OFFSET)))
        {
            return FALSE;
        }
        break;
    }
#endif
    case NDS_SDL_ROUTE_BAKED:
        if ((owner->id != nGCCommonKindGroundDisplay) ||
            (sNdsRendererAdapterItemSubmitActive != FALSE))
        {
            return FALSE;
        }
        break;
    case NDS_SDL_ROUTE_BAKED_ITEM:
        if ((owner->id != nGCCommonKindItem) ||
            (sNdsRendererAdapterItemSubmitActive == FALSE))
        {
            return FALSE;
        }
        break;
    case NDS_SDL_ROUTE_BAKED_WEAPON:
        if ((owner->id != nGCCommonKindWeapon) ||
            (sNdsRendererAdapterItemSubmitActive != FALSE))
        {
            return FALSE;
        }
        break;
#if NDS_1P_GAME
    case NDS_SDL_ROUTE_BAKED_ROOM:
        if ((sNdsRendererAdapterItemSubmitActive != FALSE) ||
            (ndsNativeBakedRootIsRoom(
                 (const void *)(uintptr_t)route->root) == FALSE))
        {
            return FALSE;
        }
        break;
#endif
#if NDS_STAGE_CASTLE
    case NDS_SDL_ROUTE_CASTLE_BUMPER:
    {
        ITStruct *ip;

        if ((owner->id != nGCCommonKindItem) ||
            (sNdsRendererAdapterItemSubmitActive == FALSE) ||
            (dobj->mobj == NULL) || (dobj->mobj->next != NULL) ||
            (ndsRendererAdapterMaterialFlags(dobj->mobj) !=
                 NDS_NATIVE_CASTLE_BUMPER_MOBJ_FLAGS) ||
            (route->data_size < NDS_NATIVE_CASTLE_BUMPER_IMAGE_END))
        {
            return FALSE;
        }
        ip = itGetStruct(owner);
        if ((ip == NULL) || (ip->kind != nITKindGBumper) ||
            (ndsRendererAdapterBuildNativeMaterialSnapshot(
                 dobj->mobj, &bumper_material, FALSE, NULL, NULL) == FALSE) ||
            (bumper_material.effects !=
                 NDS_RENDERER_NATIVE_MATERIAL_PALETTE_IMAGE))
        {
            return FALSE;
        }
        break;
    }
#endif
    case NDS_SDL_ROUTE_FFLOWER_LIVE:
    {
        ITStruct *ip;

        if ((owner->id != nGCCommonKindItem) ||
            (sNdsRendererAdapterItemSubmitActive == FALSE) ||
            (dobj->mobj == NULL) || (dobj->mobj->next != NULL) ||
            (ndsRendererAdapterMaterialFlags(dobj->mobj) !=
                 NDS_NATIVE_ITEM_FFLOWER_MOBJ_FLAGS) ||
            (route->data_size < NDS_NATIVE_ITEM_FFLOWER_FILE_END))
        {
            return FALSE;
        }
        ip = itGetStruct(owner);
        if ((ip == NULL) || (ip->kind != nITKindFFlower) ||
            (ndsRendererAdapterBuildNativeMaterialSnapshot(
                 dobj->mobj, &item_route_material, FALSE, NULL, NULL) == FALSE) ||
            (item_route_material.effects !=
                 NDS_RENDERER_NATIVE_MATERIAL_PALETTE_IMAGE))
        {
            return FALSE;
        }
        break;
    }
    case NDS_SDL_ROUTE_MBALL:
    {
        /* The body's two Poke Ball roots: the baked one without an MObj, the
         * live one with one CURRENT_IMAGE snapshot. */
        ITStruct *ip;

        if ((owner->id != nGCCommonKindItem) ||
            (sNdsRendererAdapterItemSubmitActive == FALSE) ||
            (route->data_size < NDS_NATIVE_ITEM_MBALL_FILE_END))
        {
            return FALSE;
        }
        ip = itGetStruct(owner);
        if ((ip == NULL) || (ip->kind != nITKindMBall))
        {
            return FALSE;
        }
        if (route->root == NDS_NATIVE_ITEM_MBALL_BAKED_ROOT)
        {
            if (dobj->mobj != NULL)
            {
                return FALSE;
            }
        }
        else if ((route->root != NDS_NATIVE_ITEM_MBALL_LIVE_ROOT) ||
                 (dobj->mobj == NULL) || (dobj->mobj->next != NULL) ||
                 (ndsRendererAdapterBuildNativeMaterialSnapshot(
                      dobj->mobj, &item_route_material, FALSE, NULL, NULL) ==
                  FALSE) ||
                 (item_route_material.effects !=
                      NDS_RENDERER_NATIVE_MATERIAL_CURRENT_IMAGE))
        {
            return FALSE;
        }
        break;
    }
    case NDS_SDL_ROUTE_SHELL:
    {
        ITStruct *ip;

        if ((owner->id != nGCCommonKindItem) ||
            (sNdsRendererAdapterItemSubmitActive == FALSE) ||
            (dobj->mobj == NULL) || (dobj->mobj->next != NULL))
        {
            return FALSE;
        }
        ip = itGetStruct(owner);
        if ((ip == NULL) ||
            ((ip->kind == nITKindGShell) ?
                 (route->data_size < NDS_NATIVE_ITEM_GSHELL_FILE_END) :
             (ip->kind == nITKindRShell) ?
                 (route->data_size < NDS_NATIVE_ITEM_RSHELL_FILE_END) : TRUE) ||
            (ndsRendererAdapterBuildNativeMaterialSnapshot(
                 dobj->mobj, &item_route_material, FALSE, NULL, NULL) == FALSE) ||
            (item_route_material.effects !=
                 (NDS_RENDERER_NATIVE_MATERIAL_PALETTE_IMAGE |
                  NDS_RENDERER_NATIVE_MATERIAL_PALETTE_TLUT |
                  NDS_RENDERER_NATIVE_MATERIAL_CURRENT_IMAGE)))
        {
            return FALSE;
        }
        break;
    }
    case NDS_SDL_ROUTE_BOMBHEI:
    {
        ITStruct *ip;

        if ((owner->id != nGCCommonKindItem) ||
            (sNdsRendererAdapterItemSubmitActive == FALSE) ||
            (dobj->mobj == NULL) || (dobj->mobj->next != NULL) ||
            (route->data_size < NDS_NATIVE_ITEM_BOMBHEI_FILE_END))
        {
            return FALSE;
        }
        ip = itGetStruct(owner);
        if ((ip == NULL) || (ip->kind != nITKindBombHei) ||
            (ndsRendererAdapterBuildNativeMaterialSnapshot(
                 dobj->mobj, &item_route_material, FALSE, NULL, NULL) == FALSE) ||
            (item_route_material.effects !=
                 NDS_RENDERER_NATIVE_MATERIAL_CURRENT_IMAGE))
        {
            return FALSE;
        }
        break;
    }
    case NDS_SDL_ROUTE_STAR:
    {
        ITStruct *ip;

        if ((owner->id != nGCCommonKindItem) ||
            (sNdsRendererAdapterItemSubmitActive == FALSE) ||
            (dobj->mobj == NULL) || (dobj->mobj->next == NULL) ||
            (dobj->mobj->next->next != NULL) ||
            (route->data_size < NDS_NATIVE_ITEM_STAR_FILE_END))
        {
            return FALSE;
        }
        ip = itGetStruct(owner);
        if ((ip == NULL) || (ip->kind != nITKindStar) ||
            (ndsRendererAdapterBuildNativeMaterialSnapshot(
                 dobj->mobj, &item_route_material, FALSE, NULL, NULL) == FALSE) ||
            (ndsRendererAdapterBuildNativeMaterialSnapshot(
                 dobj->mobj->next, &item_route_material1, FALSE, NULL,
                 NULL) == FALSE) ||
            (item_route_material.effects !=
                 NDS_RENDERER_NATIVE_MATERIAL_PALETTE_IMAGE) ||
            (item_route_material1.effects !=
                 NDS_RENDERER_NATIVE_MATERIAL_PALETTE_IMAGE))
        {
            return FALSE;
        }
        break;
    }
#if NDS_STAGE_YAMABUKI
    case NDS_SDL_ROUTE_HITOKAGE:
    case NDS_SDL_ROUTE_FUSHIGIBANA:
    {
        ITStruct *ip;

        if ((owner->id != nGCCommonKindItem) ||
            (sNdsRendererAdapterItemSubmitActive == FALSE) ||
            (dobj->mobj == NULL))
        {
            return FALSE;
        }
        ip = itGetStruct(owner);
        if ((ip == NULL) ||
            (ip->kind != ((route_kind == NDS_SDL_ROUTE_HITOKAGE) ?
                              nITKindHitokage : nITKindFushigibana)) ||
            (ndsRendererAdapterBuildNativeMaterialSnapshot(
                 dobj->mobj, &item_route_material, FALSE, NULL, NULL) == FALSE) ||
            (item_route_material.effects !=
                 NDS_RENDERER_NATIVE_MATERIAL_CURRENT_IMAGE))
        {
            return FALSE;
        }
        break;
    }
#endif
    case NDS_SDL_ROUTE_NBUMPER:
    {
        ITStruct *ip;

        if ((owner->id != nGCCommonKindItem) ||
            (sNdsRendererAdapterItemSubmitActive == FALSE) ||
            (dobj->mobj == NULL) || (dobj->mobj->next != NULL) ||
            (ndsRendererAdapterMaterialFlags(dobj->mobj) !=
                 NDS_NATIVE_ITEM_NBUMPER_MOBJ_FLAGS) ||
            (route->data_size < NDS_NATIVE_ITEM_NBUMPER_FILE_END))
        {
            return FALSE;
        }
        ip = itGetStruct(owner);
        if ((ip == NULL) || (ip->kind != nITKindNBumper) ||
            (ndsRendererAdapterBuildNativeMaterialSnapshot(
                 dobj->mobj, &item_route_material, FALSE, NULL, NULL) == FALSE) ||
            (item_route_material.effects !=
                 NDS_RENDERER_NATIVE_MATERIAL_PALETTE_IMAGE))
        {
            return FALSE;
        }
        break;
    }
    default:
    {
        ITStruct *ip;
        const u32 item_route = route_kind - NDS_SDL_ROUTE_ITEM;

        if ((item_route >= (u32)nNDSStageDLItemRouteCount) ||
            (sNdsRendererAdapterItemSubmitActive == FALSE) ||
            (owner->id != nGCCommonKindItem))
        {
            return FALSE;
        }
        ip = itGetStruct(owner);
        if ((ip == NULL) ||
            ((u32)ip->kind != sNdsStageDLItemRoutes[item_route].item_kind))
        {
            return FALSE;
        }
        break;
    }
    }

#if defined(NDS_LAB_FOURCPU_SWEEP) && NDS_LAB_FOURCPU_SWEEP
    const sb32 lab_item = (route_kind >= NDS_SDL_ROUTE_ITEM) ? TRUE : FALSE;
    u32 lab_item_mark = cpuGetTiming();
#endif
    saved_graphics_heap_ptr = gSYTaskmanGraphicsHeap.ptr;
    ndsRendererAdapterPrepareInitialMatrices(dobj,
                                             (camera_gobj != NULL) ?
                                                 CObjGetStruct(camera_gobj) :
                                                 ((gGCCurrentCamera != NULL) ?
                                                      CObjGetStruct(
                                                          gGCCurrentCamera) :
                                                      NULL),
                                             TRUE,
                                             &projection, &projection_ptr,
                                             &modelview, &modelview_ptr);
#if defined(NDS_LAB_FOURCPU_SWEEP) && NDS_LAB_FOURCPU_SWEEP
    if (lab_item != FALSE)
    {
        u32 lab_now = cpuGetTiming();

        NDS_DIAG(gNdsLabItemAcc[4] += lab_now - lab_item_mark);
        NDS_DIAG(gNdsLabItemAcc[8]++);
        NDS_DIAG(gNdsLabItemAcc[12] += gNdsLabPimAcc[1]);
        gNdsLabItemAcc[13] += gNdsLabPimAcc[0] + gNdsLabPimAcc[2] +
                              gNdsLabPimAcc[3];
        lab_item_mark = lab_now;
    }
#endif
    memset(&config, 0, sizeof(config));
    config.max_depth = 8u;
    config.max_commands = 8192u;
    config.max_list_commands = 512u;
    config.initial_projection = projection_ptr;
    config.initial_modelview = modelview_ptr;
    config.initial_geometry_mode = initial_geometry_mode;
    config.texture_data_layout = NDS_RENDERER_TEXTURE_DATA_O2R_WORD_SWAPPED;
    config.validate_range = ndsRendererAdapterStageValidateRange;
    config.immutable_command_span = ndsRendererAdapterImmutableCommandSpan;
    config.resolve_branch = ndsFighterDLDrawResolveBranch;
    config.resolve_data = ndsFighterDLDrawResolveRendererData;
    /* The body hands its draw state here; no routed owner reads it. */
    config.user = NULL;
    /* The owners' split-camera contract, on this copy as on the body's. */
    if ((config.initial_projection == NULL) &&
        (config.initial_modelview != NULL))
    {
        ndsRendererAdapterMtxIdentity20p12(&identity);
        config.initial_projection = &identity;
    }
    else if ((config.initial_modelview == NULL) &&
             (config.initial_projection != NULL))
    {
        ndsRendererAdapterMtxIdentity20p12(&identity);
        config.initial_modelview = &identity;
    }
    /* P2-6 (2026-10-01): a ground display's baked root wholly outside the
     * view draws nothing; skip it before the stats reset, its MObj snapshots
     * and the submit (Board the Platforms submits ~26 a frame, most of them
     * off screen; see ndsNativeBakedRootOffscreen). */
#if defined(NDS_LAB_FOURCPU_SWEEP) && NDS_LAB_FOURCPU_SWEEP
    u32 lab_offscreen = cpuGetTiming();
    sb32 lab_culled = ((route_kind == NDS_SDL_ROUTE_BAKED) ||
         (route_kind == NDS_SDL_ROUTE_BAKED_ITEM) ||
         (route_kind == NDS_SDL_ROUTE_BAKED_ROOM) ||
         (route_kind == NDS_SDL_ROUTE_BAKED_WEAPON)) &&
        (ndsNativeBakedRootOffscreen((const void *)(uintptr_t)route->root,
                                     &config) != FALSE);
    NDS_DIAG(gNdsLabBakedAcc[0] += cpuGetTiming() - lab_offscreen);
    NDS_DIAG(gNdsLabBakedAcc[15] += (lab_culled != FALSE) ? 1u : 0u);
    if (lab_culled != FALSE)
#else
    if (((route_kind == NDS_SDL_ROUTE_BAKED) ||
         (route_kind == NDS_SDL_ROUTE_BAKED_ITEM) ||
         (route_kind == NDS_SDL_ROUTE_BAKED_ROOM) ||
         (route_kind == NDS_SDL_ROUTE_BAKED_WEAPON)) &&
        (ndsNativeBakedRootOffscreen((const void *)(uintptr_t)route->root,
                                     &config) != FALSE))
#endif
    {
        gSYTaskmanGraphicsHeap.ptr = saved_graphics_heap_ptr;
        NDS_DIAG(gNdsStageDLFastLaneHits++);
        return TRUE;
    }
#if NDS_STAGE_CASTLE
    /* P2-6 (2026-10-02): the same exit for a bumper quad wholly outside the
     * view (see ndsNativeCastleBumperOffscreen). */
    if ((route_kind == NDS_SDL_ROUTE_CASTLE_BUMPER) &&
        (ndsNativeCastleBumperOffscreen(&config) != FALSE))
    {
        gSYTaskmanGraphicsHeap.ptr = saved_graphics_heap_ptr;
        NDS_DIAG(gNdsStageDLFastLaneHits++);
        return TRUE;
    }
#endif
#if defined(NDS_LAB_FOURCPU_SWEEP) && NDS_LAB_FOURCPU_SWEEP
    u32 lab_prep = cpuGetTiming();
#endif
    render_stats = &sNdsRendererAdapterStagePersistentStats;
    ndsFighterDLDrawResetRuntimeRendererStats(render_stats);
    NDS_DIAG(gNdsStageGCDrawAllLoopHardwareCarrySeedCount++);

    if (route_kind == NDS_SDL_ROUTE_VISUAL)
    {
        /* The body's effect-submit seeds, then its visual owner. */
        if ((sNdsRendererAdapterEffectColorMask & 1u) != 0u)
        {
            render_stats->prim_color = sNdsRendererAdapterEffectPrimColor;
        }
        if ((sNdsRendererAdapterEffectColorMask & 2u) != 0u)
        {
            render_stats->env_color = sNdsRendererAdapterEffectEnvColor;
        }
        if (sNdsRendererAdapterEffectOtherModeValid != 0u)
        {
            render_stats->othermode_l = sNdsRendererAdapterEffectOtherModeL;
        }
        gNdsEffectDLSubmitOtherModeIn = render_stats->othermode_l;
        handled = ndsRendererSubmitNativeVisualEffect(
            sNdsRendererAdapterVisualEffectTemplate, &config, render_stats);
        if (handled == FALSE)
        {
            ndsStageRejectNativeRender(dobj, dl,
                NDS_NATIVE_FAILURE_REJECTED_PROGRAM, render_stats);
            /* Recorded with its own reason, as the body settles it. */
            handled = TRUE;
        }
        gNdsEffectDLSubmitOtherModeOut = render_stats->othermode_l;
        NDS_DIAG(gNdsEffectDLSubmitCount++);
        NDS_DIAG(gNdsEffectDLPublishCount++);
    }
    else
    if (route_kind == NDS_SDL_ROUTE_CHARGE_SHOT)
    {
        handled = ndsRendererSubmitNativeSamusChargeShot(
            loaded->data, loaded->data_size, &config, render_stats);
        if (handled != FALSE)
        {
            NDS_DIAG(gNdsChargeShotDrawCount++);
        }
        else
        {
            NDS_DIAG(gNdsChargeShotSubmitFailCount++);
        }
    }
#if NDS_STAGE_SECTOR
    else if (route_kind == NDS_SDL_ROUTE_SECTOR_LASER)
    {
        handled = ndsRendererSubmitNativeSectorArwingLaser(
            laser_tex_base + NDS_NATIVE_SECTOR_LASER_TLUT_OFFSET,
            laser_tex_base + NDS_NATIVE_SECTOR_LASER_IMAGE_OFFSET, &config,
            render_stats);
        if (handled != FALSE)
        {
            NDS_DIAG(gNdsSectorLaserDrawCount++);
        }
        else
        {
            NDS_DIAG(gNdsSectorLaserSubmitFailCount++);
        }
    }
#endif
#if NDS_STAGE_INISHIE
    else if (route_kind == NDS_SDL_ROUTE_INISHIE_PAKKUN)
    {
        ndsStageDLFastItemSeeds(render_stats);
        handled = ndsRendererSubmitNativeInishiePakkun(
            loaded->data, loaded->data_size, inishie_pal_base + 0x3620u,
            &item_route_material, &config, render_stats);
        if (handled != FALSE)
        {
            NDS_DIAG(gNdsInishiePakkunDrawCount++);
        }
        else
        {
            NDS_DIAG(gNdsInishiePakkunSubmitFailCount++);
        }
    }
    else if (route_kind == NDS_SDL_ROUTE_INISHIE_POWBLOCK)
    {
        const u8 *pow_base = (const u8 *)loaded->data;

        ndsStageDLFastItemSeeds(render_stats);
        handled = ndsRendererSubmitNativeInishiePowblock(
            inishie_pal_base + NDS_NATIVE_INISHIE_POWBLOCK_TLUT_OFFSET,
            pow_base + NDS_NATIVE_INISHIE_POWBLOCK_IMAGE_A_OFFSET,
            pow_base + NDS_NATIVE_INISHIE_POWBLOCK_IMAGE_B_OFFSET, &config,
            render_stats);
        if (handled != FALSE)
        {
            NDS_DIAG(gNdsInishiePowblockDrawCount++);
        }
        else
        {
            NDS_DIAG(gNdsInishiePowblockSubmitFailCount++);
        }
    }
#endif
    else if (route_kind == NDS_SDL_ROUTE_KIRBYSTAR)
    {
        if (sNdsRendererAdapterEffectSubmitActive != FALSE)
        {
            if ((sNdsRendererAdapterEffectColorMask & 1u) != 0u)
            {
                render_stats->prim_color = sNdsRendererAdapterEffectPrimColor;
            }
            if ((sNdsRendererAdapterEffectColorMask & 2u) != 0u)
            {
                render_stats->env_color = sNdsRendererAdapterEffectEnvColor;
            }
            if (sNdsRendererAdapterEffectOtherModeValid != 0u)
            {
                render_stats->othermode_l = sNdsRendererAdapterEffectOtherModeL;
            }
            gNdsEffectDLSubmitOtherModeIn = render_stats->othermode_l;
        }
        handled = ndsRendererSubmitNativeItemKirbyStar(
            loaded->data, loaded->data_size, NULL, &config, render_stats);
        if (handled != FALSE)
        {
            NDS_DIAG(gNdsItemKirbyStarDrawCount++);
        }
        else
        {
            NDS_DIAG(gNdsItemKirbyStarSubmitFailCount++);
        }
        if (sNdsRendererAdapterEffectSubmitActive != FALSE)
        {
            gNdsEffectDLSubmitOtherModeOut = render_stats->othermode_l;
            NDS_DIAG(gNdsEffectDLSubmitCount++);
            NDS_DIAG(gNdsEffectDLPublishCount++);
        }
    }
    else if (route_kind == NDS_SDL_ROUTE_PIKACHU_THUNDER)
    {
        /* Under the effect layer (the trail effect, the shock roots) the
         * body's effect-submit seeds come first, as for the Kirby star. */
        if (sNdsRendererAdapterEffectSubmitActive != FALSE)
        {
            if ((sNdsRendererAdapterEffectColorMask & 1u) != 0u)
            {
                render_stats->prim_color = sNdsRendererAdapterEffectPrimColor;
            }
            if ((sNdsRendererAdapterEffectColorMask & 2u) != 0u)
            {
                render_stats->env_color = sNdsRendererAdapterEffectEnvColor;
            }
            if (sNdsRendererAdapterEffectOtherModeValid != 0u)
            {
                render_stats->othermode_l = sNdsRendererAdapterEffectOtherModeL;
            }
            gNdsEffectDLSubmitOtherModeIn = render_stats->othermode_l;
        }
        handled = ndsRendererAdapterPikachuThunder(loaded, dobj, dl, &config,
                                                   render_stats);
        if (handled == FALSE)
        {
            /* Declined before drawing anything: the body decides. */
            gSYTaskmanGraphicsHeap.ptr = saved_graphics_heap_ptr;
            return FALSE;
        }
        if (sNdsRendererAdapterEffectSubmitActive != FALSE)
        {
            gNdsEffectDLSubmitOtherModeOut = render_stats->othermode_l;
            NDS_DIAG(gNdsEffectDLSubmitCount++);
            NDS_DIAG(gNdsEffectDLPublishCount++);
        }
    }
    else if (route_kind == NDS_SDL_ROUTE_NESS_PKTHUNDER)
    {
        handled = ndsRendererSubmitNativeNessPKThunder(
            pkthunder_root_index, &pkthunder_material, pkthunder_trail_color,
            &config, render_stats);
        if (handled == FALSE)
        {
            /* Declined before drawing anything: the body decides. */
            gSYTaskmanGraphicsHeap.ptr = saved_graphics_heap_ptr;
            return FALSE;
        }
    }
    else if (route_kind == NDS_SDL_ROUTE_DAMAGE_FLY_MDUST)
    {
        /* The body's effect-submit seeds and witnesses, then its arm. */
        if (sNdsRendererAdapterEffectSubmitActive != FALSE)
        {
            if ((sNdsRendererAdapterEffectColorMask & 1u) != 0u)
            {
                render_stats->prim_color = sNdsRendererAdapterEffectPrimColor;
            }
            if ((sNdsRendererAdapterEffectColorMask & 2u) != 0u)
            {
                render_stats->env_color = sNdsRendererAdapterEffectEnvColor;
            }
            if (sNdsRendererAdapterEffectOtherModeValid != 0u)
            {
                render_stats->othermode_l = sNdsRendererAdapterEffectOtherModeL;
            }
            gNdsEffectDLSubmitOtherModeIn = render_stats->othermode_l;
        }
        if (ndsRendererAdapterSubmitDamageFlyMDust(
                dobj, loaded, &config, render_stats) == FALSE)
        {
            ndsStageRejectNativeRender(dobj, dl,
                NDS_NATIVE_FAILURE_REJECTED_PROGRAM, render_stats);
        }
        if (sNdsRendererAdapterEffectSubmitActive != FALSE)
        {
            gNdsEffectDLSubmitOtherModeOut = render_stats->othermode_l;
            NDS_DIAG(gNdsEffectDLSubmitCount++);
            NDS_DIAG(gNdsEffectDLPublishCount++);
        }
        /* Settled either way, as the body settles it. */
        handled = TRUE;
    }
#if NDS_STAGE_CASTLE
    else if (route_kind == NDS_SDL_ROUTE_CASTLE_BUMPER)
    {
        ndsStageDLFastItemSeeds(render_stats);
        handled = ndsRendererSubmitNativeCastleBumper(
            loaded->data, loaded->data_size, &bumper_material, &config,
            render_stats);
        if (handled != FALSE)
        {
            NDS_DIAG(gNdsCastleBumperDrawCount++);
        }
        else
        {
            NDS_DIAG(gNdsCastleBumperSubmitFailCount++);
        }
    }
#endif
    else if (route_kind == NDS_SDL_ROUTE_FFLOWER_LIVE)
    {
        ndsStageDLFastItemSeeds(render_stats);
        handled = ndsRendererSubmitNativeItemFFlower(
            NDS_NATIVE_ITEM_FFLOWER_LIVE_ROOT, loaded->data,
            loaded->data_size, &item_route_material, &config, render_stats);
        if (handled != FALSE)
        {
            NDS_DIAG(gNdsItemFFlowerDrawCount++);
        }
        else
        {
            NDS_DIAG(gNdsItemFFlowerSubmitFailCount++);
        }
    }
    else if (route_kind == NDS_SDL_ROUTE_MBALL)
    {
        ndsStageDLFastItemSeeds(render_stats);
        handled = ndsRendererSubmitNativeItemMBall(
            route->root, loaded->data, loaded->data_size,
            (route->root == NDS_NATIVE_ITEM_MBALL_LIVE_ROOT) ?
                &item_route_material : NULL,
            &config, render_stats);
        if (handled != FALSE)
        {
            NDS_DIAG(gNdsItemMBallDrawCount++);
        }
        else
        {
            NDS_DIAG(gNdsItemMBallSubmitFailCount++);
        }
    }
    else if (route_kind == NDS_SDL_ROUTE_SHELL)
    {
        ndsStageDLFastItemSeeds(render_stats);
        if (itGetStruct(owner)->kind == nITKindGShell)
        {
            handled = ndsRendererSubmitNativeItemGShell(
                loaded->data, loaded->data_size, &item_route_material,
                &config, render_stats);
            if (handled != FALSE)
            {
                NDS_DIAG(gNdsItemGShellDrawCount++);
            }
            else
            {
                NDS_DIAG(gNdsItemGShellSubmitFailCount++);
            }
        }
        else
        {
            handled = ndsRendererSubmitNativeItemRShell(
                loaded->data, loaded->data_size, &item_route_material,
                &config, render_stats);
            if (handled != FALSE)
            {
                NDS_DIAG(gNdsItemRShellDrawCount++);
            }
            else
            {
                NDS_DIAG(gNdsItemRShellSubmitFailCount++);
            }
        }
    }
    else if (route_kind == NDS_SDL_ROUTE_BOMBHEI)
    {
        ndsStageDLFastItemSeeds(render_stats);
        handled = ndsRendererSubmitNativeItemBombHei(
            loaded->data, loaded->data_size, &item_route_material, &config,
            render_stats);
        if (handled != FALSE)
        {
            NDS_DIAG(gNdsItemBombHeiDrawCount++);
        }
        else
        {
            NDS_DIAG(gNdsItemBombHeiSubmitFailCount++);
        }
    }
    else if (route_kind == NDS_SDL_ROUTE_STAR)
    {
        ndsStageDLFastItemSeeds(render_stats);
        handled = ndsRendererSubmitNativeItemStar(
            loaded->data, loaded->data_size, &item_route_material,
            &item_route_material1, &config, render_stats);
        if (handled != FALSE)
        {
            NDS_DIAG(gNdsItemStarDrawCount++);
        }
        else
        {
            NDS_DIAG(gNdsItemStarSubmitFailCount++);
        }
    }
#if NDS_STAGE_YAMABUKI
    else if ((route_kind == NDS_SDL_ROUTE_HITOKAGE) ||
             (route_kind == NDS_SDL_ROUTE_FUSHIGIBANA))
    {
        ndsStageDLFastItemSeeds(render_stats);
        if (route_kind == NDS_SDL_ROUTE_HITOKAGE)
        {
            handled = ndsRendererSubmitNativeItemHitokage(
                loaded->data, loaded->data_size, &item_route_material,
                &config, render_stats);
            if (handled != FALSE)
            {
                NDS_DIAG(gNdsYamabukiHitokageDrawCount++);
            }
            else
            {
                NDS_DIAG(gNdsYamabukiHitokageSubmitFailCount++);
            }
        }
        else
        {
            handled = ndsRendererSubmitNativeItemFushigibana(
                loaded->data, loaded->data_size, &item_route_material,
                &config, render_stats);
            if (handled != FALSE)
            {
                NDS_DIAG(gNdsYamabukiFushigibanaDrawCount++);
            }
            else
            {
                NDS_DIAG(gNdsYamabukiFushigibanaSubmitFailCount++);
            }
        }
    }
#endif
    else if (route_kind == NDS_SDL_ROUTE_NBUMPER)
    {
        ndsStageDLFastItemSeeds(render_stats);
        handled = ndsRendererSubmitNativeItemNBumper(
            loaded->data, loaded->data_size, &item_route_material, &config,
            render_stats);
        if (handled != FALSE)
        {
            NDS_DIAG(gNdsItemNBumperDrawCount++);
        }
        else
        {
            NDS_DIAG(gNdsItemNBumperSubmitFailCount++);
        }
    }
    else if ((route_kind == NDS_SDL_ROUTE_BAKED) ||
             (route_kind == NDS_SDL_ROUTE_BAKED_ITEM) ||
             (route_kind == NDS_SDL_ROUTE_BAKED_ROOM) ||
             (route_kind == NDS_SDL_ROUTE_BAKED_WEAPON))
    {
        /* The body's baked branch: the DObj's MObjs in order are the root's
         * live segment-E materials. */
        NDSRendererNativeMaterial baked_materials[
            NDS_NATIVE_BAKED_MATERIAL_SLOTS];
        MObj *mobj = dobj->mobj;
        u32 slots = (u32)route->pad;
        u32 i;

        if (route_kind == NDS_SDL_ROUTE_BAKED_ITEM)
        {
            ndsStageDLFastItemSeeds(render_stats);
        }

        for (i = 0u; (i < slots) && (i < NDS_NATIVE_BAKED_MATERIAL_SLOTS);
             i++)
        {
            if ((mobj == NULL) ||
                (ndsRendererAdapterBuildNativeMaterialSnapshot(
                     mobj, &baked_materials[i], FALSE, NULL, NULL) == FALSE))
            {
                break;
            }
            mobj = mobj->next;
        }
#if defined(NDS_LAB_FOURCPU_SWEEP) && NDS_LAB_FOURCPU_SWEEP
        NDS_DIAG(gNdsLabBakedAcc[1] += cpuGetTiming() - lab_prep);
#endif
        ndsNativeBakedSetSeedTranslate(&dobj->translate.vec.f);
        handled = (i == slots) ?
            ndsRendererSubmitNativeBaked(
                (const void *)(uintptr_t)route->root, loaded->data,
                loaded->data_size, baked_materials, i, &config,
                render_stats) :
            FALSE;
        ndsNativeBakedSetSeedTranslate(NULL);
        if (handled != FALSE)
        {
            NDS_DIAG(gNdsItemBakedDrawCount++);
        }
        else
        {
            NDS_DIAG(gNdsItemBakedSubmitFailCount++);
        }
    }
    else
    {
        u32 head = (sNdsRendererAdapterItemSubmitHead <
                    NDS_RENDERER_STAGE_DL_HEADS) ?
            sNdsRendererAdapterItemSubmitHead : 0u;

        gNdsItemRendererLastHead = head;
        gNdsItemRendererLastColorMask =
            sNdsRendererAdapterItemColorMask[head];
        gNdsItemRendererLastEnvColor = sNdsRendererAdapterItemEnvColor[head];
        gNdsItemRendererLastOtherModeL =
            sNdsRendererAdapterItemOtherModeL[head];
        gNdsItemRendererLastOtherModeH =
            sNdsRendererAdapterItemOtherModeH[head];
        if ((sNdsRendererAdapterItemColorMask[head] & 1u) != 0u)
        {
            render_stats->prim_color = sNdsRendererAdapterItemPrimColor[head];
        }
        if ((sNdsRendererAdapterItemColorMask[head] & 2u) != 0u)
        {
            render_stats->env_color = sNdsRendererAdapterItemEnvColor[head];
        }
        if (sNdsRendererAdapterItemOtherModeLValid[head] != 0u)
        {
            render_stats->othermode_l =
                sNdsRendererAdapterItemOtherModeL[head];
        }
        if (sNdsRendererAdapterItemOtherModeHValid[head] != 0u)
        {
            render_stats->othermode_h =
                sNdsRendererAdapterItemOtherModeH[head];
        }
        const NDSStageDLItemRoute *item =
            &sNdsStageDLItemRoutes[route_kind - NDS_SDL_ROUTE_ITEM];
        u32 replay_first = 0u;
        const sb32 replay_record =
            ndsItemReplayRootBegin(dobj, dl, route_kind, &replay_first);

#if defined(NDS_LAB_FOURCPU_SWEEP) && NDS_LAB_FOURCPU_SWEEP
        {
            u32 lab_now = cpuGetTiming();

            NDS_DIAG(gNdsLabItemAcc[5] += lab_now - lab_item_mark);
            lab_item_mark = lab_now;
        }
#endif
        handled = (item->submit_rooted != NULL) ?
            item->submit_rooted(route->root, loaded->data,
                                loaded->data_size, &config, render_stats) :
            item->submit(loaded->data, loaded->data_size, &config,
                         render_stats);
        if (replay_record != FALSE)
        {
            ndsItemReplayRootEnd(handled, replay_first);
        }
#if defined(NDS_LAB_FOURCPU_SWEEP) && NDS_LAB_FOURCPU_SWEEP
        {
            u32 lab_now = cpuGetTiming();

            NDS_DIAG(gNdsLabItemAcc[6] += lab_now - lab_item_mark);
            lab_item_mark = lab_now;
        }
#endif
        if (handled != FALSE)
        {
            (*item->draws)++;
        }
        else
        {
            (*item->fails)++;
        }
    }
    if (handled == FALSE)
    {
        /* The body's verdict for a drawn-nothing owner. */
        ndsStageRejectNativeRender(dobj, dl, NDS_NATIVE_FAILURE_NO_PROGRAM,
                                   render_stats);
    }
    ndsTaskmanSampleGraphicsHeap();
    gSYTaskmanGraphicsHeap.ptr = saved_graphics_heap_ptr;
    NDS_DIAG(gNdsStageGCDrawAllLoopHardwareCarryCaptureCount++);
    gNdsStageGCDrawAllLoopHardwareTriangleCount +=
        render_stats->hardware_triangle_count;
    gNdsStageGCDrawAllLoopHardwareZBufferTriangleCount +=
        render_stats->hardware_zbuffer_triangle_count;
    gNdsStageGCDrawAllLoopHardwareProjectedDepthTriangleCount +=
        render_stats->hardware_projected_depth_triangle_count;
    gNdsStageGCDrawAllLoopHardwareDecalDepthTriangleCount +=
        render_stats->hardware_decal_depth_triangle_count;
    gNdsStageGCDrawAllLoopHardwareTextureBindCount +=
        render_stats->hardware_texture_bind_count;
    gNdsStageGCDrawAllLoopHardwareTextureUploadCount +=
        render_stats->hardware_texture_upload_count;
    gNdsStageGCDrawAllLoopHardwareTextureReadyCount +=
        render_stats->hardware_texture_ready_count;
    gNdsStageGCDrawAllLoopHardwareTextureRejectCount +=
        render_stats->hardware_texture_reject_count;
    if (render_stats->hardware_texture_ready_count != 0u)
    {
        if (render_stats->hardware_texture_format < 32u)
        {
            gNdsStageGCDrawAllLoopHardwareTextureFormatMask |=
                1u << render_stats->hardware_texture_format;
        }
        if (render_stats->hardware_texture_width >
            gNdsStageGCDrawAllLoopHardwareTextureMaxWidth)
        {
            gNdsStageGCDrawAllLoopHardwareTextureMaxWidth =
                render_stats->hardware_texture_width;
        }
        if (render_stats->hardware_texture_height >
            gNdsStageGCDrawAllLoopHardwareTextureMaxHeight)
        {
            gNdsStageGCDrawAllLoopHardwareTextureMaxHeight =
                render_stats->hardware_texture_height;
        }
    }
    NDS_DIAG(gNdsStageDLFastLaneHits++);
#if defined(NDS_LAB_FOURCPU_SWEEP) && NDS_LAB_FOURCPU_SWEEP
    if (lab_item != FALSE)
    {
        NDS_DIAG(gNdsLabItemAcc[7] += cpuGetTiming() - lab_item_mark);
    }
#endif
    return TRUE;
}

/* P2-2p8 (2026-10-04): the fast lane runs on the DTCM hot stack
 * (port/coroutine.h). Its owners' 3,000-byte traversal states and the
 * lane's own frames lived on the main-RAM stack, where the late-match lab
 * profile charged stack-line refills to the lane's pops and frame loads
 * (the lane ~10 cycles an instruction). Static reach from the lane is
 * 5,728 B at most (a baked owner's texture allocation through
 * glTexImage2D); IRQs run on their own stack. The lane reads no storage,
 * switches no coroutine and hands DMA only static buffers (texture scratch,
 * packet words). A lane reached from a subtree already on the hot stack
 * (the fighter display's magnifier) runs in place, as before. */

typedef struct NDSStageDLFastCall
{
    DObj *dobj;
    const Gfx *dl;
    GObj *camera_gobj;
    u32 initial_geometry_mode;
} NDSStageDLFastCall;

static unsigned int ndsRendererAdapterSubmitStageDLFastOnHotStack(void *arg)
{
    const NDSStageDLFastCall *call = (const NDSStageDLFastCall *)arg;

    return (unsigned int)ndsRendererAdapterSubmitStageDLFast(
        call->dobj, call->dl, call->camera_gobj,
        call->initial_geometry_mode);
}

/* The entry-effect owners the lane does not route (entry packets, the
 * rebirth halo's groups) take the same stack. */

static unsigned int ndsRendererAdapterTryNativeEntryEffectOnHotStack(void *arg)
{
    const NDSStageDLFastCall *call = (const NDSStageDLFastCall *)arg;

    return (unsigned int)ndsRendererAdapterTryNativeEntryEffect(
        call->dobj, call->dl, call->camera_gobj,
        call->initial_geometry_mode);
}

/* P2-2p8 (2026-10-05): the RebirthHalo arm ahead of the body. It sat at the
 * top of ndsRendererAdapterSubmitStageDLBody, so its three lists (two on the
 * child DObj, one on the rotating grandchild) each paid the entry-effect
 * probe and the body's prologue -- a ~29 KB function's frame and locals --
 * before arriving at the native owner: ~100-160 frames a match after every
 * respawn, on every stage. The same statements now run from the dispatcher
 * right after the fast lane; a list they decline goes on to the entry probe
 * and the body as before (whose own copy of the arm stands down). Same-ROM
 * A/B word gNdsStageDLHaloFirst (0 = the arm in the body). */
volatile u32 gNdsStageDLHaloFirst __attribute__((used, section(".data"))) = 1u;

static sb32 __attribute__((noinline)) ndsRendererAdapterTryRebirthHalo(
    DObj *dobj, const Gfx *dl, GObj *camera_gobj, u32 initial_geometry_mode)
{
    if ((sNdsRendererAdapterRebirthHaloNativeActive != FALSE) &&
        (gEFManagerFiles[2] != NULL) &&
        ((const u8 *)dl >= (const u8 *)gEFManagerFiles[2]))
    {
        uintptr_t rebirth_offset = (uintptr_t)((const u8 *)dl -
                                               (const u8 *)gEFManagerFiles[2]);

        if ((rebirth_offset == 0x2378u) || (rebirth_offset == 0x2a88u) ||
            (rebirth_offset == 0x27e8u))
        {
            NDSRendererConfig rebirth_config = {0};
            NDSRendererStats rebirth_stats;
            NDSRendererStats *rebirth_render_stats;
            NDSRendererMatrix20p12 rebirth_projection;
            NDSRendererMatrix20p12 rebirth_modelview;
            const NDSRendererMatrix20p12 *rebirth_projection_ptr;
            const NDSRendererMatrix20p12 *rebirth_modelview_ptr;
            void *rebirth_saved_graphics_heap_ptr = gSYTaskmanGraphicsHeap.ptr;

            /* 0x2378 and 0x2a88 are the two DL links on the SAME child DObj.
             * Once the first one has emitted both native roots with one matrix
             * setup, the tree walker will immediately offer 0x2a88 again. */
            if ((rebirth_offset == 0x2a88u) &&
                (sNdsRendererAdapterRebirthHaloSkipSecondChildList != FALSE))
            {
                sNdsRendererAdapterRebirthHaloSkipSecondChildList = FALSE;
                return TRUE;
            }

            ndsRendererAdapterPrepareInitialMatrices(
                dobj,
                (camera_gobj != NULL) ? CObjGetStruct(camera_gobj) :
                    ((gGCCurrentCamera != NULL) ? CObjGetStruct(gGCCurrentCamera) : NULL),
                TRUE,
                &rebirth_projection,
                &rebirth_projection_ptr,
                &rebirth_modelview,
                &rebirth_modelview_ptr);

            if (sNdsRendererAdapterStagePersistentActive != FALSE)
            {
                rebirth_render_stats = &sNdsRendererAdapterStagePersistentStats;
                ndsFighterDLDrawResetRuntimeRendererStats(rebirth_render_stats);
            }
            else
            {
                rebirth_render_stats = &rebirth_stats;
                ndsRendererInitStats(rebirth_render_stats);
            }
            if ((sNdsRendererAdapterEffectColorMask & 1u) != 0u)
            {
                rebirth_render_stats->prim_color = sNdsRendererAdapterEffectPrimColor;
            }
            if ((sNdsRendererAdapterEffectColorMask & 2u) != 0u)
            {
                rebirth_render_stats->env_color = sNdsRendererAdapterEffectEnvColor;
            }
            if (sNdsRendererAdapterEffectOtherModeValid != 0u)
            {
                rebirth_render_stats->othermode_l = sNdsRendererAdapterEffectOtherModeL;
            }

            rebirth_config.max_depth = 8u;
            rebirth_config.max_commands = 8192u;
            rebirth_config.max_list_commands = 512u;
            rebirth_config.initial_projection = rebirth_projection_ptr;
            rebirth_config.initial_modelview = rebirth_modelview_ptr;
            rebirth_config.initial_geometry_mode = initial_geometry_mode;
            rebirth_config.texture_data_layout = NDS_RENDERER_TEXTURE_DATA_O2R_WORD_SWAPPED;

            if (ndsRendererSubmitNativeRebirthHalo(
                    (u32)rebirth_offset, &rebirth_config,
                    rebirth_render_stats) != FALSE)
            {
                NDS_DIAG(gNdsRebirthHaloNativeDrawCount++);
                if (rebirth_offset == 0x2378u)
                {
                    /* Same DObj, same source matrix, adjacent source order.
                     * Keep the live renderer state produced by 0x2378 and emit
                     * its second linked list without rebuilding the adapter. */
                    if (ndsRendererSubmitNativeRebirthHalo(
                            0x2a88u, &rebirth_config,
                            rebirth_render_stats) != FALSE)
                    {
                        NDS_DIAG(gNdsRebirthHaloNativeDrawCount++);
                        sNdsRendererAdapterRebirthHaloSkipSecondChildList = TRUE;
                    }
                    else
                    {
                        NDS_DIAG(gNdsRebirthHaloNativeFallbackCount++);
                    }
                }
                gNdsStageGCDrawAllLoopHardwareTriangleCount +=
                    rebirth_render_stats->hardware_triangle_count;
                gNdsStageGCDrawAllLoopHardwareZBufferTriangleCount +=
                    rebirth_render_stats->hardware_zbuffer_triangle_count;
                gNdsStageGCDrawAllLoopHardwareProjectedDepthTriangleCount +=
                    rebirth_render_stats->hardware_projected_depth_triangle_count;
                gNdsStageGCDrawAllLoopHardwareDecalDepthTriangleCount +=
                    rebirth_render_stats->hardware_decal_depth_triangle_count;
                gNdsStageGCDrawAllLoopHardwareTextureBindCount +=
                    rebirth_render_stats->hardware_texture_bind_count;
                gNdsStageGCDrawAllLoopHardwareTextureUploadCount +=
                    rebirth_render_stats->hardware_texture_upload_count;
                gNdsStageGCDrawAllLoopHardwareTextureReadyCount +=
                    rebirth_render_stats->hardware_texture_ready_count;
                gNdsStageGCDrawAllLoopHardwareTextureRejectCount +=
                    rebirth_render_stats->hardware_texture_reject_count;
                ndsTaskmanSampleGraphicsHeap();
                gSYTaskmanGraphicsHeap.ptr = rebirth_saved_graphics_heap_ptr;
                return TRUE;
            }
            NDS_DIAG(gNdsRebirthHaloNativeFallbackCount++);
            ndsTaskmanSampleGraphicsHeap();
            gSYTaskmanGraphicsHeap.ptr = rebirth_saved_graphics_heap_ptr;
        }
    }
    return FALSE;
}

volatile u32 gNdsStageDLBodyCalls;

static void ndsRendererAdapterSubmitStageDLImpl(DObj *dobj, const Gfx *dl,
                                                 GObj *camera_gobj,
                                                 u32 initial_geometry_mode)
{
    if ((dobj == NULL) || (dl == NULL))
    {
        return;
    }
    {
        NDSStageDLFastCall call = {
            dobj, dl, camera_gobj, initial_geometry_mode
        };

        if ((sb32)ndsDtcmHotStackRun(
                ndsRendererAdapterSubmitStageDLFastOnHotStack, &call) != FALSE)
        {
            return;
        }
    }
    if ((gNdsStageDLHaloFirst != 0u) &&
        (sNdsRendererAdapterRebirthHaloNativeActive != FALSE) &&
        (ndsRendererAdapterTryRebirthHalo(dobj, dl, camera_gobj,
                                          initial_geometry_mode) != FALSE))
    {
        return;
    }
    {
        sb32 entry_handled;
        NDSStageDLFastCall call = {
            dobj, dl, camera_gobj, initial_geometry_mode
        };

        entry_handled = (sb32)ndsDtcmHotStackRun(
            ndsRendererAdapterTryNativeEntryEffectOnHotStack, &call);

        if (entry_handled != FALSE)
        {
            return;
        }
    }
    NDS_DIAG(gNdsStageDLBodyCalls++);
    ndsRendererAdapterSubmitStageDLBody(dobj, dl, camera_gobj,
                                        initial_geometry_mode);
}
