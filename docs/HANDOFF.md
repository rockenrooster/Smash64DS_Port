# Handoff

Updated: 2026-07-15 15:46 Central

This is the exact restart surface. `P1_EXECUTION_BOARD.md` owns the queue,
`STATUS.md` owns short current truth, and `PORTING.md` is history.

## Graceful Pause Point

All three subagent lanes were stopped before this checkpoint. No repo build or
emulator process remains. The pre-existing user melonDS PID 36352 was left
untouched. No Lean snapshot was made because this is an unfinished pause.

Branch: `codex/wip-natural-combat-source-start-collision`

Checkpoint HEAD: `23c919cd27`

The only dirty source file is
`src/port/reloc_backend_renderer_dl.c`: 415 added M3 adapter-helper lines plus
one compile correction. It collects the exact eight Dream Land owners, 57
DObjs, 42 bindings, matrices, four material snapshots, and exposes prepare /
ordered commit / finish helpers. It is intentionally not yet wired into the
display loop.

The first compile found one forward-reference typo. It was corrected from
`ndsFighterDLAllDrawResolveBranch` to the already-declared
`ndsFighterDLDrawResolveBranch`. The corrected aggregate `scene_backend.o`
compiled successfully (4,039,928 bytes, 15:43:24). The 90-second full lab
command ended before link/publication, so there is no M3 lab ROM and no device
claim. Resume by wiring prepare/commit/finish, then rerun the command below.

## Resume Order

1. Reconcile Boundary membership and status; preserve the dirty M3 helper.
2. Finish M3 interception in the existing BattleShip display order. Mode 9
   must retain the accepted Mode-8 Mario/Fox owner and replace only the eight
   Dream Land stage callbacks.
3. Compile/link the isolated M3 lab; run host/ARM/static gates before emulator.
4. Integrate M4 after M3 settles: prepare water residency before current static
   allocations; reset it at battle teardown; replace M3 runs 42–43 / bindings
   31–32 only, retaining run 41 / binding 30 and its shared parent matrix.
5. Run one synchronized eight-frame same-ROM A/B/A, with screenshots only in
   `artifacts/visibility`. Do not run Full, Regression*, or P1Gate.
6. Remove rejected Mode-7 runtime code/temporary verifier allowance, refresh
   the two root ROMs only after acceptance, update docs, commit, and make the
   Lean snapshot the final project command.

## Exact Commands

```powershell
.\scripts\verify-all.ps1 -Profile Boundary -List
git status --short
$env:DEVKITPRO = 'C:/devkitPro'
$env:DEVKITARM = 'C:/devkitPro/devkitARM'
make TARGET=smash64ds-battle-playable-m3-stage-owner-lab `
  BUILD=builds/build-m3-stage-owner-lab `
  NDS_DEV_SCENE_HARNESS=battle_playable_realtime `
  NDS_DEV_LIVE_INPUT_PREVIEW=1 NDS_HARNESS_FAST_LOGIC=0 `
  NDS_RENDERER_HW_TRIANGLES=1 NDS_RENDERER_PROFILE_LEVEL=1 `
  NDS_RENDERER_FAST_RUN_DEFAULT=9 -j16
python .\scripts\check_nds_native_stage.py
.\scripts\check_nds_native_stage_arm.ps1
.\scripts\check-pupupu-water-tiled-aot.ps1 -Fast
.\scripts\check-pupupu-water-residency-arm.ps1
.\scripts\check-pupupu-water-draw-arm.ps1
```

The last completed focused outputs are:

```text
M3_NATIVE_STAGE_CHECK_OK callbacks=8 dobjs=57 bindings=42 runs=54
  epochs=49 triangles=202 cross=5/10/15 slab_bytes=12663
M3_NATIVE_STAGE_ARM_OK rodata_bytes=12663 production_linked=1
M4_PUPUPU_WATER_RESIDENCY_ARM_OK payload_bytes=167936
  runtime_upload/io/alloc=0/0/0 draw_proven=0
M4_PUPUPU_WATER_DRAW_ARM_OK cells=68 triangles=138 vertices=414
  text=840 max_stack=64 runtime_upload/io/alloc=0/0/0 draw_proven=0
```

## Milestone Truth

- M1 is accepted: retained affine BG2 costs 1,856 ticks, below 35K.
- M2 Mode 8 renders correctly. The latest detailed-ledger A0/A1 is
  477,152/477,376 ticks. Mode 7 is rejected at 518,336/518,784 and produces
  blank fighters. A read-only direct-contract design is structurally feasible,
  with an estimated 62–75K net saving, but is unimplemented and unmeasured.
- M3 has an exact 12,663-byte packet and compiled renderer core. Adapter helpers
  compile; display interception, final link, device counters, timing, and
  screenshot are unfinished. First keep gate: stage <=500K and >=300K saved.
- M4 has an exact one-pass 167,936-byte pre-GO payload and exact 138-triangle
  draw helper. Runtime prepare/draw/teardown integration, device reserve, and
  zero-post-GO fence proof remain unfinished.

The last control smoke window was about 17.7 FPS with draw about 1.646M and
loop about 2.241M ticks. No accepted M2–M4 performance cut has replaced it yet.

## Stable Project Rules

Use only repo-local melonDS and scripted TOML/window configuration. Automated
computer use only; Tyler manually tests built ROMs. Keep Fox decision/input off
by default until Tyler asks to re-enable it; CPU-on lifecycle proofs remain
required. The P1 source rule is one minute (`3600` ticks). Publish exactly
`smash64ds.nds` and `smash64ds-battle-playable-hwtri.nds` at repo root; all lab
outputs belong under `builds/`.

The current published P1 ROM remains the previously verified artifact; this
pause did not rebuild or replace it:

```text
smash64ds-battle-playable-hwtri.nds
14,368,768 bytes
SHA-256 E08C6C9EA29F671EE5AA9D9D6491B1B12E80A1DBC348AF99468CA72BE072425F
```

Compilation alone does not close M2–M4.
