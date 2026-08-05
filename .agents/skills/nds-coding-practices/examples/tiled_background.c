/*
 * Generated-in-place 4bpp tiled BG. A real project should generate tile, map,
 * and palette data at build time and retain the same allocation discipline.
 */
#include <nds.h>
#include <stdint.h>

static void build_demo_tiles(uint16_t *tile_words)
{
    // 4bpp tile = 8x8 pixels = 32 bytes = 16 halfwords. Halfword stores are
    // required: VRAM ignores 8-bit writes.
    // Tile 0: palette index 0. Tile 1: alternating indexes 1 and 2.
    for (unsigned i = 0; i < 16u; ++i) {
        tile_words[i] = 0x0000u;
        tile_words[16u + i] = (i & 1u) ? 0x1212u : 0x2121u;
    }
}

static void build_demo_map(uint16_t *map_entries)
{
    for (unsigned y = 0; y < 32u; ++y) {
        for (unsigned x = 0; x < 32u; ++x) {
            map_entries[y * 32u + x] = (uint16_t)(((x ^ y) & 1u) ? 1u : 0u);
        }
    }
}

int main(void)
{
    videoSetMode(MODE_0_2D);
    vramSetBankA(VRAM_A_MAIN_BG);

    // Text 256x256 BG: map base 31 occupies 2 KiB; tile base 0 occupies the
    // beginning of BG VRAM. These intervals do not overlap.
    const int bg = bgInit(0, BgType_Text4bpp, BgSize_T_256x256, 31, 0);

    uint16_t *const gfx = (uint16_t *)bgGetGfxPtr(bg);
    uint16_t *const map = (uint16_t *)bgGetMapPtr(bg);

    build_demo_tiles(gfx);
    build_demo_map(map);

    BG_PALETTE[0] = RGB15(0, 0, 0);
    BG_PALETTE[1] = RGB15(31, 31, 31);
    BG_PALETTE[2] = RGB15(0, 20, 31);

    int scroll_x = 0;

    while (pmMainLoop()) {
        scanKeys();
        const uint32_t held = keysHeld();
        const uint32_t down = keysDown();

        if ((down & KEY_START) != 0) {
            break;
        }

        scroll_x += ((held & KEY_RIGHT) != 0) - ((held & KEY_LEFT) != 0);
        bgSetScroll(bg, scroll_x, 0);

        swiWaitForVBlank();
        bgUpdate();
    }

    return 0;
}
