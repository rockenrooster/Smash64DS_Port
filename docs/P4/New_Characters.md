# P4 — New Characters: source-grounded DS master plan

**Revision:** 2; replaces the earlier provisional planning package.  
**Status:** proposed implementation plan; no character import/build/performance pass claimed.  
**Reviewed project:** `master` at `70e45e28d8e7b09545b1f772cf89f894452e9c5f`.  
**Review date:** September 5, 2026, America/Chicago.

## Decision

Build a **resolved Remix-to-native source adapter**, then admit each complete fighter through the existing DS production and match-residency systems. Keep reusable tooling and specialized runtime output. Do not port the whole Remix engine, load fighters on demand during combat, or create eighteen separate asset pipelines.

The source-availability problem is resolved: the project pins main Remix at `5e04fe7fcd023cd43c71f25f89bb6e810d254d55` and EXTRA at `96621afea26a83305abaf81add07dcf5a9c5fe3e`. EXTRA's nested Remix points to the same main revision. These are real project source locks now, not guesses about an ignored local checkout. See [source-lock.json](source-lock.json), [source audit](SOURCE_AUDIT.md) and [evidence ledger](SOURCES.md).

**The remaining engineering risk is conversion and integration, not merely finding the files.** The current P2 manifest consumes BattleShip's C/O2R metadata; the donor exposes ROM-based setup, linked assembly, binary assets and EXTRA configuration. The adapter must bridge both semantics and resource metadata. [D3/R1/R2.]

## Proposed product contract

| Question | Recommended decision |
|---|---|
| Hardware | Original DS resource envelope; no RAM expansion or DSi requirement. |
| New roster | Exactly the original draft's 18 selections, including EXTRA's MetaKnight, MRGAW and Snake folders. No silent addition of PLUS/THREED/Super/alternate variants. |
| Fidelity | Original common SSB64 rules plus the pinned donor's character-specific mechanics. Required common extensions are explicit; unrelated Remix toggles remain outside the profile. |
| VS admission | Any supported 2–4-fighter selection of admitted content, including mirrors, costumes, teams, ordinary items and source-defined Kirby interactions. |
| First P4 release tier | Complete VS and Training; P3 wireless becomes complete only after its actual protocol tests pass. |
| Campaign and bonus modes | A separately named integration tier; preserve the original campaign throughout. Use existing donor assignments where appropriate, but qualify them rather than assuming playability. |
| Compromises | Follow PROJECT_GOAL and its existing approval rules. No hidden matchup restriction, missing copied behavior or smaller gameplay population. |

These are proposed P4 decisions, not changes already made to the authoritative project contract. The master does not redefine P2 or invent a new performance gate. CPU, audio, UI, items, lifecycle and actual copy policy are part of each declared normal-VS fighter's completion—not a roster-wide cleanup batch.

## Architecture

```text
pinned donor + supported user-provided ROM + explicit behavior profile
       |
       v
reference build in disposable staging; export effective tables and symbols
       |
       v
source adapter: typed assets + event graphs + native-callback contracts
       |
       v
existing DS model / animation / audio / native-owner / residency generators
       |
       v
per-fighter, per-article and per-copy-ability fragments
       |
       v
selected-match union + exact placement + loading-peak preflight
       |
       v
native gameplay; required fighter resources already resident before GO
```

The adapter resolves assembly; the shipping DS does not emulate MIPS. Reuse competitive native event handling rather than demanding a new VM or blindly unrolling every event into C. Resolve native callbacks against the relevant BattleShip behavior and pinned donor modifications, then implement the fastest equivalent DS code.

Maintain four distinct dependency views: setup/action inheritance, resource donors, shared engine hooks, and copy abilities. Roy is the concrete example: Captain setup inheritance with many Marth assets. A single parent name cannot explain the closure.

## Shared plans and ownership

| Plan | Owns |
|---|---|
| [01 — Source admission](shared/01_Source_Admission.md) | Pinned inputs, staging, resolved tables/scripts, semantic object metadata, callback translation. |
| [02 — Roster integration](shared/02_Roster_Integration.md) | Legacy IDs, source identities, UI/save/AI/copy/P3, bounded preview loading and mode tiers. |
| [03 — DS resources](shared/03_DS_Resources.md) | Main RAM/code, loading peaks, articles, texture placement, geometry, gameplay joints and performance. |
| [04 — Verification](shared/04_Verification.md) | Import negative controls, behavior/interactions, native visuals, lifecycle, resource and release evidence. |
| [Character template](characters/_TEMPLATE.md) | Fighter-local facts, unresolved source tasks, implementation sequence and directed witnesses. |

