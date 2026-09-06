#!/usr/bin/env python3
"""Source-correctness proof for the Samus roll spline route (2026-09-06).

The four-CPU ROM crashed at frame 447 in syInterpGetFracFrame on Samus RollB:
the BPS1 stream pack carried the roll figatrees' interpolation block
lane-swapped with unpatched relocation words, because the pack generator's
probe derived the figatree table bound as min(reloc targets) -- a bound that
lands INSIDE the spline block -- and treated every relocation target,
including the SYInterpDesc's own pointer fields, as a command script.

This test proves the three halves of the fix against the SOURCE, not against
the fix:

  1. LOADER SIMULATION. `ftanim_reloc_probe` (the fixed pipeline) runs
     RollF/RollB through the ROM's own stages: u32 word swap, threaded
     internal pointer fixups, contiguous-top-table bound, script-region lane
     unswap, and the SYInterpDesc mixed-width header word the RAW loader now
     rearranges. Asserted: table 0x60, scripts from 0x130, desc at 0x118,
     knot floats byte-identical to the big-endian source values, desc header
     reading kind=2 (Bezier) / points_num=5 natively -- and the blanket-swap
     form demonstrably reading kind=5 / points_num=512, which is the bug.
  2. ORIGINAL CODE EXECUTION. The ORIGINAL decomp syInterpCubic (the same
     translation unit the DS links via src/import/battleship_sys_interp.c)
     is compiled on the host and executed against the loader-simulated image
     at many t values, bit-compared against the same original code running
     on an image built straight from the raw big-endian source fields. The
     two must agree bit for bit; outputs must be finite and inside the
     control-point hull.
  3. TARGET ARM LAYOUT. A _Static_assert probe is compiled with the real
     devkitARM toolchain: SYInterpDesc must be 24 bytes with points at 8,
     length at 12, keyframes at 16, quartics at 20 -- the field offsets the
     header rearrangement and the kernel's manual reads are written against.
  4. PACK EXCLUSION. The battlepack generator routes every
     SetTranslateInterp-bearing clip out of BPS1/BPA2 through a source
     command walk; the six-fighter corpus must yield exactly FTSamusAnim060
     (1013) and FTSamusAnim061 (1014), their dense directory rows must stay
     (0, 0) so the runtime stream lookup misses onto the original O2R
     loader, and no packed run may contain an op-12 command.
  5. RETENTION. The Makefile must keep the two source files staged in
     NitroFS (the retention list beside the AObj32 exceptions), or the O2R
     loader the clips are routed to has nothing to load.

Run: python scripts/test_samus_roll_spline.py
"""

from __future__ import annotations

import importlib.util
import pathlib
import struct
import subprocess
import sys
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[1]
BANK = (ROOT / "decomp" / "BattleShip-main" / "BattleShip_o2r"
        / "reloc_animations")

ROLLS = [("FTSamusAnim060", 1013), ("FTSamusAnim061", 1014)]

# Ground truth from decomp BattleShip decomp/src/relocData/
# 1014_FTSamusAnimRollB.c: word 0 on disk is 0x0200,0x0005 (kind 2, pad,
# points_num 5 BE), ptrs1[1] = 0x421A3F58 (length), and the three intern
# pointers name words 0x18/0x2D/0x32 -> bytes 0x60/0xB4/0xC8.
SRC_TABLE_BYTES = 0x60
SRC_SCRIPT_START = 0x130
SRC_DESC_OFF = 0x118
SRC_KIND = 2                          # nSYInterpKindBezier
SRC_POINTS_NUM = 5
SRC_LENGTH_BITS = 0x421A3F58
SRC_PTRS = (0x60, 0xB4, 0xC8)
SRC_KEYFRAMES = [0.0, 0.08779200166463852, 0.2068299949169159,
                 0.4862290024757385, 1.0]

T_GRID = ([0.0, 0.08779200166463852, 0.1, 0.2068299949169159, 0.3,
           0.4862290024757385, 0.5, 0.6234, 0.75, 0.9, 1.0] +
          [round(i / 64.0, 6) for i in range(65)])


def _load(name):
    spec = importlib.util.spec_from_file_location(
        name, ROOT / "scripts" / ("%s.py" % name))
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


def _desc_header_native(swapped):
    """Python mirror of ndsRelocSYInterpDescHeaderNative (reloc_backend_assets.c)."""
    return ((swapped >> 24) |
            (((swapped >> 16) & 0xFF) << 8) |
            ((swapped & 0xFF) << 16) |
            (((swapped >> 8) & 0xFF) << 24))


