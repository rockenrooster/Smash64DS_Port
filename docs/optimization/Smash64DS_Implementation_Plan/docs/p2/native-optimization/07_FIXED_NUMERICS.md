# N04 — Whole-domain fixed numerics

> **Revision 2 coverage:** Universal scope: fixed ranges and no-float closure include every required fighter, stage, legal variant, effect, service and cold scene. The current stress roster is not an adequate numeric range proof. See [16_ALL_ROSTERS_ALL_STAGES.md](16_ALL_ROSTERS_ALL_STAGES.md).

**Endpoint:** fixed/integer arithmetic through producers, state, consumers and target-side services. Moving IEEE operations into integer helper code or renaming `f32` does not satisfy it. The inspected pose clock currently uses integer binary32 arithmetic, and fixed pose values coexist with source float fields. [S13, S14, S27]

## Numeric-domain register to create

Each row must contain: source field/function; semantic unit; current storage/type; all writers/readers; min/max from source plus observed range; unobserved legal cases; selected fixed format; intermediate width; rounding; overflow rule; exact downstream decisions; conversion sites; cold/interrupt/indirect callers; and the task that deletes the legacy storage. Observed min/max alone is not a proof of all source-legal values.

Use separate chains for world motion, affine matrices, camera/projection, collision predicates, damage/knockback, AI query values, particle state, material/texture animation, audio controls and menu transitions. Establish an explicit conversion seam between different **fixed** formats. Do not choose a new format independently in each leaf.

## Kernel requirements

Prefer compile-time constant folding/reciprocals and source-normalized integer fields. Use hardware divide/sqrt only for remaining required operations and respect the existing shared math-unit owner. Avoid a universal generic fixed library that adds type dispatch or saturated arithmetic to every operation. A few typed C/static-inline primitives plus carefully justified ARM kernels are sufficient.

Keep debug overflow assertions outside the release hot path when input bounds prove safety; retain unavoidable runtime guards at actual variable-domain seams. No unchecked 64-bit multiply-of-64-bit intermediates. Inspect ARM/Thumb instruction selection and helper/veneer costs. Exact same-function placement proof is distinct from approximate numeric replacement proof.

### N04.01 — Build the producer-consumer and helper graph

**Depends on:** N00.01, N00.03

**Edit/inspect boundary:** `scripts/census-softfloat-callers.ps1`; `scripts/task37_softfloat_callers.py`; `include/nds/nds_f32_exact.h`; `src/import`; `src/nds`; `src/port`.

**Implementation sequence**

1. Inventory runtime floating arithmetic using types, preprocessed compilation units, compiler lowering and link/caller data. Include arithmetic, compares, conversions, libm, doubles/varargs and custom mantissa/exponent routines.
2. Start at all shipped scene entry points, callbacks, interrupts, service threads and startup/transition roots. Resolve function-pointer and weak-symbol targets conservatively.
3. Map helpers to their callers/domains and exact linked input sections. Include parent bridge work rather than only helper self-time.
4. Publish a per-domain field/use graph and list the smallest producer-to-consumer closure that removes a real chain.

**Required tests/evidence:** T-FLOAT audit fixtures: hidden typedef, constant-only literal, indirect libm call, integer IEEE implementation and float formatting are classified correctly.

**Work or dependency retired:** Unknown float families and leaf-only conversion proposals; no speed saving yet.

**Done:** Every known target arithmetic family and bridge has an owner and migration path.

**Stop/revert:** Unknown indirect targets remain blockers; do not declare zero runtime float from grep or the lack of FPU instructions.

### N04.02 — Freeze units, ranges and numeric ABI

**Depends on:** N04.01, N00.05

**Edit/inspect boundary:** `proposed include/nds/nds_native_numeric.h`; `include/nds/nds_anim_fixed.h`; `include/nds/nds_r2_camera_fixed.h`; `source field-use register`.

**Implementation sequence**

1. For each high-priority chain derive source-legal input bounds and worst intermediate products, sums, shifts and denominators. Add dynamic scene/fighter extremes and rare procedural states.
2. Choose fixed widths/fraction bits from that proof and C2 candidate formats; pin coordinate handedness, world-to-GX scale, angle wrap and matrix layout.
3. Define named rounding/narrowing operations and class E/B comparison policy. Decide which constants/tables are generated host-side.
4. Publish one shared header/contract owned by the integrator. Record each remaining legacy API boundary and prevent independent agents from choosing incompatible representations.
5. Range bounds include all fighters/stages, legal scale/status variants, high-speed projectiles, stage hazards and live attachments. Sampling the current four-kind Dream Land run is not a range proof for universal support.

**Required tests/evidence:** T-NUM extremes/rounding/overflow and compile-time sizeof/alignment checks; test values immediately on both sides of gameplay decision boundaries.

**Work or dependency retired:** Inconsistent numeric conventions and permanent float sandwiches.

**Done:** Chosen native types have explicit range/rounding proofs and agreed producer/consumer semantics.

**Stop/revert:** An unbounded source field requires a new representation or explicit admitted-domain proof, not guessed saturation.

