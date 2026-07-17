#include <nds.h>

#include <nds/nds_task9_float_census.h>

#if NDS_TASK9_FLOAT_CENSUS

#define NDS_TASK9_FLOAT_COST_SAMPLE_LIMIT 128u
#define NDS_TASK9_FLOAT_TIMER_CALIBRATION_COUNT 256u

enum NDS_TASK9_FLOAT_ROUTINE
{
    NDS_TASK9_FLOAT_FADD,
    NDS_TASK9_FLOAT_FSUB,
    NDS_TASK9_FLOAT_FRSUB,
    NDS_TASK9_FLOAT_FMUL,
    NDS_TASK9_FLOAT_FDIV,
    NDS_TASK9_FLOAT_FCMPEQ,
    NDS_TASK9_FLOAT_FCMPLT,
    NDS_TASK9_FLOAT_FCMPLE,
    NDS_TASK9_FLOAT_FCMPGE,
    NDS_TASK9_FLOAT_FCMPGT,
    NDS_TASK9_FLOAT_FCMPUN,
    NDS_TASK9_FLOAT_F2IZ,
    NDS_TASK9_FLOAT_F2UIZ,
    NDS_TASK9_FLOAT_I2F,
    NDS_TASK9_FLOAT_UI2F,
    NDS_TASK9_FLOAT_L2F,
    NDS_TASK9_FLOAT_UL2F,
    NDS_TASK9_FLOAT_F2D,
    NDS_TASK9_FLOAT_D2F,
    NDS_TASK9_FLOAT_DADD,
    NDS_TASK9_FLOAT_DSUB,
    NDS_TASK9_FLOAT_DRSUB,
    NDS_TASK9_FLOAT_DMUL,
    NDS_TASK9_FLOAT_DDIV,
    NDS_TASK9_FLOAT_DCMPEQ,
    NDS_TASK9_FLOAT_DCMPLT,
    NDS_TASK9_FLOAT_DCMPLE,
    NDS_TASK9_FLOAT_DCMPGE,
    NDS_TASK9_FLOAT_DCMPGT,
    NDS_TASK9_FLOAT_DCMPUN,
    NDS_TASK9_FLOAT_D2IZ,
    NDS_TASK9_FLOAT_I2D,
    NDS_TASK9_FLOAT_UI2D,
    NDS_TASK9_FLOAT_L2D,
    NDS_TASK9_FLOAT_UL2D
};

volatile u32 gNdsTask9FloatCensusArmed;
volatile u32 gNdsTask9FloatCensusUpdateCount;
volatile u32 gNdsTask9FloatCensusCurrent[NDS_TASK9_FLOAT_ROUTINE_COUNT];
volatile u32 gNdsTask9FloatCensusLast[NDS_TASK9_FLOAT_ROUTINE_COUNT];
volatile u32 gNdsTask9FloatCensusPair[NDS_TASK9_FLOAT_ROUTINE_COUNT];
volatile u32 gNdsTask9FloatCensusTotal[NDS_TASK9_FLOAT_ROUTINE_COUNT];
volatile u32 gNdsTask9FloatCensusMin[NDS_TASK9_FLOAT_ROUTINE_COUNT];
volatile u32 gNdsTask9FloatCensusMax[NDS_TASK9_FLOAT_ROUTINE_COUNT];
volatile u32 gNdsTask9FloatCostTicks[NDS_TASK9_FLOAT_ROUTINE_COUNT];
volatile u32 gNdsTask9FloatCostCalls[NDS_TASK9_FLOAT_ROUTINE_COUNT];
volatile u32 gNdsTask9FloatCostMin[NDS_TASK9_FLOAT_ROUTINE_COUNT];
volatile u32 gNdsTask9FloatCostMax[NDS_TASK9_FLOAT_ROUTINE_COUNT];
volatile u32 gNdsTask9FloatTimerReadPairTicks;
volatile u32 gNdsTask9FloatTimerReadPairCount;

static void ndsTask9FloatCalibrateTimerReads(void)
{
    u32 index;

    if (gNdsTask9FloatTimerReadPairCount != 0u)
    {
        return;
    }
    for (index = 0u; index < NDS_TASK9_FLOAT_TIMER_CALIBRATION_COUNT; index++)
    {
        u32 start = cpuGetTiming();

        gNdsTask9FloatTimerReadPairTicks += cpuGetTiming() - start;
    }
    gNdsTask9FloatTimerReadPairCount = NDS_TASK9_FLOAT_TIMER_CALIBRATION_COUNT;
}

