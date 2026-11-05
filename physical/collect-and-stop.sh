#!/bin/bash
# SPDX-License-Identifier: MIT
# Private external-USB image only. Read diagnostics, export to its EFI partition,
# sync, then request power-off. No Intel/GART driver installation or GPU writes.
set -u
base=/boot/home/gpu-boot-diagnostic
output=/boot/home/gpu-boot-results
if [ -e "$output" ]; then
    previous=$(mktemp -d /boot/home/gpu-prior-results.XXXXXX) || exit 1
    mv "$output" "$previous/results" || exit 1
fi
mkdir -p "$output"
exec >"$output/startup.log" 2>&1
sleep 10

collect() {
    uname -a >"$output/uname.txt"
    df /boot >"$output/boot-volume.txt"
    df -h >"$output/volumes.txt"
    if ! grep -q 'Device: /dev/disk/usb/0/0/0$' "$output/boot-volume.txt"; then
        echo 'Unexpected boot device; no EFI partition will be mounted.'
        return 1
    fi
    "$base/configure-boot-image" >"$output/automount.txt" || return 1
    if ! grep -q 'hrev60072+518' "$output/uname.txt"; then
        echo 'Unexpected kernel; refusing diagnostic ioctl.'
        return 1
    fi
    "$base/boot-graphics-inventory" --pci >"$output/pci.txt" 2>&1
    pci_status=$?
    echo "$pci_status" >"$output/pci-status.txt"
    if [ "$pci_status" -ne 0 ] && [ "$pci_status" -ne 3 ]; then return 1; fi
    ls /dev/graphics >"$output/graphics-devices.txt"
    graphics=
    if [ -e /dev/graphics/framebuffer ]; then
        graphics=/dev/graphics/framebuffer
    elif [ -e /dev/graphics/vesa ]; then
        graphics=/dev/graphics/vesa
    fi
    if [ -z "$graphics" ]; then return 1; fi
    for attempt in 1 2 3; do
        "$base/boot-graphics-inventory" --boot-info >"$output/boot-info-$attempt.txt" 2>&1 || return 1
        "$base/boot-graphics-inventory" --framebuffer "$graphics" \
            >"$output/display-$attempt.txt" 2>&1 || return 1
    done
    cmp "$output/boot-info-1.txt" "$output/boot-info-2.txt" || return 1
    cmp "$output/boot-info-1.txt" "$output/boot-info-3.txt" || return 1
    cmp "$output/display-1.txt" "$output/display-2.txt" || return 1
    cmp "$output/display-1.txt" "$output/display-3.txt" || return 1
    listdev >"$output/pci-devices.txt" || return 1
    sha256sum "$base/boot-graphics-inventory" \
        /boot/system/non-packaged/add-ons/kernel/drivers/bin/boot_graphics >"$output/binaries.sha256" || return 1
    return 0
}
collect
result=$?
echo "$result" >"$output/collection-status.txt"
timeout -k 5s 140s sh /boot/home/joint-boot-wifi.sh "$output/wifi"
echo "$?" >"$output/wifi-hook-status.txt"
tail -n 1600 /var/log/syslog >"$output/syslog-tail.txt" 2>&1
sync
# Never guess which disk is the external test medium.
if grep -q 'Device: /dev/disk/usb/0/0/0$' "$output/boot-volume.txt"; then
    tar -czf /boot/home/gpu-diagnostic.tgz -C /boot/home gpu-boot-results
    mkdir -p /boot/home/gpu-efi
    if mount -t fat /dev/disk/usb/0/0/1 /boot/home/gpu-efi; then
        cp /boot/home/gpu-diagnostic.tgz /boot/home/gpu-efi/GPU.TGZ
        export_status=$?
        sync
        unmount /boot/home/gpu-efi
        echo "EFI export status: $export_status"
    else
        echo 'EFI export failed; results remain on external BFS volume.'
    fi
fi
echo "GPU diagnostic finished with collection status $result; requesting power-off."
sync
sleep 5
shutdown -s
