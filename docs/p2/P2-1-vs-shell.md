# P2-1 — VS Shell (Intro → VS Battle → loop)

Turns the boot-into-match demo into a game: full menu flow with only Mario/Fox
and Dream Land selectable. Everything later plugs into the seams built here.

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
backlog from this ruling is board row P2-1h, **landed 2026-08-18**: the
original title presentation, the menu collage on both screens that draw it,
the DS system-menu banner, and the deletion of the interim splash so boot
reaches the title with no screen in between. The title's fire animation and
label slide-in remain deliberately absent (sacrificable per
`PROJECT_GOAL.md`); the title's music is the opening cinematic's and arrives
with P2-7, which is why a silent title is source-correct.

## Work breakdown

1. **Match-config seam.** Parameterize the hardcoded mode-163 configuration
   into a match descriptor: `fighters[4]` (character id, human/CPU, CPU level,
   team, costume/color), stage id, mode (Time/Stock), time limit, stock count,
   item flags (stub off). Battle consumes only the descriptor. Sized for 4
   slots now even though battle accepts 2 until P2-2.
2. **Scene manager.** Generalize the existing battle→results→sudden-death
   flow into scene transitions covering menu scenes; wipe/fade transitions;
   memory discipline: each scene entry resets its arena — audited so N loops
   leak nothing (heap low-water flat across loop iterations).
3. **2D UI kit.** Font/text renderer matching SSB64 menu identity, cursor
   sprites, menu SFX (move/confirm/back), portrait/icon asset conversion for
   Mario and Fox, shared layout helpers. This kit is also the groundwork for
   the bottom-screen battle HUD (P2-2) — build it dual-screen aware.
4. **Title + main menu + VS menu.** Rules screen (Time/Stock, minutes/stocks),
   greyed stubs for unbuilt modes.
5. **Character select.** SSB64 CSS: hand cursor, token drop, CPU toggle +
   level, team/FFA toggle stub (activates in P2-2), READY→GO. 12-slot layout
   with 10 slots visibly locked/empty until fighters land.
6. **Stage select.** SSB64 SSS layout, Dream Land selectable, others shown
   locked; random maps to Dream Land.
7. **Loop verifier.** Scripted-input walk of the full loop (menus → match →
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

Teams rules enforcement (P2-2), items UI (P2-5), options/data screens (P2-7),
unlock gating (P2-7), touch input (bottom screen stays non-interactive).

## Risks

- 2D/3D VRAM arbitration between menu scenes and battle — audit VRAM bank
  ownership per scene before building screens.
- Scene-loop leaks: P1 never tore a match down into a *different* scene; the
  START-restart path reuses state. Teardown correctness is the phase's real
  engineering content.
- Menu fidelity rabbit hole — approximate per the visual doctrine, timeboxed;
  owner is the visual oracle.

## Exit criteria

- [ ] Full loop runs indefinitely under the scripted walk (≥20 loops, heap
      watermarks flat, zero exceptions).
- [ ] Every menu screen holds its cadence (60 Hz screens ≤ ~560K ticks).
- [ ] Battle entered through the menus is Boundary-green and mechanically
      identical to P1 (same verifier arm).
- [ ] Match descriptor is the only battle input (mode 163 becomes one preset).
- [ ] Owner visual pass on the shell screens; screenshots in
      `artifacts/visibility`.
- [ ] Boundary definition upgraded to the loop verifier; board rows closed.
