# R2-07 BUGS cluster — observable contracts

Three rows from `docs/BUGS.md`, all class 3 (missing/wrong presentation), so
`BUG_FIXING_PROCESS.md` treats them as **one cluster**: contracts first, one
batched probe build, one owner acceptance batch.

**Zero ROM builds have been spent.** Every value below is rung 0–1 evidence
(source constants, port source, asset tables, existing artifacts). Two of the
three rows are already localized to their owning seam from source alone.

---

## Cluster finding 2 (supersedes the scope below): ALL THREE rows converge

Row 3 was scoped as unrelated geometry work. It is not. The gun's display list
**loads its own texture** — `315_FoxUnknown.c` documents the DL as loading the
palette and texture, and the reloc confirms it (`intern dn_DL+0x5C dn_palette`,
`+0x8C dn_Tex`). That CI4 32×16 plus its 16-entry palette is **not** among the
24 resident static keys, because the model part has never been drawn.

> **So the single capability blocking this cluster is: make additional source
> textures resident on the DS.** Rows 1 and 2 need it for the blink frames and
> the three flame frames; row 3 needs it for the gun. Three thin consumers, one
> pipeline capability — not three separate fixes.

This is the right unit of work and it should be built once.

**The "will not fit" risk is refuted, and the capability already exists.**
`scripts/generate_battle_playable_static_textures.py` already contains
`build_runtime_qualified_whispy_record` (`:901`) and
`build_runtime_qualified_fox_record` (`:994`): each takes a 59-word runtime key
captured at a named profile frame, runs it through the normal conversion and
the slow oracle, and pins it to its source texture/palette. Whispy's **mouth**
is already resident by exactly this route. Adding the blink eye frame, the
three flame frames and the gun texture is *more of a thing this generator
already does* — not a new dynamic-variant subsystem.

Budget, and it is not close:

| need | format | bytes |
|---|---|---:|
| gun texture (row 3) | CI4 32×16 + 16-entry palette | 256 + 32 |
| flame frames 1–2 (row 2) | I4 16×16 each | 256 |
| Whispy blink frame(s) (row 1) | CI4, mouth-sized | few hundred |

Against `EXPECTED_RESIDENCY_BYTES 61696` — and the 2026-08-03 `repack_paletted`
change **returned 74,496 bytes of texture VRAM** (`:62-67`). The additions are
~1 KB against tens of KB of headroom.

> So the P2 deferral was about the cost of a **general** dynamic-variant system,
> not about bytes. The specialized fix — pin these specific keys the way the
> mouth and the late Fox key are already pinned — is cheap, and is exactly the
> "specialize and precompute" doctrine `PROJECT_GOAL.md` asks for.

Row 3's *state* half is already shipped (`ftParamSetModelPartID` /
`ftParamResetModelPartAll`, commit `6c2e309b03d`), so row 3 reduces to the
shared texture capability plus a submit path.

## Cluster finding 1: rows 1 and 2 share one root cause

The DS asset pipeline **packs a single frame per animated source texture**.

- `scripts/firegrind_bake.py:16` — "Texture 5 is 16x16 I4, three frames. The DS
  atlas packs only frame 0 (frames 1/2 decimate to it), so texture animation is
  frozen at frame 0 regardless."
- `docs/KNOWN_ISSUES.md:102` — "Whispy material state and geometry remain live,
  but an unprepared post-GO mouth/eye image reuses the first pre-GO resident
  source frame when every other renderer-key word matches. This accepted P1
  visual debt … complete dynamic actor texture variants remain P2 fidelity work."

Row 1 is that debt seen through Whispy's eyes; row 2 is the same limitation on
the flame texture, on top of a missing effect maker. **The owner has now put both
in `BUGS.md` as P1 delivery blockers, which overrides the KNOWN_ISSUES deferral
to P2.** The shared seam is *dynamic actor texture variant preparation*, so one
fix there serves both rows — fix it once, at the pipeline, not per-effect.

**RAM/VRAM is the binding constraint on that seam.** Heap low-water is 24,404
against the 25,600 GObj-cap threshold and a past +14 KB `.bss` stopped the ROM
booting (`ram-is-not-free-gobj-cap`). Extra texture frames are not free; the
variant budget must be counted before the batched build, not after.

---

## Row 1 — Whispy face

```text
Bug: "Whispys face looks like it plays at low FPS (sub 15 FPS), so we miss the
     blinks that the eyes do."
Stage: LOCALIZED (seam named from source + existing artifact; not yet measured)
Trigger: any Dream Land match, post-GO; blink recurs every 40-309 ticks
```

