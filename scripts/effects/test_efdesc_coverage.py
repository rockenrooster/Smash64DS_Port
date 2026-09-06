#!/usr/bin/env python3
"""Linked-EFDesc resolver coverage for the frame-365 grapple crash.

Bounded recurrence check, not an effect audit. GDB capture
builds/resume-20260905/runtime-reserve/bad-descriptor.txt shows
efManagerMakeEffect(dEFManagerSamusGrappleBeamEffectDesc) handing
gcSetupCustomDObjs 0x044a99c4 at frame 365: the roster resolver list omitted
the grapple descriptor, so its &ll... fields still held link-time RAM
addresses where efManagerMakeEffect does raw `base + offset` arithmetic.

A plain grep over decomp sources cannot answer this, for two reasons this
checker does not skip:

1. The resolver lists in src/import/battleship_efmanager.c are sliced by
   `#if NDS_P2_*` guards. Text presence of X(dEFManager...) means nothing;
   only macros active under the build's generated config count. The checker
   evaluates those guards against builds/build-p2-fourcpu-tickhud's
   nds_build_config.h.
2. Decomp holds 50+ EFDesc globals but the link keeps a subset. Only linked
   descriptors can crash this ROM, so the checker takes its descriptor set
   from `arm-none-eabi-nm` on the linked ELF and reads each linked
   descriptor's four offset words back out of the ELF with objdump, proving
   which fields are address-valued (need the resolver) and which are
   legitimate zero-offset / particle-only cases (must stay exempt).

Exit status is 0 when every linked descriptor that needs resolving and is
reachable under the active config is visited by the resolver, 1 when a
reachable gap remains (including a removed Samus grapple row), 2 on any
tooling or parse failure. Unreachable-but-linked descriptors (fighter file
not in this roster) are reported as LATENT, never as READY and never as a
fail. Patch suggestions go to stdout / --report; this checker edits nothing.
"""
import argparse
import binascii
import os
import re
import subprocess
import sys
import unittest

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
DEFAULT_ELF = os.path.join(
    ROOT, "builds", "build-p2-fourcpu-tickhud",
    "smash64ds-p2-fourcpu-tickhud-hwtri.elf")
DEFAULT_CONFIG = os.path.join(
    ROOT, "builds", "build-p2-fourcpu-tickhud", "nds_build_config.h")
DEFAULT_EFMGR = os.path.join(ROOT, "src", "import", "battleship_efmanager.c")
DEFAULT_DECOMP = os.path.join(
    ROOT, "decomp", "BattleShip-main", "decomp", "src", "ef", "efmanager.c")

MAX_READ_BYTES = 8 * 1024 * 1024
# Same tripwire as ndsEFManagerResolveOffset: file offsets are small, link
# addresses are not. Zero is meaningful and passes through.
RAM_THRESHOLD = 0x01000000
# EFDesc words holding the four offset fields. Layout: flags/dl word,
# file_head ptr, two packed 3-byte transform-kind words, two proc ptrs, then
# o_dobjsetup, o_mobjsub, o_anim_joint, o_matanim_joint.
EF_OFF_DOBJSETUP = 0x18
EF_OFF_MOBJSUB = 0x1C
EF_OFF_ANIMJOINT = 0x20
EF_OFF_MATANIMJOINT = 0x24
EF_DESC_SIZE = 0x28

NM_CANDIDATES = [
    os.path.join("C:", os.sep, "devkitPro", "devkitARM", "bin",
                 "arm-none-eabi-nm.exe"),
    "arm-none-eabi-nm",
]
OBJDUMP_CANDIDATES = [
    os.path.join("C:", os.sep, "devkitPro", "devkitARM", "bin",
                 "arm-none-eabi-objdump.exe"),
    "arm-none-eabi-objdump",
]

