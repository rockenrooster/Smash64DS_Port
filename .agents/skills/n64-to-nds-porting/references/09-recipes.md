# 09 — Fast code-porting recipes

These are compact integration recipes. Names in pseudocode are descriptive
placeholders, not libnds APIs. Actual target calls come from the installed SDK
and `nds-coding-practices`.

## A. A source function builds the same mesh list every frame

**Avoid:** allocating a temporary `Gfx` array, resolving source addresses,
decoding commands, converting every vertex, and setting every material again.

**Use:** a native immutable geometry block plus the actual live bindings.

```text
build: decode this source list/dialect and entry state
       preserve versioned vertices; classify material and transform cases
load:  resolve native asset/material handles and allocate bounded live bindings
frame: evaluate changing pose/material inputs once
       validate/choose the complete draw path before submission
       submit the native block through the existing renderer owner
```

Retain a runtime path for genuinely generated geometry. Do not assume an address-
stable source list is immutable. Test inherited state and partial vertex loads.

## B. A rare seam appears only between joints

Inspect the source vertex-cache history before modifying mesh topology. Find the
load operation that created each triangle corner and the matrix/light/UV state at
that point. Keep prior slots live across matrix changes. Detect mixed-transform
triangles and preserve each corner's history. Do not fix it by adding arbitrary
triangles, clearing the cache, or assigning all corners the last loaded matrix.
Use the [vertex-history fixture](../examples/vertex_history.json) as a small
regression pattern; it is not a reproduction of any particular game.

## C. N64 positions wrap after converting to DS vertices

Choose a model-local origin and scale from the complete geometry range, including
supported animation/procedural extremes. Pack checked native values and compensate
at the derived transform boundary. Reject overflow instead of narrowing blindly.
Use `port_s16_to_v16()` only under its explicit scale/rounding contract. Keep
collision/world units separate until a wider unit migration is proven.

## D. A float-heavy animation evaluator runs several times per frame

Separate constant channels, live pose evaluation, and event processing. Convert
source scripts/keys to compact direct native channels. Share the correct pose
between consumers, with explicit tick/space/dependency keys. Do not replace
collision's required pose with an older render sample or suppress crossed events.
Prebake only when fractional timing, blends, ROM, and working-set costs fit.

## E. A fighter/actor/item closure is too large

Identify retained runtime consumers and late-spawn roots. Build a typed cross-file
ownership graph, retaining unknown reachable objects conservatively. Emit a native
closure and a validated relocation map; release setup-only data only after all
consumers migrate. Check shared immutable unions, distinct mutable instances, and
transition overlap. Do not add on-demand paging before measuring the native pack.

## F. Selection UI loads whole source gameplay files

Export a separate compact preview bank/index. Load the selected preview with
bounded lookahead/caching appropriate to the actual storage path. Reject stale
loads by request generation. Prepare the gameplay pack only at its loading
boundary. Avoid many scattered tiny reads by arranging related metadata and
preview data for useful sequential access.

## G. A texture cache returns the wrong colors or silently stops drawing

Check identity beyond image pointer: palette/TLUT version, tile view, dimensions,
format, source generation, and dynamic material inputs. Account pixel bytes,
palette space, and table slots separately. Reject capacity exhaustion before draw
submission; never globally reset a cache containing live handles. Convert the
material's actual intensity/alpha meaning, not just its source format name.

## H. A source thread exists only to wait for render/audio completion

Replace the source service boundary with the target owner's completion/lifetime
contract. Preserve any deferred event order it encoded. Keep a supported native
worker when useful; do not rebuild libultra scheduling or busy-wait across CPUs.
Treat source cache and physical-address calls as things to reconsider, not macros
to rename one-for-one.

## I. A linked object list is hot

Retain source insertion, spawn/delete, callback, and RNG order. Bind stable
callbacks at state transitions; separate hot fields from cold source metadata.
Use stable dense indexes/active ordering where appropriate. Do not swap-remove
actors until order independence is proven. Optimize actual per-object work before
introducing a universal entity framework.

## J. A division or float helper appears in a hot loop

First move invariant computation to setup, cache the correct result, or reformulate
the work. Choose a bounded representation with sufficient intermediates. Inspect
emitted ARM code for the changed function and its callers. Retain required source
rounding; do not blindly replace multiply/divide with shifts for negative values.
The [numeric examples](../examples/n64_numeric.h) show explicit policies, not a
universal replacement for gameplay math.

## K. Porting timers changes the game's speed

Separate source tick units, real-time timer units, animation frame units, and
presentation count. Establish the canonical source rate/region. Use elapsed
pulses/time to schedule the same logical ticks, delivering each input edge once
at the correct source time. Keep backlog explicit. Test pause, slow frames,
repeated frames, and scene transitions. A multiply-by-two delta is not equivalent
to two source collision/state-machine updates.

## L. An N64 material has no obvious DS equivalent

Derive its actual combiner/alpha/depth/sampling inputs. Bake only invariant terms,
choose a specifically validated native recipe, or implement a dedicated path.
A two-pass approximation, opaque substitute, missing effect, or texture reduction
requires the project's approval; none is an implicit optimization license.
Reject an unsupported conversion with source provenance rather than emitting a
plausible but incorrect native material.
