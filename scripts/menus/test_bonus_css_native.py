"""Bonus Practice character select: native screen against the source's SObjs.

mnplayers1pbonus.c stays the behaviour owner; nds_menu_shell_onep.c presents
it from baked BONUS_* surfaces (scripts/menus/generate_mn_ui_kit.py).  Two
claims make that presentation the source's, and both are checked here on a
host compiler rather than read off a screenshot:

1. THE BAKE PLACES WHAT THE SOURCE PLACES.  The ACTUAL decomp routines that
   build the select's static and state art -- MakeGate, MakeNameAndEmblem,
   MakeLabels, MakeBestTime, MakeBestTaskCount, MakeTotalTime and the
   MakeNumber they share -- run against recording stubs, and every SObj they
   leave (sprite, position, primitive, environment) must be a placement of the
   matching BONUS_* surface, and nothing more.

2. THE RUNTIME DRAWS THE RECORD THE SOURCE DRAWS.  The ACTUAL draw-seam reader
   (battleship_mnplayers1pbonus.c ndsMNPlayers1PBonusPresentNative) and the
   ACTUAL presenter queue (nds_menu_shell_onep.c ndsMenuShellBonusCss*) run on
   the same save data and puck position as the source's MakeHiScore /
   MakeTotalTime, and the digit glyphs they place must be the source's digit
   SObjs, in kind, count and position (4/5 frame, the bake's own rounding).

Every C body is extracted verbatim (source_test_helpers.function); only types
and object-manager stubs surround it.  A control perturbs the presenter's digit
pitch and must fail.  No ROM build, emulator or save file is touched.
"""

import importlib.util
import re
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from source_test_helpers import (  # noqa: E402
    DECOMP, braced, function, original_enum)

ROOT = Path(__file__).resolve().parents[2]
BONUS_SRC = (DECOMP / "mn/mnplayers/mnplayers1pbonus.c").read_text()
BONUS_TU = (ROOT / "src/import/battleship_mnplayers1pbonus.c").read_text()
ONEP_C = (ROOT / "src/nds/nds_menu_shell_onep.c").read_text()
SHELL_H = (ROOT / "include/nds/nds_menu_shell.h").read_text()
KIT_H = (ROOT / "include/nds/nds_ui_kit.h").read_text()


def load_bake():
    path = ROOT / "scripts/menus/generate_mn_ui_kit.py"
    spec = importlib.util.spec_from_file_location("bonus_css_bake", path)
    module = importlib.util.module_from_spec(spec)
    sys.modules["bonus_css_bake"] = module
    spec.loader.exec_module(module)
    return module


BAKE = load_bake()

SOURCE_FUNCS = (
    "mnPlayers1PBonusGetPowerOf",
    "mnPlayers1PBonusSetDigitColors",
    "mnPlayers1PBonusGetNumberDigitCount",
    "mnPlayers1PBonusMakeNumber",
    "mnPlayers1PBonusCheckFighterLocked",
    "mnPlayers1PBonusGetFighterKind",
    "mnPlayers1PBonusMakeNameAndEmblem",
    "mnPlayers1PBonusSetGateLUT",
    "mnPlayers1PBonusMakeGate",
    "mnPlayers1PBonusMakeLabels",
    "mnPlayers1PBonusGetBestTime",
    "mnPlayers1PBonusGetMins",
    "mnPlayers1PBonusGetSec",
    "mnPlayers1PBonusGetCSec",
    "mnPlayers1PBonusGetTotalMins",
    "mnPlayers1PBonusGetTotalSec",
    "mnPlayers1PBonusGetTotalCSec",
    "mnPlayers1PBonusGetForcePuckFighterKind",
    "mnPlayers1PBonusMakeBestTime",
    "mnPlayers1PBonusGetBestTaskCount",
    "mnPlayers1PBonusMakeBestTaskCount",
    "mnPlayers1PBonusCheckBonusComplete",
    "mnPlayers1PBonusMakeHiScore",
    "mnPlayers1PBonusMakeTotalTime",
)
NATIVE_FUNCS = (
    "ndsMenuShellBonusCssQueue",
    "ndsMenuShellBonusCssNumber",
    "ndsMenuShellBonusCssRecord",
    "ndsMenuShellBonusCssTotal",
)


