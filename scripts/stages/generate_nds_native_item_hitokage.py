#!/usr/bin/env python3
"""Generate/check Saffron City's Hitokage (Charmander) live-image owner."""

from generate_nds_native_yamabuki_live_item import YamabukiLiveItemSpec, run

SPEC = YamabukiLiveItemSpec(
    name="Hitokage",
    slug="hitokage",
    macro="HITOKAGE",
    kind_token="nITKindHitokage,",
    attr_token="&llGRYamabukiMapHitokageItemAttributes",
    source_path="decomp/BattleShip-main/decomp/src/it/itground/ithitokage.c",
    attr_offset=0x01FC,
    dobjdesc=0x1990,
    mobj_heads=0x17D0,
    mobj_child=0x1858,
    animjoint=0x1A20,
    anim_child=0x1A28,
    root=0x18A0,
    vtx=0x1860,
    tlut=0x1020,
    verts=(
        (252, 252, 0, 1280, 1280, 0xFFFFFFFF),
        (252, -252, 0, 1280, 0, 0xFFFFFFFF),
        (-294, -252, 0, 0, 0, 0xFFFFFFFF),
        (-294, 252, 0, 0, 1280, 0xFFFFFFFF),
    ),
    tlut_values=(
        0x5142, 0xF777, 0xED4D, 0xF603, 0x9C21, 0xF403, 0xF343, 0xD3C7,
        0x930D, 0xCA43, 0x7ACF, 0x03C1, 0x5A91, 0x9A03, 0x7183, 0x5143,
    ),
    tile_size=(0xF2000000, 0x0009C09C),
    load_block=(0xF3000000, 0x071DF2AB),
    check_line=(
        "ITEM_HITOKAGE_NATIVE_OK root=0x18a0 verts=4 tris=2 "
        "material=CURRENT_IMAGE tlut=159:0x1020 referrers=Hitokage"),
)

if __name__ == "__main__":
    raise SystemExit(run(SPEC))

