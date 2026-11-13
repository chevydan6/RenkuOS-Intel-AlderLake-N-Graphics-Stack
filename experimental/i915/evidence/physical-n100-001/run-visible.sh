#!/bin/bash
set -eu
cd /boot/home/i915-linuxkpi-physical-001/i915-linuxkpi-native
printf 'Actual N100 native build and CPU kernel-service tests\n'
uname -a
echo 'gpu_submission=false'
env SHELLOPTS=xtrace bash vm/run-native-tests.sh &
runner=$!
for attempt in {1..100}; do
    test -f validation/native-test.log && break
    kill -0 "$runner" 2>/dev/null || break
    sleep 0.1
done
tail -n +1 -f validation/native-test.log &
follower=$!
set +e
wait "$runner"
result=$?
kill "$follower" 2>/dev/null
wait "$follower" 2>/dev/null
set -e
for log in validation/probe.log validation/concurrent-a.log validation/concurrent-b.log; do
    if [ -f "$log" ]; then echo "$log"; cat "$log"; fi
done
echo "Physical native CPU test exit=$result"
sync
exit "$result"
