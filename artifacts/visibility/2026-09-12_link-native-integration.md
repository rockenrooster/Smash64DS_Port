# Link native integration — 2026-09-12

## Kept source checkpoint

Implementation and owner documentation pushed in `6c75e56f677`.

**Compact capacity passes for the source-default Donkey/Samus/Link/Kirby
one-minute window with HIGH and LOW resident. Native acceptance remains RED
at the separate Samus morph-ball family.** This final run uses the exported
source and private slot-7 FAT/save/state paths, with no concurrent emulator.
It supersedes the diagnostic measurements below for current acceptance.

- ROM: `31A46F08295C354FAF6B1B34BF5C2845E7F5DD88A8DA3A3E0FFE2CD0D27FEA71`.
- ELF: `BFAF844C1A64246468FFDDF60E3A532D44A35D9C56F04AA19A34D7BE4EA96094`.
- Frames 1–1,973, clock 60→1; both runtime item overrides zero at start/end.
- Four cores: 92,200 B; 18 extern patches; four ShieldPose packs: 11,799 B,
  36 fixups. All compact load/fixup failures zero.
- Heap free minimum **48,868 B**, margin **23,268 B** over the 25,600 B floor.
- Allocator, object-manager, animation normalization, texture and plan mismatch
  failures zero; one source spline descriptor normalized, none unresolved.
- Native failures: 290, first Samus status `0x9D`, asset `0x140`, root `0x8158`,
  rejected complete program. No global native-output closure is claimed.
- ALL ticks: P50 **2,798,208**, P95 **3,918,528**. VBlank 2/3/4/5+ =
  **75/95/495/1,308**, maximum 25, total 1,973. Performance remains deferred
  debt. One known timer-wrap correction at frame 304 gives ALL 3,358,528,
  consistent with this distribution.
- ITCommonData **82,976 B** is re-observed in this valid isolated run.

Command: `verify-p2-four-fighter-stress.ps1 -NoBuild -Build
builds/p2-session-20260912/export/builds/build-p2-battle-core -RunnerSlot 7`.
Full log: `builds/p2-session-20260912/export-isolated-stress.log`.
Telemetry hashes (rotating paths under `artifacts/verification/`):
memory `4EF9E56566A193A191D5F452DE85188A0DCAA357DBD3B15DCD37DF4A8424B676`;
coverage `8CA594BF4220A846C9AD4864403CFF9D6FDF2B4D6839AD5E7B8DD4BFDFA0636F`;
ticks `73EE818E0B207B71A621E5E2787261786A229CE29AD71867704C39AD2CFE8C3C`.

The export's earlier Dream Land rejection was a missing **shared texture-key
producer**: runtime already expected alpha-representation bit 29, but HEAD's
generator omitted it. The existing source-derived static-texture/census package
and coherent item-arrow bank owner are now included; stage geometry itself did
not change. The isolated frame-32 repeat has zero native rejection.

Concurrent earlier runs shared canonical DLDI storage; host ROM staging changed
that image despite guest read-only mode. Those runs are invalid frozen-input
evidence, including the isolated-looking frame-1,294 fault transcript below.
That fault did not recur in the valid complete match. Numbered runner slots now
default to private FAT/save/state storage; policy checks enforce distinct paths.
Exact performance/visual acceptance still runs without concurrent emulators.

The current integrated `smash64ds-p2-shell-hwtri` builds native-only (262
inputs) and passes static textures, ITCM and DTCM checks. Its isolated Mario/Fox
regression run times out after 600 s before `scVSBattleStartBattle`; this is a
shell progression failure to localize, not evidence of a CPU abort or a passed
battle. Log: `builds/p2-session-20260912/battle-regression-isolated.log`.
The Latest batch remains RED; no new publication is claimed. Prior startup
passed, but the full shell loop was not qualified after input/storage corrections.

After restoring source item defaults, the ordinary production ROM was rebuilt
and its isolated private-storage startup check passed: title frame 121,
fire frames 61, animation frames 48, all asset/scene failures zero.
Build-health ROM SHA-256:
`86AAB7BF209E683B8F5246AA3FDB42F9CB3B443AADC7A24BF8EF3EA40F03E48F`.
This does not replace the published acceptance hash while the shell guard is red.

## Scope and source

