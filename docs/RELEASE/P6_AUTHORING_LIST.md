# Palutena and Ike: what to author for a clean build

**Status:** inventory 2026-10-29 (read from the make rules, the specs, the
recipes, the P6 tools and `docs/P6/characters/*.md`). Companion to
[RELEASE_PLAN.md](RELEASE_PLAN.md) R1.

**Since 2026-10-29 the build does not read the rips.** The Ultimate-derived
clips and sounds are stored once (frozen) under
`assets/p6-art/<name>/anims/` and `.../audio/`, and every other rip-derived
input already was a stored file under `assets/p6-art/`. The play ROM rebuilt
with every Python read of the rip folders blocked is byte-identical
(`docs/P6/P6_STATUS.md`). The stored files are still rip-derived and stay
local, so the `safe_for_public_release` column is unchanged: authoring
replaces them one file at a time.

**Workspace:** `assets/p6-reauthor/` (local, ignored) holds a reference copy
of everything to re-author: Palutena's model, textures, costume sheet, stock
icon, gate, wings, effect masks, and both fighters' current clips and sounds
with the Ultimate sources they came from. Its `INDEX.csv` gives each
replacement's target (clip lengths, sound kinds), its `README.md` the
conventions; replacements go in each category's `new/` folder.

Spreadsheet: [P6_AUTHORING_LIST.csv](P6_AUTHORING_LIST.csv), one row per
build input (343: look parts, gate-file parts, every clip, every sound,
presentation, committed sources). `safe_for_public_release` is true when the
release can use the input as it is now: committed and authored, or generated
on the user's PC from their ROM and Remix. It is false when the input comes
from a 3DS, Ultimate or Brawl rip, directly or through a generated file.
`lives_in` says where the file is: in the repo, generated on the user's PC,
or only on your PC (your art; stored and rip-derived, which the build reads;
or the rip itself, read only when re-freezing).

"The build reads" means a file that make, or a hand-run step whose output make
reads, opens for content 19 (Ike) or 20 (Palutena), and that a clean folder
does not have. "Author" is what you make yourself. Everything else in the
chain regenerates from it, from the user's ROM and Remix, and from the
committed recipes.

## Summary

