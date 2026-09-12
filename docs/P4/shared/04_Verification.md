# Shared plan 04 — Evidence required before a character is complete

**Status:** proposed extension of existing verification. Parent: [master](../New_Characters.md).

## Four distinct evidence levels

| Level | What it proves | What it does not prove |
|---|---|---|
| Source inspected | Named source slices support a bounded design observation. | Complete dependencies, imported bytes, correct runtime. |
| Import qualified | Pinned donor resolves to deterministic, classified native-production input. | Correct behavior in a running DS game. |
| Behavior qualified | Directed/reference and integration tests pass for the declared mode tier. | Required stress performance or every legal resource union. |
| Release admitted | Behavioral, resource, native-rendering and standing performance gates pass, with evidence. | Untested future modes or P3 protocols. |

All eighteen cards in this package are at the first level only. No ROM extraction, build or emulator execution was performed for this review. Status must not jump from "file exists" or "CSS selectable" to complete.

## Gate A — Source and importer

Lock the three submodule revisions, source ROM/profile and toolchain. Rebuild twice with identical semantic outputs; compare hashes of canonical artifacts rather than timestamps. Resolve all effective action entries, sentinels, pointers, scripts and required callback dependencies for the candidate.

Add negative controls for wrong source pin, missing inherited asset, unsupported opcode, invalid pointer, unknown semantic atom and malformed zero-time loop. Verify that the system identifies the first cause and refuses admission rather than skipping content.

Use concrete fixtures: Falco's appended/fall-through stream, Wario's assembly-built idle and concurrent trail, Marth's random-voice table and item subroutines, Ganondorf's separate animation/event speed semantics, Lanky's actual float bit pattern, and EXTRA request-list/sound substitutions. A plain opcode histogram is not a compatibility proof.

## Gate B — Gameplay and event equivalence

Derive the move/state inventory from source. Exercise locomotion, all attacks, specials, grabs/throws, damage, shielding, ledges, recovery, items, KO and respawn, plus character-specific buffers/counters/articles. Include every ground/air transition and cancellation/interruption category, with inputs on either side of important timing boundaries.

Compare meaningful reference traces: action transitions, event times, active hit/hurtboxes, velocities/positions within approved equivalence tolerance, article creation/destruction/ownership, jump/charge state and authoritative random decisions. A host-side harness can test isolated native callbacks; a donor trace validates behavior the actual source executes. Neither one identical screenshot nor a single full-game hash establishes gameplay equivalence.

Resolve the source numeric/time semantics first. The project does not require bit-exact N64 floats or immutable 60-Hz simulation. A compensated schedule still must reproduce one-frame buffers, multi-hit/event order and source feel; visual frame rate must not become an accidental gameplay clock.

## Gate C — Cross-roster interactions

Test the candidate both as attacker and victim with relevant archetypes: capture/throw offsets, inhale/copy, egg or other special victim states, shields/reflectors, applicable absorption, source counters and ordinary items. Select interactions based on actual supported source behavior, not presumed later-game mechanics.

Required high-value configurations include the fighter without its resource donor selected; four mirrors; mixed donors/costumes; three Kirbys plus the candidate where applicable; and mixed article-heavy opponents. Prime the relevant state explicitly. Ordinary CPU play may never produce the maximum pool or a rare Judge outcome.

CPU tests include the donor's attack decisions, movement/recovery, article use and selected difficulty semantics. Preserve deterministic replay under the implemented policy. Cosmetic rendering/audio decisions must not advance authoritative state unpredictably.

## Gate D — Native presentation and lifecycle

Compare converted source assets and representative animations side by side before owner visual review. Cover all declared modelparts/materials, costume/team colors, attached weapons/effects, entry, portraits/names, HUD, selection/announcer, voices, results and applicable crowd/audio events. Inspect supported pause/camera behavior, especially facing-dependent or planar models.

The original project's native-renderer law remains binding: zero generic-renderer dependence for completed content. Instrument required resource misses, native fallback and unexpected uploads/reads. Required missing art cannot pass just because the match completes.

Soak CSS -> stage select -> load -> battle -> sudden death where applicable -> results -> rematch/change selection. Include rapid selection/cancellation, missing-resource dev failures, maximum-cost-to-minimum-cost transitions and the reverse. Confirm pointer generations, callbacks, article state, audio loops and texture handles cannot leak across epochs. Test stock replacement separately from match teardown.

Preserve old save meanings, campaign behavior, original-roster unlocks/records and every standing Boundary regression. Campaign/bonus expansion for new fighters stays explicitly pending until those modes are qualified.

## Gate E — DS resource and performance qualification

Use the [whole-match ledger](03_DS_Resources.md) for all supported unions and exact placement. Measure loading peaks and dynamic high-water, not only static estimates. Validate runtime overflow/pool/free-floor diagnostics against the actual planned values.

Use the current PROJECT_GOAL and VERIFYING contract: stable 30 FPS, approximately P95 <= 1.12M ARM9 ticks per presented frame and the adopted >=95% two-VBlank cadence population. Keep rare exceptional overruns in perspective; report P50/P95/P99 and the complete cadence distribution rather than a convenient average. Do not quietly change the measurement population.

Use the project's custom accuracy-focused melonDS reference and its prescribed configuration. Normal development need not be blocked on repeated retail hardware measurements. Performance instrumentation must not materially change the release code/layout without a accounted-for comparison.

Maintain separate measured adversaries per resource dimension and re-derive the standing stress configuration as content lands. All items and the measured hard stage remain part of the eventual release scope. Directed article/copy tests supplement the standing one-minute stress arms; passing one does not replace the other.

## P3 and closure record

When P3 is available, test source/content mismatch rejection, stable IDs, deterministic special state, loss/jitter under P3's policy, reconnection/disconnect behavior where supported, and source-relevant simultaneous events. Do not implement a second sync method inside P4. A pending P3 test is labeled pending, not assumed green.

The closure record contains exact build/source/asset hashes, declared mode tier, inventory coverage, known approved deltas, resource ledger/placement, behavior and visual evidence, performance/cadence results, regression results and P3 status. Keep live progress on the designated project execution board; these plans are not a second workflow.
