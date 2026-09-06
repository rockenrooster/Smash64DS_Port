/* Host call-contract mock ONLY: not a libnds implementation or DMA emulator. */
#ifndef NDS_PRACTICES_HOST_MOCK_NDS_H
#define NDS_PRACTICES_HOST_MOCK_NDS_H
#include <stdbool.h>
#include <stdint.h>
void DC_FlushRange(const void *base, uint32_t bytes);
void DC_InvalidateRange(void *base, uint32_t bytes);
void dmaCopyWordsAsynch(uint8_t channel, const void *src, void *dst, uint32_t bytes);
int dmaBusy(uint8_t channel);
#endif
