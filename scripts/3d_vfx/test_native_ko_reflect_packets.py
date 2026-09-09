#!/usr/bin/env python3
"""Offline KO-blast / shield-break packet checks for generate_nds_entry_effects.py.

Compiles the actual SHA-pinned EFCommonEffects2 (84) ordinary VS KO
DeadExplode DLs at 0x5218/0x52B0/0x5310 (0x5240 is nested inside 0x5218, not a
top-level root) and the ReflectBreak DLs at 0x31D0/0x3258/0x32E0, verifies
exact roots, triangles, IA8 -> A5I3 texture bytes, palettes, combine/material
state, typed-source DObjDLLink bindings, and existing corpus prefix stability.
Uses only temp output; never writes src/nds/nds_entry_effects.generated.inc.
Main regenerates with:
python scripts/3d_vfx/generate_nds_entry_effects.py

Source KO uses a PRIM/ENV texture blend (0xFC309661/0x552EFF7F) with
per-player env colors stamped by efManagerDeadExplodeMakeEffect
(MOBJ_FLAG_ENVCOLOR) and per-player MatAnimJoints (DeadExplode1..4). Source
ReflectBreak uses combine 0xFC121624/0xFF2FFFFF with a MatAnimJoint prim
white->transparent ramp and a 0xACE0FF light-1 ramp. The baked A5I3 grayscale
ramps below preserve coverage/intensity only and do NOT render those colors
by themselves: native runtime work required is per-group live prim/env/light
modulation driven by the write masks (KO: env-only 0x2, ReflectBreak: none),
which Main owns with the C runtime/admission and palette handling.
"""

from __future__ import annotations

import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))
sys.path.insert(0, str(ROOT / "scripts/3d_vfx"))

import generate_battle_playable_texture_census as census
import generate_nds_entry_effects as gen

EXPECTED_SHA = (
    "bcdcb4c90323f5ddbfc1e24813382f4c7cb974437328bb5b2225ca4b3089a88a"
)
EXPECTED_KO_ROOTS = (0x5218, 0x52B0, 0x5310)
EXPECTED_RB_ROOTS = (0x31D0, 0x3258, 0x32E0)
# 0x5240 is the nested drawable inside the 0x5218 wrapper, never top-level.
NESTED_KO_DL = 0x5240
EXPECTED_KO_COMBINE = (0xFC309661, 0x552EFF7F)
EXPECTED_RB_COMBINE = (0xFC121624, 0xFF2FFFFF)
# Generic IA8 -> A5I3 grayscale ramp. This is coverage/intensity only: the
# live prim/env/light colors above stay runtime-owned (see module docstring).
EXPECTED_GRAY_PALETTE = (0, 4228, 8456, 13741, 17969, 23254, 27482, 32767)
EXPECTED_KO_ENVS = (0x5136FFFF, 0xFCF690FF, 0x00FF00FF)

# First-triangle spot checks: exact decoded positions plus final baked UVs.
EXPECTED_KO_FIRST = [
    [(180, 300, 0, 1023, 1023), (-180, 300, 0, 0, 1023), (-180, 0, 0, 0, 0)],
    [(240, 351, 0, 511, 1023), (-240, 351, 0, 0, 1023), (-240, 0, 0, 0, 0)],
    [(99, 0, -99, 276, 0), (-99, 0, -99, 660, 0), (150, 300, -150, 276, 512)],
]
EXPECTED_RB_FIRST = [
    [(-43, 267, 0, 0, 1023), (-224, -111, 0, 0, 0), (76, -111, 0, 511, 0)],
    [(211, -252, 0, 511, 511), (150, 150, 0, 511, 0), (98, -321, 0, 0, 511)],
    [(150, 150, 0, 511, 511), (-150, 150, 0, 0, 511), (-150, -150, 0, 0, 0)],
]


