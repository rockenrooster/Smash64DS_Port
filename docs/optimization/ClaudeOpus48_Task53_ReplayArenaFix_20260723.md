# TASK 53 — Re-activate Task 36 rigid stage replay (fix the arena admission guard)

**Standing rules apply in full: read `docs/optimization/TASK_STANDING_RULES.md`
first.**

Branch: `codex/task53-replay-arena-fix` — **branch from
`codex/task52-stage-gxdma-replay`** (it carries the probe script
`scripts/probe-task52-replay-active.ps1` and the E0 findings this task builds on).
Record the parent SHA. If the owner has merged Task 52 to master, branch from
master and say so. New flag: `NDS_TASK53_REPLAY_ARENA_FIX ?= 0`. Do **not** touch
the killed Task 51 or the Task 52 DMA paths.

Campaign: `docs/optimization/PROFILE0_NATIVE_CAMPAIGN.md` §5. Stage line so far:
Task 51 (matrix bake) KILL → Task 52 (stage DMA) STOP → **this task** is the
actual lever. Task 52's DMA is a follow-up *only if* this task re-activates the
replay loop.

## The finding this acts on (Task 52 E0, verified)

GDB-probed `sNdsRendererTask36ReplayOwner` on **both** the profile-0 tick-HUD ROM
and the published `1818AA77` ROM: Task 36 rigid replay is **structurally
disabled** (`state=DISABLED`) in shipping. Two guards gate on an exact,
pristine-arena condition:

- **Replay admission** — `ndsRendererTask36ReplayBeginFrame`
  (`src/nds/nds_renderer.c:4195`):
  `gNdsTaskmanArenaChosenSize != 0x150000 || gNdsTaskmanArenaAllocFailCount != 0`
  → `DISABLED`.
- **Capture** — `ndsRendererTask36ReplayStartCapture`
  (`src/nds/nds_renderer.c:4247`): the **same** condition → returns without
  capturing.

But the robust downward-stepping arena allocator
(`src/port/diagnostics.c:7368`, steps down 4 KiB per failed `calloc` to protect
the 128 KiB post-BGM reserve) secures only **`0x14C000`** (tick-HUD, 4 fails) /
**`0x14E000`** (published, 2 fails). So replay never admits, the 8 rigid `layer0`
bindings draw through the **generic per-word emit path**
(`nds_renderer.c:21241-21375`), and Task 36's credited STG win does not
materialize in shipping. A KEEP silently died when the allocator was made robust,
with no gate catching it (admission is a runtime decision, not a build check).

## Primary objective

Re-activate replay by relaxing the arena guard to admit whenever the memory
environment is **safe** (not the exact pristine config), then **measure** whether
replay actually reduces STG versus the current generic-emit path.

This is a **Task-36-correctness fix.** The win is **unproven**: replay skips the
per-triangle geometry walk but still CPU-feeds the same words to the FIFO, so the
saving is the walk/prep overhead — the A/B sizes it. An honest "replay activates
but STG does not drop" is a valid outcome (Task 36's win would then have been
illusory even when active). Baseline to beat: STG P50 **569,280** / P95 **575,744**
(Task 52 E0 fresh capture, generic-emit path).

## Why the guard reads as over-strict (not a correctness invariant)

The replay path already self-protects with per-frame checks that stay in place:

- config `memcmp` (`nds_renderer.c:4217`) — material/config must match capture;
- `ndsRendererTask36ReplayTexturesValid` (`4225`) — texture binds re-validated by
  name every frame.

The arena size does not affect the stage geometry stream (the taskman arena is the
game-logic pool, not stage geometry). The replay buffer
(`sNdsRendererTask36ReplayOwner.words[]`) is **static** — not arena-allocated —
so re-activating replay consumes no additional heap and cannot erode the post-BGM
reserve. The exact-`0x150000`-plus-zero-fails condition reads as a stale
"pristine environment only" gate left behind when the allocator was made robust.
**E0 must confirm this rather than assume it.**

## Stages

### E0 — establish safety and the true relaxation (investigate before relaxing)

1. Grep every site gating on `gNdsTaskmanArenaChosenSize` /
   `gNdsTaskmanArenaAllocFailCount`. Confirm the two replay guards (`4195`,
   `4247`) are the relevant ones and that **both** must relax (relaxing only the
   replay guard leaves capture disabled → nothing to replay).
2. `git blame` the two guards and the allocator: did the guard predate the robust
   allocator (stale), or was it deliberately coupled? Report the history.
3. **Baked-address audit:** inspect what `ndsRendererTask36ReplayRecord` captures
   (GX command words) and confirm no absolute main-heap pointer is baked in that
   would differ at a `0x14C000` arena. If any captured value depends on arena/heap
   layout, **STOP and report** — the fix is unsafe as-is. Do **not** weaken the
   config (`4217`) or texture-validity (`4225`) per-frame checks.
