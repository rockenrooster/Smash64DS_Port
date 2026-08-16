# The dense fighter-animation runtime is wired, its on-target oracle reads 0 mismatches over 12,232 decision points — and it found two real defects on the way

**Date:** 2026-08-15 · **Branch:** `codex/r2-runtime2` · **base HEAD `df3bc581f28`**
**Task 3, animation representation — stages 3 and 4.** 4 lab builds, 4 whole-match gate runs,
1 host corpus re-verification. **No published default flipped**; `NDS_R2_FTANIM_TRACK ?= 0`.
**UNITS: 2 profile cycles = 1 project tick.** Every table states its window.

---

## 0. Outcome first

```text
DEFECT 1  THE SHIPPED PARSER HAD A ONE-FRAME SEGMENT-PHASE REGRESSION, AND IT IS FIXED.
          `69ce92e279f` (Requirement 4) hoisted `-anim_wait - anim_speed` into a
          local `len_new` in all four write cases of
          `ndsR2FtAnimParseDObjFigatree` and added the ASSIGNMENT to only one of
          them.  The other three read a function-local that is reset to 0.0F on
          every call.  Proven by `git show 69ce92e279f^`: four identical
          `-root_dobj->anim_wait - root_dobj->anim_speed` expressions at
          :334/:375/:415/:476 before that commit, one after.  The decomp writes
          it fresh in each of its four cases (`ft/ftanim.c:158/193/232/293`).
          SIZE: opcodes 4+5 are 45,679 of the 55,261 items-off write commands =
          82.7%, and they are in a case that never assigned; only opcodes 7+8
          (9.4%) ever set it.  Every new segment started at phase 0 instead of
          `-anim_wait - anim_speed`, so `gcPlayDObjAnimJoint`'s `length += speed`
          put the first evaluated sample a whole frame INTO the segment.

DEFECT 2  THE HOST CORPUS DECODER UNDER-REPORTED WHICH TRACKS opcode 11 TOUCHES,
          AND BOTH ARMS OF THE STAGE-1 PROOF SHARED IT.  `decode_script` filled
          `targets` only when a command carries per-track WORDS, so `AddLen`
          (which scans `flags` and adds the payload to `length` on every
          selected track) looked like a command that touches nothing.
          `ftanim_script_model.run_commands` iterates the same `targets`, so the
          reference and every candidate built on that decoder agreed on the same
          wrong answer -- layers A, B and C could not see it.  THE ON-TARGET
          ORACLE FOUND IT ON ITS FIRST RUN, because it decodes the command word
          itself: `gNdsFtAnimTrackOracleBad 4`, first failure
          `0x5070B` = (flags != mask, kind AddLen, opcode 11).

STAGE 3   THE DENSE RUNTIME IS WIRED, ENGAGED AND GAMEPLAY-EXACT.
          Bind once at `lbCommonAddFighterPartsFigatree`, step typed rows per
          logical update, no command-word decode, no flag scan, no per-call AObj
          list walk, no per-call Q migration, no `ftAnimGetTargetValue`.
          Whole 1,600-frame gate match, dispatch 1 vs dispatch 0 ON THE SAME ROM:
          parse calls 144,383 -> 115,288 = 29,095 ELIMINATED, and the dense
          counters read 24,197 early-out + 4,898 stepped = 29,095 EXACTLY.
          All six gameplay invariants identical between the arms.

STAGE 4   ORACLE = 0 MISMATCHES over 12,232 decision points, whole match,
          fail-closed and PROVEN ABLE TO FAIL (it fired 4 and cleared the route
          word before defect 2 was fixed).

COVERAGE  8 of 137 Fox clips, because the pack is `.rodata` and the measured
          budget is `gNdsTaskmanGeneralHeapFreeMin` 53,136 against a mandated
          32,768 B reserve.  76 of 372 clip binds (20.4%) and 4,898 of 36,255
          stepped calls (13.5%) are dense.  THE A/B THEREFORE PRICES 13.5% OF
          THE MECHANISM, NOT THE MECHANISM.  Section 5.

A/B       TICK-NEUTRAL, AND NOTHING IS BANKED.  Same ROM, one poked volatile
          word, so the placement floor is ZERO: rank-80 1,231,872 -> 1,232,000 =
          **+128**, P50 +576, trimmed mean +1,261, and `SINT` -- the bracket the
          lane lives in -- moves +77 trimmed mean / +1,664 at rank-80.  A
          PARTIAL conversion cannot win by construction: the generic parser
          still serves 79.8% of calls, so its bytes stay hot and the dense
          stepper's 3,368 B of code plus 12,244 B of sparsely-read rows are pure
          ADDITION to the fetch footprint.  The mechanism is not refuted; this
          CONFIGURATION is.  Full coverage needs the pack in the arena in place
          of `battlepack_fox.bin` (stage 2's proven -822 B drop-in), which is a
          cross-build arm and the next cycle's work.  Sections 5.1 and 6.
```

