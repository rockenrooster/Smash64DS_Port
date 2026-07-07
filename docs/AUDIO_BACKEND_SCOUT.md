# Audio Backend Scout

This is a read-only planning scout for replacing the current silent audio seams
with a DS backend while keeping BattleShip gameplay code as the source of truth.
The original scout was read-only; slices 1 and 2 have since landed as default
audio asset parsing plus one-track compatibility BGM playback.

## Landed Slice 1

`NDS_IMPORT_BATTLESHIP_AUDIO_ASSETS` now defaults on. The Makefile stages the
original O2R audio blobs under `nitro:/audio`, including the multi-MB `.tbl`
sample banks, while the DS loader keeps those sample banks non-resident. The
loader explicitly reads the raw N64 big-endian payloads inside the O2R wrappers;
they do not pass through the reloc byte-swap path. It validates all eight staged
files open, then parses source-shaped sequence, bank, instrument/wavetable, and
FGM package counts without playback.

Mode `163` reports:
`audio=seq47 bank1=1/42/117@32000 bank2=1/1/322@44100 fgm=100/464/695 raw=4422960 resident=0 scratch=64416`.
This keeps the VSBattle memory reserve intact.

## Landed Slice 2

`NDS_IMPORT_BATTLESHIP_AUDIO_BGM` now defaults on for one track:
Dream Land/Pupupu (`nSYAudioBGMPupupu`). `scripts/render-audio-bgm-pupupu.py`
derives `assets/audio/bgm_pupupu_pcm16.raw` from original O2R
`S1_music_sbk` sequence 0 plus `B1_sounds1_ctl/tbl`, using the read-only
BattleShip CSEQ, CTL, and VADPCM tools. The output is PCM16LE mono at 22050 Hz,
2,886,710 bytes, SHA-256
`581191127a00c8ddbd4395cc00b5d4722bbeca734a0990e778a2ea5e9138effa`;
`sox stat` reports RMS amplitude `0.078523`, so the committed stream is not
silent.

The DS backend implements real `syAudioPlayBGM`, `syAudioStopBGMAll`,
`syAudioCheckBGMPlaying`, and `syAudioSetBGMVolume` over a 64 KiB stream buffer.
Mode `163` naturally starts BGM through `mpCollisionSetPlayBGM` and stops it on
VSBattle cleanup, reporting:
`bgm=track0 play=1 stop=1 refills=88 read=2949120 rate=44046 loop=1 resident=65536`.
The arena reserve still holds after subtracting the 64 KiB stream buffer.
FGM/voice playback, positional audio, and the original sequence-player import
remain future slices.

## Current Port Surface

The DS port currently exposes only a narrowed audio compatibility surface:
`syAudioThreadMain`, BGM control, FGM/voice start and stop, settings flags, and
restart state are declared in `include/sys/audio.h:48-59`. The DS gmsound enum is
also narrowed to the IDs currently needed by the port: four BGM IDs at
`include/gm/gmsound.h:4-9`, selected FGM IDs at `include/gm/gmsound.h:11-66`,
and selected voice IDs at `include/gm/gmsound.h:68-104`.

The implemented surface is still mostly a diagnostic layer, except for the
one-track BGM compatibility backend:

| Entry point | Current behavior | Citations |
|---|---|---|
| `syAudioThreadMain` | Boot service thread placeholder. | `src/port/boot_stubs.c:40`, `include/sys/audio.h:48` |
| `syAudioStopBGMAll` | Stops the default Pupupu BGM stream; other sound players remain future work. | `src/port/reloc_backend_compat_shims.c`, `include/sys/audio.h:49` |
| `syAudioPlayBGM` | Plays `nSYAudioBGMPupupu` through the DS BGM backend and records diagnostics; unsupported IDs fail softly. | `src/port/reloc_backend_compat_shims.c`, `include/sys/audio.h:50` |
| `func_800266A0_272A0` | Empty stop/all-sound compatibility hook. | `src/port/reloc_backend_compat_shims.c:419`, `include/sys/audio.h:53` |
| `func_80026738_27338` | Clears an `alSoundEffect` ID field. | `src/port/reloc_backend_compat_shims.c:425`, `include/sys/audio.h:54` |
| `func_800269C0_275C0` | Returns a static `alSoundEffect`, records FGM/voice diagnostics. | `src/port/reloc_backend_compat_shims.c:433`, `include/sys/audio.h:55` |
| `syAudioCheckBGMPlaying` | Reports the one-track backend state when BGM is enabled. | `src/port/reloc_backend_compat_shims.c`, `include/sys/audio.h:51` |
| `syAudioSetBGMVolume` | Applies volume to the active DS sample channel and records diagnostics. | `src/port/reloc_backend_compat_shims.c`, `include/sys/audio.h:52` |
| `syAudioSetSettingsUpdated` / `syAudioGetSettingsUpdated` | Setter is empty, getter returns false. | `src/port/reloc_backend_compat_shims.c:494`, `src/port/reloc_backend_compat_shims.c:498`, `include/sys/audio.h:56-57` |
| `syAudioSetFXType` / `syAudioGetRestarting` | FX type ignored; restarting false. | `src/port/reloc_backend_compat_shims.c:503`, `src/port/reloc_backend_compat_shims.c:508`, `include/sys/audio.h:58-59` |
| `ftParamPlayVoice` | Starts voice through `func_800269C0_275C0`. | `src/port/reloc_backend_compat_shims.c:5048`, `include/ft/fighter.h:3469` |
| `ftParamPlayLoopSFX` / `ftParamStopLoopSFX` | Starts/stops loop SFX through the same static handle. | `src/port/reloc_backend_compat_shims.c:5058`, `src/port/reloc_backend_compat_shims.c:5068`, `include/ft/fighter.h:3470-3471` |
| `lbCommonMakePositionFGM` | Drops position and delegates to `func_800269C0_275C0`. | `src/port/reloc_backend_compat_shims.c:7566` |
| `mpCollisionSetPlayBGM` | Stage BGM seam. | `src/port/reloc_backend_compat_shims.c:12901`, `include/gr/ground.h:394` |
| `ifCommonBattleEndAddSoundQueueID` | Weak empty battle-end sound queue. | `src/port/battle_playable_compat_stubs.c:69`, `include/if/interface.h:190` |

Imported TUs already depend on these seams. The wrapper imports declare the
FGM/voice hooks in `src/import/battleship_ftcommon_dead.c:19-20`,
`src/import/battleship_ftmain.c:24-25`, and
`src/import/battleship_ftcommon_twister.c:21`.

## Gameplay Callers Already Reaching Audio

The battle root starts stage BGM and a crowd voice in both normal and sudden
death setup paths: `decomp/BattleShip-main/decomp/src/sc/sccommon/scvsbattle.c:217-218`
and `decomp/BattleShip-main/decomp/src/sc/sccommon/scvsbattle.c:494-495`.
Battle teardown/sudden-death transitions stop BGM, wait for BGM completion,
restore volume, and stop sound players at
`decomp/BattleShip-main/decomp/src/sc/sccommon/scvsbattle.c:530-537` and
`decomp/BattleShip-main/decomp/src/sc/sccommon/scvsbattle.c:549-556`.

The battle HUD/interface source adds a broader future call surface: countdown
voices, magnify FGM, BGM ducking, battle-end sound queues, pause FGM, and final
stop hooks appear in `decomp/BattleShip-main/decomp/src/if/ifcommon.c:1761`,
`decomp/BattleShip-main/decomp/src/if/ifcommon.c:2083-2105`,
`decomp/BattleShip-main/decomp/src/if/ifcommon.c:2520-2525`,
`decomp/BattleShip-main/decomp/src/if/ifcommon.c:2629-2664`,
`decomp/BattleShip-main/decomp/src/if/ifcommon.c:2894-3100`, and
`decomp/BattleShip-main/decomp/src/if/ifcommon.c:3266-3283`.

Stage hazards already touch the FGM seam too. Hyrule twister appear audio is in
`decomp/BattleShip-main/decomp/src/gr/grcommon/grhyrule.c:174`; Pupupu Whispy
wind audio is in `decomp/BattleShip-main/decomp/src/gr/grcommon/grpupupu.c:304`.
The current DS seam has diagnostics for Pupupu wind at
`src/port/reloc_backend_compat_shims.c:433-466` through the generic FGM path.

## BattleShip Audio Stack

