#!/usr/bin/env python3
"""Generate the material-complete Dream Land DS static-mesh candidate blob.

Consumes Task 57's exact world mesh and emits:
  - one triangle group per original render run
  - source run identity for the already-prepared texture/material state
  - per-emission source-world XYZ and source dense-vertex identity
  - one uniform scale-2 matrix that restores source-world size
  - a deterministic certificate

Convention (matches the proven segment0 path, ndsRendererNativeStageEmitVertex):
the Dream Land world coords reach 3816, while ``coord << 4`` admits source
coordinates [-2048, 2047]. Dividing by two makes the complete mesh fit; a
uniform scale-2 matrix restores it before the shared hierarchy camera. That
helper divides camera translation by 256, so both sides use the renderer's
source-world / 256 hardware convention once.

This control intentionally keeps every static source triangle. It is the
visual-correctness gate for later reduction: a simplified mesh is not eligible
until it can supply the same material, UV, color, alpha, and submit semantics.

Host-only. ``--check`` rebuilds and compares the include deterministically
(same contract as generate_nds_native_stage); the Makefile runs it with
``--check`` so a stale blob fails the build.

All Task 62 runtime code is gated behind ``NDS_DREAMLAND_DS_MESH``, so at the
default (0) this include is not compiled and the published ROM is byte-identical.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import sys
from pathlib import Path
from typing import Any, Sequence
import sys as _sys
from pathlib import Path as _Path

_scripts_root = _Path(__file__).resolve().parent
while _scripts_root.name != "scripts":
    _scripts_root = _scripts_root.parent
if str(_scripts_root) not in _sys.path:
    _sys.path.insert(0, str(_scripts_root))
import _paths  # noqa: E402  -- puts every scripts/ area folder on sys.path


SCRIPTS_DIR = _paths.SCRIPTS_ROOT
REPO_ROOT = SCRIPTS_DIR.parent
GENERATED_DIR = SCRIPTS_DIR / "generated"
DEFAULT_OUTPUT = REPO_ROOT / "src" / "nds" / "dreamland_ds_mesh.generated.inc"

CANDIDATE_NAME = "c120"
CANDIDATES_DIR = GENERATED_DIR / "candidates"

# DS primitive ids (libnds videoGL.h: GL_TRIANGLES=0, GL_QUAD=1, GL_TRIANGLE_STRIP=2).
# Mirrors dreamland_primitive_compiler.PRIM_*.
PRIM_TRIANGLES = 0
PRIM_QUAD = 1
PRIM_STRIP = 2
PRIM_NAME_TO_ID = {"GL_TRIANGLES": PRIM_TRIANGLES,
                   "GL_QUAD": PRIM_QUAD,
                   "GL_TRIANGLE_STRIP": PRIM_STRIP}

SUBMIT_RAW_Z = 0
SUBMIT_PROJECTED_NO_Z = 3
SUBMIT_PROJECTED_RANGE_OR_MATRIX = 6
SUBMIT_REJECT = 7
SUPPORTED_SUBMIT_CLASSES = {
    SUBMIT_RAW_Z,
    SUBMIT_PROJECTED_NO_Z,
    SUBMIT_PROJECTED_RANGE_OR_MATRIX,
}

def fnv1a_u32(words: Sequence[int], seed: int = 2166136261) -> int:
    value = seed
    for word in words:
        word &= 0xFFFFFFFF
        for shift in (0, 8, 16, 24):
            value ^= (word >> shift) & 0xFF
            value = (value * 16777619) & 0xFFFFFFFF
    return value


def build_blob(name: str, repo_root: Path = REPO_ROOT) -> dict[str, Any]:
    """Build one material-qualified, binding-local static vertex stream."""
    source = json.loads(
        (CANDIDATES_DIR / f"{name}.json").read_text(encoding="utf-8")
    )
    if not source.get("material_qualified", False):
        raise ValueError(f"{name}: candidate is not material-qualified")
    vertices = source["world_vertices"]
    triangles = source["triangles"]
    flat: list[dict[str, Any]] = []
    source_dense: list[int] = []
    groups: list[dict[str, Any]] = []

    for tri in triangles:
        submit_class = int(tri["submit_class"])
        source_run = int(tri["run_index"])
        source_segment = int(tri["source_segment"])
        if submit_class not in SUPPORTED_SUBMIT_CLASSES:
            raise ValueError(
                f"run {tri['run_index']}: unsupported submit class "
                f"{submit_class}")
        if (not groups or
                groups[-1]["source_run"] != source_run or
                groups[-1]["submit_class"] != submit_class):
            groups.append({
                "prim": PRIM_TRIANGLES,
                "first": len(flat),
                "count": 0,
                "source_run": source_run,
                "source_segment": source_segment,
                "submit_class": submit_class,
            })
        group = groups[-1]
        for key in ("v0", "v1", "v2"):
            vertex = vertices[int(tri[key])]
            dense_indices = [int(index) for index
                             in vertex["source_dense_indices"]]
            if (not dense_indices or
                    not all(0 <= index <= 0xFFFF for index in dense_indices)):
                raise ValueError(
                    f"run {tri['run_index']}: invalid dense provenance")
            coords = [int(vertex[axis])
                      for axis in ("local_x", "local_y", "local_z")]
            if not all(-32768 <= coordinate <= 32767 for coordinate in coords):
                raise ValueError(
                    f"run {tri['run_index']}: local coordinate exceeds s16: "
                    f"{tuple(coords)}")
            flat.append({
                "x": coords[0],
                "y": coords[1],
                "z": coords[2],
                "source_first": len(source_dense),
                "source_count": len(dense_indices),
            })
            source_dense.extend(dense_indices)
            group["count"] += 1

    # Certificate: FNV1a over the group table, local vertices, and provenance.
    cert_words: list[int] = []
    for g in groups:
        cert_words += [
            g["prim"], g["first"], g["count"], g["source_run"],
            g["source_segment"], g["submit_class"],
        ]
    for v in flat:
        cert_words += [
            v["x"] & 0xFFFF,
            v["y"] & 0xFFFF,
            v["z"] & 0xFFFF,
            v["source_first"],
            v["source_count"],
        ]
    cert_words.extend(source_dense)
    certificate = fnv1a_u32(cert_words)

    return {
        "candidate": name,
        "groups": groups,
        "vertices": flat,
        "source_dense": source_dense,
        "certificate": certificate,
        "position_count": len(vertices),
        "triangle_count": len(triangles),
    }


# ---------------------------------------------------------------------------
# C rendering
# ---------------------------------------------------------------------------

def c_s16(value: int) -> str:
    v = value & 0xFFFF
    if v >= 0x8000:
        v -= 0x10000
    return str(v)


def render_include(blob: dict[str, Any]) -> bytes:
    g = blob["groups"]
    v = blob["vertices"]
    lines: list[str] = []
    lines.append("/*")
    lines.append(" * Task 62 — generated Dream Land DS-mesh runtime data.")
    lines.append(" *")
    lines.append(" * Candidate: " + blob["candidate"] + " (" + str(blob["position_count"])
                 + " positions, " + str(blob["triangle_count"]) + " source triangles).")
    lines.append(" * Compiled host-side by scripts/generate_dreamland_ds_mesh.py from")
    lines.append(" * Task 57's exact world mesh. Source run and dense-vertex identities")
    lines.append(" * preserve prepared texture, UV, color, alpha, and depth semantics.")
    lines.append(" *")
    lines.append(" * DO NOT EDIT — regenerate with: python scripts/generate_dreamland_ds_mesh.py")
    lines.append(" */")
    lines.append("#ifndef NDS_DREAMLAND_DS_MESH_GENERATED_INC")
    lines.append("#define NDS_DREAMLAND_DS_MESH_GENERATED_INC")
    lines.append("")
    lines.append("#if NDS_DREAMLAND_DS_MESH")
    lines.append("")
    lines.append("/* DS primitive ids (libnds videoGL.h). */")
    lines.append("#define NDS_DREAMLAND_DS_PRIM_TRIANGLES       0u")
    lines.append("#define NDS_DREAMLAND_DS_PRIM_QUAD            1u")
    lines.append("#define NDS_DREAMLAND_DS_PRIM_TRIANGLE_STRIP  2u")
    lines.append("")
    lines.append("#define NDS_DREAMLAND_DS_GROUP_COUNT "
                 + str(len(g)) + "u")
    lines.append("#define NDS_DREAMLAND_DS_VERTEX_COUNT "
                 + str(len(v)) + "u")
    lines.append("#define NDS_DREAMLAND_DS_TRIANGLE_COUNT "
                 + str(blob["triangle_count"]) + "u")
    lines.append("")

    # Group descriptor table.
    lines.append("static const u8 sNdsDreamLandDSGroupPrim["
                 + str(len(g)) + "] = {")
    for i in range(0, len(g), 12):
        lines.append("    " + ", ".join(str(gr["prim"]) + "u" for gr in g[i:i+12]) + ",")
    lines.append("};")
    lines.append("")
    lines.append("static const u16 sNdsDreamLandDSGroupFirstVertex["
                 + str(len(g)) + "] = {")
    for i in range(0, len(g), 8):
        lines.append("    " + ", ".join(str(gr["first"]) + "u" for gr in g[i:i+8]) + ",")
    lines.append("};")
    lines.append("")
    lines.append("static const u16 sNdsDreamLandDSGroupVertexCount["
                 + str(len(g)) + "] = {")
    for i in range(0, len(g), 12):
        lines.append("    " + ", ".join(str(gr["count"]) + "u" for gr in g[i:i+12]) + ",")
    lines.append("};")
    lines.append("")

    lines.append("static const u8 sNdsDreamLandDSGroupSubmitClass["
                 + str(len(g)) + "] = {")
    for i in range(0, len(g), 12):
        lines.append("    " + ", ".join(
            str(gr["submit_class"]) + "u" for gr in g[i:i+12]) + ",")
    lines.append("};")
    lines.append("")

    lines.append("static const u8 sNdsDreamLandDSGroupSourceRun["
                 + str(len(g)) + "] = {")
    for i in range(0, len(g), 12):
        lines.append("    " + ", ".join(
            str(gr["source_run"]) + "u" for gr in g[i:i+12]) + ",")
    lines.append("};")
    lines.append("")

    lines.append("static const u8 sNdsDreamLandDSGroupSourceSegment["
                 + str(len(g)) + "] = {")
    for i in range(0, len(g), 12):
        lines.append("    " + ", ".join(
            str(gr["source_segment"]) + "u" for gr in g[i:i+12]) + ",")
    lines.append("};")
    lines.append("")

    lines.append("/* Per-emission binding-local XYZ (S20.12 source units). */")
    lines.append("static const s16 sNdsDreamLandDSVertexX["
                 + str(len(v)) + "] = {")
    for i in range(0, len(v), 12):
        lines.append("    " + ", ".join(c_s16(vv["x"]) for vv in v[i:i+12]) + ",")
    lines.append("};")
    lines.append("")
    lines.append("static const s16 sNdsDreamLandDSVertexY["
                 + str(len(v)) + "] = {")
    for i in range(0, len(v), 12):
        lines.append("    " + ", ".join(c_s16(vv["y"]) for vv in v[i:i+12]) + ",")
    lines.append("};")
    lines.append("")
    lines.append("static const s16 sNdsDreamLandDSVertexZ["
                 + str(len(v)) + "] = {")
    for i in range(0, len(v), 12):
        lines.append("    " + ", ".join(c_s16(vv["z"]) for vv in v[i:i+12]) + ",")
    lines.append("};")
    lines.append("")

    lines.append("static const u16 sNdsDreamLandDSSourceFirst["
                 + str(len(v)) + "] = {")
    for i in range(0, len(v), 12):
        lines.append("    " + ", ".join(
            str(vv["source_first"]) + "u" for vv in v[i:i+12]) + ",")
    lines.append("};")
    lines.append("")

    lines.append("static const u8 sNdsDreamLandDSSourceCount["
                 + str(len(v)) + "] = {")
    for i in range(0, len(v), 12):
        lines.append("    " + ", ".join(
            str(vv["source_count"]) + "u" for vv in v[i:i+12]) + ",")
    lines.append("};")
    lines.append("")

    source_dense = blob["source_dense"]
    lines.append("#define NDS_DREAMLAND_DS_SOURCE_DENSE_COUNT "
                 + str(len(source_dense)) + "u")
    lines.append("static const u16 sNdsDreamLandDSSourceDense["
                 + str(len(source_dense)) + "] = {")
    for i in range(0, len(source_dense), 12):
        lines.append("    " + ", ".join(
            str(index) + "u" for index in source_dense[i:i+12]) + ",")
    lines.append("};")
    lines.append("")

    # Certificate.
    lines.append("#define NDS_DREAMLAND_DS_CERTIFICATE  0x{:08X}u".format(blob["certificate"]))
    lines.append("")
    lines.append("#endif /* NDS_DREAMLAND_DS_MESH */")
    lines.append("#endif /* NDS_DREAMLAND_DS_MESH_GENERATED_INC */")
    lines.append("")
    return "\n".join(lines).encode("utf-8")


def include_sha256(payload: bytes) -> str:
    return hashlib.sha256(payload).hexdigest()


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def parse_args(argv: Sequence[str] | None = None) -> argparse.Namespace:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--repo-root", type=Path, default=REPO_ROOT)
    p.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    p.add_argument("--candidate", default=CANDIDATE_NAME)
    p.add_argument("--check", action="store_true",
                   help="compare the existing include without writing it")
    return p.parse_args(argv)


def main(argv: Sequence[str] | None = None) -> int:
    args = parse_args(argv)
    output = args.output
    if not output.is_absolute():
        output = args.repo_root / output
    blob = build_blob(args.candidate, args.repo_root)
    rendered = render_include(blob)
    if args.check:
        if not output.is_file():
            print(f"TASK62-BLOB: FAIL — include absent: {output}", file=sys.stderr)
            return 1
        if output.read_bytes() != rendered:
            print(f"TASK62-BLOB: FAIL — include stale: {output}", file=sys.stderr)
            return 1
        print(f"TASK62-BLOB: OK ({args.candidate}: {len(blob['groups'])} groups, "
              f"{len(blob['vertices'])} verts, sha256={include_sha256(rendered)[:12]})")
        return 0
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(rendered)
    print(f"TASK62-BLOB: wrote {output}")
    print(f"TASK62-BLOB: {args.candidate} -> {len(blob['groups'])} groups, "
          f"{len(blob['vertices'])} verts, cert=0x{blob['certificate']:08X}")
    print(f"TASK62-BLOB: sha256 = {include_sha256(rendered)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
