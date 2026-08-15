# RAM_RECOVERY_PLAN Phase 2 — the framebuffer collapse, and the inert ftmain patch

Date: 2026-08-15 · branch `codex/r2-runtime2` · commits `8cfbc2eaa2b`, `6e93def43cd`
Parent evidence: `../2026-08-14_native-battle-kernel/BATTLEPACK_ANIMATION.md` §6, §11, §12
Owning docs: `docs/RAM_RECOVERY_PLAN.md` Phase 2 · `docs/architecture/RUNTIME2_NATIVE_BATTLE_KERNEL.md` §6

## 0. Outcome

```text
gSYFramebufferSets   [2][230][320] 294,400 B  ->  [1][231][320] 147,840 B
MEASURED recovery                                        146,560 B
  bss        1,453,544 -> 1,306,984   (-146,560, exact)
  text         985,140 ->   985,140   (0)
  data         148,288 ->   148,288   (0)
  fake_heap_start 0x02269ee4 -> 0x02246264   (-146,560)
  proven boot headroom  174,368 -> 320,928
arm: build-tick-hud-buckets / smash64ds-battle-playable-tickhud-hwtri, SAME
     build directory both arms, only include/sys/video.h +
     src/import/battleship_sys_framebuffer.c + the mntitle patch changed
```

The ftmain patch is **inert**: exactly one symbol in the whole binary changes
size (`battleship_ftMainSetStatus`, −8 B), the deleted instructions are a
post-call reload of `fp->figatree_heap`, and both arms then execute the same
store. Boundary is **GREEN on both arms**, 0 `Exception:`, marker capture 27.8 s
and 27.5 s of a 120 s ceiling.

Root ROMs unchanged across the cycle:
`smash64ds.nds` `54c07fac80c50418949908701f7c2bdbf27512c5f96ac09086fabbb0df6ac68a`,
`smash64ds-battle-playable-hwtri.nds` `2015fbd1f68b81c03626d8c6d473c8bcbcf527a3a26dfe86ff19bd74ecbb1360`.

## 1. The reader set, from the linked ELF

Not from grep. Every word-aligned 32-bit literal in every `SHF_ALLOC` section of
`smash64ds-battle-playable-hwtri.elf` (HEAD `7d72f38085f`, object at `0x021aff30`
size `0x47e00`) whose value lands inside the object was enumerated and attributed
to its owning symbol. Unaligned matches were discarded — at 294,400 bytes of
address space a byte-granular scan is mostly instruction encodings.

| owner | what it does | affected by the collapse |
|---|---|---|
| `ndsBaseLBTransitionSetupTransition` | **the only READ of the storage**, through `gSYSchedulerCurrentFramebuffer`. VS Results photo wipe. Reached only from `mnVSResultsFuncStart`. | span re-derived below; stays fully in range |
| `ndsBaseSCManagerRunLoop` | **the only WRITE**: the startup clear, bounded by `sizeof(gSYFramebufferSets)` (`src_sc_scmanager.patch`) | shrinks with the extent automatically |
| `ndsBaseSCVSBattleStartScene` | `arena_size = &gSYFramebufferSets − &ovl4_BSS_END` — address arithmetic, never a dereference; the port overwrites that field with `ndsTaskmanArenaSize()` (`battleship_scvsbattle.c:101`, `:139`) | no |
| 16 × `*StartScene` | store `SYVIDEO_ZBUFFER_START(320,240,0,10,u16)` = `gSYZBuffer − 6,400` into their own `SYVideoSetup` | see §4 |
| the `SYVideoSetup` initializers | data only; every slot `&gSYFramebufferSets[0]` after the mntitle patch | no |

`gSYSchedulerCurrentFramebuffer` itself has exactly **four** references in the
ELF: `sySchedulerSetNextFramebuffer` (assign), `sySchedulerThreadMain` (assign),
the wipe's read, and `lbTransitionSetupTransition`'s NULL fallback
(`src/import/battleship_lbtransition.c:47-50`).

