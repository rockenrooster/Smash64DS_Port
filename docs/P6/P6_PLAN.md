# P6 — Ike and Palutena: implementation plan

**Revision:** 3 (2026-10-13): each look is picked from two best-effort
candidates at the same budget, the 3DS kit's model rebuilt and hand-tuned or
the base's model restyled ([Look bake-off](#look-bake-off-g0), D12; Palutena
first). Revision 2 (2026-10-13): the owner's Super Smash Bros. for Nintendo
3DS kit (D10, D11). Revision 1: 2026-10-12.
**Inputs:** the owner's briefs [Ike.md](Ike.md) and [Palutena.md](Palutena.md)
(kept as written; this plan records where a decision overrides a line) and the
3DS kit ([The 3DS kit](#the-3ds-kit)).
**Live state:** [P6_STATUS.md](P6_STATUS.md). **Cards:**
[characters/ike.md](characters/ike.md), [characters/palutena.md](characters/palutena.md).
**Machinery reused:** [P4 master plan](../P4/New_Characters.md),
[P4_STATUS](../P4/P4_STATUS.md).

## What P6 is

Two fighters that no source has: Ike (Fire Emblem) at the polish of Roy and
Marth, and Palutena (Kid Icarus) at the polish of Peach.

P4 lowered Smash Remix's assembled fighters. P6 has no donor to lower, but it
does not need new runtime machinery either. Everything after a content's
export is reused unchanged: the generator, native owners, select and battle
packs, shield-pose packages, the animation stream pack, Kirby powers and hats,
the select screen, HUD and Results, sounds, CPU rows, the memory work and the
lab probes. The new work sits in front of the export: an authoring pipeline
that writes the same export directory a Remix fighter has, built from

- a **base content's export**: Ike starts from Roy (setup parent Captain, the
  Marth/Roy JointTree and its large clip library), Palutena from Peach (setup
  parent Fox). The base gives the skeleton, the clips, the scripts and the
  tables, and its model is one of the two look candidates (D12);
- the **3DS kit** (D10): both fighters' Smash for 3DS models, the other look
  candidate, and the source of the weapons, wings, gate, icons and costume
  colours. A local input, never committed;
- **committed recipes and authored layers**: the look recipes of both tracks,
  the authored clip layers and the few pieces neither source has;
- a **committed spec**: statuses, motion scripts, attribute deltas, sounds and
  presentation rows.

A P6 fighter is therefore a P4 content: a registry row, `NDS_P4_<NAME>=1`,
owner pins and `src/port/nds_p4_<name>.c`. The few new runtime seams are P4
style (a content override at a source site) and are listed with their items.

## Product contract

- Same as P4: original DS envelope; release tier VS and Training; any 2-4
  fighter selection, including mirrors, costumes, teams, items and Kirby
  copies; four-CPU WORK P95 <= 1,120,000 with >= 95% of frames in two
  VBlanks; replay digest identical, 0 native failures, 0 motion reads after
  GO, heap low-water >= 25,600; residency per selected match.
- **Fidelity source:** the owner briefs, then each fighter's move card
  (approved 2026-10-13). The look follows the kit's designs (Smash 4's Ike
  keeps Brawl's design; Smash 4's Palutena is the design Ultimate keeps) at
  SSB64's budget; how each body is built is the owner's pick (D12). Brawl
  (Ike) and Ultimate (Palutena) define the intent of each move; their frame
  data does not bind. Numbers are set on SSB64's scale against the base
  fighter in the lab, and the owner judges the feel in playtests.
- **Identity:** content 19 Ike and 20 Palutena (character-select ids
  `NDS_P4_SEL_BASE` + content: 0x53 and 0x54). They are stable once
  assigned: saved, sent in the P3 descriptor.
- **Outside P6:** unlock conditions, "Challenger Approaching" fights, unlock
  screens and save records (P7); balance beyond the briefs (P7); the 1P and
  bonus modes (a later tier, as in P4); stages (P5).