DESC_NAME_RE = re.compile(r"dEFManager\w+EffectDesc")
X_CALL_RE = re.compile(r"\bX\(\s*(dEFManager\w+EffectDesc)\s*\)")
LL_RE = re.compile(r"&ll[A-Za-z0-9_]+")
OFFSET_LINE_RE = re.compile(r"^\s*(0x0|&ll[A-Za-z0-9_]+)\s*,?\s*(?://.*)?$",
                              re.M)
FILE_HEAD_RE = re.compile(r"^\s*(&g[A-Za-z0-9_]+(?:\[[^\]]*\])?)\s*,", re.M)

# Concrete maker/file scope, read off decomp (cited file:line in the report).
# kinds "common" means the descriptor's file is always resident and its maker
# is reachable through shared paths (ftparam motion-kind dispatch); otherwise
# the integer fighter kinds (ftdef.h FTKind: Mario 0, Fox 1, Donkey 2,
# Samus 3, Luigi 4, Link 5, Yoshi 6, Captain 7, Kirby 8, Pikachu 9,
# Purin 10, Ness 11) that can reach the maker.
MAKER_SCOPE = {
    "dEFManagerSamusGrappleBeamEffectDesc": {
        "maker": "efManagerSamusGrappleBeamGlowMakeEffect (efmanager.c)",
        "callers": ["ftcommoncatch1.c:98 (Samus-gated)",
                    "ftcommonthrow.c:88 (Samus-gated)"],
        "file": "&gFTDataSamusSpecial2 (SamusSpecial2)",
        "kinds": [3],
    },
    "dEFManagerStarRodSparkEffectDesc": {
        "maker": "efManagerStarRodSparkMakeEffect (efmanager.c:3386)",
        "callers": ["ftparam.c:1974-1975 (motion-kind dispatch, all fighters)",
                    "itstarrod.c:299 (Star Rod item)",
                    "efmanager.c:5867/5872 (batter)",
                    "relocData/235_CaptainMainMotion.c:1640 "
                    "(Captain script emits nEFKindStarRodSpark)"],
        "file": "&gEFManagerFiles[0] (EFCommonEffects1, always resident)",
        "kinds": "common",
    },
    "dEFManagerVulcanJabEffectDesc": {
        "maker": "efManagerKirbyVulcanJabMakeEffect (efmanager.c:4575)",
        "callers": ["ftcommonattack100.c:99, gated fkind==Kirby "
                    "(ftcommonattack100.c:87)"],
        "file": "&gFTDataKirbySpecial2 (KirbySpecial2)",
        "kinds": [8],
    },
    "dEFManagerPikachuThunderShockEffectDesc": {
        "maker": "efManagerPikachuThunderShockMakeEffect (efmanager.c:4409)",
        "callers": ["ftcommonattacks4.c:49, gated "
                    "nFTKindPikachu/NPikachu (ftcommonattacks4.c:19-20)"],
        "file": "&gFTDataPikachuSpecial2 (PikachuSpecial2)",
        "kinds": [9],
    },
    "dEFManagerYoshiShieldEffectDesc": {
        "maker": "efManagerYoshiShieldMakeEffect (efmanager.c:4172)",
        "callers": ["ftcommonguard1.c:389 (fkind==Yoshi)",
                    "ftcommonguard2.c:21 (fkind==Yoshi)"],
        "file": "&gFTDataYoshiModel (YoshiModel)",
        "kinds": [6],
    },
}

GRAPPLE = "dEFManagerSamusGrappleBeamEffectDesc"


class CoverageError(Exception):
    pass


def read_bounded(path):
    with open(path, "rb") as handle:
        data = handle.read(MAX_READ_BYTES + 1)
    if len(data) > MAX_READ_BYTES:
        raise CoverageError("input too large: %s" % path)
    return data.decode("utf-8", errors="replace")


