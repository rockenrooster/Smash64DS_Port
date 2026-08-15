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
4. **The complete pack is 651,928 B and does not fit proven RAM** (~511,904 B),
   and cycle 2 confirmed it still does not with the items-off exclusion proven
   and route 1 measured. §6 owns what that blocks and what closes it.
5. **Host equivalence is proven at mismatch = 0** over 297 clips / 5,629 scripts
   / 77,959 commands / 71,500 per-track states / 5,629 event callbacks, with two
   falsifiers demonstrating the test can fail.

## 6. What slice 1 phases 5–9 must still do — the two blockers are CLEARED

Phases 5–7 (direct runtime instance, same-build oracle mode, after-GO zero-I/O
assertion) and 8–9 (performance, re-rank) are unchanged from `plan.md` §K1.

**The blocker was RAM. MEASURED 2026-08-14 (cycle 2): it did not fit, and the
closure was a static-RAM recovery, not a pack change.** Evidence and every
citation: `BATTLEPACK_ANIMATION.md` §11–§12. **That recovery is now spent — see
the landing note below.**

```text
                                   pack        pool        short
full pack (297 AObj16 clips)      651,928     511,904     140,024
items off (259 clips, PROVEN)     553,696     511,904      41,792
  + route 1 (2..4 figatree heaps) 553,696     524,352..537,120   29,344..16,576
  + matchup lead (245, UNPROVEN)  528,624     524,352..537,120    4,272..-8,496
```

Three findings settle the three moves that were listed here:

1. **Route 1 is 12,608–25,216 B, not 140,024, and it needed no gdb read.** The
   per-status `syTaskmanMalloc` volume is `gFTManagerFigatreeHeapSize` per
   fighter — `ftmanager.c:170-206` makes it the **largest single animation file**
   over the loaded kinds, a compile-time max over the bank (max payload 6,224 B),
   and `scvsbattle.c:199`/`:472` take one per active player plus one per player
   on Sudden Death entry. The 262,144 B arena was **already** counted; the
   192,240 in `docs/HANDOFF.md` is that arena's bump high-water *inside* the
   reservation, not a second pool.
2. **The items-off exclusion is PROVEN, and priced by re-packing: −98,232.**
   Not from `ftdata.c`'s status graph but from the linked battle ELF, which is
   stronger: every function that could set an item status is a two-byte `bx lr`
   stub and the ELF contains no item spawner at all. The 259-clip pack
   re-verifies at **mismatch = 0** (corpus `c034b342…`).
3. **Neither remaining route closes it.** A more compact representation is
   refuted as a *lossless* lever — the stream is already u16 headers and **s16**
   target words, so there is no f32 to narrow and no dictionary that can pay; a
   pre-GO arena creates no RAM at all, it only chooses which pool pays.

**And the pool's three terms are not additive as written.** The 262,144 arena and
its 37,824 slack live *inside* one `calloc` of `NDS_TASKMAN_ARENA_SIZE = 0x150000`
(`diagnostics.c:7750`, `:7791-7849`), so freeing the arena releases no libnds
heap. A `.rodata` pack draws on **211,936** alone; an arena-resident pack draws on
**299,968** alone. The 511,904 exists only if `NDS_TASKMAN_ARENA_SIZE` is cut too,
which also needs the search's `0x130000` floor lowered *and* the Task 36
replay-admission guard taught the new constant (`nds_renderer.c:5734-5739`).

**BOTH BLOCKERS ARE LANDED AND VERIFIED, 2026-08-15**
(`…/2026-08-15_framebuffer-collapse/PHASE2_FRAMEBUFFER.md`; commits
`8cfbc2eaa2b`, `6e93def43cd`). Phases 5–9 are no longer blocked.

1. **`docs/RAM_RECOVERY_PLAN.md` Phase 2 is spent: +146,560 B, MEASURED.**
   `gSYFramebufferSets` `[2][230][320]` → `[1][231][320]`, 294,400 → 147,840 B.
   Same build directory both arms: bss 1,453,544 → 1,306,984 (−146,560 exact),
   text and data unchanged, `fake_heap_start` `0x02269ee4` → `0x02246264`,
   proven boot headroom 174,368 → 320,928. The reader set came from the **linked
   ELF**, not the doc comment — one reader (the wipe), one writer (`scmanager`'s
   clear, self-bounded by `sizeof`) — and the span was re-derived from the
   wipe's own compiled literals (`+0x23f14` start, `−640` row step, 220 rows,
   600 B/row → `base+7,060 .. base+147,819`). Boundary green, VS Results at
   source tic 160 byte-identical across the arms; the wipe's animated frames
   were not captured and still need the owner's eye.
