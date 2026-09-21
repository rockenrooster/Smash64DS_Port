# Remaining `BUGS.md` diagnosis and coding-agent handoff — 2026-09-19

This document covers **every entry still present in `docs/BUGS.md`** at the time
of this investigation. It is a diagnosis/fix brief, not a claim that the owner
has accepted anything as fixed.

## Snapshot and rules

- Repository HEAD at investigation start: `aac91275fbf`.
- Current root ROM: `smash64ds.nds`, 66,466,816 bytes, SHA-256
  `00E8777758F926EEA79A282B15C86B4AFC67DFBC4E2F4A7A6194C2FF153BFBCE`.
- The working tree is heavily dirty and contains active owner/agent work. **Do
  not reset, clean, regenerate over unrelated work, or replace dirty files with
  HEAD versions.** Treat the current tree as the source of truth.
- The three startup/crash reports diagnosed in the preceding crash batch are no
  longer in `BUGS.md`; do not re-add them. Sector Z's separate intermittent
  character-intro crash is still in the owner queue and is covered below.
- `docs/BUGS.md` is the owner's open queue. Do not delete entries. Do not mark
  `FIXED` without the full requirements in `BUG_FIXING_PROCESS.md`, including
  natural-path proof and owner acceptance where required.
- Every ROM, including diagnostics, remains native-renderer-only. Never make a
  missing visual disappear from diagnostics by enabling a compatibility
  renderer, interpreter, software compositor, or fallback.
- `decomp/` is read-only. Derive the observable contract from it; implement the
  DS-native replacement in `src/nds`, `src/port`, the import seam, or the
  generator that owns the data.
- For visual bugs use the full chain: **trigger -> arguments/state -> asset ->
  update -> native owner -> GX triangles/material -> pixels**. For audio use
  **trigger -> FGM id/program -> generated pack -> DS playback frequency/
  envelope/loop -> audible PCM**.

### Status terms used below

- **CONFIRMED GAP** — current source contains a concrete missing/stubbed owner or
  implementation seam that explains the report.
- **MEASURED ROOT CAUSE** — existing current/relevant measurements identify the
  first divergence well enough to prescribe the owning fix.
- **IMPLEMENTED / REPRO FIRST** — current tree already contains the expected
  implementation or strong later proof; reproduce on the current ROM before
  editing again.
- **IMPLEMENTED WIP / VERIFY** — current dirty tree contains an unaccepted
  candidate that directly targets the report. Preserve it and finish its proof
  rather than independently reimplementing the same feature.
- **FIRST DIVERGENCE NEEDED** — the symptom is real/open, but static evidence is
  not sufficient to pick one safe code change. The required discriminating
  witness is listed.
- **OWNER DEFERRED** — investigate-only record. Do not spend implementation time
  unless the owner reactivates it.

## Recommended implementation order

1. **Finish/verify existing dirty candidates first.** Yoshi's egg/egg-lay/entry
   owners, the current Data/Characters/VS Record menus, Ness self-hit, Sector Z
   intro repair, Link boomerang, and Kirby copy hats already have substantial
   current work. Do not create parallel replacements.
2. **Close confirmed missing native seams.** Kirby Vulcan Jab, Pikachu Down-B
   Thunder head/trail/shock, Jigglypuff Sing, and battle score alerts have
   concrete missing-owner/stub evidence.
3. **Repair CSS load policy as one shared change.** A resumable compact preview
   load addresses the worst frame, BGM pause, and the reason for the 13-tic
   hover debounce. Cache the door underlay separately.
4. **Then do focused state/material fidelity bugs.** Link/Samus/Pikachu model or
   VFX state should be diagnosed with one natural move at a time before changing
   shared renderer policy.
5. **Stage material/transparency work last**, except crash/corruption. Peach's
   Castle and Zebes are explicitly owner-deferred.

---

# Main menus

## Data menu goes solid blue instead of showing `Characters` and `VS Record`

**Status: IMPLEMENTED / REPRO FIRST.**

The older diagnosis is obsolete. The current tree has a native Data screen in
`src/nds/nds_menu_shell_data.c`; its row text is explicitly `CHARACTERS`,
`VS RECORD`, and conditionally `SOUND TEST`. `nSCKindCharacters` and
`nSCKindVSRecord` are registered for the menu shell in
`src/port/nds_scene_manager.c`, and the router has native
`ndsMenuShellRunCharacters()` / `ndsMenuShellRunVsRecord()` paths. The Data
screen itself checks `ndsSceneManagerFind(want_kind)` before transitioning.

The current presentation uses UI-kit font rows on the blue field; full original
Data-menu sprite art is still a TODO, but that does **not** explain a completely
blank blue screen anymore.

**Agent instruction:** first reproduce this exact row on the current root ROM.
If it is still blank, do not write another Data menu. Instrument the current
screen's populate path and capture:

1. current shell screen id / entry to `ndsMenuShellPopulateData`,
2. UI-kit enter/surface/text placement success,
3. `gNdsSceneManagerRejectCount`, and
4. whether the text OBJ layer is enabled/visible.

Fix the first current failure only. A source/scene-manager rejection is a router
bug; successful row composition with no pixels is a UI-kit/OBJ-layer bug.

