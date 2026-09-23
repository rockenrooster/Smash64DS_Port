# P2-2p8 Phase 1 slice 4: fighter lists without recorded packets

The lean path no longer adopts packets from the recorder.
- It **materializes** each list itself from the generated owner tables and the live material rows, in the LOAD4x3 + P' layout, into the slot's existing region (two entries per battle slot).
- Route 1 records **0** packets and declines **0** draws for the four stress kinds.
- Lists are keyed by content, so they survive model-part swaps and rebuilt MObjs.
- A state whose list differs from a held list by a few words is kept as a **variant record** of that list, not as a second list.

Gate ROM: `smash64ds-p2-fourcpu-tickhud-hwtri.nds`, sha256 `563f9d92...f457fb` (`canonical-sha-final.txt`). Gate arms: `b8-route0`, `b8-route1`, `b8-route2`, plus the verify arm `b8-route1-verify`. Four-CPU stress, admit word 2, 1,972 samples, runner slot 9.

## Gate table

| gate | slice 4 | reference | verdict |
|---|---|---|---|
| replay digest, route 1 vs route 0 (same ROM) | **IDENTICAL**, 1,972 frames | also identical: route 2 vs 0, the verify arm vs 0, and route 0 vs pre-slice-4 route 0 | **met** (`digest-*.json`) |
| oracle, route 2 | **0 mismatches** per kind: DK 0/1,660 runs, Samus 0/1,663, Link 0/1,735, Kirby 0/1,548; 13,620,081 words | clip row 3 max 1 LSB over 96,259 roots; `donor_memo` 192 (Link, named in section 2) | **met** |
| lean share, declines named | **100.00%** per kind (1,725 / 1,685 / 1,824 / 1,628 draws), **0** declines | 18 named decline reasons | **met** |
| native failures | 39 in route 0 = 39 in route 1 | 39 pre-slice-4 | **met** |
| FTR P50 / P95 / P99 | **227,488 / 318,557 / 812,284** | slice 3 route 1: 229,056 / 435,251 / 1,025,600. Pre-slice-4 on today's tree: 230,304 / 441,302 / 1,028,751 | **met** |
| WORK-H P50 / P95 / P99 | **1,507,808 / 2,138,586 / 2,763,228** | slice 3 route 1: 1,512,928 / 2,284,672 / 2,896,626. Pre-slice-4: 1,516,096 / 2,276,464 / 2,961,049 | **met** |
| shipping static RAM (nm, non-tick-HUD shell ELF) | **+11,312 B** loaded: +9,560 code, +2,208 bss, -456 ITCM | pre-slice-4 sources on today's tree | reported (section 5) |
| heap low-water, gate ROM | **37,636 B**; libc top minimum 19,104 B | 54,020 B pre-slice-4: exactly 4 arena pages | reported (section 5) |
| list RAM vs owner images | lean list 5.6-10.5 KB per kind at low detail, against owner images of 14.5-24.3 KB | table in section 5 | reported |

Extra check, the route-1 verify arm (`gNdsFtrLeanSlow` bit 16):
- Every event that re-selected a held list, 165 of them, also materialized the same key into the other half and compared the two.
- 134 of those were on entries that held variant records.
- There were **0** mismatches in shape, words, texture tables or fences.

## 1. Runtime design (D3)

**Materializer** (`ndsFtrLeanMaterialize`, `src/nds/nds_renderer_native_common.c`):
- It walks the production execute's own state machine over the owner's generated tables: root bind, light preamble, state spans, materials, `ResolveEpochShade`, and the run prepare's validation and texture bind.
- It packs the words the recorder's hooks would record, with the matrices in the lean layout:
  - head: LIGHT_COLOR, P' once, the light under an identity vector matrix;
  - per root: one 12-word LOAD4x3.
- There is no old-path draw and no recording.

**Entries.** Each battle slot has two entries, in the lower and upper half of its 35,360 B packet-arena region.
- Route 1 owns both halves; the recorder never arms for a slot whose lower half is lean.
- Routes 2 and 3 keep the recorder in the lower half as the oracle's reference.

**Entry key**, 6 words:
- the drawn roots' material rows, as content (no pointers);
- owner, detail, slot, costume, shade;
- owner file and heap generation;
- the roots' key[3] preamble fields;
- program and root count;
- Kirby's trio head key.

