# Palutena — P6 card

**Status:** P2 look approved and in the lab ROM (2026-10-13); P3-P8 planned. **Content:** 20 (`NDS_P4_PALUTENA`). **Base:** Peach
(content 8: setup parent Fox). **Brief:** [../Palutena.md](../Palutena.md),
with owner decisions D2-D4 of 2026-10-12 and D10 of 2026-10-13 (the look
converted from the 3DS kit). **Plan:** [../P6_PLAN.md](../P6_PLAN.md),
Wave 2.

## What the base gives

Peach's export: her own clips for the common moves, her skeleton (the owner
selects 27 joints; the limit is 37) and her held-item model parts (pan,
racket, club, parasol) on her hand. Her Remix code — the float, Peach Bomber,
the parasol, the turnip — is **not** inherited: those overrides are keyed to
Peach's content, and Palutena's statuses name her own routines. Her parent
kind is Fox, so Fox's rapid-jab, reflector and thrown rows are at hand.

## Look (G0, G1)

**Settled 2026-10-13 (night):** the owner's "3DS to DS" model, built with the
owner in Blender on the owner's baseline and approved as built: 428
triangles high, 278 low (`look_reduce.py`), the staff in her right hand on
the held-item row, turned upright for the idle; her limbs play Peach's clips
at their own lengths (translate_scales). How it was made and how it reaches
the game: [P6_STATUS look record](../P6_STATUS.md#palutena-look-record). The
brief below is the bake-off's (kept for the parts still to come: wings,
halo, pillar, gate, costumes, Kirby hat).

Source: picked by the owner from two best-effort candidates at the same
budget (plan D12, [Look bake-off](../P6_PLAN.md#look-bake-off-g0)): **Track
K**, the 3DS kit's Palutena (`decomp/alt_assets/Fighters/Super Smash Bros 3DS/Palutena/`,
local; plan D10) rebuilt onto Peach's skeleton by ring retopology and
hand-tuned (A3b); **Track R**, Peach's own model restyled into Palutena with
the fitted kit model as the target (A3c). Regions can mix. Smash 4's design is
the one Ultimate keeps; the Ultimate model is no longer needed for the look.
Both candidates must meet every line below.

- Both details: long, slim, sleeveless white dress with a high side slit;
  gold trim and accessories; long straight green hair, split between the
  head and upper-back joints so it does not cut through the back when the
  head turns; gold tiara; white over-knee stocking showing through the slit.
  Track K binds the kit's hair swing joints that way, and its skirt joints to
  Peach's dress joints if her tree has them, else to the thighs. The kit's
  UVs need mirrored-repeat sampling (its note).
- Budget: at most 426 triangles high and 278 low, staff and shield included
  (Peach 371 / 242); the kit's low model draws 1,930, its staff alone 267.
- Track R's restyle of Peach: the gown pulled into the slim dress and its
  slit, the puffed sleeves and gloves into bare arms with gold armlets, the
  hair lengthened to her knees, the crown reshaped into the tiara.
- **Staff:** the kit's (its RStickN part), rebuilt once and shared by both
  tracks: tall, gold-topped, in her right hand as the model part where
  Peach's held items sit; drawn in every status.
- **Shield:** the kit's (its ShieldN part), shared the same way, on her left
  forearm.
- **Joints appended:** two wings (forward and down smashes) from the kit's
  `godwing.dae` (774 triangles, 11 joints, rebuilt with a few joints a wing);
  the halo (up air, down taunt, an idle accent) and the light pillar (up
  smash), authored. Each is hidden unless a script shows it.
- Removed: Peach's crown, parasol and held-item variants.
- Costumes: Peach's count, chosen from the kit's eight on the palette sheet
  (white first); each is a palette over one baked texture set.
- Face and hands: as Ike's card (neutral set baked; the kit's expressions on
  Peach's face frames, if any; hand poses onto Peach's hand variants).
- Entry gate: the kit's `gate.dae` (two doors, 216 triangles), rebuilt as
  the gate article.
- Kirby hat: green hair and tiara, from the picked model's head spans.

## Move card (approved 2026-10-13, P1)

