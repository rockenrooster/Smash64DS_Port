# P2-3f47 roster-close: Kirby / Ness / Jigglypuff — 2026-09-13

## Kirby copy-hat fixes

- Detail residency: `src/port/reloc_backend_compat_shims.c:16602-16606` selects the
  copying fighter's battle slot and synchronously ensures both High and Low hat
  images at copy commit, so later source detail switches do not lose the hat.
- Multi-Kirby residency: `src/nds/nds_renderer_assets.c:3419-3436` makes the hat
  image/runtime/root/light-preamble storage `[4 battle slots][2 details]`;
  `src/port/renderer_adapter_fighter.c:3106-3108` publishes the live battle slot
  before native root resolution. The resolver/ensure path indexes that slot at
  `src/nds/nds_renderer_assets.c:3993-4002`.
- Failed-copy state reset: `src/port/reloc_backend_compat_shims.c:16596-16597`
  and `:16608-16609` clear both passive and transient Special-N copy IDs when a
  copy state is rejected, preventing the next same-victim copy from taking the
  stale Throw-cue branch.
- Polygon NKirby: `src/port/reloc_backend_compat_shims.c:16587-16599` rejects the
  copy-hat state for NKirby, whose native owner has no copy-hat binding, and
  resets both copy IDs instead of publishing undrawable geometry.

The required focused host suite passed after the four fixes before the later
concurrent Ness-owner generator edits: `test_kirby_trio_body.py` 21 passed / 1
skipped; owner-packet PASS; geometry-closure PASS; native-owner wiring
`owners=39 gaps=0`. Current-tree rerun still has the Kirby body test green
(21 passed / 1 skipped) and wiring green; owner-packet and geometry-closure now
stop in the separate protected Ness generator blocker documented below.

## Shell capacity lab

Lab target: `smash64ds-p2-shell-loop-hwtri`, build
`build-p2-shell-loop-roster`, with Ness/Purin/Kirby enabled together and shell
roster rung 8. The build printed `NATIVE_ONLY_PASS`. One slot-6 shell-loop lap
passed (`artifacts/verification/2026-09-13_p2-shell-loop.txt`). Shipped defaults
were not changed.

| Fighter | Compact CSS file | Resident | Shell result |
| --- | ---: | ---: | --- |
| Kirby | 40,064 B | 35,272 B | PASS in simultaneous three-kind shell |
| Ness | 24,580 B | 22,180 B | PASS in simultaneous three-kind shell |
| Jigglypuff/Purin | 17,740 B | 15,700 B | PASS in simultaneous three-kind shell |

Simultaneous arena evidence: PlayersVS high-water **875,180 B**; VSBattle
high-water **1,132,940 B**; minimum free floor **75,124 B** versus required
**32,768 B**; `LOOPNATIVEFAIL count=0`. Thus shell residency/capacity closes for
all three kinds together.

- Shell ROM SHA-256:
  `812E832010812CC5022D0647B4D09DE9C61D27DB2FFB61F16B7FA35D520D269F`
- Shell ELF SHA-256:
  `253A093962445AED761516F7A754A14ABC12C1E4ECC5ED40AF7251F70BFB0602`

## Natural-path battle proofs

Latest successful roster-close battle lab before the concurrent generator
change: `builds/build-p2-roster-close-battle`, roster
Kirby / Ness / Purin / Link, four level-3 CPUs, compact battle fighters enabled.
It printed `NATIVE_ONLY_PASS` when built on the then-current tree.

- Battle ROM SHA-256:
  `9F4E9666D6DB1CF88AAE49EF057E9E1423A0C97F8531E6050B3A78B6BC7C52FC`
- Battle ELF SHA-256:
  `D90F5CB73ED7BFDDB0767CD77756AF7B4D58C0755CA8DFF0A52D04FCAA7132AF`

### Kirby inhale -> capture -> copy

