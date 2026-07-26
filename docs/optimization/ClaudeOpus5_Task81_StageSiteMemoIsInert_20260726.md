# Task 81 — The stage texture memo never runs, and there are only 25 binds a frame

**Date:** 2026-07-26
**Status:** Census complete. **The texture/material direction is much smaller
than Task 79 E0 claimed, and this closes it.** The stage site memo is inert in
the battle configuration. Probe removed, source restored. No runtime change.
**Input:** `NDS_TASK81_STAGE_SITE_CENSUS=1`, frames 439–567, counters
differenced across the window.

## 1. The question

Task 80 showed the fighters make 9 texture binds a frame and 97.4% of them carry
no memo key, which killed the fighter-side memo and pointed at the stage as the
owner of the 143,434 ticks/frame Task 79 E0 measured. One question was left: how
often does the *stage* memo already hit? High means the residue is genuine
misses and uploads and the direction closes; low means the memo is the thing to
repair.

## 2. Result

```
allCalls        3,200   100.0%   25.0/frame
residentCalls   2,048    64.0%   16.0/frame
stageCalls          0     0.0%    0.0/frame
tryCalls        1,152    36.0%    9.0/frame
disabled        1,152    36.0%    9.0/frame
missFind            0
missStale           0
missLod             0
hit                 0
```

**The stage makes zero texture binds during battle.** Not few — zero, across
128 frames. And because `sNdsRendererStageTextureSitesEnabled` requires
`owner == STAGE`, every one of the 1,152 calls that reaches the memo is rejected
at its first line. Hit, and all three miss reasons, are exactly zero.

The stage site memo — `TryBind`, `SiteRemember`, `ndsRendererStageTextureSiteFind`,
`ndsRendererTextureSourceHashCommand` and a 128-entry, 12 KiB table — **does not
execute at all in the battle configuration.** That is almost certainly Task 53's
doing: the stage replays a recorded GX command stream
(`ndsRendererTask36ReplayRun`, 41,200 ticks/frame in the Task 78 census) instead
of re-resolving textures per frame. The replay replaced the memo and the memo
was never retired.

This also retro-explains Task 79 E1 completely. Extending the memo to the
fighters could not help, because the memo it extended was already dead — E1 was
not a fast path that failed to transfer, it was a fast path that had never been
running.

## 3. The number that closes the direction

**25 resolver calls per frame.** 16 resident/preflight, 9 live.

`ndsRendererHardwareResolveOrBindTexture` costs 42,420 ticks/frame, so that is
**~1,697 ticks per call** — which is not a lookup, it is texture conversion and
upload work with a lookup on the front of it.

Task 79 E0 estimated 40,000–70,000 ticks/frame available from "avoiding the
resolver". That estimate is now indefensible, and it was mine: I sized it from
symbol totals without ever counting the calls. Against 9 live binds a frame, the
memoisable part is bounded by what the *lookup* costs, and the only lookup-
specific symbol that can be isolated is `ndsRendererHardwareTextureKeyHash` at
6,350 ticks/frame, plus an unknown share of the frame's 19,400 `memcmp` ticks
and of the key construction inside the resolver. A defensible ceiling is
**15,000–25,000**, not 40,000–70,000 — and the top of that range needs a fast
path that Task 80 proved cannot be keyed on the fighter side anyway.

At 2–3× the ±8,000 placement variance, for a change in the hottest path in the
renderer, that is not a good trade. **The texture/material memo direction is
closed.**

## 4. What the 143,434 actually is

Not lookups. With 25 calls a frame, the family's cost is the work each call
does: conversion, tile sync, palette handling, GX parameter application — plus
`ndsRendererAdapterBuildNativeMaterialSnapshot` at 12,766 and
`ndsRendererNativeApplyMaterial` at 7,883, which are per-run material state
rather than per-texture identity.

Attacking it means doing fewer or cheaper *uploads*, not fewer lookups. That is
a different task with a different shape, and nothing in Tasks 79–81 has sized
it.

## 5. Deletion candidate, not yet a deletion

The stage site memo is inert in battle. `AGENTS.md` says to migrate or delete
obsolete bounded modes when natural runtime replaces them, and Task 53's replay
is exactly such a replacement.

But the measurement window is battle-only, and the memo may still earn its keep
in menus, the title screen or results, where the stage owner does draw through
display lists. **Do not delete it on this evidence.** Establishing that requires
the same counters over a non-battle window, which is a small task and worth
doing before 12 KiB of BSS and four functions are removed — or kept.

## 6. Method note — I made the mistake this campaign had just written down

The first run of this census counted only stage-owned calls, and returned
`stageCalls` delta 0 with no denominator to interpret it against. Task 80's
lesson, one task earlier, was that the denominator is the valuable number. The
corrected run added `allBindCalls` and `residentBindCalls` and immediately
produced the finding that closes the direction: 64% of resolver traffic is the
resident path, which no memo would ever have touched.

A denominator that only counts the population you already suspect is not a
denominator. Both census runs are in this task's history rather than only the
corrected one, because the first one's shape is the instructive part.
