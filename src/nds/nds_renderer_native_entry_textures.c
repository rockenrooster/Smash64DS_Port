/* Native renderer, entry effects: texture preparation -- the shield and KO
 * textures and palettes, the late-texture marks and the entry texture set,
 * prepared before GO and for gameplay. A slice of nds_renderer.c. */

/* Setup-only, with the texture set below: compact, not fast. */
NDS_RENDERER_COLD static s32 __attribute__((noinline, cold, optimize("Os")))
ndsRendererPrepareEntryShieldTextures(const NDSEntryEffectTexture *texture)
{
    u32 variant;
    /* A3I5, thirty-two entries.  The blend below runs once per PALETTE ENTRY,
     * so the entry count is exactly how many distinct colours the shield can
     * show: at A5I3's eight, the source's radial ramp collapsed to about two
     * tones over its used region and read as concentric bands.  Fail closed on
     * anything else rather than banding quietly. */
    if ((texture->ds_format != NDS_ENTRY_EFFECT_TEXTURE_A3I5) ||
        (texture->palette_entries != 32u) || (texture->palette == NULL))
    {
        return FALSE;
    }
    for (variant = 0u; variant < 5u; variant++)
    {
        u16 palette[32];
        u32 color;
        if (sNdsEntryShieldTextureName[variant] != 0u) { continue; }
        for (color = 0u; color < 32u; color++)
        {
            palette[color] = ndsRendererHardwareBlendPrimEnvTexel0(
                texture->palette[color], 0xffffffc0u,
                sNdsEntryShieldEnvironment[variant]);
        }
        if (ndsRendererHardwarePrepareIFCommonA3I5Atlas(
                texture->width, texture->height, palette,
                ndsRendererEntryEffectTextureFill, (void *)texture,
                &sNdsEntryShieldTextureName[variant]) == FALSE)
        {
            return FALSE;
        }
        NDS_DIAG(gNdsEntryEffectNativeShieldPrepareCount++);
    }
    return TRUE;
}

/* Required setup failures must stay observable even when the caller has no
 * shield GL name to draw. Keep this symbol alive for the diagnostic GDB
 * probe: --gc-sections has discarded unreferenced witnesses in this tree. */
__attribute__((used)) volatile u32 gNdsEntryShieldTexturePrepareDeclineCount;

NDS_RENDERER_COLD static s32 __attribute__((noinline, cold, optimize("Os")))
ndsRendererPrepareEntryKoPalettes(void)
{
    u32 part;
    for (part = 0u; part < 3u; part++)
    {
        const NDSEntryEffectRoot *root = ndsRendererEntryEffectRoot(
            84u, sNdsEntryKoRootOffsets[part]);
        const NDSEntryEffectTexture *texture;
        u32 texture_slot;
        u32 player;
        if ((root == NULL) || (root->group_count == 0u)) { return FALSE; }
        texture_slot = sNdsEntryEffectGroups[root->first_group].texture_slot;
        if (texture_slot >= NDS_ENTRY_EFFECT_TEXTURE_COUNT) { return FALSE; }
        texture = &sNdsEntryEffectTextures[texture_slot];
        if ((texture->ds_format != NDS_ENTRY_EFFECT_TEXTURE_A5I3) ||
            (texture->palette_entries != 8u) ||
            (sNdsRendererEntryEffectTextureName[texture_slot] == 0u)) { return FALSE; }
        for (player = 0u; player < 4u; player++)
        {
            int name = 0;
            int palette_width = 0;
            u32 color;
            if (sNdsEntryKoPaletteName[part][player] != 0u) { continue; }
            for (color = 0u; color < 8u; color++)
            {
                sNdsEntryKoPaletteScratch[color] = ndsRendererHardwareBlendPrimEnvTexel0(
                    texture->palette[color], sNdsEntryKoPrimRgb[part][player],
                    sNdsEntryKoEnvRgb[part][player]);
            }
            if (ndsRendererHardwareFencedGlGenTextures(1, &name) == 0) { return FALSE; }
            ndsRendererHardwareBindTextureState((u32)name);
            /* Palette-only GL names share the real KO image through
             * glAssignColorTable. No duplicate image or live palette writes. */
            glColorTableEXT(GL_TEXTURE_2D, 0, 8, 0, 0, sNdsEntryKoPaletteScratch);
            glGetColorTableParameterEXT(GL_TEXTURE_2D, GL_COLOR_TABLE_WIDTH_EXT,
                                       &palette_width);
            if (palette_width != 8)
            {
                ndsRendererHardwareFencedGlDeleteTextures(1, &name);
                sNdsRendererHardwareBoundTextureName = 0u;
                return FALSE;
            }
            sNdsEntryKoPaletteName[part][player] = (u32)name;
        }
        /* Drop the unused grayscale palette during setup. During drawing,
         * every switch then changes references between retained palettes. */
        ndsRendererHardwareBindTextureState(sNdsRendererEntryEffectTextureName[texture_slot]);
        glAssignColorTable(GL_TEXTURE_2D, (int)sNdsEntryKoPaletteName[part][0]);
    }
    return TRUE;
}

