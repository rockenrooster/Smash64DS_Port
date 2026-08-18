#ifndef NDS_SCENE_MANAGER_H
#define NDS_SCENE_MANAGER_H

#include <PR/ultratypes.h>
#include <sc/scene.h>

/* P2-1b. The port-owned scene seam.
 *
 * WHAT THIS IS NOT. It is not a second scene table beside BattleShip's.
 * `scManagerRunLoop` (src/import/battleship_scmanager.c -> decomp
 * sc/scmanager.c:816) already is the game's scene dispatcher: a `while (TRUE)`
 * over `gSCManagerSceneData.scene_curr` calling `<scene>StartScene()`, which
 * calls `syTaskmanStartTask`, which calls `syTaskmanInitGeneralHeap` on the
 * scene's declared arena -- so the per-scene arena rewind the P2-1 plan asks
 * for is already the source's own contract. Re-implementing that switch here
 * would duplicate the dispatcher and buy nothing.
 *
 * WHAT THIS IS. The two things the source loop does NOT own on this target:
 *
 *   1. A REGISTRY of the scenes this build actually has, with each scene's
 *      arena policy and entry transition. A transition request to a kind that
 *      is not in the table is refused and counted -- the menus P2-1d/e/f add
 *      will name their destination through here, and a typo cannot silently
 *      become "park forever" the way a raw `scene_curr = <kind>` can.
 *   2. PER-SCENE ARENA ACCOUNTING. `syTaskmanStartTask` is wrapped so every
 *      scene entry and exit is observed. Each entry records the arena base and
 *      size the scene declared; each exit records the arena high-water that
 *      entry reached. Same scene, N entries, identical high-water == the reset
 *      discipline holds and the loop leaks nothing. A rising high-water is the
 *      leak, named to the scene that grew.
 *
 * Every port-owned scene transition goes through `ndsSceneManagerRequest`.
 * That is deliberately the ONLY place outside the imported source that writes
 * `gSCManagerSceneData.scene_curr`/`scene_prev` and calls
 * `syTaskmanSetLoadScene`, so "where can this build go next" is one table and
 * one function rather than a grep. */

/* Scene descriptor flags. */
#define NDS_SCENE_FLAG_NONE 0u
/* Entry rewinds the taskman general arena (i.e. the scene reaches
 * syTaskmanStartTask, which calls syTaskmanInitGeneralHeap). Every scene in
 * the table has this today; the flag exists so a future scene that does NOT
 * -- a pause overlay pushed over a live battle, say -- is expressible and is
 * excluded from the flat-high-water assertion instead of silently failing it. */
#define NDS_SCENE_FLAG_ARENA_RESET (1u << 0)
#define NDS_SCENE_FLAG_MENU (1u << 1)
#define NDS_SCENE_FLAG_BATTLE (1u << 2)

/* Entry transition, declared per scene so it is one table rather than a fact
 * buried in each scene's start function.
 *
 * `SOURCE` means the imported scene builds its own: mnvsresults.c:3364 makes a
 * `dLBTransitionDescs` wipe at Results start, which is the wipe/fade support
 * this phase needs and it is already live -- the registry records it instead
 * of making a second one over the top.
 *
 * `NONE` is today's battle entry and MUST stay NONE. mode 163 is a fixed
 * Boundary arm, so `gNdsSceneManagerCurrTransition` reading NONE on every
 * VSBattle entry is the negative control that this row did not put a new
 * visual in front of the match.
 *
 * There is deliberately no port-side transition MAKER here. Introducing one
 * would mean either giving battle entry a transition it does not have today
 * (a Boundary change) or replacing the Results wipe's random pick with a fixed
 * one (a fidelity call that belongs to the owner, not to this row). The menus
 * in P2-1d are the first caller that needs a maker; it is built with them. */
#define NDS_SCENE_TRANSITION_NONE 0u
#define NDS_SCENE_TRANSITION_SOURCE 1u

typedef struct NdsSceneDesc {
    u8 kind;       /* SCKind */
    u8 flags;      /* NDS_SCENE_FLAG_* */
    u8 transition; /* NDS_SCENE_TRANSITION_* */
} NdsSceneDesc;

/* How many scene entries the watermark ring keeps. Three menu -> battle ->
 * results loops is nine entries; sixteen leaves headroom for the Sudden Death
 * entry a timed match can insert without wrapping the evidence away. */
#define NDS_SCENE_MANAGER_RING 16u

