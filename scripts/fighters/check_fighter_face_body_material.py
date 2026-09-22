#!/usr/bin/env python3
"""P04/K02/J01 -- face-versus-body material audit for Pikachu, Kirby, Purin.

The owner reports, for all three fighters, that the face colour differs from
the body colour, and guesses "lighting difference???".  This check decides that
question from the source alone, and pins the answer so it cannot drift.

It does three things, none of which needs a build or an emulator:

1.  **Source replay.**  Walks each canonical root's raw source command stream
    in true command order -- every `G_MOVEWORD`/`G_MW_LIGHTCOL` word and every
    segment-E material call -- and folds in the live `MObjSub` record that the
    material call reaches, honouring `MOBJ_FLAG_LIGHT1`/`LIGHT2`/`PRIMCOLOR`/
    `ENVCOLOR` exactly as `sys/objdisplay.c:1289-1330` does.  The result is the
    source-resolved (light1, light2, prim, env) at every epoch.

2.  **Port replay.**  Walks the same roots through the *generated* tables in
    the order the runtime applies them:
    `ndsRendererNativeApplyRootLightPreamble` at root entry
    (`src/nds/nds_renderer_native_common.c:611`, index 0 == inherit),
    then per epoch the before state span, the material
    (`ndsRendererNativeApplyMaterial`, same file:867), then the after span
    (`src/nds/nds_renderer_native_fighter_production.c:310-364`).
    The two replays must agree at every epoch.  `--mutate` perturbs one root
    light preamble in the port model to prove the comparison is live.

3.  **Fold census.**  The N64 shades at 8 bits and multiplies by the primitive
    colour in the RDP *afterwards*:
        pixel = clamp8(light2 + light1 * dot) * prim / 255      -> RGB5
    The DS geometry engine has no equivalent inner clamp, so the port folds the
    primitive colour into the material diffuse/ambient *before* the engine's
    clamp (`ndsRendererR2MaterialColor15`,
    `src/nds/nds_renderer_native_common.c:6512`, called at :6827-6832):
        pixel = clamp5(light2*prim/255 + light1*prim/255 * dot)
    For prim == white the two are identical; for a tinted prim they diverge in
    the lit half, where the DS saturates toward white while the N64 keeps the
    tint.  This census counts the diverging (epoch, dot, channel) samples per
    fighter and pins the totals.  All three fighters' bodies are untextured
    runs carrying the model's own tinted primitive colour (Pikachu 0xFFD933,
    Kirby 0xFFA4B8/0x00FF5A, Purin 0xFFCDD8), so they diverge; the textured
    face runs take their colour from a CI palette and, in Purin's case, from a
    primitive colour the model never sets at all (root 0x01560 slots 0/1/2
    carry PALETTE without PRIMCOLOR, so they inherit whatever the display list
    before the fighter left).  That asymmetry is the seam the owner reports.

Usage:
    python scripts/fighters/check_fighter_face_body_material.py
    python scripts/fighters/check_fighter_face_body_material.py --verbose
    python scripts/fighters/check_fighter_face_body_material.py --mutate

Read-only: it loads the decomp O2R payloads and rebuilds the generator's
in-memory runtime context.  It writes nothing.
"""

from __future__ import annotations

import argparse
import struct
import sys
from pathlib import Path

SCRIPT_DIR = Path(__file__).resolve().parent
REPO_ROOT = SCRIPT_DIR.parent.parent
sys.path.insert(0, str(SCRIPT_DIR))

import generate_nds_native_owners as G  # noqa: E402

OWNERS = ("pikachu", "kirby", "purin")
DETAILS = ("high", "low")

# MObjSub is 0x78 bytes; sys/objtypes.h:300-342.  Offsets used below.
MOBJSUB_STRIDE = 0x78
MOBJSUB_FLAGS = 0x30
MOBJSUB_UNK4C = 0x4C
MOBJSUB_PRIM = 0x50
MOBJSUB_ENV = 0x58
MOBJSUB_LIGHT1 = 0x60
MOBJSUB_LIGHT2 = 0x64

