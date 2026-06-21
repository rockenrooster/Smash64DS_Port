# Smash64DS parallel renderer acceleration pack

This pack is meant to be copied into the repository as **parallel reference work**, not blindly wired into the active verified runtime.

It targets the renderer bottleneck after the current verified state:

- original Startup scene renders the N64Logo sprite through a bounded path
- original Opening Room reaches the selected DObj/material/display-list preview
- `src/nds/nds_renderer.c` already parses a bounded branch-expanded display list
- full draw, full `lbcommon.c`, fighters, stages, audio, and real Title are still deferred

The pack gives Codex reusable design notes, small C modules, and ready-to-paste tasks for the next renderer milestones.

## Contents

- `docs/WORKFLOW_ACCELERATION.md` — recommended workflow changes and bigger-but-safe slices
- `docs/RENDERER_SLICES.md` — renderer subsystem breakdown and milestone order
- `docs/SUBSYSTEM_IMPL_NOTES.md` — implementation notes for the user’s requested renderer areas
- `prompts/codex_renderer_backend_task.txt` — next focused Codex task
- `prompts/codex_texture_material_task.txt` — later texture/material task
- `include/nds/*.h`, `src/nds/*.c`, and `code_snippets/*` — standalone helper modules Codex can adapt into `src/nds`
- `scripts/gbi_trace_summary.py` — simple log summarizer for renderer verifier output

## Integration rule

Do not drop all code into the build at once.

Recommended order:

1. Read docs.
2. Compare helper APIs against current `include/nds/nds_renderer.h` and `src/nds/nds_renderer.c`.
3. Copy one helper module at a time into `include/nds` / `src/nds` only when the active milestone needs it.
4. Add verifier markers for every new rendered primitive/texture/material claim.
5. Stop when the one selected Opening Room renderer slice improves.

