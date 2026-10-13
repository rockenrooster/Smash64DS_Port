# Palutena — P6 card

**Status:** planned. **Content:** 20 (`NDS_P4_PALUTENA`). **Base:** Peach
(content 8: setup parent Fox). **Brief:** [../Palutena.md](../Palutena.md),
with owner decisions D2-D4 of 2026-10-12. **Plan:** [../P6_PLAN.md](../P6_PLAN.md),
Wave 2.

## What the base gives

Peach's export: her own clips for the common moves, her skeleton (the owner
selects 27 joints; the limit is 37) and her held-item model parts (pan,
racket, club, parasol) on her hand. Her Remix code — the float, Peach Bomber,
the parasol, the turnip — is **not** inherited: those overrides are keyed to
Peach's content, and Palutena's statuses name her own routines. Her parent
kind is Fox, so Fox's rapid-jab, reflector and thrown rows are at hand.

## Look (G1)

Reference: the Ultimate model in the local reference scene only (D1).

- Peach's meshes restyled, both details: long, slim, sleeveless white dress
  with a high side slit; gold trim and accessories; long straight green hair
  (split between the head and upper-back joints so it does not cut through
  the back when the head turns); gold tiara; white over-knee stocking showing
  through the slit.
- **Staff:** tall, gold-topped, in her right hand as the model part where
  Peach's held items sit; drawn in every status.
- **Shield:** on her left forearm.
- **Joints appended:** two wings (forward and down smashes), the halo (up
  air, down taunt, an idle accent), the light pillar (up smash). Each is
  hidden unless a script shows it.
- Removed: Peach's crown, parasol and held-item variants.
- Costumes: Peach's count; default white and green, alternates and team
  colours on the sheet.
- Kirby hat: green hair and tiara, from her head spans.

## Move card (draft for P1)

Motions follow Ultimate's moves (SmashWiki's Palutena (SSBU) descriptions);
SSB64 has no pummel, no up or down throw and no smash charge. Numbers are lab
defaults on SSB64's scale, tuned against Peach.

| Move | Rows | Clip | Script / code |
|---|---|---|---|
| Jab | Attack11, then the rapid-jab rows (Remix's rapid_jab rows; Fox has them) | new: staff thrust with a light blast; light-burst loop; upward staff finisher | multi-hit loop, launcher finisher |
| Forward tilt | AttackS3 | new: staff spun ahead of her by telekinesis | two hits |
| Up tilt | AttackHi3 | new: kneels, spins the staff overhead | multi-hit |
| Down tilt | AttackLw3 | new: kneels, spins the staff along the ground | low hits |
| Dash attack | AttackDash | new: **shield charge** (D3) | shield hitbox |
| Forward smash | AttackS4 | new: wings conjured and flapped forward | hitboxes on the wing joints |
| Up smash | AttackHi4 | new: staff raised, a cyan light pillar erupts ahead of her | a tall column of hitboxes on the pillar joint; the highest vertical reach of her moves |
| Down smash | AttackLw4 | new: wings flap downward on both sides | wing joints |
| Neutral air | AttackAirN | new: staff spun beside her right arm | multi-hit |
| Forward air | AttackAirF | new: flying kick | — |
| Back air | AttackAirB | new: backfist with the shield arm | — |
| Up air | AttackAirHi | new: twirl, halo above her | hitboxes on the halo joint |
| Down air | AttackAirLw | new: downward roundhouse kick | — |
| Grab | Catch, CatchWait | new: telekinetic clutch | Peach's grab box, reach +20% (lab default) |
| Forward throw | ThrowF | new: telekinetic toss forward, arms out | victim pairing per the parent's thrown rows |
| Back throw | ThrowB | new: lifts and flings the victim behind her in an inverted arabesque | same |
| Floor and edge attacks | DownAttack, CliffAttack | new: staff swings behind and ahead; edge leg sweep | fallback: Peach's clips with the staff hold |
| Neutral B: **Explosive Flame** (D2) | new: cast, ground and air | new: staff pointed ahead | see below; `nds_p4_palutena.c` |
| Up B: **Warp** | new: vanish, travel, reappear; ground and air | new: vanish and appear poses | see below |
| Down B: **Counter / Reflect** | new: stance, counterattack; ground and air | new | see below |
| Taunt | Appeal | new: Ultimate's down taunt (D4): lets go of the staff beside her, brushes her hair back, halo glows, retakes the staff | — |
| Entry | entry rows | new: walks out of golden halo-shaped doors | gate article (authored) |
| Idle | Wait | new: staff upright | — |
| Walk, run, jumps, damage, shield, dodges, ledge, sleep and the rest | Peach's | Peach's, through the staff-hold transform | — |
| Results | three victory poses; clap | new (Ultimate's three, simplified); Peach's clap with the staff hold | — |

Defaults for the specials:

- **Explosive Flame:** at the cast frame the D-pad picks the distance: back
  about 600 units, neutral about 1,000, forward about 1,400, at her height on
  the ground or in the air. The explosion is an authored article modelled on
  Ness's PK Fire pillar (`itNessPKFire`: a stationary multi-hit fire with a
  lifetime): about 3 1% hits then a 15% blast exp. It is not a projectile, so it
  cannot be reflected.
- **Warp:** Meta Knight's Dimensional Cape structure (`nds_p4_metaknight.c`
  DSP): she vanishes, travels about 10 frames in the D-pad's direction (eight
  directions; neutral is up) invisible and intangible, stops at walls and
  floors, and reappears. No hitbox. From the air she ends in special fall.
