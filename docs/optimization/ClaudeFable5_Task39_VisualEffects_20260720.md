# TASK 39 — Visual-effects audit + faithful DS implementations

**STATE (2026-07-21): stopped pre-Phase-A, map corrected, ready to resume.**
The preflight (artifacts/performance/2026-07-21_task39-visual-effects-preflight.md)
correctly fired this task's first stop rule: the original structural map claimed
shield had no DS-specific code, but shield is actually a lane-3 substitute (see
corrected map below). The map is now fixed. Resume order: Phase A census →
Phase B contact sheet → Tyler marks it → Phase C.

**Standing rules apply in full: read `docs/optimization/TASK_STANDING_RULES.md`
first.** This task is melonDS-sufficient class (visual fidelity + CPU-add work);
device confirmation folds into the next batched retail checkpoint.

**SEQUENCING:** Phases A/B (census + captures) are read-mostly and may run anytime
in their own session. Phase C touches the renderer material path (shield, hurt
flash) which overlaps Task 36's territory — do not run Phase C concurrently with
the active Task 36 session (one writer); start it only after 36 lands or is
checkpointed. Independent of Tasks 37/38 (38 is the AUDIO twin of this task; same
complaint, disjoint files).

## Reported symptoms (Tyler, 2026-07-20)

Hurt/hit/shield visuals PLAY, but some look wrong / different from the original.
Audit that ALL visual effects are used/played/correct, then implement the missing
or incorrect ones in the most DS-hardware-efficient way.

## Planner's structural audit (verified 2026-07-20 — start from this map)

The effect system runs in three lanes of very different fidelity:

1. **Model-effect lane — original and live.** `ef/efmanager.c` + `efdisplay.c`
   imported and graduated (src/import/battleship_efmanager.c; PORTING.md ~:17865),
   EFCommonEffects1/2/3 heap-resident (94,944 bytes), Fox blaster/reflector run
   original effect code.
2. **Particle lane — shimmed/skipped.** `ef/efparticle.c` is NOT imported. The
   lbParticle/efParticle API is satisfied by weak stubs and DS shims:
   src/port/battle_playable_compat_stubs.c:236-249 (`gEFParticleStructsGObj` /
   `gEFParticleGeneratorsGObj` weak, `efParticleGObjSetSkipAll`) and
   src/port/reloc_backend_compat_shims.c (lbParticleMake* shims). In the original,
   most HIT/HURT visuals (damage sparks, slashes, dust, sparkles) are
   particle-driven — this lane is the prime suspect for symptom "hit visuals look
   different".
3. **Generic-substitute lane.** Several efManager one-shots are weak-stubbed to the
   simplified `ndsEFManagerMakeVisualEffect`: battle_playable_compat_stubs.c:117
   (DeadExplode→nNDSVisualEffectDeath), :126 (SparkleWhiteDead→Sparkle), :134
   (RebirthHalo→Rebirth), :211/:217/:223 (StockSnap/StealStart/StealEnd), :229
   (BattleScore). **SHIELD IS ALSO THIS LANE** (correction 2026-07-21, from the
   preflight): `efManagerShieldMakeEffect` is weak-routed to
   `ndsEFManagerMakeVisualEffect(nNDSVisualEffectShield, ...)` at
   src/port/reloc_backend_compat_shims.c:1619-1631; the import wrapper renames the
   original symbol (src/import/battleship_efmanager.c:149,184), the DS template
   table builds `nNDSVisualTemplateShield` (~:212, ~:385, selected :421-423), and
   the instance attaches to the fighter Y-rotation joint (:674-676). The original
   comparison source is decomp/BattleShip-main/decomp/src/ef/efmanager.c:4095-4148.

The hurt color-flash still has no identified DS-specific handler — treat "generic
DObj/material path, prim-color/translucency approximation, possibly dropped by
NDS_RENDERER_BATTLE_STATIC_TEXTURE_DEFAULT=1 baking" as a HYPOTHESIS to confirm or
refute in Phase A (the shield claim of this kind was wrong; sweep src/port shims
and src/import wrappers, not just src/nds, before asserting absence).

Ground truth for everything: `decomp/BattleShip-main/decomp/src/ef/` (efmanager.c/h,
efparticle.c/h, efdisplay.c, efground.c) — read-only reference.

## Phase A — inventory + runtime census (mechanical, no visuals yet)

1. Build the complete effect entry-point table: every `efManager*MakeEffect` /
   `lbParticleMake*` / `efParticle*` symbol declared in decomp `ef/efmanager.h` and
   `efparticle.h`, classified: (a) original imported, (b) weak-stubbed to
   nNDSVisualEffect*, (c) particle-shimmed, (d) no-op/skipped. Cite file:line per
   row. Include the shield draw path and the fighter hurt color-anim path (find
   where the original applies the damage flash and what the DS renderer currently
   does with it).
