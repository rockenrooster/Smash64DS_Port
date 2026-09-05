#!/usr/bin/env python3
"""Register a generated native stage packet at runtime, in every C and build site.

Landing a stage's packet means the same six edits every time: a gkind define,
the MULTI condition, a new level in the workspace-maxima chain, the packet
row, the registry slots (renderer and adapter), the capture and descriptor
rows, the assets.c include, the Makefile rule and dependencies, and the
build.ps1 generation and output lines. Sector and Hyrule were done by hand
on 2026-09-04 and one of the six needed a checker to catch a slip. This
applies all of them from the descriptor, idempotently: a site that already
carries the stage is left alone, so it can be re-run.

Usage:
    python scripts/stages/register_native_stage_runtime.py --stage zebes
    python scripts/stages/register_native_stage_runtime.py --stage zebes --dry-run

Afterwards run emit_native_stage_runtime_rows.py --stage zebes --check.
The rigid and camera binding masks are written as 0ULL with a comment; derive
them from the DObj table's transform flags and the map's joint animation
before acceptance (0 only costs the replay optimisation).
"""
import argparse
import io
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
from native_stage_descriptors import get_descriptor  # noqa: E402
import emit_native_stage_runtime_rows as emitter  # noqa: E402

SELECT = os.path.join(ROOT, "src", "nds", "nds_native_stage_select.inc")
MATRIX = os.path.join(ROOT, "src", "port", "renderer_adapter_matrix.c")
ASSETS = os.path.join(ROOT, "src", "nds", "nds_renderer_assets.c")
MAKEFILE = os.path.join(ROOT, "Makefile")
BUILDPS1 = os.path.join(ROOT, "build.ps1")


def load(p):
    return io.open(p, encoding="utf-8", newline="").read()


def save(p, s, dry):
    if dry:
        print(f"  (dry run) would write {os.path.relpath(p, ROOT)}")
        return
    io.open(p, "w", encoding="utf-8", newline="").write(s)
    print(f"  wrote {os.path.relpath(p, ROOT)}")


def nl_of(s):
    return "\r\n" if s.count("\r\n") > s.count("\n") // 2 else "\n"


def guard(mac):
    return f"#if defined(NDS_P2_STAGE_{mac}) && (NDS_P2_STAGE_{mac} == 1)"


def linked_guard(mac):
    """select.inc/assets.c guard: linked tables exist only for LINKED stages.

    The adapter capture tables in renderer_adapter_matrix.c keep the plain
    guard() -- they are not packet tables and must not vanish on a blob
    stage. Blob residency gates only the .inc include, the packet row, the
    registry slot and the workspace-maxima level.
    """
    return (f"#if defined(NDS_P2_STAGE_{mac}) && (NDS_P2_STAGE_{mac} == 1) "
            f"&& (NDS_NATIVE_STAGE_LINKED_{mac} == 1)")