P2-3f33 now has source-derived Neutral-B mixed-file binding and Spin Attack
admission. **The initial integrated ROM contained an unrecorded capsule
diagnostic. Its output is diagnostic evidence, not shipping-path acceptance.**
Existing raster defects and unmeasured siblings remain under their owners.

BattleShip `224_LinkMainMotion.c` defines the throw, empty and catch modelpart
commands. `225_LinkMain.c` resolves joint 11's held boomerang to asset 326,
root `0xF8`. The 20-root SpecialN program therefore uses LinkModel tables plus
the separate LinkBoomerangModel tables; its parent/cross tables must also have
20 entries. The runtime now dispatches both topology getters to that program.
Four host tests derive the visible vectors from the source motions in HIGH and
LOW detail, including the canonical and Entry-shaped sibling vectors.

Spin Attack is a separate weapon owner. `wplinkspinattack.c` uses LinkMain's
WPAttributes; `324_LinkModel.c` at `0x118F8` links the drawable child to root
`0x11680`. Its native owner still requires all nine PRIM-only materials.
Admission follows the live weapon's relocated DL and exact file/root identity,
as the existing Cutter and Boomerang paths do. Compact packing does not publish
the raw `gFTDataLinkModel` slot. Symbol-offset remapping now also compiles for
compact VS configurations, rather than depending on the 1P flag.

## Integrated diagnostic evidence

Frozen native-only four-CPU ROM: Donkey / Samus / Link / Kirby, one-minute
Time on Dream Land, both model details resident. Source inspection later found
`gNdsItemRateOverride=VeryHigh`, capsule-only toggles and an extra
`spawn_wait=1` branch. These bypass battle settings and change shared RNG
consumption. Identical `nds_build_config.h` files did not expose that difference.
The debug changes are now removed; source spawn timing and zero defaults return.
The stress verifier checks source defaults/law and records runtime overrides.

- ROM SHA-256: `EA2189C30F0A07EF0437040BBF6440DB0635F531D9925704946CCE5841625FD8`.
- ELF SHA-256: `15A0181ACF9E8AD577E9EB0D8024E6FEEF5DC419D37662DE20F74DE307ED7ACB`.
- Build: `make TARGET=smash64ds-p2-fourcpu-tickhud-hwtri BUILD=build-p2-battle-core`.
- Native-only link-input gate: PASS, 245 actual inputs.
- `probe-p2-fourcpu-sparse.ps1 -NoBuild -Build build-p2-battle-core
  -FirstLinkSpecialNReject -Frame 1536 -RunnerSlot 6 -TimeoutSeconds 900`:
  no family rejection through 1,536; program 3 selected eight times; boomerang
  roots submitted 29/12 times; texture reject mask zero.

The first admission candidate, before the symbol-remapping guard correction,
also produced 21 Spin draws by frame 352. That earlier probe is not the identity
of the screenshots below; their markers pin the final ROM above.

Both captures use ordinary source weapon makers and require a positive draw
delta from that maker to the shot. They do not set fighter status or fabricate
weapon state, but their surrounding match has the diagnostic item law above.
`capture-p2-shell.ps1` now launches hidden and lets the existing
capture helper restore the window only for the actual shot.

| Natural capture | Presented frame | New native root submissions | Texture rejects |
|---|---:|---:|---:|
| [Spin Attack](2026-09-12_link-native-link-spin.png) | 337 | root 26: 8 | 0 |
| [Neutral-B](2026-09-12_link-native-link-neutral-b.png) | 846 | root 27: 7 | 0 |

Marker values are summarized above; full debugger logs remain local.
These shots expose remaining presentation defects; they are output evidence,
not a claim that the complete fighter image meets visual acceptance.

## Reproducible dependency boundary

The source export starts at `5e09e477b294b94b2c4835308ec020275ad41f7a`
and includes compact battle cores, compact ShieldPose/Main extern restoration,
the Kirby mixed-width motion normalizer, Link native changes and their generator
dependencies. It excludes unrelated local CSS, 1P and item work. The export is
under `builds/`, with read-only reference/asset junctions; it is not a registered
worktree. No new worktree was created.

The LOW-only four-player preload shortcut was removed: source
`ftcommondead.c:529` still requests HIGH. Prior LOW-only capacity figures must
not be reused for this candidate. The unused battle-core header emitter and
Makefile bookkeeping were deleted; runtime packs and the manifest remain.

