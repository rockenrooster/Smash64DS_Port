# JP (NALJ) ROM Compatibility — Plan

Status: planning. Author handoff 2026-05-17.

## Goal

Let players run the port against the Japanese release ("Nintendo All-Star!
Dairantou Smash Brothers", game code **NALJ**) instead of US-only (**NALE**).

## The dumped ROM

`baserom.jp.z64` (symlinked/​present at repo root):

- Size: 16 MiB (16777216 bytes — same container size as our US dump)
- Header internal name: `SMASH BROTHERS`
- Country byte @0x3E: `0x4A` = `'J'` → game code `NALJ`, version `0x00`
- SHA1: `4b71f0e01878696733eefa9c80d11c147ecb4984`
- MD5:  `66db457b130d31a286a23d6e4dd9726e`

**Verification: PASS (Phase 0 complete, 2026-05-17).** SHA1
`4b71f0e01878696733eefa9c80d11c147ecb4984` matches **two independent
references**:
- Upstream decomp `origin/main:smashbrothers.jp.yaml` declares
  `sha1: 4b71f0e01878696733eefa9c80d11c147ecb4984` — i.e. this is the exact
  ROM the decomp toolchain is built against (authoritative).
- Public No-Intro / ROM-DB hash for "Nintendo All-Star! Dairantou Smash
  Brothers (Japan)" = same SHA1.

Our dump is the canonical, decomp-targeted JP 1.0 ROM. No further ROM
verification needed.

## Key architectural conclusion: this is a build *variant*, not runtime detection

The decomp game code is region-conditionally compiled. `port-patches` already
carries `#if defined(REGION_JP) / #if defined(REGION_US)` divergence across
~20+ source files (`src/mn/mncommon/mntitle.c` alone has ~10 region splits,
plus `ftcomputer.c`, many `relocData/*.c`, etc.) and `include/piint.h`
remaps `__osEPiRawStartDma`→`osEPiRawStartDma` for the 2.0J SDK.

Consequence: **a single binary cannot play both ROMs.** US and JP differ in
compiled code, file counts, file ordering, and every ROM offset. The
deliverable is the ability to *produce a JP build + JP asset archive*, selected
at configure/build time (`VERSION=jp`), mirroring how the decomp's own
Makefile does `VERSION=us|jp`.

## What already exists (do not rebuild)

- **decomp `port-patches`**: REGION_JP source divergence is already merged.
  `decomp/include/reloc_data.h` is a selector shim that includes
  `reloc_data.us.h` or `reloc_data.jp.h` per `REGION_US`/`REGION_JP`.
  JP-excluded startup files `src/mn/mncommon/{mncongra,mnstartup}.c` are
  present (upstream Makefile filters them out of JP builds).
- **decomp `origin/main`** has the JP source-of-truth tables we need to vendor:
  `tools/relocFileDescriptions.jp.txt`, `symbols/jp_wip.txt`,
  `symbols/jp_wip_linker.txt`, `smashbrothers.jp.yaml`, and JP credits text
  (`src/credits/{staff,titles,info,companies}.credits.jp.txt`).
- **Torch** already detects the country code (`torch/src/n64/Cartridge.cpp`
  parses 'J'/'E'/'P' at ROM 0x3E and `GetCountryCode()` returns "jp"/"us"/"eu")
  and resolves the asset YAML dir from a per-SHA1 `config.yml` entry's `path:`
  key — so JP routing is config, not code.

## What is hardcoded to US and needs a JP equivalent

| Component | File(s) | US value |
|---|---|---|
| Torch ROM→assets map | `config.yml` | single SHA1 → `path: yamls/us` |
| Asset extraction configs | `yamls/us/reloc_*.yml` | per-file US ROM offsets |
| Reloc table ROM address | `tools/generate_yamls.py:24` | `RELOC_TABLE_ROM_ADDR = 0x001AC870`, `RELOC_FILE_COUNT = 2132`, `COMPRESSED_FILE_COUNT = 499` |
| Symbol→fileID table | `tools/reloc_data_symbols.us.txt` | US file IDs/ordering |
| File descriptions | `tools/relocFileDescriptions.us.txt` | US |
| Generated reloc stubs | `include/reloc_data.h` (US, 323 KB) | from `reloc_data_symbols.us.txt` |
| fileID→resource path table | `tools/generate_reloc_table.py:15` → `port/resource/RelocFileTable.cpp` | `YAML_DIR = yamls/us` |
| Region defines | `CMakeLists.txt:247-248` | `REGION_US=1 VERSION_US=1` (unconditional) |
| YAML list / copy / config copy | `CMakeLists.txt:122-136, 464, 478-482` | `yamls/us` literals |
| Baserom selection | `CMakeLists.txt:496-502` | `baserom.us.<ext>` |
| Credits text | CMake credits encode step | `*.credits.us.txt` |

The reloc *symbol→fileID* mapping is logical, not offset-based — but JP has a
different file **count and ordering** (upstream `relocData.py`:
`COMPRESSED_FILE_COUNT = {"us":499, "jp":474}`), so the JP symbol table and
descriptions genuinely differ and must come from upstream `main`, not be
reused from US.

## Scope clarification (user direction, 2026-05-17)

- We do **not** need JP reloc data decompiled. The port consumes reloc files
  as opaque BLOBs that Torch extracts from the **user's own JP ROM at build
  time** — same pattern as US. So upstream `jp_wip` decompilation
  incompleteness does **not** block the port; it only needs (a) the shared
  source compiled under the existing upstream `REGION_JP` flag, and (b) JP
  extraction offsets for Torch.
- No ROM assets are shipped. The "JP compatibility" deliverable is: build with
  `REGION_JP` + point Torch at JP offsets so a user supplying a JP ROM gets a
  working JP archive. Same codebase, one compiler flag.
- Game code is still compile-time region-split (`#if REGION_JP`), so a JP
  *build configuration* is required — but it's a flag flip in the existing
  build, not a separate codebase or shipped binary with assets.

## Plan

### Phase 0 — Verify the ROM & feasibility — ✅ DONE (2026-05-17)
1. ✅ `baserom.jp.z64` SHA1 confirmed against upstream
   `smashbrothers.jp.yaml` (decomp's own target) **and** public No-Intro DB —
   both `4b71f0e01878696733eefa9c80d11c147ecb4984`. Canonical JP 1.0 ROM.
2. ✅ Upstream `REGION_JP` build path confirmed present (Makefile
   `VERSION=jp` → `-DREGION_JP`, jp splat/linker/symbol tables). Per the scope
   clarification, JP reloc *decompilation* completeness is not on the port's
   critical path.

### Phase 1 — Vendor JP source-of-truth — ✅ DONE (2026-05-17)
3. ✅ Vendored into the port `tools/`:
   - `tools/relocFileDescriptions.jp.txt` — 11-line provenance/license
     prefix + blank + upstream `origin/main` verbatim. `FILE_COUNT: 2107`.
     Body verified byte-for-byte against upstream.
   - `tools/reloc_data_symbols.jp.txt` — provenance header + body produced
     by upstream `tools/relocData.py genHeader` (pure text→text, no ROM)
     from the vendored JP descriptions. `llRelocFileCount = 2107`, 2107
     FileID symbols. **Generator parity proven**: regenerating the US table
     this same way reproduces the port's vendored
     `reloc_data_symbols.us.txt` with *zero* diff (4230 == 4230
     assignments), so the JP table is produced by the authoritative path.
   - JP credits text: **no action needed** — `*.credits.jp.txt` already
     present in `decomp/src/credits/` on `port-patches`.
   - `symbols/jp_wip*.txt`: not needed by the port (those are MIPS-linker /
     splat inputs for the decomp's own ROM-matching build; the port uses
     the generated `reloc_data_symbols.jp.txt` instead).
4. ✅ REGION_JP source-divergence audit (`port-patches` @ caae085e1 vs
   `origin/main`): **no divergence lost.** port-patches carries 135
   REGION-divergent files vs upstream main's 98. The 9 files marked
   REGION_* only upstream are all `src/db/*` and `src/libultra/{io,os}/*` —
   intentionally-excluded non-port code (none exist in port-patches at
   all). Spot-checked compiled files have identical REGION-directive counts
   (mntitle 17/17, ftcomputer 6/6, itkamex 3/3). Compiled source set
   retains and exceeds upstream JP coverage.

   **Note for Phase 2:** `tools/generate_reloc_stubs.py:49` hardcodes
   `SYMBOLS_TXT = tools/reloc_data_symbols.us.txt` — must be parameterized
   by version (Phase 2), not changed yet.

### Phase 2 — Parameterize the codegen pipeline — ✅ DONE (2026-05-17)
5. ✅ Added `--version {us,jp}` (default `us`) to all three codegen scripts
   via a per-version `VERSIONS` table. **JP reloc table address found:
   `0x1ACAF0`** (from upstream `smashbrothers.jp.yaml:1848`
   `- [0x1ACAF0, bin, relocData]`; US `0x1AC870` cross-checked == the
   port's existing hardcoded value). File counts: US 2132 / JP 2107.
   Step 0: removed dead `COMPRESSED_FILE_COUNT` from `generate_yamls.py`
   (defined, never used) — keep this its own commit when committing.
6. ✅ Generated JP artifacts (US sibling files, US paths untouched):
   - `include/reloc_data.jp.h` — 6000 #defines (3900 FileID), provenance
     correctly cites `reloc_data_symbols.jp.txt`.
   - `yamls/jp/reloc_*.yml` — 16 files, **2107 entries** (== JP file
     count). JP names resolve 100% from the header so the US heuristic
     name tables never fire for JP (categorisation differs from US but is
     self-consistent with the JP RelocFileTable).
   - `port/resource/RelocFileTable.jp.{cpp,h}` — 2107 entries, **0
     MISSING**, `RELOC_FILE_COUNT 2107`. `RelocFileTable.h` is now
     script-generated (was hand-maintained; its own banner already
     claimed it was) so the count is per-version.

   **Regression guard:** with default (no-arg) invocation —
   `generate_reloc_stubs.py` → `include/reloc_data.h`,
   `generate_reloc_table.py` → `RelocFileTable.{cpp,h}` — all **byte-for-byte
   identical** to the committed files (git sees zero change).
   `generate_yamls.py` US output could not be ROM-verified (US ROM absent
   from this checkout); its US path sets the globals to the exact prior
   hardcoded constants with the read/compute/write logic untouched, and the
   argument-less CMake contract still resolves to `us`.

   **Decisive cross-check:** JP table parse at `0x1ACAF0` reports **474
   VPK0-compressed** files — exactly the upstream-documented
   `COMPRESSED_FILE_COUNT["jp"] = 474`. Address + parsing confirmed.

   **Deferred to Phase 3** (build wiring, not done here): CMake selecting
   the per-version header/table/yaml/ROM; the JP `config.yml` SHA1 entry;
   making the consumers compile `RelocFileTable.jp.*` / include
   `reloc_data.jp.h` for a JP build; CMake codegen step ordering
   (`generate_yamls` currently runs before `generate_reloc_stubs` but
   parses its header output — a pre-existing US wart to handle for JP).

### Phase 3 — Build-system version switch — ✅ DONE (2026-05-18)

**Architectural decision (Senior Dev Override):** the port had a flat
`include/reloc_data.h` on the include path *before* `decomp/include/`,
shadowing the decomp's own US/JP selector shim
(`decomp/include/reloc_data.h` → `reloc_data.<region>.h`). Aligned the
port to the decomp's existing design instead of inventing a parallel one:

- Codegen now emits `include/reloc_data.{us,jp}.h` and
  `port/resource/RelocFileTable.{us,jp}.{cpp,h}`; the flat
  `include/reloc_data.h` and `port/resource/RelocFileTable.cpp` were
  removed. US content is **byte-identical** to the old flat files (only
  renamed). `port/resource/RelocFileTable.h` is now a committed selector
  shim mirroring the decomp's `reloc_data.h` shim. Both versions' headers
  coexist in the source tree → separate build dirs never collide.

7. ✅ `SSB64_VERSION` CMake cache var (default `us`, validated, STRINGS
   us/jp). Drives, all verified in the generated `build.ninja` for both
   versions:
   - `REGION_${UC}=1 / VERSION_${UC}=1` defines
   - YAML category list + `copy_directory` source → `yamls/${ver}`
   - baserom selection → `baserom.${ver}.{z64,n64,v64}` (JP configure
     found `baserom.jp.z64` and took the real extract path; US correctly
     reported "baserom absent" stub)
   - credits encode inputs → `*.credits.${ver}.txt`
   - decomp source filter excludes `mncongra.c`/`mnstartup.c` for JP
     (0 in JP build.ninja, 6 in US)
   - codegen targets pass `--version ${ver}`; BYPRODUCTS → version files
   - port glob filters both `RelocFileTable.{us,jp}.cpp` and appends only
     the selected one (JP graph compiles `.jp.cpp` only; US `.us.cpp` only)
   - asset copy / `ExtractAssets` paths/messages version-driven
   Decided **against** a distinct `output.binary` — the runtime loads a
   fixed `BattleShip.o2r`, so both config.yml entries emit `BattleShip.o2r`
   (one ROM/version per build dir). Per-version coexistence is achieved at
   the source-tree/build-dir level, matching the decomp's `build/<ver>/`.
8. ✅ `config.yml`: added the JP SHA1 entry
   (`4b71f0e0…` → `path: yamls/jp`, name "(J)", `BattleShip.o2r`). Torch
   routes by the user-supplied ROM's SHA1.
   Also fixed `tools/upstream_reconcile.py` AUTO_GEN_FILES to the new
   per-version header names.

**Verification:** `cmake -S . -B … -DSSB64_VERSION={us,jp}` both exit 0
("Configuring/Generating done", build files written); generated Ninja
graph confirms correct per-version defines, source/table selection, and
asset paths. US codegen byte-identical to pre-change (no US regression).
Full *compile* of the JP target is Phase 4 (bring-up).

### Phase 4 — Bring-up & verification — ✅ DONE (JP-specific) (2026-05-18)
9. ✅ `cmake -B /tmp/cfg-jp -DSSB64_VERSION=jp` + JP codegen/credits
   (`PrepareBuildInputs`) succeeded. **`ssb64_game` (the full decomp
   object library, 474 TUs) compiles clean under `REGION_JP`** — exactly
   **one** compile-fallout bug across all first-ever-compiled JP code:
   `gmcolscripts.c` port-only `sGMColScriptLinks[]` referenced the
   US-only `dGMColScriptsFighterPikachuSpecialHi` unguarded → fixed with
   `#if REGION_US` mirroring the decomp's own table
   (`docs/bugs/jp_gmcolscripts_pikachu_specialhi_unguarded_2026-05-18.md`).
   US `gmcolscripts.c` re-verified byte-behaviour-identical.
10. ✅ LP64 sweep: **zero** `long` tokens inside any `REGION_JP` block in
    compiled decomp source; global sweep matches the documented clean
    state; clean JP compile ⇒ every `_Static_assert(sizeof==N)` held
    under LP64+`REGION_JP`. No JP-specific LP64 exposure.
11. ✅ JP asset extraction works end-to-end. Found + fixed a second JP
    bug: Torch's `SSB64:RELOC` factory hardcoded the US reloc-table
    address; now resolves a per-ROM `RelocLayout` from the header country
    byte (`docs/bugs/jp_torch_relocfactory_us_hardcoded_table_2026-05-18.md`,
    torch submodule `ssb64`). `BattleShip.o2r` regenerated from
    `baserom.jp.z64`: **2108 entries** (2107 reloc + version), 0 errors,
    distribution matching `yamls/jp`.
    - ✅ **Full binary links.** Two link-time blockers found + fixed:
      `ftParamGetGroundHazardKnockback` (ftparam.c) and
      `func_ovl0_800C9F70` (lbcommon.c) are `#if REGION_US`-only (no JP
      `#else`, identical upstream) yet referenced unconditionally; the
      decomp's JP ROM build papers over this with `jp_wip` linker stubs.
      Relaxed both definition guards to `#if defined(REGION_US) ||
      defined(PORT)` — byte-matching guards on region-agnostic behaviour,
      US byte-behaviour unaffected (verified by standalone US compile),
      decomp ROM builds untouched
      (`docs/bugs/jp_us_only_funcs_called_unconditionally_2026-05-18.md`).
      Result: **`/tmp/cfg-jp/BattleShip` — 157 MB x86-64 ELF, exit 0,
      0 errors**, full decomp+port+libultraship under `REGION_JP`.
    - **Smoke test (boot/title/CSS/VS/sound):** NOT performed — this
      environment is headless (no display); the GUI binary cannot be run
      here. Left for an interactive run on a JP build. This is the only
      unverified item; nothing JP-specific is known to block it.
12. ✅ Docs: two `docs/bugs/` entries + README; this plan; memory.
    Pending (trivial follow-ups, not JP-blocking): add the JP hash/code to
    `docs/architecture.md` ROM Info and document `-DSSB64_VERSION=jp` in
    `docs/build_and_tooling.md`.

## Outcome (Phases 0–4 complete, 2026-05-18)

JP support builds end-to-end. `cmake -B <dir> -DSSB64_VERSION=jp`,
extract from the user's `baserom.jp.z64`, build → a 157 MB JP
`BattleShip` ELF. Total JP-specific compile/link fallout across
*never-before-built* `REGION_JP` code was **three small bugs**, all
fixed and documented under `docs/bugs/` (gmcolscripts unguarded entry;
Torch RelocFactory US-hardcoded table; two US-only funcs referenced
unconditionally). US is byte-identical/behaviour-identical throughout.

The Phase-0 "`jp_wip` incompleteness" risk did **not** materialise for
the port: the port consumes reloc files as opaque Torch-extracted BLOBs,
so upstream JP *decompilation* completeness was irrelevant — the JP
divergence in compiled source is solid (135 files, only the 3 issues
above). LP64: zero `long` exposure in `REGION_JP` code.

### Post-bring-up follow-ups (2026-05-18)

- ✅ **Dev-build o2r isolation.** Dev extraction wrote `BattleShip.o2r`
  to the shared *source root*, so US and JP builds (separate build dirs)
  clobbered each other. Changed the extraction `-d` to a per-build sink
  `${CMAKE_BINARY_DIR}/extracted/` (then staged next to the binary).
  config.yml `output.binary` stays `BattleShip.o2r` for both; runtime
  name fixed; packaged-release first-run extraction unaffected. Verified:
  JP extract lands in the build dir, source-root o2r untouched.
- ✅ **JP audio/particles — RESOLVED.** Added hand-derived
  `yamls/jp/audio.yml` (8 segments) + `yamls/jp/particles.yml` (18
  segments), transformed from the US files with JP ROM-segment addresses
  taken from `decomp origin/main:smashbrothers.jp.yaml`'s splat segment
  list (sizes computed from consecutive segment starts, same method as
  US; region-agnostic format/byte-swap docs preserved). Sanity:
  segments whose data is identical across regions reproduce the US size
  exactly (`S1_music_sbk` 0x26E10, `B1_sounds1_ctl` 0x6720, `fgm_unk`
  0x820); JP-divergent ones (B1_sounds2*, fgm.tbl/ucd) differ per JP
  splat boundaries. Re-extracted: JP `BattleShip.o2r` now has **2134
  entries = 2107 reloc + 8 audio + 18 particles + version**, full
  category parity with US, Torch exit 0. This was the last blocker to a
  runnable JP build; the `syAudioMakeBGMPlayers` crash cause (missing
  audio banks) is gone. (Visual boot smoke-test still pending — headless
  env.)
  Dev note: `ExtractAssets` doesn't list the `yamls/<v>/` files as
  dependencies, so editing a yaml doesn't auto-retrigger extraction —
  `rm <build>/extracted/BattleShip.o2r` (or touch a dep) to force it.

**Remaining (not blocking, not JP-specific engine work):**
- Interactive smoke test (boot/title/CSS/VS/sound/credits) on a JP
  build — could not run here (headless). Only unverified item.
- Commit/submodule bookkeeping: the Torch fix lives in the `torch`
  submodule (`ssb64` branch) — bump the pointer when landing
  (CLAUDE.md submodule workflow). decomp source edits (gmcolscripts,
  ftparam, lbcommon) ride the decomp submodule.

## Resolved direction (from the user)

- Distribution: existing pattern kept — user supplies their own ROM,
  extracted at build time; no assets shipped. JP is a `-DSSB64_VERSION=jp`
  build (compile-time region split is unavoidable; one ROM per build dir).
- Quality bar: "boots and is playable, known gaps documented." The two
  US-only funcs now use the US formula/helper in JP — behaviourally
  faithful; pixel-exact JP variants would need upstream `jp_wip` decomp
  (documented as a known approximation).
