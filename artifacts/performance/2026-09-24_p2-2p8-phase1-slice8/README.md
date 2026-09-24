# P2-2p8 Phase 1 slice 8: remaining fighter scenes

State: **WITHDRAWN / CAMPAIGN DEFERRED**, owner 2026-09-24. Pre-stage intros are
static images; >=95% of current work must target four concurrent VS fighters.
All five owned source/tool modifications were reverted to `98ebd1e2e51`; the
new host test was removed from the active suite. The experiment is retained as
`deferred-live-intro.patch` (SHA-256
`A3A9326590395DA92A4AA6ADFEA2824B58976DC1ECB0E0CCCC6032B0C28554CA`) and
`deferred-camera-host-check.txt` (SHA-256
`267F9BF0F138230D186D52CF31BE91F99F8814CADC0673B715224860E2EF83C0`).
The prototype ROMs in the warm freeplay/fpwalk build directories are unqualified;
retained controls and the main four-CPU baseline remain intact. Rebuild those
warm directories before treating them as current source. All runtime jobs ended.
The findings below describe the withdrawn experiment, not the current code.

Original experiment state: IMPLEMENT on `98ebd1e2e51`. Serial execution only.
Slice 7's qualified measurements remain in its receipt; do not repeat them.

## Current batch

The lean path now accepts a separate affine camera modelview and all compiled
native owner IDs. Intro transient submits use the existing scratch slot 0,
rebind on every draw and bypass normal frame dedup, preserving the source
Polygon Intro's repeated poses within a single frame. The shared draw body is
outlined in 1P builds to avoid duplicating an always-inline body at both callers.

Camera arithmetic follows `ndsRendererAdapterComposeOwnerWorldsSource` exactly:
compose joints at Q43.20, round each binding to Q20.12, then multiply by
`shuffle * camera`. The joint kernel is unchanged. The prior slice-7 handoff
note about seeding Q43.20 composition was corrected after reading the function
body at `renderer_adapter_matrix.c:7193-7211`. Existing world storage and the
existing per-camera cache are reused; no per-actor or second camera cache added.

Files: `renderer_fighter_lean.c`, `renderer_adapter_fighter.c`,
`include/nds/renderer_fighter_lean.h`; tests under `scripts/fighters`.
Host checks: 5 passed (`test_intro_transient_context.py` and
`test_lean_camera_patch.py`). They cover 21 actors, repeated poses, route 0 and
decline behavior, and the real camera patch plus real matrix arithmetic against
independent Python integer results. They do not establish runtime acceptance.

Next: preserve the slice-7 freeplay control, incrementally build the candidate,
then compare a real Intro and check the all-content CSS reservation. Extend to
variant owners, autodemo and remaining CSS/Results coverage. No phase-level
timing rerun until the integrated fighter replacement is ready.

Owed: runtime engagement/output, 1P variant coverage, D8 scene matrix, native
failures zero, retirement and final integrated verification. Phase-3 memory,
motion/audio work and the full 30 FPS/all-roster-stage goal remain open.
Root `smash64ds.nds` remains r54; this batch does not publish it.

Control preserved in `builds/p2p8-s8-control-freeplay/`:
ROM `FEE157D738B07646A1F409C18A30F922912FB18A3EEEA59B395F9EFA3A7921BC`,
ELF `ADD0858E6212A3C79A44A44879060F8F7D1D23CFC443C71769852F830B55FE92`,
config `D7C8896D007F809E9615A4BC214DEB540B130D8E465F69CE3FCE5F1F3DA27887`.
The first copy loop's PowerShell array concatenation made no copies; corrected
it while the old outputs were still intact, verified source/destination hashes
before and after each copy. No baseline was lost.

Completed: exec-command session **69124**, `tools/build-s7.ps1` targeting
`smash64ds-p2-shell-freeplay-hwtri`, reusing `build-p2p8-s7-freeplay`.
Log: `build-freeplay.log`, exit 0, native-only pass over 318 link inputs,
ROM `DB66E2378A908A72696B4B9B24D542A6B082FB340170BFD479132DADA9F82254`.
Static RAM +544 B, data/BSS unchanged (`ram-freeplay.txt`); no particle
regeneration occurred. The shared 1P draw body avoids duplicate inline copies.

Completed: exec-command session **88941**, same driver, warm `build-p2p8-s7-fpwalk`
with `NDS_P2_MENU_WALK=1`, log `build-fpwalk.log`. This is the existing campaign
probe's real-input diagnostic contract, not a publication ROM. Its prior
control is preserved at `builds/p2p8-s8-control-fpwalk/`, ROM
`E3786DA6D1B61EF9E5EAFCC0E36BAA134283F992F72A6F2E63CED483A043FBFB`.
Build exit 0, native-only pass over 318 link inputs; candidate ROM
`C9D8740E8E2EAEBDF63B1C1AE5028E3312A77F9221978868A8431437002C5AF2`.

Completed: exec-command session **31396**, `scripts/menus/probe-p2-campaign.ps1`
against this walk candidate, runner 7, timeout 240, hits 8, battle presents 8.
Transcript `intro-candidate.txt`, driver log `intro-candidate-run.log` with
`PROBE_EXIT`, captures under `artifacts/visibility/2026-09-24_p2-2p8-phase1-slice8/`.
This existing source-input probe reaches a real Intro before its first battle;
its broader campaign verdict is separate from the scoped renderer result.
The 8-scene cap stopped at Intro entry before it could draw (exit 2); this was
missing probe coverage, not a ROM failure. Extended to 12 scenes in session
**3751**, with new `-IntroLeanProof` GDB observers (no ROM instrumentation).
That run completed: at Intro update 40, 80 lean attempts, 40 camera patches and
40 old-path draws. One actor still declines; Intro is NOT accepted. The first
battle also reports 1,218 source display-list overflows (112 B last request),
so the broader campaign probe fails. Its active heap minimum is 66,852 B.
Transcript/captures: `intro-candidate-full*`. Root arithmetic host checks do not
excuse this runtime fallback. The inspected Intro image shows Mario; compare
against control before inferring whether the absent enemy is source timing.

Completed: exec-command session **11452**, same candidate and probe with hits 9,
stopping at the following battle entry. The observer now prints first-fallback
camera/instance state and arguments; `intro-decline.txt` / `intro-decline-run.log`.
It reproduced the fallback. First observation: camera flag 1, instance kind 0,
14 roots, source-compose failure 0; private cached globals alone do not identify
the declining actor/reason reliably.

Job: exec-command session **98093**, `intro-reason*` on the same candidate,
then `intro-control*` on the retained walk ROM. Both use hits 9 (stop at the
following battle entry). The candidate observer now reads the lean event's
return value in r0 at the instruction after its call, located from this ELF's
disassembly, and prints the old-path fighter kind. This avoids guessing from
cached diagnostic globals and adds no ROM instrumentation. Final marker:
`INTRO_DIAG_AND_CONTROL_DONE`. Broader campaign verdicts remain incomplete at
this intentional scene boundary. Next: use the observed reason to repair the
decline, then verify the same Intro and CSS reservation.
