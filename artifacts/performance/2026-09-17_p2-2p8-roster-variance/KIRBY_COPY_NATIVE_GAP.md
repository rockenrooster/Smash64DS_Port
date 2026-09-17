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
