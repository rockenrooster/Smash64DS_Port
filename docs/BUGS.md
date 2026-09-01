**Follow Bug fixing workflow contained in `BUG_FIXING_PROCESS.md`.**
AI Agent should mark fixed items with **FIXED** prefix or a 20 word summary (or less) if not fixed yet.

- **FIXED 2026-09-01:** Fox pistol uses one native sidecar draw; live proof shows +22 triangles, zero fallback/failures.

- **FIXED 2026-09-01:** Falcon Punch attaches at source joint 16 with its measured BattleShip world translation.
- **FIXED 2026-09-01:** Falcon Kick attaches at source joint 23 with its measured BattleShip world translation.

-4 CPU stress test ROM plays at 3 FPS with lots of textures missing/swapping/etc. looks like a lot of things are going through the generic/slow renderer. — **OPEN:** correctness/native textures fixed; P50 2.24M/P95 3.36M; performance optimization remains.

- **FIXED 2026-09-01:** Mario/DK retain Q20 hierarchy precision; host and frame-locked visual oracles match BattleShip within one DS LSB.
