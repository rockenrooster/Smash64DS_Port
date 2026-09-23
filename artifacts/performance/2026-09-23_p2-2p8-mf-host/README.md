# P2-2p8 Phase 3 prep: MF1 host encoder, runtime decoder, checker (2026-09-23)

The Phase 0 experiment chose candidate B (`../2026-09-23_p2-2p8-mf-experiment/`).
This productionises its host half so Phase 3 only has to wire the pack into the
producer and the match-load bank. Nothing here is linked into a ROM yet
(`src/nds/nds_motion_mf.c` is not in `CFILES`).

## Files

| file | role |
|---|---|
| `scripts/motion/mf_emit.py` | encoder: one global table set (cfg "fast"), every AObj16 clip of every kind -> `ftanim_mf_pack.bin` + manifest |
| `include/nds/nds_motion_mf.h` | the format contract: pack header / kind / entry structs, expanded table structs, API |
| `src/nds/nds_motion_mf.c` | runtime: `ndsMfTablesStorageBytes`, `ndsMfExpandTables`, `ndsMfDecodeClip` |
| `scripts/motion/host/mf_host.c` | ctypes harness: includes the runtime `.c` verbatim, adds nothing to the decode |
| `scripts/motion/check_mf_pack.py` | the proof (below) |
| `scripts/motion/mf_*.py` (other) | the Phase 0 experiment (corpus, parser, codecs, budget, cost model) |

## Pack layout (MFP1, little-endian; structs in `nds_motion_mf.h`)

```
header    NdsMfPackHeader, 48 B
kinds     NdsMfPackKind x 12 (name, dir_first, dir_count)
tables    MFT1 blob (expanded once at boot)
entries   NdsMfPackEntry x clips (asset id, data offset, stream bits, BPS1 size,
          CRC32, kind, class, need mask); each kind contiguous, ordered
          [always resident][victim clips by opponent][Kirby copies by opponent]
by_id     u16 x clips: entry indices sorted by asset id
data      one MSB-first bitstream per clip, 4-aligned, in entry order
```

A kind's always-resident span is contiguous, and so is each opponent group
after it, so the match loader reads one span per kind plus the groups the
roster needs. The tables blob format is documented in `mf_emit.py`'s header.

## Results

`MF_PACK_CHECK=PASS clips 1570/1570 byte-exact with the C decoder, tables 17
(blob 16928 B, expanded 16328 B), pack 1425752 B` (`check.json`).

| kind | clips | BPS1 B | MF B | ratio | largest clip |
|---|---:|---:|---:|---:|---:|
| mario | 141 | 339,560 | 122,544 | 0.361 | 6,224 |
| fox | 156 | 346,244 | 124,844 | 0.361 | 4,896 |
| donkey | 151 | 332,512 | 119,772 | 0.360 | 11,568 |
| samus | 146 | 294,576 | 105,672 | 0.359 | 6,640 |
| luigi | 12 | 35,520 | 12,888 | 0.363 | 6,720 |
| link | 142 | 310,912 | 111,368 | 0.358 | 6,304 |
| yoshi | 140 | 367,104 | 137,400 | 0.374 | 8,480 |
| captain | 148 | 414,976 | 149,840 | 0.361 | 10,784 |
| kirby | 186 | 392,768 | 136,252 | 0.347 | 7,808 |
| pikachu | 139 | 374,280 | 143,764 | 0.384 | 6,480 |
| purin | 63 | 176,088 | 61,852 | 0.351 | 8,240 |
| ness | 146 | 387,744 | 141,568 | 0.365 | 8,448 |
| **all** | **1,570** | **3,772,284** | **1,367,764** | **0.363** | |

Plus the tables: 16,928 B serialised (the experiment's bit-packed estimate was
12,864 B; this layout trades 4 KB for a decoder that reads the blob in place),
16,328 B expanded at boot.

**The checker is mutation-tested**: one flipped data byte fails exactly its clip
(`clip 0x1f9: bytes differ ... CRC32 mismatch`, 1569/1570); one flipped table
byte fails 1,442 clips with decoder errors and over/under-reads.

## What the checker proves

Oracle bytes come from the BPS1 producer (`assets/animation/ftanim_stream_pack.bin`)
and, for kinds that pack does not carry, from the producer's own normaliser over
the O2R files (`mf_corpus.build_corpus`, which first re-emits every packed clip
and requires identity). The decoder under test is the runtime C file compiled
for the host. Per clip: bytes == oracle, bytes written == directory size, bits
consumed == directory stream bits, CRC32 == directory, nothing written past the
size. Per table: Kraft sum exact, lengths <= 12, every LUT entry equal to the
canonical decode.

## ARM codegen (`arm-size.txt`)

`-O2 -marm -mcpu=arm946e-s`: `ndsMfDecodeClip` 2,464 B + `mfValue` 480 B
(hot path ~2.9 KB), table expansion 1,484 B (boot only). Thumb: 2,520 + 344 B.
The hot path is about twice the experiment's ~1.5 KB estimate; with ITCM full,
its placement is a Phase 3 decision (ITCM needs an eviction of this size; main
RAM ARM code pays I-cache misses on each bind). The bind cost must be measured
on target before the hot-set policy is sized.

## Open for Phase 3

1. Producer integration: call the emitter from `scripts/generate_battlepack_anim.py`
   (a generator edit: regenerate and re-pin), ship the pack in NitroFS, run the
   checker in the same make step.
2. Runtime: match-load bank (one span per kind + opponent groups), admission
   against the measured low-water and the 25,600 B floor, decode at bind into
   `figatree_heap` at `reloc_backend_assets.c:15087`, a CRC self-check in a lab
   build.
3. The 29 clips outside MF (AObj32 entry clips and Samus's spline rolls,
   `mf_corpus` "skipped") stay resident in their O2R form: settle which are
   gameplay-reachable.
4. Hot set: keep each kind's most-bound clips raw inside the headroom, sized
   from a per-clip bind profile (needs a counter in the ROM).
5. Memory: the shipping census (`../2026-09-23_p2-2p8-shipping-heap-census/`)
   shows the heaviest roster cannot even load today; the bank's ~460-600 KB comes
   after that deficit is closed.

## Reproduce

```
python scripts/motion/mf_emit.py --out artifacts/performance/2026-09-23_p2-2p8-mf-host/out
python scripts/motion/check_mf_pack.py artifacts/performance/2026-09-23_p2-2p8-mf-host/out/ftanim_mf_pack.bin --json check.json
```