The original stack is not just a set of sound IDs. `sys/audio.c` owns asset
loading, heap setup, bank patching, sequence players, FGM/voice dispatch, N64
audio task generation, BGM state, and public control APIs.

Key `sys/audio.c` pieces:

| Area | Source facts |
|---|---|
| ROM asset symbols | `S1_music_sbk`, `B1_sounds1_ctl`, `B1_sounds2_ctl`, `fgm_unk`, `fgm_tbl`, and `fgm_ucd` are extern ROM ranges in `decomp/BattleShip-main/decomp/src/sys/audio.c:36-47`; settings reference them at `decomp/BattleShip-main/decomp/src/sys/audio.c:105-130`. |
| Audio heap | US builds use `gSYAudioHeapBuffer[0x56000]` at `decomp/BattleShip-main/decomp/src/sys/audio.c:141`; JP uses `0x53000` at `decomp/BattleShip-main/decomp/src/sys/audio.c:143`. |
| Bank/sequence patching | `alSeqFileNew`, `alBnkfNew`, and bank/wavetable patchers are at `decomp/BattleShip-main/decomp/src/sys/audio.c:297`, `decomp/BattleShip-main/decomp/src/sys/audio.c:312`, `decomp/BattleShip-main/decomp/src/sys/audio.c:333`, `decomp/BattleShip-main/decomp/src/sys/audio.c:360`, `decomp/BattleShip-main/decomp/src/sys/audio.c:378`, and `decomp/BattleShip-main/decomp/src/sys/audio.c:394`. |
| ROM/DMA hooks | `syAudioReadRom`, `syAudioDma`, and `syAudioDmaNew` are at `decomp/BattleShip-main/decomp/src/sys/audio.c:424`, `decomp/BattleShip-main/decomp/src/sys/audio.c:441`, and `decomp/BattleShip-main/decomp/src/sys/audio.c:487`. |
| Oscillators | Oscillator init/update/stop are at `decomp/BattleShip-main/decomp/src/sys/audio.c:527`, `decomp/BattleShip-main/decomp/src/sys/audio.c:630`, and `decomp/BattleShip-main/decomp/src/sys/audio.c:749`. |
| Asset load | `syAudioLoadAssets` reads and patches bank2, bank1, sequence bank, FGM table, and FGM ucode at `decomp/BattleShip-main/decomp/src/sys/audio.c:764-875`. |
| BGM player setup | `syAudioMakeBGMPlayers` creates `n_al` synth/config/player state at `decomp/BattleShip-main/decomp/src/sys/audio.c:880-980`. |
| Audio thread | `syAudioThreadMain` initializes, loads assets, creates players, builds audio frames, stops/restarts players, streams BGM sequences, and submits buffers at `decomp/BattleShip-main/decomp/src/sys/audio.c:990-1235`. |
| Public API | BGM and FGM public calls span `decomp/BattleShip-main/decomp/src/sys/audio.c:1243-1501`, including `syAudioPlayBGM` at line 1283, `syAudioSetBGMVolume` at line 1303, `syAudioCheckBGMPlaying` at line 1362, `syAudioPlayFGM` at line 1371, and `syAudioStopFGM` at line 1458. |

The original mixer/player code lives under `decomp/BattleShip-main/decomp/src/libultra/n_audio`.
The compressed sequence player calls include `n_alCSPNew` at
`decomp/BattleShip-main/decomp/src/libultra/n_audio/n_env.c:2396`,
`n_alCSPSetSeq` at `decomp/BattleShip-main/decomp/src/libultra/n_audio/n_cspsetseq.c:23`,
`n_alCSPSetBank` at `decomp/BattleShip-main/decomp/src/libultra/n_audio/n_cspsetbank.c:24`,
`n_alCSPPlay` at `decomp/BattleShip-main/decomp/src/libultra/n_audio/n_cspplay.c:23`,
`n_alCSPSetVol` at `decomp/BattleShip-main/decomp/src/libultra/n_audio/n_cspsetvol.c:23`,
and `n_alCSPStop` at `decomp/BattleShip-main/decomp/src/libultra/n_audio/n_cspstop.c:23`.

## Audio Data

