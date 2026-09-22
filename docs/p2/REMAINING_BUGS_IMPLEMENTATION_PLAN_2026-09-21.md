# Remaining owner bugs — implementation plan, 2026-09-21 (night)

Queue: `docs/BUGS.md`. Brief: `docs/p2/Smash64DS_BUGS_Consolidated_Fix_Instructions_2026-09-21.md`.
Evidence: `artifacts/performance/2026-09-19_remaining-bugs.md`.
Board cursor: `docs/P2_EXECUTION_BOARD.md`.

This plan covers every row currently in `docs/BUGS.md`, including the two rows
the consolidated brief does not carry (Yoshi's shield egg, and the Pikachu →
frozen Fox opponent). Owner-deferred rows are diagnosed and left unimplemented.

Nothing here is an acceptance claim. The owner playtests in the morning.

---

## 0. STATUS AT 2026-09-22 09:20 — read this first

Root `smash64ds.nds` is **r36**, sha256 `1CECBEF5...3BA1`. A copy is at
`builds/remaining-bugs-playtest-r36/`. r32 was `594EB9BA...`.

**Reproducibility established, after I got this wrong once.** Two builds from a
fully regenerated tree (`src/nds/generated/` and `builds/build` cleared, the
one tracked file in there restored) both produce `1CECBEF5...3BA1` with a
clean tree. Earlier I saw a different hash from an *incremental* build over
mixed lab-flag artifacts and published that as "the shipped ROM was built from
contaminated assets" — wrong, retracted, and the retraction is measured. All
21 python static checkers are green on that regenerated tree, and a 120 s
unattended boot reaches the attract loop with zero malloc overflow, zero
objman panic and zero preview-pack failure.

Everything below is pushed; nothing here is an acceptance claim.

### Fixed and awaiting your playtest

| Row | What it actually was |
|---|---|
| Results poses, No Contest (R02/R03/K06) | 11 demo-anim arms had a token route and no path row, so the load failed silently |
| Pikachu neutral-B air jolt | the ground repair left reclaimable VRAM at 0, so the air upload had nothing to evict |
| Kirby spit-out star | `lbCommonDObjScaleXProcDisplay` is an empty function in the port |
| Yoshi shield egg | reinstated; it had been reverted for cost, not correctness |
| **Face/body colour — Pikachu, Kirby, Jigglypuff** | the packet REPLAY path re-derived the shade word without the clamp the live draw applies |
| **Ness character select** | **a real hang in the shipping ROM.** A stale header truncated Ness's owner image by 18 entries |
| Kirby copy arena | the low-detail hat is unreachable below 3 fighters; skipping it returns 7,636 B |
| **Poke Ball spawn VFX** | never a render fault -- the item ball's ray call was commented out behind a note whose stated blocker had since been fixed |
| Item-appear actor | a refused GObj was attaching its process to `gGCCurrentCommon` instead; hardening only, nothing shows it firing |

**Two of those deserve a second look from you because I had them wrong
earlier in this document.** The face/body row: your own note said holding
neutral B fixes it, and a static arithmetic error cannot be cured by a button.
That ruled out the light-vector stretch this plan spends section 5 on. The Ness
row: I wrote "not proven reachable by a human" — wrong, the halt's three guards
are all 1 in the shipping config with the walk off, so hovering Ness hung it
for anyone.

### Both rows I had parked on you are now closed

- **Fox frozen** — I measured `pkind` instead of assuming: the walk's match IS
  your setup (P0 Pikachu human level 3, P1 Fox CPU level 2). Fox changed status
  six times across eight in-match samples, and `is_ghost` read 0 for both at
  every sample — which refutes the stuck-in-Appear mechanism, the only thing
  that makes a fighter both frozen and unhittable, rather than merely failing
  to see the symptom.
- **Poke Ball rays** — root-caused to a commented-out call, not rendering. See
  the table above and `docs/p2/BUG_NOTES.md`.

