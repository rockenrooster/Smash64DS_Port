# Validation commands and evidence levels

From the pack root:

```sh
python3 tests/run_checks.py
python3 tests/run_checks.py --report checks.json
```

The runner uses Python 3.10+ and standard library modules. It discovers GCC,
Clang, G++, and Clang++ rather than downloading tools. Missing compiler checks
are reported as skipped; a failed executed check exits nonzero. Temporary build
artifacts are kept outside the pack and removed after the run.

## What is exercised

`check_pack.py` checks entrypoint frontmatter, required files, local Markdown
links, and accidental bundled asset types. It does not validate remote links or
certify every natural-language statement.

`test_helpers.c` runs in GCC and Clang debug, optimized, `NDEBUG`, and undefined-
behavior-sanitized configurations. Assertions remain active under `NDEBUG`.
Tests include all 65,536 source color words; all 65,536 signed source vertex
values under all 28 accepted scales; randomized signed rounding; split matrices
with extreme values; unaligned byte decoding; invalid spans; rational tick
accounting; and transactional overflow behavior.

`test_tools.py` checks inherited cache state, mixed transforms, immutable patching,
call/tail-branch behavior, malformed/unsupported input, bounded control flow,
whole-object closure/cycles, late-spawn reachability, unknown-reference rejection,
alignment/overflow, determinism, and randomized closure comparison.

The runner also checks command-line fixtures, byte-identical repeat output, and
that invalid input leaves an existing output unchanged. C++17 compilation checks
that the example headers integrate in that language.

## What ARM checks mean

`codegen.c` exposes selected leaf kernels, not a complete application. When Clang
is installed, compile them freestanding for ARM946E-S and ARM7TDMI in ARM and Thumb
modes, then inspect emitted assembly for unexpected calls/runtime helpers. This
specifically includes a constant-scale packing path and steady tick pulse, not
the setup-time ratio initialization that intentionally uses division.

These checks use Clang's target support and headers, **not an installed Nintendo
DS SDK**. They do not link a ROM, validate libnds/Calico ABI integration, execute
ARM code, prove the target's memory/transfer behavior, or measure performance.
The actual project must compile/link/run its integration with its own toolchain
and accepted testing authority.

[REVIEW_RESULTS.md](REVIEW_RESULTS.md) records what was actually executed for the
distributed revision. [Agent evaluation cases](agent-evaluation-cases.md) are
manual prompt probes for maintaining the skill; they are not executed by the
Python/C suite and must not be reported as automated agent-evaluation results.
