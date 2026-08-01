# Second-Entry Closeout — Sudden Death / Rematch corruption, freezes, and qualification

Owner-commissioned plan, 2026-07-31. This closes the two `docs/BUGS.md` rows
that have consumed the last several days — "the rematched match is drawn
wrong" and "Sudden Death has FPS, freezing, and animation issues" — plus the
START-rematch qualification row, so R2-07 can finish. Follow it in order; the
first two experiments decide everything else.

**Read this first: you are one confirmation away, not at the beginning.**
The uncommitted working tree (`src/nds/nds_renderer.c`, `Makefile`,
`scripts/capture-sudden-death-entry.ps1`) already contains both the leading
fix and the instrument that proves or kills it. Finish that work; do not
restart the investigation, and do not re-derive anything in §2.

---

## 1. The unified defect model

One defect class explains every open symptom: **a second entry into
`nSCKindVSBattle` (rematch via START, or Sudden Death via
`scVSBattleStartSuddenDeath`) reuses stage run data prepared for the first
entry.** The candidate mechanism, measured by R2-07 E2 and now written in the
in-flight code comment:

- `NDSNativeStageOwnerExecution.r2_prepared_*` (the stage prepared-run cache)
  is keyed on the **config pointer and asset base pointers**. On a bump
  allocator that the scene entry rewinds, a re-entry reproduces the same
  pointers by construction — the key *cannot* see a scene boundary. This is
  the row's own standing rule ("pointer identity is never evidence of object
  identity across a scene boundary") violated by a cache key.
- Measured: `gNdsR2StagePrepareBuildCount` **frozen at 2** across the Sudden
  Death entry while `PrepareReuseCount` ran 195 → 303 — the second scene draws
  its stage from run data prepared for the first. (The earlier reading of
  "BuildCount 2 = one per entry = healthy" in the BUGS row was a misread; a
  third entry must produce a third build.)
- This also explains the split nothing else explained: Task 36 **replays**
  segments 0/5/7 (mask `0xa1`) and re-keys correctly on entry
  (generation/stamp) — those look right. The live path owns segments
  1/2/3/4/6 — the Dream Land map objects and layer 1, exactly the geometry
  that renders corrupt.
- **Prediction that unifies the freeze row:** stale prepared run data drives
  wrong and doubled submission (`STG` 2.21x), and the doubled stream is what
  overflows the display-list buffer (`syTaskmanCheckBufferLengths` self-branch
  at BattleShip `taskman.c:338`, seen with corrupted geometry in the same
  capture). If the fix lands, both the corruption *and* that freeze class
  should stop reproducing. Verify, don't assume.

The in-flight fix adds `topology_generation` + `topology_stamp` to the
prepared-run cache key — the same pair the Task 36 replay owner already keys
on, so the two caches invalidate together.

---

## 2. Settled facts — do not re-litigate any of these

Proven and stable (evidence logs in the BUGS row, dated 2026-07-30/31):

- The corruption is **stage-only, second-entry-only; the fighter path is
  intact**. Matched pair: `2026-07-30_235643-matched-*`.
- **Quantitative signature, use it as the pass/fail signal:** `STG`
  ~171–174K clean vs ~379–383K corrupt (2.21x), `ALL` 1.12M → 1.68M. No
  screenshot needed for detection.
- Second match performance is otherwise healthy (28.9 FPS, FTR 385,728):
  this is not a re-warm problem.
