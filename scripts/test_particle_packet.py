#!/usr/bin/env python3
"""Host-execute the production particle FIFO packet emitters.

This fixture extracts the actual packet reserve/flush, state, quad and submit
functions from nds_renderer_textures_effects.c, compiles them as C, then
independently decodes the command words captured from packet RAM. The mock
only replaces the DS MMIO/cache/DMA sinks and texture binding lookup. It does
not claim to prove hardware DMA timing or rendered pixels.

Run from the repository root with:
    python -m pytest scripts/test_particle_packet.py -q
or:
    python scripts/test_particle_packet.py
"""

from __future__ import annotations

import re
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
RENDERER_PATH = ROOT / "src/nds/nds_renderer_textures_effects.c"
PREAMBLE_PATH = ROOT / "src/nds/nds_renderer_preamble.c"
BUILDS = ROOT / "builds"


def clean_c(source: str) -> str:
    """Blank comments without changing offsets, so declarations retain spans."""
    return re.sub(
        r"/\*.*?\*/|//[^\n]*",
        lambda match: " " * len(match[0]),
        source,
        flags=re.S,
    )


def extract_function(source: str, name: str) -> str:
    """Return one current C definition, not a model of its behavior."""
    plain = clean_c(source)
    for match in re.finditer(rf"\b{re.escape(name)}\s*\(", plain):
        opening_paren = plain.find("(", match.start())
        paren_depth = 1
        cursor = opening_paren + 1
        while cursor < len(plain) and paren_depth:
            if plain[cursor] == "(":
                paren_depth += 1
            elif plain[cursor] == ")":
                paren_depth -= 1
            cursor += 1
        if paren_depth:
            continue
        after = cursor
        while after < len(plain) and plain[after].isspace():
            after += 1
        if after >= len(plain) or plain[after] != "{":
            continue

        # Include a split return type/attribute line preceding the function
        # name, while stopping at the previous C declaration or function body.
        line_start = plain.rfind("\n", 0, match.start()) + 1
        while line_start > 0:
            previous_end = line_start - 1
            previous_start = plain.rfind("\n", 0, previous_end) + 1
            previous = plain[previous_start:previous_end].strip()
            if not previous or any(ch in previous for ch in ";{}"):
                break
            line_start = previous_start

        brace_depth = 1
        end = after + 1
        while end < len(plain) and brace_depth:
            if plain[end] == "{":
                brace_depth += 1
            elif plain[end] == "}":
                brace_depth -= 1
            end += 1
        if brace_depth:
            raise AssertionError(f"Unclosed function body for {name}")
        return source[line_start:end]
    raise AssertionError(f"Missing current C function definition: {name}")


def extract_struct(source: str, name: str) -> str:
    plain = clean_c(source)
    match = re.search(
        rf"typedef\s+struct\s+{re.escape(name)}\s*\{{", plain
    )
    if match is None:
        raise AssertionError(f"Missing current C struct: {name}")
    opening = plain.index("{", match.start())
    depth = 1
    end = opening + 1
    while depth:
        if plain[end] == "{":
            depth += 1
        elif plain[end] == "}":
            depth -= 1
        end += 1
    end = plain.index(";", end) + 1
    return source[match.start():end]


FUNCTIONS = (
    "ndsRendererParticleAbsQ8",
    "ndsRendererParticleTruncShiftS32",
    "ndsRendererParticleQ8ToV16",
    "ndsRendererParticleScaleShiftForQ8",
    "ndsRendererParticlePacketSetFinalBinding",
    "ndsRendererFlushWhispyNativePacket",
    "ndsRendererWhispyPacketReserve",
    "ndsRendererAppendParticlePacketStateRaw",
    "ndsRendererAppendWhispyPacketScale",
    "ndsRendererAppendParticlePacketQuad",
    "ndsRendererSubmitParticleQuadPacket",
)


