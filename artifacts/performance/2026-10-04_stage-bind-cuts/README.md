# Stage binding bookkeeping cuts, 2026-10-04 -- no effect, removed

Three exact cuts in the stage owner's per-frame bookkeeping, from the late
lab profile (`sz-lateprof04`): the hidden-binding mask computed in one pass
over the topology (each binding read its DObj's entry instead of walking its
ancestor chain), each live binding's MVP-recalc kind cached against its XObj
count and first XObj (the scan read every XObj's kind each frame), and the
stage world source key captured with word copies instead of memset/memcpy.

| | WORK P50 | WORK P95 | > 1.12M | two-VBlank |
|---|---:|---:|---:|---:|
| `capture-cuts/gate-cc1` | 897,408 | 1,236,352 | 234 | 1,722 / 1,961 |
| stage binding cuts (`gate-sb1`) | 897,664 | 1,235,520 | 235 | 1,721 / 1,961 |

Paired by frame: median -64, 990 of 1,958 frames better (noise); STG median
+128. Replay digest IDENTICAL. The profile's few thousand cycles did not
survive into the gate ROM; the code was reverted.
