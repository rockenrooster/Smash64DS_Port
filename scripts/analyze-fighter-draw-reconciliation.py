#!/usr/bin/env python3
"""Reconcile the whole fighter-draw cost against named symbols, in ticks/frame.

The `FTR` tick-HUD bucket is an inclusive timer; a census reports self time. The
gap between them is where four wrong conclusions have been drawn on this lane, so
this script does the conversion the one sound way and shows its work.

THE CONVERSION, and it is the only one that is valid:

    %non-idle = symbol_cycles / (census_total - armWaitForIrq)
    ticks/frame = %non-idle * <shipped frame budget>

NEVER divide census cycles by a tick-HUD frame count. They are different
instruments on different builds: the profiled build's own section E prints
control frames at 2,240,292 cycles against the shipped `WORK-H` of ~1,128,000,
because the profiled build carries profiling overhead. Doing that division
under-read this lane by 2.36x and cost a cycle's worth of retracted conclusions.

Two further honesty rules this encodes:

  - **Self time is not a subsystem budget.** The soft-float helpers are the
    largest non-idle symbols in the build and "optimize __aeabi_fadd" is not a
    task. Their cycles are re-attributed to the callers that drive them, from
    `softfloat-attribution.json`, and reported as a separate column so a group's
    self time is never silently inflated.
  - **Instrumentation is not shipped cost.** The census build carries the Task 91
    phase counters, `cpuGetTiming`, and the tick-HUD's own drawing. Those symbols
    are tagged and subtracted, because a reconciliation that counts them
    overstates what any rewrite could recover.

Usage:
  python scripts/analyze-fighter-draw-reconciliation.py \
      --census artifacts/performance/<run>/census.json \
      [--softfloat artifacts/performance/<run>/softfloat-attribution.json] \
      [--frame-budget 1128000] [--json <out.json>]
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path

IDLE_SYMBOL = "armWaitForIrq"

# Ordered: the first pattern that matches a symbol wins, so put the narrow
# groups above the broad ones.
GROUPS: list[tuple[str, tuple[str, ...]]] = [
    ("instrumentation (not shipped)", (
        "ndsFtrPreMaterialCensus",
        "cpuGetTiming",
        "ndsPlatformRenderDebugHud",
        "ndsRendererAdapterM2Census",
    )),
    ("emit / GX submission", (
        "EmitProductionRaw",
        "EmitProductionCross",
        "EmitProductionPrimitive",
        "EmitDenseRawRun",
        "HardwareLitShade",
    )),
    ("production driver", (
        "ndsRendererExecuteNativeFighterOwnerProduction",
        "ndsRendererNativePrepareProductionRun",
        "ndsRendererNativeSubmitProductionRun",
        "ndsRendererNativeApplyProductionPreamble",
        "ndsRendererNativeBindProductionRoot",
    )),
    ("matrix preparation", (
        "BuildDObjLocalMatrix",
        "BuildDObjWorldMatrix",
        "FindDObjWorldMatrix",
        "BuildFighterTraRotRpy",
        "MtxMulAffine20p12",
        "MtxMul20p12",
        "LoadHardwareSplitMatrices",
        "LoadHardwareMatrixPair",
        "BuildDObjXObjMatrix",
    )),
    ("material / shading state", (
        "BuildNativeMaterialSnapshot",
        "NativeApplyMaterial",
        "NativeApplyStateDelta",
        "ShadeProductionActions",
    )),
    ("fighter parts / params", (
        "ndsFTParamsInvalidateFighterParts",
        "ftParamsUpdateFighterParts",
        "lbCommonAddFighterPartsFigatree",
        "gmCollisionGetFighterPartsWorldPosition",
    )),
    ("display contract / plan", (
        "ndsFighterDisplayContract",
        "ndsFighterDLDrawResolve",
        "ndsFighterDLScan",
        "ndsRendererAdapterMarkDisplayProcHeads",
        "ndsFighterDrawPlan",
    )),
    ("adapter driver", (
        "ndsFighterMarioFox",
        "ftDisplayMain",
        "ftDisplayLights",
    )),
]


def load(path: Path) -> dict:
    with path.open(encoding="utf-8") as handle:
        return json.load(handle)


def classify(name: str) -> str | None:
    for group, patterns in GROUPS:
        for pattern in patterns:
            if pattern in name:
                return group
    return None


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--census", required=True, type=Path)
    parser.add_argument("--softfloat", type=Path)
    parser.add_argument("--frame-budget", type=int, default=1128000)
    parser.add_argument("--json", type=Path)
    args = parser.parse_args()

    census = load(args.census)
    symbols = census["symbols"]
    idle = next((s["cycles"] for s in symbols if s["name"] == IDLE_SYMBOL), 0)
    non_idle = census["total_cycles"] - idle
    if non_idle <= 0:
        raise SystemExit("census has no non-idle cycles")

    softfloat: dict[str, int] = {}
    if args.softfloat and args.softfloat.exists():
        callers = load(args.softfloat)["callers"]
        softfloat = {name: entry["cycles"] for name, entry in callers.items()}

    def ticks(cycles: int) -> float:
        return cycles / non_idle * args.frame_budget

    grouped: dict[str, list[tuple[str, int, int]]] = {}
    for symbol in symbols:
        if symbol["cycles"] <= 0:
            continue
        group = classify(symbol["name"])
        if group is None:
            continue
        helper = softfloat.get(symbol["name"], 0)
        grouped.setdefault(group, []).append(
            (symbol["name"], symbol["cycles"], helper))

    print(f"census        {args.census}")
    print(f"non-idle      {non_idle:,} cycles "
          f"({100.0 * idle / census['total_cycles']:.2f}% idle removed)")
    print(f"frame budget  {args.frame_budget:,} ticks")
    print()

    order = [group for group, _ in GROUPS]
    shipped_self = shipped_helper = 0.0
    report: dict[str, dict] = {}
    for group in order:
        rows = sorted(grouped.get(group, []), key=lambda row: -row[1])
        if not rows:
            continue
        self_ticks = sum(ticks(row[1]) for row in rows)
        helper_ticks = sum(ticks(row[2]) for row in rows)
        shipped = "instrumentation" not in group
        if shipped:
            shipped_self += self_ticks
            shipped_helper += helper_ticks
        report[group] = {
            "self_ticks_per_frame": round(self_ticks),
            "helper_ticks_per_frame": round(helper_ticks),
            "symbols": [
                {"name": row[0], "cycles": row[1],
                 "self_ticks_per_frame": round(ticks(row[1])),
                 "helper_ticks_per_frame": round(ticks(row[2]))}
                for row in rows
            ],
        }
        print(f"== {group}: self {self_ticks:,.0f} + soft-float "
              f"{helper_ticks:,.0f} = {self_ticks + helper_ticks:,.0f} "
              f"ticks/frame ==")
        for name, cycles, helper in rows:
            extra = f"  +sf {ticks(helper):>7,.0f}" if helper else ""
            print(f"  {ticks(cycles):>8,.0f}{extra:<16}  {name}")
        print()

    print(f"SHIPPED FIGHTER DRAW: self {shipped_self:,.0f} + soft-float "
          f"{shipped_helper:,.0f} = {shipped_self + shipped_helper:,.0f} "
          f"ticks/frame")
    report["_totals"] = {
        "non_idle_cycles": non_idle,
        "frame_budget": args.frame_budget,
        "shipped_self_ticks_per_frame": round(shipped_self),
        "shipped_softfloat_ticks_per_frame": round(shipped_helper),
        "shipped_total_ticks_per_frame": round(shipped_self + shipped_helper),
    }
    if args.json:
        args.json.parent.mkdir(parents=True, exist_ok=True)
        with args.json.open("w", encoding="utf-8") as handle:
            json.dump(report, handle, indent=1)
        print(f"wrote {args.json}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
