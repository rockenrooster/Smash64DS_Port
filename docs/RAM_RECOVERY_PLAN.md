# Main-RAM Recovery and Residency Plan

## Purpose and authority

Make the required scene/match working set fit reliably, then use recovered RAM
for measured runtime benefit. [PROJECT_GOAL.md](../PROJECT_GOAL.md) owns product
requirements; [P2_EXECUTION_BOARD.md](P2_EXECUTION_BOARD.md) owns the active
capacity package, accepted scope and deferrals. This plan supplies recovery work,
not a new queue or permission to resume owner-deferred CPU/raster optimization.
[P2-2-pack-estimator.md](p2/P2-2-pack-estimator.md) owns semantic set/cost verdicts;
[P2-texture-residency.md](p2/P2-texture-residency.md) owns required scene admission.

The c118 census, 96/128 KiB recovery targets and estimated 82 KiB animation
deficit were a two-fighter starting snapshot. Do not carry them forward as current
sizes or repeat completed cuts to satisfy an obsolete byte quota. Stop at the
measured capacity/residency outcome of the current package.

## Starting evidence, not a universal baseline

The [September 12 integration report](../artifacts/visibility/2026-09-12_link-native-integration.md)
supersedes older LOW-only and shared-disk measurements for its tested configuration.
It records isolated source-default Donkey/Samus/Link/Kirby gameplay with both detail
levels resident, while distinguishing capacity from failed native acceptance.
It does not qualify every roster, shipping-shell overlap, Results/rematch or final
performance. Use its kept-checkpoint section, not superseded numbers farther down.

The [battle-core recovery report](../artifacts/performance/2026-09-10_pack-skeleton-ceiling/BATTLE_CORE_RECOVERY.md)
explains the earlier compact closure/ShieldPose engagement. Keep its historical
configuration limits. Do not restart the fourth raw-tree investigation as though
compact cores were never implemented. Revalidate affected assumptions when code,
inputs, reachable states or lifetimes change.

Reserve domains must be explicit:

| Scope | Existing requirement / source |
|---|---|
| Four-CPU general-heap low-water | 25,600 B in `scripts/verify-p2-four-fighter-stress.ps1`. |
| Shell-loop remaining scene-arena space | 32,768 B default in `scripts/verify-p2-shell-loop.ps1`. |
| Original two-fighter residency work | 32 KiB general-heap planning reserve; retain or explicitly re-scope it for that work. |

These are not interchangeable pools or a license to lower a budget. Prefer extra
margin, but do not silently impose a new universal floor on already scoped proof.
A final-sample free count is not a lifetime minimum; name the counter and window.

## Safety boundaries

Preserve source-equivalent behavior, required content and all-ROM native rendering.
No capacity credit for hidden output, removed HIGH detail, altered item law, fewer
fighters or missing move/child states. A native rejection can yield scoped resource
evidence, but unexecuted required work may add cost; full acceptance remains open.

Prove allocations instead of only increasing arena/cache constants. Avoid new
active-frame general-heap churn or streaming to disguise static savings. Required
setup must fail cleanly when it cannot fit; never globally return NULL to unchecked
legacy callers, spin on exhaustion, corrupt memory or drop required gameplay work.
Follow [VERIFYING.md](VERIFYING.md) for stable builds, input isolation and batching.

## 1. Establish a configuration-exact ledger

Reuse a valid baseline; remeasure when its inputs or allocation contract changed.
Record commit/dirty overlay, ROM/ELF hashes, build flags, generated payload identity,
roster, stage, items/rules, storage and setup/gameplay/transition coverage. Separate
shipping from instrumentation: deleting diagnostic-only BSS is not product recovery.

Use the linker map/symbol sizes for `.main`, `.main.rw`, `.main.bss`, ITCM/DTCM
and heap-start boundaries; do not double-count containing sections. Identify the
largest current owners, then reconcile the runtime allocation actually obtained.

