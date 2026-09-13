# Manual agent cases — not executed tests

| Fixture | Acceptance |
|---|---|
| Cutout uploaded as `GL_RGB` | Detect forced bit 15; preserve alpha with the correct upload, then check state/output. |
| Correct alpha bytes, solid GX quad | Inspect decal/modulation and submitted state, not a proven decoder. |
| Two queued meshes request independent alpha thresholds | Recognize global raster registers; derive compatible native policy. |
| OAM references graphics being freed | Hide and commit before display-safe release. |
| Producer flushes before changing its cached publication sequence | Correct payload/marker visibility and acknowledgement order. |
| Main thread spins, starving its lower-priority worker | Use runtime-appropriate blocking/yield; no IRQ blocking. |
| 8 KiB loader slice still misses frame/tail budget | Separate read/fixup/publication costs and retries; bytes are not time. |
| Stable semantic slot 11 with eight enabled owners | Validate the actual ID domain/provider table, not a compacted enabled count. |
| Failed layout pin followed by proposed numeric bump | Measure producer/ELF and check semantics plus region headroom independently. |

These are unexecuted agent prompts. Host tests do not establish scheduler, GPU, application or model behavior.
