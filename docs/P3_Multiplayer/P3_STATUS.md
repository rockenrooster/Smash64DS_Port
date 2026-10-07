# P3 Local Multiplayer: Implementation Status

The plan is `Smash64DS_Multiplayer_Plan.md` (owner document). This file
records what exists, how it is tested and what is still open. Started
2026-10-06 on the owner's decision to begin P3 before the P2 P95 pass.

## What is built

| Piece | Where | State |
| --- | --- | --- |
| Radio (transport T1) | `src/nds/arm7/nds_net7_module.c`, `nds_net_arm7.c`, `linker/nds_arm7_net7.ld`, `include/nds/nds_net_link.h` | Works in the two- and four-instance harness: 0 loss over 380 exchanged packets |
| ARM9 link layer | `src/nds/net/nds_net_link.c` | Loads the radio module, starts/stops it, moves packets through the rings; no sleep while the radio is up |
| Lockstep | `src/nds/net/nds_net_session.c`, seams in `src/nds/r2/nds_r2_battle.c` | Whole matches with 0 desyncs: 4 consoles (4 humans), 2 consoles with 2 CPUs and every item on all nine VS stages, and Team Battle in every layout (evidence below) |
| Sync check | `src/nds/net/nds_net_session.c`, `src/port/nds_replay_digest.c` | Batch digests compared (the replay digest plus the battle state's results fields); the terminal tick's digest is compared before Results; a difference, a setup difference or 10 s without progress ends the match as NO CONTEST with a reason |
| Build identity | `scripts/nds_net_build_id.py`, the ROM recipe in `Makefile`, `nds_net_lobby.c` | `net/build.id` (8 bytes of SHA-256 over the ARM9/ARM7 load images and NitroFS); rooms of another build are listed but refused |
| Lobby protocol | `src/nds/net/nds_net_lobby.c`, `include/nds/nds_net_lobby.h` | Rooms, join, 30 Hz snapshots (slots, cursors, the host's unlock mask, rules and every player's name) and proposals, start handshake, rematch, leaving a room or a running match |
| Lobby UI | VS Mode's VS START value (OFF/HOST/JOIN, `nds_menu_shell_mode_vs.c`), `src/nds/net/nds_net_ui.c` (lower screen), the character select (`nds_menu_shell_css.c`) and stage select (`nds_menu_shell_sss.c`) | Every human's cursor, token and 1P-4P art on every console; costumes and teams; every player's name and ready state and the host's rules on the lower screen; guests watch the stage select |

### Radio

Calico's own Mitsumi driver (`libcalico_ds7`'s mwl objects, not vendored) runs
in infrastructure mode under the game BSSID `02:53:36:34:44:53` with no access
point. Every console broadcasts data frames marked FromDS, which that mode's
receive filter accepts, and the module marks the station associated so the
driver delivers them. One radio hop between any two consoles.

The module replaces Calico's receive drain (`src/nds/arm7/nds_net7_mwl_rx.c`,
which defines all four of `mwl_rx.o`'s public functions so the library's copy
is never linked). Calico's drain allocated a pool buffer for every data and
management frame and queued it for a later task on the same thread, retrying
forever when the pool ran dry; only those later tasks free buffers, so a burst
larger than the pool deadlocked the radio thread, and any broadcast
deauthentication from a nearby access point reset the forced association.
Both need real-world radio traffic and never happen in melonDS: on an
original DS and a DS Lite the link went silent about 90 s into a match and the
match ended as "Connection lost" (owner, 2026-10-07, play-1007b). The new
drain reads only data frames, into one static buffer, hands each straight to
the RX ring, skips every other frame in place, and never allocates or waits;
the pool now serves TX only. `src/nds/arm7/calico_mwl_common.h` is Calico
1.2.0's internal driver header, vendored unmodified for the state layout.
On the same two consoles the new drain played through (owner, 2026-10-07,
play-1007c: "works much better now").

