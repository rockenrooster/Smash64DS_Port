# Handoff

Updated: 2026-08-02 evening. **Working the `BUGS.md` queue under
`BUG_FIXING_PROCESS.md` v2.** Read that document first: the source is the oracle,
the owner is confirmation only, and a ROM build is spent to confirm a written
prediction — never to see whether it looks right.

## THE SHIELD FREEZE IS ROOT-CAUSED, and it was never the allocator

`ndsBattlePlayablePresentRealtimeFrame` **never rewound the graphics heap**.
`taskman_seam.c` has two frame loops; the fast-verify one calls
`syTaskmanResetGraphicsHeap()` beside `func_80004AB0()` citing decomp
taskman.c:1093-1100 (*"resets these arenas before every source scene draw"* —
**both** arenas), and the shipping one called only `func_80004AB0()`. The guest
therefore leaks **16 bytes a frame** — one `Light` from `scVSBattleFuncLights`,
which `ftdisplaylights.c:26` bumps and never returns — until
`syTaskmanCheckBufferLengths` hits its `while (TRUE);` at decomp taskman.c:344.

**How it was settled, and the method is the durable part: SCALE THE CEILING.**

```text
graphics heap 53,248  ->  spun at used=53,264 after 3,328 presented frames
graphics heap 81,920  ->  spun at used=81,936 after 5,120 presented frames
```

Same +16 overshoot at both sizes and `81,920/53,248 = 5,120/3,328` exactly, so
it is a leak and not a high-water. One capture could not have said that; two at
different ceilings did, for one build. **A first capture that says "N bytes over"
is not yet a mechanism.**

Two things hid it for a month. `ndsFighterDisplayContractCapture` saves and
restores the heap pointer around each fighter draw, so the largest consumer
rewinds itself and only the remainder leaks — slowly enough to read as
"sometimes". And the July investigation sampled `used=0` beside a *different*
freeze (taskman.c:338, the DL buffer), concluded "nothing accumulates in it",
and wrote its own falsifier down: *"Add it only if a capture ever shows
taskman.c:344 instead of :338."* That is what fired.

**Never say "the allocator" without the counter.** BattleShip has eleven
`while (TRUE);` give-ups. `sys/malloc.c:30` is heap exhaustion (`MALLOCOVF`);
`sys/taskman.c:338` is a DL buffer past its end (`DLBUF0..3`); `:344` is the
graphics heap (`GFXHEAP`). The soak prints all of them. `__excpt_entry`'s park is
a self-branch too, so a CPU abort reads like an allocator spin.

## Today's candidate — nine rows, one build, none owner-accepted yet

**The 7-minute both-CPU soak is NO-FREEZE at 11,179 presented frames**, against
3,328 and 5,120 in the two arms that froze. `SyMallocOverflow 0`,
`RelocHeapDecline 0`, `AudioFgmPoolExhaust 0`, `KOBurstAttempt 6 / Complete 4`
(the first run in the campaign to reach KOs at all).

| row | change | evidence so far |
|---|---|---|
| shield freeze | the missing `syTaskmanResetGraphicsHeap` | **NO-FREEZE, 11,179 frames** |
| FGM 153 wrong trigger | `alt_warning` read **+3500** for Dream Land's **-2900** | source-exact |
| VFX "extremely pixelated" | every named effect 8x8 -> **16x16**, same 8,192 bytes | checker PASS |
| particles "turn flat at end of lifetime" | `primcolor.a` was never submitted | `AlphaBreaks 16,206` |
| "right side compresses VFX" | vertex rail at 2047.9 world units | flag flipped, soak clean |
| star-KO twinkle misplaced | same rail | owner's eye owed |
| crowd cues cut off | release now starts at the audible end, not the note end | `PoolExhaust 0` |
| rolling dodge too loud | FGM 11 68 -> 48 (-8.4 dB total) | pack checker PASS |
| FGM 12 "too harsh" | 127 -> **90**, the cue's own `ucd_volume` 180/255 | pack checker PASS |

**`gNdsTaskmanGeneralHeapFreeMin` went 24,404 -> 144,336.** That is the DL-buffer
reclaim plus an anim cache that can now actually hold its arena. **The arena
margin that blocked the crowd actor and the shield rim is no longer thin — go
re-price both before calling either blocked.**

