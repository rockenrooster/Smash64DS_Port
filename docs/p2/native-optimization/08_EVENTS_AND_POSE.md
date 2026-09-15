# N05 — Event clock, compact animation and required pose

> **Revision 2 coverage:** Universal scope: four identical fighters can be in four different clips, event phases, copy/attachment states and hitlag conditions. Keep each live clock, dependency mask and mutable transform state independent while sharing only immutable tracks. See [16_ALL_ROSTERS_ALL_STAGES.md](16_ALL_ROSTERS_ALL_STAGES.md).

**Target:** source-correct discrete animation/gameplay events and compact fixed pose values, without per-joint binary32 emulation, repeated source parsing or permanent fixed→float→fixed publication. Sources [S13, S14, S15, S20].

## Event clock implementation decision

Do not assume one clock per fighter until the source has proved it. Joint scripts can have independent waits/loops, and the current engine publishes the last source-ordered writer to the GObj clock. Bind-time metadata must preserve event lanes and publication order where these are semantically observable.

Primary representation: an integer logic-tick stamp, explicit clip/event cursor, qualified fixed phase and optional rational remainder for ratio-derived speeds. For finite admitted constant-speed segments, generate source-derived integer event deadlines/transition metadata host-side. When a source operation changes speed or seeks, re-anchor according to the current logical state; do not recompute from an idealized absolute fraction and discard a behaviorally important accumulated remainder.

A concrete decision sequence is mandatory: enumerate actual speed constructors → reconstruct source event traces → test fixed phase plus residual → use generated deadline corrections for finite source regimes where needed → separately prove dynamic speed-change/rebound/interrupt cases. A generated schedule is not sufficient for an unrestricted input domain. If a case cannot yet be proven, it remains unmigrated and the **final no-float gate stays open**; do not retain integer IEEE emulation as the claimed endpoint.

Ordinary Q12 cannot represent 1/3 exactly. Nor does merely raising precision guarantee repeated-IEEE boundary identity. The existing source records real landing/animation boundary mismatches. Preserve those cases in the oracle rather than treating the historical comment as permission for a global epsilon. [S13]

## Pose representation

Compile per-clip static channels, active tracks, track-to-joint mapping, interpolation coefficients, transform class and source event dependencies. Use compact native values and indices, not a second linked AObj-like graph. Keep dynamic procedural channels explicit. Constant values need no per-frame evaluation or copy unless a consumer requires publication.

At topology/consumer change derive required-joint closure: gameplay root motion, hurt/hit volumes, grab/throw, weapon/item/attachment sockets and their ancestors. Update this closure at the existing required logic cadence. Visual-only evaluation may follow the existing sanctioned presentation cadence, but this campaign does not lower gameplay rates. Invisible fighters can still have gameplay-active joints.

Record local pose generation and world transform generation separately. Camera-only changes should not invalidate local pose. GPU matrices must not become the authoritative gameplay representation, and there is no GPU-readback loop to recover hitboxes.

### N05.01 — Derive event lanes and timing semantics

**Depends on:** N04.02, N00.05

**Edit/inspect boundary:** `src/nds/nds_ft_pose.c`; `src/import/battleship_ftanim.c`; `src/import/battleship_ftmain.c`; `scripts/fighters/test_pose_clock_differential.py`.

**Implementation sequence**

1. Enumerate wait/frame/speed constructors and every writer/reader, including source sentinels, joint lanes, GObj last-writer publication and script event order.
2. Extract real constant, ratio-derived and dynamic speed cases with attach/seek/start-frame/loop/rebound/landing semantics.
3. Build source-driven traces with exact event tick/order and discrete status outcomes, not only pose-coordinate comparisons.
4. Specify fixed clock/remainder/deadline rules from those cases, including overflow/wrap and invalid speed handling.

**Required tests/evidence:** T-CLOCK retained Q12 mismatch examples, actual landing/rebound cases, speed transitions, pauses/hitlag and distinct joint lanes.

**Work or dependency retired:** Unsupported assumptions about one global exact fixed clock.

**Done:** Every admitted speed/event regime has an explicit source-derived behavior contract and planned native representation.

**Stop/revert:** Do not remove the existing clock until its semantic replacements pass; a rational clock is not automatically source-equivalent.

### N05.02 — Implement fixed event stepping and deadline corrections

**Depends on:** N05.01, N04.03

**Edit/inspect boundary:** `src/nds/nds_ft_pose.c`; `include/nds/nds_f32_exact.h`; `proposed native event metadata/compiler`.

**Implementation sequence**

