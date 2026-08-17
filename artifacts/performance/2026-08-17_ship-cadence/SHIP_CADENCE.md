# The shipping configuration is measured on both arms: cadence 95.20% on the stress gate and 98.38% on the shipped Boundary, the tick arm re-banks to +16,209, and the whole banked basis carries a lab flag the ROM does not

**Date:** 2026-08-17 · **Branch:** `codex/r2-runtime2` · **HEAD `87056b30353`**
**5 lab builds** (`build-c241-shipcadence`, `build-c242-shipexact`,
`build-c244-shipboundary`, `build-c245-pubgate`, `build-c246-tickship`),
**6 emulator runs**,
**0 production source edits, 0 defaults flipped, nothing published.**
**Both root ROMs SHA-256 restored and verified `887D82FA…9853` / `54C07FAC…C68A`.**
**UNITS: 1 project tick = 1 `cpuGetTiming()` tick = 2 ARM9 cycles.**

`docs/P1_EXECUTION_BOARD.md:111-117` and `CADENCE_ARM.md` §7 named this run as
the next cycle's step 0: *"Settling it needs a `NDS_TICK_HUD=0` proof ROM and a
reader for `gNdsBattlePlayablePacingPresentIntervalBucket[]` that does not
depend on the tick HUD."* Both already existed. `PREDICTION.md` in this
directory was written before any measurement; it predicted 95.4% (range
94.6–96.3) for the proof ROM and measured 95.69%.

---

## 0. Outcome

```text
THE READER      scripts/probe-present-cadence.ps1, already in the tree.  It
                reads the guest's own cumulative histogram over GDB at
                ndsBattlePlayableFrameCompleteMarker and has no tick-HUD
                dependency of any kind.  The bucket increment
                (taskman_seam.c:5069), the publish (:5251) and
                NDS_PUBLISH_DEBUGGER_GROUP (nds_platform.h:212-216, a bare
                DC_FlushRange) are all UNCONDITIONAL -- so the counter exists,
                is flushed, and is readable in the PUBLISHED ROM itself.  No
                proof-ROM proxy was needed in the end.

CONTROL         The same reader on build-c240-cadence-draw0 reproduces the
                banked figure: 3/4/5+ = 95/7/2 and max 19, IDENTICAL to
                CADENCE_ARM.md.  Its 2-bucket reads 1,939 against the banked
                1,935 because this reader's window closes at Results entry
                while the sampler's closed at frame 2,039 -- all four extra
                frames are 2-VBlank.  A control that could have failed.

                Free second result: the banked run took 2,038 per-frame GDB
                stops, this one takes 3, and the slip counts are identical.
                Halting melonDS does not perturb pacing.  That claim has been
                asserted in the probe's header since it was written; it is now
                measured.

THE BASIS       A DEFECT IN THE BASIS, NOT IN THE ROM.  Every banked figure of
                this campaign -- c237's +26,449 requirement, c239's -44,544,
                c240's 94.90% -- was built with NDS_R2_BATTLEPACK=1
                NDS_R2_BATTLEPACK_KEEP_CACHE=1.  The published ROM has
                NEITHER.  Worth 13 frames of cadence.

MEASURED        Whole match, mode 163, one minute, DLDI on, denominator = the
                guest's own presented-frame counter at Results entry:

                  c240  tick-HUD DRAW=0, pack on,  stress    1,939/2,043 94.91%
                  c241  proof TICK_HUD=0, pack on, stress    1,955/2,043 95.69%
                  c242  proof TICK_HUD=0, SHIPPING, stress   1,942/2,043 95.06%
                  c245  PUBLISHED TARGET,  SHIPPING, stress  1,945/2,043 95.20%
                  c244  proof TICK_HUD=0, SHIPPING, BOUNDARY 2,010/2,043 98.38%

                Every arm: viol=0, max interval 18 (19 on the tick-HUD arm).

ISOLATIONS      tick-HUD apparatus  +16 frames  (the board modelled 13)
                battlepack flags    +13 frames  (lab only; does not ship)
                ship telemetry       +3 frames  (proof ROM -> published ROM)

THE TICK ARM    RE-BANKED AT THE SHIPPING DEFAULTS AND IT FAILS.  c246 is
                c239 with the two pack flags off: rank-80 1,161,536 raw /
                1,136,589 net, REQUIREMENT +16,209 against c239's -18,095.
                The pack is worth -34,304 at rank-80 (2.44x the >=14,080
                floor) and its over-gate delta is +13 frames -- the SAME 13
                the cadence isolation found on a different instrument.

THE VERDICT     THREE ANSWERS, AND ALL THREE MATTER.

                THE SHIPPED CONFIGURATION -- what the owner plays, Mario human
                vs level-3 Fox CPU -- holds two-VBlank cadence on 98.38% of a
                whole match, dropping 33 frames in 2,043.  That is 69 frames
                INSIDE the >=95% bar, not near it.

                THE BOTH-CPU STRESS GATE the owner set on 2026-08-05 reads
                95.20% on the literal published battle configuration: 4 frames
                over the bar on 2,043, measured on the published target with
                no proxy and no modelling.  It CLEARS, and it clears narrowly.

                THE TICK ARM FAILS BY +16,209 at the shipping defaults, and
                the two arms disagree because they are read over DIFFERENT
                POPULATIONS -- cadence over all 2,043 presented frames, ticks
                over the 1,600-frame gameplay window.  The plan's own wording
                ("the whole match ... loading states excluded") is the 1,600.

                The call is the owner's, not an agent's.
```