- **Counter / Reflect:** a stance with a counter window (Marth's numbers as
  the default) during which a melee hit triggers her counterattack, a light
  blast from the staff (Marth's counter override, `NDSP4Overrides.update_damage`),
  and a projectile is reflected (Fox's reflector rule). Ground and air.

## Audio (local inputs, never committed)

Cues come from the Ultimate WAVs in `decomp/alt_assets/Fighters/Super Smash
Bros Ultimate/Fighters/{vc_palutena,se_palutena}` (44 voice and 62 effect
WAVs, 7.4 MB): about 30 cues, mapped to SSB64's voice slots (attacks, damage,
KO, star KO, jump, ledge catch, taunt, victory, specials) and to her scripts'
sound commands. They stream from the FGM pack through the cue cache like every
other content's sounds. The build fails with a message if the folder is
missing. Announcer "Palutena" (D7): Ultimate's narrator clip
(`vc_narration_characall_palutena.wav`, 1.31 s), level-matched to SSB64's
announcer cues; select screen and Results. Crowd chant (D8): Ultimate's
cheer (`vc_palutena_cheer.wav`, 1.72 s), downmixed to mono. Victory (D9): an
excerpt of Remix's Kid Icarus "Overworld" sequence as a jingle.

## Kirby

A custom power: Explosive Flame on Kirby's own PK Fire clips
(`FTKirbyAnimPKFireGround/Air`), the same article and D-pad distances. Hat:
green hair and tiara.

## CPU

Coverage rows from the clips and scripts (A12). Recovery: double jump, then
Warp toward the ledge's direction. Post-process: Explosive Flame picks the
distance nearest the target.

## Code (`src/port/nds_p4_palutena.c`)

Explosive Flame statuses and its article (a P4 item kind, as Peach's turnip
and Dedede's minions are); Warp; Counter/Reflect; the initial script. Joint
effects need no code: her scripts show and hide them.

## Directed witnesses

- [ ] Explosive Flame at each distance, ground and air, over a ledge (no floor
      needed), multi-hit and finisher; four Palutenas' explosions independent.
- [ ] Warp in all eight directions from the ground and the air; into a wall
      and a floor; off stage into special fall; ledge catch afterwards;
      intangible and invisible while travelling.
- [ ] Counter: a melee counter; reflected Fox laser, Mario fireball, Samus
      charge shot and Link arrow; the window's edges; the air version.
- [ ] Staff and shield drawn in every status; wings only in the smashes; halo
      and pillar only where the card shows them.
- [ ] No parasol, turnip, crown, float or Peach voice anywhere. Peach and
      Palutena in one match: Peach still floats, Palutena never does.
- [ ] Both throws against every victim kind.
- [ ] Kirby copies her (explosion and hat) and Peach (Peach's power) in the
      same match.

## Closure record

Empty until P8: hashes of the art export and the build, the probe's witness
block, approved renders, the worst-case search result, P3 status.
