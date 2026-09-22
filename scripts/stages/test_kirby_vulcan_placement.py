#!/usr/bin/env python3
"""K03: where Kirby's Vulcan Jab effect is born, and how often it is mirrored.

Two independent invariants, both derived from the read-only source:

1.  **The five maker arguments must come from the real effect table.**
    ``ftCommonAttack100LoopKirbyUpdateEffect`` (decomp
    ``src/ft/ftcommon/ftcommonattack100.c:91``) does raw pointer arithmetic::

        (uintptr_t)gFTDataKirbyMainMotion + (intptr_t)&llKirbyMainMotionftKirbyAttack100Effect

    In BattleShip ``ll*`` names are link-time offset symbols, so ``&name`` IS
    the offset.  In this port they are ordinary ``uintptr_t`` objects
    (``include/ft/fighter.h`` declares them, ``src/port/diagnostics_mp_taskman_state.c``
    defines them) used only as identity TOKENS by ``ndsRelocGetFileData``'s
    address-keyed registry.  Taking ``&`` of one therefore yields a RAM
    address, not an offset, and every import TU whose included decomp source
    does this arithmetic must restore the offset with
    ``#define <symbol> NDS_RELOC_LVALUE(<offset>)``, exactly as
    ``battleship_gryamabuki_ground.c``, ``battleship_grpupupu_ground.c`` and
    the other stage imports already do.

    Without it, ``offset.x/y/z``, ``rotate``, ``vel`` and ``add`` are all read
    from outside the KirbyMainMotion file: the effect is born at a garbage
    joint-local offset and drifts with a garbage velocity.  That is the FIRST
    wrong value in this row -- upstream of every matrix.

2.  **Facing is applied exactly once per quantity.**
    The joint-local offset is mirrored by the TopN joint transform inside
    ``gmCollisionGetFighterPartsWorldPosition``; the rotation and velocity are
    mirrored by ``efManagerKirbyVulcanJabMakeEffect``'s ``lr == -1`` arm
    (negate ``rotate``/``vel``/``add``, plus ``rotate.y = 180 degrees`` on the
    effect root).  ``*pos`` is never negated by the maker.  A producer that
    also mirrors the offset double-mirrors the birth position.

Coordinate spaces: table entry = TopN-JOINT-LOCAL; after
``gmCollisionGetFighterPartsWorldPosition`` = WORLD; the maker stores WORLD in
the effect root's translate and its per-update velocity is WORLD too
(``efManagerKirbyVulcanJabProcUpdate`` adds it straight onto that translate).

Host only: no ROM, no emulator, no generated output is written.
"""

from __future__ import annotations

import math
import re
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(Path(__file__).resolve().parent))

DECOMP = REPO / "decomp" / "BattleShip-main"
ATTACK100 = DECOMP / "decomp" / "src" / "ft" / "ftcommon" / "ftcommonattack100.c"
EFMANAGER = DECOMP / "decomp" / "src" / "ef" / "efmanager.c"
RELOC_US = DECOMP / "include" / "reloc_data.us.h"
IMPORT_TU = REPO / "src" / "import" / "battleship_ftcommon_attack100.c"
IMPORT_DIR = REPO / "src" / "import"

SYMBOL = "llKirbyMainMotionftKirbyAttack100Effect"

# One source table entry, shape ftKirbyAttack100Effect (ftkirby.h:304):
# Vec3f offset; f32 rotate (degrees); f32 vel; f32 add.
SAMPLE_ENTRY = ((240.0, 610.0, 0.0), 35.0, 90.0, -12.0)
FIGHTER_POS = (1180.0, -260.0, 0.0)


def _strip_comments(text: str) -> str:
    text = re.sub(r"/\*.*?\*/", " ", text, flags=re.S)
    return re.sub(r"//[^\n]*", " ", text)


def _body(path: Path, signature: str) -> str:
    text = _strip_comments(path.read_text(encoding="utf-8", errors="replace"))
    match = re.search(re.escape(signature) + r".*?\n\}", text, re.S)
    assert match, f"{signature} not found in {path.name}"
    return match.group(0)


# --------------------------------------------------------------------------
# 1. Reloc offset restoration
# --------------------------------------------------------------------------

