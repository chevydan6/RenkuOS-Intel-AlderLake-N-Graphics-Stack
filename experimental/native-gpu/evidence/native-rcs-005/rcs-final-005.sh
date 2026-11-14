#!/bin/bash
set -euo pipefail
cd /boot/home/native-rcs-owner-005
build/gpu_render_sample probe > results/final-probe.log
build/gpu_render_sample bootstrap > results/final-bootstrap.log
for variant in 0 1 2 3; do build/gpu_render_sample draw "$variant"; done > results/final-four-draws.log
build/buffer_copy_sample > results/final-bcs-before-window.log
nohup build/render_window --frames 1800 > results/final-window.log 2>&1 < /dev/null &
echo FINAL_BOUNDED_WINDOW_STARTED
