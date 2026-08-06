# Linux stale-DL bail — specific holders pinned (Zebes acid Ground GObj + efManagerMakeEffect)

**Date:** 2026-05-23
**Status:** **FIXED 2026-05-23** — scene-boundary GObj sweep + `gGRCommonStruct` zero in
`scManagerRunLoop`. Bail count goes from 28 per triggered run (~40% trigger rate) to **0 across
multiple repro attempts**.
**Family:** [`dl_link_stale_pointer_guard_2026-05-09`](dl_link_stale_pointer_guard_2026-05-09.md) /
[`linux_stale_scene_data_family_2026-05-11`](linux_stale_scene_data_family_2026-05-11.md)
**Platform:** Linux glibc — observed on Fedora 43 / Nobara, native Release builds.
**Files affected for diagnosis:** `decomp/src/sys/objdisplay.c` (augmented bail log),
`decomp/src/sys/objman.c` (per-GObj alloc tracking macro). Both still in place as tripwires for
future regressions of this class.

## The fix (landed)

`decomp/src/sc/scmanager.c::scManagerRunLoop` — alongside the existing Issue #103 `fighter_gobj`
NULLing at the top of the `while (TRUE)` scene-dispatch loop, two new PORT-only blocks before
the `switch (gSCManagerSceneData.scene_curr)`:

1. **Sweep every linked GObj.** Iterate `gGCCommonLinks[link]` for every `link_id`; while the head
   is non-NULL, `gcEjectGObj(head)`. Two clamps: if the head doesn't move after a `gcEjectGObj`
   call (eject refusal — would be `gGCCurrentCommon`, shouldn't happen between scenes but cheap to
   guard) `port_log` + break; if a single link's loop exceeds 4096 iterations `port_log` + break.
   Mirrors the body of `gcEjectAll()` (`objhelper.c:465`) inline so we can attach the clamps.
2. **`memset(&gGRCommonStruct, 0, sizeof(gGRCommonStruct))`** — the union of per-stage var blobs
   overlays every cached `*_gobj` slot (`zebes.map_gobj`, `hyrule.twister_gobj`,
   `jungle.tarucann_gobj`, `castle.bumper_gobj`, `yamabuki.{gate,monster}_gobj`,
   `pupupu.map_gobj[]`, `inishie.{pakkun,pblock}_gobj`, ...). Zeroing nulls them all in one shot;
   the next stage init repopulates the union from scratch.

Headers added under `#ifdef PORT`: `<gr/ground.h>` (for `gGRCommonStruct`), `<sys/objman.h>` (for
`gGCCommonLinks`).

**Why this works where the `syTaskmanStartTask` sweep didn't (see "Failed fix attempt" below):**
`scManagerRunLoop` runs once per scene transition; `syTaskmanStartTask` only at task boundaries.
The attract demo cycles through Opening sub-scenes (27 → 28 → 29 → ... → 37) all inside a single
task instance, so the task-boundary sweep was a no-op for the leak window. Moving the sweep into
the scene loop catches every transition.

**Why the GOBJ_PORT_EJECTED_SENTINEL guard (`objman.c:1881`) makes this safe to overlap with
in-game eject paths:** stage routines that self-eject (e.g. `grHyruleTwister` at `grhyrule.c:334`)
remove their GObj from `gGCCommonLinks[]` as part of the eject, so the sweep doesn't see them.
A GObj that's somehow seen twice would trip the sentinel check and `port_log` rather than
corrupt the free list — observed count of `DOUBLE-EJECT DETECTED` post-fix: 0.

**Verification:** 0 `stale dl_link bail` log lines across multiple full-attract-cycle runs
post-fix (was 28/run on triggered runs pre-fix, with ~40% trigger rate). No sweep-guard breaks,
no double-eject. The diagnostic instrumentation in `objdisplay.c` + `objman.c` stays in place so
any future regression of this class trips the same in-log signal.

**Re-verification 2026-06-09 (user report: "~30% particle corruption might still exist"):**
25-minute unattended soak on macOS (the in-log oracle is platform-independent post-arena-recycle;
only the SIGSEGV-vs-silent-corruption consequence differed by platform): **190 scene transitions,
8 complete attract cycles including the Zebes-containing opening chain — 0 bails, 0 stale tokens,
0 sweep-guard trips, 0 crashes, clean shutdown.** At the pre-fix ~40%/cycle trigger rate, 8 clean
cycles ⇒ P(bug present but silent) ≈ 1.7%. A parallel exhaustive audit of the "need to enumerate"
BSS-holder list (gEFManager pools, gFT/gWP/gIT allocators, camera, wallpaper, particle globals,
gFTData* file pointers) found **no remaining unmitigated holders** — every arena-backed pointer
is reset per scene, swept at the boundary, or guarded. Verdict: fixed with high confidence; the
residual tail closes with one Linux attract run using the same `grep -c 'stale dl_link bail'`
oracle. A user-supplied ssb64.log from any corrupted run would confirm/deny instantly via the
same grep.

