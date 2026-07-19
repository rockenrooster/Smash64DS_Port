# Task 29 exact GX census and stop decision

Date: 2026-07-19
Source HEAD at capture: `214051254a80eb9d18406d765d79f25f519a7dae`
Harness: `battle_playable_realtime` / mode 163 / fast mode 9 / live Fox / static AOT / incremental wallpaper
Task-26 representation: generated segment 0 enabled

## Scope and identity

`NDS_TASK29_GX_CENSUS=1` is a profile-1, real-GX-only lab build. It observes
every renderer GX state, matrix, texture, geometry, and flush class after the
normal call/write has been selected. It records successive equal values of each
class only within explicit owner/flush boundaries; it never suppresses a write.
Profile 0 and normal profile 1 compile the census out.

- census ROM: `8CD572E096F33CABDB43EB720A204882C20EA6C248A9AC84C1DD14EC01BD4684`
- census ELF: `DCB10C9C8D56056EF1F5F0A851EF976DD13FD898583692EEBE3424FE25FE9D39`
- compiled-out control ROM: `C04071BDE55DF038705B29A950917C1E8F5F943EF5116C5151CEC7DFD2C4404F`
- compiled-out control ELF: `770672436F882B1EF3C4CEE405E5A4EB96E1DB140C92E297C365C6C5F1061685`

The full command, word, repeat, dual-stream-hash, boundary-hash, and owner
partitions are in:

- `2026-07-19_task29-gx-census-early.json`
- `2026-07-19_task29-gx-census-whispy.json`
- `2026-07-19_task29-gx-census-ko.json`
- `2026-07-19_task29-control-early.json`

## Synchronized census

| Window | Frames | Fast owner contract | Actual GX triangles | Owner vertices (stage/Mario/Fox/none) | Commands | Words | Equal-value observations | Boundaries | Faults |
|---|---:|---|---:|---|---:|---:|---:|---:|---:|
| Countdown / early | 438-445 | 121 runs, 828 tris, 202/320/306 | 828 | 606/960/918/0 | 7,227 | 14,415 | 1,976-1,991 | 13 | 0 |
| Whispy transition | 672-679 | 121 runs, 828 tris, 202/320/306 | 844-856 | 654-690/960/918/0 | 7,334-7,416 | 14,601-14,719 | 1,970-2,009 | 15-19 | 0 |
| Natural KO | 566-573 | 91 runs, 508 tris, 202/0/306 | 524 | 654/0/918/0 | 4,975 | 10,858 | 1,497-1,513 | 13 | 0 |

Whispy's 16-28 extra triangles and KO's 16 extra triangles are real dynamic
stage/effect geometry outside the fixed fast-owner total. The census conserves
them against the synchronized renderer triangle count instead of hiding them
inside the 828-triangle baseline.

| GX class | Early commands / equal values | Whispy commands / equal values | KO commands / equal values | Disposition |
|---|---:|---:|---:|---|
| control | 412 / 330 | 416-424 / 333-339 | 344 / 262 | Opportunity only; no retail A/B, no promotion |
| alpha test | 36 / 28 | 36 / 28 | 36 / 28 | Opportunity only; no retail A/B, no promotion |
| texture parameter | 44 / 0 | 44 / 0 | 41 / 0 | Existing exact Task-8 shadow already removes its safe repeats |
| texture bind | 54 / 0 | 55 / 0 | 51 / 0 | Side-effectful / never suppress |
| matrix mode | 164 / 0 | 166 / 0 | 138 / 0 | No opportunity |
| matrix load 4x4 | 310 / 53 | 312 / 54 | 284 / 54 | Side-effectful / never suppress |
| matrix store/restore | 10/84 / 0 | 10/84 / 0 | 2/14 / 0 | Side-effectful / never suppress |
| polygon format | 69 / 0 | 70 / 0 | 54 / 0 | Existing exact Task-8 shadow already removes its safe repeats |
| begin | 103 / 93 | 104-106 / 93 | 86 / 76 | Side-effectful / never suppress |
| color | 2,484 / 1,289-1,304 | 2,532-2,568 / 1,279-1,312 | 1,572 / 928-944 | Prior measured suppression regressed and remains reverted |
| texcoord | 972 / 83 | 972 / 83 | 780 / 78 | Prior measured suppression regressed and remains reverted |
| vertex16 | 2,484 / 100 | 2,532-2,568 / 100 | 1,572 / 71 | Side-effectful / never suppress |
| flush | 1 / 0 | 1 / 0 | 1 / 0 | Side-effectful / never suppress |

Fog, identity, multiply, push, pop, and end classes emitted no commands in
these windows. The fail-closed never-suppress mask is `3374912`; it covers
texture bind, all side-effectful matrix operations, begin/end, vertex, and
flush.

## Exactness and safety

- The instrumented and compiled-out frame-445 top screens differ by exactly
  `0/49,152` native pixels, mean channel delta `0.00`, overlap `100%`.
- The profile-2 independent oracle passes 828 semantic events per frame,
  exact 202/320/306 owner provenance, zero semantic overflow, and unchanged
  entry/exit state, vertex-cache, resolver, geometry, material, texture, and
  depth contracts.
- All three census windows retain zero fast fallback, zero post-GO texture
  fence failures, one flush per frame, and net taskman reserve 174,864 bytes in
  the instrumented lab.
- The census build uses 27,284/32,768 ITCM bytes. Its cold recorder and tables
  are absent when the selector is zero.
- The final profile-0 Boundary ROM/ELF are
  `21D789F3439FB2223C7F0F4F097B5A2ABD9652F2BDE4A6648B1A6808C404EEC1` /
  `89C83C403E59365BC938A2DF5745C506EE66F63DAB8AF772C93440EC5CF1C355`.
  The ELF contains zero Task-29 symbols, retains 28,820/32,768 ITCM bytes, and
  passes the canonical runtime, publication, renderer/ITCM, and visual gates.

## KEEP / STOP decision

KEEP the compile-gated Phase-A census, owner-conservation verifier, runner-slot
GDB-port isolation fix, and forensic timeout forwarding. They improve future
GX falsification without changing a shipping ROM.

Do not add a Phase-B suppression class. Control/alpha are the only untried
state-sized opportunities, and Task 29 requires a repeatable retail gain for a
GX promotion. The user declined further retail repeats. Texture/poly exact
shadows already exist; matrix mode has zero repeats; color/texcoord suppression
was previously measured slower and reverted.

Do not add a Phase-C immutable stream. Task-26 segment 0 is a different,
already-retained 26-run generated representation, not one immutable Task-29
run. Its five same-control phases save only 3,424-3,616 stage P50 ticks, below
Task 29's 5,000-tick actual-run entry gate. The single retail observation saves
21,568 draw ticks but changes active ticks by +4,288 and is explicitly
non-repeatable evidence. It cannot qualify a cache/layout-sensitive template,
and no further device sample will be requested.

Task 29 therefore closes with diagnostic infrastructure only and zero new
profile-0 GX behavior or footprint.
