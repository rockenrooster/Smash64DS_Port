//cpp
struct Actor;
extern "C" {
extern struct Actor* _ZN5Actor15FindWithActorIDEjPS_(unsigned id, struct Actor* prev);
void func_ov002_020ba3fc(char* c) {
    int ok = 1;
    struct Actor* p;
    *(short*)(c+0x338) = 0;
    *(unsigned char*)(c+0x34f) = 5;
    *(unsigned char*)(c+0x34d) = 0;
    p = _ZN5Actor15FindWithActorIDEjPS_(0xb, 0);
    while (p != 0) {
        if (p != (struct Actor*)c) {
            if (*(int*)((char*)p+0x340) != 0) ok = 0;
        }
        p = _ZN5Actor15FindWithActorIDEjPS_(0xb, p);
    }
    if (ok != 0) {
        p = _ZN5Actor15FindWithActorIDEjPS_(0xc, 0);
        while (p != 0) {
            if (p != (struct Actor*)c) {
                if (*(int*)((char*)p+0x340) != 0) ok = 0;
            }
            p = _ZN5Actor15FindWithActorIDEjPS_(0xc, p);
        }
    }
    if (ok != 0) *(int*)(c+0x334) = 0x7f;
}
}
