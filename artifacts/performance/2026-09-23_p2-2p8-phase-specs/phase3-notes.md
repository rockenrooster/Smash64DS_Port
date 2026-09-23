# Phase 3 notes (residency, memory, ARM7 audio) -- measured facts, 2026-09-23

Integrator notes gathered while Phase 1 runs; the Phase 3 spec starts here.
Investigation baseline: `../2026-09-22_p2-2p8-architecture-baseline/INVESTIGATION_RESIDENCY.md`
(Q3 FGM, Q4 BGM, Q5 RAM, Q6 overlays). Memory deficit:
`../2026-09-23_p2-2p8-shipping-heap-census/`. MF host half:
`../2026-09-23_p2-2p8-mf-host/`.

## The ARM7 today

- The ROM ships calico's prebuilt `ds7_maine.elf` (`$(DEVKITARM)/ds_rules:12`,
  `_ARM7_ELF := -7 $(CALICO)/bin/ds7_maine.elf`).
- `ds7_maine` starts: NVRAM settings, keypad ext server, RTC, PM, block devices
  (DLDI), touch, sound server, mic server, wireless manager (mwl/ntrwifi/twlwifi),
  maxmod. The port uses **none** of wireless, mic or maxmod (no `wlmgr`, `mic*`,
  `mm*` call in `src/`); its ARM9 audio uses calico's sound API only
  (`soundPlaySample`, `soundPreparePcm`, `soundStart`, `soundKill`,
  `soundSetVolume`, `soundChSetVolume`, `soundSynchronize`,
  `soundGetActiveChannels`, `soundPause/Resume`, `soundEnable`,
  `soundSetAutoUpdate`, `soundTimerFromHz`).
- DS-mode footprint of `ds7_maine` (readelf): `.main` 4,628 B + `.main.bss` 476 B
  in main RAM at 0x02FF0000 (mirror); ARM7 WRAM `.wram` 39,708 B + `.wram.bss`
  23,100 B from 0x037F8000; DLDI 16 KB at 0x0380B000. WRAM left: ~15 KB.
- `ds7_lykoi` (same services minus sound, mic, wireless, maxmod): WRAM 10,920 +
  3,384 B. A custom ARM7 = lykoi's services + calico's sound server + our engine
  leaves roughly 60 KB of ARM7 WRAM for audio buffers (ESTIMATE: sound server
  size not yet measured).
- Building one needs no new toolchain: `libcalico_ds7.a`, `ds7.specs`, `ds7.ld`
  ship in `C:/devkitPro/calico`; the combined template
  (`C:/devkitPro/examples/nds/templates/combined/arm7`) shows the init order.

## Consequences for A8 (custom ARM7 owns BGM and FGM)

- The ARM7's main-RAM use is small either way (~5 KB); the RAM win is on the
  ARM9: the FGM cache (237,568 B BSS) and BGM buffers (16,392 B) move out of ARM9
  RAM once the ARM7 streams tails into its own WRAM rings. Resident 512 B heads
  for a match's reachable cues stay in main RAM (ESTIMATE 50-100 KB; count the
  reachable cues per roster/stage before sizing).
- BGM: 22,050 Hz IMA-ADPCM, 8,196 B packets (743 ms); the ARM7 refills a WRAM
  ring from the extent map with `blkDevReadSectors` -- 0 ARM9 ticks against
  today's ~123K spike every ~13 frames.
- The extent map is built on the ARM9 at boot (FAT chain of `argv[0]`), handed to
  the ARM7 in a cache-line-owned descriptor; slot-1 boot is the identity map.

## Memory levers (from the shipping census; to be measured in the spec)

IFCommon sprite files 208,672 B (the native OAM HUD converts them into OBJ VRAM;
GameStatus alone is 0x252D4 = 152,276 B, `nds_ifcommon_oam.c`); the stage ground
file (202,816 B on Dream Land); the FGM cache 237,568 B; battle-irrelevant code
~491 KB in the shipping image (Q5: CSS 143K, 1P 134K, menus 101K, opening 48K,
diagnostics 38K, results 27K) -> calico ROM overlays (`calico/nds/arm9/ovl.h`);
Phase 1/2 deletions (~98 KB + ~73 KB BSS).