2. **The `ftmain.c:4623` patch is landed and provably inert** —
   `scripts/decomp-patches/battleship/src_ft_ftmain.patch`. Exactly one symbol
   changes size in the whole binary (`battleship_ftMainSetStatus`, −8 B): the
   deleted instructions are the post-call reload of `fp->figatree_heap`, after
   which both arms execute the same store. Source-side, the callee returns
   exactly `heap` on every path when `heap != NULL`.

> **CORRECTION, and it matters for phase 3: "+146,560 makes the full pack fit by
> 6,536" is an arithmetic statement about the COMBINED 511,904 pool, and the
> non-additivity finding below is unchanged.** Measured static headroom on the
> **published** arm is 213,216 → **359,776**; a `.rodata`-resident full pack is
> still short 292,152 and an items-off pack short 193,920. An arena-resident
> pack still draws on 299,968 alone. Only the combined row fits (full by 7,816,
> items-off by 106,048), and reaching it still needs `NDS_TASKMAN_ARENA_SIZE`
> cut, the `0x130000` floor lowered and the Task 36 guard retaught. Phase 2 is
> what makes that coupling worth doing — the freed 146,560 B is exactly the
> libnds-heap slack a larger arena draws on. Which pool the pack lands in is
> phase 3's decision.

> **THE POOL IS THE ARENA, MEASURED 2026-08-15**
> (`…/2026-08-15_battlepack-pool/BATTLEPACK_POOL.md`). A `.rodata`-resident
> blob costs the arena as well as the static image: the arena is one `calloc`
> from the libnds heap that `fake_heap_start` bounds, so the two come out of the
> same bytes. **+288,992 B of `.incbin` pushed `gNdsTaskmanArenaChosenSize`
> `0x150000 → 0x140000` with 16 alloc failures**, which against the match's
> measured 1,304,068 B of taskman use projects a general-heap low-water of
> ~6,652 against the mandated 32,768 floor. Not shippable;
> `NDS_R2_BATTLEPACK` lands **default 0**. **The boot-headroom ladder does not
> catch this** — that arm still had 66,784 B of proven headroom — so any static
> growth must now also read `gNdsTaskmanArenaChosenSize` (added to the soak's
> global list).
>
> **The fit is one fighter resident IN the arena**, replacing the 262,144 B
> raw-file anim cache: Mario 271,728 (+9,584 of the 39,420 general-heap slack)
> or Fox 287,904 (+25,760). Zero static growth, arena stays `0x150000`, ladder
> untouched. Splitting **per fighter** costs 136 B of cross-fighter dedup. What
> is not built is the setup-time load that puts the blob there.
>
> The emitted format is **BPA2**, not the banked BPA1 blob, for a consumer
> reason: BPA1's offset table carries one entry per *distinct script*, while
> `lbCommonAddFighterPartsFigatree` advances one word per *DObj slot* (~26 slots
> against ~19 distinct scripts per clip). BPA2 emits the per-slot table with
> blob-relative byte offsets; slot-level equivalence is **mismatch 0** on both
> fighters (3,611 / 2,976 slots).
>
> **AND §5 ITEM 5'S "mismatch = 0" DID NOT COVER THE RUNTIME'S BIT ORDER.** The
> host probe's `normalize()` is pipeline **4a-4b only**; the ROM also applies
> **4c**, re-encoding every command word from disk MSB-first order into the
> native C bitfield order (`ndsRelocAObj16EncodeForNativeBitfields`,
> `reloc_backend_assets.c:3313`), and the direct path skips
> `ndsRelocFinalizeLoadedFile` so nothing applied it at runtime either. Phase 4
> missed it because both sides used the same disk-order decoder. A ROM **data
> abort** found it. The generator now applies 4c, `decode_script` takes
> `native=`, and the falsifier is measured: the emitted blob decoded in disk
> order raises on **75 of 2,713** scripts, in native order **0**.

`plan.md` §K1 phase 3 still forbids the fourth option — falling back to
gameplay-time FAT loading — and that prohibition stands.

**Phase 5's premise was half the story (`BATTLEPACK_ANIMATION.md` §12), and the
other half is now fixed.** The absolute-pointer fixups are one reason the
adapter copies; the other was `decomp/…/ft/ftmain.c:4623-4624`, which discarded
the return value and hardcoded `fp->figatree = fp->figatree_heap`, so the bytes
had to be at that heap whatever the fixups did — and `src/import/battleship_ftmain.c`
`#include`s that body rather than owning it, so it could not be fixed port-side.
§3's inventory does not mention it. The returned pointer is now authoritative,
so a `const` clip path may hand back pack storage.

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
