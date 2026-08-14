#!/usr/bin/env python3
"""Audit P1 texture/effect reconstruction against BattleShip source contracts.

This is deliberately a host/source audit, not a screenshot grader.  It answers
the questions that a picture cannot answer reliably:

* Which EFCommon particle scripts are reachable from the P1 effect seams?
* Which of those scripts ever enable N64 MASKS/MASKT texture mirroring?
* Which source textures/frames are actually present in the DS common atlas?
* Which P1 texture families live outside that atlas and therefore cannot share
  its half/quarter-image failure mode?

The generated Markdown is intended to be checked in as verifier evidence.  The
script exits non-zero when the source contract or the DS reconstruction seam
drifts away from the invariants proved here.
"""

from __future__ import annotations

import argparse
import importlib.util
from collections import defaultdict
from pathlib import Path


MASK_OPS = {
    0xAE: "NONE",
    0xAF: "S",
    0xB0: "T",
    0xB1: "ST",
}


def load_generator(root: Path):
    path = root / "scripts" / "generate_nds_particle_banks.py"
    spec = importlib.util.spec_from_file_location("nds_particle_bank_generator", path)
    if spec is None or spec.loader is None:
        raise SystemExit(f"cannot load {path}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def reachable_seams(pack: dict) -> dict[int, list[str]]:
    """Map each reachable script to every direct P1 seam that can reach it."""
    result: dict[int, set[str]] = defaultdict(set)
    edges = pack["reach"]["spawn_edges"]
    for seed, names in pack["reach"]["seeds"].items():
        stack = [seed]
        seen: set[int] = set()
        while stack:
            script = stack.pop()
            if script in seen:
                continue
            seen.add(script)
            stack.extend(edges.get(script, ()))
        for script in seen:
            result[script].update(names)
    return {key: sorted(value) for key, value in result.items()}


def assert_contains(text: str, needle: str, where: str) -> None:
    if needle not in text:
        raise SystemExit(f"{where}: missing required audit token {needle!r}")


def render(root: Path) -> str:
    generator = load_generator(root)
    pack = generator.build_pack(root)
    script_seams = reachable_seams(pack)
    admitted = {row["texture"]: row for row in pack["quads"]["admitted"]
                if row["texture"] < len(pack["textures"])}
    excluded = {row["texture"]: row for row in pack["quads"]["excluded"]
                if row["texture"] < len(pack["textures"])}

    # The P1 common-particle corpus is the union of the source textures reached
    # from the scanner's P1 seeds.  Pupupu extras use ids >=64 and are handled
    # separately by the generator, so they cannot alias an EFCommon id here.
    by_texture: dict[int, dict[str, set]] = defaultdict(
        lambda: {"scripts": set(), "seams": set(), "mask_ops": set()})
    masked_scripts: list[tuple[int, int, list[str], list[str]]] = []
    for script_id in pack["reach"]["reachable"]:
        script = pack["scripts"][script_id]
        texture_id = script["texture_id"]
        ops = [MASK_OPS[opcode] for opcode, _ in script["commands"]
               if opcode in MASK_OPS]
        row = by_texture[texture_id]
        row["scripts"].add(script_id)
        row["seams"].update(script_seams.get(script_id, ()))
        row["mask_ops"].update(ops)
        if ops:
            masked_scripts.append((script_id, texture_id, ops,
                                   script_seams.get(script_id, [])))

    renderer = (root / "src" / "nds" / "nds_renderer.c").read_text(
        errors="replace")
    particle_runtime = (root / "src" / "import" /
                        "battleship_lbparticle.c").read_text(errors="replace")
    static_generator = (root / "scripts" /
                        "generate_battle_playable_static_textures.py").read_text(
                            errors="replace")
    sprite_preview = (root / "src" / "port" /
                      "sprite_preview_backend.c").read_text(errors="replace")
    particle_generator = (root / "scripts" /
                          "generate_nds_particle_banks.py").read_text(
                              errors="replace")

    # Close the actual source -> runtime contract, not just the table census.
    assert_contains(particle_runtime, "LBPARTICLE_FLAG_MASKS", "particle runtime")
    assert_contains(particle_runtime, "LBPARTICLE_FLAG_MASKT", "particle runtime")
    assert_contains(particle_runtime, "source_mirror_mask", "particle runtime")
    assert_contains(renderer, "mirror_mask &= 3u", "particle renderer")
    assert_contains(renderer, "s_uv_q4[1] = ((atlas_x + atlas_w) << 4) - 1u",
                    "particle renderer")
    assert_contains(renderer, "t_uv_q4[1] = ((atlas_y + atlas_h) << 4) - 1u",
                    "particle renderer")
    assert_contains(renderer, "s_uv_q4[2] = atlas_x << 4", "particle renderer")
    assert_contains(renderer, "t_uv_q4[2] = atlas_y << 4", "particle renderer")

    # Ledge grab is the concrete owner symptom and therefore a hard regression
    # gate, not merely one row in the census.
    ledge = pack["scripts"][5]
    ledge_ops = [MASK_OPS[opcode] for opcode, _ in ledge["commands"]
                 if opcode in MASK_OPS]
    if ledge["texture_id"] != 2 or "ST" not in ledge_ops:
        raise SystemExit(
            "FlashMiddle/ledge source contract drifted: expected script 5, "
            "texture 2, MASKST")

    if len(masked_scripts) != 19:
        raise SystemExit(
            f"P1 masked-script census drifted: {len(masked_scripts)} != 19")

    # Dream Land's live native bank is not part of this bug: none of its five
    # scripts uses the mask bytecodes.  Assert that rather than relying on ids.
    pupupu_masked = []
    for script in pack["pupupu"]["scripts"]:
        ops = [MASK_OPS[opcode] for opcode, _ in script["commands"]
               if opcode in MASK_OPS]
        if ops:
            pupupu_masked.append((script["id"], ops))
    if pupupu_masked:
        raise SystemExit(f"Pupupu mask contract changed: {pupupu_masked}")

    # Existing correct families are load-bearing proof that mirroring is an
    # asset/render-state property elsewhere too.
    assert_contains(static_generator, "cms", "static texture generator")
    assert_contains(static_generator, "cmt", "static texture generator")
    assert_contains(renderer, "GL_TEXTURE_FLIP_S", "static 3D renderer")
    assert_contains(particle_generator, '"mirror_s": True', "shield generator")
    assert_contains(particle_generator, "apply_source_quad_wrap", "shield generator")

    # The software SObj path fingerprints wrap fields but does not implement
    # their sampling semantics.  That is outside P1 because the non-default SObj
    # assignments are menu files, but record it as an explicit non-P1 debt.
    assert_contains(sprite_preview, "cmt/cms/maskt/masks/lrs/lrt",
                    "SObj preview fingerprint")

    common_admitted = [tid for tid in by_texture if tid in admitted]
    common_excluded = [tid for tid in by_texture if tid in excluded]
    animated_admitted = [tid for tid in common_admitted
                         if pack["textures"][tid]["frames"] > 1]
    one_cell_animated = [tid for tid in animated_admitted
                         if len(admitted[tid].get("frame_list", ())) == 1]

    lines: list[str] = []
    add = lines.append
    add("# P1 texture / VFX reconstruction audit — 2026-08-14")
    add("")
    add("## Result")
    add("")
    add("The owner-reported half/quarter effect bug is localized to the common "
        "LBParticle atlas draw seam. BattleShip's `MASKS`/`MASKT` bytecodes do "
        "not mean that the stored bitmap is the final rectangle: the N64 doubles "
        "the texture-coordinate rate and mirrors the tile on that axis. The DS "
        "atlas previously sampled the stored cell once, so an S/T half or ST "
        "quarter fragment was displayed literally.")
    add("")
    add("**Fix candidate:** the generic DS particle submit now consumes the live "
        "`pc->flags` mask state. A masked atlas cell is reconstructed inside the "
        "same world-space rectangle as 2 quads (S or T) or 4 quads (ST), with "
        "triangle-wave UVs `0→W→0` / `0→H→0`. The fold is held one 1/16-texel "
        "unit inside the cell so an unpadded atlas can never sample its neighbor. "
        "No atlas texels or VRAM allocations are duplicated.")
    add("")
    add("The concrete ledge-grab case is source script **5 / texture 2 / MASKST** "
        "(`efManagerFlashMiddleMakeEffect`). It therefore exercises the 4-piece "
        "quarter-image reconstruction path.")
    add("")
    add("## Complete P1 common-particle symmetry census")
    add("")
    add("There are **19 reachable scripts with mask bytecodes**. The mask is "
        "listed as the script changes it; KO scripts intentionally transition "
        "between mirrored and unmirrored states, which is why this cannot be "
        "baked as one property per texture.")
    add("")
    add("| script | texture | source size/frames | mask bytecodes | reachable seam(s) |")
    add("|---:|---:|---:|---|---|")
    for script_id, texture_id, ops, seams in masked_scripts:
        tex = pack["textures"][texture_id]
        add(f"| {script_id} | {texture_id} | {tex['width']}×{tex['height']}×"
            f"{tex['frames']} | {' → '.join(ops)} | {', '.join(seams)} |")

    add("")
    add("## Common-particle texture/frame coverage")
    add("")
    add(f"EFCommon source textures in the broad P1 seam closure: **{len(by_texture)}**. "
        f"Of those, **{len(common_admitted)}** have a common-atlas cell and "
        f"**{len(common_excluded)}** are currently fail-closed/excluded. "
        f"**{len(one_cell_animated)}** admitted animated textures currently hold "
        "one representative source frame because the measured-safe atlas policy "
        "prioritizes source resolution over animation-frame count.")
    add("")
    add("The broad-closure exclusions were **never seven equal P1 blockers**. "
        "For the current Mario/Fox, slots-0/1, items-off milestone, the real "
        "coverage holes were texture **4** (the second ShieldBreak child) and "
        "textures **11/14** (side-KO DeadExplode children). Texture 28 is the "
        "electric-damage family, which Mario/Fox do not produce here; texture 31 "
        "is the sleep music-note family; textures 35/36 are the player-2/player-3 "
        "DamageNormalLight child variants. Those four stay broad-route/future "
        "coverage debt rather than current P1 blockers, and the generator now "
        "holds them out by name (`QUAD_P1_DEFERRED`) instead of relying on the "
        "packer to run out of room.")
    add("")
    add("**CLOSED 2026-08-14.** 4, 11 and 14 are admitted. The earlier trial that "
        "seated them only by dropping `cell_cap` from 64 to 32 was reading a "
        "packer limit as a VRAM limit: the four 8 KiB sheets held 27,520 texels "
        "of 32,768 and still refused every 1,024-texel candidate, because the "
        "shelf packer abandoned a shelf whenever the cell height changed and the "
        "5,248 free texels were 16-tall shelf tails no 32×32 cell could use. "
        "First-fit-decreasing over an occupancy bitmap seats all three in space "
        "the atlas already owned — same four 8,192-byte allocations, `cell_cap` "
        "still 64, `frame_cap` still 1, no admitted texture degraded. The palette "
        "block became one table per sheet in the same change, which costs no VRAM "
        "(`glColorTableEXT` already ran once per sheet) and is what keeps the five "
        "extra cells from pulling the shared k-means centres off their sheet-mates.")
    add("")
    add("| texture | source | atlas state | packed source-key frames | scripts | seams |")
    add("|---:|---:|---|---|---|---|")
    for texture_id in sorted(by_texture):
        tex = pack["textures"][texture_id]
        data = by_texture[texture_id]
        if texture_id in admitted:
            atlas_state = "ADMITTED"
            frame_list = admitted[texture_id].get("frame_list", [])
            # Texture 25 deliberately stores source frame 2 under lookup key 0;
            # state that in the table so "frame 0" cannot be misread as content.
            if (len(frame_list) == 1 and
                    texture_id in getattr(generator, "QUAD_HELD_FRAME", {})):
                content = generator.QUAD_HELD_FRAME[texture_id]
                frame_text = f"key {frame_list[0]} → content {content}"
            else:
                frame_text = ", ".join(str(x) for x in frame_list)
        elif texture_id in excluded:
            atlas_state = "EXCLUDED / draws nothing"
            frame_text = "—"
        else:
            atlas_state = "NO COMMON QUAD ROW"
            frame_text = "—"
        add(f"| {texture_id} | {tex['width']}×{tex['height']}×{tex['frames']} | "
            f"{atlas_state} | {frame_text} | "
            f"{', '.join(str(x) for x in sorted(data['scripts']))} | "
            f"{', '.join(sorted(data['seams']))} |")

    add("")
    add("The single-frame compromise is **not** solved and is not claimed to be: "
        "the admitted animated textures above still hold one representative source "
        "frame, which is the owner's accepted 2026-08-04 sacrifice. The atlas is "
        "still four measured-safe 8 KiB allocations, and that bound has not moved "
        "— past 16/32 KiB contiguous variants starved later battle/interface "
        "texture allocations. What the 2026-08-14 packer change proved is that "
        "coverage debt and the VRAM bound were two different problems: the sheets "
        "were 84% full, not full. Raising the frame cap is still the expensive "
        "direction and still needs its own proof, because it multiplies cells per "
        "texture rather than reclaiming waste.")

    add("")
    add("## Other P1 texture/effect families")
    add("")
    add("| family | audit result |")
    add("|---|---|")
    add("| Fighter/stage/source-model 3D textures | **Wrap/mirror preserved.** The "
        "static texture generator carries N64 `cms/cmt/masks/maskt`; the renderer "
        "materializes masked addressing and uses DS wrap/flip state where legal. |")
    add("| Shield | **Already reconstructed.** The source 16×32 IA8 is explicitly "
        "documented as half a bubble and `apply_source_quad_wrap` AOT-bakes its "
        "S mirror into a 32×32 standalone shield texture. |")
    add("| Fox blaster glow | **Already reconstructed on its primary route.** The "
        "standalone 16×8 PAL16 texture uses hardware T mirror to reproduce the "
        "source 16×16 MASKT flash. The generic atlas fallback is also covered by "
        "the new live-mask submit. |")
    add("| Dream Land particle bank | **No symmetry omission found.** All five "
        "Pupupu scripts contain zero MASKS/MASKT bytecodes; live leaf/dust native "
        "textures therefore need no mask reconstruction. |")
    add("| Particle transform mirroring | **Already preserved.** "
        "`ndsParticleTransformForDraw` signs the billboard right/up axes from the "
        "source affine matrix; the new texture-mask path is independent and does "
        "not double-mirror it. |")
    add("| Source DObj/native effect models (reflector, entry models, impact/rebirth "
        "families, slash/orbs/sparks/dust/catch) | **Outside the common atlas.** "
        "They route through source-model/native geometry and inherit the 3D "
        "texture-state audit above; no LBParticle half/quarter cell seam applies. |")
    add("| P1 battle/results SObj sprites | **No non-default wrap assignment found "
        "in the P1 source paths.** The CPU preview does not generally emulate "
        "`cms/cmt/masks/maskt`; source grep finds non-default SObj wrap assignments "
        "in menu scenes. Record that as non-P1 menu debt rather than claiming the "
        "software sprite renderer is globally wrap-complete. |")

    add("")
    add("## Verification gates")
    add("")
    add("- FlashMiddle/ledge source invariant: script 5, texture 2, MASKST: **PASS**")
    add(f"- Reachable common scripts with source mask bytecodes: "
        f"**{len(masked_scripts)} / expected 19: PASS**")
    add("- Dream Land bank mask bytecodes: **0: PASS**")
    add("- Runtime consumes live `MASKS` and `MASKT`: **PASS (source assertion)**")
    add("- Renderer contains S/T triangle-wave atlas-cell reconstruction: "
        "**PASS (source assertion)**")
    add("- Existing static 3D wrap/mirror + shield AOT mirror mechanisms present: "
        "**PASS**")
    # Hard gate, not a printed row. 4 is efManagerShieldBreak's second child and
    # 11/14 are the side-KO DeadExplode children; a texture with no cell draws
    # NOTHING rather than drawing worse, so losing one again would show up as an
    # effect quietly disappearing and nothing else would say so.
    p1_coverage = [tid for tid in (4, 11, 14) if tid not in admitted]
    if p1_coverage:
        raise SystemExit(
            f"P1 coverage regressed: common atlas has no cell for {p1_coverage} "
            "(shield break / side-KO DeadExplode children)")
    add("- P1 coverage textures 4/11/14 admitted to the common atlas: **PASS**")
    add("")
    add("Owner visual acceptance: **PASS 2026-08-14** for the reconstructed "
        "half/quarter effects. Atlas coverage for P1 textures 4/11/14 landed "
        "2026-08-14 and is **awaiting the owner's ShieldBreak / side-KO "
        "playtest**; no additional MASKS/MASKT work is open.")
    add("")
    return "\n".join(lines)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    root = args.root.resolve()
    report = render(root)
    if args.output:
        output = args.output
        if not output.is_absolute():
            output = root / output
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_text(report + "\n", encoding="utf-8")
        print(output)
    else:
        print(report)


if __name__ == "__main__":
    main()
