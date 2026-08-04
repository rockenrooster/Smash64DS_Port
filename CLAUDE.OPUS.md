# CLAUDE.OPUS.md — Opus session overlay

This file is for Opus sessions in this repository. It adds constraints; it
replaces nothing. Read `CLAUDE.md` and `AGENTS.md` first — they own the truth.
`AGENTS.OPUS.md` owns your operating loop; this file owns your rails.

Written from a 65-checkpoint campaign orchestrating Opus agents here. Every
rule below exists because an Opus agent broke it at least once.

## The five behavioral rails

1. **Budget honesty.** Before starting any chain (edit → build → run → verify),
   say whether you can finish it. If not, do the largest verifiable subset and
   stop clean: done / committed / inherited. A finished subset beats an
   unverified whole. Never commit an unverified allocator, renderer, or
   provisioning change because the cycle ended.
2. **Measure before touching.** One discriminating read outranks any theory.
   Walk the existence chain (created → alive → sized → in-camera → admitted →
   submitted) before claiming a root cause. Do not fix what you have not
   measured broken — in one cycle, three of four suspected defects were
   measurement artifacts.
3. **Plausible is not measured.** Every number carries its source and window.
   A grep that finds nothing is the weakest evidence available (`decomp/` is
   gitignored — use `--no-ignore`). A validation read through the pointer
   under test is worthless by construction. Controls must be able to fail;
   prove the control differs. Retract with the same energy you claim — a
   retraction is a first-class result.
4. **Scope is the assigned step.** No drive-by refactors, no doc rewrites
   beyond the row you own, no "while I'm here." A found issue gets one line in
   the owning doc, not a detour. Never compress a re-proof to make it fit.
5. **You never conclude for the owner.** Visual rows ask; the owner's eye is
   the acceptance gate. Fidelity tradeoffs, default flips, and sacrifice-order
   calls are reported as `BLOCKED(decision: ...)` — never chosen.

## Mechanical rails (each earned the hard way)

- **Builds:** never pass `-j`, never touch `MAKEFLAGS`, one build at a time.
  Published target names (`smash64ds-battle-playable-hwtri`, `smash64ds`)
  hardcode output to the project root whatever `BUILD=` says — lab work uses
  `smash64ds-battle-playable-proof-hwtri`. `make <target-name>` without
  `TARGET=` silently falls back to `TARGET=smash64ds`. Hash the two root ROMs
  before and after any cycle that builds; state they are unchanged (or state
  the new baseline if publishing was the assignment).
- **Commits:** explicit file paths only — the owner's dirty tree is theirs and
  is never swept in. Name-scan every committed blob (case-insensitive owner
  name patterns; must return nothing). Trailer per `CLAUDE.md`. No push, no
  snapshot — both belong to the orchestrator or owner.
- **Tree:** `decomp/` is read-only; its tracked edits are patches under
  `scripts/decomp-patches/battleship/` (verify with
  `fetch-battleship-reference.ps1 -VerifyOnly`, never by grep).
- **Probes:** never call the guest allocator from gdb (it hangs the target).
  Stack locals and stack objects lie through this stub — globals and
  pointer-derefs only; validate a struct pointer by comparing one field to a
  register. Resolve breakpoints via `nm`, not debug info (`NDS_WEAK` twins
  give `Breakpoint at 0x0` then a silent timeout); accept `W` symbols.
  addr2line names deleted and inlined functions. A gdb `condition` evaluates
  on the host — conditioning a hot site stops the core on every hit.
- **Measurement:** read artifacts, never filtered console output
  (`Select-Object -First` and `Where-Object` have both eaten load-bearing
  lines). Capture tics come from the effect's own source lifetime, never a
  fixed offset from its spawn. A cross-build pixel number without its
  same-build adjacent-present floor is uninterpretable; crop to the changed
  geometry; two arms are only the same fight at the same FPS. A 40-frame P95
  is the 3rd-worst frame of 40 — state the window or the number is not real.
- **Scripts:** every `Start-Process` carries `-WindowStyle Hidden`
  (`check-melonds-policy.ps1` enforces). Edit `.ps1` only with Read/Edit —
  heredocs and `\n` escapes corrupt CRLF. A probe whose last command is an
  unbounded `continue` exits by timeout: write artifacts before the wait and
  size the timeout for the match, not the hit cap.

## The meta-rule

A documented lesson gets broken twice; a structural one cannot be. When you
catch a trap, fix the helper so the wrong form is inexpressible (the mi-async
default, the quoting wrapper, the throwing `printf` guard were all this), and
only then write it down.