**Acceptance:** Data entered naturally from Mode Select; Characters and VS
Record visibly selectable; each opens and returns; no scene reject; native-only
build; owner visual check.

## VS Options — Item Switch is missing and must be added now

**Status: MEASURED ROOT CAUSE.**

The Item Switch screen is already implemented. `src/nds/nds_menu_shell_items.c`
has the source-equivalent 16 rows (appearance rate + 15 item toggles), the router
and scene manager admit `nSCKindVSItemSwitch`, and commit logic exists in the
match-config seam.

The row is hidden in `src/nds/nds_menu_shell_vsoptions.c` by the original
`LBBACKUP_UNLOCK_MASK_ITEMSWITCH` / 100-battle unlock policy. That policy now
conflicts with the owner's explicit requirement that Item Switch be present.

**Agent instruction:** change only the VS Options availability policy so Item
Switch is shown/reachable now. Do **not** rewrite the Item Switch screen and do
not fake the unlock by mutating unrelated backup/save progression. Preserve the
source screen's rate/toggle semantics once entered.

**Acceptance:** fresh/locked save still shows the row; A enters Item Switch;
rate wraps over all six source values; both LEFT and RIGHT flip item rows as the
source does; B commits and returns; settings affect the next match.

## VS Options — Damage percentage should change about 3x faster

**Status: MEASURED ROOT CAUSE / POLICY CHANGE.**

The current native VS Options handler changes the damage ratio by **1 percentage
point per accepted repeat** over its 50..200 range. Its directional repeat helper
is shared by many menu rows, so globally tripling that helper would change
unrelated menus.

**Agent instruction:** make Damage-specific held-input adjustment about 3x the
current rate while keeping a fresh tap precise. Preferred behavior:

- tap LEFT/RIGHT: -1/+1,
- held LEFT/RIGHT: either three damage steps for each current repeat event, or a
  damage-row-local repeat cadence approximately three times faster,
- keep source bounds/wrap/clamp behavior unchanged.

Do not speed the global menu-repeat helper.

**Acceptance:** measure time to traverse a fixed interval (for example 100 ->
150) before/after; new held time should be approximately one third while single
taps remain one point.

---

# Character Select Screen (CSS)

The three CSS complaints below are two implementation defects, not three
independent renderer bugs.

## Low FPS / flashing during gate openings

**Status: MEASURED ROOT CAUSE; dirty tree has a PARTIAL optimization.**

The shipping-configuration probe in
`artifacts/performance/2026-09-17_p2-css-owner-bug-list/README.md` measured
159/1,651 CSS frames (9.6%) missing 60 Hz, with the sync phase reaching about
5.81 frame intervals. During a door slide, the panel under the doors is static,
but `ndsMenuShellCssStepDoors()` repeatedly re-blits panel data from NitroFS so
the moving doors can erase/repaint their old pixels.

The current dirty `src/nds/nds_menu_shell_css.c` batches the sliding panels into
one `ndsUiKitBlitSurfaces()` call per frame, which reduces call/open overhead, but
the underlying panel bytes are still streamed/re-hashed/re-flushed again on each
slide tic. The static underlay has not actually become resident.

**Agent instruction:** finish the fix by keeping the already-composed slot panel
underlay in a resident/static layer or a bounded in-memory cached surface for the
gate animation. Door motion should update only the door geometry/changed pixels;
it should not touch NitroFS each tic. Preserve current panel content and gate
timing.

**Acceptance:** same visual gate animation, no flashing, no panel read during
mid-slide tics, and a new VBlank histogram showing the gate phase no longer
produces multi-frame I/O spikes.

## Music pauses/resets when moving the cursor to a new fighter preview

**Status: MEASURED ROOT CAUSE.**

In the shipping compact-preview path,
`ndsMNPlayersVSPreviewAcquireResidentKind()` wraps an entire synchronous fighter
closure load with `ndsAudioBgmSuspendForBlockingLoad()` / resume. The measured
preview phase reached 4,799,488 ticks, about **8.59 60-Hz frames**, and one CSS
visit recorded **8 BGM suspend/resume pairs**. The fence is protecting the audio
system from a real blocking load; removing it by itself previously caused a more
severe stream failure.

**Agent instruction:** make the **compact/FPC2 preview acquisition itself
resumable**. Slice file streaming, retained-span/fixup work, external closure
loading, and final owner publication over bounded CSS tics. The non-compact path
already demonstrates the intended sliced-loader shape. Publication must be
atomic: an incomplete resident kind is never exposed as ready.

Only after the load can no longer monopolize a frame should the preview-specific
BGM blocking fence be removed or reduced. Do not simply delete the suspend call.

**Acceptance:** browse all 12 fighters repeatedly with BGM uninterrupted; zero
audio seam errors; no blocking preview frame; preview owner remains generation-
safe and retirement-safe.

## Delay between hover and 3D fighter preview rendering

**Status: SAME ROOT CAUSE AS THE CSS BLOCKING LOAD.**

`NDS_PLAYERS_VS_PREVIEW_DWELL_TICKS` is 13. It was intentionally chosen so a
fast cursor sweep never begins the expensive synchronous load. The same measured
visit accumulated 70 deliberate wait tics. With the current blocking loader,
that debounce is defensive rather than accidental.

