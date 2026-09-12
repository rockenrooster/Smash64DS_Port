# P2 Execution Board

## Latest integration checkpoint — 2026-09-11

**This is the current dynamic queue entry and supersedes the older pack-gate
summary later in this file.** Tested-window four-fighter capacity is recovered;
full P2 acceptance remains red. DamageSlash, Kirby SpecialN hidden-part output,
Sword, Final Cutter effects + travelling weapon, Donkey low-detail Up Smash,
and **Samus Catch** are feature-scoped CLOSED for their recorded natural paths.

- **P2-3 Samus feature:** CLOSED for the measured natural window. Source-derived
  Catch/CatchPull topology expands Samus from 14 to 21 roots; the native program
  engages without rejection and SamusSpecial2 grapple root `349:0x02E0` draws
  12 times with its live two-frame TEXID animation, fallback 0 and texture
  reject mask 0. Evidence: `artifacts/visibility/2026-09-11_samus-catch-native.md`.
- **Next integration outcome (existing P2-3 ownership):** Link Catch, LinkModel
  asset `324`, root `0x5B68`, status `0xA6`, `REJECTED_PROGRAM`, now the first
  failure in the one-minute four-CPU wide verifier. Check the complete source
  Catch feature/siblings/bindings; do not create a duplicate row.
- **P2-3f47:** CopyLink remains independently open; do not conflate it with the
  Link Catch blocker above or reopen closed Final Cutter/SpecialN work.
- Capacity/global performance, CSS, stages, remaining items and 1P acceptance
  remain scoped to their existing rows; P2-2p8 optimization and raster work stay
  owner-deferred.

The isolated Samus-only index regenerates and compiles, but its stripped harness
did not reach battle before the 900-second observation bound; that timeout is
not acceptance evidence. Natural/wide acceptance remains tied to the frozen
integrated ROM and hashes in the permanent Samus report.

Created: 2026-08-17.
Updated: 2026-09-10. **"Zero native failures" means no owner DECLINED and
nothing stronger** -- the recorder is a first-failure latch and NO_PROGRAM fires
only when no owner claims a display list, so an owner that draws nothing passes
it (BUG_NOTES). A clean checkout now builds `smash64ds.nds` (51,395,584 B); the
ITCM overflow that blocked it is cleared.

**Largest risk to P2, structural: the four-fighter pack gate.** The two smaller
RAM blockers in front of it are closed in the current integration build: the
four-distinct-kind stress ROM reaches frame 64 with all four pose slots bound,
53,128 B general-heap low-water, `sGCCommonsMaxNum=-1`, 60 active GObjs, objman
panic 0 and allocator overflow 0. The 1,536 B graphics heap peaks at 16 B with
overflow/no-room 0. The pack-disabled shipping-shell skeleton then halts before
battle on the fourth raw tree (77,360 B requested / 23,732 B free) after the
first three raw trees spent 300,304 B. Even deleting those three for free and
charging zero for every post-stop cost leaves at most 291,268 B for a pack while
preserving the 32 KiB floor. Object liveness over dependency-only extern-closure
files removes file-granular baggage (including 77,296 B of unrelated
`ITCommonObject` from Yoshi); current worst is 361,362 B raw / 351,776 B with
every still-unresolved bank moved to VRAM, therefore short **at least 60,508 B**.
The older 175,604 / 227,380 model is historical, not an exact
current-shell ceiling. Evidence:
`artifacts/performance/2026-09-10_pack-skeleton-ceiling/CEILING.md`. Kirby's
copy-hat deferral paid 103,652 (`4d8d9d27179`). **No lever reaches green**: the largest, low-only
(112,388), is dead -- High is reachable on any KO/pause -- leaving ~153,178
over; the rest is census-scale. RAM has no
sacrifice-order runway -- over floor is a halt.

**The only dynamic queue.** Normal restart reads `docs/HANDOFF.md` + this file.
Plans live in `docs/P2_PLAN.md` + `docs/p2/`. Closed row history lives in
`docs/archive/P2_CLOSED_ROWS.md`; measurements in `PERF_LEDGER.md`; chronology in
`PORTING.md`. Those large documents are lookup-only during ordinary work.

