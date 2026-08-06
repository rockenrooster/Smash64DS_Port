typedef int s32;

extern void _ZN4cstd10fdiv_asyncE5Fix12IiE5Fix12IiE(s32 numerator, s32 denominator);
extern s32 _ZN4cstd11fdiv_resultEv(void);

s32 _ZN4cstd4fdivEii(s32 numerator, s32 denominator)
{
    _ZN4cstd10fdiv_asyncE5Fix12IiE5Fix12IiE(numerator, denominator);
    return _ZN4cstd11fdiv_resultEv();
}
