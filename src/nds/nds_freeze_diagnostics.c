#include <nds.h>

#include <calico/system/tick.h>

#include <nds/nds_audio_fgm.h>
#include <nds/nds_freeze_diagnostics.h>
#include <nds/nds_startup.h>

#define NDS_FREEZE_DIAGNOSTICS_REPORT_STALL 0x5354414cu /* STAL */
#define NDS_FREEZE_DIAGNOSTICS_REPORT_EXCEPTION 0x45584350u /* EXCP */
#define NDS_FREEZE_DIAGNOSTICS_WATCHDOG_HZ 1u
#define NDS_FREEZE_DIAGNOSTICS_STALE_SAMPLES 2u
#define NDS_FREEZE_DIAGNOSTICS_VISIBLE_ROWS 24u
#define NDS_FREEZE_DIAGNOSTICS_COLUMNS 32u

volatile u32 gNdsFreezeDiagnosticsHeartbeat;
volatile u32 gNdsFreezeDiagnosticsLastBreadcrumb;
volatile u32 gNdsFreezeDiagnosticsBreadcrumbWriteCount;
volatile u32 gNdsFreezeDiagnosticsBreadcrumbs[
    NDS_FREEZE_DIAGNOSTICS_BREADCRUMB_COUNT];
volatile u32 gNdsFreezeDiagnosticsInterruptedPC;
volatile u32 gNdsFreezeDiagnosticsInterruptedLR;
volatile u32 gNdsFreezeDiagnosticsWatchdogTripCount;
volatile u32 gNdsFreezeDiagnosticsFgmEnterCount;
volatile u32 gNdsFreezeDiagnosticsFgmReturnCount;
volatile u32 gNdsFreezeDiagnosticsLastFgmID;
volatile s32 gNdsFreezeDiagnosticsLastFgmChannel = -1;
volatile u32 gNdsFreezeDiagnosticsSwapPending;
volatile u32 gNdsFreezeDiagnosticsForceTrip;
volatile u32 gNdsFreezeDiagnosticsReportKind;
volatile u32 gNdsFreezeDiagnosticsReportGXStatus;
volatile u32 gNdsFreezeDiagnosticsReportPolygonCount;
volatile u32 gNdsFreezeDiagnosticsReportVertexCount;
volatile u32 gNdsFreezeDiagnosticsReportIpcFifo;
volatile u32 gNdsFreezeDiagnosticsReportIpcSync;
volatile u32 gNdsFreezeDiagnosticsReportPresentedFrames;
volatile u32 gNdsFreezeDiagnosticsReportLogicFrames;

VoidFn gNdsFreezeDiagnosticsOriginalIrqVector;
void ndsFreezeDiagnosticsIrqVector(void);

static TickTask sNdsFreezeDiagnosticsWatchdogTask;
static u16 *sNdsFreezeDiagnosticsConsoleMap;
static u16 sNdsFreezeDiagnosticsFontBase;
static u16 sNdsFreezeDiagnosticsFontPalette;
static u16 sNdsFreezeDiagnosticsAsciiOffset;
static u16 sNdsFreezeDiagnosticsCharacterCount;
static u32 sNdsFreezeDiagnosticsLastHeartbeat;
static u32 sNdsFreezeDiagnosticsStaleSamples;
static u32 sNdsFreezeDiagnosticsWatchdogArmed;
static u32 sNdsFreezeDiagnosticsInitialized;

static u16 ndsFreezeDiagnosticsCharacterTile(char value)
{
    u32 character = (u8)value;

    if ((character < sNdsFreezeDiagnosticsAsciiOffset) ||
        (character >= (u32)sNdsFreezeDiagnosticsAsciiOffset +
                          sNdsFreezeDiagnosticsCharacterCount))
    {
        character = '?';
    }
    return sNdsFreezeDiagnosticsFontPalette |
           (sNdsFreezeDiagnosticsFontBase +
            (u16)(character - sNdsFreezeDiagnosticsAsciiOffset));
}

