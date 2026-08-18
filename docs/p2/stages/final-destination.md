# Final Destination — P2-6 venue (Master Hand)

Status: not started · Reference: BattleShip stage data via `docs/DECOMP_MAP.md`.

1P-only in the original (Master Hand fight). Geometrically the cheapest
stage in the game; its cost is the background.

## Content inventory

- **Layout**: single flat platform, two ledges, no pass-throughs, no
  hazards.
- **Background**: the animated space/wormhole flythrough — the stage's whole
  identity. DS treatment: scrolling/blended 2D BG layers or a skysphere with
  animated texture; reduced update rate fine per visual doctrine, but it must
  visibly *move* (Stage Completeness Standard).
- **Music**: Final Destination / Master Hand battle theme.
- **Boss integration**: right-side boss arena semantics, boss camera rules
  (`fighters/master-hand.md`).

## DS notes / risks

- Trivial collision (one afternoon) — schedule it exactly when P2-6 needs
  the venue; don't build early and let it idle unverified.
- The background is a visual-gate item; timebox one experiment, keep the
  cheapest owner-accepted look.

## Acceptance

- [ ] Collision parity sweep (flat + ledges).
- [ ] Boss camera integration verified with Master Hand.
- [ ] Animated background owner-accepted, screenshot recorded.
- [ ] Music live; fight-load path from campaign flow works.