**Agent instruction:** do **not** independently set dwell to zero while the load
is still blocking. First land the resumable compact-preview loader above. Then
reduce the dwell to the minimum that prevents pointless churn—likely a small
1–2-tic stability filter—and measure browse behavior. Cancellation must safely
retire a partially loaded request without leaking owner images, file refs, or
resident-block state.

**Acceptance:** cursor settles on a portrait and the model starts appearing
promptly; uninterrupted rapid sweep does not cause load thrash; no BGM pauses;
resident refs return to zero at CSS exit.

---

# Yoshi

Important: the dirty tree already contains active Yoshi native-owner work. New
files include generated/native owners for the shared egg, Egg Lay, and entry egg,
and the import/adapter files are modified to route them. **Preserve that work.**

## Yoshi is invisible

**Status: FIRST DIVERGENCE NEEDED for the current battle symptom.**

Do not reuse the old FPC2 CSS-preview diagnosis: the pair-table/raw-size FPC2
problem was repaired and later CSS proof drew all 12 preview kinds, including
Yoshi. Likewise, old matrix-angle failures are historical unless a current
witness reproduces them.

**Agent instruction:** run the shortest natural battle case with Yoshi in plain
Wait/Run before any special move. On the first invisible frame capture:

1. `FTStruct` fkind, detail, status/motion and source visibility flags,
2. selected native fighter owner and root-program id,
3. selected root vector / first validator or decline reason,
4. fighter triangle delta for that presented frame, and
5. owner image generation/residency.

If source says visible but the owner emits zero triangles, fix native owner/root
program selection or its preparation. If source itself is hidden, trace who set
the hide/model-part state. Do not add a Yoshi-specific "force visible" bypass.

## Up-B egg shells are not rendering

**Status: IMPLEMENTED WIP / VERIFY.**

The current dirty tree contains a dedicated native owner generated from
YoshiModel asset 338, root `0xA860` (4 vertices / 2 triangles, CI4 64x64). That
root is shared by the Egg Throw weapon and other Yoshi egg presentation paths.

**Agent instruction:** preserve the owner and finish its natural Up-B proof.
Confirm the Egg Throw weapon is created, its exact root is admitted, texture
bind succeeds, and hardware triangles are added while the shell is visible.
If the object exists but root admission is zero, repair adapter identity; if
triangles draw but shell is invisible, inspect material/texture/alpha state.

## Grab attacks turn Yoshi invisible

**Status: IMPLEMENTED WIP / VERIFY.**

The owner generator now carries Yoshi Catch/Throw root programs for the source
hidden-part state. Catch/Throw draw hidden part 4 (joint 9 / DL `0x2800`), and
Throw also changes joint 7. Earlier synthetic-root alias mistakes were corrected;
do not recreate those tables by hand.

**Agent instruction:** test natural grab -> forward throw and grab -> back throw
on both details. Capture selected Yoshi root program and verify the body continues
to add triangles. Fix only if current program selection or resolved-root ownership
diverges.

## Neutral-B makes Yoshi invisible and the egg is invisible

**Status: IMPLEMENTED WIP / VERIFY.**

Two presentation pieces are involved:

- Yoshi's body enters the same hidden-part family covered by the Catch-derived
  root program during Egg Lay motions.
- The dirty tree has a separate Egg Lay native owner for the egg effect itself.

**Agent instruction:** verify both independently in one natural Neutral-B run:
body root-program engagement **and** egg effect native triangles/material. Do not
consider success of one proof as proof of the other.

## Character intro is invisible (egg hatching)

**Status: IMPLEMENTED WIP / VERIFY.**

The source intentionally hides Yoshi's normal body during the egg-escape intro;
the entry egg is supposed to replace it. The historical port failure was that
the Yoshi entry-egg effect had no native presentation owner. The dirty tree now
contains an EntryEgg owner generated from YoshiSpecial2 asset 354, root `0x0530`,
and the import seam restores the effect display callback.

**Agent instruction:** finish natural entry/respawn proof. The expected result is
not "force Yoshi visible"; it is source-hidden body + visible animated entry egg
then normal body restore at the source transition.

---

# Link

## Character-intro column VFX is opaque; it should be transparent

**Status: MATERIAL/ALPHA FIRST DIVERGENCE NEEDED.**

Link's entry effects are already in the source/EFDesc resolver family, and the
native entry/rebirth renderer has graded-alpha texture support. Therefore the
problem should be treated as a per-effect material/alpha fidelity failure, not a
missing generic transparency feature.

**Agent instruction:** on Link's natural intro capture the exact entry effect
root and source material alpha/combine/render mode, then compare the native
texture format, polygon alpha and blend state. Preserve source zero-alpha as
invisible (DS alpha zero is wireframe if emitted, so zero-coverage geometry must
be skipped). Do not globally lower entry-effect alpha.

## Neutral-B makes Link invisible while throwing/catching the boomerang

**Status: IMPLEMENTED / REPRO FIRST.**

The current owner generator has a Link `SpecialN` root program that includes the
held-boomerang modelpart and foreign LinkBoomerangModel root. Existing probes
show the Link Special-N program and boomerang native owner engaging with no
native rejection through long runs. That proves the mechanism exists, but not
that the fighter body stayed visually present on every throw/catch frame.