def load_all() -> dict[int, census.O2RResource]:
    specs = (
        gen.MARIO, gen.FOX, gen.DONKEY, gen.SAMUS, gen.CAPTAIN,
        gen.LINK_SPECIAL2, gen.LINK_MODEL, gen.LINK_SPECIAL3,
        gen.EXTERN109, gen.SHIELD, gen.REFLECTOR, gen.CATCH,
    )
    return {s.file_id: census.load_o2r(ROOT, s) for s in specs}


def compile_existing(resources: dict[int, census.O2RResource]):
    mario = gen.Compiler(resources[gen.MARIO.file_id], resources)
    mario.compile_roots(gen.MARIO_ROOTS, 0)
    fox = gen.Compiler(resources[gen.FOX.file_id], resources)
    fox.compile_roots(gen.FOX_ROOTS, len(gen.MARIO_ROOTS))
    donkey = gen.Compiler(resources[gen.DONKEY.file_id], resources)
    donkey.compile_roots(gen.DONKEY_ROOTS, len(gen.MARIO_ROOTS) + len(gen.FOX_ROOTS))
    samus = gen.Compiler(resources[gen.SAMUS.file_id], resources)
    samus.compile_roots(
        gen.SAMUS_ROOTS,
        len(gen.MARIO_ROOTS) + len(gen.FOX_ROOTS) + len(gen.DONKEY_ROOTS),
    )
    captain = gen.Compiler(resources[gen.CAPTAIN.file_id], resources)
    captain.compile_roots(
        gen.CAPTAIN_ROOTS,
        len(gen.MARIO_ROOTS) + len(gen.FOX_ROOTS) + len(gen.DONKEY_ROOTS) + len(gen.SAMUS_ROOTS),
    )
    link_special2 = gen.Compiler(resources[gen.LINK_SPECIAL2.file_id], resources)
    link_special2.compile_roots(
        gen.LINK_SPECIAL2_ROOTS,
        len(gen.MARIO_ROOTS) + len(gen.FOX_ROOTS) + len(gen.DONKEY_ROOTS) +
        len(gen.SAMUS_ROOTS) + len(gen.CAPTAIN_ROOTS),
    )
    link_model = gen.Compiler(resources[gen.LINK_MODEL.file_id], resources)
    link_model.compile_roots(
        gen.LINK_MODEL_SPIN_ROOTS,
        len(gen.MARIO_ROOTS) + len(gen.FOX_ROOTS) + len(gen.DONKEY_ROOTS) +
        len(gen.SAMUS_ROOTS) + len(gen.CAPTAIN_ROOTS) + len(gen.LINK_SPECIAL2_ROOTS),
    )
    link_special3 = gen.Compiler(resources[gen.LINK_SPECIAL3.file_id], resources)
    link_special3.compile_roots(
        gen.LINK_SPECIAL3_ROOTS,
        len(gen.MARIO_ROOTS) + len(gen.FOX_ROOTS) + len(gen.DONKEY_ROOTS) +
        len(gen.SAMUS_ROOTS) + len(gen.CAPTAIN_ROOTS) + len(gen.LINK_SPECIAL2_ROOTS) +
        len(gen.LINK_MODEL_SPIN_ROOTS),
    )
    return mario, fox, donkey, samus, captain, link_special2, link_model, link_special3


