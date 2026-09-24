"""Ness's yo-yo smashes add a root, and both details must carry its program.

THE DEFECT THIS PINS (P2-2p8 Phase 1 slice 7, 2026-09-24). `ftdata.c` gives
Ness's USmash and DSmash motion rows the anim-desc mask 0x10000000, so
`ftMainSetStatus` installs `dNessMain_hiddenparts[3]` -- joint 30 under joint 4,
kind 0 (its parent's LAST child) -- before either motion runs. The JointTree
leaves joint 30 (descriptor 26) blank in both details, so the install adds no
root; two frames later the motion's `SetModelPartID(30, 0)` gives it
`dNessMain_modelparts_desc_0x254` model part 0, `dNessModel_DL_0x69E0` in both
details -- the yo-yo -- and the live vector grows 14 -> 15 with the yo-yo last.

No generated program carried that vector until this slice, so the owner
declined every draw of the rest of either smash: Ness drew NOTHING there, in
both renderer routes (15 REJECTED_PROGRAM native failures a frame, statuses
207/208), and the lean path named them `validate`. Two checks were blind to it:
the model-part census treats a write to a setup-omitted joint as a source no-op
(right in general, wrong for a hidden joint), and the hidden-part census only
followed hidden parts that carry a JointTree display list of their own.

THE DROP ARMS ARE THE POINT, as in test_native_ness_win3.py: removing the
yo-yo's appendix row for ONE detail must turn the generator red for that detail
alone, and the pre-slice tables must turn the hidden-part census red for
exactly the two smashes in both details -- each with a control that proves the
unmodified tables green first.

Usage:
    python -m pytest scripts/fighters/test_native_ness_yoyo.py -q
"""
from __future__ import annotations

import contextlib
import io
import re
import struct

import pytest

import check_hidden_part_root_coverage as hidden_census
import generate_nds_native_owners as native

SOURCE = native._paths.REPO_ROOT
RELOC = SOURCE / "decomp/BattleShip-main/decomp/src/relocData"
FTDATA = SOURCE / "decomp/BattleShip-main/decomp/src/ft/ftdata.c"

YOYO_ROOT = 0x69E0
YOYO_JOINT = 30
YOYO_DESCRIPTOR = YOYO_JOINT - 4
YOYO_HIDDEN_PART = 3
YOYO_MASK = 0x10000000
YOYO_BINDING = 0
CANONICAL_ROOT_COUNT = 14
YOYO_ROOT_COUNT = 15


