extern void _ZN9ActorBase18MarkForDestructionEv(void*);
extern void _ZN5Actor24KillAndTrackInDeathTableEv(void*);
void func_ov084_02129498(char* r0) {
  if ((*(unsigned char*)(r0 + 0x113) & 0xf) < 6)
    _ZN9ActorBase18MarkForDestructionEv(r0);
  else
    _ZN5Actor24KillAndTrackInDeathTableEv(r0);
}
