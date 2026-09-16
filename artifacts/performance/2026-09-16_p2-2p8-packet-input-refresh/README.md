# P2-2p8 N03.04 packet-hit input refresh

Verdict: **KEEP** the packet-hit input refresh. P2-2p8 remains RED against the
product target.

The 2026-09-15 current exclusive profile charged
`ndsRendererAdapterBuildNativeProductionInputs` 3,727,033 cycles. A successful
fighter packet hit still ran that complete producer even though exact packet
precheck/replay consumes only the live packet subset. The retained adapter now
refreshes that subset before precheck: geometry/color state, root/material shape,
live modelview/projection, GX-composition inputs and replay preamble. A miss still
runs the existing full production-input builder before packet record or direct
native execution. Thus packet replay keeps the same predicate/fail-closed miss
route while avoiding unrelated producer work on successful hits.

The falsifier was any packet precheck/replay disagreement or engagement loss,
native failure/reject, fighter output/state difference, memory regression, or
cadence regression. None fired.

## Verification

Focused four-CPU verification used Boundary's `p2_fourcpu_stress` arm on runner
12 (`builds/verify-p2p8-packet-input-refresh-stress.log`). It completed 1,972
samples and passed correctness, cadence, native-owner and memory gates:

- WORK-H P50/P95: **1,654,720 / 2,377,472** ticks.
- Previous particle-camera checkpoint: **1,660,224 / 2,380,288**; cross-build
  shift **-5,504 / -2,816**. This is a small supporting timing movement, not a
  claim of product-level acceptance.
- Native failures/direct rejects: **0 / 0**.
- General-heap low-water: **108,096 B**.
- Fighter draw-plan build/hit/mismatch: **618 / 6,217 / 0**.
- Four fighter slots drew (`slot triangle mask = 0xF`); renderer-camera reuse
  remained engaged at **5,919**.

The frozen candidate then completed the full Boundary profile on runner 12
(`builds/verify-boundary-p2p8-packet-input-refresh.log`). All three arms passed:

- `p2_shell_loop`: one lap, 10 scene entries, zero faults, native failures zero,
  free-floor **114,628 B**.
- `p2_battle_realtime`: 212 battle frames, published ROM contract passed. The
  known locked-30 warning remains at about **25.9 FPS**, so P2-2p8 stays RED.
- `p2_fourcpu_stress`: reproduced the focused **1,654,720 / 2,377,472** WORK-H
  P50/P95 result with native failures/direct rejects **0/0** and heap low-water
  **108,096 B**.

`Boundary verification profile passed.` No settled camera/pose experiments were
reopened.

## Final hard-on build

After Boundary, `make TARGET=smash64ds` rebuilt the normal natural-input root
target and passed `NATIVE_ONLY_PASS: smash64ds.elf, 262 actual link inputs`.
Final identities are:

- `smash64ds.nds` — 55,712,768 bytes — SHA-256
  `A10E87EF1DFF94C60EB9214CC3AB1F468331E4E55226937AD715276770F5A8C6`
- `smash64ds.elf` — 16,261,900 bytes — SHA-256
  `48303E124FD03E44F35125AB3D4BB604CA24EB93AB9BD099015D40AEECED9C00`
- `builds/build/nds_build_config.h` — SHA-256
  `BDFE59516B2F8DBAB0C7A01C60336772475D03D6FD6A968DCBC9F3CAF298F0E0`

The shipping config has fast logic off, hardware triangles on, profile level 0,
fighter packets on, and the retained particle draw/camera/runtime paths on. The
normal rebuild restored lab-generated particle outputs to their canonical state;
raw verifier/build logs remain under `builds/` and are not committed.