The module (code + BSS, 14 KB; 21 KB with Calico's drain and RX pool) lives in the homebrew bootstub area at
`0x02FF4000`; the packet rings are at `0x02FFA000`. That area is uncached on the
ARM9 and above the ARM9 heap, so the arena is unchanged. The ARM9 loads
`net/net7.bin` from NitroFS only when a session starts, so offline play never
touches the radio. Loading it overwrites the loader's return stub, so a
console that has used wireless cannot exit back to its loader menu.

### Lockstep

Batch b is ticks 2b and 2b+1 on every console. At each batch gate a console
samples its pad once, records it for both ticks of batch b+2 (input delay: 2
batches, about 67 ms), broadcasts its recent records, and runs the batch only
once it holds both ticks for every human port. While waiting nothing runs; the
last frame stays on screen, and after 20 VBlanks the lower screen says so.
Records travel in the existing controller playback pads, so the source's own
input pipeline derives the edges. Only the host's START reaches the game, so
only the host can pause. The seed is installed at the VS battle scene start,
before its setup draws a random number.

After every batch the net digest is folded and compared with each peer's: the
replay digest (`src/port/nds_replay_digest.c`, in every ROM) plus what the
match's end and Results read from the battle state -- game status, the timer,
and each player's stocks, place, KOs, falls, self-destructs, damage tallies,
combo counts and stale-move queue -- and plan 7.3's wider coverage: the
stage's moving collision (every yakumono transform and speed, which
mpcollision reads) and the hazard state each venue's ground update keys on
(Whispy's wind, the clouds, the bumper, the barrel, the acid, the tornado, the
Pokemon door, the Arwing, the scales, the POW block), and the item and weapon
fields the rules read beyond kind and position (owner, team, facing, ground
state, damage, velocity; for items also lifetime, ammo and hold/throw/pickup
state). It folds values only, never a pointer, since the arena's address may
differ between consoles; cosmetic timers stay out. Every folded field must be
set for every instance before it is read: the item structs are cleared at
creation, the weapon structs come from a pool that is never cleared (so a
weapon's `lifetime`, which only some kinds set, is not folded), and the stage
state union is cleared at every scene entry (`ndsSceneManagerEnter`): a
stage's InitAll sets only the fields it starts with (Hyrule's twister speed
and waits are first written when it moves), and the DS links every stage for
the whole run, so the rest kept the previous scene's bytes -- which differ
between two consoles whose last match ended on different ticks, as it does
when a guest leaves. Only net matches fold the extra fields, so
the replay digest every performance gate compares is unchanged. The first
batch's digest also carries the general heap's size and how much of it setup
used (they decide the source's GObj latch), so consoles whose arenas differ
stop at once. A difference ends the match within a few batches, naming the
first tick ("Out of sync at tick N" or "The consoles' setups differ"). Ten
seconds without progress (thirty for the first batch, which absorbs load-time
differences) ends it as "Connection lost". When the battle ends, every console
compares its terminal tick's digest with every peer's before Results publish a
winner (plan 7.3); a difference, or a peer that never sends one within three
seconds, ends it the same way. Each ending is the source's reset: Results
shows NO CONTEST and everyone returns to the lobby.

START pauses for the host alone, with one exception the source makes itself:
a fighter KO'd in a team Stock match takes a stock from a teammate with START
(`ftCommonSleepProcUpdate`, ftcommonsleep.c:77), and the source's pause
trigger passes that port over (ifcommon.c:2920). A guest's START is installed
exactly then (`ndsNetStartIsStockSteal`); the trigger runs before the tick's
fighters, so the state read at install is the state it reads. Otherwise a
guest's START is its Leave command: holding it for two seconds leaves the
match (the lower screen says so on the first press). Its LEAVE, like the host
closing the room, ends the match for everyone as NO CONTEST; the host drops
that player from the room and the leaver's console closes its session.

A console in a wireless room does not sleep: closing the lid leaves the game
running (`pmSetSleepAllowed`, restored when the radio stops).

### Lobby

VS Mode: the VS START button carries a value word, the way the rule button
carries TIME/STOCK -- OFF, HOST or JOIN, changed with left/right (with the same
blinking arrows) and confirmed with A/START. OFF is the ordinary VS START; HOST
opens a room and goes to the character select; JOIN holds the tab white while
the console looks for a room and joins the first open one running its own
build (another build's room is never asked, and a room that refuses -- full,
or no answer -- is skipped); B gives up. The words are set in the source's own
menu font (MNCommonFonts) at twice its size, baked into the button's states
(`generate_mn_ui_kit.py` NET_START_SURFACE_SPECS). A caption under VS Options
says what the value means (owner, 2026-10-07): "WIRELESS / MULTIPLAYER OFF",
"HOSTING WIRELESS / MULTIPLAYER ROOM", "JOINING WIRELESS / MULTIPLAYER ROOM",
white with a black drop shadow in the same font and size, two lines that
continue the buttons' staircase (NET_HINT_SURFACE_SPECS; the menu re-blits it
the frame after VS START's own state). The character select is the lobby. Each console's cursor sits on its own port, and every other human's
cursor (hand state and player tag) and token are drawn where their owner last
put them; the host publishes all four slots and each guest proposes its own,
30 times a second. The host owns CPU slots, kinds, mode, levels and START;
each guest owns its token, its team and its costume. A guest's costume press
travels as a count that the host turns into the source's costume change, and
every console shows the costume the host resolved. Remote humans use their
own 2P-4P cursor tags, tokens and panel tags: that art ships in its own 10 KB
pack (`menus/mn_ui_kit_net.bin`) and loads over two images the character
select never draws, because the OBJ bank has under 1 KB free.

When everyone has picked, the host's START goes to the stage select; guests
follow and watch the host's cursor. The host's confirm starts the match in two
phases. It offers START (the descriptor, `NdsMatchConfig` and transfer-state
handicaps, and the seed, which names the round) until every guest ACKs that
round, for up to 5 s, then sends its verdict a dozen times: GO, and every
console enters the same battle, or CANCEL, and everyone stays in the lobby
("A player did not answer."). A guest that accepted a round keeps ACKing it
and waits in the lobby for the verdict; the host's first lockstep packet counts
as GO, its next lobby snapshot as CANCEL, and 20 s of silence cancels too. An
ACK, GO or CANCEL of a withdrawn round decides nothing. Until 2026-10-07 the
guest went to battle on START itself, after one or two ACKs, so a burst of
loss over those ACKs left the host back in the lobby and the guest alone in a
match that ended NO CONTEST (owner, hardware: Yoshi vs Kirby, Sector Z).
Results return
everyone to the lobby for a rematch; a player still on Results is not ready
until they are back. The lower screen lists every player by firmware name
with "ready" once their token is down, and one line of the host's rules (rule,
time or stocks, item rate, team attack): the character select has no rule
panel of its own, and a guest never sees the host's VS menu. The host's save
unlock mask travels in every snapshot, so the stage select locks the same
cells on every console (plan 7.1); a guest's own save is never written by it.

