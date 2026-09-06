# N64 to NDS Porting — companion skill

Version 1.0 · 2026-09-06

A reusable source-porting skill focused on removing N64-specific runtime work,
compiling static structure into native target data, and implementing fast Nintendo
DS systems without silently changing the required game behavior.

## Keep both skills

```text
<your agent's skills directory>/
  nds-coding-practices/       # existing hardware / SDK / DS implementation skill
  n64-to-nds-porting/         # this source-to-target transformation skill
```

Copy the whole `n64-to-nds-porting` folder alongside the existing skill. Do not
replace or merge over `nds-coding-practices`. The pack uses a standard `SKILL.md`
entrypoint with relative references, plus optional `agents/openai.yaml` metadata.
Use the installation mechanism already supported by your agent.

Example invocation:

```text
Use $n64-to-nds-porting with $nds-coding-practices to port this subsystem.
Preserve the required source behavior, compile away immutable N64 work, and
implement the remaining work as a compact DS-native path.
```

## Division of responsibility

| Existing skill | This companion |
|---|---|
| Correct DS hardware, memory, and API usage | Which N64 machinery to eliminate or replace |
| GX, cache/DMA, VRAM, ARM7, runtime, storage | Stateful GBI conversion and source vertex histories |
| Native arithmetic and emitted target code | Source numeric boundaries, scaling, animation semantics |
| DS implementation patterns | Source closures, native packs, gameplay tick/order preservation |

The entrypoint routes to one relevant chapter instead of loading the entire pack.
The optional context template carries project budgets/policies; those are not
baked into the generic skill. No game-specific branch, frame gate, testing
platform, or approximation permission is assumed.

## Included

[SKILL.md](SKILL.md) is the entrypoint. Eleven focused chapters cover migration
choices, C/binary/address domains, RSP geometry, RDP materials, numeric/animation
conversion, residency, gameplay, services/audio, performance, concrete recipes,
and validation. [SOURCES.md](references/SOURCES.md) records primary references
and the reviewed target-source baseline.

Three original portable C headers implement explicit binary/color/matrix
conversion, checked numeric packing, and rational tick accounting. Two original
Python tools demonstrate persistent versioned vertex-cache compilation and
conservative whole-object liveness. They include runnable fixtures and clearly
stated boundaries; neither is advertised as an automatic game-porting tool.

Run the available checks from this directory:

```sh
python3 tests/run_checks.py
```

Python 3.10+ is needed for the tools. GCC/Clang are discovered for host checks;
Clang is used for optional freestanding ARM/Thumb compilation. See
[examples/README.md](examples/README.md), [tests/README.md](tests/README.md), and
[actual review results](tests/REVIEW_RESULTS.md).

## Deliberately not included

No ROMs, game code/assets, proprietary headers, SDK binaries, guessed target APIs,
or project workflow/orchestration framework. No claim that general host tests
prove N64 fidelity, that cross-compilation links an NDS ROM, or that a proposed
optimization has already improved frame time.

The original pack contents are provided under the [MIT license](LICENSE).
Linked third-party materials retain their own terms.
