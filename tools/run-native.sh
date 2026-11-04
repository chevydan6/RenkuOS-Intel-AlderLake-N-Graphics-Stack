#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
# Native component tests without requiring Python in the guest.
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
make -j1 all build/intel_extreme.control.accelerant
build/gtt-range-test
build/gtt-pte-test
build/gtt-stolen-test
build/hooks-test ./build/intel_extreme.accelerant
set +e
build/hooks-test ./build/intel_extreme.control.accelerant >validation/control-hooks.log 2>&1
control_status=$?
set -e
test "$control_status" -eq 1
awk '/^FAIL / { count++; if ($0 !~ /^FAIL drawing capability/) bad=1 }
     END { exit !(count == 21 && !bad) }' validation/control-hooks.log
echo 'PASS unpatched hook control: exactly 21 expected failures'
tar -czf validation/artifacts.tar.gz build/intel_gart build/intel_extreme.accelerant \
    build/hooks-test build/gtt-range-test build/gtt-pte-test build/gtt-stolen-test
