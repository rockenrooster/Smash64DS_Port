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

Portrait (owner playtest 2026-10-14, p6d: too small and dark, Roy's the
reference; the owner then supplied pixel portraits of both P6 fighters):
`scripts/p6/portrait_art.py` trims the art's frame and the black gap inside
it and keys its dark red backdrop out (the dark pixels that are not his
hair's blue, connected to the edge), and `p6_present.py` frames it over the
select's fire plate (the source's PortraitFireBg, which Remix's portraits
carry) with an area-average downsample (a Lanczos one rang into coloured
specks at the hair's edges). The art and the picture are local
(`builds/p6-art/ike/`).

## Move card (approved 2026-10-13, I1)

Damage and timing numbers are lab defaults on SSB64's scale, tuned against
Roy; Brawl gives the motion and the intent only.

| Move | Rows | Clip | Script | Code |
|---|---|---|---|---|
| Jab 1 | Attack11 | new: left jab | ~3% | — |
| Jab 2 | Attack12 | new: kick | ~3% | — |
| Jab 3 | Attack13; jab_3 rows on (Captain's timer) | new: overhead slash, a downward swing (owner 2026-10-14) | ~6%, launches | jab tables (S3) |
| Forward tilt | AttackS3 | Roy's (MARTH_F_TILT) at 0.85 speed | Roy's damage x1.25; frame-speed command | — |
| Dash attack | AttackDash | new: heavy upward swing | Roy's damage x1.2, launches at 30 degrees (p6d) | — |
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
- Jab 3 (owner playtest 2026-10-14: a downward swing): Marth's forward
  smash spliced (frames 10-30: overhead at 14, down by 16; its TransN travel
  dropped, 0.8x), the sword and forearm hitting frames 4-7 (6%, 45 degrees);
  first built on Marth's second jab, a rising slash, as an appended motion (224) and
  status (249, Captain's Attack13 routines) that `jab_3_action` names: Roy's
  status table has his entry where Captain's Attack13 is (motion 195, status
  220). Captain's jab chain timers (`attack1_followup_frames` 22 in Ike's main
  file, `jab_3_timer` Captain's 24).
- Dash attack: Marth's dash attack (a lunge with a rising swing) at 1.15
  times its length; Roy's three hitboxes at 1.2 times his damage, sent away
  at 30 degrees (owner playtest 2026-10-14, p6d: more horizontal; 80 before).
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
  Owner playtest 2026-10-14 (the animation needed work): the rise now holds
  the sword straight up to the top (Blazer frames 0-14, held 16); the plunge
  starts with Aether's spin, a forward flip about his hips over nine frames
  (`anim.py` recipe `spin`) while the sword comes from overhead down (Roy's
  down-air frames 2-8 at 1.5x), then holds the sword-down plunge.
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
  forced charge on port 0 plays 0x8C, 0 native failures. Owner playtest
  2026-10-14 (p6d): the charge sound (Ultimate's four-second
  se_ike_special_N01, on Roy's start sound 0x5B9 and his charge loop 0x5C0)
  ran on after a cancel, a hit or an early release: the start played it as
  a one-shot. Ike's start motions (214, 218: `motions/eruption_begin.ms`)
  play it as the fighter's loop sound, which a new status stops, and the
  charge's wait keeps it (`nds_p4_roy.c`), so the wait's own loop sound
  finds it playing and starts no second one.

Defaults for the new specials:

- **Quick Draw:** hold B to charge for up to 75 frames (60 in the card; owner
  playtest 2026-10-14, p6d: 1.25x as long to the farthest dash); on release
  he dashes for 8-20 frames (by charge) at three times Roy's run speed with a hitbox; a
  hit cuts to the slash; a miss ends in about 40 frames of lag. From the air,
  or off an edge, he ends in special fall. Owner playtest 2026-10-14 (the
  charge had no effect or sound; done): the charge glows Ike's blue (Roy's
  charge colour animation through Ike's tinted copy) with a twinkle on the
  blade every ten frames and the charge's sounds (se_ike special_S01, then
  S02); the dash keeps the glow, with the large dash dust and the dash's
  whoosh (S03); the slash on contact rings (S04).
- **Aether:** Kirby's Final Cutter state machine (rise, apex, descent,
  landing; `ftkirbyspecialhi.c` constants as the starting point) with Ike's
  statuses and clips; the source's landing weapon (`wpKirbyCutter`) is not
  made. Ledge catch during the descent as Final Cutter allows. One call per
  Aether, at the rise (owner playtest 2026-10-14: the landing's second call,
  special_H02, sounded like a second voice; removed). Owner playtest p6d
  (open): Aether as Ultimate's -- he throws Ragnell up, it stops at its high
  point and spins, he jumps to it, spins, and slices down with the blade
  held level; the owner is sending an Ultimate clip of it.
- **Animations from the owner's 3DS clips (2026-10-14).** The owner's
  reference videos (local, `decomp/alt_assets/.../Super Smash Bros 3DS/Ike/`,
  7 clips) against our moves: the jab combo (left punch, kick, overhead
  downward slash), the dash attack (lunge into a rising slash) and the
  forward smash (overhead slam) already follow them and stay. New, as key
  poses (`scripts/p6/keyposes.py`, anim.py "keyposes" recipes): the up tilt
  (a crouch, a hop with Ragnell level overhead, a low sweep; its own script,
  Roy's hitboxes on the rise and the sweep), Eruption on Roy's Flare Blade
  rows (the charge stance with the blade up at the chest, the release
  plunging it into the ground at Roy's hit frame) and Aether (the owner's
  description: Ragnell thrown up to hang spinning where he meets it -- its
  height above him follows his rise -- the catch overhead, a forward flip,
  the descent with the blade level, an impact crouch). Each is authored
  0.75x the reference's length, so his 0.75 animation rate plays it at the
  reference's speed. Lab: up tilt and Aether play through, 0 native
  failures.
- **Speed and power (owner p6f, p6h):** "more slow and lumbering compared to
  Roy, but much more powerful": his hitboxes deal 1.5x Roy's damage
  (generate_p4_fighter.py OWNER_DAMAGE_SCALES) and his attacks play at 0.75x
  (OWNER_ANIM_RATES with OWNER_ANIM_RATE_STATUSES, p6h: "only the attack
  animations (minus the dash and down b charge)"): the jabs, tilts, smashes,
  aerials, Quick Draw, Aether, the counter and Eruption's release. The dash
  attack, Eruption's charge (its begin and wait) and everything else --
  movement, landings, getup and ledge attacks, throws -- play at 1.0x, and
  his ground speeds are Roy's again (0.75x with the p6f rate). Eruption's
  charge clips were authored for the 0.75 rate, so they now play 1.33x the
  reference's speed.
- **Owner playtest p6i (2026-10-14):** Eruption plays Roy's Flare Blade clips
  again (the owner: "I would rather revert back to Roy's"; its charge sound
  stays the loop that stops with the move). His jab combo steps forward, the
  third strike most (about 25, 40 and 150 units: his code moves him over each
  jab's step, `NDSP4Overrides.before_physics_map`; a held ground speed did not,
  the floor's friction ate it in a tic). Aether: a tucked forward flip at the
  top (knees to his chest, the blade drawn in, thrown open into the chop),
  the descent still moving, an impact with a recoil; it and the up tilt move
  on a spline (keyposes "interp").
- **Owner p6j (2026-10-14): the half-speed clips.** The owner re-recorded
  his seven references at 2560x1440 and half speed (one video frame a game
  tic; Neutral_A_Combo, Running_Side_A, Strong_Side_A, Strong_Up_A, Up_A,
  Neutral_B_Charge_Dash, UP_B) and asked for his moves redone from them, as
  Palutena's were. Each clip was measured at true scale (his height matched
  to ours, a model-unit grid over the frame; Running_Side_A faces left and
  was read mirrored, the travelling ones with the floor held and his own x
  followed) -- Ragnell's grip and direction, his left hand, head, hips and
  feet -- and every key puts them where the clip has them, solved to arm and
  leg IK on his built clip (keyposes absolute IK targets, aims after IK,
  hinges). His frame data stays: each clip keeps its old length and its
  script's hit frames, the clip mapped onto them piecewise. Roy's stance is
  kept (his wait: his back half to the camera), so the poses read as the
  clip's silhouette. Now his own clips, as key poses:
  - Jabs: jab 1 a left punch with a lunge (his chest turned so the left
    shoulder leads); jab 2 a kick with his back leg, Ragnell swung back; jab
    3 Ragnell over the top into a deep lunge along the floor. His code's
    steps stay (about 25, 40 and 150 units).
  - Dash attack: Ragnell drawn up behind his head, a low lunge with it
    trailing behind, swept round his far side into a kneeling thrust ahead
    (the hit), tipped upright, rising; the old lunge travel (about 1,010).
  - Up tilt: a crouch with the blade low ahead and the left fist up, rising
    with it level at his waist, a hop with it level overhead, landing with it
    swept low ahead.
  - Forward smash (his own clip now; Roy's script): both hands lift Ragnell
    overhead, it hangs down his back and is cocked level behind him as he
    leans back, swung over the top as he steps in, slammed down into a deep
    lunge with the blade along the floor ahead (the hit, frame 14), held,
    rising. Owner p6j: "for Ike add old range back": the clip slides him in
    (keyposes "transn" "travel": about 540 units over frames 9-14, drawn
    back as he rises), so the tip lands about 1,260 units from where he
    stood, as Roy's clip reached (its own 730 alone).
  - Up smash: Ragnell drawn up behind him in a low crouch, swung down
    before him and up in an arc overhead (the hits), over and down behind
    into a wide crouch.
  - Quick Draw: the charge a deep crouch with Ragnell upright before him;
    the dash a long low lunge with it thrust out ahead; the slash (its own
    clip now) and the miss the same follow-through, Ragnell swept up and
    back over his head and brought down low ahead (the 3DS slashes at the
    dash's end either way).
  - Aether: the toss, the climb and the flip stay (they follow the clip);
    the plunge is now tucked with Ragnell level ahead at his waist, the
    landing a deep crouch with the blade along the floor ahead.
- **Counter:** Marth's window and return rule. SSB64's Appeal is ground-only,
  so the counter is too.

## Animations from Ultimate's motions (owner p6p, 2026-10-15)

Owner p6p review of his clips (the stance first: "default stance wrong",
the left foot forward facing right; jab 1 a left punch, jab 2 a forward
kick with the right foot, the up smash one strong overhead swing, Quick
Draw's charge skating back and its slash to his right, Aether "throws sword
up forcefully, pause, jumps to sword, pause for spin, comes back down"),
with the owner's extraction of Ultimate's motions beside Palutena's
(`../ssbu-recomp/build/ike_motion`, local). His clips are Ultimate's own
motions retargeted as hers are (`scripts/p6/ultimate.py`, anim.py
"ultimate" recipes in `art/p6/ike/anims`; nothing of Ultimate committed,
P6 D1), on Roy's tree with his look:

- **His skeleton** is the game's own `model.nusktb` (there is no DAE rip of
  him), extracted from the owner's data.arc beside the motions
  (`ike_motion/fighter/ike/model/body/c00/model.nusktb`, local).
- **Roy's bind is not a T-pose** (his arms hang, his legs stand apart):
  "align" turns each limb row's bind onto Ultimate's bind direction before
  the turns are carried over (W_row = W_bone B_bone^-1 A_row B_row).
- **Ragnell** follows Ultimate's Sword bone (the blade along its +y, its
  face x; ours row 14's z and y), and where a motion has its own sword
  motion (Aether's throw) the sword model's HaveR rides on the Sword bone,
  so the blade leaves his hand as Ultimate's does.
- **His cape** (Roy's rows 15-18) turns with Ultimate's centre mantle chain
  (S_MantleC1-4).
- **Timing.** His hit frames stay the card's (and Roy's for the forward
  smash, on its old slide: TransN 540 into the slam); the dash attack's
  travel is Ultimate's scaled to the old 1,012 units. Clips the game times
  by length keep Roy's lengths (walks 50/35/30, landings 11, crouch 8);
  JumpB and JumpAerialB keep their XRotN slot (left still).
- **His figatree heap** stays Roy's 11,728 bytes: the forward smash and dash
  attack key every other frame ("key_step": 2), the run one 34-frame period.
- Clips: jabs c00attack11/12/13, dash attack c00attackdash, up tilt
  c02attackhi3, forward smash c03attacks4s, up smash c04attackhi4, Quick
  Draw d01specialsstart/dash/attack/end, Aether d02specialhi1-4, Counter
  d03speciallw and its hit, and his wait, walks, dash, run, brake, jump
  squat, jumps, double jumps, falls, crouch and landings. Eruption stays
  Roy's (owner p6i), as do his other attacks, his entry and his hit
  reactions.

## Ultimate's timing and attack data (owner p6r, 2026-10-15)

Owner p6r (3/5, "almost there"): "use ultimate timing for Ike since Ike is
meant to be slower and more lumbering"; damage, hit type and angles from
Ultimate's attack data (the owner's `ike_attack_data.json`, extracted from
the game's 1.0.0 module beside the motions, local); "revert running
animation (running speed also too fast) ... remove footstep SFX"; "Up
Special looks janky and corrupted"; the forward smash stops short facing
left and lunges too far, the sword should end flat on the floor; Quick
Draw "hardly knocks the enemy back".

- **Timing.** Every attack plays Ultimate's motion frame for frame (our
  frame N is its frame N; ultimateframedata's frame N is our N - 1) to its
  first actionable frame, the motion's rest drawn into the last six frames;
  the scripts' hits are on Ultimate's frames. His 0.75 animation rate and
  1.5x damage (owner p6f/p6h) are gone from his attacks
  (`generate_p4_fighter.py`): the rate stays on Roy's run and Eruption's
  ends only.
- **All his attacks are Ultimate's now:** jabs, dash attack, the three
  tilts, the three smashes, the five aerials and their landings (Ultimate's
  landing lag: 8, 14, 11, 9, 14 frames), Quick Draw, Aether and the
  counter. Eruption, getup and ledge attacks, grabs and throws stay Roy's
  (at Roy's damage now).
- **Attack data.** The owner's JSON is Ultimate's ATTACK arguments: its
  "bkb" column is the set knockback (SSB64's knockback_weight) and its
  "wbk" the base knockback; damage rounds halves up. Where the JSON lists
  one hit of a move with two (the early up tilt and up smash, the forward
  smash's strong hit, the dash attack), the other's damage is
  ultimateframedata's with the listed hit's knockback. Quick Draw's slash:
  6% and up to 7 more by charge, 60, growth 88, base 70.
- **Hit placement.** Ultimate collides in two dimensions, SSB64 in three:
  his swings pass toward and away from the camera, so his hitboxes are put
  in his plane (`NDSP4Overrides.planar_hits`, battleship_ftmain.c). Jab 2's
  hits are on the kicking right leg (they were on his standing shin); the
  forward tilt's are ahead of him at TopN offsets as Ultimate's (the blade
  sweeps round his side at its hit frames).
- **The forward smash** plays in place with Ultimate's Trans on the body
  (his step in and back), the blade flat on the floor from the slam to
  about frame 60; the old 540-unit TransN slide is gone (it ran him into
  ledges, which is the likely "stops too soon facing left").
- **Aether** was corrupted by Ultimate's plunge loop (d02specialhi3): its
  first and last frames carry the hips 57 units below, so the clip passed
  through them and he vanished; the plunge now loops its frames 1-4.
  Ragnell no longer follows the sword model's motion (it wandered up to
  2,000 units off): "item_path" (ultimate.py) throws it up at 14, hangs it
  in the world, spinning, and puts it back in his hand when it meets it at
  46. The rise is Ultimate's: 40 frames on the ground (the throw and the
  pause), the leap on Ultimate's Trans curve (eight frames, 0.85 of its
  height: nds_p4_ike.c `sNdsP4IkeAetherRise`, the recipe's "rise" the same
  table), then he hangs for the spin until the plunge. Armour 4-34, the
  throw's hit at 14 (6%, set knockback 140 at 96), three 1% hits from 43,
  the falling hit from 50 (3%), the landing's 6%.
- **The run** is Roy's again with its dust but no step sound
  (`motions/run.ms`); his dash and run speeds 0.75 (OWNER_ATTRIBUTE_SCALES)
  and those statuses' clips at 0.75 with them; Quick Draw's dash four times
  his run speed so its reach stays.
- **Figatree heap** stays under Roy's 11,728 bytes: the dash attack, up and
  down smash, neutral air and Aether key every other frame, the forward
  smash every third.

## Faster swings, a longer Ragnell (owner p6s, 2026-10-15)

Owner p6s (4/5): "the slow wind ups are fine, but the actual swings should
be much faster and forceful"; "Up Special needs trails throughout especially
when Ike comes down with his sword parallel to the ground"; "Make Ike's
sword 1.25x longer"; the forward smash should "slam the sword 100% flat
against the ground, maybe go slightly past the ground a couple of degrees";
"Run start animation too long".

- **Swings.** Each swing (the blade's tip fast, from frame W to E of
  Ultimate's motion, about its hit H) plays twice as fast about its hit: the
  wind-up runs to H - (H - W) / 2, the follow-through ends at
  H + (E - H) / 2, the hit pose stays on the hit frame (the scripts' hits do
  not move), and the recovery takes the frames saved. The swing is keyed
  every frame (`key_dense`, ultimate.py) so the clip's every-other-frame keys
  do not smear the arc. The forward smash goes from overhead (30) to the
  slam (32) in two frames.
- **The forward smash's slam** pitches the blade 5 degrees past flat, its
  tip into the floor, from 33 to 55, back to Ultimate's by 62 (`item_pitch`:
  the item's row turned about the axis across the blade and the vertical).
- **Ragnell 1.25x.** The sword's vertices scaled 1.25 along the blade from
  the grip (`look_r.py scale-joint`, joint 14; written as 0x1A00/0x1A03 as
  before). Its hits moved out with it: the blade's tip and middle at 525 and
  250 (from 420 and 200), Aether's at 475 and 150, Quick Draw's slash at
  250; the sword's effects at 220 and 440, the forward smash's impact at
  760, the trail's reach (`sword_trails`) 87.5 to 462.5 (Aether's 62.5 to
  475).
- **Aether's trail** is on from the throw (frame 6) through the plunge and
  the landing.
- **The run start** (Dash) plays at full rate: the 0.75 rate stays on the
  run and Eruption's ends only (`OWNER_ANIM_RATE_STATUSES`).
- **Figatree heap:** the down smash keys every frame only through its two
  swings (11,508 bytes, under Roy's 11,728).
- **"Playing diagonally"** (open): measured on the clips, his swings already
  run most in his plane at Ultimate's facing -- over all his swings the
  tip's travel is 28% in depth; his body turned 30 degrees toward his facing
  makes it 36%, 45 degrees 39%. What faces the camera is his body: his chest
  turns 98 degrees from his facing in Ultimate's own motions (Palutena's 54).
  A comparison of the turn is with the owner; nothing changes until a pick.

## In the game as in the renders (owner p6t, 2026-10-15)

Owner p6t (4.5/5): "some yaw issues in game that don't show in your
renders" -- Aether's sword "off upon coming down", the forward smash; the
up throw "should hit enemy in front of ike (uppercut into throw), instead
it comes up from behind".

- **The joints the game locked.** SSB64 makes a fighter with the joints its
  `FTAttributes.animlock` names locked (`ftParamSetAnimLocks`: the local
  matrix computed once, at rest) and frees them only in a motion whose row
  sets anim_flags bit 0; Roy's are rows 3, 10, 15, 21, 24, 26 and 29 (his
  shoulders, hips' orientation, toes), which his N64 clips never key. No
  row of Ike's set the bit, so every Ultimate motion played with those
  joints at rest: his right shoulder 70 degrees off Aether's fall, his
  sword arm and Ragnell with it (a probe of the joints' DObj values matched
  the clip; their cached matrices held the rest pose). His own clips now
  key them all (`anim.py` `key_rest_rows`, at rest where Ultimate leaves
  them), his rows with his clips set bit 0 (spec `"anim_locks": "rest"`,
  `p6_author.py`), and his code frees the joints in those rows and holds
  them at rest in Roy's (`NDSP4Overrides.anim_locks_rest`,
  `ndsP4AnimLocksRest`), never taking the lock's scale-compensating
  matrices. 49 of his 169 clips moved those joints more than 15 degrees.
- **Aether's throw is an uppercut.** Ragnell stays in his hand from behind
  him, under and up in front (its hit, 14-17) and is thrown from there at
  17 (`item_path` release 17-18, from 14-16): thrown at 14 it left from
  behind him, as Ultimate's hand still was.

## Owner p6v (2026-10-16): trails, the up smash, Eruption's blast

Owner p6v (4.9/5): "ragnell sword trails should be more gold/yellow
instead of red, and should match ragnels 1.25x length"; "run start and run
stop SFX should be reverted to exactly Roy's SFX"; "Up smash has yaw issues
and I can't seem to see the overhead swipe in the animation in game";
Aether's thrown sword "a little more in front of Ike (to match the downward
long swipe more)"; the dash attack should "lunge forward about 1.25x
farther during upwards swing"; Eruption "fully charged needs a 2x bigger
hitbox and exp"; Quick Draw's "minimum dash distance should be adjusted to
be 0.75x. max unchanged".

- **Trails.** His swing trail runs gold (255, 200, 0) at the guard to a
  pale yellow tip (it was red-orange to white), along Ragnell from its
  guard to its tip, 100 to 600 (the blade is 100-601 since p6s; the trail
  stopped at 462.5); Eruption's blue trail takes the same reach.
- **Run start and stop** play Roy's own sounds again (Dash 0x75 and
  RunBrake 0x80 left out of the cue map; Ultimate's dash_start and
  dash_stop stay on his jumps).
- **Up smash.** The blade's arc in his plane -- Ultimate's 24.5-28: ahead,
  overhead, behind -- took 2.5 frames, between two shown ones (the game
  shows every other frame), so the overhead never showed; before it the
  blade comes round his side in depth (Ultimate's own path). The swing now
  runs 24-30 (time map), shown ahead low, up ahead, up behind and behind,
  keyed every frame; its hits stay at 24-28 and 29-30.
- **Aether.** The thrown sword hangs 200 units ahead of where his hand meets
  it (ultimate.py `item_path` "ahead"; it hung 280 units behind his hips).
- **Dash attack.** Its travel through the rising swing (frames 10-20) is
  1.25x as long, 611 units to 763 (ultimate.py `transn` "boost"); the whole
  lunge 1,012 to 1,165.
- **Eruption at full charge.** Its blast hitbox is twice the size (912 to
  1,824: spec `script_edits` "size_scale" on Roy's strong-end rows 217 and
  221) and its explosion twice the size: the script's (effect 29) is left
  out ("drop_effects") and `nds_p4_roy.c` makes it where the script did,
  at scale 2, as the blast opens.
- **Quick Draw** dashes 6 to 20 frames by charge (8 to 20: the shortest
  0.75x as long, the longest kept).
- **Owner p6w (4.99/5):** the trail "white/gold not fully yellow/gold": gold
  at the guard to a white tip (it had been red to white, then gold to pale
  yellow).

## Audio (local inputs, never committed)

Both sets are in `decomp/alt_assets/Fighters/Super Smash Bros Ultimate/Fighters/`
(owner supplied, ignored). The build resamples them to the pack's rates and
encodes IMA; they play through the cue cache like every content's sounds.
The clips used come to well under 1 MB of NitroFS.

- **Voice (D5): the Radiant Dawn set, `vc_ike_c01`** (Greg Chun in the older
  Ike's tone; 36 clips named by move, 48 kHz, about 28 s), played 1.5
  semitones higher (owner playtest 2026-10-14: "a tad higher but still below
  Roy's"; Roy's voice measures about 400 Hz, these about 327, 3.5 semitones
  apart; `pitch_semitones` in `scripts/p6/ike/audio.json`). attack01-07 for the
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

Hat (2026-10-14): Ike's own look's blue hair and olive headband -- his head
row's (joint 9) textured spans 3-5, the face's three before them -- cut from
his written model and drawn at Kirby's joint 6 over the plain head as the
custom powers' hats are (`p4_kirby.py` `AUTHORED_HATS`, 2.6x, 40 units down;
`scripts/p6/hat_preview.py` renders it on Kirby). Roy's power rows and Roy's
sword (the weapon keyed by Roy's hat id) stay.

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
