"""Host tests for the compact ground maps (P2-2p8 T1).

No ROM, no emulator, no make. Covers the whole chain the change rests on:

  * generator: all nine VS maps build from the real O2R export and verify
    (fixup counts -1/+2, extern table, every source word kept, stub == the
    container's Bitmap[44]+Sprite with pixels gone), and the output re-parses
    with the stage generator's own parser (generate_nds_native_stage.load_o2r);
  * extern tree: each compact tree is exactly 158,144 B smaller and no longer
    contains the wallpaper container (Dream Land: the 2026-09-23 census figure
    202,816 B becomes 44,672 B);
  * pins: source map sizes equal the packet descriptors' pinned sizes (so the
    P1 golden pin and the emitted rows are untouched) and the P2 compact size is
    exactly pin + 784 for all nine, Dream Land included;
  * runtime seams: the REAL C of src/port/reloc_backend_assets.c (sliced out,
    not re-implemented) compiled for a 32-bit host so struct layouts match the
    ARM9, driven with the generator's real output through the loader's steps
    (word swap, internal/external fixups): stub Sprite/Bitmap normalization
    matches the container's field for field, source maps are left alone, the
    wallpaper key translation is exact, the Training Mode region is right;
  * build wiring: Makefile default/targets/config echo/stamp/rules, and the
    C constants against the generator's.

The C part needs a 32-bit host clang/gcc (llvm-mingw's i686-w64-mingw32-clang);
it is skipped, loudly, where none exists.

Run:  python -m pytest scripts/stages/test_compact_ground_maps.py -q
  or: python scripts/stages/test_compact_ground_maps.py
"""

from __future__ import annotations

import hashlib
import re
import shutil
import struct
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

_p = Path(__file__).resolve().parent
while _p.name != "scripts":
    _p = _p.parent
sys.path.insert(0, str(_p))
import _paths  # noqa: F401,E402

import generate_compact_ground_maps as cgm  # noqa: E402
import generate_native_wallpapers as wall  # noqa: E402
import generate_nds_native_stage as stage_gen  # noqa: E402

ROOT = Path(__file__).resolve().parents[2]
O2R = cgm.DEFAULT_O2R_ROOT
ASSETS_C = ROOT / "src" / "port" / "reloc_backend_assets.c"
MATRIX_C = ROOT / "src" / "port" / "renderer_adapter_matrix.c"
SPRITE_C = ROOT / "src" / "port" / "sprite_preview_backend.c"
SHIMS_C = ROOT / "src" / "port" / "reloc_backend_compat_shims.c"
MAKEFILE = ROOT / "Makefile"

# gkind order -> descriptor name (scripts/stages/native_stage_descriptors)
DESCRIPTORS = ("castle", "sector", "jungle", "zebes", "hyrule", "yoster",
               "dreamland", "yamabuki", "inishie")

P1_TARGET = "smash64ds-battle-playable-hwtri"
P2_TARGETS = ("smash64ds-p2-fourcpu-tickhud-hwtri", "smash64ds-p2-shell-hwtri",
              "smash64ds-p2-shell-freeplay-hwtri",
              "smash64ds-p2-shell-loop-hwtri", "smash64ds")


def read_pair(spec):
    return cgm.read_pair(O2R, spec)


def o2r_index() -> dict[int, Path]:
    """file id -> path for every reloc file (header read only)."""
    out: dict[int, Path] = {}
    for sub in sorted(O2R.iterdir()):
        if not (sub.name.startswith("reloc_") and sub.is_dir()):
            continue
        for path in sorted(sub.iterdir()):
            with path.open("rb") as handle:
                head = handle.read(0x48)
            if len(head) >= 0x48 and head[4:8] == b"OLER":
                out[struct.unpack_from("<I", head, 0x40)[0]] = path
    return out


def extern_tree_alloc(index: dict[int, Path], root_id: int,
                      override: dict[int, bytes] | None = None) -> int:
    """lbRelocGetFileSize's arithmetic: 16-aligned payload, then each extern id
    in table order, each file once (ndsRelocExternTreeAllocSize)."""
    override = override or {}
    seen: set[int] = set()

    def payload_and_ids(file_id):
        raw = override.get(file_id)
        if raw is None:
            raw = index[file_id].read_bytes()
        f = cgm.parse_o2r(raw, f"file {file_id}")
        return len(f.payload), f.extern_ids

    def alloc(file_id, running):
        if file_id in seen:
            return running
        seen.add(file_id)
        size, ids = payload_and_ids(file_id)
        running = cgm.align16(running) + size
        for dep in ids:
            running = alloc(dep, running)
        return running

    return alloc(root_id, 0)


class GeneratorTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        assert O2R.is_dir(), f"O2R export absent: {O2R}"
        cls.built = {}
        for spec in cgm.MAPS:
            map_raw, container_raw = read_pair(spec)
            cls.built[spec.map_path] = (
                cgm.build_compact_map(spec, map_raw, container_raw),
                map_raw, container_raw)
        cls.index = o2r_index()
        cls.compact_sizes = {path: built[0].compact_size
                             for path, built in cls.built.items()}

    def test_nine_maps_gkind_order_and_unique(self):
        self.assertEqual([s.gkind for s in cgm.MAPS], list(range(9)))
        self.assertEqual(len({s.map_file_id for s in cgm.MAPS}), 9)
        self.assertEqual(len({s.container_file_id for s in cgm.MAPS}), 9)
        self.assertEqual(cgm.STUB_BYTES, 776)
        self.assertEqual(cgm.STUB_GROWTH, 784)
        self.assertEqual(cgm.WALLPAPER_SLOT, 0x5C)

    def test_specs_match_the_wallpaper_generator(self):
        by_path = {s.o2r: s for s in wall.SOURCES if s.battle}
        self.assertEqual(len(by_path), 9)
        for spec in cgm.MAPS:
            with self.subTest(stage=spec.label):
                src = by_path[spec.container_path]
                self.assertEqual(src.file_id, spec.container_file_id)
                self.assertEqual(src.sprite_offset, cgm.CONTAINER_SPRITE_OFFSET)
                runtime = (src.file_id if src.runtime_asset_id is None
                           else src.runtime_asset_id)
                self.assertEqual(runtime, spec.container_asset_id)

    def test_alias_target_is_the_wallpaper_table_key(self):
        """The runtime translates a compact map's stub to (container asset id,
        0x269C8); that pair must be exactly the key the real wallpaper
        conversion emits for that stage."""
        by_path = {s.o2r: s for s in wall.SOURCES if s.battle}
        for spec in cgm.MAPS:
            with self.subTest(stage=spec.label):
                asset = wall.convert_source(ROOT, by_path[spec.container_path])
                runtime = (asset.source.file_id
                           if asset.source.runtime_asset_id is None
                           else asset.source.runtime_asset_id)
                self.assertEqual((runtime, asset.bitmap_offset),
                                 (spec.container_asset_id,
                                  cgm.CONTAINER_BITMAP_OFFSET))

    def test_all_nine_build_verify_and_lose_the_container(self):
        for spec in cgm.MAPS:
            with self.subTest(stage=spec.label):
                compact, map_raw, container_raw = self.built[spec.map_path]
                cgm.verify_compact_map(spec, map_raw, container_raw, compact)
                self.assertEqual(compact.compact_size - compact.source_size,
                                 784)
                self.assertEqual(compact.tree_delta, 158_144)
                out = cgm.parse_o2r(compact.data, spec.map_path)
                self.assertNotIn(spec.container_file_id, out.extern_ids)

    def test_output_reparses_with_the_stage_generators_parser(self):
        with tempfile.TemporaryDirectory(prefix="smash64ds-compact-") as tmp:
            tmp = Path(tmp)
            for spec in cgm.MAPS:
                with self.subTest(stage=spec.label):
                    compact, map_raw, _ = self.built[spec.map_path]
                    rel = "out/" + Path(spec.map_path).name
                    (tmp / "out").mkdir(exist_ok=True)
                    (tmp / rel).write_bytes(compact.data)
                    res = stage_gen.load_o2r(tmp, stage_gen.InputSpec(
                        rel, hashlib.sha256(compact.data).hexdigest()))
                    src_path = O2R / spec.map_path
                    src = stage_gen.load_o2r(ROOT, stage_gen.InputSpec(
                        str(src_path.relative_to(ROOT)).replace("\\", "/"),
                        hashlib.sha256(map_raw).hexdigest()))
                    self.assertEqual(len(res.internal), len(src.internal) + 2)
                    self.assertEqual(len(res.external), len(src.external) - 1)
                    self.assertNotIn(
                        spec.container_file_id,
                        {r.asset_id for r in res.external.values()})
                    self.assertEqual(
                        res.internal[cgm.WALLPAPER_SLOT],
                        stage_gen.PointerRef(spec.map_file_id,
                                             compact.stub_sprite_offset))
                    self.assertEqual(
                        res.internal[compact.stub_sprite_offset +
                                     cgm.SPRITE_BITMAP_OFFSET],
                        stage_gen.PointerRef(spec.map_file_id,
                                             compact.stub_bitmap_offset))
                    # every other external fixup still points where it did
                    self.assertEqual(
                        {k: v for k, v in src.external.items()
                         if k != cgm.WALLPAPER_SLOT}, dict(res.external))

    def test_extern_tree_shrinks_by_exactly_the_container(self):
        for spec in cgm.MAPS:
            with self.subTest(stage=spec.label):
                compact, _, _ = self.built[spec.map_path]
                before = extern_tree_alloc(self.index, spec.map_file_id)
                after = extern_tree_alloc(
                    self.index, spec.map_file_id,
                    {spec.map_file_id: compact.data})
                self.assertEqual(before - after, 158_144)
                # ... and the container really is what left the tree
                self.assertIn(spec.container_file_id,
                              self._tree_ids(spec.map_file_id, None))
                self.assertNotIn(
                    spec.container_file_id,
                    self._tree_ids(spec.map_file_id,
                                   {spec.map_file_id: compact.data}))

    def _tree_ids(self, root_id, override):
        seen: list[int] = []

        def walk(fid):
            if fid in seen:
                return
            seen.append(fid)
            raw = (override or {}).get(fid) or self.index[fid].read_bytes()
            for dep in cgm.parse_o2r(raw, str(fid)).extern_ids:
                walk(dep)

        walk(root_id)
        return seen

    def test_dream_land_matches_the_census_figure(self):
        spec = [s for s in cgm.MAPS if s.label == "Dream Land"][0]
        compact, _, _ = self.built[spec.map_path]
        self.assertEqual(extern_tree_alloc(self.index, spec.map_file_id),
                         202_816)
        self.assertEqual(
            extern_tree_alloc(self.index, spec.map_file_id,
                              {spec.map_file_id: compact.data}), 44_672)

    def test_source_sizes_are_the_pinned_packet_sizes(self):
        """P1's golden pin and the emitted rows stay the SOURCE sizes; the P2
        compact size is exactly pin + 784 for all nine (Dream Land included)."""
        for spec, name in zip(cgm.MAPS, DESCRIPTORS):
            with self.subTest(stage=spec.label):
                desc = stage_gen._resolve_stage(name)
                ids = list(desc.adapter_asset_ids)
                pinned = list(desc.adapter_asset_sizes)[
                    ids.index(spec.map_file_id)]
                compact = self.built[spec.map_path][0]
                self.assertEqual(compact.source_size, pinned)
                self.assertEqual(compact.compact_size, pinned + 784)
        # the frozen P1 pin itself, and the P2-only Dream Land row
        sys.path.insert(0, str(Path(__file__).resolve().parent))
        import check_nds_native_stage as check  # noqa: E402
        self.assertEqual(check.DREAMLAND_ADAPTER_ASSET_SIZES,
                         (0x2FC0, 0x43F0, 0x3700, 0x00C0))
        self.assertEqual(check.DREAMLAND_ADAPTER_ASSET_SIZES[3] + 784, 0x3D0)

    def test_matrix_rows_are_untouched_source_sizes(self):
        """The emitted adapter rows (compiled into P1 too) are the descriptors'
        pinned SOURCE sizes; the compact size is applied at run time only."""
        text = MATRIX_C.read_text(encoding="utf-8")
        names = {"castle": "Castle", "sector": "Sector", "jungle": "Jungle",
                 "zebes": "Zebes", "hyrule": "Hyrule", "yoster": "Yoster",
                 "dreamland": "DreamLand", "yamabuki": "Yamabuki",
                 "inishie": "Inishie"}

        def number(token):
            return int(token.strip().rstrip("uU"), 0)

        for spec, name in zip(cgm.MAPS, DESCRIPTORS):
            with self.subTest(stage=spec.label):
                m = re.search(
                    r"sNdsRendererAdapterNativeStage%s = \{\s*"
                    r"[^{}]*?\{([^{}]*)\},\s*\{([^{}]*)\}" % names[name],
                    text)
                self.assertIsNotNone(m, "descriptor row")
                ids = [number(t) for t in m.group(1).split(",")]
                sizes = [number(t) for t in m.group(2).split(",")]
                desc = stage_gen._resolve_stage(name)
                count = len(desc.adapter_asset_ids)
                self.assertEqual(ids[:count], list(desc.adapter_asset_ids))
                self.assertEqual(sizes[:count], list(desc.adapter_asset_sizes))
                self.assertFalse(any(sizes[count:]))
                compact = self.compact_sizes[spec.map_path]
                self.assertNotIn(compact, sizes)


class MakefileAndSourceContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.make = MAKEFILE.read_text(encoding="utf-8")
        cls.assets = ASSETS_C.read_text(encoding="utf-8")

    def test_default_flag_targets(self):
        m = re.search(r"(?m)^NDS_P2_COMPACT_GROUND_MAPS \?= \\\r?\n\t"
                      r"\$\(if \$\(filter ([^,]*),\$\(TARGET\)\),1,0\)",
                      self.make)
        self.assertIsNotNone(m, "flag default line")
        self.assertEqual(sorted(m.group(1).split()), sorted(P2_TARGETS))
        self.assertNotIn(P1_TARGET, m.group(1).split())
        self.assertNotRegex(self.make,
                            r"(?m)^\s*override\s+NDS_P2_COMPACT_GROUND_MAPS",
                            "the flag must stay overridable for A/B")

    def test_config_header_echo(self):
        self.assertIn("echo '#define NDS_P2_COMPACT_GROUND_MAPS "
                      "$(NDS_P2_COMPACT_GROUND_MAPS)'; \\", self.make)

    def test_staging_rules(self):
        files = re.search(r"(?m)^NDS_COMPACT_GROUND_MAP_FILES := \\\r?\n"
                          r"((?:\t.*\r?\n)+?)(?=\S)", self.make)
        self.assertIsNotNone(files)
        self.assertEqual(sorted(files.group(1).replace("\\", " ").split()),
                         sorted(s.map_path for s in cgm.MAPS))
        conts = re.search(r"(?m)^NDS_COMPACT_GROUND_MAP_CONTAINERS := \\\r?\n"
                          r"((?:\t.*\r?\n)+?)(?=\S)", self.make)
        self.assertIsNotNone(conts)
        listed = [w.replace("$(BATTLESHIP_O2R)/", "") for w in
                  conts.group(1).replace("\\", " ").split()]
        self.assertEqual(sorted(listed),
                         sorted(s.container_path for s in cgm.MAPS))
        # the stamp is per BUILD and never inside the packed nitrofs/ directory
        self.assertIn("NDS_COMPACT_GROUND_MAPS_STAMP := $(PROJECT_ROOT)/"
                      "$(BUILD)/nds_compact_ground_maps.stamp", self.make)
        self.assertRegex(self.make,
                         r"\$\(NDS_COMPACT_GROUND_MAPS_STAMP\): FORCE")
        self.assertRegex(
            self.make,
            r"\$\(NDS_COMPACT_GROUND_MAP_TARGETS\): \$\(NITROFS_DIR\)/reloc/%: "
            r"\$\(BATTLESHIP_O2R\)/%")
        self.assertIn('--o2r-root "$(BATTLESHIP_O2R)" --map "$*" --output "$@"',
                      self.make)
        # the generic rule every other reloc file uses is still there, unchanged
        self.assertRegex(
            self.make,
            r"(?m)^\$\(NITROFS_DIR\)/reloc/%: \$\(BATTLESHIP_O2R\)/%\r?\n"
            r"\t@mkdir -p \$\(dir \$@\)\r?\n\t@cp \$< \$@\r?\n")

    def test_c_constants_match_the_generator(self):
        def macro(name):
            m = re.search(r"(?m)^#define %s\s+(0x[0-9a-fA-F]+|\d+)u?\b" % name,
                          self.assets)
            self.assertIsNotNone(m, name)
            return int(m.group(1), 0)

        self.assertEqual(macro("NDS_RELOC_COMPACT_GROUND_MAP_GROWTH"),
                         cgm.STUB_GROWTH)
        self.assertEqual(macro("NDS_RELOC_COMPACT_GROUND_MAP_HEADER"),
                         cgm.GROUND_HEADER_OFFSET)
        self.assertEqual(macro("NDS_RELOC_COMPACT_WALLPAPER_BITMAP"),
                         cgm.CONTAINER_BITMAP_OFFSET)
        self.assertEqual(macro("NDS_RELOC_COMPACT_TRAINING_WALLPAPER_BYTES"),
                         cgm.CONTAINER_SIZE)
        self.assertEqual(macro("NDS_RELOC_SYMBOL_STAGE_DREAM_LAND_SPRITE"),
                         cgm.CONTAINER_SPRITE_OFFSET)
        table = re.search(
            r"sNdsRelocCompactGroundMaps\[\] = \{\n(.*?)\n\};", self.assets,
            re.S)
        self.assertIsNotNone(table)
        rows = re.findall(r"\{ (NDS_RELOC_ASSET_\w+), (NDS_RELOC_ASSET_\w+) \}",
                          table.group(1))
        self.assertEqual(len(rows), 9)
        for spec, (map_name, wall_name) in zip(cgm.MAPS, rows):
            with self.subTest(stage=spec.label):
                self.assertEqual(macro(map_name), spec.map_file_id)
                self.assertEqual(macro(wall_name), spec.container_asset_id)

    def test_every_use_is_behind_the_flag(self):
        """The P1 build compiles none of this: each seam sits in a
        `#if defined(NDS_P2_COMPACT_GROUND_MAPS) && ...` region."""
        flag = "#if defined(NDS_P2_COMPACT_GROUND_MAPS) && NDS_P2_COMPACT_GROUND_MAPS"
        for path in (ASSETS_C, MATRIX_C, SPRITE_C, SHIMS_C):
            stack = []
            for number, line in enumerate(
                    path.read_text(encoding="utf-8").splitlines(), 1):
                stripped = line.strip()
                if stripped.startswith("#if"):
                    stack.append(stripped.startswith(flag))
                elif stripped.startswith("#endif"):
                    stack.pop()
                elif stripped.startswith("#else") and stack:
                    stack[-1] = False
                if re.search(r"\bndsRelocCompactGroundMap|\bsNdsRelocCompact"
                             r"|\bndsRelocNormalizeCompactGroundMap"
                             r"|\bgNdsRelocCompactGroundMap", line) and \
                        not stripped.startswith(("/*", "*", "//", "#")):
                    self.assertTrue(any(stack),
                                    f"{path.name}:{number} outside the flag")
        # the call sites exist where the seams are documented
        self.assertIn("ndsRelocCompactGroundMapWallpaperKey(&asset_id",
                      SPRITE_C.read_text(encoding="utf-8"))
        self.assertIn("ndsRelocCompactGroundMapExpectedSize(desc->asset_ids[index]",
                      MATRIX_C.read_text(encoding="utf-8"))
        self.assertIn("ndsRelocCompactGroundMapPrepareTraining(file, ground_data)",
                      SHIMS_C.read_text(encoding="utf-8"))