void ndsTask9FloatCensusBeginUpdate(void)
{
    u32 index;

    ndsTask9FloatCalibrateTimerReads();
    for (index = 0u; index < NDS_TASK9_FLOAT_ROUTINE_COUNT; index++)
    {
        gNdsTask9FloatCensusCurrent[index] = 0u;
    }
    gNdsTask9FloatCensusArmed = 1u;
}

void ndsTask9FloatCensusEndUpdate(void)
{
    u32 index;

    gNdsTask9FloatCensusArmed = 0u;
    for (index = 0u; index < NDS_TASK9_FLOAT_ROUTINE_COUNT; index++)
    {
        u32 count = gNdsTask9FloatCensusCurrent[index];

        if ((gNdsTask9FloatCensusUpdateCount & 1u) == 0u)
        {
            gNdsTask9FloatCensusPair[index] = 0u;
        }
        gNdsTask9FloatCensusLast[index] = count;
        gNdsTask9FloatCensusPair[index] += count;
        gNdsTask9FloatCensusTotal[index] += count;
        if ((gNdsTask9FloatCensusUpdateCount == 0u) ||
            (count < gNdsTask9FloatCensusMin[index]))
        {
            gNdsTask9FloatCensusMin[index] = count;
        }
        if (count > gNdsTask9FloatCensusMax[index])
        {
            gNdsTask9FloatCensusMax[index] = count;
        }
    }
    gNdsTask9FloatCensusUpdateCount++;
}

static u32 ndsTask9FloatCallBegin(u32 routine, u32 *start)
{
    if (gNdsTask9FloatCensusArmed == 0u)
    {
        return 0u;
    }
    gNdsTask9FloatCensusCurrent[routine]++;
    if (gNdsTask9FloatCostCalls[routine] >=
        NDS_TASK9_FLOAT_COST_SAMPLE_LIMIT)
    {
        return 0u;
    }
    *start = cpuGetTiming();
    return 1u;
}

static void ndsTask9FloatCallEnd(u32 routine, u32 start, u32 measured)
{
    u32 ticks;
    u32 calls;

    if (measured == 0u)
    {
        return;
    }
    ticks = cpuGetTiming() - start;
    calls = gNdsTask9FloatCostCalls[routine];
    gNdsTask9FloatCostTicks[routine] += ticks;
    if ((calls == 0u) || (ticks < gNdsTask9FloatCostMin[routine]))
    {
        gNdsTask9FloatCostMin[routine] = ticks;
    }
    if (ticks > gNdsTask9FloatCostMax[routine])
    {
        gNdsTask9FloatCostMax[routine] = ticks;
    }
    gNdsTask9FloatCostCalls[routine] = calls + 1u;
}

#define NDS_TASK9_WRAP_BINARY(result_type, name, argument_type, routine) \
    extern result_type __real_##name(argument_type left, argument_type right); \
    result_type __wrap_##name(argument_type left, argument_type right) \
    { \
        u32 start = 0u; \
        u32 measured = ndsTask9FloatCallBegin(routine, &start); \
        result_type result = __real_##name(left, right); \
        ndsTask9FloatCallEnd(routine, start, measured); \
        return result; \
    }

#define NDS_TASK9_WRAP_UNARY(result_type, name, argument_type, routine) \
    extern result_type __real_##name(argument_type value); \
    result_type __wrap_##name(argument_type value) \
    { \
        u32 start = 0u; \
        u32 measured = ndsTask9FloatCallBegin(routine, &start); \
        result_type result = __real_##name(value); \
        ndsTask9FloatCallEnd(routine, start, measured); \
        return result; \
    }

