# R2-07 phase audit — every sub-item, with evidence

The plan's own status line puts R2-00a/b/c, R2-01, R2-02 as gated, R2-05
complete, R2-03 shipped, R2-06 Boundary-green, and **R2-07 as the live phase**.
So "finish the plan" reduces to finishing R2-07, then R2-08. This audits R2-07's
*whole* scope, not just its Bugs clause.

| # | R2-07 sub-item | Status | Evidence |
|---|---|---|---|
| 1 | START on Results restarts the match | **CLOSED, keep closed** | plan says the `BUGS.md` row is closed; `PORTING.md:21948` records the contract |
| 2 | Ported particle banks | **DONE** | `NDS_PARTICLE_BANKS.generated.json`: 119 scripts / 47 textures banked, 33 packed |
| 3 | SFX / voice / BGM | **PARTIAL, bounded debt** | `KNOWN_ISSUES.md:109-117`: 128,196-byte pack, 21 exact source ids + 6 contacts, 2,876 B headroom; 5 composites and 5 fighter voices **fail closed** |
| 4 | HUD | **DONE** | tick-HUD and match HUD both live; `HUD` is its own bucket in every run |
| 5 | GAME SET → Results flow | **IMPLEMENTED, not gate-verified** | lifecycle flags exist (`gNdsSCVSBattleLifecycleIsSuddenDeath`), a soak covered "full match, Sudden Death and Results" (board :1879) |
| 6 | Cosmetic systems get explicit budgets | **NOT DONE** | plan §4 says "the measured margin, not the table"; no per-system cosmetic budget is recorded anywhere |
| 7 | **All `BUGS.md` rows fixed** | **3 OPEN, all localized** | see `../../bugs/2026-08-12_r2-07-cluster/CONTRACT.md`; row 3's state half shipped |
| 8 | Gate: 1-minute demo loop ≤ 1.12M, DLDI-on | **FAILS** | `BOTH_CPU 1` HEAD = **1,207,616**, +87,236 over — `ARM_MISLABEL.md` |
| 9 | Gate: **5-minute** demo loop | **NEVER MEASURED** | no 5-minute run exists on either arm |
| 10 | STRESS TEST: full match incl. Sudden Death via force switch | **NEVER MEASURED as a gate** | the soak proves no-freeze, not a P95 |
| 11 | STRESS TEST: infinite successive restarts | **NOT VERIFIED** | no harness runs N successive matches |

## The blocking fact

**Item 8 fails on the arm R2-07 itself names.** Until the gate arm is back under
1,120,380 there is nothing to gate items 9–11 against, and R2-08 cannot start —
its acceptance item 3 is a performance gate too.

This is not a small overrun that better measurement will absorb: **+87,236** is
~2.7× the entire margin the Boundary arm has, and it is far outside the
placement noise (±5,376 same-shape, ~95,000 only when an object is added) that
qualified the old "met, not stably met" reading.

## What the mislabel costs beyond the headline

Every lane ceiling in `EXHAUSTION.md` — `SRC`/`GCRA` 133,056, `SINT` 57,280,
`SHDT` 38,912, `MISC` 31,680, and the four consecutive `FTR` **0** readings —
was computed from `BOTH_CPU 0` rows. On the gate arm the distribution is
different by ~120,000 at P95, so:

- **`FTR = 0` cannot be carried over.** It was the basis for closing the whole
  renderer lane, and the board's own cycle-79 note says the Boundary arm
  flatters lanes by 4–9× on exactly this kind of comparison.
- The same applies to `STG` 2,048 and every "closed by measurement" lane whose
  number came from that table.

**Recompute the ceilings on `BOTH_CPU 1` before picking the next lever.** Doing
otherwise repeats the error this document exists to correct.

## Order of work this implies

1. Re-bank the gate on `BOTH_CPU 1` and re-attribute the top-80 there.
2. Recompute lane ceilings on that distribution; re-open any lane whose "dead"
   verdict came from the Boundary arm.
3. Close the three `BUGS.md` rows (independent of the gate; they are correctness
   and content, not performance).
4. Then items 9–11, which only mean something once item 8 passes.
5. Then R2-08 §6, whose acceptance is the **Boundary** configuration and whose
   items 2 and 5 require the owner regardless.
