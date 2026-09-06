# Nintendo DS Coding Practices — Final Revision

Revision: **2026-09-06**. This package supersedes the earlier revision from the
same date. It is a general-purpose DS coding skill, not a project policy pack.

Install by extracting and replacing the **whole `nds-coding-practices/` folder**
in your coding agent's skill location. Do not replace only `SKILL.md`; it routes
to the included references and examples. The skill directory has the same name
as before. `agents/openai.yaml` is optional host-specific metadata; other hosts
can ignore it.

The entry point is [`SKILL.md`](SKILL.md). Its task routing loads details on
demand. The default goal is efficient, correct first-pass code with minimal
necessary process. API generation is selected from the consuming project.

Highlights and scope: [`CHANGELOG.md`](CHANGELOG.md).
Source provenance: [`references/SOURCES.md`](references/SOURCES.md).
Actual validation and limitations: [`tests/REVIEW_RESULTS.md`](tests/REVIEW_RESULTS.md).
Test instructions: [`tests/README.md`](tests/README.md).

Host logic checks and illustrative ARM codegen were run for this release.
A complete devkitARM/libnds build, device/emulator run, and measured performance
improvement are **not** claimed. The target-check script reports missing SDKs
explicitly, rather than counting them as passed tests.
