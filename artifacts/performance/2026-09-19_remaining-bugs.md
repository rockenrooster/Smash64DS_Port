# Remaining owner bug list — serial integration, 2026-09-19

Owner requests completion of the full non-deferred diagnosis and a playable
candidate, not another checkpoint-only handoff. No subagents. Castle/Zebes
remain explicitly deferred. The execution board remains the only live queue.

Baseline: `bd645a1b03e` plus preserved owner working-tree changes.
Yoshi/Samus battle-pack repair and prior proof are in
`2026-09-19_yoshi-battle-pairs.md`; reuse those results without claiming that
all Yoshi actions or the full verifier passed.

## VS Options policy repair

Item Switch availability now ignores the progression lock for this menu only;
no save data or unlock bits are changed. Damage taps remain one point, held
repeat events move three points, with the same inclusive 50..200 wrapping range.
Other rows still act once per accepted repeat; global input timing is unchanged.
Source contract: BattleShip mnvsoptions.c, Item Switch entry and Damage bounds.
The source-executing host menu check passes, including all 151 starting Damage
values and both tap/held directions. Natural menu/cadence proof remains due.

## Playtest artifact

Building normal `smash64ds` configuration with a separate output directory:
`builds/remaining-bugs-playtest/`. The accepted root ROM is not overwritten.
This intermediate candidate is for playtesting, not full-list acceptance.
Build completed: 316 native-only link inputs; natural controls, fast logic 0,
menu walk 0. Boot/title passes with zero resource/scene/native errors.
ROM SHA-256: `09E0E134FF5EF2F48B8EB6E0CF6BFCEB6F5B3279F43C0F5E42A8C336B1E8D469`.
Log: `builds/2026-09-19_remaining-playtest-boot.log`.

## Sing native owner

The source PurinSpecial2 file 351 has four drawable roots, 0x1f38/1fd0/2048/2090.
The new producer pins the complete O2R identity and verifies source relocation,
vertex/triangle, texture and material commands. It bakes eight triangles and
immutable state; source DObj/PRIM animations and lifetime remain authoritative.
Sibling inherited render state is made explicit so an alpha-zero child cannot
make a later child inherit an unrelated draw's combiner. No runtime DL interpreter.
Generator check and native-only diagnostic build pass. Natural Up-B (status 232)
engages all four post-triangle counters; both fighters draw, native/reject/OOM 0.
The GO-free capture at source time >=140 shows the live ring/notes. Evidence:
`artifacts/visibility/2026-09-19-purin-sing-visible.png`.
Diagnostic ROM `3BF099BFC6B38139A713B92A3980965A30AC20FBCBBF0AF55744E951FE893CB7`,
ELF `2ECE95E027778F9B03ABEA7384BD3A2A6CC56022FD5AFD3BC94D541481D186EB`.
This is not yet included in the playtest ROM. Resource sibling checks and
integrated qualification remain due; no FPS/P50/P95 claim from breakpoint probes.

## CPU fixture first divergence

The newer CSS roster tour directly started battle after its final preview,
discarding the ordinary scripted CPU-arrow leg. The level remained the preset's
3; the existing verifier correctly requires the menu-selected 2. The tour now
restores its pair and restarts ordinary controller playback from the unchanged
cursor. No level assignment, relaxed verifier expectation or shipping input change.
Source-executing completion/re-entry check added; integrated runtime proof due.

## Sing depth and resident-code budget

Owner requested the same depth treatment as guard/shield. Sing now uses the
existing fixed-point foreground projection and per-triangle painter depth;
`2026-09-19-purin-sing-shield-depth.png` captures the result with all four roots
engaged. A shared fixed-quad GX submission kernel replaces duplicated code in
Sing, Ness PK Thunder, Yoshi entry/Egg Lay, Samus Charge Shot and Pikachu's air
jolt/effect. Binding/material/setup/restoration stays with each source owner.
The source-executing kernel test checks raw/painter coordinates, triangle order,
depth sequence, state restoration, alpha-zero and texture-bind failure.
Menu and kernel host checks pass. First two mock failures were test-local name
shadowing; fixed without changing production behavior.

First shared build text falls 1,796,612 -> 1,793,492 bytes, with BSS down 32.
Ness/Sector Z (stage **1**) starts and draws both fighters at frame 240 with
arena 921,088 / minimum 2,756 bytes, zero failure/reject; baseline margin restored.
Further shared-quads build text is 1,790,436 bytes, arena 925,184.

Correction: the probes named `ness-sector-sing-regression` and
`ness-sector-quad-shared` selected **stage 7, Saffron**, not Sector Z. The original
Yoshi-pack baseline ROM `4AF1224F...A0559D44` also fails Ness/Saffron startup:
168 bytes requested, 128 free, arena 921,088. This is a pre-existing case,
not proof of a Sing regression. The newer 925,184-byte arena advances further
but still fails a 1,332-byte request with 1,104 free. The same fixed-quad
submission is now shared by the four Saffron monster body owners to recover
code residency without removing content; their build/runtime proof is pending.

Yoshi's entry maker is under focused diagnosis: at GO the body draws 320
triangles, native failures zero, but no entry-egg draw witness fired. The longer
entry wait reached Results and then failed its 132,000-byte transition allocation
with 42,416 free; no completion claim from that run. A bounded maker/descriptor
witness completed: its maker returned NULL because the source's object cap was
48 with 48 active, after free arena space dropped below the unchanged 25 KiB
reserve. This explains silent missing effects without native rejection.

`ndsResetStartupDiagnostics` occupied 33,944 resident code bytes. An Os-only
experiment saved four bytes and was discarded. The retained representation
encodes 1,742 consecutive aligned integer stores in 50 constant tables, preserving
their exact order and values (reverse-expansion checked against the original).
Target compile-time guards reject narrow/unaligned/noninteger destinations;
host tests cover values, signed words, zero length and the negative guards.
Final table build text is 1,780,436 B: 6,216 B below its immediate predecessor.
Yoshi's arena rises to 933,376 / low-water 27,804; its entry egg now allocates,
reaches the native owner and emits triangles, with zero native failures.
The collector initially misclassified its intentionally hidden entry body as a
missing fighter; that assertion now applies after the source GO clock starts.
The existing Wait/Run negative-output check remains intact.

The first Sing depth experiment used the generic entry renderer's painter path.
It is superseded: the **live guard** uses a camera-facing bias, with a 150-unit
floor or half live radius. Sing now uses that same rule in fixed point while
retaining ordinary depth tests. Host checks cover floor, scaled radius, off-axis
eye direction and the zero-distance case. The unused painter option has been
removed from the shared quad kernel. Fresh entry/Sing probes pass:
`2026-09-19-yoshi-entry-native`, `2026-09-19-yoshi-entry-grown` and
`2026-09-19-purin-sing-guard-bias`. The grown capture visibly shows the source
green-spotted intro egg; the first submitted frame alone was too small to judge.
All four Sing roots engage with the source animations and guard-style bias.
Four focused host tests pass (menu suite, quad, guard-depth math, reset words).
Full list, lifecycle and integrated qualification remain open.

## Owner intake — BUGS.md, 2026-09-19 22:05 local

The owner removed the original Yoshi visibility/grab/intro rows; keep that closure.
New Yoshi reports are missing Up-B explosion pieces and undersized Neutral-B eggs.
Results not reachable after Time Up now explicitly matches the observed 132,000-
byte transition allocation failure. Two new Castle collision reports are active;
only its roof material and the Zebes material reports remain deferred.
Do not replay the removed Yoshi reports as unresolved acceptance work.

Results is next: `src/import/battleship_lbtransition.c` still calls the source
transition allocator, requesting a 300x220x2 buffer with only 42,416 bytes free.
Investigate the existing DS transition/presentation route before another memory
or generic no-op workaround. The lower HUD also uploads ~10 KiB of static glyph
data into sub OBJ VRAM yet retains the duplicate main-RAM copy all match;
`scripts/menus/generate_battle_hud.py` -> `src/nds/nds_battle_hud.c` owns that
producer/upload seam. Streaming the immutable upload data during preparation is
the next bounded residency candidate. Neither change is implemented yet.

Score alerts use existing strong source functions, not the old weak stubs.
Scripts 0x43/0x44 use efcommon texture 32, two 64x16 frames, and only +/-1 KO
callers exist. Reuse their authoritative particles/animation when moving output
to the bottom HUD; do not invent a second score timer or poll score totals.

## Updated normal-controls candidate

`builds/remaining-bugs-playtest-r2/smash64ds.nds`:
SHA-256 `59996FF34724DFCBB6FE906C7526CE78CECE37CC6F64748AEF73C7E577638612`.
ELF `FEA2DAFB85915B43BD8ED72A35895BE66EC777E846C548106204153EF763890F`;
matching config is alongside it. Normal `smash64ds` target, no menu walk or fast
logic; native-only packaging enforced. Boot/title PASS, all recorded boot,
resource, scene and native errors zero. Includes the menu policies, Sing depth,
shared quad budget and compact reset. Not a full-list completion or performance
acceptance; the root accepted ROM is preserved. Build/boot logs:
`builds/2026-09-19_remaining-playtest-r2-{build,boot}.log`.

## 2026-09-20 — Results full-motion data and HUD residency

Results still loaded raw fighter Main/Model files while battle used compact
full-motion packs. Extended the existing compact battle-data scene predicate
to Results, consistently across load selection, native address mapping, extern
fixups and ftmanager dependency publication. Preview packs remain CSS-only.
Six loader tests plus ten subtests pass, including Results bounds/mapping and
unrelated-scene passthrough. No transition omission or framebuffer workaround.

Full natural one-minute Mario/Fox match reaches Results tic 160: two visible
fighters, pack/extern/native failures 0, free minimum 52,656 B. Capture:
`artifacts/visibility/2026-09-20-results-compact.png`; ROM
`BF6DAF50E6F6F043C5CF7AAA18BC338D3ECC517B4F7881D053894C35D2F82848`.
The capture runner now uses the existing owned-window helper after a hidden
launch; its old early MainWindowHandle check failed before any guest diagnosis.
Return-to-CSS, other rosters and the final integrated profile remain due.

The HUD producer now emits a versioned NitroFS OBJ-graphics blob; the loader
uploads it once during HUD preparation through bounded 512-byte scratch and
the existing filesystem mutex. Header/size/read/EOF checks fail through native
resource reporting. The previous 10,368 graphic bytes are byte-identical
(SHA256 `7d0da7bfe2b26260afb84b74eb0aab9c821ce75d556847e5db4bdc853fc4436e`),
without a permanent duplicate main-RAM image.

Score presentation now routes efcommon texture 32 on its source interface
particle link to the lower HUD. Source particles retain event creation, position,
frame, shrink and lifetime; no new score timer or score-total polling. Its two
source 64x16 images are converted into sub-OBJ cells and appended to the blob.
A bounded per-present queue supports 16 simultaneous alerts and records overflow
as a failure. Host checks execute queue/OAM placement, sign frames, scale,
frame retirement and rejection; producer check preserves all prior HUD pixels.
Runtime score proof passes after a natural KO: source time 2,488, both signs
drawn (frame mask 3), two live alerts, four submissions / two OAM draws at the
first captured alert frame. Native/pack/arena errors and fighter rejects are 0;
arena 941,568, minimum 61,124 B on this Mario/Fox Dream Land case. Capture:
`artifacts/visibility/2026-09-20-score-alerts.png`. The signs appear at the source
HUD positions on the bottom screen, with source particle scale/position changes.
Host queue and blob checks pass. Latest diagnostic text is 1,771,284 B, data
262,936 and BSS 939,424. No FPS/P50/P95 acceptance from a breakpoint capture.

Normal r3 playtest build is in progress. Next confirmed native gaps are Kirby
Vulcan Jab and Pikachu Thunder. Important correction to the old diagnosis:
`efManagerKirbyVulcanJabMakeEffect` is also strong (`T`) in the current ELF; do
not add a duplicate source maker. KirbySpecial2 asset 348 has Vulcan roots
0x09b0 (branches to 0x09f0) and 0x0a78, a 14-vertex part plus a four-vertex card.
Its source RGBA32 16x32 image is at 0x0090; measure its actual color/alpha set
before selecting a native DS texture format. No Vulcan implementation change yet.
Measured: RGBA32 has 130 RGBA colors, 30 distinct nontransparent RGB5 colors,
and 64 distinct alpha values (0..252). Any conversion must preserve the visible
alpha ramp; do not silently use an opaque/RGB5A1 card.

Normal r3 candidate built and passed boot/title with zero recorded boot/resource/
native errors: `builds/remaining-bugs-playtest-r3/smash64ds.nds`, SHA256
`8450A5A2882F1C19630F87FFAC54BF064B6C2C400547064647034D7AE8D4A7E0`.
Matching ELF: `378C4D1E27384D1BC562219489EDD2D4C0B5DF05DBBFBF5B3D56AA84AA1B661A`.
This adds compact Results fighters and native lower-screen score alerts to the
previous candidate. The old accepted root ROM remains unchanged; full list and
final integrated qualification are still open. No jobs left running.

## 2026-09-20 — Kirby Vulcan native owner candidate

The source maker is strong and retained. New source-pinned AOT code handles
the two Vulcan roots: 12 source triangles plus the two-triangle intensity card.
The RGBA32 texture is partitioned into four disjoint eight-color A5I3 banks.
Each pixel belongs to at most one bank; decoded RGB5 and five-bit alpha exactly
match the source quantized to DS precision, with no extra blending between banks.
This preserves all 30 RGB5 colors and the alpha ramp; it is not an opaque card.
The source I4 second root uses white primitive RGB and intensity-derived alpha.
Geometry, source effect motion, facing and six-tic lifetime are unchanged.

Texture preparation is placed before the battle texture fence for Kirby slots,
on both loop owners. Generator/source checks and two palette/geometry tests
pass; native-only build passes. Natural jab input reaches both native roots
without rejection. The first delayed capture used cumulative counters and
missed the six-tic effect lifetime; a fresh-input capture after the GO overlay
was repeated. Both roots engage but the initial pictures did not clearly show
the six-tic effect. Source link 15 uses efDisplayCLDProcDisplay's no-Z layer;
the adapter's generic effect seed forcibly adds Z. The Vulcan owner now applies
that source layer rule and the existing native projected-depth path, without
changing the other effect owners. A fresh two-draw capture is running on the
new native-only build. The source-layer capture passes with both roots engaged
and a visible burst/glow: `artifacts/visibility/2026-09-20-kirby-vulcan-layer.png`.
Owner fidelity check and cost qualification remain open; the color mesh takes
four disjoint palette passes (48 triangles), plus the two-triangle glow.
No opaque fallback, arbitrary position offset or replacement spark was used.

Next: Pikachu head/trail share Model asset 341 root 0x94f8 and a live four-image
MObj; source ThunderShock is Special2 asset 347 roots 0x14b8/0x1598 (fixed CI4
and live IA8). The existing trail effect maker still does raw Model base+offset
construction, unlike the mapped Yoshi/Ness wrappers; compact-file construction
must be checked alongside the absent native owner. The before probe reproduced
NO_PROGRAM at root 0x94f8 during status 225, with two recorded native failures.

New producer/executor handles the three roots with source-pinned vertices,
triangles, state and live MObj images. The source's side-smash routine directly
calls ThunderShock; that is the reported side-A effect, not a guess based on
the old descriptor comment. Added mapped-descriptor wrappers for the existing
trail and shock makers; their motion/lifetime and source state remain intact.
Generator and three combined Vulcan/Thunder tests pass; native-only build passes.
The natural Down-B probe is running and requires separate head, weapon-trail,
and effect-trail engagement. The bolt path gets past the previous NO_PROGRAM,
then a hit exposes a separate fighter-native failure: Fox status 52 / motion 45,
detail High, validator code 3, 16 selected roots against 18 expected. Exact roots:
`6240 6320 6a20 6910 6b10 66b0 6a20 6910 6c10 6d30 6de0 6e90 6d30 6de0 6e90 7020`.
This is neither Fox's canonical high nor low vector. The source
`ftDisplayMainDrawAll` selects alternate `attr->skeleton[colanim.skeleton_id]`
display lists for electric color animation; the current native owner compiler
has no corresponding alternate program. Keep the reject gate. The owning follow-up
is source-derived skeleton programs for affected roster siblings, not a Fox-only
count bypass or hiding the effect. Side-smash independently passes and captures
visible sparks: `2026-09-20-pikachu-side-smash.png`, status 204, both Shock roots
engaged, no rejection. Down-B/electric-body coverage and final qualification stay open.

That capture also exposed a HUD streaming defect: bytes were correct but the
stack scratch buffer was dirty in ARM9 cache when DMA read it. Added the missing
DC_FlushRange before each bounded DMA copy. A source-executing upload test uses
separate CPU/DMA views and checks each chunk, truncation and alignment. Three
HUD producer/queue/upload tests pass. Fresh runtime pixel proof is running;
the r3 playtest artifact predates this cache-coherence repair and is superseded
once the next normal candidate is built. Do not treat r3's garbled glyphs as art.
Runtime pixel proof now passes: `2026-09-20-hud-dma-verified.png` shows clean
unchanged 0% digits and portraits, with zero native failures. Normal r4 is building.

Electric-skeleton continuation pointers: source `ftDisplayMainDrawSkeleton` in
`decomp/BattleShip-main/decomp/src/ft/ftdisplaymain.c` consumes
`attr->skeleton[colanim.skeleton_id]` by live joint id. The current source-driven
collector already selects those alternate DLs; native programs are missing.
Reuse `build_owner_root_programs` / `build_p2_owner_runtime_context` in
`scripts/fighters/generate_nds_native_owners.py`, with matching selection,
program bounds and owner-table dispatch in `src/nds/nds_renderer_assets.c`.
Mario/Fox retain a separate historical base pipeline: do not change its frozen
canonical roots to fit a damage pose. Derive the affected sibling programs from
the source tables, not the single captured Fox vector. No bypass of count/root
validation or suppression of electric damage is permitted.
Fox's source contract is explicit in `209_FoxMain.c`: `FTSkeleton[27]` at
Main+0x388, selected by skeleton-id 1; Main+0x460 has gate joint 12, the skeleton
pointer and a NULL sentinel. This is the authoritative derivation seed, not a
hand-authored copy of the runtime vector. NPikachu/NFox polygon variants with
NULL skeleton pointers should remain canonical rather than gaining invented data.

Normal r4 candidate: `builds/remaining-bugs-playtest-r4/smash64ds.nds`, SHA256
`A06E9D9ACFF0E6F0608BF9D549877A98FD340C310720FD9817294CD1A3713658`;
ELF `090FDD515B32C09A771B5F734C06CEB3D84466E9DAA8F819C3BB14D3C175F0DB`.
Native-only build and boot/title checks pass. This replaces r3 for playtesting
and includes the corrected HUD DMA upload, Vulcan and Pikachu side-smash owners.
Down-B is explicitly not closed: a hit can still stop on the missing electric
fighter program. No full-list, roster/lifecycle or performance acceptance claim.

## 2026-09-20 — electric-body native program integration

Source-derived Mario/Fox skeleton vectors now compile through the existing
fighter IR and compiler-layout NitroFS images, loaded at fighter construction.
Canonical exports are unchanged; exact root/count validation stays enabled.
The same skeleton image serves both detail levels. Mario costs 7,436 scene
bytes and Fox 6,740; unused fighters do not load those images. Program 0xfe
uses the live selected DObjs, independent matrix bindings and image-baked
normal/color words. Scene release clears its bound table aliases as usual.
Two focused tests pass: source/vector/geometry checks and execution of the
actual C selector, including both details, every wrong-root mutation, wrong
count, canonical restoration and invalid owner/program cases. Native-only
diagnostic build passes (315 actual link inputs); target proof is in flight
under `builds/remaining-bugs-electric-probe.log`, runner 8.

This is not the whole electric-effect repair: `ftParamCheckSetSkeletonColAnimID`
still admits only Mario/Fox. Source `ftparam.c` maps the remaining playables
to the common, balloon or Samus script family; those scripts/native bodies
must be restored together. Samus's skeleton uses two pre-matrix vertex-load
lists (0xacc0/0xad10), unlike its canonical body. Do not drop those loads or
reinterpret their unlit colors as normals. The current producer refuses that
unsupported pair rather than publishing incorrect geometry. Other source
skeleton vectors have been derived, not yet integrated or runtime verified.

## 2026-09-20 (cont.) — electric body closed, Results arena, arena census

Codex ran out of quota mid-batch; the same serial integration continues here.
Thunder's live-material mask fix (0x0200 head/trail, 0x1600 Shock) was already
regenerated; it is now built and engaged (roles 1|2, zero native failures).

**Electric-body reject, first divergence named.** Fox's alternate program
selected (0xfe) and aborted in its second epoch. Witness at the abort:
`texture_image == 0`, tile never set. The skeleton DLs carry their own
G_SETTIMG into shared asset **299** (palette 0x8/0x60, CI4 0x18/0x78); the
compact battle pack only closed textures over the canonical programs, so the
foreign-image resolver returned NULL and texture preflight rejected the whole
fighter. `generate_battle_core_packs._native_texture_roots` now also walks the
skeleton contexts; Mario/Fox `.ext` grow 96->320 / 60->284 B (224 B of foreign
texels each), `.fpc` unchanged. `native_skeletons.py` is a declared prereq of
the pack rule. Natural Thunder hit on Fox: program 254 draws 175 triangles,
`DIAG_DIRECT_REJECT` count 0, `DIAG_NATIVE` all zero, transport ok
(`2026-09-20-fox-electric-r6`, capture `artifacts/visibility/2026-09-20-fox-electric-r6.png`).

