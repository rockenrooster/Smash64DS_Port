# Manual agent-evaluation cases — not executed by the test runner

Use these to assess whether a coding agent applies the skill, not whether it can
recite the chapter titles. Expected answers should lead to concrete code/design
choices with a proportional verification step.

| Prompt | Required response behavior | Failure |
|---|---|---|
| “Port this static GBI mesh as fast as possible.” | Determine source dialect/entry state; generate native static geometry and live bindings | Permanent generic ARM9 GBI interpretation without justification |
| “A seam appears after loading a second joint matrix.” | Trace each corner's load-time history and partial-cache survival | Clear cache or force all vertices under the latest matrix |
| “All source positions are s16, so pass them through `inttov16`.” | Check range, derive origin/scale/compensation, reject overflow | Mechanical narrowing or wrapping |
| “Convert every float to fixed; gameplay must be bit-identical.” | Preserve required numeric semantics; target proven boundaries; explain exactness limits | Claim a global lossy conversion preserves exact behavior |
| “Tests never spawned this projectile; strip its bytes.” | Require root/schema/late-reference proof or conservative retention | Trace-only liveness certificate |
| “Render at 30 instead of 60 to halve collision work.” | Separate presentation from source simulation; preserve approved tick/event contract | Quietly halve source updates |
| “Cache by texture address.” | Include effective palette/tile/generation dependencies or prove them invariant | Stale material reuse or global cache reset |
| “Use ARM7 as the RSP.” | Identify target service/work ownership and costs; prefer existing supported owners | Unmeasured source task emulation/offload |
| “I precomputed every animation matrix; ROM is unlimited.” | Account active RAM, random access, bandwidth, blends and fractional time | Assume ROM expansion is automatically a speedup |
| “Fallback after the first three triangles failed.” | Preflight and choose a complete path before effects, or error | Replay after partial submission |
| “Does passing these host tests prove the port?” | Separate helper/graph proofs from source fidelity, target linkage/runtime and timing | Claim successful DS execution or measured speedup |
| “Add a simple projectile using existing engine systems.” | Read source behavior and owning code; implement the smallest native feature | Require a new framework, full audit, or large manifest system first |
