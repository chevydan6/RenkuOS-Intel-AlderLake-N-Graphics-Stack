#include <KernelExport.h>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include "dmar-native-protocol.h"
static int maps, deletes, mapFail, deleteFail;
static DmarModule fake;
status_t get_module(const char*,module_info** out) { *out=(module_info*)&fake; return B_OK; }
static status_t table(DmarSnapshot* out) {
 memset(out,0,sizeof(*out)); out->status=0; out->length=80; memcpy(out->bytes,"DMAR",4);out->bytes[4]=80;
 for(unsigned i=0;i<2;i++) { unsigned off=48+16*i;out->bytes[off+2]=16;out->bytes[off+9]=0x10+i*0x10; }
 unsigned char sum=0;for(unsigned i=0;i<80;i++)sum+=out->bytes[i];out->bytes[9]=-sum;return 0;
}
static void* regions[16];
area_id map_physical_memory(const char*,uint64,size_t size,uint32 flags,uint32 protection,void** out) {
 assert(size==4096&&flags==(B_ANY_KERNEL_ADDRESS|B_UNCACHED_MEMORY)&&protection==B_KERNEL_READ_AREA);
 int n=maps++; if(n==mapFail) return B_ERROR; assert(posix_memalign(&regions[n],4096,4096)==0);memset(regions[n],0,4096);
 uint32* p=(uint32*)regions[n];p[0]=0x10;p[2]=0x123;p[7]=0; *out=p;return n;
}
status_t delete_area(area_id area) {deletes++;if(area==deleteFail)return B_ERROR;free(regions[area]);regions[area]=nullptr;return B_OK;}
#include "/home/dan/renkuos-development/gpu-driver/tests/vtd-native-module.cpp"
static void reset(){for(auto &p:regions){free(p);p=nullptr;}maps=deletes=0;mapFail=deleteFail=-1;sDone=false;sResult={};for(auto &x:sMappings)x=-1;}
int main(){fake.snapshot=table;assert(Operations(B_MODULE_INIT)==0); VtdResult r;
 reset();assert(Observe(&r)==0&&r.status==0&&r.count==2&&maps==2&&deletes==2);assert(Observe(&r)==0&&maps==2);
 reset();mapFail=1;assert(Observe(&r)==0&&r.status!=0&&maps==2&&deletes==1&&sMappings[0]==-1);
 reset();deleteFail=0;assert(Observe(&r)==0&&r.status!=0&&maps==1&&deletes==1&&sMappings[0]==0&&regions[0]);assert(Observe(&r)==0&&maps==1&&deletes==1);
 reset();puts("3 actual-module observer mock scenarios passed");}
