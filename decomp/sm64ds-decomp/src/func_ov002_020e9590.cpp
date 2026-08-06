//cpp
struct Actor;
extern "C" Actor* _ZN5Actor10FindWithIDEj(unsigned int id);
extern "C" Actor* _ZN5Actor15FindWithActorIDEjPS_(unsigned int actorID, Actor* prev);
extern "C" void LinkSilverStarAndStarMarker(void* a, void* b);

extern "C" void func_ov002_020e9590(char* self){
    Actor* found;
    if (_ZN5Actor10FindWithIDEj(*(unsigned int*)(self + 0x434)))
        return;
    found = 0;
    for (;;) {
        found = _ZN5Actor15FindWithActorIDEjPS_(0xb4, found);
        if (!found)
            return;
        if (*(unsigned char*)(self + 0x49d) == *(unsigned char*)((char*)found + 0x1d9)) {
            int v444 = *(int*)(self + 0x444);
            if (v444 == 9 && *(unsigned char*)((char*)found + 0x1d8))
                break;
            if (v444 == 9)
                continue;
            if (!*(unsigned char*)((char*)found + 0x1d8))
                break;
        }
    }
    *(int*)(self + 0x434) = *(int*)((char*)found + 4);
    LinkSilverStarAndStarMarker(found, self);
}
