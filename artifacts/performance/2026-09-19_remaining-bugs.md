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
