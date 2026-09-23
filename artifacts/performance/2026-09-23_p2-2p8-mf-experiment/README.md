# P2-2p8 Phase 0 — compact motion format (MF) host experiment (2026-09-23)

Spec: `docs/p2/FOUR_FIGHTER_30FPS_ARCHITECTURE.md` §A2 "Compact motion format (MF)" (lines 259-280).
Labels: **MEASURED** = output of a script in `scripts/motion/` (command named), **ESTIMATE** = arithmetic shown.
Host-only: no build, no emulator, no existing file modified.

STATUS: complete (2026-09-23). Each script reruns in about two minutes or less on the host; the
final numbers below were re-run with the scripts as saved in `scripts/motion/`.

## 0. Lossless, defined

The consumer reads BPS1 clip bytes in place. `ndsRelocForceLoadFighterAObj16File`
(`src/port/reloc_backend_assets.c:15086-15110`) reads one clip into the fighter's existing
`figatree_heap` and registers it; `ndsFtPoseBindEntry` stores a script pointer into those bytes
(`src/nds/nds_ft_pose.c:454`) and `ndsFtPoseParse` walks it lazily, one command per event, across
ticks (`nds_ft_pose.c:709-980`, `pc` persisted in `dobj->anim_joint.event16`), including backward
`Loop` jumps (`:903-906`). So the decoded clip must stay resident for the whole bind and must be the
exact byte image the parser reads.

**Definition used here: decode(MF clip) == BPS1 clip bytes, byte for byte, for every clip.** That is
stricter than "bit-identical pose" (it also reproduces the never-read 4-byte alignment padding
between runs) and it lets the pose engine, `lbCommonAddFighterPartsFigatree` and the loaded-file
registration run unchanged.

## 1. Corpus (MEASURED, `python scripts/motion/mf_corpus.py`)

- BPS1 pack parsed and checked (magic/version/size, dense directory rows, 16 B clip alignment, no
  span overlap): **1,139 clips, 2,630,496 B blob, dense ids 0x1F3-0x67F**.
- The BPS1 per-clip layout was re-implemented (`mf_corpus.bps1_clip_bytes`, a transcription of
  `generate_battlepack_anim.py:547-583`) and fed by the producer's own normaliser
  (`read_clip`, imported unmodified). It reproduces **1,139 / 1,139 packed clips byte for byte**, so
  its output for the O2R-only files is the BPS1 image those clips would have.
- O2R-only clips normalised the same way: Yoshi 140, Pikachu 139, Ness 146, Purin-own 6 (the other
  57 Purin clips, `FTKirbyCopyAnim000-056` = 0x5A5-0x5DD, are already in the pack). Excluded, as the
  producer excludes them: 27 AObj32 entry/effect clips and Samus's 2 spline clips (0x3F5/0x3F6).
- Structural parse (`scripts/motion/mf_aobj16.py`: slot table + runs + commands + tail) round-trips
  **1,570 / 1,570 clips** byte for byte.

Per-bank BPS1 bytes (a "bank" = the clips whose files belong to that kind):

| bank | clips | BPS1 bytes | largest clip | source |
|---|---:|---:|---:|---|
| mario | 141 | 339,560 | 6,224 | pack |
| fox | 156 | 346,244 | 4,896 | pack |
| donkey | 151 | 332,512 | 11,568 | pack |
| samus | 146 | 294,576 | 6,640 | pack |
| luigi | 12 | 35,520 | 6,720 | pack |
| link | 142 | 310,912 | 6,304 | pack |
| yoshi | 140 | 367,104 | 8,480 | O2R → BPS1 (O2R files 378,864) |
| captain | 148 | 414,976 | 10,784 | pack |
| kirby | 186 | 392,768 | 7,808 | pack |
| pikachu | 139 | 374,280 | 6,480 | O2R → BPS1 (O2R files 385,568) |
| purin | 63 | 176,088 | 8,240 | pack 57 + O2R 6 |
| ness | 146 | 387,744 | 8,448 | O2R → BPS1 (O2R files 399,552) |
| **all** | **1,570** | **3,772,284** | | |

Pikachu/Yoshi/Ness BPS1-equivalent is 0.969-0.971 of the O2R bytes of the same AObj16 files
(MEASURED); Q2's ~390/381/369 KB estimates (E, ×0.935 of all O2R files incl. AObj32) become
**387,744 / 374,280 / 367,104 B** (MEASURED).

