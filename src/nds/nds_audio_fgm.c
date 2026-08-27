#include <nds.h>

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <gm/gmsound.h>
#include <nds/nds_audio_fgm.h>
#include <nds/nds_freeze_diagnostics.h>

#define NDS_AUDIO_FGM_PATH "nitro:/audio/fgm_phase_pack_ima.bin"
#define NDS_AUDIO_FGM_PACK_HEADER_BYTES 16u
#define NDS_AUDIO_FGM_PACK_ENTRY_BYTES 32u
#define NDS_AUDIO_FGM_ENVELOPE_POINT_BYTES 4u
#define NDS_AUDIO_FGM_PACK_DATA_OFFSET \
    (NDS_AUDIO_FGM_PACK_HEADER_BYTES + \
     (NDS_AUDIO_FGM_ENTRY_COUNT * NDS_AUDIO_FGM_PACK_ENTRY_BYTES))
#define NDS_AUDIO_FGM_HANDLE_COUNT NDS_AUDIO_FGM_HANDLE_CAPACITY
#define NDS_AUDIO_FGM_CHANNEL_COUNT 16u
#define NDS_AUDIO_FGM_TIMER_MICROSECONDS 5750u
/* How long a finished note takes to fall silent. ONE CONSTANT, and it is
 * standing in for data the pack does not carry yet: the N64 release comes from
 * each sound's ADSR in the bank, which the FGM pack generator reads but does
 * not emit. Ten FGM ticks is 57.5 ms, chosen as the shortest window that is
 * unambiguously a fade rather than a click while staying well under the
 * smallest tail it has to cover (PublicFox, 129 ms). Replace it with the
 * per-cue release when the pack carries one; the shape here is already right,
 * only the length is generic. Extending a handle by 57.5 ms is affordable:
 * the measured peak is six of eight handles with PoolExhaustCount 0. */
#define NDS_AUDIO_FGM_RELEASE_MICROSECONDS (NDS_AUDIO_FGM_TIMER_MICROSECONDS * 10u)
#define NDS_AUDIO_FGM_CACHE_SLOT_COUNT 8u
#define NDS_AUDIO_FGM_CACHE_SLOT_LARGE_BYTES (60u * 1024u)
#define NDS_AUDIO_FGM_CACHE_SLOT_MEDIUM_BYTES (28u * 1024u)
#define NDS_AUDIO_FGM_CACHE_SLOT_SMALL_BYTES (16u * 1024u)
#define NDS_AUDIO_FGM_CACHE_MAX_ENVELOPE_POINTS 32u
#define NDS_AUDIO_FGM_EVENT_RESTART_SAMPLE (1u << 0)

#define NDS_AUDIO_FGM_MASK_PACK_LOADED (1u << 0)
#define NDS_AUDIO_FGM_MASK_SUPPORTED_PLAY (1u << 1)
#define NDS_AUDIO_FGM_MASK_LOOP_PLAY (1u << 2)
#define NDS_AUDIO_FGM_MASK_PHASE_COMPLETE (1u << 3)

#if NDS_AUDIO_FGM_ARM7_ACK_DIAGNOSTICS
#define NDS_AUDIO_FGM_ACK_RELEASE_PARAMS \
    , u32 release_reason, u32 service_tick
#define NDS_AUDIO_FGM_ACK_RELEASE_ARGS(reason, service_tick) \
    , reason, service_tick
#else
#define NDS_AUDIO_FGM_ACK_RELEASE_PARAMS
#define NDS_AUDIO_FGM_ACK_RELEASE_ARGS(reason, service_tick)
#endif

typedef struct NDSAudioFgmPackEntry {
    u16 id;
    u16 flags;
    u32 data_offset;
    u32 data_bytes;
    u32 sample_count;
    u16 frequency;
    u16 duration_ticks;
    u8 volume;
    u8 pan;
    u16 source_sound_index;
    u32 envelope_offset;
    u16 envelope_count;
    u16 loop_point_words;
} NDSAudioFgmPackEntry;

typedef struct NDSAudioFgmHandle {
    alSoundEffect effect;
    u32 generation;
    u32 start_tick;
    u32 end_tick;
    /* end_tick is the SOURCE note's length; audible_end_tick is when this DS
     * sample actually stops making sound (sample_count / frequency).  They are
     * not close: baking net_pitch_cents into the playback rate shortens every
     * cue, and all 88 finish before their note does -- FGM 433 declares 760
     * ticks for a 165-tick sample.  Anything asking "was this cut off?" must
     * use audible_end_tick; end_tick answers yes for every cue, always. */
    u32 audible_end_tick;
    u8 envelope_points[NDS_AUDIO_FGM_CACHE_MAX_ENVELOPE_POINTS *
                       NDS_AUDIO_FGM_ENVELOPE_POINT_BYTES];
    u16 envelope_count;
    u16 envelope_index;
    u16 fgm_id;
    s8 channel;
    s8 cache_slot;
    u8 allocated;
    u8 live;
    u8 ever_allocated;
    /* Whether the hardware voice was started looping. A looping voice never
     * reaches audible_end_tick, so only the note duration can bound it; a
     * one-shot must be allowed to reach it. See the release below. */
    u8 loops;
    /* The level this cue is currently sounding at, so the release below has
     * something to ramp down FROM. Seeded with the pack entry's volume and
     * followed along the envelope, because an envelope point changes it. */
    u8 volume;
} NDSAudioFgmHandle;

typedef struct NDSAudioFgmCacheSlot {
    u8 *data;
    u32 capacity;
    u32 data_bytes;
    u16 fgm_id;
    u16 references;
} NDSAudioFgmCacheSlot;

volatile u32 gNdsAudioFgmResult;
volatile u32 gNdsAudioFgmMask;
volatile u32 gNdsAudioFgmLoaded;
volatile u32 gNdsAudioFgmResidentBytes;
volatile u32 gNdsAudioFgmSupportedCount;
volatile u32 gNdsAudioFgmOpenFailCount;
volatile u32 gNdsAudioFgmReadFailCount;
volatile u32 gNdsAudioFgmFormatFailCount;
volatile u32 gNdsAudioFgmPlayCalls;
volatile u32 gNdsAudioFgmSupportedPlayCount;
volatile u32 gNdsAudioFgmUnsupportedCallCount;
volatile u32 gNdsAudioFgmMissRingCount;
volatile u32 gNdsAudioFgmMissRingNext;
volatile u16 gNdsAudioFgmMissRingIDs[NDS_AUDIO_FGM_MISS_RING_CAPACITY];
volatile u32 gNdsAudioFgmMissRingCounts[NDS_AUDIO_FGM_MISS_RING_CAPACITY];
volatile u32 gNdsAudioFgmIncludedLookupFailCount;
volatile u32 gNdsAudioFgmPlayFailCount;
volatile u32 gNdsAudioFgmPhasePlayMask;
volatile u32 gNdsAudioFgmPhasePlayCounts[NDS_AUDIO_FGM_PHASE_COUNT];
volatile u32 gNdsAudioFgmKoPlayMask;
volatile u32 gNdsAudioFgmKoPlayCounts[NDS_AUDIO_FGM_KO_COUNT];
volatile u32 gNdsAudioFgmKoTraceCount;
volatile u32 gNdsAudioFgmKoTrace[NDS_AUDIO_FGM_KO_TRACE_CAPACITY];
volatile u32 gNdsAudioFgmLoopPlayCount;
volatile u32 gNdsAudioFgmStopCalls;
volatile u32 gNdsAudioFgmStopAllCalls;
volatile u32 gNdsAudioFgmDurationStopCount;
volatile u32 gNdsAudioFgmReleaseRampCount;
volatile u32 gNdsAudioFgmStaleStopCount;
volatile u32 gNdsAudioFgmGenerationMismatchCount;
volatile u32 gNdsAudioFgmActiveHandles;
volatile u32 gNdsAudioFgmMaxActiveHandles;
volatile u32 gNdsAudioFgmChannelMask;
volatile u32 gNdsAudioFgmLastChannel;
volatile u32 gNdsAudioFgmLastID;
volatile u32 gNdsAudioFgmLastGeneration;
volatile u32 gNdsAudioFgmLastInstanceToken;
volatile u32 gNdsAudioFgmInstanceTokenWrapCount;
volatile u32 gNdsAudioFgmPoolExhaustCount;

/* BUGS.md crowd cut-off. Counts channel retires taken while the previous owner's
 * own expected end was still in the future -- i.e. a cue that was still sounding.
 * Non-zero proves channel contention is the mechanism; zero clears it. */
volatile u32 gNdsAudioFgmPrematureRetireCount;
volatile u32 gNdsAudioFgmPrematureRetireLastID;
volatile u32 gNdsAudioFgmHandleAcquireCount;
volatile u32 gNdsAudioFgmHandleReleaseCount;
volatile u32 gNdsAudioFgmHandleRecycleCount;
volatile u32 gNdsAudioFgmHandleCapacity;
volatile u32 gNdsAudioFgmEnvelopeStepCount;
volatile u32 gNdsAudioFgmFidelityDebtMask;
#if NDS_AUDIO_FGM_ARM7_ACK_DIAGNOSTICS
volatile NDSAudioFgmArm7AckTrace gNdsAudioFgmArm7AckTrace;
#endif

static u8 sNdsAudioFgmMetadata[NDS_AUDIO_FGM_PACK_DATA_OFFSET]
    __attribute__((aligned(4)));
#define sNdsAudioFgmPack sNdsAudioFgmMetadata
static u8 sNdsAudioFgmCache[NDS_AUDIO_FGM_CACHE_BYTES]
    __attribute__((aligned(4)));
static NDSAudioFgmCacheSlot
    sNdsAudioFgmCacheSlots[NDS_AUDIO_FGM_CACHE_SLOT_COUNT];
static FILE *sNdsAudioFgmFile;
static NDSAudioFgmPackEntry
    sNdsAudioFgmEntries[NDS_AUDIO_FGM_ENTRY_COUNT];
