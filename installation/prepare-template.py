#!/usr/bin/env python3
"""Create an isolated SSH/MCP/Wi-Fi image and cold-test it in QEMU."""
import hashlib,json,os,pathlib,shutil,socket,subprocess,time
root=pathlib.Path('/home/dan/renkuos-development');build=root/'builds/gpu-004'
base=root/'testing/power-uefi-20261105/RenkuOS-power-baseline.img'
source=root/'testing/renku-install-mcp-001/session.qcow2';out=root/'testing/renku-install-template-001'
expected='0a8b87be6986acb3bf39c9a4f21431b1e75ebf56eb5a41fd0e51c479877a62a0'
def digest(path):
 h=hashlib.sha256()
 with path.open('rb') as f:
  for b in iter(lambda:f.read(1024*1024),b''):h.update(b)
 return h.hexdigest()
if out.exists() or digest(base)!=expected:raise RuntimeError('existing output or baseline mismatch')
if shutil.disk_usage(out.parent).free<4*1024**3:raise RuntimeError('need 4 GiB free including reserve')
out.mkdir(mode=0o700);os.umask(0o077)
disk=out/'prepare.qcow2';image=out/'RenkuOS-SSH-MCP-WiFi.img';vars=out/'vars.fd'
subprocess.run(['qemu-img','convert','-f','qcow2','-O','qcow2',str(source),str(disk)],check=True,timeout=180)
shutil.copy2('/usr/share/edk2/ovmf/OVMF_VARS.fd',vars)
ssh=['ssh','-F',str(build/'renku-gpu-ssh.conf'),'-o','ConnectTimeout=3','-o','ServerAliveInterval=5','-o','ServerAliveCountMax=2','gpu-renku-guest']
records=[]
for phase,current,fmt in (('stage',disk,'qcow2'),('cold',image,'raw')):
 qmp=out/(phase+'.sock');record={'phase':phase,'success':False}
 command=['qemu-system-x86_64','-name','RenkuOS-install-template','-enable-kvm','-cpu','host','-smp','2','-m','768',
  '-drive','if=pflash,format=raw,readonly=on,file=/usr/share/edk2/ovmf/OVMF_CODE.fd',
  '-drive',f'if=pflash,format=raw,file={vars}','-device','qemu-xhci,id=xhci,addr=0x4',
  '-drive',f'file={current},format={fmt},if=none,id=usbdisk','-device','usb-storage,drive=usbdisk,bus=xhci.0',
  '-netdev','user,id=net,hostfwd=tcp:127.0.0.1:10024-:22','-device','e1000,netdev=net,addr=0x3',
  '-vga','none','-device','VGA,addr=0x2,vgamem_mb=64,xres=1280,yres=800','-display','none',
  '-qmp',f'unix:{qmp},server=on,wait=off','-serial',f'file:{out}/{phase}-serial.log']
 with (out/(phase+'-qemu.log')).open('w') as log:
  vm=subprocess.Popen(command,stdout=log,stderr=subprocess.STDOUT)
  try:
   for i in range(40):
    if vm.poll() is not None:raise RuntimeError('guest stopped before readiness')
    p=subprocess.run(ssh+['uname -a'],capture_output=True,text=True,timeout=15)
    if p.returncode==0:break
    time.sleep(2)
   else:raise RuntimeError('guest SSH unavailable')
   if 'hrev60072+518' not in p.stdout:raise RuntimeError('unexpected guest')
   if phase=='stage':
    action='set -e; src=/boot/home/renku-mcp-test; dest=/boot/home/config/non-packaged/bin/renku-mcp; test -f "$src/renku-mcp"; test ! -e "$dest"; test -r /boot/home/power-preflight/join-wifi.sh; test -r /boot/home/power-preflight/wifi.private; mkdir -p /boot/home/config/non-packaged/bin /boot/home/config/non-packaged/documentation/renku-mcp; cp "$src/renku-mcp" "$dest"; chmod 755 "$dest"; cp "$src/json11/LICENSE.txt" /boot/home/config/non-packaged/documentation/renku-mcp/json11-LICENSE.txt; sha256sum "$dest"; rm -r -- "$src"; rm -- /boot/home/renku-mcp-source.tar.gz; sync'
    p=subprocess.run(ssh+[action],capture_output=True,text=True,timeout=30,check=True)
    (out/'binary.sha256').write_text(p.stdout)
   else:
    check='set -e; test -r /boot/home/power-preflight/join-wifi.sh; test -r /boot/home/power-preflight/wifi.private; test ! -e /boot/home/renku-mcp-test; sha256sum /boot/home/config/non-packaged/bin/renku-mcp'
    p=subprocess.run(ssh+[check],capture_output=True,text=True,timeout=30,check=True)
    if p.stdout!=(out/'binary.sha256').read_text():raise RuntimeError('binary changed on cold boot')
   subprocess.run(['python3',str(build/'test-mcp.py')]+ssh+['/boot/home/config/non-packaged/bin/renku-mcp'],check=True,timeout=90)
   subprocess.run(ssh+['sync; shutdown -s'],stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL,timeout=15)
   vm.wait(timeout=180)
   if vm.returncode:raise RuntimeError('abnormal QEMU exit')
   record.update(success=True,shutdown='guest power-off',vm_exit_status=0)
  finally:
   if vm.poll() is None:
    try:
     with socket.socket(socket.AF_UNIX) as s:
      s.settimeout(5);s.connect(str(qmp));f=s.makefile('rwb',buffering=0);f.readline();f.write(b'{"execute":"qmp_capabilities"}\n');f.readline();f.write(b'{"execute":"quit"}\n')
     vm.wait(timeout=10)
    except (OSError,subprocess.TimeoutExpired):vm.kill();vm.wait()
   record['vm_exit_status']=vm.returncode
   (out/(phase+'-transaction.json')).write_text(json.dumps(record,indent=2)+'\n')
   records.append(record)
 if not record['success']:raise RuntimeError('template test failed')
 if phase=='stage':
  subprocess.run(['qemu-img','check',str(disk)],check=True,timeout=60)
  subprocess.run(['qemu-img','convert','-f','qcow2','-O','raw',str(disk),str(image)],check=True,timeout=180)
if digest(base)!=expected:raise RuntimeError('baseline changed')
manifest={'image':str(image),'image_sha256':digest(image),'image_bytes':image.stat().st_size,
 'binary_sha256':(out/'binary.sha256').read_text().split()[0], 'source_sha256':expected,
 'wifi':'physical-test baseline helper and configuration preserved; long-term stability unproven',
 'mcp':'stdio over SSH, system_info tool', 'transactions':records}
(out/'template.json').write_text(json.dumps(manifest,indent=2)+'\n')
print(json.dumps(manifest,indent=2),flush=True)