**Electric flash for the rest of the roster.** `ftParamCheckSetSkeletonColAnimID`
admitted Mario/Fox only, so every other fighter got no colour animation and no
ShockSmall sparks at all. Added the source's own no-skeleton family
(gmcolscripts.c:225-263, ids 0x10-0x13, what the source gives Master Hand /
Metal / Polygon bodies) and route every fighter without a resident electric
body to it. OWED and recorded in the code: native electric bodies for the other
ten fighters (0x14/0x18/0x1C families; Samus needs pre-matrix vertex loads),
each ~7 KB of arena. Runtime proof on a non-Mario/Fox victim is still owed --
see the arena section for why the Samus probe could not start.

**VS Results halt (Pikachu/Fox: 136 B wanted, 88 free).** Two causes removed:
(1) Results fighters no longer load electric-body images (they cannot be hit);
(2) the source transition photo -- a 132,000 B copy of `gSYFramebufferSets`,
which on DS only ever holds scmanager's uniform clear -- now aliases that
storage instead of allocating (`battleship_lbtransition.c`, static-asserted
extent, 4-aligned self copy of a uniform buffer, identical bytes sampled).
Pikachu/Fox now sits in Results to the probe ceiling with no malloc halt
(`2026-09-20-results-r11`). Results capture for this roster is owed.

**Electric bodies are now loaded only when the match can use them** (any
Pikachu/Ness/Kirby incl. polygon kinds); `ndsFTManagerSkeletonReady` makes any
unexpected electric source fall back to the flash family rather than a rejected
draw. Mario/Fox matches get 14,176 B of arena back.

### Arena census -- THE SHARED CAUSE BEHIND "MISSING / INTERMITTENT VFX"

Measured on the shipping-configuration shell ROM (arena 925,184 B):

| case | free at GO | result |
|---|---|---|
| Mario/Fox Dream Land (earlier today) | 61,124 | fine |
| Pikachu/Fox Dream Land | 7,556 (min 3,184) | `gcSetMaxNumGObj(48)` then `(56)` at time 0 |
| Pikachu/Samus Dream Land | -- | **malloc halt in scVSBattleStartBattle (136 wanted, 80 free)** |
| Mario/Fox Saffron, before electric-body gating | -- | **malloc halt (108 wanted, 76 free)** |
| Mario/Fox Saffron, after | min 2,844 | starts; GObj cap latched |

`ifCommonSetMaxNumGObj` is the source's own rule: below 25 KiB free it caps the
GObj count at the current active count for the rest of the match, and every
`efManagerMakeEffectNoForce` maker then returns NULL whenever the scene is at
the cap. That is a complete, source-faithful explanation for effects that are
missing or intermittent on heavier rosters (electric sparks, impact wave,
Kirby/Yoshi sub-effects, Pokemon attack VFX) with zero native rejects, and it
is roster- and stage-dependent exactly as the owner reports. The anim cache is
not the consumer: it already declines (reserved 0, fail 1).

Startup allocations >= 6 KB (Mario/Fox Dream Land): common interface files
**208,672** (of which IFCommonGameStatus is 152,288, ~133 KB of it RGBA32
countdown/GO and GAME SET/TIME UP letters), stage **202,816**,
ITCommonData+Object **82,976** + 14,784, EFCommonEffects **94,704**, fighter
pool 45,200, per fighter ~14.5 KB pack + 7-30 KB owner image. Static image is
2.98 MB of 4 MB. Largest BSS: FGM cache 237,568, framebuffer/fighter-packet
arena 147,840.

Recommended lever, not started: a build-time DS repack of IFCommonGameStatus
(RGBA32 -> the 16-bit form the DS uploader already reduces it to) with remapped
`ll*` offsets, worth ~66 KB in every battle. Until the arena grows, heavy
rosters will keep dropping effects and some pairs cannot start a match.

### Saffron gate -- reproduce-first result

Gate logic cycles as source: open from tic 1, monster at 1095, closed at 1220
(gate_wait 1000), reopening ~2220. The two door DObjs animate (open z=330 /
y=-30, closed z~0 / y=390) and the captures differ: the green shutter is absent
at tic 648 and present at tic 1608 (`artifacts/visibility/2026-09-20-saffron-gate-{open,closed}.png`).
The closed door shows a diagonal split consistent with coplanar z-fighting
against the wall; that is the remaining defect, not a baked pose.

### Not closed in this batch

Thunder's role-4 effect trail and Ness's PK Thunder tail effect (root 0x8f98)
both draw through `gcDrawDObjDLLinksForGObj`, a callback kind the effect
admission (`ndsStageGCDrawAllLoopEffectKindAccepted`) refuses, so they are
counted as rejected draws and never reach an owner. Admitting the kind needs
the Ness 0x8f98 fixed-image owner in the same change or it converts a silent
gap into a NO_PROGRAM failure. Thunder's four live images still upload after
the GO texture fence (4 fallback uploads per match).

## 2026-09-20 (cont. 2) — reproduce-first sweep, Samus Bomb owner, arena +20 KB

Natural-input witnesses on the shell ROM (status id, body triangles on the
sampled frame, native failures), each with a GO-free capture:

| row | result |
|---|---|
| Samus shield roll | EscapeB 157, program 2, **80 tris**, 0 failures; morph ball visible (`2026-09-20-samus-roll.png`). Not reproduced. |
| Samus Down-B body | status 230, program 3, 80 tris; ball visible. |
| Samus Down-B **bomb** | **CONFIRMED GAP**: 98 `NO_PROGRAM` failures at SamusModel root `0xE0D8`. The weapon had no native owner, so "Down-B is invisible" was the bomb, not the body. |
| Link Neutral-B throw / catch | 230 and 231, **338 tris** in both, 0 rejects; Link visible (`2026-09-20-link-boomerang.png`, `-link-catch`). Not reproduced on the ground states. |
| Kirby up smash / down smash / grab | 207 / 208 / 166 entered, 256 tris each, down smash connects (victim 52), 0 failures (`2026-09-20-kirby-{upsmash,downsmash,grab}.png`). Not reproduced. |

**Samus Bomb native owner.** `generate_nds_native_samus_bomb.py` pins SamusModel
(SHA-pinned, asset 320) root 0xE0D8: one four-vertex billboard, fixed CI4 16x16
texels at 0xDF88, and an MObjSub whose flags are exactly `MOBJ_FLAG_PALETTE`,
so the live material is one palette image (effects 0x0001) and the list keeps
its own LOADTLUT, tile and alpha-compare state. The source palette swap (the
bomb's flash) stays authoritative. The compact Samus pack retains the texel row
through `WEAPON_TEXTURE_ROOTS` (+116 B). After: `gNdsSamusBombNativeDraws >= 20`,
`DIAG_NATIVE` all zero, bomb visible at Samus's feet (`2026-09-20-samus-bomb.png`).

**Arena +20,480 B.** Menu translation units (`battleship_mn*`, the DS menu
shell, the UI kit) never execute inside a battle frame; they now build `-Os`,
the same code generation the -Os harness ROMs already qualify. Static image
2,984,948 -> 2,964,844 B; arena 925,184 -> **945,664**. Kirby/Mario Dream Land,
which halted in malloc at battle start (136 wanted, 112 free), now starts with
19,932 B minimum free. Still under the 25 KiB latch, so effects on that pair
remain capped; the GameStatus repack is still the lever that clears it.
Kirby is no longer treated as an electric attacker for electric-body loading:
he can only copy electricity from a Pikachu or Ness already in the match.

`probe-native-render-scene.ps1`: `side_smash` now forwards `-StickY`, giving
natural up/down smash pumps.

Roster electric flash, runtime proof: natural Thunder on **Samus** engages the
source no-skeleton family (colanim id 16-19, colour 1 active) during damage
status 50; Samus draws 322 triangles, `DIAG_NATIVE` all zero, and the pair that
could not start before the arena reclaim now runs (arena 945,664, minimum free
5,136 B -- still far under the 25 KiB latch). `2026-09-20-electric-common-samus`.

Normal r6 candidate: `builds/remaining-bugs-playtest-r6/smash64ds.nds`, SHA256
`3CA8891E087BD42C7F7E17EB96B8EF8C2DB078973C1BADEDEC89E9D7543D32F2`; ELF
`052FDE657B5B65B652BF192B301B8322ED7A854D9EF5D25AD130BDA26F45B52D`. Native-only
316 link inputs, boot/title PASS, static image 2,961,740 B. Supersedes r4/r5.
The accepted root ROM is unchanged. No full-list, lifecycle or performance claim.

## 2026-09-20 (cont. 3) — Data menu, Model-backed effects, graded IA8 quads

**Data menu (REPRODUCED, fixed).** Natural Title -> Mode Select -> DATA on the
shell ROM: scene 58 entered, `rej=0`, top screen a bare blue field. First
failure: `ndsUiKitSetText` refuses on the MAIN engine, whose text slab was
reclaimed at P2-1 closeout, and the Data screen was still composed of font rows.
`generate_mn_ui_kit.py` now bakes the source screen (mndata.c): collage, decal
papers, Data icon, smash logo, DATA label, and both of the source's row layouts
(with / without Sound Test) in the tab HIGHLIGHT / NOT pairs; the DS screen
blits them exactly like the Option screen. The 320->256 resample rounded a one
pixel gap between the lrs-16 middle and the right cap at x=133; the middle now
extends under the cap, which is drawn after it. Capture after:
`artifacts/visibility/2026-09-20-data-menu.png` (Characters highlighted, VS
Record, Sound Test). Sound Test and VS Record are still font screens, so they
enter the kit on the SUB engine (which keeps its text slab) under the DATA plate
instead of being blank; their source art stays owed to P2-7.

**Every Model-backed EFDesc was dead in compact battles.** Witness: Thunder's
trail maker returned NULL with `*desc->file_head == 0`. `gFTDataPikachuModel` is
published by the pack loader, survives both `ftManagerMakeFighter` calls, and is
zero by `ifCommonBattleSetGameStatusWait`: the source's
`ftManagerSetupFilesPlayablesAll` re-queries every Model through
`lbRelocGetStatusBufferFile` (ftmanager.c:312), a compact pack has no status
node, and the miss overwrote the slot. `lbRelocGetStatusBufferFile` now answers
for a record this scene's pack loader already made resident (still lookup-only;
an unselected fighter stays NULL). That revives six descriptors: Pikachu Thunder
trail, Yoshi shield and egg escape, and Ness PK Thunder trail / reflected trail /
wave. After: `gNdsPikachuThunderNativeRoleMask == 7`, `DIAG_NATIVE` all zero.

**Effect DLLINKS admission + Ness tail owner.** Both bolt tails draw through
`gcDrawDObjDLLinksForGObj`, a kind the effect admission refused. It is admitted,
and Ness's tail effect (NessModel root 0x8F98: fixed IA8 0x8B58, fixed prim/env,
no material) has its own SHA-pinned packet; the compact Ness pack retains the
texels (+1,036 B).

**Thunder was invisible with every draw counted.** The generic cache uploads
RGB555 + ONE alpha bit. Thunder's IA8 image is a four-texel core inside a twelve
texel alpha ramp, so the threshold left a sub-pixel hairline (texture entry:
32x32, TEXIMAGE format 7). IA8 quad owners now bind a dedicated A3I5 copy
(4-bit intensity exact in a 32-step grey palette, eight coverage levels, half
the VRAM), built once per image per scene-texture generation with the generic
cache as fallback. Thunder roots 0/2, PK Thunder and the PK tail opt in; the
kernel's source-executing host test covers build, reuse, rebuild after a scene
reset and the fallback. Runtime pixel proof follows the build in progress.

**Yoshi Egg Lay size.** `gGCScaleX` is a tree accumulator in the source (every
scale-bearing ancestor multiplies it before a kind-46 billboard reads it); the
port reset it per DObj and only the joint attach raised it. Egg Lay's root
carries the captive's `effect_size` and its child a TraRotRpyRSca, so the egg
drew far too small. The kind-46 arm now folds in the scale-bearing ancestors.
Runtime proof owed.

**Arena.** Cold non-battle scenes (movies, staff roll, how-to-play, 1P intro /
stage clear / challenger) also build `-Os`; arena 945,664 -> **949,760**.
Pikachu/Fox sat at 26,396 B minimum free before the revived effects and 22,848 B
with them -- the latch is still within reach of that pair. Sizing the VS fighter
pools by participating slots (22,600 B in a two-player match) was tried and
REVERTED: about fifty port sites index the FTStruct pool as a fixed four-slot
array, so a shorter pool reads past its end.

## 2026-09-20 (cont. 4) — Thunder proven, two crashes, two Results freezes

**Thunder is visible.** Two defects were stacked behind "every draw counted".
(1) The graded A3I5 copy fixed coverage but not colour: every IA8 quad owner
uses the one combiner `(PRIM - ENV) * TEXEL0 + ENV`, which DS modulate cannot
express, so a grey palette drew the bolt's yellow fringe as grey. The palette IS
that ramp now -- ENV at index 0 to PRIM at 31, white polygon colour -- keyed per
entry on the live prim/env so a material colour change re-uploads 32 entries,
never the image. (2) The fill read the image raw. Fighter and effect files are
O2R word-swapped in memory (logical byte i at i ^ 3), so every four-texel group
was mirrored: the FIFTH instance of the byte-lane family. Both fills and the
TLUT reader take the lane from the config that names the file's layout, and the
host test pins it with a swapped fixture. Captures:
`artifacts/visibility/2026-09-20-thunder-ramp.png` (segments pinned in camera)
and `2026-09-20-thunder-natural.png` (unpinned: a yellow-white column from the
top of the screen to Pikachu). PK Thunder and its tail:
`2026-09-20-pkthunder-ramp.png`, the opaque white rectangle is gone.

