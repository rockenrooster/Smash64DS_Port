# P4 — New Characters: implementation plan

**Revision:** 3 (2026-10-07), rewritten for implementation speed after Falco
(P4.1) proved the whole path. Replaces Revision 2 of September 5, whose
product contract and residency rules carry over unchanged (below, condensed).
**Live state:** [P4_STATUS.md](P4_STATUS.md). **Shared rules:**
[01 source admission](shared/01_Source_Admission.md),
[02 roster integration](shared/02_Roster_Integration.md),
[03 DS resources](shared/03_DS_Resources.md),
[04 verification](shared/04_Verification.md). **Cards:** [characters/](characters/).

## Why the plan changed

Falco showed where the time goes. Almost all of it went into machinery every
fighter needs: the donor export and generator, the native-owner donor mode,
HUD, Results, character select, sounds, victory music, the CPU rows and the
select-screen preview pack. Falco's own code is about 400 lines of C
(`src/port/nds_p4_falco.c`). Revision 2 would have repeated a full
one-fighter-at-a-time cycle eighteen times. Revision 3 changes three things:

1. **Finish the shared machinery for the whole roster, not just for the next
   fighter.** The exports already say exactly what the roster needs (table
   below). Each seam is built once, data-driven, and is done when every
   fighter that uses it is covered.
2. **Order fighters by measured cost**, cheapest first, keeping families
   together, so each wave's shared work is paid once.
3. **One acceptance probe and one fixed checklist per fighter**, so "done"
   is a command and a list, not a review.

## Product contract (unchanged)

- Original DS envelope: no RAM expansion, no DSi requirement.
- Roster: Revision 2's 18 selections. These are the 14 main-tree fighters
  below, Metal Mario (the existing MMario kind) and EXTRA's Meta Knight,
  MRGAW and Snake. No other Remix variants.
- Fidelity: original SSB64 common rules plus the pinned donor's
  character-specific mechanics. Remix's engine-wide changes stay outside the
  profile. One example is the aerial fast-fall check that Remix puts on every
  fighter's aerial statuses.
- VS admission: any 2-4 fighter selection, including mirrors, costumes, teams,
  items and the donor's Kirby copies. The first release tier is VS and
  Training. Campaign and bonus modes are a later, separately named tier.
- Performance: the standing gate, four-CPU WORK P95 <= 1,120,000 and >= 95%
  of frames in two VBlanks. Replay digest identical, 0 native failures, 0
  motion reads after GO, heap low-water >= 25,600.
- Residency (shared plan 03): the unit is the selected match. Everything
  needed is resident before GO, nothing faults in during combat, and preview
  packs never hold battle closures.

## Measured roster cost (exports of 2026-10-07)

`remix_export.py` on the pinned donor. Columns:

- **Remix asm**: non-comment lines in the fighter's own source folder.
- **Routines**: distinct donor routines in status-callback slots. These need
  native ports. Remix's engine-wide `fast_fall_check` is not counted.
- **Seams**: how many of the 33 per-kind tables differ from the parent.
- **Commands**: Remix custom motion commands used. FSM is the frame-speed
  multiplier and TopN the translation multiplier; both are ported.
- **Articles**: a first source scan; S6 makes it exact.

