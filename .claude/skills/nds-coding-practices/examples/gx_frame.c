/*
 * Minimal explicit GX frame with balanced matrix state and one glFlush.
 * Float use is limited to setup in gluPerspective; hot projects should use
 * precomputed/fixed projection data after validating their conventions.
 */
#include <nds.h>
#include <stdint.h>

static void gx_initialize(void)
{
    videoSetMode(MODE_0_3D);

    // This demonstration does not use textures, but a real texture renderer
    // must map texture and texture-palette banks through one VRAM owner.
    glInit();
    glEnable(GL_ANTIALIAS);
    glClearColor(0, 0, 0, 31);
    glClearPolyID(63);
    glClearDepth(GL_MAX_DEPTH);
    glViewport(0, 0, 255, 191);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(70, 256.0 / 192.0, 0.1, 40.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

static void gx_draw_triangle(int angle)
{
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef32(0, 0, -inttof32(3));

    glPushMatrix();
    glRotateYi(angle);

    glPolyFmt(POLY_ALPHA(31) | POLY_CULL_NONE);
    glBegin(GL_TRIANGLES);

    // glColor3b components are 0-255; the hardware keeps the top 5 bits.
    // Passing 5-bit values here would render a nearly black triangle.
    glColor3b(255, 0, 0);
    glVertex3v16(-inttov16(1), -inttov16(1), 0);

    glColor3b(0, 255, 0);
    glVertex3v16(inttov16(1), -inttov16(1), 0);

    glColor3b(0, 0, 255);
    glVertex3v16(0, inttov16(1), 0);

    glEnd();
    glPopMatrix(1);
}

int main(void)
{
    int angle = 0;
    gx_initialize();

    while (pmMainLoop()) {
        scanKeys();
        if ((keysDown() & KEY_START) != 0) {
            break;
        }

        gx_draw_triangle(angle);
        angle = (angle + 128) & 0x7FFF;

        // One frame-finalization owner. Do not flush inside each model.
        glFlush(0);
        swiWaitForVBlank();
    }

    return 0;
}
