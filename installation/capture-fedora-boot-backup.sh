#!/bin/bash
# SPDX-License-Identifier: MIT
# Root, under the shared host-config lock. Reads existing disk/boot data only.
set -eu
umask 077
out=/home/dan/renkuos-development/testing/renku-install-disk-backup-001
test ! -e "$out"
mkdir -m 700 "$out"
test "$(findmnt -no SOURCE /boot/efi)" = /dev/mmcblk0p1
test "$(findmnt -no SOURCE /boot)" = /dev/mmcblk0p2
test "$(findmnt -no SOURCE / | cut -d'[' -f1)" = /dev/mmcblk0p3
sfdisk --dump /dev/mmcblk0 > "$out/partition-table.sfdisk"
sfdisk --json /dev/mmcblk0 > "$out/partition-table.json"
sectors=$(blockdev --getsz /dev/mmcblk0)
test "$sectors" -gt 100000
dd if=/dev/mmcblk0 of="$out/gpt-primary.bin" bs=512 count=34 status=none
dd if=/dev/mmcblk0 of="$out/gpt-secondary.bin" bs=512 skip="$((sectors-33))" count=33 status=none
if command -v sgdisk >/dev/null 2>&1; then sgdisk --backup="$out/gpt-sgdisk.bin" /dev/mmcblk0; fi
btrfs filesystem usage -b / > "$out/btrfs-usage.txt"
efibootmgr -v > "$out/uefi-entries.txt"
tar --numeric-owner --acls --xattrs -C /boot/efi -cf "$out/efi.tar" .
tar --numeric-owner --acls --xattrs -C /boot --exclude='./efi' -cf "$out/boot.tar" .
tar --numeric-owner --acls --xattrs -C / -cf "$out/grub-config.tar" etc/default/grub etc/grub.d etc/fstab
(cd "$out" && sha256sum partition-table.sfdisk partition-table.json gpt-primary.bin gpt-secondary.bin btrfs-usage.txt uefi-entries.txt efi.tar boot.tar grub-config.tar > SHA256SUMS)
chown -R dan:dan "$out"
echo 'Partition metadata, GPT headers, EFI, /boot, GRUB configuration and fstab captured. This is not a complete data backup.'
