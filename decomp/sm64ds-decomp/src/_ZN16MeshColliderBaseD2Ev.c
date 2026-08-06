/* MeshColliderBase::~MeshColliderBase() at 0x0203968c
 * Base-object destructor. As the root of the hierarchy it only resets the vptr
 * to the MeshColliderBase vtable; there is no base subobject to destruct.
 */

extern int _ZTV16MeshColliderBase[]; // vtable

void _ZN16MeshColliderBaseD2Ev(void* self)
{
    *(int*)self = (int)_ZTV16MeshColliderBase; // set vptr
}