def source_offset_us() -> int:
    text = RELOC_US.read_text(encoding="utf-8", errors="replace")
    match = re.search(r"#define\s+" + SYMBOL + r"\s+\(\(intptr_t\)(0[xX][0-9a-fA-F]+|\d+)\)",
                      text)
    assert match, f"{SYMBOL} not in reloc_data.us.h"
    return int(match.group(1), 0)


def test_producer_uses_raw_reloc_symbol_arithmetic():
    body = _body(ATTACK100, "void ftCommonAttack100LoopKirbyUpdateEffect")
    assert "gFTDataKirbyMainMotion" in body
    assert f"(intptr_t)&{SYMBOL}" in " ".join(body.split())
    # It is NOT routed through the address-keyed resolver, so the resolver
    # cannot rescue it.
    assert "lbRelocGetFileData" not in body


def test_port_symbol_is_an_ordinary_object_not_a_link_time_offset():
    decl = (REPO / "include" / "ft" / "fighter.h").read_text(
        encoding="utf-8", errors="replace")
    assert f"extern uintptr_t {SYMBOL};" in decl
    defn = (REPO / "src" / "port" / "diagnostics_mp_taskman_state.c").read_text(
        encoding="utf-8", errors="replace")
    assert re.search(r"^uintptr_t\s+" + SYMBOL + r"\s*(=[^;]*)?;", defn, re.M)


def test_import_tu_restores_the_source_offset():
    """RED until the owed one-line diff lands in the import TU."""
    text = IMPORT_TU.read_text(encoding="utf-8", errors="replace")
    match = re.search(r"#define\s+" + SYMBOL + r"\s+NDS_RELOC_LVALUE\(\s*(0[xX][0-9a-fA-F]+|\d+)u?\s*\)",
                      text)
    assert match, (
        f"{IMPORT_TU.name} does not map {SYMBOL} back to a file offset; "
        f"(intptr_t)&{SYMBOL} compiles to a .bss address and the Vulcan Jab "
        "effect table is read from outside KirbyMainMotion")
    assert int(match.group(1), 0) == source_offset_us(), (
        "mapped offset does not match reloc_data.us.h")
    assert "NDS_RELOC_LVALUE(offset)" in text or "#define NDS_RELOC_LVALUE" in text, (
        "NDS_RELOC_LVALUE must be defined in this TU, as the stage imports do")


def test_no_other_import_tu_silently_shares_the_defect():
    """Report the whole family; this row only owns the attack100 entry."""
    inc = re.compile(r'#include\s+"(\.\./\.\./decomp/[^"]+\.c)"')
    arith = re.compile(r"[+-]\s*\(intptr_t\)\s*&\s*(ll[A-Za-z0-9_]+)")
    offenders: list[tuple[str, str]] = []
    for tu in sorted(IMPORT_DIR.glob("*.c")):
        text = tu.read_text(encoding="utf-8", errors="replace")
        defines = set(re.findall(r"#define\s+(ll[A-Za-z0-9_]+)\s+NDS_RELOC_LVALUE", text))
        for match in inc.finditer(text):
            src = REPO / match.group(1).replace("../../", "")
            if not src.exists():
                continue
            for symbol in set(arith.findall(
                    _strip_comments(src.read_text(encoding="utf-8", errors="replace")))):
                if symbol not in defines:
                    offenders.append((tu.name, symbol))
    mine = [row for row in offenders if row[1] == SYMBOL]
    assert not mine, f"attack100 entry still unmapped: {mine}"
    # Everything else is recorded, not asserted: other rows own those files.
    if offenders:
        print("    note: same defect class, not this row's files: " +
              ", ".join(f"{tu}:{sym}" for tu, sym in offenders))


# --------------------------------------------------------------------------
# 2. Facing applied exactly once
# --------------------------------------------------------------------------

def test_producer_does_not_mirror_the_joint_local_offset():
    body = " ".join(_body(
        ATTACK100, "void ftCommonAttack100LoopKirbyUpdateEffect").split())
    assert "pos.x = effect[fp->motion_vars.flags.flag2 - 1].offset.x;" in body
    assert "gmCollisionGetFighterPartsWorldPosition(fp->joints[nFTPartsJointTopN], &pos);" in body
    # fp->lr reaches the maker as an argument and nothing else.
    assert body.count("fp->lr") == 1
    assert "-pos" not in body and "pos.x *" not in body


