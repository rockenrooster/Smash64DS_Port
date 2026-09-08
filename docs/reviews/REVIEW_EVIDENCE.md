# Native-only re-review evidence

Date: September 7, 2026.
Reviewed repository: `rockenrooster/Smash64DS_Port`.
Pinned revision: `a5f2223179def1f061604db22b947cc65e244ae3`, `master`.
At review, the branch API listed only `master`.

Scope: read-only review of repository source and documented evidence. No new
ROM, emulator execution, measured performance improvement, or bug closure is
claimed. The owner's latest observations supersede older queue text, especially
Arwing visibility and the instruction to remove Saffron haze. Local/uncommitted
code and the exact owner-played ROM may differ from this revision.

## Confirmed source-level findings

### 1. The existing checker requires the forbidden scanner

`scripts/check-renderer-itcm-placement.ps1:15-69,160-190` explicitly explains
that the generic renderer must still be emitted, only moved out of ITCM. Its
`$requiredEmittedFunctions` includes `ndsRendererScanList`; the later loop fails
when that required symbol is absent. This is a checker contradiction with the
new all-ROM requirement, independent of whether a particular build invocation
currently runs the checker.

Retain meaningful native/shared-helper placement checks but reverse the
interpreter-presence requirement. Exclude implementation units from ROM builds;
a string search or one renamed symbol is not a capability boundary.

### 2. Mario/Fox can still leave the native body path

`src/port/renderer_adapter_fighter.c:3250-3415` selects native mode at runtime,
then clears `native_owner_enabled` for animation locks in production/hierarchy
modes. The shuffle-fold variant removes only the shuffle condition. It still
rejects `fp->is_use_animlocks`.

`renderer_adapter_fighter.c:3400-3750` also clears eligibility for plan,
validation, material, and matrix failures. At `3980-4250`, callers that have not
attempted production enter the selected-list loop. At `4250-4500`, when a native
owner has not started, the hardware-triangle configuration calls
`ndsRendererExecuteDisplayListWithVertexCache` directly.

Passing a null callback when `no_oracle` is true disables that callback; it does
not disable display-list execution. Thus neither "hardware triangles" nor
"no oracle" establishes native-only rendering.

This establishes source reachability, not a measured count for the owner's
latest ROM. Coverage must include the statuses that actually set the flag.

### 3. Entry props/effects retain fallback too

`src/port/renderer_adapter_stage.c:4700-4990` classifies exact Mario pipe and Fox
Special3/Arwing roots for a native entry-effect route. At `5080-5350`, a native
submission failure increments `gNdsEntryEffectNativeFallbackCount` and returns
false. `ndsRendererAdapterSubmitStageDL` then continues into the general adapter;
its comment explicitly retains compatibility fallback after failed texture
preparation.

Checking only fighter-body production therefore misses attached/entry effects.
The scanner also remains referenced by the stage adapter and implemented in
`src/nds/nds_renderer_dispatch_profile.c` (repository search).

### 4. The workflow contains a now-obsolete exception

`docs/BUG_FIXING_PROCESS.md`, Core rules 5/6 and "Derive the observable contract",
permits temporary reference/interpreter rendering before final natural-path
proof. Under the owner's new requirement, a ROM may no longer be that reference
host. Keep source/host oracles and native-only diagnostics, and update only the
owning conflicting rules. No change to read-only decomp is required.

### 5. The 20 FPS cost-elimination argument is invalid

`docs/p2/BUG_NOTES.md:235-271`, "Stage frame rate", compares static-packet costs
and estimates a difference of about 3,000-12,500 ticks. It then treats a whole
extra VBlank (~560,000 ticks) as the workload increase needed to explain 20 FPS.
That does not follow: presentation deadlines are quantized.

Illustrative arithmetic, NOT observed project workload:

| Work | Two-VBlank budget | Margin | Earliest interval count in a next-edge model |
|---:|---:|---:|---:|
| 1,115,000 ticks | 1,120,380 | +5,380 | 2 |
| 1,127,500 ticks | 1,120,380 | -7,120 | 3 |

A 12,500-tick increase can cross the deadline; the additional wait occupies the
remainder of the next interval. Actual pacing, GPU/DMA waits, audio work, and
present phase must be measured. This does not establish the packet as the real
bottleneck; it establishes that the recorded arithmetic cannot eliminate it.

