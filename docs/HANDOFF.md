# Handoff

Current: 2026-09-02 — **the whole roster is admitted opt-in; Ness smokes green, Jigglypuff aborts (P2-3f50), Kirby is unmeasured.**

Ness, Jigglypuff (`NDS_P2_PURIN`) and Kirby landed in one generated slice
(`264760a19bc`), produced by the new `scripts/fighters/admit_fighter.py` and
`derive_native_owner_tables.py`: manifest rows, reloc/effect/entry/HUD/CSS
seams, native owners (slots 9/10/11), status promotion and three audio banks
(FGM pack 293 -> 408 entries, two lifetime-bounded loop prefixes, four 16 kHz
snores, ROM ceiling 3 -> 6 MiB). `1fa52c906f9` then fixed animation path
resolution for the whole roster and moved the shared item subsystem off Link's
flag. Both are pushed; `smash64ds.nds` rebuilds clean at 24,931,328 bytes.

**Read before measuring anything:** the working tree carries another agent's
uncommitted P2-3f47 work from 15:14-15:25 -- owner-image-size arms for the
five new owners in `nds_renderer_assets.c`, and Ness "admission witness"
diagnostic globals in `renderer_adapter_fighter.c` whose comment says it is
chasing an unassigned admission failure. Every lab ROM built after 15:25
compiles it in. Coordinate before attributing a fighter measurement, and
isolate on a clean checkout when the attribution matters.

Open: **P2-3f50** (Purin aborts in `lbCommonSetupFighterPartsDObjs` before
frame 1; arena and asset counters are clean), **P2-3f49** (the ten-flag ROM
exhausts a 1,347,584-byte arena during battle setup, which gates Kirby's smoke
because Copy links against all ten neutral-Bs), **P2-3f48** (ITCommonData
residency), P2-3f46 (four-CPU stress arm) and P2-3f33 (Link acceptance).
Last pushed: `1fa52c906f9`. Four-CPU optimization remains owner-parked.
Owner workflow (2026-09-02): no new worktrees; implement a whole phase before
running verifiers.

## Next

1. Isolate P2-3f50: rebuild `build-purin-cpu` from a clean checkout of
   `1fa52c906f9` and re-run `scripts/probe-battle-progress.ps1`. If the abort
   survives, chase the PurinMain/PurinModel pointer fixup that feeds the
   common-parts DObjDesc array.
2. P2-3f49: dump `sNdsRelocLoadedFiles` at the ten-flag halt for a real
   residency census, then reclaim arena or shrink the working set. Kirby's
   smoke unblocks with it.
3. Then the rest of the verification pass: Kirby smoke, CSS capture, Boundary,
   the stress arm on the re-argmaxed roster (P2-3f46).
4. Link integrated route (entry, Neutral-B, Up-B) acceptance rides in that pass.

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
