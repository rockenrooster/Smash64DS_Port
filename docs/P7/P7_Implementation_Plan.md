# P7 — Unlocks, saves, cheats and balance: implementation plan

**Revision:** 3 (2026-10-18): Board 4, the 1P campaign, the bonus stages and
Training for every fighter, from the "1P, Bonus and Training" section the
owner added to the brief. Revision 2 (2026-10-13): stage balance (S0, S1),
from the "Stage Balance" section; Final Destination stays the N64 original
(owner, the same day). Revision 1: 2026-10-12.
**Owner brief:** [P7_Plan.md](P7_Plan.md) (the
owner's requirement list; this plan does not replace it). **Live state:**
[P7_STATUS.md](P7_STATUS.md). **Depends on:** P4 (the eighteen contents and
selectable Metal Mario). P5 (seven stages) and P6 (Ike, Palutena) are not
prerequisites: their unlock rows are in the table from the start and stay
dormant until their content compiles.

P7 adds very little battle code. It is menu, 1P-manager and save work, plus
five small fighter-balance seams and one stage scale. The standing gate still
applies to every ROM it ships, and P7 must not cost the arena RAM (see
"Memory and code placement").

## Product contract

### Owner approval (2026-10-13)

The owner approved Board 2 (the unlock table) and assumptions A1-A9 as
written, when P5-P7 became the active goal. Rows and assumptions below are
the contract now, not a proposal.

### Owner decisions (2026-10-12)

1. **New fighters are VS-only.** They do not become playable in the 1P Game or
   the bonus stages. In 1P they appear only as challengers. (DS Training is
   still closed (`nds_menu_shell_core.c:1135`). When it opens, its select
   applies the same locks.)
2. **Original trigger sites only.** Challenger fights and the new unlock checks
   run at the end of a 1P Game run and at the end of a bonus practice, exactly
   where the source checks. VS Results keep only the source's own Item Switch
   and Mushroom Kingdom checks. No VS-count unlocks are added.
3. **Records:** every selectable fighter gets the source's VS record fields.
   The VS Record screen covers every fighter. Data > Characters keeps the
   original twelve.
4. **Metal Mario's balance applies to the selectable one only.** Dash and run
   keep their statuses (dash attack, run turn and run brake still work), but
   they move at walk speed and play the walk animation. Gravity is 1.25x. The
   1P Metal Mario stage stays as the source has it.

From the brief:

5. Original characters unlock exactly as the source does.
6. Every new fighter and stage has an unlock condition (proposal in Board 2,
   for owner approval).
7. Several "Challenger Approaching" fights can run back to back.
8. Every new fighter gets an unlock message screen like the original four.
9. Saves follow the source's design.
10. Pressing SELECT on VS Options unlocks everything: all fighters and all
    stages. The unlock is saved, and clearing data erases it.
11. Balance: Wario's jump is 0.75x (done 2026-10-11). Metal Mario as in (4).
    Dedede's neutral A hitbox is 1.25x. Sheik's animations are 1.1x. Bowser's
    walk and run speeds are 0.75x.
12. Stage balance (added to the brief 2026-10-13): Bowser's Stadium is scaled
    up 1.5x, because it leaves too little room to fight. Final Destination
    stays the N64 original in VS and 1P (owner, 2026-10-13, withdrawing the
    brief's 1.5x line for it).

### Assumptions (owner may veto any)

- **A1 Wireless:** in a room, the host's save decides fighters and stages on
  every console. This extends the rule that already governs Mushroom Kingdom
  (`nds_menu_shell_sss.c:443-452`, P3 plan 7.1).
- **A2 Item Switch** stays available from first boot, as the existing owner
  policy says (`nds_menu_shell_vsoptions.c` header). The save still counts
  the source's 100 battles, so the message still fires. Sound Test and
  Mushroom Kingdom stay locked by the save, as they are today.
- **A3 Fresh save:** the source's eight starters, and no new content. An
  existing save keeps its source progress, and its new content starts locked.
  The owner's own save will therefore lose the open roster on the first P7 ROM
  (since 2026-10-01 all twelve have been open in VS). Press SELECT on VS
  Options to get it back.
- **A4 Challenger rules:** a new challenger uses the source's challenger rows
  (`sc1pgame.c:237-292`). That means one stock, a CPU at level
  `{6,7,7,8,9}[difficulty]` minus `challenger_level_drop`, handicap 6, team
  attack on, items off, and the source's time limit. A loss adds 2 to the level
  drop (to the source's cap). The challenge comes back the next time its
  condition is met; a win resets the drop.
- **A5 Order at the end of a run:** first the queued challengers, back to back,
  each followed by its message when it is won. Then the direct unlocks and
  stage unlocks. Mushroom Kingdom comes last (the source shows challengers
  before Inishie). Source challengers keep their source priority
  (Ness > Captain > Purin) at the head of the queue. The queue holds every
  qualifying challenger, not just the first.
- **A6 Event unlocks:** Metal Mario (beaten as Luigi) and Duel Zone (Polygon
  Team with no stock lost) are earned at that stage's clear. Their messages
  show when the run ends, whether it is cleared or not.
- **A7 Cheat feedback:** one cue (the message screen's own
  `nSYAudioFGMDeadUpStar`) and no message screens. When everything is already
  unlocked, the press plays the menu's refusal cue.
- **A8 Balance readings:**
  - Dedede: the hitbox radius grows 1.25x about its centre, on every hit of his
    neutral A (each jab row and the rapid-jab rows, if he has them).
  - Sheik: 1.1x on every animation, specials included. Motion scripts follow
    the animation rate (`ftmain.c:711/769/866`), so hitbox frames come 1.1x
    sooner. Status timers counted in tics are unchanged.
  - Bowser: `walk_speed_mul`, `dash_speed` and `run_speed` are 0.75x, and
    his animations are left as they are. His feet already slide today, and
    the slower speeds are the owner's fix for that (owner, 2026-10-12).
  - Metal Mario: gravity only. Terminal velocity is unchanged.
- **A9 Replay digest:** the balance changes are gameplay changes. Each one
  changes the digest once. Record the new baseline then. "Digest identical"
  still means identical between runs of the same build.
- **A10 Stage scale reading** (S1):
  - The stage's model and collision scale 1.5x about the main floor's centre
    (x 0 at the floor's height), so the floor stays at its height and only
    its width and the walls below it grow. The stage has no platforms (3
    collision lines), so no jump or platform height changes.
  - Spawn points, item points, the respawn platform and Bowser's bombs keep
    their height above the floor; their x scales 1.5x (the bombs move from
    x +-2990 to +-4485).
  - The blast zones keep today's distance from the stage (owner,
    2026-10-13): the side bounds move out by as much as the ledges do, and
    the top and bottom stay. The camera bounds move out the same way. KO
    percentages therefore stay near today's. The blast zones are not scaled
    1.5x.
  - The select-screen preview zoom is divided by 1.5, so the preview still
    fits its frame. The wallpaper does not scale.
  - Like A9, the scale changes the digest once. Record the new baseline
    then.

## Repository evidence (2026-10-12)

- **Save medium.** `src/nds/nds_backup.c` stores `smash64ds.sav` on `fat:` or
  `sd:`, with `.tmp`/`.bak` recovery. The image is 4,096 B
  (`include/nds/nds_backup.h:26`, a static buffer).
  `src/import/battleship_lbbackup.c` writes the source's two copies of
  `LBBackupData` (1,516 B each) at offsets 0 and 1,520, ending at 3,036, and
  flushes the whole file on every `lbBackupWrite`. **1,060 B of the image are
  unused.**
- **Source save layout** (`lb/lbtypes.h:240-298`):
  - `unlock_mask` is a u8; 7 of its bits are used (`lbdef.h:134-154`).
  - `fighter_mask` and `ground_mask` are u16.
  - There are twelve `vs_records`, 92 B each (`ko_count`,
    `player_count_tallies` and `played_against` are each indexed by fkind),
    and twelve 1P records.
  - The scene queue is `unlock_messages[7]`, and `challenger_fkind` is a u8.
  - Backup Clear's classes: "Prize" clears Item Switch, Sound Test and Inishie
    and keeps the four newcomers (`lbbackup.c:176-183`); "All Data" resets
    everything.
- **Source unlock sites:**
  - `sc1PManagerTrySetChallengers` (`sc1pmanager.c:158-186`): an else-if that
    picks one challenger per cleared run.
  - The challenger fight, its message and the level drop
    (`sc1pmanager.c:503-552`).
  - Inishie at the end of a run (`sc1pmanager.c:556-575`).
  - Luigi from Break the Targets practice (`sc1pbonusstage.c:1210-1232`), and
    Sound Test from both bonuses.
  - Item Switch and Inishie from VS Results (`mnvsresults.c:3283-3307`).
  - `mnMessageApplyUnlock` sets the bits and writes the save
    (`mnmessage.c:284-301`). Each message is a text sprite on the collage,
    tint and exclaim decal (`mnmessage.c:122-171`).
- **DS imports.** The whole TUs are imported:
  `src/import/battleship_sc1pmanager.c`, `battleship_sc1pchallenger.c`,
  `battleship_mnmessage.c` and `battleship_sc1pbonusstage.c`. Their scenes
  are routed in `src/port/nds_scene_manager.c` (Message `:131`,
  1PChallenger `:176`).
- **Current locks:**
  - VS select: all twelve open since 2026-10-01
    (`ndsMenuShellCssSaveLocked` returns FALSE,
    `nds_menu_shell_css.c:478`). Contents and Metal Mario are locked only when
    they are not compiled (`:484-516`).
  - 1P and bonus selects honour the save (`nds_menu_shell_onep.c:106`).
  - SSS locks Mushroom Kingdom by the save; in a room it uses the host's mask
    (`nds_menu_shell_sss.c:443-452`).
  - The lobby sends one unlock byte (`net/nds_net_lobby.c:861`, `:1371`).
  - Item Switch is always offered. Sound Test is gated by the save
    (`nds_menu_shell_data.c:166`).
- **Records.** Contents and Metal Mario are masked out of VS records today
  (`ndsP4MNVSResultsSaveBackup`, `battleship_mnvsresults.c:631`). The VS
  Record screen has only a first, per-fighter summary view
  (`nds_menu_shell_vsrecord.c` header, `TODO(VSRECORD-KIT)`).
- **P4 identity.** `fp->fkind` is the parent, and the content rides in
  `gNdsP4PlayerContent`. It is set only for VS matches
  (`nds_match_config.c:794-806`), so a 1P battle cannot hold a content today.
  Content ids 1-18 come from `scripts/p4/contents.json`. Selectable Metal
  Mario is the vanilla kind 13 (`NDS_P4_CSS_GATE_KIND_13`), and he shares his
  main file with the 1P boss.
- **Balance seams.**
  - `OWNER_ATTRIBUTE_SCALES` in `scripts/p4/generate_p4_fighter.py:677`
    scales a content's shipped FTAttributes (this is how Wario's jump was
    done). The deviation is recorded in the content's manifest.
  - The FSM motion command (D0) is ported, and `ftMainSetStatus` /
    `gcSetAnimSpeed` set the animation rate.
- **Run statistics** for predicates: `gSC1PManagerTotalTimeTics`, `TotalFalls`
  and `TotalDamage`; `continues_used`, `spgame_difficulty`,
  `spgame_stock_count` and `spgame_score`; the No Miss Clear bonus bit; and
  the 1P records' `is_spgame_complete`, `bonus1_task_count` and
  `bonus2_task_count`.
- **Stage data (2026-10-13, from the P5 card).** Bowser's Stadium is a row in
  `scripts/p5/stages.json`, built by `scripts/p5/stage_generate.py`: Remix's
  map 0x8CA, geometry 0x8CB and wallpaper 0x8CC (gkind 42). 3 collision
  lines, 23 map objects; camera bounds 6000/-2100/6000/-6000 and blast bounds
  8000/-5000/9450/-9450 (top/bottom/right/left). Its bombs sit at x +-2990,
  y 512, constants ported from Remix's `Hazards.asm` into
  `src/port/nds_p5_stages.c`.

## One selection id

Every unlock and record uses a single **selection id**:

| Ids | Fighters |
|---|---|
| 0-11 | The source's playable kinds (the source's fkind order) |
| 12 | Metal Mario |
| 13-30 | Contents 1-18 (id = 12 + content id) |
| 31, 32 | Ike, Palutena (P6) |
| 33-39 | Reserved |

Stages that P7 can unlock get stage-unlock ids 0-7. These are the P5 stages,
not gkinds, because the source's `ground_mask` stays the source's. A header
generated from `contents.json` and a P7 row table maps between the selection
id, the fkind and the content. Nothing else hard-codes the numbers.

## Board 1 — shared machinery (each item done once)

| # | Item | Done when |
|---|---|---|
| U1 | **P7 progress block.** A small versioned block, 32-64 B, in the unused 1,060 B of the existing image, after source copy 1. It holds its own signature, version and checksum, and a second copy, as the source does. Fields: the fighter unlock mask (u64, by selection id), the stage unlock mask, the cheat flag and reserved space. `lbBackupWrite` writes it in the same flush, so the unlock state and the source save are always committed together. If the block is absent or invalid, it gets defaults; the source block is never touched by that. Backup Clear maps by class: All Data clears all of it (cheat included); Prize clears the new stages and keeps the new fighters, mirroring the source's newcomer/prize split. | Host tests pass for: fresh, both copies corrupt, one copy corrupt, an old pre-P7 save (block absent), a short file, and an interrupted write. A real write, reboot and read holds. Static RAM does not grow. |
| U2 | **Wide VS records.** A records file beside the save, using the same `.tmp`/`.bak` transaction: one 260 B row per selection id (40 reserved), with the source's VS record fields widened to 40 opponents. The original twelve-by-twelve cells are also mirrored into `LBBackupData`, so source-side readers and Backup Clear's VS Record class stay correct, and that class clears the file too. Results replaces today's masking in `ndsP4MNVSResultsSaveBackup` with rows keyed by selection id. A Falco KO counts against Falco, never against Fox. The source caps hold (9,999 / 999,999 / 1,000 minutes). Row I/O uses a buffer of 1 KB or less, so the 10 KB table is never resident. | A match with any mix of originals, contents and Metal Mario updates exactly its players' rows, and they survive a reboot. The write sits at the source's Results save point and adds no visible hitch: the frame-time ring at Results is no worse than before. |
| U3 | **Unlock registry.** One generated table row per new unlock: kind (fighter or stage), id, trigger site (1P run end, 1P stage clear, bonus 1 or bonus 2 practice end), predicate data (fighter set, minimum difficulty, maximum continues/falls/stocks, time limit, score, bonus completion set, prerequisites), delivery (challenger with stage and CPU row, or direct), message sprite and companion unlocks. A small C evaluator reads the predicates and is host-tested. A row whose content is not compiled is skipped, which keeps the P5/P6 rows dormant. | Host tests cover every row just below, at and above its threshold. Every row is skipped cleanly when its content is absent. |
| U4 | **Challenger chain.** The DS seam around `sc1PManagerUpdateScene` builds a queue instead of the one-shot else-if. Source rows come first in source order, then table rows in table order. Each queued challenger runs in turn: challenger screen, fight, message when won (A4, A5). New challenger stages use DS-side rows beside the source's `dSC1PGameStageDesc` and `SC1PGameSetup` rows (the source arrays are not edited). The challenger's content travels in DS-side scene data beside `challenger_fkind`. Bonus-practice challengers ride the source's Luigi route (`sc1pbonusstage.c:1210-1232`). | A seeded save that meets three conditions in one run plays three fights back to back. A loss, then a win, then a level-drop reset all match A4. A RESET mid-chain leaves the save consistent. |
| U5 | **Contents as challengers.** Two parts. (a) The challenger screen rotates a content's model: the content-aware fighter make, its pack and its menu-zoom scale, reusing the Results and select-screen content path. (b) The challenger fight in the 1P battle holds one content CPU: set `gNdsP4PlayerContent` for the challenger's 1P game type and load its battle pack on the 1P arena layout (the campaign keeps the overlay head and borrows the tail). **Spike first.** Measure heap low-water with the heaviest contents (Snake, Sonic, Dedede) against each original player on every challenger stage. If the 1P arena cannot hold a content within the floor, the fallback is to run the fight through the VS battle runner with a challenger descriptor that reproduces the source rules (one stock, time, level, no Results screen). The decision is recorded in P7_STATUS. | Every content and Metal Mario fights as a challenger with 0 native failures, 0 motion reads after GO and heap low-water ≥ 25,600. The challenger screen draws natively. |
| U6 | **Unlock messages.** Message ids beyond the source's seven, each with its sprite and position; a DS-side queue longer than seven; and `mnMessageApplyUnlock` extended to write the P7 block. Before generating anything, inventory the source's seven message sprites: size, format, palette, font and glyph metrics, and how positions are computed (law 7). Then build each new sprite with a generator in that exact style. Output goes under `builds/` only (the public repo carries generators, never ROM-derived art). | All 28 new messages (21 fighters, 7 stages) render natively. Each has a side-by-side against a source message, and the owner signs off on one sample before the batch. |
| U7 | **Locks everywhere.** Each surface and its change: <br>• VS select: restore the source's save lock for the four newcomers (`ndsMenuShellCssSaveLocked`). Add the P7 mask for Metal Mario, the contents and P6. A locked cell refuses the drop, random picks skip it, and it draws the source's question-mark plate (or the P4 grid's equivalent). <br>• SSS: lock P5 stages by the stage mask, and random skips them. <br>• Lobby: send the host's P7 masks; the packet grows, and the build identity is unchanged. <br>• VS Record and Data: follow the masks with the source's `CheckHaveFighterKind` semantics. <br>• Title demo: stays the source's. <br>• Harness, lab and walk builds: keep the dev-open gate (`gNdsSceneHarnessMode`), so probes see everything. | On a fresh save the VS select shows the eight starters. Each unlock opens exactly its own cell or stage on every surface. A guest in a room sees the host's roster. The all-content walk ROM still reaches every content. |
| U8 | **Cheat.** SELECT on VS Options sets every source unlock bit, `fighter_mask` ALL, every P7 fighter and stage bit, and the cheat flag. It then calls `lbBackupWrite` and plays the A7 cue. No message screens. Records and 1P completion are not faked. Backup Clear All Data erases it; Prize clear re-locks only the prize class. | Fresh save, then SELECT, then reboot: everything is open. All Data clear, then reboot: back to starters. A Prize clear re-locks only new stages, Mushroom Kingdom and Sound Test. |
| U9 | **VS Record screen for every fighter.** Finish the source's views (Battle Score, Ranking, Individual) over the wide records, with paging or scrolling over the unlocked fighters. Names and portraits come from the existing present tables. The kit sprites close `TODO(VSRECORD-KIT)`. The source's twelve-column KO matrix cannot show 33 columns at once on a 256 px screen; the layout that replaces it is a very noticeable visual approximation, so it **goes to the owner with screenshots first**. | Every view is reachable and correct against seeded records, cadence is clean, and the owner has approved the layout. |
| U10 | **Probes and fixtures.** Host tests (U1-U4). Seeded disposable saves under a diagnostic save name (never the owner's file). `NDS_1P_WALK_START_STAGE` reaches stage 11, 13 or the run end quickly. A lab-only knob in the 1P seam forces the run statistics a predicate reads; it is compiled out of ship builds, keeping the instrument out of the gate. One probe command per unlock row prints its witness block: condition met, challenger shown, fight result, message shown, bit saved, reboot persists. | Every row's witness is in P7_STATUS. |

### Memory and code placement

- U1 reuses the existing 4 KB image (0 B static). U2 keeps records on disk,
  with row I/O through a buffer of 1 KB or less.
- Menu-only code (messages, cheat, VS Record, lock art) goes in the front-end
  overlay. 1P-only code goes with the 1P TUs.
- Battle-side code is limited to the balance seams. `.main` pads to 1 KB, so
  every 1 KB step of code costs heap ([[main-pads-to-1kb-heap-steps]] in
  project memory).
- Check the VS select's overlay loan after any front-end growth
  (`docs/P4/P4_STATUS.md`, "VS select lends the overlay").
- U5 is the only new arena consumer. It is measured, not assumed.

## Board 2 — unlock table (approved 2026-10-13)

Banjo & Kazooie's row (24) is retired with him (2026-10-17, P6: his cell is
the select screen's random chooser). The table's data is
`scripts/p7/unlocks.json` (U3).

Every condition is met playing an **original** fighter, at an original trigger
site. "Clear" means the 1P Game is cleared. Difficulty names follow the
source (Very Easy to Very Hard). "Direct" means a message screen only, with no
fight. Thresholds marked † are set from a measured run before they ship.

### Fighters

| Sel | Fighter | Site | Condition | Delivery | Challenger stage |
|---|---|---|---|---|---|
| 12 | Metal Mario | 1P stage 11 | Beat Metal Mario as Luigi *(owner)* | Direct (A6) | — |
| 20 | Peach | 1P end | Mario clears on any difficulty with no stock lost (No Miss Clear) *(owner)* | Direct | — |
| 16 | Bowser | 1P end | Mario or Luigi clears on Normal+, with Peach unlocked | Challenger | Bowser's Stadium |
| 19 | Wario | 1P end |  Bowser clears on Normal+  | Challenger | Mushroom Kingdom |
| 27 | Mad Piano | 1P end | Luigi clears on any difficulty | Challenger | Peach's Castle |
| 13 | Falco | 1P end | Fox clears on Normal+ | Challenger | Sector Z |
| 15 | Wolf | 1P end | Fox clears on Hard+ with no continues, with Falco unlocked | Challenger | Sector Z |
| 14 | Ganondorf | 1P end | Link clears on Normal+ | Challenger | Hyrule Castle |
| 23 | Sheik | Bonus end | Link has 10/10 in both Break the Targets and Board the Platforms | Challenger | Hyrule Castle |
| 17 | Marth | 1P end | Clear on Hard+ with no continues | Challenger | Hyrule Castle |
| 18 | Roy | 1P end | Clear on Very Hard, with Marth unlocked | Challenger | Planet Zebes |
| 22 | Lanky Kong | 1P end | Donkey Kong clears on Normal+ | Challenger | Congo Jungle |
| 21 | Crash | 1P end | In a cleared run, finish Race to the Finish with 20 s or more left † | Challenger | Congo Jungle |
| 25 | Sonic | 1P end | Clear in under 11 minutes (the source's Captain rule is 12) | Challenger | Yoshi's Island |
| 26 | King Dedede | 1P end | Kirby clears on Normal+ | Challenger | Dream Land |
| 28 | Meta Knight | 1P end | Kirby clears on Hard+ with no continues, with Dedede unlocked | Challenger | Dream Land |
| 29 | Mr. Game & Watch | 1P end | Every original fighter has cleared the 1P Game | Challenger | Duel Zone |
| 30 | Snake | 1P end | Clear on Normal+ with 1 stock and no continues | Challenger | Saffron City |
| 31 | Ike (P6) | 1P end | Clear on Very Hard with no continues, with Marth and Roy unlocked | Challenger | Hyrule Castle |
| 32 | Palutena (P6) | Bonus 1 end | Every original fighter has 10/10 in Break the Targets | Challenger | Hyrule Castle |

### Stages (P5)

| Stage | Condition | Delivery |
|---|---|---|
| Meta Crystal | Unlocked together with Metal Mario | Companion message |
| Bowser's Stadium | Unlocked together with Bowser | Companion message |
| Hyrule Temple | Ganondorf and Sheik both unlocked | Companion message after the second |
| Final Destination | Clear on Hard+ | Direct |
| First Destination | Clear on Very Hard | Direct |
| Duel Zone | Beat the Fighting Polygon Team (stage 13) with no stock lost | Direct (A6) |
| Dinosaur Land | Yoshi clears on any difficulty | Direct |

Challenger fights on Duel Zone and Mushroom Kingdom load those stages inside
the 1P battle. U5's spike measures them like every other challenger stage.

## Board 3 — balance

### Fighters

Each item ends with: a probe on two heavy stages with the fighter in all four
slots, CPU recovery still working (lower jumps or gravity can break AI
recovery), the card updated, and the owner's playtest.

| # | Item | How |
|---|---|---|
| B1 | Wario jump 0.75x | Done (`OWNER_ATTRIBUTE_SCALES`, 2026-10-11). Verify the CPU recovers, and record it on `docs/P4/characters/wario.md`. |
| B2 | Bowser walk and run 0.75x (fixes his foot slide) | Add `bowser: walk_speed_mul, dash_speed, run_speed x0.75` to `OWNER_ATTRIBUTE_SCALES`. The animation rates stay as they are: the speeds are brought down to match the animations. First check in `ftcommonwalk.c` / `ftcommondash.c` / `ftcommonrun.c` whether the source derives the walk or run animation rate from these speeds. If it does, the derived rate must be held at its current value, or the slide would come back. Acceptance: measure how far a planted foot travels per animation cycle against ground speed, for walk-fast, dash and run. Feet hold within a few units per cycle. If 0.75x leaves a measurable slide on one gait, report that gait's measured ratio to the owner rather than changing the number. |
| B3 | Sheik animations 1.1x | A per-content animation-rate multiplier, applied where `ftMainSetStatus` and `gcSetAnimSpeed` set the rate, and inside the FSM command, so Remix's own multipliers compose with it. Content Sheik only (Kirby's copy uses Kirby's animations). This is a generated row, not a hand-edited one. |
| B4 | Dedede neutral A hitbox 1.25x | `OWNER_HITBOX_SCALES` in the generator: scale the size field of the hitbox commands in his neutral-A motion scripts, with the deviation recorded in the manifest. The generator's round-trip check proves only the size changed. |
| B5 | Metal Mario (selectable) | Patch a VS copy of his main file's attributes, never the 1P one: gravity x1.25; `dash_speed` and `run_speed` set to his maximum walk speed. Map his Dash, Run and RunBrake motions to the walk-fast animation. The campaign boss, his 1P stage and the campaign digest must be unchanged. If his run clips are no longer needed in VS, they drop from his VS pack. |

### Stages (owner brief, 2026-10-13)

Each item ends with: a four-CPU probe with items on the stage, CPU recovery
from both ledges, the fighter's on-screen
height at the widest spread measured before and after (a wider stage pulls
the camera back on a 256x192 screen), the P5 stage card updated, and the
owner's playtest.

| # | Item | How |
|---|---|---|
| S0 | Owner stage scales | An `owner_scale` field on a stage's row in `scripts/p5/stages.json`. The generator writes the scaled data and records the deviation in the stage's manifest, as `OWNER_ATTRIBUTE_SCALES` does for fighters. Before writing it, inventory every consumer of the stage's coordinates: collision vertices, map objects, the header's camera and blast bounds, the root DObj (scaled there, so joint and material animations follow), the native GX packet, the select-screen preview zoom and stage code constants (Bowser's bombs). A10 says what scales and what keeps its height. One table drives every consumer, and no code hard-codes 1.5. Acceptance: a host round trip proves a scaled stage differs from its source only in the fields A10 names. |
| S1 | Bowser's Stadium 1.5x | S0's row for gkind 42. The bomb positions in `nds_p5_stages.c` read the stage's scale (x +-4485 on the first make and on respawn), and their height stays y 512. Acceptance: the bombs hover, explode and respawn at the new x; spawns are on the floor; the respawn platform stays in the camera; the preview fits its frame; a fixed move on a fixed victim at the ledge KOs at the same percentage as before (within a few percent, A10); heap low-water and four-CPU P95 are measured on the stage. |

## Board 4 — 1P, bonus stages and Training for every fighter (owner, 2026-10-18)

The owner's brief: every fighter is allowed in the 1P campaign; every
fighter has bonus stages (Break the Targets, Board the Platforms), Palutena
and Ike sharing another fighter's pair; Training mode is enabled (it was
closed on 2026-10-02, `battleship_mn1pmode.c` answers its confirm with the
denied sound). "Every fighter" means every unlocked one: the select screens
follow U7's locks, and the SELECT cheat opens them all.

Repository evidence (2026-10-18): the 1P and bonus select screens
(`battleship_mnplayers1pgame.c`, `battleship_mnplayers1pbonus.c`) are the
source's twelve cells with its four save locks and no content path; the bonus
stages are the source's 24 (`nitro:/stages/bonus1_<kind>.gxp`,
`bonus2_<kind>.gxp`, the vanilla twelve); the Training scene and its menu are
compiled (`battleship_sc1ptrainingmode.c`, `battleship_mntraining.c`) and the
scene table already treats it as a battle (`nds_scene_manager.c`). P4's product
contract named campaign and bonus modes "a later, separately named tier"
(`docs/P4/New_Characters.md`); this board is that tier.

| # | Item | Acceptance |
|---|---|---|
| C0 | **Inventory first (law 7).** Before any code: every per-fighter asset and table the three modes read, for the source's twelve -- the select screens' cells, names and portraits; the 1P stage intro's portraits and names; the continue screen's model; the congratulations still (`mncongra`, one image per fighter); the ending and staff roll; the 1P high scores and the bonus records in `LBBackupData` (twelve wide); each bonus stage's files, collision, map objects (targets, platforms) and the kind-to-stage table; Training's menu, HUD sprites and its fighter and stage selects. For each: what a content or P6 fighter has today, and the gap. | The inventory is in P7_STATUS. Every gap has an owner (C1-C5) or goes to the owner as a question (the congratulations stills, below). |
| C1 | **1P campaign with every fighter.** The 1P select screen takes the VS grid (P4 cells, Metal Mario, Ike, Palutena) under U7's locks. A content plays the whole campaign as player 1: its battle pack on the 1P arena layout (U5's path, now for the player), its portrait and name in the stage intros, its model on the continue screen, its row in the 1P high scores (C3). The opponents stay the source's: a content plays its parent's opponent list, so the team, giant and metal stages and Master Hand are unchanged. **Spike first, as U5:** heap low-water on every 1P stage with the heaviest contents (Snake, Sonic, Dedede, Palutena, Mr. Game & Watch) as the player, the team stages (Yoshi, Kirby, Fighting Polygon) and Master Hand included. The congratulations stills are per-fighter art (`mncongra.c`, a top and bottom image a fighter): Remix's gallery (`Gallery.asm` `id_table`) shows its own for every Remix content (Roy, Lanky and Mad Piano included), exported as the contents' other art is; EXTRA's three (Meta Knight, Mr. Game & Watch, Snake) and P6's two have none, and for those five the options (the parent's still, a still rendered from the fighter's model, or the source's screen without one) **go to the owner with screenshots first**. | A run from stage 1 to the congratulations screen for one original, the heaviest content, Metal Mario, Ike and Palutena: 0 native failures, 0 motion reads after GO, heap low-water >= 25,600 on every stage, the standing gate on the busiest 1P stages; the owner has picked the stills. |
| C2 | **Bonus stages for every fighter.** Practice (the bonus select screen takes the VS grid) and in the campaign (its targets and platforms stages pick by the player's kind). A content takes the pair its donor's character row names (Remix `Character.asm` `define_character`, BTT and BTP; EXTRA's for Roy, Lanky, Meta Knight, Mr. Game & Watch and Snake): its own for Falco, Ganondorf, Wario, Bowser, Wolf, Marth, Sonic, Sheik, Dedede, Crash and Peach; the donor's share for Roy (Marth's), Lanky (Donkey Kong's), Meta Knight (Kirby's), Mad Piano, Mr. Game & Watch and Snake (the generic `BTT_STG1` and `BTP_POLY`); Metal Mario keeps Mario's. That is 24 stages beyond the source's 24, exported, converted and pinned as the P5 stages are (files, collision, map objects, the native stage program `bonus1_<id>.gxp` / `bonus2_<id>.gxp`), with kind-to-stage rows beside the source's table, never in it. **Palutena and Ike share another fighter's pair.** Candidates to test (owner picks): Ike -- Marth's pair, the one the donor gives Roy (same series; a vertical up special, Dolphin Slash and Blazer for Aether; a sword's reach), else Link's; Palutena -- Sheik's pair (a teleport recovery, Vanish for Warp), else Peach's. A candidate qualifies when every target can be broken and every platform boarded with the borrowing fighter's moveset (a lab input script per stage, or the owner's playtest), and its time is in the range of the owner's fighter. | Every fighter's two stages load and complete (all targets broken, all platforms boarded) with 0 native failures and heap >= 25,600; a completion records in C3's table and wakes U3's bonus rows; the owner has approved Ike's and Palutena's pairs. |
| C3 | **Wide 1P and bonus records.** The source keeps 1P high scores and bonus times and completion per fighter, twelve wide. Rows by selection id (40) beside U2's records file, through the same transaction; the source's twelve stay mirrored in `LBBackupData` so its readers are right; Backup Clear's classes clear them as the source's do. The 1P and bonus results screens and the Data screens read them. | Host tests as U2's: a Falco score is Falco's, never Fox's; a reboot holds; a clear clears. The write adds no visible hitch at the results screens. |
| C4 | **Training mode.** The 1P menu's confirm opens Training again (the denial goes). Its select screen takes the VS grid under U7's locks, its stage select the SSS (P5 stages by U7's stage mask), and its menu (items, the COM's behaviour, speed, view, reset, exit) and HUD are the source's, drawn natively. Training is the source's one man and one COM. Contents, Metal Mario, P6 and Kirby's copies are in it. | Training with one original, the heaviest contents and P6, on every VS and P5 stage, items on: 0 native failures, 0 motion reads after GO, heap >= 25,600; the heaviest content pair's Training on the busiest stages holds the standing gate. |
| C5 | **Probes and witnesses.** Walk builds: a 1P run per C1 class (start stage knob `NDS_1P_WALK_START_STAGE`), each fighter's two bonus stages (a lab input script per stage, or a completion poke for the records path only), Training per C4. One probe command per item prints its witness block into P7_STATUS. | Every C row's witness is in P7_STATUS. |

## Order

| Wave | Items | Why this order |
|---|---|---|
| A | U1, U3 (no rows yet), U7 for the source's four and the existing contents, U8; B1-B5, S0 and S1 (independent) | The first owner ROM: real locks, the cheat to open them, and the balance pass. No 1P battle work. |
| B | U4, U6, U10; rows Metal Mario, Peach, and the original-site stage rows that need no fight | The chain and the message pipeline, proven without a content in a 1P battle. |
| C | U5 spike, then U5; every challenger row | The one real engineering risk, isolated. |
| D | U2, U9 | The records tier, which P4 deferred. The VS Record layout goes to the owner. |
| E | P5 and P6 rows, as their content lands; the final verification matrix | The rows are already in the table and only wake up. |
| F | C0, then C4 (no new battle layout), C1 spike then C1, C2, C3, C5 | Board 4 (owner, 2026-10-18). Training first: its battle is the VS one. The 1P layout with a content player is the board's one real risk, isolated by the spike as U5's. C2's bonus rows in U3 wake with it. |

Waves B and D are independent once A lands, so D can run while C's spike
builds.

## Per-unlock checklist (the definition of done)

1. Predicate host test: just below, at and above; prerequisites; skipped
   cleanly when the content is absent.
2. A seeded run meets it at its trigger site. The challenger screen and fight
   (or the direct message) follow in A5 order.
3. A won fight shows the message; a lost fight follows A4, and the condition
   brings it back.
4. The bit is saved; after a reboot the unlock shows in the VS select or SSS,
   in random picks and in VS Record. It also shows on a guest console in a
   room (A1).
5. Backup Clear: All Data re-locks it; Prize behaves per U1.
6. Fight witness (U5): 0 native failures, 0 motion reads after GO, heap
   low-water ≥ 25,600.
7. Witness block pasted into P7_STATUS.

## P7 exit checklist

- [ ] Every Board 2 row (as approved) passes the per-unlock checklist,
      including the source's seven, which must still behave as the source.
- [ ] Back-to-back challengers proven (three in one run, mixed wins and
      losses).
- [ ] Save: U1 host matrix plus real reboot; old saves load; diagnostics never
      touch the owner's file.
- [ ] Cheat set, persisted and erased as U8 says.
- [ ] Wide VS records and the approved VS Record screen.
- [ ] B1-B5 in, digest baseline re-recorded once (A9), CPU recovery checked.
- [ ] S1 in as A10 says, digest baseline re-recorded; Final Destination
      unchanged from the N64 original in VS and 1P.
- [ ] Standing gate on the shipping config: four-CPU WORK P95 ≤ 1,120,000,
      ≥ 95% within two VBlanks, 0 native failures, 0 motion reads after GO,
      heap low-water ≥ 25,600. No P3 regression (host and guest with
      different saves).
- [ ] Board 4: the 1P campaign, both bonus stages and Training for every
      fighter as C1-C4 say; Ike's and Palutena's bonus pairs and the
      congratulations stills approved by the owner; C5's witnesses.
- [ ] Docs: P7_STATUS, the affected P4 cards and BUGS.md rows kept current.

## Working rules

- `decomp/` is read-only. Game rules stay source code. Source arrays are
  extended beside the source, never edited.
- Generated output is never hand-edited. Owner deviations (balance) live in
  the generator's owner tables with a manifest entry.
- Never edit source while a build is running. Use build time for host tests,
  inventories and docs.
- Diagnostic ROMs use their own save name. Probes run one at a time (they
  share one DLDI image).
- Ask the owner only about the Board 2 table, the VS Record layout (U9), the
  message-sprite sample (U6), Ike's and Palutena's bonus pairs (C2), the
  contents' congratulations stills (C1), and anything that is a very
  noticeable visual approximation or a contract change.

## Top risks

1. **A content in a 1P battle (U5, C1).** The campaign arena layout is not the
   VS one. The spike decides the in-scene path or the VS-runner fallback before
   any challenger row is written; C1's spike repeats it for a content player on
   every 1P stage, the team stages and Master Hand above all.
2. **Save write hitch at Results (U2).** The rewrite grows from 4 KB to
   4 KB + 10 KB. Measure it; if it shows, move the records commit to the
   scene's fade.
3. **`.main` growth costs heap in 1 KB steps.** Keep code out of the battle
   image.
4. **Owner saves.** A3 is a visible change on the owner's first P7 boot; the
   cheat is the remedy, and the playtest notes must say so.
5. **Balance knock-ons.** Lower jumps and heavier gravity can break CPU
   recovery. That is why each B row has a CPU recovery check.
6. **Stage scale knock-ons.** A wider stage pulls the camera back, so
   fighters draw smaller on the 256x192 screen when they are far apart. The
   blast zones move with the ledges (A10, owner), so KO distances stay
   today's; S1 checks a sample KO percentage before and after, and measures
   the on-screen height.

## Proposed goal text (for the owner's /goal)

> Implement P7 against docs/P7/P7_Implementation_Plan.md. Done when: every
> approved unlock row and the source's seven pass the per-unlock checklist;
> back-to-back challengers work; saves, records, the VS Record screen and the
> SELECT cheat behave as U1-U9 say; B1-B5 and S1 are in; the standing gate
> holds on the shipping config; no P3 regression; docs current. Guardrails as
> P4's.
