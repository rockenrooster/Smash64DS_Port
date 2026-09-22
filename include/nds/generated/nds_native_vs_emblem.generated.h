/* VS Results / Characters winner-emblem series roots (generated).
 * Do not hand-edit; regenerate with
 * scripts/menus/generate_nds_native_vs_emblem.py.
 *
 * Source tables: mnvsresults.c:622-648 (mnVSResultsMakeEmblem) and
 * mncharacters.c:1500-1526 (mnCharactersMakeEmblem) select one of these
 * roots per fighter kind; relocData/35_FTEmblemModels.c holds them.
 * Twelve kinds, TEN distinct roots: Mario/Luigi share the Mario entry
 * and Pikachu/Purin share the PMonsters entry.
 *
 * Recognition data only -- no geometry is baked here.  The native owner
 * reads the source Vtx/Gfx out of the loaded reloc file. */
#ifndef NDS_NATIVE_VS_EMBLEM_GENERATED_H
#define NDS_NATIVE_VS_EMBLEM_GENERATED_H

#define NDS_NATIVE_VS_EMBLEM_ASSET 0x23u
#define NDS_NATIVE_VS_EMBLEM_SERIES_COUNT 10u
#define NDS_NATIVE_VS_EMBLEM_FKIND_COUNT 12u
/* gcSetupCommonDObjs builds this many DObjs per emblem tree: an
 * origin-free root that positions the series and one child that owns
 * the display list. */
#define NDS_NATIVE_VS_EMBLEM_DOBJ_COUNT 2u
/* mnVSResultsMakeEmblem:662,665 -- GObj id and Results DL link. */
#define NDS_NATIVE_VS_EMBLEM_RESULTS_GOBJ_ID 23u
#define NDS_NATIVE_VS_EMBLEM_RESULTS_DL_LINK 33u
/* mnCharactersMakeEmblem:1529,1531 -- the second consumer of these
 * same ten roots.  Different scene, different GObj id and link; the
 * root table below is shared and must stay scene-independent. */
#define NDS_NATIVE_VS_EMBLEM_CHARACTERS_GOBJ_ID 19u
#define NDS_NATIVE_VS_EMBLEM_CHARACTERS_DL_LINK 28u

/* mnvsresults.c:649 `s32 colors[] = { 0, 1, 3 };` -- winning team to
 * material-animation colour index. */
#define NDS_NATIVE_VS_EMBLEM_TEAM_COLOR_COUNT 3u
#define NDS_NATIVE_VS_EMBLEM_TEAM_COLORS { 0u, 1u, 3u }

/* [0] Mario: 96 vertices, 528 display-list bytes. */
#define NDS_NATIVE_VS_EMBLEM_MARIO_INDEX 0u
#define NDS_NATIVE_VS_EMBLEM_MARIO_ROOT 0x0780u
#define NDS_NATIVE_VS_EMBLEM_MARIO_DL_BYTES 528u
#define NDS_NATIVE_VS_EMBLEM_MARIO_VTX_OFFSET 0x0180u
#define NDS_NATIVE_VS_EMBLEM_MARIO_VERTEX_COUNT 96u
#define NDS_NATIVE_VS_EMBLEM_MARIO_DESC_OFFSET 0x0990u
#define NDS_NATIVE_VS_EMBLEM_MARIO_MOBJSUB_OFFSET 0x0000u
#define NDS_NATIVE_VS_EMBLEM_MARIO_MATANIM_OFFSET 0x0a14u

/* [1] Fox: 97 vertices, 504 display-list bytes. */
#define NDS_NATIVE_VS_EMBLEM_FOX_INDEX 1u
#define NDS_NATIVE_VS_EMBLEM_FOX_ROOT 0x1fd8u
#define NDS_NATIVE_VS_EMBLEM_FOX_DL_BYTES 504u
#define NDS_NATIVE_VS_EMBLEM_FOX_VTX_OFFSET 0x19c8u
#define NDS_NATIVE_VS_EMBLEM_FOX_VERTEX_COUNT 97u
#define NDS_NATIVE_VS_EMBLEM_FOX_DESC_OFFSET 0x21d0u
#define NDS_NATIVE_VS_EMBLEM_FOX_MOBJSUB_OFFSET 0x1940u
#define NDS_NATIVE_VS_EMBLEM_FOX_MATANIM_OFFSET 0x2254u

