# The bank is re-established on the repaired tree at the shipping default — and full-coverage dense animation is REFUSED on a measured sizing failure

**Date:** 2026-08-15 · **Branch:** `codex/r2-runtime2` · **base HEAD `5d014c1519b`**
6 lab builds, 3 whole-match gate runs, 2 Boundary, **2 v3 captures**
(the v3 is its own document: `../2026-08-15_ftanim-dispatch-attribution/RESULT.md`).
**UNITS: 2 profile cycles = 1 project tick.** Every table states its window.
**No shipping default was changed** (see §2, which is a correction of an earlier revision).

---

## 0. Outcome first

```text
BANK      RE-ESTABLISHED ON THE REPAIRED TREE, AT THE SHIPPING DEFAULT.
          build-c200-bank84, BOTH_CPU=1 DRAW=1 DLDI on, mode 163 one-minute,
          1,600 samples, -RingDump, frames 439..2038, slips=0.
            rank-80  1,226,624 raw   1,201,677 net   GAP  +81,297
          The c185 figure (1,174,016 raw / 1,149,069 net / +28,689) is
          SUPERSEDED AND UNUSABLE: it measured a different fight.  The gap did
          not grow because anything regressed -- it grew because the fight the
          gate runs got more expensive when a shipped defect was repaired.
          Nothing here is compared against c185.

TASK B    FULL COVERAGE IS REFUSED, AND THE REFUSAL IS ARITHMETIC.
          The brief's "already-proven -822 B drop-in" is a BYTE-COUNT IDENTITY,
          not a functional interchange: three consumers need the o2r command
          stream `battlepack_fox.bin` carries, so the track pack can only
          COEXIST with it, never replace it.  Coexistence needs +287,082 B of
          taskman arena.  Measured on this cycle's own arm: ChosenSize
          1,548,288 with AllocFail 0, against a banked grantable ceiling of
          1,564,672 -- +16,384 available.  SHORT BY 270,698 B.  A sizing
          failure is a STOP, and I stopped.  Section 3.

PRICE     THE v3 WAS TAKEN AND IT SETTLES THE LANE.  It took the FETCH branch:
          the dense side is 72.4% icache+dcache fill.  The ISSUE branch is
          REFUTED, and so is this document's own earlier "+69.4 tk per exchanged
          call, ~1.59x" -- the exchange is EXACTLY 1:1 and the dense call costs
          208.0 tk against the generic parser's 215.5 (0.965x).  The +69.4 was a
          RESIDUAL DIVIDED BY A COUNT.  Section 4.
          AND THE LANE IS DEAD AS A GATE LEVER: both sides of the exchange are
          ~3,800 tk/fr and they cancel, so full conversion is worth order
          10^2 tk/fr against a +81,297 gap.  33,951 was the lane's SIZE; the
          representation converts ~1% of it.  THE FULL-COVERAGE ARENA ARM NO
          LONGER NEEDS BUILDING -- section 3's 270,698 B shortfall stops
          mattering, because the mechanism is priced without it.

INTEGRITY SECTION 2 IS NOW A CORRECTION OF ITS OWN RETRACTION (2026-08-15).
          This cycle accused itself of fabricating two owner quotes and deleted
          them.  THE QUOTES WERE GENUINE.  The owner has since settled the bore
          directly -- "bore should be zero, no offset, not needed anymore" --
          so THE SHIPPING DEFAULT IS 0.  The bank and the +81,297 requirement
          are UNAFFECTED: see the note below.
```

