#!/bin/bash
# SPDX-License-Identifier: MIT
# Only a new sparse regular file in this GPU-owned test directory is formatted.
set -euo pipefail
umask 077
out=/home/dan/renkuos-development/testing/renku-btrfs-rehearsal-001
disk="$out/scratch-btrfs.img"
mountdir="$out/mnt"
test ! -e "$out"
mkdir -m 700 "$out"
loop=
mounted=0
cleanup() {
    if [ "$mounted" = 1 ]; then umount "$mountdir" || true; fi
    if [ -n "$loop" ]; then
        back=$(losetup -n -O BACK-FILE "$loop" 2>/dev/null || true)
        if [ "$back" = "$disk" ]; then losetup -d "$loop" || true; fi
    fi
}
trap cleanup EXIT
test "$(cat /sys/class/power_supply/BAT0/capacity)" -ge 30
python3 - "$disk" <<'PY'
import os,sys
fd=os.open(sys.argv[1],os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW,0o600)
os.ftruncate(fd,12*1024**3);os.close(fd)
PY
test -f "$disk" && test ! -L "$disk"
mkfs.btrfs --force --nodiscard --label RENKU_SCRATCH "$disk" > "$out/mkfs.log"
loop=$(losetup --find --show -- "$disk")
case "$loop" in /dev/loop[0-9]*) ;; *) echo 'Unexpected loop device' >&2; exit 1;; esac
test "$(losetup -n -O BACK-FILE "$loop")" = "$disk"
mkdir "$mountdir"
mount -t btrfs -o noatime,compress=no "$loop" "$mountdir"
mounted=1
test "$(findmnt -no SOURCE "$mountdir")" = "$loop"
dd if=/dev/urandom of="$mountdir/test-data.bin" bs=1M count=128 conv=fsync status=none
sha256sum "$mountdir/test-data.bin" | cut -d' ' -f1 > "$out/content-before.sha256"
btrfs filesystem usage -b "$mountdir" > "$out/usage-before.txt"
before=$(awk '/Device size:/ {print $3;exit}' "$out/usage-before.txt")
btrfs filesystem resize -8G "$mountdir" > "$out/resize.log"
btrfs filesystem usage -b "$mountdir" > "$out/usage-after.txt"
after=$(awk '/Device size:/ {print $3;exit}' "$out/usage-after.txt")
test "$((before-after))" = 8589934592
sha256sum "$mountdir/test-data.bin" | cut -d' ' -f1 > "$out/content-after.sha256"
cmp "$out/content-before.sha256" "$out/content-after.sha256"
sync
umount "$mountdir"
mounted=0
test "$(losetup -n -O BACK-FILE "$loop")" = "$disk"
losetup -d "$loop"
loop=
btrfs check --readonly "$disk" > "$out/check.log" 2>&1
printf 'scratch_btrfs_shrink_bytes=8589934592\ncontent_hash_preserved=true\nreadonly_check_passed=true\nlaptop_filesystem_resized=false\n' > "$out/result.txt"
unlink "$disk"
chown -R dan:dan "$out"
echo 'Disposable Btrfs shrank by 8 GiB, retained test data, and passed a read-only check.'