def decomp_defines(rel, names):
    text = (DECOMP / rel).read_text()
    out = []
    for name in names:
        match = re.search(rf"^#define\s+{name}\b.*$", text, re.M)
        if match is None:
            raise AssertionError(f"{rel}: no #define {name}")
        out.append(match.group(0))
    return "\n".join(out) + "\n"


def native_defines(text, prefix):
    return "\n".join(line.strip() for line in text.splitlines()
                     if line.strip().startswith("#define " + prefix)) + "\n"


def surface_ids():
    """The manifest's BONUS_* ids, in the bake's own emit order."""
    tokens = [spec.token for spec in BAKE.BONUS_CSS_SURFACE_SPECS]
    return tokens, "".join(
        f"#define NDS_MN_UI_KIT_SURFACE_{token} {1000 + index}u\n"
        for index, token in enumerate(tokens))


PREAMBLE = r'''
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define FALSE 0
#define TRUE 1
#define REGION_US 1
typedef int8_t s8;
typedef uint8_t u8;
typedef uint8_t ub8;
typedef int16_t s16;
typedef uint16_t u16;
typedef int32_t s32;
typedef uint32_t u32;
typedef float f32;
typedef int sb32;
typedef u16 NdsUiKitSurfaceId;
typedef struct Vec2f { f32 x, y; } Vec2f; /* decomp include/ssb_types.h */
typedef struct Sprite { int unused; } Sprite;
typedef struct SObj {
    struct { u32 attr; u8 red, green, blue; int *LUT; } sprite;
    struct { u8 r, g, b; } envcolor;
    struct { f32 x, y; } pos;
    const void *source;
    struct SObj *next;
} SObj;
typedef struct GObj { SObj *obj; u32 flags; int live; } GObj;
#define SObjGetStruct(gobj) ((gobj)->obj)
#define GOBJ_FLAG_NONE 0u
#define GOBJ_FLAG_HIDDEN 1u
#define GOBJ_PRIORITY_DEFAULT 0x80000000u
#define nGCProcessKindFunc 1
#define lbRelocGetFileData(type, file, offset) ((type)(intptr_t)(offset))
static void lbCommonDrawSObjAttr(GObj *gobj) { (void)gobj; }
/* Object-manager stubs: every GObj a Make* creates is kept so the program
 * can print what it drew; eject only marks it dead. */
static GObj sPool[64];
static SObj sSObjs[512];
static int sGObjCount, sSObjCount;
static GObj *gcMakeGObjSPAfter(u32 id, void *func, s32 link, u32 priority)
{
    GObj *gobj = &sPool[sGObjCount++];
    (void)id; (void)func; (void)link; (void)priority;
    memset(gobj, 0, sizeof(*gobj));
    gobj->live = 1;
    return gobj;
}
static void gcAddGObjDisplay(GObj *gobj, void (*proc)(GObj *), s32 link,
                             u32 priority, u32 mask)
{ (void)gobj; (void)proc; (void)link; (void)priority; (void)mask; }
static void gcEjectGObj(GObj *gobj) { gobj->live = 0; }
static void gcRemoveSObjAll(GObj *gobj) { gobj->obj = NULL; }
static SObj *lbCommonMakeSObjForGObj(GObj *gobj, Sprite *sprite)
{
    SObj *sobj = &sSObjs[sSObjCount++];
    SObj **tail = &gobj->obj;

    memset(sobj, 0, sizeof(*sobj));
    sobj->source = sprite;
    /* Every sprite this select draws is authored with PRIM FF/FF/FF and the
     * SObj starts with a black ENV: an unset colour reads as the asset's. */
    sobj->sprite.red = sobj->sprite.green = sobj->sprite.blue = 0xFF;
    while (*tail != NULL) tail = &(*tail)->next;
    *tail = sobj;
    return sobj;
}
static GObj *lbCommonMakeSpriteGObj(u32 id, void *proc, s32 link, u32 prio,
                                    void (*display)(GObj *), s32 dl_link,
                                    u32 dl_prio, u32 tag, Sprite *sprite,
                                    s32 kind, void *update, s32 update_prio)
{
    GObj *gobj = gcMakeGObjSPAfter(id, proc, link, prio);
    (void)display; (void)dl_link; (void)dl_prio; (void)tag; (void)kind;
    (void)update; (void)update_prio;
    lbCommonMakeSObjForGObj(gobj, sprite);
    return gobj;
}
static void *func_800269C0_275C0(u32 id) { (void)id; return NULL; }
#define nSYAudioVoiceAnnounceBreakTheTargets 1u
#define nSYAudioVoiceAnnounceBoardThePlatforms 2u
'''


