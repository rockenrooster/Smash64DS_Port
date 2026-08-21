# P2-1 — VS Shell (Intro → VS Battle → loop)

Turns the boot-into-match demo into a game: full menu flow with only Mario/Fox
and Dream Land selectable. Everything later plugs into the seams built here.

**Implementation status (2026-08-19): COMPLETE.** The owner explicitly
deferred final verification to a later pass. All P2-1 implementation work is
in-tree; the unchecked closeout items at the bottom are verification/owner
acceptance only, not remaining implementation work.

## Scope

Boot → title screen (the original title presentation: logo art, copyright
line, music, PRESS START) → main menu → VS mode → character select → stage
select → load → battle → results → back to character select, indefinitely.
1P GAME / OPTIONS / DATA entries present but greyed until their phases land.
Intro cinematic is **deferred to P2-7** (owner, 2026-08-17); until it lands,
boot goes straight to title — the N64 game has no separate splash screen, so
none is invented here.

**Fidelity ruling (owner, 2026-08-18): this is a port — ALL original
presentation assets ship, including first-party branding, logos, title/boot
screens, menu artwork, and copyright text, converted from source like every
other asset. Never substitute invented "identity-safe" placeholders.** The
visual rework ran through P2-1h…P2-1N and the final decomp audit. The resulting
shell uses the source CSS/SSS art, buttons/panels/cursors, title fire + label
animation + translucent Smash emblem, source READY/BACK art, interlocking
shutters, and live source fighter GObjs on CSS. The final audit also corrected
Team/FFA state preservation and READY semantics, source gate startup (all gates
start shut and occupied slots open), title blend/layer ordering, and the
60 Hz source-animation / 30 Hz DS-GX split for CSS fighter previews.

## Work breakdown

1. [x] **Match-config seam.** Parameterize the hardcoded mode-163 configuration
   into a match descriptor: `fighters[4]` (character id, human/CPU, CPU level,
   team, costume/color), stage id, mode (Time/Stock), time limit, stock count,
   item flags (stub off). Battle consumes only the descriptor. Sized for 4
   slots now even though battle accepts 2 until P2-2.
2. [x] **Scene manager.** Generalize the existing battle→results→sudden-death
   flow into scene transitions covering menu scenes; wipe/fade transitions;
   memory discipline: each scene entry resets its arena — audited so N loops
   leak nothing (heap low-water flat across loop iterations).
3. [x] **2D UI kit.** Font/text renderer matching SSB64 menu identity, cursor
   sprites, menu SFX (move/confirm/back), portrait/icon asset conversion for
   Mario and Fox, shared layout helpers. This kit is also the groundwork for
   the bottom-screen battle HUD (P2-2) — build it dual-screen aware.
4. [x] **Title + main menu + VS menu.** Rules screen (Time/Stock, minutes/stocks),
   greyed stubs for unbuilt modes.
5. [x] **Character select.** SSB64 CSS: hand cursor, token drop, CPU toggle +
   level, live Team/FFA toggle, READY flow, source shutters, and live Mario/Fox
   3D previews. 12-slot layout with 10 slots visibly locked/empty until fighters
   land. Per-slot RED/BLUE/GREEN selectors and four-fighter team play remain
   P2-2.
6. [x] **Stage select.** SSB64 SSS layout, Dream Land selectable, others shown
   locked; random maps to Dream Land.
7. [x] **Loop verifier.** Scripted-input walk of the full loop (menus → match →
   results → menus), N iterations, asserting no leak (heap watermarks), no
   hang, menu cadence, and battle Boundary equivalence. Becomes the Boundary
   definition at phase close.

## Reference

- Menus: `decomp/BattleShip-main/decomp/src/mn/` — `mncommon`, `mnvsmode`,
  `mnplayers` (CSS), `mnmaps` (SSS), plus `sc/` scene sequencing.
- Match config: how BattleShip's global game state (`gm/`) carries VS settings
  into battle — mirror the *meaning*, not the structure.
- DS menu architecture: how `sm64ds-decomp` structures menu scenes and 2D
  layers.

## Non-goals

Four-fighter team gameplay and per-slot RED/BLUE/GREEN selector UI (P2-2),
items UI (P2-5), options/data screens (P2-7), unlock gating (P2-7), touch input
(bottom screen stays non-interactive).

## Risks

- 2D/3D VRAM arbitration between menu scenes and battle — audit VRAM bank
  ownership per scene before building screens.
- Scene-loop leaks: P1 never tore a match down into a *different* scene; the
  START-restart path reuses state. Teardown correctness is the phase's real
  engineering content.
- Menu fidelity rabbit hole — approximate per the visual doctrine, timeboxed;
  owner is the visual oracle.

## Implementation closeout

- [x] Match descriptor is the only battle input; mode 163 is a preset.
- [x] Title → menus → CSS → SSS → battle → results → CSS is implemented.
- [x] CSS/SSS/title presentation implementation is source-derived through the
      latest owner findings and decomp audit.
- [x] Live CSS fighter behavior follows the source state machine; DS-only GX
      presentation runs at 30 Hz while source fighter state remains 60 Hz.
- [x] Team/FFA mode state is preserved through the descriptor and source READY
      same-team rejection is implemented; P2-2 owns the remaining team UI/play.
- [x] Boundary membership already includes the shell loop plus the realtime
      battle-through-shell regression arm.

## Verification closeout — automated gate green; owner visual pass pending

- [x] Phase-close loop passes the owner-amended one-lap Boundary requirement
      with flat per-kind high-waters, a 54,256 B arena free floor, and zero
      faults. The older 20-lap soak remains historical evidence, not a standing
      phase-close requirement.
- [x] Menu cadence remains within the previously accepted per-screen cadence
      evidence; the phase-close shell arm is intentionally a fast-logic scene
      soak and is not relabeled as a cadence measurement.
- [x] Battle entered through the menus is Boundary-green and mechanically
      identical to the retained two-fighter regression contract.
- [ ] Owner visual pass on the shell screens; screenshots in
      `artifacts/visibility`.
- [x] Post-audit three-arm Boundary verification passes. Evidence:
      `artifacts/verification/2026-08-21_p2-2-boundary-closeout-final.log` and
      `artifacts/verification/2026-08-21_p2-shell-loop.txt`.