Cross-check with Q2 (`…/2026-09-22_p2-2p8-architecture-baseline/INVESTIGATION_RESIDENCY.md:52-64`):
per-kind bank totals match to the byte for all seven packed kinds; per-class sums (items, taunt,
pipes, victim-only, Kirby copy) match for DK/Samus/Link/Kirby/Captain once the one main-table-
unreferenced clip per kind is counted (e.g. Captain `FalconDiveEnd2`, Samus `BombAir`). The stress
roster Dream Land tight set is **1,221,912 B** (DK 316,688, Samus 283,748, Link 301,008, Kirby
320,468): three kinds match Q2 exactly; Link is +1,000 B vs Q2's 300,008 although every Link class
total matches Q2, so the difference sits in Q2's Link subtraction (ThrownDK 1,360 + ThrownDKPulled
2,048 are kept because DK is present).

What the bytes are (MEASURED over all 1,570 clips, `mf_aobj16` walk): values (s16 targets/rates)
2,050,220 B (54.3%), command words 840,014 (22.3%), payload frame counts 680,320 (18.0%), slot
tables 159,644 (4.2%), after-terminator tails 33,614 (0.9%), Loop/TraI offsets 8,472. Commands:
SetValRateBlock 205,734 (2.15 MB of bytes), SetValRate 88,000, Block 34,425, End 25,125, ...

## 2. Baselines (MEASURED, host zlib/lzma over the §1 clip images)

| kind | deflate per clip | deflate solid (whole kind) | LZMA solid | whole-run duplicates across clips |
|---|---:|---:|---:|---:|
| captain | 0.641 | 0.564 | 0.467 | 1.7% |
| kirby | 0.627 | 0.525 | 0.435 | 2.5% |
| ness | 0.639 | 0.566 | 0.470 | 0.9% |
| donkey | 0.649 | 0.542 | 0.439 | 5.0% |
| all 12 banks | 0.639 | 0.552 | 0.448 | 2.6% |

Q2's figures reproduce (deflate 0.64, whole-kind LZMA 0.45). Two facts shape everything below:

1. **The cross-clip redundancy is not repeated scripts.** Only 2.6% of bytes are whole runs
   (per-joint scripts) that also occur in another clip of the kind. What LZMA finds across clips
   is *statistical*: the same command words, small frame counts, narrow value deltas.
2. **A trained per-kind dictionary does not rescue byte-LZ.** deflate with a COVER-style trained
   dictionary (zstd's algorithm, k=256, d=8) resident once per kind, dictionary bytes included:

| kind | 8 KB dict | 16 KB dict | 32 KB dict |
|---|---:|---:|---:|
| captain | 0.614 | 0.614 | 0.627 |
| kirby | 0.587 | 0.587 | 0.599 |
| ness | 0.617 | 0.618 | 0.633 |
| donkey | 0.603 | 0.602 | 0.614 |

   Deflate adds Huffman on top of LZ; an LZ4-class byte-token format can only be worse (§3).

## 3. Candidate A — per-kind shared dictionary + LZ4-class tokens (BUILT, MEASURED)

`python scripts/motion/mf_candidate_a.py --dicts 8,16,32 --unit 1 --json candidate_a.json`

Format: LZ4 token byte (literal-count nibble, match-length nibble, 255-run extensions), literals,
u16 back-offset; a distance past the clip start continues into the kind's resident dictionary (a
virtual prefix), so each clip decodes alone against it. Dictionary: zstd-COVER greedy segment
selection (k=256, d=8) over that kind's clips, most valuable segment nearest the data. Parse:
lazy-greedy over hash chains (depth 64). **Round trip byte-exact: 1,570 / 1,570 clips at every
dictionary size** (the script decodes each clip with the reference decoder and compares).

Ratio **including the dictionary** (compressed clips + dictionary) / BPS1 bytes:

| kind | no dict | 8 KB | 16 KB | 32 KB | clip median / P95 / max @32 KB (B) |
|---|---:|---:|---:|---:|---|
| mario | 0.827 | 0.708 | 0.690 | 0.683 | 1,399 / 2,642 / 4,190 |
| fox | 0.825 | 0.706 | 0.691 | 0.687 | 1,351 / 2,581 / 3,436 |
| donkey | 0.837 | 0.712 | 0.694 | 0.683 | 1,202 / 2,543 / 5,528 |
| samus | 0.826 | 0.721 | 0.709 | 0.707 | 1,053 / 2,568 / 4,680 |
| link | 0.823 | 0.725 | 0.709 | 0.703 | 1,199 / 2,563 / 4,447 |
| yoshi | 0.819 | 0.729 | 0.711 | 0.702 | 1,518 / 3,115 / 5,482 |
| captain | 0.825 | 0.731 | 0.714 | 0.706 | 1,540 / 3,372 / 6,422 |
| kirby | 0.799 | 0.681 | 0.663 | 0.656 | 1,132 / 2,404 / 5,160 |
| pikachu | 0.831 | 0.733 | 0.716 | 0.708 | 1,528 / 3,615 / 4,741 |
| ness | 0.823 | 0.731 | 0.713 | 0.707 | 1,478 / 3,332 / 5,275 |
| **all 12 banks** | **0.822** | **0.716** | **0.701** | **0.697** | |

(Luigi's and Purin's own banks are 12 and 63 clips; a dictionary trained on so few clips simply
memorises them, so their rows are not meaningful and are omitted; they play Mario's and Kirby's
clips anyway.) The halfword-granular variant (`--unit 2`) is worse (Captain 0.768 vs 0.714 at 16 KB).
Excluding the dictionary the 32 KB clips alone are 0.593, i.e. the dictionary itself is a tenth of
the kind. **Candidate A cannot reach 0.45**: best 0.656 (Kirby), typical 0.70.

## 4. Candidate B — structural re-encoding + static Huffman (BUILT, MEASURED)

`python scripts/motion/mf_codec_b.py --scope global|kind --cfg fast|rich|lean --rate slope|zero`

The clip is rewritten as the symbols the pose parser consumes and decoded back to the exact BPS1
image. Per clip, one MSB-first bitstream, symbols in decode order:

- `NSLOT`, then per slot `SLOT` = NULL / next new run / back-reference d (the slot table is
  rebuilt from run offsets the decoder computes);
- per run: `CMD` = rank of the command word in the **previous word's successor list** (one shared
  rank table; escape to an order-0 word index), `JUMP` raw 16 bits for Loop/TraI, `PAY` (frame
  count; context: Block-type op or not), then per selected track `VD` = value minus the track's
  previous value in this run (context: track group rot/TraI/tra/scale × first-key-of-track) and, for
  SetValRate(Block), `RT` = rate minus a slope prediction `(VD * RECIP15[payload]) >> 15`
  (RECIP15[p] = round(32768 × 0.625 / p), exact integer arithmetic, ARM `MUL` + `ASR`);
  SetTargetRate `RT6` = rate minus previous rate; then `TAIL` = bytes after End/Loop (none /
  2m zero bytes / m raw halfwords).
- Each class/context is a length-limited (12-bit) canonical Huffman table over **frequent literal
  values + 17 JPEG-style magnitude categories** (category code, then `cat` raw bits).