def symbol_block(text):
    names = sorted(set(re.findall(r"&\s*(ll\w+)", text)))
    decls = "".join(f"static char {name};\n" for name in names)
    rows = "".join(f'    {{ &{name}, "{name}" }},\n' for name in names)
    table = ("static const struct { const void *at; const char *name; } "
             "sNames[] = {\n" + rows + "};\n"
             "static const char *symbol_name(const void *at)\n{\n"
             "    size_t i;\n"
             "    for (i = 0; i < sizeof(sNames) / sizeof(sNames[0]); i++)\n"
             "        if (sNames[i].at == at) return sNames[i].name;\n"
             '    return "?";\n}\n')
    return decls + table


HARNESS_STATE = r'''
static void *sMNPlayers1PBonusFiles[11];
static s32 sMNPlayers1PBonusBonusKind;
static u16 sMNPlayers1PBonusFighterMask;
static GObj *sMNPlayers1PBonusHiScoreGObj;
static GObj *sMNPlayers1PBonusTotalTimeGObj;
static GObj *sMNPlayers1PBonusGameModeGObj;
static sb32 sMNPlayers1PBonusIsSelected;
static s32 sMNPlayers1PBonusReadyBlinkWait;
static struct {
    GObj *cursor, *puck, *panel, *name_emblem_gobj;
    s32 fkind, cursor_status;
} sMNPlayers1PBonusSlot;
static struct { LBBackup1PRecord spgame_records[12]; } gSCManagerBackupData;
static u32 gNdsOnePlayerCssNativeSurfaceFailCount;
static NdsMenuShellBonusCssState sPresented;
static void ndsMenuShellBonusCssPresent(const NdsMenuShellBonusCssState *s)
{ sPresented = *s; }
'''

