# Maintainer Checks — Not Mandatory Coding-Agent Context

Requires Python 3.10+ for all scripts. Run from the skill directory. No test is a
substitute for the consuming application's real target build and behavior checks.

## Local checks

```sh
python3 tests/check_pack.py
python3 tests/run_host_tests.py
python3 tests/run_clang_codegen.py
```

`check_pack.py` validates frontmatter, local Markdown links, routes, UTF-8, and
final newlines. It does not recheck upstream claims or test a skill host's UI.

`run_host_tests.py` needs a GCC/Clang-compatible host C compiler with UBSan;
C++17 header checks run when a C++ compiler is available. Override `CC` and `CXX`
as needed. It runs debug, optimized, `NDEBUG`, and UBSan configurations; tests
fixed-point edge cases and 10,000 deterministic bounded pairs; mailbox layout;
zero/invalid DMA requests and cache-call order using a mock; CPU-copy logic;
10,000 deterministic PXI protocol values plus reserved/invalid cases; and the
actual C11 file-read snippet extracted from reference 11. Exported portable
probe functions also compile in each configuration. Temporary binaries are removed.

The mock `nds.h` records only DMA/cache calls. It is not a fake complete SDK,
coherence emulator, DMA model, or scheduler. Never add its directory to an NDS
build. These tests do not execute the real IRQ worker or paired PXI service.

`run_clang_codegen.py` compiles `portable_codegen.c` for ARM946E-S in ARM mode.
It uses Clang's freestanding type headers and a temporary **NDEBUG-only assert
stub**, not installed libnds or devkitARM. It confirms exported probes survive
and reports wide-multiply, division-helper, and halfword-store observations.
Those observations are compiler-specific, not universal pass/fail performance
rules. Missing Clang exits 2 with SKIP. Override `CLANG` with the executable path.

## Real SDK compile/inspection

```sh
python3 tests/run_target_checks.py --out /path/to/nds-skill-target-checks
```

Use the devkitPro environment (`DEVKITPRO`, `DEVKITARM`, optional `LIBNDS`), or
supply `--devkitpro` and `--devkitarm`. The script requires current libnds/Calico
headers and `arm-none-eabi-gcc`, `g++`, `nm`, and `objdump`. Missing tools/headers
exit 2 with SKIP, not PASS. Windows executable suffixes are supported.

It compiles the standalone examples separately, the two PXI components for their
respective CPUs, a C++17 header probe, and explicit runtime-input codegen probes.
Those probes exercise the reusable headers rather than relying on uncalled
static functions disappearing at `-O2`. It saves compiler/command provenance,
disassembly, and unresolved-symbol reports. It does **not** automatically fail
because a cold/reference function uses `__aeabi_ldivmod` or because setup uses
floating point. Review hot/cold reachability and actual generated instructions.

This is **object compilation only**, not a link or run of a demo ROM or any other project. To complete target acceptance, integrate each example into
a compatible supported template, link the final image, run the relevant behavior,
and exercise startup/teardown/resume and any asynchronous boundaries. Do not
claim target compilation from the host mock or generic Clang probe.

## Model evaluation

`agent-evaluation-cases.md` supplies proposed fixtures for comparing model/harness
versions. No model comparison has been run as part of this release. Evaluate
correctness and unnecessary work, not just prompt size or confidence.