NDS_TASK9_WRAP_BINARY(float, __aeabi_fadd, float, NDS_TASK9_FLOAT_FADD)
NDS_TASK9_WRAP_BINARY(float, __aeabi_fsub, float, NDS_TASK9_FLOAT_FSUB)
NDS_TASK9_WRAP_BINARY(float, __aeabi_frsub, float, NDS_TASK9_FLOAT_FRSUB)
NDS_TASK9_WRAP_BINARY(float, __aeabi_fmul, float, NDS_TASK9_FLOAT_FMUL)
NDS_TASK9_WRAP_BINARY(float, __aeabi_fdiv, float, NDS_TASK9_FLOAT_FDIV)
NDS_TASK9_WRAP_BINARY(int, __aeabi_fcmpeq, float, NDS_TASK9_FLOAT_FCMPEQ)
NDS_TASK9_WRAP_BINARY(int, __aeabi_fcmplt, float, NDS_TASK9_FLOAT_FCMPLT)
NDS_TASK9_WRAP_BINARY(int, __aeabi_fcmple, float, NDS_TASK9_FLOAT_FCMPLE)
NDS_TASK9_WRAP_BINARY(int, __aeabi_fcmpge, float, NDS_TASK9_FLOAT_FCMPGE)
NDS_TASK9_WRAP_BINARY(int, __aeabi_fcmpgt, float, NDS_TASK9_FLOAT_FCMPGT)
NDS_TASK9_WRAP_BINARY(int, __aeabi_fcmpun, float, NDS_TASK9_FLOAT_FCMPUN)
NDS_TASK9_WRAP_UNARY(int, __aeabi_f2iz, float, NDS_TASK9_FLOAT_F2IZ)
NDS_TASK9_WRAP_UNARY(unsigned int, __aeabi_f2uiz, float,
                     NDS_TASK9_FLOAT_F2UIZ)
NDS_TASK9_WRAP_UNARY(float, __aeabi_i2f, int, NDS_TASK9_FLOAT_I2F)
NDS_TASK9_WRAP_UNARY(float, __aeabi_ui2f, unsigned int, NDS_TASK9_FLOAT_UI2F)
NDS_TASK9_WRAP_UNARY(float, __aeabi_l2f, long long, NDS_TASK9_FLOAT_L2F)
NDS_TASK9_WRAP_UNARY(float, __aeabi_ul2f, unsigned long long,
                     NDS_TASK9_FLOAT_UL2F)
NDS_TASK9_WRAP_UNARY(double, __aeabi_f2d, float, NDS_TASK9_FLOAT_F2D)
NDS_TASK9_WRAP_UNARY(float, __aeabi_d2f, double, NDS_TASK9_FLOAT_D2F)
NDS_TASK9_WRAP_BINARY(double, __aeabi_dadd, double, NDS_TASK9_FLOAT_DADD)
NDS_TASK9_WRAP_BINARY(double, __aeabi_dsub, double, NDS_TASK9_FLOAT_DSUB)
NDS_TASK9_WRAP_BINARY(double, __aeabi_drsub, double, NDS_TASK9_FLOAT_DRSUB)
NDS_TASK9_WRAP_BINARY(double, __aeabi_dmul, double, NDS_TASK9_FLOAT_DMUL)
NDS_TASK9_WRAP_BINARY(double, __aeabi_ddiv, double, NDS_TASK9_FLOAT_DDIV)
NDS_TASK9_WRAP_BINARY(int, __aeabi_dcmpeq, double, NDS_TASK9_FLOAT_DCMPEQ)
NDS_TASK9_WRAP_BINARY(int, __aeabi_dcmplt, double, NDS_TASK9_FLOAT_DCMPLT)
NDS_TASK9_WRAP_BINARY(int, __aeabi_dcmple, double, NDS_TASK9_FLOAT_DCMPLE)
NDS_TASK9_WRAP_BINARY(int, __aeabi_dcmpge, double, NDS_TASK9_FLOAT_DCMPGE)
NDS_TASK9_WRAP_BINARY(int, __aeabi_dcmpgt, double, NDS_TASK9_FLOAT_DCMPGT)
NDS_TASK9_WRAP_BINARY(int, __aeabi_dcmpun, double, NDS_TASK9_FLOAT_DCMPUN)
NDS_TASK9_WRAP_UNARY(int, __aeabi_d2iz, double, NDS_TASK9_FLOAT_D2IZ)
NDS_TASK9_WRAP_UNARY(double, __aeabi_i2d, int, NDS_TASK9_FLOAT_I2D)
NDS_TASK9_WRAP_UNARY(double, __aeabi_ui2d, unsigned int, NDS_TASK9_FLOAT_UI2D)
NDS_TASK9_WRAP_UNARY(double, __aeabi_l2d, long long, NDS_TASK9_FLOAT_L2D)
NDS_TASK9_WRAP_UNARY(double, __aeabi_ul2d, unsigned long long,
                     NDS_TASK9_FLOAT_UL2D)

#endif