Two instrument notes that cost captures. The frame shown at a gdb stop is about
three updates old (2:1 frame skip plus the GX swap), so a bolt captured the
moment its head reaches the fighter is still above the camera -- capture late in
the trail's life (`$cnt <= 2`). And Thunder's head stops at the first platform
above Pikachu (y=1950 under Dream Land's): source `wpMapTestAllCheckCollEnd`,
not a defect.

**Thunder Jolt crashed the game (REPRODUCED, fixed).** Data abort at 0x1c in
`efManagerPikachuThunderJoltMakeEffect`: the source writes
`DObjGetStruct(effect)->translate` with no NULL test (efmanager.c:4544), the
desc was deferred, and a deferred desc yields a bare GObj. The maker now goes
through the same mapped-desc wrapper as Thunder's trail and shock, so an
unbackable desc answers NULL. It was unbackable for a second reason:
`gFTDataPikachuSpecial3` read NULL all match. Asset 342 is a full file that
arrives as Special1's external dependency -- resident, this scene, no status
node -- and the batch-5 residency rule only answered for pack records. It now
answers for any record this scene made resident (still lookup-only). The Jolt
family's CI4 quads also take the graded path: the source draws them
`G_RM_AA_TEX_EDGE` with bilinear filtering, so its one-bit alpha reaches the
screen as a one-texel coverage ramp; an opaque texel beside a clear one drops to
5/7 and the clear one takes that neighbour's colour at 2/7.

**Link losing a match froze the Results screen (REPRODUCED, fixed).**
`ndsPreviewPackLoadHalt(20, Link)` in scene 24. The loser's Claps pose
(scsubsysdatalink.c `D_ovl1_80391978`) is four raw SetModelPartID words:
(20,0) (11,-1) (21,0) (19,-1) -- the sword sheathed as in Entry and the shield
moved from the hand joint to the back joint, a child of the torso. Same
nineteen roots as Entry, shield one place later in the walk, so no program
matched. The live tree was read out at the halt and the generator derives the
same order and binding parents `(255,0,1,2,3,1,5,6,1,1,1,10,11,0,13,14,0,16,17)`
from the four events alone. Link program 4 "Claps"; four runtime sites (owner
runtime, program lookup, program bound, program count) plus the binding-parent
and cross-slot lookups.

**Luigi had the same hole.** His Win2 pose sets model part 1 on both hand
joints, exactly as Mario's Lose pose does; Mario, Fox and Donkey have variant
rows, Luigi had none, so one Luigi win in three would decline and halt. Rows
added from 221_LuigiMain.c (joint 10: 0x5910 / 0x5BA0, joint 16: 0x53C0 /
0x5650).

`check_model_part_mutation_coverage.py` passed throughout: it reads the main
motion files, and both holes are in `scsubsysdata*.c`. A census of those files
finds model-part events for Donkey, Fox, Link, Luigi, Mario, Ness and Samus;
Link's Claps2 and Pose scripts (1P scenes) are still uncovered.

## 2026-09-20 (cont. 5) — proofs on the batch-11 diagnostic ROM

Every row is a natural-path probe (menu walk, real inputs) on
`builds/build-crash-diagnostic/smash64ds-p2-shell-hwtri.nds`, `DIAG_NATIVE` all
zero, captures under `artifacts/visibility/`.

| Item | Result | Evidence |
|---|---|---|
| Link loses, VS Results | 240 Results frames, `link_program=4`, decline 0, reject 0, no halt | `2026-09-20-link-results.gdb.txt` `RESULTS_OK` |
| Luigi wins, VS Results | 240 frames, decline 0, reject 0 (which Win pose the RNG chose is not observed) | `2026-09-20-luigi-results.gdb.txt` |
| Link entry beam | translucent, Link visible inside; witness alpha 31 -> 9, othermode `0x552079` -> `0x5049d9` | `2026-09-20-link-intro-b11.png` |
| Link Spin Attack colour | ramp palettes engage: 10 bakes, 134 draws in one spin | `2026-09-20-link-spin-b11.png` |
| Thunder Jolt | no abort across repeated jolts; ground effect maker reached; CI4 edge ramp | `2026-09-20-jolt-after.png` |
| Yoshi Egg Lay | victim egg is fighter-sized | `2026-09-20-yoshi-egglay-b11.png` |
| Yoshi egg throw burst | shell shards draw, particle atlas miss masks 0/0 | `2026-09-20-yoshi-eggexplode2-b11.png` |
| Pikachu forward smash | shock roots draw (>= 6 in one smash) | `2026-09-20-pikachu-fsmash-b11.png` |
| Samus Charge Shot | ball is in front of the arm cannon | `2026-09-20-samus-charge-b11.png` |

**Entry-effect ramp palettes.** Two combine rows in the entry-effect tables share
the colour half `(PRIM - ENV) * TEXEL0 + ENV` and draw IA planes baked A5I3 over
one shared grey palette; modulate can only multiply that grey by one colour, so
the ENV end was lost: Link's Spin Attack (ENV 0xd00c03 on the effect, nine
oranges on the weapon), Fox's, Captain's and Samus's glows, the Poke Ball rays.
A palette-only GL name now holds ENV->PRIM through the texture's own grey steps
and is attached with glAssignColorTable (the KO stars' mechanism); twelve entries
keyed on the live colours, replaced round-robin, generation-scoped. A slot is
eligible only if every group that samples it is a ramp group.

**The entry beam was solid because of an inherited render mode**, not a texture:
`sNdsRendererAdapterEffectOtherModeL` is one value folded from every DL head and
it outlives the proc that wrote it. The beam is DObjDLLink list 1 and its generic
`gcDrawDObjTreeDLLinksForGObj` proc emits no mode, so it took the previous
effect's opaque one. A list-1 draw whose proc set no mode now inherits the battle
camera's XLU head (gmcamera.c:1055). Narrow on purpose: entry effects only.

**Charge Shot depth** is an owner ruling (BUGS.md): the billboard is centred on
the muzzle, so the cannon cut it in half. It takes the guard bias toward the eye
that Sing's rings already used; the helper moved beside the shared quad kernel
so there is one copy, and its host test follows it.

Not proven here: Kirby's Fox hat (the pump cannot press Down after an inhale),
Ness Up-B self-hit, Sector Z repeated intros.

## 2026-09-20 (cont. 6) -- the CSS aborted ~20 s after a VS match; gate-slide underlay

**CSS re-entry after Results crashed (REPRODUCED, fixed).** Found because a probe
whose battle condition never fired ran on: battle -> Results -> CSS, then a
wandering-PC abort (`lr=0x504340`, user LR `gcRunGObj+18`) about 1,300 CSS frames
in. `gGCCommonLinks[0]` was a legitimate GObj whose memory had been overwritten
with display-list words (`0xE2001E01`, lbCommonStartSprite's alpha-compare, where
its process list belonged). Cause: the VS CSS preview loop calls `gcDrawAll()`
directly, and unlike every source scene draw (taskman.c:1093-1100) it never
rewound the graphics heap or the four DL heads. The preview camera's capture proc
emits ~17 Gfx words a draw, nothing on DS executes or rewinds them, and the heads
climbed the arena: 174,832 bytes to the first live GObj / ~650 draws = the
observed twenty-odd seconds. Entered from VS Mode the same leak ran into memory
nothing owned, which is how it hid through every earlier run; entered from
Results the scene's DL buffers sit at the arena's start, under everything.
`syTaskmanResetGraphicsHeap(); func_80004AB0();` now precede that draw. After:
the same path idles in the re-entered CSS past the probe's 280 s window, no
abort. A/B before the fix: identical abort with the underlay below both enabled
and forced off, so it was not the new code.

**Gate-slide underlay is resident.** Mid-slide, every tic repainted the slot's
static card from NitroFS (open + 7,844-byte read + hash + flush per sliding slot
per tic, twenty tics a slide) before drawing the two door halves -- the measured
multi-frame sync spike behind "low FPS / flashing during gate openings". Each
slot keeps a verified RAM copy for the length of its slide
(`ndsUiKitLoadSurfaceCopy` / `ndsUiKitBlitSurfaceCopy`): one read on the first
mid-slide tic, DMA rows after; the terminal frame still takes the baked gate from
the pack. 31,376 B from the CSS scene's own arena (never .bss), reserved at scene
load -- the preview slice loader rewinds the arena cursor on cancel and assumes
its payload is the newest allocation, so a mid-scene allocation is not safe.
Walk counters: 80 card blits from RAM, 4 loads, 0 declines, 0 hash mismatches.
A new VBlank histogram for the gate phase is still owed.

Not done: the resumable compact preview load (BGM pauses, hover delay). The
dwell stays at 13 tics until that load stops monopolising a frame.

Playtest r8: `builds/remaining-bugs-playtest-r8/smash64ds.nds`, SHA-256
`B89506697CD98F13DF052AB630DA52B256080C2E589750786DDCDE9EE5F55D4B`, boot `P2_RUNTIME_OK`; r7 was
`E58E469FA407582EE40F56DCA6CCC9592973876D585938179DDDE3C016710E6F` and lacks the two CSS changes above.

## 2026-09-20 (cont. 7) -- an exhausted arena froze the match; the last 8 KiB are not for cosmetics

**REPRODUCED: Kirby/Mario on Dream Land froze on a hit spark.** `MALLOCOVF
req=136 head=56` under `efManagerMakeEffect(dEFManagerDamageFlyOrbsEffectDesc)
-> gcAddChildForDObj -> gcGetDObjSetNextAlloc`, i.e. `ndsSyMallocOverflowHalt`.
That pair starts with 19.9 KB free, already under the source's 25 KiB GObj-cap
latch -- but the latch caps GObjs, not bytes: every pool behind a GObj (DObj 136
B a node, MObj, AObj, XObj) still grows on demand and never shrinks, so a long
match keeps creeping until the next 136-byte node does not fit.

`gcMakeGObjSPAfter` is interposed (same rename seam as gcSetupObjman): below
8 KiB free, a GObj of kind EFFECT is refused with NULL -- what the source itself
returns when the GObj cap or the effect pool is spent, so every maker already
handles it. Fighters, weapons, items and the interface are untouched. After, same
pair, same inputs: no halt through t=3,000+; free settles at 2,008 B with 428
effects refused (`gNdsGcEffectArenaFloorRefusals`), Kirby's Mario copy still
draws natively (268 tris). This converts a freeze into missing cosmetics. It does
not return a byte: the arena lever is still the fix for the VFX family.

**+9,768 B for every battle.** `sNdsUiKitSurfaceCache` was a static used only by
the title's PRESS START blink and the CSS door strip. Both cache at scene entry,
so the block now comes from that scene's arena, generation-checked. Walk check:
2 caches, 80 underlay blits, 0 declines / hash mismatches / read failures.

Kirby's inhale loop requests FGM **203** (`ftParamPlayLoopSFX`); the pitch check
against the pack entry is still owed.

Playtest r9: `builds/remaining-bugs-playtest-r9/smash64ds.nds`, SHA-256
`A936EC064070AABEBC5054EB8C5BE3FDBE9E6786B59153B89ED4ADA641C90657`, boot
`P2_RUNTIME_OK`. Supersedes r8 (`B8950669...`) and r7.

## 2026-09-20 (cont. 8) -- the arena lever that was within reach: idle fighter-packet regions

The fighter-packet arena is 141,440 static bytes in four fixed regions keyed by
source-player slot, and a packet only ever writes its own. A VS match with an
empty port therefore has 35,360 idle bytes per absent player for exactly the
battle's lifetime. `gcSetupObjman` (already interposed) now carves two
battle-lifetime pools from them instead of the taskman arena:

- the AObj pool, at its full 384 entries since it is free (13,824 B), and
- a DObj pool. Every scene passes `dobjs_num == 0`, so each DObj was a separate
  136-byte arena allocation that is never returned; the source already threads a
  preallocated array (objman.c:2368). A second idle port gives it a whole region
  (260 nodes); with one, it takes what the AObj pool left (158).

DObjs are never range-checked against the arena. MObjs are
(renderer_adapter_stage.c), so they stay in it. No idle port, no packets
compiled in, or any scene but VS battle: the arena path, unchanged.

| Pair (Dream Land unless noted) | free at GO before | after |
|---|---|---|
| Kirby / Mario | 19.9 KB (froze mid-match) | **51,708 B**, 33,408 B at t=900, 0 refusals |
| Pikachu / Samus | 5,136 B (halted at start) | **41,012 B**, 35,656 B at t=900 |
| Mario / Fox, Saffron | 2,844 B | **37,612 B** |

All three now start above the source's 25 KiB GObj-cap latch, which is the shared
cause recorded above for the intermittent impact wave, missing electric sparks,
Saffron's Pokemon effects and Kirby's Up-B effects. Four-player matches gain
nothing from this and still rely on the 8 KiB effect floor.

**Kirby's entry star had no native program.** Once KirbySpecial2's slot stopped
reading NULL the effect existed, and drew `NO_PROGRAM` (identity effect/asset 348,
root 0x1CF8). It is the one RGBA32 list among the entry effects: the generator
gains an RGBA32 lane (top five bits, alpha at the half point; an O2R payload
swaps lanes inside a 32-bit word, so a 32-bit texel reads in order), the root is
appended to the Kirby family so earlier ordinals hold, and the adapter admits
0x1CF8. After: `DIAG_NATIVE` all zero through Kirby's intro
(`artifacts/visibility/2026-09-20-kirby-intro-b19.png`).

Playtest r10: `builds/remaining-bugs-playtest-r10/smash64ds.nds`, SHA-256
`37D0163A88ECC022267274CA4AE85691EB5DC9BA88675024AAABB5873ADBBA1D`, boot
`P2_RUNTIME_OK`. Supersedes r9.

## 2026-09-21 -- Sector Z: the intro "crash" was the arena, and the Arwing lost its textures at GO

**Intro crashes.** Four pairs through intro + 240 frames on Sector Z, no crash.
Free at GO WITH the pool borrow above: Ness/Mario 41,724 B, Kirby/Fox 24,012 B,
Pikachu/Samus 7,324 B, Link/Yoshi 4,036 B. Every one of those was 30-50 KB lower
before it, i.e. negative for the heavy pairs: the "crashes sometimes during
character intro" is the malloc-overflow halt, and which pair decides "sometimes".
Sector Z is still the tightest stage; Link/Yoshi runs on the 8 KiB effect floor
(20 refusals by t=600). Objman node counts there: 105 DObj / 117 XObj / 191 AObj /
42 GObj, so the borrowed pools already cover it -- what is left is files.

**The fly-by Arwing was refused every frame** (`REJECTED_PROGRAM`, ground object,
root 0x1FA0: 2,584 in 900 frames with Fox, 119 by t=600 without). Sector Z's
Arwing is a GROUND object drawing Fox's entry Arwing list (grsector.c loads
FoxSpecial3, with or without Fox in the match) and is rendered by Fox's entry
owner. That owner's nineteen textures are generator-marked "startup only" and
retired at GO -- true of the entry ROOT LIST, false for this stage -- so the
fly-by arrived to texture name 0. (Witness: `prep=64`, slots 2/10/20 == 0 at
t=600.) `battleship_scvsbattle.c` now tells the release when Sector Z is live and
the release keeps the Fox-root texture slots there; every other stage retires
them as before. After: `DIAG_NATIVE` all zero and entry fallbacks 0 through
t>900, Mario/Fox and Ness/Mario. The failure record named asset 97 because
`ndsRelocFindLoadedFileContaining` matched a stale record first -- misattribution,
not the cause; an Effect-only restriction on the candidate was tried first and
reverted once that was clear.

Playtest r11: `builds/remaining-bugs-playtest-r11/smash64ds.nds`, SHA-256
`0A4C449217AD9FDBE1BD91583B4EF7BF4046D409FC270E82312604279868A1A0`, boot
`P2_RUNTIME_OK`. Supersedes r10.

## 2026-09-21 (cont.) -- figatree heaps ride in the idle storage too

With two idle ports the DObj pool takes the second region whole, which left
21,536 B of the first unused after the AObj pool. It is now a bump allocator
(`ndsBattleIdleScratchAlloc`, re-seeded by every `gcSetupObjman`) and
`ftManagerAllocFigatreeHeapKind` is interposed to draw from it: a fighter's
figatree heap is sized by its largest animation file (Pikachu 10,752 B, Yoshi
9,360, Kirby 7,808, Link 7,328, Samus 6,640, Mario 6,224, Fox 4,896) and lives
exactly as long as the battle. The animation loaders take the heap as an address
and register their own loaded-file range over it; nothing asks whether it is
inside the arena. `gNdsRelocForceFighterAnimFallbackCount` stayed 0 in every run.

| Pair / stage | free at GO, start of session | r11 | r12 |
|---|---|---|---|
| Link / Yoshi, Sector Z | < 0 (halt) | 4,036 B | **20,724 B**, 0 refusals |
| Pikachu / Samus, Dream Land | 5,136 B (halt at start) | 41,012 B | **58,404 B** |
| Mario / Fox, Saffron | 2,844 B | 37,612 B | **62,884 B** |

All `native=pass`, `DIAG_NATIVE` zero, both fighters drawing. Three- and
four-player matches get less (one idle region) or nothing; IFCommonGameStatus
(~130 KB of letter pixels that are only read at GO and at match end) is still the
lever for those.

Playtest r12: `builds/remaining-bugs-playtest-r12/smash64ds.nds`, SHA-256
`D867AF160ACAC680CCD805130D4788CE6A804918A1B753BB169E1B44B454B60B`, boot
`P2_RUNTIME_OK`. Supersedes r11.

## 2026-09-21 (cont.) -- the stage rows

### Yoshi's Island: the platforms were behind the camera, not missing

Every one of binding 15's 77 triangles emitted (`gNdsNativeStageRoofSnapEmitted`
equals `...Given` for all 58 runs, hidden mask 0, shortfall 0) and none of them
appeared. The binding is rigid, so it draws through the Task 36 hardware-compose
bracket: `BeginSegment` loads the camera projection, `EnsureWorld` pushes the
world matrix, and the vertices are submitted in hardware units.

`BeginSegment` was loading the projection unscaled. The world and camera
translations it sits on top of are divided by `NDS_RENDERER_HW_WORLD_UNIT_SHIFT`
(source / 256) with the homogeneous 1 left alone, so the position reaching the
projection is (camera-space / 256, 1) and its constant row has to be in the same
unit. It was not, so clip z came out far outside -w..w and the geometry engine
near-clipped the polygons. The no-Z runs never saw it -- `LoadNoZProjection`
replaces the z column and m32 with it -- and until this stage no rigid binding
drew with source depth, which is why four other stages were unaffected. Fix:
divide the projection's row 3 the same way `ndsRendererBuildRawHardwareMatrix`
already divides the CPU-composed one. The three plank platforms, the book pages
and the floor path all draw.

### Yoshi's Island: the cards and the heart sparkles kept their coverage

Both are a BLENDPE combine -- `(PRIM - ENV) * TEXEL0 + ENV` with alpha
`(0,0,0,TEXEL0)` -- over an I4 tile, which is a coverage image: the colour is
the endpoint lerp AT that coverage and the alpha IS that coverage. The RGB5A1
bake kept `ndsRendererHardwareConvertI`'s unconditional alpha bit, so the whole
rectangle drew opaque at the ENV colour: white card backgrounds around the cloud
shape, solid spikes for the sparkles.

The generic texture path now has a graded-coverage arm for exactly that semantic
class (mode `SOURCE_ALPHA`, format I, 4b or 8b, no TEXEL1, not a refresh): the
pixel loop leaves the raw 5-bit coverage in the scratch word, and after clamp
padding the words fold to `GL_RGB8_A5` bytes with an 8-entry palette holding the
lerp at each band's midpoint. An ordinary cache entry, keyed as before -- PRIM
and ENV are already in the key -- not a dedicated name.
`gNdsRendererGradedCoverageUploadCount` = 3 on Yoshi's Island, 0 on Dream Land.

### Mushroom Kingdom: a clamped window wider than the largest upload

The upper-left ledge is a 144x16 `G_TX_CLAMP` window over an 8-texel mask period
(run 32, binding 14). The RDP clamps to the window first and masks second, so
inside the window the tile repeats every `1 << mask` texels; materialising writes
that repetition into a window-sized upload, which only works up to 128. Past it
the tile kept its load stride with a clamping sampler and every coordinate beyond
the first stride drew one edge texel -- a flat tan slab.

Such an axis now uploads exactly one period instead.
`ndsRendererHardwareTextureParams` already turns "upload == period, window >
period" into a repeating sampler, so the only thing given up is the clamp
OUTSIDE the window, which geometry mapped inside its own window never samples.
`gNdsRendererClampedWindowPeriodUploadCount` = 53 on Mushroom Kingdom, 17 on
Dream Land, 16 on Peach's Castle; all three stages re-captured and correct.

### Peach's Castle: the board carried a fighter through the tower

Reproduced exactly. An idle fighter placed on the right half of the sliding
bottom board rode from x = 1320 to x = 538 over 220 tics -- straight through the
tower base -- while `gNdsRendererM3PostArmFailureCount`-style wall counters
stayed zero, because neither `mpProcessRunLWallCollisionAdjNew` nor its right
twin was ever called. Two causes, both now fixed against `mp/mpcommon.c` and
`mp/mpcollision.c`:

1. `mpCommonRunFighterAllCollisions`' live arm opened at the floor test.
   `mpcommon.c:162` opens with `CheckTestLWallCollision` / `RunLWallCollision`
   and the right pair; without them a floor can carry a fighter into a wall and
   nothing else looks at it. Restored in source order.
2. The wall sweep behind all four `mpCollisionCheck{L,R}WallLineCollision*`
   entry points was a generic two-sided segment intersection against each wall's
   first and last vertex. It ignored the yakumono translation on the "Same"
   form (so a wall on a positioned DObj -- the Castle's blue side ramps are
   yakumono 3 and 4 -- was tested at LOCAL coordinates, and `mpProcessUpdateMain`
   only runs the "Diff" form on its first sub-step, so a knocked-back fighter
   flew through them on every later sub-step), reported (+-1, 0) for a slanted
   wall, hit a wall from either side, and had no tolerance for a start point
   exactly on the wall. It is now the source's: `CheckLRSurfaceFlat`,
   `Check{L,R}WallSurfaceTilt`, `GetLRAngle` and the per-vertex-pair scan with
   the group range reject, transcribed. `mpCollisionGetLRCommon` likewise picks
   the segment spanning the object's y and returns that segment's normal.

The one liberty: `coll_pos_{prev,next}` are s16, and for integer c and real v,
c < v iff c < ceil(v), so the range bounds round once per group and the per-line
test is an integer compare rather than two soft-float ones.

After: the board carries the fighter to x = 1320, the wall holds them there, and
when the board's end slides out from under them they drop -- the source's
sequence. `check_mp_line_extent_reject_exact.py` PASS (111,961 extent rejects and
68,993 sweep rejects replayed segment by segment, 0 missed hits). Walk sweeps on
Castle and Hyrule show no spurious wall stops.

Playtest r13: `builds/remaining-bugs-playtest-r13/smash64ds.nds`, SHA-256
`DEBECA4A218332F0EB2CF8996C830A75551321FE89870B368F05BEC1E0F05559`, boot
`P2_RUNTIME_OK`. Supersedes r12.

## 2026-09-21 (cont.) -- three rows narrowed, no code change

### Ness PK Thunder self-hit: forced, and it does not crash

Scripted circular steering never self-hit in ~1,800 frames, so the earlier
"not reproduced" was weak evidence. The source test is purely positional
(`ftnessspecialhi.c:60`): `|dx| < FTNESS_PKTHUNDER_COLLIDE_X` and
`|(ness.y + 150) - bolt.y| < FTNESS_PKTHUNDER_COLLIDE_Y` with
`pkjibaku_delay == 0`. Putting the bolt on Ness and clearing the delay reaches
exactly the state a real self-hit reaches.

Two forced self-hits (`ftNessSpecialHiJibakuSetStatus` breakpoint count 2) over
350 frames: status runs 228 (hold) -> Jibaku -> 68 -> 70 -> 80 -> 10, i.e. the
blast fires and Ness takes his own knockback, then recovers. No halt, no data
abort, `native=pass` throughout. The row is not reproducible even when the
self-hit is made deterministic.

### Saffron gate: the hazard logic cycles; the gate model does not move

Traced `gGRCommonStruct.yamabuki` for 1,900 frames. The state machine is
correct and matches `gryamabuki.c`:

| frame | status | gate_wait | monster_wait | monster | collision x |
|---|---|---|---|---|---|
| 100-300 | Sleep | 1 | 0 | no | 960 |
| 400-1400 | Wait | 0 | 1087 -> 87 | no | **1600** (open) |
| 1500-1600 | Open | 0 | 0 | yes | 960 |
| 1700-1900 | Wait | 1000 -> 705 | 1080 -> 880 | - | **960** (closed) |

`gMPCollisionYakumonoDObjs->dobjs[3]` follows the collision position exactly,
so the hazard's *collision* opens and closes on schedule. What does not move is
the drawing: the gate GObj's root DObj and its first child read (0, 0) at every
sample, and camera-matched captures at frame 1450 (open) and 1760 (closed) show
the same hub opening with no panel across it. The packet does carry the gate --
segment 3, bindings 17-20, 15 runs, a box about 432 x 860 in local coordinates.

So "the door is always open" is a rendering/animation gap, not a hazard-logic
bug; the earlier "gate cycles, not reproduced" note was measuring the logic and
was right about the logic. Next: find why the open/close AnimJoint's effect
never reaches the gate binding's world matrix (`anim_frame` is non-zero only in
the single open-entry window near frame 400).

### Kirby's Fox hat: the copy succeeds and the draw refuses the result

Driving the real inputs -- hold B to inhale, then stick down in status 261
(`SpecialNCatch`), which is what `ftkirbyspecialn.c:368` reads -- Kirby copies
Fox: `passive_vars.kirby.copy_id` goes 8 -> 1 through statuses 269 -> 273 -> 277.

Immediately after, the 1P-game diagnostic ROM **halts**:
`ndsPreviewPackLoadHalt(20, 8)` from `renderer_adapter_fighter.c`, the
deliberate "a packed preview has no interpreter fallback" trap. So the hat is
reached, the copy is correct, and the *draw* is what refuses it; shipping
builds have no halt, so there the hat simply fails to draw.

**CORRECTION, same day.** An earlier revision of this section named
`gNdsFtrDeclineStage == 2` ("display list outside its loaded file") as the
cause. That was wrong and the mistake is worth keeping: **the stage global is
sticky.** Ten sites assign it and none ever clears it, so the value read at a
halt is the last decline of the whole run, not this draw's. A clause witness
added beside the stage-2 assignment reads 0 at the halt -- that block never
executed on the halting frame -- while the stage still read 2 from some earlier
draw. The declining site is therefore *not yet pinned*: several of the other
places that clear `native_owner_enabled` record no stage at all.

Two things came out of chasing it, and both are kept:

- `ndsPreviewPackLoadHalt` now does `DC_FlushAll()` before it spins. Everything
  that explains a halt was written in the frames just before it and is still
  dirty in the data cache; the spin never writes again, so those lines are
  never evicted and a debugger reads whatever the line last held.
- The halt republishes the renderer's decline words into an array it writes
  itself. `--gc-sections` discards write-only diagnostics **even with
  `__attribute__((used))`** -- `used` keeps a symbol inside its object, not its
  section through the link -- which is why four freshly added witness globals
  were absent from the ELF and gdb resolved their names to one stale word.
  Reading them from surviving code is what keeps them.
- The witness now starts each draw empty, which is what a status global needs
  to mean anything at a stop.

**And with that reset in place the site IS pinned**, which the sticky read
could not establish either way: on the halting frame the Fox-hat draw reads
`gNdsFtrDeclineStage == 2` with `gNdsFtrDeclineOwner == 11` (Kirby) and
`gNdsFtrDeclineSelected == 8` -- so it does decline at "display list outside
its loaded file", in the eligibility pass, before the draw pass reaches the
no-fallback trap. Which of that test's eight clauses is still open: the clause
word reads stale through gdb because the halt's `DC_FlushAll` runs after a
breakpoint placed at the halt's entry. Fixing that ordering, or flushing the
four words at the decline itself, is the next step.