volatile u32 gNdsEntryEffectNativeTexturePrepareFailCount;

/* EACH TEXTURE FAILS ALONE. The first refused texture used to end the setup,
 * and the KO palettes come last: four Dededes on Dream Land ran out of texture
 * VRAM at the 18th of his 19 article images, the KO palettes were never made,
 * and every KO effect of the match failed natively. A refused image now
 * leaves only its own groups unbound, and the KO palettes need only the KO
 * image.
 *
 * Two phases: the images an entry may draw first, then the late ones (below),
 * which can first appear only after GO; at scene start they take what the
 * entries leave. Four different contents on Dream Land (Crash, Lanky, Sonic,
 * Dedede) refused 30 images, two of them Sonic's entry props, in index order.
 *
 * skip_startup is the second pass, after the startup-only names retire at
 * GO: it places the gameplay images the scene-start pass had no room for,
 * as the dust planes do, and never re-makes the retired entry props. */
#define NDS_ENTRY_TEXTURE_SKIP 3u

/* late[i]: 0, an entry may draw image i; 1, no entry draws it (the contents'
 * articles, and images only the original cast's gameplay roots name: Link's
 * spin and boomerang, shield, reflector, catch, KO and reflector break, up to
 * the Master Ball rays; Kirby's cutter, Samus's grapple, Falcon's kick and
 * punch, up to Yoshi's egg, which is also his entry); SKIP, nothing of this
 * scene can draw it: an absent content's image, or one only the original
 * cast's entry props name whose special files the scene never loaded (18.5 KB
 * in an all-P4 match; FoxSpecial3 also loads for Sector Z's fly-by).
 *
 * The same holds for the gameplay roots one fighter's file owns (owner
 * 2026-10-23: a match loads only the textures it can need): Link's spin
 * (LinkModel) and boomerang (LinkSpecial3), Fox's reflector (FoxSpecial2),
 * Kirby's cutter and entry star (KirbySpecial2) and cutter wave (KirbyModel),
 * Samus's grapple glow (SamusSpecial2), the Falcon Kick (CaptainSpecial2) and
 * Punch (CaptainSpecial3) and Yoshi's egg (YoshiModel). Their effects read
 * the file (efManagerMakeEffect), so with it not resident none can exist:
 * 14,016 B of images in a match none of those six plays. A file a Kirby's
 * copy needs comes in his own file closure, so it is resident with him. */