| Fighter | Parent | Remix asm | Routines | Seams | Commands | Articles |
|---|---|---:|---:|---:|---|---|
| Falco | Fox | 576 | 4 | 12 | FSM, TopN | none |
| Ganondorf | Captain | 221 | 0 | 12 | FSM, Armour | none |
| Wolf | Fox | 1,255 | 9 | 10 | none | blaster shot |
| Bowser | Yoshi | 1,344 | 7 | 21 | TopN | 1 weapon |
| Marth | Captain | 1,503 | 12 | 30 | Goto, HitboxFGM, SwitchDir, FSM, EnvColor, RandomSFX | none |
| Roy | Captain | 846 | 15 (8 are Marth's) | 30 | Goto, HitboxFGM, FSM, SwitchDir, EnvColor | none |
| Wario | Mario | 1,625 | 18 | 23 | HitboxDir, FSM, Kinetic | item use |
| Peach | Fox | 1,795 | 16 | 28 | HitboxFGM | items (turnip) |
| Crash | Mario | 2,379 | 23 | 23 | Goto, TopN, FSM | effects |
| Lanky | Mario | 2,356 | 23 | 23 | RandomSFX, Goto, FSM | 2 weapons |
| Sheik | Captain | 2,553 | 29 | 26 | none | 2 weapons |
| Banjo | Captain | 2,379 | 28 | 26 | Goto, FastFall, YVel, EnvColor, HitboxDir | 4 weapons (eggs) |
| Sonic | Fox | 4,005 | 24 | 23 | Goto, TopN, FSM, HitboxDir | 2 weapons |
| Dedede | Captain | 3,288 | 32 | 27 | Goto, FSM | items (Gordo, Waddle Dee) |
| Metal Mario | MMario kind | 24 | 0 | n/a | n/a | none |
| Meta Knight, MRGAW, Snake | EXTRA tree | not exported yet | | | | |

Falco used 12 of the 33 table seams. The roster needs the other 21:

- ground/air neutral, up and down special action ids;
- Kirby's copied special;
- entry action and entry script, initial and grounded script;
- jab 3, and the four rapid-jab rows;
- label height, winner-name scale, Yoshi egg.

It also needs 10 more custom motion commands: the roster uses 12, and two are
ported.

## Board 1 — shared machinery (do first; each item done once, for everyone)

Build each item generically, for all consumers in the table. Where an item
can only be proven by a fighter, its first consumer in wave order proves it.
Status lives in P4_STATUS.

| # | Item | Done when |
|---|---|---|
| S1 | **One content list drives everything.** `NDS_P4_CONTENTS` (Makefile) generates the export, generator, native-owner, preview-pack and NitroFS rules per entry. A generated X-macro header replaces the hand-written per-fighter blocks: content rows in `nds_p4.c`; native-owner slots, tables and adapter lookups in `nds_renderer_assets.c`, `nds_renderer_native_common.c`, `renderer_adapter_fighter.c` and `nds_renderer.h`; profile owners; preview kinds. Per-fighter inputs: the owner pins JSON and the fighter's `nds_p4_<name>.c`. | Falco builds from the list with no `NDS_P4_FALCO` blocks left in shared code; a second entry needs no shared-code edit. |
| S2 | **Build throughput.** Lab and menu builds take an evicted copy of the hot-text linker script through `NDS_HOT_TEXT_LINKER_SCRIPT` instead of rewriting the shared file. This removes the global mutex, so lab, menu and playable builds run at once. One script builds the P4 set (lab, menu walk, playable), each seeded from its previous build. | Three ROMs build concurrently. The tracked linker script never changes during a build. |
| S3 | **All 33 per-kind table seams**, each read at its vanilla site through the content (Remix's `define_character` semantics), with generated rows. | Every differing table in the 14 exports has a seam. A generator check fails on any differing table without one. |
| S4 | **Every Remix custom motion command the roster uses** (Command.asm D0-DC). FSM (D0) and TopN (D3) are done; the roster uses 10 more: armour, hitbox direction, Y velocity, fast fall, random SFX, kinetic, hitbox FGM, env colour, switch direction, moveset goto. | `gNdsP4UnportedMotionEvents` stays 0 in every fighter's probe. |
| S5 | **Routine work list.** A generator lists each fighter's donor routines: Remix scope, file and lines, size, which fighters share it, and which status slots use it. It also lists Remix hooks into vanilla code that test the fighter's id (`OS.patch_start` blocks with `Character.id` compares, such as Falco's Fire Bird delay). The list becomes the card's checklist. Port conventions: one `nds_p4_<name>.c` per fighter, named after the Remix scopes, static asserts on status ids. A routine shared by several fighters (Marth/Roy) is ported once. | The list exists for all 14. Shared routines are marked. |
| S6 | **Articles.** Census each fighter's weapons, items and effects from the export, not guesses. Remix weapons and items become native weapon and item kinds, through the same owners the original cast's articles use. The first consumer (Wolf's shot, then Bowser) builds the path. | Census in the table above is exact. The article path draws natively with 0 declines. |
| S7 | **CPU rows from the export.** Attack lists, attack-prevent and long-range ids are generated from the assembled tables; Falco's were read by hand. Only recovery and post-process routines stay per-fighter code. | Falco's hand table is replaced by the generated one, byte-identical. |
| S8 | **Kirby copies.** Kirby records the copied content, not only the parent kind. Copy hats come from Remix's extended Kirby file as native hat images per content. Copied specials (`kirby_ground_nsp` / `kirby_air_nsp`, Kirby's own animations and scripts for the power, Remix `clone_action`) become Kirby status overrides. Falco proves it: Phantasm on Kirby, hat 0x12. | Kirby copies Falco with his hat and Phantasm. 11 of the 14 exports need this table. |
| S9 | **Select-screen preview from the export.** Pack flags (main, model, attributes, kind) come from the export, not per-fighter Makefile lines. The anim-cache reservation covers every compiled content's menu clips. | Falco's 3D preview shows on the menu ROM; a second content needs no Makefile edit. |
| S10 | **Acceptance probe.** One command per content: build check, then a four-CPU lab match on three stages. It checks native failures, unported commands, FGM misses, missing animations, override hits, motion reads after GO and heap low-water. It also captures character select and Results, runs a Kirby copy (lab-forced inhale) and a 4x mirror with four costumes. It prints the card's witness block. | Falco passes it. Its output is pasted into the card. |
| S11 | **Whole-roster generation.** Every exported fighter is generated and compiled into the lab ROM from day one. Unported routines fall back to the parent's in lab builds only, and each fallback is counted. Pipeline breakage (owner topology, files, scripts, tables) then shows up roster-wide at once, not one fighter at a time. Ship builds compile only accepted fighters. | All 14 load and stand in a lab match with 0 native failures. |
| S12 | **EXTRA export.** Initialize the EXTRA tree's nested Remix gitlink at its pin, extend `stage_remix.py`/`remix_export.py` to EXTRA, and add Meta Knight, MRGAW and Snake to the table. | The three export with 0 failures and their rows are filled. |

Order inside Board 1: S1, S2 and S11 first. They make every later step
cheaper, and S11 surfaces the real failures early. Then S3, S4, S5 and S7,
proven by Wave 1's consumers. S8 and S9 close Falco. S6 lands with Wave 2.
S12 can run whenever a build is in progress.

## Board 2 — fighters, by measured cost

Each fighter is finished before the next starts. Generation (S11) already
covers everyone, so this order only decides who gets ported routines and
polish first.

| Wave | Fighters | What the wave proves |
|---|---|---|
| 1 | Falco (close: preview, Kirby copy), Ganondorf, Metal Mario | Captain parent; armour command; entry-script and winner-scale seams; promoting an existing kind |
| 2 | Wolf, Bowser | Fox and Yoshi parents with routines; the article path; the first non-Fox native topology; Yoshi egg seam |
| 3 | Marth, then Roy | Captain sword family; shared routines ported once; goto, hitbox-FGM, env-colour and switch-direction commands; rapid jab |
| 4 | Wario, Peach, Crash, Lanky | Mario parent; item articles; kinetic, hitbox-direction and random-SFX commands |
| 5 | Sheik, Banjo, Sonic, Dedede | The largest routine sets; egg, needle and Gordo/Waddle Dee articles; Y-velocity and fast-fall commands |
| 6 | Meta Knight, MRGAW, Snake | EXTRA tree (S12); MRGAW facing transforms; Snake's article families |

The order follows routine count and asm volume, keeping families together.
Revision 2 put Bowser and Meta Knight early as canaries. S11 and S12 now
retire that risk without porting either early.

## Per-fighter checklist (the definition of done)

A fighter is accepted when every line holds. The S10 probe checks lines 1 and
4-8.

1. Export and generator: 0 failures; every differing table has a seam (S3);
   every command it uses is ported (S4).
2. Every routine on its S5 list is ported natively, including Remix hooks in
   vanilla code that test its id. No parent fallback remains.
3. Its articles (S6) are native, with source lifetimes and caps.
4. Native owner, high and low detail: 0 declines, 0 native failures; all
   costumes; four mirrors keep independent state.
5. Presentation, all generated: HUD stock and emblem; Results name, pose,
   announcer and music; character-select cell, name, emblem and 3D preview;
   sounds; victory BGM.
6. CPU: generated rows plus its recovery and post-process routines. The probe
   shows its attack picks and recoveries.
7. Kirby: the donor's copy outcome (hat and copied special, or the parent's
   power when the donor shares it).
8. Probe: 0 unported commands, 0 FGM misses, 0 motion reads after GO, heap
   low-water >= 25,600. Four-CPU P95 sampled on two heavy stages with the
   fighter in all slots is within the gate.
9. Card updated: witness block from the probe, approved deltas, P3 status.

The full worst-case search runs at the end of each wave, not per fighter.

## Working rules for speed

- Iterate on the lab ROM. Build the menu and playable ROMs (S2 builds all
  three at once) at each fighter checkpoint, and always before handing a ROM
  to the owner. Character-select bugs only show on the menu ROM.
- Generated output is never hand-edited. Per-fighter facts go in the pins
  JSON or the fighter's C file. Rules go in the generators.
- Port routines by reading the Remix asm next to the BattleShip source of the
  parent routine it changes. Keep it native and compact.
- When a build is running, do census, export or docs work. Never edit source
  then.
- Ask the owner only about very noticeable visual approximations and contract
  changes.
- Measure memory at each wave: the CSS arena (KEEP_FREE 128 KB, 80 KB preview
  blocks), the figatree heap (Falco raised it to 23,440) and the battle heap.
  Fix pressure with the existing residency tools before the next wave.

## Superseded

Revision 2's P4.0 checkpoint is complete (P4_STATUS sections 1-6). Its rule
"do not open the full roster until Falco closes" is replaced by S11. Its
canary ordering is replaced by the wave table. Its wave numbers on the cards
follow this revision.