Confirmed **not compiled** in the battle ELF, so they are not readers here even
though decomp names the symbol: `scstaffroll.c:2336`, `mncongra.c:429`,
`sc1pstageclear.c:2126-2129`, and `mvopeningroom.c:1903` (that one sits inside
the file's own `#else` arm, lines 737–2153).

## 2. The span, re-derived from the compiled reader

The previous cycle's `base+7,060 .. base+147,819` was a doc comment. It is now
re-derived from the wipe's own literals in the linked ELF, and it is correct:

```text
ldr  r1, [0x021afbcc]          ; gSYSchedulerCurrentFramebuffer
adds r1, r1, #0x00023f14       ; +147,220 = BORDER(320,10)+BORDER(320,220)+BORDER(1,10)
inner  ldr r2,[r1,r3]  r3 = 0..596 step 4        ; 150 words = 600 bytes per row
outer  adds r1, r1, #0xfffffd80                  ; -640 = one row BACKWARD
       trip count 220  (heap advance 0x203a0 = 300*220*2, at 600 B per row)

highest byte read  147,220 + 599        = 147,819
lowest  byte read  147,220 - 640 * 219  =   7,060      (both INCLUSIVE)
```

The object must therefore span offsets `0..147,819` = 147,820 bytes = 230.97
rows, rounded up to **231 whole 640-byte rows = 147,840 B**. Loads are 32-bit and
`147,220 % 4 == 0`, so the base must stay 4-aligned; the definition now carries
an explicit `__attribute__((aligned(4)))` rather than relying on the linker
default that happened to hold.

`[1][231][320]` was chosen over `[231][320]` deliberately: it keeps
`&gSYFramebufferSets[0]` a pointer to a whole *buffer*, so a stray `[1]`/`[2]`
is out of range instead of silently overlapping rows 1 and 2 of the live buffer.

## 3. The one thing the collapse broke, fixed at the same seam

`mntitle.c:125-127` hardcoded `&gSYFramebufferSets[0]`, `[1]`, `[2]` — the
"KNOWN GAP" the old header carried. `[1]` and `[2]` no longer exist, so
`scripts/decomp-patches/battleship/src_mn_mncommon_mntitle.patch` aliases all
three to `[0]` under `SSB64_TARGET_NDS`, which is what `SYVIDEO_SETUP_DEFAULT()`
already does for every other scene. Safe for the reason the header states:
`sys/scheduler.c` only *assigns* `gSYSchedulerCurrentFramebuffer` and never
compares the buffers against each other. Verified in the rebuilt ELF —
`dMNTitleVideoSetup` now reads base in all three slots.

## 4. The one hazard the collapse created, recorded rather than hidden

`gSYZBuffer` still immediately follows the object, so
`SYVIDEO_ZBUFFER_START(...)` = `gSYZBuffer − 6,400` moved from `base+288,000`
(the dead tail of old buffer `[2]`) to `base+141,440` — **inside the wipe's read
span**. It is never dereferenced: cycle 84 measured all 6,400 of those bytes
still holding the `scmanager` clear value mid-battle, against a control that
moved in the same run (`src/import/battleship_sys_zbuffer.c`). So this is inert
today. It is recorded on the extern because the *consequence* of a future
regression there changed from "corrupts nothing" to "corrupts the Results photo".

## 5. Visual evidence — and what it does NOT cover

`artifacts/visibility/2026-08-15_framebuffer-collapse/`, captured with
`capture-results-tic.ps1` on `smash64ds-results-lab-hwtri`
(`build-c161-results-fb`), which boots straight into VS Results. Both arms
built in the **same directory**; the only difference between them is the array
extent (the ftmain patch is present in both).

```text
results-tic160-{before,after}.png   PNG SHA256 BYTE-IDENTICAL
                                    7a50c54b2a61de1d46640ae3179911313596c468d29f6dac9a5d694e490ab1fc
results-tic60-{before,after}.png    guest content identical; the files differ
                                    only in melonDS's own title-bar FPS readout
                                    ([295/60] vs [292/60])
results-tic20-before.png            kept as the calibration shot: at tic 20 the
                                    Results scene has not drawn yet
```

**NEEDS THE OWNER'S EYE, and this is not marked FIXED.** The captures show the
Results *screen* unchanged. They do **not** show the photo wipe animating: the
results lab boots directly into Results, and neither sampled tic has a
transition in flight. What is proven instead is that the wipe's *source bytes*
cannot have changed — its read span is entirely inside the new object, and the
linked ELF shows exactly one writer, a constant clear that self-bounds to
`sizeof`.

## 6. The ftmain patch — inertness, proven twice

`scripts/decomp-patches/battleship/src_ft_ftmain.patch` replaces
`ftmain.c:4623-4624`'s discard-and-hardcode with
`fp->figatree = lbRelocGetForceExternHeapFile(...)` under `SSB64_TARGET_NDS`.

**Source proof.** `lbRelocGetForceExternHeapFile`
(`src/port/reloc_backend_assets.c:7286`) returns exactly `heap` on every path
when `heap != NULL`:

- Mario/Fox arm: `(file != NULL) ? file : heap` over
  `ndsRelocForceLoadFighterAObj16File`, whose complete return set is `{heap, NULL}`.
- Generic arm: `lbRelocGetExternHeapFile` returns `heap` on every early exit and
  `loaded->data` on success; the wrapper then copies that into `heap` and
  rewrites `file = heap`. `ndsRelocFindLoadedFileByData` cannot miss, because
  every non-NULL file `ndsRelocLoadExternTreeAsset` can return is an element of
  `sNdsRelocLoadedFiles[]` by construction.
- It returns NULL only when `heap` is NULL — the same value the N64 line stores.

**ELF proof**, same build directory, this patch the only change:

```text
exactly ONE symbol changes size in the whole binary
  battleship_ftMainSetStatus   0x8e4 -> 0x8dc   (-8)
every other symbol keeps its size

deleted:  movs r3, #157 ; lsls r3, r3, #4 ; ldr r0, [r4, r3]
          (0x9d0 = NDS_FTSTRUCT_OFF_FIGATREE_HEAP 2512 -- the post-call reload)
          plus one alignment nop                          = -8 bytes
both arms then fall into the SAME  str r0, [r4, #FIGATREE]  tail
.main -8 · no bss change · fake_heap_start unchanged (absorbed by alignment)
```

Sections compared, not the `.nds` (NitroFS packs directory entries
nondeterministically). `.crt0`, `.itcm`, `.text.hot`, `.text.hot.draw`,
`.main.rw` also differ — all branch/literal re-encoding downstream of the −8;
the symbol-size comparison is what isolates the change to one function.
Full disassembly diff in `ftmain-codegen-diff.txt`, section table in
`sections-taskB-vs-taskA.log`.

**Gate arm unmoved.** Boundary's realtime smoke counters are identical
before/after the patch — `gxram`, `ftrContract`, `ftrTri`, `batch`, `submit`,
`lazy`, `mtx`, `binds`, `vtx`, `tri`, `intrinsicM3/M4`, `water`, `fastWall`
hash, `cpu`, `aobj32`, and `stageAcct frames=212` on both. Only the
wall-clock-windowed fields move (sampled `frames` 212 → 211, and `ticks` with
them, at a constant per-frame cost).

Consumer safety for the day a const-clip path returns a non-heap pointer:
`fp->figatree` is read at `:4628` (a NULL test) and `:4704` **only** — verified
across the whole tree, not just this file; the sole other reader is
`ftcommoncapturekirby.c:177/179`, the same NULL-test-then-attach pair — and the
port's `lbCommonAddFighterPartsFigatree` already resolves every entry against
the base pointer it is handed (`ndsRelocResolvePointerFromFileBase`,
`reloc_backend_compat_shims.c:9060`).

## 7. CORRECTION — what +146,560 actually buys, and it is not "the full pack fits"

`BATTLEPACK_ANIMATION.md` §11.4(c), architecture §6 and the board all say the
recovery makes the **full** 297-clip pack fit by 6,536. That arithmetic is
against the **combined 511,904** pool, and §11.1(b)'s own finding — that the
three pool terms are **not additive** without a coupled three-file change — is
untouched by this cycle. Both statements are in those documents; only the first
one got quoted.

Measured static headroom, stated with its arm (the earlier 211,936 was
`build-battle-playable-proof-hwtri-harness`, which is neither the shipped ROM
nor the measuring ROM, and today reads 208,672):

| arm | `fake_heap_start` | headroom before | after Phase 2 |
|---|---|---:|---:|
| published `smash64ds-battle-playable-hwtri` | `0x02260724` | 213,216 | **359,776** (derived) |
| `build-battle-playable-proof-hwtri-harness` | `0x022618e4` | 208,672 | 355,232 (derived) |
| `build-tick-hud-buckets` (measuring) | `0x02269ee4` → `0x02246264` | 174,368 | **320,928** (measured) |

"Derived" means: the bss delta is a property of the object and is identical in
every configuration, and text/data did not move, so `fake_heap_start` drops by
exactly 146,560 in every target. Only the tick-HUD arm was built both ways.

```text
pack draws on...                     before Phase 2   after      full 651,928   items-off 553,696
.rodata (static headroom, published)      213,216    359,776      short 292,152   short 193,920
the taskman arena alone                   299,968    299,968      short 351,960   short 253,728
combined (needs the 3-file coupling)      513,184    659,744      FITS by 7,816   FITS by 106,048
```

So: **Phase 2 delivers exactly the 146,560 B it promised, and it does not on its
own make the pack fit any single pool.** The combined row still requires cutting
`NDS_TASKMAN_ARENA_SIZE`, lowering the `0x130000` search floor
(`diagnostics.c:7810`), and reteaching the Task 36 replay-admission guard
(`nds_renderer.c:5734-5739`) — the same coupling §11.1(b) named. Phase 2 does
make that coupling *worth doing*, because the freed 146,560 B is exactly the
libnds-heap slack a larger arena would draw on. Sizing which pool the pack lands
in is phase 3's decision, not this cycle's.

## 8. What this cycle did NOT do

- **No slice 1 phase 5.** The two blockers are cleared; the direct runtime
  instance is untouched.
- **No performance measurement.** No gate run, no tick series. The **−73,659**
  at rank-80 remains a projection on the profile arm and is not banked.
- **The photo wipe's animated frames were not captured** (§5). The Results
  screen is byte-identical; the wipe itself still needs the owner's eye.
- **`docs/RAM_RECOVERY_PLAN.md` was not edited** — this landing is recorded
  here, in the architecture doc, on the board and in HANDOFF instead.
- No published ROM was rebuilt; both root ROMs are byte-unchanged.