### One thing about the Fox row worth knowing before you test it

You wrote that row on **2026-09-21**, and **18 commits to `src/` have landed
since** — including this batch's packet shade clamp, the Ness owner image, the
Kirby low-detail hat deferral, the item-appear actor guard and the restored
Poké Ball rays call. So the behaviour you described was observed on a
pre-r36 build, and several of those commits touch fighter rendering and arena
occupancy.

That is not a claim the row is fixed. It is a reason to re-test it
deliberately rather than assume it persists: if Fox behaves on r36, the row may
have been carried off by one of the eighteen, and the question becomes which.
If it still freezes on r36, the five questions below are what narrow it.

### If Fox freezes in the morning, this is what would settle it

I have measured Fox alive and hittable three separate ways in a fresh VS match
(status changing, all hurtboxes Normal, `is_ghost` 0), in your configuration —
human Pikachu vs a level-2 CPU Fox. So whatever you are hitting differs from
that in some way I have not guessed. Five things, in the order that would
narrow it fastest:

1. **Does it happen on the very first match after boot, or only later?** If
   only later, it is state carried across matches, and that is a completely
   different search from a creation-time failure.
2. **Which stage?** Every measurement I have is Dream Land, because that is
   what the harness preset seeds. If it is stage-specific that is the single
   most useful fact.
3. **Is Fox a CPU or a second human slot?** A human slot with no controller
   stands still legitimately — but it should still be hittable, so if yours is
   a human slot AND unhittable that is a different defect from the one I
   looked for.
4. **Time or Stock, and what limit?** My samples cover the opening of a
   one-minute Time match. A freeze that begins later, or after a KO and
   respawn, is outside everything I have run.
5. **Does Fox animate at all — idle breathing, turning — or is it a statue?**
   Animating-but-unresponsive and completely static are different failures.
   Your "cannot be hit" suggests the latter; my instruments say neither is
   happening in a fresh match.

The clue I still cannot use is your best one: **"sudden death works."** Sudden
death re-creates the fighters, so it points at creation-time state — and every
sample I can take begins after creation has already succeeded. If you can tell
me whether the first match after a fresh boot is affected, that alone decides
whether creation is the place to look.

### Previously open, retained for the record

| Row | State |
|---|---|
| Pikachu → Fox frozen | **My refutation was unsound and is withdrawn.** It rested on two scalar counters that are not per-slot, and neither covers collision or physics anyway. Your "sudden death works" is the strongest clue in the queue — it points at creation-time state, not fighter data. |
| Poké Ball rays | They construct and submit (`req=2 null=0 cand=100`). That is not a drawn pixel. Needs your stage and mode. |
| Saffron + Kirby crash | Unreproduced across two matches, and now also **unmechanised**: the GObj cap latch cannot crash this port, because every maker consumer substitutes `gGCCurrentCommon` on NULL. If you hit it again, the one thing that would help most is whether Kirby had **swallowed** someone first. |

### New guards, both mutation-tested RED

- `scripts/check-r2-shade-twin.py` — the two shade derivations must stay identical.
- `scripts/fighters/check_native_owner_image_spans.py` — 1,140 root spans across
  104 owners; catches the Ness class before it ships.

Neither is wired into `verify-all`; fighter checkers here are run by hand and
`expectedVerifiers` is a fragile literal. Run both after any owner regeneration.

### Static checkers: 21 of 21 green, up from 17

All four that were red were pre-existing, not from this batch. Three are now
fixed:

- `check-native-owner-wiring.py` — four owners wrote their emit-rule
  prerequisites inline instead of through a `_PREREQ` variable, and the
  matcher could not name `pikachu_thunder`'s and `samus_bomb`'s guards because
  it strips the fighter name and they then tie. **Both guards were in fact
  present in all three arms.** The rebuilt ROM is byte-identical, which proves
  the Makefile hoist is inert.
