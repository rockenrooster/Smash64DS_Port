int func_ov072_0211faf0(char *c) {
    c[0x3A2] = 0;
    
    char *sub = c + 0x300;
    *(unsigned short *)(sub + 0xA0) = 0x15;
    
    *(int *)(c + 0x394) = 1;
    return 1;
}
