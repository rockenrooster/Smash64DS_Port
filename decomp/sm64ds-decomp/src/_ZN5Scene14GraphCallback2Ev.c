/* Scene::GraphCallback2() at 0x02018ea8
 * Static scene-graph traversal callback (GraphCallbacks::unk2).
 * unk2 is called from FUN_02019144 in VBlankHandler (on a "render" frame).
 * Base implementation does nothing and just returns success (1).
 * Static method -- no `this`.
 */

typedef int s32;

s32 _ZN5Scene14GraphCallback2Ev(void)
{
    return 1;
}
