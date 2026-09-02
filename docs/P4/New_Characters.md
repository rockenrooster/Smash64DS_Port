# P4 — Smash Remix New Characters

Status: **planned, not started** (2026-08-31). Source repos, both read-only
reference material under the same `decomp/` rule:
- `decomp/smashremix/` — main Remix repo (xdelta-based z64 asset injection)
- `decomp/smashremix-plus-extra/` — extra-character fork with a
  python pipeline (`vpk0.py`, `file_appender.py`, `rom_injector.py`) and
  per-char `config.yaml`; its `smashremix` submodule carries the base template

Character list (dedupe of Wario from the original):
- Bowser, Peach, Dedede, Ganondorf, Wario, Crash, Wolf, Roy, Falco
- Sonic, Marth, Sheik, Banjo/Kazooie, Metal Mario, Lanky Kong
- **added 2026-08-31 from smashremix-plus-extra**: Snake, Meta Knight,
  Mr. Game & Watch (MRGAW)

## Verdict

Everything needed exists in-repo. No blockers found. Work per character is a
**porting task** (asm → C + data tables + asset extraction), not a research
task. First candidate: **Falco** (proves pipeline on smallest delta from the
shipped Fox). First full-custom candidate: **Bowser**.

## What the Remix repo gives per character

Per char folder `decomp/smashremix/src/<Char>/`:

| Piece | Format | Our use |
|---|---|---|
| `moveset/*.bin` | Vanilla SSB64 motion-event script binaries | Drop-in data — same script format the engine already interprets (`ftMainUpdateMotionEventsAll`) |
| `<Char>.asm` action tables | `Character.edit_action_parameters(...)` action→anim-file→script rows | Port to our ftdata action-param tables |
| `<Char>Special.asm` (NSP/USP/DSP) | **MIPS asm** | Hand-port to C, same way as `battleship_mario_special_hi.c`, `battleship_fox_blaster.c` etc. **Main labor per char.** |
| `AI/Attacks.asm` | Attack-option tables | Data for `battleship_ftcomputer.c` |
| `sounds/*.aifc` | Raw audio | Existing DS FGM conversion path (`src/nds/nds_audio_fgm.c`) |
| Attributes (weight, speeds, size) | `Character.asm` `define_character` + attrib arrays | Data, port direct |

## Models/animations

NOT loose in the repo. They live inside `decomp/smashremix/original.xdelta`
(20.8 MB) → patched `original.z64`, as VPK0-compressed SSB64 file blobs with
IDs declared in `src/File.asm` (e.g. `BOWSER_MAIN = 0x9B6`, `WOLF_MAIN =
0xB58`, `MARTH_MAIN = 0xCCA`, `DEDEDE_MAIN = 0xFFD`, `SONIC_MAIN = 0xDA8`,
`SHEIK_MAIN = 0xE6D`, `CRASH_MAIN = 0x12CC`, `PEACH_MAIN = 0x136E`,
`ROY_MAIN = 0x142A`, `LANKY_MAIN = 0x1456`, `BANJO_MAIN = 0x118F`,
`FALCO_MAIN = 0x8AB`, `WARIO_MAIN = 0x8F8`, `GANONDORF` files live at
`0x48C`-region offsets + `File.GND_MAIN`).

Same format as vanilla fighter files → same extraction/conversion tooling as
`scripts/extract-battleship-relocdata.py` + `BattleShip_o2r`. Requires a
user-supplied vanilla `ssb.rom` to apply the xdelta (same legal model the repo
already uses; never commit the ROM or the xdelta output).

Per-char file census (main remix, File.asm): Bowser ≈ 95, Marth ≈ 120, Sheik
≈ 100, Sonic ≈ 90, Dedede ≈ 120, Crash ≈ 105, Peach ≈ 115, Banjo ≈ 115,
Lanky ≈ 115, Wolf ≈ 85, Wario ≈ 50, Falco ≈ 15 (rest inherited from Fox
files), Roy ≈ 15 (inherits Marth files). For smashremix-plus-extra the assets
ship loose under `extra_characters/<Char>/animations|moveset|sounds` — no
xdelta needed for those three; counts in the table below. The plus-extra
python pipeline (`vpk0.py`, `file_appender.py`, `rom_injector.py`,
`build_single_character.bat`) is reusable as reference-conversion scripts.

## Clone bases (`define_character` parent / `config.yaml base_character`)

Clones reuse the parent's anim/skeleton files; only `MAIN`, `CHARACTER`,
and a handful of move files are new:

| Char | Parent | asm size | moveset bins | sfx | Effort |
|---|---|---|---|---|---|
| Metal Mario | (vanilla metal param, 2 KB asm) | 2 KB | 0 | 0 | Trivial — param swap only |
| Falco | Fox | 42 KB | 45 | 16 | **Low** — pipeline proof |
| Roy | Marth (via Captain struct) | 80 KB | 62 | 26 | Low-mid — needs Marth first |
| Ganondorf | Captain | 22 KB | 48 | 20 | Low-mid — smallest Special.asm set |
| Wario | Mario | 112 KB | 59 | 24 | Mid |
| Wolf | Fox | 87 KB | 47 | 17 | Mid |
| Meta Knight⁺ | Jigglypuff (plus-extras) | 105 KB | 59 | 28 | Mid-high — base Jigglypuff is a vanilla char we ship |
| Bowser | Yoshi | 101 KB | 60 | 18 | Mid-high — custom skeleton |
| Marth | Captain | 122 KB | 59 | 27 | Mid-high — counter, dancing blade |
| Sheik | Captain | 180 KB | 64 | 30 | High — needles, vanish, chain |
| Peach | Fox | 127 KB | 51 | 26 | High — float mechanic, turnips, parasol |
| Dedede | Captain | 196 KB | 66 | 31 | High — inhale, waddle throw, multi-jump |
| Crash | Mario | 161 KB | 60 | 23 | High — spin, dig, custom DSP platform |
| Mr. Game & Watch⁺ | Mario (plus-extras) | 136 KB | 79 | 17 | High — MG&W-specific move mechanics |
| Sonic | Fox | 259 KB | 63 | 35 | High — spin dash/charge, spring |
| Banjo | Captain | 166 KB | 50 | 33 | High — egg shot, wonderwing, breegull |
| Lanky | Mario | 161 KB | 46 | 29 | High — stretch grab, balloon, trombone |
| Snake⁺ | Captain (plus-extras) | 315 KB | 55 | 35 | Very high — C4/grenades/gun-volume asm; biggest of the list |

⁺ = from `decomp/smashremix-plus-extra/extra_characters/<Char>/` (config.yaml
base_character + animations + moveset + sounds, same script-bin format).

asm size = total `.asm` bytes in char folder (≈ port surface). Move-logic asm
translates at roughly 3–6 asm lines/hour of careful porting with the existing
`battleship_*` examples as pattern source; data tables are mechanical.

## Engine deltas to check per char (Remix assumes Remix engine edits)

- **Custom script opcodes**: Remix `src/Moveset.asm` extends the vanilla
  command set. Any opcode our interpreter lacks = new case in ftmain event
  parser. Check each char's `.bin` scripts against vanilla opcode coverage
  before committing a char.
- **Custom mechanics referenced by specials**: charge-smash storage (Sheik
  needles), wall jump, footstool, ledge jump are Remix engine features some
  chars hook. Port only what the char's scripts actually invoke; do NOT port
  the Remix toggle features themselves (project contract = vanilla behavior +
  DS speed).
- **Kirby hats**: `src/KirbyHats.asm` — needed only when Kirby inhale-copy of
  a new char is in scope.

## DS budget gates (measure before promising)

- **RAM**: Remix targets N64 + 8 MB expansion pak. DS = 4 MB. Vanilla Fox
  main file = 119 KB; per-fighter resident cost × 4 players must fit the
  battle heap. Measure extracted file sizes before enabling 4-player matches
  with new chars.
- **Poly budget**: DS = 2048 tris/frame hard cap. Remix chars were built to
  the N64 budget ≈ vanilla cast, likely fine; verify per char with the
  existing renderer census (`diagnostics_renderer_census.c`).
- **VRAM**: per-char textures — check against `nds_renderer_textures_effects.c`
  bank map.

## Per-char work recipe (the pipeline Falco will prove)

1. Extract char file set from Remix `original.z64` by File.asm ID list
   (extend `extract-battleship-relocdata.py` or sibling script; output into
   the same generated asset path as vanilla fighters).
2. Convert model/anims through the existing fighter production manifest
   flow (`scripts/fighters/`, `generate_nds_native_owners.py`).
3. Convert `moveset/*.bin` tables — likely 1:1 (same script format); verify
   opcode coverage first.
4. Port `<Char>Special.asm` → `src/import/battleship_<char>_*.c` following
   the mario/fox/link/samus/captain/donkey pattern.
5. Port action-param + attributes into ftdata.
6. Convert `sounds/*.aifc` → FGM.
7. CSS: portrait/name sprite from Remix z64 + `src/css/` layout data.
8. Verify: character-select → battle → all specials → results, through
   Boundary profile; poly/RAM census recorded in `artifacts/performance`.

## Order

1. **Falco** — proves steps 1–8 on the smallest delta.
2. **Metal Mario** — trivial param swap; cheap second win.
3. **Ganondorf** — smallest Special.asm of the unique chars.
4. **Meta Knight** or **Bowser** — first full-custom work; Meta Knight's
   base (Jigglypuff) is already shipped, Bowser's (Yoshi) is not yet.
5. Then by doc-list priority incl. plus-extras: Peach, Dedede, Wario, Crash,
   Wolf, Roy (after Marth), Marth, Sheik, Mr. Game & Watch, Sonic, Banjo,
   Lanky, Snake (very high — schedule last).

## Open questions (resolve at P4 kickoff, not now)

- 4-player new-char matches after RAM census?
- Kirby inhale-copy scope for new chars?
- Remix CPU AI tables (`AI/Attacks.asm`) — port per char with the fighter, or
  batch at the end?
