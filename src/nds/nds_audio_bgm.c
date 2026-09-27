#include <calico.h>
#include <nds.h>
#include <stdio.h>
#include <string.h>

#include <gm/gmsound.h>
#include <nds/nds_audio_bgm.h>
#include <nds/nds_audio_fgm.h>
#include <nds/nds_bgm_ipc.h>
#include <sys/audio.h>

#define NDS_AUDIO_BGM_PATH_PUPUPU "nitro:/audio/bgm_pupupu_ima.bin"
#define NDS_AUDIO_BGM_PATH_WIN_MARIO "nitro:/audio/bgm_win_mario_ima.bin"
#define NDS_AUDIO_BGM_PATH_WIN_FOX "nitro:/audio/bgm_win_fox_ima.bin"
#define NDS_AUDIO_BGM_PATH_RESULTS "nitro:/audio/bgm_results_ima.bin"
#define NDS_AUDIO_BGM_PATH_MODE_SELECT "nitro:/audio/bgm_mode_select_ima.bin"
#define NDS_AUDIO_BGM_PATH_BATTLE_SELECT "nitro:/audio/bgm_battle_select_ima.bin"
#if NDS_P2_STAGE_YOSTER
#define NDS_AUDIO_BGM_PATH_YOSTER "nitro:/audio/bgm_yoster_ima.bin"
#endif
#if NDS_P2_STAGE_CASTLE
#define NDS_AUDIO_BGM_PATH_CASTLE "nitro:/audio/bgm_castle_ima.bin"
#endif
#if NDS_P2_STAGE_ZEBES
#define NDS_AUDIO_BGM_PATH_ZEBES "nitro:/audio/bgm_zebes_ima.bin"
#endif
#if NDS_P2_STAGE_HYRULE
#define NDS_AUDIO_BGM_PATH_HYRULE "nitro:/audio/bgm_hyrule_ima.bin"
#endif
#if NDS_P2_STAGE_YAMABUKI
#define NDS_AUDIO_BGM_PATH_YAMABUKI "nitro:/audio/bgm_yamabuki_ima.bin"
#endif
#if NDS_P2_STAGE_INISHIE
#define NDS_AUDIO_BGM_PATH_INISHIE "nitro:/audio/bgm_inishie_ima.bin"
#endif
#if NDS_P2_STAGE_SECTOR
#define NDS_AUDIO_BGM_PATH_SECTOR "nitro:/audio/bgm_sector_ima.bin"
#endif
#if NDS_P2_STAGE_JUNGLE
#define NDS_AUDIO_BGM_PATH_JUNGLE "nitro:/audio/bgm_jungle_ima.bin"
#endif
#if NDS_P2_STAGE_INISHIE
#define NDS_AUDIO_BGM_PATH_INISHIE_HURRY "nitro:/audio/bgm_inishie_hurry_ima.bin"
#endif
#define NDS_AUDIO_BGM_PATH_WIN_DEFAULT "nitro:/audio/bgm_win_default_ima.bin"
#define NDS_AUDIO_BGM_PATH_WIN_METROID "nitro:/audio/bgm_win_metroid_ima.bin"
#define NDS_AUDIO_BGM_PATH_WIN_DONKEY "nitro:/audio/bgm_win_donkey_ima.bin"
#define NDS_AUDIO_BGM_PATH_WIN_KIRBY "nitro:/audio/bgm_win_kirby_ima.bin"
#define NDS_AUDIO_BGM_PATH_WIN_MOTHER "nitro:/audio/bgm_win_mother_ima.bin"
#define NDS_AUDIO_BGM_PATH_WIN_YOSHI "nitro:/audio/bgm_win_yoshi_ima.bin"
#define NDS_AUDIO_BGM_PATH_WIN_FZERO "nitro:/audio/bgm_win_fzero_ima.bin"
#define NDS_AUDIO_BGM_PATH_WIN_PMONSTERS "nitro:/audio/bgm_win_pmonsters_ima.bin"
#define NDS_AUDIO_BGM_PATH_WIN_ZELDA "nitro:/audio/bgm_win_zelda_ima.bin"
#if NDS_P2_1P_GAME
#define NDS_AUDIO_BGM_PATH_BOSS_STAGE "nitro:/audio/bgm_boss_stage_ima.bin"
#endif
#if NDS_P2_1P_GAME
#define NDS_AUDIO_BGM_PATH_BOSS_ENTRY "nitro:/audio/bgm_boss_entry_ima.bin"
#endif
#if NDS_P2_1P_GAME
#define NDS_AUDIO_BGM_PATH_LAST "nitro:/audio/bgm_last_ima.bin"
#endif
#if NDS_P2_1P_GAME
#define NDS_AUDIO_BGM_PATH_BONUS_STAGE "nitro:/audio/bgm_bonus_stage_ima.bin"
#endif
#if NDS_P2_1P_GAME
#define NDS_AUDIO_BGM_PATH_STAGE_CLEAR "nitro:/audio/bgm_stage_clear_ima.bin"
#endif
#if NDS_P2_1P_GAME
#define NDS_AUDIO_BGM_PATH_BONUS_STAGE_CLEAR "nitro:/audio/bgm_bonus_stage_clear_ima.bin"
#endif
#if NDS_P2_1P_GAME
#define NDS_AUDIO_BGM_PATH_GAME_CLEAR "nitro:/audio/bgm_game_clear_ima.bin"
#endif
#if NDS_P2_1P_GAME
#define NDS_AUDIO_BGM_PATH_BONUS_STAGE_FAILURE "nitro:/audio/bgm_bonus_stage_failure_ima.bin"
#endif
#if NDS_P2_1P_GAME
#define NDS_AUDIO_BGM_PATH_GAME_END_CHOICE "nitro:/audio/bgm_game_end_choice_ima.bin"
#endif
#if NDS_P2_1P_GAME
#define NDS_AUDIO_BGM_PATH_GAME_OVER "nitro:/audio/bgm_game_over_ima.bin"
#endif
#if NDS_P2_1P_GAME
#define NDS_AUDIO_BGM_PATH_OPENING "nitro:/audio/bgm_opening_ima.bin"
#endif
#if NDS_P2_1P_GAME
#define NDS_AUDIO_BGM_PATH_EXPLAIN "nitro:/audio/bgm_explain_ima.bin"
#endif
#if NDS_P2_1P_GAME
#define NDS_AUDIO_BGM_PATH_INTRO "nitro:/audio/bgm_intro_ima.bin"
#endif
#if NDS_P2_1P_GAME
#define NDS_AUDIO_BGM_PATH_ZAKO "nitro:/audio/bgm_zako_ima.bin"
#endif
#if NDS_P2_1P_GAME
#define NDS_AUDIO_BGM_PATH_METAL "nitro:/audio/bgm_metal_ima.bin"
#endif
#if NDS_P2_1P_GAME
#define NDS_AUDIO_BGM_PATH_ENDING "nitro:/audio/bgm_ending_ima.bin"
#endif
#if NDS_P2_1P_GAME
#define NDS_AUDIO_BGM_PATH_STAFFROLL "nitro:/audio/bgm_staffroll_ima.bin"
#endif
#if NDS_P2_1P_GAME
#define NDS_AUDIO_BGM_PATH_MESSAGE "nitro:/audio/bgm_message_ima.bin"
#endif
#if NDS_P2_1P_GAME
#define NDS_AUDIO_BGM_PATH_CHALLENGER "nitro:/audio/bgm_challenger_ima.bin"
#endif
#if NDS_P2_1P_GAME
#define NDS_AUDIO_BGM_PATH_TRAINING_MODE "nitro:/audio/bgm_training_mode_ima.bin"
#endif
#if NDS_P2_1P_GAME
#define NDS_AUDIO_BGM_PATH_DATA "nitro:/audio/bgm_data_ima.bin"
#endif
#if NDS_P2_ITEM_CORE
#define NDS_AUDIO_BGM_PATH_HAMMER "nitro:/audio/bgm_hammer_ima.bin"
#endif
#if NDS_P2_ITEM_CORE
#define NDS_AUDIO_BGM_PATH_STAR "nitro:/audio/bgm_star_ima.bin"
#endif
#define NDS_AUDIO_BGM_NO_LOOP 0xffffffffu

_Static_assert(NDS_AUDIO_BGM_RESIDENT_BYTES == 16392u,
               "BGM ADPCM residency changed");
_Static_assert((NDS_AUDIO_BGM_PACKET_BYTES % 4u) == 0u,
               "BGM ADPCM packet must contain whole DS words");
_Static_assert(NDS_AUDIO_BGM_BYTES_PER_SECOND == 44100u,
               "BGM source-time byte rate changed");

typedef struct NDSAudioBgmTrack {
    s32 id;
    const char *path;
    u32 stream_bytes;
    u32 loop_start_bytes;
    u32 asset_bytes;
    u32 packet_count;
    u32 loop_packet;
    u32 loop_record;
    s32 is_looping;
} NDSAudioBgmTrack;

