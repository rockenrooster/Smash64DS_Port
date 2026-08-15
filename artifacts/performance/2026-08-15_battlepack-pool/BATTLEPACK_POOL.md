# The pack does not need one pool — it needs one pool per fighter

**Date:** 2026-08-15 · **Branch:** `codex/r2-runtime2` · **base HEAD `eed61527bf1`**
**Native Battle Kernel slice 1, Task A (pool) and phase 5 (direct `const` clip path).**
Predecessors: `…/2026-08-14_native-battle-kernel/BATTLEPACK_ANIMATION.md`,
`…/2026-08-15_framebuffer-collapse/PHASE2_FRAMEBUFFER.md`.
**Builds spent: 1 lab arm + the Boundary arm.** Root ROMs unchanged (§6).

---

## 0. Outcome first

```text
THE POOL IS THE ARENA, NOT .rodata -- and that is a MEASURED reversal of what
this document said four hours earlier.  A `.rodata` blob costs the taskman
arena as well as static space, because the arena is calloc'd from the same
libnds heap `fake_heap_start` bounds and its search silently steps down.

MEASURED      +288,992 B of static image  ->  gNdsTaskmanArenaChosenSize
              0x150000 -> 0x140000, gNdsTaskmanArenaAllocFailCount 16
              (soak capture 2026-08-15_011536, `TASKARENA=1310720,16`)
CONSEQUENCE   the arena lost 65,536 B.  Against the match's measured
              high-water use of 1,304,068 that projects a general-heap
              low-water of ~6,652 against the mandated 32,768 floor.
              NOT SHIPPABLE.  `NDS_R2_BATTLEPACK` therefore lands DEFAULT 0.
THE FIT       one fighter, resident IN THE ARENA, replacing the 262,144 B
              raw-file anim cache: Mario 271,728 (+9,584 of slack) or
              Fox 287,904 (+25,760).  Zero static growth, so the arena stays
              0x150000 and `check-boot-headroom.ps1`'s ladder is untouched.
PHASE 5       the seam is built and the format is proven; what is NOT built is
              the setup-time load that puts the blob in the arena.
ALSO FOUND    the pack was shipping DISK bit order.  Root-caused from a data
              abort, fixed, and the equivalence test that missed it is fixed
              too -- see §7.
```

### The retraction, in full

This document first argued that `plan.md` §K-RAM's three-file coupling was
unnecessary because "the two pools pay independently", so
`feasible(P) ⟺ P ≤ H_static + F_arena`. **That is wrong, and the arena counter
says so.** The taskman arena is one `calloc` from the libnds heap, and that heap
runs from `fake_heap_start` to a fixed top — so `.rodata` growth and arena size
come out of the *same* bytes. Growing the image by 288,992 B pushed the arena's
downward search 16 steps of 0x1000.

What survives the retraction:

- **Freeing the anim-cache arena still releases no libnds heap** — the original
  non-additivity finding (`BATTLEPACK_ANIMATION.md` §11.1(b)) stands.
- **Arena-internal free is genuinely free of the static budget**, because the
  `calloc` is already made. That is why the arena is now the answer.
- The per-fighter split still costs only **136 B** of cross-fighter dedup
  (measured), so one-blob-per-fighter remains the right shape.

**And the boot-headroom ladder does not cover this.** It records boot/no-boot,
not arena size. The failing arm here had **66,784 B of PROVEN headroom** and
still degraded the arena — so `check-boot-headroom.ps1` is necessary and not
sufficient, and any future static growth must read
`gNdsTaskmanArenaChosenSize` as well. That is now in the soak's global list.

---

## 1. The pools, measured on this HEAD

`check-boot-headroom.ps1` reports the distance from a build's `fake_heap_start`
to `0x02294804`, the highest address **proven** to boot. The unproven band above
it is **800 bytes wide**, so this is a near-bisected cliff, not a guess.

