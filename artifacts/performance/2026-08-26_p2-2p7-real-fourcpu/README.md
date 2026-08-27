# P2-2p7 corrected four-CPU timing evidence

This directory banks the first four-CPU argmax measurement after narrowing the
`cpuGetTiming()` `2^22` correction to its proven VBlank-grid signature.

- ROM SHA-256: `8FAFCCB665B1D041A502CB332A4BEAF010D8DD44AEF1CDEE802DC7799B4388D9`.
- Full run: 1,972 samples, presented frames 2..1973, same-run clock 60 -> 1,
  four CPUs, four fighters, active mask `0xF`.
- `ALL` P50/P95: 6,719,552 / 8,400,128 ticks.
- `WORK-H` P50/P95: 6,313,728 / 7,906,176 ticks.
- VBlank histogram: 2=60, 3=3, 4=2, 5+=1908, max=25.
- General-heap low-water: 325,240 B against the 25,600 B floor; graphics-heap
  overflow and object-manager panic are zero.
- The corrected detector marks seven rows. The superseded threshold-only rule
  marked 1,824 of 1,972 rows on the previous four-CPU run.
- The 128-frame no-build sizing probe reads 60 native-stage owner rejects,
  first/last reason 6, abort=0, post-arm=0. Reason 6 is
  `ndsRendererPrepareNativeStageOwner`.
