#!/usr/bin/env python3
"""L01: the DamageSlash effect root must apply its world contact exactly once.

The source facts this test derives (it parses them, it does not restate them):

* ``dEFManagerDamageSlashEffectDesc``'s FIRST transform triple is
  ``{ 0x28, 0x45, 0x00 }`` (decomp ``src/ef/efmanager.c``).  Both are non-null,
  so ``gcAddDObj3TransformsKind`` (``src/sys/objanim.c:2208``) gives the effect
  ROOT DObj two XObjs, in that table order.
* ``efManagerDamageSlashMakeEffect`` writes the WORLD-SPACE collision contact
  (``gmCollisionGetFighterAttackDamagePosition``'s output, via
  ``ftMainProcessHitCollisionStatsMain``) into that same root's ``translate``.
* Kind ``0x28`` is decimal 40, ``gcPrepDObjMatrix``'s
  ``func_80010918(mtx, dobj, TRUE)``: a camera-facing basis whose translation
  row IS ``dobj->translate``.
* Kind ``0x45`` is decimal 69, i.e. ``dLBCommonFuncMatrixList`` pair
  ``69 - 66 == 3`` == ``lbCommonRotScaFuncMatrix``, which calls
  ``lbCommonMatrixRotSca`` and writes a ZERO translation row.

So exactly one of the root's two XObjs carries the contact.  Coordinate spaces:
the maker is handed WORLD, the billboard consumes WORLD, and the RotSca XObj is
a pure orientation/scale operator with NO space of its own.  A builder that
gives kind 0x45 a translation applies WORLD twice.

The test composes the two XObjs the way ``ndsRendererAdapterBuildDObjLocalMatrix``
composes them (later XObj first against the vertex) and requires the composed
translation row to equal the contact exactly.  ``test_reintroduced_double_translate_is_red``
performs the deliberate mutation -- kind 0x45 rebuilt with the DObj translate,
which is precisely the pre-repair ``ndsRendererAdapterBuildDObjFallbackMtx``
arm -- and requires it to fail.

Host only: no ROM, no emulator, no generated output is written.
"""

from __future__ import annotations

import re
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO / "scripts" / "stages"))

import native_matrix_math as matrix  # noqa: E402

DECOMP = REPO / "decomp" / "BattleShip-main" / "decomp" / "src"
EFMANAGER = DECOMP / "ef" / "efmanager.c"
LBCOMMON = DECOMP / "lb" / "lbcommon.c"
ADAPTER = REPO / "src" / "port" / "renderer_adapter_matrix.c"

# gcPrepDObjMatrix dispatches dLBCommonFuncMatrixList from kind 66
# (sys/objdisplay.c:432-437: "if (xobj->kind >= 66)"), two entries per kind.
CUSTOM_MATRIX_KIND_BASE = 66

SINT = matrix._load_sint_table(REPO)

# A non-origin contact, a non-zero slash angle and the size-derived xy scale
# efManagerDamageSlashMakeEffect computes for damage 12:
#     scale = ((12 - 5) * 0.18) + 1.0
CONTACT = (1523.5, -318.25, 40.0)
SLASH_ROTATE = (0.0, 0.0, 0.8517)
SLASH_SCALE = (2.26, 2.26, 1.0)


def _source_text(path: Path) -> str:
    return path.read_text(encoding="utf-8", errors="replace")


def _strip_comments(text: str) -> str:
    text = re.sub(r"/\*.*?\*/", " ", text, flags=re.S)
    return re.sub(r"//[^\n]*", " ", text)


def damage_slash_transform_kinds() -> tuple[list[int], list[int]]:
    """The two transform triples of dEFManagerDamageSlashEffectDesc."""
    body = re.search(
        r"dEFManagerDamageSlashEffectDesc\s*=\s*\{(.*?)\n\};",
        _source_text(EFMANAGER), re.S)
    assert body, "dEFManagerDamageSlashEffectDesc not found in efmanager.c"
    triples = re.findall(r"\{([^{}]*)\}", _strip_comments(body.group(1)))
    assert len(triples) >= 2, "expected two transform triples"
    out: list[list[int]] = []
    for triple in triples[:2]:
        fields = [f.strip() for f in triple.split(",") if f.strip()]
        assert len(fields) == 3, fields
        out.append([int(f, 0) if re.fullmatch(r"0[xX][0-9a-fA-F]+|\d+", f)
                    else f for f in fields])
    return out[0], out[1]


