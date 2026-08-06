# Samus Charge Shot Hit Detection — Investigation Handoff (2026-04-13)

## The bug

In the DK-vs-Samus Kongo Jungle intro (**scene 45, `nSCKindOpeningJungle`**), Samus fires her charged plasma shot but the projectile passes **through** DK with no collision. The opposite direction works: **DK's melee attacks (Giant Punch, etc.) hit Samus normally** and deal damage.

This is the critical asymmetry: same fighters, same scene, same code binary — fighter-vs-fighter melee collisions work, but projectile-vs-fighter collisions fail.

## Why the asymmetry matters

The two code paths diverge inside `ftMainSearchHitAll` → separate "search hit fighter" and "search hit weapon" subroutines. Both eventually call `gmCollisionTestRectangle`, but they reach it via different collider structs:

| Path | Attacker collider | Victim collider | Drives the loop via |
|---|---|---|---|
| DK → Samus (works) | `FTAttackColl` on DK's `FTStruct` | `FTDamageColl` on Samus | `ft_attack_coll->attack_state` + per-frame attack-collider setup |
| Samus charge shot → DK (broken) | `WPAttackColl` on the weapon `WPStruct` | `FTDamageColl` on DK | `wp_attack_coll->attack_count` from the file-backed `WPAttributes` |

Because melee works, we know the following are fine (do **not** re-investigate):
- DK's `FTDamageColl damage_colls[11]` is correctly populated with non-zero sizes, offsets, joints.
- DK's `FTAttributes.hit_detect_range` is correctly loaded (melee uses `gmCollisionCheckFighterAttackInFighterRange` against this same field).
- `gmCollisionTestRectangle` and the AABB math work.
- `ftMainProcSearchHitAll` fires every frame for both fighters (pkind=Key fighters still get the real process list at `src/ft/ftmanager.c:895`).
- Fighter-vs-fighter hit flag gating (team/owner/attack_records) is correct.
- The 6 partial-bit `FTMotionEvent*` structs fixed in commit `a43484d` — those drive the fighter attack colliders and clearly work now.

The bug is isolated to the **weapon path**: something causes `wp->attack_coll.attack_count` to be `0` when it should be `>= 1`.

## How attack_count gets to 0

Established via runtime diagnostic logging in `ftMainSearchHitWeapon` (added in this session, still present):

```
SSB64: HIT fighter=fkind2 wp=ChargeShot state=1 mask=0x7 cnt=0 owner_same=0 size=120.0 pos[0]=(0.0,0.0,0.0)
SSB64: HIT range_check fkind=2 in_range=0/0 fpos=(-551.7,-240.8,0.0) range=(1200.0,600.0,1200.0) wp_size=120.0 state=1 special_ht=1 star_ht=1 ht=1 reflect=0 absorb=0 shield=0
```

`cnt=0` → `in_range=0/0` → the entire `for (i = 0; i < wp_attack_coll->attack_count; i++)` loop at `src/ft/ftmain.c:3434` runs zero iterations → the damage-collider check never executes → the projectile sails through.

The only place `wp->attack_coll.attack_count` is ever written is `src/wp/wpmanager.c:250`:

```c
wp->attack_coll.attack_count = attr->attack_count;
```

Grep confirms no other writes to `wp->attack_coll.attack_count` anywhere in `src/wp/`, `src/ft/`, `src/it/`, or `src/gm/`. And no runtime weapon code (including `wpSamusChargeShotLaunch`) touches it. So the entire story is: **what does `attr->attack_count` read from the file-backed `WPAttributes` at `wpmanager.c:116`?**

```c
attr = lbRelocGetFileData(WPAttributes*, *wp_desc->p_weapon, wp_desc->o_attributes);
#ifdef PORT
    portFixupStructU16(attr, 0x10, 6); // Rotate16 u16 fields: Vec3h[2] + s16[4] + u16 + pad
#endif
```

For Samus Charge Shot, `*wp_desc->p_weapon = gFTDataSamusSpecial1` (set at file load time from `llSamusSpecial1FileID = 0xda`, decompressed size `0x40` = 64 bytes). `wp_desc->o_attributes = llSamusSpecial1ChargeShotWeaponAttributes` which in the port's generated `reloc_data.h` is `((intptr_t)0x0)` — so `attr` points to **byte 0** of the loaded file.

## What's actually in the ROM at that position

