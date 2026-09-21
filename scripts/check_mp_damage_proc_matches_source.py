#!/usr/bin/env python3
"""Prove the knockback collision proc still tests walls and ceilings.

`ndsMPCommonProcFighterDamage` in `src/port/reloc_backend_compat_shims.c` is
the port's `mpCommonProcFighterDamage` (BattleShip `src/mp/mpcommon.c`). It was
shipped for a long time as a floor-only body -- its own comment said the wall
and ceiling branches "remain deferred" -- and it additionally masked LWALL,
RWALL and CEIL out of `mask_stat` on the way past. A fighter in any damage
status therefore had no wall or ceiling collision at all, which is BUGS.md S02:
Castle's blue upper side ramps failing during knockback and falling while the
same ramps behave when walked on.

Those branches are back. This checker exists because the failure mode is a
DELETION, and a deletion leaves nothing behind to notice: the port still
compiles, still runs, still lands on floors, and the only visible symptom is a
fighter passing through geometry at speed in one specific state. So assert the
structure directly against the decomp:

  * all three source tests are called, in the source's order;
  * each one's decision keeps BOTH arms -- the 30-unit displacement and
    110-degree approach that end the sub-step loop, and the `coll_mask_ignore`
    arm that lets a grazing contact continue -- because dropping either turns a
    graze into a full stop or a slam into a pass-through;
  * the mask-clearing lines that used to suppress the three flags are gone;
  * the source function this mirrors still has the shape being asserted, so a
    decomp refresh that changes it fails here instead of silently diverging.

This is a structural check, not a numerical one. The arithmetic inside the
branches is BattleShip's, called through the same helpers; what is worth
pinning is that the branches are present and complete.
"""

from __future__ import annotations

import pathlib
import re
import sys

REPO = pathlib.Path(__file__).resolve().parents[1]
PORT = REPO / "src/port/reloc_backend_compat_shims.c"
SOURCE = REPO / "decomp/BattleShip-main/decomp/src/mp/mpcommon.c"

PORT_FUNCTION = "ndsMPCommonProcFighterDamage"
SOURCE_FUNCTION = "mpCommonProcFighterDamage"

# The three source tests, in the order mpCommonProcFighterDamage runs them.
ORDERED_TESTS = (
    "mpProcessCheckTestLWallCollisionAdjNew",
    "mpProcessCheckTestRWallCollisionAdjNew",
    "mpProcessCheckTestCeilCollisionAdjNew",
    "mpProcessRunFloorCollisionAdjNewNULL",
)

# Each test's resolver, and the angle field its 110-degree comparison reads.
BRANCH_PARTS = {
    "LWall": ("mpProcessRunLWallCollisionAdjNew", "lwall_angle", "MAP_FLAG_LWALL"),
    "RWall": ("mpProcessRunRWallCollisionAdjNew", "rwall_angle", "MAP_FLAG_RWALL"),
    "Ceil": ("mpProcessRunCeilCollisionAdjNew", "ceil_angle", "MAP_FLAG_CEIL"),
}

# Lines whose reappearance would re-suppress the flags the branches set.
FORBIDDEN = (
    r"mask_unk\s*&=\s*\(u16\)~\(MAP_FLAG_LWALL",
    r"mask_stat\s*&=\s*\(u16\)~\(MAP_FLAG_LWALL",
)


def extract_body(text: str, signature_fragment: str, path: pathlib.Path) -> str:
    """The brace-balanced body of the first function whose header matches."""
    start = text.find(signature_fragment)
    if start < 0:
        raise SystemExit(f"{path}: cannot find {signature_fragment}")
    opening = text.index("{", start)
    depth = 1
    index = opening + 1
    while depth:
        depth += (text[index] == "{") - (text[index] == "}")
        index += 1
    return text[opening + 1:index - 1]


def strip_comments(text: str) -> str:
    """Comments quote the source freely; assertions must read real code."""
    text = re.sub(r"/\*.*?\*/", " ", text, flags=re.S)
    return re.sub(r"//[^\n]*", " ", text)


def check_ordering(body: str, label: str, failures: list[str]) -> None:
    positions = []
    for name in ORDERED_TESTS:
        index = body.find(name)
        if index < 0:
            failures.append(f"{label}: {name} is not called")
            return
        positions.append((name, index))
    for (first, first_at), (second, second_at) in zip(positions, positions[1:]):
        if first_at >= second_at:
            failures.append(
                f"{label}: {first} must run before {second}")


def check_branches(body: str, label: str, failures: list[str]) -> None:
    for branch, (resolver, angle_field, flag) in BRANCH_PARTS.items():
        if resolver not in body:
            failures.append(f"{label}: {branch} never calls {resolver}")
            continue
        window_start = body.index(resolver)
        window = body[window_start:window_start + 1400]
        if angle_field not in window:
            failures.append(
                f"{label}: {branch} does not compare against {angle_field}")
        if "30.0F" not in window:
            failures.append(
                f"{label}: {branch} lost the 30-unit displacement gate")
        if "110.0F" not in window:
            failures.append(
                f"{label}: {branch} lost the 110-degree approach gate")
        if "lbCommonMag2D" not in window:
            failures.append(
                f"{label}: {branch} lost the displacement magnitude")
        if "syVectorAngleDiff3D" not in window:
            failures.append(
                f"{label}: {branch} lost the approach-angle comparison")
        if f"coll_mask_curr |= {flag}" not in window.replace("\n", " ").replace(
                "  ", " "):
            # The port wraps lines; normalise whitespace before comparing.
            squashed = re.sub(r"\s+", " ", window)
            if f"coll_mask_curr |= {flag}" not in squashed:
                failures.append(
                    f"{label}: {branch} does not record {flag} on a real hit")
        squashed = re.sub(r"\s+", " ", window)
        if f"coll_mask_ignore |= {flag}" not in squashed:
            failures.append(
                f"{label}: {branch} lost the ignore arm -- a grazing contact "
                "would become a full stop")
        if "is_coll_end = TRUE" not in squashed:
            failures.append(
                f"{label}: {branch} does not end the sub-step loop on a hit")


def main() -> int:
    failures: list[str] = []

    port_text = PORT.read_text(encoding="utf-8", errors="replace")
    port_body = strip_comments(
        extract_body(port_text, f"sb32 {PORT_FUNCTION}(", PORT))
    check_ordering(port_body, "port", failures)
    check_branches(port_body, "port", failures)
    for pattern in FORBIDDEN:
        if re.search(pattern, port_body):
            failures.append(
                f"port: {PORT_FUNCTION} masks wall/ceiling flags out again "
                f"({pattern})")

    if SOURCE.is_file():
        source_body = strip_comments(
            extract_body(source_text := SOURCE.read_text(
                encoding="utf-8", errors="replace"),
                f"sb32 {SOURCE_FUNCTION}(", SOURCE))
        del source_text
        check_ordering(source_body, "decomp", failures)
        check_branches(source_body, "decomp", failures)
    else:
        print(f"note: {SOURCE} absent; checked the port only", file=sys.stderr)

    if failures:
        for failure in failures:
            print(f"FAIL {failure}", file=sys.stderr)
        return 1
    print(
        "damage proc check passed: left wall, right wall, ceiling and floor "
        "run in source order; all three non-floor branches keep both the "
        "end-the-loop and the ignore arm; no flag suppression.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