---

## 1. Why the published ROM could be measured directly

The plan was a proof-ROM proxy. It turned out not to be necessary.

`smash64ds-battle-playable-proof-hwtri` is pinned at `NDS_TICK_HUD := 0` and
`NDS_SHIP_TELEMETRY := 1` (`Makefile:1890-1896`); the published battle ROM is
`NDS_TICK_HUD := 0` and `NDS_SHIP_TELEMETRY := 0` (`Makefile:1663-1664`).
`NDS_SHIP_TELEMETRY` only *adds* work — the `cpuGetTiming()` pairs bracketing
draw and HUD (`taskman_seam.c:4890-4913`), `ndsRendererProfileFrameBegin`
(`:4671`), `ndsRendererProfileFramePublish` (`:5022`) — so a proof-ROM reading
is a strict lower bound.

But the histogram itself is not conditional. `taskman_seam.c:5069` increments
the bucket in every configuration; `ndsBattlePlayableFinalizePresentedIteration`
calls `ndsPlatformPublishBattleFrameCompleteGroups()` (`:5251`) with no `#if`
around it; and the macro that publishes is a bare `DC_FlushRange`
(`nds_platform.h:212-216`). `ndsBattlePlayableFrameCompleteMarker` is
`__attribute__((noinline, used))`, so it survives `-ffunction-sections` +
`--gc-sections`.

Therefore `build-c245-pubgate` is `TARGET=smash64ds-battle-playable-hwtri` —
the published target, all of its own overrides — with **`NDS_R2_BOTH_CPU=1`
and nothing else** on the command line. Its generated `nds_build_config.h`
differs from `c242`'s by exactly one line, `NDS_SHIP_TELEMETRY 1 → 0`.
**That is the gate configuration, not a stand-in for it.**

Its ROM and ELF land in the project root, not in the build directory
(`scripts/lib/build-output.ps1` treats the two published targets specially),
which is why the run had to be taken before the root ROM was restored.

---

## 2. The measurements

All five arms use the identical invocation shape — `-Hits 1
-EndBreak mnVSResultsStartScene`, i.e. **three GDB stops for a whole match**:
one at the first frame-complete, one at the second, one after the battle loop
has exited into `mnVSResultsStartScene`, where the cumulative histogram is
read. DLDI on (pinned by `Set-MelonDSAutomationProfile`), DS console mode,
JIT off, `LimitFPS=false`.

| build | target | `TICK_HUD` | `SHIP_TELEM` | pack | `BOTH_CPU` | 2 | 3 | 4 | 5+ | presented | two-VBlank | max |
|---|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| `c240-cadence-draw0` | tickhud | 1 (`DRAW=0`) | 0 | 1 | 1 | 1,939 | 95 | 7 | 2 | 2,043 | 94.91% | 19 |
| `c241-shipcadence` | proof | 0 | 1 | 1 | 1 | 1,955 | 82 | 4 | 2 | 2,043 | 95.69% | 18 |
| `c242-shipexact` | proof | 0 | 1 | **0** | 1 | 1,942 | 90 | 9 | 2 | 2,043 | 95.06% | 18 |
| **`c245-pubgate`** | **published** | **0** | **0** | **0** | **1** | **1,945** | **88** | **8** | **2** | **2,043** | **95.20%** | **18** |
| `c244-shipboundary` | proof | 0 | 1 | **0** | **0** | 2,010 | 30 | 1 | 2 | 2,043 | **98.38%** | 18 |