- `check_native_owner_weld_consistency.py` — had **never run**. It assumed one
  binding per root; Mario has 16 roots and 14 bindings, so it died with
  IndexError before testing anything. Now green and reporting real content.
- `check_nds_native_stage.py` — a consumed-field manifest one commit behind
  `76b3c1c6366` (yesterday's 0x45 matrix fix started reading
  `dobj.xobjs_num`). Red since then, hiding everything else it audits.

- `check_nds_native_owner_hierarchy.py` — Mario's retained packet against the
  direct draw, 35 of 960 corners each off by exactly ±1 unit per axis. I first
  read that as a rounding difference and planned to defer it; it is the **DS
  coverage seam guard**, which deliberately moves selected boundary vertices
  one VERTEX16 lattice unit so a material seam overlaps by at most 1/16 unit.
  Not a runtime divergence at all — the guarded positions are what the
  generator bakes and both runtime paths draw. Only the checker's direct trace
  was unguarded, re-deriving from canonical vertices. It now reads the guarded
  positions the context already carries.

No ROM change came from any of it: the ROM is byte-identical across all three
rebuilds.

### One correction to section 5 below

`NDS_R2_LIGHT_VECTOR_MATRIX` and `NDS_R2_LIGHT_VECTOR_STRETCH_FIX` stay default
**0**. The stretch is real (row_norm 4695–4911 against a rigid 4096, determinant
positive) but it was never the reported defect. Section 5 is kept for the
measurement, not as a live lever.

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
| r29 | `DAB94410BAADDFA0050FBC20134DD55E061E894850BCC2A0A7970F76A99EDA8D` | r28 plus the Kirby-star NO_PROGRAM arms |
| r30 | `9E7F5EF91D86790A4E22CE3811288C256970E0664EBA612438F5207BFB4AFB00` | r29 plus the Kirby low-detail hat counter |
| **r31** | `86A0AE715A431F7642910DCF58F8BA00B5224AC9C66BDC22184DB5812554D366` | r30 plus the stretch classification kept readable with the repair off |

**Playtest r31.** r28's hash reproduced byte-identically across a rebuild, so the
build is deterministic. **Runtime evidence WAS taken** — see §7. Two repo harnesses could not produce
it (`probe-battle-progress.ps1` ignores `-Build` for this target;
`verify-nogba-smoke.ps1` passes on desktop wallpaper — both recorded in
`docs/p2/BUG_NOTES.md`), but a direct `arm-none-eabi-gdb -batch` attach against a
purpose-built Pikachu-vs-Fox ROM worked and answered three rows. Do not read the
failed probe's `presented=0 / allocfail=187 / __excpt_entry` output as a crash
report — the same output came back for a ROM the owner had already played.

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

### 6.0 THE ARENA CENSUS IS STALE — read this before using any figure below

Measured 2026-09-22 by walking the shipping build into real matches. Every
number the arena reasoning in 6.1/6.1b was built on has moved, and not slightly:

| roster / stage | census 09-20 | measured 09-22 | latch |
|---|---|---|---|
| Mario/Fox Dream Land | 61,124 | **85,708** | never fired |
| Pikachu/Fox | 7,556 | **48,216** | never fired |
| Saffron, Kirby | 2,844 | **32,504** | never fired |

`ifCommonSetMaxNumGObj` fired in none of them. **The arena is not the blocker
for the rays, the frozen Fox, or Saffron+Kirby**, and every row routed there on
the strength of the census has to be explained some other way. Sections 6.1 and
6.1b below remain accurate about SIZES — the FGM cache really is 237,568 bytes
and really is right-sized — but their premise that ~18 KB must be found is
withdrawn.

### 6.1b The arena, measured — and it needs far less than the big lever

Three rows point here, so the deficit was measured rather than estimated.

| configuration | arena | free (min) | GObj latch |
|---|---|---|---|
| shipping shell, Pikachu/Fox (recorded) | 933,376 | **7,556** | fired |
| full-roster direct battle (measured 09-21) | 1,220,352 | **121,192** | never fired |