NDS_RENDERER_COLD static void ndsRendererEntryTextureMarkLate(u8 *late)
{
    static const u8 entry_end[6] = {
        NDS_ENTRY_EFFECT_FOX_ROOT_FIRST, NDS_ENTRY_EFFECT_DONKEY_ROOT_FIRST,
        NDS_ENTRY_EFFECT_SAMUS_ROOT_FIRST, NDS_ENTRY_EFFECT_CAPTAIN_ROOT_FIRST,
        NDS_ENTRY_EFFECT_LINK_ROOT_FIRST,
        NDS_ENTRY_EFFECT_LINK_SPIN_WEAPON_ROOT_FIRST
    };
    static const u16 entry_asset[6] = { 356u, 161u, 355u, 349u, 350u, 353u };
    static const u8 owner_first[9] = {
        NDS_ENTRY_EFFECT_LINK_SPIN_WEAPON_ROOT_FIRST,
        NDS_ENTRY_EFFECT_LINK_BOOMERANG_ROOT_FIRST,
        NDS_ENTRY_EFFECT_REFLECTOR_ROOT_FIRST,
        NDS_ENTRY_EFFECT_KIRBY_CUTTER_ROOT_FIRST,
        NDS_ENTRY_EFFECT_KIRBY_CUTTER_WEAPON_ROOT_FIRST,
        NDS_ENTRY_EFFECT_SAMUS_GRAPPLE_ROOT_FIRST,
        NDS_ENTRY_EFFECT_FALCON_KICK_ROOT_FIRST,
        NDS_ENTRY_EFFECT_FALCON_PUNCH_ROOT_FIRST,
        NDS_ENTRY_EFFECT_YOSHI_EGG_ROOT_FIRST
    };
    static const u8 owner_end[9] = {
        NDS_ENTRY_EFFECT_LINK_BOOMERANG_ROOT_FIRST,
        NDS_ENTRY_EFFECT_SHIELD_ROOT_FIRST,
        NDS_ENTRY_EFFECT_REFLECTOR_ROOT_FIRST +
            NDS_ENTRY_EFFECT_REFLECTOR_ROOT_COUNT,
        NDS_ENTRY_EFFECT_KIRBY_CUTTER_ROOT_FIRST +
            NDS_ENTRY_EFFECT_KIRBY_CUTTER_ROOT_COUNT,
        NDS_ENTRY_EFFECT_KIRBY_CUTTER_WEAPON_ROOT_FIRST +
            NDS_ENTRY_EFFECT_KIRBY_CUTTER_WEAPON_ROOT_COUNT,
        NDS_ENTRY_EFFECT_SAMUS_GRAPPLE_ROOT_FIRST +
            NDS_ENTRY_EFFECT_SAMUS_GRAPPLE_ROOT_COUNT,
        NDS_ENTRY_EFFECT_FALCON_KICK_ROOT_FIRST +
            NDS_ENTRY_EFFECT_FALCON_KICK_ROOT_COUNT,
        NDS_ENTRY_EFFECT_FALCON_PUNCH_ROOT_FIRST +
            NDS_ENTRY_EFFECT_FALCON_PUNCH_ROOT_COUNT,
        NDS_ENTRY_EFFECT_YOSHI_EGG_ROOT_FIRST +
            NDS_ENTRY_EFFECT_YOSHI_EGG_ROOT_COUNT
    };
    static const u16 owner_asset[9] = {
        324u, 325u, 346u, 348u, 328u, 349u, 350u, 333u, 338u
    };
    u32 loaded = 0u;
    u32 owned = 0u;
    u32 g;
    u32 i;

    for (i = 0u; i < 6u; i++)
    {
        const void *data;
        u32 size;

        if (ndsRelocGetLoadedAssetView(entry_asset[i], &data, &size) != FALSE)
        {
            loaded |= 1u << i;
        }
    }
    for (i = 0u; i < 9u; i++)
    {
        const void *data;
        u32 size;

        if (ndsRelocGetLoadedAssetView(owner_asset[i], &data, &size) != FALSE)
        {
            owned |= 1u << i;
        }
    }
    /* Bits a root sets: 1 early, 2 gameplay, 4 an entry prop not loaded. */
    for (i = 0u; i < NDS_ENTRY_EFFECT_TEXTURE_COUNT; i++)
    {
        late[i] = 0u;
    }
    for (g = 0u; g < NDS_ENTRY_EFFECT_GROUP_COUNT; g++)
    {
        u32 slot = sNdsEntryEffectGroups[g].texture_slot;
        u32 root = sNdsEntryEffectGroups[g].root_index;
        u32 f;

        if (slot >= NDS_ENTRY_EFFECT_TEXTURE_COUNT)
        {
            continue;
        }
        for (f = 0u; (f < 6u) && (root >= entry_end[f]); f++)
        {
        }
        if (f < 6u)
        {
            late[slot] |= (((loaded >> f) & 1u) != 0u) ? 1u : 4u;
        }
        else
        {
            for (f = 0u; (f < 9u) &&
                         ((root < owner_first[f]) || (root >= owner_end[f]));
                 f++)
            {
            }
            if ((f < 9u) && (((owned >> f) & 1u) == 0u))
            {
                late[slot] |= 4u;
            }
            /* Kirby's entry warp star is the cutter roots' last (the
             * generator appends it so the cutters' ordinals stay): an entry
             * image, wanted before GO like the six entries' above. As a
             * gameplay image it came after every entry image, and since the
             * stage's images go first (S6) Hyrule Castle had no room left
             * for it: Dedede, who enters on Kirby's star, entered without it
             * beside Bowser, Crash and Ike (NO_PROGRAM, then REJECTED, 30
             * frames each). */
            else if ((root != NDS_ENTRY_EFFECT_KIRBY_CUTTER_ROOT_FIRST +
                                  NDS_ENTRY_EFFECT_KIRBY_CUTTER_ROOT_COUNT - 1u) &&
                     (((root >= NDS_ENTRY_EFFECT_LINK_SPIN_WEAPON_ROOT_FIRST) &&
                       (root < NDS_ENTRY_EFFECT_MBALLRAYS_ROOT_FIRST)) ||
                      ((root >= NDS_ENTRY_EFFECT_KIRBY_CUTTER_ROOT_FIRST) &&
                       (root < NDS_ENTRY_EFFECT_YOSHI_EGG_ROOT_FIRST))))
            {
                late[slot] |= 2u;
            }
            else
            {
                late[slot] |= 1u;
            }
        }
    }
    for (i = 0u; i < NDS_ENTRY_EFFECT_TEXTURE_COUNT; i++)
    {
        u32 bits = late[i];

        late[i] = (((bits & 1u) != 0u) || (bits == 0u)) ? 0u :
                  ((bits & 2u) != 0u) ? 1u : NDS_ENTRY_TEXTURE_SKIP;
#ifdef NDS_ENTRY_EFFECT_P4_ROOT_COUNT
        if (sNdsEntryEffectTextureP4Content[i] != 0u)
        {
            late[i] = (ndsRendererEntryEffectP4ContentInMatch(
                           sNdsEntryEffectTextureP4Content[i]) == FALSE) ?
                NDS_ENTRY_TEXTURE_SKIP :
                (sNdsEntryEffectTextureStartupOnly[i] == 0u) ? 1u : 0u;
        }
#endif
    }
}