**Agent instruction:** reproduce on the current ROM before editing. During
`MissingBoomerang` and `CatchingBoomerang` motions, log the Link root-program id,
body triangle delta, and boomerang root delta separately. If the body alone drops
to zero, fix the Link model-part/root vector selected for those motions; do not
touch weapon physics or add a boomerang-specific visibility override.

## Up-B effects render the wrong color; they should be orange tinted

**Status: PRESENTATION FIDELITY.**

Link Spin Attack already reaches a native path; prior probe evidence records
native Spin draws with no texture rejection. The open dimension is color, not
geometry/admission.

**Agent instruction:** derive the source `SpinAttack` effect's live prim/env/
material/light color for the relevant frames and feed that state into the exact
native owner. Do not hard-code "orange" as an arbitrary DS color if the source
already animates the tint; preserve source frame/material behavior.

---

# Pikachu

## Neutral-B should have transparency/alpha and no hard edges

**Status: NATIVE OWNER EXISTS; TEXTURE/ALPHA FIDELITY REMAINS.**

The current adapter already has exact native Thunder Jolt owners for the air
weapon, grounded continuation, and Jolt effect. The report is therefore not a
missing-weapon bug.

**Agent instruction:** decode the source Jolt texture/combine alpha semantics and
compare them against the current DS-native texture encoding. If source coverage
is graded, use a graded-alpha DS format such as A5I3/A3I5 as appropriate rather
than reducing it to one-bit PAL transparency. Keep the change scoped to Jolt or
the proven semantic texture class.

**Acceptance:** screenshot/capture on dark and bright stage backgrounds; soft
edge survives; no texture reject; trajectory/collision unchanged.

## Down-B effect does not render and sometimes crashes

**Status: CONFIRMED GAP in native presentation; crash must still be localized.**

`ftPikachuSpecialLwMakeThunder()` creates a `wpPikachuThunderHead` weapon. The
effect resolver knows `dEFManagerPikachuThunderTrailEffectDesc` (PikachuModel)
and `dEFManagerPikachuThunderShockEffectDesc` (PikachuSpecial2), but the current
native adapter does **not** have Pikachu Thunder head/trail/shock owners analogous
to the already-landed Ness PK Thunder owner family. Existing Pikachu native code
is primarily the Neutral-B Jolt family.

**Agent instruction:**

1. reproduce once with exception/first-failure capture so the crash is not
   silently conflated with missing pixels;
2. prove the Thunder head weapon and each trail/shock effect object are made;
3. generate exact AOT/native owners for the source Pikachu Thunder roots and
   their live material/animation state, following the **pattern** of the Ness
   PK Thunder generator/executor but using Pikachu's own assets/contracts;
4. keep resolver/deferred mapping generation-safe.

Do not suppress Down-B effects to avoid the crash and do not reuse Ness geometry
by identity unless source proves the assets are actually identical.

## Electric damage effects seem to be missing overall

**Status: FIRST DIVERGENCE NEEDED; shared visual-template support already exists.**

The DS has a generic native visual-template path including an electric template,
so "electric effects missing" cannot safely be diagnosed as "no electric
renderer" from static code alone. It may instead be a missing maker/particle
mapping, an unengaged template, or a material/palette problem.

**Agent instruction:** cause one ordinary electric hit that is independent of
Pikachu's Down-B. Record the source effect/particle maker, template id, native
template submit/triangle count, and final material/alpha. Branch from the first
zero/wrong value. Keep this separate from the Pikachu Thunder owner work unless
the same source maker is proven.

## Strong side-A effects are not rendering

**Status: FIRST DIVERGENCE NEEDED.**

Do not assume this is the Down-B ThunderShock effect. The resolver comment for
PikachuSpecial2's ThunderShock identifies an up-smash shock use; the owner's
"strong side A" may exercise a different common hit/electric effect.

**Agent instruction:** enter the exact side strong/tilt/smash move naturally and
record which effect maker(s) fire. Then follow maker -> descriptor/template ->
native owner. Add/repair only the missing exact owner or common template proven
by that trace.

---

# Samus

## Shield rolling is invisible

**Status: ROOT VECTOR THEORY RULED OUT; PRESENT-AWARE PROGRAM PROOF NEEDED.**

As already annotated in `BUGS.md`, RollF/RollB carry only the TransN animation
flag; there is no drawing-hidden-part root vector to "fix." Samus morph/native
program infrastructure already exists.

**Agent instruction:** run a natural forward and backward roll with a witness on
the actual presented frame. Record status/motion, selected Samus root program,
modelpart state and fighter triangle delta. If the intended MorphUnfold/MorphBall
program is not selected, fix the status/modelpart-to-program seam. If it is
selected but emits zero, inspect that program's owner execution/matrix state.
Do not invent a Roll root vector.

## Down-B is invisible

**Status: ROOT VECTOR THEORY RULED OUT; PRESENT-AWARE PROGRAM PROOF NEEDED.**

The Bomb animation is `FTANIM_FLAG_NONE`; its source morph collapse is already
represented by the existing Samus morph programs. A prior Samus state tour
reached ground/air Bomb but did not close final present-aware visibility proof.