def run_tool(candidates, args):
    last_error = None
    for candidate in candidates:
        try:
            completed = subprocess.run(
                [candidate] + args, capture_output=True, text=True,
                timeout=120)
        except (OSError, subprocess.SubprocessError) as exc:
            last_error = exc
            continue
        if completed.returncode != 0:
            raise CoverageError(
                "%s failed: %s" % (candidate, completed.stderr.strip()))
        return completed.stdout
    raise CoverageError("no usable tool among %s: %s"
                        % (candidates, last_error))


def parse_config(text):
    """NDS_* integer defines from the generated build config."""
    config = {}
    for match in re.finditer(
            r"^\s*#define\s+(NDS_[A-Za-z0-9_]+)\s+"
            r"(0[xX][0-9a-fA-F]+|\d+)[uUlL]*\s*$", text, re.M):
        config[match.group(1)] = int(match.group(2), 0)
    return config


def eval_guard(expr, config, unknowns):
    work = re.sub(r"defined\s*\(\s*(\w+)\s*\)",
                  lambda m: "1" if m.group(1) in config else "0", expr)
    work = re.sub(r"defined\s+(\w+)",
                  lambda m: "1" if m.group(1) in config else "0", work)

    def replace_token(match):
        name = match.group(0)
        if re.fullmatch(r"\d+", name):
            return name
        if name in config:
            return str(config[name])
        unknowns.add(name)
        return "0"

    work = re.sub(r"[A-Za-z_]\w*", replace_token, work)
    work = work.replace("!=", " NEQ ").replace("&&", " and ")
    work = work.replace("||", " or ")
    work = re.sub(r"!(?!=)", " not ", work).replace(" NEQ ", " != ")
    return bool(eval(work, {"__builtins__": {}}, {}))  # noqa: S307


def strip_block_comments(text):
    # C replaces each comment with one space (interior newlines vanish),
    # so the spliced ItemGetSwirl row stays on the define's logical line.
    return re.sub(r"/\*.*?\*/", " ", text, flags=re.S)


def splice_lines(text):
    """C phase 2: join every line ending in backslash-newline.

    This runs BEFORE comment stripping, so the backslash after the
    ItemGetSwirl note's `*/` still continues the NDS_EF_MANAGER_DESCS
    define even though the note's own first lines endbare. A grep-style
    line scan that ends the block at the first non-backslash line
    would drop the six rows below that note.
    """
    logical = []
    pending = ""
    for line in text.splitlines():
        stripped = line.rstrip()
        if stripped.endswith("\\"):
            pending += stripped[:-1]
        else:
            logical.append(pending + line)
            pending = ""
    if pending:
        logical.append(pending)
    return logical


def parse_resolver_coverage(text, config):
    """Descs visited by the resolver under the active config.

    Collects X(name) from the NDS_EF_MANAGER_DESCS and
    NDS_EF_ROSTER_DESCS_* define blocks, honouring the #if NDS_P2_*
    guards around them. A static grep would count inactive rows; this
    does not. Preprocessing runs in C phase order (splice, then strip
    comments), so a block comment inside a define cannot fake its end.
    """
    cleaned = strip_block_comments("\n".join(splice_lines(text)))
    covered = set()
    unknowns = set()
    cond_stack = []

    def active():
        return all(cond_stack)

    for lineno, raw in enumerate(cleaned.splitlines(), 1):
        code = raw.split("//")[0].strip()
        directive = re.match(r"#\s*(if|ifdef|ifndef|elif|else|endif)\b(.*)$",
                             code)
        if directive:
            kind, rest = directive.group(1), directive.group(2).strip()
            if kind == "if":
                cond_stack.append(eval_guard(rest, config, unknowns))
            elif kind == "ifdef":
                cond_stack.append(rest.split()[0] in config)
            elif kind == "ifndef":
                cond_stack.append(rest.split()[0] not in config)
            elif kind == "elif":
                if not cond_stack:
                    raise CoverageError("stray #elif at line %d" % lineno)
                cond_stack[-1] = eval_guard(rest, config, unknowns)
            elif kind == "else":
                if not cond_stack:
                    raise CoverageError("stray #else at line %d" % lineno)
                cond_stack[-1] = not cond_stack[-1]
            elif kind == "endif":
                if not cond_stack:
                    raise CoverageError("stray #endif at line %d" % lineno)
                cond_stack.pop()
            continue
        define = re.match(r"#define\s+(NDS_EF_\w*DESCS\w*)\(X\)", code)
        if define and active():
            covered.update(X_CALL_RE.findall(code))

    if unknowns:
        region_start = cleaned.find("NDS_EF_MANAGER_DESCS")
        region_end = cleaned.rfind("NDS_EF_DESC_COUNT_ONE")
        region = cleaned[region_start:region_end]
        affecting = sorted(name for name in unknowns if name in region)
        if affecting:
            raise CoverageError(
                "unresolved guard macros inside resolver lists: %s"
                % ", ".join(affecting))
    return covered