def register_select(stage, desc, dry):
    s = load(SELECT)
    name, MAC = emitter.cname(stage), stage.upper()
    gk = emitter.GKIND[stage]
    changed = False
    # 0. LINKED default: a new stage is blob-resident unless the build links
    # it (-DNDS_NATIVE_STAGE_LINKED_<MAC>=1), like every other blob stage.
    linkdef = (f"#ifndef NDS_NATIVE_STAGE_LINKED_{MAC}\n"
               f"#define NDS_NATIVE_STAGE_LINKED_{MAC} 0\n#endif\n")
    if f"NDS_NATIVE_STAGE_LINKED_{MAC}" not in s:
        anchor_inc = "#include <nds/nds_native_stage_blob.h>\n"
        assert anchor_inc in s, "blob.h include anchor"
        s = s.replace(anchor_inc, anchor_inc + linkdef, 1)
        changed = True
    # 1. gkind define
    if f"#define NDS_NATIVE_STAGE_GKIND_{MAC} " not in s:
        s = s.replace("#define NDS_NATIVE_STAGE_GKIND_PUPUPU 6u\n",
                      f"#define NDS_NATIVE_STAGE_GKIND_{MAC} {gk}u\n#define NDS_NATIVE_STAGE_GKIND_PUPUPU 6u\n", 1)
        changed = True
    # 2. MULTI condition
    if f"(defined(NDS_P2_STAGE_{MAC}) && (NDS_P2_STAGE_{MAC} == 1))" not in s.split("#define NDS_NATIVE_STAGE_MULTI 1")[0]:
        m = re.search(r"(#if \(defined\(NDS_P2_STAGE_\w+\) && \(NDS_P2_STAGE_\w+ == 1\)\)(?: \|\| \\\n    \(defined\(NDS_P2_STAGE_\w+\) && \(NDS_P2_STAGE_\w+ == 1\)\))*)\n#define NDS_NATIVE_STAGE_MULTI 1", s)
        assert m, "MULTI condition"
        s = s.replace(m.group(1), m.group(1) + f" || \\\n    (defined(NDS_P2_STAGE_{MAC}) && (NDS_P2_STAGE_{MAC} == 1))", 1)
        changed = True
    # 3. maxima chain: add a level BASEn -> MAX, renaming the current MAX level
    if f"NDS_NATIVE_STAGE_{MAC}_SEGMENT_COUNT)" not in s.split("};", 1)[0]:
        enum_m = re.search(r"enum\n\{\n(.*?)\n\};\n", s, re.S)
        assert enum_m, "maxima enum"
        body = enum_m.group(1)
        levels = sorted(set(int(x) for x in re.findall(r"NDS_NATIVE_STAGE_BASE(\d+)_SEGMENT_COUNT", body)) or [1])
        prev = f"BASE{levels[-1]}" if re.search(r"NDS_NATIVE_STAGE_BASE\d+_", body) else "BASE"
        newbase = f"BASE{levels[-1] + 1 if re.search(r'NDS_NATIVE_STAGE_BASE\d+_', body) else 2}"
        # the current MAX block becomes newbase
        body2 = body.replace("NDS_NATIVE_STAGE_MAX_", f"NDS_NATIVE_STAGE_{newbase}_")
        # its last enumerator needs a trailing comma in both arms
        body2 = re.sub(r"(_DENSE_VERTEX_COUNT\))\n#else", r"\1,\n#else", body2)
        body2 = re.sub(r"(NDS_NATIVE_STAGE_" + newbase + r"_DENSE_VERTEX_COUNT = NDS_NATIVE_STAGE_\w+_DENSE_VERTEX_COUNT)\n#endif$",
                       r"\1,\n#endif", body2)
        tail = (f"\n{linked_guard(MAC)}\n"
                f"    NDS_NATIVE_STAGE_MAX_SEGMENT_COUNT =\n"
                f"        NDS_NATIVE_STAGE_MAX2(NDS_NATIVE_STAGE_{newbase}_SEGMENT_COUNT,\n"
                f"                              NDS_NATIVE_STAGE_{MAC}_SEGMENT_COUNT),\n"
                f"    NDS_NATIVE_STAGE_MAX_RUN_COUNT =\n"
                f"        NDS_NATIVE_STAGE_MAX2(NDS_NATIVE_STAGE_{newbase}_RUN_COUNT,\n"
                f"                              NDS_NATIVE_STAGE_{MAC}_RUN_COUNT),\n"
                f"    NDS_NATIVE_STAGE_MAX_DENSE_VERTEX_COUNT =\n"
                f"        NDS_NATIVE_STAGE_MAX2(NDS_NATIVE_STAGE_{newbase}_DENSE_VERTEX_COUNT,\n"
                f"                              NDS_NATIVE_STAGE_{MAC}_DENSE_VERTEX_COUNT)\n"
                f"#else\n"
                f"    NDS_NATIVE_STAGE_MAX_SEGMENT_COUNT = NDS_NATIVE_STAGE_{newbase}_SEGMENT_COUNT,\n"
                f"    NDS_NATIVE_STAGE_MAX_RUN_COUNT = NDS_NATIVE_STAGE_{newbase}_RUN_COUNT,\n"
                f"    NDS_NATIVE_STAGE_MAX_DENSE_VERTEX_COUNT = NDS_NATIVE_STAGE_{newbase}_DENSE_VERTEX_COUNT\n"
                f"#endif")
        s = s.replace(enum_m.group(0), "enum\n{\n" + body2 + tail + "\n};\n", 1)
        changed = True
    # 4. packet row (after the last existing packet row's #endif)
    if f"sNdsNativeStagePacket{name} = {{" not in s:
        text, _rows = emitter.emit(desc, stage)
        row = text.split("/* ---- nds_native_stage_select.inc: packet row")[1]
        row = row.split("\n", 1)[1]                     # drop the banner line
        row = row.split("\n/* registry:")[0].rstrip() + "\n"
        anchor = "static const NDSNativeStagePacket *const\n    sNdsNativeStagePacketTable["
        assert anchor in s, "packet table anchor"
        s = s.replace(anchor, row + "\n" + anchor, 1)
        changed = True
    # 5. registry slot (and a ninth entry for a kind past the VS starters)
    if f"&sNdsNativeStagePacket{name}," not in s:
        tm = re.search(r"(sNdsNativeStagePacketTable\[NDS_NATIVE_STAGE_GKIND_COUNT\] = \{\n)(.*?)(\n    \};)", s, re.S)
        assert tm, "packet table"
        s = s.replace(tm.group(0), tm.group(1) + replace_slot(tm.group(2), gk, f"&sNdsNativeStagePacket{name},", MAC, linked_guard) + tm.group(3), 1)
        s = bump_count(s, "NDS_NATIVE_STAGE_GKIND_COUNT", gk + 1)
        changed = True
    if changed:
        save(SELECT, s, dry)
    else:
        print("  select.inc: already registered")


