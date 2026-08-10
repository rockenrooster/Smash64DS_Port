The highest-ROI target right now is SINT / ftMainProcUpdateInterrupt and its working set, not rendering and not another isolated soft-float function.
The current both-CPU head is about 1,411,283 P95, so the remaining gap to 1,120,380 is roughly 290,903 ticks. No single currently measured micro-optimization can close that. The reason I’d center the campaign on SINT is that the over-gate analysis points there much more strongly than anywhere else:
SRC: +171,383 over-gate vs under-gate.
SINT: +88,082 by itself.
SPHD: +28,941.
SHDT: +27,190.
SCPU: +23,531.
FTR: only +13,768.
So SINT is the largest single place where expensive work correlates with actually missing 30 FPS. P1_EXECUTION_BOARD.md
And importantly, ftMainProcUpdateInterrupt is running at CPI 11.53. That's awful in a useful way: it means there's potentially a lot more to recover from memory/layout/work elimination than from instruction tuning.
The first optimization I would actually implement
Wire up the existing dead contiguous FTParts pool and eliminate the recursive DObj-tree invalidation walk.
This is unusually attractive because most of the implementation appears to already exist:
reloc_backend_compat_shims.c already has:
C
static FTParts sNdsFTManagerPartsAllocPool[64];
static FTParts *sNdsFTManagerPartsAllocFree;
static sb32 sNdsFTManagerPartsAllocInit;
…but the pool is unused.
Meanwhile ndsFTParamsInvalidateFighterParts recursively does:
DObj
 ↓ pointer
FTParts
 ↓
write zero

child → sibling → child → sibling...
Its two dominant loads execute 79,874 times each at 37.1 and 30.8 cycles/load, accounting for ~4.95M of its 6.53M load-stall excess.
So the intended transformation is basically:
scattered heap FTParts
+
recursive DObj pointer chase
→
contiguous FTParts[]
+
linear invalidation sweep
That's exactly the kind of optimization the latest profile says the DS wants.
And compared with the fixed-animation rewrite we just discussed, this has much better immediate ROI:
much smaller implementation
already partially written
no numerical fidelity issue
no animation semantic rewrite
attacks memory stalls directly
lives inside the simulation bucket that determines the gate
likely very easy to A/B
I would do that before the full fixed-point animation representation.
But it won't get you to 1.12M alone
That's the important caveat.
I don't think there is currently evidence for a magical ~291K single cut.
My path to 1.12M would be:
FTParts contiguous pool + flat invalidation — lowest-hanging structural SINT win.
Finish decomposing SINT on over-gate frames. This is the big one. ftMainProcUpdateInterrupt contains AI, animation events, color animation, status proc_update, proc_interrupt, jostle, timers, etc. We need to know which child causes the +88K switching cost.
Compact fixed-point fighter animation representation — the design from the last answer. This is probably the best large body-wide architectural cut once the cheap SINT layout work is gone.
SPHD next. Its +28.9K discriminator and CPI 8.80 make it another memory-bound simulation target.
SCPU after that for the both-CPU gate specifically. Because it is structurally doubled in this stress arm, reducing AI frequency/work can be valuable, but it's only +23.5K of the current discriminator.
SHDT last among those four. It's spectacular when active, but earlier evidence showed most over-gate frames occur with hit detection near its floor. It's a minority-tail mechanism, not the general 30-FPS blocker.
So if you're asking me “what should the next coding agent actually work on?”, I'd give it:
Replace the recursive ndsFTParamsInvalidateFighterParts DObj-tree traversal with contiguous FTParts storage and a bounded linear invalidation path, after proving the two public invalidation entrypoints' root/subtree semantics.

If you're asking “what subsystem ultimately has the best chance of taking us from 1.41M to 1.12M?”, my answer is broader:
Collapse the memory-heavy fighter simulation working set inside SINT, with the FTParts fix first and the DS-native compact animation runtime probably the next large architectural step.

I would not spend another optimization cycle on FTR, memcpy/memset, geometry submission, or generic soft-float replacement right now. The measurements have moved the center of gravity very clearly into simulation data layout and repeated pointer-rich fighter state work.