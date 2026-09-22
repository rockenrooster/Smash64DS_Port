# VFX lane: r36 diagnosis (read-only agent)

Scope: rows 1 and 2 below. Row 3 (roster-dependent VFX regression) was reassigned
to the integrator mid-investigation and is not diagnosed here; a factual note
about the worktree state of its seam is at the end.

**Provenance of every claim below.** `git diff --stat 260b754be00 HEAD` is EMPTY
for every source and generator file cited in rows 1 and 2, so the code I read is
byte-identical to the ROM the owner played, and still is after the integrator's
row-3 commit `2dc42e97624` landed mid-write. Nothing outside rows 1 and 2's own
seams is cited as evidence here. No build or emulator was run. Two
claims are measurements taken from files already on disk (`builds/build/battle-core/06.fpc`
and `09.fpc`, and `src/nds/generated/nds_particle_banks.generated.inc`), decoded
with the producers' own documented formats.

---

## Row 1 — Yoshi

```
Bug: Yoshi's guard/shield (egg) is invisible
```

### Contract

Yoshi is the one fighter whose guard is not the shared bubble. Both guard entry
paths branch on `fkind` and send him somewhere else:

- `decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommonguard1.c:387-397` —
  `if (fp->fkind == nFTKindYoshi) { fp->status_vars.common.guard.effect_gobj =
  efManagerYoshiShieldMakeEffect(fighter_gobj); ftParamHideModelPartAll(fighter_gobj);
  ftCommonGuardSetHitStatusYoshi(...); fp->is_shield = TRUE; ftCommonGuardUpdateJoints(...); }`
- `decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommonguard2.c:18-28` — the
  same branch out of `ftCommonGuardSetStatusFromEscape`, with
  `else fp->...effect_gobj = efManagerShieldMakeEffect(...)` for everybody else.

So the source **deliberately hides Yoshi's entire drawable tree** and expects the
egg to draw in its place. There is no state in which "generic shield repaired"
can cover him: the call sites are an if/else on `fkind`.

The egg itself:

- `efManagerYoshiShieldMakeEffect` — `decomp/.../src/ef/efmanager.c:4172-4199`:
  `efManagerMakeEffectForce(&dEFManagerYoshiShieldEffectDesc)`, then
  `fp->is_effect_attach = TRUE`, `DObjGetStruct(effect_gobj)->user_data.p =
  fp->joints[nFTPartsJointYRotN]`, `scale.x = scale.y = 1.5F`,
  `effect_vars.shield.player = fp->player`, `is_damage_shield = FALSE`.
- `dEFManagerYoshiShieldEffectDesc` — `decomp/.../src/ef/efmanager.c:490-517`:
  flags `EFFECT_FLAG_USERDATA` **only** (`= 0x2`, `decomp/.../src/ef/efdef.h:7`),
  DL link 15, texture file `&gFTDataYoshiModel`, transforms `{0x50, 0x2C, 0}` /
  `{Null, Null, 0}`, `proc_update = efManagerShieldProcUpdate`,
  `proc_display = efManagerYoshiShieldProcDisplay`,
  `o_dobjsetup = &llYoshiModelShieldDObjDesc`, and `o_mobjsub`/`o_anim_joint`/
  `o_matanim_joint` all `0x0`.
- `llYoshiModelShieldDObjDesc` **is the constant `0xA860`** —
  `decomp/BattleShip-main/include/reloc_data.us.h:4168` (and `.jp.h:4119`). This
  is a data fact, not a file comment: it is the same YoshiModel source root the
  port's generated owner already names,
  `include/nds/generated/nds_native_yoshi_egg.generated.h:6-7`
  (`NDS_NATIVE_YOSHI_EGG_ASSET 338u`, `NDS_NATIVE_YOSHI_EGG_ROOT 0xa860u`). The
  source shares one quad between the guard egg and EggThrow's projectile; that
  sharing is in the source data, and it does not make this row the projectile row.
- Because the flags carry neither `0x1` nor `0x4`, `efManagerMakeEffect` takes
  its last branch — `decomp/.../src/ef/efmanager.c:2044` —
  `lbCommonInitDObj3Transforms(gcAddDObjForGObj(effect_gobj, (void*)(addr + effect_desc->o_dobjsetup)), ...)`,
  and `gcAddDObjForGObj` stores that pointer verbatim as the DObj's display list:
  `new_dobj->dv = dvar` (`decomp/.../src/sys/objman.c:1389,1420`). **For this desc,
  `o_dobjsetup` is a Gfx display-list address, not a DObjDesc.** That is what
  makes it unlike every other fighter-file effect desc in the port.