- **P7 alignment:** [P7's plan](../P7/P7_Implementation_Plan.md) numbers
  content n as selection id 12 + n, so Ike is 31 and Palutena 32 there. Its
  unlock rows for both stay dormant until these contents compile, and new
  fighters are VS-only (P7 owner decision 1). P6 needs nothing from P7.

## The 3DS kit

`decomp/alt_assets/Fighters/Super Smash Bros 3DS/`, supplied by the owner on
2026-10-13 and ignored by `.gitignore`. Measured 2026-10-13 (triangles "drawn
at once" count the body, the neutral face set, one hand pose a side and the
weapon):

| Part | Ike | Palutena | Use in P6 |
|---|---|---|---|
| Model, two details (COLLADA, skinned, at most 2 influences a vertex) | `ike_{high,low}_n_c00.dae`: 3,321 / 1,515 triangles drawn at once; 3,726 / 2,530 vertices | `palutena_{high,low}_n_c00.dae`: 3,550 / 1,930 triangles; 4,141 / 2,615 vertices | Track K's source (A3b); Track R's shape reference (A3c) |
| Skeleton | 33 joints, HAL's names (TransN, RotN, HipN, legs, WaistN, BustN, arms, NeckN, HeadN, LHaveN, RHaveN, SwordN, ThrowN) and swing joints for the cape and headband tails | 37 joints: the same body chain, skirt joints (LSkirtJa/b, RSkirtJa/b), hair swing joints, ShieldN, RStickN (staff) | the fit's source; mapped onto the base's JointTree by name and role |
| Weapons | Ragnell: 98 / 290 triangles in the model, and `Ike/Sword/sword.dae` | staff 267 / 564 and shield 8 / 29 triangles in the model | rebuilt in both tracks: Ragnell replaces Roy's sword part; the staff takes Peach's held-item part; the shield sits on her left forearm |
| Effect meshes | — | `Palutena/gate/gate.dae` (two doors, 216 triangles, 4 joints); `Palutena/godwing/godwing.dae` (wings, 774 triangles, 11 joints) | entry article; wing joints (rebuilt after the pick) |
| Textures (low; high is twice the size) | body 256x256, face 128x128, hair and cape cards 128x128, Ragnell 128x128 | body 256x256, face 128x64, hair and lash cards 128x64, gate and wing 256x256 | texel source for both tracks (baked or projected), at the base's texture budget |
| Costumes | 8: the default and 7 alternates (`Alternate/c01-c07` low, `h01-h07` high), full recolours on shared UVs | 8, the same (`Alternates/`) | the base's count chosen from them, as palettes |
| Face and hands | one eye and one mouth mesh per expression (blink, half blink, hurt, attack and more) over the face texture's atlas; three hand poses a side | the same, plus eyebrow and lash sets | the neutral set baked into the face texture; the base's face frames and hand variants matched to the kit's |
| Icons | every 3DS fighter's select portrait (about 130 px) and stock icons (one row a fighter, 8 costumes, about 15 px) | the same two sheets | stock icon per costume (A9); framing reference for the portrait |
| Audio | voice 39, effects 65, crowd cheer (mono, 14-16 kHz, numbered) | voice 52, effects 51, cheer | not used (below) |
| Not in the kit | any animation (D11), effect textures | | clips as Board 2 plans them |

Points the readers and both tracks must handle:

- Blender 5.2 has no COLLADA importer (5.2.2 LTS checked 2026-10-13: FBX and
  glTF only), so the kit is read by the project's own pure-Python reader (A3).
  The third-party "Simple COLLADA" add-on the owner installed (ekztal, v0.7.2,
  the repository's latest, no license; audited 2026-10-13: XML parsing and
  `bpy` only) imports the kit's meshes and textures but not its rig. It looks
  up joints by node id (`TransN_bone_id`) where the skins name them by sid
  (`TransN`), so its armature has no bones in any of the six kit files. It
  also leaves the files Y-up (its up-axis matrix is computed but never
  applied), drops the normals stored in `<vertices>`, keeps each mesh's whole
  vertex pool as loose vertices and ignores Palutena's mirrored UVs. It is a
  quick mesh viewer at most; it cannot be vendored without a license.
- Every geometry in a kit file indexes the model's whole vertex pool; the face
  expressions and hand poses are separate meshes over that pool.
- Palutena's UVs expect mirrored-repeat addressing (the kit's
  `Mirror UV.txt`), although her files declare plain wrap.
- The metal, gold, specular and reflection textures have no SSB64 use.

The kit's audio holds the same recordings as the sets already chosen
(envelope cross-correlation, 2026-10-13): Ike's voice is the Brawl set (37 of
39 clips match outright) and his effects are Ultimate's `se_ike` (58 of 65);
Palutena's voice matches Ultimate's `vc_palutena` in 40 of 52 clips and her
effects `se_palutena` in 50 of 51; both cheers are Ultimate's (D8). D5-D8
stand, with Ultimate as the source (named clips, higher rate). Palutena's 12
unmatched 3DS voice lines are unidentified spares.

## Owner decisions

2026-10-12 (D1-D4 asked with this plan; D5-D9 after the owner added Ike's
Brawl and Ultimate audio, the announcer clips and the chants); 2026-10-13
(D10-D11 after the owner added the 3DS kit; D12 after the owner reviewed
revision 2):

