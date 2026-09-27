/* Copied from $(CALICO)/lib/ds7.ld for NDS_P2_ARM9_WRAM; only the wram region differs. */
/*--------------------------------------------------------------------------------
	** Linker script for DS/DSi homebrew, ARM7 side **
--------------------------------------------------------------------------------*/
OUTPUT_ARCH(arm)
ENTRY(__ds7_bootstub)

MEMORY {
	/* Physical binaries loaded from ROM (LMAs) */
	lma7    : ORIGIN = 0x2380000, LENGTH = 0x40000
	lma7i   : ORIGIN = 0x2e80000, LENGTH = 0x108000

	/* Runtime memory layout for code/data areas (VMAs) */
	main    : ORIGIN = 0x2fcc000, LENGTH = 0x28000 /* Main RAM (usable area for ARM7) */
	twlwram : ORIGIN = 0x37c0000, LENGTH = 0x38000 /* DSi exclusive ARM7 WRAM (minus 32KB used as part of below) */
	/* P2-2p8 (NDS_P2_ARM9_WRAM): the SDK script with the ARM7 image moved up
	 * 16 KB, into the second shared-WRAM block and private WRAM, so ARM9 can
	 * take the first block (WRAMCNT = 2). __sys_start is unchanged. */
	wram    : ORIGIN = 0x37fc000, LENGTH = 0x13000 /* ARM7 WRAM (minus stacks/reserved BIOS memory) */
}

PHDRS {
	/* ELF program headers for the ARM7 binary */
	crt0    PT_LOAD FLAGS(7);
	main    PT_LOAD FLAGS(7);
	wram    PT_LOAD FLAGS(7);
	ldlist  PT_LOAD FLAGS(7);

	/* ELF program headers for the ARM7i binary */
	twlwram PT_LOAD FLAGS(0x100007);
	twlld   PT_LOAD FLAGS(0x100007);
}

/* Default initial stack locations and BIOS variables */
PROVIDE_HIDDEN( __sys_start  = ORIGIN(wram) + LENGTH(wram) );
PROVIDE_HIDDEN( __sp_usr     = __sys_start + 0xe80         );
PROVIDE_HIDDEN( __sp_irq     = __sp_usr + 0x100            );
PROVIDE_HIDDEN( __sp_svc     = __sp_irq + 0x40             );
PROVIDE_HIDDEN( __irq_flags2 = __sp_svc + 0x00             );
PROVIDE_HIDDEN( __scfg_buf   = __sp_svc + 0x04             );
PROVIDE_HIDDEN( __dma_fill   = __sp_svc + 0x20             );
PROVIDE_HIDDEN( __irq_flags  = __sp_svc + 0x38             );
PROVIDE_HIDDEN( __irq_vector = __sp_svc + 0x3c             );

/* Not used */
PROVIDE_HIDDEN( __end__ = 0 );

