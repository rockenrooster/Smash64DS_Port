# Campaign 03 — Compact AOT Fighter Animation Representation

> **Planning baseline:** `codex/r2-runtime2` at `a63dd0e4b3af9c6601713f70c179c96c0aa67735` (2026-08-16).
>
> If HEAD has moved when implementation begins, re-run the inventory/measurement steps first and update symbol names/line references rather than blindly applying this document.
>
> **Campaign rule:** optimize toward a DS-native architecture and four-fighter headroom. The current two-fighter P95 gate is a checkpoint, not the architectural finish line. Never bank projected savings; measure the shipping configuration. Prefer same-binary route A/B when practical because this tree is placement-sensitive.

## Objective

Replace the Mario/Fox gameplay animation hot path with a compact DS-native representation compiled offline.

Normal gameplay must no longer need:

- FIGATREE decoding;
- generic linked `AObj` traversal;
- repeated track discovery;
- relocation-token interpretation;
- runtime endian/normalization work;
- float animation state that is immediately converted back to fixed.

Preserve **exact 60 Hz semantics**, especially the first pose produced on a motion-transition frame.

This is a representation replacement, not a sidecar cache layered beside the old system.

## Existing foundations to reuse

- `docs/optimization/FIXEDPOINT_ANIMATION.md`
- `src/nds/nds_ftanim_track.c`
- `src/nds/nds_battlepack_anim.c`
- `src/port/reloc_backend_assets.c`
- `scripts/ftanim_bake.py`
- `scripts/ftanim_reloc_probe.py`
- `scripts/ftanim_script_model.py`
- `scripts/generate_ftanim_track_pack.py`
- `scripts/generate_ftanim_dense_bank.py`
- `scripts/generate_battlepack_anim.py`
- `scripts/check_ftanim_transcribe.py`
- `scripts/check_ftanim_opcode_surface.py`
- `scripts/check_ftanim_real_bake.py`
- `scripts/check_ftanim_target_exact.py`
- `scripts/check_ftanim_dense_layout.py`
- `scripts/check_r2_cubic_error_bound.py`
- `scripts/capture-fighter-animation-audit.ps1`
- `artifacts/performance/2026-08-16_sitr-excursion/SITR_EXCURSION.md`
- `artifacts/performance/2026-08-16_sitr-attach-lane/ATTACH_LANE.md`

The fixed Hermite/cubic arithmetic already has substantial proof. Do not spend this campaign merely reinventing that kernel.

**Why representation replacement rather than more caching:** 288 of 1,600
sampled frames (18%) carry an attach or force-load, hold 81.2% of the run's
`SITR` excess (72,768 at rank-80 ceiling), and the growth is the attach-driven
re-parse itself — `ndsR2FtAnimParseDObjFigatree` runs 1.62× per call on event
frames because a fresh `AOBJ_ANIM_CHANGED` attach must consume the clip's first
event block, and a previous sidecar conversion cache was measured slower. A
compact indexed representation deletes the parse/discovery class instead of
caching around it. Size any candidate slice with the `convert.py`-style
uniform-D re-rank at rank-80 before building it (`ATTACH_LANE.md` law:
conversion is not monotone — 0.860 at D=4,118 falling to 0.244 at D=54,907).

## Critical semantic invariant

Do **not** skip/defer transition-frame animation play. This is **CLOSED —
REJECTED by the owner** (2026-08-16,
`artifacts/performance/2026-08-16_transition-play-verdict/VERDICT.md`; commit
`a63dd0e4b3a`): *"kill animhold, lots of issues with animations not visibly
playing"*. The owner played the cleanest possible variant (previous-pose
carry-over, one frame of pose/hitbox lag on transition frames only) and it
still failed. **That closes the whole family — do not re-propose it in any
form**, including the attach chain (+23,801), which is the same question.

The mechanism, from the source, so the native representation preserves it
exactly: `ftMainSetStatus` resets **every** common joint to the model's bind
transform (`ftmain.c:4655-4668`), zeroes TransN/XRotN/YRotN, attaches the new
figatree **without posing it** (`gcAddDObjAnimJoint` sets every AObj to
`nGCAnimKindNone`, `objanim.c:137-149`), and only then plays (`:4787-4795`).
That play is the **sole writer of the new status's first pose**, and
`ftMainProcSearchHitAll` (hitboxes/hurtboxes) and the draw read those joints
later in the same frame. The native evaluator must produce the new motion's
first pose on the transition frame itself, every time. "Most transitions do not
visibly change" is not sufficient.

(Correction carried from `VERDICT.md` §2.3 so it is not re-inherited:
`ftMainRunUpdateColAnim` is the **colour** animation, not hitbox placement.)

## Target architecture

Build time:

`BattleShip animation source -> host compiler -> validated native motion pack -> ROM/BattlePack`

Runtime:

`motion_id + integer/fixed time -> indexed compact segments -> fixed evaluator -> fixed joint pose`

Final goal:

`fixed animation -> fixed pose -> fixed simulation/render consumers`

with no generic `AObj` walk for Mario/Fox gameplay.

## Phase 0 — Define the exact semantic surface

Enumerate every FIGATREE command Mario/Fox actually use.

Record:

