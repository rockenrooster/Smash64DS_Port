# P2-2p8: the pose clock's whole-frame step (2026-09-29)

## Where it came from

The whole-match gate profile (`builds/p2p8-prof-match`, see
`2026-09-28_p2-2p8-spline-bisection`) counted ~135 `ndsFtPoseParse` calls a
frame (`gate-pc-pose-parse.txt`): one per bound joint per tick. Most of them do
nothing but step the joint's exact binary32 clock -- two `__aeabi_fadd` calls
(wait -= speed, frame += speed) -- and return because the wait is still
positive; the function's push/pop and those two calls are its top rows.

`cf95.py` (counterfactual P95 per function: the gate's work P95 with that
function's cycles removed from every frame, `gate-counterfactual-p95.txt`) puts
the pose engine's parse at -25K and play at -37K if either vanished.

## Change (`src/nds/nds_ft_pose.c`)

`ndsFtPoseWholeStep`, taken in both run loops before the parser: at speed 1.0,
with the joint RUNNING, a wait that is a whole number w >= 2 and a frame that is
a whole number below 2^24, the step is integer arithmetic binary32 performs
exactly, and w - 1 > 0 means the parser would return right after it. The same
wait, frame and published GObj frame bits are written from integers and the
call is skipped. Everything else goes to `ndsFtPoseParse` unchanged. The bit
conversions were checked on the host against IEEE float32 for all small values
and 200K random values; the step identity for 100K random whole numbers.
Same-ROM A/B word `gNdsFtPoseWholeStep`.

ITCM: the inlined step needed 200 bytes, so `ndsRendererHardwareBindTextureName`
(192 B) and `ndsRendererHardwareApplyTextureParams` (92 B) went back to main RAM;
the whole-match census ranks them at the bottom of ITCM rent (~2.9K and ~1.6K
cycles/byte over a whole match).

## Measured (same ROM, word 0 -> 1; runs in `2026-09-26_p2-2p8-ftr-item-tail/`)

| Run | P50 | P95 | P99 | replay |
|---|---|---|---|---|
| Gate `ws0/ws1_gate` | 935,744 -> **930,240** | 1,274,240 -> 1,274,944 | 1,468,352 -> 1,468,480 | IDENTICAL (2 ring-stop seam words) |
| Saffron lab `wsl0/wsl1_fp_g7` | 1,154,112 -> 1,150,848 | 1,568,320 -> **1,564,352** | | IDENTICAL |
| Kirby x4 Dream Land `wsl0/wsl1_kk_g6` | 903,232 -> 899,776 | 1,202,112 -> **1,196,544** | | IDENTICAL |

The gate's SRC median fell 4.8K; its P95 did not move. Hitlag skips the whole
animation update (`ftMainProcUpdateInterrupt` calls `ftMainPlayAnimEventsAll`
only when `hitlag_tics == 0`), so the step saves nothing in hit frames, and
the gate's tail is hit and status-change frames.

## Tried and reverted: skipping fighter joints' `mobj` reads

`ftParamUpdateAnimKeys` reads every joint's `mobj` each tick; the census put
~350 D-cache fills a frame in that function. The attempt moved the MObj
playback into one pass after the joint loop (material and joint animations do
not read each other) and kept, per fighter, which joints had no MObj, invalidated
by any `gcAddMObjForDObj` call. Same ROM, mask 0 -> 1 (`mm0/mm1_gate`): P50
933,376 -> 933,376, P95 1,275,328 -> 1,281,408; against the previous ROM the
reordered pass alone read P50 +3.1K (`ws1_gate` -> `mm0_gate`). Items and
effects add MObjs 684 times a match, which invalidates every fighter's record
each time; the reads were not the cost the census suggested. Reverted.

## Also in this batch's evidence

`longstall.py` (`gate-long-stalls.txt`): instructions averaging over 100
cycles are ~32K ticks a frame outside the idle wait -- the stage GX draw's
DMA0 poll (9.2K, the drain removed in `7a0708dd905`), the fighter packet
submit's first main-RAM access after arming its DMA (3.1K), and literal-pool
loads in the stage and fighter display code stalled behind GX DMA bursts.