C_PRELUDE = r"""
#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef int16_t s16;
typedef uint32_t u32;
typedef int32_t s32;
typedef int64_t s64;
typedef uint64_t u64;
typedef int32_t sb32;
typedef float f32;
typedef s16 v16;
typedef s16 t16;

#define TRUE 1
#define FALSE 0
#define NDS_RENDERER_WHISPY_PACKET_WORDS 1024u
#define NDS_RENDERER_PARTICLE_EXTENT_GUARD_Q8 64u
#define NDS_RENDERER_PARTICLE_COORD_SHIFT 12u
#define NDS_RENDERER_PARTICLE_MAX_SCALE_SHIFT 4u
#define NDS_PARTICLE_ENV_VARIANT_WHITE 0x7FFFu
#define NDS_RENDERER_FAST_RUN_CODE

#define FIFO_NOP         0x00u
#define FIFO_COLOR       0x20u
#define FIFO_TEX_COORD   0x21u
#define FIFO_VERTEX16    0x22u
#define FIFO_TEX_FORMAT  0x30u
#define FIFO_PAL_FORMAT  0x31u
#define FIFO_POLY_FORMAT 0x32u
#define FIFO_BEGIN       0x33u
#define MATRIX_POP       0x40u
#define MATRIX_PUSH      0x41u
#define MATRIX_SCALE     0x42u
#define GL_QUAD          0x07u
#define REG2ID(reg)      (reg)
#define FIFO_COMMAND_PACK(a,b,c,d) \
    ((u32)(a) | ((u32)(b) << 8) | ((u32)(c) << 16) | ((u32)(d) << 24))
#define TEXTURE_PACK(s,t) \
    ((u32)(u16)(s) | ((u32)(u16)(t) << 16))
#define POLY_ALPHA(a)    (((u32)(a) & 31u) << 16)
#define POLY_CULL_NONE   0x00004000u
#define POLY_ID(id)      (((u32)(id) & 63u) << 24)

/*__PACKET_TYPES__*/

typedef struct { int activeTexture; int activePalette; } TestGlState;
typedef struct { u32 poly_fmt; u32 valid_mask; } TestGXShadow;

static NDSRendererWhispyPacket sNdsRendererWhispyPacket;
static u32 sNdsRendererParticleQuadTexture;
static u32 sNdsRendererParticleQuadPalette;
static u32 sNdsRendererParticleQuadAlpha;
static u32 sNdsRendererParticlePacketStateDirty;
static u32 sNdsRendererParticleQuadOpen;
static u32 sNdsRendererParticleScaleShift;
static u32 sNdsRendererParticleViewSpace;
static u32 sNdsRendererHardwareBoundTextureName;
static void *sNdsRendererHardwareActiveTextureEntry;
static TestGlState sTestGlState;
static TestGlState *glGlob = &sTestGlState;
static TestGXShadow sNdsRendererGXStateShadow;

static volatile u32 gNdsWhispyAOTTier4PacketQuads;
static volatile u32 gNdsWhispyAOTTier4PacketStateGroups;
static volatile u32 gNdsWhispyAOTTier4PacketFlushes;
static volatile u32 gNdsWhispyAOTTier4PacketWords;
static volatile u32 gNdsWhispyAOTTier4PacketFallbacks;
static volatile u32 gNdsParticlePacketQuads;
static volatile u32 gNdsParticlePacketStateGroups;
static volatile u32 gNdsParticlePacketFlushes;
static volatile u32 gNdsParticlePacketWords;
static volatile u32 gNdsParticlePacketFallbacks;
static volatile u32 gNdsParticlePacketDynamicBinds;
static volatile u32 gNdsParticleQuadSheetBreaks;
static volatile u32 gNdsParticleQuadPaletteBreaks;
static volatile u32 gNdsParticleQuadAlphaBreaks;
static volatile u32 gNdsParticleWorldClampCount;
static volatile u32 gNdsParticleScaleEscalations;
static volatile u32 gNdsParticleScaleShiftMax;
static u32 gTextureBindCalls;
static u32 gPrepareCalls;

static u32 gPublished[20000];
static u32 gPublishedCount;
static u32 gDmaCr[1];
static u32 gDmaSrc[1];
static u32 gDmaDest[1];
static u32 gDmaFifoDummy;
#define DMA_CR(channel)   (gDmaCr[(channel)])
#define DMA_SRC(channel)  (gDmaSrc[(channel)])
#define DMA_DEST(channel) (gDmaDest[(channel)])
#define DMA_BUSY 0x80000000u
#define DMA_FIFO 0x04000000u
#define DC_FlushRange(ptr, bytes) testCaptureDmaPacket((ptr), (bytes))
#define NDS_RENDERER_GX_STATE_TEXTURE_PARAMS 1u
#define NDS_RENDERER_GX_STATE_POLY_FMT 2u

static void testAppendPublished(u32 word)
{
    assert(gPublishedCount < (u32)(sizeof(gPublished) / sizeof(gPublished[0])));
    gPublished[gPublishedCount++] = word;
}

static void testCaptureDmaPacket(const void *source, size_t bytes)
{
    const u32 *words = (const u32 *)source;
    size_t count = bytes / sizeof(u32);
    size_t i;
    assert((bytes % sizeof(u32)) == 0u);
    for (i = 0; i < count; i++) testAppendPublished(words[i]);
}

static void ndsRendererHardwareInvalidateGXState(u32 mask)
{
    (void)mask;
}

static void ndsRendererProfileRecordTextureBind(void)
{
    gTextureBindCalls++;
}

static void ndsRendererFlushWhispyNativePacket(void);
static u32 *ndsRendererWhispyPacketReserve(u32 words);

static sb32 ndsRendererParticlePacketBindingFor(
    u32 texture_name, u32 env_palette_name,
    NDSRendererWhispyNativeBinding *out)
{
    if ((out == NULL) || ((texture_name != 11u) && (texture_name != 22u)))
        return FALSE;
    out->texture_name = texture_name;
    out->texture_format = (texture_name == 11u) ? 0x00111111u : 0x00222222u;
    out->palette_format = 0x00330000u | (env_palette_name & 0xFFFFu);
    out->palette_name = (env_palette_name == 0u)
        ? 0 : (s32)(7000u + env_palette_name);
    out->valid = TRUE;
    return TRUE;
}

static void ndsRendererPrepareWhispyQuadState(
    u32 texture_name, u32 poly_alpha, u32 palette_name, u32 mode)
{
    (void)texture_name; (void)poly_alpha; (void)palette_name; (void)mode;
    gPrepareCalls++;
}

static s32 inttof32(s32 value)
{
    return value * 65536;
}

"""


