# The bank is re-established on the repaired tree, the Fox bore was compensation for the defect and is now 0 — and full-coverage dense animation is REFUSED on a measured sizing failure

**Date:** 2026-08-15 · **Branch:** `codex/r2-runtime2` · **base HEAD `5d014c1519b`**
3 lab builds, 2 whole-match gate runs, 1 Boundary, 0 emulator captures for Task C.
**UNITS: 2 profile cycles = 1 project tick.** Every table states its window.
**One shipping default was flipped, on the owner's explicit verdict** (§2).

---

## 0. Outcome first

```text
BANK      RE-ESTABLISHED ON THE REPAIRED, BORE-0 TREE.  build-c199-bank0,
          BOTH_CPU=1 DRAW=1 DLDI on, mode 163 one-minute, 1,600 samples,
          -RingDump, frames 439..2038, slips=0.
            rank-80  1,230,720 raw   1,205,773 net   GAP  +85,393
          The c185 figure (1,174,016 raw / 1,149,069 net / +28,689) is
          SUPERSEDED AND UNUSABLE: it measured a different fight.  The gap did
          not grow by an optimization regressing -- it grew because the fight
          the gate runs got more expensive when a shipped defect was repaired.
          Nothing in this document is compared against c185, and every place an
          older figure appears it is labelled superseded.

BORE      THE +84 FOX BLASTER OFFSET WAS COMPENSATION FOR THE PARSER DEFECT AND
          IS NOW 0, OWNER-CONFIRMED.  "fox beam is perfect!" (owner,
          2026-08-15, on build-c198-bore0).  The 2026-08-14 owner-confirmed
          "perfect" at 84 is superseded: it was hand-tuned against a gun-joint
          pose the parser left a whole frame stale.  Beam draw, muzzle/impact
          glow and the weapon attack collision read ONE constant, proven by
          enumeration over the whole tree, so presentation and hitbox moved
          together in both directions and never desynced.
          MEASURED, AND IT SURPRISED ME: the flip is gameplay-INERT over this
          match -- all eight end-of-match invariants and all four parser
          counters are IDENTICAL to the bore-84 arm.  §2.3.

TASK B    FULL COVERAGE IS REFUSED, AND THE REFUSAL IS ARITHMETIC, NOT AN
          OPINION.  The brief's "already-proven -822 B drop-in" is a BYTE-COUNT
          IDENTITY, not a functional interchange: three consumers need the o2r
          command stream that `battlepack_fox.bin` carries, so the track pack
          can only COEXIST with it, never replace it.  Coexistence needs
          +287,082 B of taskman arena.  Measured on this cycle's own arm:
          ChosenSize 1,548,288 with AllocFail 0, against a measured grantable
          ceiling of 1,564,672 -- +16,384 available.  SHORT BY 270,698 B.
          A sizing failure is a STOP, and I stopped.  §3.

PRICE     WHAT THE EXISTING PAIR ALREADY SAYS, computed here for the first
          time: the dense path costs +69.4 ticks per exchanged call, i.e.
          ~1.59x the generic path it replaced, at 13.51% stepped coverage.
          Derived, with its inputs and its caveats stated (§4.1).  Whether
          coverage would fix that depends entirely on whether the excess is
          compulsory FETCH or steady-state ISSUE, and that is one v3 capture
          which this cycle did NOT take.  §4.
```

---

## 1. Task A — the new bank

### 1.1 The arm

`build-c199-bank0`, `smash64ds-battle-playable-tickhud-hwtri`, flags
`NDS_R2_BOTH_CPU=1 NDS_R2_BATTLEPACK=1 NDS_R2_BATTLEPACK_KEEP_CACHE=1
NDS_R2_FIGHTER_GX_COMPOSE_LAB=1`. Its `nds_build_config.h` differs from
`build-c193-segfix`'s in exactly the three inert `NDS_R2_FTANIM_TRACK*` defines
(all 0, TU not linked), the bore, and the git string — machine-diffed.
`NDS_R2_FIGHTER_GX_COMPOSE` reads **1** in the generated header, so the
documented `_LAB` escape took, and this is the c185 bank *configuration* on the
repaired tree.

ROM `11f5d7d988aeb0a05627f7804c2ccc9e1f930c0b1e9a243d54f269b88ee2046c`.

### 1.2 The numbers, `WORK-H`, 1,600 samples, frames 439..2038, `slips=0`

