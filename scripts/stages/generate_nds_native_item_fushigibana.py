#!/usr/bin/env python3
"""Generate/check Saffron City's Fushigibana (Venusaur) live-image owner."""

from generate_nds_native_yamabuki_live_item import YamabukiLiveItemSpec, run

SPEC = YamabukiLiveItemSpec(
    name="Fushigibana",
    slug="fushigibana",
    macro="FUSHIGIBANA",
    kind_token="nITKindFushigibana,",
    attr_token="&llGRYamabukiMapFushigibanaItemAttributes",
    source_path="decomp/BattleShip-main/decomp/src/it/itground/itfushigibana.c",
    attr_offset=0x0278,
    dobjdesc=0x2340,
    mobj_heads=0x2180,
    mobj_child=0x2208,
    animjoint=0x23D0,
    anim_child=0x23D8,
    root=0x2250,
    vtx=0x2210,
    tlut=0x1B50,
    verts=(
        (480, 360, 0, 1536, 1024, 0xFFFFFFFF),
        (480, -360, 0, 1536, 0, 0xFFFFFFFF),
        (-540, -360, 0, 0, 0, 0xFFFFFFFF),
        (-540, 360, 0, 0, 1024, 0xFFFFFFFF),
    ),
    tlut_values=(
        0x741E, 0xFFFF, 0xA98D, 0x5D85, 0x818B, 0xE18F, 0x3345, 0x21C7,
        0xB6B5, 0x6FFF, 0x741F, 0x2C21, 0x3445, 0x1A95, 0x4EFB, 0x456D,
    ),
    tile_size=(0xF2000000, 0x000BC07C),
    load_block=(0xF3000000, 0x0717F2AB),
    check_line=(
        "ITEM_FUSHIGIBANA_NATIVE_OK root=0x2250 verts=4 tris=2 "
        "material=CURRENT_IMAGE tlut=159:0x1b50 referrers=Fushigibana"),
)

if __name__ == "__main__":
    raise SystemExit(run(SPEC))

