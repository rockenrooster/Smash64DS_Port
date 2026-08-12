**Follow Bug fixing workflow contained in `BUG_FIXING_PROCESS.md`.**
AI Agent should mark fixed items with **FIXED** prefix or a 20 word summary if not fixed yet.
These bugs should be fixed for P1 delivery:

-Whispys face looks like it plays at low FPS (sub 15 FPS), so we miss the blinks that the eyes do.

> LOCALIZED: Whispy blink timing is source-rate; post-GO eye texture variants reuse the resident frame, hiding the blink.
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
