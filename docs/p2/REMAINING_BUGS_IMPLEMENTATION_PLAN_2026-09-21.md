# Remaining owner bugs — implementation plan, 2026-09-21 (night)

Queue: `docs/BUGS.md`. Brief: `docs/p2/Smash64DS_BUGS_Consolidated_Fix_Instructions_2026-09-21.md`.
Evidence: `artifacts/performance/2026-09-19_remaining-bugs.md`.
Board cursor: `docs/P2_EXECUTION_BOARD.md`.

This plan covers every row currently in `docs/BUGS.md`, including the two rows
the consolidated brief does not carry (Yoshi's shield egg, and the Pikachu →
frozen Fox opponent). Owner-deferred rows are diagnosed and left unimplemented.

Nothing here is an acceptance claim. The owner playtests in the morning.

---

## 1. The finding that reorders this plan

Two independent root causes account for most of the remaining queue, and
neither is the effect-by-effect renderer work the rows look like.

### 1.1 Static routing is not a runtime load — and the path field was never consumed

**This is the whole of R02, R03 and K06, and it is now fixed.**

`NDS_*_DEMO_ANIM_ASSET_ROWS` in
`include/nds/generated/nds_fighter_production.generated.h` carries three
fields per row: the `ll*FileID` symbol, the asset id, and the NitroFS path.
`sNdsRelocDemoAnimTokens` in `src/port/reloc_backend_assets.c:3717` expanded the
first two, so every Results demo pose resolved its asset id and answered
`ndsRelocIsFighterAnimID`. **Nothing anywhere expanded the third.**

`ndsRelocAssetFindEntry` (`src/nds/nds_reloc_assets.c:688`) resolves a path from
three sources: the Mario range `0x1f3..0x281`, the Fox range `0x282..0x31f`, and
`ndsRelocAssetP2FighterAnimEntry`, which formats
`nitro:/reloc/reloc_animations/<stem><NNN>` over contiguous segments starting at
`0x320`. The 35 demo ids are `357..479` (`0x165..0x1df`) and live under
`reloc_submotions/` with source-derived names. They fall in no range, match no
formatter, and had no literal row. Measured: **all 11 demo arms had a token
route and zero path routes at HEAD; zero of the 35 ids collide with any existing
row or range.**

So `ndsRelocForceLoadFighterAObj16File` reached its
`ndsRelocAssetGetPath(asset_id) == NULL` guard
(`src/port/reloc_backend_assets.c:14957`), recorded an external-fixup fail and
returned NULL. `lbRelocGetForceExternHeapFile` then handed back the raw heap, and
`ftMainSetStatus` assigns `fp->figatree` **unconditionally while discarding the
return value** — so every fighter bound whatever figatree the heap still held,
which is its last battle motion. No decline, no reject, no halt.

That is exactly the reported symptom: fighters holding a battle pose on the
Results screen, No Contest not clapping, Kirby stuck in EggLay. It also explains
why packing the 35 payloads and closing 47/47 static routes changed nothing on
screen, and why the new `gNdsRelocExternHeapUnresolvedCount` witness would have
read **zero** — the failure was one arm further down, in
`gNdsRelocForceFighterAnimFallbackCount`, a counter shared with battle
animations that nobody had read for Results.

**Landed.** Both files below, plus a falsifier that follows the producer.

### 1.2 The arena is the shared upstream cause of several "missing effect" rows

Measured in `artifacts/performance/2026-09-19_remaining-bugs.md`, "Arena census"
(arena 925,184 B):

| roster / stage | free at GO | result |
|---|---|---|
| Mario/Fox Dream Land | 61,124 | fine |
| **Pikachu/Fox Dream Land** | **7,556 (min 3,184)** | `gcSetMaxNumGObj(48)` then `(56)` |
| Pikachu/Samus Dream Land | — | malloc halt in `scVSBattleStartBattle` |
| Mario/Fox Saffron (after gating) | min 2,844 | starts, GObj cap latched |

`ifCommonSetMaxNumGObj` caps the GObj count for the rest of the match below a
25,600-byte floor, after which every `efManagerMakeEffectNoForce` returns NULL
with **zero native rejects**. Kirby's copy costs a further 18,480 bytes
permanently (two ~8,556-byte hat details loaded eagerly at
`reloc_backend_compat_shims.c:12522-12525`), and free never recovers.

Rows this plausibly drives: the frozen Fox opponent (§4.9), Yoshi's intermittent
shield egg (§4.4), intermittent VFX generally, and the Saffron/Kirby crash
(§4.14). Treat "the effect is missing" and "the effect could not be allocated"
as different failures and prove which one applies per row — but stop spending
per-row effort on symptoms the arena explains.