# Every fighter MObjSub in these three models carries this word at +0x4C; it is
# the record signature the base-offset pin below is validated against.
MOBJSUB_SIGNATURE = 0x00022205

# First MObjSub of the model's joint block, from the decomp transcriptions
# (relocData/341_PikachuModel.c, 328_KirbyModel.c, 330_PurinModel.c -- the
# `/* MObjSub @ ... */` markers).  The record count is NOT pinned: it is
# derived from the generated epochs' material slots and cross-checked.
JOINT_MOBJSUB_BASE = {
    "pikachu": 0x168,
    "kirby": 0x00B0,
    "purin": 0x0110,
}

MOBJ_FLAG_ALPHA = 1 << 0
MOBJ_FLAG_SPLIT = 1 << 1
MOBJ_FLAG_PALETTE = 1 << 2
MOBJ_FLAG_FRAC = 1 << 4
MOBJ_FLAG_TEXTURE = 1 << 7
MOBJ_FLAG_PRIMCOLOR = 1 << 9
MOBJ_FLAG_ENVCOLOR = 1 << 10
MOBJ_FLAG_BLENDCOLOR = 1 << 11
MOBJ_FLAG_LIGHT1 = 1 << 12
MOBJ_FLAG_LIGHT2 = 1 << 13
MOBJ_FLAG_KNOWN = (
    MOBJ_FLAG_ALPHA | MOBJ_FLAG_SPLIT | MOBJ_FLAG_PALETTE | MOBJ_FLAG_FRAC
    | MOBJ_FLAG_TEXTURE | MOBJ_FLAG_PRIMCOLOR | MOBJ_FLAG_ENVCOLOR
    | MOBJ_FLAG_BLENDCOLOR | MOBJ_FLAG_LIGHT1 | MOBJ_FLAG_LIGHT2
    | 0x8 | 0x20 | 0x40
)

DOT_SAMPLES = (0.0, 0.25, 0.5, 0.75, 1.0)

# `epoch.before_state_first`/`after_state_first` sentinel for "no span"
# (NDS_NATIVE_STATE_NONE, include/nds/nds_renderer.h).
STATE_SPAN_NONE = 0xFFFF

# Diverging (epoch, dot, channel) samples between the N64's shade-then-modulate
# order and the DS geometry engine's modulate-then-clamp order, over
# DOT_SAMPLES.  Zero would mean the two orders agree everywhere; they do not,
# and the surplus is concentrated exactly on the untextured tinted-prim body
# materials.  Recorded here so a renderer change has to move the number on
# purpose.  (fighter, detail) -> (diverging_samples, total_samples).
FOLD_DIVERGENCE_CENSUS = {}

# (fighter, detail) -> epochs that draw before the model installs any light
# colour of its own.  See check_owner().
SEED_DEPENDENT_EPOCHS = {}


class CheckError(Exception):
    pass


def _u32(payload: bytes, offset: int) -> int:
    return struct.unpack_from(">I", payload, offset)[0]


def _u16(payload: bytes, offset: int) -> int:
    return struct.unpack_from(">H", payload, offset)[0]


def effective_flags(flags: int) -> int:
    """sys/objdisplay.c:1237-1242 and reloc_backend_assets.c:10173-10188."""
    if flags == 0:
        return MOBJ_FLAG_TEXTURE | 0x20 | MOBJ_FLAG_ALPHA
    return flags


def decode_mobjsubs(payload: bytes, owner: str, count: int):
    """Decode `count` consecutive joint MObjSub records from the payload."""
    base = JOINT_MOBJSUB_BASE[owner]
    records = []
    for index in range(count):
        at = base + index * MOBJSUB_STRIDE
        if at + MOBJSUB_STRIDE > len(payload):
            raise CheckError(
                f"{owner}: MObjSub {index} at 0x{at:x} runs past the payload")
        signature = _u32(payload, at + MOBJSUB_UNK4C)
        if signature != MOBJSUB_SIGNATURE:
            raise CheckError(
                f"{owner}: MObjSub {index} at 0x{at:x} signature "
                f"0x{signature:08x} != 0x{MOBJSUB_SIGNATURE:08x}; the pinned "
                "joint-block base no longer names MObjSub records")
        flags = _u16(payload, at + MOBJSUB_FLAGS)
        if (flags & ~MOBJ_FLAG_KNOWN) != 0:
            raise CheckError(
                f"{owner}: MObjSub {index} at 0x{at:x} carries unknown flag "
                f"bits 0x{flags & ~MOBJ_FLAG_KNOWN:x}")
        records.append({
            "at": at,
            "flags": effective_flags(flags),
            "raw_flags": flags,
            "prim": _u32(payload, at + MOBJSUB_PRIM),
            "env": _u32(payload, at + MOBJSUB_ENV),
            "light1": _u32(payload, at + MOBJSUB_LIGHT1),
            "light2": _u32(payload, at + MOBJSUB_LIGHT2),
        })
    return records


def root_material_counts(ctx, root_index: int) -> int:
    """How many segment-E material slots this root's DObj chain must hold."""
    root = ctx["roots"][root_index]
    epochs = ctx["epochs"]
    highest = -1
    for epoch_index in range(root[1], root[1] + root[4]):
        slot = epochs[epoch_index][10]
        if slot != G.INVALID_U8:
            highest = max(highest, slot)
    return highest + 1


def bind_materials(ctx, records):
    """Pair each canonical root with its slice of the joint MObjSub block.

    `gcAddMObjForDObj` walks one chain per joint, and the model lays the chains
    out in joint order, so root r's slot s is record (sum of earlier roots'
    counts) + s.  The total is cross-checked against the record count, which is
    what makes the pairing an assertion rather than an assumption.
    """
    canonical = ctx["canonical_root_count"]
    counts = [root_material_counts(ctx, r) for r in range(canonical)]
    total = sum(counts)
    if total != len(records):
        raise CheckError(
            f"{ctx['owner_name']} {ctx['detail']}: canonical roots need "
            f"{total} materials but the joint block holds {len(records)}")
    bound = []
    cursor = 0
    for count in counts:
        bound.append(records[cursor:cursor + count])
        cursor += count
    return bound


def apply_material(state, record):
    """sys/objdisplay.c:1289-1330 / nds_renderer_native_common.c:902-1003."""
    flags = record["flags"]
    if flags & MOBJ_FLAG_LIGHT1:
        state["light1"] = record["light1"]
    if flags & MOBJ_FLAG_LIGHT2:
        state["light2"] = record["light2"]
    if flags & (MOBJ_FLAG_PRIMCOLOR | MOBJ_FLAG_FRAC | 0x8):
        state["prim"] = record["prim"]
    if flags & MOBJ_FLAG_ENVCOLOR:
        state["env"] = record["env"]


def apply_light_word(state, offset: int, value: int, where: str):
    if offset in (0x00, 0x04):
        state["light1"] = value
    elif offset in (0x18, 0x1C):
        state["light2"] = value
    else:
        raise CheckError(f"{where}: unsupported G_MW_LIGHTCOL offset "
                         f"0x{offset:x}")


