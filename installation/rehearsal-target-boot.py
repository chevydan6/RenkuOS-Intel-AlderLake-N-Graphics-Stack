#!/usr/bin/env python3
"""Cold-boot only the disposable installed disk and verify SSH/MCP/Wi-Fi files."""
import json,pathlib,socket,subprocess,time
base=pathlib.Path('/home/dan/renkuos-development/installation');out=base/'private/rehearsal-001';qmp=out/'target-qmp.sock'
ssh=['ssh','-p','10029','-o','BatchMode=yes','-o','StrictHostKeyChecking=yes','-o','ConnectTimeout=3','-o','HostKeyAlias=renku-install-rehearsal','-o','UserKnownHostsFile='+str(out/'target-known-hosts'),'-i','/home/dan/.ssh/id_ed25519','user@127.0.0.1']
cmd=['qemu-system-x86_64','-name','RenkuOS-installed-rehearsal','-enable-kvm','-cpu','host','-smp','2','-m','1024',
 '-drive','if=pflash,format=raw,readonly=on,file=/usr/share/OVMF/OVMF_CODE_4M.fd',
 '-drive',f'if=pflash,format=raw,file={out}/vars.fd',
 '-drive',f'file={out}/target-disk.img,format=raw,if=ide,index=0',
 '-netdev','user,id=net,hostfwd=tcp:127.0.0.1:10029-:22','-device','e1000,netdev=net,addr=0x3',
 '-vga','none','-device','VGA,addr=0x2,vgamem_mb=64,xres=1280,yres=800','-display','none',
 '-qmp',f'unix:{qmp},server=on,wait=off','-serial',f'file:{out}/target-serial.log']
with (out/'target-qemu.log').open('w') as log:
 vm=subprocess.Popen(cmd,stdout=log,stderr=subprocess.STDOUT)
 try:
  for i in range(55):
   if vm.poll() is not None:raise RuntimeError('target VM stopped before SSH')
   p=subprocess.run(ssh+['uname -a'],capture_output=True,text=True,timeout=10)
   if p.returncode==0:break
   time.sleep(2)
  else:raise RuntimeError('installed target SSH unavailable')
  assert 'hrev60072+518' in p.stdout
  query='set -e; df /boot; test -r /boot/home/power-preflight/join-wifi.sh; test -r /boot/home/power-preflight/wifi.private; test -r /boot/home/config/settings/ssh/authorized_keys; sha256sum /boot/home/config/non-packaged/bin/renku-mcp /boot/home/config/non-packaged/add-ons/kernel/drivers/bin/mediatekwifi'
  p=subprocess.run(ssh+[query],capture_output=True,text=True,timeout=30,check=True)
  assert 'Device: /dev/disk/ata/0/master/2' in p.stdout
  assert '6af9faf1efcf767c7a19c298d8a7db5346b09811f2f326f457eec25884672e2c' in p.stdout
  assert '4404c0ff2f52ec651d9a8d1212d9864df8343ae5fd209899d19400340c725f6d' in p.stdout
  (out/'installed-guest-check.txt').write_text(p.stdout)
  sdk=[str(base/'.venv/bin/python'),str(base/'test-sdk.py')]+ssh+['/boot/home/config/non-packaged/bin/renku-mcp']
  subprocess.run(sdk,check=True,timeout=45)
  subprocess.run(ssh+['sync; shutdown -s'],stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL,timeout=15)
  vm.wait(timeout=180)
  if vm.returncode:raise RuntimeError('installed target QEMU exit failure')
 finally:
  if vm.poll() is None:
   try:
    with socket.socket(socket.AF_UNIX) as s:
     s.settimeout(5);s.connect(str(qmp));f=s.makefile('rwb',buffering=0);f.readline();f.write(b'{"execute":"qmp_capabilities"}\n');f.readline();f.write(b'{"execute":"quit"}\n')
    vm.wait(timeout=10)
   except (OSError,subprocess.TimeoutExpired):vm.kill();vm.wait()
record={'installer_completed':True,'target_only_boot':True,'ssh':True,'official_mcp_sdk':True,'wifi_driver_and_configuration_preserved':True,'physical_wifi_tested':False,'guest_power_off':True}
(out/'installed-validation.json').write_text(json.dumps(record,indent=2)+'\n');print(json.dumps(record,indent=2),flush=True)
