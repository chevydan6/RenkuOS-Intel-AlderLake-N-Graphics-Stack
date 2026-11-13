#!/bin/bash
set -euo pipefail
cd /boot/home/i915-mapping-002
exec > >(tee physical/final-postcheck.log) 2>&1
uname -a
uptime
test "$(cat validation/mapping.EiiwJV/exit.txt)" = 0
cat validation/mapping.EiiwJV/module-lifecycle.log
test ! -e /dev/misc/i915_mapping_test
test ! -e /boot/system/non-packaged/add-ons/kernel/drivers/bin/i915_mapping_test
test ! -L /boot/system/non-packaged/add-ons/kernel/drivers/dev/misc/i915_mapping_test
ls /dev/graphics > physical/final-graphics.log
cmp physical/graphics-before.log physical/final-graphics.log
sha256sum -c physical/staged.sha256
sha256sum -c physical/unchanged-baseline.sha256
(cd /boot/home/i915-mapping-001 && sha256sum -c physical/recovery.sha256)
bash -n rollback-to-original.sh
echo 'PASS final mapping release, actual diagnostic unload, absent files/device and preserved display baseline'
sync
