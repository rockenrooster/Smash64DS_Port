#include <nds/nds_mp_floor_crossing.h>

typedef struct MPFloorCrossingFixture {
    float px;
    float py;
    float qx;
    float qy;
    float ax;
    float ay;
    float bx;
    float by;
    int expected_hit;
    float expected_x;
    float expected_y;
} MPFloorCrossingFixture;

static float mpFloorFixtureAbs(float value)
{
    return (value < 0.0F) ? -value : value;
}

int smash64dsMPFloorCrossingFixture(void)
{
    static const MPFloorCrossingFixture cases[] = {
        { 5, 5, 5, -5, 0, 0, 10, 0, 1, 5, 0 },
        { 5, 5, 5, -5, 10, 0, 0, 0, 1, 5, 0 },
        { 5, -5, 5, 5, 0, 0, 10, 0, 0, 0, 0 },
        { 5, -0.0005F, 5, -1, 0, 0, 10, 0, 1, 5, 0 },
        { 5, -0.01F, 5, -1, 0, 0, 10, 0, 0, 0, 0 },
        { 5, 10, 5, 0, 0, 0, 10, 10, 1, 5, 5 },
        { 5, 10, 5, 0, 10, 10, 0, 0, 1, 5, 5 },
        { 5, 0, 5, 10, 0, 0, 10, 10, 0, 0, 0 },
        { 5, 10, 5, 0, 0, 10, 10, 0, 1, 5, 5 },
        { 5, 10, 5, 0, 10, 0, 0, 10, 1, 5, 5 },
        { 0, 1, 10, 9, 0, 0, 10, 10, 1, 5, 5 },
        { 0, 10, 0, -10, 0, 0, 0, 20, 0, 0, 0 },
        { 0.5F, 60, 0.5F, 40, 0, 0, 1, 100, 1, 0.5F, 50 },
        { 10, 1, 10, -1, 0, 0, 10, 0, 1, 10, 0 },
        { 20, 5, 20, -5, 0, 0, 10, 0, 0, 0, 0 },
        { 5, 10, 5, 5, 0, 0, 10, 10, 0, 0, 0 }
    };
    unsigned int i;

    for (i = 0; i < (sizeof(cases) / sizeof(cases[0])); i++)
    {
        float hit_x = -9999.0F;
        float hit_y = -9999.0F;
        int hit = ndsMPFloorSegmentCrossesDownwardKernel(
            cases[i].px, cases[i].py, cases[i].qx, cases[i].qy,
            cases[i].ax, cases[i].ay, cases[i].bx, cases[i].by,
            &hit_x, &hit_y);

        if (hit != cases[i].expected_hit)
        {
            return 100 + (int)i;
        }
        if ((hit != 0) &&
            ((mpFloorFixtureAbs(hit_x - cases[i].expected_x) > 0.01F) ||
             (mpFloorFixtureAbs(hit_y - cases[i].expected_y) > 0.01F)))
        {
            return 200 + (int)i;
        }
    }
    return 0;
}

#if MP_FLOOR_CROSSING_HOST_MAIN
#include <stdio.h>

int main(void)
{
    int result = smash64dsMPFloorCrossingFixture();

    if (result != 0)
    {
        fprintf(stderr, "mp floor crossing fixture failed: %d\n", result);
        return result;
    }
    printf("mp floor crossing fixtures passed: 16\n");
    return 0;
}
#endif