### Team Battle, rules and items (audit against the source, 2026-10-07)

- Commit: `ndsMatchConfigApply` (`src/port/nds_match_config.c`) is
  `mnPlayersVSSetSceneData` (mnplayersvs.c:4379-4423) field for field: `player`
  is the team in Team Battle, `color` is `dIFCommonPlayerTeamColorIDs[team]`
  (Red 0, Blue 1, Green 3), `tag` is the port for humans and CP for CPUs. The
  descriptor carries all of it, plus team attack, item switch and rate,
  damage ratio, handicaps, time and stocks; a guest's own VS options never
  reach the match.
- Lobby rules: a player picks their own team, the host also picks for CPUs;
  the host resolves team costumes and shades with the source's own helpers
  (`ftParamGetCostumeTeamID`, `mnPlayersVSGetShade`); READY is refused while
  every picked fighter is on one team (`mnPlayersVSCheckReady`).
- Battle: friendly fire and team hit checks are the imported source
  (`ftmain.c`, `itprocess.c`, `wpprocess.c`, `ftcomputer.c`); the port's
  wrappers call it. Team Stock's stock steal works over the link (above).
- Presentation: player tags take the player's colour (`players[].color`,
  `nds_ifcommon_oam.c`), costumes the team costume, Results the source's team
  screen ("GREEN WINS!"). The lower HUD's damage-meter badge is the source's
  emblem (`ft_sprites->emblem`, ifcommon.c:929-951) tinted with the stage's
  emblem colour for the player's colour -- the team's in Team Battle -- behind
  the digits (`nds_battle_hud.c`, baked from the loaded sprite once per
  battle). It replaced a 16x15 CSS portrait the P2 HUD drew there. Two port
  bugs surfaced on the way: only Mario's and Fox's emblem sprites were
  byte-normalized (the other ten fighters, Metal Mario, the Donkey Kong icon
  and the Polygons' Master Hand icon are now), and a CPU's colour, which the
  source reads one past `emblem_colors` from the ground data's `unused` word
  (0xDCDCDC, light grey), read in native byte order as cyan.

### Freezes on hardware (owner, 2026-10-07, play-1007e)

"Sometimes a freeze occurs during a 2P match" (Donkey Kong against Luigi,
several stages, items on; the link also dropped and recovered by itself).
Nothing in the lockstep waits without a bound (a stall ends the match after
10 s), so a console that stays frozen is stuck outside it. Two things changed:

- **No halt on a music error.** Since the ARM7 took over BGM streaming (A8),
  the ARM9 halted the game for good whenever the ARM7 reported a stream error
  stop -- an underrun (a refill not ready by the seam), a failed storage read,
  or a full command queue -- and when the ARM7 refused a STOP or RESET
  (`ndsAudioBgmControlHalt` 5 and 4). Any of those freezes the picture and
  silences the music, and the radio adds load and SD traffic the offline game
  never had. The ARM9 now restarts the track instead (a new generation from
  the top, at most 16 times per play; `gNdsAudioBgmRecoveries`,
  `gNdsAudioBgmLastError`), and a refused STOP or RESET is counted
  (`gNdsAudioBgmCommandRefusals`); only an INIT the ARM7 rejects still halts.
  The source sets the BGM volume every tick of a timed match's last five
  seconds (`ifCommonTimerFuncRun`), 300 posts that each took a mailbox slot on
  the ARM7, and a full mailbox failed the stream: the ARM9 now posts only a
  change of the ARM7's 128-step channel volume, and the ARM7 treats VOLUME as a
  doorbell (the handler keeps the latest value, at most one VOLUME message
  waits). `scripts/sfx/test_bgm_service.py` covers a 40-post volume run and the
  queue-full failure.
- **A freeze report.** Playtest builds with `NDS_FREEZE_DIAGNOSTICS=1` arm a
  1 Hz watchdog on battle frames (from the first presented frame to the
  battle's end; the lockstep's wait for another console counts as progress).
  After 3-4 s without a finished frame it draws a report on the lower screen
  and keeps redrawing it every second: the interrupted PC/LR, the game
  thread's state and saved PC/LR/SP with up to six return addresses from its
  stack, the BGM halt/error/recovery counters, the ARM7's live BGM report
  sequence, storage requests, the net session state, batch and packets, the
  radio's frames sent and received, and the last eight breadcrumbs. It never
  halts the game: a frame that finishes marks the report RESUMED. The same
  builds print the radio's counters on line 20 while the lockstep waits, so a
  photo of each console during a dropout shows which side stopped sending or
  receiving. Lab: `NDS_NET_LAB_HANG=1|2` freezes the guest at batch 600 (a
  spin, or the game thread blocked on a mailbox) to check the report.

## Testing

- `melonDS-mp`: an export of the owner's melonDS-Accurate `master` (the build
  the runner slots use) plus a frontend patch. `MELONDS_MP_INSTANCES=N` opens
  N in-process instances of the ROM, which share emulated wifi.
  `MELONDS_NO_ACTIVATE=1` keeps every window minimized and never activated.