MAIN = r'''
static void print_drawn(const char *tag, int first)
{
    int g;
    for (g = first; g < sGObjCount; g++)
    {
        SObj *s;
        if (!sPool[g].live) continue;
        for (s = sPool[g].obj; s != NULL; s = s->next)
            printf("SRC %s %s %.1f %.1f %u %u %u %u %u %u %s\n", tag,
                   symbol_name(s->source), s->pos.x, s->pos.y,
                   s->sprite.red, s->sprite.green, s->sprite.blue,
                   s->envcolor.r, s->envcolor.g, s->envcolor.b,
                   (s->sprite.LUT != NULL) ?
                       symbol_name(s->sprite.LUT) : "-");
    }
}
static void print_native(const char *tag)
{
    u32 i;
    for (i = 0; i < sNdsBonusCssBatch.count; i++)
        printf("DS %s %u %d %d\n", tag,
               (unsigned)sNdsBonusCssBatch.surface[i],
               sNdsBonusCssBatch.site[2u * i],
               sNdsBonusCssBatch.site[2u * i + 1u]);
}
/* Park the puck on a portrait cell: GetForcePuckFighterKind reads its centre
 * (pos + 13, pos + 12), so the cell's own top-left minus that lands inside. */
static void park_puck(int portrait)
{
    static GObj puck;
    static SObj puck_sobj;
    puck.obj = &puck_sobj;
    puck_sobj.pos.x = (f32)(((portrait % 6) * 45) + 25 + 20 - 13);
    puck_sobj.pos.y = (f32)(((portrait / 6) * 43) + 36 + 20 - 12);
    sMNPlayers1PBonusSlot.puck = &puck;
}
static void native_state(const char *tag)
{
    int first = sGObjCount;
    static GObj cursor;
    static SObj cursor_sobj;
    cursor.obj = &cursor_sobj;
    sMNPlayers1PBonusSlot.cursor = &cursor;
    (void)first;
    ndsMNPlayers1PBonusPresentNative();
    sNdsBonusCssBatch.count = 0u;
    ndsMenuShellBonusCssRecord(&sPresented);
    ndsMenuShellBonusCssTotal(&sPresented);
    print_native(tag);
}
static void scenario(const char *tag, int kind, int portrait, int total)
{
    int first = sGObjCount;
    sMNPlayers1PBonusBonusKind = kind;
    park_puck(portrait);
    sMNPlayers1PBonusHiScoreGObj = NULL;
    sMNPlayers1PBonusTotalTimeGObj = NULL;
    mnPlayers1PBonusMakeHiScore();
    if (total) mnPlayers1PBonusMakeTotalTime();
    print_drawn(tag, first);
    native_state(tag);
}
int main(void)
{
    int fkind, first, i;

    sMNPlayers1PBonusFighterMask = 0xFFFF;
    first = sGObjCount;
    mnPlayers1PBonusMakeGate(0);
    print_drawn("GATE", first);
    for (fkind = 0; fkind < 12; fkind++)
    {
        char tag[16];
        GObj *gobj = gcMakeGObjSPAfter(0, NULL, 22, GOBJ_PRIORITY_DEFAULT);
        first = sGObjCount - 1;
        mnPlayers1PBonusMakeNameAndEmblem(gobj, 0, fkind);
        snprintf(tag, sizeof(tag), "NAME%d", fkind);
        print_drawn(tag, first);
    }
    for (i = 0; i < 2; i++)
    {
        first = sGObjCount;
        sMNPlayers1PBonusBonusKind = i;
        mnPlayers1PBonusMakeLabels();
        print_drawn(i ? "LABELS1" : "LABELS0", first);
    }
    /* Records: Mario (portrait 1) cleared with 1'23"45-ish, Fox (portrait 9)
     * at 7 targets, every fighter cleared for the all-fighter total. */
    for (i = 0; i < 12; i++)
    {
        gSCManagerBackupData.spgame_records[i].bonus1_task_count = 10;
        gSCManagerBackupData.spgame_records[i].bonus2_task_count = 10;
        gSCManagerBackupData.spgame_records[i].bonus1_time = 3000u + 977u * i;
        gSCManagerBackupData.spgame_records[i].bonus2_time = 4111u + 1301u * i;
    }
    gSCManagerBackupData.spgame_records[0].bonus1_time = 5025u;
    scenario("TIME", 0, 1, 1);
    gSCManagerBackupData.spgame_records[1].bonus1_task_count = 7;
    scenario("COUNT0", 0, 9, 0);
    gSCManagerBackupData.spgame_records[1].bonus2_task_count = 3;
    scenario("COUNT1", 1, 9, 0);
    gSCManagerBackupData.spgame_records[1].bonus2_task_count = 10;
    scenario("TOTAL1", 1, 4, 1);
    return 0;
}
'''


def build_program(onep_text=ONEP_C):
    _tokens, ids = surface_ids()
    lb_record = braced((DECOMP / "lb/lbtypes.h").read_text(),
                       r"^struct LBBackup1PRecord", semicolon=True)
    source = "\n".join(function(BONUS_SRC, name) for name in SOURCE_FUNCS)
    state_type = braced(SHELL_H, r"^typedef struct NdsMenuShellBonusCssState",
                        semicolon=True)
    wrapper_statics = "\n".join(
        re.findall(r"^static [^\n;(]*sNdsMNPlayers1PBonusTotal\w*[^\n]*;$",
                   BONUS_TU, re.M))
    wrapper = function(BONUS_TU, "ndsMNPlayers1PBonusPresentNative")
    batch = braced(onep_text, r"^static struct \{", semicolon=True)
    native = "\n".join(function(onep_text, name) for name in NATIVE_FUNCS)
    return (PREAMBLE +
            decomp_defines("../include/macros.h",
                           ("UPDATE_INTERVAL", "TIME_SEC", "TIME_MIN",
                            "TIME_HRS", "I_HRS_TO_TICS")) +
            decomp_defines("../include/PR/sp.h",
                           ("SP_TRANSPARENT", "SP_FASTCOPY")) +
            decomp_defines("sc/scdef.h", ("SCBATTLE_BONUSGAME_TASK_MAX",)) +
            original_enum("ft/ftdef.h", "FTKind") + "\n" +
            "typedef struct LBBackup1PRecord LBBackup1PRecord;\n" +
            lb_record + "\n" +
            native_defines(SHELL_H, "NDS_MENU_SHELL_") +
            native_defines(KIT_H, "NDS_UI_KIT_SITE_BAKED") +
            native_defines(onep_text, "NDS_BONUS_CSS_") +
            ids + state_type + "\n" + HARNESS_STATE +
            symbol_block(source) + source + "\n" + wrapper_statics + "\n" +
            batch + "\n" + native + "\n" + wrapper + "\n" + MAIN)


