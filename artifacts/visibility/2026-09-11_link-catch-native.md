# Link Catch/CatchPull native root-program closure — 2026-09-11

Scope: existing P2-3 Link fighter-production owner. This closes the source-defined
Catch/CatchPull hidden-part topology for the recorded four-CPU natural path. It
does **not** close all Link states or global P2 native-output acceptance.

## Source contract

BattleShip defines Link Catch and CatchPull as one topology family:

- `ftdata.c` gives both rows animation flags `0x1C000000`.
- `224_LinkMainMotion.c` applies the same five model-part writes in both motions:
  joint 21 -> part 0, joint 19 -> hidden, and joints 16/17/18 -> part 0.
- `225_LinkMain.c::dLinkMain_hiddenparts` maps hidden-part IDs 3..5 to source
  joints 35, 17 and 18. Joint 35 is non-drawing; joints 17 and 18 are inserted
  beneath joint 16.
- LinkModel low-detail roots for those two drawing hidden joints are `0x7EA8`
  and `0x7F98` (same source offsets in high detail).

The old generated Catch program handled joint 16 but omitted joints 17/18. It
therefore had no exact 22-root program for the live Catch tree: canonical and
Entry were 19 roots and Catch was 20 roots.

## Pre-fix discriminator

Frozen pre-fix integrated ROM:

- ROM SHA-256: `476E86B2B9B0520922B3B53FAC1B7F0C5C4C5608AC58E057EF692F23436A90E5`
- ELF SHA-256: `AC53F36EEA9AEFC23346E64705CD31C8A42B754C1BD6F08177F3DA9210EC30D2`
- focused artifact SHA-256: `3966F0C225DB9D5592538CAB6F56C0C4025AA726B08E0E5660A7BE114E23D6FD`

At frame 1204, common status `0xA6` (Catch), the live low-detail tree contained
22 selected roots. The validator reported observed 22 versus canonical 19 after
all three available programs failed selection. The live vector included
`0x7DB0`, `0x7EA8`, `0x7F98` consecutively at joints 16/17/18, proving this was
the complete hidden-part topology rather than an isolated root failure.

## Implementation

`scripts/fighters/generate_nds_native_owners.py` now derives the complete Link
Catch program from source data:

- adds program-only resident bakes for `0x7EA8` and `0x7F98`;
- consumes all five source Catch/CatchPull model-part writes;
- reads LinkMain hidden-part IDs 3..5 instead of copying the observed runtime
  root vector;
- emits an exact 22-root Catch program in high and low detail;
- keeps the Catch-only roots unreachable as passive per-binding variants;
- captures each dynamic Catch root from its actual live DObj tree while
  remapping only canonical cross-cache dependencies by display identity;
- proves the two new roots self-contained through the existing vertex-cache
  closure.

The ignored owner IR is reproducible from the generator. On the integrated
candidate it was 5,333,754 bytes, SHA-256
`1D4AA75EB549D94DE260C0988040A5139933BD56F2E72333AE9CDD3F4F02F502`.

Cheap verification:

- `python scripts/fighters/generate_nds_native_owners.py --check` — PASS.
- `python scripts/fighters/check_native_owner_geometry_closure.py` — PASS for
  every owner/detail; every source triangle reaches the emitted primitive stream
  exactly once with source vertex, matrix slot, facing and winding preserved.
- isolated Link-only worktree build from pushed Samus HEAD succeeded:
  ROM `4F80BFE1F94B15D2CF63703F7C10FFBF2D014BD528D8B34651E5A5103D70135C`,
  ELF `4E03D0A3BFE8BBB642D08DF5D96BFA733899FD32B4C0C08D1B8F9C3FCA47C954`.

## Natural-path proof

Authoritative integrated candidate:

- ROM: `builds/build-p2-battle-core/smash64ds-p2-fourcpu-tickhud-hwtri.nds`
- ROM bytes: 30,056,448
- ROM SHA-256: `6BB5A14A560F88891CDE141577821E10E764F65B8B2A776A9221F15E5AA51F2C`
- ELF SHA-256: `AB0437B718966A7C76C0A23B1213E393DBC8B7D93AB413CEE7F48237E876FC2F`

Focused command:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\probe-p2-fourcpu-sparse.ps1 `
  -NoBuild -Target smash64ds-p2-fourcpu-tickhud-hwtri `
  -Build build-p2-battle-core -FirstLinkReject -Frame 1536 `
  -Artifact artifacts\verification\p2-3-link-catch-native-1536.txt `
  -TimeoutSeconds 300
```

Focused artifact SHA-256:
`5FDB4896B0E6851A53CD14650409D39B13F78BD9FFE3948EB7F05DD56618218C`.

Terminal witness:

```text
LINKREJECT_NONE_THROUGH=1536
LINKFINAL=catchProgram2:5,program:0,tried:0,rejects:0,0,160,64
```

The five program-2 selections are positive natural engagement; this is not a
zero/unengaged counter proof. Catch/CatchPull produced no native-program reject
through frame 1536.

## Widest batch verifier

Same ROM, one widest verifier:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\verify-p2-four-fighter-stress.ps1 `
  -NoBuild -Build build-p2-battle-core
```

Fresh artifact hashes:

- tick HUD: `797BFCD577CAAEBCE779F206F2AD598A82DA9FE7D9212DE63175A3467B1F6135`
- memory: `1DF403E0043570F50F9E68276AD6250A93491A59669329F2D82FD589E0E9B25F`
- coverage: `CA4A268992C8CD4E7AA4ABB8DCF57DE2ADBF00E3409BC042503C4A3973CD5D20`

Coverage reached frame 1973, clock 60 -> 1, 59/60 match seconds, presented
delta 1972 and logic delta 3944 with Donkey/Samus/Link/Kirby observed and draw
mask `0xF`.

Resource/lifetime gates stayed green for this candidate:

- general heap free-min 90,796 B; safety floor 25,600 B; margin 65,196 B;
- graphics heap 1,536 B capacity, 232 B peak, overflow/no-room 0/0;
- scene file-buffer declines 0; DObj active max 241;
- effects 38 capacity, free-min 16, active max 22;
- particles 53/112 structs, 15/24 generators, 27/80 transforms, rejects 0;
- AObjEvent32 high-water 1,623, normalize/hash failures 0/0;
- `syMalloc` overflow 0; objman panic 0; texture reject mask `0`.

The first global native-output failure advanced away from Link Catch to the
already-open P2-3f47 Kirby CopyLink family:

```text
identity = 0x00080148  (asset 328 / KirbyModel)
status   = 0x122       (CopyLink)
root     = 0x115C8
material = 0x023BC4B8
reason   = 2           (REJECTED_PROGRAM)
```

An unrelated later Link-special rejection was observed by an earlier broad Link
discriminator at frame 1364/status `0xE5`; it is not Catch/CatchPull and is not
claimed closed here. The widest first-failure latch reaches CopyLink first, so
the next integration outcome remains the existing P2-3f47 CopyLink package.

Periodic production build-health check after this batch:

- root `smash64ds.nds`: 53,117,952 bytes;
- SHA-256: `127E56B9A441CBC1BAEF8AFA46010C6EC61D7262C6F13511305125925A7BC599`.

This plain build is not a substitute for the verifier-covered candidate above.