**Source oracle** — `decomp/.../gr/grcommon/grpupupu.c`, `grPupupuWhispyUpdateBlink`:
the blink fires when `whispy_blink_wait` hits `0` **and again at `-10`**, because
the random reset is skipped on the `0` case. Constants in `grvars.h`:
`GRPUPUPU_WHISPY_BLINK_WAIT_BASE 30`, `GRPUPUPU_WHISPY_BLINK_WAIT_RANDOM 270`.

> **Derived contract: Whispy double-blinks, the pair 10 ticks apart, every
> 40–309 ticks.** Not a slow animation — a rare, fast, paired event.

That matters: the owner's "sub 15 FPS" reading is a *symptom description*, not
the mechanism. Nothing is running at a reduced rate; the blink is requested at
full rate and always resolves to the same open-eye image.

| Quantity | Expected (source) | Tol | Probe |
|---|---|---|---|
| Blink requests / minute | 2 per interval, interval 40–309 tk (`grpupupu.c` `grPupupuWhispyUpdateBlink`, `grvars.h`) | ±1 interval | counter on the `gcAddAnimAll` blink call |
| Pair spacing | 10 ticks (`wait==0` then `wait==-10`) | exact | same counter, frame-stamped |
| Distinct eye images bound | ≥2 (open, blink) | exact | renderer texture-ID log on segment `whispy_eyes` |
| Eye image at a blink tick | the blink frame, not the resident pre-GO frame | exact | frame-step capture at a counted blink tick |
| Mouth images bound | source variant count | exact | same log on segment `whispy_mouth` |

**Earliest divergence — now confirmed statically, no emulator, no build:** row 3.
The asset census classifies owners into `static_owners` and
`dynamic_animated_owners`, and the dynamic group carries an explicit
`"complete_runtime_key_variants": False`
(`scripts/generate_battle_playable_texture_census.py:1465`). Whispy's eyes and
mouth are dynamic animated owners — `:1157` records that material/animation
state "changes … and thus they are not static keys." The resident static table
holds only `NDS_BATTLE_STATIC_TEXTURE_KEY_COUNT 24` keys →
`…OUTPUT_COUNT 23` outputs (`battle_playable_static_textures.generated.inc:8`),
so one key already collapses onto another output.

> **Confirmed: the blink frame was never prepared as a resident DS texture.**
> Rows 4 and 5 cannot pass at any animation rate — which is exactly why the
> symptom reads as a slow face rather than as a missing image.

The remaining probe rows measure the *live* blink cadence to confirm the
animation side is healthy — not to find the defect, which is now located.

**Row 1 is a KEY-DISCRIMINATION problem, not merely a missing texture.** Two
facts have to be read together:

- The corpus header states its scope outright: *"Other animated actors, fighter
  variants, weapons, effects, and shadows remain outside this corpus"*
  (`generate_battle_playable_static_textures.py:14`). The generator carries a
  `WHISPY_MOUTH_OWNER_MASK` record and **no eye record at all**.
- `KNOWN_ISSUES:102` says the unprepared image reuses the resident frame **"when
  every other renderer-key word matches"** — and the corpus is
  `KEY_COUNT 24 → OUTPUT_COUNT 23`, i.e. a collapse already happens once.

> So the blink key **is** being formed at runtime and is matching the open-eye
> entry. Adding a resident blink texture without making the key discriminate the
> differing word would leave the match intact and change nothing on screen.

That also predicts the "static key misses" counter — which
`required_device_promotion_evidence.must_latch_zero` requires to be **zero** —
stays zero today, because nothing is missing: it is matching the wrong thing.
**Check that counter before writing the fix**; a zero there is consistent with
this reading and inconsistent with "the texture is absent", which is the cheap
discriminator between the two.

**Fix seam:** dynamic actor texture variant preparation for the
`whispy_eyes` / `whispy_mouth` segments (`scripts/stages/generate_nds_native_stage.py`,
`segment_order` at :4161). Not the animation clock — do not add a rate divider or
"speed up the face"; that is treating the symptom.

**Prediction:** Whispy's eyes shut and reopen in a quick double-blink a few times
per ten seconds, at the same cadence as the N64.

---

## Row 2 — missing fire burn

```text
Bug: "Missing fire burn effects. the explosion effect is there but not the
     flame burn"
Stage: LOCALIZED (substitute identified at its seam)
Trigger: any fire hit (Mario fireball contact, fire attacks)
```

