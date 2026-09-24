"""Ness's VS Results Win3 pose lights joint 17, and both details must bake it.

THE DEFECT THIS PINS. `scsubsysdataness.c` `D_ovl1_8039272C` is
`dFTNessSubMotionDescs` rows 3 AND 4, so `mnVSResultsGetStatusWin` can hand a
winning Ness demo status Win3 and reach it. Its first word is the raw
`0xA0880001` -- opcode 40, joint 17, model part 1 -- and nothing restores it.

Joint 17 is container index 13. `dNessMain_setup_parts` 0xFFFFFFC0 SELECTS that
descriptor, so the live DObj exists, but the JointTree gives it no display list
in either detail. The model-part write therefore ADDS a drawable root rather
than replacing one: the live vector grows 14 -> 15 and every binding from joint
20 onward shifts by one. No `(binding, offset)` variant row can express that --
the runtime resolver rejects on `root_count` before it ever compares offsets --
so the pose needs a complete `OWNER_ROOT_PROGRAMS["ness"]` program plus its
`P2_ROOT_PROGRAM_APPENDIX` bake.

Without the bake the owner declines, and a packed fighter that declines is
`ndsPreviewPackLoadHalt(20, kind)`: a Ness Results win with the Win3 draw would
freeze the Results screen exactly as Link's Claps pose did. The character
select cannot see it, because `mnPlayersVSGetStatusSelected` gives Ness row 2.

THE DROP ARM IS THE POINT. `test_win3_root_dropped_from_either_detail_fails`
removes the appendix row one detail at a time and requires BOTH the generator
and the coverage census to go red for that detail alone, with a control that
proves the unmodified state is green first -- an inventory assertion would pass
just as well against a half-baked table that only carries High.

Usage:
    python -m pytest scripts/fighters/test_native_ness_win3.py -q
"""
from __future__ import annotations

import re
from pathlib import Path

import pytest

import check_model_part_mutation_coverage as census
import generate_nds_native_owners as native

SOURCE = native._paths.REPO_ROOT
RUNTIME = Path(__file__).resolve().parents[2]
RELOC = SOURCE / "decomp/BattleShip-main/decomp/src/relocData"
SCSUBSYS = SOURCE / "decomp/BattleShip-main/decomp/src/sc/scsubsys"

WIN3_CLIP = "D_ovl1_8039272C"
WIN3_ROOT = 0x6D90
WIN3_JOINT = 17
WIN3_MODELPART = 1
# Container index 13 -> joint 17; joint 16 is index 12 and canonical binding 7,
# joint 17's depth-6 parent and the source of its matrix/cache provenance.
WIN3_DESCRIPTOR = 13
WIN3_PARENT_BINDING = 7
CANONICAL_ROOT_COUNT = 14
WIN3_ROOT_COUNT = 15
# The insertion point: descriptor 13 sorts after descriptor 12 (joint 16,
# binding 7) and before descriptor 16 (joint 20, binding 8).
WIN3_ROOT_INDEX = 8


