# P6 status — Ike and Palutena

Owner of the live P6 state. The plan is [P6_PLAN.md](P6_PLAN.md); the cards
are [characters/ike.md](characters/ike.md) and
[characters/palutena.md](characters/palutena.md).

**State (2026-10-13, late):** active (goal P5-P7). Move cards approved (I1,
P1). A1 done: Ike (19) and Palutena (20) are registry contents built from
Roy's and Peach's exports (null clones) and play in every ROM. G0 settled by
the owner: Palutena's look is Peach's model reshaped onto the 3DS kit's
proportions (owner route, after a first pick of the decimated kit), built in
the live Blender against the kit's wire overlay; the low detail is in owner
review (v60, 250 of 262 triangles, [look record](#palutena-look-record)).
Nothing goes into a ROM before the owner's OK. The high detail is the
approved low refined (owner). A2's writer and A7's file substitution are
done and checked on the host (the native owner generator and the P4
generator accept the written look). Next: the owner's verdict on the low,
then the high, then the look's build rules and pins.

## Board 1

| Item | State |
|---|---|
| A1 registry, build, null clones | done 2026-10-13 ([A1 record](#a1-record)) |
| A2 model reader and writer (F1) | done 2026-10-13 (`model_writer.py`: Peach's own low scene written back draws the same 211 triangles; the written look passes the native owner generator, host) |
| A3 Blender bridge, kit reader, fit | done 2026-10-13 (`dae_reader.py`, `kit_fit.py`, `scripts/p6/blender/`) |
| A3b Track K: the kit rebuilt (F6) | dropped by the owner's route (ops kept for reference) |
| A3c Track R: the base restyled (F6) | Palutena low in owner review (v51) |
| A4 animation codec and retarget (F2) | planned |
| A5 motion-script assembler (F3) | planned |
| A6 review harness | planned |
| A7 authored export (F4) | file substitution done 2026-10-13 (spec "files", `remix_rom.OverlayRom`; host-checked through `generate_p4_fighter.py`); the rest planned |
| A8 audio | planned |
| A9 presentation | planned |
| A10 effects | planned |
| A11 Kirby | planned |
| A12 CPU (F5) | planned |
| A13 checks | planned |

## Board 2

| Fighter | Step | State |
|---|---|---|
| Palutena | G0 look (owner route: Peach reshaped onto the kit) | low in review; high next |
| Ike | I0 null clone done (A1); I1 approved; I2-I6 | planned |
| Palutena | P0 null clone done (A1); P1 approved; P2-P8 | planned |

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

**Route (owner, 2026-10-13 evening).** The owner first picked the decimated
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

**Low detail, v60 (250 triangles, budget 262; Peach low 211).** The torso is
rebuilt from rings sampled on the kit's surface (neckline, bust with a
shallow valley between its two forms, waist; a collar of clavicles,
shoulder tops and the neck's base, closed on top), every band triangulated
as a half and its mirror image; the dress continues the bodice's waist ring
and runs as one A-line to the hip ring (the owner preferred v40's hips to a
kit-sampled hip ring), its hem lower at the front than the back as the
kit's; the hair ends above the dress so it never passes through it; the
neck is on the kit's. Earlier (v51) and kept: Peach's head
kept and scaled, face snapped to the kit face at 1.12 (the owner allows a
bigger face), Peach's own face frames recoloured (green iris, Palutena's
skin) with the gold circlet painted across the forehead rows (no crown
geometry; ears and crown dropped); hair cap and the long hair curtain on the
kit's hair; strapless bodice in one surface with the skirt, a level
neckline and a two-form bust on the kit chest; arms joined to the torso,
gold bracers; skirt hem round (eight points) with a red, gold and cream hem
texture, two-sided, slit on the left side up near the hip over a white
stocking leg; staff = a three-sided shaft, gold wing sprite and a billboard
gem; shield = a flat round sprite on the left forearm.

**Owner directions on the way (2026-10-13).** Waist and dress one mesh;
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

## Owner decisions

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
