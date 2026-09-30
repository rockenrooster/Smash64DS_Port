# CSS selection intro / Pikachu and Ness contortion: fix verified (r56)

Cause and evidence: `REPORT.md` (Codex investigation, 2026-09-30). The CSS
Selected loader in `lbRelocGetForceExternHeapFile`
(`src/port/reloc_backend_assets.c`) returned before replacing the heap's
authoritative force result and before dropping the zero-copy pin of the clip
the heap held before, so `ftMainSetStatus` resolved `fp->figatree` to that
previous (stream) clip: the Selected animation never ran and the preview
posed with the wrong joint tables. Regression since zero-copy clips (after
r54).

Fix: on a successful Selected load, `ndsR2AnimPinsDropHeap(heap)` and
`ndsRelocRecordAuthoritativeForceFile(heap, file)`, the two things every other
force load already does.

Verification, candidate r56 (`builds/p2p8-playtest-r56/smash64ds.nds`, SHA-256
`1EEB82FD76A1B286D39E1E6C15D6F26138185EB884089AEE751E4D2ECF5D46A1`, HEAD
1246ddfe5a1 + this fix), the investigation's symbol-only feed
(`css-r54-long-trace-feed.gdb`, natural CSS input, no debugger repair):

| Fighter | Selected bind | `fp->figatree` after | Intro clock |
| --- | --- | --- | --- |
| Pikachu | tic 243, first joint `0x21867b0` (Selected table) | heap (was the stale `0x68`-headed stream clip on r55) | 0 -> 116 over 120 frames, then ends |
| Ness | tic 525, first joint `0x2183f38` (Selected table) | heap | bound through tic 700 |

Captures: `r56-fix-pikachu-280.png`, `r56-fix-ness-680.png` (normal poses; r55
equivalents contorted, r54 normal: `r55-trace-*.png`, `r54-long-trace-*.png`).
Not yet covered: the other ten fighters' intros, deselect/reselect loops,
Results -> CSS; owner playtest owed.