2. Add a runtime census (profile-1, RAM counters): per-entry-point spawn counter +
   a skipped/substituted counter. One scripted natural match that provably covers:
   weak+strong hits, shield on/hit/break, KO + blast-zone explosion, star KO,
   rebirth halo, jump/land dust, run-turn dust. Report: called N times /
   rendered-original / rendered-substitute / skipped.
3. Deliverable: a markdown checklist table — the "is everything being played"
   answer with numbers, not impressions.

## Phase B — visual triage (Tyler is the oracle)

1. For every entry that fired, capture melonDS screenshots at the effect moment
   (scripted scenarios). Assemble ONE contact-sheet doc: effect name → screenshot →
   Phase A classification → decomp citation of what the original does (descriptor,
   texture, prim colors, frame count — readable from efmanager.c/efparticle.c).
2. Tyler marks each row OK / wrong / missing / don't-care. Do NOT start Phase C
   before the marked sheet returns. Pre-populate his known complaints: hurt flash,
   hit sparks, shield look.

## Phase C — implement, most-DS-efficient-first

Fix order: hit sparks + hurt flash + shield first, then KO/stock/score
substitutes, then the rest of the marked sheet.

Efficiency ladder — take the highest rung that passes Tyler's eye:

1. **2D hardware (OAM sprites, ~free):** billboard-type effects (hit sparks,
   slashes, sparkles, dust) are screen-facing quads in the original. Render as OAM
   sprites on the 3D screen's 2D layer: project effect position to screen once per
   frame per live effect, animate by OAM tile index from a VRAM sprite sheet
   converted from the ORIGINAL EFCommon textures (already resident — reuse their
   frames, draw no new art). Zero GX cost, hardware composition. Report OAM-entry
   and OBJ-VRAM budgets before/after (NDS_IFCOMMON_HYBRID_OAM machinery exists);
   2D-layer effects cannot be occluded by stage geometry — acceptable for sparks,
   anything needing occlusion drops to rung 2.
2. **3D quads with original textures (cheap):** camera-facing textured quads via
   the existing renderer, poly-alpha translucency, prim color from the original
   descriptor. Costs GX words — keep per-effect budgets small and counted.
3. **Import original TU (last resort):** `efparticle.c` runs the real particle sim
   on the update path — CPU + arena cost. Only if Tyler rejects rung 1/2 fidelity
   for a specific effect class; behind its own flag with a 25R-window cost A/B.

Specific items:

- **Hurt color-flash:** material color modulation at draw time (per-part
  diffuse/prim override while the flash is active — a few GX writes per fighter,
  no texture rebaking). Flash timing/color comes from the original fighter
  color-anim state which gameplay already computes — read it, never reimplement it.
- **Shield:** this is a SUBSTITUTE-TEMPLATE fidelity fix, not a material-path fix
  (see corrected map). Diff `nNDSVisualTemplateShield` + its adapter against the
  original `efManagerShieldMakeEffect` (decomp efmanager.c:4095-4148): geometry/
  shape, translucency (poly_alpha vs original combiner alpha), color source,
  size-from-shield-health scaling, and joint attachment. Fix the template/adapter
  in place; one fidelity A/B pair for Tyler.
- **Generic-substitute upgrades (death explosion, rebirth halo, stock/score):**
  upgrade `ndsEFManagerMakeVisualEffect` toward the original descriptors via rung
  1/2 — correct EFCommon textures, frame counts, colors from the descriptor tables
  cited in Phase A.

## Task-specific constraints (beyond standing rules)

- **Return-value semantics are gameplay-visible.** Callers may branch on returned
  GObj*/LBParticle* pointers. When upgrading a stub, preserve its current return
  behavior exactly unless you cite the caller proving otherwise. Never alter
  imported gameplay TUs to "help" an effect.
- **Budget guard:** add a typed effects tick counter (spawn+update+draw) to the
  profile-1 HUD; gate Phase C on a 25R-window A/B showing typical-frame effects
  cost ≤ ~20K melonDS ticks and no new P95 spike. Taskman arena size unchanged
  after boot (assert + HUD row — the affine-OOM lesson).
- Flags: one per lane — NDS_TASK39_FX_SPRITES / NDS_TASK39_FX_FLASH /
  NDS_TASK39_FX_SHIELD.

## Stop rules

- Phase A census contradicts the structural map above (e.g., a "wrong" effect is
  actually rendering original code) → report before coding; the fix may belong in
  the renderer material path instead.
- OAM/VRAM budget can't fit the sprite sheet → report the budget math, fall to
  rung 2 for the overflow; never evict existing OAM users.
- Any gameplay verifier goes red → stop, bisect, report; effects work must never
  move gameplay.
