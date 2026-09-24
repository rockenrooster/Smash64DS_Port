"""Ness's forward smash (the bat) adds a root, and both details carry its program.

THE DEFECT THIS PINS (P2-2p8 Phase 1 slice 7, 2026-09-24). Two frames into
`dNessMainMotion_FSmash` the motion runs `SetModelPartID(17, 0)`. Joint 17 is
container index 13: `dNessMain_setup_parts` selects it, but the JointTree gives
it no display list in either detail -- the same joint Win3 lights -- so model
part 0 of `dNessMain_modelparts_desc_0x204`, `dNessModel_DL_0x6B50` (the bat,
both details), ADDS a root at joint 17's place: 15 against a canonical 14.

No program carried that vector, so the owner declined and Ness drew nothing for
the rest of the smash, in both renderer routes (status 204, AttackS4). Slice 6
had counted those draws with the yo-yo's under one `validate` total; slice 7's
high-detail run named them apart. `check_model_part_mutation_coverage.py` could
not see it: it reads every part-0 write as "restores canonical", which is true
only where the joint has a canonical display list.

Usage:
    python -m pytest scripts/fighters/test_native_ness_fsmash.py -q
"""
from __future__ import annotations

import re

import pytest

import generate_nds_native_owners as native

SOURCE = native._paths.REPO_ROOT
RELOC = SOURCE / "decomp/BattleShip-main/decomp/src/relocData"

BAT_ROOT = 0x6B50
BAT_JOINT = 17
BAT_DESCRIPTOR = BAT_JOINT - 4
BAT_BINDING = 7            # joint 16, joint 17's parent (as for Win3)
BAT_ROOT_INDEX = 8         # after joint 16 (binding 7), before joint 20
CANONICAL_ROOT_COUNT = 14


def test_source_fsmash_writes_part_0_on_a_blank_joint():
    """Re-derive the diagnosis from decomp, not from the generator."""
    motions = (RELOC / "238_NessMainMotion.c").read_text(
        encoding="utf-8", errors="replace")
    body = re.search(
        r"ftMotionCommand dNessMainMotion_FSmash\[\]\s*=\s*\{(.*?)\n\};",
        motions, re.S)
    assert body is not None
    writes = re.findall(
        r"ftMotionCommandSetModelPartID\((\d+),\s*(-?\d+)\)", body.group(1))
    assert writes == [(str(BAT_JOINT), "0")]
    main = (RELOC / "239_NessMain.c").read_text(
        encoding="utf-8", errors="replace")
    desc = re.search(
        r"FTModelPart dNessMain_modelparts_desc_0x204\[4\]\s*=\s*\{(.*?)\n\};",
        main, re.S)
    assert desc is not None
    # rows are modelparts[part][detail]: part 0 = rows 0/1 (the bat), part 1 =
    # rows 2/3 (Win3's 0x6D90)
    assert re.findall(r"dNessModel_(\w+)", desc.group(1))[:2] == [
        "DL_0x6B50", "DL_0x6B50"]


@pytest.mark.parametrize("detail", ("high", "low"))
def test_fsmash_program_puts_the_bat_at_joint_17(detail):
    payload = native.load_o2r_payload(SOURCE, "ness")
    descriptors = native._owner_joint_descriptors(payload, "ness", detail)[:-1]
    assert descriptors[BAT_DESCRIPTOR][1] is None     # blank in the JointTree
    context = native.build_p2_owner_runtime_context(SOURCE, "ness", detail)
    assert (BAT_BINDING, BAT_ROOT) in [
        (binding, offset)
        for binding, offset in context["root_program_appendix_specs"]]
    programs = native.build_owner_root_programs(SOURCE, context)
    assert [program["name"] for program in programs] == [
        "Win3", "YoYo", "FSmash"]
    fsmash = programs[2]
    canonical_offsets = [row[0] for row in
                         context["roots"][:CANONICAL_ROOT_COUNT]]
    assert list(fsmash["root_offsets"]) == (
        canonical_offsets[:BAT_ROOT_INDEX] + [BAT_ROOT]
        + canonical_offsets[BAT_ROOT_INDEX:])
    assert fsmash["root_joints"][BAT_ROOT_INDEX] == BAT_JOINT
    assert fsmash["root_bindings"][BAT_ROOT_INDEX] == BAT_BINDING
    # Same topology as Win3: a real source parent schedule.
    win3 = programs[0]
    assert fsmash["binding_parents"] == win3["binding_parents"]
    assert set(fsmash["cross_slots"]) == {native.PACKED_GX_SLOT_CURRENT}

    emitted = "\n".join(native.render_p2_owner_runtime_program(
        dict(context, root_programs=programs)))
    suffix = "" if detail == "high" else "Low"
    for array in ("Roots", "CrossPaletteSlots"):
        assert "sNdsNativeNessFSmash%s%s" % (array, suffix) in emitted
    assert ("sNdsNativeNessFSmashBindingParents" in emitted) == (
        detail == "high")


@pytest.mark.parametrize("victim", ("high", "low"))
def test_bat_root_dropped_from_either_detail_fails(victim):
    """Remove the bat's bake for ONE detail: the generator refuses that detail
    alone (control first)."""
    for detail in ("high", "low"):
        context = native.build_p2_owner_runtime_context(SOURCE, "ness", detail)
        assert "FSmash" in [p["name"] for p in
                            native.build_owner_root_programs(SOURCE, context)]
    saved = native.P2_ROOT_PROGRAM_APPENDIX["ness"][victim]
    native.P2_ROOT_PROGRAM_APPENDIX["ness"][victim] = tuple(
        row for row in saved if row[1] != BAT_ROOT)
    try:
        context = native.build_p2_owner_runtime_context(SOURCE, "ness", victim)
        with pytest.raises(ValueError, match="0x6b50"):
            native.build_owner_root_programs(SOURCE, context)
        other = "low" if victim == "high" else "high"
        context = native.build_p2_owner_runtime_context(SOURCE, "ness", other)
        assert "FSmash" in [p["name"] for p in
                            native.build_owner_root_programs(SOURCE, context)]
    finally:
        native.P2_ROOT_PROGRAM_APPENDIX["ness"][victim] = saved