**Agent instruction:** same method as Roll, but separately prove Samus's body
morph and the bomb weapon. A visible bomb does not prove the body, and vice
versa. Fix model-program selection or exact bomb owner based on the first
divergence.

## B charge/shots render behind Samus's gun

**Status: NATIVE CHARGE SHOT EXISTS; DEPTH/ORDER FIDELITY.**

The port already has an exact Samus Charge Shot native owner. The complaint is
foreground ordering at the gun, not missing shot geometry.

**Agent instruction:** compare the source charge/shot world transform and draw
link/depth semantics with the native owner. Apply a **Charge-Shot-scoped**
foreground solution—e.g. the source-equivalent no-Z/painter behavior or a
camera-relative bias proven not to break world placement. Do not globally
disable Z for weapons. Verify both held charge and fired shot, small and full
charge, left/right facing.

---

# Kirby

## Not all Up-B VFX are visible

**Status: PARTIAL OWNER COVERAGE; PER-DESCRIPTOR CENSUS NEEDED.**

Final Cutter's travelling weapon has a native path, and the resolver knows the
Kirby Cutter Up/Down/Draw/Trail effect descriptors. The remaining report implies
that some subeffects are missing rather than the move being absent wholesale.

**Agent instruction:** instrument one natural Up-B and count each source Cutter
descriptor/weapon root independently. For every object made, require an exact
native owner and a post-triangle witness. Generate only the missing roots. Keep
live source DObj/MObj animation and weapon gameplay; replace immutable geometry/
state only.

## Grab attack / Up-down smash does not work

**Status: FIRST DIVERGENCE NEEDED; owner wording is ambiguous and must be
preserved.**

Do not turn this into a visual-only bug without evidence. For each natural action
(grab, up smash, down smash), first determine whether the source status is
entered and whether attack/catch collision becomes active. Then check animation
file resolution and native root/program output.

**Agent instruction:** build a tiny action matrix:

| action | status entered | motion/anim resolved | hit/catch active | native tris |
|---|---|---|---|---|
| grab | | | | |
| up smash | | | | |
| down smash | | | | |

Fix the first failing column per action. Do not use the many zero-valued legacy
symbol aliases in `reloc_backend_ftdata_symbols.c` as proof by themselves; show
that the live compact-motion loader actually requests the missing row.

## Kirby model texture changes colors when flying

**Status: FIRST DIVERGENCE NEEDED; likely lighting/material-state fidelity.**

**Agent instruction:** use one costume and detail, capture Kirby in Wait and the
reported multi-jump/flying state at the same camera/light conditions, and compare
the selected native root runs' lighting flag, vertex-color/FIFO_COLOR metadata,
material palette/texture id, and live color-animation state. The renderer already
supports source-unlit vertex-color runs; reuse that mechanism if the source
animation clears lighting. Do not add a "Kirby air color" special case until the
source state proves one is required.

## Neutral-A punch flurry VFX is not visible

**Status: CONFIRMED GAP.**

`src/port/reloc_backend_compat_shims.c` still defines a weak
`efManagerKirbyVulcanJabMakeEffect(...)` that simply returns `NULL`. The EFDesc
resolver knows `dEFManagerVulcanJabEffectDesc`, but no effect can be presented if
the maker is stubbed out, and no exact native Vulcan Jab owner was found in the
current stage/effect adapter.

**Agent instruction:** replace the weak NULL stub with the real source-equivalent
maker/import and implement the exact native owner for the Vulcan Jab descriptor's
immutable roots. Preserve its live position/lr/rotation/velocity/add parameters.
This is a confirmed owning seam; do not paper over it with a generic spark.

## Kirby Neutral-B SFX loop has the wrong pitch

**Status: AUDIO FIRST DIVERGENCE NEEDED.**

Kirby's relevant FGM ids, including `nSYAudioFGMKirbySpecialNStart` and copy
cues, are admitted to the DS FGM pack. The DS pack stores a per-entry playback
`frequency`; the runtime plays the sample at that generated rate and also carries
source duration/envelope/loop metadata. Static admission therefore does not prove
correct pitch.

**Agent instruction:** capture the exact FGM id requested during the audible
wrong loop, then inspect that entry's generated source sound index, net pitch/
frequency, loop point, note schedule and DS `soundPlaySample` rate. Compare with
the source audio program, not by ear alone. Fix the audio generator/entry if the
frequency is wrong; do not change Kirby's animation timing to tune pitch.

## Kirby Fox hat is not working; all hats should work

**Status: IMPLEMENTED / REPRO FIRST.**

The 2026-09-17 copy-hat work moved all ten deferred copy-hat bodies into per-slot
hat images, enabled all 11 copyable victim contexts, recovered heap/cadence, and
passed the four-CPU native-failure gate. Later actual-input CopyLink proof also
demonstrated both-detail hat loads and copied-weapon output.

**Agent instruction:** do **not** rebuild the hat architecture. Reproduce a
natural Kirby -> Fox inhale/copy on the current ROM. Capture copy id,
`copy_modelpart_id`, detail, `ndsRendererNativeEnsureKirbyCopyHat` result, hat
image generation, selected trio program and hat-root triangle delta. If Fox alone
fails, repair that exact generated hat image/identity. Then sample other victims
to ensure the shared fix remains intact.

