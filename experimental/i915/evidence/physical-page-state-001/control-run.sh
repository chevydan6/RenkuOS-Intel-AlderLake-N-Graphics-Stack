#!/bin/bash
set -euo pipefail
cd /boot/home/i915-native-state-001
test ! -e controls-final
mkdir controls-final
cp src/native_page_bridge.cpp controls-final/original.cpp
cp -R build controls-final/positive-build
restore() { cp controls-final/original.cpp src/native_page_bridge.cpp; }
trap restore EXIT
for name in no-dirty no-accessed; do
    mkdir "controls-final/$name"
    cp "mutations/$name.cpp" src/native_page_bridge.cpp
    sha256sum src/native_page_bridge.cpp > "controls-final/$name/source.sha256"
    bash completion/build-native.sh > "controls-final/$name/build.log" 2>&1
    set +e
    bash completion/run-native-tests.sh > "controls-final/$name/runtime.log" 2>&1
    result=$?
    set -e
    printf '%s\n' "$result" > "controls-final/$name/exit.txt"
    test "$result" = 1
    grep -E 'run=0 checks=[0-9]+ failures=[1-9][0-9]* first_line=[1-9]' "controls-final/$name/runtime.log"
    grep 'I915_LINUX_COMPLETION_UNINIT status=0' "controls-final/$name/runtime.log"
    cp -R build "controls-final/$name/build"
    echo "NATIVE_STATE_MUTATION_REJECTED=$name"
done
restore
trap - EXIT
echo NATIVE_STATE_CONTROLS_FINISHED
