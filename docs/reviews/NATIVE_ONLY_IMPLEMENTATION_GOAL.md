# Smash64DS: native-only ROMs and the September 7 repair queue

Review base: `master`, `a5f2223179def1f061604db22b947cc65e244ae3`.
This is an implementation directive, not a claim that its changes were made.

## Scope and non-negotiable outcome

Every newly built `.nds` must be incapable of executing the generic N64
display-list renderer or generic software scene/sprite compositor. This applies
to P2, newly rebuilt P1 targets, diagnostic targets, profiling targets, and
verifier targets. A default-off switch, a zero observed fallback count, or a
runtime preference for native rendering does not satisfy this requirement.

Keep any necessary generic/reference renderer on the host only, outside every
ROM's compilation and link inputs. Do not create an opt-in generic-rendering ROM
as an exception. Source-faithful gameplay code, CPU matrix calculations, native
packet submission, and copying converted images to BG/OBJ are not inherently
non-native rendering. The forbidden capability is interpreting source graphics
commands/rasterizing source scenes as a substitute for the DS-native path.

Preserve the accepted Main Menu/VS Mode behavior and presentation. Keep 1P
campaign work paused. Reuse the VS CSS backend for future 1P CSS work without
resuming the campaign now. The owner has authorized removal of Saffron's white
haze: do not wait for another source-comparison decision on that removal.

Follow `docs/BUG_FIXING_PROCESS.md`: source contract, first divergence, owning
fix, natural-path proof, relevant verifier, performance, and owner acceptance
where needed. The owner's newer all-ROM prohibition supersedes that workflow's
permission to temporarily enable a generic renderer in a ROM. Host reference
tools remain permissible. Do not edit `decomp/`.

## First deliverable: remove the renderer escape routes

1. Read current `docs/HANDOFF.md`, `docs/P2_EXECUTION_BOARD.md`, the workflow,
   `docs/BUGS.md`, and `docs/p2/BUG_NOTES.md`; inspect relevant source and existing
   regression checks. Preserve unrelated dirty work. Recheck the commit before
   applying this review: local owner fixes may be ahead of the reviewed branch.
2. Inventory the actual graphics execution entry points, not functions merely
   containing "generic" in comments. Start with:
   - `src/port/renderer_adapter_fighter.c`: native-owner eligibility failures and
     `ndsRendererExecuteDisplayListWithVertexCache` fallthrough.
   - `src/port/renderer_adapter_stage.c`: stage submission, entry models, effects,
     and the remaining interpreter branches.
   - `src/nds/nds_renderer_dispatch_profile.c`: interpreter public entry points
     and the scanner they reach.
   - Source-SObj/software-preview callers, player tags, and source-menu pumps.
     Trace the actual fallback from the callers before removing a helper.
   - `scripts/check-renderer-itcm-placement.ps1`, which currently requires the
     generic scanner to exist in the ELF.
3. Separate shared native math/GX/texture services from the forbidden scanner
   and rasterizer. Remove the latter's implementation units from every ARM9/ROM
   build, including textual `.c` inclusions. Do not delete shared hardware
   helpers merely because the old interpreter also calls them.
4. Remove or reject runtime/build switches that select the forbidden routes.
   Profiling must instrument the same native runtime, not select a different
   renderer. Reject incompatible legacy target configurations rather than
   silently changing their meaning.
5. Replace native-to-generic fallthrough with an explicit native-path failure.
   Keep a compact first-failure record: scene, fighter/stage/actor identity,
   status or root/material identity, and rejection reason. Normal, intentional
   invisibility/offscreen culling is not a missing-renderer failure. A required
   unsupported state is a failure, not a successful empty draw.
6. Make required resource/asset admission happen before the active scene where
   possible. Unsupported required content must not trigger slow rendering,
   invisible substitution, discarded geometry, or a reduced roster advertised
   as fixed. Failure containment is not bug closure.
7. Reverse the obsolete emitted-scanner assertion. Add compile/link exclusion
   checks on actual build inputs, plus a post-link audit before packaging each
   ROM. A symbol-name scan alone is insufficient because inlining, aliases, or
   differently named implementations can evade it. Keep the existing native
   ITCM placement checks for genuinely shared/native functions.
8. Add a negative control that attempts to reference the forbidden interpreter
   from a temporary test translation unit. It must fail compilation/linking or
   the binary gate; it must never produce an accepted ROM. A deliberately
   contaminated object/build manifest must also fail the relevant gate.

