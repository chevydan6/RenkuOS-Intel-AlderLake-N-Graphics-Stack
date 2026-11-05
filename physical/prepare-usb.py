#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Fedora, under shared locks: prepare and cold-test an external USB diagnostic."""
import argparse
import hashlib
import json
import os
import pathlib
import shutil
import socket
import subprocess
import time

ROOT = pathlib.Path('/home/dan/renkuos-development')
SOURCE = ROOT / 'testing/power-uefi-20261105/RenkuOS-power-baseline.img'
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output', required=True, type=pathlib.Path)
args = parser.parse_args()
OUT = args.output.resolve()
if OUT.parent != ROOT / 'testing' or not OUT.name.startswith('gpu-physical-'):
    parser.error('output must be a new GPU-owned testing directory')
BUILD = ROOT / 'builds/gpu-004'
BASE_HASH = '0a8b87be6986acb3bf39c9a4f21431b1e75ebf56eb5a41fd0e51c479877a62a0'
def digest(path):
    h = hashlib.sha256()
    with path.open('rb') as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b''):
            h.update(block)
    return h.hexdigest()

if OUT.exists():
    raise SystemExit('use a fresh preparation directory')
for name, expected in (
    ('astra-wifi-joint-boot.sh', '16073dcbcc2dc4b9d8de7176688f59cac063e06eb257d4409cfb856ef48fde28'),
    ('astra-wifi-capture-packet-path.sh', 'f9393abe739668bbcff582db013fbfb8d826fc7b21b3b178700996e1878f5ff5')):
    if digest(pathlib.Path('/tmp') / name) != expected:
        raise SystemExit('Wi-Fi handoff hash mismatch: ' + name)
if digest(SOURCE) != BASE_HASH:
    raise SystemExit('baseline image changed')
if shutil.disk_usage(OUT.parent).free < SOURCE.stat().st_size + 2 * 1024**3:
    raise SystemExit('insufficient space including 2 GiB reserve')
OUT.mkdir()
image = OUT / 'RenkuOS-gpu-boot.img'
subprocess.run(['qemu-img', 'convert', '-f', 'raw', '-O', 'raw', str(SOURCE), str(image)],
               check=True, timeout=180)
shutil.copy2('/usr/share/edk2/ovmf/OVMF_VARS.fd', OUT / 'vars.fd')
ssh = ['ssh', '-F', str(BUILD / 'renku-gpu-ssh.conf'), '-o', 'ConnectTimeout=3',
       '-o', 'ServerAliveInterval=5', '-o', 'ServerAliveCountMax=2', 'gpu-renku-guest']