def test_source_installs_a_blank_joint_30_and_draws_the_yoyo_on_it():
    """Re-derive the diagnosis from decomp, not from the generator."""
    ftdata = FTDATA.read_text(encoding="utf-8", errors="replace")
    rows = re.findall(
        r"&llFTNessAnim(USmash|DSmash)FileID\s*,\s*(\w+)\s*,([^}]*)\}", ftdata)
    assert {name for name, _motion, _flags in rows} == {"USmash", "DSmash"}
    for name, motion, flags in rows:
        assert motion == "dNessMainMotion_" + name
        mask = 0
        for literal in re.findall(r"0x([0-9A-Fa-f]+)", flags):
            mask |= int(literal, 16)
        assert mask & ~0xFFFF == YOYO_MASK
    # bit 28 -> hidden-part index 31 - 28
    assert 31 - (YOYO_MASK.bit_length() - 1) == YOYO_HIDDEN_PART

    main = (RELOC / "239_NessMain.c").read_text(
        encoding="utf-8", errors="replace")
    table = re.search(
        r"FTHiddenPart dNessMain_hiddenparts\[4\]\s*=\s*\{(.*?)\n\};", main, re.S)
    assert table is not None
    hidden = [tuple(int(v, 16) for v in re.findall(r"0x([0-9A-Fa-f]+)", row))
              for row in re.findall(r"\{([^}]*)\}", table.group(1))]
    # { root joint, parent joint, partindex, kind }: kind 0 appends the joint
    # as its parent's last child (ftMainUpdateHiddenPartID).
    assert hidden[YOYO_HIDDEN_PART] == (YOYO_JOINT, 4, 1, 0)

    motions = (RELOC / "238_NessMainMotion.c").read_text(
        encoding="utf-8", errors="replace")
    for name in ("USmash", "DSmash"):
        body = re.search(
            r"ftMotionCommand dNessMainMotion_" + name + r"\[\]\s*=\s*\{(.*?)\n\};",
            motions, re.S)
        assert body is not None
        writes = re.findall(
            r"ftMotionCommandSetModelPartID\((\d+),\s*(-?\d+)\)", body.group(1))
        assert writes == [(str(YOYO_JOINT), "0")]

    # Joint 30 is container index 26: desc_0x254, whose model part 0 rows
    # (both details) name DL_0x69E0.
    container = re.search(
        r"dNessMain_modelparts_container\[27\]\s*=\s*\{(.*?)\};", main, re.S)
    assert container is not None
    entries = [entry.strip()
               for entry in container.group(1).replace("\n", " ").split(",")
               if entry.strip()]
    assert "desc_0x254" in entries[YOYO_DESCRIPTOR]
    desc = re.search(
        r"FTModelPart dNessMain_modelparts_desc_0x254\[2\]\s*=\s*\{(.*?)\n\};",
        main, re.S)
    assert desc is not None
    assert re.findall(r"dNessModel_(\w+)", desc.group(1)) == [
        "DL_0x69E0", "DL_0x69E0"]


@pytest.mark.parametrize("detail", ("high", "low"))
def test_joint_30_is_blank_and_outside_setup_parts(detail):
    """Why the install alone adds no root, and why the resolver drops (30, 0)."""
    payload = native.load_o2r_payload(SOURCE, "ness")
    descriptors = native._owner_joint_descriptors(payload, "ness", detail)[:-1]
    selected = native._owner_selected_descriptor_indices(
        "ness", len(descriptors) + 1)
    assert YOYO_DESCRIPTOR not in selected
    assert descriptors[YOYO_DESCRIPTOR][1] is None
    main_payload, container = native._load_owner_root_program_payload(
        SOURCE, "ness")
    assert native._owner_modelpart_display_offset(
        main_payload, container, YOYO_JOINT, 0, detail) == YOYO_ROOT
    row = native.NESS_MAIN_HIDDENPARTS_OFFSET + YOYO_HIDDEN_PART * 16
    assert struct.unpack_from(">iiii", main_payload, row) == (YOYO_JOINT, 4, 1, 0)


@pytest.mark.parametrize("detail", ("high", "low"))
def test_yoyo_program_appends_the_root_last(detail):
    context = native.build_p2_owner_runtime_context(SOURCE, "ness", detail)
    assert context["canonical_root_count"] == CANONICAL_ROOT_COUNT
    assert (YOYO_BINDING, YOYO_ROOT) in [
        (binding, offset)
        for binding, offset in context["root_program_appendix_specs"]]

    programs = native.build_owner_root_programs(SOURCE, context)
    assert [program["name"] for program in programs] == [
        "Win3", "YoYo", "FSmash"]
    yoyo = programs[1]
    canonical_offsets = [row[0] for row in
                         context["roots"][:CANONICAL_ROOT_COUNT]]
    assert len(yoyo["roots"]) == YOYO_ROOT_COUNT
    assert list(yoyo["root_offsets"]) == canonical_offsets + [YOYO_ROOT]
    assert yoyo["root_joints"][-1] == YOYO_JOINT
    assert yoyo["root_bindings"][-1] == YOYO_BINDING
    # Dynamically inserted hidden joint: no source parent schedule, each live
    # DObj matrix captured directly (Yoshi's grab family, Link's Catch).
    assert set(yoyo["binding_parents"]) == {native.INVALID_U8}
    # 0x69e0 loads its own four vertices; no root stores or restores a slot.
    assert set(yoyo["cross_slots"]) == {native.PACKED_GX_SLOT_CURRENT}
    # Every live root keeps the resident bake row the owner publishes.
    rows = {row[0]: (row, light) for row, light
            in zip(context["roots"], context["light_preamble_indices"])}
    assert yoyo["roots"] == [rows[offset][0]
                             for offset in yoyo["root_offsets"]]

    emitted = "\n".join(native.render_p2_owner_runtime_program(
        dict(context, root_programs=programs)))
    suffix = "" if detail == "high" else "Low"
    for array in ("Roots", "CrossPaletteSlots"):
        assert "sNdsNativeNessYoYo%s%s" % (array, suffix) in emitted
    assert ("sNdsNativeNessYoYoBindingParents" in emitted) == (detail == "high")


