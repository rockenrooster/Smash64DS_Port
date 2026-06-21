# Emulator Strategy

Use the emulator that answers the current question with the least ambiguity.

## Default Choice

- Use melonDS for automated pass/fail verification.
- Use no$gba for deep DS hardware and renderer debugging.
- Use both when renderer behavior is suspicious or emulator-specific.

## Task Routing

| Task | Primary Tool | Reason |
| --- | --- | --- |
| Boot chain, scene flow, relocation, controller injection, task/object diagnostics | melonDS | The scripts attach to the ARM9 GDB stub and read exact globals from `smash64ds.elf`. |
| Runtime regression verification before handoff | melonDS | `verify-runtime.ps1` and `verify-opening-skip.ps1` are deterministic enough for pass/fail checks. |
| Visual HUD capture for docs/handoff | melonDS | `capture-melonds.ps1` captures the normal visible debug HUD path. |
| DS renderer hardware state, VRAM, OAM, palettes, BG/3D registers, DMA/timing questions | no$gba | The debugger build exposes hardware inspection UI that is better suited to low-level DS rendering issues. |
| Suspected emulator-specific rendering bug | both | Cross-check no$gba and melonDS before changing ROM code. |
| Final hardware confidence | real DS hardware | Emulator agreement is useful but not a hardware guarantee. |

## no$gba Automation Boundary

The local no$gba package is the debugger/fullversion build. Its window layout
is configuration-dependent. The current local setting opens one combined debug
window:

- `No$gba Debugger (Fullversion)`

Other settings can expose a separate emulator window as well:

- `No$gba Emulator`

Both layouts are valid. The automated no$gba scripts enumerate all visible
top-level windows owned by the no$gba process and can capture whichever windows
are present.

Current automated no$gba support:

```powershell
.\scripts\debug-nogba.ps1 -Build
.\scripts\capture-nogba.ps1 -Build -AllWindows
.\scripts\verify-nogba-smoke.ps1 -Build
```

`verify-nogba-smoke.ps1` proves that no$gba launches the ROM and exposes at
least one capturable debugger/emulator window. It does not yet prove runtime
globals, VRAM contents, register values, or renderer correctness.

## When To Prefer no$gba

Choose no$gba for a renderer task when the next question is about DS hardware
state rather than original BattleShip control flow. Examples:

- Did a texture upload land in the expected VRAM bank?
- Are palette entries correct after conversion?
- Are BG/OAM/3D engine registers configured as expected?
- Is a DMA/timing issue causing blank or partial frames?
- Is the DS GPU rejecting a polygon/texture mode that melonDS tolerates?

Choose melonDS for the same task when the next question is about original code
progress:

- Did the original display callback run?
- Which GBI command was parsed or skipped?
- Did relocation resolve a pointer or symbol?
- Did an imported scene request the next scene?
- Did controller injection hit the original callback?

## Future no$gba Automation

Do not replace the melonDS verifier until no$gba has a machine-readable channel
for the specific evidence we need. Useful future work:

1. Add image-diff baselines for no$gba captures once visuals are stable.
2. Investigate whether no$gba debugger state can be exported or controlled
   without fragile UI automation.
3. Add targeted ROM-side hardware diagnostics that render encoded register/VRAM
   summaries into the captured frame.
4. Keep no$gba scripts optional for normal build verification unless a milestone
   explicitly touches DS renderer hardware behavior.