| arm | ELF (date) | `fake_heap_start` | proven static headroom |
|---|---|---:|---:|
| proof = published configuration | `build-battle-playable-proof-hwtri-harness` (08-15 00:14) | `0x0223dc44` | **355,264** |
| tick-HUD (the measurement instrument) | `build-tick-hud-buckets` (08-15 00:10) | `0x02246264` | **320,928** |
| published root ROM — **stale, pre-Phase-2** | `smash64ds-battle-playable-hwtri.elf` (08-14 12:58) | `0x02260724` | 213,216 |

**The tick-HUD arm binds**, not the shipping arm: every gate figure is taken on
it, and it is 34,336 B larger statically. `docs/HANDOFF.md`'s "published reads
213,216 → **359,776**" is a *prediction* — the published ROM has not been rebuilt
since Phase 2 — and the measured post-Phase-2 published-configuration number is
**355,264**.

Arena-internal free, from `HANDOFF` and `reloc_backend_assets.c:6377`:

```text
NDS_R2_ANIM_CACHE_ARENA_BYTES        262,144   reclaimed outright by residency
gSYTaskmanGeneralHeap free-min        72,188   less the 32,768 mandated floor
                                    --------
arena-internal free                  301,564
```

**The two are not one pool.** The arena is a single `calloc` of
`NDS_TASKMAN_ARENA_SIZE 0x150000` (`diagnostics.c:7750`, `:7791-7849`), so
releasing the anim arena frees no libnds heap and cannot move `fake_heap_start`.
That finding (`BATTLEPACK_ANIMATION.md` §11.1(b)) stands; only the conclusion
drawn from it does not.

### Why not the coupled route

The alternative — pack the whole 553,696 B into `.rodata` and pay for it by
cutting `NDS_TASKMAN_ARENA_SIZE` — was priced and rejected:

1. It needs the arena cut by ~233,000 B, which puts it **below the `0x130000`
   search floor**, so the floor moves and the Task 36 admission guard
   (`nds_renderer.c:5735-5739`) must be retaught a third constant.
2. **It invalidates the boot ladder.** Every one of the eight `(address, boots)`
   rows in `check-boot-headroom.ps1` was measured with a `0x150000` arena. Cut
   the arena and the cliff moves up by the cut, so the script — the project's
   only defence against a failure that "reads exactly like a hung emulator, and
   has now cost four builds" — reports OVER CLIFF for every candidate and has to
   be re-bisected with real boot probes before any measurement is trustworthy.
3. The split keeps every ladder row valid, because it changes neither
   `NDS_TASKMAN_ARENA_SIZE` nor any other libnds-heap consumer.

### Was another `RAM_RECOVERY_PLAN` phase cheaper?

Asked and answered before committing to the split. Against the items-off pack's
**553,696 B in one pool**, `.rodata` alone is short **232,768** on the binding
arm. Phase 1 (DL preview pair, 21,600) and Phase 3 (decode-cache extent, 18,592)
are already spent; Phase 3's ~150 KiB premise is refuted in its own text; Phase 4's
scratch overlay is refuted with evidence. **No remaining phase closes 232,768**,
and Phase 5 (arena consumer audit) is an unsized audit. The split needs none of
them.

### The reserve this leaves, and why it is sufficient

`PROJECT_GOAL.md` requires "sufficient reserve … for reliable operation";
`RAM_RECOVERY_PLAN` invariant 3 fixes the general-heap floor at 32,768.

```text
static, proof/published arm      66,816   MEASURED on build-c162-battlepack
static, tick-HUD arm            ~31,968   PREDICTED (320,928 - 288,960); must be
                                          measured before the next gate run
general-heap low-water           72,188   UNCHANGED -- nothing moved in the arena
```

At the end state (Mario also resident) the arena allocation grows 262,144 →
271,728, i.e. **+9,584**, taking the general-heap low-water to ~62,604 against
the 32,768 floor. Neither pool is spent to zero.

---

## 2. The pack, re-measured and re-shaped

