# The shield anim-joint install was never ported — 2026-08-13

Cycle 12 (`../2026-08-13_c-anim-anomalies/ANOMALIES.md`, commit `a7b28b66c98`)
attributed the DObj-parser runaway to a broken invariant —
`fp->anim_desc.flags.is_anim_joint` true while a fighter's joints hold
figatrees — and handed the fix forward with one open question: **who is supposed
to clear the flag?** The answer is that nobody was supposed to. The flag is set
truthfully; the function that is supposed to make it true has no body in this
port.

## 1. The flag's full lifecycle in BattleShip — three writers, all present here

`FTAnimDesc` is a **union** of a `u32 word` and a bitfield
(`ft/fttypes.h:48-66`), which is why cycle 12's field-name grep could not find a
clear: two of the three writers write the *word*.

| # | role | BattleShip | port |
|---|---|---|---|
| 1 | init to 0 | `ft/ftmanager.c:718` `fp->anim_desc.word = 0;` | same TU, imported verbatim |
| 2 | **re-derive on every motion install** | `ft/ftmain.c:4633` `fp->anim_desc.word = motion_desc->anim_desc.word;` | same line, via `src/import/battleship_ftmain.c:81` (`NDS_IMPORT_BATTLESHIP_FTMAIN := 1`, `Makefile:1909`) |
| 3 | set for the shield | `ft/ftcommon/ftcommonguard1.c:275` `fp->anim_desc.flags.is_anim_joint = TRUE;` | same line, via `src/import/battleship_ftcommon_guard.c:92` |

Writer 2 is the clear. It is inside `ftMainSetStatus`, guarded by
`fp->figatree != NULL`, and it copies the **whole flag word** out of the motion
descriptor — so a motion whose descriptor does not carry `FTANIM_FLAG_ANIMJOINT`
clears `is_anim_joint` as a side effect of being installed. Mario's own table
proves the shield motions are exactly such motions
(`ft/ftdata.c:275-276`):

```c
{ &llFTMarioAnimShieldOnFileID,  0x80000000, FTANIM_FLAG_TRANSN_JOINT | FTANIM_FLAG_YROTN_JOINT },
{ &llFTMarioAnimShieldOffFileID, 0x80000000, FTANIM_FLAG_TRANSN_JOINT | FTANIM_FLAG_YROTN_JOINT },
```

No `FTANIM_FLAG_ANIMJOINT` (`ft/ftdef.h:33`, `0x00000008`). So the source order
inside one frame is: `ftMainSetStatus(GuardOn)` clears the flag and installs the
**shield-on figatree** on every joint (`ft/ftmain.c:4704`), and then
`ftCommonGuardInitJoints` sets the flag and — in the same breath — **replaces
those scripts**:

```c
fp->anim_desc.flags.is_anim_joint = TRUE;              // ftcommonguard1.c:275
lbCommonAddDObjAnimJointAll(fp->joints[nFTPartsJointXRotN],
                            attr->shield_anim_joints[angle_i], angle_f);
```

`lbCommonAddDObjAnimJointAll` (`lb/lbcommon.c:785`) walks `root_dobj`'s subtree
and gives every joint in it an `AObjEvent32*` from the table, or stops the joint
(`anim_wait = AOBJ_ANIM_NULL`) where the entry is NULL. **That is what makes the
flag true.** There is no missing clear on either side; there is a missing set.

Zero port-side writers of `anim_desc` exist — `rg anim_desc src/ include/`
returns reads only. The port is source-exact on the flag.

## 2. The seam: the installer is an empty stub, and the ELF says so

`src/port/reloc_backend_compat_shims.c:2140` (pre-fix):

```c
void lbCommonAddDObjAnimJointAll(DObj *dobj, AObjEvent32 **anim_joint,
                                 f32 anim_frame)
{
    (void)dobj; (void)anim_joint; (void)anim_frame;
}
```

and in the shipped ROM, `builds/build-c132-stress/…tickhud-hwtri.elf`:

```
02052eac <lbCommonAddDObjAnimJointAll>:
 2052eac:	4770      	bx	lr
```

`src/import/battleship_lb_common.c` — the wrapper that would compile the real
`lb/lbcommon.c` — is **PARKED and not in `CFILES`**, so the stub is the only
definition in the link.

Consequence, exactly as cycle 12 measured it: the flag says AnimJoint, every
joint still holds the GuardOn **figatree**, and `ftParamUpdateAnimKeys`'
per-fighter dispatch (`ft/ftparam.c:386`, mirrored at
`reloc_backend_compat_shims.c:2025`) runs the 32-bit parser over 16-bit
commands. `gcParseDObjAnimJoint`'s `default:` then sets
`anim_wait = AOBJ_ANIM_NULL` and returns, so that joint's animation is dropped
until something re-attaches it — a fighter's joints freezing in their last pose,
in bursts, which is the visible symptom.

