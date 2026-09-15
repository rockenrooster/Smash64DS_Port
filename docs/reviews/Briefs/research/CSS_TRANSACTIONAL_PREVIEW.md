# CSS: bounded preview loading, publication and audio continuity

Applies to issues 05–11 and, after separate scene integration, the 1P preview.
Sources: R41/R42/R48/R102/R107 in SOURCE_LEDGER.md; S10 for already-landed repairs.

## Current code versus proposed change

The current acquire path already has an action budget. Its compact branch still
performs file setup and native-owner preparation while BGM is explicitly
suspended. A one-action budget is not a bound on bytes, relocations, CPU ticks or
filesystem latency. Adding a second action counter or deleting audio suspension
is not the proposed fix. R03 only restores release-before-reset order at three
remaining seams.

The raw YoshiModel shared pin collision, owner-image allocation into the wrong
arena, and CSS particle-bank reinitialization have existing repairs/evidence.
Keep them. Do not revert to twelve eagerly resident full fighter closures.

## Concrete runtime shape to implement

Extend the existing resident block and acquire cursor; do not invent a global
async job framework. The compact loader needs an incremental equivalent of its
current whole-pack work. Names below are a **proposed state contract**, not APIs
that currently exist:

```c
/* Design sketch; translate into the existing resident block/cursor ABI. */
enum PreviewPhase {
    PV_HEADER, PV_SECTIONS, PV_FIXUPS, PV_NATIVE_IMAGES,
    PV_POSE_PREPARE, PV_READY_TO_PUBLISH, PV_CANCEL
};
/* Required cursor state:
   request_generation, requested_kind, phase, section_index, byte_offset,
   validated_source_ranges, owner_image_index, work_budget, error_identity.
   No published fighter pointer may reference an unfinished cursor. */
```

The owning update performs this transaction:

```text
coalesce the latest hover request; retain the current valid visible identity
if the desired identity is already resident: acquire its reference and publish
otherwise reserve an existing legal block (never evict a referenced block)
validate header, section sizes, relocation ranges and total admitted capacity
read a bounded chunk; leave the filesystem critical section
service audio if due; do not reenter a locked filesystem callback
relocate a bounded set of records; keep registrations private to the job
load/validate native images in bounded chunks in the same owning block
prepare only the required source preview pose/material set
validate complete source kind/detail/program/generation correspondence
publish the complete fighter + image + pose identity at a safe frame boundary
retire the prior unreferenced identity only after outstanding GX/DMA use ends
```

Before any block reset, invalidate native image/table bindings and reloc
registrations whose storage belongs to that block. Respect current prepared-dense
cache and animation-cache lifetimes; source data, native images and shared caches
are not automatically one lifetime. On cancellation, stop at a safe bounded
boundary, retire only that job's registrations/ownership and never overwrite the
newer requested identity. Failure must remain observable and unaccepted.

## Audio requirements

First distinguish a deliberate pause from a new track start or seek. Capture the
BGM suspend/resume counters, active sequence ID, stream position, start/seek/stop
call reasons, buffered samples and underruns. The inspected implementation comment
says resume continues rather than restarts; verify the actual code and signal.

Choose chunk byte/node/tick limits from measured worst-case storage latency and
audio service margin. A fixed byte count alone is insufficient on a slow seek.
Prebuffer BGM before each potentially blocking slice and service it outside the
filesystem lock. Only after this bound is demonstrated should the CSS caller stop
suspending BGM. Do not run arbitrary filesystem work from an interrupt to hide the
pause. Do not promise absolute zero hover I/O unless resident-set evidence proves it.

## Gate flashing and hover latency are separate acceptance dimensions

Keep input, doors, current preview and audio alive while a miss is pending. The
presentation commit must not show a new owner with old pose/material tables, or
old geometry with a reclaimed texture/palette. Check VRAM/BG/OBJ bank handoffs,
GX/DMA completion and per-frame publication order. If a fully resident gate-only
run still flashes, repair that renderer/bank transition independently.

Timestamp request, dwell expiry, acquisition start/end, image preparation and
first correct visible frame. Optimize the measured longest component. Cache hits
must bypass miss preparation. Rapid cursor reversals must cancel/coalesce safely,
and settling on a final cell must eventually finish rather than being starved.

## Required proof

Use repeated cold/warm tours, rapid reversals, two slots requesting the same kind,
selected/idle transitions, low/high detail where reachable, Results->CSS reentry
and cancellation at every phase. Check bounded allocator high-water, no stale
registration/image tags, positive correct preview pixels, unchanged selection
behavior, continuous recorded BGM and natural 30 Hz/two-VBlank menu pacing.
An ordinary loop guard is not proof of the new slice's time bound.