/* P2-1L bug (b1), 2026-08-19: [loop_start_bytes, stream_bytes) is played
 * forever once the file runs out of packets (ndsAudioBgmReadPacket()
 * below seeks to loop_record/loop_packet). Both fields, and every derived
 * *_LOOP_PACKET/*_LOOP_RECORD/*_PACKET_COUNT constant below, come only
 * from scripts/sfx/bgm/render-audio-bgm-pupupu.py's collect_loop_metadata()
 * -- see its docstring for why a naive max() over every CSEQ channel's
 * raw loop marker is wrong (Mode Select had an outlier channel dominate
 * it) and why the render used to leave up to a second of true digital
 * silence inside the loop for every track (measured offline, RMS 0.0
 * right before the wrap). Never hand-adjust these constants; re-render
 * and re-pin (include/nds/nds_audio_bgm.h,
 * scripts/check-audio-bgm-derived-assets.ps1) instead. */
static const NDSAudioBgmTrack sNdsAudioBgmTracks[] = {
    {
        nSYAudioBGMPupupu,
        NDS_AUDIO_BGM_PATH_PUPUPU,
        NDS_AUDIO_BGM_PUPUPU_STREAM_BYTES,
        NDS_AUDIO_BGM_PUPUPU_LOOP_START_BYTES,
        NDS_AUDIO_BGM_PUPUPU_ASSET_BYTES,
        NDS_AUDIO_BGM_PUPUPU_PACKET_COUNT,
        NDS_AUDIO_BGM_PUPUPU_LOOP_PACKET,
        NDS_AUDIO_BGM_PUPUPU_LOOP_RECORD,
        TRUE
    },
    {
        nSYAudioBGMWinMario,
        NDS_AUDIO_BGM_PATH_WIN_MARIO,
        NDS_AUDIO_BGM_WIN_MARIO_STREAM_BYTES,
        0u,
        NDS_AUDIO_BGM_WIN_MARIO_ASSET_BYTES,
        NDS_AUDIO_BGM_WIN_MARIO_PACKET_COUNT,
        NDS_AUDIO_BGM_NO_LOOP,
        0u,
        FALSE
    },
    {
        nSYAudioBGMWinFox,
        NDS_AUDIO_BGM_PATH_WIN_FOX,
        NDS_AUDIO_BGM_WIN_FOX_STREAM_BYTES,
        0u,
        NDS_AUDIO_BGM_WIN_FOX_ASSET_BYTES,
        NDS_AUDIO_BGM_WIN_FOX_PACKET_COUNT,
        NDS_AUDIO_BGM_NO_LOOP,
        0u,
        FALSE
    },
    {
        nSYAudioBGMResults,
        NDS_AUDIO_BGM_PATH_RESULTS,
        NDS_AUDIO_BGM_RESULTS_STREAM_BYTES,
        NDS_AUDIO_BGM_RESULTS_LOOP_START_BYTES,
        NDS_AUDIO_BGM_RESULTS_ASSET_BYTES,
        NDS_AUDIO_BGM_RESULTS_PACKET_COUNT,
        NDS_AUDIO_BGM_RESULTS_LOOP_PACKET,
        NDS_AUDIO_BGM_RESULTS_LOOP_RECORD,
        TRUE
    },
    {
        nSYAudioBGMModeSelect,
        NDS_AUDIO_BGM_PATH_MODE_SELECT,
        NDS_AUDIO_BGM_MODE_SELECT_STREAM_BYTES,
        NDS_AUDIO_BGM_MODE_SELECT_LOOP_START_BYTES,
        NDS_AUDIO_BGM_MODE_SELECT_ASSET_BYTES,
        NDS_AUDIO_BGM_MODE_SELECT_PACKET_COUNT,
        NDS_AUDIO_BGM_MODE_SELECT_LOOP_PACKET,
        NDS_AUDIO_BGM_MODE_SELECT_LOOP_RECORD,
        TRUE
    },
    {
        nSYAudioBGMBattleSelect,
        NDS_AUDIO_BGM_PATH_BATTLE_SELECT,
        NDS_AUDIO_BGM_BATTLE_SELECT_STREAM_BYTES,
        NDS_AUDIO_BGM_BATTLE_SELECT_LOOP_START_BYTES,
        NDS_AUDIO_BGM_BATTLE_SELECT_ASSET_BYTES,
        NDS_AUDIO_BGM_BATTLE_SELECT_PACKET_COUNT,
        NDS_AUDIO_BGM_BATTLE_SELECT_LOOP_PACKET,
        NDS_AUDIO_BGM_BATTLE_SELECT_LOOP_RECORD,
        TRUE
    }
#if NDS_P2_STAGE_YOSTER
    /* P2-4 Yoster BGM, rendered 2026-09-03 from music sequence 8. Its loop
     * starts at packet 10 rather than packet 1 because this track's channels
     * disagree on a period and the renderer takes the majority; pins are in
     * include/nds/nds_audio_bgm.h. */
    ,
    {
        nSYAudioBGMYoster,
        NDS_AUDIO_BGM_PATH_YOSTER,
        NDS_AUDIO_BGM_YOSTER_STREAM_BYTES,
        NDS_AUDIO_BGM_YOSTER_LOOP_START_BYTES,
        NDS_AUDIO_BGM_YOSTER_ASSET_BYTES,
        NDS_AUDIO_BGM_YOSTER_PACKET_COUNT,
        NDS_AUDIO_BGM_YOSTER_LOOP_PACKET,
        NDS_AUDIO_BGM_YOSTER_LOOP_RECORD,
        TRUE
    }
#endif
#if NDS_P2_STAGE_CASTLE
    /* P2-4 Castle BGM, rendered 2026-09-03 from music sequence 6. */
    ,
    {
        nSYAudioBGMCastle,
        NDS_AUDIO_BGM_PATH_CASTLE,
        NDS_AUDIO_BGM_CASTLE_STREAM_BYTES,
        NDS_AUDIO_BGM_CASTLE_LOOP_START_BYTES,
        NDS_AUDIO_BGM_CASTLE_ASSET_BYTES,
        NDS_AUDIO_BGM_CASTLE_PACKET_COUNT,
        NDS_AUDIO_BGM_CASTLE_LOOP_PACKET,
        NDS_AUDIO_BGM_CASTLE_LOOP_RECORD,
        TRUE
    }
#endif
#if NDS_P2_STAGE_ZEBES
    ,
    {
        nSYAudioBGMZebes,
        NDS_AUDIO_BGM_PATH_ZEBES,
        NDS_AUDIO_BGM_ZEBES_STREAM_BYTES,
        NDS_AUDIO_BGM_ZEBES_LOOP_START_BYTES,
        NDS_AUDIO_BGM_ZEBES_ASSET_BYTES,
        NDS_AUDIO_BGM_ZEBES_PACKET_COUNT,
        NDS_AUDIO_BGM_ZEBES_LOOP_PACKET,
        NDS_AUDIO_BGM_ZEBES_LOOP_RECORD,
        TRUE
    }
#endif
#if NDS_P2_STAGE_HYRULE
    ,
    {
        nSYAudioBGMHyrule,
        NDS_AUDIO_BGM_PATH_HYRULE,
        NDS_AUDIO_BGM_HYRULE_STREAM_BYTES,
        NDS_AUDIO_BGM_HYRULE_LOOP_START_BYTES,
        NDS_AUDIO_BGM_HYRULE_ASSET_BYTES,
        NDS_AUDIO_BGM_HYRULE_PACKET_COUNT,
        NDS_AUDIO_BGM_HYRULE_LOOP_PACKET,
        NDS_AUDIO_BGM_HYRULE_LOOP_RECORD,
        TRUE
    }
#endif
#if NDS_P2_STAGE_YAMABUKI
    ,
    {
        nSYAudioBGMYamabuki,
        NDS_AUDIO_BGM_PATH_YAMABUKI,
        NDS_AUDIO_BGM_YAMABUKI_STREAM_BYTES,
        NDS_AUDIO_BGM_YAMABUKI_LOOP_START_BYTES,
        NDS_AUDIO_BGM_YAMABUKI_ASSET_BYTES,
        NDS_AUDIO_BGM_YAMABUKI_PACKET_COUNT,
        NDS_AUDIO_BGM_YAMABUKI_LOOP_PACKET,
        NDS_AUDIO_BGM_YAMABUKI_LOOP_RECORD,
        TRUE
    }
#endif
#if NDS_P2_STAGE_INISHIE
    /* P2-4 Mushroom Kingdom BGM, music sequence 2: the same IMA packet stream
     * as every other track (owner, docs/BUGS.md Audio), trellis-encoded by
     * the renderer; pins in include/nds/nds_audio_bgm.h. */
    ,
    {
        nSYAudioBGMInishie,
        NDS_AUDIO_BGM_PATH_INISHIE,
        NDS_AUDIO_BGM_INISHIE_STREAM_BYTES,
        NDS_AUDIO_BGM_INISHIE_LOOP_START_BYTES,
        NDS_AUDIO_BGM_INISHIE_ASSET_BYTES,
        NDS_AUDIO_BGM_INISHIE_PACKET_COUNT,
        NDS_AUDIO_BGM_INISHIE_LOOP_PACKET,
        NDS_AUDIO_BGM_INISHIE_LOOP_RECORD,
        TRUE
    }
#endif
#if NDS_P2_STAGE_SECTOR
    /* P2-4 Sector BGM. Pins in include/nds/nds_audio_bgm.h. */
    ,
    {
        nSYAudioBGMSector,
        NDS_AUDIO_BGM_PATH_SECTOR,
        NDS_AUDIO_BGM_SECTOR_STREAM_BYTES,
        NDS_AUDIO_BGM_SECTOR_LOOP_START_BYTES,
        NDS_AUDIO_BGM_SECTOR_ASSET_BYTES,
        NDS_AUDIO_BGM_SECTOR_PACKET_COUNT,
        NDS_AUDIO_BGM_SECTOR_LOOP_PACKET,
        NDS_AUDIO_BGM_SECTOR_LOOP_RECORD,
        TRUE
    }
#endif
#if NDS_P2_STAGE_JUNGLE
    /* P2-4 Jungle BGM. Pins in include/nds/nds_audio_bgm.h. */
    ,
    {
        nSYAudioBGMJungle,
        NDS_AUDIO_BGM_PATH_JUNGLE,
        NDS_AUDIO_BGM_JUNGLE_STREAM_BYTES,
        NDS_AUDIO_BGM_JUNGLE_LOOP_START_BYTES,
        NDS_AUDIO_BGM_JUNGLE_ASSET_BYTES,
        NDS_AUDIO_BGM_JUNGLE_PACKET_COUNT,
        NDS_AUDIO_BGM_JUNGLE_LOOP_PACKET,
        NDS_AUDIO_BGM_JUNGLE_LOOP_RECORD,
        TRUE
    }
#endif
#if NDS_P2_STAGE_INISHIE
    /* nSYAudioBGMInishieHurry (sequence 3), the <=30-second swap; the same
     * trellis-encoded IMA packet stream as sequence 2. Pins in
     * nds_audio_bgm.h. */
    ,
    {
        nSYAudioBGMInishieHurry,
        NDS_AUDIO_BGM_PATH_INISHIE_HURRY,
        NDS_AUDIO_BGM_INISHIE_HURRY_STREAM_BYTES,
        NDS_AUDIO_BGM_INISHIE_HURRY_LOOP_START_BYTES,
        NDS_AUDIO_BGM_INISHIE_HURRY_ASSET_BYTES,
        NDS_AUDIO_BGM_INISHIE_HURRY_PACKET_COUNT,
        NDS_AUDIO_BGM_INISHIE_HURRY_LOOP_PACKET,
        NDS_AUDIO_BGM_INISHIE_HURRY_LOOP_RECORD,
        TRUE
    }
#endif
    /* nSYAudioBGMWinDefault (sequence 11), rendered 2026-09-05; pins in nds_audio_bgm.h. */
    ,
    {
        nSYAudioBGMWinDefault,
        NDS_AUDIO_BGM_PATH_WIN_DEFAULT,
        NDS_AUDIO_BGM_WIN_DEFAULT_STREAM_BYTES,
        0u,
        NDS_AUDIO_BGM_WIN_DEFAULT_ASSET_BYTES,
        NDS_AUDIO_BGM_WIN_DEFAULT_PACKET_COUNT,
        NDS_AUDIO_BGM_NO_LOOP,
        0u,
        FALSE
    }
    /* nSYAudioBGMWinMetroid (sequence 13), rendered 2026-09-05; pins in nds_audio_bgm.h. */
    ,
    {
        nSYAudioBGMWinMetroid,
        NDS_AUDIO_BGM_PATH_WIN_METROID,
        NDS_AUDIO_BGM_WIN_METROID_STREAM_BYTES,
        0u,
        NDS_AUDIO_BGM_WIN_METROID_ASSET_BYTES,
        NDS_AUDIO_BGM_WIN_METROID_PACKET_COUNT,
        NDS_AUDIO_BGM_NO_LOOP,
        0u,
        FALSE
    }
    /* nSYAudioBGMWinDonkey (sequence 14), rendered 2026-09-05; pins in nds_audio_bgm.h. */
    ,
    {
        nSYAudioBGMWinDonkey,
        NDS_AUDIO_BGM_PATH_WIN_DONKEY,
        NDS_AUDIO_BGM_WIN_DONKEY_STREAM_BYTES,
        0u,
        NDS_AUDIO_BGM_WIN_DONKEY_ASSET_BYTES,
        NDS_AUDIO_BGM_WIN_DONKEY_PACKET_COUNT,
        NDS_AUDIO_BGM_NO_LOOP,
        0u,
        FALSE
    }
    /* nSYAudioBGMWinKirby (sequence 15), rendered 2026-09-05; pins in nds_audio_bgm.h. */
    ,
    {
        nSYAudioBGMWinKirby,
        NDS_AUDIO_BGM_PATH_WIN_KIRBY,
        NDS_AUDIO_BGM_WIN_KIRBY_STREAM_BYTES,
        0u,
        NDS_AUDIO_BGM_WIN_KIRBY_ASSET_BYTES,
        NDS_AUDIO_BGM_WIN_KIRBY_PACKET_COUNT,
        NDS_AUDIO_BGM_NO_LOOP,
        0u,
        FALSE
    }
    /* nSYAudioBGMWinMother (sequence 17), rendered 2026-09-05; pins in nds_audio_bgm.h. */
    ,
    {
        nSYAudioBGMWinMother,
        NDS_AUDIO_BGM_PATH_WIN_MOTHER,
        NDS_AUDIO_BGM_WIN_MOTHER_STREAM_BYTES,
        0u,
        NDS_AUDIO_BGM_WIN_MOTHER_ASSET_BYTES,
        NDS_AUDIO_BGM_WIN_MOTHER_PACKET_COUNT,
        NDS_AUDIO_BGM_NO_LOOP,
        0u,
        FALSE
    }
    /* nSYAudioBGMWinYoshi (sequence 18), rendered 2026-09-05; pins in nds_audio_bgm.h. */
    ,
    {
        nSYAudioBGMWinYoshi,
        NDS_AUDIO_BGM_PATH_WIN_YOSHI,
        NDS_AUDIO_BGM_WIN_YOSHI_STREAM_BYTES,
        0u,
        NDS_AUDIO_BGM_WIN_YOSHI_ASSET_BYTES,
        NDS_AUDIO_BGM_WIN_YOSHI_PACKET_COUNT,
        NDS_AUDIO_BGM_NO_LOOP,
        0u,
        FALSE
    }
    /* nSYAudioBGMWinFZero (sequence 19), rendered 2026-09-05; pins in nds_audio_bgm.h. */
    ,
    {
        nSYAudioBGMWinFZero,
        NDS_AUDIO_BGM_PATH_WIN_FZERO,
        NDS_AUDIO_BGM_WIN_FZERO_STREAM_BYTES,
        0u,
        NDS_AUDIO_BGM_WIN_FZERO_ASSET_BYTES,
        NDS_AUDIO_BGM_WIN_FZERO_PACKET_COUNT,
        NDS_AUDIO_BGM_NO_LOOP,
        0u,
        FALSE
    }
    /* nSYAudioBGMWinPMonsters (sequence 20), rendered 2026-09-05; pins in nds_audio_bgm.h. */
    ,
    {
        nSYAudioBGMWinPMonsters,
        NDS_AUDIO_BGM_PATH_WIN_PMONSTERS,
        NDS_AUDIO_BGM_WIN_PMONSTERS_STREAM_BYTES,
        0u,
        NDS_AUDIO_BGM_WIN_PMONSTERS_ASSET_BYTES,
        NDS_AUDIO_BGM_WIN_PMONSTERS_PACKET_COUNT,
        NDS_AUDIO_BGM_NO_LOOP,
        0u,
        FALSE
    }
    /* nSYAudioBGMWinZelda (sequence 21), rendered 2026-09-05; pins in nds_audio_bgm.h. */
    ,
    {
        nSYAudioBGMWinZelda,
        NDS_AUDIO_BGM_PATH_WIN_ZELDA,
        NDS_AUDIO_BGM_WIN_ZELDA_STREAM_BYTES,
        0u,
        NDS_AUDIO_BGM_WIN_ZELDA_ASSET_BYTES,
        NDS_AUDIO_BGM_WIN_ZELDA_PACKET_COUNT,
        NDS_AUDIO_BGM_NO_LOOP,
        0u,
        FALSE
    }
#if NDS_P2_1P_GAME
    /* nSYAudioBGMBossStage (sequence 23), rendered 2026-09-05; pins in nds_audio_bgm.h. */
    ,
    {
        nSYAudioBGMBossStage,
        NDS_AUDIO_BGM_PATH_BOSS_STAGE,
        NDS_AUDIO_BGM_BOSS_STAGE_STREAM_BYTES,
        0u,
        NDS_AUDIO_BGM_BOSS_STAGE_ASSET_BYTES,
        NDS_AUDIO_BGM_BOSS_STAGE_PACKET_COUNT,
        NDS_AUDIO_BGM_NO_LOOP,
        0u,
        FALSE
    }
#endif
#if NDS_P2_1P_GAME
    /* nSYAudioBGMBossEntry (sequence 24), rendered 2026-09-05; pins in nds_audio_bgm.h. */
    ,
    {
        nSYAudioBGMBossEntry,
        NDS_AUDIO_BGM_PATH_BOSS_ENTRY,
        NDS_AUDIO_BGM_BOSS_ENTRY_STREAM_BYTES,
        0u,
        NDS_AUDIO_BGM_BOSS_ENTRY_ASSET_BYTES,
        NDS_AUDIO_BGM_BOSS_ENTRY_PACKET_COUNT,
        NDS_AUDIO_BGM_NO_LOOP,
        0u,
        FALSE
    }
#endif
#if NDS_P2_1P_GAME
    /* nSYAudioBGMLast (sequence 25), rendered 2026-09-05; pins in nds_audio_bgm.h. */
    ,
    {
        nSYAudioBGMLast,
        NDS_AUDIO_BGM_PATH_LAST,
        NDS_AUDIO_BGM_LAST_STREAM_BYTES,
        NDS_AUDIO_BGM_LAST_LOOP_START_BYTES,
        NDS_AUDIO_BGM_LAST_ASSET_BYTES,
        NDS_AUDIO_BGM_LAST_PACKET_COUNT,
        NDS_AUDIO_BGM_LAST_LOOP_PACKET,
        NDS_AUDIO_BGM_LAST_LOOP_RECORD,
        TRUE
    }
#endif
#if NDS_P2_1P_GAME
    /* nSYAudioBGM1PBonusStage (sequence 26), rendered 2026-09-05; pins in nds_audio_bgm.h. */
    ,
    {
        nSYAudioBGM1PBonusStage,
        NDS_AUDIO_BGM_PATH_BONUS_STAGE,
        NDS_AUDIO_BGM_BONUS_STAGE_STREAM_BYTES,
        NDS_AUDIO_BGM_BONUS_STAGE_LOOP_START_BYTES,
        NDS_AUDIO_BGM_BONUS_STAGE_ASSET_BYTES,
        NDS_AUDIO_BGM_BONUS_STAGE_PACKET_COUNT,
        NDS_AUDIO_BGM_BONUS_STAGE_LOOP_PACKET,
        NDS_AUDIO_BGM_BONUS_STAGE_LOOP_RECORD,
        TRUE
    }
#endif
#if NDS_P2_1P_GAME
    /* nSYAudioBGM1PStageClear (sequence 27), rendered 2026-09-05; pins in nds_audio_bgm.h. */
    ,
    {
        nSYAudioBGM1PStageClear,
        NDS_AUDIO_BGM_PATH_STAGE_CLEAR,
        NDS_AUDIO_BGM_STAGE_CLEAR_STREAM_BYTES,
        0u,
        NDS_AUDIO_BGM_STAGE_CLEAR_ASSET_BYTES,
        NDS_AUDIO_BGM_STAGE_CLEAR_PACKET_COUNT,
        NDS_AUDIO_BGM_NO_LOOP,
        0u,
        FALSE
    }
#endif
#if NDS_P2_1P_GAME
    /* nSYAudioBGM1PBonusStageClear (sequence 28), rendered 2026-09-05; pins in nds_audio_bgm.h. */
    ,
    {
        nSYAudioBGM1PBonusStageClear,
        NDS_AUDIO_BGM_PATH_BONUS_STAGE_CLEAR,
        NDS_AUDIO_BGM_BONUS_STAGE_CLEAR_STREAM_BYTES,
        0u,
        NDS_AUDIO_BGM_BONUS_STAGE_CLEAR_ASSET_BYTES,
        NDS_AUDIO_BGM_BONUS_STAGE_CLEAR_PACKET_COUNT,
        NDS_AUDIO_BGM_NO_LOOP,
        0u,
        FALSE
    }
#endif
#if NDS_P2_1P_GAME
    /* nSYAudioBGM1PGameClear (sequence 29), rendered 2026-09-05; pins in nds_audio_bgm.h. */
    ,
    {
        nSYAudioBGM1PGameClear,
        NDS_AUDIO_BGM_PATH_GAME_CLEAR,
        NDS_AUDIO_BGM_GAME_CLEAR_STREAM_BYTES,
        NDS_AUDIO_BGM_GAME_CLEAR_LOOP_START_BYTES,
        NDS_AUDIO_BGM_GAME_CLEAR_ASSET_BYTES,
        NDS_AUDIO_BGM_GAME_CLEAR_PACKET_COUNT,
        NDS_AUDIO_BGM_GAME_CLEAR_LOOP_PACKET,
        NDS_AUDIO_BGM_GAME_CLEAR_LOOP_RECORD,
        TRUE
    }
#endif
#if NDS_P2_1P_GAME
    /* nSYAudioBGM1PBonusStageFailure (sequence 30), rendered 2026-09-05; pins in nds_audio_bgm.h. */
    ,
    {
        nSYAudioBGM1PBonusStageFailure,
        NDS_AUDIO_BGM_PATH_BONUS_STAGE_FAILURE,
        NDS_AUDIO_BGM_BONUS_STAGE_FAILURE_STREAM_BYTES,
        0u,
        NDS_AUDIO_BGM_BONUS_STAGE_FAILURE_ASSET_BYTES,
        NDS_AUDIO_BGM_BONUS_STAGE_FAILURE_PACKET_COUNT,
        NDS_AUDIO_BGM_NO_LOOP,
        0u,
        FALSE
    }
#endif
#if NDS_P2_1P_GAME
    /* nSYAudioBGM1PGameEndChoice (sequence 31), rendered 2026-09-05; pins in nds_audio_bgm.h. */
    ,
    {
        nSYAudioBGM1PGameEndChoice,
        NDS_AUDIO_BGM_PATH_GAME_END_CHOICE,
        NDS_AUDIO_BGM_GAME_END_CHOICE_STREAM_BYTES,
        0u,
        NDS_AUDIO_BGM_GAME_END_CHOICE_ASSET_BYTES,
        NDS_AUDIO_BGM_GAME_END_CHOICE_PACKET_COUNT,
        NDS_AUDIO_BGM_NO_LOOP,
        0u,
        FALSE
    }
#endif
#if NDS_P2_1P_GAME
    /* nSYAudioBGM1PGameOver (sequence 32), rendered 2026-09-05; pins in nds_audio_bgm.h. */
    ,
    {
        nSYAudioBGM1PGameOver,
        NDS_AUDIO_BGM_PATH_GAME_OVER,
        NDS_AUDIO_BGM_GAME_OVER_STREAM_BYTES,
        0u,
        NDS_AUDIO_BGM_GAME_OVER_ASSET_BYTES,
        NDS_AUDIO_BGM_GAME_OVER_PACKET_COUNT,
        NDS_AUDIO_BGM_NO_LOOP,
        0u,
        FALSE
    }
#endif
#if NDS_P2_1P_GAME
    /* nSYAudioBGMOpening (sequence 33), rendered 2026-09-05; pins in nds_audio_bgm.h. */
    ,
    {
        nSYAudioBGMOpening,
        NDS_AUDIO_BGM_PATH_OPENING,
        NDS_AUDIO_BGM_OPENING_STREAM_BYTES,
        0u,
        NDS_AUDIO_BGM_OPENING_ASSET_BYTES,
        NDS_AUDIO_BGM_OPENING_PACKET_COUNT,
        NDS_AUDIO_BGM_NO_LOOP,
        0u,
        FALSE
    }
#endif
#if NDS_P2_1P_GAME
    /* nSYAudioBGMExplain (sequence 34), rendered 2026-09-05; pins in nds_audio_bgm.h. */
    ,
    {
        nSYAudioBGMExplain,
        NDS_AUDIO_BGM_PATH_EXPLAIN,
        NDS_AUDIO_BGM_EXPLAIN_STREAM_BYTES,
        NDS_AUDIO_BGM_EXPLAIN_LOOP_START_BYTES,
        NDS_AUDIO_BGM_EXPLAIN_ASSET_BYTES,
        NDS_AUDIO_BGM_EXPLAIN_PACKET_COUNT,
        NDS_AUDIO_BGM_EXPLAIN_LOOP_PACKET,
        NDS_AUDIO_BGM_EXPLAIN_LOOP_RECORD,
        TRUE
    }
#endif
#if NDS_P2_1P_GAME
    /* nSYAudioBGM1PIntro (sequence 35), rendered 2026-09-05; pins in nds_audio_bgm.h. */
    ,
    {
        nSYAudioBGM1PIntro,
        NDS_AUDIO_BGM_PATH_INTRO,
        NDS_AUDIO_BGM_INTRO_STREAM_BYTES,
        0u,
        NDS_AUDIO_BGM_INTRO_ASSET_BYTES,
        NDS_AUDIO_BGM_INTRO_PACKET_COUNT,
        NDS_AUDIO_BGM_NO_LOOP,
        0u,
        FALSE
    }
#endif
#if NDS_P2_1P_GAME
    /* nSYAudioBGMZako (sequence 36), rendered 2026-09-05; pins in nds_audio_bgm.h. */
    ,
    {
        nSYAudioBGMZako,
        NDS_AUDIO_BGM_PATH_ZAKO,
        NDS_AUDIO_BGM_ZAKO_STREAM_BYTES,
        NDS_AUDIO_BGM_ZAKO_LOOP_START_BYTES,
        NDS_AUDIO_BGM_ZAKO_ASSET_BYTES,
        NDS_AUDIO_BGM_ZAKO_PACKET_COUNT,
        NDS_AUDIO_BGM_ZAKO_LOOP_PACKET,
        NDS_AUDIO_BGM_ZAKO_LOOP_RECORD,
        TRUE
    }
#endif
#if NDS_P2_1P_GAME
    /* nSYAudioBGMMetal (sequence 37), rendered 2026-09-05; pins in nds_audio_bgm.h. */
    ,
    {
        nSYAudioBGMMetal,
        NDS_AUDIO_BGM_PATH_METAL,
        NDS_AUDIO_BGM_METAL_STREAM_BYTES,
        NDS_AUDIO_BGM_METAL_LOOP_START_BYTES,
        NDS_AUDIO_BGM_METAL_ASSET_BYTES,
        NDS_AUDIO_BGM_METAL_PACKET_COUNT,
        NDS_AUDIO_BGM_METAL_LOOP_PACKET,
        NDS_AUDIO_BGM_METAL_LOOP_RECORD,
        TRUE
    }
#endif
#if NDS_P2_1P_GAME
    /* nSYAudioBGMEnding (sequence 38), rendered 2026-09-05; pins in nds_audio_bgm.h. */
    ,
    {
        nSYAudioBGMEnding,
        NDS_AUDIO_BGM_PATH_ENDING,
        NDS_AUDIO_BGM_ENDING_STREAM_BYTES,
        0u,
        NDS_AUDIO_BGM_ENDING_ASSET_BYTES,
        NDS_AUDIO_BGM_ENDING_PACKET_COUNT,
        NDS_AUDIO_BGM_NO_LOOP,
        0u,
        FALSE
    }
#endif
#if NDS_P2_1P_GAME
    /* nSYAudioBGMStaffroll (sequence 39), rendered 2026-09-05; pins in nds_audio_bgm.h. */
    ,
    {
        nSYAudioBGMStaffroll,
        NDS_AUDIO_BGM_PATH_STAFFROLL,
        NDS_AUDIO_BGM_STAFFROLL_STREAM_BYTES,
        NDS_AUDIO_BGM_STAFFROLL_LOOP_START_BYTES,
        NDS_AUDIO_BGM_STAFFROLL_ASSET_BYTES,
        NDS_AUDIO_BGM_STAFFROLL_PACKET_COUNT,
        NDS_AUDIO_BGM_STAFFROLL_LOOP_PACKET,
        NDS_AUDIO_BGM_STAFFROLL_LOOP_RECORD,
        TRUE
    }
#endif
#if NDS_P2_1P_GAME
    /* nSYAudioBGMMessage (sequence 40), rendered 2026-09-05; pins in nds_audio_bgm.h. */
    ,
    {
        nSYAudioBGMMessage,
        NDS_AUDIO_BGM_PATH_MESSAGE,
        NDS_AUDIO_BGM_MESSAGE_STREAM_BYTES,
        0u,
        NDS_AUDIO_BGM_MESSAGE_ASSET_BYTES,
        NDS_AUDIO_BGM_MESSAGE_PACKET_COUNT,
        NDS_AUDIO_BGM_NO_LOOP,
        0u,
        FALSE
    }
#endif
#if NDS_P2_1P_GAME
    /* nSYAudioBGM1PChallenger (sequence 41), rendered 2026-09-05; pins in nds_audio_bgm.h. */
    ,
    {
        nSYAudioBGM1PChallenger,
        NDS_AUDIO_BGM_PATH_CHALLENGER,
        NDS_AUDIO_BGM_CHALLENGER_STREAM_BYTES,
        0u,
        NDS_AUDIO_BGM_CHALLENGER_ASSET_BYTES,
        NDS_AUDIO_BGM_CHALLENGER_PACKET_COUNT,
        NDS_AUDIO_BGM_NO_LOOP,
        0u,
        FALSE
    }
#endif
#if NDS_P2_1P_GAME
    /* nSYAudioBGMTrainingMode (sequence 42), rendered 2026-09-05; pins in nds_audio_bgm.h. */
    ,
    {
        nSYAudioBGMTrainingMode,
        NDS_AUDIO_BGM_PATH_TRAINING_MODE,
        NDS_AUDIO_BGM_TRAINING_MODE_STREAM_BYTES,
        NDS_AUDIO_BGM_TRAINING_MODE_LOOP_START_BYTES,
        NDS_AUDIO_BGM_TRAINING_MODE_ASSET_BYTES,
        NDS_AUDIO_BGM_TRAINING_MODE_PACKET_COUNT,
        NDS_AUDIO_BGM_TRAINING_MODE_LOOP_PACKET,
        NDS_AUDIO_BGM_TRAINING_MODE_LOOP_RECORD,
        TRUE
    }
#endif
#if NDS_P2_1P_GAME
    /* nSYAudioBGMData (sequence 43), rendered 2026-09-05; pins in nds_audio_bgm.h. */
    ,
    {
        nSYAudioBGMData,
        NDS_AUDIO_BGM_PATH_DATA,
        NDS_AUDIO_BGM_DATA_STREAM_BYTES,
        NDS_AUDIO_BGM_DATA_LOOP_START_BYTES,
        NDS_AUDIO_BGM_DATA_ASSET_BYTES,
        NDS_AUDIO_BGM_DATA_PACKET_COUNT,
        NDS_AUDIO_BGM_DATA_LOOP_PACKET,
        NDS_AUDIO_BGM_DATA_LOOP_RECORD,
        TRUE
    }
#endif
#if NDS_P2_ITEM_CORE
    /* nSYAudioBGMHammer (sequence 45), rendered 2026-09-05; pins in nds_audio_bgm.h. */
    ,
    {
        nSYAudioBGMHammer,
        NDS_AUDIO_BGM_PATH_HAMMER,
        NDS_AUDIO_BGM_HAMMER_STREAM_BYTES,
        NDS_AUDIO_BGM_HAMMER_LOOP_START_BYTES,
        NDS_AUDIO_BGM_HAMMER_ASSET_BYTES,
        NDS_AUDIO_BGM_HAMMER_PACKET_COUNT,
        NDS_AUDIO_BGM_HAMMER_LOOP_PACKET,
        NDS_AUDIO_BGM_HAMMER_LOOP_RECORD,
        TRUE
    }
#endif
#if NDS_P2_ITEM_CORE
    /* nSYAudioBGMStar (sequence 46), rendered 2026-09-05; pins in nds_audio_bgm.h. */
    ,
    {
        nSYAudioBGMStar,
        NDS_AUDIO_BGM_PATH_STAR,
        NDS_AUDIO_BGM_STAR_STREAM_BYTES,
        NDS_AUDIO_BGM_STAR_LOOP_START_BYTES,
        NDS_AUDIO_BGM_STAR_ASSET_BYTES,
        NDS_AUDIO_BGM_STAR_PACKET_COUNT,
        NDS_AUDIO_BGM_STAR_LOOP_PACKET,
        NDS_AUDIO_BGM_STAR_LOOP_RECORD,
        TRUE
    }
#endif
};