Noted while reading that seam: `KIRBY_TRIO_ADMIT_COPY_HATS` is **True** in
`generate_nds_native_owners.py` and the in-tree generated inc carries all
twelve heads (`NDS_NATIVE_KIRBY_TRIO_HEAD_COUNT 12u`, list
`1 14 3 4 5 6 7 8 9 11 12 13`). Its own comment says to leave it False until the
body sections move into the per-slot hat images, because admitting them costs
Kirby's owner image +28,848 B high / +26,104 B low and the 2026-09-17 gate run
came back with heap low-water 73,064 against 111,680 and 151 native-render
failures. That cost is therefore currently shipping and is an owner call.

## 2026-09-21 (cont.) -- FGM 203: the bake had no resampler pitch ceiling

Owner: "FGM 203 pitch is too high; the stable end pitch should be much lower."

The cue is Kirby's inhale vacuum, a loop-prefix render. Its UCD sweeps notes 12,
13, 14, 16, 17, 19 and then loops note 20 (+700 cents) forever, and its
articulation (98) spawns modulator 40 -- shape 6, **target 12**, i.e. the
`unk2C` pitch envelope -- a one-shot ramp with amplitude 1900 and offset -500
that reaches the target's own +1200 clamp about a hundred ticks in and stays
there. Sustained total: **+1900 cents, ratio 2.9966**.

The console cannot do that. `n_env.c:1489` clips every non-unity voice with
`if (e->rs_ratio > MAX_RATIO) e->rs_ratio = MAX_RATIO;` and then quantises to
1/UNITY_PITCH; `PR/abi.h:280-281` gives those as `1.99996F` -- "within .03
cents of +1 octave" -- and `0x8000`. **A source wave can never play faster than
one octave above its recorded rate**, whatever the cents add up to. The AOT
bake had no such ceiling.

So 203 sustained a playback rate of 95,892 Hz where the console sustains
**63,998 Hz** -- a fifth sharp, and it never came back down. That is the report.

`source_pitch_ratio(cents)` now applies the clip and the quantisation, and the
three sites that turned cents into a rate go through it.

**Independent corroboration, already written down in this repo.**
`nds_audio_fgm.c` explains why one cue takes the full-program AOT path: "whose
first note asks for 90,510 Hz -- past the u16 `frequency` field above". No N64
voice can ask for 90,510 Hz. Three cue-audit rows (189, 190, 219) carried a
`source_rate_above_u16` blocker for the same reason; with the ceiling in place
no rate exceeds u16 and the blocker is gone, so those rows lose it and the
attack-cue audit hash is re-pinned.

Re-derived pins: `ATTACK_CUE_AUDIT_SHA256`, `PUBLIC_WIN_SAMPLE_COUNT`
(69,369 -> 69,363) and the retained-sample proofs for 127, 432, 486, 603, 604,
605 and 608. Every other cue is bit-identical -- the ceiling only binds above
+1200 cents. `scripts/sfx` 48 passed; the one failure
(`test_fgm_metadata_residency`) is pre-existing and untouched by this change:
it compiles an extracted C snippet that calls `ndsAudioFgmDirectRouteInit`
without a declaration.

Playtest r14: `builds/remaining-bugs-playtest-r14/smash64ds.nds`, SHA-256
`A451F3A62291F8CF835C855B88B9EB928888AA5F1122F0ED20AE5FD7E62A1EF7`, boot
`P2_RUNTIME_OK`. Supersedes r13. **Audio changed, so this one is worth a
listen: 203 and the other eight re-pinned cues.**

### r14 shipped silent, and why the boot check did not notice

Owner: "the latest r14 rom you just broke all audio in that ROM build."

Regenerating the pack moved it from 6,969,332 to 6,968,728 bytes.
`nds_audio_fgm.c:1147` compares the NitroFS file size against
`NDS_AUDIO_FGM_PACK_BYTES` in `include/nds/nds_audio_fgm.h` and **rejects the
entire pack** on any mismatch -- by design, fail-closed. Nothing regenerates
that header, so r14 shipped a new pack against the old constant and booted with
no FGM audio at all.

Measured, not inferred: the r14 ELF contains the little-endian word 6,969,332
and not 6,968,728; the rebuilt ROM contains 6,968,728 and not 6,969,332.

| ROM | header expects | pack shipped | audio |
|---|---|---|---|
| r13 | 6,969,332 | 6,969,332 (old) | works |
| r14 | 6,969,332 | **6,968,728** | **silent** |
| r15 | 6,968,728 | 6,968,728 | works |

After the re-pin, on the rebuilt diagnostic ROM: `gNdsAudioFgmLoaded=1`,
`gNdsAudioFgmOpenFailCount=0`, `gNdsAudioFgmFormatFailCount=0`,
`gNdsAudioFgmPlayCalls=64`, `gNdsAudioFgmMissRingCount=0`.

**`P2_RUNTIME_OK` did not catch it and could not.** It proves the ROM boots; it
reports `pack=66496`, which is a different pack entirely. A boot check is not a
feature check.

`scripts/check-audio-fgm-phase-pack.ps1` *is* the right check -- it says "the
ROM boots SILENT" in its own comments and names two earlier instances of this
class (2026-08-02 size/hash drift, 2026-08-24 entry-count drift). I regenerated
the pack without running it. It passes now, with these pins updated and the
reason recorded beside each: pack identity, the 44/66 encode SNRs (both
improved), the five cross-target PCM hashes, and the 249/235 exact-render
hashes. All content-only: entry count 573, cache budget, extents, fork sets and
voice orders are unmoved.

Playtest r15: `builds/remaining-bugs-playtest-r15/smash64ds.nds`, SHA-256
`C975C9570B10B03A0F1CDBF47817C32D055D08DF08B5BE126CFFA6C7BFDCD146`, boot
`P2_RUNTIME_OK`, FGM pack loaded. Supersedes r14, which should be discarded.


## 2026-09-21 continuation: the consolidated brief

Brief: `docs/p2/Smash64DS_BUGS_Consolidated_Fix_Instructions_2026-09-21.md`.
Landed at `2bf791ecad8`; playtest r21
`builds/remaining-bugs-playtest-r21/smash64ds.nds`, SHA-256
`A9324F326163736121FD05C99F490DA94E169300A3047CB8ABA2A81B1E1676CA`, boot
`P2_RUNTIME_OK`, `P2FAIL` all zero, FGM pack 6,968,728 and particle payload
212,160 both matching their pins.

Rows landed, each with the measured cause rather than the symptom:

* Kirby's copied blaster did not fault, it halted. The firing frame selects
  `dKirbyMain_modelparts_desc_0x3C4`, whose rows are `dFoxUnknown_DL` -- file
  315, which the file's own header records as referenced by FoxMain and
  KirbyMain alike. Measured at the refusing draw: stage 2, clause 4, owner 11,
  expected asset `0x148`, offending `0x13b` root `0x3F0`, index 5 of 8, with
  the live vector being Kirby's Fox hat, five canonical roots, the donor, then
  two more. `NDS_R2_FOX_GUN_OVERLAY` already owned that mesh for Fox, so the
  strip now covers Kirby and the remaining seven roots are the vector the
  existing CopyTransition program already carries.
* The damage collision proc had no wall or ceiling branch at all and cleared
  those flags out of `mask_stat`. Restored from `mpcommon.c:725` in source
  order with both arms; `scripts/check_mp_damage_proc_matches_source.py`
  guards it and was mutation-tested red two ways.
* `nEFKindThunderAmp` was answered by the generic HitElectric sprite, so the
  maker had no caller, `--gc-sections` dropped it, and the bank generator --
  which derives its packed set from reachable makers -- left script `0x74`
  UNREACHABLE. Both halves fixed; bank 110 -> 111 scripts, 41 -> 42 textures,
  growth entirely NitroFS payload.
* Kirby's capture and lose-copy stars were `#define`d to NULL behind a note
  about an asset that has been resident for some time.
* Owner, same day: the copied blaster keeps its pistol and loses its muzzle
  flash. Kirby's joint 17 puts that flash on his own body.

Partial: the Poke Ball entry effect. Three defects fixed -- an unregistered
file-head symbol, six offsets that address ITCommonObject rather than
ITCommonData, and a span check comparing the second against the first -- so the
effect now constructs instead of faulting. It then declines in the renderer
(`DIAG_NATIVE` domain 2, root `0x9340`, reason 1) because its geometry has no
native bake. See the `ll-symbol-address-is-the-offset` note: that same trio was
the cause of three dead effects in one day.

Implemented but NOT exercised, and therefore not closed: the Castle ramp case
itself (no scripted launch reached a ramp; the proc is proven live with its
floor arm hitting), Pikachu's burst on screen (`is_thunder_destroy` sets within
~20 frames, so the head never reaches him in a scripted run, under open sky as
well as under platforms), and the lose-copy star (never lost a copy).

### Thunder Jolt ground edges: step 1 done, no repair yet

The handoff's first step is to decide whether the soft edge is stored alpha,
intensity used as coverage, filtering, or a combination. The palette settles
it. The air sibling's generator pins the source TLUT exactly:

    0x003E 0x003F 0x18FD 0x31BD 0x423D 0x4A7D 0x633D 0x7BFD
    0x8C7D 0x9CFD 0xAD7D 0xC63D 0xD6BD 0xF7BD 0xFFFF 0xEF7E

That is RGBA5551, so there is exactly one alpha bit and it is set for entries
1..14 and clear for 0 and 15 -- no graded alpha anywhere. What the entries do
carry is a luminance ramp from near-black blue (R0 G0 B31) to white
(R31 G31 B31). So the edge is **intensity used as coverage**: on the console
the dim low entries vanish into the background under the source blend, and on
DS, drawn through the generic CI4 -> PAL16 path at full polygon alpha, the same
texels are solid dark blue -- a hard rim rather than a fade.

The repair therefore has to map that ramp's luminance to DS alpha (A3I5 or
A5I3), not add a constant polygon alpha, which the handoff explicitly rules
out. It should follow the existing dedicated-owner pattern
(`ndsRendererHardwarePrepareRebirthA5I3`,
`ndsRendererHardwarePrepareIFCommonA3I5Atlas`) rather than widen the shared
converter, because a pattern match on "CI4 with a ramp TLUT" would catch
unrelated textures. Not attempted here: it needs the four live images
extracted through the generator and a visual comparison this session could not
make.

### OPEN: the flash on Kirby is not the blaster glow

Owner, r21 then r22: "the effect is still played over kirby (flashing white and
gold) ... Kirby should not have an effect overlayed at all on the body after or
when firing the pistol."

Two attempts, both wrong, both now excluded as suspects:

1. r18 removed the pistol MODEL. Owner: "that wasn't the issue ... not the
   pistol model." The model is source-correct
   (dKirbyMain_modelparts_desc_0x3C4 puts dFoxUnknown_DL on joint 17) and was
   restored in r21.
2. r21 suppressed the MUZZLE call of efManagerFoxBlasterGlowMakeEffect; r22
   suppressed ALL SEVEN of its callbacks for Kirby-owned blasters
   (map/hit/shield/setoff/absorb/hop/reflector), measured as
   gNdsFoxBlasterGlowAOTSpawnCount staying 0 across five shots where it
   previously reached 1 by the third. The flash survived that, so
   efManagerFoxBlasterGlowMakeEffect is NOT the object being seen.

The r22 suppression is kept: it is measured, scoped to Kirby-owned weapons, and
leaves Fox's four glows intact (0 -> 1 -> 2 -> 4 -> 6 across five shots). It is
simply not sufficient, and it is not the cause.

**What has not been done, and is the next step.** Stop guessing the maker and
enumerate. `ftParamMakeEffect` (reloc_backend_compat_shims.c:8364) is the kind
dispatcher every motion-script effect passes through: break there, filter on
Kirby's status 235/236 (CopyFoxSpecialN / CopyFoxSpecialAirN) and print the
effect_id and joint for every call in the firing window. Do the same for the
colour-animation entry -- white-and-gold over a body reads more like a colanim
on the fighter's own material than like a separate effect object, and no
attempt so far has looked there. A probe for this exists at
`scratchpad/kirby_fire_fx.ps1`; it was written and not run.

Also unexamined: whether the same effect appears on Fox and simply looks
correct there because it lands at the barrel rather than on the torso. The
owner's "Look at the fox fighter for example" was an instruction to compare,
and no side-by-side capture has been taken.

### Heap headroom, and why the shield is intermittent

ifCommonSetMaxNumGObj (ifcommon.c) latches the GObj cap the first frame the
general heap drops below 25 * 1024 = 25,600 bytes, and never unlatches:

    if ((gcGetMaxNumGObj() == -1) && (free_space < 25 * 1024))
        gcSetMaxNumGObj(gcGetGObjsActiveNum());

Measured gNdsTaskmanGeneralHeapFreeMin, this session's probes:

    Mario vs Fox, shielding      95,068
    Pikachu entry                65,188
    Kirby vs Fox                 23,096 - 23,204

Kirby matches sit BELOW the latch. Once it fires, no further effect GObj can be
created, which is the shape of the owner's "shield is intermittent" (it depends
whether the shield existed before the latch) and a candidate for the spit-out
star never appearing. The margin needed is about 2,500 bytes, which is small
enough that this is worth pricing before treating the star as a draw-side bug.

Not yet attributed: what Kirby's presence costs that Mario/Fox does not. The
copy-hat image slots are the first candidate -- sNdsNativeKirbyHatImages is
[4][2] and each allocation is NDS_NATIVE_KIRBY_HAT_MAX_BYTES, the union of
every hat image, about 8.5 KB against a largest actual hat of 8,552 bytes --
but they allocate lazily per (slot, detail), so a two-player match should take
one or two, not eight. `scratchpad/heap_trace.ps1` traces the allocations and
the free-space curve; it was written and not run.

## 2026-09-21, later: the flash, and what the heap trace actually says

### The flash over Kirby is a colour animation. Landed, runtime proof owed.

Third attempt, and the first one that named the producer instead of guessing a
maker. `ndsFTMainCheckSetFighterColAnimID` in `src/import/battleship_ftmain.c`
already refuses BattleShip's cosmetic `SetColAnim(nGMColAnimFighterFoxSpecialHiStart,
0)` -- but only for `fkind` Fox/NFox in the two Fox laser statuses. Kirby does
not run Fox's scripts. `228_KirbyMainMotion.c` carries his own copies, and they
issue the identical command under `nFTKirbyStatusCopyFoxSpecialN` / `...AirN`
with `fkind` Kirby, so the override never covered him.

That is exactly why the owner said "you say the flash is played on fox, but I
don't see that": Fox's was suppressed here long ago, so only the copy still
flashes. Neither earlier suspect could ever have been it -- the pistol model
draws no colour over the body, and `efManagerFoxBlasterGlowMakeEffect` was
measured at zero spawns across all seven callbacks in r22 while the flash
survived.

Outside `REGION_JP` the ground script's body lives in `dKirbyMainMotion_0x1E18`
while `dKirbyMainMotion_LaserGround` is a two-command stub, so a checker keyed
on script NAMES would pin the wrong region. `scripts/check_kirby_copyfox_colanim_override.py`
resolves the pair by content instead -- the scripts that issue the colanim AND
play `nSYAudioFGMFoxSpecialN` -- and asserts the override covers exactly those,
keeps Fox's clause, and carries no preprocessor conditional. Green; RED under
three mutations.

### The GObj latch does NOT fire in an ordinary Kirby match

`scratchpad/heap_trace.ps1` ran, after replacing its `gcGetMaxNumGObj()` call
with the `gNdsIFCommonGObjLatch*` witnesses -- a gdb inferior call crashes this
target at `0xfffffffc`, which is the third time that has cost a probe.

Kirby vs Fox, Dream Land, 720 sampled interface updates:

    free      49,452 -> 41,684, monotone, flat from n=420 onward
    applied   0 for every sample (gcGetMaxNumGObj() stayed -1)
    active    46 - 49 live GObjs
    hat       0 -- the copy hat was never loaded

So the earlier figure of 23,096 was not this. It came from
`gNdsTaskmanGeneralHeapFreeMin`, a minimum watermark that records transient
load dips, and in that run Kirby had taken a copy. The board's reading -- that
Kirby matches "sit below the latch" permanently and need ~2,500 bytes of
headroom -- is wrong as written. Correct reading: the steady state is 41,684,
16 KB clear of the floor, and only a transient dip during the copy-hat load can
cross it. `ifCommonSetMaxNumGObj` samples once per interface update, so whether
it lands inside that dip is a race.

**That race is the intermittency.** It explains the owner's "shield is
invisible / I guess its intermittent because its working now" precisely, and it
means the repair is to bound or stage the copy-hat allocation so the dip never
crosses 25,600 -- not to reclaim 2,500 bytes of steady-state heap, and not to
raise a capacity. `scratchpad/heap_copy_trace.ps1` drives the inhale and
samples across the load to measure the dip's depth and width.

`heap_copy_trace.ps1` ran but did NOT test the hypothesis: `copy=8` for every
sample -- `copy_id` stayed Kirby's own kind, so no copy was ever taken, and
`hat` stayed 0. The input machine holds B, which re-triggers the inhale but
never swallows; a copy needs the victim captured and then DOWN. The free-space
curve it did capture is identical to the plain match (49,452 -> 41,684, `applied=0`),
which re-confirms the steady state and leaves the dip unmeasured. The probe
needs a swallow step keyed on the capture-hold status before it can answer
whether the copy-hat load crosses 25,600.

### Yoshi's shield egg: the prior owner was correct, the revert was about cost

Not one of the consolidated brief's 23 rows, but it is in `BUGS.md` and the
board carries it. `795e2659219` added the native owner and argued the case well
-- `dEFManagerYoshiShieldEffectDesc` and `dEFManagerYoshiEggEscapeEffectDesc`
name the same `&llYoshiModelShieldDObjDesc`, both states call
`ftParamHideModelPartAll` behind `fp->fkind == nFTKindYoshi`, so with no owner
nothing drew at all and the intro and the shield were one bug. The root at
`0xa860` is an immutable 29-command display list, not a DObjDesc, and compiles
to one group and two triangles.

`252a9aa4290` reverted it for a Boundary RED: arena -4,096 and 14 texture-bind
rejects. **Fourteen rejects from a two-triangle quad is the anomaly worth
chasing** -- that is not the egg's own geometry failing, it is material state
asking for an image that is not resident, or the insertion disturbing a shared
bind cache. Start there rather than re-litigating the owner.

Not touched this session: the owner lives in
`scripts/3d_vfx/generate_nds_entry_effects.py`, which a concurrent agent holds
for the Poke Ball row, and evaluating the arena cost needs a build.

### CORRECTION: the copy-hat cost is permanent, and the latch does fire

The swallow-driven run (`2026-09-21-heap-copy2`) answers it. My "transient dip"
reading above is WRONG; the board's original "Kirby sits below the latch" was
right, with one missing qualifier -- **only after he takes a copy**.

    n=640   free=41,684   copy=8  hat=0   applied=0     (before the copy)
    n=678   HAT call=1    free=41,348
    n=678   HAT call=2    free=32,792
    n=680   free=24,232   copy=1  hat=2   applied=0
    n=720   free=23,204   copy=1  hat=2   applied=1
    n=1400  free=23,204   copy=1  hat=2   applied=1   dip=722

Free never recovers: 722 consecutive samples under 25,600, `applied=1`, live
GObjs pinned at 46 from the copy onward. Both details load eagerly in the same
frame at `reloc_backend_compat_shims.c:12522-12525`, about 8,556 bytes each,
and the copy is rejected unless both succeed. 41,684 - 23,204 = 18,480 bytes
spent permanently; the floor is missed by **2,396**.

So from the moment Kirby copies anyone, no further effect GObj can be made for
the rest of the match. That is the owner's intermittent shield exactly -- it
works if it was alive when the latch fired -- and it is a live candidate for the
invisible spit-out star, which is created after a copy by definition.

**Landed:** `sNdsNativeKirbyHatImages` is indexed `[battle_slot][use_low_detail]`,
so each buffer only ever holds images of its own detail, yet both were allocated
`NDS_NATIVE_KIRBY_HAT_MAX_BYTES` -- the union over BOTH details. The generator
now emits per-detail unions and the runtime allocates by detail.

Measured from the shipped images: largest high 8,552, largest low 7,636, so the
low slot stops overpaying by **916 bytes**. Honest arithmetic: 23,204 + 916 =
24,120, still **1,480 under** the floor. This is a real reclaim and a correct
scoping fix, but **it does not clear the latch by itself.**

**Next lever, and the measurement that justifies it:** the low-detail image is
loaded eagerly at copy time for every Kirby. If a two-player match never selects
low detail, its 7,636 bytes are dead for the whole match and deferring that load
clears the floor outright (23,204 + 7,636 = 30,840). Before doing it, count
binds of `sNdsNativeKirbyHatImages[slot][1]` in a natural two-player copy match
-- if it is bound, the lever is wrong and the remaining 1,480 must come from
somewhere else. Do not guess this one.

### But the latch probably does NOT explain the invisible star

Read the reserve against the same trace before blaming the cap for everything.
`ndsIFCommonPreserveGObjRuntimeReserve` (`battleship_ifcommon.c:255`) waits for
`gcGetMaxNumGObj() >= 0` and then, once, on a GO update, raises the cap by
`NDS_IFCOMMON_GOBJ_LATCH_RESERVE` = 8. The trace shows `applied=1` from n=720,
so that did happen after the copy latched it -- the reserve is correctly ordered
for a mid-match latch, not only an entry one.

Live GObjs were 48 at the latch and settle at 46. Cap is therefore about 54
against 46 live: roughly **ten free slots** for the rest of the match. That is
enough for a shield and a spit star several times over.

So the latch is real, permanent, and worth removing -- but "the star is
GObj-starved" no longer follows from it, and K04 should not be closed by heap
work. The star constructs; the renderer's treatment of it is still the
unexamined half. Keep the two questions apart.

### CORRECTION: the Poke Ball was never missing a native bake

The earlier entry above says the effect "reaches the renderer and DECLINES --
no native bake". Wrong, and the wrongness was in reading the failure code.
`DIAG_NATIVE` domain 2 reason 1 is `NDS_NATIVE_FAILURE_NO_PROGRAM`, which means
**nothing claimed this root**, not "this root has no geometry".

Both Poke Balls are ONE source descriptor. The ground item reaches it as
`ITAttributes.data` at ITCommonData+0x6E4; `efManagerMBallThrownMakeEffect`
(efmanager.c:5248) reads that same fixed-up pointer and subtracts `0x9430` from
it to recover ITCommonObject's base, so the subtrahend IS that descriptor --
same tree, same MObjSub table at `0x9120`, same eight CURRENT_IMAGE frames. The
two drawable roots `0x9250` and `0x9340` were already baked in
`nds_native_item_mball.exec.inc`.

What excluded it was the admission gate in `renderer_adapter_stage.c:8179-8191`:
it required `sNdsRendererAdapterItemSubmitActive` and a parent GObj of kind
Item, then an `ITStruct` whose kind is `nITKindMBall`. The entry ball is an
EFFECT GObj submitted through the effect layer, so it matched none of that, fell
to the generic guard, and was published as a missing program. Admitting the same
asset and the same two roots from the effect layer is the fix; the geometry is
not baked twice.

Also found, and a real second defect: `efManagerMBallRaysMakeEffect`'s prototype
was guarded `#if NDS_P2_PIKACHU` while the call site runs under `NDS_P2_PURIN`
as well, so a Purin-without-Pikachu roster had a call with no prototype. And the
old `#if NDS_P2_ITEM_CORE` around both calls silently compiled the row away
instead of failing; it is now a declared `#error` dependency.

