# The two cycle-11 animation anomalies, attributed — 2026-08-13

Cycle 11's stress battery (`../2026-08-13_c-stress/STRESS_GATE.md`, HEAD
`bb3424c4d11`) left two counters open and unattributed. Both are attributed
here at their owning seams, from **one 1,600-sample gate run and two GDB probes
on the already-built ROMs — no build was spent on either attribution.**

| | anomaly 1 | anomaly 2 |
|---|---|---|
| counter | `gNdsObjAnimRunawayCount` | `sNdsAObjEvent32NormalizedCount` |
| cycle 11 said | "`Mask` 1, `Script` 0x023C138A, `Opcode` 100 — not attributed" | "linear growth to 1,019/1,024 in five minutes" |
| verdict | **the 32-bit parser is running on a 16-bit figatree**; opcode 100 is an artefact, not vocabulary | **capacity-shaped, not a leak**; growth is corpus coverage |
| owning seam | `fp->anim_desc.flags.is_anim_joint` true while joints hold figatrees | `NDS_AOBJ_EVENT32_NORMALIZED_MAX` |
| this cycle | attributed, **fix handed forward** | attributed **and fixed** |

## ROM identity

| | |
|---|---|
| gate + anomaly 2 trajectory | `builds/build-c132-stress` — `NDS_R2_BOTH_CPU 1`, `SOAK_MATCH_MINUTES 0` |
| anomaly 1 probes | `builds/build-c132-stress5` — `NDS_R2_BOTH_CPU 1`, `SOAK_MATCH_MINUTES 5` |
| emulator | repo-local `emulators/melonds/melonDS.exe`, DLDI **ON** |
| root ROMs | unchanged this cycle (hashes in the commit packet) |

---

# Anomaly 1 — the DObj-parser runaway

## 1. Opcode 100 is not source vocabulary, and never was

