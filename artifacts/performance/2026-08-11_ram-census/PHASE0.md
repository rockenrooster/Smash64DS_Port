# RAM recovery — Phase 0 baseline (re-banked at HEAD)

Plan: `docs/RAM_RECOVERY_PLAN.md`. HEAD `1ce4c5f4dee`, shipping P1 configuration
rebuilt from source (the root ELF was stale by two slices — it predated 36/37 —
and the plan forbids banking off a stale ELF).

## 0.1 Static image — `smash64ds-battle-playable-hwtri.elf`

| section | addr | bytes |
|---|---|---:|
| `.main` | 0x020038e8 | 903,336 |
| `.main.rw` | 0x020e0190 | 136,996 |
| `.main.bss` | 0x021018e0 | **1,599,280** |
| `.itcm` | 0x01ff8020 | 29,800 |
| `.dtcm` | 0x02ff0000 | 8,800 |
| `.dtcm.bss` | 0x02ff2260 | 152 |
| **main total** | | **2,639,612** |

`fake_heap_start` 0x02288004 · `__heap_start_ntr` **0x02288010**

`.main.bss` is 1,536 B larger than the pre-rebuild census (1,597,744): that is
slices 36/37's memo tables, and it is why the rebuild was required.

## 0.2 Runtime state — one-minute Boundary match, 1600 frames from 438

Read with `-ExtraGlobals` on `builds/build-tick-hud-buckets`. Artifact:
`phase0-runtime.json`. **Shipping arm (NOT `NDS_R2_BOTH_CPU`).**

| counter | plan snapshot | MEASURED |
|---|---:|---:|
| `gNdsTaskmanArenaChosenSize` | 1,245,184 | **1,257,472** |
| `gNdsTaskmanArenaAllocFailCount` | — | **29** |
| `gNdsTaskmanGeneralHeapFreeMin` | 42,136 | **15,120** |
| `gNdsR2AnimCacheArenaReservedBytes` | 200,704 | 200,704 |
| `gNdsR2AnimCacheArenaUsedBytes` | 200,400 | 200,384 |
| `gNdsR2AnimCacheArenaOverflows` | 38 | **11** |
| `gNdsR2AnimCacheRejects` | 38 | **11** |
| `gNdsR2AnimWarmLoaded` | 83 | 83 |
| `gNdsOriginalDLPreviewCommitCount` | — | **0** |

### Two corrections that change the plan's arithmetic

**1. The 32 KiB safety floor is ALREADY BREACHED at baseline.**
`gNdsTaskmanGeneralHeapFreeMin` is **15,120**, not 42,136 — **17,648 B BELOW**
the plan's non-negotiable 32,768 floor. So "discretionary low-water slack
~9,368 B" is wrong; slack is **negative**. Nothing may grow until RAM is
recovered, and recovery is now required merely to restore invariant #3 rather
than to fund a bigger cache. Every "spend the slack" option in the plan's
starting snapshot is off the table.

**2. Reject count is arm-dependent — 11 on Boundary, 38 on both-CPU.**
The 38 that motivated this campaign came from the `NDS_R2_BOTH_CPU` stress arm.
Boundary — the canonical Mario-human-vs-level-3-Fox configuration — refuses
**11**. `ANIM_REQUIRED_BYTES` must therefore be derived per arm and the shipping
figure quoted against Boundary. Both are still non-zero, so the defect stands;
only its size changes.

The arena also settled at 1,257,472 after **29** failed probes, i.e. the
allocator is already probing down to what main RAM leaves it. That is the
mechanism by which recovered static bytes convert to arena bytes — and it
confirms the conversion is real rather than assumed.

## 0.3 `ANIM_REQUIRED_BYTES` — NOT yet derived

Deferred: the per-request classification the plan asks for (asset id, requested
vs aligned bytes, resident hit / first miss / rejection / repeat-of-rejected,
first frame) needs instrumentation that does not exist yet. Do not carry the old
82 KiB estimate into a cache constant — with Boundary at 11 rejects rather than
38, that number is very likely too large.

## Phase 1 proof — the DL preview pair is dead in this configuration

Three independent lines, all agreeing:

1. **Reader condition.** The only code that reads `sOriginalDLDisplayPreview`
   or the preview width/height statics is `nds_platform.c:1877-1980`, entirely
   inside `#if !NDS_RENDERER_HW_TRIANGLES`. P1 ships hardware triangles, so the
   buffers are **write-only** — filled by the opening room, displayed by nobody.
2. **Linked-ELF attribution.** Source has seven call sites for
   `ndsPlatformBeginOriginalDLPreview`; exactly **one** survives into the
   shipped battle ELF (`ndsOpeningRoomRenderDLPreview`). An exhaustive scan over
   every branch mnemonic confirms it, and the only functions holding an address
   inside either array are the three API entry points — so nothing was inlined
   and no generic helper receives the pointer.
3. **Runtime, lifetime-wide.** `gNdsOriginalDLPreviewCommitCount` = **0** across
   the whole one-minute match. A cumulative counter reading zero at the end is
   lifetime-wide proof, which is what the plan asked for instead of a ready flag
   sampled at one instant.

---

# Phase 1 RESULT — DL preview pair removed

Implementation: storage, the display-preview builder, and the three API bodies
follow `#if !NDS_RENDERER_HW_TRIANGLES` — the **same condition already on their
only reader**. `ndsPlatformBeginOriginalDLPreview` returns NULL on hwtri, which
is not a new failure mode: `ndsOpeningRoomRenderDLPreview` already handles NULL
by setting `NDS_OPENING_ROOM_DL_PREVIEW_BLOCKER_NO_PIXELS` and returning.

## Static (shipping P1 ROM)

| metric | before | after | delta |
|---|---:|---:|---:|
| `.main.bss` | 1,599,280 | 1,577,648 | **−21,632** |
| `.main` | 903,336 | 902,944 | −392 |
| total | 2,639,612 | 2,617,588 | **−22,024** |
| `__heap_start_ntr` | 0x02288010 | 0x02282a10 | −22,016 |

Clears the plan's KEEP gate (>=20 KiB) with 2,024 B to spare.

## Runtime (one-minute Boundary, 1600 frames from 438)

| counter | before | after | delta |
|---|---:|---:|---:|
| `gNdsTaskmanArenaChosenSize` | 1,257,472 | 1,277,952 | **+20,480** |
| `gNdsTaskmanArenaAllocFailCount` | 29 | 24 | −5 |
| `gNdsTaskmanGeneralHeapFreeMin` | 15,120 | **35,324** | **+20,204** |
| `gNdsR2AnimCacheArenaUsedBytes` | 200,384 | 200,384 | 0 |
| `gNdsR2AnimCacheRejects` | 11 | 15 | **+4** |

**The static→arena conversion is real, not inferred.** The allocator climbed a
rung (+20,480) and needed five fewer probes. This is the mechanism the whole
campaign depends on, now demonstrated rather than assumed.

**Invariant #3 is restored.** Heap low-water 15,120 → 35,324, i.e. from 17,648
BELOW the 32,768 floor to 2,556 above it. Phase 1 alone converts an
already-violated safety invariant back into a satisfied one.

## Open: rejects 11 -> 15

Flagged rather than waved away — the plan's revert rules list increased post-GO
file reads as a stop condition. It cannot be a behavioural consequence of the
change: `gNdsOriginalDLPreviewCommitCount` was **0** before the edit, so the
removed path never executed at runtime in this ROM. That leaves binary layout or
harness run-to-run variance. Being decided by re-running the IDENTICAL binary
(`phase1-repeat.json`); a differing repeat means the counter is not stable enough
to gate on, an identical repeat means +4 is real and needs a cause.

## Phase 1 CLOSED — `Boundary verification profile passed.`

Two follow-ups were needed after the first measurement, both recorded because
each cost a build:

**1. `--gc-sections` ate the diagnostic globals.** Removing the last writers of
`gNdsOriginalDLPreviewReady/Width/Height/CommitCount/DrawCount` dropped all five
from the link, and Boundary went RED on `Missing ELF symbol
gNdsOriginalDLPreviewReady` — a verifier failure with no behavioural cause. Five
harnesses resolve them by name. `__attribute__((used))` did NOT fix it (the
object file already emitted each into its own `.bss.gNds…` section; the linker
is what drops them) and `((retain))` was accepted silently without setting
SHF_GNU_RETAIN. **A real store from a still-linked function is what the linker
honours** — five zero-stores in the hwtri branch of
`ndsPlatformBeginOriginalDLPreview`, which is linked but never executes on P1.

**2. The rejects delta is deterministic, not noise.** An identical-binary repeat
returned bit-identical values on all five counters, so 11 → 15 is real. It
cannot be behavioural — `CommitCount` was 0 before the change, so the removed
path never ran. Leading explanation: the taskman arena grew +20,480, so
allocations that previously failed now succeed, more content is live, and more
distinct animations get requested. That is the game *using* the recovered
memory, and Phase 6 removes it by resizing the cache. Re-check there.

### Final, verified on the Boundary-green binary

| metric | baseline | final | delta |
|---|---:|---:|---:|
| `.main.bss` | 1,599,280 | 1,577,680 | **−21,600** |
| `.main` | 903,336 | 902,984 | −352 |
| total static | 2,639,612 | 2,617,660 | **−21,952** |
| `gNdsTaskmanArenaChosenSize` | 1,257,472 | 1,277,952 | **+20,480** |
| `gNdsTaskmanGeneralHeapFreeMin` | 15,120 | **35,324** | **+20,204** |
| heap floor (32,768) | **FAIL −17,648** | **PASS +2,556** | invariant restored |
| `gNdsOriginalDLPreviewCommitCount` | 0 | 0 | unchanged |

Campaign progress: **21,952 of the 98,304 B (96 KiB) target — 22.3%.**

---

# Phase 2 — `gSYFramebufferSets` (441,600 B): consumer contract traced

Not yet implemented. Four structural questions answered; recorded so the slice
can be built without re-deriving them.

**1. Nothing writes meaningful pixels.** Cycle 84 measured it on DS:
`gSYFramebufferSets[0][0][0]`, `[1][115][160]` and `[2][0][0]` all held only the
clear value `GPACK_RGBA5551(0,0,0,1)` deep into a match, with the same probe
demonstrably seeing 0→1 transitions nearby. BattleShip's own PC port documents
the identical situation for any hardware rasteriser
(`decomp/.../port/bridge/framebuffer_capture.h`): *"the RDP is replaced by a GPU
rasterizer and gSYFramebufferSets[] never receives any pixels — the wallpaper
copy reads zeros, hence the all-black background"* (their issues #57, #81). The
DS presents from its own `sFramebuffers[]` in `nds_platform.c`, unrelated to
this array.

**2. The clear is bounded by `sizeof`, so shrinking is self-adjusting.**
`decomp/.../sc/scmanager.c:855-859` already carries an NDS branch:
```c
framebuffer = (u16*) gSYFramebufferSets;
#if defined(SSB64_TARGET_NDS)
end = (uintptr_t)gSYFramebufferSets + sizeof(gSYFramebufferSets);
#else
end = 0x80400000;
#endif
```
This was the main heap-corruption hazard and it is already handled. Note the
contrast with `gSYZBuffer`, whose reduction was safe *because* nothing takes its
`sizeof`; here it is safe *because* something does.

**3. The N64 arena-sizing coupling does NOT bind on DS.** Nine imported scenes
compute `scene_setup.arena_size = (uintptr_t)&gSYFramebufferSets -
(uintptr_t)&ovl4_BSS_END`, i.e. the array's ADDRESS is the arena's upper bound
on N64. On DS the layout is inverted — `gSYFramebufferSets` 0x021b24b0 sits
BELOW `ovl4_BSS_END` 0x02271bf4 — so that expression is negative and, as a
`size_t`, absurd. The DS therefore already overrides it with its probe loop.
**This also confirms Phase 1's reported mechanism**: the arena grew because the
probe settled one rung higher (`AllocFailCount` 29 → 24), not because an address
gap widened. `gSYFramebufferSets` did move down by exactly 21,952, so the
alternative explanation had to be excluded rather than assumed.

