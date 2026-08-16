# Task B — the v3 took the FETCH branch, and it closes the animation-representation lane anyway

**Date:** 2026-08-15 · **Branch:** `codex/r2-runtime2` · **base HEAD `1eb6b453803`**
**2 lab builds, 2 whole-match v3 captures, 0 default flips.** `NDS_R2_FTANIM_TRACK ?= 0`.
**UNITS: 2 profile cycles = 1 project tick.** Prediction registered before the first build in
`PREDICTION.md` (same directory); nothing in it was edited afterwards.

---

## 0. Outcome first

```text
BRANCH TAKEN   FETCH.  The pre-registered ISSUE branch is REFUTED on three
               independent placement-immune measurements.

REFUTED        "+69.4 tk per exchanged call, ~1.59x the generic path."  The call
               exchange is EXACTLY 1:1 (ON 54.01 generic + 16.36 dense = 70.37;
               OFF 70.37, two decimals), and the dense call costs 208.0 tk
               against the generic parser's 215.5 -- 0.965x, not 1.59x.  The
               +69.4 was a residual divided by a count and it never belonged to
               this exchange.

REFUTED        the ISSUE branch's consequence, "full Fox ~ +3,090 tk/fr".
               Measured whole-match named exchange at 23.25% parse-call
               coverage: -74 tk/fr.  Linear to 100%: -319 tk/fr.  Opposite sign,
               9.7x smaller magnitude.

CONFIRMED      FETCH is the character of the added cost: the dense side is
               72.4% icache+dcache fill (2,736 of 3,781 tk/fr whole match), and
               the eviction signature is present -- the generic parser's
               SURVIVING calls cost +5.14% more per call in the ON arm.

AND YET        THE LANE IS DEAD AS A GATE LEVER, and this is the cycle's real
               result.  Both sides of the exchange are ~3,800 tk/fr and they
               CANCEL.  Full conversion is worth order 10^2 tk/fr = ~0.004x of
               the +85,393 gap.  33,951 tk/fr was the lane's SIZE; the
               representation converts ~1% of it, because the parse path is
               FETCH-bound and the replacement is another fetch-bound path of
               similar footprint.

CONSEQUENCE    THE FULL-COVERAGE ARENA ARM NO LONGER NEEDS TO BE BUILT.  That
               arm was blocked on 270,698 B the tree does not have; this capture
               prices the mechanism without it.

RETRACTED      MY OWN first reading, before publication.  On each arm's OWN
               top-80 mask the dense path appeared to delete 4,214 soft-float
               instructions/frame worth -2,395 tk/fr.  It deletes ZERO: whole
               match __mulsf3+__aeabi_fadd+__divsf3 is 66,457 (ON) vs 66,446
               (OFF) tk/fr at 3,722.64 calls/fr on BOTH arms -- identical to two
               decimals.  The apparent deletion was the 3-of-80 frame
               population difference between the two arms' own masks.

NOT BANKED     Nothing here is a win.  The measured placement floor on this pair
               is ~10,000 tk/fr ABSOLUTE (Section 4) and every net in this
               document is inside it.  The MECHANISM conclusions do not rest on
               any net.
```

---

## 1. The instrument, and one way the brief's recipe is wrong

Two `NDS_TASK37_PROFILE=1` builds, `smash64ds-battle-playable-tickhud-hwtri`, differing only in
`NDS_R2_FTANIM_TRACK_DISPATCH` (1 vs 0), everything else at the c199 bank's configuration
(`BOTH_CPU=1 BATTLEPACK=1 KEEP_CACHE=1 GX_COMPOSE_LAB=1 TICK_HUD_DRAW=0 FTANIM_TRACK=1`,
DLDI on), window frames **438..2038 = 1,601 regions**, `region = frame - 439`.

**Bore caveat, stated because the default moved under this cycle.** Both arms were built while
`NDS_FOX_BLASTER_BORE_OFFSET_Y` defaulted to **0**; the owner has since restored the shipping
default to **84** and reopened that row as `BLOCKED(decision: Fox bore)` (`docs/BUGS.md`). The
value is **common-mode across the pair**, so every ON−OFF delta here is unaffected — but these two
ROMs are *not* at the shipping default, and no figure in this document is a bank.