- Display: `efManagerYoshiShieldProcDisplay` —
  `decomp/.../src/ef/efmanager.c:4148-4168`: PipeSync, `gDPSetEnvColor` derived
  from `fp->shield_health`, then `gcDrawDObjDLHead1(effect_gobj)` **and**
  `efDisplayCLDProcDisplay(effect_gobj)`. Two source draws: the egg through DL
  head 1, and the collision/CLD overlay.

### Divergence

**First wrong value: `efManagerYoshiShieldMakeEffect` returns `NULL` before the
source maker is ever entered.** The port's wrapper is
`src/import/battleship_efmanager.c:1860-1872`; it returns NULL when
`ndsEFManagerBeginMappedDesc(&dEFManagerYoshiShieldEffectDesc, &saved) == FALSE`.
That call fails, permanently, in the owner's configuration.

Owning seam: `ndsEFManagerMapFileOffset`, `src/import/battleship_efmanager.c:1271-1277`.

```c
1271:        const void *mapped = ndsRelocNativeAssetAddress(
1272:            base, (u32)(uintptr_t)source_offset);
1273:
1274:        if (mapped == NULL)
1275:        {
1276:            return FALSE;
1277:        }
```

Chain, each link cited:

1. The owner ROM is the all-content target, which sets `NDS_P2_MENU_SHELL := 1`,
   `NDS_P2_1P_GAME := 1` and `NDS_P2_COMPACT_BATTLE_FIGHTERS := 1`
   (`Makefile:3292-3302`), and `NDS_P2_SHELL_ROSTER ?= 10` gives
   `NDS_P2_YOSHI = 1` (`Makefile:752, 788`). So the `#if` at
   `src/import/battleship_efmanager.c:1269` is **active** and the
   `#else relative = source_offset` identity path at `:1281` is **not** compiled.
2. In a VS match `gSCManagerSceneData.scene_curr == nSCKindPlayersVS`, so
   `ndsRelocNativeAssetAddress` (`src/port/reloc_preview_pack.c:198-216`) skips
   its early identity return and goes to
   `ndsPreviewFileOffset(loaded, offset, 1u, &mapped)`.
3. `ndsPreviewFileOffset` (`src/port/reloc_preview_pack.c:107-131`) searches
   **retained data spans only**. Geometry is not in a span: the battle-core
   producer forbids it — "A retained structural span must never swallow a
   geometry row. Geometry is replaced by native root identity cells, not copied
   accidentally" (`scripts/fighters/generate_battle_core_packs.py:483-493`), and
   each retained geometry root becomes one 8-byte
   `struct.pack(">2I", fpc.ENDDL, source)` cell appended after the compact model
   (`:552-554`).
4. **Measured, from the pack on disk.** `builds/build/battle-core/06.fpc` is
   Yoshi's battle pack (FPC header `fkind 6`, `main 247`, `model 338`,
   `file_bytes 25292`, matching `builds/build/battle-core/battle_core_manifest.json`).
   Decoding it with the producer's own `HEADER_FMT`/`SECTION_FMT`/`SPAN_FMT`
   (`scripts/fighters/generate_preview_core_packs.py:50-53, 423-462`):
   the Model section has **49 spans** and **55 root cells at `roots_offset 0x477c`**;
   `0xA860` is **in none of the 49 spans** and **is** root cell #54. The nearest
   retained spans start at `0xA958`.
   So `ndsPreviewFileOffset(loaded, 0xA860, ...)` returns FALSE,
   `ndsRelocNativeAssetAddress` returns NULL, `ndsEFManagerMapFileOffset` returns
   FALSE at line 1276, `ndsEFManagerMapDescOffsets`
   (`src/import/battleship_efmanager.c:1291-1316`) returns FALSE, and
   `ndsEFManagerBeginMappedDesc` (`:1723-1746`) returns FALSE at `:1734`.
5. The retry mechanism cannot recover it. `ndsEFManagerResolveDescOffsets`
   (`:1613-…`) disables the desc (`desc->proc_display = NULL`,
   `gNdsEFDescDisabledCount++` at `:1690`/`:1704`) and defers it;
   `ndsEFManagerRetryDeferredDescs` (`:1584-1611`) gates recovery on the *same*
   `ndsEFManagerMapDescOffsets`, so `dEFManagerYoshiShieldEffectDesc` and
   `dEFManagerYoshiEggEscapeEffectDesc` (both listed at
   `src/import/battleship_efmanager.c:1459,1462`) can never come back — every
   generation, every match. The mechanism is not broken and does not need
   replacing; it is being handed an offset the data mapper structurally cannot
   represent.
