# 15 — Boundary invariants

Select applicable rows, not a new review campaign. Project acceptance and reporting remain in `docs/VERIFYING.md` and `docs/BUG_FIXING_PROCESS.md`.

| Boundary | Technical check |
|---|---|
| API/build | Installed declarations/implementation, target mode, layout and generated dependencies agree. |
| Memory | Bounds, arithmetic overflow, alignment, capacity, VRAM bank roles, full cache-line ownership, stack and peak lifetime. |
| Async | Channel/service owner, bounded queue, publication order, cancellation/generation and true release event. |
| Graphics | Source equation and packed/uploaded mask agree with actual format/palette/material/layer state. |
| Timing | Expected workload executes; measured intervals distinguish CPU work, waits, I/O and idle. |

Alpha fixtures include opaque source index 0/black, nonzero transparent entries, palette animation, graded alpha, overwritten bind flags, depth/IDs and reload. Select those affected by the change; use contrasting backgrounds for visibility. A host mask test does not prove raster output, and a screenshot does not prove release safety or every animation phase.

[Pack tests](../tests/README.md) cover their named helpers/mocks/codegen only. SDK, ROM, actual hardware services and timing require different evidence; unexecuted manual prompts establish none of them.
