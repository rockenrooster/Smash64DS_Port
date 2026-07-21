#!/usr/bin/env python3
"""Generate Task 39 effect IDs, call counters, and the Phase-A table."""

from __future__ import annotations

import json
import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
MANAGER_H = ROOT / "decomp/BattleShip-main/decomp/src/ef/efmanager.h"
MANAGER_C = ROOT / "decomp/BattleShip-main/decomp/src/ef/efmanager.c"
PARTICLE_H = ROOT / "decomp/BattleShip-main/decomp/src/ef/efparticle.h"
PORT_SHIMS = ROOT / "src/port/reloc_backend_compat_shims.c"
PORT_STUBS = ROOT / "src/port/battle_playable_compat_stubs.c"
IMPORT = ROOT / "src/import/battleship_efmanager.c"

SUBSTITUTES = {
    "efManagerDamageNormalLightMakeEffect",
    "efManagerDamageNormalHeavyMakeEffect",
    "efManagerDamageFireMakeEffect",
    "efManagerDamageElectricMakeEffect",
    "efManagerDamageCoinMakeEffect",
    "efManagerDamageSlashMakeEffect",
    "efManagerDustExpandSmallMakeEffect",
    "efManagerFireGrindMakeEffect",
    "efManagerSparkleWhiteMakeEffect",
    "efManagerSparkleWhiteScaleMakeEffect",
    "efManagerDamageSpawnOrbsRandomMakeEffect",
    "efManagerDamageSpawnSparksRandomMakeEffect",
    "efManagerDamageSpawnMDustRandomMakeEffect",
    "efManagerImpactWaveMakeEffect",
    "efManagerSetOffMakeEffect",
    "efManagerCatchSwirlMakeEffect",
    "efManagerFlashMiddleMakeEffect",
    "efManagerDeadExplodeMakeEffect",
    "efManagerSparkleWhiteDeadMakeEffect",
    "efManagerRebirthHaloMakeEffect",
    "efManagerFoxReflectorMakeEffect",
}

SKIPPED_OVERRIDES = {
    "efManagerQuakeMakeEffect",
    "efManagerYoshiShieldMakeEffect",
    "efManagerKirbyVulcanJabMakeEffect",
    "efManagerSamusGrappleBeamGlowMakeEffect",
    "efManagerStockSnapMakeEffect",
    "efManagerStockStealStartMakeEffect",
    "efManagerStockStealEndMakeEffect",
    "efManagerBattleScoreMakeEffect",
    "efManagerEggBreakMakeEffect",
}


def line_of(path: Path, needle: str) -> int:
    for number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        if needle in line:
            return number
    raise SystemExit(f"{path}: missing {needle}")


def prototypes(path: Path, pattern: str) -> list[tuple[str, str]]:
    text = path.read_text(encoding="utf-8")
    return [
        (match.group(2), " ".join(match.group(1).split()))
        for match in re.finditer(
            r"extern\s+([^;]*?\b(" + pattern + r")\s*\([^;]*\))\s*;",
            text,
            re.S,
        )
    ]


def function_body(text: str, name: str) -> str:
    match = re.search(r"\b" + re.escape(name) + r"\s*\([^;]*?\)\s*\{", text, re.S)
    if match is None:
        return ""
    start = match.end() - 1
    depth = 0
    for index in range(start, len(text)):
        depth += text[index] == "{"
        depth -= text[index] == "}"
        if depth == 0:
            return text[start : index + 1]
    return ""


def ident(name: str) -> str:
    return re.sub(r"(?<!^)(?=[A-Z])", "_", name).upper()


def route_for(name: str, body: str) -> tuple[str, str]:
    if name in SUBSTITUTES:
        return "NDS_TASK39_EFFECT_SUBSTITUTE", "DS substitute"
    if name in SKIPPED_OVERRIDES:
        return "NDS_TASK39_EFFECT_SKIPPED", "no-op / skipped"
    if "lbParticleMake" in body:
        return "NDS_TASK39_EFFECT_SKIPPED", "particle-shimmed / skipped"
    return "NDS_TASK39_EFFECT_ORIGINAL", "original imported"


