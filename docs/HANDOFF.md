# Handoff

Updated: 2026-07-17 14:12 Central
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

The integrated fixed-two candidate is 14,613,504 bytes, SHA-256 `FE0C8893C37F43934DB4BEEB8169F52BB0AADEB97C5C4BA07B69416A37B743A9`.
Stage painter depth and pause-orbit containment are fixed and user-confirmed.
M3 retains no-Z codegen, dense prepare-once, AOT coordinate shifts, the
zero-shift matrix builder, exact bounded `s16` rounding, Task 6 first-use
attribute preparation, and the valid-color stage seam. Combat stage is now
489,184/489,536 ticks P50/P95.

The M4 Whispy lifecycle repair is kept. Countdown assets now reverse the source
odd-row texture interleave: big GO is direct RGB555+A1 OAM, the opaque shaded
traffic box is A3I5, and only the foreground flare is A5I3. Compact source
atlases use 57,344 texture bytes total and restore pre-GO source-frame residency;
do not add transition-time texture deletion. The large GO is clean, but the
separate 12x9 traffic-box `ShadowGo` is still unreadable at the current 0.8x DS
footprint and remains an open playtest finding.

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
180..187 with 686 triangles. Boundary passes on `FE0C8893...`; current
profile-0 smoke is 22.3 FPS, so full-speed remains red.

Natural KO/rebirth uses real source events and measures 1,261,344/1,524,864 and 1,110,528/1,112,256 active ticks with exact stage/M4/fence contracts.

## Checkpoint
Effects, FGM, Task 9 identity, one-minute lifecycle, natural KO/rebirth timing,
the M2 split, source-exact light parity, capture reset, raw-corner cut, and
Boundary pass are retained. Countdown is no longer broadly marked fixed: only
the large GO is accepted, while the tiny traffic-box GO remains unreadable.
The Down+A asset seam is corrected but not closed: BattleShip's complete Fox
combat bank is now mapped coherently from files 751..771 (`0x2ef..0x303`), with
Down-Air at `FTFoxAnim129` / `0x303`. The source-identity checker passes and the
canonical ROM builds. Mario status 213 / motion 188 completed eight updates
across four presents with clean `1/2` payload/header reads and 203,536-byte
reserve (`2026-07-17_134614-9673720_down-air-mario.png`).

The WIP Fox arm in `scripts/verify-battle-playable-down-air-stall.ps1` promotes
Fox to human P2 only before fighter creation; the shipped mode-163 CPU setup is
unchanged. Aligned GDB writes avoid melonDS byte-write disconnects. Live evidence
proves Fox is human player 1, bound to controller 1, receives tap `0x8`, and
enters status 20 (KneeBend), but it has not yet reached Down-Air before timeout.
Resume by instrumenting `ndsBaseFTCommonKneeBendProcUpdate` to classify its
animation counter/length, then rerun:

```powershell
pwsh -NoProfile -File .\scripts\verify-battle-playable-down-air-stall.ps1 -Actor Fox -NoBuild -RunnerSlot 3 -TimeoutSeconds 90
```

Do not mark the playtest finding fixed until Fox P2, Mario, and the canonical
CPU-on Current gate pass. Then timebox the single point-sampled tiny-GO experiment.
