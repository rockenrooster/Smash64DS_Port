# P2-1c — VRAM bank ownership, per scene, both engines

The `docs/p2/P2-1-vs-shell.md` risk this closes: *"2D/3D VRAM arbitration
between menu scenes and battle — audit VRAM bank ownership per scene before
building screens."* Read before adding any surface that wants VRAM.

Everything below is read from `ndsPlatformInit`
(`src/nds/nds_platform.c:409`-`:465`) at `NDS_RENDERER_HW_TRIANGLES=1`, which
is the only place this build maps a bank. **The map is set once at boot and no
scene remaps it** — the sole exceptions are the renderer's transient
`VRAM_x_LCD` window while it DMAs a texture (`nds_renderer.c:11105`-`:11120`,
restored in the same function) and P2-1c's own bank I claim below.

## Boot map

### Main engine — `MODE_5_3D | DISPLAY_BG2_ACTIVE | DISPLAY_BG3_ACTIVE`

| Bank | Bytes | Mapping | Owner | Headroom |
|---|---:|---|---|---|
| A | 131,072 | `VRAM_A_TEXTURE` | GX texture pool | shared pool, no fixed slack |
| B | 131,072 | `VRAM_B_TEXTURE` | GX texture pool | ditto |
| C | 131,072 | `VRAM_C_MAIN_BG_0x06000000` | main BG2 | **zero** |
| D | 131,072 | `VRAM_D_MAIN_BG_0x06020000` | main BG3 | **zero** |
| E | 65,536 | `VRAM_E_MAIN_SPRITE` | main OBJ | scene-dependent, see below |
| F | 16,384 | `VRAM_F_TEX_PALETTE_SLOT0` | GX texture palette | — |
| G | 16,384 | `VRAM_G_TEX_PALETTE_SLOT1` | GX texture palette | — |

C and D have **exactly zero** headroom, and that is arithmetic rather than an
estimate: BG2 and BG3 are each `BgType_Bmp16, BgSize_B16_256x256`, which is
256 x 256 x 2 = 131,072 bytes, the whole bank. They are the compositor the
imported source-sprite path draws into (`src/port/sprite_preview_backend.c`).

Layer composition: BG0 is the 3D engine at priority 1, BG2 the sprite overlay
at priority 2, BG3 the foreground overlay at priority 0, `REG_BLDCNT` alpha
BG0 over BG2. **BG1 is unused** — MODE_5 gives it as a tiled text layer, and
it is free as a *layer*, but there is no main BG VRAM left to put tiles and a
map in, so it cannot be used without taking C or D from the battle.

### Sub engine — `MODE_0_2D`

| Bank | Bytes | Mapping | Owner | Headroom |
|---|---:|---|---|---|
| H | 32,768 | `VRAM_H_SUB_BG` | sub BG0, libnds text console (`consoleInit`, map base 15, tile base 0) | most of the bank |
| I | 16,384 | **unmapped before P2-1c** | — | whole bank |

Bank I was the only bank in the build with no mapping at all.

## Per scene

Nothing remaps, so "ownership" here means *who has content in the bank while
that scene runs*.

| Scene | A/B | C/D | E (main OBJ) | F/G | H | I |
|---|---|---|---|---|---|---|
| `nSCKindVSBattle` | fighter + stage textures | sprite overlay + foreground | **IFCommon** — `ndsIFCommonNativeOamPrepareGameStatus` packs upward from offset 0; the sixteen asset specs sum to ~41.7 KB before the Task 39 hit-spark sheet | A5I3 flare palettes | console | — |
| `nSCKindVSResults` | results textures | same compositor | IFCommon, still prepared | palettes | console | — |
| `nSCKindTitle`, `nSCKindVSMode`, `nSCKindPlayersVS`, `nSCKindMaps` | menu textures | same compositor | **free** — IFCommon's prepare is driven by the battle scene's own asset load and its latch is cleared at teardown (`ndsIFCommonNativeOamDiscardTextures`) | palettes | console | **P2-1c UI kit** when a sub-engine surface is entered |

## What P2-1c takes, and why that is safe

