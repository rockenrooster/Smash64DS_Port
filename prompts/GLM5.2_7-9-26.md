Plan: Fix white stage objects + rainbow/broken fighters (canonical HW renderer)
Root causes (evidence-verified, not yet runtime-confirmed)
White stage objects — ndsRendererHardwareUseTexture (src/nds/nds_renderer.c:1893-1919) returns FALSE on one of 5 paths, but no counter exists for any of them. Only BindTexture failures are counted, so the reported reject0 is blind to white objects. The decorations' combine modes live in opaque binary DL blobs (decomp confirms no C-level gsDPSetCombine), so only runtime instrumentation can tell us which path they hit. Most likely: a later SETCOMBINE clobbering the TEXEL0-referencing word (lines 1014-1015 overwrite unconditionally), OR a decoration DL that never raises TEXTURE_STATE_ON (only G_TEXTURE with texture_on!=0 sets it, line 697).

Rainbow/broken fighters — the projected-submit fallback (nds_renderer.c:3742-3748) diverts z-buffered fighters onto ndsRendererHardwareClipVertex (nds_renderer.c:3451-3468), which does x=(x<<12)/w with only a w==0 guard, no near-plane clip, then saturates to ±32767 (nds_renderer.c:1643-1654). Vertices at/behind the near plane wrap to screen corners, stretching triangles across the screen. The gNdsRendererProfileHWVertexSaturateCount counter exists but is not surfaced in any doc/marker.

Why diagnostics-first (not jump to fixes)
Per systematic-debugging Phase 1 and AGENTS.md ("ban proof code whose only purpose is to rerun one branch"; "remove temporary probes before handoff"): the combine/clip fixes are non-trivial and I must confirm WHICH UseTexture path and WHETHER saturation is the fighter cause before writing them. The diagnostics below are temporary probes, removed or graduated before handoff.

Phase 1 — Diagnostic instrumentation (both tracks)
1a. Per-path counters in UseTexture (src/nds/nds_renderer.c:1893-1919) Add 5 profile globals (in src/port/diagnostics.c, reset in taskman_seam.c): gNdsRendererProfileUseTextureFalseNull/NoStateOn/NoCombine/Decal/NoTexel0. Increment one at each early-return. Also capture, once per path, the offending texture_combine_w0/w1, texture_state_flags, and texture_mask into one-shot snapshot globals so the combine word can be decoded offline.

1b. Fighter clip-path saturation surfacing Surface gNdsRendererProfileHWVertexSaturateCount and the raw/HW vertex min/max ranges (already tracked at nds_renderer.c:1656-1698) into a new RENDER_CLIP marker line in docs/DIAGNOSTIC_REFERENCE.md and into the RENDER_PROFILE/BPLAY_PACE verifier output. Goal: confirm fighters saturate.

1c. Build + run canonical, read evidence .\scripts\verify-battle-playable-realtime-harness.ps1 -DelaySeconds 3 (the canonical -canonical-hwtri target with live_input). Capture the new counters. Decode the snapshot combine words against the G_CCMUX_* bit layout (decomp/sm64-nds/include/PR/gbi.h:451-471, layout per nds_renderer.c g_setcombine equivalent).

Exit criteria for Phase 1: know (a) which UseTexture path white objects hit and what their combine word decodes to, and (b) whether fighter vertices saturate. Do NOT write fixes in this phase.

Phase 2 — Fix white stage objects (Track A)
Driven by Phase 1 evidence. Likely fixes, smallest first, each source-backed:

If path "NoStateOn" (decorations never raise TEXTURE_STATE_ON): the N64 RDP treats a tile as textured once SETTILE+SETTIMG+LOADTILE arm a rendertile, even without an explicit G_TEXTURE on-field. Honor NDS_RENDERER_TEXTURE_SETTILE | SETTIMG | LOADTILE (already tracked in texture_mask) as implicit texture-on when a rendertile is loaded. Cite decomp/sm64-nds draw path (objdisplay.c:1453-1462 uses tile 0 as rendertile unconditionally).
If path "NoTexel0" due to combine clobbering: stop unconditionally overwriting texture_combine_w0/w1 (lines 1014-1015) when a TEXTURE command has armed texturing — preserve the last TEXEL0-referencing combine, or snapshot combine per-primitive at triangle time rather than at SETCOMBINE time.
If the combine genuinely references TEXEL1 or is a 2-cycle TEXEL0→COMBINED chain: broaden CombineUsesColor to match NDS_RENDERER_CCMUX_TEXEL1 and the COMBINED-feed case already handled in OutputUsesColor (lines 1807-1819) — apply the same 2-cycle awareness to the line-1911 test.
Verify: rebuild canonical, assert the white-object count drops and assert-melonds-top-visible.ps1 non-white/non-green detail fraction rises (one-way ratchet tightens). Capture before/after PNGs into artifacts/visibility/.

Phase 3 — Fix rainbow/broken fighters (Track B)
3a. Near-plane triangle clipping in the projected/clip submit path (nds_renderer.c:3451-3468 and the per-vertex submit at 3535-3602). Replace per-vertex post-divide saturation with pre-divide near-plane triangle clipping: for each submitted triangle, classify vertices by w against a small positive near epsilon; clip the triangle against w=epsilon using Sutherland-Hodgman, interpolating position/UV/color across the new edge vertices; emit 0, 1, or 2 resulting triangles. This is the standard fix for "behind-camera vertices stretch across screen." Keep the existing w==0 guard as a final safety net. Remove or downgrade the post-divide ClampS64ToV16 saturation (it currently masks the real bug).

3b. Confirm against the DS hardware matrix path. Separately validate whether the raw z-buffered path (ndsRendererLoadHardwareMatrices 3368-3437 + glVertex3v16 at 3584) can be repaired so the fallback is unnecessary — but only as investigation; do NOT remove the fallback until the matrix path is pixel-proven. Document findings in docs/HW_RENDERER_VISIBILITY_FINDINGS.md.

Verify: rebuild canonical, confirm gNdsRendererProfileHWVertexSaturateCount drops toward zero, the Test-FighterDetailPixel fighter-region fraction rises, and a capture shows coherent (non-stretched) fighter silhouettes. Tighten the fighter-region ratchet.

Phase 4 — Graduate, doc, verify, snapshot
Remove temporary Phase-1 probes that only confirmed root cause; keep any that become permanent regression markers (e.g. the UseTexture per-path counters are genuinely useful long-term — propose graduating them into RENDER_TEXTURE).
Update docs/STATUS.md, docs/HANDOFF.md, docs/HW_RENDERER_VISIBILITY_FINDINGS.md, docs/KNOWN_ISSUES.md to reflect the actual post-fix visual state honestly (and, separately, reconcile the over-claimed "input works" framing — though the input fix itself is out of scope for this plan).
Static checks: check-docs, check-architecture, check-harness-registry, check-gbi-decode-fixtures.
Tiered verify: verify-dev-fast -Build, then verify-boundary, then RegressionCore sharded.
Final action only: .\scripts\New-Smash64DSSnapshot.ps1 -Mode Lean.
Scope & sequencing note
Phase 1+2 (white objects) is a contained, verifiable win suitable for one session. Phase 3 (fighter near-plane clipping) is the larger documented renderer-debt slice and can land as a follow-up if the session runs long. I recommend committing Phase 1+2 as one verified increment and Phase 3 as the next, rather than bundling — each must be verifier-backed per AGENTS.md.

Out of scope (flagged, not fixed here)
Dead input in shipped ROM (scripted playback default-on) — separate, higher-confidence fix.
Misplaced fence (GL_TRANS_MANUALSORT + no app sort) — separate.
Doc integrity for the input claim — noted for reconciliation in Phase 4 but no code fix.