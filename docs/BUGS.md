**Follow Bug fixing workflow contained in `BUG_FIXING_PROCESS.md`.**
AI Agent should mark fixed items with **FIXED** prefix or a 100 char summary (or less) if not fixed yet, always using ** ** to encapsulate agent added text.
(Investigation details live in `docs/p2/BUG_NOTES.md`; this queue stays lean.)
**Owner: removal from this file means FIXED; absence never means a symptom was unreal.**

treat anything not going through native renderer a failure.

**Plan and evidence: docs/p2/REMAINING_BUGS_IMPLEMENTATION_PLAN_2026-09-22.md**

Owner notes: This isn't meant to be comprehensive, just my quick observations:

Main Menus:
-VS options
-CSS Bugs still present and need to be fixed:
    -delay between cursor hover and 3d fighter preview rendering. **NOT FIXED in R36 ** **r37: preview loader spent 1 of its 4 byte units per tic; now spends all four.** **DEFERRED by owner 9/22; per-step profile and plan in BUG_NOTES "C1".**
    -Fighter faces not looking right:
-Yoshi:
-Link:
-Pikachu
    
-Samus
-Kirby
-Jigglypuff
-Captain falcon
-Ness
-General
Stages:
-peaches castle: 
-Zebes: 
-Yoshi's Island: 
-SectorZ:
-Saffron city: **NOT FIXED in R36 ** the pokemon garage door hazard is always open. It should close and open periodically. **Transform chain VERIFIED by injection: the door's matrix tracks its joint, geometry submits, timing matches source. Only pixels unverified. Does it ever close over a full minute?** **DEFERRED by owner 9/22.** 

Audio:
