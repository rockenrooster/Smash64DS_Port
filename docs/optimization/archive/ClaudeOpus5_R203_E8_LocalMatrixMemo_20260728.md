# R2-03 E8 — the local-matrix memo is refuted: the only correct key costs more than the build

**Date:** 2026-07-28
**Phase:** R2-03 (fighter direct draw)
**Verdict:** REFUTED. +16,301 ticks/frame on MatrixPrep. Code deleted.

## 1. The claim being tested

E7 measured 48% of `BuildDObjLocalMatrix` calls producing a matrix bit-identical
to the last one for that joint, and E6 priced a build at 1,061 ticks. Reusing the
previous matrix replays the exact bytes the builder produced, so unlike E6's
fixed-point lever this needed no numerical equivalence proof — only that the
memo key be complete and cheap.

It is complete. It is not cheap.

## 2. The verifier earned the whole experiment

The memo shipped with two levels: level 1 returns the cached matrix, **level 2
always rebuilds and compares**. Level 2 exists because the key had to be derived
from reading which fields the builder touches, and that reading is exactly the
thing not to trust — `GetDObjVectorTracks` can repoint translate/rotate/scale
into `dobj->vec->data`, and several cases read the DObj directly instead.

Level 2 was right to exist. Three successive keys, each verified over a full
match:

| key | mismatches/frame | hits/frame |
|---|---|---|
| resolved tracks only | 0.2 | 12.1 |
| + `dobj->` tracks, billboards (33..40) excluded | 0.2 | 17.0 |
| + fighter-parts kind excluded | **0.0** | **0.0** |
| + `FTParts` transform folded in | **0.0** | 13.6 |

Every one of those rows is a fact I would have got wrong by reading.

- **Billboards.** Kinds 33..40 build from `gGCCurrentCamera`. Their matrix is not
  a function of the DObj at all, so no DObj key can cover them.
- **Fighter parts.** `NDS_RENDERER_ADAPTER_FIGHTER_PARTS_MTX_KIND` builds from
  `ftGetParts(dobj)->unk_dobjtrans_0x10`, a `Mtx44f` living in the fighter parts
  struct. Excluding it drove mismatches to zero — and hits to zero with them,
  because **97.5% of all local-matrix builds are that kind**. The entire win was
  there or nowhere.

So the key must fold 16 floats of `FTParts` plus the mode flag, on top of the ten
DObj floats and the xobj kinds.

## 3. The measurement

Identical source, identical Task 91 probes, level 0 versus level 1, 128 presented
frames:

| | level 0 | level 1 | delta |
|---|---|---|---|
| `BuildDObjLocalMatrix` (`locT`) | 62,213 | 79,132 | **+16,919** |
| world build | 105,425 | 121,522 | +16,097 |
| **MatrixPrep** | **122,765** | **139,066** | **+16,301** |
| OwnerPrep | 143,839 | 160,464 | +16,625 |

Hit rate at level 1 was 13.6 per frame against 33.4 misses.

## 4. Why it loses

The arithmetic is not subtle. ~47 builds per frame each pay ~27 folds of key,
including a `ftGetParts` call and 64 bytes of `Mtx44f`; 13.6 of them then skip a
1,061-tick build. Gross saving ~14,400/frame, gross key cost ~31,000/frame.

**71% of calls pay the key and then build anyway.** That is the structural
problem: a memo whose key is nearly as expensive as its payload only wins at a
high hit rate, and 29% is not high.

It is also the Task 104 lesson from the other side. The key touches the same 64
bytes of `FTParts` that `syMatrixF2LFixedW` is about to convert, so on a miss
those bytes are walked **twice** — the memo adds a full extra pass over the exact
data the build already reads.

## 5. Why the hit rate is lower than E7 said

E7 hashed the **output** matrix and saw 48% unchanged. This keys on **inputs** and
sees 29%. The gap is real and expected: `syMatrixF2LFixedW` converts float to
fixed point, and quantization collapses input differences that the key can still
distinguish. The float inputs move; the fixed-point output does not.

That gap cannot be closed from the input side. Closing it means comparing the
output — which means building it — which is the thing being avoided.

## 6. What this leaves

The redundancy E7 measured is real and is not reachable by memoisation. The
remaining lever on this bucket is the one E6 named: **make the build cheaper**
rather than skip it — a fixed-point local-matrix builder emitting DS 20.12
directly, avoiding the float angle scaling and the N64 intermediate. That still
requires its own falsifier (E6 §6), and it now carries an extra piece of
evidence: 97.5% of builds go through the fighter-parts path, so
`syMatrixF2LFixedW` is the specific conversion worth attacking, not the RPY
rotation cases.

## 7. Disposition

Deleted. The flag, the table, the key builder and both verify levels are gone.
A refuted lever left behind a default-off flag is a future reader's dead end, and
`AGENTS.md` prefers deletion. The Task 91 census counters that measured it stay —
they are E6's instrument and still referee this bucket.