volatile u32 gNdsAudioBgmResult;
volatile u32 gNdsAudioBgmMask;
volatile u32 gNdsAudioBgmPlaying;
volatile u32 gNdsAudioBgmTrackID;
volatile u32 gNdsAudioBgmVolume;
volatile u32 gNdsAudioBgmPlayCalls;
volatile u32 gNdsAudioBgmStopCalls;
volatile u32 gNdsAudioBgmCheckCalls;
volatile u32 gNdsAudioBgmSetVolumeCalls;
volatile u32 gNdsAudioBgmOpenFailCount;
volatile u32 gNdsAudioBgmReadFailCount;
volatile u32 gNdsAudioBgmUnsupportedTrackCount;
volatile u32 gNdsAudioBgmReadBytes;
volatile u32 gNdsAudioBgmResidentBytes;
volatile u32 gNdsAudioBgmChunkBytes;
volatile u32 gNdsAudioBgmChunkPlayCount;
volatile u32 gNdsAudioBgmStoppedOnTeardown;
volatile u32 gNdsAudioBgmElapsedFrames;
volatile u32 gNdsAudioBgmStreamedBytes;
volatile u32 gNdsAudioBgmStreamBytesPerSecond;
volatile u32 gNdsAudioBgmExpectedBytesPerSecond;
volatile u32 gNdsAudioBgmLoopCount;
volatile u32 gNdsAudioBgmRefillCount;
__attribute__((used)) volatile u32 gNdsAudioBgmDirectReadCount;
__attribute__((used)) volatile u32 gNdsAudioBgmDirectFallbackCount;
#if NDS_RENDERER_PROFILE_LEVEL >= 1
volatile u32 gNdsAudioBgmRefillTicksLast;
volatile u32 gNdsAudioBgmRefillTicksMax;
#endif
volatile u32 gNdsAudioBgmPlaybackPositionBytes;
volatile u32 gNdsAudioBgmWritePositionBytes;
volatile u32 gNdsAudioBgmPlaybackHalf;
volatile u32 gNdsAudioBgmWriteHalf;
volatile u32 gNdsAudioBgmUnsafeWriteCount;
volatile u32 gNdsAudioBgmTimerTicks;
volatile u32 gNdsAudioBgmPlaybackBytes;
volatile u32 gNdsAudioBgmPlaybackLoopCount;
volatile u32 gNdsAudioBgmOverrunCount;
volatile u32 gNdsAudioBgmStreamBytes;
volatile u32 gNdsAudioBgmLoopStartBytes;
volatile u32 gNdsAudioBgmIsLooping;
volatile u32 gNdsAudioBgmPupupuPlayCount;
volatile u32 gNdsAudioBgmWinMarioPlayCount;
volatile u32 gNdsAudioBgmWinFoxPlayCount;
volatile u32 gNdsAudioBgmResultsPlayCount;
volatile u32 gNdsAudioBgmModeSelectPlayCount;
volatile u32 gNdsAudioBgmBattleSelectPlayCount;
#if NDS_P2_STAGE_YOSTER
volatile u32 gNdsAudioBgmYosterPlayCount;
#endif
#if NDS_P2_STAGE_CASTLE
volatile u32 gNdsAudioBgmCastlePlayCount;
#endif
volatile u32 gNdsAudioBgmNaturalStopCount;
volatile u32 gNdsAudioBgmLastNaturalStopTrackID;
volatile u32 gNdsAudioBgmPostNaturalTransitionCount;
volatile u32 gNdsAudioBgmPostNaturalTransitionFromTrackID;
volatile u32 gNdsAudioBgmPostNaturalTransitionToTrackID;
volatile u32 gNdsAudioBgmTrackSwitchCount;
volatile u32 gNdsAudioBgmFinitePaddingBytes;
volatile u32 gNdsAudioBgmFileOpen;
volatile u32 gNdsAudioBgmSoundActive;
volatile u32 gNdsAudioBgmPlayFailCount;
volatile u32 gNdsAudioBgmHeaderFailCount;
volatile u32 gNdsAudioBgmPacketFailCount;
volatile u32 gNdsAudioBgmPreparedCount;
volatile u32 gNdsAudioBgmSeamStartCount;
volatile u32 gNdsAudioBgmSeamMissCount;
volatile u32 gNdsAudioBgmTimerEventDropCount;
volatile u32 gNdsAudioBgmWorkerWakeCount;
volatile u32 gNdsAudioBgmErrorStopCount;
volatile u32 gNdsAudioBgmErrorCleanupFailCount;
volatile u32 gNdsAudioBgmFirstMissFrame;
volatile u32 gNdsAudioBgmFirstMissTrackID;
extern volatile u32 gNdsRendererProfileFrameCount;

