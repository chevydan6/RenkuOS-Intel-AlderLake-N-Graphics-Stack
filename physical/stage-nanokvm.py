#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Run under Fedora coordination locks; stage without switching USB media."""
import hashlib
import argparse
import json
import pathlib
import shlex
import subprocess

root = pathlib.Path('/home/dan/renkuos-development')
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--resume', action='store_true',
                    help='resume this GPU-owned staging file after its writer has stopped')
parser.add_argument('--candidate', choices=('gpu-physical-004', 'gpu-physical-005'),
                    default='gpu-physical-005')
args = parser.parse_args()
candidate = root / 'testing' / args.candidate
image = candidate / 'RenkuOS-gpu-boot.img'
record = json.loads((candidate / 'preparation.json').read_text())
assert record['collection_status'] == 0
assert all(t['success'] and t['shutdown'] == 'guest power-off'
           for t in record['transactions'])
h = hashlib.sha256()
with image.open('rb') as stream:
    for block in iter(lambda: stream.read(1024 * 1024), b''):
        h.update(block)
assert h.hexdigest() == record['image_sha256']
ssh = ['ssh', '-o', 'BatchMode=yes', '-o', 'StrictHostKeyChecking=yes',
       '-o', 'ConnectTimeout=10', '-o', 'ServerAliveInterval=10',
       '-o', 'ServerAliveCountMax=3', '-o',
       'UserKnownHostsFile=' + str(root / 'builds/gpu-004/gpu-nanokvm-known-hosts')]
destination = '/root/gpu-boot-20261105-004'
remote_image = destination + '/RenkuOS-gpu-boot.img'
# Refuse an existing destination. The previous physical image is only a delta seed.
if args.resume:
    subprocess.run(ssh + ['root@192.168.1.210',
        'test -f ' + remote_image + ' && test ! -L ' + remote_image
        + ' && test "$(cat /sys/kernel/config/usb_gadget/g0/functions/mass_storage.disk0/lun.0/file)" != '
        + remote_image], check=True, timeout=20)
else:
    subprocess.run(ssh + ['root@192.168.1.210',
    'set -eu; umask 077; test ! -e ' + destination + '; mkdir ' + destination
    + '; cp /root/power-recovery-20261105/RenkuOS-power-baseline.img '
    + remote_image], check=True, timeout=300)
print('Synchronizing verified candidate to inactive GPU-owned staging file', flush=True)
subprocess.run(['rsync', '--inplace', '--checksum', '--timeout=60', '-e',
    shlex.join(ssh), str(image), 'root@192.168.1.210:' + remote_image],
    check=True, timeout=600)
print('Verifying full NanoKVM image hash', flush=True)
result = subprocess.run(ssh + ['root@192.168.1.210', 'sha256sum ' + remote_image],
                        check=True, capture_output=True, text=True, timeout=300)
if result.stdout.split()[0] != record['image_sha256']:
    raise SystemExit('NanoKVM staged image hash mismatch; do not attach')
status = {'path': remote_image, 'sha256': record['image_sha256'],
          'media_switched': False}
(candidate / 'nanokvm-staging.json').write_text(json.dumps(status, indent=2)+'\n')
print(json.dumps(status), flush=True)
