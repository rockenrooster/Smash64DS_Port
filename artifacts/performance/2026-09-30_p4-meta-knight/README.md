# Meta Knight isolated experiment — September 30, 2026

Owner request: a brand new Meta Knight selection, its complete CSS path, and a
playable native DS match. Branch `codex/meta-knight` starts at
`2e093297c5cabff53deeefd89d499aebfdf9f728`; workspace is
`D:/Stuff/DevFolder/Smash64DS_Port/.worktrees/meta-knight`.

The main checkout has active owner edits and is read-only to this experiment.
No main ROM, build directory, runner, generated file or live cursor is changed.
There were three existing directories under `.worktrees/`; this is the fourth.
Retain the experiment through October 7, 2026 unless the owner extends it.

Implementation checkpoint `92bddf1b3e47b3d5a086b645fd4fa8f76c455949` is committed
and pushed to `origin/codex/meta-knight` as `IMPLEMENTED_NOT_ACCEPTED`. It preserves
122 scoped source/test/evidence files. Main/decomp/AGENTS/CLAUDE are unchanged;
no lab binaries, raw logs or runner configs were committed. Build five must use
this source identity; later root documentation updates do not change its code.

## Current identity and evidence

- P4 plans and shared contracts read. They describe an unimplemented source
  adapter, not an existing Meta Knight port.
- Main reference Remix HEAD: `5e04fe7fcd023cd43c71f25f89bb6e810d254d55`.
- Main reference EXTRA HEAD: `96621afea26a83305abaf81add07dcf5a9c5fe3e`.
- EXTRA's nested Remix is uninitialized. Disposable staging must provide the
  exact same Remix revision without writing to read-only `decomp/`.
- Meta Knight sources, motions, model binary, UI images and sound files exist.
- Donor Bass invocation supports `-sym logfile.log`; export does not depend on
  an assumed assembler feature.
- Donor resolution and native ROM build are completed below; no gameplay, pixel,
  audio or timing PASS yet.
- Host preflight passed: both exact source pins clean, 8,363 tracked inputs,
  bundled tools and hash-pinned appender dependencies present, Python 3.13.15.
- Original ROM found at
  `D:/Stuff/DevFolder/Battleship/BattleShip/baserom.us.z64`: 16,777,216 bytes,
  SHA-1 `e2929e10fccc0aa84e5776227e798abc07cedabf`. This removes the source-ROM
  blocker; no user response is needed for the pending path question.
- Stage launch: `prepare_extra_donor.py --phase stage`, private
  `builds/p4/meta-knight-donor`, originating `exec session_id=90440`. Expected
  completion: exit 0, `stage passed`, complete `donor-manifest.json` with all
  copied source hashes matching. Next command uses the same roots/ROM/destination
  with `--phase resolve`; root owns this producer writer.

## Work and checks owed

Resolve the donor appender output in disposable staging; hash the source ROM,
toolchain and canonical outputs. Produce typed actions, resources and callback
contracts with unsupported semantics explicit. Preserve existing runtime kinds,
sentinels, save identities and roster behavior. Feed native geometry/motion/event
and audio producers before enabling CSS selection.

Required runtime coverage includes Meta Knight without Jigglypuff selected,
locomotion/multi-jumps/attacks/specials/items/CPU/copy, costume and wing/sword
states, CSS → SSS → battle → results/rematch, a natural-input playable ROM,
Latest regression coverage and actual resource/performance measurements.

`preparation-recipe.md` preserves the exact pins, captured commands and separately
labeled reconstruction commands for ignored production inputs. Cold replay and
the missing automatic Make dependency edges remain open. Documentation validation
passed (`check-docs.ps1`: 17 docs, six registry entries, 77 AGENTS lines); the isolated
route stays within the compact handoff/board limits.

Root initially owned shared integration, producer execution, builds and timing.
Ownership transferred to `runtime_integration` for the frozen build/runtime phase;
root now performs documentation and review only.

## Completed producer batch