The clean-source build exposed a missing producer for
`NDS_NATIVE_KIRBY_HAT_BINDING`; the runtime had compiled only because a dirty
generated header supplied it. The image generator now derives this binding
from its source hat contexts. Generator source and generated tracked header
travel together in the reproducible checkpoint.

## Diagnostic one-minute measurement — not the standing gate

The same-ROM stress verifier completed frames 1–1,973, clock 60→1, with all four
fighter slots drawn. It returned RED: 31 native failures, first domain 1,
identity `0x30140` (Samus / asset 320), status `0x9D`, root `0x8158`, reason 2.
Link's former mixed-program and Spin admission failures are no longer first.

Both detail levels remain resident. Free minimum is **14,152 B**, below the
25,600 B floor by 11,448 B. Neither this diagnostic population nor the older
86,700 B LOW-only figure establishes the normal complete-presentation margin.
SyMalloc overflow, object-manager panic, animation normalization failure,
graphics heap overflow/no-room, texture rejection and plan mismatch are all
zero. DamageSlash submits both roots and ITCommonData is 82,976 B, closing the
specific runtime-size observation owed by P2-3f48.

Diagnostic cadence is not accepted performance: ALL P50 **5,039,104** and P95
**6,159,872** ticks. VBlank intervals: 2 = 76, 3 = 86, 4 = 10, 5+ = 1,801;
maximum 25, total 1,973. The sampler corrected two documented 2^22 timer-wrap
artifacts (frames 516 and 1,752); corrected ALL values are 5,039,168 and
4,479,040, consistent with this distribution. No optimization gate is waived.

Rotating telemetry identities:

- Memory JSON: `72A2F418B12D62CFA2DFC2CBC0DEF1DD931B2D2125FCD8721F7109F914F0F7B2`.
- Coverage JSON: `79C4FC01B483F130FE519F4F7F46EADD01CEB877955C7A4147B3A59B439EE9B0`.
- Tick JSON: `75044AAE352D5067058DFCF2FAAC50FAFF899BB72BE12779FB5719621BDA61CE`.

## Wider integration

The first Latest attempt stopped at the shipping texture census: the owner's
64-byte item-arrow OBJ bank was absent from its total. The checker now validates
all five contiguous banks, 63,808 / 65,536 bytes; deliberate overlap and overflow
cases still fail. Production build and startup passed before discovery of the
item override. The old shell-loop run was stopped after that discovery and is
not a completed loop verdict. No new ROM publication or full Link closure is
claimed here.

Curated source tree `4e8bb441b3ebb41cb8693e8c9f9340b35bf39ca6` builds with
native-only PASS (245 inputs). ROM `B0B7653AF01FAE0198FC2C3C2D97DEA4497F1FADECD87B6D5BFF1A394337D5FD`,
ELF `F2D87CBB23A1B774BCF44DBDE38A1DD6BE04B30D3716728F1566633AB747498A`.
Its frame-32 probe proves 4 cores / 92,200 B, 18 extern patches, 4 ShieldPose
packs / 11,799 B, 36 fixups and 65,476 B heap free, all loader failures zero.
It also exposes an earlier native rejection: domain 2, asset `0x68`, root
`0x6C0`. Its Neutral-B run reaches 1,536 but does not engage program 3 or the
boomerang, so the probe correctly fails. Buildability and capacity engagement
do not qualify native output. Reconcile the remaining integrated dependencies
and prove source-default input before promotion.

The corrected integrated source-default ROM is
`198FF2BF8EE43DCABB43B2A1DD6CD916ACE7DC34B21D3707B4B27024D39AE8C5`
(ELF `80442D727320BF99084B591A446BEB7FD3F584B315DCFC66423DC5ADC98140D5`).
Its frame-32 probe has zero native failures, four compact cores and 57,884 B
free. The updated stress sampler observes zero item overrides at every stop,
but aborts at frame 1,294 in `syInterpGetFracFrame(desc=NULL, t=0)`,
`interp.c:305`. The fault witness has 38,704 B free, zero allocator/panic/texture
or native failure, and all four compact cores/ShieldPose sets live. This is an
animation-descriptor failure, not a capacity pass or a completed match.
