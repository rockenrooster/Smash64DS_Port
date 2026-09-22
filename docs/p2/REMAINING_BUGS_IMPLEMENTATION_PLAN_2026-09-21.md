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
| Falsifier: every `*_DEMO_ANIM_ASSET_ROWS` arm expanded into the token table must also be expanded into the path table, and staged by the Makefile | `scripts/fighters/check_results_demo_motion_closure.py` |
| P01 air-jolt reclaim hook, graded-quad recycle, refused-upload tracker clear (§4.5, §4.5a, §4.5b) | `nds_renderer_textures_effects.c`, `nds_native_textured_quad.exec.inc` |
| K04 Kirby star bake, admission and display routing (§4.11) | `generate_nds_native_item_wave1_core.py`, `nds_native_item_kirbystar.exec.inc`, `renderer_adapter_stage.c`, `battleship_efmanager.c` |
| Face/body witness ON, stretch repair OFF pending the packet twin (§4.8) | `nds_renderer_native_common.c`, `renderer_adapter_fighter.c` |
| Yoshi egg owner reinstated, its checker rewritten to follow the producer (§4.4) | `generate_nds_entry_effects.py`, `check-p2-yoshi-egg-efdesc-native.ps1`, `verify-all.ps1` |

Verification performed: `check_results_demo_motion_closure.py` green (47 rows),
its path arm proven **RED at HEAD** (11 of 11 arms missing) and its Makefile arm
red under mutation; `check-r2-light-stretch.py` green (5 claims);
`test_pikachu_thunderground_reclaim.py` green (8 tests, red under 9 mutations);
`kirbystar`, `mball` and `star` generator `--check` green;
`check-p2-yoshi-egg-efdesc-native.ps1` green and now producer-derived;
`check-nds-particle-banks.ps1` green; `check-architecture.ps1` green.

Two ROMs built, both `NATIVE_ONLY_PASS` with 316 link inputs:

| build | SHA-256 | contents |
|---|---|---|
| r27 | `E077AF60D9E60F75F7C81748D6DE76C10576ED08C570B1DAA812F42B9E998A96` | Results paths, P01, K04, witnesses |
| r28 | `7A2D46B4F47DAC53111C80AAA0A2544EBF614E93A4537474F3DDAB455BC28E0D` | r27 plus the Yoshi egg owner |
| **r29** | `DAB94410BAADDFA0050FBC20134DD55E061E894850BCC2A0A7970F76A99EDA8D` | r28 plus the Kirby-star NO_PROGRAM arms |

**Playtest r29.** r28's hash reproduced byte-identically across a rebuild, so the
build is deterministic. No runtime proof: nothing here is observed on screen.

`check-native-owner-wiring.py` caught a real gap in the K04 work after r28 was
built: `item_kirbystar` was missing from all three NO_PROGRAM guard arms in
`renderer_adapter_stage.c`, so the star's display list would have published a
native failure even on the frames its own executor handled — the exact symptom
K04 was about. Fixed and rebuilt as r29. The checker's four remaining failures
(`kirby_vulcan`, `ness_pktail`, `pikachu_thunder`, `samus_bomb`, all missing
Makefile PREREQ variables) are pre-existing: `git log -S` shows those variables
never existed.

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

### 4.4 Yoshi shield egg invisible — **owner reinstated, cost measured**
`795e2659219` added the correct native owner:
`dEFManagerYoshiShieldEffectDesc` and `dEFManagerYoshiEggEscapeEffectDesc` both
name `&llYoshiModelShieldDObjDesc`, both states call `ftParamHideModelPartAll`
behind `fp->fkind == nFTKindYoshi`, and the root at `0xa860` is an immutable
29-command display list compiling to one group and two triangles.
`252a9aa4290` reverted it for a Boundary RED: arena −4,096 and **14
texture-bind rejects**. The revert was about cost, not correctness.