Two callers of the stub are live in P1, and the second was silently dead too:

| caller | what never happened |
|---|---|
| `ft/ftcommon/ftcommonguard1.c:277` — the shield | the shield pose's own anim joints were never installed |
| `ef/efmanager.c:5734/5736` — **Fox's entry Arwing** | `llFoxSpecial2EntryArwing{R,L}AnimJoint` were never installed |

The Arwing is corroboration rather than a coincidence: `NDS_RELOC_ASSET_FOX_ANIM_ARWING`
is one of only four assets the port classifies as **AObj32**
(`reloc_backend_assets.c:118-124`), i.e. the port already knew those scripts are
32-bit — it just never installed them.

## 3. The fix

`src/port/reloc_backend_compat_shims.c` — implement `lbCommonAddDObjAnimJointAll`
faithfully to `lb/lbcommon.c:785`, beside `lbCommonAddFighterPartsFigatree`,
sharing its tree walk and its pointer resolve
(`ndsRelocResolvePointerFromFileBase`, so a file-relative table entry is rebased
and an unresolvable one is treated as the NULL entry rather than handed to the
parser).

One line of the shared walk was restored at the same time:
`ndsLBCommonGetTreeDObjNextFromRoot` was missing source's
`else if (a == b) a = NULL;` (`lb/lbcommon.c:757`), so a **childless root** would
have left its own subtree through its sibling. Every root used today has
children, so this restores a guard rather than changing an observed behaviour.

No frame check, no per-joint alignment test at the parser, no loosened bound —
the three things cycle 12 named as forbidden.

## 4. The negative control, and what it says about the runaway counter

`NDS_ANIM_JOINT_AUDIT` (`Makefile:411`, default 0) counts the invariant where it
is consumed. Three counters make a zero readable, which matters: **the
one-minute gate arm reads zero for every one of them and is not evidence of
anything** (`ctl-c134.json`, `build-c134-auditctl`, 1,600 samples, frames
439–2038): `FlagFrames` 0, `Dispatch32` 0, `Figatree` 0 — with
`InstallCalls` **80**, i.e. the shield ran 80 times and the fault still never
occurred. That is the same arm on which cycle 12 measured
`gNdsObjAnimRunawayCount` 0, and it is why this row needs the five-minute arm.

Five-minute both-CPU arm, `build-c134-auditctl5` (stub retained, audit on),
8,448 samples, frames 439–8886:

| counter | value | what it means |
|---|---:|---|
| `gNdsAnimJointFlagFrames` | 1,067 | calls with `is_anim_joint` set |
| `gNdsAnimJointIdleSkip32Count` | 29,605 | joints already stopped, skipped by slice 33 |
| `gNdsAnimJointDispatch32Count` | **144** | joints that reached the 32-bit parser |
| `gNdsAnimJointDispatchFigatreeCount` | **144** | of those, figatrees — **100%** |
| `gNdsAnimJointDispatchMisalignCount` | 96 | 2 mod 4, the subclass the runaway bound can see |
| `gNdsObjAnimRunawayCount` / Mask / Opcode | **50** / 1 / 100 | reproduces cycle 12's 50 exactly |
| `gNdsShieldAnimJointInstallCalls` | 542 | engagement |
| `gNdsAObjEvent32NormalizedHighWater` | 1,019 | reproduces cycle 12's corpus exactly |

**Every 32-bit dispatch in a five-minute match was a misread.** Not most — all
144. And the runaway counter under-reports it by 2.9x: 96 of the 144 were 2 mod 4
and could produce an out-of-range opcode, but **48 were 4-aligned**, so they read
the low 7 bits of a *16-bit* command, decoded to a legal DObj opcode (0..23), and
were consumed as if they were real — no fault, no counter, silent corruption of
that joint's animation. The defect is larger than the counter that found it.

Cost of the audit itself: `WORK-H` P50/P95 929,408 / 1,204,736 against cycle
12's same-arm control 929,344 / 1,205,760 — 64 and 1,024, i.e. nothing.

## 5. Arms

| build | source | arm | role |
|---|---|---|---|
| `build-c134-auditctl` | stub + audit | `BOTH_CPU 1`, soak 0 | 1-minute control — reads 0 for the *right* reason |
| `build-c134-auditctl5` | stub + audit | `BOTH_CPU 1`, **soak 5** | the firing control above |
| `build-c135-animjoint5` | **fix** + audit | `BOTH_CPU 1`, **soak 5** | correctness candidate |
| `build-c136-animjoint` | **fix**, audit off | `BOTH_CPU 1`, soak 0 | gate re-bank |