NDS_RENDERER_COLD static s32 __attribute__((noinline, cold, optimize("Os")))
ndsRendererPrepareEntryEffectTextureSet(u32 skip_startup)
{
    u8 late[NDS_ENTRY_EFFECT_TEXTURE_COUNT];
    u32 k;
    s32 ok = TRUE;

    ndsRendererEntryTextureMarkLate(late);
    for (k = 0u; k < (2u * NDS_ENTRY_EFFECT_TEXTURE_COUNT); k++)
    {
        u32 i = k % NDS_ENTRY_EFFECT_TEXTURE_COUNT;
        const NDSEntryEffectTexture *texture = &sNdsEntryEffectTextures[i];
        s32 prepared;

        if ((sNdsRendererEntryEffectTextureName[i] != 0u) ||
            ((skip_startup != 0u) && (sNdsEntryEffectTextureStartupOnly[i] != 0u)) ||
            (late[i] != (k / NDS_ENTRY_EFFECT_TEXTURE_COUNT)))
        {
            continue;
        }
        if ((texture->texels == NULL) ||
            (texture->width == 0u) || (texture->height == 0u))
        {
            ok = FALSE;
            continue;
        }
        if (i == sNdsEntryEffectGroups[
                sNdsEntryEffectRoots[NDS_ENTRY_EFFECT_SHIELD_ROOT_FIRST].first_group].texture_slot)
        {
            if (ndsRendererPrepareEntryShieldTextures(texture) == FALSE)
            {
                NDS_DIAG(gNdsEntryShieldTexturePrepareDeclineCount++);
                ok = FALSE;
            }
            continue;
        }
        if (texture->ds_format == NDS_ENTRY_EFFECT_TEXTURE_PAL16)
        {
            if (texture->palette_entries != 16u)
            {
                ok = FALSE;
                continue;
            }
            /* Unlike IFCommon's PAL16 atlases, an entry texture does not
             * necessarily reserve index 0 for transparency. Mario's pipe uses
             * index 0 as opaque green in both CI4 images; forcing COLOR0 there
             * removes whole texel rows from otherwise valid polygons. */
            prepared = ndsRendererHardwarePrepareIFCommonAtlas(
                texture->width, texture->height, GL_RGB16,
                texture->palette, 16u,
                ndsRendererEntryEffectTextureFill, (void *)texture,
                &sNdsRendererEntryEffectTextureName[i],
                texture->color0_transparent);
        }
        else if (texture->ds_format == NDS_ENTRY_EFFECT_TEXTURE_A5I3)
        {
            if (texture->palette_entries != 8u)
            {
                ok = FALSE;
                continue;
            }
            prepared = ndsRendererHardwarePrepareIFCommonCloudAtlas(
                texture->width, texture->height, texture->palette,
                ndsRendererEntryEffectTextureFill, (void *)texture,
                &sNdsRendererEntryEffectTextureName[i]);
        }
        else if (texture->ds_format == NDS_ENTRY_EFFECT_TEXTURE_RGBA)
        {
            if ((texture->palette != NULL) || (texture->palette_entries != 0u))
            {
                ok = FALSE;
                continue;
            }
            prepared = ndsRendererHardwarePrepareIFCommonAtlas(
                texture->width, texture->height, GL_RGBA,
                NULL, 0u,
                ndsRendererEntryEffectTextureFill, (void *)texture,
                &sNdsRendererEntryEffectTextureName[i], FALSE);
        }
        else
        {
            ok = FALSE;
            continue;
        }
        if (prepared == FALSE)
        {
            gNdsEntryEffectNativeTexturePrepareFailCount++;
            ok = FALSE;
            continue;
        }
        gNdsEntryEffectNativeTexturePrepareCount++;
    }
    ndsRendererEntryTexelsClose();
    if (ndsRendererPrepareEntryKoPalettes() == FALSE)
    {
        ok = FALSE;
    }
    return ok;
}

NDS_RENDERER_COLD s32 ndsRendererHardwarePrepareEntryEffectTextures(void)
{
#ifdef NDS_ENTRY_EFFECT_P4_CORNER_BLOCKS
    ndsRendererEntryP4CornersLoad();
#endif
    return ndsRendererPrepareEntryEffectTextureSet(0u);
}

NDS_RENDERER_COLD s32 ndsRendererHardwarePrepareEntryEffectGameplayTextures(void)
{
    return ndsRendererPrepareEntryEffectTextureSet(1u);
}
