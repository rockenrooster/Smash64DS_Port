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

### `FTAttributes` normalizer — landed 2026-08-25 (row P2-3f12)

He shipped without one from the day he landed. His `FTAttributes` sits at
**0x580** — `ftdata.c`'s `FTData dFTLuigiData` field 24, the same field that
gives Mario 0x428 and Fox 0x46c, and `221_LuigiMain.c` agrees twice over. The
O2R payload is big-endian and the loader's blanket u32 byte swap reverses the
two u16 lanes inside the six mixed-width attribute words, so with no arm in
`ndsRelocNormalizeFighterAttributesFile` he loaded:

| lane | shipped (no arm) | source |
|---|---|---|
| `dead_fgm_ids[0..1]` | 292, 427 | **427** (`LuigiDead`), **292** (`MarioDeadSlam`) |
| `deadup_sfx` / `damage_sfx` | 422, 420 | **420**, **422** |
| `smash_sfx[0..2]` | 417, 416, **0** | **416, 417, 418** |
| `itemthrow_vel/damage_scale` | 100, 100 | 100, 100 (identity) |
| `heavyget_sfx` | **0** | **426** |

**"Copy Mario's arm" would have been wrong on nine of the ten values, and
"substitute Luigi for Mario in the names" would have been wrong on one more:
his dead-slam is MARIO's `nSYAudioFGMMarioDeadSlam` (292), not a Luigi id.**
His smash triple *is* the ordinary Smash1..3, unlike Captain Falcon's
`{Smash3, Smash2, JumpAerial}` — which is exactly why neither fighter's arm can
be derived from the other's shape.

**What it was costing: nothing audible, and one phantom.** Luigi has exactly
two cues in the FGM pack (`nSYAudioVoiceAnnounceLuigi`,
`nSYAudioVoiceLuigiFuraFura`), so 416/417/418/420/422/426/427 all fail closed
whichever lane they land in. His KO sounds the same either way — both
`dead_fgm_ids` are queued unconditionally and only 292 is packed. The one
observable difference is `smash_sfx[2] == 0`: one smash-voice roll in three
asked the FGM backend for **id 0**, which is not a source cue at all, and it
entered the miss ring as a phantom — precisely the false "missing pack entry"
signal the next audio row would have had to explain. `heavyget_sfx == 0` is
latent the same way and worse: 0 passes the source's `!= nSYAudioFGMVoiceEnd`
guard, so once `it/itmain.c` lands a heavy pickup would request id 0 instead
of skipping.

Lanes measured from the staged NitroFS payload with Mario/Donkey/Captain as
landed-arm controls, ordinals independently re-derived by compiling
`gm/gmsound.h` with `-DREGION_US`, readers and the landed arm both verified on
the linked ELF:
`artifacts/verification/2026-08-25_p2-3f/luigi-ftattributes-lanes.txt`.

**Confirmed at runtime by P2-3f13's probe.** A four-kind shell match with Luigi
as the human reads `nSYAudioVoiceLuigiDamage` **422** in the FGM miss ring,
three times — 422 is the post-normalizer value of `attr->damage_sfx`, and
before the arm the same field read 420 (`LuigiDeadUp`), so the ring would have
named 420. No phantom id 0 appears either.

**His voices are the next gap.** 422 and 427 (`LuigiDead`) are in that ring
because none of his voice bank is packed — he has exactly two cues
(`nSYAudioVoiceAnnounceLuigi`, `nSYAudioVoiceLuigiFuraFura`). His down-bounce id
is unpacked too. Shape the work on the Donkey/Captain banks in
`scripts/sfx/render-audio-fgm-phase-pack.py`.

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