static NDSAudioFgmHandle sNdsAudioFgmHandles[NDS_AUDIO_FGM_HANDLE_COUNT];
static NDSAudioFgmHandle *sNdsAudioFgmChannelOwners[NDS_AUDIO_FGM_CHANNEL_COUNT];
static u32 sNdsAudioFgmChannelGenerations[NDS_AUDIO_FGM_CHANNEL_COUNT];
static u32 sNdsAudioFgmNextGeneration = 1u;
#if NDS_AUDIO_FGM_ARM7_ACK_DIAGNOSTICS
static u32 sNdsAudioFgmArm7AckSequence;
#endif
static u16 sNdsAudioFgmInstanceToken;

/* 16-byte header + N * 32-byte entries. The header and entry SIZES are the
 * layout and are what this pins; the entry count is data. It used to pin the
 * product against the literal 2032, so adding one cue failed the build with
 * "FGM pack header layout changed" -- a true statement about nothing that had
 * changed. Assert the relation instead: a real layout move still trips it,
 * a new cue does not. */
_Static_assert(NDS_AUDIO_FGM_PACK_HEADER_BYTES == 16u,
               "FGM pack header layout changed");
_Static_assert(NDS_AUDIO_FGM_PACK_ENTRY_BYTES == 32u,
               "FGM pack entry layout changed");
_Static_assert(NDS_AUDIO_FGM_PACK_DATA_OFFSET ==
                   (16u + (NDS_AUDIO_FGM_ENTRY_COUNT * 32u)),
               "FGM pack header layout changed");
_Static_assert(NDS_AUDIO_FGM_CACHE_BYTES == (208u * 1024u),
               "FGM cache budget changed");
_Static_assert(NDS_AUDIO_FGM_CACHE_BYTES ==
                   (NDS_AUDIO_FGM_CACHE_SLOT_LARGE_BYTES +
                    (3u * NDS_AUDIO_FGM_CACHE_SLOT_MEDIUM_BYTES) +
                    (4u * NDS_AUDIO_FGM_CACHE_SLOT_SMALL_BYTES)),
               "FGM cache slots no longer cover the resident cache exactly");
_Static_assert(offsetof(NDSAudioFgmHandle, effect) == 0u,
               "BattleShip audio handle must be the backend handle prefix");
_Static_assert(offsetof(alSoundEffect, sfx_id) == 0x26u,
               "BattleShip FGM instance-token field moved");
_Static_assert(NDS_AUDIO_FGM_HANDLE_COUNT >= 3u,
               "FGM pool must support the source KO call burst");
_Static_assert(NDS_AUDIO_FGM_CHANNEL_COUNT == SOUND_NUM_CHANNELS,
               "FGM channel count must match the Calico mixer");
#if NDS_AUDIO_FGM_ARM7_ACK_DIAGNOSTICS
_Static_assert(sizeof(NDSAudioFgmArm7AckEvent) == 32u,
               "FGM ARM7 ACK event layout changed");
_Static_assert(offsetof(NDSAudioFgmArm7AckTrace, events) == 48u,
               "FGM ARM7 ACK event array moved");
_Static_assert(sizeof(NDSAudioFgmArm7AckTrace) == 112u,
               "FGM ARM7 ACK trace layout changed");
#endif

/* The release window in CPU ticks, derived the same way the duration is so the
 * two cannot drift apart. */
static u32 ndsAudioFgmReleaseCpuTicks(void)
{
    return (u32)(((u64)BUS_CLOCK * NDS_AUDIO_FGM_RELEASE_MICROSECONDS) /
                 1000000u);
}

static u16 ndsAudioFgmReadLe16(const u8 *data)
{
    return (u16)(((u16)data[1] << 8) | data[0]);
}

static u32 ndsAudioFgmReadLe32(const u8 *data)
{
    return ((u32)data[3] << 24) | ((u32)data[2] << 16) |
           ((u32)data[1] << 8) | data[0];
}

