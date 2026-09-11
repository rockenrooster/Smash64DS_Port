# DamageSlash native-output closure — 2026-09-11

## Outcome

BattleShip's source-backed `DamageSlash` effect now has native Nintendo DS
output for both `EFCommonEffects1` drawable roots.  The implementation preserves
the source DObj transforms and live MObj/material animation; geometry, palettes,
source image membership and display-list state are generated from the pinned
file-83 O2R.  There is no generic display-list fallback or procedural visual
substitute on this path.

This closes only the `DamageSlash` native-output blocker exposed after P2-2's
Main-RAM recovery.  The same one-minute four-CPU verifier remains RED on an
independent Kirby fighter rejection at model asset 328/root `0x4728`, status
`0x116`; that work remains under the existing P2-3 fighter-output ownership.
P2-2p8 CPU optimization remains owner-deferred.

## Source contract

- Immutable repository baseline for this cycle: `c25bc157fb5147218e66cef4d49d5dd0ef37e3bb`.
- Source asset: `EFCommonEffects1`, reloc asset **83**.
- Source maker: `efManagerDamageSlashMakeEffect` / `dEFManagerDamageSlashEffectDesc`.
- Drawable roots: **`0x75A0`** and **`0x7668`**.
- Each root is one source-derived 4-vertex / 2-triangle textured quad.
- Both lists retain their segment-E material hook.  The live typed material
  supplies the source light colours, primitive colour and current CI4 image.
- The generator pins the DObj root referrers, both palette/vertex relocations,
  all **13** source CI4 image pointers, exact vertices/triangles and source
  palette.  `python scripts/3d_vfx/generate_nds_damage_slash.py --check` passes.

## Bounded DS texture ownership

The first native implementation used the generic dynamic texture cache and
failed late in the natural match when later slash frames needed fresh VRAM.
Preloading all 13 source images also failed the battle's VRAM budget after four
allocations.  The accepted owner therefore keeps exactly **two** PAL16 texture
allocations, one for each simultaneously drawable source child (about **1.5
KiB** total), and updates the same allocation in place as the source MObj image
advances.  This preserves all 13 source frames without allocator growth during
the effect lifetime.

The verifier makes that bound observable: exactly two initial allocations are
required, along with positive in-place updates and direct binds, while any
prepare failure, unknown source image, material rejection, snapshot failure or
submit failure is fatal to the `DamageSlash` assertion.

## Natural focused proof

The exact candidate ROM ran the normal Donkey/Samus/Link/Kirby four-CPU Dream
Land match for 512 presented frames.  Same-run counters at frames 2..513:

- root mask **`0x3`** — both source roots engaged;
- **18** source-root draws, **16** triangle-emitting draws;
- **2** source-alpha-zero frames correctly emitted no DS wireframe geometry;
- live material effects `0x21C`, rejected **0** times;
- **2** texture allocations, **11** in-place source-frame updates, **16** direct binds;
- texture prepare failures **0**, unknown source images **0**;
- snapshot failures **0**, submit failures **0**, submit step **9**.

The first global native failure in that run had already moved away from the
effect to Kirby (`fkind=8`, asset 328, root `0x4728`, reason
`REJECTED_PROGRAM`).

## Widest relevant verifier

Command:

`pwsh -NoProfile -File scripts/verify-p2-four-fighter-stress.ps1 -NoBuild -Build build-p2-battle-core`

Frozen ROM SHA-256:
`3B5DD15548453DAA78D3B3B6188BE9DAD24E7A7CFF63FA52A6734D026A1CEA88`

The same-run coverage spans frame 1 through 1973, clock 60→1 (**59 seconds of
the one-minute match**; timing samples 2..1973, 1,972 samples).  `DamageSlash`
remains clean for that lifetime:

- root mask **`0x3`**;
- **136** source-root draws, **120** triangle-emitting draws;
- **16** source-alpha-zero frames;
- **2** fixed texture allocations, **89** in-place updates, **120** direct binds;
- material rejects **0**;
- texture prepare failures **0**, unknown source images **0**;
- snapshot failures **0**, submit failures **0**, submit step **9**.

Resource safety is unchanged from the capacity recovery: arena 1,396,736 B,
general-heap free minimum **94,076 B**, required floor **25,600 B**, margin
**68,476 B**; graphics heap 232/1,536 B with zero overflow/no-room; no malloc or
object-manager panic.  Cadence was recorded but is not promoted here because
P2-2p8 optimization is owner-deferred.

Raw rotating-verifier artifacts are pinned by hash:

- memory JSON: `E764DE708E29A7C5D14E6AAD6BCE4F1CC4312BD346B0389C20A74385525B52D3`
- coverage JSON: `E9FF320BDDBA78A44845C2D5EE6DBBD77AC52FA4CB5754258DAA44FDA6FBBB79`
- tick-HUD JSON: `F047D200D0C6125E141225705630A00C0462C008BBC054B64A0708EAA893ED3F`
- candidate ELF: `5BEA043E93E0E8911187B92DCA3231C911284CE0BE74BF977175DBF29C2E9B1C`

The exact ELF also passes `scripts/check_native_only_rom.py`: **245 actual link
inputs, NATIVE_ONLY_PASS**.

## Native pixels

`artifacts/visibility/2026-09-11_damage-slash-native.png`

PNG SHA-256:
`1A659B93E5535E80DFCEF5F807AE34C351D04E9820C18BF4724DDF411645AEE5`

The capture uses the same ROM and natural four-CPU match.  It stops on the first
completed battle frame for which the native `DamageSlash` triangle counter is
positive; no guest state is written.  The screenshot shows the active source
slash presentation in the Dream Land fight.  The same stop reports:

`rootmask=0x3 draws=2 tris=1 submitfail=0 prepares=1 updates=0 binds=1 nativefail=0`

Thus the evidence includes actual native pixels at positive owner engagement,
not merely a compiled owner or an unengaged zero-failure counter.  `DamageSlash`
itself does not own a distinct audio cue; collision/hit audio remains on its
unchanged source caller path.
