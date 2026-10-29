"""Apply (or with --revert remove) the GXTRAP diagnostic to melonDS-mp's GPU3D.cpp.

Runtime-gated: inert unless MELONDS_GX_TRAP_LOG names a file. Logs
  * CPU writes to a geometry-command port (direct 0x440-0x5CB or the packed
    FIFO 0x400) while an ARM9 DMA in GXFIFO mode is enabled (mid-transfer);
  * packed headers holding a command the game's lists never pack (0x31, 0x34,
    0x50, or an undefined id), with the source of the header word;
each with the last 64 FIFO writes (value, CPU PC or DMA source, frame).

Usage: py -3 apply_gxtrap.py [--src PATH/GPU3D.cpp] [--revert]
Build, deploy and run: README.md beside this file.
"""
import sys
from pathlib import Path

DEFAULT_SRC = r"D:\Stuff\DevFolder\melonDS-mp\src\GPU3D.cpp"
BEGIN = "// GXTRAP-BEGIN"
END = "// GXTRAP-END"

HELPERS = r'''
// GXTRAP-BEGIN (Smash64DS diagnosis 2026-10-28; runtime-gated, see MELONDS_GX_TRAP_LOG)
#include <stdlib.h>
namespace
{
struct GxTrapWord { melonDS::u32 val, src, frame, kind; };
GxTrapWord gGxTrapRing[64];
melonDS::u32 gGxTrapPos = 0;
FILE* gGxTrapFile = nullptr;
int gGxTrapState = -1;
melonDS::u32 gGxTrapEvents = 0;

bool GxTrapOn()
{
    if (gGxTrapState < 0)
    {
        const char* p = getenv("MELONDS_GX_TRAP_LOG");
        gGxTrapFile = (p && *p) ? fopen(p, "a") : nullptr;
        gGxTrapState = gGxTrapFile ? 1 : 0;
        if (gGxTrapFile) { fprintf(gGxTrapFile, "GXTRAP armed\n"); fflush(gGxTrapFile); }
    }
    return gGxTrapState == 1;
}

bool GxTrapPackedOk(melonDS::u8 c)
{
    if (c == 0x00) return true;
    if (c >= 0x10 && c <= 0x1C) return true;
    if (c >= 0x20 && c <= 0x2B) return true;
    if (c == 0x30 || c == 0x32 || c == 0x33) return true;
    if (c == 0x40 || c == 0x41 || c == 0x60) return true;
    if (c >= 0x70 && c <= 0x72) return true;
    return false;
}

// kind: 0 CPU packed, 1 DMA packed, 2 CPU direct (src = PC, val = addr<<20|param low bits not kept)
void GxTrapRecord(melonDS::u32 val, melonDS::u32 src, melonDS::u32 frame, melonDS::u32 kind)
{
    GxTrapWord& w = gGxTrapRing[gGxTrapPos++ & 63];
    w.val = val; w.src = src; w.frame = frame; w.kind = kind;
}

void GxTrapDump(const char* what, melonDS::u32 a, melonDS::u32 b, melonDS::u32 c, melonDS::u32 d, melonDS::u32 frame)
{
    gGxTrapEvents++;
    if (gGxTrapEvents > 200) return;
    fprintf(gGxTrapFile, "EVENT %u frame=%u %s a=%08X b=%08X c=%08X d=%08X\n",
            gGxTrapEvents, frame, what, a, b, c, d);
    if (gGxTrapEvents <= 12)
    {
        for (melonDS::u32 i = 0; i < 64; i++)
        {
            const GxTrapWord& w = gGxTrapRing[(gGxTrapPos + i) & 63];
            fprintf(gGxTrapFile, "  ring kind=%u val=%08X src=%08X frame=%u\n", w.kind, w.val, w.src, w.frame);
        }
    }
    fflush(gGxTrapFile);
}
}
// GXTRAP-END
'''

WRITE_FIFO_OLD = '''void GPU3D::WriteToGXFIFO(u32 val) noexcept
{
    if (NumCommands == 0)
    {'''

