#include <nds.h>

#include <calico/system/tick.h>

#include <nds/nds_audio_bgm.h>
#include <nds/nds_audio_fgm.h>
#include <nds/nds_freeze_diagnostics.h>
#include <nds/nds_startup.h>

#define NDS_FREEZE_DIAGNOSTICS_REPORT_STALL 0x5354414cu /* STAL */
#define NDS_FREEZE_DIAGNOSTICS_REPORT_EXCEPTION 0x45584350u /* EXCP */
#define NDS_FREEZE_DIAGNOSTICS_WATCHDOG_HZ 1u
/* A battle frame that has not finished for three to four seconds. The
 * lockstep's wait for another console beats the heartbeat itself
 * (ndsFreezeDiagnosticsNetWait), so a stalled link is not a freeze. */
#define NDS_FREEZE_DIAGNOSTICS_STALE_SAMPLES 3u
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
/* The game's thread, so a report can show where it waits when it is blocked
 * (the interrupted PC is then the idle thread's). */
static Thread *sNdsFreezeDiagnosticsMainThread;
static u32 sNdsFreezeDiagnosticsReporting;
/* The lower screen's sprite enable before a report hid it (the battle HUD's
 * timer and damage meters are sprites over the text). */
static u32 sNdsFreezeDiagnosticsSubSprites;

static void ndsFreezeDiagnosticsShowSubSprites(void)
{
    REG_DISPCNT_SUB |= sNdsFreezeDiagnosticsSubSprites;
    sNdsFreezeDiagnosticsSubSprites = 0u;
}

/* Counters the report shows when their owners are linked (the net session,
 * the ARM9 storage client, the radio link, the BGM client). */
extern volatile u32 gNdsNetSessionState __attribute__((weak));
extern volatile u32 gNdsNetBatch __attribute__((weak));
extern volatile u32 gNdsNetStallMaxVBlanks __attribute__((weak));
extern volatile u32 gNdsNetPacketsSent __attribute__((weak));
extern volatile u32 gNdsNetPacketsRecv __attribute__((weak));
extern volatile uint32_t gNdsAudioStorageRequests __attribute__((weak));
extern volatile uint32_t gNdsAudioStorageFailures __attribute__((weak));
extern u32 ndsNetLinkStat(u32 index) __attribute__((weak));
extern u32 ndsAudioBgmReportSequence(void) __attribute__((weak));

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

static u32 ndsFreezeDiagnosticsPutFourCC(u32 row, u32 column, u32 value)
{
    u32 shift;

    for (shift = 32u; shift != 0u; shift -= 8u)
    {
        const char c = (char)((value >> (shift - 8u)) & 0xffu);

        ndsFreezeDiagnosticsPutCharacter(row, column++,
                                         ((c >= ' ') && (c <= '~')) ? c : '.');
    }
    return column;
}

static u32 ndsFreezeDiagnosticsWeak(const volatile u32 *word)
{
    return (word != NULL) ? *word : 0xffffffffu;
}

static u32 ndsFreezeDiagnosticsLinkStat(u32 index)
{
    return (ndsNetLinkStat != NULL) ? ndsNetLinkStat(index) : 0xffffffffu;
}

/* A stacked return address: Thumb code in main RAM (bit 0 set) or any word
 * in ITCM. */