**Event path** (first draw, rebind, material, preamble, tint-set, fence): resolve the plan, build the rows and the key, then:
- the held entry, or a variant record of one, whose key matches;
- or a new materialization into the victim entry. The victim is an empty or stale entry first, then the inactive one.

**Changes this slice made to cut events**, driven by the census in section 4:
1. **Lists survive a slot rebind.** `ndsRendererFighterPacketInvalidateSlot` still kills the recorder's packet, which is keyed by MObj address, but keeps the lean lists (`ndsFtrLeanPacketRebind`). A model-part swap or rebuilt MObj re-selects by key. Before this change, every DK and Samus event found both entries dropped.
2. **Fold-free tint repatch.** A list with no tint folds depends on the tint-tile set only through its bind words. When the set moves, the patch re-points every bind (`tint_repatch`) instead of re-materializing. A list with folds still re-materializes, and a colour whose tile left the set still invalidates that entry.
3. **Variants.** After a materialization, `ndsFtrLeanLearnVariant` compares the new list with the other entry's list.
   - Same shape means the same words at the same places, the same patch sites and site colours, and the same frame accounting.
   - If the new list has the same shape and differs outside the patch sites by at most 16 words, the new state becomes a record in the held entry's word tail: key, word diffs with the base words, texture table, fences. The record takes 384 B, up to 8 per entry.
   - Switching record is a few word writes, marked dirty.
   - The run learned 12 records, averaging 3.7 differing words (max 6), and made 144 switches.
4. **Re-record shadow.** The old path re-records whenever `ndsFighterPacketBuildKey`'s words move, or when its packet is invalidated. A record derives the shade words at modulate 0 while naming the live modulate (the production quirk slice 4 already mirrors). A held list must take that derivation exactly where the old path would re-record.
   - `ndsFtrLeanRerecordKey` shadows that key over the lean inputs. The tint-set generation is left out, because the repatch handles it.
   - On a hit with no switch, the event path calls `ResetShade` if the shadow moved or the slot was invalidated.
   - The tint repatch also calls `ResetShade`.
   - Without this, b7's oracle read 286 DK shade mismatches. With it, 0 (b8).

**Declines.** 18 named reasons, with 2 reserved codes: Kind, Skeleton, Camera, Plan, Validate, KirbyHead, Roots, Material, Tables, Policy, Texture, Capacity, Topology, Kernel, Texgen, Tint, Stale, Inputs. The four-CPU match hits none of them.

**Adoption is gone.** Slice 3's `PacketAdopt`, `SelectVariant` and the adoption variants are deleted. Route 2 is the oracle: it compares the materialized list with the recorder's packet on every replay hit.

## 2. Generator and proofs (D1, D2)

**Host emitter.** `scripts/fighters/fighter_list_emitter.py` replays an owner's generated tables exactly as the production execute does.
- It packs either the recorder layout or the lean layout.
- Tagged placeholders mark the patch sites: matrices, the light vector, TEXIMAGE_PARAM/PLTT_BASE, DIF_AMB with its inputs, and texgen coordinates.
- It covers every program of each kind, both details, Kirby's hats and trio heads, and the Entry/Appear programs as variants.
- It imports `generate_nds_native_owners.py`'s contexts. The stale `#if 0` fixture in that generator was not rebuilt, and no generated output changed, so nothing was re-pinned.

**Host proof.** `check_nds_native_owner_packet.py --fighter-lists dumps/r0 ...` runs `scripts/fighters/fighter_list_proof.py`; output in `host-proof.txt` and `host-proof.json`.

| kind (order) | recorded packets | words exact (masked) | tier 1: non-matrix commands that differ | tier 2 (GX differ): vertices, clip, max screen error |
|---|---:|---:|---:|---|
| Samus | 15 | 15 | 0 / 11,944 | 3,421; ±1 LSB; 0.0042 px |
| DK | 47 | 47 | 0 / 67,584 | 17,749; ±1 LSB; 0.0057 px |
| Link | 54 | 54 (2 need the memo defect) | 0 / 84,372 | 22,642; ±1 LSB; 0.0059 px |
| Kirby | 49 | 49 | 0 / 39,374 | 14,196; ±1 LSB; 0.0040 px |