def func_matrix_list() -> list[str]:
    """dLBCommonFuncMatrixList entries, REGION_US arms included."""
    # The declaration carries empty /* */ comments inside the [] and (), so
    # strip comments before matching: `sb32 (*name[ ])( ) = { ... };`.
    body = re.search(r"dLBCommonFuncMatrixList[^=]{0,64}=\s*\{(.*?)\n\};",
                     _strip_comments(_source_text(LBCOMMON)), re.S)
    assert body, "dLBCommonFuncMatrixList not found in lbcommon.c"
    entries: list[str] = []
    skipping = False
    for line in body.group(1).splitlines():
        token = line.strip()
        if token.startswith("#if"):
            # REGION_US is the shipped region (Makefile: -DREGION_US).
            skipping = "REGION_US" not in token
            continue
        if token.startswith(("#else", "#elif")):
            skipping = not skipping
            continue
        if token.startswith("#endif"):
            skipping = False
            continue
        if skipping:
            continue
        for name in token.split(","):
            name = name.strip()
            if name:
                entries.append(name)
    return entries


def custom_kind_of(callback: str) -> int:
    entries = func_matrix_list()
    index = entries.index(callback)
    assert index % 2 == 0, f"{callback} is not a pair head"
    assert entries[index + 1] == callback, f"{callback} pair is not doubled"
    return CUSTOM_MATRIX_KIND_BASE + index // 2


def billboard_kind40(translate, contact_in_row3=True):
    """func_80010918(mtx, dobj, TRUE) -- orientation only matters as a basis.

    The composed translation row is independent of this 3x3 (the RotSca factor
    contributes a zero translation and a unit w), so the test pins the basis to
    a fixed non-identity, non-symmetric rotation rather than re-deriving the
    camera vector.  What it does NOT abstract away is row 3: kind 40 is the
    ``is_translate != FALSE`` arm and carries dobj->translate.
    """
    out = matrix.build_local_from_descriptor(
        translate if contact_in_row3 else (0.0, 0.0, 0.0),
        (0.31, -0.77, 1.4), (1.0, 1.0, 1.0), SINT)
    return out


def rotsca_kind45(rotate, scale, *, leak_translate=None):
    """lbCommonRotScaFuncMatrix, and its deliberate double-translate mutation.

    ``leak_translate`` reintroduces the pre-repair fallback,
    ``ndsRendererAdapterBuildDObjFallbackMtx`` == syMatrixTraRotRpyRSca WITH
    dobj->translate.
    """
    return matrix.build_local_from_descriptor(
        leak_translate if leak_translate is not None else (0.0, 0.0, 0.0),
        rotate, scale, SINT)


def compose_root_local(first_xobj, later_xobj):
    """ndsRendererAdapterBuildDObjLocalMatrix's two-XObj arm.

    The first contributing XObj lands in ``out``; every later one is applied
    BEFORE it (``ndsRendererAdapterMulBefore``), because gcPrepDObjMatrix emits
    one G_MTX_MUL per XObj in table order and F3DEX MUL forms ``new * top``.
    """
    return matrix.mul_affine_20p12(later_xobj, first_xobj)


def to_20p12(value: float) -> int:
    return matrix.round_shift_s32(matrix._ftofix32(value),
                                  matrix.N64_TO_DS_SHIFT)


# --------------------------------------------------------------------------
# Source facts
# --------------------------------------------------------------------------

def test_damage_slash_root_carries_billboard_and_rotsca():
    first, second = damage_slash_transform_kinds()
    assert first == [0x28, 0x45, 0x00], first
    # The CHILD triple is an ordinary TraRotRpyRSca; only the root is mixed.
    assert second[0] == "nGCMatrixKindTraRotRpyRSca", second


def test_custom_kind_indexing_matches_kinds_the_port_already_handles():
    """Anchor the ``kind - 66`` arithmetic on four kinds the adapter names."""
    assert custom_kind_of("lbCommonFighterPartsFuncMatrix") == 0x4B
    assert custom_kind_of("func_ovl0_800C994C") == 0x4F
    assert custom_kind_of("func_ovl0_800C9F70") == 0x52
    assert custom_kind_of("grSectorArwingLaser3DFuncMatrix") == 0x53
    # Only then derive the one under repair.
    assert custom_kind_of("lbCommonRotScaFuncMatrix") == 0x45


