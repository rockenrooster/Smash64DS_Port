#!/usr/bin/env python3
"""How many DISTINCT render states does the fighter actually occupy?

Host-side only: no build, no ROM, no emulator. It reads the generated owner
tables and replays the state-delta sequence exactly as
ndsRendererNativeApplyStateSpan does, then reports the resolved state at every
run boundary.

WHY THIS QUESTION. The 2026-08-14 delta census measured 189.4 delta
applications a frame of which 63.9% re-apply a delta index the same frame had
already applied -- which reads like redundancy until you also read that only
7.2% carry operands identical to the previous application of the same effect.
Those two numbers together say the replay is NOT re-writing the same values: the
fighter genuinely cycles among a handful of distinct render states, and the
per-frame cost is reconstructing them one 12-byte delta at a time.

So the elidable quantity is not "repeated writes". It is the reconstruction
itself, and its size is the number of distinct states -- which is a property of
the STATIC tables and therefore answerable here, for free, before any ROM is
spent on a bake.

WHAT COUNTS AS A STATE. Every field the consumer reads, tracked by which delta
index last wrote it, which is exact because a delta's writes are a pure function
of its own immutable w0/w1. GEOMETRY is the one exception -- it is a masked
read-modify-write over the previous mode -- so that one is simulated
arithmetically rather than by provenance. Fields no delta writes (env_color,
material colours) come from the material path and are deliberately excluded;
they are what the epoch key must still validate at runtime.
"""

from __future__ import annotations

import argparse
import re
from collections import Counter
from pathlib import Path

# src/nds/nds_renderer.c
EFFECT = {
    2: "OTHERMODE", 3: "COMBINE", 4: "TEXTURE", 5: "GEOMETRY", 6: "IMAGE",
    7: "TILE", 8: "LOAD_TLUT", 9: "LOAD_BLOCK", 10: "TILE_SIZE", 11: "PRIM",
    12: "BLEND", 13: "MATERIAL", 14: "LIGHT_COLOR",
}
STATE_NONE = 0xFFFF

# Which state slot each effect owns. Effects sharing a slot overwrite each
# other; effects in different slots are independent. TILE/TILE_SIZE/LOAD_* all
# move the 20-word tile block, so they share it -- that is the block E25c
# called "too expensive to compare per run", and the reason a state ID beats a
# field-by-field comparison.
SLOT = {
    2: "othermode", 3: "combine", 4: "texture", 6: "image", 7: "tile",
    8: "tile", 9: "tile", 10: "tile", 11: "prim", 14: "light",
}


def parse_braced_rows(text: str, decl: str) -> list[tuple[int, ...]]:
    """Top-level rows only.

    NDSNativeStateDelta ends in `reserved[3]`, so a row is
    `{ w0, w1, effect, { 0u, 0u, 0u } }` and an innermost-brace regex silently
    returns the RESERVED ARRAY instead of the row -- 70 rows of (0, 0, 0), an
    effect of 0 that no slot claims, and a clean-looking "1 distinct state"
    answer that is really "the simulator never wrote anything". Track depth.
    """
    start = text.index(decl)
    body = text[text.index("{", start) + 1:]
    body = body[:body.index("\n};")]
    rows: list[tuple[int, ...]] = []
    depth = 0
    row_start = 0
    for i, ch in enumerate(body):
        if ch == "{":
            if depth == 0:
                row_start = i + 1
            depth += 1
        elif ch == "}":
            depth -= 1
            if depth == 0:
                nums = re.findall(r"(0x[0-9a-fA-F]+|\d+)u?", body[row_start:i])
                if nums:
                    rows.append(tuple(int(n, 0) for n in nums))
    return rows


