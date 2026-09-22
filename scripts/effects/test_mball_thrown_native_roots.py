#!/usr/bin/env python3
"""The thrown Poke Ball's native roots, and the reuse that justifies them.

P03. Pikachu's and Jigglypuff's entry throws a Master Ball through
`efManagerMBallThrownMakeEffect`. Once its three producer defects were fixed
the effect CONSTRUCTED and then declined in the renderer -- `DIAG_NATIVE`
domain 2 (`NDS_NATIVE_FAILURE_STAGE`), root `0x9340`, reason 1
(`NDS_NATIVE_FAILURE_NO_PROGRAM`) -- not because the geometry had no bake, but
because the only admission clause for those roots demanded an ITEM GObj in the
item display layer and this is an EFFECT GObj in the effect layer.

There is no second bake, and there must never be one. Both Poke Balls are the
SAME source DObjDesc, ITCommonObject+0x9430:

  * the ground item reaches it as `ITAttributes.data` at ITCommonData+0x6E4
    (asserted by the wave-1 generator);
  * the thrown effect reaches it by reading that same fixed-up +0x6E4 pointer
    and SUBTRACTING 0x9430 to recover ITCommonObject's base, so the subtrahend
    IS this descriptor (efmanager.c:5258-5259 / the port's address-as-offset
    define).

That descriptor has exactly two drawable children, display lists 0x9250 and
0x9340, and `ndsRendererSubmitNativeItemMBall` already owns both. This checker
fails if any leg of that reuse is dropped:

  1. either generated root constant disappearing or moving;
  2. the executor no longer handling both roots;
  3. the renderer's admission block no longer admitting an effect-owned DObj,
     or no longer admitting both roots, or dropping the item;
  4. the thrown descriptor's DObjDesc/MObjSub offsets diverging from the item
     ITAttributes offsets the generator pins -- at which point the two balls
     are no longer one descriptor and reuse is no longer correct;
  5. the fighter-entry seam losing the Pikachu or the Purin call, or hiding it
     behind a silent `#if` again instead of declaring the dependency.

It reads source only: no build, no emulator, no generator run. Exit 0 pass,
1 fail, 2 tooling/parse error. `--selftest` runs the mutation unit tests.
"""
import argparse
import os
import re
import sys
import unittest

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

GEN_HEADER = os.path.join(
    ROOT, "include", "nds", "generated", "nds_native_item_mball.generated.h")
EXEC_INC = os.path.join(ROOT, "src", "nds", "nds_native_item_mball.exec.inc")
ADAPTER = os.path.join(ROOT, "src", "port", "renderer_adapter_stage.c")
EF_SYMBOLS = os.path.join(
    ROOT, "src", "import", "battleship_efmanager_symbols.h")
ENTRY_SEAM = os.path.join(
    ROOT, "src", "import", "battleship_ftcommon_entry.c")
WAVE1_GENERATOR = os.path.join(
    ROOT, "scripts", "stages", "generate_nds_native_item_wave1_core.py")

MAX_READ_BYTES = 8 * 1024 * 1024

# The source truth this row rests on. Both balls are one DObjDesc.
EXPECTED_ASSET = 86
EXPECTED_BAKED_ROOT = 0x9250
EXPECTED_LIVE_ROOT = 0x9340
EXPECTED_DOBJDESC = 0x9430
EXPECTED_MOBJSUB = 0x9120

DEFINE_RE = re.compile(
    r"^\s*#define\s+(NDS_NATIVE_ITEM_MBALL_[A-Z0-9_]+)\s+"
    r"(0[xX][0-9a-fA-F]+|\d+)[uUlL]*\s*$", re.M)
# `#define llITCommonDataMBallThrownDObjDesc (*(uintptr_t *)(uintptr_t)0x9430u)`
ADDR_AS_OFFSET_RE = re.compile(
    r"^\s*#define\s+(llITCommonDataMBallThrown[A-Za-z0-9_]+)\s+"
    r"\(\*\(uintptr_t\s*\*\)\(uintptr_t\)(0[xX][0-9a-fA-F]+)[uUlL]*\)\s*$",
    re.M)
EXPECT_PTR_RE = re.compile(
    r"_expect_ptr\(\s*attr\s*,\s*(0[xX][0-9a-fA-F]+)\s*,\s*"
    r"(0[xX][0-9a-fA-F]+)\s*,")