**NOT CLOSED.** The slot-6 `FirstCopyLinkReject` natural run reached neither a
copy terminal nor a screenshot before its 900-second bound. The diagnostic file
`artifacts/visibility/2026-09-13_roster-close-kirby-copylink.txt` contains only
breakpoint setup, so it is not acceptance evidence. No dated PNG satisfying the
both-detail visible-hat requirement was produced. A fresh proof ROM cannot be
built while the protected Ness generator blocker below is present.

### Ness PK Thunder -> self-hit

**BLOCKED BEFORE SELF-HIT PROOF.** A deterministic slot-6 natural-status entry
run on the 14:39 battle ROM reached live Ness Low detail but exposed that the ROM
predates the newly generated Ness model-part variant:

`NESS_ENTRY_END frame=120 player=1 fkind=11 status=0xdf detail=2 ...`

`NATIVEFAIL=252,1,22,721231,223,16272,37385312,2`

`NESS_VALIDATE code=4 slot=9 low=1 root=4 observed=0x6760 expected=0x44e8 count=14`

Evidence: `artifacts/visibility/2026-09-13_roster-close-ness-entry.txt`. Current
generator source now declares the `0x6760` Ness root variant and PASS2 combine
families, but a fresh native-only ROM is blocked during generation by
`scripts/fighters/generate_nds_native_owners.py:6617`:
`ness high: unlit root 0x6760 has vertex colours ['0x0', '0xffffff00']; only one
colour bakes as ambient`. `_bake_unlit_uniform_roots` is explicitly owned by the
other concurrent agent, so this package did not modify it. No PK-Thunder
self-hit PNG/counter witness was produced.

### Jigglypuff/Purin Sing -> sleep

**NOT CLOSED.** No current-code natural sleep PNG/counter witness was produced.
The available 14:39 roster ROM is contaminated by the stale Ness native failure
above, so it cannot satisfy the required global native-failure count of zero;
older Purin-only ROMs predate this roster-close work. A current rebuild is
blocked by the same protected Ness generator failure.

## End checks

- `python scripts/check-untracked-dependencies.py` — PASS.
- `pwsh -NoProfile -File scripts/check-architecture.ps1` — PASS (2 warnings).
- `pwsh -NoProfile -File scripts/check-melonds-policy.ps1` — PASS.
- `pwsh -NoProfile -File scripts/check-docs.ps1` — PASS.
- `python scripts/fighters/generate_nds_native_owners.py --check` — BLOCKED at
  protected `_bake_unlit_uniform_roots`, line 6617, Ness High root `0x6760`.
- `python scripts/fighters/generate_nds_native_owner_images.py --check` — same
  protected generator blocker via `build_p2_owner_runtime_context`.

No shipped shell default was flipped. No 1P-owned source/router/data/menu probe
file was edited by this package, and the known particle-bank generated outputs
were left untouched.

## Cycle 2 — 2026-09-13

### Step 1 — NDO6 unlit vertex-colour owner format

Closed the Ness Appear1 generator blocker. NDO6 keeps `NDSNativeRun` at 8
bytes: submit-class bits 0..1, polygon alpha bits 2..6, and bit 7 as the
source-unlit vertex-colour flag. Flagged dense shade words are RGB15
`FIFO_COLOR`; ordinary runs remain generated normal words. The image tag is
`0x364F444E` in both generator/header and loader. The Kirby trio verifier now
checks each root against its actual table context; its appended run metadata
and dense shade vectors are extended with the appended runs/dense rows. Runtime
validation masks the metadata byte with `NDS_NATIVE_RUN_SUBMIT_CLASS_MASK`
before physical submit-class admission, matching dispatch/accounting.

Ness root `0x6760` is source-unlit and flagged in both details: High runs
27..31 = `0xfc`; Low runs 25..29 = `0xfc`. Existing one-colour bakes are
unchanged: Samus Catch `0x8d90/0x9140/0x8a70` retain light indices 2/2/3 and
their existing ambient provenance comments; Link Catch `0x7db0/0x7ea8/0x7f98`
retain 8/9/9 and their provenance comments; Link boomerang donor `0xf8`
retains light index 1.

