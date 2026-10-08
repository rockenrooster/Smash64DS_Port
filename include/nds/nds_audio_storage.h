#ifndef NDS_AUDIO_STORAGE_H
#define NDS_AUDIO_STORAGE_H

#include <stdint.h>
#include <nds/nds_audio_extent.h>

/* A8 storage owner. One cache-line-owned request and one immediate PXI reply.
 * ARM9 never writes a destination cache line while ARM7 owns the transaction.
 * Byte offsets remain relative to the immutable, currently running ROM. */
#define NDS_AUDIO_STORAGE_ABI 0x41535431u
#define NDS_AUDIO_STORAGE_CHANNEL 23u /* Calico PxiChannel_User0 */
#define NDS_AUDIO_STORAGE_MAX_READ 32768u
#define NDS_AUDIO_STORAGE_MAIN_START 0x02000000u
/* The final 64 KiB belong to ARM7/SDK shared state, never to a read output. */
#define NDS_AUDIO_STORAGE_MAIN_END 0x023f0000u

enum {
    NDS_AUDIO_STORAGE_OPEN_CARD = 1,
    NDS_AUDIO_STORAGE_READ_CARD = 2,
    NDS_AUDIO_STORAGE_CLOSE_CARD = 3,
    NDS_AUDIO_STORAGE_OPEN_MAP = 4,
    /* READ_CARD without a PXI reply: ARM7 writes the reply word, with
     * NDS_AUDIO_STORAGE_ASYNC_DONE set, into reserved[1] of the request line
     * when the read has landed. The ARM9 polls that line (invalidate, read) and
     * never waits; it must not write the line or the destination meanwhile. */
    NDS_AUDIO_STORAGE_READ_CARD_ASYNC = 5
};
#define NDS_AUDIO_STORAGE_ASYNC_DONE 0x80000000u

enum {
    NDS_AUDIO_STORAGE_OK = 0,
    NDS_AUDIO_STORAGE_BAD_REQUEST = 1,
    NDS_AUDIO_STORAGE_NOT_OPEN = 2,
    NDS_AUDIO_STORAGE_IO_ERROR = 3
};

typedef struct NdsAudioStorageRequest {
    uint32_t abi;
    uint32_t operation;
    uint32_t sequence;
    uint32_t offset;
    uint32_t destination;
    uint32_t bytes;
    uint32_t reserved[2];
} __attribute__((aligned(32))) NdsAudioStorageRequest;

typedef struct NdsAudioStorageMap {
    uint32_t abi;
    uint32_t device;
    uint32_t rom_bytes;
    uint32_t extent_count;
    uint32_t extents;
    uint32_t reserved[3];
} __attribute__((aligned(32))) NdsAudioStorageMap;

/* The ARM7 service's health, one ARM9-owned line the ARM7 writes as it
 * works, read by the P3 freeze report: requests taken from the mailbox and
 * answered, the request line being served (address >> 5, 0 = idle), the
 * most mailbox messages seen waiting, and the ARM7 main loop's VBlanks
 * (the lowest-priority ARM7 thread: it stops when anything starves it).
 * Its address rides in the map's reserved[0] (OPEN_MAP); 0 = none. */
typedef struct NdsAudioStorageHealth {
    uint32_t received;
    uint32_t completed;
    uint32_t current;
    uint32_t pending_max;
    uint32_t vblanks;
    uint32_t reserved[3];
} __attribute__((aligned(32))) NdsAudioStorageHealth;

_Static_assert(sizeof(NdsAudioStorageHealth) == 32u,
               "storage health must own exactly one cache line");
_Static_assert(sizeof(NdsAudioStorageRequest) == 32u,
               "storage request must own exactly one cache line");
_Static_assert(sizeof(NdsAudioStorageMap) == 32u,
               "storage map header must own exactly one cache line");

static inline int ndsAudioStorageMainRange(uint32_t address, uint32_t bytes)
{
    return (address >= NDS_AUDIO_STORAGE_MAIN_START) &&
           (address < NDS_AUDIO_STORAGE_MAIN_END) &&
           (bytes <= NDS_AUDIO_STORAGE_MAIN_END - address);
}

/* Used by the real ARM7 receiver before touching the requested destination. */
static inline int ndsAudioStorageValidate(
    const NdsAudioStorageRequest *request, uint32_t request_address,
    uint32_t rom_bytes)
{
    if ((request_address & 31u) ||
        !ndsAudioStorageMainRange(request_address, sizeof(*request)) ||
        (request->abi != NDS_AUDIO_STORAGE_ABI) ||
        (request->sequence == 0u) || (request->sequence > 0xffffu) ||
        request->reserved[0] || request->reserved[1])
        return 0;
    if ((request->operation == NDS_AUDIO_STORAGE_READ_CARD) ||
        (request->operation == NDS_AUDIO_STORAGE_READ_CARD_ASYNC))
    {
        if ((request->bytes == 0u) ||
            (request->bytes > NDS_AUDIO_STORAGE_MAX_READ) ||
            ((request->destination | request->bytes) & 31u) ||
            !ndsAudioStorageMainRange(request->destination, request->bytes) ||
            (request->offset > rom_bytes) ||
            (request->bytes > rom_bytes - request->offset))
            return 0;
        /* An output overlapping the descriptor would change its own command
         * while the receiver is still using it. */
        return (request->destination >= request_address + sizeof(*request)) ||
               (request->destination + request->bytes <= request_address);
    }
    if (request->operation == NDS_AUDIO_STORAGE_OPEN_MAP)
        return request->offset == 0u && request->bytes == sizeof(NdsAudioStorageMap) &&
               !(request->destination & 31u) &&
               ndsAudioStorageMainRange(request->destination, request->bytes) &&
               request->destination != request_address;
    return ((request->operation == NDS_AUDIO_STORAGE_OPEN_CARD) ||
            (request->operation == NDS_AUDIO_STORAGE_CLOSE_CARD)) &&
           (request->offset == 0u) && (request->destination == 0u) &&
           (request->bytes == 0u);
}

static inline uint32_t ndsAudioStorageReply(uint32_t sequence, uint32_t status)
{
    return ((sequence & 0xffffu) << 8) | (status & 0xffu);
}

static inline uint32_t ndsAudioStorageCardCapacity(uint32_t device_capacity)
{
    return device_capacity <= 12u ? 0x20000u << device_capacity : 0u;
}

#ifndef ARM7
int ndsAudioStorageReadAsync(NdsAudioStorageRequest *request,
                             uint32_t rom_offset, void *destination,
                             uint32_t bytes);
int ndsAudioStorageReadAsyncPoll(NdsAudioStorageRequest *request);
#endif

#ifdef ARM7
/* The ARM7 audio worker and ARM9 RPC server share this serialized media owner.
 * Unlike RPC outputs, internal audio destinations may live in ARM7 WRAM. */
int ndsAudioStorageReadRom(uint32_t offset, void *destination, uint32_t bytes);
uint32_t ndsAudioStorageRomSize(void);
#endif

#endif
