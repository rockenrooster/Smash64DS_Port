# Handoff

Current: 2026-09-02 — **Link weapon data/native seams GREEN; integrated runtime acceptance OPEN.**

BattleShip Boomerang and Spin Attack state machines remain the behavior owner.
LinkMain@0x0C and LinkSpecial1@0 WPAttributes receive the required DS s16-lane repair.
The byte oracle pins both source map boxes; weapon submit admits source TREE/TREE_DLLINKS while exact asset/offset admission stays fail-closed.
The native bake includes Boomerang and Spin effect/weapon roots; Link weapon checker, attribute oracle, Link-enabled build, and GBI/decomp checks are green.
Natural Link entry/Neutral-B/Up-B gameplay and visual acceptance remain open.
The implementation slice is still uncommitted; `git status --short` is the dirty-path authority.
Last pushed checkpoint: `f331fe85c56`. Four-CPU optimization remains owner-parked.

## Next

1. Run the integrated Link route through entry, Neutral-B, and Up-B.
2. Verify natural wave/beam, Boomerang, and Spin creation/draw/eject behavior and visuals.
3. Fix only reproduced owning-seam defects; keep BattleShip behavior authoritative.
4. On a clean acceptance slice, update the board/PORTING evidence, rebuild `smash64ds.nds`, then commit/push.

## Active gates

Boundary membership is authoritative from `scripts/verify-all.ps1 -Profile Boundary -List`:

- `p2_shell_loop` — scene-boundary shell lap; fast-logic timing is not cadence evidence.
- `p2_battle_realtime` — mode 163, shell-driven Mario vs level-3 Fox, Dream Land, one-minute Time, items off.
- `p2_fourcpu_stress` — four distinct level-3 CPUs; memory/native-path regression arm, no 1.12M tick assertion.

The canonical published-ROM SHA-256 line is owned by `docs/P2_EXECUTION_BOARD.md`;
historical docs may mention old/current build hashes as chronology.
P1 stays frozen; nothing routine rebuilds its published artifact.
Current owner directive: **no snapshot** unless explicitly re-enabled.

## Token-efficient context discipline

- Normal restart reads **this file + `docs/P2_EXECUTION_BOARD.md` only** after the two start-of-cycle commands.
- Use CodeGraph first for code ownership/call paths; then bounded reads of only the returned DS and BattleShip seams.
- `PORTING.md`, `PERF_LEDGER.md`, `VERIFYING.md`, `KNOWN_ISSUES.md`, archived docs, and fighter/stage plans are **lookup-only** during ordinary work. Search first; never full-read them unless the task is explicitly a whole-document audit.
- Bank verbose build/verifier/profile output to files. Bring back exit status, summary/failure lines, and only the small surrounding failure window.
- While editing, use the smallest checker that answers the question. Run one widest relevant verifier only for a kept checkpoint.
- Prefer one substantial owning-seam slice over repeated tiny build/test cycles. One build at a time; never pass `-j` or override `MAKEFLAGS`.
- Keep evidence in its owner: queue/current result → board; measurements → `PERF_LEDGER.md`; chronology → `PORTING.md`; durable gaps → `KNOWN_ISSUES.md`; procedure → `VERIFYING.md`.

## Start of cycle

```powershell
.\scripts\verify-all.ps1 -Profile Boundary -List
git status --short
```

Then read this file and `docs/P2_EXECUTION_BOARD.md`; take the highest-impact unowned red/current row.