`0x023C138A` is **2 mod 4**, so it cannot be an `AObjEvent32*`; the 2026-08-03
report (`docs/KNOWN_ISSUES.md:475`) carried `0x238561A`, also 2 mod 4, with the
same mask and the same opcode. The port's legal DObj opcode space is 0..23
(`ndsAObjEvent32PlanStream`'s switch); 100 is outside it and could never be
decoded "source-faithfully". **There is no decoder gap to close** — the value is
what an ARM9 `LDR` returns from a misaligned address, and the capture proves it
arithmetically rather than by argument:

| hit | script | aligned word | rotation | `& 0x7f` | reported opcode |
|---|---|---|---|---|---|
| 1 | `0x023611da` | `0x80e4ff78` | ror 16 → `0xff7880e4` | `0x64` | **100** |
| 3 | `0x0236128e` | `0x80e4ff67` | ror 16 → `0xff6780e4` | `0x64` | **100** |
| 5 | `0x0236130e` | `0x80e4ffc2` | ror 16 → `0xffc280e4` | `0x64` | **100** |

and the three 4-aligned hits need no rotation at all — the parser simply read
the low 7 bits of a **16-bit** command:

| hit | script | halfword at script | `& 0x7f` | reported opcode |
|---|---|---|---|---|
| 2 | `0x02361218` | `0x0ec9` | `0x49` | **73** |
| 4 | `0x023612cc` | `0x00c9` | `0x49` | **73** |
| 6 | `0x02361344` | `0x0029` | `0x29` | **41** |

Six hits, six opcodes, all six predicted exactly. `runaway-owner.txt`.

## 2. Whose script — named, not guessed

`scripts/probe-objanim-runaway.ps1` (new, no build) derives the fault block from
the ELF rather than hard-coding it — in `gcParseDObjAnimJoint` the two blocks are
the only code that touches `gNdsObjAnimRunawayCount`, and each opens with the
`movs r1, #MASK` that supplies its own mask bit. It resolves to `0x02068100`,
where the store trail proves the live registers:

```
2068100:  2101  movs r1, #1           <- mask bit 0
2068114:  601c  str  r4, [r3, #0]     <- r4 = dobj->anim_joint.event32
2068118:  601e  str  r6, [r3, #0]     <- r6 = command_kind
206811c:  676b  str  r3, [r5, #116]   <- r5 = dobj (+0x74 = anim_wait)
```

All six hits, and all four hits of the second probe run, agree:

- **one GObj**, `0x2362950`, `id` 1000, **`link_id` 3 = `nGCCommonLinkIDFighter`**
  (`objdef.h:76`) — *a fighter*, `func_run`/`func_anim` both NULL;
- **six different DObjs of its tree**, `0x2362e90 … 0x2363d90`;
- every faulting script inside loaded file **`asset=557` = `0x22d` =
  `NDS_RELOC_ASSET_MARIO_ANIM_SHIELD_ON`** (`reloc_backend_assets.c:121`),
  base `0x02361100`, size 768 — the only one of the 38 loaded files that
  contains the address. Offsets `0xDA, 0x118, 0x18E, 0x1CC, 0x20E, 0x244`.

**The Whispy hypothesis is refuted, and refuted by identity rather than by
absence:** the owner is a fighter GObj and a Mario animation asset. It is not the
tree face, not the flower beds, not the wind. `docs/BUGS.md` row 1 is untouched
by this cycle, and so is `OWNER_DECISIONS.md` §1 — that question did not change.

## 3. The mechanism, and why the pointer is 2 mod 4

`DObj.anim_joint` is a **union**: `event32` (4-byte commands) and `event16`
(2-byte commands). `ftAnimParseDObjFigatree` advances `anim_joint.event16` in
place, so a joint part-way through a figatree legitimately holds a **2 mod 4**
pointer. That is the whole of the misalignment: three of six hits are joints
caught mid-figatree, three are joints that happened to sit on an even command.
The alternating 2,0,2,0,2,0 across six independently-advanced joints is the
signature.

`bt` names the caller on every hit (`runaway-caller.txt`):

```
#0  gcParseDObjAnimJoint (dobj=0x2362e90)  objanim.c:668
#1  0x020553c2 in ftParamUpdateAnimKeys    reloc_backend_compat_shims.c:2017
```

and that function chooses the parser **per fighter, not per joint**:

```c
if (fp->anim_desc.flags.is_anim_joint) gcParseDObjAnimJoint(joint);
else                                   ftAnimParseDObjFigatree(joint);
```

**The port's dispatch is source-exact** — `decomp/.../ft/ftparam.c:386` and
`:412` are the same two lines. So the divergence is not the dispatch; it is the
flag. `fttypes.h:59` documents the invariant: *"whether current animation is type
Figatree (0) or AnimJoint (1)"*. The fault is the invariant broken — the flag
says AnimJoint while the joints hold figatrees.

## 4. What it costs, exactly

`default:` records the fault and does `dobj->anim_wait = AOBJ_ANIM_NULL; return;`
(`scripts/decomp-patches/battleship/src_sys_objanim.patch:57-63`). `AOBJ_ANIM_NULL`
is `F32_MIN` (`objtypes.h:41`) and the parser's own entry test is
`anim_wait != AOBJ_ANIM_NULL`, so **that joint's animation is dropped** and stays
dropped until something re-attaches — and slice 33's idle skip
(`compat_shims.c:1998`) then skips the joint entirely. So the visible cost is a
fighter's joints freezing in their last pose, in bursts of about six, until the
next action change re-arms them. No freeze: the bound is doing its job, and this
is the containment working, not the bug.

## 5. Where the next cycle picks it up

The seam is `fp->anim_desc.flags.is_anim_joint`, and the open question is
narrow. In this tree the **only** writer is
`decomp/.../ft/ftcommon/ftcommonguard1.c:275`:

```c
fp->anim_desc.flags.is_anim_joint = TRUE;
lbCommonAddDObjAnimJointAll(fp->joints[nFTPartsJointXRotN],
                            attr->shield_anim_joints[angle_i], angle_f);
```

i.e. the **shield** sets the flag and installs event32 shield scripts in the same
breath, which is correct and self-consistent. `grep` over `decomp/src`, `src/`
and `include/` finds **no writer that clears it** — so on this evidence the flag
is latched by the first guard and never restored when the next ordinary figatree
motion is installed. That is a hypothesis with one grep behind it, **not a
measured fact**, and the honest statement is "not found where I looked": the
clear may live in a whole-struct assignment the field-name grep cannot see.

The next cycle needs exactly two things, neither expensive:

1. **The installer side.** Read BattleShip's figatree-install path
   (`lbCommonAddFighterPartsFigatree` and its `ftMain*`/`ftAnim*` callers)
   against the port shim (`reloc_backend_compat_shims.c:8990`) and find who is
   supposed to clear `is_anim_joint`. If nothing does, the flag's clear is the
   port's to add at the install seam — never a frame check and never a
   per-joint alignment test at the parser.
2. **The engagement counter that this cycle could not add without a build.**
   One counter incremented in `ftParamUpdateAnimKeys` when
   `is_anim_joint` is true, plus a second when the joint's `anim_joint`
   pointer lies in an asset `ndsRelocPointerIsFighterAObj16` claims — the
   ratio names how often the invariant is broken, and it is the *negative
   control* for any fix (it must go to zero while the shield's own parse count
   stays non-zero).

**Do not "fix" this by loosening the parser's bound or by teaching it opcode
100.** The bound is correct, the fault is correct, and the data it is refusing
is genuinely not its data.

## 6. Rate — cycle 11's "≈1 per 6 s of scene time" is withdrawn

Measured on this cycle's gate run (1,600 samples, frames 439–2038, ~87% of a
one-minute both-CPU match): `gNdsObjAnimRunawayCount` **0**, `Mask` **0**,
`Script` **0**, `Opcode` **0**. Zero, not ten. The event is bursty and
match-phase-dependent, not a constant rate; the five-minute figure of 50 is
roughly eight bursts of six, not 50 independent events.

