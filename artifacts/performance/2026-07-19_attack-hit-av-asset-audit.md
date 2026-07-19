# Attack / hit audio-visual asset audit — 2026-07-19

Status: **KEEP** the three exact audio additions and the source-to-DS visual
coverage checker. Preserve the existing five-effects branches/worktrees. Do not
substitute an approximate sample for any source cue that remains fail-closed.

## Scope and preserved in-flight work

The audit started from `master` base `47adafffd15933cc5e0054119f4b0520c5299fba`
and inspected the BattleShip Mario/Fox motion sources, hit routing, special
projectiles, current DS effect seams, generated FGM pack, and the live focused
runtime routes. No branch, worktree, or patch-unique evidence was reset, merged,
or deleted.

| Preserved branch | Commit | Disposition |
|---|---|---|
| `codex/five-effects-attack` | `e56003c2adabcaae2a5cf95a05843403a91c1fdb` | preserve |
| `codex/five-effects-fox` | `636737c555fb0a0920f63845434cf705444c5cec` | preserve |
| `codex/five-effects-hit` | `85d0ac30202b5220fdbb5aa41e236e640fc614b7` | preserve |
| `codex/five-effects-audio-union` | `cb7b70c89579cb2abb4f109a6f965983faaf36aa` | preserve |
| `codex/five-effects-integration` | `7cd139af32f2fe14999890f3c18b05264f0ecdfc` | preserve |
| `codex/five-effects-mario` | `c76d0d1022f23f4cd835b796d436460b1ce33f18` | preserve |
| `codex/five-effects-visual` | `4d23dedf6ca339756a20d87fe52bda52176b4d29` | preserve |

The three patch-unique worktrees remain at
`D:\Stuff\DevFolder\Smash64DS_Port-worktrees\{attack,fox,hit}`.

## Exact audio additions

The generated pack grows from 24 / 121,720 bytes to 27 / 128,196 bytes. It
contains 20 unique sample bodies / 127,044 sample bytes and remains below the
128 KiB resident cap by 2,876 bytes.

| ID | Source cue | Exact retained behavior |
|---:|---|---|
| 303 | `nSYAudioFGMMarioDownBounce` | Its source root and render programs are byte-identical to Fox ID 300. It adds one 32-byte mapping record, reuses the exact ID-300 AOT sample body, and has no fidelity debt. |
| 373 | `nSYAudioVoiceFoxSmash2` | Source sound 106, 4,880 samples, 2,444 IMA bytes, 60 ticks, and the one exact volume-envelope edge; no loop, fork, FX, or pitch-schedule debt. |
| 374 | `nSYAudioVoiceFoxSmash3` | Source sound 107, 7,856 samples, 3,932 IMA bytes, 86 ticks; no loop, fork, FX, or pitch-schedule debt. |

The BattleShip random Fox smash table is `{Smash3, Smash1, Smash2}`; all three
variants are now packed. Mario/Fox down-bounce dispatch remains the shared
source table with six Mario-family and two Fox-family entries. The ID-303 and
ID-300 root program SHA-256 is
`0a7645ae1249ff5140ddbf80859b52c127b73d2b80e0b97d90cc3b61b0c4b262`;
their render-program SHA-256 is
`9ed69d587dab562768d6321d349477c4f522c0b65115fb7cb2c1f27d5b27c4c2`.

Pack identity:

```text
resident bytes       128196
entry count          27
unique samples       20 / 127044 bytes
mapping checksum lo  0x682295bf
pack SHA-256         e8f723fc96ab5c2424d0dd58c4c479cbd77fc230d43d549dd5321f5fe5e1daaa
sample SHA-256       966377eb9067a94e644d7291b9c5da5e3e3f7ad3eff5fbed55587ab19619ac49
envelope SHA-256     a9b69e65e7843e938644939295e5737c7582d977f07c578cffc8fdc3532e1eb6
```