# --------------------------------------------------------------------------
# The real C, sliced out of reloc_backend_assets.c, driven with real maps.
# --------------------------------------------------------------------------

def find_host_i686_compiler() -> str | None:
    for name in ("i686-w64-mingw32-clang", "i686-w64-mingw32-gcc"):
        found = shutil.which(name)
        if found:
            return found
    return None


def mask_comments_and_literals(text: str) -> str:
    """Same length, comments/strings blanked, so braces can be counted."""
    out = list(text)
    i, n = 0, len(text)
    while i < n:
        two = text[i:i + 2]
        if two == "/*":
            j = text.find("*/", i + 2)
            j = n if j < 0 else j + 2
            for k in range(i, j):
                if out[k] != "\n":
                    out[k] = " "
            i = j
        elif two == "//":
            j = text.find("\n", i)
            j = n if j < 0 else j
            for k in range(i, j):
                out[k] = " "
            i = j
        elif text[i] in "\"'":
            quote = text[i]
            j = i + 1
            while j < n and text[j] != quote:
                j += 2 if text[j] == "\\" else 1
            for k in range(i + 1, min(j, n)):
                out[k] = " "
            i = j + 1
        else:
            i += 1
    return "".join(out)


def extract_function(source: str, name: str) -> str:
    masked = mask_comments_and_literals(source)
    pattern = r"(?m)^static\b[^\n]*\b%s\s*\(" % re.escape(name)
    for match in re.finditer(pattern, masked):
        stop = re.search(r"[{;]", masked[match.end():])
        if stop is None or stop.group(0) == ";":
            continue                        # a prototype, not the definition
        depth, i = 0, match.end() + stop.start()
        while True:
            if masked[i] == "{":
                depth += 1
            elif masked[i] == "}":
                depth -= 1
                if depth == 0:
                    return source[match.start():i + 1]
            i += 1
    raise AssertionError(f"function {name} not found")