def linked_descs(nm_output):
    """Actually-linked EFDesc symbols from nm --defined-only -S output."""
    found = {}
    for line in nm_output.splitlines():
        parts = line.split()
        if len(parts) != 4:
            continue
        addr, _size, _type, name = parts
        if DESC_NAME_RE.fullmatch(name):
            found[name] = int(addr, 16)
    if not found:
        raise CoverageError("nm returned no dEFManager*EffectDesc symbols")
    return found


def parse_desc_body(decomp_text, name):
    match = re.search(
        r"EFDesc\s+%s\s*(?:\[[^\]]*\])?\s*=\s*\{(.*?)\};"
        % re.escape(name), decomp_text, re.S)
    if not match:
        raise CoverageError("decomp initializer not found: %s" % name)
    return match.group(1)


def classify_desc(body):
    """needs_resolve (&ll present) vs particle-only (all-zero offsets).

    offset_kinds holds the four offset fields in struct order as
    'addr' (&ll-sourced, link-time RAM address in the ELF) or 'zero'
    (0x0-sourced). Fewer than four offset lines means the body shape
    drifted and the caller must fail closed.
    """
    ll_symbols = LL_RE.findall(body)
    offset_lines = OFFSET_LINE_RE.findall(body)
    tail = [line for line in offset_lines[-4:]]
    if len(tail) != 4:
        raise CoverageError(
            "expected four offset fields, found %d" % len(tail))
    kinds = ["addr" if field.startswith("&") else "zero" for field in tail]
    particle_only = (not ll_symbols and all(kind == "zero" for kind in kinds))
    file_head = None
    head_match = FILE_HEAD_RE.search(body)
    if head_match:
        file_head = head_match.group(1)
    return {"needs_resolve": bool(ll_symbols),
            "particle_only": particle_only,
            "offset_kinds": kinds,
            "ll_symbols": ll_symbols,
            "file_head": file_head}


def verify_elf_fields(offset_kinds, words):
    """Cross-check decomp offset shape against linked ELF words.

    Proves the +0x18..+0x24 field offsets used to read the ELF are the
    real ones: every &ll-sourced field must read as a link-time address
    and every 0x0-sourced field as zero. Returns mismatch descriptions.
    """
    fields = ("o_dobjsetup", "o_mobjsub", "o_anim_joint", "o_matanim_joint")
    mismatches = []
    for field, kind, word in zip(fields, offset_kinds, words):
        if kind == "addr" and word < RAM_THRESHOLD:
            mismatches.append("%s: &ll-sourced but ELF holds 0x%08x"
                              % (field, word))
        if kind == "zero" and word != 0:
            mismatches.append("%s: 0x0-sourced but ELF holds 0x%08x"
                              % (field, word))
    return mismatches