class CheckError(Exception):
    pass


def read_bounded(path):
    try:
        with open(path, "rb") as handle:
            data = handle.read(MAX_READ_BYTES + 1)
    except OSError as exc:
        raise CheckError("cannot read %s: %s" % (path, exc))
    if len(data) > MAX_READ_BYTES:
        raise CheckError("input too large: %s" % path)
    return data.decode("utf-8", errors="replace")


def strip_block_comments(text):
    return re.sub(r"/\*.*?\*/", " ", text, flags=re.S)


def parse_generated_defines(text):
    out = {}
    for match in DEFINE_RE.finditer(text):
        out[match.group(1)] = int(match.group(2), 0)
    if not out:
        raise CheckError("no NDS_NATIVE_ITEM_MBALL_* defines found")
    return out


def parse_thrown_offsets(text):
    """The address-as-offset defines the thrown descriptor is built from."""
    out = {}
    for match in ADDR_AS_OFFSET_RE.finditer(text):
        out[match.group(1)] = int(match.group(2), 0)
    return out


def parse_item_attributes(text):
    """`attr` pointer pins from the wave-1 generator's _mball body.

    Returns {attribute_offset: pinned_file_offset}. The two that matter are
    +0x6E4 (ITAttributes.data -> the shared DObjDesc) and +0x6E8
    (ITAttributes.p_mobjsubs -> the shared MObjSub table).
    """
    start = text.find("def _mball(")
    if start < 0:
        raise CheckError("wave-1 generator has no _mball body")
    end = text.find("\ndef ", start + 1)
    body = text[start:end if end > 0 else len(text)]
    return {int(m.group(1), 0): int(m.group(2), 0)
            for m in EXPECT_PTR_RE.finditer(body)}


def executor_roots(text):
    """Root macros the executor actually branches on."""
    code = strip_block_comments(text)
    roots = set()
    for name in ("NDS_NATIVE_ITEM_MBALL_BAKED_ROOT",
                 "NDS_NATIVE_ITEM_MBALL_LIVE_ROOT"):
        if re.search(r"root_offset\s*==\s*%s\b" % name, code):
            roots.add(name)
    return roots


def extract_mball_admission(text):
    """The renderer's MBall admission block, by its outer asset test.

    Returned with comments stripped: a checker that accepts a clause because
    it appears in a comment proves nothing.
    """
    anchor = re.search(
        r"loaded->asset_id\s*==\s*NDS_NATIVE_ITEM_MBALL_ASSET", text)
    if anchor is None:
        raise CheckError("renderer has no NDS_NATIVE_ITEM_MBALL_ASSET test")
    start = text.rfind("\n", 0, anchor.start())
    end = text.find("NDS_NATIVE_ITEM_GSHELL_ASSET", anchor.end())
    if end < 0:
        end = min(len(text), anchor.end() + 8000)
    return strip_block_comments(text[start:end])


def admission_facts(block):
    return {
        "baked_root": bool(re.search(
            r"root\s*==\s*NDS_NATIVE_ITEM_MBALL_BAKED_ROOT", block)),
        "live_root": bool(re.search(
            r"root\s*==\s*NDS_NATIVE_ITEM_MBALL_LIVE_ROOT", block)),
        "item_owner": bool(re.search(r"nITKindMBall", block)) and bool(
            re.search(r"nGCCommonKindItem", block)),
        "effect_owner": bool(re.search(r"nGCCommonKindEffect", block)) and bool(
            re.search(r"sNdsRendererAdapterEffectSubmitActive", block)),
    }


def entry_seam_facts(text):
    code = strip_block_comments(text)
    calls = []
    for match in re.finditer(
            r"nFTKind(Pikachu|Purin)\b", code):
        tail = code[match.end():match.end() + 900]
        calls.append((match.group(1),
                      "ndsEFManagerMBallThrownMakeEffectChecked" in tail))
    kinds = {kind: ok for kind, ok in calls}
    return {
        "pikachu_call": kinds.get("Pikachu", False),
        "purin_call": kinds.get("Purin", False),
        # A silent `#if NDS_P2_ITEM_CORE` around the call is what produced the
        # missing effect. The dependency must be declared, not conditional:
        # `#if !NDS_P2_ITEM_CORE` + `#error` (an UNDEFINED macro trips it too),
        # or a _Static_assert if someone prefers that form.
        "declared_dependency": bool(re.search(
            r"#if\s*!\s*NDS_P2_ITEM_CORE\s*\n\s*#error", code)) or
            ("_Static_assert(NDS_P2_ITEM_CORE" in code),
        "conditional_call": bool(re.search(
            r"#if\s+NDS_P2_ITEM_CORE\s*\n\s*\(void\)"
            r"ndsEFManagerMBallThrownMakeEffectChecked", code)),
    }


