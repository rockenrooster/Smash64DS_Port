# P2-2p8: status-change path overheads (2026-09-27)

**Outcome: BANKED (three exact cuts measured together).**

## Where status-change frames spend

Per-PC profile `builds/p2p8-tail-profile-bc35`, the 10% of frames with the most
`battleship_ftMainSetStatus`, excess over the other half (ticks/frame): pose
parse 36K, memcpy 25K, pose play 23K, `ndsRelocP2FighterAnimAssetIDForToken`
21K, `ndsAObjEvent32ForgetRange` 17K, pose bind 13K, `_svfiprintf_r` +
`__utf8_mbtowc` 8.6K.

## Changes (identical results by construction)

1. **Token lookup**: the ~690-row animation token table was scanned per
   lookup (several per motion load). A numeric token can only equal an asset id
   and an address token only a symbol address, so two u16 index arrays sorted by
   (id, row) and (address, row) answer both with a binary search; the row key
   keeps the first row in table order first. Built once (shell sort), 2.8 KB.
2. **Animation paths**: the three motion path builders used `sniprintf`
   (`%s%03lu`) on every asset->path resolution. A small formatter writes the
   same bytes (and refuses the same overflow); each builder also returns its
   last answer when asked for the same id again.
3. **Event32 ledger ForgetRange**: every fighter motion load retires its
   figatree heap range and scanned the whole ledger (high-water 1,623) for
   commands inside it. The ledger now keeps entry counts per 4 KiB page of main
   RAM; a range whose pages hold none skips the scan (485 skips per match; an
   entry outside main RAM disables the skip).

## Result

`token` `2EDB9475` (1 + 2 only): WORK-H P95 -7.2K but P50 +5.2K and STG +4K
with unchanged stage code -- layout noise, not banked alone.

`forget` `077110CD` (1 + 2 + 3) against `nozlocal` `70D3B9DE` (route 1, frames
2..1973): WORK-H P50/P95/P99 1,302,080/1,808,576/2,225,408 -> 1,301,504/
1,794,240/2,197,184; SRC P95 946,688 -> 925,504. Replay digest IDENTICAL; event32
hash overflow 0. Evidence: `token-route1`, `forget-route1` (json/rows/log).

Not changed: pose parse/play and the figatree copy (A3/A4 pre-bound motions).

## 4. Pose clock adder in ARM state -- BANKED

`include/nds/nds_f32_exact.h` asks to be compiled in ARM state (CLZ is one
instruction there). `nds_ft_pose.c` is a Thumb TU, so the static inline could
not inline into the ARM-state `ndsFtPoseParse` and became an out-of-line Thumb
function in main RAM calling `__clzsi2`: 306 calls a frame at 105 cycles
(profile). The header is now included under `#pragma GCC target("arm")`; the
adder is ARM with native CLZ. Same arithmetic, same bits.

`f32arm` `A8E6AFD9` against `forget` `077110CD`: WORK-H P50/P95/P99
1,301,504/1,794,240/2,197,184 -> 1,293,056/1,782,784/2,189,632; SRC P50
588,352 -> 581,120; two-VBlank 311 -> 327. Pose oracle mismatches 0, 702
binds. Replay digest IDENTICAL. Evidence: `f32arm-route1` (json/rows/log).