def replace_slot(table_body, gk, entry, MAC, guard_fn=None):
    """The registry lists eight slots; some are `#if ... &x, #else NULL, #endif`
    blocks and some bare `NULL,`. Replace slot `gk` with a guarded entry."""
    lines = table_body.split("\n")
    slots = []          # (start_line, end_line) per slot
    i = 0
    while i < len(lines):
        if lines[i].strip().startswith("#if "):
            j = i
            while not lines[j].strip().startswith("#endif"):
                j += 1
            slots.append((i, j))
            i = j + 1
        elif lines[i].strip():
            slots.append((i, i))
            i += 1
        else:
            i += 1
    assert len(slots) >= 8, f"expected at least 8 registry slots, saw {len(slots)}"
    guard_fn = guard_fn or guard
    new_slot = [f"{guard_fn(MAC)}", f"        {entry}", "#else", "        NULL,", "#endif"]
    if gk >= len(slots):
        # A kind past the VS starters (Mushroom Kingdom is nGRKindUnlockStart, 8;
        # the 1P arenas run 9..16 with holes at PupupuNew 10, Explain 11 and
        # Bonus3 15 until those land): pad every missing index with a bare
        # NULL, so the table stays gkind-indexed, then append the slot; the
        # caller bumps the table's COUNT define to match. The previous last
        # slot was written without a trailing comma.
        a, b = slots[-1]
        for k in range(a, b + 1):
            body = lines[k].strip()
            if body and not body.startswith("#") and not body.endswith(","):
                lines[k] = lines[k] + ","
        for _hole in range(len(slots), gk):
            lines.append("        NULL,")
        lines.extend(new_slot)
        return "\n".join(lines)
    assert gk < len(slots), f"gkind {gk} is beyond the table ({len(slots)} slots)"
    a, b = slots[gk]
    assert lines[a].strip() == "NULL,", f"slot {gk} is not free: {lines[a].strip()}"
    lines[a:b + 1] = new_slot
    return "\n".join(lines)


def bump_count(s, macro, needed):
    """Raise `#define <macro> Nu` to `needed` when it is smaller."""
    m = re.search(rf"#define {macro} (\d+)u", s)
    assert m, macro
    if int(m.group(1)) < needed:
        s = s.replace(m.group(0), f"#define {macro} {needed}u", 1)
    return s


def register_matrix(stage, desc, dry):
    s = load(MATRIX)
    name, MAC = emitter.cname(stage), stage.upper()
    gk = emitter.GKIND[stage]
    changed = False
    if f"sNdsRendererAdapterNativeStageCapture{name}[" not in s:
        text, _rows = emitter.emit(desc, stage)
        block = text.split("/* ---- renderer_adapter_matrix.c:")[1].split("\n", 1)[1]
        block = block.split("/* ---- nds_native_stage_select.inc")[0].rstrip() + "\n"
        # The count define is the anchor; its value moves as kinds are added
        # (8u until Inishie, 9u after), so match it by name, not by value.
        am = re.search(r"#define NDS_RENDERER_ADAPTER_NATIVE_STAGE_KIND_COUNT \d+u\n", s)
        assert am, "NDS_RENDERER_ADAPTER_NATIVE_STAGE_KIND_COUNT define"
        s = s.replace(am.group(0), block + am.group(0), 1)
        changed = True
    if f"&sNdsRendererAdapterNativeStage{name}," not in s:
        tm = re.search(r"(sNdsRendererAdapterNativeStageTable\[\n\s*NDS_RENDERER_ADAPTER_NATIVE_STAGE_KIND_COUNT\] = \{\n)(.*?)(\n    \};)", s, re.S)
        assert tm, "adapter table"
        s = s.replace(tm.group(0), tm.group(1) + replace_slot(tm.group(2), gk, f"&sNdsRendererAdapterNativeStage{name},", MAC) + tm.group(3), 1)
        s = bump_count(s, "NDS_RENDERER_ADAPTER_NATIVE_STAGE_KIND_COUNT", gk + 1)
        changed = True
    if changed:
        save(MATRIX, s, dry)
    else:
        print("  renderer_adapter_matrix.c: already registered")


