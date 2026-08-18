# Race to the Finish — P2-6 Bonus 3

Status: not started · Reference: `gr/grbonus/` + course data via
`docs/DECOMP_MAP.md`.

The campaign's Bonus 3: a timed escape course — the only scrolling course in
the game.

## Content inventory

- **Course**: branching corridor run left-to-right with obstacle gaps,
  multiple exit doors (nearer exits score less; the far exit scores most),
  countdown timer, Fighting Polygons placed as obstacles (verify count and
  placement from data).
- **Camera**: auto/forced-scroll semantics — falling behind or missing exits
  vs the timer (exact fail/finish rules from source).
- **Scoring**: exit-based + time bonus into the campaign score system.
- **Music**: bonus-stage theme with hurry-up behavior (verify).

## DS notes / risks

- Forced-scroll camera + off-camera KO semantics differ from every VS stage
  — keep them course-owned; do not touch shared camera code.
- Course geometry streams left-to-right — natural chunked-culling client
  (inherit from Hyrule/Sector Z work).
- One-shot mode logic (timer, exits, results hand-off) lives in the campaign
  driver, not in shared match flow.

## Acceptance

- [ ] Course collision + exits + timer rules equivalent.
- [ ] Polygon obstacle placement/behavior equivalent.
- [ ] Scoring feeds campaign totals correctly.
- [ ] Cadence held across the full run; owner plays it; screenshot recorded.
