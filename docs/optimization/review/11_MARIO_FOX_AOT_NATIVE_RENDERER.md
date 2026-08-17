# Campaign 11 — Mario / Fox AOT-Native Renderer

> **Planning baseline:** `codex/r2-runtime2` at `a63dd0e4b3af9c6601713f70c179c96c0aa67735` (2026-08-16).
>
> If HEAD has moved when implementation begins, re-run the inventory/measurement steps first and update symbol names/line references rather than blindly applying this document.
>
> **Campaign rule:** optimize toward a DS-native architecture and four-fighter headroom. The current two-fighter P95 gate is a checkpoint, not the architectural finish line. Never bank projected savings; measure the shipping configuration. Prefer same-binary route A/B when practical because this tree is placement-sensitive.

## Objective

Make Mario and Fox use a purpose-built DS-native renderer as the normal hot path.

Generate `Mario_DrawFast()` / `Fox_DrawFast()`-style behavior offline so runtime does **not** rediscover immutable N64 display-list structure every frame.

Compile offline:

- topology;
- already-shipped primitive groups/strips;
- model traversal order;
- material ordering;
- texture/material bindings;
- GX state transitions;
- immutable hierarchy;
- dynamic-slot locations.

Runtime supplies only genuinely dynamic:

- matrices/local transforms;
- visibility;
- colors/tints;
- material/texture epochs;
- binding epochs;
- other transient values proven necessary.

Initial target: **20–50K+ ticks/frame** with larger upside as generic machinery disappears. This is a target, not guaranteed savings.

**What the target is made of** (`FTR_LANE.md`, per-PC attribution taken at
`GX_COMPOSE=0` before the 2026-08-16 ITCM repack and GX-compose default flip —
directionally right, numerically stale; re-derive on the fresh shipping
census before sizing): `FTR` is 291,051 tk/fr and **59.3% cache
fill, not arithmetic** — icache_fill 88,486 (30.4%) beats issue 62,929
(21.6%), the six hottest draw bodies are 21,508 B against an 8 KB I-cache
walked twice a frame, and the lane holds 24.1% of the whole run's instruction
fetch and 52.6% of its GX-FIFO stall. By phase: native production emit 71,448
(24.5%), per-joint matrix build 61,848 (21.3%), the two walker bodies 52,817
(18.1%), material/texture 29,539, capture pass 24,919 (now memoised by
Campaign 04), soft float only 4,038 (1.4%). A generated renderer wins by
**shrinking and specializing the resident code that walks/decodes**, not by
optimizing arithmetic. `FTR` is dead flat (1.00× at the band), so cuts here
convert ~1:1 at rank-80.

## Non-negotiable prohibition

Do **not** resurrect the giant copied FIFO packet that regressed roughly **+124K ticks** (`docs/optimization/FTR_STG_OPTIMIZATION.md`; board: "whole-owner FIFO copy/patch/DMA packet (+124K), forbidden, proven dead").

The native renderer must consume generated code/data directly; it must not construct/copy a huge per-frame GX packet and replay it.

## Existing infrastructure