/* decomp sys/audio.c:76. Exported current sound quality the Options menu reads
 * (mnoption.c:808): 0 = mono, 1 = stereo. Default stereo, like the source. */
sb32 dSYAudioSoundQuality = 1;

/* ARM9 owns source commands and frame-counted fades. ARM7 owns all packet
 * reads, buffers, sound-channel timing and natural completion. */
static NdsBgmSpec sBgmSpecs[sizeof(sNdsAudioBgmTracks)/sizeof(sNdsAudioBgmTracks[0])]
    __attribute__((aligned(32)));
static volatile NdsBgmReport sBgmReport;
static NdsBgmInit sBgmInit;
static Mutex sBgmCommandMutex;
static u32 sBgmInitialized, sBgmGeneration, sBgmPendingPlaying, sBgmLastSequence;
static s32 sNdsAudioBgmNaturalStopArmed;
static u32 sNdsAudioBgmFadeFramesLeft, sNdsAudioBgmFadeTarget, sNdsAudioBgmFadeDenom;
static s32 sNdsAudioBgmFadeStep, sNdsAudioBgmFadeRem, sNdsAudioBgmFadeError;
volatile u32 gNdsAudioBgmArm7Commands;
volatile u32 gNdsAudioBgmArm7Ready;
volatile u32 gNdsAudioBgmArm7Failure;

