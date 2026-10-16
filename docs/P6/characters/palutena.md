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

**Legs fuller (owner, 2026-10-15: "they look a little too skinny").** The
hand-built legs (5-sided tubes, 2026-10-14) were about 70-80% of the
baseline's width below the knee. Their rings now sit near the baseline's
cross-sections (measured by slicing the baseline's leg mesh): hip ring
0.80 -> 0.86, knee 0.52 -> 0.68, ankle 0.33 -> 0.48 (kit units; the
baseline's ankle, still a taper from the knee as the owner asked), the
shoes unchanged; the knee as wide as the baseline's, the calf and ankle
about 90%. Only the leg rings moved (the work copy's `DS_legs` in the
owner's .blend; the skinny one kept hidden in `_tmp` as
`DS_legs_v4_skinny_bak`); the leg texture is baked again, the low detail
reduced again (`look_reduce.py`, 56 leg triangles as before), and the look
rebuilt through the same chain (`look_blend.py`, `p6_costumes.py`,
`look_skirt.py`, `look_parts.py`, `model_writer.py`). The dress solver
holds the fuller legs as well.

**Shield without its rod (owner, 2026-10-15: "remove the thing that sticks
out from her shield").** The shield sprite's image, baked from the kit, held
the shield and a thin rod with a gold loop beside it (texel columns 77-117
of its 128x64 image), so the rod stood out past her hand. The sprite card
now ends at the shield's rim (u 0.60, the card 6.5 units across instead of
10.8) and the rod's texels are cleared; the old card is kept hidden in
`_tmp` as `DS_shield_sprite_rod_bak`. Nothing else moved.

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

**Owner requirement for her animations (playtest 2026-10-14).** In EVERY
animation the dress surrounds her legs (except at the slit). Her skirt was a
cone of long triangles from the bodice's waist ring (row 2) to a hem on the
pelvis (row 1), so a leg that swung away from the pelvis left it
(`scripts/p6/dress_check.py`: 205 of 218 motions put a leg outside, many by
more than 100 units).

**The skirt on Peach's dress rows (2026-10-14).** Peach's own gown hangs from
her dress rows (row 25 its upper band, row 26 its hem, drawn by row 26 from
row 25's seam load) and her clips key both. `scripts/p6/look_skirt.py`
remeshes Palutena's skirt the same way: a short upper band from the waist
ring to a ring at y 288 (row 1), drawn by the bodice as before, and the lower
band from a copy of that ring on row 25 (raised 6 and drawn in 4% so a turned
ring stays inside the upper band) to the hem on row 26, open at the slit (its
left front, the +x leg) as the approved model is: an upside-down V from the
old skirt's slit top (y 272, a row 25 vertex on the band) to the hem's two
slit vertices (owner playtest 2026-10-14, p6d: the first remesh left the
whole span open below the ring, a flat-topped notch). Row 25's origin moves up to her hip joints' height
(y 280) so the whole thigh hangs inside the band that turns. The seam costs
two of the owner's GX cross slots (15, 16-30), and her look used 14: each
foot takes its own copy of the ankle ring (its origin is the ankle, where the
ring sits) and gives its slot back. (Moving the thighs' top rings onto the
thigh rows instead was tried and reverted: those rows' origins are the
knees, so the thigh tops swung away from the pelvis; owner, 2026-10-14.)
Every clip she plays is keyed for it (`p6_author.py` spec "skirt",
`scripts/p6/skirt_keys.py`): row 25 turns its band toward the middle of her
knees and ankles (a tenth of the way toward straight down, at most 75
degrees) and about that axis so the ring lies along her legs' spread, and
row 26 shapes the hem. Base clips are copied into her window keyed (151 of
Peach's), her own are rewritten in place. Peach's gown keys do not fit her
slim skirt and are replaced.

**Owner p6j (2026-10-14): the slit leg never leaves the dress, and the
dress stretches over a stepping knee.** "The slit leg actually never comes
out of the dress at all on the 3DS" and "if Palutena takes a step forward
where the knee pokes out, the dress should be stretched over it like on the
3DS". The 3DS's clips show what holds: her thighs and knees are always under
the dress, while a shin or a foot may show below the hem (her run's back
foot, her flying kick's shin from under the hem over the knee). So:
- `dress_check.py` measures her legs at points along the leg mesh's tubes
  (hip ring to knee ring to ankle ring; the vertices alone are only those
  rings), the band as it is drawn (each ring's depth and radius by angle
  about row 25's axis, so a tilted or stretched hem is measured as it is):
  the thigh and knee must be inside it and above its hem, the shin must not
  come through its side above the hem, the feet are free. Both legs count;
  the slit leg's worst is reported apart.
- `skirt_keys.py` shapes the hem (row 26 in row 25's frame: its ring scaled
  across and along the spread, tilted about the across axis, moved along the
  spread and up the band, up to 1.45x across and 1.75x along) as the least
  costly shape holding them: a change from rest costs, a point outside costs
  far more, the cloth's length costs (each panel from row 25's ring to its
  hem vertex shortened or stretched: a hem lifted all round is a shorter
  dress, not one held up by a knee), and a change from the frame before
  costs (Nelder-Mead from the frame before's). A knee raised ahead lifts the
  hem's front over it while the back hangs behind the other leg; a stride
  stretches it along the stride. The band's spring lags at most 25 degrees
  (a tumble's spin left the legs behind it).
- Where no hem can hold a leg (another fighter's lunge or kick), the leg is
  held: its thigh and shin turned toward rest by the least of 15%-steps that
  clears the frame, eased over the frames around, and the clip solved again
  (up to four passes).
Dress check: 208 of 221 motions clean, both legs (20 before, the slit leg
counted); what is left is within 11 units but Peach's special motion 198 and
EntryNull (neither hers in play).

Her locomotion is Peach's made hers (spec "pose", `scripts/p6/pose_ops.py`,
applied before the skirt's keys; the owner's 3DS references, 2026-10-14):
the walks with a smaller stride (legs 35% of the way to standing) and the
staff held up as in her wait; the dash, run, run brake and run turn with a
smaller stride (40%) and the body leaning into the run; the turn, knee
bend, jumps, falls and landings with the legs closer (35%); the crouch with
the knees together (45%). Dress check of those motions: walks, dash, falls
and crouch clean, the run within 10 units, the jumps within 40.

Legs flung far out in the borrowed clips (Marth's lunges, the borrowed
kicks, Peach's acrobatics) are held since p6j (above). `pose_ops.py` also
takes "turn" (a row about the model's axes) and key poses ("keys"), and
anim.py recipes take its "ops", for authoring them.

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
`scripts/p6/palutena/motions` with her cues. The wings show in the forward
and down smashes, the halo in the up air, the light pillar in the up smash;
the taunt, the entry gate and the victory poses are hers (2026-10-14,
below).

**Her chest (owner playtest 2026-10-14, p6d: "an upside-down V on her
chest").** The baseline paints the skin between her bust and her neck on
the dress mesh, beside the chest's own skin; at 16 colours the dress
texture's k-means folded that small patch into the cloth's cream, a pale
inverted V on the bust. `look_blend.py` KEEP_PATCHES gives the patch two
palette entries of its own (k-means over the patch), in place of the two
entries whose texels lose least by eye (CIELAB) -- near-duplicate greys, not
the dress's red, which an RGB cost would have dropped. Every other texture
quantizes as before.

**Portrait (owner, 2026-10-14).** The owner's pixel portrait of her (a
framed square of about 32 pixels; local, `builds/p6-art/palutena/owner_art/`):
`scripts/p6/portrait_art.py` trims its frame and the black gap inside it and
keys its dark teal backdrop out (the dark pixels that are not her hair's
green, connected to the edge), and `p6_present.py` frames it over the
select's fire plate as the roster's portraits are ("backgrounds will have to
be changed to match the portrait style of the existing roster").

**Costumes, wings, halo, stock icon (2026-10-14).** Costumes are palettes
over her one index image per texture (P6_PLAN A3b): `blender/costume_corr.py`
records, headless in the 3DS-to-DS work file, where each texel of her
textures sampled the kit; `scripts/p6/p6_costumes.py` samples each kit
alternate at those places and moves every palette entry by its texels' mean
change, so her baked colours and retouches stay where a costume does not
recolour. The dress, chest, arms, hands, legs and staff take costume
palettes (face and hair keep one); `model_writer.py` writes them as the
base's do: a palette MObj each (Peach's palette MObjSub and costume
animation as the template) on the rows and model-part rows that draw them.
Picks, in Peach's costume order (VS uses 0-3 on the buttons and 2, 3, 4 for
the red, blue and green teams): 0 white (c00), 1 black (c06), 2 red (c05),
3 blue (c04), 4 green (c03), then purple (c07), pink (c01), sky (c02). Palette
sheet: `builds/p6-art/palutena/costumes/sheet.png` (local; for the owner's
approval). Her stock icon is the kit's stock icon row (the sheet's 28th),
reduced to the HUD's 8x10 CI4 sprite with one LUT a costume
(`scripts/p6/p6_stock.py`, written in place over Peach's). The victory
poses' wings and the halo are model-part variants of existing rows
(`scripts/p6/look_parts.py`, recipe `art/p6/palutena/look/parts.json`): the
torso row's part 1 draws its list plus the smash wings' art (owner: "use the
new wing art for the victory poses too"), the rig's eight cards posed once
by `wings_pose` (a wings recipe key), spread up and out from her shoulder
blades for the Results camera, which sees her from the front; the head
row's part 2 adds an authored gold ring above her head, two crossed cards:
one faces the battle camera, the other the select and Results cameras. Her up air shows the halo (joint 12);
a status change resets the parts. Her smashes' wings are an effect of their
own (below, Animations).

**Pillar, taunt, entry, victory poses (2026-10-14).** The up smash's light
pillar is one column of cyan light 325 units ahead of her on her floor
(1.25x the first 260, owner p6h notes), 2,500 tall (2x her first, p6f; 1.5x
after p6d, 1.25x before), exactly vertical, standing 36 tics as the 3DS's
(owner p6j; 32 since p6i, 12 before), and its hitboxes are the column (TopN z 325, frames
12-38; their knockback 1.5x since p6h: scale 158/150, base 60, the same
damage): attacks 0 and 1, its
lower (16%) and upper (14%) halves, 400 wide, move every frame between their
half's ends in her code (the hit test sweeps a hitbox from where it was to
where it is), and attacks 2 and 3 at the halves' middles cover the first
frame, which tests a new hitbox's point only. It is an effect, not a model part:
the first build drew it as cards on her hips row (joint 5), which her
animation turned so that the owner saw two columns on her back side (round 2
of the playtest). The script's flag0 at frame 12 (`motions/usmash.ms`) makes
it in her code (`nds_p4_palutena.c`, `NDSP4Overrides.after_proc_map`) as a
world-aligned effect at her floor ahead of her facing, 120 units toward the
camera; its tree is the second in her special file 2 (`scripts/p6/p6_gate.py`
PILLAR_TREE, `p4_articles.py` "pillar"), cards facing the camera, unlit,
from one authored 64x64 CI4 atlas. Owner p6j ("redo the blue pillar column
VFX since we have a good animation to follow for it now"): it follows the
3DS's Heavenly Light in the owner's half-speed Strong_Up_A clip, read on a
model-unit grid (a white core about 85 across in a cyan band about 140; dust
banks either way along the floor). Its cards (`p6_gate.py` PILLAR_CARDS,
atlas `look_parts.py` pillar_light_atlas): the glow (translucent:
G_RM_ZB_XLU_SURF, the DS polygon at its PRIM alpha), the core (white down
its middle to cyan, cut out), the flare on the floor at its foot and the
dust either way (translucent). Her code (`nds_p4_palutena.c`
ndsP4PalutenaPillarProcUpdate) raises the column from the floor with a round
head over three tics (18%, 45%, full), fades the flare by tic 9, rolls the
dust out from tic 2 (full by tic 7, settled by tic 13: the 3DS's frames 26
to 38), shimmers the glow while it stands and thins the column from tic 24
to 36 (the 3DS's frames 50 to 62). Two more rows move the wings' tree to
0x210 (its table word to 0x420). Cards light by an up normal, not
their face's: a two-sided card's back face turned away from the light drew
black (the pillar's first capture). No explosion effects, only the pillar
(round 2). Its whoosh is a plain sound (PlayFGM) since p6h: a stored one
(PlayFGMStoreInfo) is stopped by the source when her hit lands
(ftMainPlayHitSFX). The down taunt (Appeal, `motions/appeal.ms`) keeps Peach's clip
for now and shows the halo with sparkles, her taunt voice and down-taunt
sound. The entry (statuses 0xDC/0xDD, motions 195/196: `motions/entry.ms`,
clip `art/p6/palutena/anims/entry.json`, Marth's entry on her tree, the
staff where his sword is) has her own entry case (`src/port/
nds_p4_palutena.c`, kind table `entry_script`): her gate, the kit's golden
double door (`scripts/p6/p6_gate.py`: its front faces, twelve triangles a
door, a 64x64 CI4 door image, unlit and two-sided) in her special file 2
in place of Peach's turnip (`p6_author.py` special roles: the main's header
word that named the turnip file names the gate, since the source loader
finds special files only in the main's extern closure, and the turnip model
leaves the closure; her Mr. Saturn slot keeps Fox's file), drawn natively as an entry
effect (`p4_articles.py` "gate"). The gate stands 920 units tall, 260 units
toward the camera from her: its doors part from tic 10 over 24 tics, 80
degrees each on its hinge, and it shrinks away from tic 84 to 100. Her halo
shows in the doorway, with sparkles and her entry sound as the doors part
and her line as they open. Victory poses (`menu_motions`, Peach's clips):
Win1 with her wings, Win2 with her halo, Win3 with both, each with Peach's
blinks and her win line where Peach's voice was; the select screen's
Selected pose (row 4, Peach's third clip) shows the halo and a line once it
settles. Results name: "PALUTENA" at x 20, scale 0.7, "WINS!" at x 185
(Remix's Ganondorf and Marth layout; Peach's left "WINS!" over the name).

**Grab and throws (2026-10-14).** Her grab and throws keep Peach's clips,
timing and throw parameters, so her victims' thrown rows (Peach's, Fox's
style) still pair with them; they are made telekinetic by script: the grab's
box reaches 20% farther (its far edge 504 to 604 units ahead) with a psionic
flash where it closes (`motions/catch.ms`); both throws glow psionic at her
hands while she holds the victim and play her throw sound at the release
(`motions/throwf.ms`, `motions/throwb.ms`; Peach's releases at frames 23 and
26). New clips (arms out, the inverted arabesque) were tried as Ness's PK
throws retargeted onto her: his crouched stance read hunched on her, and his
victims' thrown rows (ThrownCommon) differ from Peach's, so they wait for a
better source. The idle needs no new clip: Peach's wait with the staff hold
already stands the staff upright. Lab: four Palutena CPUs over 2,400 frames
grab 14 times, throw forward twice and back once; 0 native failures.

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
| Forward smash | AttackS4 | new: wings conjured and flapped forward | hitboxes on the wings' reach (TopN) and the staff |
| Up smash | AttackHi4 | new: staff raised, a cyan light pillar erupts ahead of her | a tall column of hitboxes where the pillar effect stands; the highest vertical reach of her moves |
| Down smash | AttackLw4 | new: wings flap downward on both sides | the wings' reach front and back (TopN) |
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
  Owner playtest 2026-10-14 (done): distances back 450, neutral 750 (0.75x),
  forward 1,610 (1.15x; 1.5x in the first round was too far); the bursts'
  and the blast's hitboxes twice the size (640 and 860), their effects at
  their own size (owner: the hitbox radius only); a white twinkle on the
  spot six frames before the first burst; no green flash ring; no burst
  sound (Ultimate's special_N01, a ringing, removed in round 2); her call is
  Ultimate's side-special voice ("Explosive flame!"). Kirby's copy matches.
  Round 3 (p6d): the fire is a weapon of its own, made where the twinkle
  shows (her script's flag2 at frame 10, its value the blast's sound), so a
  hit on her after the twinkle no longer cancels it (she still takes the
  hit). It burns as the Fire Flower's flames do (their hit: fire, angle 0, a
  fixed weight-3 knockback, the small burn sound) from 5 tics after the
  twinkle, hitting again every 5 tics, so a fighter caught in it is held
  there; the blast at tic 19 is 19% (1.25x the 15%; 18.75), three tics, the
  large burn sound. The cast to the blast is 0.85x (blast at frame 29, was
  34). The weapon takes the Fire Flower flame's attributes (the common item
  file) and is hidden: her fire is the effects. Kirby's copy makes the same
  weapon at his frame 12. The CPU's rows for it are measured from the
  weapon's numbers (`coverage.py` "weapon"). Owner playtest p6h: two fires
  a few tics apart. A ground/air switch mid-cast sets the twin status at the
  same frame and the source fast-forwards its script there, setting flag2
  again; a cast now makes one fire (status_vars word 1, as Kirby's copy).
  Owner p6i: one more double cast, trigger unknown: no fire is made while
  one of hers is burning at all (her cast outlasts a fire).
- **Warp:** Meta Knight's Dimensional Cape structure (`nds_p4_metaknight.c`
  DSP): she vanishes, travels about 10 frames in the D-pad's direction (eight
  directions; neutral is up) invisible and intangible, stops at walls and
  floors, and reappears. No hitbox. From the air she ends in special fall.
  Owner playtest 2026-10-14 (done): 2,000 units (10 frames at 200, twice
  the first; three times in the first round was too far); on the ground a
  level or downward stick travels along the floor (round 2: the ground
  speed is carried into her air velocity each frame, as every ground physics
  proc does; it did not move before); a wall or ceiling met within 30 degrees of head-on stops
  her, one met at a glancing angle she slides along (Pikachu's Quick Attack
  test), so an upward warp beside a wall rides it to the ledge; white
  twinkles where she vanishes and reappears (no PSI sparkles). A recovering
  CPU travels only as far as its ledge.
- **Counter / Reflect:** a stance with a counter window (Marth's numbers as
  the default) during which a melee hit triggers her counterattack, a light
  blast from the staff (Marth's counter override, `NDSP4Overrides.update_damage`),
  and a projectile is reflected (Fox's reflector rule). Ground and air.

## Animations from Ultimate's motions (owner p6p, 2026-10-15)

Owner p6p review (rating 1/5 -> 2/5, a note per move) with "I also have some
source files for palutenas animations for SSB Ultimate which may help
immensely" (the owner's extraction of Ultimate's `.nuanmb` motions, local,
`../ssbu-recomp/build/palutena_motion`). Every move she plays is now
Ultimate's own motion retargeted onto her rig (`scripts/p6/ultimate.py`,
anim.py recipe "ultimate"; `art/p6/palutena/anims/*.json` name the motion and
the timing, never its data: the motions and the skeleton they play on, the
owner's DAE rip, stay local, P6 D1). Warp stays as the owner approved it;
Turn and TurnRun stay Peach's (their facing flip is the game's).

- **The retarget.** Both rigs' binds are T-poses facing +z, so each of her
  rows takes its Ultimate bone's turn from its bind (W_row = W_bone
  B_bone^-1 B_row). Her proportions are Ultimate's at about 24.9 model units
  an Ultimate unit (her legs' ratio), but Peach's torso turns about the
  pelvis origin, below her hips: the pelvis, torso, shoulders and neck are
  placed where Ultimate's scaled hips, waist, shoulders and neck are (keyed
  translations, bounded), and her ankles reach Ultimate's by two-bone IK, the
  soles kept. The staff takes the Stick bone's pose through a calibration of
  the two staffs (their length and the ornament's plane), held where
  Ultimate's hand holds it (61 units nearer the gem than before: owner p6p,
  "grips staff higher") and leaving the hand as Ultimate's does (the up
  tilt's and neutral air's floating staff). The shield sprite turns with
  the forearm twisted about its length to follow the Shield bone's face.
- **Timing.** Hit frames stay SSB64's (the card's): each recipe maps its
  frames onto Ultimate's piecewise (`time` anchors: Ultimate's frame data's
  hit onto the script's), so wind-ups are quicker where SSB64's moves are
  faster (forward air's kick at 3 against Ultimate's 9); the last frames
  reach Ultimate's last, which is her wait or fall, and attacks ease in from
  the wait or fall over three frames ("blend_from"). Clips the game times
  by length keep Peach's lengths (the walks the attributes' 69/48/60, the
  landings 10, the crouch 5).
- **Where the 3DS differs.** Ultimate turns her back to the camera in its
  rapid jab and up tilt; the 3DS faces the camera: those clips turned the
  whole motion about her vertical ("yaw": -50 and -60), easing back, until
  owner p6r (Ultimate's facing is back: below).
- **The dress follows Ultimate's skirt.** Ultimate's eight skirt chains are
  keyed in every motion; each clip writes their hem's moves beside it (a
  `.skirt.json` guide, p6_author.py hands it to `skirt_keys.py`), and the
  hem solver aims her band at the guide's hem and stays nearest its shape
  (fitted to her five hem parameters each frame) while still holding her
  legs: the dress lies on the floor around her when she kneels, flies up
  with a kick, and the kicking shin leaves it below the knee as the 3DS's
  (owner p6p: "dress doesn't articulate well").
- **Hair.** Her long hair panels moved from the bodice to Peach's row 9
  (the head's child; `look_blend.py` OBJECT_ROWS; a separate list, no seam):
  her Ultimate clips turn it along Ultimate's two back hair
  chains, so it hangs and swings from her head instead of standing out
  behind a leaning torso; every other clip keeps it on the bodice as before
  (`skirt_keys.py` keys row 9).
- **Wings.** Ultimate's godwing is the kit's godwing rig (the same bones and
  rest), so `art/p6/palutena/wings/{fsmash,dsmash}.json` bake Ultimate's
  godwing motions (c03attacks4s, c04attacklw4) onto her wing table, the
  article started where its slam meets the body's hit (her clip's motion
  frames 11 and 14).
- **Moves.** jab 1 c00attack11 (pull back, thrust); jab 2 and the rapid jab
  c00attack100start/c00attack100 (the staff pushed with each burst), the
  finisher c00attackend; dash attack c00attackdash (its TransN travel scaled
  to the old 765 units); tilts c01attacks3s, c02attackhi3, c02attacklw3;
  smashes c03attacks4s, c04attackhi4, c04attacklw4; aerials c05attackair*;
  Explosive Flame d01specials/d01specialairs; Counter d03speciallwr,
  d03specialairlwr and their attacks; down taunt j01appeallwr; entry
  j00entryr; and now her wait, walks, dash, run, brake, jump squat, jumps,
  double jumps (Peach's TransN slots), falls, crouch, landings, platform
  drop, teeter, rolls (Peach's TransN slots) and hit reactions and tumbles.
  Her figatree heap stays Peach's 12,128 bytes: the largest clip, launched
  straight up, drops its shoulder and neck keys to fit.

## Owner p6r (2026-10-15): jabs, down smash, run, dress, attack data

Owner p6r: "much much better 3/5, theme of this review: twitchy and
stiff/non flowy dress are main blockers ... the SSB Ultimate data is working
well, should be more authoritative than the 3DS mp4s"; the down smash's
wings "look wierd at full impact" and it is "too fast use ultimate timing";
the floating run's "legs and dress look twitchy should be smooth and feet
hardly need to move"; "jabs seem to be offset diagonally preventing hitting
enemies"; damage, hit type and angles from Ultimate's attack data (the
owner's `palutena_attack_data.json`, local) for most attacks, not Explosive
Flame.

- **The jabs.** Jab 1's hits were on her shield arm, behind her in
  Ultimate's motion (it thrusts the staff): they are on the staff's head
  and middle now. Jab 2 and the rapid jab turned her toward the camera (the
  3DS's facing, "yaw" -50), which put the staff's thrust 280-550 units off
  her plane: Ultimate's facing is back (the up tilt's too), and her
  hitboxes lie in her plane whatever the motion (`NDSP4Overrides.planar_hits`:
  Ultimate collides in two dimensions, SSB64 tests the third).
- **Down smash** plays Ultimate's motion frame for frame (its hit 16-18,
  Ultimate's 17-19; 67 frames to her first actionable one), keyed every
  other frame for her figatree heap. Its wings, flat on the floor at the
  slam, were edge-on to the side camera (thin crumpled lines): from the
  slam until they lift each wing bone turns about its length to show its
  face, the feathers kept up (`p6_wings.py` "face_camera"), so they lie as
  two broad crescents along the floor.
- **The run** holds her legs near their mean pose (a quarter of Ultimate's
  swing, averaged over nine frames round the loop: ultimate.py "still") and
  the dress's guide averaged alike ("skirt_smooth"); the hem's shift moved
  up to 18 units a frame, now 4.
- **The dress, every clip.** The hem solver's five parameters are averaged
  over five frames (wrapped through a loop) and the band over five (it was
  three): its twitch was a frame's search landing near the last one's
  solution rather than on it.
- **Attack data.** Ultimate's ATTACK arguments (its "bkb" column the set
  knockback, "wbk" the base); the owner's file is Ultimate 1.0.0 and lists
  one hit of some moves, ultimateframedata's damage fills the rest. Jab 1
  3%; the finisher 4%; forward tilt one hit over Ultimate's two windows
  (6% then 7%, 40, growth 80, base 78); up tilt's four light hits and a 3%
  launcher (85, growth 195, base 60); down tilt 5% (75); dash attack 9% /
  5% late (60); forward smash 16% close / 13% far (361, growth 102, base
  40); up smash 16/12% early, 13/9% late (88; the owner's 1.5x knockback
  kept); down smash 15% close / 13% far; neutral air five light hits and a
  5% launcher (55, growth 180, base 30); forward air 10% (45, growth 92,
  base 31) and back air 12% (361, growth 95, base 30), single hits as
  Ultimate's; up air Ultimate's multi-hit, five light hits and a 5%
  launcher (88, growth 152, base 50); down air 11% (55: Ultimate's is not
  a spike); the counterattack 10% (361, growth 65, base 30). Multi-hits
  keep a light set knockback (10-15) where Ultimate pulls the target in
  with angles SSB64 has not (365-367). Explosive Flame, Warp, grabs and
  throws unchanged.

## Owner p6s (2026-10-15): a dress that hangs and flows

Owner p6s (3.5/5): "Skirt/hem issues, skirt behaves like a paper cone,
instead of a heavy, flowy dress" -- the hem "flat against ground" when she
kneels or crouches (up tilt, down tilt, down smash), "more flat relative to
horizontal plane" (dash, up air, run brake), narrower over a kick (forward
air) and in the run, no twitching or spinning (forward tilt, forward smash,
wait, run); "lets go with option A and B for her dress"; "Lets try High for
a 1v1"; her Ultimate attack data at the latest patch's values but for
Explosive Flame and the up smash; "Brighter colors: lets go with the A+B";
the up tilt "leaning back far, facing upwards ... like the limbo"; the down
smash's wings "more left/right symmetrical (staff-side wing goes down
further)".

- **The dress is three rings (option A's rig).** The lower band (row 25's
  ring at her hips to the hem) was one straight band: it is split at her
  knees (`look_skirt.py` split_band, MID_Y 165) by a mid ring on row 26,
  the hem on row 27 hung from row 26, so the dress bends there. Each band
  is drawn by its lower ring's row from its parent's seam load (row 27 is
  the tree's last row, drawn right after row 26); the slit's V runs through
  both (the mid ring has a vertex on each of its edges). +22 triangles high
  (467), +18 low (307); her GX cross slots are now all 15 (row 26's
  binding).
- **Row 27 was the grab's hold.** Peach's row 27 is the joint a grab holds
  the caught fighter by and a heavy item rides
  (`FTAttributes.joint_itemheavy_id` 31, hidden part 3: catch, the throws,
  heavy items). The role moved to a new row 28 hung from row 0 as row 27
  was (the scene's "joint_roles": `model_writer.py` writes the row,
  `tree_depths` takes each row's depth from the scene's parents, the
  attributes and the hidden part name joint 32); a clip that installs the
  hidden part keys it one slot later (`skirt_keys.py`).
- **The keys (option A).** `skirt_keys.py` is a cloth model baked over each
  clip: a chain a sample of the rings hangs from row 25's ring, its two
  segments pointing the way the rest dress does about the pelvis drawn
  toward straight down (HANG 0.45 at the hips, HANG_HEM 0.7 below), drawn
  0.65 of the way to Ultimate's own skirt chains in a clip made from
  Ultimate's motion (ultimate.py's guide now carries the chains' third
  joints for the mid ring as well as their ends for the hem); each point
  follows its target through a damped spring, the hem more slowly than the
  mid ring (the cloth's weight); her legs push it out from her middle
  (thigh and shin capsules of the leg mesh's radius plus 7 units, against
  the points, the chains' segments and the rings' edges: a kick lifts the
  dress over the thigh, the shin hangs from under it); the hem's points
  are drawn level with each other and, where her feet are on a floor, kept
  on it and spread out from her (kneeling, the hem lies on the ground). The
  rows are then fitted to the cloth: row 25's turn by least squares (its
  twist about the band held to 20 degrees), rows 26 and 27 translation,
  turn and scale across. The hold that turned her legs toward their rest
  where the dress could not hold them is gone (the "jumpy legs" of the down
  smash and neutral air). The rows' keys are put on shared knots within
  half a degree / 1.5 units, so the clips stay small (her largest,
  13,148 bytes).
- **The spring at run time (option B).** `nds_p4_palutena.c`
  `ndsP4PalutenaDress`, every frame before the parts' transforms: the dress
  rows swing back as she speeds up and forward as she stops or turns,
  trail at speed (0.0035 radians a unit of speed, at most 24 degrees),
  flare as she falls (scale across, at most 0.18), and settle; hitlag
  freezes it. A field the pose engine held (`NDS_FT_POSE_HOLD`) is told
  from one it rewrote by the value this wrote last.
- **High in a 1v1.** A VS match of one or two fighters draws her high
  detail (`NDSP4Overrides.duel_high_detail`, `ndsP4DuelHighDetail`); such a
  match loads her whole pack, both details, already.
- **Brighter (A+B).** Every palette of her files lifted (`model_writer.py
  --lift-palettes`: whites brighter and cleaner, greens and blues brighter
  and more saturated) and her lists' ambient light raised 0x4C to 0x6A
  (`--ambient`).
- **Up tilt** keeps Ultimate's kneeling spin with her torso turned 25
  degrees and her head 40 more toward her facing through it, her hair
  turned back by both so it falls as Ultimate's does (ultimate.py "turn").
- **Down smash wings** are both the forward wing's pose, mirrored: the one
  behind her went about 100 units into the floor (`p6_wings.py`
  "symmetric").
- **Attack data at the latest patch.** ultimateframedata's damage where it
  differs from the 1.0.0 data: dash attack 11% / 7% late, the up tilt's
  launcher 5% (4.5), down tilt 9% (8.5) to a grounded opponent and 5% to
  one in the air (ground-only and air-only hitboxes). Angles and knockback
  stay the owner's data's; Explosive Flame and the up smash unchanged.
- **Peach's clips ported ("port the rest of her animations that still use
  peach's"), first batch.** Ultimate's motions on Peach's rows' lengths and
  flags, so their scripts' frames and travel stay (a TransN row keeps
  Peach's slot): the turn and the run's turn (their pose groups gone), the
  helpless fall after Warp and its landing, the electric shock, wall and
  ceiling hits, the knockdowns, getups, getup rolls and attacks, the techs,
  the clank, item pick-up, throws, swings (sword, bat, fan and star rod
  share them), shooting and the hammer, the shield-break fall and the dizzy
  loop (a loop plays at Ultimate's own pace).
- **Ports, second batch: the ledge, the grab and throws, heavy items.**
  The ledge's statuses are windows of Ultimate's one motion each (the
  catch, the hang at Ultimate's own pace; the shared pull-up, then each
  climb's, attack's and roll's rise and its step, swing or roll onto the
  stage), with Ultimate's Trans where it is in the TransN slot (ultimate.py
  "transn": {"absolute"}): she hangs from the corner by her raised hand
  (650 units below it, Peach's 442), and the second parts' TransN steps
  carry her onto the stage as SSB64 moves a fighter there
  (`ftCommonCliffCommon2ProcPhysics`; the climb ends 167 units in, as
  Peach's). The grab, its pull and the throws keep her scripts' frames (the
  box at 6 on Ultimate's grab at 7, each release on Ultimate's release
  frame 20), the held joint (row 28) where Ultimate's Throw bone is
  (ultimate.py "held"; `keyposes.scene_skeleton` gives the recipes' tree the
  scene's added rows, `frames_to_clip` keys the hidden part's row for
  them): the caught fighter in front of her at the waist, lifted and flung.
  Heavy items too (Peach plays one throw both ways: the game turns her).
  Still Peach's: being grabbed and thrown, the guard (its shield-pose
  rows), the shield-break flight, the rebirth and the pipe.

## Owner p6t (2026-10-15): the dress's shape, the slit, her joints in game

Owner p6t (4/5): the dress's "natural" pose "seems to look more like a hoop
dress instead of palutena's slightly concave A-Line dress"; "its only very
slightly concave ... stick to a-line that you can measure"; "make the slit
smaller" ("I only wanted to move it a miniscule amount"; of four candidates
"Let's go with 12 lower for now"); "hem radius should be 10% smaller at
default rest ... While keeping a line"; concavity candidates ("Oh wow I
really like D"); "for the default resting pose the dress cone should point
more backwards to give the legs even space around them"; "we will need leg
clipping checks"; the down air "a little janky ... head/chest/arm areas
seem out of sorts"; the up tilt's backbend was right (only the dress was
wrong: its turn is gone again).

- **An A-line at rest.** The cloth's natural pose is the model's again
  (`skirt_keys.py`): Ultimate's skirt chains are measured from Ultimate's
  wait (ultimate.py "skirt_rest"), not its bind, whose dress is narrower,
  so its wait no longer flares hers; the dress hangs toward straight down
  by the pelvis's tilt only; her legs' capsules are 0.85 of the leg mesh's
  radius with no margin. Her wait's rings are the scene's to the unit.
- **The slit** keeps its V through both bands, its top 12 units lower and
  its hem opening 0.125 of the way round (it was 0.139; `look_skirt.py`
  SLIT_DROP, SLIT_HEM).
- **The rest shape (candidate D, tilted back).** The hem is 0.9 of the
  approved model's radius about its centre (HEM_SCALE: 114 to 103), the mid
  ring 9% of its radius in from the line from the hips to the hem (CONCAVE;
  the candidates were 0, 3, 6 and 9%), and the hem moves back and toward
  her left (HEM_SHIFT, the band tilting about row 25's ring): her wait
  stands with one leg well behind the other, and the hem centred on the
  model's bind legs met that leg (0 units of room behind it, 92 in front).
  Now 40 in front and 40 behind at the hem, 21 and 20 at her knees
  (measured on the wait's cloth), at least 31 through the idle loop.
- **The joints the game locked.** SSB64 makes a fighter with the joints
  its `FTAttributes.animlock` names locked at rest (`ftParamSetAnimLocks`)
  and frees them only in a motion whose row sets anim_flags bit 0. Peach's
  are rows 3, 10, 15, 18, 20 and 23 -- her shoulders, her hips'
  orientation and her ankles -- which her N64 clips never key; no row of
  Palutena's set the bit, so every Ultimate motion played with those at
  rest in the game (her arms and legs off the renders: the down air's arms,
  legs through the dress the cloth was solved around). Her own clips now
  key them all (`anim.py` `key_rest_rows`), her rows with her clips set bit
  0 (spec `"anim_locks": "rest"`), and her code frees them there and holds
  them at rest in Peach's clips (`NDSP4Overrides.anim_locks_rest`,
  `ndsP4AnimLocksRest`). 144 of her 184 clips moved them more than 15
  degrees.
- **Her legs held** (owner p6t: "we will need leg clipping checks"). The
  check (`dress_check.py`, every clip as built): before, 12 of her 133
  clips kept her thighs and knees inside, out on 2,677 of 4,843 frames
  (walking 26 units out, the back leg through the back of the dress
  mid-stride; jumps and the squat 40-60; flips and tumbles up to 178). The
  cloth itself held the walk and the run; the rings' rigid fit lost them,
  and the cloth alone lost a kick or a tuck. Now, after each frame's fit,
  `skirt_keys.py` holds her legs: row 25 turns and rows 26 and 27 move,
  turn and widen as little as puts every point of her thighs, knees and
  shins 3 units inside (a shin may show below the hem, never through the
  dress's side), drawn toward the fit and the frame before, so the dress
  eases from hold to hold; a knee raised far out (kneeling, thrown) is
  tried again from row 25 turned toward it; gravity turns the dress at
  most 90 degrees of her tilt, so a flip leaves it on her legs. The check
  measures as it holds (`dress_outs`): a point near the mid ring on both
  bands, one at her hips on the upper band or the lower band's top,
  whichever holds it. As built: her thighs and knees out on 24 of
  4,843 frames (2,677 before), her shins through the dress's side on 7
  (1,053); 113 of the 133 clips clean, the rest out on one to three frames
  (the back tech, the forward air, getup rolls, the back throw); the hem
  moves less frame to frame than the cloth alone gave it (the walk's worst
  18 units a frame, 20 before; the forward air's 126, 278); her largest
  clip 14,048 bytes (14,144).

## Owner p6v (2026-10-16): the entry, the smashes' sounds, items, down air

Owner p6v (4.8/5): the entry's door "should be behind fighter plane so
palutena can walk through it and stop on the fighter plane ... currently she
walks left along the fighter plane. Door should 100% oscure Palutena until
doors open"; the down smash's "wing slam animaiton is not syncronized
correctly"; the smashes' "Wing SFX and enemy hit SFX should play together
if there is a hit"; items "should revert back to peaches" (the beam sword,
fan, ray gun and bat swing and scale wrongly); the down air "uses wrong
SFX", its torso "twists too much out of alignment with hips at full
impact"; neutral air "plays several of the same SFX"; Warp's delay "can be
longer (1.25x longer)".

- **The entry.** Ultimate's entry walk runs along the model's -x (its
  fighters enter facing their side); SSB64's Appear turns her to the camera
  (lr 0), so she walked left along the stage behind a gate standing 260
  units in front of her. The clip is turned 90 degrees (ultimate.py
  "yaw"): she walks 583 units out of the background toward the camera and
  stops on the stage's plane. The gate stands 380 units behind the plane
  (`p6_gate.py` FRONT), its closed doors hiding her (she starts 200 behind
  them); they part toward the camera from tic 10, wide enough for her by
  22, and she reaches them at about 31; the gate shrinks into its doorstep
  from 84. Her sparkles are at the doors; her shadow, drawn under her entry
  position, shows from 96 as she nears it.
- **Down smash's wings** were an old table: her special file 2 (gate,
  pillar, wings; `p6_gate.py`) is built by hand, not by make, and was last
  built before p6r retimed the down smash. Rebuilt.
- **Sounds.** The wings' sound (both smashes) and the down air's burst were
  stored sounds, which SSB64 stops when a hit lands, so a hit played only
  its own. They play to their end now, started so their loud part meets the
  hit (the wings' rises over five frames: from 9 for the forward smash's hit
  at 14, from 11 for the down smash's at 16; the down air's from 4 for 7).
  The down air's hit sound is a kick's (it was a punch's). Neutral air
  started its one-second whirl at each of its five light hits; it plays
  once, from frame 1.
- **Items.** Item pickups, throws, swings, shooting and the hammer (motions
  88-133) are Peach's clips again; her dress is keyed on them as on every
  clip she plays.
- **Down air.** At the kick her torso turned about 160 degrees from her hips
  (Ultimate spreads that twist over its waist bone; her tree has none).
  From 6 to 9 her hips turn 30 degrees toward the camera and her torso 55
  toward her hips, each about its own up so its lean is kept (ultimate.py
  "turn" with "axis": "up"), held to 24 and eased out by 30: the twist
  about 105 degrees.
- **Warp** waits 1.25x as long before she vanishes (the start's clip, 12
  frames to 15: anim.py "retime").
- **Her legs in the dress** (`dress_check.py` as built): her 114 own clips'
  thighs and knees out on 21 of 4,265 frames, her shins through the side on
  7; the entry clean; Peach's item clips at most 4.8 units out, on 2 frames.

## Animations (the owner's 3DS references, 2026-10-14)

Superseded by Ultimate's motions above (the recipes below were keyposes;
kept as the record of p6i/p6j).

The owner's 3DS clips (local, `decomp/alt_assets/Fighters/Super Smash Bros
3DS/Palutena/`; Explosive Flame from Ultimate) are the reference for her
moves; "I author, you review": each move is key poses (`scripts/p6/keyposes.py`,
anim.py "keyposes" recipes in `art/p6/palutena/anims/`) on Peach's tree with
her own bone lengths (recipe "scene"). Hurt animations stay Peach's.

Owner playtest p6i (2026-10-14): "ALL the animations look very stiff with no
momentum or force ... follow them as close as possible." Since then:

- **Motion.** The recipes move through their keys on a spline (keyposes
  "interp": "spline": Catmull-Rom tangents, stopping only at the ends and at
  held keys), not eased to a stop at every key; her torso, hips and head are
  keyed as world orientations ("orient": yaw, then the lean in her own
  facing) so a lunge leans into its facing.
- **The dress flows.** Her skirt's axis follows her legs through a damped
  spring (`skirt_keys.py`: it lags her turns and swings and settles); her
  run trails it behind her (spec "pose" op "skirt").
- **Owner p6j: the half-speed clips, every move redone.** The owner
  re-recorded the references at 2560x1440 and half speed (one video frame a
  game tic): "please look at all of them and redo all of the animations"
  (Explosive Flame excepted: a better reference is coming; Warp stays as
  approved). Each clip was measured frame by frame at true scale (her height
  matched to ours, her feet on our floor, a model-unit grid over the frame;
  Side_A faces left and was read mirrored): the staff's gem, the shield,
  her head, hips and feet. The keys put those where the clip has them,
  solved to arm and leg IK on her built clip's real geometry (a scratch
  solver iterating each target on the measured error). Keyposes gained what
  that needs: an IK target as a model position (a hand where the clip holds
  it), "aims_after" (the staff aimed after the arm is bent; an aim "to"
  "keep" turns a row about its own length: the shield's face), "hinge" (an
  elbow's or knee's hinge kept across the chain's plane, so two keys don't
  flip a forearm between them), and a base reference's "blend" (her fall's
  and crouch's legs narrowed as her pose group narrows them, so an aerial
  or a down tilt meets the game's fall and crouch without blending its own
  kick). Her hit frames stay SSB64's (owner p6i): each clip is mapped onto
  them piecewise (wind-up, the hit, the hold, the recovery).
- **Jab.** Jab 1 the shield bash, her shoulders turned away and the staff
  swung up behind her; jab 2 the staff thrust level ahead at her waist with a
  lunge (her slit's leg ahead, so the dress holds the other), the shield back; her own rapid jab (Fox's rows and routines,
  appended statuses 0x102-0x104 on motions 228-230; four A presses counted
  in jab 2): the staff held level ahead, pushed out with each of four bursts
  a loop (`Goto -> self` scripts, p6_author.py), and a finisher thrust with
  the staff swung up and over her head when A stops.
- **Tilts.** Forward (Side_A): a twirl on the spot sweeping the staff low
  ahead (the first hit), then the staff let go to spin upright before her
  open palm (one and a half turns over the later hits; keyposes "spin"),
  caught upright and held out. Up: dropped to her knees, the arm straight
  up, the staff spun overhead three and a half turns in a plane low ahead
  and high behind, standing up leaning back as it stops. Down: from her
  crouch a low lunge, her slit's leg ahead, the staff thrust out along the
  floor at full stretch (her hips kept up: a kneeling lunge's shin lay out
  behind the dress, `dress_check.py`).
- **Dash attack.** From her run the staff swings under her into a wide
  stance behind the shield, the bash a deep lunge with the shield thrust
  far out (its own TransN curve, keyposes "transn" "travel": about 760
  units), sliding upright behind it.
- **Smashes and wings.** Owner p6i follow-up ("they should articulate and
  stretch and be mostly symmetrical ... polished and like they belong in the
  DS version"): the wings are a rigged effect in her special file 2
  (`scripts/p6/p6_wings.py`, written by `p6_gate.py`; drawn natively,
  `p4_articles.py` "wings"). The kit's godwing gives the rig, a wing a side
  of four bones (LwingNa-d; the right wing the left's mirror), and the art,
  rendered flat in the wing's plane into one 128x32 CI4 image with a light
  rim on the feathers. Owner p6j: "your wings look disconnected in parts":
  each wing is now one continuous mesh (a grid of three or four columns a
  bone, the corners at a seam on the bones either side of it: the entry
  effect's cross-matrix corners), so it bends without gaps. Her code
  (`nds_p4_palutena.c`) makes the effect on the smash's first tic, attached
  to her (the next status removes it, hitlag holds it), and poses it every
  tic after her map proc at her animation frame from a table in the file:
  the root on her TopN, a node on her clip's torso row, the pair node (its
  turn and size: the wings grow and shrink), each bone's turn and stretch.
  The motion is keyed in `art/p6/palutena/wings/fsmash.json` and
  `dsmash.json` against the half-speed clips at true scale ("level" turns
  the pair from the torso's twist to the battle camera's plane, "pair_yaw" a
  quarter turn lays the down smash's wings across the screen, "twist" turns
  a wing about its length). Forward smash (Strong_Side_A): the shield out at
  her shoulder as she rises on her toes, the wings sprouting behind her
  shoulders (the sparkle at 4), a hop with the wings raised in a big V (11),
  swung up ahead (13) and laid out long and low ahead to the floor at the
  hit (frame 14, SSB64's speed; about 850 units, the 3DS's 1.7 her height)
  as she bows with the staff upright ahead, held to 31, curling up into a V
  behind her and gone at 44; the staff swung down across her and round her
  hip as she rises. Down smash (Strong_Down_A): turned to the camera, the
  staff over her head in a ring of wings, flung open into two thin crescents
  (9), the hit a deep squat (her knees forward, not the 3DS's straddle: the
  dress holds them) with the staff's head slammed to the floor as the
  wings lie flat along it both sides (10; about 650 each side, kept above
  the floor), held to 33, rising into an arch, gull wings and a wide shallow
  V, gone at 49. Their hits are on the wings' inner and outer reach (TopN)
  and, forward, the staff. Up smash: turned to the camera gathering the
  light with the staff low at her right, thrust straight up overhead as the
  pillar rises and held while it stands. Owner p6j follow-up: the thrust is
  at frame 12, the script's hit and flag0 (AsyncWait waits to an animation
  frame; the clip had it at 23, so the pillar rose 11 frames before her
  staff did), the clip's frames 11-23 on hers 0-12 at the 3DS's own pace.
- **Aerials.** Neutral: the staff whirled once round before her in the
  camera's plane. Forward: a flying kick (the half-speed clip shows the kick
  the card named): her left leg out through the slit with a burst at the
  foot, leaning back with the staff raised behind her head; its hits on the
  leg. Back: her back turned, the shield thrust out behind her in a burst of
  light. Up: turned to the camera, the staff upright before her in the
  light, the halo above. Down: a stomp, turned to the camera with arms
  spread, her left leg driven straight down (a burst at the foot; a spike);
  its hits on the leg.
- **Locomotion.** Her run floats (the 3DS's glide: leaning far forward, the
  staff carried slanted with its gem low behind her, the shield arm out
  ahead, a slow flutter) with a push into it (Dash: the staff swung back) and
  a brake (the staff stood gem down, then swung up behind her); her walks
  keep Peach's stride under the dress with her upper body upright, the
  staff carried as the Walking clip's (its camera is zoomed in and follows
  her: read for the pose): in her near hand at her waist, slanted, its gem
  leading up ahead at her head's height and its tail trailing low behind,
  swinging a little with her stride.
- **Specials.** Explosive Flame: a quarter turn into a wide stance, the staff
  arm straight at the target (owner p6i: no kneel; the Ultimate clip), her
  halo shown. Warp stays as the owner approved it ("Palutena's teleport is
  good": the staff raised overhead). Down taunt (Taunt): the staff stood on
  the floor behind her and let go (keyposes "pin"), her hand brushing her
  hair back while the halo glows, her arm out back with an open palm, the
  staff retaken.

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
stick at the cast picks the distance, and at his frame 12 her fire's weapon
starts at that distance and his height (round 3, as hers), its blast with
her explosion sound (`NDS_P4_KIRBY_PALUTENA_FGM_BLAST`, from `audio.json`).
Ground and air switch at the same frame. The hat is one list in
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
