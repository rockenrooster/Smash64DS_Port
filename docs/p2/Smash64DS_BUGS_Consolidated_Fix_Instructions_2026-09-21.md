# Smash64DS — consolidated remaining-bug diagnosis and coding-agent instructions

**Consolidation date:** September 21, 2026
**Repository:** `rockenrooster/Smash64DS_Port`
**Shared review baseline, rechecked through GitHub:** `master` at `77d868b0d9be8022b3c6950ef9213f6e21629a50`
**Authoritative owner report:** `docs/BUGS.md` — not a root-level `BUGS.md`
**Coverage:** all 23 complaint rows: 21 active rows and 2 owner-deferred stage rows. The two Zebes visual defects are separate subparts of one deferred row.
**Deliverable:** one consolidated implementation handoff. This is not a repository patch, new execution board, runtime verification receipt, or acceptance of any fix.

## Read this before implementation

This handoff merges the detailed prior review (**Review A**) and the additional uploaded review (**Review B**). Shared instructions appear once; unique diagnostics, prerequisites, source locations, sibling checks, and acceptance conditions are retained. Disagreements are resolved explicitly below, or kept as distinct diagnostic branches where neither review establishes which symptom the owner observed.

| Input | Filename | SHA-256 |
|---|---|---|
| Review A | `Smash64DS_Remaining_Bugs_Fix_Instructions_2026-09-21.md` | `63a2a5814d1cfae0708ca836c7e4b9adcaf7b23e0aeb091b997ee00785cc77ce` |
| Review B | `BUGS_REMAINING_FIX_INSTRUCTIONS_2026-09-21.md` | `0ea36599dc03fe8377146d2a88be2a7c774b035f2c34915e618a796854d9640f` |

**Evidence boundary.** Both reviews concern the same commit. GitHub rechecks for this consolidation confirmed that `master` still resolves to that commit and re-read the owner queue plus the conflicting Damage-step, Thunder event/trail, Kirby throw-alias, Results-emblem, and Castle-proof evidence. Other detailed findings are carried forward from the reviews and their cited repository material; they were not all independently re-investigated in this consolidation. No ROM was built, emulator run, fresh screenshot/audio captured, or repository file changed. All quoted runtime observations remain historical, scoped to the binaries in their original receipts. [Queue][bugs] · [Repair log][repairlog]

Use these evidence labels throughout implementation:

- **Code-confirmed:** the reviewed source contains the named omission, alias, or execution-path difference. This does not automatically prove every reported symptom has that sole cause.
- **Recorded runtime evidence:** a repository receipt reports the observation for its named candidate and scenario; do not silently promote it to a new candidate's proof.
- **First divergence required:** the source identifies what to inspect, but a coherent event-local witness must choose the repair.
- **Owner deferred:** retain the instructions, but do not implement or close the row until the owner reactivates it.

`PROJECT_GOAL.md` owns product scope; `docs/VERIFYING.md` and `docs/BUG_FIXING_PROCESS.md` own verification and closure. The existing execution board remains the live work cursor. These IDs only map the reviews to the owner queue. Empty headings create no task, and complaints already removed by the owner must not be resurrected. [Workflow][workflow] · [Verification][verifying] · [AGENTS][agents]

## Consolidation decisions — do not reintroduce the conflicting instructions

| Topic | Difference between reviews | Consolidated instruction and basis |
|---|---|---|
| M01: disabled DATA entries | A proposed skipping disabled focus and new disabled presentation; B proposed keeping the source focus/layout and blocking activation. | Required outcome is no transition into Characters/VS Record. Use one local activation guard; preserve existing visible rows, navigation, and Sound Test policy. Focus-skipping/new art is not required by this handoff. Do not unregister scenes globally. |
| M02: Damage limits | A says wrap; B says clamp at min/max. | **Wrap, not clamp.** The current C adds/subtracts the inclusive domain size after crossing a bound. Preserve 50–200 wrapping, taps ±1, and Damage-only held step ±5. The current adjuster takes one `direction` argument. [VS options][vsoptions] |
| M03/M04: loader design | Both call for slicing; A describes conceptual transaction phases while B stresses reuse of existing state. | Extend the existing resident-block transaction/cancel/refcount mechanism to the compact loader. The phase names in M03 are conceptual, not a request for a second loader framework. Cancellation must preserve requests still needed by another slot. [CSS import][css_import] |
| P01: which Jolt owner | B emphasizes air/effect generators; A identifies a distinct ground-segment path. | Preserve all three owners and start at the **ground** executor/generator for the terrain complaint. A repaired quad helper does not establish ground-triangle coverage. A path difference is not yet proof of the exact alpha defect. [Ground owner][jolt_ground] |
| P02: missing blue burst and trail | B leaves burst identity open and treats role-4 callback rejection as current. | Source Hit motion explicitly emits **ThunderAmp**; its source maker starts script **0x74**. The later repair-log entry reports DLLINKS admission plus the Ness-tail owner and role mask 7. Preserve/recheck that repair; do not reimplement it as an assumed open gap. Identify the first missing ThunderAmp consumer. [Hit script][pika_motion] · [Makers][efmanager] · [Later repair entry][trail_repair] |
| P03/K04: ITCommonData readiness | B says item-core landing removes the old prerequisite; A warns that un-suppressing a call is not complete closure. | Restore the complete family, but verify the exact build's resident data, descriptor mapping, native roots and lifetime first. Item-core availability alone does not prove every required root or configuration. Pair CaptureStar/LoseStar coverage; keep the entry warp star separate. |
| K01: Kirby slam | B asks for generic capture localization; A finds the explicit Kirby-to-common ThrowF alias. | Inspect the alias first and prove its compiled/natural call path. It is a concrete semantic mismatch, but the victim teleport remains causally unproven until the paired state/position trace. Do not turn the report into unrelated up/down-smash work. [Catch import][catch_import] · [Source throw][throw_source] |
| K05: Fox-copy crash | B preserves an earlier pre-shot hat eligibility halt; A prioritizes the owner's newer visible-hat/firing-crash report. | Distinguish pre-shot draw eligibility, firing-time gun roots, joint lookup and weapon construction. Use the historical stage-2 clause witness only if that failure still occurs; do not require re-solving an already-working hat before investigating the shot. |
| R01 / B's G1: “first-place emblem” | A investigates `llMNVSResultsWinnerSprite`; B investigates `FTEmblemModels` and claims its native owner is missing. | These are **two different source objects**. Keep both branches under R01, identify the missing pixels, and repair the affected branch. Search absence is not proof of a missing compiled native owner. Do not replace either object with the other. [Results source][results_source] |
| R02/R03/K06: Results poses | Both identify downstream demo/motion/program risks; imported source selection was sometimes treated as excluding a selection bug. | Source Win/Lose/No-Contest policy is the oracle, not proof of the effective compiled mapping. Verify transfer kind, actual status and motion before changing native root programs. Keep Link Claps and Luigi Win2 repairs. |
| S02: Castle ramps | B calls this implemented and suggests treating the owner row as stale after a passing probe. A keeps the refined knockback report open. | **Keep active.** The repair log documents broad wall fixes and a moving-board/tower proof, not full acceptance of both upper ramps in knockback/fall. The current queue explicitly says “Still not fixed.” Reuse code/proof within its scope; capture the exact remaining scenario. [Queue][bugs] · [Castle receipt][castle_repair] |
| S03: Zebes gradients | B raises existing-subdivision and apparently opaque source-light concerns; A prescribes tracing spatial coverage. | Keep both subparts deferred. Treat those concerns as constraints/hypotheses to check, not proof that the owner's mismatch is imaginary. Constant polygon alpha alone does not decide whether texture, vertex or filtering stages contain a gradient. |
| S04: Saffron gate | Both point toward visual animation, but sparse root samples can be mistaken for a complete animation diagnosis. | Retain the recorded logic/collision cycle, inspect the **full gate tree**, then choose animation decode, instance binding, native matrices or occlusion as the first bad boundary. No renderer-owned gate timer. |
| Verification | B asks for a fresh build for every batch; A preserves qualified incremental reuse. | Follow `VERIFYING.md`: focused checks while editing, a matched native-only candidate, and one widest relevant verifier per frozen integrated batch. A new clean directory is not mandatory per row. No ROM rebuild is required merely for this document. |

### ID crosswalk

Canonical IDs follow Review A. Review B uses the same letters without leading zeroes (`M1` → `M01`, `P2` → `P02`, and so on), except its **G1/G2/G3 are R01/R02/R03** here. Review B's **K6 remains K06** and shares the Results implementation batch without losing its own acceptance row. S03a/S03b are the two Zebes subparts, not additional owner rows.

---

## 1. Complete issue map

