**Follow Bug fixing workflow contained in `BUG_FIXING_PROCESS.md`.**
AI Agent should mark fixed items with **FIXED** prefix or a 20 word summary if not fixed yet.
These bugs should be fixed for P1 delivery:

-Whispys face looks like it plays at low FPS (sub 15 FPS), so we miss the blinks that the eyes do.

> LOCALIZED: Whispy blink timing is source-rate; post-GO eye texture variants reuse the resident frame, hiding the blink.

-Missing fire burn effects. the explosion effect is there but not the flame burn 

> LOCALIZED: fire damage reaches DamageFire, but fire colanim scripts 12–15 are absent, so FlameLR/FlameRandom never dispatch.

-Fox's pistol model is missing. Also is the pistol beam emitted at correct y location of muzzle?

> MEASURED: Fox model-part state toggles correctly; fighter draw never consumes it, so pistol geometry is never submitted.
> Beam Y: no defect — source local offset is 0.0F and the port retains the formula. Do not add a Y offset.

Contracts for all three: `artifacts/bugs/2026-08-12_r2-07-cluster/CONTRACT.md`
(one cluster, one batched probe build, one acceptance batch).