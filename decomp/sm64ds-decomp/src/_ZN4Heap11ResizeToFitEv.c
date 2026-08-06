extern void Crash(void);

struct Heap {
    void* heapStart;
    unsigned int heapSize;
    Heap* parentHeap;
    unsigned int flags;

    void Rescue();
    unsigned int ResizeToFit();
    virtual ~Heap();
    virtual void VDestroy() = 0;
    virtual void* VAllocate(unsigned int size, int align) = 0;
    virtual bool VDeallocate(void* ptr) = 0;
    virtual void VDeallocateAll() = 0;
    virtual bool VIntact() = 0;
    virtual void VRescue() = 0;
    virtual unsigned int VReallocate(void* ptr, unsigned int newSize) = 0;
    virtual unsigned int VSizeof(void* ptr) = 0;
    virtual unsigned int VMaxAllocationUnitSize() = 0;
    virtual unsigned int VMaxAllocatableSize() = 0;
    virtual unsigned int VMemoryLeft() = 0;
    virtual unsigned int VSetNodeID(unsigned int id) = 0;
    virtual unsigned int VGetNodeID() = 0;
    virtual unsigned int VResizeToFit() = 0;
};

unsigned int Heap::ResizeToFit()
{
    unsigned int result = VResizeToFit();
    if (!result)
    {
        if (flags & 0x4000)
        {
            Rescue();
            Crash();
        }
    }
    return result;
}
