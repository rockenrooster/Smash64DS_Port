# Kirby's copy ability leaves the native path for 10 of 11 copyable fighters

Exercising a second legal roster produced **7,679 native rendering failures**
where the canonical roster produces 0. Tracing them found a shipping-blocking
correctness gap that has nothing to do with the roster that exposed it.

**Kirby's swallow-copy renders natively only when the victim is Link.** For the
other ten copyable kinds the fighter's parts are rejected and **do not reach the
screen**.

## The failure record, decoded

| field | value | decodes to |
|---|---|---|
| `domain` | 1 | `NDS_NATIVE_FAILURE_FIGHTER` |
| `reason` | 2 | `NDS_NATIVE_FAILURE_REJECTED_PROGRAM` |
| `scene` | 22 | `nSCKindVSBattle` |
| `identity` | `0x0008_0148` | `(fkind << 16) | asset_id` = **kind 8 Kirby**, asset `0x148` `llKirbyModelFileID` (`renderer_adapter_fighter.c:3035`) |
| `status` | 277 | `nFTCommonStatusSpecialStart(220) + 57` = **`nFTKirbyStatusSpecialNCopy`** |
| `root` | `0xFE18` | **joint 6, model part 9, LOW detail** — confirmed by `src/nds/generated/nds_native_fighter_kirby_hat_09_low.image.c:12-19` (`copy_modelpart_id = 9`, `root_offset = 0x0000fe18`) |
| `material` | `0x023B48C8` | the rejecting DObj's first `MObj`, for debugger walking |
| direct rejects | 0 | not a GBI-decode reject |

So: **Kirby, joint 6, model part 9 — the Captain Falcon copy hat.**

## The mechanism

BattleShip's copy table (`decomp/.../relocData/228_KirbyMainMotion.c:132-160`)
maps each victim to the hat model part Kirby wears:

| victim | hat | victim | hat |
|---|---:|---|---:|
| Mario | 12 | Captain | **9** |
| Fox | 7 | Kirby | 0 (none) |
| Donkey | 4 | Pikachu | 6 |
| Samus | 8 | Purin | 3 |
| Luigi | 11 | Ness | 13 |
| **Link** | **10** | Yoshi | 5 |

During `SpecialNCopy` joint 6 swaps to the hat while the hidden trio body at
joint 7 is still live. The port then accepts only three heads:

```c
/* src/port/renderer_adapter_fighter.c:1125-1127 */
if ((fp->modelpart_status[slot].modelpart_id_curr == 1) ||
    (fp->modelpart_status[slot].modelpart_id_curr == 10) ||
    (fp->modelpart_status[slot].modelpart_id_curr == 14))
```

1 is the inhale face, 14 the boomerang face, **10 is Link's hat**. Head 9 is not
in the set, so `kirby_trio_unknown` becomes TRUE, `native_owner_enabled` is
cleared with `gNdsFtrDeclineStage = 11`, and every selected DObj falls to the
`else` at `renderer_adapter_fighter.c:4489` recording `REJECTED_PROGRAM`.

That three-head limit is not arbitrary — it is the generator's own contract:

```python
# scripts/fighters/generate_nds_native_owners.py:2465
KIRBY_TRIO_CONTEXTS = ((1, 0), (14, 0))  # (head_mp, body_mp)
```

plus `KIRBY_COPY_LINK_MODELPART_ID = 10` (:2353). The trio body's bake inherits
the head's vertex cache, so **there is one body program per head, and only three
heads have one.**

## This is not diagnostic-only

At `NDS_RENDERER_PROFILE_LEVEL 0` the `else` branch draws nothing — the only
executor in that loop is `ndsRendererExecuteNativeFighterRoot`. **A rejected
root is a root that never reaches the screen.** The comment at `:3411` about
failing "closed to the generic renderer" is stale: there is no generic renderer
in a `< 2` build, and `AGENTS.md` forbids one.

It also explains the arm's apparent speed. Captain/Luigi/Donkey/Kirby measured
WORK-H 1,471,552 against the canonical 1,588,544 — **cheaper because it is not
drawing.** That figure must not be used as a roster-cost datum.

## What was refuted along the way

