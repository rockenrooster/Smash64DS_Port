/* Calico/ndstool overlay 0. Keep mutable menu state in the resident image;
 * only code/read-only data are reloaded after a VS match borrows this range.
 * This script is passed BEFORE nds_hot_text.ld, as in Calico's overlay example,
 * so these input sections are claimed before the resident .main wildcard. */
PHDRS {
    ovltable PT_LOAD FLAGS(0x200007);
    ovl_frontend PT_LOAD FLAGS(0x200007);
}

/* These guards are called from shared frame/input/sprite code even in battle.
 * Their mutable state already stays resident. Keep the complete UI commit
 * dependency set here so that no unloaded-code call is hidden behind a branch. */
SECTIONS {
    .text.frontend_resident : {
        :nds_menu_shell.o (.text.ndsMenuShellOnePlayerCssOwnsSource2D
            .text.ndsMenuShellWalkWantsResultsStart)
        :nds_ui_kit.o (.text.ndsUiKitCommit .text.ndsUiKitOam
            .text.ndsUiKitObjBase .text.ndsUiKitObjAt .text.ndsUiKitSpriteSize
            .rodata.kNdsUiKitImageMetrics .rodata.ndsUiKitCommit.*)
        . = ALIGN(4);
    } >main AT>lma9 :main = 0xff
}
INSERT BEFORE .main;