static s32 ndsAudioFgmIDIsIncluded(u16 id)
{
    switch (id)
    {
    case nSYAudioVoicePublicExcited:
    case nSYAudioVoiceAnnounceThree:
    case nSYAudioVoiceAnnounceTwo:
    case nSYAudioVoiceAnnounceOne:
    case nSYAudioVoiceAnnounceGo:
    case nSYAudioFGMFoxLanding:
    case nSYAudioVoiceFoxJumpAerial:
    case nSYAudioVoiceFoxEscape:
    case nSYAudioVoiceFoxSmash1:
    case nSYAudioVoiceFoxSmash2:
    case nSYAudioVoiceFoxSmash3:
    case nSYAudioVoiceMarioSmash2:
    case nSYAudioVoiceMarioDead:
    case nSYAudioFGMMarioDeadSlam:
    case nSYAudioVoiceFoxDead:
    case nSYAudioFGMFoxDeadSlam:
    case nSYAudioFGMFoxDownBounce:
    case nSYAudioFGMMarioDownBounce:
    case nSYAudioFGMDeadExplodeL:
    case nSYAudioFGMMarioLanding:
    case nSYAudioFGMMarioSpecialN:
    case nSYAudioFGMPunchS:
    case nSYAudioFGMPunchM:
    case nSYAudioFGMPunchL:
    case nSYAudioFGMKickS:
    case nSYAudioFGMKickM:
    case nSYAudioFGMKickL:
    case nSYAudioVoiceFoxDamage:
    case nSYAudioVoiceMarioSmash1:
    case nSYAudioVoiceMarioSmash3:
    case 435u: /* Mario jump voice */
    case nSYAudioVoiceMarioDamage:
    case nSYAudioFGMCatch:
    case nSYAudioFGMLightSwingL:
    case nSYAudioFGMLightSwingM:
    case nSYAudioFGMLightSwingS:
    case nSYAudioFGMFoxSpecialN:
    case nSYAudioFGMFoxSpecialHiStart:
    case nSYAudioFGMFoxSpecialHiFly:
    case nSYAudioFGMFoxSpecialLwStart:
    case nSYAudioFGMFoxAttackAirLw:
    case nSYAudioFGMMarioSpecialHiJump:
    case nSYAudioFGMMarioUnkSwing1:
    case nSYAudioFGMMarioUnkSwing2:
    case nSYAudioFGMMarioSpecialHiCoin:
    case nSYAudioFGMBurnS:
    case nSYAudioFGMFireShoot1:
    case nSYAudioFGMExplodeS:
    case 188u: /* Fox reflector-hit cue */
    /* BUGS.md #4/#6/#8. Each of these was requested by live battle code and
     * failed closed for want of a pack entry: 436 by the JumpAerial motion
     * script (435, the grounded jump, was packed, so only the double jump was
     * silent), 432/362 by the Mario down-B and Fox up-B scripts, and 12/433/360
     * by the star-KO path in ftcommondead.c. */
    case nSYAudioFGMDeadUpStar:
    case nSYAudioVoiceFoxDeadUp:
    case nSYAudioVoiceFoxSpecialHi:
    case nSYAudioVoiceMarioSpecialLw:
    case nSYAudioVoiceMarioDeadUp:
    case nSYAudioVoiceMarioJumpAerial:
    /* BUGS.md #3: Whispy's wind gust, requested by the Pupupu ground loop. */
    case nSYAudioFGMPupupuWhispyWind:
    /* The announcer. Every one of these is already requested by live P1 code
     * and was failing closed for want of a pack entry, which is why the match
     * ended in silence: ifCommonAnnounceTimeUpMakeInterface plays 527, the
     * GAME SET path plays 488, and the Results scene plays "this game's winner
     * is" followed by the winner's own name. */
    case nSYAudioVoiceAnnounceTimeUp:
    case nSYAudioVoiceAnnounceGameSet:
    case nSYAudioVoiceAnnounceWinnerIs:
    case nSYAudioVoiceAnnounceNoContest:
    case nSYAudioVoiceAnnounceMario:
    case nSYAudioVoiceAnnounceFox:
    case nSYAudioVoiceAnnounceLuigi:
    case nSYAudioVoiceLuigiFuraFura:
    /* P2-3 Donkey Kong production bank. These are the exact BattleShip IDs
     * reachable from DonkeyMain/MainMotion, DK's CSS selected clip, the CSS
     * announcer table and ftpublic's fighter-call table. Keep the complete
     * fighter bank together so newly admitted source states do not silently
     * become audio stubs. */
    case nSYAudioVoiceDonkeyFuraSleep:
    case nSYAudioVoiceDonkeyAppeal:
    case nSYAudioVoiceDonkeySmash1:
    case nSYAudioVoiceDonkeySmash2:
    case nSYAudioVoiceDonkeySmash3:
    case nSYAudioVoiceDonkeySpecialN:
    case nSYAudioVoiceDonkeyDeadUp:
    case nSYAudioVoiceDonkeyFuraFura:
    case nSYAudioVoiceDonkeyDamage:
    case nSYAudioVoiceDonkeyDead1:
    case nSYAudioVoiceDonkeyHeavyGet:
    case nSYAudioVoiceDonkeyHeavyUnk:
    case nSYAudioVoiceDonkeyDead2:
    case nSYAudioVoiceAnnounceDonkey:
    case nSYAudioVoicePublicDonkey:
    /* And the two the miss ring surfaced only once the five above stopped
     * filling it: the countdown announces FIVE and FOUR before the THREE that
     * was already here. */
    case nSYAudioVoiceAnnounceFive:
    case nSYAudioVoiceAnnounceFour:
    /* The crowd's win roar, queued at Results scene start 81 ticks ahead of
     * "this game's winner is". Second cue on PublicExcited's wave. */
    case nSYAudioVoicePublicWin:
    /* No Contest Results has its own source crowd response at tic 71. It is the
     * same articulation/wave family as PublicWin/PublicExcited and is packed by
     * the same finite AOT loop renderer. */
    case nSYAudioVoicePublicNoContest:
    /* BUGS.md crowd row. Every one of these is requested by ft/ftpublic.c --
     * the chant for whichever fighter is being called, and the reaction its
     * knockback thresholds select. Silent until now because the actor was a
     * stub AND the pack had no entries; both halves land together. */
    case nSYAudioVoicePublicFox:
    case nSYAudioVoicePublicMario:
    case nSYAudioVoicePublicGaspL:
    case nSYAudioVoicePublicGaspM:
    case nSYAudioVoicePublicGaspS:
    case nSYAudioVoicePublicCheer:
    case nSYAudioVoicePublicAmazed:
    case nSYAudioVoicePublicGaspClap:
    case nSYAudioVoicePublicDamageL:
    case nSYAudioVoicePublicDamageM:
    case nSYAudioVoicePublicDamageS:
    /* The two loudest survivors of the natural-match miss ring: the ground
     * grind a match requests six times a minute, and the altitude warning --
     * the second cue in the pack to ship as a DS hardware repeat, because its
     * 300-tick schedule outlives its 0.757 s sample. And the third, whose
     * first note asks for 90,510 Hz -- past the u16 `frequency` field above,
     * so it renders its whole note schedule AOT and stores 32,000 like every
     * other full-program cue. */
    case nSYAudioFGMGroundGrind2:
    case nSYAudioFGMAltitudeWarn:
    case nSYAudioFGMUnkGrind4:
    /* The five only a BOTH-CPU stress match reaches, all core P1 gameplay:
     * the dodge, the shield going up and down, the pause, and the noise Fox
     * makes teetering on a ledge. Same full-program AOT render as 85. */
    case nSYAudioFGMEscape:
    case nSYAudioFGMGuardOn:
    case nSYAudioFGMGuardOff:
    case nSYAudioFGMGamePause:
    case nSYAudioVoiceFoxOttotto:
    /* And the last two the miss ring named: the zoom pulse and Fox's win
     * voice at Results. */
    case nSYAudioFGMMagnify:
    case nSYAudioVoiceFoxWin:
    /* And three the ring only reached once every fireball spawned and the
     * match went to Sudden Death. */
    case nSYAudioFGMLightSwingLw1:
    case nSYAudioVoiceFoxSelected:
    case nSYAudioVoiceAnnounceSuddenDeath:
    /* P2-1c-1: the UI kit's SFX seam (ndsUiKitSfx, nds_ui_kit.c) already asks
     * for 164/158/165 with the source's own ids -- move/confirm/back -- and
     * missed for want of a pack entry, same as every case above. 163 has no
     * live caller yet; packed and declared together with the other three. */
    case nSYAudioFGMMenuSelect:
    case nSYAudioFGMMenuScroll1:
    case nSYAudioFGMMenuScroll2:
    case nSYAudioFGMMenuDenied:
    /* P2-1e-1: the character select's own audio seam (nds_menu_shell.c,
     * NDS_CSS_FGM_ANNOUNCE_WHOOSH/_GRAB/_SLOT_WHOOSH/NDS_CSS_VOICE_FREE_FOR_ALL)
     * already asks for these four with the source's own ids and missed for
     * want of a pack entry, same as every case above. */
    case nSYAudioFGMMarioDash:
    case nSYAudioFGMSamusDash:
    case nSYAudioFGMPlayerSlotWhoosh:
    case nSYAudioVoiceAnnounceFreeForAll:
    /* P2-1f-1 closing a residual P2-1e-1 recorded: 157 nSYAudioFGMTitlePressStart
     * (the title screen's own confirm cue) joined the pack at P2-1d-1 but never
     * gained its case here -- harmless (this switch is diagnostic-only
     * consistency bookkeeping, not a playback gate), but out of step with every
     * other packed id. And the stage select's own confirm cue (nds_menu_shell.c,
     * NDS_SSS_FGM_CONFIRM), already asking for it with the source's own id and
     * missing for want of a pack entry, same as every case above. */
    case nSYAudioFGMTitlePressStart:
    case nSYAudioFGMStageSelect:
    /* P2-1N (3)+(4): the shutter's arrival cue and the mode toggle's
     * announcer line, packed 2026-08-19 with the shell already asking. */
    case nSYAudioFGMPlayerSlotClose:
    case nSYAudioVoiceAnnounceTeamBattle:
    /* P2-3 Samus CSS: the announcer line and the selected-pose BladeDraw
     * motion command are both source-owned and both live in the shell. */
    case nSYAudioVoiceAnnounceSamus:
    case nSYAudioFGMBladeDraw:
    /* P2-3 Samus bounded gameplay bank. These are the exact source IDs from
     * SamusMainMotion/SamusMain, the shared DownBounce/public tables and the
     * Charge Shot/Bomb weapon code. The generator bakes their complete bounded
     * source programs AOT; the harder unbounded Charge0..7 sequencer remains
     * deliberately absent until its DS-native representation is source-
     * equivalent. Full-charge ShootF is exact now: the cache grew by 8 KiB
     * rather than truncating its 57,596-byte AOT body. Screw Attack / SpecialHi
     * is in the bounded set now that its n_env active-modulator target is
     * reproduced. */
    case nSYAudioFGMHeavySwing1:
    case nSYAudioFGMShockL:
    case nSYAudioFGMShockM:
    case nSYAudioFGMShockS:
    case nSYAudioFGMSamusLanding:
    case nSYAudioFGMSamusJumpAerial:
    case nSYAudioFGMGroundGrind4:
    case nSYAudioFGMSamusFoot:
    case nSYAudioFGMGroundBrakeGrind:
    case nSYAudioFGMSamusSpecialNShootF:
    case nSYAudioFGMSamusSpecialNShootL:
    case nSYAudioFGMSamusSpecialNShootM:
    case nSYAudioFGMSamusSpecialNShootS:
    case nSYAudioFGMSamusSpecialLw:
    case nSYAudioFGMSamusCatchGrappleBeam:
    case nSYAudioFGMSamusSpecialHi:
    case nSYAudioFGMSamusUnkSwing:
    case nSYAudioFGMSamusUnkCharge:
    case nSYAudioFGMSamusDeadSlam:
    case nSYAudioFGMSamusDownBounce:
    case nSYAudioVoiceSamusSmash1:
    case nSYAudioVoiceSamusSmash2:
    case nSYAudioVoiceSamusSmash3:
    case nSYAudioVoiceSamusDeadUp:
    case nSYAudioVoiceSamusFura:
    case nSYAudioVoiceSamusAttackHi4:
    case nSYAudioVoiceSamusUnkSlash:
    case nSYAudioVoiceSamusAppeal:
    case nSYAudioVoiceSamusDamage:
    case nSYAudioVoiceSamusDead:
    case nSYAudioVoicePublicSamus:
    case nSYAudioFGMCharacterUnkZip10:
    /* P2-3f13 Captain Falcon's production bank. He landed selectable at P2-3f8
     * with NOTHING packed, so every id below was already being requested by his
     * motion scripts, his FTAttributes lanes, his CSS clip and the
     * announcer/crowd tables, and every one of them fell into the miss ring.
     * The set is gm/gmsound.h's complete `nSYAudio{FGM,Voice}Captain*` run,
     * minus 356 FuraSleep -- that one cue's AOT body is 65,324 bytes against
     * the 53,248-byte largest cache slot, so it CANNOT be played and is
     * deliberately absent from both this list and the pack. Keep the whole
     * fighter bank together so a newly admitted source state does not silently
     * become an audio stub. */
    case nSYAudioFGMCaptainLanding:
    case nSYAudioFGMCaptainFoot:
    case nSYAudioFGMCaptainDash:
    case nSYAudioFGMCaptainAppearCar1:
    case nSYAudioFGMCaptainAppearCar2:
    case nSYAudioFGMCaptainSpecialHi:
    case nSYAudioFGMCaptainSpecialNStart:
    case nSYAudioFGMCaptainSpecialNPunch:
    case nSYAudioFGMCaptainDeadSlam:
    case nSYAudioFGMCaptainDownBounce:
    case nSYAudioVoiceCaptainAppeal:
    case nSYAudioVoiceCaptainSpecialHi:
    case nSYAudioVoiceCaptainSmash1:
    case nSYAudioVoiceCaptainSmash2:
    case nSYAudioVoiceCaptainSmash3:
    case nSYAudioVoiceCaptainSmash4:
    case nSYAudioVoiceCaptainFinalComeOn:
    case nSYAudioVoiceCaptainSmash5:
    case nSYAudioVoiceCaptainAttackS4:
    case nSYAudioVoiceCaptainSpecialLw:
    case nSYAudioVoiceCaptainSpecialNPunch:
    case nSYAudioVoiceCaptainSpecialNFalcon:
    case nSYAudioVoiceCaptainDeadUp:
    case nSYAudioVoiceCaptainFuraFura:
    case nSYAudioVoiceCaptainDamage:
    case nSYAudioVoiceCaptainUnkPing1:
    case nSYAudioVoiceCaptainJumpAerial:
    case nSYAudioVoiceCaptainHeavyGet:
    case nSYAudioVoiceCaptainDead:
    case nSYAudioVoiceCaptainUnkQuick:
    case nSYAudioVoiceCaptainUnkPing2:
    case nSYAudioVoiceCaptainUnkPing3:
    case nSYAudioVoiceAnnounceCaptain:
    case nSYAudioVoicePublicCaptain:
    /* P2-3f14 Donkey Kong's FGM bank. He received his VOICE bank at P2-3
     * (324..336 plus announcer 483 and crowd 603) and never his sound
     * effects, so every id below was already being asked for by
     * 212_DonkeyMainMotion.c, his `dead_fgm_ids[1]` and ft/ftcommondata.c's
     * shared DownBounce table -- 90 requests in one measured minute, 56 of
     * them nSYAudioFGMDonkeyCharge, every one missing. 10 DonkeySlap2 is not
     * his: Captain/Kirby/Purin/Yoshi play it and his 175/176 fork it, and it
     * is one of the seven roster-wide shared cues P2-3f13 left open. */
    case nSYAudioFGMDonkeySlap1:
    case nSYAudioFGMDonkeySlap2:
    case nSYAudioFGMDonkeyLanding:
    case nSYAudioFGMDonkeyFoot:
    case nSYAudioFGMDonkeyDash:
    case nSYAudioFGMBossSlam:
    case nSYAudioFGMBossUnk1:
    case nSYAudioFGMBossUnk2:
    case nSYAudioFGMDonkeySpin:
    case nSYAudioFGMDonkeyCharge:
    case nSYAudioFGMDonkeyDeadSlam:
    case nSYAudioFGMDonkeyDownBounce:
    /* P2-3f15 Luigi's voice bank. He shipped selectable with exactly TWO cues
     * packed -- his announcer line 498 and his selected-clip FuraFura 421 --
     * so every voice his `FTAttributes` lanes and his motion scripts name
     * failed closed. That is why P2-3f12's normalizer repair was latent:
     * correcting his mixed-u16 lanes only changed WHICH unpacked id he asked
     * for. The set is gm/gmsound.h's contiguous run 416..428 plus his row in
     * ft/ftcommondata.c's fighter-call table, 608. 425 Lets and 428 HereWe are
     * source-marked unused and packed anyway so the bank is never reopened.
     * His KO slam and DownBounce are MARIO's own 292/303, already packed --
     * the source spells them that way, so neither is a Luigi cue. */
    case nSYAudioVoiceLuigiSmash1:
    case nSYAudioVoiceLuigiSmash2:
    case nSYAudioVoiceLuigiSmash3:
    case nSYAudioVoiceLuigiSpecialLw:
    case nSYAudioVoiceLuigiDeadUp:
    case nSYAudioVoiceLuigiDamage:
    case nSYAudioVoiceLuigiJump:
    case nSYAudioVoiceLuigiJumpAerial:
    case nSYAudioVoiceLuigiLets:
    case nSYAudioVoiceLuigiHeavyGet:
    case nSYAudioVoiceLuigiDead:
    case nSYAudioVoiceLuigiHereWe:
    case nSYAudioVoicePublicLuigi:
    /* P2-3f16 fighter entry audio. These are not inferred conveniences: the
     * source entry motion scripts play 214 for Mario/Luigi's pipe, 191 for
     * Fox's Arwing, and 59 beside DK's BoxSmash effect. The pack generator
     * bakes each multi-note schedule AOT so the source timing survives the DS
     * one-shot backend, including 214's two explicit rest spans. */
    case nSYAudioFGMMarioDokan:
    case nSYAudioFGMFoxAppearArwing:
    case nSYAudioFGMContainerSmash:
        return TRUE;
    default:
        return FALSE;
    }
}

