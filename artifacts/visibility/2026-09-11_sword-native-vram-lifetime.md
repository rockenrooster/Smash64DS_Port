# P2-5i1 Sword native output + VSBattle entry-texture lifetime

Date: 2026-09-11

Scope: closes the measured ITCommonObject Beam Sword native-output blocker in
the four-distinct-CPU stress lifetime. It does **not** close all of P2-5i1.

## Starting blocker

The previous one-minute native-only stress first latched ITCommonObject asset
`86`, root `0x17D8`, reason `NO_PROGRAM`. BattleShip source identifies this as
Sword joint 1 / the blade display list in
`decomp/BattleShip-main/decomp/src/relocData/86_ITCommonObject.c`.

The native Sword owner already admitted both source lists. A live breakpoint at
the old failure showed its admission state complete (`candidate=9`, Sword kind,
correct root, no MObj, relocated blade vertex pointer exact), so adding another
owner/whitelist was not the fix.

## Blade failure: source-untextured run

The source blade list explicitly disables texturing (`G_TEXTURE` off). The
shared wave-1 item emitter nevertheless required
`ndsRendererHardwareBindTexture()` before every run. That rejected a valid
source-untextured blade before any triangles could be emitted.

`src/nds/nds_native_item_wave1_emit.exec.inc` now derives `use_texture` from the
decoded source state, binds only textured runs, begins an untextured GX batch
when appropriate, and emits texture coordinates only for textured runs. The
textured path is otherwise unchanged.

## Hilt failure exposed by the blade fix

Once `0x17D8` drew, the sibling Sword root `0x1850` became visible as the next
failure. It was admitted correctly but failed at texture submit step 3. The
texture reject was `TEXIMAGE`, not a malformed image.

The decisive frame-512 census separated slot pressure from byte pressure:

- 79 dynamic cache slots available.
- 47 dynamic cache entries live at the old failure point.
- all 47 had been touched in that renderer frame; none was stale/evictable.
- those cache entries accounted for only 37,504 texture bytes and were mostly
  small CI4 images.
- the scene's generated entry-effect textures are direct GL residents, outside
  that normal cache and therefore outside its LRU eviction policy.

The failure was therefore texture-VRAM lifetime pressure, not a need to grow the
cache-slot count or steal another DS VRAM bank.

## Source-derived lifetime fix

`scripts/3d_vfx/generate_nds_entry_effects.py` now derives which converted
entry-effect textures are referenced **only** by fighter match-entry roots.
Any texture also referenced by a later gameplay root is retained automatically.
In particular, LinkSpecial2 is split logically so roots `0x02D8`/`0x0698` are
entry presentation while root `0x1100` remains a gameplay Spin effect.

Generated result for the current packet:

- `NDS_ENTRY_EFFECT_STARTUP_ONLY_TEXTURE_COUNT = 41`
- `NDS_ENTRY_EFFECT_STARTUP_ONLY_TEXTURE_BYTES = 18528`

The generated boolean table is consumed by
`ndsRendererHardwareReleaseEntryStartupTextures()`. VSBattle resets its one-shot
latch at scene texture preparation and retires those direct GL names only after
BattleShip transitions to `nSCBattleGameStatusGo`. Shield, reflector, catch,
KO, reflect-break, MBall/item-get, Link Spin and Link boomerang textures remain
resident. The retirement is deliberately VSBattle-local: 1P can introduce new
fighters later in one scene and has a different lifetime contract.

## Host/source checks

All passed on the candidate used below:

- `python scripts/3d_vfx/test_native_entry_mask_words.py`
- `python scripts/3d_vfx/test_native_shield_reflector_packets.py`
- `python scripts/3d_vfx/test_native_catch_swirl_packet.py`
- `python scripts/3d_vfx/test_native_ko_reflect_packets.py`
- `python scripts/3d_vfx/generate_nds_entry_effects.py --check`
- relevant `git diff --check`

## Frozen stress candidate

Built with:

`make TARGET=smash64ds-p2-fourcpu-tickhud-hwtri BUILD=build-p2-battle-core`

