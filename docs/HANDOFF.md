# Handoff

Updated: 2026-07-16 05:12 Central
`P1_EXECUTION_BOARD.md` owns all current state. This file is only the restart
surface.

## Restart

Branch: `codex/wip-natural-combat-source-start-collision`

Boundary: `battle_playable_realtime`, mode `163`.

```powershell
.\scripts\verify-all.ps1 -Profile Boundary -List
git status --short
```

Preserve the published intrinsic mode-9 / mip-0 / static-residency / bitmap-OAM
configuration. Dream Land water is exact frame 0/fraction 114 on the original
12 triangles; the animated replacement and its dead implementation are removed.

The executable fleet is four registry records under `Latest`/`Boundary`; the
unreachable source-side mode 1-162 lattice is separate ROM-parity cleanup.
The current user-facing candidate is 14,565,376 bytes, SHA-256
`593FBBA217D2AD7F9F87DE2013F38C82517A1DDDF1FE36CDF6110894C379C91E`;
its next Boundary republication is intentionally deferred to the release
checkpoint.
The normal-play stage painter bug is fixed. BattleShip's 66 source-Z / 126
no-Z classification was already correct; the DS counter advanced only one
sixth of a submitted v16 step, so groups of six painter triangles collided.
The final zero-collision profile-2 census is 202 triangles. Published-ROM pixel
evidence is `artifacts/visibility/20260715-stage-depth-final-cutg-frame438.png`
and `artifacts/visibility/20260715-stage-depth-final-moving-frame501.png`.
Clip-space near-plane containment now removes the pause-orbit corruption and
Tyler confirmed the fix. Paused −33.6° is the preferred Mario underside view.

The retained M3 path is now 611,392/611,584 stage ticks at frames 438–445 after
removing erroneous cold/Os codegen from the 126-triangle no-Z emitter. Exact
121/828 ownership, frozen water 2/0/1, 22/131072 static residency, zero post-GO
texture work, and identical DS pixels remain intact. M3 is still REWORK,
111,392 ticks above its first 500K gate.

## Rejected M3 Stack

The current dense prepare-once stack was exact but too small. Against the kept
611,392 baseline it measured 563,296, still above 500K, so the stack-only rule
required a full revert. Incremental no-Z matrix transport then regressed to
579,712 and was also removed. Evidence remains at:

```text
artifacts/performance/2026-07-16_m3-dense-reuse-b.json
artifacts/visibility/2026-07-16_m3-dense-reuse-b-frame445.png
artifacts/performance/2026-07-16_m3-noz-projection-b.json
```

Do not retry either cut without a new attributable bound.

## One-Minute Gate

The published/manual ROM and source runtime gates retain flag `1`, preserving
Fox CPU and the original Wait → countdown → GO/timer path. The separate
`-FastIteration` screenshot launch selects `0` before battle, skipping the
countdown and Fox decisions and freezing the timer at `1:00`. The focused
source-timer gate passes:

```powershell
.\scripts\check-one-minute-match-verifier.ps1
.\scripts\verify-battle-playable-one-minute-match.ps1 -RunnerSlot 2
```

The run completed 3,891 logic updates, exercised imported level-3 Fox AI,
reached Time Up and Results, retained 163,312 bytes after BGM, and reported one
normal M4 teardown with every post-GO fence counter zero. The DS taskman seam
now matches BattleShip by breaking on `LoadScene` before drawing; the verifier
samples the battle ledger before Results reuses the globals.

## Rejected M2 Cut

The Mode-8 ITCM-only experiment was exact but too small. Frames 600–607 measured
416,576/416,704 → 398,496/398,592 combined fighter ticks, saving only 18,080
versus the required 80K. Stage pixels were byte-identical; only 13 bottom-HUD
pixels changed with live FPS text. The three placements/checker requirements
were reverted. Evidence remains at:

```text
artifacts/performance/2026-07-15_m2-itcm-a.json
artifacts/performance/2026-07-15_m2-itcm-b.json
artifacts/visibility/2026-07-15_m2-itcm-a-frame607.png
artifacts/visibility/2026-07-15_m2-itcm-b-frame607.png
```

The source-faithful pre-GX rejection for active animlocks/shuffle remains; it
falls back to the ordinary BattleShip path before native preparation or GX.

Jump C also stopped before renderer code. Current Mode-8 local construction is
53,824 ticks; the live RPY builders already use BattleShip's sine table and
integer products. Lighting is already prepared-direction plus the exact shade
LUT. Even deleting the complete local bucket and re-adding the rejected 18,080
ITCM gain bounds at 71,904, 8,096 short of the first gate.

## Next Packet

Qualify the throw-origin recovery route using the existing sparse mode-163 gate.
The attack-origin DamageFall path is now green: five source up-smashes, two
damage events, one status-54 DamageFall, one line-3 crossing, direct
result/invalid `1/0`, and 214,544 bytes reserve. Evidence is
`2026-07-16_050836-0416581_damagefall-recovery-p11728.png` under
`artifacts/visibility`. Read the exact BattleShip catch/throw/release source,
run the focused throw gate once, and change only its first reproduced seam.

## Checkpoint

Run one widest relevant verifier only: Boundary for battle-only work, or Current
instead if normal/shared startup changed. Finish docs and `git status`
inspection, commit, then run the Lean snapshot as the final command.