static void ndsFreezeDiagnosticsPutCharacter(u32 row, u32 column, char value)
{
    if ((sNdsFreezeDiagnosticsConsoleMap == NULL) ||
        (row >= NDS_FREEZE_DIAGNOSTICS_VISIBLE_ROWS) ||
        (column >= NDS_FREEZE_DIAGNOSTICS_COLUMNS))
    {
        return;
    }
    sNdsFreezeDiagnosticsConsoleMap[
        row * NDS_FREEZE_DIAGNOSTICS_COLUMNS + column] =
            ndsFreezeDiagnosticsCharacterTile(value);
}

static u32 ndsFreezeDiagnosticsPutText(u32 row, u32 column, const char *text)
{
    while ((*text != '\0') && (column < NDS_FREEZE_DIAGNOSTICS_COLUMNS))
    {
        ndsFreezeDiagnosticsPutCharacter(row, column++, *text++);
    }
    return column;
}

static u32 ndsFreezeDiagnosticsPutHex(u32 row, u32 column, u32 value,
                                      u32 digits)
{
    static const char hex[] = "0123456789ABCDEF";
    u32 shift = digits * 4u;

    while ((digits-- != 0u) && (column < NDS_FREEZE_DIAGNOSTICS_COLUMNS))
    {
        shift -= 4u;
        ndsFreezeDiagnosticsPutCharacter(
            row, column++, hex[(value >> shift) & 0xfu]);
    }
    return column;
}

static void ndsFreezeDiagnosticsClearReport(void)
{
    u32 row;
    u32 column;

    for (row = 0u; row < NDS_FREEZE_DIAGNOSTICS_VISIBLE_ROWS; row++)
    {
        for (column = 0u; column < NDS_FREEZE_DIAGNOSTICS_COLUMNS; column++)
        {
            ndsFreezeDiagnosticsPutCharacter(row, column, ' ');
        }
    }
}

static const char *ndsFreezeDiagnosticsClassifyStall(void)
{
    u32 last = gNdsFreezeDiagnosticsLastBreadcrumb;

    if ((last == NDS_FREEZE_BREADCRUMB_FGM_ENTER) &&
        (gNdsFreezeDiagnosticsFgmEnterCount !=
         gNdsFreezeDiagnosticsFgmReturnCount))
    {
        return "FGM COMMAND";
    }
    if ((last == NDS_FREEZE_BREADCRUMB_FLUSH) ||
        (last == NDS_FREEZE_BREADCRUMB_VBLANK_WAIT) ||
        (last == NDS_FREEZE_BREADCRUMB_DRAW_START))
    {
        return "GX/PRESENT";
    }
    if ((last == NDS_FREEZE_BREADCRUMB_HIT_SEARCH) ||
        (last == NDS_FREEZE_BREADCRUMB_DAMAGE_ENTER) ||
        (last == NDS_FREEZE_BREADCRUMB_EFFECT_SPAWN))
    {
        return "HIT/DAMAGE";
    }
    return "UNKNOWN";
}

static void ndsFreezeDiagnosticsSnapshotReportState(void)
{
    gNdsFreezeDiagnosticsReportGXStatus = GFX_STATUS;
    gNdsFreezeDiagnosticsReportPolygonCount = GFX_POLYGON_RAM_USAGE;
    gNdsFreezeDiagnosticsReportVertexCount = GFX_VERTEX_RAM_USAGE;
    gNdsFreezeDiagnosticsReportIpcFifo = REG_IPC_FIFO_CR;
    gNdsFreezeDiagnosticsReportIpcSync = REG_IPC_SYNC;
    gNdsFreezeDiagnosticsReportPresentedFrames =
        gNdsBattlePlayablePacingPresentedFrames;
    gNdsFreezeDiagnosticsReportLogicFrames =
        gNdsBattlePlayablePacingLogicFrames;
}

