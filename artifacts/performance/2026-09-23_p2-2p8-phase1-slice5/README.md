# P2-2p8 Phase 1 slice 5: the lean draw's own cost

Slice 5 attributes the lean fighter draw part by part, then cuts the parts that did work nobody needed:
- The **camera LookAt** runs once a frame instead of once a fighter (head -4.5 to -4.9K a draw for three of the four fighters).
- The **patch** reads a per-draw view, not the shared production inputs, which are no longer refreshed every draw.
- A **material event keeps its plan** (no resolve, no joint-table rebuild) when a held entry has the new key: 128 of 140 events; event-path ticks -2.36M over the match (-16.9K per material event).
- The **joint kernel and the list's draw code run from ITCM** (ARM). The production execute leaves ITCM to fund them (the lean kinds' miss path; section 4 prices what that costs non-lean fighters).
- The submit stops filling a 1,300 B stats block every draw.

Route 1 FTR mean falls 236,827 to 194,898 (-41.9K, -17.7%), and WORK-H minus STG falls by the same 41.8K. STG's own -104K is a layout effect that route 0 shows too (section 4), and it is not claimed. Shipping static RAM falls 136 B.

Gate ROM: `smash64ds-p2-fourcpu-tickhud-hwtri.nds`, sha256 `c366e5d4ab4abd576b6d336ba75d9d33d67b6373c6c1e6cc130018ec349120f3` (`canonical-sha-final.txt`). Gate arms: `a5-route0`, `a5-route1`, `a5-route2`, and the verify arm `a5-route1-verify`. Pre: `pre-route0`, `pre-route1` on ROM `5d68834f...` (`build-p2p8-s5-pre`: today's HEAD `fd3018730b2`, with the pre-slice-5 sources of the six files this slice edits, built the same way). Four-CPU stress, admit word 2, 1,972 samples, runner slot 9.

## Gate table

| gate | slice 5 | reference | verdict |
|---|---|---|---|
| replay digest, route 1 vs route 0 (same ROM) | **IDENTICAL**, 1,972 frames | also identical: route 2 vs 0, verify arm vs 0, pre route 0 vs route 1, pre route 0 vs slice 5 route 0 | **met** (`digest-*.json`) |
| oracle, route 2 | **0 mismatches** per kind: DK 0/1,660 runs, Samus 0/1,663, Link 0/1,735, Kirby 0/1,548; 13,620,081 words | clip row 3 max 1 LSB; `donor_memo` 192 and `record_diff` shade 98, both as slice 4 named them | **met** |
| verify arm (`gNdsFtrLeanSlow` bit 16) | **0 mismatches**: 165 re-selected lists (134 on variant entries); **140 kept plans resolved anyway, 0 differences** | new this slice: the kept-plan compare (`ndsFtrLeanVerifyPlan`) | **met** |
| native failures | 39 in route 0 = 39 in route 1 = 39 in route 2 | 39 pre | **met** |
| lean share, declines | **100.00%** per kind (1,725 / 1,685 / 1,824 / 1,628 draws), **0** declines | pre: the same | **met** |
| FTR P50 / P95 / P99, route 1 | **183,552 / 268,333 / 766,360** (mean 194,898) | pre: 228,992 / 321,715 / 812,019 (mean 236,827) | **fell** |
| WORK-H P50 / P95 / P99, route 1 | **1,404,128 / 2,031,187 / 2,690,529** (mean 1,455,092) | pre: 1,556,096 / 2,190,358 / 2,815,923 (mean 1,600,900) | **fell**; see the stop rule |
| stop rule (FTR falls, WORK-H does not) | **not fired.** WORK-H minus STG mean -41,756 against FTR mean -41,929 | STG also fell 104K, in route 0 too: layout, not a lever (section 4) | **not fired** |
| per-draw cost per kind | table in section 3 | | reported |
| shipping static RAM | **-136 B** main RAM: .main +1,144, .main.bss -1,280; ITCM -648; DTCM 0 | pre-slice-5 sources, same tree, non-tick-HUD shell | **met** (must not grow) |
| ITCM / DTCM, gate ROM | ITCM 32,208 to 32,280 B (456 B free); DTCM 8,800 + 2,028, unchanged | | reported |
| heap low-water, gate ROM | general heap 122,412 B (pre 122,164); libc top chunk min 19,200 (pre 18,784); arena 1,257,216 (pre 1,236,736) | | reported |

## 1. Attribution

`gNdsFtrLeanAttr` (lab only: a tick-HUD build made with `NDS_FTR_LEAN_KTIME=1`) splits every lean draw per kind: head (4 parts), guard (8), patch (8), submit (4), book, the event path's 8 steps and the kernel's parts. Two kernel arms run on the same ROM:
- `gNdsFtrLeanSlow` bit 32 (`KERNEL_RERUN`): after the draw's own pass, the first 8 joints (a preorder prefix) run twice more (the second is code- and data-warm), then once after `DC_FlushAll` (data cold, code warm). Every pass writes the same words to the same sites.
- bit 64 (`KERNEL_QUIET`): the previous fighter's list DMA is drained before the kernel reads a DObj.

Base: the pre-slice-5 sources plus the instrument (`ktime-a`, `ktime-quiet`, `ktime-rerun`, `ktime-joint`; ROM `7062bbe5`). Final: `ktime5-a`, `ktime5-rerun`, `ktime5-joint` (ROM `5014e9dc`). The attribution ROM's timers cost about 160 ticks a part; the gate ROM runs the guard and patch part timers too, and shipping runs none.

**Where the guard's 5.5-14K a draw went (base).**
- The guard window held the event paths. Amortized per draw: DK 9.9K, Link 9.5K, Kirby 2.7K, Samus 2.3K.
  - A held-entry event cost ~57K: resolve 9.2K, program + validate 9.6K, rows + key 16.5K, refresh + identity + re-record key 5.9K, held 5.6K, joint table 9.6K, watch + proofs 2.7K.
  - A materialization cost ~470K on top.
- The per-draw proofs cost 3.1-4.7K: tuple 620-770; camera 230-300 (DK 1,160: it draws first after the camera moves); identity watch 380-650; preamble 365-685; packet guard ~550; shuffle latch ~300; topology ~160.

**Where the patch's 4.4-15K went (base).**
- Link's texgen: 8.7K.
- The per-draw refresh of the shared production inputs: 1.3-2.7K.
- The tint shade re-derive: 0.6-3.7K. DK pays 3.7K because its flashing modulate re-derives every site, three divides a channel.
- P' 0.9K, the program select 0.4K, light 0.3K.

**Head (base).** 18-21K a draw: capture setup + camera LookAt 6.7-7.5K, the head 3.9-4.4K, the display walk 5.4-7.7K, finish 0.7-1.3K.

**Kernel: stall against arithmetic** (ticks a joint; the re-run columns cover the first 8 joints of each draw; `ktime*-rerun`, `ktime*-joint`):

| kind | cold pass | code warm, data cold | code + data warm | parts (bit 8): fast local / compose / output |
|---|---|---|---|---|
| DK | 949 → 818 | 860 → 868 | 626 → 623 | 483 / 291 / 192 → 401 / 265 / 177 |
| Samus | 899 → 776 | 838 → 824 | 584 → 574 | 434 / 291 / 185 → 482 / 263 / 170 |
| Link | 971 → 842 | 852 → 840 | 602 → 591 | 489 / 290 / 212 → 400 / 267 / 194 |
| Kirby | 889 → 713 | 789 → 773 | 541 → 510 | 495 / 302 / 137 → 380 / 271 / 116 |

- **Base** (ARM, main RAM): instruction fetch (cold minus code-warm) ~60-120 a joint; data stall (code-warm minus warm) ~235-255; arithmetic (warm) 541-626.
- **Final** (ARM, ITCM): the cold pass is now at or below the code-warm figure, so the instruction fetch is gone. The data stall is unchanged at ~245-265 a joint (the DObj lines: TRS floats, parent, FTParts, read once per draw). The arithmetic is unchanged at 510-623; the kernel was already ARM.
- Against the spec's ~450 estimate (fast TRS ~150, compose ~110, output ~40, loads/stall ~150): the fast local (float-to-fixed conversion, sin/cos) reads ~380-480, the compose ~265, the output ~115-195. Those parts include their timers: the part-timed pass reads 892-1,057 a joint against 720-848 untimed (`ktime5-a`), so ~160-270 a joint is timer.
- The quiet arm (base) drained 73-75 ticks of DMA per draw: the previous list's stream does not stall the kernel.

**Final attribution (`ktime5-a`) against base (`ktime-a`), per draw:**
- Head setup + LookAt: Samus 7,416 to 2,945, Link 6,681 to 2,392, Kirby 7,457 to 2,591. DK 7,340 to 7,666: it draws first, after the camera moves, and pays the frame's one LookAt.
- Guard: the packet guard 555 to 222-239 (ITCM); the event path's resolve and program + validate ran 108 times instead of 236, and the joint table 108 of 236.
- Patch: refresh 1.3-2.7K to ~300 (the view build); DK's tint core 3,701 to 2,734 (in main RAM on this ROM, see section 5).
- Submit: stats 1.2-1.5K to ~250; flush and DMA about unchanged (they are the list's own lines).

## 2. Levers, in the brief's order

Each number is a same-ROM A/B (a lab bit restores the old form) or a build-to-build step over the same match (the replay digest is identical in every arm). The build ladder is in `gates-ladder.txt`.

| step | change | FTR mean | FTR P95 | kernel /joint (DK, Samus, Link, Kirby) |
|---|---|---:|---:|---|
| pre | | 236,827 | 321,715 | 931, 874, 952, 860 |
| a1, bits 128 + 256 | a1's ROM with the LookAt every draw and the input refresh (the slice 4 forms) | 223,113 | 305,146 | 759, 742, 778, 676 |
| a1 | production execute out of ITCM; kernel in ITCM with the sin half-table, inlined angle index, one cell conversion; LookAt once a frame; patch view | 200,851 | 282,070 | 759, 739, 775, 674 |
| a2 | + the skipped LookAt keeps its heap footprint (memo fills 1,786 back to 1,662) | 199,382 | 280,960 | 760, 742, 776, 673 |
| a2nosin | a2 with the sin half-table off (one-edit variant) | 203,554 | 286,138 | 810, 783, 823, 711 |
| a3 | sin table off; its 2 KB of ITCM and the rest of the freed space take the draw code | 197,426 | 278,742 | 896, 774, 815, 697 |
| a4 | + kept plan on material events; shared tint core; no stats block | 195,238 | 268,864 | 805, 776, 820, 706 |
| **a5 (gate)** | + RAM offsets (LookAt hash out of line, sites bit 31), kept-plan verify | **194,898** | **268,333** | **804, 779, 822, 704** |

- The LookAt and refresh pair is worth -22.3K of FTR mean on one ROM (a1 against its bits 128 + 256 arm).
- The pre-to-a1 kernel step (-135 to -186 a joint) is the ITCM move plus the sin table (-40 to -50 of it: a2 against a2nosin), the inlined angle index and one cell conversion.
- DK's a3 kernel (896) did not persist: a4 and a5 read 805 and 804.

### 2.1 Head: the camera LookAt runs when it can write something new

`ndsFighterDisplayContractCapture` (`src/port/renderer_adapter_fighter.c`) called `gmCameraLookAtFuncMatrix` for every fighter, every frame, for the same camera.
- It is now called when the CObj differs from the last call's, when its inputs hash differently (eye, at, up, the five perspective words, the two camera-path words), or when its outputs (`gGMCameraMatrix`, the look-at, the perspective norm) no longer hash as that call left them. Otherwise the call would rewrite the values already there.
- A skipped call keeps its graphics-heap footprint (one Mtx below camera path level 3), so every later head allocation lands where it did. This matters: the head's scene Light is copied whole, uninitialised colour bytes included, into the draw memo's key. Moving it 64 B cost 124 extra memo fills a match in a development build.
- One call a frame remains, paid by the first fighter drawn after the camera moves (DK, slot 0).
- `gNdsFtrLeanSlow` bit 128 calls it every time. Route 0 takes the same skip (section 4).

### 2.2 Guard: event-time proofs; per-frame checks only for per-frame inputs

The guard was already built from writer-side event counters. Slice 5 changed where the event work sits and what it costs, not what it proves.
- **Event-time** (a writer bumps a counter; the guard compares one word): status generation (with the re-tuple proof), texture-part serial, the preamble hash (once per draw-memo fill), the tint-tile set generation, the texture fence, the slot rebind. Topology in route 1 is the kernel's per-joint parent check.
- **Per frame, because the input changes per frame**, each a few loads:
  - the camera matrices (once per frame per camera, cached);
  - the colour modulate and the electric-skeleton state (colour animation);
  - the material identity watch (live material animations write the watched words every frame);
  - the shuffle offset (moves every shuffle frame);
  - the tuple compares: detail, heap generation, root joint and owner file identity, five words with no single writer to hook.
- **The patch view** (`NDSFtrLeanPatchView`): the patch reads the preambles through the instance's event indices, the modulate, P' and the binding worlds directly. The shared production inputs are refreshed only for a texgen list without a site map (bit 31 of the modelview-sites mask, `NDS_FTR_LEAN_SITES_INPUTS`) and in the slice 4 form (bit 256).
- **A material event keeps the plan.** A material event means the identity moved and nothing else did, so the resolve would return the instance's plan.
  - The event rebuilds only the material rows and the key; a held entry takes the draw.
  - A key no entry holds, a moved MObj count or a moved Kirby head key restarts it as a full event (12 of 140 did).
  - The joint table is kept, as every event-free draw keeps it: the kernel proves each kept joint's parent link per draw and rebuilds on a refusal. That added 16 rebuilds over the match (kernel refusals 57 to 73).
  - The verify arm resolves every kept plan anyway and compares it field by field (`ndsFtrLeanVerifyPlan`): 140 compared, 0 differences. Route 2 has one lean entry, so every one of its material events restarts; the verify arm is the kept plan's proof.
  - Event ticks over the match: 42,120,960 to 39,760,832.

### 2.3 Kernel: ARM in ITCM

- `ndsFtrLeanKernelCompose` now runs from ITCM (`section(".itcm.ftr_lean")`); it was already ARM, in main RAM. `ndsFtrLeanAngleIndex` is inlined, and the fast local converts its cells once (`ndsFtrLeanTrsCells` called once, the scale test computed once).
- **Sin half-table**: a 2 KB copy of `gSYSinTable`'s first half in ITCM (`NDS_FTR_LEAN_SIN_ITCM`) was measured at -40 to -50 ticks a joint (a2 against a2nosin). It is left off, because those 2 KB of ITCM went to the draw code in 2.4. The table is exact: `gSYSinTable` is symmetric about 0x3ff.5, checked over all 2,048 entries on the host, and the lab fill counts mismatches.
- **DTCM joint scratch** was not done. The spec places it at step 8 (Mario's freed tables). DTCM today has 5,172 B between `.dtcm.bss` and the user stack top, and the per-depth world stack is cache-resident after its first touch.
- **ITCM funding**: the production execute leaves ITCM in builds that compile a lean kind (`NDS_FTR_LEAN_EVICT_PRODUCTION`). That is 7,880 B: `ndsRendererExecuteNativeFighterOwnerProduction`, `PrepareProductionRun`, `ShadeProductionActions`, `EmitProductionPrimitiveGroups` and `EmitProductionCrossRun`.
  - It is the lean kinds' miss path (0 declines).
  - The spec's R10 names the cost: record frames of non-lean fighters, and route 0, run it from main RAM. Section 4 prices it.
  - Builds without a lean kind keep it in ITCM by construction, including P1's `smash64ds-battle-playable-hwtri`, whose placement `scripts/check-renderer-itcm-placement.ps1` pins. No P1 target was rebuilt in this slice.

### 2.4 Patch and flush

- **The draw code is in ITCM** (`section(".itcm.ftr_lean_draw")`): the active-entry lookup, the packet guard, the modelview sites, the dirty-line flush and submit, and the tint shade core. Every draw ran these 3,328 B cold from main RAM, after the head.
- **The tint shade core is shared** (`ndsFighterPacketApplyTintPrims`): the replay's caller and the lean path's pass the roots' prim colours. A site whose inputs equal the previous site's takes that site's word; under a flashing modulate each `MaterialColor15` costs three divides a channel.
- **The submit** does its frame accounting before it starts the DMA and returns the hardware triangle count. The 1,300 B `NDSRendererStats` static it filled every draw is gone; the materializer keeps its renderer state on its own stack, as the old path's `DrawForSlot` does.
- **The flush was already line-exact**: 1,422 B a draw, which is the modelview sites (12 words a root) plus any patched word. Nothing is left to cut there without cutting the patches.

### 2.5 Events: DK's hand swaps

Not attempted (optional in the brief). DK still materializes 26 times (530K each), as in slice 4.

## 3. Per kind, per lean draw, route 1 (`kinds-final.txt`)

Ticks per draw. Guard includes the event paths, which are also shown alone; g-net is the guard without them. Sum = guard + kernel + patch + submit, as slice 4 reported it; head is apart.

| kind | head | guard | event | g-net | kernel | patch | submit | **sum** | kernel /joint |
|---|---|---|---|---|---|---|---|---|---|
| DK | 20,132 → 20,808 | 14,034 → 13,724 | 9,717 → 9,694 | 4,318 → 4,030 | 23,307 → 20,124 | 8,841 → 5,499 | 4,350 → 2,650 | **50,533 → 41,997** | 931 → 804 |
| Samus | 17,986 → 13,307 | 5,742 → 5,185 | 2,290 → 2,275 | 3,452 → 2,910 | 19,643 → 17,507 | 6,089 → 3,497 | 4,015 → 2,439 | **35,488 → 28,628** | 874 → 779 |
| Link | 18,450 → 13,921 | 12,958 → 11,612 | 9,431 → 8,641 | 3,527 → 2,972 | 25,831 → 22,313 | 14,874 → 11,958 | 4,561 → 2,606 | **58,224 → 48,490** | 952 → 822 |
| Kirby | 17,105 → 12,198 | 5,700 → 4,652 | 2,641 → 2,116 | 3,059 → 2,536 | 18,452 → 15,101 | 4,443 → 3,076 | 3,686 → 2,025 | **32,282 → 24,854** | 860 → 704 |

With the head, a draw costs DK 70.7K → 62.8K, Samus 53.5K → 41.9K, Link 76.7K → 62.4K, Kirby 49.4K → 37.1K. Book (per draw, all kinds): 3,071 → 2,652.

**Events and materializations** (unchanged by design):
- 236 events: first 4, rebind 87, material 140, tint-set 5.
- 62 materializations (DK 26, Samus 10, Link 15, Kirby 11), 174 entry hits (156 switches), 12 variants learned, 144 variant switches, 9 tint repatches, 18 re-record resets.
- New: 128 kept plans, 12 restarts.

## 4. Performance

`gates-final.txt`: bands by WORK-H rank, per-range means (frame 3, the admission frame, apart).

| band | pre route 1 | slice 5 route 1 |
|---|---:|---:|
| FTR P40-60 | 233,945 | 189,769 |
| FTR P90-95 | 332,065 | 302,417 |
| FTR P95-99 | 441,022 | 379,407 |
| FTR P99+ | 573,062 | 539,318 |
| WORK-H P40-60 | 1,557,535 | 1,405,661 |
| WORK-H P90-95 | 2,094,191 | 1,943,338 |
| WORK-H P95-99 | 2,386,413 | 2,245,848 |
| WORK-H P99+ | 4,269,821 | 4,168,029 |

| range | FTR pre / s5 | WORK-H pre / s5 |
|---|---|---|
| 2-195 entry | 102,533 / 87,659 | 1,065,565 / 974,672 |
| 196-797 | 256,176 / 210,425 | 1,549,782 / 1,396,142 |
| 798-1043 P0 episode | 264,466 / 216,224 | 1,751,364 / 1,594,288 |
| 1044-1974 | 245,097 / 201,659 | 1,678,488 / 1,529,268 |

VBlank shares, route 1: 2-VBlank 6.1% → 10.1%, 3-VBlank 57.0% → 70.1%, 4-VBlank 32.3% → 17.0%, 5+ 4.5% → 2.8%.

**Stop rule.** FTR mean fell 41,929. WORK-H minus STG fell 41,756 (mean; P50 1,120,064 → 1,077,024, P95 1,754,720 → 1,698,093): the rest of the frame did not absorb the cut. The other work buckets are flat pre → final (SRC +0.7K, MISC -0.1K, AUD -0.4K, HUD -0.4K). OTHR +18K is the VBlank wait inside it (WAIT +18K) of frames that now finish earlier. One step moved the other way: a3 → a4 cut FTR 2.2K while SRC rose 2.4K (SINT +3.7K), and a5 left SRC 0.7K above pre. That is within the build-to-build layout spread of these buckets, and it does not survive into the slice's total.

**STG is not this slice's.** STG fell 435,506 → 331,455 (mean) in route 1 **and** route 0 (435,521 → 330,780). No stage code changed. The drop arrived in one step (a2 441K → a3 332K): the build that took the 3.3K of draw code out of main RAM and so shifted the main-RAM code behind it. STG is known to be I-cache-layout sensitive. Of WORK-H's -145.8K, about 104K is this layout effect, and a later layout change can take it back. The FTR and WORK-H minus STG numbers are the slice's own.

**Route 0** (`gates-route0.txt`; pre route 0 against slice 5 route 0; the digest is identical):

| metric | pre route 0 | slice 5 route 0 |
|---|---:|---:|
| FTR P50 | 358,624 | 344,800 |
| FTR P95 | 910,240 | 992,627 |
| FTR P99 | 1,162,496 | 1,312,798 |
| FTR mean | 384,500 | 381,177 |
| WORK-H P50 | 1,690,208 | 1,566,880 |
| WORK-H P99 | 3,070,051 | 3,084,660 |

- Paired per frame, route 0's FTR moved -17K to -10K from P5 to P75: replay frames gain the LookAt skip and the shared tint core.
- It moved **+88K at P95 and +154K at P99**. These are record and memo-fill frames, which now run the production execute from main RAM. This is R10's named cost ("slows record frames ... until step 8 (tail, not P50)").
- The lean kinds never pay it in route 1: 0 declines, 0 records. It applies to the non-lean kinds in a mixed roster, until coverage (step 7) makes production their miss path too.

## 5. RAM

**Shipping static** (`ram-delta-shipping.txt`, `ram-delta-shipping-symbols.txt`):
- Measured with nm plus section sizes on `smash64ds-p2-shell-hwtri` (non-tick-HUD).
- Pre-slice-5 sources (`build-p2p8-s5-shipcheck-pre`, ROM `26052668...`) against the final sources (`build-p2p8-s5-shipcheck`, ROM `dae44c45...`), same tree. Each was re-made until its sha held.

| section | pre | final | delta |
|---|---:|---:|---:|
| .main (code + rodata) | 1,610,804 | 1,611,948 | +1,144 |
| .main.bss | 906,064 | 904,784 | -1,280 |
| **main RAM, net** | | | **-136** |
| .itcm (not counted) | 31,440 | 30,792 | -648 |
| .dtcm / .dtcm.bss | 8,800 / 2,000 | 8,800 / 2,000 | 0 |

- **Left main RAM**:
  - into ITCM: the kernel (4,620 B) and the draw code (submit 1,340, guard 472, modelview sites 384);
  - `sNdsFtrLeanStats` (1,300);
  - the replay's tint wrapper (436; it now calls the shared core);
  - `ndsFtrLeanAngleIndex` (256; inlined into the kernel).
- **Entered main RAM**:
  - the production execute (7,880 B, from ITCM);
  - `ndsFtrLeanTintTileWords` (252; out of line now that its caller's neighbours are ARM/ITCM);
  - the LookAt skip: +204 in `ndsFighterDisplayContractSubmit`, 190 B of hash helpers, 12 B of statics;
  - the kept-plan event path (+104), and smaller moves.
- **Offsets made to get there**: the LookAt hash helpers are out of line (their inlined, unrolled copies cost ~230 B more), and the per-event `ndsFtrLeanPacketNeedsInputs` (144 B) became bit 31 of the modelview-sites mask.

**Tick-HUD gate image** (`ram-delta-tickhud.txt`): .main +880, .main.bss -1,280 (net -400).
- ITCM: 32,208 → 32,280 (456 B free of 32,736).
- The new ITCM contents: kernel 4,640; submit 1,716; guard 516; modelview sites 484; tint core 476; active-entry lookup 136. That is 7,968 B, funded by the 7,880 B production execute.
- DTCM unchanged (8,800 + 2,028).
- The attribution ROM's extra timers overflowed ITCM by 64 B. There, and only there, the tint core stays in main RAM (`NDS_FTR_LEAN_TINT_CODE`), so its `apply_tint` part reads a little high.

**Heap on the gate ROM** (compaction on):
- General-heap low-water: 122,412 B (pre 122,164).
- Libc top-chunk minimum: 19,200 (pre 18,784).
- Arena chosen: 1,257,216 (pre 1,236,736; 5 pages more; the arena first grew at a3).
- Arena alloc failures: 107 (pre 112).

## 6. Deviations and open items

1. **The production execute left ITCM.** The spec's R10 evicts `PrepareProductionRun` (2.4 KB). This slice evicts the whole 7.9 KB execute, to place the list's draw code as well as the kernel. Route 0's record frames pay +88K (P95) and +154K (P99); non-lean kinds in a mixed roster pay the same on their record frames until coverage. Keeping it would mean giving up either the draw code's ITCM (~3.3K a draw of cold fetch, 2.4) or the kernel's.
2. **STG's -104K is layout** (section 4), in both routes. It is not claimed.
3. **Kernel at 704-822 ticks a joint, not ~450.** The rest is data stall on the DObj lines and the fast local's float-to-fixed conversion (section 1). Next levers are outside this slice: DTCM joint scratch (step 8, when DTCM is freed), and a pose-side Q12 local (arch doc A3), which removes the float conversion.
4. **Head is now the largest part**: 12-21K a draw (the display walk 4.9-7.6K, the head 3.5-4.1K). The spec keeps the head in Phase 1.
5. **DK materializations** (26 x ~530K) are unchanged. They need shape-changing variants or a third entry.
6. **Kept plans are proven by the verify arm, not by the oracle.** Route 2 has one lean entry, so all 140 of its material events restart. The verify arm's plan compare covers every kept plan: 140 compared, 0 differences.
7. **Pre-slice-5 runs reproduce the start-of-day base** (`base-*`, ROM `c7e16a54`, HEAD `c0e17ce24e9`) byte for byte in every row and counter, although two commits landed in between. `pre-*` are the reference in this README; `base-*` is kept.
8. **Canonical sha**: `c366e5d4...` came from a5 and was reproduced after every later build: the pre builds, the KTIME build and the shell re-makes (`build-final-recheck.log`, `build-final-remake2.log`). Each first make after a shell build compiled the particle-bank objects before the generator rewrote the header (`8d4efd80...`, not used as evidence); the second make restored `c366e5d4`. The same held for the shell pair: `7bc9bedf...` was an intermediate, and `dae44c45...` is stable.
9. **Rules.** No subagents. The build lock was held for every build. Slot 9 only. Nothing under `decomp/`. None of the do-not-touch files and no Makefile default was edited. **One read-only `git status --porcelain` was run by mistake** while listing the final files; its output was discarded, and git may have refreshed its index stat cache. No other git command was run (the sampler reads HEAD for its header line, as in every slice).

**Not verified in this slice:**
- P1 and the published targets were not rebuilt. Their ITCM placement and head are unchanged by construction (both are under `NDS_FTR_LEAN_KINDS_BUILD`). The shared tint core also compiles into P1's replay; it was verified here only through the gate ROM (route 2's oracle on the lean lists that use it, and route 0).
- Mixed rosters (lean and non-lean kinds in one match) were not run. Route 0 stands in for the non-lean cost (section 4).
- The texgen-without-site-map refresh (`NDS_FTR_LEAN_SITES_INPUTS`) has no counter; whether this match takes it is not measured. Route 2 and the verify arm pass either way.
- Route 3, high-detail lists and screen captures were not run. Exactness rests on the oracle, the verify arm and the digest, as in slice 4.

## 7. Files

- **Runtime**:
  - `include/nds/renderer_fighter_lean.h`: patch view, sites bit 31, attribution block, slow bits 32-256, kept-plan counters
  - `src/nds/nds_ftr_lean_kernel.c`: ITCM placement, inlined angle index, one cell conversion, sin half-table (off)
  - `src/nds/nds_renderer_native_common.c`: draw code in ITCM, shared tint core, view-based patch, submit accounting
  - `src/nds/nds_renderer_preamble.c`: `NDS_FTR_LEAN_EVICT_PRODUCTION`
  - `src/port/renderer_adapter_fighter.c`: LookAt skip, head attribution
  - `src/port/renderer_fighter_lean.c`: view, kept plan, `ndsFtrLeanVerifyPlan`, attribution parts, no stats static
- **Tools** (`tools/`):
  - `build-s5.ps1` (lock, `-SeedFrom`, `-ForceParticleDeps`)
  - `build-pre-s5.ps1`: builds the pre-slice-5 sources under the lock, then restores and hash-checks them
  - `build-variant-s5.ps1`: one-edit A/B under the lock
  - `run-s5.ps1` (slot 9)
  - `gates5.py`, `kinds5.py`, `ramdelta4.py`, `cost4.py`
- **Evidence**:
  - gate arms `a5-*`; pre arms `pre-*`; start-of-day `base-*`
  - development arms `a1`-`a4`, `a2nosin`, `a1-route1-slow384`
  - attribution: `ktime-*` (base), `ktime5-*` (final)
  - `digest-*.json`, `gates-final.txt`, `gates-route0.txt`, `gates-ladder.txt`, `kinds-final.txt`, `kinds-ktime.txt`
  - `ram-delta-shipping.txt`, `ram-delta-shipping-symbols.txt`, `ram-delta-tickhud.txt`
  - `canonical-sha-final.txt`, build logs
