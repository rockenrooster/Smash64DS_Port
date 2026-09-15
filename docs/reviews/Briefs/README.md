# Smash64DS: runtime candidate patches and researched fix briefs

This revision replaces the documentation-only handoff with **five actual C/Python
source candidate patches**, **one executable offline gradient experiment**, and
**36 updated issue documents** (35 reports plus the empty Captain Falcon heading).
All target fixes remain unaccepted until the required ROM and owner proof.

## Start here

[Research findings and stale diagnoses corrected](research/RESEARCH_FINDINGS.md)
summarizes the decisions that changed the candidate fixes.

The source diffs are in `runtime/`. The individual numbered briefs contain current
findings, concrete conditional runtime changes, applicable diffs, falsifiers and
symptom-specific acceptance. `research/issue_matrix.json` is the same mapping in
machine-readable form; it is not a replacement for the project's execution board.

Do not extract over a modified existing `Briefs` directory without reviewing the
changes. Preserve local owner work. Do not automatically apply every runtime
patch: they have different evidence, scope and configuration requirements.

| Candidate | Actual source change | Important limit |
|---|---|---|
| [R01](runtime/R01_impact_wave_source_depth.patch) | Cache/emit real projected ring Z instead of per-triangle painter Z | Full-alpha depth writes and native near clipping still need separate proof |
| [R02](runtime/R02_effect_initial_z.patch) | Restore source initial Z state for both effect-model call variants | Broader effect-sibling audit required; not a global weapon/stage change |
| [R03](runtime/R03_css_reset_image_lifetime.patch) | Invalidate native image bindings before three CSS load/reset paths | Defensive lifetime repair, not a hover/audio/roster fix |
| [R04](runtime/R04_link_foreign_tables_without_kirby.patch) | Add missing Link foreign-table AND light-preamble routing in disabled-Kirby helpers | Applies to that configuration; does not establish cause in the reported ROM |
| [R05](runtime/R05_data_native_surfaces.patch) | DATA source-art generator + native surface rendering + inventory | Asset rebake/staging required; child screens remain separate |

[E01](experiments/README.md) implements tested offline alpha-isoband splitting for
a bounded Zebes gradient experiment. It is not wired into the stage producer or
the ROM. Its object-space interpolation is not proof of N64 screen-space shading.

## Per-issue `.patch` handoff files

`issue_tasks/` contains 36 task-brief patches for the requested per-issue Codex
handoff. They install the researched Markdown, including potential runtime
repairs and source-candidate references. **They are not engine fixes.** The same
Markdown is already present in the ZIP, so do not apply a task patch over it.
Only `runtime/*.patch` changes target source or its producers. Load the matching
brief and its required sources, not all 36 investigations or the entire history.

## Read-only applicability check

From the prepared repository root:

```powershell
python .\Briefs\tools\check_candidates.py --repo . --candidate R01
```

It prints the actual HEAD, checks unique old/new contexts and relevant dirty paths,
and runs `git apply --check --whitespace=error`. It never applies a patch, resets
Git or edits files. A context mismatch means rebase/review, not force apply.

After reading the corresponding brief and reconciling applicability:

```powershell
git apply --check --whitespace=error .\Briefs\runtime\R01_impact_wave_source_depth.patch
git apply .\Briefs\runtime\R01_impact_wave_source_depth.patch
```

This is `git apply`, not `git am`. Do not apply the same shared patch once per
linked issue. Rebase each subsequent candidate against any already-applied edits.

## Host checks supplied

```powershell
python .\Briefs\tests\test_candidates.py
python .\Briefs\tests\test_alpha_isobands.py
```

The delivered results record 12 candidate tests and 8 analytic gradient tests,
with no skipped tests in this environment. Details and limits are in
[research/VALIDATION.md](research/VALIDATION.md). Host C tests use exact changed
helper fragments with mocked tables/blitter; patch apply/reverse tests use
reconstructed preimage fixtures. They are **not** a complete checkout/ARM build,
original-asset regeneration, DS renderer emulation or visual acceptance.

## Codex handoff

```text
Read Briefs/COMMON.md, Briefs/README.md, the matching issue brief and the
current repository owners. Start with issue 01's native depth defect.

Revalidate candidate applicability against the actual working tree and build
configuration. Use the read-only candidate checker; never force or bulk apply.
Implement/rebase the applicable source changes and complete the remaining
source-derived native requirements. R02 needs a sibling depth/material audit.
Retain already-landed fixes rather than repeating stale diagnoses.

For every other reported issue, follow its concrete conditional repair and
falsifier. Do not turn an unresolved hypothesis into a blind patch. The source
assets and natural ROM path decide missing roots, materials and lifetimes.

Preserve unrelated dirty work; never edit decomp or enable non-native rendering.
Serialize builds and shared generated inputs. Collect natural-path positive
native engagement, required pixels/audio, lifetime/resource and cadence proof.
Report implemented, host-tested, ROM-tested and accepted portions separately.
Do not mark FIXED from the brief, a host test, compilation or zero counters.
```