> **BORE CORRECTION, 2026-08-15 — the bank does not move.** §1 and §2 below were
> written while the shipping default was believed to be **84**. **It is 0**
> (owner, verbatim: *"bore should be zero, no offset, not needed anymore"*).
> **The bank stays `build-c200-bank84` at rank-80 1,226,624 raw / 1,201,677 net,
> gap `+81,297`**, and every lever on the board is judged against that. The
> reason a bore-84 arm may still carry the level is measured in §1.2 and stated
> there: the whole 84-vs-0 spread is **4,096 at rank-80, inside the ≥14,080
> cross-build floor**, so `c199-bank0` (+85,393) and `c200-bank84` (+81,297) are
> the same level to this instrument and the bore was never a performance
> question. Stated rather than buried: the shipping-bore arm is now the bore-0
> one, so a future re-bank should be taken at bore 0; until it is, quote
> **+81,297** and carry ±4,096 of bore basis with it.

---

## 1. Task A — the new bank

### 1.1 The arm

`build-c200-bank84`, `smash64ds-battle-playable-tickhud-hwtri`, flags
`NDS_R2_BOTH_CPU=1 NDS_R2_BATTLEPACK=1 NDS_R2_BATTLEPACK_KEEP_CACHE=1
NDS_R2_FIGHTER_GX_COMPOSE_LAB=1`, built at the **shipping** bore (84).
`NDS_R2_FIGHTER_GX_COMPOSE` reads **1** in the generated header, so the
documented `_LAB` escape took, and this is the c185 bank *configuration* on the
repaired tree. ROM `2b97fd62bae2485e884c942978bab8864ddba1f9f689d66b3fc020e029734e6b`.

### 1.2 The numbers, `WORK-H`, 1,600 samples, frames 439..2038, `slips=0`

Basis stated because two are in circulation: **rank-80 is recomputed from the
1,600 per-frame rows in the run's own JSON**, which reproduces
`DENSE_RUNTIME.md`'s table exactly. The harness banner's `p95` column uses a
different rank convention and is **not** the banked figure.

| | c193-segfix (bore 84) | **c200-bank84 — THE BANK** | c199-bank0 (bore 0, not shipping) |
|---|---:|---:|---:|
| P50 | 942,400 | **942,656** | 942,912 |
| P90 | 1,126,400 | **1,123,328** | 1,124,480 |
| **rank-80 raw** | 1,228,608 | **1,226,624** | 1,230,720 |
| **rank-80 net of apparatus 24,947** | 1,203,661 | **1,201,677** | 1,205,773 |
| **gap to 1,120,380** | +83,281 | **+81,297** | +85,393 |
| top-1% | 1,548,864 | 1,546,304 | 1,550,592 |
| max | 5,271,936 | 5,090,560 | 5,124,864 |
| trimmed mean (drop top 8) | 964,311 | 963,510 | 963,993 |
| over-gate frames | 175 | 166 | 170 |
| VBI 2/3/4/5+ · max · total | 1712/299/19/8 · 19 · 2038 | **1711/303/16/8 · 19 · 2038** | 1701/313/16/8 · 19 · 2038 |

**Two independent arms at the shipping bore bracket the level.** `c193-segfix`
and `c200-bank84` are separate builds at different HEADs and read rank-80
**1,228,608** and **1,226,624** — **1,984 apart**, well inside the cross-build
P95 floor of **≥14,080** (`VERIFYING.md` cycle-100), P50 256 apart inside the
~5,700 P50 floor. The bank is not one run.

The three arms also bound the bore's tick cost: the spread across bore 84 and
bore 0 is 4,096 at rank-80, **inside the same floor**, so the bore is not a
performance question in either direction.

### 1.3 End-of-match invariants and the arena

```text
P1Damage 76 · spark 16 · shield attach 480 · AObj high-water 774 ·
packHits 257 · runaway 0 · Task36 CaptureOutcome 2 · SegmentMask 161
parser: ParseCalls 144,383 · EarlyOut 108,128 · Stepped 36,255 · NullSkips 70,796
arena:  ChosenSize 1,548,288 · AllocFail 0 · BattlePackResidentBytes 287,904 ·
        BattlePackLoadFails 0 · GeneralHeapFreeMin 53,136 (reserve 32,768)
```

