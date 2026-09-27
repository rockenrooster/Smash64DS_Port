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

## Continue: scene-owned relocation workspaces

Previous turn was **progress**: audited checkpoint250665eec56/2c9511de5e2 is intact,
all jobs terminal, only unrelated owner edits remain. No completed probe or codec
experiment is restarted. Current `9F7C46CD` is preserved as the capacity baseline.

The linked frontend TUs retain66,195 B of data/BSS. Of that,58 source LBFileNode
status/force-status arrays total18,672 B. Each is installed by its own scene's
`lbRelocInitSetup` with both active counts reset to zero; these are workspace,
not persistent menu/campaign state. Move exactly those arrays into Calico's
overlay BSS, and include that BSS in the exclusive VS asset loan. The SDK loader
clears it on reload. The resident dispatcher retires old workspace pointers before
loading/lending the range; the next scene installs its own buffers normally.

The ELF gate now prices both file-backed and total runtime overlay bytes and
rejects a gap/overlap between code and BSS. Fifteen focused checker/loan tests pass.
First target falsifier is the same heavy roster/config; warm build
`build-p2p8-s7-fpwalk`, producer stamp remains canonicalYoster1. Log/exit
`builds/p2-phase3-status-bss-heavy-build.{log,exit}`. Source inputs frozen.
After capacity, natural CSS reserve/captures and scene reload still need proof;
MF's697,760 B worst bank remains a separate unfunded requirement.
Build session88898 exited0, native-only319 and ELF crossing gate GREEN. ROM
`B96BEF342906263CD950192F198001F435A3A8977385EFF88B0DE710F4527179`.
The ROM's actual overlay table agrees with the ELF:232,672 B loaded code/data,
18,688 B zero-filled BSS including alignment, total251,360 B loan. No new
resident crossing exemptions were needed. Run the existing two-update heavy
probe on this exact ROM, runner7, `builds/p2-phase3-status-bss-heavy-probe.*`.
Probe session8433 exited0: all four fighters and two updates, no allocator/reloc
failures. Main arena896,512 B, free31,324 B (+18,768 versus9F7C46CD), startup
reserve margin5,724 B. Loan251,360 B, used250,384 B in18 allocations. This is
startup capacity only; GO/full-match/lifecycle/native visuals remain unproved.
Next frozen batch check is the actual natural-input all-content twin in warm
`build-p2p8-s7-freeplay`; preserved8062C536 CSS baseline is unchanged. Log/exit
`builds/p2-phase3-status-bss-natural-build.{log,exit}`. No emulator active.
Natural build46401 exited0: ROM `3ADAE9979183FD9A4B82E9610856B6516902B009D0EE5F14BFA3787C27FE9D19`,
native-only319 and ELF gate GREEN. Config differs from heavy only by the two
expected walk/roster flags. CSS probe68996 exited0: reserve200,656 B, failure0,
margin17,584 B. Link/Yoshi/Pikachu400x280 crops are pixel-identical to8062C536.
Dated captures are in `artifacts/visibility/2026-09-26_p2-2p8-status-bss/`.

Extend the existing heavy probe with `-ThroughResults` for the missing reload
coverage. It records the first battle update, then disables that breakpoint so
the source match runs uninterrupted until its natural exit; it records end-of-
match low-water before the next scene resets it and stops after three Results
updates. It performs no guest state writes. This is lifecycle/resource evidence,
not timing acceptance. Use B96BEF34, runner7, logs
`builds/p2-phase3-status-bss-results-probe.{log,exit,txt}`; all runtime inputs frozen.
Lifecycle probe50120 exited0: natural exit after2,043 presents, second overlay
load succeeds, three Results updates reached, no allocator/reloc failures. Match
low-water14,816 B remains **RESERVE_RED** (10,784 B short); startup-only margin
did not cover16,508 B of subsequent allocations. Results free41,976 B. This is
the new resource constraint; no separate timing acceptance is inferred.

Next add13,829 B of immutable frontend tables from the linked map: Characters
motion descriptors, Sound Test cue IDs, and credits name/job/company text/metadata.
Source readers copy/read these tables. StaffRoleCharacters remains resident:
`scStaffrollSetTextQuetions` mutates it based on unlocks. Persistent selections
and other mutable state remain resident. No runtime inputs or gameplay code are
changed. Frozen heavy relink: `builds/p2-phase3-frontend-tables-heavy-build.*`.
Qualify the complete match/reload directly; no repeated startup-only ladder.
Build65940 exited0, native-only319, no new crossing exemptions. ROM
`FB299B25B21B0A70503BC2A8F31453441398B1F131577800EB289891661BEB97`,
saved with ELF/config/gate in `builds/p2p8-frontend-tables-fb299b25/`. Overlay
code/data246,528 B + BSS18,688 B =265,216 B. Active full-match/reload probe
session32477; log `builds/p2-phase3-frontend-tables-results-probe.log`.