static u32 ndsFreezeDiagnosticsIsCodeWord(u32 value)
{
    if ((value >= 0x01ff8000u) && (value < 0x02000000u))
    {
        return 1u;
    }
    return (((value & 1u) != 0u) && (value >= 0x02000000u) &&
            (value < 0x02400000u)) ? 1u : 0u;
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

static void ndsFreezeDiagnosticsRenderStall(u32 resumed)
{
    const u32 write_count = gNdsFreezeDiagnosticsBreadcrumbWriteCount;
    Thread *const main_thread = sNdsFreezeDiagnosticsMainThread;
    u32 main_pc = gNdsFreezeDiagnosticsInterruptedPC;
    u32 main_lr = gNdsFreezeDiagnosticsInterruptedLR;
    u32 main_sp = 0u;
    u32 main_status = 0xfu;
    u32 found = 0u;
    u32 row;

    /* Running when the timer fired: the interrupted PC is the game's own.
     * Blocked: its saved context says where it waits. */
    if (main_thread != NULL)
    {
        main_status = (u32)main_thread->status;
        if (main_thread != threadGetSelf())
        {
            main_pc = main_thread->ctx.r[15];
            main_lr = main_thread->ctx.r[14];
            main_sp = main_thread->ctx.r[13];
        }
    }

    ndsFreezeDiagnosticsClearReport();
    ndsFreezeDiagnosticsPutText(0u, 0u, (resumed != 0u) ?
        "FREEZE REPORT (RESUMED)" : "FREEZE REPORT: BATTLE STALL");
    ndsFreezeDiagnosticsPutText(1u, 0u, "PC ");
    ndsFreezeDiagnosticsPutHex(1u, 3u, gNdsFreezeDiagnosticsInterruptedPC, 8u);
    ndsFreezeDiagnosticsPutText(1u, 12u, "LR ");
    ndsFreezeDiagnosticsPutHex(1u, 15u, gNdsFreezeDiagnosticsInterruptedLR, 8u);
    ndsFreezeDiagnosticsPutText(2u, 0u, "MAIN ");
    ndsFreezeDiagnosticsPutHex(2u, 5u, main_status, 1u);
    ndsFreezeDiagnosticsPutText(2u, 7u, "PC ");
    ndsFreezeDiagnosticsPutHex(2u, 10u, main_pc, 8u);
    ndsFreezeDiagnosticsPutText(2u, 19u, "LR ");
    ndsFreezeDiagnosticsPutHex(2u, 22u, main_lr, 8u);
    ndsFreezeDiagnosticsPutText(3u, 0u, "SP ");
    ndsFreezeDiagnosticsPutHex(3u, 3u, main_sp, 8u);
    ndsFreezeDiagnosticsPutText(3u, 12u, "HB ");
    ndsFreezeDiagnosticsPutHex(3u, 15u, gNdsFreezeDiagnosticsHeartbeat, 8u);
    /* Up to six return addresses on the blocked thread's stack. */
    ndsFreezeDiagnosticsPutText(4u, 0u, "STK");
    if ((main_sp >= 0x02000000u) && (main_sp < 0x03000000u) &&
        ((main_sp & 3u) == 0u))
    {
        const u32 *stack = (const u32 *)main_sp;
        u32 i;

        for (i = 0u; (i < 96u) && (found < 6u); i++)
        {
            const u32 value = stack[i];

            if (ndsFreezeDiagnosticsIsCodeWord(value) == 0u)
            {
                continue;
            }
            ndsFreezeDiagnosticsPutHex(4u + found / 3u, (found % 3u) * 9u + 4u,
                                       value, 8u);
            found++;
        }
    }
    ndsFreezeDiagnosticsPutText(6u, 0u, "BGM H");
    ndsFreezeDiagnosticsPutHex(6u, 5u, gNdsAudioBgmArm7Failure, 1u);
    ndsFreezeDiagnosticsPutText(6u, 7u, "ES ");
    ndsFreezeDiagnosticsPutHex(6u, 10u, gNdsAudioBgmErrorStopCount, 2u);
    ndsFreezeDiagnosticsPutText(6u, 13u, "ER ");
    ndsFreezeDiagnosticsPutHex(6u, 16u, gNdsAudioBgmLastError, 1u);
    ndsFreezeDiagnosticsPutText(6u, 18u, "RC ");
    ndsFreezeDiagnosticsPutHex(6u, 21u, gNdsAudioBgmRecoveries, 2u);
    ndsFreezeDiagnosticsPutText(6u, 24u, "RF ");
    ndsFreezeDiagnosticsPutHex(6u, 27u, gNdsAudioBgmCommandRefusals, 2u);
    /* Live: the ARM7 rewrites its report at every seam of a playing track. */
    ndsFreezeDiagnosticsPutText(7u, 0u, "BGM SEQ ");
    ndsFreezeDiagnosticsPutHex(7u, 8u, (ndsAudioBgmReportSequence != NULL) ?
                               ndsAudioBgmReportSequence() : 0xffffffffu, 8u);
    ndsFreezeDiagnosticsPutText(7u, 17u, "STO ");
    ndsFreezeDiagnosticsPutHex(7u, 21u,
        ndsFreezeDiagnosticsWeak(&gNdsAudioStorageRequests), 6u);
    ndsFreezeDiagnosticsPutText(7u, 28u, "F");
    ndsFreezeDiagnosticsPutHex(7u, 29u,
        ndsFreezeDiagnosticsWeak(&gNdsAudioStorageFailures), 2u);
    ndsFreezeDiagnosticsPutText(8u, 0u, "NET ");
    ndsFreezeDiagnosticsPutHex(8u, 4u,
        ndsFreezeDiagnosticsWeak(&gNdsNetSessionState), 1u);
    ndsFreezeDiagnosticsPutText(8u, 6u, "B ");
    ndsFreezeDiagnosticsPutHex(8u, 8u, ndsFreezeDiagnosticsWeak(&gNdsNetBatch), 6u);
    ndsFreezeDiagnosticsPutText(8u, 15u, "STALL ");
    ndsFreezeDiagnosticsPutHex(8u, 21u,
        ndsFreezeDiagnosticsWeak(&gNdsNetStallMaxVBlanks), 4u);
    ndsFreezeDiagnosticsPutText(9u, 0u, "PKT TX ");
    ndsFreezeDiagnosticsPutHex(9u, 7u,
        ndsFreezeDiagnosticsWeak(&gNdsNetPacketsSent), 6u);
    ndsFreezeDiagnosticsPutText(9u, 14u, "RX ");
    ndsFreezeDiagnosticsPutHex(9u, 17u,
        ndsFreezeDiagnosticsWeak(&gNdsNetPacketsRecv), 6u);
    /* Live, from the ARM7 radio module: frames sent and received
     * (NDS_NET_STAT_TX_SENT, _RX_FRAMES, _TX_NO_BUFFER, _RX_RING_FULL). */
    ndsFreezeDiagnosticsPutText(10u, 0u, "RADIO TX ");
    ndsFreezeDiagnosticsPutHex(10u, 9u, ndsFreezeDiagnosticsLinkStat(0u), 6u);
    ndsFreezeDiagnosticsPutText(10u, 16u, "RX ");
    ndsFreezeDiagnosticsPutHex(10u, 19u, ndsFreezeDiagnosticsLinkStat(3u), 6u);
    ndsFreezeDiagnosticsPutText(11u, 0u, "NOBUF ");
    ndsFreezeDiagnosticsPutHex(11u, 6u, ndsFreezeDiagnosticsLinkStat(6u), 4u);
    ndsFreezeDiagnosticsPutText(11u, 11u, "RXFULL ");
    ndsFreezeDiagnosticsPutHex(11u, 18u, ndsFreezeDiagnosticsLinkStat(4u), 4u);
    ndsFreezeDiagnosticsPutText(12u, 0u, "IPC ");
    ndsFreezeDiagnosticsPutHex(12u, 4u, REG_IPC_FIFO_CR, 4u);
    ndsFreezeDiagnosticsPutText(12u, 9u, "GX ");
    ndsFreezeDiagnosticsPutHex(12u, 12u, gNdsFreezeDiagnosticsReportGXStatus, 8u);
    ndsFreezeDiagnosticsPutText(12u, 21u, "FGM ");
    ndsFreezeDiagnosticsPutHex(12u, 25u,
        gNdsFreezeDiagnosticsFgmEnterCount - gNdsFreezeDiagnosticsFgmReturnCount, 2u);
    ndsFreezeDiagnosticsPutText(13u, 0u, "FRAME ");
    ndsFreezeDiagnosticsPutHex(13u, 6u, gNdsFreezeDiagnosticsReportPresentedFrames, 8u);
    ndsFreezeDiagnosticsPutText(13u, 15u, "UPD ");
    ndsFreezeDiagnosticsPutHex(13u, 19u, gNdsFreezeDiagnosticsReportLogicFrames, 8u);
    /* The last breadcrumbs, newest first, as their four letters. */
    for (row = 0u; row < NDS_FREEZE_DIAGNOSTICS_BREADCRUMB_COUNT; row++)
    {
        const u32 index = (write_count - 1u - row) &
                          (NDS_FREEZE_DIAGNOSTICS_BREADCRUMB_COUNT - 1u);

        ndsFreezeDiagnosticsPutFourCC(15u + row / 4u, (row % 4u) * 6u,
                                      gNdsFreezeDiagnosticsBreadcrumbs[index]);
    }
    ndsFreezeDiagnosticsPutText(18u, 0u, "Please photo this screen.");
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
            if (sNdsFreezeDiagnosticsReporting != 0u)
            {
                /* The game moved again: say so and stop redrawing. */
                sNdsFreezeDiagnosticsReporting = 0u;
                ndsFreezeDiagnosticsShowSubSprites();
                ndsFreezeDiagnosticsRenderStall(1u);
            }
            return;
        }
        sNdsFreezeDiagnosticsStaleSamples++;
        if (sNdsFreezeDiagnosticsStaleSamples <
            NDS_FREEZE_DIAGNOSTICS_STALE_SAMPLES)
        {
            return;
        }
    }

    /* Redrawn every second while the game stays stuck, so the live rows (BGM
     * SEQ, RADIO) show whether the ARM7 still runs. No halt: a frame that
     * does finish resumes the game. */
    if (sNdsFreezeDiagnosticsReporting == 0u)
    {
        sNdsFreezeDiagnosticsReporting = 1u;
        sNdsFreezeDiagnosticsSubSprites = REG_DISPCNT_SUB & DISPLAY_SPR_ACTIVE;
        REG_DISPCNT_SUB &= ~DISPLAY_SPR_ACTIVE;
        gNdsFreezeDiagnosticsWatchdogTripCount++;
        gNdsFreezeDiagnosticsReportKind = NDS_FREEZE_DIAGNOSTICS_REPORT_STALL;
        ndsFreezeDiagnosticsSnapshotReportState();
        ndsFreezeDiagnosticsStallMarker();
    }
    gNdsFreezeDiagnosticsForceTrip = 0u;
    ndsFreezeDiagnosticsRenderStall(0u);
}

