#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Read Fedora recovery state under host-config and haiku-vm locks."""
import json
from pathlib import Path
import subprocess
import time

out = Path('/home/dan/renkuos-development/testing/gpu-physical-005')
def run(*args):
    return subprocess.check_output(args, text=True, timeout=30).strip()
record = {'time': time.time(),
          'boot_id': Path('/proc/sys/kernel/random/boot_id').read_text().strip(),
          'root': json.loads(run('findmnt', '--json', '/')),
          'gpu': run('lspci', '-nnk', '-s', '00:02.0'),
          'wifi_driver': str(Path('/sys/bus/pci/devices/0000:01:00.0/driver').resolve()),
          'haiku_service': run('systemctl', '--user', 'is-active', 'haiku-driver-dev.service'),
          'routes': run('ip', 'route')}
assert '8086:46d1' in record['gpu'] and 'Kernel driver in use: i915' in record['gpu']
assert record['wifi_driver'].endswith('/mt7921e')
assert record['haiku_service'] == 'active'
assert 'mmcblk' in record['root']['filesystems'][0]['source']
record['battery'] = {}
for battery in Path('/sys/class/power_supply').glob('BAT*'):
    record['battery'][battery.name] = {
        name: (battery / name).read_text().strip() for name in ('capacity', 'status')}
    assert int(record['battery'][battery.name]['capacity']) >= 30
path = out / 'host-before.json'
if path.exists():
    raise SystemExit('Preserve existing preflight record; reconcile before replacing')
path.write_text(json.dumps(record, indent=2)+'\n')
print('Fedora root, i915, Wi-Fi driver, battery and original VM verified; state recorded')