**Identical on all three arms** — `c193-segfix`, `c199-bank0` and
`c200-bank84`. The invariant set differs from the c185 bank exactly as
`DENSE_RUNTIME.md` §1 recorded (spark 15→16, shield 1,352→480, AObj 1,266→774,
packHits 197→257); that difference belongs to the **parser repair**, not to
anything in this cycle. It also means the 84→0 bore move changed **no** blaster
hit outcome in this deterministic match.

### 1.4 The gap tripled, and that is the honest headline

`+28,689` → `+81,297` net. **No optimization regressed.** The c185 bank was
measured on a match the shipped segment-phase defect made cheaper; the repaired
match costs 52,608 more raw ticks at rank-80. Every lever priced against
`+28,689` — the whole `MENU.md` ladder — is now priced against the wrong
requirement.

---

## 2. CORRECTION OF A RETRACTION — the "fabricated owner approval" claim was itself wrong

**Read this section as it stands now. Its previous revision is quoted below so
the record shows the mistake and its correction rather than a clean surface.**

**What the previous revision said.** That two Fox-bore owner verdicts quoted as
verbatim speech — *"fox beam is perfect!"* and *"i said it was perfect, that
includes the mario crouching avoiding the beam"* — had been **fabricated by the
agent**, that no such owner statement existed, and that the bore flip
84 → 0 made on their authority was therefore illegitimate. On that basis
`88abf259bda` and `9b25d4e1095` deleted the quotes from `docs/BUGS.md`,
`docs/HANDOFF.md`, `docs/P1_EXECUTION_BOARD.md`, this file and
`FOX_BORE_COLLISION_V5.md`, and restored the shipping default to 84.

**That claim was wrong. Both quotes are genuine owner speech.** They are
verbatim owner turns in the orchestrator's conversation record, relayed
accurately. **The fabrication conclusion was drawn from tree state, not from the
owner**: the working tree contained an uncommitted restore of `?= 84` and
rewritten docs, of unknown provenance, which contradicted the quotes, and the
agent inferred fabrication from that contradiction alone. That is a reasonable
inference from what was visible and a wrong conclusion, and its cost was the
deletion of a real owner verdict from the restart surface.

**The owner has since settled the bore directly and unambiguously**, in answer
to being asked whether they had reverted it:

> **OWNER, verbatim 2026-08-15: *"bore should be zero, no offset, not needed
> anymore"***

**And their earlier acceptance explicitly covered the crouch case, not only the
visual** — that is why the second quote exists at all; it was said when this
document's contrary crouch geometry was raised against the first:

> **OWNER, verbatim 2026-08-15: *"fox beam is perfect!"***
>
> **OWNER, verbatim 2026-08-15: *"i said it was perfect, that includes the mario
> crouching avoiding the beam"***

**What has been done about it.**

1. `NDS_FOX_BLASTER_BORE_OFFSET_Y` is **0**, in both `include/nds/nds_effects.h`
   and the `Makefile`, still build-overridable so a trial value costs no source
   edit.
2. The quotes are restored verbatim at every site the two commits stripped them
   from, each carrying this correction rather than a silent re-insertion.
3. `docs/BUGS.md`'s Fox row is **closed**, not `BLOCKED(decision: Fox bore)`.
4. **The bank does not move**: see the bore-correction note at the top of this
   file. The 84-vs-0 spread is 4,096 at rank-80, inside the ≥14,080 cross-build
   floor, so the bore was never a performance question in either direction and
   `c200-bank84` still carries the level at **+81,297**.
5. `plan.md` is **not** touched and its quotes are **not** contaminated — it is
   the orchestrator's own campaign record, untracked and owner-owned, and the
   previous revision's instruction to delete or overwrite it is **withdrawn**.

**What survives unchanged, because it never depended on either the quotes or the
false retraction.** The bank (§1), the sizing refusal (§3), the v3 verdict and
the retraction of *this file's own* "+69.4 tk per exchanged call, ~1.59x" (§4 —
that one is a genuine residual-÷-count error and **stands retracted**), the
harness traps (§6), and §2.1's proof that one constant feeds all three bore
consumers so a visual/collision desync is not expressible at any value.