**What the GX differ proves.** It simulates the GE matrix pipeline for both representations: P' plus LOAD4x3 against the split projection plus the MULT4x3 chain.
- The difference is a rounding of row 3 at ±1 LSB of clip space.
- No vertex moves more than 0.006 px; 19 of 58,008 cross a pixel boundary.

**Runtime oracle (route 2).** 0 mismatches over 6,606 hits (see the gate table). Two named effects:
- `donor_memo` 192: production's run texture memo keys on the run index alone, so Link's SpecialN boomerang, a donor-table root, replays the owner root's texture and UV. The materializer mirrors it and the oracle counts it.
- `record_diff` shade 98 over 256 records: on a record frame, the fresh record's DIF_AMB words are at modulate 0. The count is identical to the first slice-4 oracle build (b3). Outside-site diffs are 0.

## 3. D4: Link HW texgen

Not attempted. The ported PatchTexgen stays: 2,348 ticks per draw across all kinds, texgen patches 1,775.

## 4. Per kind, route 1 (`kinds-final.txt`, b8 = gate ROM)

| kind | draws | events: first / rebind / material / tint-set | materializations | ticks per materialization | variants learned / switches | per draw: guard / kernel / patch / submit |
|---|---:|---|---:|---:|---|---|
| DK | 1,725 | 1 / 56 / 0 / 1 | 26 | 537,044 | 0 / 0 | 13,956 / 23,240 / 8,750 / 4,301 |
| Samus | 1,685 | 1 / 14 / 0 / 1 | 10 | 355,424 | 0 / 0 | 5,574 / 19,538 / 6,080 / 4,166 |
| Link | 1,824 | 1 / 14 / 68 / 1 | 15 | 865,472 | 6 / 69 | 12,774 / 25,834 / 14,823 / 4,653 |
| Kirby | 1,628 | 1 / 3 / 72 / 2 | 11 | 160,884 | 6 / 75 | 5,529 / 18,317 / 4,417 / 3,741 |

**Totals.**
- 236 events, 62 materializations, 174 entry hits (156 switches), 9 tint repatches, 18 re-record resets.
- A hit's event path costs about 56,700 ticks.
- A materialization's list walk costs 443,118 ticks: bind 136,513, spans 101,493, corners 66,758, prepare 34,115, roots 33,679, shade 29,297, words 24,545, head 1,561.

**How the cuts were found (`b6`, census build).**
- 161 materializations over 245 events. 222 of those events re-selected a key the slot had drawn before.
- At materialization, the usable entries were invalid 218 times; all of DK's and Samus's had been dropped by rebinds.
- The key differed 104 times, only in word 0 (materials).
- Every new list had the same shape as the other entry's, and differed from it in at most 6 words.

| build | change | materializations | FTR P95 | FTR P99 | oracle |
|---|---|---:|---:|---:|---|
| b5 | 2 entries; lists dropped by every rebind and every tint-set move | 161 | 715,907 | 1,083,943 | 0 (b3) |
| b7 | lists kept across rebinds, fold-free repatch, variants | 62 | 317,533 | 814,472 | **286** DK shade |
| b8 (gate) | re-record shadow | 62 | 318,557 | 812,284 | **0** |

**Remaining events.**
- **DK**: 26 of 58 events still materialize. Its model-part swaps change the hand roots' display lists (key[3]), so the lists have different shapes: 42 shape rejects. Two entries cannot hold its hand states.
- **Samus**: its key[5], the Kirby trio head key that the draw publishes for every fighter, moved 8 times and forced materializations. Worth dropping from non-Kirby keys in a later slice.

## 5. Performance and RAM

### Performance

`gates-final.txt`: bands by WORK-H rank; per-range means with frame 3, the admission frame, listed apart.

| band | slice 3 route 1 | pre-slice-4 route 1 | slice 4 route 1 |
|---|---:|---:|---:|
| FTR P40-60 | 233,296 | 234,096 | 231,581 |
| FTR P90-95 | 423,118 | 440,276 | 341,383 |
| FTR P95-99 | 607,814 | 607,445 | 426,993 |
| FTR P99+ | 868,854 | 864,896 | 585,440 |
| WORK-H P40-60 | 1,514,178 | 1,517,019 | 1,509,777 |
| WORK-H P90-95 | 2,113,648 | 2,114,546 | 2,048,054 |
| WORK-H P95-99 | 2,485,954 | 2,493,813 | 2,339,077 |
| WORK-H P99+ | 4,363,933 | 4,383,990 | 4,229,638 |