> ### The prescribed pair is NOT "one `.data` word apart", and this is a reusable trap
>
> `gNdsFtAnimTrackDispatch` (`src/nds/nds_ftanim_track.c:5`) is initialised from the macro. At
> `=1` it lands in `.data` (`020ee708 D`); **at `=0` it is zero-initialised, so the toolchain puts
> it in `.bss` (`0224873c B`)** — `.main.bss` grows 32 B and **every later `.data` object shifts
> 4 bytes**. Verified: **`.text` is address-identical** (3,842 text symbols, `nm` diff = 0 lines),
> so PC attribution and the icache side are sound, but the **data** side is a cross-build pair.
> `census-marginal-frame-owners.py --diff` checks layout from the symbol tables and did **not**
> refuse — its guard covers the text side, which is the side it joins on.
>
> **One-line fix for whoever next A/Bs a `_DISPATCH` word:** give the route word an explicit
> `__attribute__((section(".data")))` so a `0` default cannot relocate it. Recorded rather than
> made — it is a source change to the module under measurement and this cycle's deliverable is
> the attribution.

**Engagement, proven with a control that reads a hard zero:**

| symbol | ON calls/fr | ON tk/fr | OFF calls/fr | OFF tk/fr |
|---|---:|---:|---:|---:|
| `ndsFtAnimTrackStep` | **16.36** | **3,403** | **0.00** | **0** |
| `ndsR2FtAnimParseDObjFigatree` | 54.01 | 12,235 | 70.37 | 15,162 |
| `ndsFtAnimTrackIsDense` (the route check itself) | **70.37** | 896 | **70.37** | 871 |

`54.01 + 16.36 = 70.37 = 70.37`. **The exchange is exactly 1:1, on both sides, to two decimals**
— `count-both-sides-of-an-engagement` satisfied by measurement. The route check runs at an
identical rate on both arms, which is the same-binary property the design claimed.

Same fight: paired per-region work correlation **r = 0.9945** over all 1,601 regions.

---

## 2. The decisive numbers — placement-immune, whole window

`instructions` is an architectural count. Placement cannot create or destroy one.

| quantity | ON (disp=1) | OFF (disp=0) | ON − OFF | d tk/fr |
|---|---:|---:|---:|---:|
| **instructions** | 1,007,533,193 | 1,008,197,631 | **−664,438** | (−415/frame) |
| work cycles (`total − halt_wait`) | 3,113,792,088 | 3,116,363,892 | −2,571,804 | −803.2 |
| `issue` | 682,565,050 | 683,648,863 | **−1,083,813** | **−338.5** |
| `icache_fill` | 1,118,331,614 | 1,117,657,126 | **+674,488** | **+210.6** |
| `dcache_fill` | 862,570,417 | 862,414,420 | **+155,997** | **+48.7** |
| `write_buffer` | 168,891,077 | 171,138,204 | −2,247,127 | −701.8 |
| `interlock` | 135,229,563 | 135,301,032 | −71,469 | −22.3 |
| `bus_contention` / `dma_hold` | | | −498 / +618 | −0.2 / +0.2 |

Stall-class deltas sum to the work delta exactly (−2,571,804), so this is a partition, not a
selection.

> **The ISSUE branch dies here.** The ON arm executes **415 fewer instructions per frame** and its
> `issue` stalls fall by **338.5 tk/fr**. A path that executes fewer instructions *and* stalls less
> on issue cannot be paying a steady-state issue excess. **The FETCH component is real, positive
> and sized: +259.4 tk/fr (icache +210.6, dcache +48.7).** That is the branch the data took.

---

## 3. The named exchange, whole match — both sides are ~3,800 tk/fr and they cancel

Whole match, ON − OFF, tk/fr (`--diff`, PC-joined, `.text` identical):

| symbol | total | issue | icache | dcache | **instr/fr** |
|---|---:|---:|---:|---:|---:|
| `ndsFtAnimTrackStep` | **+3,403** | +693 | **+1,454** | **+1,047** | **+2,263** |
| `ndsFtAnimTrackBindJoint` | +256 | +44 | +82 | +68 | +216 |
| `ndsBattlePackAssetIdForSlotTable` | +64 | +13 | +20 | +25 | +27 |
| `ndsFtAnimTrackBeginClip` | +33 | +5 | +21 | +5 | +10 |
| `ndsFtAnimTrackIsDense` | +25 | +12 | −0 | +14 | 0 |
| **ADDED** | **+3,781** | +767 | **+1,577** | **+1,159** | +2,516 |
| `ndsR2FtAnimParseDObjFigatree` | −2,927 | −681 | −1,310 | −783 | −2,123 |
| `ndsR2AnimBuildTrackTable` | −521 | −29 | −148 | −260 | −257 |
| `ndsR2AnimTargetValue` | −407 | −71 | −144 | −144 | −246 |
| **DELETED** | **−3,855** | −781 | −1,602 | −1,187 | −2,626 |
| **NAMED NET** | **−74** | −14 | −25 | −28 | **−110** |