static void ndsAudioFgmRecordMiss(u16 id)
{
    u32 i;

    for (i = 0u; i < gNdsAudioFgmMissRingCount; i++)
    {
        if (gNdsAudioFgmMissRingIDs[i] == id)
        {
            gNdsAudioFgmMissRingCounts[i]++;
            return;
        }
    }
    i = gNdsAudioFgmMissRingNext;
    gNdsAudioFgmMissRingIDs[i] = id;
    gNdsAudioFgmMissRingCounts[i] = 1u;
    gNdsAudioFgmMissRingNext =
        (i + 1u) % NDS_AUDIO_FGM_MISS_RING_CAPACITY;
    if (gNdsAudioFgmMissRingCount < NDS_AUDIO_FGM_MISS_RING_CAPACITY)
    {
        gNdsAudioFgmMissRingCount++;
    }
}

static s32 ndsAudioFgmKoIndex(u16 id)
{
    switch (id)
    {
    case nSYAudioVoiceMarioDead:
        return 0;
    case nSYAudioFGMMarioDeadSlam:
        return 1;
    case nSYAudioVoiceFoxDead:
        return 2;
    case nSYAudioFGMFoxDeadSlam:
        return 3;
    case nSYAudioFGMDeadExplodeL:
        return 4;
    default:
        return -1;
    }
}

static s32 ndsAudioFgmPhaseIndex(u16 id)
{
    switch (id)
    {
    case nSYAudioVoicePublicExcited:
        return 0;
    case nSYAudioVoiceAnnounceThree:
        return 1;
    case nSYAudioVoiceAnnounceTwo:
        return 2;
    case nSYAudioVoiceAnnounceOne:
        return 3;
    case nSYAudioVoiceAnnounceGo:
        return 4;
    default:
        return -1;
    }
}

static NDSAudioFgmPackEntry *ndsAudioFgmFindEntry(u16 id)
{
    u32 i;

    for (i = 0u; i < NDS_AUDIO_FGM_ENTRY_COUNT; i++)
    {
        if (sNdsAudioFgmEntries[i].id == id)
        {
            return &sNdsAudioFgmEntries[i];
        }
    }
    return NULL;
}

static void ndsAudioFgmCacheReset(void)
{
    static const u32 capacities[NDS_AUDIO_FGM_CACHE_SLOT_COUNT] = {
        NDS_AUDIO_FGM_CACHE_SLOT_LARGE_BYTES,
        NDS_AUDIO_FGM_CACHE_SLOT_MEDIUM_BYTES,
        NDS_AUDIO_FGM_CACHE_SLOT_MEDIUM_BYTES,
        NDS_AUDIO_FGM_CACHE_SLOT_MEDIUM_BYTES,
        NDS_AUDIO_FGM_CACHE_SLOT_SMALL_BYTES,
        NDS_AUDIO_FGM_CACHE_SLOT_SMALL_BYTES,
        NDS_AUDIO_FGM_CACHE_SLOT_SMALL_BYTES,
        NDS_AUDIO_FGM_CACHE_SLOT_SMALL_BYTES
    };
    u32 offset = 0u;
    u32 i;

    memset(sNdsAudioFgmCacheSlots, 0, sizeof(sNdsAudioFgmCacheSlots));
    for (i = 0u; i < NDS_AUDIO_FGM_CACHE_SLOT_COUNT; i++)
    {
        sNdsAudioFgmCacheSlots[i].data = &sNdsAudioFgmCache[offset];
        sNdsAudioFgmCacheSlots[i].capacity = capacities[i];
        offset += capacities[i];
    }
}

static s32 ndsAudioFgmCacheAcquire(const NDSAudioFgmPackEntry *entry)
{
    s32 best = -1;
    u32 i;

    for (i = 0u; i < NDS_AUDIO_FGM_CACHE_SLOT_COUNT; i++)
    {
        NDSAudioFgmCacheSlot *slot = &sNdsAudioFgmCacheSlots[i];
        if ((slot->fgm_id == entry->id) &&
            (slot->data_bytes == entry->data_bytes))
        {
            return (s32)i;
        }
        if ((slot->references == 0u) &&
            (slot->capacity >= entry->data_bytes) &&
            ((best < 0) ||
             (slot->capacity < sNdsAudioFgmCacheSlots[best].capacity)))
        {
            best = (s32)i;
        }
    }
    if ((best < 0) || (sNdsAudioFgmFile == NULL) ||
        (fseek(sNdsAudioFgmFile, (long)entry->data_offset, SEEK_SET) != 0) ||
        (fread(sNdsAudioFgmCacheSlots[best].data, 1u, entry->data_bytes,
               sNdsAudioFgmFile) != entry->data_bytes))
    {
        gNdsAudioFgmReadFailCount++;
        return -1;
    }
    sNdsAudioFgmCacheSlots[best].fgm_id = entry->id;
    sNdsAudioFgmCacheSlots[best].data_bytes = entry->data_bytes;
    DC_FlushRange(sNdsAudioFgmCacheSlots[best].data, entry->data_bytes);
    return best;
}

static u16 ndsAudioFgmNextInstanceToken(void)
{
    sNdsAudioFgmInstanceToken++;
    if (sNdsAudioFgmInstanceToken == 0u)
    {
        sNdsAudioFgmInstanceToken++;
        gNdsAudioFgmInstanceTokenWrapCount++;
    }
    return sNdsAudioFgmInstanceToken;
}

#if NDS_AUDIO_FGM_ARM7_ACK_DIAGNOSTICS
static s32 ndsAudioFgmIsArm7AckTarget(u16 fgm_id)
{
    return (fgm_id == nSYAudioVoicePublicExcited) ? TRUE : FALSE;
}

static void ndsAudioFgmArm7AckTraceBegin(
    const NDSAudioFgmHandle *handle, const NDSAudioFgmPackEntry *entry)
{
    memset((void *)&gNdsAudioFgmArm7AckTrace, 0,
           sizeof(gNdsAudioFgmArm7AckTrace));
    if (++sNdsAudioFgmArm7AckSequence == 0u)
    {
        sNdsAudioFgmArm7AckSequence = 1u;
    }
    gNdsAudioFgmArm7AckTrace.sequence = sNdsAudioFgmArm7AckSequence;
    gNdsAudioFgmArm7AckTrace.fgm_id = handle->fgm_id;
    gNdsAudioFgmArm7AckTrace.generation = handle->generation;
    gNdsAudioFgmArm7AckTrace.channel = (u32)handle->channel;
    gNdsAudioFgmArm7AckTrace.instance_token = handle->effect.sfx_id;
    gNdsAudioFgmArm7AckTrace.handle_start_tick = handle->start_tick;
    gNdsAudioFgmArm7AckTrace.handle_end_tick = handle->end_tick;
    gNdsAudioFgmArm7AckTrace.duration_ticks = entry->duration_ticks;
    gNdsAudioFgmArm7AckTrace.envelope_count = entry->envelope_count;
}