def register_assets(stage, dry):
    s = load(ASSETS)
    MAC = stage.upper()
    inc = f"nds_native_stage_{stage}.generated.inc"
    if inc in s:
        print("  nds_renderer_assets.c: already included")
        return
    # Insert ahead of the blob maxima block when it exists so every
    # per-stage include stays contiguous; the select.inc anchor is the
    # fallback the pre-blob file offered.
    anchor = "/* Generated blob maxima (scripts/stages/emit_native_stage_runtime_rows.py"
    if anchor not in s:
        anchor = "/* Must follow every generated packet: it names their tables. */"
    assert anchor in s
    linkdef = (f"#ifndef NDS_NATIVE_STAGE_LINKED_{MAC}\n"
               f"#define NDS_NATIVE_STAGE_LINKED_{MAC} 0\n#endif\n")
    s = s.replace(anchor, f"{linkdef}{linked_guard(MAC)}\n#include \"{inc}\"\n#endif\n{anchor}", 1)
    save(ASSETS, s, dry)


def register_makefile(stage, dry):
    s = load(MAKEFILE)
    nl = nl_of(s)
    MAC = stage.upper()
    var = f"NDS_NATIVE_STAGE_{MAC}_INC"
    if var in s:
        print("  Makefile: already registered")
        return
    # The blob maxima header (NDS_NATIVE_STAGE_BLOB_MAXIMA_INC) is defined
    # after the per-stage includes and also ends in _INC; skip it.
    last_var = re.findall(r"^(NDS_NATIVE_STAGE_(?!BLOB_)\w+_INC) := .*$", s, re.M)[-1]
    last_line = re.search(rf"^{last_var} := .*$", s, re.M).group(0)
    s = s.replace(last_line + nl, last_line + nl +
                  f"{var} := $(PROJECT_ROOT)/src/nds/nds_native_stage_{stage}.generated.inc{nl}", 1)
    rule_anchor = re.search(rf"\t@touch \$\({last_var}\){re.escape(nl)}", s)
    assert rule_anchor, "last inc rule"
    s = s.replace(rule_anchor.group(0), rule_anchor.group(0) +
                  f"$({var}): $(NDS_NATIVE_STAGE_GENERATOR_PREREQ){nl}"
                  f"\tpython \"$(PROJECT_ROOT)/scripts/stages/generate_nds_native_stage.py\" --repo-root \"$(PROJECT_ROOT)\" --stage {stage}{nl}"
                  f"\t@touch $({var}){nl}", 1)
    dep = re.search(r"^nds_renderer_assets\.o: .*$", s, re.M)
    assert dep
    s = s.replace(dep.group(0), dep.group(0) + f" $({var})", 1)
    elf = re.search(rf"\t\$\({last_var}\) \\{re.escape(nl)}|\t\$\(NDS_NATIVE_STAGE_\w+_INC\) \$\({last_var}\) \\{re.escape(nl)}", s)
    assert elf, "elf prereq line"
    s = s.replace(elf.group(0), elf.group(0) + f"\t$({var}) \\{nl}", 1)
    save(MAKEFILE, s, dry)


def register_buildps1(stage, dry):
    s = load(BUILDPS1)
    nl = nl_of(s)
    if f"'--stage', '{stage}'" in s:
        print("  build.ps1: already registered")
        return
    calls = list(re.finditer(r"    Invoke-Python \$python 'generate-native-stage-(\w+)' `\r?\n.*?\r?\n.*?\) \$RepoRoot\r?\n", s, re.S))
    assert calls, "build.ps1 generation calls"
    last = calls[-1].group(0)
    s = s.replace(last, last +
                  f"    Invoke-Python $python 'generate-native-stage-{stage}' `{nl}"
                  f"        @((Join-Path $RepoRoot 'scripts\\stages\\generate_nds_native_stage.py'),{nl}"
                  f"          '--repo-root', $RepoRoot, '--stage', '{stage}') $RepoRoot{nl}", 1)
    outs = re.findall(r"^        'src\\nds\\nds_native_stage_\w+\.generated\.inc',$", s, re.M)
    assert outs
    s = s.replace(outs[-1] + nl, outs[-1] + nl + f"        'src\\nds\\nds_native_stage_{stage}.generated.inc',{nl}", 1)
    save(BUILDPS1, s, dry)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--stage", required=True)
    ap.add_argument("--dry-run", action="store_true")
    args = ap.parse_args()
    desc = get_descriptor(args.stage)
    counts = emitter.read_generated_counts(desc, args.stage)
    if counts is None or counts["SEGMENT"] is None:
        raise SystemExit(f"generate the packet first: src/nds/nds_native_stage_{args.stage}.generated.inc")
    print(f"registering {args.stage} (gkind {emitter.GKIND[args.stage]}), packet counts {counts}")
    register_select(args.stage, desc, args.dry_run)
    register_matrix(args.stage, desc, args.dry_run)
    register_assets(args.stage, args.dry_run)
    register_makefile(args.stage, args.dry_run)
    register_buildps1(args.stage, args.dry_run)
    print("now run: python scripts/stages/emit_native_stage_runtime_rows.py --stage", args.stage, "--check")
    return 0


if __name__ == "__main__":
    sys.exit(main())