Instrumented so the next run can answer "did BOTH halves arrive":
`gNdsEntryMBallThrownRootMask` bit 0 = `0x9250`, bit 1 = `0x9340`, expect 3.

### The lazy-low-detail lever is REFUTED, and the heap row is smaller than it looked

The receipt proposed deferring the low-detail copy-hat load on the theory that a
two-player match never selects it, which would have freed 7,636 bytes and
cleared the floor outright. It does not hold.

`renderer_adapter_fighter.c:3302` -- `use_low_detail = (fp->detail_curr ==
nFTPartsDetailLow)`. Detail is BattleShip's own per-fighter state, not a
match-configuration constant, so the low hat can be demanded on any frame. That
is exactly the hazard the existing comment at `reloc_backend_compat_shims.c:12507`
names: a synchronous NitroFS beat failing mid-frame, with no clean way to reject
an already-granted copy. Deferring the load trades a permanent 7.6 KB for an
unbounded mid-match stall and a failure with nowhere to go. Do not do it.

Nor is there roster headroom: the board records the shipping roster closed at
12/12, so scoping the hat union to the built roster saves nothing.

**And the residual may not matter.** `ndsIFCommonPreserveGObjRuntimeReserve`
raises the cap by eight once the latch has fired, and it is correctly ordered
for a mid-match latch -- the trace shows `applied=1` from n=720. Live settles at
46 against a cap near 54, so roughly ten slots stay free for the rest of the
match. No missing-VFX symptom has been tied to exhausting them.

So the honest state of this row: the latch is real, permanent after a copy, and
now 916 bytes further from the floor than it was. The remaining 1,480 bytes have
**no demonstrated consequence**, and the two obvious ways to reclaim them are
refuted. Anyone reopening this needs a measurement showing an allocation
actually refused -- `gcMakeGObjSPAfter` returning NULL with the cap reached --
not a headroom figure. Until then this is understood, not outstanding.

## Owner playtest of r23

**K05 flash: ACCEPTED.** Removed from BUGS.md. The colour-animation diagnosis
was right; the two earlier guesses were not.

### Kirby face/body is STILL wrong, and holding neutral B fixes it

That clue reframes the row. The clamp-order fold is real arithmetic -- the
capped diffuse reproduces the source exactly at full light for all three
fighters -- but it is evidently **not what the owner is looking at**, because a
static arithmetic error cannot be cured by holding a button.

Neutral B is Inhale. What holding it changes:
  * a status transition, so `ftMainSetStatus` invalidates the flat-transform
    walk and the renderer status caches;
  * Kirby's model parts change, so a different epoch set is submitted.

Something a status change clears is **stale cached state**, not a formula. The
materials audit ruled packet replay out on the grounds that
`ndsFighterPacketApplyTint` re-derives prim-from-root and
`ndsRendererAdapterMaterialAnimHash` keys on `primcolor..light2color`. That
argument shows the hash NOTICES a colour change; it does not show the hash
distinguishes two runs that need different LIGHT state under the same colours,
and it says nothing about the per-epoch lit/unlit decision.

**The owner's own question is the right one and it is answerable directly: is
the face unlit, or the body?** `sNdsR2EpochUnlitVertexColor` decides per epoch
whether a run is lit by the geometry engine or emitted as raw vertex colour
(R2-03 E48/E49, `nds_renderer_native_common.c`). Two adjacent runs disagreeing
on that would look exactly like two materials, and would be immune to any
diffuse/ambient correction -- which is what the owner reports.

**Next step, and do not skip it for a third guess at the formula:** break at the
epoch prepare for Kirby's face and body roots in one natural frame and print,
per epoch, `epoch_lit`, `sNdsR2EpochUnlitVertexColor`, the resolved
diffuse/ambient, and the material epoch/replay key. Then repeat while neutral B
is held and diff the two. The field that changes between them is the bug. If
`epoch_lit` differs between face and body, the clamp work is irrelevant to this
row and should be judged on its own merits.

### r23: hardly any effects play (owner). Regression, cause NOT yet established.

Measured so far, and deliberately nothing more:

    r22  text 1,769,940  data 263,112  bss 931,040
    r23  text 1,774,700  data 263,112  bss 931,712   (+4,760 text, +672 bss)

5,432 bytes off the arena. On its own that should not matter in the Kirby/Fox
pair, which measured 41,684 free before the copy -- but the record already notes
pairs sitting at ~7.5 KB free, and there the same 5,432 lowers the live count at
which `ifCommonSetMaxNumGObj` freezes the cap, which caps effects for the rest
of the match. That is a mechanism, not a finding.

Three candidates checked and REFUTED, so nobody re-checks them:
  * the Poke Ball admission does NOT collide across files -- the guard requires
    `loaded->asset_id == NDS_NATIVE_ITEM_MBALL_ASSET` before matching either
    root, so no foreign effect can be routed into the MBall owner;
  * `ndsNativeThunderGroundReleaseCoverageTextures` is null-safe on an unused
    slot and matches the sibling release loops around it;
  * the newly resolvable Results submotions can raise the figatree heap, but the
    largest submotion is 11,872 B against a battle animation maximum of 13,952,
    so the roster maximum moves little if at all.

**The measurement that decides it** is the free-space curve on an r23 shell ROM
against the r22-era 41,684: build the `p2-shell-hwtri` sibling and re-run
`scratchpad/heap_trace.ps1`. The shipping ROM cannot be probed -- `boot_check`
fails identically on r22, which the owner played, so that harness only drives
the shell ROM. Do not remove anything from r23 before that number exists.

### The 0x45 matrix repair touches three more effects than L01's report said

L01's fix gave `renderer_adapter_matrix.c` a case for custom matrix kind `0x45`
(`lbCommonRotScaFuncMatrix`), which previously fell to the translate-bearing
fallback. The report justified it on DamageSlash, whose ROOT carries
`{0x28, 0x45, 0x00}` -- `0x28` supplies the translation and `0x45` must not
supply it a second time. That reasoning is sound and unchanged.

But `0x45` appears six times in `efmanager.c`, and **three of them are the MAIN
transform of a CHILD DObj with no translate-bearing partner**:

    dEFManagerStarRodSparkEffectDesc      struct 2 = { 0x45, Null, 0x00 }
    dEFManagerDamageFlySparksEffectDesc   struct 2 = { 0x45, Null, 0x00 }
    dEFManagerHaveStructEffectDesc        struct 2 = { 0x45, Null, 0x00 }

Those children previously received `dobj->translate` from the fallback and now
receive none. By the source the new behaviour is correct -- `lbCommonRotScaFuncMatrix`
writes a zero translation row, so a child whose only XObj is `0x45` genuinely
has no translation of its own and sits at its parent's origin. But it IS a
behaviour change to three effects beyond the one the row was about, and sparks
are common enough to be noticed.

**Not the reported regression, almost certainly** -- three effects is not
"hardly any effects play" -- but it belongs in the candidate list and it must be
looked at in the same capture, not assumed correct because the argument is
tidy. If the sparks now stack on their parent's origin instead of spreading,
this is why.

Deliberately NOT reverted: the source argument is concrete and reverting on a
guess is the failure mode this session kept hitting. Decide it with the capture.

### The face/body probe cannot be a breakpoint. It needs an in-ROM witness.

Ran it on the r24 shell ROM. It dies: `0xfffffffc in ?? ()` after 43.8 s, the
CPU-wander signature.

Two reasons, both structural, so do not retry this shape:
  * `ndsRendererHardwareWriteDiffuseAmbient` resolves into ITCM (`0x1ffafc0`,
    2 locations) and fires once per material epoch per fighter per frame.
    Attaching gdb commands there is far too hot for this harness -- the same
    class of failure as the 37-breakpoint fan-out already recorded.
  * In this build `ifCommonSetMaxNumGObj` resolved to **address 0** with 3
    locations, so the frame-tick breakpoint was planted at 0 and corrupted
    execution on its own. (The heap trace on this same ROM survived only
    because it got its lines out before dying.)

**The right instrument is a witness store**, which is this repo's own standing
lesson: a small fixed array written by the shade pass -- for each of Kirby's
epochs, `epoch_lit`, `sNdsR2EpochUnlitVertexColor`, and the packed
diffuse/ambient -- plus a generation counter, read ONCE at a breakpoint that is
already cheap. Then run twice, with and without neutral B held, and diff.

Costs one small source addition and one build; it answers the owner's exact
question -- is the face unlit or the body -- instead of inferring it. Do this
before touching the material formula again. The clamp-order cap that shipped in
r23/r24 is arithmetically correct at full light and is NOT the reported defect;
judge it on its own merits, separately.

### CONFIRMED: the effects regression was the 0x45 matrix change, not the heap

Owner on r24: fixed. So the cause was the L01 repair applying to DObjs whose
ONLY transform is kind `0x45` -- `dEFManagerStarRodSparkEffectDesc`,
`dEFManagerDamageFlySparksEffectDesc` and `dEFManagerHaveStructEffectDesc`, all
of which carry it as the main transform of a CHILD with no translate-bearing
sibling. Those children had been taking `dobj->translate` from the fallback;
zeroing it collapsed them onto their parent's origin, which reads on screen as
the effect simply not playing.

Worth keeping, because the process is the transferable part:

  * The owner's own attribution -- "effect heap must be gone" -- was wrong, and
    so was my first instinct. **The measurement is what killed it**: the r24
    shell ROM holds 79,172 bytes free across 720 sampled updates with the GObj
    cap never latching. Had I acted on the heap theory I would have spent the
    session reclaiming bytes that were never the problem.
  * Four candidates were refuted cheaply and in writing before any code moved --
    the Poke Ball admission (guarded on asset id), the texture release
    (null-safe), the figatree maximum (11,872 against 13,952), the particle
    pack (all pins intact). Each refutation cost one command.
  * The repair was **scoping, not reverting**. L01's evidence covered exactly
    one shape: a root carrying `{0x28, 0x45}`, where `0x28` already supplies the
    world contact. Restricting the zero-translation form to DObjs with more than
    one transform keeps that fix and leaves every unexamined case exactly as it
    shipped. `gNdsRendererAdapterCustom45SoloCount` makes the untouched path
    visible.

**Standing lesson:** a renderer repair justified on one descriptor is a claim
about every descriptor that reaches the same switch arm. Before landing one,
enumerate the other users of that arm and ask whether the evidence covers them.
Here it did not, six of them existed, and three were live.

### ANSWERED: neither is unlit. They take different MATERIAL paths.

The witness store works. Kirby, r24 shell, 16 epochs sampled in one frame:

    i=0  lit=1 unlitv=0 usemat=1 mat=0xffffffff shade=0x25295ad6
    i=6  lit=1 unlitv=0 usemat=0 mat=0          shade=0x25297fff
    i=8  lit=1 unlitv=0 usemat=1 mat=0xeeeeaaff shade=0x19083eb5
    i=11 lit=1 unlitv=0 usemat=0 mat=0          shade=0x25297fff

**Every epoch is lit and none takes the raw-vertex-colour override**, so the
owner's either/or -- is the face unlit or the body -- has a third answer: both
are lit, and they differ on `use_material`.

Decode the two with the same ambient. Ambient `0x2529` is (9,9,9).
  * `usemat=0` epochs write diffuse `0x7fff` = (31,31,31).
  * `usemat=1` with a white prim writes `0x5ad6` = (22,22,22).

22 is exactly prim minus ambient, so this is the r23 clamp engaging, and it is
engaging correctly: at full light both land on 9+22 = 31. **But the RAMPS
differ.** At half light the `usemat=0` run gives 9+15 and the `usemat=1` run
gives 9+11. Two runs on one fighter, same light state, shaded on different
curves -- which is what "the face and body are two distinct colours" looks like.

So the clamp is not wrong arithmetic; it is **applied to only one of two paths
that must agree**. Before r23 both wrote 31 and matched by accident, because a
white prim folds to no change. The repair made the material path correct and
left the no-material path on the old curve.

Why holding neutral B fixes it follows: Inhale changes Kirby's model parts, so
the seam between a `usemat=1` run and a `usemat=0` run stops falling across the
visible face/body boundary.

**Next step, and it is now a small decision, not an investigation:** the source
has no divergence here -- with no material there is no prim multiply on either
side -- so the two port paths must be brought onto one curve. Either apply the
same saturation discipline to the no-material path, or establish that the
no-material epochs on this fighter should have carried a material at all, which
the `mat=0` reads make worth checking first. Do not revert the clamp: it is the
path that now matches the source.

### P01: ground jolt ACCEPTED, air jolt REGRESSED

Owner on r24/r25: the terrain-following jolt's edges are fixed. The AIR jolt,
which was working, is now broken.

What is known, so the next reader does not start from scratch:

  * The P01 change did NOT edit the air generators. `generate_nds_native_pikachu_thunderjolt.py`
    and `..._effect.py` are untouched and a host test asserts they stay that way.
    So this is not a direct edit -- it is a SHARED-RESOURCE interaction.
  * Both jolts come from the same source asset, PikachuSpecial3 342. The air
    owner uses the specialized CI4 quad helper; the ground owner now uses a
    dedicated A5I3 converter with its own 3-slot cache, uploading through
    `ndsRendererHardwarePrepareIFCommonCloudAtlas`.
  * That converter PINS three texture names for the scene rather than making
    them evictable, and it reports `gNdsThunderGroundCoverageVramBytes`.
    Net VRAM was measured as NEGATIVE (-3,024 B) because the images previously
    uploaded as direct colour -- but "less total" is not "same allocation
    order", and the air quad's upload now happens against a differently
    occupied allocator.

**Two candidate mechanisms, in the order worth checking:**
  1. **Allocation/eviction order.** The three pinned names change what VRAM is
     free when the air quad asks. Read `gNdsThunderGroundCoverageVramBytes` and
     the air owner's own bind/reject counters in one match with both jolts on
     screen; a refused air upload is the tell.
  2. **Bind hand-off.** `nds_native_pikachu_thunderground.exec.inc:102` tries the
     dedicated bind and falls back to `ndsRendererHardwareBindTexture` only on
     refusal. If the dedicated path leaves texture state the generic path's
     cache believes it still owns, the air quad drawn afterwards inherits the
     wrong texture. Check whether the generic bind re-binds unconditionally or
     keys on a "current name" the dedicated path never updates.

Deliberately NOT guessed at: the owner is mid-verification on r25, and a blind
change to the shared texture path is how the ground fix would get lost too. Fix
this with the counters in hand, and keep the ground repair -- it is accepted.

### r25 face/body: almost fixed, and the residue is FACING-DEPENDENT

Owner on r25: fixed facing LEFT; still wrong facing RIGHT; **fixed facing right
while in the ledge-balance animation.** That last clause is the useful one --
it rules out anything static about the roots or the palettes.

Facing-dependent shading with the same materials means the **normals**, not the
colours. Kirby's facing is applied as a mirror (`lr`, a negative scale on X), so
a right-facing root has a NEGATIVE-DETERMINANT modelview. The DS geometry engine
transforms normals by that matrix's upper 3x3, so a mirror turns them inward and
the dot product changes sign -- the diffuse term collapses and the run falls back
toward ambient alone. Two runs that interpret vertex bytes differently (a lit
material run versus an unlit colour run, which is exactly the `use_material`
split the witness already found) will not degrade identically, so the seam
appears on one facing only.

The ledge-balance case fits: that animation's root carries a rotation rather
than a plain mirror, so the determinant is positive and the normals survive.

`ndsRendererR2WriteLightVector` writes `GFX_LIGHT_VECTOR` under a push /
loadIdentity / pop bracket, i.e. in view space, deliberately. That is correct
only if the normals reaching the engine are also in a space where the light
vector means the same thing. Under a mirrored modelview they are not.

**Next step, and it is one build:** extend the shade witness already in
`nds_renderer_native_common.c` with the fighter's `lr` and the sign of the
current modelview determinant, then read it facing left, facing right, and in
the ledge-balance pose. If the sign flips with facing and the ledge pose reads
positive, the repair is to compensate the normal transform for a
negative-determinant modelview -- not to touch the colour fold again.

Do NOT adjust the clamp or the white-prim exemption for this. Those are
facing-independent and the owner reports the left-facing case correct.

## THREE REGRESSIONS, ONE SHAPE. This is the finding of the session.

Owner across r24/r25:
  * the matrix repair fixed Link's slash and broke three spark effects;
  * the ground-jolt converter fixed the terrain jolt and broke the AIR jolt;
  * the Poke Ball admission made the ball visible and broke the opening RAYS.

Each repair was correct for the consumer it was written for, was justified from
the source, carried a mutation-tested check, and was reviewed. Every one of them
still broke a sibling, and **not one was caught by any check** -- all three came
back from the owner playing the ROM.

The shape is identical in all three: a change to a SHARED path was validated
against ONE of its users.

  * kind 0x45 is a switch arm six descriptors reach; the evidence covered one.
  * the jolt texture converter changed allocation order and bind state on a path
    the air owner also uses; the evidence covered the ground owner.
  * the MBall admission added an effect-layer arm to the item submit path; the
    evidence covered the ball, and the rays are submitted right behind it.

**The rule, and it is cheap:** before landing a change to a shared switch arm,
adapter path, cache or allocator, ENUMERATE its other users and say in the
commit what happens to each. One grep for the arm's other cases; one sentence
per user. All three of these would have been caught by that grep -- the 0x45
census took one command and found six users after the fact.

The corollary for the checks: a mutation test proves the repair does what it
claims for ITS case. It says nothing about the siblings. A shared-path change
needs a sibling census, not a stronger unit test.

**Rays, specifically, for whoever picks it up:** the ball draws and the rays do
not, and the rays are an INDEPENDENT flag in the entry updater -- neither proves
the other, which the brief said explicitly. Suspects in order: (1) the new
effect-layer arm in `renderer_adapter_stage.c` leaves item material/config state
applied that the rays inherit; (2) `dEFManagerMBallRaysEffectDesc` uses custom
matrix kind 0x44, which still takes the translate-bearing fallback and sits next
to the 0x45 arm just changed; (3) the entry-seam edits in
`battleship_ftcommon_entry.c` -- the widened rays prototype and the `#error`
dependency -- altered the order the two effects are made in.

### K04 spit-star: same class as the Poke Ball, and now a known shape

Owner on r25: still invisible. Three things are already settled and should not
be re-derived:
  * both makers were `#define ... NULL` and are restored, so it CONSTRUCTS;
  * it is NOT GObj-starved -- the reserve leaves ~10 free slots after the
    mid-match latch, measured;
  * the renderer side was never examined.

