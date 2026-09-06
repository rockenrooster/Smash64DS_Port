#!/usr/bin/env python3
"""Runtime proof for the actual C spline normalizer (2026-09-06).

Measured result: this test compiles the verbatim C functions from
src/port/reloc_backend_assets.c and runs them on real RollF and RollB
source data. Proposal: none in this file, only checks.

Coverage gap: scripts/test_samus_roll_spline.py proves the loader with a
Python mirror of ndsRelocSYInterpDescHeaderNative before the original
syInterpCubic. This test closes the gap by executing the actual C
ndsRelocSYInterpDescHeaderNative and ndsRelocNormalizeAObj16Script on the
host. Static substring checks alone would not satisfy this test.

Source proof: the C text is extracted verbatim from
src/port/reloc_backend_assets.c at run time by brace matching. The test
fails if any signature moves. Opcode values are derived from
decomp/BattleShip-main/decomp/src/sys/objdef.h enum order.

Cases, all bounded:
  1. Header unit on the real C function, swapped 0x02000005 becomes
     0x00050002 which reads kind 2 and points_num 5.
  2. Valid RollF (FTSamusAnim060, id 1013) and RollB (FTSamusAnim061,
     id 1014): full payload after u32 swap, internal fixups, and script
     lane swap, with table 0x60 and scripts from 0x130 and descriptor at
     0x118. The actual C normalizer must return unresolved 0 and fix the
     header to kind 2 and points_num 5.
  3. Error path: one SetTranslateInterp command points outside the
     descriptor bounds. The actual C function must return unresolved 1
     and must leave the entire payload byte identical. Usable raw cache
     means zero bytes change, so a declined prebake entry normalizes
     exactly once later instead of twice.
  4. Repeated descriptor in one script: a synthetic second TraI naming
     the same 0x118 descriptor. The header fix is not idempotent, so the
     actual C function must refuse (unresolved 1) and leave the entire
     payload byte identical. Limit: sharing one descriptor across two
     scripts has no real Roll coverage (each Roll file carries exactly
     one TraI); the file-level validator refuses it the same way, and
     the corpus invariant holds one TraI pass per descriptor.

Run: python scripts/test_samus_roll_runtime_normalizer.py
Scratch: builds/resume-20260905/samus_roll_runtime_normalizer_work
"""

from __future__ import annotations

import importlib.util
import pathlib
import shutil
import struct
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
C_PATH = ROOT / "src" / "port" / "reloc_backend_assets.c"
OBJDEF_PATH = (ROOT / "decomp" / "BattleShip-main" / "decomp" / "src"
               / "sys" / "objdef.h")
BANK = (ROOT / "decomp" / "BattleShip-main" / "BattleShip_o2r"
        / "reloc_animations")
WORK = (ROOT / "builds" / "resume-20260905"
        / "samus_roll_runtime_normalizer_work")

ROLLS = [("FTSamusAnim060", 1013), ("FTSamusAnim061", 1014)]
SRC_TABLE_BYTES = 0x60
SRC_SCRIPT_START = 0x130
SRC_DESC_OFF = 0x118
SRC_KIND = 2
SRC_POINTS_NUM = 5
SRC_LENGTH_BITS = 0x421A3F58
SRC_PTRS = (0x60, 0xB4, 0xC8)

WANT_FUNCS = [
    "static u32 ndsRelocReadNative32(const void *addr)",
    "static void ndsRelocWriteNative32(void *addr, u32 value)",
    "static u16 ndsRelocReadNative16(const void *addr)",
    "static void ndsRelocWriteNative16(void *addr, u16 value)",
    "static u16 ndsRelocAObj16EncodeForNativeBitfields(u16 source)",
    "static u32 ndsRelocAObj16FlagCount(u16 flags)",
    "static u32 ndsRelocAObj16CommandWords(u16 opcode, u16 flags, u16 toggle)",
    "static u32 ndsRelocSYInterpDescHeaderNative(u32 swapped)",
    "static u32 ndsRelocValidateAObj16Script(const u16 *script, u32 word_count,",
    "static u32 ndsRelocNormalizeAObj16Script(u16 *script, u32 word_count,",
]


