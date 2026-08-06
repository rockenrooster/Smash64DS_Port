/* SolidHeap::~SolidHeap (deleting / D0) at 0x0203c970
 *
 * SolidHeap : Heap.
 *   [this+0] = _ZTV9SolidHeap (0x02099d48)
 *   bl 0x0203ca10 = Heap::~Heap(this)  (immediate base dtor)
 *   bl 0x0203cbcc = Memory::operator_delete2(this)
 *   return this;
 */

struct SolidHeap { void **vtable; /* 0x0 */ };

extern void *_ZTV9SolidHeap[];
extern void _ZN4HeapD2Ev(struct SolidHeap *self);     /* 0x0203ca10 */
extern void _ZN6Memory16operator_delete2EPv(void *p); /* 0x0203cbcc */

struct SolidHeap *_ZN9SolidHeapD0Ev(struct SolidHeap *self)
{
    self->vtable = (void **)_ZTV9SolidHeap;
    _ZN4HeapD2Ev(self);
    _ZN6Memory16operator_delete2EPv(self);
    return self;
}
