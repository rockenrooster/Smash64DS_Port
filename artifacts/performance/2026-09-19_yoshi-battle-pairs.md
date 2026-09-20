# Yoshi battle visibility — 2026-09-19

Status: **IMPLEMENTED_NOT_ACCEPTED**; owner queue remains open.
Brief: `docs/p2/BUGS_REMAINING_DIAGNOSIS_2026-09-19.md`.

## First divergence

The current full-content diagnostic ROM
`F4AC5B10E56B83FB36BFB15D2B89B986E31F8CD3961DE09CAFB1B7518D7334E6`
reproduced invisible Yoshi in Wait on Dream Land after the natural menu route.
At presented frame 240 / source time 88, status 10, motion 4, detail 1 and
is_invisible 0: Yoshi emitted zero triangles; Fox emitted 306 and stage 202.
The native failure and slot-reject counters stayed zero. The old probe's total
scene-output check incorrectly reported PASS; the screenshot shows no Yoshi.

The focused model witness shows case-1 joints pointing at `{0xdf000000, 0x3308}`
identity cells instead of the source `{pre-DL pointer, post-DL pointer}` table.
The renderer therefore reads 0x3308 as a pointer and declines it outside any
loaded file (stage 2, owner 8, selected 16, index 0, asset 0xffffffff).
Yoshi's high native owner image is resident: 19,196 bytes, heap generation 7.

BattleShip `338_YoshiModel.c` declares the 152-byte table at 0x3308 as `Gfx *[38]`.
The battle pack's structural closure tested only the base type name `Gfx`, so it
mistook pointer arrays for command payloads and replaced the array with a root
identity. This is the battle producer, not the already-repaired CSS preview
producer. No force-visible workaround or new root-program table is needed.

## Repair and checks

- A shared producer predicate treats only pointer-depth-zero Gfx/Vtx as geometry.
  Pointer arrays remain structural data; their pointed-to geometry becomes the
  existing native identity cells. Geometry-overlap and texture-command guards
  retain their validation strength.
- A source-derived regression walks Yoshi's DObjDesc pointers and checks both
  pair members against O2R relocation targets. Before the fix it fails at
  `missing structural pair/descriptor span 0x3308`.
- After the fix: **28 tests + 10 subtests PASS** (18.56 s), covering battle
  texture/foreign closure, preview core packs and preview loading.
- The scene probe now measures output per fighter over the sampled frame;
  stage/opponent output can no longer mask an empty source-visible fighter.
- Baseline ROM/ELF/config retained in `builds/2026-09-19-yoshi-baseline/`.
- Initial observations: `artifacts/verification/2026-09-19_yoshi-current/`;
  screenshot `artifacts/visibility/2026-09-19-yoshi-wait-current.png`.

## Frozen candidate observations

ROM `4AF1224F5C5FEBF34E4FC3984C59806BE21E7CD292B04C9F95F26AA6A0559D44`;
ELF remains `7204FDF4A1725E3578CB65E2089EDE907DC7761E7F88746CA3797130D9318A1E`.
Only generated data changed; build enforcement reports 315 native-only inputs.
Among twelve battle packs, ten hashes are unchanged. Yoshi grows 24,652 ->
25,292 file bytes, Samus 35,084 -> 35,356. The shared classification affects
their structural Gfx-pointer tables, not native command/vertex payloads.

- Yoshi Wait: now visible, 320 native triangles; Fox 306; native declines,
  failures/rejects, pack errors and OOM 0. Heap minimum 14,908 / arena 921,088.
  Joint 5 now points at `{NULL, valid native root cell}` instead of ENDDL data.
- Yoshi Run: natural held stick reaches status 16; body adds 320 triangles.
- Yoshi Up-B: natural button input reaches status 222; a host breakpoint after
  the hardware-triangle increment proves the existing egg owner engages.
  A later current-frame capture (source time >=140) visibly shows the egg
  outside the GO overlay, with the fighter still drawn and no native failures.
- Samus sibling: natural Shield -> EscapeF reaches status 156 and the nonzero
  morph root program, 80 native triangles, no rejection; the ball is visible.
  This does not close backwards roll, Bomb, both details or other Samus rows.

The first movement collector used a proof-only counter absent from the normal
diagnostic ELF. Its Run-state output was observed but the capture was incomplete.
The corrected collector uses source-located post-triangle breakpoints without
adding production instrumentation or enabling a synthetic tour; the complete
Run and Up-B observations above passed after that correction.

Evidence: the `*pairs-fixed*` and `*witness-fixed*` transcripts/JSON under
`artifacts/verification/2026-09-19_yoshi-current/`; dated PNGs in
`artifacts/visibility/2026-09-19-yoshi-*.png` and
`artifacts/visibility/2026-09-19-samus-roll-pairs-fixed.png`.