static void ndsFreezeDiagnosticsRenderStall(void)
{
    u32 row;
    u32 write_count = gNdsFreezeDiagnosticsBreadcrumbWriteCount;

    ndsFreezeDiagnosticsClearReport();
    ndsFreezeDiagnosticsPutText(0u, 0u, "FREEZE DIAGNOSTICS: STALL");
    ndsFreezeDiagnosticsPutText(1u, 0u, "PC ");
    ndsFreezeDiagnosticsPutHex(1u, 3u,
                               gNdsFreezeDiagnosticsInterruptedPC, 8u);
    ndsFreezeDiagnosticsPutText(1u, 12u, "LR ");
    ndsFreezeDiagnosticsPutHex(1u, 15u,
                               gNdsFreezeDiagnosticsInterruptedLR, 8u);
    ndsFreezeDiagnosticsPutText(2u, 0u, "CLASS ");
    ndsFreezeDiagnosticsPutText(2u, 6u,
                                ndsFreezeDiagnosticsClassifyStall());
    ndsFreezeDiagnosticsPutText(3u, 0u, "HB ");
    ndsFreezeDiagnosticsPutHex(3u, 3u,
                               gNdsFreezeDiagnosticsHeartbeat, 8u);
    ndsFreezeDiagnosticsPutText(3u, 12u, "LAST ");
    ndsFreezeDiagnosticsPutHex(3u, 17u,
                               gNdsFreezeDiagnosticsLastBreadcrumb, 8u);
    ndsFreezeDiagnosticsPutText(4u, 0u, "GX ");
    ndsFreezeDiagnosticsPutHex(4u, 3u,
                               gNdsFreezeDiagnosticsReportGXStatus, 8u);
    ndsFreezeDiagnosticsPutText(5u, 0u, "POLY ");
    ndsFreezeDiagnosticsPutHex(5u, 5u,
                               gNdsFreezeDiagnosticsReportPolygonCount, 4u);
    ndsFreezeDiagnosticsPutText(5u, 10u, "VERT ");
    ndsFreezeDiagnosticsPutHex(5u, 15u,
                               gNdsFreezeDiagnosticsReportVertexCount, 4u);
    ndsFreezeDiagnosticsPutText(5u, 20u, "SWAP ");
    ndsFreezeDiagnosticsPutHex(5u, 25u,
                               gNdsFreezeDiagnosticsSwapPending, 1u);
    ndsFreezeDiagnosticsPutText(6u, 0u, "IPC ");
    ndsFreezeDiagnosticsPutHex(6u, 4u,
                               gNdsFreezeDiagnosticsReportIpcFifo, 4u);
    ndsFreezeDiagnosticsPutText(6u, 9u, "SYNC ");
    ndsFreezeDiagnosticsPutHex(6u, 14u,
                               gNdsFreezeDiagnosticsReportIpcSync, 4u);
    ndsFreezeDiagnosticsPutText(7u, 0u, "FGM ENTER ");
    ndsFreezeDiagnosticsPutHex(7u, 10u,
                               gNdsFreezeDiagnosticsFgmEnterCount, 8u);
    ndsFreezeDiagnosticsPutText(8u, 0u, "FGM RETURN ");
    ndsFreezeDiagnosticsPutHex(8u, 11u,
                               gNdsFreezeDiagnosticsFgmReturnCount, 8u);
    ndsFreezeDiagnosticsPutText(9u, 0u, "FGM ID ");
    ndsFreezeDiagnosticsPutHex(9u, 7u,
                               gNdsFreezeDiagnosticsLastFgmID, 4u);
    ndsFreezeDiagnosticsPutText(9u, 12u, "CH ");
    ndsFreezeDiagnosticsPutHex(9u, 15u,
                               (u32)gNdsFreezeDiagnosticsLastFgmChannel, 2u);
    ndsFreezeDiagnosticsPutText(9u, 18u, "MASK ");
    ndsFreezeDiagnosticsPutHex(9u, 23u, gNdsAudioFgmChannelMask, 4u);
    ndsFreezeDiagnosticsPutText(10u, 0u, "FRAME ");
    ndsFreezeDiagnosticsPutHex(10u, 6u,
                               gNdsFreezeDiagnosticsReportPresentedFrames, 8u);
    ndsFreezeDiagnosticsPutText(11u, 0u, "UPDATE ");
    ndsFreezeDiagnosticsPutHex(11u, 7u,
                               gNdsFreezeDiagnosticsReportLogicFrames, 8u);
    for (row = 0u; row < NDS_FREEZE_DIAGNOSTICS_BREADCRUMB_COUNT; row++)
    {
        u32 index = (write_count - 1u - row) &
                    (NDS_FREEZE_DIAGNOSTICS_BREADCRUMB_COUNT - 1u);
        u32 report_row = 13u + row;

        ndsFreezeDiagnosticsPutText(report_row, 0u, "B");
        ndsFreezeDiagnosticsPutHex(report_row, 1u, row, 1u);
        ndsFreezeDiagnosticsPutText(report_row, 2u, " ");
        ndsFreezeDiagnosticsPutHex(
            report_row, 3u, gNdsFreezeDiagnosticsBreadcrumbs[index], 8u);
    }
}