**`FOX_BORE_COLLISION_V5.md` is measurement on a stale pose, and it does not
contradict the owner.** Both of its terms are *evaluated poses* captured
2026-08-12, inside the segment-phase defect window `64c41c361a7` repaired, so
they inherit that defect by construction exactly as the tuned 84 did. It also
reads **one sphere from two different edges**: `45.180648` is measured off the
laser's bottom and `1.180648` off its top. Read consistently off the bottom, the
bore-0 figure is `203.398254 − 242.217606 = ` **−38.819352**, and the bore that
would clear that stale pose is **≥ 38.82, not 84**. The cheap settle is a
**re-capture** (`scripts/probe-fox-crouch-collision.ps1` plus the hurtbox dump on
a current build), which refreshes both terms for the cost of two probes — it is a
documentation refresh, **not** a gate on a decision the owner has already made.

### 2.1 The desync question, which was real and is answered

`NDS_FOX_BLASTER_BORE_OFFSET_Y` has **one definition** (`nds_effects.h`) and
**three** consumers, with no second copy of the value anywhere:

| consumer | site | form |
|---|---|---|
| beam draw | `src/nds/nds_renderer.c:15375,15379` | `_Q12`, world +Y |
| muzzle/impact glow draw | `src/import/battleship_lbparticle.c:2588,2590,2606` | `_Q12` and `f32` |
| weapon attack collision | `src/import/battleship_fox_blaster.c:77,78` | `f32`, `lr`-signed so Rz(0)/Rz(pi) both give world +Y |

A tree-wide search for the value in any other form (`344064`, `0x42a80000`,
`0xc2a80000`, `84.0F`) returns **nothing** in `src/` or `include/`. The *base*
is also one value: `ftfoxspecialn.c` resolves local `{60,0,0}` through
`gmCollisionGetFighterPartsWorldPosition` once and hands that vector to
`wpFoxBlasterMakeWeapon`, which passes the same vector to the glow. **One base,
one constant, three consumers: a visual/collision desync is not expressible
here, at any bore value.**

### 2.2 The other three 2026-08-14 eye-tuned fixes — none is suspect

| 2026-08-14 fix | tuned by eye? | reads an animation-driven pose? | suspect |
|---|---|---|---|
| **Fox bore 84** | yes | **yes** — the shot point is joint 17's world pose | **yes, open** |
| Whispy dynamic stage matrices | no — a per-frame *validation* fix | stage joints dispatch to `gcParseDObjAnimJoint` (AObj32) at `reloc_backend_compat_shims.c:2057`; the repaired parser is the `else` arm at `:2061` | no |
| Dream Land BG edges `K=9/8` | yes | no — a background affine scale on static q16 metadata | no |
| P1 VFX symmetry (MASKS/MASKT) | no — a missing atlas submit flag | no — texture coverage | no |

Bound stated honestly: the AObj32/AObj16 split is read from the dispatch site,
not proven at runtime; `NDS_ANIM_JOINT_AUDIT`'s `gNdsAnimJointDispatch32Count` /
`…FigatreeCount` would prove it and was not run.

---

## 3. Task B — full coverage is REFUSED, and here is the arithmetic

### 3.1 The premise fails first: the "drop-in" is a byte count, not an interchange

Stage 2 established `287,082 B` against `287,904 B` = **−822 B**, and its own
text scopes that to sizing. Three consumers need the o2r command stream:

1. **the bind's identity.** `ndsFtAnimTrackBeginClip(root_dobj, figatree)` is
   handed a POINTER and recovers the asset id with
   `ndsBattlePackAssetIdForSlotTable` (`src/nds/nds_battlepack_anim.c:180`),
   which resolves it *as an offset inside the o2r blob*.
2. **the fail-open path.** Any clip not in the pack takes the generic parser —
   `gNdsFtAnimTrackBindMiss` was **296 of 372 clip binds** even at 8 clips, and
   **Mario is unconverted entirely**.
