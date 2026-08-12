**Follow Bug fixing workflow contained in `BUG_FIXING_PROCESS.md`.**
AI Agent should mark fixed items with **FIXED** prefix or a 20 word summary if not fixed yet.
These bugs should be fixed for P1 delivery:

-Whispys face looks like it plays at low FPS (sub 15 FPS), so we miss the blinks that the eyes do.

> Only one eye image is packed, so every blink resolves to the open frame. Not a rate problem.

-Missing fire burn effects. the explosion effect is there but not the flame burn 

> Source flame makers were never linked; four fire kinds all alias one substitute burst. Same texture-variant seam.

-Fox's pistol model is missing. Also is the pistol beam emitted at correct y location of muzzle?

> Gun is model part 13 on joint 17; setter is a no-op stub. Beam Y already source-exact.

Contracts for all three: `artifacts/bugs/2026-08-12_r2-07-cluster/CONTRACT.md`
(one cluster, one batched probe build, one acceptance batch).