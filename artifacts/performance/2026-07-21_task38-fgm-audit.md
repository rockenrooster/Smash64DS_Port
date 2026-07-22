# Task 38 FGM audit

Verdict: **BLOCKED** for exact missing special cues; hit-sound fidelity is
**PENDING OWNER LISTEN**. No sample substitution was made.

## Source-to-speaker chain

BattleShip motion commands and `dFTMainHitCollisionFGMs` request an FGM through
the common position-FGM seam (`decomp/BattleShip-main/decomp/src/ft/ftmain.c:22-31,
2107-2115`). `scripts/render-audio-fgm-phase-pack.py` resolves the source UCD,
wavetable, schedules, forks, loops, pitch, volume, and custom-FX constraints into
the resident DS IMA-ADPCM pack. `src/nds/nds_audio_fgm.c` then resolves the exact
ID, allocates a bounded handle, and starts a DS hardware channel; unsupported IDs
return `NULL` without playing another sample.

Pack identity: 27 entries, 20 unique samples, 128,196 / 131,072 resident bytes,
2,876 bytes headroom (`assets/audio/fgm_phase_pack_ima.json`).

## Manifest inventory

| Class | IDs and names | Result / fidelity debt |
|---|---|---|
| Exact phase/voice | 626 PublicExcited; 470 Three; 469 Two; 467 One; 490 Go; 74 FoxLanding; 363 FoxJumpAerial; 364 FoxEscape; 372/373/374 FoxSmash1/2/3; 430 MarioSmash2; 439 MarioDead; 292 MarioDeadSlam; 370 FoxDead; 289 FoxDeadSlam; 300 FoxDownBounce; 303 MarioDownBounce; 77 MarioLanding | Included |
| Exact special | 215 MarioSpecialN | Included; only qualified special/activation cue |
| Exact primary only | 154 DeadExplodeL; hits 40/38/37 Punch S/M/L and 34/32/31 Kick S/M/L | Included primary voice; omitted fork programs 685, 655, 654, 653, 658, 657, 656 remain explicit debt |
| Excluded voices | 375 FoxDamage; 429/431 MarioSmash1/3; 435 MarioJump; 440 MarioDamage | Pitch/automation debt; fail-closed |
| Excluded attack/activation | 19 Catch; 41/42/43 LightSwing L/M/S; 185 FoxSpecialN; 186/187 FoxSpecialHi start/fly; 189 FoxSpecialLwStart; 190 FoxAttackAirLw; 217 MarioSpecialHiJump; 218/219 MarioUnkSwing1/2 | Loop, pitch/volume schedule, fork/custom-FX, source-rate, overlap/handle, and/or resident-cap blockers; fail-closed |

## Special-move source census

| Fighter/move | Source event | Exact-pack result |
|---|---|---|
| Mario neutral-B | ID 215, `relocData/202_MarioMainMotion.c:1339` | Included |
| Mario Up-B | ID 217, `202_MarioMainMotion.c:1358` | Excluded |
| Mario Down-B | IDs 218/219, `202_MarioMainMotion.c:1295,1302,1395,1421` | Excluded |
| Fox neutral-B | ID 185, `relocData/208_FoxMainMotion.c:1450,1464` | Excluded |
| Fox Up-B | IDs 186/187, `208_FoxMainMotion.c:1488,1534,1552,1564,1582` | Excluded |
| Fox Down-B | ID 189, `208_FoxMainMotion.c:1622` | Excluded |
| Fox reflector hit | ID 188, `208_FoxMainMotion.c:1642` | Not a start cue; separately routed by the reflector collision path |

## Runtime request census

A profile-agnostic 16-ID volatile miss ring now records every unsupported request
and coalesces repeated IDs. The Boundary GDB harness requires and prints the ring
whenever audio assets are imported; it deliberately does **not** require an empty
ring because exact unsupported cues remain fail-closed.

Pre-gate frame-212 marker:
`AUDIO_FGM_MISS=0,0,0:0,0:0,0:0,0:0,0:0,0:0,0:0,0:0,0:0,0:0,0:0,0:0,0:0,0:0`.
This proves the marker/parser path, not the requested full special-move census.
DevFast and Boundary both stopped before the one-minute route at the unrelated
Task-36 frozen-water assertion (`RENDER_TEXEL1=2,2,0,...`; mode-2 verifier
expected all zero), so no final Boundary census is claimed.

## Capacity decision

Simply enabling selectors is unsafe. ID 34's smallest exact dry fused-fork form
adds 18,436 bytes and totals 146,632, exceeding the cap by 15,560; a paired form
adds 24,912 and exceeds it by 22,036. Several specials additionally need looping,
custom FX, overlapping voices, or source rates above the current representation.
Therefore Phase C stops at the resident/representation gate.

Symptom 1 remains blocked for exact representation/capacity. Symptom 2 cannot be
closed from manifests or counters: the owner's batched melonDS listen pass remains
the acoustic oracle for the six primary-only common hit sounds.
