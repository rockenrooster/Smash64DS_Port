#!/usr/bin/env python3
"""Offline catch-swirl packet checks for generate_nds_entry_effects.py.

Compiles the actual SHA-pinned EFCommonEffects2 (84) catch-swirl DLs at
0x2500/0x2588/0x2610/0x2698, verifies exact roots, triangles, the I4 -> A5I3
texture conversion, palettes, combine/material state, and existing corpus
prefix stability. Uses only temp output; never writes
src/nds/nds_entry_effects.generated.inc. Main regenerates with:
python scripts/3d_vfx/generate_nds_entry_effects.py
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
EXPECTED_ROOTS = (0x2500, 0x2588, 0x2610, 0x2698)
EXPECTED_COMBINE = (0xFCFF97FF, 0xFF2DFEFF)
EXPECTED_PALETTE = (0x7FFF,) * 8
EXPECTED_POSITIONS = {(0, -211, 73), (0, 11, -78), (0, 235, 72)}


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


def expected_i4_texels(resource: census.O2RResource) -> bytes:
    """Recompute the A5I3 payload straight from source nibbles.

    Independent of Compiler internals except the pinned 32x16 BLOCK layout:
    Nibble-packed bytes read direct. Source intensity supplies alpha only;
    RGB remains the primitive color, so the DS palette index stays white.
    """
    image_offset = 0x21B8
    width, height = 32, 16
    out = bytearray(width * height)
    for y in range(height):
        for x in range(width):
            si = y * width + x
            packed = resource.payload[image_offset + (si >> 1)]
            nibble = (packed >> 4) if not (si & 1) else (packed & 0xF)
            alpha = nibble * 0x11
            a5 = (alpha * 31 + 127) // 255
            out[y * width + x] = a5 << 3
    return bytes(out)


def main() -> None:
    if gen.CATCH.file_id != 84:
        raise SystemExit(f"catch file id {gen.CATCH.file_id} != 84")
    if gen.CATCH.sha256 != EXPECTED_SHA:
        raise SystemExit("catch source SHA drifted from pinned EFCommonEffects2")
    if tuple(gen.CATCH_ROOTS) != EXPECTED_ROOTS:
        raise SystemExit(f"catch roots {gen.CATCH_ROOTS!r} != {EXPECTED_ROOTS!r}")

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

    if len(catch.groups) != 4:
        raise SystemExit(f"catch groups {len(catch.groups)} != 4")
    catch_tris = sum(len(g.corners) // 3 for g in catch.groups)
    if catch_tris != 4:
        raise SystemExit(f"catch triangles {catch_tris} != 4")

    # One shared I4 texture, not one per root.
    catch_tex = list(catch.textures.values())
    if len(catch_tex) != 1:
        raise SystemExit(f"catch textures {len(catch_tex)} != 1")
    texture = catch_tex[0]
    if texture.ds_format != gen.TEX_A5I3:
        raise SystemExit("catch texture is not one A5I3 conversion")
    if texture.key is None:
        raise SystemExit("catch group lost its texture key")
    key = texture.key
    if (key.image_asset, key.image_offset) != (84, 0x21B8):
        raise SystemExit(f"catch image {key.image_asset}@0x{key.image_offset:x} != 84@0x21b8")
    if (key.fmt, key.size) != (gen.FMT_I, gen.SIZ_4B):
        raise SystemExit(f"catch format {key.fmt}/{key.size} != I4")
    if (key.width, key.height, key.upload_width, key.upload_height) != (32, 16, 32, 16):
        raise SystemExit("catch dimensions drifted from 32x16 source tile")
    if (key.cms, key.cmt, key.masks, key.maskt) != (2, 2, 5, 4):
        raise SystemExit("catch wrap/mask drifted from sourced tile state")
    if len(texture.texels) != 32 * 16:
        raise SystemExit(f"catch texels {len(texture.texels)} != 512 A5I3 bytes")
    if tuple(texture.palette) != EXPECTED_PALETTE:
        raise SystemExit(f"catch palette {tuple(texture.palette)!r} would alter source primitive RGB")
    # Exact texel inspection, not just counts: recompute from source nibbles
    # and pin two lane-sensitive endpoints (transparent 0x00, dense 0xE8).
    expected = expected_i4_texels(resources[gen.CATCH.file_id])
    if texture.texels != expected:
        raise SystemExit("catch texels drifted from source I4 nibbles")
    if texture.texels[0] != 0x00 or texture.texels[1] != 0xE8:
        raise SystemExit(
            f"catch texel endpoints {texture.texels[0]:#x}/{texture.texels[1]:#x} "
            "!= 0x0/0xe8; nibble lane or I4 mapping drifted"
        )
    if 0xF8 not in texture.texels:
        raise SystemExit("catch texels lost graded alpha (no full-density texel)")
    alphas = {byte >> 3 for byte in texture.texels}
    if len(alphas) < 8:
        raise SystemExit("catch alpha collapsed instead of preserving I gradation")
    if any(texture.palette[texel & 7] != 0x7FFF for texel in texture.texels):
        raise SystemExit("texture intensity would incorrectly modulate live primitive RGB")
    saved_combine = catch.combine_w0, catch.combine_w1
    catch.combine_w0 = 0
    try:
        catch.group_state(catch_base)
    except SystemExit:
        pass
    else:
        raise SystemExit("an incompatible combine reused the cached I4 texture")
    catch.combine_w0, catch.combine_w1 = saved_combine

    positions: set[tuple[int, int, int]] = set()
    for group in catch.groups:
        state = group.state
        if len(group.corners) != 3:
            raise SystemExit("catch group is not one triangle")
        # Segment-E material slot identities: each root selects slot 0 via the
        # 0x0E000000 branch; live MObj/animation state is never baked.
        if state.material_slot != 0:
            raise SystemExit(
                f"catch material slot {state.material_slot} != 0; "
                "segment-E branch identity drifted"
            )
        if state.texture_key is None:
            raise SystemExit("catch group lost its texture reference")
        if (state.combine_w0, state.combine_w1) != EXPECTED_COMBINE:
            raise SystemExit("catch combine drifted from sourced DL")
        if state.prim_color != 0xFFFFFFFF or state.env_color != 0xFFFFFFFF:
            raise SystemExit("catch baked live prim/env; MatAnimJoint owns color")
        if state.color_write_mask != 0:
            raise SystemExit("catch claimed a color write the DL never made")
        if (state.othermode_h, state.othermode_l) != (0, 0):
            raise SystemExit("catch baked othermode the DL never wrote")
        if (state.othermode_h_mask, state.othermode_l_mask) != (0, 0):
            raise SystemExit("catch claimed othermode writes the DL never made")
        if not (state.geometry_clear & 0x00020000) or (state.geometry_mode & 0x00020000):
            raise SystemExit("catch lighting is not cleared/unlit as sourced")
        if state.light_mask != 0:
            raise SystemExit("catch carries baked light colors")
        if (state.texture_scale_s, state.texture_scale_t) != (0xFFFF, 0xFFFF):
            raise SystemExit("catch UV scale drifted from sourced G_TEXTURE")
        for corner in group.corners:
            positions.add((corner.x, corner.y, corner.z))
    if positions != EXPECTED_POSITIONS:
        raise SystemExit(f"catch positions {sorted(positions)!r} != source Vtx")
    first = [(v.x, v.y, v.z, v.s, v.t) for v in catch.groups[0].corners]
    if first != [(0, 235, 72, 511, 0), (0, 11, -78, 338, 251), (0, -211, 73, 0, 1)]:
        raise SystemExit(f"catch root 0x2500 UV/positions drifted: {first!r}")

    # Backward-compatible emit plus extended emit, via temp output only.
    old_text = gen.emit(*existing, shield, reflector)
    new_text = gen.emit(*existing, shield, reflector, catch)
    with tempfile.TemporaryDirectory() as tmp:
        old_path = Path(tmp) / "entry_old.inc"
        new_path = Path(tmp) / "entry_new.inc"
        old_path.write_text(old_text, encoding="ascii")
        new_path.write_text(new_text, encoding="ascii")
        old_back = old_path.read_text(encoding="ascii")
        new_back = new_path.read_text(encoding="ascii")

    if "#define NDS_ENTRY_EFFECT_ROOT_COUNT 31u" not in old_back:
        raise SystemExit("old emit root count drifted from 31")
    if "#define NDS_ENTRY_EFFECT_GROUP_COUNT 73u" not in old_back:
        raise SystemExit("old emit group count drifted from 73")
    if "#define NDS_ENTRY_EFFECT_ROOT_COUNT 35u" not in new_back:
        raise SystemExit("new emit root count != 35")
    if "#define NDS_ENTRY_EFFECT_GROUP_COUNT 77u" not in new_back:
        raise SystemExit("new emit group count != 77")
    for token in (
        "#define NDS_ENTRY_EFFECT_CATCH_ROOT_FIRST 31u",
        "#define NDS_ENTRY_EFFECT_CATCH_ROOT_COUNT 4u",
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
    if len(old_roots) != 31 or len(new_roots) != 35:
        raise SystemExit(f"root rows old={len(old_roots)} new={len(new_roots)}")
    if old_roots != new_roots[:31]:
        raise SystemExit("existing 31 root rows are not a stable prefix")
    for row, root in zip(new_roots[31:], EXPECTED_ROOTS):
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
    if len(old_groups) != 73:
        raise SystemExit(f"old group rows {len(old_groups)} != 73")
    if len(new_groups) != 77:
        raise SystemExit(f"new group rows {len(new_groups)} != 77")
    if old_groups != new_groups[:73]:
        raise SystemExit("existing 73 group rows are not a stable prefix")

    print(
        "NATIVE_CATCH_SWIRL_PACKET_OK "
        "catch_asset=84@0x2500,0x2588,0x2610,0x2698 groups=4 triangles=4 "
        "format=A5I3 texels=512 palette=white8 "
        "combine=0xfcff97ff/0xff2dfeff material_slots=0,0,0,0 "
        "roots=35 groups=77 prefix_stable=31/73 "
        "regen=python scripts/3d_vfx/generate_nds_entry_effects.py"
    )


if __name__ == "__main__":
    main()