def parse_objdump_words(objdump_output):
    rows = []
    for line in objdump_output.splitlines():
        match = re.match(r"^\s*([0-9a-fA-F]+)\s+((?:[0-9a-fA-F]{8}\s*)+)",
                         line)
        if not match:
            continue
        base = int(match.group(1), 16)
        raw = binascii.unhexlify("".join(match.group(2).split()))
        rows.append((base, raw))
    rows.sort()
    if not rows:
        raise CoverageError("objdump returned no .main.rw words")

    def read_u32(addr):
        for base, raw in rows:
            if base <= addr < base + len(raw):
                off = addr - base
                return int.from_bytes(raw[off:off + 4], "little")
        raise CoverageError("ELF word outside dumped range: 0x%08x" % addr)

    return read_u32


def read_elf_offsets(read_u32, addr):
    return [read_u32(addr + off) for off in (
        EF_OFF_DOBJSETUP, EF_OFF_MOBJSUB,
        EF_OFF_ANIMJOINT, EF_OFF_MATANIMJOINT)]


def present_kinds(config):
    kinds = []
    for index in range(4):
        key = "NDS_P2_FOUR_CPU_KIND%d" % index
        if key in config:
            kinds.append(config[key])
    return kinds


def evaluate(linked, bodies, covered, elf_words, kinds_present):
    """Split linked descs into covered gaps, latent notes, and clean rows."""
    gaps, latent, clean, exempt = [], [], [], []
    for name in sorted(linked):
        info = classify_desc(bodies[name])
        if info["particle_only"]:
            exempt.append(name)
            continue
        if not info["needs_resolve"]:
            clean.append(name)
            continue
        scope = MAKER_SCOPE.get(name, {})
        kinds = scope.get("kinds")
        if kinds == "common":
            reachable = True
        elif isinstance(kinds, list):
            reachable = any(kind in kinds_present for kind in kinds)
        else:
            reachable = None
        words = elf_words.get(name, [])
        ram_fields = sum(word >= RAM_THRESHOLD for word in words)
        row = {"name": name, "scope": scope, "ram_fields": ram_fields,
               "covered": name in covered, "reachable": reachable}
        if name in covered:
            clean.append(name)
        elif reachable:
            gaps.append(row)
        elif reachable is None:
            row["note"] = "unclassified maker scope; failing closed"
            gaps.append(row)
        else:
            latent.append(row)
    return gaps, latent, clean, exempt


def check_grapple_invariant(covered):
    return GRAPPLE in covered