| # | Decision |
|---|---|
| D1 | Authored art is **committed to this (public) repository**: Blender sources and their exports. Ripped game data is never committed: the Ultimate Palutena model, textures and WAVs and Ike's Ultimate WAVs (`decomp/alt_assets/Fighters/Super Smash Bros Ultimate/`) and the Brawl Ike voices (`decomp/alt_assets/Fighters/Super Smash Bros brawl/`) stay local and ignored, and only build output under `builds/` derives from them. |
| D10 | The **3DS kit** (`decomp/alt_assets/Fighters/Super Smash Bros 3DS/`) is a source for both fighters' art: models, textures, costumes, weapons, Palutena's wings and her entry gate (whether a body comes from it is D12's pick). **Kit-derived art is a local build output**: committed are the recipes (joint map, fit, budgets, scripted fixes and hand-tuning ops, costume and palette picks), the authored clip layers and the pieces neither source has; the built meshes, textures, icons and their .blend regenerate under `builds/p6-art/` from the ignored kit, as the Remix staging does. D1 still governs hand-made art. |
| D11 | No Smash 4 (3DS) motion files are available: the clip sources stay as Board 2 plans them. |
| D12 | The owner doubts that the kit's models, cut to the DS budget, can look good without hand-tuning. Palutena's look is therefore built twice, each with best effort and at the same budget: **Track K**, the kit's model rebuilt at the budget and hand-tuned; **Track R**, Peach's model restyled into Palutena, which starts in the same triangle range. The owner judges both on one sheet and picks ([Look bake-off](#look-bake-off-g0)). |
| D5 | Ike's voice is **Ultimate's Radiant Dawn set** (`vc_ike_c01`: Greg Chun in the older Ike's tone, 36 clips named by move, 48 kHz), asked after the owner added Ultimate's Ike audio. It replaces the brief's "Roy's audio with pitch changes" for every voice slot, Aether's shout included. The Brawl clips the owner supplied first (40 WAVs) stay local as a reference. |
| D6 | Ike's sound effects are **Ultimate's** (`se_ike`: swings, Eruption, Quick Draw, Aether, Counter, entry, steps, jumps, landings; owner supplied). They replace Roy's for Ike. |
| D7 | The announcer's name calls are **Ultimate's narrator clips** (owner supplied: `vc_narration_characall_ike.wav` 0.87 s and `vc_narration_characall_palutena.wav` 1.31 s, 48 kHz), level-matched to SSB64's announcer cues and used wherever a content's call plays (the select screen and Results). |
| D8 | The crowd chants are **Ultimate's cheers** (owner supplied: `vc_ike_cheer.wav` 1.50 s and `vc_palutena_cheer.wav` 1.72 s, stereo 48 kHz, downmixed to mono), on each content's crowd-chant row as Remix's chants are. |
| D9 | Palutena's victory music is **an excerpt of Remix's Kid Icarus "Overworld"** sequence (`MIDI.asm` SKYWORLD), rendered as a short jingle (owner approved the default). |
| D2 | Palutena's neutral B is **Ultimate's Explosive Flame**: no projectile; the multi-hit explosion appears ahead of her at a distance the D-pad picks. |
| D3 | Palutena's dash attack is **Ultimate's shield charge**. The brief's "running side A" line was copied from Ike's. |
| D4 | Palutena's taunt is **Ultimate's down taunt**. |

Defaults this plan uses until the owner says otherwise. Each one is a small
change late in its wave:

| Default | Alternative |
|---|---|
| The select grid's two '?' cells take the new fighters: Palutena at row 1 column 0, Ike at row 2 column 0. The 11x3 grid is then full (33 of 33). | Shift row 2 so Ike sits beside Roy and Marth. |
| Ike's victory music: Marth and Roy's Fire Emblem theme. | A track the owner names. |
| Attributes: Ike takes Roy's, Palutena Peach's. Peach's float is her own code and is not inherited. Balance tuning belongs to P7. | Brawl-like heavier and slower Ike. |
| Bake-off scope: Palutena's body, staff and shield, default costume, both details. Costumes, wings, halo, pillar and gate follow the pick. | More on the sheet (a costume, the wings). |
| Ike's look follows Palutena's pick: K, the kit's Ike rebuilt onto Roy's skeleton; R, Roy restyled into Ike. | Ike gets his own two-track sheet before I2. |
| Costumes: the base's count, chosen from the kit's eight (the default first), approved on a palette sheet. | Other picks from the eight. |
| Select portrait: rendered from the picked high model in the SSB64 portrait style, framed after the kit's portrait. | The kit's 3DS portrait, reduced to the portrait format. |
| Stock icon: the kit's stock icons, one LUT per costume. | Rendered from the picked model. |
| Explosive Flame distance: D-pad back, neutral or forward at the cast frame picks near, mid or far. The DS D-pad is digital (the stick reads only 0 or +-80, `controller_backend.c`), so Ultimate's tilt-amount distance cannot be read. | |

## Look bake-off (G0)

D12: Palutena's look is built twice with the same effort, budget, checklist
and review sheet, and the owner picks.

| | Track K: the kit rebuilt | Track R: the base restyled |
|---|---|---|
| Start | the kit's model fitted onto Peach's skeleton (1,930 triangles drawn in its low model; about seven times the budget) | Peach's model (371 high / 242 low triangles), already split and stitched for her clips |
| Method | **ring retopology**, not automatic reduction: each part's kit surface is sliced at stations along its bone, each slice is fitted with a small polygon, and the rings are bridged into SSB64-style tubes and capped; hair, dress, hands, staff and shield take shaped variants (ribbons, an open cone for the slit dress, mittens, prisms, plates). Then hand-tuned | Peach's mesh edited by region against the fitted kit model overlaid in her rest pose: the gown pulled into the slim dress with its high slit, the puffed sleeves and gloves into bare arms with armlets, the hair lengthened to her knees and split between the head and upper-back joints, the crown reshaped into the tiara. Then hand-tuned |
| Texels | the kit's albedo baked onto new per-part UVs, then retouched (palette, eyes, trim) | the kit's albedo projected onto Peach's UVs, then retouched, at her texture budget |
| Strengths | the true proportions and details | starts inside the budget; her proven joint seams; SSB64 topology from the start |
| Risks | new joint seams to prove; can read as "decimated" | the distance from a ball gown to a slim slit dress; hair length past Peach's |

Shared by both, so the sheet compares the bodies: the joint fit and budgets;
the staff and shield (one part, rebuilt from the kit's by ring retopology);
the capture tool, harness and checklist.

**Budget split.** Starting targets, the same for both tracks; A1's per-joint
counts of Peach calibrate them:

| Region | Low | High |
|---|---|---|
| Head and face | 30 | 48 |
| Hair | 40 | 70 |
| Tiara | 6 | 12 |
| Torso | 24 | 40 |
| Dress | 40 | 64 |
| Arms | 32 | 48 |
| Hands | 20 | 32 |
| Legs | 32 | 44 |
| Feet | 12 | 16 |
| Staff | 18 | 28 |
| Shield | 8 | 10 |
| **Total (budget)** | **262 (278)** | **412 (426)** |

**Best effort, the same for both.** Each part is built in the live Blender
(the MCP), part by part in the same order for both tracks (head and hair,
torso and dress, arms and hands, legs and feet, staff and shield), and goes
through at least three rounds of render, check and fix before the sheet. The
checklist:

- the silhouette against the kit model at battle size, front, side and
  three-quarter;
- no hole and no sliver triangle thinner than about 2 pixels at battle size;
- joint seams closed in six of Peach's clips (wait, run, jump, crouch, a
  smash, damage), both details;
- the face readable at select-screen size; texel density even across parts;
  colours within the palette budget;
- totals within the budget, each region within its target by 20%.

**Hand-tuning is captured as ops, not stored as geometry.**
`scripts/p6/blender/capture_ops.py` writes the edited mesh back as committed
ops: a vertex placed on the kit's surface is an anchor (kit triangle index,
barycentric coordinates, an optional offset along the normal), a free-hand
move is an offset (on a base vertex in Track R, on the rebuilt vertex in
Track K), and faces, UV seams and authored UVs are listed as they are. The
headless rebuild replays the ops from the kit and the base export. No kit
coordinate or texel enters a tracked file (D10), and two rebuilds are
byte-identical (F6).

**The sheet** (A6) shows Track K, Track R, the kit model and Peach side by
side in the same views:

1. battle size: the low model at the size the default battle camera draws a
   fighter (measured from a lab still on Final Destination), at 256x192 with
   nearest filtering and 15-bit colour, and enlarged 3x;
2. close-up: the high model in an eight-view turntable at select-screen and
   Results scale;
3. poses: the six clips of the checklist, both details;
4. counts: triangles and vertices per region and in total, and texture bytes,
   against the budget and Peach's;
5. lab ROM stills (melonDS): each candidate as Palutena's model in a
   four-fighter VS match and in the select-screen preview.

The owner picks K, R, or K for some regions and R for others (both bind to
the same joints, so regions mix). The pick is logged in P6_STATUS and written
to `art/p6/palutena/recipe.json`; the losing track's ops stay committed for
reference, its builds are deleted.

## Architecture: the authored export

```
decomp/alt_assets/Fighters/Super Smash Bros 3DS/<Name>/   local, ignored (D10)
builds/p4-staging/export/<base>/            local: the base's skeleton, model, clips
   │  scripts/p6/dae_reader.py                     pure-Python COLLADA reader
   │  scripts/p6/blender/kit_rebuild.py            Track K (A3b) ┐ Blender 5.2 LTS,
   │  scripts/p6/blender/restyle_base.py           Track R (A3c) ┘ headless
   │     ◄── art/p6/<name>/look/{k,r}.json         committed: ring stations, anchors,
   │                                               offsets, faces, UV seams
   │     ◄── art/p6/<name>/recipe.json             committed: fit, budgets, the pick
   │                                               (per region), costume picks
   ▼
builds/p6-art/<name>/          local: model.json, textures/*.png, palettes.json,
   │                           icons/, <name>.local.blend, manifest.json
   │                           (k/ and r/ side by side while G0 runs)
   │
   ├── art/p6/<name>/anims/                    committed: authored clip layers
   ├── art/p6/<name>/authored.blend + export/  committed: pieces neither source has
   ├── scripts/p6/<name>/spec.json + motions/*.ms     (committed)
   ├── builds/p4-staging/export/<base>/               (local, ROM-derived)
   ▼
scripts/p6/p6_author.py
   ▼
builds/p4-staging/export/<name>/   resolved.json, events.json, o2r/, summary.txt
   │                               (the layout remix_export.py writes)
   ▼
the P4 rules, unchanged: generate_p4_fighter.py, p4_native_owner.py,
p4_preview_pack.py, p4_shield_pose.py, p4_anim_stream.py, p4_audio.py,
p4_kirby.py, p4_kirby_hats.py, p4_kirby_custom_hats.py, p4_articles.py,
p4_colanims.py, p4_css.py
```

- **Blender is an art tool, not a ROM-build dependency.** The look build is
  its own target (`scripts/p6/convert_art.ps1`, one fighter or both; both
  tracks while G0 runs, the pick after): it runs Blender 5.2 LTS headless and
  writes `builds/p6-art/<name>/`. Its manifest records the SHA-256 of every
  kit and base input, the recipe and ops, the scripts and the Blender
  version. ROM builds read `builds/p6-art/` only and stop with a message
  naming the target when the manifest is missing or stale. The same freshness
  rule covers the committed `authored.blend` and its export.
- **Base data stays the base's.** Roy's and Marth's clips, Peach's clips and
  the base's special files keep their file ids. Only new or changed files get
  new ids (the picked model's files among them), so Ike in a match with Roy or
  Marth loads their shared clips once.
- **Ids.** New files take a P6 window (proposed 0x1A00-0x1BFF, 0x100 a
  content; the donor files end at 0x16DA and the synthesized and Kirby files
  at 0x1944). A1 checks every table indexed by file id before fixing it. New
  statuses append to the content's own action array (S14), new motions to its
  motion array, and authored motion scripts enter the generator as inserted
  streams in a reserved address window, so the synthesized main-motion file
  and its round-trip check run as they do for Remix scripts.
- **Code.** `src/port/nds_p4_ike.c` and `nds_p4_palutena.c`. Ike calls Roy's
  Flare Blade port and Marth's counter; `nds_p4_roy.c` and `nds_p4_marth.c`
  compile when Ike is enabled and they are not, the pattern Roy already uses
  for Marth.
- **Tools.** `scripts/p6/` is a new area folder (added to `_paths.py`
  `_AREA_DIRS`); scripts that run inside Blender live in `scripts/p6/blender/`
  and import only `bpy`, `scripts/p6/dae_reader.py` and the pure-Python codecs.
- **Art layout.** Committed: `art/p6/<name>/recipe.json`, `art/p6/<name>/look/`
  (both tracks' ops), `art/p6/<name>/anims/` (authored clip layers),
  `art/p6/<name>/authored.blend` and its export (the pieces neither source
  has), `art/p6/common/` (the Kid Icarus emblem). Local: `builds/p6-art/<name>/`
  (the built look and its .blend) and `artifacts/p6/review/` (review renders
  and the G0 sheet). Ignored: the three rip folders, `art/**/*.local.blend`
  (scenes that link kit or Ultimate files) and `art/**/voice.local/`.

## Board 1 — authoring machinery

Each item is built once and proven by Ike first, except A3b and A3c, which
G0 proves on Palutena.

| # | Item | Done when |
|---|---|---|
| A1 | **Registry, build and null clones.** `contents.json` rows with `base` instead of `remix` (19 ike: roy, 20 palutena: peach); `p4_contents.py`, `p4_rules.mk` and `p4_css.py` accept them; `NDS_P4_IKE`, `NDS_P4_PALUTENA`; the lab content knob takes 19 and 20; the file-id window is checked; `p6_author.py` runs with an empty spec. Per-content tables sized by a literal instead of `NDS_P4_CONTENT_LIMIT` are found and fixed. | Ike as Roy and Palutena as Peach, under ids 19 and 20 and without the base's code overrides, build into the lab, menu-walk and playable ROMs and play four-CPU mirrors with 0 native failures; select cell, Results, HUD, Kirby copy and the P3 descriptor work. The base budgets are measured (triangles and vertices per joint, textures, clips, setup heap, low-water, P95). |
| A2 | **Model reader and writer** (`scripts/p6/n64_model.py`): both JointTrees, display lists (including the cross-joint vertex loads and bare-ENDDL joints Remix models use), vertices, MObjs and their costume palettes, model-part variants, face texture frames, sprites. The writer re-emits the base's own model with edits (Track R) and emits fresh geometry (Track K): per-joint display lists, vertex batches sized to the vertex cache, cross-joint loads for seam triangles, and material spans of the kinds the base uses (lit texture, lit primitive colour, two-sided), with new CI textures and costume palettes. The parsing already spread over the owner generator and the hat scripts moves into this module. | F1: Roy and Peach read and written back with no edits give byte-identical files (at least identical display lists), identical native owner images, and unchanged `p4_native_owner.py --learn` pins. |
| A3 | **Blender bridge** (Blender 5.2.2 LTS, headless): import a content's model and clips into a .blend (armature from the JointTree, rigid per-joint meshes, cross-joint stitch groups, MObj materials with costume palettes, clips as actions) and export back to the intermediate. **Kit import:** `scripts/p6/dae_reader.py` (pure Python, `xml.etree`) reads the kit's models, skins, skeletons (joints by sid), normals, materials and texture paths; the bridge builds an armature and skinned meshes from it, Z-up, each mesh trimmed to the vertices it uses. **Fit** (shared by both tracks): the kit's joints map onto the base's by name and role (TransN and RotN onto TransN, XRotN and YRotN; legs, waist, bust, arms, neck, head; HaveN onto the held-item joints; RStickN onto the staff part, ShieldN onto the forearm; swing joints for capes, headband tails, hair and skirt onto the base's matching joints or the nearest one); the kit model is scaled to the base's height and posed so every mapped joint lands on the base's joint in its rest pose. The two-influence skin carries the mesh: limbs take the base's lengths, widths stay the kit's. Interactive sessions use the MCP for Blender add-on (v1.8, protocol 13, in Blender 5.2), registered with Claude Code on 2026-10-12 as the project-local server `blender` (`mcp-for-blender@2.1.9`, pinned to the add-on's protocol; telemetry off); Blender must be open, and its add-on listens on port 9876. The headless scripts stay the canonical path. | Roy imported and exported with no edits reproduces F1; every kit file reads with the counts in [The 3DS kit](#the-3ds-kit) (part of F6); the fitted kit Palutena overlays Peach in her rest pose. |
| A3b | **Track K: the kit rebuilt** (`scripts/p6/blender/kit_rebuild.py`, ops in `art/p6/<name>/look/k.json`). (1) **Select** the default model, the neutral eye and mouth set, the hand poses and the weapons. (2) **Ring retopology** on the fitted kit: per part, stations along its bone (count, polygon sides, twist, and the stations a parent and child share, all from the ops) fitted to the kit's cross-sections; rings bridged, ends capped; shared joint stations are the stitch rings, so bends stay closed; shaped variants for hair (ribbons), the dress (an open cone for the slit), hands (mittens), the staff (prisms) and the shield (a plate). (3) **Hand-tuning** in the live Blender, captured by `capture_ops.py` (G0). (4) **Bind** each vertex to its part's joint; seam triangles become the writer's cross-joint triangles. (5) **Bake** the kit's albedo onto the per-part UVs at the base's texture budget (SSB64 lights by vertex normals, so no baked shading), eyes and mouth into the face texture (no decal geometry), mirrored-repeat sampling for Palutena; alpha cards (hair strands, lashes) become opaque geometry or 1-bit cutout texels, no translucent faces; quantize to CI4 or CI8. **Costumes:** one index image and one palette per chosen costume, each computed from the matching kit alternate (shared UVs), so a costume costs a palette, not a texture. **Face frames:** one baked face per face texture frame the base has, from the kit's matching expressions. (6) **Write** model.json, textures, palettes, icons (A9) and the manifest under `builds/p6-art/<name>/`, and `<name>.local.blend`. | F6: every kit file reads with its counts; two rebuilds from the committed ops give byte-identical outputs; the written model reads back with the same triangles and joints. The candidate plays the base's clips in the lab within budget with 0 lean declines and 0 native failures. |
| A3c | **Track R: the base restyled** (`scripts/p6/blender/restyle_base.py`, ops in `art/p6/<name>/look/r.json`). The base model (A2, A3) opens with the fitted kit model overlaid in the base's rest pose. Edits by region: move, extrude, split, dissolve, delete, add faces. The base's joint splits and seams stay wherever geometry stays on its joint; new geometry (longer hair, tiara, armlets) binds as the base's does. Edits are captured by `capture_ops.py` as base-vertex offsets and kit-surface anchors. Texels: the kit's albedo projected onto the base's UVs (new UVs only where geometry is new), retouched, at the base's texture budget; costumes and face frames as in A3b. The staff and shield are A3b's shared part. | F6 for this track; the candidate plays the base's clips in the lab within budget with 0 lean declines and 0 native failures. |
| A4 | **Animation codec and retarget.** Figatree reader and writer: sample at the game's 60 frames a second, write dense linear keys, then `p4_anim_compact.py`. Track transforms: retime, mirror, splice, per-joint override (Palutena's staff hold). Retarget between humanoid skeletons by joint-role maps (Mario, Luigi, Link, Captain, Ness, Fox, Marth/Roy, Peach, Sheik, Ganondorf); A3's fit uses the same role map. Authored edits are stored as layers (pose sheets, per-joint key overrides, the transforms above) over a base or retargeted clip; the composed clip is generated. | F2: every Roy, Marth and Peach clip decodes, re-encodes and decodes equal within the compactor's tolerance (`ANIM_EPS`); a retargeted Captain jab plays on Ike's skeleton. |
| A5 | **Motion-script assembler** (`scripts/p6/mscript.py`): a text form and bytes, both ways. One op table with `remix_export.py`'s decoder, so the names are the ones `events.json` already prints (MakeAttackColl, ClearAttackCollAll, SyncWait, Effect, PlayFGM, SetFlag0, SetModelPartID, RemixFrameSpeedMultiplier and the rest; 49 ops appear in Roy, Marth and Peach). Field layouts come from BattleShip's motion-event structures. Script edits for reused moves: scale damage or knockback, insert a frame-speed command, drop Effect commands of a kind, resize hitboxes, swap sounds. | F3: every script of the eighteen exports disassembles and re-assembles byte-identical. |
| A6 | **Review harness** (`scripts/p6/blender/review.py`): turntables per costume and detail beside the base and beside the kit model in the same pose; per clip a contact sheet and a 30 fps GIF with the script's hitboxes drawn as spheres on their joints (hurtboxes optional) and sound cues marked; the **G0 sheet** ([Look bake-off](#look-bake-off-g0)): battle size at DS resolution, close-ups, poses, counts and lab stills for K, R, the kit model and the base. Output goes to `artifacts/p6/review/<date>/` (local) and to the owner. | Roy's forward smash renders with its hitboxes where the lab's hitbox census puts them; the sheet's battle-size view matches a lab still of Peach within a pixel of height. |
| A7 | **Authored export** (`scripts/p6/p6_author.py`): base export + spec + the built look (`builds/p6-art/`) + committed layers in, export directory out. File substitutions and new ids; appended motions and statuses whose callbacks name DS ports or decomp functions; kind-table rows (special starters, Kirby rows, jab and rapid-jab rows, entry, Results rows, winner music and call); attribute deltas; sword-trail rows; authored colour animations. The generator learns one authored-callback marker and nothing else. | F4: an empty spec reproduces the base export but for identity; Ike's spec builds; a G0 candidate substitutes for Palutena's null-clone model. |
| A8 | **Audio.** Authored cues from local WAVs (trim, normalize, resample, IMA) through `p4_audio.py`'s pack writer, FGM ids past Remix's: Ultimate's voices and effects for both fighters (D5, D6), the narrator's name calls (D7, level-matched to the source announcer) and the crowd cheers (D8, mono). The kit's audio is not used (the same recordings; [The 3DS kit](#the-3ds-kit)). Each spec maps every voice slot and every sound command of its scripts, reused or new, to a clip; SSB64's own hit sounds stay. Victory jingle: `render-audio-bgm.py` with an excerpt and fade option. A missing local folder fails that content's build with a message naming it. | The owner approves each fighter's slot map by ear (the sample sheet); four-CPU mirrors play 0 FGM misses. |
| A9 | **Presentation.** Select portrait rendered from the picked high model with a portrait preset (framing after the kit's portrait) and quantized to Remix's 32x32 portrait format; name textures composed from the existing name font's glyphs at build time; series emblems (Fire Emblem's exists; Kid Icarus authored); Results rows and menu zoom; menu clips. Stock icon: the kit's default-costume icon reduced to the HUD's 8x8 OBJ4 cell, with one LUT per costume from the matching kit icon (the format `generate_p4_fighter.py` writes). Portrait and icon are kit-derived (local, D10). | Both cells, previews, HUD and Results draw on the menu and playable ROMs. |
| A10 | **Effects.** A content tint for effect makers and colour animations (Ike's blue fire, also on a Kirby holding Ike's power). Articles through `p4_articles.py`: Palutena's entry gate (the kit's gate, rebuilt as in A3b) and her explosion (authored). Effects that move with Palutena are joints of her own tree, animated by her clips and shown by SetModelPartID: the wings (the kit's, rebuilt as in A3b), the halo and the light pillar (authored). | Lab: each effect drawn natively, 0 declines, 0 native failures. |
| A11 | **Kirby.** Ike's power is Roy's power rows with Ike's tint and hat. Palutena's is a custom power (`p4_kirby.py` CUSTOM_POWERS) on Kirby's own PK Fire clips. Hats come from each picked model's head spans (`p4_kirby_custom_hats.py`). | Kirby copies both and draws their hats, 0 weapon or effect refusals. |
| A12 | **CPU.** Attack-coverage rows computed from clips and scripts (each hitbox's world box per frame); recovery and post-process routines per fighter. | F5: the tool reproduces the decomp's coverage rows for two original fighters within tolerance; four-CPU lab matches use every authored attack. |
| A13 | **Checks.** `scripts/p6/check_p6_art.py`: the look manifest and the authored export fresh; no 3DS-, Ultimate- or Brawl-derived data in a tracked file (packed images, paths into the rip folders or `builds/p6-art/`, coordinates in the look ops other than anchors and offsets, hashes of tracked files against the built outputs and against the rip folders when present); no WAV tracked. The ignore rules for the three rip folders and for the local art files are in `.gitignore` (2026-10-12; the 3DS folder 2026-10-13); `builds/` and `artifacts/` were already ignored. F1-F6 run as one host test. | Mutation-tested: each check goes red on a planted fault. |

**Order.** A1 first: the plumbing is proven before any art exists. Then A2
and A3 (F1, the kit reader and the fit), then A3b and A3c on Palutena up to
the G0 sheet, both tracks through the same rounds, while A5 proceeds (F3).
The owner's pick goes to P2 and, by default, to Ike's I2. A4 and A7 land with
Ike's first clips. A8-A12 land as Ike needs them, the WAV ingest included
(D5, D6); the Palutena-only parts (wings and gate, joint effects, articles,
the custom Kirby power) land in Wave 2.

## Board 2 — fighters

Ike goes first. His base shares the Marth/Roy skeleton, so most of his clips
already exist and his new routines are few: he proves the pipeline on a small
delta. Palutena follows with the large animation load and the new audio
ingest. Her look comes first, though: G0 runs during Wave 1, because its pick
also decides how Ike's look is built.

### G0 — look bake-off (Palutena, during Wave 1)

| Step | Work | Gate |
|---|---|---|
| G0a | Track K: the kit's Palutena rebuilt at the budget and hand-tuned (A3b) | the checklist, three rounds |
| G0b | Track R: Peach restyled into Palutena (A3c) | the checklist, three rounds |
| G0c | The sheet and lab stills of both, beside the kit model and Peach | the owner picks K, R or per region |

### Wave 1 — Ike

| Step | Work | Gate |
|---|---|---|
| I0 | Null clone (A1) | builds and plays |
| I1 | Move card ([ike.md](characters/ike.md)) | owner approved (2026-10-13) |
| I2 | Look on G0's pick (default): the kit's Ike rebuilt onto Roy's skeleton (K) or Roy restyled into Ike (R), both details; Ragnell rebuilt from the kit; costumes from the kit's eight | G1: turntables beside Roy and the kit model; palette sheet |
| I3 | Normals: jab 1-3, forward tilt, dash attack, forward smash, up smash | G2: clips with hitboxes |
| I4 | Specials: Quick Draw (new), Eruption (Roy's Flare Blade, blue), Aether (on the Final Cutter structure), Counter on taunt (Marth's) | G2 |
| I5 | Audio, presentation, Kirby, CPU | sample sheet |
| I6 | Acceptance probe, worst-case search, playtest ROM | checklist; owner playtest |

I3-I5 do not wait for G0: they run on Roy's model until I2 lands.

### Wave 2 — Palutena

| Step | Work | Gate |
|---|---|---|
| P0 | Null clone (A1) | builds and plays |
| P1 | Move card ([palutena.md](characters/palutena.md)) | owner approved (2026-10-13) |
| P2 | Look: G0's pick completed, both details, all costumes; wing joints (the kit's wings, rebuilt), halo and pillar joints (authored); entry gate (the kit's gate, rebuilt) | G1 |
| P3 | Base motion pass: Peach's reused clips with the staff-hold transform | G2a |
| P4 | Normals: jab and rapid jab, tilts, shield charge, smashes (wings, light pillar), aerials, grab and throws, floor and edge attacks | G2b |
| P5 | Specials: Explosive Flame, Warp, Counter/Reflect | G2c |
| P6 | Entry gate, down taunt, idle, victory poses | G2d |
| P7 | Audio (WAV ingest), presentation, Kirby, CPU | sample sheet |
| P8 | Acceptance probe, worst-case search, playtest ROM | checklist; owner playtest |

The full worst-case search runs at the end of each wave: every VS stage,
rosters that include the new fighter, Kirby copies, item-dense scenes.

## Per-fighter checklist (the definition of done)

P4's checklist lines 1-9 apply, read for an authored content: line 1's export
is the authored export (0 look-build, encoder, assembler or author failures;
every table that differs from the base has a seam; every command used is
ported), and line 2's routine list is the move card. Added:

10. **Art approved.** The G0 pick, then the model in both details and all
    costumes, every new or changed clip, every effect; each approval logged in
    P6_STATUS with the renders it saw (local paths and hashes).
11. **No base leakage.** None of the base's visuals, sounds or routines where
    the card says otherwise: no Roy fire on Ike's forward smash, no Double-Edge
    Dance or Blazer, no Roy voice or sound effect; no Peach parasol, turnip,
    crown, float or Peach voice.
12. **Clean art.** `check_p6_art.py` passes: the look manifest and the
    authored export fresh, nothing from the 3DS, Ultimate or Brawl folders
    tracked.
13. **P3.** A two-console harness match (melonDS-mp) with the fighter: 0
    desyncs.

## Art and animation rules

- **Style:** SSB64's low-poly look — flat colours, small textures, readable
  silhouettes at DS battle size. Every VS battle draws the low detail (owner,
  2026-10-01), so the low model gets the same care as the high one, which the
  select screen, Results and close-ups show; G0 judges the battle size first.
  The kit's 3DS shading (specular, reflection, metal) is dropped: colours are
  the albedo under SSB64's vertex lighting.
- **Budgets** start from the base's counts (A1 measures them per joint; the
  owner pins already give the totals). Ike within 10% of Roy: at most 386
  triangles high and 222 low (Roy 351 / 202). Palutena within 15% of Peach,
  staff and shield included: at most 426 / 278 (Peach 371 / 242); the wings,
  halo and pillar draw only in their moves and are budgeted with them. No more
  texture than the base. The kit's low models draw about seven times that
  (Ike 1,515 triangles, Palutena 1,930). Going over is an owner question, as
  are very noticeable visual approximations.
- **Skeletons:** Ike keeps Roy's JointTree with no overall scale change, so
  every Roy and Marth clip stays valid and his hurtboxes and map box stay
  Roy's. Palutena keeps Peach's joints and appends the wing, halo and pillar
  joints; the fighter limit is 37 joints (`FTPARTS_JOINT_NUM_MAX`) and Peach's
  owner selects 27. The kit's skeleton is only the fit's source: its extra
  joints (swing joints, Palutena's skirt) bind to the base's matching or
  nearest joint and add none.
- **Where geometry comes from:** G0's pick, per fighter and per region: Track
  K (the kit rebuilt by ring retopology, hand-tuned) or Track R (the base's
  mesh restyled). Automatic reduction alone is not used for anything that
  ships. The staff, shield, Ragnell, wings and gate are rebuilt from the kit
  in either case; the halo, light pillar, explosion and emblem are hand-made
  in `authored.blend`.
- **Where a clip comes from**, in order of preference: the base's clip; a clip
  on the same skeleton (Marth's for Ike); a retargeted clip as a start, then
  edited; keyed from a pose sheet. No Smash 4 motions are available (D11).
  Pose sheets and edit layers are committed JSON, so edits are reviewable
  diffs; the composed clips are generated.
- **Hitboxes are authored with the clip**: the review render shows them and
  the lab's hitbox census confirms them.
- **Reference use:** the kit and the Ultimate models open only in local
  scenes (`*.local.blend`, `builds/p6-art/`). No tracked file holds their
  data: built geometry, textures, icons and renders stay under `builds/` and
  `artifacts/`, and the look ops hold anchors and offsets only.

## Risks

| Risk | Mitigation |
|---|---|
| Palutena needs about 40 new clips; animation volume and quality set P6's length. No Smash 4 motions exist to start from (D11). | Retargeted starts and pose sheets; gameplay clips first; batched reviews with hitboxes visible; SSB64-level simplicity is the bar, not Ultimate's. |
| The kit cut to a seventh of its triangles loses the look (the owner's doubt, D12). | No automatic reduction: ring retopology and hand-tuning (Track K), and Track R as a full alternative on the same sheet; regions can mix; the sheet judges battle size first and includes lab stills. |
| Track R cannot reach the slim dress or the long hair from Peach's gown and hair within the budget. | Regions mix: Track K's dress or hair can join Track R's body; the budget split shows the cost per region before the owner picks. |
| Two tracks double the look work before a pick. | Body only, default costume; the fit, staff, shield, capture tool, harness and checklist are shared; Ike follows the pick by default; Ike's gameplay work continues while the sheet waits. |
| The fit: the kit's skeletons (33 and 37 joints) differ from the bases' in proportions and extra joints. | Pose-to-base fit before any track starts; extra joints bind to the base's matching or nearest joint; hurtboxes and map boxes stay the base's. |
| Local-only art (D10): a checkout without the kit cannot build P6 contents. | As with the Remix staging: the build stops with a message naming the missing folder or the stale manifest; the manifest pins every input hash, so anchors always meet the kit file they were captured on. |
| The model writer must satisfy the native owner, lean path and pack validators (cross-joint loads, material spans, root cells), with fresh geometry (K) and edited geometry (R). | F1 and F6 before any art; material spans of the kinds the base already uses. |
| Palutena's appended joints reach every per-joint consumer (packs, shield pose, lean, owner). | Measured in P2; if a validator blocks, the wings fall back to sidecar roots (the Kirby hat and held-gun path). |
| SSB64 throws pair the thrower with the victim's thrown motion through the parent's capture and thrown rows (Fox's for Palutena). | New throw clips keep the parent's release timing, or the content adds thrown rows as Remix contents do. |
| Memory: two more contents in the worst-case mixes, while P4 still has mixes under the floor (P4_STATUS "Memory"). | Budgets above; the P4 residency tools; the wave-end search reports any gap rather than gating the select screen (owner, 2026-10-08). |
| New multi-hit effects (explosion, pillar) cost frame time. | The pillar draws as part of her own lists; the explosion is one article; both measured on two heavy stages. |
| The repository is public (D1, D10). | No rip-derived file is tracked: `check_p6_art.py`, ignore rules for the three rip folders, `builds/` and `artifacts/`, anchors and offsets instead of coordinates in the look ops, the local scenes and owner voices. |
| Blender API drift. | Scripts assert Blender 5.2 LTS; ROM builds read `builds/p6-art/` and never run Blender. |

## Working rules for speed

P4's working rules apply (lab ROM first; menu and playable ROMs at each
checkpoint; the all-content walk ROM for memory; generated output never
hand-edited; no source edits while a build runs; owner asked only about very
noticeable visual approximations and contract changes). Added:

- Art goes to the owner in batches with renders; work continues on the next
  item while a batch waits (Ike's gameplay while the G0 sheet waits).
- A clip is not "done" until its hitboxes match its swing in the review
  render and in the lab census.
- Builds of P6 contents need the local inputs: the Remix staging, the 3DS kit
  (built once into `builds/p6-art/`, again when the ops, the recipe or the kit
  change) and the Ultimate folder (both fighters' audio). Build output stays
  under `builds/`.

## Where work starts

P6 is active under the P5-P7 goal (2026-10-13). A2-A6, A3b and A3c are
host-only tools that touch no runtime code. First steps:

1. A1: both null clones.
2. A2 and A3 with F1; the kit reader with its measured counts (F6); the fit
   of the kit Palutena onto Peach.
3. G0: Track K and Track R of Palutena through the same rounds, the sheet and
   lab stills to the owner, the pick logged.
4. Ike's I2 on the picked track (or his own sheet if the owner asks), while
   I3-I5 proceed on Roy's model.
