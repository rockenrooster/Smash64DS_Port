"""Host-execute the native Meta Knight material policies and coverage bakes.

Family 7 retains PRIM*SHADE then COMBINED*PRIM and IA8 alpha*ENV_A.
Family 6 is an IMPLEMENTED_NOT_ACCEPTED C_A=0/LOD=1 experiment; source
feedback and bilinear-edge comparison remain due. These tests cannot accept it.
"""

from pathlib import Path
import re
import shutil
import struct
import subprocess
import sys
import tempfile
import unittest
import os

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/menus"))
from source_test_helpers import braced, function

SOURCE = (ROOT / "src/nds/nds_renderer_textures_effects.c").read_text()
COMMON = (ROOT / "src/nds/nds_renderer_native_common.c").read_text()
CONSTANTS = (ROOT / "src/nds/nds_renderer_preamble.c").read_text() + "\n" + \
            (ROOT / "include/nds/nds_renderer.h").read_text()
ROSTER = (ROOT / "include/nds/nds_p4_roster.h").read_text().replace("#include <PR/ultratypes.h>", "")
CONSTANTS += "\n" + ROSTER + "\n#define NDS_FIGHTER_ADMISSION_KINDS (12u + NDS_P4_METAKNIGHT)\n"


FUNCTIONS = (
    "ndsRendererCombineUsesColor", "ndsRendererCombineOutputUsesColor",
    "ndsRendererCombineSecondOutputUsesColor", "ndsRendererHardwareUseSecondCycle",
    "ndsRendererHardwareMetaKnightFeedbackSurface",
    "ndsRendererHardwareMetaKnightPrimSquaredSurface",
    "ndsRendererHardwareSquarePrimitiveRgb",
    "ndsRendererHardwareAlphaOnlyIa8Coverage5", "ndsRendererHardwareGradedCoverageByte",
    "ndsRendererHardwareSecondCycleIsShadePassThrough",
    "ndsRendererHardwarePrimEnvTexel0BlendMode", "ndsRendererHardwareBlendModeKeepsTexelCoverage",
    "ndsRendererHardwareSecondCyclePassesCombined", "ndsRendererHardwareOutputUsesColor",
    "ndsRendererCombineUsesAlpha", "ndsRendererCombineSecondOutputUsesAlpha",
    "ndsRendererHardwareOutputUsesAlpha", "ndsRendererHardwareUseDecal",
    "ndsRendererHardwarePrimitiveDecal", "ndsRendererHardwareUseTexture",
    "ndsRendererHardwareUsesLitPrimitiveModulate", "ndsRendererHardwareColorSource",
    "ndsRendererHardwareLitShadeCombine", "ndsRendererHardwareUseMaterialColor",
    "ndsRendererHardwareUseVertexColor", "ndsRendererHardwareBlendAlphaUsesMemory",
    "ndsRendererHardwareAlpha", "ndsRendererHardwareAlphaUsesVertex",
)


def macro_closure(source):
    definitions = dict(re.findall(r"^#define\s+(NDS_\w+)\s+([^\n]+)", CONSTANTS, re.M))
    required = set(re.findall(r"\bNDS_\w+\b", source)) - {"NDS_P4_METAKNIGHT", "NDS_RENDERER_PROFILE_LEVEL"}
    result = []
    while required:
        name = required.pop()
        if name not in definitions:
            raise AssertionError(f"Missing production constant: {name}")
        value = definitions.pop(name)
        result.append(f"#define {name} {value}")
        required.update(n for n in re.findall(r"\bNDS_\w+\b", value) if n in definitions)
    return "\n".join(result)


class MetaKnightMaterialTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cc = shutil.which("gcc") or shutil.which("clang")
        if cc is None:
            raise unittest.SkipTest("host C compiler unavailable")
        cls.temporary = tempfile.TemporaryDirectory()
        path = Path(cls.temporary.name)
        bodies = "\n".join(function(SOURCE, name) for name in FUNCTIONS)
        bodies += "\n" + function(COMMON,"ndsRendererNativeMetaKnightRunTransparent")
        bodies += "\n" + function(COMMON,"ndsRendererNativeRecordTransparentRun")
        bodies += "\n" + function(SOURCE,"ndsFtrAdmitIndexForKind")
        fixture = r'''
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int32_t s32;
typedef int32_t sb32;
enum { FALSE=0, TRUE=1 };
typedef struct {
    u32 texture_combine_count, texture_combine_w0, texture_combine_w1;
    u32 geometry_mode, othermode_h, othermode_l, prim_color, env_color;
    u32 texture_state_flags;
    u32 triangle_count;
} NDSRendererStats;
typedef struct { u8 a; } NDSRendererInputVertex;
static s32 ndsRendererHardwareTextureImplicitStateOn(const NDSRendererStats *s) {
    (void)s; return TRUE;
}
static void ndsRendererHardwareRecordUseTextureReject(const NDSRendererStats *s,u32 reason) {
    (void)s; (void)reason;
}
static volatile u32 gNdsP4TransparentRunCount, gNdsP4TransparentTriangleCount;
''' + ROSTER
        main = r'''
int main(int argc,char **argv) {
    NDSRendererStats s={0}; NDSRendererInputVertex v={255};
    if(argc!=7) return 2;
    s.texture_combine_count=1;
    s.texture_combine_w0=(u32)strtoul(argv[1],NULL,16);
    s.texture_combine_w1=(u32)strtoul(argv[2],NULL,16);
    s.prim_color=(u32)strtoul(argv[3],NULL,16);
    s.env_color=(u32)strtoul(argv[4],NULL,16);
    s.othermode_h=(u32)strtoul(argv[5],NULL,16);
    s.othermode_l=(u32)strtoul(argv[6],NULL,16);
    s.geometry_mode=NDS_RENDERER_GEOM_LIGHTING;
    s.texture_state_flags=NDS_RENDERER_TEXTURE_STATE_ON;
    printf("%x %u %u %u %u %u %u %u\n",
        ndsRendererHardwareColorSource(&s),
        ndsRendererHardwareUseMaterialColor(&s),
        ndsRendererHardwareUseVertexColor(&s),
        ndsRendererHardwareUseTexture(&s),
        ndsRendererHardwareAlpha(&s,&v),
        ndsRendererHardwareAlphaUsesVertex(&s),
        ndsRendererHardwareOutputUsesAlpha(&s,NDS_RENDERER_ACMUX_TEXEL0),
        ndsRendererHardwareOutputUsesAlpha(&s,NDS_RENDERER_ACMUX_ENVIRONMENT));
    for(unsigned i=0;i<256;i++) {
        unsigned a=ndsRendererHardwareAlphaOnlyIa8Coverage5(i);
        printf("%u %u %u\n", a, ndsRendererHardwareGradedCoverageByte(a,TRUE),
            ndsRendererHardwareGradedCoverageByte(a,FALSE));
    }
    /* One resident view is retained while live ENV alpha moves through zero.
     * The production transparent policy/record bodies do not mutate it. */
    struct { unsigned valid,name,generation,last_used; } resident={1,77,9,14};
    const unsigned alphas[3]={255,0,255};
    unsigned emitted=0;
    s.texture_combine_w0=0xfc321803; s.texture_combine_w1=0xff17ffff;
    s.othermode_h=0x100000;
    for(unsigned i=0;i<3;i++) {
        s.env_color=0xffffff00u|alphas[i];
        unsigned alpha=ndsRendererHardwareAlpha(&s,NULL);
        if(ndsRendererNativeMetaKnightRunTransparent(&s,alpha))
            ndsRendererNativeRecordTransparentRun(&s,2);
        else emitted+=2;
    }
    printf("lifecycle %u %u %u %u %u %u %u\n",emitted,
        gNdsP4TransparentRunCount,gNdsP4TransparentTriangleCount,
        resident.valid,resident.name,resident.generation,resident.last_used);
    printf("admission %u %u %u %u %u %u\n",ndsFtrAdmitIndexForKind(0,0),
        ndsFtrAdmitIndexForKind(11,1),ndsFtrAdmitIndexForKind(29,0),
        ndsFtrAdmitIndexForKind(29,1),ndsFtrAdmitIndexForKind(28,0),
        ndsFtrAdmitIndexForKind(29,2));
    return 0;
}
'''
        cls.executables = {}
        for enabled in (0, 1):
            cfile, executable = path / f"materials{enabled}.c", path / f"materials{enabled}.exe"
            cfile.write_text(f"#define NDS_P4_METAKNIGHT {enabled}\n#define NDS_RENDERER_PROFILE_LEVEL 0\n" +
                             fixture + macro_closure(bodies) + "\n" + bodies + main)
            built = subprocess.run([cc, "-std=c11", "-Wall", "-Wextra", "-Werror",
                                    str(cfile), "-o", str(executable)], capture_output=True, text=True)
            if built.returncode:
                raise AssertionError(built.stderr)
            cls.executables[enabled] = executable

    @classmethod
    def tearDownClass(cls):
        cls.temporary.cleanup()

    def run_material(self, enabled, w0, w1, prim=0xF8F8C0FF, env=0xFFFFFFFF,
                     high=0x100000, low=0x00504130):
        result = subprocess.run([str(self.executables[enabled])] +
                                [f"{v:x}" for v in (w0,w1,prim,env,high,low)],
                                capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        lines = result.stdout.splitlines()
        first = lines[0].split()
        return [int(first[0],16)] + [int(x) for x in first[1:]], \
               [tuple(map(int,line.split())) for line in lines[1:257]]

    def test_transparent_transition_and_resident_view_lifetime(self):
        result=subprocess.run([str(self.executables[1])] +
                              [f"{v:x}" for v in (0xfc321803,0xff17ffff,0xf8f8c0ff,
                                                   0xffffffff,0x100000,0x00504130)],
                              capture_output=True,text=True)
        self.assertEqual(result.returncode,0,result.stderr)
        self.assertEqual(result.stdout.splitlines()[-2],"lifecycle 4 1 2 1 77 9 14")
        # Positive tiny source alpha must not be confused with source zero or
        # sent as DS wireframe: preserve visible output in the minimum A5 lane.
        small,_=self.run_material(1,0xfc321803,0xff17ffff,env=0xffffff01)
        self.assertEqual(small[4],1)
        zero,_=self.run_material(1,0xfc321803,0xff17ffff,env=0xffffff00)
        self.assertEqual(zero[4],0)
        # Consumer boundaries retain state/texture preparation, then stop only
        # the pixel-emission portion. Both native modes and hierarchy do this.
        ordinary=function(COMMON,"ndsRendererNativeSubmitProductionRun")
        self.assertLess(ordinary.index("ndsRendererNativePrepareProductionRun("),
                        ordinary.index("ndsRendererNativeMetaKnightRunTransparent("))
        self.assertLess(ordinary.index("ndsRendererNativeRecordTransparentRun("),
                        ordinary.index("ndsRendererNativeEmitProductionCrossRun("))
        lean=braced(COMMON,r"^ndsFtrLeanMatRun\([^;]*?\)\s*\{")
        self.assertLess(lean.index("ndsFtrLeanMatNoteTexture("),
                        lean.index("ndsRendererNativeRecordTransparentRun("))
        self.assertLess(lean.index("ndsRendererNativeRecordTransparentRun("),
                        lean.index("ndsFtrLeanMatCorners("))
        hierarchy=function(COMMON,"ndsRendererNativeCommitHierarchyRoot")
        self.assertIn("texture_entry->last_used_frame",hierarchy)
        self.assertLess(hierarchy.index("ndsRendererNativeRecordTransparentRun("),
                        hierarchy.index("ndsRendererNativeBeginHierarchyBatch("))

    def test_admission_uses_compact_identity_and_rejects_sentinels(self):
        args=[f"{v:x}" for v in (0xfc321803,0xff17ffff,0xf8f8c0ff,
                                 0xffffffff,0x100000,0x00504130)]
        for enabled in (0,1):
            result=subprocess.run([str(self.executables[enabled])]+args,
                                  capture_output=True,text=True)
            self.assertEqual(result.returncode,0,result.stderr)
            expected=("admission 4 50 52 54 4294967295 4294967295" if enabled else
                      "admission 4 50 4294967295 4294967295 4294967295 4294967295")
            self.assertEqual(result.stdout.splitlines()[-1],expected)

    def test_legacy_families_are_unchanged_with_p4_enabled(self):
        legacy = ((0xfc127e05,0xff17f3ff),(0xfc327e05,0xff17fdff),
                  (0xfcfffe05,0xff167dff),(0xfc327e05,0xff17f7ff),
                  (0xfc127fff,0xfffff238),(0xfcffffff,0xfffe7c38))
        for words in legacy:
            for prim,env in ((0xFFFFFFFF,0xFFFFFFFF),(0x80112230,0x33112287)):
                a,_=self.run_material(0,*words,prim,env)
                b,_=self.run_material(1,*words,prim,env)
                self.assertEqual(a,b)

    def test_prim_squared_rgb_and_alpha_coverage_dependencies(self):
        values,_=self.run_material(1,0xfc321803,0xff17ffff,env=0x12010287)
        self.assertEqual(values,[0xF1F191FF,1,1,1,16,0,1,1])
        # A dropped second factor would produce F8F8C0, visibly brighter.
        self.assertNotEqual(values[0] >> 8,0xF8F8C0)
        one_cycle,_=self.run_material(1,0xfc321803,0xff17ffff,high=0)
        disabled,_=self.run_material(0,0xfc321803,0xff17ffff,high=0)
        self.assertEqual(one_cycle,disabled)
        unrelated,_=self.run_material(1,0xfc321803,0xff17fffe)
        prior,_=self.run_material(0,0xfc321803,0xff17fffe)
        self.assertEqual(unrelated,prior)

    def test_defined_feedback_experiment_keeps_texture_and_inverts_prim_alpha(self):
        values,_=self.run_material(1,0xfc123245,0x00400087,prim=0xFFFFFF30,
                                  env=0x22005587,low=0x441049D8)
        self.assertEqual(values[:6],[0xFFFFFFFF,0,1,1,25,0])
        self.assertEqual(values[6:], [1,0])
        # Binary T=0/1 makes A=T-P*T under the explicitly defined C=0 policy.
        for texel_alpha in (0,255):
            alpha0=(48*texel_alpha+127)//255
            alpha1=max(0,texel_alpha-alpha0)
            self.assertEqual(alpha1,207 if texel_alpha else 0)

    def test_alpha_only_retains_all_16_levels_and_ignores_intensity(self):
        _,rows=self.run_material(1,0xfc321803,0xff17ffff)
        self.assertEqual(len(rows),256)
        for source,(coverage,alpha_only,legacy) in enumerate(rows):
            expected=((source&15)*31+7)//15
            self.assertEqual(coverage,expected)
            self.assertEqual(alpha_only,expected<<3)
            self.assertEqual(legacy,(expected<<3)|(expected>>2))
        self.assertEqual(len(set(r[1] for r in rows[:16])),16)
        self.assertEqual([r[1] for r in rows[:16]],[r[1] for r in rows[240:]])

    def test_alpha_representation_has_distinct_cache_identity(self):
        self.assertIn("key.flags |= NDS_RENDERER_HW_TEXTURE_KEY_ALPHA_ONLY",SOURCE)
        bits=[]
        for name in ("ALPHA_ONLY","ALPHA_IGNORES_TEXELS","PRIM_RGB_TEXEL0_ALPHA","PRIM_ENV_BLEND"):
            match=re.search(rf"#define NDS_RENDERER_HW_TEXTURE_KEY_{name} \(1u << (\d+)\)",CONSTANTS)
            self.assertIsNotNone(match)
            bits.append(int(match.group(1)))
        self.assertEqual(len(set(bits)),4)

    def test_source_geometry_and_binary_ci4_palette_prerequisites(self):
        source=os.environ.get("META_KNIGHT_SOURCE_DIR")
        path=Path(source)/"character.bin" if source else ROOT/"decomp/smashremix-plus-extra/extra_characters/MetaKnight/character.bin"
        if not path.exists():
            self.skipTest("source-qualified EXTRA corpus unavailable; set META_KNIGHT_SOURCE_DIR")
        raw=path.read_bytes()
        for offset,count in ((0x9390,4),(0x94b8,4),(0x95e0,27),(0x98c0,27)):
            for index in range(count):
                self.assertEqual(struct.unpack_from(">I",raw,offset+index*16+12)[0],0xFFFFFFFF)
        palette=struct.unpack_from(">16H",raw,0xF648)
        pixels=raw[0xF670:0xF870]
        indices={i for byte in pixels for i in (byte>>4,byte&15)}
        self.assertEqual(indices,set(range(7)))
        self.assertEqual([palette[i]&1 for i in sorted(indices)],[0,1,1,1,1,1,1])
        # Filtering across this actual opaque/transparent boundary can create
        # fractional alpha; these source facts do not accept the experiment.
        self.assertTrue(any((palette[i]&1)==0 for i in indices))
        self.assertTrue(any((palette[i]&1)==1 for i in indices))


if __name__ == "__main__":
    unittest.main()
