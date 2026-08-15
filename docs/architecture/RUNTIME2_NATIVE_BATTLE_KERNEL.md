# Runtime2 — the Native Battle Kernel

Owner direction, 2026-08-14: stop treating Runtime2 as "BattleShip with
increasingly optimized adapters" and turn the DS path into a compiled battle
kernel. `plan.md` §1 carries the direction and the six-step order; this document
owns the kernel's *design*, and is the place a later cycle reads before touching
any of it.

Status: **slice 1 phases 1–4 complete (2026-08-14). No DS runtime path exists
yet.** Evidence: `artifacts/performance/2026-08-14_native-battle-kernel/BATTLEPACK_ANIMATION.md`.

---

## 0. The order, and why it is this order

```text
1  BattlePack / direct AnimClip          <- slice 1  (phases 1-4 done)
2  flattened process scheduler           <- only after slice 1 is measured
3  compact pose + dirty-joint evaluator
4  baked one-pass world matrices
5  direct native renderer
6  only then consider fidelity tradeoffs
```

The measurements force it. On the `NDS_R2_BOTH_CPU=1` gate arm the P95 excess is
spread across `icache_fill` +155,795 / `dcache_fill` +96,800 / `issue` +94,029
with **no function above 3.6%** — the signature of too much generic machinery,
not one slow function — and ~90% of it sits inside `gcRunAll`
(`…/2026-08-14_runtime2-p95-closure/GATE_ARM_OWNERS.md`).

## 1. The migration law — not optional

**The generic BattleShip-compatible path remains the correctness ORACLE and
fallback, never the normal DS hot path.** Every DS-native path first runs in a
verification mode where old and new both produce outputs and are compared, and
the shipping configuration stops running the generic half only after mismatches
are **zero**. Slice 1 phase 4 is the host half of that law, already satisfied
(§5). Phase 6 is the same law on the DS.

Corollary the owner stated explicitly: **do not ask the owner to debug
intermediate versions.**

## 2. K0 — the rule that defines slice 1

```text
AFTER THE BATTLE ENTERS GO:
  fighter-animation FAT reads            = 0
  get_fat / f_lseek for fighter anim     = 0
  animation payload byte-swaps           = 0
  animation-file relocation / fixups     = 0
  AObj16 file normalization              = 0
  raw animation-file cache copies        = 0
  token -> file discovery                = 0
```

Normal gameplay consumes `clip_id -> immutable DS AnimClip -> per-fighter
AnimInstance -> pose`.

**Do NOT merely enlarge the existing raw-file cache.** Measured on the gate arm:
the cache already serves **95.0%** of acquisitions (284 hits of 299), and a *hit*
still `memcpy`s the whole payload into caller-owned heap, registers a loaded
file, runs finalization and fixups, re-normalizes the AObj16 script region,
strips alias status nodes and writes three status-buffer entries. **The measured
cost is that per-acquisition work, not the file read** — see §4.

## 3. Immutable clip vs mutable instance — the inventory

Phase 2 required an inventory of *every* write the runtime makes into a loaded
fighter animation file, each classified precompute / move-to-instance /
eliminate. All line numbers are `src/port/reloc_backend_assets.c` unless noted.

| write into the loaded file | what it writes | verdict |
|---|---|---|
| `ndsRelocApplyWordByteSwap` :2956 | every u32 of the payload, byte-reversed | **precompute** — the pack stores native words |
| `ndsRelocApplyInternalPointerFixups` :2986 | absolute `data + target*4` pointers into the entry table, threaded through the intern chain | **eliminate** — the pack stores u32 *word offsets*, so nothing needs patching after load |
| `ndsRelocApplyExternalPointerFixups` :3746 | external asset pointers | **not applicable** — measured `extern_file_ids_num == 0` on all 301 animation files |
| `ndsRelocNormalizeFighterAObj16File` :3387 (a) | derives `table_bytes` as the smallest resolved pointer (read, then drives the rest) | **precompute** — the pack carries an explicit `script_count` |
| … (b) :3439 | swaps the two u16 lanes of every word from `table_bytes` to end | **precompute** |
| … (c) :3488 | re-encodes every command header word from disk MSB-first order to the native bitfield order | **precompute** |
| `ndsR2AnimPrebakeAObj16` | applies (c) once per warm entry | **precompute** — this is the half-step the current cache already takes |
| `memcpy(heap, cached->payload, cached->size)` :7207 | the entire payload into caller-owned heap | **eliminate** — the clip is shared and `const` |
| `memset(heap, 0, asset_size)` :7280 (fail path) | zeroes the destination | **eliminate** with the destination |
| `memset(dst, 0, alloc_size)` `nds_reloc_assets.c`:550 | zeroes the aligned payload before the read | **eliminate** with the read |
| `ndsR2FtAnimParseDObjFigatree` `battleship_ftanim.c`:492 | **nothing** | **already immutable** |

**The last row is the finding that makes slice 1 safe, and it was verified rather
than assumed.** Across 26 mentions of `event16` in the parser there is not one
assignment through it; every write targets the `AObj`, the `DObj` or the `GObj`.
`AObjAnimAdvance` (`decomp/.../sys/obj.h:36`) is `p++` — a cursor bump, not a
store.

**So `NDSAnimInstance` does not have to be invented.** The mutable playback state
already lives outside the file, in the `DObj`/`AObj` pair the engine already
owns: `anim_joint.event16` (the cursor), `anim_wait`, `anim_frame`, `anim_speed`,
and the per-track `AObj` chain. Slice 1 therefore does **not** need a new
instance object; it needs the cursor to point into a shared `const` clip instead
of into a private per-fighter copy.