def format_report(linked, covered, gaps, latent, clean, exempt, bodies,
                  elf_words, config):
    out = []
    out.append("linked EFDesc symbols in ELF: %d" % len(linked))
    out.append("resolver-covered under active config: %d" % len(covered))
    out.append("four-CPU kinds present: %s"
               % present_kinds(config))
    out.append("")
    if GRAPPLE in covered:
        out.append("GRAPPLE ROW PRESENT: %s is resolver-covered" % GRAPPLE)
    else:
        out.append("GRAPPLE ROW MISSING: %s is NOT resolver-covered"
                   % GRAPPLE)
    out.append("")
    for row in gaps:
        scope = row["scope"]
        out.append("GAP %s ram_fields=%d/4 covered=%s reachable=%s"
                   % (row["name"], row["ram_fields"], row["covered"],
                      row["reachable"]))
        out.append("  maker: %s" % scope.get("maker", "unclassified"))
        for caller in scope.get("callers", []):
            out.append("  caller: %s" % caller)
        out.append("  file: %s" % scope.get("file", "unknown"))
        if row.get("note"):
            out.append("  note: %s" % row["note"])
    for row in latent:
        scope = row["scope"]
        out.append("LATENT %s ram_fields=%d/4 (fighter absent this config)"
                   % (row["name"], row["ram_fields"]))
        out.append("  maker: %s" % scope.get("maker", "unclassified"))
        out.append("  file: %s" % scope.get("file", "unknown"))
    out.append("")
    out.append("clean: %d (%s)" % (len(clean), ", ".join(clean)))
    out.append("particle-only exempt: %d (%s)"
               % (len(exempt), ", ".join(exempt)))
    void = [name for name in sorted(linked)
            if classify_desc(bodies[name])["needs_resolve"]]
    out.append("linked descs needing resolution: %d" % len(void))
    return "\n".join(out) + "\n"


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--elf", default=DEFAULT_ELF)
    parser.add_argument("--config", default=DEFAULT_CONFIG)
    parser.add_argument("--efmgr", default=DEFAULT_EFMGR)
    parser.add_argument("--decomp", default=DEFAULT_DECOMP)
    parser.add_argument("--report", default=None)
    parser.add_argument("--selftest", action="store_true")
    args = parser.parse_args(argv)
    if args.selftest:
        suite = unittest.defaultTestLoader.loadTestsFromName(__name__)
        result = unittest.TextTestRunner(verbosity=2).run(suite)
        return 0 if result.wasSuccessful() else 1
    try:
        config = parse_config(read_bounded(args.config))
        efmgr_text = read_bounded(args.efmgr)
        decomp_text = read_bounded(args.decomp)
        covered = parse_resolver_coverage(efmgr_text, config)
        nm_output = run_tool(NM_CANDIDATES,
                             ["--defined-only", "-S", args.elf])
        linked = linked_descs(nm_output)
        objdump_output = run_tool(
            OBJDUMP_CANDIDATES,
            ["-s", "-j", ".main.rw", args.elf])
        read_u32 = parse_objdump_words(objdump_output)
        bodies = {name: parse_desc_body(decomp_text, name)
                  for name in linked}
        elf_words = {name: read_elf_offsets(read_u32, addr)
                     for name, addr in linked.items()}
        for name in sorted(linked):
            info = classify_desc(bodies[name])
            mismatches = verify_elf_fields(info["offset_kinds"],
                                           elf_words[name])
            if mismatches:
                raise CoverageError(
                    "%s: ELF words disagree with decomp offset shape "
                    "(stale ELF or wrong field offsets?): %s"
                    % (name, "; ".join(mismatches)))
        gaps, latent, clean, exempt = evaluate(
            linked, bodies, covered, elf_words, present_kinds(config))
        report = format_report(linked, covered, gaps, latent, clean,
                               exempt, bodies, elf_words, config)
        sys.stdout.write(report)
        if args.report:
            with open(args.report, "w", encoding="utf-8") as handle:
                handle.write(report)
        if not check_grapple_invariant(covered):
            sys.stdout.write(
                "FAIL: Samus grapple row missing from resolver coverage\n")
            return 1
        if gaps:
            sys.stdout.write(
                "FAIL: %d reachable linked descriptor(s) lack coverage: %s\n"
                % (len(gaps), ", ".join(row["name"] for row in gaps)))
            return 1
        sys.stdout.write("PASS: linked EFDesc coverage holds\n")
        return 0
    except CoverageError as exc:
        sys.stdout.write("ERROR: %s\n" % exc)
        return 2


