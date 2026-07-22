# Handoff

Updated: 2026-07-22 (Task 37 placement repack KEEP)

`P1_EXECUTION_BOARD.md` owns current state. This file contains only the restart
surface and next packet.

## Restart

Branch: `codex/task37-itcm-repack`
Boundary: `battle_playable_realtime`, mode `163`

```powershell
.\scripts\verify-all.ps1 -Profile Boundary -List
git status --short
```

Preserve canonical mode 163, intrinsic renderer mode 9, mip 0, static texture
residency, source countdown, exact Dream Land water frame 0, and Task 16
compare/i2f/addsub `1/1/1`. Do not edit `decomp/`.

## Next Packet

Task 44 is a KEEP on master and ships enabled in profile-0. Steady-state stage
admission is a Dream Land asset-mutation generation compare plus an eight-segment
structural guard; matrix preparation and rigid validation walk dense 16/26
binding lists; the wrapped GX record sites test a hoisted capture-active scalar.
melonDS typed stage owner 281,688 -> 265,680 ticks (-16,008, -5.68%); item 2
alone -6,248. The retail A/B pair is queued in
`builds/device-queue/task44-stage-steady-pair/` and the device HUD `GIT` row now
carries an `S` engagement digit. Full detail and the invalidation-seam
enumeration: `docs/optimization/ClaudeFable5_Task44_StageSteadyState_20260721.md`.

Task 37 is a KEEP and ships enabled in profile-0 (`1818AA77...`). Seven measured
hot leaves moved from `.main` into ITCM free space — placement only, state-hash
exact, no eviction. Named work P50 -59,328 ticks; VBlank 3-share 71.7% -> 76.0%,
5+ 5.2% -> 3.1%. Retail pair queued in
`builds/device-queue/task37-itcm-leaves-pair/`; HUD `GIT` row carries an `L`
digit. Note the plan's stated FTR+SRC gate was NOT met (-18,944 of -40,000
required) — it named the wrong buckets and the win landed in STG. Full detail:
`docs/optimization/ClaudeFable5_Task37_ItcmRepack_20260722.md`.

**Two Task 37 findings that change how later perf work should be planned:**

1. `.text.hot` (the Task 17 update tier) measures 30.0% non-memory stall against
   ordinary `.main` code's 29.5%. It is not buying anything. `.text.hot.draw`
   (22.4%) and `.itcm` (14.7%) both work. Grouping appears to pay only for code
   re-entered many times inside one frame, so do not assume a new hot-text tier
   helps until it is measured.
2. The census tooling generalizes. `scripts/run-task37-profile-census.ps1` plus
   `scripts/task37_census.py` will rank any build's symbols by recoverable
   stall, cycles-per-byte, and tier, and enumerate a section's residents
   including the ones that never execute.

Still open: `ndsRendererHardwareResolveOrBindTexture` is the single largest cost
in the program (21.8M cycles, 4.36%) and is an algorithmic target, not a
placement one. `memcmp` is called ~3,900 times per frame; reducing that call
count is worth more than any placement of it. Task 41's runtime gates and Task
43's micro-sweep are still open, as is the unspent 5,040-byte ITCM eviction
budget.

`check-architecture.ps1` fails on tracked `artifacts/performance/*.json`. That is
pre-existing on clean HEAD and unrelated to Task 44.


Task 38's unsupported-FGM census is implemented and verifier-exported. Exact
special cues remain blocked by representation and the 128 KiB resident cap;
never substitute another sound. the owner's listen pass remains the hit-sound oracle.

Task 39's corrected-map Phase C implements hurt flash, OAM hit sparks, and the
flat transparent 2D shield with white shine. the owner approved all three in the
same ROM. The published/release-equivalent targets enable them; DevFast and
Boundary pass. Task 39 is complete and no visual row needs replay.

Task 40 is complete: Mario 195/195 and Fox 209/209 non-null motions are
the owner-approved. The full source-
backed bank, AObj16/AObj32 loader fix, `DEATH` letter `T`, watchdog, resume, and
Fox-AI-off cycler are retained. Do not rerun known-good rows. The full corrected
numeric duration matrix remains provisional; only resume specifically requested
or unverified rows if the owner asks.

Evidence is under `artifacts/performance/2026-07-21_task38/39/40-*`; Task 40's
423-row CSV and two contact sheets are permanent. Profile-0 ROM is 14,958,592
bytes / `AEE10EB3...`, net reserve 166,672, audit-symbol hits zero. DevFast's
components and the final Boundary are green. The retired Regression fleet stays
retired.

Preserve the unrelated `.gitignore` edit and all untracked optimization task,
stage, review, and README files; they are user-owned.