### 6. "FIXED" and "native" have been used before full closure

`docs/BUGS.md` labels the Hyrule descriptor fix **FIXED** while acknowledging that
a live hit trace is still owed. The handoff says the latest Boundary run was
interrupted and the four-CPU stress arm remains red. The notes document successful
native admission or triangle counts while reporting remaining invisible content.

Those measurements are useful intermediate evidence. They are not the complete
closure chain defined by the project's bug workflow. The reviewed queue draft
does not promote any issue to FIXED.

## Source contracts that narrow the updated bugs

### Hyrule

`decomp/BattleShip-main/decomp/src/gr/grcommon/grhyrule.c:118-190,290-477`
contains random selection from map-defined Twister positions, randomized wait,
80-tick summon, randomized active lifetime, Move/Turn lifetime decrement,
capture-aware stop, 32-tick subside, and repeat. The source is included by
`src/import/battleship_grhyrule_ground.c`.

`grHyruleMakeTwister` also depends on floor projection and successful effect
creation. A failed creation resets the randomized wait. Imported timing code
alone does not prove successful hazard creation or collision on the ROM.

### Zebes

`decomp/BattleShip-main/decomp/src/gr/grcommon/grzebes.c:151-182,225-242`
updates acid root height over a 240-tick movement phase and tests contact against
root Y plus child Y, with `acid_wait == 0`. The comparison uses fighter root Y,
not feet or a generic hurtbox edge. The root-only poke described in the old notes
cannot settle the current dome/contact complaint.

### Sector Z

`decomp/BattleShip-main/decomp/src/gr/grcommon/grsector.c:991-1040` enables and
updates yakumono 1 only in eligible pilot/line/near-Z states and disables it on
departure. Collision X is root X plus `arwing_target_x`; collision Y is root Y
plus child Y. Visible flybys are not automatically rideable.

`grsector.c:663-718` has two source-local gun points for 2D lasers with explicit
axis rotation/remapping. `:798-883` constructs the 3D flight basis and transforms
the local forward muzzle before targeting. Preserve those separate paths.

### Yoster alpha

`src/nds/nds_renderer_textures_effects.c:530-626` now recognizes the exact
supported second-cycle SHADE multiplication and classifies from the first
cycle for that form. This is an implemented change; it is not proof that all
clouds, spinning textures, particles, and colors satisfy the updated report.

The notes' cloud A5I3 preparation witness shows engagement, not final cutout
coverage. The same notes flag single-slot graded-alpha coexistence as a separate
unverified risk. Treat it as a test requirement, not a confirmed new root cause.

### Saffron

`docs/p2/BUG_NOTES.md`, "Saffron white band" and "Saffron gate display lists",
identifies a source layer-3 haze panel and separates opaque door lists from
translucent head-1 content. The owner's current removal instruction resolves the
old "compare before removal" decision. Omit only that identified panel through
the generator/descriptor; do not globally erase white pixels or surfaces.

### CSS and geometry

`docs/HANDOFF.md` records genuine Link/Yoshi/Pikachu CSS repairs, native stage
admission, compact preview generation, and remaining runtime acceptance. Do not
repeat the prior VS Options repair or restart a previously corrected CSS visual
investigation without a new reproduction.

The updated owner queue still controls what remains broken. For Castle, Yoster,
and Inishie, packet presence/acceptance does not prove screen coverage. For
Congo, the final documented gate-success probe supersedes the earlier
gate-failure theory. For Sector, current owner-visible Arwings supersede the
earlier no-spawn observation unless different build identity explains it.

## Suggested next integration boundaries

1. Remove the all-ROM generic capability and complete its live callers,
   including Mario/Fox and their effects; build exclusion plus negative tests.
2. Fix native visible geometry/materials and gameplay contact/attachment using
   the same-tick contracts above, grouped by existing code ownership.
3. Verify actual present deadlines and source/asset identity, then run the
   widest relevant verifier and owner acceptance. Keep failed rows open.

Do not turn these into three new permanent workflow documents. This bundle is
an external review/handoff; integrate findings into the existing workflow,
`BUG_NOTES.md`, board and lean queue.