**The anim-cache arena is CLOSED as a red herring, with numbers.** The new
counters answer it outright: `OverflowLastUsed 92,160` against `ReservedBytes
92,160` and `OverflowLastSize 896` — the arena is genuinely full and refusing an
896-byte store, which is the designed degrade, not a defect.

**`alt_warning` is the shape to remember.** `ndsRelocApplyWordByteSwap` swaps
every 32-bit word, which EXCHANGES the positions of two `s16` sharing one.
`MPGroundData` undid that with an `if (min > max) swap` heuristic that never
touched `alt_warning` at all — and because `alt_warning` shifts the rest of the
run by one `s16`, the team fields do not pair top-with-bottom in a word either.
Replaced by the exact inverse (swap both halves of every word across the two
`s16`-only runs) with `_Static_assert`s pinning the layout. **A heuristic that
recovers the cases somebody checked is not a decoder.**

**`NDS_R2_PARTICLE_V16_HEADROOM` had been sitting at 0 with a positive
measurement** — 1,118 of 5,590 quads a match railed at ±2047.9 world units. A
railed vertex is not a misplaced one: x clamps while y does not, so the quad is
SQUASHED, which is the owner's "right side of stage compresses VFX" verbatim.
**Second time in a fortnight** that a measured working fix was invisible behind a
default of 0 (the fixed sqrt, 2026-07-31). **Audit the 0 flags whenever a symptom
matches one of their descriptions.**

**The quad sheet now spends its bytes on resolution, not frames.** Swept on the
host inside the unchanged 8,192: long cell 8 at cap 2 admits 33 in 8,000 texels
with DustDash/DamageFire/DamageNormal* at 8x8; long cell 16 at cap 1 admits 32 in
6,592 texels with all of them at **16x16**. No cap from 6 down to 2 seats the
live set at 16x16, so it is genuinely 8x8x2f against 16x16x1f. The search tries
resolution first now, so nothing is pinned. **Never grow the ALLOCATION** —
16,384 and 32,768 each broke stage/interface resolves with VRAM free.

**61,440 bytes came back to `gSYTaskmanGeneralHeap`.** DL buffers 0/1 reserve
81,920 and use 16 (the `gSPEndDisplayList` the reset writes); they are sized for
the RSP/RDP pipeline the DS renderer replaced. Now 16,384/4,096, graphics heap
back at the source's `0xD000`. That should lift the free-space low-water (24,404)
clear of the 25,600 `ifCommonSetMaxNumGObj` latch — **which is the arena margin
the crowd actor and the shield rim were blocked on. Re-price them.**
**Do NOT set a DL buffer to zero**: `syTaskmanCheckBufferLengths` tests
`start + length < head` for all four and the reset writes into each, so zero
hangs at taskman.c:338 instead.

## Still open, with the next step named

- **Respawn platform** — source is `dEFCommonEffects3_RebirthHalo`, a four-node
  DObjDesc chain reusing the MBallRays lists (node[1] at `(0,-60,0)`, node[2]
  spinning `rotY 0->2π` over 30 frames). The port draws a procedural disc. Blocked
  on the same seam as the shield: **the battle hardware path does not submit
  source effect DL links**. That seam closes both rows.
- **Shield VFX** — `gSPVertex` loads FOUR vertices, so the source shield is a
  textured billboard QUAD, not a sphere and not a disc; its texture is reloc file
  `0xa3` (DL at 0x248, DObjDesc at 0x300). The quad sheet now has ~1,600 free
  texels, which fits a 32x32 cell — routing the shield through
  `ndsRendererSubmitParticleQuad` would give it the source's soft rim with no new
  vertex, command or VRAM budget. That is the cheapest route and it is untried.
- **KO VFX** — owner: *"not all vfx are played or its too low quality"*. KO cells
  are still capped at 8x8 (`QUAD_KO_CELL_MAX`); at 16 the sheet drops texture 25,
  which is live. Ask which effect is absent before spending the sheet.
- **Crowd cue cut-off** — the release fix is unverified against the owner's own
  example (FGM 360 during a big hit). Note that every counter that "cleared"
  channel contention was collected on runs where `gNdsKOBurstAttemptCount` was 0,
  i.e. runs that never reached a KO. **A run that never reaches the trigger
  proves nothing.**