BattleShip documents the data layout as BGM sequences in `S1_music.sbk` and
instrument/sample banks in `B1_sounds1.ctl/.tbl` and `B1_sounds2.ctl/.tbl` at
`decomp/BattleShip-main/decomp/audio.md:3-5`. The same document states that
`B1_sounds1` has 117 waveforms and 42 instruments, while `B1_sounds2` has 322
waveforms and one percussion instrument at
`decomp/BattleShip-main/decomp/audio.md:251-252`. FGM/voice scripts are the
`fgm.unk`, `fgm.tbl`, and `fgm.ucd` blobs; `fgm.ucd` is indexed by
`gmFGMVoiceID` and drives `syAudioPlayFGM(id)` through a script interpreter at
`decomp/BattleShip-main/decomp/audio.md:348-359`.

Raw BattleShip documentation sizes:

| Asset | Raw size | Citation |
|---|---:|---|
| `S1_music.sbk` | 159,248 | `decomp/BattleShip-main/decomp/MUSIC_AND_SFX_DISCOVERIES.md:16` |
| `B1_sounds1.ctl` | 26,400 | `decomp/BattleShip-main/decomp/MUSIC_AND_SFX_DISCOVERIES.md:17` |
| `B1_sounds1.tbl` | 1,141,104 | `decomp/BattleShip-main/decomp/MUSIC_AND_SFX_DISCOVERIES.md:18` |
| `B1_sounds2.ctl` | 64,416 | `decomp/BattleShip-main/decomp/MUSIC_AND_SFX_DISCOVERIES.md:19` |
| `B1_sounds2.tbl` | 2,998,752 | `decomp/BattleShip-main/decomp/MUSIC_AND_SFX_DISCOVERIES.md:20` |
| `fgm.unk` | 2,080 | `decomp/BattleShip-main/decomp/MUSIC_AND_SFX_DISCOVERIES.md:196` |
| `fgm.tbl` | 11,728 | `decomp/BattleShip-main/decomp/MUSIC_AND_SFX_DISCOVERIES.md:197` |
| `fgm.ucd` | 19,232 | `decomp/BattleShip-main/decomp/MUSIC_AND_SFX_DISCOVERIES.md:198` |

The checked `BattleShip_o2r/audio` payloads are present, with filesystem sizes
measured during this scout: `S1_music_sbk` 159,316; `B1_sounds1_ctl` 26,468;
`B1_sounds1_tbl` 1,141,172; `B1_sounds2_ctl` 64,484; `B1_sounds2_tbl`
2,998,820; `fgm_unk` 2,148; `fgm_tbl` 11,796; `fgm_ucd` 19,300. These are
filesystem metadata under `decomp/BattleShip-main/BattleShip_o2r/audio`, so
there is no source line number for the byte counts. The raw formats and sizes
above are line-cited in the BattleShip docs.

Current NitroFS staging does not include audio data. The Makefile stages only
relocated scene/fighter/interface assets through `NDS_NITROFS_RELOC_FILES` at
`Makefile:669-682`, and the `.nds` rule depends on reloc and relocdata payloads
at `Makefile:743`.

## Backend Shapes

### Shape 1: maxmod-First Compatibility Backend

Use generated DS soundbanks for a small set of BGM, FGM, and voice IDs, while
preserving the existing `sys/audio.h` call surface. This is the fastest audible
path and keeps gameplay imports untouched, but it is not a faithful port of the
N64 compressed sequence player, FGM scripts, ADPCM resampler, or envelope logic.

Slice shape:

1. Add a DS-owned backend under `src/nds` or `src/port` that implements the
   declared surface from `include/sys/audio.h:48-59`.
2. Generate a small `gmMusicID`/`gmFGMVoiceID` to maxmod asset table from
   BattleShip IDs; keep the DS enum narrowed until the call surface demands more.
3. Start with BGM IDs reached by current menu/battle paths and the FGM/voice IDs
   already named in `include/gm/gmsound.h:11-104`.
4. Gate each slice on natural runtime calls: stage BGM starts from
   `mpCollisionSetPlayBGM`, voice starts from `func_800269C0_275C0`, pause/end
   volume and stop hooks follow the imported source when those TUs are live.

This path should be labeled compatibility audio, not original audio.

### Shape 2: libnds Hardware-Channel Backend

Drive DS sound channels directly for FGM/voice and either stream converted BGM
or pair with a small BGM player. This keeps the runtime small and predictable,
which matters because the original raw sample banks are multi-megabyte assets.
The downside is limited polyphony and a high risk of losing BattleShip envelope,
priority, and FGM script behavior.

