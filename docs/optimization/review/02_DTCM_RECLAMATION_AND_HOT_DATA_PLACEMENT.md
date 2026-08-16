# Campaign 02 — DTCM Reclamation + Hot-Data Placement

> **Planning baseline:** `codex/r2-runtime2` at `a63dd0e4b3af9c6601713f70c179c96c0aa67735` (2026-08-16).
>
> If HEAD has moved when implementation begins, re-run the inventory/measurement steps first and update symbol names/line references rather than blindly applying this document.
>
> **Campaign rule:** optimize toward a DS-native architecture and four-fighter headroom. The current two-fighter P95 gate is a checkpoint, not the architectural finish line. Never bank projected savings; measure the shipping configuration. Prefer same-binary route A/B when practical because this tree is placement-sensitive.

## Objective

Turn DTCM into a deliberately managed ARM9 hot-data region rather than a side effect of object compilation. Remove dead/diagnostic occupants, decouple ARM-mode objects from automatic DTCM placement, and place only compact, CPU-hot mutable structures there.

Priority candidates include the fighter draw memo, compact fighter animation state, and small renderer/fighter state touched repeatedly each frame.

## Current layout constraint

`linker/nds_hot_text.ld` defines nominal DTCM at `0x02ff0000` with length `0x3e80`, but the entire nominal region is **not safely available for data**.

The repo measured the boot stack's deepest reach at `0x02ff3340` and currently asserts:

`__dtcm_bss_end <= 0x02ff3000`

So the presently qualified data ceiling is **12 KiB** (`0x02ff0000..0x02ff2fff`). Do not raise it without repeating the stack-low-water experiment.

## Current repo anchors

- `linker/nds_hot_text.ld`
- `scripts/check-task20-dtcm-layout.ps1`
- `scripts/census-dcache-working-set.py`
- `scripts/compare-elf-sections.py`
- `src/port/reloc_backend_renderer_dl.c`
- `src/port/reloc_backend_fighter_model.c`
- `src/nds/nds_ftanim_track.c`
- Campaign 04 memo: **2,924 B bss** measured by `nm -S` (`DRAW_MEMO.md` §4)
- Campaign 03 compact animation state
- Campaign 11 native-renderer dynamic state

## Hard constraints

1. Never exceed `0x02ff3000` under the current stack proof.
2. DTCM is for **ARM9 CPU-local hot state**. Do not place DMA/GX source buffers there unless hardware visibility/coherency is explicitly proven.
3. Do not waste DTCM on large immutable asset tables merely because they are frequently read.
4. ARM mode must not automatically imply DTCM placement.
5. Measure structures individually before final packing.

## Phase 0 — Byte-accurate ownership report

For every DTCM/DTCM-BSS symbol list:

- address/size/alignment;
- initialized vs BSS;
- read/write hotness;
- active frames;
- subsystem;
- CPU-only vs shared/hardware-facing;
- explicit placement vs `*.32.o` wildcard;
- diagnostic/lab status.

Report used bytes, free bytes to `0x02ff3000`, padding waste, and largest contiguous free block.

## Phase 1 — Reclaim accidental/dead occupants

Remove in this order:

1. lab-only counters/diagnostic arrays from shipping;
2. obsolete experiment state;
3. cold globals present only because their object is `*.32.o`;
4. large low-frequency buffers with no measured DTCM benefit.

Measure the clean reclaimed layout before adding replacements.

## Phase 2 — Decouple ISA from data placement

**Campaign 09 Phase 1 owns this work item** (one decoupling covering both TCMs,
including the shipping check that reports/blocks accidental `*.32.o`
placement). This campaign consumes the result:

- ARM/Thumb controls instruction encoding;
- explicit sections control DTCM;
- a small ARM kernel may remain in `.main` without its globals entering DTCM;
- hot DTCM state can serve Thumb or ARM consumers.

## Phase 3 — Rank candidate structures

Rank by **hot CPU accesses avoided per DTCM byte**.

### Candidate A — Fighter draw memo (~2.9 KB)

Only after Campaign 04 removes the 1,280 B/frame local copy and stabilizes invalidation. If the renderer directly dereferences the memo entry repeatedly, DTCM may be valuable.

### Candidate B — Compact fighter-animation mutable state

Campaign 03 should replace linked `AObj` state with compact contiguous per-fighter state. If repeatedly traversed and only a few KB, it is a strong candidate.

### Candidate C — Native renderer per-fighter dynamic state

Campaign 11 should leave a small mutable block containing matrices/visibility/colors/material/binding epochs. Keep immutable draw programs in normal memory; test only the mutable block in DTCM.

### Candidate D — Other compact fighter/collision state

Only after the D-cache census proves repeated CPU access and no hardware transfer dependency.

## Phase 4 — Make layouts DTCM-friendly before placement

For every candidate:

- split hot/cold fields;
- pack frequently co-accessed fields;
- remove diagnostics;
- prefer fixed arrays/indices over linked nodes;
- align only as required;
- keep rarely touched metadata in main RAM.

Shrink the state before consuming TCM.

## Phase 5 — Same-binary location A/B

Where practical, maintain identical main-RAM and DTCM copies temporarily and route all consumers through one base pointer chosen before GO.

Requirements:

- same logical contents;
- no per-access branch;
- no duplicated update work;
- identical outputs/state.

A move that requires synchronizing two permanent copies is a failure.

## Phase 6 — Final packing

After individual winners:

1. sort by measured benefit/byte;
2. account for alignment;
3. retain safety headroom below `0x02ff3000`;
4. test the combined pack;
5. rerun D-cache and whole-frame timing.

Add post-link assertions for expected residents.

## Verification

- `scripts/check-task20-dtcm-layout.ps1`;
- boot and scene-transition checks;
- one-minute match;
- state hashes where relevant;
- renderer parity for draw state;
- stack-low-water invariant;
- no DMA/GX pointer into unsupported DTCM;
- corrected P50/P95 and owner movement;
- any banked gate claim reports the 2/3/4/5+ VBlank-interval histogram and max
  interval (AGENTS.md device-report law).

## Completion criteria

1. No diagnostic/accidental shipping occupants remain.
2. ARM mode no longer implies DTCM ownership.
3. Each resident has measured benefit/byte justification.
4. Compact renderer/fighter candidates are tested.
5. The qualified stack ceiling remains intact.
6. The combined final layout beats the clean reclaimed baseline.