- Lab builds: `NDS_NET_LAB_PING=1` (radio exchange at boot),
  `NDS_NET_LAB_MATCH=1` (two consoles agree a fixed match at boot), and
  `NDS_NET_LAB_LOBBY=1` (an autopilot drives the real menus through host,
  join, pick, stage, match, Results and rematch). `NDS_NET_LAB_INPUT=1`
  scripts each console's battle pad. With the autopilot, `NDS_NET_LAB_DROP=1`
  mutes the guest's radio for 15 s in the first match, and
  `NDS_NET_LAB_SWEEP=1` runs ten matches whose descriptor the host fills in:
  the nine VS stages in turn, two level-9 CPUs (Kirby among them), every item
  at the highest rate, one-minute time matches. With the sweep,
  `NDS_NET_LAB_TEAMS=1` cycles the matches through four Team Battle layouts
  (each human with a CPU partner, team attack off; the same in four-stock
  Stock with team attack on, where the guest's scripted START steals; three
  teams; both humans against the CPUs) and a free-for-all, and
  `NDS_NET_LAB_LEAVE=N` makes the guest hold START (Leave) in its Nth battle.
  `NDS_NET_LAB_SWEEP_REPRO=N` replays a reported setup instead of the sweep
  (2: Donkey Kong host against Luigi guest, each VS stage in turn, every
  item at Very High), and `NDS_NET_LAB_HANG=1|2` freezes the guest at batch
  600 for the freeze report.
  `NDS_NET_LAB_SOAK=1` (with the sweep) plays rooms without end: the two
  consoles swap host and guest every room (host rotation), and the rooms cycle
  through a lobby-only visit (join, pick, the host closes), one match, a match
  and a rematch, and one where the guest leaves mid-match; the host closes
  every room by holding B. Every VS Mode entry samples the libc heap
  (`gNdsNetSoakHeapFirst/Last/Max`), so a per-room leak shows as growth, and
  each batch's net digest is kept after each of its parts (replay digest,
  battle state, stage, items, weapons: `gNdsNetLabParts`), so a mismatch
  names the first part that differs.
