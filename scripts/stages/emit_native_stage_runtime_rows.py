#!/usr/bin/env python3
"""Emit the runtime C rows a native stage packet needs, from its descriptor.

Registering a stage's packet at runtime means transcribing the same facts
into three hand-written C sites: the adapter's capture table and descriptor
(src/port/renderer_adapter_matrix.c), the packet row and static assert in
src/nds/nds_native_stage_select.inc, and the includes/build lines. Sector and
Hyrule were transcribed by hand on 2026-09-04 and the transcription is where
a typo would go unnoticed until the final build. This prints those rows from
the descriptor and the generated include so the C is copied, not retyped;
run it on a landed stage and diff against the C to catch drift both ways.

Usage:
    python scripts/stages/emit_native_stage_runtime_rows.py --stage zebes
    python scripts/stages/emit_native_stage_runtime_rows.py --stage sector --check

--check compares the emitted adapter rows with what renderer_adapter_matrix.c
holds for that stage and exits non-zero on a difference.
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

GKIND = {"castle": 0, "sector": 1, "jungle": 2, "zebes": 3, "hyrule": 4,
         "yoster": 5, "dreamland": 6, "yamabuki": 7, "inishie": 8,
         # The 1P-only arenas, include/sc/scene.h nGRKind* order after Inishie
         # (decomp gr/grdef.h): PupupuSmall 9, PupupuNew 10, Explain 11,
         # YosterSmall 12, Metal 13, Zako 14, Bonus3 15, Last 16.
         "pupupusmall": 9, "yostersmall": 12, "metal": 13, "zako": 14, "last": 16,
         # The bonus boards (nGRKindBonus1Mario 17 .. Bonus2Ness 40; Race is
         # nGRKindBonus3 15), the same enum order as include/sc/scene.h.
         "bonus3": 15,
         "bonus1_mario": 17, "bonus1_fox": 18, "bonus1_donkey": 19, "bonus1_samus": 20, "bonus1_luigi": 21, "bonus1_link": 22, "bonus1_yoshi": 23, "bonus1_captain": 24, "bonus1_kirby": 25, "bonus1_pikachu": 26, "bonus1_purin": 27, "bonus1_ness": 28,
         "bonus2_mario": 29, "bonus2_fox": 30, "bonus2_donkey": 31, "bonus2_samus": 32, "bonus2_luigi": 33, "bonus2_link": 34, "bonus2_yoshi": 35, "bonus2_captain": 36, "bonus2_kirby": 37, "bonus2_pikachu": 38, "bonus2_purin": 39, "bonus2_ness": 40}
# The C symbol stem per stage; everything else is the capitalised name. The two
# small islands keep the descriptors' own symbol_prefix (YosterSmall,
# PupupuSmall), which capitalize() would flatten.
CNAME = {"dreamland": "DreamLand", "yostersmall": "YosterSmall",
         "pupupusmall": "PupupuSmall", "bonus3": "Bonus3",
         "bonus1_mario": "Bonus1Mario", "bonus1_fox": "Bonus1Fox", "bonus1_donkey": "Bonus1Donkey", "bonus1_samus": "Bonus1Samus", "bonus1_luigi": "Bonus1Luigi", "bonus1_link": "Bonus1Link", "bonus1_yoshi": "Bonus1Yoshi", "bonus1_captain": "Bonus1Captain", "bonus1_kirby": "Bonus1Kirby", "bonus1_pikachu": "Bonus1Pikachu", "bonus1_purin": "Bonus1Purin", "bonus1_ness": "Bonus1Ness",
         "bonus2_mario": "Bonus2Mario", "bonus2_fox": "Bonus2Fox", "bonus2_donkey": "Bonus2Donkey", "bonus2_samus": "Bonus2Samus", "bonus2_luigi": "Bonus2Luigi", "bonus2_link": "Bonus2Link", "bonus2_yoshi": "Bonus2Yoshi", "bonus2_captain": "Bonus2Captain", "bonus2_kirby": "Bonus2Kirby", "bonus2_pikachu": "Bonus2Pikachu", "bonus2_purin": "Bonus2Purin", "bonus2_ness": "Bonus2Ness"}


def cname(stage):
    return CNAME.get(stage, stage.capitalize())
# grdisplay.c:10-43: display layer N is drawn by grDisplayLayerNPriProcDisplay
# (single dv lists) or grDisplayLayerNSecProcDisplay (DObjDLLink arrays).
CALLBACK = re.compile(r"grDisplayLayer(\d)(Pri|Sec)ProcDisplay")


def macro_prefix(desc):
    return "NDS_NATIVE_STAGE_" + (desc.macro_prefix or "")


def symbol_prefix(desc):
    return "sNdsNativeStage" + (desc.symbol_prefix or "")


def read_generated_counts(desc, stage):
    """The five counts the packet row and the static assert read, from the
    generated include (the descriptor's expected_counts pins the same values,
    but the include is what the C compiles against)."""
    name = "nds_native_stage_owner.generated.inc" if stage == "dreamland" else f"nds_native_stage_{stage}.generated.inc"
    path = os.path.join(ROOT, "src", "nds", name)
    if not os.path.isfile(path):
        return None
    text = io.open(path, encoding="utf-8").read()
    out = {}
    for key in ("SEGMENT", "DOBJ", "BINDING", "RUN", "DENSE_VERTEX", "MATERIAL_EVENT"):
        m = re.search(rf"#define {macro_prefix(desc)}{key}_COUNT (\d+)u", text)
        out[key] = int(m.group(1)) if m else None
    out["has_heads"] = f"{symbol_prefix(desc)}BindingHeads" in text
    return out


def capture_rows(desc):
    """One row per segment: {source, index, link, layer, dobj_count, owner, dl_links}.
    Display-layer segments live on gGRCommonLayerGObjs[layer]; Dream Land's map
    GObjs are the only pupupu-map rows and are already hand-written."""
    rows = []
    callbacks = {name: (cb, link) for name, cb, link in desc.callback_partition}
    for owner_spec in desc.owner_specs:
        owner, name = owner_spec[0], owner_spec[1]
        cb, link = callbacks[name]
        m = CALLBACK.match(cb)
        if not m:
            raise SystemExit(f"{name}: callback {cb} is not a display-layer proc")
        layer, kind = int(m.group(1)), m.group(2)
        # live DObj count = the segment's dobj span from the segment partition
        seg = next(s for s in desc.segment_partition if s[0] == owner)
        # Dream Land's animated map GObjs (gGRCommonStruct.pupupu.map_gobj[n])
        # are the only non-layer rows; the descriptor names them map0..map3.
        if name.startswith("map"):
            source, index = "PUPUPU_MAP", int(name[3:])
        else:
            source, index = "LAYER", layer
        rows.append({"name": name, "source": source, "index": index, "link": link,
                     "layer": layer, "dobj_count": None, "owner": owner,
                     "dl_links": 1 if kind == "Sec" else 0, "segment": seg})
    return rows


def camera_mask_from_include(desc, stage):
    """Bindings whose DObj carries source transform flag 2, 4 or 8 (the
    camera-relative matrix shapes) form the packet's camera_binding_mask;
    check_nds_native_stage.py (verify_camera_binding_contract) re-derives the
    same mask from the same three flags and rejects a row that disagrees --
    flag 8 was missing here until Yamabuki's binding 2 showed it (0x4890 vs
    0x4894). Rigidity is not derivable here and stays 0."""
    name = "nds_native_stage_owner.generated.inc" if stage == "dreamland" else f"nds_native_stage_{stage}.generated.inc"
    path = os.path.join(ROOT, "src", "nds", name)
    if not os.path.isfile(path):
        return 0
    text = io.open(path, encoding="utf-8").read()
    m = re.search(rf"{symbol_prefix(desc)}DObjs\[\d+\] = \{{(.*?)\n\}};", text, re.S)
    if not m:
        return 0
    mask = 0
    for row in re.findall(r"\{\s*0x[0-9a-f]+u,\s*0x([0-9a-f]+)u,\s*0x([0-9a-f]+)u,\s*0x([0-9a-f]+)u,", m.group(1)):
        binding, flags = int(row[1], 16), int(row[2], 16)
        if binding != 0xffff and flags in (2, 4, 8):
            mask |= 1 << binding
    return mask


def dobj_counts_from_include(desc, stage, rows):
    """Segments in the generated include carry first_dobj/dobj_count."""
    name = "nds_native_stage_owner.generated.inc" if stage == "dreamland" else f"nds_native_stage_{stage}.generated.inc"
    path = os.path.join(ROOT, "src", "nds", name)
    if not os.path.isfile(path):
        return
    text = io.open(path, encoding="utf-8").read()
    m = re.search(rf"{symbol_prefix(desc)}Segments\[\d+\] = \{{(.*?)\n\}};", text, re.S)
    if not m:
        return
    entries = re.findall(r"\{\s*0x[0-9a-f]+u,\s*(\d+)u,\s*(\d+)u,", m.group(1))
    by_owner = {int(owner): int(count) for count, owner in entries}
    for row in rows:
        row["dobj_count"] = by_owner.get(row["owner"])


def emit(desc, stage):
    name = cname(stage)
    MAC = stage.upper()
    counts = read_generated_counts(desc, stage)
    rows = capture_rows(desc)
    dobj_counts_from_include(desc, stage, rows)
    layer0 = sum(1 for r in rows if r["link"] == 4)
    assets = list(desc.adapter_asset_ids) + [0] * (4 - len(desc.adapter_asset_ids))
    sizes = list(desc.adapter_asset_sizes) + [0] * (4 - len(desc.adapter_asset_sizes))
    out = []
    out.append(f"/* ---- renderer_adapter_matrix.c: capture + descriptor for {name} (gkind {GKIND[stage]}) ---- */")
    out.append(f"#if defined(NDS_P2_STAGE_{MAC}) && (NDS_P2_STAGE_{MAC} == 1)")
    out.append("static const NDSRendererAdapterNativeStageCaptureSegment")
    out.append(f"    sNdsRendererAdapterNativeStageCapture{name}[{len(rows)}] = {{")
    for i, r in enumerate(rows):
        dc = "??" if r["dobj_count"] is None else f"{r['dobj_count']}u"
        comma = "," if i + 1 < len(rows) else ""
        out.append(f"        {{ NDS_RENDERER_ADAPTER_STAGE_CAPTURE_{r['source']}, {r['index']}u, {r['link']:2d}u, {r['layer']}u, {dc:>4}, {r['owner']}u, {r['dl_links']}u }}{comma}")
    out.append("    };")
    out.append("static const NDSRendererAdapterNativeStageDescriptor")
    out.append(f"    sNdsRendererAdapterNativeStage{name} = {{")
    out.append(f"        {desc.adapter_segment_count}u, {desc.adapter_dobj_count}u, {desc.adapter_binding_count}u, {desc.adapter_asset_count}u, {desc.adapter_material_count}u,")
    out.append("        { " + ", ".join(f"{a}u" for a in assets) + " },")
    out.append("        { " + ", ".join(f"{s}u" for s in sizes) + " },")
    out.append(f"        sNdsRendererAdapterNativeStageCapture{name},")
    out.append(f"        {len(rows)}u,")
    out.append(f"        {layer0}u")
    out.append("    };")
    out.append("#endif")
    out.append("")
    census = desc.expected_counts.get("submit_classes")
    out.append(f"/* ---- nds_native_stage_select.inc: packet row for {name} ---- */")
    out.append(f"#if defined(NDS_P2_STAGE_{MAC}) && (NDS_P2_STAGE_{MAC} == 1) && (NDS_NATIVE_STAGE_LINKED_{MAC} == 1)")
    P, M = symbol_prefix(desc), macro_prefix(desc)
    out.append(f"_Static_assert({M}SEGMENT_COUNT <= NDS_NATIVE_STAGE_MAX_SEGMENT_COUNT &&")
    out.append(f"               {M}RUN_COUNT <= NDS_NATIVE_STAGE_MAX_RUN_COUNT &&")
    out.append(f"               {M}DENSE_VERTEX_COUNT <= NDS_NATIVE_STAGE_MAX_DENSE_VERTEX_COUNT,")
    out.append(f"               \"{name} packet exceeds the native stage workspace\");")
    out.append(f"static const NDSNativeStagePacket sNdsNativeStagePacket{name} = {{")
    for t in ("Assets", "Segments", "DObjs", "Bindings", "Runs", "Vertices", "Corners",
              "TextureEpochs", "MaterialEvents", "StatePolicies", "StateDeltas",
              "StateSequence", "StateSpans"):
        out.append(f"    {P}{t},")
    out.append(f"#if NDS_TASK51_STAGE_NATIVE\n    {P}BakedWorldMatrices,\n#endif")
    for c in ("ASSET_COUNT", "SEGMENT_COUNT", "DOBJ_COUNT", "BINDING_COUNT", "RUN_COUNT",
              "TEXTURE_EPOCH_COUNT", "MATERIAL_EVENT_COUNT", "STATE_POLICY_COUNT",
              "STATE_DELTA_COUNT", "STATE_SEQUENCE_COUNT", "STATE_SPAN_COUNT",
              "DENSE_VERTEX_COUNT", "CORNER_COUNT", "TRIANGLE_COUNT", "SOURCE_COMMAND_COUNT",
              "SOURCE_VERTEX_COUNT", "VERTEX_COMMAND_COUNT", "TRIANGLE_COMMAND_COUNT",
              "BAKED_WORLD_COUNT"):
        out.append(f"    {M}{c},")
    out.append("    " + (", ".join(f"{v}u" for v in census) if census else "/* submit census ?? */") + ",")
    for c in ("CROSS_MATRIX_RUN_COUNT", "CROSS_MATRIX_TRIANGLE_COUNT", "CROSS_MATRIX_FOREIGN_CORNER_COUNT"):
        out.append(f"    {M}{c},")
    camera = camera_mask_from_include(desc, stage)
    out.append("    /* rigid_binding_mask: not derivable here, 0 costs only the replay.")
    out.append("     * camera_binding_mask: bindings whose DObj carries source flag 2 or 4")
    out.append("     * (matrix kinds 48/46), read off the generated DObj table. */")
    out.append(f"    0ULL,\n    {'0x%xULL' % camera if camera else '0ULL'},\n    0u,")
    out.append(f"    NDS_NATIVE_STAGE_GKIND_{MAC},")
    out.append("    { 0u, 0u }" + ("," if counts and counts["has_heads"] else ""))
    if counts and counts["has_heads"]:
        out.append(f"    {P}BindingDObjs,\n    {P}BindingHeads")
    out.append("};\n#endif")
    out.append("")
    out.append(f"/* registry: index {GKIND[stage]} of sNdsNativeStagePacketTable and sNdsRendererAdapterNativeStageTable;")
    out.append(f"   #define NDS_NATIVE_STAGE_GKIND_{MAC} {GKIND[stage]}u; MULTI condition; MAX chain; assets.c include;")
    out.append(f"   Makefile NDS_NATIVE_STAGE_{MAC}_INC rule + nds_renderer_assets.o dep + elf prereq; build.ps1 gen + outputs. */")
    if counts:
        out.append(f"/* generated counts: {counts} */")
    return "\n".join(out), rows


def check_against_c(stage, rows, desc):
    path = os.path.join(ROOT, "src", "port", "renderer_adapter_matrix.c")
    text = io.open(path, encoding="utf-8").read()
    name = cname(stage)
    m = re.search(rf"sNdsRendererAdapterNativeStageCapture{name}\[(\d+)\] = \{{(.*?)\n    \}};", text, re.S)
    if not m:
        print(f"{name}: no capture table in renderer_adapter_matrix.c")
        return 1
    have = re.findall(r"\{\s*NDS_RENDERER_ADAPTER_STAGE_CAPTURE_(\w+),\s*(\d+)u,\s*(\d+)u,\s*(\d+)u,\s*(\d+)u,\s*(\d+)u(?:,\s*(\d+)u)?\s*\}", m.group(2))
    bad = 0
    for i, (row, got) in enumerate(zip(rows, have)):
        want = (row["source"], str(row["index"]), str(row["link"]), str(row["layer"]),
                str(row["dobj_count"]), str(row["owner"]), str(row["dl_links"]))
        got = tuple(got[:6]) + ((got[6] or "0"),)
        if want != got:
            print(f"{name} row {i}: descriptor {want} vs C {got}")
            bad += 1
    if len(have) != len(rows):
        print(f"{name}: {len(rows)} descriptor segments vs {len(have)} C rows")
        bad += 1
    m = re.search(rf"sNdsRendererAdapterNativeStage{name} = \{{\s*(\d+)u, (\d+)u, (\d+)u, (\d+)u, (\d+)u,", text)
    if m:
        got = tuple(int(x) for x in m.groups())
        want = (desc.adapter_segment_count, desc.adapter_dobj_count, desc.adapter_binding_count,
                desc.adapter_asset_count, desc.adapter_material_count)
        if got != want:
            print(f"{name} descriptor counts: descriptor {want} vs C {got}")
            bad += 1
    # The packet row in select.inc: submit census, gkind define, registry slot.
    sel = io.open(os.path.join(ROOT, "src", "nds", "nds_native_stage_select.inc"), encoding="utf-8").read()
    census = desc.expected_counts.get("submit_classes")
    m = re.search(rf"sNdsNativeStagePacket{name} = \{{(.*?)\n\}};", sel, re.S)
    if m is None:
        if stage != "dreamland":
            print(f"{name}: no packet row in nds_native_stage_select.inc")
            bad += 1
    else:
        row = m.group(1)
        cm = re.search(r"\n\s*(\d+)u, (\d+)u, (\d+)u,\n", row)
        if census and cm and tuple(int(x) for x in cm.groups()) != tuple(census):
            print(f"{name} submit census: descriptor {tuple(census)} vs C {cm.groups()}")
            bad += 1
        MAC = "PUPUPU" if stage == "dreamland" else stage.upper()
        gm = re.search(rf"#define NDS_NATIVE_STAGE_GKIND_{MAC} (\d+)u", sel)
        if gm is None or int(gm.group(1)) != GKIND[stage]:
            print(f"{name}: NDS_NATIVE_STAGE_GKIND_{MAC} is {gm.group(1) if gm else 'absent'}, want {GKIND[stage]}")
            bad += 1
        tm = re.search(r"sNdsNativeStagePacketTable\[NDS_NATIVE_STAGE_GKIND_COUNT\] = \{(.*?)\n    \};", sel, re.S)
        if tm:
            slots = []
            for line in tm.group(1).split("\n"):
                line = line.strip()
                if line.startswith("&sNdsNativeStagePacket") or line == "NULL,":
                    slots.append(line)
            # #if/#else pairs leave both arms in the text; count the &packet
            # lines only, in order, as the flag-on layout.
            packets = [l for l in slots if l.startswith("&")]
            expect = f"&sNdsNativeStagePacket{name},"
            if expect not in packets and stage != "dreamland":
                print(f"{name}: not in sNdsNativeStagePacketTable")
                bad += 1
    print(f"{name}: {'OK' if bad == 0 else str(bad) + ' difference(s)'}")
    return 1 if bad else 0


def blob_row(stage):
    """Relocatable-blob registry row for a blob stage.

    Prints (gkind, NitroFS path, slab bytes, FNV) plus the per-stage scalar
    counts the C loader decodes from the blob header. The .inc files stay
    the checkers' C surface; this row is what a blob-only stage links.
    """
    import generate_nds_native_stage as generator

    desc = get_descriptor(stage)
    repo_root = __import__("pathlib").Path(ROOT)
    packet = generator.generate(repo_root, desc)
    blob = generator.build_stage_blob(packet, desc)
    header, _parsed = generator.parse_stage_blob(blob)
    counts = header["counts"]
    out = []
    out.append(f"/* ---- blob row for {cname(stage)} (gkind {GKIND[stage]}) ---- */")
    out.append(f"/* nitrofs: {generator.blob_nitrofs_path(stage)} */")
    out.append(f"/* slab_bytes={packet.slab_bytes()}u blob_bytes={len(blob)}u "
               f"fnv=0x{header['fnv']:08x} */")
    out.append(f"static const NDSNativeStageBlobEntry "
               f"sNdsNativeStageBlob{cname(stage)} = {{")
    out.append(f"    {GKIND[stage]}u, /* gkind */")
    out.append(f"    \"{generator.blob_nitrofs_path(stage)}\",")
    out.append(f"    {packet.slab_bytes()}u, /* slab bytes */")
    out.append(f"    0x{header['fnv']:08x}u, /* FNV-1a-32 of the blob body */")
    for key in generator._BLOB_COUNT_KEYS:
        out.append(f"    {counts[key]}u, /* {key} */")
    out.append(f"    0x{header['rigid_mask']:016x}ULL, /* rigid mask */")
    out.append(f"    0x{header['camera_mask']:016x}ULL, /* camera mask */")
    out.append(f"    {header['has_generated_segment0']}u, /* has_generated_segment0 */")
    out.append("};")
    return "\n".join(out)


def blob_maxima_header():
    """Generated maxima header over every registered stage's .inc counts.

    When no packet but Dream Land is linked, the MAX-sized workspaces in
    nds_renderer_assets.c keep their sizing from these blob maxima instead
    of shrinking to the Dream Land literals.
    """
    import generate_nds_native_stage as generator

    _ = generator.NDS_STAGE_BLOB_ABI  # keep the layout owner import-local
    maxima: dict[str, int] = {}
    for stage in GKIND:
        desc = get_descriptor(stage)
        name = ("nds_native_stage_owner.generated.inc" if stage == "dreamland"
                else f"nds_native_stage_{stage}.generated.inc")
        path = os.path.join(ROOT, "src", "nds", name)
        if not os.path.isfile(path):
            continue
        text = io.open(path, encoding="utf-8").read()
        prefix = "NDS_NATIVE_STAGE_" + (desc.macro_prefix or "")
        for m in re.finditer(r"#define (NDS_NATIVE_STAGE_\w+?) (\d+)u", text):
            macro, value = m.group(1), int(m.group(2))
            if not macro.startswith(prefix):
                continue
            suffix = macro[len(prefix):]
            if not suffix or "/" in suffix:
                continue
            maxima[suffix] = max(maxima.get(suffix, 0), value)
    out = ["/* Generated by emit_native_stage_runtime_rows.py --maxima. Do not edit. */",
           "/* Max of every COUNT across all registered stage packets. */",
           "#ifndef SSB64_NDS_NATIVE_STAGE_BLOB_MAXIMA_H",
           "#define SSB64_NDS_NATIVE_STAGE_BLOB_MAXIMA_H",
           ""]
    for suffix in sorted(maxima):
        out.append(f"#define NDS_NATIVE_STAGE_BLOB_MAX_{suffix} {maxima[suffix]}u")
    out.extend(["", "#endif"])
    return "\n".join(out)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--stage", required=True)
    ap.add_argument("--check", action="store_true")
    ap.add_argument("--blob-row", action="store_true",
                    help="print the relocatable-blob registry row for --stage")
    ap.add_argument("--maxima", action="store_true",
                    help="print the generated blob maxima header")
    args = ap.parse_args()
    if args.maxima:
        print(blob_maxima_header())
        return 0
    if args.blob_row:
        print(blob_row(args.stage))
        return 0
    desc = get_descriptor(args.stage)
    text, rows = emit(desc, args.stage)
    if args.check:
        return check_against_c(args.stage, rows, desc)
    print(text)
    return 0


if __name__ == "__main__":
    sys.exit(main())