def extract_flag_block(source: str, marker: str) -> str:
    lines = source.splitlines()
    for i, line in enumerate(lines):
        if line.startswith("#if defined(NDS_P2_COMPACT_GROUND_MAPS)") and \
                i + 1 < len(lines) and lines[i + 1].startswith(marker):
            depth = 0
            for j in range(i, len(lines)):
                if lines[j].startswith("#if"):
                    depth += 1
                elif lines[j].startswith("#endif"):
                    depth -= 1
                    if depth == 0:
                        return "\n".join(lines[i:j + 1])
    raise AssertionError(f"flag block {marker!r} not found")


HARNESS = r'''
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <PR/sp.h>

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif
#define ARRAY_COUNT(a) (sizeof(a) / sizeof((a)[0]))
#define NDS_P2_COMPACT_GROUND_MAPS 1
#define NDS_P2_1P_GAME 1

@@ASSET_DEFINES@@
#define NDS_RELOC_SYMBOL_STAGE_DREAM_LAND_SPRITE 0x26c88u

/* MPGroundData up to `wallpaper` is all the sliced code reads; the size is the
 * source struct's (asserted by the port), the offset is gr/mp mptypes.h. */
typedef struct MPGroundData {
    struct { void *a, *b, *c, *d; } gr_desc[4];
    void *map_geometry;
    u8 layer_mask;
    Sprite *wallpaper;
    u8 rest[0xA8 - 0x4C];
} MPGroundData;
_Static_assert(offsetof(MPGroundData, wallpaper) == 0x48, "wallpaper offset");
_Static_assert(sizeof(MPGroundData) == 0xA8, "MPGroundData size");

@@LOADED_FILE_STRUCT@@

static NDSRelocLoadedFile sNdsRelocLoadedFiles[32];
static u32 sNdsRelocLoadedFileCount;
static u32 sNdsRelocLoadedFilesEpoch = 1u;
static u32 sNdsRelocSceneGeneration = 1u;

static struct { u8 scene_curr, scene_prev; } gSCManagerSceneData;
enum { nSCKindOther = 3, nSCKind1PTrainingMode = 100 };

static void *gLastAlloc;
static size_t gLastAllocSize;
static u32 gAllocCount;
static void *syTaskmanMalloc(size_t size, u32 align)
{
    u8 *raw = (u8 *)calloc(size + align, 1);
    gLastAlloc = (void *)(((uintptr_t)raw + align - 1u) & ~(uintptr_t)(align - 1u));
    gLastAllocSize = size;
    gAllocCount++;
    return gLastAlloc;
}

@@SLICES@@

static u32 le32(const u8 *p) { return p[0] | (p[1] << 8) | (p[2] << 16) | ((u32)p[3] << 24); }
static u32 le16(const u8 *p) { return p[0] | (p[1] << 8); }
static u32 be32(const u8 *p) { return ((u32)p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3]; }

typedef struct {
    u8 *raw; u32 size; const u8 *payload; u32 file_id;
    u32 internal_head, external_head, extern_count;
} O2RFile;

static void read_o2r(const char *path, O2RFile *o)
{
    FILE *f = fopen(path, "rb");
    long n;
    if (f == NULL) { fprintf(stderr, "cannot open %s\n", path); exit(2); }
    fseek(f, 0, SEEK_END); n = ftell(f); fseek(f, 0, SEEK_SET);
    o->raw = (u8 *)malloc((size_t)n);
    if (fread(o->raw, 1, (size_t)n, f) != (size_t)n) exit(2);
    fclose(f);
    if (memcmp(o->raw + 4, "OLER", 4) != 0) { fprintf(stderr, "bad magic %s\n", path); exit(2); }
    o->file_id = le32(o->raw + 0x40);
    o->internal_head = le16(o->raw + 0x44);
    o->external_head = le16(o->raw + 0x46);
    o->extern_count = le32(o->raw + 0x48);
    o->size = le32(o->raw + 0x4C + 2 * o->extern_count);
    o->payload = o->raw + 0x4C + 2 * o->extern_count + 4;
}

static void reset_registry(void)
{
    sNdsRelocLoadedFileCount = 0;
    sNdsRelocLoadedFilesEpoch++;
}

/* The loader's steps for one file: word swap, internal chain, external chain. */
static NDSRelocLoadedFile *load_file(const O2RFile *o, u32 asset_id)
{
    NDSRelocLoadedFile *lf = &sNdsRelocLoadedFiles[sNdsRelocLoadedFileCount++];
    u32 aligned = (o->size + 15u) & ~15u;
    /* Mirrors the runtime registration's drop of the compact-map record. */
    if (sNdsRelocCompactGroundMapActive.map_asset_id == asset_id)
        sNdsRelocCompactGroundMapActive.map_asset_id = 0u;
    u8 *raw = (u8 *)calloc(aligned + 16u, 1);
    u8 *data = (u8 *)(((uintptr_t)raw + 15u) & ~(uintptr_t)15u);
    u32 i, cursor, index = 0;

    memcpy(data, o->payload, o->size);
    for (i = 0; i + 4 <= o->size; i += 4) {
        u32 v = be32(data + i);
        memcpy(data + i, &v, 4);
    }
    memset(lf, 0, sizeof(*lf));
    lf->asset_id = asset_id;
    lf->data = data;
    lf->data_size = o->size;
    for (cursor = o->internal_head; cursor != 0xffffu;) {
        u32 slot = cursor * 4u, word, ptr;
        memcpy(&word, data + slot, 4);
        ptr = (u32)(uintptr_t)(data + (word & 0xffffu) * 4u);
        memcpy(data + slot, &ptr, 4);
        cursor = word >> 16;
    }
    for (cursor = o->external_head; cursor != 0xffffu; index++) {
        u32 slot = cursor * 4u, word, ptr;
        u8 *dep = (u8 *)calloc(0x40000u, 1);   /* another file's memory */
        memcpy(&word, data + slot, 4);
        ptr = (u32)(uintptr_t)(dep + (word & 0xffffu) * 4u);
        memcpy(data + slot, &ptr, 4);
        cursor = word >> 16;
    }
    if (index != o->extern_count) { fprintf(stderr, "extern chain %u != table %u\n", index, o->extern_count); exit(3); }
    return lf;
}

static int failures;
#define CHECK(cond, ...) do { if (!(cond)) { failures++; printf("FAIL %s:%d ", __func__, __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)

static int same_header(const Sprite *a, const Sprite *b)
{
    const u8 *x = (const u8 *)a, *y = (const u8 *)b;
    return memcmp(x, y, 32) == 0 && memcmp(x + 36, y + 36, 16) == 0 &&
           memcmp(x + 64, y + 64, 4) == 0 &&
           a->LUT == NULL && b->LUT == NULL && a->rsp_dl == NULL &&
           b->rsp_dl == NULL && a->rsp_dl_next == NULL && b->rsp_dl_next == NULL;
}

int main(int argc, char **argv)
{
    O2RFile compact, source, container;
    NDSRelocLoadedFile *lf, *ref;
    u32 map_id, wall_id, bm_off, spr_off, source_size;
    Sprite *sprite, *refsprite;
    MPGroundData *gd;
    u32 i, key_asset, key_off, before_count;

    if (argc != 9) { fprintf(stderr, "usage\n"); return 2; }
    read_o2r(argv[1], &compact);
    read_o2r(argv[2], &source);
    read_o2r(argv[3], &container);
    map_id = (u32)strtoul(argv[4], NULL, 0);
    wall_id = (u32)strtoul(argv[5], NULL, 0);
    bm_off = (u32)strtoul(argv[6], NULL, 0);
    spr_off = (u32)strtoul(argv[7], NULL, 0);
    source_size = (u32)strtoul(argv[8], NULL, 0);

    /* reference: the container, normalized by the code that always did it */
    ref = load_file(&container, wall_id);
    ndsRelocNormalizeStageDreamLandSprite(ref);
    refsprite = (Sprite *)((u8 *)ref->data + 0x26c88u);
    CHECK(refsprite->width == 300 && refsprite->height == 220 && refsprite->nbitmaps == 44, "reference sprite");
    CHECK(gNdsRelocCompactGroundMapCount == 0, "container normalization must not count as compact");
    reset_registry();

    /* ---- the compact map ---- */
    lf = load_file(&compact, map_id);
    gd = (MPGroundData *)((u8 *)lf->data + 0x14);
    sprite = ndsRelocCompactGroundMapSprite(lf);
    CHECK(sprite == (Sprite *)((u8 *)lf->data + spr_off), "stub sprite pointer");
    CHECK(sprite != NULL && sprite->width == 220 && sprite->height == 300, "pre-normalization lanes are swapped (%d x %d)", sprite ? sprite->width : -1, sprite ? sprite->height : -1);
    CHECK(sprite != NULL && (u8 *)sprite->bitmap == (u8 *)lf->data + bm_off, "stub bitmap pointer");
    CHECK(ndsRelocCompactGroundMapExpectedSize(map_id, source_size) == source_size + 784u, "packet size = pinned source size + 784");
    CHECK(ndsRelocCompactGroundMapExpectedSize(map_id, source_size + 16u) == source_size + 16u, "a pin that is not this file's source size is left alone");
    CHECK(ndsRelocCompactGroundMapExpectedSize(wall_id, 1234u) == 1234u, "container id is not a map");

    ndsRelocNormalizeStageDreamLandSprite(lf);
    CHECK(gNdsRelocCompactGroundMapCount == 1u, "one normalization counted, got %u", gNdsRelocCompactGroundMapCount);
    CHECK(sprite->width == 300 && sprite->height == 220 && sprite->nbitmaps == 44 && sprite->bmfmt == G_IM_FMT_RGBA && sprite->bmsiz == G_IM_SIZ_16b, "normalized stub sprite");
    CHECK(same_header(sprite, refsprite), "stub Sprite header == container's, field for field");
    for (i = 0; i < 44; i++) {
        const u8 *a = (const u8 *)&sprite->bitmap[i], *b = (const u8 *)&refsprite->bitmap[i];
        CHECK(memcmp(a, b, 8) == 0 && memcmp(a + 12, b + 12, 4) == 0, "bitmap %u fields", i);
        CHECK(sprite->bitmap[i].buf == NULL && refsprite->bitmap[i].buf != NULL, "bitmap %u buf null vs container's", i);
    }
    ndsRelocNormalizeStageDreamLandSprite(lf);
    CHECK(gNdsRelocCompactGroundMapCount == 1u, "normalizer is idempotent");

    /* wallpaper identity */
    key_asset = map_id; key_off = bm_off;
    CHECK(ndsRelocCompactGroundMapWallpaperKey(&key_asset, &key_off) == TRUE, "key translates");
    CHECK(key_asset == wall_id && key_off == 0x269c8u, "key is (container, 0x269c8), got %x/%x", key_asset, key_off);
    CHECK(gNdsRelocCompactGroundMapKeyCount == 1u, "key counted");
    key_asset = map_id; key_off = bm_off + 16u;
    CHECK(ndsRelocCompactGroundMapWallpaperKey(&key_asset, &key_off) == FALSE && key_asset == map_id && key_off == bm_off + 16u, "an offset that is not the stub table is refused, key untouched");
    key_asset = map_id; key_off = spr_off;
    CHECK(ndsRelocCompactGroundMapWallpaperKey(&key_asset, &key_off) == FALSE, "the Sprite is not the Bitmap table");
    key_asset = wall_id; key_off = 0x269c8u;
    CHECK(ndsRelocCompactGroundMapWallpaperKey(&key_asset, &key_off) == FALSE && key_asset == wall_id, "the container's own key is not translated");
    key_asset = 0x12345u; key_off = bm_off;
    CHECK(ndsRelocCompactGroundMapWallpaperKey(&key_asset, &key_off) == FALSE, "an unrelated asset is refused");

    /* Training Mode: its own wallpaper loads at wallpaper - 0x26c88 */
    {
        Sprite *original = gd->wallpaper;
        gSCManagerSceneData.scene_curr = nSCKindOther;
        before_count = gAllocCount;
        ndsRelocCompactGroundMapPrepareTraining(lf->data, gd);
        CHECK(gd->wallpaper == original && gAllocCount == before_count, "no region outside Training");
        gSCManagerSceneData.scene_curr = nSCKind1PTrainingMode;
        ndsRelocCompactGroundMapPrepareTraining(lf->data, gd);
        CHECK(gAllocCount == before_count + 1u && gLastAllocSize == 0x26cd0u, "container-sized region");
        CHECK((u8 *)gd->wallpaper - 0x26c88u == (u8 *)gLastAlloc, "training base = region base");
        /* Training then replaces `wallpaper` with its own file's Sprite. The
         * packet size check must not depend on that pointer. */
        gd->wallpaper = (Sprite *)((u8 *)gLastAlloc + 0x26c88u);
        CHECK(ndsRelocCompactGroundMapSprite(lf) == NULL, "after Training the pointer no longer says compact");
        CHECK(ndsRelocCompactGroundMapExpectedSize(map_id, source_size) == source_size + 784u, "packet size still the compact one after Training rewrote wallpaper");
        gd->wallpaper = original;
    }

    /* ---- the source map is left exactly alone ---- */
    reset_registry();
    gSCManagerSceneData.scene_curr = nSCKindOther;
    lf = load_file(&source, map_id);
    gd = (MPGroundData *)((u8 *)lf->data + 0x14);
    CHECK(ndsRelocCompactGroundMapSprite(lf) == NULL, "source map is not compact");
    CHECK(ndsRelocCompactGroundMapExpectedSize(map_id, source_size) == source_size, "source map keeps its pinned size");
    before_count = gNdsRelocCompactGroundMapCount;
    ndsRelocNormalizeStageDreamLandSprite(lf);
    CHECK(gNdsRelocCompactGroundMapCount == before_count, "source map not normalized as a stub");
    key_asset = map_id; key_off = (u32)((u8 *)gd->wallpaper - (u8 *)lf->data);
    CHECK(ndsRelocCompactGroundMapWallpaperKey(&key_asset, &key_off) == FALSE, "source map key not translated");
    gSCManagerSceneData.scene_curr = nSCKind1PTrainingMode;
    before_count = gAllocCount;
    ndsRelocCompactGroundMapPrepareTraining(lf->data, gd);
    CHECK(gAllocCount == before_count, "source map needs no training region");

    printf(failures ? "FAILED %d\n" : "ALL_OK\n", failures);
    return failures ? 1 : 0;
}
'''