static void ndsAudioFgmArm7AckTraceRecord(
    const NDSAudioFgmHandle *handle, u32 kind, u32 source_tick, u32 value,
    u32 service_tick, u32 command_tick, u32 command_return_tick,
    u32 acknowledge_tick, u32 active_channels)
{
    volatile NDSAudioFgmArm7AckEvent *event;
    u32 event_index = gNdsAudioFgmArm7AckTrace.event_count;

    if ((gNdsAudioFgmArm7AckTrace.fgm_id != handle->fgm_id) ||
        (gNdsAudioFgmArm7AckTrace.generation != handle->generation) ||
        (gNdsAudioFgmArm7AckTrace.channel != (u32)handle->channel))
    {
        gNdsAudioFgmArm7AckTrace.mismatch_count++;
        return;
    }
    if (event_index >= NDS_AUDIO_FGM_ARM7_ACK_EVENT_CAPACITY)
    {
        gNdsAudioFgmArm7AckTrace.overflow_count++;
        return;
    }

    event = &gNdsAudioFgmArm7AckTrace.events[event_index];
    event->kind = kind;
    event->source_tick = source_tick;
    event->value = value;
    event->service_tick = service_tick;
    event->command_tick = command_tick;
    event->command_return_tick = command_return_tick;
    event->acknowledge_tick = acknowledge_tick;
    event->active_channels = active_channels;
    gNdsAudioFgmArm7AckTrace.event_count = event_index + 1u;
}
#endif

static void ndsAudioFgmReleaseHandle(
    NDSAudioFgmHandle *handle, s32 kill_channel
    NDS_AUDIO_FGM_ACK_RELEASE_PARAMS)
{
    s32 channel = handle->channel;

    if (handle->allocated == FALSE)
    {
        return;
    }

    if ((channel >= 0) && (channel < (s32)NDS_AUDIO_FGM_CHANNEL_COUNT) &&
        (sNdsAudioFgmChannelOwners[channel] == handle) &&
        (sNdsAudioFgmChannelGenerations[channel] == handle->generation))
    {
        if (kill_channel != FALSE)
        {
#if NDS_AUDIO_FGM_ARM7_ACK_DIAGNOSTICS
            if ((release_reason == NDS_AUDIO_FGM_RELEASE_REASON_DURATION) &&
                (ndsAudioFgmIsArm7AckTarget(handle->fgm_id) != FALSE))
            {
                u32 command_tick = cpuGetTiming();
                u32 command_return_tick;
                u32 active_channels;
                u32 acknowledge_tick;

                soundKill(channel);
                command_return_tick = cpuGetTiming();
                active_channels = (u32)soundGetActiveChannels();
                acknowledge_tick = cpuGetTiming();
                ndsAudioFgmArm7AckTraceRecord(
                    handle, NDS_AUDIO_FGM_ARM7_ACK_KIND_STOP,
                    gNdsAudioFgmArm7AckTrace.duration_ticks,
                    release_reason, service_tick, command_tick,
                    command_return_tick, acknowledge_tick, active_channels);
            }
            else
            {
#endif
                soundKill(channel);
#if NDS_AUDIO_FGM_ARM7_ACK_DIAGNOSTICS
            }
#endif
        }
        sNdsAudioFgmChannelOwners[channel] = NULL;
        sNdsAudioFgmChannelGenerations[channel] = 0u;
    }
    else if (handle->live != FALSE)
    {
        gNdsAudioFgmGenerationMismatchCount++;
    }
    if (handle->live != FALSE)
    {
        handle->live = FALSE;
        handle->channel = -1;
        if (gNdsAudioFgmActiveHandles != 0u)
        {
            gNdsAudioFgmActiveHandles--;
        }
    }
    if ((handle->cache_slot >= 0) &&
        (handle->cache_slot < (s8)NDS_AUDIO_FGM_CACHE_SLOT_COUNT) &&
        (sNdsAudioFgmCacheSlots[(u32)handle->cache_slot].references != 0u))
    {
        sNdsAudioFgmCacheSlots[(u32)handle->cache_slot].references--;
    }
    handle->cache_slot = -1;
    handle->effect.sfx_id = 0u;
    handle->fgm_id = 0u;
    handle->allocated = FALSE;
    gNdsAudioFgmHandleReleaseCount++;
}

static NDSAudioFgmHandle *ndsAudioFgmHandleFromEffect(alSoundEffect *effect)
{
    uintptr_t start = (uintptr_t)&sNdsAudioFgmHandles[0];
    uintptr_t end = (uintptr_t)&sNdsAudioFgmHandles[NDS_AUDIO_FGM_HANDLE_COUNT];
    uintptr_t address = (uintptr_t)effect;

    if ((address < start) || (address >= end) ||
        (((address - start) % sizeof(NDSAudioFgmHandle)) != 0u))
    {
        return NULL;
    }
    return (NDSAudioFgmHandle *)effect;
}

/* P2-3 DK 324 (FuraSleep) is a source sequencer cue: three long notes restart
 * the SAME wave at ticks 0/400/810. Baking seven seconds of timeline into one
 * sample exceeds the real 52 KiB cache slot, while retaining the source wave
 * once is only ~19 KiB. Replay the N64 note boundary on DS hardware instead.
 *
 * This is deliberately a rare event path (two calls over a seven-second cue),
 * not per-frame synthesis. It keeps the compressed source wave resident and
 * spends one ARM7 play command at each source retrigger. */
static s32 __attribute__((noinline, cold)) ndsAudioFgmRestartHandleSample(
    NDSAudioFgmHandle *handle, u8 volume, u32 now)
{
    NDSAudioFgmPackEntry *entry;
    NDSAudioFgmHandle *completed_handle;
    s32 old_channel;
    s32 channel;

    if ((handle == NULL) || (handle->live == FALSE) ||
        (handle->cache_slot < 0) ||
        (handle->cache_slot >= (s8)NDS_AUDIO_FGM_CACHE_SLOT_COUNT))
    {
        return FALSE;
    }
    entry = ndsAudioFgmFindEntry(handle->fgm_id);
    if (entry == NULL)
    {
        return FALSE;
    }
    old_channel = handle->channel;
    if ((old_channel < 0) ||
        (old_channel >= (s32)NDS_AUDIO_FGM_CHANNEL_COUNT) ||
        (sNdsAudioFgmChannelOwners[old_channel] != handle) ||
        (sNdsAudioFgmChannelGenerations[old_channel] != handle->generation))
    {
        return FALSE;
    }

    soundKill(old_channel);
    sNdsAudioFgmChannelOwners[old_channel] = NULL;
    sNdsAudioFgmChannelGenerations[old_channel] = 0u;
    handle->channel = -1;

    channel = soundPlaySample(
        sNdsAudioFgmCacheSlots[(u32)handle->cache_slot].data,
        SoundFormat_ADPCM,
        entry->data_bytes - ((u32)entry->loop_point_words * 4u),
        entry->frequency, volume, handle->effect.balance,
        ((entry->flags & 1u) != 0u), entry->loop_point_words);
    if ((channel < 0) || (channel >= (s32)NDS_AUDIO_FGM_CHANNEL_COUNT))
    {
        return FALSE;
    }

    /* soundPlaySample chooses an inactive hardware channel. Its software owner
     * may still be alive because the DS one-shot ended before the source note
     * duration; retire that stale owner without killing the sample just started. */
    completed_handle = sNdsAudioFgmChannelOwners[channel];
    if ((completed_handle != NULL) && (completed_handle != handle))
    {
        if ((completed_handle->allocated == FALSE) ||
            (completed_handle->live == FALSE) ||
            (completed_handle->channel != channel) ||
            (sNdsAudioFgmChannelGenerations[channel] !=
             completed_handle->generation))
        {
            soundKill(channel);
            return FALSE;
        }
        ndsAudioFgmReleaseHandle(
            completed_handle, FALSE
            NDS_AUDIO_FGM_ACK_RELEASE_ARGS(
                NDS_AUDIO_FGM_RELEASE_REASON_DURATION, now));
        gNdsAudioFgmDurationStopCount++;
    }

    handle->channel = (s8)channel;
    handle->volume = volume;
    handle->audible_end_tick =
        now + (u32)(((u64)BUS_CLOCK * entry->sample_count) / entry->frequency);
    sNdsAudioFgmChannelOwners[channel] = handle;
    sNdsAudioFgmChannelGenerations[channel] = handle->generation;
    gNdsAudioFgmChannelMask |= 1u << channel;
    gNdsAudioFgmLastChannel = (u32)channel;
    return TRUE;
}

static s32 ndsAudioFgmValidateCachedEntry(u32 index, const u8 *raw)
{
    NDSAudioFgmPackEntry *entry = &sNdsAudioFgmEntries[index];
    u32 prior;

    entry->id = ndsAudioFgmReadLe16(&raw[0]);
    entry->flags = ndsAudioFgmReadLe16(&raw[2]);
    entry->data_offset = ndsAudioFgmReadLe32(&raw[4]);
    entry->data_bytes = ndsAudioFgmReadLe32(&raw[8]);
    entry->sample_count = ndsAudioFgmReadLe32(&raw[12]);
    entry->frequency = ndsAudioFgmReadLe16(&raw[16]);
    entry->duration_ticks = ndsAudioFgmReadLe16(&raw[18]);
    entry->volume = raw[20];
    entry->pan = raw[21];
    entry->source_sound_index = ndsAudioFgmReadLe16(&raw[22]);
    entry->envelope_offset = ndsAudioFgmReadLe32(&raw[24]);
    entry->envelope_count = ndsAudioFgmReadLe16(&raw[28]);
    entry->loop_point_words = ndsAudioFgmReadLe16(&raw[30]);
    if ((entry->flags & ~1u) || (entry->data_bytes < 4u) ||
        ((entry->data_bytes & 3u) != 0u) ||
        (entry->data_offset < NDS_AUDIO_FGM_PACK_DATA_OFFSET) ||
        (entry->data_offset > NDS_AUDIO_FGM_PACK_BYTES) ||
        (entry->data_bytes > NDS_AUDIO_FGM_PACK_BYTES - entry->data_offset) ||
        (entry->sample_count == 0u) || (entry->frequency == 0u) ||
        (entry->duration_ticks == 0u) || (entry->volume > 127u) ||
        (entry->pan != 64u) ||
        (entry->envelope_count > NDS_AUDIO_FGM_CACHE_MAX_ENVELOPE_POINTS) ||
        /* The loop bit and PNT are one setting: soundPlaySample derives LEN
         * from loop_point_words either way, and a set bit with PNT at the IMA
         * header repeats the state word as audio.  Reject the mismatched
         * halves rather than play one. */
        (((entry->flags & 1u) != 0u) != (entry->loop_point_words != 0u)) ||
        ((u32)entry->loop_point_words * 4u >= entry->data_bytes))
    {
        return FALSE;
    }
    for (prior = 0u; prior < index; prior++)
    {
        if (sNdsAudioFgmEntries[prior].id == entry->id)
        {
            return FALSE;
        }
    }
    return TRUE;
}

