# Handoff

Updated: 2026-07-22 (Task 45 — Task 37 SHIPPED over a red gate, owner's call)

`P1_EXECUTION_BOARD.md` owns current state. This file contains only the restart
surface and next packet.

## Published ROM changed

`smash64ds-battle-playable-hwtri.nds` is now
`1818AA775DCFFD52C82B35ED3D4FA6C6D02FCE232E9EE70D9B3F1DA3FDF54207`
(was `9E27BD3D…37CE369`). README and `DECOMP_PIN.txt` pins travel with it.

Task 37's seven ITCM leaves ship enabled (`NDS_TASK37_ITCM_LEAVES := 7`).
`.itcm` 31,676 → 32,596 of a 32,736 cap — **140 bytes free**, so ITCM is now
effectively full and any future placement work must evict first. The 5,040-byte
never-executed resident budget identified by the Task 37 census is where to look.

**The state-hash gate is still RED and was not repaired.** Task 45 established
the divergence is relocated heap pointers, not gameplay (215/215 differing words
at a constant +0x180, zero gameplay values), but the leak mechanism was never
found — two hypotheses were falsified and the speculative fix reverted. Shipping
was the owner's explicit decision on that evidence. Do not read a red
`verify-task37-itcm-state-hash-ab.ps1` as a new regression; it is the known
state. Do not "fix" it by loosening the canonicalizer without evidence.

`verify-dev-fast.ps1` is red on the `battle_playable` locked-30 pacing contract.
Pre-existing emulator-fork artefact, see `PERF_LEDGER.md:14-22`.

## Always build the tick-HUD ROM too (the owner, 2026-07-22)

Rebuilding the published ROM means rebuilding
`smash64ds-battle-playable-tickhud-hwtri` in the same change — it is the same
program plus the Task 41 timers, and it is the instrument every measurement uses
(device VBlank histograms, `sample-tick-hud-buckets.ps1`, and the Task 37 census,
which hardcodes that target). A tick-HUD build that lags the published one
reports a different binary's buckets while looking authoritative.

Task 37 shipped without it: `NDS_TASK37_ITCM_LEAVES := 7` went into the published
target block only, so the tick-HUD and GDB-proof targets stayed at 0. Both now
carry it, and the current tick-HUD ROM is
`builds/build-task41-tickhud-current/smash64ds-battle-playable-tickhud-hwtri.nds`
= `60C68AFFC1154A072B07A3B01D0850985A2E4293A76F3900BD39EBBA1D51EECC`,
11,430,912 bytes, `.itcm` 32,596 with all four libc/libm leaves resident.

`scripts/check-tickhud-parity.ps1` now guards this and runs inside
`verify-dev-fast.ps1`. It diffs `make print-benchmark-flags` between the two
targets — the Makefile's own resolved values, not a text scrape — and allows only
`BENCH_MAKE_TARGET` and `BENCH_MAKE_TICK_HUD` to differ. 41 flags compared, 0
drift. It builds nothing and takes about a second.

**Two device-queue pairs are stale against this.**
`task37-itcm-leaves-pair/` predates both Makefile fixes from the ship (the `:=`
no-op and the boolean-vs-bitmask `1`), so its candidate is not what ships.
`task44-stage-steady-pair/` still builds both arms with Task 37 off — internally
valid as an A/B, but it measures Task 44 against a pre-Task-37 baseline. Rebuild
both pairs before flashing either.

## Restart

Branch: `codex/task45-ftstruct-localize` (merged to master)
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
