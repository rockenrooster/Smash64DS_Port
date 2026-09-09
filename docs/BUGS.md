**Follow Bug fixing workflow contained in `BUG_FIXING_PROCESS.md`.**
AI Agent should mark fixed items with **FIXED** prefix or a 20 word summary (or less) if not fixed yet, always using ** ** to encapsulate agent added text.
(Investigation details live in `docs/p2/BUG_NOTES.md`; this queue stays lean.)

treat anything not going through native renderer a failure.

**Every newly built ROM, including diagnostics, must exclude non-native renderer implementations. Runtime defaults and zero observed fallbacks are insufficient.**

Owner notes: This isn't meant to be comprehensive, just my quick observations:

Main Menu:
-1P mode not selectable — **Paused by owner; do not resume campaign work.**
-VS Mode is perfect and should be used as a reference as well as the main menu for the
-Data not selectable — **Native entry is documented; children and natural-path acceptance remain unverified. Keep behind match repairs.**

VS mode CSS:
-Kirby not selectable — **Shared CSS residency/storage blocker; integrate compact previews and classify the libfat stop before enabling the roster.**
-Jigglypuff not selectable — **Same shared CSS residency/storage blocker; no independent rendering fix established.**
-Ness not selectable — **Same shared CSS residency/storage blocker; no independent rendering fix established.**

For 1P mode CSS, look at the VS Mode CSS since they are VERY similar.

**Match rendering regressions (owner, 2026-09-08):**
-KO blast pillar — **OPEN: thin yellow streaks instead of the intended blast pillar; owner screenshot preserved.**
-Shield texture — **OPEN: blocky red bands around Mario; owner screenshot preserved. Native packet tests do not establish visual correctness.**

Stages:
-peaches castle: NOT FIXED The steep red roof on the upper central tower is missing large portions of its polygon faces. Only several narrow triangular strips/edges render, leaving large transparent holes through the roof. The roof should form a continuous solid red pyramidal/conical surface around the tower. — **Near-plane and near-fan causes both measured dead 2026-09-08; runs admitted, 0 vertices behind near plane.** The colors on the geometry that does render appear correct. — **Still open; compare missing and visible triangles through native clipping, emitted vertices, depth, and coverage at the failing camera.**
-Zebes: Acid plane STILL NOT FIXED. still looks like a low poly dome instead of a flat plane. Still get submerged a good amount before damage occurs. — **Still open; compare rendered plane with source root-plus-child contact height and fighter-root predicate on identical simulation ticks.**
-Mushroom kingdom: STILL MISSING THE LARGE SIDE BRICK PLATFORMS. Still renders at 20FPS. — **Geometry and cadence remain open; inspect native coverage and actual deadline margin, not packet counts alone.**
-Yoshi's Island: TRANPARENCY STILL NOT BEING APPLIED IN TEXTURES/SPRITES (both spinning textures, all three clouds (which are also the wrong color), the white sparkle textures around the heart). also new thing i noticed is that part of the BGM sounds garbled, but like only one instrument. STILL MISSING THE MAIN PLATFORMS AND MAIN FLOOR PATH. — **Still open: verify each material/particle, cloud colors, floor/platform coverage, and isolate the garbled instrument across source PCM, encoding, playback.** **2026-09-08: sparkles never admitted to the particle bank (P1 seam list only); cloud colour loses its env tint in the I4 converter.**
-SectorZ: Arwings are visible now, but do not act like platforms and the lasers do not spawn at correct locations. — **Visible now; verify source-gated yakumono collision and separate 2D/3D muzzle transforms. Do not reuse the obsolete no-spawn diagnosis.**
-saffron city: transparency around the pokemon garage door hazard thing is missing. REMOVE WHITE HAZE. — **2026-09-09: a claimed `silent empty draw` here was RETRACTED the same day. `DIAG_OWNERTRI` is indexed by NDSRendererProfileOwner (STAGE, MARIO, FOX, LUIGI...), not by stage owner_spec, so its zero slot was simply a fighter not in the match. The gate geometry IS in the packet: roots 0x0420 and 0x04f0 carry runs and triangles. The door transparency remains open on your own report and nothing else about it is established.** — **Door transparency remains open; remove only the source haze panel as authorized, preserving other geometry and native rendering.**