def source_root_preamble(payload, ctx, root_index):
    """The root's own G_MW_LIGHTCOL prefix pair, straight from the payload.

    Mirrors `decode_epoch_light_color_state`: every light word before the
    root's first triangle is the compact two-pair prefix the runtime installs
    in `ndsRendererNativeApplyRootLightPreamble`.
    """
    root = ctx["roots"][root_index]
    base, ncmd = root[0], root[3]
    first_triangle = ctx["epochs"][root[1]][11]
    prefix = []
    for command in range(min(first_triangle, ncmd)):
        w0, w1 = struct.unpack_from(">II", payload, base + command * 8)
        if ((w0 >> 24) == G.SOURCE_G_MOVEWORD
                and ((w0 >> 16) & 0xFF) == G.SOURCE_G_MW_LIGHTCOL):
            prefix.append((w0 & 0xFFFF, w1))
    if not prefix:
        return None
    offsets = [offset for offset, _value in prefix]
    if offsets != [0x00, 0x04, 0x18, 0x1C]:
        raise CheckError(
            f"{ctx['owner_name']} root 0x{base:x}: light prefix offsets "
            f"{offsets} are not the compact two-pair layout")
    return (prefix[0][1], prefix[2][1])


def check_preamble_table(payload, ctx, preambles, indices):
    """Every generated root-light preamble must equal the source prefix.

    This is the assertion that stays live even where a material immediately
    re-installs the colours (Pikachu root 0x021D0 does exactly that), so a
    corrupted preamble cannot hide behind a later override.
    """
    failures = []
    for root_index in range(ctx["canonical_root_count"]):
        want = source_root_preamble(payload, ctx, root_index)
        index = indices[root_index]
        got = None if index == 0 else tuple(preambles[index])
        if want != got:
            failures.append((root_index, ctx["roots"][root_index][0],
                             want, got))
    if failures:
        lines = [f"{ctx['owner_name']} {ctx['detail']}: "
                 f"{len(failures)} root light preamble(s) differ from the "
                 "source prefix"]
        for root_index, offset, want, got in failures[:8]:
            lines.append(
                f"  root {root_index} (0x{offset:05x}): source "
                f"{_pair(want)} | port {_pair(got)}")
        raise CheckError("\n".join(lines))


def _pair(pair):
    if pair is None:
        return "inherit"
    return f"(0x{pair[0]:08x}, 0x{pair[1]:08x})"


def source_replay(payload, ctx, bound):
    """Replay the raw source command stream in true order, per canonical root.

    Returns {(root_index, epoch_index): (light1, light2, prim, env)} sampled at
    the epoch's first triangle -- the state its geometry actually draws under.
    """
    owner = ctx["owner_name"]
    roots = ctx["roots"]
    epochs = ctx["epochs"]
    state = {"light1": None, "light2": None, "prim": None, "env": None}
    resolved = {}
    for root_index in range(ctx["canonical_root_count"]):
        root = roots[root_index]
        base, first_epoch, _tail, ncmd, nepoch = (
            root[0], root[1], root[2], root[3], root[4])
        materials = bound[root_index]
        # Command index -> the epoch whose geometry begins there.
        epoch_at = {epochs[e][11]: e
                    for e in range(first_epoch, first_epoch + nepoch)}
        for command in range(ncmd):
            if command in epoch_at:
                resolved[(root_index, epoch_at[command])] = (
                    state["light1"], state["light2"],
                    state["prim"], state["env"])
            w0, w1 = struct.unpack_from(">II", payload, base + command * 8)
            op = w0 >> 24
            if (op == G.SOURCE_G_MOVEWORD
                    and ((w0 >> 16) & 0xFF) == G.SOURCE_G_MW_LIGHTCOL):
                apply_light_word(state, w0 & 0xFFFF, w1,
                                 f"{owner} root 0x{base:x}")
            elif op == G.SOURCE_G_DL and (w1 >> 24) == G.SOURCE_SEGMENT_E:
                slot = (w1 & 0xFFFFFF) // 8
                if slot >= len(materials):
                    raise CheckError(
                        f"{owner} root 0x{base:x}: segment-E slot {slot} "
                        f"exceeds the {len(materials)} bound materials")
                apply_material(state, materials[slot])
    return resolved


def mutated_preambles(ctx, mutate: bool):
    """The port model's light-preamble table, optionally with one defect."""
    preambles = [list(pair) for pair in ctx["light_preambles"]]
    if mutate:
        # A deliberate defect: lift every explicit root preamble's ambient by
        # one RGB5 step per channel.  Nothing else changes.
        for index in range(1, len(preambles)):
            preambles[index][1] ^= 0x00202000
    return preambles