6. Net effect on screen: `ftcommonguard1.c:391` still runs
   `ftParamHideModelPartAll`, `effect_gobj` is NULL, nothing is created, nothing
   is drawn. **Yoshi vanishes entirely while guarding** — which is exactly the
   owner's report, and exactly the historic pair of rows ("B attack turns yoshi
   invisible and egg is also invisible", "character intro is invisible (egg
   hatching)"), because `dEFManagerYoshiEggEscapeEffectDesc`
   (`decomp/.../src/ef/efmanager.c:1375-1402`) names the same `0xA860` and fails
   identically.

**Contrast that proves this is specific and not a general desc failure.** The
shared bubble shield, `dEFManagerShieldEffectDesc`
(`decomp/.../src/ef/efmanager.c:460-487`), carries flags `0x4 | EFFECT_FLAG_USERDATA`,
so its `o_dobjsetup` is a *DObjDesc* (structural, span-retained), and its file is
`gFTManagerCommonFile`, which is not a compact fighter pack at all. Pikachu's
thunder trail is the closest same-shape case — `EFFECT_FLAG_USERDATA` only,
owning a compact-packed Model — and it works: decoding
`builds/build/battle-core/09.fpc` (Pikachu, `fkind 9`, `model 341`) shows
`o_dobjsetup 0x95B0` and `o_mobjsub 0x9420` **inside retained spans**, and only
the geometry root `0x94F8` as a root cell. Yoshi's shield is the one desc in the
roster whose `o_dobjsetup` names pruned geometry.

### Root cause

`ndsEFManagerMapFileOffset` maps EFDesc source offsets through
`ndsRelocNativeAssetAddress`, which resolves **data** spans only. A compact
battle pack does not keep geometry as data; it keeps one identity root cell per
retained Gfx root. An EFDesc whose `o_dobjsetup` is a display list therefore maps
to NULL and the whole descriptor is disabled. The port already solved exactly
this, for exactly this source root, on the weapon side:
`src/import/battleship_wpmanager_core.c:364-389` resolves EggThrow's
WPAttributes data pointer with `ndsRelocNativeRootAddress(gFTDataYoshiModel,
NDS_NATIVE_YOSHI_EGG_ROOT)` and says why in its comment. The EFDesc side was
never given the same resolver.

### Why the previous "FIXED" did not hold

Three separate things, and none of them is "the maker is missing":

1. **The reinstated change is present in r36 and is not the blocker.**
   `05fc0add5de` ("Yoshi's egg: reinstate the owner", 2026-09-21 23:14) is an
   ancestor of `260b754be00` (r36, 2026-09-22 06:35). Its two C hunks are in HEAD:
   `src/port/renderer_adapter_stage.c:5280-5288` and
   `src/nds/nds_renderer_native_common.c:4932-4948`. They are *renderer admission*
   arms. The effect never gets that far, because no effect GObj and no DObj are
   ever created.
2. **The admission arm it added cannot fire in the shipped ROM anyway.** It tests
   `((uintptr_t)dl == (uintptr_t)gFTDataYoshiModel + 0xa860u)`
   (`src/port/renderer_adapter_stage.c:5282`) — a *raw source* address. In the
   compact pack there is no such address: the Gfx at `0xA860` is pruned and the
   live address is the root cell at Model-relative `0x492C`
   (`roots_offset 0x477C + 54*8`, from the `.fpc` decode above). Every other
   Yoshi/Pikachu owner in the same file uses the cell-aware
   `ndsRelocNativeRootOffset()` instead — including an owner for **this very
   root**, `src/port/renderer_adapter_stage.c:7010-7027`, whose comment states
   the rule outright: "The production compact battle pack intentionally PRUNES
   this root's palette/image/vertex spans and replaces its Gfx program with one
   ENDDL+source-offset identity cell. `ndsRelocNativeRootOffset()` is the
   compact-pack-aware proof of source identity."
3. **The premise "the egg had no native owner" was wrong.** That pre-existing
   owner at `:7010-7027` already admits `dobj->parent_gobj->id ==
   nGCCommonKindEffect` with `dobj->mobj == NULL` (which the shield desc
   satisfies: `o_mobjsub = 0x0`). So a second owner was baked into the
   entry-effect bank for a root that already had one. The 2026-09-17 revert
   (`252a9aa4290`) and the cost analysis around it (`3398d404347`,
   `7c3665eddad`) were therefore arguing about the price of a duplicate.