- "fast" = 17 tables in all. Tables are trained at build time on the clips they will encode (closed
  world, as candidate A's dictionary is); their serialised size is charged in every ratio.

**Round trip byte-exact: 1,570 / 1,570 clips in every configuration run** (decoder writes bytes
from the bitstream; compared against the §1 BPS1 images).

| configuration | tables | ratio incl. tables, all 3,772,284 B |
|---|---|---:|
| fast, one global table set (12,864 B, 17 tables) | global | **0.366** |
| fast, per-kind tables (0.8-4.4 KB each) | per kind | 0.367 |
| rich (per-prev-word command tables, VD per op) | per kind | 0.365 |
| lean | per kind | 0.374 |
| rich without the rate predictor (`--rate zero`) | per kind | 0.372 |

Per kind, **fast / global** (clip bytes padded to 4 B; the 12,864 B table set is shared by all
kinds and charged once in §6):

| kind | BPS1 bytes | B fast, per-kind tables incl. | B fast, global (clips only) | clip median / P95 / max (B) |
|---|---:|---:|---:|---|
| mario | 339,560 | 0.368 | 0.361 | 844 / 1,456 / 2,168 |
| fox | 346,244 | 0.365 | 0.361 | 806 / 1,408 / 1,948 |
| donkey | 332,512 | 0.362 | 0.360 | 736 / 1,384 / 3,672 |
| samus | 294,576 | 0.365 | 0.359 | 708 / 1,292 / 2,360 |
| link | 310,912 | 0.362 | 0.358 | 756 / 1,356 / 2,196 |
| yoshi | 367,104 | 0.381 | 0.374 | 934 / 1,840 / 3,672 |
| captain | 414,976 | 0.366 | 0.361 | 904 / 2,136 / 3,360 |
| kirby | 392,768 | 0.349 | 0.347 | 688 / 1,344 / 2,648 |
| pikachu | 374,280 | 0.384 | 0.384 | 924 / 2,044 / 2,588 |
| ness | 387,744 | 0.371 | 0.365 | 908 / 1,824 / 2,832 |
| purin (own 63) | 176,088 | 0.361 | 0.351 | 928 / 1,804 / 2,588 |
| luigi (own 12) | 35,520 | 0.384 | 0.363 | 974 / 1,424 / 2,780 |

Global "fast" model (MEASURED): 1,248 distinct command words, 203 previous-word contexts with a
successor list (4,241 entries), 1,778 literal values across the 17 tables.

Why it works where LZ does not (MEASURED, empirical order-0 entropies over Captain's clips): a
value delta costs 8.3-8.7 bits against 16 raw, a rate 7.6 bits after the slope prediction (8.0
without), a command word 2.2 bits given the previous word (vs 16), a frame count 2.6 bits.
Whole-kind LZMA (0.448) only approximates this with byte contexts; the structural model gets 0.366
with no cross-clip state at all, so every clip stays independently decodable.

What was not needed: cross-clip dedup of identical per-joint runs (§2: 2.6% of raw bytes, which B
already codes at ~0.37, so dedup would save < 1%); keys "the interpolation reproduces exactly"
(dropping them changes the command stream, so it is not byte-exact and would need the pose oracle
as its only proof; the budget closes without it).

### 4b. Candidate C — B's structural split followed by A (MEASURED, ratio only, not built as a codec)

B's symbol streams serialised per clip as u16 (and as separate low/high byte planes), then candidate
A's LZ4-class coder with a 32 KB trained dictionary, dictionary included:

| kind | u16 streams + LZ4/32K | byte planes + LZ4/32K | byte planes + deflate/32K |
|---|---:|---:|---:|
| captain | 0.610 | 0.529 | 0.510 |
| kirby | 0.584 | 0.536 | 0.515 |
| pikachu | 0.622 | 0.547 | 0.531 |

The structural split helps LZ (0.70 → 0.53) but an LZ-class decoder still misses 0.45: the
remaining redundancy is in skewed value distributions, which only an entropy coder removes.

## 5. Decode cost on ARM9 (candidate B "fast", global tables) — ESTIMATE from MEASURED symbol counts

`python scripts/motion/mf_cost_model.py --lut-bits 6,8 --json cost_model.json`

Decode happens at bind, into the fighter's existing `figatree_heap` (§NEXT STEP), with the decoder
in ITCM (ARM mode) and the compressed clip in main RAM. The model multiplies **measured** per-clip
counts (symbols per class, each symbol's code length and escape width, bits consumed — logged while
encoding every clip) by per-operation cycle costs taken from the listing below, and adds memory
stalls from a **measured access trace**: the encoder logs every table lookup the decoder will make
(table + bit position, hence the exact LUT entry) and every successor/word-list read, and counts the
distinct 32 B lines per bind × 46 cycles (23 bus cycles per main-RAM line refill). 1 tick = 2 cycles.

Decoder sketch (C, the shape the cost model prices; not compiled here):

```c
/* LUT entry (u32): [31:16] literal value or escape category, [4] ESC, [3:0] code length (0 = long) */
typedef struct { const u32 *lut; const u32 *limit; const u16 *base; const u32 *sym; } MfTab;
typedef struct { u32 bits; s32 avail; const u16 *src; } MfBr;   /* MSB-aligned window */

#define MF_P 6
static inline void mfFill(MfBr *b)            /* keeps >= 16 valid bits */
{ if (b->avail <= 16) { b->bits |= (u32)*b->src++ << (16 - b->avail); b->avail += 16; } }

static inline s32 mfGet(MfBr *b, const MfTab *t)
{
    u32 e = t->lut[b->bits >> (32 - MF_P)], len = e & 15;
    if (__builtin_expect(len == 0, 0)) { e = mfGetLong(b, t); len = e & 15; }  /* canonical limits */
    b->bits <<= len; b->avail -= len; mfFill(b);
    if (!(e & 0x10)) return (s32)e >> 16;                 /* literal */
    u32 k = e >> 16; if (k == 0) return 0;                /* JPEG category: k raw bits */
    u32 x = b->bits >> (32 - k); b->bits <<= k; b->avail -= k; mfFill(b);
    return (x >> (k - 1)) ? (s32)x : (s32)x - (1 << k) + 1;
}

u32 ndsMfDecodeClip(const MfClipRow *row, u16 *out)       /* -> bytes written (== BPS1 size) */
{
    MfBr b; mfOpen(&b, row);
    u32 nslot = mfGet(&b, &T[NSLOT]), nrun = mfSlots(&b, nslot, slot_run);  /* NULL/next/back */
    u16 *o = out + 2 * nslot;
    for (u32 r = 0; r < nrun; r++) {
        s16 lastv[10] = {0}, lastr[10] = {0}; u32 seen = 0, prev = MF_RUN_START;
        run_off[r] = (u8 *)o - (u8 *)out;
        for (;;) {
            s32 k = succ_len[prev] ? mfGet(&b, &T[CRANK]) : -1;
            u32 wi = (k >= 0) ? succ[succ_off[prev] + k] : mfGet(&b, &T[CRANK0]);
            u32 w = words[wi], op = w & 31, flags = (w >> 5) & 0x3ff;
            *o++ = w; prev = wi;
            if (op == END) break;
            if (op == LOOP || op == TRAI) { *o++ = mfRaw16(&b); if (op == LOOP) break; continue; }
            u32 p = 0;
            if (w & 0x8000) *o++ = p = mfGet(&b, &T[PAY0 + IS_BLOCK(op)]) & 0xffff;
            if (op == SETTARGETRATE) { for (t...) *o++ = lastr[t] += mfGet(&b, &T[RT6]); continue; }
            if (!PER[op]) continue;
            s32 rq = (p <= 255) ? RECIP15[p] : 0;
            for (u32 t = 0; flags; t++, flags >>= 1) {
                if (!(flags & 1)) continue;
                s32 d = mfGet(&b, &T[VD0 + 2 * TG[t] + !((seen >> t) & 1)]);
                *o++ = lastv[t] = (s16)(lastv[t] + d); seen |= 1u << t;
                if (PER[op] == 2) *o++ = lastr[t] = (s16)(((d * rq) >> 15) + mfGet(&b, &T[RT0 + TG[t]]));
                else lastr[t] = 0;
            }
        }
        o = mfTail(&b, o);                                   /* none / zero pad / raw halfwords */
    }
    for (u32 i = 0; i < nslot; i++) ((u32 *)out)[i] = (slot_run[i] < 0) ? 0 : run_off[slot_run[i]];
    return (u8 *)o - (u8 *)out;
}
```

Hot path as ARM946E-S would run it (literal, code ≤ P bits), the basis of `C_SYM`:
`mov r0,bits,lsr#26` 1 · `ldr r1,[lut,r0,lsl#2]` 1+1 interlock · `ands r2,r1,#15` 1 · `beq long` 1 ·
`mov bits,bits,lsl r2` 2 (register shift) · `sub avail,avail,r2` 1 · `tst r1,#16` 1 · `bne esc` 1 ·
`mov r0,r1,asr#16` 1 · `cmp avail,#16` 1 → **12 cycles**, plus a 10-cycle refill per 16 bits
consumed. Constants (all ESTIMATE): symbol 12; refill 10 per 16 bits; long code +9 + 3 per bit
beyond P; escape +16; glue per symbol class CMD 25 (successor + word LDRH, STRH, dispatch, flags),
VD 15, RT 9 (`MUL`+`ASR` prediction, STRH ×2), PAY 5, RT6 8, SLOT 11, TAIL 30 (incl. per-run state
clear), JUMP/RAW 10; 250 per bind. Model inputs (MEASURED): mean clip 2,403 B raw, 870 B MF,
1,184 symbols; 223 distinct table lines touched per bind at P=6 (P95 299, max 373).

| per bind (ticks) | P=6 LUTs, tables in DTCM | P=6, tables cold in main RAM | P=8 LUTs, DTCM | P=8, cold |
|---|---:|---:|---:|---:|
| instruction cycles per output byte | 18.5 | 18.5 | 17.4 | 17.4 |
| mean clip (2,403 B; all 1,570 clips) | **22,851** | **27,985** | 21,605 | 29,473 |
| mean clip, pack clips only (1,139) | 21,763 | 26,808 | 20,591 | 28,336 |
| normalised to a 2.2 KB clip | **20,858** | **26,449** | 19,728 | 28,222 |
| P95 clip (4,512 B) | 43,801 | 50,390 | 41,440 | 51,666 |
| largest clip (DK 0x3B2, 11,568 B) | 103,338 | 108,054 | 98,399 | 105,782 |
| decode-table RAM (global, expanded) | 26,812 B | 26,812 B | 39,868 B | 39,868 B |

(Table RAM at P=6: LUTs 4,352 + canonical limits 1,326 + long-code symbol arrays 7,660 + successor
lists 10,978 + word list 2,496 B. The 12,864 B serialised tables are only read to build these.)

For comparison (ESTIMATE, arithmetic on MEASURED token counts): candidate A at 32 KB averages 222
sequences per clip (10.9 output bytes per sequence, 33% literals; Captain/Kirby/Pikachu/DK, 624
clips). At ~3 cycles per copied byte, ~20 per sequence and its stream + dictionary misses:
2,403×3 + 222×20 + (46+50)×46 ≈ 16.1K cycles ≈ **8.0K ticks** per mean clip (~7.3K per 2.2 KB),
matching the §A2 "LZ-class ~5-11K" guess.
**B's bind costs ~3× an LZ-class bind (20.9-26.4K vs ~7.3K per 2.2 KB); that is the price of reaching
the budget.**

A hand-specialised inner loop (ESTIMATE): the dominant command shape, SetValRateBlock over the three
rotations (16 output bytes, 8 symbols), costs 8×15 (decode + refill) + 15 (command) + 5 (payload)
+ 3×6 (value) + 3×6 (rate) + ~10 (escapes) ≈ 186 cycles = 11.6 cycles/B, i.e. ~13K ticks per 2.2 KB
with DTCM LUTs; the straightforward-C figure above is the safer planning number.

## 6. Budget (MEASURED sets and sizes)

`python scripts/motion/mf_candidate_a.py --dicts 32 --unit 1 --clips-json a_clips.json` then
`python scripts/motion/mf_budget.py --a-clips a_clips.json --json budget.json`

Resident set per fighter = its main motion table + the one unreferenced own-bank clip, with the
worst-case switches: pipes ON (Mushroom Kingdom), items ON, taunt ON; victim-only clips and Kirby
copy clips **only for opponents present**; clips shared across kinds (Luigi ← Mario, Purin ← Kirby)
counted once. All C(12,4) = 495 rosters of four distinct shipping kinds were evaluated. "B" = fast,
global tables, 4 B-aligned clip streams + 8 B directory row per clip + the 12,864 B serialised
tables. "B resident" swaps the serialised tables for the expanded decode tables (26,812 B at P=6,
§5) and adds the gameplay clips MF does not cover, kept in O2R form (Captain BlueFalcon1/2 AObj32
5,264 B; Kirby DKStaringGround/Air AObj32 5,920 B; Samus RollF/RollB spline 5,456 B; O2R payload
sizes from `nds_fighter_production.generated.h:653-654, 730-731, 1637-1638`).

| roster | clips | BPS1 (today) | A: LZ4 + 32 KB dict/bank | B fast, global | **B resident** | vs ~710,000 B |
|---|---:|---:|---:|---:|---:|---|
| **worst four: Yoshi + Captain + Pikachu + Ness** | 559 | 1,516,776 | 1,075,815 (0.709) | 579,904 (0.382) | **599,116 (0.395)** | fits, 110,884 spare |
| DK + Captain + Pikachu + Ness (2nd) | 572 | 1,492,872 | 1,052,674 (0.705) | 566,392 (0.379) | 585,604 (0.392) | fits |
| Fox + Captain + Pikachu + Ness (3rd) | 571 | 1,492,204 | 1,054,387 (0.707) | 566,352 (0.380) | 585,564 (0.392) | fits |
| §A2's "worst four" as whole banks: Captain + Kirby + Ness + Pikachu, every clip | 619 | 1,569,768 | n/a | 589,240 (0.375) | 614,372 (0.391) | fits |
| stress roster DK/Samus/Link/Kirby, worst stage | 590 | 1,244,308 | 864,065 (0.694) | 460,328 (0.370) | 485,652 (0.390) | fits |
| stress roster, Dream Land (Q2's tight set) | 578 | 1,221,912 | 851,404 (0.697) | 452,788 (0.371) | 478,112 (0.391) | fits |

- **No roster of the 495 exceeds 710,000 B with B** (MEASURED); A exceeds it on every roster
  above, by 0.14-0.37 MB. Per-kind tables instead of global: worst four 573,776 B (0.378) before
  expansion, but four table sets to expand at run time; global is the better runtime trade.
- With exact rules, Kirby's worst set is only 339,000 B raw (three copies, not 31), so the true
  worst four is Yoshi/Captain/Pikachu/Ness, 1,516,776 B, not §A2's ~1.58 MB estimate (which counted
  every Kirby copy clip). Both fit.
- The 25,600 B GObj floor is a runtime admission fact (low-water with the bank resident); it cannot
  be measured on the host. The table above is the bank's own size.

Per-fighter decode buffer (MEASURED largest resident decoded clip per kind, worst opponents):
Mario 6,224 · Fox 4,896 · DK 11,568 · Samus 6,640 · Luigi 6,720 · Link 6,304 · Yoshi 8,480 ·
Captain 10,784 · Kirby 7,808 · Pikachu 6,480 · Purin 8,240 · Ness 8,448 B. Every one is at most the
kind's existing `figatree_heap` (sized by `FTData.file_anim_size` = the largest O2R payload,
`src/import/battleship_ftmanager.c:127-143`; BPS1 ≤ payload is enforced at emit,
`scripts/generate_battlepack_anim.py:579-582`), so **no new buffer is needed**.

## 7. Recommendation and the byte-exact checker

**Build candidate B "fast" with one global table set ("MF1")**: expected ratio **0.366** over all
3.77 MB, **0.382** on the worst roster (0.395 with decode tables and the non-AObj16 extras), bind
cost ~21-26K ticks per 2.2 KB clip as straightforward C (§5), ~13K hand-tuned. Candidates A and C
are rejected on ratio (0.70 and ~0.53); nothing LZ-class reaches 0.45 on this corpus.

Checker (one command, run by the build over every clip of every kind; any mismatch fails the build):

1. **Oracle bytes are produced independently of MF**: every clip's BPS1 image is rebuilt from the
   O2R bank by the producer's own normaliser (`read_clip`) and per-clip layout, and must equal the
   BPS1 pack byte for byte (today: 1,139 / 1,139; the O2R-only kinds get the same image). The MF
   encoder never touches this path.
2. **The decoder under test is the runtime decoder**: `ndsMfDecodeClip` (the same C file the
   ARM9 builds) compiled for the host and driven over the MF pack exactly as the runtime reads it
   (directory row → stream → output buffer sized to the kind's `file_anim_size`). Not the Python
   encoder's own decode (an oracle sharing a decoder proves nothing).
3. Per clip assert: decoded bytes == oracle bytes; bytes written == directory size ≤
   `file_anim_size`; bits consumed == stream bits (no over-read beyond the 4 B pad); every
   table's Kraft sum == 1 and max length ≤ 12; every LUT entry agrees with the canonical decode.
4. Emit a manifest (clip id, BPS1 SHA-256, MF bytes, decoded size) and store a CRC32 per clip in
   the MF directory so a lab build can decode the whole bank once at match load and compare on
   target (catches ARM codegen or bank-relocation faults the host run cannot).
5. The pose oracle (`NDS_FT_POSE_ORACLE`) stays as the semantic proof on target; it is no longer
   the only one.

## Files

Scripts (`scripts/motion/`, host Python 3, no dependencies beyond the standard library):
`mf_corpus.py` (BPS1 pack reader, O2R → BPS1 re-emitter proven against the pack, clip → kind /
motion-class mapping, parsed-corpus cache in the system temp dir), `mf_aobj16.py` (exact structural
parse/serialise of a clip), `mf_candidate_a.py` (dictionary LZ4-class codec + COVER trainer +
round trip), `mf_codec_b.py` (structural static-Huffman codec + round trip), `mf_budget.py`
(495-roster budget), `mf_cost_model.py` (ARM9 bind-cost model + access trace).

Outputs here: `corpus.json` (per-bank/per-kind/per-class bytes), `candidate_a.json`,
`candidate_b_fast_global.json`, `candidate_b_fast_kind.json`, `candidate_b_rich_kind.json`,
`budget.json`, `cost_model.json`.

## VERDICT

**Yes: candidate B meets the ~0.45x worst-case budget; A and C do not.** MEASURED on the exact
worst roster of all 495 four-kind rosters (Yoshi + Captain + Pikachu + Ness, 1,516,776 B of BPS1):
B = 579,904 B (**0.382x**), 599,116 B (0.395x) with expanded decode tables and the O2R-form
non-AObj16 clips, **~111 KB under 710,000 B**; lossless by the strictest definition (the exact BPS1
bytes) on **1,570 / 1,570 clips**; random access per clip (independent bitstreams, static global
tables, no stream-wide state). Candidate A (per-kind 8/16/32 KB dictionary + LZ4-class tokens) is
0.66-0.73x per kind and 0.709x on the worst roster; B-then-A (candidate C) reaches only ~0.53x.

## BIND COST

ESTIMATE (§5; instruction model x MEASURED symbol counts, memory stalls from a MEASURED access
trace): candidate B **~22.9K ticks per average clip (2,403 B) with the LUTs in DTCM, ~28.0K with
tables cold in main RAM; ~20.9K / ~26.4K per 2.2 KB; P95 clip ~44-50K; largest clip (DK 11,568 B)
~103-108K ticks.** That is ~3x an LZ-class bind (~7.3K per 2.2 KB, §5) and roughly 2-5x §A2's
5-11K planning range. A hand-specialised loop is ~13K per 2.2 KB (ESTIMATE). Averaged over the canon
stress match (681 acquisitions in 1,972 presented frames, Q2) that is ~681 × 25K / 1,972 ≈ 8.6K
ticks per presented frame (ESTIMATE); it is one bounded, RAM-only operation per status change
instead of a FAT walk + DLDI read, but on frames where several fighters bind at once it is a real
P99 cost against SPRM's 40K P99+ target (architecture doc §2). Not a STOP: a risk for the owner,
with the mitigation in NEXT STEP 5.

## NEXT STEP FOR THE RUNTIME

1. **Encoder in the pack producer.** Productionise `scripts/motion/mf_codec_b.py` (cfg "fast",
   global scope) as an emitter called by `scripts/generate_battlepack_anim.py` after
   `emit_stream_pack`, also covering Pikachu/Yoshi/Ness/Purin-own through `read_clip` (§1). Output
   `animation/ftanim_mf_pack.bin`: header, the serialised global tables (12,864 B), a dense
   directory (id → stream offset, decoded size, CRC32), then per-kind contiguous segments ordered
   [always-resident][victim clips by opponent][Kirby copies by opponent] so the match loader reads
   one span per kind plus the groups it needs. Plus the checker of §7, run in the same make step.
2. **Tables** (B has no per-kind dictionary): expand the global tables once at boot into ~26.8 KB;
   the P=6 LUTs + canonical limits (5,678 B) go in **DTCM** if the space exists (else main RAM:
   +~5K ticks per bind), successor/word/long-code arrays in main RAM. Roster-independent.
3. **Decoder** `ndsMfDecodeClip` in **ITCM**, ARM mode (~1.5 KB), writing into the fighter's
   existing **`figatree_heap`**: per-fighter decode buffer = `FTData.file_anim_size`, unchanged
   (largest decoded clip 11,568 B, DK). Replace the ROM read at
   `src/port/reloc_backend_assets.c:15087` (`ndsRelocAssetLoadFighterStreamClip(asset_id, heap, ...)`)
   with the resident decode; keep the header synthesis, registration and status-buffer writes at
   `:15090-15110` as they are, so `lbCommonAddFighterPartsFigatree` and the pose engine see today's
   bytes.
4. **Match load builds the bank** (A2): bulk-read the roster's MF spans (worst ~599 KB resident incl.
   tables), admit against the measured low-water + 25,600 B floor, and keep Samus's two spline rolls
   and any gameplay-reachable AObj32 clips (Captain BlueFalcon1/2, Kirby DKStaring; confirm
   reachability) resident in O2R form; they are outside MF (+5.3-5.9 KB per affected kind).
5. **Spend the ~111 KB of worst-roster headroom on bind cost**: keep each kind's most-bound clips raw
   (bind = pointer return, the BattlePack path at `reloc_backend_assets.c:14941-14949`) or as a
   decoded-clip LRU; each raw clip costs 0.63x its size extra. The Mario/Fox arena measured 206 hits
   of 353 acquisitions at 92,160 B (`reloc_backend_assets.c:12902-12904`), so a hot set of this size
   plausibly removes about half the decodes (ESTIMATE). Needs a per-clip acquisition profile (K0
   markers).
6. Prove on target in this order: host checker green → lab CRC self-test of the whole bank →
   pose oracle over the stress roster → a ticks-per-bind counter (P50/P95) under the four-CPU match.