Do not stop at making failures loud. Port every live required caller so the
real game builds, renders completely, and passes. Use one inventory/compiler
pass to expose the call-site worklist, not one ROM build per discovered call.
Do not publish an assertion-filled or content-stripped ROM as the repair.

## Mario/Fox are mandatory regression coverage

The reviewed fighter adapter disables native production for
`fp->is_use_animlocks`. Shuffle folding addresses a separate condition and does
not remove that animation-lock branch. Admission, material, matrix, and
validation failures can also reach the generic per-list loop before production
has been attempted. A null oracle callback does not prevent that interpreter
from executing.

Implement the correct native handling of those states. Never simply ignore an
animation lock to keep eligibility true. Compare the affected source transforms
and attachments with the native output.

Exercise Mario/Fox beyond idle: entry and respawn props, ordinary motion,
damage/hitlag, the source statuses that actually set animation locks, specials,
Fox's gun/model-part transition, shields, grabs/throws, particles, tags,
high/low detail, four-player play, and Results/rematch. First find the actual
flag setters; do not assume an ordinary CPU match reaches them all.

The same native-only rule covers every newly enabled fighter and item. Native
body rendering does not exempt a fighter's effects or attached objects.

## Batch the visual and gameplay repairs by their owning code

### Missing native geometry: Castle, Mushroom Kingdom, Yoshi's Island

Do not rebuild complete packets merely because geometry is missing on screen.
Existing notes report source geometry/bindings are present. The tested Castle
near-fan explanation was not engaged at the failing camera, and a later review
found the range-shift arithmetic internally consistent. Neither proves all
clipping behavior correct; neither warrants repeating the same static census.

At a camera that actually shows the defect, compare one missing source triangle
with an adjacent visible control through source position, composed transform,
clip coordinates, native range/no-Z path, packed GX vertices, primitive-group
boundaries/winding, effective depth/material state, and visible coverage.
Use the same capture mechanism for all three stages without presuming they
share one root cause. Keep their owner reports separately open.

### Congo cannon

The latest recorded probe has zero adapter rejects and two emitted triangles
per callback. That refutes the earlier gate-failure explanation for that ROM;
it does not establish visible rendering. Identify the directional-arrow cannon
by its actual object/asset, not the unrelated decorative platform barrel.

Check the emitted quad's actual screen footprint, culling, material/texture
binding, alpha and depth against its source root/child transforms. Preserve
horizontal travel below the stage and local aiming rotation. Do not remove
rotation, clamp the source path, or add arbitrary offsets to conceal a matrix
error. Verify capture/launch alignment after the visual repair.

### Zebes acid

The source contact predicate is:

`acid_wait == 0 && fighter_root_y < acid_root_y + acid_child_y`.

Capture the rendered surface at gameplay depth, both acid translations, and the
fighter root on the same simulation tick. Check plane geometry/projection and
depth against that contact height throughout a rise/fall cycle. Root height
alone is insufficient. Do not change contact to feet/hurtbox bounds or invent a
damage offset. Preserve any source-designed root-versus-feet distinction.

The earlier Z-buffer repair addressed cliff occlusion; it did not close the
dome-like shape or submerged-before-damage reports. Any acid rendering fix must
be implemented in a native owner, not left in the generic stage adapter.

### Hyrule tornado

The imported source already has random selection among Twister map-object
positions and a repeated seven-state lifecycle. Preserve it rather than invent
new random positions or movement. Test seeded source inputs with the existing
update code: wait `1600 + Rand(1200)` ticks; summon 80; active lifetime
`520 + Rand(600)` decremented in Move/Turn; capture-aware Stop; subside 32;
return to the randomized wait. These are simulation ticks, not presents.

Verify a successful effect/obstacle creation, one actual capture/release, damage
and launch angle, lifetime expiration, cleanup, and subsequent creation. The
recorded descriptor readback of 14 damage/90 degrees did not observe a hit and
must not be used as full hazard closure. Shorten investigation with host state
tests/seeded inputs, not source gameplay changes.

### Yoshi's Island and Saffron materials

The reviewed two-cycle cloud classifier now recognizes the exact supported
cycle-1 shade modulation. Do not claim that this proves the owner's pixels are
fixed. Verify each of the three clouds' color/coverage, both spinning cutouts,
and the heart sparkles. Required particle textures/banks must be resident and
render natively; a cloud-only change cannot close missing particle cutouts.

For each affected source material, validate color and alpha independently,
including texture format, palette alpha, primitive/environment colors,
shade multiplication, draw head, blend/depth policy and cache identity. A5I3
upload engagement is necessary for that path, not sufficient visual evidence.
The notes flag singleton graded-alpha residency: verify coexistence of a cloud
and beam rather than accepting repeated reprepare or stale texture binding.