**Measured tonight, rather than assumed.** Compiling root `0xa860` with the
generator's own `Compiler` shows it *is* textured — a single 64×64 CI4 image
with a TLUT, which converts to PAL16 at **2,048 bytes** of VRAM. Regenerating
with the egg restored moves the entry-effect bank from 63 roots / 65 textures to
64 / 66, and leaves `NDS_ENTRY_EFFECT_STARTUP_ONLY_TEXTURE_BYTES` unchanged at
18,528 — so the egg's texture is **persistent, not startup-only**, which is
correct: the shield appears mid-match, long after the GO retirement that would
have stranded a startup-only entry (the failure recorded on the Sector Z Arwing).

So the 14 rejects were almost certainly a *victim*, not the egg: one more
persistent name on a 256 KB pool where, as §4.5 establishes, a dozen owners hold
names the evictor cannot reach. **Two reclaim paths now exist that did not when
this was reverted** — the coverage hook in §4.5 returns up to 3,072 bytes, more
than the egg costs, and the graded-quad recycle in §4.5a returns VRAM that
previously leaked permanently. Reinstating on top of those is a materially
different proposition from reinstating on top of neither.

Reinstated with the revert's checker, and the checker rewritten because it was
the exact failure this repository keeps recording. It pinned five absolute
numbers — egg ordinal 62, Falcon Kick 60, Falcon Punch 61, root count 63, and
the group index inside the egg's own root row — every one of which had moved
when Samus Grapple and the Falcon Punch TEXID frames landed. It now derives
them: the egg's ordinal must equal `ROOT_COUNT − EGG_ROOT_COUNT` (the tail-append
invariant that keeps every earlier ordinal stable), four named families must
start before it, and the group and triangle counts come from the generator's own
summary line. `verify-all.ps1`'s `$expectedVerifiers` literal moved 18 → 19 in
the same edit.

Owed: the rejects are not yet observed to be gone. Read
`gNdsThunderGroundCoverageReclaimCount` and `gNdsGradedQuadTextureRecycles`
beside any remaining bind rejects — if reclaim is firing and rejects persist,
the pool is genuinely short and the entry-effect bank needs its own hook.

### 4.5 P01 — AIR Thunder Jolt regression — **root cause found and fixed**
It is allocation, and the ground repair was priced in the wrong unit. That
repair moved three images off `sNdsRendererHardwareTextureCache` and onto
dedicated GL names in `sNdsNativeThunderGroundCoverage[]`. Total bytes went
6,144 to 3,120, a *negative* 3,024 — but **reclaimable** bytes went 6,144 to 0,
because `ndsRendererHardwareEvictTexture` is the only reclaim either upload-retry
loop has and it sweeps the cache array alone. The air jolt's single 4,096-byte
A3I5 upload (its render tile is 64x64, from `SetTileSize(..., 0x000fc0fcu)`)
then had nothing to evict, fell to the 1-bit-alpha path, and on a hard refusal
was not drawn at all (`gNdsThunderJoltSubmitStep = 3`, no fallback route).

The bind hand-off candidate is **refuted**: `ndsRendererHardwareBindTextureName`
does update the current name on both arms, the graded quad re-binds
unconditionally, and admission is disjoint (air needs `dobj->mobj == NULL` plus
root `0x0270`, ground needs `dobj->mobj != NULL` plus one of six roots).

Fixed by giving the coverage slots a reclaim hook that
`ndsRendererHardwareEvictTexture` calls **last**, after the ordinary cache
sweep — so the normal victim order is unchanged and dedicated names are
surrendered only when nothing else can be. The ground repair is untouched.
`gNdsThunderGroundCoverageReclaimCount` is the discriminator: non-zero proves a
sibling could not find room; zero across a match with a refused air upload
falsifies this root cause outright.

**The census is itself a finding.** A dozen owners now hold dedicated,
unreclaimable GL names on a 256 KB pool — the IFCommon cloud names, the graded
quad table, entry-effect/shield/KO palettes, Hyrule, impact wave, rebirth halo,
the particle atlas, Whispy, the Fox blaster glow and gun. The ground jolt was
merely the newest. The same repair shape applies to each, and until it does, any
starved sibling fails the same way.

Two amplifiers were found in the same pass and are fixed here.

