# AGENTS.OPUS.md — the Opus operating loop

Read `AGENTS.md` first; it owns the mission, hard rules, and boundary.
`CLAUDE.OPUS.md` owns your rails. This file owns how an Opus agent runs a
cycle here, distilled from the 2026-08 bug campaign (65 checkpoints, seven
implementer contexts).

## The cycle contract

One cycle = one bounded objective, stated in one line before the first tool
call. The loop:

1. **Inherit.** Read the board (`docs/BUGS.md`), the workflow
   (`docs/BUG_FIXING_PROCESS.md`), and the commit trail for your row. Do not
   re-derive what is recorded; do not re-walk a chain the board marks green.
2. **Discriminate.** Choose the single cheapest measurement that splits the
   live hypotheses, and take it FIRST — not after the easy half of the brief.
   Rung order: source/artifact reads → static checkers → gdb on the existing
   ROM → one instrumented build. Publish counters from code (declare in
   header, define in `diagnostics.c`, nm-verify against `--gc-sections`)
   rather than trusting raw register reads.
3. **Act at the owning seam.** BattleShip source is the oracle for behavior;
   `sm64-nds`/`sm64ds-decomp` for DS architecture. Fix the seam, never the
   symptom: no re-pads, no sleeps, no bolted-on retries, no arbitrary
   constants hiding a shared defect.
4. **Prove engagement.** Every fix ships with (a) a counter or capture showing
   it firing, and (b) a negative control showing it inert where it must be.
   "Measured zero" only counts against a control that can be non-zero.
5. **Commit and report.** Explicit paths, name-scan, trailer. The packet
   leads with the outcome and states what was NOT done as plainly as what
   was: changes and where; evidence with numbers; row states; root-ROM
   hashes; commit hash; surprises; inheritance for the next cycle.
6. **Stop clean.** When context runs thin, stop at a verified boundary —
   never mid-fix, never with an unproven candidate committed. Handing forward
   a confirmed seam with no fix beats handing a fix with no proof.

## Escalation

- `BLOCKED(decision: ...)` for anything the sacrifice order, fidelity budget,
  or product contract assigns to the owner. State the options and the price;
  do not recommend by choosing.
- A held publish beats a wrong one: if a stop rule fires (regression over the
  noise floor, red verifier, moved pixels at the shipping default), stop and
  attribute before anything ships.
- If your brief conflicts with a repo rule, say so in one line and follow the
  repo rule.

## Known Opus failure modes — check yourself against these

| Pattern | The tell | The rule |
|---|---|---|
| Confident absence | "nothing references X" from one grep | `--no-ignore`, a second modality, or say "not found where I looked" |
| Self-validating read | the discriminator read through the pointer under test | validate against an independent register or code-published global |
| Budget misallocation | the decisive measurement postponed for the easy half | discriminating read first, always |
| Premise inheritance | building on last cycle's conclusion without its evidence | restate the evidence line you depend on; if it has none, measure it |
| Phantom defect | fixing behavior nobody measured broken | existence chain first — artifacts outnumbered real defects 3:1 once |
| Scope creep | "while I'm here" edits outside the row | one line in the owning doc, move on |
| Heroic finish | starting a chain that cannot be verified in-cycle | splitting rule: finished subset > unverified whole |
| Name-driven logic | a function body trusted because its name matches | the body matched the name, not the contract, for a whole campaign once |

## Packet discipline

Lead with the outcome. Numbers, not adjectives. Quote `EXACT_LOCK` lines and
counter names verbatim. Never headline a number whose window or pairing you
have not stated — short-window percentiles and host-side FPS readouts on
unpaced arms are not prices. Arm identity comes from the build directory, not
from the sampler's `gitShort` (it records the repo HEAD at sample time).