| | Palutena | Ike |
|---|---|---|
| Model + textures + face expressions | **author** (now the 3DS kit model, reduced and hand-tuned; textures baked from the kit) | nothing: Roy's model recoloured by committed ops |
| Costumes | **8 colourings** (now sampled from the kit's alternates); colour rules like Ike's work | nothing (palette rules) |
| Stock icon | **author** an 8x10 icon (now the kit's) | nothing (Roy's icon recoloured) |
| Entry gate | **author** (now the kit's gate) | nothing (Roy's entry clip) |
| Smash wings: model, texture, 2 motions | **author** (kit godwing + Ultimate godwing motions) | - |
| Light effect masks | **3 textures** (now Ultimate effect PNGs) | - |
| Clips | **134** (Appendix A) | **46** (Appendix B) |
| Sounds | **71 WAVs** (Appendix C) | **48 WAVs** (Appendix D) |
| Select portrait | your pixel art: commit it if you made it | same |
| Congratulations still | `assets/congra/c20.png` (yours): commit if original | `assets/congra/c19.png` (same) |
| 1P intro cards (16 each) | none: baked from the model (RELEASE_PLAN R3) | same |

## Palutena: what the build reads, and where it comes from now

| The build reads | Made by | From now | You author |
|---|---|---|---|
| `assets/p6-art/palutena/look/1b00` (main), `1b03` (model): spec `files` | `model_writer.py` (by hand, outside make) from `3ds2ds/scene_high` + `scene_low`, `costumes/stock.json` and the costume palettes | Your `3DS_to_DS_Palutena_v2.blend` (mesh decimated from the 3DS kit, textures baked from the kit) via `dump_look.py`, `look_reduce.py` and `look_blend.py --kit palutena_low_n_c00.dae`; `p6_costumes.py` (kit alternates c01-c07); `p6_stock.py` (the kit's stock icon row) | The model, high (~451 tris) and low (~332), rigged to Peach's rows (dress, hair panels row 9, staff row 14, forearm shield sprite), its textures, face expression frames and hands; 8 costume colourings; the stock icon (8x10, one LUT per costume) |
| `assets/p6-art/palutena/3ds2ds/scene_high/scene.json`: every clip recipe and the skirt solver read it | `look_blend.py` | The same model chain | Nothing extra: generated from your model |
| `assets/p6-art/palutena/gate/1b10` (special file 2: gate, light pillar, smash wings, gem glow, light wings): spec `files` | `p6_gate.py` with `p6_wings.py` and `p6_light.py` (by hand) | Kit `Palutena/gate/gate.dae` + `gate.png`; kit `Palutena/godwing/godwing.dae` + `Yoshi_Wing.png`; Ultimate godwing motions `c03attacks4s` (forward smash) and `c04attacklw4` (down smash) via `art/p6/palutena/wings/*.json`; Ultimate `ef_palutena_flash01.png`, `ef_palutena_backlight00.png`, `ef_palutena_backlight01.png` | Gate: a double door, one 64x64 texture. Wings: model or cards + texture, and their two smash motions. Masks: gem glow, feather wing, taunt ring. The pillar atlas is already authored (`look_parts.py`). |
| `builds/p6-art/palutena/anims/*.o2r` (138 clips) | make (`anim.py`) from `art/p6/palutena/anims/*.json` | 134 recipes: the stored clip `assets/p6-art/palutena/anims/<recipe>.o2r` (+ `.skirt.json`), converted once from Ultimate `.nuanmb` on the Ultimate DAE skeleton. 4 recipes: Peach's clips (fine) | 134 clips (Appendix A) |
| 71 cues of `scripts/p6/palutena/audio.json` | make (`p4_audio.py` / `p6_audio.py`) | The stored renders `assets/p6-art/palutena/audio/<cue>.wav`, rendered once from the Ultimate WAVs | 71 sounds (Appendix C), or cues pointed at Peach/Remix sounds through the recipe's `map` |
| `assets/p6-art/palutena/portrait.png` | `portrait_art.py` from `owner_art/portrait_source.png` | Your pixel portrait | Nothing, if you drew it: commit the source |
| `assets/congra/c20.png` | Yours (`p6_congra_still.py` is not used) | Your art | Nothing, if original: commit it |
| `assets/intro/p*_c20_*.s1i` (16) | The intro bake (melonDS) | The DS renderer drawing her model | Nothing: rebaked from your model |

## Ike: what the build reads, and where it comes from now

| The build reads | Made by | From now | You author |
|---|---|---|---|
| `assets/p6-art/ike/look/1a00`, `1a03`: spec `files` | `look_r.py compose` + `model_writer.py` (by hand) | Roy's files in the Remix staging + `art/p6/ike/look/*.json` (committed); stock icon = Roy's recoloured (`p6_stock.py`) | Nothing: the release script has to run this chain |
| `builds/p6-art/ike/anims/*.o2r` (47 clips) | make (`anim.py`) | 46 recipes: the stored clip `assets/p6-art/ike/anims/<recipe>.o2r`, converted once from Ultimate `.nuanmb` + `ike_motion/.../model.nusktb` (skeleton) + Ragnell's sword-model motion in `aether_rise` (`d02specialhi2`). `entry`: Roy's clip eased into `wait` | 46 clips (Appendix B); `entry` follows the new `wait` |
| 48 cues of `scripts/p6/ike/audio.json` | make | The stored renders `assets/p6-art/ike/audio/<cue>.wav`, rendered once from the Ultimate WAVs | 48 sounds (Appendix D), or Roy/Remix sounds through the recipe's `map` |
| `assets/p6-art/ike/portrait.png` | `portrait_art.py` from `owner_art/portrait_source.png` | Your pixel portrait | Nothing, if you drew it: commit the source |
| `assets/congra/c19.png` | Yours | Your art | Nothing, if original: commit it |
| `assets/intro/p*_c19_*.s1i` (16) | The intro bake | His model | Nothing: rebaked |

## How your files can enter the build (tooling, my side)

These recipe kinds already build without a rip:

- **Clips** (`anim.py`): `keyposes` (key poses over base-clip frames, with
  turns, aims and leg IK; `keyposes.py`), `keys`, and `source` plus
  `ops`/`frames`/`retime`/`retarget` (an SSB64 or Remix clip adapted). All of
  them are text recipes, rebuilt from the user's ROM. There is no Blender
  action exporter yet: animating in Blender needs a new tool that writes a
  clip or a `keys` recipe.
- **Costumes:** palette rules (Ike's route, the `look_r.py` palette op).

Replacing a stored clip or sound: rewrite the clip's recipe in one of the kinds
above (its `ultimate` and `frozen` blocks go); for a sound, drop the cue's
`frozen` block and name your WAV (a cue renders from `source` + `file`; the
`source` change is below). `scripts/p6/p6_freeze.py` re-freezes a clip or cue
from the rips if one has to be re-tuned before then (it needs the rips and
`ssbh_data_py`, installed in Python 3.13 only).

These need small changes before an authored file can replace the rip:

- `look_blend.py` takes its rig from the 3DS kit DAE (`--kit`). It needs a mode
  that reads the rig from your Blender file (weights named by Peach's rows).
- `p6_stock.py` needs a "from PNG" mode for an authored icon.
- `p6_gate.py`, `p6_wings.py`, `p6_light.py` and `face_frames.py` read fixed
  kit/Ultimate paths. Point them at a committed folder of your art.
- `p6_audio.py` reads `source` + `file`. Point `source` at a committed folder.
- `scripts/p6/check_p6_art.py` refuses tracked audio anywhere outside `decomp/` and tracked images
  under `art/p6/`. Authored files need allow rules (plan D1 already says
  authored art is committed).
- The hand-run chains (both looks, gate, stock icons, portraits) become
  release-script steps.

Suggested home for authored sources: `art/p6/<name>/` (tracked, public),
including the authored `.blend` (D1). `assets/` stays ignored for generated
files.

## Already committed or generated (not on your list)

- Committed: `scripts/p6/<name>/spec.json` (statuses, kind tables, CPU rows,
  sword trails), `motions/*.ms` (Palutena 33, Ike 26), `art/p6/*/look/*.json`
  ops, `src/port/nds_p4_palutena.c` / `nds_p4_ike.c`. The wing recipes stay,
  with a new motion source.
- Generated: Kirby hats (from each model's head), select name textures, the
  Kid Icarus emblem (drawn in code), base scenes `assets/p6-art/base/peach|roy`
  (from the staged ROM), the export and native owners.

## Appendix: every clip and sound

### A. Palutena: clips (138 recipes, 134 from Ultimate)

Recipe = `art/p6/palutena/anims/<recipe>.json`; built clip = `builds/p6-art/palutena/anims/<recipe>.o2r`.

| Recipe | Built from now | Ultimate motion(s) / source clip |
|---|---|---|
| `appeal` | Ultimate | `j01appeallwr` |
| `bair` | Ultimate | `c05attackairb` |
| `catch` | Ultimate | `e00catch` |
| `catch_pull` | Ultimate | `e00catchpull` |
| `cliff_attack_quick1` | Ultimate | `g02cliffattack` |
| `cliff_attack_quick2` | Ultimate | `g02cliffattack` |
| `cliff_attack_slow1` | Ultimate | `g02cliffattack` |
| `cliff_attack_slow2` | Ultimate | `g02cliffattack` |
| `cliff_catch` | Ultimate | `g01cliffcatch` |
| `cliff_climb_quick1` | Ultimate | `g02cliffclimb` |
| `cliff_climb_quick2` | Ultimate | `g02cliffclimb` |
| `cliff_climb_slow1` | Ultimate | `g02cliffclimb` |
| `cliff_climb_slow2` | Ultimate | `g02cliffclimb` |
| `cliff_escape_quick1` | Ultimate | `g02cliffescape` |
| `cliff_escape_quick2` | Ultimate | `g02cliffescape` |
| `cliff_escape_slow1` | Ultimate | `g02cliffescape` |
| `cliff_escape_slow2` | Ultimate | `g02cliffescape` |
| `cliff_quick` | Ultimate | `g02cliffclimb` |
| `cliff_slow` | Ultimate | `g02cliffclimb` |
| `cliff_wait` | Ultimate | `g01cliffwait` |
| `counter` | Ultimate | `d03speciallwr` |
| `counter_air` | Ultimate | `d03specialairlwr` |
| `counter_hit` | Ultimate | `d03speciallwattackr` |
| `counter_hit_air` | Ultimate | `d03specialairlwattackr` |
| `dair` | Ultimate | `c05attackairlw` |
| `damage_air1` | Ultimate | `f01damageair1` |
| `damage_air2` | Ultimate | `f01damageair2` |
| `damage_air3` | Ultimate | `f01damageair3` |
| `damage_elec` | Ultimate | `f02damageelec` |
| `damage_fall` | Ultimate | `a04damagefall` |
| `damage_fly_hi` | Ultimate | `f01damageflyhi` |
| `damage_fly_lw` | Ultimate | `f01damageflylw` |
| `damage_fly_n` | Ultimate | `f01damageflyn` |
| `damage_fly_roll` | Ultimate | `f01damageflyroll` |
| `damage_fly_top` | Ultimate | `f01damageflytop` |
| `damage_hi1` | Ultimate | `f00damagehi1` |
| `damage_hi2` | Ultimate | `f00damagehi2` |
| `damage_hi3` | Ultimate | `f00damagehi3` |
| `damage_lw1` | Ultimate | `f00damagelw1` |
| `damage_lw2` | Ultimate | `f00damagelw2` |
| `damage_lw3` | Ultimate | `f00damagelw3` |
| `damage_n1` | Ultimate | `f00damagen1` |
| `damage_n2` | Ultimate | `f00damagen2` |
| `damage_n3` | Ultimate | `f00damagen3` |
| `dash` | Ultimate | `c00attackdash` |
| `dash_run` | Ultimate | `a02dash` |
| `down_attack_d` | Ultimate | `f04downattackd` |
| `down_attack_u` | Ultimate | `f03downattacku` |
| `down_back_d` | Ultimate | `f04downbackd` |
| `down_back_u` | Ultimate | `f03downbacku` |
| `down_bounce_d` | Ultimate | `f04downboundd` |
| `down_bounce_u` | Ultimate | `f03downboundu` |
| `down_forward_d` | Ultimate | `f04downforwardd` |
| `down_forward_u` | Ultimate | `f03downforwardu` |
| `down_stand_d` | Ultimate | `f04downstandd` |
| `down_stand_u` | Ultimate | `f03downstandu` |
| `dsmash` | Ultimate | `c04attacklw4` |
| `dtilt` | Ultimate | `c02attacklw3` |
| `entry` | Ultimate | `j00entryr` |
| `entry_l` | Ultimate | `j00entryl` |
| `escape_b` | Ultimate | `b02escapeb` |
| `escape_f` | Ultimate | `b02escapef` |
| `fair` | Ultimate | `c05attackairf` |
| `fall` | Ultimate | `a04fall` |
| `fall_aerial` | Ultimate | `a04fallaerial` |
| `fall_special` | Ultimate | `a04fallspecial` |
| `flame` | Ultimate | `d01specials` |
| `flame_air` | Ultimate | `d01specialairs` |
| `fsmash` | Ultimate | `c03attacks4s` |
| `ftilt` | Ultimate | `c01attacks3s` |
| `fura_fura` | Ultimate | `f06furafura` |
| `hammer_wait` | Ultimate | `h05itemhammerwait` |
| `hammer_walk` | Ultimate | `h05itemhammermove` |
| `heavy_get` | Ultimate | `h03heavyget` |
| `heavy_throw` | Ultimate | `h03heavythrowf` |
| `intro_l` | base clip | `0x1415` (peach) |
| `item_shoot` | Ultimate | `h11itemshoot` |
| `item_shoot_air` | Ultimate | `h11itemshootair` |
| `item_swing1` | Ultimate | `h04swing1` |
| `item_swing3` | Ultimate | `h04swing3` |
| `item_swing4` | Ultimate | `h04swing4` |
| `item_swing_dash` | Ultimate | `h04swingdash` |
| `jab1` | Ultimate | `c00attack11` |
| `jab2` | Ultimate | `c00attack100start`, `c00attack100` |
| `jump_aerial_b` | Ultimate | `a03jumpaerialb` |
| `jump_aerial_f` | Ultimate | `a03jumpaerialf` |
| `jump_b` | Ultimate | `a03jumpb` |
| `jump_f` | Ultimate | `a03jumpf` |
| `kneebend` | Ultimate | `a03jumpsquat` |
| `landing_fall_special` | Ultimate | `a04landingfallspecial` |
| `landing_heavy` | Ultimate | `a05landingheavy` |
| `landing_light` | Ultimate | `a05landinglight` |
| `light_get` | Ultimate | `h00lightget` |
| `light_throw_air_f` | Ultimate | `h02lightthrowairf` |
| `light_throw_air_hi` | Ultimate | `h02lightthrowairhi` |
| `light_throw_air_lw` | Ultimate | `h02lightthrowairlw` |
| `light_throw_dash` | Ultimate | `h01lightthrowdash` |
| `light_throw_drop` | Ultimate | `h01lightthrowdrop` |
| `light_throw_f` | Ultimate | `h01lightthrowf` |
| `light_throw_hi` | Ultimate | `h01lightthrowhi` |
| `light_throw_lw` | Ultimate | `h01lightthrowlw` |
| `nair` | Ultimate | `c05attackairn` |
| `ottotto` | Ultimate | `g00ottotto` |
| `ottotto_wait` | Ultimate | `g00ottottowait` |
| `pass` | Ultimate | `g00pass` |
| `passive` | Ultimate | `f05passive` |
| `passive_stand_b` | Ultimate | `f05passivestandb` |
| `passive_stand_f` | Ultimate | `f05passivestandf` |
| `rapid_end` | Ultimate | `c00attackend` |
| `rapid_loop` | Ultimate | `c00attack100` |
| `rapid_start` | Ultimate | `c00attack100start`, `c00attack100` |
| `rebound` | Ultimate | `b02rebound` |
| `run` | Ultimate | `a02run` |
| `run_brake` | Ultimate | `a02runbraker` |
| `shield_break_fall` | Ultimate | `a04damagefall` |
| `squat` | Ultimate | `a05squat` |
| `squat_rv` | Ultimate | `a05squatrv` |
| `squat_wait` | Ultimate | `a05squatwait` |
| `stop_ceil` | Ultimate | `g00stopceil` |
| `throw_b` | Ultimate | `e01throwb` |
| `throw_f` | Ultimate | `e01throwf` |
| `turn` | Ultimate | `a01turn` |
| `turn_run` | Ultimate | `a02turnrun` |
| `uair` | Ultimate | `c05attackairhi` |
| `usmash` | Ultimate | `c04attackhi4` |
| `utilt` | Ultimate | `c02attackhi3` |
| `wait` | Ultimate | `a00wait1` |
| `walk_end` | Ultimate | `a01walkbraker` |
| `walk_fast` | Ultimate | `a01walkfast` |
| `walk_middle` | Ultimate | `a01walkmiddle` |
| `walk_slow` | Ultimate | `a01walkslow` |
| `wall_damage` | Ultimate | `g00walldamage` |
| `warp_end` | base clip | `0x1384` (peach) |
| `warp_start` | base clip | `0x13fe` (peach) |
| `warp_travel` | base clip | `0x13fe` (peach) |
| `win1` | Ultimate | `j02win1` |
| `win2` | Ultimate | `j02win2` |
| `win3` | Ultimate | `j02win3` |

### B. Ike: clips (47 recipes, 46 from Ultimate)

Recipe = `art/p6/ike/anims/<recipe>.json`; built clip = `builds/p6-art/ike/anims/<recipe>.o2r`.

| Recipe | Built from now | Ultimate motion(s) / source clip |
|---|---|---|
| `aether_fall` | Ultimate | `d02specialhi3` |
| `aether_land` | Ultimate | `d02specialhi4` |
| `aether_rise` | Ultimate | `d02specialhi1`, `d02specialhi2` |
| `bair` | Ultimate | `c05attackairb` |
| `counter` | Ultimate | `d03speciallw` |
| `counter_hit` | Ultimate | `d03speciallwhit` |
| `dair` | Ultimate | `c05attackairlw` |
| `dash` | Ultimate | `c00attackdash` |
| `dsmash` | Ultimate | `c04attacklw4` |
| `dtilt` | Ultimate | `c02attacklw3` |
| `entry` | base clip | `0xd4d` (roy) |
| `fair` | Ultimate | `c05attackairf` |
| `fall` | Ultimate | `a04fall` |
| `fall_aerial` | Ultimate | `a04fallaerial` |
| `fsmash` | Ultimate | `c03attacks4s` |
| `ftilt` | Ultimate | `c01attacks3s` |
| `jab1` | Ultimate | `c00attack11` |
| `jab2` | Ultimate | `c00attack12` |
| `jab3` | Ultimate | `c00attack13` |
| `jump_aerial_b` | Ultimate | `a03jumpaerialb` |
| `jump_aerial_f` | Ultimate | `a03jumpaerialf` |
| `jump_b` | Ultimate | `a03jumpb` |
| `jump_f` | Ultimate | `a03jumpf` |
| `kneebend` | Ultimate | `a03jumpsquat` |
| `landing_air_b` | Ultimate | `c05landingairb` |
| `landing_air_f` | Ultimate | `c05landingairf` |
| `landing_air_hi` | Ultimate | `c05landingairhi` |
| `landing_air_lw` | Ultimate | `c05landingairlw` |
| `landing_air_n` | Ultimate | `c05landingairn` |
| `landing_heavy` | Ultimate | `a05landingheavy` |
| `landing_light` | Ultimate | `a05landinglight` |
| `nair` | Ultimate | `c05attackairn` |
| `qd_charge` | Ultimate | `d01specialsstart` |
| `qd_dash` | Ultimate | `d01specialsdash` |
| `qd_end` | Ultimate | `d01specialsend` |
| `qd_slash` | Ultimate | `d01specialsattack` |
| `squat` | Ultimate | `a05squat` |
| `squat_rv` | Ultimate | `a05squatrv` |
| `squat_wait` | Ultimate | `a05squatwait` |
| `uair` | Ultimate | `c05attackairhi` |
| `usmash` | Ultimate | `c04attackhi4` |
| `utilt` | Ultimate | `c02attackhi3` |
| `wait` | Ultimate | `a00wait1` |
| `walk_end` | Ultimate | `a01walkbraker` |
| `walk_fast` | Ultimate | `a01walkfast` |
| `walk_middle` | Ultimate | `a01walkmiddle` |
| `walk_slow` | Ultimate | `a01walkslow` |

### C. Palutena: audio (71 WAV files for 71 cues)

Recipe = `scripts/p6/palutena/audio.json`; files read from `decomp/alt_assets/Fighters/Super Smash Bros Ultimate/Fighters/<file>`.

- **Voice (35):** `vc_palutena_002.wav`, `vc_palutena_appeal01.wav`, `vc_palutena_appeal02.wav`, `vc_palutena_appeal03.wav`, `vc_palutena_attack01.wav`, `vc_palutena_attack02.wav`, `vc_palutena_attack03.wav`, `vc_palutena_attack04.wav`, `vc_palutena_attack05.wav`, `vc_palutena_attack06.wav`, `vc_palutena_attack07.wav`, `vc_palutena_cliffcatch.wav`, `vc_palutena_damage01.wav`, `vc_palutena_damage02.wav`, `vc_palutena_damage03.wav`, `vc_palutena_damage_twinkle.wav`, `vc_palutena_damagefly01.wav`, `vc_palutena_damagefly02.wav`, `vc_palutena_furafura.wav`, `vc_palutena_furasleep.wav`, `vc_palutena_heavyget.wav`, `vc_palutena_jump.wav`, `vc_palutena_knockout.wav`, `vc_palutena_missfoot01.wav`, `vc_palutena_ottotto.wav`, `vc_palutena_passive.wav`, `vc_palutena_special_H01.wav`, `vc_palutena_special_L01.wav`, `vc_palutena_special_L02.wav`, `vc_palutena_special_N01.wav`, `vc_palutena_special_S01.wav`, `vc_palutena_wakeup.wav`, `vc_palutena_win01.wav`, `vc_palutena_win02.wav`, `vc_palutena_win03.wav`
- **Sound effects (34):** `se_palutena_appeal_L01.wav`, `se_palutena_appear01.wav`, `se_palutena_attack100.wav`, `se_palutena_attack100end.wav`, `se_palutena_attackair_B01.wav`, `se_palutena_attackair_F01.wav`, `se_palutena_attackair_H01.wav`, `se_palutena_attackair_L01.wav`, `se_palutena_attackair_N01.wav`, `se_palutena_attackdash.wav`, `se_palutena_attackhard_H01.wav`, `se_palutena_attackhard_L01.wav`, `se_palutena_attackhard_S01.wav`, `se_palutena_dash_start.wav`, `se_palutena_dash_stop.wav`, `se_palutena_escape.wav`, `se_palutena_escapeair.wav`, `se_palutena_jump01.wav`, `se_palutena_jump02.wav`, `se_palutena_landing01.wav`, `se_palutena_smash_H01.wav`, `se_palutena_smash_L01.wav`, `se_palutena_smash_S01.wav`, `se_palutena_special_H01.wav`, `se_palutena_special_H02.wav`, `se_palutena_special_L01.wav`, `se_palutena_special_L02.wav`, `se_palutena_special_N01.wav`, `se_palutena_special_N02.wav`, `se_palutena_special_N03.wav`, `se_palutena_swing_l.wav`, `se_palutena_swing_m.wav`, `se_palutena_swing_s.wav`, `se_palutena_throw.wav`
- **Announcer and cheer (2):** `vc_narration_characall_palutena.wav`, `vc_palutena_cheer.wav`

### D. Ike: audio (48 WAV files for 48 cues)

Recipe = `scripts/p6/ike/audio.json`; files read from `decomp/alt_assets/Fighters/Super Smash Bros Ultimate/Fighters/<file>`.

- **Voice (26):** `vc_ike_appeal01.wav`, `vc_ike_attack01.wav`, `vc_ike_attack02.wav`, `vc_ike_attack03.wav`, `vc_ike_attack04.wav`, `vc_ike_attack05.wav`, `vc_ike_attack06.wav`, `vc_ike_attack07.wav`, `vc_ike_cliffcatch.wav`, `vc_ike_damage01.wav`, `vc_ike_damage02.wav`, `vc_ike_damage_twinkle.wav`, `vc_ike_damagefly01.wav`, `vc_ike_furafura.wav`, `vc_ike_furasleep.wav`, `vc_ike_heavyget.wav`, `vc_ike_jump01.wav`, `vc_ike_knockout.wav`, `vc_ike_special_H01.wav`, `vc_ike_special_H02.wav`, `vc_ike_special_L01.wav`, `vc_ike_special_N01.wav`, `vc_ike_special_S01.wav`, `vc_ike_win01.wav`, `vc_ike_win02.wav`, `vc_ike_win03.wav`
- **Sound effects (20):** `se_ike_appear01.wav`, `se_ike_dash_start.wav`, `se_ike_dash_stop.wav`, `se_ike_escape.wav`, `se_ike_landing01.wav`, `se_ike_special_H01.wav`, `se_ike_special_H05.wav`, `se_ike_special_L01.wav`, `se_ike_special_N01.wav`, `se_ike_special_N06.wav`, `se_ike_special_S01.wav`, `se_ike_special_S02.wav`, `se_ike_special_S03.wav`, `se_ike_special_S04.wav`, `se_ike_step_right_m_souen01.wav`, `se_ike_swing_l.wav`, `se_ike_swing_ll.wav`, `se_ike_swing_m.wav`, `se_ike_swing_s.wav`, `se_ike_swordgroundhit.wav`
- **Announcer and cheer (2):** `vc_ike_cheer.wav`, `vc_narration_characall_ike.wav`