- ROM: `builds/build-p2-battle-core/smash64ds-p2-fourcpu-tickhud-hwtri.nds`
  - size: 30,046,208 bytes
  - SHA-256: `1AB9C18CDF324D3D4C23AC0290B576AF07A16F4C7C1778557F8E9BBB612A89F8`
- ELF: `builds/build-p2-battle-core/smash64ds-p2-fourcpu-tickhud-hwtri.elf`
  - size: 15,195,280 bytes
  - SHA-256: `F20F304311466FDEB5F06ED62614EB944A200880A1C32423CA921024DCFECE8D`

## Focused natural Sword proof

Artifact:
`artifacts/verification/p2-5i1-sword-native-512.txt`

SHA-256:
`F8FBDF7D74A6B80FF67CA2B93F99C7B1DFD1EF3F0BB38125E073FB7D52BC54F9`

Key observations at frame 512:

- `SWORDREJECT_NONE_THROUGH=512`
- candidate step `9`
- 596 Sword draws
- 0 Sword submit failures
- submit step `9` (drawn)
- final natural Sword root `0x1850` (hilt)
- texture reject mask `0`
- item texture rejects `0`
- entry lifetime release count `41`
- entry lifetime release bytes `18,528`

This directly covers both the former blade failure and the sibling textured hilt
that became visible once the blade was fixed.

## Widest same-candidate verifier

`scripts/verify-p2-four-fighter-stress.ps1 -NoBuild -Build build-p2-battle-core`
completed on the same ROM.

Coverage:

- start frame 1; timing start frame 2
- end frame 1,973
- clock 60 -> 1
- 59/60 match seconds (0.983333)
- presented delta 1,972
- logic delta 3,944
- observed/expected roster Donkey / Samus / Link / Kirby
- fighter draw mask `0xF`

Resource/continuity checks remain green:

- selected scene arena: 1,392,640 bytes
- general heap free-min: 94,076 bytes
- safety floor: 25,600 bytes
- margin above floor: 68,476 bytes
- graphics heap: 1,536-byte capacity / 232-byte peak / 0 overflow / 0 no-room
- syMalloc overflow: 0
- objman panic: 0
- effect pool: 38 capacity / 16 free-min / 22 active max
- particle maxima: 53 structs / 15 generators / 27 transforms; rejects 0
- compact fighter cores: 4 loads / 92,200 bytes / failure 0
- BattleCore externs: 18 patches / 12 loads / failure 0
- ShieldPose: 4 loads / 11,799 bytes / 36 fixups / no failures
- DamageSlash: 136 source-root draws / 120 triangle draws / 0 submit failures /
  0 texture-prepare failures
- renderer texture reject reason mask: `0`

Verifier hashes:

- `artifacts/verification/p2-2-fourcpu-tickhud.json`
  - `42DBB06053586C132942606E14A0EDFB47DDFE35850CCC43BB64F436B4001755`
- `artifacts/verification/p2-2-fourcpu-memory.json`
  - `FD0396C6A188BBF9629260156B9450E81ABAA6B6ADF0C4614377D1432F8BBAB9`
- `artifacts/verification/p2-2-fourcpu-coverage.json`
  - `12572658DDF1248B2433752683E92D0AD77B0CE0E5130DE97CE5789E1BB2C3B8`

## Next native-only blocker

The same widest run no longer records Sword. Its first native failure is now:

- identity `0x03F3015C`, whose asset low word is **348**
- asset: `348_KirbySpecial2`
- root: `0x27A0`
- reason: `NO_PROGRAM`

BattleShip source maps `0x27A0` to
`dKirbySpecial2_Joint_0x27A0_DisplayList`, the drawable child of
`dKirbySpecial2_CutterDraw`. This belongs to the existing P2-3f47 Kirby package
(Cutter/Stone/unique states), not to P2-5i1. The independently known Kirby
CopyLink mixed-file rejection remains open as a later P2-3f47 state.

## Acceptance statement

For this measured four-distinct-CPU natural lifetime, Sword asset 86 roots
`0x17D8` and `0x1850` are no longer a native-output blocker. P2-5i1 itself
remains open for the rest of its common-item state/child/interaction coverage.
The all-ROM native-output gate remains red at the newly exposed Kirby Cutter
`348:0x27A0` blocker.
