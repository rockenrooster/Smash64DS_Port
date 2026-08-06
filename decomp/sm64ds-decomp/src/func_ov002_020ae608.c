extern int _ZNK12WithMeshClsn10IsOnGroundEv(void*);
extern int _ZN5Enemy9SpawnCoinEv(void*);
extern int _ZN5Actor24KillAndTrackInDeathTableEv(void*);
int func_ov002_020ae608(void* c, void* a){
  if(_ZNK12WithMeshClsn10IsOnGroundEv(a)==0) return 0;
  _ZN5Enemy9SpawnCoinEv(c);
  _ZN5Actor24KillAndTrackInDeathTableEv(c);
  *(int*)((char*)c+0x10c)=0;
  return 1;
}