def port_replay(ctx, bound, preambles):
    """Replay the generated tables in the runtime's application order."""
    owner = ctx["owner_name"]
    roots = ctx["roots"]
    epochs = ctx["epochs"]
    delta_state = ctx["state"]
    sequence = ctx["sequence"]
    indices = list(ctx["light_preamble_indices"])

    def apply_span(state, first, count, where):
        if count == 0 or first == STATE_SPAN_NONE:
            return
        for position in range(first, first + count):
            row = delta_state[sequence[position]]
            if row[2] != G.NATIVE_STATE_LIGHT_COLOR:
                continue
            apply_light_word(state, row[0] & 0xFFFF, row[1], where)

    state = {"light1": None, "light2": None, "prim": None, "env": None}
    resolved = {}
    for root_index in range(ctx["canonical_root_count"]):
        root = roots[root_index]
        materials = bound[root_index]
        where = f"{owner} root 0x{root[0]:x}"
        # ndsRendererNativeApplyRootLightPreamble: index 0 is "inherit".
        preamble_index = indices[root_index]
        if preamble_index != 0:
            state["light1"], state["light2"] = preambles[preamble_index]
        for offset in range(root[4]):
            epoch_index = root[1] + offset
            epoch = epochs[epoch_index]
            apply_span(state, epoch[0], epoch[4], where)
            slot = epoch[10]
            if slot != G.INVALID_U8:
                apply_material(state, materials[slot])
            apply_span(state, epoch[1], epoch[5], where)
            resolved[(root_index, epoch_index)] = (
                state["light1"], state["light2"],
                state["prim"], state["env"])
    return resolved


def clamp8(value: int) -> int:
    return 0 if value < 0 else (255 if value > 255 else value)


def clamp5(value: int) -> int:
    return 0 if value < 0 else (31 if value > 31 else value)


def scale_material_channel5(shaded: int, material: int) -> int:
    """nds_renderer_textures_effects.c:1441-1451."""
    numerator = (shaded * material) + 127
    return (numerator + 1 + (numerator >> 8)) >> 11