---

## 1. Defect 1 — the segment-phase regression, and why it had to be fixed here

`ndsR2AnimSegmentStart` computes `-anim_wait - anim_speed`, the phase a new
animation segment starts at. `gcPlayDObjAnimJoint` then adds `anim_speed` to
`length` before evaluating, so a segment that starts at `-anim_wait - anim_speed`
is evaluated at phase `-anim_wait` on its first frame — 0 when the script lands
on a frame boundary, which is the whole point.

Requirement 4 (`69ce92e279f`) introduced `f32 len_new = 0.0F;` at function scope
and replaced all four `track_aobjs[i]->length = -root_dobj->anim_wait -
root_dobj->anim_speed;` with `track_aobjs[i]->length = len_new;`. It added
`len_new = ndsR2AnimSegmentStart(root_dobj, q);` to **one** case.

| case | opcodes | items-off count | assigned `len_new`? |
|---|---|---:|---|
| `SetVal0Rate{,Block}` | 7, 8 | 5,210 | **yes** |
| `SetValRate{,Block}` | 4, 5 | **45,679** | no |
| `SetValAfter{,Block}` | 9, 10 | 3,067 | no |
| `SetVal{,Block}` | 2, 3 | 1,305 | no |

82.7% of write commands are in the case that never assigned it.

**Repaired** by restoring the per-case assignment (three lines), which is what
the decomp does and what this file did before `69ce92e279f`. It is not a
fidelity trade: `PROJECT_GOAL.md` makes the decomp the specification, and this
was a transcription slip, not a DS adaptation.

**IT MOVES THE FIGHT, AND THAT IS STATED PLAINLY.** `build-c193-segfix` against
the `build-c185-gxcompose-bank` arm:

| end-of-match invariant | c185 bank | c193 repaired |
|---|---:|---:|
| `gNdsBattleTextHudP1Damage` | 76 | 76 |
| `gNdsDamageSparkScaleCount` | 15 | **16** |
| `gNdsShieldAnimJointAttachCount` | 1,352 | **480** |
| `gNdsAObjEvent32NormalizedHighWater` | 1,266 | **774** |
| `gNdsBattlePackHits` | 197 | **257** |
| `gNdsObjAnimRunawayCount` | 0 | 0 |

**So no tick comparison between c185 and any arm in this document is a
like-for-like comparison, and none is made.** `c193` reads rank-80 1,228,608
against the bank's 1,174,016, and that difference is a different match, not a
regression — `route-ab-cannot-price-a-gameplay-change`, applied to my own work.
**The bank must be re-established on the repaired tree before the next
performance verdict.** That is stated as required work, not done here.

---

## 2. Stage 3 — what the dense runtime is

### 2.1 The bind, once per status/motion selection

`lbCommonAddFighterPartsFigatree` is the only site that attaches a fighter
figatree, and it walks the DObj tree reading `figatree_entries[j]`. The AOT pack
is therefore keyed by **(asset id, figatree entry j)** rather than by the
generator's convenient "sorted distinct script offset" index, so a bind is one
array index. `generate_ftanim_track_pack.py` layer D proves that keying: every
emitted entry is decoded back out of the rows at the offset the directory
publishes and compared against the o2r script that entry actually points at.

The bind site has a POINTER and never an id, so `ndsBattlePackAssetIdForSlotTable`
was added to `nds_battlepack_anim.c` — the reverse of `ndsBattlePackFindFigatree`,
resolved out of the same directory rather than a second address map that could
disagree with it. It runs once per clip bind (372 in a whole gate match).

