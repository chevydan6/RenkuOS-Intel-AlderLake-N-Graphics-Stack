#!/bin/bash
# SPDX-License-Identifier: MIT
# Run from the prepared native execution bundle, after its build. Requires a
# controlled reboot: replacing a file does not replace B_KEEP_LOADED code.
set -euo pipefail
owner=/boot/system/non-packaged/add-ons/kernel/bus_managers/intel_memory_owner
driver=/boot/system/non-packaged/add-ons/kernel/drivers/bin/intel_native_execution_bridge
link=/boot/system/non-packaged/add-ons/kernel/drivers/dev/misc/intel_native_execution
test "$(uname -s)" = Haiku
test ! -e previous-intel_memory_owner
test ! -e "$driver" && test ! -e "$link" && test ! -L "$link"
test "$(sha256sum "$owner" | cut -d' ' -f1)" = 0a6ff95a029f125f89fecaab2b899c1346555793b2e620c922837eb27b5a483a
sha256sum -c build/binaries.sha256
cp "$owner" previous-intel_memory_owner
sha256sum previous-intel_memory_owner > previous-owner.sha256
cp /boot/home/i915-native-state-001/baseline-components.sha256 baseline-components.sha256
grep -v 'bus_managers/intel_memory_owner$' baseline-components.sha256 > unchanged-components.sha256
sha256sum -c unchanged-components.sha256
cp build/intel_memory_owner "$owner.native-new"
cmp build/intel_memory_owner "$owner.native-new"
mv "$owner.native-new" "$owner"
cp build/intel_native_execution_bridge "$driver"
ln -s ../../bin/intel_native_execution_bridge "$link"
sha256sum "$owner" "$driver" > installed-execution.sha256
sync
echo 'Native execution owner installed; reboot before invoking gpu_copy_sample.'
echo 'Rollback binary: previous-intel_memory_owner. Reboot also required after rollback.'
