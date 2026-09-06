# Sources, scope, and compatibility

Prepared/reviewed 2026-09-06. The pack's implementation advice and original
examples synthesize these contracts; they are not copied SDK implementations or
claims that every game uses the same microcode, layout, or behavior.

## Companion reviewed

The supplied `nds-coding-practices-final-2026-09-06(1).zip` was extracted and its
entrypoint, porting chapter, fast-implementation recipes, source/version map, and
example/metadata structure inspected while defining this companion's boundary.

Input archive SHA-256:

```text
7306dbd9027ce5dda4473544b481da2058faea647f2aabfd12db203e7d2125a4
```

That existing skill remains the authority for its DS API/hardware material. It is
not bundled or modified by this new pack. Its broad porting chapter can stay;
this companion adds the deeper N64-specific translation layer.

## Source priority

For a real port, use the consuming game's source revision, matching GBI headers,
actual microcode, binary schemas, and intended behavior. Archived Nintendo/SGI
manuals document standard interfaces; they can include version-specific limits,
legacy text, and differences between manual revisions. In particular, do not
copy a vertex-cache capacity from one manual page into every microcode decoder.

For target APIs, use installed headers and implementation at the project's
version. The explicit libnds baseline below matches the supplied companion's
reviewed baseline; it is not a dependency upgrade instruction or a claim about
upstream HEAD. This pack avoids reproducing Calico/legacy/BlocksDS API guidance.

## N64 primary documentation (archived originals)

| Contract used | Primary document |
|---|---|
| Vertex loading, coordinate/attribute interpretation, matrix-dependent load behavior | [gSPVertex](https://ultra64.ca/files/documentation/online-manuals/functions_reference_manual_2.0i/gsp/gSPVertex.html) |
| Triangle-time rendering state and flat-shaded face-vertex selection | [gSP1Triangle](https://ultra64.ca/files/documentation/online-manuals/man/n64man/gsp/gSP1Triangle.html) |
| Split matrix storage and source matrix operations | [gSPMatrix](https://ultra64.ca/files/documentation/online-manuals/functions_reference_manual_2.0i/gsp/gSPMatrix.html) |
| Color/ST/screen changes to an already-loaded vertex | [gSPModifyVertex](https://ultra64.ca/files/documentation/online-manuals/functions_reference_manual_2.0i/gsp/gSPModifyVertex.html) |
| Lighting changes affect future loaded vertices | [gSPLight](https://ultra64.ca/files/documentation/online-manuals/functions_reference_manual_2.0i/gsp/gSPLight.html) |
| Segment registers and reference resolution | [gSPSegment](https://ultra64.ca/files/documentation/online-manuals/functions_reference_manual_2.0i/gsp/gSPSegment.html) |
| CPU versus RSP address domains and documented segment-bit masking | [Programming Manual 3.6 — Memory Maps](https://ultra64.ca/files/documentation/online-manuals/man/pro-man/pro03/03-06.html) |
| TMEM and tile descriptors | [Programming Manual 12.4 — Texture Memory](https://ultra64.ca/files/documentation/online-manuals/man/pro-man/pro12/12-04.html) |
| Load/tile shift, mask, mirror, and clamp parameters | [gDPLoadTexture](https://ultra64.ca/files/documentation/online-manuals/functions_reference_manual_2.0i/gdp/gDPLoadTexture.html) |
| Color-combiner inputs and cycle-dependent combination | [gDPSetCombineMode](https://ultra64.ca/files/documentation/online-manuals/man/n64man/gdp/gDPSetCombineMode.html) |
| Blender/render/depth/coverage modes | [gDPSetRenderMode](https://ultra64.ca/files/documentation/online-manuals/man-v5-1/n64man/gdp/gDPSetRenderMode.htm) |
| Source point/filter modes and three-texel approximation note | [gDPSetTextureFilter](https://ultra64.ca/files/documentation/online-manuals/man-v5-2/allman52/n64man/gdp/gDPSetTextureFilter.htm) |
| Texture pixel formats including RGBA5551 and intensity/alpha | [Programming Manual 13.1 — Texture Formats](https://ultra64.ca/files/documentation/online-manuals/man/pro-man/pro13/13-01.html) |
| Configurable retrace-event cadence | [osViSetEvent](https://ultra64.ca/files/documentation/online-manuals/man/n64man/os/osViSetEvent.html) |
| Compressed audio predictors and sample loop state | [Audio bank structures, Programming Manual 19.1](https://ultra64.ca/files/documentation/online-manuals/man/pro-man/pro19/19-01.html); [ADPCM AIFC format, 19.2](https://ultra64.ca/files/documentation/online-manuals/man/pro-man/pro19/index19.2.html) |

These links point to historical primary documentation, not current SDK releases.
No manuals, ROMs, game code, assets, or proprietary headers are redistributed in
this pack. No PDF material is needed for the implemented examples.

## Target-source baseline checked

`devkitPro/libnds` commit:

```text
84e6082ce27c87ed218fb369a9944644aa2243a6
```

| Narrow contract used here | Versioned source |
|---|---|
| Named native fixed formats and geometry interface | [videoGL.h](https://github.com/devkitPro/libnds/blob/84e6082ce27c87ed218fb369a9944644aa2243a6/include/nds/arm9/videoGL.h) |
| RGB/direct-alpha bit positions | [video.h](https://github.com/devkitPro/libnds/blob/84e6082ce27c87ed218fb369a9944644aa2243a6/include/nds/arm9/video.h) |
| Native sound-format API boundary | [sound.h](https://github.com/devkitPro/libnds/blob/84e6082ce27c87ed218fb369a9944644aa2243a6/include/nds/arm9/sound.h) |

The upstream source files were inspected through their pinned raw views. The
portable C examples are original implementations of explicit conversion policies,
not copied library macros. They call no DS API and do not need SDK headers to
compile; consequently their host/cross-codegen tests do not validate SDK linkage.

## Compiler documentation and actual versions

[GCC ARM options](https://gcc.gnu.org/onlinedocs/gcc/ARM-Options.html) and
[Clang cross-compilation documentation](https://clang.llvm.org/docs/CrossCompilation.html)
explain target/CPU selection and the distinction between code generation and a
complete target environment. Those are moving documentation links; the exact
compilers executed for this pack are recorded in
[REVIEW_RESULTS.md](../tests/REVIEW_RESULTS.md).

## What is engineering inference rather than a hardware quotation

Compile-time material recipes, versioned semantic vertex identities, conservative
ownership closure, preflight-before-draw, compact animation channels, and the
performance prioritization order are design recommendations derived from the
constraints. Their effectiveness is workload-dependent. They are not universal
measured speedups, a complete N64 emulator specification, or a proof that an
arbitrary game fits on DS.

Updating an example's format/policy requires updating its tests and documented
boundary. Updating a source-specific command decoder requires checking the real
microcode/header version, not just revisiting these standard manual pages.
