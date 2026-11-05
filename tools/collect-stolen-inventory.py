#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Read-only Fedora N100 PCI/stolen-memory inventory; run under host-config lock.

Uses fixed PCI reads and kernel-maintained text. No MMIO mapping, PCI writes,
debugfs mount, module loading, display configuration or firmware changes.
"""
import datetime
import json
import os
import pathlib
import re
import subprocess

if os.geteuid() != 0:
    raise SystemExit('root is needed for unredacted PCI configuration and iomem')


def pci(device, registers):
    result = subprocess.run(['setpci', '-s', device, *registers], check=True,
                            capture_output=True, text=True, timeout=10)
    values = result.stdout.splitlines()
    if len(values) != len(registers) or any(not re.fullmatch('[0-9a-fA-F]+', v) for v in values):
        raise SystemExit('unexpected setpci response')
    return dict(zip(registers, values))


identities = {device: pci(device, ['00.w', '02.w']) for device in ('00:00.0', '00:02.0')}
if identities != {
        '00:00.0': {'00.w': '8086', '02.w': '461c'},
        '00:02.0': {'00.w': '8086', '02.w': '46d1'}}:
    raise SystemExit('unexpected hardware; review register selection before collecting')

record = {
    'timestamp_utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
    'kernel': os.uname().release,
    'identities': identities,
    'bridge': pci('00:00.0', ['50.w', 'b0.l', 'b4.l']),
    'gpu': pci('00:02.0', ['50.w', 'c0.l', 'c4.l']),
    'iomem_graphics': [line for line in pathlib.Path('/proc/iomem').read_text().splitlines()
                       if re.search(r'graphics|stolen|0000:00:02.0', line, re.I)],
    'driver': pathlib.Path('/sys/bus/pci/devices/0000:00:02.0/driver').resolve().name,
}
journal = subprocess.run(['dmesg', '--color=never'], capture_output=True,
                         text=True, timeout=10)
record['kernel_memory_messages'] = [line for line in journal.stdout.splitlines()
    if re.search(r'stolen|DSM|WOPCM|reserved.*[Gg][Tt][Tt]|[Gg][Tt][Tt].*reserved', line)]
record['dmesg_status'] = journal.returncode
record['limits'] = ('No protected-range MMIO descriptor or boot scanout PTEs read. '
                    'Linux state does not prove RenkuOS firmware mapping ownership.')
print(json.dumps(record, indent=2))
