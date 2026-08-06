extern int _ZN5Enemy9SpawnCoinEv(void*);
extern int _ZN5Actor24KillAndTrackInDeathTableEv(void*);
int func_ov002_020ae5c8(void* c){
  unsigned short* p=(unsigned short*)((char*)c+0x100);
  if(p[1]!=0) return 0;
  _ZN5Enemy9SpawnCoinEv(c);
  _ZN5Actor24KillAndTrackInDeathTableEv(c);
  *(int*)((char*)c+0x10c)=0;
  return 1;
}
