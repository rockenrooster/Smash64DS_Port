extern int _ZN16MeshColliderBase9IsEnabledEv(void *);
extern void _ZN16MeshColliderBase7DisableEv(void *);
extern void _ZN13SharedFilePtr7ReleaseEv(void *);
int func_ov002_020b5be0(char *t)
{
    void **fp;
    if (_ZN16MeshColliderBase9IsEnabledEv(t + 0x124)) {
        _ZN16MeshColliderBase7DisableEv(t + 0x124);
    }
    fp = *(void ***)(t + 0x32c);
    _ZN13SharedFilePtr7ReleaseEv(fp[0]);
    fp = *(void ***)(t + 0x32c);
    _ZN13SharedFilePtr7ReleaseEv(fp[1]);
    return 1;
}
