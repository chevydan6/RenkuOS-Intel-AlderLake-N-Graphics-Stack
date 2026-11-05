#!/usr/bin/env python3
"""Boot a private source overlay and a disposable virtual installation disk."""
import json,pathlib,signal,socket,subprocess,time
base=pathlib.Path('/home/dan/renkuos-development/installation/private');out=base/'rehearsal-001';qmp=out/'qmp.sock'
ssh=['ssh','-p','10029','-o','BatchMode=yes','-o','StrictHostKeyChecking=yes','-o','ConnectTimeout=3','-o','HostKeyAlias=renku-wifi-baseline','-o','UserKnownHostsFile='+str(base/'renku-known-hosts'),'-i','/home/dan/.ssh/id_ed25519','user@127.0.0.1']
cmd=['qemu-system-x86_64','-name','RenkuOS-dualboot-rehearsal','-enable-kvm','-cpu','host','-smp','2','-m','1024',
 '-drive','if=pflash,format=raw,readonly=on,file=/usr/share/OVMF/OVMF_CODE_4M.fd',
 '-drive',f'if=pflash,format=raw,file={out}/vars.fd','-device','qemu-xhci,id=xhci,addr=0x4',
 '-drive',f'file={out}/source-overlay.qcow2,format=qcow2,if=none,id=usb','-device','usb-storage,drive=usb,bus=xhci.0',
 '-drive',f'file={out}/target-disk.img,format=raw,if=ide,index=0',
 '-netdev','user,id=net,hostfwd=tcp:127.0.0.1:10029-:22','-device','e1000,netdev=net,addr=0x3',
 '-vga','none','-device','VGA,addr=0x2,vgamem_mb=64,xres=1280,yres=800','-display','none',
 '-qmp',f'unix:{qmp},server=on,wait=off','-serial',f'file:{out}/serial.log']
def interrupt(signum,frame):raise KeyboardInterrupt()
signal.signal(signal.SIGTERM,interrupt)
with (out/'qemu.log').open('w') as log:
 vm=subprocess.Popen(cmd,stdout=log,stderr=subprocess.STDOUT)
 try:
  for i in range(50):
   if vm.poll() is not None:raise RuntimeError('VM stopped')
   p=subprocess.run(ssh+['uname -a'],capture_output=True,text=True,timeout=10)
   if p.returncode==0:break
   time.sleep(2)
  else:raise RuntimeError('VM SSH unavailable')
  assert 'hrev60072+518' in p.stdout
  (out/'ready.json').write_text(json.dumps({'guest':p.stdout.strip(),'pid':vm.pid,'port':10029},indent=2)+'\n')
  print('Disposable VM ready; SSH on localhost:10029.',flush=True)
  deadline=time.monotonic()+900
  while time.monotonic()<deadline and not (out/'stop').exists() and vm.poll() is None:time.sleep(1)
 finally:
  if vm.poll() is None:
   try:
    with socket.socket(socket.AF_UNIX) as s:
     s.settimeout(5);s.connect(str(qmp));f=s.makefile('rwb',buffering=0);f.readline();f.write(b'{"execute":"qmp_capabilities"}\n');f.readline();f.write(b'{"execute":"quit"}\n')
    vm.wait(timeout=10)
   except (OSError,subprocess.TimeoutExpired):vm.kill();vm.wait()
  print('Disposable VM stopped; QEMU exit',vm.returncode,flush=True)
