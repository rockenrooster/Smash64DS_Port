# Pack validation

Run from this skill directory with Python 3.10+:

```sh
python3 tests/check_pack.py
python3 tests/run_host_tests.py
CC=clang CXX=clang++ python3 tests/run_host_tests.py
python3 tests/run_graphics_checks.py
python3 tests/run_clang_codegen.py
python3 tests/run_target_checks.py --out /path/to/target-checks
```

| Runner | Evidence and limitations |
|---|---|
| `check_pack.py` | Local frontmatter/routes/Markdown links, UTF-8/newlines. Not semantic or remote-source certification. |
| `run_host_tests.py` | C11 debug/optimized/`NDEBUG`/UBSan: fixed arithmetic (10,000 bounded pairs), CPU copy, mailbox layout, mocked DMA/cache calls, PXI value protocol (10,000 values), extracted exact-read snippet; optional C++17 headers. Override `CC`/`CXX`. |
| `run_graphics_checks.py` | Actual example pixel producers compiled/executed against host RAM with GCC/Clang in four configurations. Checks OBJ tile packing, mask/bounds and direct cutout fixture including opaque black. Does not execute SDK upload/OAM/GX calls or teardown. |
| `run_clang_codegen.py` | Freestanding Clang ARM946E-S ARM-mode probes; reports wide multiply, division-helper and halfword-store observations. Uses no installed SDK. Missing Clang exits 2/SKIP. |
| `run_target_checks.py` | Compiles standalone examples (including `gx_cutout.c`), ARM9/ARM7 PXI components and helper probes against installed devkitARM/libnds/Calico. Saves command/assembly/symbol evidence. **Object compilation only**, not ROM linkage or execution. Missing tools/headers exit 2/SKIP. |

For target checks, set `DEVKITPRO`/`DEVKITARM` (optional `LIBNDS`) or use the script's flags. Do not add `tests/mocks` to a target build. Mocks are not a complete SDK, coherency/DMA model or scheduler. Presence of a software helper is not an automatic failure without hot/cold reachability analysis.

Complete native acceptance in the real application: SDK build/link, paired startup/teardown, real IRQ/PXI/cache ownership, native screenshots, scene transitions and the project's accepted timing tests. A host mask check does not certify compositing. [Results](REVIEW_RESULTS.md) state what ran; [agent cases](agent-evaluation-cases.md) are unexecuted manual prompts unless separately recorded.
