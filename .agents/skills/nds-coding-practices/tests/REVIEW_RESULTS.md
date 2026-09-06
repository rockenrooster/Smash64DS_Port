# Executed Checks — Final Revision, 2026-09-06

This records checks actually run in the release environment. It supersedes the
validation note from the earlier revision on the same date.

## Environment

- Host GCC / G++: Debian 14.2.0-19.
- Host and illustrative ARM compiler: Clang / Clang++ 17.0.0.
- No installed devkitARM compiler or libnds/Calico SDK headers.
- No NDS linking, emulator/device execution, or performance capture.

## Results

| Check | Result and scope |
|---|---|
| GCC host helper suite | PASS in debug, optimized, `NDEBUG`, and UBSan configurations. |
| Clang host helper suite | PASS in the same four configurations. |
| Fixed math | Edge/rounding/range cases plus 10,000 deterministic bounded input pairs per configuration. Not an exhaustive arithmetic proof. |
| DMA/cache helper contracts | Zero no-op, invalid channel/unit/alignment/count, busy-channel rejection, and flush/invalidate ordering passed against a recording mock. No real DMA/cache is modeled. |
| CPU upload helper | Copy/no-op/invalid arguments and destination bounds sentinel passed against host RAM. Not a video-memory hardware test. |
| Shared mailbox layout | Size, alignment, and producer/consumer cache-line offsets checked. No cross-CPU publication test. |
| PXI value protocol | Reserved/invalid commands, stop reply, boundaries, and 10,000 deterministic valid echo values passed per configuration. No transport or scheduler is modeled. |
| Actual file-read snippet | Extracted C11 snippet compiled and passed exact/short/zero-length read checks under both host compilers. |
| C++17 headers | Reusable headers compiled with both host C++ compilers. |
| Nonconstant portable probes | Exported helper call sites compiled in all host configurations. |
| Illustrative ARM9 codegen | Clang ARM946E-S ARM-mode compile succeeded; expected exported probes survived. Wide multiply, `__aeabi_ldivmod`, and halfword stores were observed in assembly. |
| Python scripts | Bytecode compilation passed. |
| Local package structure | Frontmatter, UTF-8/newlines, root reference routes, and internal Markdown links passed. |
| Real-SDK target-check script | **SKIP, exit 2**: required toolchain and SDK headers not present. No successful target compile is claimed. |

## Commands executed

```sh
python tests/check_pack.py
python tests/run_host_tests.py
CC=clang CXX=clang++ python tests/run_host_tests.py
python tests/run_clang_codegen.py
python tests/run_target_checks.py
python -m py_compile tests/*.py
```

`run_target_checks.py` listed the missing ARM compiler/tools and current SDK
headers, then exited 2. The generic Clang probe used its freestanding headers
and a temporary NDEBUG-only `assert.h` stub. It did not use a Nintendo DS SDK.
Assembly observations describe this compiler invocation, not measured costs or
universal compiler output. The division helper is expected for the portable
reference division; its presence is not declared a failure.

## Source review versus execution

The relevant upstream declarations/implementations support the runtime guide
and new example composition. In particular, the PXI mailbox adapter was inspected
and its unchecked full-queue behavior was documented. Source review does not
establish that the newly composed target programs execute correctly.

**Not verified:** a devkitARM/libnds target build, final NDS image linkage, paired
ARM9/ARM7 startup or shutdown, IRQ worker scheduling, real PXI delivery, DMA/cache
behavior, video output, hardware memory placement, emulator/device timing, or a
performance improvement. The proposed model-evaluation fixtures were not run.
Use the consuming application's target build and acceptance process for those
claims. This pack is the final editorial/code revision, not hardware certification.
