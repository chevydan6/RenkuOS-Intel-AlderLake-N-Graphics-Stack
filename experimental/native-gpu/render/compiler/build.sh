#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
set -euo pipefail
if [ "$#" != 2 ]; then
    echo 'usage: build.sh PRISTINE_MESA_ARCHIVE NEW_BUILD_WORKSPACE' >&2
    exit 2
fi
archive=$(realpath "$1")
workspace=$(realpath -m "$2")
here=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
if [ -e "$workspace" ]; then
    echo 'Build workspace must not exist (protects existing Mesa trees).' >&2
    exit 1
fi
printf '%s  %s\n' b320f65874fd9653ac6c0bd1616605387344e1247411a50c797b5f3fb9dc0b55 "$archive" | sha256sum -c -
mkdir -p "$workspace"
tar -xf "$archive" -C "$workspace"
python3 "$here/integrate.py" "$workspace/mesa-26.1.8"
meson setup "$workspace/build" "$workspace/mesa-26.1.8" \
    -Dgallium-drivers= -Dvulkan-drivers= -Dplatforms= -Dglx=disabled \
    -Degl=disabled -Dgbm=disabled -Dopengl=false -Dgles1=disabled \
    -Dgles2=disabled -Dllvm=disabled -Dintel-elk=false -Dbuild-tests=false \
    -Dexpat=disabled -Dzstd=disabled -Dzlib=enabled -Dshader-cache=disabled
ninja -C "$workspace/build" -j "${JOBS:-4}" src/intel/compiler/brw/native-rcs-shaders
compiler="$workspace/build/src/intel/compiler/brw/native-rcs-shaders"
# Clean process environment prevents Intel compiler debug/tuning overrides.
env -i PATH="$PATH" LD_LIBRARY_PATH="${LD_LIBRARY_PATH:-}" "$compiler" "$workspace/artifacts"
python3 "$here/check_artifacts.py" "$workspace/artifacts"
env -i PATH="$PATH" LD_LIBRARY_PATH="${LD_LIBRARY_PATH:-}" "$compiler" "$workspace/artifacts-repeat"
for artifact in vs.bin fs.bin vs.nir fs.nir metadata.json; do
    cmp "$workspace/artifacts/$artifact" "$workspace/artifacts-repeat/$artifact"
done
(cd "$workspace/artifacts" && sha256sum vs.bin fs.bin vs.nir fs.nir metadata.json > SHA256SUMS)
echo "Verified artifacts: $workspace/artifacts"
