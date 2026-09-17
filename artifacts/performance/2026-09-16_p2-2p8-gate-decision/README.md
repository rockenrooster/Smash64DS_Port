# P2-2p8: every class is measured. The gate needs an owner decision.

Owner requirement: **30 FPS at four players, non-negotiable** ("that is why I
have you to figure it out"), with **no 30 Hz simulation**.

This is where that stands after measuring every candidate class rather than
arguing about them. It is not a request to lower the bar. It is the evidence
needed to choose between the three things that could still reach it.

## The gate, and the frame

| | tk/fr |
|---|---:|
| gate (two VBlank intervals, locked 30 FPS) | **1,120,000** |
| WORK-H P50 today, four fighters | **1,588,928** |
| **gap** | **-496,382** |

Roster Donkey/Samus/Link/Kirby on Dream Land, 1,972 samples. **This is the
attribution anchor, not a release result** — the contract is every legal
four-fighter lineup on every selectable VS stage
(`p2/native-optimization/16_ALL_ROSTERS_ALL_STAGES.md`), and this campaign has
already seen a bank accepted at -8,096 on Mario+Fox cost **+22,848** at four.

## The frame is not doing too much work. It is waiting for memory.

| non-idle frame | tk/fr | share |
|---|---:|---:|
| instruction issue | 576,491 | 35.7% |
| **memory stall** | **1,039,931** | **64.3%** |

**The instruction-issue floor alone is 543,509 ticks UNDER the gate.** No amount
of arithmetic deletion can close a gap that is entirely stall. That single fact
explains why every lane failed, and they failed *identically* — each traded
issue for fetch:

| lane | deleted | paid | net |
|---|---|---|---:|
| collision ring | issue -1,717 | icache +1,854 | **+64** |
| GX compose bank at four fighters | CPU multiply -18,165 | FIFO traffic | **+22,848** |
| N05.03 cache repair | SRC -15,040 | dcache eviction | **+33,984** |
| SRC bound-domain (sized) | <=57,034 issue | +3,900..7,800 icache | no-go |

## Every class, measured

| class | ceiling | % of gap | status |
|---|---:|---:|---|
| **data locality (theoretical)** | **560,739** | **113%** | only class that exceeds the gap |
| leaf levers (matrix, collision, material, audio, GX) | ~90,000 | 18% | spent, measured |
| renderer streaming repack | 78,000 | 15.7% | compulsory traffic; 227 symbols, none above 3,509 |
| SRC bound pose/transform domain | 70,000 | 14.1% | sized NO-GO; 3 premises absent from the profile |
| **VRAM arena for the object graph** | **55,669** | **11.2%** | **buildable, needs allocator surgery** |
| **deliberate data placement** | **~50,000 observed swing** | **~10%** | **measured 3x; direction not yet controllable** |
| `DObj`/`GObj` field repack | 40,000 | 8.1% | blocked by pristine `decomp/` |
| DTCM placement | 15,040 | 3.0% | works (-10,176) but 1,680 bytes short of finishable |
| per-fighter geometry / joints / animation | 12,144 | 2.4% | spent; skeleton cut aborts the CPU AI |
| 30 Hz simulation | 294,016 | 59% | **withdrawn by owner** |

Nothing implementable reaches 10% except the last two live candidates, and
together they are roughly **21%**.

## The one number that says the gate is physically reachable

Running the match with the ARM9 data cache **disabled** costs +1,394,560
(WORK-H 1,588,928 -> 2,983,488). So the 4 KB cache is already worth more than
the entire gap, and it captures **71.3%** of the available benefit. The residual
is exactly the measured data stall:

| | tk/fr |
|---|---:|
| WORK-H today | 1,588,928 |
| less ALL data-miss stall | -560,739 |
| **perfect data locality** | **1,028,189** |
| gate | 1,120,000 |
| **margin** | **under by 91,811** |

**Perfect data locality clears the gate.** That is a bound, not a plan — a 100%
hit rate is not achievable and the residual is diffuse (renderer 254,344,
unattributed 158,777, object graph 84,304, pose 53,931). But it is the first
time in this campaign that the arithmetic has permitted the answer at all.

## What the placement evidence actually shows

Moving **one 4 KB array** to a 4 KB boundary — no code, no algorithm, only its
address — cost **+49,152 WORK-H**, of which **+46,336 landed in STG**. Third
sighting of the mechanism, second at ~50,000. And uncaching that array returned
**2.2x** what its own access cost predicted, so eviction relief is real and
every locality figure above is a **floor**.

Two readings, and they demand different responses:

- **If placement is tunable**, ~50,000 is available without allocator surgery,
  decomp edits or any fidelity cost — the cheapest 10% on the board.
- **If placement is chaotic**, it is a standing *hazard*: any unrelated edit can
  swing the frame 50,000 ticks, which silently invalidates measurements. Note
  that the 2026-09-16 clean rebuild moved WORK-H by **-50,432** with no source
  change at all — consistent with the same mechanism.

That question is under investigation; it is the difference between a lever and a
liability.

## The three ways to 30 FPS, and what each costs

**1. Data locality, pursued to its limit.** Ceiling 113% of the gap; the two
buildable pieces are ~21% and both are real work — a VRAM arena means
relocating `syTaskmanMalloc` allocations and auditing 59 byte-store sites.
**No fidelity cost.** Needs the placement question answered first, because a
±50,000 chaotic term makes every increment unmeasurable.

**2. Reduced fighter joints.** -207,168 (42%) at 2.48x. **Measured and it is not
a switch**: cap=1 aborts the CPU AI on a NULL joint (`ftcomputer.c:7970`),
because `damage_coll_descs` names joints by id. A smaller skeleton re-derives
the hurtbox table, effect joint ids and every animation binding **per fighter**
— it moves hit-part resolution, so it spends Sacrifice Order **2 and 3**, not 2
alone. Triangles are refuted separately: no CPU work is per-vertex, so deleting
every fighter triangle caps at **-12,144**.

**3. 30 Hz simulation.** -294,016 (59%), the single largest lever measured, and
**withdrawn**. Sacrifice Order ranks the 60 Hz simulation (4) as more protected
than gameplay (3), so joints are the cheaper sacrifice by the project's own
ordering — but joints cost gameplay fidelity in a way the owner has not yet
been asked to price.

## What is being asked

Only one of these needs an owner decision now; the other two are engineering.

**If 30 FPS at four players is to be reached without touching fidelity**, the
path is data locality and it needs the placement question settled, then the VRAM
arena built — a multi-week structural change with an 11-21% measured yield and a
113% theoretical ceiling. That is the honest recommendation: it is the only path
that costs nothing the project has said it values.

**If that yield proves insufficient** once placement is understood, the decision
is between reduced joints (gameplay, Sacrifice Order 2+3) and the withdrawn
30 Hz simulation (order 4). The project's own ordering prefers joints. Nothing
else measured comes close.

**What should not happen** is another leaf lane. Eleven have now been measured
and the arithmetic that says they cannot sum to 496,382 is closed.
