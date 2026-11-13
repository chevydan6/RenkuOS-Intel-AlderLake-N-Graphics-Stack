#!/bin/bash
# SPDX-License-Identifier: MIT
# Run visibly from a prepared original completion bundle after its native build. No GPU module replacement.
set -euo pipefail
test "$(uname -s)" = Haiku
test "$(uname -m)" = x86_64
mkdir -p validation
result_dir=$(mktemp -d validation/completion.XXXXXX)
exec > >(tee "$result_dir/run.log") 2>&1
uname -a
sha256sum -c build/binaries.sha256
driver=/boot/system/non-packaged/add-ons/kernel/drivers/bin/i915_linux_completion_test
link=/boot/system/non-packaged/add-ons/kernel/drivers/dev/misc/i915_linux_completion_test
test ! -e "$driver" && test ! -L "$driver"
test ! -e "$link" && test ! -L "$link"
syslog_line=$(wc -l < /var/log/syslog)
ls /dev/graphics > "$result_dir/graphics-before.log"
installed=0
cleanup() {
    status=$?
    trap - EXIT
    if [ "$installed" = 1 ]; then
        if cmp build/i915_linux_completion_test.driver "$driver" && [ "$(readlink "$link")" = ../../bin/i915_linux_completion_test ]; then
            rm "$link" "$driver"
        else
            echo 'Diagnostic identity changed; refusing guessed removal'
            status=1
        fi
    fi
    removed=0
    for attempt in {1..100}; do
        ls /dev/misc > "$result_dir/misc-after.log"
        tail -n +$((syslog_line + 1)) /var/log/syslog > "$result_dir/new-syslog.log"
        grep I915_LINUX_COMPLETION_ "$result_dir/new-syslog.log" > "$result_dir/module-lifecycle.log" || true
        last_event=$(tail -n 1 "$result_dir/module-lifecycle.log")
        if [ ! -e /dev/misc/i915_linux_completion_test ] && [[ "$last_event" == *I915_LINUX_COMPLETION_UNINIT\ status=0 ]]; then
            removed=1
            break
        fi
        sleep 0.1
    done
    test "$removed" = 1 || status=1
    cat "$result_dir/module-lifecycle.log"
    grep -q 'I915_LINUX_COMPLETION_INIT status=0' "$result_dir/module-lifecycle.log" || status=1
    test ! -e "$driver" && test ! -L "$link" || status=1
    ls /dev/graphics > "$result_dir/graphics-after.log"
    cmp "$result_dir/graphics-before.log" "$result_dir/graphics-after.log" || status=1
    echo "$status" > "$result_dir/exit.txt"
    echo "LINUX_COMPLETION_NATIVE_TEST_EXIT=$status results=$result_dir"
    sync
    exit "$status"
}
trap cleanup EXIT
cp build/i915_linux_completion_test.driver "$driver"
ln -s ../../bin/i915_linux_completion_test "$link"
installed=1
sync
published=0
for attempt in {1..100}; do
    ls /dev/misc > /dev/null
    if [ -e /dev/misc/i915_linux_completion_test ]; then published=1; break; fi
    sleep 0.1
done
test "$published" = 1
build/linux_completion_probe