The stage job exited 0. The reference donor assembled and CRC-updated successfully.
Review export initially failed because Bass evaluated names require scoped braces;
two tiny Bass canaries isolated the syntax. The producer was repaired and only the
review step resumed. Reference bytes were retained. An initial retry reused a log
name; the driver now records unique ordinal logs, marking the overwritten failure
as unavailable instead of reusing it as proof.

Final donor manifest is `builds/p4/meta-knight-donor/donor-manifest.json`, phase
`resolved`. Reference ROM: 65,536,000 bytes. Review: 65,536,276 bytes, SHA-256
`6a5f54d7dd5582586914d09bfd7b2fa17fce68ef3fbffb3f811ac55a7010f793`.
Every original byte matches except CRC header bytes 16–23. The resolved export
contains 252 actions, 225 main parameters, 15 menu parameters and 205 callback
addresses. Native callback admission maps all 205; unknowns fail closed.

Actual serial production commands completed with exit 0:

- `extra_native_asset_adapter.py`: 172 resources, 225 main + 15 menu descriptors,
  motion asset `0x6000`, 51 local pointer fixups. Main 2,416 B, model 75,296 B.
  Maximum source main animation is 65,264 B; this is required sizing, not waived.
- `extra_audio_adapter.py`: 601 cues, 7,162,036 B, all original 573 records retained.
  Pack SHA-256 `fbbc8db045ffef3b6ecd6fb0bda53d033eb3f9f188c43189fda8ef0a2098a621`,
  mapping `0xbed77fc6`. Runtime pins emitted by `generate_p4_audio_header.py`.
- `generate_p4_runtime_data.py`: four typed files, legacy enum/sentinels preserved,
  Meta Knight runtime kind 29. Source consumers are transformed in build-owned
  includes; read-only BattleShip remains untouched.
- `extra_resource_adapter.py --model-ir`: both detail storage unions have 23
  roots, 407 triangles and 713 vertices; canonical live draw has 13 roots.
- `generate_nds_native_owner_images.py`: preserved 25 legacy image slots and
  appended Meta Knight slot 25. Each detail has 28 arrays / 9,649 elements.
  Compiler-derived byte sizes and selected-root runtime wiring remain to qualify.

Source and focused host checks passed for raw resources, callback admission,
audio, CSS/HUD art/hit tests, compact preview identity, geometry/image ABI and
material policies. Native callback C passes DS GCC syntax-only. These establish
the checked source/host properties, not a running match or performance pass.

Source family 7 is implemented natively. Family 6 retains full art with one
explicit initial-alpha experiment; feedback/bilinear-edge comparison and owner
approval for a permanent visual delta remain pending. Transparent source output
has native state/lifetime accounting and avoids DS wireframe submission.

## Current next action and jobs

Selected-root/source-joint/matrix routing, mixed-lane attributes and compact Sprite
normalization are integrated. All eleven BEX dependencies prewarm in the scene
heap before the resettable CSS block; actual C boundary fixtures passed. The full
13-row admission includes both electric skeletons: 180 Meta records, 281,344 B,
FNV `D3A2073E`. It supersedes the base-only admission recorded below.

`runtime_integration` owns integration/producers/builds/timing. Its delegated four
source transformations are frozen; all helpers are frozen and root handles
documentation/review. Additional helper dispatch
and reuse were rejected at the nine-thread limit; serial progress continued, and
retries occurred only after workers completed. No donor job remains running.

The first two DS builds exited 2 on missing ignored reference inputs. Explicit
read-root repairs passed 23 host fixtures and cleared those failures in build
three. Build three exited 2 on the native-owner consumer closure policy for
prepared polygon/texture fields. Its unique log is
`builds/p4/meta-knight-native/build-third.log`. That producer classification is now
repaired, with all six consumed-field closures passing. No owned build or emulator
job remains live at this handoff. The next build follows the bounded interaction
repair and the main build writer's exit.