**The top unstarted lever is §6.1, and its case is now much stronger than the
receipt's original note.**

---

## 2. What landed tonight

| Change | Files |
|---|---|
| Register the 35 Results demo animation NitroFS paths so the force loader can name them (§1.1) | `src/nds/nds_reloc_assets.c` |
| Falsifier: every `*_DEMO_ANIM_ASSET_ROWS` arm expanded into the token table must also be expanded into the path table | `scripts/fighters/check_results_demo_motion_closure.py` |

Verification performed: `check_results_demo_motion_closure.py` green (47 rows);
the new arm proven **RED at HEAD** (11 of 11 arms missing) and green in the
worktree; `src/nds/nds_reloc_assets.c` compiles clean under the shipping flags.

---

## 3. Delegated, in flight

Three investigators on disjoint file scopes; this session is the sole builder
and integrator.

| Scope | Rows | Owns |
|---|---|---|
| A | P01 AIR Thunder Jolt regression | `pikachu_thunderground` generator + exec/coverage inc |
| B | P03 Poké Ball rays, K04 spit-star, and the frozen-Fox hypothesis (§4.9) | entry seam, Kirby common header, item wave1 generators, `renderer_adapter_stage.c`, `renderer_adapter_matrix.c` |
| C | P04 / K02 / J01 face-body normals | `nds_renderer_native_common.c`, `renderer_adapter_fighter.c`, `generate_nds_native_owners.py` |

Each is required to deliver a **sibling census** of any shared path it changes
before landing. That requirement is not procedural: three regressions in one day
(the `0x45` matrix arm, the ground-jolt texture converter, the Poké Ball
admission) were each correct for the one consumer they were written for, each
carried a green mutation test, and not one was caught by CI.

---

## 4. Row-by-row

### 4.1 M01 — Characters / VS Record must not be selectable — **implemented**
`ndsMenuShellDataKindActivatable` (`src/nds/nds_menu_shell_data.c:189`) rejects
both kinds before the confirmation cue, the BGM stop and the scene request,
rechecked on every activation so a restored `scene_prev` cannot enter a deferred
screen. Rows, focus movement and Sound Test are untouched. Landed in
`4d265e3ad34`. Owed: playtest.

### 4.2 M04 — CSS hover-to-preview delay — **owner deprioritised**
Residual is one 30,160-byte owner-image read; dwell is 2 cold / 1 warm. Do not
re-solve with a debounce. No work planned this cycle.

### 4.3 R02 / R03 / K06 — Results poses, No Contest claps, Kirby's pose — **root cause fixed**
See §1.1. The static-routing and payload-packing halves were already proven
necessary; the path half was the missing one. Owed: playtest of all
source-reachable Win1/Win2/Win3, ordinary Lose and No Contest, including Kirby's
two-win restriction. Keep the Link "Claps" program-4 and Luigi Win2 hand-root
repairs.

### 4.4 Yoshi shield egg invisible — **blocked on a build, cause narrowed**
`795e2659219` added the correct native owner:
`dEFManagerYoshiShieldEffectDesc` and `dEFManagerYoshiEggEscapeEffectDesc` both
name `&llYoshiModelShieldDObjDesc`, both states call `ftParamHideModelPartAll`
behind `fp->fkind == nFTKindYoshi`, and the root at `0xa860` is an immutable
29-command display list compiling to one group and two triangles.
`252a9aa4290` reverted it for a Boundary RED: arena −4,096 and **14
texture-bind rejects**.

Fourteen rejects from a two-triangle quad is the anomaly, not the egg's
geometry. Plan: reinstate the owner, then read the bind rejects — the candidates
are material state requesting a non-resident image, and the insertion disturbing
a shared bind cache. Do not re-litigate the owner. The generator
(`scripts/3d_vfx/generate_nds_entry_effects.py`) is adjacent to agent B's entry
work, so sequence this after B lands.

### 4.5 P01 — AIR Thunder Jolt regression — **agent A**
Ground jolt accepted; the air jolt regressed through a shared-resource
interaction, not a direct edit. Candidates in order: allocation/eviction order
against the three pinned A5I3 names, then the bind hand-off at
`nds_native_pikachu_thunderground.exec.inc:102`. Keep the ground repair.

### 4.6 P02 — Down-B blue self-hit explosion — **narrowed, verify on playtest**
Producer identified exactly: `dPikachuMainMotion_GettingThundered_0x1668` emits
`nEFKindThunderAmp`, `ftParam` maps it to `efManagerThunderAmpMakeEffect`, whose
constructor starts particle script `0x74`. The real maker is routed and `0x74`
is packed. Owed: confirm actual script-0x74 engagement and pixels, not Thunder
role-mask coverage; and that a stage-intercepted Thunder does not read as a
missing self-hit. If it is still absent, check allocation refusal first — §1.2.

