# Decisions only you can make — 2026-08-13

Everything an agent could measure, refute or price without you has been done.
This is the list of things that are now waiting on your call, in plain language,
each with what it costs, what it risks, and what happens the moment you answer.

**Where the gate stands.** The stress arm (`NDS_R2_BOTH_CPU=1`, both fighters
level-3 CPU, 60-second Dream Land match) reads **1,207,616** ticks on the frame
that decides the score. The budget is **1,120,380**. So it is **87,236 over**.

**But roughly 25,000 of that is the stopwatch, not the game** — see decision 8.
On the shipped ROM the real gap is closer to **62,000**.

**Nothing below is urgent in the sense that the game is broken.** The
one-fighter-CPU configuration you actually play (`Boundary`) **passes** at
1,087,616. The 87,236 is the harder both-CPU arm.

---

## 1. Whispy — which motion looks wrong to you?

**What's being asked:** name the motion. Blink, eye turn, mouth, or the wind gust.

**Why it's stuck:** every measurement says the port is doing exactly what the
original does. The blink runs **6 presented frames** (animation frames
1,3,5,7,9,11), and the eye's model node squashes its vertical scale
0.948 → 0.104 → 1.0 through the original's own transform kind. There is **no
blink texture in the source and that is correct.** Four independent probes all
came back "source-exact", so an agent cannot tell which motion you mean by
"looks low FPS" — and guessing would mean changing something the original does
on purpose.

**Cost of answering:** one sentence from you. Cost of not answering: the row
stays open, and nobody can work it.

**What happens on your answer:** the agent goes straight to that one motion's
update cadence and either raises it to 30 Hz or explains in numbers why it is
already there.

