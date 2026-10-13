# P6 status — Ike and Palutena

Owner of the live P6 state. The plan is [P6_PLAN.md](P6_PLAN.md); the cards
are [characters/ike.md](characters/ike.md) and
[characters/palutena.md](characters/palutena.md).

**State (2026-10-13):** active (goal P5-P7). Move cards approved (I1, P1).
Plan revision 3: Palutena's look goes to the owner as two best-effort
candidates at the same budget, the 3DS kit rebuilt and hand-tuned (Track K)
or Peach restyled (Track R), and the pick also sets Ike's (D12, G0);
kit-derived art stays local (D10); no Smash 4 motions (D11). No P6 code, art
or build exists yet.

## Board 1

| Item | State |
|---|---|
| A1 registry, build, null clones | planned |
| A2 model reader and writer (F1) | planned |
| A3 Blender bridge, kit reader, fit | planned |
| A3b Track K: the kit rebuilt (F6) | planned |
| A3c Track R: the base restyled (F6) | planned |
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
| Palutena | G0 look bake-off (K and R, sheet, pick) | planned |
| Ike | I0-I6 | planned |
| Palutena | P0-P8 | planned |

## Art approvals

One row per owner verdict: date, fighter, gate (G0 bake-off pick, G1 look, G2
clips), what was shown (local paths under `artifacts/p6/review/`, with
hashes), verdict.

| Date | Fighter | Gate | Shown | Verdict |
|---|---|---|---|---|

## Owner decisions

- 2026-10-13: D12, after reading plan revision 2: the owner doubts the kit's
  models can be cut to the DS budget and look good without hand-tuning, and
  asked to see both: the kit model reduced with best-effort hand-tuning, and
  Peach restyled into Palutena (same triangle range), each with best effort.
  Planning only for now. Plan revision 3: the G0 look bake-off.
- 2026-10-13: the owner supplied the Super Smash Bros. for Nintendo 3DS kit
  (`decomp/alt_assets/Fighters/Super Smash Bros 3DS/`: both fighters' models
  in two details with skeletons, 8 costumes each, Ragnell, the staff and
  shield, Palutena's wings and entry gate, icon sheets, audio; no
  animations). D10: it is the source of both looks, converted onto Roy's and
  Peach's skeletons at their budgets (the body's source later made a pick by
  D12); converted art is a local build output
  under `builds/p6-art/` and only the recipe and authored layers are
  committed. D11: no Smash 4 motion files exist; clip sources unchanged. The
  kit's audio is the same recordings as the chosen Brawl and Ultimate sets
  (cross-correlation), so D5-D8 stand. Plan revision 2.
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

- 2026-10-13: Blender 5.2.2 LTS has no COLLADA importer (FBX and glTF only);
  the kit's .dae files go through `scripts/p6/dae_reader.py` (A3). The 3DS
  folder was untracked and not ignored; `.gitignore` now ignores it beside
  the Ultimate and Brawl folders. The owner then installed the third-party
  "Simple COLLADA" add-on (ekztal v0.7.2, in the user add-ons folder); it
  builds an empty armature on every kit file (node ids vs skin sids), so the
  project reader stays canonical ([P6_PLAN.md](P6_PLAN.md) "The 3DS kit").
- 2026-10-12: Blender MCP registered with Claude Code for this project
  (`blender`, local scope: `uvx mcp-for-blender@2.1.9`, `DISABLE_TELEMETRY=true`).
  The MCP for Blender add-on (v1.8, protocol 13) is enabled in Blender 5.2 and
  starts its socket on port 9876 when Blender opens with its UI; checked with
  `get_addon_info` (protocol 13). The tools load in sessions started after the
  registration.