/* [2] Donkey: 87 vertices, 464 display-list bytes. */
#define NDS_NATIVE_VS_EMBLEM_DONKEY_INDEX 2u
#define NDS_NATIVE_VS_EMBLEM_DONKEY_ROOT 0x1178u
#define NDS_NATIVE_VS_EMBLEM_DONKEY_DL_BYTES 464u
#define NDS_NATIVE_VS_EMBLEM_DONKEY_VTX_OFFSET 0x0c08u
#define NDS_NATIVE_VS_EMBLEM_DONKEY_VERTEX_COUNT 87u
#define NDS_NATIVE_VS_EMBLEM_DONKEY_DESC_OFFSET 0x1348u
#define NDS_NATIVE_VS_EMBLEM_DONKEY_MOBJSUB_OFFSET 0x0b00u
#define NDS_NATIVE_VS_EMBLEM_DONKEY_MATANIM_OFFSET 0x13ccu

/* [3] Metroid: 40 vertices, 232 display-list bytes. */
#define NDS_NATIVE_VS_EMBLEM_METROID_INDEX 3u
#define NDS_NATIVE_VS_EMBLEM_METROID_ROOT 0x1778u
#define NDS_NATIVE_VS_EMBLEM_METROID_DL_BYTES 232u
#define NDS_NATIVE_VS_EMBLEM_METROID_VTX_OFFSET 0x14f8u
#define NDS_NATIVE_VS_EMBLEM_METROID_VERTEX_COUNT 40u
#define NDS_NATIVE_VS_EMBLEM_METROID_DESC_OFFSET 0x1860u
#define NDS_NATIVE_VS_EMBLEM_METROID_MOBJSUB_OFFSET 0x1470u
#define NDS_NATIVE_VS_EMBLEM_METROID_MATANIM_OFFSET 0x18e4u

/* [4] Zelda: 6 vertices, 144 display-list bytes. */
#define NDS_NATIVE_VS_EMBLEM_ZELDA_INDEX 4u
#define NDS_NATIVE_VS_EMBLEM_ZELDA_ROOT 0x2490u
#define NDS_NATIVE_VS_EMBLEM_ZELDA_DL_BYTES 144u
#define NDS_NATIVE_VS_EMBLEM_ZELDA_VTX_OFFSET 0x2430u
#define NDS_NATIVE_VS_EMBLEM_ZELDA_VERTEX_COUNT 6u
#define NDS_NATIVE_VS_EMBLEM_ZELDA_DESC_OFFSET 0x2520u
#define NDS_NATIVE_VS_EMBLEM_ZELDA_MOBJSUB_OFFSET 0x22b0u
#define NDS_NATIVE_VS_EMBLEM_ZELDA_MATANIM_OFFSET 0x25a4u

/* [5] Yoshi: 95 vertices, 520 display-list bytes. */
#define NDS_NATIVE_VS_EMBLEM_YOSHI_INDEX 5u
#define NDS_NATIVE_VS_EMBLEM_YOSHI_ROOT 0x2d08u
#define NDS_NATIVE_VS_EMBLEM_YOSHI_DL_BYTES 520u
#define NDS_NATIVE_VS_EMBLEM_YOSHI_VTX_OFFSET 0x2718u
#define NDS_NATIVE_VS_EMBLEM_YOSHI_VERTEX_COUNT 95u
#define NDS_NATIVE_VS_EMBLEM_YOSHI_DESC_OFFSET 0x2f10u
#define NDS_NATIVE_VS_EMBLEM_YOSHI_MOBJSUB_OFFSET 0x2690u
#define NDS_NATIVE_VS_EMBLEM_YOSHI_MATANIM_OFFSET 0x2f94u

/* [6] FZero: 91 vertices, 512 display-list bytes. */
#define NDS_NATIVE_VS_EMBLEM_FZERO_INDEX 6u
#define NDS_NATIVE_VS_EMBLEM_FZERO_ROOT 0x3628u
#define NDS_NATIVE_VS_EMBLEM_FZERO_DL_BYTES 512u
#define NDS_NATIVE_VS_EMBLEM_FZERO_VTX_OFFSET 0x3078u
#define NDS_NATIVE_VS_EMBLEM_FZERO_VERTEX_COUNT 91u
#define NDS_NATIVE_VS_EMBLEM_FZERO_DESC_OFFSET 0x3828u
#define NDS_NATIVE_VS_EMBLEM_FZERO_MOBJSUB_OFFSET 0x2ff0u
#define NDS_NATIVE_VS_EMBLEM_FZERO_MATANIM_OFFSET 0x38acu