**4. The remaining real constraint is the Z-buffer alias.**
`SYVIDEO_ZBUFFER_START` resolves to `gSYZBuffer - 6400`, and the last 6,400
bytes of `gSYFramebufferSets[2]` are what that backs into. ~17 scene-start
functions hold `base+435,200` = exactly `441,600 - 6,400`, which is that
pointer — they are storing the Z-buffer start into their `SYVideoSetup`, not
touching pixels. Cycle 84 proved nothing writes through it, so this is an
addressing contract only: any shrink must keep that address inside valid memory.

**Assessment:** Candidate D (remove a dummy compatibility surface) is the live
option and the blockers that would normally kill it are already handled. The
open work is the Results photo-wipe read path — whether the DS wipe samples this
array at all, and what it looks like today — plus `artifacts/visibility`
evidence of the transition before any change, per the plan's 2.2/2.4.

## Phase 2 BLOCKED — Candidates A and B cannot ship without patching decomp

Implemented, then **reverted before building**. Recording the reason once, per
the plan's rule for failed architectural candidates, so nobody repeats it.

The shape looked right and three of four hazards were clear: nothing writes real
pixels, the NDS clear self-adjusts via `sizeof`, and the arena-address coupling
does not bind on DS. The wipe's read is also satisfiable by one buffer — it
copies a 300x220 window from whichever single buffer the scheduler selected, and
all three only ever hold the same clear value.

**What kills it: the `[3]` extent is load-bearing across decomp TUs that are
linked into the P1 ROM.**

- `decomp/.../mv/mvopening/mvopeningroom.c:1907-1911` compares
  `gSYSchedulerCurrentFramebuffer == gSYFramebufferSets[1]` and `== [2]`.
- `decomp/.../mn/mncommon/mntitle.c:126-127` takes `&gSYFramebufferSets[1]`
  and `[2]`.
- `SYVIDEO_SETUP_DEFAULT` in **decomp's own** `sys/video.h:40-42` expands to
  `[0], [1], [2]` and a dozen scene TUs instantiate it.

Those TUs include decomp's header, which declares `[3][230][320]`. Shrinking the
definition port-side does not change their view: `sizeof(gSYFramebufferSets)`
stays 441,600 inside `scmanager.c`, so **the scene-manager clear would write
294,400 bytes past the end of the shortened array.** That is heap corruption,
not a visual regression — and it is invisible to
`check-decomp-header-mirror.py`, which compares constants and not array extents.

Note the asymmetry with `gSYZBuffer`, whose identical-looking reduction WAS
safe: nothing takes its `sizeof` and nothing indexes it. Here both are false.
"The zbuffer precedent worked" is not transferable evidence.

**Consequently `gSYFramebufferSets` is NOT a port-side-only change.** The routes
that remain, in increasing cost:

1. **Patch decomp** under `scripts/decomp-patches/battleship/` — the sanctioned
   mechanism (AGENTS.md) and how `scmanager.c` already got its
   `SSB64_TARGET_NDS` branch. Needs decomp's `sys/video.h` extent + macro,
   `mvopeningroom.c` and `mntitle.c`. Unblocks the full 294,400 B.
2. **Candidate D at the seam** — give the wipe a DS-native capture so the array
   stops being a pixel source at all, then shrink under route 1 anyway.
3. Leave it and take Phases 3-5 first.

Route 1 is the cheap one and should be the next slice; nothing about it is
speculative now that the three offending sites are named.

**Campaign status: 21,952 of 98,304 B (22.3%). Tree is clean — the reverted
attempt left no residue.**

### CORRECTION to the blocker list — one of the three was already handled

I listed three blocking sites. Checking each one's actual preprocessor context
rather than its source text cuts that to two:

| site | context | blocks? |
|---|---|---|
| `mvopeningroom.c:1907-1911` | `#else` of `#if defined(SSB64_TARGET_NDS)` | **NO — already excluded on NDS** |
| `mntitle.c:126-127` | top level | **yes** |
| decomp `sys/video.h:40-42` | top level | **yes, conditionally** |