Verification: both owner generators `--check` PASS; owner packet PASS (Mario
`0x40f586c1`, Fox `0x791eb7a6`); geometry closure PASS; wiring PASS
`owners=39 gaps=0`; fighter pytest PASS `261 passed, 2 skipped, 205 subtests`;
`check-gbi-decode-fixtures.ps1` PASS. The fixture checker now pins NDO6 tag,
8-byte run rows, metadata masks, 23 image slots, and measured Ness capacities
(High dense/runs 399/33; Low 293/31). Tarucann's host harness was brought back
to the current flat libnds `m4x4` ABI and the preview/ledger host pins were
updated to their current measured generated values.

### Step 2 — host checks

All four host gates PASS on the Cycle 2 tree: `check-untracked-dependencies.py`
reports `untracked_source_files=0 committed_refs_clean=YES`;
`check-architecture.ps1` passes with its two pre-existing warnings;
`check-melonds-policy.ps1` passes with 13/13 runner slots matching source and
`local_config_audit=0`; `check-docs.ps1` passes (`docs=18`, registry entries 6,
`AGENTS.md=112` lines).

### Step 3 — native-only shell and roster battle builds

The shared lane returned literal `ACQUIRED codex-roster` before each build and
was released immediately afterward. Final-source shell and roster battle both
print `NATIVE_ONLY_PASS`. The battle config records roster 2, kinds
`8/11/10/5` (Kirby/Ness/Purin/Link), with `NDS_P2_NESS=1`, `NDS_P2_PURIN=1`,
`NDS_P2_KIRBY=1`, and `NDS_P2_LINK=1`.

- shell ROM SHA-256 `4538C7CF92468B67F1691EE2467FE78EA1EBADC705BE51686681269446F2DEB9`
- shell ELF SHA-256 `D204FDDA272E6DFA43B29D0C8FDBF9C4D13AE11DF68F1EAA11DCCF86EE754167`
- battle ROM SHA-256 `BD2030391D13E36DADEC5D6E9E0881A9B7184E91178503E05762D8A8DD1B7E3A`
- battle ELF SHA-256 `C7517988E736832B3E155102FE13096D70B741852A5BEAB9D186F5D7F9F96EB8`

The first battle link exposed `.itcm` overflow by 144 bytes: O3 had cloned
the primitive/cross loops around the new invariant run flag. Keeping that
single metadata-byte read volatile at the per-corner shade decision prevents
loop unswitching without changing emitted FIFO data; final
`.itcm.native_fighter` is 7,900 bytes (failed build 8,044 bytes).

### Review item 1 — encoded run-byte admission/table selection

The reported unmasked production validator defect was already fixed: fighter
run validation masks `submit_class` with `NDS_NATIVE_RUN_SUBMIT_CLASS_MASK`
before comparing against `NDS_NATIVE_RUN_CROSS_MATRIX`. A second review seam
was real: hierarchy commit indexed the legacy `sNdsNativeFighterRuns` while
shade/production indexed `sNdsNativeFighterActiveTables->runs`. Hierarchy commit
now reads the active table too, so submit class and shade metadata always come
from the same selected owner/detail/donor table. A repo grep confirms the other
unmasked `submit_class` reads are native-stage rows, not the NDO6 fighter-run
byte.

### Review item 2 — packet prepare reuse

The packet recorder now records one shared `ndsFighterPacketRecordPrepare`
after both the fresh texture-prepare arm and the reuse arm, after NDO6 has
rewritten the run-specific `POLY_ALPHA` and `POLY_FORMAT_LIGHT0` bits. This
prevents replay from inheriting the previous run's lit/opaque polygon format
while replaying FIFO_COLOR corners. Keeping the hook at the common join also
avoids duplicating its branch/call in scarce fighter ITCM.

### Review item 3 — source-alpha provenance

