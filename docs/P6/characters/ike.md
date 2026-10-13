# Ike — P6 card

**Status:** planned. **Content:** 19 (`NDS_P4_IKE`). **Base:** Roy (content 6:
setup parent Captain, the Marth/Roy JointTree). **Brief:** [../Ike.md](../Ike.md),
with owner decisions D5 and D6 of 2026-10-12 (Ultimate's Radiant Dawn voice,
Ultimate's sound effects).
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

- Roy's high and low meshes restyled: Ike's heavier build (broad chest and
  shoulders, thick arms, heavy boots), short blue hair and headband, tunic and
  cape after his Brawl design. Colours are fixed on the palette sheet.
- **Ragnell** replaces Roy's sword part: a longer, broader blade, gold (brief).
  Sword hitbox offsets and the sword-trail endpoints of every reused script
  scale by the blade-length ratio measured from the two models.
- No overall scale change: Roy's hurtboxes and map box stay valid.
- Costumes: Roy's count; default colours from the brief, alternates and team
  colours on the sheet.
- Kirby hat: Ike's blue hair and headband, taken from his head spans.

## Move card (draft for I1)

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