The kit draws on **main OBJ (bank E) in menu scenes** and on **sub OBJ (bank
I) whenever a bottom-screen surface is entered**. It takes no BG bank on
either engine, so it cannot collide with the battle compositor or the console.

Main bank E, allocated top-down (`src/nds/nds_ui_kit.c`):

| Range | Bytes | Content |
|---|---:|---|
| 61,440 – 65,536 | 4,096 | cursor, hand point (32x64 cell) |
| 59,392 – 61,440 | 2,048 | cursor, hand grab (32x32 cell) |
| 51,200 – 59,392 | 8,192 | Mario portrait (64x64 cell) |
| 43,008 – 51,200 | 8,192 | Fox portrait (64x64 cell) |
| 30,720 – 43,008 | 12,288 | 6 text fields x 4 cells of 32x8 |
| 0 – 30,720 | 30,720 | **left for the battle's OBJ tenant** |

The battle tenant needs ~42 KB, so the two do **not** fit together, and the
top-down layout is a mitigation rather than a proof. The actual guarantee is
scene exclusivity, enforced rather than asserted: `ndsUiKitEnter` calls
`ndsIFCommonNativeOamIsPrepared()` and refuses, counting
`gNdsUiKitEnterRejectCount`, if the battle's assets are still resident. A
future overlay that genuinely needs both must shrink one side first.

OAM ids are split the same way and in the opposite direction: IFCommon
allocates downward from 127 (`sNdsIFCommonNextOamID`), the kit upward from 0
and never past `NDS_UI_KIT_OAM_IDS` (32). They grow away from each other.

Sub bank I: `VRAM_I_SUB_SPRITE`, the same 12,288-byte text layout, 4,096 bytes
spare. The 64x64 portrait cells do **not** fit beside it, which is why the
sub-engine surface is text-only. That is the whole main/sub difference, and it
is a capacity fact — the P2-2 bottom-screen HUD is text and small sprites.

## Open items for later phases

- **P2-2 bottom-screen HUD** needs the sub BG layer that the libnds text
  console currently holds (bank H, map base 15). Retiring or relocating that
  console is P2-2's call, not this row's; the kit's sub path deliberately
  claims OBJ only so it does not pre-empt the decision.
- **A menu background image** would need a main BG bank, and there is none.
  The options are (a) draw it as 3D through BG0, (b) reuse the existing BG2
  bitmap compositor the imported source sprites already target, or (c) take a
  texture bank. All three carry a cost; none is free.

  **P2-1d took none of them.** It uses a fourth surface the audit had not
  named: the main engine's BACKDROP, which is BG palette entry 0 and costs no
  VRAM at all. It shows wherever no BG and no OBJ covers a pixel, so with the
  overlay layers cleared it is the flat field behind a menu, and P2-1d sets it
  to the source's own decal blue (`mnmodeselect.c:517`, `0x083365`). One
  halfword, no bank, no arbitration with the battle, restored to black on
  scene exit.

  What is still open is the source's own artwork — the 300x220 CI
  `llMNCommonSmashBrosCollageSprite` both menus draw at (10,10) — and that is
  a fidelity decision with a real price, so it is parked as
  `BLOCKED(decision: menu artwork background)` on the board rather than
  chosen here.

- **The main OBJ layer needs the sprite overlay left DISPLAYED**, and this is
  the row's most expensive finding. A menu scene wants BG2/BG3 empty, and
  `ndsPlatformSetOriginalSpriteOverlayEnabled(FALSE)` looks like the way to
  say so -- but it takes the 3D clear to alpha 31 and `bgHide`s both overlay
  layers, and with that state the main OBJ layer does not reach the screen at
  all. Measured on P2-1d's own build: DISPCNT bit 12 read 1, OAM held valid
  32x8 bitmap-OBJ entries at priority 0, the composed texels were in bank E at
  exactly the offset attr2 named, and three separate captures still measured
  0/49152 top-screen pixels differing from the clear colour. Keeping the
  overlay ENABLED and clearing both layers instead (which is the state P2-1c's
  demo rendered in) restores the OBJ layer: the same captures then measure
  0.5-2.0% drawn content. A menu surface clears the overlay; it does not
  disable it.