The direct-battle target is 286,976 bytes *larger* because it does not link the
menu shell, the 1P game or the UI kit — and BSS and linked image come out of the
arena one for one. That is why the arena story cannot be reproduced in a
direct-battle probe at all: remove the menus and the problem disappears.

**The floor is 25,600 and the shipping figure is 7,556, so the whole deficit is
`+18,044 bytes`.** Every lever below is larger than that, which reframes this
from "find 130 KB" to "find eighteen".

**The largest single reclaimable item is not the HUD asset — it is the audio
sample cache.** `arm-none-eabi-nm --size-sort` on the shipping ELF puts
`sNdsAudioFgmCache` at **237,568 bytes** of BSS, a quarter of the arena, ahead of
`gSYFramebufferSets` at 147,840 (already reused for idle packet storage). Its
layout is eight slots pinned by a `_Static_assert`:

    1 x LARGE   60 KiB
    2 x MEDIUM  40 KiB
    1 x COMPACT 28 KiB
    4 x SMALL   16 KiB   = 232 KiB

Candidate trims, all of which clear the 18,044-byte deficit:

| change | reclaims | structural cost |
|---|---|---|
| both MEDIUM 40 -> 28 KiB | 24,576 | none — 8 slots kept, 60 KiB slot still covers any cue |
| drop two SMALL slots | 32,768 | slot count 8 -> 6 |
| COMPACT 28 -> 16 KiB plus one SMALL dropped | 28,672 | slot count 8 -> 7 |

**The first is now REFUTED — do not act on it.** A cue histogram (573 cues, in
the receipt) shows a miss is not a re-read: `ndsAudioFgmCacheAcquire` returns -1
and the cue **does not play**. Trimming MEDIUM to 28 KiB would push 34 cues in
the 28-40 KiB band onto the single LARGE slot, taking the population depending
on that one slot from 28 to 62. Two such cues overlapping already fail today.

What bounds the real options is concurrency: the measured peak is **six of eight
handles**. Dropping two SMALL slots reclaims 32,768 and clears the deficit, but
it consumes exactly the headroom that peak leaves.

**This is an owner decision, not an engineering one, and that is why it is not
implemented here.** It trades audio-cache residency for effects appearing, while
the owner has open audio rows (FGM 203, BGM garble) — exactly the kind of
cross-domain tradeoff that should not be made unilaterally at the far end of a
session. The arithmetic above makes it a one-constant change plus its
`_Static_assert`, so it is a minutes-long edit once the direction is chosen.

Before flipping it, confirm what a cache miss costs: `gNdsAudioFgmMissRing*`
already tracks misses, so the question "does an over-large cue fail to play or
merely re-read" is answerable from the existing instrument rather than by
listening.

### 6.2 Custom matrix kinds still on the translate-bearing fallback

`0x44`, `0x49`, `0x4A`, `0x51`. `0x44` is `dEFManagerMBallThrownEffectDesc`.
Each needs its own census before a case is added; adding one changes behaviour
for every descriptor using that kind.

### 6.3 Kirby copy-hat low-detail deferral — **now measurable**

The low-detail hat image (7,636 bytes) loads eagerly at copy time for every
Kirby. Deferring it clears the floor outright (23,204 + 7,636 = 30,840), which
would by itself lift a Kirby match back over the 25,600 GObj latch.

The lever was blocked on a count nobody had taken, so r30 adds it.
`gNdsNativeKirbyHatTableHits[2]` increments at the draw-time root match in
`nds_renderer_assets.c` — the renderer asks for a specific root and the root
names the detail, so this is a use, not a load. **`[1]` staying zero across a
natural two-player copy match is the proof the deferral is safe; any non-zero
reading refutes the lever.** Read it beside `gNdsTaskmanGeneralHeapFreeMin`.

---

## 7. The runtime read, 2026-09-21 night

