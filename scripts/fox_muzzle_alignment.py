#!/usr/bin/env python3
"""Replay Fox's gun and beam X/Y through the exact DS fixed-point camera path.

Input is produced by scripts/probe-fox-muzzle-alignment.ps1. The gun point is
the source local muzzle/shot point {60,0,0} transformed by the production gun
MVP. The beam point is the weapon's live world translation transformed by the
same particle camera path used by ndsRendererSubmitFoxBlasterQuad. That submit's
no-Z projection changes clip Z only, so X/Y/W are unchanged.
"""

from __future__ import annotations

import argparse
import re
from pathlib import Path

WORLD_UNIT_SHIFT = 8
SCREEN_W = 256.0
SCREEN_H = 192.0


def round_shift(value: int, shift: int) -> int:
    if shift == 0:
        return value
    mag = -value if value < 0 else value
    mag = (mag + (1 << (shift - 1))) >> shift
    return -mag if value < 0 else mag


def mul(a: list[list[int]], b: list[list[int]]) -> list[list[int]]:
    out = [[0] * 4 for _ in range(4)]
    for row in range(4):
        for col in range(4):
            total = sum(a[row][k] * b[k][col] for k in range(4))
            out[row][col] = round_shift(total, 12)
    return out


def transform(m: list[list[int]], xyz_v16: tuple[int, int, int]) -> tuple[int, ...]:
    x, y, z = xyz_v16
    return tuple(
        (m[0][col] * x + m[1][col] * y + m[2][col] * z +
         m[3][col] * 4096) >> 12
        for col in range(4)
    )


def screen(clip: tuple[int, ...]) -> tuple[float, float]:
    if clip[3] <= 0:
        raise SystemExit(f"FAIL: point is behind camera: clip={clip}")
    return (
        (clip[0] / clip[3] + 1.0) * (SCREEN_W / 2.0),
        (1.0 - clip[1] / clip[3]) * (SCREEN_H / 2.0),
    )


def parse_dump(lines: list[str], marker: str) -> list[list[int]]:
    try:
        start = next(i for i, line in enumerate(lines)
                     if line.strip().startswith(marker))
    except StopIteration as exc:
        raise SystemExit(f"FAIL: missing marker {marker}") from exc
    words: list[int] = []
    for line in lines[start + 1:]:
        if line.startswith("FOXALIGN") and words:
            break
        hit = re.match(r"^0x[0-9a-fA-F]+(?:\s*<[^>]*>)?:\s*(.*)$", line.strip())
        if hit:
            vals = re.findall(r"-?\d+", hit.group(1))
            if len(vals) == 4:
                words.extend(int(v) for v in vals)
                if len(words) == 16:
                    break
    if len(words) != 16:
        raise SystemExit(f"FAIL: {marker} yielded {len(words)} matrix words")
    return [words[0:4], words[4:8], words[8:12], words[12:16]]


def parse_vec(lines: list[str], prefix: str) -> tuple[float, float, float]:
    for line in lines:
        if line.startswith(prefix):
            vals = line[len(prefix):].strip().split()
            if len(vals) == 3:
                return tuple(float(v) for v in vals)  # type: ignore[return-value]
    raise SystemExit(f"FAIL: missing vector {prefix}")


def first_spawn_frame(lines: list[str]) -> tuple[int, tuple[float, float, float]]:
    pattern = re.compile(
        r"^FOXALIGN SPAWN frame=(\d+)\b.*?\s"
        r"([-+0-9.eE]+)\s+([-+0-9.eE]+)\s+([-+0-9.eE]+)$"
    )
    for line in lines:
        hit = pattern.match(line.strip())
        if hit:
            return int(hit.group(1)), tuple(float(hit.group(i)) for i in range(2, 5))  # type: ignore[return-value]
    raise SystemExit("FAIL: no natural Fox spawn in probe")


def beam_for_frame(lines: list[str], frame: int) -> tuple[float, float, float]:
    pattern = re.compile(
        rf"^FOXALIGN BEAM frame={frame}\b.*?\s"
        r"([-+0-9.eE]+)\s+([-+0-9.eE]+)\s+([-+0-9.eE]+)\b"
    )
    for line in lines:
        hit = pattern.match(line.strip())
        if hit:
            return tuple(float(hit.group(i)) for i in range(1, 4))  # type: ignore[return-value]
    raise SystemExit(f"FAIL: no beam draw in spawn frame {frame}")