**Probe caveat, stated because it affects a number in the capture:** the counter
read at hit *n* lags the faults already taken (hit 3 reads 2, hit 6 still reads
2, and `RWDONE` reads 2 after six real hits). The breakpoint hits are the
ground truth — each is a distinct DObj at a distinct address — and the global
reads through the GDB stub do not see writes still sitting in the ARM9 data
cache. `r5ok=0` on the even-aligned hits is the same effect: the register is
current, the memory read of the same field is not.

---

# Anomaly 2 — the AObj event-32 ledger

## 1. It is not a leak. Measured, on the gate arm

`-PerStopGlobals` on the 1,600-sample gate run gives the whole trajectory for
free (`gate-c132.json`, `ringStopReads`):

| stop | frame | commands | Δ | reuses | Δ | scripts | Δ |
|---|---:|---:|---:|---:|---:|---:|---:|
| 0 | 534 | 322 | — | 59 | — | 43 | — |
| 2 | 726 | 467 | +138 | 95 | +9 | 62 | +17 |
| 4 | 918 | 586 | +93 | 129 | +20 | 79 | +12 |
| **9** | 1398 | **641** | **0** | 201 | +16 | 89 | 0 |
| **10** | 1494 | **641** | **0** | 220 | +19 | 89 | 0 |
| **11** | 1590 | **641** | **0** | 236 | +16 | 89 | 0 |
| **12** | 1686 | **641** | **0** | 252 | +16 | 89 | 0 |
| 13 | 1782 | 767 | +126 | 261 | +9 | 103 | +14 |
| 16 | 2038 | **889** | 0 | **294** | 0 | **119** | 0 |

**Four consecutive stops — 384 presented frames — add nothing while the reuse
path fires 16-19 times each.** A key that included anything per-spawn (an arena
address, an instance id) could not do that: every spawn in that window would
have taken a new slot. The key is the command's address inside its loaded file,
it is stable, and 294 of 413 normalize calls (71%) hit reuse. **Growth is
coverage of a finite corpus of distinct scripts**, and it saturates: 889 at ~52 s
against **1,019 at ~296 s** — the marginal rate falls from 17/s to 0.53/s.
Cycle 11's "linear ~3.4/s" is withdrawn.

## 2. Reclaim is structurally impossible, so the bound is the fix