Per joint the bind then does, ONCE, what the shipped parser does per stepped
call: resolve the ten track AObjs by one list walk, create the missing ones, and
migrate each into the Q representation (`ndsR2FtAnimAObjToQ`, exported from
`battleship_ftanim.c` so both paths share one body).

### 2.2 The step

`ftAnimParseDObjFigatree` dispatches on one range compare against a static
array: a converted joint carries its cursor block in `anim_joint.event16`, and
nothing else in the tree reads that field for a figatree joint. At dispatch 0
no block is ever installed, so the compare is FALSE for every joint **on the
same binary at the same addresses**.

What a converted stepped call no longer executes: the 15-way opcode jump table,
the `command.flags` bit scan, the `track_aobjs[10]` rebuild, the per-node Q
migration, `ftAnimGetTargetValue` (now one shift from a per-track table), and
`ndsR2AnimRecipSlot`'s f32→Q30 conversion (now one `ldr` from a generated table
built by a bit-for-bit Python model of `ndsR2F32ToFixed`).

What it still executes, deliberately: the animation clock, `anim_wait` /
`anim_frame` / `parent_gobj->anim_frame`, the `AOBJ_ANIM_END` / `AOBJ_ANIM_NULL`
sentinels, the `func_anim(-1)` / `func_anim(-2)` call sites verbatim, and the
same AObj fields `gcPlayDObjAnimJoint` reads. Gameplay reads all of those.

### 2.3 Live state and static bytes, measured

```text
nds_ftanim_track.o        text 15,612   data 4   bss 3,604
  of which the pack       12,244 .rodata   (8 clips, 208 entries, 5,682 u16 rows)
  code                     3,368
  cursor blocks            3,552 = 2 fighters x 37 joints x 48 B
ELF delta c193 -> c196    text +15,752   data +8   bss +3,584   total +19,344
gNdsTaskmanGeneralHeapFreeMin   c193 53,136   c196 (see section 5)   floor 32,768
```

The 48 B block is `const u16 *cursor` + `AObj *slot[10]` + `u16 mask`. It is a
cursor and a resolved binding, not a second copy of the pose: the pose still
lives in the AObj fields the evaluator reads, which is what keeps
`gcPlayDObjAnimJoint` untouched and the change confined to the PARSE half.

---

## 3. Stage 4 — the oracle, and the trap it was built to avoid

`func_anim` has **no writer** anywhere in `decomp/src` or `src/`, so the −1/−2
callbacks are inert and an oracle that compares *observed* callbacks is a
control that cannot fail. This oracle compares **decision points**: for every
converted logical update, the command the generic parser would have dispatched —
opcode class, block bit, flag mask, payload, and every per-track target word, in
order — against the row the dense stepper actually consumed. The reference
cursor is advanced by the parser's own rules, including `Loop`'s
`event16 += event16->s / 2`, so a wrong jump desynchronises the reference and
every later row fails rather than one row mis-comparing.

It is **fail-closed**: the first mismatch clears `gNdsFtAnimTrackDispatch`, so
every later bind takes the generic parser.

```text
build-c194-trackoracle (before the decoder fix)
  gNdsFtAnimTrackOracleRows 64   OracleBad 4   OracleFirst 0x5070B   Dispatch 0
build-c195-trackoracle2 (after)
  gNdsFtAnimTrackOracleRows 12,232   OracleBad 0   OracleFirst 0   Dispatch 1
```

**The control can fail, and it did.** That is the whole reason the c194 run
exists in this directory.

### 3.1 What the two arms share, named as the campaign requires

The on-target oracle's two arms share the **DObj clock** (both read the same
`anim_wait`/`anim_speed`) and the **row stream's own emission** is not
re-derived here. What they do NOT share is the decoder: the reference side reads
the o2r command word through the shipped bitfield layout, the candidate side
reads the pack rows. That is exactly the leg the host-side layers A/B/C could
not cover, and defect 2 lived in it.

