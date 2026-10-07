/* P3 net layer: what the lobby (nds_net_lobby.c) and the lockstep
 * (nds_net_session.c) share. Not a public header. */
#ifndef NDS_NET_INTERNAL_H
#define NDS_NET_INTERNAL_H

#include <stdint.h>

#define NDS_NET_PROTO_VERSION     1u
#define NDS_NET_HDR_BYTES         8u

/* Message kinds (the header's fourth byte). */
#define NDS_NET_KIND_HELLO        1u  /* lab discovery */
#define NDS_NET_KIND_START        2u  /* host -> all: descriptor, seed, ports */
#define NDS_NET_KIND_START_ACK    3u  /* guest -> host */
#define NDS_NET_KIND_INPUT        4u  /* lockstep records and digest */
#define NDS_NET_KIND_LOBBY        5u  /* host -> all: room and lobby snapshot */
#define NDS_NET_KIND_JOIN_REQ     6u  /* guest -> host */
#define NDS_NET_KIND_JOIN_ACK     7u  /* host -> all, addressed by MAC */
#define NDS_NET_KIND_PROPOSE      8u  /* guest -> host: its own slot */
#define NDS_NET_KIND_LEAVE        9u  /* guest leaves, or host closes the room */

static inline uint32_t ndsNetGet32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) |
           ((uint32_t)p[3] << 24);
}

static inline void ndsNetPut32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16);
    p[3] = (uint8_t)(v >> 24);
}

static inline uint32_t ndsNetGet16(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8);
}

static inline void ndsNetPut16(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
}

static inline uint32_t ndsNetHeaderWrite(uint8_t *buf, uint32_t kind, uint32_t session)
{
    buf[0] = 'S';
    buf[1] = '6';
    buf[2] = NDS_NET_PROTO_VERSION;
    buf[3] = (uint8_t)kind;
    ndsNetPut32(buf + 4, session);
    return NDS_NET_HDR_BYTES;
}

static inline uint32_t ndsNetHeaderRead(const uint8_t *buf, uint32_t len,
                                        uint32_t *kind, uint32_t *session)
{
    if (len < NDS_NET_HDR_BYTES || buf[0] != 'S' || buf[1] != '6' ||
        buf[2] != NDS_NET_PROTO_VERSION)
        return 0u;
    *kind = buf[3];
    *session = ndsNetGet32(buf + 4);
    return 1u;
}

/* nds_net_session.c: arm the lockstep for the next battle; the seed is
 * installed when the VS battle scene starts (ndsNetBattleSceneStart), before
 * its setup draws any random number. Disarm after the match. */
void ndsNetLockstepConfigure(uint32_t session, uint32_t local_port,
                             uint32_t host_port, uint32_t human_mask);
void ndsNetLockstepSetSeed(uint32_t seed);
void ndsNetLockstepDisarm(void);

#endif
