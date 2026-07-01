/* Import the original BattleShip Wait status helpers.
 *
 * The broad ftmain/status-table implementation is still deferred. The DS port
 * provides a bounded ftMainSetStatus seam that accepts only Wait in the
 * Mario/Fox Wait proof harnesses.
 */
#include <PR/ultratypes.h>
#include <ft/fighter.h>
#include <gr/ground.h>
#include <nds/nds_startup.h>
#include <sys/obj.h>
#include <sys/objhelper.h>
#include <sys/objman.h>

#include "../../decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommonwait.c"
