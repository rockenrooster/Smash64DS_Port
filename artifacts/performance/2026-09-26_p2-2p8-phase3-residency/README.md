# P2-2p8 Phase 3: residency and memory (2026-09-26)

Status: IMPLEMENTING, not accepted. Phase2 default checkpoint `ab7bcec4729` is
pushed. Reuse its full-match/replay/CSS proofs; performance remains RED.
Current shipping capacity baseline is `1E3A5E9D` (all-content + source menu walk
and argmax roster only): arena 896,512 B, Link FPC request 33,520 B with 20,316 B
free, first battle frames zero. Full deficit remains unsized. Matched baseline
and detailed summary are linked by the preceding Phase2 receipt.

## First batch: lend front-end code memory to VS assets

Edit boundaries: a Calico front-end overlay linker fragment, native scene loan
owner, the existing source dispatch producer/scene lifecycle, and existing FPC,
native-image and secondary-reloc allocations. No source gameplay changes.
The scene's existing calloc arena remains unchanged: its allocator header and
preceding live allocations are never included in the loan. Mutable front-end
data/BSS stays resident. Only text/rodata move; nested 1P retains the overlay.
VS borrows it after the previous scene returns to the resident source dispatcher.
Every scene instance resets the asset pool, including Sudden Death; source heap
generations remain the authority for all existing resident-asset caches.

References inspected: installed Calico 1.2.0 `ovl.h` and its overlay linker/example;
`sm64-nds/src/nds/nds_overlay.c` and linker; `sm64ds-decomp/notes/overlay-residency.md`;
BattleShip scene dispatch, taskman and existing native asset allocation lifetimes.
The original no-op `syDmaLoadOverlay` is not repurposed: 1P invokes it before
changing scene identity, so using current scene there is unsafe.

First falsifiers: successful native-only link, exact resident-to-overlay relocation
audit, then the existing heavy-roster probe. No emulator until unknown crossings
are reviewed/removed. Same-image runtime must prove positive loan use, four fighter
construction, valid native resources and later menu reload. CSS capture/reserve and
full performance/lifecycle qualification remain owed after the input batch freezes.

Helpers: overlay/lifecycle audit read-only; ELF crossing checker owns only its new
script/test; MF metadata helper owns only motion scripts and MF passive structs.
Root owns shared integration, builds and timing. First heavy candidate uses warm
`build-p2p8-s7-fpwalk`, same flags as baseline, Yoster producer already canonical1.
Logs: `builds/p2-phase3-overlay-heavy-build.log` and matching `.exit`.
Active build session **6864**. Source/linker/import-producer inputs frozen;
MF header/scripts are disjoint and currently unlinked. No emulator is running.
Build 6864 exited 0, native-only 319 link inputs. First overlay ROM `3F3FA976`.
ELF contains `.ovl.frontend` at `0x022a4dc0`, size `0x2d6a0` (186,016 B),
ending exactly at the newlib
heap start `0x022d2460`. No runtime run yet; relocation checker is collecting
all resident crossings before reviewing the lifecycle allowlist.
Architecture check session 91845 exited 0. Actual linked source dispatcher calls
`ndsFrontendOverlayPrepareDispatch` before its switch (producer-generated line873;
linked BL at `0x0206a22a`). The focused actual-C pool fixture passes (one test):
alignment/bounds/overflow, inactive loan, scene exit, same-kind re-entry and menu
reload. It mocks only the linker region and DS services; target code/callback
lifetime and output remain unproved. The capacity probe now reports optional
overlay counters from this candidate without changing old-ROM behavior.

The initial reference audit found 116 pairs. Shared frame/input/sprite callers
could reach three front-end functions even during battle; these functions and
their constant dependencies now stay resident. Guard relink `6DE96494` removed
all four unsafe caller pairs, leaving UI assertion strings to move with their
owner. The remaining 112 pairs are individually recorded with lifecycle reasons
in `scripts/frontend_overlay_allowlist.json`: source dispatch, menu task tables,
retained campaign calls, source-menu pump, guarded Results getter, pointer-only
opening checks and linker-boundary references. A second read-only audit agrees.
Direct non-VS diagnostic entries bypassing the source dispatcher are not covered.

The first checker implementation performed repeated full-symbol scans; three
slow owned processes were terminated before the indexed implementation was run.
The completed scan takes ~0.9 s; all eight synthetic parser/policy tests pass.
The source-matched pool test passes separately. The checker now gates ROM
packaging. Linker regions were factored, with unchanged addresses/sizes, into
`linker/nds_memory.ld` so both selector scripts see declarations before use.
The guarded final relink is session **5333**, log
`builds/p2-phase3-overlay-heavy-build-r2.log`. No runtime proof yet.
Relink 5333 exited 0; native-only 319 inputs and ELF crossing gate GREEN:
184,128 B overlay, 112 reviewed pairs, zero unknowns. ROM
`39790FA258E8DB84DAA6F123B39058E7119CA95AB845B6E97863347D8DDFA483`,
ELF `710A7864C5E581652B89F1C2D7E7585162776668E1EAD686F1578280FE31A1D7`.
Config is byte-identical to heavy baseline1E3A5E9D. First runtime falsifier:
existing four-kind probe, two battle frames, no allocation tracing; runner7,
`builds/p2-phase3-overlay-heavy-probe.{log,exit,txt}`. Inputs frozen.
Probe session **24598** exited 0 with a complete failure dump. The target
verdict remains **CAPACITY_RED**: overlay loads 1/fails 0, loan 184,128 B, used 184,112 B
in 15 allocations. Captain, Link and Pikachu are constructed; Kirby's external
asset 348 is declined with 6,210 B in the main arena, then pack halt 14. First
battle frames 0. This is a measured memory-enabling result, not playable-roster
or performance acceptance. The UINT_MAX pre-frame low-water is invalid, and the
reported final external-failure counter 0 is not a clean result: the breakpoint
and first-asset latch both identify 348. No runtime process remains active.
`overlay-heavy-summary.json` preserves exact identities, markers and raw-log hash;
`overlay-crossings.json` is the successful final ELF gate. Matched ROM/ELF/config
are retained in `builds/p2p8-frontend-overlay-39790fa2/`.

Next A7 work: price additional non-VS scene code for the same exclusive overlay.
The measured 184 KiB loan is much smaller than the original front-end estimate;
do not treat that estimated supply as available RAM for MF. Natural-input CSS,
menu reload, all four fighters/GO and final integrated performance remain owed.

## MF contract finding

Reuse 1,570/1,570 exact C decode evidence in the September23 MF-host receipt.
Production selection was absent; the family map in the experiment was unused.
The metadata producer is being extended to derive actual main-table users,
including Luigi/Mario and Purin/Kirby sharing, and to price retained table/index
storage and all 29 raw exceptions. Old ~599 KiB bank estimates are provisional
until complete admission metadata is priced. No production bank is linked yet.