`arm-none-eabi-size`, control5 → candidate5: text **981,356 → 981,516 (+160)**,
data unchanged, **bss unchanged**. The stub `bx lr` at `0x02052eac` is replaced
by a real prologue at `0x02059bd8`.

Emulator: repo-local `emulators/melonds/melonDS.exe`, DLDI **ON**,
`NDS_TICK_HUD_DRAW 1`. `NDS_R2_SOAK_MATCH_MINUTES` is a lab flag on lab builds
only and is not left configured.

## 6. The candidate — five-minute both-CPU, same window, same sample count

`cand5-c135.json`, frames 439–8886, 8,448 samples, `slips=0`.

| counter | control5 | **candidate5** | required |
|---|---:|---:|---|
| `gNdsAnimJointDispatchFigatreeCount` | 144 | **0** | 0 |
| `gNdsAnimJointDispatchMisalignCount` | 96 | **0** | 0 |
| `gNdsObjAnimRunawayCount` / Mask / Script / Opcode | 50 / 1 / … / 100 | **0 / 0 / 0 / 0** | 0 |
| `gNdsAnimJointDispatch32Count` | 144 | **9,154** | non-zero — the audit is armed |
| `gNdsAnimJointFlagFrames` | 1,067 | **1,067** | unchanged — the flag was never the bug |
| `gNdsShieldAnimJointInstallCalls` | 542 | **542** | unchanged — same shields, same match |
| `gNdsShieldAnimJointAttachCount` | — | **9,154** | — |
| `gNdsShieldAnimJointNullCount` | — | **5,420** | — |
| `gNdsAnimJointIdleSkip32Count` | 29,605 | 20,595 | joints no longer left stopped |
| `gNdsAObjEvent32NormalizeFailCount` | 0 | **0** | 0 |
| `gNdsRelocResolveMisalignCount` | 0 | **0** | 0 |
| `gNdsTaskmanGeneralHeapFreeMin` | — | **70,000** | ≥ 32,768 |
| `gNdsTaskmanArenaAllocFailCount` | — | **0** | 0 |

Three of these are cross-checks rather than results:

- **`FlagFrames` is byte-identical at 1,067 and `InstallCalls` at 542.** The fix
  did not touch the flag or the shield's control flow — it gave the flag
  something true to describe. If the fix had changed *when* fighters shield,
  these would have moved.
- **`AttachCount` equals `Dispatch32` exactly (9,154).** Every script the
  installer attaches is parsed exactly once by the 32-bit parser, which is what
  a correct install/parse pair looks like. Attach + Null = 14,574 over 542 calls
  = 26.9 joints walked per call.
- `Figatree` 0 **while `Dispatch32` is 9,154** is the distinction the one-minute
  arm could not make: the counter is armed, looking at 9,154 dispatches, and
  finds no figatree among them.

### The one number that moved the wrong way

`gNdsAObjEvent32NormalizedHighWater` **1,019 → 1,598**. The ledger is doing its
job (`NormalizeFailCount` 0), but the shield's own scripts are 579 distinct
commands that were never being normalised because they were never being
installed. Against last cycle's `NDS_AOBJ_EVENT32_NORMALIZED_MAX` of 2,048 the
margin falls from **1,029 spare (2.01x the corpus)** to **450 spare (1.28x)**.
That is still a margin and nothing overflowed, but cycle 12's "capacity ≥ 2x the
largest observed corpus" derivation no longer holds and the next cycle should
re-derive it rather than assume it.

### Performance, five-minute arm

| bucket | control5 | candidate5 | delta |
|---|---:|---:|---:|
| **`WORK-H` P50** | 929,408 | **930,688** | **+1,280** |
| **`WORK-H` P95** | 1,204,736 | **1,249,280** | **+44,544** |
| `ALL` P95 | 1,678,784 | — | — |
| VBlank 2 / 3 / 4 / 5+ / max | 7,419 / 1,382 / 70 / 15 / 26 | 7,208 / 1,575 / 84 / 19 / 26 | 211 frames move 2→3 |

**The fix costs P95, and the cost is real work, not placement.** +44,544 is 3.2x
the ±14,080 cross-build floor, while P50 moves +1,280 — the signature of work
that **clusters**: 542 install calls × 26.9 joints, on the ~1,067 logic frames
where a fighter is shielding, i.e. about 12% of the match. Those frames were
previously doing nothing at all for those joints because the installer had no
body. This is owed work: it is what BattleShip does on every frame of
`ftCommonGuardProcUpdate` (`ftcommonguard1.c:483`).

## 7. Gate re-bank — the shipping arm, audit off

`gate-c136.json`, `build-c136-animjoint` (`NDS_R2_BOTH_CPU 1`, soak 0,
`NDS_ANIM_JOINT_AUDIT` **0**), 1,600 samples, frames 439–2038, DLDI ON,
`NDS_TICK_HUD_DRAW 1`, `slips=0`.