Conversely, the **decision → AObj state** mapping is not covered on target: it is
covered host-side by layer B over 100% of the corpus (4,901 scripts, 81,646
commands over 3 loop passes, 75,237 per-track states, 0 mismatches, falsifiers
1 / 1 / 4,272), re-run after the decoder fix. The two halves together cover the
chain; neither covers it alone, and that split is stated rather than blurred.

### 3.2 The host corpus re-verified after the decoder fix

```text
items off  259 clips / 4,901 scripts / 66,022 rows / 81,646 commands /
           75,237 states / 6,409 callbacks -> MISMATCHES 0
layer C    19,303 triples -> 0    max Q width 24 bits    over s16 2,102
falsifiers 1 / 1 / 4,272
corpus     64b2f5a6a7e86ee0bd80ef04b77542356703ff57c7b623534c683f8d521ad954
```

**The corpus hash changed** from `cb28f9bf65c4…`: the AddLen fix adds those
commands' track bits to the row masks. Row *sizes* are unchanged (AddLen carries
no per-track word), so every size figure in `FTANIM_TRACK_PACK.md` still holds.

---

## 4. Engagement and the negative control — the parser call elimination is EXACT

One ROM, `build-c196-trackperf`, `-SetGlobals gNdsFtAnimTrackDispatch=1` against
`=0`. Whole 1,600-frame `BOTH_CPU=1` `DRAW=1` gate match, frames 439–2038, DLDI
on, `slips=0` on both arms.

| counter | dispatch 0 | dispatch 1 | delta |
|---|---:|---:|---:|
| `gNdsR2FtAnimParseCalls` (generic parser) | 144,383 | 115,288 | **−29,095** |
| `gNdsR2FtAnimParseEarlyOut` | 108,128 | 83,931 | −24,197 |
| `gNdsR2FtAnimParseStepped` | 36,255 | 31,357 | −4,898 |
| `gNdsFtAnimTrackEarlyOut` | 0 | 24,197 | +24,197 |
| `gNdsFtAnimTrackSteps` | 0 | 4,898 | +4,898 |
| `gNdsFtAnimTrackBinds` | 0 | 1,980 | +1,980 |
| `gNdsFtAnimTrackBindMiss` (clip binds not in pack) | 2 | 296 | — |
| `gNdsFtAnimTrackRowsRun` | 0 | 12,232 | +12,232 |

**24,197 + 4,898 = 29,095 exactly**, on both sides of the exchange, so every
call the generic parser stopped receiving is one the dense stepper took — the
`count-both-sides-of-an-engagement` rule satisfied rather than asserted. The
negative-control arm reads a hard zero on all five dense counters, and its
`gNdsFtAnimTrackBindMiss` of 2 is the two clip binds that happen before the poke
lands.

**Coverage, measured, not estimated:** 4,898 / 36,255 = **13.51%** of stepped
calls, 29,095 / 144,383 = **20.15%** of parse calls, and 76 of 372 clip binds
(20.4%) resolve to one of the 8 packed clips.

**All six gameplay invariants are identical between the two arms** — P1Damage 76,
spark 16, shield 480, AObj high-water 774, packHits 257, runaway 0 — and equal
to the generic-only `build-c193-segfix` arm. **The dense path reproduces the
repaired parser's whole-match outcome exactly.**

`gNdsTaskmanGeneralHeapFreeMin` = **40,848** on both arms, against the mandated
32,768 B reserve: the 12,244 B pack plus 3,552 B of blocks plus code cost 12,288
B of general heap and the reserve holds.

---

## 5. The A/B — TICK-NEUTRAL. No win, no re-bank.

Same ROM, one poked `volatile` word, so **the placement floor on this comparison
is zero**. `WORK-H`, 1,600 frames:

| | dispatch 0 | dispatch 1 | ON − OFF |
|---|---:|---:|---:|
| P50 | 945,088 | 945,664 | **+576** |
| P90 | 1,126,784 | 1,132,288 | +5,504 |
| **rank-80** | **1,231,872** | **1,232,000** | **+128** |
| top-1% | 1,529,024 | 1,551,232 | +22,208 |
| max | 1,869,824 | 5,203,712 | +3,333,888 |
| trimmed mean (drop top 8) | 966,270 | 967,531 | +1,261 |
| over-gate frames | 172 | 179 | +7 |
| VBI 2/3/4/5+ · max | 1694/314/22/8 · 20 | 1694/316/20/8 · 20 | — |

