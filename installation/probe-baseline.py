#!/usr/bin/env python3
"""Run under shared Fedora locks; inspect a disposable RenkuOS baseline VM."""
import hashlib,json,pathlib,shutil,socket,subprocess,time,sys
root=pathlib.Path('/home/dan/renkuos-development')
source=root/'testing/power-uefi-20261105/RenkuOS-power-baseline.img'
mcp=len(sys.argv)==2 and sys.argv[1]=='--mcp'
if len(sys.argv)>1 and not mcp:raise SystemExit('usage: probe-baseline.py [--mcp]')
out=root/('testing/renku-install-mcp-001' if mcp else 'testing/renku-install-probe-001')
expected='0a8b87be6986acb3bf39c9a4f21431b1e75ebf56eb5a41fd0e51c479877a62a0'
def digest(p):
 h=hashlib.sha256()
 with p.open('rb') as f:
  for b in iter(lambda:f.read(1048576),b''):h.update(b)
 return h.hexdigest()
assert not out.exists() and digest(source)==expected
assert shutil.disk_usage(out.parent).free>3*1024**3
out.mkdir();disk=out/'session.qcow2';vars=out/'vars.fd';qmp=out/'qmp.sock'
subprocess.run(['qemu-img','convert','-f','raw','-O','qcow2',str(source),str(disk)],check=True,timeout=180)
shutil.copy2('/usr/share/edk2/ovmf/OVMF_VARS.fd',vars)
cmd=['qemu-system-x86_64','-name','RenkuOS-install-probe','-enable-kvm','-cpu','host','-smp','2','-m','768',
 '-drive','if=pflash,format=raw,readonly=on,file=/usr/share/edk2/ovmf/OVMF_CODE.fd',
 '-drive',f'if=pflash,format=raw,file={vars}','-device','qemu-xhci,id=xhci,addr=0x4',
 '-drive',f'file={disk},format=qcow2,if=none,id=usbdisk','-device','usb-storage,drive=usbdisk,bus=xhci.0',
 '-netdev','user,id=net,hostfwd=tcp:127.0.0.1:10024-:22','-device','e1000,netdev=net,addr=0x3',
 '-vga','none','-device','VGA,addr=0x2,vgamem_mb=64,xres=1280,yres=800','-display','none',
 '-qmp',f'unix:{qmp},server=on,wait=off','-serial',f'file:{out}/serial.log']
ssh=['ssh','-F',str(root/'builds/gpu-004/renku-gpu-ssh.conf'),'-o','ConnectTimeout=3','-o','ServerAliveInterval=5','-o','ServerAliveCountMax=2','gpu-renku-guest']
with (out/'qemu.log').open('w') as log:
 vm=subprocess.Popen(cmd,stdout=log,stderr=subprocess.STDOUT)
 try:
  for i in range(40):
   if vm.poll() is not None:raise RuntimeError('VM stopped before readiness')
   p=subprocess.run(ssh+['uname -a'],capture_output=True,text=True,timeout=15)
   if p.returncode==0:break
   time.sleep(2)
  else:raise RuntimeError('SSH unavailable')
  query='uname -a; for x in python3 python node g++ sshd package curl; do command -v "$x" || true; done; test -r /boot/home/power-preflight/join-wifi.sh && echo WIFI_HELPER_PRESENT; test -r /boot/home/power-preflight/wifi.private && echo WIFI_CONFIG_PRESENT; ls /boot/system/add-ons/kernel/drivers/bin/mediatekwifi /boot/home/config/non-packaged/add-ons/kernel/drivers/bin/mediatekwifi 2>/dev/null || true'
  p=subprocess.run(ssh+[query],capture_output=True,text=True,timeout=30,check=True)
  (out/'capabilities.txt').write_text(p.stdout);print(p.stdout,flush=True)
  if mcp:
   archive=root/'builds/gpu-004/renku-mcp-source.tar.gz'
   subprocess.run(['scp','-F',str(root/'builds/gpu-004/renku-gpu-ssh.conf'),str(archive),'gpu-renku-guest:/boot/home/renku-mcp-source.tar.gz'],check=True,timeout=60)
   build='set -e; mkdir -p /boot/home/renku-mcp-test; cd /boot/home/renku-mcp-test; tar xzf /boot/home/renku-mcp-source.tar.gz; g++ -std=c++11 -O2 -Wall -Wextra -Werror renku-mcp.cpp json11/json11.cpp -o renku-mcp'
   subprocess.run(ssh+[build],check=True,timeout=180)
   subprocess.run(['python3',str(root/'builds/gpu-004/test-mcp.py')]+ssh+['/boot/home/renku-mcp-test/renku-mcp'],check=True,timeout=90)
  subprocess.run(ssh+['sync; shutdown -s'],stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL,timeout=15)
  vm.wait(timeout=180)
  if vm.returncode:raise RuntimeError('VM failed to power off cleanly')
 finally:
  if vm.poll() is None:
   try:
    with socket.socket(socket.AF_UNIX) as s:
     s.settimeout(5);s.connect(str(qmp));f=s.makefile('rwb',buffering=0);f.readline();f.write(b'{"execute":"qmp_capabilities"}\n');f.readline();f.write(b'{"execute":"quit"}\n')
    vm.wait(timeout=10)
   except (OSError,subprocess.TimeoutExpired):vm.kill();vm.wait()
assert digest(source)==expected
print('Source unchanged; disposable VM shut down.',flush=True)