## Overlays (A7): what calico offers

- `calico/nds/arm9/ovl.h`: `ovlInit`, `ovlLoadInPlace`, `ovlActivate`,
  `ovlDeactivate`; the example linker script
  (`C:/devkitPro/examples/nds/filesystem/nitrofs/overlays/overlays.ld`) places
  overlays at one shared VMA (`__ovlarea_main_start`) with one PHDR each, selecting
  input sections **by object file** (`:test0.o (.text ...)`); ndstool turns the
  segments into overlay files.
- This port builds large unity translation units (`scene_backend`,
  `nds_renderer` hold 294 KB / 455 KB of code by object), so an overlay needs
  either split TUs or section selection by name: with `-ffunction-sections`
  every function has its own `.text.<name>`, so a section attribute macro on the
  menu / 1P / CSS code (`.text.ovl_front.*`) lets the script pick it without
  splitting files.
- Two questions the spec must answer before sizing the win: (1) every battle ->
  front-end call edge while the front-end overlay is out (a call into an
  unloaded overlay is a wild jump -- enumerate edges from the linked ELF, not by
  grep); (2) how the battle arena reclaims the overlay area: the taskman arena is
  chosen once at boot (`diagnostics_taskman_heap.c:117-215`), so either the
  overlay area sits inside a per-scene arena extension or the arena is re-seated
  at the scene boundary.

## BGM today (read 2026-09-23, `src/nds/nds_audio_bgm.c`)

- One NitroFS file per track (`nitro:/audio/bgm_*_ima.bin`), a header then
  packets of an 8 B record (samples, payload bytes) plus an IMA-ADPCM payload
  (4 B header + nibbles, <= 16,384 samples = 743 ms at 22,050 Hz); loop point
  as (record offset, packet index) (`ndsAudioBgmReadPacket` :1223-1293).
- Two hardware channels (14, 15) play alternate packets one-shot;
  `soundPreparePcm` arms the idle one (:1295-1310); ARM9 timer 0, divider 1024,
  fires at the packet's end (:1336-1355) and posts to a worker thread that only
  `soundStart`s the prepared channel (:1357-1424, seam). The **refill** -- two
  `nitroromReadFile` reads + `DC_FlushRange` + prepare -- runs on the **main
  thread** in `ndsAudioBgmUpdate` (:2141) via `ndsAudioBgmServiceRefills`
  (:1559-1602): the ~123K-tick AUD spike every ~13 presented frames.
- ARM9 BGM API the game uses: `ndsAudioBgmPlay(player, id)`, `StopAll`,
  `CheckPlaying`, `IsPlaying`, `SetVolume`, `SetVolumeFade`, the
  `syAudioSetBGMVolumeFade` shim, `SuspendForBlockingLoad` / `Resume`.

## A8 design notes

- **Target (D7):** the custom ARM7 owns channels 14/15 and the seam timer, reads
  packets itself and applies loop/volume/fade; the ARM9 posts one PXI word per
  command (play track t, stop, volume, fade, suspend) and reads a status word
  (playing / finished) -- 0 ARM9 ticks per packet.
- **The ARM7 needs byte-addressable access to the ROM image.** Calico reads the
  NitroFS through libdvm/FatFs on the ARM9 (`dvm.h`), which exposes no cluster
  chain, so an ARM7 reader needs an **extent map**: a read-only walk of the FAT
  (FAT32; exFAT for SDXC) for the ROM file at boot, on the ARM9 with
  `blkDevReadSectors`, handed to the ARM7 as (ROM offset -> LBA run) pairs, and
  the identity map for slot-1 / emulator direct boot. That is a small read-only
  FAT driver; it is the long pole of A8 and must be validated byte-for-byte
  against `nitroromReadFile` at boot.
- **Interim, if A8 slips:** a low-priority ARM9 refill thread (runs only while
  the main thread waits for VBlank; storage access serialised with the main
  thread's reads) removes the spike from WORK without the extent map. It is not
  the target and would be deleted when A8 lands (no dual paths).
- FGM (Q3 of the residency investigation): the same extent map lets the ARM7
  stream cue tails; resident 512 B heads start a voice at once; the ARM9 FGM
  cache (237,568 B BSS) goes away.