/* Registry lookup. NULL when the kind is not a scene this build has. */
const NdsSceneDesc *ndsSceneManagerFind(u32 kind);

/* The single port-owned transition. Writes scene_prev/scene_curr and asks
 * taskman to unwind the current scene. Refused (and counted in
 * gNdsSceneManagerRejectCount) when `next` is not in the registry, which
 * leaves the current scene running rather than parking on a bad kind. */
void ndsSceneManagerRequest(u32 next_kind, u32 prev_kind);

/* Called by the syTaskmanStartTask wrapper in
 * src/import/battleship_sys_taskman.c, around the whole lifetime of one scene:
 * Enter runs before the arena is re-initialised, Exit after the scene's run
 * loop has returned. */
void ndsSceneManagerEnter(const void *arena_start, u32 arena_size);
void ndsSceneManagerExit(void);

/* --- Published state. All of it is read by scripts/probe-scene-loop-walk.ps1
 * and by the board's leak evidence; none of it is read by gameplay. --- */

/* Scene entries and exits observed at the taskman seam. */
extern volatile u32 gNdsSceneManagerEnterCount;
extern volatile u32 gNdsSceneManagerExitCount;
/* Transitions requested through ndsSceneManagerRequest, and refused ones. */
extern volatile u32 gNdsSceneManagerRequestCount;
extern volatile u32 gNdsSceneManagerRejectCount;
/* The scene kind of the current (or last) entry, and the one before it. */
extern volatile u32 gNdsSceneManagerCurrKind;
extern volatile u32 gNdsSceneManagerPrevKind;
/* Arena the scenes declare. Every scene must declare the SAME arena or the
 * "reset" is really a move, and a high-water comparison across scenes is
 * meaningless; a mismatch is counted rather than assumed away. */
extern volatile u32 gNdsSceneManagerArenaBase;
extern volatile u32 gNdsSceneManagerArenaSize;
extern volatile u32 gNdsSceneManagerArenaMismatchCount;
/* Per-entry ring, indexed by (entry index % NDS_SCENE_MANAGER_RING). The three
 * arrays are written together: the scene kind that entry ran, the arena
 * high-water in bytes that entry reached (ptr - start at exit), and the free
 * bytes remaining at exit. THIS IS THE LEAK EVIDENCE: same kind, same
 * high-water, N entries apart. */
extern volatile u8 gNdsSceneManagerRingKind[NDS_SCENE_MANAGER_RING];
extern volatile u32 gNdsSceneManagerRingArenaHigh[NDS_SCENE_MANAGER_RING];
extern volatile u32 gNdsSceneManagerRingArenaFree[NDS_SCENE_MANAGER_RING];
/* Transition ring: ((from << 8) | to) per request, same indexing rule. Proves
 * the flow actually walked menu -> battle -> results -> menu rather than
 * inferring it from a screenshot. */
extern volatile u32 gNdsSceneManagerRingTransition[NDS_SCENE_MANAGER_RING];
/* Entries the wrapper saw for a kind that is not in the registry. Non-zero
 * means the table is incomplete, so a flat-high-water reading from this run
 * covers less than it claims. */
extern volatile u32 gNdsSceneManagerUnregisteredEnterCount;
/* The declared entry transition of the scene currently running. Reads
 * NDS_SCENE_TRANSITION_NONE for every VSBattle entry; that is the negative
 * control for "mode 163 gained no new visual". */
extern volatile u32 gNdsSceneManagerCurrTransition;

/* Remaining automatic hops for the scene-loop walk, and completed loops.
 * Seeded from NDS_R2_SCENE_LOOP_WALK (loop count) on the first scene entry;
 * each bounded scene tail that would otherwise park spends one hop and
 * requests its successor instead. Lab only: the flag is 0 in every published
 * and Boundary configuration, where ndsSceneWalkAdvance returns FALSE without
 * touching anything and mode 163 parks exactly where it did.
 *
 * Declared unconditionally on purpose. Guarding a declaration on a
 * `nds_build_config.h` macro makes the header's contents depend on include
 * order, and a translation unit that included it first would see a different
 * API -- the class of bug this repo has paid for more than once. The gate
 * lives in the definition and at the call sites, not here. */
extern volatile u32 gNdsSceneWalkHopsRemaining;
extern volatile u32 gNdsSceneWalkLoopsCompleted;
/* Returns TRUE when it consumed a hop and requested `next_kind`; the caller
 * must then return to the scene manager instead of parking. */
sb32 ndsSceneWalkAdvance(u32 next_kind);

#endif
