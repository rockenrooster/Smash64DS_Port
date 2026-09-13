# 09 — Symptom to native fix boundary

| Problem | First useful action |
|---|---|
| Opaque texture/sprite quad | Walk [03](03-rdp-materials-textures.md): effective alpha, packed output, upload, material/global state. Correct bytes move suspicion to draw state. |
| Wrong colors after palette change | Verify TLUT mode/bank, palette + remap generation, tint and bind cache; don't key by image pointer alone. |
| Missing joint triangles/seams | Replay load-time vertex histories, partial reloads and edits; mixed transforms cannot use one current matrix. See [02](02-rsp-to-ds-geometry.md). |
| Large geometry overflows `v16` | Derive local rebasing/scaling and compensating transforms; checked packing, not narrowing. |
| Repeated float animation traversal | Compact channels and share the correct tick/space pose; preserve blends/events/procedural inputs. |
| Actor closure too large | Typed complete live roots, shared immutable union, per-instance state and peak overlap; no speculative byte deletion. |
| Selection UI stalls with more actors | Separate preview representation; sequential-friendly bounded loads; stale-request generations. |
| Native command cache costs too much | Count copy/patch/flush/wait traffic; emit small live bindings around reusable static data. |
| Linked-list update is hot | Preserve source traversal/RNG order while binding stable callbacks and separating hot data. |
| Target game speed differs | Establish source tick/region and pulse contract; don't conflate VBlank, animation and simulation. |
| Source queue/task seems redundant | Preserve deferred order and completion using the existing target owner. |
| Material has no exact simple DS mapping | Derive equations and live inputs; dedicate a validated native recipe or fail explicitly. |

These are technical entry points, not closure rules. Source-required geometry, alpha and events cannot be replaced with silent success; project acceptance belongs to its owning documents.