The deterministic focused window improves from 22 supported / 19 unsupported
play requests to 26 supported / 15 unsupported. The four newly supported
requests are Mario down-bounce twice plus Fox Smash2 and Smash3 once each.

## Intentionally fail-closed audio

These are audited source requirements, not forgotten mappings:

- Fighter voices 375, 429, 431, 435, and 440 require live pitch schedules that
  the current packed DS entry format cannot reproduce exactly.
- Direct activation IDs 19, 41, 42, 43, 185, 186, 187, 189, 190, 217, 218,
  and 219 require some combination of pitch/volume automation, custom FX-bus
  output, infinite source loops, simultaneous fork voices, rates above the DS
  `u16` channel-frequency field, more handles, or more resident bytes. ID 215
  is the one exact packed special activation in that census.
- Hit composites 216, 28, 2, 0, and 188 remain fail-closed because their exact
  source composite/fork/loop/FX behavior is not represented.
- Common contacts 40, 38, 37, 34, 32, and 31 use their exact primary
  BattleShip samples. Composite forks/custom FX remain explicit fidelity debt;
  no unrelated fallback sample is played.

## Visual coverage

`scripts/check-battle-playable-attack-effects.ps1` regenerates the exact static
census from `202_MarioMainMotion.c` and `208_FoxMainMotion.c`: all 178 effect
calls across all 17 source effect kinds have a bounded live DS route. It also
guards the direct source paths for Fox reflector, Fox blaster glow, and the
Mario fireball dust/fire/sparkle routes. The checker is part of normal Boundary
verification.

This proves that the P1 Mario/Fox calls are wired to recognizable source-derived
presentation. It does not claim original N64 common particle-bank texture/script
exactness; those banks remain outside the DS resident set and are durable visual
fidelity debt rather than missing call routing.

Focused visual/runtime evidence:

- Mario fireball: 3 created effects, 13 rendered submissions, 96 triangles,
  kind mask `0x45`, reserve 184,656.
  `artifacts/visibility/2026-07-19_055856-3573794_fireball-long-travel-p51056.png`,
  SHA-256 `E2DFBDF8DFE10290417EEDC8A1D9AEF8C26ACC8FE9857C7E0CB7A15D43BBC365`.
- Natural attack/hit: source contact ID 32, pan 80, source frame 125,
  audio play/generation/active `1/0/1`, visual create/submit/triangles
  `3/3/24`, reserve 183,056.
  `artifacts/visibility/2026-07-19_060122-3744199_fox-recovery.png`,
  SHA-256 `E71288849F41534F8559646C9EA64C010C888F72E4FE5EC3CDA2009FFE2D8424`.

## Verification

The generator `--check`, static FGM pack checker, runtime fixture checker,
focused audio verifier, focused natural hit verifier, and Boundary all pass.
The focused audio run reports pack 128,196, peak minimum 22,022, RMS minimum
4,467.425, phase mask `0x1f`, channel mask `0xe`, BGM channel 0, three maximum
live FGM channels, 66 envelope steps, and 183,056 bytes headroom. Its object
budget is text/rodata/data/BSS/ITCM `3456/553/4/130018/0`.

The rebuilt profile-0 Boundary ROM is 14,688,256 bytes, SHA-256
`C344CA8B89903A35678A6DC4A849F217B690CBCE16841FAC47974626899EB9DF`.
The authoritative fixed-frame screenshot pair passes at 786 / 49,152
meaningfully changed pixels (1.599%) with all fighter, stage, texture, and
detail gates green. This was a runtime motion gate, not a cross-ROM pixel A/B.
The smoke reports 25.9 presentations/s and 51.6 source updates/s.

The first verifier integration accidentally inspected `$LASTEXITCODE` after a
PowerShell-only checker; `$null` caused a successful early exit before the
canonical build. The integration now relies on the checker's thrown failures
under `ErrorActionPreference = Stop`. A subsequent unsynchronized early-capture
pair correctly rejected 44.922% live camera/input drift; the authoritative
Boundary fixed-frame path then passed without weakening either gate.
