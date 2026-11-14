#!/bin/bash
set -euo pipefail
cd /boot/home/i915-shmem-native-001
sha256sum -c payload.sha256 > source-check.log 2>&1
echo SHMEM_INITIAL_HASH_CHECK=0
printf 'source_commit=015f1632c04212a6c6f713c5a458e361edaf72d8 report_version=10 report_bytes=128\n'
bash -x completion/build-native.sh 2>&1 | tee completion-build.log
echo SHMEM_NATIVE_BUILD=0
bash completion/run-native-tests.sh
mkdir completion-build
cp build/* completion-build/ 2>/dev/null || true
bash vma/build-native.sh > vma-build.log 2>&1
bash vma/run-native-tests.sh
mkdir vma-build
cp build/* vma-build/ 2>/dev/null || true
set +e
bash completion/attempt-whole-native.sh
whole=$?
set -e
echo SHMEM_WHOLE_EXIT=$whole
sha256sum -c payload.sha256 > final-source-check.log 2>&1
sha256sum -c baseline-components.sha256 > final-components-check.log 2>&1
ps > final-processes.txt
echo SHMEM_FINAL_HASH_CHECK=0
echo SHMEM_RUN_FINISHED
sync
