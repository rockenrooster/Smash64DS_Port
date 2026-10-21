# Stability and reliability audit (P7, 2026-10-20)

The goal asks for every mode 100% working on every fighter: 1P/VS character
select (3D previews), the 1P campaign, both bonus practices, Training, the
stage select, any 4-player match with all items for 5 minutes (and 2-4 player
multiplayer), Results and back to the rematch. This audit drives each of them
on the current tree, every fighter, and records halts, native failures,
event-script ledger refusals, lean declines and the general heap's low-water.

## Method

Scripted emulator runs on ship-layout ROMs, one probe per runner slot (each
slot has its own SD image, so four ran side by side), gdb on each:

- **Walk ROM** (`walk-u10`, then `walk-u11`, `walk-u13` and `walk-u16` with the fixes below: `TARGET=smash64ds`
  with every content, P5 and `NDS_P2_MENU_WALK=1`, the shipping layout) drives
  the real menus: Mode Select, the 1P / bonus / Training / VS selects, the stage
  select, the battle, the interstitials.
- **Soak ROM** (`build-soak-s31`: the gate's ship-layout four-CPU target with
  `NDS_R2_SOAK_MATCH_MINUTES=5`) for the 5-minute 4-player matches. Its tick HUD
  costs 24,832 B of arena against the play ROM, so its heap low-water reads that
  much below what ships.
- At every scene entry: the scene, the heap low-water of the scene that ended
  (`gNdsTaskmanGeneralHeapFreeMin`, reset per scene), native failures
  (`gNdsRendererNativeFailure`), lean declines, event-script refusals. Breakpoints
  on the data-abort vector, the malloc-overflow halt and the first native failure
  (with a backtrace).

Scratch drivers: `audit/campaign.ps1`, `soak.ps1`, `bonus.ps1`, `training.ps1`,
`vswalk.ps1`, `mm.ps1`; multiplayer `tools/mplag.ps1` (two linked consoles,
melonDS-mp).

## Results

### 1P campaign, every fighter

The walk plays the whole ladder (route 1) from stage 1 through Master Hand,
the Congratulations screen and the staff roll, 9 minutes a run, on
`walk-u13` (fixes 1 and 2 below); Sonic, King Dedede and Mr. Game & Watch
again on `walk-u15` (fix 5). The walk's player is a level-9 CPU: the four
"time cap" rows lost to Master Hand, continued and were still fighting him
when the run ended.

| fighter | reached | lowest heap (stage) | native failures |
|---|---|---|---|
| Mario | staff roll (completed), 1 continue | 26,120 (Yoshi Team) | 0 |
| Luigi | Congratulations | 30,744 (Yoshi Team) | 0 |
| Donkey Kong | Congratulations | 29,288 (Yoshi Team) | 0 |
| Link | Congratulations | 37,552 (Yoshi Team) | 0 |
| Samus | Congratulations | 30,416 (Yoshi Team) | 0 |
| Captain Falcon | Congratulations | 45,704 (Yoshi Team) | 0 |
| Ness | Congratulations | 33,536 (Yoshi Team) | 0 |
| Yoshi | Congratulations | 31,256 (Yoshi Team) | 0 |
| Kirby | Master Hand, 1 continue (time cap) | 42,000 (Polygon Team) | 0 |
| Fox | Congratulations, 1 continue | 38,480 (Yoshi Team) | 0 |
| Pikachu | Congratulations | 28,864 (Yoshi Team) | 0 |
| Jigglypuff | Master Hand, 1 continue (time cap) | 34,208 (Yoshi Team) | 0 |
| Metal Mario | Master Hand, 2 continues (time cap) | 50,712 (Yoshi Team) | 0 |
| Falco | Congratulations | 45,736 (Giant DK) | 0 |
| Ganondorf | Congratulations | 32,624 (Giant DK) | 0 |
| Wolf | Congratulations | 43,644 (Polygon Team) | 0 |
| Bowser | Congratulations | 32,500 (Giant DK) | 0 |
| Marth | Congratulations | 33,452 (Yoshi Team) | 0 |
| Roy | Congratulations | 41,296 (Yoshi Team) | 0 |
| Wario | staff roll (completed), 1 continue | 36,996 (Giant DK) | 0 |
| Peach | Congratulations | 35,300 (Yoshi Team) | 0 |
| Crash | Congratulations | 44,344 (Yoshi Team) | 0 |
| Lanky Kong | Congratulations | 33,004 (Giant DK) | 0 |
| Sheik | Congratulations | 37,496 (Yoshi Team) | 0 |
| Sonic | Congratulations | 34,152 (Polygon Team; Yoshi Team 24,584 before fix 5, 42,440 after) | 0 |
| King Dedede | Congratulations | 40,960 (Giant DK; Yoshi Team 27,948 before fix 5) | 0 |
| Mad Piano | Congratulations | 46,232 (Yoshi Team) | 0 |
| Meta Knight | Master Hand, 2 continues (time cap) | 36,780 (Yoshi Team) | 0 |
| Mr. Game & Watch | Congratulations | 45,404 (Yoshi Team; 27,516 before fix 5) | 0 |
| Snake | Congratulations | 42,744 (Polygon Team) | 0 |
| Ike | Congratulations | 33,352 (Yoshi Team) | 0 |
| Palutena | Congratulations | 37,856 (Polygon Team) | 0 |

