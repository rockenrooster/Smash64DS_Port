#ifndef NDS_PRACTICES_PXI_DEMO_H
#define NDS_PRACTICES_PXI_DEMO_H
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

/* Components for a matching Calico ARM9/ARM7 pair; not a replacement ARM7 core.
 * User0 must be reserved on both CPUs. See README.md for blocking/lifetime rules.
 */
#ifdef ARM9
void nds_pxi_demo_connect(void);
bool nds_pxi_demo_echo(uint32_t value, uint32_t *reply);
bool nds_pxi_demo_stop(void);
#endif
#ifdef ARM7
bool nds_pxi_demo_server_start(void);
/* Blocks until ARM9 sends STOP. Only call after a successful start. */
int nds_pxi_demo_server_join(void);
#endif

#ifdef __cplusplus
}
#endif
#endif