| ID | Current complaint | Diagnosis / first repair boundary | Implementation status |
|---|---|---|---|
| [M01](#m01) | Characters and VS Record must not be selectable | Local DATA activation eligibility; keep source rows/navigation | Active; implementation of those destinations remains deferred |
| [M02](#m02) | Damage percentage should step 5x, not 3x | Held-repeat Damage step is currently 3; preserve single-tap behavior | Active; direct source change |
| [M03](#m03) | CSS music pauses/resets on new previews | Compact preview closure blocks synchronously inside BGM suspend/resume | Active; confirmed code and prior timing evidence |
| [M04](#m04) | CSS hover-to-preview delay | Blocking closure plus 13-tic dwell; shares M03's loader | Active; do not solve with debounce alone |
| [L01](#l01) | Link slash damage VFX at wrong positions | Identify exact effect, then contact/attachment coordinate ownership and native matrices | Active; runtime localization needed |
| [P01](#p01) | Grounded Thunder Jolt has hard edges | Ground owner uses a different binding path from repaired quads | Active; concrete path difference, not yet a proven coverage root cause |
| [P02](#p02) | Down-B lacks blue explosion on Pikachu | Source self-hit event is `nEFKindThunderAmp` → particle script `0x74` | Active; exact producer identified; first missing consumer needs tracing |
| [P03](#p03) | Poké Ball spawn intro VFX absent | Pikachu entry branch omits the Poké Ball constructor | Active; confirmed caller omission |
| [P04](#p04) | Pikachu face/body color mismatch | Material/light/normal/palette and packet-state comparison | Active; shared investigation with K02/J01 |
| [K01](#k01) | Kirby grab slam teleports victim | Catch import aliases Kirby's special forward-throw status to common ThrowF | Active; confirmed semantic mismatch; correlate with reported teleport |
| [K02](#k02) | Kirby face/body color mismatch | Same material audit as P04, with Kirby-specific model/costume evidence | Active; not a proven lighting-only bug |
| [K03](#k03) | Kirby jab flurry VFX misplaced | Existing Vulcan owner: source origin/velocity/age versus submitted transform | Active; visibility repair is not placement proof |
| [K04](#k04) | Kirby spit-out star invisible | CaptureStar and neighboring LoseStar calls are macro-suppressed | Active; repair complete shared family; spit-star remains the owner row |
| [K05](#k05) | Copied Fox hat visible, pistol crashes | Firing-frame dependency/attribute dereference or new gun/model-part root | Active; highest-priority crash investigation |
| [K06](#k06) | Kirby Results pose wrong | Demo motion/status and native root program, including both reachable win variants | Active; part of R02, separately accepted |
| [J01](#j01) | Jigglypuff face/body color mismatch | Shared material audit, independent Purin model/texture evidence | Active |
| [R01](#r01) | Results first-place emblem absent | Identify WinnerSprite badge versus FTEmblemModels series object; follow the matching native path | Active; two distinct source objects, no assumed missing-owner verdict |
| [R02](#r02) | All Results poses/animations incorrect | Full demo-script/model-part/program/animation closure, not just scene entry | Active; existing partial fixes must be retained |
| [R03](#r03) | No Contest fighters should clap | Source already selects DemoLose for all; verify transfer kind and downstream animation | Active; don't replace the correct source rule |
| [S01](#s01) | Restored Castle roof geometry lacks texture | Material/texture state for newly admitted roof runs | **Owner deferred: diagnose/plan only** |
| [S02](#s02) | Castle blue side-ramp collision fails during knockback/fall | State-specific map callback, swept contact and line-space boundary | Active; earlier wall fixes do not close this report |
| [S03](#s03) | Zebes acid edges and ground-light gradients too hard | Two source alpha/coverage paths; acid color is already corrected | **Owner deferred: both subparts plan only** |
| [S04](#s04) | Saffron gate looks permanently open | Gate animation → local DObjs → native moving bindings → pixels | Active; recorded logic/collision cycling is not the missing behavior |

Priority is about safe dependency order, not permission to omit a row. Address K05 before repeated crash-prone testing, and K01/S02 before treating gameplay as correct. Batch M03/M04 together; batch the three color reports together; batch K06/R02/R03 while retaining separate acceptance results.

---

## 2. Preserve current repairs and avoid stale diagnoses

Read the append-only repair record chronologically, particularly its latest September 21 entries. The older September 19 diagnosis describes work that has since landed, and sometimes uses wording the owner has subsequently clarified. [Repair record][repairlog] · [Earlier diagnosis, historical only][oldguide]

The following are **not** justified starting points for this task:

- “Add a native renderer for all these effects.” DamageSlash, Kirby Vulcan Jab, the Thunder Jolt family, and substantial Pikachu Thunder presentation already have native implementations. This queue now includes placement, coverage, and a distinct self-impact particle effect.
- “Thunder's role-4 callback is still refused.” That diagnosis predates the repair-log entry **Effect DLLINKS admission + Ness tail owner**, which reports admission, the Ness `0x8F98` owner, and Thunder role mask 7. Recheck the current candidate's engagement when needed; do not reinstall the same repair from the older note. [Later trail evidence][trail_repair]
- “Kirby's hat is invisible.” The current report says the hat is visible and the **shot crashes**. Inspect the firing transition and dependencies rather than repeating the hat-visible repair.
- “Results needs its first full-motion loader.” The record already describes Results-specific compact full-motion loading, a Link clapping model-part program, and Luigi Win2 hand roots. Those are partial repairs, not universal animation proof.
- “Invalidate fighter topology caches on every status change.” The current `ftMainSetStatus` wrapper already invalidates the flat transform walk and renderer status caches. Diagnose any still-stale generation at its actual mutation point. [Current status wrapper][ftmain]
- “Fix the Saffron hazard timer.” The owner and the later trace agree that state and collision cycle. Fix what makes the visual gate fail to follow its animation.
- “Castle still lacks all wall collision passes.” Earlier passes, normal/segment handling, and local-space fixes are recorded. The remaining symptom is specifically the **knocked-back/falling** ramp case.
- “Remove BGM suspend calls and reduce the CSS dwell.” That would leave a long synchronous load competing with audio. Implement bounded loading first.

Preserve the accepted CSS underlay cache and post-Results heap reset, current entry alpha fixes, source-specific Fox laser strobe override, and existing native owner coverage. Do not reverse previous fixes merely to make a new test resemble an old report.

### Memory is a possible upstream cause, not an excuse to omit effects

The repair log records effect allocation being constrained by the source's low-memory object budget, including the `ifCommonSetMaxNumGObj` behavior around a 25,600-byte floor. It also records different free-space minima across rosters/stages. A renderer can report **zero declines while a required effect was never allocated**. Reduced-allocation safety guards are not full visual repairs. [Repair record][repairlog]

For each missing effect, distinguish: maker not called; descriptor unresolved; object allocation refused; particle script not admitted; object alive but not submitted; native triangles emitted but wrong pixels. Do not merge these into one “VFX missing” counter. If memory is the failing boundary, reclaim or correctly scope the owning allocation and prove the required content fits. Do not blindly raise capacities, preload every fighter, borrow four-player-active storage, or resurrect the abandoned reduced-fighter-pool experiment.

---

## 3. Implementation and evidence rules

### 3.1 Working tree, source ownership, and builds

Start by reading the agent instructions and checking the actual local revision and dirty overlay. Do not reset, stash, overwrite, or sweep other work. A coding agent may already have repaired a cited seam after this snapshot: inspect the relevant diff once, preserve valid completed work, and advance to the next missing proof. [AGENTS][agents] · [Verification][verifying]

Keep `decomp/` read-only. Use it as the behavioral and asset oracle. Correct import wrappers, compatibility code, generators, and native owners. Do not hand-edit generated tables or substitute another game's geometry/effects. Check preprocessed imports and linked symbols where macros or conditional compilation affect the actual path; a search hit or source declaration alone does not establish what the ROM executes.

Every new ROM, including diagnostics, must exclude non-native renderer implementations from the linked inputs. A disabled flag, an unexercised fallback, or a zero counter does not satisfy the owner's requirement. Use the current native-only linked-input gate.

Use PowerShell 7 and explicit Python. There is one build/producer writer across build directories and worktrees because generated paths are shared. Do not override `-j`, `-Jobs`, or `MAKEFLAGS`. Use existing qualified lab configurations for iteration; protect accepted root ROMs. Do not make this investigation a wholesale documentation rewrite or a new mandatory report per small edit.

### 3.2 The shortest useful diagnostic record

Use a small, bounded event record rather than always-on per-vertex logging. Identify each observation with the ROM/ELF/config, scene generation, source update number, presented-frame number, fighter instance/kind, and effect instance plus lifetime generation. For a relevant event record:

| Boundary | Minimum observation |
|---|---|
| Input/status | Natural action; selected status, motion, flags, callback address |
| Producer | Motion/effect ID or collision event; exact constructor arguments |
| Asset/lifetime | File ID, base/span, relocation state, owner generation; valid dependency closure |
| Object | Successful allocation, source visibility, DObj/MObj/AObj identity, birth/age/death |
| Transform | Source local position and parent; world transform; submitted modelview/projection |
| Native output | Selected owner/program/root; current material/image/palette; actual triangle delta |
| Final presentation | Captured frame at the same event/age; expected visible region and required sound |
| Failure | First current failure code or exception PC/LR; not a stale cumulative latch |

A debugger's memory read must be coherent with the running ARM9 state. Use the project's qualified observer/capture path; don't base a diagnosis on a possibly stale cache-backed memory snapshot. Reset or timestamp event-local failure latches. Preserve the first corrupting event instead of repeatedly running through a known fault.

For position bugs, declare coordinate space at each boundary. A world-space contact must not receive the fighter's world transform a second time. A local joint offset must receive the joint transform exactly once. Compose the source child hierarchy and camera in the same order as the current renderer's matrix convention; do not change matrix conventions to fit a screenshot.

For visual closure, positive allocation and triangle counts are necessary but not sufficient. Require actual pixels with the correct position, color, alpha, timing, and lifecycle. A source-hidden fighter may legitimately be replaced by a star or intro object; making that fighter visible is not an acceptable substitute.

---

## 4. Main menus and CSS

<a id="m01"></a>

### M01 — Make Characters and VS Record non-selectable

**Diagnosis: code-confirmed local menu-policy mismatch.** `src/nds/nds_menu_shell_data.c::ndsMenuShellUpdateData` still maps A/START on these rows to `nSCKindCharacters` and `nSCKindVSRecord` when their scene definitions exist. The owner has deferred those destinations, not the DATA plate or Sound Test. [Data menu][menu_data] · [Queue][bugs]

**Consolidated interaction choice:** retain the source rows and focus navigation, but make the two deferred destinations non-activatable. The reviews differed on cursor focus; the owner wording does not specify a new cursor/art policy. Blocking entry is the required behavior in this handoff. Do not add a navigation redesign or mandatory disabled-art bake to the bug's scope.

**Implementation instructions**

1. Add or reuse a local activation-eligibility predicate that rejects Characters and VS Record while retaining the existing Sound Test unlock/availability rule. Use it before any confirmation sound, BGM stop, asset acquisition, or scene request.
2. Recheck the selected row on every A/START activation. A stale selection index or restored `scene_prev` must not enter a deferred destination merely because `ndsSceneManagerFind` succeeds.
3. Preserve the displayed rows, current source focus movement, and existing DATA art. Use an existing disabled presentation only if it already fits the UI; new artwork is not necessary for this repair.
4. Keep Back/Cancel and return-to-DATA behavior intact whether Sound Test is available or not. If an already-approved local implementation skips disabled focus, preserve it only with bounded navigation and an all-disabled state; do not introduce an infinite search for an enabled row.
5. Do not globally unregister the scenes, change save unlock bits, or start implementing the deferred screens.

**Acceptance:** natural Title → Mode Select → DATA, both Sound Test availability states, each row, A and START separately, repeated/held input, stale restored focus, Back and re-entry. Characters/VS Record never cause a transition or audio interruption. Sound Test behaves as before. All-disabled destinations do not trap the user.

<a id="m02"></a>

### M02 — Damage repeat step 5, not 3

**Diagnosis: directly identified constant/branch.** In `nds_menu_shell_vsoptions.c`, the Damage row uses a larger magnitude for held repeat than for a fresh tap. The current held magnitude is three. The existing Damage domain is inclusive 50–200. [VS options][vsoptions]

**Implementation instructions**

1. Change the Damage-specific held-repeat magnitude from 3 to 5. Keep fresh taps at ±1 unless the owner later explicitly changes that requirement.
2. Preserve current input-repeat delay/cadence and the direction/sign convention. Do not multiply every VS option or accelerate the global menu repeat routine.
3. Preserve the row's existing **wrap**, not clamp, behavior. Review B's clamp wording conflicts with the current C and must not be implemented. Test signed wrap before storage into any unsigned field. With the existing 151-value domain, 198 + 5 wraps to 52, and 52 − 5 wraps to 198. The real signature is `ndsMenuShellVsOptionsAdjust(s32 direction)`; it takes one argument, not a row plus a delta.
4. Add or extend the source-executing host fixture to enumerate all 151 values for both directions and tap/held input. Require every result to stay in range and to match the current wrap law with the new magnitude. In the caller, choose magnitude 5 only for the Damage row on a held-repeat event; all other option semantics and one cue per accepted adjustment remain unchanged.

**Acceptance:** single tap changes by one; held repeats change by five; boundary wrapping and the other VS rows remain correct. Confirm that save/restore and match configuration consume the chosen value unchanged.

<a id="m03"></a>

### M03 — CSS music pauses or resets during preview creation

**Diagnosis: confirmed blocking load and intentional audio interruption.** The compact preview path in `ndsMNPlayersVSPreviewAcquireResidentKind` suspends BGM, switches allocation context, synchronously performs fighter-file setup/preparation, restores context, then resumes BGM. The queue records eight suspends per visit and a closure occupying about 8.6 frame budgets in one call. Those are prior observations, not measurements made in this review. The noncompact incremental loader is not evidence that the compact shipping closure is incremental. [CSS import][css_import] · [Queue][bugs]

**Required architectural repair: extend the existing preview transaction, not cosmetic audio changes or a second loader framework.** Existing resident blocks already carry `load_cursor`, `loading_fkind`, `load_root`, `load_tree_done`, `cancel_requested`, references and cancel/retire helpers. Reuse their ownership model. The compact FPC path needs resumable lower-level read/decode/fixup/preparation support; the raw-tree slice API is a pattern, not proof that compact loading is already bounded.

1. List all work currently inside the blocking compact closure: file/header reads, decompression or unpacking, relocation/external dependencies, owner image setup, texture preparation, and initial pose/DObj preparation. Measure those phases separately once; slicing only the initial read leaves the rest blocking.
2. Extend the existing pending/resident request state to identify fighter kind, requested costume/detail, slot request generation, scene generation, and resident block. A useful conceptual sequence is `REQUEST → READ → DECODE → RELOCATE → PREPARE → READY → COMMIT`. These are conceptual phase names, not new APIs or a requirement to duplicate state. If `ftManagerSetupFilesAllKind` cannot yield safely, add the resumable work at the compact-pack producer/loader it calls and keep publication at the existing owner boundary.
3. Give the CSS **one aggregate per-update service budget**, not an independent full budget for each of four slots. Bound both CPU work and I/O/decode span size. A time check after one huge blocking read is not a bound; make each work unit small enough to be preemptible at supported boundaries.
4. Preserve playback continuously. Service the existing audio stream independently while the preview transaction advances. Do not call Suspend/Resume as part of routine preview loading, restart the sequence, alter its playhead, or mask an underrun with silence. Audit any filesystem critical section that could still prevent refill service.
5. Restore global allocator and relocation context before every yield, failure, or cancellation. Private partially built resources must not become authoritative global fighter-file pointers visible to other slots. Do not make the existing global setup routine re-entrant merely by placing a loop around it.
6. Commit readiness atomically: valid asset closure, native owner, textures, first pose, and reference ownership must all be ready before the slot publishes the new preview. Keeping the preceding valid preview briefly is preferable to exposing half-relocated state.
7. Extend cancellation beyond the existing raw-tree `load_cursor`: compact decode state, temporary registrations, file handles, native-owner staging, and resident block ownership all need cleanup. A stale completion must never replace a more recent hover selection. Cancel a shared kind only when no other slot/pending request still needs it; changing one cursor is not permission to retire another slot's transaction.
8. Retain the four-block cache and reference sharing for duplicate fighter kinds. Do not retire a block still referenced by another slot or in-flight draw. Treat scene exit/Results return as generation invalidation, with deterministic cleanup rather than a global reset of unrelated assets.
9. Keep the accepted cached CSS underlay and graphics-heap reset fixes. They address different costs/lifetimes and must not be reverted to simplify this transaction.

**Acceptance:** record continuous audio output and stream/playhead state while rapidly sweeping cold and warm previews, reversing direction, changing costumes, sharing a fighter across slots, and leaving CSS mid-load. Require zero preview-induced BGM suspends/restarts and no audible gap. Record the maximum loader service interval, request latency, cancellations, stale-commit rejections, and heap high-water. Verify the normal shell path and a Results→CSS return, not only a direct-boot preview lab.

<a id="m04"></a>

### M04 — Hover-to-preview delay

**Diagnosis: same load bottleneck plus debounce policy.** The 13-tic dwell delays starting a closure that then blocks. The queue's recorded 70 tics of waiting per visit does not mean changing the dwell alone removes the load latency. [CSS import][css_import] · [Queue][bugs]

**Implementation instructions**

1. Complete M03's transaction and cancellation mechanism first. Then separate input stabilization delay from actual loading latency in the measurements.
2. Reuse a ready cached fighter immediately, subject only to the legitimate short input/selection rule. For a cold miss, enqueue the latest stable request without blocking the cursor.
3. Replace the long fixed dwell with the shortest measured stabilization interval that avoids wasteful churn. One or two source updates is a candidate to evaluate, not an asserted final requirement. A warmed cache must not pay the full cold-load debounce.
4. Coalesce superseded hover requests. Prefer the currently requested preview over completing a no-longer-visible queued selection. Preserve generation-safe cleanup; never publish an old request simply because it completed first.
5. Measure pointer/selection-change→first correct visible preview in **source updates and elapsed time**, with warm/cold P50/P95 and worst case. Keep cursor responsiveness, audio continuity, and aggregate loader budget as simultaneous requirements.

**Acceptance:** no stale fighter flashes after rapid hover changes; warm previews appear promptly; cold previews finish without freezing input/audio; cancellation and menu exit leak nothing. M03 and M04 may share one capture/run, but report both audio continuity and visual latency separately.

---

## 5. Link and Pikachu effects

<a id="l01"></a>

### L01 — Link slash damage VFX at incorrect locations

**Diagnosis: native output exists; exact positional divergence is not yet established.** The source-backed DamageSlash owner uses EFCommonEffects1 asset 83, roots `0x75A0` and `0x7668`, animated material/CI4 frames, and two bounded texture allocations. Its prior proof establishes emitted pixels and resource bounds, not that every slash appears at the correct hit position. The current executor explicitly consumes the supplied projection/modelview matrices and source-local vertices. [DamageSlash executor][slash_exec] · [Generator][slash_gen] · [Prior native proof][slash_proof]

**Implementation instructions**

1. Reproduce a natural Link sword hit and separately a sword swing that misses. First identify whether the offending object is `efManagerDamageSlashMakeEffect`, a sword trail, or another hit effect. The owner's phrase does not justify modifying every Link slash.
2. For the actual effect, capture the collision contact or motion-event position **before** the maker, the maker's rotation/size arguments, the effect root and drawable-child transforms, and the submitted matrices. Capture birth and subsequent visible ages. For DamageSlash, compare the caller's contact calculation and `gmCollisionGetDamageSlashRotation` with the source behavior.
3. If the maker receives a wrong position, fix the collision/contact or joint-world query. If it receives the correct world position but the object is wrong, fix constructor/update ownership. If the object's hierarchy is correct but the submitted matrix is not, fix the adapter's hierarchy/camera binding or stale transform key.
4. In particular, reject a double fighter transform on a world-space contact, omission of an animated effect child transform, use of a previous fighter's matrix, or replay keyed only by root asset instead of object identity/generation.
5. If all transforms match, project the source geometry through the same camera and compare the visible center/extent. Investigate depth/occlusion separately from XY position; changing Z cannot repair an incorrect world origin.
6. Preserve the source's motion and effect lifetime. A detached hit slash must not be glued to the sword every update. Do not add an arbitrary screen-space offset or replace the existing native owner.

**Acceptance:** both facing directions, translated positions far from world origin, airborne/grounded hits, different opponent sizes, hit/miss distinction, camera movement, and two simultaneous slash instances. Capture correct contact placement and age progression while preserving source rotation/size, current texture budget, and zero native rejection.

<a id="p01"></a>

### P01 — Terrain-following Thunder Jolt has hard edges

**Diagnosis: a specific path difference remains; an unconverted coverage defect is a candidate, not yet proved.** The air/effect quad owner uses the specialized CI4 quad helper. The grounded owner in `nds_native_pikachu_thunderground.exec.inc` instead binds through `ndsRendererHardwareBindTexture` and emits one triangle per source segment. Therefore the earlier quad coverage repair cannot be assumed to cover this row. This is a confirmed implementation difference; the exact missing coverage/filter contribution needs a source/native texture comparison. [Ground executor][jolt_ground] · [Ground generator][jolt_gen] · [Jolt effect executor][jolt_effect]

The source closure is concrete: PikachuSpecial1 asset 244 attributes at `0x34` refer to PikachuSpecial3 asset 342, DObjDesc `0x1888`; six drawable roots are `0x1490`, `0x1528`, `0x15C0`, `0x1660`, `0x16F8`, `0x1790`. Each uses a live current-image material. Root `0x15C0` has two source vertex loads; the generator already flattens its corners correctly. Do not replace these segments with a quad.

**Owning files:** start at `scripts/stages/generate_nds_native_pikachu_thunderground.py` and `src/nds/nds_native_pikachu_thunderground.exec.inc`. Keep the air/effect siblings `generate_nds_native_pikachu_thunderjolt.py` and `generate_nds_native_pikachu_thunderjolt_effect.py` in regression coverage; those two generators alone do not describe terrain-following ground geometry.

**Implementation instructions**

1. Inspect the six roots' source texture format, palette alpha, combiner, render mode, filter/edge behavior, UVs, tile dimensions, and all four live images. Determine whether the graded visible edge derives from stored alpha, intensity used as coverage, filtering, or a combination.
2. Compare the actual ground-owner texture upload format and effective coverage against that contract. Do not assume that CI4 means “one bit of transparency is sufficient” or that every black texel must be transparent.
3. Extend the existing semantic texture conversion/binding path, or add a narrow generated ground-jolt conversion, to retain the required coverage. A3I5/A5I3 are candidate DS encodings only after checking color count, coverage precision, palette organization, and existing budgets. Preserve the source RGB/coverage relationship; a single constant polygon alpha does not restore a spatial gradient.
4. Keep the live image selector and every segment's source transform/visibility. Include asset generation, image identity, palette/coverage class, dimensions, and relevant material state in any converted-texture cache key. Do not let a prior hard-alpha upload satisfy a soft-alpha request accidentally.
5. Preserve zero-coverage skip behavior and avoid accidentally emitting alpha-zero wireframe geometry. Test segment junctions and UV clamping so the conversion does not produce seams between otherwise correct pieces.

**Acceptance:** natural ground travel, turns over terrain edges/slopes, transition from air to ground, all six source roots and all source images over time, against dark and bright backgrounds. Require soft intended edges without changing trajectory, collision, lifetime, overall color, or silently dropping segments. Measure the bound on added texture RAM/VRAM and conversion cost.

<a id="p02"></a>

### P02 — Down-B blue self-hit explosion missing

**Diagnosis: the missing effect's source identity is now localized.** This is not simply the falling Thunder bolt. `ftPikachuSpecialLwCheckCollideThunder` transitions to the ground/air Hit status when the head reaches Pikachu. The source script `dPikachuMainMotion_GettingThundered_0x1668` emits `nEFKindThunderAmp`, alongside dust, quake, and a separate color animation. `ftParam` maps ThunderAmp to `efManagerThunderAmpMakeEffect`, whose source constructor starts particle script **`0x74`** in `gEFManagerParticleBankID`. The source enum explicitly identifies it as Pikachu's Thunder self-hit. [Thunder state machine][pika_lw] · [Exact motion script][pika_motion] · [Effect mapping][ftparam] · [Effect definitions][efdef] · [Source makers][efmanager]

**Implementation instructions**

1. Reproduce Down-B with unobstructed sky first. A stage platform intercepting the descending Thunder head is not a missing self-hit effect. Record that Pikachu enters `SpecialLwHit` or `SpecialAirLwHit` before evaluating the burst.
2. Trace this exact chain: Hit motion script → `nEFKindThunderAmp` → `ftParam` dispatch → `efManagerThunderAmpMakeEffect` → bank/script `0x74` → particle object(s)/children → native particle presentation. Count each link and log the current first refusal.
3. If the motion event is absent, repair compact motion/event resolution or the Hit-status mapping; do not invent a duplicate C-side timed burst. If dispatch is absent, repair the active import seam. If the particle script is missing, add its exact source-derived bank/script/dependent textures to the existing native particle producer and resolver.
4. If particles are made but rejected or invisible, correct this script's native template/material/coverage path. Use the actual source script and texture/color evolution, not a generic blue sprite and not the already-implemented ThunderShock/vertical bolt geometry.
5. Verify correct joint-0/world placement and source age. The zero-valued event offsets are intentional arguments, not permission to place the burst at world origin. Preserve the separate fighter color animation, dust, quake, damage hitbox, and audio.
6. Check budget refusal separately. A successful bolt owner and zero renderer failures do not establish that script `0x74` was allocated or drawn.
7. Preserve the later DLLINKS-admission/Ness-tail repair. Review B's role-4 rejection is historical, superseded by the same repository log's subsequent admission and role-mask-7 observation. On the current candidate, independently observe descending head, weapon trail, effect trail, ThunderAmp, fighter color animation and end state. If the callback rejection recurs, inspect configuration/regression first and repair admission **with** its exact native owner, including Ness PK Thunder/`0x8F98` sibling coverage. Do not treat a repaired trail as proof that the blue self-hit burst works. [Later trail evidence][trail_repair]

**Acceptance:** ground and air self-hit produce the blue source burst at Pikachu, in the correct sequence with bolt disappearance and fighter flash. Missed/intercepted Thunder must not produce a false self-burst. Test damage interruption, repeated moves, camera movement, and multiple Pikachus. Prove positive script-0x74 engagement and actual native pixels, not merely Thunder role-mask coverage.

<a id="p03"></a>

### P03 — Poké Ball spawn intro VFX absent

**Diagnosis: confirmed missing producer call.** The port's fighter-entry branch for Pikachu selects the entry status but omits the Poké Ball throw effect, with an old comment about unavailable item assets. The entry updater has a separate rays trigger, so ball and rays must be audited independently. Pikachu's source appearance script sets its opening flag and sound at update 40; that is useful timing evidence, not a reason to synthesize the effect in the renderer. [Entry import][entry] · [Appearance script][pika_motion] · [Source effect makers][efmanager]

**Implementation instructions**

1. Restore the source `efManagerMBallThrownMakeEffect` call through the port's entry seam, using source placement and saved entry facing. Entry setup can set `fp->lr` to zero while retaining the original direction in entry status variables; don't pass the temporarily cleared facing field.
2. Ensure the required ITCommonData effect closure is present and mapped before the call. Review B identifies current item-core loading, `battleship_item_mball.c`, and `dEFManagerMBallThrownEffectDesc` resolution as the newly available prerequisite. Validate it in the actual candidate: descriptor, DObj/MObj/animation pointers, mapped spans and generation. Do not merely remove the omission and rely on a raw deferred file head. Add the source-signature declaration `GObj *efManagerMBallThrownMakeEffect(Vec3f *pos, s32 lr)` through the appropriate port header.
   Supported configurations enabling Pikachu/Purin with `NDS_P2_ITEM_CORE=0` need an explicit minimal shared asset/constructor/native-owner dependency, or another project-approved equivalent build arrangement. Do not silently restore a NULL stub or force all unrelated item gameplay on to obtain one effect.
3. Check whether the existing shared entry/native effect path already owns the Poké Ball's exact roots. Reuse it when correct; add only missing source-backed roots and state, through generators rather than hand-authored substitute art.
4. Follow the independent opening-rays flag in the entry update. Require both ball motion/opening and the appropriate rays; neither proves the other. Preserve opening sound and body visibility timing.
5. Audit the Jigglypuff/Purin sibling branch because the same omitted Poké Ball policy/comment appears in that family. Treat it as sibling coverage of the repaired path, not an excuse to change unrelated entry sequences.
6. Distinguish initial match introduction from the normal post-KO rebirth platform. Do not spawn a Poké Ball on every respawn merely because both are called “spawn.”

**Acceptance:** natural first match and subsequent match entry, both facing/slot arrangements, Pikachu and the shared Poké Ball sibling, continuous audio, correct body replacement/reveal, no stale pointers after returning from Results, and no new demand loads during active combat.

---

## 6. Shared face/body material audit: P04, K02, J01

<a id="p04"></a>

### P04 — Pikachu face differs from body

**Diagnosis: likely shared rendering-state class, but not proven to be lighting.** The owner reports the mismatch; the reviewed code does not by itself establish whether the face or body is wrong. The production fighter executor has per-root material epochs, source diffuse/ambient light colors, hardware-normal handling, owner/costume/shade texture identity, and packet replay that can bypass repeated material preparation. Any comparison must cover these actual paths. [Production fighter executor][fighter_exec] · [Queue][bugs]

The hardware-light path documents the intended source relationship as ambient plus diffuse times the lit normal contribution, with a white hardware light color and a separately installed light vector. Writing white light color does **not** prove that the light direction, current normal transform, or ambient/diffuse material is correct.

**Shared implementation instructions**

1. Choose a stable neutral pose and identify the exact native roots/runs on either side of the visible seam. Decode each run's source combiner, geometry-lighting bit, vertex interpretation, texture/palette, primitive/environment colors, and material-light changes. Do not assume face and body use the same source program.
2. At the same source frame, record the resolved material/light state and uploaded texture/palette for both runs. Compare source expected output after the same final RGB5 quantization. A small expected quantization difference is not automatically a code defect; conversely, do not dismiss a persistent source-absent seam as unavoidable.
3. Check state inheritance at root and material-epoch boundaries. If the face inherits an earlier root's primitive/environment/light state or the body misses a required light preamble, correct the generated state dependency or root bind. Do not set all runs to one global color.
4. Check normal semantics. A lit run interprets source vertex bytes differently from an unlit color run. Confirm normal conversion, current root/camera transform, light-vector installation timing, and scaling treatment. A vector transformed under the wrong modelview can make adjacent geometry light differently even with correct RGB values.
5. Check textured-color multiplication. A face whose palette already contains a tint must not receive an unintended second tint or a source-absent vertex modulation. Conversely, a source SHADE term must not be omitted merely to match one front-facing screenshot.
6. Check cache identity and invalidation: fighter instance, owner image generation, costume, shade, current texture frame, material epoch, and any light state baked into recorded packets. Compare a fresh prepared draw with replay of the same state. An experiment bypassing replay is diagnostic only; the final fix must work with the production path enabled.
7. Change the first incorrect state representation. Put immutable source facts in the generator and live animated state in the runtime binding; do not move full source-list interpretation into the frame loop.

**Acceptance for P04:** a source-faithful Pikachu face/body boundary through turning, blinking/expression changes, source light changes, all relevant costume/shade combinations, multiple same-kind slots, and CSS→battle→Results transitions. Record the affected roots and corrected state, not simply “looks less yellow.”

<a id="k02"></a>

### K02 — Kirby face/body pink mismatch

Apply the same audit, but **do not accept P04 as proof for Kirby**. Kirby's textured face and body may have different source state and the owner's observation that the face pink looks more correct is not a color oracle. Include normal, inflated/flying, and copy-hat states where the same material infrastructure is used; preserve intentional texture and color animation changes. [Production fighter executor][fighter_exec] · [Current owner complaints][bugs]

**Implementation instructions:** identify Kirby's seam roots and source palettes; verify costume/shade and current face texture; compare fresh versus replayed draws and copied-owner root-table transitions. Review B specifically flags resident FACE-head/copied-head color-source and self-shading handling in `scripts/fighters/generate_nds_native_owners.py`; inspect those generated root-light preambles before assuming a generic light fix. Compare the live light1/light2 snapshots in `src/port/renderer_adapter_fighter.c` and packed-color conversion in `src/port/reloc_backend_assets.c` at the same boundary. Correct demonstrated inheritance, normal, palette, or replay-key defects. Do not recolor the body to a guessed pink or suppress light animation across the fighter.

**Acceptance for K02:** source-correct continuity in neutral and inflated poses, with both facings and relevant costumes, without breaking facial animation, copied hats, or intentional damage/color effects. Keep a separate captured result for Kirby.

<a id="j01"></a>

### J01 — Jigglypuff face/body mismatch

Apply the shared audit to Jigglypuff's **Purin** assets and runtime owner. Similar symptoms across three fighters are useful evidence for prioritizing a common seam, not proof that one patch covers every model. [Production fighter executor][fighter_exec] · [Queue][bugs]

**Implementation instructions:** compare Purin's face/material program and body light state at the same pose, inspect texture/palette and packet-cache identity, and fix only the demonstrated discrepancy. Preserve source facial changes rather than baking one neutral face into a corrected palette.

**Acceptance for J01:** independent before/after material evidence and native capture across turning, facial animation, costume/shade changes, duplicate fighter instances, and scene transitions. A successful Kirby screenshot is not this row's acceptance.

---

## 7. Kirby gameplay, effects, and crash

<a id="k01"></a>

### K01 — Grab rise/fall slam teleports the victim

**Diagnosis: confirmed import-time status mismatch, with a strong but not yet runtime-proven causal link.** The current catch translation unit defines:

```c
#define nFTKirbyStatusThrowF nFTCommonStatusThrowF
```

around the included common throw source. The original `ftCommonThrowSetStatus` explicitly chooses Kirby's **special** forward-throw status for Kirby. Macro substitution changes that branch to common ThrowF, which can install the wrong motion/callback family for the rising/falling slam. The special Kirby forward-throw implementation exists separately. Verify that the normal compiled call reaches this imported branch before attributing every teleport to it. [Catch import][catch_import] · [Source common throw][throw_source] · [Source Kirby forward throw][kirby_throw]

The current wording describes the grab slam's upward/downward motion. Do not reinterpret it as separate up-smash/down-smash bugs or implement modern-game up/down throws. Review B correctly notes that the earlier generic-grab/down-smash probe did not exercise the complete reported slam. Its negative observation is not evidence against this report; the alias and the victim transform are two sequential questions, not competing excuses to stop.

**Implementation instructions**

1. Inspect the preprocessed catch translation unit with the shipping flags and the linked `ftCommonThrowSetStatus` call path. Record source and port values for the true Kirby status and common ThrowF; prove what value actually reaches `ftMainSetStatus` during a natural forward throw.
2. Remove the semantic alias in the port seam. Supply the correct source enum/declaration through the proper ABI mirror/include arrangement. Do not invent a replacement numeric value, edit `decomp/`, or globally remap common ThrowF for other fighters.
3. Trace the paired state change: Kirby's selected special status/callbacks, victim's thrown status, catch/capture pointers, captured-fighter update scheduling, and the first joint-world attachment query. Confirm the intended Kirby rise/fall/landing procedures execute.
4. Capture victim position away from world origin. The source captured-fighter routine derives a position from the holder's item-heavy joint and the victim's child transform/scale. An attachment matrix that is identity, stale, or owned by the wrong fighter is distinct from a wrong status selection. [Source captured-fighter transform][capture_transform]
5. Retain the already-landed status-change cache invalidation in `ftMainSetStatus`. If the teleport persists after the status repair, find the first topology/root/pose mutation not reflected in the attachment matrix or native pose generation. Do not add a redundant global walk as the assumed fix. [Current wrapper][ftmain]
6. Respect the region actually compiled. The US captured-fighter path calls `func_ovl0_800C9A38`; the alternative branch uses the FTParts matrix path. An upstream stale-root note is a useful hypothesis, not proof that this DS build executes the branch described there. [Upstream note, historical][capture_note]
7. Verify release, interruption, victim destruction, and Kirby landing restore the source's capture, collision, visibility, and death-ignore flags exactly. Never hide a state bug by writing the victim to Kirby's position every frame or allowing the victim to escape early.

**Acceptance:** naturally grab and complete the forward rise/fall slam in both directions, at non-origin positions and with camera movement; cover the supported victim kinds/sizes, relevant ground/slope cases, interruption and KO cleanup. Verify damage/launch timing as well as position. Regress ordinary common throws, Donkey Kong carries, and Yoshi/Kirby capture paths that share the transform machinery.

<a id="k03"></a>

### K03 — Neutral-A punch flurry VFX at wrong positions

**Diagnosis: not a missing native owner.** The repair record already describes source-derived Kirby Vulcan Jab output from KirbySpecial2 asset 348, including mesh and glow coverage, alpha conversion, and a source-specific foreground treatment. That work established visibility, not correct anchoring. The source maker takes position, facing, rotation, velocity, and an additional motion parameter; its update/lifetime behavior must remain authoritative. [Source effect makers][efmanager] · [Repair record][repairlog] · [Kirby declarations/import seam][kirby_common]

**Implementation instructions**

1. Trace the natural rapid-jab producer in the active `ftcommonattack100.c` path into `efManagerKirbyVulcanJabMakeEffect`. Capture all five arguments and any joint-local-to-world conversion before entry. Do not treat a rapid-jab trail's origin as the same semantic thing as DamageSlash's collision-contact position. Work L01/K03 in one investigation only if their recorded divergences point to the same adapter/hierarchy boundary; matching symptom wording is insufficient.
2. Record the created object's local/world position and rotation, source velocity/update fields, and age from birth through its short source lifetime. Compare each age to the submitted native modelview. A correct birth position followed by wrong drift is an update/velocity bug, not an attachment bug.
3. Check both facing directions. Determine whether facing was applied in the joint transform, the maker, or both. Verify source angular units/conversion and that a local offset is not mirrored twice.
4. Check source-update versus presented-frame timing. A short effect can be born and advanced in the same presented frame; correlate by source update and age, not wall-clock screenshot delay. Do not extend its lifetime to make a probe easier.
5. Check the existing no-Z/foreground presentation adaptation independently. It must preserve XY, scale, camera motion, and the source hierarchy. A projection-layer adjustment that changes apparent XY or scale should be replaced with a correct scoped depth/order treatment, not compensated by a position constant.
6. If the source object is correct but native output is wrong, fix the effect instance/child matrix binding or packet generation. Preserve the current source-derived mesh/glow representation and color/alpha work rather than replacing it with a simplified streak.

**Acceptance:** continuous natural jab at translated positions, both facings, moving camera and multiple concurrent instances; compare birth and later ages. Require correct hand-relative emission and source drift, no detached origin-zero flashes, preserved visual shape/lifetime and no unbounded texture/object growth.

<a id="k04"></a>

### K04 — Inhale then spit-out star is invisible

**Diagnosis: confirmed effect suppression.** `battleship_kirby_common.h` macro-replaces the source call with:

```c
#define efManagerCaptureKirbyStarMakeEffect(fighter_gobj) ((GObj *)NULL)
```

The neighboring `#define efManagerLoseKirbyStarMakeEffect(fighter_gobj) ((void)0)` suppresses the lost-copy sibling. The captured-fighter import includes that header. The source capture-star state intentionally hides the ordinary fighter while expecting the star effect; the replacement prevents it from being made. The source's retry/attachment flag behavior explains why a valid gameplay object can remain visually absent. [Kirby common header][kirby_common] · [Capture import][capture_import] · [Source capture-star state][capture_source]

**Implementation instructions**

1. Restore the real source constructor path in the port. Do **not** just delete the macro and stop: complete the effect's actual descriptor, asset residency, relocation, update, and native presentation closure together.
2. Identify the source CaptureKirbyStar descriptor and its ITCommonData references in `efmanager.c`. Map the descriptor and all used DObj/MObj/animation pointers to current resident storage with valid bounds and generation. Distinguish a legitimately nullable field from an unresolved raw file offset.
3. Reuse any existing exact native owner for those source roots. Otherwise generate a narrow native owner preserving the source geometry/material animation. Do not substitute the Kirby fighter model, an arbitrary star sprite, or generic display-list fallback.
4. Preserve ownership by the captured fighter and source effect update. Set attachment success only when the constructor truly succeeds. Ensure interruption, breakout, damage, despawn, match exit, and repeated inhale/spit cannot leave a stale pointer or an orphan object.
5. Keep source hiding/intangibility/reveal transitions. “Force the victim visible” would conceal the missing star rather than repair it. Any low-memory refusal remains a failed required-effect acceptance condition, not a reason to claim the crash was avoided.
6. Repair **CaptureKirbyStar and LoseKirbyStar together as a bounded family**: remove each suppression only alongside its complete source constructor, mapped descriptor/asset, native root and lifetime closure. Prove spit-star and lose-copy-star separately. Review B's item-core prerequisite is useful, but does not establish every supported Kirby configuration; make the minimal required residency/build dependency explicit rather than leaving a conditional NULL stub. Do not re-enable unrelated suppressed effects.
7. Do not confuse either effect with Kirby's entry warp star. The entry star has separate source roots and recorded native work; its successful intro capture does not cover spit or copy loss.

**Acceptance:** naturally inhale and spit out different victims, ground/air where source permits, both directions, repeated captures, collisions/breakout, and scene transitions. Require visible source star while the ordinary body is intentionally hidden, correct travel/rotation/lifetime, body restoration, positive native draws, and leak-free cleanup. Separately trigger the source lose-copy event and verify its own star, state transition and retirement; do not count a spit capture as lose-copy proof.

<a id="k05"></a>

### K05 — Fox hat visible, firing the pistol crashes

**Diagnosis: the dangerous boundary is identified; the actual exception is still required.** Kirby's copied-Fox status uses a joint-derived shot position and calls the shared Fox blaster maker. The descriptor points through `gFTDataFoxSpecial1`. In the reviewed `wpManagerMakeWeapon`, weapon attributes are computed and dereferenced before a general validation of the pointed-to file/attribute span. A null, stale, unmapped, or wrong-generation copied-ability dependency can therefore fault. However, a newly enabled gun/model-part native root on the firing frame is a second plausible boundary. A visible hat proves neither weapon dependency readiness nor gun-root admission. [Kirby copied Fox source][kirby_fox] · [Shared Fox source][fox_source] · [Fox import wrapper][fox_import] · [Weapon manager][wpmanager]

**Historical pre-shot gate — retained, not assumed current.** Review B preserves a recorded diagnostic copy sequence (`copy_id 8 → 1`) followed by `ndsPreviewPackLoadHalt(20, 8)`. The repair log later pinned a stage-2 “display list outside its loaded file” eligibility failure but did not establish which clause. It also records stale/sticky witness and ARM9 cache-observation problems. This is evidence for a particular older diagnostic path, not proof that a visible hat on the owner's current ROM is still broken. [Repair record][repairlog]

If the exact candidate cannot reach a drawable copied state, give the stage-2 exits distinct clause IDs and capture them **at the failing draw**, with current owner/root/span/generation. Use a surviving in-ROM observer and a coherent read taken after the intended flush; `__attribute__((used))` alone must not be assumed to defeat linker section collection. Fix the demonstrated identity/residency failure without bypassing eligibility. Once that gate passes—or if it already passes—continue directly to the firing crash. A pre-shot repair does not close K05.

**Do this before another long crash-prone run**

1. Reproduce once with exception capture. Record PC/LR, faulting address/instruction, stack, source status/motion, firing flag, joint pointer, current copy kind, asset generations, and the earliest native decline in the same event. Stop unsafe execution on corruption.
2. Bracket the chain: copied-Fox firing flag → joint-world position query → `wpFoxBlasterMakeWeapon` → attribute normalization → DObj/material/animation setup → native gun/weapon submission. The first invalid access or rejected live root chooses the repair below.

**Repair branch A: weapon/dependency/attribute failure**

- Validate descriptor pointers, the current base and its accessible span, attribute offset/size/alignment, normalized `WPAttributes`, and every subsequently dereferenced render/animation pointer before use. Use the existing asset provenance/mapping facilities rather than raw pointer-range guesses.
- Make the minimal copied-Fox weapon closure a dependency of the active copy ability with a lifetime independent of the donor fighter's ordinary match residency. Do not assume the donor's file pointer stays valid after a copy change, KO, scene transition, or resident block reuse.
- Establish readiness at the supported ability-acquisition/residency boundary; do not add an unbudgeted synchronous asset load to the shot frame. Do not preload every donor fighter to solve one copied weapon.
- Inspect allocation order. The current manager can already have allocated WP/GObj state before the bad attribute dereference. Any new validation failure path must unwind those allocations or validate earlier. An early return that leaks or silently removes the projectile is only containment, not the final feature fix.

**Repair branch B: firing-frame model/native-program failure**

- Capture the exact live model-part/root vector when the pistol appears, including external asset bindings. Extend the copied-Fox/Kirby generated owner only for the proven missing source roots/material/transform state.
- Include the correct owner image and generation in program selection/cache identity. Do not substitute an unrelated hat table merely because it makes preflight succeed.
- Keep weapon physics separate: a native pistol-root rejection is not evidence that projectile attributes need rewriting.

**Repair branch C: joint or argument failure**

- Verify the copied-Fox source joint index and local offset against Kirby's current topology. Source Kirby and Fox use different local shot offsets; don't use Fox's unadjusted muzzle observation as Kirby's expected coordinate.
- Repair current-pose/topology resolution or the precise import mismatch. Preserve the approved shared blaster wrapper's behavior unless the witness identifies it as wrong.

**Acceptance:** natural copy acquisition followed by repeated ground and air shots, both facings, donor KO, copy loss/reacquisition, multiple Kirby instances, weapon lifetime and reflection/hit behavior, match exit/re-entry, and ordinary Fox regression. Require the actual projectile and pistol pixels, correct damage/lifetime/audio, no exception/decline, valid lifetime dependencies and no leaks. Merely refusing to fire or hiding the gun is not a fix.

<a id="k06"></a>

### K06 — Kirby has an incorrect Results pose

This is a specific acceptance case within R02, not a separate replacement animation system. The source Results selector chooses **only Win1 or Win2 for Kirby**, while the normal loss path chooses DemoLose. Testing a random Kirby win once does not cover both possible winners' poses. [Results source][results_source] · [Demo fighter update][scsubsys]

**Implementation instructions:** first record selected status, motion/script, figatree owner/generation, animation frame/speed and all live model-part roots. Check that a correct demo motion is not being paired with a stale battle, inflated, stone, copied-hat, or another demo program. Extend the exact missing root/state closure or fix wrong demo animation binding; do not map both wins to the one pose that happens to render.

**Acceptance:** source-reachable Win1, Win2, ordinary Lose, and No Contest clapping, through animation start and looping/end behavior. Preserve Kirby's source scaling/orientation and distinguish the Results root placement from limb pose. Record Kirby separately even if a shared R02 repair fixes it.

---

## 8. Results screen

<a id="r01"></a>

### R01 — First-place emblem is not displayed

**Diagnosis: the reviews target two distinct source objects; identify the missing visual before choosing a repair.** The owner says “1st place emblem.” Review A traces the first-place badge sprite; Review B traces the larger winner-series emblem. Both objects exist in the Results source, but the reviews do not supply a matched screenshot or object-local failure proving which one this report names. They must not be silently merged into one asset or treated as substitute implementations. [Results source][results_source]

| Branch | Source producer / asset | Representation and distinguishing behavior |
|---|---|---|
| **R01-A: first-place badge** | `mnVSResultsMakePlaceNumber` → `llMNVSResultsWinnerSprite`, Results asset offset `0xE2A0` | An SObj used instead of a numerical place digit for displayed place 1, under source team/shared-winner rules. Follow sprite provenance, format/strip decode, tile/OAM and commit. |
| **R01-B: winner-series emblem** | `mnVSResultsMakeEmblem` → selected `FTEmblemModels` DObjDesc/MObjSub/MatAnimJoint | A separate DObj tree on Results DL link 33, selected by the winning fighter's series and colored by source player/team rules. Follow its DObj/typed material/native owner and camera, not the sprite OAM path. |

**First observation:** on one natural Results screen, identify the expected missing region in a source-matched capture; record both source objects' creation and actual native submission. If both fail, retain two sub-results under this one row. Review B's “no dedicated native owner found” is a search-level hypothesis, not proof of absence from the active generated/common native path or linked binary. Inspect actual routing before adding an owner.

#### R01-A — WinnerSprite badge path

Internal `sMNVSResultsPlaces` uses **0 for the winner**; the display helper receives **1** for first place. Mixing them is a hazard, not an established root cause. Keep the distinction explicit. [Relocation symbols][reloc_symbols]

1. Record internal place, display-place argument, team/shared-winner condition, returned SObj, sprite identity, bitmap format/size/strips, LUT and source flags. If creation fails, fix the outcome/argument/caller boundary rather than changing every zero-based comparison.
2. Follow that exact SObj through native provenance resolution, cell lookup/bake, palette setup, tile planning, emit and OAM commit. The existing implementation records unsupported format, bad provenance, tile/cell/palette/OAM capacity and VRAM failures; capture the first current one. [Results OAM][results_oam]
3. For bad provenance, correct loaded-asset/generation registration or mapping, without weakening bounds. For unsupported format/strip handling, implement the exact source layout in the existing native path/producer, respecting `nbitmaps`, `bmheight`, `actualHeight`, `width_img`, LUT and byte-lane/texture-shuffle conventions.
4. For shape/capacity failure, extend the bounded legal tile plan or reclaim duplicate cached cells with full four-slot OAM/VRAM/palette accounting. Do not hide other results, shrink the art or add a non-native fallback.
5. If emitted cells still produce no pixels, inspect priority, coordinates, clipping, affine flags, palette transparency, tint-plane interaction and commit ordering. A successful bake is not presentation proof.

#### R01-B — FTEmblemModels series-object path

The source builds a winner-selected DObj tree, adds MObjs, samples its material animation using the source player/team color index, and registers `mnVSResultsEmblemProcUpdate`. The source root starts at `(0, 100, -11000)` with X/Y scale 25; its later movement/scale progression belongs to that updater. These are source-contract anchors, not replacement renderer constants. [Source constructor and update][results_emblem]

1. Record the source selected DObjDesc, MObjSub, MatAnimJoint and loaded asset view; created GObj/DObj tree; current transforms; material/color and animation state; Results camera and link 33 routing.
2. Trace the actual compiled native dispatch, including generated/shared owner paths. If the source object exists but admission/program selection fails, capture the precise root and reason. Only then add/extend the missing source-derived native capability.
3. Generate the distinct series roots once from source tables. Mario/Luigi and Pikachu/Purin share their source series entries; do not create twelve independent hand-authored emblems. Preserve exact geometry, ordered bindings, materials and source color selection.
4. Keep the live root update, typed material state and camera/depth ordering. Do not turn the object into the WinnerSprite badge, a fixed overlay or a static screenshot.
5. Validate preparation and teardown across Results re-entry, and any same-owner consumers affected by changing the shared FTEmblemModels representation. Search for those consumers; do not infer their coverage from asset residency alone.

**Acceptance for R01:** identify which visual(s) were missing and provide object-specific before/after evidence. For R01-A, cover clear-winner 2/3/4-player and relevant team/shared-winner conditions, with source-correct behavior for ties and No Contest. For R01-B, cover distinct source series and player/team colors, the source root movement/scale sequence and layer order. Preserve the source decision about whether each object is created for each outcome; do not invent a winner for No Contest. Both branches need native output, resource/lifetime safety and clean re-entry. A passing OAM test is not a DObj-emblem proof, and vice versa.

<a id="r02"></a>

### R02 — Fighters use incorrect Results poses/animations

**Diagnosis: incomplete demo-state closure remains the main repair area, not absence of the Results scene.** The original scene selects Win1/Win2/Win3 for most winners, only Win1/Win2 for Kirby, and DemoLose for losing participants. Its demo update executes animation events and color animation. Correct status selection alone is not enough if the port loads the wrong motion or selects a native model program that lacks that demo's model-part changes. [Results source][results_source] · [Demo fighter subsystem][scsubsys]

The repair record already contains Results full-motion asset work, Link clapping root-order/program work, and Luigi Win2 hand-root work. It also identifies a coverage gap: the model-part mutation census had focused on MainMotion scripts while demo scripts in `scsubsysdata*.c` can mutate parts. Therefore a green MainMotion-only census is not full Results coverage. [Repair record][repairlog]

**Owning seam:** `src/import/battleship_mnvsresults.c` imports the source scene; `scSubsysFighterSetStatus`/`ftMainSetStatus` and the demo data determine effective status/motion; `scripts/fighters/generate_nds_native_owners.py` and active adapter/program tables own the native root closure. Importing the correct source decision tree does not prove compiled macros, transfer data, enums, dispatch or animation binding are correct. Verify those before concluding every failure lies in geometry.

**Implementation instructions**

1. Build one explicit Results state matrix for the supported roster: every source-reachable win variant, ordinary Lose, and No Contest Lose. Include the special Kirby restriction. Distinguish fighter presence/kind/costume/shade from rank and outcome. Start with the existing `gNdsVSResultsFighterPlace`, `gNdsVSResultsFighterStatus`, `gNdsVSResultsFighterMotion` and Results-kind observations where present; extend only missing event-local dimensions.
2. For a failing matrix cell, record: selected demo status; motion/script ID and resolved bytes; current figatree/animation asset owner and generation; frame/speed; event cursor; relevant model/texture-part IDs; native owner/program; ordered root vector and per-root bindings. Sample at start and after a model-part or animation-loop transition.
3. Extend the existing mutation/geometry closure tooling, including the `check_model_part_mutation_coverage.py` census identified in the repair record, to **include the demo scripts** used by Results, not just fighter MainMotion. Follow script Goto/loop behavior and external references. Add fixtures that fail when a required demo model-part root is intentionally removed, not an inventory check that merely counts files. Preserve Link program 4 “Claps” and the source-derived Luigi Win2 variants; derive further variants from `scsubsysdata<kind>.c`, not captured root counts alone.
4. Generate exact reachable model programs, preserving each live root's source joint, root order, material state, external asset binding, and transform slot. The repaired Link clapping program demonstrates why equal root counts are insufficient: the order/binding can still be wrong.
5. Inspect the active enum and demo-status mapping in the actual build. If a correct status maps to the wrong motion/script, repair that mapping before touching geometry. Do not alias a failing Win2/Win3/Lose to Wait or Win1.
6. If scripts and programs are correct but animation is static, inspect the registered demo update, AObj/figatree progression, end/loop behavior, and presentation rate. Do not call animation twice per source update or reset its frame every draw.
7. If the pose is correct in CPU state but wrong natively, correct dynamic topology/pose cache invalidation and per-program root/transform binding. Retain existing status-change invalidation; locate the missing generation update at the actual mutation point.
8. Keep Results asset and owner lifetimes separate from stale CSS compact-preview or prior battle state. Re-entry must resolve the current fighter and costume, not recycle another participant's cached palette/program.
9. Preserve scene placement, winner-facing behavior, source character scale, outcome text, BGM transition, confetti and input exit behavior. Fixing limbs by altering the global Results camera or scaling every fighter is not a valid correction.

**Coverage and acceptance:** exercise the matrix's source-reachable states, animation starts and sustained/looping poses, 2/3/4 participants, duplicate kinds, relevant costumes, ordinary Time/Stock outcomes, teams/shared winners, Sudden Death outcome and No Contest. Use reproducible initial configurations or seeds to obtain variants; forcing internal status values can diagnose a program but is not the final natural-transition proof. Record which matrix cells are actually covered. A one-minute stress match that never enters Results covers none of this lifecycle.

<a id="r03"></a>

### R03 — No Contest should make every fighter clap

**Diagnosis: the source rule already exists.** `mnVSResultsSetFighterStatus` checks `nMNVSResultsKindNoContest` first and assigns `mnVSResultsGetStatusLose(...)`, which returns DemoLose, to each present fighter. The current symptom therefore requires distinguishing an incorrect No Contest transfer/classification from a failure to load, animate, or render DemoLose. Do not add a second general “everyone loses” rule while leaving the actual failure unknown. [Results source][results_source]

**Implementation instructions**

1. Use the normal user input path that aborts a match into No Contest. Record the outgoing battle/transfer state and incoming Results kind. Compare the compiled enum values and the code that chooses the result kind; don't infer No Contest from text alone.
2. If the kind is wrong, repair the handoff/initialization at that boundary. Preserve normal winner, tie and Sudden Death classification. Ensure no default or rematch seed overwrites the current outcome before Results consumes it.
3. If the kind is correct, verify every present slot receives DemoLose and the correct source loss motion. Inspect the R02 script/program chain for any fighter that is still frozen, posing as a winner, or drawing the wrong model parts.
4. Keep absent slots absent. Do not manufacture fighters or overwrite ranking/statistical data simply to force the animation.
5. Treat clapping as a sustained animation, not a single acceptable frame. Check event progression, looping, and source per-character behavior through the permitted exit window.

**Acceptance:** natural No Contest with 2/3/4 present participants, including Kirby and the already-repaired Link loss program; all present fighters perform their source losing/clapping animation, no one performs a win animation, and re-entry into a normal completed match restores normal winner/loser behavior.

---

## 9. Stages

<a id="s01"></a>

### S01 — Castle foreground roof texture missing on restored geometry

**Owner deferred. Do not implement without the owner lifting that deferral.** This guide covers the diagnosis and future repair procedure so the row is not omitted. The owner explicitly says the geometry now renders; the remaining dimension is texture continuity. Do not repeat the old missing-geometry repair. [Queue][bugs]

**Diagnosis: newly admitted roof runs need their complete source material/texture contract checked.** A run can emit the correct triangles while retaining a wrong image, missing TLUT, stale segment material, or UV/tile state from another group. The reviewed evidence does not identify which one is currently wrong.

Review B cites historical geometry closure for roof root `0x2240` (56 triangles / 16 runs); treat those as review-carried identifiers for locating the prior proof, not fresh measurements or a guarantee of current packet indices. The named producer boundaries are `scripts/stages/generate_nds_native_stage.py` and `scripts/stages/native_stage_descriptors/castle.py`. Preserve the completed geometry while investigating `use_texture`/NO_TEXEL0, semantic texture key, resolved name/palette and UV state.

**Future implementation instructions**

1. Capture the exact foreground roof runs that were restored, their source root/asset IDs, live material, texture image/TLUT, tile and texture scale, source UVs, and native bind result. Compare one correctly textured adjacent roof run with the failing run.
2. If the generator restored triangles without required inherited setup or external material binding, repair its emitted program and dependency closure. If live MObj image/tiling state is omitted, expose it through the existing typed material contract rather than interpreting a source display list at runtime.
3. If the native bind resolves the wrong resident asset or stale palette, fix provenance and cache identity. Preserve the successful geometry and source roof surface ordering.
4. Match repeat/clamp, UV origin/scale and source tile boundaries so adjacent faces form the intended continuous tiled roof. Do not stretch one texture over the entire restored roof as a substitute.

**Deferred acceptance:** source-faithful roof texture across all restored foreground surfaces, camera positions and relevant material frames; no regression to the now-complete geometry; no added fallback or resource growth beyond the measured native budget.

<a id="s02"></a>

### S02 — Blue upper side ramps fail during knockback/falling

**Diagnosis: unresolved state-specific collision boundary after earlier broad fixes.** The owner still reports this precise failure. Prior repairs to missing wall passes, tilted normals, segment handling, local-space sweeps and broadphase bounds are recorded. They do not prove that the knockback/fall callback and swept contact for the blue upper ramp are correct. A walking test on that ramp is not a reproduction. [Queue][bugs] · [Repair record][repairlog] · [Source collision control flow][mpcommon]

**Why this is not “already fixed.”** Review B correctly preserves the earlier wall-pass/translated-sweep work, but its suggestion to call the owner row stale is not supported. The actual receipt's after-proof describes the sliding board holding a fighter against the tower and then letting it drop, plus extent/walk checks. It does not establish acceptance of both blue upper ramps in the newly refined knockback/fall case. The current owner queue explicitly retains that case as “Still not fixed.” [Castle repair receipt][castle_repair] · [Queue][bugs]

Reuse the existing code and applicable host fixtures. First compare the exact candidate and natural scenario, not generic walking. A pass on one setup establishes only that setup; retain the row and any missing coverage/owner acceptance instead of deleting it or re-labeling the report unreal.

**Implementation instructions**

1. Reproduce a natural hit that knocks the fighter into the offending upper side ramp while airborne/falling. Preserve input/seed and the pre-contact source updates. Identify the exact source map line and segment; do not infer collision type or one-way behavior from its visible blue color.
2. At the first incorrect source update, record status, kinetic state, actual `proc_map`, collision flags/masks, current and previous fighter position, collision-diamond dimensions, velocity/substeps, selected floor/wall line IDs, and any stage/yakumono transforms.
3. Compare callback dispatch to the source. If the knockback status bypasses the required all-collision/map routine, repair the port's status callback/import mapping or the missing call at that boundary. Do not globally route every airborne state through a different collision policy.
4. If the callback is correct, follow candidate discovery and contact resolution for the identified segment: broadphase inclusion; line orientation/type/material and active flags; side/backface tests; swept intersection; endpoint inclusion; normal selection; final push-out and state transition.
5. Keep previous/current positions and line endpoints in the **same declared coordinate space**. For moving geometry, apply the source current/previous transform and relative motion exactly once. Preserve the earlier local-space and rational-bound repairs unless a failing fixture disproves them.
6. Compare per-segment slope/normal against any cached line-level value. A polyline's neighboring segments need not share the correct collision normal. Check wall/floor endpoint handoff and resolution order when a falling collision diamond crosses their junction in one update.
7. Turn the captured failing case into a bounded host fixture using the source algorithm/data as oracle. Include both sides, approaching from above/below, small and large displacements, exact endpoints, adjacent segments, and relevant moving-stage deltas. Extend/reuse `check_mp_line_extent_reject_exact.py` and the applicable existing fixtures instead of discarding their proven range/segment behavior. Keep arithmetic/rounding consistent with the port's supported fixed/rational representation; don't add float-heavy fallback collision to pass one case.
8. Patch the earliest incorrect mapping/test/resolution step. Do not make the ramp infinitely solid, inflate all bounds, snap airborne fighters onto the floor, disable knockback, or change one-way source behavior.

**Acceptance:** natural knocked-back/falling cases on both upper sides, representative small/large collision diamonds, fast and slow sweeps, normal landing/sliding/edge behavior, relevant stage motion, and regression on other stages' slopes/walls/one-way platforms. Prove that current wall/sweep optimization fixtures still pass and that the normal game—not only a direct collision harness—uses the repaired path.

<a id="s03"></a>

### S03 — Zebes hard acid edges and flat ground-light trapezoids

**Owner deferred. Both subparts remain deferred.** The queue says acid **color is accurate now**. Preserve it. Acid coverage and the ground-light gradients are two distinct visual contracts even though they share one row. [Queue][bugs] · [Stage notes][bugnotes]

**Review-carried constraints, not new runtime findings:** Review B reports an existing acid subdivision and a rejected further midpoint subdivision because the current `s16` vertex/UV representation could not encode many exact half-integer midpoints. It also reports prior observations of fully opaque polygon alpha on the light-shaft quads. Locate and verify the corresponding source/receipt before using either observation as a design premise. Neither establishes that the required visible gradient is absent from the complete source pipeline.

#### S03a — Acid blend/edge softness

Identify the acid roots and the precise origin of smooth coverage: stored alpha/intensity, combiner, material animation, filtering, vertex interpolation and overlap/render mode. Compare native data and alpha histograms against those inputs in a matched source frame. Keep the source plane, existing qualified geometry and height animation; do not add waves, thickness or another subdivision to compensate for a texture/coverage error.

If the missing quantity is texture coverage, fix the owning conversion/cache semantic class with an appropriate graded-alpha native encoding and source-matched RGB/alpha relationship. A lower constant polygon alpha does not restore spatial coverage. Preserve the corrected acid RGB, UV behavior and depth/material timing. If exact geometry interpolation is actually required, first demonstrate representability and budget; do not round new half-integer vertices blindly in the existing packed format.

**Deferred acceptance:** source-correct soft edges over dark/bright backgrounds through acid height/animation changes, unchanged color and gameplay collision/damage, no regression to other transparent stage content, and measured storage/submission cost.

#### S03b — Ground-light fade toward transparency

Establish the visual reference first. The owner requests a tapered fade; Review B raises the possibility that the source's hard silhouette was already correct. Do not decide that question from a single full-alpha polygon field: texture/intensity coverage, interpolated source vertex alpha, filtering and blend composition may still contribute a gradient.

Compare the source light's actual texture/material/vertices/render mode to a matched reference and the native output. If source coverage is graded, repair the first conversion/interpolation/binding stage that flattens it, using a bounded source-derived coverage texture or another qualified native representation. A constant alpha reduction is not a spatial fade. Preserve corner geometry, tint, position, taper direction and intended transparent endpoint; do not invent a hand-painted glow or blur all stage transparency.

If a complete source comparison instead shows the observed hard edge is source-faithful, report that precise finding and the remaining owner-requested visual difference for acceptance. Do not silently declare the report stale or invent source support for a new artistic effect. Implementation remains deferred in either case until reactivated.

**Deferred acceptance:** reference-supported coverage and taper at the intended endpoints, no opaque fringe, stable placement during camera movement, and no acid-color change from shared conversion work.

<a id="s04"></a>

### S04 — Saffron Pokémon garage looks permanently open

**Diagnosis: visual animation propagation is the established area; exact first bad boundary still needs a full-tree witness.** The later record follows 1,900 frames and sees hazard state and collision position cycle while the visible opening does not change. The limited root/first-child observation does not prove every descendant is static. Do not reset the timer or rewrite the Pokémon spawner. [Queue][bugs] · [Latest repair record][repairlog]

The source has distinct owners: a ground-state process and a gate GObj with its own `gcPlayAnimAll` process. Gate animation is attached through open/close animation arrays, while collision position is separately updated. The port wrapper exposes `ndsGRYamabukiGateGObj`; its older comments about absent monsters are stale context, not current proof of missing item logic. [Source Yamabuki gate][yamabuki_source] · [Gate import/accessor][yamabuki_import]

**Implementation instructions**

1. Observe a natural close→open→close cycle. Record gate state, gate GObj identity, source animation request, full descendant list, and every affected node's local translation/rotation/scale and AObj state. Capture immediately before attachment, after the first source animation update, and during the transition.
2. Verify the gate's registered animation process actually runs and that a repeated request is not resetting its frame each update. Verify ordering relative to state changes without adding a second animation advance.
3. Validate `map_head` and the open/close animation pointers against the current mapped asset. The reviewed wrapper's source offsets are MapHead `0x8A0`, open animation `0x9B0`, close animation `0xA20`. Resolve the actual source asset/generation; don't add offsets to a wrong compact base or assume a raw pointer table is already relocated.
4. If AObjs are missing or never change local transforms, fix attachment, pointer normalization, channel decoding or evaluation. Preserve the source channels/timing. A correct state enum and collision X do not compensate for an unread animation stream.
5. If local transforms change correctly, trace the animated gate instance through the movement accessor/route, native stage program, ordered map bindings, world matrices and packet invalidation. Review B names `scripts/stages/native_stage_descriptors/yamabuki.py` and `scripts/stages/generate_nds_native_stage.py` as producer entry points. The repair record identifies segment 3 and bindings 17–20 as the observed gate area; validate current producer output instead of treating those historical indices as a permanent ABI.
6. Correct a static-world-matrix substitution, wrong GObj identity, missing animated binding, omitted ancestor transform, or packet keyed only by asset/root. A packet containing transform commands must be invalidated/refilled when those commands' source pose changes; an immutable geometry packet must receive the current transforms at replay.
7. If the submitted gate vertices/matrices actually move but the pixels do not, inspect admission/visibility, depth and occlusion: the moving panel may be behind an always-open static duplicate or the wrong source surface. Only take this branch after proving the complete transform path.
8. Keep collision movement at the source's independently owned position. Do not move the visual gate using the collision X range as a substitute for its actual open/close animation.
9. Only if the existing static-stage representation cannot carry the proven live binding, split **the gate alone** into the existing dynamic native-actor path, preserving source ownership/timing and removing any duplicate static submission. Prefer extending the existing dynamic/yakumono mechanism over a new renderer state machine. Check closed-panel depth for diagonal coplanar flicker/z-fighting, as raised in the earlier capture interpretation; that is separate from whether the panel animates.

**Acceptance:** natural closed, transition and open poses are visibly distinct at the correct times; Pokémon entry/exit and collision cycle remain synchronized with source behavior; complete at least two natural cycles and a stage exit/re-entry. Record actual animated descendant changes, native binding/triangle output, and matched screenshots. A repeated “state changed” assertion is not this row's closure.

---

## 10. Execution order and bounded batches

This is a dependency/priority recommendation, not a second live queue or a requirement for one full verifier per row. Preserve the current execution cursor and valid unfinished work. A cheap state/provenance check is not permission to spend another turn merely re-reading the same reports.

| Order / batch | Scope | First decisive check | Required sibling/integration coverage |
|---|---|---|---|
| **A — Crash and gameplay** | K05 copied-shot fault; K01 special-throw alias; exact S02 knockback case | Current exception/eligibility clause, compiled throw value, or first missed ramp contact | Fox/copy lifecycle, paired capture/release, earlier Castle wall/sweep fixes |
| **B — Small local menu policy** | M01/M02 | Deferred scene activation and Damage ±5 wrapping | Sound Test, Back/re-entry, single taps and all 151 Damage values; no global repeat change |
| **C — Shared effect prerequisites** | P03 plus K04 CaptureStar/LoseStar; trace P02 ThunderAmp separately | Constructor suppression, dependency mapping, or first absent script-0x74 step | Pikachu/Purin intros, captured/lose-copy-star lifetimes, current Thunder/Ness trail coverage |
| **D — CSS transaction** | M03/M04 together | Maximum compact-loader work unit and audio gap | Cancellation, shared-kind references, stale publication, warm/cold latency, Results→CSS |
| **E — Results presentation** | R01-A/R01-B as actually affected; R02/R03/K06 | Exact missing object, wrong outcome/status/motion, or native root mismatch | All source-reachable demo variants, No Contest, retained Link/Luigi repairs, re-entry |
| **F — Effect placement/coverage** | L01/K03/P01 | First wrong coordinate/age or ground-jolt coverage value | Source caller distinctions, shared matrix siblings, all ground roots/images, lifetime/depth |
| **G — Fighter material** | P04/K02/J01 | First source-to-native material/light/palette mismatch | Independent three-fighter evidence, face-head handling, costume/replay/scene generations |
| **H — Gate animation** | S04 | Full-tree source animation → actual native binding/pixels | Hazard/collision timing, multiple cycles/re-entry, no duplicate gate or z-fighting |
| **Deferred** | S01 and S03a/S03b | Work only after explicit reactivation | Completed Castle geometry, corrected Zebes color, source-reference and native-only constraints |

The isolated menu patch need not wait for a long unrelated crash investigation if it can be completed without touching frozen inputs. Likewise, an exact S02 witness should be collected early, but one passing platform/walking test must not be used to close the refined ramp report. Combine L01/K03 only after witnesses demonstrate a shared seam; keep material and Results changes separately attributable even when they share an integrated candidate.

If a required effect fails allocation, reclaim the actual owning resource before claiming its presentation repaired. Do not remove content, lower roster scope or borrow storage that is live with four fighters. One integrator owns shared generators, builds and final candidate identity; do not require subagents or a new worker fleet for this consolidation.

### Minimal start commands

These inspect state or existing documentation; they do not publish a ROM:

```powershell
# PowerShell 7, in the coding agent's existing checkout.
git status --short
git rev-parse HEAD
git diff --stat
Get-Content AGENTS.md
Get-Content docs/BUGS.md
Get-Content docs/BUG_FIXING_PROCESS.md
Get-Content docs/VERIFYING.md

# Locate actual compiled seams before editing. Ripgrep availability is assumed.
rg -n 'nFTKirbyStatusThrowF|efManagerCaptureKirbyStarMakeEffect' src/import
rg -n 'wpManagerMakeWeapon|gFTDataFoxSpecial1' src include
rg -n 'ThunderAmp|0x74' src/import src/port scripts decomp/BattleShip-main/decomp/src/ef
rg -n 'mnVSResultsMakeEmblem|mnVSResultsMakePlaceNumber|WinnerSprite' src decomp/BattleShip-main/decomp/src/mn
rg -n 'Effect DLLINKS admission|board carried a fighter' artifacts/performance/2026-09-19_remaining-bugs.md
rg -n 'mpCommonRunFighterAllCollisions|ndsGRYamabukiGateGObj' src
```

Use symbol searches to find the current implementation, not as absence proofs. Large files, conditional imports and generated code can evade an indexed remote search. Inspect relevant translation units and the exact link map.

### Existing check anchors versus proposed tests

Existing repository tools to select from after checking their current parameter blocks include:

- `scripts/check-architecture.ps1`, `scripts/check-decomp-header-mirror.py`, `scripts/check-untracked-dependencies.py` for changed import/dependency ownership.
- `scripts/check-native-owner-wiring.py`, `scripts/check-fighter-production-manifest.ps1`, `scripts/fighters/check_native_owner_geometry_closure.py`, and `scripts/check_native_only_rom.py` for actual native code/asset/link closure.
- `scripts/check-mp-floor-crossing-fixtures.ps1`, `scripts/check-mp-topology-fixtures.ps1`, and the existing `check_mp_line_extent_reject_exact.py` identified by the Castle receipt for the collision batch; locate its current path and extend applicable fixtures with the captured failing ramp case.
- `scripts/check-audio-bgm-derived-assets.ps1`, `scripts/check-audio-fgm-phase-pack.ps1`, `scripts/check-mn-screen-coverage.ps1`, plus the normal shell lifecycle tests for menu/audio changes.
- `scripts/check-generator-staleness.ps1` and relevant generator `--check` modes for changed producer outputs. Run only flags the current scripts actually support.

The exhaustive Damage-step fixture, cancellable compact-preview transaction fixtures, script-0x74 engagement witness, Results demo-script mutation extension, and event-position/material comparisons described above are **proposed additions or extensions**, not claims that named ready-made verifiers already exist.

### Integration, timing and release evidence

Follow the current verification registry. When selecting coverage or after registry changes, list the relevant profile once:

```powershell
pwsh -NoProfile -File scripts/verify-all.ps1 -Profile Latest -List
# For a strictly battle-only integration, inspect Boundary instead of Latest.
```

Use the actual script parameter blocks to build/run the selected frozen candidate. Do not invent `-Build`/`-NoBuild` combinations or claim a subset is a complete profile. Review B's blanket fresh-build wording is not an additional requirement: reuse a configuration-qualified incremental directory when valid, and perform a fresh build when dependency state, reproducibility or the current verification contract requires one. Distinct BUILD directories do not isolate shared generated inputs. For a final batch spanning shared startup, CSS and Results, Latest is the appropriate registry to inspect; a battle-only Boundary result does not cover the missing menu lifecycle by itself. Run additional targeted source-state cases where profile coverage is insufficient. [Verification procedure][verifying]

The repository distinguishes timing experiments from integration. Four-CPU tick experiments use the direct `verify-p2-four-fighter-stress.ps1` route with its qualified build inputs rather than repeating a full shell/profile cycle for every lever. Correctness and publication still owe their wider checks. Preserve ROM/ELF/config, producer inputs, NitroFS payload identity, emulator configuration, seed/input and exact observed window. A changed linked image needs new timing qualification.

Report **WORK-H P50 and P95 separately**, with units and population, plus presented VBlank interval distribution/max. The documented product gate is approximately P95 ≤1.12 million ticks and at least 95% two-VBlank intervals; a correctness/resource PASS or zero cadence-violation latch does not establish that gate. Record each candidate's actual memory/object/texture headroom and mandatory post-GO demand reads. Do not apply a historical task-specific RAM threshold as a universal rule or count absent required effects as performance improvement. [Verification procedure][verifying]

Use the repo-qualified melonDS capture/measurement procedure with interpreter/JIT settings required by the current workflow; do not silently substitute a different emulator or runtime mode. Natural-input release proof and runtime-forced diagnostic experiments must be labeled separately. Preserve the existing all-rosters/all-stages release requirements; the focused matrix in this guide is bug coverage, not a reduction of the product scope.

---

## 11. What constitutes completion

A row is complete only when the responsible code change and its proof address the owner's actual symptom. For each active row, record the exact first divergence/root cause, edited source/generator ownership, candidate identity, relevant positive/negative fixtures, natural same-ROM scenario, native output, lifecycle/resource results, and any remaining subjective visual acceptance. Reuse compatible completed evidence instead of rerunning it, but state the scope it proves. [Bug workflow][workflow] · [Verification][verifying]

| Rows | Required closure evidence |
|---|---|
| M01/M02 | Disabled-destination behavior; exhaustive Damage step/wrap; normal navigation/save behavior |
| M03/M04 | Audio continuity plus bounded load service and cold/warm first-preview latency; cancellation/re-entry |
| L01/K03 | Exact source effect identified; source/world/native transform agreement by effect age; correct pixels |
| P01 | Ground segments and current images covered; source-matching edge coverage; unchanged gameplay |
| P02 | Actual Thunder self-hit; ThunderAmp/script-0x74 production and native pixels; correct non-self-hit absence |
| P03 | Poké Ball and rays separately present; opening/body/audio timing; shared intro sibling |
| P04/K02/J01 | Independent material-state and visual acceptance for all three fighters |
| K01 | Correct special throw status and paired capture state; no teleport; source damage/release; sibling regressions |
| K04 | Visible capture-star replacement/body restoration plus separate lose-copy-star sibling proof; complete dependency/lifetime closure |
| K05 | Fault cause removed; working repeated copied shots and gun output; no lifetime leak or silent suppression |
| K06/R02/R03 | Reachable demo-state matrix, animation progression, natural outcome/No Contest handoff, re-entry |
| R01 | Identify and prove the affected WinnerSprite and/or series-emblem object; correct outcome/color/layer rules and native resource/lifetime safety |
| S02 | Natural knockback/fall ramp case plus source-derived swept-contact fixture and stage/callback siblings |
| S04 | Source animation and native gate pixels cycle correctly; hazard/collision remain intact |
| S01/S03 | Stay deferred; do not mark fixed from this diagnosis or any unrun proposed change |

Do not mark a row FIXED merely because a build succeeds, an owner is compiled, a counter stays zero, a scene is reached, or a crash disappears after dropping content. Use `IMPLEMENTED_NOT_ACCEPTED` for coherent unaccepted work where the workflow permits it. Preserve owner wording. Any agent-added queue annotation must obey the current bold/length rules; keep diagnosis detail in the existing notes/evidence, not in the short queue. Do not publish or overwrite accepted release artifacts on the basis of this document alone.

### Suggested durable agent directive

> Execute the active rows in this consolidated guide against the current `docs/BUGS.md` and existing execution cursor. Preserve owner deferrals, source wording and already completed repairs. Prioritize the copied-Fox firing crash, Kirby throw alias and exact remaining ramp case. Restore complete effect families; trace ThunderAmp rather than reopening the repaired trail. Distinguish Results badge from series emblem. Reuse the CSS transaction and qualified evidence. Fix the first demonstrated source-to-native divergence without offsets, suppression or fallback. Keep decomp read-only and every ROM native-only. Report implemented, verified and owner-accepted separately.

### Document-only delivery

This handoff does not require a ROM build. When adding it to the repository, use the current documentation placement/registry rules and run the relevant document check and `git diff --check` there. Those repository checks were **not run during this consolidation**. The downloadable file is the deliverable; no commit, push or alteration of the owner queue has been performed.

---

## 12. Source provenance and commit-pinned references

The references below preserve Review A's primary-source ledger and add the narrow GitHub rechecks used to resolve disagreements. Review B's additional file/symbol pointers are included beside the affected instructions. A carried-forward pointer is an implementation entry point, not a claim that its complete file was newly inspected. Source-oracle files under `decomp/` are evidence, not edit targets. Historical reports describe their own candidate binaries and must not be presented as new execution evidence for this consolidation.

**Fresh reconciliation reads:** branch `master`; current `docs/BUGS.md`; VS-options adjustment/held handling; Pikachu Hit motion events; the catch-import alias; Results `MakeEmblem` construction; and the repair-log trail-admission and Castle-board sections. All resolve to the same immutable baseline. The first-place badge branch is additionally supported by the original review's source excerpt and existing Results/OAM references.

| Reference | Source and purpose |
|---|---|
| S01 | [Current owner bug queue][bugs] — `docs/BUGS.md` |
| S02 | [Bug-fixing acceptance and queue rules][workflow] — `docs/BUG_FIXING_PROCESS.md` |
| S03 | [Build, verification, measurement and publication procedure][verifying] — `docs/VERIFYING.md` |
| S04 | [Current repository agent instructions][agents] — `AGENTS.md` |
| S05 | [September 19–21 repair evidence and superseding observations][repairlog] — `artifacts/performance/2026-09-19_remaining-bugs.md` |
| S06 | [Earlier September 19 diagnosis — historical, partly superseded][oldguide] — `docs/p2/BUGS_REMAINING_DIAGNOSIS_2026-09-19.md` |
| S07 | [Detailed bug investigation notes][bugnotes] — `docs/p2/BUG_NOTES.md` |
| S08 | [Current fighter status wrapper and topology/render-cache invalidation][ftmain] — `src/import/battleship_ftmain.c` |
| S09 | [Data-menu selection and scene dispatch][menu_data] — `src/nds/nds_menu_shell_data.c` |
| S10 | [VS-options Damage input and wrap behavior][vsoptions] — `src/nds/nds_menu_shell_vsoptions.c` |
| S11 | [CSS preview residency, compact loading and cancellation][css_import] — `src/import/battleship_mnplayersvs.c` |
| S12 | [Current native DamageSlash executor][slash_exec] — `src/nds/nds_native_damage_slash.exec.inc` |
| S13 | [Source-derived DamageSlash generator][slash_gen] — `scripts/3d_vfx/generate_nds_damage_slash.py` |
| S14 | [Historical DamageSlash native-output proof, September 11][slash_proof] — `artifacts/visibility/2026-09-11_damage-slash-native.md` |
| S15 | [Ground Thunder Jolt six-segment native executor][jolt_ground] — `src/nds/nds_native_pikachu_thunderground.exec.inc` |
| S16 | [Ground Thunder Jolt source contract and generator][jolt_gen] — `scripts/stages/generate_nds_native_pikachu_thunderground.py` |
| S17 | [Thunder Jolt effect quad path][jolt_effect] — `src/nds/nds_native_pikachu_thunderjolt_effect.exec.inc` |
| S18 | [Source Pikachu Thunder state machine and self-contact handling][pika_lw] — `decomp/BattleShip-main/decomp/src/ft/ftchar/ftpikachu/ftpikachuspeciallw.c` |
| S19 | [Source Pikachu appearance and Thunder self-hit motion events][pika_motion] — `decomp/BattleShip-main/decomp/src/relocData/242_PikachuMainMotion.c` |
| S20 | [Source fighter effect dispatch and coordinate preparation][ftparam] — `decomp/BattleShip-main/decomp/src/ft/ftparam.c` |
| S21 | [Source effect kinds, including ThunderAmp self-hit][efdef] — `decomp/BattleShip-main/decomp/src/ef/efdef.h` |
| S22 | [Source effect constructors and particle scripts][efmanager] — `decomp/BattleShip-main/decomp/src/ef/efmanager.c` |
| S23 | [Current fighter-entry import and omitted Poké Ball call][entry] — `src/import/battleship_ftcommon_entry.c` |
| S24 | [Production native fighter material, light and packet execution][fighter_exec] — `src/nds/nds_renderer_native_fighter_production.c` |
| S25 | [Catch/throw import and Kirby status alias][catch_import] — `src/import/battleship_ftcommon_catch.c` |
| S26 | [Source common throw selection and paired victim state][throw_source] — `decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommonthrow.c` |
| S27 | [Source Kirby special forward-throw motion and callbacks][kirby_throw] — `decomp/BattleShip-main/decomp/src/ft/ftchar/ftkirby/ftkirbythrowf.c` |
| S28 | [Source captured-fighter attachment transform, including region branches][capture_transform] — `decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommoncapturepulled.c` |
| S29 | [Historical upstream attachment-cache investigation, not a DS runtime proof][capture_note] — `decomp/BattleShip-main/docs/bugs/grab_pose_stale_root_transform_2026-05-23.md` |
| S30 | [Kirby import declarations and capture-star suppression macro][kirby_common] — `src/import/battleship_kirby_common.h` |
| S31 | [Current Kirby captured-fighter import][capture_import] — `src/import/battleship_ftcommon_capturekirby.c` |
| S32 | [Source Kirby capture/spit star lifecycle and body visibility][capture_source] — `decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommoncapturekirby.c` |
| S33 | [Source Kirby copied-Fox firing path][kirby_fox] — `decomp/BattleShip-main/decomp/src/ft/ftchar/ftkirby/ftkirbycopyfoxspecialn.c` |
| S34 | [Source Fox blaster descriptor and constructor][fox_source] — `decomp/BattleShip-main/decomp/src/wp/wpfox/wpfoxblaster.c` |
| S35 | [Current Fox blaster import and shared wrapper][fox_import] — `src/import/battleship_fox_blaster.c` |
| S36 | [Current weapon construction and attribute dereference path][wpmanager] — `src/import/battleship_wpmanager_core.c` |
| S37 | [Source Results outcome, demo selection and WinnerSprite construction][results_source] — `decomp/BattleShip-main/decomp/src/mn/mnvsmode/mnvsresults.c` |
| S38 | [Source demo fighter status/update machinery][scsubsys] — `decomp/BattleShip-main/decomp/src/sc/scsubsys/scsubsysfighter.c` |
| S39 | [Current relocated asset symbol registry][reloc_symbols] — `include/reloc_data.h` |
| S40 | [Native Results sprite decoding, tiling, resources and output][results_oam] — `src/nds/nds_results_oam.c` |
| S41 | [Source map-collision control flow and callbacks][mpcommon] — `decomp/BattleShip-main/decomp/src/mp/mpcommon.c` |
| S42 | [Source Saffron gate animation, hazard state and collision updates][yamabuki_source] — `decomp/BattleShip-main/decomp/src/gr/grcommon/gryamabuki.c` |
| S43 | [Current Saffron gate import, animation offsets and accessor][yamabuki_import] — `src/import/battleship_gryamabuki_ground.c` |
| C01 | [Later Thunder/DLLINKS/Ness-tail repair entry][trail_repair] — `artifacts/performance/2026-09-19_remaining-bugs.md`, lines 486–502; supersedes the older rejection claim |
| C02 | [Actual Castle wall/board verification scope][castle_repair] — same repair log, lines 861–895; not a blanket upper-ramp acceptance |
| C03 | [Source series-emblem constructor/update][results_emblem] — Results source, lines 586–677; distinct from the WinnerSprite |

[bugs]: https://github.com/rockenrooster/Smash64DS_Port/blob/77d868b0d9be8022b3c6950ef9213f6e21629a50/docs/BUGS.md
[workflow]: https://github.com/rockenrooster/Smash64DS_Port/blob/77d868b0d9be8022b3c6950ef9213f6e21629a50/docs/BUG_FIXING_PROCESS.md
[verifying]: https://github.com/rockenrooster/Smash64DS_Port/blob/77d868b0d9be8022b3c6950ef9213f6e21629a50/docs/VERIFYING.md
[agents]: https://github.com/rockenrooster/Smash64DS_Port/blob/77d868b0d9be8022b3c6950ef9213f6e21629a50/AGENTS.md
[repairlog]: https://github.com/rockenrooster/Smash64DS_Port/blob/77d868b0d9be8022b3c6950ef9213f6e21629a50/artifacts/performance/2026-09-19_remaining-bugs.md
[oldguide]: https://github.com/rockenrooster/Smash64DS_Port/blob/77d868b0d9be8022b3c6950ef9213f6e21629a50/docs/p2/BUGS_REMAINING_DIAGNOSIS_2026-09-19.md
[bugnotes]: https://github.com/rockenrooster/Smash64DS_Port/blob/77d868b0d9be8022b3c6950ef9213f6e21629a50/docs/p2/BUG_NOTES.md
[ftmain]: https://github.com/rockenrooster/Smash64DS_Port/blob/77d868b0d9be8022b3c6950ef9213f6e21629a50/src/import/battleship_ftmain.c
[menu_data]: https://github.com/rockenrooster/Smash64DS_Port/blob/77d868b0d9be8022b3c6950ef9213f6e21629a50/src/nds/nds_menu_shell_data.c
[vsoptions]: https://github.com/rockenrooster/Smash64DS_Port/blob/77d868b0d9be8022b3c6950ef9213f6e21629a50/src/nds/nds_menu_shell_vsoptions.c
[css_import]: https://github.com/rockenrooster/Smash64DS_Port/blob/77d868b0d9be8022b3c6950ef9213f6e21629a50/src/import/battleship_mnplayersvs.c
[slash_exec]: https://github.com/rockenrooster/Smash64DS_Port/blob/77d868b0d9be8022b3c6950ef9213f6e21629a50/src/nds/nds_native_damage_slash.exec.inc
[slash_gen]: https://github.com/rockenrooster/Smash64DS_Port/blob/77d868b0d9be8022b3c6950ef9213f6e21629a50/scripts/3d_vfx/generate_nds_damage_slash.py
[slash_proof]: https://github.com/rockenrooster/Smash64DS_Port/blob/77d868b0d9be8022b3c6950ef9213f6e21629a50/artifacts/visibility/2026-09-11_damage-slash-native.md
[jolt_ground]: https://github.com/rockenrooster/Smash64DS_Port/blob/77d868b0d9be8022b3c6950ef9213f6e21629a50/src/nds/nds_native_pikachu_thunderground.exec.inc
[jolt_gen]: https://github.com/rockenrooster/Smash64DS_Port/blob/77d868b0d9be8022b3c6950ef9213f6e21629a50/scripts/stages/generate_nds_native_pikachu_thunderground.py
[jolt_effect]: https://github.com/rockenrooster/Smash64DS_Port/blob/77d868b0d9be8022b3c6950ef9213f6e21629a50/src/nds/nds_native_pikachu_thunderjolt_effect.exec.inc
[pika_lw]: https://github.com/rockenrooster/Smash64DS_Port/blob/77d868b0d9be8022b3c6950ef9213f6e21629a50/decomp/BattleShip-main/decomp/src/ft/ftchar/ftpikachu/ftpikachuspeciallw.c
[pika_motion]: https://github.com/rockenrooster/Smash64DS_Port/blob/77d868b0d9be8022b3c6950ef9213f6e21629a50/decomp/BattleShip-main/decomp/src/relocData/242_PikachuMainMotion.c
[ftparam]: https://github.com/rockenrooster/Smash64DS_Port/blob/77d868b0d9be8022b3c6950ef9213f6e21629a50/decomp/BattleShip-main/decomp/src/ft/ftparam.c
[efdef]: https://github.com/rockenrooster/Smash64DS_Port/blob/77d868b0d9be8022b3c6950ef9213f6e21629a50/decomp/BattleShip-main/decomp/src/ef/efdef.h
[efmanager]: https://github.com/rockenrooster/Smash64DS_Port/blob/77d868b0d9be8022b3c6950ef9213f6e21629a50/decomp/BattleShip-main/decomp/src/ef/efmanager.c
[entry]: https://github.com/rockenrooster/Smash64DS_Port/blob/77d868b0d9be8022b3c6950ef9213f6e21629a50/src/import/battleship_ftcommon_entry.c
[fighter_exec]: https://github.com/rockenrooster/Smash64DS_Port/blob/77d868b0d9be8022b3c6950ef9213f6e21629a50/src/nds/nds_renderer_native_fighter_production.c
[catch_import]: https://github.com/rockenrooster/Smash64DS_Port/blob/77d868b0d9be8022b3c6950ef9213f6e21629a50/src/import/battleship_ftcommon_catch.c
[throw_source]: https://github.com/rockenrooster/Smash64DS_Port/blob/77d868b0d9be8022b3c6950ef9213f6e21629a50/decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommonthrow.c
[kirby_throw]: https://github.com/rockenrooster/Smash64DS_Port/blob/77d868b0d9be8022b3c6950ef9213f6e21629a50/decomp/BattleShip-main/decomp/src/ft/ftchar/ftkirby/ftkirbythrowf.c
[capture_transform]: https://github.com/rockenrooster/Smash64DS_Port/blob/77d868b0d9be8022b3c6950ef9213f6e21629a50/decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommoncapturepulled.c
[capture_note]: https://github.com/rockenrooster/Smash64DS_Port/blob/77d868b0d9be8022b3c6950ef9213f6e21629a50/decomp/BattleShip-main/docs/bugs/grab_pose_stale_root_transform_2026-05-23.md
[kirby_common]: https://github.com/rockenrooster/Smash64DS_Port/blob/77d868b0d9be8022b3c6950ef9213f6e21629a50/src/import/battleship_kirby_common.h
[capture_import]: https://github.com/rockenrooster/Smash64DS_Port/blob/77d868b0d9be8022b3c6950ef9213f6e21629a50/src/import/battleship_ftcommon_capturekirby.c
[capture_source]: https://github.com/rockenrooster/Smash64DS_Port/blob/77d868b0d9be8022b3c6950ef9213f6e21629a50/decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommoncapturekirby.c
[kirby_fox]: https://github.com/rockenrooster/Smash64DS_Port/blob/77d868b0d9be8022b3c6950ef9213f6e21629a50/decomp/BattleShip-main/decomp/src/ft/ftchar/ftkirby/ftkirbycopyfoxspecialn.c
[fox_source]: https://github.com/rockenrooster/Smash64DS_Port/blob/77d868b0d9be8022b3c6950ef9213f6e21629a50/decomp/BattleShip-main/decomp/src/wp/wpfox/wpfoxblaster.c
[fox_import]: https://github.com/rockenrooster/Smash64DS_Port/blob/77d868b0d9be8022b3c6950ef9213f6e21629a50/src/import/battleship_fox_blaster.c
[wpmanager]: https://github.com/rockenrooster/Smash64DS_Port/blob/77d868b0d9be8022b3c6950ef9213f6e21629a50/src/import/battleship_wpmanager_core.c
[results_source]: https://github.com/rockenrooster/Smash64DS_Port/blob/77d868b0d9be8022b3c6950ef9213f6e21629a50/decomp/BattleShip-main/decomp/src/mn/mnvsmode/mnvsresults.c
[scsubsys]: https://github.com/rockenrooster/Smash64DS_Port/blob/77d868b0d9be8022b3c6950ef9213f6e21629a50/decomp/BattleShip-main/decomp/src/sc/scsubsys/scsubsysfighter.c
[reloc_symbols]: https://github.com/rockenrooster/Smash64DS_Port/blob/77d868b0d9be8022b3c6950ef9213f6e21629a50/include/reloc_data.h
[results_oam]: https://github.com/rockenrooster/Smash64DS_Port/blob/77d868b0d9be8022b3c6950ef9213f6e21629a50/src/nds/nds_results_oam.c
[mpcommon]: https://github.com/rockenrooster/Smash64DS_Port/blob/77d868b0d9be8022b3c6950ef9213f6e21629a50/decomp/BattleShip-main/decomp/src/mp/mpcommon.c
[yamabuki_source]: https://github.com/rockenrooster/Smash64DS_Port/blob/77d868b0d9be8022b3c6950ef9213f6e21629a50/decomp/BattleShip-main/decomp/src/gr/grcommon/gryamabuki.c
[yamabuki_import]: https://github.com/rockenrooster/Smash64DS_Port/blob/77d868b0d9be8022b3c6950ef9213f6e21629a50/src/import/battleship_gryamabuki_ground.c

[trail_repair]: https://github.com/rockenrooster/Smash64DS_Port/blob/77d868b0d9be8022b3c6950ef9213f6e21629a50/artifacts/performance/2026-09-19_remaining-bugs.md#L486-L502
[castle_repair]: https://github.com/rockenrooster/Smash64DS_Port/blob/77d868b0d9be8022b3c6950ef9213f6e21629a50/artifacts/performance/2026-09-19_remaining-bugs.md#L861-L895
[results_emblem]: https://github.com/rockenrooster/Smash64DS_Port/blob/77d868b0d9be8022b3c6950ef9213f6e21629a50/decomp/BattleShip-main/decomp/src/mn/mnvsmode/mnvsresults.c#L586-L677
