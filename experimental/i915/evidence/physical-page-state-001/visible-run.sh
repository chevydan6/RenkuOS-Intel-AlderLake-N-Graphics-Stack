#!/bin/bash
set -euo pipefail
cd /boot/home/i915-native-state-001
sha256sum -c final-payload.sha256 > initial-final-source-check.log 2>&1
echo NATIVE_STATE_INITIAL_HASH_CHECK=0
bash -x completion/build-native.sh 2>&1 | tee final-build.log
bash completion/run-native-tests.sh 2>&1 | tee final-runtime.log
mkdir final-completion-build
cp build/* final-completion-build/ 2>/dev/null || true
bash vma/build-native.sh > final-vma-build.log 2>&1
bash vma/run-native-tests.sh 2>&1 | tee final-vma-runtime.log
mkdir final-vma-build
cp build/* final-vma-build/ 2>/dev/null || true
set +e
bash completion/attempt-whole-native.sh > final-whole-attempt.log 2>&1
whole=$?
set -e
echo NATIVE_STATE_WHOLE_EXIT=$whole
sha256sum -c final-payload.sha256 > final-source-check.log 2>&1
sha256sum -c baseline-components.sha256 > final-components-check.log 2>&1
ps > final-processes.txt
echo NATIVE_STATE_FINAL_HASH_CHECK=0
echo NATIVE_STATE_RUN_FINISHED
sync
