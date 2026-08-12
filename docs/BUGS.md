**Follow Bug fixing workflow contained in `BUG_FIXING_PROCESS.md`.**
AI Agent should mark fixed items with **FIXED** prefix or a 20 word summary if not fixed yet.
These bugs should be fixed for P1 delivery:

-Whispys face looks like it plays at low FPS (sub 15 FPS), so we miss the blinks that the eyes do.

> ROOT CAUSE FOUND 2026-08-12 (`build-c128-foxgun`, no rebuild): **the blink animation lasts ONE frame.**
> `map_gobj[0]->anim_frame` reaches 1.0 for exactly one sample and is back to 0 on the next; at a 30 Hz
> presented cadence a one-frame animation is often not presented at all. Everything around it measured
> GREEN -- the countdown runs, blink #1 fires at wait==0, the `0` arm correctly does not reseed, blink #2
> fires at -10 and reseeds, and the wind-driven eye texture request cycles 0/1/2.
> The earlier "texture residency" framing was wrong twice over: the BLINK entry of
> `dGRPupupuWhispyEyesAnims` carries a NULL material anim (`grpupupu.c:76`), so the blink is a pure JOINT
> animation and there is no blink texture; and the six `dGRPupupuWhispyEyesTextures` belong to the WIND
> cycle on `map_gobj[3]`, not to the blink on `map_gobj[0]`.
> Next: why does `llGRPupupuMapWhispyEyes{Left,Right}BlinkAnimJoint` resolve to a 1-frame animation?
> First suspect is `ndsAObjEvent32NormalizeScript`'s 1,024-entry table, already flagged at 973/1,024 with
> overflow silently skipping the animation attach. Evidence:
> `artifacts/verification/2026-08-12_whispy-blink-window.txt`.
> MEASURED 2026-08-12 (`build-c128-foxgun`, whole match): texture lookup misses 0, conversion calls 0,
> conversion ticks 0, pinned static hits 70,072. The blink texture is not ABSENT — a key is being formed
> and it MATCHES a resident entry. So the fix is two halves in order: (1) make the renderer key
> discriminate the differing word, (2) then make all six `dGRPupupuWhispyEyesTextures[2][3]` variants
> DS-native and resident before GO. Six entries behind a non-discriminating key change nothing on screen.

**FIXED** -Missing fire burn effects. the explosion effect is there but not the flame burn 

> Root cause: fire colanim scripts 12-15 had no port entry, so `ftParamCheckSetColAnimID`
> rejected the burn before a single Flame command ran; and even once they ran,
> `ndsFTParamMakeSourceEffect` had no Flame case, so the requests fell to the generic
> HitFire substitute -- the real makers were not even linked (`--gc-sections` dropped them).
> Fix: scripts + descriptors restored (`shims.c`), the three Flame seams packed into the
> particle bank, and FlameLR/FlameRandom/FlameStatic routed to `efManagerFlame*MakeEffect`.
> Accepted on `build-c127-fire` (both-CPU, 1600 samples, DLDI on): effect-kind mask bits 6
> and 7 set (FlameLR, FlameRandom dispatched), `gNdsVisualEffectKindMask=0` (no HitFire
> substitute was made), all three makers present in the linked ELF.
> Evidence: `artifacts/bugs/2026-08-12_r2-07-cluster/flame-real-makers.log`.

**FIXED** -Fox's pistol model is missing. Also is the pistol beam emitted at correct y location of muzzle?

> Root cause: the model-part STATE was already correct; nothing consumed it. Source draws
> the gun by pointing joint 17's own `dl` at a display list in reloc asset 0x13b, which the
> DS cannot copy -- `ndsFighterDrawPlanResolve` rejects the whole selected collection when
> any dl resolves outside the fighter's model asset, so the assignment would push the
> ENTIRE fighter off the native draw path for one 22-triangle part.
> Fix: `NDS_R2_FOX_GUN_OVERLAY`. The part's mesh is resolved offline by
> `scripts/fox_gun_bake.py` (44 vertices, 22 triangles, CI4 32x16 + 16-entry palette, both
> already DS formats) and submitted at joint 17's world matrix straight after the fighter's
> own production run. The baked body is untouched.
> Accepted on `build-c128-foxgun`: `tris = 22 x draws`, `fail=0`, `bytes=288`, `prepare=1`,
> and the blaster is visible in Fox's hand at the muzzle in
> `artifacts/visibility/2026-08-12_fox-gun-overlay-shot.png` -- the same firing pose whose
> pre-fix control (`2026-08-09_fox-blaster-native-promoted.png`) shows an empty glove.
> Culling is deliberately NONE pending an owner playtest; the source winding is the N64's.
> Beam Y: no defect — source local offset is 0.0F and the port retains the formula. Do not add a Y offset.

Contracts for all three: `artifacts/bugs/2026-08-12_r2-07-cluster/CONTRACT.md`
(one cluster, one batched probe build, one acceptance batch).
