# P2-2p8 N04.03 s32 split-modelview round-shift

Verdict: **KEEP** the exact 32-bit signed round-shift used by fighter packet
split-modelview translation. P2-2p8 remains RED against the product target.

The current profile charged `ndsFighterPacketStoreSplitModelview` 3,648,967
cycles. Its translation-only loop always calls `RoundShiftS32Signed` with shift
8, but the helper promoted every value through the generic signed 64-bit path.
The retained helper computes unsigned magnitude, adds the round bias and shifts
in 32 bits, then reapplies the sign. Shifts at or above 32 retain the old generic
fallback. A 2,400,022-value differential was bit-exact against the old helper,
including `INT_MIN` and `INT_MAX`.

The ARM9 codegen for `ndsFighterPacketStoreSplitModelview` now performs the hot
translation conversion with 32-bit xor/sub/add/lsr/conditional-rsb operations;
the loop no longer calls the 64-bit round-shift helper or a 64-bit runtime helper.

## Verification

Focused four-CPU verification on runner 12
(`builds/verify-p2p8-s32-roundshift-stress.log`) completed 1,972 samples and
passed correctness, cadence, native-owner and memory gates:

- WORK-H P50/P95: **1,649,728 / 2,378,560** ticks, mean **1,695,965**.
- N03.04 checkpoint: **1,654,720 / 2,377,472**, mean **1,704,369**. The
  cross-build movement is **-4,992 / +1,088** P50/P95 and **-8,404** mean.
- FTR P50/P95: **359,936 / 742,656**, mean **371,081**; versus N03.04 this is
  **-1,600 / -5,376** and **-2,008** mean.
- Native failures/direct rejects: **0 / 0**.
- General-heap low-water: **108,096 B**.
- Fighter draw-plan build/hit/mismatch: **618 / 6,217 / 0**.
- VBlank 2/3/4/5+: **103 / 819 / 837 / 214**, max interval **13**, slips **0**.

The frozen candidate then completed the full Boundary profile on runner 12
(`builds/verify-p2p8-s32-roundshift-boundary.log`). All three arms passed:

- `p2_shell_loop`: one lap, 10 scene entries/exits, zero faults/mismatches,
  native failures zero, free-floor **114,628 B**.
- `p2_battle_realtime`: 212 battle frames; presentation reports **25.8 FPS**
  (`present=258 x0.1 fps`). Published-ROM, same-ROM and visual/detail checks pass.
  The lab ROM SHA-256 is
  `F1379C32CBE6ED677FB76AAD960AA17C9A942547E4952E8D2B88C172B02B1517`.
- `p2_fourcpu_stress`: reproduces the focused WORK-H and FTR numbers exactly;
  native failures/direct rejects remain **0/0**, heap low-water **108,096 B**.

`Boundary verification profile passed.` The slight WORK-H P95 increase is not
assigned as a win; the retained local mechanism is supported by the fighter
bucket and lower WORK-H P50/mean. Locked 30 FPS remains unmet.

## Final hard-on build

After Boundary, `make TARGET=smash64ds` rebuilt the normal natural-input root
target and passed `NATIVE_ONLY_PASS: smash64ds.elf, 262 actual link inputs` plus
`check-published-roms.ps1`.

- `smash64ds.nds` — 55,712,768 bytes — SHA-256
  `AB38C207C6F2C0DC39EF4066A5F92548126B2DD8DA287E550446A612E39B58ED`
- `smash64ds.elf` — 16,257,420 bytes — SHA-256
  `5907F9C0B92620601109A0D3236A35C4336F2EA2BB94648C3B92C07227E53EB4`
- `builds/build/nds_build_config.h` — 11,560 bytes — SHA-256
  `BDFE59516B2F8DBAB0C7A01C60336772475D03D6FD6A968DCBC9F3CAF298F0E0`

Shipping config has fast logic off, hardware triangles on, renderer profile 0,
Task-36 HW compose mode 2 and fighter packets on. Raw verifier/build logs remain
under `builds/` and are not committed.
