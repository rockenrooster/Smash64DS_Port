extern void func_020731dc(void*, void*, void*);
extern int data_ov071_021230b8[];
extern int func_020072c0[];
extern int data_ov071_021230c4[];

void __sinit_ov071_02122a1c(void) {
    *(unsigned int*)data_ov071_021230b8 = 0;
    *(unsigned int*)((char*)data_ov071_021230b8 + 4) = (unsigned int)-0x19000;
    *(unsigned int*)((char*)data_ov071_021230b8 + 8) = 0;
    func_020731dc(data_ov071_021230b8, func_020072c0, data_ov071_021230c4);
}
