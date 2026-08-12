**Follow Bug fixing workflow contained in `BUG_FIXING_PROCESS.md`.**
AI Agent should mark fixed items with **FIXED** prefix or a 20 word summary if not fixed yet.
These bugs should be fixed for P1 delivery:

-Whispys face looks like it plays at low FPS (sub 15 FPS), so we miss the blinks that the eyes do.

> LOCALIZED 2026-08-12: the blink joint animation plays out in one frame; why it resolves to one is open.

-Missing fire burn effects. the explosion effect is there but not the flame burn

> **FIXED** (2026-08-12) visibility: FlameLR's texture had no atlas cell and drew nothing; admitted, flames visible, owner confirmed.

-Fighter burn flames spawn at the wrong places on the damaged fighter.

> MEASURED 2026-08-12: flames spawned at the victim's feet (Y=Z=0); source joint rotation restored, maker-side read still owed.

-Fox's pistol model is missing. Also is the pistol beam emitted at correct y location of muzzle?

> **FIXED** (2026-08-12) visibility: submit skipped the renderer's world-unit shift and drew 0.036px wide; owner confirmed the pistol.

-Fox's muzzle flash and laser spawn at the wrong Y. Also: on N64 Mario can crouch under the laser, in the port it hits him.

> CONTRACT 2026-08-12: source spawns both from joint 17 local {60,0,0}; A-vs-B world-position invariant not yet measured.

Contracts, evidence, and the per-row work packets:
`artifacts/bugs/2026-08-12_r2-07-cluster/CONTRACT.md` (one cluster, one batched
probe build, one acceptance batch). **A counter, a maker call, or a submitted
triangle is engagement, never closure — every row of this cluster now owes
screen-space pixel evidence.**
