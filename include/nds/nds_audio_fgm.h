#ifndef SSB64_NDS_AUDIO_FGM_H
#define SSB64_NDS_AUDIO_FGM_H

#include <PR/ultratypes.h>
#include <sys/audio.h>

#define NDS_AUDIO_FGM_PASS 0x46474d31u /* FGM1 */
#define NDS_AUDIO_FGM_ENTRY_COUNT 153u
#define NDS_AUDIO_FGM_PHASE_COUNT 5u
#define NDS_AUDIO_FGM_PHASE_COMPLETE_MASK 0x1fu
#define NDS_AUDIO_FGM_KO_COUNT 5u
#define NDS_AUDIO_FGM_KO_TRACE_CAPACITY 8u
#define NDS_AUDIO_FGM_MISS_RING_CAPACITY 16u
#ifndef NDS_AUDIO_FGM_ARM7_ACK_DIAGNOSTICS
#define NDS_AUDIO_FGM_ARM7_ACK_DIAGNOSTICS 0
#endif
#if NDS_AUDIO_FGM_ARM7_ACK_DIAGNOSTICS
#define NDS_AUDIO_FGM_ARM7_ACK_EVENT_CAPACITY 2u
#define NDS_AUDIO_FGM_ARM7_ACK_KIND_PLAY 1u
#define NDS_AUDIO_FGM_ARM7_ACK_KIND_STOP 3u
#define NDS_AUDIO_FGM_RELEASE_REASON_RESET 1u
#define NDS_AUDIO_FGM_RELEASE_REASON_GENERATION_LOST 2u
#define NDS_AUDIO_FGM_RELEASE_REASON_DURATION 3u
#define NDS_AUDIO_FGM_RELEASE_REASON_STOP_ALL 4u
#define NDS_AUDIO_FGM_RELEASE_REASON_EXPLICIT 5u
#endif
/* BOTH of these must equal the generated pack exactly, and BOTH move together
 * whenever the pack is re-rendered.  ndsAudioFgmLoad checks the size against the
 * file length, the header field, and the envelope cursor, and checks the mapping
 * hash against header[12]; either mismatch rejects THE WHOLE PACK, which is not
 * a size nit, it is total silence.  On 2026-08-02 the size moved 725900 -> 725896
 * for the FGM 430/439 note schedules and the hash was left behind -- the ROM
 * booted mute with gNdsAudioFgmFormatFailCount 1.  check-audio-fgm-phase-pack.ps1
 * now derives both from the pack binary and prints the values to set here, so
 * re-render, run it, and paste what it names.  Never pin them anywhere else. */
/* 920152 -> 938996 and 0x5d1c7cf5 -> 0x885657f4 on 2026-08-06: FGM 617 GaspS
 * and 622 DamageL joined FULL_PROGRAM_AOT_IDS, because both carry a multi-note
 * schedule the flat path cannot express -- it renders one one-shot and every
 * note after the first is silence.  617 1,138 -> 1,437 ms, 622 1,441 -> 2,185
 * ms, each now matching its own note total exactly. */
/* 938996 -> 948068 and 0x885657f4 -> 0xe4b8921c on 2026-08-18 (P2-1c-1): the
 * UI kit's four menu SFX (158 MenuSelect, 163 MenuScroll1, 164 MenuScroll2,
 * 165 MenuDenied) joined SELECTED, 88 -> 92 entries.  The kit's seam already
 * asked for 164/158/165 and missed -- UKMISS id0=164 c0=17 id1=165 c1=6,
 * 2026-08-17 P2-1c evidence -- so this is the same silent-miss class as every
 * prior repin here, not a new one. */
/* 948068 -> 950168 and 0xe4b8921c -> 0x9bc3e069 on 2026-08-18 (P2-1d-1): FGM
 * 157 nSYAudioFGMTitlePressStart (the title screen's own confirm cue,
 * mntitle.c:501) joined SELECTED, 92 -> 93 entries.  The menu shell's seam
 * already asked for it with the source's own id and missed -- MSMISS ring=1
 * id0=157 c0=1, P2-1d evidence, the only cue any menu screen misses -- same
 * silent-miss class as every prior repin here. */
