#ifndef SSB64_NDS_OPENING_NAME_SCENE_H
#define SSB64_NDS_OPENING_NAME_SCENE_H

void ndsOpeningNameRecordStart(u32 scene_kind);
void ndsOpeningNameRecordFuncStart(u32 scene_kind);
void ndsOpeningNameRecordRunTick(u32 scene_kind, u32 tick);
void ndsOpeningNameRecordFighterDeferred(u32 scene_kind, u32 tick);

#define NDS_OPENING_NAME_SCENE_IMPL(name, scene_kind, wait_tic, next_kind)       \
    void mvOpening##name##FuncRun(GObj *gobj);                                  \
                                                                                \
    static void ndsOpening##name##PatchMovieActorRunFunc(void)                  \
    {                                                                           \
        GObj *gobj = gGCCommonLinks[nGCCommonLinkIDMovie];                      \
                                                                                \
        while (gobj != NULL)                                                     \
        {                                                                       \
            if ((gobj->id == nGCCommonKindMovie) &&                             \
                (gobj->func_run == ndsBaseMVOpening##name##FuncRun))            \
            {                                                                   \
                gobj->func_run = mvOpening##name##FuncRun;                      \
                return;                                                         \
            }                                                                   \
            gobj = gobj->link_next;                                             \
        }                                                                       \
    }                                                                           \
                                                                                \
    void mvOpening##name##FuncRun(GObj *gobj)                                   \
    {                                                                           \
        if (sMVOpening##name##TotalTimeTics < 14)                               \
        {                                                                       \
            ndsBaseMVOpening##name##FuncRun(gobj);                              \
        }                                                                       \
        else                                                                    \
        {                                                                       \
            sMVOpening##name##TotalTimeTics++;                                  \
                                                                                \
            if (scSubsysControllerGetPlayerTapButtons(                          \
                    A_BUTTON | B_BUTTON | START_BUTTON) != FALSE)               \
            {                                                                   \
                gSCManagerSceneData.scene_prev = gSCManagerSceneData.scene_curr;\
                gSCManagerSceneData.scene_curr = nSCKindTitle;                  \
                syTaskmanSetLoadScene();                                        \
            }                                                                   \
            if (sMVOpening##name##TotalTimeTics == 15)                          \
            {                                                                   \
                ndsOpeningNameRecordFighterDeferred(                            \
                    scene_kind, (u32)sMVOpening##name##TotalTimeTics);           \
            }                                                                   \
            if (sMVOpening##name##TotalTimeTics == 60)                          \
            {                                                                   \
                gSCManagerSceneData.scene_prev = gSCManagerSceneData.scene_curr;\
                gSCManagerSceneData.scene_curr = next_kind;                     \
                syTaskmanSetLoadScene();                                        \
            }                                                                   \
        }                                                                       \
        ndsOpeningNameRecordRunTick(scene_kind,                                 \
                                    (u32)sMVOpening##name##TotalTimeTics);       \
    }                                                                           \
                                                                                \
    void mvOpening##name##FuncStart(void)                                       \
    {                                                                           \
        sySchedulerSetTicCount(wait_tic);                                       \
        ndsBaseMVOpening##name##FuncStart();                                    \
        ndsOpening##name##PatchMovieActorRunFunc();                             \
        ndsOpeningNameRecordFuncStart(scene_kind);                              \
    }                                                                           \
                                                                                \
    void mvOpening##name##StartScene(void)                                      \
    {                                                                           \
        ndsOpeningNameRecordStart(scene_kind);                                  \
        dMVOpening##name##TaskmanSetup.func_start =                             \
            mvOpening##name##FuncStart;                                         \
        dMVOpening##name##VideoSetup.zbuffer =                                  \
            SYVIDEO_ZBUFFER_START(320, 240, 0, 10, u16);                       \
        syVideoInit(&dMVOpening##name##VideoSetup);                             \
        dMVOpening##name##TaskmanSetup.scene_setup.arena_start =                \
            ndsTaskmanArenaStart();                                             \
        dMVOpening##name##TaskmanSetup.scene_setup.arena_size =                 \
            ndsTaskmanArenaSize();                                              \
        syTaskmanStartTask(&dMVOpening##name##TaskmanSetup);                    \
    }

#endif