The final collector makes the effect breakpoints opt-in with
`-YoshiEffectDiagnostics`; ordinary Yoshi body probes do not require the
uncommitted native effect owners. Add that switch when replaying the Up-B
conditions involving `$yoshi_egg_draws` or `$yoshi_egg_frame`. No ROM changes.
Read-only review of the producer and regression found no actionable issue.
The final default collector was runtime-checked without effect breakpoints:
`2026-09-19-yoshi-default-collector`, exit 0, Run status 16 at frame 205,
Yoshi 320 / Fox 306 triangles, neither invisible, zero native/reject failures.
This rerun covers the changed collector default, not a new game-code candidate.
PowerShell parsing, generated JSON parsing, diff whitespace and docs checks pass.

### Reproduction and scope

Host check:
`python -m pytest scripts/fighters/test_battle_core_texture_closure.py scripts/fighters/test_preview_core_packs.py scripts/fighters/test_preview_pack_loader.py -q`.
Build:
`make TARGET=smash64ds-p2-shell-hwtri BUILD=build-crash-diagnostic NDS_P2_1P_GAME=1 NDS_P2_COMPACT_BATTLE_FIGHTERS=1`.
Frozen config has `NDS_HARNESS_FAST_LOGIC=0`, `NDS_P2_YOSHI_BUG_PROOF=0`,
`NDS_P2_PROOF_FIGHTER0=-1`, campaign and compact fighters both 1.
Runtime uses the repo-local accurate melonDS slot 8 with JIT disabled from boot.
The diagnostic shell walks menus; battle actions use pad input without status,
position or visibility writes. This is not final human-input shipping proof.

Replay the focused current-frame egg witness with the final collector:

```powershell
pwsh -NoProfile -File scripts/diagnostics/probe-native-render-scene.ps1 `
  -Name yoshi-pairs-egg -RunnerSlot 8 -StageKind 6 `
  -Fighter1Kind 6 -Fighter2Kind 1 -Presents 240 -TimeoutSeconds 180 `
  -Rom builds/build-crash-diagnostic/smash64ds-p2-shell-hwtri.nds `
  -Elf builds/build-crash-diagnostic/smash64ds-p2-shell-hwtri.elf `
  -OutputDirectory artifacts/verification/yoshi-pairs-replay `
  -Pump special -StickY 80 -YoshiEffectDiagnostics `
  -Condition '($yoshi_egg_frame == $fttick) && (gSCManagerBattleState->time_passed >= 140)'
```

Baseline is commit `aac91275fbffc0753ef90209bffbcb8e223e2fe9` plus the preserved
owner working tree. The scoped repair is the battle producer, its regression,
the producer-generated manifest and host-only scene collector. The observed ROM
also includes the owner's uncommitted Yoshi egg/entry/Egg Lay native executors,
generators, wiring and other existing work; those are not part of this repair's
checkpoint. A clean checkout of just this producer fix is not claimed to recreate
the full observed working-tree ROM.

## Integrated verdict and next action

One frozen-batch Boundary invocation completed with exit 1:
`pwsh -File scripts/verify-all.ps1 -Profile Boundary -RunnerSlot 8`.
Log: `builds/2026-09-19_yoshi-pairs-boundary.log`.
Static/source/generator checks passed. The shell-loop arm passed one lap / ten
scene entries, flat deterministic highs, bounded variable highs, free floor
118,796 bytes and zero faults. Realtime then stopped at the existing CSS
CPU-level fixture mismatch: expected Fox level 2, observed
`CPU_CONFIG=0,1,3,1,1,1,0,0,1` (level 3). This is the same blocker recorded in
the prior crash receipt, not evidence that this producer repair passed Boundary.
Four-CPU stress and full-match performance were not reached. Do not weaken the
fixture or change gameplay defaults simply to make this gate pass.

The accepted root ROM `00E87777...53BFBCE` has not been replaced; its matching
ROM/ELF/map/config are also preserved in `builds/2026-09-19-owner-accepted-crashes/`.
The new lab ROM is not a publishable replacement. Grab/throws, Egg Lay,
entry/respawn, low detail and the rest of the owner's diagnosis remain due.
Ticks, WORK-H P50/P95, whole-match FPS and cadence are **unmeasured** for this
batch; short breakpoint probes establish output, not performance acceptance.
Next after checkpoint: continue the missing natural Yoshi action/detail proofs,
then repair/requalify the CPU fixture before publication.

Checkpoint recovery: the owner authorized moving the stale zero-byte index lock
and requested no further approval questions for verified stale-lock recovery.
After rechecking its identity and absence of related Git processes, it was moved
recoverably to `builds/stale-index-20260919-193656.lock`. No repository data was
deleted. Nine unrelated staged paths, including two deletions, are excluded from
this scoped checkpoint; the accepted root ROM remains unchanged.