Slice shape:

1. Implement `func_800269C0_275C0`, `func_80026738_27338`,
   `ftParamPlayVoice`, and `ftParamPlayLoopSFX` as real DS channel allocation.
2. Keep BGM separate behind `syAudioPlayBGM`, `syAudioSetBGMVolume`,
   `syAudioStopBGMAll`, and `syAudioCheckBGMPlaying`.
3. Add priority/loop ownership to the returned `alSoundEffect` compatibility
   handle so fighter loop SFX stop paths at
   `src/port/reloc_backend_compat_shims.c:5068-5079` become meaningful.
4. Gate on selected live callers first: Pupupu Whispy wind, Hyrule twister,
   battle start crowd voice, fighter voice, and loop SFX stop.

This path is useful as a DS hardware smoke test and may remain a fallback, but
it should not block a later original-sequence backend.

### Shape 3: Ported BattleShip Sequence/FGM Player

Import the original stack coherently: `sys/audio.c` plus the required
`libultra/n_audio` TUs, then replace ROM/DMA/AI/RSP task boundaries with a DS
mixer/output backend. This best matches the repo mission, but it is the highest
risk shape because `syAudioThreadMain` assumes N64 audio tasks, triple sample
buffers, `Acmd` lists, `osAiSetNextBuffer`, and `n_alAudioFrame`
(`decomp/BattleShip-main/decomp/src/sys/audio.c:1019-1080`).

Whole-TU import plan:

1. Import the read-only original audio public API through `src/import`, renamed
   where necessary, without making it live for the scene.
2. Bring the parser/patcher half live first: `alSeqFileNew`, `alBnkfNew`,
   `syAudioLoadAssets`, and FGM table/ucode pointer patching. Gate by loading
   O2R audio assets from NitroFS and validating counts, offsets, and selected
   IDs without playback.
3. Bring the sequence player half live next: `n_alCSPNew`, `n_alCSPSetBank`,
   `n_alCSPSetSeq`, `n_alCSPPlay`, `n_alCSPSetVol`, and `n_alCSPStop`. Gate by
   playing/stopping one BGM from `syAudioPlayBGM`.
4. Bring the FGM/voice interpreter live after bank2/sample streaming is solved.
   Gate by replacing the current diagnostic return from `func_800269C0_275C0`
   with a real sound player handle while preserving fighter stop semantics.
5. Only then graduate `syAudioThreadMain` behavior from stub service thread to
   actual backend driver.

This is the source-backed long-term path. The memory plan must land first,
because the raw audio banks alone exceed 4 MiB when naively resident.

## Recommended Direction

Start with a small DS compatibility backend only if audible feedback is needed
soon; otherwise, the first durable slice should be a data/loader slice for the
ported sequence path. Do not import one FGM branch as proof code. The first
useful backend gate is scene-level: current battle can start/stop BGM and play
at least one voice/FGM through natural callers, with the old diagnostic markers
reduced once live audio owns those calls.

## Proposed /task-Sized Slice Sequence

1. Done: `/task Audio asset loader scout implementation` stages O2R audio files
   into NitroFS, parses `S1_music_sbk`, both `.ctl` files, and `fgm_*`, and
   gates counts/offsets from the `alSeqFileNew` / `alBnkfNew` source shapes
   without playback.
2. Done: `/task Minimal BGM backend` implements real `syAudioPlayBGM`,
   `syAudioStopBGMAll`, `syAudioCheckBGMPlaying`, and `syAudioSetBGMVolume`
   with a tiny DS streamer for Pupupu BGM, gated by natural
   `mpCollisionSetPlayBGM` battle entry, VSBattle cleanup, 44100 B/s stream
   rate, and whole-track wrap.
3. `/task Minimal FGM/voice backend`: Replace `func_800269C0_275C0` static
   handle with a real playable handle for battle start crowd voice, Pupupu wind,
   Hyrule twister, and fighter loop stop semantics.
4. `/task Original sequence-player import gate`: Import the coherent
   `sys/audio.c` parser/player plus required `libultra/n_audio` TUs behind a DS
   backend seam, prove BGM start/volume/stop through the live battle scene, then
   delete the superseded compatibility-only diagnostics.