| Evidence | Existing witnesses |
|---|---|
| Taskman selection | `gNdsTaskmanArenaChosenSize`, `gNdsTaskmanArenaAllocFailCount` |
| General-heap minimum | `gNdsTaskmanGeneralHeapFreeMin` |
| Graphics capacity/peak | `gNdsTaskmanGraphicsHeapCapacity`, `gNdsTaskmanGraphicsHeapHighWater` |
| Refused/corrupt graphics work | `gNdsTaskmanGraphicsHeapNoRoomCount`, `gNdsTaskmanGraphicsHeapOverflowCount` |
| Allocation/object faults | `gNdsSyMallocOverflowCount`, `gNdsObjmanPanicCount`, affected pool witnesses |
| Animation arena/demand | `gNdsR2AnimCacheArenaReservedBytes`, `gNdsR2AnimCacheArenaUsedBytes`, misses/rejects/read counters |
| Older animation battlepack | `gNdsBattlePackResidentBytes`, carve-decline/match-kind counters |
| Compact cores/ShieldPose | Load counts, actual resident bytes, Main extern patches and ShieldPose fixup/failure witnesses from the current probe |
| Native output | `gNdsRendererNativeFailure` plus positive state/geometry/visible-output evidence |

Compact fighter cores and the older animation battlepack are different owners;
do not use one counter to claim the other engaged. Inspect current diagnostic
parameters before reuse. Keep witnesses from the same run and coherent publication
boundary; add only the small counter needed, not an instrumentation-heavy census.
Distinguish arena search retries from final allocation failure. Zero overflow does
not prove success where the writer instead returns a no-room rejection.

**Exit:** the deficit, memory domain, required reserve, lifetime and largest owners
are known for one reproducible configuration. Static boot headroom alone is not enough.

## 2. Price the complete simultaneous live set

Reuse `scripts/fighters/estimate_fighter_pack.py`, the production manifest and
native-image census under the estimator contract; do not rediscover their file
inventory. Trace Main/Model, both details, hidden/copy parts, poses, materials,
collision, children and audio. Include semantic selectors, late reset readers,
transitive externs, aliases, fixups, alignment and preparation workspace.
Unclassified required consumers are STOP, not dead data or zero-cost entries.
Prove superseded raw storage actually leaves the simultaneous live set.

Shared data counts once only when runtime ownership shares it. Per-player mutable
state still counts per player, including mirrors. Record each allocation as:

`owner | requested/aligned bytes | first write → last read | peak | reserve`

Charge old/new scene overlap, decode/binding work and asynchronous consumers.
A partial allocation wrapper may miss intra-translation-unit calls; reconcile
those before claiming an absolute ledger. Unmeasured setup after a probe stop is
unknown, not zero. A relaxed upper capacity bound can prove RED when required
minimum demand exceeds it; it cannot prove GREEN. Apply the estimator's allowance
equation at one common phase/allocator, charging each retained or removed byte once.

Required capacity is the peak simultaneous live set plus reserve, not a sum of
unrelated high-waters. For all-roster claims, cover the estimator's full declared
set/profile domain, including multiplicity, costumes, details and capabilities;
targeted runtime samples do not replace that host proof. Stage/items/audio and
campaign profiles add their own costs. The memory worst case need not be the
P95 worst case; direct battle alone does not cover shell/Results overlap.

**Exit:** record the estimator's STOP, RED, CONDITIONAL or GREEN-for-migration
verdict and its exact domain. Migration permission is not gameplay acceptance.

## 3. Recover bytes at their owning representation

Select the largest unrefuted lever for the measured deficit; these are alternatives,
not phases to repeat after enough capacity exists:

1. Remove unreachable storage or duplicate raw/decoded residency.
2. Compact source-derived closures/poses/tables while preserving every reachable
   consumer and identity. Preserve HIGH/LOW and restore Main externs/ShieldPose
   dependencies; a CSS-only preview pack is not a complete battle pack.
3. Move scene-only storage out of permanent residency, or share scratch with proven
   disjoint lifetimes, including transitions and asynchronous users.
4. Right-size bounded pools from measured peaks plus justified overlap/reserve;
   distinguish leaks from legitimate peaks and exercise bounded failure behavior.