static void ndsFreezeDiagnosticsRenderException(ExcptContext *context,
                                                unsigned flags)
{
    u32 row;
    u32 fault_address = getExceptionAddress(context, context->r[15]);

    ndsFreezeDiagnosticsClearReport();
    ndsFreezeDiagnosticsPutText(0u, 0u, "FREEZE DIAGNOSTICS: EXCEPTION");
    ndsFreezeDiagnosticsPutText(1u, 0u, "FLAGS ");
    ndsFreezeDiagnosticsPutHex(1u, 6u, flags, 8u);
    ndsFreezeDiagnosticsPutText(2u, 0u, "PC ");
    ndsFreezeDiagnosticsPutHex(2u, 3u, context->r[15], 8u);
    ndsFreezeDiagnosticsPutText(2u, 12u, "LR ");
    ndsFreezeDiagnosticsPutHex(2u, 15u, context->r[14], 8u);
    ndsFreezeDiagnosticsPutText(3u, 0u, "FAR ");
    ndsFreezeDiagnosticsPutHex(3u, 4u, fault_address, 8u);
    ndsFreezeDiagnosticsPutText(3u, 13u, "CPSR ");
    ndsFreezeDiagnosticsPutHex(3u, 18u, context->cpsr, 8u);
    ndsFreezeDiagnosticsPutText(4u, 0u, "LAST ");
    ndsFreezeDiagnosticsPutHex(4u, 5u,
                               gNdsFreezeDiagnosticsLastBreadcrumb, 8u);
    for (row = 0u; row < 7u; row++)
    {
        u32 first = row * 2u;
        u32 second = first + 1u;
        u32 report_row = 6u + row;

        ndsFreezeDiagnosticsPutText(report_row, 0u, "R");
        ndsFreezeDiagnosticsPutHex(report_row, 1u, first, 1u);
        ndsFreezeDiagnosticsPutText(report_row, 2u, " ");
        ndsFreezeDiagnosticsPutHex(report_row, 3u, context->r[first], 8u);
        ndsFreezeDiagnosticsPutText(report_row, 12u, "R");
        ndsFreezeDiagnosticsPutHex(report_row, 13u, second, 1u);
        ndsFreezeDiagnosticsPutText(report_row, 14u, " ");
        ndsFreezeDiagnosticsPutHex(report_row, 15u, context->r[second], 8u);
    }
}

static void ndsFreezeDiagnosticsExceptionHandler(ExcptContext *context,
                                                 unsigned flags)
{
    gNdsFreezeDiagnosticsReportKind =
        NDS_FREEZE_DIAGNOSTICS_REPORT_EXCEPTION;
    ndsFreezeDiagnosticsSnapshotReportState();
    ndsFreezeDiagnosticsRenderException(context, flags);
    for (;;)
    {
        __asm__ volatile("nop");
    }
}

void __attribute__((noinline, used)) ndsFreezeDiagnosticsStallMarker(void)
{
    __asm__ volatile("" ::: "memory");
}

static void ndsFreezeDiagnosticsWatchdog(TickTask *task)
{
    u32 heartbeat = gNdsFreezeDiagnosticsHeartbeat;

    (void)task;
    if (gNdsFreezeDiagnosticsForceTrip == 0u)
    {
        if (sNdsFreezeDiagnosticsWatchdogArmed == 0u)
        {
            if (heartbeat == 0u)
            {
                return;
            }
            sNdsFreezeDiagnosticsWatchdogArmed = 1u;
            sNdsFreezeDiagnosticsLastHeartbeat = heartbeat;
            return;
        }
        if (heartbeat != sNdsFreezeDiagnosticsLastHeartbeat)
        {
            sNdsFreezeDiagnosticsLastHeartbeat = heartbeat;
            sNdsFreezeDiagnosticsStaleSamples = 0u;
            return;
        }
        sNdsFreezeDiagnosticsStaleSamples++;
        if (sNdsFreezeDiagnosticsStaleSamples <
            NDS_FREEZE_DIAGNOSTICS_STALE_SAMPLES)
        {
            return;
        }
    }

    gNdsFreezeDiagnosticsWatchdogTripCount++;
    gNdsFreezeDiagnosticsReportKind = NDS_FREEZE_DIAGNOSTICS_REPORT_STALL;
    ndsFreezeDiagnosticsSnapshotReportState();
    ndsFreezeDiagnosticsRenderStall();
    ndsFreezeDiagnosticsStallMarker();
    for (;;)
    {
        __asm__ volatile("nop");
    }
}

