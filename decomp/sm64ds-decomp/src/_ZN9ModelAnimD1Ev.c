/* ModelAnim::~ModelAnim (D1/complete) at 0x0201691c
 *
 * Byte-identical codegen to the D2 variant: ModelAnim : Model with an
 * Animation member subobject at +0x50.
 *   [this+0]    = _ZTV9ModelAnim                  (0x0208e980)
 *   [this+0x50] = VTable_Animation_ModelAnimThunk (0x0208e9a4)
 *   bl 0x02015cb4 = Animation::~Animation(this+0x50)
 *   bl 0x02016ca8 = Model::~Model(this)
 * returns this.
 */

struct ModelAnim {
    void **vtable;       /* 0x00 */
    char pad[0x50 - 4];
    void **animVtable;   /* 0x50: Animation subobject vtable */
};

extern void *_ZTV9ModelAnim[];
extern void *VTable_Animation_ModelAnimThunk[];

extern void *_ZN9AnimationD2Ev(void *anim);
extern void *_ZN5ModelD2Ev(struct ModelAnim *thiz);

struct ModelAnim *_ZN9ModelAnimD1Ev(struct ModelAnim *thiz)
{
    thiz->vtable = (void **)_ZTV9ModelAnim;
    thiz->animVtable = (void **)VTable_Animation_ModelAnimThunk;
    _ZN9AnimationD2Ev((char *)thiz + 0x50);
    _ZN5ModelD2Ev(thiz);
    return thiz;
}
