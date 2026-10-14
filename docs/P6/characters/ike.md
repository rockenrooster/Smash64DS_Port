# Ike — P6 card

**Status:** planned. **Content:** 19 (`NDS_P4_IKE`). **Base:** Roy (content 6:
setup parent Captain, the Marth/Roy JointTree). **Brief:** [../Ike.md](../Ike.md),
with owner decisions D5 and D6 of 2026-10-12 (Ultimate's Radiant Dawn voice,
Ultimate's sound effects) and D10 of 2026-10-13 (the look converted from the
3DS kit).
**Plan:** [../P6_PLAN.md](../P6_PLAN.md), Wave 1.

## What the base gives

Roy's export: 224 motions (213 own animations, 126 own scripts), 249 statuses,
a file closure of 168 files, many of them Marth's (Roy's action table names
MARTH_* clips for most common moves). Ported routines: Flare Blade
(`nds_p4_roy.c`), and Marth's Dolphin Slash and Dancing Blade, which Roy runs
as Blazer and Double-Edge Dance (`nds_p4_marth.c`); Marth's counter is
Remix's head of `ftParamUpdateDamage` (`NDSP4Overrides.update_damage`).
Roy's jab is one hit (Jab1; Jab2 disabled). His entry is Link's wave and beam
on MARTH_ENTRY_EFFECTS. His Kirby power is Flare Blade.

Ike keeps Roy's skeleton, so every Roy and Marth clip plays on him.

## Look (G1)

Owner decision 2026-10-14 (after the play-p6b playtest ROM): "Just use roy;
Make hair blue, recolor outfit to match Ike, add a headband, gold sword,
slightly smaller eyes, slightly broader shoulders. Ike doesn't need very much
at all since Roy is so close." This replaces the K/R bake-off for Ike.

