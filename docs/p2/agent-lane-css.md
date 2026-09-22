# CSS lane — three-row diagnosis (read-only agent, 2026-09-22)

Scope: the three character-select rows the owner re-opened after playtesting r36.
Read-only pass; no file under `src/`, `include/`, `scripts/` or `assets/` was
touched. Every claim below cites `file:line` against the current worktree.

Conventions used throughout:

- **"update" / "tic"** = one iteration of the shell loop, i.e. one
  `ndsMenuShellCssSyncPreviews()` call (`src/nds/nds_menu_shell_router.c:253`).
  The CSS presents at 60 Hz (P2-1h evidence: 1,612 presents, VBlank interval 1
  on every one), so one update is one 60 Hz tic.
- The **3D preview pass is 30 Hz**: `ndsMNPlayersVSPreviewFrame` submits GX only
  on alternating tics (`src/import/battleship_mnplayersvs.c:2380-2402`,
  `sNdsPlayersVSPreviewDrawPhase ^= 1u`). Source state (rotation, figatree,
  status) still advances every tic. So a latency of N updates presents in N or
  N+1 tics.

---

## Row 1 — hover-to-preview delay

```
Bug: "delay between cursor hover and 3d fighter preview rendering."
```

### Contract

The CSS is a menu screen, not a frame-budget-critical scene; the source loads a
hovered fighter's closure and shows it. The port's own design contract is
written into the constants it ships:

- `src/import/battleship_mnplayersvs.c:115-125` —
  `NDS_PLAYERS_VS_PREVIEW_SERVICE_BYTES (8u * 1024u)`, documented as
  *"ONE aggregate read/decode span per CSS update, shared by all four slots — not
  a budget each. The compact transaction spends it in units of
  NDS_PLAYERS_VS_LOAD_STEP_BYTES"*, and sized explicitly: *"the largest shipped
  preview pack is Link's at 32,032 data bytes, so 8 KiB per update finishes its
  read in four updates while leaving the frame's own work room."*
- `src/import/battleship_mnplayersvs.c:126` —
  `NDS_PLAYERS_VS_LOAD_STEP_BYTES (2u * 1024u)`, the unit that bounds how long
  one work item may hold the filesystem mutex (the M03 audio-gap bound).
- `src/import/battleship_mnplayersvs.c:134-135` — dwell is **2** cold, **1**
  warm, not 13.

So the contract is: **8 KiB of pack payload per CSS update, moved in ≤2 KiB
units.** Link should finish its read in four updates.

### Divergence — the aggregate budget is never spent

`src/import/battleship_mnplayersvs.c:1141-1153`:

```c
        do
        {
            step_bytes = 0u;
            step = ndsRelocPreviewFighterLoadStep(block->load_cursor,
                                                  (drain != FALSE) ? 0u : budget,
                                                  &step_bytes);
            gNdsPlayersVSPreviewServiceStepCount++;
            unit_bytes += step_bytes;
            ...
        } while ((drain != FALSE) && (step == NDS_PREVIEW_PACK_STEP_IN_PROGRESS));
```

The loop continues **only when `drain != FALSE`**, and `drain` is
`sNdsPlayersVSPreviewEntryDrain` (`:1071`), which is TRUE only on the router's
entry sync and FALSE on every live browsing tic
(`src/import/battleship_mnplayersvs.c:1757-1766`). So during cursor browsing the
service call issues **exactly one bounded step per update**.

The outer scheduler does not make up for it either. Per CSS update:
`ndsMenuShellCssSyncPreviews` (`src/nds/nds_menu_shell_css.c:2874-2896`) calls
`ndsMNPlayersVSPreviewSyncRules` **once** (refreshing the 8 KiB budget at
`:1779-1780` and the residency action budget to **1** at `:1764`), then
`ndsMNPlayersVSPreviewSync` once per slot. A slot with `acquire_pending` reaches
`ndsMNPlayersVSPreviewServiceCompactLoad` exactly once
(`:2006-2010` → `:1269` → `:1377`). One pending fighter therefore gets **one**
service call per update.

**Bytes actually moved by one pending fighter, per update:**

| pack phase | per step | per update today | budget allowed |
|---|---|---|---|
| READ (`reloc_preview_pack.c:626-638`) | `min(remaining, 2048)` | **2,048 B** | 8,192 B |
| FIXUP (`reloc_preview_pack.c:664-671`) | 32 records = **256 B** (`chunk` capped at `ARRAY_COUNT(batch)`=32 before the byte clamp ever binds at a 2 KiB budget) | **256 B** | 8,192 B |
| SPANS | `span_count*12` ≈ 324 B | 324 B | 8,192 B |
| REGISTER | 0 B | 0 B | 8,192 B |

So the screen spends **25 % of its own budget during READ and 3 % during
FIXUP**. The comment's "four updates for Link" is 26 updates in the shipped
binary.

### The arithmetic reproduces the recorded measurement exactly

Cold-acquire cost, counted off the state machine:

- 2 updates of dwell (`:2111-2136`; the second update is the commit, which
  destroys the old fighter and returns with `acquire_pending = TRUE`, `:2203`),
- 1 update to retire a zero-ref victim block when all four are occupied
  (`:1349-1362`),
- 1 update for the transaction open, which takes a residency action and then
  returns Retry without moving a payload byte (`:1075-1110`),
