# Kirby CopyLink native mixed-file closure — 2026-09-12

## Scope

This closes the measured natural **Kirby CopyLink Neutral-B family** under
P2-3f47.  It does not claim every Kirby copy power, Stone, Ness/Purin roster
acceptance, or global fighter-native closure.

The six BattleShip sibling statuses are `0x122..0x127` (ground throw/get/empty
and air throw/return/empty).  The proof below is feature-scoped to that family
so a later unrelated Kirby state cannot steal the terminal witness.

## Source contract and the original wrong model

The first natural failure was frame 1324, status `0x122`, with eight selected
fighter roots.  Early attempts incorrectly treated the foreign subtree as
`LinkModel`.  The decisive pre-validation witness instead reported:

```text
idx:3 asset:326 detail:0xf8
```

BattleShip explains that exactly:

- `decomp/BattleShip-main/decomp/src/ft/ftdata.c` gives every CopyLink motion
  descriptor animation flags `0x02000000`.
- `decomp/BattleShip-main/decomp/src/ft/ftmain.c` scans those animation bits
  from bit 31 downward when adding hidden parts, so `0x02000000` enables Kirby
  hidden-part ID 6.
- `decomp/BattleShip-main/decomp/src/relocData/229_KirbyMain.c` hidden-part row
  6 inserts **joint 12 under joint 11**.
- The same file's `dKirbyMain_modelparts_desc_0x39C` resolves joint-12
  modelpart 0 to `dLinkBoomerangModel_Joint_0x00F8_DisplayList`.

Therefore CopyLink is a mixed-file fighter program: the copy hat and ordinary
body remain Kirby-owned, while one dynamically inserted fighter DObj is owned
by **LinkBoomerangModel asset 326 / file ID `0x146`, root `0xF8`**.  It is not a
Link body graft.

The generated Low-detail root vector now matches the live source tree:

```text
0x115C8, 0x29A0, 0x2A08, 0x00F8, 0x2A90, 0x2AF8, 0x2B80, 0x2C28
```

Its per-root source ownership is:

```text
KirbyHat, Kirby, Kirby, LinkBoomerangModel, Kirby, Kirby, Kirby, Kirby
```

The donor resource is pinned by the generator:

- `decomp/BattleShip-main/BattleShip_o2r/reloc_extern_data/MiscData326`
- file ID: `0x146`
- decoded source bytes: `0x1D0`
- SHA-256:
  `1ba848d69dcdc21fec1b1a00cb5235d7ff40a686ff35dd14ed50a011d5e90ba9`
- one executable root `0xF8`, 1 epoch, 1 run, 6 triangles, 10 dense vertices.

The runtime keeps that donor's compact tables separate from Kirby's tables and
selects table/light ownership per root.  Admission is exact: only Kirby's
mixed-file path may admit asset `0x146`, and only root `0xF8` inside the exact
decoded payload.  Mixed-file draws do not enter the single-file Cycle-99 plan
cache because that cache cannot prove the foreign reloc lifetime.

## Static proof

After regeneration:

```text
python scripts/fighters/generate_nds_native_owners.py --check
PASS

python scripts/fighters/check_native_owner_geometry_closure.py
NATIVE_OWNER_GEOMETRY_CLOSURE_OK
```

The generated CopyLink source-owner row is `1,0,0,2,0,0,0,0` where 0 is
Kirby, 1 is the deferred copy-hat mini image and 2 is LinkBoomerangModel.

The curated commit index was also exported independently of Main's dirty
worktree, regenerated, checked and compiled.  That exact staged source produced
an isolated four-CPU ROM of `28,335,104 B`, SHA-256
`13B672FAD4440D767F0CDBAD832465A251FA5DB59F7A16A779D2B13D0AE540E4`
and ELF SHA-256
`493AF4FB659D94C3AFEE308B4718734CF542E27DC8EC5261553EC61291EC9C27`.
Those hashes are reproducibility/build-health evidence only: natural acceptance
below remains tied to the integrated candidate carrying the current capacity
and roster work.

## Frozen integrated candidate

Four-CPU native-only candidate:

- ROM:
  `builds/build-p2-battle-core/smash64ds-p2-fourcpu-tickhud-hwtri.nds`
- bytes: `30,058,496`
- SHA-256:
  `848E890883154C4DA3B7EA7A102AEEF1EDB3F7BCB0151741658C402D5C76FB56`