**`max` is not attributable and is not read as a result.** Multi-megatick single
frames appear on generic-only arms too in this same session (`c193` max
5,271,936, `c195` 5,575,744) and the control simply did not draw one; the
trimmed mean exists in the table for exactly that reason.

**And the lane it aims at did not move.** Per-bucket, trimmed mean / rank-80:

| bucket | d trimmed mean | d rank-80 |
|---|---:|---:|
| `SINT` (contains the animation lane) | **+77** | **+1,664** |
| `FTR` | +25 | +320 |
| `GCRA` | +357 | +3,840 |
| `SCPU` | −6 | −192 |
| `WORK-H` | +1,261 | +128 |

**Verdict: at 13.5% stepped-call coverage the dense representation is
tick-neutral to slightly negative. Nothing is banked and no default moves.**

### 5.1 Why, and what it does and does not refute

This is the fixed-point collision ring's shape again, and the brief predicted the
mechanism by name: *"a flat 3.1%-top-PC lane means your win must come from
executing fewer instructions overall, and a large cold kernel will eat it."*

The structural reason a PARTIAL conversion cannot win is worth stating exactly,
because it is a property of the configuration and not of the representation:

- **the generic parser is not deleted.** It still serves 79.8% of parse calls, so
  its bytes stay resident and hot. The dense stepper's 3,368 B of code and
  12,244 B of sparsely-read `.rodata` rows are therefore **pure addition** to the
  fetch footprint, not a replacement for it.
- **the deletion may not land on the percentile.** 33,951 tk/fr is a *marginal-80*
  figure; 13.51% is a *whole-match* coverage. Whether the 8 packed clips are the
  ones stepping on the rank-80 frames is not measured here, and
  `cluster-where-the-percentile-lives` says that is exactly the question.

**No v3 capture was taken, so the neutrality is NOT attributed** between those
two candidates. Saying "it is icache" would be the third static-quantity
mispricing of this campaign; it is one `-RingDump`-free v3 run on this same
byte-identical pair to split them, with no rebuild.

**What is refuted: this configuration.** What is *not* refuted: the 33,951 tk/fr
mechanism, which needs a **full-coverage** arm where the generic parser stops
running for a fighter entirely — and that needs the pack in the taskman arena in
place of `battlepack_fox.bin` (stage 2 proved the drop-in fit at −822 B), not in
`.rodata`. Section 6 sizes that.

---

## 6. Why coverage is 8 clips, and what full coverage costs

The pack is `.rodata`, which is what makes the route a **same-binary** A/B and
the oracle a same-binary comparison — both representations have to be resident
at once for either to exist. Static image and the taskman arena come out of the
same bytes, so the budget is not `check-boot-headroom.ps1`'s 312,448 B ladder
figure; it is the measured general-heap free minimum:

```text
gNdsTaskmanGeneralHeapFreeMin   53,136  (build-c193-segfix, generic only)
mandated reserve                32,768
available for static growth     20,368
spent   pack 12,244 + blocks 3,552 + code ~3,400 + oracle          = 12,288 measured
gNdsTaskmanGeneralHeapFreeMin   40,848  (build-c195 / build-c196)
```

8 of 137 Fox clips fit. Selection is **ascending asset id**, which is ascending
motion index, so the budget buys the common motions rather than a lottery — and
it shows: 8 clips of 137 (5.8%) take 20.4% of clip binds.

**Full Fox coverage is 287,082 B and does not fit as `.rodata`.** It fits exactly
where stage 2 said it does — as the resident blob, 822 B *smaller* than the
287,904 B `battlepack_fox.bin` already streamed into the arena. That arm cannot
carry the generic path for Fox at all, so it is a cross-build A/B and it needs
the residency loader taught a second blob format. That is the next cycle's work
and it is the only configuration in which the mechanism can be priced.

---

## 7. What this cycle did NOT do

- **Did not re-bank.** The bank is still `build-c185-gxcompose-bank`, and it is
  now measured on a **different fight** (§1): the segment-phase repair moves four
  of six invariants. **A fresh bank on the repaired tree is required before the
  next performance verdict, and this cycle did not take one.**