The diagnostic dump I added in `wpManagerMakeWeapon` prints the raw 52 bytes of the struct right after `portFixupStructU16` runs. For the two weapons that actually spawn in the 4000-frame intro run:

### Fox Blaster (`kind=1`, from Fox intro, scene ~30)

```
raw [00]=14060000 [04]=00000000 [08]=00000000 [0C]=00000000
raw [10]=00000000 [14]=00000000 [18]=00000000 [1C]=0a000000
raw [20]=f6ff0a00 [24]=28008002 [28]=01800119 [2C]=3c114001 [30]=00000000
```

Post-pass1 + post-`portFixupStructU16` LE-host u32 values:
- bitfield word 1 (memcpy from offset `0x28`) = `0x19018001`
- bitfield word 2 (offset `0x2C`) = `0x0140113c`
- bitfield word 3 (offset `0x30`) = `0x00000000`

### Samus Charge Shot (`kind=2`, from scene 45)

```
raw [00]=070a0000 [04]=00000000 [08]=00000000 [0C]=00000000
raw [10]=00000000 [14]=00000000 [18]=00000000 [1C]=00000000
raw [20]=00000000 [24]=0000405a [28]=00080019 [2C]=3c016001 [30]=00000000
```

- bitfield word 1 = `0x19000800`
- bitfield word 2 = `0x0160013c`
- bitfield word 3 = `0x00000000`

Word 3 is **all zeros** for both weapons. Word 2 differs in a handful of bits, most notably around byte offset `0x2D`: Fox has `0x11`, Charge Shot has `0x01`. That is the byte whose bit 4 (u32 bit 12) encodes `attack_count[0]` in the LE layout — and that's the bit that decides cnt=1 vs cnt=0.

### The 2-byte "padding" at `0x26..0x27` is non-zero

For both weapons the 2 bytes between `u16 size` at `0x24..0x26` and what should be bitfield word 1 at `0x28` are *not* zero:
- Fox Blaster: `[0x02, 0x80]` (appears as `8002` in `[24]=28008002` because `portFixupStructU16` swaps the upper/lower halfwords of that u32)
- Charge Shot: `[0x40, 0x5a]` (appears as `405a` in `[24]=0000405a`)

This is significant. In a padded SGI layout those bytes would be compiler-inserted zero padding. Non-zero values here means either (a) SGI left garbage in the padding, (b) SGI *didn't* pad and those bytes are actually the start of a packed bitfield word 1, or (c) there's a different field there that neither the port nor the decomp declaration knows about.

## Clang's actual bitfield layout (from `clang -cc1 -fdump-record-layouts-simple`)

### Before my fix (packed)

```
Size: 416  DataSize: 416  Alignment: 32
FieldOffsets: [..., 288, 304, 308, 320, 330, 340, 341, 351, 352, 354, 362, 372, ...]
```

Map to field list:
- `u16 size` at bit `288` = byte `0x24` ✓
- `u32 element : 4` at bit `304` = byte `0x26` ← **clang placed `element` immediately after `size`, no 2-byte padding**
- `u32 damage : 8` at bit `308`
- (4-bit gap 316..319)
- `u32 knockback_scale : 10` at bit `320` = byte `0x28`
- `u32 angle : 10` at bit `330`
- `u32 : 1` at bit `340`
- `u32 sfx : 10` at bit `341`
- `u32 can_setoff : 1` at bit `351`
- `u32 attack_count : 2` at bit `352` = byte `0x2C` bit 0

So in the pre-fix state, clang was reading `element` at 0x26 and `attack_count` at bits 0..1 of the u32 at offset `0x2C`. Struct still `sizeof == 0x34` because clang added trailing padding to round to 4-byte alignment.

### After my (now-reverted) `u16 port_pad_word1` fix

```
FieldOffsets: [..., 288, 304, 320, 324, 332, 342, 352, 353, 363, 364, 366, 374, ...]
```

- `u16 size` at bit `288` = byte `0x24`
- `u16 port_pad_word1` at bit `304` = byte `0x26`
- `u32 element : 4` at bit `320` = byte `0x28` ← shifted to u32 boundary
- `u32 damage : 8` at bit `324`
- (8-bit gap 332..339)
- `u32 knockback_scale : 10` at bit `332`
- `u32 angle : 10` at bit `342`
- `u32 : 1` at bit `352`
- `u32 sfx : 10` at bit `353`
- `u32 can_setoff : 1` at bit `363`
- `u32 attack_count : 2` at bit `364` = byte `0x2D` bit 4

