#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Fixed N100 protected-stolen descriptor read through installed IGT 2.4.

Run under the shared host-config lock. IGT maps BAR0 writable internally, but
only its CPU-MMIO read command is invoked. No batchbuffer, register write,
debugfs mount or driver rebind is requested. A Linux forcewake handle is held
explicitly; unavailable prerequisites are errors, never bypassed.
"""
import datetime
import json
import os
import pathlib
import subprocess

device = pathlib.Path('/sys/bus/pci/devices/0000:00:02.0')
if os.geteuid() != 0:
    raise SystemExit('root required')
if (device / 'vendor').read_text().strip() != '0x8086' or (device / 'device').read_text().strip() != '0x46d1':
    raise SystemExit('unexpected GPU')
if (device / 'driver').resolve().name != 'i915':
    raise SystemExit('i915 must own the GPU')
mounts = [line.split() for line in pathlib.Path('/proc/mounts').read_text().splitlines()]
if not any(fields[1:3] == ['/sys/kernel/debug', 'debugfs'] for fields in mounts):
    raise SystemExit('debugfs must already be mounted; refusing automatic setup')
bar = [int(value, 16) for value in (device / 'resource').read_text().splitlines()[0].split()]
if bar[0] != 0xbf000000 or bar[1] - bar[0] + 1 <= 0x1082c7:
    raise SystemExit('BAR0 differs from verified inventory')
version = subprocess.check_output(['rpm', '-qf', '/usr/bin/intel_reg'], text=True).strip()
if version != 'igt-gpu-tools-2.4-1.fc44.x86_64':
    raise SystemExit('utility version changed; review its access path')

debugfs = pathlib.Path('/sys/kernel/debug/dri/0000:00:02.0')
wake_path = next((debugfs / name for name in ('forcewake_all', 'i915_forcewake_user')
                  if (debugfs / name).exists()), None)
if wake_path is None:
    raise SystemExit('existing i915 forcewake handle unavailable')
wake = os.open(wake_path, os.O_RDONLY | os.O_CLOEXEC)
try:
    reads = []
    for _ in range(2):
        result = subprocess.run(['/usr/bin/intel_reg', '--pci-slot', '0000:00:02.0',
                                 'read', 'mmio:0x1082c0', 'mmio:0x1082c4'],
                                env={'PATH': '/usr/sbin:/usr/bin:/sbin:/bin', 'LC_ALL': 'C'},
                                capture_output=True, text=True, timeout=10, check=True)
        reads.append({'stdout': result.stdout, 'stderr': result.stderr})
finally:
    os.close(wake)

if (device / 'driver').resolve().name != 'i915':
    raise SystemExit('GPU ownership changed during collection')
print(json.dumps({
    'timestamp_utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
    'utility': version,
    'source_reference': 'IGT v2.4 e6b603e4984e4ce30fe1cc2705734d0f130e0550',
    'forcewake': str(wake_path),
    'registers': ['0x1082c0', '0x1082c4'],
    'reads': reads,
    'driver_after': 'i915',
    'gpu_commands_submitted': False,
}, indent=2))