**Authored 2026-10-13 (P3, P4).** Specials as below, in
`src/port/nds_p4_palutena.c` (her own code row: Peach's float, parasol and
turnip are not hers). Clips, retargeted onto Peach's tree (`anim.py`;
`art/p6/palutena/anims`): Explosive Flame on Ness's PK Fire; Warp on
Peach's parasol-opening start and landing; Counter / Reflect on Marth's
counter and counterattack; the jabs, tilts, up and down smashes, neutral
and up air on Marth's, the staff turned to follow his sword; the forward
smash on Peach's own (the staff where her pan was); forward air on Mario's
neutral air (a flying kick), back air on Captain's (the backfist, the shield
arm), down air on Captain's stomp. The dash attack is Peach's clip with
the shield charge's hitboxes on the shield arm. Scripts in
`scripts/p6/palutena/motions` with her cues. Not yet: the wings (forward
and down smash use the staff), the halo joint (up air hits above her), the
pillar effect (the up smash's column is hitboxes and sparkles), the taunt,
the entry gate and the victory poses (Peach's for now).

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
| Entry | entry rows | new: walks out of golden halo-shaped doors | gate article (the kit's gate, converted) |
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
missing. The 3DS kit's Palutena audio is not used: 40 of its 52 voice clips
and 50 of its 51 effects are the same recordings at 14-16 kHz, unnamed; its
12 other voice lines are unidentified spares. Announcer "Palutena" (D7): Ultimate's narrator clip
(`vc_narration_characall_palutena.wav`, 1.31 s), level-matched to SSB64's
announcer cues; select screen and Results. Crowd chant (D8): Ultimate's
cheer (`vc_palutena_cheer.wav`, 1.72 s), downmixed to mono. Victory (D9): an
excerpt of Remix's Kid Icarus "Overworld" sequence as a jingle.

## Kirby

A custom power: Explosive Flame on Kirby's own PK Fire clips
(`FTKirbyAnimPKFireGround/Air`), the same article and D-pad distances. Hat:
green hair and tiara.

Built 2026-10-14 (A11). `scripts/p4/p4_kirby.py` CUSTOM_POWERS["palutena"]:
two statuses on Kirby's PK Fire motions and scripts (his cast, his call, the
flag at frame 20), her code in `nds_p4_palutena.c` (`#if NDS_P2_KIRBY`): the
stick at the cast picks the distance, and at the flag her script's three 1%
bursts and the 15% blast come out on TopN at that distance and her height,
timed as `motions/flame.ms` times them, with her flame effects and her two
explosion sounds (`NDS_P4_KIRBY_PALUTENA_FGM_*`, from `audio.json`). Ground
and air switch at the same frame keeping the hitbox. The hat is one list in
file 0x1944 (`p4_kirby_custom_hats.py`): her head's hair span over the top
of his head, a gold ring under its front edge (her staff's band texture: her
own circlet is painted on her face and turns brown at 16 colours), and a
sheet of her hair panels' texture down his back, a third of the way round
behind him. `scripts/p6/hat_preview.py` renders it on Kirby from four sides
(local review image).

Lab check (four-CPU lab ROM, Kirby in port 0 tapping B against three CPU
Palutenas, `p6kirbypal.ps1`): he copies her (copy 20) and casts Explosive
Flame on the ground and in the air: bursts 1% at frames 21, 25 and 31, the
blast 15% (staled to 12-13 when repeated) at 37, fire, on TopN 1,000 ahead
and 320 up; his hair draws in battle; 0 native failures, 0 lean declines.

## CPU

Coverage rows from the clips and scripts (A12). Recovery: double jump, then
Warp toward the ledge's direction. Post-process: Explosive Flame picks the
distance nearest the target.

Built 2026-10-14. Her spec's `cpu` takes Peach's attack rows (named from
Remix's `Peach/AI/Attacks.asm`, in table order) and measures each on her own
clips and scripts with `scripts/p6/coverage.py` (the first active frame, the
box the hitbox spheres sweep while active, in world units): Explosive Flame
over its three distances, the dash attack with its slide from run speed,
the Warp rows dropped (no hitbox). The rows (first frame, x, y):

| Row | Frame | x | y |
|---|---|---|---|
| Jab | 4 | 85..622 | 155..806 |
| Down smash | 6 | -685..893 | -217..303 |
| Grab | 6 | 119..403 | 199..483 |
| Forward tilt | 7 | -22..714 | -4..573 |
| Up tilt | 5 | -135..440 | 243..845 |
| Down tilt | 6 | 236..727 | 49..320 |
| Up smash | 12 | 82..458 | 265..1394 |
| Forward smash | 15 | -138..618 | 36..904 |
| Explosive Flame (ground and air) | 18 | 496..1874 | 164..594 |
| Dash attack | 5 | 326..952 | -7..477 |
| Neutral air | 5 | -243..646 | 178..825 |
| Back air | 7 | -411..-59 | 207..536 |
| Up air | 5 | -518..370 | 214..805 |
| Forward air | 3 | -244..285 | 106..437 |
| Down air | 7 | -264..116 | -69..322 |

In her code a CPU's Explosive Flame takes the distance nearest its target
ahead, and a CPU recovering warps toward its recovery target (the nearest of
the eight directions). Lab (four CPU Palutenas, 60 s): every normal, smash,
aerial, the grab, Explosive Flame (a target 322 ahead took the near
distance, one 1,112 ahead the middle) and Warp toward the ledge while
recovering; 0 native failures, 0 lean declines.

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
      same match. (Her copy lab-checked 2026-10-14; Peach's in the same match
      still to witness.)

## Closure record

Empty until P8: hashes of the art export and the build, the probe's witness
block, approved renders, the worst-case search result, P3 status.