def _extract_func(src: str, sig: str) -> str:
    """Verbatim extraction by brace matching. Measured source text."""
    start = src.find(sig)
    assert start >= 0, "C source moved, missing: %s" % sig
    brace = src.find("{", start)
    assert brace > start, "C source moved, no body for: %s" % sig
    depth = 0
    i = brace
    while i < len(src):
        ch = src[i]
        if ch == "{":
            depth += 1
        elif ch == "}":
            depth -= 1
            if depth == 0:
                return src[start:i + 1] + "\n"
        i += 1
    raise AssertionError("C source moved, unterminated: %s" % sig)


def _load(name: str):
    spec = importlib.util.spec_from_file_location(
        name, ROOT / "scripts" / ("%s.py" % name))
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


def _opcode_values() -> dict:
    """Measured enum order from objdef.h, the source of truth."""
    text = OBJDEF_PATH.read_text(encoding="utf-8")
    start = text.find("typedef enum AObjEvent16Kind")
    assert start >= 0, "objdef.h moved, AObjEvent16Kind missing"
    body = text[start:text.find("} AObjEvent16Kind;", start)]
    names = []
    for line in body.splitlines():
        line = line.split("//")[0].strip().rstrip(",")
        if line.startswith("nGCAnimEvent16") or line.startswith("nGCAnimEvent1611"):
            names.append(line.split()[0])
    want = ["nGCAnimEvent16End", "nGCAnimEvent16Block",
            "nGCAnimEvent16SetValBlock", "nGCAnimEvent16SetVal",
            "nGCAnimEvent16SetValRateBlock", "nGCAnimEvent16SetValRate",
            "nGCAnimEvent16SetTargetRate", "nGCAnimEvent16SetVal0RateBlock",
            "nGCAnimEvent16SetVal0Rate", "nGCAnimEvent16SetValAfterBlock",
            "nGCAnimEvent16SetValAfter", "nGCAnimEvent1611",
            "nGCAnimEvent16SetTranslateInterp", "nGCAnimEvent16Loop",
            "nGCAnimEvent16SetFlags"]
    assert names == want, "objdef.h AObjEvent16 order moved: %r" % names
    return {name: i for i, name in enumerate(names)}