---

# Jigglypuff / Purin

## Poke Ball spawn-intro color column texture colors are wrong

**Status: NATIVE OWNER EXISTS; LIVE MATERIAL/COLOR FIDELITY.**

Pikachu and Purin share the Master Ball entry-ray effect. The current adapter
has an exact native owner for the EFCommonEffects3 ray roots and snapshots the
live PRIM materials; it also has explicit candidate/material-reject counters.
Thus the likely failure is color/material translation, not missing geometry.

**Agent instruction:** during Purin's natural intro record each ray child's
source MObj prim/env values and material animation, then the native material
snapshot/palette actually submitted. Compare Pikachu and Purin if their expected
entry colors differ. Fix the snapshot/palette conversion at the owning effect,
not a global entry-effect tint.

## Up-B VFX is not visible

**Status: CONFIRMED GAP.**

`dEFManagerPurinSingEffectDesc` is admitted and deferred-resolved, but there is
no exact Purin Sing native submitter in the current native effect adapter. That
means the source can create the Sing note-ring object while the native-only
renderer has no legal presentation owner.

**Agent instruction:** generate an exact native owner for the PurinSpecial2 Sing
note-ring roots, retaining live DObj/MObj/MatAnim state and source lifetime. Prove
object creation separately from post-triangle output. Do not substitute a generic
music-note sprite.

---

# Ness

## Up-B self-damage attack crashes

**Status: IMPLEMENTED / STRONG PROOF EXISTS / REPRO FIRST.**

This row is still owner-open, but the later 2026-09-18 investigation found and
repaired a concrete compact-pack effect-descriptor bug: NessModel EFDesc fields
were source offsets while deferred recovery validated them as packed offsets,
which could leave a constructed effect without its required DObj/display path.
The fix maps through `ndsRelocNativeAssetAddress()` and keeps deferred descriptors
pending until the packed mapping exists.

Real-input evidence after the fix includes:

- 8/8 ordinary real Up-B executions completing without exception, and
- five independent real-input PK Thunder self-hit/Jibaku runs, each entering the
  self-hit state and recovering.

The PK Thunder head/trail/wave native presentation was also separately proven.

**Agent instruction:** before changing Ness again, reproduce self-hit on the
current root ROM. If it no longer crashes, leave the candidate intact for owner
verification. If it does crash, capture the exception/stack plus current compact
pack generation and mapped descriptor/DObj state. Do not add another Ness-only
NULL guard or suppress PK Thunder effects.

---

# General

## Green impact-wave effect is intermittent on some characters

**Status: EXACT NATIVE OWNER EXISTS; INTERMITTENT FIRST DIVERGENCE NEEDED.**

The port already has `ndsRendererSubmitNativeImpactWave()`, five preloaded native
texture variants, variant selection through the effect object, and explicit draw/
fallback/texture-bind counters. Therefore this is not a blanket "impact wave is
unimplemented" defect.

**Agent instruction:** collect a small natural matrix of one working and one
missing character/event. For each impact record:

1. `efManagerImpactWaveMakeEffect` creation and requested index/variant,
2. effect object lifetime/display callback,
3. adapter candidate + selected variant,
4. native submit result / fallback count,
5. resident texture name and bind count, and
6. post-triangle output.

The first divergence decides whether the fix belongs in spawn logic, variant
identity, texture residency or owner admission. `verify-battle-playable-
damagefall-recovery.ps1` already contains a useful engagement pattern for a real
DownBounce impact. Do not replace the exact owner with a generic green quad.

## Score alert counters should render on the bottom screen (`Score -1`, `Score +1`, ...)

**Status: CONFIRMED GAP.**

The imported interface expects `efManagerBattleScoreMakeEffect()` plus stock
snap/steal effects, but the weak-stub census recorded BattleScore and the stock
snap/steal family as unresolved weak implementations in the port. The source has
a real BattleScore effect maker, but restoring the original top-screen particle
alone would still miss the owner's explicit DS presentation requirement: these
alerts belong on the bottom screen.

**Agent instruction:** preserve the source scoring trigger and signed score value,
replace the weak/no-op seam with a strong source-equivalent event, and feed a
small native bottom-screen HUD alert queue. The UI should display the actual
signed delta (`+1`, `-1`, etc.), with bounded lifetime and no gameplay coupling.
Do not recompute score by polling totals; the source event is the authoritative
moment/value.

**Acceptance:** generate +1 and -1 naturally in score mode; bottom screen shows
the right player/value exactly once per event; simultaneous/rapid events queue or
coexist deterministically; no top-screen compatibility particle required.

---

# Stages

## Peach's Castle — roof geometry is visible but its texture is missing

**Status: OWNER DEFERRED; diagnosis retained only.**

The geometry problem is no longer the primary report. Historical run-level proof
showed the roof triangles reach the native stage pipeline. The later stage audit
identified the likely remaining class for newly admitted Castle/Inishie surfaces:
texture use can resolve false through the `NO_TEXEL0`/texture-state path even
though geometry submits.

**If reactivated:** probe the roof runs' texture epoch, `use_texture`, semantic
texture key, resolved GL name, and material state. Fix the exact stage texture
state/epoch producer. Do not reopen geometry generation or make the roof opaque
untextured as a "fix."