_Static_assert(sizeof(sBgmSpecs)/sizeof(sBgmSpecs[0]) <= NDS_BGM_MAX_TRACKS, "BGM index encoding");
/* soundPlaySample scans upward from channel 0. These are its only callers,
 * so twelve FGM handles cannot claim the two BGM channels at 14/15. */
_Static_assert(NDS_AUDIO_FGM_HANDLE_CAPACITY <= 14u, "FGM overlaps BGM channels");

void __attribute__((noinline, used, noreturn)) ndsAudioBgmControlHalt(u32 reason)
{
    gNdsAudioBgmArm7Failure = reason;
    DC_FlushAll();
    for (;;) { __asm__ volatile("" ::: "memory"); }
}

static u32 ndsBgmNextGeneration(void)
{
    sBgmGeneration = (sBgmGeneration + 1u) & NDS_BGM_GENERATION_MASK;
    if (!sBgmGeneration) sBgmGeneration = 1u;
    return sBgmGeneration;
}

static void ndsBgmPost(u32 operation, u32 argument, u32 wait)
{
    u32 message = ndsBgmCommand(operation, argument);
    mutexLock(&sBgmCommandMutex);
    gNdsAudioBgmArm7Commands++;
    if (wait)
    {
        if (pxiSendAndReceive((PxiChannel)NDS_BGM_IPC_CHANNEL, message) != message)
            ndsAudioBgmControlHalt(4u);
    }
    else pxiSend((PxiChannel)NDS_BGM_IPC_CHANNEL, message);
    mutexUnlock(&sBgmCommandMutex);
}