| hypothesis | verdict |
|---|---|
| missing owner image | **No.** All 22 `kirby_hat_NN_{high,low}.bin` are staged, including `kirby_hat_09_low.bin` (4,584 B). |
| guard compiled out | **No.** `NDS_P2_KIRBY`, `NDS_P2_CAPTAIN`, `NDS_P2_LUIGI`, `NDS_NATIVE_OWNER_IMAGE_KIRBY`, `NDS_NATIVE_KIRBY_ROOT_PROGRAMS_PRESENT` are all 1 in this build. |
| Captain's or Luigi's own model | **No.** Captain appears only as the swallowed victim; the rejected model is Kirby's. |

**One sub-claim in the investigation was itself wrong and is corrected here:** it
reported that `NDS_NATIVE_OWNER_IMAGE_LINKBOOMERANG` "does not exist anywhere in
the tree". It appears **58 times** in `src/nds/nds_native_fighter_owner.generated.inc`
and is **never defined**, so every one of those `#if` blocks evaluates 0 and the
compiler emits `-Wundef` warnings for them. That file is generated, which is why
a tracked-file search missed it. The separate report about it stands.

## The canonical roster passes on luck, not structure

The defect depends on **who Kirby swallows**, not on who is in the lineup.

| copyable victim | hat | trio body program? |
|---|---:|---|
| Link | 10 | **yes** |
| Kirby (mirror) | 0 | n/a |
| Mario, Fox, Donkey, Samus, Luigi, Yoshi, Captain, Pikachu, Purin, Ness | 12,7,4,8,11,5,9,6,3,13 | **no** |

**10 of 11 copyable kinds break.** The canonical roster contains Donkey (hat 4)
and Samus (hat 8) — both broken — and scored 0 only because its CPU Kirby never
completed a swallow-copy of a non-Link victim in that input trace. **`count = 0`
on one roster is a statement about one input stream, not about the roster.**

`NDS_P2_KIND_ADMITTED` (`src/port/nds_match_config.c:160-180`) admits all twelve
playable kinds, so on the shipping menu this is reachable by hand immediately
with any Kirby plus non-Link lineup.

## Fix, and the check that should have caught it

**Fix — bake a trio body context per copy hat.** Extend `KIRBY_TRIO_CONTEXTS` to
include `(h, 0)` for each needed hat, widen the accept set at
`renderer_adapter_fighter.c:1125-1127` to match *and derive it from the
generated table* so the two cannot drift again, and emit the per-head body bake
into the existing deferred hat image rather than the resident owner — zero ARM9
and arena cost, and `NDS_NATIVE_KIRBY_HAT_MAX_BYTES` already sizes for the
largest hat.

Do all **10 hats x 2 details = 20 bakes**, not the 3 this roster needs: a
3-hat patch reintroduces the same roster-shaped hole. **Do not widen the accept
set without the bakes** — that resolves the body against the wrong head's vertex
cache, which is the corruption the generator refuses to ship
(`generate_nds_native_owners.py:2248-2254`).

**Check — a table cross-product, seconds, no ROM:**

```
for kind in admitted_kinds(nds_build_config.h):
    hat = copy_table[kind].copy_modelpart_id     # 228_KirbyMainMotion.c:132-160
    assert hat == 0 or (hat, 0) in KIRBY_TRIO_CONTEXTS
assert head_accept_set(renderer_adapter_fighter.c) \
       == {h for (h, _) in KIRBY_TRIO_CONTEXTS} | {KIRBY_COPY_LINK_MODELPART_ID}
```

Home it in `scripts/fighters/check_native_owner_geometry_closure.py`, which
already imports `native.KIRBY_TRIO_CONTEXTS` (:324). It must **parse the copy
table and run the generator**, not grep: the hat ids are decomp source data and
the accept list is three bare integer literals that no string search connects to
them. That is the same trap that has now produced four wrong "nothing references
it" claims in this session, including one inside the investigation above.

**Harness gap worth closing at the same time.** A nonzero
`gNdsRendererNativeFailure.count` tells you a fighter vanished but not why.
`gNdsFtrDeclineStage`, `gNdsNativeFighterValidateRejectCode/Index` and
`gNdsNativeKirbyHatFailCount/LoadCount` are all `__attribute__((used)) volatile
u32` and none is read by `verify-p2-four-fighter-stress.ps1`. Adding them to
`$memoryGlobals` costs nothing and would have named this in the first run
instead of requiring source archaeology.

