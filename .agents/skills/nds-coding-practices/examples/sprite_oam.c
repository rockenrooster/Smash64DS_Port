/*
 * One 16x16 256-color sprite using current libnds shadow OAM conventions.
 * Based on the composition used by the official devkitPro sprite examples.
 */
#include <nds.h>
#include <stdint.h>

static void fill_sprite_pixels(volatile uint16_t *gfx)
{
    // Two 8-bit pixels per halfword. Palette index 1 fills the sprite.
    // Halfword stores are mandatory: VRAM ignores 8-bit writes, so a
    // uint8_t pixel loop or byte-path memset would silently store nothing.
    for (unsigned i = 0; i < (16u * 16u) / 2u; ++i) {
        gfx[i] = 1u | (1u << 8);
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

    oamFreeGfx(&oamMain, gfx);
    return 0;
}
