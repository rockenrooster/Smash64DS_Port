# P2-2p8 N04.04 packet 4x4 generic memcpy

Verdict: **REJECTED**. No Boundary run.

The candidate replaced only `ndsFighterPacketStoreMatrix4x4`'s exact 64-byte
scalar copy with `memcpy`. The linked `ndsFighterPacketTryReplay` shrank from
0xee4 to 0xe60 bytes, and each live 4x4 patch called the existing ITCM Thumb
`memcpy` at `0x01fff7f0`. The 4x3 and split-modelview paths were unchanged.

Focused four-CPU verification on runner 12
(`builds/verify-p2p8-matrix4x4-memcpy-stress.log`) passed correctness, cadence,
native-owner and memory gates across 1,972 samples, but regressed the targeted
fighter bucket against qualified N04.03:

- FTR P50/P95/mean: **362,496 / 746,112 / 373,602** versus
  **359,936 / 742,656 / 371,081**: **+2,560 / +3,456 / +2,521**.
- WORK-H P50/P95/mean: **1,648,896 / 2,370,176 / 1,695,477** versus
  **1,649,728 / 2,378,560 / 1,695,965**: **-832 / -8,384 / -488**.
- Native failures/direct rejects: **0 / 0**.
- General-heap low-water: **108,096 B**.
- Fighter draw-plan build/hit/mismatch: **618 / 6,217 / 0**.
- VBlank 2/3/4/5+: **104 / 836 / 819 / 214**, max interval **13**, slips **0**.

Because this candidate changes fighter packet copying, the FTR regression is the
local falsifier even though WORK-H moved slightly lower. The generic ITCM memcpy
still copies four words per 16-byte loop and adds call/setup work. The source
change was removed; N04.03 remains the qualified checkpoint.

Lab identity:

- ROM SHA-256 `422766A8E2F7FE30CF5F72891E31BBF4DB2D5F2BAB78EE950651163965A848EB`
- ELF SHA-256 `5492E9B55F33A0ADEDF43753764219C7D7D3F723E886D7E3D8727B276412FAA9`
- config SHA-256 `23314B62FBBECCAC413A9D677555EB1233137F84C26D5D9DF0177F801F5415F3`

Raw build/verifier logs remain under `builds/` and are not committed.