Built (route R, `scripts/p6/look_r.py compose` with
`art/p6/ike/look/ik_low.json`, written by `model_writer.py` over Roy's files
as Ike's own main and model files 0x1A00 and 0x1A03, spec `files`):

- Roy's low model (202 triangles; the look is written at both details) on
  Roy's tree, skeleton and budget.
- Colours by palette rules per material (hue, saturation, value), applied to
  every costume palette in place, so each of Roy's six costumes keeps its own
  recolour: costume 0 navy hair, charcoal-navy tunic, dark red cape, gold
  trim; 1 red, 2 green, 3 blue, 4 gold, 5 black (Roy's order: the team
  costumes are Roy's rows).
- Headband: Roy's circlet recoloured to Ike's olive-green cloth.
- Ragnell: Roy's sword recoloured gold.
- Eyes 0.85x (the face texture's eye region scaled about its centre,
  `texel_scale`).
- Shoulders broader: the arm roots 8 units out, the shoulder pads 5, the
  torso's shoulder corners moved with them (`joint_shift`, `place`).
- The writer ends every list in the default cull state: Roy's preludes clear
  the back cull for his two-sided parts and restored it in spans the look
  re-emits; a list left two-sided handed the next joint a no-cull state the
  runtime resets, and the lean path declined every Ike draw (Policy).

Lab (four CPUs, Ike in port 0): drawn, 0 native failures, 0 lean declines.
Next: his Kirby hat, portrait and stock icon from this look.

## Move card (approved 2026-10-13, I1)

Damage and timing numbers are lab defaults on SSB64's scale, tuned against
Roy; Brawl gives the motion and the intent only.

| Move | Rows | Clip | Script | Code |
|---|---|---|---|---|
| Jab 1 | Attack11 | new: left jab | ~3% | — |
| Jab 2 | Attack12 | new: kick | ~3% | — |
| Jab 3 | Attack13; jab_3 rows on (Captain's timer) | new: overhead slash | ~6%, launches | jab tables (S3) |
| Forward tilt | AttackS3 | Roy's (MARTH_F_TILT) at 0.85 speed | Roy's damage x1.25; frame-speed command | — |
| Dash attack | AttackDash | new: heavy upward swing | Roy's damage x1.2, launches upward | — |
| Forward smash | AttackS4 | Roy's (MARTH_F_SMASH) | Roy's damage x1.2; his fire Effect commands dropped | — |
| Up smash | AttackHi4 | new: wide arc from his left over his head to his right | hitboxes along the arc, Roy's damage x1.1 | — |
| Up tilt, down tilt, down smash, aerials, grab and throws, floor and edge attacks, dodges | Roy's | Roy's | Roy's, sword hitboxes resized | — |
| Neutral B: **Quick Draw** | new: charge, dash, slash, end; ground and air | new: charge pose, dash pose, end; slash = Roy's forward tilt clip | dash hitbox; the slash on contact | `nds_p4_ike.c` |
| Down B: **Eruption** | Roy's eight Flare Blade rows | Roy's | Roy's; blue fire: effect tint, blue colour animations, blue trail | `nds_p4_roy.c` |
| Up B: **Aether** | new: toss and rise, spin and descent, landing; ground and air | new (three) | sword hitboxes; armour on the rise (Remix Armour command); no fire; no landing shockwave | `nds_p4_ike.c`, on the structure of `ftkirbyspecialhi.c` |
| Taunt: **Counter** | Appeal sends him to counter rows copied from Marth's down special | Marth's counter clips | Marth's counter | appeal override seam; `nds_p4_marth.c` |
| Entry | Roy's | Roy's | Roy's | — |
| Results | Roy's three victory poses and clap | Roy's | — | — |

How the new clips are made (I3, 2026-10-13; recipes in `art/p6/ike/anims/`,
scripts in `scripts/p6/ike/motions/`, rows in `scripts/p6/ike/spec.json`):

- Jab 1: Captain's jab retargeted onto Roy's tree; hitboxes on the left
  forearm and upper arm.
- Jab 2: Captain's forward-tilt kick retargeted, at 0.65 of its length (a
  grounded straight kick; Mario's jab-3 kick leaves the ground).
- Jab 3: Marth's second jab, a rising slash, as an appended motion (224) and
  status (249, Captain's Attack13 routines) that `jab_3_action` names: Roy's
  status table has his entry where Captain's Attack13 is (motion 195, status
  220). Captain's jab chain timers (`attack1_followup_frames` 22 in Ike's main
  file, `jab_3_timer` Captain's 24).
- Dash attack: Marth's dash attack (a lunge with a rising swing) at 1.15
  times its length; Roy's three hitboxes at 1.2 times his damage, sent up (80).
- Up smash: Marth's up tilt (an arc from in front over his head to behind)
  at 1.4 times its length; Roy's final hit at 1.1 (14 and 13), his launch.
- Forward tilt and forward smash: Roy's scripts edited (`script_edits`): the
  tilt at 0.85 speed and 1.25 times the damage; the smash at 1.2 times the
  damage, his fire hitboxes slash and his fire hit sounds dropped.
- Ike's up tilt stays Roy's (an overhead arc), so jab 3 and the up smash come
  from other clips.

The specials (I4, 2026-10-13; `src/port/nds_p4_ike.c`, appended motions
225-233 and statuses 250-263 in the spec; the starters are the spec's
`ground_nsp`/`air_nsp`/`ground_usp`/`air_usp` rows):

- Quick Draw: the charge holds Marth's dash-attack stance (frame 5); the dash
  his stride (frames 3-5), with a 4% hold-in hit; contact (`proc_hit`,
  `proc_shield`) cuts to the slash on Roy's forward-tilt clip (6% plus up to
  9 by charge); a miss plays Marth's dash swing and recovery at 1.15 (about
  40 frames). The dash runs off an edge and ends in special fall; the other
  statuses stop at one.
- Aether: Roy's Blazer crouch and rise (frames 0-22, the last held 8),
  jumping at frame 9 (110 up, 100 from the air), three carrying hits and
  armour to frame 18; the plunge at 130 a frame on Roy's down-air frame 8
  with a meteor; the landing on Roy's Flare Blade end (the sword planted)
  with a 6% hit and no shockwave. The ledge can be caught on the way.
- Counter: Ike's taunt runs his counter status through
  `NDSP4Overrides.status_replace` (Appeal stays in `status_id`); Marth's
  window (frames 6-30), clips and 8% strike; `update_damage` counts the hit.
- Eruption: Roy's Flare Blade rows, blue (A10, 2026-10-14). The charge glow
  is a copy of Roy's ROY_CHARGE colour animation with red and blue swapped
  (spec `colanims`): `scripts/p4/p4_colanims.py` writes it as a row of its
  own after Remix's table (0x8C), and a SetColAnim of 0x7E by Ike, or by a
  Kirby holding his power, plays it (`ndsP4ColAnimRemap`,
  `reloc_backend_compat_shims.c`). The swing's trail (Remix trail 21) runs
  blue to azure (spec `sword_trails`). The hit's burn is still the source's
  orange fire (`efManagerDamageFireMakeEffect` takes no tint). Lab: a
  forced charge on port 0 plays 0x8C, 0 native failures.

Defaults for the new specials:

- **Quick Draw:** hold B to charge for up to 60 frames; on release he dashes
  for 8-20 frames (by charge) at three times Roy's run speed with a hitbox; a
  hit cuts to the slash; a miss ends in about 40 frames of lag. From the air,
  or off an edge, he ends in special fall.
- **Aether:** Kirby's Final Cutter state machine (rise, apex, descent,
  landing; `ftkirbyspecialhi.c` constants as the starting point) with Ike's
  statuses and clips; the source's landing weapon (`wpKirbyCutter`) is not
  made. Ledge catch during the descent as Final Cutter allows.