**Source oracle** — the flame makers exist in BattleShip and are **absent from
`src/`**: `efManagerFlameLRMakeEffect` (`efmanager.c:2512`),
`efManagerFlameRandomMakeEffect` (`:2579`), `efManagerFlameStaticMakeEffect`
(`:2643`).

**Port disposition** — `src/port/reloc_backend_compat_shims.c:8088` maps
`nEFKindFlameLR`, `nEFKindFlameRandom`, `nEFKindFlameStatic` and
`nEFKindFireSpark` **all** to a single
`ndsEFManagerMakeVisualEffect(nNDSVisualEffectHitFire, &pos, 0.7F, lr, NULL)`.
The comment at `:7956` records the intent: the source makers "fall through
because P1 cannot request them, while linking their maker would still cost
`.text`."

> Per `BUG_FIXING_PROCESS.md`, **substituting other content is containment, not
> a fix.** The explosion the owner sees is that substitute; the burn is the
> distinct effect that was never linked.

| Quantity | Expected (source) | Tol | Probe |
|---|---|---|---|
| Effect kinds requested | `FlameLR` / `FlameRandom` / `FlameStatic` distinct from `FireSpark` | exact | counter per `nEFKind` at the shim |
| Particle count per burn | source maker's spawn count (`efmanager.c:2512/2579/2643`) | exact | spawn counter |
| Texture frames shown | 3 (16×16 I4, `firegrind_bake.py:16`) | exact | texture-ID log per presented frame |
| Lifetime | source lifetime | ±1 frame | despawn counter |
| Attachment | follows the burning fighter | ±4 world units | position read vs owner |
| Spawn frame vs hit | source offset | ±1 frame | frame-step capture |

**Earliest divergence — confirmed from the generated bank, no build.** The burn
is a **particle script**: `efManagerFlameLRMakeEffect` calls
`lbParticleMakeScriptID(gEFManagerParticleBankID, 0x12)` (`efmanager.c:2540`).
`docs/optimization/NDS_PARTICLE_BANKS.generated.json` settles its fate:

| fact | value |
|---|---|
| script `0x12` (18) in `reachable_scripts` | **NO** (the set runs …17, 19…) |
| Flame seams in `p1_seams` | **absent entirely** |
| `efManagerFireSparkMakeEffect` | in `p1_seams`, but listed `p1_seams_without_bank_scripts` |
| `efManagerDamageFireMakeEffect` | seed script **77**, reachable ✓ |
| `efManagerFireGrindMakeEffect` | seed script **11**, reachable ✓ |
| source texture 5 (the flame texture) | **already packed** |

> That is the owner's sentence, exactly. **"The explosion effect is there"** —
> scripts 11 and 77 are packed and reachable. **"but not the flame burn"** —
> script `0x12` was never reachable, because the Flame seams were never in the
> P1 reachability set, so the packer had no reason to include it.

So the alias at `shims.c:8088` is a consequence, not the cause: there was no
bank script to call even if the makers had been linked.

**Fix, in order:** add the Flame seams to the P1 reachability set so script
`0x12` is packed, then link the real makers and drop the alias. The texture is
already resident, so the only new bytes are one script in a 10,912-byte,
119-script bank — and frames 1/2 of texture 5, which is the shared
texture-variant capability above.

#### First measurement, and why it does NOT settle reachability

`flame-engage.log`, whole match, same instrument as row 3a:

```
gNdsFighterFlameEffectRequestCount = 0
gNdsFighterFireSparkRequestCount   = 0
gNdsFighterModelPartOnCount        = 18   <- control
gNdsFighterProjectileProofSpawnSuccessCount = 17
```

The control proves the run was real and the counter mechanism works, so the
zeros are trustworthy *as readings*. They are **not** evidence that the Flame
family is unreachable in P1, and reporting them that way would have killed the
row for the wrong reason.

> This arm is **Mario as an idle human** versus a level-3 Fox CPU. Mario never
> attacks, and **Mario's fireball is the fire source** — Fox's moveset is not
> fire-based. Zero fire damage occurred, so zero flame requests is the expected
> reading whether or not the seam is reachable.

The arm that can answer it is `NDS_R2_BOTH_CPU=1`, where Mario is also a
level-3 CPU and actually throws fireballs. Re-measure there before touching
`P1_PARTICLE_SEAMS`.

#### Second measurement (`NDS_R2_BOTH_CPU=1`) — the Flame lane is DEAD

