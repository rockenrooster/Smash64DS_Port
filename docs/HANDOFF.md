# Handoff

Updated: 2026-07-17 19:26 Central
`P1_EXECUTION_BOARD.md` owns all current state. This is only the restart surface.

## Restart
Branch: `master`

Boundary: `battle_playable_realtime`, mode `163`.

```powershell
.\scripts\verify-all.ps1 -Profile Boundary -List
git status --short
```

Preserve the published intrinsic mode-9 / mip-0 / static-residency /
source-countdown configuration. Dream Land water is exact frame 0/fraction 114
on the original 12 triangles; the animated replacement is removed.

The integrated fixed-two candidate is 14,655,488 bytes, SHA-256 `70DCD33BFC19D39460B2199FFD94606368DF03EEDD5984C067CA5CAFCC9ABFF2`.
Stage painter depth and pause-orbit containment are fixed and user-confirmed.
M3 retains no-Z codegen, dense prepare-once, AOT coordinate shifts, the
zero-shift matrix builder, exact bounded `s16` rounding, Task 6 first-use
attribute preparation, and the valid-color stage seam. Combat stage is now
489,184/489,536 ticks P50/P95.

The M4 Whispy lifecycle repair is kept. Countdown assets now reverse the source
odd-row texture interleave: big GO is direct RGB555+A1 OAM, the opaque shaded
traffic box is A3I5, and only the foreground flare is A5I3. Compact source
atlases use 57,344 texture bytes total and restore pre-GO source-frame residency;
do not add transition-time texture deletion. The large RGB555+A1 GO and the
source-backed point-sampled 12x9 traffic-box `ShadowGo` now pass the full
runtime gate; the countdown playtest finding is fixed.

## One-Minute Gate

Mode 163 uses locked-30 fixed-two pacing: exactly two source updates per present,
with no vblank debt or catch-up. Slow frames uniformly slow gameplay; zero-slip
phases must also hold 59.0..61.0 updates/s. Old baselines must be resampled.

The published/manual ROM and source runtime gates retain flag `1`, preserving
Fox CPU and the original Wait → countdown → GO/timer path. The separate
`-FastIteration` screenshot launch selects `0` before battle, skipping the
countdown and Fox decisions and freezing the timer at `1:00`. The focused
source-timer gate passes:

```powershell
.\scripts\check-one-minute-match-verifier.ps1
.\scripts\verify-battle-playable-one-minute-match.ps1
```

The isolated published-equivalent proof passes natural fixed-two qualification:
4,084 committed updates / 2,042 presents, phase rates
39.9/37.4/39.3/n.a./58.2 updates/s, and phase slips 196/1088/946/0/3. It
exercised imported level-3 Fox AI, expired at 3,600 source ticks, reached
Results, retained 166,672 bytes after BGM, and reported exactly one normal M4
teardown with every post-GO fence counter zero. Its ROM is `9C35F4B3...`; the
isolated build leaves the public ROM byte-identical.
The DS taskman seam matches BattleShip by breaking on `LoadScene` before drawing;
the verifier samples the battle ledger before Results reuses the globals.

## Integrated July 17 Candidate

- Source countdown verifier passes with GO `3 OBJ + 10 quads`, 31,168 OBJ,
  57,344 texture, 608 palette bytes, and zero gameplay conversion/upload.
- Natural visual-effects proof passes: 3 created, 13 rendered, 96 triangles,
  kind mask `0x45`, zero rejects, 176,464-byte reserve.
- The 107,536-byte FGM pack covers 18 exact IDs / 16 unique samples and 11
  collision cues. Natural phase qualification passes 14 plays, 21 envelope
  steps, max 3 live handles, and 174,864-byte headroom. IDs 429/435 continuous
  pitch and fork voice 685 remain explicit fidelity debt.
- Current canonical startup reaches frame 212 with M4 `22/131072`, 646 hits,
  zero hot conversion/upload, and zero post-GO fence work. The full current
  one-minute lifecycle passes through Results and teardown.

Task 9's unchanged-libgcc ITCM KEEP moves the source owner
311,744/312,960 -> 260,192/261,312 and matches all 3,892 state rows. Current
ITCM after the emitter split, source-light repair, and raw-corner cut is
28,020/32,768;
gameplay's 16 KiB main-RAM stack has 8,284 bytes measured headroom. Phase 2
stays skipped.

The M2 raw/cross emitter split remains retained. The next measured cut reuses
the fully overwritten capture arena instead of clearing 6,240 bytes per fighter:
detailed capture falls 47,296/47,360 -> 41,152/41,152, combined fighter falls
452,640/455,808 -> 446,464/449,600, and draw falls
1,077,568/1,080,832 -> 1,071,488/1,074,816 with exact pixels and non-timing
state. Raw corners now carry only their 10-bit dense ID, removing 1,746
dynamic masks per frame and shrinking the raw emitter 0xD0 -> 0xBC. Current
post-light-repair ledger-off is 385,088/388,224; the older
372.1K sample is retired. Generic/fast profile 2 remains exact on frames
180..187 with 686 triangles. Boundary passes on `70DCD33B...`; current
profile-0 smoke is 22.1 FPS, so full-speed remains red.