void ndsAudioFgmDiagnosticsReset(void)
{
    u32 i;

    /* BattleShip stores a nonzero instance token in sfx_id, snapshots that
     * token in source-side holders, and compares it before stopping a handle.
     * Keep that contract: completed handles clear the token and return to the
     * reusable backend pool. */
    for (i = 0u; i < NDS_AUDIO_FGM_HANDLE_COUNT; i++)
    {
        if (sNdsAudioFgmHandles[i].live != FALSE)
        {
            ndsAudioFgmReleaseHandle(
                &sNdsAudioFgmHandles[i], TRUE
                NDS_AUDIO_FGM_ACK_RELEASE_ARGS(
                    NDS_AUDIO_FGM_RELEASE_REASON_RESET, 0u));
        }
    }
    if (sNdsAudioFgmFile != NULL)
    {
        fclose(sNdsAudioFgmFile);
        sNdsAudioFgmFile = NULL;
    }
    memset(sNdsAudioFgmHandles, 0, sizeof(sNdsAudioFgmHandles));
    memset(sNdsAudioFgmChannelOwners, 0,
           sizeof(sNdsAudioFgmChannelOwners));
    memset(sNdsAudioFgmChannelGenerations, 0,
           sizeof(sNdsAudioFgmChannelGenerations));
    memset(sNdsAudioFgmEntries, 0, sizeof(sNdsAudioFgmEntries));
    for (i = 0u; i < NDS_AUDIO_FGM_HANDLE_COUNT; i++)
    {
        sNdsAudioFgmHandles[i].channel = -1;
        sNdsAudioFgmHandles[i].cache_slot = -1;
    }
    ndsAudioFgmCacheReset();
    if (++sNdsAudioFgmNextGeneration == 0u)
    {
        sNdsAudioFgmNextGeneration = 1u;
    }
#if NDS_AUDIO_FGM_ARM7_ACK_DIAGNOSTICS
    memset((void *)&gNdsAudioFgmArm7AckTrace, 0,
           sizeof(gNdsAudioFgmArm7AckTrace));
    sNdsAudioFgmArm7AckSequence = 0u;
#endif

    gNdsAudioFgmResult = 0u;
    gNdsAudioFgmMask = 0u;
    gNdsAudioFgmLoaded = 0u;
    gNdsAudioFgmResidentBytes = 0u;
    gNdsAudioFgmSupportedCount = 0u;
    gNdsAudioFgmOpenFailCount = 0u;
    gNdsAudioFgmReadFailCount = 0u;
    gNdsAudioFgmFormatFailCount = 0u;
    gNdsAudioFgmPlayCalls = 0u;
    gNdsAudioFgmSupportedPlayCount = 0u;
    gNdsAudioFgmUnsupportedCallCount = 0u;
    gNdsAudioFgmMissRingCount = 0u;
    gNdsAudioFgmMissRingNext = 0u;
    memset((void *)gNdsAudioFgmMissRingIDs, 0,
           sizeof(gNdsAudioFgmMissRingIDs));
    memset((void *)gNdsAudioFgmMissRingCounts, 0,
           sizeof(gNdsAudioFgmMissRingCounts));
    gNdsAudioFgmIncludedLookupFailCount = 0u;
    gNdsAudioFgmPlayFailCount = 0u;
    gNdsAudioFgmPhasePlayMask = 0u;
    memset((void *)gNdsAudioFgmPhasePlayCounts, 0,
           sizeof(gNdsAudioFgmPhasePlayCounts));
    gNdsAudioFgmKoPlayMask = 0u;
    memset((void *)gNdsAudioFgmKoPlayCounts, 0,
           sizeof(gNdsAudioFgmKoPlayCounts));
    gNdsAudioFgmKoTraceCount = 0u;
    memset((void *)gNdsAudioFgmKoTrace, 0,
           sizeof(gNdsAudioFgmKoTrace));
    gNdsAudioFgmLoopPlayCount = 0u;
    gNdsAudioFgmStopCalls = 0u;
    gNdsAudioFgmStopAllCalls = 0u;
    gNdsAudioFgmDurationStopCount = 0u;
    gNdsAudioFgmStaleStopCount = 0u;
    gNdsAudioFgmGenerationMismatchCount = 0u;
    gNdsAudioFgmActiveHandles = 0u;
    gNdsAudioFgmMaxActiveHandles = 0u;
    gNdsAudioFgmChannelMask = 0u;
    gNdsAudioFgmLastChannel = 0xffffffffu;
    gNdsAudioFgmLastID = 0u;
    gNdsAudioFgmLastGeneration = 0u;
    gNdsAudioFgmLastInstanceToken = 0u;
    gNdsAudioFgmInstanceTokenWrapCount = 0u;
    gNdsAudioFgmPoolExhaustCount = 0u;
    gNdsAudioFgmHandleAcquireCount = 0u;
    gNdsAudioFgmHandleReleaseCount = 0u;
    gNdsAudioFgmHandleRecycleCount = 0u;
    gNdsAudioFgmHandleCapacity = NDS_AUDIO_FGM_HANDLE_COUNT;
    gNdsAudioFgmEnvelopeStepCount = 0u;
    gNdsAudioFgmFidelityDebtMask = 0u;
}

void ndsAudioFgmLoadFenced(void)
{
    FILE *file;
    u8 *header = sNdsAudioFgmPack;
    long file_size;
    u32 i;
    u32 sample_end = NDS_AUDIO_FGM_PACK_DATA_OFFSET;
    u32 envelope_cursor;

#if NDS_AUDIO_FGM_ARM7_ACK_DIAGNOSTICS
    /* Keep Calico's low-load manual mode for the two explicit, blocking
     * ID-626 command acknowledgments in this diagnostic-only build. */
    soundSetAutoUpdate(false);
#endif
    if (gNdsAudioFgmLoaded != 0u)
    {
        return;
    }
    file = fopen(NDS_AUDIO_FGM_PATH, "rb");
    if (file == NULL)
    {
        gNdsAudioFgmOpenFailCount++;
        return;
    }
    if ((fseek(file, 0, SEEK_END) != 0) ||
        ((file_size = ftell(file)) != (long)NDS_AUDIO_FGM_PACK_BYTES) ||
        (fseek(file, 0, SEEK_SET) != 0))
    {
        fclose(file);
        gNdsAudioFgmFormatFailCount++;
        return;
    }
    if (fread(sNdsAudioFgmMetadata, 1, NDS_AUDIO_FGM_PACK_DATA_OFFSET,
              file) != NDS_AUDIO_FGM_PACK_DATA_OFFSET)
    {
        fclose(file);
        gNdsAudioFgmReadFailCount++;
        return;
    }

    if ((memcmp(header, "FGM1", 4) != 0) ||
        (ndsAudioFgmReadLe16(&header[4]) != 4u) ||
        (ndsAudioFgmReadLe16(&header[6]) != NDS_AUDIO_FGM_ENTRY_COUNT) ||
        (ndsAudioFgmReadLe32(&header[8]) != NDS_AUDIO_FGM_PACK_BYTES) ||
        (ndsAudioFgmReadLe32(&header[12]) !=
         NDS_AUDIO_FGM_PACK_MAPPING_SHA256_LO))
    {
        fclose(file);
        gNdsAudioFgmFormatFailCount++;
        return;
    }
    for (i = 0u; i < NDS_AUDIO_FGM_ENTRY_COUNT; i++)
    {
        const u8 *raw = &sNdsAudioFgmPack[
            NDS_AUDIO_FGM_PACK_HEADER_BYTES +
            (i * NDS_AUDIO_FGM_PACK_ENTRY_BYTES)];

        if (ndsAudioFgmValidateCachedEntry(i, raw) == FALSE)
        {
            memset(sNdsAudioFgmEntries, 0,
                   sizeof(sNdsAudioFgmEntries));
            fclose(file);
            gNdsAudioFgmFormatFailCount++;
            return;
        }
    }
    for (i = 0u; i < NDS_AUDIO_FGM_ENTRY_COUNT; i++)
    {
        u32 entry_end = sNdsAudioFgmEntries[i].data_offset +
                        sNdsAudioFgmEntries[i].data_bytes;

        if (entry_end > sample_end)
        {
            sample_end = entry_end;
        }
    }
    envelope_cursor = sample_end;
    for (i = 0u; i < NDS_AUDIO_FGM_ENTRY_COUNT; i++)
    {
        const NDSAudioFgmPackEntry *entry = &sNdsAudioFgmEntries[i];

        if (entry->envelope_count != 0u)
        {
            if (entry->envelope_offset != envelope_cursor)
            {
                break;
            }
            envelope_cursor += (u32)entry->envelope_count *
                               NDS_AUDIO_FGM_ENVELOPE_POINT_BYTES;
        }
    }
    if ((sNdsAudioFgmEntries[0].data_offset !=
         NDS_AUDIO_FGM_PACK_DATA_OFFSET) ||
        (i != NDS_AUDIO_FGM_ENTRY_COUNT) ||
        (envelope_cursor != NDS_AUDIO_FGM_PACK_BYTES))
    {
        memset(sNdsAudioFgmEntries, 0, sizeof(sNdsAudioFgmEntries));
        fclose(file);
        gNdsAudioFgmFormatFailCount++;
        return;
    }

    /* The production boot path reaches this load fence without ever calling
     * ndsAudioFgmDiagnosticsReset, so the cache slot table must be initialized
     * here or every ndsAudioFgmCacheAcquire fails eligibility (capacity 0). */
    ndsAudioFgmCacheReset();
    for (i = 0u; i < NDS_AUDIO_FGM_HANDLE_COUNT; i++)
    {
        sNdsAudioFgmHandles[i].channel = -1;
        sNdsAudioFgmHandles[i].cache_slot = -1;
    }

    sNdsAudioFgmFile = file;
    gNdsAudioFgmLoaded = 1u;
    gNdsAudioFgmResidentBytes = NDS_AUDIO_FGM_CACHE_BYTES +
                                NDS_AUDIO_FGM_PACK_DATA_OFFSET;
    gNdsAudioFgmSupportedCount = NDS_AUDIO_FGM_ENTRY_COUNT;
    gNdsAudioFgmHandleCapacity = NDS_AUDIO_FGM_HANDLE_COUNT;
    gNdsAudioFgmMask |= NDS_AUDIO_FGM_MASK_PACK_LOADED;
    gNdsAudioFgmFidelityDebtMask =
        NDS_AUDIO_FGM_EXPECTED_FIDELITY_DEBT_MASK;
    gNdsAudioFgmResult = NDS_AUDIO_FGM_PASS;
}