C_DRAW_HARNESS = r"""
struct TestDraw
{
    u32 texture, palette, alpha, color;
    s32 center[3], right[3], up[3];
    u32 atlas_x, atlas_y, atlas_w, atlas_h;
};

static const struct TestDraw sDraws[] = {
    {11u, 0u, 31u, 0xFF010203u,
     {-768, 512, -256}, {256, -128, 64}, {-512, 256, 128}, 3u, 5u, 7u, 9u},
    {11u, 0u, 15u, 0xFF111213u,
     {1024, -512, 768}, {256, 512, -256}, {512, -256, 128}, 11u, 2u, 4u, 6u},
    {11u, 100u, 15u, 0xFF212223u,
     {-256, 256, 0}, {256, 0, 256}, {0, 256, -256}, 1u, 10u, 3u, 5u},
    {22u, 100u, 15u, 0xFF313233u,
     {512, 768, -768}, {-256, 256, 512}, {512, -256, 0}, 20u, 1u, 2u, 4u},
    {22u, 100u, 15u, 0xFF414243u,
     {-1024, 256, 512}, {512, 256, -256}, {-256, -512, 256}, 7u, 8u, 5u, 3u},
    {22u, 100u, 15u, 0xFF515253u,
     {256, -768, 1024}, {-512, 256, 256}, {256, 512, -512}, 9u, 4u, 6u, 2u}
};

static void reset_fixture(void)
{
    memset(&sNdsRendererWhispyPacket, 0, sizeof(sNdsRendererWhispyPacket));
    memset(&sTestGlState, 0, sizeof(sTestGlState));
    memset(&sNdsRendererGXStateShadow, 0, sizeof(sNdsRendererGXStateShadow));
    memset(gPublished, 0, sizeof(gPublished));
    memset(gDmaCr, 0, sizeof(gDmaCr));
    memset(gDmaSrc, 0, sizeof(gDmaSrc));
    memset(gDmaDest, 0, sizeof(gDmaDest));
    gPublishedCount = 0u;
    sNdsRendererParticleQuadTexture = 11u;
    sNdsRendererParticleQuadPalette = 0u;
    sNdsRendererParticleQuadAlpha = 31u;
    sNdsRendererParticlePacketStateDirty = FALSE;
    sNdsRendererParticleQuadOpen = TRUE;
    sNdsRendererParticleScaleShift = 0u;
    sNdsRendererHardwareBoundTextureName = 11u;
    sNdsRendererHardwareActiveTextureEntry = NULL;
    sNdsRendererParticleViewSpace = FALSE;
    gNdsWhispyAOTTier4PacketQuads = 0u;
    gNdsWhispyAOTTier4PacketStateGroups = 0u;
    gNdsWhispyAOTTier4PacketFlushes = 0u;
    gNdsWhispyAOTTier4PacketWords = 0u;
    gNdsWhispyAOTTier4PacketFallbacks = 0u;
    gNdsParticlePacketQuads = 0u;
    gNdsParticlePacketStateGroups = 0u;
    gNdsParticlePacketFlushes = 0u;
    gNdsParticlePacketWords = 0u;
    gNdsParticlePacketFallbacks = 0u;
    gNdsParticlePacketDynamicBinds = 0u;
    gNdsParticleQuadSheetBreaks = 0u;
    gNdsParticleQuadPaletteBreaks = 0u;
    gNdsParticleQuadAlphaBreaks = 0u;
    gNdsParticleWorldClampCount = 0u;
    gNdsParticleScaleEscalations = 0u;
    gNdsParticleScaleShiftMax = 0u;
    gTextureBindCalls = 0u;
    gPrepareCalls = 0u;
}

static void submit_draw(u32 index)
{
    const struct TestDraw *draw = &sDraws[index];
    if (index == 4u) sNdsRendererParticlePacketStateDirty = TRUE;
    assert(ndsRendererSubmitParticleQuadPacket(
        draw->texture, draw->palette,
        draw->center, draw->right, draw->up,
        draw->color, draw->alpha, 0u,
        draw->atlas_x, draw->atlas_y, draw->atlas_w, draw->atlas_h));
}

static void print_trace(const char *name)
{
    u32 i;
    printf("TRACE %s %u\n", name, gPublishedCount);
    for (i = 0; i < gPublishedCount; i++) printf("%08x\n", gPublished[i]);
}

static void run_buffered_route(void)
{
    u32 i;
    reset_fixture();
    for (i = 0; i < 6u; i++) submit_draw(i);
    ndsRendererFlushWhispyNativePacket();
    assert(gNdsParticlePacketQuads == 6u);
    assert(gNdsParticleQuadSheetBreaks == 1u);
    assert(gNdsParticleQuadPaletteBreaks == 1u);
    assert(gNdsParticleQuadAlphaBreaks == 1u);
    assert(gPrepareCalls == 0u);
    assert(gTextureBindCalls == 1u);
    assert(sTestGlState.activeTexture == 22);
    assert(sTestGlState.activePalette == 7100);
    assert(sNdsRendererWhispyPacket.word_count == 0u);
    print_trace("buffered");
}

static void run_reserve_boundary(void)
{
    u32 i;
    reset_fixture();
    sNdsRendererWhispyPacket.final_texture_name = 11u;
    sNdsRendererWhispyPacket.final_texture_format = 0x00111111u;
    sNdsRendererWhispyPacket.final_palette_format = 0x00330000u;
    sNdsRendererWhispyPacket.final_palette_name = 0;
    sNdsRendererWhispyPacket.final_poly_alpha = 31u;
    sNdsRendererWhispyPacket.word_count = 1008u;
    for (i = 0; i < 1008u; i++) sNdsRendererWhispyPacket.words[i] = 0u;
    submit_draw(3u);
    assert(gNdsParticlePacketFlushes == 1u);
    assert(gNdsParticlePacketQuads == 1u);
    ndsRendererFlushWhispyNativePacket();
    assert(gNdsWhispyAOTTier4PacketFlushes == 2u);
    assert(gPrepareCalls == 0u);
    print_trace("boundary");
}

struct TestPlaneDraw
{
    u32 mirror_mask;
    u32 color;
    s32 center[3], right[3], up[3];
    u32 atlas_x, atlas_y, atlas_w, atlas_h;
};

static const struct TestPlaneDraw sPlaneDraws[] = {
    {0u, 0xFF600000u,
     {256, -512, 768}, {512, 0, 0}, {0, -768, 0}, 3u, 5u, 7u, 9u},
    {1u, 0xFF610000u,
     {8400000, -512, 256}, {-512, 0, 0}, {0, 256, 0}, 11u, 2u, 4u, 6u},
    {2u, 0xFF620000u,
     {2048, -1024, 512}, {768, 0, 0}, {0, -512, 0}, 1u, 10u, 3u, 5u},
    {3u, 0xFF630000u,
     {17000000, 2560, -2048}, {-1024, 0, 0}, {0, 768, 0}, 20u, 1u, 2u, 4u}
};

static void submit_plane_draw(u32 index)
{
    const struct TestPlaneDraw *draw = &sPlaneDraws[index];
    assert(ndsRendererSubmitParticleQuadPacket(
        11u, 0u, draw->center, draw->right, draw->up,
        draw->color, 31u, draw->mirror_mask,
        draw->atlas_x, draw->atlas_y, draw->atlas_w, draw->atlas_h));
}

static void run_plane_route(const char *trace_name, u32 view_space)
{
    u32 i;
    reset_fixture();
    sNdsRendererParticleViewSpace = view_space;
    for (i = 0; i < 4u; i++) submit_plane_draw(i);
    ndsRendererFlushWhispyNativePacket();
    assert(gNdsParticleScaleEscalations == 2u);
    assert(gNdsParticleScaleShiftMax == 2u);
    assert(sNdsRendererParticleScaleShift == 2u);
    assert(gNdsParticlePacketQuads == 9u);
    assert(gPrepareCalls == 0u);
    print_trace(trace_name);
}

int main(void)
{
    run_buffered_route();
    run_reserve_boundary();
    run_plane_route("generic3d", FALSE);
    run_plane_route("viewspace3", TRUE);
    return 0;
}
"""