### 4.5a Graded quad table — eight slots, twelve owners, no recycling
`ndsRendererNativeBindGraded` freed a slot only when `generation` changed, i.e.
at a whole-scene texture reset. Several owners pass a live
`material->current_image`, so their image pointer changes *within* a scene: the
old entry then matched nothing (stale `image`) and could never be chosen as free
(non-zero `name`), so its VRAM leaked and the 8-slot table filled permanently.
After that every graded request in the match returned FALSE and its owner fell
to the 1-bit-alpha path or was not drawn. Twelve owners share the table:
Fushigibana, GLucky, Hitokage, Porygon, Ness PK tail, Thunder Jolt, Purin Sing,
Ness PK Thunder, Pikachu Thunder, Thunder Jolt FX, Samus Bomb and Samus Charge
Shot. This is a strong candidate for the owner reports of effects not playing,
independent of P01.

Fixed by stamping each slot with its bind frame and, **only in the branch that
previously always failed**, reclaiming the least-recently-bound slot not bound
this frame — the same rule the engine's own evictor applies. Nothing that works
today can be made worse: the only paths whose behaviour changes were already
returning FALSE. The victim's name is no longer zeroed before the upload, so
`PrepareIFCommonA3I5Atlas` releases it and the VRAM actually comes back.
`gNdsGradedQuadTextureRecycles` separates "recycled, drew" from "gave up".

### 4.5b A refused upload left the bound-texture tracker lying
On the give-up path of `ndsRendererHardwarePrepareIFCommonAtlas` the hardware
was bound to a texture the function had just deleted, while
`sNdsRendererHardwareBoundTextureName` still claimed the caller's previous name.
`ndsRendererHardwareBindTextureName` elides a rebind when the requested name
equals the tracker, so the next consumer to ask for that previous name would
draw through the deleted texture. Both the tracker and the active-entry pointer
are now cleared on that path. This is live for every generic-bind consumer after
any refused dedicated upload, not only this row.

### 4.6 P02 — Down-B blue self-hit explosion — **narrowed, verify on playtest**
Producer identified exactly: `dPikachuMainMotion_GettingThundered_0x1668` emits
`nEFKindThunderAmp`, `ftParam` maps it to `efManagerThunderAmpMakeEffect`, whose
constructor starts particle script `0x74`. The real maker is routed and `0x74`
is packed. **Verified statically tonight:** the maker is routed at
`reloc_backend_compat_shims.c:8226`, script `0x74` resolves to particle bank
offset `0x2948`, `check-nds-particle-banks.ps1` passes, and
`generate_nds_particle_banks.py:3108-3111` already errors when a reachable
script's texture is unpacked — so the producer side is closed and no routing
work is owed. If the burst is still absent on playtest it is an allocation
refusal, not a missing producer: go to §1.2, not to the effect. Still confirm a
stage-intercepted Thunder does not read as a missing self-hit.

### 4.7 P03 — Poké Ball spawn rays — **all three suspects refuted; instrumented**
First, a premise correction: **the rays are not asset 86.** They are
EFCommonEffects3 = **asset 85, roots `0x0440` and `0x0518`**. The ball is asset
86. Different files, different adapter paths.

- *The effect-layer arm leaves item state* — refuted. The MBall commit is purely
  additive and writes none of the inherited effect state the rays consume.
  Decisively, the rays are handled inside
  `ndsRendererAdapterTryNativeEntryEffect`, called at
  `renderer_adapter_stage.c:6273`, which returns before the MBall block at
  `:8228`. The ball's arm cannot run before the rays' arm for the same DL.
- *Custom matrix kind `0x44`* — refuted, and the earlier commit note that
  flagged it was wrong. `sGCMatrixFuncList` entries are **pairs**, so `0x44` is
  pair 2 = `func_ovl0_800CA024`, a Translate-Scale matrix that already carries
  `dobj->translate` — not `lbCommonRotScaFuncMatrix`, which is pair 3 = `0x45`.
  It differs from the fallback only by a rotation term, and in all four `0x44`
  users the secondary kind is `nGCMatrixKindNull`, so `0x44` **cannot**
  double-translate by construction.
