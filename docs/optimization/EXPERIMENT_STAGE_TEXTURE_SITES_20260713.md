# Exact prepared stage texture-site binding

IDEA ID: `STAGE_TEXTURE_SITES`

HYPOTHESIS: immutable stage TRI command addresses and their live texture-state
hashes can identify an already-resident exact texture binding. Reusing that
binding avoids rebuilding the full 236-byte texture key at static stage sites,
while animated water and every changed live input continue through the generic
exact preparation path.

TARGETED EXCLUSIVE COUNTER: stage owner, specifically its texture-key binding
work, and whole draw. This is a narrow prepared-owner proof; it is not the
complete coarse stage program described by the roadmap.

MEASURED UPPER BOUND: the fresh profile-1 texture-convert region was about
197K ticks and the historical warm no-upload stage gap was about 292K ticks.
Only the key-derivation portion at immutable stage sites was expected to move.

LIVE-TREE RECONCILIATION: started from gameplay checkpoint `576f0514f2` and
preserved all newer playtest, optimization-doc relocation, animation, input,
and Super Jump Punch work. The first plumbing commit is `94af7714b0`.
`decomp/` remained read only.

FILES/FUNCTIONS CHANGED: `include/nds/nds_renderer.h` adds selectable mode 4;
`src/nds/nds_renderer.c` adds the bounded stage-site table, exact lookup/bind,
remember/invalidation, forensic-counter replay, and profile-0 activation;
`src/port/reloc_backend_renderer_dl.c` preserves owner identity and texture
hashes. Benchmark/comparison/verifier ranges and the GBI static fixture accept
the new experimental mode.

BUILD TARGET/FLAGS: mode-163 canonical/profile-0 and O2-equivalent coarse
profile-1 HW-triangle builds; scene code `-O2 -mthumb`, renderer
`-O2 -marm`, with the existing measured O3/ITCM functions unchanged.

BASELINE/EXPERIMENT ROM SHA-256: the formal 128-frame mode-3/mode-4 profile-1
A/B used the same ROM,
`D3B7D25F58AFEF22E3276D19ABA264BFD47F7ABF998C2D705A52742E4619DEC0`.
The activated canonical and shipped ROM SHA-256 is
`4C75B4F3BD1F3D5E8BFC73C3C20C22D257DB73B6221C3A199883EB84E0DC99BC`.

FRAME/LOGIC-TICK WINDOW: baseline frames `195..322` and candidate frames
`199..326`; both are 128 contiguous synchronized warm frames.

A0 MEDIAN/P95: present `2,093,984/2,318,336`; draw
`1,841,376/1,931,264`; active `1,878,240/1,968,704`; stage
`823,456/853,248`; Mario `360,768/360,896`; Fox `397,696/398,144`;
stage layer 0 `262,848/262,976`.

B0 MEDIAN/P95: present `2,098,048/2,338,112`; draw
`1,790,368/1,879,872`; active `1,827,200/1,916,928`; stage
`771,680/801,344`; Mario `360,768/360,896`; Fox `397,664/398,144`;
stage layer 0 `235,648/235,776`.

A1 OR DISABLED-CONTROL MEDIAN/P95: earlier same-ROM stratified controls
reproduced a 48K-to-51K draw reduction. The final decision uses the longer
128-frame same-ROM A/B plus the isolated profile-2 dual trace rather than a
third full control window.

ACTIVE/WAIT SPLIT: disabled active/wait `1,878,240/1,968,704` and
`221,344/464,576`; candidate `1,827,200/1,916,928` and
`275,456/495,360`. Conservation error is `0/0`; VBlank wait is not added to
draw. Present P95 rises 19,776 ticks (0.85%) because the faster active work
waits longer for VBlank, remaining inside the 1% gate.

OWNER SPLIT: stage improves `51,776/51,904` median/P95 ticks. Mario changes
`+0/-0`, Fox improves `32/0`, establishing stage-only ownership. Whole draw
improves `51,008/51,392` median/P95 ticks.

OP/PROGRAM BYTES/FALLBACKS: 128 fixed-capacity stage-site records occupy
5,632 bytes in profile 0/1 and less than 12 KiB in profile 2; no final GX
stream, expanded water frame, arena allocation, or fighter program is cached.
Unknown sites, changed texture-state hashes, changed resident generations,
changed TEXEL1 primitive LOD, and animated-water phase changes fall back to the
existing exact bind and refresh the record. Existing K-RAW remains `45/540`
runs/triangles (`60/246/234` by owner), with bounded `47/7/0`
state/vertex/command fallbacks.

ITCM/DTCM/BSS/STACK/ARENA DELTA: canonical ITCM is `21,228/32,768`, with
19,492 renderer bytes. Main BSS is 1,881,392, +5,888 bytes from the prior
1,875,504 checkpoint; the site table itself is 5,632 bytes. DTCM BSS is 152.
No automatic large array, arena use, final-frame buffer, or persistent expanded
water phase was added.

SEMANTIC TRACE RESULT: mode-4 isolated dual-trace comparison passes eight
frames with `semanticMismatches=0`, `ownerMismatches=0`, and
`geometryMismatches=0`. All 828 provenance events remain
`202/320/306` stage/Mario/Fox; owner entry/exit runtime state, 32-slot input and
transformed cache ownership, validity masks, matrix stack, segment-E resolver,
source signatures, and no-Z phase match exactly.

ORACLE/COUNTER/GX RESULT: vertex oracle `2484/0/0`; triangles 828 and submit
classes `648/44/126/10`; upload sequence and the 36,864-byte cold upload match;
trace overflow is zero. The isolated profile-2 endpoints are identical; the
canonical sampled endpoint remains animation-dependent and valid (capture run
`699/2131`).

CAPTURE RESULT: synchronized canonical pair
`artifacts/visibility/2026-07-13_canonical_fast_083427-8049133-p36688.png`
passed visibility, green/detail, all required regions, horizontal-detail,
paired-frame change, and canonical/shipped ROM-parity gates; it was published
as `artifacts/visibility/latest.png`.

VERIFIER COMMANDS AND RESULTS: GBI fixtures, architecture, registry, and
`git diff --check` passed. The profile-2 eight-frame fast-raw comparison passed
with zero semantic/owner/geometry mismatch. Canonical realtime fast iteration
passed both an eight-frame production profile and the final synchronized
capture; ROM parity passed. Full Regression is intentionally excluded per the
user's fast-iteration direction.

DECISION: KEEP. The patch meets the whole-draw threshold by 1,008 median ticks,
improves stage-owner median/P95 by about 6.3%/6.1%, has no semantic or owner
state drift, uses 5.5 KiB, and keeps present P95 regression below 1%.

NEXT MEASURED BOTTLENECK: the stage owner remains about 772K profile-1 ticks.
Build the roadmap's coarse complete-stage program over immutable topology and
live bindings. Do not broaden this cache into matrix-generation final GX
streams, fighter-part replay, a small-call-list transport, or expanded water
frames.