def production_harness() -> str:
    renderer = RENDERER_PATH.read_text(encoding="utf-8")
    preamble = PREAMBLE_PATH.read_text(encoding="utf-8")
    packet_type = extract_struct(renderer, "NDSRendererWhispyPacket")
    binding_type = extract_struct(preamble, "NDSRendererWhispyNativeBinding")
    functions = {name: extract_function(renderer, name) for name in FUNCTIONS}

    # Compile the real flush body against an inert address target for GFX_FIFO.
    # The mock DC_FlushRange captures its RAM block at the same point the real
    # cache publication happens.
    flush = functions["ndsRendererFlushWhispyNativePacket"]
    functions["ndsRendererFlushWhispyNativePacket"] = (
        "#define GFX_FIFO gDmaFifoDummy\n"
        + flush
        + "\n#undef GFX_FIFO\n"
    )
    ordered = (
        "ndsRendererParticleAbsQ8",
        "ndsRendererParticleTruncShiftS32",
        "ndsRendererParticleQ8ToV16",
        "ndsRendererParticleScaleShiftForQ8",
        "ndsRendererParticlePacketSetFinalBinding",
        "ndsRendererFlushWhispyNativePacket",
        "ndsRendererWhispyPacketReserve",
        "ndsRendererAppendParticlePacketStateRaw",
        "ndsRendererAppendWhispyPacketScale",
        "ndsRendererAppendParticlePacketQuad",
        "ndsRendererSubmitParticleQuadPacket",
    )
    bodies = "\n\n".join(functions[name] for name in ordered)
    return (
        C_PRELUDE.replace(
            "/*__PACKET_TYPES__*/",
            packet_type + "\n" + binding_type,
        )
        + "\n"
        + bodies
        + "\n"
        + C_DRAW_HARNESS
    )