```
gNdsFighterFlameEffectRequestCount = 0
gNdsFighterFireSparkRequestCount   = 0
gNdsFighterModelPartOnCount        = 5    <- was 18: the arm really differs
gNdsFighterProjectileProofSpawnSuccessCount = 10   <- was 17
```

`ModelPartOn = 5` accounts for Fox's five SpecialN raises, so of the ten spawns
roughly **five are Mario's fireballs** — Mario threw fire on this arm. Still zero
Flame requests.

> **So `P1_PARTICLE_SEAMS` must NOT be touched: the Flame family is genuinely
> unreachable for Mario and Fox, and packing script `0x12` would have added
> bytes and fixed nothing.** The `nEFKindFlame*` ids come from a *fighter's own
> motion-script effect events*, and neither of these two fighters emits them.

**Corrected candidate for what the owner is seeing.** The burn that IS reachable
is `efManagerFireGrindMakeEffect` — **seed script 11, reachable and packed** —
alongside `efManagerDamageFireMakeEffect` (script 77). And `firegrind_bake.py:16`
already records its defect: *"Texture 5 is 16x16 I4, three frames. The DS atlas
packs only frame 0 (frames 1/2 decimate to it), so texture animation is frozen at
frame 0 regardless."*

That fits the report better than the Flame lane ever did: the effect is present
(so the owner sees *something* fiery) but frozen on one frame, so it reads as a
missing burn rather than a missing effect. **It also returns row 2 to the
cluster's shared capability** — packing frames 1/2 — instead of a seam-list
change.

**…and that candidate is wrong too. Both packed fire effects are accounted for.**

- **`FireGrind` is an owner-approved presentation delta, not a defect.**
  `NDS_R2_FIREGRIND_NATIVE ?= 1` (`Makefile:1025`) replaces the source hierarchy
  (root particle + 3 generators + 6 sparks) with three source-derived quads, and
  `nds_firegrind.h:20` records it **"owner-playtested and accepted 2026-08-07"**.
  `BUG_FIXING_PROCESS.md` Step 2 is explicit: cite a recorded approval, do not
  "fix" it. Its frozen-at-frame-0 texture is called out there as **the same
  presentation the generic path already drew**, so that is not a regression
  either.
- **`DamageFire` runs the REAL source implementation.**
  `battleship_efmanager.c:144` renames the imported source maker to
  `ndsBaseEFManagerDamageFireMakeEffect` and `:1926` forwards to it. The
  substitute at `shims.c:8399` is `__attribute__((weak))` — the import-off
  fallback only.

> So all three fire candidates examined so far are closed: the Flame family is
> unreachable (measured, both arms), FireGrind is approved, DamageFire is real.
> **Row 2's missing content has not been identified yet**, and no change should
> be made until it is.

**Remaining candidates, cheapest first — none requires a speculative edit:**

1. The **Mario fireball projectile's own visual** (`battleship_mario_fireball.c`)
   — the flame that travels, distinct from anything counted above.
2. A **burn that rides the victim** after a fire hit, which would be a fighter
   motion-script effect and therefore visible as a different `nEFKind` at
   `ftParamMakeEffect`.

Both are answered by the same cheap instrument already in the ROM: **log the
distinct `effect_id` values reaching `ftParamMakeEffect` over a both-CPU match**,
rather than counting one hypothesis at a time. That converts "which effect does
the owner mean" from guesswork into an enumeration, and it is one build.

#### Enumeration result — the whole `nEFKind` fire space is dead in P1

`effectkind-mask.log` / `damagefire.log`, both-CPU, whole match. **14** distinct
kinds, identical across two independent runs (mask0 `0xb84ba800`, mask1 `2051`):

```
 11 DustLight        13 DustHeavyDouble    15 DustHeavy
 16 DustHeavyReverse 17 DustExpandLarge    19 DustDashSmall
 22 ImpactWave       27 DamageFlyMDustReverse
 28 SparkleWhite     29 SparkleWhiteMultiExplode  31 SparkleWhiteScale
 32 QuakeMag0        33 QuakeMag1          43 FlashMiddle
```

**Correction — an earlier revision of this file reported "three kinds: 0, 1,
33". That was a DECODER BUG, not a reading.** The harness prints thousands
separators, and the decoder's `(\d+)` truncated `3,091,965,952` to `3`, so the
"kinds" were the leading digits of three numbers. The raw words were identical
in both runs all along; only the decode was wrong. Ids are read with
`([\d,]+)` and comma-stripped now.