static void ndsBgmInitialize(void)
{
    if (sBgmInitialized) return;
    NitroRom *rom = nitroromGetSelf();
    if (!rom) ndsAudioBgmControlHalt(1u);
    for (u32 i = 0; i < sizeof(sBgmSpecs)/sizeof(sBgmSpecs[0]); ++i)
    {
        const NDSAudioBgmTrack *track = &sNdsAudioBgmTracks[i];
        if (strncmp(track->path, "nitro:/", 7u)) ndsAudioBgmControlHalt(2u);
        int file = nitroromResolvePath(rom, NITROROM_ROOT_DIR, track->path + 7u);
        if (file < 0 || file >= NITROROM_ROOT_DIR ||
            nitroromGetFileSize(rom, (u16)file) != track->asset_bytes)
            ndsAudioBgmControlHalt(3u);
        sBgmSpecs[i] = (NdsBgmSpec){
            nitroromGetFileOffset(rom, (u16)file), track->asset_bytes,
            track->stream_bytes / 2u,
            track->is_looping ? track->loop_start_bytes / 2u : NDS_BGM_NO_LOOP,
            track->packet_count, track->loop_packet, track->loop_record, (u32)track->id
        };
    }
    sBgmInit = (NdsBgmInit){
        NDS_BGM_IPC_ABI, (u32)(uintptr_t)sBgmSpecs,
        sizeof(sBgmSpecs)/sizeof(sBgmSpecs[0]), (u32)(uintptr_t)&sBgmReport,
        gNdsAudioBgmVolume > 0x7800u ? 0x7800u : gNdsAudioBgmVolume, {0,0,0}
    };
    DC_FlushRange(sBgmSpecs, sizeof(sBgmSpecs));
    DC_FlushRange((void *)&sBgmReport, sizeof(sBgmReport));
    DC_FlushRange(&sBgmInit, sizeof(sBgmInit));
    pxiWaitRemote((PxiChannel)NDS_BGM_IPC_CHANNEL);
    ndsBgmPost(NDS_BGM_INIT, (u32)(uintptr_t)&sBgmInit >> 5, TRUE);
    sBgmInitialized = 1u;
    gNdsAudioBgmArm7Ready = 1u;
}