void ndsAudioFgmUpdate(void)
{
    u32 now;
    u32 i;

    if (gNdsAudioFgmActiveHandles == 0u)
    {
        return;
    }
    now = cpuGetTiming();
    for (i = 0u; i < NDS_AUDIO_FGM_HANDLE_COUNT; i++)
    {
        NDSAudioFgmHandle *handle = &sNdsAudioFgmHandles[i];

        if (handle->live != FALSE)
        {
            u32 elapsed_cpu_ticks = now - handle->start_tick;
            u32 elapsed_fgm_ticks = (u32)(
                ((u64)elapsed_cpu_ticks * 1000000u) /
                ((u64)BUS_CLOCK * NDS_AUDIO_FGM_TIMER_MICROSECONDS));

            while (handle->envelope_index < handle->envelope_count)
            {
                const u8 *point = &handle->envelope_points[
                    (u32)handle->envelope_index *
                    NDS_AUDIO_FGM_ENVELOPE_POINT_BYTES];
                u16 point_tick = ndsAudioFgmReadLe16(point);
                u8 event_flags = point[3];

                if (elapsed_fgm_ticks < point_tick)
                {
                    break;
                }
                if ((event_flags & ~NDS_AUDIO_FGM_EVENT_RESTART_SAMPLE) != 0u)
                {
                    gNdsAudioFgmFormatFailCount++;
                    gNdsAudioFgmPlayFailCount++;
                    ndsAudioFgmReleaseHandle(
                        handle, TRUE
                        NDS_AUDIO_FGM_ACK_RELEASE_ARGS(
                            NDS_AUDIO_FGM_RELEASE_REASON_DURATION, now));
                    break;
                }
                if ((event_flags & NDS_AUDIO_FGM_EVENT_RESTART_SAMPLE) != 0u)
                {
                    if (ndsAudioFgmRestartHandleSample(handle, point[2], now) ==
                        FALSE)
                    {
                        gNdsAudioFgmPlayFailCount++;
                        ndsAudioFgmReleaseHandle(
                            handle, FALSE
                            NDS_AUDIO_FGM_ACK_RELEASE_ARGS(
                                NDS_AUDIO_FGM_RELEASE_REASON_GENERATION_LOST,
                                now));
                        break;
                    }
                }
                if ((handle->channel < 0) ||
                    (sNdsAudioFgmChannelOwners[(u32)handle->channel] !=
                     handle) ||
                    (sNdsAudioFgmChannelGenerations[(u32)handle->channel] !=
                     handle->generation))
                {
                    gNdsAudioFgmGenerationMismatchCount++;
                    ndsAudioFgmReleaseHandle(
                        handle, FALSE
                        NDS_AUDIO_FGM_ACK_RELEASE_ARGS(
                            NDS_AUDIO_FGM_RELEASE_REASON_GENERATION_LOST,
                            now));
                    break;
                }
                soundSetVolume(handle->channel, point[2]);
                handle->volume = point[2];
                handle->envelope_index++;
                gNdsAudioFgmEnvelopeStepCount++;
            }
            /* THE NOTE ENDING IS A RELEASE, NOT A KILL, and this used to be a
             * bare soundKill the instant end_tick passed.
             *
             * BUGS.md "Some Crowd noise audio cues get cut off". Five cues in
             * the pack own a sample LONGER than the note that plays them, so
             * that kill landed mid-waveform -- a click, and a lost tail. Four
             * of the five are the crowd, which is exactly the row:
             *   PublicDamageM  862 ms note vs 1,172 ms sample   -309 ms
             *   PublicGaspM  1,322 ms note vs 1,618 ms sample   -295 ms
             *   PublicMario  1,840 ms note vs 2,123 ms sample   -283 ms
             *   GroundGrind2   316 ms note vs   457 ms sample   -141 ms
             *   PublicFox    1,840 ms note vs 1,969 ms sample   -129 ms
             * and all five carry zero packed envelope points, so nothing was
             * fading them either. The other eight crowd cues run out before
             * their note ends, which is what the N64 does too -- their samples
             * do not loop (source_loop_infinite false, loop_start/end 0) -- so
             * they are untouched by this.
             *
             * The N64 sequence player hands a finished note to the envelope's
             * release phase rather than silencing the voice, so ramping to zero
             * and then killing is the source-shaped behaviour, not a cosmetic
             * fade. The window is one constant rather than per-cue release data
             * because the pack does not carry release yet; see the note on
             * NDS_AUDIO_FGM_RELEASE_MICROSECONDS. */
            if (handle->live != FALSE)
            {
                /* ...AND THE RELEASE STARTS AT WHICHEVER END COMES LAST.
                 *
                 * The ramp above turned the note-end kill from a click into a
                 * fade, but it still SILENCED a one-shot that was genuinely
                 * mid-waveform: for the five cues whose sample outlives their
                 * note (PublicDamageM/GaspM/Mario/Fox, GroundGrind2 -- four of
                 * them crowd) the ramp began 129-309 ms before the sample was
                 * done, so the tail was faded out rather than played. The owner
                 * still hears that as "some crowd noise audio cues get cut off".
                 *
                 * A non-looping DS voice stops itself when the sample runs out,
                 * so waiting for audible_end_tick costs nothing audible on the
                 * other 83 cues -- their samples finish first and this reduces
                 * to the old test -- and on these five it lets the waveform
                 * finish, which is what the N64 voice does. A LOOPING voice
                 * never reaches audible_end_tick and must still be bounded by
                 * the note, hence handle->loops.
                 *
                 * The cost is that a long one-shot holds its handle and its
                 * cache slot a little longer, and the pool is eight deep with a
                 * measured high-water of eight. gNdsAudioFgmPoolExhaustCount is
                 * the counter that would show it; it must stay 0. */
                u32 release_from = handle->end_tick;
                s32 into_release;

                if ((handle->loops == FALSE) &&
                    (handle->audible_end_tick != 0u) &&
                    ((s32)(handle->audible_end_tick - release_from) > 0))
                {
                    release_from = handle->audible_end_tick;
                }
                into_release = (s32)(now - release_from);

                if (into_release >= (s32)ndsAudioFgmReleaseCpuTicks())
                {
                    ndsAudioFgmReleaseHandle(
                        handle, TRUE
                        NDS_AUDIO_FGM_ACK_RELEASE_ARGS(
                            NDS_AUDIO_FGM_RELEASE_REASON_DURATION, now));
                    gNdsAudioFgmDurationStopCount++;
                }
                else if ((into_release >= 0) && (handle->channel >= 0))
                {
                    u32 span = ndsAudioFgmReleaseCpuTicks();
                    u32 remaining = span - (u32)into_release;

                    soundSetVolume(
                        handle->channel,
                        (u8)(((u32)handle->volume * remaining) / span));
                    gNdsAudioFgmReleaseRampCount++;
                }
            }
        }
    }
}

void ndsAudioFgmStopAll(void)
{
    u32 i;

    gNdsAudioFgmStopAllCalls++;
    for (i = 0u; i < NDS_AUDIO_FGM_HANDLE_COUNT; i++)
    {
        if (sNdsAudioFgmHandles[i].live != FALSE)
        {
            ndsAudioFgmReleaseHandle(
                &sNdsAudioFgmHandles[i], TRUE
                NDS_AUDIO_FGM_ACK_RELEASE_ARGS(
                    NDS_AUDIO_FGM_RELEASE_REASON_STOP_ALL, 0u));
        }
    }
}

void ndsAudioFgmStop(alSoundEffect *effect)
{
    NDSAudioFgmHandle *handle;

    gNdsAudioFgmStopCalls++;
    if (effect == NULL)
    {
        return;
    }
    handle = ndsAudioFgmHandleFromEffect(effect);
    if ((handle == NULL) || (handle->allocated == FALSE) ||
        (handle->live == FALSE))
    {
        gNdsAudioFgmStaleStopCount++;
        return;
    }
    ndsAudioFgmReleaseHandle(
        handle, TRUE
        NDS_AUDIO_FGM_ACK_RELEASE_ARGS(
            NDS_AUDIO_FGM_RELEASE_REASON_EXPLICIT, 0u));
}

