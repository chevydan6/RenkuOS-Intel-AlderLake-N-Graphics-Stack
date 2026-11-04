#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
# Run on Fedora inside coord.py's haiku-vm + build resource transaction.
set -eu
if [ "$#" -ne 5 ]; then
    echo 'usage: guest-component-test.sh ARCHIVE RUN_NAME SSH_CONFIG GUEST OUTPUT' >&2
    exit 2
fi
archive=$1
run_name=$2
ssh_config=$3
guest=$4
output=$5
[[ "$run_name" =~ ^[a-zA-Z0-9][a-zA-Z0-9_-]*$ ]] || exit 2
mkdir -p "$output"
ssh_options=(-F "$ssh_config" -o BatchMode=yes -o ConnectTimeout=10)
ssh "${ssh_options[@]}" "$guest" 'command -v timeout; command -v python3; command -v make'
scp "${ssh_options[@]}" "$archive" "$guest:/boot/home/renkuos-gpu-$run_name.tar.gz"
set +e
ssh "${ssh_options[@]}" "$guest" "set -e; mkdir -p /boot/home/renkuos-gpu-tests; cd /boot/home/renkuos-gpu-tests; test ! -e '$run_name'; tar -xzf '/boot/home/renkuos-gpu-$run_name.tar.gz'; cd '$run_name'; timeout --signal=TERM --kill-after=5s 240s python3 tools/run-native.py ." > "$output/native-run.log" 2>&1
result=$?
# The guest's own timeout ends build descendants before the host lock deadline.
scp -r "${ssh_options[@]}" "$guest:/boot/home/renkuos-gpu-tests/$run_name/validation" "$output/"
copy_result=$?
set -e
cat "$output/native-run.log"
if [ "$result" -ne 0 ]; then exit "$result"; fi
exit "$copy_result"