def evaluate(defines, thrown, attrs, roots, admission, seam):
    """Every failure this row can regress into, as named rows."""
    fails = []

    def want(name, got, expected):
        if got != expected:
            fails.append("%s is %s, expected %s"
                         % (name,
                            hex(got) if isinstance(got, int) else got,
                            hex(expected) if isinstance(expected, int)
                            else expected))

    want("NDS_NATIVE_ITEM_MBALL_ASSET",
         defines.get("NDS_NATIVE_ITEM_MBALL_ASSET"), EXPECTED_ASSET)
    want("NDS_NATIVE_ITEM_MBALL_BAKED_ROOT",
         defines.get("NDS_NATIVE_ITEM_MBALL_BAKED_ROOT"), EXPECTED_BAKED_ROOT)
    want("NDS_NATIVE_ITEM_MBALL_LIVE_ROOT",
         defines.get("NDS_NATIVE_ITEM_MBALL_LIVE_ROOT"), EXPECTED_LIVE_ROOT)

    if "NDS_NATIVE_ITEM_MBALL_BAKED_ROOT" not in roots:
        fails.append("executor no longer branches on the baked root 0x9250")
    if "NDS_NATIVE_ITEM_MBALL_LIVE_ROOT" not in roots:
        fails.append("executor no longer branches on the live root 0x9340")

    if not admission["baked_root"] or not admission["live_root"]:
        fails.append("renderer admission dropped one of the two MBall roots")
    if not admission["item_owner"]:
        fails.append("renderer admission dropped the ground ITEM Poke Ball")
    if not admission["effect_owner"]:
        fails.append(
            "renderer admission does not admit the thrown ENTRY effect "
            "(effect-layer DObj with an nGCCommonKindEffect parent); this is "
            "the exact NO_PROGRAM decline P03 was measured at")

    dobjdesc = thrown.get("llITCommonDataMBallThrownDObjDesc")
    mobjsub = thrown.get("llITCommonDataMBallThrownMObjSub")
    if dobjdesc is None:
        fails.append("llITCommonDataMBallThrownDObjDesc is no longer an "
                     "address-as-offset define")
    else:
        want("llITCommonDataMBallThrownDObjDesc", dobjdesc, EXPECTED_DOBJDESC)
    if mobjsub is None:
        fails.append("llITCommonDataMBallThrownMObjSub is no longer an "
                     "address-as-offset define")
    else:
        want("llITCommonDataMBallThrownMObjSub", mobjsub, EXPECTED_MOBJSUB)

    item_data = attrs.get(0x6E4)
    item_mobjsubs = attrs.get(0x6E8)
    if item_data is None or item_mobjsubs is None:
        fails.append("wave-1 generator no longer pins MBall ITAttributes "
                     "+0x6E4/+0x6E8")
    else:
        if dobjdesc is not None and item_data != dobjdesc:
            fails.append(
                "the two Poke Balls no longer share one DObjDesc: item "
                "ITAttributes.data=%s vs thrown %s. Reusing the item's native "
                "owner is only correct while these agree."
                % (hex(item_data), hex(dobjdesc)))
        if mobjsub is not None and item_mobjsubs != mobjsub:
            fails.append(
                "the two Poke Balls no longer share one MObjSub table: item "
                "%s vs thrown %s" % (hex(item_mobjsubs), hex(mobjsub)))

    if not seam["pikachu_call"]:
        fails.append("fighter-entry seam no longer throws Pikachu's ball")
    if not seam["purin_call"]:
        fails.append("fighter-entry seam no longer throws Purin's ball "
                     "(same source case label as Pikachu)")
    if seam["conditional_call"]:
        fails.append("the ball call is back behind a silent "
                     "`#if NDS_P2_ITEM_CORE`; declare the dependency instead")
    if not seam["declared_dependency"]:
        fails.append("the Pikachu/Purin -> NDS_P2_ITEM_CORE dependency is no "
                     "longer declared at the entry seam")
    return fails


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--generated-header", default=GEN_HEADER)
    parser.add_argument("--exec-inc", default=EXEC_INC)
    parser.add_argument("--adapter", default=ADAPTER)
    parser.add_argument("--symbols", default=EF_SYMBOLS)
    parser.add_argument("--entry-seam", default=ENTRY_SEAM)
    parser.add_argument("--wave1-generator", default=WAVE1_GENERATOR)
    parser.add_argument("--selftest", action="store_true")
    args = parser.parse_args(argv)
    if args.selftest:
        suite = unittest.defaultTestLoader.loadTestsFromName(__name__)
        result = unittest.TextTestRunner(verbosity=2).run(suite)
        return 0 if result.wasSuccessful() else 1
    try:
        defines = parse_generated_defines(read_bounded(args.generated_header))
        thrown = parse_thrown_offsets(read_bounded(args.symbols))
        attrs = parse_item_attributes(read_bounded(args.wave1_generator))
        roots = executor_roots(read_bounded(args.exec_inc))
        admission = admission_facts(
            extract_mball_admission(read_bounded(args.adapter)))
        seam = entry_seam_facts(read_bounded(args.entry_seam))
    except CheckError as exc:
        sys.stdout.write("ERROR: %s\n" % exc)
        return 2

    sys.stdout.write(
        "asset=%s baked_root=%s live_root=%s dobjdesc=%s mobjsub=%s\n"
        % (defines.get("NDS_NATIVE_ITEM_MBALL_ASSET"),
           hex(defines.get("NDS_NATIVE_ITEM_MBALL_BAKED_ROOT", 0)),
           hex(defines.get("NDS_NATIVE_ITEM_MBALL_LIVE_ROOT", 0)),
           hex(thrown.get("llITCommonDataMBallThrownDObjDesc", 0)),
           hex(thrown.get("llITCommonDataMBallThrownMObjSub", 0))))
    sys.stdout.write(
        "executor_roots=%d admission=%s entry=%s\n"
        % (len(roots), admission, seam))

    fails = evaluate(defines, thrown, attrs, roots, admission, seam)
    for line in fails:
        sys.stdout.write("FAIL: %s\n" % line)
    if fails:
        sys.stdout.write("FAIL: %d thrown-Poke-Ball invariant(s) broken\n"
                         % len(fails))
        return 1
    sys.stdout.write(
        "PASS: both MBall roots owned, one shared DObjDesc, item and thrown "
        "entry effect both admitted, both fighters call it\n")
    return 0