SECTIONS {
    .ovl.frontend ALIGN(32) : {
        __nds_frontend_start = .;
        :battleship_mntitlefiles.o (.text .text.* .gnu.linkonce.t.* .glue_7t .glue_7 .vfp11_veneer .v4_bx
            .rodata .rodata1 .rodata.* .gnu.linkonce.r.*)
        :battleship_mntitle.o (.text .text.* .gnu.linkonce.t.* .glue_7t .glue_7 .vfp11_veneer .v4_bx
            .rodata .rodata1 .rodata.* .gnu.linkonce.r.*)
        :battleship_mnvsmode.o (.text .text.* .gnu.linkonce.t.* .glue_7t .glue_7 .vfp11_veneer .v4_bx
            .rodata .rodata1 .rodata.* .gnu.linkonce.r.*)
        :battleship_mnplayersvs.o (.text .text.* .gnu.linkonce.t.* .glue_7t .glue_7 .vfp11_veneer .v4_bx
            .rodata .rodata1 .rodata.* .gnu.linkonce.r.*)
        :battleship_mnmaps.o (.text .text.* .gnu.linkonce.t.* .glue_7t .glue_7 .vfp11_veneer .v4_bx
            .rodata .rodata1 .rodata.* .gnu.linkonce.r.*)
        :battleship_mnoption.o (.text .text.* .gnu.linkonce.t.* .glue_7t .glue_7 .vfp11_veneer .v4_bx
            .rodata .rodata1 .rodata.* .gnu.linkonce.r.*)
        :battleship_mnbackupclear.o (.text .text.* .gnu.linkonce.t.* .glue_7t .glue_7 .vfp11_veneer .v4_bx
            .rodata .rodata1 .rodata.* .gnu.linkonce.r.*)
        :battleship_mnsoundtest.o (.text .text.* .gnu.linkonce.t.* .glue_7t .glue_7 .vfp11_veneer .v4_bx
            .rodata .rodata1 .rodata.* .gnu.linkonce.r.*)
        :battleship_mnscreenadjust.o (.text .text.* .gnu.linkonce.t.* .glue_7t .glue_7 .vfp11_veneer .v4_bx
            .rodata .rodata1 .rodata.* .gnu.linkonce.r.*)
        :battleship_mnmessage.o (.text .text.* .gnu.linkonce.t.* .glue_7t .glue_7 .vfp11_veneer .v4_bx
            .rodata .rodata1 .rodata.* .gnu.linkonce.r.*)
        :battleship_mndata.o (.text .text.* .gnu.linkonce.t.* .glue_7t .glue_7 .vfp11_veneer .v4_bx
            .rodata .rodata1 .rodata.* .gnu.linkonce.r.*)
        :battleship_mnvsrecord.o (.text .text.* .gnu.linkonce.t.* .glue_7t .glue_7 .vfp11_veneer .v4_bx
            .rodata .rodata1 .rodata.* .gnu.linkonce.r.*)
        :battleship_mncharacters.o (.text .text.* .gnu.linkonce.t.* .glue_7t .glue_7 .vfp11_veneer .v4_bx
            .rodata .rodata1 .rodata.* .gnu.linkonce.r.*)
        :battleship_mn1pmode.o (.text .text.* .gnu.linkonce.t.* .glue_7t .glue_7 .vfp11_veneer .v4_bx
            .rodata .rodata1 .rodata.* .gnu.linkonce.r.*)
        :battleship_mn1pcontinue.o (.text .text.* .gnu.linkonce.t.* .glue_7t .glue_7 .vfp11_veneer .v4_bx
            .rodata .rodata1 .rodata.* .gnu.linkonce.r.*)
        :battleship_mnplayers1pgame.o (.text .text.* .gnu.linkonce.t.* .glue_7t .glue_7 .vfp11_veneer .v4_bx
            .rodata .rodata1 .rodata.* .gnu.linkonce.r.*)
        :battleship_mnplayers1pbonus.o (.text .text.* .gnu.linkonce.t.* .glue_7t .glue_7 .vfp11_veneer .v4_bx
            .rodata .rodata1 .rodata.* .gnu.linkonce.r.*)
        :battleship_mncongra.o (.text .text.* .gnu.linkonce.t.* .glue_7t .glue_7 .vfp11_veneer .v4_bx
            .rodata .rodata1 .rodata.* .gnu.linkonce.r.*)
        :battleship_mnvsresults.o (.text .text.* .gnu.linkonce.t.* .glue_7t .glue_7 .vfp11_veneer .v4_bx
            .rodata .rodata1 .rodata.* .gnu.linkonce.r.*)
        :battleship_mvopeningroom.o (.text .text.* .gnu.linkonce.t.* .glue_7t .glue_7 .vfp11_veneer .v4_bx
            .rodata .rodata1 .rodata.* .gnu.linkonce.r.*)
        :battleship_mvopeningportraits.o (.text .text.* .gnu.linkonce.t.* .glue_7t .glue_7 .vfp11_veneer .v4_bx
            .rodata .rodata1 .rodata.* .gnu.linkonce.r.*)
        :battleship_mvopeningmario.o (.text .text.* .gnu.linkonce.t.* .glue_7t .glue_7 .vfp11_veneer .v4_bx
            .rodata .rodata1 .rodata.* .gnu.linkonce.r.*)
        :battleship_mvopeningdonkey.o (.text .text.* .gnu.linkonce.t.* .glue_7t .glue_7 .vfp11_veneer .v4_bx
            .rodata .rodata1 .rodata.* .gnu.linkonce.r.*)
        :battleship_mvopeninglink.o (.text .text.* .gnu.linkonce.t.* .glue_7t .glue_7 .vfp11_veneer .v4_bx
            .rodata .rodata1 .rodata.* .gnu.linkonce.r.*)
        :battleship_mvopeningsamus.o (.text .text.* .gnu.linkonce.t.* .glue_7t .glue_7 .vfp11_veneer .v4_bx
            .rodata .rodata1 .rodata.* .gnu.linkonce.r.*)
        :battleship_mvopeningyoshi.o (.text .text.* .gnu.linkonce.t.* .glue_7t .glue_7 .vfp11_veneer .v4_bx
            .rodata .rodata1 .rodata.* .gnu.linkonce.r.*)
        :battleship_mvopeningkirby.o (.text .text.* .gnu.linkonce.t.* .glue_7t .glue_7 .vfp11_veneer .v4_bx
            .rodata .rodata1 .rodata.* .gnu.linkonce.r.*)
        :battleship_mvopeningfox.o (.text .text.* .gnu.linkonce.t.* .glue_7t .glue_7 .vfp11_veneer .v4_bx
            .rodata .rodata1 .rodata.* .gnu.linkonce.r.*)
        :battleship_mvopeningpikachu.o (.text .text.* .gnu.linkonce.t.* .glue_7t .glue_7 .vfp11_veneer .v4_bx
            .rodata .rodata1 .rodata.* .gnu.linkonce.r.*)
        :battleship_mvending.o (.text .text.* .gnu.linkonce.t.* .glue_7t .glue_7 .vfp11_veneer .v4_bx
            .rodata .rodata1 .rodata.* .gnu.linkonce.r.*)
        :nds_ui_kit.o (.text .text.* .gnu.linkonce.t.* .glue_7t .glue_7 .vfp11_veneer .v4_bx
            .rodata .rodata1 .rodata.* .gnu.linkonce.r.*)
        :nds_menu_shell.o (.text .text.* .gnu.linkonce.t.* .glue_7t .glue_7 .vfp11_veneer .v4_bx
            .rodata .rodata1 .rodata.* .gnu.linkonce.r.*)
        . = ALIGN(32);
        __nds_frontend_end = .;
    } AT>ovlmem :ovl_frontend = 0xff
}
INSERT AFTER .main.bss;

SECTIONS {
    .ovltable 0 : {
        /* No constructors or destructors: mutable .data/.bss stay resident. */
        LONG(0)
        LONG(0)
        LONG(0)
    } AT>ovlmem :ovltable
}
