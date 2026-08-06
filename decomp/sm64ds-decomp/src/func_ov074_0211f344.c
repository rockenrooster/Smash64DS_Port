extern int _ZN5Actor15FindWithActorIDEjPS_(unsigned int, void*);
void func_ov074_0211f344(void* r6, unsigned char r5) {
  void* r1 = (void*)_ZN5Actor15FindWithActorIDEjPS_(0xc7, 0);
  while (r1) {
    if (r1 != r6) *(unsigned char*)((char*)r1 + 0x60a) = r5;
    r1 = (void*)_ZN5Actor15FindWithActorIDEjPS_(0xc7, r1);
  }
}
