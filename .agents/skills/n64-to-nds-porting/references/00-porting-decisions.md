# 00 — Choose the port boundary

The efficient unit of porting is a source **behavior or subsystem**, not an N64
function name. Retain useful algorithmic code and give it a DS-native boundary.
A thin adapter is appropriate for a cheap portable service; translating an entire
machine interface is not a prerequisite for a source port.

## What normally survives, and what normally changes

| Source work | Starting treatment | Preserve or prove |
|---|---|---|
| Rules/state machines | Keep control flow and state transitions | Source order, tick counts, RNG consumption, integer/float behavior |
| Collision | Retain narrow-phase rules; specialize candidates/data if needed | Boundary decisions, contact order, normals, response timing |
| Animation | Compile immutable channels/topology; evaluate required dynamic channels | Interpolation, blends, events, hierarchy, attachment timing |
| Static display lists | Host decode to typed geometry/material plan | Inherited state, load-time vertex versions, ordering |
| Generated display lists | Replace the producer with direct native draws or typed updates | Every changing source input and its lifetime |
| N64 texture loads/tiles | Convert logical sampled images and sampling rules | Palette generation, tile origin/shift/mask, alpha semantics |
| N64 OS/RSP/RDP plumbing | Replace with existing DS owners/services | Required ordering and completion, not source task structure |
| Source object/asset graph | Compact only the expensive representation | Identity, aliasing, late references, writes, callback order |

These are design defaults, not permission to skip semantic verification.

## Four migration shapes

**Direct native implementation.** Best when source logic is already understandable
and its inputs are bounded. Example: a projectile update retains its state machine
and uses an existing DS collision and effect API. Do not send it through a fake
`OSTask` merely because the source did.

**Build-time translation.** Best when a stable data program creates repeated work:
static display lists, animation instruction streams, texture conversion, and
cross-file relocation metadata. Prefer output the target actually consumes.
An exporter that emits an easier-to-interpret source format only moves the problem.

**Setup-time preparation.** Best when the selected scene, actors, equipment,
palettes, or binding addresses are not known at build time. Resolve once for that
lifetime; keep runtime invalidation explicit. Setup float or division can be a
sensible trade if it eliminates thousands of repeated operations.

**Bounded runtime interpretation.** Appropriate for truly dynamic scripts,
modding requirements, or unresolved bring-up. Specialize its hot cases only after
identifying them. “No interpreters ever” can inflate ROM/code and prevent useful
features; “interpret everything forever” wastes the advantage of source access.

## Fidelity is a set of contracts

Treat these separately: gameplay state; event/tick sequence; collision predicates;
pose/attachment positions; draw ordering/material intent; sound timing/loops; and
pixel reproduction. A project can require exact state but accept a different DS
raster result. Another can demand exactness against its already-established DS
baseline. Neither contract is implied by “port.”

Without explicit approval, do not change simulation cadence, hitboxes, visible
content, effect count, animation timing, audio cues, or accepted precision merely
to hit a budget. Source-machine plumbing can change without such permission when
observable semantics are preserved. A quantized asset or simplified material is
not automatically an exact transformation.

## Avoid a permanent double runtime

A common failed shape retains the N64 asset graph, a compatibility graph, a DS
render graph, converted vertices, and staging copies simultaneously. Instead,
identify final target ownership early. Load/conversion data should be temporary
unless a live runtime consumer still needs it. Store a source identity for
provenance without retaining the full source representation on DS.

Likewise, do not keep a generated DS fast path and execute the complete source
renderer “for synchronization” every frame unless that remaining source work is
actually required. Extract the necessary events/state changes or prove a smaller
update function. Keep source execution on the host for comparison where practical.

## Minimum useful context

Usually a few facts in the existing task are enough: source revision/dialect;
behavior to preserve; source-to-target owner; hot versus setup work; live inputs;
worst-case storage; unsupported behavior. The optional
[context template](../templates/PORT_CONTEXT.md) is for projects that lack those
facts, not a new paperwork requirement for every code edit.