class CompactSeamCTests(unittest.TestCase):
    """The real C, on the real generator output, for every map."""

    @classmethod
    def setUpClass(cls):
        cls.compiler = find_host_i686_compiler()
        if cls.compiler is None:
            return
        source = ASSETS_C.read_text(encoding="utf-8")
        defines = "\n".join(re.findall(
            r"(?m)^#define NDS_RELOC_ASSET_\w+ +(?:0x[0-9a-fA-F]+|\d+)u?$",
            source))
        loaded_file = re.search(
            r"typedef struct NDSRelocLoadedFile \{.*?\} NDSRelocLoadedFile;",
            source, re.S).group(0)
        slices = "\n\n".join([
            extract_function(source, "ndsRelocFindLoadedFileByAsset"),
            extract_function(source, "ndsRelocFindLoadedFileByData"),
            extract_function(source, "ndsRelocRangeInLoadedFile"),
            extract_function(source, "ndsRelocPointerRangeInLoadedFile"),
            extract_flag_block(source, "/* P2-2p8 T1: compact ground maps."),
            extract_function(source, "ndsRelocSwapS16Pair"),
            extract_function(source, "ndsRelocSwapSpriteAttrZDepth"),
            extract_function(source, "ndsRelocReverseSpriteColorBytes"),
            extract_function(source, "ndsRelocNormalizeSpriteHeaderFields"),
            extract_function(source, "ndsRelocNormalizeSpriteBitmapTable"),
            extract_function(source, "ndsRelocIsStageWallpaperAsset"),
            extract_flag_block(
                source, "/* A compact ground map's appended Sprite and Bitmap"),
            extract_function(source, "ndsRelocNormalizeStageDreamLandSprite"),
        ])
        cls.tmp = tempfile.TemporaryDirectory(prefix="smash64ds-compact-c-")
        tmp = Path(cls.tmp.name)
        code = (HARNESS.replace("@@ASSET_DEFINES@@", defines)
                .replace("@@LOADED_FILE_STRUCT@@", loaded_file)
                .replace("@@SLICES@@", slices))
        (tmp / "harness.c").write_text(code, encoding="utf-8")
        cls.binary = tmp / "harness.exe"
        result = subprocess.run(
            [cls.compiler, "-O1", "-Wall", "-Wextra", "-Wno-unused-function",
             "-Wno-unused-parameter", f"-I{ROOT / 'include'}",
             str(tmp / "harness.c"), "-o", str(cls.binary)],
            capture_output=True, text=True, timeout=120)
        cls.compile_output = result.stdout + result.stderr
        cls.compile_ok = result.returncode == 0
        cls.compact_dir = tmp / "compact"
        for spec in cgm.MAPS:
            compact = cgm.build_compact_map(spec, *read_pair(spec))
            cgm.write_atomic(cls.compact_dir / spec.map_path, compact.data)
            setattr(cls, "info_" + Path(spec.map_path).name, compact)

    @classmethod
    def tearDownClass(cls):
        if getattr(cls, "tmp", None) is not None:
            cls.tmp.cleanup()

    def test_sliced_c_compiles_for_a_32_bit_host(self):
        if self.compiler is None:
            self.skipTest("no i686 host compiler (llvm-mingw i686-w64-mingw32-clang)")
        self.assertTrue(self.compile_ok, self.compile_output)
        self.assertNotIn("warning:", self.compile_output, self.compile_output)

    def test_real_c_on_real_maps(self):
        if self.compiler is None:
            self.skipTest("no i686 host compiler (llvm-mingw i686-w64-mingw32-clang)")
        self.assertTrue(self.compile_ok, self.compile_output)
        for spec in cgm.MAPS:
            with self.subTest(stage=spec.label):
                info = getattr(self, "info_" + Path(spec.map_path).name)
                args = [str(self.binary),
                        str(self.compact_dir / spec.map_path),
                        str(O2R / spec.map_path),
                        str(O2R / spec.container_path),
                        hex(spec.map_file_id), hex(spec.container_asset_id),
                        hex(info.stub_bitmap_offset),
                        hex(info.stub_sprite_offset),
                        hex(info.source_size)]
                result = subprocess.run(args, capture_output=True, text=True,
                                        timeout=60)
                self.assertEqual(result.returncode, 0,
                                 result.stdout + result.stderr)
                self.assertIn("ALL_OK", result.stdout)


