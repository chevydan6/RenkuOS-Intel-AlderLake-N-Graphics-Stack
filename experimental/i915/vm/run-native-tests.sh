#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
set -eu
cd "$(dirname "$0")/.."
test "$(uname -s)" = Haiku
test "$(uname -m)" = x86_64
mkdir validation
exec >validation/native-test.log 2>&1
installed=0
driver=/boot/system/non-packaged/add-ons/kernel/drivers/bin/i915_linuxkpi_test
device=/boot/system/non-packaged/add-ons/kernel/drivers/dev/misc/i915_linuxkpi_test
cleanup() {
    result=$?
    if [ "$installed" -eq 1 ]; then
        rm -f "$device" "$driver" || result=1
        if [ -e "$device" ] || [ -L "$device" ] || [ -e "$driver" ]; then result=1; fi
    fi
    echo "$result" >validation/exit-status
    sync
    exit "$result"
}
trap cleanup EXIT
uname -a >validation/kernel.txt
sha256sum -c manifest.sha256 >validation/payload-check.txt
bash vm/build-native.sh
sync # Preserve compiled artifacts and logs even if the experimental test panics.
test ! -e "$driver"
test ! -e "$device"
mkdir -p "$(dirname "$driver")" "$(dirname "$device")"
installed=1
cp build/native/i915_linuxkpi_test.driver "$driver"
ln -s ../../bin/i915_linuxkpi_test "$device"
sync
# devfs processes driver add/remove notifications asynchronously. Reinstallation
# can return ENOENT before it has published the new device; bound the real wait.
published=0
for attempt in {1..100}; do
    ls /dev/misc >/dev/null
    if [ -e /dev/misc/i915_linuxkpi_test ]; then published=1; break; fi
    sleep 0.1
done
test "$published" -eq 1
echo 'Running native kernel tests'
build/native/linuxkpi_native_probe --repeat 8 >validation/probe.log
build/native/linuxkpi_native_probe --repeat 4 >validation/concurrent-a.log &
first=$!
build/native/linuxkpi_native_probe --repeat 4 >validation/concurrent-b.log &
second=$!
set +e
wait "$first"
first_status=$?
wait "$second"
second_status=$?
set -e
test "$first_status" -eq 0
test "$second_status" -eq 0
cp build/native/kernel.txt build/native/compiler.txt build/native/devel-packages.txt \
    build/native/binaries.sha256 build/native/imports.txt \
    build/native/execution-imports.txt validation/
echo 'PASS native kernel services, repeated and concurrent clients; GPU execution not tested'
