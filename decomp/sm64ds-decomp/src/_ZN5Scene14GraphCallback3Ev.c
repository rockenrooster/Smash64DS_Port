/* Scene::GraphCallback3() at 0x02018ea0
 * Static scene-graph traversal callback (GraphCallbacks::unk3).
 * unk3 is called from FUN_02019100 in VBlankHandler (on a "lag" frame).
 * Base implementation does nothing and just returns success (1).
 * Static method -- no `this`.
 */

typedef int s32;

s32 _ZN5Scene14GraphCallback3Ev(void)
{
    return 1;
}
