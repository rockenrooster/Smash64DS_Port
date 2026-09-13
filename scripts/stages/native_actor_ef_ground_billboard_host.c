/* Host oracle for the actual ef-ground kind72 billboard rows (inserted by pytest).
 * The ROWS function is the VERBATIM adapter source between
 * BEGIN/END-EF-GROUND-BILLBOARD-ROWS in src/port/renderer_adapter_matrix.c
 * (spliced at NATIVE_ACTOR_IMPLEMENTATION); the oracle below is an
 * independent double-precision regrouping of lbcommon.c:1879-1912, and the
 * driver cross-checks them over an angle/scale sweep plus exact identity
 * and accumulator-threading checks. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
typedef float f32;
typedef uint32_t u32;
typedef int32_t s32;

/* NATIVE_ACTOR_IMPLEMENTATION */

/* Independent oracle: same 800CAB48 construction, regrouped, double. */
static void billboard_oracle(const double P[4][4], double rotx, double roty,
    double sx, double sy, double *accum, double out[3][4])
{
    double sinx = sin(rotx);
    double cosx = cos(rotx);
    double siny = sin(roty);
    double cosy = cos(roty);
    double old = *accum;
    double scalex = old * sx;
    double scaley = sy * old;
    int c;

    *accum = scalex;
    for (c = 0; c < 4; c++)
    {
        double a0 = P[0][c];
        double a1 = P[1][c];
        double a2 = P[2][c];

        out[0][c] = scalex * (a0 * cosy - a2 * siny);
        out[1][c] = scaley * (sinx * (a0 * siny + a2 * cosy) + a1 * cosx);
        out[2][c] = scalex * (cosx * (a0 * siny + a2 * cosy) - a1 * sinx);
    }
}

static int g_failures;
#define CHECK(name, cond) do { if (!(cond)) { \
    printf("FAIL %s line %d\n", name, __LINE__); g_failures++; } } while (0)

/* BILLBOARD_DRIVER */
static const float kPersp[4][4] = {
    { 2.0f, 0.0f, 0.0f, 0.0f },
    { 0.0f, 2.4f, 0.0f, 0.0f },
    { 0.1f, -0.2f, -1.5f, -1.0f },
    { 0.0f, 0.0f, -0.3f, 0.0f },
};

int main(void)
{
    static const float rots[] = { 0.0f, 0.3f, -0.7f, 1.0f, 2.5f };
    static const float scales[] = { 0.5f, 1.0f, 1.1f, 1.4f };
    static const float accums[] = { 1.0f, 2.0f };
    u32 ri, rj, si, sj, ak;
    u32 i, c;

    /* Exact identity: zero rotation, unit scales, unit accumulator. */
    {
        float rows[3][4];
        float accum = 1.0f;
        int exact = 1;

        ndsRendererAdapterEfGroundBillboardRows(kPersp, 0.0f, 0.0f,
            1.0f, 1.0f, &accum, rows);
        for (i = 0u; i < 3u; i++)
        {
            for (c = 0u; c < 4u; c++)
            {
                if (rows[i][c] != kPersp[i][c]) { exact = 0; }
            }
        }
        CHECK("identity", exact && (accum == 1.0f));
    }
    /* Sweep against the independent oracle; accumulator threading exact. */
    for (ri = 0u; ri < 5u; ri++)
    {
        for (rj = 0u; rj < 5u; rj++)
        {
            for (si = 0u; si < 4u; si++)
            {
                for (sj = 0u; sj < 4u; sj++)
                {
                    for (ak = 0u; ak < 2u; ak++)
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
                        ndsRendererAdapterEfGroundBillboardRows(kPersp,
                            rots[ri], rots[rj], scales[si], scales[sj],
                            &accum, rows);
                        billboard_oracle(P, (double)rots[ri],
                            (double)rots[rj], (double)scales[si],
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
    }
    if (g_failures == 0) { printf("BILLBOARD-HOST-OK\n"); }
    return g_failures ? 1 : 0;
}