## Standing rules

1. **Measurement law:** `docs/VERIFYING.md` owns procedure. Boundary is
   `p2_shell_loop`, `p2_battle_realtime`, and `p2_fourcpu_stress`.
   `p2_battle_realtime` is mode 163: shell-driven Mario human vs level-3 Fox,
   Dream Land, one-minute Time, items off. Gate arms use the one-minute match;
   long soak length is a separate flag.
2. Cadence verdicts use all presented frames; the 1,600-frame gameplay rank-80
   remains candidate-sizing evidence. Measure the configuration that actually
   ships (`nds_build_config.h` is truth).
3. **Publish law:** P2 publishes only verifier-covered `smash64ds.nds`, from the
   shipping VS shell with human input, no scripted walk, and no fast logic.
   Rebuild it after each verified fix batch. The frozen P1 artifact is not
   rebuilt routinely.
4. The canonical current P2 ROM hash appears on exactly this line:

SHA-256 2CB6B86242F9BF2B0CF8D99FF0405C1C4F87DE38F1A03AA51D3514BED421DF99

5. Performance/visibility evidence is permanent under `artifacts/performance`
   and `artifacts/visibility`. Device A/B reports include 2/3/4/5+ VBlank
   histogram, max interval, and P50/P95.

## Phase status

| Phase | State | Gate summary |
|---|---|---|
| P2-1 VS shell | **VS Mode reference; VS Options visuals accepted** | `eafdf226c52` connects native VS Options/Item Switch entries, accepted visually. `p2_shell_loop` is red on **its own free-floor assertion** -- 1,968 B against a 32,768 minimum. **CSS cadence remains RED after the 8 KiB/4-node load slice:** the atomic load+finish tic is gone, but stop 5 is still 4,409,600 ticks and `MSVB3 1482/110/24/35 max=8`; 7 loads finish with zero failures while retry rises to 84. Do not tune K before pricing read vs publish/finalize inside one continuation (BUG_NOTES). FPS HUD `b242a60acaa`, latch `705e39b4be0`. |
| P2-2 Four-fighter engine | **Pack RAM cliff and performance RED; startup/latch CLOSED locally** | Current four-kind stress reaches frame 64 with all four pose slots bound; free-min 53,128 B >= 25,600, `sGCCommonsMaxNum=-1`, objman panic/allocator overflow 0. Graphics heap is 1,536 B, peak 16 B, overflow/no-room 0. The old frame-0 tag/pose OOM and frame-45 GObj latch are therefore out of the critical path. Shipping-shell skeleton gives a relaxed pack ceiling <=291,268 B; current worst is 361,362 B raw / 351,776 B VRAM-bound, short >=60,508 B before charging fighter 4/later startup/binder. WORK-H P95 2,808,768 exceeds target. |
| P2-3 Fighter production | **Nine landed; no owner declines, which is weaker than it reads** | 09-09, 1,200 presents: no owner declined on any of the nine -- but the probe never grabs, rolls or specials, and `renderer_adapter_fighter.c:2152` records Yoshi declining 8,360 times in 1,200 presents. Battle acceptance, Ness smoke, Kirby heap and roster acceptance remain. Details: `docs/p2/fighters/`; pose clock: P2-3c1. |
| P2-4 Stage production | **Nine landed; no owner declines, four surfaces still invisible** | **Castle roof CLOSED 09-09** -- texture conversion, not geometry: the steep-roof CI4 carries colour behind source alpha zero while the N64 combiner's final alpha ignores TEXEL0/1, and the DS path discarded it. Six geometry theories died first. Capture `0909-roofalpha2-castle.png`; account in BUG_NOTES. Collision parity verifies vs source on nine stages in 0.188 s. **Unproven on screen** (submit-proved only): Yoster platforms, Inishie bricks, Congo barrel. Zebes shafts DRAW: hard-edged, not missing. |
| P2-5 Items | **DRAW is the blocker: 25 of 45 kinds have owners, 20 remain** | Spawn law, switch mask, frequency and mball chain are live and source-faithful; 45/45 makers registered; pickup FileIDs resolved. The remaining 20 are mechanical, not twenty investigations: most fit existing generator templates as table rows, ~5 need a shape parameter. **The real constraint may be the particle atlas** at 32,768/32,768, full; 5 excluded need 5,120 B. Two batches shipped with no Makefile rules and a clean-checkout build had to find it. |
| P2-6 1P Game | **UNPAUSED by owner 09-10** | 13 plan items now need owners and none has a study. CSS pushed (`d9161127d46`); local integration reaches Intro and Link/Hyrule play after GO at 8,356 B free. Memory margin and campaign acceptance red. Ships `NDS_P2_1P_GAME=0` until a flag flip is verified. |
| P2-7 Modes & meta | **Options/Backup Clear visuals accepted; validation open; Data inaccessible** | Owner (09-06): VS Options, Option and Backup Clear look good; native route, cancellation and host confirmation/clear tests pass. Cadence and disposable-save persistence need verification. |

