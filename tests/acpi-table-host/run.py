#!/usr/bin/env python3
import pathlib,subprocess,sys,os,json,hashlib
root=pathlib.Path(__file__).resolve().parents[2]; out=pathlib.Path(sys.argv[1]).resolve();out.mkdir(parents=True,exist_ok=True)
subprocess.run(['python3',str(root/'tests/acpi-callback-host/run.py'),str(out/'callback')],check=True)
bus=(out/'callback/composed/src/add-ons/kernel/bus_managers/acpi/BusManager.cpp').read_text()
def function(start):
 a=bus.index(start);return bus[a:bus.index('\n}\n',a)+3]
code=bus[bus.index('enum ACPILifecyclePhase'):bus.index('static uint32 sACPIAttempted')]+function('static status_t\nacpi_retain_lifecycle(')+function('status_t\nget_table(')
(out/'production.inc').write_text(code)
module=(out/'callback/composed/src/add-ons/kernel/bus_managers/acpi/Module.cpp').read_text();assert 'get_table' in module
assert 'if (ready)\n\t\t\t\tsACPILifecycle = ACPIReady;' in bus
cmd=['g++','-std=c++11','-pthread','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-omit-frame-pointer','-no-pie','-I'+str(out),str(root/'tests/acpi-table-host/harness.cpp'),'-o',str(out/'test')]
subprocess.run(cmd,check=True)
logs=[]
for case in ['unused','starting','ready','failure','closed','overlap','overlap-failure']:
 p=subprocess.run([str(out/'test'),case],text=True,capture_output=True,timeout=10,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'});logs.append(dict(case=case,exitCode=p.returncode,output=p.stdout+p.stderr));assert p.returncode==0,p.stderr
(out/'results.json').write_text(json.dumps(dict(passed=True,hostOnly=True,leakDetection=False,limitation='Known traced LeakSanitizer restriction; no native/GPU proof',command=cmd,cases=logs,sourceSha256=hashlib.sha256(bus.encode()).hexdigest()),indent=2)+'\n')
print('7 table admission schedules passed')