class ResolverCoverageTests(unittest.TestCase):
    def test_grapple_row_removed_fails(self):
        source = (
            "#define NDS_EF_MANAGER_DESCS(X) \\\n"
            "    X(dEFManagerShieldEffectDesc)\n"
            "#if NDS_P2_SAMUS\n"
            "#define NDS_EF_ROSTER_DESCS_SAMUS(X) \\\n"
            "    X(dEFManagerSamusEntryPointEffectDesc)\n"
            "#else\n"
            "#define NDS_EF_ROSTER_DESCS_SAMUS(X)\n"
            "#endif\n")
        covered = parse_resolver_coverage(source, {"NDS_P2_SAMUS": 1})
        self.assertNotIn(GRAPPLE, covered)
        self.assertFalse(check_grapple_invariant(covered))

    def test_grapple_row_present_passes_invariant(self):
        source = (
            "#if NDS_P2_SAMUS\n"
            "#define NDS_EF_ROSTER_DESCS_SAMUS(X) \\\n"
            "    X(dEFManagerSamusEntryPointEffectDesc) \\\n"
            "    X(dEFManagerSamusGrappleBeamEffectDesc)\n"
            "#else\n"
            "#define NDS_EF_ROSTER_DESCS_SAMUS(X)\n"
            "#endif\n")
        covered = parse_resolver_coverage(source, {"NDS_P2_SAMUS": 1})
        self.assertTrue(check_grapple_invariant(covered))

    def test_inactive_guard_excluded(self):
        source = (
            "#if NDS_P2_KIRBY\n"
            "#define NDS_EF_ROSTER_DESCS_KIRBY(X) \\\n"
            "    X(dEFManagerVulcanJabEffectDesc)\n"
            "#else\n"
            "#define NDS_EF_ROSTER_DESCS_KIRBY(X)\n"
            "#endif\n")
        self.assertNotIn("dEFManagerVulcanJabEffectDesc",
                         parse_resolver_coverage(source, {"NDS_P2_KIRBY": 0}))
        self.assertIn("dEFManagerVulcanJabEffectDesc",
                      parse_resolver_coverage(source, {"NDS_P2_KIRBY": 1}))

    def test_particle_only_exempt(self):
        body = ("EFFECT_FLAG_USERDATA,\n0,\n&gEFManagerFiles[0],\n"
                "{0,0,0},\n{0,0,0},\nproc,\nNULL,\n"
                "0x0,\n0x0,\n0x0,\n0x0\n")
        info = classify_desc(body)
        self.assertFalse(info["needs_resolve"])
        self.assertTrue(info["particle_only"])

    def test_address_valued_fields_need_resolving(self):
        body = ("0x4 | EFFECT_FLAG_USERDATA,\n15,\n&gFTDataSamusSpecial2,\n"
                "{0x4F, 0, 0},\n{0x2E, 0, 0},\nproc,\nproc,\n"
                "&llSamusSpecial2GrappleBeamDObjDesc,\n"
                "&llSamusSpecial2GrappleBeamMObjSub,\n"
                "&llSamusSpecial2GrappleBeamAnimJoint,\n"
                "&llSamusSpecial2GrappleBeamMatAnimJoint\n")
        info = classify_desc(body)
        self.assertTrue(info["needs_resolve"])
        self.assertFalse(info["particle_only"])
        self.assertEqual(info["offset_kinds"],
                         ["addr", "addr", "addr", "addr"])

    def test_block_comment_inside_define_does_not_end_block(self):
        source = (
            "#define NDS_EF_MANAGER_DESCS(X) \\\n"
            "    X(dEFManagerShieldEffectDesc) \\\n"
            "    /* multi-line note\n"
            "       spanning lines */ \\\n"
            "    X(dEFManagerItemGetSwirlEffectDesc)\n")
        covered = parse_resolver_coverage(source, {})
        self.assertIn("dEFManagerShieldEffectDesc", covered)
        self.assertIn("dEFManagerItemGetSwirlEffectDesc", covered)

    def test_elf_words_must_match_decomp_offset_shape(self):
        self.assertEqual(
            verify_elf_fields(["addr", "zero", "addr", "zero"],
                              [0x021561b4, 0, 0x021561ac, 0]), [])
        self.assertEqual(len(verify_elf_fields(
            ["addr", "addr", "addr", "addr"], [0x7c28, 0, 0, 0])), 4)
        self.assertEqual(len(verify_elf_fields(
            ["zero", "zero", "zero", "zero"], [0, 0, 1, 0])), 1)

    def test_unclassified_scope_fails_closed(self):
        gaps, _latent, _clean, _exempt = evaluate(
            {"dEFManagerMysteryEffectDesc": 0x1000},
            {"dEFManagerMysteryEffectDesc": "&llFooDObjDesc,\n"
             "0x0,\n0x0,\n0x0\n"},
            set(), {}, [3])
        self.assertEqual(len(gaps), 1)
        self.assertIn("failing closed", gaps[0]["note"])


if __name__ == "__main__":
    sys.exit(main())