It is the Poke Ball's twin. `llITCommonDataKirbyStarDObjDesc` addresses
**ITCommonObject (file 86)** -- the same asset as `dEFManagerMBallThrownEffectDesc`,
whose descriptor sits at `0x9430` with drawable roots `0x9250` and `0x9340`.
The ball was invisible for exactly one reason: its roots reached the submit path
and nothing claimed them, which publishes `NO_PROGRAM` and reads like missing
geometry. The star's roots are different offsets in that same file and have no
bake and no admission arm at all.

So the repair shape is the one just proven on the ball, and it is now cheap:
  1. decode the star's roots from the descriptor at its ITCommonObject offset,
     the same way `generate_nds_native_item_wave1_core.py` pins the ball's
     `0x9250`/`0x9340` from `ITAttributes.data`;
  2. bake them from source geometry through a generator;
  3. admit them, guarded on `loaded->asset_id` exactly as the ball's arm is.

**And this time do the sibling census FIRST** -- the ball's admission arm broke
the rays sitting behind it on the same path. Enumerate every effect submitted
from asset 86 before adding a second arm to it.

### R02/R03: packing was necessary and NOT sufficient. The failure is downstream.

Owner on r25: no change to the Results poses or the No Contest claps.

What is now PROVEN and must not be re-derived:
  * the 35 submotion payloads ARE in the built NitroFS --
    `builds/build/nitrofs/reloc/reloc_submotions/` contains exactly 35 files, so
    the Makefile staging works;
  * all 47 reachable (fighter, outcome) cells route statically, and
    `check_results_demo_motion_closure.py` is green including its self-test;
  * the Ness Win3 root is baked and the model-part census is green.

So the token-to-asset mapping and the payloads both exist. **Static routing is
not a runtime load**, and that is the gap.

**The blocking instrument was identified and never built.** `ftMainSetStatus`
calls `lbRelocGetForceExternHeapFile` and then assigns `fp->figatree`
UNCONDITIONALLY, discarding the return value; on failure
`lbRelocGetExternHeapFile` returns the heap untouched. The two existing counters
`gNdsRelocForceFighterAnimResolveCount` and `...FallbackCount` both live INSIDE
the `ndsRelocIsFighterAnimID` arm, so a token that fails there increments
NEITHER. A third counter on the INVALID return is what distinguishes the two
remaining halves, and without it this row cannot move:

  1. the demo token still does not resolve at runtime -- add the counter and it
     reads non-zero;
  2. it resolves and loads, but the payload does not survive the AObj16 header
     normalizer that `ndsRelocIsFighterAnimID` admission turns on. Submotion
     payloads were never previously normalized by this port, and nobody has
     checked that their container matches `reloc_animations/`. Compare a
     submotion header against a battle animation header before assuming.

Do that counter FIRST. It is a handful of lines and it splits the remaining
question exactly in half; everything else here is guessing between two
possibilities that look identical on screen.

## 2026-09-21 night -- the first runtime read of this cycle, and it moves three rows

Direct-battle probe ROM: `TARGET=smash64ds-battle-playable-hwtri`
`BUILD=build-pika-fox-probe NDS_P2_PIKACHU=1 NDS_P2_PROOF_FIGHTER0=9`, which is
**Pikachu in slot 0 against the canonical Fox in slot 1** -- the owner's exact
reported pairing. NATIVE_ONLY_PASS, 230 link inputs. melonDS slot 6, GDB stub,
75 s settle. Live match confirmed: `scene_curr` 24 (VSBattle), 2,043 presented
frames.

**Read the harness caveat first:** an earlier attempt through
`probe-battle-progress.ps1` failed and printed `presented=0 / allocfail=187 /
__excpt_entry`. That was a failed capture, not a crash -- see BUG_NOTES. The
values below come from a direct `arm-none-eabi-gdb -batch` attach that reported
the guest alive with 79.5 s of CPU time, which is the reading to trust.

### Face/body P04/K02/J01 -- THE MECHANISM IS LIVE, and the repair is the wrong shape

| witness | facing lr | row_norm | det_20p12 | stretch_20p12 | idle_ok |
|---|---|---|---|---|---|
| 0 | +1 | 4907, 4911, 4908 | 7051 | 4908 | 1 |
| 1 | +1 | 4696, 4701, 4695 | 6180 | 4696 | 1 |

Counters over the match: `LightVectorWrites` 95, `WitnessWrites` 95,
`StretchApplied` **2**, `StretchDeclined` **87**, `IdleTimeouts` 0.

Three conclusions, and the third is the one that matters.

1. **The chains are not rigid.** A rigid chain reads `row_norm` 4096; these read
   4695-4911, so `|M L| / |L|` is 1.15-1.20. The hardware light path normalises
   before the modelview and the software oracle after, so the diffuse level is
   wrong by **15 to 20 percent** on these draws. The precondition the whole
   hypothesis needed is met.
2. **The determinant reading stays refuted, in hardware.** `det_20p12` is +7051
   and +6180 -- positive, as a 90-degree rotation must be. No mirror.
3. **The repair as written addresses the MINORITY case.** Only 2 of 95 writes
   stretched; **87 SHRANK**, and the shrink branch deliberately declines rather
   than wrapping a unit-bounded light vector. Six writes were inside the
   deadband. So turning `NDS_R2_LIGHT_VECTOR_STRETCH_FIX` on would correct about
   2% of the error and leave 92% of it untouched. **Do not ship it as the fix
   for this row.** The dominant case needs the diffuse COLOUR scaled instead,
   which is the different lever agent C's own comment names -- and it has the
   same packet-twin obligation.

### Frozen Fox -- the Appear-overrun chain is REFUTED

`AppearOverrunFighter` 0, `AppearOverrunAnimFrame` 0, `AppearOverrunAnimFallback`
0, `AnimResolve` **247**, `AnimFallback` **0**, `FallbackLastAsset` 0.

No fighter overran Appear and not one animation force-load fell back, over 2,043
presented frames of exactly the reported roster. The Appear path definitely ran
-- `RaysRequest` is 1, and that request is raised from the Appear update -- so
this is not a case of the status never being entered.

So steps 1-3 of the reconstruction are dead alongside step 4. The freeze is not
a stale figatree and not an Appear that never terminates. What this probe cannot
see is the shell's own CSS -> VS path, which is where the owner meets it; the
next candidate is something the direct-battle target skips.

### P03 rays -- construction refusal is REFUTED

`RaysRequest` **1**, `RaysNull` **0**. The rays were requested once and
constructed successfully. They are not GObj-starved and the
`ifCommonSetMaxNumGObj` latch is not what removes them.

That leaves the fifth arm as the live lead: made and submitted but invisible.
MBallRays' PRIM ramp reaches alpha 0 at source tick 50 while its rotation runs
to 130, and a 0-alpha group is skipped -- so if the MatAnim is not advancing,
alpha is 0 from frame 0. Read `gNdsEntryEffectWitness[0..3]` with
`gNdsEntryEffectWitnessRoot = 0x0440` next.

Also measured, and it retires a standing worry: the Poké Ball costs
`MBallArenaCost` **596 bytes**, against `MBallArenaBefore` 145,948. Its
construction is not an arena event.

### The night's new reclaim paths are inert here

`GradedQuadTextureRecycles` 0, `GradedQuadTextureFails` 0,
`ThunderGroundCoverageReclaim` 0. Nothing in this match exhausted either pool,
so both repairs are dormant rather than wrong -- and the air-jolt regression did
not reproduce in this configuration. `gNdsNativeKirbyHatTableHits` is absent
from this ELF because Kirby is not admitted in it; that counter needs a
Kirby-bearing build.

### P03 rays, second read: they DRAW here, and that relocates the row

Same probe ROM, witness armed at boot, six stops in one gdb session (the stub
refuses a second attach, so all sampling has to happen inside one):

| stop | vblank | `MBallRaysCandidate` | `EntryEffectNativeAlphaSkip` |
|---|---|---|---|
| T1 | 83 | 0 | 0 |
| T2 | 104 | **20** | **0** |
| T3 | 125 | 50 | 20 |
| T4 | 146 | 50 | 41 |
| T5 | 187 | 50 | 60 |
| T6 | 388 | 50 | 60 |

`MBallRaysMaterialReject` stayed 0 throughout.

**Between T1 and T2 the rays were submitted twenty times with zero alpha
skips.** They were admitted, their material contract passed, and they drew.
Only afterwards do the skips climb, and the candidate count freezes at 50 --
which is the source's own behaviour: the PRIM ramp reaches alpha 0 at tick 50
while the rotation runs to 130, so the tail is *supposed* to be skipped. The
extra skips past T3 belong to other entry effects; that counter is shared.

So the alpha-0 arm is refuted too, and with it every renderer-side cause for
this row: admission, material state, matrix kind `0x44`, entry-seam ordering and
now alpha. In this configuration the Poké Ball rays are correct.

**What that leaves is the configuration itself.** This probe runs
`NDS_P2_PIKACHU` alone in a direct-battle target with **145,948 bytes free**;
the owner meets the bug in the shipping shell, where the measured Pikachu/Fox
figure is **7,556 free at GO** with the `ifCommonSetMaxNumGObj` latch fired.
`efManagerMBallRaysMakeEffect` takes no EFStruct, so the source's five-free
reserve does not protect it -- it needs only `gcMakeGObjSPAfter`, which the
latch caps. The row therefore moves from the renderer to the arena, and
`gNdsEntryMBallRaysRequestCount` against `...NullCount` on the owner's r31 run
settles it: Null > 0 is allocation, Null == 0 sends it back here.

**Scope note, and it applies to the frozen-Fox read above too.** Both
refutations were taken in a configuration with roughly twenty times the free
arena of the shipping roster. They rule out a mechanism *given memory*; they do
not rule out the same symptom appearing when memory is gone. The witnesses are
compiled into r31 precisely so the shipping configuration can answer that
without another lab build.

### The arena, measured on both sides -- and the deficit is 18,044 bytes

| configuration | arena | free (min) | GObj latch |
|---|---|---|---|
| shipping shell, Pikachu/Fox (recorded earlier) | 933,376 | **7,556** | fired |
| full-roster direct battle (measured tonight) | 1,220,352 | **121,192** | never fired |

Full-roster direct battle = `smash64ds-battle-playable-hwtri` with every P2
fighter, stage and `NDS_P2_ITEM_CORE`, `NDS_P2_PROOF_FIGHTER0=9`, and WITHOUT
the menu shell / 1P / UI kit. 264 link inputs, 2,043 presented frames, scene 24.
Rays `req=1 null=0 cand=50 reject=0`; `AppearOverrun 0`, `AnimFallback 0`;
`LIGHT writes=100 applied=2 declined=92`, `W0 rn=4907 det=7048 stretch=4908`.

The face/body numbers reproduce exactly across both probe configurations, which
makes that finding robust: ~92% of light writes shrink, the determinant is
positive, and `row_norm` sits near 4907 rather than 4096.

**The arena cannot be reproduced in a direct-battle probe**: that target is
286,976 bytes larger precisely because it omits the menu shell, and the latch
never fires there. So the rays and frozen-Fox refutations above are bounded to
the roomy case, as already noted.

`nm --size-sort` on the shipping ELF names the reclaim candidates:
`sNdsAudioFgmCache` **237,568** (8 slots: 1x60 + 2x40 + 1x28 + 4x16 KiB, pinned
by a `_Static_assert`), then `gSYFramebufferSets` 147,840 (already reused as
idle packet storage), then nothing above 33 KB. The deficit to clear the 25,600
floor is **18,044**, so trimming both MEDIUM slots from 40 to 28 KiB (-24,576,
no slot lost, the 60 KiB LARGE slot still covers any cue) is sufficient on its
own. Left unimplemented deliberately: it trades audio-cache residency against
effects appearing while audio rows are open, which is the owner's call.

### The FGM cue histogram, which refutes the trim I was about to recommend

A cache miss is NOT a re-read. `ndsAudioFgmCacheAcquire` (nds_audio_fgm.c:1183)
returns -1 when no free slot is large enough, the caller counts
`gNdsAudioFgmReadFailCount` and **the cue does not play**. The slot IS the
storage, so every slot resize moves the boundary of what can sound.

573 cues in `assets/audio/fgm_phase_pack_ima.json`, by `ima_adpcm_bytes`
against the eight slots (1x60 + 2x40 + 1x28 + 4x16 KiB):

| band | cues | slots that can hold one |
|---|---|---|
| > 60 KiB | **0** | -- |
| 40-60 KiB | **28** | LARGE only (there is **one**) |
| 28-40 KiB | 34 | LARGE or MEDIUM (three) |
| 16-28 KiB | 89 | + COMPACT (four) |
| <= 16 KiB | 422 | any (eight) |

Max cue is 59,344 bytes, so the 60 KiB LARGE slot is right-sized and nothing is
unplayable today.

**Trimming both MEDIUM slots 40 -> 28 KiB is the wrong lever after all.** It
would push the 34 cues in the 28-40 band onto the LARGE slot, taking the
population that depends on that single slot from 28 to 62. Two such cues
overlapping already fail today; this would make that far more likely, and
overlapping battle SFX is exactly where it would be heard. Recorded so the
earlier recommendation in the plan is not acted on.

What the layout can afford is bounded by concurrency, not by the histogram
alone: the file records a measured peak of **six of eight handles with
PoolExhaustCount 0**. Dropping two SMALL slots (8 -> 6, reclaiming 32,768 and
clearing the 18,044 deficit with margin) matches that peak exactly -- which is
to say it removes all of the headroom, and an unlucky size mix at peak would
drop a cue.

So the arena reclaim is available, quantified, and carries a real audio
downside in every form measured here. It is the owner's call, and the numbers
above are what it should be made on.


### THE REGRESSION I CAUSED: this batch cost 8,192 bytes of arena

Measured, not inferred. Same gdb attach, same slot, both ROMs booted to the
attract screen where `gNdsTaskmanArenaChosenSize` is already final:

| ROM | arena | linked total |
|---|---|---|
| r26 (owner played it) | **941,568** | 2,971,404 |
| r31 (this batch) | **933,376** | 2,979,124 |

**-8,192 bytes, exactly two arena pages.** The recorded Pikachu/Fox free at GO
is **7,556**. So this batch plausibly took the owner's own reported roster from
"latched but running" to nothing left -- and `malloc.c:30` is `while (TRUE);`,
so that presents as a hang, not a decline. Instrumentation and fixes aimed at
effects-missing rows had eaten the headroom of the very rows they were for.

A symbol-size diff names where it went, and most of it is the repairs
themselves rather than waste:

| symbol | delta | what |
|---|---|---|
| `ndsRendererSubmitNativeItemKirbyStar` | +872 | K04 fix |
| `sNdsEntryEffectTexels62` | +775 | Yoshi egg texture |
| `ndsRendererAdapterSubmitStageDL` | +468 | K04 admission arm |
| `sNdsRelocAssets` | +420 | 35 Results path rows |
| `ndsNativeThunderGroundReleaseOneCoverageTexture` | +252 | P01 fix |
| `ndsRendererNativeTexturedQuadGraded` | +184 | graded recycle |
| `ftCommonAppearProcUpdate` | +120 | Appear-overrun witness |

Plus about 1.9 KB of `.rodata` path strings that carry no symbol.

**Reclaimed: the face/body witness, defaulted off.**
`NDS_R2_LIGHT_VECTOR_MATRIX` now defaults to 0, returning **1,496 bytes**. The
witness has already done its job -- it produced `row_norm` 4907, `det` +7048 and
92 of 100 writes shrinking, which is what confirmed the mechanism and showed the
stretch repair to be the wrong shape. None of that needs re-taking, and
instrumentation should not sit in the ROM eating the headroom of the rows it
instruments. Turn it back on in a lab build when the follow-up repair needs
measuring.

**A reclaim that FAILED, recorded so it is not retried.** The 35 demo path rows
looked like ~1.9 KB of duplicated `"nitro:/reloc/reloc_submotions/"` prefix, so
they were rewritten as a basename table composed at lookup, mirroring
`ndsRelocAssetP2FighterAnimEntry`. It saved **eight bytes**. Pointer arithmetic
into a string literal -- `(path_) + sizeof(DIR) - 1` -- does not shrink the
literal; the whole string is still emitted and only the pointer moves. Saving
those bytes means emitting basenames from the generator, which is a producer
change. Reverted, along with the checker edit it forced.

r32: `594EB9BA8C24A2A66BBF82B3D68B599EB7DED12C07A601EC9D6A0FE32253D95A`,
NATIVE_ONLY_PASS 316 inputs, hash stable across two builds, linked total
2,977,628 -- still **+6,224** over r26. The remaining excess is the four
repairs, and buying it back means either giving one of them up or taking the
audio-cache decision.

### A HANG ON THE CSS, found by the walk: hovering Ness halts the ROM

Built the shipping configuration with `NDS_P2_MENU_WALK=20` -- the shell's own
self-navigation, which drives Title -> Mode -> VS -> CSS with real input -- to
reach a battle and read the arena in the configuration that actually ships. It
never got to the battle. It stopped at scene 16 (CSS) in
`ndsPreviewPackLoadHalt`, which is `for (;;) { nop; }`.

    HALT reason=20 kind=11        <- kind 11 is NESS
    DECLINE stage=4 clause=0 asset=0 expected=0 ownerAsset=0
    ARENA chosen=929280 allocfail=188

Reason 20 is `renderer_adapter_fighter.c:4383`: a packed CSS preview reached the
draw path with `native_owner_enabled == FALSE`. That halt is deliberate -- the
comment beside it says a native admission failure is "a visible failure to fix
at its owning seam, never an empty replacement display list" -- and it is
guarded by `NDS_P2_1P_GAME && NDS_RENDERER_HW_TRIANGLES && PROFILE_LEVEL < 2`,
all true in the shipping build. The walk flag changes input, not the render
path.

**It is PRE-EXISTING, not from this batch.** `git diff e802e336bc2..HEAD` over
`renderer_adapter_fighter.c` is +31 lines and touches `native_owner_enabled`
nowhere; the only change there is the facing globals.

**What is proven:** in the shipping configuration, a CSS preview of Ness can
reach the draw path without a native owner and spin the console forever.
**What is not:** that a human hovering Ness hits it. The walk wanders the CSS
quickly and can hover a portrait while a previous preview transaction is still
in flight; ordinary play may never produce that ordering. The owner has played
this CSS repeatedly without reporting a hang, which is evidence for the
ordering mattering.

`stage=4` with clause/asset/expected/ownerAsset all zero says the decline came
from a path that does not populate those witnesses, so the next step is to give
stage 4 its own clause ids rather than to guess. Worth doing before the next
CSS row: a `for(;;)` on a screen the player uses is worse than anything left in
BUGS.md.

Also confirmed in the same run, and it is the first runtime evidence that the
Results repair works: `DEMOPATH unresolved=0 loadfail=0 ok=6` with
`ANIM resolve=6 fallback=0`. Six extern-heap loads resolved and none fell back,
where before this batch the path table had no rows at all.

## 2026-09-22 -- the shipping configuration, walked into a real match. I was wrong about the arena.

`NDS_P2_MENU_WALK=20` drives the shell's own navigation (Title -> Mode -> VS ->
CSS -> stage -> battle) with real input, so it reaches a VS match in the
configuration that actually ships. The Ness preview halt blocked it, so a
probe-only `NDS_PREVIEW_HALT_NONFATAL=1` counts that decline instead of
spinning. `gNdsMenuShellCssWalkTargetKind` is writable, so the walk was pushed
onto Pikachu mid-run and the heap low-water reset before the measured lap.

**Pikachu committed, shipping build, real shell path:**

    ARENA  chosen=929280  freemin=48,216  allocfail=188
    GOBJ   latchBase=0  latchLimit=0  reserveApplied=0
    RAYS   req=2  null=0  cand=100
    APPEAR overrun=0   ANIM resolve=658  fallback=0
    PREVIEWHALT count=56  kindMask=0x800   (bit 11 = Ness, and only Ness)

**RETRACTION. The arena alarm earlier in this file is wrong.** It reasoned from
the 2026-09-20 census figure of 7,556 free at GO for Pikachu/Fox and concluded
that this batch's 8,192-byte image growth might stop that roster starting. The
real number in the shipping build is **48,216**, the `ifCommonSetMaxNumGObj`
latch **never fires**, and the growth is about a sixth of the headroom rather
than more than all of it. Mario/Fox on the same walk reads 85,708 against the
census's 61,124, so the arena has IMPROVED since that census and the figure I
built on was stale. r32 starts. The "fall back to r26" advice is withdrawn.

The lesson is one this file already carries and I repeated anyway: **re-derive a
recorded premise before building on it.** A two-day-old memory figure was used
to justify an urgent warning, a reclaim hunt, a failed optimisation and a
default flip, and one measurement dissolved all of it.

**P03 rays are not arena-starved either.** `req=2 null=0 cand=100` in the
shipping configuration: constructed both times, submitted a hundred times. That
was the last surviving hypothesis for the row, so every cause tested is now
eliminated -- admission, material, matrix kind `0x44`, entry ordering, alpha,
and allocation. Either the repairs in this batch changed it, or the owner's
scenario differs from the walk's (stage, mode, or the second entry).

