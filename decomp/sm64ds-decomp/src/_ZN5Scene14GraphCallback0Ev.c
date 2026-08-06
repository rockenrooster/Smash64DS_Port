/* Scene::GraphCallback0() at 0x02018eb8
 * Static scene-graph traversal callback (one of GraphCallbacks::unk0..unk3).
 * unk0 is called from FUN_02019390 in Game::Loop (loopState = 2).
 * Base implementation does nothing and just returns success (1).
 * Static method -- no `this`.
 */

typedef int s32;

s32 _ZN5Scene14GraphCallback0Ev(void)
{
    return 1;
}