| range | FTR s3 / pre / s4 | WORK-H s3 / pre / s4 |
|---|---|---|
| 2-195 entry | 114,703 / 114,975 / 101,537 | 1,068,794 / 1,072,552 / 1,057,420 |
| 196-797 | 273,781 / 275,101 / 254,106 | 1,522,113 / 1,525,090 / 1,503,424 |
| 798-1043 P0 episode | 272,281 / 273,465 / 262,236 | 1,713,556 / 1,716,377 / 1,704,261 |
| 1044-1974 | 261,044 / 262,178 / 243,345 | 1,648,092 / 1,651,311 / 1,632,030 |

**Route 0** (`gates-route0.txt`) against pre-slice-4 route 0 on today's tree:

| metric | pre-slice-4 route 0 | slice 4 route 0 |
|---|---:|---:|
| FTR P50 | 358,848 | 359,744 |
| FTR mean | 386,078 | 385,964 |
| FTR P95 | 915,350 | 910,182 |
| WORK-H P50 | 1,644,832 | 1,646,080 |
| WORK-H P99 | 3,040,703 | 3,114,861 |
| WORK-H P99+ band | 4,527,488 | 4,524,326 |

- WORK-H P99 moved by one percentile (+74K) while the P99+ band stayed flat.
- One codegen change reaches route 0:
  - The materializer is now the second caller of `ndsRendererNativePreflightProductionOwner` and `ndsRendererNativeBindProductionRoot`, so GCC no longer inlines them into `ndsRendererExecuteNativeFighterOwnerProduction` (ITCM, 3,656 to 3,196 B). They run from main RAM.
  - Forcing them inline overflows ITCM by 32 B.
  - Calling them through volatile pointers out-of-lined them anyway (the execute went to 2,984 B).
  - Both attempts were reverted. Route 0's behaviour is unchanged, and its timing is unchanged within the table above.

### RAM

**Shipping static** (`ram-delta-shipping.txt`, nm plus section sizes):
- ELF: `smash64ds-p2-shell-hwtri`, non-tick-HUD, built from the pre-slice-4 lean sources (`build-p2p8-s4-shipcheck-pre`) against the final sources (`build-p2p8-s4-shipcheck`), both on today's tree.
- **+11,312 B loaded**: .main +9,560 (code +9,152), .main.bss +2,208, .itcm -456.
- bss: `sNdsFtrLeanInstances` +1,440, now 4 x 1,040 B (plan arrays, file identity, re-record shadow), and `sNdsFtrLeanSlots` +768 (entry state: tint folds, variants, fences).
- The lab counters, key census, verify and oracle compile out of the shipping image. The lean behaviour itself has no lab-only branch.

**Tick-HUD gate image** (`ram-delta-tickhud.txt`): +15,880 B loaded (+13,808 .main, +2,528 bss, -456 ITCM).

**Heap low-water on the gate ROM: 37,636 B**, against 54,020 B pre-slice-4 on the same tree.
- The difference is exactly 16,384 B, 4 pages. The taskman arena is sized in 4 KiB pages from what the static image leaves (arena chosen: 1,261,312 B). The +15,880 B tick-HUD growth costs 4 pages, and every reading this slice is 772 mod 4,096.
- Margin: 4,868 B above the 32,768 B safety floor, and 12,036 B above the 25,600 B GObj-cap latch.
- Development builds b1, b4 and b5 read 25,348 B, under the latch. Those layouts lost 7 pages, and none of them is the gate ROM.

**List RAM against owner images** (host emitter at maximum structure; `host-proof.txt`; 1 word = 4 B):

| kind, detail | lean list, canonical program | worst program | recorded packet | owner image |
|---|---:|---:|---:|---:|
| Samus low | 1,791 w (7,164 B) | Catch 2,313 w | 2,093 w | 14,544 B |
| Samus high | 2,763 w | Catch 3,286 w | 3,066 w | 19,316 B |
| DK low | 2,357 w (9,428 B) | - | 2,705 w | 15,232 B |
| DK high | 3,535 w | - | 3,883 w | 21,340 B |
| Link low | 2,632 w (10,528 B) | Catch 2,759 w | 3,048 w | 15,892 B |
| Link high | 3,746 w | Catch 3,873 w | 4,162 w | 19,744 B |
| Kirby low | 1,398 w (5,592 B) | TrioHead8 2,292 w | 1,541 w | 24,268 B, plus 144,244 B for 22 hat files |
| Kirby high | 1,792 w | TrioHead4 2,848 w | 1,936 w | 30,160 B |