The read-only interaction census identified six source routing failures for the
new kind: Kirby pre-GO hat bounds; Falcon Dive, Yoshi Egg and ordinary throw victim
tables; down-bounce audio; and the disconnected captor-side DK breakout hook.
Kirby admission is repaired and passes a compiled 13^4 roster fixture. Captor-side
DK hook routing also passes its compiled fixture, retaining the vanilla path when
Meta is the victim of DK. The four other fixes, including the reached Yoshi
effect-size lookup, are frozen and pass eight fixtures. Actual donor qualification
confirms the three alias columns are 10, the Yoshi row equals source parent 10
byte-for-byte, and the bounce cue is FGM 306. Lifecycle regeneration exited 0
and emitted nine source copies. New manifest SHA256
`3F28E068B66D2BAF2633C7964FD3B4898529C2ABF86CE6ACE540F2CAA45AA772`;
producer SHA256 `CB81589C9B6EDF52EE6A7CAE958BA4FEC331E301B3378015285E4335F0B45957`.
Build four exec session 57783 exited 2, log `build-fourth.log`, after the main writer
idle check. It found a literal local O2R NessModel prerequisite in Make. The
integrator's one-time audit repaired all five remaining literal O2R prerequisites
in the native Ness/Yoshi/Purin owner block, with default ignored input roots also
following `NDS_REFERENCE_ROOT`. Focused diff checks pass. Fifth launch found the
main `build-lab-baked2` writer active (make PIDs 53176/14004), so no fifth job/log
existed at that check. The subsequent atomic idle check passed and build five is
ran on checkpoint `92bddf1b3e4`, integrator exec session 53064, log `build-fifth.log`.
It exited 2 after native stage generation and the start of C compilation: static
textures rejected absent local `ExternDataBank103`. Integrator repairs that owning
reader while `export_validation` audits other active ignored-input read boundaries
without editing. The complete pinned static-texture census and unconditional
Dream Land water 103/104 reader are now repaired; logical paths/source hashes stay
unchanged, physical reference reads are recorded and outputs stay local. Two
override/negative fixtures pass. The bounded audit is closed with two remaining
active chains: legacy CSS Main/Idle/corpus reads in `preview_source_metadata`, and
shield-pose manifest O2Rs through `estimate_fighter_pack.load_pinned_sources`.
Both are now repaired: preview Main/Idle and ID lookup use the selected corpus with
a root-scoped cache; shield-pose reads honor the same pinned O2R root. Four new
override/negative fixtures and four existing affected fixtures pass; focused diff
checks are clean. No other concrete missing read surfaced in the bounded audit.
Sixth build exec session 76673 exited 2, log `build-sixth.log`, on four ignored
credit encoded inputs; static textures now retain their payload/metadata hashes.
Those input reads were repaired without changing local C/patch/output roots.
Expanded shell-hwtri config also proved menu walk 1, so natural proof switched to
the existing `smash64ds-p2-shell-freeplay-hwtri` target. Seventh build exec session
37888 exited 2, log `build-seventh.log`, on companion credit metadata includes,
after legacy preview/shield progress and ShieldPose qualification. Its config
confirms Meta 1, fast logic 0, menu walk 0. The complete credit family is now closed:
eight includes (four encoded, four metadata) are read/hashed from the explicit
reference corpus; narrowed encoded tables and exact metadata copies are emitted
only inside the owned overlay, with bounded output deletion verified. Diff checks
pass. Eighth freeplay build is live after main baked5 exited and the idle check,
integrator exec session 82919, log `build-eighth.log`; expected exit 0 plus native
ROM. It passed credit includes; 1,570 direct-ROM clips had zero mismatches and
retained their pinned hash, and all 172 Meta resources were staged. ARM7's
native-only check passed over five actual inputs; UI/HUD Meta baking completed.
The command ultimately exited 2 at ARM9 link with one unresolved symbol,
`ftCommonJumpAerialUpdateModelYaw`, called by Meta's multi-jump helper. All current
producers completed. The normal-moveset import already exports the exact source
routine as strong symbol `ndsBaseFTCommonJumpAerialUpdateModelYaw`; Meta now calls
it through a two-line declaration/binding change. Ninth freeplay build is live
after idle check, integrator exec session 30239, log `build-ninth.log`, and exited 0.
First ROM is `builds/build-p4-meta-knight/smash64ds-p2-shell-freeplay-hwtri.nds`,
64,928,768 B; SHA256
`C0F93220BBD24477A3E7AE62CEF9FF4B0A69856C4B8D274CFD84BB8FD5D3422F`.
ARM9 native-only passes over 324 actual inputs, ARM7 over five; frontend overlay
passes. All six Meta image bins are staged. This proves a native build, not
playability. Main slot1 baked5 collector PID40408 is active; owned ports5593/5594
are free. Integrator prepares read-only guest probes and waits for the collector
exit before natural owned GUI/timing. That collector and main writers have now
exited; only owned slot0 melonDS PID17492 is active, at its ROM picker. Private
portable TOML confirms JIT off, isolated storage and ports5593/5594, with audio
muted. Its keyboard bindings are all -1; integrator configures owned controls
before loading/natural input. Controls are now configured (Z attack, X special,
A/S jump, Q grab, W shield, arrows stick, Enter Start, Backspace taunt), through
the owned TOML setter with the emulator stopped. Config hash
`451606FA0581A09322AAF91B5BB2C097464446AB9D4AD3D489A1BD22D720C30B`;
empty saves/states and private disposable DLDI. ARM7 map SHA256
`87FF8A883782F11ADD81FA93ADD29C90D5FF7F6A142105EBC9BB2EA4E821863D`.
Owned PID30364 now listens only on ports5593/5594; main collector is absent.
Native boot reached `ndsPlatformEndFrame` with playback disabled; startup GDB
exited 0 and detached. Visible title/flame background and FPS HUD render. Read-only
scene probe session6729 is live; next input is natural Start into menus. Meta
selection/gameplay remain unverified. A second GDB attachment failed negotiation;
its values and prior partial/fire-only title observation are discarded. This fork
requires one persistent MI session per emulator run, with explicit owned port.
Persistent MI session97088, owned PID49456, now confirms coherent title/logo,
copyright and PRESS START. UI kit loads 66,496 B; open/title-load fails 0, playback 0.
**Correction:** exact enum/source/input evidence shows `next_kind=60` is automatic
Explain attract, not natural Start. Input count/ring stayed zero. Unthrottled title
runs about600FPS and reaches650 idle tics before UI input. The claimed Start
witness is retracted. Explain incurs192 arena allocation failures and recursively
data-aborts (`__excpt_entry`, mode0x97, abort SP0x02fffd8e,
lr_usr0x0206ea51/`ndsMPVertexF32Reset+52`, screen0xFFFFFFFF). This is a real unresolved
startup fault; whether it is a Meta regression remains unknown. **Causal correction:**
192 is `gNdsTaskmanArenaAllocFailCount`, the boot `calloc` sizing ladder, not Explain
exhaustion. Chosen arena is905,728 B; heap starts0x022D2CC0. Explain loads only
Mario/Luigi and allocates their per-kind animation sizes, not the Meta-raised global
65,264 B maximum. Its native task buffers are3,408 B. The first fault PC/DFAR/LR,
heap bounds/free space and title-to-Explain counter delta are still needed.
Owned PID49456/session97088 exited; main baked6 collector fleet is now active.
Private UI profile caps60FPS, JIT still off, config SHA256
`D9B181E3F4F2CA13241F4F3067311F12EB51D5496666F236004E40AFD1E438E7`.
ROM/ELF are unchanged. `target-artifact-identity.json` under the private native
build records full ROM/ELF/map/config/runner identities, copied unchanged to
`build-nine-identity.json` here (metadata only, not runner settings or binaries).
Native asset helper
reviews the actual Explain scene budget read-only. Natural Start, Meta CSS and
match proof remain due, after the main fleet exits. Owned capped run MI session
21235/PID29316 currently localizes input without timing/visual acceptance during
the main fleet. Input/hotkeys UI confirms Return=Start, Z=A, X=B and the remaining
bindings. Instantaneous sky chords, including an immediate-resume attempt at early
title, have no positive input witness. Integrator tests only supported OSK/sky
actions next; no raw Windows automation or fighter/scene mutation. Bounded OSK
did not register input; Windows reports its higher integrity and denies automated
close/shutdown. User was asked to close OSK and signal readiness for held Enter.
Owned PID53264/MI93574 stays halted at title (`armWaitForIrq`, frame243 then280),
exception breakpoint4 armed. No manual handoff run was consumed. Seed1 at
0x021873A0, playback0, saves/states empty, backup load result3. Baseline taskman heap
{start0x02307800,end0x023E4A00,ptr0x0238FD98}; chosen905728 and probe failures192
already precede Explain. Fault cause and natural input remain unresolved.
Identity remains
checkpoint `92bddf1b` plus the documented read-root/Make/credit overlay; gameplay
and capture code remain frozen. No ROM packaged yet. Meta's
65,264 B maximum animation is included by the actual runtime allocation path.
No additional unchecked victim table surfaced in the bounded damage/item/visibility
review; target interaction proof remains due.

