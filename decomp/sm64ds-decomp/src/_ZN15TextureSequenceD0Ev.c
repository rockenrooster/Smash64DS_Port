struct TextureSequence { void **vtable; };
extern void *_ZTV15TextureSequence[];
extern void _ZN9AnimationD2Ev(struct TextureSequence *thiz);
extern void _ZN6Memory16operator_delete2EPv(void *ptr);

struct TextureSequence *_ZN15TextureSequenceD0Ev(struct TextureSequence *thiz)
{
    thiz->vtable = (void **)_ZTV15TextureSequence;
    _ZN9AnimationD2Ev(thiz);
    _ZN6Memory16operator_delete2EPv(thiz);
    return thiz;
}
