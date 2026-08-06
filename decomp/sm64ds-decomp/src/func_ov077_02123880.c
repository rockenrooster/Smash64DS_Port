extern int _ZN5Actor15FindWithActorIDEjPS_(unsigned int, void*);
int func_ov077_02123880(void) {
  int r5 = 0;
  void* r1 = 0;
  do {
    r1 = (void*)_ZN5Actor15FindWithActorIDEjPS_(0x104, r1);
    if (r1) r5++;
  } while (r1);
  return r5;
}
