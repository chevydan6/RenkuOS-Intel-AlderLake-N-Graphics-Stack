#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
# Disposable std-VGA VM only. The fixed QEMU IDs are deliberate test controls.
set -eu
test "$(uname -s)" = Haiku
test "$(uname -m)" = x86_64
mkdir validation
exec >validation/build-test.log 2>&1
diagnostic_bin=/boot/system/non-packaged/add-ons/kernel/drivers/bin/boot_graphics
diagnostic_dev=/boot/system/non-packaged/add-ons/kernel/drivers/dev/misc/boot_graphics
installed=0
staging_dir=
cleanup() {
    result=$?
    cleanup_status=0
    if [ "$installed" -eq 1 ]; then
        rm -f "$diagnostic_dev" "$diagnostic_bin" || cleanup_status=1
        if [ -e "$diagnostic_dev" ] || [ -L "$diagnostic_dev" ] \
            || [ -e "$diagnostic_bin" ] || [ -L "$diagnostic_bin" ]; then
            cleanup_status=1
        fi
    fi
    if [ -n "$staging_dir" ]; then
        rm -f "$staging_dir/boot_graphics" || cleanup_status=1
        rmdir "$staging_dir" || cleanup_status=1
    fi
    echo "cleanup_status=$cleanup_status" >validation/cleanup.log
    if [ "$result" -eq 0 ] && [ "$cleanup_status" -ne 0 ]; then result=1; fi
    echo "$result" >validation/exit-status
    sync
    exit "$result"
}
trap cleanup EXIT
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
g++ -std=gnu++17 -O2 -g -Wall -Wextra -Werror -fno-exceptions -fno-rtti \
    -I. tests/boot-framebuffer-info-test.cpp -o boot-framebuffer-info-test
./boot-framebuffer-info-test
g++ -std=gnu++17 -O2 -g -Wall -Wextra -Werror -fno-exceptions -fno-rtti \
    -I. tests/boot-graphics-driver-test.cpp -o boot-graphics-driver-test
private=/boot/system/develop/headers/private
g++ -std=gnu++17 -O2 -g -Wall -Wextra -Werror -Wno-unused-parameter \
    -fPIC -fno-exceptions -fno-rtti -D_KERNEL_MODE=1 \
    -I"$private" -I"$private/kernel" -I"$private/drivers" -I"$private/system" \
    -I"$private/kernel/arch/x86" -I"$private/kernel/boot/platform/bios_ia32" \
    -I"$private/system/arch/x86_64" -Isource/build/config_headers \
    -ffreestanding -fno-use-cxa-atexit -fno-tree-vectorize -fno-strict-aliasing \
    -fno-delete-null-pointer-checks -mno-red-zone -fno-omit-frame-pointer \
    -c boot_graphics.cpp -o boot_graphics.o
g++ -shared -nostdlib -Wl,--no-undefined -Wl,-soname,boot_graphics \
    -Wl,-z,max-page-size=0x1000 -o boot_graphics boot_graphics.o \
    -L/boot/system/develop/lib -l:_KERNEL_ \
    /boot/system/develop/lib/haiku_version_glue.o
readelf -d boot_graphics >validation/driver-dynamic.txt
test "$(sed -n 's/.*(NEEDED).*\[\(.*\)\].*/\1/p' validation/driver-dynamic.txt)" = _KERNEL_
nm -D --defined-only boot_graphics >validation/driver-symbols.txt
grep -q ' init_driver$' validation/driver-symbols.txt
grep -q ' publish_devices$' validation/driver-symbols.txt
# The exact QEMU IDs were checked above, before any guest installation.
# Refuse to replace any existing diagnostic, including a dangling symlink.
test ! -e "$diagnostic_bin" && test ! -L "$diagnostic_bin"
test ! -e "$diagnostic_dev" && test ! -L "$diagnostic_dev"
mkdir -p "$(dirname "$diagnostic_bin")" "$(dirname "$diagnostic_dev")"
staging_candidate=/boot/system/non-packaged/add-ons/kernel/drivers/.boot-graphics-$$
mkdir "$staging_candidate"
staging_dir=$staging_candidate
cp boot_graphics "$staging_dir/boot_graphics"
chmod 0644 "$staging_dir/boot_graphics"
installed=1
mv "$staging_dir/boot_graphics" "$diagnostic_bin"
rmdir "$staging_dir"
staging_dir=
ln -s ../../bin/boot_graphics "$diagnostic_dev"
# The legacy driver directory watcher publishes asynchronously (once/second).
for attempt in 1 2 3 4 5 6 7 8 9 10; do
    test -e /dev/misc/boot_graphics && break
    sleep 1
done
test -e /dev/misc/boot_graphics
./boot-graphics-driver-test
./boot-graphics-inventory --boot-info >validation/boot-info.log
cat validation/boot-info.log
readelf -h boot-graphics-inventory >validation/elf-header.txt
readelf -d boot-graphics-inventory >validation/elf-dynamic.txt
sha256sum boot-graphics-inventory boot_graphics >validation/artifact.sha256
tar -czf validation/artifacts.tar.gz boot-graphics-inventory boot_graphics \
    boot-framebuffer-info-test boot-graphics-driver-test