OPCODES = {
    "color": 0x20,
    "texcoord": 0x21,
    "vertex16": 0x22,
    "tex_format": 0x30,
    "pal_format": 0x31,
    "poly_format": 0x32,
    "begin": 0x33,
    "matrix_pop": 0x40,
    "matrix_push": 0x41,
    "matrix_scale": 0x42,
}


def s16(value: int) -> int:
    return value - 0x10000 if value & 0x8000 else value


def s32(value: int) -> int:
    return value - 0x100000000 if value & 0x80000000 else value


def decode_fifo(words: list[int]) -> list[tuple]:
    """Decode command IDs and their data words without consulting emitter code."""
    events: list[tuple] = []
    cursor = 0
    while cursor < len(words):
        command_word = words[cursor]
        cursor += 1
        for slot in range(4):
            opcode = (command_word >> (slot * 8)) & 0xFF
            if opcode == 0:
                continue
            if opcode in (OPCODES["color"], OPCODES["texcoord"],
                          OPCODES["tex_format"], OPCODES["pal_format"],
                          OPCODES["poly_format"], OPCODES["begin"],
                          OPCODES["matrix_pop"]):
                if cursor >= len(words):
                    raise AssertionError("truncated FIFO parameter word")
                value = words[cursor]
                cursor += 1
                if opcode == OPCODES["color"]:
                    events.append(("color", value))
                elif opcode == OPCODES["texcoord"]:
                    events.append(("texcoord", s16(value & 0xFFFF),
                                   s16((value >> 16) & 0xFFFF)))
                elif opcode == OPCODES["tex_format"]:
                    events.append(("tex_format", value))
                elif opcode == OPCODES["pal_format"]:
                    events.append(("pal_format", value))
                elif opcode == OPCODES["poly_format"]:
                    events.append(("poly_format", value))
                elif opcode == OPCODES["begin"]:
                    events.append(("begin", value))
                elif opcode == OPCODES["matrix_pop"]:
                    events.append(("matrix_pop", value))
            elif opcode == OPCODES["matrix_push"]:
                events.append(("matrix_push",))
            elif opcode == OPCODES["vertex16"]:
                if cursor + 1 >= len(words):
                    raise AssertionError("truncated FIFO VERTEX16 parameter")
                xy = words[cursor]
                z = words[cursor + 1]
                cursor += 2
                events.append(("vertex", s16(xy & 0xFFFF),
                               s16((xy >> 16) & 0xFFFF), s32(z)))
            elif opcode == OPCODES["matrix_scale"]:
                if cursor + 2 >= len(words):
                    raise AssertionError("truncated FIFO MATRIX_SCALE params")
                events.append(("matrix_scale", words[cursor], words[cursor + 1],
                               words[cursor + 2]))
                cursor += 3
            else:
                raise AssertionError(f"unknown FIFO command 0x{opcode:02x}")
    return events


