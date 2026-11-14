#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
# Ubuntu 24.04: extract compiler-only dependencies without root or host installs.
set -euo pipefail
workspace=$(realpath -m "${1:?usage: bootstrap-tools.sh NEW_TOOLS_DIRECTORY}")
if [ -e "$workspace" ]; then echo 'Tools directory must not exist.' >&2; exit 1; fi
mkdir -p "$workspace/debs" "$workspace/sysroot" "$workspace/python"
python3 -m venv --without-pip "$workspace/python"
curl -fsSL https://bootstrap.pypa.io/get-pip.py -o "$workspace/get-pip.py"
"$workspace/python/bin/python" "$workspace/get-pip.py" pip==26.3
"$workspace/python/bin/python" -m pip install meson==1.12.1 ninja==1.13.2 mako==1.4.3 MarkupSafe==3.0.4 ply==3.11 packaging==26.3 pyyaml==6.0.3
(cd "$workspace/debs" && apt-get download pkgconf-bin libpkgconf3 zlib1g-dev zlib1g libdrm-dev libdrm2 libpciaccess-dev libpciaccess0)
for package in "$workspace"/debs/*.deb; do dpkg-deb -x "$package" "$workspace/sysroot"; done
ln -s pkgconf "$workspace/sysroot/usr/bin/pkg-config"
# Shell-escaped paths; source this environment for Meson and Ninja.
{
    printf 'export PATH=%q:%q:$PATH\n' "$workspace/python/bin" "$workspace/sysroot/usr/bin"
    printf 'export LD_LIBRARY_PATH=%q\n' "$workspace/sysroot/usr/lib/x86_64-linux-gnu"
    printf 'export PKG_CONFIG_SYSROOT_DIR=%q\n' "$workspace/sysroot"
    printf 'export PKG_CONFIG_LIBDIR=%q\n' "$workspace/sysroot/usr/lib/x86_64-linux-gnu/pkgconfig"
} > "$workspace/env.sh"
(cd "$workspace/debs" && sha256sum ./*.deb > SHA256SUMS)
echo "Run: source $workspace/env.sh"
