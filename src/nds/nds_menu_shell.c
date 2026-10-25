/* P2-1d -- the VS shell's real screens. Contract and reasoning:
 * include/nds/nds_menu_shell.h. Per-scene VRAM ownership:
 * docs/p2/P2-1c-vram-map.md. */

#include "nds_build_config.h"
#include <nds/nds_scene_harness.h>
#include <nds/nds_net_session.h>
#include <nds/nds_net_lobby.h>
#include <nds/nds_net_ui.h>
#include <nds/nds_p7.h>

#if NDS_MENU_SHELL
#include "nds_menu_shell_core.c"
#include "nds_menu_shell_onepmode.c"
#include "nds_menu_shell_onep.c"
#include "nds_menu_shell_mode_vs.c"
#include "nds_menu_shell_css.c"
#include "nds_menu_shell_css_panels.c"
#include "nds_menu_shell_css_cursor.c"
#include "nds_menu_shell_css_modes.c"
#include "nds_menu_shell_sss.c"
#include "nds_menu_shell_items.c"
#include "nds_menu_shell_vsoptions.c"
#include "nds_menu_shell_option.c"
#include "nds_menu_shell_backupclear.c"
#include "nds_menu_shell_router.c"
#endif /* NDS_MENU_SHELL */