## Traps that are still live

- **`soak-freeze-watch.ps1` only covers gameplay for `time_limit` minutes.** It
  was 1, so a 7-minute soak watched six minutes of Results and read NO-FREEZE
  having exercised less play than the 3.5-minute run that caught the original
  hang. `NDS_R2_BOTH_CPU` now seeds 7. Read the script's own NOTE line.
- **The anim-cache arena is fine and was a red herring**: `ReserveCount 3`,
  `ReserveFail 0`, `ReservedBytes 92,160`, mismatches 2 (ordinary second entry).
  `Overflows 126` beside `UsedBytes 3,728` is not self-consistent, which is why
  `OverflowLastSize`/`OverflowLastUsed` now exist. Do not re-derive this.
- **Adjacent volatile stores are not an atomic publish** — `REG_IME = 0` around
  any counter group a halted reader must see whole.
- **A value that prints `0.000000` but tests `!= 0` is a DENORMAL**, i.e. a
  missed byte swap. Print `%e` or the raw bits.
- **gdb aborts a command file on the first missing symbol, silently.** Both the
  soak and `capture-sudden-death-entry.ps1` ask `nm` and strip absent reads.
- **`addr2line` names deleted AND inlined functions.** **`pwsh`, never
  `powershell`** (`lib/melonds.ps1:349` is a PS7 ternary).
- **A counter with no compiled writer reads 0, which looks clean.** Every probe
  needs an engagement count or a positive control.
- **`--gc-sections` had already discarded the particle textures** — check the
  `.map` before believing a size claim about linked data nothing reads.

## Refuted — do not re-derive

**The shield freeze as heap exhaustion** (the July fix was real and fixed a
different class); **the graphics-heap overflow as a per-frame high-water**
(refuted by scaling the ceiling); **"a source effect costs ~5 DObjs"**; **L7 as
"convert `gmCollisionSetInvertMatrix`"** (wired, measured, reverted: +6,481
cycles/frame of placement against a 534 win); **wrapping a decomp function to
count its INTERNAL callers** (the rename bypasses the wrapper and
`--gc-sections` deletes it); **"the Whispy dust spawns in the wrong place"**
(measured source-exact at both table entries); **E51 `line_id`**; **the pose
table (E61)**; **`.text.hot` (E66)**; **an AObj pool (E12)**; **the atlas
allocation-order theory**. **STOP ACCUMULATING SMALL LOAD-FRAME CUTS** — E11
removed real work and P95 still rose 15,744.

## Restart

Branch `codex/r2-runtime2`, not merged to master. Boundary
`battle_playable_realtime`, mode `163`.

```powershell
$env:DEVKITPRO = 'C:/devkitPro'; $env:DEVKITARM = 'C:/devkitPro/devkitARM'
.\scripts\verify-all.ps1 -Profile Boundary -List; git status --short
```

**Do not rebuild `smash64ds.nds`** (owner, 2026-07-28); **do rebuild the tick-HUD
ROM whenever the published one is** (owner, 2026-07-22), flag-identical.
`-j`/`MAKEFLAGS` rules are in `AGENTS.md` `## Builds`. A clean checkout must build
through `build.ps1`, not bare `make`: four of six generated `.inc` files are
gitignored. Preserve canonical mode 163, renderer mode 9, mip 0, static textures,
source countdown, Dream Land water at frame 0, Task 16 `1/1/1`. Do not edit
`decomp/`.

```powershell
# freeze class, now with real gameplay for its whole duration
.\scripts\soak-freeze-watch.ps1 -Build build-r2-bothcpu -MinutesToRun 7 -PollSeconds 5
# KO / star-KO / rebirth-halo captures, no per-frame stops
.\scripts\probe-ko-vfx.ps1
# announcements
.\scripts\capture-sudden-death-entry.ps1 -CaptureAnnounce 20   # TIME UP
```

Widest relevant verifier for the current candidate is `verify-current.ps1
-Build`, not Boundary: `scVSBattleStartScene` and the reloc loader are shared
startup. Run `New-Smash64DSSnapshot.ps1` last, and nothing after it.