Evidence limitation: B96BEF34's intermediate heavy binary/ELF was overwritten
by the next relink before preservation. Its original log/hash/markers are kept
in the status-bss summaries, but it is not a reusable binary baseline. The
matched9F7C46CD baseline remains saved; natural3ADAE997 is now saved in
`builds/p2p8-status-bss-natural-3adae997/`. Preserve FB299B25 before future reuse.
Probe32477 exited0: first-update free46,092 B; natural exit after2,043 presents;
end/low-water9,672 B; Results reload passes. This remains **RESERVE_RED** and
does not establish an end-of-match gain against B96BEF34's different workload.

Specific invalidator for one repeat:36,420 B of runtime heap growth needed an
owner, not a guess. The added `-TraceRuntimeAllocations` observer records live
allocation size/alignment/caller LR only during the match. Probe92984 exited0,
reproducing the same2,043 presents/9,672 B low-water. It records205 allocations
totalling36,300 B (120 B remainder is alignment):16,188 B from two Kirby copy-hat
buffers,4,216 B thread stack, and the remainder source GObj/DObj/MObj/AObj/XObj/
SObj/process growth. Kirby hats are currently loaded/read on demand after GO;
required content and its full admission footprint remain unfunded. Native owner
images include writable prepared-dense data, so sharing their buffers blindly
would be incorrect. Follow that producer-to-consumer residency seam next.

All30 moved lookup tables are byte-identical between the saved9F7C46CD ELF and
FB299B25:13,829 B verified by symbol, recorded in `frontend-table-bytes.json`.
No new ELF exemptions were needed. The natural twin's final relink is session
37478, log `builds/p2-phase3-frontend-tables-natural-build.log`; no emulator active.
Natural build37478 exited0, native-only319 and ELF gate GREEN. ROM
`7E0B1C7F995C0F8564D7B419A350D1049FCFF0DD98E6558FCFDC029B7AA60F1B`, saved
with ELF/config/gate in `builds/p2p8-frontend-tables-natural-7e0b1c7f/`.
CSS probe63034 exited0: free200,656 B, reserve margin17,584 B, no reservation
failure. All three400x280 captures remain pixel-identical to8062C536; dated
copies/comparisons are in `artifacts/visibility/2026-09-26_p2-2p8-frontend-tables/`.

The ordered allocation trace independently reproduces the exact36,420 B heap
growth:36,300 B requested plus120 B alignment. `frontend-memory-summary.json`
and `frontend-runtime-allocations.json` own the final identities and resource
evidence. New reclaimable range versus9F7C46CD is32,544 B; no FPS/cadence gain is
claimed. All jobs are terminal,33 focused tests pass, no root/P1 ROM published.
KEEP/IMPLEMENTED_NOT_ACCEPTED: full-match reserve is15,928 B short. Next is
source-faithful Kirby hat residency/admission, preserving mutable per-consumer
prepared data, then the complete motion bank and ARM7 audio. Do not treat the
startup-only margin or natural match completion as performance acceptance.

## Continue: complete VS Kirby hat admission

The previous checkpoint is pushed as5d0325d2eff; no old job is live. Root
continues alone. The source's Catch selects the donor's copy_id, then CopyInit
selects that row's modelpart. The reachable VS set is the union of the active
roster; a second Kirby can transfer a power from that same set. Both normal VS
and Sudden Death start with the source default descriptor's empty Kirby power.
High images are always needed; low images are needed when the source battle
player count is at least three. Prepare after fighter setup and before BGM/GO.

The new bank gives each image one table identity, shared between Kirby slots.
This matches the renderer's mutable prepared-dense/UV memo ownership. Bind-only
copies cannot read storage or allocate in VS; non-VS preview working buffers
retain their existing bounded lifetime. Missing admission is a named failure,
never a branch that silently removes Kirby's gameplay power. The existing copy
failure branch did clear copy_id; that defect is removed in this batch.

Raw heavy-roster images total37,640 B before metadata/alignment. Reusing the
existing LZ10 codec prices23,100 B plus16,188 B decode workspaces, worse than
raw, so that one experiment is rejected. All22 codec roundtrips passed in
`builds/p2-phase3-hat-lz-pricing.json`; do not rerun it. Shipping memory is still
unfunded. First checks are actual-C host admission/bind failures, shared table
identity, generation invalidation and all20,736 ordered normal rosters. The
canonical four-CPU lab has enough headroom to test the mechanism; its evidence
will not prove shipping admission or any final performance acceptance.

