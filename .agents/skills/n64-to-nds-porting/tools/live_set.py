#!/usr/bin/env python3
"""Conservative whole-object closure/layout for an explicitly supplied graph.

This does NOT discover roots/edges from code, scan raw pointers, prove metadata
complete, relocate assets, write packed bytes, or authorize byte-range stripping.
Metadata must come from a source-aware extractor/review. Unknown reachable edges
fail closed. Python 3.10+, standard library only.
"""
from __future__ import annotations
import argparse
import json
from pathlib import Path
import sys
from typing import Any


class LiveSetError(ValueError):
    """Malformed or insufficient liveness metadata."""


def nat(value: Any, name: str, *, positive: bool = False) -> int:
    if type(value) is not int or value < int(positive) or value > 0xffffffff:
        raise LiveSetError(f"{name} must be {'positive' if positive else 'nonnegative'} uint32")
    return value


def name(value: Any, field: str) -> str:
    if not isinstance(value, str) or not value or len(value) > 256:
        raise LiveSetError(f"{field} must be a nonempty string <= 256 characters")
    return value


def analyze(document: Any) -> dict[str, Any]:
    required = {"schema", "roots_complete", "roots", "objects"}
    if not isinstance(document, dict) or set(document) != required:
        raise LiveSetError(f"expected exactly {sorted(required)}")
    if type(document["schema"]) is not int or document["schema"] != 1:
        raise LiveSetError("schema must be 1")
    if document["roots_complete"] is not True:
        raise LiveSetError("a complete externally justified root set is required")
    if not isinstance(document["roots"], list) or not document["roots"]:
        raise LiveSetError("supply explicit nonempty runtime roots")
    if not isinstance(document["objects"], list):
        raise LiveSetError("objects must be an array")
    objects: dict[str, dict[str, Any]] = {}
    for obj in document["objects"]:
        if not isinstance(obj, dict) or set(obj) != {"id", "size", "align", "edge_status", "edges"}:
            raise LiveSetError("object fields must be id,size,align,edge_status,edges")
        key = name(obj["id"], "object id")
        if key in objects:
            raise LiveSetError(f"duplicate object id: {key}")
        nat(obj["size"], "size", positive=True)
        alignment = nat(obj["align"], "align", positive=True)
        if alignment & (alignment - 1):
            raise LiveSetError("align must be a power of two")
        if obj["edge_status"] not in ("complete", "unknown"):
            raise LiveSetError("edge_status must be complete or unknown")
        if not isinstance(obj["edges"], list):
            raise LiveSetError("edges must be an array of object IDs")
        for edge in obj["edges"]:
            name(edge, "edge target")
        objects[key] = obj
    pending = [name(root, "root") for root in document["roots"]]
    kept: set[str] = set()
    while pending:
        key = pending.pop()
        if key in kept:
            continue
        if key not in objects:
            raise LiveSetError(f"missing reachable object: {key}")
        obj = objects[key]
        if obj["edge_status"] != "complete":
            raise LiveSetError(f"unknown references in reachable object: {key}; retain opaque closure or improve metadata")
        kept.add(key)
        pending.extend(obj["edges"])
    # Stable ID order is a reproducible example, not a claim about optimal locality.
    layout: list[dict[str, Any]] = []
    offset = 0
    maximum_alignment = 1
    for key in sorted(kept):
        obj = objects[key]
        alignment = obj["align"]
        maximum_alignment = max(maximum_alignment, alignment)
        offset = (offset + alignment - 1) & -alignment
        end = offset + obj["size"]
        if end > 0xffffffff:
            raise LiveSetError("layout exceeds uint32 addressable size")
        layout.append({"id": key, "offset": offset, "size": obj["size"], "align": alignment})
        offset = end
    kept_bytes = sum(objects[key]["size"] for key in kept)
    return {"schema": 1, "kind": "conditional-whole-object-live-set",
            "kept": sorted(kept), "dropped": sorted(set(objects) - kept),
            "kept_object_bytes": kept_bytes, "layout_bytes": offset,
            "alignment_padding_bytes": offset - kept_bytes,
            "required_base_alignment": maximum_alignment, "layout": layout,
            "assumptions": ["external root/edge metadata is complete and justified",
                            "input objects are canonical nonoverlapping ownership units",
                            "interior pointers, writes and identity dependencies are represented by edges"],
            "not_proven": ["metadata extraction", "relocation correctness", "byte-level deletability",
                           "runtime capacity including code/stacks/staging", "DS performance"]}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    try:
        if args.input.stat().st_size > 8 * 1024 * 1024:
            raise LiveSetError("input exceeds the 8 MiB teaching-tool limit")
        result = analyze(json.loads(args.input.read_text(encoding="utf-8")))
        args.output.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    except (OSError, ValueError, RecursionError) as exc:
        print(f"live-set: {exc}", file=sys.stderr)
        return 2
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
