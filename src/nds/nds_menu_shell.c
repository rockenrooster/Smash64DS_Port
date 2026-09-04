/* P2-1d -- the VS shell's real screens. Contract and reasoning:
 * include/nds/nds_menu_shell.h. Per-scene VRAM ownership:
 * docs/p2/P2-1c-vram-map.md. */

#include "nds_build_config.h"

#if NDS_P2_MENU_SHELL
#include "nds_menu_shell_core.c"
#include "nds_menu_shell_mode_vs.c"
#include "nds_menu_shell_css.c"
#include "nds_menu_shell_sss.c"
#include "nds_menu_shell_items.c"
#include "nds_menu_shell_router.c"
#endif /* NDS_P2_MENU_SHELL */