The first focused run passes15 tests: actual-C bank/roster/loan fixtures plus
prepared-dense and native-image ABI tests. Two older fixtures were stale (the
alternate Mario/Fox skeleton image binds and the scratch/loan allocator mocks)
and now cover the current seams; no ABI guard or negative control was removed.
The savedF5CA9FE7 ROM/ELF/config remains the canonical baseline. Build the warm
`build-p2-fourcpu-tickhud` after its Yoster-off particle preflight; log/exit
`builds/p2-phase3-hat-bank-build.{log,exit}`. Inputs frozen for this build.
Build92510 exited2: startup.h needs the source sb32 type before its allocator
declaration. Reordered the new include after scene/taskman headers; no producer
or flag changed. Retry log `builds/p2-phase3-hat-bank-build-r1.{log,exit}`.
Build70032 compiled/linked but exited2 at the ELF gate: the first post-overlay
four-CPU build exposes two shell-off taskman crossings not present in the
shipping configuration. Root traced the Title-only bounded update branch and
the Results-only inlined recorder; both run after dispatcher reload and cannot
be entered from a VS frame. Added exactly these reviewed pairs, retaining the
strict checker. Resume packaging unchanged ELF in `hat-bank-build-r2`.
Build90889 exited0: ROM `D675C034D70833EBA6A006BEDD19D642A2D21BD018EABBA9B31BA504822A45BC`,
native-only248 and ELF crossing gate pass. Saved ROM/ELF/config/map/check in
`builds/p2p8-hat-bank-d675c034/`. Compared with F5CA9FE7: config byte-identical;
all404 NitroFS files byte-identical, total29,066,719 B. ARM hat record164 B.
`hat-bank-inputs.json` records identities. Run the natural four-CPU stress arm
without pokes, runner4; log/exit `builds/p2-phase3-hat-bank-run.{log,exit}`,
sidecars `hat-bank-*`. This new executable requires its own timing/replay proof;
F5CA9FE7 remains the baseline. No build/producer runs alongside the emulator.
Run11976 exited0;1972 gameplay digest pairs are identical toF5CA9FE7. Native
failure0, general low-water122,412 B, arena1,281,792 B. Required masks0x510,
bank38,656 B, six images loaded, admission1, failures0. **Copy UNENGAGED**:
resident binds and hat draws are0. This proves admission and unchanged canonical
gameplay, not the bind/draw path. WORK-H P50/P95 1,352,128/2,601,664 ticks;
18.89 FPS from meanALL1,773,705. VBI2/3/4/5+267/1347/214/145, max10, total1973;
13.53% two-VBlank, performance RED. No per-hat speed gain is claimed.

