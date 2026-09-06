# 06 — Preserve gameplay time, order, and object semantics

## Simulation rate is not a presentation detail

Identify what advances the source: VI retraces, a scheduler divisor, elapsed time,
fixed logical ticks, or an intentionally frame-coupled loop. Check region-specific
behavior and pause/catch-up. The source VI event interface can select a retrace
count between messages; the existence of a nominal video rate alone does not
specify the game's logical rate. [Source VI events][vi]

Preserve the chosen canonical behavior. A target that presents less frequently
can still execute the required number of source ticks and render the latest state.
Do not halve timers, multiply velocities, or skip every other update as a blanket
30-FPS conversion. Two discrete collision/state updates are not generally
identical to one larger update.

[The tick helper](../examples/tick_ratio.h) accounts for rational source ticks per
caller-defined clock pulse. Its steady path has no divide. It keeps accumulated
debt until an update is actually committed, and reports overflow instead of
silently losing time. It does not decide what a pulse means or solve overload.

A fixed 2:1 source-tick/presentation ratio is valid only when the project's clock
contract makes it so. Actual display cadence and source timing may differ. Use
the project's timer or explicit chosen cadence; do not infer “exact 60 Hz” from
an API name or a VBlank count. Account elapsed pulses even if no frame was drawn.

## Input belongs to the source tick sequence

When multiple source ticks execute per target presentation, held state may carry
through, but press/release edges must be delivered according to the chosen input
sampling contract. Sampling once and treating `keysDown()` as true for every
catch-up tick can duplicate a jump, menu activation, or attack. Merely clearing
all edges after the first tick can also lose an intervening press/release if
samples were queued. Preserve source-relevant sample order/timestamps.

Map N64 analog input deliberately: dead zone, magnitude, directional priority,
neutral/reversal behavior, and button mapping. Do not fabricate full analog
magnitude from a digital direction without acknowledging the chosen adaptation.
UI and gameplay should consume one consistent normalized input owner.

## Updates have observable order

For source object/process lists, determine insertion/removal behavior and whether
newly spawned objects run in the same tick. Callback order can change RNG,
collision tie-breaking, effects, and gameplay. Preserve it when converting linked
lists to compact arrays, buckets, or callback tables.

A swap-remove pool changes traversal order. That can be fine for order-independent
particles, but not automatically for actors or collision contacts. Use stable
indexes, an ordered active list, deferred mutation matching the source, or another
explicit order-preserving representation. Resolve known callbacks once without
reordering their invocation.

Eliminate dead dispatch work where state is known, not semantic work. For example,
select an actor's update variant when its action changes rather than branching
through all action classes at each substep. Invalidation must occur on every
source transition that affects the selected path.

## Collision optimization with a behavioral boundary

Specialize static stage geometry offline into useful cells, planes, bounds, or
adjacency. Keep moving collision in an appropriate dynamic representation.
Broad-phase pruning must be conservative over motion, not only the end position;
it must not miss fast traversal or source-defined swept tests.

Retain narrow-phase predicates, source endpoint conventions, one-way platform
rules, contact ordering, and response timing unless changes are approved. An
alternative spatial index can return the same candidates in a different order;
restore the source tie-break when that order is observable. Quantized bounds
should expand conservatively rather than exclude a possible source contact.

Use squared distance, precomputed plane terms, or reciprocal transforms only
when sign, overflow, degenerate cases, and required rounding preserve the result.
Measure whether candidate count or narrow-phase arithmetic is the real cost.
Do not automatically partition a tiny actor set into a costly general structure.

## Scripts and events

Compile immutable scripts into typed actions/direct tables where it removes
repeated decode or relocation. Preserve branch conditions, wait-unit semantics,
callback order, loops, and runtime patching. A script pointer can be a continuation
state or identity token; replacing it with a byte index requires consistent
consumer migration.

Animation sound/effect/hitbox events should follow authoritative simulation time,
not whether that animation frame was rendered. When an update crosses multiple
events, process the source-ordered interval, including wrap/loop boundaries.
Pausing display or using a cached render pose must not accidentally repeat or
suppress gameplay events.

## Bound overload without pretending it did not happen

A catch-up cap can bound work attempted in one service iteration; it does not
authorize discarding remaining simulation time. Retain debt and implement the
project's explicit slow/pause/failure policy. Exact real-time behavior cannot be
maintained if the sustained required work exceeds available time. Report and
remove that work rather than claiming that a dropped update is an optimization.

Read-only presentation interpolation is an optional project decision. Never write
interpolated positions back into authoritative state. Collision/hitboxes and
joint-attached gameplay effects still consume the correct source tick/pose.

[vi]: https://ultra64.ca/files/documentation/online-manuals/man/n64man/os/osViSetEvent.html