Basis stated because two are in circulation: **rank-80 is recomputed from the
1,600 per-frame rows in the run's own JSON**, which reproduces
`DENSE_RUNTIME.md`'s table exactly. The harness banner's `p95` column uses a
different rank convention and is **not** the banked figure.

| | c193-segfix (bore 84) | **c199-bank0 (bore 0) — THE BANK** |
|---|---:|---:|
| P50 | 942,400 | **942,912** |
| P90 | 1,126,400 | **1,124,480** |
| **rank-80 raw** | 1,228,608 | **1,230,720** |
| **rank-80 net of apparatus 24,947** | 1,203,661 | **1,205,773** |
| **gap to 1,120,380** | +83,281 | **+85,393** |
| top-1% | 1,548,864 | 1,550,592 |
| max | 5,271,936 | 5,124,864 |
| trimmed mean (drop top 8) | 964,311 | 963,993 |
| over-gate frames | 175 | 170 |
| VBI 2/3/4/5+ · max · total | 1712/299/19/8 · 19 · 2038 | **1701/313/16/8 · 19 · 2038** |

**Two independent arms bracket the new level.** `c193-segfix` and `c199-bank0`
are separate builds at different HEADs and different bore values, and they read
rank-80 **1,228,608** and **1,230,720** — 2,112 apart, well inside the
cross-build P95 floor of **≥14,080** (`VERIFYING.md` cycle-100), with P50 512
apart inside the ~5,700 P50 floor. The bank is not one run.

### 1.3 End-of-match invariants and the arena

```text
P1Damage 76 · spark 16 · shield attach 480 · AObj high-water 774 ·
packHits 257 · runaway 0 · Task36 CaptureOutcome 2 · SegmentMask 161
parser: ParseCalls 144,383 · EarlyOut 108,128 · Stepped 36,255 · NullSkips 70,796
arena:  ChosenSize 1,548,288 · AllocFail 0 · BattlePackResidentBytes 287,904 ·
        BattlePackLoadFails 0 · GeneralHeapFreeMin 53,136 (reserve 32,768)
```

Every one of those is **identical** to `build-c193-segfix`. The invariant pair
differs from the c185 bank exactly as `DENSE_RUNTIME.md` §1 recorded (spark
15→16, shield 1,352→480, AObj 1,266→774, packHits 197→257) and that difference
belongs to the **parser repair**, not to anything in this cycle.

### 1.4 The gap tripled, and that is the honest headline

`+28,689` → `+85,393` net. **No optimization regressed.** The c185 bank was
measured on a match the shipped segment-phase defect made cheaper; the repaired
match costs 56,704 more raw ticks at rank-80. Every lever priced against
`+28,689` — the whole `MENU.md` ladder — is now priced against the wrong
requirement and must be re-read against **+85,393**. That is the single most
consequential number this cycle produced and it makes the milestone harder, not
easier.

---

## 2. The Fox bore — an owner decision, and what it does and does not mean

### 2.1 The desync question, answered by enumeration over the whole tree

`NDS_FOX_BLASTER_BORE_OFFSET_Y` has **one definition** (`nds_effects.h`) and
**three** consumers, and there is no second copy of the value anywhere:

| consumer | site | form |
|---|---|---|
| beam draw | `src/nds/nds_renderer.c:15375,15379` | `_Q12`, world +Y |
| muzzle/impact glow draw | `src/import/battleship_lbparticle.c:2588,2590,2606` | `_Q12` and `f32` |
| weapon attack collision | `src/import/battleship_fox_blaster.c:77,78` | `f32`, `lr`-signed weapon-local so Rz(0)/Rz(pi) both give world +Y |

A tree-wide search for the raw value in any other form (`344064`, `0x42a80000`,
`0xc2a80000`, `84.0F`) returns **nothing** in `src/` or `include/`. And the
*base* the three add to is also one value: `ftfoxspecialn.c` resolves local
`{60,0,0}` through `gmCollisionGetFighterPartsWorldPosition` once and hands that
vector to `wpFoxBlasterMakeWeapon`, which passes the same vector to the glow —
recorded at `battleship_fox_blaster.c:110-130`. **One base, one constant, three
consumers: a visual/collision desync is not expressible here.** The repair moved
the base for all three together; removing the offset moves all three together.

### 2.2 Why 84 was wrong the moment the parser was repaired