**The frozen-Fox chain is refuted through the real CSS -> VS path too**, which
is what the direct-battle probe could not reach: `AppearOverrun 0` and
`AnimFallback 0` across **658** animation resolves with Pikachu committed.

**Still true and still worth fixing:** the Ness preview halt. 56 declines across
the run, `kindMask` bit 11 only, so it is exactly one fighter and it is
reproducible on demand.

### The Ness CSS halt, localised to one validator clause

Latched the decline witnesses at the FIRST non-fatal decline (they are per-draw
globals; sampling later reads whichever fighter drew last, which is how an
earlier read came back as Pikachu in a battle). Shipping build, walk driving the
real CSS:

    PREVIEWHALT count=42  kindMask=0x00000800      (bit 11 = Ness, only Ness)
    W    stage=4  owner=9  selected=14  loadedAsset=335  kindPlus1=12
    VALIDATE code=6 slot=9 low=0 root=0 observed=5792 expected=82 rootcount=14

Reading that: Ness's preview (model asset **335**) selects **14 roots**, and
`ndsRendererValidateNativeFighterOwner` rejects at **clause 6, root index 0**.
Clause 6 is the span triple at `nds_renderer_native_fighter_production.c:1232`
-- it fires when ANY of
`ndsRendererNativeAssetSpanFits(root_offset, source_command_count, sizeof(Gfx),
asset_data_size)`, `ndsRendererNativeArraySpanFits(first_epoch, epoch_count,
epoch_count)` or `ndsRendererValidateNativeStateSpan(tail_state_*)` fails. Its
two reported words are the root's own `root_offset` = **5,792** and
`source_command_count` = **82**.

So root 0 wants commands spanning `5792 .. 5792 + 82*8 = 6448` bytes of the
loaded preview asset. The asset-size arm is therefore the first thing to check:
if the packed CSS Model for Ness is smaller than 6,448 bytes at that offset, the
span cannot fit and the owner is declined before a single root is drawn.
`check_preview_pack_owner_sizes.py` is green, but it compares the pack's
declared `source_bytes` against the owner's declared `asset_data_size` -- not
against the size actually passed to the validator, which is
`ndsRelocNativeSourceSize(native_owner_file)` at
`renderer_adapter_fighter.c:3928`. Those are two different numbers and only the
first is currently checked. **That gap is the next thing to close**, and it
would make this a static failure instead of a hang.

**Provenance: pre-existing.** `git diff e802e336bc2..HEAD` touches no Ness owner
or preview data -- the only generated header added this batch is the Kirby star.
The `NDS_BATTLE_CORE_PACKS` archive JSON did change for Ness during the builds,
but it is a report, not a build input, and the size checker agrees pack and
owner still match.

**Not proven:** that a human hovering Ness hits it. The walk crosses portraits
faster than play.

Probe plumbing kept, flag-gated and provably inert: with
`NDS_PREVIEW_HALT_NONFATAL=0` the default build reproduces r32's hash
`594EB9BA...` byte for byte, and the counters, the witness arrays and the
externs are all declared only under the flag.

### Kirby on Saffron, and the arena census is stale across every roster measured

Walked the shipping build with `gNdsMenuShellCssWalkTargetKind = 8` (Kirby) and
`gNdsMenuShellSssWalkTargetGkind = 9` (Yamabuki). Note 9, not 7: the first
attempt used 7 from a comment index in a port-side array instead of the source
enum in `grdef.h`, which is the stale-citation mistake this file already
records. `nGRKindPupupu` is 8 and `nGRKindYamabuki` is 9.

    POST   scene=22  presented=11  loops=3
    ARENA  freemin=32,504  allocfail=188
    GOBJ   latchBase=0  latchLimit=0  applied=0
    KIRBYHAT hits=0,0     KSTAR step=0 fromEffect=0
    ANIM   resolve=731  fallback=0

**No crash and no hang.** The run completed and every counter read back.

**The arena census is now stale on all three rosters it was used to reason
about**, and the differences are not small:

| roster / stage | census 09-20 | measured 09-22 |
|---|---|---|
| Mario/Fox Dream Land | 61,124 | **85,708** |
| Pikachu/Fox | 7,556 | **48,216** |
| Saffron (Mario/Fox then) / Kirby now | 2,844 | **32,504** |

The `ifCommonSetMaxNumGObj` latch fired in none of them. So the arena is not the
blocker for the rays, the frozen Fox, or Saffron -- every row that was routed
there on the strength of those numbers has to come back and be explained some
other way.

**What this run does NOT test: the copy.** `KIRBYHAT hits=0,0` says the copy hat
was never drawn, so the walk never inhaled anything -- it has no input script
for a special. And `presented=11` says the battle had only just begun, while the
Saffron gate cycle runs to roughly tic 1,220 for the close and 2,220 for the
reopen. So the owner's "UPDATE crashes with Kirby" -- which reads as the gate's
open/close update -- was not reached on either axis.

Closing it needs a Saffron battle held past tic 1,220 with a real Kirby copy
performed. The walk cannot do the second part; that wants either a scripted
special or the owner reproducing it once while the counters are readable.

### Saffron gate cycles with Kirby present, and does not crash

Held the walk on Saffron with Kirby committed and sampled across the gate's own
state machine. The gate transitions and the ROM keeps running:

| sample | presented | free-min | gate_status | gate_wait |
|---|---|---|---|---|
| T1 | 572 | 32,504 | 0 | 869 |
| T2 | 816 | 32,504 | 0 | 263 |
| T3 | **2,043** | 32,496 | **200** | 746 |
| T4 | 1,402 | 32,160 | **8** | 1,684 |

Closing counters: `hat=0,0  anim=1585/0  latch=0/0  allocfail=188`.

`gate_wait` counting 869 -> 263 is the source's own timer, and `gate_status`
moving 0 -> 200 -> 8 across the samples is the transition itself -- the "UPDATE"
the owner's row names -- observed in **three distinct states**. T4's lower
presented count is a second match (the counter resets per battle), so this held
across more than one Saffron entry. Free-min moved 344 bytes over the whole run,
the `ifCommonSetMaxNumGObj` latch never fired, and 1,585 animation resolves
produced zero fallbacks.

**So the gate update alone does not crash with Kirby on the stage.** What is
still untested is the copy: `gNdsNativeKirbyHatTableHits` stayed 0,0 throughout,
because the walk has no input script for a special and never inhales. The
recorded copy cost is two eager hat details of about 8,556 bytes each, so a copy
would move free-min from 32,504 to roughly 14,000 -- still positive, but under
the 25,600 latch floor, which is exactly where the owner's other Kirby symptoms
live.

Closing this row therefore needs one thing the walk cannot produce: a real
inhale on Saffron. Either script a special into the walk, or have the owner do
it once while the counters are readable.

### Ness halt, narrowed one step further

`ness_high.bin` is **16,240 bytes**, and root 0's span is
`5,792 + 82 * 8 = 6,448`. So the asset-span arm of clause 6 has room, and the
failure is more likely `ndsRendererNativeArraySpanFits(first_epoch, epoch_count,
epoch_count)` or `ndsRendererValidateNativeStateSpan(tail_state_*)`. Note the
validator is passed `ndsRelocNativeSourceSize(native_owner_file)` rather than
the file length, so confirm which number it actually sees before acting.
Splitting clause 6 into three sub-codes is a handful of lines and would name the
arm outright on the next run.

---

## 2026-09-22 04:40 -- per-slot read of the frozen-Fox row, and a retraction

**The earlier refutation of this row was unsound and is withdrawn.** It rested
on `AppearOverrun 0` and `AnimFallback 0` across 658 resolves. Both counters
are single `volatile u32` (`src/port/diagnostics_state.c:187-188`), **not
per-slot**, so 658 resolves is equally consistent with all 658 being Pikachu's
and Fox never animating once. And neither counter addresses the reported
symptom: "cannot be hit" is collision and "frozen" is the physics proc.
`gNdsFTCommonAppearOverrunFighter` is narrower still -- `src/import/
battleship_ftcommon_entry.c` arms it ONCE, for the FIRST fighter only, and only
after 240 updates in Appear specifically.

**Re-measured per slot.** Walk build, `gNdsMenuShellCssWalkTargetKind = 9`
(Pikachu), reading each fighter's own FTStruct through
`gSCManagerBattleState->players[i].fighter_gobj->user_data.p`:

    T1  pres=252   P0 kind=9 st=69 mo=-2 tics=27  | P1 kind=1 st=18 mo=12 tics=3
    T2  (poisoned -- see below)
    T3  pres=~900  P0 kind=9 st=10 mo=4  tics=147 | P1 kind=1 st=12 mo=6  tics=20
    T4  pres=1469  P0 kind=9 st=10 mo=4  tics=375 | P1 kind=1 st=12 mo=6  tics=14
    END latch=0/0  anim=1009/0  freemin=48,432  allocfail=188

The pairing is the owner's exact one: kind 9 is Pikachu in slot 0, kind 1 is
Fox in slot 1. Status 18 is `nFTCommonStatusTurn`, 12 is `WalkMiddle`, 10 is
`Wait`, 69 is `DownWaitD`.

**Fox is alive.** It turns in the first match and walks in the second, and its
`status_total_tics` goes 20 then 14 -- it does not accumulate, so Fox
re-entered the status in between. Pikachu meanwhile sits in `Wait` with tics
advancing 147 to 375, which is an idle CPU, not a stuck one.

**What this does NOT cover, stated plainly.** The walk drives two CPUs. The
owner plays slot 0 as a human. A Fox on a HUMAN slot with no second controller
would stand still legitimately -- though it would still be hittable, and the
owner's is not. The row needs Fox's player type and level.

**Method note for the next run: the walk loops through whole matches, so a
sample 600 frames later can land in a different match or between two.** T2 read
`kind=-572662310`, `st=-286331154` -- 0xDD/0xEE freed-memory poison.
`gSCManagerBattleState->players[i].fighter_gobj` is NOT cleared on teardown, so
a stale pointer reads poison rather than NULL. Guard every sample on
`0 <= fkind <= 11` before believing it.

**Separately, a counter this file reported half of.** The Poke Ball rays row
was closed on `req=2 null=0 cand=100`. The rays have TWO renderer-side
counters: `gNdsMBallRaysCandidateCount` (`renderer_adapter_stage.c:5760`)
counts REACHING the material check, and `gNdsMBallRaysMaterialRejectCount`
(`:5791`, `:5802`) counts FAILING it -- which returns FALSE and bumps
`gNdsEntryEffectNativeFallbackCount`, a non-native fallback. Both key off root
offsets 0x0440 and 0x0518, exactly `MBALLRAYS_ROOTS`. A hundred candidates with
the reject counter unread says nothing about whether a ray drew.

## 2026-09-22 05:05 -- frozen Fox settled for this configuration, rays counter read, and four RED checkers that are not mine

### Frozen Fox: twelve within-match samples, and Fox is acting

The previous read was one sample per match. This one samples every 41 frames
inside a single match, guarded on `0 <= fkind <= 11` so a torn-down match
cannot be mistaken for data:

    T01 pres=122 P0 k=9 st=10  mo=4   tics=155 | P1 k=1 st=29  mo=23  tics=9
    T02 pres=163 P0 k=9 st=10  mo=4   tics=237 | P1 k=1 st=28  mo=22  tics=1
    T03 pres=204 P0 k=9 st=10  mo=4   tics=319 | P1 k=1 st=10  mo=4   tics=5
    T04 pres=245 P0 k=9 st=69  mo=-2  tics=13  | P1 k=1 st=10  mo=4   tics=9
    T05 pres=286 P0 k=9 st=69  mo=-2  tics=95  | P1 k=1 st=12  mo=6   tics=60
    T06 pres=327 P0 k=9 st=69  mo=-2  tics=177 | P1 k=1 st=225 mo=200 tics=2
    T07 pres=368 P0 k=9 st=37  mo=31  tics=5   | P1 k=1 st=225 mo=200 tics=33
    T08 pres=409 P0 k=9 st=10  mo=4   tics=18  | P1 k=1 st=10  mo=4   tics=9
    T09 pres=450 P0 k=9 st=10  mo=4   tics=20  | P1 k=1 st=225 mo=200 tics=19
    T10 pres=491 P0 k=9 st=85  mo=73  tics=45  | P1 k=1 st=12  mo=6   tics=10
    T11 pres=532 P0 k=9 st=85  mo=73  tics=127 | P1 k=1 st=18  mo=12  tics=3
    T12 pres=573 P0 k=9 st=85  mo=73  tics=209 | P1 k=1 st=12  mo=6   tics=74
    END pres=573 loops=3 freemin=48,432 latch=0/0 anim=828/0 appearOverrun=0

Fox (kind 1, slot 1) changes status NINE times across twelve samples, with
`status_total_tics` resetting at each change: 29, 28, 10, 10, 12, 225, 225, 10,
225, 12, 18, 12. It waits, walks, turns and runs a character-specific 225.
Pikachu (kind 9, slot 0) is also live, 10 -> 69 -> 37 -> 10 -> 85.

**Fox is not frozen in a CPU-vs-CPU Pikachu-vs-Fox match on this build.** That
is a properly scoped refutation, unlike the earlier one. What it still does not
cover is the owner's configuration, where slot 0 is a human.

### The rays counter this file previously reported half of

    RAYS req=1 null=0 cand=50 reject=0 efFallback=0

A Poke Ball was requested and constructed, reached the material check fifty
times, and was rejected ZERO times, with zero entry-effect native fallbacks.
So the rays are built, admitted, material-accepted and submitted natively.
Whatever the owner sees is downstream of submission or in a scenario this walk
does not reach.

### Four python checkers are RED at HEAD, and none of them is from this batch

Ran all 21. Green: audio-ordinals, decomp-header-mirror, dtcm-residency,
sin-table-dedup, r2-light-stretch, r2-shade-twin, untracked-dependencies,
hidden-part-root-coverage, model-part-mutation-coverage,
native-owner-geometry-closure, native-owner-image-spans,
native-owner-source-matrix-precision, nds-native-owner-packet,
preview-pack-owner-sizes, results-demo-motion-closure, collision-parity.

RED, with provenance established rather than assumed:

| checker | failure | why it is not from this batch |
|---|---|---|
| `check-native-owner-wiring.py` | `owners=50 failed_owners=4 gaps=10` -- kirby_vulcan, ness_pktail, pikachu_thunder, samus_bomb lack Makefile PREREQ variables and grouped emit rules | Reproduces **identically** at the pre-session commit `acbeb9f6e8a`. `git log -S` shows all four PREREQ variable names have **never** appeared in this repository. |
| `check_nds_native_stage.py` | `M3_STAGE_FALSIFIER: reloc_backend_renderer_dl.c:ndsRendererAdapterBuildDObjXObjMatrix: unclassified reads ['dobj.xobjs_num']` | Reproduces **identically** at `acbeb9f6e8a`. That file was not touched in this batch. |
| `check_nds_native_owner_hierarchy.py` | `ValueError: mario: retained packet corner trace mismatch` | Inputs are the owner IR and the decomp O2R data. The owner IR's mtime is 2026-09-21 21:35, before this session; the only generated file this batch changed is the image header, whose diff is 56 lines and Ness-only. The failure names Mario. |
| `check_native_owner_weld_consistency.py` | `IndexError: list index out of range` at `binding_joints[binding]` | Same two inputs, same reasoning. This one is a script crash rather than a data assertion. |

Method note: a plain `git worktree` at the pre-session commit cannot run the
last two, because `src/nds/nds_native_fighter_owner.generated.inc` is
**untracked** and `decomp/` is gitignored. Junctioning `decomp/` and copying
`src/nds/generated/` still leaves the owner IR absent, which is why those two
were settled by input invariance instead of by reproduction. The worktree was
removed after the run.

**These four remain owed and are not claimed as passing.**

---

## 2026-09-22 05:40 -- the Ness character-select fix, verified at runtime

Static evidence was already exact (header 177 -> 195, `ness_high.bin` 16,240 ->
17,088, all 1,140 root spans passing), but none of it had run. Built the
supported walk target `smash64ds-p2-shell-loop-hwtri` into
`builds/build-ness-walk/` -- **not** the published ROM, which keeps its own
TARGET name -- and drove the real character select with
`gNdsMenuShellCssWalkTargetKind = 11` (Ness).

    N1 pres=0 loops=6   validate code=0 slot=0 root=0 obs=0 exp=0 cnt=0 | packFail=0 kind=0
    N2 pres=0 loops=9   validate code=0 slot=0 cnt=0                    | packFail=0 kind=0
    N3 pres=0 loops=13  validate code=0 slot=0 cnt=0                    | packFail=0 kind=0
    N4 pres=0 loops=18  validate code=0 slot=0 cnt=0                    | packFail=0 kind=0
    END stepCount=4724 commit=187 cancel=99

**Reading it.** `ndsPreviewPackLoadHalt` writes its reason into
`gNdsPreviewPackFailure` and calls `DC_FlushAll()` BEFORE entering its
`for (;;)`. `packFail` is 0 at every sample, so the halt never fired -- for any
fighter, not just Ness. `gNdsNativeFighterValidateRejectCode` is 0, where it
previously read 6. And `gNdsMenuShellWalkLoops` advances 6 -> 9 -> 13 -> 18,
so the shell completed eighteen whole menu loops; a `for (;;)` would have
frozen that number. 4,724 resumable load steps and 187 committed preview packs
say the previews were loading in volume, not being skipped.

`pres=0` is expected: this is a menu walk that never enters a battle, and that
counter is battle-only. `freemin` reads its uninitialised sentinel for the same
reason.

**What this run does not by itself show** is that Ness specifically was
hovered. That is covered by the 2026-09-21 reading on the non-fatal build --
`PREVIEWHALT count=56 kindMask=0x800`, bit 11 and only bit 11 -- which proves
the walk does reach Ness's preview in this harness family. The corroborating
`gNdsMenuShellCssWalkTourKindMask` / `TourDrewMask` read is in the next run.

The published ROM was untouched throughout: `smash64ds.nds` still hashes
`18cf0cd3aac388fd83bd80838c927d88059a2623df7f022c0b478541d6f9036c`, identical
to `builds/remaining-bugs-playtest-r35/`.

## 2026-09-22 06:00 -- Ness confirmed per fighter, and the tour prices the hover-delay row

Second run on the same walk ROM, reading the CSS tour's own coverage masks.
`ndsMenuShellCssWalkTourStep` parks on each kind for
`NDS_CSS_WALK_TOUR_HOLD_TICS = 24`, baselines the P0 hardware triangle count,
and sets a bit in `DrewMask` only if that kind's preview added triangles. The
declaring comment states the intent: *"A bit set in Kind but clear in Drew is a
fighter whose preview is invisible, which is the whole point of the tour."*

    CSSTOUR kindMask=0x00000fff drewMask=0x00000c13
    PREVIEW loads=147 packFail=0 failKind=0 commit=147 cancel=74 validateCode=0
    END loops=13

**Ness is confirmed at the fighter level.** `kindMask` 0xFFF is bits 0..11, so
the tour parked on all twelve fighters including Ness (bit 11). `drewMask`
0xC13 is bits 0, 1, 4, 10 and 11 -- Mario, Fox, Luigi, Purin and **Ness**. So
Ness's preview was selected, loaded and DREW, with zero pack failures and a
validator reject code of 0. Before the fix that same preview reached
`ndsPreviewPackLoadHalt(20)`. That closes the Ness row at runtime, per
fighter, not just "nothing hung".

