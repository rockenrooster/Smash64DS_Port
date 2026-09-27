# P2-2p8 A9: cpuGetTiming without its call chain (2026-09-27)

**Outcome: BANKED (instrument overhead, not game work).**

## What the instrument cost

The per-PC profile (`builds/p2p8-tail-profile-bc35`, 385 frames) enters
`tickGetCount` 499 times per presented frame. Each `cpuGetTiming` is a Thumb
call in main RAM to Calico's `tickGetCount`, which masks IME around five I/O
accesses: 45,288 + 19,052 cycles = ~32K ticks per frame, all inside WORK-H.
The shipping ROM has no tick-HUD brackets, so this is measurement, not game.

## Change

`-Wl,--wrap=cpuGetTiming` and `__wrap_cpuGetTiming` in `src/nds/nds_platform.c`:
ARM code in ITCM that returns the same value from the same state -- Calico's
TIMER2 count, its overflow count and tickRef, and its pending-overflow rule --
but retries on a changed overflow count instead of masking IRQs. Calico keeps
the two words static, so their addresses come from the literal pools of
`tickGetCount` and the real `cpuGetTiming`, accepted only if the surrounding
instructions decode as expected. Init brackets 64 fast reads between real ones;
any disagreement leaves the fast path off (every call then goes to the real
function). The earlier TIMER0/1 cascade clock (+2^22 on 21% of frames) is not
reused: this is the same clock.

## Result

`fasttime` `E6DB1E1A` against `arena160final` `18A992BB` (route 1, frames
2..1973): WORK-H P50/P95/P99 1,328,512/1,839,488/2,263,936 ->
1,314,880/1,822,144/2,232,704; VBlanks 2/3/4/5+ 274/1,477/197/25 ->
296/1,471/182/24. Every top bucket falls a little (SRC P50 -8.6K, MISC -4K).
`gNdsFastTimingEnabled` 1, `gNdsFastTimingMismatch` 0; no frame reads a 2^22
step (the two ALL > 3M frames are the same load events as control). Replay
digest IDENTICAL. Evidence: `fasttime-route1` (json/rows/log).
