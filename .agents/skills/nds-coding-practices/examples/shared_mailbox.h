#ifndef NDS_PRACTICES_SHARED_MAILBOX_H
#define NDS_PRACTICES_SHARED_MAILBOX_H

/*
 * Layout pattern for a bounded ARM9/ARM7 mailbox.
 *
 * Prefer current Calico/libnds message services. Use raw shared memory only
 * when the project needs it and supplies the required cache publication,
 * barriers, notifications, acknowledgements, and reset protocol.
 */
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
#define NDS_MAILBOX_ASSERT static_assert
#else
#define NDS_MAILBOX_ASSERT _Static_assert
#endif

#define NDS_MAILBOX_CACHE_LINE 32u
#define NDS_MAILBOX_PAYLOAD_BYTES 48u

struct NdsMailboxMessage {
    uint16_t type;
    uint16_t byte_count;
    uint32_t generation;
    uint32_t sequence;
    uint8_t payload[NDS_MAILBOX_PAYLOAD_BYTES];
};

/*
 * Producer and consumer publication fields are placed on distinct cache lines
 * so ARM9 cache maintenance never discards unrelated ownership. The exact
 * padding and memory section must be confirmed against the project's linker
 * script and runtime.
 */
struct __attribute__((aligned(NDS_MAILBOX_CACHE_LINE))) NdsMailbox {
    struct NdsMailboxMessage command;
    uint8_t command_padding[4];

    uint32_t producer_sequence;
    uint8_t producer_padding[NDS_MAILBOX_CACHE_LINE - sizeof(uint32_t)];

    uint32_t consumer_sequence;
    uint8_t consumer_padding[NDS_MAILBOX_CACHE_LINE - sizeof(uint32_t)];
};

NDS_MAILBOX_ASSERT(sizeof(struct NdsMailboxMessage) == 60,
               "mailbox message layout changed");
NDS_MAILBOX_ASSERT((sizeof(struct NdsMailbox) % NDS_MAILBOX_CACHE_LINE) == 0,
               "mailbox must occupy complete cache lines");

NDS_MAILBOX_ASSERT(offsetof(struct NdsMailbox, producer_sequence) == 64,
                   "producer publication must have its own cache line");
NDS_MAILBOX_ASSERT(offsetof(struct NdsMailbox, consumer_sequence) == 96,
                   "consumer publication must have its own cache line");
#undef NDS_MAILBOX_ASSERT

/*
 * ARM9 producer protocol:
 *   - verify slot/sequence is free;
 *   - fill command and validate byte_count;
 *   - DC_FlushRange over owned command/publication cache lines;
 *   - publish/notify through the current runtime;
 *   - retain referenced buffers until acknowledgement.
 *
 * ARM9 consumer protocol for ARM7-produced data:
 *   - wait for runtime notification/completion;
 *   - DC_InvalidateRange over a dedicated 32-byte-aligned owned range;
 *   - validate generation, sequence, type, byte_count, and handles;
 *   - consume, then acknowledge.
 */

#endif