def _build_harness(tmp: pathlib.Path, funcs: str, opcodes: dict) -> pathlib.Path:
    src = tmp / "runtime_normalizer.c"
    exe = tmp / "runtime_normalizer.exe"
    harness = r"""
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32;
typedef int16_t s16; typedef int32_t s32;
#define NDS_TASK85_ALIGNED_NATIVE_ACCESS 0
#define nGCAnimEvent16End %END%
#define nGCAnimEvent16Block %BLOCK%
#define nGCAnimEvent16SetValBlock %SETVALBLOCK%
#define nGCAnimEvent16SetVal %SETVAL%
#define nGCAnimEvent16SetValRateBlock %SETVALRATEBLOCK%
#define nGCAnimEvent16SetValRate %SETVALRATE%
#define nGCAnimEvent16SetTargetRate %SETTARGETRATE%
#define nGCAnimEvent16SetVal0RateBlock %SETVAL0RATEBLOCK%
#define nGCAnimEvent16SetVal0Rate %SETVAL0RATE%
#define nGCAnimEvent16SetValAfterBlock %SETVALAFTERBLOCK%
#define nGCAnimEvent16SetValAfter %SETVALAFTER%
#define nGCAnimEvent16SetTranslateInterp %INTERP%
#define nGCAnimEvent16Loop %LOOP%
#define nGCAnimEvent16SetFlags %SETFLAGS%
volatile u32 gNdsRelocSYInterpDescFixCount;
volatile u32 gNdsRelocSYInterpDescUnresolvedCount;
volatile u32 gNdsRelocSYInterpDescUnresolvedFirstAsset;
%FUNCS%
static u32 rd32le(const u8 *p) { u32 v; memcpy(&v, p, 4); return v; }
int main(int argc, char **argv) {
    const char *mode;
    FILE *f;
    long n;
    static u8 data[8192];
    u32 interp_start, interp_end, script_off, word_count;
    u32 before_desc, after_desc, unresolved;
    u16 before_cmd, after_cmd;
    if (argc < 2) { fprintf(stderr, "usage\n"); return 2; }
    mode = argv[1];
    if (strcmp(mode, "header") == 0) {
        u32 swapped = (u32)strtoul(argv[2], NULL, 0);
        printf("HEADER %08x\n", ndsRelocSYInterpDescHeaderNative(swapped));
        return 0;
    }
    if (argc < 8) { fprintf(stderr, "usage: prog run image out script_off words istart iend\n"); return 2; }
    f = fopen(argv[2], "rb");
    if (!f) { perror(argv[2]); return 2; }
    fseek(f, 0, SEEK_END); n = ftell(f); fseek(f, 0, SEEK_SET);
    if (n <= 0 || n > (long)sizeof(data)) { fprintf(stderr, "bad image size %ld\n", n); return 2; }
    if (fread(data, 1, (size_t)n, f) != (size_t)n) return 2;
    fclose(f);
    script_off = (u32)strtoul(argv[4], NULL, 0);
    word_count = (u32)strtoul(argv[5], NULL, 0);
    interp_start = (u32)strtoul(argv[6], NULL, 0);
    interp_end = (u32)strtoul(argv[7], NULL, 0);
    before_desc = rd32le(data + 0x118);
    before_cmd = (u16)(data[script_off] | ((u16)data[script_off + 1] << 8));
    gNdsRelocSYInterpDescFixCount = 0;
    unresolved = ndsRelocNormalizeAObj16Script((u16 *)(data + script_off),
        word_count, data, interp_start, interp_end);
    after_desc = rd32le(data + 0x118);
    after_cmd = (u16)(data[script_off] | ((u16)data[script_off + 1] << 8));
    printf("RUN unresolved=%u fix=%u before_desc=%08x after_desc=%08x before_cmd=%04x after_cmd=%04x\n",
        unresolved, gNdsRelocSYInterpDescFixCount,
        before_desc, after_desc, before_cmd, after_cmd);
    {
        FILE *o = fopen(argv[3], "wb");
        if (!o) { perror(argv[3]); return 2; }
        if (fwrite(data, 1, (size_t)n, o) != (size_t)n) { fclose(o); return 2; }
        fclose(o);
    }
    return 0;
}
"""
    harness = harness.replace("%FUNCS%", funcs)
    for key, macro in [("nGCAnimEvent16End", "END"),
                       ("nGCAnimEvent16Block", "BLOCK"),
                       ("nGCAnimEvent16SetValBlock", "SETVALBLOCK"),
                       ("nGCAnimEvent16SetVal", "SETVAL"),
                       ("nGCAnimEvent16SetValRateBlock", "SETVALRATEBLOCK"),
                       ("nGCAnimEvent16SetValRate", "SETVALRATE"),
                       ("nGCAnimEvent16SetTargetRate", "SETTARGETRATE"),
                       ("nGCAnimEvent16SetVal0RateBlock", "SETVAL0RATEBLOCK"),
                       ("nGCAnimEvent16SetVal0Rate", "SETVAL0RATE"),
                       ("nGCAnimEvent16SetValAfterBlock", "SETVALAFTERBLOCK"),
                       ("nGCAnimEvent16SetValAfter", "SETVALAFTER"),
                       ("nGCAnimEvent16SetTranslateInterp", "INTERP"),
                       ("nGCAnimEvent16Loop", "LOOP"),
                       ("nGCAnimEvent16SetFlags", "SETFLAGS")]:
        harness = harness.replace("%%%s%%" % macro, str(opcodes[key]))
    src.write_text(harness, encoding="utf-8")
    cc = (shutil.which("gcc") or shutil.which("cc")
          or shutil.which("clang"))
    assert cc, "no host C compiler found"
    build = subprocess.run([cc, "-O1", "-w", "-std=gnu99", str(src),
                            "-o", str(exe)],
                           capture_output=True, text=True, timeout=60)
    assert build.returncode == 0, "harness build failed:\n%s" % build.stderr
    return exe


def _prepare_image(probe, name: str) -> dict:
    """Measured loader state from real source bytes, before C normalize."""
    raw = (BANK / name).read_bytes()
    f = probe.load(raw)
    assert f is not None, "%s is not an O2R RELO file" % name
    fx = probe.fixups(f)
    image = f["data"]
    for slot, target in fx.items():
        struct.pack_into("<I", image, slot, target)
    table, script_start = probe.normalize_full(f, fx)
    return {"image": bytes(image), "size": f["size"], "table": table,
            "script_start": script_start, "file_id": f["file_id"]}


def _run_case(exe: pathlib.Path, image: bytes, script_off: int,
              words: int, istart: int, iend: int) -> tuple:
    raise AssertionError("unused helper, _run_on_file owns harness runs")


