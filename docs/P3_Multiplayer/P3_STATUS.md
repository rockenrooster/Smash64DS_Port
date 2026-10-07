# P3 Local Multiplayer: Implementation Status

The plan is `Smash64DS_Multiplayer_Plan.md` (owner document). This file
records what exists, how it is tested and what is still open. Started
2026-10-06 on the owner's decision to begin P3 before the P2 P95 pass.

## What is built

| Piece | Where | State |
| --- | --- | --- |
| Radio (transport T1) | `src/nds/arm7/nds_net7_module.c`, `nds_net_arm7.c`, `linker/nds_arm7_net7.ld`, `include/nds/nds_net_link.h` | Works in the two- and four-instance harness: 0 loss over 380 exchanged packets |
| ARM9 link layer | `src/nds/net/nds_net_link.c` | Loads the radio module, starts/stops it, moves packets through the rings; no sleep while the radio is up |
| Lockstep | `src/nds/net/nds_net_session.c`, seams in `src/nds/r2/nds_r2_battle.c` | Whole matches with 0 desyncs: 4 consoles (4 humans), and 2 consoles with 2 CPUs and every item on all nine VS stages (evidence below) |
| Sync check | `src/nds/net/nds_net_session.c`, `src/port/nds_replay_digest.c` | Batch digests compared; a difference, a setup difference or 10 s without progress ends the match as NO CONTEST with a reason |
| Build identity | `scripts/nds_net_build_id.py`, the ROM recipe in `Makefile`, `nds_net_lobby.c` | `net/build.id` (8 bytes of SHA-256 over the ARM9/ARM7 load images and NitroFS); rooms of another build are listed but refused |
| Lobby protocol | `src/nds/net/nds_net_lobby.c`, `include/nds/nds_net_lobby.h` | Rooms, join, 30 Hz snapshots and proposals (cursors included), start handshake, rematch |
| Lobby UI | VS Mode (X host, Y join), `src/nds/net/nds_net_ui.c` (lower screen), the character select (`nds_menu_shell_css.c`) and stage select (`nds_menu_shell_sss.c`) | Every human's cursor, token and 1P-4P art on every console; costumes; guests watch the stage select |

### Radio

Calico's own Mitsumi driver (`libcalico_ds7`'s mwl objects, not vendored) runs
in infrastructure mode under the game BSSID `02:53:36:34:44:53` with no access
point. Every console broadcasts data frames marked FromDS, which that mode's
receive filter accepts, and the module marks the station associated so the
driver delivers them. One radio hop between any two consoles.

The module (code + BSS, 21 KB) lives in the homebrew bootstub area at
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

After every batch the replay digest (`src/port/nds_replay_digest.c`, in every
ROM) is folded and compared with each peer's. The first batch's digest also
carries the general heap's size and how much of it setup used (they decide the
source's GObj latch), so consoles whose arenas differ stop at once. A
difference ends the match within a few batches, naming the first tick ("Out
of sync at tick N" or "The consoles' setups differ"). Ten seconds without
progress (thirty for the first batch, which absorbs load-time differences)
ends it as "Connection lost". Each ending is the source's reset: Results shows
NO CONTEST and everyone returns to the lobby.

A console in a wireless room does not sleep: closing the lid leaves the game
running (`pmSetSleepAllowed`, restored when the radio stops).

### Lobby

VS Mode: X toggles hosting, and VS START then opens a room. Y lists nearby
rooms; A joins one. A room running another build shows "(other build)" and
refuses the join, as does the host (plan 7.1). The character select is the
lobby. Each console's cursor sits on its own port, and every other human's
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
follow and watch the host's cursor. The host's confirm sends the descriptor
(`NdsMatchConfig`, transfer-state handicaps) and the seed; every guest
acknowledges, then every console enters the same battle. Results return
everyone to the lobby for a rematch; a player still on Results is not ready
until they are back. Players appear on the lower screen by firmware name.

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
  at the highest rate, one-minute time matches.

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

## Open

- Hardware validation on DS and DS Lite: the harness proves the protocol, not
  real radio timing. Four physical consoles, host rotation and a long soak are
  the release set (plan section 12).
- Pinning the arena to a fixed start and size for net matches (plan 7.2 item
  4); the setup check compares size and use, not the address.
- Reproducible builds, so players who build the same commit from their own
  N64 ROM get the same build identity (no absolute paths in the binary).
- Perturbation runs (plan 7.4): different boot environments, menu histories,
  poisoned arenas, storage delays.
- The host's unlock masks do not travel yet; a guest's own unlocks lock its
  stage-select cells (only the host chooses, so this affects the view only).
