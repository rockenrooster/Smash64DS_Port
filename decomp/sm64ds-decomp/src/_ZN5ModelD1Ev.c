/* Model::~Model (D1/complete) at 0x02016d20
 *
 * Model : ModelBase.
 *   [this+0]    = _ZTV5Model           (0x0208e90c)
 *   if (this->unk4C) operator delete(this->unk4C)  (bl 0x0203cbc0 -> _ZdlPv)
 *   bl 0x020170b8 = ModelBase::~ModelBase(this)    (D2/base)
 * returns this. Identical codegen to D2.
 */

struct Model {
    void **vtable;       /* 0x00 */
    char pad[0x4c - 4];
    void *unk4C;         /* 0x4c: freed pointer */
};

extern void *_ZTV5Model[];

extern void _ZdlPv(void *ptr);
extern void *_ZN9ModelBaseD2Ev(struct Model *thiz);

struct Model *_ZN5ModelD1Ev(struct Model *thiz)
{
    thiz->vtable = (void **)_ZTV5Model;
    if (thiz->unk4C)
        _ZdlPv(thiz->unk4C);
    _ZN9ModelBaseD2Ev(thiz);
    return thiz;
}