- `ceil(data_bytes/2048)` READ updates + `ceil(fixups/32)` FIXUP updates
  + 1 SPANS + 1 REGISTER,
- 5 prepare updates — `COMMIT, ANIM_IDLE, ANIM_SELECTED, OWNER_HIGH, OWNER_LOW`
  (`:141-148`), each taking the single per-update residency action and returning
  Retry (`:1192-1210`, `:1243-1248`).

Header fields read out of the shipped packs
(`builds/build/nitrofs/fighters/preview/*.fpc`, `NDSPreviewPackHeader`,
`include/nds/nds_preview_pack.h:23-42`):

| kind | data B | fixups | updates today | after fix A | after A+B |
|---|---|---|---|---|---|
| Mario | 10,192 | 149 | **21** | 11 | 10 |
| Fox | 9,948 | 158 | **21** | 11 | 10 |
| Luigi | 11,700 | 172 | **23** | 11 | 10 |
| Purin | 11,872 | 150 | **22** | 11 | 10 |
| Ness | 12,688 | 182 | **24** | 11 | 10 |
| Kirby | 17,212 | 229 | **28** | 12 | 11 |
| Yoshi | 16,916 | 281 | **29** | 12 | 11 |
| Pikachu | 18,944 | 250 | **29** | 12 | 11 |
| Donkey | 15,424 | 365 | **31** | 12 | 11 |
| Samus | 21,636 | 326 | **33** | 12 | 11 |
| Captain | 17,008 | 385 | **33** | 12 | 11 |
| Link | 32,032 | 244 | **35** | 14 | 13 |

The recorded CSS-tour reading is `kindMask=0xFFF drewMask=0xC13` at
`NDS_CSS_WALK_TOUR_HOLD_TICS = 24` (`src/nds/nds_menu_shell_css.c:2349`).
`0xC13` = bits 0,1,4,10,11 = **Mario, Fox, Luigi, Purin, Ness** — and those are
exactly and only the five kinds this model puts at ≤24 updates. The next
cheapest kind is Kirby at 28. The partition is reproduced with no fitted
parameter, which is as close to a proof as a static read gets.

Worst case today is Link at 35 updates ≈ **0.58 s**, and because the 3D pass is
30 Hz it can present one tic later still.

**Two stale claims in the tree, corrected:**

1. `src/nds/nds_menu_shell_css.c:2343-2344` says *"`NDS_PLAYERS_VS_PREVIEW_DWELL_TICKS`
   is 13 before a closure load may begin"*. It is **2**
   (`src/import/battleship_mnplayersvs.c:134`). The comment predates the M04
   dwell reduction and its "24 tics is the floor plus margin" derivation is
   therefore built on the wrong term — the real term is the 26-update tree walk,
   not the dwell.
