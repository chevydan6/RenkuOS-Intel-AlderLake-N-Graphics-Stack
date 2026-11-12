#!/usr/bin/env python3
"""Exercise unchanged production EFI root selection and lookup under sanitizers."""
import pathlib, subprocess, sys, os, json, hashlib
root=pathlib.Path(__file__).resolve().parents[2]
out=pathlib.Path(sys.argv[1]).resolve(); out.mkdir(parents=True,exist_ok=True)
repo=sys.argv[2]; path='src/system/boot/platform/efi/acpi.cpp'
original=subprocess.check_output(['git','--git-dir='+repo,'show','4799bb5a188c4bb6bbef967287640cc21a061a0d:'+path])
composed=out/'composed'; target=composed/path; target.parent.mkdir(parents=True,exist_ok=True); target.write_bytes(original)
subprocess.run(['git','init','-q',str(composed)],check=True)
for patch in sorted((root/'patches').glob('*.patch')):
 if ('+++ b/'+path) in patch.read_text():
  subprocess.run(['git','apply','--check',str(patch)],cwd=composed,check=True)
  subprocess.run(['git','apply',str(patch)],cwd=composed,check=True)
source=target.read_text()
prelude=r'''
#include <cassert>
#include <cstdint>
#include <cstring>
#include <cstdlib>
using uint8=uint8_t; using uint32=uint32_t; using uint64=uint64_t; using int32=int32_t; using addr_t=uintptr_t; using status_t=int;
#define TRACE(x) ((void)0)
#define B_OK 0
#define B_ERROR -1
#define B_BAD_DATA -2
#define ACPI_XSDT_SIGNATURE "XSDT"
#define ACPI_RSDT_SIGNATURE "RSDT"
struct __attribute__((packed)) acpi_descriptor_header {char signature[4]; uint32 length; uint8 revision,checksum; char rest[26];};
struct __attribute__((packed)) acpi_rsdp_legacy {char signature[8]; uint8 checksum; char oem_id[6]; uint8 revision; uint32 rsdt_address;};
struct __attribute__((packed)) acpi_rsdp_extended {char signature[8]; uint8 checksum; char oem_id[6]; uint8 revision; uint32 rsdt_address; uint32 length; uint64 xsdt_address; uint8 extended_checksum; uint8 reserved[3];};
using acpi_rsdp=acpi_rsdp_extended;
static_assert(sizeof(acpi_descriptor_header)==36,"header");
alignas(8) unsigned char rsdtData[40];
// Align the XSDT entry payload to isolate root-kind publication from the
// existing production unaligned-entry-access issue (outside this task).
struct alignas(8) {uint32 pad; unsigned char data[44];} xsdtStorage;
#define xsdtData xsdtStorage.data
acpi_descriptor_header child;
void checksum(void* p,size_t n,size_t offset) {auto b=(uint8*)p; b[offset]=0; uint8 sum=0; for(size_t i=0;i<n;i++)sum+=b[i]; b[offset]=-sum;}
'''
main=r'''
int main(int argc,char** argv) {
 assert(argc==2); const char* c=argv[1];
 // Every case is a new process; explicitly establish all production static state.
 sAcpiRsdt=nullptr; sAcpiXsdt=nullptr; sNumEntries=-1;
 assert((uintptr_t)&child<=UINT32_MAX && (uintptr_t)rsdtData<=UINT32_MAX);
 memcpy(child.signature,"TEST",4);
 auto rsdt=(acpi_descriptor_header*)rsdtData; auto xsdt=(acpi_descriptor_header*)xsdtData;
 memcpy(rsdt->signature,"RSDT",4);rsdt->length=sizeof(rsdtData);
 memcpy(xsdt->signature,"XSDT",4);xsdt->length=sizeof(xsdtData);
 uint32 p32=(uintptr_t)&child; uint64 p64=(uintptr_t)&child;
 memcpy(rsdtData+36,&p32,4);memcpy(xsdtData+36,&p64,8);
 acpi_rsdp rsdp={};rsdp.revision=2;rsdp.rsdt_address=(uintptr_t)rsdt;rsdp.xsdt_address=0;
 bool success=true,extended=false;
 if(!strcmp(c,"xsdt")){rsdp.xsdt_address=(uintptr_t)xsdt;extended=true;}
 else if(!strcmp(c,"bad-xsdt-signature")){rsdp.xsdt_address=(uintptr_t)xsdt;memcpy(xsdt->signature,"NOPE",4);}
 else if(!strcmp(c,"legacy"))rsdp.revision=0;
 else if(!strcmp(c,"missing")){rsdp.rsdt_address=0;success=false;}
 else if(!strcmp(c,"bad-rsdt-signature")){memcpy(rsdt->signature,"NOPE",4);success=false;}
 else assert(!strcmp(c,"null-xsdt") || !strcmp(c,"bad-rsdt-checksum") || !strcmp(c,"bad-rsdp") || !strcmp(c,"bad-xsdt-checksum"));
 checksum(rsdt,rsdt->length,9); checksum(xsdt,xsdt->length,9);
 if(!strcmp(c,"bad-rsdt-checksum")){rsdt->checksum++;success=false;}
 if(!strcmp(c,"bad-xsdt-checksum")){rsdp.xsdt_address=(uintptr_t)xsdt;xsdt->checksum++;success=false;}
 checksum(&rsdp,20,8);checksum(&rsdp,sizeof(rsdp),32);
 if(!strcmp(c,"bad-rsdp")){rsdp.checksum++;success=false;}
 int result=acpi_check_rsdt(&rsdp);assert((result==B_OK)==success);
 if(success){assert(sAcpiRsdt==(extended?nullptr:rsdt));assert(sAcpiXsdt==(extended?xsdt:nullptr));assert(acpi_find_table("TEST")==&child);assert(acpi_find_table("NONE")==nullptr);assert(sNumEntries==1);}
 else {assert(!sAcpiRsdt && !sAcpiXsdt);assert(acpi_find_table("TEST")==nullptr);assert(sNumEntries==-1);}
}
'''
results=[]
for label,data in [('patched',source),('original',original.decode())]:
 functions=data[data.index('static acpi_descriptor_header*'):data.index('void __attribute__((weak))')]
 unit=out/(label+'.cpp');unit.write_text(prelude+functions+main);binary=out/label
 subprocess.run(['g++','-std=c++11','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-omit-frame-pointer','-no-pie',str(unit),'-o',str(binary)],check=True)
 cases=['null-xsdt','xsdt','bad-xsdt-signature','legacy','missing','bad-rsdt-signature','bad-rsdt-checksum','bad-xsdt-checksum','bad-rsdp'] if label=='patched' else ['null-xsdt','xsdt','legacy']
 for case in cases:
  run=subprocess.run([str(binary),case],capture_output=True,text=True,timeout=5,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0','UBSAN_OPTIONS':'halt_on_error=1'})
  fail=label=='original' and case=='null-xsdt'
  assert (run.returncode!=0)==fail,(label,case,run.stderr)
  if fail:assert 'sAcpiRsdt==' in run.stderr
  results.append(dict(source=label,case=case,exitCode=run.returncode,expectedFailure=fail,output=run.stdout+run.stderr))
(out/'results.json').write_text(json.dumps(dict(hostOnly=True,leakDetection=False,preparedSourceSha256=hashlib.sha256(target.read_bytes()).hexdigest(),cases=results),indent=2)+'\n')
print('Nine production patched cases, two original positive controls and original root-publication negative control matched expectations.')
