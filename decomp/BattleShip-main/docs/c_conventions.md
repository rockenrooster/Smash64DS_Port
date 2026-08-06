# C Language Conventions

## Type System
The codebase uses N64 SDK types from `PR/ultratypes.h`. Use these consistently:

| Type | Meaning | Size |
|------|---------|------|
| `u8, u16, u32, u64` | Unsigned integers | 1/2/4/8 bytes |
| `s8, s16, s32, s64` | Signed integers | 1/2/4/8 bytes |
| `f32, f64` | Float / double | 4/8 bytes |
| `sb8, sb16, sb32` | Signed booleans | 1/2/4 bytes |
| `ub8, ub16, ub32` | Unsigned booleans | 1/2/4 bytes |

**Do not use** `int`, `short`, `long`, `float`, `double` in game code. Use the SDK typedefs.

Custom vector/color types from `ssb_types.h`:
- `Vec2f`, `Vec2h`, `Vec2i`, `Vec3f`, `Vec3h`, `Vec3i`
- `SYColorRGB`, `SYColorRGBA`, `SYColorRGBPair`, `SYColorPack`
- `Mtx44f` — 4x4 float matrix

## Naming Conventions (Decomp Style)
The decomp uses a consistent prefix system. Preserve it in all original game code:

- **Module prefixes**: `sy` (system), `ft` (fighter), `sc` (scene), `gm` (game mode), `gr` (graphics), `mn` (menu), `it` (item), `ef` (effect), `lb` (library), `mp` (map), `wp` (weapon), `if` (interface), `mv` (movie)
- **Global variables**: `gXXYyyy` — `g` prefix + module prefix + name (e.g., `gSYMainThread5`)
- **Static variables**: `sXXYyyy` — `s` prefix + module prefix + name (e.g., `sSYMainThread1Stack`)
- **Data (initialized)**: `dXXYyyy` — `d` prefix + module prefix + name (e.g., `dSYMainSceneManagerOverlay`)
- **Functions**: `xxYyyy` — module prefix lowercase + name (e.g., `syMainSetImemStatus`)
- **Enums**: `nXXYyyy` — `n` prefix + module prefix + name (e.g., `nSYColorRGBAIndexR`)
- **Structs/Types**: `XXYyyy` — module prefix uppercase + name (e.g., `SYOverlay`, `SYColorRGB`)

Port-specific code (in `port/`) may use modern C/C++ naming but should maintain clean boundaries with decomp code.

## Code Style
- **Indentation**: Tabs (matching decomp)
- **Braces**: GNU/Allman style — opening brace on its own line for function bodies
- **Section banners**: The decomp uses decorated comment blocks to separate sections:
  ```c
  // // // // // // // // // // // //
  //                               //
  //       EXTERNAL VARIABLES      //
  //                               //
  // // // // // // // // // // // //
  ```
  Preserve these in existing files. Not required in new port-specific code.
- **Boolean values**: Use `TRUE` / `FALSE` (defined as 1/0), not `true`/`false`
- **NULL**: Defined as `0`, not `((void*)0)`

## Macro Conventions
Key macros from `macros.h` — use these instead of rolling your own:
- `ARRAY_COUNT(arr)` — element count of static arrays
- `ALIGN(x, align)` — align value up
- `ABS(x)` / `ABSF(x)` — absolute value (int / float)
- `SQUARE(x)`, `CUBE(x)`, `BIQUAD(x)` — power macros
- `PI32`, `HALF_PI32`, `DOUBLE_PI32` — float pi constants
- `DTOR32` / `RTOD32` — degrees-to-radians / radians-to-degrees
- `F_CST_DTOR32(x)` / `F_CLC_DTOR32(x)` — degree-to-radian conversion (use CST for const multiplication, CLC for step-by-step calculation)
- `UPDATE_INTERVAL` (60) — ticks per second
- `TIME_SEC`, `TIME_MIN`, `TIME_HRS` — timing constants
- `I_SEC_TO_TICS(q)`, `F_SEC_TO_TICS(q)` — time conversion macros
- `PHYSICAL_TO_ROM(x)` — convert physical address to 0xB0 ROM address