1. Implement integer event state and qualified phase/remainder update with a bounded number of source-ordered events per tick.
2. Generate deadline/transition metadata for proved finite source regimes; handle dynamic speed changes by the specified re-anchor rule, not a global epsilon.
3. Preserve hitlag freezing, multiple events in one tick, loops/end/changed/null states, interruptions and the correct GObj publication lane.
4. Compare events against the independent source oracle and isolate each mismatch to source regime and boundary before changing the rule.

**Required tests/evidence:** T-CLOCK exhaustive admitted finite regimes plus generated dynamic traces; exact event/order/status match, bounded runtime work and no integer overflow.

**Work or dependency retired:** Per-joint integer IEEE add/sub and repeated source wait arithmetic in qualified regimes.

**Done:** Native stepping passes the admitted semantic corpus and target-cost check; unresolved regimes remain explicitly open.

**Stop/revert:** Do not pad every duration by one tick or silently keep binary32 under a new name. Failure requires case-specific redesign.

### N05.03 — Compile compact fixed tracks and constant channels

**Depends on:** N02.03, N04.03, N05.01

**Edit/inspect boundary:** `src/nds/nds_ftanim_track.c`; `src/nds/nds_ft_pose.c`; `include/nds/nds_anim_fixed.h`; `existing fighter motion generator`.

**Implementation sequence**

1. Compile static channels, active track maps, interpolation coefficients and interpolation classes using source values and approved quantization.
2. Generate bounded indices into resident data; use constant/linear/cubic-specific kernels without scanning inactive track classes.
3. Keep event metadata separate from visual samples. Procedural state, live facing/scale and attachments are runtime inputs, not baked as final world coordinates.
4. Compare compact coefficient/keys versus selective sampled locals for both memory and integrated CPU cost before choosing by family.

**Required tests/evidence:** T-POSE constant endpoints, interpolation extrema, per-track error/range proof, malformed spans and bank determinism; no event timing changes.

**Work or dependency retired:** Repeated source track interpretation, invariant coefficient generation and inactive/constant work beyond the existing masks.

**Done:** A compact resident representation replaces a named track path with measured whole-domain benefit.

**Stop/revert:** Do not claim the already-landed live/run masks again; a larger bake that increases mandatory reads is rejected.

### N05.04 — Create topology-owned required-joint closures

**Depends on:** N03.02, N05.03

**Edit/inspect boundary:** `src/nds/nds_ft_pose.c`; `src/import/battleship_ftmain.c`; `src/port/renderer_adapter_matrix.c`; `gameplay joint/socket consumers`.

**Implementation sequence**

1. Enumerate gameplay consumers of each joint and their ancestor dependencies, including hurtboxes, attacks, captures, items and procedural attachments.
2. Build a bounded dense closure on bind/topology/consumer change and maintain source ordering where it matters.
3. Use explicit counts/bitsets sized for the actually admitted hierarchy; a future wide hierarchy requires a generated complete plan, not silently truncated 64-bit masks.
4. Keep visual-only work separate while preserving existing logic/event rates and gameplay-active invisible/offscreen joints.
5. Dependency closures are per live instance and topology generation. Test four copies of one fighter with different events/attachments and mixed copy/capture relationships; changing one instance must not change another instance's running-joint mask.

**Required tests/evidence:** T-POSE ancestor closure, hidden active attacks, dynamically enabled heavy-item joint, copy/morph and wide-hierarchy negative fixtures.

**Work or dependency retired:** Repeated dependency discovery and unnecessary full hierarchy processing for gameplay consumers.

**Done:** Every required socket is current at its consumption tick with no dependence on whether the fighter was drawn.

**Stop/revert:** Missing one ancestor or treating visibility as gameplay inactivity blocks the change.

### N05.05 — Make native pose authoritative through the draw pilot

**Depends on:** N05.02, N05.03, N05.04, N03.07

**Edit/inspect boundary:** `src/nds/nds_ft_pose.c`; `include/nds/nds_ft_pose.h`; `src/port/renderer_adapter_matrix.c`; `src/nds/nds_renderer_native_fighter_production.c`.

**Implementation sequence**

1. Publish native local pose/matrices directly to the bound draw/patch API; delete fixed→DObj-float→renderer-fixed conversions in that chain.
2. Distinguish local pose, world and camera-dependent generations, directly reference stable data and avoid per-draw whole-matrix workspace copies.
3. For unmigrated gameplay consumers, publish a temporary one-way legacy bridge once at the required logic boundary; name its deletion dependency and measure its cost.
4. Check the full pilot move/entry/capture/damage lifecycle and four-instance behavior before expanding the representation.

