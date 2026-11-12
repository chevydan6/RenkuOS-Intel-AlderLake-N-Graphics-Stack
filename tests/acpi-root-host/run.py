#!/usr/bin/env python3
"""Exercise the prepared production root-pointer function, including a bounded control."""
import pathlib, subprocess, sys, os, json
out=pathlib.Path(sys.argv[1]).resolve();out.mkdir(parents=True,exist_ok=True)
source=pathlib.Path(sys.argv[2]).read_text()
start=source.index('ACPI_PHYSICAL_ADDRESS\nAcpiOsGetRootPointer()'); end=source.index('\n}',start)+2
function=source[start:end]
prelude=r'''
#include <cassert>
#include <cstdint>
#include <cstring>
#include <cstdlib>
using phys_addr_t = uint64_t;
using ACPI_PHYSICAL_ADDRESS = uint64_t;
#define DEBUG_FUNCTION() ((void)0)
static uint64_t sACPIRoot;
static void* payload;
static size_t extent;
static int calls;
void* get_boot_item(const char* name,size_t* size) { assert(!strcmp(name,"ACPI_ROOT_POINTER")); calls++; if(size) *size=extent; return payload; }
uint64_t AeLocalGetRootPointer() {return 73;}
'''
main=r'''
int main(int argc,char** argv) {
 assert(argc==2); int mode=atoi(argv[1]);
#ifndef _KERNEL_MODE
 assert(AcpiOsGetRootPointer()==73 && calls==0); return 0;
#endif
 uint64_t value=0xfedcba9876543210ULL;
 if(mode==0) {payload=nullptr;extent=128;assert(AcpiOsGetRootPointer()==0 && sACPIRoot==0);}
 if(mode==1) {for(size_t n=0;n<sizeof(value);n++){payload=malloc(n?n:1);extent=n;assert(AcpiOsGetRootPointer()==0 && sACPIRoot==0);free(payload);} assert(calls==8);}
 if(mode==2 || mode==3) {char bytes[32];payload=bytes+1;extent=mode==2?8:16;memcpy(payload,&value,8);assert(AcpiOsGetRootPointer()==value && calls==1);payload=nullptr;assert(AcpiOsGetRootPointer()==value && calls==1);}
 if(mode==4) {uint64_t zero=0;payload=&zero;extent=8;assert(AcpiOsGetRootPointer()==0);payload=&value;assert(AcpiOsGetRootPointer()==value && calls==2);}
 if(mode==5) {payload=malloc(1);extent=1;(void)AcpiOsGetRootPointer();free(payload);}
}
'''
original=function.replace('size_t size = 0;\n\t\tconst void* acpiRootPointer = get_boot_item("ACPI_ROOT_POINTER", &size);\n\t\tif (acpiRootPointer != NULL && size >= sizeof(phys_addr_t)) {\n\t\t\tphys_addr_t root;\n\t\t\tmemcpy(&root, acpiRootPointer, sizeof(root));\n\t\t\tsACPIRoot = root;\n\t\t}', 'phys_addr_t* acpiRootPointer = (phys_addr_t*)get_boot_item("ACPI_ROOT_POINTER", NULL);\n\t\tif (acpiRootPointer != NULL)\n\t\t\tsACPIRoot = *acpiRootPointer;')
assert original!=function
results=[]
for label,body,kernel,cases in [('patched',function,True,range(5)),('nonkernel',function,False,[0]),('original',original,True,[5])]:
 unit=out/(label+'.cpp');unit.write_text(prelude+body+main);binary=out/label
 cmd=['g++','-std=c++11','-fsanitize=address,undefined','-fno-omit-frame-pointer','-no-pie',str(unit),'-o',str(binary)]+(['-D_KERNEL_MODE'] if kernel else [])
 subprocess.run(cmd,check=True)
 for case in cases:
  proc=subprocess.run([str(binary),str(case)],capture_output=True,text=True,timeout=5,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0','UBSAN_OPTIONS':'halt_on_error=1'})
  negative=label=='original';assert (proc.returncode!=0)==negative,proc.stderr
  if negative:assert 'heap-buffer-overflow' in proc.stderr
  results.append(dict(label=label,case=case,exitCode=proc.returncode,expectedFailure=negative,output=proc.stderr))
(out/'results.json').write_text(json.dumps({'hostOnly':True,'leakDetection':False,'cases':results},indent=2)+'\n')
print('Production function: six positive runs passed; original undersized control failed as expected. Host only; LeakSanitizer disabled.')
