# P6 status — Ike and Palutena

Owner of the live P6 state. The plan is [P6_PLAN.md](P6_PLAN.md); the cards
are [characters/ike.md](characters/ike.md) and
[characters/palutena.md](characters/palutena.md).

**State (2026-10-13, night):** active (goal P5-P7). Move cards approved (I1,
P1). A1 done: Ike (19) and Palutena (20) are registry contents built from
Roy's and Peach's exports (null clones) and play in every ROM. Palutena's
look is settled: the owner's "3DS to DS" route (the owner's 1,505-triangle
baseline of the kit, reduced and hand-tuned in Blender with the owner, 428
triangles; approved "this model we created is what i want our palutena to
be", [look record](#palutena-look-record)). It is in the lab ROM through
`look_blend.py`, `look_reduce.py` (low detail, 278) and `model_writer.py`
(main 0x1B00, model 0x1B03, spec "files"); its limbs play Peach's clips at
their own lengths through FTAttributes.translate_scales. Next: the in-game
check and the owner's look at the low, then her moves (P3-P4).

## Board 1

| Item | State |
|---|---|
| A1 registry, build, null clones | done 2026-10-13 ([A1 record](#a1-record)) |
| A2 model reader and writer (F1) | done 2026-10-13 (`model_writer.py`: Peach's own low scene written back draws the same 211 triangles; the written look passes the native owner generator, host) |
| A3 Blender bridge, kit reader, fit | done 2026-10-13 (`dae_reader.py`, `kit_fit.py`, `scripts/p6/blender/`) |
| A3b Track K: the kit rebuilt (F6) | dropped by the owner's route (ops kept for reference) |
| A3c Track R: the base restyled (F6) | superseded for Palutena by the owner's 3DS-to-DS route (look record); `look_blend.py` (Blender dump to scenes, translate_scales), `look_reduce.py` (low detail) |
| A4 animation codec and retarget (F2) | codec done 2026-10-13 (`anim.py`: F2 over every Roy, Marth and Peach clip, 486 clips, 10,802 streams, 0 differ within ANIM_EPS from two previous poses; blend-in leads and loops that skip their opening steps kept); retarget written (Captain's jab on Roy's tree: head and feet on the source's paths, hands along them; Mario and Link role maps too; 2026-10-13 Peach's tree from Mario, Ness, Captain, Link, Marth and Peach herself, `first_slot` for a TransN source, and `item`: a held item's axis follows the source's, so Palutena's staff goes where Marth's sword goes); recipes (`art/p6/<name>/anims/*.json`: source clip, optional retarget, optional `retime` length factor) built by `anim.py --build`; a clip whose motion row has the TransN flag carries one extra leading slot (the travel the status physics reads), so playback starts its rows at slot 1; mirror and splice not yet needed |
| A5 motion-script assembler (F3) | done 2026-10-13 (`mscript.py`: text and back; F3 over every export's motion scripts, 18,741 commands, 0 differ) |
| A6 review harness | clip playback done 2026-10-13 (`blender/clip_view.py` poses an imported model through `anim.world_frames` per frame and renders a strip; script hitboxes from `mscript.attack_timeline`, joint id minus four = model row, drawn as spheres; Captain's jab retargeted onto Roy's tree plays); the lab hitbox census comparison and GIFs next |
| A7 authored export (F4) | file substitution done 2026-10-13 (spec "files", `remix_rom.OverlayRom`); authored motions done 2026-10-13 (spec "motions": clips built from `art/p6/<name>/anims` recipes by `anim.py --build` take the content's own ids (Ike 0x1A20 on), scripts in `scripts/p6/<name>/motions/*.ms` assemble into the authored RAM window (`remix_rom.AUTHORED_RAM` 0x80F00000) as events.json blocks; the generator overlays both, its synthesized motion file carries the script and round-trips). With scripts of its own a content synthesizes its own motion file (its own content id's synth index: Ike 0x1820) and ships the base's main file as its own copy (0x1A00, `copies`), since the main names the motion file: NitroFS staging refuses one file id with two contents' bytes (Ike's first lab build stopped on Roy's 0x1750). Added 2026-10-13: `script_edits` (the base's script with damage scaled, elements mapped, commands dropped, a frame-speed command first; `p6_author.py --staging` reads the words), appended motions (the index past the base's last), appended statuses (`statuses`: the id past the last, routines by name), `attributes` (FTAttributes floats set in the content's main copy) and authored kind-table values (`{"status": N}` for `jab_3_action`). Sword trails, colour animations and audio rows next |
| A8 audio | authored cues done 2026-10-13 (`scripts/p6/p6_audio.py`, `scripts/p6/<name>/audio.json`): local recordings trimmed, peak-normalized, resampled (voices 16 kHz, effects 22.05 kHz) and IMA-encoded into the P4 pack (`p4_audio.py`) under each content's id window (0x700 + 0x80 per authored content); the map sends the base's FGM ids to cues in the content's synthesized motion file (the shipped copy; the round trip checks the image's), its main file's voice fields and its announcer and chant rows; scripts name cues as `cue:<name>`. Ike's set built; the owner's by-ear approval (sample sheet) and the FGM census next; Palutena's set with P7 |
| A9 presentation | started 2026-10-14: `p6_present.py` appends each authored content's select art to the build's copies of Remix's portrait, name and logo files (`p4_css.py`). Palutena's portrait is rendered from her look (`blender/portrait.py`: head and shoulders, three-quarter, transparent) and framed with Remix's portrait frame and background, its flash white; names are composed from Remix's name letters, kerned as Remix's own words are (PALUTENA 65 of the card's 66 columns); Palutena's Kid Icarus emblem is authored, Ike keeps Fire Emblem's; the select call is each content's announcer cue. Checked in the UI kit's host bake (`generate_mn_ui_kit.py --preview --preview-dir`: the grid cell, the flash, the card in every state). Palutena's stock icon (2026-10-14): the kit's stock icon row reduced to the HUD's 8x10 CI4 sprite, one LUT a costume (`scripts/p6/p6_stock.py`). Palutena's Results (2026-10-14): her victory poses (Peach's clips with her wings, halo, blinks and win lines; the spec's `menu_motions`), the select screen's Selected pose with her halo, and her name laid out as Remix lays out its long names (x 20, scale 0.7, "WINS!" at 185). Ike's portrait and icon wait for his look |
| A10 effects | started 2026-10-14: parts shown on demand are model-part variants of existing rows (`scripts/p6/look_parts.py`: the row's list plus cut-out cards; `model_writer.py` writes the variant lists and the rows' FTModelPartDesc, variant 0 the tree's list): Palutena's wings (kit godwing image, forward and down smash) and halo (authored ring, up air) in the lab ROM, 0 native failures, 0 lean declines. 2026-10-14: her light pillar (two crossed cards, up smash; cards light by an up normal) and her entry gate (the kit's golden double door, `scripts/p6/p6_gate.py`, in her special file 2 in place of Peach's turnip; her own entry case makes it and parts its doors; drawn natively as an entry effect). Ike's blue Eruption (2026-10-14): his charge glow a red-blue swapped copy of Roy's ROY_CHARGE (spec `colanims`, `p4_colanims.py` tinted rows, `ndsP4ColAnimRemap`) and his swing trail blue (spec `sword_trails`); the hit's burn stays orange |
| A11 Kirby | Palutena done 2026-10-14: her custom power (Explosive Flame on Kirby's PK Fire motions, her bursts and blast made in code at the stick's distance, her sounds) and her hat (her hair over his head, a gold ring, her hair panels down his back; `p4_kirby_custom_hats.py` now reads inline-material spans by order and draws bands of several rows, part of the way round, from other joints' spans; `scripts/p6/hat_preview.py` renders a hat on Kirby); lab: copied, cast on the ground and in the air, 0 native failures, 0 lean declines ([card](characters/palutena.md#kirby)). Ike: Roy's power for now; his tint and hat follow his look |
| A12 CPU (F5) | Palutena done 2026-10-14. `scripts/p6/coverage.py` measures a move's CPU row from its clip and script (the first active frame; the box its hitbox spheres sweep, in world units: clip slots to the rows FTAttributes.setup_parts makes, scale tracks, translate_scales, the TransN root, FTAttributes.size); its hitbox positions match the lab's in-game ones within 2 units (Palutena's forward tilt, Kirby's jab). F5: `--check-vanilla Kirby,Fox,Mario` measures the original fighters from the original image against the decomp's dFTComputerAttacks rows: Fox 8 of 10 within 60 units and a frame (7 within 4), Kirby 5, Mario 5; the rest are rows the source copied (Kirby's and Fox's AttackS3 are their Attack11's) or hand-set (Fox's AttackS4 reaches 1,020). A spec's `cpu` (`p6_author.py`) measures the base's rows on the content's own moves for the generator. Palutena's rows, her CPU Explosive Flame distance and recovery Warp aim: [card](characters/palutena.md#cpu); lab census clean. Ike done 2026-10-14: his jab 1, slowed forward tilt, up smash and dash attack measured, Roy's hand-tuned rows kept for the moves he plays as Roy (Blazer's for Aether), Shield Breaker's dropped; four-Ike CPU census clean ([card](characters/ike.md#cpu)) |
| A13 checks | started 2026-10-14: `scripts/p6/check_p6_art.py` checks the tracked tree: no tracked file in a rip folder or under builds/, no tracked audio outside decomp/, no tracked image under art/p6, scripts/p6 or docs/P6 unless listed as authored, no tracked file byte-identical to a rip or builds/p6-art file (every file hashed), no number array or string dump in a JSON recipe, the rip folders and builds/ ignored, each authored export fresh against its spec, scripts, audio recipe and spec files; `--self-test` plants a fault for each of the seven and expects red (7 of 7). Repository: 0 problems. Not yet: F1-F6 as one host test |

## Board 2

| Fighter | Step | State |
|---|---|---|
| Palutena | G0/G1 look (owner route: 3DS to DS, 2026-10-13 night) | approved by the owner (428 triangles); written (high 428, low 278) and in the lab ROM |
| Ike | I0 null clone done (A1); I1 approved; I2 look built 2026-10-14 to the owner's direction ("Just use roy": Roy's low model with blue hair, the outfit in Ike's colours on all six costumes, Roy's circlet as an olive headband, gold Ragnell, eyes 0.85x, shoulders broader; written as Ike's own files 0x1A00/0x1A03; lab: 0 native failures, 0 lean declines; [look](characters/ike.md#look-g1)); in owner review; I3 normals authored 2026-10-13 (jabs 1-3, dash attack, forward tilt, forward smash, up smash; [card record](characters/ike.md#move-card-approved-2026-10-13-i1)); lab check: four-CPU lab ROM, Ike in port 0 jabbing every 10 frames runs Attack11, Attack12 and the appended Attack13 (249) with their hitboxes, his entry plays Roy's, 0 native failures; I4 specials authored 2026-10-13 (Quick Draw, Aether, the taunt counter in `src/port/nds_p4_ike.c` on appended statuses 250-263; Eruption stays Roy's Flare Blade, its blue tint is A10): lab, forced inputs on port 0: Quick Draw charges, dashes 14 frames at 30 frames held and ends in its miss lag; Aether rises, plunges, lands; the taunt counters a CPU's hit and strikes; 0 native failures. A8 Ike's sounds: four-CPU Ike census plays his cues; I5-I6 next | I2 built, in owner review; I3, I4 lab-checked |
| Palutena | P0 null clone done (A1); P1 approved; P2 look in the lab ROM; P4 specials lab-checked 2026-10-13 (her own code row, `src/port/nds_p4_palutena.c`: Explosive Flame at three distances, Warp in eight directions, Counter / Reflect; statuses 246-257, motions 219-227); P3 normals authored 2026-10-13 (jabs, dash attack, tilts, smashes, aerials on Marth's, Captain's, Mario's and Peach's clips; [card record](characters/palutena.md#move-card-approved-2026-10-13-p1)); A8 her sound set (`scripts/p6/palutena/audio.json`) | P3 lab check next; A9 select portrait, name, emblem and call (host bake); A11 Kirby's copy and hat lab-checked; A12 CPU rows, flame distance and recovery Warp lab-checked; costumes (kit alternates as palettes, `p6_costumes.py`; 0 white, 1 black, 2 red, 3 blue, 4 green) and stock icon; wings, halo, pillar, taunt, entry gate and victory poses (A10, A9) in the lab ROM |

## A1 record

**Plumbing (2026-10-13).** `contents.json` rows `{"id": 19, "name": "ike",
"base": "roy"}` and `{"id": 20, "name": "palutena", "base": "peach"}`.
`p4_contents.py` validates them and adds the helpers every script uses:
`source_row` (the Remix content whose donor data a content starts from),
`table_key` (the per-content table rows it takes: its base's), `token` (its
select token, `IKE`/`PALUTENA`), `code_row` (whose code it runs) and
`special_storage` (the global an own special file loads into). `--list`
prints `name:NAME:TOKEN:id:BASE`. The Makefile defines every registry row's
export (so a base exports when only the authored content is enabled): an
authored one runs `scripts/p6/p6_author.py`, which with no spec writes the
base export plus `content`, `base` and `authored` digests (F4: the rest is
byte-equal). The generator, articles, colour animations, native owner,
Kirby, CSS and audio scripts take the base's rows through these helpers.

- **Code.** Until `src/port/nds_p4_<name>.c` exists, an authored content runs
  its base's code as the base: `NDS_P4_CODE_<BASE>` compiles the base's file
  (`NDS_P4_CODE_NAMES` in the Makefile), `NDS_P4_CODE_ROWS` gives it the
  base's OnStatus, CPU and override hooks, and the base's tests of its own id
  go through `ndsP4CodeContent` (Roy's Dolphin Slash speeds and Kirby's
  Roy-power statuses in `nds_p4_marth.c`). Without this the clones could not
  run their base's specials: Peach's turnip reads `gNdsP4PeachSpecial2` and
  her held turnip needs her item override.
- **Storage and files.** An own special file the authored content shares
  with its base loads into the base's global (Roy's entry wave and beam file
  into `gNdsP4RoySpecial3`), where the base's code reads it. The authored
  content also takes the base's synthesized motion file id: the main file
  names that file, so a per-content id would give the two contents' main
  files (one id) different bytes. Ike's generated data and all 164 files are
  byte-identical to Roy's but for names; the 18 Remix contents' generated
  data is unchanged (Crash's RAM data gains include guards only).
- **Select screen.** Palutena takes row 1 column 0 and Ike row 2 column 0 (the
  two spare cells); each shows its base's portrait, name, logo and call until
  A9. Kirby's copy of either is the base's power (statuses packed once, hat
  texture file shared), A11 gives them their own.
- **Limits checked.** The VS select's preview-kind masks were u32 (12 kinds +
  contents + Metal Mario = 33 bits with twenty contents): now u64. File ids
  stay u16 keys everywhere (`reloc/p4` runs, reloc caches, NitroFS names),
  and no table is sized by the file count, so the P6 window (0x1A00-0x1BFF)
  needs no change. Synthesized ids end at 0x183F (content 20), Kirby's hat
  files at 0x1924 and custom hat files at 0x1944. Renderer owner slots scale
  with `NDS_P4_CONTENT_MAX_ID` (Ike 43, Palutena 44). The lab content knob
  and the P3 descriptor carry a content as a byte.

**Proof (2026-10-13).** All twenty contents built into the playable ROM
(play-p6a1, 113.5 MB), the menu-walk ROM (walk-p6a1) and the lab ROM
(build-lab-p6a1, which with `NDS_FREEZE_DIAGNOSTICS=1` needs two more
lab-only ITCM evictions, `ndsRendererNativeStageMaterialMask` and
`ftDisplayLightsDrawReflect`; no ITCM function changed size between the
18- and 20-content play builds). The play ROM boots to the title with the
`reloc/p4` index at 2,174 files in 145 runs (cap 192).

- Four-CPU mirrors (lab, Dream Land, 1,800 presented frames each): Ike and
  Palutena 0 native failures, 0 lean declines, no exception.
- Kirby against three CPUs of each (lab, Kirby tapping B, 2,400 frames):
  Kirby took Ike's power (Roy's statuses 0x179 and 0x180, Roy's hat 0x2c)
  and Palutena's (Peach's 0x176 and 0x177, her crown 0x2b) again after each
  loss; 0 native failures. One lean camera decline on an Ike fighter in that
  run (a frame drawn with a camera the lean path does not seed); not seen in
  the mirrors or the Palutena run. Open: traced with the KO-camera check
  (BUGS.md, upward KOs).
- Menu-walk ROM: the select commits Ike (0x53) and Palutena (0x54) with both
  3D previews ready, the match plays Ike against Palutena (HUD stocks and
  percentages drawn, 0 native failures) and Results names the winner
  (Palutena, as "PEACH": names and art are the base's until A9). Stills:
  `artifacts/p6/review/2026-10-13_a1/w1/` (local). The select's preview
  failure latch also holds Jigglypuff and Ness on the walk's second pass;
  the 18-content walk ROM (walk-1012a) does the same, so it predates P6.
- P3: the descriptor carries each port's content as a byte (19 and 20 pass
  unchanged); the two-console run is checklist line 13, per fighter.

**Memory.** The two contents cost every scene 16,480 B of heap (heap start
0x0232FCC0 -> 0x02333D20): renderer tables +6,572 B (native owner roots and
programs, entry-effect texels of their article roots), generated data
+4,392 B, renderer .bss +1,352 B, the select overlay +2,528 B (UI kit
surfaces). Every content costs about 8 KB resident; the memory pass must
budget it (moving per-content renderer tables to per-match NitroFS loads,
as the match tables already are, is the largest lever).

**Base budgets.** Per joint, triangles and vertex loads (G_VTX), high / low:

| Joint | Roy high | Roy low | Peach high | Peach low |
|---|---|---|---|---|
| 1 | 14 / 24 | 13 / 21 | | |
| 2 | 20 / 62 | 10 / 34 | 34 / 101 | 21 / 75 |
| 4 | 10 / 20 | 7 / 13 | 30 / 50 | 18 / 35 |
| 5 | 16 / 7 | 7 / 3 | 18 / 17 | 7 / 5 |
| 6 | 10 / 22 | 6 / 16 | 14 / 15 | 9 / 11 |
| 7 | | | 4 / 6 | 3 / 5 |
| 8 | 6 / 10 | 3 / 7 | 85 / 348 | 61 / 322 |
| 9 | 101 / 249 | 65 / 273 | 31 / 50 | 24 / 42 |
| 11 | 10 / 20 | 7 / 13 | 30 / 50 | 18 / 34 |
| 12 | 16 / 7 | 7 / 3 | 18 / 17 | 7 / 5 |
| 13 | 10 / 22 | 4 / 16 | 14 / 15 | 9 / 11 |
| 14 | 38 / 152 | 21 / 128 | | |
| 16 | 8 / 18 | 4 / 12 | | |
| 17 | 8 / 18 | 4 / 12 | 4 / 9 | 3 / 8 |
| 18 | 8 / 12 | 4 / 8 | | |
| 19 | 6 / 7 | 3 / 5 | 11 / 23 | 7 / 17 |
| 20 | 6 / 7 | 3 / 5 | | |
| 22 | 10 / 8 | 6 / 6 | 4 / 9 | 3 / 8 |
| 23 | 10 / 15 | 7 / 13 | | |
| 24 | | | 11 / 23 | 7 / 17 |
| 25 | 12 / 13 | 4 / 10 | 8 / 19 | 4 / 10 |
| 26 | | | 24 / 34 | 10 / 18 |
| 27 | 10 / 8 | 6 / 6 | | |
| 28 | 10 / 15 | 7 / 13 | | |
| 30 | 12 / 13 | 4 / 10 | | |
| Tree | 351 / 735 | 202 / 631 | 340 / 786 | 211 / 623 |

Roy's tree is his whole model (the owner pins' 351 / 202 triangles); Peach's
model parts outside the tree add 31 (371 / 242). The native owner's dense
vertices: Roy 525 / 382, Peach 581 / 420. Main file (setup heap): Roy
156,624 B, Peach 140,000 B; clips: Roy 159 (largest main-motion clip
11,728 B), Peach 149 (12,608 B); shipped files: Roy 164, Peach 156.

## Palutena look record

**3DS to DS (owner route, 2026-10-13 night; approved).** The owner set the
earlier routes aside: "lets try a different strategy ... call it '3DS to DS
Palutena'. we will start from MY baseline 1505 tri palutena". A Blender file
holds two copies of the owner's baseline (`Palutena_Owner_Attempt.blend`, the
kit's low model on its HAL armature); one is never touched and is the bake
source, the other was reduced to the owner's list (one-layer A-line dress
with the same slit, armlets, belt, gems, choker and the face's features as
baked texture, Peach-style blocky hands, minimal calf-length hair with the
kit's hairline, shield and staff wings as sprites, the gem a billboard) and
then hand-tuned with the owner through many rounds: the face, arms, neck and
legs rebuilt from named vertex layouts (automatic collapse decimation looked
"deformed" to the owner every time), the jaw angle, the face texture baked by
front projection with the dark jaw shading lifted (the face read "skinnier
from a distance"), heeled shoes, an inverted-V slit top, skin rejection where
the kit's openings showed through. Result: 428 triangles (face 70, dress 60,
legs 86, hair cap 44 and panels 36, chest and neck 40, arms 40, hands 28,
staff 18, gem, wings and shield 2 each), twelve baked textures. Owner: "Ok i
think we are done" and "this model we created is what i want our palutena to
be". The Blender file and the textures are local (kit-derived, D10).

**Into the game.** The work copy is dumped as JSON (kit space, bone
weights, per-corner uvs, materials). `scripts/p6/look_blend.py` poses each
vertex into Peach's rest pose by its dominant kit bone (`kit_fit.fit`, which
now returns each bone's rotation) and binds it rigidly to one of Peach's
rows: face and hair cap on the head (8), long hair, chest and bodice on the
torso (2), the neck tube on the neck (7), the skirt's waist ring on the torso
and the rest of the skirt on the hips (1, a seam 1 to 2; Peach's dress rows
are not used, their clips scale row 26 up to five times for a floor-length
gown), arms 4/5 and 11/12, hands 6/13, legs 16-19 and 21-24 with seams at the
knees and ankles, the staff on the held-item row (14), the shield on the
forearm (5). The staff is turned so Peach's idle (Wait, averaged) stands it
10 degrees off vertical with the wing sprite facing the battle camera, and
slid up through the hand so its foot meets the floor (Peach's idle hand
hangs at the hip). Textures: CI4 at half the baked size (face, dress, legs,
arms 64x64), shared by both details; 13.5 KB.

**Proportions.** The look keeps the kit's proportions (owner: no
stretching), but Peach's clips key translations: rows 0 and 1 in about 140
of her 165 clips, the torso in 12, neck, head, wrists and knees in 15-25. Rows
whose clips move them around a rest of zero (root, hips, torso) or that the
look does not draw (crown, held point, dress, floor) keep Peach's offsets;
every other row's keyed translation plays at the look's length through
FTAttributes.translate_scales (the look's rest over Peach's per axis; vanilla
Luigi plays Mario's clips this way). `model_writer.py` appends the table to
the main file. The guard multiplies the ShieldPose file's rest rows by the
same table, so every row's rest comes out exact (the thigh roots take a
negative factor); the P4 native guard package cannot scale, so
`p4_shield_pose.py` keeps the raw file for a translate-scaled content (the
source guard path, as Luigi's).

**Low detail.** `scripts/p6/look_reduce.py` reduces each object to the
recipe's target (`art/p6/palutena/look/low.json`) by quadric-error
half-edge collapses that keep mirror symmetry, rows, open borders, UV
islands, a manifold surface and facing; the face keeps its front outline,
the leg tubes four sides, and the shoes' stiletto blocks (a pixel in battle)
go. 278 triangles (face 50, dress 36, legs 56, hair 28 and 22, chest 28, arms
32, hands 12, staff 8 and the three quads). Renders of both details read
back from the written files: `artifacts/p6/review/` (local).

Files: main 0x1B00 and model 0x1B03 (spec `scripts/p6/palutena/spec.json`
"files", from `builds/p6-art/palutena/look/`); the native owner's pins
re-learned (19 drawn rows).

**Lab check (2026-10-13).** Four-CPU lab ROM, Palutena in port 0 on Dream
Land: drawn in every status the CPU took (idle, walk, jumps, Peach's
specials still), 0 native failures, 0 lean declines; the staff stands
beside her in the idle and her feet stay on the floor. The first build drew
nothing: the lean path declined every draw (reason Policy: a run's cull
state against its epoch's). The writer left a two-sided seam material's
no-cull state at the end of the hips' list, and the native owner's replay
carried it into the torso's first run where the runtime did not; every list
now ends in the default state (`model_writer.py`).

**Earlier routes (kept for reference).** Route (owner, 2026-10-13 evening): The owner first picked the decimated
kit (G0), then proposed, and we agreed, to start from another copy of
Peach's model and give it the proportions of the owner's baseline
(`Palutena_Owner_Attempt.blend`: the kit's low model with a HAL armature).
The kit is fitted at Peach's height by uniform scale only, crown top to hair
top (`kit_fit.py`, scale 24.922; the owner: no per-bone stretching). The
look is built in the live Blender with the fitted kit drawn as a wire
overlay on the model, the owner's proportion check.

**Tools.** `look_r.py compose` applies, in order: kit-surface anchors
(`pp_low_anchors.json`), rigid parts (Peach's head at 0.7, hands at 0.75),
mirror symmetry, placed vertices, squashes, kit snaps (vertices pushed along
rays from a centre, axis or bone onto the kit surface, mirror pairs
together), a texture keeper (a moved face corner keeps its drawing; the
eye drawing scaled to the face), dropped base triangles, recolours, authored
and composed textures, then the edits captured in Blender
(`capture_ops.py`: moves, deleted triangles by vertex set, added vertices
and triangles, recolours by colour), then the added parts (`look_k.py`:
tubes, plates, two-sided sprites, a billboard flag). Ops in
`art/p6/palutena/look/` hold parameters, anchors, authored positions and
colours only (D10); scenes, textures and renders are local (`builds/p6-art/`,
`artifacts/p6/review/pp_v*`).

**Low detail, v66 (278 triangles, the plan's cap with the staff and shield;
Peach low 211; the owner: close for the low detail).** The face is back at
the kit's size (owner: 1.1x looked weird), slimmer than the kit's at the
cheekbones, upper jaw and jaw corners (owner); the side locks run from the
temples to the jaw and the face's side points sit behind them (owner: the
ears made the face too wide). The neck is thinner front to back than the
kit's (owner: less pointy). The neck is a column: its base sits on the torso's neck hole, its
front and sides rise to the chin and to two points under the jaw (seam
triangles drawn with the head), the jaw's underside is closed, and its back
meets a flat upper back on the kit's nape. The torso is rebuilt again
(`pp_low_edits_torso.json`): neckline and bust rings of eight points with no
centre points (a flat bust front, no valley; a flat back), waist and skirt
top of six; the collar is one closed surface whose armholes are the arms'
own root rings, so torso and arm meet edge to edge, and Peach's inner cones
of the arms stay so a lowered arm leaves no hole. The slit is the kit's: its
apex on the hip ring at 56 degrees, open at the hem from 38 degrees to the
side. Both legs are skin (owner); the left is one piece, a four-sided thigh
(the front-outer face toward the slit) and a calf seamed to it, wearing one
16x32 texture (`edit_materials` in the ops): the bare upper thigh, the
stocking's lace top, the white stocking (on the left leg only, owner) and
the sandal's crossed straps on the calf. The written look (both details
from the low scene, 0x1A10/0x1A11) reads back with every triangle in place
and the native owner generator accepts it (host). The triangles come from Peach's sleeve
break in both upper arms (one tube from shoulder to elbow) and two wrist caps
hidden in the hands (`pp_low_edits_v64.json`). Earlier (v63) and kept: the
face rebuilt on the kit's face: 17 kit points per half (centre line, the eyes' inner and outer
rings, nose, cheekbones, jaw, chin) and their mirror, the lips three
triangles of their own; the recoloured face frames are mapped by a front
projection that puts the drawn eye on the kit's eye and the mouth frame on
the kit's lips (`pp_low_edits_face.json`). Earlier (v60) and kept: the
torso's rings are sampled on the kit's surface and every band is
triangulated as a half and its mirror image; the dress continues the bodice's waist ring
and runs as one A-line to the hip ring (the owner preferred v40's hips to a
kit-sampled hip ring), its hem lower at the front than the back as the
kit's; the hair ends above the dress so it never passes through it.
Earlier (v51) and kept: Peach's head
kept and scaled (its face since rebuilt, above), Peach's own face frames recoloured (green iris, Palutena's
skin) with the gold circlet painted across the forehead rows (no crown
geometry; ears and crown dropped); hair cap and the long hair curtain on the
kit's hair; strapless bodice in one surface with the skirt, a level
neckline and the bust on the kit chest; arms joined to the torso,
gold bracers; skirt hem round (eight points) with a red, gold and cream hem
texture, two-sided; staff = a three-sided shaft, gold wing sprite and a billboard
gem; shield = a flat round sprite on the left forearm.

**Owner directions on the way (2026-10-13).** Face on the kit's at about
1.1x, then back to 1.0x; no cleavage valley; the neck not fatter, only longer; face
and neck connected; back in line with the neck; chest decimated and smoothed,
back flattened; shoulders connected to the torso; slit as big as the kit's;
the left leg connected; both legs skin, the white stocking on the left leg only; ears
under the hair; neck thinner front to back; face slimmer at the cheeks and
upper jaw; waist and dress one mesh;
face flatter and slightly bigger, cheeks (not the jaw) narrower, forehead
not concave; hair without a missing chunk; calves and shoes from Peach;
hands solid like Peach's and smaller; shoulders seamless with the arms;
chest not "mono", two forms, no points outside the wire; neck area flat
(no point at the upper sternum); every part fills the kit wire as closely
as possible, neither inside nor outside, the face excepted; eyes were
upside down (Peach's face texture runs bottom-up); circlet in the face
texture; staff wings sprites, staff gem a billboard, shield a flat sprite;
slit over the left thigh, toward the side, up almost to the hip.

## Art approvals

One row per owner verdict: date, fighter, gate (G0 bake-off pick, G1 look, G2
clips), what was shown (local paths under `artifacts/p6/review/`, with
hashes), verdict.

| Date | Fighter | Gate | Shown | Verdict |
|---|---|---|---|---|
| 2026-10-13 | Palutena | G0/G1 look | the 3DS-to-DS work copy in the owner's live Blender (428 triangles; Desktop `3DS_to_DS_Palutena.blend`, local) | approved: "this model we created is what i want our palutena to be" |

## Owner decisions

- 2026-10-13 (night): Palutena's look is the "3DS to DS" model built
  with the owner on the owner's baseline (look record); it replaces the
  Peach-restyle route and its high/low plan. Approved as built (428
  triangles); continue P6.
- 2026-10-13 (night): Palutena's high detail is the approved low look
  refined (detail added where close-ups show it, up to about 412 triangles,
  2x textures), not a second restyle of Peach's high model and not the low
  model everywhere.
- 2026-10-13 (evening): G0 route, sizing and review rules: see the
  [look record](#palutena-look-record). Nothing goes into a ROM before the
  owner OKs the look.
- 2026-10-13: D12, after reading plan revision 2: the owner doubts the kit's
  models can be cut to the DS budget and look good without hand-tuning, and
  asked to see both: the kit model reduced with best-effort hand-tuning, and
  Peach restyled into Palutena (same triangle range), each with best effort.
  Planning only for now. Plan revision 3: the G0 look bake-off.
- 2026-10-13: the owner supplied the Super Smash Bros. for Nintendo 3DS kit
  (`decomp/alt_assets/Fighters/Super Smash Bros 3DS/`: both fighters' models
  in two details with skeletons, 8 costumes each, Ragnell, the staff and
  shield, Palutena's wings and entry gate, icon sheets, audio; no
  animations). D10: it is the source of both looks, converted onto Roy's and
  Peach's skeletons at their budgets (the body's source later made a pick by
  D12); converted art is a local build output
  under `builds/p6-art/` and only the recipe and authored layers are
  committed. D11: no Smash 4 motion files exist; clip sources unchanged. The
  kit's audio is the same recordings as the chosen Brawl and Ultimate sets
  (cross-correlation), so D5-D8 stand. Plan revision 2.
- 2026-10-13: both move cards approved as drafted (I1 for Ike, P1 for
  Palutena), when P5-P7 became the active goal.

- 2026-10-12: D1 art committed to the public repository, Ultimate data never;
  D2 Explosive Flame in Ultimate's form; D3 Palutena's dash attack is the
  shield charge; D4 her taunt is Ultimate's down taunt
  ([P6_PLAN.md](P6_PLAN.md) "Owner decisions", with the defaults still open).
- 2026-10-12: the owner supplied Ike's audio: Brawl voices
  (`decomp/alt_assets/Fighters/Super Smash Bros brawl/Ike (International)/`,
  40 WAVs) and Ultimate's Ike sets (`.../Super Smash Bros Ultimate/Fighters/`
  `se_ike`, `se_ike_c01`, `vc_ike`, `vc_ike_c01` and the two Japanese voice
  sets); all local and ignored. D5 Ike's voice is Ultimate's Radiant Dawn set
  (`vc_ike_c01`); D6 his sound effects are Ultimate's (`se_ike`). The Brawl
  clips stay as a reference.
- 2026-10-12: D7 the announcer's calls are Ultimate's narrator clips
  (`vc_narration_characall_ike.wav`, `vc_narration_characall_palutena.wav`,
  owner supplied, local and ignored), level-matched to SSB64's announcer.
- 2026-10-12: D8 the crowd chants are Ultimate's cheers (`vc_ike_cheer.wav`,
  `vc_palutena_cheer.wav`, owner supplied, local and ignored); D9 Palutena's
  victory music is the Remix Kid Icarus "Overworld" excerpt (default approved).

## Environment

- 2026-10-13: Blender 5.2.2 LTS has no COLLADA importer (FBX and glTF only);
  the kit's .dae files go through `scripts/p6/dae_reader.py` (A3). The 3DS
  folder was untracked and not ignored; `.gitignore` now ignores it beside
  the Ultimate and Brawl folders. The owner then installed the third-party
  "Simple COLLADA" add-on (ekztal v0.7.2, in the user add-ons folder); it
  builds an empty armature on every kit file (node ids vs skin sids), so the
  project reader stays canonical ([P6_PLAN.md](P6_PLAN.md) "The 3DS kit").
- 2026-10-12: Blender MCP registered with Claude Code for this project
  (`blender`, local scope: `uvx mcp-for-blender@2.1.9`, `DISABLE_TELEMETRY=true`).
  The MCP for Blender add-on (v1.8, protocol 13) is enabled in Blender 5.2 and
  starts its socket on port 9876 when Blender opens with its UI; checked with
  `get_addon_info` (protocol 13). The tools load in sessions started after the
  registration.
