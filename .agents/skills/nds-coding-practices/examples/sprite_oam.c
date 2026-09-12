/*
 * One 16x16 256-color sprite using current libnds shadow OAM conventions.
 * Based on the composition used by the official devkitPro sprite examples.
 */
#include <nds.h>
#include <stdint.h>

static void fill_sprite_pixels(volatile uint16_t *gfx)
{
    // 1D 8-bpp OBJ: four 8x8 tiles in tile-row order, NOT a linear bitmap.
    // Index 0 is transparent; keep a two-pixel border around a red square.
    // Every destination store is a halfword (VRAM does not accept byte writes).
    for (unsigned ty = 0; ty < 2u; ++ty) {
        for (unsigned tx = 0; tx < 2u; ++tx) {
            for (unsigned y = 0; y < 8u; ++y) {
                for (unsigned x = 0; x < 8u; x += 2u) {
                    const unsigned py = ty * 8u + y;
                    const unsigned px = tx * 8u + x;
                    const unsigned low = (px >= 2u && px < 14u && py >= 2u && py < 14u);
                    const unsigned high = (px + 1u >= 2u && px + 1u < 14u && py >= 2u && py < 14u);
                    gfx[(ty * 2u + tx) * 32u + y * 4u + x / 2u] = (uint16_t)(low | (high << 8));
                }
            }
        }
    }
}

int main(void)
{
    int x = 120;
    int y = 88;

    videoSetMode(MODE_0_2D);
    vramSetBankA(VRAM_A_MAIN_SPRITE);
    oamInit(&oamMain, SpriteMapping_1D_32, false);

    uint16_t *const gfx = oamAllocateGfx(
        &oamMain, SpriteSize_16x16, SpriteColorFormat_256Color);

    if (gfx == NULL) {
        return 1;
    }

    fill_sprite_pixels(gfx);
    SPRITE_PALETTE[1] = RGB15(31, 0, 0);

    // oamInit establishes the OAM shadow owner. Keep all sprite updates through
    // that owner rather than mixing raw hardware OAM writes.
    while (pmMainLoop()) {
        scanKeys();
        const uint32_t held = keysHeld();
        const uint32_t down = keysDown();

        if ((down & KEY_START) != 0) {
            break;
        }

        x += ((held & KEY_RIGHT) != 0) - ((held & KEY_LEFT) != 0);
        y += ((held & KEY_DOWN) != 0) - ((held & KEY_UP) != 0);

        oamSet(&oamMain,
               0,                         // sprite index
               x, y,
               0,                         // priority: 0 is highest
               0,                         // palette index
               SpriteSize_16x16,
               SpriteColorFormat_256Color,
               gfx,
               -1,                        // no affine matrix
               false,                     // no affine double size
               false,                     // visible
               false, false,              // hflip, vflip
               false);                    // mosaic

        swiWaitForVBlank();
        oamUpdate(&oamMain);               // one bounded shadow-OAM commit
    }

    // Remove the hardware reference before its graphics block can be reused.
    oamSetHidden(&oamMain, 0, true);
    swiWaitForVBlank();
    oamUpdate(&oamMain);
    oamFreeGfx(&oamMain, gfx);
    return 0;
}