class CliTests(unittest.TestCase):
    def test_makefile_cli_matches_the_library_from_another_cwd(self):
        with tempfile.TemporaryDirectory(prefix="smash64ds-compact-cli-") as tmp:
            tmp = Path(tmp)
            spec = cgm.MAPS[6]                      # Dream Land
            out = tmp / "nitrofs" / "reloc" / spec.map_path
            result = subprocess.run(
                [sys.executable, str(Path(cgm.__file__).resolve()),
                 "--o2r-root", str(O2R), "--map", spec.map_path,
                 "--output", str(out)],
                cwd=tmp, capture_output=True, text=True, timeout=60)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            want = cgm.build_compact_map(spec, *read_pair(spec)).data
            self.assertEqual(out.read_bytes(), want)
            self.assertFalse(list(out.parent.glob("*.tmp")), "no temp left behind")

    def test_cli_refuses_a_map_it_was_not_written_for(self):
        with tempfile.TemporaryDirectory(prefix="smash64ds-compact-cli-") as tmp:
            result = subprocess.run(
                [sys.executable, str(Path(cgm.__file__).resolve()),
                 "--o2r-root", str(O2R), "--map", "reloc_stages/GRLastMap",
                 "--output", str(Path(tmp) / "x")],
                capture_output=True, text=True, timeout=60)
            self.assertNotEqual(result.returncode, 0)
            self.assertFalse((Path(tmp) / "x").exists())

    def test_generator_rejects_a_container_that_is_not_the_wallpaper(self):
        spec = cgm.MAPS[6]
        map_raw, container_raw = read_pair(spec)
        wrong = read_pair(cgm.MAPS[0])[1]          # another stage's container
        with self.assertRaises(cgm.CompactMapError):
            cgm.build_compact_map(spec, map_raw, wrong)
        with self.assertRaises(cgm.CompactMapError):
            cgm.build_compact_map(spec, container_raw, container_raw)
        truncated = container_raw[:-4]
        with self.assertRaises(cgm.CompactMapError):
            cgm.build_compact_map(spec, map_raw, truncated)


if __name__ == "__main__":
    unittest.main()
