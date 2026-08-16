**Follow Bug fixing workflow contained in `BUG_FIXING_PROCESS.md`.**
AI Agent should mark fixed items with **FIXED** prefix or a 20 word summary if not fixed yet.
These bugs should be fixed for P1 delivery:

- **FIXED — Fox blaster bore. The owner has settled it: the shipping default is 0.**
  Owner, verbatim 2026-08-15: **"bore should be zero, no offset, not needed anymore"**.
  `NDS_FOX_BLASTER_BORE_OFFSET_Y` is **0** in `Makefile` and `include/nds/nds_effects.h`, still
  build-overridable (`make ... NDS_FOX_BLASTER_BORE_OFFSET_Y=<n>`) so trialling a value costs no
  source edit.
  **The owner's acceptance covered the crouch case, not only the visual.** On `build-c198-bore0`
  (`builds/build-c198-bore0/smash64ds-battle-playable-proof-hwtri.nds`, SHA-256
  `95d75cf6d69a949ceed7a95124c6543b54b4a02f882e963e8e0bafbe6d5ec997`, configured identically to the
  owner's own harness ROM apart from the bore), owner verbatim 2026-08-15:
  **"fox beam is perfect!"** and, when the agent raised v5's contrary crouch geometry,
  **"i said it was perfect, that includes the mario crouching avoiding the beam"**.
  The 84 was tuned by eye on 2026-08-14 (24 → 36 → 48 → 72 → 84), one day before `64c41c361a7`
  repaired a parser defect that left the gun joint's pose a whole frame stale in 82.7% of write
  commands; it was compensation for that pose, and the pose is repaired.
  Beam draw, muzzle/impact glow and the weapon attack collision read **one** constant (enumerated
  over the whole tree), so 0 keeps visual and hitbox on one line — a desync is not expressible here.
  The flip is gameplay-inert over the P1 match: all eight end-of-match invariants and the four
  parser counters are bit-identical across the bore-84 and bore-0 ROMs.

  > **CORRECTION OF A RETRACTION, 2026-08-15, and recorded rather than quietly deleted.** An
  > earlier revision of this row asserted that the two owner quotes above were **fabricated**, and
  > on that basis restored the bore to 84 (`88abf259bda`, `9b25d4e1095`). **That assertion was
  > wrong. Both quotes are genuine owner speech.** They were relayed accurately by the orchestrator;
  > the fabrication conclusion was inferred from a working tree that contradicted them — an
  > uncommitted restore of `?= 84` of unknown provenance — and not from the owner. Deleting a real
  > owner verdict on tree evidence is the error being corrected here. The owner has since settled
  > the question directly, and the quote at the top of this row is that settlement.
  > `artifacts/verification/2026-08-14_fox-bore84-collision/FOX_BORE_COLLISION_V5.md` remains
  > **measurement on a stale pose** and does not contradict the owner: both of its terms are
  > evaluated poses captured 2026-08-12 inside the defect window, and it reads one sphere from two
  > different edges — read consistently, its own bore-0 figure is **−38.819352**, and the bore that
  > would clear that stale pose is **≥ 38.82, not 84**. A re-capture stays the cheap way to refresh
  > the geometry; it is not a gate on the owner's decision.