`unlit_vertex_alpha_deltas` is now consumed. A non-uniform flagged run emits an
inline generated provenance comment containing owner/detail/run, the exact
source-alpha histogram, and the encoded five-bit polygon alpha; the owner packet
checker reports those comments as `unlit-alpha-delta` rows. A focused falsifier
proved the comment/report path with a synthetic 0x80/0xff histogram. Current
Ness High and Low contexts both report no deltas: all ten `0x6760` flagged runs
remain uniform opaque (`UNLIT_ALPHA_PROVENANCE=PASS ness_high=none ness_low=none`).

### Review item 4 — CopyLink context-exact verification

`_verify_program_roots_lit` now requires one explicit table context per root;
omitting contexts is a generation error. CopyLink supplies hat/Kirby/Kirby/
LinkBoomerang/Kirby contexts in source order. The stricter check exposed the
real cross-file light carry: the hat establishes `0xffffff00/0x80808000` inside
its state spans, the baked boomerang `0xf8` overwrites it, and the next Kirby
root now gets explicit preamble index 5 restoring that exact pair. Generated
CopyLink High/Low both emit the provenance mark `0xf8=0xffffff00`. Both
generators `--check` PASS; focused Kirby tests PASS `21 passed, 1 skipped, 66
subtests passed`.

### Review item 5 — deliberate PASS2/cull-probe relaxations

Families 4/5 are recorded as the source-exact Ness `0x6760` combines
`TEXEL0*SHADE,G_CC_PASS2` and `SHADE,G_CC_PASS2`. PASS2's second cycle is
COMBINED passthrough and has no ENVIRONMENT input, so those two families
deliberately skip the historical white-ENV admission test; older families keep
it. `NDS_LAB_CULL_PROBE` deliberately uses
`ndsRendererHardwareWriteFighterColorWord`: the tint is emitted inside the
fighter corner loop and must use the fighter GX-record path, avoiding the
broader stage/capture hooks that the fighter writer was split from.

### Step 3 final review rebuild — supersedes the earlier hashes

After the review fixes, both final-source builds reacquired literal
`ACQUIRED codex-roster`, printed `NATIVE_ONLY_PASS`, and released the lane.
The shipped shell keeps its default policy. The lab battle additionally enables
`NDS_TASK68_FALLBACK_CENSUS=1` and `NDS_NATIVE_OWNER_IMAGE_VERIFY=1` so the
requested fallback/image counters are link-resident for Step 4. Its roster is
still `8/11/10/5` (Kirby/Ness/Purin/Link). The proof-enabled renderer fits with
`.itcm.native_fighter = 7,624` bytes.

- shell ROM SHA-256 `F185D97E20CA84CEDE4D763FCA54630E00439149FB6CF535C943BD3E239045CF`
- shell ELF SHA-256 `62D4FF16C30A551AD3A4940EECF6DB7EA3D6C976FB60B222B1938094BADC7B08`
- battle ROM SHA-256 `BACB6C6E1BB6C05C3F9CE49BD5F842B3D07EE5F8E0438343EA65C1AED7CA00E3`
- battle ELF SHA-256 `C4D584AB9BAD07B1811DAF8975341D47332C0AB3D772C491023D86128E081DFC`

### Step 4 — slot-6 Ness `0x6760` runtime proof

Closed on the final proof-enabled battle ROM with repo-local accurate melonDS,
interpreter/JIT-off, runner slot 6. Ness load and entry begin with image/native
failure counters zero. The live validator sees Low-detail root 4 `0x6760` and
reports, in the requested order: `ValidateRejectCode=0`, `DeclineStage=0`,
`FallbackValidate=0`, `ImageFail=0`, `ImageMismatch=0`, and
`gNdsRendererNativeFailure.count=0`.

Ness's source entry ladder is `AppearR/LStart -> AppearWait -> AppearR/LEnd`.
At frame 95 the exact `ftNessAppearWaitSetStatus` transition is entered from
status 221 into idle status 223 with every counter above still zero. On the
first idle draw at frame 96 the selected vector contains root 4 `0x6760`.
Runs 25..29 all carry `meta=0xfc` and execute the packet FIFO_COLOR emitter.
The packet-recorded per-run polygon words are `0x061f0080`, `0x061f00c0`,
`0x071f0080`, `0x071f0080`, `0x071f00c0`; all five decode to
`POLY_FORMAT_LIGHT0=0` and `POLY_ALPHA=31`.

