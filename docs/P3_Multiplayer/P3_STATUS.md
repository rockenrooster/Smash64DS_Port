# P3 Local Multiplayer: Implementation Status

The plan is `Smash64DS_Multiplayer_Plan.md` (owner document). This file
records what exists, how it is tested and what is still open. Started
2026-10-06 on the owner's decision to begin P3 before the P2 P95 pass.

## What is built

| Piece | Where | State |
| --- | --- | --- |
| Radio (transport T1) | `src/nds/arm7/nds_net7_module.c`, `nds_net_arm7.c`, `linker/nds_arm7_net7.ld`, `include/nds/nds_net_link.h` | Works in the two-instance harness: 0 loss over 380 exchanged packets |
| ARM9 link layer | `src/nds/net/nds_net_link.c` | Loads the radio module, starts/stops it, moves packets through the rings |
| Lockstep | `src/nds/net/nds_net_session.c`, seams in `src/nds/r2/nds_r2_battle.c` | Full 2-minute match on two consoles, identical Results, 0 desyncs over 2,124+ compared batches |
| Lobby protocol | `src/nds/net/nds_net_lobby.c`, `include/nds/nds_net_lobby.h` | Rooms, join, snapshots, proposals, start handshake, rematch |
| Lobby UI | VS Mode (X host, Y join), `src/nds/net/nds_net_ui.c` (lower screen), the character select (`nds_menu_shell_css.c`) and stage select | Two and four consoles, unattended: host + joins, picks, stage, match, identical Results, back to the lobby |

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
last frame stays on screen. Records travel in the existing controller
playback pads, so the source's own input pipeline derives the edges. Only the
host's START reaches the game, so only the host can pause. After every batch
the replay digest (`src/port/nds_replay_digest.c`, now in every ROM) is folded
and compared with each peer's. The seed is installed at the VS battle scene
start, before its setup draws a random number. Ten seconds without progress
ends the lockstep.

### Lobby

VS Mode: X toggles hosting, and VS START then opens a room. Y lists nearby
rooms; A joins one. The character select is the lobby. Each console's cursor
sits on its own port. The host owns CPU slots, kinds, mode, levels and START,
and each guest owns its token and its team. The host publishes all four slots
and guests propose their own, both every 6 frames. Remote cursors are not
drawn yet; remote tokens move. When everyone has picked, the host's START
goes to the stage select, where guests wait. The host's stage confirm sends
the descriptor (`NdsMatchConfig`, transfer-state handicaps) and the seed. Every
guest acknowledges, then every console enters the same battle. Results return
everyone to the lobby for a rematch. Players appear on the lower screen by
firmware name.

## Testing

- `melonDS-mp`: an export of the owner's melonDS-Accurate `master` (the build
  the runner slots use) plus a frontend patch. `MELONDS_MP_INSTANCES=N` opens
  N in-process instances of the ROM, which share emulated wifi.
  `MELONDS_NO_ACTIVATE=1` keeps every window minimized and never activated.
- Lab builds: `NDS_NET_LAB_PING=1` (radio exchange at boot),
  `NDS_NET_LAB_MATCH=1` (two consoles agree a fixed match at boot), and
  `NDS_NET_LAB_LOBBY=1` (an autopilot drives the real menus through host,
  join, pick, stage, match, Results and rematch). `NDS_NET_LAB_INPUT=1`
  scripts each console's battle pad.

## Evidence (2026-10-06, melonDS-mp, lab autopilot)

| Run | Consoles | Result |
| --- | --- | --- |
| `ping1` | 2 | 600 sent each; every packet of the 380-frame overlap received, 0 gaps, 0 TX errors |
| `match1` | 2 | Fixed match from boot: 2,124 batch digests compared, 0 desyncs; identical Results (Mario wins, KOs 2/0, TKO 7/8) |
| `lobby1`/`lobby2` | 2 | Title to battle through the real menus in 9-20 s: 1 join, 1 start, 0 desyncs; identical Results (DK wins); both back in the lobby |
| `lobby4` | 4 | 3 joins, human mask 0xF, ports 0-3; 784-1,432 digest compares per console across 3 peers, 0 desyncs, 0 bad packets; all four back in the lobby after Results |

## Open

- Player-number art for remote humans: their panel tag, token and cursor still
  use the 1P images (the UI kit bakes only 1P and CP); remote cursors are not
  drawn; guest costume choice; a guest also sees the READY/PRESS START banner.
- Waiting indicator during a stall; a clean abort to the lobby when a player
  drops mid-match; lid-close policy.
- Determinism over the worst-case set: every stage, items on, 3-4 humans plus
  CPUs, Kirby copies, sudden death (plan section 7.4). The setup handshake
  (arena geometry, tick-0 digest) is not implemented yet.
- Build identity check between consoles.
- Hardware validation on DS and DS Lite: the harness proves the protocol, not
  real radio timing.
