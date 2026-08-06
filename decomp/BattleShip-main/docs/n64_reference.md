# Nintendo 64 Technical Reference

## Memory Architecture
- **RDRAM**: 4 MB (8 MB with expansion pak). All game data lives here.
- **Segmented addressing**: The N64 uses segment registers. Addresses like `0x06001234` mean segment 6, offset 0x1234. The segment table maps segment IDs to physical RDRAM addresses.
- **DMA**: Data is transferred from ROM cartridge to RDRAM via DMA (Direct Memory Access) through the PI (Peripheral Interface). All ROM access is async DMA, not memory-mapped reads.
- **Overlays**: Code and data are loaded from ROM into RDRAM on demand. SSB64 uses overlays extensively (see `SYOverlay` struct with ROM_START/END, VRAM, TEXT/DATA/BSS segments).

## Graphics Pipeline (Reality Co-Processor)
- **RSP** (Reality Signal Processor): Programmable MIPS-based coprocessor that runs "microcode" (ucode). SSB64 uses **F3DEX2** microcode for geometry processing.
- **RDP** (Reality Display Processor): Fixed-function rasterizer. Handles texturing, blending, z-buffer, anti-aliasing.
- **Display lists**: GPU commands are built as arrays of `Gfx` structs (64-bit words each). The GBI macros (`gSPVertex`, `gDPSetTextureImage`, `gSPDisplayList`, etc.) write into these arrays.
- **Framebuffer**: 320x240 (NTSC) at 16-bit or 32-bit color. Double-buffered.
- **TMEM**: 4 KB of texture memory on the RDP. Textures must be loaded into TMEM before use, limiting texture size per draw call.

## GBI (Graphics Binary Interface)
Display list commands fall into three categories:
- **SP commands** (`gSP*`): RSP geometry commands — vertex loading, matrix operations, display list calls, lighting
- **DP commands** (`gDP*`): RDP rasterization commands — texture loading, color combiners, blend modes, fill/rect operations
- **DMA commands**: Bulk data transfer commands

When porting, these GBI calls are intercepted by libultraship's Fast3D renderer, which translates them to modern GPU API calls. The decomp code continues to call GBI macros normally.

## Audio
- **N64 audio**: Software-mixed on the RSP using audio microcode. Audio banks contain instrument definitions, samples (ADPCM compressed), and sequences (MIDI-like).
- The audio subsystem (`src/sys/audio.c`, `include/n_audio/`) manages sound effects, music, and mixing.
- In the port, audio processing routes through SDL2 instead of the RSP.

## Threading Model
SSB64 uses the N64 OS threading system:
- **Thread 0**: Idle thread (lowest priority)
- **Thread 1**: Boot/init
- **Thread 3**: Scheduler (priority 120) — manages RSP/RDP task submission
- **Thread 4**: Audio (priority 110) — processes audio DMA and mixing
- **Thread 5**: Game logic (priority 50) — main game loop
- **Thread 6**: Controller polling (priority 115)

In the port, this threading model is collapsed. libultraship runs a single main loop with explicit calls for graphics, audio, and input at the appropriate points.

## Controller Input
- N64 controller: analog stick (s8 x/y, range ~-80 to +80), 14 digital buttons, D-pad
- `I_CONTROLLER_RANGE_MAX` = 80, `F_CONTROLLER_RANGE_MAX` = 80.0f
- Controller data read via `OSContPad` struct
- In the port, libultraship's ControlDeck maps modern gamepad/keyboard input to `OSContPad` format

## Save Data
- SSB64 uses **SRAM** for save data (battery-backed cartridge RAM)
- In the port, SRAM read/write calls are redirected to filesystem operations

## Endianness
- N64 MIPS R4300i is **big-endian**. All multi-byte values in ROM and RDRAM are big-endian.
- The decomp's C code already handles this correctly (the compiler managed byte ordering).
- On PC (little-endian x86), libultraship handles any necessary byte swapping transparently through the resource system. Data loaded from .o2r archives is already in native host byte order.
- **Do not** add manual byte-swap code in game logic. If you encounter endianness issues, it means the asset extraction or resource loading layer needs fixing, not the game code.
