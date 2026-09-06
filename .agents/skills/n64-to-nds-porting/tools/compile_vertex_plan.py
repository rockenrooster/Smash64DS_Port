#!/usr/bin/env python3
"""Compile a small NORMALIZED semantic trace, not raw N64 opcodes, into a plan.

The input producer owns dialect decoding, complete immutable state IDs, effective
vertex-load processing, and material classification. See examples/README.md.
No proprietary assets/headers are needed. Python 3.10+, standard library only.
"""
from __future__ import annotations

import argparse
import copy
import json
from pathlib import Path
import sys
from typing import Any


class PlanError(ValueError):
    """Unsupported, malformed, or unbounded normalized input."""


def require_keys(value: Any, required: set[str], optional: set[str] | None = None) -> None:
    if not isinstance(value, dict):
        raise PlanError("expected an object")
    allowed = required | (optional or set())
    if not required <= value.keys() or not value.keys() <= allowed:
        raise PlanError(f"expected keys {sorted(required)}, optional {sorted(allowed - required)}")


def integer(value: Any, low: int, high: int, what: str) -> int:
    if type(value) is not int or not low <= value <= high:
        raise PlanError(f"{what} must be an integer in [{low}, {high}]")
    return value


def identity(value: Any, what: str) -> str:
    if not isinstance(value, str) or not value or len(value) > 256:
        raise PlanError(f"{what} must be a nonempty string of at most 256 characters")
    return value


def compile_plan(document: Any, *, max_steps: int = 100_000, max_depth: int = 32) -> dict[str, Any]:
    require_keys(document, {"schema", "cache_slots", "entry", "lists"})
    integer(document["schema"], 1, 1, "schema")
    count = integer(document["cache_slots"], 1, 256, "normalized cache_slots")
    entry = identity(document["entry"], "entry")
    lists = document["lists"]
    if not isinstance(lists, dict) or not lists:
        raise PlanError("lists must be a nonempty object")
    for name, commands in lists.items():
        identity(name, "list name")
        if not isinstance(commands, list):
            raise PlanError(f"list {name} must be an array")
    integer(max_steps, 1, 10_000_000, "max_steps")
    integer(max_depth, 1, 128, "max_depth")

    slots: list[int | None] = [None] * count
    versions: list[dict[str, Any]] = []
    triangles: list[dict[str, Any]] = []
    used: set[str] = set()
    state: dict[str, str | None] = {"transform": None, "vertex_state": None, "material": None}
    steps = 0

    def run(name: str, active: tuple[str, ...]) -> None:
        nonlocal steps
        if name not in lists:
            raise PlanError(f"missing list: {name}")
        if name in active:
            raise PlanError(f"recursive/cyclic control flow: {' -> '.join(active + (name,))}")
        if len(active) >= max_depth:
            raise PlanError("control-flow depth limit exceeded")
        used.add(name)
        active = active + (name,)
        ended = False
        for pc, command in enumerate(lists[name]):
            steps += 1
            if steps > max_steps:
                raise PlanError("expanded command limit exceeded")
            if not isinstance(command, dict) or not isinstance(command.get("op"), str):
                raise PlanError(f"{name}:{pc}: missing operation")
            op = command["op"]
            location = {"list": name, "command": pc}
            if op == "state":
                require_keys(command, {"op", "transform", "vertex_state"})
                state["transform"] = identity(command["transform"], "immutable transform ID")
                state["vertex_state"] = identity(command["vertex_state"], "immutable vertex-state ID")
            elif op == "material":
                require_keys(command, {"op", "id"})
                state["material"] = identity(command["id"], "immutable material ID")
            elif op == "load":
                require_keys(command, {"op", "first", "sources"})
                first = integer(command["first"], 0, count - 1, "first slot")
                sources = command["sources"]
                if not isinstance(sources, list) or not sources or len(sources) > count - first:
                    raise PlanError("load must fit a nonempty source array in the cache")
                if state["transform"] is None or state["vertex_state"] is None:
                    raise PlanError("vertex load without defined source state")
                for index, source in enumerate(sources):
                    source = identity(source, "source vertex/version ID")
                    version = len(versions)
                    versions.append({"id": version, "source": source,
                                     "transform": state["transform"],
                                     "vertex_state": state["vertex_state"],
                                     "overrides": {}, "origin": location})
                    slots[first + index] = version
            elif op == "patch":
                require_keys(command, {"op", "slot", "field", "value"})
                slot = integer(command["slot"], 0, count - 1, "patch slot")
                old = slots[slot]
                if old is None:
                    raise PlanError("patch of an unloaded vertex")
                field, value = command["field"], command["value"]
                if field not in ("rgba", "st"):
                    raise PlanError("only normalized rgba/st patches are supported; screen edits need another path")
                n, low, high = (4, 0, 255) if field == "rgba" else (2, -32768, 32767)
                if not isinstance(value, list) or len(value) != n:
                    raise PlanError(f"{field} requires {n} components")
                value = [integer(v, low, high, field) for v in value]
                new = copy.deepcopy(versions[old])
                new["id"] = len(versions)
                new["overrides"][field] = value
                new["origin"] = location
                new["previous_version"] = old
                slots[slot] = new["id"]
                versions.append(new)
            elif op == "tri":
                require_keys(command, {"op", "slots"})
                indices = command["slots"]
                if not isinstance(indices, list) or len(indices) != 3:
                    raise PlanError("triangle requires exactly three slots")
                selected = [slots[integer(v, 0, count - 1, "triangle slot")] for v in indices]
                if any(v is None for v in selected):
                    raise PlanError("triangle reads an unloaded vertex")
                if state["material"] is None:
                    raise PlanError("triangle without defined material")
                vertices = [int(v) for v in selected if v is not None]
                transforms = {versions[v]["transform"] for v in vertices}
                triangles.append({"vertices": vertices, "material": state["material"],
                                  "single_position_transform": len(transforms) == 1,
                                  "position_transform": next(iter(transforms)) if len(transforms) == 1 else None,
                                  "origin": location})
            elif op in ("call", "branch"):
                require_keys(command, {"op", "list"})
                run(identity(command["list"], "target list"), active)
                # State and cache are shared. Calls do not push matrices implicitly.
                if op == "branch":
                    ended = True
                    break
            elif op == "end":
                require_keys(command, {"op"})
                ended = True
                break
            else:
                raise PlanError(f"{name}:{pc}: unsupported normalized op {op!r}")
        if not ended:
            raise PlanError(f"list {name} lacks explicit end/branch")

    run(entry, ())
    return {"schema": 1, "kind": "normalized-vertex-history-plan", "entry": entry,
            "reachable_lists": sorted(used), "expanded_commands": steps,
            "vertex_versions": versions, "triangles": triangles,
            "mixed_position_triangles": sum(not t["single_position_transform"] for t in triangles),
            "scope": "semantic trace only; not a raw GBI decoder or a GX-ready stream"}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    try:
        if args.input.stat().st_size > 8 * 1024 * 1024:
            raise PlanError("input exceeds the 8 MiB teaching-tool limit")
        result = compile_plan(json.loads(args.input.read_text(encoding="utf-8")))
        text = json.dumps(result, indent=2, sort_keys=True) + "\n"
        # Nothing is written until parsing and complete reachable-plan validation finish.
        args.output.write_text(text, encoding="utf-8")
    except (OSError, ValueError, RecursionError) as exc:
        print(f"vertex-plan: {exc}", file=sys.stderr)
        return 2
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
