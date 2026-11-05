#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
# Disposable std-VGA VM only. The fixed QEMU IDs are deliberate test controls.
set -eu
test "$(uname -s)" = Haiku
test "$(uname -m)" = x86_64
mkdir validation
exec >validation/build-test.log 2>&1
trap 'result=$?; echo "$result" >validation/exit-status; sync' EXIT
uname -a >validation/kernel.txt
g++ --version >validation/compiler.txt
ls /boot/system/packages/haiku_devel*.hpkg >validation/devel-package.txt
sha256sum -c manifest.sha256 >validation/manifest-check.log
g++ -std=gnu++17 -O2 -g -Wall -Wextra -Werror -Wno-multichar \
    -fno-exceptions -fno-rtti \
    -Isource/headers/private/drivers -Isource/headers/private/graphics/vesa \
    -Isource/headers/private/graphics/common \
    boot-graphics-inventory.cpp -o boot-graphics-inventory
set +e
./boot-graphics-inventory --invalid >validation/invalid.log 2>&1
invalid_status=$?
./boot-graphics-inventory --pci >validation/pci.log 2>&1
pci_status=$?
set -e
test "$invalid_status" -eq 2
test "$pci_status" -eq 3
grep -qx 'bridge_id=12378086' validation/pci.log
grep -qx 'gpu_id=11111234' validation/pci.log
grep -qx 'target_present=false' validation/pci.log
test "$(wc -l <validation/pci.log)" -eq 5
echo 'PASS fixed PCI identity reads reject emulated GPU before target registers'
for attempt in 1 2 3; do
    ./boot-graphics-inventory --framebuffer /dev/graphics/vesa \
        >"validation/framebuffer-$attempt.log" 2>&1
done
cmp validation/framebuffer-1.log validation/framebuffer-2.log
cmp validation/framebuffer-1.log validation/framebuffer-3.log
awk -F= '
    $1 == "width" { width = $2 }
    $1 == "height" { height = $2 }
    $1 == "bytes_per_row" { stride = $2 }
    END { exit !(width > 0 && height > 0 && stride > 0) }
' validation/framebuffer-1.log
grep -qx 'accelerant=vesa.accelerant' validation/framebuffer-1.log
grep -qx 'physical_address_available=false' validation/framebuffer-1.log
grep -qx 'ggtt_translation_verified=false' validation/framebuffer-1.log
echo 'PASS three stable framebuffer inventories; no pixel access'
cat validation/pci.log validation/framebuffer-1.log
readelf -h boot-graphics-inventory >validation/elf-header.txt
readelf -d boot-graphics-inventory >validation/elf-dynamic.txt
sha256sum boot-graphics-inventory >validation/artifact.sha256
tar -czf validation/artifacts.tar.gz boot-graphics-inventory
