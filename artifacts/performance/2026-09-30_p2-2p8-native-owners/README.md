# P2-2p8: baked native owners for the items, weapons and effects drawn with none (2026-09-30)

Done-when item: 0 native failures. Every root the lab drew with no native owner
went through `ndsStageRejectNativeRender` as NO_PROGRAM and was not drawn at
all: Ness's PK Fire pillar, the Ray Gun's shot, most Poke Ball Pokemon.

## Census

The lab failure table kept the first 16 (identity, status, reason); for the
stage domain the status is the gkind, one value a run, so a count could not
say which root failed. It is keyed on the root now
(`nds_renderer_dispatch_profile.c`, tick-HUD builds only).

Full-match lab census, 25 arms (Fox/Pikachu/Ness/Samus and the default
DK/Samus/Link/Kirby on all nine stages, plus Luigi/Kirby/Ness/Purin,
Captain/Yoshi/Kirby/DK, Pikachu x4, Kirby x4 and the owner roster), items on
(`census-before.txt`; GObj kind 1011 effect, 1012 weapon, 1013 item):

| What | Root | Arms | Draws lost a match |
|---|---|---|---|
| PK Fire pillar (itnesspkfire.c, two DObjs) | 336:0x0870 + 0x0960 | 6 stages | 51-176 each |
| Bob-omb walking left (itbombhei.c:214, runtime-selected) | 86:0x34c0 | Jungle, Zebes | 13, 108 |
| Bumper, placed (NBumperWaitDisplayList) | 86:0x7af8 | Sector Z | 116 |
| Ray Gun shot (LGunAmmo weapon) | 86:0x40a8 | Hyrule | 12 |
| Goldeen | 86:0xb618 | Jungle | 197 |
| Koffing, and its smog weapon | 86:0x12730, 0x13050 | Hyrule | 177, 434 |
| Venusaur's Razor Leaf weapon (two DL links) | 159:0x28a8 + 0x2998 | Saffron | 38 each |
| A Ness effect (NessSpecial2) | 352:0x08e0 | Castle | 12 |
| Samus's entry effect (owner refuses its last 3 frames) | 349:0x0930 | 4 stages | 3 |
| Link, whole fighter, entry | fighter 5, owner failure | Sector Z, Saffron | 3 |