- Gate ROM: `NDS_NET_LAB_GATE_RADIO=1` compiles `gNdsNetLabGateRadio`; set
  to 1 at boot (`sample-tick-hud-buckets.ps1 -BootSetGlobals`), the
  four-fighter gate battle opens a room and runs the lockstep with the console
  as its only human, so the same ROM measures the gate with and without the
  radio.

## Evidence (2026-10-06, melonDS-mp, lab autopilot)

| Run | Consoles | Result |
| --- | --- | --- |
| `ping1` | 2 | 600 sent each; every packet of the 380-frame overlap received, 0 gaps, 0 TX errors |
| `match1` | 2 | Fixed match from boot: 2,124 batch digests compared, 0 desyncs; identical Results (Mario wins, KOs 2/0, TKO 7/8) |
| `lobby1`/`lobby2` | 2 | Title to battle through the real menus in 9-20 s: 1 join, 1 start, 0 desyncs; identical Results (DK wins); both back in the lobby |
| `lobby4` | 4 | 3 joins, human mask 0xF, ports 0-3; 784-1,432 digest compares per console across 3 peers, 0 desyncs, 0 bad packets; all four back in the lobby after Results |
| `drop3` | 2 | Guest radio muted 15 s mid-match: both end it as NO CONTEST, meet in the lobby, and play the next matches (3 matches, 3 starts, 0 start failures) |
| `sweep1` | 2 | 8 rematches, 2 humans, items off: 17,041 / 17,114 digest compares, 0 desyncs, 0 aborts, 0 record conflicts |
| `sweep2` | 2 | 9 matches in 12.7 min: all nine VS stages in turn, 2 humans + 2 level-9 CPUs (Kirby in two), every item at Very High, 1-minute time: 19,486 / 19,536 digest compares, 0 desyncs, 0 aborts, 0 record conflicts, 0 bad packets, 9 starts / 0 failures; mid-match captures show identical damage on both consoles |
| `css2` | 2 | Real-time lobby captures: 1P/2P tokens, panel tags and both cursors on both consoles; the guest's costume press reaches the host (DK costume 1 in both previews and both descriptors); PRESS START only on the host; the guest watches the host's stage-select cursor |

## Evidence (2026-10-07, team audit)

