# CLAUDE.md

`AGENTS.md` is the contract for every agent working in this repo, Claude Code
included. It is imported here rather than restated, because `AGENTS.md` line 141
forbids duplicating current truth and a second copy of these rules would drift
from the first the day either is edited.

@AGENTS.md

## Where the rest of the truth lives

Read these before starting, not after being surprised by them:

- `PROJECT_GOAL.md` — the product contract. Mechanical equivalence is required;
  bit-exactness is not. Carries the performance gate and the sacrifice order
  (audio fidelity, then visual fidelity, then 60 Hz simulation, then gameplay
  fidelity) that decides what may be traded for frame rate.
- `docs/P1_EXECUTION_BOARD.md` — the only dynamic queue. `docs/HANDOFF.md` — the
  restart surface, and nothing else.
- `docs/optimization/TASK_STANDING_RULES.md` — how a performance task is run,
  measured, and judged. Read it before proposing or gating an optimization; it
  is where the campaign records what previous tasks learned the hard way.
- `docs/VERIFYING.md` — which verifier covers which runtime, and why stacking
  them is wasted time.

## Claude Code specifics

These are about this tool, so they are not in `AGENTS.md`:

- **Never push to the public GitHub remote without the owner's explicit
  authorization for that specific push.** Committing locally is ordinary work;
  publishing is not, and one approval does not carry to the next push.
- The owner's given name must not appear in tracked files. Scan before pushing.
- End commit messages with
  `Co-Authored-By: Claude Opus 5 <noreply@anthropic.com>`.
- `.codegraph/` exists here, so the CodeGraph section of `AGENTS.md` applies:
  reach for `codegraph_explore` before grep or Read.
- Prefer the Bash tool for POSIX scripts and PowerShell for the `scripts/*.ps1`
  harnesses. Editing a `.ps1` with Python heredocs or `\n` escapes has corrupted
  these files more than once — CRLF plus PowerShell quoting do not survive it.
  Use Read/Edit.