**Required tests/evidence:** T-POSE/T-XFORM/T-GEOM complete pilot states; audit shows converted draw never reads legacy float pose; T-MEAS includes temporary bridge cost.

**Work or dependency retired:** Pose-to-render conversion sandwiches, duplicated visual pose authority and redundant copied transforms.

**Done:** Native pose drives the full pilot render chain with a net integrated improvement; legacy gameplay bridge is explicitly not final closure.

**Stop/revert:** A native evaluator with all old publication and rebuild work still running is not a completed pose slice.

### N05.06 — Share local/world transform results correctly

**Depends on:** N05.05

**Edit/inspect boundary:** `src/port/renderer_adapter_matrix.c`; `src/nds/nds_ft_pose.c`; `gameplay attachment and matrix consumers`.

**Implementation sequence**

1. Use one native local result per changed joint and one required world/socket result per consumer epoch.
2. Compose only required CPU ancestor paths; use generated/bound GX matrix routes for visual-only transforms when that eliminates rather than duplicates CPU work.
3. Preserve source special transform classes and dynamic scale/facing/attachment semantics; distinguish affine model work from camera projection.
4. Replace whole-tree per-tick invalidation with precise mutation-owned dirty descendants after all writers are accounted for.

**Required tests/evidence:** T-XFORM CPU socket/GX visual correspondence, camera-only/world-only changes, procedural attachments and negative scales; matrix count/bytes witnesses.

**Work or dependency retired:** Duplicated CPU/GX hierarchy composition and whole-tree invalidation for unaffected work.

**Done:** Transform counts scale with actual changed/required nodes and no gameplay result waits on GX readback.

**Stop/revert:** A faster visual hierarchy with stale gameplay sockets is rejected; keep special cases native rather than forcing generic TRS.

### N05.07 — Expand pose and event conversion across content

**Depends on:** N05.06

**Edit/inspect boundary:** `src/nds/nds_ft_pose.c`; `src/import/battleship_ftanim.c`; `fighter motion/event banks`; `source-derived pose coverage manifest`.

**Implementation sequence**

1. Convert the current stress roster, duplicated fighter kinds, all remaining fighters, copy variants and non-battle previews by explicit family coverage.
2. Include animation end, landing speed, hitlag, simultaneous status changes, entry, KO, respawn, grabs/throws and uncommon procedural tracks.
3. Keep the independent source oracle host-side and use source-controller tours for states ordinary CPU behavior does not trigger.
4. Update domain helper/bridge inventory and compact bank capacity after each qualified family; do not accumulate permanent per-fighter compatibility engines.
5. Qualify clocks, motion variants and sockets for every required fighter, including duplicate instances on different clips and source-legal paired captures/throws across stages. A shared animation bank never shares mutable event cursors.

**Required tests/evidence:** T-CLOCK/T-POSE complete admitted regime matrix; T-COVER real event/variant engagement; T-RES fit and no mandatory motion reads for claimed locked profiles.

**Work or dependency retired:** Remaining converted-family source pose/event parsing and representation conversion.

**Done:** All claimed families have semantic/runtime coverage; unimplemented content remains explicitly open.

**Stop/revert:** A zero mismatch count over zero comparisons or untriggered state does not close a family.

### N05.08 — Remove legacy pose and clock authority

**Depends on:** N05.07, N06.04

**Edit/inspect boundary:** `src/nds/nds_ft_pose.c`; `include/nds/nds_ft_pose.h`; `include/nds/nds_f32_exact.h`; `src/import/battleship_ftanim.c`.

**Implementation sequence**

1. Confirm all required gameplay, render, preview and event consumers of the converted scene/domain use native state.
2. Remove one-way float publication bridges, old event-clock representations, shadow AObj/DObj state and obsolete parser helpers for those consumers.
3. Keep only necessary source compatibility outside the fully converted scope and label it a remaining runtime-float dependency until N09 closure.
4. Reclaim retired memory and rerun semantic, resource and whole-frame tests on the final hard-on build.

**Required tests/evidence:** T-FLOAT domain closure, T-RETIRE reader/caller proof, T-CLOCK and T-LIFE state/cadence coverage; memory availability confirmed at admission.

**Work or dependency retired:** Permanent duplicate pose/event authority and integer binary32 clock emulation in completed domains.

**Done:** Native pose/event producers and all consumers agree on one representation, with the old bridge physically gone.

**Stop/revert:** Do not delete fields while a source-imported callback still consumes their float ABI; move that consumer first.

