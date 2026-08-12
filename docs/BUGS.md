**Follow Bug fixing workflow contained in `BUG_FIXING_PROCESS.md`.**
AI Agent should mark fixed items with **FIXED** prefix or a 20 word summary if not fixed yet.
These bugs should be fixed for P1 delivery:

-Whispys face looks like it plays at low FPS (sub 15 FPS), so we miss the blinks that the eyes do.

> LOCALIZED 2026-08-12: the blink joint animation plays out in one frame; why it resolves to one is open.

-Missing fire burn effects. the explosion effect is there but not the flame burn

> OWNER-QUEUED 2026-08-12: FlameLR's texture had no atlas cell and drew nothing; admitted, flames captured on the candidate.

-Fox's pistol model is missing. Also is the pistol beam emitted at correct y location of muzzle?

> OWNER-QUEUED 2026-08-12: submit skipped the renderer's world-unit shift and drew 0.036px wide; fixed, pixels captured on c129.
> OPEN, measured separately: on N64 Mario can crouch under Fox's laser, in the port it still hits him.

Contracts, evidence, and the per-row work packets:
`artifacts/bugs/2026-08-12_r2-07-cluster/CONTRACT.md` (one cluster, one batched
probe build, one acceptance batch). **A counter, a maker call, or a submitted
triangle is engagement, never closure — every row of this cluster now owes
screen-space pixel evidence.**