def parse_flat(text: str, decl: str) -> list[int]:
    start = text.index(decl)
    body = text[text.index("{", start) + 1:]
    body = body[:body.index("\n};")]
    return [int(n, 0) for n in re.findall(r"(0x[0-9a-fA-F]+|\d+)u?", body)]


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", type=Path,
                        default=Path(__file__).resolve().parents[2])
    args = parser.parse_args()
    text = (args.root / "src/nds/nds_native_fighter_owner.generated.inc"
            ).read_text(errors="replace")

    deltas = parse_braced_rows(text, "sNdsNativeFighterStateDeltas[70]")
    sequence = parse_flat(text, "sNdsNativeFighterStateSequence[196]")
    epochs = parse_braced_rows(text, "sNdsNativeFighterEpochs[49]")
    roots = {
        "mario": parse_braced_rows(text, "sNdsNativeMarioRoots[14]"),
        "fox": parse_braced_rows(text, "sNdsNativeFoxRoots[18]"),
    }
    policy = parse_flat(text, "sNdsNativeFighterEpochDirectPolicy[49]")
    runs = parse_braced_rows(text, "sNdsNativeFighterRuns[67]")
    print(f"tables: {len(deltas)} deltas, {len(sequence)} sequence, "
          f"{len(epochs)} epochs, {len(runs)} runs, "
          f"{sum(len(r) for r in roots.values())} roots")

    # NDSNativeEpoch field order, from the struct in nds_renderer.c.
    (BSF, ASF, _FA, _FR, BSC, ASC, _BSY, _ASY, _AC, RC, MAT, _FTC) = range(12)
    # NDSNativeRoot: root_offset, first_epoch, tail_state_first,
    # source_command_count, epoch_count, tail_state_count, ...
    (_RO, FE, TSF, _SCC, EC, TSC) = range(6)

    def apply(state: dict, index: int) -> None:
        w0, w1, effect = deltas[index][0], deltas[index][1], deltas[index][2]
        if effect == 5:  # GEOMETRY is a masked read-modify-write.
            state["geometry"] = (state.get("geometry", 0) & w0) | w1
            return
        slot = SLOT.get(effect)
        if slot is not None:
            # Provenance, not value: a delta's writes are a pure function of
            # its own w0/w1, so equal last-writer implies equal fields.
            state[slot] = index

    def key(state: dict) -> tuple:
        return tuple(sorted(state.items()))

    def span(state: dict, first: int, count: int) -> None:
        if count == 0 or first == STATE_NONE:
            return
        for i in range(count):
            apply(state, sequence[first + i])

    boundary_states: Counter = Counter()
    per_owner: dict[str, set] = {}
    epoch_state_ids: dict[tuple[str, int], int] = {}
    order: dict[tuple, int] = {}
    entry_divergence: dict[int, set] = {}
    total_applications = 0
    run_boundaries = 0

    for owner, owner_roots in roots.items():
        seen = set()
        for root in owner_roots:
            state: dict = {}
            for e in range(root[EC]):
                epoch = epochs[root[FE] + e]
                span(state, epoch[BSF], epoch[BSC])
                total_applications += epoch[BSC]
                # THE RUN BOUNDARY IS AFTER THE AFTER-SPAN, NOT BEFORE IT.
                # ndsRendererExecuteNativeFighterOwnerProduction runs
                # before-span -> material -> after-span -> runs, and the E34
                # proof hook's own comment says so ("Hashed AFTER the material
                # and after-span, i.e. the complete state an epoch hands to its
                # runs"). Sampling between the spans reported 21 distinct
                # states and a bogus combine mismatch at fox epoch 22, whose
                # COMBINE delta sits in its after-span.
                span(state, epoch[ASF], epoch[ASC])
                total_applications += epoch[ASC]
                if epoch[RC] > 0:
                    k = key(state)
                    boundary_states[k] += 1
                    seen.add(k)
                    order.setdefault(k, len(order))
                    idx = root[FE] + e
                    epoch_state_ids[(owner, idx)] = order[k]
                    entry_divergence.setdefault(idx, set()).add(order[k])
                    run_boundaries += epoch[RC]
            span(state, root[TSF], root[TSC])
            total_applications += root[TSC]
        per_owner[owner] = seen

    print(f"\nstatic replay: {total_applications} delta applications to draw "
          f"every root once, {run_boundaries} run boundaries")
    print(f"DISTINCT resolved states at a run boundary: "
          f"**{len(boundary_states)}**")
    for owner, seen in per_owner.items():
        print(f"   {owner:<6} {len(seen)} distinct")

    print("\nmost-occupied states (occurrences at a run boundary):")
    for k, n in boundary_states.most_common(8):
        fields = {a: b for a, b in k}
        print(f"   state {order[k]:>2}  x{n:<4} "
              f"{ {a: (hex(b) if a == 'geometry' else b) for a, b in fields.items()} }")

    # Epoch ranges are DISJOINT -- each epoch belongs to exactly one root -- so
    # "no epoch resolves to two states" is true by construction and proves
    # nothing about a per-epoch bake. Report the coverage instead of pretending
    # the check had content.
    covered: dict[int, list[str]] = {}
    for owner, owner_roots in roots.items():
        for root in owner_roots:
            for e in range(root[EC]):
                covered.setdefault(root[FE] + e, []).append(owner)
    shared = {k: v for k, v in covered.items() if len(v) > 1}
    print(f"\nepoch coverage: {len(covered)} of {len(epochs)} epochs reached, "
          f"{len(shared)} reached by more than one root")
    print("   epoch ranges are disjoint, so a per-epoch state id is exact BY "
          "CONSTRUCTION -- the open question is the state a ROOT is ENTERED "
          "with, which is inherited from whatever drew before it.")

    # THE ELIDABLE FRACTION. Walk the whole frame the way the runtime does --
    # every root in table order, and per epoch before-span, material, after-span
    # -- and count the delta applications that would write a slot whose current
    # contents already came from that same delta. Those are the applications a
    # baked state id can skip; everything else is a genuine state change.
    #
    # A material writes image/tile/light/prim/env (ndsRendererNativeApplyMaterial),
    # so it poisons those slots with a value no static delta index can match --
    # which is what makes this a lower bound rather than a hopeful one.
    MATERIAL_SLOTS = ("image", "tile", "light", "prim")
    live: dict = {}
    applied = 0
    elidable = 0
    per_effect_elidable: Counter = Counter()
    per_effect_total: Counter = Counter()

    def walk(first: int, count: int) -> None:
        nonlocal applied, elidable
        if count == 0 or first == STATE_NONE:
            return
        for i in range(count):
            index = sequence[first + i]
            effect = deltas[index][2]
            applied += 1
            per_effect_total[effect] += 1
            slot = SLOT.get(effect)
            if effect == 5:
                w0, w1 = deltas[index][0], deltas[index][1]
                if live.get("geometry") == ((live.get("geometry", 0) & w0) | w1):
                    elidable += 1
                    per_effect_elidable[effect] += 1
                apply(live, index)
                continue
            if slot is not None and live.get(slot) == index:
                elidable += 1
                per_effect_elidable[effect] += 1
            apply(live, index)

    for owner, owner_roots in roots.items():
        for root in owner_roots:
            for e in range(root[EC]):
                epoch = epochs[root[FE] + e]
                walk(epoch[BSF], epoch[BSC])
                for s in MATERIAL_SLOTS:
                    live[s] = ("material", root[FE] + e, s)
                walk(epoch[ASF], epoch[ASC])
            walk(root[TSF], root[TSC])

    # CROSS-CHECK AGAINST THE LIVE CENSUS. This model assumes every root is
    # drawn once per frame in table order; if that is wrong the elidable figure
    # is worthless. The 2026-08-14 whole-match census measured 189.43 delta
    # applications and 47.09 epoch executions a frame, and it counts intra-frame
    # REPEATS -- a delta index applied more than once in the same frame -- at
    # 63.9%. Reproduce all three here from the static tables alone.
    frame_repeats = 0
    seen_this_frame: set = set()
    for owner, owner_roots in roots.items():
        for root in owner_roots:
            for e in range(root[EC]):
                epoch = epochs[root[FE] + e]
                for first, count in ((epoch[BSF], epoch[BSC]),
                                     (epoch[ASF], epoch[ASC])):
                    if count == 0 or first == STATE_NONE:
                        continue
                    for i in range(count):
                        index = sequence[first + i]
                        if index in seen_this_frame:
                            frame_repeats += 1
                        seen_this_frame.add(index)
            if root[TSC] and root[TSF] != STATE_NONE:
                for i in range(root[TSC]):
                    index = sequence[root[TSF] + i]
                    if index in seen_this_frame:
                        frame_repeats += 1
                    seen_this_frame.add(index)
    print(f"\nMODEL vs LIVE CENSUS (2026-08-14, 500-frame steady-state window):")
    print(f"   {'':<26}{'model':>10}{'measured':>11}")
    print(f"   {'delta applications/frame':<26}{applied:10d}{189.43:11.2f}")
    print(f"   {'epoch executions/frame':<26}{len(covered):10d}{47.09:11.2f}")
    print(f"   {'intra-frame repeat share':<26}"
          f"{100.0 * frame_repeats / max(applied, 1):9.1f}%{63.9:10.1f}%")
    print("   Agreement on all three means the static walk IS the frame: every "
          "root is drawn once, in order.")

    print(f"\nONE FULL PASS OVER EVERY ROOT, materials poisoning their slots:")
    print(f"   {applied} delta applications, {elidable} would write what is "
          f"already there = **{100.0 * elidable / max(applied, 1):.1f}% elidable**")
    print(f"   {'effect':<14}{'total':>7}{'elidable':>10}{'share':>8}")
    for effect in sorted(per_effect_total):
        t = per_effect_total[effect]
        el = per_effect_elidable[effect]
        print(f"   {EFFECT.get(effect, effect):<14}{t:7d}{el:10d}"
              f"{100.0 * el / t:7.1f}%")

    pol = Counter(policy)
    print(f"\nepoch direct-policy bytes over {len(policy)} epochs: "
          f"{dict(sorted(pol.items()))}")
    # A state id is only worth baking if the runtime can key on it cheaply.
    # Report the mapping so the bake's table size is known before it is written.
    print(f"\nper-(owner, epoch) state ids: {len(epoch_state_ids)} entries, "
          f"{len(order)} distinct -- a {len(order)}-entry baked state table "
          f"and a u8 id per epoch")


if __name__ == "__main__":
    main()