Keep dynamic status on the project's designated execution board. The character cards are static intent with evidence and acceptance criteria; this package does not introduce a competing queue or new mandatory workflow documents elsewhere in the repository.

## P4.0 — Retire import and representation uncertainty

This is a bounded engineering experiment, not an attempt to build a universal modding framework before adding a fighter.

**First checkpoint: Falco resolved import.** Reconstruct the appropriate donor outputs in staging, export Falco's inherited/overridden actions and linked event streams, identify required callbacks and typed resource roots, and feed the existing downstream generator contract. Its appended loops, throw pointers and deliberate fall-through make this a real compatibility test, not merely a file copy.

**In parallel with that checkpoint, do a cheap census of all 18 candidates.** Record actual source availability, variant identity, known action/model/joint limits, binary resource formats, shared hooks, article families, copy policy and unresolved questions. Do not start eighteen runtime ports. Unknown loaded sizes remain unknown until conversion; source directory sizes are not RAM estimates.

Run two bounded asset/conversion canaries early: **Bowser** for non-Fox topology and effective detail/modelpart handling, and **Meta Knight** for EXTRA configuration/request-list/sound resolution. MRGAW facing transforms and Snake article families are early risk fixtures too. These canaries are not complete-character claims or a request to parallelize five runtime rewrites.

Before expanding runtime kinds, census legacy enum/mask/table/save/AI/capture assumptions. Before promising the full roster, forecast linked code/data growth and identify whether the selected-match resource strategy has sufficient runway. Verify the current P2 residency implementation instead of treating old recommended designs as already passed.

**Exit:** Falco input reproducibly resolves with no unclassified dependency in its admitted source profile; unresolved dependencies keep this gate open. The adapter seam serves both donor formats; all candidates have a concrete source/resource risk record. There is a measured or bounded plan for the next native slice—not a claim all eighteen already fit.

## P4.1 — Finish Falco, do not just make it selectable

