#!/bin/bash
# SPDX-License-Identifier: MIT
set -eu
cd /boot/home/i915-store-001
sha256sum -c payload.sha256 > source-check.log 2>&1
echo PAGE_STORE_SOURCE_CHECK=$?
bash completion/build-native.sh > completion-build.log 2>&1
echo PAGE_STORE_NATIVE_BUILD=$?
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
echo PAGE_STORE_WHOLE_EXIT=$status
sha256sum -c payload.sha256 > final-source-check.log 2>&1
echo PAGE_STORE_FINAL_SOURCE_CHECK=$?
echo PAGE_STORE_RUN_FINISHED
sync