4. The checker `scripts/check-p2-yoshi-egg-efdesc-native.ps1` is entirely static:
   it re-runs the generator and regex-matches generated text plus the two source
   files (`:44-142`), including an assertion that the admission arm's *text* sits
   inside its `#if NDS_P2_YOSHI` guard (`:143-144`). GREEN there proves the bank
   row and the source text exist. It cannot observe a creation failure one layer
   above the renderer, which is where this bug lives.
5. Minor, but it cost a reader: `05fc0add5de`'s message directs a future
   investigator to read `gNdsThunderGroundCoverageReclaimCount` and
   `gNdsGradedQuadTextureRecycles` "beside any remaining rejects". Both symbols do
   exist (`include/nds/nds_native_pikachu_thunderground_coverage.h:131`,
   `src/nds/nds_native_textured_quad.exec.inc:85`), but neither can say anything
   about this row — the egg is not reaching a texture bind at all.

### Proposed fix

One edit. `src/import/battleship_efmanager.c`, inside the compact-pack branch of
`ndsEFManagerMapFileOffset`.

BEFORE (lines 1271-1277):

```c
        const void *mapped = ndsRelocNativeAssetAddress(
            base, (u32)(uintptr_t)source_offset);

        if (mapped == NULL)
        {
            return FALSE;
        }
```

AFTER:

```c
        const void *mapped = ndsRelocNativeAssetAddress(
            base, (u32)(uintptr_t)source_offset);

        if (mapped == NULL)
        {
            /* A desc field can name a Gfx display list rather than a data
             * span. An EFDesc carrying neither 0x1 nor 0x4 hands
             * `*file_head + o_dobjsetup` straight to gcAddDObjForGObj, which
             * stores it as DObj::dv (efmanager.c:2044, objman.c:1420) --
             * dEFManagerYoshiShieldEffectDesc and
             * dEFManagerYoshiEggEscapeEffectDesc both do, and both name
             * YoshiModel 0xA860. The compact battle pack PRUNES geometry and
             * leaves one ENDDL+source-offset identity cell per retained root,
             * outside every retained data span, so the data mapper answers
             * NULL for a perfectly valid source root and the desc is disabled
             * for the whole match. ndsRelocNativeRootAddress is the cell-aware
             * resolver wpManagerMakeWeapon already uses for this exact root
             * (battleship_wpmanager_core.c:377-379): identity on an unpacked
             * file, NULL for an offset that is not a root, so no desc that
             * maps today changes. */
            mapped = ndsRelocNativeRootAddress(
                base, (u32)(uintptr_t)source_offset);
        }
        if (mapped == NULL)
        {
            return FALSE;
        }
```

No new include: `nds/nds_preview_pack.h`, which declares
`ndsRelocNativeRootAddress` at line 140, is already included at
`src/import/battleship_efmanager.c:14`.

Bounds check downstream is satisfied: Yoshi's Model section registers
`data_size = data_bytes = 18,740 = 0x4934` (the `.fpc` section record), and the
egg's root cell lands at Model-relative `0x477C + 54*8 = 0x492C`, so
`relative >= span` at `:1283` does not trip.

After this edit the draw is served by the **pre-existing** owner at
`src/port/renderer_adapter_stage.c:7010-7027` (asset 338 / root `0xA860` /
`nGCCommonKindEffect` / `mobj == NULL`), reached through the DLHEAD1 submit seam:
`gcDrawDObjDLHead1` (`src/port/opening_movie_backend.c:1870-1878`) →
`ndsRendererAdapterSubmitStageDObjNode` case
`NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_DLHEAD1`
(`src/port/renderer_adapter_stage.c:12180-12196`) →
`ndsRendererAdapterSubmitStageDL(dobj, dobj->dl, ...)`. The per-frame env fade
from `fp->shield_health` stays runtime-owned, as the source has it.