2. `docs/p2/BUGS_REMAINING_DIAGNOSIS_2026-09-19.md:207-221` ("SAME ROOT CAUSE AS
   THE CSS BLOCKING LOAD … do not independently set dwell to zero") is
   **obsolete**: the resumable loader landed and the dwell is already 2/1. The
   residual named in `docs/p2/REMAINING_BUGS_IMPLEMENTATION_PLAN_2026-09-21.md:319`
   ("one 30,160-byte owner-image read") is real but is only 1 of 35 updates; it
   is not the delay.

### Root cause

The service loop's continuation condition tests the *entry-drain flag* instead
of the *remaining aggregate byte budget*. The 8 KiB screen budget is refreshed
every update (`:1779-1780`) and then thrown away unspent, so the transaction
advances at one unit per update instead of up to four, and the FIXUP phase
advances at one eighth of that again.

### Proposed fix

**Fix A (primary).** `src/import/battleship_mnplayersvs.c:1141-1153` — keep
issuing bounded units while this update's aggregate budget allows. Each unit is
still capped at `NDS_PLAYERS_VS_LOAD_STEP_BYTES`, so the audio-gap bound that
`NDS_PLAYERS_VS_LOAD_STEP_BYTES` exists to enforce is unchanged; only the number
of units per update changes, which is what the aggregate budget was written to
govern. The entry-only unbounded `drain` path is untouched.

Before (`:1141-1153`):

```c
        do
        {
            step_bytes = 0u;
            step = ndsRelocPreviewFighterLoadStep(block->load_cursor,
                                                  (drain != FALSE) ? 0u : budget,
                                                  &step_bytes);
            gNdsPlayersVSPreviewServiceStepCount++;
            unit_bytes += step_bytes;
            if (step_bytes > gNdsPlayersVSPreviewServiceByteMax)
            {
                gNdsPlayersVSPreviewServiceByteMax = step_bytes;
            }
        } while ((drain != FALSE) && (step == NDS_PREVIEW_PACK_STEP_IN_PROGRESS));
```

After:

```c
        do
        {
            step_bytes = 0u;
            step = ndsRelocPreviewFighterLoadStep(block->load_cursor,
                                                  (drain != FALSE) ? 0u : budget,
                                                  &step_bytes);
            gNdsPlayersVSPreviewServiceStepCount++;
            unit_bytes += step_bytes;
            if (step_bytes > gNdsPlayersVSPreviewServiceByteMax)
            {
                gNdsPlayersVSPreviewServiceByteMax = step_bytes;
            }
            if (drain == FALSE)
            {
                /* The aggregate span is a property of the update, not of a
                 * unit: spend it here in <=NDS_PLAYERS_VS_LOAD_STEP_BYTES
                 * units instead of leaving 75% of it unspent every tic. A
                 * unit that moved nothing (REGISTER) cannot extend the loop,
                 * because it can only be followed by DONE. */
                budget = (sNdsPlayersVSPreviewServiceByteBudget > unit_bytes) ?
                    (sNdsPlayersVSPreviewServiceByteBudget - unit_bytes) : 0u;
                if (budget > NDS_PLAYERS_VS_LOAD_STEP_BYTES)
                {
                    budget = NDS_PLAYERS_VS_LOAD_STEP_BYTES;
                }
                if (budget == 0u)
                {
                    break;
                }
            }
        } while (step == NDS_PREVIEW_PACK_STEP_IN_PROGRESS);
```

(The existing `sNdsPlayersVSPreviewServiceByteBudget` decrement at `:1155-1160`
stays as written — it subtracts the accumulated `unit_bytes` once, after the
loop, which is still correct.)

**Fix B (secondary, 1 update).** `src/import/battleship_mnplayersvs.c:1109-1110`
— the transaction open reads "under 200 bytes" (`:1076-1078`) and then burns a
whole update. Let it fall through into the tree phase it has just armed:

Before:

```c
        block->load_generation = gNdsTaskmanHeapGeneration;
        gNdsPlayersVSPreviewAcquireRetryCount++;
        return nNDSPlayersVSResidentAcquireRetry;
    }
```

After:

```c
        block->load_generation = gNdsTaskmanHeapGeneration;
        /* The open moved <200 bytes of header/section table and spent a
         * residency action; the update's aggregate span is untouched, so the
         * first payload units belong on this tic, not the next one. */
    }
```

This is safe because everything below re-reads `block->load_cursor` and
`block->load_generation` from the block it just initialised; the
generation compare at `:1113` is trivially satisfied on the same tic.

**Expected result:** every one of the twelve kinds lands at 10-14 updates
(≤0.23 s), against 21-35 today. The tour's `drewMask` should read `0xFFF` at an
unchanged 24-tic hold.

### The existing latency instrument cannot see this bug — corrected wiring

`ndsMNPlayersVSPreviewRecordLatency(pending)` is called **before**
`ndsMNPlayersVSPreviewRebuildChangedKind(...)` at both commit sites
(`src/import/battleship_mnplayersvs.c:2024-2026` and `:2192-2194`), i.e. it
stops the clock at *residency ready*, one update before the source fighter is
even created, and two to three tics before the 30 Hz pass can present it. It
also counts a cursor-returns-to-the-same-kind case as a resolved request
(`:2049-2053`). The owner's symptom is *first correct visible preview*, so the
warm/cold buckets at `:1522-1558` are not measuring it, and **any "N of 12
within 24 tics" figure taken from them is not a measurement of this row**. The
tour masks (`gNdsMenuShellCssWalkTourDrewMask`) do measure the right thing and
are what the `0xC13` reading above comes from.

Corrected instrumentation points, in order, all of which already have a natural
home in this file:

1. `request` — cursor kind differs from the slot (`:1960-1967`, already
   `pending->request_updates++`).
2. `stable hover` — dwell satisfied, commit taken (`:2136`,
   `gNdsPlayersVSPreviewDwellCommitCount`).
3. `transaction start` — cursor opened (`:1086`,
   `gNdsPlayersVSPreviewAcquireLoadCount`).
4. `each service unit` — `:1147` `gNdsPlayersVSPreviewServiceStepCount` plus
   `gNdsPlayersVSPreviewServiceUpdateByteMax` (`:1767-1774`), which is the
   direct falsifier for this row: **it should read 8,192 after the fix and reads
   ≤2,048 today.**
5. `closure ready` — `:1184` `block->load_tree_done`.
6. `each preparation stage` — `:1197` `gNdsPlayersVSPreviewPrepareStageCount`.
7. `rebuild complete` — move the `RecordLatency` call to **after**
   `ndsMNPlayersVSPreviewRebuildChangedKind` returns with
   `sMNPlayersVSSlots[slot].player != NULL`.
8. `first correct native draw` — the existing per-kind triangle delta
   `gNdsMenuShellCssWalkTourTriangles[kind]` / `gNdsFighterDLAllDrawP0HardwareTriangleCount`
   (`src/nds/nds_menu_shell_css.c:2357-2366`).
9. `presentation` — `gNdsPlayersVSPreviewDrawCount` (`:2402`), which is the 30 Hz
   term.

### Prefetch / warming while the CSS is idle

Asked and answered, with the structural limit named:

- There is **no prefetch today**. Acquisition is purely demand-driven from
  `ndsMNPlayersVSPreviewSync`; the only bulk warm is the entry drain, which
  covers the four initial slots once (`:1757-1762`).
- The resident cache is **four blocks**
  (`NDS_PLAYERS_VS_RESIDENT_BLOCKS = GMCOMMON_PLAYERS_MAX`, `:110`) of 80 KiB
  each (`include/nds/nds_preview_pack.h:18`). With four live slots every block
  can be referenced, so a 4-player CSS has **zero** spare blocks to prefetch
  into; a 2-player CSS has two. Speculative warming is therefore capped at 2 of
  12 kinds and only in some configurations — it cannot be the fix.
- The same four-block ceiling is why the commit deliberately destroys the old
  fighter before the new one loads (`:2116-2119`, `:2145-2166`): holding the old
  closure while loading the new one needs five blocks. That is also why the gap
  reads *blank* rather than *stale*. Making it stale-until-ready would cost one
  more 80 KiB block; not proposed here (RAM is not free on this target).
- The genuinely idle resource is the one Fix A reclaims: the 8 KiB aggregate
  span refreshed every update and discarded. Spending it is "warming while
  idle", done at the seam that owns it.

### Can any prepare stage be deferred?

- `NDS_PLAYERS_VS_PREPARE_ANIM_SELECTED` (`:144`) warms the *Selected* demo clip
  — the pose applied only once the puck lands (`:524-548`, and the comment at
  `:513-523` says so). It is **not** needed for the idle preview and is a clean
  deferral candidate, provided it is forced before `is_fighter_selected` is
  allowed to become TRUE.
- `NDS_PLAYERS_VS_PREPARE_OWNER_LOW` (`:146`) loads the low-detail owner image.
  The VS CSS draws **high** detail: `use_low_detail = (fp->detail_curr ==
  nFTPartsDetailLow)` (`src/port/renderer_adapter_fighter.c:3357`), and
  `src/import/battleship_ftmanager.c:624-635` states *"CSS uses HIGH"* while
  narrowing the ensure range to one detail **only** for `nSCKind1PGamePlayers`
  and `nSCKind1PIntro`. For `nSCKindPlayersVS` the range stays 0..1, so both
  details are loaded twice over (once by the prepare stage, once idempotently by
  `ndsFTManagerEnsureOwnerImages`; `ndsRendererNativeEnsureOwnerImage` early-outs
  on a resident slot, `src/nds/nds_renderer_assets.c:4779-4783`). For Link the
  pair is 35,612 B and for Kirby the high image alone is 30,160 B
  (`docs/p2/BUG_NOTES.md:4530-4532`, `:414-415`).
  **Deferring OWNER_LOW is only a win if `ndsFTManagerEnsureOwnerImages` is
  narrowed to the CSS's actual detail at the same time** — otherwise the read
  simply moves onto the rebuild tic, which is worse. Do this as a second,
  separately measured step, keep `ndsMNPlayersVSPreviewOwnerImagesFit`
  (`:654-696`) checking **both** details so the capacity proof is unchanged, and
  watch `gNdsNativeOwnerImageMismatchCount` / the wrong-detail resolver decline
  (`src/nds/nds_renderer_assets.c:5738`) as the falsifier.
- `NDS_PLAYERS_VS_PREPARE_COMMIT`, `ANIM_IDLE`, `OWNER_HIGH` are all required
  before the first idle frame can draw.

```
Confidence: HIGH for the root cause and the arithmetic — the model reproduces the
recorded drewMask partition (5 kinds ≤24, 7 kinds ≥28) with no free parameter,
and the defect is a one-token continuation condition against a comment in the
same file that states the intended behaviour.
Falsified by: gNdsPlayersVSPreviewServiceUpdateByteMax reading 8,192 (or anything
above 2,048) on a shipping CSS capture today. If it already reads 8,192 the
loop is being driven more than once per update by something I did not find, and
this whole row is wrong.
Open question: none required to land Fix A/B. To price the deferral step, the
runtime read still needed is fp->detail_curr for a VS CSS preview fighter
(probe-fighter-anim-state.ps1 already prints the FTStruct) — it must be
nFTPartsDetailHigh for all four slots before OWNER_LOW may be dropped.
```

---

## Row 2 — Link intermittently untextured

```
Bug: Link: "sometimes missing textures/color (turns gray)."
```

### Contract

Link's CSS preview draws through the native fighter owner. A run whose material
requests texturing must bind the source image the root program names; a CI4
image must bind with the TLUT that the root replayed for it. Provenance is
pinned offline: `scripts/fighters/test_native_image_provenance.py:66-88` asserts
that **Link root `0x2C88` replays each CI palette (source offsets `0xDD58`,
`0xDE88`, 16 entries) immediately before its texel image**, in both details.

### What "gray" can and cannot be in this renderer

This matters, because it eliminates one whole family:

- A failed texture bind does **not** draw untextured. `use_texture == FALSE`
  from `ndsRendererHardwareBindTexture` returns
  `ndsRendererNativeDirectReject(stats)`
  (`src/nds/nds_renderer_native_common.c:8551-8555`), which sets
  `stats->blocker = NDS_RENDERER_BLOCKER_UNSUPPORTED`
  (`:3332-3342`). The run is **not submitted**. The blocker latch is then
  re-read at the top of the epoch loop (`:11152-11155`) and at the production
  entry (`src/nds/nds_renderer_native_fighter_production.c:107-110`), so one
  rejected run truncates the rest of the owner.
- Therefore the owner's "turns gray" is **surfaces disappearing, leaving only
  the lit/untextured and already-drawn runs**, or a replay drawing through VRAM
  that no longer holds the recorded texels — not a "fall back to untextured"
  path. There is no untextured fallback in this renderer, by policy and by code.

So the three candidates separate as:

- **(a) intentionally untextured source surface** — ruled out as the *cause*:
  Link's untextured runs are the same every frame; an intermittent symptom
  cannot come from a static source property.
- **(b) white texture × grey lighting** — not supported by the owner's wording
  ("missing textures/color"), and the lighting path is shared with the eleven
  fighters that do not show it. Not pursued; see the open question for how to
  kill it in one read.
- **(c) missing / evicted texture** — the only candidate that is both
  intermittent and order-dependent. Everything below is about (c).

### Recorded premises checked — one is not transferable, two are refuted

1. **"Link LOW root 0x2C88: the CI4 reject is a stale TMEM load record"**
   (`docs/p2/BUG_NOTES.md:4394-4415`). The mechanism is real and the seam is
   real: `ndsRendererHardwareResolveOrBindTexture` prefers the per-TMEM load
   record over the live global image state
   (`src/nds/nds_renderer_textures_effects.c:10826-10866`), the record ring is
   only **two entries deep** (`include/nds/nds_renderer.h:393`,
   `NDS_RENDERER_TEXTURE_LOAD_HISTORY_COUNT 2`), and
   `ndsRendererHardwareFindTextureLoadForTmem` takes the highest-sequence record
   for the tile's TMEM address with no check that its image still belongs to the
   current owner (`:743-766`).
   **But that note is battle-core evidence, not CSS evidence** — it was taken on
   `build-p2-battle-core` at frame 152 and its reject site is reached through
   the legacy-DL data-pointer callback (`renderer_adapter_legacy_dl_probes.c:93-139`).
   It does not transfer to the CSS preview unmodified, and I could not close the
   step it itself leaves open ("still unobserved: which write puts the palette-block
   address into the TMEM-0 record"). Do not treat it as established for this row.
   One cross-check that *narrows* it: the record ring lives in `NDSRendererStats`,
   and the fighter draw uses a **stack-local** `persistent_stats`
   (`src/port/renderer_adapter_fighter.c:3249`) cleared by
   `ndsRendererInitStats` → `memset` (`src/nds/nds_renderer_dispatch_profile.c:73`)
   at `:3553`. So the ring **cannot** carry a record from a previously hovered
   fighter; any stale record must be created inside Link's own owner execution.
   `ndsFighterDLDrawResetRuntimeRendererStats`
   (`src/port/renderer_adapter_legacy_dl_probes.c:376-402`) deliberately does
   *not* clear it, but it runs after the full init on the same struct.

2. **REFUTED: the per-run texture memo carrying a foreign fighter's texture.**
   `ndsRendererR2RunTextureMemoApply`
   (`src/nds/nds_renderer_native_common.c:7818-7885`) is keyed on
   `texture_memo_owner_key`, which packs owner slot, detail, player slot and
   costume/shade (`src/port/renderer_adapter_fighter.c:4060-4065`), is indexed
   per player slot (`:7812-7815`), and revalidates the cache entry by
   `ready`, `name` and `key_generation` before trusting itself (`:7850-7857`).
   It cannot hand Link another fighter's texture.

3. **REFUTED: a packet replay outliving its textures.**
   `ndsFighterPacketTexturesResident` checks slot/name/`key_generation` for every
   recorded texture (`:9480-9497`), `ndsFighterPacketTouchTextures` keeps them
   off the LRU (`:9513-9526`), and a packet that could **not** record every
   texture individually — more than `NDS_FIGHTER_PACKET_TEXTURE_MAX = 24`
   (`src/nds/nds_renderer_preamble.c:3403`), or a bind with no active cache
   entry — sets `needs_fence` (`:9451-9472`), which folds
   `sNdsRendererHardwareTextureKeyGeneration ^ (evictCount << 16)` into the match
   key (`:9427-9435`, `:9892`, `:10206`). Such a packet self-invalidates on any
   texture churn. The replay path is fail-safe.

### The remaining mechanism, and why it is intermittent

The CSS is the only screen that draws up to **four** different fighters'
textures in one frame with **no pinned corpus**. The eviction and allocation
paths both refuse any entry used this frame:

```c
        if ((entry != exclude) && (entry->name != 0) &&
            (entry->pinned == 0u) && (entry->stage_warm == 0u) &&
            (entry->last_used_frame !=
             (sNdsRendererHardwareFrameSerial + 1u)))
```
`src/nds/nds_renderer_textures_effects.c:2818-2824`, and the same predicate in
`ndsRendererHardwareAllocTexture` at `:2872-2880`. When every dynamic slot is
either pinned or touched this frame, `AllocTexture` returns NULL, the upload
retry has nothing to evict, and the resolve rejects with
`NDS_RENDERER_HW_TEXREJECT_ALLOC` (`src/nds/nds_renderer_preamble.c:1895`) — and
by the section above, the run and everything after it in that owner vanish.

Why Link: he is the largest preview pack (32,032 B against a 9,948-21,636 B
field) and root `0x2C88` alone needs two CI4 palettes plus their texel images.
Why intermittent: whether the pool has a takeable victim depends on how many
other slots are populated and which fighters they hold — i.e. on the hover
history, exactly as the owner describes. Nothing releases a retired preview's
texture entries: `ndsMNPlayersVSPreviewRetireResidentBlock`
(`src/import/battleship_mnplayersvs.c:999-1030`) releases owner images, the
reloc heap range and the arena, and
`ndsFighterRendererInvalidateMaterialCachesForSlot`
(`src/port/renderer_adapter_matrix.c:495-503`) clears material rows and the
packet — **no path clears the hardware texture cache entries keyed on the
retired block's bytes**. They survive as ordinary LRU entries and are reclaimable,
so this is pressure rather than a leak, but it is pressure that tracks browsing
order.

I could not confirm the coordinator's `ndsRendererNativeBindGraded` lead for
*this* row: the graded-quad table and `ndsRendererHardwarePrepareIFCommonAtlas`
are effect-side consumers (thunder jolt, Poké Ball, PK effects — the twelve
owners listed in `docs/p2/REMAINING_BUGS_IMPLEMENTATION_PLAN_2026-09-21.md:404-420`),
and the CSS runs no effects. On the CSS the evictor pressure is fighter-on-fighter.
That lead stays live for the battle-side reports; the evidence here does not
point at it.

### Proposed fix

I am **not** proposing a code change for this row yet, because the discriminator
already exists in the tree and has not been run against the CSS. Proposing an
eviction-policy change without it would be exactly the "unengaged counter proves
nothing" failure this repo keeps recording. The measurement is one lab ROM and
one stop:

- Build a CSS lab ROM with `NDS_R2_STAGE_ROUTE_PROBE=1` (`Makefile:157`,
  default 0). That flag alone links the reject witness at
  `src/nds/nds_renderer_textures_effects.c:9278-9323` without dragging in
  profile level 2.
- Hover Link **after** three other fighters are resident, and read:
  - `gNdsRendererProfileTextureRejectReasonMask` — names the reason bit. `ALLOC`
    (1<<10) confirms the mechanism above; `BAD_SOURCE_PTR` (1<<7) or
    `BAD_SOURCE_BYTES` (1<<6) instead confirms the TMEM/provenance family and
    makes the 0x2C88 note transferable after all.
  - `gNdsR2TexRejectCensusFree / Live / Pinned / ThisFrame / Evictable`
    (`:9291-9322`) — these were written precisely to separate "VRAM genuinely
    full (bytes)" from "every slot pinned or touched-this-frame so nothing MAY
    be evicted (slots)". `Evictable == 0` with `ThisFrame` large is the
    four-fighter starvation signature.
  - `gNdsFtrDeclineStage` (`src/port/renderer_adapter_fighter.c:2374`) and
    `stats->hardware_texture_reject_count` — how far the owner got.
- `scripts/capture-sudden-death-entry.ps1:477`, `:774`, `:778` already print all
  of these; the recipe is a copy, not new work.

If the census reads `Evictable == 0 / ThisFrame` high, the owning seam is the
allocator's victim policy at `src/nds/nds_renderer_textures_effects.c:2860-2886`
and the minimal repair is to give the CSS preview retire path a texture-release
hook — release the entries whose `image` lies inside the retired block's byte
range, in `ndsMNPlayersVSPreviewRetireResidentBlock`
(`src/import/battleship_mnplayersvs.c:1012-1016`), beside the existing
`ndsRendererNativeReleaseOwnerImagesInRange` call and with the same range
argument. That is the owning seam (the CSS owns the lifetime), it is not a
renderer-policy change, and it cannot affect any screen that does not retire
preview blocks.

```
Confidence: MEDIUM for the mechanism class (c) and the seam; LOW that it is
specifically ALLOC starvation rather than the TMEM/provenance family, because
the reject witness is compiled out of every shipping ROM and has never been read
on the CSS.
Falsified by: gNdsRendererProfileTextureRejectReasonMask reading only
BAD_SOURCE_PTR / BAD_SOURCE_BYTES with a healthy census (Evictable > 0) — that
would move this row onto the 0x2C88 TMEM-record seam and make
ndsRendererHardwareFindTextureLoadForTmem's missing ownership check
(src/nds/nds_renderer_textures_effects.c:743-766) the repair site instead.
Also falsified if the reject count stays zero on a gray frame — then the runs
ARE being submitted and the symptom is (b), a lighting/material problem, which
gNdsRendererProfileTextureRejectReasonMask == 0 plus a non-zero fighter triangle
delta would establish in the same stop.
Open question: the one runtime observation still needed is that stop.
Specifically: the reject reason mask, the five census counters, and
gNdsMenuShellCssWalkTourTriangles[5] (Link) on a frame the owner calls gray —
the last one separates "runs rejected, triangles missing" from "runs drawn,
colour wrong" and therefore settles (b) vs (c) by itself.
```

---

## Row 3 — Yoshi and Jigglypuff, one eye closed

```
Bug: "Yoshi: sometimes one eye is closed" / "Jigglypuff ... sometimes one eye is closed."
```

### Contract

The shared cause is a real, named source mechanism, and it is **not** a blink
timer and **not** an MObj material-animation loop.

- A fighter has exactly **two texture parts**:
  `struct FTTexturePartContainer { FTTexturePart textureparts[2]; }`
  (`decomp/BattleShip-main/decomp/src/ft/fttypes.h:172-175`) and
  `FTTexturePartStatus texturepart_status[2]`
  (`decomp/.../fttypes.h:1235`). Each part is
  `{ u8 joint_id; u8 detail[2]; }` (`fttypes.h:166-170`) — a joint plus the
  **index of the MObj within that joint's chain**, chosen per detail level.
- Setting one writes that single MObj's texture selector:
  `ftParamSetTexturePartID` walks `detail` links down the chain and assigns
  `mobj->texture_id_curr = texture_id`
  (`decomp/.../ft/ftparam.c:1113-1144`).
- `mobj->texture_id_curr` selects the image:
  `sprites[texture_id_curr]` is emitted as the current SETTIMG when the MObj's
  flags carry `MOBJ_FLAG_FRAC | MOBJ_FLAG_ALPHA`
  (`decomp/.../sys/objdisplay.c:1421-1429`), the DS port mirroring it exactly at
  `src/port/renderer_adapter_stage.c:2264-2272`
  (`NDS_RENDERER_NATIVE_MATERIAL_CURRENT_IMAGE`), applied per epoch at
  `src/nds/nds_renderer_native_common.c:968-975` and `:11141-11145`.
- The **driver is the motion script**, not a timer:
  `case nFTMotionEventSetTexturePartID: ftParamSetTexturePartID(gobj,
  ...->texturepart_id, ...->frame);`
  (`decomp/.../ft/ftmain.c:597-605`). `ftMainSetStatus` resets both parts on any
  status change without `FTSTATUS_PRESERVE_TEXTUREPART`
  (`decomp/.../ft/ftparam.c:1147-1189`), and a costume or detail rebuild
  re-applies the live ids onto the fresh MObjs via `ftParamInitTexturePartAll`
  (`decomp/.../ft/ftparam.c:1069-1110`, called from
  `ftParamSetModelPartDetailAll` `:970` and `ftParamInitAllParts` `:1065`).

Counted off the shipped scripts, the two fighters the owner names are among the
heaviest users of this event — `ftMotionCommandSetTexturePartID` occurrences per
MainMotion file: **Kirby 102, Yoshi 66, Purin 56, Pikachu 50, Luigi 43, Ness 29,
Link 20, Mario 17, Fox 11, Captain 9; Donkey, Samus, MMario, Boss 0**
(`decomp/BattleShip-main/decomp/src/relocData/*MainMotion.c`). Donkey and Samus
use it zero times and are the two fighters the owner has never reported a face
problem on. That is the shared cause the row asks for: **two independent
per-MObj selectors, driven asymmetrically by script events.** "Sometimes one eye
is closed" is what you see when one selector follows the script and the other is
stuck: the stuck eye reads wrong only during the frames the driven eye is
closed.

### Refuted along the way (each cost a cycle, so they are recorded)

- **The port does dispatch the event.** `nFTMotionEventSetTexturePartID` appears
  nowhere in `src/`, which looks like a missing case — but
  `src/import/battleship_ftmain.c:148` `#include`s the whole decomp
  `src/ft/ftmain.c`, so the dispatcher is compiled. Not a gap.
- **The bitfield order is handled.** `FTMotionEventSetTexturePartID`
  (`include/ft/fighter.h:2873-2883`) reverses field order under
  `NDS_FTMOTION_BIGFIELD_ORDER` to match IDO's MSB-first packing against the
  decomp's `fttypes.h:466-471`. Correct.
- **`ftParamInitTexturePartAll` has both source call sites.**
  `src/port/reloc_backend_compat_shims.c:1920` (end of `ftParamInitAllParts`)
  and `:6715` (end of `ftParamSetModelPartDetailAll`), matching decomp `:1065`
  and `:970`.
- **The joint walk is not truncated.** `ndsFTStructJointLoopLimit`
  (`src/port/reloc_backend_compat_shims.c:3201-3217`) would shorten
  `ftParamUpdateAnimKeys`' walk to `4 + fp->nds_common_joint_count`, which would
  silently stop animating tail joints. It does not fire, because
  **`fp->nds_common_joint_count` has no writer anywhere in `src/`** — it is
  declared at `include/ft/fighter.h:3889`, read at `:3206` and `:3211`, printed
  by `scripts/probe-fighter-anim-state.ps1:99`, and never assigned, so the
  function always takes its `return nFTPartsJointNumMax` early exit. **This is a
  latent trap worth a separate row:** the day anyone populates that field, every
  joint above the common count stops being parsed, played *and* material-animated.
- **Both branches of the port's `ftParamUpdateAnimKeys` do run the MObj walk**
  (`src/port/reloc_backend_compat_shims.c:3309-3315` for pose-owned joints and
  `:3394-3399` for the generic path), matching decomp `ftparam.c:394-402` /
  `:419-427`.
- **The material anim cache key covers the eye selector.**
  `ndsRendererAdapterMaterialAnimHash`
  (`src/port/renderer_adapter_stage.c:4247-4264`) hashes
  `texture_id_curr | texture_id_next`, `lfrac` and `palette_id` alongside the
  five colour words, and the row is identity-checked per MObj
  (`:4387-4396`). A texture-id change cannot be cached over.

### Divergence

`ftParamSetTexturePartID`, port vs source, in the write-back of the status
mirror. Source (`decomp/BattleShip-main/decomp/src/ft/ftparam.c:1120-1143`):

```c
    if (joint != NULL)
    {
        MObj *mobj = joint->mobj;
        s32 i = 0;
    loop:
        if (mobj != NULL)
        {
            if (i != detail) { mobj = mobj->next; i++; goto loop; }
            mobj->texture_id_curr = texture_id;
            fp->texturepart_status[texturepart_id].texture_id_curr = texture_id;
            fp->is_texturepart_modify = TRUE;
        }
    }
```

The mirror and the modify flag are inside the `mobj != NULL` arm: **if the
joint's MObj chain is shorter than `detail`, the source records nothing.** Port
(`src/port/reloc_backend_compat_shims.c:2641-2651`):

```c
    mobj = (joint != NULL) ? joint->mobj : NULL;
    for (i = 0; (mobj != NULL) && (i < detail); i++)
    {
        mobj = mobj->next;
    }
    if (mobj != NULL)
    {
        mobj->texture_id_curr = texture_id;
    }
    fp->texturepart_status[texturepart_id].texture_id_curr = texture_id;
    fp->is_texturepart_modify = TRUE;
```

The mirror and the flag are written **unconditionally**. The consequence is not
cosmetic, because two later functions treat the mirror as the authority and
write it back into whatever MObj now sits at that index:

- `ftParamResetTexturePartAll` (`:2662-2706`) skips a part only when
  `texture_id_curr == texture_id_base`, so a phantom record makes it write.
- `ftParamInitTexturePartAll` (`:1747-1792`) has the same predicate and runs
  after every costume change (`:1920`) and every detail switch (`:6715`) — the
  two events that *rebuild the MObj chains*, i.e. exactly the moments at which
  the index that was out of range becomes in range.

So a `SetTexturePartID` the source deliberately dropped is resurrected later and
applied to one — and only one — of the two texture parts. Asymmetric by
construction, and intermittent because it only bites after the next status
change or parts rebuild. `detail` differs per detail level by design
(`FTTexturePart.detail[2]`), which is precisely how one part can be in range at
one detail and out of range at the other.

### Proposed fix

`src/port/reloc_backend_compat_shims.c:2646-2651`. Restore the source's
conditional record. This can only *remove* a write the source never makes, so it
cannot regress a fighter that is correct today.

Before:

```c
    if (mobj != NULL)
    {
        mobj->texture_id_curr = texture_id;
    }
    fp->texturepart_status[texturepart_id].texture_id_curr = texture_id;
    fp->is_texturepart_modify = TRUE;
}
```

After:

```c
    if (mobj != NULL)
    {
        /* BattleShip ftparam.c:1127-1141 keeps the status mirror and the
         * modify flag INSIDE this arm: a chain shorter than `detail` records
         * nothing. Writing the mirror anyway made ftParamResetTexturePartAll
         * and ftParamInitTexturePartAll replay a selection the source dropped
         * onto whichever MObj later occupies that index -- and because the two
         * texture parts carry different per-detail indices, it can reach one
         * of a fighter's two eye materials and not the other. */
        mobj->texture_id_curr = texture_id;
        fp->texturepart_status[texturepart_id].texture_id_curr = texture_id;
        fp->is_texturepart_modify = TRUE;
    }
}
```

The two guard clauses the port adds above this point
(`:2616-2626` for a null container, `:2631-2635` for an out-of-range joint id or
an uninitialised detail) are defensive additions over a source that would fault;
leave them, but note that the `container == NULL` arm at `:2622-2625` writes the
same phantom record and should be made to `return` without writing it, for the
same reason.

```
Confidence: MEDIUM. The contract is established end to end from the source with
citations, the two named fighters are the 2nd and 3rd heaviest users of the
mechanism, and the two fighters with zero uses (Donkey, Samus) are exactly the
two the owner has never reported a face defect on -- that correlation is strong.
The divergence is certain (the code differs from the source as quoted); what is
not certain is that it is THE cause of this particular symptom rather than one
of several.
Falsified by: fp->texturepart_status[0..1].texture_id_curr both equalling
texture_id_base on a Yoshi or Purin preview that shows the defect -- that would
mean no texture-part write is live at all and the asymmetry is in the model data
or in the two MObjs' flags instead.
Open question: one runtime read, on a CSS preview of Yoshi and of Purin while the
defect is visible. For each fighter dump (1) both texturepart_status entries,
(2) attr->textureparts_container->textureparts[0..1] (joint_id and detail[] for
the live fp->detail_curr), and (3) the two selected MObjs' texture_id_curr and
sub.flags. If the two MObjs' texture_id_curr differ, this row is the texture-part
path and the patch above applies; if they agree, the asymmetry is downstream and
the next seam is whether both eye MObjs carry MOBJ_FLAG_FRAC|MOBJ_FLAG_ALPHA
(only then is CURRENT_IMAGE emitted at all -- src/port/renderer_adapter_stage.c:2264).
scripts/probe-fighter-anim-state.ps1 already walks FTStruct and is the shortest
place to add the three reads.
```

---

## Cross-row notes

- **Recorded claims corrected here:** the "13-tic dwell" comment
  (`src/nds/nds_menu_shell_css.c:2343`), the "same root cause as the CSS blocking
  load" status (`docs/p2/BUGS_REMAINING_DIAGNOSIS_2026-09-19.md:209`), and the
  transferability of the `0x2C88` stale-TMEM note
  (`docs/p2/BUG_NOTES.md:4394`) from battle-core to the CSS.
- **Latent defect found in passing, not on any row:** `fp->nds_common_joint_count`
  (`include/ft/fighter.h:3889`) is read by `ndsFTStructJointLoopLimit`
  (`src/port/reloc_backend_compat_shims.c:3201-3217`) and written by nothing.
  It is dead today only because it is always zero. If it is ever populated,
  `ftParamUpdateAnimKeys` silently stops animating every joint at or above the
  limit — including the character-specific joints the function's own comment
  (`:3275-3281`) says must keep being played, naming Samus's grapple joint 36.
- **Row 1 and Row 2 share an ordering dependency.** Fix A shortens the window in
  which several fighters' textures are simultaneously live in the CSS, so it may
  move Row 2's symptom without fixing it. Capture Row 2's reject witness
  **before** landing Fix A, or the comparison is lost.
