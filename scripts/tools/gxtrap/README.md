# gxtrap -- a melonDS that logs stray geometry commands

Built for the owner's fighter tint (2026-10-26 red, 2026-10-28 green): a colour
over every fighter, kept through VS Results and the character select until a
restart. The cause class is a stray FIFO command that latches the GX
SPECULAR_EMISSION register, which no port code writes. Fix `b086fb52d5e`
rewrites that register every frame. This tool hunts the command that wrote it.

`apply_gxtrap.py` patches melonDS-mp's `src/GPU3D.cpp` (default
`D:\Stuff\DevFolder\melonDS-mp`, or `--src`; `--revert` takes it out again,
byte-exact). The patch is inert unless the environment variable
`MELONDS_GX_TRAP_LOG` names a file. With it set, the log gets `GXTRAP armed`
at the first geometry write, then an `EVENT` line for each of these:

- `BAD_HEADER`: a packed header with a command the game's lists never pack
  (0x31 SPECULAR_EMISSION, 0x34 SHININESS, 0x50 SWAP_BUFFERS, or an undefined
  id). `a` is the header, `b` its source and `c` the kind: 0 for a CPU write
  (`b` = ARM9 PC), 1 for a DMA (`b` = the DMA's current source address).
- `CPU_PACKED_DURING_GXDMA` / `CPU_DIRECT_DURING_GXDMA`: a CPU write to the
  packed port or a command port while an ARM9 DMA in GXFIFO mode is enabled.
  This breaks the "whoever writes the FIFO next waits first" contract
  (`nds_renderer_preamble.c`, `ndsFighterPacketDmaWait`).
- `CPU_DIRECT_INSIDE_PACKED`: a command-port write while a packed command
  still waits for parameters.

The first twelve events also dump the last 64 FIFO writes (`ring kind=0`
CPU packed, `1` DMA packed, `2` CPU direct as cmd<<24|param; `src` is the PC or
the DMA source; `frame`).

## Build and deploy

1. `py -3 scripts\tools\gxtrap\apply_gxtrap.py`
2. Git Bash: `export PATH=/c/msys64/ucrt64/bin:/c/msys64/usr/bin:$PATH; cmake --build D:/Stuff/DevFolder/melonDS-mp/build -j 8`
   (incremental: GPU3D.cpp and the link, about a minute).
3. Copy `melonDS-mp\build\melonDS.exe` to `emulators\melonds-gxtrap\` (the lab
   scripts only run emulators under `emulators\`). Give the copy a
   `melonDS.toml` from a runner slot whose DLDI `ImagePath` and save paths
   point at its own `storage\` (copy that slot's `storage\dldi.bin` and
   `.idx` there). One directory per parallel instance.
4. `py -3 scripts\tools\gxtrap\apply_gxtrap.py --revert` and rebuild, so the
   melonDS-mp tree goes back to its own binary.

## Run

`capture_battle.ps1` (gate ROM) and `probe_session.ps1` (walk ROM) take
`-MelonPath emulators\melonds-gxtrap\melonDS.exe -TrapLog <file>`. gdb then
listens on 4380 + 2 * `-Slot`. The shared SD image repair is skipped
(`common.ps1` `Initialize-S64AltMelonDSContext`), so runs start while other
melonDS instances are up.

Positive control: two gdb writes at a frame marker,
`set *(volatile unsigned int *)0x04000400 = 0x00000031` then
`= 0x00100000`, must log a `BAD_HEADER` whose ring shows kind 1 DMA words
from main-RAM list addresses.

For the owner to play on it: `emulators\melonds-gxtrap-play\` holds the trap
build with a copy of the owner's own `melonDS.toml` and
`Play-With-GX-Trap.cmd` (drop a ROM on it; the log is `gxtrap-play.log`
beside it).

## 2026-10-28

Every run below logged no events and no GX stalls (`gNdsGxStallCount`).

- **Gate ROM `gate-p9d`, CPU-vs-CPU:** six matches, items high or very high
  (one hammers-only), up to 472 off-screen magnifier draws a match:
  - Hyrule Temple: Ike vs Fox ×2, Palutena vs Ike, Ike + Palutena + Bowser + Fox.
  - Final Destination: Palutena ×2 + Bowser + Fox.
  - Sector Z: Palutena vs Fox.
- **Walk ROM `walk-p9d`:** two runs of 6 laps (stage select, battle, Results,
  character select). The second rewrote each commit to the owner's sequence:
  Palutena vs Fox on Sector Z, then Ike vs Fox on Hyrule Temple.

Not covered yet: a human player's inputs, pause, stock-match endings, team
battles (`fuzz_vs.ps1` draws stock and team rules; it does not take
`-MelonPath` yet).
