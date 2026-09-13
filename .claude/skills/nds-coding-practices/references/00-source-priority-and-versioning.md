# 00 — Smash64DS backend and SDK map

`PROJECT_GOAL.md` owns supported behavior and performance; `docs/README.md` assigns workflow owners. Do not freeze those policies or current P2 status inside this skill.

| Boundary | Repository entry point |
|---|---|
| Hardware/native rendering | `src/nds`; `nds_renderer.c` identifies its component includes |
| Platform/task/relocation integration | `src/port`; declarations under `include` |
| Build and generated dependencies | `Makefile`, `build.ps1`, `scripts/README.md` |
| Stable design | `docs/ARCHITECTURE.md`, subject to the product contract and current instructions |
| Backend reference implementations | Read-only `decomp/sm64-nds`, `decomp/sm64ds-decomp` |

For API/layout decisions, inspect the installed header, implementation and linker script at the project's actual flags/mode. Match ARM7 services, filesystem stack and SDK generation. Calico-backed devkitPro libnds 2.x is not interchangeable with legacy libnds or BlocksDS. Retain the working runtime rather than migrating it to match this pack.

The pins in [SOURCES](SOURCES.md) are a checked baseline, not today's HEAD. Hardware behavior remains binding when a wrapper omits checks; an old parameter comment cannot override its implementation/register width. [17](17-libnds2-calico-facts.md) supplies concrete version-specific traps.
