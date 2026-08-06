/* ExpandingHeap::~ExpandingHeap (deleting / D0) at 0x0203c9c0
 *
 * ExpandingHeap : Heap.
 *   [this+0] = _ZTV13ExpandingHeap (0x02099dd8)
 *   bl 0x0203ca10 = Heap::~Heap(this)  (immediate base dtor)
 *   bl 0x0203cbcc = Memory::operator_delete2(this)
 *   return this;
 */

struct ExpandingHeap { void **vtable; /* 0x0 */ };

extern void *_ZTV13ExpandingHeap[];
extern void _ZN4HeapD2Ev(struct ExpandingHeap *self);  /* 0x0203ca10 */
extern void _ZN6Memory16operator_delete2EPv(void *p);  /* 0x0203cbcc */

struct ExpandingHeap *_ZN13ExpandingHeapD0Ev(struct ExpandingHeap *self)
{
    self->vtable = (void **)_ZTV13ExpandingHeap;
    _ZN4HeapD2Ev(self);
    _ZN6Memory16operator_delete2EPv(self);
    return self;
}