## Current integration checkpoint

**Critical path, re-derived 09-10 against 131 items:** pack residency + skeleton
build (`D_other`/`D_binder`, ceiling re-pin) -> census-scale answer -> roster/CSS
capture -> items-ON stress
argmax -> stage closure -> final gate. Owner reports in `docs/BUGS.md` are the
authoritative symptoms. 1P unpaused 09-10.

| Unit | SOURCE PRESENT | COMPILED/LINKED | RUNTIME VERIFIED | ACCEPTED |
|---|---|---|---|---|
| Four distinct fighters + real items | `e99db8cf004`, `e5ac85862f8`, `46a5aa33c52`, `21420ebd843` | Strong providers, ITEM_CORE=1 | 2 items, 31,988 B floor, all four draw slots | No: cache engagement, performance, ending/Results owed |
| Shell memory/pacing evidence | `7244f63a95a` | Shell ROM hash above | One-minute lifecycle passes; snapshot flush fixes stale debugger reads | Instrument correction verified; P2 acceptance open |
| Options source-menu handoff/admission | `68c0e522d3c`, `f33c5aa039f` | Root human-input ROM | Startup/nine-entry route, 23 host cases, shell/battle checks pass | Test-ready; remaining modes/visual acceptance open |
| Compact 1P previews | Producer `87c6be2549b`; local loader/bridge | Twelve FPC packs compiled; source and actual-C tests pass | Link preview and menu render, 66,972 B free; onward battle OOM | No: roster tour, Intro rendering and campaign gates remain |

Main owns live integration and serialized builds. Preview artifacts sit in
`builds/resume-20260905/{preview-compact,preview-binding}`.
**1P is unpaused (owner, 09-10)**; the compact preview packs in flight are no
longer a pause question. Preserve local campaign integration.

Root `smash64ds.nds` republished 2026-09-09 (51,454,976 B); hash above re-pinned.

## Queue — acceptance only

Owner checks, not implementation work unless a reproduction fails.

- P2-1 shell presentation; P2-2 four-way camera, lower HUD, Team feel, Results and Sudden Death.
- P2-3: Mario/Luigi pipe (`P2-3r1`), Luigi animation (`P2-3r2`), intro visibility (`P2-3r5`), CSS preview rebuild (`P2-3r7`), Falcon/Samus feel.

## Queue — P2-3 engineering

