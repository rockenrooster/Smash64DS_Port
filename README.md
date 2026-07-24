# Smash 64 DS Port

This project combines the original game logic reconstructed by the
[BattleShip](https://github.com/JRickey/BattleShip) project with a Nintendo DS
backend. The result is a playable Nintendo DS port rather than a new Smash-style
game written from scratch.

The current build focuses on a one-minute Mario-versus-Fox match on Dream Land,
with items off. Bring your own legally obtained North American Super Smash Bros.
ROM; this repository contains no Nintendo assets or ROM-derived data.

Please support me on Patreon to help me continue this project! (slow progress for now) https://www.patreon.com/cw/Rockenrooster

## Quick start

```powershell
git clone https://github.com/rockenrooster/Smash64DS_Port.git Smash64DS_Port
# Install the Windows prerequisites listed below, then open PowerShell 7.
pwsh -NoProfile -File .\Smash64DS_Port\build.ps1 -Rom C:\path\to\baserom.us.z64
```

The script downloads pinned source dependencies, checks the ROM, regenerates the
required assets locally, and builds `smash64ds-battle-playable-hwtri.nds`.

## Current status

The game timing is designed around two game updates per displayed frame, with a
target of steady 30 fps. Stable 30 fps has not been reached: heavy combat
currently runs at about 13.5–15 fps on a real Nintendo DS. Gameplay, audio, and
visual work are still in progress.

## Prerequisites

The one-command build currently targets Windows. Install devkitPro's `nds-dev`
group and set `DEVKITPRO` and `DEVKITARM`, then install Git, Python 3, CMake,
GNU Make, PowerShell 7, and Visual Studio C++ Build Tools with a Windows SDK.

These are the exact versions used for the reference build. Other versions may
work, but they may not reproduce the same ROM byte for byte.

| Component | Reference version |
| --- | --- |
| PowerShell | 7.6.3 |
| Git | 2.54.0 |
| Python | 3.10.11 |
| CMake | 4.3.3 |
| GNU Make | 4.4.1 |
| Visual Studio | Community 2026 18.7.4 |
| devkitARM | r67.1-1 |
| ARM GCC / binutils / newlib | 15.2.0 / 2.45.1-2 / 4.6.0.20260123-5 |
| devkitARM rules | 1.6.0-4 |
| libnds | 2.0.2-1 |
| calico | 1.2.0-1 |
| ndstool | 2.3.1-1 |
| devkitPro general-tools | 1.4.4-1 |
| Ninja | 1.12.1 |

The required ROM is the big-endian NTSC-U v1.0 release (`NALE`), exactly
16,777,216 bytes with SHA-1
`e2929e10fccc0aa84e5776227e798abc07cedabf`. Byte-swapped `.v64` and
little-endian `.n64` dumps are rejected.

## Expected output

The reference toolchain produces:

- File: `smash64ds-battle-playable-hwtri.nds`
- Size: 11,428,864 bytes
- SHA-256: `4D795B4E83B335598B20A3B5953FDB1821797CC5E0A825FA96A0643ABBA4A090`

The build script prints the versions it found and clearly reports whether the
output matches this identity.

## Credits

- [BattleShip](https://github.com/JRickey/BattleShip), the Super Smash Bros. 64
  decompilation-based port that supplies the original game-code foundation.
- [ssb-decomp-re](https://github.com/VetriTheRetri/ssb-decomp-re), the pinned
  decompilation source used by the build.
- [Hydr8gon's sm64 Nintendo DS port](https://github.com/Hydr8gon/sm64), used as
  an architectural reference for Nintendo DS hosting.
- [libultraship](https://github.com/Kenix3/libultraship) and
  [Torch](https://github.com/HarbourMasters/Torch), used by BattleShip's asset
  extraction tools.
- [devkitPro](https://devkitpro.org/) and the libnds/calico contributors for the
  Nintendo DS toolchain and runtime libraries.

## Legal

You must supply your own legally obtained Super Smash Bros. cartridge dump.
Nintendo game assets, extracted archives, ROMs, and generated ROM-derived files
are deliberately excluded from this repository and ignored by Git. The build
creates them only on your computer.

Super Smash Bros., Nintendo 64, and Nintendo DS are trademarks of their
respective owners. This is an unofficial fan project and is not affiliated with
or endorsed by Nintendo or HAL Laboratory. See [NOTICE.md](NOTICE.md) for source
provenance and third-party notices.