void ndsFreezeDiagnosticsInit(void)
{
    PrintConsole *console = consoleGetDefault();

    if (sNdsFreezeDiagnosticsInitialized != 0u)
    {
        return;
    }
    sNdsFreezeDiagnosticsConsoleMap = console->fontBgMap;
    sNdsFreezeDiagnosticsFontBase = console->fontCharOffset;
    sNdsFreezeDiagnosticsFontPalette = (u16)(console->fontCurPal << 12);
    sNdsFreezeDiagnosticsAsciiOffset = console->font.asciiOffset;
    sNdsFreezeDiagnosticsCharacterCount = console->font.numChars;
    sNdsFreezeDiagnosticsLastHeartbeat = gNdsFreezeDiagnosticsHeartbeat;
    sNdsFreezeDiagnosticsStaleSamples = 0u;
    sNdsFreezeDiagnosticsWatchdogArmed = 0u;
    gNdsFreezeDiagnosticsOriginalIrqVector = SystemVectors.irq;
    SystemVectors.irq = ndsFreezeDiagnosticsIrqVector;
    setExceptionHandler(ndsFreezeDiagnosticsExceptionHandler);
    tickTaskStart(&sNdsFreezeDiagnosticsWatchdogTask,
                  ndsFreezeDiagnosticsWatchdog,
                  ticksFromHz(NDS_FREEZE_DIAGNOSTICS_WATCHDOG_HZ),
                  ticksFromHz(NDS_FREEZE_DIAGNOSTICS_WATCHDOG_HZ));
    sNdsFreezeDiagnosticsInitialized = 1u;
}

void ndsFreezeDiagnosticsBreadcrumb(u32 marker)
{
    u32 write_count = gNdsFreezeDiagnosticsBreadcrumbWriteCount;

    gNdsFreezeDiagnosticsLastBreadcrumb = marker;
    gNdsFreezeDiagnosticsBreadcrumbs[
        write_count & (NDS_FREEZE_DIAGNOSTICS_BREADCRUMB_COUNT - 1u)] = marker;
    gNdsFreezeDiagnosticsBreadcrumbWriteCount = write_count + 1u;
}

void ndsFreezeDiagnosticsFgmEnter(u16 fgm_id)
{
    gNdsFreezeDiagnosticsLastFgmID = fgm_id;
    gNdsFreezeDiagnosticsFgmEnterCount++;
    ndsFreezeDiagnosticsBreadcrumb(NDS_FREEZE_BREADCRUMB_FGM_ENTER);
}

void ndsFreezeDiagnosticsFgmReturn(s32 channel)
{
    gNdsFreezeDiagnosticsLastFgmChannel = channel;
    gNdsFreezeDiagnosticsFgmReturnCount++;
    ndsFreezeDiagnosticsBreadcrumb(NDS_FREEZE_BREADCRUMB_FGM_RETURN);
}

void ndsFreezeDiagnosticsFlush(void)
{
    gNdsFreezeDiagnosticsSwapPending = 1u;
    ndsFreezeDiagnosticsBreadcrumb(NDS_FREEZE_BREADCRUMB_FLUSH);
}

void ndsFreezeDiagnosticsVBlankWait(void)
{
    gNdsFreezeDiagnosticsSwapPending = 1u;
    ndsFreezeDiagnosticsBreadcrumb(NDS_FREEZE_BREADCRUMB_VBLANK_WAIT);
}

void ndsFreezeDiagnosticsHeartbeat(void)
{
    gNdsFreezeDiagnosticsSwapPending = 0u;
    gNdsFreezeDiagnosticsHeartbeat++;
    ndsFreezeDiagnosticsBreadcrumb(NDS_FREEZE_BREADCRUMB_PRESENT_DONE);
}
