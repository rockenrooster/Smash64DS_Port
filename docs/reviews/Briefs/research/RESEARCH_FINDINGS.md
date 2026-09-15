# Research findings that change the proposed fixes

This is the decision summary for the inspected `a5c5bc08d8e...` snapshot, not a
list of ROM-verified root causes. Read SOURCE_LEDGER.md for pinned source paths
and VALIDATION.md for exactly what ran.

## 1. Impact wave: a concrete wrong-depth seam

The ring owner projects XY and then uses a new painter depth for each triangle.
The original display callback selects a Z-buffered translucent render mode. R01
uses the same source-depth conversion already used by real-depth native submits,
with one cached Z per unique vertex. It does not choose an arbitrary camera bias.
R02 independently restores the source initial geometry state for effect models.
The asset command decode remains owner-supplied; the source callback, emitter and
current depth helpers were directly read. Full-alpha depth writes/near clipping
are still separate requirements. Sources: S05/S07/R76/R78/R105.

## 2. Link SpecialN: an additional configuration bug

The `NDS_P2_KIRBY && NDS_NATIVE_OWNER_IMAGE_KIRBY` branch contains Link's special
foreign table and light-preamble routing. Its disabled-Kirby alternative does not.
The old helpers return Link body tables for a boomerang-owned root in that
configuration. The host negative control reproduces this return; R04 passes with
the existing routing mirrored into the alternative branch. This is stronger than
an untested guess, but still not attribution to the owner's ROM without its flags
and natural move frames. Sources: R83/R84; host tests in tests/test_candidates.py.

## 3. DATA is still a retired-consumer problem

The native DATA fragment continues to call text slots, while Options uses native
surfaces. R05 adds source DATA surfaces and changes the runtime consumer, rather
than reviving the retired text slab. The original source also has different
locked/unlocked layouts; those are represented separately. The generator appends
new IDs after existing surfaces and the coverage inventory gains the new table.
Source art decoding/staging has not run. DATA children are not silently claimed
fixed. Sources: R92/R93/R97–R100/R103/R104/R114–R117.

## 4. CSS needs more than the older residency fixes

The shared YoshiModel collision, escaped owner-image allocations and particle
bank reinit already have repairs/evidence. The one-action budget already exists.
Repeating those edits is not a new CSS fix. R03 addresses three remaining
release-before-reset seams. Blocking compact acquisition remains and explicitly
suspends BGM, so a bounded incremental load and safe publication contract is the
candidate for gate hitches, audio interruptions and hover delays. A heard restart
must still be distinguished from a pause or a separate start/seek call. Sources:
S10/R41/R42/R48/R102/R107.

## 5. Samus morph programs already exist

MorphUnfold/MorphBall and their source roots are present. The current source tests
cover the model-part helper family, and the board notes that the tested CPU does
not perform roll/Bomb. The next step is natural human engagement and inspection
of the first wrong program/image/joint—not adding another morph implementation.
Bomb weapon and charge/shot effects remain separate owners. Sources: S02/S11/R18.

## 6. Yoshi needs complete source replacement/child closure

An ordinary Yoshi pose does not qualify Catch/Throw, egg lay, egg throw shells or
entry egg. Model-part/DL-pair state and separate egg/effect children need their own
source-derived native bindings and lifetimes. Whole-fighter unhide or generic egg
substitutes would hide the missing capability. No fabricated root-offset patch
was produced without the required source/packed asset closure. Sources: R52/R71
and the relevant board record, S02.

## 7. The alpha reports do not have one universal decoder fix

The current effect converter already has graded-alpha IA paths and newer IA8
handling. Link's entry wave/column use CI4 plus primitive alpha, not the same
material as Spin. Opaque texture coverage, ignored primitive opacity, wrong
blend mode, stale prepared state, vertex-alpha averaging and filtering mismatch
must be distinguished. Reapplying an obsolete byte swap or globally keying white
would introduce regressions. Sources: R61/R64/R65/R66 and HW01/HW02.

## 8. Zebes already has one subdivision pass

The acid producer subdivides an identified root, then replaces per-corner alpha
with a triangle average. Another instruction to “add the missing subdivision”
would be stale. E01 instead provides a bounded isoband experiment with exact
analytic geometry tests. Its potentially large output, fixed-point and native
run integration, projection/lighting behavior and DS blend/depth policy still
need qualification. It is not a shipped smooth-gradient fix. Sources: R60/R88/R91.

## 9. Visible-but-untextured is not missing geometry

Peach's roof and Mushroom's left platform now require per-run texture/material/
UV provenance investigation. Preserve the recent geometry/admission repairs.
A texture-state inheritance or foreign image binding error should be fixed at
that seam, not by painting every triangle or rebuilding the stage as a backdrop.
Yoster's missing main structure is a separate root/transform/material coverage
problem; collision and total triangle counts cannot prove it appears. Sources:
S12/R90/R91 and the current owner reports.

## 10. Saffron closing depends on monster retirement

The original imported gate code already plays animation and enters closing when
its monster pointer becomes NULL. A permanently open door may therefore be a
monster cleanup problem, an animation/process problem or a stale native transform.
The gate's baked pose alone cannot choose among them. A new fixed-period oscillator
would change source behavior. Sources: R73/R74/R106.

## 11. Sector Z and Thunder crash causes remain unmeasured

Potential code repairs are documented for custom matrix context, foreign/native
image lifetime, child cleanup and valid allocation handling. No first-fault trace
from the reported ROM was available to select a branch. Disabling a hazard or
adding a return that removes required content is not represented as a fix. Sources:
S02/R72 and the code anchors in issues 20 and 34.

## Delivery interpretation

Five files under runtime/ are actual candidate code diffs. E01 is executable
experiment code. The other issue documents contain conditional code-level repair
plans, not concealed claims of implementation. All 35 owner reports retain their
own acceptance; the empty Captain Falcon heading remains scope-only. No natural
ROM behavior, current performance gate or subjective acceptance was established
by the host tests.