- ELF bytes: `15,217,892`
- ELF SHA-256:
  `8D44590362CEF71F07AE9D477C3F39A59B15530C0E597194C84811512F0BE2FD`

## Natural feature proof

Command:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass `
  -File .\scripts\probe-p2-fourcpu-sparse.ps1 `
  -NoBuild -Build build-p2-battle-core `
  -FirstCopyLinkReject -Frame 1536 `
  -Artifact artifacts\verification\p2-3f47-copylink-boomerang-native-1536.txt `
  -TimeoutSeconds 300
```

Final artifact SHA-256:

```text
4D20C923BC7C42299CF6BD8FDF23E320244D9B4FC7585F831F702CEE3B6429DA
```

Terminal witness:

```text
COPYLINKREJECT_NONE_THROUGH=1536
COPYLINKFINAL=program4:8,boomerang:2,0,current:0,tried:0,hat:10,hatDetail:1,texReject:0x0
```

This is positive engagement, not a no-op owner: program 4 was selected eight
times, modelpart-10 copy-hat residency is live in Low detail, and the spawned
boomerang's visible `0x458` source root submitted twice.

The `0x580` boomerang counter is intentionally informational and is zero in
this natural window.  That is **source-correct**, not missing pixels:

- `decomp/BattleShip-main/decomp/src/wp/wplink/wplinkboomerang.c`
  `wpLinkBoomerangSetReturnVars` sets
  `child->child->flags = DOBJ_FLAG_NOTEXTURE` on return.
- `decomp/BattleShip-main/decomp/src/lb/lbcommon.c`
  `lbCommonDrawDObjScaleX` still walks that node's child tree but only submits
  the node's own DL when `DOBJ_FLAG_NOTEXTURE` is clear.
- A bounded live-tree diagnostic observed the `0x580` grandchild with
  `flags=0x1`, exactly `DOBJ_FLAG_NOTEXTURE`.

Requiring a nonzero `0x580` draw count would therefore require pixels the N64
source renderer itself suppresses in the observed return state.

## One widest verifier on the same ROM

`scripts/verify-p2-four-fighter-stress.ps1 -NoBuild -Build build-p2-battle-core`
completed to frame 1,973 / clock 1 on the same frozen ROM.

Fresh verifier hashes:

- `p2-2-fourcpu-tickhud.json` —
  `9BF73680CB48720BF60B9EE95B3891077CCA22E2766F218C3609B930897E3C6E`
- `p2-2-fourcpu-memory.json` —
  `DF3B9303FD160CDF9C359C64E92A41572F805B786A03A058F85A9E0BD9B5E425`
- `p2-2-fourcpu-coverage.json` —
  `A29AA1A8D5F9EB522E202FB0A72DD20C08E8917B017D5A4D8C04F2BC8106F568`

Resource/lifetime verdict:

- general heap free-min `86,700 B`
- safety floor `25,600 B`
- margin `61,100 B`
- graphics heap `1,536 B`, peak `232 B`, overflow `0`, no-room `0`
- texture reject mask `0`
- syMalloc overflow `0`
- objman panic `0`
- native plan verify mismatch `0`
- particle/AObj hard rejects remain zero.

CopyLink is no longer the first native-only failure.  The global first failure
advances to **Link `nFTLinkStatusSpecialN` (`0xE5`)**, LinkModel asset 324,
root `0x5B68`, `REJECTED_PROGRAM`.  That is Link's grounded Boomerang throw and
belongs the existing Link-special package; it is independent of closed Link
Catch/CatchPull and of this Kirby CopyLink closure.

## Periodic production build health

After this package settled, the ordinary production build completed:

- `smash64ds.nds`
- bytes: `53,117,952`
- SHA-256:
  `2CF8F100A4725C30D25566335480930ED1D4776A254D425F2E3D88A9B1E0E1E7`

This is build-health evidence only; it does not replace the native-only
focused/wide acceptance above.

## Acceptance / next action

**CopyLink is feature-scoped CLOSED for the measured natural path.**  Do not
reopen it absent contradictory source or natural runtime evidence.

The next global native-output blocker is Link grounded Neutral-B
(`nFTLinkStatusSpecialN`, status `0xE5`, asset 324 root `0x5B68`) under the
existing Link-special work.  Kirby Stone and other unmeasured copy-power states
remain independently open; generated support is not acceptance.