Recommended, but **separate and not required** (it has checker blast radius, so
do not fold it into the repair): the entry-effect arm added by `05fc0add5de`
(`src/port/renderer_adapter_stage.c:5280-5288` plus
`src/nds/nds_renderer_native_common.c:4932-4948`) is dead in every compact-pack
build and, in an unpacked lab build, runs *before* the pre-existing owner
(`ndsRendererAdapterTryNativeEntryEffect` is called first, at `:6305`) and
shadows it — so lab and shipped configurations would draw this egg from two
different banks. Removing the arm, its generated bank row and
`scripts/check-p2-yoshi-egg-efdesc-native.ps1` (with `$expectedVerifiers`
19 -> 18 in `scripts/verify-all.ps1`, a call-site literal) collapses that back to
one owner and returns 2,048 B of persistent VRAM. "Measure the config you ship"
argues for doing it; it is a second commit.

### Confidence

**High** on the cause. Falsifiers, in order of cost:

- Read `gNdsEFDescDisabledCount` / `gNdsEFDescDisabledLast`
  (`include/nds/nds_effects.h:211,225`) in a Yoshi match. If `DisabledLast` is
  not `&dEFManagerYoshiShieldEffectDesc` and `gNdsEFDescDeferRecoverCount`
  (`:230`) shows the desc recovering, the mapping is not the blocker and this
  diagnosis is wrong.
- If the pack in the owner's ROM differs from `builds/build/battle-core/06.fpc`
  (rebuild it and re-decode: 0xA860 must be a root cell and must be in no span).
- If, after the edit, Yoshi still vanishes, the next suspect is the draw, not the
  creation, and `gNdsEntryEffectNativeFallbackCount` plus the renderer decline
  reason 20 ("a packed preview reached the draw with no native owner",
  `src/port/reloc_preview_pack.c` decline-witness block) is where to look.

### Open question

None blocking the patch. One cheap runtime confirmation is available if wanted
before applying: `gNdsEFDescDisabledCount` should be **2** higher with Yoshi in
the roster than without (the shield desc and the egg-escape desc), and
`gNdsEFDescDeferRecoverCount` should never count them. Both counters already
exist; no new instrumentation is needed.

---

## Row 2 — Pikachu

```
Bug: down B effect doesn't render all related VFX, missing blue exp on pikachu.
```

### Contract

Down-B is Thunder. The burst the owner is missing is the **self-hit** effect: it
only exists when the descending head actually reaches Pikachu and he enters
`SpecialLwHit` / `SpecialAirLwHit`. An intercepted bolt legitimately produces no
burst; everything below describes the case where the self-hit *does* occur.

Full chain the source spawns for down-B, with the one that is missing marked:

| # | Effect | Spawn site | Kind |
|---|---|---|---|
| E1/E2 | `efManagerPikachuThunderTrailMakeEffect` | `decomp/.../src/wp/wppikachu/wppikachuthunder.c:93, :97` | EFDesc, `efmanager.c:640` |
| E3 | `efManagerDustExpandSmallMakeEffect` | `wppikachuthunder.c:115` | particle `0x55` |
| E4 | `efManagerQuakeMakeEffect(1)` | `wppikachuthunder.c:132` | anim-joint GObj |
| E5 | `efManagerSparkleWhiteMakeEffect` | `wppikachuthunder.c:133` | particle `0x73` |
| E6 | `efManagerImpactShockMakeEffect` | `wppikachuthunder.c:206` | particle `0x25` |
| **E7** | **`efManagerThunderAmpMakeEffect` — the blue burst** | motion event, below | **particle `0x74`** |
| E8 | `efManagerDustHeavyDoubleMakeEffect` | `242_PikachuMainMotion.c:1379` | particle `0x58` |
| E9 | `efManagerQuakeMakeEffect` | `242_PikachuMainMotion.c:1380` | anim-joint GObj |
| E10 | `efManagerShockSmallMakeEffect` | colanim `gmcolscripts.c:872` | EFDesc, `efmanager.c:111` |
| E11 | `efManagerSparkleWhiteScaleMakeEffect` | colanim `gmcolscripts.c:873` | particle `0x5B` |

E7 in detail:

- Emitted by `ftMotionCommandEffect(0, nEFKindThunderAmp, 0,0,0,0,0,0,0)` —
  `decomp/BattleShip-main/decomp/src/relocData/242_PikachuMainMotion.c:1378`,
  first command block of `dPikachuMainMotion_GettingThundered_0x1668`, i.e.
  frame 0 of `SpecialLwHit`. That is the only producer of the kind in the tree.
- `nEFKindThunderAmp = 70`, commented "Pikachu's Thunder self-hit" —
  `decomp/.../src/ef/efdef.h:50`.
