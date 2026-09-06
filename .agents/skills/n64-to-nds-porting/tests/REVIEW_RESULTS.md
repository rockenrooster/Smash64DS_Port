# Review results — 2026-09-06

## Executed result

**38 runner checks passed; 0 skipped.** The Python semantic-tool stage contains
17 unit tests; the C executable contains exhaustive and randomized checks. These
are separate validation scopes, not 38 hardware or game-equivalence tests.

Reproduce from the skill root:

```sh
python3 tests/run_checks.py --report validation.json
```

The runner uses available host compilers and optional Clang cross-compilation;
missing compilers are reported as skips rather than successes. No tests require
ROMs, game assets, libultra, libnds, or Calico.

## Environment used

- Python 3.13.5 on Linux.
- GCC: `gcc (Debian 14.2.0-19) 14.2.0`.
- Clang: `clang version 17.0.0 (https://github.com/swiftlang/llvm-project.git 10999b6d034fe318f3d56c83bddb6572593a8bb0)`.
- No installed devkitARM or devkitPro SDK was used.

## Checks actually executed

| Scope | Result and boundary |
|---|---|
| Pack integrity | Frontmatter, expected files, and relative Markdown links passed. |
| Python semantic tools | 17 tests passed, including mixed-transform vertex history, inherited child-list state, immutable post-load patches, bounded call/branch expansion, rejected unsupported inputs, and conservative object closure. |
| Independent randomized checks | 100 generated vertex traces compared with an eager translated-position oracle; 100 generated object graphs compared with fixed-point reachability. These oracles cover the examples, not full N64 microcode. |
| CLI integration | Both fixtures ran twice with byte-identical output. Invalid input returned failure and did not overwrite an existing output. |
| C host builds and execution | GCC and Clang passed C11 `-Wall -Wextra -Werror` at `-O0`, `-O2`, `-O2 -DNDEBUG`, and `-O1` with UndefinedBehaviorSanitizer and no recovery. Test checks remain active with `NDEBUG`. |
| C data/numeric coverage | All 65,536 RGBA5551 words; all 65,536 signed source coordinates across 28 accepted scale values (1,835,008 pairs); signed extremes, randomized rounding/shifts, split matrices, unaligned input, rejected spans/segments, and rational tick debt/overflow. |
| C++ headers | G++ and Clang++ compiled the integration translation unit as C++17 with warnings treated as errors. |
| Freestanding target compilation | Clang generated objects and assembly for ARM946E-S and ARM7TDMI, each in ARM and Thumb mode, using `-O2 -ffreestanding -mfloat-abi=soft`. This is not a DS SDK build. |
| Selected target leaf wrappers | Generated assembly checks found no calls/runtime helpers in the selected leaf wrappers, including constant-scale numeric conversion and steady tick accumulation. This does not cover every possible call site or prove speed. |

## Fixture results

The vertex-history fixture emitted three triangles and five immutable vertex
versions, with two mixed-position-transform triangles. Earlier triangle records
kept the values that preceded a later patch.

The conditional live-set fixture retained five objects totaling 3,224 object
bytes, including the late-created projectile. Layout padding was accounted for
separately. This validates graph processing under the supplied metadata, not the
completeness of an actual game's roots or references.

## Documentation and scope review

The existing supplied DS skill was inspected to define the companion boundary.
The new pack leaves target API/hardware contracts with that skill, keeps the
entrypoint compact, and routes tasks to focused chapters. No project-specific
frame gate, reserve, testing authority, or approximation permission is imposed.
Primary source contracts and a pinned libnds comparison baseline are recorded in
[the source map](../references/SOURCES.md).

The host plan compiler is expressly a normalized semantic example, not a raw
GBI decoder, RSP emulator, complete material implementation, or GX exporter. The
live-set analyzer requires externally justified roots and edges; it neither
extracts nor proves them. Numeric rounding policies are explicit, not advertised
as universal source-microcode equivalence.

## Not executed or established

No devkitARM/libnds/Calico integration build, NDS ROM link, source N64 runtime
oracle, DS emulator/device execution, representative game asset conversion, or
performance measurement was performed. Cross-compilation does not establish any
of these. No frame-time improvement is claimed.

[Agent evaluation cases](agent-evaluation-cases.md) are supplied for future manual
behavioral review; they were not run as model evaluations during this validation.