**One thing changed since you last looked, and it is NOT Whispy:** a separate
animation-freeze bug was found and fixed on 2026-08-13 — a **fighter's** joints
(Mario's, during and after shielding) were being dropped and frozen in their last
pose, 144 times in a five-minute match. That is the same *kind* of thing "looks
low FPS" describes, on a different object. If what you were seeing was ever the
fighters rather than the tree, it may now be gone; if the tree still looks wrong
after that fix, the question above stands unchanged and still needs your sentence.
`artifacts/performance/2026-08-13_c-animjoint-fix/ANIMJOINT_FIX.md`.

**Evidence:** `artifacts/verification/2026-08-12_whispy-{cadence-armed,channels,xobj-kinds}.txt`

---

## 2. Fox — the laser really does sit below the barrel in the original

**What's being asked:** accept it, or approve a cosmetic nudge.

**What we found:** the shot and the muzzle flash spawn **23.651 world units
below the centre of the gun barrel**, and that number comes entirely out of
BattleShip's own data — the spawn offset, the gun's vertices, and the beam
quad's vertices. **63.8% of the beam hangs below the barrel in the original game
too.** Only 51% of the beam overlaps the gun's face. Nothing in the port moved it.

**Your two options:**

| | what it means | gameplay | risk |
|---|---|---|---|
| **A — leave it** | the beam draws exactly where SSB64 draws it | unchanged | none |
| **B — cosmetic nudge** | a draw-only `(0, −24, 0)` offset in the gun joint's local space, applied to the beam quad and the flash quad only | **unchanged** — the projectile's real position, its hitbox and its collision are untouched | it moves a telegraph the original defines, so the beam would no longer line up with the source. Costs one vector add per shot drawn. |

An agent must not pick B on its own — that is the rule about not changing
source-defined presentation without you.

**If you pick A**, the remaining question is whether the *pose* of Fox's arm is
being updated at 30 Hz when it should be 60, which is a separate and cheaper
investigation the agent can run next.

**Also worth knowing:** you asked whether you could duck the beam as Mario.
Because the sag is source-exact, if you cannot duck it in Smash64DS you could
not duck it in SSB64 either.

**Evidence:** `artifacts/bugs/2026-08-12_fox-crouch/BEAM_QUAD_ANCHOR.md`

---

## 3. Unfreeze the collision maths for a fixed-point rewrite?

**What's being asked:** permission to convert one specific cluster of collision
floating-point maths to fixed point.

**Why:** the biggest single lump of remaining cost is a chain of transforms that
fires when the two fighters are close enough to actually test hitboxes. On the
88 frames a match where that happens, it costs **67,230 ticks a frame — 42.3% of
what makes those frames expensive — and 65% of that is software floating point**
the DS has no hardware for.

**Why it's frozen:** floating-point code in the collision and fighter-main files
is currently under a standing rule that only allows exact transformations
(caching a value, hoisting it, deleting it) — never rewriting the maths. That
rule exists because a rounding difference there can flip a hit into a miss.

**The honest size:** to be worth 16,000 on the score, this needs **47,424 ticks a
frame** off those 88 frames — about **35% of the whole chain**. There is nothing
left to cache: the chain already computes each value exactly once per joint per
frame. So the only way to 35% is the rewrite.

**Risk:** this is the highest-risk item on the page. A fixed-point collision
rewrite changes hit decisions if it is wrong, and the previous attempt in this
class (L7) taught that such a change has to beat its own memory footprint before
it pays anything.

**What happens on approval:** the agent builds the replacement against an error
bound (the same method that settled the animation cubic — a host-side sweep
proving the worst deviation is smaller than hitbox scale), not against
bit-exactness. The design is already written up in
`docs/optimization/OPTIMIZATION_IDEAS.md` Phase 4.

**Evidence:** `artifacts/performance/2026-08-13_shdt-band-owner/BAND_OWNER.md`

---

## 4. Visual trades — three of them, priced

Each of these buys speed by removing something you can see. None will be taken
without you. Sizes are ticks off the deciding frame.

| | what disappears | measured | notes |
|---|---|---:|---|
| **4a — the depth-free stage band** | the background/foreground scenery cards Dream Land draws without depth testing | **−22,510** | the largest single cosmetic item anywhere. A *partial* version exists: a build-time list that drops the 16 cheapest cards — 36 triangles, 20.6% of the stage's geometry — worth **up to ~4,600**. |
| **4b — the particle draw half** | every particle and every Whispy leaf in the game stops drawing | **−30,676** (−33,818 including the update side) | the biggest number here, and the most visible loss. |
| **4c — Whispy's mouth only** | one stage element stops drawing during play; measured at **117 changed pixels out of 49,152**, which passed the project's own 500-pixel "barely visible" bar back in July | **≈ −2,600 today** | the July measurement said −7,104, but that was taken before the stage got 2.7× cheaper, so it is worth far less now. Cheap to try, small payoff. |

**A note on 4b:** quarter-rating particles instead of deleting them was
investigated this week and is **not available** — the particle update draws
random numbers from the same generator the level-3 CPU uses, so changing how
often particles update changes what the AI does, and the match diverges.

**Evidence:** `RESIDUE.md` §6 rung 3; `../2026-08-13_c-particle-rate/`;
`docs/PERF_LEDGER.md` (Task 11 economy rows)

---

## 5. Re-enable the fighter GX compose path? (needs a bug fixed first)

**What's being asked:** whether an agent should spend a cycle fixing the defect
that keeps a measured **−13,632** switched off.

**Background:** this path hands the fighter's joint maths to the DS graphics
hardware instead of the CPU. It measured −13,632 and frame-locked screenshots
were pixel-identical. It was withdrawn because **you bisected a periodic
one-frame fighter disappearance to it**.

**We now know the mechanism:** the hardware's matrix stack pointer leaks about
**3 pushes per frame** and wraps around every 32 frames — which is exactly a
periodic one-frame glitch. That is a findable, fixable bug, not a mystery.

**Risk:** medium. It is a correctness fix an agent can attempt and prove with a
counter (stack depth constant across a whole match), but the final acceptance is
your eye on a real ROM, because the symptom is something only you saw.

**Cost:** roughly one cycle, one or two builds. Payoff **−13,632** — under the
16,000 bar on its own, but it is the single largest already-measured number that
is currently switched off.

---

## 6. The one that closes the gate by itself — 30 Hz simulation

**What's being asked:** written approval to build a **compensated 30 Hz
simulation**.

**Size: −119,744.** That is 1.37× the whole 87,236 gap. **It is the only
remaining change measured large enough to close the gate alone.**

**What "compensated" means:** the game logic would run 30 times a second instead
of 60, with timers, physics and animation advanced two steps each time so that
the *outcome* matches. Done well, it plays the same. Done badly, it feels
different — and "feels different" is exactly what this project is trying not to
be.

**Where this sits in your own priority list:** `PROJECT_GOAL.md` ranks what may
be sacrificed as audio → visuals → gameplay → the original 60 Hz simulation →
stable 30 FPS. So the 60 Hz simulation is the **last thing** to go before the
frame rate itself, and the document says stable 30 FPS is the most protected
requirement. The **−119,744 already measured** is the *uncompensated* version
(the game literally plays at half speed); it is a size, not a proposal.

**Risk:** the highest of anything on this page in terms of feel, and the largest
in scope. It is also the only one that finishes the job.

**This needs your approval in writing**, per the plan of record.

---

## 7. Cheap re-prices — are they worth a build?

Two default-off switches carry positive measurements taken on windows the
project's own rules now class as unusable. Neither is likely to matter; both
would cost a build to say so for certain.

- **Shield as a flat quad instead of the source model.** You bought the model
  route on 2026-08-04 with *"36k p95 is worth it for correctness"*. That 36k came
  off a 128-frame window, and the project now only trusts whole-match numbers.
  **Recommendation: skip.** The shield is only on screen while someone shields,
  so it cannot move the deciding frame much whatever it costs.
- **Whispy's mouth cut (4c above).** Same situation, and it is already listed as
  a visual trade.

**No action needed unless you want the numbers.**

---

## 8. Should the gate be scored net of the stopwatch?

**What's being asked:** a scoring question, and it is worth about **25,000**.

The ROM we measure on is not the ROM you play. To report a per-frame tick count
at all, the measuring ROM has to read a timer many times a frame and print a
small amount of text. Those reads land **inside** the number being reported.
Three separate pieces of it are now identified:

| what it is | ticks per frame |
|---|---:|
| timer reads (`cpuGetTiming` + `tickGetCount`), 175 a frame | 14,691 |
| the debug/HUD text path | 3,984 |
| a per-graphics-command test that only exists in the measuring build (**found today**) | 6,272 |
| **total measuring apparatus inside the reported number** | **≈ 24,947** |

**The published ROM executes none of it.**

**Option A — score the gate net of the apparatus.** The gap becomes ≈62,000
instead of 87,236 and every future number is compared the same way. Nothing
about the game changes; the honest distance to done gets shorter.

**Option B — slim the instrument instead.** An agent spends one cycle making the
measuring ROM cheaper, so the reported number naturally drops toward the real
one. The catch: **changing the instrument invalidates every banked figure** —
every past measurement was taken on the current instrument, and they stop being
comparable. That is a real cost; this project has already lost a week once to a
mislabelled measurement arm.

**Option C — change nothing.** Keep scoring as today and accept that the
published ROM is roughly 25,000 better than its own scoreboard says.

These are stated neutrally on purpose; all three are defensible and the choice is
about how you want progress reported, not about the game.

**Evidence:** `RESIDUE.md` §5;
`../2026-08-13_c-flagsweep/gxrecord-and-texvalid-owner-line.txt`

---

## 9. The push is still blocked — untrack `decomp/` or scrub 16 files?

**What's being asked:** which of two fixes you want.

**The problem:** 16 tracked files under `decomp/` are Rust build artifacts that
contain your name in a file path. The project's rule is that your name must not
appear in tracked files, so **pushing to GitHub is blocked**. There is also a
documentation mismatch: `AGENTS.md` says `decomp/` is ignored by git, and it is
not — 26,276 files under it are tracked.

**Local commits are unaffected** and have been continuing normally.

**Your two options:**

- **Untrack `decomp/` entirely** — matches what the documentation already claims,
  removes 26,276 files from the repository, and the reference sources are
  re-fetchable with the existing script.
- **Scrub the 16 files** — smallest change, leaves the documentation mismatch in
  place.

You deferred this on 2026-08-12 ("ignore for now"), so it is repeated here only
because it is still the one thing standing between the work and a push.

---

## 10. What does *not* need you

So the list above is not read as "everything is blocked":

- A named piece of agent work is queued and needs no decision — three per-frame
  re-checks in the renderer that re-discover facts that cannot change within a
  frame. Ceiling **20,562**, realistic band **9,800–16,500**, and it changes no
  pixel, no gameplay value and no memory allocation. Details in
  `../2026-08-13_c-flagsweep/FLAG_SWEEP.md` §4.
- Every default-off build switch has now been audited to exhaustion — 169 of
  them — and **none hides an unshipped win**. That question is closed; see
  the same file, §2.
