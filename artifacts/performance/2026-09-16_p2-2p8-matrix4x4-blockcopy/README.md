# P2-2p8 N04.05 packet 4x4 ARM block copy

Verdict: **KEEP**. P2-2p8 remains RED against the locked-30 product target.

The current ARM9 profile charged `ndsFighterPacketTryReplay` 6,959,771 cycles.
Its successful replay path repeatedly copied exact 64-byte 4x4 renderer matrices
into the packet FIFO-word buffer with scalar word loads/stores. N04.04 proved the
existing generic ITCM `memcpy` was the wrong mechanism: it added call/setup cost
and regressed the fighter bucket.

N04.05 keeps the exact copy local. Both objects are naturally word-aligned typed
storage and do not overlap, so the retained helper copies the fixed 64-byte payload
as four inline ARM `ldmia/stmia` pairs. The 4x3 and split-modelview serializers are
unchanged. Linked `ndsFighterPacketTryReplay` is **0xecc** bytes versus **0xee4**
at N04.03, with no copy helper call in the 4x4 sites.

## Verification

Focused four-CPU verification on runner 12
(`builds/verify-p2p8-matrix4x4-blockcopy-stress.log`) completed 1,972 samples:

- FTR P50/P95/mean: **357,248 / 743,616 / 368,779** ticks versus N04.03
  **359,936 / 742,656 / 371,081**: **-2,688 / +960 / -2,302**.
- WORK-H P50/P95/mean: **1,649,728 / 2,373,632 / 1,696,317** versus N04.03
  **1,649,728 / 2,378,560 / 1,695,965**: **0 / -4,928 / +352**.
- Native failures/direct rejects: **0 / 0**.
- General-heap low-water: **108,096 B**.
- Fighter draw-plan build/hit/mismatch: **618 / 6,217 / 0**.
- VBlank 2/3/4/5+: **106 / 824 / 820 / 223**, max interval **13**, slips **0**.

The frozen candidate then completed full Boundary on runner 12
(`builds/verify-p2p8-matrix4x4-blockcopy-boundary.log`):

- `p2_shell_loop`: one lap, 10 scene entries, zero faults, free-floor
  **114,628 B**, native failures zero.
- `p2_battle_realtime`: 212 frames, same-ROM/visual checks GREEN, presentation
  **25.7 FPS** (`present=257 x0.1 fps`). Same-ROM SHA-256:
  `62914C3A2AD020B6DE37B8C874D2701B1F3CA64F4748DEDA1F2DC6CEECB50338`.
- `p2_fourcpu_stress`: reproduces the focused FTR and WORK-H numbers exactly;
  native failures/direct rejects remain **0/0** and heap low-water **108,096 B**.

`Boundary verification profile passed.` The KEEP is the fighter P50/mean reduction
plus the WORK-H P95 reduction. The small FTR P95 and WORK-H mean increases are
recorded rather than treated as wins.

Boundary four-CPU lab identity: ROM
`FBEE4FC4EE9A2377B1E797B73010FBC7B38F526C34609200948C724EF74E53E6`, ELF
`A5C2ABFBD7BC37B8526A1CF8115ADD6888EDED6F8224A3A8EA95C5BBAA84AE04`, config
`23314B62FBBECCAC413A9D677555EB1233137F84C26D5D9DF0177F801F5415F3`.

## Final hard-on build

`make TARGET=smash64ds` rebuilt the normal natural-input root target and passed
`NATIVE_ONLY_PASS: smash64ds.elf, 262 actual link inputs` plus
`check-published-roms.ps1`.

- `smash64ds.nds` — 55,712,768 bytes — SHA-256
  `FE4C064BBF3DF2EE9FBB22F4A483AC13B502829779A23E7BACE01E6CE9AFDBB9`
- `smash64ds.elf` — 16,256,532 bytes — SHA-256
  `736BCBE6EE02901AA5E3252EAB8DF071C6C251F327331FD94B4F2D84CED60426`
- `builds/build/nds_build_config.h` — 11,560 bytes — SHA-256
  `BDFE59516B2F8DBAB0C7A01C60336772475D03D6FD6A968DCBC9F3CAF298F0E0`

Raw verifier/build logs remain under `builds/` and are not committed.