### 4.7 P03 — Poké Ball spawn rays — **agent B**
Ball visible and accepted; rays regressed behind the same admission. Suspects:
the new effect-layer arm leaving item state; descriptor kind `0x44` still on the
translate-bearing fallback beside the `0x45` arm just changed; entry-seam
ordering. Agent B now owns `renderer_adapter_matrix.c` for the `0x44` case.

### 4.8 P04 / K02 / J01 — face vs body colour — **agent C**
Residue is facing-dependent: fixed facing left, wrong facing right, and correct
facing right in the ledge-balance pose. A mirror `lr` gives a
negative-determinant modelview, the geometry engine transforms normals by its
upper 3×3, and the diffuse term collapses; the ledge-balance root carries a
rotation instead, so its determinant stays positive. Do not touch the clamp or
the white-prim exemption — those are facing-independent and the left-facing case
is correct.

### 4.9 Pikachu chosen → Fox opponent frozen and cannot be hit — **hypothesis, agent B**
Not in the brief. Reconstructed chain:

1. "Frozen and cannot be hit" is the signature of a fighter stuck in **Appear**.
   `ftCommonAppearInitStatusVars` sets `is_ghost = TRUE` (unhittable),
   `is_shadow_hide` and `is_playertag_hide`, and the Appear proc takes no input.
   `src/import/battleship_ftcommon_entry.c:190-194` records that an earlier
   gating bug in this same function already "made the fighters look frozen".
2. "Sudden death works" fits: match start uses `ftCommonAppearSetStatus`; sudden
   death uses `ftCommonEntrySetStatus` / `ftCommonEntryNullProcUpdate`, a plain
   `entry_wait` countdown that needs no Appear animation.
3. Appear exits only on `fighter_gobj->anim_frame <= 0.0F`, which requires Fox's
   AppearR/L figatree to load. The same silent-fallback shape as §1.1 applies.
4. Pikachu's entry is the only one that now constructs an item effect, inside
   `ftCommonAppearSetStatus`, on the tightest roster in the game (§1.2), with the
   entry-focus phase calling each fighter's `ftCommonAppearSetStatus` in
   sequence.

If confirmed this is the fourth instance of one shape: a newly admitted owner
leaving state the next consumer assumed it had. Required next: prove whether the
Poké Ball construction and the Appear figatree force-load share an allocation
path and an ordering, then read
`gNdsRelocForceFighterAnimFallbackCount` / `...FallbackLastAsset`. Do not stub
the effect; the ball is accepted.

### 4.10 K03 — Kirby jab flurry at wrong positions — **fixed, awaiting playtest**
`ftcommonattack100.c:91` read Kirby's rapid-jab effect table as
`gFTDataKirbyMainMotion + (intptr_t)&llKirbyMainMotionftKirbyAttack100Effect`.
In BattleShip that name is a link-time offset symbol so `&name` IS the offset;
in the port it is an ordinary `uintptr_t` identity token, so `&name` is a `.bss`
address. The table was read from far outside KirbyMainMotion, making offset,
rotate, vel and add garbage for all five maker arguments — the flurry was never
a renderer placement bug. Fixed in `5947945361f` with the address-as-offset form,
`0x1220` verified against `reloc_data.us.h`. `test_kirby_vulcan_placement.py`
8/8, two arms RED before the commit by design. This is the fifth recurrence of
the `ll*` trap.

### 4.11 K04 — Kirby spit-out star invisible — **agent B**
Makers restored (constructs), not GObj-starved (~10 free slots after the latch),
renderer side never examined. Twin of the Poké Ball:
`llITCommonDataKirbyStarDObjDesc` addresses ITCommonObject (file 86), the same
asset as `dEFManagerMBallThrownEffectDesc`; the star's roots have no bake and no
admission arm, so they reach the submit path, nothing claims them, and the
result publishes `NO_PROGRAM` — which reads as missing geometry. Repair shape is
the one proven on the ball. Census asset 86 first. Pair with LoseKirbyStar; keep
the entry warp star separate.

### 4.12 S01 — Peach's Castle roof texture — **owner deferred**
Geometry renders; texture continuity is the remaining dimension. A run can emit
correct triangles while holding a wrong image, missing TLUT, stale segment
material or another group's UV/tile state. Diagnose only.

### 4.13 S03 — Zebes acid edges and ground-light gradients — **owner deferred**
Both subparts. Constant polygon alpha alone does not decide whether the gradient
belongs to the texture, vertex or filtering stage. Diagnose only.