def run_program(text):
    compiler = shutil.which("gcc") or shutil.which("clang")
    if compiler is None:
        raise unittest.SkipTest("host C compiler required")
    with tempfile.TemporaryDirectory(prefix="smash64ds-bonus-css-") as temp:
        source = Path(temp) / "bonus_css.c"
        program = Path(temp) / "bonus_css.exe"
        source.write_text(text)
        build = subprocess.run(
            [compiler, "-std=c11", "-Wall", "-Wno-unused-variable",
             "-Wno-unused-parameter", "-Wno-unused-function",
             "-Wno-int-conversion", "-Wno-missing-braces", str(source),
             "-o", str(program)], capture_output=True, text=True, timeout=120)
        if build.returncode != 0:
            raise AssertionError(f"harness build failed:\n{build.stderr}")
        run = subprocess.run([str(program)], capture_output=True, text=True,
                             timeout=60)
        if run.returncode != 0:
            raise AssertionError(f"harness failed:\n{run.stderr}")
        return run.stdout


def parse(stdout):
    tokens, _ids = surface_ids()
    source, native = {}, {}
    for line in stdout.splitlines():
        fields = line.split()
        if fields[0] == "SRC":
            tag, name = fields[1], fields[2]
            x, y = float(fields[3]), float(fields[4])
            prim = tuple(int(v) for v in fields[5:8])
            env = tuple(int(v) for v in fields[8:11])
            source.setdefault(tag, []).append((name, x, y, prim, env,
                                               fields[11]))
        elif fields[0] == "DS":
            tag = fields[1]
            native.setdefault(tag, []).append(
                (tokens[int(fields[2]) - 1000], int(fields[3]),
                 int(fields[4])))
    return source, native


def placements(parts):
    return sorted((p.symbol, float(p.x), float(p.y),
                   p.tint if p.tint is not None else (0xFF, 0xFF, 0xFF),
                   p.env if p.env is not None else (0, 0, 0))
                  for p in parts)


def drawn(rows, digits=False):
    return sorted(row[:5] for row in rows
                  if row[0].startswith("llIFCommonDigits") == digits)


class BonusCssNativeTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.source, cls.native = parse(run_program(build_program()))
        cls.states = {spec.token: spec
                      for spec in BAKE.BONUS_CSS_SURFACE_SPECS}

    def test_base_and_gate_are_the_source_placements(self):
        base = placements(self.states["BONUS_CSS_SCREEN"].parts)
        gate = drawn(self.source["GATE"])
        back = [row for row in drawn(self.source["LABELS0"])
                if row[0] == "llMNPlayersCommonBackButtonSprite"]
        for row in gate + back:
            self.assertIn(row, base)
        # The gate's palette is the GateMan1P LUT the source assigns.
        self.assertEqual(self.source["GATE"][0][5],
                         "llMNPlayersCommonGateMan1PLUT")
        for fkind, token in enumerate(BAKE.ONEP_FIGHTER_TOKEN):
            state = self.states[f"BONUS_GATE_{token}"]
            self.assertEqual(placements(state.parts),
                             drawn(self.source[f"NAME{fkind}"]), token)
        self.assertEqual(self.states["BONUS_GATE_EMPTY"].parts, ())

    def test_titles_are_the_source_labels(self):
        for kind, token in enumerate(("TARGETS", "PLATFORMS")):
            title = [row for row in drawn(self.source[f"LABELS{kind}"])
                     if row[0] != "llMNPlayersCommonBackButtonSprite"]
            self.assertEqual(
                placements(self.states[f"BONUS_TITLE_{token}"].parts), title)

    def test_record_states_are_the_source_labels(self):
        expect = {
            "TIME": "BONUS_RECORD_TIME",
            "COUNT0": "BONUS_RECORD_TARGETS",
            "COUNT1": "BONUS_RECORD_PLATFORMS",
        }
        for tag, token in expect.items():
            labels = [row for row in drawn(self.source[tag])
                      if row[2] < 206.0]
            self.assertEqual(placements(self.states[token].parts), labels, tag)
        total = [row for row in drawn(self.source["TOTAL1"])
                 if row[2] >= 206.0]
        self.assertEqual(placements(self.states["BONUS_TOTAL_TIME"].parts),
                         total)
        self.assertEqual(self.states["BONUS_RECORD_NONE"].parts, ())
        self.assertEqual(self.states["BONUS_TOTAL_NONE"].parts, ())

    def test_digit_glyphs_are_the_source_digits(self):
        colours = {row[3:5] for tag in ("TIME", "COUNT0", "TOTAL1")
                   for row in drawn(self.source[tag], digits=True)}
        self.assertEqual(len(colours), 1)
        for digit in range(10):
            (part,) = self.states[f"BONUS_DIGIT_{digit}"].parts
            self.assertEqual(part.symbol, f"llIFCommonDigits{digit}Sprite")
            self.assertEqual((part.tint, part.env), next(iter(colours)))

    def check_native(self, source, native):
        frame = BAKE.frame_pos
        for tag in ("TIME", "COUNT0", "COUNT1", "TOTAL1"):
            want = sorted((f"BONUS_DIGIT_{name[-7]}", frame(int(x)),
                           frame(int(y)))
                          for name, x, y, *_ in source[tag]
                          if name.startswith("llIFCommonDigits"))
            got = sorted(row for row in native[tag]
                         if row[0].startswith("BONUS_DIGIT_"))
            self.assertEqual(got, want, tag)
            states = [row[0] for row in native[tag]
                      if not row[0].startswith("BONUS_DIGIT_")]
            self.assertEqual(len(states), 2, tag)

    def test_native_record_digits_are_the_source_digits(self):
        self.check_native(self.source, self.native)
        self.assertIn(("BONUS_RECORD_TIME", -32768, 0), self.native["TIME"])
        self.assertIn(("BONUS_TOTAL_TIME", -32768, 0), self.native["TIME"])
        self.assertIn(("BONUS_RECORD_TARGETS", -32768, 0),
                      self.native["COUNT0"])
        self.assertIn(("BONUS_RECORD_PLATFORMS", -32768, 0),
                      self.native["COUNT1"])
        self.assertIn(("BONUS_TOTAL_NONE", -32768, 0), self.native["COUNT0"])

    def test_control_wrong_digit_pitch_fails(self):
        """CONTROL: the digit check must be able to fail."""
        broken = ONEP_C.replace("x -= 8;", "x -= 9;", 1)
        self.assertNotEqual(broken, ONEP_C)
        source, native = parse(run_program(build_program(broken)))
        with self.assertRaises(AssertionError):
            self.check_native(source, native)

    def test_scene_wiring(self):
        draw = function(BONUS_TU, "ndsMNPlayers1PBonusDraw")
        self.assertLess(draw.index("ndsMNPlayers1PBonusPresentNative();"),
                        draw.index("gcDrawAll();"))
        start = function(BONUS_TU, "mnPlayers1PBonusStartScene")
        self.assertLess(start.index("ndsBaseMNPlayers1PBonusStartScene();"),
                        start.index("ndsMenuShellOnePlayerCssExit();"))
        backend = (ROOT / "src/port/sprite_preview_backend.c").read_text()
        sink = function(backend, "ndsMenuFillSinkOnePlayerNativeOwner")
        for scene in ("nSCKind1PGamePlayers", "nSCKind1PBonus1Players",
                      "nSCKind1PBonus2Players"):
            self.assertIn(scene, sink)
        tenant = function((ROOT / "src/nds/nds_source2d.c").read_text(),
                          "ndsS2DSceneIsTenant")
        guarded = tenant[tenant.index("#if !NDS_P2_MENU_SHELL"):
                         tenant.index("#endif", tenant.index(
                             "#if !NDS_P2_MENU_SHELL"))]
        self.assertIn("nSCKind1PBonus1Players", guarded)
        self.assertIn("nSCKind1PBonus2Players", guarded)


if __name__ == "__main__":
    unittest.main()
