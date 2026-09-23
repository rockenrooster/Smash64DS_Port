# P2-2p8 phase specs (2026-09-23)

Implementation specs for the phases of `docs/p2/FOUR_FIGHTER_30FPS_ARCHITECTURE.md`,
written by read-only investigations against the tree at `5c594a59810`. Each spec
labels its figures MEASURED (with the evidence file) or ESTIMATE.

| file | phase | summary |
|---|---|---|
| `phase1-spec.md` | 1 Fighters (A1) | lean fighter path: kept display head, 4-field tuple, ARM ITCM joint kernel (Q43.20 from 16.16 locals), LOAD4x3 + P' lists, TEXIMAGE/PLTT tint patches over resident tiles (never a re-record), texture admission before GO, host-generated lists, deletion list; first slice = Samus LOW program 0 on an adopted packet with routes 0-3 and an exact oracle |
| `phase2-spec.md` | 2 Stage + MISC (A1) | stage compiler per run class (almost no stage triangle needs CPU per-vertex work; the "non-rigid" counts come from placeholder masks), `near_inside` reuse bug confirmed in code, painter depth patched per frame (cannot be baked), MISC fully split by counters, native draw list on the existing camera walk, resident DamageSlash |
| `stage_census.py` | 2 | per-stage triangle census over the generated stage tables (`src/nds/nds_native_stage_select.inc`) |

The Phase 3 motion-format experiment is in `../2026-09-23_p2-2p8-mf-experiment/`
(verdict: candidate B, structural re-encoding + static Huffman, 0.382x on the worst
four-kind roster, byte-exact on 1,570 / 1,570 clips).