Struct still 52 bytes because the padding replaces existing trailing slack. **Note the "8-bit gap" between damage and knockback_scale — clang is byte-aligning 10-bit bitfields even in the padded version**, which is a behavior worth double-checking.

## The probe confirms clang's layout is internally consistent

To decouple "struct layout" questions from "ROM bit layout" questions I added a stack-local `WPAttributes probe`, memset to zero, then wrote known values to each named field and dumped the raw bytes. After the padding fix, with writes `el=1, dmg=40, kbs=1, ang=96, cnt=1, can_set=1, sfx=200, pri=1, kbb=40`, the probe produced:

```
pbf1=0x18001281 pbf2=0x00001990 pbf3=0x20014000
```

Manual decode of `pbf1 = 0x18001281` with LE LSB-first layout (element at bits 0..3, damage at 4..11, kbs at 12..21, angle at 22..31) gives **exactly** el=1, dmg=40, kbs=1, ang=96. Same for pbf2 (sfx=200 at bits 1..10 gives bits 4, 7, 8 set; can_setoff at bit 11; cnt=1 at bit 12 → `0x1990` ✓) and pbf3 (kbb=40 at bits 11..20 gives u32 bits 14, 16 set; pri=1 at bit 29 → `0x20014000` ✓).

**Clang's post-fix layout reads and writes the fields where the LE/BE mirror declaration expects them to be.** If the ROM bytes at those same positions held the "correct" values, we'd get them back.

## Post-fix Fox Blaster vs Charge Shot decoded values

Applying the same decoding to the actual ROM reads:

| Field | Fox Blaster | Charge Shot |
|---|---|---|
| element | 1 | 0 |
| damage | 0 | 128 |
| knockback_scale | 24 | 0 |
| angle | 100 | 100 |
| sfx | 158 | 158 |
| can_setoff | 0 | 0 |
| **attack_count** | **1** | **0** |
| shield_damage | 0 | 128 |
| knockback_weight | 5 | 5 |
| knockback_base | 0 | 0 |
| priority | 0 | 0 |

Fox Blaster's `cnt=1` is the evidence that the padding fix "worked" for at least one weapon. Charge Shot stubbornly stays at `cnt=0`. Note that `angle=100`, `sfx=158`, and `knockback_weight=5` are **identical** between the two weapons, which is plausible (they share template defaults) but also suspicious-looking. `damage=0` for Fox Blaster is not plausible — Fox Blaster does 3% damage on the real N64.

## The file-size clue

From `yamls/us/reloc_fighters_main.yml`:
```yaml
FoxSpecial1:    file_id: 210    size: 0x2E (compressed)   # decompressed_size: 0x40
SamusSpecial1:  file_id: 218    size: 0x26 (compressed)   # decompressed_size: 0x40
```

Both files are **64 bytes** decompressed. `sizeof(WPAttributes)` is **52 bytes**. That's 12 extra bytes per file. We don't know what's in those 12 bytes yet — maybe a DL, maybe a small DObjDesc referenced by the `data` field token — but we know WPAttributes occupies at most the first 52 bytes.

## What doesn't add up