- *Entry-seam ordering* — refuted. The seam edit widened a prototype guard and
  turned an `#if` into an `#error`; it did not move, gate or reorder the rays
  call.

Leading cause, not yet proven: **construction-time refusal.** The rays
descriptor has no `EFFECT_FLAG_USERDATA`, so it takes no EFStruct and the
source's five-free reserve does not protect it; it needs only
`gcMakeGObjSPAfter`, which the `ifCommonSetMaxNumGObj` latch caps once the arena
drops under 25,600. `efManagerMBallRaysMakeEffect` then returns NULL silently and
nothing recorded it — section 1.2 again. Now counted by
`gNdsEntryMBallRaysRequestCount` against `gNdsEntryMBallRaysNullCount`.

A second arm must be excluded before blaming allocation: MBallRays' PRIM ramp
reaches alpha 0 at source tick 50 while its rotation runs to 130, and a 0-alpha
group is skipped. If the MatAnim is not playing, alpha is 0 from frame 0 and the
rays draw invisibly with candidate > 0 and reject 0. Read
`gNdsEntryEffectWitness[0..3]` with `gNdsEntryEffectWitnessRoot = 0x0440`.

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

**Step 4 was investigated and is REFUTED on mechanism.** The Appear figatree
force load does not draw from the same allocator as the Poké Ball: its
destination `fp->figatree_heap` is a caller-owned pre-allocated buffer, the anim
cache has its own arena and is forbidden from calling `syTaskmanMalloc`, and a
resident pack hit allocates nothing. More decisively, the one arena they could
share does not fail by returning NULL — `malloc.c:30` is `while (TRUE);`, so an
overflow **hangs the console**. A single frozen fighter while the rest of the
match keeps running is therefore not general-heap exhaustion.

(That same fact re-reads the Saffron/Kirby report in §4.14: an arena overflow
there would present as a hang, which is exactly what "crashes" describes.)

The *ordering* in step 4 does hold — the entry-focus phase walks
`gGCCommonLinks[nGCCommonLinkIDFighter]` one fighter at a time, so the ball is
constructed before any later fighter's entry, though only if Pikachu precedes
Fox in player-slot order. And the ball does permanently cost DObj/MObj/AObj pool
growth out of `gSYTaskmanGeneralHeap`, which lowers the arena every later entry
sees and can tighten the `gcSetMaxNumGObj` latch. That is a real coupling to
GObj-starved *effects* such as the rays, not to the figatree load.

So steps 1-3 stand and step 4 does not. The freeze is still most likely a
fighter stuck in Appear; what is now open is *why* its animation never
terminates. An event-local witness was added at
`battleship_ftcommon_entry.c:283-320` — it latches once, never rewrites, and
touches no source state:

| global | meaning |
|---|---|
| `gNdsFTCommonAppearOverrunFighter` | 0 = never fired, else `fkind + 1`; written last, so non-zero guarantees the whole record |
| `gNdsFTCommonAppearOverrunAnimFrame` | raw `f32` bits of `anim_frame` at the boundary |
| `gNdsFTCommonAppearOverrunAnimFallback` | snapshot of `gNdsRelocForceFighterAnimFallbackCount` |
| `gNdsFTCommonAppearOverrunArena` | free arena at the boundary |
| `gNdsEntryMBallThrownArenaBefore` / `...Cost` | what the Poké Ball actually costs |

Reading: `Fallback > 0` with `gNdsRelocForceFighterAnimFallbackLastAsset` naming
a Fox Appear animation means the figatree fell back and the fighter can never
reach `anim_frame <= 0` — that is the freeze. `Fallback == 0` with a non-zero
`OverrunFighter` means the animation loaded and the stall is elsewhere, which
refutes step 3 too. Do not stub or defer the Poké Ball: it is source-faithful in
`ftCommonAppearSetStatus` and the owner accepted it.

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

