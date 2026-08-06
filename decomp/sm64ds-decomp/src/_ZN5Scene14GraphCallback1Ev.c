/* Scene::GraphCallback1() at 0x02018eb0
 * Static scene-graph traversal callback (GraphCallbacks::unk1).
 * unk1 is called from FUN_02019404 in Game::Loop (loopState = 2).
 * Base implementation does nothing and just returns success (1).
 * Static method -- no `this`.
 */

typedef int s32;

s32 _ZN5Scene14GraphCallback1Ev(void)
{
    return 1;
}
