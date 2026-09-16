# P2-2p8 packet helper draw-hot placement

Verdict: **REJECT / REVERT**. The placement fit and preserved correctness, but
the focused whole-frame timing gate regressed. No full Boundary run was needed.

Starting point was qualified/pushed N03.04 commit `f2fee059af5`. The current
exclusive profile still charged three packet helpers in `.main`:

- `ndsFighterPacketBuildKey`: 1,981,867 measured cycles, 0x1cc bytes.
- `ndsFighterPacketPatchTexgen`: 2,077,638 cycles, 0x390 bytes.
- `ndsFighterPacketStoreSplitModelview`: 3,648,967 cycles, 0x98 bytes.

The experiment added only those exact input sections to the existing Task-32
`.text.hot.draw` generated placement list. Source bodies, compiler flags and ISA
were unchanged. The focused four-CPU map proved all three sections moved and the
group fit at **0x1d68 = 7,528 / 8,192 bytes**.

Focused `p2_fourcpu_stress` on runner 12 completed 1,972 samples and passed its
correctness/native/memory gates. Candidate WORK-H was **P50 1,659,904 / P95
2,384,640** versus the immediately preceding qualified N03.04 checkpoint
**1,654,720 / 2,377,472**: **+5,184 / +7,168** ticks. Native failures/direct
rejects remained **0/0**, heap low-water remained **108,096 B**, and draw-plan
build/hit/mismatch remained **618/6,217/0**.

Candidate identities:

- ROM `CAD24D74F920EC6475E957283EE64EBBAB1C03A8A12C3904103758EB7DD5E9C5`
- ELF `2986205333EB9672BA1FEA7701A912AAB7A6CBA238CBE08543CD26F88CF6E24C`
- config `23314B62FBBECCAC413A9D677555EB1233137F84C26D5D9DF0177F801F5415F3`
- raw focused log: `builds/verify-p2p8-packet-hot-placement-stress.log`

Because focused whole-frame timing loss was the recorded falsifier, the three
placement entries were removed immediately. The verification run's generated
particle outputs were restored to the qualified tree state. This result does not
invalidate Task-32's existing draw-hot placement; it rejects only adding these
three packet helpers to that group on this binary/layout.
