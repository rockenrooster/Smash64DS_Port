struct TextureTransformer { void **vtable; };
extern void *_ZTV18TextureTransformer[];
extern void _ZN9AnimationD2Ev(struct TextureTransformer *thiz);
extern void _ZN6Memory16operator_delete2EPv(void *ptr);

struct TextureTransformer *_ZN18TextureTransformerD0Ev(struct TextureTransformer *thiz)
{
    thiz->vtable = (void **)_ZTV18TextureTransformer;
    _ZN9AnimationD2Ev(thiz);
    _ZN6Memory16operator_delete2EPv(thiz);
    return thiz;
}