The worktree now has its own copy of the repo-local accurate melonDS and private
runner storage. Its boot-policy check session 6161 exited 0; JIT is disabled from
boot. No target emulator has launched. The main multi-roster
collector fleet must exit before isolated visual/timing work; TCP endpoints must
be checked separately from private configuration paths.

Next: package the native Meta-enabled natural-input ROM, measure startup/preview
admission, exercise CSS → SSS → human Meta Knight match → results/rematch, and
complete Latest. Authoritative visual/timing runs wait for main collectors to
finish; main runners, configs and save state remain untouched.

Native ROM link/package passes. Still unrun: actual image bytes/resource peaks, source-required
move/interruption/CPU/copy/entry/results/audio lifecycle, native pixels, full match,
timing/cadence and Latest. No ticks/FPS/P50/P95 values exist for this character yet.

## Integration findings and invalidators

- Mixed attributes/Sprite metadata was generated from the actual ARM32 compiler
  layout and frozen native containers. Stock/emblem ownership is model 5456;
  native normalizer host checks passed.
- A DS syntax check found a generated `void` declaration for a source `sb32` map
  callback. The producer now derives real unary-GObj return types and explicitly
  discards returns at the status callback ABI. Regeneration and focused DS compiler
  checks passed for callbacks, registry and common special-input routing.
