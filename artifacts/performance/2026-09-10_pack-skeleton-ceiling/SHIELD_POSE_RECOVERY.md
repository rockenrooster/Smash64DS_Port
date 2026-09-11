# P2-2 ShieldPose recovery and natural-path capacity result — 2026-09-10

Immutable implementation baseline: `d060f8941e1820dc48c770646002170bd9cfc18d`.

## Outcome

The lossless ShieldPose/AObjEvent32 subproblem is implemented and measured, but
**P2-2 remains RED on the natural four-kind runtime**.  The compact ShieldPose
representation moves the semantic estimator below the relaxed upper bound; the
actual battle still dies before frame 1 because the fourth fighter's **raw Main
file** is allocated before a compact fighter pack exists.

This is the discriminating blocker outcome: do not spend another cycle trimming
ShieldPose or reviving low-only/discrete-angle compromises.  The next capacity
work must replace the raw resident fighter Main/closure representation (or prove
an equivalent non-overlapping recovery) before the fourth Main allocation.

## Source-equivalent ShieldPose representation

BattleShip guard behavior (`ftcommonguard1.c`) selects one of eight 45-degree
ShieldPose tables, evaluates it once at the continuous local `angle_f`, blends
against the ShieldPose base transform by stick magnitude, and clears the
animation.  CPU stick vectors can produce intermediate angles, so no discrete
angle reduction is used.

Seven currently binding base fighters use one compact NitroFS blob each.  The
blob is loaded into the taskman arena only when that fighter's Main external
fixups reach ShieldPose; the roster does not pay all seven blobs at once.

| Fighter | Old estimator ShieldPose W | Compact resident blob | Per-kind reduction before shared reader |
|---|---:|---:|---:|
| Donkey | 9,460 B | 2,760 B | 6,700 B |
| Samus | 9,564 B | 2,856 B | 6,708 B |
| Link | 11,560 B | 3,042 B | 8,518 B |
| Kirby | 11,708 B | 3,141 B | 8,567 B |
| Captain | 10,736 B | 3,334 B | 7,402 B |
| Pikachu | 13,884 B | 4,061 B | 9,823 B |
| Purin | 17,700 B | 6,037 B | 11,663 B |
| **Total** | **84,612 B** | **25,231 B** | **59,381 B** |

The generated host oracle checks **745,720** source-vs-compact pose samples and
**11,088** stale-AObj-history probes.  Maximum pose error is
`0.0117993961`; maximum base-transform error is `0.0077620149`, both below the
existing `0.02` absolute joint-value gate.  Event32 Linear, Cubic, Step,
target-rate, wait and length-adjust semantics are preserved; runtime decoding
feeds the existing BattleShip parser/player rather than a second animation VM.

Generated manifest SHA-256:
`012BDF1BC660F7B5E5FE263FC0DAC41372B799FF8331148C32B98D04D4A998BD`.

Generator SHA-256:
`39E84BFC97033ACAC535A903E82EE0123EA7EBBB26C45C06F2E2A80C33436F4D`.

## Shared-memory charge

The per-fighter blobs stay in W.  Shared decoder/glue costs are charged against
the shell ceiling instead of hidden in the recovery:

- `nds_shield_pose.o` main-RAM allocation: **2,965 B**.
- Guard interposition same-config delta: **100 B**.
- Relocation external-fixup hook same-config delta: **48 B**.
- **Shared main-RAM charge: 3,113 B**.
- The 1,408-byte synchronous `DObjDesc` compatibility scratch is in
  `.sbss.shield_pose`, therefore NOLOAD DTCM rather than main RAM.

Post-link map for the full-content argmax build:

- `.itcm = 0x7e88` (344 B below the `0x7fe0` ITCM budget).
- `.dtcm = 0x2220`.
- `.dtcm.bss = 0x0618`, ending at **`0x02ff2838`**.
- The existing hard linker assertion requires `__dtcm_bss_end <= 0x02ff3000`,
  leaving **1,992 B** to the measured boot-stack low-water boundary.

The unrelated 16-byte full-content ITCM overflow encountered during integration
was closed without dropping Task37 residents or fighter content: the native
submission failure cleanup moved out of the hot ITCM executor.  This placement
repair is required for the full-content link but is **not** counted as pack
recovery.

## Exhaustive semantic estimator

Commands:

```text
python -m unittest scripts.fighters.test_estimate_fighter_pack
python scripts/fighters/generate_nds_shield_pose_pack.py --check-assets
python scripts/fighters/estimate_fighter_pack.py --ledger --strict --json builds/p2-2-shieldpose-resident-capacity.json
```

Results:

- Estimator tests: **78/78 PASS**.
- Generated asset/oracle check: **PASS** (`fighters=7`, `blobs=25231`,
  `old=84612`, `recovery=59381`, `max_base=32`, `max_scratch=58`).
- All 793 one-through-four-kind sets enumerate without STOP.
- Source-complete raw worst: **299,648 B**, Link+Pikachu+Ness+Kirby.
- VRAM-resolved worst: **287,688 B**, Donkey+Samus+Link+Kirby.
- Raw relaxed skeleton upper bound: **291,268 B**.
- Shared ShieldPose reader/glue charge: **3,113 B**.
- Effective relaxed upper bound: **288,155 B**.
- Optimistic estimator margin: **467 B**.
- Static verdict: **UNKNOWN**, not GREEN.  The unresolved band straddles the
  relaxed bound and the old skeleton never measured the exact four-slot ceiling.

Permanent JSON:
`SHIELD_POSE_CAPACITY.json`, SHA-256
`3FC0256CBB298E4C87E1DB5EF350EAED132E3D20BFBC7094F2BE7BE6D3351383`.

## Natural four-kind runtime

Build: `smash64ds-p2-fourcpu-tickhud-hwtri`, custom full-content argmax config:

```text
kind0=2 Donkey
kind1=3 Samus
kind2=5 Link
kind3=8 Kirby
```

Pikachu/Yoshi/Ness/Purin/Captain are also built because Kirby copy support
requires those landed dependencies; only Donkey/Samus/Link/Kirby are
instantiated by the stress descriptor.

ROM SHA-256:
`24FBC52D3CDAA34E56DE87636CC811DFE74F10EA6D22A9F7AA3EC3A86263C1F6`.

ELF SHA-256:
`B91A21B6616A92C35887EB5491CB1A95E348A8ECE9743DCE5CF226D162B99A9B`.

The native-only build check passes.  NitroFS contains the exact seven generated
ShieldPose blobs at `fighters/shield_pose/{02,03,05,07,08,09,10}.bin` with the
sizes listed above.

Widest relevant natural verifier:

```text
scripts/verify-p2-four-fighter-stress.ps1 -NoBuild \
  -Build builds/build-p2-shieldpose-argmax-full
```

It halts at **presented frame 0**, before battle acceptance:

```text
TICKFAULT ndsSyMallocOverflowHalt
TICKFAULT_HEAP request=201456 free=47980 arena=1335296 animcache=0
...
ftManagerSetupFilesMainKind -> fkind=8 (Kirby)
```

The first three fighters prove the compact replacement engaged before the halt:

- `gNdsShieldPoseLoadCount = 3`
- `gNdsShieldPoseResidentBytes = 8,658` = Donkey 2,760 + Samus 2,856 + Link 3,042
- `gNdsShieldPoseNativeFixupCount = 27` = 9 exact Main→ShieldPose fixups each
- `gNdsShieldPoseLoadFailCount = 0`
- `gNdsShieldPoseNativeFixupRejectCount = 0`
- `gNdsShieldPoseDecodeFailCount = 0`
- Guard apply counters are 0 because the battle never starts.

Kirby's compact blob is never requested: `ftManagerSetupFilesMainKind` first
tries to allocate the **raw 201,456-byte Kirby Main file** with only **47,980 B
free**, so the scene stops before its external ShieldPose fixups run.

Permanent failure log:
`artifacts/verification/2026-09-10_p2-2-shieldpose-argmax-frame0-failure.log`,
SHA-256
`1334A1B1D551EDB3F608F0300D731A64F591508AA22CA354C5B2B3E975E70720`.

## Verdict / next action

**P2-2 capacity remains RED on the natural runtime.**  The ShieldPose recovery
is valid and engaged, and it removes the earlier AObjEvent32 representation
unknown, but it cannot close the match because the current runtime still
allocates raw fighter Main payloads.  The immediate blocker is now concrete:

> fourth-kind Kirby Main wants 201,456 B; only 47,980 B is free.

Continue with the planned compact resident fighter Main/closure pack (or an
equivalent non-overlapping source-exact recovery) and measure that natural
four-kind lifetime.  Do not spend more capacity work on ShieldPose, low-only
detail, discrete shield angles, or required-content removal.
