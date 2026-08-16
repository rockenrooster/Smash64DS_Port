# The AOT track pack is 822 bytes SMALLER than the pack already resident, and it verifies at zero mismatches

**Date:** 2026-08-15 · **Branch:** `codex/r2-runtime2` · **base HEAD `36516db214f`**
**Task 3, animation representation — stages 1 and 2 only.** Host-side; **0 builds, 0 emulator
runs, no runtime code, no flag flipped, no ROM rebuilt.** Root ROMs byte-identical (§8).
Consumes `…/2026-08-15_sitr-direct-children/SITR_DIRECT_CHILDREN.md` unchanged.
**UNITS: 2 profile cycles = 1 project tick.** Every table states its window.

---

## 0. Outcome first

```text
STAGE 1  CORPUS EQUIVALENCE = PASS, on the WHOLE Mario/Fox corpus, three layers deep.
         items-off (the P1 corpus): 259 clips / 4,901 scripts / 66,022 rows /
         81,646 commands over 3 loop passes / 75,237 per-track states / 6,409
         callbacks -> MISMATCHES 0.  corpus cb28f9bf65c4511e8234c2487ae7b3c3e0764d72d406366e2f75e39fee5ebc56
         full 297 clips: 5,629 / 77,129 / 94,135 / 86,846 / 7,289 -> MISMATCHES 0.
         Three falsifiers fire: 1, 1, and 4,272 mismatches.

STAGE 2  SIZING = PASS, and it needs no new headroom at all.
         The Fox items-off track pack is 287,082 B against the 287,904 B
         `battlepack_fox.bin` that is ALREADY streamed into the taskman arena on
         every shipped battle boot.  -822 B.  It is a drop-in replacement, so
         "does it fit" is answered by a thing that already fits.
         Two fighters, items off: 557,670 B, 0.954x the 584,720 B of source
         FIGATREE payload it replaces.  Worst-case mutable state for two
         fighters: 11,440 B, bounded by two measured corpus maxima.

MECHANISM RE-SIZED, and it survives.  The PARSE half's 41,376 tk/fr is not all
         deletable: the animation CLOCK is irreducible and was never separated
         before.  Split at instruction level out of the c192 capture --
         CLOCK 5,305 + EPILOGUE 1,293 + EARLYOUT 763 + CHANGED 65 = 7,426
         irreducible, leaving a **33,951 tk/fr ceiling**: 1.18x the +28,689 net
         requirement and 2.12x the 16K floor.  The 41,376 headline is retired in
         favour of 33,951.

FREE FINDINGS the corpus produced, each of which removes a case from the runtime:
         opcode 12 `SetTranslateInterp` and opcode 14 `SetFlags` occur ZERO times
         -> no `aobj->interpolate`, no animation-driven `dobj->flags` write;
         track `TraI` is touched ZERO times -> `syInterpCubic` is unreachable
         from fighter animation and the one non-power-of-two quantisation arm is
         dead for fighters; `func_anim` has NO writer anywhere in `decomp/src` or
         `src/` -> the -1/-2 callbacks are currently INERT, which makes them a
         control that cannot fail and disqualifies them as an oracle signal.

NOT DONE  No runtime, no route bit, no oracle arm, no A/B.  Stages 3 and 4 are
         untouched, as the brief's staging allows.  §7.
```

---

## 1. Method, and what it is not

The generator is `scripts/generate_ftanim_track_pack.py`. It reads the 297 AObj16 o2r files
through `ftanim_reloc_probe` — slice 32's proven reader, the one that walked 100.0% of the bank
— and emits one compact typed row per executed command.

