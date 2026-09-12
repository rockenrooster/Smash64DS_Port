# Architecture

[PROJECT_GOAL.md](../PROJECT_GOAL.md) owns scope, fidelity and acceptance. This
file owns stable implementation boundaries, not completion status or a refactoring
queue. [P2_EXECUTION_BOARD.md](P2_EXECUTION_BOARD.md) owns current configurations,
budgets, blockers and deferrals.

## Design rule

**Original SSB64 behavior + the fastest correct Nintendo DS implementation.**

BattleShip specifies mechanics, timing, collision, rules, camera meaning and state
flow; it does not prescribe the DS representation. Keep competitive original code.
Generated, specialized, precomputed or manually rewritten implementations are equally
valid when mechanically equivalent. Generalize tooling, not necessarily the runtime.
The goal owns fidelity adaptations; there is no separate likeness percentage or
automatic approximation allowance here.

## Ownership and entry points

| Surface | Responsibility |
|---|---|
| `decomp/` | Read-only behavior and DS implementation references; see [DECOMP_MAP.md](DECOMP_MAP.md). |
| `src/import/` | Project-owned coherent BattleShip imports and narrow import adaptations. |
| `src/port/` | Source/platform integration, task scheduling, relocation and diagnostics. |
| `src/nds/` | DS presentation, input, audio, storage and hardware services. |
| `include/` | Required compatibility layouts, declarations and backend APIs. |
| `scripts/` | Asset producers, build integration, host checks and runtime instruments. |
| `assets/` | Source-derived payloads; some require local generation/extraction. |
| `builds/` | Build directories, generated overlays and non-published lab outputs. |

`src/nds/main.c` initializes platform/relocation/coroutine services and bridges
source boot into the DS loop. `src/nds/nds_renderer.c` is the renderer composition
file: its component includes locate assets, textures/effects, native owners,
fighter production and dispatch. Included `.c` components are not automatically
separate translation units; consult the Makefile before moving code or storage.

Change generated output through its producer. Keep the producer, required tracked
output and build dependency coherent. Preserve source provenance with imports;
do not patch read-only reference files to make this runtime compile.

## Product flow and scheduling

`smash64ds.nds` is the P2 user ROM. The VS shell connects title, menus, character
select, stage select, battle, Results and re-entry. Additional source scenes use
the same platform boundaries. Source presence, an enabled build flag and acceptance
are different states; active 1P development does not imply a qualified shipping flag.

`smash64ds-battle-playable-hwtri.nds` is the frozen P1 artifact, not an ordinary
iteration target. All other ROM outputs belong under `builds/`. Harness mode 163
names a retained battle capability, not the whole game or a renderer permission.

The task/coroutine seams host source-equivalent scheduling on ARM9: controller
state feeds scene/gameplay updates, object processes and display traversal; native
owners submit presentation, audio services run, and presentation is VBlank-paced.
Do not submit/present again in the outer loop when the scene already owns that
frame. At boundaries, retire the outgoing scene before reusing its live storage.

Preserve observable process/event order, countdown input lock, timer start, pause
and transition semantics. Logic ticks, presented frames and VBlanks are separate
quantities. Original 60 Hz simulation is desirable, not sacred; compensated rates
and independent pose/effect/presentation rates require the goal's behavioral proof.
Battle/menu presentation targets 30 Hz. Platform timers and interrupts are shared
resources; inspect their current owners before claiming them for another service.

## Native presentation

**Every built ROM is native-only, including bring-up, debug and profiling.**
Exclude N64 graphics interpreters, generic compatibility renderers and software
scene compositors from target inputs and linked binaries. Reference rendering
stays host-side. This is the required architecture, not a claim that all current
content has completed migration. Hardware triangles alone do not prove it.

Native CPU transforms, typed bindings and shared GX/BG/OAM kernels are valid.
Owners select source-derived programs from live topology, asset identity, detail
level, hidden parts and material state. Validate the complete required program
before partial GX mutation. An unsupported state needs a native implementation,
not an interpreter fallback, an opaque substitute or silent omission. Rejection
is containment; it is not completed rendering.

