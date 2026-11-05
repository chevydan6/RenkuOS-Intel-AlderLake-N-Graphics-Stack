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
make -j1 all build/intel_extreme.control.accelerant build/intel_extreme.lifecycle-control.accelerant build/gating-control build/gart-range-control build/gart-lifecycle-control
make -j1 build/gart-allocation-control build/kernel-resources-control
build/kernel-resources-test
set +e
build/kernel-resources-control >validation/control-kernel-resources.log 2>&1
control_status=$?
set -e
test "$control_status" -eq 1
awk '/^FAIL / { count++; failures[$0]++ }
END { exit !(count == 56 &&
    failures["FAIL Alder Lake requests only cursor storage"] == 20 &&
    failures["FAIL Alder Lake ring remains empty"] == 16 &&
    failures["FAIL Alder Lake overlay remains absent"] == 16 &&
    failures["FAIL Alder Lake cursor remains independent"] == 4) }
' validation/control-kernel-resources.log
echo 'PASS original kernel resource control: exactly 56 expected failures'
build/gart-allocation-test
set +e
build/gart-allocation-control >validation/control-gart-allocation.log 2>&1
control_status=$?
set -e
test "$control_status" -eq 1
cat >validation/expected-gart-allocation.log <<'EXPECTED'
FAIL failure clears page pointer
FAIL failure releases pointer table
FAIL cleanup releases pointer table
66 GART allocation checks, 3 failures; mock VM allocation
EXPECTED
cmp validation/expected-gart-allocation.log validation/control-gart-allocation.log
echo 'PASS original GART allocation control: exactly 3 expected failures'
build/gtt-range-test
build/gtt-pte-test
build/gtt-stolen-test
build/hooks-test ./build/intel_extreme.accelerant
build/lifecycle-test ./build/intel_extreme.lifecycle.accelerant
build/gating-test
build/gart-range-test
build/gart-lifecycle-test
set +e
build/gart-lifecycle-control >validation/control-gart-lifecycle.log 2>&1
control_status=$?
set -e
test "$control_status" -eq 1
awk '/^FAIL / { count++; failures[$0]++ }
END {
    exit !(count == 39 &&
        failures["FAIL no free with owned PTEs installed"] == 14 &&
        failures["FAIL completed cleanup removes every owned PTE"] == 8 &&
        failures["FAIL cleanup preserves owned backing on failure"] == 6 &&
        failures["FAIL cleanup preserves allocation metadata"] == 6 &&
        failures["FAIL failed borrowed bind clears installed prefix"] == 1 &&
        failures["FAIL failed borrowed bind removes new metadata after cleanup"] == 1 &&
        failures["FAIL public reserved rebind rejected"] == 1 &&
        failures["FAIL rejected public rebind preserves output handle"] == 1 &&
        failures["FAIL rejected public rebind preserves existing mapping"] == 1)
}' validation/control-gart-lifecycle.log
echo 'PASS original GART lifecycle control: exactly 39 expected failures'
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
    build/lifecycle-test build/intel_extreme.lifecycle.accelerant build/gating-test build/gart-range-test build/gart-lifecycle-test build/gart-allocation-test build/kernel-resources-test
