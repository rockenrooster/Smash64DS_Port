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
