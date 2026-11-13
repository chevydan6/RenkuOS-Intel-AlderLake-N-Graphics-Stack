#!/bin/bash
set -eu
cd /boot/home/i915-folio-001
sha256sum -c payload.sha256 > source-check.log 2>&1
echo FOLIO_SOURCE_CHECK=$?
bash completion/build-native.sh > completion-build.log 2>&1
echo FOLIO_NATIVE_BUILD=$?
tail -n 4 completion-build.log
bash completion/run-native-tests.sh
mkdir completion-build
cp build/* completion-build/ 2>/dev/null || true
bash vma/build-native.sh > vma-build.log 2>&1
bash vma/run-native-tests.sh
mkdir vma-build
cp build/* vma-build/ 2>/dev/null || true
set +e
bash completion/attempt-whole-native.sh
status=$?
set -e
echo FOLIO_WHOLE_EXIT=$status
sha256sum -c payload.sha256 > final-source-check.log 2>&1
echo FOLIO_FINAL_SOURCE_CHECK=$?
echo FOLIO_RUN_FINISHED
sync