The BPA1 pack of `BATTLEPACK_ANIMATION.md` §5 is a *clip* pack: its offset table
carries one entry per **distinct script**. The consumer needs one entry per
**DObj slot** — `lbCommonAddFighterPartsFigatree`
(`reloc_backend_compat_shims.c:9044`) advances one word per DObj, and several
DObjs share one script. Emitting the clip pack directly would have handed the
walk a shorter table than it reads. **This was found by reading the consumer, and
it is the reason the emitted format is BPA2 rather than the banked blob.**

BPA2, one blob per fighter, `--items-off` (the 38 clips proven unreachable from
the linked battle ELF, `BATTLEPACK_ANIMATION.md` §11.2):

| blob | clips | slots (linked / null) | bytes | sha256 |
|---|---:|---|---:|---|
| Fox `0x282..0x31e` | 137 | 3,611 (2,713 / 898) | **287,904** | `f6a49219a32583f4…` |
| Mario `0x1f3..0x280` | 122 | 2,976 (2,188 / 788) | **271,728** | `c97284811b687cb9…` |
| **total** | **259** | **6,587** | **559,632** | |

+5,936 over the single-blob BPA1 items-off 553,696: the per-slot tables cost
26,348 B where the per-script table cost 19,604, and the split itself costs 136.

**Stray slots: 0.** Every slot with no relocation holds a zero word in all 259
clips, so emitting 0 for them is exact, not an approximation. That was a real
open question — `reloc_backend_assets.c:2849` records a figatree slot holding "a
plain data word rather than a relocated pointer" as the 2026-08-02 shield-freeze
mechanism — and the generator counts it rather than assuming it.

### The walk cannot run off the end of a slot table

`lbCommonAddFighterPartsFigatree` advances one word per DObj and stops when the
tree does, not when the table does, so a table shorter than the tree would read
past it. Measured over the Fox blob rather than assumed:

```text
26 slots : 91 clips     27 slots : 43 clips     28 slots : 3 clips
```

