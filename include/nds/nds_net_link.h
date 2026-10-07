/* P3 local wireless link: the contract between the ARM9 net layer
 * (src/nds/net/nds_net_link.c), the ARM7's resident PXI shim
 * (src/nds/arm7/nds_net_arm7.c) and the ARM7 radio module
 * (src/nds/arm7/nds_net7_module.c).
 *
 * The radio is Calico's own Mitsumi driver (mwl), run in infrastructure mode
 * under a game BSSID with no access point: every console broadcasts data
 * frames that carry FromDS=1 and the game BSSID, which the driver's receive
 * filter for that mode accepts, and the module marks the station associated so
 * the driver hands those frames to its data callback. One hop between any two
 * consoles, no association, no beacons (docs/P3_Multiplayer, transport T1).
 *
 * MEMORY. The module and its rings live in the homebrew bootstub area of the
 * high shared main RAM (0x02FF4000..0x02FFC000): above the ARM7's main-RAM
 * image, so the ARM9 arena is unchanged, and uncached on the ARM9 (Calico's
 * MPU region 4 covers 0x02FF0000..0x02FFFFFF without caching), so both CPUs
 * see the rings without cache maintenance. Offline play never loads the
 * module: the ARM9 copies it from NitroFS when a net session starts. Loading
 * it overwrites the loader's return stub, so a console that has hosted or
 * joined a session cannot exit back to its loader menu. */
#ifndef NDS_NET_LINK_H
#define NDS_NET_LINK_H

#include <stdint.h>

#define NDS_NET_AREA_BASE       0x02FF4000u
#define NDS_NET_MODULE_MAX      0x6000u     /* image + BSS */
#define NDS_NET_SHARED_ADDR     0x02FFA000u
#define NDS_NET_SHARED_SIZE     0x2000u

#define NDS_NET_PXI_CHANNEL     25u         /* Calico PxiChannel_User2 */
#define NDS_NET7_MAGIC          0x374E3653u /* "S6N7" */
#define NDS_NET_SHARED_MAGIC    0x484E3653u /* "S6NH" */
#define NDS_NET_ABI             1u

/* PXI words, ARM9 -> ARM7: command in bits 0..3, argument in bits 4..25. */
#define NDS_NET_CMD_ATTACH      1u  /* module image is in place: start it */
#define NDS_NET_CMD_START       2u  /* argument: channel 1..13 */
#define NDS_NET_CMD_STOP        3u
#define NDS_NET_CMD_KICK        4u  /* the TX ring has new packets */
#define NDS_NET_CMD(cmd, arg)   (((uint32_t)(cmd) & 0xFu) | (((uint32_t)(arg) & 0x3FFFFFu) << 4))

/* NdsNetShared.state, written by the ARM7. */
#define NDS_NET_STATE_OFF       0u
#define NDS_NET_STATE_ATTACHED  1u  /* module thread running, radio off */
#define NDS_NET_STATE_RUNNING   2u  /* radio on, sending and receiving */
#define NDS_NET_STATE_ERROR     3u

/* NdsNetShared.error */
#define NDS_NET_ERR_NONE        0u
#define NDS_NET_ERR_CALIB       1u  /* no Mitsumi calibration data */
#define NDS_NET_ERR_CHANNEL     2u  /* channel not enabled by the firmware */

/* Largest application payload in one frame: 24-byte 802.11 header plus this
 * fits the driver's 256-byte packet buffers. */
#define NDS_NET_MAX_PAYLOAD     232u

#define NDS_NET_TX_RING_SIZE    2048u
#define NDS_NET_RX_RING_SIZE    5888u

/* Ring records are 4-byte aligned: a u16 payload length, u16 flags, then (RX
 * only) the sender's 6-byte MAC address and 2 bytes of padding, then the
 * payload. A length of 0xFFFF marks the unused tail before a wrap. */
#define NDS_NET_RING_WRAP       0xFFFFu
#define NDS_NET_RX_HDR_BYTES    12u
#define NDS_NET_TX_HDR_BYTES    4u

/* The game BSSID: locally administered, unicast. Every Smash64DS console uses
 * it; sessions are told apart by the application header. */
#define NDS_NET_BSSID_INIT      { 0x02, 0x53, 0x36, 0x34, 0x44, 0x53 }

/* Statistics words in NdsNetShared.stats. */
enum
{
    NDS_NET_STAT_TX_SENT = 0,
    NDS_NET_STAT_TX_DROPPED,
    NDS_NET_STAT_TX_ERRORS,
    NDS_NET_STAT_RX_FRAMES,
    NDS_NET_STAT_RX_RING_FULL,
    NDS_NET_STAT_RX_FOREIGN,
    NDS_NET_STAT_TX_NO_BUFFER,
    NDS_NET_STAT_KICKS,
    NDS_NET_STAT_COUNT
};

typedef struct NdsNetShared
{
    uint32_t magic;               /* ARM9: NDS_NET_SHARED_MAGIC before attach */
    uint32_t abi;                 /* ARM9: NDS_NET_ABI */
    volatile uint32_t state;      /* ARM7 */
    volatile uint32_t error;      /* ARM7 */
    volatile uint32_t channel;    /* ARM7, once running */
    volatile uint8_t mac[8];      /* ARM7, once attached (6 used) */
    volatile uint32_t tx_head;    /* ARM9 produces */
    volatile uint32_t tx_tail;    /* ARM7 consumes */
    volatile uint32_t rx_head;    /* ARM7 produces */
    volatile uint32_t rx_tail;    /* ARM9 consumes */
    volatile uint32_t stats[NDS_NET_STAT_COUNT];
    uint8_t tx_ring[NDS_NET_TX_RING_SIZE];
    uint8_t rx_ring[NDS_NET_RX_RING_SIZE];
} NdsNetShared;

_Static_assert(sizeof(NdsNetShared) <= NDS_NET_SHARED_SIZE, "net shared block too large");
_Static_assert(NDS_NET_AREA_BASE + NDS_NET_MODULE_MAX <= NDS_NET_SHARED_ADDR, "net module overlaps rings");

/* The module image starts with this header (linker/nds_arm7_net7.ld). */
typedef struct NdsNet7Header
{
    uint32_t magic;
    uint32_t abi;
    uint32_t image_end;           /* address: end of the bytes the loader copies */
    uint32_t bss_end;             /* address: end of BSS, which the loader zeroes */
    void (*attach)(void);         /* IRQ context: start the module thread */
    void (*post)(uint32_t word);  /* IRQ context: queue a PXI word */
} NdsNet7Header;

#define gNdsNetShared ((NdsNetShared *)NDS_NET_SHARED_ADDR)

#endif