def test_rotsca_callback_writes_no_translation_in_source():
    body = re.search(r"void lbCommonMatrixRotSca\(.*?\n\}", _source_text(LBCOMMON),
                     re.S)
    assert body, "lbCommonMatrixRotSca not found"
    text = _strip_comments(body.group(0))
    # Row 3 of the N64 Mtx is the m[1][2]/m[1][3] (integral) pair.
    assert "m[1][2] = COMBINE_INTEGRAL(0, 0)" in text
    assert "m[1][3] = COMBINE_INTEGRAL(0, 0x10000)" in text
    assert "translate" not in text


def test_maker_writes_the_contact_on_the_root():
    body = re.search(r"GObj\* efManagerDamageSlashMakeEffect\(.*?\n\}",
                     _source_text(EFMANAGER), re.S)
    assert body, "efManagerDamageSlashMakeEffect not found"
    text = _strip_comments(body.group(0))
    assert "dobj = DObjGetStruct(effect_gobj);" in text
    assert "dobj->translate.vec.f = *pos;" in text


# --------------------------------------------------------------------------
# Placement
# --------------------------------------------------------------------------

def test_composed_root_translation_equals_the_world_contact():
    composed = compose_root_local(
        billboard_kind40(CONTACT),
        rotsca_kind45(SLASH_ROTATE, SLASH_SCALE))
    expected = [to_20p12(axis) for axis in CONTACT]
    assert [composed.m[3][0], composed.m[3][1], composed.m[3][2]] == expected
    assert composed.m[3][3] == 1 << matrix.DS_MTX_FRAC_BITS


def test_composition_is_not_accidentally_order_insensitive():
    """Guard the guard: the two factors must actually differ."""
    bb = billboard_kind40(CONTACT)
    rs = rotsca_kind45(SLASH_ROTATE, SLASH_SCALE)
    assert bb.m != rs.m
    assert compose_root_local(bb, rs).m != compose_root_local(rs, bb).m


def test_reintroduced_double_translate_is_red():
    """The deliberate mutation: kind 0x45 rebuilt with dobj->translate.

    This is exactly ndsRendererAdapterBuildDObjFallbackMtx, the arm an
    unhandled custom kind used to take.
    """
    composed = compose_root_local(
        billboard_kind40(CONTACT),
        rotsca_kind45(SLASH_ROTATE, SLASH_SCALE, leak_translate=CONTACT))
    expected = [to_20p12(axis) for axis in CONTACT]
    got = [composed.m[3][0], composed.m[3][1], composed.m[3][2]]
    assert got != expected, (
        "the double-transform mutation did not move the slash; the test cannot "
        "detect the defect it exists for")
    # And it is a large, camera-dependent error, not a rounding difference.
    assert max(abs(g - e) for g, e in zip(got, expected)) > (1 << matrix.DS_MTX_FRAC_BITS)


def test_adapter_builds_kind_0x45_without_a_translation():
    """The repair must be present in the shipped builder, not just in theory."""
    text = _strip_comments(_source_text(ADAPTER))
    assert "#define NDS_RENDERER_ADAPTER_ROT_SCA_MTX_KIND 0x45u" in text
    case = re.search(
        r"case NDS_RENDERER_ADAPTER_ROT_SCA_MTX_KIND:(.*?)break;", text, re.S)
    assert case, "no kind-0x45 case in ndsRendererAdapterBuildDObjXObjMatrix"
    body = " ".join(case.group(1).split())
    assert "syMatrixTraRotRpyRSca(&mtx, 0.0F, 0.0F, 0.0F," in body, body
    assert "dobj->translate" not in body, body
    # A plain local affine: it must not be classed as an MVP-recalc kind.
    recalc = re.search(r"ndsRendererAdapterIsMvpRecalcKind\(u32 kind\)(.*?)\n\}",
                       text, re.S)
    assert recalc and "ROT_SCA" not in recalc.group(1)


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
    print("DAMAGE_SLASH_PLACEMENT_" + ("FAIL" if failures else "OK"))
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