def test_maker_mirrors_rotation_and_velocity_but_never_the_position():
    body = " ".join(_body(
        EFMANAGER, "GObj* efManagerKirbyVulcanJabMakeEffect").split())
    assert "dobj->translate.vec.f = *pos;" in body
    assert "if (lr == -1) { dobj->rotate.vec.f.y = F_CLC_DTOR32(180.0F); "\
           "rotate = -rotate; vel = -vel; add = -add; }" in body
    # Each quantity negated exactly once, position not at all.
    for quantity in ("rotate = -rotate;", "vel = -vel;", "add = -add;"):
        assert body.count(quantity) == 1, quantity
    assert "*pos = -" not in body and "pos->x = -" not in body


def topn_world(offset, facing, fighter_pos=FIGHTER_POS):
    """gmCollisionTransformMatrixAll + gmCollisionGetWorldPosition, for TopN.

    The fighter root joint carries rotate.y = pi when facing left, which is how
    the source mirrors every TopN-local offset exactly once.
    """
    ry = 0.0 if facing == 1 else math.pi
    sy, cy = math.sin(ry), math.cos(ry)
    # gmCollisionTransformMatrixAll rows, rotate.x = rotate.z = 0, scale 1.
    m = [[cy, 0.0, -sy],
         [0.0, 1.0, 0.0],
         [sy, 0.0, cy],
         list(fighter_pos)]
    return tuple(
        m[0][c] * offset[0] + m[1][c] * offset[1] + m[2][c] * offset[2] + m[3][c]
        for c in range(3))


def maker_velocity(rotate_deg, vel, add, facing):
    """efManagerKirbyVulcanJabMakeEffect's vel/add decomposition."""
    if facing == -1:
        rotate_deg, vel, add = -rotate_deg, -vel, -add
    theta = math.radians(rotate_deg)
    return ((vel * math.cos(theta), vel * math.sin(theta), 0.0),
            (add * math.cos(theta), add * math.sin(theta), 0.0))


def birth_and_drift(facing, *, mirror_offset_in_producer=False):
    offset, rotate_deg, vel, add = SAMPLE_ENTRY
    if mirror_offset_in_producer:
        offset = (offset[0] * facing, offset[1], offset[2])
    pos = topn_world(offset, facing)
    velocity, accel = maker_velocity(rotate_deg, vel, add, facing)
    # efManagerKirbyVulcanJabProcUpdate: 6 updates, v += a then p += v.
    trail = [pos]
    v = list(velocity)
    p = list(pos)
    for _ in range(6):
        v[0] += accel[0]
        p[0] += v[0]
        v[1] += accel[1]
        p[1] += v[1]
        trail.append(tuple(p))
    return trail


def _mirrored(right, left, axis_x=FIGHTER_POS[0]):
    return all(
        abs((axis_x - r[0]) - (l[0] - axis_x)) < 1e-6 and abs(r[1] - l[1]) < 1e-6
        for r, l in zip(right, left))


def test_single_mirror_gives_a_symmetric_trail():
    assert _mirrored(birth_and_drift(1), birth_and_drift(-1))


def test_reintroduced_double_mirror_is_red():
    """Deliberate mutation: the producer mirrors the offset as well."""
    right = birth_and_drift(1, mirror_offset_in_producer=True)
    left = birth_and_drift(-1, mirror_offset_in_producer=True)
    assert not _mirrored(right, left), (
        "the double-mirror mutation did not move the jab trail; the test "
        "cannot detect the defect it exists for")
    # And it is a whole offset-width error, not rounding.
    assert abs(left[0][0] - birth_and_drift(-1)[0][0]) > 1.0


def main() -> int:
    failures = 0
    for name, fn in sorted(globals().items()):
        if not name.startswith("test_") or not callable(fn):
            continue
        try:
            fn()
        except AssertionError as exc:
            failures += 1
            print(f"FAIL {name}: {exc}")
        else:
            print(f"ok   {name}")
    print("KIRBY_VULCAN_PLACEMENT_" + ("FAIL" if failures else "OK"))
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
