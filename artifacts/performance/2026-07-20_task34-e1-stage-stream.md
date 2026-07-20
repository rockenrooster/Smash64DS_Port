# Task 34 E1 — native-stage stream boundary certificate

Verdict: **STOP_BELOW_60_PERCENT. Do not begin E2 or E3.** Across the required
24 synchronized frames, zero Dream Land DObjs have a bit-identical GX stream.
The identical partition is `0 / 6,894` parameter words (`0.000%`), far below
the approximately 60% continuation gate.

## Identity and measurement scope

- source checkpoint before E1: `d2d2818c18d5b251bd0ff604f0bb4796b93eab27`;
- profile-1 Task-34 ROM: `F64EA4E6EC15334DD730FF75A92D8CAB4E7BE0E90FA42E615328ABF8A8F130A2`;
- profile-1 Task-34 ELF: `E698CE3A708E2A278F3CD45E90A659A66128207499725E7C5CFB37D9E7BAECD4`;
- mode 163, renderer mode 9, Task-26 segment 0 enabled, Task-29 census enabled,
  live Fox, production wallpaper, frozen water, and strict texture fence;
- windows: countdown `438–445`, early combat `600–607`, and Whispy
  `1398–1405`;
- all three captures use the same ROM/ELF and contain eight contiguous frames,
  zero Task-34 overflow/faults, and exact 57-DObj / 42-binding / 54-run /
  202-triangle native-stage accounting.

Task 34 is default-off diagnostic code. Its GDB gate enables copying only for
the requested eight frames, so pre-window execution is unchanged. The census
does not alter GX commands or pixels and makes no timing claim.

## Exact stream and partition

Every frame contains exactly 2,557 logical GX commands and 6,894 parameter
words. Packed as four one-byte commands per command word, the complete stream
would occupy:

| Component | Exact bytes |
|---|---:|
| 2,557 packed command bytes, padded to words | 2,560 |
| 6,894 32-bit parameter words | 27,576 |
| Complete per-frame native-stage stream | 30,136 |
| Certified bit-identical partition | **0** |

All 42 display-bearing DObjs vary:

```text
0,1,2,3,4,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,
23,26,27,28,29,32,33,35,36,38,40,41,42,45,46,48,49,51,52,54,55,56
```

The other 15 hierarchy nodes have no display stream, matching their generated
`binding_index = 0xffff`:

```text
5,21,22,24,25,30,31,34,37,39,43,44,47,50,53
```

The first frame's native-stage command classes are 216 control, 36 alpha-test,
36 texture-param, 41 texture-bind, 100 matrix-mode, 246 matrix-load-4x4,
25 polygon-format, 54 begin, 606 color, 591 texcoord, and 606 vertex16.
There are no matrix push, pop, store, or restore commands.

Task 29's owner 0 is deliberately broader than this DObj owner: it also absorbs
non-fighter transient effects. For example, frame 600 reports 2,664 commands /
7,080 parameter words in broad owner 0, while the bracketed native stage remains
2,557 / 6,894. Task 34 therefore proves an ordered, fault-free subset of owner 0
instead of mislabeling transient effects as stage DObjs.

## Task-23R cross-check and cause

`NDS_NATIVE_STAGE_CONSUMED_FIELDS.generated.json` classifies `projection` and
all 42 `binding_composed` matrices as live-camera-dependent. E1 confirms the
consequence at the final GX boundary: every display-bearing DObj emits one or
more live matrix loads, so even rigid geometry is not a bit-identical stream
when the camera moves. The prepared control/state lanes measured by Task 23R
can remain stable while their final serialized matrix operands vary.

## Captured seam

The recorder begins immediately inside `ndsRendererCommitNativeStageSegment()`
after segment admission and ends after its last run. On entry, the exact stage
owner is active, the segment index/order is valid, preflight has prepared live
matrices/materials/textures, the GX FIFO is available, and any prior primitive
has been closed. Each run then selects/loads its matrix, binds texture or no
texture, applies alpha/fog/poly state, opens triangles, emits vertices, and
closes the primitive.

On exit, the primitive is closed and matrix-stack depth is unchanged: the
stream contains zero push/pop/store/restore commands. The last run's matrix,
texture/material, alpha, fog, polygon, color, and texcoord state remain bound;
the owner does not restore a neutral state. Any replay would therefore have to
preserve the renderer's matching software shadows and the original eight
callback boundaries. E1's kill result prevents that replay from being built.

## Decision and graveyard relation

The mandatory continuation threshold is missed by 60 percentage points, so no
arena buffer, CPU replay, GX-FIFO DMA, update overlap, fallback state machine,
or device A/B is authorized. This is not the graveyard CPU-copy replay: no
stream is replayed at all. The proposed differences from that rejected path
were DMA transport and overlap with the fixed source-update pair; neither can
help when the certified immutable payload is zero bytes.

## Evidence

- compact analysis: `2026-07-20_task34-e1-stage-stream-analysis.json`, SHA-256
  `370130E735CEBD0CBE2CCF5F2A8D0D7554AF9553017A56A0FB51B78EEBF7CD77`;
- lossless raw capture archive: `2026-07-20_task34-e1-raw-captures.zip`,
  1,284,189 bytes, SHA-256
  `2195DA35FCF5EEBABFF8030FA4E1EFB7E9FBC413BFED9BEBF19581B765E67E76`;
- raw capture SHA-256 values inside that archive:
  - countdown: `71B291004BBB2EB908F261B9713716DEB3A717D25F532A0BABCC4DC00B13FDD3`;
  - early: `13066FB0B211CCEEAB8B12BDB15CAC7BB0BA387CD1CE85F3E02F296BA1C7689D`;
  - Whispy: `9F795D2990B4E003B242B191C7BDFC1FF83891A856506BDF30FE24502F48F84E`.

Task 34 is closed at E1. Preserve the default-off certificate and do not
restore E2/E3 work without a new architecture and a new explicit task.