Probe: `TARGET=smash64ds-battle-playable-hwtri BUILD=build-pika-fox-probe
NDS_P2_PIKACHU=1 NDS_P2_PROOF_FIGHTER0=9` — Pikachu in slot 0 against the
canonical Fox in slot 1, the owner's exact pairing. Live match confirmed:
`scene_curr` 24 (VSBattle), 2,043 presented frames, guest alive with 79.5 s of
CPU. Full numbers in `artifacts/performance/2026-09-19_remaining-bugs.md`.

### Face/body — mechanism CONFIRMED live, and the repair is the wrong shape

| witness | lr | row_norm | det_20p12 | stretch_20p12 |
|---|---|---|---|---|
| 0 | +1 | 4907, 4911, 4908 | **+7051** | 4908 |
| 1 | +1 | 4696, 4701, 4695 | **+6180** | 4696 |

A rigid chain reads 4096. These read 4695–4911, so the hardware light is
mis-normalised by **15–20%** — the precondition the hypothesis needed is met,
and the determinant is positive, so it was never a mirror.

**But the repair addresses the minority case.** Of 95 light writes,
`StretchApplied` 2 and `StretchDeclined` **87**. The dominant case is the chain
*shrinking* the light, which the current branch deliberately declines rather
than wrapping a unit-bounded vector. Enabling
`NDS_R2_LIGHT_VECTOR_STRETCH_FIX` would correct ~2% of the error. **Do not ship
it as this row's fix.** The shrink case needs the diffuse *colour* scaled — a
different lever, with the same packet-twin obligation (§4.8).

### Frozen Fox — the Appear-overrun chain is REFUTED

`AppearOverrunFighter` 0, `AnimResolve` 247, `AnimFallback` **0** across 2,043
frames of the reported roster. The Appear path definitely ran (`RaysRequest` is
1, and that request comes from the Appear update), so this is not a status that
was never entered. Steps 1–3 of the reconstruction join step 4 as dead.

What this probe cannot see is the shell's own CSS → VS path, which is where the
owner meets the bug. That is the next place to look, not the figatree.

### P03 rays — they DRAW here, so the row moves to the arena

`RaysRequest` 1, `RaysNull` 0, and a six-stop sample inside one gdb session:

| vblank | candidates | alpha skips |
|---|---|---|
| 83 | 0 | 0 |
| 104 | **20** | **0** |
| 125 | 50 | 20 |
| 187 | 50 | 60 |

Twenty submissions with **zero** alpha skips and zero material rejects before
the skips start, and the candidate count freezes at 50 — which is the source's
own fade (PRIM ramp hits alpha 0 at tick 50, rotation runs to 130). So the rays
are admitted, materially correct, and visible in this configuration.

That refutes the last renderer-side cause. **Every one is now eliminated:**
admission, material state, matrix kind `0x44`, entry-seam ordering, alpha.

What differs is the configuration. This probe runs Pikachu alone with **145,948
bytes free**; the shipping shell measures **7,556 at GO** for Pikachu/Fox with
the `ifCommonSetMaxNumGObj` latch fired, and `efManagerMBallRaysMakeEffect`
takes no EFStruct so the five-free reserve does not protect it. The row belongs
to §1.2 now. `RaysNull > 0` on the owner's r31 run confirms it; `RaysNull == 0`
sends it back here.

Also measured, retiring a standing worry: the Poké Ball costs **596 bytes**.
Its construction is not an arena event.

### Scope limit on both refutations above

The frozen-Fox and rays reads were taken with roughly twenty times the shipping
roster's free arena. They rule out those mechanisms **given memory**; they do
not rule out the same symptoms once memory is gone. That is why the witnesses
ship in r31 — so the shipping configuration can answer it without another lab
build.

### Tonight's reclaim paths are dormant, not wrong

`GradedQuadTextureRecycles` 0, `GradedQuadTextureFails` 0,
`ThunderGroundCoverageReclaim` 0 — nothing in this match exhausted either pool,
and the air-jolt regression did not reproduce in this configuration.
