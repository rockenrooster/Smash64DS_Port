# Validation — project-scoped revision, 2026-09-11

Only guidance and metadata changed from the log-informed pack; executable examples/tools/tests remain byte-identical. [Commands and limits](README.md).

`tests/run_checks.py`: **38 runner checks PASS, 0 SKIP**. Its Python stage contains **44 unit tests**. Coverage includes package links, geometry/live-set/alpha/JSON helpers, deterministic/failed-output CLI behavior, GCC/Clang C11 debug/optimized/NDEBUG/UBSan, C++17 integration, and freestanding ARM946E-S/ARM7TDMI ARM/Thumb code generation.

**Not executed:** installed devkitARM/libnds/Calico compilation, ROM link, original-game source execution, actual game-asset conversion, upload/raster, emulator/device runs or performance. Manual agent prompts were not run against a model. These checks do not prove the game correct or the agent faster.