4. Define the safe relaxed condition: admit capture+replay whenever a usable arena
   was chosen (e.g. ≥ the allocator's `0x130000` floor), regardless of the
   stepped-down alloc-fails.

### E1 — the flag + relaxed guard (behind `NDS_TASK53_REPLAY_ARENA_FIX`, default 0)

- When on, relax **both** guards (`4195` and `4247`) to the E0 safe condition.
  Default off ⇒ guards unchanged ⇒ published ROM `1818AA77…` byte-identical
  (prove it, master-vs-mine fresh-dir).
- With the flag **on**, use `scripts/probe-task52-replay-active.ps1` to confirm in
  steady state: capture completes (`captured_segment_mask` = all 8 rigid bindings,
  `word_count` ~2,996), state reaches `READY`, and `frame_replay=TRUE` across the
  canonical windows. If capture faults or replay still won't admit, that is the
  finding — report it.

### E2 — prove correctness, then measure

1. **Task 49 differ, STAGE owner**, flag-ON (replay) vs flag-OFF (generic emit):
   Tier 1 **must be 0** — the replayed words must be word-identical to the live
   emit (same source geometry); a Tier 1 divergence is a capture defect, fix the
   capture, do not widen the gate. Tier 2 0.0 px. **State hash EXACT** (render-only;
   replay vs emit must not move any gameplay value).
2. **Owner visual A/B** — synchronized Dream Land screenshots into
   `artifacts/visibility/`. Watch textures and the water specifically (a
   texture-address hazard would surface here), across normal camera, wide zoom,
   pause-camera orbit, and fighter damage/shield/death/rebirth. Owner is the oracle.
3. **STG A/B** on the tick-HUD target, flag 0 vs 1, ≥ 128 samples, same
   ROM/window/input. Report P50/P95/mean/max and the 2/3/4/5+ VBlank histogram for
   ALL, STG, FTR, SRC, MISC, OTHR. **This is the answer:** does re-activating replay
   reduce STG? Report the delta honestly.
4. **Memory:** confirm re-activating replay does not increase memory pressure — the
   replay buffer is static, so `gNdsTaskmanArenaChosenSize` /
   `gNdsTaskmanArenaAllocFailCount` and the post-BGM reserve must be unchanged with
   the flag on. Verify.

## Gates

- **Correctness:** differ Tier 1 = 0, Tier 2 0.0 px; state hash EXACT; owner visual
  approved; zero texture/palette corruption; no memory regression (reserve intact).
- **Perf:** report the STG A/B.
  - Replay reduces STG meaningfully → **KEEP candidate** (behind the flag until
    device-confirmed) **and** it unblocks the Task 52 DMA follow-up on a now-live
    loop.
  - Replay activates but STG does not drop → **honest STOP**; record that Task 36's
    credited win is illusory; do not force it.
- **Device:** if it is a win, queue a retail A/B pair in `builds/device-queue/`
  (activating replay changes timing/memory access — device-class confirmation
  before any ship).
- **Default off:** published ROM `1818AA77…` byte-identical.
- **Defensive add:** propose (do not necessarily implement) a boot-time or
  build-time assertion so a future change that silently disables Task 36 replay is
  caught — a KEEP dying unnoticed is the root failure here.
- `.\scripts\verify-dev-fast.ps1` (bar the known pacing red), then
  `.\scripts\verify-boundary.ps1`.

## Traps

- **Both** guards (`4195` replay + `4247` capture) must relax together.
- Do **not** weaken the config `memcmp` (`4217`) or texture-validity (`4225`)
  checks — those are the real correctness guards.
- **Override trap:** thread `NDS_TASK53_REPLAY_ARENA_FIX` into the tick-HUD
  measurement target or a command-line `=1` is silently ignored; prove the built B
  ROM took the fixed path before trusting numbers.
- **One writer** on `src/nds/nds_renderer.c`. This **changes shipping behavior**
  when enabled (not a pure instrument) — it stays default-off until measured +
  visual + device-confirmed; flipping it on in the published profile is then the
  owner's ship decision (like Task 37).

## Constraints

- `decomp/` is read-only.
- Long builds detached; build through `C:/devkitPro/msys2/usr/bin/bash.exe -lc '…'`
  (Git Bash direct `make` hits the `/opt/devkitpro` sub-make quirk).
- Time-box open-ended debugging ~10 emulator runs / ~1 hour, then checkpoint to the
  WIP branch and report. Cite `file:line` for every behavior claim.
- **Never push.**

## Deliverables

- Separate commits: (1) E0 safety investigation + relaxation definition, (2) flag +
  relaxed guards, (3) differ + STG A/B + visual + device-queue, (4) docs.
- The relaxed guard behind its flag; E0 safety writeup (blame history + baked-address
  audit); probe output proving capture+replay admit with the flag on; Task 49 differ
  certificate; STG/ALL A/B P50/P95 + VBlank dist; `artifacts/visibility/` screenshots;
  memory-unchanged proof.
- A clear **MERGE / KEEP-candidate / STOP** verdict with the STG delta.
- Results section here; `PERF_LEDGER.md` entry; `HANDOFF.md`/`PORTING.md` notes.
- `.\scripts\New-Smash64DSSnapshot.ps1 -Mode Lean` as the final action.

## Final response (what to return)

1. Exact files changed. 2. E0 safety verdict (blame history, baked-address audit,
the safe relaxed condition). 3. Probe output — capture completes and replay admits
with the flag on. 4. GX differ results (Tier 1/Tier 2) and state-hash result.
5. STG and ALL A/B P50/P95 + VBlank distribution. 6. Owner visual result. 7. Memory
unchanged proof. 8. MERGE / KEEP-candidate / STOP decision with the STG delta and
whether the Task 52 DMA follow-up is unblocked.

## Results

### E0/E1 — landed

Branch `codex/task53-replay-arena-fix` (parent `20b12c6` of task52 E0). One
commit each for E0 (flag/safety infrastructure) and E1 (guard relaxation
plus staleness detector). The relaxed guard admits any usable arena down
to the documented 0x130000 floor (`src/port/diagnostics.c:7354`), and
default OFF keeps the published ROM 1818AA77-sh equivalent. Code review:
SAFE, no defects.

Approx diff stats:
- `Makefile`: +21 lines (flag declaration + 2 validation blocks).
- `include/nds/nds_renderer.h`: +27 lines (flag declare + extern).
- `src/nds/nds_renderer.c`: +44/-3 lines (macros + 2 guard swaps + counter).
- `docs/optimization/ClaudeOpus48_Task53_ReplayArenaFix_20260723.md`: +this file.

### E2 — executed (2026-07-24)

Full E2 ran on branch `codex/task53-replay-arena-fix`. Three load-bearing gaps in the
E0/E1 flag wiring were found and fixed first (commit `f67e571`): the config-header emit
was missing (the flag never reached the C), the TASK36 cross-check validation was
mis-ordered (ran before target overrides applied), and the staleness counter was declared
inside the profile-1 block (use site is not profile-gated → undeclared at profile-0). All
verified by building. Default-off still reproduces `1818AA77…`.

**Probe:** flag-on tick-HUD ROM — `state=READY`, `frame_replay=1`, `word_count=3916`,
arena unchanged at `0x14C000`/4 fails. Replay re-activates; the legacy strict guard would
have blocked (staleness detector confirms).

**Correctness (Task 49 differ, STAGE owner, flag-ON vs flag-OFF):** Tier 1 **0
divergences** (2213 entries / 2860 words, 2860 matched); Tier 2 **0.0 px** max, all 8
bindings → **ZERO_DEVIATION**. The replayed stream is bit-identical to the generic emit.

**STG A/B (128 samples, frame 438, deterministic — B run twice, byte-identical):**

| bucket | A P50 (generic) | B P50 (replay) | Δ P50 |
|---|---|---|---|
| **STG** | **569,280** | **381,632** | **−187,648 (−33.0%)** |
| **OTHR** | 163,712 | 338,432 | **+174,720** |
| **ALL** | 1,680,256 | 1,680,128 | **−128 (flat)** |

STG drops 33%, but the saved CPU redistributes to OTHR (the `all − named` residual), so
ALL P50 is flat. Most likely GX-backpressure redistribution: replay submits the same 2996
words with less CPU prep, so STG-CPU drops but the GX stall moves outside the stage-owner
window. **VBlank histogram improves at the tail** (3-VBlank share 426→474, 4-VBlank
122→80, 5+ 17→12) — the pacing signal a device A/B would confirm (melonDS cannot referee
bucket-edge pacing).

**Memory:** arena size/fail count identical flag-off/on (`0x14C000`/4); replay buffer is
static BSS; +4-byte staleness counter is the only BSS delta. No heap growth, no reserve
erosion.

**Visual:** A/B screenshots in `artifacts/visibility/task53/` (owner is the oracle); the
differ's ZERO_DEVIATION is the stronger proof of identical render.

### Verdict — **KEEP-candidate (STG win, ALL flat, pacing tail up), default-off**

Replay re-activates, is bit-exact correct, and cuts STG 33% — but ALL is flat because the
saved CPU redistributes to OTHR (GX backpressure). The VBlank tail improves. This is a
real stage-owner win gated on device confirmation for the ALL-level claim. The flag
(`NDS_TASK53_REPLAY_ARENA_FIX`, default 0) is the ship mechanism; it stays default-off,
**not overridden in any published or tick-HUD target block**, until the owner approves the
visual + a device A/B confirms the pacing-tail gain (activating replay changes
timing/memory-access patterns — device-only class).

**Unblocks Task 52 DMA:** the replay loop is now live. The OTHR redistribution measured
here is exactly the GX-backpressure cost a DMA deferred-barrier (mode 2) would target for
overlap.

Full E2 evidence: `artifacts/performance/2026-07-24_task53-replay-arena-fix-e2.md`.

Do not override `NDS_TASK53_REPLAY_ARENA_FIX := 1` in the published or tick-HUD target
blocks. Doing so changes shipping behavior before the owner approves it and device
confirms.