### 4.14 S04 — Saffron garage gate — **bindings landed; crash open**
`a6c548f40de` found the cause: `nds_renderer_native_owners.c:3642` routes any
binding that is neither rigid nor in `camera_binding_mask` through
`ndsRendererNativeStageTask51EnsureWorld`, which multiplies a generator-baked
constant world matrix that cannot follow an AObj. Yamabuki's rigid mask is 0 and
its camera mask was `0x4894`, so the gate's bindings took the baked constant —
and that baked pose is the CLOSE animation's end state. The always-open look is
layer geometry from file 112. Mask widened to `0xE4894`; the generator now
derives which bindings a runtime joint animation moves by decoding the real O2R
payload.

Open, from the owner: **"UPDATE crashes with Kirby."** Two candidates, in order:
(a) the arena — Saffron already runs at min 2,844 bytes free and a Kirby copy
costs 18,480 permanently (§1.2), so this may not be the gate change at all;
(b) the live-compose path now taken by bindings 17-19 reading a
`binding_composed[]` slot that the movement accessor never populates for them.
Settle (a) before touching (b): an exception capture with free-arena at the
fault is one run. Also still open and separate: coplanar z-fighting on the
closed panel, only visible now that the panel moves.

### 4.15 Rows with no open text
`VS options` (M02 Damage cadence accepted and removed), Link, Samus, Captain
Falcon, Ness, Yoshi's Island, Sector Z, Audio. No task.

---

## 5. Order of work

1. Land §1.1 and its falsifier. **Done.**
2. Integrate agents A, B and C serially, each with its sibling census. One build
   at a time; the generators write shared paths outside `$(BUILD)`.
3. §4.9 frozen Fox, with agent B's ordering answer.
4. §4.4 Yoshi egg, after B releases the entry-effects generator.
5. §4.14 Saffron crash: exception capture first.
6. §6.1 arena lever, as its own change with its own proof.

---

## 6. Levers not started

### 6.1 IFCommonGameStatus — the letter texels are never uploaded

The receipt already recommended repacking this file's RGBA32 payload to the
16-bit form the DS uploader reduces it to, worth ~66 KB in every battle. The
static reading below makes the case stronger and changes the shape of the fix.

`IFCommonGameStatus` is **152,368 bytes resident**, the single largest battle
asset after the stage. Its consumers are:

- `dIFCommonTrafficSpriteOffsets[color_id]`, `llIFCommonGameStatusRodSprite`
  (`0x20990`), `...FrameSprite` (`0x21760`), `...RodShadowSprite` (`0x21878`)
  and the nine lamp sprites (`0x21950..0x25290`) — real sprites, all at offsets
  **≥ `0x20990`**;
- `llIFCommonGameStatusBlueLetterTSprite` (`0x144e0`) and
  `...BlueLetterGSprite` (`0x20788`) in `src/import/battleship_ifcommon.c:162-164`
  — used **only for pointer identity**, to decide whether the starting
  announcement is TIME UP or GAME SET, before handing off to
  `ndsIFCommonNativeOamPrepareAnnouncement`. Their pixels are never uploaded.

So roughly the first 132 KB of the file is texel data for letters the port draws
natively and reads only two header addresses from. No offset-preserving shrink
exists (headers are interleaved with texels, and the used data is at the tail),
so this needs a real repack: rewrite the O2R payload and its internal pointer
table, then regenerate the 24 `ll*` offsets in `include/reloc_data.h` — which are
the single source of truth, since `lbRelocGetFileData` is `file + (uintptr_t)&llX`
and the decomp's own tables resolve through the same symbols.

Sizing: recovering even 120 KB takes the Pikachu/Fox pair from 7,556 bytes free
to roughly 128,000 — clear of the 25,600 GObj floor by a wide margin, with the
Kirby copy's permanent 18,480 absorbed. That is the difference between "some
pairs cannot start a match" and headroom.

Risk, and why it was not done tonight: it rewrites the most visible HUD asset,
and the `ll*` offset class has already caused five recorded failures in this
repository. It needs its own change, its own falsifier (decode the repacked file
and assert every symbol still lands on a valid sprite header), and a visual
check. Do not fold it into a bug batch.

### 6.2 Custom matrix kinds still on the translate-bearing fallback

`0x44`, `0x49`, `0x4A`, `0x51`. `0x44` is `dEFManagerMBallThrownEffectDesc`.
Each needs its own census before a case is added; adding one changes behaviour
for every descriptor using that kind.

### 6.3 Kirby copy-hat low-detail deferral

The low-detail hat image (7,636 bytes) loads eagerly at copy time for every
Kirby. Deferring it clears the floor outright (23,204 + 7,636 = 30,840). Before
doing it, count binds of `sNdsNativeKirbyHatImages[slot][1]` in a natural
two-player copy match — if it is bound, the lever is wrong. Do not guess this one.
