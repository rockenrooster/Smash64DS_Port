/* The packaged SDK ds7_maine uses this upper 16 KiB of main RAM. Keep that
 * contract: the generic ds7.ld default starts at 0x02fcc000, whose DS mirror
 * overlaps the existing ARM9 scene arena. The link command fixes .main at
 * 0x02ff0000, while the SDK script still owns crt0, WRAM and load lists. */
ASSERT(__main_start == 0x02ff0000, "ARM7 main RAM overlaps the ARM9 arena")
ASSERT(__main_bss_end <= 0x02ff4000, "ARM7 main RAM exceeds the SDK reservation")
ASSERT(__heap_start <= __heap_end, "ARM7 main heap has negative capacity")
/* The project reserves this upper WRAM block for its sheltered DLDI driver. */
ASSERT(__wram_bss_end <= 0x0380b000, "ARM7 code/data overlaps sheltered DLDI")