- `scripts/fighters/generate_nds_native_owners.py`
- `scripts/fighters/check_nds_native_owner_hierarchy.py`
- `scripts/fighters/check_nds_native_owner_packet.py`
- `scripts/fighters/check_fighter_primitive_streams.py`
- `docs/optimization/NDS_NATIVE_FIGHTER_CONSUMED_FIELDS.generated.json`
- `src/nds/nds_renderer.c`
- `src/port/reloc_backend_renderer_dl.c`
- renderer parity corpus/checkers
- Task 56 primitive streams already shipping
- the shipping renderer's **baked plan** path — `ndsFighterDrawPlanApply` on
  `native_owner_plan_hit` (0 hash variants over 3,961 comparisons), validator
  99.95% cached, resolve elided as the plan-miss `else`, reset dead at the
  shipped profile (board: "four of five seams already elided"). The native
  renderer starts from this, it does not reinvent it; the board also records
  why a resolve memo was refused (key narrower than the function's inputs)
- Campaign 04 memo
- Campaign 05 hierarchy classification (GX compose itself ships since
  2026-08-16)

The Makefile already describes `NDS_BATTLE_PROFILE=0` as the intended native precompiled path while profile 1 is the shipping translation/correctness oracle (`Makefile:1643-1649`: profile 0 "lands with **Task 51**" and errors out until the native path exists, so it can never silently fall through to profile 1). Build on that seam; this campaign is the Task 51 tracked on the board.

## Phase 0 — Minimize the consumed dynamic surface

Use consumed-field generation plus runtime instrumentation to classify every Mario/Fox renderer input:

1. fighter/model immutable;
2. epoch-stable;
3. dynamic every frame;
4. oracle/diagnostic only;
5. generic feature unused by Mario/Fox.

The native runtime must not carry category-5 machinery.

Deliver a compact native dynamic contract and an oracle mapping for each consumed generic field.

## Phase 1 — Freeze a renderer parity corpus

Capture generic-renderer behavior for:

- idle/run/jump/fall;
- attacks;
- damage/knockback;
- shield;
- grabs/throws if P1;
- fireball/blaster attachments;
- hit flash/colors;
- visibility/model-part changes;
- texture/material changes;
- both facings;
- KO/rebirth;
- transition frames.

Capture semantic GX operations or normalized state/primitive stream, not screenshots alone.

## Phase 2 — Generate immutable draw structure

Extend `generate_nds_native_owners.py` to emit:

- ordered node list;
- parent/hierarchy metadata;
- references to **existing Task 56 primitive groups**;
- material/state sequence;
- texture binding IDs;
- static polygon/GX state;
- visibility dependencies;
- matrix/color dynamic slot indices;
- safe redundant-state elisions.

Do not generate a monolithic mutable FIFO packet.

Two reasonable shapes:

### A. Generated straight-line C

Good for small fixed owner paths.

### B. Compact native draw-op stream

Use a tiny purpose-built fighter executor, not the generic N64 renderer and not a new universal VM.

Benchmark only if necessary.

## Phase 3 — Mario minimal native path

Implement Mario first:

1. obtain Campaign 04 cached/native contract;
2. populate compact dynamic state;
3. execute generated hierarchy/draw program;
4. submit native GX state/vertices directly.

Bypass for qualified frames:

- N64 display-list decode;
- generic primitive discovery;
- generic material traversal;
- reloc-token interpretation;
- binding searches;
- topology reconstruction.

Keep generic renderer as oracle/fallback.

## Phase 4 — Semantic differ

From the same captured fighter state compare generic vs native:

- triangle coverage/count;
- winding;
- material order;
- texture bindings;
- color/alpha;
- matrix association;
- output-affecting GX state;
- visibility/model-part choice.

Word-for-word GX identity is not required if redundant writes are intentionally removed, but every semantic difference must be explained.

## Phase 5 — Same-binary route A/B

Prefer one ROM with generic/native route selected before the draw.

Counters:

- native draws;
- generic fallbacks;
- fallback reasons;
- primitive/vertex counts;
- binding/state changes;
- matrix loads/mults;
- bytes copied;
- renderer phase ticks.

Normal Mario gameplay is not qualified while regular states still fall back.

## Phase 6 — Add Fox

Repeat consumed-field/parity work independently.

Pay special attention to gun/blaster attachment, model-part visibility, material variants, and effect attachment joints.

## Phase 7 — Consume Campaign 04 memo directly

Desired seam:

`memo/native owner -> small dynamic inputs -> generated renderer`

Not:

`memo hit -> copy 1,280 B -> rebuild generic contract -> native renderer`.

## Phase 8 — Integrate Campaign 05 hierarchy

Use generated ownership:

- CPU matrices only where gameplay/render contract requires;
- GX-compose render-only hierarchy;
- AOT-fold static transforms;
- draw primitive groups at generated hierarchy points.

Do not build a complete CPU world-matrix array if native draw does not need it.

## Phase 9 — Integrate Campaign 13 fixed/native math

Accept DS-native fixed matrices/vertices/colors directly and remove float adapters on the native path.

## Phase 10 — Make generic machinery oracle-only for qualified P1 fighters

Once coverage is complete:

- profile 0 becomes candidate normal P1 native path;
- profile 1 remains correctness oracle;
- generic N64 renderer does not execute for qualified Mario/Fox frames;
- only explicit unqualified fallbacks use it.

Do not delete the oracle prematurely.

## Performance accounting

Measure separately:

1. traversal/decode deletion;
2. material/binding lookup deletion;
3. memo direct consumption;
4. hierarchy CPU work removed;
5. matrix/fixed boundary changes;
6. GX state-write changes;
7. combined native path.

Also report per-fighter scaling where possible. The campaign exists for four-fighter headroom, so scalable per-fighter structural savings are especially valuable.

## Verification

Mandatory:

- primitive-stream checker;
- native-owner hierarchy checker;
- renderer parity corpus;
- Mario/Fox targeted visual tests;
- Bug #10/missing-underside regression check;
- attachments/effects/shield;
- one-minute battle;
- gameplay hashes;
- no generic renderer on qualified normal frames;
- no giant packet copy;
- FTR/WORK-H P50/P95;
- GX WAIT/stalls;
- any banked gate claim reports the 2/3/4/5+ VBlank-interval histogram and max
  interval (AGENTS.md device-report law);
- RAM/ROM/TCM impact.

## Kill conditions

Reject designs that:

- copy giant packets every frame;
- reconstruct generic N64 state under a new name;
- permanently duplicate full generic/native mutable state;
- redo Task 56 stripification;
- lose model/material/attachment correctness;
- merely shift CPU cost into GX waits.

## Completion criteria

Mario and Fox render through a compact generated DS-native path in normal P1 gameplay. Immutable structure is AOT, runtime inputs are truly dynamic only, shipped primitive streams are reused, generic renderer is oracle/fallback rather than hot path, and measured savings scale meaningfully toward four-fighter headroom.