def parse_traces(output: str) -> dict[str, list[int]]:
    lines = output.splitlines()
    traces: dict[str, list[int]] = {}
    cursor = 0
    while cursor < len(lines):
        parts = lines[cursor].split()
        if len(parts) != 3 or parts[0] != "TRACE":
            raise AssertionError(f"unexpected harness output: {lines[cursor]!r}")
        name, count = parts[1], int(parts[2])
        cursor += 1
        end = cursor + count
        if end > len(lines):
            raise AssertionError(f"trace {name} is truncated")
        traces[name] = [int(value, 16) for value in lines[cursor:end]]
        cursor = end
    return traces


EXPECTED_QUADS = (
    # Manually tabulated from the six test inputs; this is an oracle, not an
    # encoder or a second implementation of the production conversion path.
    (0xFF010203, ((48, 224), (160, 224), (160, 80), (48, 80)),
     ((-2, 1, -1), (0, 0, -1), (-4, 2, 0), (-6, 3, 0))),
    (0xFF111213, ((176, 128), (240, 128), (240, 32), (176, 32)),
     ((1, -3, 3), (3, 1, 1), (7, -1, 2), (5, -5, 4))),
    (0x7FFF, ((16, 240), (64, 240), (64, 160), (16, 160)),
     ((-2, 0, 0), (0, 0, 2), (0, 2, 0), (-2, 2, -2))),
    (0x7FFF, ((320, 80), (352, 80), (352, 16), (320, 16)),
     ((1, 3, -5), (-1, 5, -1), (3, 3, -1), (5, 1, -5))),
    (0x7FFF, ((112, 176), (192, 176), (192, 128), (112, 128)),
     ((-5, 2, 2), (-1, 4, 0), (-3, 0, 2), (-7, -2, 4))),
    (0x7FFF, ((144, 96), (240, 96), (240, 64), (144, 64)),
     ((2, -6, 5), (-2, -4, 7), (0, 0, 3), (4, -2, 1))),
)