def loader_simulate(probe, name):
    """The RAW O2R loader's end state for one roll figatree, host-side.

    pipeline 2: probe.load (u32 swap). pipeline 3: the threaded chain's slots
    written as base+target pointers (base 0, so a slot holds its target
    offset -- exactly the relative form the kernel rebases). pipeline 4a/4b:
    the fixed contiguous-table normalize. Then the SYInterpDesc header word
    fix the runtime's script walk applies, located the same way: by the
    SetTranslateInterp command inside the top table's scripts.
    """
    raw = (BANK / name).read_bytes()
    f = probe.load(raw)
    assert f is not None, "%s: not an O2R RELO file" % name
    fx = probe.fixups(f)
    image = f["data"]
    for slot, target in fx.items():
        struct.pack_into("<I", image, slot, target)
    table, script_start = probe.normalize_full(f, fx)
    entries = probe.table_targets(fx, table)

    descs = []
    for entry in entries:
        for cmd in probe.decode_script(image, f["size"], entry):
            if cmd["op"] == probe.OP_INTERP:
                # decoded in the full image: the jump is already absolute
                descs.append(cmd["jump"])
    for desc in descs:
        word = struct.unpack_from("<I", image, desc)[0]
        struct.pack_into("<I", image, desc, _desc_header_native(word))
    return {"image": bytes(image), "size": f["size"], "table": table,
            "script_start": script_start, "descs": descs,
            "file_id": f["file_id"]}


def source_image(name):
    """The independent oracle image, built straight from raw BE disk values.

    No probe pipeline touches this: the desc fields and knot arrays are
    byte-swapped by hand out of the raw file at the source-derived offsets,
    then written little-endian at the same offsets in a fresh buffer. If the
    loader simulation is correct, the ORIGINAL syInterpCubic cannot tell the
    two images apart.
    """
    raw = (BANK / name).read_bytes()
    d = raw[0x50:]
    image = bytearray(d)
    be_word = lambda off: struct.unpack_from(">I", d, off)[0]
    # knots: plain u32 swap of every word (each is a full-width f32)
    for off in range(0, len(image) - 3, 4):
        struct.pack_into("<I", image, off, be_word(off))
    # desc: header word in ARM-native {kind, pad, pn_lo, pn_hi} plus pointer
    # fields as offsets -- the state AFTER correct fixups + header fix
    kind = d[0x118]
    pn = struct.unpack_from(">h", d, 0x11A)[0]
    word0 = kind | ((pn & 0xFF) << 16) | ((pn >> 8) & 0xFF) << 24
    struct.pack_into("<I", image, 0x118, word0)
    struct.pack_into("<I", image, 0x11C, be_word(0x11C))          # unk04
    struct.pack_into("<I", image, 0x120, SRC_PTRS[0])             # points
    struct.pack_into("<I", image, 0x124, be_word(0x124))          # length
    struct.pack_into("<I", image, 0x128, SRC_PTRS[1])             # keyframes
    struct.pack_into("<I", image, 0x12C, SRC_PTRS[2])             # quartics
    return image


def run_kernel(exe, image_path, desc_off):
    out = subprocess.run(
        [str(exe), str(image_path), "0x%x" % desc_off] +
        [repr(t) for t in T_GRID],
        capture_output=True, text=True, check=True).stdout.splitlines()
    header = out[0].split()
    assert header[0] == "H" and header[-1] == "1", \
        "kernel pointer check failed: %r" % out[0]
    rows = [line.split() for line in out[1:]]
    assert len(rows) == len(T_GRID), "kernel t count %d" % len(rows)
    return header, [tuple(r[1:4]) for r in rows]


