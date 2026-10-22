/* P3 lab: a boot-time radio exchange for the two-instance harness
 * (scratchpad mp/, melonDS-mp with MELONDS_MP_INSTANCES=2). Each console
 * broadcasts one numbered packet a VBlank and counts the peers' packets; gdb
 * reads the counters below. Compiled only with NDS_NET_LAB_PING. */
#include <nds.h>
#include <string.h>

#include <nds/nds_net.h>
#include <nds/nds_net_link.h>

