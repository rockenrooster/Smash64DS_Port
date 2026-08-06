//cpp
struct Actor {
    void UpdatePosWithOnlySpeed(void *);
};

extern "C" void func_ov015_02111fb8(void *self, int idx);

extern "C" void func_ov015_02111d98(char *c) {
    ((Actor *)c)->UpdatePosWithOnlySpeed(0);
    int r2 = *(int *)(c + 0x320);
    int r0 = *(int *)(c + 0x5c);
    if (r0 < r2) return;
    *(int *)(c + 0x5c) = r2;
    func_ov015_02111fb8(c, 5);
}