**It is not the 20-byte baked-write-record bank.** The owner closed that form
(`ftanim_bake.py` / `generate_ftanim_dense_bank.py`) for expanding the source. Measured on this
corpus it is **3.68x** the source script region — 2,414,596 B against 656,012 — so the closure is
confirmed, not merely inherited. A write record stores `value_base`, `rate_base`, `length` and
`length_invert` per (command, track), and **every one of those is derivable at run time**:
`value_base`/`rate_base` chain from the previous row of the same track, `length` is the clock,
`length_invert` is a table lookup on the frame count (the shipped `sNdsR2Recip` table already
exists and the corpus's max frame count is **185**, so it always hits). The row therefore stores
only what is authored.

### 1.1 The row

```text
u16 hdr   kind:4 | track mask:10 | has_frames:1 | block:1
[u16 frames]                         when has_frames
[s16 relative row offset]            when kind == Loop
[s16 value (, s16 rate)] x popcount(mask), in ascending bit order
```

Nine kinds, because the fifteen opcodes collapse once each `*Block` half folds into the header's
`block` bit: `End Block Linear Cubic2 Rate Cubic0 Step AddLen Loop`. Jumps are byte offsets
**relative to their own row**, so the stream is position independent and needs no load-time
fixups — the property `plan.md` §K1 phase 2 asks for.

### 1.2 Authored values are stored unchanged, and that is a size result

`ndsR2AnimTargetValue` scales an s16 by a per-track power of two, so the Q12 word it writes is a
pure left shift of the authored word (`ndsR2AnimArgToQ`, `nds_anim_fixed.h:166`). Measured over
the corpus alphabet, **the Q word needs up to 24 bits and 2,378 of the 20,601 distinct values
exceed 16 bits** — so baking the Q word would force s32 and double the value lane. Storing the
authored s16 and shifting at run time is **exact and half the size**. Layer C proves the identity
rather than asserting it.

---

## 2. Stage 1 — the equivalence, three layers, because one would be a tautology

| layer | what it compares | why the others cannot cover it |
|---|---|---|
| **A emitter fidelity** | the pack decoded with its own reader vs the same (kind, mask, frames, per-track words, jump target) sequence derived from the o2r file | catches wrong offsets, dropped or reordered commands, a wrong mask, a lost rate word, a mis-targeted jump |
| **B semantic equivalence** | `ftanim_script_model.run_commands` driven from the **o2r bytes** vs a dense replayer driven from the **pack bytes**, over 3 loop passes: per-command per-track state, the `anim_wait` timeline, the callback tag sequence, `stopped`, the terminal `anim_frame` | neither side can see the other's input |
| **C quantisation exactness** | the shipped `ndsR2AnimTargetValue(..., q=1)` expression vs the shift a dense runtime applies, over every (track, value-or-rate, authored word) triple the corpus contains | A and B are representation independent and would still pass with a wrong shift |

```text
                       clips  scripts    rows  commands   states  callbacks  MISMATCH
items off (P1 corpus)    259    4,901  66,022    81,646   75,237      6,409         0
full corpus              297    5,629  77,129    94,135   86,846      7,289         0

layer C  alphabet 19,303 (items off) / 20,601 (full)              MISMATCH  0
         max Q width 24 bits · values needing >16 bits 2,102 / 2,378

corpus hash  items off  cb28f9bf65c4511e8234c2487ae7b3c3e0764d72d406366e2f75e39fee5ebc56
             full       f01725cc2d71152960a0ebee1daaa2e7d995a3694683f3d3de84654c2ebf6f26
pack sha256  items off  fdbcaeaca54100dcbe56b5d3a59066e906c12107a7f2ef2173f57b10fd58324b
             full       9106f7dd5464e135dad174493fca743aa0624876259e5ca4c646849caef15468
```

### 2.1 The test can fail — proven on three axes, not asserted

| falsifier | items off | full |
|---|---:|---:|
| one byte flipped in the row stream | 1 | 1 |
| one script's row offset shifted by one word | 1 | 1 |
| candidate drops the `value_base <- value_target` chain | **4,272** | **4,913** |
| control: unmodified pack | **0** | **0** |

The third is the one that matters. The first two are storage falsifiers of the kind the 2026-08-15
BattlePack proof already carried; the third breaks a *semantic* rule that only a real state
comparison can see, and it fires on 87% of scripts.

### 2.2 What the two arms share — the defect classes this cannot see

Both arms read the o2r bank through `ftanim_reloc_probe`, and both take the parser's semantics
from the same transcription. **A wrong reader and a wrong model of the shipped parser are
invisible to all three layers.** This is the exact shape that produced `mismatch = 0` against a
wrong bit order on 2026-08-15, so it is named rather than left implicit. Neither is closed by this
script and neither is claimed to be:

- **the reader is validated by execution** — the same module emits `battlepack_fox.bin`, which the
  shipping ROM's own C parser consumes and the game plays;
- **the parser transcription** is guarded by `check_ftanim_transcribe.py` and
  `check_ftanim_target_exact.py` — and **both of those are currently RED and unwired**, see §6.

`check_ftanim_opcode_surface.py`, which *is* wired into `check-gbi-decode-fixtures.ps1` and runs on
every `verify-all.ps1` profile, is GREEN and independently confirms two premises this pack rests
on: `FTANIM_OPCODE_SURFACE=CLOSED` (all 15 opcodes defined are handled, and the parser invents
none) and `FTANIM_EFFECT_SURFACE=STABLE` (`func_anim` is called only from `End` and `Loop`,
`root_dobj->flags` is written only by `SetFlags`).

---

## 3. Stage 2 — the sizing gate, and it needs no new headroom

### 3.1 ROM / pack bytes

| configuration | clips | AOT track pack | source script region | source o2r payload | resident today |
|---|---:|---:|---:|---:|---:|
| **Fox, items off** | 137 | **287,082** | 285,300 | 299,744 | **287,904** (`battlepack_fox.bin`) |
| Mario, items off | 122 | 270,718 | 273,072 | 284,976 | — (still FAT/raw-cache) |
| **both, items off** | 259 | **557,670** | 558,372 | 584,720 | 553,696 (measured items-off BattlePack) |
| full corpus | 297 | 656,708 | 656,012 | 686,192 | 651,928 (measured full BattlePack) |

```text
PACK / SOURCE SCRIPT REGION   0.9987x (items off)   1.0011x (full)
PACK / SOURCE o2r PAYLOAD     0.954x  (items off)   0.957x  (full)
composition, items off        header 20 · clip dir 3,108 · script table 29,406 · rows 525,136
identical script runs shared  421 (27,672 B)
```

**The decisive row is the first one.** `battlepack_fox.bin` is 287,904 B and is streamed into the
taskman arena on every shipped battle boot (`src/nds/nds_battlepack_anim.c`, `NDS_R2_BATTLEPACK=1`
and `NDS_R2_BATTLEPACK_BLOB_BYTES 287904u` in `build-c185-gxcompose-bank/nds_build_config.h`). The
Fox track pack is **287,082 B — 822 bytes smaller**. It is a drop-in replacement for a resident
object of the same size, so the sizing question is answered by construction and not by a new
headroom measurement. The 32,768 B reserve is not touched at all.

For the two-fighter case the pack is 557,670 B against 553,696 B — **+3,974 B (+0.7%)** — and it
would additionally make the 262,144 B raw-file animation cache arena unnecessary for Mario, which
is a net *reclaim*. That reclaim is a lead, not a saving: it needs `NDS_TASKMAN_ARENA_SIZE`, the
`0x130000` search floor and the Task 36 replay-admission guard moved together
(`BATTLEPACK_ANIMATION.md` §11.1(b)), and none of that is done here.

### 3.2 Live mutable state, two fighters — bounded by two measured maxima

The runtime state a converted joint owns is a flat array of typed tracks: `kind` u8 plus the six
s32 words `ndsR2AnimValueQ` reads (`length`, `length_invert`, `value_base`, `value_target`,
`rate_base`, `rate_target`) = 28 B aligned, plus an 8 B per-joint header (row cursor + union mask).
`anim_wait`/`anim_frame`/`anim_speed` stay in the DObj — §5 says why they must.

```text
max joint scripts in one clip        22        (measured, whole corpus)
max union track popcount per script   9        (measured; mode is 3, mean 4.06)
per joint, worst case             9 x 28 + 8 = 260 B
TWO FIGHTERS, WORST CASE          22 x 260 x 2 = 11,440 B
two fighters at the mean          22 x (4.06 x 28 + 8) x 2 =  5,353 B
what it replaces: AObj nodes      36 B x 4.06 x 22 x 2     =  6,431 B
```

**11,440 B worst case, against a 32,768 B mandated reserve and a measured
`gNdsTaskmanGeneralHeapFreeMin` of 52,864 (arm G) to 72,188 (shipping arena, `docs/HANDOFF.md`).**
Both bounds are corpus maxima, not estimates, and the pack carries the per-script union mask so a
bind can size to the actual track count instead of the bound.

This is the owner's "compact and bounded per active fighter/motion" requirement met with two
measured numbers. It is **not** a 2.78 MB resident expansion and it is **not** ten reserved
20-byte slots per joint.

---

## 4. The mechanism, re-sized — 41,376 was too big, and 33,951 is the number

`SITR_DIRECT_CHILDREN.md` handed over a PARSE half of 41,376 tk/fr. That figure includes the
**animation clock**, which no representation can remove: `anim_wait -= anim_speed`,
`anim_frame += anim_speed`, the store into the parent GObj, and the two sentinel compares run on
every call whether or not the script steps. Nobody had separated it. Split here at instruction
level out of `c192-pc.csv` and `c192.dis` — same capture, same marginal-80 mask, basis
`cycles/160`, **zero new runs**:

| block in `ndsR2FtAnimParseDObjFigatree` | marg tk/fr | whole | insn/fr | dfill/fr |
|---|---:|---:|---:|---:|
| TOTAL | 26,368 | 12,936 | 23,054 | 14,048 |
| CLOCK (prologue + clock + compares; **every** call) | 5,305 | 4,025 | 3,178 | 4,196 |
| EPILOGUE (shared by every return) | 1,293 | 911 | 679 | 1,443 |
| EARLYOUT (counter, branch to epilogue) | 763 | 688 | 275 | 445 |
| CHANGED (`anim_wait = -anim_frame`) | 65 | 12 | 72 | 0 |
| **STEPPED-EXCLUSIVE** | **18,942** | 7,300 | 18,852 | 7,964 |

```text
PARSE half                  41,376   (26,368 + 7,308 + 5,014 + 2,687)
  irreducible clock/return   7,426   (5,305 + 1,293 + 763 + 65)
  DELETABLE CEILING         33,951   = 1.18x the +28,689 requirement, 2.12x the 16K floor
```

### 4.1 The call split, measured from the counters' own entry PCs

The parser's three shipped `volatile` counters give an exact per-path call rate for free
(`entry-PC-gives-exact-call-counts`), because each increment sits on its own basic block:

```text
gNdsR2FtAnimParseCalls++      96.9 exec/marginal frame   every call
  of which ANIM_CHANGED       18.0                        first tick of a newly bound animation
  arithmetic clock path       78.9
gNdsR2FtAnimParseEarlyOut++   54.9                        56.7% -- the clock, and nothing else
gNdsR2FtAnimParseStepped++    42.0                        43.3% -- what the rows replace
```

### 4.2 The function is FLAT, so the lever is executing fewer instructions

Top PC inside the stepped body is **3.1%** of it. Under the ~4% rule
(`a-flat-function-only-lever-is-not-entering-it`) there is no instruction to delete; the cost is
23,054 instructions a frame at 2.49 cycles each, spread across a 15-way jump table, a flag-bit
scan, and six field writes per track into 36-byte `AObj` nodes reached through a linked list.
The named structural pieces, for anyone sizing the replacement:

| site | marg tk/fr | exec/fr | what it is |
|---|---:|---:|---|
| `ldr r3,[r7,#4]` + `str r0,[r3,#0x78]` | ~1,570 | 78.9 | `root_dobj->parent_gobj->anim_frame = …` — a cold second-object hop, the single most expensive instruction in the parser (17.9% of the CLOCK block, 2,095 dcache-fill cyc/fr) |
| `ldr r2,[sp,#8]` + `ldr r3,[r2,r3]` + `mov pc,r3` | 923 | 89.5 | the 15-way opcode jump table |
| `ldrb r3,[r3,#0]` | 651 | 89.5 | the FIGATREE command byte — ~100% dcache fill, i.e. cold on essentially every touch |
| the three diagnostic counters | ~1,900 | 96.9 / 54.9 / 42.0 | `volatile` global RMWs in the **shipped** path |

### 4.3 What is NOT claimed

- **No win is banked.** 33,951 is a ceiling from a measured decomposition, not an A/B.
- **The replacement's own instruction fetch is not priced.** The fixed-point collision ring
  executed 204 fewer instructions a frame and still lost on `icache_fill +1,854`. A dense
  evaluator's bytes are compulsory-miss on entry exactly the same way, and that must be measured
  on the candidate, not argued here.
- **The EVALUATE half (53,818 tk/fr) is untouched by this document.** A dense pose makes it
  cheaper, and by how much is stage 3's measurement.

---

## 5. What must still happen, exactly — the callback and output enumeration

Enumerated from the fifteen case bodies of `src/import/battleship_ftanim.c` and cross-checked by
the wired `check_ftanim_opcode_surface.py`.

| output | when | who consumes it |
|---|---|---|
| `parent_gobj->anim_frame` (f32) | every arithmetic-clock call, and again at `Loop` and `End` | **gameplay**: `ftanimend.c:6` (`<= 0.0F`), `ftmain.c:188/563/723/727/781/785/878/882` motion-script waits, and per-character specials (`ftfoxspecialhi.c` 11 refs, `ftpikachuspecialhi.c` 18) |
| `dobj->anim_wait` + the `AOBJ_ANIM_END` / `AOBJ_ANIM_NULL` sentinels | clock, `End`, runaway | `gcPlayDObjAnimJoint`, `ftParamUpdateAnimKeys`'s idle-joint skip |
| `dobj->anim_frame` | clock, `Loop`, `End` | the parser itself, `ANIM_CHANGED` |
| the AObj/pose fields | every write command | `gcPlayDObjAnimJoint` → `dobj->rotate/translate/scale` |
| `func_anim(dobj, -2, 0)` | `Loop`, only when `is_anim_root` | **nothing — see below** |
| `func_anim(dobj, -1, 0)` | `End`, only when `is_anim_root` | **nothing — see below** |
| `dobj->flags` | `SetFlags` | **never reached in this corpus** (opcode 14 count 0) |
| `aobj->interpolate` | `SetTranslateInterp` | **never reached in this corpus** (opcode 12 count 0) |
| `gNdsObjAnimRunaway{Count,Mask,Script,Opcode}` | unknown opcode, or 64 events in one call | the runaway diagnostic |

> **`func_anim` has no writer.** Searched with `--no-ignore` across `decomp/BattleShip-main/decomp/`
> and `src/`: the field is declared at `objtypes.h:220`, set to `NULL` at `objman.c:1717`, and read
> at four sites (two in `objanim.c` for the AObj32 path, two in `ftanim.c`). **No assignment other
> than NULL exists in either tree.** Modality: text search over both trees plus the wired
> effect-surface checker; that is not runtime proof of absence, so the dense arm must keep the call
> sites verbatim. But it has a hard consequence for stage 4: **an oracle that compares observed
> `func_anim` calls is a control that cannot fail.** The oracle must compare the *decision points*
> — the tag sequence the parser would emit — which is what layer B does here.

### 5.1 Two cases the corpus deletes, and one it does not

- **`TraI` is never touched.** Track-usage over the full corpus: RotX 27,970 · RotY 25,083 ·
  RotZ 39,648 · TraX 3,385 · TraY 3,824 · TraZ 3,768 · ScaX 609 · ScaY 519 · ScaZ 498 · **TraI 0.**
  So `syInterpCubic` is unreachable from fighter animation, and the one non-power-of-two
  quantisation arm (`1/16384 - 3e-12`, ids 3 and 7) is dead for fighters. The generator still
  implements it, and layer C still checks it, because a *stage* clip could use it.
- **`SetFlags` and `SetTranslateInterp` never occur.** The generator **REFUSES** a clip containing
  either rather than mis-encoding it, and prints the count.
- **The clock cannot move to fixed point casually.** `parent_gobj->anim_frame` is an f32 that
  gameplay compares against f32 motion-script values on eight lines of `ftmain.c` alone. Any
  change to its representation is a gameplay-fidelity question, not a codegen one.

---

## 6. Two guards this proof leans on are RED, and have been since `514fad238da`

| checker | wired into a verifier? | state | why |
|---|---|---|---|
| `check_ftanim_opcode_surface.py` | **yes** — `check-gbi-decode-fixtures.ps1:68`, every profile | **GREEN** | — |
| `check_ftanim_transcribe.py` | no | **RED** | `FAIL: 1 inverse rule(s) matched nothing … ndsR2Recip(payload_u)` — the shipped body now calls `ndsR2AnimRecipSlot(payload_u, q)` |
| `check_ftanim_target_exact.py` | no | **RED** | its extracted harness does not compile: `implicit declaration of function 'ndsR2AnimArgToQ'` |

Both were stale before this cycle — neither file is modified in this tree and both broke when
Requirement 4 rewrote the parser in fixed point (`514fad238da`, *"The figatree parser had 45
soft-float calls in one function; now 21"*). Neither is a defect in shipped code; both are checker
drift, and because neither is wired, nothing went red to say so.

**How much this actually costs the claim above:** less than it looks. `check_ftanim_target_exact.py`
guards the **float** arm of the value scaling, which no published ROM runs (`q == 1` whenever
`NDS_R2_CUBIC_FIXED` is 1 and `NDS_R2_ANIM_CUT_ROUTE` is 0 — both true in `build-c185-gxcompose-bank`
and `build-c192-sitr-profile-gxc`). The **Q** arm is what ships, and layer C proves it directly over
20,601 corpus values. `check_ftanim_transcribe.py` is the real gap: nothing currently proves the
port parser is still a faithful transcription of the decomp one.

> Item, not a detour: **re-point both checkers at the Requirement-4 body and wire them into
> `check-gbi-decode-fixtures.ps1` next to the opcode-surface checker.** Recorded, not done — it is
> outside this row and the transcribe checker needs its whole `ALLOWED` rule set rewritten for the
> Q arm.

---

## 7. What this cycle did NOT do

- **No runtime, no `NDSAnimTrackInstance`, no route bit, no oracle arm, no A/B.** Stages 3 and 4
  of the brief are untouched. **0 builds, 0 emulator runs.**
- **Nothing is banked.** The bank stays `build-c185-gxcompose-bank`, rank-80 1,174,016 raw /
  1,149,069 net, **+28,689 net**. §4 is a ceiling, not a win.
- **The pack is not wired into the Makefile** and no ROM contains it. `--out` writes it on demand.
- **The 262,144 B raw-cache reclaim is a lead, not a saving** (§3.1).
- **The two stale checkers were not fixed** (§6).
- **The ~1,900 tk/fr of shipped diagnostic counters was measured, not removed** — `gc-sections
  drops harness globals` says a deleted diagnostic has turned Boundary red here before, so that
  needs the verifier's ELF symbol list checked first.
- **`func_anim`'s absence is a search result, not runtime proof** (§5).

## 8. State and reproduction

Root ROMs **unchanged** across the cycle, hashed before the first command and after the last:
`smash64ds.nds` `54c07fac80c50418949908701f7c2bdbf27512c5f96ac09086fabbb0df6ac68a`,
`smash64ds-battle-playable-hwtri.nds` `6c939434c53c9b3a76ff016540b810a84f207b1a4e24540b8653b15717367c99`.

```powershell
# the pack, its sizing and the three-layer proof (~90 s per configuration)
python scripts/generate_ftanim_track_pack.py --items-off --verify --passes 3 `
    --json artifacts/performance/2026-08-15_ftanim-track-pack/trackpack-itemsoff.json
python scripts/generate_ftanim_track_pack.py --verify --passes 3 `
    --json artifacts/performance/2026-08-15_ftanim-track-pack/trackpack-full.json
# per-fighter sizing
python scripts/generate_ftanim_track_pack.py --items-off --fighter fox
python scripts/generate_ftanim_track_pack.py --items-off --fighter mario
```

`parse-block-split.txt` and `parse-clock-detail.txt` carry §4's instruction-level split, reproduced
from `…/2026-08-15_sitr-direct-children/c192-pc.csv` and `c192.dis` with no new capture.
