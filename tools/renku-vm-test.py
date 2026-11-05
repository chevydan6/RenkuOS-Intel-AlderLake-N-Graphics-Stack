#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Fedora: clone a stopped guest, test components, and stop the private VM.

Invoke inside coord.py with host-config, haiku-vm and build held throughout.
No PCI passthrough. Source disk, host graphics and existing guests are unchanged.
"""
import argparse
import json
import pathlib
import shutil
import socket
import subprocess
import time

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--disk', required=True, type=pathlib.Path)
parser.add_argument('--archive', required=True, type=pathlib.Path)
parser.add_argument('--ssh-config', required=True, type=pathlib.Path)
parser.add_argument('--output', required=True, type=pathlib.Path)
parser.add_argument('--run-name', required=True)
parser.add_argument('--port', type=int, default=10024)
args = parser.parse_args()
out = args.output.resolve()
# Leave space for logs, the coordinator and guest build growth. Measure a
# flattened destination before creating anything; source image locking applies.
measure = json.loads(subprocess.check_output(
    ['qemu-img', 'measure', '-f', 'qcow2', '-O', 'qcow2', '--output=json',
     str(args.disk.resolve())], text=True, timeout=60))
required = int(measure['required']) + 2 * 1024 * 1024 * 1024
storage_parent = out.parent
while not storage_parent.exists():
    storage_parent = storage_parent.parent
if shutil.disk_usage(storage_parent).free < required:
    raise SystemExit(f'insufficient free space: need {required} bytes including 2 GiB reserve')
out.mkdir(parents=True, exist_ok=False)
disk = out / 'session.qcow2'
# qemu-img's source locking deliberately rejects a disk opened by another VM.
subprocess.run(['qemu-img', 'convert', '-f', 'qcow2', '-O', 'qcow2',
                str(args.disk.resolve()), str(disk)], check=True, timeout=180)
subprocess.run(['qemu-img', 'check', str(disk)], check=True, timeout=60)
ssh = ['ssh', '-F', str(args.ssh_config.resolve()), '-o', 'ConnectTimeout=3',
       '-o', 'ServerAliveInterval=5', '-o', 'ServerAliveCountMax=2', 'gpu-renku-guest']
qmp = out / 'qmp.sock'
command = ['qemu-system-x86_64', '-name', 'RenkuOS-GPU-component-test',
           '-enable-kvm', '-cpu', 'host', '-smp', '2', '-m', '768',
           '-drive', f'file={disk},format=qcow2,if=ide',
           '-netdev', f'user,id=gpunet,hostfwd=tcp:127.0.0.1:{args.port}-:22',
           '-device', 'e1000,netdev=gpunet', '-vga', 'std', '-display', 'none',
           '-qmp', f'unix:{qmp},server=on,wait=off',
           '-serial', f'file:{out / "serial.log"}']
(out / 'qemu-command.json').write_text(json.dumps(command, indent=2) + '\n')
result = {'component_exit_status': None, 'shutdown': None, 'gpu_execution': False}
with (out / 'qemu.log').open('w') as log:
    vm = subprocess.Popen(command, stdout=log, stderr=subprocess.STDOUT)
    try:
        ready = False
        for attempt in range(20):
            if vm.poll() is not None:
                raise RuntimeError('private VM exited before SSH readiness')
            probe = subprocess.run(ssh + ['uname -a'], capture_output=True,
                                   text=True, timeout=15)
            if probe.returncode == 0:
                (out / 'guest-identity.txt').write_text(probe.stdout)
                ready = True
                break
            time.sleep(3)
        if not ready:
            raise RuntimeError('private guest SSH did not become ready')
        script = pathlib.Path(__file__).with_name('guest-component-test.sh')
        run = subprocess.run(['bash', str(script), str(args.archive.resolve()),
                              args.run_name, str(args.ssh_config.resolve()),
                              'gpu-renku-guest', str(out / 'results')], timeout=290)
        result['component_exit_status'] = run.returncode
    finally:
        if vm.poll() is None:
            try:
                subprocess.run(ssh + ['sync; shutdown -s'], timeout=15,
                               stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
                vm.wait(timeout=30)
                result['shutdown'] = 'guest shutdown'
            except (subprocess.TimeoutExpired, OSError):
                # Only this disposable VM is stopped. Never terminate a shared guest.
                try:
                    with socket.socket(socket.AF_UNIX) as sock:
                        sock.settimeout(3)
                        sock.connect(str(qmp))
                        stream = sock.makefile('rwb', buffering=0)
                        stream.readline()
                        stream.write(b'{"execute":"qmp_capabilities"}\n')
                        stream.readline()
                        stream.write(b'{"execute":"quit"}\n')
                    vm.wait(timeout=10)
                    result['shutdown'] = 'QMP quit; discard guest disk'
                except (OSError, subprocess.TimeoutExpired):
                    vm.kill()
                    vm.wait()
                    result['shutdown'] = 'forced stop; discard guest disk'
        else:
            result['shutdown'] = 'VM already stopped'
        result['vm_exit_status'] = vm.returncode
        (out / 'transaction.json').write_text(json.dumps(result, indent=2) + '\n')
raise SystemExit(result['component_exit_status'] if result['component_exit_status'] is not None else 1)
