/* P3 ARM7 radio module (include/nds/nds_net_link.h). Linked at the base of
 * the net area with the resident ARM7 image's symbols (--just-symbols), so
 * calls into Calico's threads, IRQs and PXI land on the resident copies; only
 * what the resident image lacks (the Mitsumi driver, netbuf, wlan) is linked
 * in here. objcopy -O binary of this ELF is net/net7.bin in NitroFS. */
OUTPUT_ARCH(arm)
ENTRY(gNdsNet7Header)

MEMORY {
	net7 : ORIGIN = 0x02ff4000, LENGTH = 0x6000
}

SECTIONS {
	.net7 : {
		KEEP( *(.net7.header) )
		*(.text .text.* .stub .gnu.linkonce.t.*)
		*(.glue_7t .glue_7 .vfp11_veneer .v4_bx .iplt)
		*(.rodata .rodata.* .gnu.linkonce.r.*)
		*(.data .data.* .gnu.linkonce.d.*)
		. = ALIGN(4);
		__net7_image_end = .;
	} >net7

	.net7.bss (NOLOAD) : {
		*(.dynbss .bss .bss.* .gnu.linkonce.b.* COMMON)
		. = ALIGN(4);
		__net7_bss_end = .;
	} >net7

	/DISCARD/ : {
		*(.ARM.exidx* .ARM.extab* .gnu.linkonce.armexidx.* .init_array* .fini_array* .ctors* .dtors* .eh_frame* .comment .note* .tbss* .tdata*)
	}

	/* Debug sections stay in the ELF for gdb; objcopy drops them. */
	.debug_info     0 : { *(.debug_info .gnu.linkonce.wi.*) }
	.debug_abbrev   0 : { *(.debug_abbrev) }
	.debug_line     0 : { *(.debug_line .debug_line.* .debug_line_end) }
	.debug_frame    0 : { *(.debug_frame) }
	.debug_str      0 : { *(.debug_str) }
	.debug_loc      0 : { *(.debug_loc) }
	.debug_ranges   0 : { *(.debug_ranges) }
	.debug_aranges  0 : { *(.debug_aranges) }
	.debug_line_str 0 : { *(.debug_line_str) }
	.debug_loclists 0 : { *(.debug_loclists) }
	.debug_rnglists 0 : { *(.debug_rnglists) }
	.ARM.attributes 0 : { KEEP (*(.ARM.attributes)) }
}

ASSERT(__net7_bss_end <= 0x02ffa000, "ARM7 net module overlaps the shared rings")