Evidence:
- `artifacts/visibility/2026-09-14_roster-close-ness-appear-cycle2.txt`
- `artifacts/visibility/2026-09-14_roster-close-ness-idle-cycle2.txt`
- `artifacts/visibility/2026-09-14_roster-close-ness-appear1-cycle2.png`
- `assert-melonds-top-visible.ps1 -WindowScaledCapture` PASS: `49152/49152`
  top-screen pixels differ from clear (100.000%).

### Cycle 2c — Kirby copy-hat review table (pre-proof)

| Item | Verdict | Evidence |
| --- | --- | --- |
| (a) copy-state seam writes | done | `src/port/reloc_backend_compat_shims.c:16584-16604`: NKirby leaves both copy unions untouched; real Kirby ensure failure resets only `passive_vars.kirby.copy_id`. No `status_vars.kirby.specialn.copy_id` write remains in the seam. |
| (b) NKirby copy preservation / hat suppression | done | `src/port/reloc_backend_compat_shims.c:16588-16595`: NKirby increments `gNdsNativeKirbyHatSuppressedCount` and selects modelpart 0 at the canonical copy joint without changing copied gameplay state. |
| (c) battle-slot ownership | done | `include/nds/nds_renderer.h:25`, `src/port/renderer_adapter_fighter.c:8`, `src/nds/nds_renderer_assets.c:3932-3937`, `3992-4007`, `5843-5848`: capacity is four slots with a `GMCOMMON_PLAYERS_MAX` static assert and `battle_slot` is explicit in bind/ensure/root resolution; no ambient Kirby-hat battle-slot setter/global remains. |
| (d) assign/consume compile guard | done | `src/nds/nds_renderer_assets.c:5853-5910`: `kirby_hat_slot`/`kirby_hat_root` declaration, assignment, and both consume paths share `NDS_P2_KIRBY && NDS_NATIVE_OWNER_IMAGE_KIRBY`. |
| (e) heap rows / low-water | done | `src/port/reloc_backend_compat_shims.c:16584-16604` reaches hat allocation only for a real Kirby copy commit; NKirby suppresses without allocating. `src/nds/nds_renderer_assets.c:4007-4029` allocates lazily by `(battle_slot, detail)`. The final lab roster is one Kirby (`builds/build-p2-roster-close-battle/nds_build_config.h:85-89`), so at most two rows can exist in this match. `include/nds/generated/nds_native_fighter_image.generated.h:11811` bounds each row by the largest generated hat image; the built maximum is 5,856 B (`kirby_hat_08_high.bin`), so this roster's hard hat-row cap is 11,712 B. The four-Kirby theoretical eight-row ceiling is 46,848 B and is unreachable in this proof roster. |
| (f) public prototypes | done | `include/nds/nds_renderer.h:1561-1568`: `ndsRendererNativeEnsureKirbyCopyHat` lives beside the owner-image sibling. No Kirby-hat prototype is hand-written at `src/port/reloc_backend_compat_shims.c:1-45` or `src/port/renderer_adapter_fighter.c:1-35`. |
| (g) bind/helper/resident-state review | done | `src/nds/nds_renderer_assets.c:3932-3939`: bind has its own slot/detail/base bounds check. `4075` and `4160` helpers have zero call sites, so they are off the runtime hot path. No `gNdsNativeKirbyHatResidentModelPart` / `gNdsNativeKirbyHatResidentDetail` globals remain. |

### Cycle 2c closeout — 2026-09-14

