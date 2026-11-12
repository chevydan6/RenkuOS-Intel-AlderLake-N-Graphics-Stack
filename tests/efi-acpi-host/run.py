#!/usr/bin/env python3
"""Host regression for patched production acpi_init; no firmware or GPU access."""
import json, os, pathlib, subprocess, sys
root = pathlib.Path(__file__).resolve().parents[2]
out = pathlib.Path(sys.argv[1]).resolve()
out.mkdir(parents=True, exist_ok=True)
repo = sys.argv[2]
revision = '4799bb5a188c4bb6bbef967287640cc21a061a0d'
path = 'src/system/boot/platform/efi/acpi.cpp'
original = subprocess.check_output(['git', '--git-dir='+repo, 'show', revision+':'+path])
source = out/'composed'/path
source.parent.mkdir(parents=True, exist_ok=True)
source.write_bytes(original)
subprocess.run(['git', 'apply', '--check', str(root/'patches/0116-efi-acpi-null-rsdp.patch')], cwd=out/'composed', check=True)
subprocess.run(['git', 'apply', str(root/'patches/0116-efi-acpi-null-rsdp.patch')], cwd=out/'composed', check=True)
prelude = r'''
#include <cassert>
#include <cstdint>
#include <cstring>
using uint32 = uint32_t;
#define TRACE(x) ((void)0)
#define ACPI_20_TABLE_GUID 2
#define ACPI_RSDP_SIGNATURE "RSD PTR "
#define B_OK 0
struct efi_guid { int value; efi_guid(int v):value(v){} bool equals(efi_guid g) const {return value==g.value;} };
struct acpi_rsdp { char signature[8]; int status; };
struct efi_configuration_table { efi_guid VendorGuid; void* VendorTable; };
struct System { efi_configuration_table* ConfigurationTable; size_t NumberOfTableEntries; };
System systemTable; System* kSystemTable = &systemTable;
struct { struct { acpi_rsdp* acpi_root; } arch_args; } gKernelArgs;
int checks=0, hooks=0;
int acpi_check_rsdt(acpi_rsdp* p) { assert(p); checks++; return p->status; }
void arch_handle_acpi() { hooks++; }
'''
main = r'''
int main(int argc, char** argv) {
 assert(argc==2); acpi_rsdp good={{'R','S','D',' ','P','T','R',' '},0};
 acpi_rsdp bad=good; bad.status=-1;
 efi_configuration_table t[]={{2,nullptr},{2,&good},{2,&bad}};
 int expectedChecks=1, expectedHooks=1;
 if (!strcmp(argv[1],"all-null")) {t[1].VendorTable=nullptr; t[2].VendorTable=nullptr; expectedChecks=expectedHooks=0;}
 else if (!strcmp(argv[1],"valid")) {t[0].VendorTable=&good;}
 else if (!strcmp(argv[1],"invalid-then-valid")) {t[0].VendorTable=&bad; expectedChecks=2;}
 else if (!strcmp(argv[1],"wrong-guid")) {t[0].VendorGuid=efi_guid(1);}
 else if (!strcmp(argv[1],"empty")) {expectedChecks=expectedHooks=0;}
 else assert(!strcmp(argv[1],"null-then-valid"));
 systemTable={t,!strcmp(argv[1],"empty") ? size_t(0) : size_t(3)};
 acpi_init(); assert(checks==expectedChecks && hooks==expectedHooks);
 assert(gKernelArgs.arch_args.acpi_root==(expectedHooks ? &good : nullptr));
}
'''
results=[]
for label, data in [('patched',source.read_text()), ('original', original.decode())]:
 # Compile the actual production function, unchanged after extraction.
 function=data[data.index('void\nacpi_init()'):]
 unit=out/(label+'.cpp'); unit.write_text(prelude+function+main)
 binary=out/label
 cmd=['g++','-std=c++11','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-omit-frame-pointer','-no-pie',str(unit),'-o',str(binary)]
 subprocess.run(cmd,check=True)
 cases=['null-then-valid','all-null','valid','invalid-then-valid','wrong-guid','empty'] if label=='patched' else ['null-then-valid','valid']
 for case in cases:
  run=subprocess.run([str(binary),case],capture_output=True,text=True,timeout=5,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0','UBSAN_OPTIONS':'halt_on_error=1'})
  expectedFailure=label=='original' and case=='null-then-valid'
  assert (run.returncode!=0)==expectedFailure, (label,case,run.stderr)
  if expectedFailure: assert 'null pointer' in run.stderr or 'AddressSanitizer' in run.stderr
  results.append(dict(source=label,case=case,exitCode=run.returncode,expectedFailure=expectedFailure,output=run.stdout+run.stderr))
(out/'results.json').write_text(json.dumps(dict(hostOnly=True,leakDetection=False,sourceRevision=revision,cases=results),indent=2)+'\n')
print('Six patched cases and original valid control passed; original null negative control failed as expected. Host only; LeakSanitizer disabled.')
