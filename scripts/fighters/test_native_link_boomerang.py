"""Source Neutral-B siblings and the live mixed-owner topology must agree."""
import re
from pathlib import Path

import pytest

import generate_nds_native_owners as native


ROOT = Path(__file__).resolve().parents[2]


@pytest.fixture(scope="module", params=("high", "low"))
def link(request):
    context = native.build_p2_owner_runtime_context(ROOT, "link", request.param)
    if request.param == "low":
        # Production shares the union of both details' light preambles.
        high = native.build_p2_owner_runtime_context(ROOT, "link", "high")
        merged = list(high["light_preambles"])
        merged.extend(row for row in context["light_preambles"] if row not in merged)
        context["light_preamble_indices"] = [
            merged.index(context["light_preambles"][index])
            for index in context["light_preamble_indices"]]
        context["light_preambles"] = merged
    return context, native.build_owner_root_programs(ROOT, context)


def test_source_throw_empty_and_return_vectors(link):
    context, programs = link
    detail = context["detail"]
    payload = native.load_o2r_payload(ROOT, "link")
    source = (ROOT / "decomp/BattleShip-main/decomp/src/relocData/224_LinkMainMotion.c").read_text()
    admitted = {tuple(row[0] for row in context["roots"][:19])}
    admitted.update(program["root_offsets"] for program in programs)
    seen = set()
    for motion in ("MissingBoomerang_0x1D2C", "MissingBoomerang_0x1D88", "CatchingBoomerang"):
        body = re.search(r"dLinkMainMotion_" + motion + r"\[\] = \{(.*?)\n\};", source, re.S)[1]
        # These three source scripts contain only unnested US/JP conditionals.
        # Resolve them explicitly so JP trailer commands cannot become US states.
        body = re.sub(r"#if defined\(REGION_JP\).*?#endif", "", body, flags=re.S)
        body = body.replace("#if defined(REGION_US)", "").replace("#endif", "")
        assert "#" not in body
        events = []
        for command, args in re.findall(r"ftMotionCommand(SetModelPartID|WaitAsync|End)\((.*?)\)", body):
            if command == "SetModelPartID":
                events.append(tuple(map(int, args.split(","))))
                continue
            overrides = native._owner_root_program_overrides(ROOT, "link", detail, tuple(events))
            descriptors = native._owner_joint_descriptors(payload, "link", detail, overrides)[:-1]
            selected = native._owner_selected_descriptor_indices("link", len(descriptors))
            roots = tuple(descriptors[i][1] for i in selected if descriptors[i][1] is not None)
            assert roots in admitted, (detail, motion, command, roots)
            seen.add(roots)
    # The complete family needs the body, sword-on-back, and held-boomerang
    # programs. Catch/grapple is a different topology and stays independent.
    assert seen == {tuple(row[0] for row in context["roots"][:19]),
                    programs[0]["root_offsets"], programs[1]["root_offsets"]}


def test_specialn_runtime_uses_complete_topology(link):
    _, programs = link
    special = next(program for program in programs if program["name"] == "SpecialN")
    assert len(special["roots"]) == 20
    assert special["binding_parents"] == (255,) * 20
    assert special["source_owners"].count("linkboomerang") == 1
    assert special["source_owners"][5] == "linkboomerang"
    assert special["cross_slots"][5] == native.PACKED_GX_SLOT_CURRENT
    source = (ROOT / "src/nds/nds_renderer_native_common.c").read_text()
    # A missing dispatch case silently returns the 19-root canonical table;
    # owner selection alone therefore cannot establish valid matrix routing.
    for suffix in ("BindingParents", "CrossPaletteSlots"):
        match = re.search(r"if \(program == 3u\)\s*\{([^{}]*sNdsNativeLinkSpecialN" + suffix + r"[^{}]*)\}", source)
        assert match, f"SpecialN does not select its {suffix}"
        assert "*count =" in match[1]
        assert "return sNdsNativeLinkSpecialN" + suffix + ";" in match[1]


def test_catch_appendix_root_does_not_alias_canonical_palette_slot(link):
    context, programs = link
    catch = next(program for program in programs if program["name"] == "Catch")
    appendix_offset = 0x7F98
    appendix_index = catch["root_offsets"].index(appendix_offset)
    assert appendix_index == 11
    assert catch["cross_slots"][appendix_index] == native.PACKED_GX_SLOT_CURRENT
    assert catch["cross_slots"][14] == {"high": 20, "low": 16}[context["detail"]]
    physical = [slot for slot in catch["cross_slots"] if slot <= 30]
    assert len(physical) == native.DETAIL_GX_PLAN_COUNTS[context["detail"]]["link"][3]
    assert len(physical) == len(set(physical))


def test_program_cross_slot_validation_rejects_duplicate_and_invalid_slots():
    with pytest.raises(ValueError, match="physical slot 20 is not unique"):
        native._assert_owner_root_program_cross_slot_uniqueness(
            "link", "high", "Catch", (20, 20))
    with pytest.raises(ValueError, match="illegal GX palette slot 255"):
        native._assert_owner_root_program_cross_slot_uniqueness(
            "kirby", "high", "Stone", (native.INVALID_U8,))