- **Counter:** Marth's window and return rule. SSB64's Appeal is ground-only,
  so the counter is too.

## Audio (local inputs, never committed)

Both sets are in `decomp/alt_assets/Fighters/Super Smash Bros Ultimate/Fighters/`
(owner supplied, ignored). The build resamples them to the pack's rates and
encodes IMA; they play through the cue cache like every content's sounds.
The clips used come to well under 1 MB of NitroFS.

- **Voice (D5): the Radiant Dawn set, `vc_ike_c01`** (Greg Chun in the older
  Ike's tone; 36 clips named by move, 48 kHz, about 28 s). attack01-07 for the
  attack shouts; damage01-02, damagefly01-02, damage_twinkle (star KO),
  knockout (KO), cliffcatch, jump01, heavyget (heavy item), furafura (dizzy),
  furasleep, ottotto and missfoot01-02 (teeter, trip), passive (tech); the
  specials special_N01-02 (Eruption), special_S01 (Quick Draw), special_H01-02
  (Aether), special_L01 (the counter, on his taunt); win01-03. appeal01-03 are
  spare (his taunt is the counter) unless the sample sheet gives one to the
  entry; final01 and swimup have no SSB64 use.
- **Effects (D6): `se_ike`** (61 clips, 48 kHz, about 46 s): swing_s, swing_m,
  swing_l and swing_ll for his swings; special_N01-N11 for Eruption's charge
  and blast; special_S01-05 for Quick Draw; special_H01-H06 for Aether;
  special_L01-02 for the counter; appear01 for the entry; his armoured steps
  (souen), jumps, landings, dash, dodges, squat, rise, swordgroundhit and the
  aerial whooshes. Hits keep SSB64's own hit sounds. final01-06 (his Final
  Smash) and `se_ike_c01` (the other design's step variants) are unused.
- The spec maps every Roy FGM id that his reused scripts and kind tables name
  to one of these clips (voice slots to voices, swings to swings, Flare
  Blade's fire and charge to Eruption's), and his new scripts name Ike's clips
  directly. The owner approves the map by ear on the sample sheet.
- The Brawl clips supplied first (`Super Smash Bros brawl/Ike (International)/`)
  stay local as a reference.
- Built (A8, 2026-10-13): `scripts/p6/ike/audio.json` names 46 cues (the
  voices, the narrator's call, the cheer, his swings, steps, landing, dash,
  dodge, entry and specials) and maps 42 ids his scripts, attributes and
  kind tables play (Roy's voices, Captain's steps and landing, the vanilla
  sword swings) to them; `scripts/p6/p6_audio.py` trims, normalizes and
  resamples each (voices 16 kHz, effects 22.05 kHz) into the P4 pack under
  ids 0x700 on (about 330 KB of IMA). His own scripts name cues as
  `cue:<name>`.
- The 3DS kit's Ike audio is not used: its voice is the Brawl set (37 of 39
  clips match outright) and its effects are `se_ike`'s recordings (58 of 65), at
  14-16 kHz and unnamed.
