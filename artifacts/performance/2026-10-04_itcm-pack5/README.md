# ITCM pack 5 (late-window census section D), 2026-10-04 -- removed

Fifteen small main-RAM functions ranked 3,000+ non-mem stall cycles a byte
by the late-window lab census (`artifacts/task37-census/sz-lateprof04`,
section D) were admitted by name to `.itcm` (the gate ROM had a few hundred
bytes free; the census's 1.1 KB came from the MISC-split build's layout, and
the first two attempts overflowed by 464 and 160 bytes).

| | WORK P50 | WORK P95 | > 1.12M | two-VBlank |
|---|---:|---:|---:|---:|
| `capture-cuts/gate-cc1` | 897,408 | 1,236,352 | 234 | 1,722 / 1,961 |
| pack 5 (`gate-p5`) | 896,256 | 1,237,952 | 234 | 1,723 / 1,961 |

Paired by frame: median -1,088, 1,323 of 1,958 frames better; P95 within
the single-run spread. Replay digest IDENTICAL. The ~1K did not justify
filling ITCM to 24 bytes free (every later placement would need evictions);
reverted.