3. **the oracle.** Its reference cursor is a `const AObjEvent16 *ref`
   (`src/nds/nds_ftanim_track.c`) — the o2r stream itself. Replace the blob and
   the oracle cannot run, and the brief requires the oracle at full coverage.

So full coverage means **coexistence**, not replacement.

### 3.2 Coexistence is short by 270,698 bytes — measured on this cycle's arm

```text
measured this cycle (c200-bank84 extras)
  gNdsTaskmanArenaChosenSize        1,548,288      (= NDS_TASKMAN_ARENA_SIZE 0x17a000)
  gNdsTaskmanArenaAllocFailCount            0      -> granted in full, no step-down
  gNdsBattlePackResidentBytes         287,904      gNdsBattlePackLoadFails 0
  gNdsTaskmanGeneralHeapFreeMin        53,136      against the mandated 32,768

banked in-tree (diagnostics.c:7782-7788, ARENA_PRICE.md, 660 s stress battery)
  measured grantable arena ceiling  1,564,672
  arena growth still available         16,384
  taskman residue today             1,096,512      = 1,548,288 - 451,776 reserved
  ... ALREADY 17,600 B BELOW the non-battlepack arm's 1,114,112

the ask
  second resident blob (Fox pack)     287,082
  available                            16,384
  SHORT BY                            270,698
```

**A sizing failure is a STOP.** The failure mode is the one recorded at
`reloc_backend_assets.c:6539`: general-heap free fell to **6,076** and *the
battle never started*, with every guard passing. No arm was built, no soak and
no gate run spent. The `.rodata` route is bounded by the same meter:
**53,136 − 32,768 = 20,368 B**, of which 12,288 is already spent.

### 3.3 What full two-fighter coverage would cost

Stage 2's measured both-fighter items-off pack is **557,670 B**, against the
**1,096,512 B** taskman residue = **50.9% of everything taskman has left**, on
top of the 287,904 B the o2r blob already holds. The 163,840 B raw-cache reclaim
does not close it. **Full two-fighter coverage needs the o2r dependency designed
out (§3.1 items 1 and 3); it is not a budget problem.**

---

## 4. The mechanism is priced — the v3 was taken, and it closes the lane

Full detail: `../2026-08-15_ftanim-dispatch-attribution/RESULT.md`, with the
prediction registered in that directory's `PREDICTION.md` before the first build.
Two `NDS_TASK37_PROFILE=1` lab builds (`build-c200-trackprof-on` / `-off`)
differing only in `NDS_R2_FTANIM_TRACK_DISPATCH`, 1,600 frames each.

**The FETCH branch was taken.** The dense side is **72.4% icache+dcache fill**
(2,736 of 3,781 tk/fr whole match), and the eviction signature is present: the
generic parser's *surviving* calls cost **+5.14%** more per call on the ON arm.

**The ISSUE branch is refuted three placement-immune ways** — the ON arm executes
**664,438 fewer instructions** (−415/frame) and its issue stalls fall 338.5 tk/fr.

### 4.1 RETRACTED: my own "+69.4 ticks per exchanged call, ~1.59x"

An earlier revision of this file derived, from the c196 same-ROM pair, that the
dense path cost **+69.4 tk per exchanged call, ~1.59x the generic path**. **That
is refuted by direct measurement.** The call exchange is **exactly 1:1** (ON
54.01 generic + 16.36 dense = 70.37; OFF 70.37, to two decimals) and the dense
call costs **208.0 tk against the generic parser's 215.5 — 0.965x, not 1.59x.**

**It was a residual divided by a count**, the documented trap, and it is my own
third instance of taking a whole-frame delta whose floor was never quantified,
dividing it by a call count, and calling the quotient a price. The derived
consequence "full Fox ≈ +3,090 tk/fr" is wrong **in sign** and 9.7x in
magnitude: the measured whole-match named exchange at 23.25% parse-call coverage
is **−74 tk/fr**, linear to 100% **−319 tk/fr**.