**Do not assume the raw file must be copied merely because the current adapter
copies it.** The adapter copies because `ndsRelocApplyInternalPointerFixups`
writes *absolute* pointers into the image, which makes a fixed-up image
position-dependent (`reloc_backend_assets.c`:6222-6228 says so). Remove the
absolute pointers and the reason to copy goes with them.

## 4. The format

Position independent, native endian, no runtime relocations, direct clip index,
direct script index, no token/path lookup, no pointer patching. Packed-bit
channels stay packed bits **by construction** — the generator performs no
arithmetic on any value, so `ndsBaseGcPlayMObjMatAnim`'s five `0xRRGGBBAA`-in-f32
tracks are byte-identical in the pack whatever they mean. This is the trap slice
41 fell into and this format cannot repeat it.

```text
header          'BPA1', version, clip_count, offset_count, stream_words   (20 B)
clip directory  clip_count x { u16 asset_id; u16 script_count;
                               u32 offset_table_byte_off; u32 stream_bytes }
offset table    one u32 word-offset per script, into the shared stream
command stream  u16[] -- the normalized AObj16 script region, verbatim
```

The consumer decides all of this and there is exactly one:
`ndsR2FtAnimParseDObjFigatree` walks `root_dobj->anim_joint.event16` one u16 at a
time, reading `command.opcode/.flags/.toggle`, `->u` and `->s`. **A clip is a u16
array plus one entry point per animated DObj.** Every other field of the O2R file
exists only to get those bytes into RAM.

Offsets are u32 rather than u16 because the shared stream passes 128 KiB; the
table costs 22,516 B against the 11,258 a per-clip u16 form would cost, and the
shared stream saves 30,184 B of exact duplicate script runs, so the u32 form is
18,926 B ahead. Emitting u32 unconditionally also means the format cannot
silently overflow when the bank grows to twelve fighters.

The four `AObjEvent32` animations (`0x279`, `0x27a`, `0x309`, `0x30a` — Mario
Appear1/Appear2, Fox Appear/Arwing) are **not** in this format and are named in
the generator rather than silently skipped. They keep the generic path.

## 5. What slice 1 phases 1–4 established

1. **The FAT read is not the lever.** 15 animation file loads in a 1,600-frame
   match. Deleting those 13 frames *entirely* moves the 80th-largest frame by
   **9,874** against a 64,452 requirement.
2. **The per-acquisition work is the lever.** 299 acquisitions, 95.0% cache hits,
   on 62 of the 80 P95 frames against 174 of 1,520 body frames — **6.8x
   presence**. A modelled full deletion moves rank-80 by **73,659**.
3. **The complete reachable set is all 301 clips**, from `ftdata.c` — not the
   87-entry observational warm list.
4. **The complete pack is 651,928 B and does not fit proven RAM** (~511,904 B).
   §6 owns what that blocks.
5. **Host equivalence is proven at mismatch = 0** over 297 clips / 5,629 scripts
   / 77,959 commands / 71,500 per-track states / 5,629 event callbacks, with two
   falsifiers demonstrating the test can fail.

## 6. What slice 1 phases 5–9 must still do, and the one thing blocking them

Phases 5–7 (direct runtime instance, same-build oracle mode, after-GO zero-I/O
assertion) and 8–9 (performance, re-rank) are unchanged from `plan.md` §K1.

**The blocker is RAM, and it is a measurement, not a decision.** The complete
pack needs ~140 KB more than is proven available. Three lawful moves remain, in
the order a next cycle should take them:

1. **Measure what the resident pack makes unnecessary.** A `const` shared clip
   removes the per-status `syTaskmanMalloc` animation allocations *and* the
   262,144 B raw-file arena *and* the animation entries in
   `sNdsRelocLoadedFiles[96]`. The arena is already counted in the 511,904;
   the status-heap allocations are **not**, because nobody has measured their
   volume. That is one gdb read on the existing gate ROM, not a build.
2. **Items are off for P1**, and 38 of the 301 animations are item-flavoured
   (101,472 payload bytes, 14.4%). Excluding them is provable from the match
   rules rather than guessed — but it must be *proven* from `ftdata.c`'s status
   graph, not from symbol names, before a byte is dropped.
3. **If neither closes it, a deterministic pre-GO loading arena** sized to the
   whole pack. `plan.md` §K1 phase 3 forbids the fourth option — falling back to
   gameplay-time FAT loading — and that prohibition stands.

**No lossless compaction is available and this was measured, not assumed:** dead
tail bytes past each script's terminator are 6,262 (1.0%), exact duplicate runs
30,400 (4.6%), and substring merging a further 28 bytes (0.004%). The command
stream is genuinely ~620 KB.

## 7. What this design explicitly is NOT

- **It is not slice 32's dense bank** (`include/nds/nds_anim_dense.h`,
  `scripts/generate_ftanim_dense_bank.py`). That expands each script into
  resolved per-track write records at **4.46x** the source and is dead on size.
  Its verdict stands and must not be re-opened. This pack keeps the command
  stream and deletes the file architecture around it instead — 95.0% of the raw
  payload, not 446%.
- **It is not a bigger cache.** §2.
- **It is not a fidelity trade.** Nothing here changes a value, a duration, an
  interpolation mode, an event order or a packed payload bit; §5 item 5 is the
  proof.