FlameLR (6), FlameRandom (7), FlameStatic (8) and FireSpark (37) are **all
absent** under the corrected decode too — the conclusion survives its own
correction, and it now rests on 14 positively-identified kinds rather than on
three digits:

> **The four-way fire alias at `shims.c:8180` is dead code in P1** — none of the
> kinds it collapses is ever requested. Removing it would change nothing on
> screen, and "fixing" it could never have fixed this row.

**The instrument was on the wrong seam, and that is the finding.**
`ftParamMakeEffect` is the *fighter motion-script* effect path, and Mario's and
Fox's scripts only ask for normal-element hits and quake. Fire never enters
there.

#### The real seam: hit ELEMENT, not motion script

`ftmain.c:2713` dispatches on `ft_attack_coll->element`:

```c
case nGMHitElementFire:
    efManagerDamageFireMakeEffect(&pos, ft_attack_coll->damage);
```

with two more sites at `:2771` and `:2808`. The port `#include`s `ftmain.c`
verbatim, so this dispatch is live, and `efManagerDamageFireMakeEffect` resolves
to the real source maker (script 77, packed).

**Next measurement — one counter, at the seam this time:** count calls to
`efManagerDamageFireMakeEffect`, and alongside it the count of attack collisions
carrying `nGMHitElementFire`. The pair separates the three remaining
possibilities without another guess:

| DamageFire calls | reading |
|---:|---|
| 0 | Mario's fireball is not tagged fire, or never connects — defect upstream of effects |
| > 0 | the dispatch reaches the maker; the effect IS made |

#### Measured: `gNdsFighterDamageFireCallCount = 1`

`damagefire.log`, both-CPU, whole match, alongside `SpawnSuccess 10` /
`ModelPartOn 5`. So **the fire path is live and the real source maker did run** —
the element dispatch at `ftmain.c:2713` works, and script 77 was requested.

**But once per match makes this arm a poor instrument for a visual defect.** Two
level-3 CPUs land exactly one fire hit in sixty seconds; the owner, playing,
lands far more. So the remaining question for row 2 — *does the effect that is
made look right* — cannot be settled by this harness at this rate.

That is the point at which the process says to stop instrumenting and hand over
a ROM: row 2's remaining dimension is appearance, and the owner is the oracle for
it. What the counters have bought is that the handover is now specific — "watch a
fire hit; the effect IS being created" — rather than "is the burn missing?".

Note also that `gNdsFighterProjectileProofSpawnSuccessCount` is incremented by
**both** `battleship_mario_fireball.c:791` and `battleship_fox_blaster.c:80`, so
on the both-CPU arm it stops being a Fox-only control and becomes a combined
count — which is itself the check that Mario started acting.

**Fix seam:** two seams, in order — (a) link the real flame makers and stop
aliasing them in `reloc_backend_compat_shims.c:8088`; (b) pack the 3 flame
frames as texture variants. `.text` and VRAM cost must be counted against the
RAM headroom noted above.

**Prediction:** a fire hit shows a short flame that clings to and follows the
struck fighter, visibly different from the existing burst.

---

## Row 3 — Fox's pistol model

```text
Bug: "Fox's pistol model is missing. Also is the pistol beam emitted at correct
     y location of muzzle?"
Stage: LOCALIZED (both halves answered from source; no build needed to localize)
Trigger: Fox Neutral-B (SpecialN), ground or air
```

### 3a — the gun model

**Source oracle.** Fox's gun is not an object or an effect: it is a **model
part**, one of SSB64's per-joint alternate models driven by motion events
(`ftMotionCommandSetModelPartID(jid, mid)`, `ftdef.h:189`).

Chain, all verified in-tree:

| link | value | file |
|---|---|---|
| hold joint | `FTFOX_BLASTER_HOLD_JOINT 17` | `ftfox.h:6` |
| container slot | index **13** | `209_FoxMain.c:117` |
| slot → joint | `13 + nFTPartsJointCommonStart(4)` = **17** | `fighter.h` enum |
| descriptor | `dFoxMain_modelparts_desc_0x100[2]` | `209_FoxMain.c:110` |
| geometry | file **315**, DL at `0x3F0` — 44 `Vtx`, 38 `Gfx`, 32×16 CI4, 16-colour RGBA5551 palette | `315_FoxUnknown.c` |