- Dispatched by `ftParamMakeEffect`, `decomp/.../src/ft/ftparam.c:2068-2070`.
  Joint id `0` = `nFTPartsJointTopN`, no offset, so the position is a world-space
  snapshot of the TopN joint (`ftparam.c:1890`), not a parented attachment.
- `efManagerThunderAmpMakeEffect` — `decomp/.../src/ef/efmanager.c:4202-4228`:
  `lbParticleMakeScriptID(gEFManagerParticleBankID, 0x74)`,
  `lbParticleAddTransformForStruct(pc, nLBTransformStatusReady)`,
  `LBParticleProcessStruct(pc)`, `if (xf->users_num == 0) return NULL;`,
  `xf->translate = *pos`. No EFDesc, no DL link, no proc.
- Script `0x74` is decompiled at
  `decomp/BattleShip-main/decomp/src/particles/efcommon_scb.c:4489-4534`: kind 0,
  **texture_id 46**, particle_lifetime 34, size 100.0, env colour `00 64 FF 00` —
  **this is the blue**.

### Divergence

**First wrong value: `ndsParticleQuadFrameFor(46, frame)` returns NULL, and the
particle draw loop takes the `continue` that emits zero pixels.** The particle is
created, allocated a transform, processed, and positioned correctly. It simply
has no cell on the quad sheet.

Owning seam: the producer `scripts/generate_nds_particle_banks.py`, live-set and
admission (lines 540-548 and 1867-1901). The runtime consequence is at
`src/import/battleship_lbparticle.c:4498-4527`:

```c
4498:                row = ndsParticleQuadFrameFor(id, pc->frame_id);
...
4499-4502:            if ((row == NULL) ... )
...
4518:                gNdsParticleQuadMissCount++;
4519-4520:            gNdsParticleQuadMissMask[pc->texture_id >> 5] |= 1u << (pc->texture_id & 31u);
4526:                continue;
```

and `ndsParticleQuadFrameFor` itself returns NULL immediately when
`gNdsParticleQuadFirstRow[texture_key] == NDS_PARTICLE_QUAD_FIRST_ROW_NONE`
(`src/import/battleship_lbparticle.c:3020-3031`). The generated bank states the
rule in its own words: "A texture with NO row here draws nothing at all --
`ndsParticleQuadFrameFor` returns NULL and the caller takes the `continue` that
emits zero pixels -- so this table IS the coverage contract"
(`src/nds/generated/nds_particle_banks.generated.inc:88-93`).

**Measured, from the generated bank in HEAD** (decoded with the struct layouts at
`decomp/.../src/lb/lbtypes.h:79-93` and
`include/nds/generated/nds_particle_banks.generated.h:174-186`):

- `gNdsParticleScriptOffsets[0x74] = 0x00002948` — the script IS reachable
  (packed), so the 2026-09-21 half of the fix holds.
- Its header at bank `0x2948` reads `texture_id 46`, `particle_lifetime 34`,
  `size 100.0` — matching the decomp script exactly.
- `gNdsParticleTextures[46] = { 64, 64, 1, 16, 0x00030740, 0x000002B0 }` — the
  texels and palette ARE in the NitroFS pack.
- **`gNdsParticleQuadFirstRow[46] = 0xFF`** — no quad row. Every other script in
  the down-B chain has one:

  | script | texture | quad first row |
  |---|---|---|
  | `0x74` ThunderAmp | 46 | **0xFF — draws nothing** |
  | `0x58`/`0x59` DustHeavyDouble | 16 | 0x0F |
  | `0x5B` SparkleWhiteScale | 24 | 0x18 |
  | `0x55`/`0x56` DustExpandSmall | 15 | 0x0E |
  | `0x73` SparkleWhite | 45 | 0x25 |
  | `0x25` ImpactShock | 30 | 0x1C |

That table is the owner's sentence: *some* of the down-B VFX render, the blue one
on Pikachu does not.

### Root cause

Texture 46 is in no live set in the producer. `QUAD_MEASURED_LIVE`
(`scripts/generate_nds_particle_banks.py:542-544`) tops out at 45,
`QUAD_KO_LIVE` (`:541`) and `QUAD_RESTORED_SOURCE_LIVE` (`:548`) do not name it,
and `QUAD_P1_DEFERRED` (`:540`) names 28/31/35/36 only. Admission sorts
`(not live, source cost, id)` (`:1928-1931`) and is greedy, so a non-live
64x64x3 texture (source cost 12,288, the largest bucket) falls off the tail.

