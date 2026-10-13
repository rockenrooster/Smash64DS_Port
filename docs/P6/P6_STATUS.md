# P6 status — Ike and Palutena

Owner of the live P6 state. The plan is [P6_PLAN.md](P6_PLAN.md); the cards
are [characters/ike.md](characters/ike.md) and
[characters/palutena.md](characters/palutena.md).

**State (2026-10-13):** active (goal P5-P7). Move cards approved (I1, P1).
No P6 code, art or build exists yet.

## Board 1

| Item | State |
|---|---|
| A1 registry, build, null clones | planned |
| A2 model reader and writer (F1) | planned |
| A3 Blender bridge | planned |
| A4 animation codec and retarget (F2) | planned |
| A5 motion-script assembler (F3) | planned |
| A6 review harness | planned |
| A7 authored export (F4) | planned |
| A8 audio | planned |
| A9 presentation | planned |
| A10 effects | planned |
| A11 Kirby | planned |
| A12 CPU (F5) | planned |
| A13 checks | planned |

## Board 2

| Fighter | Step | State |
|---|---|---|
| Ike | I0-I6 | planned |
| Palutena | P0-P8 | planned |

## Art approvals

One row per owner verdict: date, fighter, gate (G1 look, G2 clips), what was
shown (paths under `art/p6/<name>/review/` or `artifacts/p6/review/`), verdict.

| Date | Fighter | Gate | Shown | Verdict |
|---|---|---|---|---|

## Owner decisions

- 2026-10-13: both move cards approved as drafted (I1 for Ike, P1 for
  Palutena), when P5-P7 became the active goal.

- 2026-10-12: D1 art committed to the public repository, Ultimate data never;
  D2 Explosive Flame in Ultimate's form; D3 Palutena's dash attack is the
  shield charge; D4 her taunt is Ultimate's down taunt
  ([P6_PLAN.md](P6_PLAN.md) "Owner decisions", with the defaults still open).
- 2026-10-12: the owner supplied Ike's audio: Brawl voices
  (`decomp/alt_assets/Fighters/Super Smash Bros brawl/Ike (International)/`,
  40 WAVs) and Ultimate's Ike sets (`.../Super Smash Bros Ultimate/Fighters/`
  `se_ike`, `se_ike_c01`, `vc_ike`, `vc_ike_c01` and the two Japanese voice
  sets); all local and ignored. D5 Ike's voice is Ultimate's Radiant Dawn set
  (`vc_ike_c01`); D6 his sound effects are Ultimate's (`se_ike`). The Brawl
  clips stay as a reference.
- 2026-10-12: D7 the announcer's calls are Ultimate's narrator clips
  (`vc_narration_characall_ike.wav`, `vc_narration_characall_palutena.wav`,
  owner supplied, local and ignored), level-matched to SSB64's announcer.
- 2026-10-12: D8 the crowd chants are Ultimate's cheers (`vc_ike_cheer.wav`,
  `vc_palutena_cheer.wav`, owner supplied, local and ignored); D9 Palutena's
  victory music is the Remix Kid Icarus "Overworld" excerpt (default approved).

## Environment

- 2026-10-12: Blender MCP registered with Claude Code for this project
  (`blender`, local scope: `uvx mcp-for-blender@2.1.9`, `DISABLE_TELEMETRY=true`).
  The MCP for Blender add-on (v1.8, protocol 13) is enabled in Blender 5.2 and
  starts its socket on port 9876 when Blender opens with its UI; checked with
  `get_addon_info` (protocol 13). The tools load in sessions started after the
  registration.
