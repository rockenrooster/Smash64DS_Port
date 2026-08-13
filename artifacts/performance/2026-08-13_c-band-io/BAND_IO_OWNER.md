# The band's file-I/O half is the SOUND EFFECT loader — and it is 12,736, not 16,000

**Outcome: named, partitioned with zero unexplained frames, and REFUTED as a
lever.** In-match file I/O is exactly two mechanisms — the BGM worker's packet
reads and **the FGM (sound effect) pack read that `ndsAudioFgmPlayAtPan` does
synchronously on the gameplay frame when its 8-slot cache misses.** The
inherited attribution (`../2026-08-13_shdt-band-owner/BAND_OWNER.md` §5: "the
synchronous animation-file load a landed hit's status change triggers") is
**stale** — it was measured on a profile that predates slice 46.

Eliminating every one of the 91 in-match sound-effect reads prices **−12,736
`WORK-H` P95** at the measured alignment and **−13,580 under the worst frame
placement the data allows**. Both are under the 16,000 bar. And it is not
reachable anyway: the pack holds **942,272 bytes** of cue data against a
**204,800-byte** cache, so full residency is arithmetically impossible and the
RAM law refuses the difference.

**No build, no emulator run, no ROM, no runtime source change.** Root ROMs
unchanged: `smash64ds.nds` 54c07fac…6ac68a, `smash64ds-battle-playable-hwtri.nds`
524448c9…23adee. Boot-headroom price: **zero bytes**.

---

## 0. Why the inherited answer was wrong: the profile predates the fix

`BAND_OWNER.md` §5 split `../2026-08-12_c123-rebank/profile/` —
`build-c123-profile`, which is **before slice 46**. On that arm the anim cache
missed 32 times a match and the 30 unwarmed animations were re-read off the card,
so `f_read` ×20.9, `_read_r` ×20.0, `ndsRelocNormalizeFighterAObj16File` ×4.5 and
`strncasecmp`/`ndsRelocApplyWordByteSwap` all sat on the band. Slice 46 took
misses **32 → 2**. The same 20-region mask on
`../2026-08-12_c123-rebank/profile-warm/` (`build-c124-profile`, **after**
slice 46 — `warm-sprm-split.txt`) shows what survived:

| symbol | c123 (pre-46) +tk/fr | c124 (post-46) +tk/fr |
|---|---:|---:|
| `f_read` | 5,586 | **out of the top 40** |
| `_read_r` | 4,529 | **out of the top 40** |
| `ndsRelocNormalizeFighterAObj16File` | 8,183 | **out of the top 40** |
| `get_fat.isra.0` | 14,969 | 7,639 |
| `f_lseek` | 9,498 | 4,816 |
| `armCopyMem32` | 18,434 | 14,981 |

Whole-match on the post-46 arm, `ndsRelocNormalizeFighterAObj16File` is **23
ticks/frame over 327 calls**, and deleting it *and* `ndsR2AnimCacheFind`
entirely prices **+0** P95 (`io-series-warm.txt`). **The animation-load lane is
closed, and it was closed by slice 46, not by this cycle.**

The two profiles are frame-aligned to each other at offset 0 (r = **0.864** on
region totals), so this is the same match on the same frames, one build apart.

---

## 1. The partition — 167 frames, two owners, nothing left over

`split-bgm-vs-sfx.txt`. Profile region `i+1` ↔ gate row `i` (offset +1 peaks at
r = 0.354 on `ALL` against 0.06 at its best neighbour; **sort
`arm9-profile.regions.csv` by `region` first — it is not written in region
order**, and reading it as written makes every offset look equally wrong).

| | frames | runs | file-I/O cyc | tk/fr | seek share | `armCopyMem32` | `get_fat` | `f_lseek` | `f_read` |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| BGM packet reads | 76 | 76 | 5,153,422 | 1,610 | 77% | 2,511,512 | 44,817 | 67 | 257 |
| **FGM (SFX) loads** | **91** | **89** | 4,782,140 | 1,494 | 74% | 2,762,206 | 37,467 | 117 | 300 |
| total | 167 | 156 | 9,935,562 | 3,105 | **76%** | 5,273,718 | 82,284 | 184 | 557 |

**The discriminator, and it has no exceptions in the direction that matters:**

| candidate trigger | fires on | of those, carry SFX I/O | **SFX-I/O frames WITHOUT it** |
|---|---:|---:|---:|
| **`ndsAudioFgmPlayAtPan`** | 128 | 91 (**71.1%**) | **0** |
| `battleship_ftMainSetStatus` | 263 | 66 (25.1%) | 25 |
| `lbCommonAddFighterPartsFigatree` | 257 | 66 (25.7%) | 25 |
| `ndsRelocAssetIDForToken` / `ndsR2AnimCacheFind` | 257 | 66 (25.7%) | 25 |

Every non-BGM file-I/O frame in the match carries a sound-effect play. The
status change the previous cycle blamed is absent from 25 of them and present on
181 frames that read nothing. **`ndsAudioBgmReadPacket` covers the other 76
frames at 100%.** 76 + 91 = 167 = every file-I/O frame. The partition is
complete.

**This answers the brief's (a)-vs-(b): it is (a), a main-thread synchronous
load — in the AUDIO path, not the animation path.** Not BGM preemption: the
worker's own reads are the separately-identified 76.

---

## 2. The mechanism, from the source

`src/nds/nds_audio_fgm.c`. Gameplay reaches it through
`ndsPlayFGMAtPan` (`src/port/reloc_backend_compat_shims.c:702`) →
`ndsAudioFgmPlayAtPan` (`:1087`) → `ndsAudioFgmCacheAcquire` (`:456`):

```c
for (i = 0u; i < NDS_AUDIO_FGM_CACHE_SLOT_COUNT; i++) {        /* 8 slots */
    if ((slot->fgm_id == entry->id) && (slot->data_bytes == entry->data_bytes))
        return (s32)i;                                          /* hit */
    if ((slot->references == 0u) && (slot->capacity >= entry->data_bytes) && ...)
        best = (s32)i;                                          /* victim */
}
if ((best < 0) || (sNdsAudioFgmFile == NULL) ||
    (fseek(sNdsAudioFgmFile, (long)entry->data_offset, SEEK_SET) != 0) ||
    (fread(sNdsAudioFgmCacheSlots[best].data, 1u, entry->data_bytes,
           sNdsAudioFgmFile) != entry->data_bytes)) { ... }
```

- **8 cache slots against 88 pack entries**, fixed capacities
  53,248 / 3×28,672 / 4×16,384 = 204,800 B, victim rule "smallest free slot that
  fits", **no recency**. `NDS_AUDIO_FGM_HANDLE_CAPACITY` is also 8 and a live
  handle holds `references`, so with the measured peak of six live handles only
  two slots are ever eligible. A 71.1% miss rate is what that structure predicts.
- The file (`nitro:/audio/fgm_phase_pack_ima.bin`, 938,996 B) is held **open**
  for the whole match and every miss `fseek`s to an absolute offset.
- The seek is **76% of all in-match file-I/O cost**: 184 `f_lseek` calls at
  **13,159 cycles each**, driving **82,284 `get_fat`** and 82,983 `move_window`
  — **447 FAT-chain steps per seek**. That is FatFs restarting the cluster walk
  from the chain head on a backward seek, served by calico's FatFs + DLDI stack
  (`f_lseek`/`get_fat`/`move_window`/`disk_read`/`_dvmDiscCache*`), i.e. the
  NitroFS-over-DLDI path — **toolchain, not repo source.** 447 steps is
  consistent with the 12 MB ROM image's own chain, not the 917 KB pack's, so
  **this cost grows with ROM size** (`PROJECT_GOAL.md` trades ROM for speed).
- The envelope re-seek at `:1148` is **not** a driver: only 9 of 88 entries carry
  an envelope and the whole envelope region is **256 bytes**. Checked, not assumed.

---

## 3. Sizing — and the alignment-free bound that closes it

P95 is the 80th largest of 1,600; baseline `WORK-H` **1,220,480**, gate
1,120,380. Two columns: subtract at the aligned frame, and — because a
cross-build alignment is the exact trap that produced §0's wrong answer —
subtract with the bursts **sorted onto the costliest frames they could possibly
have hit**, which needs no alignment at all.

| change | ΔP95 aligned | ΔP95 worst-case pairing |
|---|---:|---:|
| **FGM loads eliminated (residency/preload)** | **−12,736** | **−13,580** |
| FGM loads, seek + read only (payload copy stays) | −7,104 | −9,350 |
| FGM seek walk only | −3,648 | −4,476 |
| seek walk, both halves | −5,843 | −26,512 |
| ALL file I/O, no payload copy | −8,153 | −37,666 |
| **ALL file I/O + payload copy — lane saturation** | **−19,648** | −51,899 |
| FGM preload + BGM seek walk removed | −19,264 | −31,334 |

**The FGM row is under 16,000 on BOTH columns.** The worst-case bound is the
decisive one: even if every sound-effect load had landed on the most expensive
frames in the match, deleting all 91 of them pays **13,580**. There is no
alignment error that rescues this lever.

**Why the lane saturates so low.** The rank-80 frame — the frame that *is* P95 —
carries **zero** file I/O. 37 of the top 80 carry some, but the bursts cluster
off the percentile: the single largest burst (228,298 tk of I/O + copy) sits at
gate rank **1,350**, i.e. one of the cheapest frames in the match. BGM's 76
frames have a median gate rank of 744 and only 4 inside the top 80.

**And ≥16,000 is unreachable on top of that.** The only rows that clear the bar
require deleting the payload copy on the BGM side too. BGM is a stream: slice 48
already refuted deprioritising it during the match (+8,064) and closed buffer and
packet sizing, and its seek lives in the toolchain's FatFs. The FGM side cannot
be preloaded to residency either — **942,272 B of cue data, 204,800 B of cache;
51 of 88 entries fit packed smallest-first** — and the RAM law refuses the
difference (`gSYTaskmanGeneralHeap` free-min 72,188 against a 32,768 floor,
highest `fake_heap_start` proven to boot `0x02294804`, and +14 KB of bss once
stopped the ROM booting).

---

## 4. The "38" the brief asked about — coincidence, and the other 38 is dead

The `SHDT` band is **88 frames in 38 runs** (38 engagements). The board's
"38 in-use assets are refused" row is `builds/build-c118-gate`, where the anim
arena read 200,400/200,704 with `gNdsR2AnimCacheArenaOverflows`/`…Rejects` = 38.
**Slice 46 killed that**: the c123 bank measures `ArenaUsed` **192,240 of
262,144**, `Rejects` **0**, `Overflows` **0**, `Misses` **2**. The board's 38 is
a dead counter on a superseded arm; the band's 38 is a count of engagements.
Nothing connects them, and only **18 of the 88 band frames** carry any SFX I/O
(6 carry BGM I/O) — so the file-I/O half was never most of that band either.

---

## 5. What is left, and what it is worth

Recorded so nobody re-derives it, **all under the bar**:

1. **Repartition the existing 204,800 B into more slots with LRU.** Zero new
   RAM, and the only shape that attacks the 71.1% miss rate. Its **ceiling** is
   −12,736 and it cannot reach it (residency is impossible), so the honest
   estimate is a fraction of that. There is **no cache hit/miss counter** on the
   slot cache — `ndsAudioFgmRecordMiss` (`:363`) counts *cues absent from the
   pack*, not slot misses, and the 71.1% above is a profile inference. One `++`
   pair is the instrument any future attempt needs first.
2. **The seek walk (76% of all file-I/O cost, 447 FAT steps per seek).** Needs
   FatFs fast-seek (`CREATE_LINKMAP`) or an equivalent, inside calico —
   toolchain, not repo source. Prices −5,843 aligned even if free.
3. **`STDIO` is apparatus, like `cpuGetTiming`.** The debug/HUD text path
   (`ndsPlatformPrintDebugLine` → `vsniprintf`/`iprintf` → `_vfiprintf_r` →
   `consolePrintChar`, plus the DS console's `siscanf` ANSI-escape parse) costs
   **3,984 tk/frame** whole-match on the profile ROM and prices −9,194. The
   published ROM builds `NDS_TICK_HUD=0`; optimising it improves the number
   without improving the game. Add it to `RESIDUE.md` §5's 14,691.

## 6. Reproduce

```
# one pass over the 2.6 GB profile writes the (region, symbol) cache
python scripts/analyze-profile-region-split.py \
  artifacts/performance/2026-08-12_c123-rebank/profile-warm/arm9-profile.csv \
  --census artifacts/performance/2026-08-12_c123-rebank/profile-warm/census.json \
  --gate-csv artifacts/performance/2026-08-12_c130-fire-gate/c130-gate-rows.csv \
  --gate-lane SPRM --gate-min 30000 --control-lane SPRM --control-max 10000 \
  --check-align --check-controls --head 40 --cache <scratch>/warm-cache.npz   # -> warm-sprm-split.txt

# then every family below is instant off that cache
python scripts/analyze-io-lane-series.py --cache <scratch>/warm-cache.npz \
  --gate-csv artifacts/performance/2026-08-12_c130-fire-gate/c130-gate-rows.csv \
  --group "FAT-IO=get_fat.isra.0,f_lseek,f_read,..." \
  --cooccur "fileIO=..." "FGMplay=ndsAudioFgmPlayAtPan" "BGMpacket=ndsAudioBgmReadPacket" \
            "statusChange=battleship_ftMainSetStatus"                         # -> io-series-warm.txt
```

`split-bgm-vs-sfx.txt` is the partition table, the placement analysis and the
pack parse in one pass; its source is inlined at the top of this directory's
commit. Keep the `.npz` cache OUT of the tree — it is derived, large, and its
path carries the build machine's user directory.

## 7. What this cycle did NOT do

- No instrumented build, so no slot hit/miss counter and no measured working-set
  bytes. The 71.1% is presence-derived from the profile.
- Did not re-profile on current code. The mask is the c130 gate arm; the
  per-symbol composition is `build-c124-profile` (post-46, pre-slice-48-ship).
  Slice 48 moved only the worker's *creation* priority; in-window the worker runs
  above main in both, so the BGM half is comparable.
- Did not test whether `armCopyMem32` on I/O frames is entirely payload copy. It
  is on exactly the same 167 frames with an infinite call ratio against control,
  which is why it is charged here; that is co-presence, not attribution.