alSoundEffect *ndsAudioFgmPlayAtPan(u16 fgm_id, u8 pan)
{
    NDSAudioFgmPackEntry *entry;
    NDSAudioFgmHandle *handle = NULL;
    s32 phase_index;
    s32 ko_index;
    s32 channel;
    s32 cache_slot;
    u32 i;
    u32 duration_cpu_ticks;
#if NDS_AUDIO_FGM_ARM7_ACK_DIAGNOSTICS
    u32 play_command_tick = 0u;
    u32 play_command_return_tick = 0u;
#endif

    gNdsAudioFgmPlayCalls++;
    gNdsAudioFgmLastID = fgm_id;
    ndsAudioFgmUpdate();
    entry = ndsAudioFgmFindEntry(fgm_id);
    if (entry == NULL)
    {
        if (ndsAudioFgmIDIsIncluded(fgm_id) != FALSE)
        {
            gNdsAudioFgmIncludedLookupFailCount++;
            gNdsAudioFgmPlayFailCount++;
        }
        else
        {
            gNdsAudioFgmUnsupportedCallCount++;
            ndsAudioFgmRecordMiss(fgm_id);
        }
        return NULL;
    }
    if ((gNdsAudioFgmLoaded == 0u) ||
        (gNdsAudioFgmResult != NDS_AUDIO_FGM_PASS))
    {
        gNdsAudioFgmIncludedLookupFailCount++;
        gNdsAudioFgmPlayFailCount++;
        return NULL;
    }
    for (i = 0u; i < NDS_AUDIO_FGM_HANDLE_COUNT; i++)
    {
        if (sNdsAudioFgmHandles[i].allocated == FALSE)
        {
            handle = &sNdsAudioFgmHandles[i];
            break;
        }
    }
    if (handle == NULL)
    {
        gNdsAudioFgmPoolExhaustCount++;
        gNdsAudioFgmPlayFailCount++;
        return NULL;
    }
    cache_slot = ndsAudioFgmCacheAcquire(entry);
    if (cache_slot < 0)
    {
        gNdsAudioFgmPlayFailCount++;
        return NULL;
    }
    if ((entry->envelope_count != 0u) &&
        ((fseek(sNdsAudioFgmFile, (long)entry->envelope_offset, SEEK_SET) != 0) ||
         (fread(handle->envelope_points, NDS_AUDIO_FGM_ENVELOPE_POINT_BYTES,
                entry->envelope_count, sNdsAudioFgmFile) !=
          entry->envelope_count)))
    {
        gNdsAudioFgmReadFailCount++;
        gNdsAudioFgmPlayFailCount++;
        return NULL;
    }

    NDS_FREEZE_DIAGNOSTICS_FGM_ENTER(fgm_id);
    soundEnable();
#if NDS_AUDIO_FGM_ARM7_ACK_DIAGNOSTICS
    if (ndsAudioFgmIsArm7AckTarget(fgm_id) != FALSE)
    {
        play_command_tick = cpuGetTiming();
    }
#endif
    channel = soundPlaySample(
        sNdsAudioFgmCacheSlots[cache_slot].data, SoundFormat_ADPCM,
        entry->data_bytes - ((u32)entry->loop_point_words * 4u),
        entry->frequency, entry->volume, pan,
        ((entry->flags & 1u) != 0u), entry->loop_point_words);
    NDS_FREEZE_DIAGNOSTICS_FGM_RETURN(channel);
#if NDS_AUDIO_FGM_ARM7_ACK_DIAGNOSTICS
    if (ndsAudioFgmIsArm7AckTarget(fgm_id) != FALSE)
    {
        play_command_return_tick = cpuGetTiming();
    }
#endif
    if ((channel < 0) || (channel >= (s32)NDS_AUDIO_FGM_CHANNEL_COUNT))
    {
        gNdsAudioFgmPlayFailCount++;
        return NULL;
    }
    if (sNdsAudioFgmChannelOwners[channel] != NULL)
    {
        NDSAudioFgmHandle *completed_handle =
            sNdsAudioFgmChannelOwners[channel];

        /* soundPlaySample synchronizes with ARM7 and only selects an inactive
         * hardware channel. Retire its completed software owner instead of
         * killing the newly started sample when the source-duration clock
         * trails the hardware one-shot completion. */
        if ((completed_handle->allocated != FALSE) &&
            (completed_handle->live != FALSE) &&
            (completed_handle->channel == channel) &&
            (sNdsAudioFgmChannelGenerations[channel] ==
             completed_handle->generation))
        {
            /* BUGS.md "Some Crowd noise audio cues get cut off (the for big
             * hits)". The retire above is justified by soundPlaySample only
             * choosing an INACTIVE hardware channel -- so the owner must be
             * finished. Measure that rather than trust it.
             *
             * Measure it against audible_end_tick, NOT end_tick. This test was
             * written against end_tick first and reported 3 hits over a 5-minute
             * both-CPU soak, which read as confirmed channel contention and was
             * wrong: end_tick is the source note length and every one of the 88
             * cues outlives its own DS sample, so that form fires on ordinary
             * completion and can only ever over-report. Against the audible end
             * a hit means the hardware channel was reused while this cue was
             * genuinely still sounding, which is the row's mechanism; zero
             * clears contention and moves the search to the release ramp. */
            u32 retire_now = cpuGetTiming();

            if ((completed_handle->audible_end_tick != 0u) &&
                ((s32)(completed_handle->audible_end_tick - retire_now) > 0))
            {
                gNdsAudioFgmPrematureRetireCount++;
                gNdsAudioFgmPrematureRetireLastID = completed_handle->fgm_id;
            }
            ndsAudioFgmReleaseHandle(
                completed_handle, FALSE
                NDS_AUDIO_FGM_ACK_RELEASE_ARGS(
                    NDS_AUDIO_FGM_RELEASE_REASON_DURATION,
                    retire_now));
            gNdsAudioFgmDurationStopCount++;
        }
        else
        {
            soundKill(channel);
            gNdsAudioFgmGenerationMismatchCount++;
            gNdsAudioFgmPlayFailCount++;
            return NULL;
        }
    }

    memset(&handle->effect, 0, sizeof(handle->effect));
    handle->effect.sfx_id = ndsAudioFgmNextInstanceToken();
    handle->effect.balance = pan;
    handle->fgm_id = fgm_id;
    handle->generation = sNdsAudioFgmNextGeneration++;
    if (sNdsAudioFgmNextGeneration == 0u)
    {
        sNdsAudioFgmNextGeneration = 1u;
    }
    duration_cpu_ticks = (u32)(((u64)BUS_CLOCK * entry->duration_ticks *
                                NDS_AUDIO_FGM_TIMER_MICROSECONDS) /
                               1000000u);
    handle->start_tick = cpuGetTiming();
    handle->end_tick = handle->start_tick + duration_cpu_ticks;
    handle->audible_end_tick =
        handle->start_tick +
        (u32)(((u64)BUS_CLOCK * entry->sample_count) / entry->frequency);
    handle->volume = entry->volume;
    handle->loops = (((entry->flags & 1u) != 0u) ? TRUE : FALSE);
    handle->envelope_count = entry->envelope_count;
    handle->envelope_index = 0u;
    handle->channel = (s8)channel;
    handle->cache_slot = (s8)cache_slot;
    sNdsAudioFgmCacheSlots[cache_slot].references++;
    if (handle->ever_allocated != FALSE)
    {
        gNdsAudioFgmHandleRecycleCount++;
    }
    handle->ever_allocated = TRUE;
    handle->allocated = TRUE;
    handle->live = TRUE;
    sNdsAudioFgmChannelOwners[channel] = handle;
    sNdsAudioFgmChannelGenerations[channel] = handle->generation;

#if NDS_AUDIO_FGM_ARM7_ACK_DIAGNOSTICS
    if (ndsAudioFgmIsArm7AckTarget(fgm_id) != FALSE)
    {
        u32 active_channels;
        u32 acknowledge_tick;

        ndsAudioFgmArm7AckTraceBegin(handle, entry);
        active_channels = (u32)soundGetActiveChannels();
        acknowledge_tick = cpuGetTiming();
        ndsAudioFgmArm7AckTraceRecord(
            handle, NDS_AUDIO_FGM_ARM7_ACK_KIND_PLAY, 0u, entry->volume,
            handle->start_tick, play_command_tick, play_command_return_tick,
            acknowledge_tick, active_channels);
    }
#endif

    gNdsAudioFgmSupportedPlayCount++;
    gNdsAudioFgmHandleAcquireCount++;
    gNdsAudioFgmActiveHandles++;
    if (gNdsAudioFgmActiveHandles > gNdsAudioFgmMaxActiveHandles)
    {
        gNdsAudioFgmMaxActiveHandles = gNdsAudioFgmActiveHandles;
    }
    gNdsAudioFgmChannelMask |= 1u << channel;
    gNdsAudioFgmLastChannel = (u32)channel;
    gNdsAudioFgmLastGeneration = handle->generation;
    gNdsAudioFgmLastInstanceToken = handle->effect.sfx_id;
    gNdsAudioFgmMask |= NDS_AUDIO_FGM_MASK_SUPPORTED_PLAY;
    if ((entry->flags & 1u) != 0u)
    {
        gNdsAudioFgmLoopPlayCount++;
        gNdsAudioFgmMask |= NDS_AUDIO_FGM_MASK_LOOP_PLAY;
    }
    phase_index = ndsAudioFgmPhaseIndex(fgm_id);
    if (phase_index >= 0)
    {
        gNdsAudioFgmPhasePlayCounts[phase_index]++;
        gNdsAudioFgmPhasePlayMask |= 1u << phase_index;
        if (gNdsAudioFgmPhasePlayMask ==
            NDS_AUDIO_FGM_PHASE_COMPLETE_MASK)
        {
            gNdsAudioFgmMask |= NDS_AUDIO_FGM_MASK_PHASE_COMPLETE;
        }
    }
    ko_index = ndsAudioFgmKoIndex(fgm_id);
    if (ko_index >= 0)
    {
        gNdsAudioFgmKoPlayCounts[ko_index]++;
        gNdsAudioFgmKoPlayMask |= 1u << ko_index;
        if (gNdsAudioFgmKoTraceCount < NDS_AUDIO_FGM_KO_TRACE_CAPACITY)
        {
            gNdsAudioFgmKoTrace[gNdsAudioFgmKoTraceCount++] = fgm_id;
        }
    }
    return &handle->effect;
}

alSoundEffect *ndsAudioFgmPlay(u16 fgm_id)
{
    return ndsAudioFgmPlayAtPan(fgm_id, 64u);
}
