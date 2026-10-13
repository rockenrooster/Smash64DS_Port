# P6 — Ike and Palutena: implementation plan

**Revision:** 1 (2026-10-12).
**Inputs:** the owner's briefs [Ike.md](Ike.md) and [Palutena.md](Palutena.md)
(kept as written; this plan records where a decision overrides a line).
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
  parent Fox);
- **committed art**: Blender sources and their exported intermediate;
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
- **Fidelity source:** the owner briefs, then each fighter's move card once the
  owner approves it. Brawl (Ike) and Ultimate (Palutena) define the look and
  the intent of each move; their frame data does not bind. Numbers are set on
  SSB64's scale against the base fighter in the lab, and the owner judges the
  feel in playtests.
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

## Owner decisions

2026-10-12 (D1-D4 asked with this plan; D5-D9 after the owner added Ike's
Brawl and Ultimate audio, the announcer clips and the chants):

| # | Decision |
|---|---|
| D1 | Authored art is **committed to this (public) repository**: Blender sources and their exports. Ripped game data is never committed: the Ultimate Palutena model, textures and WAVs and Ike's Ultimate WAVs (`decomp/alt_assets/Fighters/Super Smash Bros Ultimate/`) and the Brawl Ike voices (`decomp/alt_assets/Fighters/Super Smash Bros brawl/`) stay local and ignored, and only build output under `builds/` derives from them. |
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
| Costumes: the base's count; colours from each brief, approved on a palette sheet. | |
| Explosive Flame distance: D-pad back, neutral or forward at the cast frame picks near, mid or far. The DS D-pad is digital (the stick reads only 0 or +-80, `controller_backend.c`), so Ultimate's tilt-amount distance cannot be read. | |

## Architecture: the authored export

