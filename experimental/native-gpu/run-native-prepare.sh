#!/bin/bash
# SPDX-License-Identifier: MIT
set -euo pipefail
test "$(uname -s)" = Haiku
module=/boot/system/non-packaged/add-ons/kernel/bus_managers/intel_native_prepare
driver=/boot/system/non-packaged/add-ons/kernel/drivers/bin/intel_native_prepare_bridge
link=/boot/system/non-packaged/add-ons/kernel/drivers/dev/misc/intel_native_prepare
test ! -e "$module" && test ! -e "$driver" && test ! -e "$link" && test ! -L "$link"
test ! -e runtime
mkdir runtime
sha256sum -c final-payload.sha256 > runtime/source-before.log
sha256sum -c build/binaries.sha256
cp /boot/home/i915-native-state-001/baseline-components.sha256 runtime/production.sha256
sha256sum -c runtime/production.sha256 > runtime/production-before.log
sha256sum /boot/system/non-packaged/add-ons/kernel/bus_managers/intel_memory_owner > runtime/owner.sha256
syslog_line=$(wc -l < /var/log/syslog)
installed_module=0
installed_driver=0
installed_link=0
cleanup() {
    status=$?
    trap - EXIT
    if [ "$installed_link" = 1 ]; then
        if [ "$(readlink "$link")" = ../../bin/intel_native_prepare_bridge ]; then rm "$link"; else status=1; fi
    fi
    if [ "$installed_driver" = 1 ]; then
        if cmp build/intel_native_prepare_bridge.driver "$driver"; then rm "$driver"; else status=1; fi
    fi
    if [ "$installed_module" = 1 ]; then
        if cmp build/intel_native_prepare.addon "$module"; then rm "$module"; else status=1; fi
    fi
    unloaded=0
    for attempt in {1..50}; do
        ls /dev/misc > /dev/null
        tail -n +$((syslog_line + 1)) /var/log/syslog | grep NATIVE_GPU_PREPARE_BRIDGE > runtime/bridge-lifecycle.log || true
        if [ ! -e /dev/misc/intel_native_prepare ] && grep -q NATIVE_GPU_PREPARE_BRIDGE_UNINIT runtime/bridge-lifecycle.log; then
            unloaded=1
            break
        fi
        sleep 0.1
    done
    test "$unloaded" = 1 || status=1
    sha256sum -c final-payload.sha256 > runtime/source-after.log || status=1
    sha256sum -c runtime/production.sha256 > runtime/production-after.log || status=1
    sha256sum -c runtime/owner.sha256 > runtime/owner-after.log || status=1
    ps | grep '/boot/system/servers/app_server' > runtime/app-server.log || status=1
    cat runtime/bridge-lifecycle.log runtime/app-server.log
    printf '%s\n' "$status" > runtime/exit.txt
    echo "NATIVE_GPU_PREPARATION_RUNTIME_EXIT=$status"
    echo 'Resident B_KEEP_LOADED preparation module remains in memory until reboot; its installed test file is removed.'
    sync
    exit "$status"
}
trap cleanup EXIT
cp build/intel_native_prepare.addon "$module"
installed_module=1
cp build/intel_native_prepare_bridge.driver "$driver"
installed_driver=1
ln -s ../../bin/intel_native_prepare_bridge "$link"
installed_link=1
for attempt in {1..50}; do
    ls /dev/misc > /dev/null
    test ! -e /dev/misc/intel_native_prepare || break
    sleep 0.1
done
test -e /dev/misc/intel_native_prepare
build/native_prepare | tee runtime/client.log