### N04.03 — Implement and qualify the small primitive set

**Depends on:** N04.02

**Edit/inspect boundary:** `include/nds/nds_r2_hwmath_unit.h`; `proposed native numeric primitives`; `existing fixed matrix/vector kernels`.

**Implementation sequence**

1. Implement only required multiply-accumulate, rounded shifts, vector operations, fixed ratio, angle lookup and normalization primitives with documented domains.
2. Use wide intermediates where proved necessary, reciprocal/lookup precompute for invariant operands, and eliminate unnecessary normalization/division before accelerating it.
3. Compile with the pinned ARM9 toolchain; inspect generated ARM/Thumb code, library calls, stack pressure and interworking. Compare a C reference and a target kernel independently.
4. Integrate overflow/corruption diagnostics at contract boundaries; do not add a global runtime saturation framework.

**Required tests/evidence:** T-NUM independently generated arithmetic corpus, exhaustive reduced domains, randomized legal extremes, divide-by-zero negatives and sanitizer/UB checks on the host.

**Work or dependency retired:** Software-float operations and redundant divides inside the selected fixed primitives.

**Done:** Primitives meet documented numeric semantics and target codegen obligations; speed is measured with their real callers.

**Stop/revert:** A primitive faster alone but slower after conversions/64-bit helpers is not adopted into the full chain.

### N04.04 — Close residual camera, vector and transform chains

**Depends on:** N04.03

**Edit/inspect boundary:** `src/import/battleship_gmcamera.c`; `src/port/renderer_adapter_matrix.c`; `include/nds/nds_r2_camera_fixed.h`; `include/nds/nds_r2_hwmath_unit.h`.

**Implementation sequence**

1. Keep the already-fixed camera route; identify remaining source float publication, billboard/look-at, projection, material direction and conversion consumers.
2. Carry fixed values through those consumers and specialize affine versus perspective operations correctly. Avoid full 4×4 work for a proved affine-only chain.
3. Generate invariant trigonometric/reciprocal data where useful without baking live camera or gameplay-dependent state into giant tables.
4. Remove obsolete conversion wrappers only after all consumers of that chain use the native ABI.

**Required tests/evidence:** T-XFORM camera boundary/near-plane/degenerate-up/negative-scale and T-DEPTH regressions; same source-controlled camera trajectories.

**Work or dependency retired:** Residual mixed-representation camera/transform work, not the fixed route that already exists.

**Done:** Converted camera/transform producers and consumers are native end to end with no extra mirror.

**Stop/revert:** A numerical error that changes clipping/attachment/depth behavior must be corrected before acceptance.

### N04.05 — Migrate constants and target asset numerics

**Depends on:** N04.02, N02.03

**Edit/inspect boundary:** `scripts/fighters/generate_nds_native_owners.py`; `existing stage/material/audio generators`; `Makefile`; `proposed numeric asset schema`.

**Implementation sequence**

1. Convert target-consumed float constants and source numeric payloads into the chosen fixed formats in host tooling; preserve original asset provenance and source values for oracle use.
2. Fail generation on overflow, undefined rounding, missing type metadata or an unsupported required asset rather than silently truncating.
3. Update bank hashes/versions and loader validation together. A bank schema change cannot be applied to an old resident image.
4. Check emitted code/data for runtime initializer/conversion routines that should have become constants.

**Required tests/evidence:** T-BANK deterministic generation, version mismatch and numeric boundary fixtures; target link audit finds no runtime constant-conversion initializer in converted banks.

**Work or dependency retired:** Load-time and per-use floating numeric conversion of host-convertible target assets.

**Done:** Fixed constants/data arrive ready for native runtime consumption across converted domains.

**Stop/revert:** Do not edit extracted source or reference decomp to make the converter accept invalid inputs.

### N04.06 — Build a no-float regression gate

**Depends on:** N04.01, N04.03

**Edit/inspect boundary:** `proposed scripts/check-native-runtime-numerics.py`; `Makefile`; `scripts/verify-all.ps1`; `include/nds/nds_f32_exact.h`.

**Implementation sequence**

1. Combine source/type and compiler-output checks with ELF symbol/caller analysis; cover libgcc/libm helpers, custom IEEE code, weak aliases and indirect roots.
2. Initially report by converted domain with an explicit temporary migration allowlist. Each entry names consumer, reason and removal task; new entries require review.
3. At final closure reject all reachable target floating arithmetic in shipped configurations, not merely the profiled path. Keep host generators/oracles outside the target gate.
4. Include independent negative fixtures that introduce one prohibited arithmetic path without using a literal float keyword or a familiar helper name.

**Required tests/evidence:** T-FLOAT hidden typedef/varargs/indirect/custom IEEE/source-include/constant-folding fixtures; build graph proves host references are not linked into ROM.

**Work or dependency retired:** Future reintroduction of float and false success based only on helper-symbol disappearance.

**Done:** The checker distinguishes static data from arithmetic and detects all exercised negative patterns; domain migration is auditable.

**Stop/revert:** A finite trace or blacklist alone is insufficient proof; unresolved roots keep closure open.

