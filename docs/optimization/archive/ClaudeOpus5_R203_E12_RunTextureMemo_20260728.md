# R2-03 E12 — the fighter had no key for the cache that already existed

**Date:** 2026-07-28
**Phase:** R2-03 (fighter direct draw)
**Verdict:** KEEP. `PrepareProductionRun` 82,042 → 49,318, texture prepare
45,952 → 12,362. Graduated.

## 1. What this actually is

Not a new mechanism. The texture resolver already opens with a site cache,
`ndsRendererStageTextureSiteTryBind`, which skips the whole resolve when it
recognises the call site. It is keyed on `state->source_command_site` — the
address of the `Gfx` command being interpreted.

**The native fighter path does not interpret display lists**, so it has no
command site, `ndsRendererStageTextureSiteFind` returns `NULL` on its first
test, and every fighter run pays the full resolve: `SyncTextureTile`, the
~30-field key build, its hash, and the cache lookup.

E5 had already measured what the fighter's answer *does* depend on: `run_index`,
and nothing else. So this memo is the existing site cache re-keyed for a path
that has no `Gfx` site.

That framing matters for R2-05. The next fighter through the pipeline will have
the same gap for the same reason, and the fix is already generic.

## 2. Sizing, and a correction to E11

E11 read the texture prepare as 45,952 ticks over 45.3 calls and I described it
as "45.3 texture resolutions per frame". That was wrong. Only **8.4** of those
45.3 calls are textured at all; the other ~37 enter the same branch and do only
the thirteen field writes. So the cost is 8.4 resolutions at roughly 5,500 ticks
each, not 45.3 at 1,013.

The memo target was the same either way, but the per-call figure was off by 5x
and is corrected here.

## 3. Result

Identical source, `NDS_R2_FIGHTER_RUN_MEMO` 0 versus 1, 128 presented frames.
The 0 arm reproduces E11 to the digit (82,042; texture prepare 45,952), so the
memo code present-but-disabled costs nothing and the two arms differ only in the
flag.

| phase | memo off | memo on | delta |
|---|---:|---:|---:|
| validate | 8,308 | 9,106 | +798 |
| **texture prepare** | **45,952** | **12,362** | **−33,590** |
| texture reuse | 1,090 | 1,199 | +109 |
| uv loop | 11,853 | 11,424 | −429 |
| tail | 14,838 | 15,228 | +390 |
| **total** | **82,042** | **49,318** | **−32,724** |

The three small movements are placement, not behaviour — the file changed, and
all of them are far inside the 5,000–7,000 build-placement floor.

**−32,724 against a predicted 35,000–45,000.** Below the band I set in E11,
recorded as measured rather than rounded up to it.

## 4. Why it is safe

| counter | value |
|---|---|
| memo hits | 1,074 (8.4/frame — every textured call) |
| **fills** | **9 in total, not per frame** |
| stale entries | 0 |
| **mismatches, level 2** | **0 of 1,083** |

Nine distinct textured runs, each resolved once for the entire match.

Level 2 never takes the memo. It lets the full path run and then asks whether
the memo would have answered differently, so a disagreement is counted instead
of drawn — the same arm shape that caught E8's incomplete key three times.

Two hazards were handled rather than assumed away:

**Residency is not identity.** E5 deliberately excluded `resolved->entry` from
its stable set — *"a pointer into the hardware texture cache, which rotates for
reasons unrelated to what is drawn"*. The memo therefore stores a cache **slot
index**, not a pointer, and revalidates `entry->ready` and `entry->name` before
trusting itself, which is exactly the contract Task 36's replay already uses for
this hazard. A stale entry falls through and refills. It never fired in 129
frames, but the check is what makes that a measurement rather than a hope.

**The resolver's cache-hit tail has live effects.** The memo replays all of
them: `last_used_frame` (this is the eviction LRU — dropping it would let the
cache reclaim the entry the memo points at), the name and param binds, the
active-entry pointer, the pinned static-texture hit, and the four `stats`
fields.

One effect is deliberately not replayed: `ndsRendererStageTextureSiteRemember`.
Skipping it can only cost a later site-cache miss, never produce a wrong bind,
because a plan is keyed by exact site pointer plus two state hashes.

## 5. Disposition

`override NDS_R2_FIGHTER_RUN_MEMO := 1` in the published
`smash64ds-battle-playable-hwtri` block, and a non-`override` default in the
tick-HUD block so the instrument stays flag-identical to the shipped ROM.
Default remains 0 so the level-0 arm stays measurable and level 2 stays
available.

## 6. The fighter was the only warm-frame texture lookup

Boundary failed after graduation on
`Performance/coarse texture hash lookup lacked mode-applicable active/table
coverage`, reading `RENDER_TEXHASH=0,0,0,0,0`. Every texture lookup counter was
zero.

Not a regression — the memo working. Once the fighter stops resolving, **nothing
else in a warm frame performs a texture lookup at all.** The stage binds through
descriptors resolved at prepare time, and the resident cache survives frame
boundaries. That is a stronger statement of E12's premise than the tick counts:
the entire warm-frame texture-resolution cost was the fighter re-asking a
question with a constant answer.

It also broke a guarantee. That assertion existed to prove the lookup path is
alive, and with every counter zero **a working memo and a dead texture path are
the same observation** — the exact failure the E5 rule in
`TASK_STANDING_RULES.md` was written about.

So the liveness proof moved onto the memo's counters rather than being dropped:
coverage now passes if the hash lookup is live *with* its existing exact
accounting, **or** if lookups are zero and the memo is demonstrably live (hits
> 0, stale bounded by the fills that answered them, and never a level-2
mismatch). The counters were also hoisted out of the flag guard, so a memo-off
build reads five honest zeros instead of whatever the linker left there — two
probes this cycle read an ARM opcode out of a garbage-collected counter, and the
verifier cannot afford that ambiguity. `check-harness-registry.ps1` pins the new
contract.

## 7. It did not compile in the configuration it ships in

Worth recording rather than quietly fixing. The memo's implementation was
written beside the E5 globals it derives from, which put it inside the
`NDS_R2_FIGHTER_RUN_PROOF` guard. Every measurement build carried `PROOF=2`, so
the definitions were always there.

The shipping combination is `PROOF=0, MEMO=1`: definitions compiled out, call
sites compiled in. Boundary failed on it immediately after graduation, with
`implicit declaration of ndsRendererR2RunTextureMemoApply`.

The block now sits outside both guards, and `NDS_R2_RUN_MEMO_MAX` — a fact about
the data (`sNdsNativeFighterRuns[67]`) that both flags use — was hoisted with it.
All four combinations of the two flags are built.

This is the second instance the same day of validating only in the configuration
that carries the instrument; the falsifier failure in R2-02 F was the first.
Both are now one rule in `TASK_STANDING_RULES.md`.

## 7. For the standing rules

E5 rejected this cut in one line — *"~119 UV writes/frame can't explain 21,504
ticks"* — and the arithmetic was right. What was wrong was the assumption that
the function's cost was inside the function. **When a candidate is rejected for
being too small, check whether the instrument measured the symbol or the work**:
the census row and E5's bracket both read ~22,000 self time for a function whose
inclusive cost is 82,042.

The second, narrower lesson: a *generic* fast path can be structurally
unavailable to a specialized caller without anyone noticing, because it fails
silently by returning `FALSE`. The stage site cache has been in this file for
many tasks and the fighter has never once hit it. Worth asking of every other
shared cache the native paths inherit.
