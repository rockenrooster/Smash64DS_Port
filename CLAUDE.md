# CLAUDE.md

Context limit: you have unlimited context
Subagents switch = see AGENTS.md
@AGENTS.md
READ AGENTS.md
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

- **You can push to the github repo periodically on confirmed progress
- ** Committing locally is ordinary work;
- The owner's given name must not appear in tracked files. Scan before pushing.
- End commit messages with a `Co-Authored-By` trailer crediting the model that
  actually authored the change — e.g.
  `Co-Authored-By: Claude Opus 5 <noreply@anthropic.com>` or
  `Co-Authored-By: Claude Fable 5 <noreply@anthropic.com>`. The trailer is an
  authorship record, not a fixed string; an orchestrator committing an agent's
  work credits the agent's model.
- `.codegraph/` exists here, so the CodeGraph section of `AGENTS.md` applies:
  reach for `codegraph_explore` before grep or Read.
- **Thirteen project-local skills own Nintendo DS work.** Eleven generic
  `nds-*` skills cover routing, measurement, subsystems, and review;
  `smash64ds-project-context` loads current repository constraints; and
  `n64-to-nds-asset-conversion` owns source-asset conversion. The canonical text is
  `.agents/skills/<name>/SKILL.md` (harness-neutral, with its `references/` and
  `agents/openai.yaml`); `.claude/skills/<name>/SKILL.md` is a one-paragraph
  bridge that points at it. Edit the canonical file, never the bridge, and never
  fork a Claude-specific copy. `nds-port-and-optimize` is the entry
  point when a task spans subsystems or a performance report needs triage.
  `python scripts/validate-smash64ds-skills.py` checks that every canonical
  skill has a matching bridge; nothing runs it automatically, so run it after
  adding or renaming one. Both trees are tracked — `.gitignore` excludes the
  *contents* of `/.agents/` and `/.claude/` precisely so `skills/` can be
  re-included, because a gitignored skill set silently disappears in a fresh
  clone and in every worktree.
- Prefer the Bash tool for POSIX scripts and PowerShell for the `scripts/*.ps1`
  harnesses. Editing a `.ps1` with Python heredocs or `\n` escapes has corrupted
  these files more than once — CRLF plus PowerShell quoting do not survive it.
  Use Read/Edit.
- **Every `Start-Process` in a harness or probe needs `-WindowStyle Hidden`.**
  Without it each launch throws a console window into the owner's foreground and
  steals focus, and a long measurement session launches dozens. The melonDS
  launches already had it; the `gdb` launches did not, which is how this was
  noticed — and a 2026-07-29 AST sweep found nineteen still unhidden, because
  the rule was only ever applied by hand. `scripts/check-melonds-policy.ps1` now
  parses every `.ps1` under `scripts/` — recursively, since the 2026-07-30
  area-folder reorganisation (`scripts/README.md`) — and fails on any unhidden
  `Start-Process`, so do not re-audit this by hand. `-NoNewWindow` is the accepted alternative (it is
  mutually exclusive with `-WindowStyle`); `debug-melonds.ps1` and
  `debug-nogba.ps1` are allowlisted because the owner drives them interactively.
  `make` recipes spawn their own short-lived shells and are not controllable
  this way — run builds through the PowerShell tool rather than
  `Start-Process` so they inherit a hidden console.
- **Builds are parallel by default; never pass `-j` and never clear
  `MAKEFLAGS`.** The Makefile sets `MAKEFLAGS += -j$(NDS_JOBS)` from `nproc`.
  Until 2026-07-29 nothing anywhere set `-j`, so every build in the campaign ran
  single-threaded — about thirteen minutes for a full tickhud rebuild on a
  32-thread machine — and one probe made it worse by exporting `MAKEFLAGS=""`
  before invoking `make`. Do not reintroduce either. One build at a time is
  still correct: the asset generators write into shared paths such as
  `include/nds/generated/`, outside `$(BUILD)`, so concurrent makes with
  different flags corrupt each other regardless of `-j`. `make NDS_JOBS=1` is
  the escape hatch if a generator's prerequisites ever turn out to be
  under-declared — that failure races into a subtly wrong binary rather than an
  error, so it would surface here as an unexplained measurement.
