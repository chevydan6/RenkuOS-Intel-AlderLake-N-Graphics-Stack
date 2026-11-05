#!/bin/bash
# SPDX-License-Identifier: MIT
# Run inside a disposable USB/UEFI VM, before preparing a native observation.
set -eu
test "$(uname -s)" = Haiku
test "$(uname -m)" = x86_64
uname -a | grep -q 'hrev60072+518'
target=/boot/home/gpu-boot-diagnostic
test ! -e "$target"
mkdir "$target"
tar -xzf /boot/home/gpu-boot-kit.tar.gz -C "$target" --strip-components=1
cd "$target"
sha256sum -c SHA256SUMS
chmod 0755 boot-graphics-inventory
set +e
./boot-graphics-inventory --pci >preparation-pci.txt
status=$?
set -e
test "$status" -eq 3
cat preparation-pci.txt
grep -qx 'bridge_id=12378086' preparation-pci.txt
grep -qx 'gpu_id=11111234' preparation-pci.txt
# Reuse the previously tested automount reporter; do not print startup contents.
tool=$(find /boot/home -name configure-boot-image -type f | head -1)
test -n "$tool"
cp "$tool" configure-boot-image
chmod 0755 configure-boot-image
./configure-boot-image
bin=/boot/system/non-packaged/add-ons/kernel/drivers/bin/boot_graphics
dev=/boot/system/non-packaged/add-ons/kernel/drivers/dev/misc/boot_graphics
test ! -e "$bin" && test ! -L "$bin"
test ! -e "$dev" && test ! -L "$dev"
mkdir -p "$(dirname "$bin")" "$(dirname "$dev")"
cp boot_graphics "$bin"
chmod 0644 "$bin"
ln -s ../../bin/boot_graphics "$dev"
for attempt in 1 2 3 4 5 6 7 8 9 10; do
    test -e /dev/misc/boot_graphics && break
    sleep 1
done
./boot-graphics-inventory --boot-info
cp /boot/home/gpu-collect-and-stop.sh collect-and-stop.sh
chmod 0755 collect-and-stop.sh
test -r /boot/home/power-preflight/join-wifi.sh
test -r /boot/home/power-preflight/wifi.private
sh -n /boot/home/joint-boot-wifi.sh
sh -n /boot/home/wifi-capture.sh
command -v timeout >/dev/null
startup=/boot/home/config/settings/boot/UserBootscript
if [ -e "$startup" ]; then cp "$startup" "$startup.pre-gpu"; fi
cat >"$startup" <<'STARTUP'
#!/bin/sh
/bin/bash /boot/home/gpu-boot-diagnostic/collect-and-stop.sh &
STARTUP
chmod 0755 "$startup"
sync
echo 'GPU USB image staged; next boot captures metadata and powers off.'
