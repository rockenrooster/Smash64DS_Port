# TASK 43 — Review-validated micro-sweep (small exact wins, one commit each)

**Standing rules apply in full: read `docs/optimization/TASK_STANDING_RULES.md`
first.** Run after Tasks 41/42. These are NOT approximations — every item here
must stay pixel-identical and behavior-exact; the fidelity doctrine is not
needed or invoked.

Source: `docs/optimization/reviews/gpt56.txt` — each item cites its finding.
Verify each claim against live code before changing it (external reviews have
been wrong before); if a claim doesn't reproduce, record that and skip the item.

## Items, in order (separate commit + melonDS A/B each)

1. **Replay per-triangle depth bookkeeping (P0-5).** Replayed rigid stage runs
   still loop once per triangle solely to advance synthetic painter/depth
   state. Precompute the post-run state at bake/prepare time and advance in one
   step per run. Safe, small, every stage frame.
2. **Native OAM single pass (P1-1).** The SObj chain is scanned three times per
   UI draw with repeated linear asset classification. Fold into one pass with a
   classification cache keyed by asset id (invalidate on scene load). Verify
   sprite output identical via synchronized screenshots (lower screen too).
3. **FGM service restructure (P1-3 + gpt56_2 §8.2).** Per update the FGM path
   reads the CPU timer even with zero live handles, scans the WHOLE handle
   pool, and does a 64-bit multiply/divide per live handle. Fix set: early
   return before cpuGetTiming when the active count is zero; an active-handle
   list (and free-list for allocation) instead of full-pool scans; absolute
   CPU-tick deadlines per envelope point advanced incrementally instead of
   converting total elapsed time each update. Exactness bar: identical output
   volume codes and voice lifecycles across a full match (temporary
   assert-compare soak, then remove the check).
4. **BGM rate-marker math (P1-4).** If Task 41 gated the markers out of LEAN,
   this item is only for telemetry builds: hoist the per-update 64-bit
   divisions in `ndsAudioBgmUpdateRateMarkers` (nds_audio_bgm.c:308) behind the
   half-second HUD cadence instead of every update. Skip if LEAN made it moot
   everywhere that matters.
5. **Stats zeroing (P1-6).** Large `NDSRendererStats` objects are memset every
   frame during native-stage preparation; zero only the fields the active
   profile level actually publishes (or dirty-track). Bounded, measurable.
6. **Batch-end alpha-test toggle (P2-4) — investigate only.** Batch end
   disables alpha test unconditionally; determine whether the next batch always
   re-establishes it. If redundant, drop the toggle; if load-bearing, document
   why and close the item. Correctness-sensitive: screenshots on transparent-
   edge content (tree leaves, HUD numerals) before/after.
7. **Battle-update wrapper specialization (gpt56_2 §4.3/P1).** First VERIFY
   which dispatch actually executes per update in mode 163 (the harness else-if
   machinery around scVSBattleFuncUpdate call sites in src/port/taskman_seam.c
   vs the direct :4353 seam call). If a per-update predicate chain really runs,
   compile a direct profile-0 path (base update + the one known post-update
   call) behind the ship flag; if the chain is boot-time-only, record that and
   close the item without changes. Depends on Task 41's ftMainSetStatus-style
   proof discipline for anything that can alter control flow.

## Gates (per item)

- Synchronized screenshots: 0 changed pixels (both screens where relevant).
- Gameplay verifiers untouched/green; full-match soak after the last item.
- melonDS typed counter delta reported per item + running total with the
  calibration-predicted device figure; queue one retail pair (all items ON vs
  base) in `builds/device-queue/`.

**Kill criterion per item:** if an item's melonDS win measures < ~1K and the
change adds any complexity, revert that item and note it — this sweep ships
only free wins. Time-box the OAM item hardest; it is the only one with real
structural risk.