def citation_for(name: str, classification: str) -> str:
    declaration = line_of(MANAGER_H, name)
    if classification in ("original imported", "particle-shimmed / skipped"):
        include_needle = '#include "../../decomp/BattleShip-main/decomp/src/ef/efmanager.c"'
        owner = f"src/import/battleship_efmanager.c:{line_of(IMPORT, include_needle)}"
        if classification == "particle-shimmed / skipped":
            owner += f"; src/port/reloc_backend_compat_shims.c:{line_of(PORT_SHIMS, 'lbParticleMakeScriptID')}"
    else:
        owner_path = next(
            path for path in (PORT_SHIMS, PORT_STUBS, IMPORT)
            if name in path.read_text(encoding="utf-8")
        )
        owner = f"{owner_path.relative_to(ROOT).as_posix()}:{line_of(owner_path, name)}"
    return f"decomp/BattleShip-main/decomp/src/ef/efmanager.h:{declaration}; {owner}"


def main() -> None:
    manager = prototypes(MANAGER_H, r"efManager\w*MakeEffect")
    if len(manager) != 97:
        raise SystemExit(f"expected 97 efManager entry points, found {len(manager)}")
    manager_text = MANAGER_C.read_text(encoding="utf-8")
    rows: list[tuple[str, str, str, str]] = []
    for name, _prototype in manager:
        route, classification = route_for(name, function_body(manager_text, name))
        rows.append((name, route, classification, citation_for(name, classification)))

    particle = prototypes(PARTICLE_H, r"efParticle\w+")
    if len(particle) != 7:
        raise SystemExit(f"expected 7 efParticle entry points, found {len(particle)}")
    shim_text = PORT_SHIMS.read_text(encoding="utf-8")
    stub_text = PORT_STUBS.read_text(encoding="utf-8")
    for name, _prototype in particle:
        owner = PORT_SHIMS if name in shim_text else (
            PORT_STUBS if name in stub_text else None
        )
        ownership = (
            f"{owner.relative_to(ROOT).as_posix()}:{line_of(owner, name)}"
            if owner is not None else "no port definition (unreached)"
        )
        rows.append(
            (name, "NDS_TASK39_EFFECT_SKIPPED", "particle control shim",
             f"decomp/BattleShip-main/decomp/src/ef/efparticle.h:{line_of(PARTICLE_H, name)}; "
             f"{ownership}")
        )

    effect_h = ROOT / "include/ef/effect.h"
    for name in ("lbParticleMakeScriptID", "lbParticleMakeCommon",
                 "lbParticleMakePosVel", "lbParticleMakeGenerator"):
        rows.append(
            (name, "NDS_TASK39_EFFECT_SKIPPED", "particle-shimmed / skipped",
             f"include/ef/effect.h:{line_of(effect_h, name)}; "
             f"src/port/reloc_backend_compat_shims.c:{line_of(PORT_SHIMS, name)}")
        )

    rows.append(
        ("efManagerDamageNormalHeavyMakeEffect",
         "NDS_TASK39_EFFECT_SUBSTITUTE", "DS substitute",
         f"decomp/BattleShip-main/decomp/src/ef/efmanager.c:"
         f"{line_of(MANAGER_C, 'LBParticle* efManagerDamageNormalHeavyMakeEffect')}; "
         f"src/port/reloc_backend_compat_shims.c:"
         f"{line_of(PORT_SHIMS, 'efManagerDamageNormalHeavyMakeEffect')}")
    )
    rows.append(
        ("efManagerShieldMakeEffect", "NDS_TASK39_EFFECT_SUBSTITUTE", "DS substitute",
         f"decomp/BattleShip-main/decomp/src/ef/efmanager.c:{line_of(MANAGER_C, 'GObj* efManagerShieldMakeEffect')}; "
         f"src/port/reloc_backend_compat_shims.c:{line_of(PORT_SHIMS, 'efManagerShieldMakeEffect')}")
    )
    rows.append(
        ("ftParamCheckSetFighterColAnimID", "NDS_TASK39_EFFECT_ORIGINAL",
         "source color state; DS draw adapter",
         "decomp/BattleShip-main/decomp/src/gm/gmcolscripts.c:48; "
         f"src/port/reloc_backend_compat_shims.c:{line_of(PORT_SHIMS, 'ftParamCheckSetFighterColAnimID')}")
    )

    generated = ROOT / "include/nds/generated/task39_effect_census.generated.h"
    generated.parent.mkdir(parents=True, exist_ok=True)
    enum_rows = [f"    NDS_TASK39_EFFECT_{ident(name)}," for name, *_ in rows]
    name_rows = [f'    "{name}",' for name, *_ in rows]
    route_rows = [f"    {route}," for _name, route, _class, _cite in rows]
    generated.write_text(
        "/* Generated by scripts/generate_task39_effect_census.py. */\n"
        "#ifndef SSB64_NDS_TASK39_EFFECT_CENSUS_GENERATED_H\n"
        "#define SSB64_NDS_TASK39_EFFECT_CENSUS_GENERATED_H\n\n"
        "typedef enum NDSTask39EffectRoute {\n"
        "    NDS_TASK39_EFFECT_ORIGINAL,\n"
        "    NDS_TASK39_EFFECT_SUBSTITUTE,\n"
        "    NDS_TASK39_EFFECT_SKIPPED\n"
        "} NDSTask39EffectRoute;\n\n"
        "typedef enum NDSTask39EffectID {\n" + "\n".join(enum_rows) +
        "\n    NDS_TASK39_EFFECT_COUNT\n} NDSTask39EffectID;\n\n"
        "void ndsTask39EffectCensusRecord(u32 id, u32 route);\n"
        "void ndsTask39EffectCensusReset(void);\n"
        "extern const char *const gNdsTask39EffectNames[NDS_TASK39_EFFECT_COUNT];\n"
        "extern const u8 gNdsTask39EffectRoutes[NDS_TASK39_EFFECT_COUNT];\n"
        "extern volatile u32 gNdsTask39EffectSpawnCount[NDS_TASK39_EFFECT_COUNT];\n"
        "extern volatile u32 gNdsTask39EffectOriginalCount[NDS_TASK39_EFFECT_COUNT];\n"
        "extern volatile u32 gNdsTask39EffectSubstituteCount[NDS_TASK39_EFFECT_COUNT];\n"
        "extern volatile u32 gNdsTask39EffectSkippedCount[NDS_TASK39_EFFECT_COUNT];\n\n"
        "#define NDS_TASK39_EFFECT_NAME_ROWS \\\n" + " \\\n".join(name_rows) + "\n\n"
        "#define NDS_TASK39_EFFECT_ROUTE_ROWS \\\n" + " \\\n".join(route_rows) + "\n\n"
        "#endif\n",
        encoding="ascii",
        newline="\n",
    )

    report = ROOT / "artifacts/performance/2026-07-21_task39-visual-effects-census.md"
    report.parent.mkdir(parents=True, exist_ok=True)
    runtime_path = report.with_name("2026-07-21_task39-runtime-census.json")
    runtime = json.loads(runtime_path.read_text(encoding="utf-8"))
    runtime_counts = runtime["effects"]
    unknown = sorted(set(runtime_counts) - {name for name, *_ in rows})
    if unknown:
        raise SystemExit(f"unknown runtime effect rows: {', '.join(unknown)}")
    table = [
        "# Task 39 Phase A visual-effects census",
        "",
        f"Mechanical inventory: {len(manager)} efManager entry points, {len(particle)} efParticle entry points, 4 lbParticle constructors, plus shield and hurt-color seams.",
        "",
        f"Runtime snapshot: `{runtime['rom_sha256']}` with Fox AI off, fast natural-input update {runtime['stop_update']} / phase {runtime['stop_phase']}. Unlisted rows observed zero calls.",
        "",
        "| Entry point | Classification | Called | Original | Substitute | Skipped | Ownership evidence |",
        "|---|---|---:|---:|---:|---:|---|",
    ]
    table.extend(
        f"| `{name}` | {classification} | "
        f"{' | '.join(str(value) for value in runtime_counts.get(name, [0, 0, 0, 0]))} | {citation} |"
        for name, _route, classification, citation in rows
    )
    report.write_text("\n".join(table) + "\n", encoding="utf-8", newline="\n")
    print(f"TASK39_EFFECT_CENSUS=PASS rows={len(rows)} manager={len(manager)} particle={len(particle)} runtime_nonzero={len(runtime_counts)}")


if __name__ == "__main__":
    main()
