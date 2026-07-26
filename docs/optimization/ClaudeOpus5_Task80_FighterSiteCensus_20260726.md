# Task 80 — 97.4% of fighter texture binds have no site key, and there are only 9 of them

**Date:** 2026-07-26
**Status:** Census complete. **The fighter-side texture memo is dead** — not
"needs a better table", dead. Probe removed, source restored. No runtime change.
**Input:** `NDS_TASK80_FIGHTER_SITE_CENSUS=1` build, frames 439–567, counters
differenced across the window so boot and menu binds are excluded.

## 1. The question

Task 79 E1 extended the stage's per-site texture memo to the fighters. It bought
them **−716 ticks/frame** — nothing — while costing the stage 6,880 on every
frame through table contention. Two explanations fit: the fighters' key does not
repeat, or it repeats and the shared 128-entry table simply thrashed. Those point
at opposite next steps, so guessing was not an option.

This gave the fighters their own table, ran the lookup, classified the outcome
and **counted it without acting on it** — so contention is out of the question
entirely and the only thing under test is whether the key works.

## 2. Result

```
calls           1,152
nullSite        1,122   97.4%
notFound            0    0.0%
hashMismatch        0    0.0%
staleEntry          0    0.0%
wouldHit           30    2.6%
tableFull           0    0.0%
distinctSites       0          (no new site learned inside the window)
```

**97.4% of fighter texture binds arrive with `state->source_command_site ==
NULL`.**

There is exactly one writer of that field (`nds_renderer.c:22978`), inside the
generic display-list execution loop. The fighters bind their textures from the
native production path, which executes generated runs and never walks a display
list, so there is no source command pointer to key on. The memo could not hit
because **the key does not exist on that path** — and the three counters that
would have indicated a fixable problem (`hashMismatch`, `staleEntry`,
`tableFull`) are all exactly zero.

That fully accounts for Task 79 E1's −716. At most 30 binds per 128 frames were
ever memoisable.

## 3. The larger finding: fighter texture binds are 9 per frame

1,152 binds over 128 frames is **9.0 per frame**, for both fighters combined.

Task 79 E0 measured the texture/material family at 143,434 ticks/frame and
proposed attacking it via the fighter path. Nine binds a frame cannot be a large
share of that. The cost is overwhelmingly the **stage** — which already has the
memo, and has had it since it was written.

So E0's recommendation was right about the size of the family and wrong about
where inside it the addressable cost sits. The correction matters more than the
dead end it closes: **before optimising a call, count the calls.** The census
that would have prevented both Task 79 E1 and this task was 1,152 divided by
128, and it cost one observation-only build.

## 4. What this closes and what it opens

**Closed.** A fighter-side texture site memo, in any table configuration. The
key is absent, not contended.

**Open, and now the only live question in this family.** What fraction of *stage*
binds already hit the existing memo? If the hit rate is high, the residual
143,434 is genuine misses and uploads, most of it is not addressable by
memoisation at all, and the whole texture/material direction should be closed
out rather than pursued. If it is low, the stage memo is the thing to fix and it
is a much smaller change than anything proposed so far.

That is one counter on `ndsRendererStageTextureSiteTryBind`'s two exits, on the
same standard window. It should be answered before any further texture work is
scoped, for exactly the reason this task exists.

## 5. Method

The probe followed Task 73's shape: separate table, classify, count, never act,
and difference the counters across the measurement window so boot traffic is
excluded. It also followed Task 69's rule — instrument the denominator
(`calls`) alongside the reasons, because "no hits" and "never called" are
indistinguishable otherwise, and here the denominator is what produced the more
valuable finding.

Symbols were checked with `nm` before the run, per Task 70, where a
write-only static array was deleted by the compiler and the census silently
measured nothing.

Probe removed per `AGENTS.md` — temporary probes do not survive handoff. The
numbers above are the deliverable.
