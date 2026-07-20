# Task 30 — BGM refill slicing

Status: **candidate; listen and retail engagement gates passed, performance gate pending**.

Source HEAD: `91858977402e66a399d4fe6aa487d25faff296c8` plus the focused
Task-30 diff. Boundary is `battle_playable_realtime`, mode 163.

## Source and runtime contract

- BattleShip starts the stage BGM through
  `scvsbattle.c:217` -> `mpCollisionSetPlayBGM()` ->
  `syAudioPlayBGM(0, gMPCollisionBGMDefault)`. The port bridge retains that
  source event and routes track 0 to `ndsAudioBgmPlay()`.
- Dream Land remains PCM16 mono, 22,050 Hz, 2,886,710 bytes, loop start 8,798,
  SHA-256
  `581191127A00C8DDBD4395CC00B5D4722BBECA734A0990E778A2EA5E9138EFFA`.
- The DS still owns one 65,536-byte looping PCM ring on a Calico sound channel.
  Track selection, preload, loop offsets, volume/pan, channel start/stop, and
  source transitions are unchanged.
- Each steady-state 32,768-byte half refill is now four 8,192-byte reads and
  cache flushes. One slice runs per normal audio update. If the write-half
  deadline is less than two `BUS_CLOCK / 15` frame periods away, all remaining
  slices finish immediately. Match-start preload and rare resync remain whole.
- The deadline is the live
  `sNdsAudioBgmNextRefillTick + NDS_AUDIO_BGM_HALF_TICKS`; this is the next
  time playback can enter the half being written. The next byte/tick boundary
  advances only after the fourth slice completes.
- Stop, track switch, natural finish, resync, diagnostics reset, and failure
  cancel pending slice state. A playback/write-half collision fails playback
  loudly and increments the existing unsafe counter.

## Synchronized melonDS A/B

Profile 1, affine wallpaper off, mode 9, static texture AOT 1, Fox active,
frames 600–607, eight samples, no JIT.

| Metric | whole-half control P50/P95 | sliced candidate P50/P95 | P50/P95 delta |
|---|---:|---:|---:|
| audio update shell | 2,368 / 268,672 | 3,040 / 156,160 | +672 / **-112,512** |
| complete update | 156,800 / 423,104 | 157,536 / 311,424 | +736 / **-111,680** |
| source-update pair | 153,952 / 155,584 | 154,048 / 155,712 | +96 / +128 |
| draw | 1,068,544 / 1,091,200 | 1,071,392 / 1,094,144 | +2,848 / +2,944 |
| loop wall | 1,680,448 / 1,680,512 | 1,680,448 / 1,680,512 | 0 / 0 |

Candidate refill last/max is 76,160/76,224 ticks, below the 100K gate. The
window ends at 160 slices / 40 complete half refills with unsafe/overrun/read
fail `0/0/0`. Synchronized frame 607 is exactly 0/49,152 changed top-screen
pixels and mean channel delta 0.00.

Artifacts:

- control JSON/screenshot:
  `artifacts/performance/2026-07-20_task30-bgm-slice-control-early600-607.json`,
  `artifacts/visibility/2026-07-20_task30-bgm-slice-control-early607.png`;
- candidate JSON/screenshot:
  `artifacts/performance/2026-07-20_task30-bgm-slice-candidate-early600-607.json`,
  `artifacts/visibility/2026-07-20_task30-bgm-slice-candidate-early607.png`.

## Lifecycle and audio proof

- `verify-dev-fast.ps1`: pass.
- `verify-boundary.ps1`: pass.
- Published-equivalent one-minute lifecycle: pass through countdown, combat,
  KO/rebirth, Time Up, Results, and teardown with the sliced-stream guard.
- Focused Results audio: Fox winner, natural `16 -> 22` transition, plays
  `Pupupu/Mario/Fox/Results = 1/0/1/1`, 95 complete refills, 3,273,042 bytes
  read, 44,040 B/s measured versus 44,100 expected, zero audio errors/unsafe/
  overrun/cleanup failures, and 462,160 bytes reserve after the 64 KiB ring.
- The independent derived-asset check retains four exact PCM assets and a
  nonzero Dream Land acoustic signal (`peak=9,928`, `RMS=2,283.623071`).

## Retail packet and remaining gate

The first copied device pair used `LowerTextHudMode=1`, which rendered the
retired generic debug wall instead of the required phase rows. It was unusable
for this gate and has been removed from `builds/`. The corrected pair rebuilds
the same documented control/candidate revisions with `NDS_DEBUG_HUD=0`:

- control:
  `builds/task30-bgm-slice-clean-hud-pair/smash64ds-task30-control-clean-hud.nds`,
  SHA-256
  `FFC2FEA839566D0ACB93B8BF700A69B4F9079ED4B50965DE9550947FB93E6B51`;
- candidate:
  `builds/task30-bgm-slice-clean-hud-pair/smash64ds-task30-sliced-clean-hud.nds`,
  SHA-256
  `EB01127F9AA5EE997AD2143C370002C6B113D7A1FE312A12564595A7D60053F5`.

Their generated configs differ only in the embedded Git hash (`9185897`
control, `8add112` candidate); the focused Task-30 commit remains the source
delta. Both are profile 1, mode 9, static textures on, generated segment 0 on,
Task 16 `1/1/1`, affine wallpaper off, and clean phase HUD on. Repo-local
captures prove the bottom timing panel and candidate engagement:

- control: `artifacts/visibility/2026-07-20_task30-control-clean-hud-device-preview.png`,
  SHA-256 `58EB79F2365A4C92FD734DCA9078E34E9468524AF3643086A96B548799743F8F`;
- candidate: `artifacts/visibility/2026-07-20_task30-sliced-clean-hud-device-preview.png`,
  SHA-256 `FCEFE3AACBE20EAC59D5CC4A709237A0DF11296084EFE812899BE3C43AAAFE07`.

Retail checklist:

1. Boot control, then candidate with identical flashcart/loader settings.
2. At approximately the same match timer, photograph the complete bottom
   screen. Record the
   2/3/4/5+ interval histogram, max interval, BGM last/max, and candidate
   `BGM slices`; the slice counter must climb above zero.
3. Listen for an unchanged Dream Land track start. Leave Results running long
   enough to hear a Results loop seam and report any click, gap, or corruption.

Tyler reported on 2026-07-20 that
`smash64ds-task30-sliced-profile1.nds` "audio sounds good." This passes the
required human listen-quality gate. The later Task-32 retail photos preserve
the sliced path running on device at `BGM slices 180` and `184`, proving retail
engagement:
`artifacts/visibility/2026-07-20_task32-retail-control-dht0.jpg` and
`artifacts/visibility/2026-07-20_task32-retail-candidate-dht1.jpg`. Both ROMs
have slicing enabled, so these photos do not replace the whole-half versus
sliced performance A/B.

The candidate is not a KEEP or shipping promotion until the dedicated control
and candidate pacing histograms/maxima pass.