/* 950168 -> 973524 and 0x9bc3e069 -> 0xcb181af6 on 2026-08-18 (P2-1e-1): the
 * character select's own four cues (121 MarioDash, 127 SamusDash, 167
 * PlayerSlotWhoosh, 512 AnnounceFreeForAll) joined SELECTED, 93 -> 97
 * entries.  The menu shell's seam already asked for all four with the
 * source's own ids and missed -- MSMISS ring=4 id0=512 c0=1 id1=127 c1=1
 * id2=121 c2=2 id3=167 c3=1, 2026-08-18 P2-1e evidence -- same silent-miss
 * class as every prior repin here.  121 forks to 118 FoxDash with no local
 * notes and 118's own first note overflows the pack entry's u16 frequency
 * field (71,838 Hz), so it renders through FULL_PROGRAM_AOT_IDS like 85/189/
 * 190/219 -- the only one of the four that is not a plain flat render. */
/* 973524 -> 990120 and 0xcb181af6 -> 0x3d9a9ac2 on 2026-08-18 (P2-1f-1): the
 * stage select's own confirm cue (159 nSYAudioFGMStageSelect,
 * NDS_SSS_FGM_CONFIRM in nds_menu_shell.c) joined SELECTED, 97 -> 98
 * entries.  The menu shell's seam already asked for it with the source's own
 * id and missed -- MSMISS ring=1 id0=159 c0=1, 2026-08-18 P2-1f evidence --
 * same silent-miss class as every prior repin here.  159 has a real local
 * note of its own AND forks two voices (163 MenuScroll1, 6 UnkSmallPing1) at
 * tick 0, so it joins FULL_PROGRAM_AOT_IDS to render all three voices fused
 * -- the same mechanism 154/616-625/121 above already use, extended to two
 * simultaneous forks instead of one or zero. */
/* P2-3: Luigi's 498/421 plus DK's complete 324..336 bank, announcer 483 and
 * crowd chant 603 are now source-backed. DK 324 uses compact timed retriggers
 * of one cached source wave instead of a 112 KiB baked timeline, so the runtime
 * still streams through the unchanged 200 KiB cache and 52 KiB largest slot. */
#define NDS_AUDIO_FGM_PACK_BYTES 1511844u
#define NDS_AUDIO_FGM_PACK_MAPPING_SHA256_LO 0x17d8f4ffu
#define NDS_AUDIO_FGM_CACHE_BYTES 204800u
#define NDS_AUDIO_FGM_HANDLE_CAPACITY 8u
#define NDS_AUDIO_FGM_FIDELITY_DEBT_PITCH_AUTOMATION (1u << 2)
#define NDS_AUDIO_FGM_FIDELITY_DEBT_FORK_VOICE (1u << 3)
#define NDS_AUDIO_FGM_FIDELITY_DEBT_VOLUME_AUTOMATION (1u << 4)
#define NDS_AUDIO_FGM_FIDELITY_DEBT_CUSTOM_FX (1u << 5)
#define NDS_AUDIO_FGM_EXPECTED_FIDELITY_DEBT_MASK \
    (NDS_AUDIO_FGM_FIDELITY_DEBT_PITCH_AUTOMATION | \
     NDS_AUDIO_FGM_FIDELITY_DEBT_VOLUME_AUTOMATION | \
     NDS_AUDIO_FGM_FIDELITY_DEBT_CUSTOM_FX)

#if NDS_AUDIO_FGM_ARM7_ACK_DIAGNOSTICS
typedef struct NDSAudioFgmArm7AckEvent {
    u32 kind;
    u32 source_tick;
    u32 value;
    u32 service_tick;
    u32 command_tick;
    u32 command_return_tick;
    u32 acknowledge_tick;
    u32 active_channels;
} NDSAudioFgmArm7AckEvent;

typedef struct NDSAudioFgmArm7AckTrace {
    u32 sequence;
    u32 event_count;
    u32 overflow_count;
    u32 mismatch_count;
    u32 fgm_id;
    u32 generation;
    u32 channel;
    u32 instance_token;
    u32 handle_start_tick;
    u32 handle_end_tick;
    u32 duration_ticks;
    u32 envelope_count;
    NDSAudioFgmArm7AckEvent
        events[NDS_AUDIO_FGM_ARM7_ACK_EVENT_CAPACITY];
} NDSAudioFgmArm7AckTrace;
#endif