Before structural DS memory choices, inspect relevant BattleShip and both DS
references via [DECOMP_MAP.md](DECOMP_MAP.md). Trace every producer, consumer,
escaped pointer, extent/stride and teardown. DMA/GX/audio ownership ends at final
consumption, not producer return. Use explicit alignment/size assertions; avoid
undefined typed aliasing or dangling source-offset pointers after compaction.
VRAM relocation needs its own bank/lifetime and transfer-cost budget.

### Already changed or refuted: do not rediscover blindly

- `gSYFramebufferSets` is now `[1][231][320]`, 147,840 B, aligned to four bytes
  in `src/import/battleship_sys_framebuffer.c`, not the old 441,600 B triple.
  The extra row covers the Results wipe's read extent; keep its contract in
  `include/sys/video.h`. Further reduction needs a new consumer/lifetime proof.
- DL-preview storage is hardware-guarded; sprite staging and decoded wallpaper
  historically served different jobs. Output dimensions do not prove write extent
  or scratch-tail size. Audit the current linked representation before counting it.
- Texture refresh staging can remain queued until VBlank while conversion scratch
  is reused. The historical Scratch/Small/Large union was unsafe; sharing requires
  changed ownership, not just similar buffer names.
- LOW-only battle packing was removed from the current integration: ordinary source
  paths can request HIGH. Do not promote its larger heap margin as current evidence.

**Exit:** predicted linked/runtime savings are observed, replacement engagement is
positive, and required behavior/lifetimes remain intact. Record why a rejected lever
failed; reopen it only when its relevant representation or consumer changed.

## 4. Spend recovery deliberately

Report static-image change, chosen-arena change, heap-minimum change, resident
bytes and transient peak separately. They are related observations, not independent
savings to sum. Growing a cache inside the recovered arena spends those same bytes.

For animation residency, derive unique required asset/file requests, aligned sizes,
first-use status/phase and repeats. Separate setup admission, resident hits and
post-GO misses/rejects. Include reachable unobserved states before calling a working
set complete. Prefer `prepare → normalize/bind → validate → GO → bounded use`.

Required P2 admission is deterministic before GO: mandatory battle/motion/texture
demand reads are zero afterward, and required texture create/upload/delete/evict/
convert work is zero in the locked epoch. Legitimate material/palette animation
uses pre-admitted native representations. Prove resource-class deltas, complete
required/admitted sets and positive use; cache statistics alone cannot prove this.
A compact-capacity result or green four-CPU script does not establish residency.

Do not introduce gameplay LRU/reload when a simultaneous required set cannot fit.
Recover/compact more or use an approved representation; admission must fail safely
rather than silently omit content. Retire resources at qualified scene boundaries
only after all users finish. BGM is a separate declared streaming client; required
one-shot cues need their own resident/deadline-safe policy. CSS loads and transition
peaks still require responsive presentation and valid lifetimes.

**Exit:** required resources are admitted, preparation/transition peaks and reserves
fit, post-GO demand gates pass, and declared audio service remains correct. Keep
capacity-only progress separate when any residency or output proof remains owed.

## 5. Qualify the outcome and stop

Use the existing verification workflow, not a second RAM test stack. Start with
closure/layout checks and cheap startup/admission proof, then the relevant integrated
checkpoint. Cover changed lifetimes through selection/loading, specials/children,
shield, KO/rebirth, pause/detail changes, Time Up, Results and re-entry as applicable.
Do not run unrelated whole-game coverage after every edit; preserve P2 per-unit gates.

Bank before/after identity, static footprint, arena/heap floor, pool/graphics peaks
and faults, compact/cache bytes, animation I/O/rejects and state/output coverage in
the existing artifact/ledger owner. Label failed/unrun checks and diagnostic overhead.
A timeout or no-op output cannot close capacity for work that never executed.
Owner playtesting supplements, not replaces, the measurable proof.

Stop speculative RAM hunting once the affected live set and reserve are proved.
Reject corruption, lost content, unsafe exhaustion or unaccepted CPU/streaming
tradeoffs instead of weakening coverage. Re-bank representative ticks/cadence when
authorized for performance acceptance; subsequent work comes from current evidence
and the board, not the old plan's automatic next-CPU instruction.
