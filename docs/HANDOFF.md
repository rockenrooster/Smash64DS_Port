# Handoff

Current: 2026-09-02 — **Pikachu and Yoshi landed opt-in (`NDS_P2_PIKACHU`, `NDS_P2_YOSHI`); Link integrated runtime acceptance OPEN; work is on master in the main tree.**

The `p2-pikachu` worktree is merged (`b9bd5f7d84f`) and removed. Pikachu and
Yoshi have source gameplay, articles, native owners (slots 7/8), audio banks
(FGM pack 293 entries), HUD/CSS surfaces and gdb-driven specials tours; detail
in `docs/p2/fighters/{pikachu,yoshi}.md`, closed rows in the archive.
Open: P2-3f46 (the Yoshi/Fox/Captain/Samus stress arm halts before its first
sample; `NDS_P2_FOUR_CPU_KIND0..3` knobs landed) and P2-3f33 (Link runtime
acceptance). Link's Boomerang/Spin data seams are green (see P2-3f33 evidence).
Last pushed: `3d8ab3710fe`. Four-CPU optimization remains owner-parked.
Owner workflow (2026-09-02): no new worktrees; implement a whole phase before
running verifiers.

## Next

1. Roster close as one slice: Ness, Jigglypuff, Kirby end to end (inventory,
   owner, gameplay, audio, shell), compile checks only in between.
2. Then one verification pass: both-CPU smokes, CSS capture, Boundary, the
   stress arm on the re-argmaxed roster (P2-3f46).
3. Link integrated route (entry, Neutral-B, Up-B) acceptance rides in that pass.
4. On a clean slice, update the board/evidence, rebuild `smash64ds.nds`, commit/push.

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