| Run | Consoles | Result |
| --- | --- | --- |
| `teams1` | 2 | 10 matches, every item at Very High: 7 Team Battle (each human with a CPU partner, team attack off; both humans against the CPUs in Stock with team attack on; three teams) and 3 FFA (one sudden death): 19,077 digest compares, 0 desyncs, 0 aborts, 10 starts / 0 failures. The guest's battle state matches the descriptor in every match (teams, colours 0/1/3/CP 4, team costumes); identical team Results on both consoles ("GREEN WINS!") |
| `repro1` | 2 | The owner's failing setup in the harness (Kirby against Yoshi on Hyrule, two humans, two-minute time, every item at Medium) with the scripted pad widened to every mapped control (grab, shield, taunt, all specials): 8 matches, 26,285 digest compares, 0 desyncs, 0 conflicts, 0 bad packets -- the game stays in step; the hardware failure was the radio driver (above) |
| `radio1` | 2 | Calico's drain replaced, the native VS START host/join: host chose HOST and the guest JOIN, the lobby and two matches ran, 4,431-4,495 digest compares, 0 desyncs; each radio about 5,050 frames sent and 5,000-5,100 received, 0 ring-full, 0 TX buffer stalls |
| `gate-radio` | 1 | Plan section 12, P3-0C: the official four-fighter gate ROM built with `NDS_NET_LAB_GATE_RADIO`, measured with the switch off and on (on: the battle opens a room and runs the lockstep as a one-human net match, so every batch pumps the radio, sends its INPUT packet and folds the net digest; 1,961 batches). WORK-H P50 744,576 -> 767,488, P95 1,050,048 -> 1,068,672, two-VBlank share 96.8% -> 96.0%, replay digest identical over all 1,960 frames: the gate stays green with the radio running (`artifacts/performance/2026-10-07_p3-gate-radio`, local) |
| `soak1`/`soak2` | 2 | `NDS_NET_LAB_SOAK` with the play-1007d digest: the rooms after a guest's Leave desynced -- Jungle at batch 297, Hyrule and Yoshi's Island at batches 0-1 -- while both consoles' setup heap size and use were identical (traced on both): the digest read bytes that depend on each console's history (the stage state union, the weapon pool's `lifetime`), which a Leave makes differ (the two consoles' last match ends on different ticks) |
| `soak4` | 2 | The same soak with the stage union cleared per scene and weapon `lifetime` out of the digest: 19 rooms and 20 matches in 25 minutes, the roles swapped every room, lobby-only rooms, rematches and four guest Leaves: 30,051 digest compares, 0 desyncs, aborts only the four Leaves; libc heap at each VS Mode entry 1,027,176 B first and 1,042,736-1,044,032 B after (no growth) |
| `race` | 2 | The owner's start race, reproduced: the guest's radio deaf for 6 s as it accepts START (lab `NDS_NET_LAB_STARTLOSS`; first by a gdb write) -- before the fix the host gave up after 3 s and went back to the lobby (1 start failure) while the guest entered the battle alone and aborted ("Connection lost", NO CONTEST). After it: a 6 s dropout withdraws the round on both consoles (host 1 failure, guest 1 cancel, nobody in battle), a 3 s dropout still starts the match from the late ACKs; then 2-3 matches, 0 aborts, 0 desyncs. A 5.5-minute soak without loss: 5 matches, 0 failures or cancels, 0 desyncs, the one abort the scheduled Leave |
| `soak5` | 2 | The same soak for 2 h 15 min: 107 rooms (each console host 53-54 times) and 110 matches over all nine VS stages, 26 guest Leaves: about 162,000 digest compares per console, 0 desyncs, aborts only the Leaves; setup heap identical on both consoles every room; libc heap at each VS Mode entry 1,027,176 B first and 1,042,464-1,045,200 B after (flat over two hours) |
| `teams2` | 2 | Same sweep with the second layout set (the four-stock Stock match pairs the guest with a CPU) and the terminal-digest check: 10 matches plus 2 sudden deaths (one in a Team Battle), 20,719 digest compares, 0 desyncs; the guest took its CPU partner's stock 5 times over the link (`ifCommonPlayerStockStealMakeInterface`, thief 1); in the tenth match the guest held START: it left, the host ended the match as NO CONTEST (1 abort), dropped it from the room (human mask 0x1) and returned to the lobby |
| `frzr` | 2 | Freeze report: the guest's game thread blocked at batch 600 (`NDS_NET_LAB_HANG=2`, DK against Luigi on Dream Land). The host waited 10 s, ended the match ("Connection lost", 1 abort) and went back to the lobby; the guest's lower screen showed the report 3-4 s after its last frame: game thread waiting (status 3), first stacked return `ndsNetBattleGate` (the blocked call), then `ndsR2HostBattlePresent`, `ndsR2BattleRun`, `syTaskmanRunTask`; BGM SEQ 0x68E then 0xA1C a few seconds later (the ARM7 still streaming), RX ring overflows climbing (nobody drained the frozen console's ring). Breakpoints on the trip marker and the renderer confirmed one trip and a redraw each second |

## Open

- Hardware validation: an original DS and a DS Lite play through (owner,
  play-1007c). Four physical consoles, host rotation and a long soak on
  hardware are the rest of the release set (plan section 12).
- Pinning the arena to a fixed start and size for net matches (plan 7.2 item
  4); the setup check compares size and use, not the address.
- Reproducible builds, so players who build the same commit from their own
  N64 ROM get the same build identity. The binary holds no repository path,
  but libnds's inline asserts embedded the devkitPro install path
  (`C:/devkitPro/libnds/include/nds/arm9/sprite.h`, `background.h`,
  `videoGL.h`), so a toolchain installed elsewhere gave another identity: the
  compiler now maps that prefix to `devkitPro` (`-ffile-prefix-map`). A
  second build of the same commit on another machine is still to be
  compared.
- Perturbation runs (plan 7.4): different boot environments, menu histories,
  poisoned arenas, storage delays.
- The net digest folds no AI state (plan 7.3 adds it only if a divergence
  ever escapes the current fields).
- The lifecycle soak ran 2 h 15 min and 107 rooms in the harness (`soak5`,
  above); the same soak on hardware is part of the release set.
