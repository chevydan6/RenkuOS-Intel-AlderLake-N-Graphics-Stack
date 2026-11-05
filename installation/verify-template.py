#!/usr/bin/env python3
"""Boot a throwaway overlay and verify native Wi-Fi binary and MCP over SSH."""
import hashlib,json,pathlib,shutil,socket,subprocess,time
r=pathlib.Path('/home/dan/renkuos-development');build=r/'builds/gpu-004'
src=r/'testing/renku-install-template-001';out=r/'testing/renku-install-verify-001'
m=json.loads((src/'template.json').read_text());image=pathlib.Path(m['image'])
def sha(p):
 h=hashlib.sha256()
 with p.open('rb') as f:
  for b in iter(lambda:f.read(1048576),b''):h.update(b)
 return h.hexdigest()
assert not out.exists() and sha(image)==m['image_sha256']
out.mkdir(mode=0o700);overlay=out/'overlay.qcow2';vars=out/'vars.fd';qmp=out/'qmp.sock'
subprocess.run(['qemu-img','create','-f','qcow2','-F','raw','-b',str(image),str(overlay)],check=True,timeout=30)
shutil.copy2('/usr/share/edk2/ovmf/OVMF_VARS.fd',vars)
ssh=['ssh','-F',str(build/'renku-gpu-ssh.conf'),'-o','ConnectTimeout=3','-o','ServerAliveInterval=5','-o','ServerAliveCountMax=2','gpu-renku-guest']
cmd=['qemu-system-x86_64','-name','RenkuOS-install-verify','-enable-kvm','-cpu','host','-smp','2','-m','768',
 '-drive','if=pflash,format=raw,readonly=on,file=/usr/share/edk2/ovmf/OVMF_CODE.fd',
 '-drive',f'if=pflash,format=raw,file={vars}','-device','qemu-xhci,id=xhci,addr=0x4',
 '-drive',f'file={overlay},format=qcow2,if=none,id=usb','-device','usb-storage,drive=usb,bus=xhci.0',
 '-netdev','user,id=net,hostfwd=tcp:127.0.0.1:10024-:22','-device','e1000,netdev=net,addr=0x3',
 '-vga','none','-device','VGA,addr=0x2,vgamem_mb=64,xres=1280,yres=800','-display','none',
 '-qmp',f'unix:{qmp},server=on,wait=off','-serial',f'file:{out}/serial.log']
with (out/'qemu.log').open('w') as log:
 vm=subprocess.Popen(cmd,stdout=log,stderr=subprocess.STDOUT)
 try:
  for i in range(40):
   if vm.poll() is not None:raise RuntimeError('guest stopped before SSH')
   p=subprocess.run(ssh+['uname -a'],capture_output=True,text=True,timeout=15)
   if p.returncode==0:break
   time.sleep(2)
  else:raise RuntimeError('SSH unavailable')
  assert 'hrev60072+518' in p.stdout
  query='set -e; test -r /boot/home/power-preflight/join-wifi.sh; test -r /boot/home/power-preflight/wifi.private; test ! -e /boot/home/renku-mcp-test; sha256sum /boot/home/config/non-packaged/add-ons/kernel/drivers/bin/mediatekwifi /boot/home/config/non-packaged/bin/renku-mcp'
  p=subprocess.run(ssh+[query],capture_output=True,text=True,timeout=30,check=True)
  lines=p.stdout.splitlines();assert lines[0].split()[0]=='4404c0ff2f52ec651d9a8d1212d9864df8343ae5fd209899d19400340c725f6d';assert lines[1].split()[0]==m['binary_sha256']
  (out/'binary-hashes.txt').write_text(p.stdout)
  subprocess.run(['python3',str(build/'test-mcp.py')]+ssh+['/boot/home/config/non-packaged/bin/renku-mcp'],check=True,timeout=90)
  subprocess.run(ssh+['sync; shutdown -s'],stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL,timeout=15)
  vm.wait(timeout=180)
  if vm.returncode:raise RuntimeError('guest did not power off cleanly')
 finally:
  if vm.poll() is None:
   try:
    with socket.socket(socket.AF_UNIX) as s:
     s.settimeout(5);s.connect(str(qmp));f=s.makefile('rwb',buffering=0);f.readline();f.write(b'{"execute":"qmp_capabilities"}\n');f.readline();f.write(b'{"execute":"quit"}\n')
    vm.wait(timeout=10)
   except (OSError,subprocess.TimeoutExpired):vm.kill();vm.wait()
assert sha(image)==m['image_sha256']
record={'image_sha256':m['image_sha256'],'mcp_sha256':m['binary_sha256'],'wifi_driver_sha256':lines[0].split()[0],'guest_power_off':True,'source_unchanged':True,'physical_wifi_tested_in_this_vm':False}
(out/'validation.json').write_text(json.dumps(record,indent=2)+'\n');print(json.dumps(record,indent=2),flush=True)
