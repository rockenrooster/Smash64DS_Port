# Standing rules for all optimization /task files

These rules apply IN FULL to every task file under `docs/optimization/`. Each task
file states "Standing rules apply" at the top instead of restating them. If a task
file contradicts this document, the task file wins only where it says so explicitly.

## File convention (the owner, 2026-07-20)

- **One task per .md file.** Naming: `ClaudeFable5_Task<NN>_<slug>_<YYYYMMDD>.md`.
  Resolved task files stay in place as history; never append a second task to one.

## Process

- `decomp/` is read-only. Port-side edits only (src/nds, src/port, include, linker,
  scripts, docs).
- Verify chain: `.\scripts\verify-dev-fast.ps1` then `.\scripts\verify-boundary.ps1`.
  Full sharded Regression ONLY if shared/imported TUs changed, once, at session end.
- Long builds run detached (Start-Process → log → poll completion stamp), never
  foreground. One full Regression sweep per session maximum.
- Separate commits per task/phase. Never leave verified work uncommitted; checkpoint
  honest WIP to a branch if a session ends mid-flow.
- Time-box open-ended debugging: ~10 emulator runs or ~1 hour, then checkpoint and
  report. Cite file:line for any verifier-expectation change.
- Final steps of every task: brief HANDOFF.md/PORTING.md update, then
  `.\scripts\New-Smash64DSSnapshot.ps1 -Mode Lean`.
- Agents cannot see or hear. Rendered-output claims need `capture-melonds.ps1`
  screenshots with non-clear-color pixel assertions (counters prove submission, not
  display). Audio-quality claims are flagged for a the owner listen check.
- Diagnostics/observer ROMs build `NDS_FAST_WALLPAPER_AFFINE=0` when
  `NDS_RENDERER_PROFILE_LEVEL>=1` (profile-1 + affine OOMs the taskman arena —
  known, unfixed, accepted). Shipping profile-0 keeps affine=1.

## Fidelity doctrine (the owner, 2026-07-20)

Pixel exactness is NOT a requirement for rendering-side changes. "If we can get 95%
of the way there while saving 50% CPU cost, I'm all for giving it a try."

- Rendering changes gate on a fidelity budget: synchronized A/B screenshot pairs,
  changed-pixel counts + mean delta reported, PNGs attached, and **the owner is the
  approval oracle** — never self-approve visuals. Soft flag threshold ≈5% changed
  top-screen pixels, and no structural artifacts (missing geometry, wrong textures,
  Z-fighting flicker).
- **Gameplay/source behavior remains bit-exact and verifier-gated.** Source updates,
  camera math, collision, RNG, the fixed-two scheduler: non-negotiable. Only the
  GX/render derivation path may approximate.

## Content-completeness doctrine (the owner, 2026-07-21)

Battle-reachable game content — SFX cues, animations, visual effects the original
game plays in the shipped scene — may NOT be runtime-excluded for capacity or
representation convenience. Fail-closed exclusion is a temporary audit state, not
a shipping state. Substituting a different asset stays banned. Any exclusion that
survives a fix task requires the owner's explicit per-item sign-off in the task
report; silent or agent-chosen exclusions are a failed task. Capacity budgets
(resident caps, reserves) are negotiable levers to present to the owner with numbers
— never reasons to drop content unilaterally.

## Merge & branch policy (the owner, 2026-07-21)

- Every task runs on its own branch `codex/task<NN>-<slug>`, branched from
  CURRENT master. Do not stack a new task branch on an unmerged task branch
  unless the two tasks are declared dependent in their files — independent
  branches stay independently mergeable and revertable.
- **A KEEP is not done until it is on master.** When all gates are green and
  docs + snapshot are committed, the session's final git actions are: merge the
  task branch into master with `--no-ff` (merge message = one-line task
  verdict), then run `verify-dev-fast.ps1` once on master. Report the merge
  commit hash.
- STOP / REJECT / WIP outcomes do NOT merge. The branch stays as the
  checkpoint; name it in the report. Evidence/docs-only commits may merge when
  verify is green.
- Merging a KEEP that is still flag-gated pending its device checkpoint is
  expected (device-economy rule) — master stays one-line revertable.
- **KEEPs ship ENABLED in the published profile-0 target at merge time (the owner,
  2026-07-21).** The Makefile flag exists so a device-checkpoint revert is one
  line — it is NOT a deferral mechanism. A task whose keep is merged but not
  forced on in the published target block is incomplete; state the published
  on/off decision explicitly in the task report.
- Master must remain stranger-buildable at every merge: publish identity pins
  (README expected SHA-256, DECOMP_PIN outputs) travel in the same branch as
  any change to the published ROM.
- **NEVER push.** Master is the public GitHub repository; `git push` is the owner's
  explicit, per-event call. No agent runs push, ever.

## Device-test economy (the owner, 2026-07-20)

Retail sessions cost the owner real time. Minimize them by CLASS, not by skipping proof:

- **melonDS-sufficient class (device deferred):** mechanisms that REMOVE CPU work —
  visible in melonDS typed buckets. melonDS A/B + engagement counters incrementing
  in melonDS + a code-review note that no device-divergent fallback exists = KEEP
  behind the feature's Makefile flag. Also report the calibration-predicted device
  delta (Task 10 multipliers: streaming ×1.50, cache-resident ×0.88, GX ×0.87,
  draw ×1.51, update ×1.73).
- **Device-only class (melonDS cannot referee):** icache/dcache locality, TCM
  residency, DMA timing, card-I/O timing, pacing near VBlank bucket edges (Task
  17/32: melonDS blind; Task 30: melonDS green-lit a retail regression). This class
  is DEPRIORITIZED in planning; when attempted, build the A/B pair into
  `builds/device-queue/` and QUEUE it — do not ask the owner to run it per-task.
- **Batched checkpoints:** one retail session per campaign (whenever the owner chooses)
  runs everything in `builds/device-queue/` plus one smoke boot of the current
  published ROM, whose shared engagement-counter HUD row proves ALL shipped features
  engaged in a single photo.
- **Every keep stays behind its Makefile flag until device-confirmed**, so a
  checkpoint revert is one line. Device evidence format: the 2/3/4/5+ VBlank
  interval histogram + typed HUD rows, normalized by sample count — never min-FPS.
