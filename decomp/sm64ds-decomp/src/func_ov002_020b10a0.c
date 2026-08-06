extern int _ZN5Actor18GetBitInDeathTableEv(void*);
extern int func_ov002_020b1328(void*);
extern int _ZN10StarMarker27SpawnRedCoinStarIfNecessaryEv(void);
extern int _ZN5Actor24KillAndTrackInDeathTableEv(void*);
int func_ov002_020b10a0(char* c){
  if(_ZN5Actor18GetBitInDeathTableEv(c)==0) return 0;
  if(func_ov002_020b1328(c)) _ZN10StarMarker27SpawnRedCoinStarIfNecessaryEv();
  _ZN5Actor24KillAndTrackInDeathTableEv(c);
  return 1;
}