def test_source_clip_is_results_reachable_and_adds_joint_17():
    """Re-derive the whole diagnosis from decomp, not from the generator."""
    demo = (SCSUBSYS / "scsubsysdataness.c").read_text(
        encoding="utf-8", errors="replace")

    # The clip is Results-reachable: rows 3 and 4 of the submotion table, and
    # a demo status IS its submotion row (ft/ftmain.c:4559-4562,4608-4610).
    table = re.search(
        r"FTMotionDesc dFTNessSubMotionDescs\[\]\s*=\s*\{(.*?)\n\};",
        demo, re.S)
    assert table is not None
    fields = [field.strip() for field in table.group(1).split(",")]
    rows = {
        row: fields[row * census.SUBMOTION_FIELDS_PER_ROW + 1].lstrip("&")
        for row in range(len(fields) // census.SUBMOTION_FIELDS_PER_ROW)
    }
    assert {row for row, clip in rows.items() if clip == WIN3_CLIP} == {3, 4}
    assert 3 in census.RESULTS_WIN_ROWS
    # ... and the character select plays a DIFFERENT row, which is why a
    # CSS-scoped census could never have seen this.
    assert census.CSS_SELECTED_ROW["ness"] == 2

    # The clip's ONLY root-vector command is (17, 1), and it is never restored.
    body = re.search(
        r"s32 " + WIN3_CLIP + r"\[\]\s*=\s*\{(.*?)\n\};", demo, re.S)
    assert body is not None
    mutations = [
        census.decode_raw_modelpart(int(word, 16))
        for word in census.RAW_HEX.findall(body.group(1))
    ]
    assert [m for m in mutations if m is not None] == [
        (WIN3_JOINT, WIN3_MODELPART)]

    # Joint 17 resolves to 0x6d90 through the same chain ftParamSetModelPartID
    # walks, and BOTH detail rows name the one display list.
    main = (RELOC / "239_NessMain.c").read_text(
        encoding="utf-8", errors="replace")
    container = census.CONTAINER.search(main)
    assert container is not None
    entries = [entry.strip()
               for entry in container.group(1).replace("\n", " ").split(",")
               if entry.strip()]
    assert WIN3_JOINT - 4 == WIN3_DESCRIPTOR
    tag = re.search(r"modelparts_desc_(0x[0-9A-Fa-f]+)",
                    entries[WIN3_DESCRIPTOR])
    assert tag is not None and tag.group(1) == "0x204"
    descriptors = {
        "desc_%s" % name.lower(): census.GFXROW.findall(rows_body)
        for name, rows_body in census.DESC.findall(main)
    }
    symbols = descriptors["desc_0x204"]
    offsets = census.model_offsets("Ness")
    for detail_index in (0, 1):
        row = WIN3_MODELPART * 2 + detail_index
        assert offsets[symbols[row]] == WIN3_ROOT


@pytest.mark.parametrize("detail", ("high", "low"))
def test_joint_17_is_selected_but_carries_no_canonical_display_list(detail):
    """The reason this is a program and not a per-binding variant row."""
    payload = native.load_o2r_payload(SOURCE, "ness")
    descriptors = native._owner_joint_descriptors(payload, "ness", detail)[:-1]
    selected = native._owner_selected_descriptor_indices(
        "ness", len(descriptors) + 1)
    # Created by setup_parts...
    assert WIN3_DESCRIPTOR in selected
    # ...and drawing nothing until the clip writes a DL into it.
    assert descriptors[WIN3_DESCRIPTOR][1] is None
    # Its parent is joint 16 (descriptor 12, depth 6), whose canonical binding
    # is the one the appendix bake is keyed to.
    assert descriptors[WIN3_DESCRIPTOR][0] == descriptors[12][0] + 1
    drawable = [index for index in selected
                if index < len(descriptors)
                and descriptors[index][1] is not None]
    assert len(drawable) == CANONICAL_ROOT_COUNT
    assert drawable.index(12) == WIN3_PARENT_BINDING


@pytest.mark.parametrize("detail", ("high", "low"))
def test_win3_program_inserts_the_root_in_source_order(detail):
    context = native.build_p2_owner_runtime_context(SOURCE, "ness", detail)
    assert context["canonical_root_count"] == CANONICAL_ROOT_COUNT
    # Win3's row first; slice 7 appended the yo-yo's and the forward-smash
    # bat's after it (test_native_ness_yoyo.py, test_native_ness_fsmash.py),
    # so Win3's bake keeps its indices.
    assert [(binding, offset)
            for binding, offset in context["root_program_appendix_specs"]][0] == (
        WIN3_PARENT_BINDING, WIN3_ROOT)

    programs = native.build_owner_root_programs(SOURCE, context)
    assert [program["name"] for program in programs] == [
        "Win3", "YoYo", "FSmash"]
    win3 = programs[0]
    assert len(win3["roots"]) == WIN3_ROOT_COUNT
    assert win3["root_offsets"][WIN3_ROOT_INDEX] == WIN3_ROOT
    assert win3["root_joints"][WIN3_ROOT_INDEX] == WIN3_JOINT
    # The added root borrows its parent's logical binding; every canonical root
    # keeps its own, and the ones after the insertion keep their ORDER.
    assert win3["root_bindings"][WIN3_ROOT_INDEX] == WIN3_PARENT_BINDING
    canonical_offsets = [row[0] for row in
                         context["roots"][:CANONICAL_ROOT_COUNT]]
    assert list(win3["root_offsets"]) == (
        canonical_offsets[:WIN3_ROOT_INDEX] + [WIN3_ROOT]
        + canonical_offsets[WIN3_ROOT_INDEX:])
    # Every live root keeps the exact resident bake row and light preamble the
    # canonical owner publishes; nothing is re-baked for the program.
    rows = {row[0]: (row, light) for row, light
            in zip(context["roots"], context["light_preamble_indices"])}
    assert win3["roots"] == [rows[offset][0]
                             for offset in win3["root_offsets"]]
    assert win3["light_indices"] == [rows[offset][1]
                                     for offset in win3["root_offsets"]]
    # 0x6d90 is a self-contained RAW program: it loads its own vertices, so no
    # live root restores another root's vertex cache.
    assert set(win3["cross_slots"]) == {native.PACKED_GX_SLOT_CURRENT}
    # Unlike Link's and Yoshi's dynamically inserted hidden parts, joint 17 is
    # an ordinary JointTree node, so a real source parent schedule is published.
    assert len(win3["binding_parents"]) == WIN3_ROOT_COUNT
    assert win3["binding_parents"][WIN3_ROOT_INDEX] == WIN3_PARENT_BINDING

    emitted = "\n".join(native.render_p2_owner_runtime_program(
        dict(context, root_programs=programs)))
    suffix = "" if detail == "high" else "Low"
    for array in ("Roots", "CrossPaletteSlots"):
        assert "sNdsNativeNessWin3%s%s" % (array, suffix) in emitted
    assert ("sNdsNativeNessWin3BindingParents" in emitted) == (detail == "high")
    assert ("NDS_NATIVE_NESS_ROOT_PROGRAMS_PRESENT" in emitted) == (
        detail == "high")


@pytest.mark.parametrize("victim", ("high", "low"))
def test_win3_root_dropped_from_either_detail_fails(victim):
    """Remove the bake for ONE detail and require the red, with a control.

    A table that carried only High would satisfy any assertion written against
    High alone, and Low is the detail a four-fighter Results screen actually
    draws -- so each detail gets its own removal arm.
    """
    # CONTROL: prove the unmodified state is green, or the removal proves
    # nothing. (The same trap check_results_demo_motion_closure's self-test
    # guards against.)
    assert not [line for line in census_failures() if "0x6d90" in line]
    for detail in ("high", "low"):
        context = native.build_p2_owner_runtime_context(SOURCE, "ness", detail)
        assert native.build_owner_root_programs(SOURCE, context)

    saved = native.P2_ROOT_PROGRAM_APPENDIX["ness"][victim]
    native.P2_ROOT_PROGRAM_APPENDIX["ness"][victim] = tuple(
        row for row in saved if row[1] != WIN3_ROOT)
    try:
        # The generator must refuse to build a program whose root has no bake,
        # rather than silently emitting a short vector that matches nothing.
        context = native.build_p2_owner_runtime_context(SOURCE, "ness", victim)
        with pytest.raises(ValueError, match="0x6d90"):
            native.build_owner_root_programs(SOURCE, context)
        # ...and the mutation census must name that detail, and only it.
        failures = [line for line in census_failures() if "0x6d90" in line]
        assert failures, "dropping the %s bake left the census green" % victim
        assert all(("part 1 %s" % victim) in line for line in failures), failures
    finally:
        native.P2_ROOT_PROGRAM_APPENDIX["ness"][victim] = saved

    # The table is restored, so the census is green again and a later test in
    # this session cannot inherit the removal.
    assert not [line for line in census_failures() if "0x6d90" in line]


def census_failures() -> list[str]:
    """The mutation census's Ness failures, re-resolved against live tables."""
    import io
    import contextlib

    captured = io.StringIO()
    with contextlib.redirect_stdout(captured):
        census.main()
    return [line.strip() for line in captured.getvalue().splitlines()
            if line.strip().startswith("ness ")]
