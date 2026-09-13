# Native stage wallpapers — 2026-09-12

## Scope and verdict

All eight non-Dream-Land VS stage backgrounds are wallpaper SObj sprites in
BattleShip (`gr/grwallpaper.c:45` camera-follow, `:126` common update, `:192`
Sector variant, `:267-300` kind selection); the native stage packet generator
emits only DObj layers, and until this change the battle renderer never called
the wallpaper drawer (only the stage-select preview did). A native BG2 owner now
draws the converted wallpaper behind the 3D stage in battle, bound to the live
camera once per presented frame. **Landed locally and captured on three stages;
five stages and the widest verifier remain owed.** No publication is claimed.

## Candidate identity

Integrated uncommitted tree on `381f24af321` (compact residency, CSS residency,
weak-stub wrappers and this owner), built 20:58 as
`make TARGET=smash64ds-p2-shell-hwtri BUILD=build-p2-shell`
(native-only link gate `NATIVE_ONLY_PASS`, 262 actual link inputs).

- ROM `builds/build-p2-shell/smash64ds-p2-shell-hwtri.nds`
  SHA-256 `4A2BF76BFB00F71D083870760464BE38C951E9865976AEAD2D5E50A0C6AA1D93`
- ELF `builds/build-p2-shell/smash64ds-p2-shell-hwtri.elf`
  SHA-256 `79A8EDA1DF35C71FC738EE2BFB40C5C93A1963C6319A8442897723DC118AF4FA`

## Implementation

- `src/nds/nds_native_wallpaper.c` / `include/nds/nds_native_wallpaper.h`:
  `ndsNativeBattleWallpaperPreload(gkind)` uploads the converted asset once at
  battle setup (84,480 B NitroFS read, retained BG2 epoch);
  `ndsNativeBattleWallpaperDraw(gkind, eye, at)` recomputes only the affine from
  the live camera per present (source `grWallpaperCalcPersp` semantics, the
  static Yoster and scrolling Sector variants included, source-exact for the
  degenerate Sector camera-distance case); `ndsNativeWallpaperInvalidate()` drops
  the resident identity at battle exit so Results or a rematch cannot inherit it.
  Dream Land (gkind 6) returns without touching BG2: its accepted sky/cloud/Whispy
  backdrop is DObj geometry in its packet.
- `src/port/renderer_adapter_stage.c`: binds the wallpaper before stage packet
  admission so an unrelated packet decline cannot blank a valid background.
- `src/port/taskman_seam_battle_host.c`: preload at pacing start (fast logic
  off), invalidate at lifecycle exit.
- `docs/p2/P2-1c-vram-map.md`: battle claim recorded — VRAM C is already the
  main BG2 256×256 RGB555 bitmap at priority 2 behind the 3D BG0; no bank remap
  or new capacity.
- Host tests: `scripts/stages/test_native_wallpaper_runtime.py` (C runtime
  harness with MSYS2 GCC, 16 tests / 19 subtests: retained-epoch reuse, affine
  redraws do not reread NitroFS) plus the pure-Python conversion checks (7 tests
  / 49 subtests); combined 23 tests / 68 subtests. `check-untracked-dependencies`
  and `check-docs` pass.

## Natural-shell captures (this ROM, shell walk to the stage, human slot idle)

| Stage | Variant | Witness | Capture |
|---|---|---|---|
| Yoshi's Island (gkind 5) | static | 8 wallpaper draws, 0 failures / 0 read failures; affine origin −8,−3, scale 73728 (228,228,1573,661) after the existing DS overscan correction | `2026-09-12_stage-wallpaper-yoshi-island.png` |
| Sector Z (gkind 1) | Sector scroll | 8 draws, 0 failures; upper-left background crop 2,831/12,760 non-black pixels, 189 distinct colours (starfield and nebula behind the ship) | `2026-09-12_stage-wallpaper-sector-z.png` |
| Peach's Castle (gkind 0) | common perspective | captured; sky and clouds visible behind the roof (the implementing session ran out of context before recording its counters) | `2026-09-12_stage-wallpaper-castle.png` |

## Remaining

- Congo Jungle, Hyrule, Zebes, Saffron and Mushroom Kingdom captures with the
  same witness; the Boundary profile on this tree; source-comparable side by
  side against the extracted wallpaper assets (P2 law 6).
- The capture wrapper knobs (stage target, runner slots 9/10) were not landed;
  the captures above were taken through a scratch copy under `builds/`.
- Yoshi's Island still shows the unrelated opaque texture-card backgrounds on
  its rotating textures (owner report, separate row).