- **Exonerated by measurement** (each cost a cycle; none may return):
  static battle textures (Prepare 2/2, violations 0); the OAM/cloud/spark
  boot-scoped latches (fixed, firing 2/2 — real defects, didn't close this);
  MObj chain (91,482 probes clean); scene-cache eviction (now 2/2); stale
  workspace admission (topology rebuilds 2/2); stage material bindings
  (reject 0); camera — masks, tags, passes, convergence (phase-aligned
  enumeration byte-identical; `frame_draw_last` line retracted in full as a
  sampling artifact); GObj headers (800 bytes compared, identical); arena
  footprint (SD is 19,248 B *lower* than match one; ledger blind spot 232 B);
  direct-path dropout (Build/Reuse/Elide all healthy-shaped); Task 36 replay
  identity (reset → re-captured → re-keyed; payload size, mask, fault, and
  sampled words identical); Yakumono storage (permanent .bss, refuted in
  three commands); arena heap exhaustion (fixed earlier: anim-cache arena +
  heap generation contract).
- **Latent, correct, and NOT progress on this row:** the `leaves_xf`/`dust_xf`
  per-scene null (`ab660eb6c01`) arms only when `NDS_R2_PARTICLE_RUNTIME=1`.
  The L9 duplicate sine table fix (`cc5bc2ff9`) stands. Do not cite either as
  movement on the corruption.
- **Method rules this row already paid for** (each was violated once at the
  cost of a withdrawn claim): match confounds before comparing (same stop,
  same frame count, both arms); know whether a counter is cumulative or
  per-frame before quoting it; state the mechanism a counter brackets before
  inferring from it; never reason a cause from a screenshot; pointer equality
  across a scene boundary means nothing; deref-probes go last in a gdb block;
  when a lane prints nothing, check ELF exports (`nm`) before suspecting the
  emulator; `capture-sudden-death-entry.ps1` rebuilds on every run — flags go
  on its make line and are verified by header readback, never hand-built
  around.

---

## 3. E1 — prove the route, one run (the experiment the probe was built for)

The tree's `NDS_R2_STAGE_ROUTE_PROBE` routes each stage segment independently
at runtime (gdb-written overrides; `Enable` off = shipping behavior, which is
the probe's own control). Three routes per segment: REPLAY / REUSE (live
prepared-run cache) / REBUILD (from source).

Run the Sudden Death lane on the probe build and record, on the second entry:

1. **All-REUSE (default today): corrupt** — reproduces the baseline.
2. **Live segments forced REBUILD: clean** — if the stage renders correctly
   and `STG` returns to ~1x, the prepared-run reuse is confirmed as the
   defect carrier.
3. **Optional sharpening:** flip one live segment at a time to name the
   minimal corrupt set.

Decision rule: if forcing REBUILD does **not** clean the second entry, the
cache-key theory is dead — stop, record it in the BUGS row exactly like the
previous refutations, and fall back to §7. Do not proceed to E2 on a failed
E1.

Bookkeeping: the probe verdict goes in the BUGS row with its log name; the
probe itself is deleted after E2 lands (temporary probes do not survive
handoff).

---

## 4. E2 — land the fix and verify both lanes

The fix (already drafted in the tree): key `r2_prepared_*` on
`topology_generation` + `topology_stamp` alongside the existing config/asset
keys, invalidating with the Task 36 owner.

Falsifiable predictions, all cheap to read:

- `gNdsR2StagePrepareBuildCount` becomes **one per scene entry** (3 after a
  run that reaches Sudden Death; 2 after a plain match; N+1 after N rematches).
- Second-entry `STG` returns to ~171–174K; `ALL` to ~1.12M.
- Matched screenshot pair (`-MatchedCapture`) renders Dream Land correctly on
  both entries.

Verify on **both lanes of the same defect**:

- **Sudden Death lane:** `scripts/capture-sudden-death-entry.ps1` (the 0-0
  tie via one `time_remain` write; ~90 s per run). Three consecutive clean
  runs — this bug has burned too many one-sample conclusions for one run to
  close it.
- **Rematch lane:** the soak's Results→START redirect
  (`soak-freeze-watch.ps1`), same three-run standard, same STG signature and
  `BuildCount` reads.

Then the freeze prediction from §1: a long both-CPU soak (past the ~3.5 min
historical reproduction window, through at least one rematch) with zero
display-list-overflow freezes. If that freeze still reproduces after the
corruption is fixed, it is a separate defect — split it into its own BUGS row
with the capture rather than stretching this one.

---

## 5. Qualification and closure (the part that was always deferred)

With E2 green, close the whole cluster properly:

1. **START-rematch row** (`FIX IDENTIFIED, NOT YET QUALIFIED`): run its
   written proof on the canonical mode-163 ROM — natural match end, one
   ~500 ms START press past tick 410, `InputTapMask & 0x1000`,
   `RematchCount == 1`, countdown/GO reached, second match completes clean,
   one held press does not double-trigger.
2. **Pause-abort follow-up** (the `ifCommonBattlePauseMakeInterface` ABORT):
   run its discriminator — prove the match-one pause interface is
   *constructed* (not just that a press happened) before attributing the
   abort to second entries. If it is second-entry-specific and survives the
   E2 fix, it is its own row.
3. **Boundary** green on the kept checkpoint, plus **one Latest run** —
   scene-transition machinery is shared startup surface, which is Latest's
   domain, and this campaign already had one "Boundary alone would not have
   covered it" incident.
4. **Owner eye check**: one natural tie into Sudden Death, one rematch,
   watched on the owner's config. The owner's texture observation was right
   when the instruments pointed elsewhere; the owner signs off the visual.
5. **Cleanup before commit:** delete the route probe and every
   `SD-*`/second-entry diag read that is no longer cited; keep the two
   permanent counters the fix's predictions rely on (`BuildCount`,
   `ReuseCount`) — they are the cheap regression guard. The
   `-SecondEntryDiag` harness plumbing stays (it strips its reads when off
   and is how the next scene-boundary bug gets diagnosed in hours instead of
   days).
6. **Docs:** rewrite both BUGS rows to closed-with-root-cause (keep the
   refutation ledger — it is the most reusable output of this campaign);
   append the result to `PORTING.md`; board row updated; HANDOFF updated
   (and while there, bring it back under its 150-line cap — check-docs is
   red on it today).
7. Commit per standing rules; snapshot script last if the tree is clean.

---

## 6. Standing lesson to promote (one line of charter work)

This defect and the animation-cache freeze before it are the same law twice:
**a cache that survives a scene boundary must key on something the boundary
actually moves** (heap generation, topology generation/stamp) — never on
pointers, sizes, or addresses that a rewound bump allocator reproduces.
`Smash64DS_Runtime2_SwitchPlan.md` §3.11 already bans gameplay-time
allocation; add this as its sibling sentence (scene-lifetime caches key on a
generation, with the two incidents as citations) in the same commit that
closes the rows.

---

## 7. If E1 refutes the route theory (fallback, bounded)

Do not return to mechanism-guessing; the row's own conclusion stands: compare
**content**, not state. In order:

1. Full `owner->words[0..3915]` dump on both entries, diffed (the 8-sample
   spot check was suggestive, not proof; note `dump binary memory` silently
   kills the gdb script — write a loop of `x/64xw` instead and check the
   generated script ran to its last marker).
2. Per-segment emitted-command diff of the five live segments at equal frame
   counts (the matched-capture stages already exist for the stop points).
3. Only then new hypotheses — written as falsifiable predictions with the
   counter that kills them, per this row's format.

Timebox the fallback to two cycles; if both are inconclusive, stop and report
to the owner with the dumps attached rather than opening a fourth week of
mechanism roulette.