scp = ['scp', '-F', str(BUILD / 'renku-gpu-ssh.conf')]
transactions = []
for phase in ('prepare', 'cold-capture', 'cold-repeat'):
    phase_dir = OUT / phase
    phase_dir.mkdir()
    qmp = phase_dir / 'qmp.sock'
    command = ['qemu-system-x86_64', '-name', 'RenkuOS-GPU-USB-preparation',
        '-enable-kvm', '-cpu', 'host', '-smp', '2', '-m', '768',
        '-drive', 'if=pflash,format=raw,readonly=on,file=/usr/share/edk2/ovmf/OVMF_CODE.fd',
        '-drive', f'if=pflash,format=raw,file={OUT / "vars.fd"}',
        '-device', 'qemu-xhci,id=xhci,addr=0x4',
        '-drive', f'file={image},format=raw,if=none,id=usbdisk',
        '-device', 'usb-storage,drive=usbdisk,bus=xhci.0',
        '-device', 'usb-tablet,bus=xhci.0',
        '-netdev', 'user,id=gpunet,hostfwd=tcp:127.0.0.1:10024-:22',
        '-device', 'e1000,netdev=gpunet,addr=0x3', '-vga', 'none',
        '-device', 'VGA,addr=0x2,vgamem_mb=64,xres=1280,yres=800', '-display', 'none',
        '-qmp', f'unix:{qmp},server=on,wait=off',
        '-serial', f'file:{phase_dir / "serial.log"}']
    (phase_dir / 'command.json').write_text(json.dumps(command, indent=2)+'\n')
    record = {'phase': phase, 'success': False, 'shutdown': None}
    with (phase_dir / 'qemu.log').open('w') as log:
        vm = subprocess.Popen(command, stdout=log, stderr=subprocess.STDOUT)
        try:
            if phase == 'prepare':
                for attempt in range(30):
                    if vm.poll() is not None:
                        raise RuntimeError('guest stopped before SSH')
                    probe = subprocess.run(ssh + ['uname -a'], capture_output=True,
                                           text=True, timeout=15)
                    with (phase_dir / 'ssh-readiness.log').open('a') as ready:
                        ready.write(f'attempt={attempt+1} exit={probe.returncode}\n'
                                    + probe.stdout + probe.stderr)
                    if probe.returncode == 0:
                        break
                    time.sleep(2)
                else:
                    raise RuntimeError('guest SSH unavailable')
                for source, destination in (
                    (pathlib.Path('/tmp/astra-wifi-joint-boot.sh'), '/boot/home/joint-boot-wifi.sh'),
                    (pathlib.Path('/tmp/astra-wifi-capture-packet-path.sh'), '/boot/home/wifi-capture.sh'),
                    (BUILD / 'boot-snapshot-kit-001.tar.gz', '/boot/home/gpu-boot-kit.tar.gz'),
                    (BUILD / 'collect-and-stop.sh', '/boot/home/gpu-collect-and-stop.sh'),
                    (BUILD / 'prepare-guest.sh', '/boot/home/gpu-prepare-guest.sh')):
                    subprocess.run(scp + [str(source), 'gpu-renku-guest:' + destination],
                                   check=True, timeout=60)
                with (phase_dir / 'stage.log').open('w') as stage:
                    subprocess.run(ssh + ['timeout 90s bash /boot/home/gpu-prepare-guest.sh'],
                                   stdout=stage, stderr=subprocess.STDOUT,
                                   check=True, timeout=105)
                subprocess.run(ssh + ['sync; shutdown -s'], timeout=15,
                               stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            vm.wait(timeout=180)
            if vm.returncode != 0:
                raise RuntimeError('QEMU exited abnormally')
            record.update(success=True, shutdown='guest power-off', vm_exit_status=0)
        finally:
            if vm.poll() is None:
                try:
                    with socket.socket(socket.AF_UNIX) as sock:
                        sock.settimeout(5)
                        sock.connect(str(qmp))
                        stream = sock.makefile('rwb', buffering=0)
                        stream.readline()
                        stream.write(b'{"execute":"qmp_capabilities"}\n')
                        stream.readline()
                        stream.write(b'{"execute":"quit"}\n')
                    vm.wait(timeout=10)
                    record['shutdown'] = 'QMP quit; preparation failed'
                except (OSError, subprocess.TimeoutExpired):
                    vm.kill()
                    vm.wait()
                    record['shutdown'] = 'forced stop; preparation failed'
            record['vm_exit_status'] = vm.returncode
            (phase_dir / 'transaction.json').write_text(json.dumps(record, indent=2)+'\n')
            transactions.append(record)
    if not record['success']:
        raise SystemExit('USB image validation failed')
# Read only the known test-image EFI partition; no host filesystem mount.
subprocess.run(['mcopy', '-i', str(image) + '@@' + str(1343488 * 512),
                '::GPU.TGZ', str(OUT / 'GPU-VM.TGZ')], check=True, timeout=20)
import tarfile
with tarfile.open(OUT / 'GPU-VM.TGZ') as tar:
    def read(name):
        entry = tar.extractfile('gpu-boot-results/' + name)
        if entry is None:
            raise RuntimeError('missing result ' + name)
        return entry.read().decode()
    if read('collection-status.txt').strip() != '0' or read('pci-status.txt').strip() != '3':
        raise RuntimeError('guest collection failed or wrong PCI control')
    if 'target_present=false' not in read('pci.txt'):
        raise RuntimeError('unexpected target identity')
    if read('wifi-hook-status.txt').strip() != '0':
        raise RuntimeError('Wi-Fi hook failed')
    if 'Physical MT7921 absent' not in read('wifi/no-radio.txt'):
        raise RuntimeError('missing expected VM no-radio control')
    if any(line.endswith('=true') for line in read('automount.txt').splitlines()):
        raise RuntimeError('automount enabled')
    for kind in ('boot-info', 'display'):
        if len({read(f'{kind}-{number}.txt') for number in (1, 2, 3)}) != 1:
            raise RuntimeError('unstable metadata')
    (OUT / 'VM-boot-info.txt').write_text(read('boot-info-1.txt'))
    (OUT / 'VM-display.txt').write_text(read('display-1.txt'))
if digest(SOURCE) != BASE_HASH:
    raise RuntimeError('source image changed during preparation')
record = {'source_sha256': BASE_HASH, 'image_sha256': digest(image),
          'image_bytes': image.stat().st_size, 'transactions': transactions,
          'collection_status': 0, 'gpu_execution': False}
(OUT / 'preparation.json').write_text(json.dumps(record, indent=2)+'\n')
print(json.dumps(record, indent=2), flush=True)
