# Shared plan 01 — Admit Remix stages into the native stage pipeline

**Status:** plan; the census inputs are verified (P5_STATUS, P5.0), the
exporter and the generator's donor mode are not built. Parent:
[master](../New_Stages.md).

## Decision: one more input adapter, the same output

The native stage generator (`scripts/stages/generate_nds_native_stage.py`)
reads pinned BattleShip O2R banks plus typed relocData C and reloc symbols:
each descriptor in `scripts/stages/native_stage_descriptors/` names its text
inputs and contract tokens, and the map header's offset comes from
`reloc_data.us.h`. Remix stages have no typed C. Add a donor input
(`stage.json` plus O2R containers written from the staged build) and keep
everything downstream: the packet format, blob residency, compact collision
maps, native wallpapers, the stage GX compiler and the runtime stage owner.
Do not patch read-only BattleShip, and do not write a stage interpreter.

Meta Crystal, Duel Zone and Final Destination do not use this adapter.
Their Remix files are byte-identical to vanilla, and their BattleShip
descriptors (`metal`, `zako`, `last`) already pin those bytes.

## 1. Inputs are the linked result

- The staged donor build from P4.0: `builds/p4-staging/remix-5e04fe7`
  (`ssb64asm.z64`, made by `scripts/p4/stage_remix.py` at the P4 source
  lock). The export refuses a staging whose recorded HEAD differs from
  `docs/P4/source-lock.json`.
- `scripts/p4/remix_rom.py` reads the grown file table (5,455 files in the
  staged ROM against 2,132 vanilla), VPK0 bodies, relocation chains and
  extern ids cut to the chain length, and writes O2R containers that the P2
  generators already read.
- Remix's stage tables live in `src/Stages.asm` (`add_stage` rows,
  `stage_file_table`, `function_table`, `zoom_table`, `position_table`,
  `background_table`, `icon_offset_table`, `class_table`). Read them from the
  source text, then confirm each value against the assembled ROM through
  bass's `-sym` log, as `scripts/p4/p4_css.py` reads `CharacterSelect.asm`.
  Where the two disagree the export stops.
- Every exported object carries `(donor revision, file id, offset, type)`.

## 2. What the export resolves

`scripts/p5/stage_export.py --staging <staging> --stage <REMIX_NAME> --out
builds/<build>/p5/stages/<name>/` writes `o2r/<id>` for the map, geometry
and wallpaper files and a `stage.json` with:

| Group | Fields |
|---|---|
| Header | file id; offset = Remix `type` (0x14 for every P5 Remix stage; Final Destination's 0x00 stays on its BattleShip descriptor); the MPGroundData fields: `map_geometry`, `layer_mask`, `wallpaper`, fog colour and alpha, emblem colours, light angle, camera and map bounds (normal and team), `bgm_id`, `map_nodes`, `item_weights`, `alt_warning`, zoom start and end |
| Layers 0-3 | DObjDesc array (file, offset, entries to the id-18 sentinel); secondary flag from `layer_mask`; for a secondary layer, each DObj's DObjDLLink array (list ids 0-3, ended by 4); AnimJoint, MObjSub and MatAnim table pointers |
| Materials | each DObj's MObjSubs in branch order (segment 8*i for MObj i, the P2 rule) and their MatAnim scripts |
| Collision | yakumono count, vertex positions and flags, vertex ids and links, line info per group, map objects by kind |
| Wallpaper | sprite file and offset, size (all four are 300x220) |
| Remix rows | `add_stage` (display name, default song, class, series logo, hazard type), the `function_table` entry, zoom, position, background parent, icon offset |
| Identity | file sha256 and payload sha256, export schema version, staging manifest hash |

The song is the `add_stage` default, not the header's `bgm_id`: Hyrule
Temple's header still names Hyrule Castle's track (9) while Remix plays
`HYRULE_TEMPLE` through its default-music table.

## 3. The generator's donor mode

- A generated descriptor replaces the hand-written one: owner `i` for each
  present layer, link 4/6/13/17, the primary or secondary display callback
  from `layer_mask`, `dl_links` for secondary layers, the asset order, the
  material sources from the export, and the adapter asset ids and sizes from
  the files.
- Export checks replace the text contracts: file ids, payload hashes, the
  header offset and the layer roots. The light-angle bake reads the header
  at the exported offset instead of a reloc symbol.
- `expected_counts` are written on first generation to
  `scripts/p5/pins/<name>.json` and compared on every later run. Drift is a
  falsifier to review, as for the P2 stages.
- The generator's fail-closed segment-E walk still decides material events;
  an unresolved branch stops the stage.
- Collision: `generate_compact_ground_maps.py` and `check_collision_parity.py`
  take the donor map container; parity compares the generated maps with the
  export's collision walk.
- Wallpaper: `generate_native_wallpapers.py` takes the donor sprite.
- Animated layers (Dinosaur Land) use the AnimJoint and MatAnim paths the
  vanilla stages already use; no new runtime.
- Packets register as NitroFS blobs (`register_native_stage_runtime.py`,
  `emit_native_stage_runtime_rows.py`), never linked.

## 4. Music inputs (for T7)

- Sequence: the Remix song from the staged ROM's music table, checked
  against `src/music/<name>.bin`.
- Bank: the staged ROM's music instrument bank. Remix appends instruments
  after the vanilla 42 (`midi.asm`: `instrument_count` starts at 0x2A;
  `add_instrument_sample` and `add_instrument` rows; samples in
  `src/music/instruments/*.aifc`). Reading the assembled bank, not the
  `.aifc` files, follows the linked-result rule.
- Per-song overrides from `midi.asm` (master volume, bend range, priority).
- The census of programs per song is in P5_STATUS. The renderer must fail on
  a program outside the bank.

## 5. Negative controls

Each must stop the export or generator with its first cause, not skip
content:

- staging HEAD different from the source lock;
- a header offset whose layer pointers are not relocation slots;
- a DObjDesc array without its sentinel inside the file;
- a display-list command outside the supported F3DEX2 set;
- a relocation chain that leaves the file, or an extern id outside the table;
- a registry stage with no `add_stage` row, or a Remix table value that
  differs from the assembled bytes;
- a song program outside the bank.

## Deliverables and exit

`scripts/p5/stage_export.py`, the `stages.json` schema, the pins, the
generator's donor mode, donor inputs for the collision and wallpaper tools,
and `scripts/p5/test_stage_export.py` with the negative controls.

First Destination exits admission when it exports and generates twice
bit-identically, passes host collision parity and every negative control,
and draws natively in the lab ROM. The other three then export through the
same path; any new branch they need lands in the generator, not in a stage
file.
