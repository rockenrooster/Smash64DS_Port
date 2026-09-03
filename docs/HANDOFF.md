# Handoff

Current: 2026-09-03 — **P2-4 is open: Yoshi's Island has its gameplay half
behind `NDS_P2_STAGE_YOSTER`. Jigglypuff still aborts (P2-3f50).**

Pushed today through `b762e4654b9`. `make` with no overrides builds
`smash64ds.nds` green at 24,941,568 bytes, SHA-256
`0B9D25C80C0A1213FB0C16BD5E0CE8CE7B17E25DF7BBA63BF7E859D2B9F53AD9`.

Landed: the per-owner dense-normal tables are baked by the image generator and
shipped inside the NitroFS owner images instead of being built at load into
static bss, which removes **21,928 bytes** of main RAM from a ten-flag ROM
(verified by symbol: only the Mario/Fox pair survives, and its high table is
DTCM). Yoshi's Island went in the same shape as Dream Land — a wrapper that
includes source `gryoster.c` verbatim, an admission arm in the shared common
setup, its reloc payload, its ground data through the same loader, and its
stage-select slot — all behind a default-off flag.

Sized today, not guesses: the ten-flag static growth of 259,387 bytes is only
about 98,096 bytes of relocatable data, so the arena gap does not close on
data paging alone; and P2-3f48 costs **3,392 bytes, not 82,976**, because its
only dependency, MiscData086, is already staged and rowed by Yoshi.

**Delegation note (2026-09-02):** the opencode paid route
(`opencode-go/muse-spark-1.3-contributor`) hit its monthly cap and agents then
stall **silently** — live processes, zero output, no error. The tell is
`~/.local/share/opencode/log/opencode.log`. The free twins
(`opencode/<name>-free`) work; `swarm-*` agent files also need `mode: all`,
not `mode: subagent`, or `opencode run --agent` silently uses the wrong model.

Open: **P2-3f50** (Purin aborts in `lbCommonSetupFighterPartsDObjs` before
frame 1 — `extern_count == 0` for asset 233 at the external-fixup guard, and
no corpus file lists 233 as a dependency, so the zero comes from an earlier
registration that saw an empty header count), **P2-3f49** (arena), **P2-3f48**
(ITCommonData, now cheap), **P2-4s1** (Yoster presentation: native stage
packet, particle banks, stage-select art, music), P2-3f46 and P2-3f33.
Four-CPU optimization remains owner-parked. Owner workflow: no new worktrees;
implement a whole phase before running verifiers.

## Next

1. P2-3f50 needs a runtime answer, not more reading. Static analysis has taken
   it as far as it goes: the fixup pass bails at `extern_count == 0` for asset
   233, that field is written only in `ndsRelocRegisterLoadedFileImpl`, the
   branches there that zero it also record a failure, and the failure count was
   1 — so some earlier registration saw `extern_file_ids_num == 0` in the
   header it was handed. Build a Purin lab with a probe that records *which*
   caller registered 233 and what header it saw, and read it.
2. P2-3f49: land the remaining data levers and re-measure. Sized: Mario/Fox
   owner tables ~64,147 B, `sNdsP2FighterAnimTokens` 10,384 B (deletable
   outright — the O(1) range path at `reloc_backend_assets.c:2242-2311`
   already covers it), prepared-dense to slot-owned scratch 65,784 B. Measure
   the arena on a *battle* target, not `smash64ds`: the shell ROM needs human
   input to reach a match, so `probe-arena-overflow.ps1` just times out on it.
3. P2-3f48 is cheap now (3,392 B): two rows in `sNdsRelocAssets`, one NitroFS
   entry, the load inside the port's own `itManagerInitItems`, then drop the
   two Kirby stubs and restore the Master Ball entry article.
4. P2-4s1 presentation: native stage packet (law 8), particle banks,
   stage-select art, music. Then Kirby smoke, CSS capture, Boundary, P2-3f46,
   and the Link integrated route.

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