Saffron's opaque door material and its translucent head-1 content are different
source draws. Do not make all black/white texels or palette index zero
transparent globally. Remove only the owner-rejected haze panel through the
stage's generator/descriptor, using stable source identity; regenerate it.
Do not hand-edit generated packets or remove unrelated floor/door geometry.

### Sector Z Arwings and lasers

Use the owner's NEW report: Arwings are visible. Do not continue the older
"nothing spawned in 1,400 presents" investigation without reconciling its ROM.

The source enables rideable yakumono 1 only for eligible pilot/line/near-Z
states, updates it from the flight state, and disables it on departure.
Do not make all visible background flybys solid. Compare source gate states,
yakumono position, rider displacement, and the drawn ship on the same tick.

Check 2D and 3D laser paths separately. 2D emission uses two local gun positions
and the source axis rotation/remapping; 3D emission transforms a local forward
point through the flight basis before aiming. Compare expected world muzzle,
actual weapon spawn, and rendered muzzle. Wrong appearance alone cannot decide
whether the spawn or the ship transform is wrong. No screen-space offsets.

### CSS roster and sound

Yoshi-to-Mario: verify human-input CSS -> SSS -> battle retains the selected kind;
do not accept a scripted walk that forces the expected configuration. Keep this
candidate open until the exact ROM passes.

Kirby/Jigglypuff/Ness share the CSS residency/storage blocker. A stopped libfat
walk does not prove an intrinsic ten-fighter limit. Use canaries and the actual
read/wait state to distinguish memory corruption, allocator pressure, and I/O.
Integrate the existing compact-preview generator with a bounded VS CSS loader;
do not load full battle closures for every portrait or merely flip roster flags.
Keep preview pruning separate from battle data. Do not resume 1P to solve VS CSS.

Yoster's one-instrument garble needs source rendering/encoding/playback
separation: compare host PCM, encoded-and-decoded audio, and DS output; isolate
the instrument if already wrong in host PCM. Check refill evidence if only the
device mix is bad. Do not automatically repeat Mushroom Kingdom's PCM16 change;
codec fidelity and refill deadlines are different questions.

## Correct the performance investigation

`BUG_NOTES.md` incorrectly reasons that a 30-to-20 FPS change requires roughly
one extra VBlank's worth of CPU work. Deadline quantization invalidates that
inference: a small increase can cross the two-VBlank threshold. With the repo's
560,190-tick interval, an illustrative 1,115,000-tick workload is below the
1,120,380-tick two-interval budget; adding 12,500 reaches 1,127,500 and can miss
the deadline. These example values are not new project measurements.

Use the existing exact ROM and a synchronized short failing window. Record
absolute work start/end, GX/DMA wait, intended/actual present VBlank, audio refill
events, and remaining deadline margin; include native actor/particle and UI
costs outside the static packet. Native-only does not imply fast enough.
Do not rule out small costs by comparing them with an entire VBlank duration.

## Integration and acceptance

Batch independent work by existing owners (native boundary/fighters; geometry;
materials/actors; collision/audio/CSS), with one integration owner. Do not edit
shared generated outputs or compile inputs while a build/verifier is running.
One build at a time, no manual `-j` override, no new worktrees, no snapshots.
Use an existing ROM first for unmeasured questions; each new build has a written
prediction. Do not run a full match just to inspect one spawn or one triangle.

For each candidate require all of:

- Forbidden renderer implementation absent from actual ROM build inputs and
  linked executable; negative controls fail as intended.
- Required visible content and native states complete; no successful-empty
  substitute, fallback, unexplained native reject, or hidden admission failure.
- Source-equivalent gameplay/contact/attachment and the owner-approved haze
  omission; affected sibling paths tested.
- Applicable menu/battle cadence, memory, and relevant verifier requirements
  on the exact candidate; no source edits invalidate the captured build.
- Recorded ROM/ELF/config/asset identity, permanent required visual/performance
  evidence, and owner acceptance where needed.

Keep `docs/BUGS.md` lean. Preserve owner wording/order. Add only bold statuses of
20 words or fewer; mark **FIXED** only after the complete workflow. Put detailed
evidence in `docs/p2/BUG_NOTES.md`, correct the VBlank inference there, and update
the existing board/handoff instead of creating another permanent work queue.

The final handoff reports concrete changes, proofs, and remaining failures.
"Native path entered", "triangles emitted", "host test passed", or "ROM built"
alone is not completion. Publish only the verified `smash64ds.nds` configuration.