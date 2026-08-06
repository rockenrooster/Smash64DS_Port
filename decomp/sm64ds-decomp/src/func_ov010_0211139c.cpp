//cpp
extern "C" {
int _ZN9ActorBase18MarkForDestructionEv(void*);
void* _ZN5Actor10FindWithIDEj(unsigned int);
int func_ov010_0211139c(char *c){
  unsigned int id=*(unsigned int*)(c+0x3ac);
  if(id==0){ _ZN9ActorBase18MarkForDestructionEv(c); return 0; }
  void *a=_ZN5Actor10FindWithIDEj(id);
  if(a==0){ _ZN9ActorBase18MarkForDestructionEv(c); }
  return (int)a;
}
}