def quad_events(index: int) -> list[tuple]:
    color, uv, vertices = EXPECTED_QUADS[index]
    events: list[tuple] = [("color", color)]
    for texcoord, vertex in zip(uv, vertices):
        events.append(("texcoord", *texcoord))
        events.append(("vertex", *vertex))
    return events


def expected_complete_trace() -> list[tuple]:
    events: list[tuple] = []
    events += quad_events(0)
    events += [("poly_format", (15 << 16) | 0x4000), ("begin", 7)]
    events += quad_events(1)
    events += [("tex_format", 0x00111111), ("pal_format", 0x00330064),
               ("begin", 7)]
    events += quad_events(2)
    events += [("tex_format", 0x00222222), ("pal_format", 0x00330064),
               ("begin", 7)]
    events += quad_events(3)
    # The dirty transition deliberately re-emits both texture words unchanged.
    events += [("tex_format", 0x00222222), ("pal_format", 0x00330064),
               ("begin", 7)]
    events += quad_events(4)
    events += quad_events(5)
    return events


def expected_boundary_trace() -> list[tuple]:
    return [
        ("tex_format", 0x00222222),
        ("pal_format", 0x00330064),
        ("poly_format", (15 << 16) | 0x4000),
        ("begin", 7),
    ] + quad_events(3)


PLANE_EXPECTED_QUADS = (
    (0xFF600000, ((48, 224), (160, 224), (160, 80), (48, 80)),
     ((-1, 1, 3), (3, 1, 3), (3, -5, 3), (-1, -5, 3))),
    (0xFF610000, ((176, 128), (239, 128), (239, 32), (176, 32)),
     ((16407, -1, 0), (16406, -1, 0), (16406, 0, 0), (16407, 0, 0))),
    (0xFF610000, ((239, 128), (176, 128), (176, 32), (239, 32)),
     ((16406, -1, 0), (16405, -1, 0), (16405, 0, 0), (16406, 0, 0))),
    (0xFF620000, ((16, 160), (64, 160), (64, 239), (16, 239)),
     ((2, -1, 1), (5, -1, 1), (5, -2, 1), (2, -2, 1))),
    (0xFF620000, ((16, 239), (64, 239), (64, 160), (16, 160)),
     ((2, -2, 1), (5, -2, 1), (5, -3, 1), (2, -3, 1))),
    (0xFF630000, ((320, 16), (351, 16), (351, 79), (320, 79)),
     ((16602, 1, -2), (16601, 1, -2), (16601, 2, -2), (16602, 2, -2))),
    (0xFF630000, ((351, 16), (320, 16), (320, 79), (351, 79)),
     ((16601, 1, -2), (16600, 1, -2), (16600, 2, -2), (16601, 2, -2))),
    (0xFF630000, ((320, 79), (351, 79), (351, 16), (320, 16)),
     ((16602, 2, -2), (16601, 2, -2), (16601, 3, -2), (16602, 3, -2))),
    (0xFF630000, ((351, 79), (320, 79), (320, 16), (351, 16)),
     ((16601, 2, -2), (16600, 2, -2), (16600, 3, -2), (16601, 3, -2))),
)


