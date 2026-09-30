"""Host-execute the actual Meta Knight selected-root translation boundary.

Uses the producer's real immutable root/joint metadata and a bounded fixture
native image. GPU drawing, source poses and target lifetime captures remain due.
"""

from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/"scripts/menus"))
from source_test_helpers import braced,function

ASSETS=(ROOT/"src/nds/nds_renderer_assets.c").read_text()
META=(ROOT/"src/nds/generated/nds_native_metaknight.generated.inc").read_text()


class SelectedRootsTests(unittest.TestCase):
    def test_real_selected_route_and_negative_domains(self):
        compiler=shutil.which("gcc") or shutil.which("clang")
        if compiler is None:
            self.skipTest("host C compiler unavailable")
        data=META.replace("#if defined(NDS_P4_METAKNIGHT) && NDS_P4_METAKNIGHT", "")
        data=re.sub(r"^#endif[^\n]*$","",data,flags=re.M)
        route=braced(ASSETS,r"typedef struct NDSNativeMetaKnightSelectedRoute",True)
        program=braced(ASSETS,r"typedef struct NDSNativeMetaKnightProgram",True)
        programs=ASSETS[ASSETS.index("#define NDS_META_PROGRAM"):ASSETS.index("#undef NDS_META_PROGRAM")]
        image_header=(ROOT/"include/nds/generated/nds_native_fighter_image.generated.h").read_text()
        counts="\n".join(re.findall(r"^#define NDS_NATIVE_IMAGE_METAKNIGHT\w*_RUN_UNIQUE_DENSE_COUNT \d+u",image_header,re.M))
        bodies="\n".join(function(ASSETS,name) for name in
                         ("ndsNativeMetaKnightRouteOwned","ndsNativeMetaKnightSourceRoot",
                          "ndsRendererNativeMetaKnightMaterialContext",
                          "ndsRendererNativeMetaKnightMaterialReference",
                          "ndsRendererNativeMetaKnightSelectRoots"))
        head=r'''
#include <stdint.h>
#include <string.h>
#include <stdio.h>
typedef uint32_t u32; typedef int32_t s32; typedef uint16_t u16; typedef uint8_t u8;
typedef int sb32;
enum {FALSE,TRUE};
#define NDS_FTR_COUNT(a) ((u32)(sizeof(a)/sizeof((a)[0])))
#define NDS_NATIVE_FIGHTER_ROOT_MAX 32u
#define NDS_NATIVE_GX_MATRIX_SLOT_MAX 30u
#define NDS_RENDERER_NATIVE_FIGHTER_OWNER_METAKNIGHT 25u
#define NDS_NATIVE_IMAGE_SLOT_METAKNIGHT 25u
#define NDS_NATIVE_IMAGE_SLOT_METAKNIGHT_SKELETON1 26u
#define NDS_NATIVE_IMAGE_SLOT_METAKNIGHT_SKELETON2 27u
#define NDS_NATIVE_MATERIAL_NONE 255u
typedef struct {u32 root_offset;u16 first_epoch,tail_state_first,source_command_count;
 u8 epoch_count,tail_state_count,tail_sync_count,light_preamble;} NDSNativeRoot;
typedef struct {u16 first_run;u8 run_count,material_slot;} NDSNativeEpoch;
typedef struct {u8 matrix_binding;} NDSNativeDenseVertex;
typedef struct {unsigned unused;} NDSNativeRun;
typedef struct {const NDSNativeEpoch *epochs;const NDSNativeRun *runs;
 const NDSNativeDenseVertex *dense_vertices;const u16 *run_first_unique,*run_unique_count,*run_unique_dense;
 u32 epoch_count,run_count,dense_count;} NDSNativeFighterRuntimeTables;
typedef struct {const NDSNativeFighterRuntimeTables *tables;const NDSNativeRoot *roots;u32 root_count;
 const u8 *cross_palette_slots;const u32 (*root_light_preambles)[2];u32 root_light_preamble_count,asset_data_size;
} NDSNativeFighterOwnerRuntime;
static u32 gNdsTaskmanHeapGeneration=9;
static int resident=1;
static s32 ndsRendererNativeOwnerImageResident(u32 slot,u32 detail) {return resident &&slot>=25 &&slot<=27 &&detail<2;}
'''
        globals_=r'''
static NDSNativeFighterRuntimeTables sNdsNativeMetaknightFighterHighTables,sNdsNativeMetaknightFighterLowTables;
static NDSNativeFighterRuntimeTables sNdsNativeMetaknightSkeleton1HighTables,sNdsNativeMetaknightSkeleton1LowTables;
static NDSNativeFighterRuntimeTables sNdsNativeMetaknightSkeleton2HighTables,sNdsNativeMetaknightSkeleton2LowTables;
static NDSNativeMetaKnightSelectedRoute sNdsMetaKnightSelectedRoute;
static NDSNativeFighterOwnerRuntime sNdsNativeMetaknightSelectedOwner;
'''
        main=r'''
#define CHECK(c) do {if(!(c)) {fprintf(stderr,"line %d\n",__LINE__);return 1;}} while(0)
static NDSNativeEpoch epochs[128];static NDSNativeRun runs[128];
static NDSNativeDenseVertex dense[128];static u16 first[128],number[128],unique[128];
static unsigned prepare(unsigned detail,u32 *offsets,u32 *materials,u8 *joints,unsigned selected,u32 *inputs) {
 return ndsRendererNativeMetaKnightSelectRoots(detail,0,(void*)0x1000,75296,17,offsets,materials,joints,joints,selected,inputs);
}
int main(void) {
 u32 offsets[32],materials[32],inputs;u8 joints[32];
 for(unsigned detail=0;detail<2;detail++) {
  const NDSNativeRoot *storage=detail?sNdsNativeMetaknightStorageRootsLow:sNdsNativeMetaknightStorageRoots;
  const NDSNativeMetaknightSourceRoot *source=detail?sNdsNativeMetaknightSourceRootsLow:sNdsNativeMetaknightSourceRoots;
  for(unsigned i=0;i<23;i++) for(unsigned e=0;e<storage[i].epoch_count;e++) {
   unsigned k=storage[i].first_epoch+e;
   epochs[k].first_run=k;epochs[k].run_count=1;first[k]=k;number[k]=1;unique[k]=k;
   epochs[k].material_slot=source[i].material_count?0:255;
   dense[k].matrix_binding=source[i].binding;
  }
  NDSNativeFighterRuntimeTables table={epochs,runs,dense,first,number,unique,128,128,128};
  if(detail) sNdsNativeMetaknightFighterLowTables=table;else sNdsNativeMetaknightFighterHighTables=table;
  for(unsigned i=0;i<13;i++) {offsets[i]=source[i].root_offset;materials[i]=source[i].material_count;joints[i]=source[i].source_joint;}
  CHECK(prepare(detail,offsets,materials,joints,13,&inputs));
  CHECK(inputs==13 &&sNdsNativeMetaknightSelectedOwner.root_count==13);
  CHECK(ndsNativeMetaKnightRouteOwned(detail));
  CHECK(!ndsNativeMetaKnightRouteOwned(1-detail));
  CHECK(sNdsMetaKnightSelectedRoute.source_binding[4]==5); /* sourcejoint13, not denseordinal4 */
  CHECK(sNdsMetaKnightSelectedRoute.source_binding[12]==13);
  /* A hidden wing's geometry can reference its child's matrix while that
     child's own root is not selected. Append only that qualified pose input. */
  offsets[13]=source[13].root_offset;materials[13]=source[13].material_count;joints[13]=30;
  dense[storage[13].first_epoch].matrix_binding=15; /* typed sourcejoint31 */
  CHECK(prepare(detail,offsets,materials,joints,14,&inputs));
  CHECK(inputs==15 &&joints[14]==31 &&sNdsMetaKnightSelectedRoute.binding_input[15]==14);
  CHECK(sNdsNativeMetaknightSelectedOwner.root_count==14); /* tail emits no root */
  /* A reached passive variant selects exactly its storage record, not all
     four sourcejoint34 variants and not a parent character's root. */
  offsets[13]=source[19].root_offset;materials[13]=source[19].material_count;joints[13]=34;
  CHECK(prepare(detail,offsets,materials,joints,14,&inputs));
  CHECK(sNdsMetaKnightSelectedRoute.storage_root[13]==19);
  joints[13]=33;CHECK(!prepare(detail,offsets,materials,joints,14,&inputs));
  CHECK(!sNdsMetaKnightSelectedRoute.valid);
  joints[13]=34;materials[13]++;CHECK(!prepare(detail,offsets,materials,joints,14,&inputs));
  materials[13]--;offsets[13]=offsets[0];joints[13]=joints[0];
  CHECK(!prepare(detail,offsets,materials,joints,14,&inputs)); /* duplicate logical binding */
  CHECK(!prepare(detail,offsets,materials,joints,33,&inputs));
  CHECK(!prepare(2,offsets,materials,joints,13,&inputs));
  CHECK(prepare(detail,offsets,materials,joints,13,&inputs));
  gNdsTaskmanHeapGeneration++;CHECK(!ndsNativeMetaKnightRouteOwned(detail));
  resident=0;CHECK(!prepare(detail,offsets,materials,joints,13,&inputs));
  resident=1;CHECK(prepare(detail,offsets,materials,joints,13,&inputs));
  unsigned k=storage[0].first_epoch;number[k]=562;
  CHECK(!prepare(detail,offsets,materials,joints,13,&inputs)); /* source-span bound */
  number[k]=1;
 }
 /* All electric variants select their own image/roots. Skeleton2 roots with
    source segment-E inheritance use the actual qualified prior binder. */
 for(unsigned skeleton=1;skeleton<3;skeleton++) for(unsigned detail=0;detail<2;detail++) {
  const NDSNativeMetaKnightProgram *p=&sNdsMetaKnightPrograms[skeleton][detail];
  u8 binders[32];
  for(unsigned i=0;i<7;i++) {
   const NDSNativeMetaknightSourceRoot *s=&p->sources[i];
   offsets[i]=s->root_offset;materials[i]=s->material_count;joints[i]=s->source_joint;binders[i]=joints[i];
   if(p->inherited[i]) {materials[i]=1;binders[i]=10;} /* source-permitted changed visibility */
   for(unsigned e=0;e<p->storage[i].epoch_count;e++) {
    unsigned k=p->storage[i].first_epoch+e;
    epochs[k].first_run=k;epochs[k].run_count=1;epochs[k].material_slot=255;
    first[k]=k;number[k]=1;unique[k]=k;dense[k].matrix_binding=s->binding;
   }
  }
  *p->tables=(NDSNativeFighterRuntimeTables){epochs,runs,dense,first,number,unique,128,128,128};
  CHECK(ndsRendererNativeMetaKnightSelectRoots(detail,skeleton,(void*)0x1000,75296,17,
      offsets,materials,binders,joints,7,&inputs));
  CHECK(sNdsMetaKnightSelectedRoute.skeleton==skeleton &&sNdsNativeMetaknightSelectedOwner.root_count==7);
  CHECK(sNdsNativeMetaknightSelectedOwner.tables==p->tables);
  CHECK(!ndsRendererNativeMetaKnightSelectRoots(detail,0,(void*)0x1000,75296,17,
      offsets,materials,binders,joints,7,&inputs)); /* no base/art fallback */
  if(skeleton==2) {
   u32 asset,offset,required,inherited;
   CHECK(ndsRendererNativeMetaKnightMaterialContext(detail,2,14,offsets[3],&required,&inherited));
   CHECK(required==1 &&inherited==1);
   CHECK(ndsRendererNativeMetaKnightMaterialReference(detail,2,14,10,offsets[3],0,&asset,&offset));
   CHECK(asset==5456);
   CHECK(!ndsRendererNativeMetaKnightMaterialReference(detail,2,14,63,offsets[3],0,&asset,&offset));
   binders[3]=63;CHECK(!ndsRendererNativeMetaKnightSelectRoots(detail,2,(void*)0x1000,75296,17,
       offsets,materials,binders,joints,7,&inputs));
   binders[3]=10;materials[3]=0;
   CHECK(!ndsRendererNativeMetaKnightSelectRoots(detail,2,(void*)0x1000,75296,17,
       offsets,materials,binders,joints,7,&inputs));
  }
 }
 return 0;
}
'''
        with tempfile.TemporaryDirectory() as temporary:
            file=Path(temporary)/"selected.c";exe=Path(temporary)/"selected.exe"
            file.write_text(head+counts+"\n"+data+route+program+globals_+programs+bodies+main)
            built=subprocess.run([compiler,"-std=c11","-Wall","-Wextra","-Werror",
                                  "-Wno-unused-const-variable",str(file),"-o",str(exe)],capture_output=True,text=True)
            self.assertEqual(built.returncode,0,built.stderr)
            ran=subprocess.run([str(exe)],capture_output=True,text=True)
            self.assertEqual(ran.returncode,0,ran.stderr)


if __name__=="__main__":unittest.main()
