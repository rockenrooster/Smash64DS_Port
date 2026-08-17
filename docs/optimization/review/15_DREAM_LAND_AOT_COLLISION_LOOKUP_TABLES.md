# Campaign 15 — Dream Land AOT Collision Lookup Tables

> **Planning baseline:** `codex/r2-runtime2` at `a63dd0e4b3af9c6601713f70c179c96c0aa67735` (2026-08-16).
>
> If HEAD has moved when implementation begins, re-run the inventory/measurement steps first and update symbol names/line references rather than blindly applying this document.
>
> **Campaign rule:** optimize toward a DS-native architecture and four-fighter headroom. The current two-fighter P95 gate is a checkpoint, not the architectural finish line. Never bank projected savings; measure the shipping configuration. Prefer same-binary route A/B when practical because this tree is placement-sensitive.
>
> **Basis (2026-08-17):** the shipping level is **+26,449** at rank-80 and the fresh per-PC census is `artifacts/performance/2026-08-17_shipping-rebank/v4-c238`. `c200` and `v3-c221` are retired. **Read `SHIPPING_REBANK.md` §7.7 before quoting any figure in this brief** — it lists what the new census contradicts, and mask the census by the GATE's own rank-80 frames.

## Objective

Specialize Dream Land's immutable collision geometry into build-time indexed data **without changing collision numerics**.

Replace repeated searches/walks with direct indexed loads for relationships such as:

- `line_id -> endpoints`;
- `line_id -> kind`;
- `line_id -> yakumono`;
- neighboring-line metadata;
- static line/group ownership;
- other immutable topology facts.

Treat this as one static-stage-data campaign rather than dozens of tiny memos.

## Existing evidence

Slices 35–37 banked **−10,752** combined against map collision without
touching a single numeric — three memos of answers that are pure functions of
static stage geometry, each bit-identical by construction
(`docs/P1_EXECUTION_BOARD.md`, "Slices 35–37 are what the frozen half looks
like when it is done right"). That entry states the playbook this campaign
generalizes: **memoise the answer, cut the call count, delete redundant work**
— and the board's fixed-point census row marks map collision **FROZEN — exact
moves only**, which is exactly this campaign's hard constraint.

`ndsMPFindLineEndpoints` was profiled at **5,861 tk/fr** on the c117
whole-match profile (38,890 calls at 543 cycles each; board). Newer microprobes
show different raw self costs on other bases — re-measure current shipping
before assigning savings.

Two named follow-ups already on the board in the same playbook:
line→yakumono, and `mpCollisionGetFCCommonFloor` — 818 cycles a call over
45,372 calls, with `func_ovl2_800F8FFC` calling it once per floor line while
an object is over at most one.

## Current repo anchors

- `src/port/reloc_backend_mp_collision.c`
- stage generators under `scripts/stages/`
- `scripts/stages/dreamland/`
- `scripts/check_mp_floor_crossing_exact.py`
- `scripts/check_mp_line_extent_reject_exact.py`
- `scripts/check-mp-topology-fixtures.ps1`
- `scripts/probe-collision-fixed-domain.ps1` only for separate numeric work

## Hard constraint

**Do not change collision numerics here.**

If authoritative endpoint data is float, generated tables must supply the same values/bits more directly. Fixed-point collision belongs to Campaign 12 and requires separate behavioral proof.

## Phase 0 — Map immutable relationships

Enumerate hot functions that:

- search line arrays;
- walk groups to find a line;
- map line to kind/material/yakumono;
- discover adjacent lines;
- reconstruct endpoint values/pointers;
- repeatedly rediscover static topology.

Record calls/frame, algorithmic complexity, bytes touched, static/dynamic ownership, and exact return representation.

Separate immutable topology from dynamic platform transforms.

## Phase 1 — Generated collision descriptor

Extend Dream Land/stage generation to emit compact tables indexed by runtime line ID, such as:

- endpoint A index/value;
- endpoint B index/value;
- kind;
- yakumono/static owner;
- neighbor IDs;
- invalid flags/sentinel.

Choose SoA vs packed structs based on co-access patterns. Do not overpack if bit extraction costs more than cache savings.

Use smallest proven index width with explicit invalid sentinel.

## Phase 2 — Exhaustive host exactness checker

For **every line ID**, compare source vs generated:

- endpoint bit pattern/value;
- kind;
- owner/yakumono identity;
- neighbor relations;
- invalid behavior.

Generator/check must fail if source topology changes and tables become stale.

## Phase 3 — Replace endpoint lookup first

Route `ndsMPFindLineEndpoints` to direct indexed data for Dream Land.

Requirements:

- same signature/output initially;
- invalid IDs preserve behavior;
- dynamic platform transform remains at same later stage;
- no caller semantic change.

Prefer same-binary generic vs indexed route.

Bank only after exact fixtures and match behavior pass.

## Phase 4 — Add kind/yakumono/neighbor tables

For each relationship:

1. prove table;
2. replace search/walk;
3. measure;
4. then combine.

Avoid separate runtime caches; generated descriptor is the static truth.

## Phase 5 — Remove dead Dream Land search work

After native lookups:

- identify generic loops no longer used for Dream Land;
- retain for other stages/oracle if necessary;
- prove Dream Land hot path does not enter them;
- only treat code as removable if P1 shipping link reachability supports it.

## Phase 6 — Optimize layout, not numerics

Use D-cache/access traces to choose:

- contiguous arrays;
- field packing;
- hot/cold metadata split;
- lookup order.

Large immutable tables belong in normal read-only/main memory unless separate measurement proves a tiny subset deserves TCM.

## Dynamic yakumono caution

If a line maps to a moving/dynamic object:

- AOT only the **identity/relationship**;
- read current transform/state from authoritative runtime object;
- never bake a moving position as static.

## Verification

- exhaustive host checker;
- topology fixtures;
- floor/edge crossing fixtures;
- ledge/platform behavior;
- Mario/Fox movement/recovery;
- one-minute match;
- state hashes/invariants;
- generic/native route counters;
- per-lookup and whole-frame timing;
- any banked gate claim reports the 2/3/4/5+ VBlank-interval histogram and max
  interval (AGENTS.md device-report law);
- no collision numeric change.

## Completion criteria

Dream Land's immutable collision relationships are generated once at build time and consumed by O(1) indexed loads. Gameplay no longer searches static topology it could know AOT, while collision values and behavior remain exactly authoritative.
