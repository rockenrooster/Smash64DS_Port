---
name: n64-to-nds-porting
description: Port, review, and optimize Nintendo 64 source or decompilation code for the original Nintendo DS. Use for libultra/GBI/RSP/RDP migration, display-list compilation, persistent vertex caches and mixed-matrix geometry, N64 binary/relocation conversion, native DS render plans and materials, fixed-point boundaries, animation and collision specialization, scene residency, source tick preservation, and replacing N64 OS/audio tasks. Prioritize removing source-machine work and generating compact DS-native data. Pair with nds-coding-practices for DS hardware, SDK APIs, cache/DMA, GX, ARM7, and code generation.
---

Canonical text: `../../../.agents/skills/n64-to-nds-porting/SKILL.md`.
Read and follow it. It owns what to translate from N64 source, what to
compile away on the host, and which N64 semantics must survive on DS.
Pair it with the `nds-coding-practices` skill, which owns DS hardware,
SDK APIs, cache/DMA, GX, ARM7, and code generation.