---

## The fix has a dependency the investigation missed, and it is the risky part

Extending `KIRBY_TRIO_CONTEXTS` is **not** sufficient, and is not a data change.
Each head also needs an entry in `KIRBY_TRIO_PROGRAM_CROSS_SLOTS`
(`generate_nds_native_owners.py:2471`), and the generator validates only its
*length*:

```python
# generate_nds_native_owners.py:2560
cross_slots = KIRBY_TRIO_PROGRAM_CROSS_SLOTS[head_mp]
if len(cross_slots) != len(roots):
    raise ValueError(...)
```

The two shipped entries are:

```
 1: (17, 16, 17, 16, 19, 18, 31, 31, 31)
14: (17, 16, 17, 16, 19, 18, 31, 31, 31, 31)
```

These are **physical GX matrix-palette slots** deciding which roots stay
resident while root 1 (the body) executes its `MODIFYVTX` reads. The tempting
inference is that the rule is "(17,16,17,16,19,18) then 31 padding to the root
count", which would make all ten heads derivable in one line.

**That inference is not safe to act on, and this is where the work stops.**

The length check would pass for a wrong slot *sequence*. A wrong sequence does
not raise — it resolves the trio body against the wrong head's vertex cache and
emits corrupted geometry, which is precisely the failure the generator's own
comment at `:2248-2254` refuses to ship. So the one guard that exists cannot
catch the one mistake that matters.

Deriving the ten sequences requires each hat's actual weld topology: how many
canonical two-root welds it has, which roots those are, and which must stay
resident across the body's reads. That is real per-hat analysis against the
BattleShip joint-6 model-part DLs, not a pattern extrapolated from a sample of
two.

**Recommended order for whoever takes this:**

1. Derive the cross-slot sequence for one new hat (Donkey, part 4 — it is in the
   canonical roster, so the existing gate exercises it once it works).
2. Bake it, run the four-CPU stress with Kirby and Donkey, confirm
   `gNdsRendererNativeFailure.count` stays 0 **and** that Kirby's copy is
   visually correct — the count alone cannot see corruption.
3. Only then generalise the rule to the remaining nine, with the visual check
   repeated per hat.

**Definition of done:** `scripts/fighters/check_native_owner_geometry_closure.py`
goes green. It is RED today by design and names every missing victim, so it is
both the specification and the regression test.

## Harness additions worth doing alongside

A nonzero `gNdsRendererNativeFailure.count` says a fighter vanished, not why.
These are all `__attribute__((used)) volatile u32` already and none is read by
`verify-p2-four-fighter-stress.ps1`:

- `gNdsFtrDeclineStage` — would have read 11 ("live trio body, unknown joint-6
  part") and named this immediately
- `gNdsNativeFighterValidateRejectCode` / `...Index`
- `gNdsNativeKirbyHatFailCount` / `...LoadCount`

Adding them to `$memoryGlobals` costs nothing and turns "a fighter vanished"
into "this decline stage, this part".

### Scope, measured rather than estimated

Running the generator's own spec builder settles what each new head actually
needs. `build_kirby_trio_faithful_specs(canon_roots, 'low', head)`:

| head | result |
|---|---|
| 1 (inhale face) | **9 roots**, bindings 0..8 |
| 14 (boomerang face) | **10 roots**, bindings 0..9 |
| 10 (Link hat) | `ValueError: unknown head modelpart 10` |
| 4 (Donkey), 8 (Samus) | `ValueError: unknown head modelpart` |

Two things fall out that change the estimate.

**Link's hat does not go through this path at all.** Head 10 raises like any
other unknown head; it works through `KIRBY_COPY_LINK_MODELPART_ID` as a
separate mechanism. So the trio-context path supports exactly **two** heads, both
of them faces, and **zero** copy hats. Every copy hat is new work, Link included
in spirit if not in code.

**The root count is structural, not arbitrary.** Kirby's canonical low program
has **7** roots. `SpecialN` adds hidden joints 7 and 19, giving 9 — head 1. The
docstring at `:2494-2495` says "Link-copy adds joint 18 too", giving 10 — head
14. So an ordinary copy hat that adds no further joint should be **9 roots with
head 1's shape**, and `(17,16,17,16,19,18,31,31,31)` is then a *derivation*
rather than an extrapolation.

**That still must be verified per hat, not assumed**, because the one guard that
would catch a wrong slot assignment is switched off on this path:

```python
# generate_nds_native_owners.py:2570
build_direct_dense_tables(..., validate_cross_census=False)
```

So the four per-head sites are narrower than first recorded:

| site | what it needs | risk |
|---|---|---|
| `KIRBY_TRIO_CONTEXTS:2465` | one tuple entry | none |
| `kirby_trio_head_offset:2485` | **only the guard** — the body is already generic (`specs[head_mp - 1][1]`) | none |
| `kirby_trio_variant_schema:2637` | `list(range(9 if head_mp == 1 else 10))` generalised to the real root count | low |
| `KIRBY_TRIO_PROGRAM_CROSS_SLOTS:2471` | the slot sequence | **the whole risk** |

**Recommended first move, unchanged but now cheap:** admit Donkey's hat (4)
alone, let the spec builder report its root count, and if it is 9 use head 1's
sequence. Then bake, run the canonical four-CPU roster — which contains Donkey,
so the existing gate exercises the copy — and check it **visually**, because
`gNdsRendererNativeFailure.count` cannot see wrong-cache corruption. If that
holds, the remaining nine follow the same procedure one at a time.

---

## The fix is not twenty bakes. The trio seam assumes a RESIDENT head, and every copy hat is DEFERRED.

Admitting Donkey's hat (head 4) got as far as the bake and stopped on:

```
ValueError: kirby trio head4: color escape dense 116 has no
            value-identical main-table row
```

Diagnosing that row settles what the remaining work actually is.

`head4 dense116 = (x=21, y=170, z=-36, s=512, t=358, binding=0, cache_slot=1,
rgba=1749280255)`. Relaxing **any single field** of the match key still finds no
twin, so this is not a keying mismatch. The resident main table has **zero** rows
at that position; head 4's own bake has three.

Comparing all three heads against the resident table (`high` detail):

| head | distinct positions | **absent from the resident main table** |
|---|---:|---:|
| 1 (inhale face) | 143 | **0** |
| 14 (boomerang face) | 184 | **0** |
| **4 (Donkey copy hat)** | 185 | **91** |

**Heads 1 and 14 are faces — ordinary model parts, fully resident. Copy hats are
deferred images** (`kirby_hat_NN_{high,low}.bin`), and 91 of Donkey's 185
positions exist only inside its own image.

The trio body's `MODIFY_ST` copies take their shade from head and canon rows and
resolve each escape into the **resident** table. A deferred hat's geometry is not
there, and cannot be without making the hat resident — which is precisely what
the deferred design exists to avoid.

**That is why this seam has only ever carried faces.** It is not an oversight in
a table; the mechanism has never supported a non-resident head.

### What this means for the work

The earlier plan — "extend `KIRBY_TRIO_CONTEXTS`, add cross slots, twenty bakes"
— is **wrong about the shape of the job**. The table entries and the slot
derivation are now done and verified, and they were never the hard part. The
real choice is:

| option | cost |
|---|---|
| **(a) make copy hats resident** for the trio case | pays back exactly the ARM9 resident bytes the deferred hat design was built to save, for 10 hats x 2 details |
| **(b) resolve colour escapes into the deferred image's own table** | a real change to `_append_kirby_trio_sections`' escape resolution, but no residency cost and it generalises to every hat at once |

**(b) is the right shape** and matches where the hat data already lives, but it
is renderer-seam work on the path that `validate_cross_census=False` leaves
unguarded — so it needs the closure checker green *and* a visual check per hat,
not one or the other.

### What is already banked

- `check_native_owner_geometry_closure.py` fails and names all ten victims: the
  specification and the regression test.
- Cross slots derive from the root count and **regenerate heads 1 and 14's
  original hand-authored tuples exactly**, so no future head can be authored
  with a wrong sequence that the length check would wave through.
- `kirby_trio_root_count()` replaces `9 if head_mp == 1 else 10`, which would
  have silently given every newly admitted head ten bindings when an ordinary
  copy hat has nine.
- Head 4's failure is recorded at the table it would be added to, so the next
  attempt starts here instead of rediscovering it.
