// SPDX-License-Identifier: MIT
#include <image.h>
#include <OS.h>
#include <dlfcn.h>
#include <stdio.h>
#include <string.h>
int main() {
 void* vk=dlopen("/boot/home/native-indexed-094/libvulkan.so",RTLD_NOW);
 if(!vk){fprintf(stderr,"%s\n",dlerror());return 1;}
 int32 cookie=0;image_info info;
 while(get_next_image_info(B_CURRENT_TEAM,&cookie,&info)==B_OK)
  if(strstr(info.name,"vulkan") || strstr(info.name,"compiler")
   || strstr(info.name,"drm"))printf("%s\n",info.name);
 return 0;
}