Use [Falco's card](characters/falco.md). Complete source-specified normal actions, Phantasm, Fire Bird/reflector differences, assets, AI, copy behavior, costumes, items, UI/audio and all declared lifecycle states. Preserve original-cast behavior and native-renderer coverage.

Prove Falco works without Fox occupying a slot. Test mirrors, mixed donors/costumes, Kirby and the current resource adversaries. Resolve any pipeline special cases here rather than multiplying them across the roster. A two-player demonstration or passing import alone is not the completion gate.

**Exit:** VS/Training behavior, source comparison, native visuals, supported resource unions and the standing performance/cadence arms pass. P3 status is separately stated. Record the actual source-to-completion work categories to refine remaining estimates; discard assembly-lines-per-hour estimates.

## P4.2 — Complete dependency-ordered waves

The proposed full-production order is below. It is an engineering default, not a measured ranking of difficulty, byte cost or owner preference. Marth precedes Roy's full implementation; early source/asset work can cover both together.

| Wave | Characters | Purpose |
|---|---|---|
| 1 | [Falco](characters/falco.md), [Metal Mario](characters/metal_mario.md) | Import and promotion |
| 2 | [Ganondorf](characters/ganondorf.md), [Wolf](characters/wolf.md), [Wario](characters/wario.md) | Reuse-heavy production |
| 3 | [Bowser](characters/bowser.md), [Marth](characters/marth.md), [Roy](characters/roy.md), [Meta Knight](characters/meta_knight.md) | New topology / sword family / EXTRA |
| 4 | [Sheik](characters/sheik.md), [Peach](characters/peach.md), [King Dedede](characters/dedede.md) | Persistent state and capture |
| 5 | [Sonic](characters/sonic.md), [Banjo & Kazooie](characters/banjo_kazooie.md), [Crash Bandicoot](characters/crash.md), [Lanky Kong](characters/lanky_kong.md) | Fast movement and unusual attachments |
| 6 | [Mr. Game & Watch](characters/mr_game_and_watch.md), [Snake](characters/snake.md) | Conditional outcomes and dense articles |

Bowser remains the first full custom-topology target; Meta Knight proves the EXTRA source format. Metal Mario is a separately qualified promotion/diff against existing P2 MMario, not an automatic parameter swap. Do not justify P4 ordering with a stale statement that a vanilla parent is still unshipped: P2 prerequisite status must be checked at kickoff.

Finish one complete fighter slice at a time, or independent narrowly scoped generator/census work. Share helpers only when source semantics and measured cost justify them. New source exceptions belong in reviewed adapter rules, not hand-edited generated output.

## Match-residency rule

The unit of admission is the **complete selected match**, not a fighter main-file size. Count fixed code/data, unique immutable dependencies, four-instance mutable state, source-supported article populations, copy abilities, stage/items, renderer/audio/network buffers, stacks, placement constraints and transition/loading peaks.

Whole-roster linked code and tables can still consume RAM even when unselected assets stay on storage. Track them. Prefer measured sharing/representation improvements first; consider match-boundary executable overlays only after a projected linker-map deficit justifies the extra engineering. No runtime code paging on button press.

Required texture/animation/model/copy resources enter residency before GO. Keep intentional existing BGM streaming accounted for rather than turning this into a blanket ban on all I/O. Scene-specific preview/result packs must not keep every fighter's battle closure resident.

Source-prescribed live-article rules govern pools. Banjo has different forward/backward egg lifetime constants; Snake lists five article resource families; both invalidate a generic guessed per-character projectile allowance. Gameplay may not silently lose an article because a cosmetic budget was exceeded.

With 30 selections, four slots allowing repeats yield **40,920 unordered multisets**. Use host-side resource enumeration and justified equivalence bounds; do not create 40,920 payload packs or demand a full emulator match for each. Keep distinct RAM, mirror-pool, copy, palette, geometry and CPU adversaries. Resource equivalence is not behavioral or slot-order equivalence.

## Timing and DS specialization

Keep animation speed, script-event execution and rendering separate. Pinned-source fixtures include Ganondorf's animation-only speed command, Dedede's one-frame input buffer, Falco's fall-through and Wario's concurrent trail. A 30-FPS renderer must not double input windows or skip events. Follow PROJECT_GOAL for any compensated simulation-rate adaptation.

Generate native geometry/material programs and compact source-derived animation data. Compute gameplay-relevant hitbox, grab and projectile origins at the time they are needed; they cannot blindly reuse a throttled visual skeleton. Preserve MRGAW's facing-dependent transforms and Lanky's separate Kirby origin through explicit native handling, not an expensive generic fallback.

Graphics compromises start from an actual measured conflict and follow the project's approval/fidelity rules. Source art is the target; lower-poly geometry, sprites or fewer effects are not default admission shortcuts.

## Kirby, CPU and modes

Each source-defined copy policy is part of normal VS. A shared power or no-copy outcome is valid only when supported by the pinned donor. MRGAW's numeric hat entry must be resolved; do not invent a unique copied Chef. Snake's separated grenade data provides a useful ability-fragment seam.

When Kirby is selected, prepare the union of all copy resources reachable under that match's source rules, including late construction/reset and persistence after donor KO where applicable. Per-instance copy state is never shared just because resources are deduplicated.

Port the candidate's effective CPU behavior with its normal-VS slice. EXTRA Meta Knight includes a CPU source file; a base fighter name is not a complete AI contract. Campaign/bonus support uses an explicit tier with actual completion tests. Meta Knight's Kirby bonus-stage assignments are useful source content, not proof that every bonus target/platform is reachable.

## Completion and next action

A release-admitted fighter has a resolved source inventory; complete declared gameplay/UI/audio/CPU/copy/items; source-compared native visuals; stable IDs and scene lifetimes; supported-match resource proof; standing performance/cadence results; original-roster regressions; and an explicit mode/P3 status. Unknown is not zero, selectable is not complete, and source presence is not a performance result.

The immediate implementation task is **P4.0 Falco source export + semantic adapter contract + legacy roster census**, with Bowser/Meta Knight conversion canaries and all-candidate risk inventory kept bounded. Do not begin by porting every special into C and discover the common importer or memory ceiling afterward.

The target remains the project's stable **30 FPS**, approximately **P95 <= 1.12M ARM9 ticks** and the adopted **>=95% two-VBlank cadence** stress criterion on the canonical accuracy-focused melonDS configuration. No new hard-maximum-frame rule, arbitrary free-memory floor or guessed per-fighter polygon allotment is introduced here.