The review fixes are structurally closed. File/line ownership is: (a)/(b)
`src/port/reloc_backend_compat_shims.c:16584-16604`; (c)
`include/nds/nds_renderer.h:25`, `src/port/renderer_adapter_fighter.c:8`, and
`src/nds/nds_renderer_assets.c:3932-3937,3992-4007,5843-5848`; (d)
`src/nds/nds_renderer_assets.c:5853-5910`; (e)
`src/port/reloc_backend_compat_shims.c:16584-16604`,
`src/nds/nds_renderer_assets.c:4007-4029`, and generated cap
`include/nds/generated/nds_native_fighter_image.generated.h:11811`; (f)
`include/nds/nds_renderer.h:1561-1568`; (g)
`src/nds/nds_renderer_assets.c:3932-3939,4075-4105,4160-4192`.

Host closure is green on the cycle 2c tree. `python -m pytest -q scripts/fighters`
finished `262 passed, 2 skipped, 205 subtests passed`. The packet generator,
native-owner geometry closure, owner wiring (`owners=39 gaps=0`), untracked
dependency, architecture, melonDS policy, and docs checks all exited 0.

Both required cycle 2c builds reacquired the shared lane and printed
`NATIVE_ONLY_PASS`. Current identities are:

- shell ROM `4FC6F310750D0F8307D5A1CE5F90FEE17A2FAABAE1C6895DAA6354240544CCCC`
- shell ELF `4E250FD4344A05CD4C35817E3C2A54801CE96C94E15C09154A4BB5EE721E53B9`
- proof battle ROM `8145974A9D72888EB1EBF8E2F88ED182C785299E6BDFA4894E1A17E8AB97D433`
- proof battle ELF `1214E806E636B89A1B3563D9EA2B2C45F32A06B1A4DABA88B1EBF3B3FA7F3E7A`

The proof battle config is still roster `8/11/10/5` (Kirby/Ness/Purin/Link),
compact battle fighters, `NDS_TASK68_FALLBACK_CENSUS=1`, and
`NDS_NATIVE_OWNER_IMAGE_VERIFY=1`. No runtime-owned file was newer than
`builds/codex-roster-stress-itcm.out`, so the conditional ROSTER 1 rebuild was
not triggered. The standing ITCM comparison requested at handoff is ROSTER 1
**7,680 B** versus the pre-cycle2c ROSTER 2 lab **7,624 B**. After the cycle 2c
review fixes, the current ROSTER 2 map records `.itcm.native_fighter = 0x1e00 =
7,680 B` (`builds/build-p2-roster-close-battle/.map:12071-12072`).

Runtime proof status:

- **Ness remains CLOSED** on the already-banked slot-6 witnesses above: Low
  Appear1/idle native validation and failure counters are zero and the dated PNG
  passes the non-clear assertion.
- **Kirby is NOT CLOSED by cycle 2c runtime evidence.** The first slot-6 launch
  reached source Wait at frame 105 with Low detail, live Link (`fkind=5`) and
  `gNdsRendererNativeFailure.count=0`, but its CPU-owned direct-input staging was
  overwritten before capture. The one permitted relaunch switched to external
  controller playback but failed while formatting its generated GDB commands
  before GDB attached. No copy-commit, two-detail hat, pause-zoom, suppression /
  hat-fail terminal, or dated Kirby PNG was produced. Evidence:
  `artifacts/visibility/2026-09-14_roster-close-kirby-cycle2c.txt`.
- **Purin entry is proven; the requested idle/special proof is NOT CLOSED.** The
  single slot-6 launch reached source entry at frame 105, Low detail, with
  `nativefail=0 imageFail=0 imageMismatch=0`. Its first harness waited for Wait
  at a presented-frame boundary and missed that transient. The same melonDS
  process survived detaching, but slot 6 did not reopen its ARM9 GDB listener,
  so the same launch could not be salvaged and no second launch was made. No
  special PNG was produced. Evidence:
  `artifacts/visibility/2026-09-14_roster-close-purin-cycle2c.txt`.

Cycle 2c therefore closes review items (a)-(g), host checks, native-only builds,
hashes and heap-row bounds. Package runtime closure still has two explicit gaps:
Kirby's natural copy/two-detail pause witness and Purin idle + one-special witness.
