# Validation — project-scoped revision, 2026-09-11

Only guidance and metadata changed; executable examples/tools/tests remain byte-identical to the log-informed pack. [Commands and limits](README.md).

| Rerun | Result |
|---|---|
| Package/link checks | PASS |
| GCC and Clang host suites | PASS: debug, optimized, NDEBUG, UBSan, helper/mock contracts, exact-read snippet, C++17 |
| Actual graphics pixel producers | PASS: eight host configurations |
| Freestanding Clang ARM code generation | PASS; observations, not a speed claim |
| Real-SDK compilation runner | **SKIP, exit 2:** devkitARM/libnds/Calico unavailable |

**Not executed:** SDK compilation, ROM linkage, actual upload/OAM/GX, real DMA/cache/PXI/scheduler behavior, emulator/device runs, timing or manual agent evaluations. No measured improvement to the game or agent throughput is claimed.