## Zebes — acid/light transparency is too hard

**Status: OWNER DEFERRED; diagnosis retained only.**

The owner reports correct acid color but hard boundaries and a floor light that
looks like an opaque/hard trapezoid instead of a fading source. This is a graded-
alpha fidelity issue. Earlier stage work already found the general DS failure
mode: source intensity/alpha textures collapse to one-bit edges when converted as
ordinary PAL/color0 transparency instead of preserving the source coverage ramp.

**If reactivated:** derive the acid and light source combine/alpha channels and
use a graded-alpha DS representation (or the already-established bounded alpha
subdivision where that is the source-equivalent solution). Inspect each asset
independently; do not globally blur stage transparency.

## Mushroom Kingdom — left side platform is visible but mostly untextured

**Status: MATERIAL/TEXTURE PATH FIRST DIVERGENCE.**

Historical packet work proved the static side-brick geometry exists downstream;
this report now specifically says the platform is visible. That again points to
texture state rather than omitted triangles.

**Agent instruction:** identify the exact left static-platform binding/root and
capture its texture epoch, `use_texture`, NO_TEXEL0 decision, texture resolve,
and native bind. Compare to the corresponding textured central/right brick
surface. Fix the first state mismatch. Keep this separate from moving platform,
pipe and POW/Piranha hazard logic.

## Yoshi's Island — rotating textures expose white card backgrounds

**Status: TEXTURE-ALPHA FIDELITY.**

This has the classic one-bit-alpha shape. Prior Yoster work measured cases where
an I4/intensity texture's source coverage never reached the dedicated graded A5I3
upload and therefore fell back to hard thresholded edges.

**Agent instruction:** isolate one rotating-card root, capture its source combine
and texture format plus the DS texture-prepare semantic branch, and make the
graded-alpha path engage for that exact semantic class. Do not use a global
"white becomes transparent" rule.

## Yoshi's Island — Heart sparkle sprites are opaque/hard-edged

**Status: TEXTURE-ALPHA FIDELITY.**

Treat the sparkles separately from the rotating cards even if both eventually
reuse the same A5I3/A3I5 helper. First prove which source texture supplies alpha
and whether the particle/effect path quantizes it to one bit.

**Agent instruction:** require a source-alpha histogram/semantic match and a
post-conversion alpha histogram with more than two useful coverage levels before
claiming the edge is fixed.

## Yoshi's Island — main platforms / main floor-path geometry appears missing

**Status: DOWNSTREAM VISIBILITY FIRST DIVERGENCE, not a generator omission.**

Historical packet analysis found Yoster layer-1 root `0x49A0` in the generated
packet with 77 triangles across 22 runs, and run-emission witnesses showed the
geometry reaching the submit path. A later capture proved at least the main
floor itself visible at one camera, while the platform subset was not isolated.
Therefore the owner's current observation must be re-localized on the current
ROM; deleting/re-adding geometry in the generator is not justified.

**Agent instruction:** map the visibly missing platform/floor pixels to exact
binding/run ids, then capture current-frame run validity, matrices, polygon
format/alpha/depth, texture state and emitted triangle counts. If emitted but not
visible, investigate transform/cull/depth/material at GX; if a specific binding
is now not emitted, find the current admission divergence. Do not treat old
aggregate stage triangle counts as pixel proof.

## Sector Z — sometimes crashes during character intro

**Status: IMPLEMENTED / REPRO FIRST.**

The 2026-09-19 startup/crash batch found a Sector Z intro failure in the Arwing
factory: an effect-descriptor recovery branch could leave a known-file/empty-slot
descriptor with its callback disabled and no saved retry, producing a NULL DObj.
The shared resolver was repaired rather than adding a fighter-specific bypass.
Later diagnostics reached GO with Mario/Fox/Ness combinations on Sector Z, all
eight Arwing roots drew repeatedly, and pack/OOM/stage/native failure counters
were zero.

**Agent instruction:** because the owner row remains, reproduce several natural
Sector Z intros on the current ROM before touching the repair. If a crash still
occurs, break on `__excpt_entry` plus the existing pack/OOM/stage diagnostics and
name the new first failing constructor. Reopen Arwing recovery only if the stack/
object state points there.

## Saffron City — Pokémon garage door is always open

**Status: MEASURED ROOT CAUSE.**

The source `grYamabukiMakeGate` creates a live gate DObj and applies separate
open/close animation joints. The native stage descriptor includes a gate owner,
but the packet's world/geometry state is effectively baked from one pose. A
renderer-side fake timer would duplicate source hazard state and is the wrong
fix.

**Agent instruction:** keep the source gate actor/process authoritative and make
the native gate owner consume its **live DObj transform/animation state** each
frame, just as other dynamic native actors do. If the static stage packet cannot
legally represent that binding, move only the gate to a dynamic actor packet;
do not animate the whole Saffron stage packet.

**Acceptance:** observe at least one complete source-timed open -> closed -> open
cycle, with collision/hazard behavior synchronized with the visual door.

## Saffron City — Pokémon are missing VFX for their attacks

**Status: FIRST DIVERGENCE NEEDED per Pokémon/effect family.**