GOOD_DEFINES = {
    "NDS_NATIVE_ITEM_MBALL_ASSET": EXPECTED_ASSET,
    "NDS_NATIVE_ITEM_MBALL_BAKED_ROOT": EXPECTED_BAKED_ROOT,
    "NDS_NATIVE_ITEM_MBALL_LIVE_ROOT": EXPECTED_LIVE_ROOT,
}
GOOD_THROWN = {
    "llITCommonDataMBallThrownDObjDesc": EXPECTED_DOBJDESC,
    "llITCommonDataMBallThrownMObjSub": EXPECTED_MOBJSUB,
}
GOOD_ATTRS = {0x6E4: EXPECTED_DOBJDESC, 0x6E8: EXPECTED_MOBJSUB}
GOOD_ROOTS = {"NDS_NATIVE_ITEM_MBALL_BAKED_ROOT",
              "NDS_NATIVE_ITEM_MBALL_LIVE_ROOT"}
GOOD_ADMISSION = {"baked_root": True, "live_root": True,
                  "item_owner": True, "effect_owner": True}
GOOD_SEAM = {"pikachu_call": True, "purin_call": True,
             "declared_dependency": True, "conditional_call": False}


class MBallThrownRootTests(unittest.TestCase):
    def test_good_state_passes(self):
        self.assertEqual(
            evaluate(GOOD_DEFINES, GOOD_THROWN, GOOD_ATTRS, GOOD_ROOTS,
                     GOOD_ADMISSION, GOOD_SEAM), [])

    def test_effect_admission_removed_fails(self):
        admission = dict(GOOD_ADMISSION, effect_owner=False)
        fails = evaluate(GOOD_DEFINES, GOOD_THROWN, GOOD_ATTRS, GOOD_ROOTS,
                         admission, GOOD_SEAM)
        self.assertTrue(any("thrown ENTRY effect" in line for line in fails))

    def test_live_root_dropped_from_executor_fails(self):
        roots = {"NDS_NATIVE_ITEM_MBALL_BAKED_ROOT"}
        fails = evaluate(GOOD_DEFINES, GOOD_THROWN, GOOD_ATTRS, roots,
                         GOOD_ADMISSION, GOOD_SEAM)
        self.assertTrue(any("live root 0x9340" in line for line in fails))

    def test_baked_root_dropped_from_admission_fails(self):
        admission = dict(GOOD_ADMISSION, baked_root=False)
        fails = evaluate(GOOD_DEFINES, GOOD_THROWN, GOOD_ATTRS, GOOD_ROOTS,
                         admission, GOOD_SEAM)
        self.assertTrue(any("dropped one of the two MBall roots" in line
                            for line in fails))

    def test_descriptor_divergence_fails(self):
        attrs = {0x6E4: 0x9500, 0x6E8: EXPECTED_MOBJSUB}
        fails = evaluate(GOOD_DEFINES, GOOD_THROWN, attrs, GOOD_ROOTS,
                         GOOD_ADMISSION, GOOD_SEAM)
        self.assertTrue(any("no longer share one DObjDesc" in line
                            for line in fails))

    def test_purin_sibling_dropped_fails(self):
        seam = dict(GOOD_SEAM, purin_call=False)
        fails = evaluate(GOOD_DEFINES, GOOD_THROWN, GOOD_ATTRS, GOOD_ROOTS,
                         GOOD_ADMISSION, seam)
        self.assertTrue(any("Purin" in line for line in fails))

    def test_silent_item_core_guard_fails(self):
        seam = dict(GOOD_SEAM, conditional_call=True)
        fails = evaluate(GOOD_DEFINES, GOOD_THROWN, GOOD_ATTRS, GOOD_ROOTS,
                         GOOD_ADMISSION, seam)
        self.assertTrue(any("silent" in line for line in fails))

    def test_root_constant_moved_fails(self):
        defines = dict(GOOD_DEFINES,
                       NDS_NATIVE_ITEM_MBALL_LIVE_ROOT=0x9344)
        fails = evaluate(defines, GOOD_THROWN, GOOD_ATTRS, GOOD_ROOTS,
                         GOOD_ADMISSION, GOOD_SEAM)
        self.assertTrue(any("LIVE_ROOT" in line for line in fails))

    def test_address_as_offset_form_required(self):
        text = ("#define llITCommonDataMBallThrownDObjDesc "
                "(*(uintptr_t *)(uintptr_t)0x9430u)\n"
                "static uintptr_t llITCommonDataMBallThrownMObjSub = 0x9120u;\n")
        parsed = parse_thrown_offsets(text)
        self.assertEqual(parsed["llITCommonDataMBallThrownDObjDesc"], 0x9430)
        # A `static uintptr_t` can never resolve (the registry keys on &sym),
        # so it must NOT be accepted as a thrown offset.
        self.assertNotIn("llITCommonDataMBallThrownMObjSub", parsed)

    def test_comment_only_effect_clause_is_not_credit(self):
        block = strip_block_comments(
            "if (loaded->asset_id == NDS_NATIVE_ITEM_MBALL_ASSET)\n"
            "/* one day admit nGCCommonKindEffect with "
            "sNdsRendererAdapterEffectSubmitActive */\n"
            "{ if (root == NDS_NATIVE_ITEM_MBALL_BAKED_ROOT ||"
            " root == NDS_NATIVE_ITEM_MBALL_LIVE_ROOT) {"
            " if (ip->kind == nITKindMBall &&"
            " dobj->parent_gobj->id == nGCCommonKindItem) {} } }")
        facts = admission_facts(block)
        self.assertFalse(facts["effect_owner"])
        self.assertTrue(facts["item_owner"])

    def test_executor_roots_ignores_comments(self):
        self.assertEqual(
            executor_roots("/* root_offset == NDS_NATIVE_ITEM_MBALL_LIVE_ROOT"
                           " used to be here */\n"
                           "if (root_offset == NDS_NATIVE_ITEM_MBALL_BAKED_ROOT)"),
            {"NDS_NATIVE_ITEM_MBALL_BAKED_ROOT"})


if __name__ == "__main__":
    sys.exit(main())
