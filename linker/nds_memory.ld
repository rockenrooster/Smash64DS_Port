/* Calico DS9 memory map, declared before both resident and overlay selectors. */
MEMORY {
    lma9    : ORIGIN = 0x2004000, LENGTH = 0x27c000
    lma9i   : ORIGIN = 0x2400000, LENGTH = 0x280000
    vectors : ORIGIN = 0x0000000, LENGTH = 0x20
    itcm    : ORIGIN = 0x1ff8020, LENGTH = 0x7fe0
    main    : ORIGIN = 0x2001000, LENGTH = 0xf7f000
    dtcm    : ORIGIN = 0x2ff0000, LENGTH = 0x3e80
    dldi    : ORIGIN = 0x380b000, LENGTH = 0x4000
    stub    : ORIGIN = 0xe000000, LENGTH = 0x4000
    tls     : ORIGIN = 0, LENGTH = 1K
    /* ndstool extracts these overlay LMAs into the ROM's overlay files. */
    ovlmem  : ORIGIN = 0xb0000000, LENGTH = 0x10000000
}