def check_typed_sources() -> None:
    """Verify the new roots are the actual DObjDLLink-submitted drawables."""
    efmanager = (
        ROOT / "decomp/BattleShip-main/decomp/src/ef/efmanager.c"
    ).read_text(encoding="utf-8")
    for token in (
        "EFDesc dEFManagerDeadExplodeEffectDesc",
        "EFDesc dEFManagerReflectBreakEffectDesc",
        "dEFManagerDeadExplodeMatAnimJoints[player]",
        "MOBJ_FLAG_ENVCOLOR",
    ):
        if token not in efmanager:
            raise SystemExit(f"efmanager.c lost live-color owner {token!r}")
    asset84 = (
        ROOT / "decomp/BattleShip-main/decomp/src/relocData/84_EFCommonEffects2.c"
    ).read_text(encoding="utf-8")
    for root in EXPECTED_KO_ROOTS + EXPECTED_RB_ROOTS:
        token = "{ 1, dEFCommonEffects2_DL_0x%04X }" % root
        if token not in asset84:
            raise SystemExit(
                f"asset 84 typed source has no DObjDLLink binding for 0x{root:x}"
            )
    for token in (
        "dEFCommonEffects2_DeadExplodeDefaultDObjDesc",
        "dEFCommonEffects2_ReflectBreakDObjDesc",
    ):
        if token not in asset84:
            raise SystemExit(f"asset 84 typed source lost scene graph {token!r}")


def check_graded_alpha(texels: bytes, label: str) -> None:
    if 0xF8 in texels:
        raise SystemExit(f"{label} texels lost graded alpha to full-density 0xf8")
    alphas = {byte >> 3 for byte in texels}
    if len(alphas) < 8:
        raise SystemExit(f"{label} alpha collapsed instead of preserving gradation")