Natural KO/rebirth uses real source events and measures 1,261,344/1,524,864 and 1,110,528/1,112,256 active ticks with exact stage/M4/fence contracts.

## Checkpoint
Effects, FGM, Task 9 identity, one-minute lifecycle, natural KO/rebirth timing,
the M2 split, source-exact light parity, capture reset, raw-corner cut, and
Boundary pass are retained. The countdown finding is fixed. The source-backed
point-sampled tiny `ShadowGo` changes 49 atlas pixels only inside its exact 12x9
rectangle and passes its 70-pixel crop lock across all five GO frames with
10/10/10 draws and zero hot conversion/upload. The apparent large-GO mismatch
was a stale crop lock after the later source-light repair: 125/26,400 pixels
changed, all inside Mario's 22x14 area, while the GO RGB555 payload remained
byte-identical. The rebuilt full verifier passes with crop `d968b0cc...`, GO
`3 OBJ + 10 quads`, 31,168 OBJ bytes, 57,344 texture bytes, and 608 palette
bytes. No GO source change was needed.

The Down+A report remains open. The audio-load hypothesis is disproven and an
observer-free target stall is now proven.
The verifier-only pre-spawn override makes Fox human P2 without changing shipped
mode 163. Fox enters status 213 / motion 188 with exact asset `FTFoxAnim129` /
`0x303`. Source trace maps the six observed callbacks to normal animation
frames 2..7; callback 6 completes the second ID-190 event, after which the
same-frame map step may transition Fox, including to status 219
`LandingAirNull`. Exact tracing sees both ID-190 calls and both tick-7 attack
refreshes return, followed by entry into the next effect-script `SyncWait`.
ID 190 is absent from the resident 18-entry FGM pack, so
`ndsAudioFgmPlayAtPan` increments the unsupported counter and returns before
`soundPlaySample`; NitroFS reads occur only during the fenced pack load. The
reported `nitroromReadIter` / `_lseek_r` PCs were initial GDB attach locations,
not terminal samples. Do not add ID 190 to the pack as a stall fix; its source
pitch schedule and custom-FX fidelity remain a separate unqualified audio item.
The common-bank repair also restores Fox's omitted files 642..661
(`0x282..0x295`) and passes its static source-identity checker; runtime
qualification is pending.

`Invoke-GdbMarkerScript -MiInteractive` and the verifier's observer-free and
freeze-diagnostic arms are retained. The observer-free arm stops only at the
first Down-Air entry, disables all five observer breakpoints, resumes, and
interrupts through GDB/MI after 12 seconds. Its target snapshot is unchanged
from the earlier 4-second sample: logic 6->12, presents 3->6, reads 11->19,
status 213, motion 188, tic 6, animation frame `0x40e00000` (7), motion frame 0,
update `0x02070ded`, map `0x0203a385`, and timer 3600/0. Therefore the target
itself freezes at the frame-7 boundary. The observer-free command intentionally
fails its advancement assertion while preserving the decisive snapshot:

```powershell
pwsh -NoProfile -File .\scripts\verify-battle-playable-down-air-stall.ps1 -Actor Fox -ObserverFreeSnapshot -NoBuild -RunnerSlot 3 -TimeoutSeconds 120
```

The `-FreezeDiagnostics` arm selects the intrinsic isolated diagnostic ROM,
reuses the same human-P2 Fox route, and now stops at the retained exception
handler after first Down-Air:

```powershell
pwsh -NoProfile -File .\scripts\verify-battle-playable-down-air-stall.ps1 -Actor Fox -FreezeDiagnostics -NoBuild -RunnerSlot 3 -TimeoutSeconds 120
```

The retained run captures exception flags `2`, status 213 / motion 188 / tic 6,
context PC `0x020631f6`, and saved caller return `0x02088873`. The faulting PC is
`gmCollisionGetFighterPartsWorldPosition` at `gmcollision.c:494`, dereferencing
its `main_dobj` argument. The caller is `ftMainProcPhysicsMap` at
`ftmain.c:1884`; immediately before the call it loads `attack_coll->joint`.
Repeated captures pass invalid small values including `0x1a0`, `0x1ac`, and
`0x206` instead of a DObj pointer. The scheduler snapshot has Calico main as the
current thread and shows `0x01ffebe4` as the seeded idle-thread WFI context, so
the earlier IRQ-wait conclusion is disproven.

Resume by tracing every live write to `FTAttackColl.joint`, the motion-event
joint decode, and the imported structure layout/stride against BattleShip
source. Identify where the frame-7 attack refresh installs the small value and
fix that shared seam. Do not add a collision-transform guard, a Down-Air special
case, or ID 190 as a stall fix.

Do not mark the playtest finding fixed until Fox P2, Mario, and one canonical
CPU-on Current gate pass. The five-minute goal heartbeat is intentionally
paused for the requested model-change checkpoint.