/* [7] Kirby: 60 vertices, 288 display-list bytes. */
#define NDS_NATIVE_VS_EMBLEM_KIRBY_INDEX 7u
#define NDS_NATIVE_VS_EMBLEM_KIRBY_ROOT 0x3d48u
#define NDS_NATIVE_VS_EMBLEM_KIRBY_DL_BYTES 288u
#define NDS_NATIVE_VS_EMBLEM_KIRBY_VTX_OFFSET 0x3988u
#define NDS_NATIVE_VS_EMBLEM_KIRBY_VERTEX_COUNT 60u
#define NDS_NATIVE_VS_EMBLEM_KIRBY_DESC_OFFSET 0x3e68u
#define NDS_NATIVE_VS_EMBLEM_KIRBY_MOBJSUB_OFFSET 0x3900u
#define NDS_NATIVE_VS_EMBLEM_KIRBY_MATANIM_OFFSET 0x3eecu

/* [8] PMonsters: 84 vertices, 392 display-list bytes. */
#define NDS_NATIVE_VS_EMBLEM_PMONSTERS_INDEX 8u
#define NDS_NATIVE_VS_EMBLEM_PMONSTERS_ROOT 0x4588u
#define NDS_NATIVE_VS_EMBLEM_PMONSTERS_DL_BYTES 392u
#define NDS_NATIVE_VS_EMBLEM_PMONSTERS_VTX_OFFSET 0x4048u
#define NDS_NATIVE_VS_EMBLEM_PMONSTERS_VERTEX_COUNT 84u
#define NDS_NATIVE_VS_EMBLEM_PMONSTERS_DESC_OFFSET 0x4710u
#define NDS_NATIVE_VS_EMBLEM_PMONSTERS_MOBJSUB_OFFSET 0x3f40u
#define NDS_NATIVE_VS_EMBLEM_PMONSTERS_MATANIM_OFFSET 0x4794u

/* [9] Mother: 204 vertices, 1144 display-list bytes. */
#define NDS_NATIVE_VS_EMBLEM_MOTHER_INDEX 9u
#define NDS_NATIVE_VS_EMBLEM_MOTHER_ROOT 0x5588u
#define NDS_NATIVE_VS_EMBLEM_MOTHER_DL_BYTES 1144u
#define NDS_NATIVE_VS_EMBLEM_MOTHER_VTX_OFFSET 0x48c8u
#define NDS_NATIVE_VS_EMBLEM_MOTHER_VERTEX_COUNT 204u
#define NDS_NATIVE_VS_EMBLEM_MOTHER_DESC_OFFSET 0x5a00u
#define NDS_NATIVE_VS_EMBLEM_MOTHER_MOBJSUB_OFFSET 0x4840u
#define NDS_NATIVE_VS_EMBLEM_MOTHER_MATANIM_OFFSET 0x5a84u

/* Every distinct root, in selection-table order.  The owner accepts a
 * tree only when its child display list sits at one of these offsets
 * inside the loaded FTEmblemModels file. */
#define NDS_NATIVE_VS_EMBLEM_ROOTS { \
    0x0780u, \
    0x1fd8u, \
    0x1178u, \
    0x1778u, \
    0x2490u, \
    0x2d08u, \
    0x3628u, \
    0x3d48u, \
    0x4588u, \
    0x5588u, }

#define NDS_NATIVE_VS_EMBLEM_ROOT_DL_BYTES { \
    528u, \
    504u, \
    464u, \
    232u, \
    144u, \
    520u, \
    512u, \
    288u, \
    392u, \
    1144u, }

/* FTKind -> index into the root table above.  Mario/Luigi and
 * Pikachu/Purin repeat, which is the source's own sharing. */
#define NDS_NATIVE_VS_EMBLEM_FKIND_SERIES { \
    0u, /* 0 Mario */ \
    1u, /* 1 Fox */ \
    2u, /* 2 Donkey */ \
    3u, /* 3 Metroid */ \
    0u, /* 4 Mario */ \
    4u, /* 5 Zelda */ \
    5u, /* 6 Yoshi */ \
    6u, /* 7 FZero */ \
    7u, /* 8 Kirby */ \
    8u, /* 9 PMonsters */ \
    8u, /* 10 PMonsters */ \
    9u, /* 11 Mother */ }

#endif /* NDS_NATIVE_VS_EMBLEM_GENERATED_H */
