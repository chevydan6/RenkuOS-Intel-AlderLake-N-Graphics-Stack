#!/bin/bash
set -euo pipefail
cd /boot/home/i915-mapping-002
sha256sum -c physical/staged.sha256
original=/boot/home/i915-mapping-001
(cd "$original" && sha256sum -c physical/recovery.sha256)
owner=/boot/system/non-packaged/add-ons/kernel/bus_managers/intel_memory_owner
gart=/boot/system/non-packaged/add-ons/kernel/busses/agp_gart/intel
for path in "$owner.mapping-rollback" "$gart.mapping-rollback"; do test ! -e "$path"; done
cp -p "$original/recovery/intel_memory_owner" "$owner.mapping-rollback"
cp -p "$original/recovery/intel_gart" "$gart.mapping-rollback"
cmp "$original/recovery/intel_memory_owner" "$owner.mapping-rollback"
cmp "$original/recovery/intel_gart" "$gart.mapping-rollback"
mv "$owner.mapping-rollback" "$owner"
mv "$gart.mapping-rollback" "$gart"
sha256sum -c "$original/physical/expected-before.sha256"
sync
echo 'Original pre-mapping owner/GART restored on disk. Reboot before further tests.'
