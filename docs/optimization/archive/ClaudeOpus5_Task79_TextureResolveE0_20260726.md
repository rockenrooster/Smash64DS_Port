# Task 79 E0 — 143,434 ticks rebuilding a 236-byte key for state that is already compiled

**Date:** 2026-07-26
**Status:** E0 complete, **GO**. The lever is real, the mechanism is identified,
and the replacement data already exists in the generated include. No runtime
change yet.
**Input:** `artifacts/task78-anim-census/` — 128 settled frames (439–567), the
same window and ROM configuration as every measurement since Task 69.

## 1. The family

143,434 ticks/frame, 9.5% of the 1,515,768 ticks of work, across 30 symbols
(vertex-emit functions excluded — see Task 78 E0 §7).

| ticks/frame | cyc/inst | symbol |
|---|---|---|
| 42,420 | 6.57 | `ndsRendererHardwareResolveOrBindTexture` |
| 17,489 | 4.53 | `ndsRendererSyncTextureTile` |
| 12,766 | 3.98 | `ndsRendererAdapterBuildNativeMaterialSnapshot` |
| 7,883 | 5.79 | `ndsRendererNativeApplyMaterial` |
| 7,461 | **10.81** | `ndsRendererHardwareFindTexture` |
| 6,350 | 8.61 | `ndsRendererHardwareTextureKeyHash` |
| 5,853 | 4.39 | `ndsRendererHardwareBindTextureName` |
| 5,538 | 8.87 | `glBindTexture` |
| 5,346 | 7.75 | `ndsRendererCaptureTextureLoad` |
| 5,271 | 3.38 | `ndsRendererRecordSetTile` |
| | | …20 more, 27,057 combined |

## 2. This family is stall, not arithmetic

The right-hand column is the finding. Cycles per instruction across this family
runs **4.0 to 10.8**. For comparison, the vertex-emit functions next door —
`ndsRendererNativeEmitProductionRawUntexturedRun` and its textured twin — run at
**2.46–2.59**, close to the ARM9's issue rate.

`ndsRendererHardwareFindTexture` at 10.81 cycles per instruction is executing
roughly four cycles of memory stall for every cycle of work. That is not a
function that needs a better algorithm; it is a function that is walking memory
it should not be touching.

This is Task 65's 62%-stall finding localised to a specific subsystem, and it
sets the shape of the fix: **make the lookup not happen**, rather than make it
cheaper. Which is what `COMPILER_FIRST_ARCHITECTURE.md` already says for this
task — "the goal is to avoid calling the resolver, not make the resolver
slightly faster" — now with a measurement behind it.

## 3. Why it stalls: a 236-byte key, rebuilt and re-compared every bind

`NDSRendererHardwareTextureKey` is **236 bytes** (59 words). The lookup path is:

1. `ndsRendererHardwareResolveOrBindTexture` declares ~60 locals and reconstructs
   the whole key from traversal state on every call — image pointers, TLUT,
   width/height/format/size/flags, both load and tile rectangles for texel0 and
   texel1, prim LOD fraction, and both combine words.
2. `ndsRendererHardwareTextureKeyHash` mixes a *subset* of those fields into a
   fingerprint. The comment on it is explicit that this is deliberate: *"This is
   an index fingerprint, not the equality oracle… Hashing all 59 words cost more
   ARM9 time than the open-address lookup saved."*
3. `ndsRendererHardwareTextureKeyEqual` then does `memcmp(a, b, 236)` on **every
   candidate hit**, because the hash is only a fingerprint.

So a cache *hit* — the common case — costs a full key reconstruction, a hash, an
open-address probe, and a 236-byte `memcmp`. The frame's `memcmp` total is 19,400
ticks and this is where most of it goes.

The existing design is not careless: it was tuned, and the comment records the
experiment that chose a partial hash over a full one. It is a well-optimised
answer to the wrong question. Both the hash and the comparison exist only because
the caller arrives with no identity — and the caller *does* have identity, it
just throws it away.

## 4. The identity already exists, compiled

Task 77 E0 found in `src/nds/nds_native_fighter_owner.generated.inc`:

- `sNdsNativeFighterStateDeltas[70]` — material and polygon state transitions
- `sNdsNativeFighterStateSequence[196]` — the order they occur in
- `sNdsNativeFighterEpochDirectPolicy[49]` — per-epoch submission policy
- `sNdsNativeFighterRuns[67]` — runs, each inside an epoch that already owns
  material/texture/polygon state

The generator has already proven, at build time, which run uses which material
and texture. Every field the runtime reconstructs into that 236-byte key is a
compile-time constant for a given run, with the exception of genuinely animated
state (Dream Land water is the known case, and it is frozen at source frame 0 by
the project contract).

`ndsRendererNativeApplyStateDelta` at 20,280 ticks/frame shows the prepared
tables *are* partly consumed — this is not a subsystem that ignores its
generated data. But `ResolveOrBindTexture` at 42,420 alongside it shows the
identity is being re-derived after the fact anyway.

## 5. E1 direction

Carry the resolution outcome in the generated data rather than recomputing it:
per run, a resolved cache-slot index plus a generation stamp. On a match — slot
still holds the same asset, generation unchanged — skip key construction, hash,
probe and `memcmp` entirely and bind directly. On a miss, fall through to
today's path unchanged, which keeps the existing code as the oracle exactly as
the plan's A/B structure requires.

Available if that lands cleanly: the same treatment for the stage, whose runs
carry `texture_epoch` in the Task 57 IR already.

### Sizing

The full 143,434 is not available — uploads, genuine misses, and the first bind
of each texture must still happen. The avoidable part is key construction, hash,
probe and comparison on hits: `TextureKeyHash` (6,350), the `memcmp` share of
19,400, plus the majority of `ResolveOrBindTexture`'s 42,420 that precedes the
bind. A reasonable expectation is **40,000–70,000 ticks/frame**, which is 5–9×
the ±8,000 placement noise floor and comfortably measurable.

That is a smaller claim than the plan's ≥100K for this task, and it is stated
before the work rather than after — Task 78 E0 is the reason.

### Gate

No fidelity budget applies. A texture binding is the correct one or it is a bug;
there is nothing to approximate and nothing to put in front of the owner. The
gate is `WORK-H` P95 on the standard 128-frame window plus the Boundary
verifier, with the GX differ available if any binding is suspected of changing.
