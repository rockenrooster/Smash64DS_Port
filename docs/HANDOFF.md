# Handoff

Current: 2026-09-03 — **P2-5 items: all twenty common items and five of the
thirteen Poké Ball Pokémon are in the ROM and exercised on a running ROM.**
All eight VS stages boot and play. `smash64ds.nds` rebuilds green at default
flags; the board owns per-slice state.

Landed today: the eight-stage sweep closed (P2-4h1 — four defects of one shape,
a hardcoded stage list standing in for "does this stage have valid data");
nineteen common items plus the Poké Ball; the monster bus and five Pokémon;
P2-4n1 step 2, the native stage descriptor threaded through the runtime with
Dream Land output byte-identical (`sha256=eda2dbd6…`).

**Boundary 2026-09-03:** `p2_shell_loop` PASS, `p2_battle_realtime` PASS.
`-Profile Boundary -List` is the membership authority; P2-2p8 is parked.

## Next

1. **P2-5: the remaining eight Pokémon.** Iwark, Lizardon, Spear, Kamex,
   MLucky, Starmie, Sawamura, Pippi. Each is a verbatim TU under
   `src/it/itmonster/`; follow `battleship_item_nyars.c`. Register the maker in
   `sNdsITManagerProcMakeList` or the kind is unreachable however it is rolled.
   Land in two batches of four and measure the arena between — see below.
2. **P2-5: the VS Options and Item Switch screens.** The state already exists
   (`NdsMatchConfig.item_toggles` / `item_appearance_rate`); what is missing is
   the two screens. Source: `mn/mnvsmode/mnvsoptions.c` and `mnvsitemswitch.c`;
   reloc file `llMNVSItemSwitchFileID` 0x8, sprite offsets
   `reloc_data.us.h:2295-2333`. Sixteen options: index 0 is the rate, 1..15 the
   toggles in `dMNVSItemSwitchTogglesItemKinds`. Commit rule
   (`mnVSItemSwitchSetItemToggles`): all-off writes 0, otherwise Green Shell
   also sets Red Shell and the four containers are forced on. The VS screen's
   OPTIONS row is the entry point and currently refuses
   (`nds_menu_shell_mode_vs.c:612`). Art needs a screen block in
   `scripts/menus/generate_mn_ui_kit.py`.
3. **P2-4n1 steps 3-5** — checker parameterisation, then a second stage packet.
   Runtime accessors and what is still Dream Land-hardcoded: commit
   `aa1ba3949b1`.
4. **P2-3f47**: ten-flag both-CPU smokes, CSS capture, Boundary, stress arm.

**The arena, measured.** It is a newlib calloc that steps down in 4 KiB pages,
so binary size costs it in page granules; spawned items barely touch the peak
(38,944 B free floor items off, 38,168 on). Item TUs cost ~870 B each, so the
last eight Pokémon are about two pages against ~1.5 pages of headroom over the
32,768 B P2-1 reserve. Reclaim 4 KiB rather than lower the reserve — that is
an owner call.

Congo Jungle and Sector Z music is rendered but **held**: both loop starts sit
near the track midpoint (50.1% and 53.6%), the signature of a doubled decode.

Owner decision owed: `lbRelocGetForceExternHeapFile` hands back a raw heap
pointer on a miss instead of failing closed. Counted now
(`gNdsRelocForceFighterAnimFallbackCount`), so visible rather than silent.

## Traps that cost a build each today

- A macro glob in a comment (`ITNYARS_*/ITMONSTER_*`) contains `*/` and closes
  it, swallowing the extern block below. Checked now by the item checker.
- A port header named after a decomp header replaces it for decomp TUs too,
  because `include` precedes the decomp root; a narrow subset needs its own
  name under `include/nds/`.
- `battleship_efmanager.c` `#include`s the whole of decomp `ef/efmanager.c`,
  so porting a function into it is a redefinition. Check before adding.
- Building while a subagent holds a compiled file gets you its half-finished
  edit. Check `git status` before a build.

## Delegation

OpenCode is the active skill (owner, 2026-09-03: "opencode-agent is back").
`opencode run --agent swarm-build|swarm-probe --variant Xhigh --auto`;
permissions are tool-enforced, so prompts carry scope, not rules. Build agents
write only their own new files and REPORT the Makefile and header deltas —
those are shared and the orchestrator's to apply. Make each build agent
syntax-check its own files before reporting; four of five batches this session
arrived with an error a one-line compile would have caught. Long `Xhigh` runs
can stall silently with zero output while a trivial call still succeeds --
but check `Get-Process opencode` CPU time first: a backgrounded run piped
through `Select-Object` writes nothing until it FINISHES, which reads exactly
like a stall and cost three healthy agents this session.

## Context discipline

Restart reads this file + `docs/P2_EXECUTION_BOARD.md` only. CodeGraph first,
then bounded reads of the returned seams. `PORTING.md`, `PERF_LEDGER.md`,
`VERIFYING.md`, `KNOWN_ISSUES.md` and the fighter/stage plans are lookup-only.
Bank verbose output to files and bring back status plus the failure window.
One build at a time; never pass `-j` or override `MAKEFLAGS`.
Owner directives: **no snapshot**; no new worktrees.

Start of cycle: `scripts/verify-all.ps1 -Profile Boundary -List`, `git status --short`.