## Issue map

01. [01 — Impact wave: restore fighter occlusion](01_Impact_Wave_Depth.md) — R01, R02.
02. [02 — Main menu: 1P mode selection](02_Main_Menu_1P_Route.md) — conditional repair.
03. [03 — Campaign: invisible 1P character select](03_Campaign_1P_CSS_Invisible.md) — conditional repair.
04. [04 — Data menu: replace the blue-only presentation](04_Data_Menu_Blue_Screen.md) — R05.
05. [05 — VS CSS: enable Kirby after proving the native closure](05_CSS_Kirby_Selectability.md) — R03.
06. [06 — VS CSS: Jigglypuff selection and natural native proof](06_CSS_Jigglypuff_Selectability.md) — R03.
07. [07 — VS CSS: Ness selection through the normal shell](07_CSS_Ness_Selectability.md) — R03.
08. [08 — VS CSS: gate-opening stalls and flashing](08_CSS_Gates_FPS_Flashing.md) — R03.
09. [09 — VS CSS: missing Yoshi 3D preview](09_CSS_Yoshi_Preview.md) — R03.
10. [10 — VS CSS: music pauses or restarts on preview changes](10_CSS_Preview_BGM_Interruptions.md) — R03.
11. [11 — VS CSS: delayed preview after cursor hover](11_CSS_Preview_Hover_Latency.md) — R03.
12. [12 — Yoshi Up-B: egg and shell presentation closure](12_Yoshi_UpB_Egg_Shells.md) — conditional repair.
13. [13 — Yoshi grab/throw: restore source model-part programs](13_Yoshi_Grab_Invisible.md) — conditional repair.
14. [14 — Yoshi neutral-B: body and egg visibility](14_Yoshi_NeutralB_Egg_Invisible.md) — conditional repair.
15. [15 — Yoshi intro: visible entry egg and transition](15_Yoshi_Intro_Egg.md) — conditional repair.
16. [16 — Link intro: translucent column effect](16_Link_Intro_Column_Alpha.md) — R02.
17. [17 — Link neutral-B: throw/catch body visibility](17_Link_NeutralB_Invisible.md) — R04.
18. [18 — Link Up-B: complete spin-attack effects](18_Link_UpB_Effects.md) — R02.
19. [19 — Pikachu neutral-B: transparent soft-edged electric effect](19_Pikachu_NeutralB_Alpha.md) — conditional repair.
20. [20 — Pikachu down-B: missing Thunder and intermittent crash](20_Pikachu_DownB_Missing_Crash.md) — conditional repair.
21. [21 — Combat: missing electrical damage presentation](21_Electric_Damage_Effects.md) — conditional repair.
22. [22 — Pikachu strong side-A: missing attack effect](22_Pikachu_Strong_SideA.md) — conditional repair.
23. [23 — Samus shield roll: prove and repair morph visibility](23_Samus_Shield_Roll_Invisible.md) — conditional repair.
24. [24 — Samus down-B: morph ball and Bomb visibility](24_Samus_DownB_Invisible.md) — conditional repair.
25. [25 — Samus charge/shot: correct overlap with the arm cannon](25_Samus_Charge_Shot_Gun_Occlusion.md) — R02.
26. [26 — Captain Falcon: preserve unspecified current report](26_Captain_Falcon_Scope_Intake.md) — scope only.
27. [27 — Peach's Castle: restore textures on recovered roof geometry](27_Peachs_Castle_Roof_Texture.md) — conditional repair.
28. [28 — Zebes acid: preserve soft texture/blend transitions](28_Zebes_Acid_Soft_Blend.md) — E01 experiment.
29. [29 — Zebes ground lights: taper to transparency](29_Zebes_Ground_Light_Gradient.md) — E01 experiment.
30. [30 — Mushroom Kingdom: texture the restored left platform](30_Mushroom_Left_Platform_Texture.md) — conditional repair.
31. [31 — Yoshi's Island: transparent rotating texture cards](31_Yoshis_Island_Rotating_Card_Alpha.md) — conditional repair.
32. [32 — Yoshi's Island: remove opaque sparkle quads](32_Yoshis_Island_Heart_Sparkle_Alpha.md) — conditional repair.
33. [33 — Yoshi's Island: restore main platforms and floor/path](33_Yoshis_Island_Missing_Platforms_Floor.md) — conditional repair.
34. [34 — Sector Z: intermittent crash during fighter intro](34_SectorZ_Intro_Crash.md) — conditional repair.
35. [35 — Saffron City: restore the garage-door state/animation cycle](35_Saffron_Door_Cycle.md) — conditional repair.
36. [36 — Saffron City: native Pokemon attack effects](36_Saffron_Pokemon_Attack_VFX.md) — conditional repair.

## What was not done

No ROM was built/run, no packed original assets were independently regenerated,
no repository changes were committed/pushed, and no complete-working-tree patch
application was claimed. The five diffs are real candidate code; the remaining
conditional proposals are not represented as already-implemented patches.
