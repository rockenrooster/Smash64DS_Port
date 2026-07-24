# Handoff

Updated: 2026-07-24 — all logged to master: Task 49 (`NDS_BATTLE_PROFILE` axis + GX equivalence differ) MERGED; Task 51 (Dream Land stage-native) KILLED — stage cost is vertex-emit, not matrix; Task 50 (hardware divider/sqrt) STOP at E0; Task 52 (Dream Land stage GX DMA replay) STOP at E0 — Task 36 replay structurally disabled in shipping; **Task 53 (re-activate Task 36 replay via arena-guard relaxation) KEEP-candidate** — replay re-activates, STG −33% (−187,648) but ALL flat (saved CPU redistributes to OTHR; VBlank tail up), bit-exact differ, default-off behind `NDS_TASK53_REPLAY_ARENA_FIX`, unblocks the Task 52 DMA follow-up. Task 50/51/52 shipped no code; Task 53 ships the flag default-off (not overridden in any target block); published ROM `1818AA77…` unchanged. Section statuses below are point-in-time and pre-date these merges.

`P1_EXECUTION_BOARD.md` owns current state. This file contains only the restart
surface and next packet.

## Task 49 — GX equivalence differ (KEEP candidate, not merged)

Branch `codex/task49-battle-profile-axis` (4 commits). Ships no rendering
change. Part 1: `NDS_BATTLE_PROFILE` axis (additive; `=0` fails the build
closed until Task 51; default 1 = today's shipping path). Part 2: the GX
equivalence differ (default off; capture instrument + host analyzer, Tier 1
bit-exact / Tier 2 screen-space pixels). Part 3: both controls pass —
positive (profile-1 vs profile-1, 0 divergences, 0 px) and negative (VERTEX16
word + matrix LSB both named by the differ). Published ROM byte-identical
`1818AA77...`. Ready to judge Tasks 51/52. The `60C68AFF` tick-HUD reference
is unreproducible from clean master today (47 bytes header relocation; the
honest no-op test is master-vs-mine in matched fresh dirs, both `C24867BA...`).

## Task 50 — STOP at E0 (2026-07-23)

Branch `codex/task50-hardware-divider` (from master `61469f7`) census-classified
every divide/sqrt call site. Eligible render-side ceiling ~0.55% of the battle
budget under generous static-site attribution; realistic recoverable below the
~0.5% bar, and the DS divider's async busy-wait can negate the win at battle
call density (device-only measurability). The `__aeabi_ddiv` "free win" is
absent — every double caller is cold in battle (`syMatrix*D` not reached). **No
code converted; E1 did not run; nothing merges.** Published ROM unchanged
`1818AA77...`. Full table:
`artifacts/performance/2026-07-23_task50-divide-census.md`,
`docs/optimization/ClaudeFable5_Task50_HardwareDivider_20260723.md`.

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

## Task 51 — Dream Land native stage: KILL (branch `codex/task51-dreamland-native`)

**Outcome: STG KILL, does not merge.** STG P50 587,968 ≫ the 200,000 kill line
(target was ≤120,000); the native path is ~18,752 ticks *worse*. The path is
mechanically correct (differ ZERO_DEVIATION) but the 16 non-rigid bindings it
targets **do not draw** in the battle_playable scene — they are economy-skipped
stage elements, so there is no STG cost to recover. Published ROM stays
`1818AA77…` (default-off; `NDS_TASK51_STAGE_NATIVE` is not in any target block).

Branch `codex/task51-dreamland-native` (6 commits) is the checkpoint:
matrix-math foundation + E0 generator bake + E1a/E1c flag+enum + E1b runtime +
E2 measurement. The differ result (Tier 1 = 0, Tier 2 = 0.0 px) and the
bit-exact matrix math are retained as recoverable work should a later task find
a scene state where bindings 20–29 / 33–38 actually draw.

**Build environment note (applies to all future work):** Git Bash's direct
`make` hits the documented `/opt/devkitpro` recursive sub-make path quirk.
**Build through the devkitPro msys2 bash instead:**
`C:/devkitPro/msys2/usr/bin/bash.exe -lc 'cd repo && make TARGET=... BUILD=... -j16'`.
This reproduces `1818AA77` byte-for-byte from a fresh dir.

**Decisive question for any revisit of native-stage work:** find a scene/match
state where bindings 20–29 / 33–38 submit GX, or confirm they are structurally
undrawn in battle_playable. The campaign's STG 597,632 baseline is owned by the
8 rigid `layer0` bindings (indices 0–7) that draw every frame via
`LoadNoZMatrix` — and Task 36 (`NDS_TASK36_HW_COMPOSE==2`, already shipping)
already owns the rigid subset.

Full evidence: `artifacts/performance/2026-07-23_task51-stage-native.md`;
spec + results: `docs/optimization/ClaudeFable5_Task51_DreamLandNative_20260723.md`;
PERF_LEDGER entry appended.

Do **not** flip `NDS_BATTLE_PROFILE=0` or remove its `$(error)` — fighters are
not native until Task 52. Never push.

## Task 52 — Dream Land stage GX DMA replay: STOP at E0 (branch `codex/task52-stage-gxdma-replay`)

**Outcome: STOP at E0, does not merge.** The FIFO-replay loop this task was
chartered to DMA-replace **does not run** in the shipping profile-0 program.

E0 path-activity proof (the spec's first gate) probed the always-compiled-in
internal struct `sNdsRendererTask36ReplayOwner` (gated only on
`NDS_TASK36_HW_COMPOSE==2`; the `gNds*` replay counters are profile-1-only) at
`ndsBattlePlayableFrameCompleteMarker`, frames 438–445, on both the profile-0
tick-HUD ROM and the published `1818AA77…` ROM: `state=DISABLED`,
`frame_replay=0`, `word_count=0` in both.

Root cause: `ndsRendererTask36ReplayBeginFrame` (src/nds/nds_renderer.c:4195)
admits replay only when `gNdsTaskmanArenaChosenSize == 0x150000u`, but the robust
downward-stepping arena allocator (src/port/diagnostics.c:7368-7381) cannot
secure the full `0x150000` on the DS heap — it steps down to `0x14C000`
(tickhud) / `0x14E000` (published), so the exact-size admission guard disables
replay. `rigid_binding_mask` matches (`0x00000381c00fffff`); the arena guard is
what fires. The 8 rigid layer0 bindings draw through the generic per-word emit
loop (nds_renderer.c:21241-21375), not any replay FIFO. No DMA runtime path was
added. DMA ownership census: ARM9 DMA channels 1/2 unused throughout the tree
(channel 0 live during stage draw for texture staging; channel 3 mid-frame
fills) — channel 1 is the provably-free choice, retained as recoverable evidence.

**This STOP reframes the campaign's STG premise and corrects Task 51's
attribution:** Task 36 replay is dead code in the shipping ROM, so STG 569K is
owned by the generic emit path, not "Task 36 replay" (which Task 51 cited).
Decisive question for the owner: is the `== 0x150000` arena guard a latent bug
(replay was meant to ship — fix: admit when the buffer fits the *actual* chosen
arena, a Task-36-correctness fix, not a DMA task) or is the replay path
intentionally retired?

Full evidence: `artifacts/performance/2026-07-23_task52-stage-gxdma-e0.md`;
spec + results:
`docs/optimization/ClaudeOpus48_Task52_StageGxDmaReplay_20260723.md`;
PERF_LEDGER entry appended. Branch is the checkpoint; published ROM stays
`1818AA77…`. Never push.

## Task 53 — Task 36 replay arena-guard relaxation: KEEP-candidate, STG win / ALL flat

Branch `codex/task53-replay-arena-fix`. Re-activates the Task 36 rigid-stage replay path
that Task 52 E0 found structurally disabled in shipping (the arena admission guard at
`nds_renderer.c:4195`/`:4247` demanded exactly `0x150000`; the robust downward-stepping
allocator at `src/port/diagnostics.c:7368` secures only `0x14C000`). Default-off flag
`NDS_TASK53_REPLAY_ARENA_FIX`; relaxed guard admits any arena ≥ `0x130000`; per-frame
`rigid_binding_mask`/config memcmp/texture-validity envelope left intact; staleness
detector `gNdsRendererTask36ReplayArenaStaleCount` catches a future re-tightening.

**E1 build-fixes (commit `f67e571`):** the prior session's flag never reached the C —
config-header emit was missing, the TASK36 cross-check validation was mis-ordered (before
target overrides), and the staleness counter was declared inside the profile-1 block (use
site isn't profile-gated). All fixed and verified by building. Default-off still
`1818AA77…`.

**E2 results (2026-07-24):**
- **Probe:** replay now admits — `state=READY`, `frame_replay=1`, `word_count=3916`,
  arena unchanged at `0x14C000`/4 fails.
- **Correctness (Task 49 differ, STAGE owner, flag-ON vs flag-OFF):** Tier 1 **0
  divergences** (2860/2860 words bit-identical); Tier 2 **0.0 px** → ZERO_DEVIATION.
- **STG A/B (128 samples, deterministic, B run twice byte-identical):** STG P50
  569,280 → 381,632 (**−187,648, −33%**); but OTHR 163,712 → 338,432 (+174,720), so
  **ALL P50 is flat (1,680,256 → 1,680,128, −128)**. The saved stage CPU redistributes to
  OTHR (most likely GX-backpressure redistribution). **VBlank tail improves:** 3-VBlank
  share 426→474, 4-VBlank 122→80, 5+ 17→12.
- **Memory:** unchanged (arena `0x14C000`/4 fails identical off/on; static BSS replay
  buffer; +4-byte staleness counter only).

**Verdict: KEEP-candidate, default-off.** Real STG-owner win + pacing-tail improvement,
but ALL is flat pending a device A/B to confirm the bucket-edge pacing gain (activating
replay changes timing/memory-access — device-only class). Not overridden in any published
or tick-HUD target block; published ROM stays `1818AA77…`. **Unblocks Task 52 DMA** on a
now-live replay loop — the OTHR redistribution is the GX-backpressure cost DMA overlap
would target. Full evidence:
`artifacts/performance/2026-07-24_task53-replay-arena-fix-e2.md`; visual A/B in
`artifacts/visibility/task53/` (owner is the oracle). Never push.

**Published ROM stays `1818AA77…`** — flag default is 0, no override
in the published or tick-HUD target blocks (`Makefile:209`, `:280`).