def presentation_point(lines: list[str]) -> tuple[float, float, float] | None:
    pattern = re.compile(
        r"^FOXALIGN GLOW_PRESENT 0\s+source=[^ ]+\s+draw="
        r"([-+0-9.eE]+),([-+0-9.eE]+),([-+0-9.eE]+)$"
    )
    for line in lines:
        hit = pattern.match(line.strip())
        if hit:
            return tuple(float(hit.group(i)) for i in range(1, 4))  # type: ignore[return-value]
    return None


def raw_modelview(m: list[list[int]]) -> list[list[int]]:
    out = [row[:] for row in m]
    out[3] = [round_shift(v, WORLD_UNIT_SHIFT) for v in out[3]]
    return out


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("probe", type=Path)
    args = ap.parse_args()
    lines = args.probe.read_text(encoding="utf-8", errors="replace").splitlines()

    frame, spawn = first_spawn_frame(lines)
    # Directly dereferencing the maker's r1 stack pointer is vulnerable to the
    # ARM9 D-cache/GDB coherency trap. Probe builds also copy that exact value
    # into ROM globals; prefer the delayed read when present.
    try:
        saved_spawn = parse_vec(lines, "FOXALIGN SPAWN_SAVED")
        if any(abs(v) > 0.0001 for v in saved_spawn):
            spawn = saved_spawn
    except SystemExit:
        pass
    display_spawn = presentation_point(lines) or spawn
    beam = beam_for_frame(lines, frame)
    # GUNCOMPOSED, not GUNMATRIX: the probe now dumps the fighter camera cache
    # between the two, and parse_dump takes the FIRST word block after its
    # marker, so anchoring on GUNMATRIX would silently read the camera as the
    # gun's composed matrix.
    gun_cpu = parse_dump(lines, f"FOXALIGN GUNCOMPOSED frame={frame}")
    beam_projection = parse_dump(lines, f"FOXALIGN BEAMCAM PROJECTION frame={frame}")
    beam_modelview = parse_dump(lines, f"FOXALIGN BEAMCAM MODELVIEW frame={frame}")

    # Gun vertices use source * 16 in DS v16. The production raw-composed
    # loader divides row 3 by 256 before loading it.
    gun_hw = raw_modelview(gun_cpu)
    gun_clip = transform(gun_hw, (60 * 16, 0, 0))
    gun_xy = screen(gun_clip)

    # The beam DObj has already advanced horizontally by the time it is drawn;
    # that is source weapon motion, not a muzzle offset. Its Y/Z must remain the
    # spawn Y/Z. To test the attachment seam rather than projectile travel,
    # project the presentation shot point through the exact camera the beam draw
    # uses on this same frame. Before the fixed-two resampling fix this is the
    # original source spawn. After it, GLOW_PRESENT records the one derived
    # translation that moves the hidden substep-0 event onto the visible muzzle;
    # gameplay/collision still use `spawn` unchanged.
    beam_mv = raw_modelview(beam_modelview)
    beam_mvp = mul(beam_mv, beam_projection)
    shot_v16 = tuple(int(v * 16.0) for v in display_spawn)
    beam_clip = transform(beam_mvp, shot_v16)  # type: ignore[arg-type]
    beam_xy = screen(beam_clip)

    world_yz_delta = (beam[1] - spawn[1], beam[2] - spawn[2])
    dx = beam_xy[0] - gun_xy[0]
    dy = beam_xy[1] - gun_xy[1]
    print(f"matched frame       : {frame}")
    print(f"source spawn world : {spawn[0]:.6f} {spawn[1]:.6f} {spawn[2]:.6f}")
    print(f"display shot world : {display_spawn[0]:.6f} {display_spawn[1]:.6f} {display_spawn[2]:.6f}")
    print(f"beam draw world    : {beam[0]:.6f} {beam[1]:.6f} {beam[2]:.6f}")
    print(f"beam-spawn Y/Z d   : {world_yz_delta[0]:+.6f} {world_yz_delta[1]:+.6f}")
    print(f"gun {{60,0,0}} px   : {gun_xy[0]:.4f} {gun_xy[1]:.4f}")
    print(f"shot via beam cam px: {beam_xy[0]:.4f} {beam_xy[1]:.4f}")
    print(f"shot-gun screen d   : dx={dx:+.4f}px dy={dy:+.4f}px")

    if max(abs(v) for v in world_yz_delta) > 0.01:
        print("VERDICT: FAIL -- weapon motion changed source Y/Z")
        return 1
    if abs(dx) > 0.25 or abs(dy) > 0.25:
        print("VERDICT: FAIL -- gun and beam use divergent X/Y camera transforms")
        return 1
    print("VERDICT: PASS -- source shot point, beam center, and visible gun agree in X/Y")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