- wait/duration;
- interpolation kind;
- value/rate interpretation;
- loops/restarts;
- target track;
- length adjustment;
- callbacks/events;
- animation end behavior;
- speed changes;
- exact frame side effects become visible.

Any live unknown opcode blocks native qualification for that motion.

One reconciliation, so the owner's `docs/OPTIMIZE_LIST.md` line "30hz
Animations" is not silently dropped or silently adopted: a 30 Hz animation
*update rate* is a separate sacrifice-order trade (gameplay/visual fidelity)
requiring its own owner-approved decision. This campaign's semantic surface is
**exact 60 Hz behavior**; the compact representation must not bake in a rate
change, but should also not preclude one being layered later.

## Phase 1 — Golden animation corpus

For every P1 Mario/Fox motion, capture the legacy oracle at each 60 Hz logical frame:

- motion ID/time;
- all animated joint T/R/S values;
- track cursor/event state;
- completion flags;
- gameplay-relevant event bits;
- animation-driven callback effects.

Include loops, speed changes, motion changes, and the first frame of every transition.

This corpus must be deterministic and host-checkable.

## Phase 2 — Make one AOT compiler authoritative

Extend the existing generator stack instead of creating a second converter.

The compiled format should be:

- DS-endian ready;
- pointer-free;
- relocation-free at runtime;
- compact and contiguous;
- indexed by motion/joint/track;
- explicit about interpolation/duration.

Preserve compact original `s16` coefficients where a Q value can be obtained by shifts. Do not expand all coefficients to 32-bit without measuring ROM/cache pressure.

Use 16-bit indices unless a per-fighter bound proves 8-bit safe; the old global `<256 AObj` assumption is stale.

## Phase 3 — Delete parser/discovery for one motion

Pick one high-frequency Mario motion.

Implement direct:

1. motion ID → native descriptor;
2. descriptor → preindexed tracks/segments;
3. no FIGATREE parse;
4. no reloc-token walk;
5. no target/track discovery;
6. legacy evaluator allowed temporarily only as oracle/output boundary.

Verify generated segment stream against generic parse offline.

Then expand to all Mario motions, then Fox.

Bank parser/decode deletion independently before numerical/state rewrites.

## Phase 4 — Replace linked `AObj` state

Do not add another conversion cache. A previous sidecar cache increased BSS/working set and was slower.

Replace linked mutable AObjs with a compact state containing only what gameplay needs, e.g.:

- current motion;
- fixed/integer animation time;
- event cursor;
- active segment index where necessary;
- compact joint outputs.

Constant/inactive tracks should need little or no mutable state.

## Phase 5 — Fixed time/phase without drift

Prefer phase derived from integer frame/time numerator and segment duration rather than indefinitely accumulating `phase += step`.

Any reciprocal optimization belongs to Campaign 07 and must preserve exact rounding.

## Phase 6 — Fixed evaluator

Feed the already-proven fixed cubic/Hermite math directly from native coefficients.

The hot path should have:

- no float→fixed AObj conversion;
- no fixed→float round trip inside evaluation;
- no generic kind dispatch beyond compact native segment type;
- native integer step/linear/cubic handling.

Campaign 09 may keep multiply-heavy kernels ARM if measured beneficial.

## Phase 7 — Move output boundary outward

A first milestone may convert one final pose into legacy `DObj` fields for isolated qualification.

Then move toward:

1. fixed animation output;
2. fixed local pose;
3. fixed render matrix producer;
4. fixed gameplay consumers proven by Campaign 12.

Do not retain a permanent float mirror if no hot consumer needs it.

## Phase 8 — Integrate BattlePack/GO preparation

Coordinate with Campaign 14.

All required Mario/Fox native motion data must be ready before GO. Post-GO native animation must perform no card read, normalization, reloc fixup, or first-use compilation.

## RAM/DTCM strategy

Use compact ROM/BattlePack storage plus a small mutable runtime state. Do not generate giant dense per-frame pose tables or coexist permanently with full linked AObjs.

After the state layout stabilizes, Campaign 02 can test the compact mutable state in DTCM.

## Qualification

Required:

- transcriber matches source/oracle;
- all live opcodes covered;
- exact transition-frame behavior;
- the `ftMainSetStatus` invalidation seams are preserved or provably
  re-owned: since 2026-08-17 that function invalidates the fighter's flattened
  transform walk (grab/back-throw fix, `GRAB_THROW_WORLD_CACHE.md`) and the
  renderer cache at the topology-writer seam — a native representation that
  changes when/how joints are attached must keep those consumers correct;
- identical motion/event sequence;
- gameplay hashes/invariants unchanged;
- attachment/effect positions unchanged;
- zero post-GO animation card miss;
- zero generic AObj traversal on native Mario/Fox frames;
- zero runtime normalization/reloc-token work;
- one-minute match plus targeted transition tests;
- any banked gate claim reports the 2/3/4/5+ VBlank-interval histogram and max
  interval alongside P50/P95 (AGENTS.md device-report law).

Measure parser deletion, compact state, fixed evaluator, output-boundary removal, and total separately.

## Completion criteria

Mario and Fox use the compact native motion format as their normal path. Generic parser/AObj machinery remains only as oracle/fallback for unqualified content. The native path performs no FIGATREE decode, no generic linked track walk, no runtime reloc/normalization, and no avoidable float round trips while preserving exact 60 Hz gameplay behavior.