Preserve source-observable draw order, depth, winding and alpha behavior. A positive
submission count is not proof of visible coverage. The sticky first native-failure
record is a diagnostic, not an exhaustive inventory; zero failures cannot detect an
owner that falsely claims content while drawing nothing. [VERIFYING.md](VERIFYING.md)
owns positive engagement, sibling-state and source-comparison evidence.

High and low fighter detail remain reachable contracts, including state-specific
body/hidden-part changes. Do not treat a compact CSS preview as a complete battle
model. Source-mandated invisibility is valid; accidental missing parts are not.
Top-screen gameplay/menu presentation and bottom-screen battle information are
separate native surfaces. The HUD is game content, not just developer text.

## Assets, compact data and residency

Convert immutable assets offline and prepare scene-specific resources at explicit
loading boundaries. Native records must preserve the identities and state needed
by their consumers without retaining the N64 graphics interpreter representation.
Live resource bindings need bounded ownership and validated dimensions/formats;
the P1 texture manifest is not a universal P2 layout.

Compact fighter cores retain structural/state dependencies while native programs
own geometry. `scripts/fighters/generate_battle_core_packs.py` follows HIGH/LOW
closures and records Main externs for restoration. Dedicated ShieldPose fixups are
separate. Native root identity cells are not executable source display lists.
Source file IDs/offsets, packed offsets and native addresses are different domains;
relocation/binding must translate them explicitly and preserve pointer lifetime.

[P2-texture-residency.md](p2/P2-texture-residency.md) requires complete deterministic
admission before GO and stable handles for that epoch. Mandatory battle/motion/
texture demand reads and required texture create/upload/delete/evict/convert work
are zero after lock. Pre-admitted native material/palette animation remains valid.
Unplanned gameplay LRU/reload is not an over-capacity solution. This is a required
contract, not a claim that current streaming and output debt have been eliminated.

[P2-2-pack-estimator.md](p2/P2-2-pack-estimator.md) owns semantic cost/profile verdicts;
[P2-1c-vram-map.md](p2/P2-1c-vram-map.md) owns scene bank claims. Admission validates
all independent storage, view, palette, atlas and transient constraints before
publishing handles. A capacity-only result does not prove complete resident output.

Main RAM, chosen taskman arena/general heap, ITCM, DTCM, VRAM and asynchronous
buffer lifetimes are distinct budgets. A linked-byte saving is not automatically
an arena gain. Charge simultaneous setup/teardown overlap, metadata, alignment and
reserve, not only steady-state assets. No universal P1-era 128 KiB reserve applies
to every P2 scene. [RAM_RECOVERY_PLAN.md](RAM_RECOVERY_PLAN.md) owns the method and
scoped floor references. Avoid new per-frame general-heap dependencies.

## Audio, teardown and ABI

Source behavior determines music, voices and effect events. ARM9 selects resources
and services the DS audio backend, including BGM file reads/refill/cache maintenance;
ARM7 owns sample playback. BGM is a declared streaming client with reserved buffers
and service deadlines, not an exception for mandatory animation/texture demand or
all one-shot cues. Host muting does not disable guest audio verification.

A buffer remains live until its final CPU, DMA, GX or audio consumer finishes.
Cache maintenance belongs at the ownership handoff. Results/rematch and other
scene transitions must release or invalidate renderer, texture, relocation, audio
and arena state in dependency order, without stale references or double teardown.
[p2/RESULTS_OAM_DESIGN.md](p2/RESULTS_OAM_DESIGN.md) owns Results composition.
Observe re-entry for leaks; legitimate content-dependent peaks are not automatically
leaks, and one completed gameplay window does not prove transition safety.

Expose only the ABI needed by live imported paths. Preserve relevant enum values,
layouts, offsets and callback signatures; do not globally import N64 libc headers
into the devkitARM build. Fix shared defects at their owning seam rather than
adding per-caller offsets or duplicate state. Weak/compile-only stubs are not runtime
completion. Durable gaps belong in [KNOWN_ISSUES.md](KNOWN_ISSUES.md); archived
Runtime 1/2 plans are rationale, not current acceptance rules.