static void ndsBgmObserve(void)
{
    if (!sBgmInitialized) return;
    DC_InvalidateRange((void *)&sBgmReport, 32u);
    u32 control = sBgmReport.control;
    u32 state = control >> 17;
    gNdsAudioBgmPlaying = (control & NDS_BGM_GENERATION_MASK) == sBgmGeneration ?
        (state == NDS_BGM_STARTING || state == NDS_BGM_PLAYING) : sBgmPendingPlaying;
    if ((sBgmReport.sequence & 1u) || sBgmReport.sequence == sBgmLastSequence) return;
    NdsBgmReport snapshot;
    u32 before;
    do
    {
        DC_InvalidateRange((void *)&sBgmReport, sizeof(sBgmReport));
        before = sBgmReport.sequence;
        if (before & 1u) continue;
        snapshot = sBgmReport;
        /* Re-read the sequence from RAM, not the cache line fetched above. */
        DC_InvalidateRange((void *)&sBgmReport, 32u);
    } while ((before & 1u) || before != sBgmReport.sequence);
    sBgmLastSequence = before;
    gNdsAudioBgmReadBytes = snapshot.read_bytes;
    gNdsAudioBgmDirectReadCount = snapshot.reads;
    gNdsAudioBgmRefillCount = snapshot.refills;
    gNdsAudioBgmPreparedCount = snapshot.prepared;
    gNdsAudioBgmChunkPlayCount = snapshot.chunks;
    gNdsAudioBgmSeamStartCount = snapshot.seams;
    gNdsAudioBgmLoopCount = snapshot.loops_loaded;
    gNdsAudioBgmPlaybackLoopCount = snapshot.loops_played;
    gNdsAudioBgmSeamMissCount = snapshot.seam_misses;
    gNdsAudioBgmOverrunCount = snapshot.seam_misses;
    gNdsAudioBgmTimerEventDropCount = snapshot.event_drops;
    gNdsAudioBgmWorkerWakeCount = snapshot.seams;
    gNdsAudioBgmHeaderFailCount = snapshot.header_errors;
    gNdsAudioBgmPacketFailCount = snapshot.packet_errors;
    gNdsAudioBgmReadFailCount = snapshot.read_errors;
    gNdsAudioBgmErrorStopCount = snapshot.error_stops;
    gNdsAudioBgmPlayFailCount = snapshot.bad_commands;
    gNdsAudioBgmNaturalStopCount = snapshot.natural_stops;
    gNdsAudioBgmLastNaturalStopTrackID = snapshot.last_natural_track;
    if (snapshot.error_stops) ndsAudioBgmControlHalt(5u);
    if (snapshot.seam_misses && !gNdsAudioBgmFirstMissFrame)
    {
        gNdsAudioBgmFirstMissFrame = gNdsRendererProfileFrameCount + 1u;
        gNdsAudioBgmFirstMissTrackID = snapshot.track_id;
    }
#if NDS_RENDERER_PROFILE_LEVEL >= 1
    gNdsAudioBgmRefillTicksLast = snapshot.refill_ticks_last;
    gNdsAudioBgmRefillTicksMax = snapshot.refill_ticks_max;
#endif
    if ((snapshot.control & NDS_BGM_GENERATION_MASK) != sBgmGeneration) return;
    state = snapshot.control >> 17;
    gNdsAudioBgmPlaying = state == NDS_BGM_STARTING || state == NDS_BGM_PLAYING;
    gNdsAudioBgmSoundActive = state == NDS_BGM_PLAYING;
    /* Legacy field denotes an owned stream, with no open FILE on ARM9. */
    gNdsAudioBgmFileOpen = gNdsAudioBgmPlaying;
    if (state == NDS_BGM_NATURAL) { sNdsAudioBgmNaturalStopArmed = TRUE; gNdsAudioBgmMask |= 1u << 5; }
    gNdsAudioBgmResidentBytes = NDS_AUDIO_BGM_RESIDENT_BYTES;
    gNdsAudioBgmPlaybackHalf = snapshot.current_buffer;
    gNdsAudioBgmWriteHalf = snapshot.current_buffer ^ 1u;
    gNdsAudioBgmWritePositionBytes = gNdsAudioBgmWriteHalf * NDS_AUDIO_BGM_PACKET_BYTES;
    gNdsAudioBgmChunkBytes = snapshot.current_bytes;
#if NDS_SHIP_TELEMETRY
    u64 ticks = ((u64)snapshot.elapsed_ticks_hi << 32) | snapshot.elapsed_ticks_lo;
    u64 played = ticks * NDS_AUDIO_BGM_BYTES_PER_SECOND / BUS_CLOCK;
    u64 loaded = ((u64)snapshot.loaded_samples_hi << 32) | snapshot.loaded_samples_lo;
    gNdsAudioBgmStreamedBytes = loaded >= snapshot.initial_loaded_samples ?
        (u32)((loaded - snapshot.initial_loaded_samples) * 2u) : 0u;
    gNdsAudioBgmTimerTicks = (u32)ticks;
    gNdsAudioBgmPlaybackBytes = (u32)played;
    gNdsAudioBgmStreamBytesPerSecond = ticks ? (u32)(played * BUS_CLOCK / ticks) : 0u;
    if (played < gNdsAudioBgmStreamBytes) gNdsAudioBgmPlaybackPositionBytes = (u32)played;
    else if (!gNdsAudioBgmIsLooping) gNdsAudioBgmPlaybackPositionBytes = gNdsAudioBgmStreamBytes;
    else gNdsAudioBgmPlaybackPositionBytes = gNdsAudioBgmLoopStartBytes +
        (u32)((played - gNdsAudioBgmStreamBytes) %
              (gNdsAudioBgmStreamBytes - gNdsAudioBgmLoopStartBytes));
#endif
}

void ndsAudioBgmDiagnosticsReset(void)
{
    if (sBgmInitialized) ndsBgmPost(NDS_BGM_RESET, ndsBgmNextGeneration(), TRUE);
    sBgmPendingPlaying = sBgmLastSequence = 0u;
    sNdsAudioBgmNaturalStopArmed = FALSE;
    sNdsAudioBgmFadeFramesLeft = sNdsAudioBgmFadeTarget = sNdsAudioBgmFadeDenom = 0u;
    sNdsAudioBgmFadeStep = sNdsAudioBgmFadeRem = sNdsAudioBgmFadeError = 0;
    gNdsAudioBgmResult = 0u;
    gNdsAudioBgmMask = 0u;
    gNdsAudioBgmPlaying = 0u;
    gNdsAudioBgmTrackID = 0u;
    gNdsAudioBgmVolume = 0x7800u;
    gNdsAudioBgmPlayCalls = 0u;
    gNdsAudioBgmStopCalls = 0u;
    gNdsAudioBgmCheckCalls = 0u;
    gNdsAudioBgmSetVolumeCalls = 0u;
    gNdsAudioBgmOpenFailCount = 0u;
    gNdsAudioBgmReadFailCount = 0u;
    gNdsAudioBgmUnsupportedTrackCount = 0u;
    gNdsAudioBgmReadBytes = 0u;
    gNdsAudioBgmResidentBytes = 0u;
    gNdsAudioBgmChunkBytes = 0u;
    gNdsAudioBgmChunkPlayCount = 0u;
    gNdsAudioBgmStoppedOnTeardown = 0u;
    gNdsAudioBgmElapsedFrames = 0u;
    gNdsAudioBgmStreamedBytes = 0u;
    gNdsAudioBgmStreamBytesPerSecond = 0u;
    gNdsAudioBgmExpectedBytesPerSecond = NDS_AUDIO_BGM_BYTES_PER_SECOND;
    gNdsAudioBgmLoopCount = 0u;
    gNdsAudioBgmRefillCount = 0u;
    gNdsAudioBgmDirectReadCount = 0u;
    gNdsAudioBgmDirectFallbackCount = 0u;
#if NDS_RENDERER_PROFILE_LEVEL >= 1
    gNdsAudioBgmRefillTicksLast = 0u;
    gNdsAudioBgmRefillTicksMax = 0u;
#endif
    gNdsAudioBgmPlaybackPositionBytes = 0u;
    gNdsAudioBgmWritePositionBytes = 0u;
    gNdsAudioBgmPlaybackHalf = 0u;
    gNdsAudioBgmWriteHalf = 0u;
    gNdsAudioBgmUnsafeWriteCount = 0u;
    gNdsAudioBgmTimerTicks = 0u;
    gNdsAudioBgmPlaybackBytes = 0u;
    gNdsAudioBgmPlaybackLoopCount = 0u;
    gNdsAudioBgmOverrunCount = 0u;
    gNdsAudioBgmStreamBytes = 0u;
    gNdsAudioBgmLoopStartBytes = 0u;
    gNdsAudioBgmIsLooping = 0u;
    gNdsAudioBgmPupupuPlayCount = 0u;
    gNdsAudioBgmWinMarioPlayCount = 0u;
    gNdsAudioBgmWinFoxPlayCount = 0u;
    gNdsAudioBgmResultsPlayCount = 0u;
#if NDS_P2_STAGE_YOSTER
    gNdsAudioBgmYosterPlayCount = 0u;
#endif
#if NDS_P2_STAGE_CASTLE
    gNdsAudioBgmCastlePlayCount = 0u;
#endif
    gNdsAudioBgmNaturalStopCount = 0u;
    gNdsAudioBgmLastNaturalStopTrackID = NDS_AUDIO_BGM_NO_LOOP;
    gNdsAudioBgmPostNaturalTransitionCount = 0u;
    gNdsAudioBgmPostNaturalTransitionFromTrackID = NDS_AUDIO_BGM_NO_LOOP;
    gNdsAudioBgmPostNaturalTransitionToTrackID = NDS_AUDIO_BGM_NO_LOOP;
    gNdsAudioBgmTrackSwitchCount = 0u;
    gNdsAudioBgmFinitePaddingBytes = 0u;
    gNdsAudioBgmFileOpen = 0u;
    gNdsAudioBgmSoundActive = 0u;
    gNdsAudioBgmPlayFailCount = 0u;
    gNdsAudioBgmHeaderFailCount = 0u;
    gNdsAudioBgmPacketFailCount = 0u;
    gNdsAudioBgmPreparedCount = 0u;
    gNdsAudioBgmSeamStartCount = 0u;
    gNdsAudioBgmSeamMissCount = 0u;
    gNdsAudioBgmTimerEventDropCount = 0u;
    gNdsAudioBgmWorkerWakeCount = 0u;
    gNdsAudioBgmErrorStopCount = 0u;
    gNdsAudioBgmErrorCleanupFailCount = 0u;
    if (sBgmInitialized) ndsBgmPost(NDS_BGM_VOLUME, 0x7800u, FALSE);
}