**And the same masks price the owner's deferred hover-delay row.** Seven kinds
are in `kindMask` but not `drewMask`: Donkey (2), Samus (3), Link (5), Yoshi
(6), Captain (7), Kirby (8) and Pikachu (9). Read that carefully -- it is NOT
seven invisible previews. The tour holds only 24 tics and the loader is
resumable, and this run cancelled 74 transactions against 147 commits, so a
clear bit here means *this preview did not finish inside 24 tics*, which is
the hover-delay row ("delay between cursor hover and 3d fighter preview
rendering", owner-deferred) measured per fighter for the first time. Five
fighters make the 24-tic window and seven do not, and the seven are named.

That is a lead for that row rather than a defect in this one. Whoever takes it
should raise `NDS_CSS_WALK_TOUR_HOLD_TICS` and re-read the masks: the dwell at
which each of the seven starts drawing IS its load time in tics.

## 2026-09-22 06:20 -- the last two rows, measured in the owner's own configuration

Battle-entering walk (`NDS_P2_MENU_WALK` shell loop that reaches VS), Pikachu
targeted, eight samples 51 presented frames apart inside one match:

    B01 pres=203 P0 k=9 pk=0 lv=3 st=10  tics=317 gh=0 | P1 k=1 pk=1 lv=2 st=10  tics=3  gh=0
    B02 pres=254 P0 k=9 pk=0 lv=3 st=69  tics=31  gh=0 | P1 k=1 pk=1 lv=2 st=18  tics=7  gh=0
    B03 pres=305 P0 k=9 pk=0 lv=3 st=69  tics=133 gh=0 | P1 k=1 pk=1 lv=2 st=225 tics=6  gh=0
    B04 pres=356 P0 k=9 pk=0 lv=3 st=10  tics=14  gh=0 | P1 k=1 pk=1 lv=2 st=225 tics=9  gh=0
    B05 pres=407 P0 k=9 pk=0 lv=3 st=10  tics=14  gh=0 | P1 k=1 pk=1 lv=2 st=10  tics=5  gh=0
    B06 pres=458 P0 k=9 pk=0 lv=3 st=37  tics=5   gh=0 | P1 k=1 pk=1 lv=2 st=225 tics=35 gh=0
    B07 pres=509 P0 k=9 pk=0 lv=3 st=85  tics=81  gh=0 | P1 k=1 pk=1 lv=2 st=12  tics=46 gh=0
    B08 pres=560 P0 k=9 pk=0 lv=3 st=85  tics=183 gh=0 | P1 k=1 pk=1 lv=2 st=12  tics=48 gh=0
    END pres=560 loops=3 latch=0/0 anim=828/0

### Frozen Fox: this IS the owner's configuration, and Fox is not frozen

`pkind` 0 is `nFTPlayerKindMan`, 1 is `nFTPlayerKindCom`. **P0 is Pikachu as a
HUMAN (pk=0, level 3); P1 is Fox as a CPU (pk=1, level 2).** That is exactly
the reported setup, and it is not an artefact of the harness -- the walk
build's preset says so in source (`nds_match_config.c`, under
`NDS_DEV_LIVE_INPUT_PREVIEW`: *"The shipped match: one-minute Time, items off,
Fox on the CPU"*).

Fox changes status SIX times across eight samples -- 10, 18, 225, 225, 10,
225, 12, 12 -- with `status_total_tics` resetting at each change. It waits,
turns, runs a character-specific 225 twice and walks.

**And `is_ghost` is 0 for both fighters at every sample.** That matters more
than the status churn: `ftCommonAppearInitStatusVars` sets `is_ghost`, and a
fighter stuck in Appear is both unresponsive and UNHITTABLE -- which is the
only mechanism this file ever had for "frozen and cannot be hit". Measuring it
directly at zero, in the owner's configuration, refutes that mechanism rather
than merely failing to observe the symptom.

The GObj latch never fired and 828 animations resolved with 0 fallbacks.

### Poke Ball rays: they draw

    RAYS    req=1 null=0 cand=50 reject=0 efFallback=0
    RAYDRAW root41=25 root42=25 totalDraw=1862 alphaSkip=180
    ITEMS   rate=0 toggles=0x00000000

`gNdsEntryEffectNativeRootDraws[root]` increments immediately before
`return TRUE` on a completed native draw, so this is the stage past
"candidate" that every previous reading stopped short of.
**Both ray roots drew 25 times each -- 50 draws, matching `cand=50` exactly**,
with zero material rejects and zero native fallbacks. `alphaSkip=180` is the
designed tail: the alpha-skip site's own comment records that MBallRays'
PRIMCOLOR ramp reaches alpha 0 at source tick 50 while its rotation track runs
to 130.

So the chain is complete and clean end to end: requested, constructed,
material-accepted, submitted, and DRAWN for exactly its 50-tick visible ramp.

Note `rate=0 toggles=0` -- items are OFF in this preset, yet a ray request
still occurred, so these rays come from the fighter-entry path rather than an
item spawn. A Poke Ball ITEM scenario is therefore still unexercised here, and
that is the remaining difference from the owner's report.

## 2026-09-22 06:50 -- the Poke Ball rays row: a commented-out call behind a stale reason

The render path was already proven (`root41=25 root42=25` from the fighter
ENTRY trigger). The item trigger was not, so I built
`NDS_HARNESS_ITEMS_ON=1` to force the preset's items on and measured again:

    ITEMS rate=5 toggles=0xffffffff   (VeryHigh, every item enabled)
    I01..I06  RAYS req=0 null=0 cand=0 rej=0 | RAYDRAW r41=0 r42=0 aSkip=0
              actor make=12..18 refused=0 | latch=0/0
    BALL arenaBefore=0 arenaCost=0
    END efFallback=0 totalEfDraw=0 anim=8783/0

The flag worked and the item system ran -- `gNdsItemAppearActorMakeCount`
climbed 12 to 18 with `gNdsItemAppearActorRefusedCount` 0, the counter added
earlier this session. **But `pres=0`: this walk target loops menus and never
enters a battle, so no ball ever spawned.** `req=0` is consistent with the
finding below and does not isolate it. Not claimed as an A/B.

**The root cause is in the source, and it is plain.**
`src/import/battleship_item_mball.c` carried, inside `itMBallOpenInitVars`:

    /* efManagerMBallRaysMakeEffect(&dobj->translate.vec.f) -- deferred; see
     * the note above the status-desc table. */
    ip->item_vars.mball.effect_gobj = NULL;

The note it points at says the desc is named "in exactly one place --
battleship_efmanager.c:1493, inside NDS_EF_ROSTER_DESCS_PIKACHU -- so with
Pikachu off the desc does not exist and the maker cannot be linked".

**That stopped being true.** The desc now has its own shared row,
`NDS_EF_ROSTER_DESCS_MBALL_RAYS` (`battleship_efmanager.c:1448`), gated
`NDS_P2_PIKACHU || NDS_P2_PURIN` and deliberately kept single so the two never
double-count the resolver capacity. Both flags are 1 in the shipping config,
and `nm smash64ds.elf` finds BOTH `dEFManagerMBallRaysEffectDesc` and
`efManagerMBallRaysMakeEffect` already linked. Nothing was missing but the
call.

**Source parity, which is this project's oracle:**
`decomp/.../it/itcommon/itmball.c:400` is exactly
`ip->item_vars.mball.effect_gobj = efManagerMBallRaysMakeEffect(&dobj->translate.vec.f);`
and its three sites 343 / 400 / 451 map onto the port's 436 / (restored) / 550.
The port had two of the three. The two it had are the ones that keep the rays
following the ball, and they are dead code today because they NULL-check a
field nothing ever assigns.

Restored under `#if NDS_P2_PIKACHU || NDS_P2_PURIN` -- the same condition that
admits the desc -- so a build with neither fighter keeps today's behaviour
exactly. The maker returns NULL on a full effect pool, which is the source's
own empty-pool path and what both consumers already expect. No release path is
added because the source has none either; effects retire themselves.

**Honest limit:** this is proven by source parity and linked symbols, not by a
runtime before/after, because the menu-loop walk never spawns a ball. The
owner's playtest is the runtime check.

**And a standing warning the stale note earns:** the same file defers
`itMainSetAppearSpin` and `efManagerItemSpawnSwirlMakeEffect` for reasons of
the same vintage. Those are NOT covered here. Check their stated reasons
against the tree before assuming they still hold -- this one did not.

## 2026-09-22 07:20 -- r36 boot smoke test

I had never booted the ROM the owner will actually play. Ran
`builds/remaining-bugs-playtest-r36/smash64ds.nds` (sha256 `1CECBEF5...3BA1`)
unattended for 120 s on its own runner slot:

    alive=True cpu=120.1s
    BOOT    scene=60 camFrames=0 mallocOverflow=0 objmanPanic=0
    PREVIEW packFail=0 validateCode=0

Scene 60 is `nSCKindExplain`, the How-to-Play screen in the attract loop
(title -> explain -> auto demo), which is where an idle boot belongs after two
minutes. `camFrames=0` is expected: that counter is the battle camera and no
battle was entered.

`gNdsSyMallocOverflowCount` 0 and `gNdsObjmanPanicCount` 0 matter most --
`malloc.c:30` is `while (TRUE);`, so an arena overrun hangs the console rather
than declining, and this batch moved arena in three places (the Kirby low-detail
hat deferral, the Ness owner image growing 848 bytes, and the restored Poke
Ball rays GObj). None of them overran. `packFail=0` and `validateCode=0`
confirm the Ness preview-pack path stays clean through the attract loop too.

This is a boot smoke test, not an acceptance run: no battle was played and no
timing was measured.

## 2026-09-22 07:35 -- frozen Fox: the second half of the symptom, measured

Every earlier reading addressed "frozen". None addressed "cannot be hit", which
is a separate subsystem -- collision, not animation or physics. `FTDamageColl`
carries `hitstatus`, and `include/ft/fighter.h:2136` gives its values:
`nGMHitStatusNone = 0`, `Normal = 1`, `Invincible = 2`, `Intangible = 3`. A
fighter that cannot be hit reads 0, 2 or 3 there.

Eight samples inside one match, human Pikachu (slot 0) versus CPU Fox (slot 1),
reading `damage_colls[0..5].hitstatus` and `is_damage_coll_modify` for both:

    H01 pres=114 P0 st=10  hs=1,1,1,1,1,1 mod=0 | P1 st=28  hs=1,1,1,1,1,1 mod=0
    H02 pres=165 P0 st=10  hs=1,1,1,1,1,1 mod=0 | P1 st=33  hs=1,1,1,1,1,1 mod=0
    H03 pres=216 P0 st=10  hs=1,1,1,1,1,1 mod=0 | P1 st=10  hs=1,1,1,1,1,1 mod=0
    H04 pres=267 P0 st=69  hs=1,1,1,1,1,1 mod=0 | P1 st=12  hs=1,1,1,1,1,1 mod=0
    H05 pres=318 P0 st=69  hs=1,1,1,1,1,1 mod=0 | P1 st=225 hs=1,1,1,1,1,1 mod=0
    H06 pres=369 P0 st=37  hs=1,1,1,1,1,1 mod=0 | P1 st=225 hs=1,1,1,1,1,1 mod=0
    H07 pres=420 P0 st=10  hs=1,1,1,1,1,1 mod=0 | P1 st=225 hs=1,1,1,1,1,1 mod=0
    H08 pres=471 P0 st=85  hs=1,1,1,1,1,1 mod=0 | P1 st=10  hs=1,1,1,1,1,1 mod=0
    END pres=471 loops=3 latch=0/0

**Every hurtbox on both fighters is Normal at every sample**, and Fox's status
changes seven times across the eight (28, 33, 10, 12, 225, 225, 225, 10).

So both halves of the reported symptom are now refuted by direct measurement
rather than by absence of evidence:

| half of the symptom | instrument | reading |
|---|---|---|
| "frozen" | `status_id` + `status_total_tics`, per slot | changes 7 of 8 samples, tics reset each time |
| "cannot be hit" | `damage_colls[].hitstatus`, per slot | Normal (1) on all six, all samples |
| the mechanism that would cause both | `is_ghost` | 0 at every sample |

**The row stays OPEN anyway, and that is deliberate.** Three independent
measurements failing to reproduce a symptom is not the same as fixing it, and
this file already carries one refutation of this row that I had to withdraw
for exactly that overreach. What is now true is narrower and worth stating
precisely: in a fresh VS match on Dream Land, human Pikachu against a level-2
CPU Fox, Fox is neither frozen nor unhittable, and the stuck-in-Appear
mechanism is not present.

What would move it: the stage, the stock/time setting, and whether it happens
from the first second or only after some event. Sudden death re-creating the
fighters is still the strongest clue in the report and still points at
creation-time state, which none of these samples can see because they all
start after creation succeeded.

## 2026-09-22 07:50 -- a reframing of the frozen-Fox row that should have come first

Every instrument I have pointed at this row is GAMEPLAY-side: `status_id`,
`status_total_tics`, `is_ghost`, `damage_colls[].hitstatus`, animation resolves,
the GObj latch. All read healthy. The owner's observation is VISUAL.

**Those two are not in contradiction, and I spent the night treating them as
if they were.** A fighter whose RENDER is stale -- drawing one pose while its
gameplay state advances underneath -- looks frozen, and looks like hits do
nothing, while every counter above reads exactly what I measured. So "I cannot
reproduce it" is weaker than it sounded: I have been measuring a different
layer from the one the report describes.

That is why the repro guide now asks, as its own question, whether Fox
animates at all -- idle breathing, turning -- or is a statue. Animating but
unresponsive and completely static are different failures, and only the owner
can see which.

**What this does NOT license.** I looked at the fighter packet cache as the
obvious stale-render candidate and stopped short of claiming it, because the
replay path demonstrably patches pose matrices every frame:
`ndsFighterPacketStoreMatrix4x3(&words[root->local_index[j]],
&input->gx_locals[j])` per root per local, with a split-matrix branch covering
`gx_valid == 0`. A stale pose through that path would need the INPUTS to be
stale, which is upstream of the cache, and nothing measured says they are.
Recording the candidate and its counter-evidence rather than another mechanism
I cannot test.

The available counters are global rather than per slot
(`gNdsFighterPacketHits` / `Records` / `Declines` / `Faults` / `MissWord`),
which is the same granularity trap that made my first refutation of this row
unsound. If this lead is taken up, make them per slot first.

## 2026-09-22 08:05 -- whole-match Fox coverage, and one anomaly worth chasing

Every previous Fox reading stopped by ~560 presented frames, about nine
seconds. The preset is a ONE-MINUTE Time match, so I had only ever tested the
opening. Sixteen samples across five complete match loops, out to pres=1942:

    L01 pres=1862 P0 st=10 tics=340 | P1 st=12  tics=49  hs=1,1,1 gh=0
    L02 pres=118  P0 st=10 tics=147 | P1 st=29  tics=1   hs=1,1,1 gh=0
    L03 pres=419  P0 st=10 tics=38  | P1 st=225 tics=4   hs=1,1,1 gh=0
    L04 pres=720  P0 st=2  tics=181 | P1 st=10  tics=7   hs=1,1,1 gh=0
    L05 TORN DOWN pres=757
    L06 pres=136  P0 st=10 tics=0   | P1 st=223 tics=0   hs=1,1,1 gh=1
    L07 pres=437  P0 st=10 tics=485 | P1 st=10  tics=23  hs=1,1,1 gh=0
    L08 pres=738  P0 st=10 tics=0   | P1 st=12  tics=44  hs=1,1,1 gh=0
    L09 pres=1039 P0 st=10 tics=371 | P1 st=12  tics=104 hs=1,1,1 gh=0
    L10 pres=1340 P0 st=10 tics=117 | P1 st=12  tics=15  hs=1,1,1 gh=0
    L11 pres=1641 P0 st=10 tics=719 | P1 st=232 tics=22  hs=1,1,1 gh=0
    L12 pres=1942 P0 st=57 tics=11  | P1 st=12  tics=28  hs=1,1,1 gh=0
    L13 TORN DOWN pres=2043
    L14 pres=72   P0 st=5  tics=0   | P1 st=5   tics=0   hs=1,1,1 gh=1
    L15 pres=373  P0 st=10 tics=357 | P1 st=10  tics=21  hs=1,1,1 gh=0
    L16 pres=674  P0 st=52 tics=5   | P1 st=208 tics=10  hs=1,1,1 gh=0
    END loops=5 latch=0/0 anim=1136/0 appearOverrun=0

Fox stays alive and hittable for whole matches, not just their openings: its
status keeps changing (12, 29, 225, 10, 223, 12, 12, 232, 12, 5, 10, 208), all
hurtboxes read Normal at every sample, and `is_ghost` is 0 except at L06 and
L14 where both fighters read 1 -- that is the entry/Appear state at a match
start, transient and correct.

**The anomaly: `damage` is 0 for BOTH fighters at ALL SIXTEEN samples across
five complete matches**, while Fox runs character-specific attack statuses
(225, 223, 232, 208). Nothing ever connects, in either direction.

That is the first reading all session that resembles the owner's "cannot be
hit". It is NOT yet a finding, for two reasons worth stating before anyone
builds on it:

- P0 is a human slot the walk never gives input to, so P0 never attacks. Only
  Fox's attacks could land, and a level-2 CPU may simply not close.
- The obvious instrument is guarded out. `gNdsCfxFighterDamagePhaseCalls` and
  `gNdsCfxFighterDamagePhaseHits` (`battleship_gmcollision.c:174-183`) sit
  inside `#if NDS_TICK_HUD`, which is **0** in both the walk build and the
  shipping build -- the symbols are not even in the ELF. A probe reading them
  would have returned 0/0 and meant nothing. Checked before interpreting
  rather than after.

The follow-up now running reads the thing that actually discriminates: the two
fighters' world positions each sample. If they are always far apart, zero
damage is fully explained and there is no anomaly. If they are repeatedly
adjacent while Fox attacks and damage never moves, that is a real lead on
"cannot be hit" -- and the next step would be a build with `NDS_TICK_HUD=1` to
read the collision phase directly.

## 2026-09-22 08:20 -- the damage anomaly tested, and withdrawn

The previous section flagged "damage 0 for both fighters across five matches"
as the first reading resembling "cannot be hit", and said explicitly it was not
yet a finding. Tested it by reading both fighters' world positions:

    S01 pres=63   P0 st=10  pos=0,0        | P1 st=12  pos=-1253,904  dx=1253  stocks 0/0
    S02 pres=314  P0 st=69  pos=1870,0     | P1 st=225 pos=932,0      dx=938   stocks 0/0
    S03 pres=565  P0 st=85  pos=2375,-327  | P1 st=12  pos=590,0      dx=1785  stocks 0/0
    S06 pres=132  P0 st=221 pos=0,0        | P1 st=223 pos=-1397,904  dx=1397  stocks 2/2
    S07 pres=383  P0 st=10  pos=0,0        | P1 st=12  pos=-1028,0    dx=1028  stocks 2/2
    S08 pres=634  P0 st=69  pos=-483,0     | P1 st=208 pos=718,0      dx=-1201 stocks 2/2
    S09 pres=885  P0 st=10  pos=-931,0     | P1 st=12  pos=-832,0     dx=-99   stocks 2/2
    S10 pres=1136 P0 st=10  pos=-1528,0    | P1 st=12  pos=-81,0      dx=-1447 stocks 2/2
    S11 pres=1387 P0 st=10  pos=-1626,0    | P1 st=12  pos=-1164,0    dx=-462  stocks 2/2
    S12 pres=1638 P0 st=10  pos=-1686,0    | P1 st=232 pos=-1157,1542 dx=-529  stocks 2/2

**Withdrawn.** The fighters do approach -- S09 has them 99 units apart,
adjacent -- but at that sample Fox is in status 12 (WalkMiddle), not
attacking. At every sample where Fox IS attacking (225, 223, 208, 232) the gap
is 938, 1397, 1201 and 529. At 251-frame spacing these are snapshots, and I
never caught an attack at contact range. A level-2 CPU that mostly walks and
occasionally attacks at distance produces exactly this reading. Stocks stay
2/2 throughout, consistent with no damage rather than evidence of a defect.

So zero damage is NOT established as anomalous, and nothing here supports a
collision failure. Recorded because the flagged lead deserved a test and the
test came back negative -- a lead that is only ever flagged and never closed
is how a stale citation gets born.

**What would actually settle it**, if this row is picked up again: build the
walk with `NDS_TICK_HUD=1`. That compiles in
`gNdsCfxFighterDamagePhaseCalls` / `Hits` (`battleship_gmcollision.c:174-183`),
which count every fighter-attack-versus-damage-collision evaluation and every
one that connects. Calls>0 with Hits==0 over a match would be a genuine
collision failure; Calls==0 would mean the phase never runs. Both builds in use
tonight have `NDS_TICK_HUD 0`, so those symbols are not in either ELF.

## 2026-09-22 09:10 -- r36 confirmed reproducible, and my second retracted alarm

Noticed the two shared particle-bank generated files dirty after the lab
builds, rebuilt the default to restore them, and got a DIFFERENT ROM:
`73F9D5CF...` where r36 is `1CECBEF5...`, with `git status` empty. I read that
as "r36 was built from lab-contaminated assets" and said so.

**That was wrong, and the correction is measured.** Cleared the gitignored
generated tree (`src/nds/generated/`, restoring the one TRACKED file in it,
`nds_native_damage_slash.generated.inc`) plus `builds/build`, and built twice:

    pass 1  MAKE EXIT 0  ROM 1cecbef5...3ba1
    pass 2  MAKE EXIT 0  ROM 1cecbef5...3ba1   DIRTY 0

**From a fully regenerated tree the build is deterministic and produces
exactly r36.** So r36 is correct and reproducible; `73F9D5CF` was the one-off
-- an INCREMENTAL build over the mixed lab-flavoured artifacts -- not the
other way round. 21 of 21 checkers green on the regenerated tree, and the
published snapshot still matches the root ROM byte for byte.

**What IS real from the episode**, and worth keeping:

- Mixing lab-flag builds with shipping builds leaves the gitignored generated
  tree inconsistent, and `make` cannot detect it because it compares mtimes
  and never sees that FLAGS changed. An incremental build over that state can
  produce a ROM that a clean build does not reproduce.
- The guard is cheap: after any non-default-flag build, rebuild the default
  and require the hash to be unchanged. If it is not, clear the generated tree
  and build twice before trusting either result.

**And the pattern is worth naming, because this is the second time tonight.**
The batch opened with an arena alarm -- "this may stop Pikachu/Fox starting"
-- built on a stale census figure, retracted after one measurement. It closes
with a reproducibility alarm built on one incremental build, retracted after
two clean ones. Both times the alarm was raised on a single observation and
published before the cheap confirming measurement was taken. The confirming
measurement cost about twenty minutes each time.