void ndsFreezeDiagnosticsDisarm(void)
{
    const ArmIrqState state = armIrqLockByPsr();

    gNdsFreezeDiagnosticsHeartbeat = 0u;
    sNdsFreezeDiagnosticsWatchdogArmed = 0u;
    sNdsFreezeDiagnosticsStaleSamples = 0u;
    sNdsFreezeDiagnosticsReporting = 0u;
    ndsFreezeDiagnosticsShowSubSprites();
    armIrqUnlockByPsr(state);
}

void ndsFreezeDiagnosticsNetWait(void)
{
    gNdsFreezeDiagnosticsHeartbeat++;
    ndsFreezeDiagnosticsBreadcrumb(NDS_FREEZE_BREADCRUMB_NET_WAIT);
}

void ndsFreezeDiagnosticsInit(void)
{
    /* The live console (the platform's consoleInit(NULL, ...)): libnds's
     * consoleGetDefault() is only the template it was copied from, whose map
     * pointer is NULL, so a report drawn through it never showed. */
    PrintConsole *console = consoleSelect(NULL);

    consoleSelect(console);
    if (console == NULL)
    {
        console = consoleGetDefault();
    }

    if (sNdsFreezeDiagnosticsInitialized != 0u)
    {
        return;
    }
    sNdsFreezeDiagnosticsConsoleMap = console->fontBgMap;
    sNdsFreezeDiagnosticsFontBase = console->fontCharOffset;
    /* consoleInit stores the palette already shifted into map-entry bits;
     * shift only a bare palette number. */
    sNdsFreezeDiagnosticsFontPalette = (console->fontCurPal < 16u) ?
        (u16)(console->fontCurPal << 12) : (u16)console->fontCurPal;
    sNdsFreezeDiagnosticsAsciiOffset = console->font.asciiOffset;
    sNdsFreezeDiagnosticsCharacterCount = console->font.numChars;
    sNdsFreezeDiagnosticsLastHeartbeat = gNdsFreezeDiagnosticsHeartbeat;
    sNdsFreezeDiagnosticsStaleSamples = 0u;
    sNdsFreezeDiagnosticsWatchdogArmed = 0u;
    sNdsFreezeDiagnosticsMainThread = threadGetSelf();
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