Do not conflate monster-body native owners with attack effects. The stage/item
logic can spawn Pokémon while their attack particles/EFDesc roots still lack
native presentation.

**Agent instruction:** choose each reachable Pokémon that visibly attacks and
record: monster kind -> attack state -> effect/weapon maker -> descriptor/asset
root or particle template -> native owner/triangles. Group only effects proven
to share the same source descriptor/template. Implement missing exact owners or
generic visual templates at that shared seam. Never hide an attack or replace it
with a generic effect merely to get a zero failure counter.

---

# Coding-agent execution checklist

Use this as the actual implementation handoff.

1. **Preserve the dirty tree.** Record `git status --short`, HEAD, root ROM hash,
   and the paths touched by the selected bug before editing. Never reset owner
   work to make a clean baseline.
2. **Do not work all 39 rows in one giant patch.** Batch shared owning seams:
   CSS loader policy; one effect-owner family; one fighter model-state family;
   one stage material family. Commit coherent progress when permitted.
3. **Before each edit, name the first wrong value.** If this report says
   `REPRO FIRST` or `FIRST DIVERGENCE NEEDED`, instrumentation comes before the
   code change.
4. **Reuse current candidates.** Especially Yoshi dirty owners, Ness Up-B,
   Sector Z Arwing recovery, Kirby hats, Link boomerang and current native menus.
5. **No native fallback escapes.** A missing effect requires a native owner; a
   rejected native owner is a failed candidate, not permission to route through
   a generic renderer.
6. **Regenerate from producers, never hand-edit generated artifacts.** If an
   owner/generator changes, run its `--check`/closure tests and the relevant pack
   generation checks. FPC2 stays FPC2; do not add stale FPC1 compatibility.
7. **Use natural input/status for final proof.** Synthetic status entry may
   diagnose a seam but cannot close the owner row.
8. **For every visual candidate prove both engagement and pixels.** Object
   existence, no reject, or triangle counts alone do not prove visibility,
   texture, alpha, color, depth or correct pose.
9. **For every audio candidate prove the actual FGM id and PCM contract.** Check
   frequency/pitch, duration, envelope, pan, loop/retrigger and stop reason.
10. **Use the widest relevant verifier once per coherent frozen batch**, per
    `docs/VERIFYING.md`; do not rerun the umbrella after every small edit. Changed
    linked code invalidates timing/cadence evidence even when source semantics
    remain valid.
11. **Keep `docs/BUGS.md` entries open.** Put detailed new findings/proofs in
    `docs/p2/BUG_NOTES.md`. Owner acceptance remains required for subjective
    presentation rows.

## Minimum owner verification matrix after implementation

The owner said they will verify the final `smash64ds.nds`, so hand them one
normal native-only root ROM after the coherent fixes are integrated, with its
SHA-256 and a short matrix:

| Area | Required owner path |
|---|---|
| Menus | Data -> Characters / VS Record; VS Options -> Item Switch; hold Damage +/- |
| CSS | sweep all 12 previews with BGM; open/close gates repeatedly |
| Yoshi | idle visibility; grab+throws; Neutral-B; Up-B egg; match intro |
| Link | intro column; Neutral-B throw/catch; Up-B color |
| Pikachu | Neutral-B edge alpha; Down-B full cycle; electric hit; side strong A |
| Samus | forward/back roll; Down-B; small/full B charge and fired shot |
| Kirby | Up-B; grab/up/down smash; multi-jump color; jab flurry; Neutral-B audio; Fox copy hat |
| Jigglypuff | intro rays/colors; Sing VFX |
| Ness | PK Thunder self-hit/recovery |
| General | impact waves from multiple fighters/events; +1/-1 bottom-screen score alerts |
| Stages | Mushroom left platform; Yoshi Island cards/sparkles/floor/platforms; Sector Z repeated intros; Saffron door cycle + Pokémon attacks |

Peach's Castle roof texture and Zebes transparency remain **owner-deferred** in
the current queue and should not consume implementation time unless the owner
explicitly reactivates them.

## Bottom line

The remaining queue is not 39 independent unknowns. The highest-confidence
shared seams are:

1. **CSS synchronous I/O/residency policy** — explains the worst frame, BGM
   suspension, and deliberate hover delay; gate underlay re-read is the second
   CSS I/O bug.
2. **Missing exact native effect owners** — confirmed for Kirby Vulcan Jab,
   Pikachu Down-B Thunder family, Purin Sing, and the score-alert path; current
   Yoshi dirty work is already closing the same class for its eggs.
3. **Fighter root-program/model-part selection** — the likely owner for any
   still-reproducing Yoshi/Link/Samus body disappearance, but must be proven on
   the current presented frame before another table is added.
4. **Texture/material fidelity** — Link entry/Spin tint, Pikachu Jolt alpha,
   Purin entry colors, Mushroom/Castle textures, and Yoshi Island/Zebes alpha
   are all presentation-state problems, not reasons to re-add geometry.
5. **Existing later candidates need owner verification before more code** —
   Ness self-hit, Sector Z intro, Kirby copy hats, Link boomerang, and the Data
   menu have substantial current implementation/proof and should be re-tested,
   not rewritten by default.