The Yoshi Team is the tightest stage for most rows above (`walk-u13`, the
lean caches' 12 KB margin); fix 5 lifts every row's Yoshi Team low by about
12 KB.

### 4-player matches, 5 minutes, every item at Very High

Four CPUs, every fighter once, a stage each; every match ran its 5 minutes
(9,243 presented frames) to Results (two through Sudden Death first).

| match | stage | fighters | battle heap low-water (soak ROM) | native / lean / ledger |
|---|---|---|---|---|
| m1 | Peach's Castle | Mario, Fox, Donkey Kong, Samus | 46,296 | 0 / 0 / 0 |
| m2 | Sector Z | Luigi, Link, Yoshi, Captain Falcon | 34,348 | 0 / 0 / 0 |
| m3 | Congo Jungle | Kirby, Pikachu, Jigglypuff, Ness | 38,048 | 0 / 0 / 0 |
| m4 | Planet Zebes | Falco, Ganondorf, Wolf, Bowser | 40,792 | 0 / 0 / 0 |
| m5 | Hyrule Castle | Marth, Roy, Wario, Peach | 28,568 | 0 / 0 / 0 |
| m6 | Yoshi's Island | Crash, Lanky Kong, Sheik, Sonic | 24,056 | 0 / 0 / 0 |
| m7 | Saffron City | King Dedede, Mad Piano, Meta Knight, Mr. Game & Watch | 25,200 | 0 / 0 / 0 |
| m8 | Mushroom Kingdom | Snake, Ike, Palutena, Mario | 41,464 | 0 / 0 / 0 |

m6 and m7 dip under 25,600 on the soak ROM. That ROM starts a battle 24,832 B
poorer than the shipping layout -- m6 had 43,856 B free at its first frame,
too little for any lean spare -- and the source's object pools then took
19,800 B over the five minutes (issue 5). In the shipping layout those 24.8 KB
go first to the lean caches, which leave 25,600 + 24 KB, so the same match
bottoms out near 31 KB.

### Bonus practices, every fighter

Break the Targets and Board the Platforms through the native bonus select, a
fresh SD image, the twelve, Metal Mario and the nineteen contents in turn: all
64 boards loaded, played and returned to their select with 0 native failures;
lowest board heap 86,628 (Jigglypuff's Board the Platforms). The run met the
source's unlocks on the way (Luigi's challenge after the eighth starter's
targets, then his fight and message, back to the practice; the Sound Test's
message and the return to the title, as the source routes it), and found the
bug below. Each content's 3D preview drew on its select
(`artifacts/visibility/p7-walk/bonus-select-sheet.png`).

### Training, every fighter

Every fighter (the twelve, Metal Mario, the nineteen contents) through the
Training select, each on a different VS or P5 stage with the next fighter as
the CPU, out to the Training select again: 32 of 32, 0 native failures,
lowest heap 35,480 (Mr. Game & Watch on Dinosaur Land).

### Multiplayer

Two linked consoles (melonDS-mp), the lobby autopilot through VS Mode, the
shared character select, the host's stage select, the match, Results and the
rematch:

- Four 5-minute matches of two humans and two level-9 CPUs, the four heavy
  content mixes in turn (Bowser, Crash, Ike, Dedede; Sonic, Lanky, Meta
  Knight, Wolf; Ganondorf, Peach, Snake, Kirby; Marth, Roy, Sheik, Mr. Game &
  Watch), every item at Very High, Hyrule Castle and Sector Z: 0 desyncs,
  0 aborts, heap low-water 31,100 (`NDS_NET_LAB_SWEEP_REPRO=3`).
- Three 1-minute matches (the sweep): both consoles load the selected stage in
  150-153 VBlanks (2.5 s) and start within a frame of each other. The owner's
  "client lags the host loading the stage" is not in the game
  (docs/BUG_NOTES.md).

The first stress run stopped at the character select after its first match:
the stress descriptors play contents a fresh save keeps locked, and the select
cleared those picks on the way back, so the host's START never took. Lab
builds of the autopilot now see every fighter open (`ndsP7DevOpen`), as the
other lab builds do; a player can only pick an unlocked fighter.

## Issues found

1. **A content's records counted as its parent's in the source's unlock tests**
   (fixed, `fb1b096b36d`). During a content's or Metal Mario's practice or
   campaign run the parent's 1P row holds the player's (P7 C3), so the source's
   tests over every fighter's row read the content's clears as the parent's:
   the bonus pass opened the Sound Test on Falco's Board the Platforms while
   Fox's own board stood at 8 of 10. The Sound Test's test now runs with the
   parent's own row in place; Luigi's challenge and Mushroom Kingdom's message
   are withdrawn when only the player's row supplied the parent's clear.
   Witnessed both ways on `walk-u11`.
2. **Heavy contents ran the campaign's Giant Donkey Kong stage 12-13 KB from
   the heap's end, and Bowser's ending room went undrawn** (fixed,
   `ce34551ba4c`).
   Ganondorf's low-water there was 12,900 and Bowser's 12,592 (the first
   battle frame of Bowser's: 14,980), against 39,800-104,128 for the twelve
   and the other contents so far; Bowser's ending movie then logged 25,992
   native failures (STAGE, NO_PROGRAM, MVCommon root 0x5A18): the room's
   baked table (`room_baked.bin`, 22 KB plus a 16 KB margin) is admitted only
   with that much heap free after the ending fighter's files, and Bowser's
   left less, so the desk, books, lamp, pencils and tissues drew nothing.
   Both are the same shortage: every 1P battle kept the overlay's whole 73 KB
   1P head resident, of which a stage runs only its own part. The head is now
   five output sections -- the core (the manager and the 1P Game, 15,040 B),
   Giant Donkey Kong's motion table (2,656), the Fighting Polygon Team's
   (30,880), Master Hand's code and table (16,384), the bonus boards' and Race
   to the Finish's code (8,672) -- and a battle lends every part but its own
   (58,592 B more on an ordinary stage; a second loan range covers the parts
   before a kept one; Race to the Finish runs Polygons, so it keeps their
   tables beside its own code); the ending lends all five. The continue
   screen keeps them -- the manager goes from it straight back into the stage
   it continues, and the first try (lending there too) data-aborted Wario's
   Master Hand retry. After the fix: Ganondorf 32,624 and Bowser 32,500 at
   Giant Donkey Kong, Bowser's ending 0 native failures, Wario's forced
   Master Hand loss continued and won.
3. **Heap under the floor in two soak mixes on the lab ROM** (m6, m7 above):
   the soak ROM's 24.8 KB less arena plus the pool growth of issue 5; in the
   shipping layout the lean caches leave the floor and 24 KB.
4. **Walk tooling** (not the game): the bonus walk boards 8-9 of Fox's ten
   platforms; Metal Mario's walk portrait is his kind (13), so the first
   campaign and bonus passes ran him as Mario (re-run in `mm.ps1`).
5. **Long battles ran the heap under the floor after the lean caches took
   their share** (fixed, `027a3a964b0`). Sonic's campaign
   Yoshi Team bottomed at 24,584. A census of every allocation from that
   battle's first frame: Sonic's deferred entry plane (75 KB), the lean
   texture memo, the lean pool (107 KB, six slots) and four lean spares
   (17.7 KB each) took the heap down to the caches' floor, 25,600 + 12 KB;
   then the source's own pools (GObjs, DObjs, XObjs, MObjs, SObjs, processes)
   grew 13,784 B more as the Yoshis came in and at COMPLETE!. A five-minute
   four-player match grows the same pools 14,780 B with items at the default
   rate and 19,800 B at Very High (Yoshi's Island, Crash, Lanky Kong, Sheik,
   Sonic): the 12 KB margin set on 2026-10-18 was a one-minute match's. The
   lean caches now keep 24 KB: Sonic's Yoshi Team low 42,440, Dedede's
   45,804 (was 27,948). The gate pays nothing measurable for it
   (`build-sweep-s34` against s27: cast Hyrule Temple P95 1,216,064 vs
   1,215,616, Saffron 1,215,552 vs 1,214,720, Ike mirrors 1,300,096 vs
   1,303,872) and every replay digest holds (four Bowsers differ from s27
   since the owner's p7h balance, `c9228f1817b`; s31 and s34 match).

## Still open against the goal

Owner answers, 2026-10-20 (`docs/OWNER_QUESTIONS.md`): the victory end poses
were never in question (the 1P card was: c4, shield rolled, `8e37a1646e7`);
the up smash column stays at the drawn pillar; the fan speed is the source's;
the wireless lag was seen on two DS units with different SD cards; the old
machinery goes (`docs/OLD_MACHINERY_DELETED.md`).

- **Motion reads after GO.** The owner replaced "0" with: a motion read after
  GO never pushes a frame past two VBlanks. A four-fighter match still reads
  clips from the card (361 misses in a 1,960-frame cast match on Hyrule
  Temple: four movesets do not fit the heap's motion cache); the gate ROM
  rings each frame's storage wait (`STRD`, `e383dd324d6`) and
  `scripts/p7/check_motion_read_push.py` counts the frames a read pushed past
  two VBlanks. `build-sweep-s35`: cast on Hyrule Temple, Saffron, Sector Z
  and Dream Land, Ike mirrors on Hyrule Temple, Bowser mirrors on Saffron --
  203-300 frames a match with a read, 19-88 frames past two VBlanks, 11-48 of
  those with a read, **0 pushed by it** (each would be late without its
  read). Met on those six; the worst-case sweep carries it from here.
- **Four-CPU WORK P95** (docs/optimization/P7_P4_ROSTERS.md): the owner set the
  cast gate at 1,250,000 (2026-10-20; >=95% within two VBlanks kept), which
  the cast meets on every stage (1,232,640 at worst, Hyrule Temple); the heavy
  4-identical mirrors are still over their 1,250,000 / 90% target.