Next missing question is all-content admission, not another unchanged match.
Reuse warm `build-p2p8-s7-fpwalk`, preservedFB299B25 baseline, flags menu-walk1
and argmax-roster1, canonicalYoster1 via producer. Build log/exit
`builds/p2-phase3-hat-heavy-build.{log,exit}`. The short existing heavy probe
now stops at the named hat admission failure with required bytes and headroom;
no gameplay writes or timeout-based inference. Positive copies, mirror slots,
Sudden Death/reload, pixels and final natural shipping qualification stay owed.
Heavy build89610 exited0: ROM `9E4A666D5A5C325D04835EDE9826CADCE047E61D946BAD38EF8A3C4A561EA497`,
native-only320, ELF gate pass. Saved binary/ELF/config/map/check in
`builds/p2p8-hat-heavy-9e4a666d/`; identity `hat-heavy-inputs.json`. Probe16473
is the two-update admission check, runner7, log/exit
`builds/p2-phase3-hat-heavy-probe.{log,exit,txt}`. Its fatal breakpoint resolves
to the halt loop after DC_FlushAll (verified in ARM disassembly), so resource
witnesses describe current RAM. ROM/source/producer inputs remain frozen.
Probe16473 captured the expected **FAILED_ADMISSION**, not a match pass (the
probe's exit0 denotes successful capture). All four fighters are created; before
BGM, reason4, masks0x640, bank38,672 B, free41,996 B, alignment12 B. Keeping the
25,600 B floor therefore requires **22,288 additional bytes**, before later
match allocations. Overlay265,216 B is already265,152 B used. Arena892,416 B;
zero allocator/reloc failures; no battle update and no valid low-water yet.
`hat-heavy-summary.json` preserves the failure and raw-capture hash. This is
containment of insufficient residency, not a fixed or accepted shipping game.

All jobs are terminal. Canonical timing/admission,15 focused tests and both
native-only/ELF gates are retained; heavy natural shipping, positive copying,
mirror sharing on target, Sudden Death/reload and visual checks remain due.
The remaining >=256 B frontend objects sum only17,872 B before excluding shared
UI state (`builds/p2-phase3-frontend-resident-candidates.json`); they cannot alone
fund even this admission shortfall. The two repeated native image binders occupy
22,416 B of text, an upper bound before replacement data/code, also insufficient
for hats plus future play and MF. These are measurements, not reclaimed RAM.

Continue perD7/D9 with **A8 PREP**: trace the existing ARM7 sound/extent services
and price source-reachable resident SFX heads before replacing the237,568 B
eight-slot FGM cache with the architecture's ARM7 tail streaming/voice owner.
The current cache already contains DS ADPCM, so recompressing PCM is not a
valid saving. Preserve all cue, envelope, loop, channel and pause semantics;
fund hat/full-MF admission and resume the missing natural proofs afterwards.
No root or P1 ROM was published. This is an IMPLEMENTED_NOT_ACCEPTED checkpoint.

## Continue: A8 storage ownership

Previous turn was progress: checkpoint2564d1d5ae0 is pushed, all jobs terminal,
and the22,288 B admission shortfall is retained. Root continues alone. Installed
Calico headers, its packaged ARM7 main disassembly, sm64-nds ARM7 channel code
and sm64ds sound-bank declarations were inspected before this architecture step.
Read-only SDK source references are in `builds/a8-sdk-reference/`: Calico commit
81b75e314d57ed1784545e28554e567f26f572f1; libdvm aa0b5aa7573de30e838a0700432b89e059b91bc3.

The real573-cue FGM1 pack has513 wave ranges. Globally shared512 B heads still
cost259,424 B, before voice rings, so loading every head cannot supply the planned
RAM. `a8-head-pricing.json` is pricing only, not a source reachability proof.
Source-derived VS admission remains necessary; no cue is omitted by this step.

Direct-card NitroROM currently reads on ARM9; DLDI's block device already lives
on ARM7. The custom ARM7 retains keypad/touch, RTC, power, block and sound services
and owns native cartridge reads on one PXI mailbox. A linked wrapper selects
that reader for direct-card boot; file-backed boot retains the SDK's device path.
There is no storage route toggle. This is the storage dependency for autonomous
audio, not its finished streaming/voice owner or a memory/performance gain.

Requests occupy their own32 B cache line, with sequence-checked replies. ARM7
validates ROM/main-RAM bounds and rejects request/output overlap. ARM9 transfers
only complete owned cache lines; unaligned destinations and short tails use a
private bounce buffer. Three actual-C host fixtures cover all32x32 source/output
lanes,512 EOF tails, bulk splitting, failed/foreign acknowledgements and cache
handoff ordering. ARM7 source compilation passes.

The first ARM7 ELF exposed the generic SDK linker's0x02fcc000 main-RAM default,
which would overlap this project's ARM9 scene heap. It was never packaged/run.
The final link fixes .main at the packaged SDK's0x02ff0000 and asserts its end,
heap validity and the sheltered DLDI WRAM boundary. Both CPU link inputs now run
the native-only checker before ROM packaging. Target full build first falsifier:
warm `build-p2p8-s7-fpwalk`, Yoster1, walk1/argmax1, log/exit
`builds/p2-phase3-a8-storage-build.{log,exit}`. Source inputs frozen. Preserve the
saved9E4A666D baseline; next short probe must positively engage ARM7 reads and
reach the existing measured hat admission boundary without new corruption.
Build53531 exited0: ROM30615F5D61511AC12EA7C81E96A109C90B209F4A21D9D5F5A767FEEF3ADFB5E7;
ARM9/ARM7 native-only and overlay gates pass. Every packaged ARM7 load segment
matches the custom ELF. Main RAM0x02ff0000..0x02ff01f4; WRAM ends0x037fdd58,
leaving53,928 B below the fixed DLDI block for subsequent audio code/rings. This
is ARM7 headroom, not freed ARM9 scene memory. Saved full identities and binaries
in `a8-storage-inputs.json` / `builds/p2p8-a8-storage-30615f5d/`.
Run the short heavy probe, runner7, `builds/p2-phase3-a8-storage-probe.*`.
Probe50669 exited0 and reaches the existing hat boundary with unchanged41,996 B
free. The custom ARM7's retained services boot, but its direct-card route has
zero requests: the primary verifier is actually a file-backed DLDI boot. This
is **UNENGAGED storage**, not a reader pass. Do not rerun the same route.

Continue the real file-backed seam. Link wrappers capture the ROM volume's
public dvmMountVolume arguments (the installed archive has an undefined call
from dvm_prober.o, so interception is effective). fstat supplies the32-bit first
cluster. A boot-only FAT12/16/32 walker builds a compact immutable extent map;
ARM7 validates/reads it using the block API. No private FatFs structure casts or
guessed partition/path mapping. The map is retained before scene arenas exist.

The SDK's DLDI calls are not serialized across its block thread and an independent
audio worker. A public DISC_INTERFACE adapter adds a priority-inheriting recursive
mutex and bounded16-sector calls. Its driver export restores the original
interface in the exported copy, preserving later chainloading. Driver code/data
are unchanged. Twelve host tests now cover FAT fragmentation/cycles/bounds,
mapped partial reads and actual concurrent read/write callers; exported driver
bytes match the original fixture. Target validation is still owed. Build frozen
inputs in `p2-phase3-a8-mapped-build.{log,exit}`, then probe the primary DLDI path.
Build97402 exited0, ROM901B3CEEBCB5AB7B6C774DB6B33EEBF6F7B421EC29FFDC598BE536F1DE83E097,
saved as `builds/p2p8-a8-mapped-901b3cee/`. Probe76562 positively engages10,473
ARM7 reads but has16 relocation failures and a CPU abort; it is FAILED, and the
probe stopped execution. Transport's zero error count is not content correctness.

Root cause: libnds argv begins at0x02fffe70 and overlaps the application header's
ntr_rom_size at0x02fffe80. That field held the argv pointer (~50 MB), truncating
the new extent map/read bound before late assets in this63 MB ROM. The SDK's old
reader used only the earlier FAT/FNT fields. The file-backed owner now reads the
immutable file header and checks its FAT/FNT identity and file/chip bounds;
direct-card bounds use the preserved device-capacity byte. ARM7 retains the
validated size instead of rereading the aliased RAM field. A host regression
fixture must reject the old-field mutation. Retry after this specific repair:
`p2-phase3-a8-mapped-build-r1.{log,exit}`, then the short mapped probe r1.
The13-test suite passes, including the negative old-field mutation. Build57914
exited0 with both native-only gates and ELF gate green. ROM
4905A1C6254B7CC196D5BBB07207F2E19EBA89197977B17209DE58AB0E04A447 is saved in
`builds/p2p8-a8-mapped-4905a1c6/`; `a8-mapped-r1-inputs.json` verifies its ARM7
payload against the ELF and records the correct63,184,896-byte NTR region.
Probe21642 runs this exact corrected ROM, runner7, mapped-probe-r1 logs.
Probe21642 exited0 and reaches all four heavy fighters with zero storage or
relocation failures. Backend2 is the real mapped DLDI path:30,170 reads,
7,111,460 B returned,2,469,060 B through the partial-line bounce. The one extent
atLBA328032 now covers123,408 sectors. Independent host reconstruction from the
actual slot7 storage image matches **all61,043,200 B from the FNT through NTR end**,
including NitroFS payloads and overlays. ARM9 code before that range contains the
emulator's DLDI patch and is not mislabelled byte-identical. Structured result:
`a8-mapped-r1-summary.json`; source/SDK hashes and both CPU identities are in
`a8-mapped-r1-inputs.json`. ARM7 WRAM ends0x037fe6b0 (51,536 B below DLDI).

The hat admission failure remains exactly38,672 B requested versus41,996 B free,
12 B alignment and25,600 B reserve:22,288 B short. This run reaches no gameplay
update; its UINT_MAX low-water is invalid. It is storage/boot proof, not timing,
audio output, full-match reserve or publication acceptance. Direct-card/DSi
target routes, fragmented media and driver export have only their noted host/
source coverage. The probe now stops at the first relocation rejection rather
than letting unsafe consumers reach a subsequent abort; the passing run had no
such rejection, so it needs no repeat for that observer-only failure-path change.

**Next A8 implementation:** move BGM playback, timer/seam handling and refills to
ARM7 using the admitted ROM media. Then the source-derived FGM head set and voice/
envelope owner can replace the237,568 B fixed cache. Audio code and refills still
run on ARM9 today; no cache/RAM/FPS saving is claimed by this storage dependency.
Reuse all completed proof above. All jobs terminal, no root/P1 publication;
coherent progress remains IMPLEMENTED_NOT_ACCEPTED.
Checkpoint staging encountered another zero-byte index lock,24 minutes old,
with no Git process and an exclusive-open check passing. Removed that exact
abandoned lock under the owner's existing authorization; proof is unchanged.