`viol=0` on every arm — no presented interval below two VBlanks anywhere, in
any match. `presented` is the guest's own
`gNdsBattlePlayablePacingPresentedFrames`, read after the battle loop exited,
so the denominator is the whole match rather than a sampler window. ≥95% of
2,043 requires 1,941.

### 2.1 Three isolations, each one flag wide

- **`c240 → c241`** — instrument only. **+16 frames.** The board's model
  (subtract a uniform 24,947 from c240's rows and re-count against the
  bracketed boundary) predicted **13**. Right in kind, 19% low in size.
- **`c241 → c242`** — the generated configs differ by exactly the two
  battlepack lines. **−13 frames.**
- **`c242 → c245`** — the generated configs differ by exactly
  `NDS_SHIP_TELEMETRY`. **+3 frames**, and in the direction §1 requires.

Sum check: 1,939 + 16 − 13 + 3 = 1,945. Measured 1,945.

### 2.2 The stress arm is the whole difference between "at the bar" and "clear"

`c244` is `c242` with `NDS_R2_BOTH_CPU 1 → 0` and nothing else — the generated
config diff is that one line. **+68 frames**, 95.06% → 98.38%. The shipped
Boundary configuration drops 33 frames of cadence in a 2,043-frame match, of
which one is presented frame 2 (the stage's and fighters' first full draw,
`CADENCE_ARM.md` §6). The owner's decision to gate on the stress arm is
carrying essentially the entire remaining risk.

---

## 3. The basis defect, stated plainly

`NDS_R2_BATTLEPACK` is the Native Battle Kernel slice-1 figatree pack.
**Every line number and quotation in this section is the tree AS MEASURED,
before the owner's flip in §5 rewrote both comment blocks.** At measurement
time `Makefile:360-361` was explicit: *"The default stays 0: turning the pack on
by default is the owner's call, not a build flag's."* The board's
**"PHASE 8 IS DONE"** section closes with **"`NDS_R2_BATTLEPACK` stays default
0. No flip proposed."**

Yet the reproduce line of every recent measurement artifact carries
`NDS_R2_BATTLEPACK=1 NDS_R2_BATTLEPACK_KEEP_CACHE=1` — `SHIPPING_REBANK.md`,
`ITCM_REPACK2.md`, `CADENCE_ARM.md`, `CAMERA_SHIP.md`, `ANIM_ITCM.md`,
`HWMATH.md`, `FTR_LANE.md`, `CAPTURE_MEMO.md`, `BASIS.md`. The convention was
adopted as "the gate arm" and never re-checked against what ships.

Three independent proofs that the published ROM does not carry it:

1. `Makefile:326` `NDS_R2_BATTLEPACK ?= 0` and `:362` `..._KEEP_CACHE ?= 0` are
   the only assignments in the file; the published block
   (`Makefile:1653-1712`) never overrides them.
2. `strings` on the root `smash64ds-battle-playable-hwtri.nds` finds **0**
   occurrences of `battlepack_fox`; the same tree built with the flag finds 2.
3. Size: root 12,250,112 B against `c241`'s 12,533,760 B — a difference of
   283,648 B against the blob's own 287,904 B.

**Consequence.** The banked cadence arm was **0.49 points optimistic** relative
to the published ROM (95.69% vs 95.20%), and the banked **tick** arm's
`−18,095` margin (`ITCM_REPACK2.md`) was taken on the same flag — §4 re-measures
it at the shipping defaults and it becomes **+16,209**. Sizing a candidate
against `+26,449` was sizing it against a ROM nobody ran.

---

## 4. The tick arm re-banked at the shipping defaults — and it FAILS by +16,209

`build-c246-tickship` is `build-c239-itcm-repack2` with the two battlepack
flags off and nothing else (the generated headers differ by those two lines
plus the git stamp, which advanced only over docs commits). Same instrument,
same window, same 1,600-sample `-RingDump` at `-StartFrame 439`, DLDI on,
`NDS_R2_BOTH_CPU=1`. ROM SHA-256 `DECCB900…`.

