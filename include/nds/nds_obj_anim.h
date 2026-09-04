#ifndef NDS_OBJ_ANIM_H
#define NDS_OBJ_ANIM_H

/* The few animation and draw entry points imported item source calls.
 *
 * It exists because nine translation units had grown their own copy of the
 * same three externs. That was fine while it was one imported item; the Poke
 * Ball monsters make it thirteen more, each an opportunity to mistype a
 * signature the linker cannot catch, since a wrong prototype for a defined
 * symbol links cleanly and corrupts arguments at runtime.
 *
 * DELIBERATELY NOT include/sys/objanim.h. The build puts `include` ahead of
 * the decomp source root, so a port header named after a decomp header
 * replaces it for decomp translation units too -- and decomp's own objanim.h
 * declares about forty entry points, of which this needs seven. Mirroring the
 * name broke sys/objhelper.c and mv/mvopening/mvopeningroom.c immediately, on
 * gcGetTreeDObjNext, gcSetupCommonDObjs and gcAddMObjAll. Shadowing a decomp
 * header is only safe when the mirror is complete enough for every consumer of
 * the original; a narrow subset needs its own name.
 *
 * Signatures are decomp's, line-cited. The two draw entry points from
 * sys/objdisplay.h ride along at the bottom -- they are one line each and a
 * second header would buy nothing.
 */

#include <sys/objtypes.h>

/* decomp sys/objanim.h:16 */
void gcAddDObjAnimJoint(DObj *dobj, AObjEvent32 *anim_joint, f32 anim_frame);
/* decomp sys/objanim.h:19 */
void gcAddMObjMatAnimJoint(MObj *mobj, AObjEvent32 *matanim_joint,
                           f32 anim_frame);
/* decomp sys/objanim.h:22 */
void gcAddAnimJointAll(GObj *gobj, AObjEvent32 **anim_joints, f32 anim_frame);
/* decomp sys/objanim.h:25 */
void gcAddMatAnimJointAll(GObj *gobj, AObjEvent32 ***p_matanim_joints,
                          f32 anim_frame);
/* decomp sys/objanim.h:52 */
void gcPlayAnimAll(GObj *gobj);

/* decomp sys/objdisplay.h:46 and its DL-links sibling. */
void gcDrawDObjTreeForGObj(GObj *gobj);
void gcDrawDObjTreeDLLinksForGObj(GObj *gobj);

#endif /* NDS_OBJ_ANIM_H */
