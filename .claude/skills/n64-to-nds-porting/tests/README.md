# Pack validation

Run from this skill directory with Python 3.10+:

```sh
python3 tests/run_checks.py --report validation.json
```

The runner checks package links/frontmatter, discovers `test_*.py`, runs both CLI fixtures deterministically, and uses available GCC/Clang/G++/Clang++ without installing tools. Missing compiler checks are skips; an executed failure is an error. Temporary binaries are removed.

| Test | Scope |
|---|---|
| `test_helpers.c` | All 65,536 RGBA5551 words; all signed source-s16 values × 28 packing scales; signed rounding, split matrices, unaligned decode, spans and rational tick/debt/overflow. GCC/Clang C11 debug, optimized, `NDEBUG`, UBSan; checks remain active in release. |
| `test_tools.py` | Vertex history, inherited state, patches, calls/branches, unsupported/bounded input; conditional closure/unknown edges, alignment/overflow and independent randomized examples. |
| `test_texture_alpha.py` | Exhaustive RGBA bit repacking; synthetic CI4 index-15 mask (60 opaque/68 transparent); TLUT IA16, IA/I alpha, nibble order, opaque black, reserved capacity, graded quantization, animation remap, RGB upload and restricted draw-contract failures. |
| `test_json_io.py` | Duplicate keys/nonfinite/overflowed numbers, UTF-8 and read bounds, deterministic output, injected fsync/replace failures and both actual CLIs. |

C++17 checks integrate the reusable headers. Optional freestanding Clang generates ARM946E-S and ARM7TDMI objects/assembly in ARM/Thumb mode and checks selected leaf wrappers for unexpected calls. It does not use devkitARM/libnds/Calico, link a ROM, execute ARM code, implement RSP/RDP, validate GPU/cache behavior or measure speed.

[Results](REVIEW_RESULTS.md) record the executed revision. [Manual agent cases](agent-evaluation-cases.md) are proposed prompts, not automated model evaluations. Native visual checks and the consuming project's real build/timing process remain required for rendering/performance claims.
