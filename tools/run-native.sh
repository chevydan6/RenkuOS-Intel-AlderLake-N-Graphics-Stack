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
make -j1 all build/intel_extreme.control.accelerant build/intel_extreme.lifecycle-control.accelerant build/gating-control build/gart-range-control
build/gtt-range-test
build/gtt-pte-test
build/gtt-stolen-test
build/hooks-test ./build/intel_extreme.accelerant
build/lifecycle-test ./build/intel_extreme.lifecycle.accelerant
build/gating-test
build/gart-range-test
set +e
build/gart-range-control >validation/control-gart-range.log 2>&1
control_status=$?
set -e
test "$control_status" -eq 1
awk '/^FAIL / {
    count++;
    if ($0 == "FAIL unbind adjusted page count") pages++;
    if ($0 == "FAIL unbind preserves reserved and adjacent mappings") guards++;
} END { exit !(count == 6 && pages == 3 && guards == 3) }' validation/control-gart-range.log
echo 'PASS original GART range control: exactly 6 expected failures'
set +e
build/gating-control >validation/control-gating.log 2>&1
control_status=$?
set -e
test "$control_status" -eq 1
awk '/^FAIL / {
    count++;
    if ($0 == "FAIL Alder Lake gating unsupported") unsupported++;
    if ($0 == "FAIL Alder Lake gating registers preserved") preserved++;
} END { exit !(count == 2 && unsupported == 1 && preserved == 1) }' validation/control-gating.log
echo 'PASS original gating control: exactly 2 expected failures'
set +e
build/hooks-test ./build/intel_extreme.control.accelerant >validation/control-hooks.log 2>&1
control_status=$?
set -e
test "$control_status" -eq 1
awk '/^FAIL / { count++; if ($0 !~ /^FAIL drawing capability/) bad=1 }
     END { exit !(count == 21 && !bad) }' validation/control-hooks.log
echo 'PASS unpatched hook control: exactly 21 expected failures'
set +e
build/lifecycle-test ./build/intel_extreme.lifecycle-control.accelerant >validation/control-lifecycle.log 2>&1
control_status=$?
set -e
test "$control_status" -eq 1
awk '/^FAIL / {
    count++;
    if ($0 == "FAIL Alder Lake engine count") engines++;
    if ($0 == "FAIL Alder Lake ring state") rings++;
    if ($0 == "FAIL Alder Lake register writes") writes++;
} END { exit !(count == 3 && engines == 1 && rings == 1 && writes == 1) }' validation/control-lifecycle.log
echo 'PASS original ring control: exactly 3 expected failures'
tar -czf validation/artifacts.tar.gz build/intel_gart build/agp_gart build/intel_extreme.driver build/intel_extreme.accelerant \
    build/hooks-test build/gtt-range-test build/gtt-pte-test build/gtt-stolen-test \
    build/lifecycle-test build/intel_extreme.lifecycle.accelerant build/gating-test build/gart-range-test