**The added side is 72.4 % fetch** (1,577 + 1,159 = 2,736 of 3,781). Every row carries an
instruction delta of the sign and scale its mechanism predicts.

**Per-call, and the composition caveat stated rather than buried.** `ndsFtAnimTrackStep` costs
**3,403 / 16.36 = 208.0 tk/call**; the generic parser on the control costs **15,162 / 70.37 =
215.5 tk/call**. A 1.59x path would have cost the 16.36 converted calls **5,606 tk/fr**; measured
**3,403**. *Caveat:* the converted calls are the 8 packed clips (ascending asset id), not a random
sample, so 208.0 vs 215.5 is not strictly like-for-like. **The composition-free comparison is the
whole-cluster one — the same 70.37 calls/frame cost 15,162 tk/fr generic and 15,638 tk/fr split
(+476), and the surrounding helpers give back −928.** Either way, nothing here is 1.59x.

---

## 4. What is NOT resolvable, stated as plainly as what is

**The placement floor on this pair is ~10,000 tk/fr ABSOLUTE**, and it is measured, not assumed.
Splitting all 900 listed symbols by whether their instruction delta is zero:

| | symbols | sum tk/fr | Σ|tk/fr| | d instr/fr |
|---|---:|---:|---:|---:|
| non-zero instruction delta (real work moved) | 30 | −221 | — | −410 |
| **zero instruction delta (pure placement/data)** | **870** | −488 | **10,550** | **0** |

The largest single placement mover is `ndsRendererSyncTextureTile`: **−1,046 tk/fr with 0
instructions and 0 icache** — all `dcache` (−320) and `write_buffer` (−680). It is a renderer
symbol with no relationship to animation.

**So every net in this document — whole-window −803, top-80 −1,514, named −74 — is inside that
floor and none is a win.** What is *outside* the floor is the per-symbol structure, because a
placement artifact moves cycles with a **zero** instruction delta and a real change moves them
**with** one. That test is the reason this cycle can conclude anything at all.

**A retraction I make against my own first reading.** On each arm's own top-80 mask the soft-float
trio appeared to lose 4,214 instructions/frame worth −2,395 tk/fr, and I nearly published it as
"the dense representation deletes soft float". It does not:

```text
whole match, both arms      __mulsf3 + __aeabi_fadd + __divsf3
  ON   66,457 tk/fr   at   3,722.64 calls/fr
  OFF  66,446 tk/fr   at   3,722.64 calls/fr     <- identical to two decimals
```

The two arms' own top-80 masks share only **77 of 80** regions; three swapped frames carried the
whole apparent effect. Re-running the diff on a **common** mask (the control's own 80 regions
applied to both) removes all three float rows from the table entirely. **Two masks, one honest
answer: use a common region list, or do not read a marginal delta.**

---

## 5. What this means, and it is bigger than the question that was asked

The lane was sized at **33,951 tk/fr** (the parse half of the animation bracket). This capture
measures what the representation change *converts* of it:

```text
coverage measured          16.36 / 70.37 parse calls = 23.25%
named exchange, whole      -74 tk/fr
linear to 100% coverage    -319 tk/fr        = 0.004x of the +85,393 gap
magnitude bound            |net| < 1,100 tk/fr at 23.25% even allowing +-1,000 of
                           floor contamination  ->  < 4,700 tk/fr at 100%
                                                 = < 0.055x of the gap
```

**The mechanism is real and it is fetch-shaped, exactly as predicted — and that is precisely why it
cannot pay.** The generic parse path is fetch-bound (its own cost is ~70 % icache+dcache). Replacing
3,016 B of fetch-bound parser with 1,808 B of fetch-bound stepper plus 12,244 B of sparsely-read
rows moves the work from one fill to another. The instruction count barely moves (−110/frame across
the whole named cluster) because **the parser was never issue-bound in the first place** — which is
the same lesson `K-CLOSE` learned when fixed-point collision priced at 2.68x, and the same one
`the-compare-was-never-the-cost` records.