SECTIONS {
	/* Initialization code section (crt0) */
	.crt0 : {
		KEEP ( *(.prepad) )
		. = ALIGN(4);

		KEEP( *(.bootstub .bootstub.*) )
		*(.crt0 .crt0.*
			/* libc functions that are only used during crt0 */
			.text.build_argv
			.text.__libc_init_array
			/* Other functions */
			.text._blkShelterDldi
		)
		*.crt0.* (.text .stub .text.* .gnu.linkonce.t.*)
		*.crt0.* (.glue_7t .glue_7 .vfp11_veneer .v4_bx)
		*.crt0.* (.rodata .rodata.* .gnu.linkonce.r.*)
		*.crt0.* (.rodata1)
		. = ALIGN(4);
	} >lma7 :crt0 = 0xff

	/* DSi exclusive code/read-only data section */
	.twl : {
		*(.twl .twl.*
			/* Miscellaneous functions that are intended to be DSi exclusive */
			.text.irqSet2
			.text.threadIrqWait2
		)
		*.twl.* (.text .stub .text.* .gnu.linkonce.t.*)
		*.twl.* (.glue_7t .glue_7 .vfp11_veneer .v4_bx)
		*.twl.* (.rodata .rodata.* .gnu.linkonce.r.*)
		*.twl.* (.rodata1)
		. = ALIGN(4);
	} >twlwram AT>lma7i :twlwram = 0xff

	/* DSi exclusive data section */
	.twl.rw : ALIGN_WITH_INPUT {
		*.twl.* (.data .data.* .gnu.linkonce.d.*)
		*.twl.* (.data1)
		. = ALIGN(4);
	} >twlwram AT>lma7i :twlwram = 0xff

	/* Load information for the DSi exclusive section */
	PROVIDE_HIDDEN( __twl_lma   = LOADADDR(.twl)            );
	PROVIDE_HIDDEN( __twl_start = ADDR(.twl)                );
	PROVIDE_HIDDEN( __twl_end   = ADDR(.twl) + SIZEOF(.twl) );

	/* DSi exclusive BSS area */
	.twl.bss . (NOLOAD) : {
		*(.twl_bss .twl_bss.*)
		*.twl.* (.dynbss)
		*.twl.* (.bss .bss.* .gnu.linkonce.b.*)
		*.twl.* (COMMON)
		. = ALIGN(4);
	} >twlwram AT>lma7i :twlwram

	/* Information for DSi exclusive BSS area */
	PROVIDE_HIDDEN( __twl_bss_start = ADDR(.twl.bss)                    );
	PROVIDE_HIDDEN( __twl_bss_end   = ADDR(.twl.bss) + SIZEOF(.twl.bss) );

	/* Main RAM code/read-only data section */
	.main : {
		KEEP( *(SORT_NONE(.init)) )
		*(.main .main.*
			/* libc functions that do not really need to be in fast ARM7 WRAM */
			.text.frame_dummy
			.text.exit
			.text.__call_exitprocs
			.text.__do_global_dtors_aux
			.text.__libc_fini_array
			.text._exit
			.text._pmJumpToBootstub /* must be outside ARM7 WRAM */
			.text._pmJumpToNextApp  /* as above */
			.text._*_r
		)
		*.16.o (.text .stub .text.* .gnu.linkonce.t.*)
		*.16.o (.glue_7t .glue_7 .vfp11_veneer .v4_bx)
		KEEP( *(SORT_NONE(.fini)) )

		*.16.o (.rodata .rodata.* .gnu.linkonce.r.*)
		*.16.o (.rodata1)
		. = ALIGN(4);
	} >main AT>lma7 :main = 0xff

	/* Main RAM data section */
	.main.rw : ALIGN_WITH_INPUT {
		*(.data._impure_data) /* Likewise for this */
		*.16.o (.data .data.* .gnu.linkonce.d.*)
		*.16.o (.data1)
		*(.igot.plt)
		. = ALIGN(4);
	} >main AT>lma7 :main = 0xff

	/* ARM exception table */
	.ARM.extab : ALIGN_WITH_INPUT {
		*(.ARM.extab* .gnu.linkonce.armextab.*)
	} >main AT>lma7 :main = 0xff

	/* ARM exception index table (replaces .eh_frame on AArch32) */
	.ARM.exidx : ALIGN_WITH_INPUT {
		*(.ARM.exidx* .gnu.linkonce.armexidx.*)
	} >main AT>lma7 :main = 0xff

	/* Symbols needed by exception unwind code */
	PROVIDE_HIDDEN( __exidx_start = ADDR(.ARM.exidx)                      );
	PROVIDE_HIDDEN( __exidx_end   = ADDR(.ARM.exidx) + SIZEOF(.ARM.exidx) );

	/* Exception frame header (not used on AArch32) */
	.eh_frame_hdr : ALIGN_WITH_INPUT {
		*(.eh_frame_hdr)
		*(.eh_frame_entry .eh_frame_entry.*)
	} >main AT>lma7 :main = 0xff

	/* Exception frame information (not used on AArch32) */
	.eh_frame : ALIGN_WITH_INPUT {
		KEEP( *(.eh_frame) )
		*(.eh_frame.*)
	} >main AT>lma7 :main = 0xff

	/* GCC language agnostic exception table (not used on AArch32?) */
	.gcc_except_table : ALIGN_WITH_INPUT {
		*(.gcc_except_table .gcc_except_table.*)
	} >main AT>lma7 :main = 0xff

	/* GNU exception table (not used on AArch32?) */
	.gnu_extab : ALIGN_WITH_INPUT {
		*(.gnu_extab*)
	} >main AT>lma7 :main = 0xff

	/* Table of special constructors that run before _init */
	.preinit_array : ALIGN_WITH_INPUT {
		PROVIDE_HIDDEN( __preinit_array_start = . );
		KEEP( *(.preinit_array) )
		PROVIDE_HIDDEN( __preinit_array_end = . );
	} >main AT>lma7 :main = 0xff

	/* Table of global constructors (runs after _init) */
	.init_array : ALIGN_WITH_INPUT {
		PROVIDE_HIDDEN( __init_array_start = . );
		KEEP( *(SORT_BY_INIT_PRIORITY(.init_array.*) SORT_BY_INIT_PRIORITY(.ctors.*)) )
		KEEP( *(.init_array .ctors) )
		PROVIDE_HIDDEN( __init_array_end = . );
	} >main AT>lma7 :main = 0xff

	/* Table of global destructors (runs before _fini) */
	.fini_array : ALIGN_WITH_INPUT {
		PROVIDE_HIDDEN( __fini_array_start = . );
		KEEP( *(SORT_BY_INIT_PRIORITY(.fini_array.*) SORT_BY_INIT_PRIORITY(.dtors.*)) )
		KEEP( *(.fini_array .dtors) )
		PROVIDE_HIDDEN( __fini_array_end = . );
	} >main AT>lma7 :main = 0xff

	/* Load information for main RAM */
	PROVIDE_HIDDEN( __main_lma   = LOADADDR(.main)                         );
	PROVIDE_HIDDEN( __main_start = ADDR(.main)                             );
	PROVIDE_HIDDEN( __main_end   = ADDR(.fini_array) + SIZEOF(.fini_array) );

	/* Main RAM BSS area */
	.main.bss . (NOLOAD) : {
		*(.sbss .sbss.*)
		*.16.o (.dynbss)
		*.16.o (.bss .bss.* .gnu.linkonce.b.*)
		*.16.o (COMMON)
		. = ALIGN(4);
	} >main AT>lma7 :main

	/* Information for main RAM BSS */
	PROVIDE_HIDDEN( __main_bss_start = ADDR(.main.bss)                     );
	PROVIDE_HIDDEN( __main_bss_end   = ADDR(.main.bss) + SIZEOF(.main.bss) );

	/* Define start/end of main heap */
	. = .;
	PROVIDE_HIDDEN( __heap_start = .                           );
	PROVIDE_HIDDEN( __heap_end   = ORIGIN(main) + LENGTH(main) );

	/* WRAM code/read-only data section */
	.wram : {
		*(.text.unlikely .text.*_unlikely .text.unlikely.*)
		*(.text.exit .text.exit.*)
		*(.text.startup .text.startup.*)
		*(.text.hot .text.hot.*)
		*(.text .iplt .stub .text.* .gnu.linkonce.t.*)
		*(.glue_7t .glue_7 .vfp11_veneer .v4_bx)

		*(.rodata .rodata.* .gnu.linkonce.r.*)
		*(.rodata1)
		. = ALIGN(4);
	} >wram AT>lma7 :wram = 0xff

	/* WRAM data section */
	.wram.rw : ALIGN_WITH_INPUT {
		*(.data .data.* .gnu.linkonce.d.*)
		*(.data1)
		SORT(CONSTRUCTORS)
		. = ALIGN(4);
	} >wram AT>lma7 :wram = 0xff

	/* Load information for code/data on ARM7 RAM */
	PROVIDE_HIDDEN( __wram_lma   = LOADADDR(.wram)                   );
	PROVIDE_HIDDEN( __wram_start = ADDR(.wram)                       );
	PROVIDE_HIDDEN( __wram_end   = ADDR(.wram.rw) + SIZEOF(.wram.rw) );

	/* WRAM BSS area */
	.wram.bss . (NOLOAD) : {
		*(.dynbss)
		*(.bss .bss.* .gnu.linkonce.b.*)
		*(COMMON)
		. = ALIGN(4);
	} >wram AT>lma7 :wram

	/* Information for BSS */
	PROVIDE_HIDDEN( __wram_bss_start = ADDR(.wram.bss)                     );
	PROVIDE_HIDDEN( __wram_bss_end   = ADDR(.wram.bss) + SIZEOF(.wram.bss) );

	/* Craft a loadlist */
	.loadlist : {
		LONG(__main_start)
		LONG(__main_end)
		LONG(__main_bss_end)

		LONG(__wram_start)
		LONG(__wram_end)
		LONG(__wram_bss_end)
	} >lma7 :ldlist = 0xff

	PROVIDE_HIDDEN( __loadlist_lma   = __main_lma                              );
	PROVIDE_HIDDEN( __loadlist_start = LOADADDR(.loadlist)                     );
	PROVIDE_HIDDEN( __loadlist_end   = LOADADDR(.loadlist) + SIZEOF(.loadlist) );

	/* Craft a loadlist for DSi exclusive sections */
	.twl.loadlist : {
		LONG(__twl_start)
		LONG(__twl_end)
		LONG(__twl_bss_end)
	} >lma7i :twlld = 0xff

	PROVIDE_HIDDEN( __twl_loadlist_lma   = __twl_lma                                       );
	PROVIDE_HIDDEN( __twl_loadlist_start = LOADADDR(.twl.loadlist)                         );
	PROVIDE_HIDDEN( __twl_loadlist_end   = LOADADDR(.twl.loadlist) + SIZEOF(.twl.loadlist) );

	/* ====================== */
	/*   Debugging metadata   */
	/* ====================== */

	/* Stabs debugging sections.  */
	.stab          0 : { *(.stab) } :NONE
	.stabstr       0 : { *(.stabstr) } :NONE
	.stab.excl     0 : { *(.stab.excl) } :NONE
	.stab.exclstr  0 : { *(.stab.exclstr) } :NONE
	.stab.index    0 : { *(.stab.index) } :NONE
	.stab.indexstr 0 : { *(.stab.indexstr) } :NONE
	.comment       0 : { *(.comment) } :NONE
	.gnu.build.attributes : { *(.gnu.build.attributes .gnu.build.attributes.*) } :NONE
	/* DWARF debug sections.
	   Symbols in the DWARF debugging sections are relative to the beginning
	   of the section so we begin them at 0. */
	/* DWARF 1 */
	.debug          0 : { *(.debug) } :NONE
	.line           0 : { *(.line) } :NONE
	/* GNU DWARF 1 extensions */
	.debug_srcinfo  0 : { *(.debug_srcinfo) } :NONE
	.debug_sfnames  0 : { *(.debug_sfnames) } :NONE
	/* DWARF 1.1 and DWARF 2 */
	.debug_aranges  0 : { *(.debug_aranges) } :NONE
	.debug_pubnames 0 : { *(.debug_pubnames) } :NONE
	/* DWARF 2 */
	.debug_info     0 : { *(.debug_info .gnu.linkonce.wi.*) } :NONE
	.debug_abbrev   0 : { *(.debug_abbrev) } :NONE
	.debug_line     0 : { *(.debug_line .debug_line.* .debug_line_end) } :NONE
	.debug_frame    0 : { *(.debug_frame) } :NONE
	.debug_str      0 : { *(.debug_str) } :NONE
	.debug_loc      0 : { *(.debug_loc) } :NONE
	.debug_macinfo  0 : { *(.debug_macinfo) } :NONE
	/* SGI/MIPS DWARF 2 extensions */
	.debug_weaknames 0 : { *(.debug_weaknames) } :NONE
	.debug_funcnames 0 : { *(.debug_funcnames) } :NONE
	.debug_typenames 0 : { *(.debug_typenames) } :NONE
	.debug_varnames  0 : { *(.debug_varnames) } :NONE
	/* DWARF 3 */
	.debug_pubtypes 0 : { *(.debug_pubtypes) } :NONE
	.debug_ranges   0 : { *(.debug_ranges) } :NONE
	/* DWARF Extension.  */
	.debug_macro    0 : { *(.debug_macro) } :NONE
	.debug_addr     0 : { *(.debug_addr) } :NONE
	.ARM.attributes 0 : { KEEP (*(.ARM.attributes)) KEEP (*(.gnu.attributes)) } :NONE
	.note.gnu.arm.ident 0 : { KEEP (*(.note.gnu.arm.ident)) } :NONE
	/DISCARD/ : { *(.note.GNU-stack) *(.gnu_debuglink) *(.gnu.lto_*) *(.stack) }
}
