//cpp
struct Vector3;
extern "C" {
int _ZN5Actor15IsPlayerInRangeEi(void* c, int n);
void* _ZN5Actor13ClosestPlayerEv(void* c);
short Vec3_HorzAngle(const void* a, const void* b);
}
extern "C" void func_ov100_02141800(char* c){
    if(_ZN5Actor15IsPlayerInRangeEi(c, 0x3e8) == 0) return;
    *(int*)(c+0x3e4) = 2;
    void* p = _ZN5Actor13ClosestPlayerEv(c);
    *(short*)(c+0x94) = Vec3_HorzAngle(c+0x5c, (char*)p+0x5c);
}