- The base-only 13-row admission payload was emitted: 280,672 B, FNV `5119846B`.
  It contains 174 Meta records. **New missing coverage invalidates release use:**
  source electric colanim family 24 reaches Meta skeleton 2 then 1. Their Main
  descriptor tables at `0x428` and `0x520` each have seven live roots. Image and
  texture/semantic closure must include them before runtime qualification.
- Existing material pointer ownership must come from source external references,
  including CHARACTER 5456 at offset `0x188`; it cannot be inferred from offset
  range or the Main file. The provider now emits parallel material asset IDs.
- The private reference-read adapters passed path tests. 131 missing prepared
  asset files were copied under this worktree's `assets/`, retaining candidate
  outputs. These are prerequisites, not new qualified evidence.
- `make prepare-particle-banks` completed with exit 0 in the private build. Its
  source reads remain under the main read-only reference corpus; outputs/config
  and stamp remain under this worktree. Particle producer reported PASS.
- Main slot 0 Results tracing was observed and left untouched. It subsequently
  exited; no target emulator/build belonging to this worktree has run yet.

Live integration now includes selected-root/material/matrix routing, source
electric skeleton closure, semantic preview/core packs, and entry/copy/results
lifecycle. These are concrete engineering checks owed, not accepted content.

The preview pack and selected HIGH image use 62,160 B of the 81,920 B slot, with
19,760 B headroom. The source-preserving pack retains 36,332 B of resident data;
animation/status resources use their separate source lifetimes. Own victory music
was converted to the existing native ARM7 audio service (65,160 B BGA1 payload);
Results engagement and finite completion remain unrun.
