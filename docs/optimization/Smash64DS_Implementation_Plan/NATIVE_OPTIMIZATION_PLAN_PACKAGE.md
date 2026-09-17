# Smash64DS implementation plan — revision 2

Start at `docs/p2/native-optimization/README.md`, then `00_MASTER.md`.
The universal configuration specification is `16_ALL_ROSTERS_ALL_STAGES.md`.

Revision 2 makes every legal four-fighter lineup on every selectable VS stage a
binding support and qualification requirement, including duplicates and legal
slot assignments. There are 81 task cards across 11 existing packages; six cards
are new. P2_EXECUTION_BOARD.md remains the only live queue.

Original research baseline: 75f7f6b4b4864c82c01872d0fd2771d171005272.
Repository rechecked: e67e5871ba8c4ae972f4826bfeb89757d3686401.
No new game source edits, ROM builds, game tests or benchmarks are claimed.
Nothing in this package was pushed to GitHub.

## First installation versus upgrade

On a clean review branch with no v1 campaign files, run `git apply --check INSTALL_ADDITIVE.patch`, then apply that patch. For an EXACT original-v1
installation, use `git apply --check UPGRADE_FROM_V1.patch` and then that patch
instead. Never apply both. Preserve hand-edited or active files; on any check
failure review and merge rather than overwrite. See CHANGES_v2.md.

The patches install planning documents, templates and host planning tools only.
They do not edit existing game source, P2 board, reference trees or generated assets.
The original research already present under docs/optimization remains historical
supporting material; add a board pointer on adoption without creating a new queue.

## Static package checks

```text
python planning-tools/validate_native_plan.py
python planning-tools/test_validate_native_plan.py
python planning-tools/coverage_counts.py --fighters 12 --stages 9
```

These are plan/schema/count checks, NOT runtime coverage or performance evidence.
The production source catalogue adapter, scenario runner, resource solver and
strict runtime-evidence reconciler remain planned implementation tasks.

The combined Markdown reader is generated from the split sources. Edit split
source documents and regenerate all task/graph/combined views together.
