# Luigi — P2-3 fighter 1 (pipeline prover, Mario variant)

Status: pipeline/runtime asset bootstrap in progress · Reference: `decomp/BattleShip-main/decomp/src/ft/ftchar/ftluigi/`

## Pipeline inventory (2026-08-21)

The P2-3 manifest now derives Luigi directly from BattleShip's `dFTLuigiData`
and `dFTLuigiMotionDescs`. Core ownership is exact: Luigi owns
`LuigiMain` (`0xDD`), `LuigiMainMotion` (`0xDC`), `LuigiModel` (`0x143`) and
`LuigiSpecial1` (`0xDE`); he shares Mario's ShieldPose, Special2 and Special3,
and has no Special4. The 12 Luigi-local animation resources are recovered from
BattleShip's generated `1103_FTLuigiAnimEggLay.c` …
`1114_FTLuigiAnimFSmashLow.c` provenance, not hand-assigned IDs. His full
motion inventory is 143 files and includes 19 source-shared Mario item motions.

The inventory now also drives a default-off runtime bootstrap. With
`NDS_P2_LUIGI=1`, the generated Make fragment stages all 16 incremental O2R
resources and the generated runtime catalog binds BattleShip's semantic symbol
addresses to those exact source IDs. The Luigi status table is the source
nine-entry table rather than the historical 16-entry compatibility stub. A
Luigi-enabled P2 shell build is green. Luigi is still not selectable until the
native-owner, CSS/audio, move-inventory and four-CPU budget slices pass.

The native-renderer input is source-derived too now, without changing the
qualified Mario/Fox program. The existing AOT display-list decoder independently
walks `LuigiModel` (`0x143`, SHA-256
`793c2f3ae89aa8925f4cd715b40a79b3fe9236c033d84a4e270f09bc88dd4247`)
and proves both BattleShip JointTrees: High `0x2410` and Low `0x49e8`, 25 live
joints / 14 drawable bindings each. High decodes to 320 triangles, 32 runs and
264 dense DS vertices; Low to 200 triangles, 20 runs and 181 dense vertices.
Both preserve the source hierarchy (5 pushes / 5 pops), the same eight
cross-matrix logical bindings as Mario, and the exact 44 root-prefix + 8
intra-root light commands. The generator also proves 70 High / 46 Low GX
restores from the decoded corner stream. These facts are emitted into
`fighter_production_manifest.json`; the next slice consumes them to add owner
kind Luigi to the production renderer instead of hand-copying model topology
into `nds_renderer.c`.

## Role

First new fighter on purpose: shares Mario's skeleton/kit shape, so he proves
the variant path of the pipeline (data-driven divergence from an existing
fighter) before any hard archetype. Unlockable in the original (gating P2-7;
selectable in dev builds).

## Moveset uniques (all numbers from source, never memory)

- **Fireball (B)**: green, travels straight with no bounce/gravity (Mario's
  arcs and bounces) — different projectile physics, same article machinery.
- **Super Jump Punch (Up-B)**: point-blank "fire" sweetspot — one large hit
  instead of Mario's multi-hit rise; sourspot is a near-whiff.
- **Luigi Cyclone (Down-B)**: mash-rise behavior and hit pattern differ from
  Mario Tornado.
- Physics: floatier, lower traction, different jump/air values; several
  normals share Mario's frames with different parameters, some differ.
- Taunt has a hitbox (the famous kick) — verify in source; players know it.

## Assets & audio

Own model/textures (not a palette of Mario), own voice bank, announcer
"Luigi!", 4 costumes, CSS portrait/icon. Item-hold anim set baked per P2-3
pipeline rule.

## DS notes / risks

- The pipeline must express "same state machine, divergent data + a few
  divergent states" without forking Mario's code — this fighter's real
  deliverable is that mechanism.
- If Luigi needs hand-written one-offs beyond declared divergence points,
  stop and fix the pipeline before DK.

## Acceptance

- [ ] Move inventory sweep vs `ftluigi` data (all states visited, frame data
      equivalent).
- [ ] Fireball/SJP/Cyclone behavior verified against source-derived traces.
- [ ] CPU behavior at all levels equivalent (shared CPU tables + Luigi
      entries).
- [ ] Budgets: VRAM/RAM/sound within P2-2 per-fighter budget.
- [ ] Stress-config measurement banked; CSS slot live; owner feel pass.