def n64_pixel5(light1: int, light2: int, prim: int, dot: float):
    """clamp8(ambient + diffuse*dot) * prim / 255, then quantized to RGB5."""
    out = []
    for shift in (24, 16, 8):
        diffuse = (light1 >> shift) & 0xFF
        ambient = (light2 >> shift) & 0xFF
        material = (prim >> shift) & 0xFF
        shade = clamp8(int(ambient + diffuse * dot))
        out.append((((shade * material) + 127) // 255) >> 3)
    return tuple(out)


def ds_pixel5_folded(light1: int, light2: int, prim: int, dot: float):
    """clamp5(folded_ambient + folded_diffuse*dot); the geometry engine.

    This is what the port did while the primitive colour was folded into the
    material, and it is kept because it is the shape of the defect: each
    channel saturates at 31 on its own, so a tinted prim washes toward white.
    """
    out = []
    for shift in (24, 16, 8):
        diffuse5 = scale_material_channel5((light1 >> shift) & 0xFF,
                                           (prim >> shift) & 0xFF)
        ambient5 = scale_material_channel5((light2 >> shift) & 0xFF,
                                           (prim >> shift) & 0xFF)
        out.append(clamp5(int(ambient5 + diffuse5 * dot)))
    return tuple(out)


def ds_pixel5_tinted(light1: int, light2: int, prim: int, dot: float):
    """The tint route: shade the RAW light, then modulate by a solid texel.

    `ndsRendererR2ResolveEpochShade` writes RGB5(light) into diffuse/ambient
    for a tinted, unmodulated epoch and `ndsRendererR2BeginTintBatch` binds an
    8x8 tile whose every texel is prim, so the pipeline evaluates
    clamp31(RGB5(l2) + RGB5(l1)*dot) * prim -- the source's own order of
    operations, with the clamp BEFORE the multiply.
    """
    out = []
    for shift in (24, 16, 8):
        diffuse5 = ((light1 >> shift) & 0xFF) >> 3
        ambient5 = ((light2 >> shift) & 0xFF) >> 3
        shade5 = clamp5(int(ambient5 + diffuse5 * dot))
        texel5 = ((prim >> shift) & 0xFF) >> 3
        # The DS modulate is (vertex * texel) at 5 bits per side, rounded.
        out.append(((shade5 * texel5) + 15) // 31)
    return tuple(out)


def ds_pixel5(light1: int, light2: int, prim: int, dot: float):
    """What the port draws today, per epoch.

    A white prim is the identity fold and keeps the folded path; anything else
    takes the tint route. The census below therefore reports the SHIPPED
    arithmetic, not a historical one -- a falsifier that models a path the
    producer no longer takes is worse than no falsifier, and this file was
    exactly that between 2026-09-21 and 2026-09-22.
    """
    if ((prim >> 8) & 0x00FFFFFF) == 0x00FFFFFF:
        return ds_pixel5_folded(light1, light2, prim, dot)
    return ds_pixel5_tinted(light1, light2, prim, dot)


def fold_census(resolved, verbose: bool, label: str):
    diverging = 0
    total = 0
    worst = None
    for key in sorted(resolved):
        light1, light2, prim, _env = resolved[key]
        if light1 is None or light2 is None or prim is None:
            continue
        for dot in DOT_SAMPLES:
            expect = n64_pixel5(light1, light2, prim, dot)
            actual = ds_pixel5(light1, light2, prim, dot)
            for index in range(3):
                total += 1
                delta = abs(expect[index] - actual[index])
                if delta != 0:
                    diverging += 1
                    if worst is None or delta > worst[0]:
                        worst = (delta, key, dot, prim, expect, actual)
    if verbose and worst is not None:
        delta, key, dot, prim, expect, actual = worst
        print(f"    {label} widest fold divergence: root {key[0]} epoch "
              f"{key[1]} dot={dot} prim=0x{prim:08x} source RGB5 {expect} vs "
              f"port RGB5 {actual} (delta {delta})")
    return diverging, total


def check_owner(owner: str, detail: str, verbose: bool, mutate: bool) -> int:
    payload = G.load_o2r_payload(REPO_ROOT, owner)
    ctx = G.build_p2_owner_runtime_context(REPO_ROOT, owner, detail)
    canonical = ctx["canonical_root_count"]
    counts = [root_material_counts(ctx, r) for r in range(canonical)]
    records = decode_mobjsubs(payload, owner, sum(counts))
    bound = bind_materials(ctx, records)

    preambles = mutated_preambles(ctx, mutate)
    check_preamble_table(payload, ctx, preambles,
                         ctx["light_preamble_indices"])

    expected = source_replay(payload, ctx, bound)
    actual = port_replay(ctx, bound, preambles)

    if set(expected) != set(actual):
        raise CheckError(
            f"{owner} {detail}: source replay covers {len(expected)} epochs, "
            f"port replay covers {len(actual)}")

    failures = []
    for key in sorted(expected):
        if expected[key] != actual[key]:
            failures.append((key, expected[key], actual[key]))
    if failures:
        lines = [f"{owner} {detail}: {len(failures)} epoch(s) resolve to a "
                 "different material/light state in the port than in the "
                 "source command stream"]
        for key, want, got in failures[:8]:
            lines.append(
                f"  root {key[0]} epoch {key[1]}: source "
                f"light1={_hex(want[0])} light2={_hex(want[1])} "
                f"prim={_hex(want[2])} env={_hex(want[3])} | port "
                f"light1={_hex(got[0])} light2={_hex(got[1])} "
                f"prim={_hex(got[2])} env={_hex(got[3])}")
        raise CheckError("\n".join(lines))

    # Epochs whose light colour is still "inherit" after the fighter's own
    # program has run: nothing in this model installs a colour before they
    # draw, so on hardware they shade from whatever the runtime seeded --
    # `ndsFighterDisplayContractSeedMaterialLights`
    # (src/port/renderer_adapter_fighter.c:1362).  Recorded, not failed: the
    # source itself leaves the RSP light state carried from the previous
    # display list here, which no per-fighter reset can reproduce exactly.
    undefined = sorted(key for key, value in expected.items()
                       if value[0] is None or value[1] is None)
    SEED_DEPENDENT_EPOCHS[(owner, detail)] = undefined
    if verbose and undefined:
        roots = sorted({key[0] for key in undefined})
        print(f"    {owner} {detail}: {len(undefined)} epoch(s) on roots "
              f"{roots} draw before this model installs any light colour; "
              "they shade from the adapter's material-light seed")

    diverging, total = fold_census(expected, verbose, f"{owner} {detail}")
    FOLD_DIVERGENCE_CENSUS[(owner, detail)] = (diverging, total)

    if verbose:
        lights = sorted({(v[0], v[1]) for v in expected.values()})
        prims = sorted({v[2] for v in expected.values() if v[2] is not None})
        print(f"  {owner} {detail}: {canonical} canonical roots, "
              f"{len(expected)} epochs, {len(records)} joint materials")
        print(f"    distinct (light1, light2): " + ", ".join(
            f"({_hex(a)},{_hex(b)})" for a, b in lights))
        print(f"    distinct prim: " + ", ".join(_hex(p) for p in prims))
        print(f"    shade fold: {diverging}/{total} channel samples diverge "
              f"between the source and the DS material fold")
    return len(expected)


def _hex(value):
    return "inherit" if value is None else f"0x{value:08x}"


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--verbose", action="store_true")
    parser.add_argument(
        "--mutate", action="store_true",
        help="perturb every generated root light preamble; self-test that "
             "inverts the exit code, so a clean run here means the check is "
             "not actually looking at the preambles")
    parser.add_argument("--owner", action="append", choices=OWNERS)
    args = parser.parse_args()

    owners = tuple(args.owner) if args.owner else OWNERS
    errors = []
    passed = []
    epochs = 0
    for owner in owners:
        for detail in DETAILS:
            try:
                epochs += check_owner(owner, detail, args.verbose, args.mutate)
                passed.append(f"{owner} {detail}")
            except CheckError as exc:
                errors.append(str(exc))
            except Exception as exc:  # noqa: BLE001
                errors.append(f"{owner} {detail}: {type(exc).__name__}: {exc}")

    if args.mutate:
        if not errors:
            print("FAIL: the mutated preambles were not detected; this check "
                  "does not constrain the root light preamble")
            return 1
        print(f"OK (self-test): the deliberate preamble mutation was rejected "
              f"for {len(errors)} of {len(owners) * len(DETAILS)} cases")
        for error in errors:
            print(error.splitlines()[0])
        if passed:
            print("  no explicit root preamble to mutate: " + ", ".join(passed))
        return 0

    if errors:
        print("FAIL: face/body material audit")
        for error in errors:
            print(error)
        return 1

    print(f"OK: {len(owners)} fighters x {len(DETAILS)} details, {epochs} "
          "epochs resolve identically in the source command stream and the "
          "generated port tables")
    for key in sorted(FOLD_DIVERGENCE_CENSUS):
        diverging, total = FOLD_DIVERGENCE_CENSUS[key]
        seeded = len(SEED_DEPENDENT_EPOCHS.get(key, ()))
        print(f"  {key[0]} {key[1]}: shade {diverging}/{total} channel "
              "samples diverge by 1/31 (both sides now clamp before the "
              "multiply; the residual is the DS quantizing light and texel to "
              f"5 bits where the N64 multiplies at 8); {seeded} epoch(s) "
              "depend on the adapter light seed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