91 clips carry exactly 26, so the walk consumes at most 26 entries or those 91
would already over-read in the shipped ROM. Every clip has at least 26.
(The 26/27/28 spread is a property of the source files: the table bound is
*derived* as the first script's offset, so unused trailing slots vary.)

### Slot-level equivalence: mismatch 0

`--blob-out` runs it before writing, so a bad pack cannot reach a build:

```text
Fox    3,611 slots (2,713 linked, 898 null)   34,523 commands   MISMATCHES 0
Mario  2,976 slots (2,188 linked, 788 null)   32,253 commands   MISMATCHES 0
```

Each slot is decoded **out of the emitted blob, at the blob's own offset**, and
compared command-for-command against the same slot decoded from the o2r file
through the ROM's own pipeline (`ftanim_reloc_probe`). This is a different
statement from §7's clip-level check and is the one the runtime depends on: §7
proved the deduped stream reproduces every distinct script; this proves slot *i*
of clip *c* names the same script the file's relocation chain named for it.

The blob generated from `decomp/…/BattleShip_o2r/reloc_animations` is
**byte-identical** (`6e47f430…`) to the blob generated from the shipped bank
`builds/build-c158-gate/nitrofs/reloc/reloc_animations`, which independently
confirms `BATTLEPACK_ANIMATION.md` §5's claim that the Mario/Fox members agree.

---

## 3. Phase 5 — what was actually deleted

`src/port/reloc_backend_assets.c`, `ndsRelocForceLoadFighterAObj16File`. On a
resident clip the function now returns before it does any of this:

```text
memcpy(heap, cached->payload, cached->size)      the whole payload, every time
ndsRelocRemoveFighterAObj16LoadedAliases(...)    strip the stale alias nodes
ndsRelocRegisterLoadedFile(...)                  re-register a loaded file
ndsRelocFinalizeLoadedFile(...)                  internal pointer fixups
                                                 AObj16 lane normalization
                                                 attribute + weapon normalization
                                                 external pointer fixups
                                                 battle-interface sprite pass
ndsRelocAssetGetPath(asset_id)                   the token -> path table lookup
```

What replaces it is a range test and a binary search over 137 entries.

**Three status writes stay** (`ndsRelocSetStatusBufferFile` ×2 and
`ndsRelocSetForceStatusBufferFile`), pointed at the pack. They are one table
store each, they are what `lbRelocGetStatusBufferFile` answers from, and keeping
them correct is cheaper than proving no reader exists.

### Why a `const` blob pointer is a legal figatree

Three properties, each from the code rather than from the design:

1. **`ftmain.c:4623` takes the return value.** The decomp patch landed
   2026-08-15 (`6e93def43cd`); before it the body hardcoded
   `fp->figatree = fp->figatree_heap` and no pointer this function returned
   could ever have been used.
2. **The resolver rebases against whatever base it is handed.**
   `ndsRelocResolvePointerFromFileBase` (`:2850`) treats a slot word that is not
   already inside a known file as a **byte offset from the containing file's
   base**. Registering the blob as that containing file — one `O(1)` range test
   added at the head of `ndsRelocFindKnownFileContaining` — makes a table of
   blob-relative byte offsets resolve with no fixups at all.
3. **The stream is position independent.** `OP_LOOP`/`OP_INTERP` targets are
   computed as `pc + 2 + half*2` (`ftanim_reloc_probe.py:186-194`), and the
   phase-4 equivalence compared every jump **relative to its script start** at
   mismatch 0. Nothing in a script names an absolute address.

The resolver **refuses** a result that is not 4-byte aligned — that refusal is
the 2026-08-02 shield-freeze fix — so every script start in the blob is padded
to 4 and the blob itself is `.balign 16`. Measured: the alignment cost zero
bytes, because every run was already a multiple of 4.

`fp->figatree` is read at `ftmain.c:4628` (a NULL test) and `:4704`
(`lbCommonAddFighterPartsFigatree`) and nowhere else; every other reader in the
port tests `fp->figatree_heap`, which is untouched.

---

## 7. The pack was shipping the wrong bit order — root cause, fix, falsifier

The first ROM built with the pack took a **data abort** on its first frame
(`rodata-arm-abort-capture.txt` beside this file — copied out of the rotatable
`artifacts/verification/freeze-soak/` because it is the primary evidence for
two retractions;
`cpsr` low bits `0x17`, PC parked in `__excpt_entry`). The discriminating line
in that capture was not a register — it was
`gNdsObjAnimRunawayScript=34458634` = `0x020DB60A`, **an address inside the
linked pack**, with `gNdsObjAnimRunawayOpcode=26` against an opcode maximum
of 14.

**Root cause.** The ROM's load pipeline has four stages, and stage 4 has three
parts: 4a derive the table bound, 4b unswap the u16 lanes, **4c re-encode every
command word from the disk's MSB-first `opcode:5, flags:10, toggle:1` into the
native C bitfield order** (`ndsRelocAObj16EncodeForNativeBitfields`,
`reloc_backend_assets.c:3313-3320`, driven by
`ndsRelocNormalizeAObj16Script` at `:3378-3402`). The host probe's
`normalize()` implements **4a-4b only** — its own docstring says so — and the
pack was built from its output. The direct path skips
`ndsRelocFinalizeLoadedFile` entirely, so nothing applied 4c at runtime either:
the parser read `opcode = w & 0x1f` out of an MSB-first word, walked off, and
aborted.

**Why phase 4's "mismatch = 0" did not catch it.** Both sides of that test were
decoded by the same host decoder, which reads disk order. The test was
self-consistent and still wrong about the runtime's format — a subtler version
of the tautology its own §7 was written to avoid. `decode_script` now takes
`native=`, the pack side is decoded in native order, and the check therefore
tests the format the ROM consumes.

**The fix**, in the generator, mirrors the C exactly — a *linear* walk by the
per-opcode step, rewriting command words only, terminating at `End` or on an
overrunning step, with `word_count` running to the next-higher table target.
The ROM normalizes each script exactly once, and that is only safe because
`len(set(fx.values())) == len(fx)` on every file in the bank (4c is not
idempotent); the generator now raises rather than packs if that ever stops
holding.

**Falsifier, and it can fail:**

```text
the emitted blob decoded in DISK   order: 75 of 2,713 scripts raise outright
the emitted blob decoded in NATIVE order:  0
re-verified after the fix: clip mismatch 0 (corpus 278775508583b68e…)
                           slot mismatch 0 over 3,611 slots / 34,523 commands
```

Blob sha256 `6e47f430…` → **`f6a49219a32583f46c1ccab32b0b11750d504e9cb38ed39724735c7e563facef`**;
the pre-4c hashes in §2 are superseded and are recorded only as the defect's
fingerprint.

---

## 4. What this cycle did NOT do

- **Nothing is resident, and `NDS_R2_BATTLEPACK` ships default 0.** The seam,
  the format, the generator and the equivalence proofs are landed; the
  setup-time arena load that the corrected pool answer requires is not written.
  Enabling the flag as it stands links the blob into `.rodata` and degrades the
  arena — §0.
- **The flag-1 arm never reached a green verifier, and the failure IS ours.**
  Three Boundary attempts on it timed out at marker 2
  (`ndsRendererHardwareArmBattleStaticTextures`), and one soak on the same ROM
  produced the data abort that root-caused §7. **The control ran: the same
  target at `NDS_R2_BATTLEPACK=0` is Boundary GREEN, zero `Exception:`** — so
  the timeout is not environmental and not pre-existing. The bit-order fix did
  not clear it, so **at least one more defect remains in the flag-1 path and it
  is unattributed.** Do not read §7 as the whole story. Prime suspects, in
  order: (a) the arena degradation itself starving battle-static-texture
  preparation, which is exactly where marker 2 lives — and which the arena move
  removes; (b) a reader of the figatree that still requires a registered loaded
  file, beyond the two seams already taught (`ndsRelocFindKnownFileContaining`,
  `ndsRelocPointerIsFighterAObj16`).
- **No gate measurement.** The **−73,659 at rank-80 remains a projection** on the
  profile arm and is not banked. Phase 8 is next cycle's.
- **`gNdsBattlePackHits` was never read on a live ROM**, so the acquisition path
  is proven deleted *in source* and not yet *in a counter*. Its readers are now
  in the soak's global list, which is where the next cycle should collect them.
- **No phase-6 oracle mode.** Phase 5 does not introduce a new evaluator: the
  same generic parser runs on the same bytes from a different address, and
  phase 4 already proved those bytes equal at mismatch 0. An oracle comparing
  old and new *evaluators* belongs with the evaluator slice.
- **The tick-HUD arm's headroom is predicted, not measured** (§1).
- **The blob's link placement is not tuned.** 287,904 B of read-only data landed
  at `0x020d8d80`, inside the text output section. It is never fetched as
  instructions, but it displaces code, which is exactly the ±8,544 cross-build
  placement term — relevant to the next arm's P95, not to correctness.

---

## 5. Reproduction

```powershell
# the two blobs, each with its own slot-level equivalence run
python scripts/generate_battlepack_anim.py --fighter fox   --items-off `
    --blob-out assets/animation/battlepack_fox.bin `
    --json artifacts/performance/2026-08-15_battlepack-pool/battlepack-fox-blob.json
python scripts/generate_battlepack_anim.py --fighter mario --items-off `
    --blob-out $env:TEMP\battlepack_mario.bin `
    --json artifacts/performance/2026-08-15_battlepack-pool/battlepack-mario-blob.json

make TARGET=smash64ds-battle-playable-proof-hwtri BUILD=build-c162-battlepack
.\scripts\check-boot-headroom.ps1 -Build build-c162-battlepack `
    -Target smash64ds-battle-playable-proof-hwtri
.\scripts\verify-all.ps1 -Profile Boundary
```

## 6. Root ROMs

Unchanged across the cycle — no published target was built.

```text
smash64ds.nds                          54c07fac80c50418949908701f7c2bdbf27512c5f96ac09086fabbb0df6ac68a
smash64ds-battle-playable-hwtri.nds    2015fbd1f68b81c03626d8c6d473c8bcbcf527a3a26dfe86ff19bd74ecbb1360
```