- Announcer "Ike" (D7): Ultimate's narrator clip
  (`vc_narration_characall_ike.wav`, 0.87 s), level-matched to SSB64's
  announcer cues; select screen and Results.
- Crowd chant (D8): Ultimate's cheer (`vc_ike_cheer.wav`, 1.50 s), downmixed
  to mono, on his crowd-chant row.
- Victory: the Fire Emblem theme (Marth's and Roy's row).

## Kirby

Power: Eruption, through Roy's Kirby power rows with Ike's tint, so a Kirby
holding Ike's power charges blue fire. Hat: Ike's hair. With Roy in the same
match, Kirby holding Roy's power keeps Roy's red fire and Roy's hat.

## CPU

Coverage rows for every attack from the clips and scripts (A12). Recovery:
Aether when below the ledge; Quick Draw toward the ledge when level with it.
Post-process: no Quick Draw toward open air unless it reaches the ledge.

Done 2026-10-14 (spec `cpu`, Roy's `AI/Attacks.asm` rows). Measured on his
own moves: jab 1 (frames 5-8), the forward tilt at 0.85 (7-10), the up smash
(7-17), the dash attack with its run travel (14-18, x 872 to 1,620). Kept:
Roy's hand-tuned rows for the moves he plays as Roy does (tilts, down smash,
the forward smash's timing, grab, aerials, Eruption's rows and held rows),
and Blazer's row for Aether, since both rise through their hits and Aether's
rise is code, not clip. Dropped: Shield Breaker's row; Quick Draw is a
charged dash. Census: four Ike CPUs over 1,800 frames use the jabs, tilts,
smashes, aerials, Eruption on the ground and in the air, and Aether; 0 native
failures, 0 lean declines. Not yet: Quick Draw in the CPU's recovery (the CPU
recovers with Aether).

## Code (`src/port/nds_p4_ike.c`)

Quick Draw statuses; Aether statuses; the Appeal override into the counter
rows; the initial script that clears the charge. Uses `nds_p4_roy.c` (Flare
Blade) and `nds_p4_marth.c` (counter), compiled when Ike is enabled and they
are not.

## Directed witnesses

- [ ] Quick Draw at charge 0, 30 and 60: distances; hit and miss; from the
      air and off an edge into special fall; four Ikes keep separate charges.
- [ ] Aether: rise, descent and landing on the floor and on a platform; ledge
      catch during the descent; armour takes a hit on the rise; no cutter
      weapon made.
- [ ] Eruption: blue fire at every charge level; the strong end's 10% recoil.
- [ ] Taunt counter: counters a melee hit; a CPU taunting does not break it.
- [ ] Roy, Marth and Ike in one match: shared clips loaded once; each keeps
      his own colours, sword and effects.
- [ ] Kirby copies Ike (blue fire, Ike's hat) and Roy (red fire, Roy's hat)
      in the same match.
- [ ] No Roy fire on the forward smash; no Double-Edge Dance or Blazer.
- [ ] No Roy sound: every voice and effect cue of a four-Ike match plays an
      Ike clip (FGM census), 0 misses.

## Closure record

Empty until I6: hashes of the art export and the build, the probe's witness
block, approved renders, the worst-case search result, P3 status.