1. **If SGI BE also packed `element` at offset 0x26 (no padding), my fix is wrong** — I'm forcing clang to 0x28 while the ROM data was laid out starting at 0x26. Decoded values would be shifted by 2 bytes.
2. **If SGI BE padded to 0x28, my fix should work** — and it does for Fox Blaster, but Charge Shot still shows `attack_count = 0` with this layout.
3. **The item-side code (`src/it/ittypes.h:243`) added an unconditional `u16 _pad_before_combat_bits;`** — the companion fix for `ITAttributes`. Presumably items on the port work (reel, MSBomb, etc.), which suggests BE-SGI did add the padding and my WPAttributes fix *should* be analogous.
4. **Charge Shot `attack_count = 0` is unsolvable with bit-layout reasoning alone** — decoded with both BE MSB-first and LE LSB-first on both packed and padded offsets, I got `cnt=0` under every interpretation except one outlier (bits 30..31 of the raw u32 — which wouldn't make sense as the field position).
5. **`wpSamusChargeShotLaunch` does NOT set `attack_count`** — it only overrides damage, size, fgm, priority, and map-coll dims. So the 0 is not getting rewritten at firing time.
6. **DK's melee hits Samus, proving the rest of the collision pipeline works.** This narrows the bug to "something specific about how the weapon's `attack_count` is populated from the loaded `WPAttributes`", not a general collision-system problem.

## Unexplored leads for next session

These are the most promising angles I didn't get to:

1. **Dump the raw Samus Special 1 file from the .o2r archive** *before* pass1 byte-swap and `portFixupStructU16`. Compare to what the port has in memory after both. This decides packed-vs-padded ROM layout unambiguously. The bridge code at `port/bridge/lbreloc_bridge.cpp:302` (`lbRelocLoadAndRelocFile`) is where the raw bytes enter the port, pre-pass1.
2. **Is there a MISSING u32 field at the start of the file?** The 64-byte file vs 52-byte struct leaves 12 bytes unaccounted for. If those 12 bytes are at the *start* (header), then `WPAttributes` is actually at file offset 12, not 0, and everything I read is shifted by 12 bytes. The `ll*WeaponAttributes` symbol resolving to `((intptr_t)0x0)` doesn't prove the offset is really 0 — the port's generated reloc stubs set every such symbol to 0 regardless. Check how the ORIGINAL decomp (pre-port) compiles these symbols.
3. **Verify `gFTDataSamusSpecial1` is actually set to the decompressed Samus-Special-1 file content**, not some other file. The port's RelocFile factory is keyed by file_id; something in the load chain could be wiring the wrong file. Grep `gFTDataSamusSpecial1` — I found only declarations, never an assignment, which suggests a linker-generated symbol table sets it. Check `src/ft/ftmanager.c:ftManagerLoadFiles` or equivalent.
4. **Build a quick standalone C program** that just includes `wptypes.h`, declares a `WPAttributes` on the stack, writes Fox-Blaster-plausible values through the named fields (`damage=3, angle=0`, etc.), and dumps the bytes. Then compare to the ROM bytes — if they differ, the struct declaration doesn't match the ROM format.
5. **Check the upstream `Killian-C/ssb-decomp-re` source** for `wptypes.h` / `WPAttributes`. If the pre-port decomp doesn't match the ROM on matching builds, that's a pre-existing decomp bug that the port inherited. If it *does* match, SGI IDO 7.1 lays out bitfields differently from clang-LE and the fix needs to be offset-based (not `#ifdef PORT` struct padding). The likely candidate is that SGI uses a **different storage-unit allocation rule** for bitfields than clang — specifically, that SGI might pack bitfields into the *current* allocation unit (including the `u16 size` unit) whereas clang starts a new u32 unit.
6. **Why does Fox Blaster have `damage=0`?** If Fox Blaster's ROM damage field is really 0, it means either (a) the real N64 also reads 0 from this field and the laser damage comes from a completely different source we haven't identified, or (b) my layout is still wrong for Fox Blaster too and I'm reading the wrong bits but happened to get `attack_count=1` by accident. This is an independent consistency check: if (a) is true, we should find where Fox Blaster damage actually comes from; if (b) is true, the whole layout needs to be re-derived.
7. **Check whether `portFixupStructU16(attr, 0x10, 6)` is over- or under-reaching for WPAttributes.** The call rotates 6 u32 words covering `0x10..0x28`. If the ROM has bitfield word 1 starting at `0x26` (packed), then the 6th rotated word (`0x24..0x28`) is half u16 `size` + half bitfield — the rotate would garble the bitfield's upper 2 bytes. Consider: maybe the item-side `u16 _pad_before_combat_bits` is the decomp's way of declaring that the struct really is padded *in the source*, and the ROM file was generated by re-compiling the padded struct with SGI. In that case the port's job is to match the padded layout, which is what my fix does — so Charge Shot's `cnt=0` must be a *different* bug, not a layout bug.
8. **Does the item-side fix actually work for items that fire projectiles?** If anyone can verify in-game that e.g. a Beam Sword throw or a dropped Party Ball actually damages a fighter in the port, that confirms the `u16 _pad_before_combat_bits` approach is at least correct for the same class of struct. If items also don't hit, the padding approach is not the answer.

## Files touched this session (still present on disk)

**Permanent changes:** None right now — my `u16 port_pad_word1;` fix in `src/wp/wptypes.h` was reverted before writing this doc to leave the tree in a clean state.

**Diagnostic scaffolding still in place (remove before committing any real fix):**

- `src/ft/ftmain.c` — new `portTraceHitEnabled()` static gated on `SSB64_TRACE_HIT=1`, plus three `port_log` diagnostic lines inside `ftMainSearchHitWeapon` (around the weapon-loop entry, the range-check loop, and the damage-collider inner loop). Also added `extern char *getenv(...);` near the top of the file.
- `src/wp/wpmanager.c` — a large `#ifdef PORT` diagnostic block right after `wp->attack_coll.interact_mask = GMHITCOLLISION_FLAG_ALL;` (around line 254) that:
  - Constructs a stack-local `WPAttributes probe`, writes known values via named fields, then memcpy-reads the three bitfield words. Logs as `SSB64: PROBE`.
  - Dumps the full 52-byte raw blob of the real `attr` in three `SSB64: WP make raw` lines.
  - Dumps the compiler-extracted field values in one `SSB64: WP make extracted` line.
  - Pulls `getenv` via `extern char *getenv(const char *);`, which was essential — without the prototype, ARM64 clang silently generated a non-variadic call shape that crashed with `EXC_BAD_ACCESS` inside `getenv`.
- Everything is gated on `SSB64_TRACE_HIT=1`, so it's free when the env var is unset.

## Important finding: ARM64 getenv class of bug

This session found and worked around a new variant of the existing "port_log ARM64 needs visible prototype" rule (see `memory/feedback_portlog_arm64.md`). The same rule applies to `getenv`: without `extern char *getenv(const char *name);` visible to the call site, ARM64 clang emits a non-standard calling convention and the returned pointer is garbage. Manifested as a SIGSEGV inside `portTraceHitEnabled()` at `ftmain.c:3303`. **Any future decomp `.c` file that calls `getenv` needs the extern in the same `#ifdef PORT` block as the `port_log` extern.** Worth rolling into the memory note.

## Test data captured

- 4000-frame intro run with the padding fix reached scene 45 (`scManagerRunLoop — entering scene 45` at frame 2880, lasted until frame 3240). During that window the Samus Charge Shot spawned, the new `HIT` diagnostics fired ~200 times, and every one showed `cnt=0 in_range=0/0` — i.e. the hit loop never even started.
- No successful hit was recorded for Charge Shot (`result=1` never appears in the log).
- Fox Blaster is not tested against a target in any intro scene, so we can't visually confirm its post-fix behavior. Its `cnt=1` is only established analytically.

## Key code references

- `src/ft/ftmain.c:3296` — `ftMainSearchHitWeapon` (the weapon hit loop that never iterates)
- `src/ft/ftmain.c:3434` — the `for (i = 0; i < wp_attack_coll->attack_count; i++)` at the heart of the bug
- `src/wp/wpmanager.c:116` — `lbRelocGetFileData` load and `portFixupStructU16` call
- `src/wp/wpmanager.c:250` — `wp->attack_coll.attack_count = attr->attack_count;` (only write)
- `src/wp/wpsamus/wpsamuschargeshot.c:152` — `wpSamusChargeShotLaunch` (doesn't touch `attack_count`)
- `src/wp/wptypes.h:36-120` — `WPAttributes` struct with the `#if IS_BIG_ENDIAN` bitfield mirror
- `src/it/ittypes.h:242-243` — the working companion `u16 _pad_before_combat_bits;` for items
- `port/bridge/lbreloc_byteswap.cpp:757` — `portFixupStructU16` implementation (idempotent via `sStructU16Fixups`)
- `port/bridge/lbreloc_bridge.cpp:302` — `lbRelocLoadAndRelocFile` where raw file bytes enter the port

## The single most useful next step

Add a dump of the **raw pre-pass1 bytes** of Samus Special 1 file to `lbRelocLoadAndRelocFile`, gated on an env var. The port passes the raw bytes through `memcpy` before any byte-swap, so capturing them at that moment gives ground truth for what's actually on disk. Compare the dumped bytes to (a) what clang's `WPAttributes` expects with and without padding, (b) what the decomp devs wrote in the BE source struct, and (c) the matching Fox Blaster file. The three-way comparison will definitively identify which layout the ROM uses and whether `llSamusSpecial1ChargeShotWeaponAttributes` really maps to offset 0.