The 84 was reached by eye in steps 24 → 36 → 48 → 72 → 84 on 2026-08-14. On that
day the figatree parser started every new animation segment at phase `0` instead
of `-anim_wait - anim_speed` in **82.7% of write commands**, so the gun joint's
pose at the firing instant was a whole frame stale and the derived shot point sat
low. The offset was compensation for a defect. With `64c41c361a7` in, the owner
looked at offset **0** and said, verbatim:

> **"fox beam is perfect!"** — owner, 2026-08-15

Landed as the shipping default (`nds_effects.h`, `Makefile`), build-overridable:
`make TARGET=... BUILD=... NDS_FOX_BLASTER_BORE_OFFSET_Y=<n>`. The owner's ROM
was `builds/build-c198-bore0/smash64ds-battle-playable-proof-hwtri.nds`,
SHA-256 `95d75cf6d69a949ceed7a95124c6543b54b4a02f882e963e8e0bafbe6d5ec997`,
whose config differs from the owner's own harness ROM in the bore and the git
string only.

### 2.3 It is gameplay-inert over this match — measured, and I expected otherwise

Moving a hitbox 84 world units is a gameplay change and I re-ran the bank rather
than reconcile, as required. The result is that **all eight end-of-match
invariants and all four parser counters are bit-identical** between the bore-84
arm (`c193-segfix`) and the bore-0 arm (`c199-bank0`) — `P1Damage 76`,
`spark 16`, `shield 480`, `AObj 774`, `packHits 257`, `runaway 0`, and the ROMs
themselves differ (`f53b9efb…` vs `11f5d7d9…`). In this deterministic one-minute
match **no blaster hit outcome changed**. That is a measurement of this match,
not a proof that the hitbox move is inert in general: v5's own geometry says
crouch clearance goes 45.181 → **−38.8** (the laser now sits on the old source
line, `Y=223.398 ± 20`, against a crouching Mario hurtbox topping out at
242.218), i.e. **crouching no longer clears the beam by geometry**. That was the
symptom v5 existed to fix, and it is now back by construction. **Flagged for the
owner's eye, not decided here** — it is a gameplay-fidelity question and the
source line is what BattleShip itself uses.

### 2.4 The other three 2026-08-14 eye-tuned fixes — enumerated, and none is suspect

The concern is correct in principle: anything hand-tuned by eye in that window
was tuned against a stale animation pose. Enumerated rather than re-tuned:

| 2026-08-14 owner-confirmed fix | is it a constant tuned by eye? | reads an animation-driven pose? | suspect |
|---|---|---|---|
| **Fox bore 84** | yes (24→36→48→72→84) | **yes** — the shot point is joint 17's world pose | **CONFIRMED, fixed above** |
| Whispy dynamic stage matrices | no — a per-frame *validation* fix, no constant | stage joints dispatch to `gcParseDObjAnimJoint` (AObj32) at `reloc_backend_compat_shims.c:2057`; the repaired parser is the `else` arm at `:2061`, the fighter figatree (AObj16) path | no |
| Dream Land BG edges `K=9/8` | yes | no — a background affine scale on static q16 metadata | no |
| P1 VFX symmetry (MASKS/MASKT) | no — a missing atlas submit flag | no — texture coverage | no |

**Only the Fox bore was pose-derived, and it is the one that was already caught.**
Bound stated honestly: the AObj32/AObj16 split above is read from the dispatch
site, not proven at runtime this cycle; `NDS_ANIM_JOINT_AUDIT`'s
`gNdsAnimJointDispatch32Count` / `…FigatreeCount` is the counter that would prove
it, and it was not run.

---

## 3. Task B — full coverage is REFUSED, and here is the arithmetic

### 3.1 The premise fails first: the "drop-in" is a byte count, not an interchange

Stage 2 established `287,082 B` against `287,904 B` = **−822 B**, and its own
text scopes that claim to sizing: *"the sizing question is answered by
construction"*. It is **not** a functional interchange. Three consumers need the
o2r command stream `battlepack_fox.bin` carries, and all three are in the shipped
or the candidate path:

1. **the bind's identity.** `ndsFtAnimTrackBeginClip(root_dobj, figatree)` is
   handed a POINTER and recovers the asset id with
   `ndsBattlePackAssetIdForSlotTable` (`src/nds/nds_battlepack_anim.c:180`),
   which resolves it *as an offset inside the o2r blob*. Remove the blob and
   there is no pointer to resolve.