def main() -> None:
    if gen.CATCH.file_id != 84:
        raise SystemExit(f"effect file id {gen.CATCH.file_id} != 84")
    if gen.CATCH.sha256 != EXPECTED_SHA:
        raise SystemExit("effect source SHA drifted from pinned EFCommonEffects2")
    if tuple(gen.KO_ROOTS) != EXPECTED_KO_ROOTS:
        raise SystemExit(f"KO roots {gen.KO_ROOTS!r} != {EXPECTED_KO_ROOTS!r}")
    if tuple(gen.REFLECTBREAK_ROOTS) != EXPECTED_RB_ROOTS:
        raise SystemExit(f"ReflectBreak roots {gen.REFLECTBREAK_ROOTS!r} != {EXPECTED_RB_ROOTS!r}")
    if NESTED_KO_DL in tuple(gen.KO_ROOTS):
        raise SystemExit("nested KO DL 0x5240 must not be a top-level root")
    check_typed_sources()

    resources = load_all()
    existing = compile_existing(resources)

    shield_base = (len(gen.MARIO_ROOTS) + len(gen.FOX_ROOTS) + len(gen.DONKEY_ROOTS) +
                   len(gen.SAMUS_ROOTS) + len(gen.CAPTAIN_ROOTS) +
                   len(gen.LINK_SPECIAL2_ROOTS) + len(gen.LINK_MODEL_SPIN_ROOTS) +
                   len(gen.LINK_SPECIAL3_ROOTS))
    if shield_base != 29:
        raise SystemExit(f"shield base {shield_base} != 29; prior ordinals drifted")
    shield = gen.Compiler(resources[gen.SHIELD.file_id], resources)
    shield.compile_roots(gen.SHIELD_ROOTS, shield_base)
    reflector = gen.Compiler(resources[gen.REFLECTOR.file_id], resources)
    reflector.compile_roots(gen.REFLECTOR_ROOTS, shield_base + len(gen.SHIELD_ROOTS))
    catch_base = shield_base + len(gen.SHIELD_ROOTS) + len(gen.REFLECTOR_ROOTS)
    if catch_base != 31:
        raise SystemExit(f"catch base {catch_base} != 31; prior ordinals drifted")
    catch = gen.Compiler(resources[gen.CATCH.file_id], resources)
    catch.compile_roots(gen.CATCH_ROOTS, catch_base)
    ko_base = catch_base + len(gen.CATCH_ROOTS)
    if ko_base != 35:
        raise SystemExit(f"KO base {ko_base} != 35; prior ordinals drifted")
    ko = gen.Compiler(resources[gen.CATCH.file_id], resources)
    ko.compile_roots(gen.KO_ROOTS, ko_base)
    rb_base = ko_base + len(gen.KO_ROOTS)
    if rb_base != 38:
        raise SystemExit(f"ReflectBreak base {rb_base} != 38; prior ordinals drifted")
    reflectbreak = gen.Compiler(resources[gen.CATCH.file_id], resources)
    reflectbreak.compile_roots(gen.REFLECTBREAK_ROOTS, rb_base)

    # The 0x5218 wrapper's texture (84@0x4708) is only reachable through the
    # nested 0x5240 call: compiling 0x5218 alone must already carry it.
    nested = gen.Compiler(resources[gen.CATCH.file_id], resources)
    nested.compile_roots((0x5218,), 900)
    nested_keys = {(k.image_asset, k.image_offset) for k in nested.textures}
    if (84, 0x4708) not in nested_keys:
        raise SystemExit("KO 0x5218 lost its nested 0x5240 texture reference")

    # KO groups: 2 + 2 + 8 triangles sharing one ordered RSP state.
    if len(ko.groups) != 3:
        raise SystemExit(f"KO groups {len(ko.groups)} != 3")
    ko_tris = sum(len(g.corners) // 3 for g in ko.groups)
    if ko_tris != 12:
        raise SystemExit(f"KO triangles {ko_tris} != 12")
    ko_tex = list(ko.textures.values())
    if len(ko_tex) != 3:
        raise SystemExit(f"KO textures {len(ko_tex)} != 3")
    for key, width, height, texel_len, first8 in (
        ((84, 0x4708), 64, 64, 4096, [16] * 8),
        ((84, 0x3F00), 32, 64, 2048, [0, 0, 0, 0, 0, 64, 153, 137]),
        ((84, 0x3AF8), 32, 32, 1024, [255] * 8),
    ):
        matches = [t for t in ko_tex
                   if (t.key.image_asset, t.key.image_offset) == key]
        if len(matches) != 1:
            raise SystemExit(f"KO texture 0x{key[1]:x} missing or duplicated")
        texture = matches[0]
        if texture.ds_format != gen.TEX_A5I3:
            raise SystemExit(f"KO texture 0x{key[1]:x} is not one A5I3 conversion")
        if (texture.key.fmt, texture.key.size) != (gen.FMT_IA, gen.SIZ_8B):
            raise SystemExit(f"KO texture 0x{key[1]:x} is not source IA8")
        if (texture.key.width, texture.key.height) != (width, height):
            raise SystemExit(f"KO texture 0x{key[1]:x} dimensions drifted")
        if len(texture.texels) != texel_len:
            raise SystemExit(f"KO texture 0x{key[1]:x} texel bytes drifted")
        if tuple(texture.palette) != EXPECTED_GRAY_PALETTE:
            raise SystemExit(f"KO texture 0x{key[1]:x} palette drifted")
        if list(texture.texels[:8]) != first8:
            raise SystemExit(f"KO texture 0x{key[1]:x} leading bytes drifted")
        check_graded_alpha(texture.texels, f"KO 0x{key[1]:x}")
    for index, group in enumerate(ko.groups):
        state = group.state
        if state.material_slot != 0:
            raise SystemExit(
                f"KO group {index} material slot {state.material_slot} != 0; "
                "segment-E branch identity drifted"
            )
        if (state.combine_w0, state.combine_w1) != EXPECTED_KO_COMBINE:
            raise SystemExit(f"KO group {index} combine drifted from sourced DL")
        if state.prim_color != 0xFFFFFFFF:
            raise SystemExit(f"KO group {index} baked live prim; MatAnim owns color")
        if state.env_color != EXPECTED_KO_ENVS[index]:
            raise SystemExit(
                f"KO group {index} env 0x{state.env_color:08x} != "
                f"0x{EXPECTED_KO_ENVS[index]:08x}"
            )
        if state.color_write_mask != gen.COLOR_WRITE_ENV:
            raise SystemExit(
                f"KO group {index} color mask 0x{state.color_write_mask:x} "
                "!= env-only 0x2; runtime must take live per-player env"
            )
        if (state.othermode_h_mask, state.othermode_l_mask) != (0, 0):
            raise SystemExit(
                f"KO group {index} claimed othermode writes the DL never made"
            )
        if not (state.geometry_clear & 0x00020000) or (state.geometry_mode & 0x00020000):
            raise SystemExit(f"KO group {index} lighting is not cleared/unlit")
        if state.light_mask != 0:
            raise SystemExit(f"KO group {index} carries baked light colors")
        if len(group.matrix_roots) != len(group.corners):
            raise SystemExit(f"KO group {index} matrix provenance drifted")
        if set(group.matrix_roots) != {ko_base + index}:
            raise SystemExit(f"KO group {index} corners escaped their own matrix root")
        first = [(v.x, v.y, v.z, v.s, v.t) for v in group.corners[:3]]
        if first != EXPECTED_KO_FIRST[index]:
            raise SystemExit(f"KO group {index} UV/positions drifted: {first!r}")
    if (ko.groups[2].state.texture_scale_s, ko.groups[2].state.texture_scale_t) != (0xFFFF, 0x8000):
        raise SystemExit("KO 0x5310 UV scale drifted from sourced G_TEXTURE")
    if ko.groups[2].state.texture_origin_s != 0xC0:
        raise SystemExit("KO 0x5310 texture origin drifted from sourced tile")

    # ReflectBreak groups: three 2-triangle shards, no baked colors at all.
    if len(reflectbreak.groups) != 3:
        raise SystemExit(f"ReflectBreak groups {len(reflectbreak.groups)} != 3")
    rb_tris = sum(len(g.corners) // 3 for g in reflectbreak.groups)
    if rb_tris != 6:
        raise SystemExit(f"ReflectBreak triangles {rb_tris} != 6")
    rb_tex = list(reflectbreak.textures.values())
    if len(rb_tex) != 2:
        raise SystemExit(
            f"ReflectBreak textures {len(rb_tex)} != 2 "
            "(32x64 clamp-materialized plus shared 32x32)"
        )
    for texture in rb_tex:
        if texture.ds_format != gen.TEX_A5I3:
            raise SystemExit("ReflectBreak texture is not one A5I3 conversion")
        if (texture.key.image_asset, texture.key.image_offset) != (84, 0x2B78):
            raise SystemExit("ReflectBreak image drifted from 84@0x2b78")
        if (texture.key.fmt, texture.key.size) != (gen.FMT_IA, gen.SIZ_8B):
            raise SystemExit("ReflectBreak texture is not source IA8")
        if tuple(texture.palette) != EXPECTED_GRAY_PALETTE:
            raise SystemExit("ReflectBreak palette drifted from grayscale ramp")
        if list(texture.texels[:8]) != [7, 7, 7, 7, 7, 23, 55, 103]:
            raise SystemExit("ReflectBreak leading texel bytes drifted")
        check_graded_alpha(texture.texels, "ReflectBreak 0x2b78")
    tall = [t for t in rb_tex if (t.key.width, t.key.height) == (32, 64)]
    small = [t for t in rb_tex if (t.key.width, t.key.height) == (32, 32)]
    if len(tall) != 1 or len(small) != 1:
        raise SystemExit("ReflectBreak texture dimensions drifted from 32x64/32x32")
    if (tall[0].key.cms, tall[0].key.cmt) != (3, 2):
        raise SystemExit("ReflectBreak 0x31d0 wrap drifted from sourced tile")
    if (small[0].key.cms, small[0].key.cmt) != (2, 2):
        raise SystemExit("ReflectBreak 0x3258/0x32e0 wrap drifted from sourced tile")
    # Clamp-materialized rows repeat the loaded extent instead of reading past
    # the 1024-byte source image into the following MObjSub data.
    if tall[0].texels[1024:1032] != tall[0].texels[0:8]:
        raise SystemExit("ReflectBreak clamp rows escaped the loaded source extent")
    for index, group in enumerate(reflectbreak.groups):
        state = group.state
        if state.material_slot != 0:
            raise SystemExit(
                f"ReflectBreak group {index} material slot "
                f"{state.material_slot} != 0"
            )
        if state.texture_key is None:
            raise SystemExit(f"ReflectBreak group {index} lost its texture reference")
        if (state.combine_w0, state.combine_w1) != EXPECTED_RB_COMBINE:
            raise SystemExit(f"ReflectBreak group {index} combine drifted")
        if state.prim_color != 0xFFFFFFFF or state.env_color != 0xFFFFFFFF:
            raise SystemExit(
                f"ReflectBreak group {index} baked live prim/env; "
                "MatAnimJoint owns color"
            )
        if state.color_write_mask != 0:
            raise SystemExit(
                f"ReflectBreak group {index} claimed a color write the DL never made"
            )
        if (state.othermode_h_mask, state.othermode_l_mask) != (0, 0):
            raise SystemExit(
                f"ReflectBreak group {index} claimed othermode writes the DL never made"
            )
        if not (state.geometry_clear & 0x00020000) or (state.geometry_mode & 0x00020000):
            raise SystemExit(f"ReflectBreak group {index} lighting is not cleared/unlit")
        if state.light_mask != 0:
            raise SystemExit(f"ReflectBreak group {index} carries baked light colors")
        if set(group.matrix_roots) != {rb_base + index}:
            raise SystemExit(
                f"ReflectBreak group {index} corners escaped their own matrix root"
            )
        first = [(v.x, v.y, v.z, v.s, v.t) for v in group.corners[:3]]
        if first != EXPECTED_RB_FIRST[index]:
            raise SystemExit(f"ReflectBreak group {index} UV/positions drifted: {first!r}")
    if any(
        (g.state.texture_scale_s, g.state.texture_scale_t) != (0xFFFF, 0xFFFF)
        for g in reflectbreak.groups
    ):
        raise SystemExit("ReflectBreak UV scale drifted from sourced G_TEXTURE")

    # Backward-compatible emit plus extended emit, via temp output only.
    old_text = gen.emit(*existing, shield, reflector, catch)
    new_text = gen.emit(*existing, shield, reflector, catch, ko, reflectbreak)
    with tempfile.TemporaryDirectory() as tmp:
        old_path = Path(tmp) / "entry_old.inc"
        new_path = Path(tmp) / "entry_new.inc"
        old_path.write_text(old_text, encoding="ascii")
        new_path.write_text(new_text, encoding="ascii")
        old_back = old_path.read_text(encoding="ascii")
        new_back = new_path.read_text(encoding="ascii")

    if "#define NDS_ENTRY_EFFECT_ROOT_COUNT 35u" not in old_back:
        raise SystemExit("old emit root count drifted from 35")
    if "#define NDS_ENTRY_EFFECT_GROUP_COUNT 77u" not in old_back:
        raise SystemExit("old emit group count drifted from 77")
    if "#define NDS_ENTRY_EFFECT_ROOT_COUNT 41u" not in new_back:
        raise SystemExit("new emit root count != 41")
    if "#define NDS_ENTRY_EFFECT_GROUP_COUNT 83u" not in new_back:
        raise SystemExit("new emit group count != 83")
    for token in (
        "#define NDS_ENTRY_EFFECT_KO_ROOT_FIRST 35u",
        "#define NDS_ENTRY_EFFECT_KO_ROOT_COUNT 3u",
        "#define NDS_ENTRY_EFFECT_REFLECTBREAK_ROOT_FIRST 38u",
        "#define NDS_ENTRY_EFFECT_REFLECTBREAK_ROOT_COUNT 3u",
    ):
        if token not in new_back:
            raise SystemExit(f"new emit missing {token}")

    def root_table_rows(text: str) -> list[str]:
        in_roots = False
        rows: list[str] = []
        for line in text.splitlines():
            if "sNdsEntryEffectRoots[" in line:
                in_roots = True
                continue
            if in_roots:
                if line.strip().startswith("};"):
                    break
                if line.strip().startswith("{"):
                    rows.append(line.strip())
        return rows

    old_roots = root_table_rows(old_back)
    new_roots = root_table_rows(new_back)
    if len(old_roots) != 35 or len(new_roots) != 41:
        raise SystemExit(f"root rows old={len(old_roots)} new={len(new_roots)}")
    if old_roots != new_roots[:35]:
        raise SystemExit("existing 35 root rows are not a stable prefix")
    for row, root in zip(new_roots[35:], EXPECTED_KO_ROOTS + EXPECTED_RB_ROOTS):
        if f"{{ 0x{root:04x}u," not in row:
            raise SystemExit(f"appended root row {row!r} != 0x{root:04x}")

    def group_rows(text: str) -> list[str]:
        in_groups = False
        rows: list[str] = []
        for line in text.splitlines():
            if "sNdsEntryEffectGroups[" in line:
                in_groups = True
                continue
            if in_groups:
                if line.strip().startswith("};"):
                    break
                if line.strip().startswith("{"):
                    rows.append(line.strip())
        return rows

    old_groups = group_rows(old_back)
    new_groups = group_rows(new_back)
    if len(old_groups) != 77:
        raise SystemExit(f"old group rows {len(old_groups)} != 77")
    if len(new_groups) != 83:
        raise SystemExit(f"new group rows {len(new_groups)} != 83")
    if old_groups != new_groups[:77]:
        raise SystemExit("existing 77 group rows are not a stable prefix")

    def mask_table_rows(text: str, table: str, braced: bool) -> list[str]:
        in_table = False
        rows: list[str] = []
        for line in text.splitlines():
            if table + "[" in line:
                in_table = True
                continue
            if in_table:
                stripped = line.strip()
                if stripped.startswith("};"):
                    break
                if braced and stripped.startswith("{"):
                    rows.append(stripped)
                elif not braced and stripped.endswith("u,"):
                    rows.append(stripped)
        return rows

    # Per-group mask tables run in sNdsEntryEffectGroups order: 77 stable
    # entries, then three KO (env-only) and three ReflectBreak (inherited).
    for label, table, braced, tail in (
        ("color", "sNdsEntryEffectColorWriteMasks", False,
         ["2u,", "2u,", "2u,", "0u,", "0u,", "0u,"]),
        ("othermode", "sNdsEntryEffectOtherModeWriteMasks", True,
         ["{ 0x00000000u, 0x00000000u },"] * 6),
    ):
        old_rows = mask_table_rows(old_back, table, braced)
        new_rows = mask_table_rows(new_back, table, braced)
        if len(old_rows) != 77 or len(new_rows) != 83:
            raise SystemExit(
                f"{label} mask rows old={len(old_rows)} new={len(new_rows)}"
            )
        if old_rows != new_rows[:77]:
            raise SystemExit(f"existing 77 {label} mask rows are not a stable prefix")
        if new_rows[77:] != tail:
            raise SystemExit(
                f"appended {label} masks {new_rows[77:]!r} != {tail!r}"
            )

    print(
        "NATIVE_KO_REFLECT_PACKETS_OK "
        "ko_asset=84@0x5218,0x52b0,0x5310(nested:0x5240) groups=3 triangles=12 "
        "reflectbreak_asset=84@0x31d0,0x3258,0x32e0 groups=3 triangles=6 "
        "format=A5I3 palettes=gray8 "
        "ko_masks=color:0x2othermode:0x0/0x0 "
        "reflectbreak_masks=color:0x0othermode:0x0/0x0 "
        "roots=41 groups=83 prefix_stable=35/77/77 "
        "runtime_colors=live-per-player-env+matanim-prim "
        "regen=python scripts/3d_vfx/generate_nds_entry_effects.py"
    )


if __name__ == "__main__":
    main()
