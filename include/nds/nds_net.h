/* P3 local multiplayer, ARM9 API (docs/P3_Multiplayer/Smash64DS_Multiplayer_Plan.md). */
#ifndef NDS_NET_H
#define NDS_NET_H

#include <stdint.h>

#define NDS_NET_OK            0
#define NDS_NET_ERR_MODULE   -1   /* net/net7.bin missing or rejected */
#define NDS_NET_ERR_TIMEOUT  -2   /* the ARM7 did not answer */
#define NDS_NET_ERR_RADIO    -3   /* the ARM7 reported a radio error */
#define NDS_NET_ERR_DOWN     -4   /* radio not running */
#define NDS_NET_ERR_SIZE     -5   /* payload empty or too large */
#define NDS_NET_ERR_FULL     -6   /* TX ring full */

/* The channel every room uses (owner, 2026-10-18: 13, hard-coded first).
 * Nintendo's own local play uses 1, 7 and 13; US Wi-Fi routers sit on 1, 6
 * and 11 and never on 12-13, so 13 is usually the quietest. The ARM7 refuses
 * a channel the console's firmware calibration does not enable
 * (NDS_NET_ERR_CHANNEL); retail units enable 1-13 in every region. */
#define NDS_NET_DEFAULT_CHANNEL 13u

/* --- Link (src/nds/net/nds_net_link.c) --------------------------------- */
int ndsNetLinkAttach(void);
int ndsNetLinkStart(uint32_t channel);
void ndsNetLinkStop(void);
uint32_t ndsNetLinkRunning(void);
void ndsNetLinkGetMac(uint8_t mac[6]);
/* Broadcasts one payload (1..NDS_NET_MAX_PAYLOAD bytes) to every console on
 * the channel. Odd lengths may arrive with one byte of padding, so payloads
 * carry their own length. */
int ndsNetLinkSendPacket(const void *data, uint32_t len);
/* Takes the oldest received payload: its length (0 if none, or if it does not
 * fit `capacity` -- it is consumed either way) and the sender's MAC. */
uint32_t ndsNetLinkRecvPacket(uint8_t src_mac[6], void *buffer, uint32_t capacity);
uint32_t ndsNetLinkStat(uint32_t index);

#if NDS_NET_LAB_PING
/* Lab: exchange packets for `frames` VBlanks at boot (gNdsNetLabPing*). */
void ndsNetLabPingRun(uint32_t frames);
#endif

#endif