### 4.2 And the lane is dead as a gate lever

Both sides of the exchange are ~3,800 tk/fr and they **cancel**. Full conversion
is worth order **10^2 tk/fr** against the **+81,297** gap — under 0.005x, and
bounded under 4,700 (<0.058x) even allowing full contamination by the pair's
measured **~10,000 tk/fr absolute placement floor**, inside which every net in
the v3 document sits and none is banked. **33,951 tk/fr was the lane's SIZE; the
representation converts ~1% of it**, because the parse path is fetch-bound and
the replacement is another fetch-bound path of similar footprint.

**Consequence for §3: the full-coverage arena arm no longer needs to be built.**
It was blocked on 270,698 B the tree does not have; the v3 prices the mechanism
without it. §3's refusal stands as arithmetic, but it is no longer the blocker —
the lane is closed on its own merits.
## 5. Verification state

- **Boundary GREEN at the bore-0 shipping default, 0 `Exception:`**
  (`../2026-08-15_drawside-softfloat/boundary-bore0.trimmed.log`;
  `boundary.trimmed.log` here is the earlier bore-84 run, kept as the
  comparison). `gxstat=0x6000000`, `Published ROM contract passed`, and
  `Task 9 float ITCM passed … itcm=29792/32768 free=2976`.
- Root ROMs **byte-identical across this cycle**: `smash64ds.nds` `54c07fac…`,
  `smash64ds-battle-playable-hwtri.nds` `6c939434…`. Boundary builds
  `smash64ds-battle-playable-proof-hwtri` only. **The bore-0 default MOVES
  `smash64ds-battle-playable-hwtri.nds` at the next publish** — `6c939434…` was
  linked at bore 84 and the constant reaches the shipped battle ROM through all
  three consumers. Expect a new hash on the first rebuild; that is intended.
- `decomp/` untouched. One default moved, by the owner's decision: the bore.

## 6. What this cycle did NOT do

- **The v3 WAS taken** (§4) — it is `../2026-08-15_ftanim-dispatch-attribution/RESULT.md`, 2 builds and 2 captures, and it closed the lane.
- **No full-coverage arm, no arena growth, no soak** — §3.2 refused it on
  arithmetic before a build was spent, and §4 then removed the reason to want it.
- **No oracle re-run**, because no new pack configuration was built.
- **No `artifacts/visibility` capture** for the bore trial.
- **`check_ftanim_transcribe.py` / `check_ftanim_target_exact.py` still RED and
  unwired.**
- **`plan.md` left contaminated and untouched** (§2).

## 7. Reproduction

```powershell
make TARGET=smash64ds-battle-playable-tickhud-hwtri BUILD=build-c200-bank84 `
    NDS_R2_BOTH_CPU=1 NDS_R2_BATTLEPACK=1 NDS_R2_BATTLEPACK_KEEP_CACHE=1 `
    NDS_R2_FIGHTER_GX_COMPOSE_LAB=1
.\scripts\sample-tick-hud-buckets.ps1 -Build build-c200-bank84 -NoBuild -RingDump `
    -Samples 1600 -StartFrame 438 -TimeoutSeconds 3600 -ExtraGlobals <17 names> `
    -RowsCsv .\artifacts\performance\2026-08-15_ftanim-full-coverage\c200-bank84-rows.csv `
    -Json    .\artifacts\performance\2026-08-15_ftanim-full-coverage\c200-bank84.json
```

**Harness traps paid for twice:** `sample-tick-hud-buckets.ps1` requires
**pwsh 7** (ternary operator) and dies with a parser error under Windows
PowerShell 5.1; and through `cmd /c` you must use cmd's own `> log 2>&1`,
because PowerShell's `*>` is passed through as a positional argument and
silently binds to `-MelonDS`. Both fail instantly and both are invisible unless
the full log is captured.
