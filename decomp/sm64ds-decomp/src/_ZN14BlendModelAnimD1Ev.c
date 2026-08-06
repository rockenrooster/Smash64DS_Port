/* BlendModelAnim::~BlendModelAnim (D1/complete) at 0x02016690
 *
 * BlendModelAnim : ModelAnim : Model, Animation.
 *   [this+0]    = _ZTV14BlendModelAnim                  (0x0208e94c)
 *   [this+0x50] = VTable_Animation_BlendModelAnimThunk  (0x0208e970)
 *   if (this->unk6C) operator delete(this->unk6C)   (bl 0x0203cbc0 -> _ZdlPv)
 *   bl 0x0201689c = ModelAnim::~ModelAnim(this)     (D2/base)
 * returns this.
 */

struct BlendModelAnim {
    void **vtable;       /* 0x00 */
    char pad[0x50 - 4];
    void **animVtable;   /* 0x50: Animation subobject vtable */
    char pad2[0x6c - 0x50 - 4];
    void *unk6C;         /* 0x6c: freed pointer */
};

extern void *_ZTV14BlendModelAnim[];
extern void *VTable_Animation_BlendModelAnimThunk[];

extern void _ZdlPv(void *ptr);
extern void *_ZN9ModelAnimD2Ev(struct BlendModelAnim *thiz);

struct BlendModelAnim *_ZN14BlendModelAnimD1Ev(struct BlendModelAnim *thiz)
{
    thiz->vtable = (void **)_ZTV14BlendModelAnim;
    thiz->animVtable = (void **)VTable_Animation_BlendModelAnimThunk;
    if (thiz->unk6C)
        _ZdlPv(thiz->unk6C);
    _ZN9ModelAnimD2Ev(thiz);
    return thiz;
}