### 4.11 K04 — Kirby spit-out star invisible — **root cause found and fixed**
The renderer was never the first failure. `lbCommonDObjScaleXProcDisplay` — the
`proc_display` of **both** star descriptors — is **an empty function in the
port** (`src/import/battleship_wpmanager_core.c:138-141`), so the restored makers
built the effect and nothing ever handed its tree to the renderer. Falcon Punch
and Yoshi's entry egg hit this identical wall and were repaired the same way.

Two naming traps resolved on the way. `llITCommonDataKirbyStarDObjDesc = 0x5458`
is an offset into **file 86**, not file 251; and what is there is **not a
DObjDesc** — it is `dITCommonObject_StarRod_Weapon_data[22]`, a 22-command
display list. Both star descriptors clear EFDesc flag `0x4`, so
`efManagerMakeEffect` passes `addr + o_dobjsetup` to `gcAddChildForDObj` as a
display list. The Poké Ball sets `0x4` and really does name a DObjDesc, which is
why the twin framing held for the asset but not for the shape.

A whole-image referrer census found exactly three pointers reaching `0x5458`, so
**one bake serves four source owners**: the Star Rod's two weapon swings and both
of Kirby's stars. The Star Rod's swings were invisible for the same reason.

Landed: root `0x5458` baked through the wave1 generator (4 verts, 2 triangles,
no material, no `0xDE`), a native executor, an admission arm gated on
`asset_id == 86 && root == 0x5458` that admits from the weapon *and* effect
layers, and both descriptors routed to `gcDrawDObjDLHead1` — head 1, not the
head-0 tree callback, because source `lbCommonDrawDObjScaleX` submits through
`gSYTaskmanDLHeads[1]` and the source's own `DObjDLLink` selects list 1.
Artefacts regenerated; `kirbystar --check` GREEN, `mball --check` and
`star --check` still GREEN.

Asset-86 census: 22 native consumers, and `0x5458` collides with no existing
root. Owed: trigger spit-out and lose-copy **separately**;
`gNdsItemKirbyStarFromEffectCount > 0` is the only counter that proves K04
rather than the Star Rod.

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
the `ll*` offset class has already caused five recorded failures here, and it
cuts across the grain of the port's asset architecture.
`scripts/menus/stage_reloc_file.py` derives all five offset surfaces —
`include/reloc_data.h`, `diagnostics_mp_taskman_state.c`,
`reloc_backend_assets.c`, `nds_reloc_assets.c` and the Makefile — from
`decomp/BattleShip-main/include/reloc_data.us.h`, which is read-only source of
truth. A repack makes one asset's offsets diverge from that authority, so the
repacker has to become a recognised producer for this file rather than a one-off
script. It needs its own change, its own falsifier (decode the repacked file and
assert every symbol still lands on a valid sprite header with a valid bitmap
pointer), and a visual check of READY/GO, GAME SET, TIME UP and the stock lamps.
Do not fold it into a bug batch.

Rejected alternatives, recorded so they are not re-derived. An offset-preserving
shrink does not exist: headers are interleaved with texels and the *used* data
is at the tail. Zeroing the texels saves nothing, because the loader allocates
the file's length. Biasing the file base pointer to load only a suffix fails,
because the "GO!" sprites at `0x4D78`, `0xA730` and `0xC370` are dereferenced and
`ndsIFCommonMakeSObjForGObj` still passes every letter sprite to
`lbCommonMakeSObjForGObj`.

### 6.2 Custom matrix kinds still on the translate-bearing fallback

`0x44`, `0x49`, `0x4A`, `0x51`. `0x44` is `dEFManagerMBallThrownEffectDesc`.
Each needs its own census before a case is added; adding one changes behaviour
for every descriptor using that kind.

### 6.3 Kirby copy-hat low-detail deferral

The low-detail hat image (7,636 bytes) loads eagerly at copy time for every
Kirby. Deferring it clears the floor outright (23,204 + 7,636 = 30,840). Before
doing it, count binds of `sNdsNativeKirbyHatImages[slot][1]` in a natural
two-player copy match — if it is bound, the lever is wrong. Do not guess this one.