2. **the fail-open path.** Any clip not in the pack takes the generic parser —
   `gNdsFtAnimTrackBindMiss` was **296 of 372 clip binds** even at 8 clips, and
   **Mario is unconverted entirely**, so the generic parser is load-bearing
   whatever Fox's coverage is.
3. **the oracle.** Its reference cursor is a `const AObjEvent16 *ref` field on
   the joint block (`src/nds/nds_ftanim_track.c`) — the o2r stream itself.
   Replace the blob and the oracle cannot run, and the brief requires the oracle
   at full coverage.

So full coverage means **coexistence**, not replacement.

### 3.2 Coexistence is short by 270,698 bytes — measured on this cycle's arm

```text
measured this cycle (c199-bank0 extras)
  gNdsTaskmanArenaChosenSize        1,548,288      (= NDS_TASKMAN_ARENA_SIZE 0x17a000)
  gNdsTaskmanArenaAllocFailCount            0      -> granted in full, no step-down
  gNdsBattlePackResidentBytes         287,904      gNdsBattlePackLoadFails 0
  gNdsTaskmanGeneralHeapFreeMin        53,136      against the mandated 32,768

banked in-tree (diagnostics.c:7782-7788, ARENA_PRICE.md, 660 s stress battery)
  measured grantable arena ceiling  1,564,672
  arena growth still available         16,384      = 1,564,672 - 1,548,288
  taskman residue today             1,096,512      = 1,548,288 - 451,776 reserved
  ... which is ALREADY 17,600 B BELOW the non-battlepack arm's 1,114,112

the ask
  second resident blob (Fox pack)     287,082
  available                            16,384
  SHORT BY                            270,698
```

**That is a sizing failure and a sizing failure is a STOP.** The failure mode is
not a slow ROM, it is the one recorded at `reloc_backend_assets.c:6539` and in
`docs/HANDOFF.md`: general-heap free fell to **6,076** and *the battle never
started*, with every guard passing. I did not build it, and I did not spend a
gate run or a soak on it.

The `.rodata` route is bounded by the same measurement and is unchanged from last
cycle: free-min **53,136** − reserve **32,768** = **20,368 B**, of which 12,288 is
already spent. Neither route buys 287,082 B.

### 3.3 What full two-fighter coverage would cost, since the brief asks

Mario is unconverted. Stage 2's measured both-fighter items-off pack is
**557,670 B**. Against the **1,096,512 B** taskman residue measured above that is
**50.9% of everything taskman has left**, on top of the 287,904 B the o2r blob
already holds, and it does not remove the o2r blob for the three reasons in §3.1.
The reclaim stage 2 names — dropping the 163,840 B raw animation cache once both
fighters are packed — is worth **163,840** against an ask of **557,670**, so it
does not close it either. **Full two-fighter coverage is not a byte problem that
can be solved inside the current arena; it needs the o2r blob to stop being
required, which is §3.1 items 1 and 3, and that is a design change, not a
budget.**

---

## 4. What the existing pair already prices, and the one measurement that decides it

### 4.1 The exchange rate: +69.4 ticks per exchanged call, ~1.59x

Computed here for the first time from `build-c196-trackperf`'s two arms — one
ROM, one poked `volatile` word, so the placement floor is **zero**. Whole match,
1,600 frames.

```text
exchanged calls        29,095 over 1,600 frames        = 18.184 / frame
  early-out            24,197                          = 15.123 / frame
  stepped               4,898                          =  3.061 / frame
measured net           trimmed mean +1,261 tk/fr
NET PER EXCHANGED CALL +69.4 ticks
```

Priced against the generic parser's own per-call cost
(`FTANIM_TRACK_PACK.md` §4, whole-match column, c192 capture):

```text
clock+return per call   (4,025 + 911 + 688 + 12) / 90.24  =  62.5 tk
stepped-exclusive extra  7,300 / 22.66                    = 322.2 tk
generic work removed    15.123 x 62.5 + 3.061 x 384.7     = 2,122 tk/fr
dense work added        2,122 + 1,261                     = 3,383 tk/fr
per call                3,383 / 18.184                    = 186.0 tk
same mix, generic       2,122 / 18.184                    = 116.7 tk
                                                      RATIO 1.59x
```

**Caveats, stated rather than buried.** The block split is from the `c192`
capture — a different HEAD, and the pre-repair fight — so it is a *rate* carried
across arms, which this campaign has been burned by before; and `+1,261` is a
same-binary trimmed-mean delta whose run-to-run floor has never been quantified
for this harness. It is a **derived estimate with named inputs**, not a measured
price, and it is not banked.