Every item and weapon attribute in ITCommonData, walked to its DL roots
(`tools/itroots.py`), shows the rest of the gap the census could not reach:
of the 13 Poke Ball Pokemon only Onix had an owner, and none of their
weapons (Onix's rocks, Meowth's coins, Beedrill's swarm, Blastoise's hydro,
Starmie's swift) or runtime-selected lists (Onix's, Blastoise's, Hitmonlee's
second list, the box's break effect).

## The owner

Every one of those roots has the shape the hand-written item owners bake one
at a time: fixed state words, fixed geometry, at most live segment-E
materials (most are single textured quads). So one compiler writes them all
as data (`scripts/stages/generate_nds_native_item_baked.py`):

- The root is walked once at build time from the SHA-pinned O2R payloads,
  same-file G_DL calls inlined and the vertex cache simulated; triangles are
  grouped between state changes and chunked to the emitter's 24 vertices.
- State words become the shared `NDSNativeStateDelta` effects and run through
  `ndsRendererNativeApplyStateDelta`, the stage and fighter programs' own
  applier; G_SETENVCOLOR is written as the hand owners write it; a segment-E
  call applies the DObj's MObj snapshot for that slot
  (`ndsRendererNativeApplyMaterial`); geometry goes to
  `ndsNativeItemWave1Emit`.
- Anything outside the subset refuses the build: foreign images or
  vertices, matrix commands, G_MOVEWORD other than light colours,
  G_MODIFYVTX other than ST.
- Blastoise's hydro list 1 draws with list 0's vertex cache and state (the
  source's lists run back to back): it is compiled from list 0's final cache,
  with list 0's state ops replayed first, and its two G_MODIFYVTX ST patches
  folded in.

32 roots, 597 ops, 33 geometry groups, 66 triangles.
`src/nds/nds_native_item_baked.exec.inc` runs them; the stage adapter makes
one lookup per DL of an item, weapon or effect whose file holds a baked root
(`NDS_NATIVE_BAKED_ASSET_MATCH`), checks the root's ENDDL word as a layout
fingerprint, and snapshots the DObj's MObjs for the material slots. Nothing
reads a source display list at runtime.

## Verification

Same census arms on the baked lab ROM (`census-after.txt`): every baked root
draws, 0 submit failures (PK Fire pillar 102-365 draws a match, Bumper 116,
Ray Gun shot 12, Goldeen 197, Bob-omb left 108, Koffing and smog 611). What
remains is the Samus entry effect's last 3 frames and Link's 3 entry frames,
not baked roots.

The Pokemon, forced: new lab words `gNdsLabItemToggles`/`gNdsLabItemRate`
(Poke Balls only, very high) with the source's own
`dITManagerForceMonsterKind`, one arm per Pokemon on Dream Land
(`census-pokemon.txt`): 0 native failures on all 13, 335-1,108 baked draws a
match, 0 submit failures. Hitmonlee is made (probe: f732, a live GObj) but
never reaches the adapter's drawable check, so it draws nothing and records
nothing; it moves itself to DL link 18 at creation. OPEN.

Pixels: frame-exact captures on the lab ROM (`tools/bakedcap.ps1`,
`tools/moncap.ps1`: the guest halted on the frame marker, PrintWindow of the
melonDS window). `pkfire-pillar-sheet.png`: the pillar on Dream Land frames
724-760, a textured orange column with its flame quad on top.
`pokemon-b90-sheet.png` (quantized to 256 colours for size): each Pokemon's
run at its 90th baked draw (Snorlax with its light rays, Charizard, Meowth,
Beedrill, Blastoise, Chansey, Starmie's swift, Clefairy, Mew).

## Two defects that sheet showed: Koffing's smog, and every weapon's mode

Koffing's smog drew as solid black squares with yellow blotches
(`koffing-smog-before-after.png`, left). Two causes, both fixed:

- **Weapons drew opaque.** Every weapon draws after `wpDisplayDrawNormal`
  (wpdisplay.c:131): `G_RM_AA_XLU_SURF` with Z off. The port's weapon
  traversal starts from `ndsRendererInitStats` (render mode 0) and seeds only
  the geometry mode, so a weapon list that sets no render mode of its own --
  the smog, the Ray Gun's shot, Starmie's swift, Clefairy's swarm, Razor
  Leaf's translucent list -- lost its blend. Each baked weapon root now
  starts with the source's mode (a root that sets its own still overrides it).
- **An I texture with texel alpha bakes opaque.** The smog is an I4 tile under
  `TEXEL0 * SHADE`, alpha `TEXEL0 * PRIM`. `ndsRendererHardwareConvertI` sets
  every I texel opaque -- right for the combines that ignore texel alpha, and
  the renderer's own coverage bakes (BLENDPE, the flat-PRIM beam) are separate
  modes. The generator marks a group drawing an I4/I8 render tile under a
  combine that reads TEXEL0 alpha (only the smog, today); the executor sets
  `sNdsRendererHardwareIntensityCoverage` around its emit, and the bake takes
  the existing graded path -- GL_RGB8_A5, intensity as coverage -- over a grey
  ramp palette (the vertex/PRIM modulate is the hardware's). Keyed by its own
  flag bit (`NDS_RENDERER_HW_TEXTURE_KEY_I_TEXEL_ALPHA`); no other surface's
  key or the static corpus moves.

Same frames after (right): a translucent yellow-brown cloud.

## Samus's entry effect: textures retired under it

The census rows 349:0x0930 were Samus's entry effect refused on its last three
frames (Jungle, Zebes, Hyrule, Dream Land). A lab witness on the entry owner's
refusal sites (`gNdsEntryEffectRejectLine`, lab builds only) named the texture
check: VSBattle retires the startup-only entry textures at GO
(`ndsRendererHardwareReleaseEntryStartupTextures`), and Samus's effect drew
three frames past GO into texture name 0. The owner now stamps the frame of
its last startup-texture draw, and the retirement waits for one presented
frame with none (`ndsRendererEntryEffectStartupIdleFrames() >= 2`). Same
arms: 0 failures, the textures still retired (41 a match; 19 on Sector Z,
where Fox's Arwing keeps its own).

## A heap cliff the extra RAM tipped: the world caches

The first baked build read Sector Z's default roster at STG +23.5K a frame,
every frame from 64 on -- the same constant shift the 09-30 flat-2048 margin
trial showed. Not the Arwing table (it loads in every Sector Z arm with
~750 KB free) but the two optional world-matrix caches
(`renderer_adapter_matrix.c`, ~17 KB): they are taken once, on the first
battle frame, only if 128 KB stays free after them, and Sector Z's default
roster had 133,616 B free there -- 6 KB short once the image grew
(`world-cache-census.txt`).

Free heap over that match (`heap-post-go.txt`, probes `tools/goprobe.ps1`):
the drop from the first frame to the low-water is the countdown's (133.6K at
the attempt, 73.9K at GO, 61.2K at the end); after GO the tight rosters take
8-13 KB more (Sector Z default 12.6K, Luigi/Kirby/Ness/Purin 8.0K, Charizard
forced 9.8K), and the large post-GO drops (57-83 KB) are rosters with 150-530
KB free, i.e. caches that size themselves to the heap. So an attempt that
misses the first-frame reserve is retried once at GO with 48 KB kept (the
GObj latch's 25,600 floor plus headroom).

## Final census (`census-final.txt`; lab ROM with all of the above)

18 arms: the 12 event32-margin rosters plus Sector Z with five more rosters
and two forced Pokemon. Native failures: only Link's 3 entry frames on Sector
Z and Saffron (`nds_renderer_native_common.c:10181`, a texture bind fails at
the entry burst: VRAM, open). Event-32 refusals 0. Baked draws 0 failures.
World caches allocated in 17 of 18 (Sector Z Captain/Yoshi/Kirby/DK gets the
DObj cache only); low-water >= 46,792 everywhere. Sector Z default STG
140,352 against the pre-change 139,776, low-water 47,936.

Against the margin ROM (`m2_*`, cross-build): P50 +0.3..+5.6K on every arm,
also where nothing new draws (layout drift of this size has been seen before,
~3K); P95 +58K on Hyrule Kirby x4 (611 Koffing/smog draws), +23K Dream Land
Fox/Pikachu/Ness/Samus and +13K Castle Luigi/Kirby/Ness/Purin (PK Fire's
pillar), -9K Sector Z default. Replay digest identical where checked
(Sector Z default, Saffron Fox/Pikachu/Ness/Samus).

## Gate

Gate ROM (default Dream Land roster, items on), two runs each, each pair
identical run to run:

| Build | P50 | P95 | P99 | two-VBlank | heap low-water |
|---|---:|---:|---:|---:|---:|
| `build-p2p8-b6` (control: the event32-margin receipt's) | 915,776 | 1,233,408 | 1,500,096 | 1,755 | 329,644 |
| `build-p2p8-b7` (this batch, witnesses in tick-HUD builds) | 920,000 | 1,238,720 | 1,510,272 | 1,751 | 316,332 |
| `build-p2p8-b8` (as committed: witnesses in lab builds only) | **917,824** | **1,235,520** | **1,501,696** | 1,755 | **317,356** |

Replay digest identical over 1,972 samples, b6 against b7 and b8; 0 native
failures. The image grew 13,312 B in b7 (`.main`; the low-water drop is the
same): the baked tables are 10,760 B of it (ops 7,164, roots 768, groups
528, vertices 1,360, colours and triangles the rest), the executor 588 + 184,
and 852 B were the entry owner's refusal witness, which b8 moves out of the
gate ROM into the lab builds (`NDS_LAB_FOURCPU_SWEEP`) with the world-cache
witnesses. The gate roster draws none of the new roots (`g7_c`, `g8_a`:
`gNdsItemBakedDrawCount` 0), so b8's +2.0K P50 / +2.1K P95 is carrying cost
and placement, not work: the price of drawing what was not drawn. Moving the
baked tables out of the resident image is the lever if it is ever wanted
back.