The same DL at the same offset is referenced by **`KirbyMain`**
(`229_KirbyMain.reloc`) — Kirby copies Fox's blaster, which independently
confirms this asset is the gun. The decomp's "FoxUnknown" is an unresolved
label, not an unknown asset.

**Port state — exactly two breaks.**

1. `ftParamSetModelPartID` is a **no-op stub** that discards all three
   arguments — `src/port/reloc_backend_compat_shims.c:6399`.
   `ftParamSetModelPartDefaultID` (`:13386`) forwards into that stub.
   Source behaviour to mirror (`ftparam.c:768-785`): set
   `modelpart_status->modelpart_id_curr`, then `joint->dl = modelpart->dl`.
   The N64 mechanism is a **display-list pointer swap on the joint DObj**; the
   DS equivalent is to submit the gun's baked stream at joint 17's matrix,
   because the DS fighter has no per-joint DL to swap.
2. The DS fighter geometry pipeline emits **no model-part geometry at all**:
   `scripts/fighters/generate_nds_native_owners.py` has no model-part concept,
   so the gun is absent from the baked owner plan.

**Dispatch is NOT a break — it already works.** An earlier draft of this
contract claimed no dispatch existed because `nFTMotionEvent*` appears nowhere
in `src/`. That inference was wrong: the port `#include`s the original
`ftmain.c` verbatim (`src/import/battleship_ftmain.c:81`), and the source
decoder handles the event at `ftmain.c:575`, calling
`ftParamSetModelPartID(fp, ftParamGetJointID(...), modelpart_id)`. The motion
script's request is live today and is being swallowed by the stub in break 1.
`nFTMotionEventResetModelPartAll` / `HideModelPartAll` (`:585` / `:591`) are
dispatched the same way, and their shims (`:1346` / `:1454`) already exist.

> Absence from `src/` proves nothing in this repo — imported source is compiled
> through `#include`, so grep for the caller before concluding a seam is dead.

| Quantity | Expected (source) | Tol | Probe |
|---|---|---|---|
| Model-part event dispatched | `SetModelPartID(17, id)` on the SpecialN script | exact | counter at the event decoder |
| Part id applied | `modelpart_status[13].modelpart_id_curr` = source id | exact | read the status array |
| Gun geometry submitted | 44 verts / 38 Gfx present in the frame | exact | submitted-primitive counter |
| Attachment | rides joint 17's world matrix | ±2 world units | position read vs joint 17 |
| Visible window | on for the SpecialN frames the script covers, off after | exact | per-frame counter |
| Texture/palette | 32×16 CI4 + 16-colour palette from file 315 | exact | texture-ID log |

**The gun asset is present and structurally verified — no ROM extraction needed.**
It is *not* under a Fox name, which is why a name search says it is missing:

```
decomp/BattleShip-main/BattleShip_o2r/reloc_extern_data/MiscData315
  file id 0x13b (315)   resource 1392 B   data_size 0x520 (1312 B, exactly as
  315_FoxUnknown.c documents)   sha256 2bb01cdd7c846c63b0946cae9b83c2a0d4ddd532f42921fe6eaece5c61b72cc7
  0x008 palette 16 x RGBA5551   0x030 CI4 32x16   0x130 Vtx[44] (first -54,36,9)
  0x3F0 Gfx[38], opens 0xE7 G_RDPPIPESYNC
```

Found by scanning all 2,132 O2R resources by **file id**, after a name search
returned nothing and the decomp build tree turned out to hold only three
extracted asset directories (`MarioModel`, `StageCastleFile2`,
`StageHyruleFile2`) — Fox geometry comes from O2R, not from `.inc.c`. Neighbours
313/314/316 are `FoxModel`/`FoxShieldPose`/`FoxSpecial4`, so 315 landing in
`reloc_extern_data` is a naming artifact, not a missing asset. Do not re-derive
this: search O2R by id, never by name.

**The gun's texture key, derived offline from its own display list.** No probe
run is needed: `build_runtime_qualified_whispy_record` *constructs* its
`DisplayState` explicitly, and its "captured at profile frame 699" note is
provenance, not a runtime dependency. Decoding the 38 `Gfx` at `0x3F0` gives
the same thing directly.

| field | value | from |
|---|---|---|
| format / size | `FMT_CI` / `SIZ_4B` | `SETTILE` cmd 10 `fmt=2 siz=0` |
| dimensions | **16 wide × 32 tall** | `masks=4` → 16, `maskt=5` → 32 |
| line / tmem / palette | `1` / `0` / `0` | `SETTILE` cmd 10 |
| clamp | `cms=2`, `cmt=2` | `SETTILE` cmd 10 |
| TLUT | 16 entries → tile 5, tmem 256 | `LOADTLUT` cmd 13, `SETTILE` cmd 8 |
| texel bytes | 256 (`LOADBLOCK texels=128` × 16-bit) | cmd 19 |