### 4.2 The pre-registered prediction for full coverage

Coverage would scale by **2.45x**, derived rather than guessed: 76 of 372 clip
binds are dense, and with both fighters binding and only Fox packed, Fox's own
share is ~186 binds, so 76/186 = **41% of Fox's clip binds are already covered**
and full Fox coverage is 1/0.41 = 2.45x the current dense traffic.

- **if the +69.4 is steady-state ISSUE** (the stepper simply executes more), the
  cost scales with it: **≈ +3,090 tk/fr trimmed mean at full Fox coverage** — full
  coverage makes it strictly worse and the mechanism is refuted.
- **if it is compulsory FETCH of cold rows**, the same rows are re-touched by
  2.45x the calls, the per-call fetch amortises, and the sign can flip.

**These two predictions differ in sign, one v3 capture separates them, and this
cycle did not take it.** That is Task C, and it is now the decision, not a
garnish: it is worth more than any further coverage work, because §3.2 says
coverage is unavailable anyway until the o2r dependency is removed.

---

## 5. What this cycle did NOT do

- **Did not take the Task C v3 capture.** The `build-c196-trackperf` pair is
  *not* usable for it as briefed: a v3 needs `NDS_TASK37_PROFILE=1` baked in and
  c196 was not built with it, so Task C costs **two profiler builds plus two
  captures**, not "no rebuild". The two builds differ only in
  `NDS_R2_FTANIM_TRACK_DISPATCH`, which initialises one `.data` word, so the
  one-byte-pair property is available and `compare-elf-sections.py` should assert
  it. Nothing else about the plan changes.
- **Did not build any full-coverage arm**, did not grow the arena, did not run a
  soak — §3.2 refused it on arithmetic before a build was spent.
- **Did not re-run the oracle**, because no new pack configuration was built.
- **Did not take an `artifacts/visibility` capture.** The owner looked at the ROM
  directly and returned a verdict, which is the acceptance gate the capture
  exists to feed; the capture would have been evidence for a decision already
  made. Stated plainly rather than skipped silently.
- **Did not re-tune Whispy, the BG stretch or the VFX symmetry work** (§2.4).
- **Did not repair `check_ftanim_transcribe.py` / `check_ftanim_target_exact.py`**
  — still RED and unwired, and defect 1 remains exactly what they would have
  caught.
- **Did not touch** `AGENTS.md`, `CLAUDE.md`, `CLAUDE.OPUS.md`, `plan.md`,
  `docs/OPTIMIZE_LIST.md`, `docs/RAM_RECOVERY_PLAN.md`, `decomp/`, or the
  owner's untracked probe scripts.

## 6. State and reproduction

```powershell
# the bank arm
make TARGET=smash64ds-battle-playable-tickhud-hwtri BUILD=build-c199-bank0 `
    NDS_R2_BOTH_CPU=1 NDS_R2_BATTLEPACK=1 NDS_R2_BATTLEPACK_KEEP_CACHE=1 `
    NDS_R2_FIGHTER_GX_COMPOSE_LAB=1
.\scripts\sample-tick-hud-buckets.ps1 -Build build-c199-bank0 -NoBuild -RingDump `
    -Samples 1600 -StartFrame 438 -TimeoutSeconds 3600 -ExtraGlobals <17 names> `
    -RowsCsv .\artifacts\performance\2026-08-15_ftanim-full-coverage\c199-bank0-rows.csv `
    -Json    .\artifacts\performance\2026-08-15_ftanim-full-coverage\c199-bank0.json

# the owner's bore ROM (bore is now the DEFAULT 0; this reproduces the trial)
make TARGET=smash64ds-battle-playable-proof-hwtri BUILD=build-c198-bore0 `
    NDS_FOX_BLASTER_BORE_OFFSET_Y=0 NDS_RENDER_ECONOMY_OWNER_MASK=0
```

**Harness note, paid for twice this cycle:** `sample-tick-hud-buckets.ps1`
requires **pwsh 7** (it uses the ternary operator) and dies with a parser error
under Windows PowerShell 5.1; and launching it through `cmd /c` must use cmd's
own `> log 2>&1`, because PowerShell's `*>` is passed through as a positional
argument and silently binds to `-MelonDS`. Both failures are instant and both
are invisible unless the full log is captured.