| | `c239` pack **on** (banked) | **`c246` pack off = SHIPPING** | delta |
|---|---:|---:|---:|
| P50 | 841,024 | 844,992 | +3,968 |
| P90 | 1,027,776 | 1,046,272 | +18,496 |
| **rank-80 raw** | 1,127,232 | **1,161,536** | **+34,304** |
| **rank-80 net** | 1,102,285 | **1,136,589** | **+34,304** |
| band 41–120 | 1,130,086.4 | 1,165,102.4 | +35,016 |
| top-1% | 1,396,288 | 1,428,864 | +32,576 |
| max | 1,812,672 | 1,928,320 | +115,648 |
| over gate, of 1,600 | 88 | **101** | **+13** |
| **REQUIREMENT vs 1,120,380** | **−18,095** | **+16,209** | |

`artifacts/performance/2026-08-17_ship-cadence/rebank.py` reproduces **every**
banked `c239` figure from that run's own rows before it is pointed at `c246` —
P50 841,024, P90 1,027,776, rank-80 1,127,232 / 1,102,285, band 1,130,086.4,
top-1% 1,396,288, max 1,812,672, over-gate 88, requirement −18,095. It is
checked against the bank, not asserted.

**+34,304 is 2.44× the ≥14,080 cross-build placement floor**, so this is a real
cost, not a link shuffle. And **the over-gate delta is +13, the same 13 frames
the cadence isolation in §2.1 measured on a completely different instrument.**
Two instruments, two metrics, one number.

**Equivalence: 19 of 21 retained counters bit-identical to `c239`** — Selected
/Submitted 62,952, LightDirection 4,010, BoundsPass/Fail 3,823/142, P0/P1
600,000/623,934, StageFighter 1,223,934, the whole draw memo, GxCompose Roots
62,952 / Declines 0. Two moved, and both are explained rather than waved at:

- `gNdsParticleCameraCacheHitCount` 4,324 → 4,323 — one hit.
- `gNdsTaskmanGeneralHeapFreeMin` 53,136 → **70,736, exactly +17,600.**
  `Makefile:345-347` predicted that number before this run: *"1,548,288 −
  451,776 leaves taskman 1,096,512 against the shipping arm's 1,376,256 −
  262,144 = 1,114,112, i.e. **17,600 B LESS**."* A control that could have
  failed, to the byte.

### 4.1 The two gate arms now disagree in sign, and the reason is the population

They are not measured over the same frames:

```text
cadence arm   ALL 2,043 presented frames, entry window included
              c245 published: 98 over the boundary -> 95.20%   PASSES

tick arm      the 1,600-frame gameplay window, frames 440-2039
              c246 shipping:  101 over the gate   -> 93.69%    FAILS by 16,209
```

The ~443 entry frames are almost entirely 2-VBlank (`CADENCE_ARM.md` §3 read
that window at 97.49%), so including them lifts the rate by roughly 1.5 points.
**`Smash64DS_Runtime2_SwitchPlan.md` §7 says "the whole match … loading states
excluded", which is the 1,600-frame window, not 2,043.** On the plan's own
wording the tick arm's population is the right one and the gate is **not met**
at the shipping defaults.

> **OWNER RULING, 2026-08-17, asked and answered: the gate population is ALL
> 2,043 PRESENTED FRAMES, not the 1,600-frame gameplay window.**

**On that population the shipping configuration reads 95.20% two-VBlank and the
gate PASSES by 4 frames** (`c245`, the published target, both-CPU stress arm).
The 1,600-frame rank-80 figure keeps its job as the **sizing** basis — it is
what a candidate lever is priced against — but it is no longer the verdict.
Label which of the two any figure is.

---

## 5. What is actually available, and it is a decision rather than engineering

The single largest lever against the +16,209 requirement is
**`NDS_R2_BATTLEPACK=1`**, worth **−34,304 at rank-80** — 2.1× the requirement,
turning +16,209 into −18,095 — plus 13 frames of cadence.

