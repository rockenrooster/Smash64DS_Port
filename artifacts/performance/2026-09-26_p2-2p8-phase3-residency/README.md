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
Overlay checkpoint **feaa37dc3a7** committed/pushed. Root's final combined
checker/pool fixture run passes 9/9; docs pass. No root/P1 ROM was rebuilt or
published. MF metadata edits remain a separate in-progress helper-owned batch.

## MF contract finding

Reuse 1,570/1,570 exact C decode evidence in the September23 MF-host receipt.
Production selection was absent; the family map in the experiment was unused.
The metadata producer is being extended to derive actual main-table users,
including Luigi/Mario and Purin/Kirby sharing, and to price retained table/index
storage and all 29 raw exceptions. Old ~599 KiB bank estimates are provisional
until complete admission metadata is priced. No production bank is linked yet.

MFP2 now records main-table users independently of owner/opponent masks and lists
all 29 raw O2R exceptions (27 AObj32, two splines). Roster selection preserves
mirror opponents by slot. The planner charges the retained index/table storage,
16-byte raw-block alignment and individually rounded raw asset allocations.
An unresolved non-NULL motion symbol fails admission instead of disappearing.

Root review also found that the corpus cache only keyed the BPS1 pack. It now
keys O2R sources, normalizer code, parsed motion-table/symbol metadata and the
verification mode, under `builds/`. This prevents stale borrower masks and clips
without invalidating the corpus for unrelated backend edits. Eight focused tests
pass, including cache changes with an unchanged BPS1 pack and missing mappings.

First full MFP2 producer/checker job **81758**: `mf_emit.py --out builds/p2-2p8-mf2`,
then `check_mf_pack.py .../ftanim_mf_pack.bin --json .../check.json`. Logs/exits:
`builds/p2-phase3-mf2-{emit,check}.{log,exit}`. This regenerates the corrected
production metadata/pack and rechecks its unchanged codec; it does not repeat
the rejected codec experiments. Source inputs frozen; no ROM/emulator active.
Job 81758 exited 0 for both producer and checker: 1,570/1,570 exact C decodes,
29 raw exceptions, 1,432,612 B pack / 64,848 B metadata. Expanded tables are
16,328 B on host, 16,260 B on ARM; retained backing blob is 16,928 B. The actual
manifest prices all 1,365 unordered four-slot rosters, including mirrors:
canonical bank 566,336 B, heavy probe bank 651,824 B, maximum 697,760 B
(Yoshi/Captain/Pikachu/Ness). These exclude existing fighter/scene heaps.

The next overlay slice moves ten more non-VS TUs and three small file companions,
priced from the linked map at 54,144 B. Keep the complete `sc1pgameboss` TU
resident: its wallpaper initializer is reachable for Last-stage ground through
a shared constructor, regardless of scene kind. Training's wallpaper call is
explicitly guarded by its non-VS scene. The existing ELF gate must still review
new exact crossing pairs before packaging. Kirby asset348 is KirbySpecial2,
10,512 B aligned; with 6,210 B free and 2 B padding the current miss is 4,304 B
for that file alone. Extra space is still needed to construct Kirby and retain
the runtime floor. Warm heavy relink log `builds/p2-phase3-overlay-heavy-build-r3.log`.
Extended relink session5081 linked but the packaging gate rejected58 unreviewed
crossings (exit2); no candidate ROM was produced. Review found additional shared
fighter-death/camera/boss closures in the 1P runtime TU. Their complete local
functions/constants stay resident, as do the whole bonus-stage code/files: its
ground/target calls are selected by stage kind, independently of scene kind.
The corrected relink is session66655, log
`builds/p2-phase3-overlay-heavy-build-r4.log`. Its remaining scene callback pairs
still require exact review before packaging/running. No emulator is active.

`mf2-{check,manifest,budgets,inputs}.json` and `mf2-rosters.csv` preserve the full
MF-host result and all roster budgets. Pack SHA-256 `1AA26F69790C09F6D6A68B748BF2F55D9920B3161955EEE382666AB7FB65D480`.
Both the MFT1 table blob and aligned stream-data range are byte-identical to the
prior proven MFP1 artifact; only metadata/selection changed. Full pack remains
in `builds/p2-2p8-mf2/`; it is not in a ROM or an admitted runtime bank.

Extended gate review kept the complete shared boss/death/camera function closure
and its own constants resident, while moving unrelated jump tables with their
owning scene functions. The checker also now normalizes ELF Thumb function
address bits before bounding source symbols; two fixtures cover a relocation
at entry and one immediately after the real function end. All 11 checker/pool
tests pass. The final fresh scan passes 155 exact reviewed pairs, zero unknowns.
Relink session55952 exited 0, native-only 319 inputs, overlay232,672 B; ROM
`9F7C46CD83B7C2352C1E05BE192432B93A08DA1D9DED413B95402DBAB77F720D`.
Next: same two-frame heavy probe, runner7, logs
`builds/p2-phase3-overlay-heavy-probe-r1.{log,exit,txt}`. Inputs frozen.
Probe session **89166** exited 0: all four fighters constructed and two initial
battle updates reached, no heap decline/external failure/overflow. Loan232,672 B,
used231,632 B in20 allocations, one load/no load failure. Main arena896,512 B,
immediate free12,556 B: **RESERVE_RED**, 13,044 B below the25,600 B floor before
any long match or motion bank. Pre-sample low-water remains invalid. This clears
the previous first-load OOM but does not qualify GO, pixels, timing or stability.
`overlay-wide-heavy-summary.json` and `overlay-wide-crossings.json` own the exact
proof; matched binaries/config are in `builds/p2p8-frontend-wide-9f7c46cd/`.
All jobs/helpers are terminal. Next: scoped memory reclamation to restore the
runtime reserve, then complete MF admission/bind and ARM7 audio. The full motion
bank remains an additional priced requirement, not funded by this startup pass.

## Owner-directed solo audit (2026-09-26)

Owner requested no further subagents and an independent audit of their work.
All four existing helpers are terminal. Root owns all further edits and checks.
Audit scope: MFP2 producer/selection/budget/checker; ELF overlay crossing checker
and lifetime claims; retained particle fixture and DamageFlyMDust producer/owner.
Already corrected by root: missing raw alignment, pack-only corpus cache key,
unmapped motion symbols silently omitted, Thumb source-range bounds, and shared
gameplay/frame callers incorrectly considered eligible for overlaying. Recheck
the retained implementations against source and hostile/boundary inputs; prior
helper PASS reports alone do not close this audit. Do not restart unrelated
performance experiments or publish the candidate while reserve remains RED.
Scoped audit completed in `solo-audit.md`: four reproducible false-PASS gaps
fixed in the pack/ELF checkers; 31 focused tests pass. Actual pack decodes and
the current ELF remain GREEN under the strengthened validators. Budget pricing
now consumes the checked binary with source-bound receipts; all roster figures
are unchanged. Target reserve/performance and remaining lifecycle proof stay RED/open.
Checkpoint staging initially encountered another empty Git index lock. Its
timestamp was over 33 minutes old, no Git process was active, and an exclusive
read handle succeeded. Root removed that exact lock under the owner's earlier
explicit approval, then retried scoped staging. The first failure was not a commit.
Audited implementation/evidence checkpoint **2c9511de5e2** is committed and
pushed. Owner's unrelated edits remain outside it. Current next action is solo
memory reclamation: restore the 13,044 B reserve shortfall and fund MF residency.