16×32 CI4 = 256 B, which matches `dFoxUnknown_Tex[0x100]` exactly. Note the
decomp's header comment calls this "32×16"; the tile masks say 16 wide by 32
tall, and the byte count agrees with the masks either way.

The decode is confirmed against the reloc's own intern pointers rather than
trusted: `dn_DL+0x5C`, `+0x8C`, `+0xAC`, `+0xF4` land exactly on the `w1` words
of commands 11, 17, 21 and 30 — the palette, the texture, and the two vertex
loads. The raw address words read as placeholders because the reloc backend
patches them at load.

**This table is a PREDICTION, not yet a pinnable key — and that is the correct
sequencing.** The generator's contract is that entries are "qualified from
complete runtime captures and their pinned source bytes" (`:13`), and both
existing builders cite a real capture (Whispy at profile frame 699, Fox at
frame 1111 / native run 43). **The gun's texture has never been requested at
runtime**, because the model part was never applied — so no capture can exist
yet, and inventing an `expected_key` would break the very invariant those
`falsify()` guards protect.

Order, therefore:

1. **State half — shipped** (`6c2e309b03d`). The runtime now records that model
   part 13 is on.
2. **Probe build**: attempt the gun draw so the runtime genuinely requests the
   texture, and capture the resulting 59-word key. The table above is the
   written prediction that build confirms — which is exactly the one thing
   `BUG_FIXING_PROCESS.md` permits a ROM build to be spent on.
3. **Pin it**: add `MiscData315` as a census input and a
   `build_runtime_qualified_fox_gun_record` beside the two that exist, then
   update the deliberately hash-pinned `EXPECTED_KEY_COUNT` /
   `EXPECTED_OUTPUT_COUNT` / `…_PAYLOAD_BYTES` / `…_PAYLOAD_SHA256`.

The prediction is worth having before the build rather than after: if the
captured key disagrees with this table, the disagreement localizes the defect
instead of being absorbed as "whatever the runtime said".

**Fix seam — two candidate shapes; pick by a code read, not a build.**

The port keeps **live** `fp->joints[]` DObjs with MObj chains: the sibling
`ftParamSetTexturePartID` (`shims.c:1351-1396`) walks `joint->mobj` and writes
`mobj->texture_id_curr`, and its neighbour carries a `BUGS.md #7` comment about
a previous fix in exactly this class. Fighter part state is live, not frozen.

A general F3D executor also already exists — `ndsRendererExecuteDisplayList`
(`reloc_backend_mp_collision.c:13665`) and `ndsRendererAdapterSubmitStageDL`
(`renderer_dl.c:11351`), both driven from `dobj->dl`.

1. **Mirror the source**: set `joint->dl = <gun DL>` and let the existing
   executor draw it — exactly what `ftparam.c:781` does. Cheapest, *if* the
   fighter draw path routes joints through that executor.
2. **Bake a tiny owner**: 44 verts submitted after Fox at joint 17's matrix,
   gated by one visibility flag. Correct regardless of how fighters draw, and
   the right shape if the Task 56 native primitive stream ignores `dobj->dl`.

**Resolved by code read — shape 1 is dead, and a third, cheaper shape wins.**
The fighter body draws from a baked primitive stream,
`src/nds/nds_native_fighter_owner.generated.inc` gated on
`NDS_TASK56_FIGHTER_PRIMITIVES` (`nds_renderer.c:26069`, `:26459`). It never
reads `dobj->dl`, so setting `joint->dl` the way the source does would write a
pointer nothing draws.

3. **Draw the gun the way the beam is already drawn.** `gcDrawDObjDLHead1`
   (`opening_movie_backend.c:4555`) is a live, working DL draw used by the
   blaster beam itself (`battleship_fox_blaster.c:73`,
   `wpDisplayMain(weapon_gobj, gcDrawDObjDLHead1)`), by the spotlight and by
   the boss shadow (`taskman_seam.c:6162`, `:6903`). Give the gun a small DObj
   parented to joint 17 with `proc_display = gcDrawDObjDLHead1` and the gun's
   DL, and it renders through a path this ROM already exercises every time Fox
   shoots — without touching the baked fighter pipeline at all.

   The per-frame cost is DL interpretation for 44 verts, and only during the
   SpecialN window, which is short and rare. Against `PROJECT_GOAL.md`'s
   "fastest correct" rule that is the right trade for P1: shape 2 (baking a
   dedicated owner) is faster still and remains the fallback if the interpreted
   cost measures badly, but it should not be paid for before it is shown to be
   needed.

