/* Camera::CleanupResources at 0x0200d9d0
 *
 * Frees the object pointed to by this->unk148 (delete-expression: the
 * outer guard plus the inlined deleting-destructor's own null check
 * produce two consecutive `cmp r0,#0; beq`), then returns 1 (VS_SUCCESS).
 *   r0 = this->unk148
 *   if (r0) { if (r0) Memory::operator_delete2(r0); }
 *   return 1;
 */

struct Camera {
    char pad[0x148];
    void *unk148;   /* 0x148: owned object pointer */
};

extern void _ZN6Memory16operator_delete2EPv(void *p); /* 0x0203cbcc */

int _ZN6Camera16CleanupResourcesEv(struct Camera *thiz)
{
    void *p = thiz->unk148;
    if (p) {
        if (p)
            _ZN6Memory16operator_delete2EPv(p);
    }
    return 1;
}
