# Implementation handoff — where P5 work resumes

Parent: [master plan, Revision 1](New_Stages.md). Live state:
[P5_STATUS.md](P5_STATUS.md). This is a pointer, not a second board.

P5.0 is complete: the census (`scripts/p5/stage_census.py`, results in
P5_STATUS) and the owner's four decisions of 2026-10-12. P5 starts when the
owner makes it the active goal; P4 is active today.

Resume in this order (master plan, Board 1 then Board 2):

1. T1 stage registry and T2 identity seams; then T6, the 6x3 stage select
   with locked cells for stages not landed yet.
2. Wave 1: Meta Crystal, Duel Zone, Final Destination, with T10 per-gkind
   rows, the T11 probe and Final Destination's VS wallpaper, music and
   no-item rules.
3. Wave 2: First Destination, proving T3 export, T4 generator donor mode and
   T7 music with Remix's instruments.
4. Wave 3: Dinosaur Land (movers, camera), then Bowser's Stadium (bombs):
   T8 and T9.
5. Wave 4: Hyrule Temple; finish T5; T12 worst-case search.

Standing rules: `decomp/` is read-only; the donor build runs in ignored
staging; ROM-derived output stays under `builds/`; generated files are never
hand-edited; no runtime interpreter or code paging; never edit source while
a build runs.