Either shape needs a real `ftParamSetModelPartID` writing `modelpart_status`,
but **not** a general model-part interpreter: P1 has exactly one model part that
matters. All three model-part shims are no-ops today and nothing in `src/` reads
`modelpart_status`, so the state half is unowned as well as the geometry half.

### 3a status: MEASURED — the state half is proven, the defect is downstream

Whole match on `build-c126-modelpart`, 1,600 samples from frame 438, `-RingDump`,
DLDI ON (`modelpart-engage.log`, ROM sha `DE80E46BDCF1FD98`, `slips=0`):

```
gNdsFighterModelPartSetCount   = 18
gNdsFighterModelPartOnCount    = 18
gNdsFighterModelPartResetCount = 18
gNdsFighterProjectileProofSpawnSuccessCount = 17   <- control
```

| contract row | expected | measured | verdict |
|---|---|---|---|
| model-part event dispatched | > 0 on a match where Fox shoots | 18 | **GREEN** |
| part id applied | every set is an "on" (id ≥ 0) | `Set == On == 18` | **GREEN** |
| visible window closes | reset must match set, or the gun latches | `Reset == 18` | **GREEN** |

The control is what makes this readable rather than a bare count: Fox actually
fired 17 times, so 18 gun-on events is the ~1:1 relationship SpecialN implies —
the one extra is consistent with a SpecialN that raised the gun without its
spawn frame landing inside the sampled window. Had the event not been reaching
the setter, this would have read `spawns 17 / On 0`.

`Reset == Set` is the specific thing worth having measured: it is the
`BUGS.md #7` latch failure not happening.

**So row 3's remaining defect is entirely in the draw**, and that is now a
measured statement rather than an inference from reading the renderer.

### 3b — the muzzle Y (answered, no build required)

The port **`#include`s the original `ftfoxspecialn.c` verbatim** at
`src/import/battleship_fox_blaster.c:90`, and re-defines the two constants
identically (`:15` `FTFOX_BLASTER_HOLD_JOINT 17`, `:19`
`FTFOX_BLASTER_SPAWN_OFF_X 60.0F`).

Source spawn (`ftfoxspecialn.c:20-25`):

```c
pos.x = FTFOX_BLASTER_SPAWN_OFF_X;   /* 60.0F */
pos.y = 0.0F;
pos.z = 0.0F;
gmCollisionGetFighterPartsWorldPosition(fp->joints[FTFOX_BLASTER_HOLD_JOINT], &pos);
wpFoxBlasterMakeWeapon(fighter_gobj, &pos);
```

> **The source local Y offset is exactly `0.0F`, so the beam's Y is by
> definition joint 17's world Y — and the port runs that same code on the same
> joint with the same constants. The beam Y is source-exact by construction.**

So 3b is not a defect in its own right. What the owner is seeing is 3a: with no
gun drawn, the beam originates from empty space at Fox's hand and reads as
mis-placed. **Predicted resolution: fixing 3a resolves 3b with no change to the
spawn path.** The one residual risk is joint 17's DS transform itself, which is
already covered by the existing fighter-joint checkers — the batched probe logs
joint 17's world position so this is confirmed rather than assumed.

**Prediction:** Fox holds a visible blaster during Neutral-B, and the beam leaves
its muzzle.

---

## What the one batched probe build must answer

Every open row above, in a single ROM, on the natural path (Fox Neutral-B and a
fire hit are reachable in seconds — no waiting out a match):

1. Distinct texture-IDs bound on `whispy_eyes` / `whispy_mouth` (row 1, row 3).
2. Blink request count and pair spacing (row 1, rows 1–2).
3. Per-`nEFKind` request counts at the fire shim (row 2, row 1).
4. Model-part event dispatch count and `modelpart_status[13]` (row 3a, rows 1–2).
5. Joint 17 world position at the spawn tick (row 3b residual).
6. Heap low-water and VRAM headroom, so the variant budget is known before any
   asset is added.

None of these needs the fix to be written first — they are all counters on the
current binary, which is what makes one build sufficient for the whole cluster.
