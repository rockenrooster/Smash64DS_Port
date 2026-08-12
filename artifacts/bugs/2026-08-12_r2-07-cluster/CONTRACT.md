# R2-07 BUGS cluster — observable contracts

Three rows from `docs/BUGS.md`, all class 3 (missing/wrong presentation), so
`BUG_FIXING_PROCESS.md` treats them as **one cluster**: contracts first, one
batched probe build, one owner acceptance batch.

**Zero ROM builds have been spent.** Every value below is rung 0–1 evidence
(source constants, port source, asset tables, existing artifacts). Two of the
three rows are already localized to their owning seam from source alone.

---

## Cluster finding: rows 1 and 2 share one root cause

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

**Earliest divergence (predicted):** row 1 — all four kinds collapse to one, so
the burn is never requested as a distinct effect. Row 3 fails independently even
after row 1 is fixed, because the atlas packs frame 0 only; that is the shared
row-1/row-2 seam above.

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

**Port state — three breaks, any one of which is sufficient:**

1. `ftParamSetModelPartID` is a **no-op stub** that discards all three
   arguments — `src/port/reloc_backend_compat_shims.c:6399`.
   `ftParamSetModelPartDefaultID` (`:13386`) forwards into that stub.
2. **No dispatch exists** for `nFTMotionEventSetModelPartID`,
   `nFTMotionEventResetModelPartAll`, or `nFTMotionEventHideModelPartAll`
   anywhere in `src/` — the enum is declared in `include/ft/fighter.h:1722`
   and never handled, so the stub is not even reached from the motion script.
3. The DS fighter geometry pipeline emits **no model-part geometry at all**:
   `scripts/fighters/generate_nds_native_owners.py` has no model-part concept,
   so the gun is absent from the baked owner plan.

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

**Fix seam.** The DS owner plan is a **statically baked FIFO command stream**, so
a conditional sub-DL inside it is the wrong shape. The correct DS-specialized
form — and the one `PROJECT_GOAL.md` asks for — is to bake the gun as its own
tiny owner (44 verts) drawn after Fox using joint 17's already-computed matrix,
gated by one visibility flag. That needs a real `ftParamSetModelPartID` (setting
`modelpart_status`) and event dispatch, but **not** a general model-part
interpreter: P1 has exactly one model part that matters.

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