@pytest.mark.parametrize("victim", ("high", "low"))
def test_yoyo_root_dropped_from_either_detail_fails(victim):
    """Remove the yo-yo's bake for ONE detail and require the red for that
    detail alone, with a control that proves the tables green first."""
    for detail in ("high", "low"):
        context = native.build_p2_owner_runtime_context(SOURCE, "ness", detail)
        assert "YoYo" in [p["name"] for p in
                          native.build_owner_root_programs(SOURCE, context)]

    saved = native.P2_ROOT_PROGRAM_APPENDIX["ness"][victim]
    native.P2_ROOT_PROGRAM_APPENDIX["ness"][victim] = tuple(
        row for row in saved if row[1] != YOYO_ROOT)
    try:
        # The generator refuses a program whose root has no bake rather than
        # emitting a vector that matches nothing...
        context = native.build_p2_owner_runtime_context(SOURCE, "ness", victim)
        with pytest.raises(ValueError, match="0x69e0"):
            native.build_owner_root_programs(SOURCE, context)
        # ...and only in the detail that lost it.
        other = "low" if victim == "high" else "high"
        context = native.build_p2_owner_runtime_context(SOURCE, "ness", other)
        assert "YoYo" in [p["name"] for p in
                          native.build_owner_root_programs(SOURCE, context)]
    finally:
        native.P2_ROOT_PROGRAM_APPENDIX["ness"][victim] = saved


def test_hidden_part_census_names_the_yoyo_without_its_program():
    """The census builds programs through the generator's own tables: back at
    the pre-slice-7 tables (no YoYo program, no 0x69e0 bake) it names exactly
    Ness's two smashes in both details, and nothing else; restored, green."""
    assert not yoyo_census_failures()
    saved = native.OWNER_ROOT_PROGRAMS["ness"]
    saved_appendix = dict(native.P2_ROOT_PROGRAM_APPENDIX["ness"])
    native.OWNER_ROOT_PROGRAMS["ness"] = tuple(
        program for program in saved if program[0] != "YoYo")
    for detail, rows in saved_appendix.items():
        native.P2_ROOT_PROGRAM_APPENDIX["ness"][detail] = tuple(
            row for row in rows if row[1] != YOYO_ROOT)
    try:
        failures = yoyo_census_failures()
    finally:
        native.OWNER_ROOT_PROGRAMS["ness"] = saved
        native.P2_ROOT_PROGRAM_APPENDIX["ness"].update(saved_appendix)
    assert len(failures) == 4, failures
    assert {line.split()[1] for line in failures} == {"high", "low"}
    assert {line.split("(")[1].split(")")[0] for line in failures} == {
        "USmash", "DSmash"}
    assert all("mask 0x10000000" in line and
               "hidden part 3 joint 30 0x69e0" in line for line in failures)
    assert not yoyo_census_failures()


def yoyo_census_failures() -> list[str]:
    """The hidden-part census's Ness failure lines."""
    captured = io.StringIO()
    with contextlib.redirect_stdout(captured):
        hidden_census.main()
    return [line.strip() for line in captured.getvalue().splitlines()
            if line.strip().startswith("ness ")]