def main() -> int:
    if not BANK.is_dir():
        print("SKIP: %s absent" % BANK)
        return 0
    probe = _load("ftanim_reloc_probe")
    tmp = pathlib.Path(tempfile.mkdtemp(prefix="samus_roll_spline_"))
    try:
        import shutil
        cc = (shutil.which("gcc") or shutil.which("cc") or
              shutil.which("clang"))
        assert cc, "no host C compiler found"
        exe = tmp / "kernel.exe"
        build = subprocess.run(
            [cc, "-O1", "-w", "-std=gnu99", "-I", str(ROOT / "include"),
             "-I", str(ROOT / "decomp" / "BattleShip-main" / "decomp" / "src"),
             str(ROOT / "scripts" / "test_samus_roll_spline_kernel.c"),
             "-o", str(exe), "-lm"],
            capture_output=True, text=True)
        assert build.returncode == 0, "kernel build failed:\n%s" % build.stderr

        for name, file_id in ROLLS:
            sim = loader_simulate(probe, name)
            assert sim["file_id"] == file_id, \
                "%s file id 0x%x != 0x%x" % (name, sim["file_id"], file_id)

            # -- the derived bounds match the source layout
            assert sim["table"] == SRC_TABLE_BYTES, \
                "%s table 0x%x != 0x%x" % (name, sim["table"], SRC_TABLE_BYTES)
            assert sim["script_start"] == SRC_SCRIPT_START, \
                "%s script start 0x%x != 0x%x" % (name, sim["script_start"],
                                                  SRC_SCRIPT_START)
            assert sim["descs"] == [SRC_DESC_OFF], \
                "%s TraI descs %r" % (name, sim["descs"])

            # -- knot data untouched by every transform after the word swap
            src = source_image(name)
            assert sim["image"][SRC_TABLE_BYTES:SRC_DESC_OFF] == \
                bytes(src[SRC_TABLE_BYTES:SRC_DESC_OFF]), \
                "%s knot bytes diverge from source" % name
            kf = [struct.unpack_from("<f", sim["image"], SRC_PTRS[1] + 4 * i)[0]
                  for i in range(SRC_POINTS_NUM)]
            assert kf == SRC_KEYFRAMES, "%s keyframes %r" % (name, kf)

            # -- desc header: native-valid after the fix, wrong before it.
            # The blanket u32 swap leaves word 0 as the reversed disk bytes
            # [05,00,00,02]: the ORIGINAL struct then reads kind = 5 (no
            # such nSYInterpKind) and points_num = 0x0200 -- the crash
            # family. The runtime fix rearranges it to [02,00,05,00].
            kind = sim["image"][SRC_DESC_OFF]
            pn = struct.unpack_from("<h", sim["image"], SRC_DESC_OFF + 2)[0]
            raw = (BANK / name).read_bytes()
            disk_word = raw[0x50 + SRC_DESC_OFF:0x50 + SRC_DESC_OFF + 4]
            blanket = disk_word[::-1]
            assert (blanket[0],
                    struct.unpack_from("<h", blanket, 2)[0]) == (5, 0x0200), \
                "%s blanket-swap form moved; bug demonstration stale" % name
            assert (kind, pn) == (SRC_KIND, SRC_POINTS_NUM), \
                "%s fixed header kind %d pn %d" % (name, kind, pn)

            # -- pointers: patched, in-file, aligned (the resolver's demand)
            for field, want in zip((8, 16, 20), SRC_PTRS):
                got = struct.unpack_from("<I", sim["image"],
                                         SRC_DESC_OFF + field)[0]
                assert got == want and (got & 3) == 0 and got < sim["size"], \
                    "%s desc ptr field %d = 0x%x" % (name, field, got)
            length_bits = struct.unpack_from("<I", sim["image"],
                                             SRC_DESC_OFF + 12)[0]
            assert length_bits == SRC_LENGTH_BITS, \
                "%s length bits %08x" % (name, length_bits)

            # -- ORIGINAL syInterpCubic: loader-sim image vs source image
            sim_path = tmp / ("%s_sim.bin" % name)
            src_path = tmp / ("%s_src.bin" % name)
            sim_path.write_bytes(sim["image"][:sim["script_start"]])
            src_path.write_bytes(bytes(src[:SRC_SCRIPT_START]))
            sim_header, sim_rows = run_kernel(exe, sim_path, SRC_DESC_OFF)
            src_header, src_rows = run_kernel(exe, src_path, SRC_DESC_OFF)
            assert sim_header[1:] == src_header[1:], \
                "%s desc headers diverge:\n%s\n%s" % (name, sim_header,
                                                      src_header)
            assert sim_rows == src_rows, \
                "%s original syInterpCubic outputs diverge at %d of %d t" % (
                    name,
                    next((i for i, (a, b) in enumerate(zip(sim_rows, src_rows))
                          if a != b), -1), len(T_GRID))
            # The original kind-2 weights are NOT a partition of unity
            # (syInterpBezier3Points sums to 1.5 at t=1), and at t=1 the
            # code indexes &points[points_num-1] then reads four control
            # points -- two words past the authored knots, exactly as the
            # N64 did (the source extractor kept them in the block). The
            # bound therefore covers every f32 in the points region; what it
            # must still catch is the lane-swapped-garbage class (1e9-scale).
            region = sim["image"][SRC_PTRS[0]:SRC_PTRS[1]]
            bound = 2.0 * max(
                abs(struct.unpack_from("<f", region, i)[0])
                for i in range(0, len(region) - 3, 4))
            for row in sim_rows:
                for bits in row:
                    value = struct.unpack("<f", struct.pack("<I", int(bits, 16)))[0]
                    assert value == value and abs(value) <= bound, \
                        "%s output not finite/in bound: %r" % (name, bits)
            print("%s: bounds 0x%x/0x%x, desc 0x%x, kind %d pn %d, "
                  "%d t values bit-identical to source" %
                  (name, sim["table"], sim["script_start"], SRC_DESC_OFF,
                   kind, pn, len(T_GRID)))

        # -- the pack routes the rolls out; nothing else moves
        gen = _load("generate_battlepack_anim")
        blob, directory, pool, table, stats = gen.build(BANK)
        spline_ids = sorted(row["id"] for row in stats["splines"])
        assert spline_ids == [1013, 1014], \
            "six-fighter spline census %r != [1013, 1014]" % spline_ids
        stream, sstats = gen.emit_stream_pack(directory)
        bad, checked = gen.verify_stream_pack(BANK, directory, stream)
        assert not bad, "stream pack mismatches: %r" % bad[:4]
        fields = gen.STREAM_HEADER.unpack_from(stream, 0)
        first_id, dir_off = fields[4], fields[6]
        for file_id in (1013, 1014):
            off = gen.STREAM_DIR.unpack_from(
                stream, dir_off + (file_id - first_id) * gen.STREAM_DIR.size)
            assert off == (0, 0), \
                "id %d dense row %r must stay (0, 0) so the runtime misses " \
                "onto the O2R loader" % (file_id, off)
        for clip in directory:
            for run in clip["runs"]:
                for cmd in probe.decode_script(run, len(run), 0, native=True):
                    assert cmd["op"] != probe.OP_INTERP, \
                        "%s packed with SetTranslateInterp" % clip["name"]
        print("battlepack: %d clips packed, splines %r routed to the O2R "
              "loader, dense rows (0,0), no packed TraI command" %
              (stats["clips"], spline_ids))

        # -- the O2R sources the rolls are routed to must stay staged
        makefile = (ROOT / "Makefile").read_text()
        block = makefile.split("NDS_FTANIM_STREAM_REPLACED_RELOC_FILES :=",
                               1)[-1].split("\nendif", 1)[0]
        for name, _file_id in ROLLS:
            entry = "reloc_animations/%s" % name
            assert entry in block, (
                "Makefile retention list is missing %s. Required patch "
                "(Main-owned file): add\n"
                "  NDS_FTANIM_STREAM_SPLINE_FILES := \\\n"
                "  \treloc_animations/FTSamusAnim060 \\\n"
                "  \treloc_animations/FTSamusAnim061\n"
                "and restore it alongside NDS_FTANIM_STREAM_AOBJ32_FILES in "
                "both filter-outs." % entry)
        print("Makefile: roll O2R sources retained for the stream misses")

        # -- target ARM layout, on the real target toolchain
        arm = (pathlib.Path(__import__("os").environ.get(
            "DEVKITARM", "C:/devkitPro/devkitARM")) / "bin" /
            "arm-none-eabi-gcc.exe")
        if arm.exists():
            probe_c = tmp / "arm_layout_probe.c"
            probe_c.write_text(
                "#include <stddef.h>\n"
                "#include <stdint.h>\n"
                "#include <sys/interp.h>\n"
                "_Static_assert(offsetof(SYInterpDesc, kind) == 0u, \"kind\");\n"
                "_Static_assert(offsetof(SYInterpDesc, points_num) == 2u, "
                "\"points_num\");\n"
                "_Static_assert(offsetof(SYInterpDesc, unk04) == 4u, "
                "\"unk04\");\n"
                "_Static_assert(offsetof(SYInterpDesc, points) == 8u, "
                "\"points\");\n"
                "_Static_assert(offsetof(SYInterpDesc, length) == 12u, "
                "\"length\");\n"
                "_Static_assert(offsetof(SYInterpDesc, keyframes) == 16u, "
                "\"keyframes\");\n"
                "_Static_assert(offsetof(SYInterpDesc, quartics) == 20u, "
                "\"quartics\");\n"
                "_Static_assert(sizeof(SYInterpDesc) == 24u, \"size\");\n"
                "int arm_layout_probe;\n")
            arm_build = subprocess.run(
                [str(arm), "-fsyntax-only", "-mthumb",
                 "-I", str(ROOT / "include"),
                 "-I", str(ROOT / "decomp" / "BattleShip-main" /
                           "decomp" / "src"),
                 str(probe_c)],
                capture_output=True, text=True)
            assert arm_build.returncode == 0, \
                "target ARM SYInterpDesc layout moved:\n%s" % arm_build.stderr
            print("target ARM layout: SYInterpDesc 24 B, points@8 length@12 "
                  "keyframes@16 quartics@20 -- static asserts compiled")
        else:
            print("NOTE: devkitARM not found; target ARM layout probe skipped")

        print("SAMUS_ROLL_SPLINE=PASS")
        return 0
    finally:
        import shutil
        shutil.rmtree(tmp, ignore_errors=True)


if __name__ == "__main__":
    sys.exit(main())
