#include <cassert>
#include <cstdlib>
using status_t=int; const int B_OK=0,B_ERROR=-1; int sModulesLock;
struct module_image {int ref_count;bool lifecycle_retained;char* path;int image;unsigned int transition_pins;};
int removed,unloaded;
struct Hash {void Remove(module_image*) {++removed;}} hash;
Hash* sModuleImagesHash=&hash;
#define TRACE(x) ((void)0)
#define FATAL(x) ((void)0)
#define ASSERT_LOCKED_RECURSIVE(x) ((void)0)
void unload_kernel_add_on(int){++unloaded;}
#include "unload.inc"
int main(){for(int refs=0;refs<2;refs++)for(int remove=0;remove<2;remove++){
module_image image{refs,true,nullptr,0,0};assert(unload_module_image(&image,remove)==B_ERROR);assert(removed==0&&unloaded==0);
}auto image=(module_image*)calloc(1,sizeof(module_image));assert(unload_module_image(image,true)==B_OK);assert(removed==1&&unloaded==1);}