**A previously recorded claim in this repo is wrong, and it is the reason this
row reads as "fixed but still broken".** Commit `9d0de28d3b2` (2026-09-21 17:02,
"P02: Pikachu's Thunder self-hit burst is a source particle, not a generic
spark", an ancestor of r36) did two correct things — routed `nEFKindThunderAmp`
to its source maker (`src/port/reloc_backend_compat_shims.c:8211-8228`) and
seeded the maker so the bank derivation packs script `0x74`. Its message then
says:

> "Texture 46 joins the deferred quad-set rather than evicting anything: this
> burst reads its texels from the packed pack, not from the 2D quad sheet, so
> being outside that sheet is its intended state."

That is not true. There is no general runtime path that binds
`gNdsParticleTextures[id]` texels for a common-bank particle. The only readers of
that block are two dedicated, hand-written preparers with their own immutable GL
names — Fox's blaster glow for texture 27
(`src/nds/nds_renderer_textures_effects.c:5465-5530`) and the Whispy/Hyrule
native texture paths guarded by `NDS_R2_WHISPY_NATIVE_TEXTURES` /
`NDS_P2_STAGE_HYRULE` (`src/import/battleship_lbparticle.c:4479-4502`) — plus the
shield and fireball, which the generated header explicitly carves out
("THE SHIELD IS NOT A QUAD-SHEET CELL", `..._banks.generated.h:97`;
"THE FIREBALL IS NOT A QUAD-SHEET CELL EITHER", `:123`). ThunderAmp has none of
those. For it, the quad sheet is the only draw path, and `gNdsParticleTextures[46]`
serves the sheet *builder*, not the renderer. `include/nds/nds_particle_runtime.h:73-75`
says the same thing from the other side: "a particle whose texture is not in the
atlas draws NOTHING rather than a neighbouring cell" (the counter itself is
declared at `:80`, its mask at `:91`).

This is the identical failure class the producer already documents for texture 12
(the fighter fire burn) at `scripts/generate_nds_particle_banks.py:497-515`: a
live, correctly positioned, correctly scaled particle emitting zero pixels
because its cell was excluded while its texels were packed — and for the same
reason, that the live set is regraded from soak masks and no soak can observe an
effect that was unreachable when the soak ran. The same paragraph tells the
reader what to do next: "after restoring a dead effect, re-check this list before
the soak."

### Proposed fix

Producer only. Never hand-edit `src/nds/generated/nds_particle_banks.generated.inc`.

**Edit 1** — `scripts/generate_nds_particle_banks.py:548`.

BEFORE:

```python
QUAD_RESTORED_SOURCE_LIVE = frozenset((23, 32, 44))
```

AFTER:

```python
# 46 ADDED: Pikachu's Thunder self-hit burst. 9d0de28d3b2 routed
# nEFKindThunderAmp to efManagerThunderAmpMakeEffect and seeded script 0x74, but
# recorded "being outside that sheet is its intended state" -- which is false for
# a common-bank particle: the sheet is its only draw path, so texture 46 was
# packed and still drew nothing. Exactly the texture-12 case above.
QUAD_RESTORED_SOURCE_LIVE = frozenset((23, 32, 44, 46))
```

**Edit 2** — a per-texture cell cap, in the same idiom the file already uses for
texture 7. New constant after `scripts/generate_nds_particle_banks.py:620`
(`QUAD_HEAL_SPARKLE_CELL_MAX = 8`):

```python
# ThunderAmp texture 46 is the only 64x64 in the live set, and it has three
# frames: 12,288 texels at source against the 1,536 the current five sheets have
# free, so admitting it at source size would evict working cells. At 16x16 it is
# 768 and seats in the space already there. The sheet carries shape only --
# glColor supplies the blue from the script's prim/env -- and 16x16 is the cell
# textures 25 and 44 already use.
QUAD_THUNDER_AMP_CELL_MAX = 16
```

and at `scripts/generate_nds_particle_banks.py:1880-1881`:

BEFORE:

```python
            if texture["id"] == 7:
                cell_max = min(cell_max, QUAD_HEAL_SPARKLE_CELL_MAX)
```

AFTER:

```python
            if texture["id"] == 7:
                cell_max = min(cell_max, QUAD_HEAL_SPARKLE_CELL_MAX)
            elif texture["id"] == 46:
                cell_max = min(cell_max, QUAD_THUNDER_AMP_CELL_MAX)
```

