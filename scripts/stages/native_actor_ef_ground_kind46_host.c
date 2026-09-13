/* Host oracle for the actual ef-ground kind46 billboard rows (inserted by pytest).
 * The ROWS function is the VERBATIM adapter source between
 * BEGIN/END-EF-GROUND-KIND46-ROWS in src/port/renderer_adapter_matrix.c
 * (spliced at NATIVE_ACTOR_IMPLEMENTATION); the oracle below is an
 * independent double-precision regrouping of objdisplay.c:960-983 (case 46:
 * f12 = scale.y * gGCScaleX, gGCScaleX *= scale.x, six writes plus the six
 * zeroes), and the driver cross-checks them over a rotz/scale sweep covering
 * both lr families (unit scales, desc scales 1.0/1.3/1.4, C14 flap 1.1, the
 * pi rotz of the lr==+1 180-degree arm) plus exact zero-rotz and
 * accumulator-threading checks. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
typedef float f32;
typedef uint32_t u32;
typedef int32_t s32;

/* NATIVE_ACTOR_IMPLEMENTATION */

/* Independent oracle: same objdisplay.c:960-983 construction, regrouped. */
static void kind46_oracle(const double P[4][4], double rotz,
    double sx, double sy, double *accum, double out[3][4])
{
    double sinz = sin(rotz);
    double cosz = cos(rotz);
    double old = *accum;
    double scalex = old * sx;
    double scaley = sy * old;
    int c;

    (void)c;
    *accum = scalex;
    out[0][0] = scalex * P[0][0] * cosz;
    out[0][1] = scaley * P[1][1] * sinz;
    out[0][2] = 0.0;
    out[0][3] = 0.0;
    out[1][0] = scalex * P[0][0] * -sinz;
    out[1][1] = scaley * P[1][1] * cosz;
    out[1][2] = 0.0;
    out[1][3] = 0.0;
    out[2][0] = 0.0;
    out[2][1] = 0.0;
    out[2][2] = scalex * P[2][2];
    out[2][3] = scalex * P[2][3];
}

static int g_failures;
#define CHECK(name, cond) do { if (!(cond)) { \
    printf("FAIL %s line %d\n", name, __LINE__); g_failures++; } } while (0)

/* KIND46_DRIVER */
static const float kPersp[4][4] = {
    { 2.0f, 0.0f, 0.0f, 0.0f },
    { 0.0f, 2.4f, 0.0f, 0.0f },
    { 0.1f, -0.2f, -1.5f, -1.0f },
    { 0.0f, 0.0f, -0.3f, 0.0f },
};

int main(void)
{
    /* Legal-class rotz/scales: drawables carry rotz 0 live (no drawable
     * anim), but the core must match source for any rotz, including the pi
     * of the lr==+1 setup arm; scales cover unit, R-Lakitu 1.3, Bronto 1.4,
     * C14 flap 1.1; accums cover fresh trees (1.0) and ancestor-seeded
     * priors (1.3/1.4, the A-root desc scales). */
    static const float rotzs[] = { 0.0f, 0.3f, -0.7f, 3.14159265f, 2.5f };
    static const float scales[] = { 0.5f, 1.0f, 1.1f, 1.3f, 1.4f };
    static const float accums[] = { 1.0f, 1.3f, 1.4f };
    u32 ri, si, sj, ak;
    u32 i, c;

    /* Exact zero-rotz: six writes land, six zeroes land, accum threads. */
    {
        float rows[3][4];
        float accum = 1.0f;
        int exact = 1;

        ndsRendererAdapterEfGroundKind46Rows(kPersp, 0.0f,
            1.0f, 1.0f, &accum, rows);
        if (rows[0][0] != 2.0f) { exact = 0; }
        if (rows[0][1] != 0.0f) { exact = 0; }
        if (rows[0][2] != 0.0f) { exact = 0; }
        if (rows[0][3] != 0.0f) { exact = 0; }
        if (rows[1][0] != 0.0f) { exact = 0; }
        if (rows[1][1] != 2.4f) { exact = 0; }
        if (rows[1][2] != 0.0f) { exact = 0; }
        if (rows[1][3] != 0.0f) { exact = 0; }
        if (rows[2][0] != 0.0f) { exact = 0; }
        if (rows[2][1] != 0.0f) { exact = 0; }
        if (rows[2][2] != -1.5f) { exact = 0; }
        if (rows[2][3] != -1.0f) { exact = 0; }
        CHECK("identity", exact && (accum == 1.0f));
    }
    /* Sweep against the independent oracle; accumulator threading exact. */
    for (ri = 0u; ri < 5u; ri++)
    {
        for (si = 0u; si < 5u; si++)
        {
            for (sj = 0u; sj < 5u; sj++)
            {
                for (ak = 0u; ak < 3u; ak++)
                {
                    float rows[3][4];
                    double want[3][4];
                    float accum = accums[ak];
                    double accum_want = (double)accums[ak];
                    double P[4][4];
                    int good = 1;

                    for (i = 0u; i < 4u; i++)
                    {
                        for (c = 0u; c < 4u; c++)
                        {
                            P[i][c] = (double)kPersp[i][c];
                        }
                    }
                    ndsRendererAdapterEfGroundKind46Rows(kPersp,
                        rotzs[ri], scales[si], scales[sj],
                        &accum, rows);
                    kind46_oracle(P, (double)rotzs[ri],
                        (double)scales[si],
                        (double)scales[sj], &accum_want, want);
                    for (i = 0u; i < 3u; i++)
                    {
                        for (c = 0u; c < 4u; c++)
                        {
                            double d = (double)rows[i][c] - want[i][c];
                            double m = (want[i][c] < 0.0) ?
                                -want[i][c] : want[i][c];

                            if (!((d > -0.002 && d < 0.002) ||
                                  (m > 0.0 &&
                                   d / m > -0.0005 &&
                                   d / m < 0.0005)))
                            {
                                good = 0;
                            }
                        }
                    }
                    CHECK("sweep", good &&
                        (accum == (float)accum_want));
                }
            }
        }
    }
    if (g_failures == 0) { printf("KIND46-HOST-OK\n"); }
    return g_failures ? 1 : 0;
}