WRITE_FIFO_NEW = '''void GPU3D::WriteToGXFIFO(u32 val) noexcept
{
    // GXTRAP-BEGIN
    if (GxTrapOn())
    {
        u32 src = 0, kind = 0;
        for (int d = 0; d < 4; d++)
        {
            if (NDS.DMAs[d].Executing) { src = NDS.DMAs[d].CurSrcAddr; kind = 1; break; }
        }
        if (kind == 0) src = NDS.GetPC(0);
        GxTrapRecord(val, src, NDS.NumFrames, kind);
        if (NumCommands == 0)
        {
            bool bad = false;
            for (int i = 0; i < 4; i++)
            {
                u8 c = (val >> (i * 8)) & 0xFF;
                if (!GxTrapPackedOk(c)) bad = true;
                if (c == 0) break;
            }
            if (bad) GxTrapDump("BAD_HEADER", val, src, kind, NDS.DMAs[0].Cnt, NDS.NumFrames);
        }
        if (kind == 0)
        {
            for (int d = 0; d < 4; d++)
            {
                if ((NDS.DMAs[d].Cnt & 0x80000000) && (NDS.DMAs[d].StartMode == 0x07))
                    GxTrapDump("CPU_PACKED_DURING_GXDMA", val, src, NDS.DMAs[d].CurSrcAddr, NDS.DMAs[d].Cnt, NDS.NumFrames);
            }
        }
    }
    // GXTRAP-END
    if (NumCommands == 0)
    {'''

DIRECT_OLD = '''    if (addr >= 0x04000440 && addr < 0x040005CC)
    {
        CmdFIFOEntry entry;
        entry.Command = (addr & 0x1FC) >> 2;
        entry.Param = val;
        CmdFIFOWrite(entry);
        return;
    }'''

DIRECT_NEW = '''    if (addr >= 0x04000440 && addr < 0x040005CC)
    {
        // GXTRAP-BEGIN
        if (GxTrapOn())
        {
            u32 pc = NDS.GetPC(0);
            GxTrapRecord(((addr & 0x1FC) >> 2) << 24 | (val & 0xFFFFFF), pc, NDS.NumFrames, 2);
            for (int d = 0; d < 4; d++)
            {
                if ((NDS.DMAs[d].Cnt & 0x80000000) && (NDS.DMAs[d].StartMode == 0x07))
                    GxTrapDump("CPU_DIRECT_DURING_GXDMA", addr, pc, NDS.DMAs[d].CurSrcAddr, NDS.DMAs[d].Cnt, NDS.NumFrames);
            }
            if (NumCommands != 0)
                GxTrapDump("CPU_DIRECT_INSIDE_PACKED", addr, pc, CurCommand, (NumCommands << 16) | ParamCount, NDS.NumFrames);
        }
        // GXTRAP-END
        CmdFIFOEntry entry;
        entry.Command = (addr & 0x1FC) >> 2;
        entry.Param = val;
        CmdFIFOWrite(entry);
        return;
    }'''

ANCHOR = '#include "GPU3D.h"\n'


def apply(text):
    if BEGIN in text:
        raise SystemExit("already applied")
    assert text.count(ANCHOR) == 1
    assert text.count(WRITE_FIFO_OLD) == 1
    assert text.count(DIRECT_OLD) == 1, text.count(DIRECT_OLD)
    text = text.replace(ANCHOR, ANCHOR + HELPERS, 1)
    text = text.replace(WRITE_FIFO_OLD, WRITE_FIFO_NEW, 1)
    text = text.replace(DIRECT_OLD, DIRECT_NEW, 1)
    return text


def revert(text):
    text = text.replace(ANCHOR + HELPERS, ANCHOR, 1)
    text = text.replace(WRITE_FIFO_NEW, WRITE_FIFO_OLD, 1)
    text = text.replace(DIRECT_NEW, DIRECT_OLD, 1)
    if BEGIN in text:
        raise SystemExit("revert incomplete")
    return text


def main():
    args = sys.argv[1:]
    src = Path(args[args.index("--src") + 1]) if "--src" in args else Path(DEFAULT_SRC)
    raw = src.read_bytes()
    crlf = b"\r\n" in raw
    text = raw.decode("utf-8").replace("\r\n", "\n")
    text = revert(text) if "--revert" in sys.argv else apply(text)
    out = text.replace("\n", "\r\n") if crlf else text
    src.write_bytes(out.encode("utf-8"))
    print("reverted" if "--revert" in sys.argv else "applied", src, "crlf" if crlf else "lf")


if __name__ == "__main__":
    main()
