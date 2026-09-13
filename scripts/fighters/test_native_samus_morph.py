"""Samus's source morph family replaces the full body with one joint."""
import re
from pathlib import Path

import pytest

import generate_nds_native_owners as native


SOURCE = native._paths.REPO_ROOT
RUNTIME = Path(__file__).resolve().parents[2]


@pytest.mark.parametrize("detail", ("high", "low"))
def test_morph_programs_reuse_source_geometry_and_live_joint(detail):
    context = native.build_p2_owner_runtime_context(SOURCE, "samus", detail)
    programs = native.build_owner_root_programs(SOURCE, context)
    assert [p["name"] for p in programs] == ["Catch", "MorphUnfold", "MorphBall"]
    assert len(programs[0]["roots"]) == 21
    for program, offset in zip(programs[1:], (0x8158, 0x8708)):
        assert program["root_offsets"] == (offset,)
        assert program["roots"] == [next(row for row in context["roots"] if row[0] == offset)]
        assert program["binding_parents"] == (255,)
        assert program["cross_slots"] == (native.PACKED_GX_SLOT_CURRENT,)
        emitted = "\n".join(native.render_p2_owner_runtime_program(
            dict(context, root_programs=programs)))
        assert "sNdsNativeSamus" + program["name"] + "Roots" in emitted


def test_all_source_morph_siblings_share_two_programs_and_restore():
    source = (SOURCE / "decomp/BattleShip-main/decomp/src/relocData/216_SamusMainMotion.c").read_text()
    scripts = dict(re.findall(r"ftMotionCommand dSamusMainMotion_(\w+)\[\] = \{(.*?)\n\};", source, re.S))
    helpers = ("0x0000", "0x0044", "0x005C", "0x0074")
    assert "ftMotionCommandHideModelPartAll()" in scripts[helpers[0]]
    for name, modelpart in zip(helpers[:3], (1, 2, 1)):
        assert re.findall(r"ftMotionCommandSetModelPartID\((\d+), (\d+)\)", scripts[name]) == [("6", str(modelpart))]
    assert "ftMotionCommandResetModelPartAll()" in scripts[helpers[3]]
    callers = {
        name: tuple(re.findall(r"ftMotionCommandSubroutine\(dSamusMainMotion_(0x0000|0x0044|0x005C|0x0074)\)", body))
        for name, body in scripts.items()
        if "ftMotionCommandSubroutine(dSamusMainMotion_0x0000)" in body
    }
    assert callers == {name: helpers for name in (
        "0x0484", "CliffEscapeQuick2", "CliffEscapeSlow2", "Bomb", "0x1E6C")}
    for roll in ("RollF", "RollB"):
        assert "ftMotionCommandSetParallelScript(dSamusMainMotion_0x0484)" in scripts[roll]


def test_runtime_has_each_complete_program_and_topology():
    assets = (RUNTIME / "src/nds/nds_renderer_assets.c").read_text()
    common = (RUNTIME / "src/nds/nds_renderer_native_common.c").read_text()
    for program_id, name in ((2, "MorphUnfold"), (3, "MorphBall")):
        assert re.search(r"program == " + str(program_id) + r"u\)\).*?return.*?&sNdsNativeSamus" + name + r"LowOwner.*?&sNdsNativeSamus" + name + r"HighOwner", assets, re.S)
        for suffix in ("BindingParents", "CrossPaletteSlots"):
            assert re.search(r"ndsRendererNativeFighterRootProgram\(slot\) == " + str(program_id) + r"u\)\s*\{[^{}]*return sNdsNativeSamus" + name + suffix + r";", common)
