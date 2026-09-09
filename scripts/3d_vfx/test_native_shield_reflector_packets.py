#!/usr/bin/env python3
"""Offline shield/reflector packet checks for generate_nds_entry_effects.py.

Compiles the actual SHA-pinned FTManagerCommon (163) shield DL and FoxSpecial2
(346) reflector DL, verifies exact roots, triangles, texture formats, write
masks, and existing corpus prefix stability. Uses only temp output; never
writes src/nds/nds_entry_effects.generated.inc. Main regenerates with:
python scripts/3d_vfx/generate_nds_entry_effects.py
"""

from __future__ import annotations

import struct
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))
sys.path.insert(0, str(ROOT / "scripts/3d_vfx"))

import generate_battle_playable_texture_census as census
import generate_nds_entry_effects as gen


def load_all() -> dict[int, census.O2RResource]:
    specs = (
        gen.MARIO, gen.FOX, gen.DONKEY, gen.SAMUS, gen.CAPTAIN,
        gen.LINK_SPECIAL2, gen.LINK_MODEL, gen.LINK_SPECIAL3,
        gen.EXTERN109, gen.SHIELD, gen.REFLECTOR,
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


def synthetic_compiler(words: list[tuple[int, int]]) -> gen.Compiler:
    """Run one synthetic display list through a fresh Compiler.

    Each entry is a raw (w0, w1) command word pair; G_ENDDL terminates.
    No triangles are submitted, so masks are read back via group_state(),
    which snapshots the same cumulative compiler state G_TRI1 would capture.
    """
    payload = b"".join(struct.pack(">II", w0, w1) for w0, w1 in words)
    resource = census.O2RResource(
        spec=census.InputSpec("synthetic", "0" * 64, 9999),
        source=b"",
        payload=payload,
        source_sha256="0" * 64,
        payload_sha256="0" * 64,
        file_id=9999,
        data_offset=0,
        internal={},
        external={},
    )
    compiler = gen.Compiler(resource, {})
    compiler.walk(0, 0, ())
    return compiler


def merge_word(incoming: int, baked: int, mask: int) -> int:
    """Mirror of Main's runtime merge: (incoming & ~mask) | (baked & mask)."""
    return ((incoming & ~mask) & 0xFFFFFFFF) | (baked & mask)


def merge_color(incoming: int, baked: int, write_mask: int, bit: int) -> int:
    """Mirror of Main's runtime color select: incoming wins unless written."""
    return baked if write_mask & bit else incoming


def check_synthetic_write_masks() -> None:
    end = (0xDF000000, 0)
    # Mask helper unit checks, including the exact shield/reflector fields.
    if gen.othermode_write_mask(0xE2001E01) != 0x3:
        raise SystemExit("othermode_write_mask missed the shield L field")
    if gen.othermode_write_mask(0xE3001001) != 0xC000:
        raise SystemExit("othermode_write_mask missed the reflector H field")
    if gen.othermode_write_mask(0xE200001C) != 0xFFFFFFF8:
        raise SystemExit("othermode_write_mask missed the reflector L field")
    if gen.othermode_write_mask(0xE200001F) != 0xFFFFFFFF:
        raise SystemExit("othermode_write_mask missed a full-word write")
    try:
        gen.othermode_write_mask(0xE200FF1F)
    except ValueError:
        pass
    else:
        raise SystemExit("othermode_write_mask accepted an out-of-range write")

    seed_prim, seed_env = 0x11223344, 0x55667788
    seed_h, seed_l = 0xA5A5A5A5, 0x5A5A5A5A

    # Explicitly written white must beat a nonzero incoming seed, while an
    # unwritten default white must lose to it.  Both bake 0xFFFFFFFF, so only
    # the mask distinguishes them: this is the false-inheritance regression.
    written = synthetic_compiler([(0xFA000000, 0xFFFFFFFF), end]).group_state(0)
    inherited = synthetic_compiler([end]).group_state(0)
    if written.prim_color != 0xFFFFFFFF or inherited.prim_color != 0xFFFFFFFF:
        raise SystemExit("synthetic white prim did not bake 0xffffffff")
    if written.color_write_mask != gen.COLOR_WRITE_PRIM:
        raise SystemExit("explicit prim write left no color mask bit")
    if inherited.color_write_mask != 0:
        raise SystemExit("unwritten prim default claimed a color mask bit")
    if merge_color(seed_prim, written.prim_color, written.color_write_mask,
                   gen.COLOR_WRITE_PRIM) != 0xFFFFFFFF:
        raise SystemExit("explicit white lost to the incoming seed")
    if merge_color(seed_prim, inherited.prim_color, inherited.color_write_mask,
                   gen.COLOR_WRITE_PRIM) != seed_prim:
        raise SystemExit("inherited white overwrote the incoming seed")

    # Env bit is independent of prim.
    env = synthetic_compiler([(0xFB000000, 0x00112233), end]).group_state(0)
    if env.color_write_mask != gen.COLOR_WRITE_ENV:
        raise SystemExit(f"env write mask {env.color_write_mask:#x} != 0x2")
    if merge_color(seed_env, env.env_color, env.color_write_mask,
                   gen.COLOR_WRITE_ENV) != 0x00112233:
        raise SystemExit("written env lost to the incoming seed")
    if merge_color(seed_env, inherited.env_color, inherited.color_write_mask,
                   gen.COLOR_WRITE_ENV) != seed_env:
        raise SystemExit("inherited env overwrote the incoming seed")

    # Partial OtherMode update against a nonzero seed: only written bits win.
    partial = synthetic_compiler([(0xE2001807, 0xAA), end]).group_state(0)
    if (partial.othermode_h_mask, partial.othermode_l_mask) != (0, 0xFF):
        raise SystemExit("partial L write mask drifted from 0x0/0xff")
    if merge_word(seed_l, partial.othermode_l, partial.othermode_l_mask) != 0x5A5A5AAA:
        raise SystemExit("partial L merge did not preserve unwritten seed bits")
    if merge_word(seed_h, partial.othermode_h, partial.othermode_h_mask) != seed_h:
        raise SystemExit("unwritten H word did not pass the seed through")

    # Actual shield/reflector masks against the same seed: shield keeps the
    # display proc's XLU flags except L bits 0..1, reflector keeps only the
    # unwritten L bit 2 from the incoming word.
    shield_l = merge_word(seed_l, 1, 0x3)
    if shield_l != 0x5A5A5A59:
        raise SystemExit(f"shield L merge {shield_l:#x} != 0x5a5a5a59")
    reflector_h = merge_word(seed_h, 0x8000, 0xC000)
    reflector_l = merge_word(seed_l, 0x553049, 0xFFFFFFFB)
    if reflector_h != (seed_h & ~0xC000) | 0x8000:
        raise SystemExit("reflector H merge drifted")
    if reflector_l & 0x4 != seed_l & 0x4:
        raise SystemExit("reflector merge overwrote unwritten L bit 2")
    if reflector_l & ~0x4 != 0x553049 & ~0x4:
        raise SystemExit("reflector merge dropped baked L bits")


def main() -> None:
    # SHA-pinned source identities.
    if gen.SHIELD.file_id != 163:
        raise SystemExit(f"shield file id {gen.SHIELD.file_id} != 163")
    if gen.REFLECTOR.file_id != 346:
        raise SystemExit(f"reflector file id {gen.REFLECTOR.file_id} != 346")
    if gen.SHIELD_ROOTS != (0x0248,):
        raise SystemExit(f"shield roots {gen.SHIELD_ROOTS!r} != (0x0248,)")
    if gen.REFLECTOR_ROOTS != (0x01B8,):
        raise SystemExit(f"reflector roots {gen.REFLECTOR_ROOTS!r} != (0x01B8,)")

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

    # Roots and triangles.
    if len(shield.groups) != 1:
        raise SystemExit(f"shield groups {len(shield.groups)} != 1")
    if len(reflector.groups) != 1:
        raise SystemExit(f"reflector groups {len(reflector.groups)} != 1")
    shield_tris = sum(len(g.corners) // 3 for g in shield.groups)
    reflector_tris = sum(len(g.corners) // 3 for g in reflector.groups)
    if shield_tris != 2:
        raise SystemExit(f"shield triangles {shield_tris} != 2")
    if reflector_tris != 6:
        raise SystemExit(f"reflector triangles {reflector_tris} != 6")

    # Format and material: shield IA8 -> A3I5, reflector CI4 -> PAL16, no live
    # MObj. The shield takes the colour-heavy side of the IA trade because its
    # radial colour ramp visibly bands at eight entries. Its source alpha is
    # graded too, however, so A3I5 really does reduce that 4-bit channel to
    # three bits. Pin the source levels below so visual acceptance owns that
    # trade instead of relying on the old flat-alpha premise.
    shield_tex = list(shield.textures.values())
    reflector_tex = list(reflector.textures.values())
    if len(shield_tex) != 1 or shield_tex[0].ds_format != gen.TEX_A3I5:
        raise SystemExit("shield texture is not one A3I5 conversion")
    if len(shield_tex[0].palette) != 32:
        raise SystemExit(
            f"shield palette has {len(shield_tex[0].palette)} entries, not 32")
    if len(reflector_tex) != 1 or reflector_tex[0].ds_format != gen.TEX_PAL16:
        raise SystemExit("reflector texture is not one PAL16 conversion")

    shield_key = shield.groups[0].state.texture_key
    if shield_key is None or gen.texture_key(shield.display) != shield_key:
        raise SystemExit("shield source texture state drifted after compile")
    if (shield_key.image_asset, shield_key.fmt, shield_key.size,
            shield_key.width, shield_key.height) != (
                163, gen.FMT_IA, gen.SIZ_8B, 32, 32):
        raise SystemExit(f"shield source IA8 identity drifted: {shield_key!r}")
    shield_image = resources[shield_key.image_asset]
    source_alpha_nibbles: set[int] = set()
    source_intensity_nibbles: set[int] = set()
    for y in range(shield_key.height):
        for x in range(shield_key.width):
            sx, sy, source_width, _w, _h = gen.source_coords(
                shield.display, x, y)
            source_index = sy * source_width + sx
            physical = shield_key.image_offset + (source_index ^ 3)
            if physical >= len(shield_image.payload):
                raise SystemExit("shield IA8 source texel escaped file 163")
            value = shield_image.payload[physical]
            source_intensity_nibbles.add((value >> 4) & 0xF)
            source_alpha_nibbles.add(value & 0xF)
    expected_alpha_nibbles = {0, 1, 4, 6, 8, 10, 11, 12, 13, 14, 15}
    expected_intensity_nibbles = set(range(3, 16))
    if source_alpha_nibbles != expected_alpha_nibbles:
        raise SystemExit(
            f"shield source alpha levels {sorted(source_alpha_nibbles)} != "
            f"{sorted(expected_alpha_nibbles)}"
        )
    if source_intensity_nibbles != expected_intensity_nibbles:
        raise SystemExit(
            f"shield source intensity levels {sorted(source_intensity_nibbles)} != "
            f"{sorted(expected_intensity_nibbles)}"
        )
    if len(source_alpha_nibbles) <= 1:
        raise SystemExit("shield source alpha unexpectedly became flat")
    for g in shield.groups + reflector.groups:
        if g.state.material_slot != gen.MATERIAL_NONE:
            raise SystemExit("shield/reflector group carries a live material slot")

    # Write masks derived from actual DL commands, never from color values.
    # Runtime merges (incoming & ~mask) | (baked & mask) and takes incoming
    # colors only when their write bit is clear; a 0xFFFFFFFF default is an
    # unwritten default, not an inheritance sentinel.
    for g in shield.groups:
        if g.state.prim_color != 0xFFFFFFC0:
            raise SystemExit(f"shield prim {g.state.prim_color:#x} != 0xffffffc0")
        if g.state.env_color != 0xFFFFFFFF:
            raise SystemExit("shield baked env drifted from unwritten default")
        if g.state.color_write_mask != gen.COLOR_WRITE_PRIM:
            raise SystemExit(
                f"shield color mask {g.state.color_write_mask:#x} != prim-only 0x1; "
                "env must stay per-player from efManagerShieldProcDisplay"
            )
        if not (g.state.geometry_clear & 0x00020000) or (g.state.geometry_mode & 0x00020000):
            raise SystemExit("shield lighting is not cleared/unlit as sourced")
        if (g.state.combine_w0, g.state.combine_w1) != (0xFC309661, 0x552EFF7F):
            raise SystemExit("shield combine drifted from sourced DL")
        if (g.state.othermode_h, g.state.othermode_l) != (0, 1):
            raise SystemExit("shield othermode drifted from sourced XLU state")
        if (g.state.othermode_h_mask, g.state.othermode_l_mask) != (0x0, 0x3):
            raise SystemExit(
                f"shield othermode masks {g.state.othermode_h_mask:#x}/"
                f"{g.state.othermode_l_mask:#x} != 0x0/0x3; the list writes "
                "only L bits 0..1, the display proc's XLU blend flags must "
                "survive the merge"
            )
        if g.state.light_mask != 0:
            raise SystemExit("shield carries baked light colors")
    for g in reflector.groups:
        if g.state.prim_color != 0xFFFFFFFF or g.state.env_color != 0xFFFFFFFF:
            raise SystemExit("reflector baked prim/env drifted from unwritten default")
        if g.state.color_write_mask != 0:
            raise SystemExit(
                f"reflector color mask {g.state.color_write_mask:#x} != 0; "
                "prim/env must stay inherited from battle display"
            )
        if not (g.state.geometry_clear & 0x00020000) or (g.state.geometry_mode & 0x00020000):
            raise SystemExit("reflector lighting is not cleared/unlit as sourced")
        if (g.state.combine_w0, g.state.combine_w1) != (0xFC121824, 0xFF33FFFF):
            raise SystemExit("reflector combine drifted from sourced DL")
        if (g.state.othermode_h, g.state.othermode_l) != (0x8000, 0x553049):
            raise SystemExit(
                f"reflector othermode {g.state.othermode_h:#x}/{g.state.othermode_l:#x} drifted"
            )
        if (g.state.othermode_h_mask, g.state.othermode_l_mask) != (0xC000, 0xFFFFFFFB):
            raise SystemExit(
                f"reflector othermode masks {g.state.othermode_h_mask:#x}/"
                f"{g.state.othermode_l_mask:#x} != 0xc000/0xfffffffb"
            )
        if g.state.light_mask != 0:
            raise SystemExit("reflector carries baked light colors")

    # Backward-compatible emit plus extended emit, via temp output only.
    old_text = gen.emit(*existing)
    new_text = gen.emit(*existing, shield, reflector)
    with tempfile.TemporaryDirectory() as tmp:
        old_path = Path(tmp) / "entry_old.inc"
        new_path = Path(tmp) / "entry_new.inc"
        old_path.write_text(old_text, encoding="ascii")
        new_path.write_text(new_text, encoding="ascii")
        old_back = old_path.read_text(encoding="ascii")
        new_back = new_path.read_text(encoding="ascii")

    def root_rows(text: str) -> list[str]:
        return [line.strip() for line in text.splitlines() if line.strip().startswith("{ 0x")]

    # Existing corpus prefix must remain stable; new roots append after.
    if "#define NDS_ENTRY_EFFECT_ROOT_COUNT 29u" not in old_back:
        raise SystemExit("old emit root count drifted from 29")
    if "#define NDS_ENTRY_EFFECT_GROUP_COUNT 71u" not in old_back:
        raise SystemExit("old emit group count drifted from 71")
    if "#define NDS_ENTRY_EFFECT_ROOT_COUNT 31u" not in new_back:
        raise SystemExit("new emit root count != 31")
    for token in (
        "#define NDS_ENTRY_EFFECT_FOX_ROOT_FIRST 2u",
        "#define NDS_ENTRY_EFFECT_DONKEY_ROOT_FIRST 10u",
        "#define NDS_ENTRY_EFFECT_SAMUS_ROOT_FIRST 11u",
        "#define NDS_ENTRY_EFFECT_CAPTAIN_ROOT_FIRST 13u",
        "#define NDS_ENTRY_EFFECT_LINK_ROOT_FIRST 23u",
        "#define NDS_ENTRY_EFFECT_LINK_SPIN_WEAPON_ROOT_FIRST 26u",
        "#define NDS_ENTRY_EFFECT_LINK_BOOMERANG_ROOT_FIRST 27u",
        "#define NDS_ENTRY_EFFECT_SHIELD_ROOT_FIRST 29u",
        "#define NDS_ENTRY_EFFECT_SHIELD_ROOT_COUNT 1u",
        "#define NDS_ENTRY_EFFECT_REFLECTOR_ROOT_FIRST 30u",
        "#define NDS_ENTRY_EFFECT_REFLECTOR_ROOT_COUNT 1u",
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
    # Root table rows: first 29 identical, then shield/reflector appended.
    if len(old_roots) != 29 or len(new_roots) != 31:
        raise SystemExit(f"root rows old={len(old_roots)} new={len(new_roots)}")
    if old_roots != new_roots[:29]:
        raise SystemExit("existing 29 root rows are not a stable prefix")
    if "{ 0x0248u," not in new_roots[29] or "{ 0x01b8u," not in new_roots[30]:
        raise SystemExit(f"appended roots are not shield/reflector: {new_roots[29:31]!r}")

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
    if len(old_groups) != 71:
        raise SystemExit(f"old group rows {len(old_groups)} != 71")
    if len(new_groups) != 73:
        raise SystemExit(f"new group rows {len(new_groups)} != 73")
    if old_groups != new_groups[:71]:
        raise SystemExit("existing 71 group rows are not a stable prefix")

    # Follow the checked-in packet exactly as runtime does: shield root 29 ->
    # first group -> texture slot. This catches a stale or wrongly remapped
    # generated table even when the shield compiler in isolation is correct.
    checked_back = gen.OUTPUT.read_text(encoding="ascii")
    checked_roots = root_table_rows(checked_back)
    checked_groups = group_rows(checked_back)

    def texture_rows(text: str) -> list[str]:
        in_textures = False
        rows: list[str] = []
        for line in text.splitlines():
            if "sNdsEntryEffectTextures[" in line:
                in_textures = True
                continue
            if in_textures:
                if line.strip().startswith("};"):
                    break
                if line.strip().startswith("{"):
                    rows.append(line.strip())
        return rows

    checked_textures = texture_rows(checked_back)
    shield_root_fields = [
        part.strip()
        for part in checked_roots[shield_base].strip("{}, ").split(",")
    ]
    shield_first_group = int(shield_root_fields[1].rstrip("u"), 0)
    shield_group_fields = [
        part.strip()
        for part in checked_groups[shield_first_group].strip("{}, ").split(",")
    ]
    shield_texture_slot = int(shield_group_fields[3].rstrip("u"), 0)
    shield_texture_fields = [
        part.strip()
        for part in checked_textures[shield_texture_slot].strip("{}, ").split(",")
    ]
    shield_texture_shape = tuple(
        int(shield_texture_fields[index].rstrip("u"), 0)
        for index in (3, 4, 5, 6)
    )
    if shield_texture_shape != (32, 32, 32, gen.TEX_A3I5):
        raise SystemExit(
            f"runtime shield texture slot {shield_texture_slot} shape/format "
            f"{shield_texture_shape!r} != (32, 32, 32, A3I5)"
        )

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

    # Per-group mask tables run in sNdsEntryEffectGroups order: 71 stable
    # entries, then shield, then reflector.
    for label, table, braced, old_len, new_len, tail in (
        ("color", "sNdsEntryEffectColorWriteMasks", False, 71, 73, ["1u,", "0u,"]),
        ("othermode", "sNdsEntryEffectOtherModeWriteMasks", True, 71, 73,
         ["{ 0x00000000u, 0x00000003u },", "{ 0x0000c000u, 0xfffffffbu },"]),
    ):
        old_rows = mask_table_rows(old_back, table, braced)
        new_rows = mask_table_rows(new_back, table, braced)
        if len(old_rows) != old_len or len(new_rows) != new_len:
            raise SystemExit(
                f"{label} mask rows old={len(old_rows)} new={len(new_rows)}"
            )
        if old_rows != new_rows[:old_len]:
            raise SystemExit(f"existing 71 {label} mask rows are not a stable prefix")
        if new_rows[old_len:] != tail:
            raise SystemExit(
                f"appended {label} masks {new_rows[old_len:]!r} != {tail!r}"
            )

    check_synthetic_write_masks()

    print(
        "NATIVE_SHIELD_REFLECTOR_PACKETS_OK "
        f"shield_asset=163@0x0248 groups=1 triangles=2 format=A3I5 "
        f"shield_runtime_slot={shield_texture_slot} "
        f"source_alpha_levels={len(source_alpha_nibbles)} "
        f"source_intensity_levels={len(source_intensity_nibbles)} "
        f"reflector_asset=346@0x01b8 groups=1 triangles=6 format=PAL16 "
        "shield_masks=color:0x1othermode:0x0/0x3 "
        "reflector_masks=color:0x0othermode:0xc000/0xfffffffb "
        "roots=31 groups=73 prefix_stable=29/71/71 "
        "regen=python scripts/3d_vfx/generate_nds_entry_effects.py"
    )


if __name__ == "__main__":
    main()