def _run_on_file(exe: pathlib.Path, tmp: pathlib.Path, tag: str,
                 image: bytes, script_off: int, words: int,
                 istart: int, iend: int) -> tuple:
    path = tmp / ("%s.bin" % tag)
    out = tmp / ("%s_out.bin" % tag)
    path.write_bytes(image)
    if out.exists():
        out.unlink()
    proc = subprocess.run([str(exe), "run", str(path), str(out),
                           "0x%x" % script_off, "%u" % words,
                           "0x%x" % istart, "0x%x" % iend],
                          capture_output=True, timeout=30)
    assert proc.returncode == 0, "%s harness failed: %r" % (
        tag, proc.stderr[:400])
    line = proc.stdout.split(b"\n", 1)[0].decode("utf-8")
    assert line.startswith("RUN "), "bad harness line: %r" % line
    fields = dict(part.split("=", 1) for part in line.split()[1:])
    return fields, out.read_bytes()


def main() -> int:
    if not BANK.is_dir():
        print("SKIP: %s absent" % BANK)
        return 0
    opcodes = _opcode_values()
    assert opcodes["nGCAnimEvent16End"] == 0, "End must be 0"
    assert opcodes["nGCAnimEvent16SetTranslateInterp"] == 12, "TraI must be 12"
    src = C_PATH.read_text(encoding="utf-8")
    funcs = "".join(_extract_func(src, sig) for sig in WANT_FUNCS)
    assert "((swapped >> 24)" in funcs, "header lane logic moved"
    assert "unresolved++" in funcs, "unresolved count logic moved"
    assert "ndsRelocSYInterpDescHeaderNative(" in funcs, "desc fix call moved"
    assert "ndsRelocValidateAObj16Script(" in funcs, "validator moved"
    WORK.mkdir(parents=True, exist_ok=True)
    tmp = WORK / "host"
    if tmp.exists():
        shutil.rmtree(tmp)
    tmp.mkdir(parents=True)
    try:
        exe = _build_harness(tmp, funcs, opcodes)

        # 1. Header unit on the actual C function, measured.
        proc = subprocess.run([str(exe), "header", "0x02000005"],
                              capture_output=True, text=True, timeout=30,
                              check=True)
        assert proc.stdout.strip() == "HEADER 00050002", \
            "actual C header gave %r" % proc.stdout.strip()
        print("header: actual C 0x02000005 becomes 0x00050002, "
              "kind 2 points_num 5")

        probe = _load("ftanim_reloc_probe")
        for name, file_id in ROLLS:
            prep = _prepare_image(probe, name)
            assert prep["file_id"] == file_id, \
                "%s id 0x%x" % (name, prep["file_id"])
            assert prep["table"] == SRC_TABLE_BYTES, \
                "%s table 0x%x" % (name, prep["table"])
            assert prep["script_start"] == SRC_SCRIPT_START, \
                "%s scripts 0x%x" % (name, prep["script_start"])
            assert len(prep["image"]) > SRC_SCRIPT_START, \
                "%s image too short" % name

            # Locate the real TraI script entry inside the top table.
            fx = probe.fixups(probe.load((BANK / name).read_bytes()))
            entries = sorted({t for off, t in fx.items()
                              if off < SRC_TABLE_BYTES})
            assert entries, "%s has no table entries" % name
            # Find the entry whose script holds the TraI command that
            # names descriptor 0x118. The image here already has the
            # script lane swap applied by normalize_full.
            image = bytearray(prep["image"])
            found_off = None
            found_words = None
            for entry in entries:
                if entry < SRC_SCRIPT_START or entry >= len(image):
                    continue
                cmds = list(probe.decode_script(image, len(image), entry))
                for cmd in cmds:
                    if (cmd["op"] == probe.OP_INTERP
                            and cmd["jump"] == SRC_DESC_OFF):
                        found_off = entry
                        # Word count the C walk sees: to file end is a
                        # bounded over approximation, the walk stops at End.
                        found_words = (len(image) - entry) // 2
                        break
                if found_off is not None:
                    break
            assert found_off is not None, \
                "%s: no TraI naming 0x118 in table entries %r" % (name,
                                                                  entries)
            before = bytes(image)
            fields, after = _run_on_file(exe, tmp, "%s_valid" % name,
                                         bytes(image), found_off,
                                         found_words, SRC_TABLE_BYTES,
                                         SRC_SCRIPT_START)
            assert fields["unresolved"] == "0", \
                "%s actual C unresolved=%s" % (name, fields["unresolved"])
            kind = after[SRC_DESC_OFF]
            pn = struct.unpack_from("<h", after, SRC_DESC_OFF + 2)[0]
            assert (kind, pn) == (SRC_KIND, SRC_POINTS_NUM), \
                "%s actual C header kind %d pn %d" % (name, kind, pn)
            assert struct.unpack_from("<I", after,
                                      SRC_DESC_OFF + 12)[0] == \
                SRC_LENGTH_BITS, "%s length bits moved" % name
            for field, want in zip((8, 16, 20), SRC_PTRS):
                got = struct.unpack_from("<I", after,
                                         SRC_DESC_OFF + field)[0]
                assert got == want, \
                    "%s desc field %d 0x%x" % (name, field, got)
            print("%s: actual C unresolved 0, desc 0x118 kind 2 pn 5, "
                  "script 0x%x" % (name, found_off))

            # 3. Error path on the same real bytes with bad bounds.
            # Descriptor 0x118 is outside [0x130, 0x140), so the actual C
            # function must count one unresolved and leave the entire
            # payload byte identical. Measured requirement for a usable raw
            # cache: zero bytes change, so a declined prebake entry
            # normalizes exactly once later instead of twice.
            fields_bad, after_bad = _run_on_file(
                exe, tmp, "%s_badbound" % name, bytes(image), found_off,
                found_words, SRC_SCRIPT_START, SRC_SCRIPT_START + 0x10)
            assert fields_bad["unresolved"] == "1", \
                "%s bad bound unresolved=%s" % (name,
                                               fields_bad["unresolved"])
            assert after_bad[SRC_TABLE_BYTES:SRC_SCRIPT_START] == \
                before[SRC_TABLE_BYTES:SRC_SCRIPT_START], \
                "%s error path mutated the interpolation block" % name
            diffs = [i for i in range(len(before))
                     if before[i] != after_bad[i]]
            assert not diffs, \
                "%s error path mutated %d bytes, first 0x%x" % (
                    name, len(diffs), diffs[0] if diffs else 0)
            print("%s: actual C bad bound unresolved 1, entire payload "
                  "preserved (%d bytes)" % (name, len(before)))

            # 4. Repeated descriptor in one script, synthetic but bounded.
            # Overwrite one 2-word non-TraI command in the same script with
            # a second TraI naming the same 0x118 descriptor. The header fix
            # is not idempotent, so the actual C function must refuse
            # (unresolved 1) and leave the entire payload byte identical.
            cmds_before = list(probe.decode_script(bytearray(before),
                                                   len(before), found_off))
            victim = None
            for cmd in cmds_before:
                if cmd["op"] in (probe.OP_END, probe.OP_INTERP,
                                 probe.OP_LOOP):
                    continue
                raw = struct.unpack_from("<H", before, cmd["pc"])[0]
                toggle = raw & 1
                if probe.command_words(cmd["op"], cmd["flags"],
                                       toggle) == 2:
                    victim = cmd["pc"]
                    break
            assert victim is not None, \
                "%s: no 2-word victim command for duplicate test" % name
            dup_image = bytearray(before)
            rel = SRC_DESC_OFF - (victim + 2)
            assert -32768 <= rel <= 32767, \
                "%s duplicate rel %d out of s16 range" % (name, rel)
            struct.pack_into("<H", dup_image, victim, 0x6000)
            struct.pack_into("<h", dup_image, victim + 2, rel)
            dup_before = bytes(dup_image)
            fields_dup, after_dup = _run_on_file(
                exe, tmp, "%s_dupdesc" % name, dup_before, found_off,
                found_words, SRC_TABLE_BYTES, SRC_SCRIPT_START)
            assert fields_dup["unresolved"] == "1", \
                "%s duplicate unresolved=%s" % (name,
                                               fields_dup["unresolved"])
            dup_diffs = [i for i in range(len(dup_before))
                         if dup_before[i] != after_dup[i]]
            assert not dup_diffs, \
                "%s duplicate path mutated %d bytes, first 0x%x" % (
                    name, len(dup_diffs),
                    dup_diffs[0] if dup_diffs else 0)
            print("%s: actual C duplicate TraI unresolved 1, entire "
                  "payload preserved" % name)

        print("SAMUS_ROLL_RUNTIME_NORMALIZER=PASS")
        return 0
    finally:
        pass


if __name__ == "__main__":
    sys.exit(main())
