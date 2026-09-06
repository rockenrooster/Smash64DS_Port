#ifndef NDS_PRACTICES_PXI_PROTOCOL_H
#define NDS_PRACTICES_PXI_PROTOCOL_H
#include <stdint.h>

/* Simple PXI immediates are 26-bit. This deliberately tiny protocol reserves
 * the top two bits for commands/status and echoes a 24-bit value. No pointer,
 * extended message, credit system, or general shared-memory framework.
 */
#define NDS_PXI_DEMO_VALUE_MAX UINT32_C(0x00ffffff)
#define NDS_PXI_DEMO_STOP      UINT32_C(0x01000000)
#define NDS_PXI_DEMO_STOPPED   UINT32_C(0x01000000)
#define NDS_PXI_DEMO_ERROR     UINT32_C(0x03ffffff)

static inline uint32_t nds_pxi_demo_answer(uint32_t request)
{
    if (request <= NDS_PXI_DEMO_VALUE_MAX) return request;
    if (request == NDS_PXI_DEMO_STOP) return NDS_PXI_DEMO_STOPPED;
    return NDS_PXI_DEMO_ERROR;
}
#endif
