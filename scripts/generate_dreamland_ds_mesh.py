#!/usr/bin/env python3
"""Task 62 (Commit 1) — generate the runtime Dream Land DS-mesh data blob.

Consumes the Task 60 primitive stream + Task 61 encoded stream + Task 57 source
mesh for candidate c120, and emits a generated C include
``src/nds/dreamland_ds_mesh.generated.inc`` containing:
  - per-group descriptor table (prim, first-vertex, vertex-count, binding)
  - flat vertex stream: each entry is (opcode, rebased XYZ in s10.3) in group
    emission order, ready for the runtime to emit verbatim
  - the VERTEX10 rebasis (scale + origin) as the compensating-matrix constants
  - a certificate (checksums) the runtime validates at init

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
import struct
import sys
from pathlib import Path
from typing import Any, Sequence

import dreamland_primitive_compiler as pc
import dreamland_quantizer as q

SCRIPTS_DIR = Path(__file__).resolve().parent
REPO_ROOT = SCRIPTS_DIR.parent
GENERATED_DIR = SCRIPTS_DIR / "generated"
CANDIDATES_DIR = GENERATED_DIR / "candidates"
STREAMS_DIR = GENERATED_DIR / "primitive_streams"
DEFAULT_OUTPUT = REPO_ROOT / "src" / "nds" / "dreamland_ds_mesh.generated.inc"

# The candidate Task 62 integrates (Task 60/61 recommended).
CANDIDATE_NAME = "c120"

# DS primitive ids (libnds videoGL.h: GL_TRIANGLES=0, GL_QUAD=1, GL_TRIANGLE_STRIP=2).
# Mirrors dreamland_primitive_compiler.PRIM_*.
PRIM_TRIANGLES = 0
PRIM_QUAD = 1
PRIM_STRIP = 2
PRIM_NAME_TO_ID = {"GL_TRIANGLES": PRIM_TRIANGLES,
                   "GL_QUAD": PRIM_QUAD,
                   "GL_TRIANGLE_STRIP": PRIM_STRIP}

# DS vertex opcode ids (GBATEK) — mirrors dreamland_quantizer.
OP_VERTEX16 = 0x23
OP_VERTEX10 = 0x2D
OP_VERTEX_XY = 0x21
OP_VERTEX_XZ = 0x22
OP_VERTEX_YZ = 0x24


def fnv1a_u32(words: Sequence[int], seed: int = 2166136261) -> int:
    value = seed
    for word in words:
        word &= 0xFFFFFFFF
        for shift in (0, 8, 16, 24):
            value ^= (word >> shift) & 0xFF
            value = (value * 16777619) & 0xFFFFFFFF
    return value


def load_candidate(name: str) -> tuple[list[tuple[float, float, float]],
                                       list[tuple[int, int, int]], list[int]]:
    ir = json.loads((CANDIDATES_DIR / f"{name}.json").read_text(encoding="utf-8"))
    positions = [(v["world_x_f"], v["world_y_f"], v["world_z_f"])
                 for v in ir["world_vertices"]]
    triangles = [(t["v0"], t["v1"], t["v2"]) for t in ir["triangles"]]
    bindings = [v["binding_index"] for v in ir["world_vertices"]]
    return positions, triangles, bindings


def load_stream(name: str) -> list[dict[str, Any]]:
    return json.loads((STREAMS_DIR / f"{name}.json").read_text(encoding="utf-8"))["groups"]


def build_blob(name: str) -> dict[str, Any]:
    """Build the runtime blob: groups + flat encoded vertex stream + rebasis."""
    positions, triangles, bindings = load_candidate(name)
    stream_groups = load_stream(name)

    # VERTEX10 rebasis over the candidate's positions.
    rebasis = q.find_v10_rebasis(positions)
    if rebasis is None:
        raise ValueError(f"{name}: VERTEX10 rebasis infeasible")
    scale, origin = rebasis

    # Walk groups in emission order, selecting the cheapest legal opcode per
    # vertex (mirrors dreamland_quantizer.select_vertex_opcode). Each emitted
    # vertex carries its opcode + rebased s10.3 coords.
    flat: list[dict[str, Any]] = []   # per-emission: opcode, x, y, z (rebased)
    groups: list[dict[str, Any]] = []  # per-group: prim, first, count, binding
    prev: tuple[float, float, float] | None = None
    for g in stream_groups:
        prim_id = PRIM_NAME_TO_ID[g["prim"]]
        verts = g["verts"]
        first = len(flat)
        for vi in verts:
            x, y, z = positions[vi]
            ev = q.select_vertex_opcode(x, y, z, prev, True, scale, origin)
            # Rebased s10.3 coords (quantized), for the runtime to emit verbatim.
            if ev.opcode == q.OP_VERTEX10:
                sx = q._quantize_v10((x - origin[0]) / scale)
                sy = q._quantize_v10((y - origin[1]) / scale)
                sz = q._quantize_v10((z - origin[2]) / scale)
            elif ev.opcode == q.OP_VERTEX_XZ:
                sx = q._quantize_v10((x - origin[0]) / scale)
                sz = q._quantize_v10((z - origin[2]) / scale)
                sy = 0
            elif ev.opcode == q.OP_VERTEX_XY:
                sx = q._quantize_v10((x - origin[0]) / scale)
                sy = q._quantize_v10((y - origin[1]) / scale)
                sz = 0
            elif ev.opcode == q.OP_VERTEX_YZ:
                sy = q._quantize_v10((y - origin[1]) / scale)
                sz = q._quantize_v10((z - origin[2]) / scale)
                sx = 0
            else:  # VERTEX16 fallback (s16.12)
                sx = int(round(x * 4096))
                sy = int(round(y * 4096))
                sz = int(round(z * 4096))
            flat.append({"opcode": ev.opcode, "x": sx, "y": sy, "z": sz})
            prev = ev.decoded
        groups.append({
            "prim": prim_id, "first": first, "count": len(verts),
            "binding": g["binding"],
        })

    # Certificate: FNV1a over the group table + flat vertex stream + rebasis.
    cert_words: list[int] = []
    for g in groups:
        cert_words += [g["prim"], g["first"], g["count"], g["binding"]]
    for v in flat:
        cert_words += [v["opcode"], v["x"] & 0xFFFF, v["y"] & 0xFFFF, v["z"] & 0xFFFF]
    # Rebasis as s20.12 fixed point.
    scale_s20p12 = int(round(scale * 4096))
    origin_s20p12 = [int(round(o * 4096)) for o in origin]
    cert_words += [scale_s20p12] + origin_s20p12
    certificate = fnv1a_u32(cert_words)

    return {
        "candidate": name,
        "groups": groups,
        "vertices": flat,
        "rebasis_scale_s20p12": scale_s20p12,
        "rebasis_origin_s20p12": origin_s20p12,
        "certificate": certificate,
        "position_count": len(positions),
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


def c_s32(value: int) -> str:
    v = value & 0xFFFFFFFF
    if v >= 0x80000000:
        v -= 0x100000000
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
    lines.append(" * the Task 60 primitive stream + Task 61 encoded stream. The runtime")
    lines.append(" * emits this verbatim under NDS_DREAMLAND_DS_MESH; default-off keeps")
    lines.append(" * the shipping segment0 path byte-identical.")
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
    lines.append("/* DS vertex opcode ids (GBATEK). */")
    lines.append("#define NDS_DREAMLAND_DS_OP_VERTEX16   0x23u")
    lines.append("#define NDS_DREAMLAND_DS_OP_VERTEX10   0x2Du")
    lines.append("#define NDS_DREAMLAND_DS_OP_VERTEX_XY  0x21u")
    lines.append("#define NDS_DREAMLAND_DS_OP_VERTEX_XZ  0x22u")
    lines.append("#define NDS_DREAMLAND_DS_OP_VERTEX_YZ  0x24u")
    lines.append("")
    lines.append("#define NDS_DREAMLAND_DS_GROUP_COUNT "
                 + str(len(g)) + "u")
    lines.append("#define NDS_DREAMLAND_DS_VERTEX_COUNT "
                 + str(len(v)) + "u")
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
    lines.append("static const u8 sNdsDreamLandDSGroupVertexCount["
                 + str(len(g)) + "] = {")
    for i in range(0, len(g), 12):
        lines.append("    " + ", ".join(str(gr["count"]) + "u" for gr in g[i:i+12]) + ",")
    lines.append("};")
    lines.append("")

    # Flat vertex stream: opcode + rebased XYZ per emission.
    lines.append("/* Per-emission vertex: opcode (u8) + rebased s10.3 XYZ (s16 each). */")
    lines.append("static const u8 sNdsDreamLandDSVertexOpcode["
                 + str(len(v)) + "] = {")
    for i in range(0, len(v), 16):
        lines.append("    " + ", ".join(hex(vv["opcode"]) for vv in v[i:i+16]) + ",")
    lines.append("};")
    lines.append("")
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

    # Rebasis compensating-matrix constants (s20.12).
    lines.append("/* VERTEX10 rebasis: world = rebased * scale + origin. */")
    lines.append("#define NDS_DREAMLAND_DS_REBASIS_SCALE_S20P12  "
                 + c_s32(blob["rebasis_scale_s20p12"]))
    lines.append("#define NDS_DREAMLAND_DS_REBASIS_ORIGIN_X_S20P12 "
                 + c_s32(blob["rebasis_origin_s20p12"][0]))
    lines.append("#define NDS_DREAMLAND_DS_REBASIS_ORIGIN_Y_S20P12 "
                 + c_s32(blob["rebasis_origin_s20p12"][1]))
    lines.append("#define NDS_DREAMLAND_DS_REBASIS_ORIGIN_Z_S20P12 "
                 + c_s32(blob["rebasis_origin_s20p12"][2]))
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
    blob = build_blob(args.candidate)
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