- **Per instance**: 2 entries x (3,840 B header + 13,840 B words) = 35,360 B. This is the slot's existing region in the packet arena (the borrowed framebuffer), not heap, as in slice 3.
- Variant records live in each entry's word tail beyond the list.
- Largest lists materialized in the match: DK 2,467 w, Samus 2,313 w, Link 2,634 w, Kirby 1,403 w.
- **High detail**: DK (3,535 w) and Link's worst program (3,873 w) exceed the 3,460-word entry. They decline as `Capacity`, which is named. The four-CPU match draws low detail only.
- **The lists do not replace the owner images yet.** The materializer reads them at every event that builds a list. What slice 4 removes is lean's dependence on recorded packets.

## 6. Deviations and open items

1. **Materialized at the first draw's event path, not admitted at the creation seam.**
   - The list is built before the draw submits, so first draws, part swaps and new materials no longer reach production: 0 declines, 0 recordings in route 1.
   - The source is the owner tables on the device, not a host-built payload.
   - A schedule generator (`generate_nds_fighter_lists.py`) was drafted early in the slice but never wired into the ROM. It was taken out of the tree (kept in the session scratchpad), and its mentions were dropped from the header and the emitter.
2. **DK's shape-changing model parts** still materialize: 26 x 537K ticks over the match. They need either shape-changing variants or a third entry.
3. **Variants are route 1 only.** Route 2 has one lean entry and no scratch half. The verify arm covers them (0 mismatches over 134 variant-entry re-selections); the oracle does not.
4. **High-detail DK and Link** lists do not fit an entry (section 5).
5. **Canonical sha.**
   - `563f9d92...` came from b8 and was reproduced by the final re-make (`build-final-recheck.log`) after all the shell builds.
   - One intermediate forced build (`build-b9.log`, `75cb2430...`) compiled `battleship_efmanager.o` and `battleship_lbparticle.o` before the particle-bank generator rewrote the header that the shell builds had changed. `-ForceParticleDeps` deletes those objects but does not order them after the generator.
   - That ROM's four arms (`final-*`) reproduce b8's rows byte for byte, and they are not used as gate evidence.
6. **Rules.** No subagents. Build lock held for every build. Slot 9 only. Nothing under `decomp/`. `nds_ifcommon_oam.c`, `reloc_backend_assets.c` and `nds_ifcommon_oam.h` were not edited. **One read-only `git diff --stat` was run by mistake.** It wrote nothing, and no other git command was run.

## 7. Files

- **Runtime**:
  - `include/nds/renderer_fighter_lean.h`
  - `src/nds/nds_renderer_native_common.c` (the lean section)
  - `src/nds/nds_ftr_lean_kernel.c` (12-word LOAD4x3 output)
  - `src/port/renderer_fighter_lean.c`
  - `src/port/renderer_adapter_fighter.c` (call site)
- **Host**:
  - `scripts/fighters/fighter_list_emitter.py` (new)
  - `scripts/fighters/fighter_list_proof.py` (new)
  - `scripts/fighters/check_nds_native_owner_packet.py` (`--fighter-lists*`)
- **Tools** (`tools/`):
  - `build-s4.ps1` (`-ForceParticleDeps`)
  - `build-base-s4.ps1`: builds the pre-slice-4 sources under the lock, then restores and hash-checks them
  - `run-s4.ps1`, `run-base.ps1`
  - `kinds4.py`, `gates4.py`, `ramdelta4.py`
  - capture and debug helpers: `dump-packets.ps1`, `pkt.py`, `match.py`, `cmp.py`, `hostlist.py`, `cost4.py`, `decisions.py`
- **Evidence**:
  - arms `b8-*`: the gate ROM
  - `base-route0`, `base-route1`: pre-slice-4 on today's tree
  - development arms `b1`-`b7`
  - `digest-*.json`, `gates-final.txt`, `gates-route0.txt`, `kinds-final.txt`
  - `ram-delta-shipping.txt`, `ram-delta-tickhud.txt`
  - `host-proof.*`, `canonical-sha-final.txt`
  - build logs
