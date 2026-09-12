# 06 — Time, order, collision, and events

Identify the real source clock: VI messages/divisor, logical ticks, elapsed time, region-specific rate and pause/catch-up behavior. Presentation rate does not define simulation rate. Do not halve timers, multiply velocities or drop alternate updates as a blanket 30-FPS conversion; two collision/state-machine ticks are not generally one double-size tick.

[tick_ratio.h](../examples/tick_ratio.h) accounts for rational ticks per **caller-defined pulse**, retaining debt until a real update commits. It does not define an exact 60-Hz clock or solve overload. Account elapsed pulses even when no frame renders. A catch-up cap bounds attempted work, not permission to erase remaining time; apply the project's explicit slowdown/pause/failure policy and report sustained overload.

Sample input under one owner and deliver held/edge state to the appropriate source ticks. Repeating one `keysDown()` over catch-up ticks duplicates actions; clearing all queued edges after the first tick can lose intervening presses/releases. Preserve timestamps/order and deliberately map N64 analog deadzone, magnitude, reversals and directional priorities to DS controls.

## Observable update order

Preserve source insertion/removal, same-tick spawn rules, callbacks, collision ties and RNG consumption. Dense pools or prebound callbacks are valid when invocation order remains equivalent. Swap-remove silently changes traversal; use stable indices/ordered active lists unless independence is proven. Rebind specialized variants at every state transition affecting them.

Compile immutable scripts to typed actions/tables without changing branches, wait units, continuation identity, callback order, loops or dynamic patches. Events follow simulation time, not rendered animation frames. Process every crossed event interval in source order, including wrap/reverse/loop semantics; cached poses and paused display must not repeat or suppress events.

## Collision

Precompute static stage cells/planes/bounds/adjacency and retain moving geometry dynamically. Broad-phase bounds must conservatively cover required motion/sweeps, not merely end positions. Restore source candidate/contact tie order where observable. Preserve narrow-phase predicates, endpoint conventions, one-way rules and response timing unless adaptation is approved.

Quantized bounds expand conservatively. Squared-distance, reciprocal or precomputed-plane substitutions require matching sign, width, degenerates and rounding. Measure candidate count versus arithmetic before adding a general spatial structure for a tiny actor set.

Presentation interpolation is read-only and optional; never write interpolated state back into authoritative simulation. Hitboxes and attached gameplay effects still need the correct tick/pose. Validate per-tick canonical state/events across slow frames, no-render ticks, press/release queues, pause, spawn/delete, reset and rematch. [Sources](SOURCES.md).