void ndsAudioBgmPlay(s32 player, s32 bgm_id)
{
    (void)player;
    ndsBgmObserve();
    gNdsAudioBgmPlayCalls++;
    u32 index;
    for (index = 0; index < sizeof(sBgmSpecs)/sizeof(sBgmSpecs[0]); ++index)
        if (sNdsAudioBgmTracks[index].id == bgm_id) break;
    if (index == sizeof(sBgmSpecs)/sizeof(sBgmSpecs[0]))
    {
        gNdsAudioBgmUnsupportedTrackCount++;
        return;
    }
    const NDSAudioBgmTrack *track = &sNdsAudioBgmTracks[index];
    if (gNdsAudioBgmPlaying && gNdsAudioBgmTrackID != (u32)bgm_id) gNdsAudioBgmTrackSwitchCount++;
    if (sNdsAudioBgmNaturalStopArmed)
    {
        gNdsAudioBgmPostNaturalTransitionCount++;
        gNdsAudioBgmPostNaturalTransitionFromTrackID = gNdsAudioBgmLastNaturalStopTrackID;
        gNdsAudioBgmPostNaturalTransitionToTrackID = (u32)bgm_id;
        sNdsAudioBgmNaturalStopArmed = FALSE;
    }
    gNdsAudioBgmTrackID = (u32)bgm_id;
    gNdsAudioBgmStreamBytes = track->stream_bytes;
    gNdsAudioBgmLoopStartBytes = track->loop_start_bytes;
    gNdsAudioBgmIsLooping = track->is_looping != FALSE;
    switch (bgm_id)
    {
    case nSYAudioBGMPupupu: gNdsAudioBgmPupupuPlayCount++; break;
    case nSYAudioBGMWinMario: gNdsAudioBgmWinMarioPlayCount++; break;
    case nSYAudioBGMWinFox: gNdsAudioBgmWinFoxPlayCount++; break;
    case nSYAudioBGMResults: gNdsAudioBgmResultsPlayCount++; break;
    case nSYAudioBGMModeSelect: gNdsAudioBgmModeSelectPlayCount++; break;
    case nSYAudioBGMBattleSelect: gNdsAudioBgmBattleSelectPlayCount++; break;
#if NDS_P2_STAGE_YOSTER
    case nSYAudioBGMYoster: gNdsAudioBgmYosterPlayCount++; break;
#endif
#if NDS_P2_STAGE_CASTLE
    case nSYAudioBGMCastle: gNdsAudioBgmCastlePlayCount++; break;
#endif
    }
    if (!gNdsAudioBgmSetVolumeCalls && !gNdsAudioBgmVolume) gNdsAudioBgmVolume = 0x7800u;
    soundEnable();
    ndsBgmInitialize();
    u32 generation = ndsBgmNextGeneration();
    sBgmPendingPlaying = 1u;
    gNdsAudioBgmPlaying = 1u;
    gNdsAudioBgmFileOpen = 1u;
    gNdsAudioBgmSoundActive = 0u;
    gNdsAudioBgmMask |= 1u;
    gNdsAudioBgmResult = NDS_AUDIO_BGM_PASS;
    ndsBgmPost(NDS_BGM_PLAY, (generation << 6) | index, FALSE);
}

void ndsAudioBgmStopAll(void)
{
    gNdsAudioBgmStopCalls++;
    if (gNdsAudioBgmPlaying) gNdsAudioBgmStoppedOnTeardown = 1u;
    if (sBgmInitialized) ndsBgmPost(NDS_BGM_STOP, ndsBgmNextGeneration(), TRUE);
    sBgmPendingPlaying = 0u;
    gNdsAudioBgmPlaying = gNdsAudioBgmFileOpen = gNdsAudioBgmSoundActive = 0u;
    sNdsAudioBgmNaturalStopArmed = FALSE;
    gNdsAudioBgmMask |= 1u << 1;
}

s32 ndsAudioBgmCheckPlaying(s32 player)
{
    (void)player;
    ndsBgmObserve();
    gNdsAudioBgmCheckCalls++; gNdsAudioBgmMask |= 1u << 2;
    return gNdsAudioBgmPlaying != 0u;
}
s32 ndsAudioBgmIsPlaying(void) { ndsBgmObserve(); return gNdsAudioBgmPlaying != 0u; }

static void ndsAudioBgmApplyVolume(u32 volume)
{
    if (volume > 0x7800u) volume = 0x7800u;
    if (sBgmInitialized) ndsBgmPost(NDS_BGM_VOLUME, volume, FALSE);
}

void ndsAudioBgmSetVolume(s32 player, u32 vol)
{
    (void)player;
    if (vol > 0x7800u) vol = 0x7800u;
    gNdsAudioBgmSetVolumeCalls++;
    gNdsAudioBgmVolume = vol;
    /* The source's syAudioSetBGMVolume clears that player's fade timer; an
     * immediate set always wins over a running ramp. */
    sNdsAudioBgmFadeFramesLeft = 0u;
    ndsAudioBgmApplyVolume(vol);
    gNdsAudioBgmMask |= 1u << 3;
}

void ndsAudioBgmSetVolumeFade(s32 player, u32 vol, u32 frames)
{
    s32 diff;

    (void)player;
    if (vol > 0x7800u)
    {
        vol = 0x7800u;
    }
    if (frames == 0u)
    {
        ndsAudioBgmSetVolume(player, vol);
        return;
    }
    diff = (s32)vol - (s32)gNdsAudioBgmVolume;
    sNdsAudioBgmFadeTarget = vol;
    sNdsAudioBgmFadeDenom = frames;
    sNdsAudioBgmFadeFramesLeft = frames;
    sNdsAudioBgmFadeStep = diff / (s32)frames;
    sNdsAudioBgmFadeRem = diff - sNdsAudioBgmFadeStep * (s32)frames;
    sNdsAudioBgmFadeError = 0;
}

static void ndsAudioBgmStepFade(void)
{
    s32 next;

    if (sNdsAudioBgmFadeFramesLeft == 0u)
    {
        return;
    }
    next = (s32)gNdsAudioBgmVolume + sNdsAudioBgmFadeStep;
    sNdsAudioBgmFadeError += sNdsAudioBgmFadeRem;
    if (sNdsAudioBgmFadeError >= (s32)sNdsAudioBgmFadeDenom)
    {
        next++;
        sNdsAudioBgmFadeError -= (s32)sNdsAudioBgmFadeDenom;
    }
    else if (sNdsAudioBgmFadeError <= -(s32)sNdsAudioBgmFadeDenom)
    {
        next--;
        sNdsAudioBgmFadeError += (s32)sNdsAudioBgmFadeDenom;
    }
    sNdsAudioBgmFadeFramesLeft--;
    if (sNdsAudioBgmFadeFramesLeft == 0u)
    {
        next = (s32)sNdsAudioBgmFadeTarget;
    }
    if (next < 0)
    {
        next = 0;
    }
    else if (next > 0x7800)
    {
        next = 0x7800;
    }
    gNdsAudioBgmVolume = (u32)next;
    ndsAudioBgmApplyVolume((u32)next);
    gNdsAudioBgmMask |= 1u << 3;
}

void syAudioSetQuality(s32 quality) { dSYAudioSoundQuality = quality; }
void syAudioSetBGMVolumeFade(s32 player, u32 volume, u32 frames)
{
    ndsAudioBgmSetVolumeFade(player, volume, frames);
}

void ndsAudioBgmUpdate(void)
{
    ndsBgmObserve();
    if (!gNdsAudioBgmPlaying) return;
    gNdsAudioBgmElapsedFrames++;
    ndsAudioBgmStepFade();
}
