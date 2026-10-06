/* Source-backed matrix concatenation used by BattleShip's camera contract.
 *
 * P2-2p8 (2026-10-05, owner: "Software floating point should not exist, fixed
 * point only"): guMtxCatF without the products against a zero of nf. Its
 * callers concatenate a look-at with a syMatrixPerspFastF projection, which
 * has five non-zero elements of sixteen (the camera's own sparse concat, W3s
 * in battleship_gmcamera.c, is the same observation), so 44 of the 64
 * multiplies and 44 of the adds were against a literal zero.
 *
 * BIT-EXACT. The source accumulates temp = 0.0 + p0 + p1 + p2 + p3 left to
 * right. A finite value times a zero of either sign is a zero, and adding a
 * zero leaves the running sum unchanged unless that sum is -0 -- which it
 * never is: it starts at +0, +0 + -0 is +0, and an exact cancellation rounds
 * to +0 under round-to-nearest. Skipping those terms therefore leaves the
 * same operations on the same values in the same order. A non-finite mf
 * element (where inf * 0 is NaN) keeps its products; the zero tests read the
 * bits, so no float compare is added. */
#define guMtxCatF ndsBaseGuMtxCatF
#include "../../decomp/BattleShip-main/decomp/src/libultra/gu/mtxcatf.c"
#undef guMtxCatF

void guMtxCatF(float mf[4][4], float nf[4][4], float res[4][4]);

static inline unsigned int ndsMtxCatBits(const float *value)
{
    unsigned int bits;

    __builtin_memcpy(&bits, value, sizeof(bits));
    return bits;
}

void guMtxCatF(float mf[4][4], float nf[4][4], float res[4][4])
{
    float temp[4][4];
    unsigned int column_terms[4];
    int i;
    int j;
    int k;

    for (j = 0; j < 4; j++)
    {
        unsigned int terms = 0u;

        for (k = 0; k < 4; k++)
        {
            if ((ndsMtxCatBits(&nf[k][j]) & 0x7FFFFFFFu) != 0u)
            {
                terms |= 1u << k;
            }
        }
        column_terms[j] = terms;
    }
    for (i = 0; i < 4; i++)
    {
        unsigned int non_finite = 0u;

        for (k = 0; k < 4; k++)
        {
            if ((ndsMtxCatBits(&mf[i][k]) & 0x7F800000u) == 0x7F800000u)
            {
                non_finite |= 1u << k;
            }
        }
        for (j = 0; j < 4; j++)
        {
            const unsigned int terms = column_terms[j] | non_finite;
            float sum = 0.0F;

            for (k = 0; k < 4; k++)
            {
                if ((terms & (1u << k)) != 0u)
                {
                    sum += mf[i][k] * nf[k][j];
                }
            }
            temp[i][j] = sum;
        }
    }
    for (i = 0; i < 4; i++)
    {
        for (j = 0; j < 4; j++)
        {
            res[i][j] = temp[i][j];
        }
    }
}