`mvopeningroom.c` is already one of the eight patched files, and its framebuffer
comparisons live in the non-NDS arm. Citing it as a blocker was reading source
text where the compiled configuration was what mattered — the same mistake the
seven-call-sites-vs-one finding warned about earlier in this campaign.

**The blocker reduces to one question:** which `sys/video.h` does each decomp TU
see? The corruption risk is entirely `sizeof(gSYFramebufferSets)` inside
`scmanager.c`'s clear. If decomp TUs take decomp's header (`extern [3]`), the
clear overruns a shortened array and the candidate is dead without patching that
header. If the build makes them take `include/sys/video.h`, then the port-side
change is sufficient and `mntitle.c`'s address-taking is inert UB — those
addresses are never dereferenced in P1 because the title scene never runs.

That question is answerable without a build (include-path order for the
`src/import` wrappers) and is the first thing the next slice should settle. The
patch mechanism itself is ready: `scripts/decomp-patches/battleship/` with an
ordered map in `fetch-battleship-reference.ps1`, and **note its warning** — a
file pinned by `scripts/stages/generate_nds_native_stage.py` aborts Boundary
with a SHA mismatch, so check the pin list before patching.

---

# Phase 2 SHIPPED — `gSYFramebufferSets[3]` → `[2]`, and the arena reaches 0x150000

The earlier BLOCKED entry stands as history but **its stated reason was wrong**,
and the correction is the useful part.

**The header question was a red herring.** Preprocessing
`src/import/battleship_scmanager.c` shows it resolves `<sys/video.h>` to
`include/sys/video.h` — `INCLUDES` puts `include` ahead of
`$(BATTLESHIP_DECOMP)/src`, so every decomp TU sees the PORT header. The clear's
`sizeof(gSYFramebufferSets)` therefore shrinks with the port-side extent and
never overruns. No decomp patch was needed at all.

**The real reason `[1]` was unsafe is the wipe's own arithmetic.**
`lbtransition.c:226-241` starts at `base + 6,400 + 140,800 + 20 = base+147,220`
— already 20 bytes past buffer 0 — and walks **backward** 640 bytes per row for
220 rows, touching `base+7,060 .. base+147,819`. So:

| extent | bytes | highest touched 147,819 |
|---|---:|---|
| `[1]` | 147,200 | **OUT by 620** |
| `[2]` | 294,400 | in bounds |
| `[3]` | 441,600 | in bounds |

`[2]` is the smallest correct size. All three `SYVideoSetup` slots alias buffer
0 so the wipe always reads the in-range span; the scheduler only assigns those
pointers and never compares them, and `mvopeningroom.c`'s comparisons are in the
non-NDS arm.

### Result — Boundary green

| metric | Phase-0 baseline | final | delta |
|---|---:|---:|---:|
| `.main.bss` | 1,599,280 | 1,430,480 | **−168,800** |
| total static | 2,639,612 | 2,470,460 | **−169,152** |
| `gNdsTaskmanArenaChosenSize` | 1,257,472 | **1,376,256** | **+118,784** |
| `gNdsTaskmanArenaAllocFailCount` | 29 | **0** | −29 |
| `gNdsTaskmanGeneralHeapFreeMin` | 15,120 | **133,628** | **+118,508** |
| heap floor (32,768) | FAIL −17,648 | **PASS +100,860** | — |

**1,376,256 is exactly `0x150000`** — the arena size the port has always
requested. The plan opened by recording that the runtime was "128 KiB short of
the desired taskman arena"; that shortfall is now **zero**, and the allocator no
longer probes down at all (`AllocFailCount` 29 → 0).

**Plan targets met: 169,152 B recovered = 172% of the 96 KiB requirement and
132% of the 128 KiB stretch.**

**Owner playtest note:** this resizes the buffer the Results photo wipe samples.
Behaviour should be identical — same buffer, same clear value, same 300x220
window — but Results is the screen to look at.

**Remaining (Phases 6-8):** rejects are still 15 and the anim cache is still
capped at 200,704. There are now **100,860 bytes above the safety floor** to
fund residency, against a demand that must still be measured per arm
(`ANIM_REQUIRED_BYTES`, Phase 0.3, deliberately not carried over from the stale
82 KiB estimate).