## Symptom

Particle and stage-geometry rendering corruption observable in attract-mode fighter intros and
persisting into subsequent matches. Specific visual reports from the user:

- Particle effects on fighter actions corrupted (dust kicked up, Samus's gun flash, DK floor
  particles, "any particle that renders when a fighter does an action")
- Stage backgrounds corrupted in matches
- Mario's intro background corrupted (when the bug is active for the run) — no other fighter's
  intro background corrupts
- Bug persists across title-skip → match transition

Reproducibility: ~40% per attract-demo run (5/12 in a sampled session). Once a run triggers, the
bail fires every frame from the transition point onward; the consequences are visually constant for
that run. **NOT** correlated with which fighters get picked from the box (same fighter pair both
triggers and doesn't across runs).

## Class

Defensive guard from
[`dl_link_stale_pointer_guard_2026-05-09`](dl_link_stale_pointer_guard_2026-05-09.md) fires:

```
SSB64: gcDrawDObjTreeDLLinks: stale dl_link bail dobj=0x... dl_link=0x...
       list_id=16777216 (= 0x01000000) walk=0 frame=N
```

`list_id=0x01000000` is the raw bytes `01 00 00 00` little-endian — the low byte of a fresh `Gfx`
command word (G_NOOP / G_VTX-shaped) that has been written into a recycled scene-arena slot which
the dangling `DObjDLLink*` still points at. Hex dump at the stale `dl_link` address (added to the
bail log this session) confirmed the pattern: `01 00 00 00 da 01 40 00 00 00 00 00 04 00 00 00`
— the trailing bytes are `G_MTX` (`0xDA`) and `G_DL` (`0x04`) GBI opcodes from the new scene's
fresh display-list allocation overwriting the prior scene's `DObjDLLink[]` slot.

## Holder pin — what `linux_stale_scene_data_family_2026-05-11` couldn't identify

Pinned by augmenting the bail log with GObj context (parent_gobj, id, link_id, func_run pointer,
stale-memory hex dump), adding a per-allocation macro on the GObj-creation wrappers in `objman.c`
that records `(gobj, id, link_id, __builtin_return_address(0), frame)` for the leaking kinds, then
cross-referencing the bailed `gobj=` address against the `gobj_alloc` records and running
`addr2line` on the captured caller LR.

| Stale GObj | Allocator call | Source line | BSS holder |
|---|---|---|---|
| **Ground** (`id = 1010 = nGCCommonKindGround`, `link_id = 1 = nGCCommonLinkIDGround`) — persistent every frame | `grZebesMakeAcid` | `decomp/src/gr/grcommon/grzebes.c:84` | `gGRCommonStruct.zebes.map_gobj` (set at line 85, **never cleared**) |
| **Effect** (`id = 1011 = nGCCommonKindEffect`, `link_id = 6 = nGCCommonLinkIDEffect`) — transient, frame 30 only | `efManagerMakeEffect` (and many siblings in `efmanager.c`) | `decomp/src/ef/efmanager.c:1964` etc. | Various per-effect storage; effect free-list and instance lists in `gEFManager...` globals |

Both wrappers pass `func_run=NULL` by design (effects are driven by `efManagerFuncRun`; ground
GObjs run via `gcAddGObjProcess`-attached callbacks), so the `func_run=(nil)` in the bail log isn't
damage — it's confirmation that we're looking at the right kinds.

## Mechanism

1. Attract demo loads a scene that includes Zebes (e.g. `nSCKindOpeningRoom`'s Zebes sub-scene).
   `grZebesMakeAcid` runs, allocates a `Ground` GObj via `gcMakeGObjSPAfter`, links it into
   `gGCCommonLinks[nGCCommonLinkIDGround]`, and stores the pointer in
   `gGRCommonStruct.zebes.map_gobj`. The GObj's DObj tree (created by `gcSetupCustomDObjs` against
   `llGRZebesMapAcidDObjDesc`) has `dl_link` fields pointing into the loaded reloc data which
   lives in the **scene arena**.
2. The attract demo advances to the next opening scene. The port's recycle path in
   `syTaskmanStartTask` (`decomp/src/sys/taskman.c:1356`) — and equivalent scene-cleanup paths
   inside `scManagerRunLoop` — `memset(0)` the scene arena. **The Zebes Ground GObj is NOT
   destroyed** — there is no `grZebesDestroy` / `grZebesUnload`, no per-stage teardown hook, and
   no generic mass-clear of `gGCCommonLinks[]`. The GObj remains linked.
3. The new scene allocates its own data into the (just-zeroed) arena. Some allocations land on
   the bytes the old Zebes GObj's `dl_link` field still points at, replacing them with `Gfx[]`
   command bytes.
4. The renderer keeps walking the Zebes GObj's DObj subtree every frame.
   `gcDrawDObjTreeDLLinks` reads `dl_link->list_id`, sees `0x01000000` (out-of-range, because the
   bytes are now part of a G_NOOP-shaped `Gfx` word), and **bails — which means the entire DObj
   subtree under that Ground GObj does not render this frame**. Visible: dropped/garbled
   geometry from anything chained through that subtree. Because the Ground GObj remains in
   `gGCCommonLinks[]` forever, this happens every frame for the rest of the session — including
   after the user skips through the title screen into a match.

## Failed fix attempt (do NOT re-try without addressing the root cause below)

Tried adding a scene-boundary GObj sweep in `syTaskmanStartTask` (`taskman.c:~1361`,
before the existing `port_taskman_evict_arena_caches` + `memset` of `gPortSceneHeap`):

```c
for (s32 link = 0; link < ARRAY_COUNT(gGCCommonLinks); link++)
    while (gGCCommonLinks[link] != NULL)
        gcEjectGObj(gGCCommonLinks[link]);
```

**Did not fix the bug.** Bail count stayed at 28 per trigger run, bailing GObjs were the same
addresses as before, allocated at the same frame (14) by the same callers (`grZebesMakeAcid` and
`efManagerMakeEffect`).

**Why it failed:** `syTaskmanStartTask` runs only at **task** boundaries, not **scene**
boundaries. The attract demo cycles through many sub-scenes (`InitGroundData scene=30..37` etc.
visible in `ssb64.log`) inside a single task invocation. The sweep correctly destroyed any GObjs
present at task boundary but never ran between intra-task scene transitions, so the Zebes Ground
GObj was created early in the attract task, the demo cycled past Zebes within the same task, and
the GObj kept its stale `dl_link` for the rest of the run. Reverted.

## Where the fix actually needs to go

The right hook is **inside `scManagerRunLoop`** in `decomp/src/sc/scmanager.c`, around the
top of the `while (TRUE) { switch (gSCManagerSceneData.scene_curr) { ... } }` block at line 944.
A PORT-gated cross-scene cleanup block already exists there at lines 961-965 — the existing block
NULLs `fighter_gobj` slots in the three persistent battle-state structs across scene boundaries
for exactly the same reason (cross-scene GObj-pointer staleness, Issue #103 family). That's the
right place and the right precedent. The fix should:

1. **Walk `gGCCommonLinks[link_id]` for the scene-scoped link IDs and `gcEjectGObj` each**
   (`gcEjectGObj` is the existing teardown API — already does
   `gcEndProcessAll → gcRemoveDObjAll → gcRemoveGObjFromDLLinkedList → gcRemoveGObjFromLinkedList
   → gcSetGObjPrevAlloc` and stamps `GOBJ_PORT_EJECTED_SENTINEL` to catch double-eject). Per
   `objdef.h::GObjLinkID` (lines 65-106), the link IDs all appear to be scene-scoped — there is
   no documented persistent-across-scenes GObj kind. Sweep all of them or carefully whitelist
   from the enum.
2. **Also NULL the BSS holders that point at those GObjs.** Just unlinking from the chain leaves
   the BSS pointer dangling. Known holders to clear:
   - `gGRCommonStruct.zebes.map_gobj` (and the equivalent for every other stage in
     `gGRCommonStruct.*.{map_gobj, ground_gobj, twister_gobj, gate_gobj, tarucann_gobj}` — see
     `grep -rn 'gGRCommonStruct\.[a-z_]*\.[a-z_]*gobj' decomp/src/gr/`)
   - Effect-manager free lists / instance arrays in `gEFManager*` globals (need to enumerate)
   - Likely similar holders in `gMP*`, `gFT*`, `gWP*`, `gIT*` namespaces.
3. **Order matters:** sweep BEFORE any subsequent arena memset on the same boundary, so
   `gcEjectGObj`'s child-traversal (`gcRemoveDObjAll`, `gcEndProcessAll`) can walk the GObj's
   children (DObjs, gobjprocs) while the arena memory still holds them.
4. Guard against `gGCCommonLinks[link] == head` after eject (non-progress) and log + break.
   `gcEjectGObj` refuses to eject `gGCCurrentCommon`; we won't be inside the run loop here, but
   future-proof against it anyway.

## Diagnostic instrumentation in place (KEEP)

Two patches added this session that are NOT cleanup-worthy on their own and should be **retained**
in `linux-stability-fixes` to help the agent that picks this up:

- **`decomp/src/sys/objdisplay.c::gcDrawDObjTreeDLLinks` bail log** augmented to include
  `dobj.flags`, `dobj.vec`, `parent_gobj` (pointer + `id`, `link_id`, `obj_kind`, `func_run`
  pointer), and a 16-byte hex dump of the stale `dl_link` memory. The `gobj.id` + caller LR
  combination is the pin-down — without these the family doc could only narrow to "some Ground /
  some Effect GObj."
- **`decomp/src/sys/objman.c::PORT_LOG_GOBJ_ALLOC` macro** at the four
  `gcMakeGObj{SPAfter,SPBefore,After,Before}` wrappers, filtered to the leak-prone kinds
  (`id=1010` Ground, `id=1011` Effect) to keep log volume low. Each record:

      SSB64: gobj_alloc gobj=0x... id=N link=N caller=0x... frame=N

  The `caller=` is `__builtin_return_address(0)` from inside the wrapper, which is the
  user-level allocation site. `addr2line -e build-rel-clang/BattleShip -f -C 0x...` resolves to
  the function name (`grZebesMakeAcid`, `efManagerMakeEffect`, etc.).

Reproduction recipe for the next agent:

```bash
# Build native Release (Wayland-gate fix from this branch is required for visual confirmation)
cmake -S . -B build-rel-clang -GNinja -DSSB64_VERSION=us -DCMAKE_BUILD_TYPE=Release -DNON_PORTABLE=ON
CC=clang CXX=clang++ cmake --build build-rel-clang --target ssb64 -j
# Run a handful of times, watch attract; trigger rate ~40%
(cd build-rel-clang && ./BattleShip)
# After observing trigger, count bails:
grep -c 'stale dl_link bail' ~/.local/share/BattleShip/ssb64.log
# Cross-reference each bail's gobj address to its alloc record:
for addr in $(grep 'stale dl_link bail' ~/.local/share/BattleShip/ssb64.log \
              | grep -oE 'gobj=0x[0-9a-f]+' | sort -u | sed 's/gobj=//'); do
    echo "--- bail gobj $addr ---"
    grep "gobj_alloc gobj=$addr" ~/.local/share/BattleShip/ssb64.log
done
# Pin caller(s):
grep 'gobj_alloc' ~/.local/share/BattleShip/ssb64.log | grep -oE 'caller=0x[0-9a-f]+' \
    | sort -u | sed 's/caller=//' | xargs addr2line -e build-rel-clang/BattleShip -f -C
```

## Audit hook

When triaging the next "Linux-only intermittent visual corruption that survives scene
transitions" report:

1. **Re-grep the ssb64.log for `stale dl_link bail` first.** If present, this is the same family.
   Bail count > 0 + persistent same-`dobj` across consecutive frames is the signature.
2. **Cross-reference the bailed `gobj=` address against the `gobj_alloc` records** (the macro
   in objman.c only records Ground/Effect by default; widen the kind filter if the bail's
   `gobj.id` is different).
3. **`addr2line` the caller LR to identify the allocator.** That function's stage / subsystem is
   the leaker. Find its BSS holder, add a cleanup, and verify across ~10 runs (silently, with
   `grep -c 'stale dl_link bail'` as the oracle — no visual observation required since the
   in-log signal is deterministic).
4. The structural fix is what the next agent is taking on (above). Once landed, the bail log
   will go silent for these kinds — the diagnostic instrumentation can stay in place permanently
   as a tripwire for new instances of the same class.

## Related

- [`dl_link_stale_pointer_guard_2026-05-09`](dl_link_stale_pointer_guard_2026-05-09.md) — the
  defensive guard whose bail count we're using as the in-log oracle. Originally documented as
  "specific BSS holder isn't identified"; this entry identifies the first two holders.
- [`linux_stale_scene_data_family_2026-05-11`](linux_stale_scene_data_family_2026-05-11.md) —
  the family catalogue (this is a sibling of its "variant 4 segment-E DL pointer" entry, but
  one consumer site upstream — at `gcDrawDObjTreeDLLinks` rather than `gfx_step` — with visible
  geometry drop instead of SIGSEGV because the stale bytes happen to trip the bound check
  before any actual deref).
- [`wayland_set_window_icon_renderer_corruption_2026-05-22`](wayland_set_window_icon_renderer_corruption_2026-05-22.md) —
  fixed in this same branch; was a DIFFERENT bug masking this one for a long time. With the
  Wayland gate landed, this stale-dl_link symptom became cleanly observable.
