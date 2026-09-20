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