Its *"LAB ONLY, NOT SHIPPABLE AS CONFIGURED"* label was **withdrawn 2026-08-15**
(`Makefile:340-351`, `artifacts/performance/2026-08-15_battlepack-arena-price/ARENA_PRICE.md`)
after two of its three premises turned out to be wrong, and it was measured on
a stress battery rather than projected: **12 battle entries, 7 matches, 7 START
restarts, 4 Sudden Deaths, NO-FREEZE**, `ChosenSize` 1,548,288, `AllocFail` 0,
`ReserveFail` 0, `Rejects` 0, `SyMallocOverflow` 0, general-heap low-water
52,400 against the 32,768 floor, GObj cap never firing. This run adds the
independent confirmation: heap low-water 53,136 with the pack against 70,736
without, i.e. the pack costs **17,600 B** of general heap and no more.

The only thing holding the default at 0 was `Makefile:360-361`: *"turning the
pack on by default is the owner's call, not a build flag's."*

> **OWNER DECISION, 2026-08-17: FLIP IT, BUT SOAK IT FIRST.** The default moves
> to 1 only after a fresh freeze soak and a full Boundary on the flipped
> default are run and reported. **R2-08 (`NDS_R2_PATH=1`) is HELD until the
> gate settles** — same exchange.

**What this cycle changes about that decision: the campaign has been implicitly
banking the pack for weeks.** Every requirement, every candidate size and both
gate arms were measured with it on. Flipping it does not *find* 34,304 ticks —
it stops the basis and the ROM disagreeing.

---

## 6. What this does NOT say

- **It does not declare the P1 gate passed.** It measures the cadence arm at
  **95.20%** on the published battle configuration under the owner's stress
  arm — 4 frames over the bar on 2,043 — and at **98.38%** on the shipped
  Boundary arm. The `≥95%` verdict is the owner's.
- **It does not price the pack as a candidate.** §5 measures what the flag is
  worth; whether it can ship is the decision `Makefile:360` reserves for the
  owner.
- **It does not touch the visual, stability, or retail arms** of the switch
  plan's §6 acceptance list.
- **It does not build, flip, or publish anything.** No production source was
  edited and no default changed. Both root ROMs are byte-identical to the
  hashes in `docs/HANDOFF.md`, restored from a pre-run backup after the
  `c245` run, which by construction writes the published target into the
  project root.
- **One side effect is recorded rather than hidden:** the root
  `smash64ds-battle-playable-hwtri.elf` is now the `c245` lab link, not the
  link that produced the restored `887D82FA…` ROM. Nothing tracked depends on
  it (`.gitignore:10`), and the next no-override
  `make TARGET=smash64ds-battle-playable-hwtri` regenerates the pair — with a
  new hash, because `NDS_TASK10_GIT_SHORT` has advanced.

---

## 7. Recurrence

The root cause is that a measurement build's flag set is chosen on the make
command line and nothing compares it to the published target's. Every build
already emits its full resolved flag set to `nds_build_config.h`, so the check
is a diff, not an investigation. **Actionable item recorded on the board:** a
`scripts/check-shipping-basis.ps1` that resolves the published target's config
header into a throwaway build directory and fails on any divergence outside a
named instrument allowlist (`NDS_TICK_HUD`, `NDS_SHIP_TELEMETRY`,
`NDS_RENDERER_PROFILE_LEVEL`, `NDS_R2_BOTH_CPU`, the census flags). It was not
written in this cycle only because resolving the config-only make goal needs
the Makefile's own `$(CURDIR)` spelling, which is a build-system change and
not in this cycle's scope.

---

## 8. Reproduce

```powershell
make TARGET=smash64ds-battle-playable-hwtri BUILD=build-c245-pubgate NDS_R2_BOTH_CPU=1

pwsh -NoProfile -File scripts\probe-present-cadence.ps1 `
     -Build build-c245-pubgate -Target smash64ds-battle-playable-hwtri `
     -Hits 1 -EndBreak mnVSResultsStartScene -TimeoutSeconds 2700 -RunnerSlot 6 `
     -Artifact artifacts\performance\2026-08-17_ship-cadence\c245-cadence.txt
```

**Back up the two root ROMs first** — that build writes the published ROM into
the project root — and restore them afterwards.

The other arms are the same two commands with
`TARGET=smash64ds-battle-playable-proof-hwtri` and
`BUILD=build-c242-shipexact` / `build-c244-shipboundary`
(`NDS_R2_BOTH_CPU=1` / omitted), which write into `builds/` and leave the root
alone.

Then, needing no emulator, from the repo root:

```powershell
python artifacts\performance\2026-08-17_ship-cadence\cadence.py
```