def decode_quads(events: list[tuple]) -> list[tuple]:
    quads = []
    for index, event in enumerate(events):
        if event[0] != "color":
            continue
        uv = []
        vertices = []
        for corner in range(4):
            texcoord = events[index + 1 + corner * 2]
            vertex = events[index + 2 + corner * 2]
            if texcoord[0] != "texcoord" or vertex[0] != "vertex":
                raise AssertionError("FIFO quad lost interleaved UV/vertex order")
            uv.append(texcoord[1:])
            vertices.append(vertex[1:])
        quads.append((event[1], tuple(uv), tuple(vertices)))
    return quads


class ParticlePacketHostRegression(unittest.TestCase):
    def test_buffered_packet_overflow_and_plane_specialization(self) -> None:
        compiler = next(
            (shutil.which(candidate) for candidate in ("gcc", "clang", "cc")
             if shutil.which(candidate)),
            None,
        )
        self.assertIsNotNone(compiler, "Host C compiler required (gcc/clang/cc)")
        BUILDS.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(prefix="particle-packet-", dir=BUILDS) as temp:
            temp_path = Path(temp)
            source = temp_path / "particle_packet_host.c"
            executable = temp_path / "particle_packet_host.exe"
            source.write_text(production_harness(), encoding="utf-8")
            built = subprocess.run(
                [compiler, "-std=c11", "-O0", str(source), "-o", str(executable)],
                capture_output=True,
                text=True,
            )
            self.assertEqual(
                built.returncode, 0,
                f"host harness did not compile:\n{built.stdout}\n{built.stderr}",
            )
            run = subprocess.run(
                [str(executable)], capture_output=True, text=True, check=False
            )
            self.assertEqual(run.returncode, 0, f"C harness failed:\n{run.stderr}")
            traces = parse_traces(run.stdout)

        self.assertEqual(set(traces), {"buffered", "boundary", "generic3d", "viewspace3"})
        buffered = decode_fifo(traces["buffered"])
        boundary = decode_fifo(traces["boundary"])
        generic3d = decode_fifo(traces["generic3d"])
        viewspace3 = decode_fifo(traces["viewspace3"])
        expected = expected_complete_trace()
        self.assertEqual(buffered, expected, "buffered packet command/data sequence changed")
        self.assertEqual(boundary, expected_boundary_trace(),
                         "reserve-triggered DMA split changed state/quad order")
        self.assertEqual(viewspace3, generic3d,
                         "view-space plane specialization diverged from generic 3D")
        self.assertEqual(decode_quads(viewspace3), list(PLANE_EXPECTED_QUADS),
                         "mode3 signed plane corners or mirrored UV order changed")
        self.assertEqual(
            [event for event in viewspace3 if event[0].startswith("matrix_")],
            [
                ("matrix_pop", 1), ("matrix_push",),
                ("matrix_scale", 0x20000, 0x20000, 0x20000),
                ("matrix_pop", 1), ("matrix_push",),
                ("matrix_scale", 0x40000, 0x40000, 0x40000),
            ],
            "mode3 scale escalation commands or ordering changed",
        )


if __name__ == "__main__":
    unittest.main()