| ID | Slice | Status | Next / evidence |
|---|---|---|---|
| P2-3r17 | Intermittent fighter seams/holes around DK and Mario cap | **DEFERRED BY OWNER** | Root-caused as an N64-to-DS raster coverage mismatch, not missing geometry; the production fix is a bounded AOT guard band in the owner generator. Full analysis and acceptance: `docs/BUGS.md`. |
| P2-3f33 | Link entry wave/beam native graduation + integrated specials acceptance | **PARTIAL — static/native checks green; runtime acceptance owed** | Detail: `docs/p2/fighters/link.md`. |
| P2-3f46 | Yoshi stress arm: the landed argmax moves and the roster arm halts before its first sample | **BLOCKED behind P2-2p8** | Same tick-HUD ceiling as the four-CPU arm; resume with it. |
| P2-3f47 | Roster close: Ness, Jigglypuff and Kirby | **IN PROGRESS** | Kirby/Fox OOM on Fox's 115,440 B allocation; fix residency. Jigglypuff fixup closed; Ness smoke, CSS capture and stress remain. Detail: `docs/p2/fighters/{ness,jigglypuff,kirby}.md`. |
| P2-3f48 | ITCommonData (0xfb) residency | **LANDED (`45d5fead788`); runtime unverified** | `gITManagerCommonData` loads in `itManagerInitItems`; both asset rows and the address-shaped token row are in. **To close:** read `gNdsITCommonDataBytes` on a booted ROM and confirm 82,976, not 68. |
| P2-3c1 | Exact pose clock | **WIRED; runtime differential/cost owed** | Binary32 clock replaces Q12 timing (`f6f65a…`, `nds_f32_exact.h`); pose values stay Q12. Run `test_pose_clock_differential.py` live set through ROM oracle and measure cost. |

## Queue — P2-4 engineering

| ID | Slice | Status | Next / evidence |
|---|---|---|---|
| P2-4s1..s8 | All eight VS stages | **REOPENED — owner rejects current result** | Missing backgrounds on all eight; geometry, moving platforms/collision, hazards, pipes and audio failures vary by stage. See verbatim `docs/BUGS.md`; verify source behavior and native routing before closing. |
| P2-4n1 | Native stage packet and actors | **38 blob packets plus Dream Land linked; acceptance open** | Host tests pass. Actor arms run since the ground-vars union fix; acid keeps G_ZBUFFER. Barrel submits but is unproved on screen. Lakitu/Bronto and visual acceptance open (`docs/p2/BUG_NOTES.md`). |

## Queue — P2-5 items

| ID | Slice | Status | Next / evidence |
|---|---|---|---|
| P2-5i1 | Item manager and twenty common items | **ALL 20 IN THE ROM** | Runtime acceptance remains. |
| P2-5i2 | The 13 Poke Ball Pokemon | **ALL 13 IN THE ROM; draw owners missing** | Dispatch proved by `gNdsItMonsterMakerMask` = `1fff`, read off the table: a ball opens only when thrown or hit, so a 60 s CPU match can spawn five and open none. |
| P2-5i3 | Stage-spawned kinds | **8 OF 10 IN THE ROM; two behind the 1P flag** | POW, Piranha and Saffron's five ship; Target and TaruBomb behind `NDS_P2_1P_GAME`. Native owners exist for 1 of 42 distinct item shapes. |
| P2-5i4 | Pick up, throw, shoot and swing | **LANDED; acceptance open** | Pickup animation FileIDs (all `0u`) resolved 09-09. |
| P2-5u1 | Item Switch and VS Options screens | **Entry/row repair committed; acceptance open** | `eafdf226c52`. Switch mask is honoured by the spawn law; the UI half is uncensused. |
| P2-5x1 | Audio cue coverage | **SOURCE WIRED; ROM acceptance pending** | FGM header pins 573 entries / 6,874,344 B; census covers all 47 banks. |
| P2-5a1 | Item TU fidelity audit | **CLEAN** | All 21 item TUs line-by-line against decomp, 09-03/04. |

## Queue — P2-2 performance debt

| ID | Slice | Status | Next / evidence |
|---|---|---|---|
| P2-2p8 | Four-CPU renderer/performance, target `<1.12m` ticks | **ACTIVE; red** | Deferred by owner: nothing is optimized until content is correct. |

## Queue discipline

- Keep only red/current/deferred/owner-acceptance summaries; move closed detail out at once, and keep each row short enough to decide the next action without loading its history.
- After verified progress update this queue and the owning evidence doc, once each. Agent scratch under `builds/` is gitignored -- a finding left there is lost.