```
art/p6/<name>/<name>.blend
   │  scripts/p6/blender/export_art.py  (Blender 5.2 LTS, headless)
   ▼
art/p6/<name>/export/        committed: model.json, textures/*.png,
   │                         anims/*.json, manifest.json (.blend SHA-256)
   │
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

- **Blender is an authoring tool, not a build dependency.** Builds read the
  committed export. Its manifest records the source .blend's SHA-256, and the
  art check fails when the export is stale.
- **Base data stays the base's.** Roy's and Marth's clips, Peach's clips and
  the base's special files keep their file ids. Only new or changed files get
  new ids, so Ike in a match with Roy or Marth loads their shared clips once.
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
  and import only `bpy` and the pure-Python codecs.
- **Art layout.** `art/p6/<name>/` holds the .blend, its export and the
  approved review stills; `art/p6/common/` holds shared pieces (the Kid
  Icarus emblem). Ignored: `art/**/*.local.blend` (the reference scenes that
  link the Ultimate files) and `art/**/voice.local/`.

## Board 1 — authoring machinery

Each item is built once and proven by Ike first.

| # | Item | Done when |
|---|---|---|
| A1 | **Registry, build and null clones.** `contents.json` rows with `base` instead of `remix` (19 ike: roy, 20 palutena: peach); `p4_contents.py`, `p4_rules.mk` and `p4_css.py` accept them; `NDS_P4_IKE`, `NDS_P4_PALUTENA`; the lab content knob takes 19 and 20; the file-id window is checked; `p6_author.py` runs with an empty spec. Per-content tables sized by a literal instead of `NDS_P4_CONTENT_LIMIT` are found and fixed. | Ike as Roy and Palutena as Peach, under ids 19 and 20 and without the base's code overrides, build into the lab, menu-walk and playable ROMs and play four-CPU mirrors with 0 native failures; select cell, Results, HUD, Kirby copy and the P3 descriptor work. The base budgets are measured (triangles, vertices, textures, clips, setup heap, low-water, P95). |
| A2 | **Model reader and writer** (`scripts/p6/n64_model.py`): both JointTrees, display lists (including the cross-joint vertex loads and bare-ENDDL joints Remix models use), vertices, MObjs and their costume palettes, model-part variants, sprites. The writer keeps the donor's material spans (combine, other mode, texture loads) and writes only geometry and textures anew. The parsing already spread over the owner generator and the hat scripts moves into this module. | F1: Roy and Peach read and written back with no edits give byte-identical files (at least identical display lists), identical native owner images, and unchanged `p4_native_owner.py --learn` pins. |
| A3 | **Blender bridge** (Blender 5.2.2 LTS, headless): import a content's model and clips into a .blend (armature from the JointTree, rigid per-joint meshes, cross-joint stitch groups, MObj materials with costume palettes, clips as actions) and export back to the committed intermediate. Interactive sessions use the MCP for Blender add-on (v1.8, protocol 13, in Blender 5.2), registered with Claude Code on 2026-10-12 as the project-local server `blender` (`mcp-for-blender@2.1.9`, pinned to the add-on's protocol; telemetry off); Blender must be open, and its add-on listens on port 9876. The headless scripts stay the canonical path. | Roy imported and exported with no edits reproduces F1. |
| A4 | **Animation codec and retarget.** Figatree reader and writer: sample at the game's 60 frames a second, write dense linear keys, then `p4_anim_compact.py`. Track transforms: retime, mirror, splice, per-joint override (Palutena's staff hold). Retarget between humanoid skeletons by joint-role maps (Mario, Luigi, Link, Captain, Ness, Fox, Marth/Roy, Peach, Sheik, Ganondorf). | F2: every Roy, Marth and Peach clip decodes, re-encodes and decodes equal within the compactor's tolerance (`ANIM_EPS`); a retargeted Captain jab plays on Ike's skeleton. |
| A5 | **Motion-script assembler** (`scripts/p6/mscript.py`): a text form and bytes, both ways. One op table with `remix_export.py`'s decoder, so the names are the ones `events.json` already prints (MakeAttackColl, ClearAttackCollAll, SyncWait, Effect, PlayFGM, SetFlag0, SetModelPartID, RemixFrameSpeedMultiplier and the rest; 49 ops appear in Roy, Marth and Peach). Field layouts come from BattleShip's motion-event structures. Script edits for reused moves: scale damage or knockback, insert a frame-speed command, drop Effect commands of a kind, resize hitboxes, swap sounds. | F3: every script of the eighteen exports disassembles and re-assembles byte-identical. |
| A6 | **Review harness** (`scripts/p6/blender/review.py`): turntables per costume and detail beside the base; per clip a contact sheet and a 30 fps GIF with the script's hitboxes drawn as spheres on their joints (hurtboxes optional) and sound cues marked. Output goes to `artifacts/p6/review/<date>/` (local) and to the owner. | Roy's forward smash renders with its hitboxes where the lab's hitbox census puts them. |
| A7 | **Authored export** (`scripts/p6/p6_author.py`): base export + spec + art export in, export directory out. File substitutions and new ids; appended motions and statuses whose callbacks name DS ports or decomp functions; kind-table rows (special starters, Kirby rows, jab and rapid-jab rows, entry, Results rows, winner music and call); attribute deltas; sword-trail rows; authored colour animations. The generator learns one authored-callback marker and nothing else. | F4: an empty spec reproduces the base export but for identity; Ike's spec builds. |
| A8 | **Audio.** Authored cues from local WAVs (trim, normalize, resample, IMA) through `p4_audio.py`'s pack writer, FGM ids past Remix's: Ultimate's voices and effects for both fighters (D5, D6) the narrator's name calls (D7, level-matched to the source announcer) and the crowd cheers (D8, mono). Each spec maps every voice slot and every sound command of its scripts, reused or new, to a clip; SSB64's own hit sounds stay. Victory jingle: `render-audio-bgm.py` with an excerpt and fade option. A missing local folder fails that content's build with a message naming it. | The owner approves each fighter's slot map by ear (the sample sheet); four-CPU mirrors play 0 FGM misses. |
| A9 | **Presentation.** Select portrait rendered from the high model with a portrait preset and quantized to Remix's 32x32 portrait format; name textures composed from the existing name font's glyphs at build time; series emblems (Fire Emblem's exists; Kid Icarus authored); Results rows and menu zoom; menu clips; the stock icon from the existing bake. | Both cells, previews, HUD and Results draw on the menu and playable ROMs. |
| A10 | **Effects.** A content tint for effect makers and colour animations (Ike's blue fire, also on a Kirby holding Ike's power). Authored articles through `p4_articles.py` (Palutena's explosion and entry gate). Effects that move with Palutena are joints of her own tree, animated by her clips and shown by SetModelPartID: wings, halo, light pillar. | Lab: each effect drawn natively, 0 declines, 0 native failures. |
| A11 | **Kirby.** Ike's power is Roy's power rows with Ike's tint and hat. Palutena's is a custom power (`p4_kirby.py` CUSTOM_POWERS) on Kirby's own PK Fire clips. Hats come from each model's head spans (`p4_kirby_custom_hats.py`). | Kirby copies both and draws their hats, 0 weapon or effect refusals. |
| A12 | **CPU.** Attack-coverage rows computed from clips and scripts (each hitbox's world box per frame); recovery and post-process routines per fighter. | F5: the tool reproduces the decomp's coverage rows for two original fighters within tolerance; four-CPU lab matches use every authored attack. |
| A13 | **Checks.** `scripts/p6/check_p6_art.py`: every export fresh; no Brawl or Ultimate data in a tracked file (packed images, external paths, hashes against the local folders when present); no WAV tracked. The ignore rules for both folders and for the local art files are in `.gitignore` (2026-10-12). F1-F5 run as one host test. | Mutation-tested: each check goes red on a planted fault. |

**Order.** A1 first: the plumbing is proven before any art exists. Then A2,
A3, A5 and A6 (host tools, F1 and F3). A4 and A7 land with Ike's first clips.
A8-A12 land as Ike needs them, the WAV ingest included (D5, D6); the
Palutena-only parts (joint effects, articles, the custom Kirby power) land in
Wave 2.

## Board 2 — fighters

Ike goes first. His base shares the Marth/Roy skeleton, so most of his clips
already exist and his new routines are few: he proves the pipeline on a small
delta. Palutena follows with the large art and animation load and the new
audio ingest.

### Wave 1 — Ike

| Step | Work | Gate |
|---|---|---|
| I0 | Null clone (A1) | builds and plays |
| I1 | Move card ([ike.md](characters/ike.md)) | owner approves the card |
| I2 | Look: Roy restyled into Ike, both details; Ragnell in gold; costumes | G1: turntables and palette sheet |
| I3 | Normals: jab 1-3, forward tilt, dash attack, forward smash, up smash | G2: clips with hitboxes |
| I4 | Specials: Quick Draw (new), Eruption (Roy's Flare Blade, blue), Aether (on the Final Cutter structure), Counter on taunt (Marth's) | G2 |
| I5 | Audio, presentation, Kirby, CPU | sample sheet |
| I6 | Acceptance probe, worst-case search, playtest ROM | checklist; owner playtest |

### Wave 2 — Palutena

| Step | Work | Gate |
|---|---|---|
| P0 | Null clone (A1) | builds and plays |
| P1 | Move card ([palutena.md](characters/palutena.md)) | owner approves the card |
| P2 | Look: Peach restyled into Palutena, both details; staff in her hand (where Peach's held items sit), shield on her forearm; wing, halo and pillar joints | G1 |
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
is the authored export (0 encoder, assembler or author failures; every table
that differs from the base has a seam; every command used is ported), and
line 2's routine list is the move card. Added:

10. **Art approved.** Model in both details and all costumes, every new or
    changed clip, every effect; each approval logged in P6_STATUS with the
    renders it saw.
11. **No base leakage.** None of the base's visuals, sounds or routines where
    the card says otherwise: no Roy fire on Ike's forward smash, no Double-Edge
    Dance or Blazer, no Roy voice or sound effect; no Peach parasol, turnip,
    crown, float or Peach voice.
12. **Clean art.** `check_p6_art.py` passes: exports fresh, nothing from the
    Ultimate folder tracked.
13. **P3.** A two-console harness match (melonDS-mp) with the fighter: 0
    desyncs.

## Art and animation rules

- **Style:** SSB64's low-poly look — flat colours, small textures, readable
  silhouettes at DS battle size. Every VS battle draws the low detail (owner,
  2026-10-01), so the low model gets the same care as the high one, which the
  select screen, Results and close-ups show.
- **Budgets** start from the base's counts measured in A1: Ike within 10% of
  Roy's triangles and vertices and no more texture than Roy; Palutena within
  15% of Peach's (staff, wings, halo, pillar) and no more texture than Peach.
  Going over is an owner question, as are very noticeable visual
  approximations.
- **Skeletons:** Ike keeps Roy's JointTree with no overall scale change, so
  every Roy and Marth clip stays valid and his hurtboxes and map box stay
  Roy's. Palutena keeps Peach's joints and appends the wing, halo and pillar
  joints; the fighter limit is 37 joints (`FTPARTS_JOINT_NUM_MAX`) and Peach's
  owner selects 27.
- **Where a clip comes from**, in order of preference: the base's clip; a clip
  on the same skeleton (Marth's for Ike); a retargeted clip as a start, then
  edited; keyed from a pose sheet. Pose sheets are scripts and every clip's
  keys are JSON in the committed export, so edits are reviewable diffs.
- **Hitboxes are authored with the clip**: the review render shows them and
  the lab's hitbox census confirms them.
- **Reference use:** the Ultimate FBX and its textures are opened only in a
  local reference scene (`*.local.blend`) beside the model being built.
  Committed textures are hand-made or derived from the base's own textures.

## Risks

| Risk | Mitigation |
|---|---|
| Palutena needs about 40 new clips; animation volume and quality set P6's length. | Retargeted starts and pose sheets; gameplay clips first; batched reviews with hitboxes visible; SSB64-level simplicity is the bar, not Ultimate's. |
| The model writer must satisfy the native owner, lean path and pack validators (cross-joint loads, material spans, root cells). | F1 before any art; the writer copies the donor's material spans. |
| Palutena's appended joints reach every per-joint consumer (packs, shield pose, lean, owner). | Measured in P2; if a validator blocks, the wings fall back to sidecar roots (the Kirby hat and held-gun path). |
| SSB64 throws pair the thrower with the victim's thrown motion through the parent's capture and thrown rows (Fox's for Palutena). | New throw clips keep the parent's release timing, or the content adds thrown rows as Remix contents do. |
| Memory: two more contents in the worst-case mixes, while P4 still has mixes under the floor (P4_STATUS "Memory"). | Budgets above; the P4 residency tools; the wave-end search reports any gap rather than gating the select screen (owner, 2026-10-08). |
| New multi-hit effects (explosion, pillar) cost frame time. | The pillar draws as part of her own lists; the explosion is one article; both measured on two heavy stages. |
| Committed art is public (D1). | Brawl and Ultimate files never enter a tracked file: `check_p6_art.py`, ignore rules for both folders, the local reference scenes and owner voices. |
| Blender API drift. | Scripts assert Blender 5.2 LTS; builds never run Blender. |

## Working rules for speed

P4's working rules apply (lab ROM first; menu and playable ROMs at each
checkpoint; the all-content walk ROM for memory; generated output never
hand-edited; no source edits while a build runs; owner asked only about very
noticeable visual approximations and contract changes). Added:

- Art goes to the owner in batches with renders; work continues on the next
  item while a batch waits.
- A clip is not "done" until its hitboxes match its swing in the review
  render and in the lab census.
- Builds of P6 contents need the local inputs: the Remix staging and the
  Ultimate folder (both fighters' audio, Palutena's reference model). Build
  output stays under `builds/`.

## Where work starts

P6 starts when P4's goal closes; it depends on P4's seams and its memory
state. A2-A6 are host-only tools that touch no runtime code, so they can start
earlier if the owner wants. First steps:

1. A1: both null clones.
2. A2, A3, A5, A6 with F1 and F3: Roy round-trips through Blender unchanged.
3. Ike's card to the owner (I1), then I2: the first visible Ike is Roy
   re-coloured and re-shaped.