void ndsAudioFgmDiagnosticsReset(void);
void ndsAudioFgmLoadFenced(void);
void ndsAudioFgmUpdate(void);
void ndsAudioFgmStopAll(void);
void ndsAudioFgmStop(alSoundEffect *effect);
alSoundEffect *ndsAudioFgmPlay(u16 fgm_id);
alSoundEffect *ndsAudioFgmPlayAtPan(u16 fgm_id, u8 pan);

extern volatile u32 gNdsAudioFgmResult;
extern volatile u32 gNdsAudioFgmMask;
extern volatile u32 gNdsAudioFgmLoaded;
extern volatile u32 gNdsAudioFgmResidentBytes;
extern volatile u32 gNdsAudioFgmSupportedCount;
extern volatile u32 gNdsAudioFgmOpenFailCount;
extern volatile u32 gNdsAudioFgmReadFailCount;
extern volatile u32 gNdsAudioFgmFormatFailCount;
extern volatile u32 gNdsAudioFgmPlayCalls;
extern volatile u32 gNdsAudioFgmSupportedPlayCount;
extern volatile u32 gNdsAudioFgmUnsupportedCallCount;
extern volatile u32 gNdsAudioFgmMissRingCount;
extern volatile u32 gNdsAudioFgmMissRingNext;
extern volatile u16
    gNdsAudioFgmMissRingIDs[NDS_AUDIO_FGM_MISS_RING_CAPACITY];
extern volatile u32
    gNdsAudioFgmMissRingCounts[NDS_AUDIO_FGM_MISS_RING_CAPACITY];
extern volatile u32 gNdsAudioFgmIncludedLookupFailCount;
extern volatile u32 gNdsAudioFgmPlayFailCount;
extern volatile u32 gNdsAudioFgmPhasePlayMask;
extern volatile u32 gNdsAudioFgmPhasePlayCounts[NDS_AUDIO_FGM_PHASE_COUNT];
extern volatile u32 gNdsAudioFgmKoPlayMask;
extern volatile u32 gNdsAudioFgmKoPlayCounts[NDS_AUDIO_FGM_KO_COUNT];
extern volatile u32 gNdsAudioFgmKoTraceCount;
extern volatile u32
    gNdsAudioFgmKoTrace[NDS_AUDIO_FGM_KO_TRACE_CAPACITY];
extern volatile u32 gNdsAudioFgmLoopPlayCount;
extern volatile u32 gNdsAudioFgmStopCalls;
extern volatile u32 gNdsAudioFgmStopAllCalls;
extern volatile u32 gNdsAudioFgmDurationStopCount;
/* Ramp steps written while a finished note falls silent. Zero with a non-zero
 * DurationStopCount means the release never engaged. */
extern volatile u32 gNdsAudioFgmReleaseRampCount;
extern volatile u32 gNdsAudioFgmStaleStopCount;
extern volatile u32 gNdsAudioFgmGenerationMismatchCount;
extern volatile u32 gNdsAudioFgmActiveHandles;
extern volatile u32 gNdsAudioFgmMaxActiveHandles;
extern volatile u32 gNdsAudioFgmChannelMask;
extern volatile u32 gNdsAudioFgmLastChannel;
extern volatile u32 gNdsAudioFgmLastID;
extern volatile u32 gNdsAudioFgmLastGeneration;
extern volatile u32 gNdsAudioFgmLastInstanceToken;
extern volatile u32 gNdsAudioFgmInstanceTokenWrapCount;
extern volatile u32 gNdsAudioFgmPoolExhaustCount;
extern volatile u32 gNdsAudioFgmHandleAcquireCount;
extern volatile u32 gNdsAudioFgmHandleReleaseCount;
extern volatile u32 gNdsAudioFgmHandleRecycleCount;
extern volatile u32 gNdsAudioFgmHandleCapacity;
extern volatile u32 gNdsAudioFgmEnvelopeStepCount;
extern volatile u32 gNdsAudioFgmFidelityDebtMask;
#if NDS_AUDIO_FGM_ARM7_ACK_DIAGNOSTICS
extern volatile NDSAudioFgmArm7AckTrace gNdsAudioFgmArm7AckTrace;
#endif

#endif
