/* Standalone devkitPro/libnds native cutout smoke test, not a material compiler.
 * The opaque blue rear plane is intentional for this full-3D demonstration.
 * A transparent border and 2x2 central hole must show blue, not a filled quad;
 * one opaque black texel must stay black. No extracted game assets are used.
 */
#include <nds.h>
#include <stdint.h>

static uint16_t cutout_pixels[8u * 8u] __attribute__((aligned(32)));

static void make_cutout(void)
{
    for (unsigned y = 0; y < 8u; ++y) {
        for (unsigned x = 0; x < 8u; ++x) {
            const bool visible = x != 0u && x != 7u && y != 0u && y != 7u &&
                                 !(x >= 3u && x <= 4u && y >= 3u && y <= 4u);
            const uint16_t color = (x == 2u && y == 2u) ? RGB15(0, 0, 0) : RGB15(31, 8, 0);
            cutout_pixels[y * 8u + x] = color | (visible ? 0x8000u : 0u);
        }
    }
}

int main(void)
{
    int texture = 0;
    videoSetMode(MODE_0_3D);
    vramSetBankA(VRAM_A_TEXTURE);
    glInit();
    // These are global raster settings, not attributes attached to queued draws.
    glEnable(GL_TEXTURE_2D | GL_ALPHA_TEST);
    glDisable(GL_BLEND); // This demo has binary coverage, no graded compositing.
    glAlphaFunc(0);
    glClearColor(4, 10, 24, 31);
    glClearPolyID(63);
    glClearDepth(GL_MAX_DEPTH);
    glViewport(0, 0, 255, 191);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(70, 256.0 / 192.0, 0.1, 40.0); // Setup only.

    make_cutout();
    if (!glGenTextures(1, &texture)) return 1;
    glBindTexture(0, texture);
    // GL_RGBA preserves bit 15. GL_RGB would make all 64 texels opaque.
    if (!glTexImage2D(0, 0, GL_RGBA, TEXTURE_SIZE_8, TEXTURE_SIZE_8,
                      0, TEXGEN_OFF, cutout_pixels)) {
        glDeleteTextures(1, &texture); // No draw ever referenced this allocation.
        return 1;
    }

    while (pmMainLoop()) {
        scanKeys();
        if (keysDown() & KEY_START) break;
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glTranslatef32(0, 0, -inttof32(3));
        glBindTexture(0, texture);
        glPolyFmt(POLY_MODULATION | POLY_ALPHA(31) | POLY_ID(1) | POLY_CULL_NONE);
        glColor3b(255, 255, 255);
        glBegin(GL_QUADS);
        glTexCoord2t16(0, 0);       glVertex3v16(-inttov16(1),  inttov16(1), 0);
        glTexCoord2t16(0, 8 << 4);  glVertex3v16(-inttov16(1), -inttov16(1), 0);
        glTexCoord2t16(8 << 4, 8 << 4); glVertex3v16(inttov16(1), -inttov16(1), 0);
        glTexCoord2t16(8 << 4, 0);  glVertex3v16(inttov16(1), inttov16(1), 0);
        glEnd();
        glFlush(0);
        swiWaitForVBlank();
    }
    // Process-lifetime demonstration: leave texture memory owned until exit.
    // A reusable scene must retire the raster consumer before deleting textures;
    // neither a single glFlush nor command-DMA completion is such a fence.
    return 0;
}
