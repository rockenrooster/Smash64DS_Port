# Smash64DS 1.0: one-script clean build (plan)

**Status:** DRAFT 2026-10-28. Planning only: nothing was built, installed or
changed for it. Owner decisions D1-D7 are open. Bug fixing (P8) continues in
parallel; this plan's builds wait for it (one build writer at a time, and no
release build during perf measurements or owner playtests on this PC).

## Goal

From an empty folder (`D:\Stuff\DevFolder\Smash64DS_Port_Clean\` for the dry
run) on Windows 10/11, a user with this repository, its submodules and their
own `baserom.us.z64` runs **one PowerShell script**. It installs whatever
prerequisite is missing and produces the full `smash64ds.nds` (today's play
ROM: `builds/play-p9d/smash64ds.nds`, 130,932,736 B).

Done when: two clean builds (different folders, one on a fresh Windows) give
the same SHA-256, which is pinned in `DECOMP_PIN.txt`. Byte identity is a
hard requirement, not polish: the wireless build id
(`scripts/nds_net_build_id.py`) hashes the ARM9/ARM7 load segments and every
NitroFS file, so two players can only play together on identical ROMs.

## What exists today (measured 2026-10-28)

| Piece | State |
|---|---|
| `build.ps1` (2026-09-05) | Stale and broken. It calls `scripts/generate_battle_playable_static_textures.py` (renamed to `generate_battle_static_textures.py`) and checks the old output names. It builds bare `TARGET=smash64ds`: no `NDS_P4_*`, no `NDS_P5`, so no Remix fighters and no new stages. It never stages Remix/EXTRA and installs nothing. It needs MSVC + CMake to build Torch. |
| Release flag set | `$S64PlayFlags` in `scripts/tools/common.ps1`: 19 `NDS_P4_<NAME>=1` + `NDS_P5=1` on `TARGET=smash64ds` (D7). |
| Submodules | `decomp/smashremix` @ `5e04fe7` (2.0.1, about 88 MB), `decomp/smashremix-plus-extra` @ `96621af` (about 17 MB). Neither upstream repo has a license. |
| ROM to O2R | Torch (BattleShip `ExtractAssets`, CMake + MSVC + vcpkg), 2,159 files. **The Python writer in `scripts/p4/remix_rom.py` is byte-identical to Torch for 2,132/2,132 reloc files** (`verify_vanilla_o2r`, run 2026-10-28). Not covered yet: 8 `audio/`, 18 `particles/`, 1 `version`. |
| ROM to relocData | splat (spimdisasm, rabbitizer, pygfxd, n64img, crunch64, ...) + `scripts/extract-battleship-relocdata.py`, 3,130 files. |
| BGM / FGM renders | `build.ps1` step 5 only (no make rule). |
| Remix + EXTRA staging | Manual: `scripts/p4/stage_extra.py --rom ... --dest builds/p4-staging/extra-96621af --python <venv>`. Uses the donor's xdelta.exe + bass and EXTRA's `character_appender.py` (pyyaml, lineinfile, pillow, pandas; its Pipfile is unpinned and adds pyside6). No make rule. |
| Everything else | make (generators, P4 export, P5 stages, ROM, banner icon). |
| Local-only inputs that make cannot rebuild | `assets/p6-art/` (Palutena/Ike), Ultimate motions in `../ssbu-recomp/build` (P6 clips), Ultimate WAVs (P6 audio), `assets/intro/*.s1i` (432 baked stills), `assets/congra/c19.png`, `c20.png` (owner art). |
| Toolchain on this PC | devkitARM r67.1-1, devkitarm-gcc 15.2.0-7, binutils 2.45.1-2, newlib 4.6.0.20260123-5, libnds 2.0.2-1, calico 1.2.0-1, ndstool 2.3.1-1, devkitarm-rules 1.6.0-4, general-tools 1.4.4-1. |
| Earlier publish tooling | `scripts/publish/audit_minimal.ps1` (leak audit) and the July clean-room identity gate (`docs/publish/`). Neither has run since the 2026-09-15 decomp sync. |

## Roadblocks

### R1 (blocker): Palutena and Ike cannot be built from repo + N64 ROM, and their data comes from rips

**2026-10-29:** the build no longer reads the rips. The Ultimate-derived
clips and cues are stored once under `assets/p6-art/<name>/` (frozen;
`docs/P6/P6_STATUS.md`), and the rebuilt play ROM is byte-identical. The
stored files are still rip-derived and local, so R1 stands until the owner's
authored replacements land ([P6_AUTHORING_LIST.md](P6_AUTHORING_LIST.md)).

The project's own records (`docs/P6/P6_PLAN.md` D1/D5-D10, the P6 receipts)
show:

- **Palutena's model** is the owner's decimation of the Smash 3DS kit model
  (`decomp/alt_assets/Fighters/Super Smash Bros 3DS/Palutena/`), hand-tuned to
  428 triangles. Its textures are baked from the kit's albedo
  (`assets/p6-art/palutena/3ds2ds/`, "kit-derived, local"). Wings, gate,
  portrait and stock icons are also from the kit (D10, A9).
- **Clips:** 134 of Palutena's 138 recipes and 46 of Ike's 47 are retargeted
  at build time from Ultimate `.nuanmb` motions (`scripts/p6/ultimate.py`,
  default root `../ssbu-recomp/build`, plus the Ultimate DAE skeleton).
- **Audio:** voices, effects, announcer calls and cheers are Ultimate's
  (D5-D8; `scripts/p6/*/audio.json` names the owner's local WAVs).
- **Ike's model** is Roy's (Remix) recoloured, which regenerates from the ROM.
  Only his clips and audio are blocked.

Consequence: a clean folder has none of these inputs, so neither fighter
builds. Committing or bundling them would distribute derivatives of Nintendo
assets, which D1/D10 and the README rule out. (This is a record of where the
bytes came from, not legal advice.)

Options:
- **A (recommended for 1.0):** release without them (`NDS_P4_IKE=0`,
  `NDS_P4_PALUTENA=0` in the release set). The code stays; local play builds
  keep them.
- **B:** clean re-author. Palutena on Track R (Peach restyled, which
  regenerates from ROM + Remix) or a model built from scratch; hand-made
  textures; clips keyed with the existing keypose tools instead of Ultimate;
  audio from Remix/N64 sources or new recordings. Weeks of work.
- **C:** a "bring your own kit" step. Not practical: it needs Smash 3DS and
  Ultimate dumps, and the hand-tuned `.blend` edits would have to become
  committed ops.

### R2 (blocker, partly fixed): ROM-derived data tracked in the public repo

- **Fixed 2026-10-28 (`6322c0a0140`, pushed):** `decomp/alt_assets/` (2,169
  ripped SSB64 files, tracked by accident since `75f7f6b4b48`) is untracked and
  the whole folder is ignored; the files stay on disk. History was not
  rewritten (owner's call), so old commits still hold them.
- About 1.26 MB of tracked `*.generated.*` files hold ROM-derived geometry or
  tables, for example `src/nds/nds_entry_effects.generated.inc` (1,692
  vertices of fighter props), `src/nds/dreamland_ds_mesh.generated.inc` and
  `include/nds/generated/nds_native_fighter_image.generated.h` (588 KB). Three
  are dirty in the tree right now, so builds rewrite tracked files.
- `decomp/sm64/lib/PR/**/*.bin` (Nintendo microcode from the upstream sm64
  decomp) is a reference-only file, not a build input.

Left: generate the ROM-derived `*.generated.*` files into the build directory
from the user's ROM, then rerun the leak audit.

### R3 (blocker): 1P intro stills need an emulator bake

The 432 `assets/intro/*.s1i` stills come from `scripts/menus/bake_1p_intro_stills.ps1`.
It builds a bake ROM (`NDS_1P_INTRO_BAKE=1`), boots it once per still in melonDS
runner slots with gdb, then packs the captures. `mk/rules_link.mk` stages
`$(wildcard assets/intro/*.s1i)`, so a clean build **silently ships no intro
stills**. melonDS is not in the repo (`emulators/` is ignored).

Options (D3):
- **a:** render each still on the DS at intro load time (the bake already uses
  the DS renderer + display capture). Removes the asset family; costs intro
  load time.
- **b:** automate the bake in the release script: download a pinned melonDS,
  two-pass build, about 432 boots, emulator windows on the user's screen,
  hours.
- **c:** a host-side renderer that writes the same stills. Large; matching
  DS GX output exactly is hard.

Whichever is chosen, required content must not come from a wildcard. A release
build must fail when a still is missing. The same goes for
`assets/congra/c*.png`, which the owner can commit if they are original art.

### R4: Torch/MSVC is the heaviest prerequisite, and it can go

Visual Studio Build Tools (about 6-8 GB, admin rights), CMake, vcpkg downloads
and Torch exist only to write O2R. The Python writer already matches Torch on
2,132/2,132 reloc files. Python writers for the remaining 27 files (audio,
particles, version), checked the same way, plus relocData cut straight from
the ROM, drop MSVC, CMake, vcpkg, Torch and the splat dependencies. What is
left: devkitPro, Python, Git, PowerShell 7.

### R5: reproducibility (required for wireless)

- **Toolchain drift.** devkitPro pacman serves only current packages, and
  `.itcm` is exactly full (32,736/32,736 B; +8 B broke the ship link on
  10/19). A newer GCC can fail to link or change bytes, which changes the build
  id. Pin the exact package versions above, verify them with `pacman -Q`, and
  keep an archive of those package files for installs (D6). ITCM headroom
  would make drift fail softer.
- **Install path.** The ROM embeds libnds header paths (`sprite.h`,
  `background.h`, `videoGL.h`), so devkitPro must sit at `C:\devkitPro`, or the
  build adds `-ffile-prefix-map`/`-fmacro-prefix-map`. The project path is not
  embedded (0 hits in play-p9d), so the clone folder does not matter.
- **Python.** Pin 3.15.2 (tested identical to 3.14.7, below). Set
  `PYTHONHASHSEED=0`, pin numpy/Pillow/pyyaml/lineinfile/pandas
  versions (EXTRA's Pipfile pins nothing; take versions from the owner's
  `Pipfile.lock` venv), and use one interpreter version. EXTRA's appender
  numbers files in `os.listdir` order (alphabetical on NTFS; other file
  systems change one o2r header byte), so require NTFS.
- **Accumulated NitroFS.** Seeded build directories ship every file left in
  `nitrofs/` from earlier flag sets. A clean build can lack a file the play ROM
  only has by accident. Diff the clean ROM's NitroFS list against play-p9d's;
  every difference needs an explanation.
- `DECOMP_PIN.txt` `OUTPUT_*` still pins the P1 artifact. Re-pin at release.
- **NitroFS directory order (found 2026-10-29).** ndstool numbers NitroFS
  directories in the order the file system lists them. NTFS lists sorted, but
  this PC's D: is a StableBit DrivePool pool, which lists folder by folder
  per underlying disk. Two builds of the same tree on D: then differ in the
  file-name table only: file data, file table, ARM9/ARM7 and
  `net/build.id` match, so wireless still pairs them. FAT32/exFAT drives
  list in creation order too. Fix before re-pinning: pack NitroFS from a
  sorted listing, either a staging copy on plain NTFS or (portable) a
  post-pass that renumbers the directory table in sorted order.

### Clean-build trial and Python 3.15.2 (2026-10-29)

Two clean worktrees of `a3f978a1d08`, outside the repo. Each had the same
inputs make cannot produce, and each built the play ROM from scratch
(~26-29 min, 25 of them Palutena's dress re-keying): one with Python 3.14.7,
one with a 3.15.2 venv (same numpy 2.5.4 and Pillow 13.0.0).

- **Result:** all 3,822 NitroFS files, ARM9 and ARM7 are byte-identical
  between the two. Packed from a sorted listing (a copy on C:), both give the
  same ROM (129,394,688 B, SHA-256 `2EF9A9D9...A0CB`). As built on D: they
  differ in the file-name table only (DrivePool order, above). Python 3.15.2
  is safe to pin for the release.
- **Inputs the worktrees needed besides the checkout:** junctions to the
  staging (`builds/p4-staging/extra-96621af`), `BattleShip_o2r`,
  `decomp/BattleShip-main/decomp/assets` and `.../decomp/build`; copies of
  `assets/audio` (BGM/FGM renders), `assets/intro`, `assets/congra`,
  `assets/p6-art`; and the decomp's encoded credits
  (`decomp/BattleShip-main/decomp/src/credits/*.credits.encoded` and
  `.metadata`, made by the decomp's `tools/creditsTextConverter.py`, read by
  `scripts/generate-battleship-import-overlay.ps1`). The release script
  produces each of these.
- **Build bugs only a clean tree shows (fixed):** Ike's `entry` clip reads
  the built `wait.o2r` but make did not know it (a parallel build ran it
  first; edge in `scripts/p4/p4_rules.mk`), and the shield-pose generator's
  declared outputs lacked Mario's and Yoshi's files 00 and 06, so no rule made
  them (`mk/rules_native.mk`).

### R6: prerequisite install mechanics

- A fresh Windows has PowerShell 5.1 only. The entry script's first part must
  run on 5.1, install PowerShell 7 (winget), then relaunch itself in `pwsh`.
- A downloaded `.ps1` is blocked by execution policy/Mark-of-the-Web.
  Document `powershell -ExecutionPolicy Bypass -File build.ps1`, or unblock the
  repo's scripts in the bootstrap.
- devkitPro's Windows installer is a GUI updater, and its silent mode is
  untested. MSYS2 + devkitPro pacman is scriptable but installs under
  `C:\msys64\opt\devkitpro` (see R5 install path). Test both in Windows
  Sandbox.
- Admin rights: `C:\devkitPro`, machine-wide Git (and MSVC, if R4 is not done).
- PATH order inside make: the release venv's `python`, then devkitPro's
  `msys2\usr\bin` (`cp`, `mkdir -p`, `cmp`, `nproc`, `tar`) and devkitPro
  tools. Never Git Bash (the msys2 `make` path differs).
- A GitHub "Download ZIP" has no submodules and no `.git`, while
  `stage_extra.py` uses `git archive`. The script must clone the submodules at
  their pinned SHAs when `.git` is absent.
- Disk: about 3 GB (staging 520 MB, build 610 MB, assets, O2R, relocData,
  repo), plus about 7 GB if MSVC stays. Network: GitHub, PyPI, devkitPro.

### R7: license and credits

- The repository has no LICENSE (D4). BattleShip, libultraship, Torch and
  sm64ds-decomp are MIT; sm64 is CC0; ssb-decomp-re, sm64-nds, Smash Remix and
  EXTRA have no license file.
- `src/port/nds_p4_*.c` ports Remix/EXTRA character code. Ask both teams for
  permission and credit them; the README names neither.
- Mirror both submodules under the owner's account, so an upstream
  force-push or deletion cannot break the release build.
- The README's "contains no Nintendo assets or ROM-derived data" is false
  until R2 is fixed.

## Owner decisions

| | Decision | Recommendation |
|---|---|---|
| D1 | Ike/Palutena in 1.0 (R1) | A: leave them out of the release set; re-author later if wanted |
| D2 | ROM-derived generated files (R2) | Build outputs, untracked. (`decomp/alt_assets`: decided and done 2026-10-28, no history rewrite) |
| D3 | Intro stills (R3) | Try a: render at load, timeboxed; fallback b |
| D4 | Repo license; Remix/EXTRA permission (R7) | Owner's choice; ask the Remix and EXTRA teams before release |
| D5 | Drop Torch/MSVC via Python writers (R4) | Yes |
| D6 | Toolchain: exact pin + package archive, or warn only (R5) | Exact pin (wireless needs identical ROMs) |
| D7 | Release flag set | `$S64PlayFlags` (minus D1), defined in one place for make, the tools and the script |

## Phases

1. **Dry run (measure).** When no build or perf run is active and the owner is
   not playtesting: clone `origin/master` with submodules into
   `Smash64DS_Port_Clean`. Run today's steps by hand (O2R, relocData, BGM/FGM,
   `stage_extra.py`, make with the play flags into `builds/release`). Log every
   failure and missing input, then diff the NitroFS list against play-p9d.
   Output: the real missing-input list (expected: R1, R3 and congra, plus
   unknowns).
2. **ROM to assets in Python (D5).** Writers for all 2,159 O2R files and the
   3,130 relocData files, proved byte-identical against today's outputs. Torch,
   MSVC, CMake and splat leave the release path.
3. **Producers (D1, D3).** Intro stills; BGM/FGM and Remix/EXTRA staging as
   make prerequisites or script steps with manifests; ROM-derived tracked
   `*.generated.*` files become build outputs; wildcards over required content
   become hard checks in the release set.
4. **Hygiene (D2, D4).** LICENSE; README (quick start, prerequisites,
   credits); submodule mirrors;
   leak audit (`scripts/publish/audit_minimal.ps1`, forbidden list updated)
   green.
5. **New `build.ps1`, one entry point:**
   1. Bootstrap on PowerShell 5.1: elevate if needed, install `pwsh`, relaunch.
   2. Prerequisites: detect, then install what is missing (Git, Python 3.10.x,
      devkitPro at the pinned versions in `C:\devkitPro`); verify versions.
   3. Sources: `git submodule update --init --recursive`, or clone the pins
      for a ZIP; check `docs/P4/source-lock.json` and `DECOMP_PIN.txt`.
   4. ROM: size, SHA-1 and byte order (offer `.v64`/`.n64` conversion); file
      picker when `-Rom` is omitted.
   5. Release venv with pinned requirements; `PYTHONHASHSEED=0`; PATH order
      from R6.
   6. ROM-derived stages (O2R, relocData, BGM/FGM, Remix/EXTRA), each skipped
      when its manifest matches, so a rerun resumes.
   7. `make TARGET=smash64ds BUILD=builds/release <release set>` (no `-j`).
   8. Verify: size, SHA-256 against the pin, build id, NitroFS count. Print
      where the ROM is. Logs in `builds/release-logs/`.
6. **Proof.** A fresh-machine run in Windows Sandbox (Windows 11 Pro) covers
   the prerequisite install. Two clean builds in different folders must give
   identical SHA-256 and build id. Boot check and smoke play, then re-pin
   `DECOMP_PIN.txt`.
7. **Release.** BUGS.md/PLAYTEST_NOTES.md rows closed and the
   `ACTIVE_GOAL.md` stability matrix green on the release HEAD. Tag `v1.0`,
   GitHub release notes (never a ROM or assets), README quick start.

Rough size: phase 1 half a day, 2 one to two days, 3 from a day to weeks
(D1/D3), 4 a day, 5 one to two days, 6 a day. Phases 2 and 5 are mostly new
files and can start before bug fixing ends. Makefile changes (phase 3) wait
for it, or go in serialized with its builds.
