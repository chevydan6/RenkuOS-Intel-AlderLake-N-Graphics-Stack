#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Hold the existing cross-boot lock; never switch media or clear an outage."""
import fcntl
import json
import os
from pathlib import Path
import time

os.umask(0o077)
legacy = Path('/root/power-recovery-20261105')
out = Path('/root/gpu-boot-20261105-004')
lock = (legacy / 'physical-test.lock').open('a')
fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
if (out / 'lease.json').exists():
    raise SystemExit('Existing lease record requires reconciliation')
gadget = Path('/sys/kernel/config/usb_gadget/g0')
lun = gadget / 'functions/mass_storage.disk0/lun.0'
snapshot = {name: (lun / name).read_text().strip() for name in ('file', 'ro', 'cdrom')}
snapshot['UDC'] = (gadget / 'UDC').read_text().strip()
(out / 'original-lun.json').write_text(json.dumps(snapshot, indent=2)+'\n')
owner = 'ASTRA-GPU joint diagnostic 20261105-004'
(legacy / 'physical-test.owner').write_text(owner+'\n')
record = {'owner': owner, 'pid': os.getpid(), 'started': time.time(),
          'deadline': time.time()+1800, 'state': 'held'}
def save():
    (out / 'lease.json').write_text(json.dumps(record, indent=2)+'\n')
save()
while time.time() < record['deadline']:
    if (out / 'release-after-recovery').exists():
        record['state'] = 'released-after-recovery'
        break
    time.sleep(1)
else:
    record['state'] = 'expired-recovery-unconfirmed'
record['finished'] = time.time()
save()
# The durable outage reservation must remain until independently verified recovery.