| | banked HEAD (`build-c132-stress`) | **fixed (`build-c136-animjoint`)** | delta |
|---|---:|---:|---:|
| **`WORK-H` P50** | 923,392 | **924,928** | **+1,536** |
| **`WORK-H` P95** | 1,210,880 | **1,260,096** | **+49,216** |
| `ALL` P95 | 1,678,720 | 1,678,720 | 0 (VBlank-quantized — it cannot see this) |
| `FTR` P95 | 324,032 | 323,008 | −1,024 |
| `STG` P95 | 165,888 | 167,168 | +1,280 |
| `SRC` P95 | 574,720 | 596,480 | +21,760 |
| VBlank 2 / 3 / 4 / 5+ / max | 1,743 / 267 / 15 / 13 / 26 | **1,697 / 310 / 18 / 13 / 26** | 46 frames move 2→3 |

**Gap to the 1,120,380 gate: 90,500 → 139,716.** Engagement on this arm:
`InstallCalls` **80**, `AttachCount` **1,344**, `NullCount` **800** (2,144 joints
over 80 calls = 26.8/call), `RunawayCount` **0**, `NormalizeFailCount` **0**,
`NormalizedHighWater` **1,177** of 2,048, heap free-min **70,592** (floor
32,768), `ArenaAllocFailCount` **0**, `RelocResolveMisalignCount` **0**.

No spin: this is a P95 regression of 49,216 on an arm that was already 90,500
over its budget, and it is not placement — the 5-minute pair isolates it to
+44,544 with one variable changed. It buys the removal of 144 silent animation
misreads per five minutes and the 50 dropped-joint faults that follow them.
**Whether that trade stands is the owner's call, not this cycle's.**

### The cheapest lever if it has to come back — named, not taken

The N64 does this work once per shielding frame and so does the port now. What
the port *adds* on top, per joint, is two lookups the original never had:

- `ndsRelocResolvePointerFromFileBase` — a loaded-file table scan, **2,144
  calls/minute** on the gate arm (once per joint walked);
- `ndsAObjEvent32NormalizeScript` inside `gcAddDObjAnimJoint` — a ledger lookup,
  **1,344 calls/minute** (once per joint attached).

Both are functions of `(fighter, angle_i)` only, and `attr->shield_anim_joints`
is immutable for a match, so both are memoisable to a per-fighter 8-entry table
of already-resolved, already-normalised pointers built at attach time — without
changing a single consumed value. That is a sizing, not a measurement; price it
before writing it.

## 8. Verifier, ROMs, and the visible check

**`scripts/verify-boundary.ps1 -DelaySeconds 3 -RunnerSlot 2` — "Boundary
verification profile passed."** Full log scanned for `Exception:` and `=FAIL`:
none (`boundary.log` beside this file). Boundary is the right width — the change
is battle-only and touches no normal or shared startup path.

**Root ROMs byte-identical across the whole cycle**, hashed before the first
build and after the last:

```
smash64ds.nds                       11,915,264  54c07fac80c50418949908701f7c2bdbf27512c5f96ac09086fabbb0df6ac68a
smash64ds-battle-playable-hwtri.nds 12,225,536  524448c99c31b62672a63f29914438059d5f9700e10306d147d6342b3223adee
```

No published target was built; every build here is a lab build. **The next
published build WILL differ** — this is a correctness fix in a TU that is linked
into the shipped ROM, and it carries the +49,216 above with it.

### Visible check — matched-tic pair, and it is nearly pixel-identical

The freeze is intermittent (0 in a minute, 144 in five) so it cannot be summoned
for a screenshot. What can be checked is whether the fix disturbs the picture,
and the answer is essentially no. Both arms locked on the guest's own match clock
(`EXACT_LOCK=gSCManagerBattleState->time_remain,30,28`, software renderer):

| region | differing pixels | fraction |
|---|---:|---:|
| guest viewport, both screens | 2,721 / 240,000 | 1.1338% |
| **3D battle screen only** (crop 0,0,400,300) | **18 / 120,000** | **0.0150%** |

The whole-window 1.13% is the tick-HUD's own digits, which differ by
construction. Cropped to the battle, **18 pixels** — and the two arms show the
same fighters in the same places with the same damage (Mario 0%, Fox 58%, one
stock each), i.e. the match had not diverged by 30 seconds in. Captures:
`artifacts/visibility/2026-08-13_animjoint-fix_{ctl,cand}_t{30,29}.png`, diff
`…_diff_t30.png`.

Stated plainly: **neither fighter is shielding in that frame**, so this pair
proves "no visual regression", not "the shield pose is now correct". The counters
are the proof of the latter.