The repack is a bit permutation with no spare bit
(source `opcode[31:25] flags[24:15] payload[14:0]` → native
`opcode[6:0] flags[16:7] payload[31:17]`), so **a word cannot say which layout it
is in** and this table is the only record. Therefore:

- **evicting an entry is corruption**, not a cache miss: the next attach of that
  script re-applies the permutation and the script is destroyed;
- **reclaiming at the owning lifetime cannot help a long match** — the only
  correct discard point is `ndsAObjEvent32ResetNormalizedScripts()` from
  `ndsRelocResetLoadedFiles()` (`reloc_backend_assets.c:2093`), and a match
  unloads nothing.

It is a **ledger, not a cache**. That leaves capacity, which is what the brief
reserves for exactly this case.

## 3. What overflow actually does — worse than "skips the attach"

Every wrapper in `battleship_sys_objanim.c` skips the whole base call on a
FALSE: `gcAddDObjAnimJoint:1508`, `gcAddMObjMatAnimJoint:1521`,
`gcAddAnimJointAll:1539`, `gcAddMatAnimJointAll:1560`, `gcAddAnimAll:1570`,
`gcAddCObjCamAnimJoint:1581`. And `ndsAObjEvent32NormalizeDObjTable:1416`
returns FALSE on the **first** failing joint, so one over-cap script cancels
**every joint of that GObj**, not just its own — the object keeps its previous
pose and its `anim_frame` is never set. That is the same class of stale-script
state anomaly 1 shows is dangerous.

## 4. The fix, and why 2048 is derived rather than picked

`NDS_AOBJ_EVENT32_NORMALIZED_MAX` 1024 → **2048**, entry 8 B, **+8,192 B bss**
against 176,128 B of proven boot headroom.

- largest corpus ever measured: **1,019** (five-minute both-CPU match);
- shipping match length already at **889 of 1,024 = 87%**;
- 2048 leaves **1,029 spare slots — more than the entire first minute's
  demand** — i.e. capacity ≥ 2× the largest observed corpus.

`gNdsAObjEvent32NormalizedHighWater` is added in the same change and is the
reason the next cycle can re-derive this instead of re-measuring it.
**It also fixes a reporting defect**: `sNdsAObjEvent32NormalizedCount` is reset
on every scene teardown, so an end-of-run read reports the LAST scene only —
which is how a five-entry chain reported 297 and was written up as "a chain
cannot fill the table" while a single match stood at 889.

## 5. Qualification

See `QUALIFICATION.md` beside this file.

---

# Task 3 — the same-tree gate bank

`sample-tick-hud-buckets.ps1 -Build build-c132-stress -NoBuild -RingDump
-Samples 1600 -StartFrame 438`, DLDI **ON**, `NDS_TICK_HUD_DRAW 1`, both-CPU,
frames **439–2038**, `slips=0`, one ring-seam duplicate LABEL at row 576 (no
payload repeat). `gate-c132.{json,-rows.csv,-run.log}`.

| bucket | P50 | P95 | mean |
|---|---:|---:|---:|
| **`WORK-H`** | **923,392** | **1,210,880** | 947,305 |
| `ALL` | 1,118,272 | 1,678,720 | 1,207,139 |
| `FTR` | 298,048 | 324,032 | 292,049 |
| `STG` | 161,600 | 165,888 | 161,991 |
| `SRC` | 320,704 | 574,720 | 349,804 |
| `WAIT` | 211,136 | 493,568 | 224,913 |

**VBlank interval histogram: 2 × 1,743, 3 × 267, 4 × 15, 5+ × 13, max 26**, over
2,038 presented frames — 85.5% hold the 2-VBlank (30 Hz) cadence, 98.6% within 3.

**The bank is confirmed on HEAD's own tree, and it is byte-identical to the
banked figure** (923,392 / 1,210,880 from `build-c131-cand`, slice 50). HANDOFF
recorded that reference as "not this HEAD" and owed a fresh reading; the reading
reproduces it exactly, which is what a bit-deterministic sampler on an
unchanged-behaviour tree should do. **Gap to the 1.12M gate: 90,880.**