- **Did not take a v3 capture**, so the A/B's neutrality is unattributed (§5.1).
- **Did not run the DRAW=0 cadence arm** — that is gated on net P95 ≤ 1,120,380
  and nothing here approaches it.
- **Did not convert Mario**, did not touch `gcPlayDObjAnimJoint`, and did not
  remove the AObj list: the EVALUATE half (53,818 tk/fr) is untouched by design,
  and `End`'s tail still walks the list once per script end. The brief asked for
  "no generic AObj linked-list traversal for converted fighter joints"; the
  **per-stepped-call** traversal is gone (moved to the bind), the per-`End` one
  and the evaluator's own are not. Stated as a deviation, not as done.
- **Did not repair `check_ftanim_transcribe.py` / `check_ftanim_target_exact.py`**
  (still RED and unwired). §8 states what that costs the claim.
- **Did not remove the ~1,900 tk/fr of shipped `volatile` parser counters.** They
  are not dead: `gNdsR2FtAnimParseCalls`/`EarlyOut`/`Stepped` are the exact
  negative control this cycle's engagement proof is built on, and `nm` on
  `build-c196-trackperf` shows the three dense oracle counters were dropped by
  `--gc-sections` despite `__attribute__((used))` — so deleting a counter here is
  still the documented Boundary hazard.
- **Did not grow the arena or move `NDS_TASKMAN_ARENA_SIZE`.**

## 8. The unvalidated leg, stated rather than left implicit

`check_ftanim_transcribe.py` is still RED and unwired, so **nothing statically
proves the port parser is a faithful transcription of the decomp one** — and
this cycle is the proof that this matters: defect 1 is exactly a transcription
slip, and it was found by reading the code, not by a checker. Two things bound
the risk now that did not before:

1. the on-target oracle compares the dense stepper against the **o2r command
   stream itself**, decoded through the shipped bitfield layout, over 12,232
   decision points — that is an independent read of the source data, not of the
   port parser's model of it;
2. the repair was proven against `git show 69ce92e279f^` and the decomp source,
   both of which are outside the port.

Neither is a substitute for the checker. **Re-pointing and wiring it remains the
right next hygiene item**, and it is now worth more than it looked: a live
transcribe checker would have caught defect 1 the day it landed.

## 9. State and reproduction

```powershell
# the resident pack (0.6 s) -- layer D runs inside --emit-c and gates it
python scripts/generate_ftanim_track_pack.py --items-off --fighter fox `
    --emit-c include/nds/generated/nds_ftanim_track_pack.generated.h --max-bytes 12288
# the three-layer corpus proof (~90 s), re-run after the decoder fix
python scripts/generate_ftanim_track_pack.py --items-off --verify --passes 3 `
    --json artifacts/performance/2026-08-15_ftanim-dense-runtime/trackpack-itemsoff-refix.json

$f = 'NDS_R2_BOTH_CPU=1 NDS_R2_BATTLEPACK=1 NDS_R2_BATTLEPACK_KEEP_CACHE=1 ' +
     'NDS_R2_FIGHTER_GX_COMPOSE_LAB=1'
make TARGET=smash64ds-battle-playable-tickhud-hwtri BUILD=build-c193-segfix ($f -split ' ')
make TARGET=smash64ds-battle-playable-tickhud-hwtri BUILD=build-c195-trackoracle2 `
     NDS_R2_FTANIM_TRACK=1 NDS_R2_FTANIM_TRACK_ORACLE=1 ($f -split ' ')
make TARGET=smash64ds-battle-playable-tickhud-hwtri BUILD=build-c196-trackperf `
     NDS_R2_FTANIM_TRACK=1 ($f -split ' ')

# the pair -- ONE ROM, one poked word
.\scripts\sample-tick-hud-buckets.ps1 -Build build-c196-trackperf -NoBuild -RingDump `
    -Samples 1600 -StartFrame 438 -TimeoutSeconds 3600 `
    -SetGlobals gNdsFtAnimTrackDispatch=1 -ExtraGlobals <16 names> `
    -RowsCsv .../c196-on-rows.csv -Json .../c196-on.json      # and =0 for the control
```