Budget arithmetic, measured from the generated bank: five sheets of
128x64 = 40,960 texels; `gNdsParticleQuadFrames` currently occupies 39,424
(matching `NDS_PARTICLE_QUAD_TEXEL_BYTES 39424`,
`include/nds/generated/nds_particle_banks.generated.h:164`), all free space being
on sheet 4 (6,656/8,192 used). 3 frames x 16x16 = 768 texels fits in the 1,536
free without demoting anything; `quad_texture_frame_list(46, 3)` returns
`[0,1,2]` under `QUAD_FRAME_CAP = 6` (`:646`), and A5I3 is one byte per texel so
texels and bytes are the same number here.

Then regenerate with the producer and re-pin its consumers, which is a fixed list
this repo has walked before: `src/nds/generated/nds_particle_banks.generated.inc`,
`include/nds/generated/nds_particle_banks.generated.h`, the NitroFS quad asset
`assets/particles/efcommon_particle_quads.a5i3.bin`, the derived numbers and
checksum in `scripts/check-nds-particle-banks.ps1`, and the `_Static_assert`
checksum in `src/nds/nds_particle_banks.c` — the two have disagreed before and
`9d0de28d3b2` re-pinned both for this reason.

Do **not** add an explosion call to `ftPikachuSpecialLwHitSetStatus`; the trigger
is already correct and that experiment was already tried and rejected.

### Confidence

**High** that the missing quad cell is why the burst renders nothing when the
self-hit occurs. **Medium** that the 16x16 cap is the right size and seats on the
first try — the packer, not a byte count, decides admission
(`scripts/generate_nds_particle_banks.py:1856-1860`), and the 1,536 free texels
may be fragmented across sheet 4's shelves.

Falsifiers:

- Run the producer after the edit and read its own report: if texture 46 appears
  in `excluded`, drop the cap to `QUAD_CELL_LADDER`'s next rung (8) and re-run.
  If even 8x8x3 = 192 texels cannot seat, the sheet is genuinely full and the
  honest options are `QUAD_HELD_FRAME`-style single-frame packing for 46 or
  demoting a measured-dead texture — not growing the atlas.
- Runtime: `gNdsParticleQuadMissMask[1]` bit 14 (texture 46) set, with
  `gNdsParticleQuadMissCount` non-zero, in a match where Pikachu's down-B self-hit
  lands, is direct confirmation of the pre-fix state; after the fix that bit must
  be clear.
- If the burst is still absent with the bit clear, the remaining suspects in
  order are particle-pool saturation (`gNdsParticleStructsMax` /
  `gNdsParticleGeneratorsMax` / `gNdsParticleTransformsMax` against their caps at
  `src/import/battleship_lbparticle.c:265-273` — read these, **not**
  `gNdsParticleRejectCount`, which the file records at `:223-225` as having read 0
  through two real saturations) and `xf->users_num == 0` at
  `decomp/.../src/ef/efmanager.c:4215`.

### Open question

Whether the owner's self-hit case is being reached at all is already answered in
the affirmative by the report itself ("missing blue exp **on pikachu**" implies
the hit lands), but it is worth recording that `9d0de28d3b2` could not reproduce
the self-hit in the scripted walk: `is_thunder_destroy` was set within ~20 frames
and `ftPikachuSpecialLwCheckCollideThunder`
(`decomp/.../src/ft/ftchar/ftpikachu/ftpikachuspeciallw.c:124-175`) returned
FALSE every time. That is a separate weapon-side question and does not gate this
patch. The counter that settles it without a capture is
`gNdsParticleQuadMissMask[1]` bit 14: it can only be set if the ThunderAmp
particle was created, which can only happen if the motion event on `SpecialLwHit`
frame 0 ran.

---

## Row 3 — note only

Reassigned to the integrator, and repaired by `2dc42e97624` ("Graded quad cache:
key on the whole identity, and hold a roster-sized live set") while this document
was being written. One note that touches evidence hygiene rather than the
diagnosis: `NDS_GRADED_QUAD_TEXTURES` was `8u` in the ROM the owner played
(`git show 260b754be00:src/nds/nds_native_textured_quad.exec.inc`, line 34) and is
`16u` in HEAD (`src/nds/nds_native_textured_quad.exec.inc:59`). Anything measured
against current HEAD for that seam is therefore not measuring r36, and the
8-entry figure is the one that describes the owner's symptom.
`gNdsGradedQuadTextureRecycles` and `gNdsGradedQuadLiveHighWater` both exist
alongside `gNdsGradedQuadTextureFails` and separate "recycled, drew" from
"gave up"; the third is the one that says whether 16 is now enough.