**So: the full-coverage arena arm should not be built.** §12f found it short by 270,698 B and
called the o2r dependency a design problem. This capture says the prize on the other side of that
redesign is order 10^2–10^3 tk/fr. **That is the cheapest possible way to close a lane: price the
mechanism without paying for the configuration.**

**What this does NOT refute:** the animation *evaluate* half (53,818 tk/fr, untouched here), and
the possibility that some other change to the same lane pays. It refutes **this representation
change at any coverage**.

---

## 6. Verdict against the registered prediction — honest scoring

| # | registered claim | FETCH predicted | measured | verdict |
|---|---|---|---|---|
| 1 | `|REST| / |DTOTAL|` >= 30 % | >= 30 % | REST dominates; both sides ~3,800 vs a −74 net | **holds — for the wrong reason.** I predicted eviction collateral; it is that both halves of the exchange are large and cancel |
| 2 | generic parser per-call cost rises >= 5 % | >= 5 % | **+5.14 %** whole, +6.28 % own-mask | **holds** — but composition (the packed clips may be the cheap calls) is an untested alternative to eviction and I cannot separate them |
| 3 | dense cost per early-out > 25 tk/call | > 25 | not measurable — no early-out/stepped counter split in a profiler build | **premise refuted**: there is no per-call excess to locate. Registered a claim my chosen instrument could not test |

Registered rule was ">= 2 of 3 hold -> FETCH". **2 of 3 hold, and the direct stall partition says
the same thing far more strongly than the proxies did.**

**Where the prediction was wrong in the way that matters.** It said FETCH implies "the mechanism
survives and the full-coverage arena arm is the priced configuration". FETCH is right; that
conclusion is not. Both halves of the exchange are fetch-bound and nearly equal, so **the
per-call measurement made the full-coverage arm unnecessary rather than necessary.** I predicted
which branch, and drew the wrong consequence from it.

---

## 7. Reproduction, root ROMs, and what was not done

```powershell
foreach ($d in 1,0) {
  cmd /c "pwsh -NoProfile -File scripts\run-task37-profile-census.ps1 ``
     -MelonDS emulators\melonds-attributor\melonDS.exe ``
     -Build build-c200-trackprof-$(if($d){'on'}else{'off'}) -StartFrame 438 -Frames 1600 ``
     -OutDir artifacts\performance\2026-08-15_ftanim-dispatch-attribution\v3-... ``
     -MakeFlags NDS_R2_BOTH_CPU=1,NDS_R2_BATTLEPACK=1,NDS_R2_BATTLEPACK_KEEP_CACHE=1,``
NDS_R2_FIGHTER_GX_COMPOSE_LAB=1,NDS_TICK_HUD_DRAW=0,NDS_R2_FTANIM_TRACK=1,``
NDS_R2_FTANIM_TRACK_DISPATCH=$d > <log> 2>&1"
}
python scripts/census-marginal-frame-owners.py --reduce  --profile v3-<arm> --out c200-<arm>-pc.csv --marginal 80
python scripts/census-marginal-frame-owners.py --reduce  --profile v3-<arm> --out c200-<arm>-common80.csv \
       --region-list @control-top80-regions.txt          # the COMMON mask -- use this one
python scripts/census-marginal-frame-owners.py --diff --pc-csv c200-on-*.csv --pc-csv-base c200-off-*.csv \
       --build builds/build-c200-trackprof-on --build-base builds/build-c200-trackprof-off --top 900
python scripts/census-marginal-frame-owners.py --concentration --pc-csv <arm>.csv --build <dir> --symbols <list>
```

Root ROMs, **before the first build and after the last, both unchanged**:

```text
smash64ds.nds                        54c07fac80c50418949908701f7c2bdbf27512c5f96ac09086fabbb0df6ac68a
smash64ds-battle-playable-hwtri.nds  6c939434c53c9b3a76ff016540b810a84f207b1a4e24540b8653b15717367c99
```

**Not done:** no default flipped (`NDS_R2_FTANIM_TRACK ?= 0`, `NDS_R2_FIGHTER_GX_COMPOSE ?= 0`);
**no re-bank** — this is a profiler pair, not a gate arm, and no figure here is a bank; no
`.data`-section fix to the route word (§1, recorded not made); no cadence arm; the two RED
`check_ftanim_*` checkers are still RED and unwired; the 7.3 GB of raw `arm9-profile.csv` in
`v3-on/` and `v3-off/` are gitignored and deletable in a cleanup slot — the reduced CSVs and this
document are the evidence.
