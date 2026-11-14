#!/bin/bash
set -euo pipefail
cd /boot/home/native-rcs-owner-004
mkdir -p results
build/gpu_render_sample probe > results/probe.log
build/gpu_render_sample bootstrap > results/bootstrap.log
build/gpu_render_sample draw 0 > results/draw-before-boundary.log
build/buffer_copy_sample > results/bcs-before-boundary.log
build/gpu_lifecycle quiesce > results/quiesce.log
if build/gpu_render_sample draw 1 > results/draw-suspended.log; then exit 21; fi
build/gpu_lifecycle resume > results/resume.log
if build/gpu_render_sample draw 1 > results/draw-needs-bootstrap.log; then exit 22; fi
build/gpu_render_sample bootstrap > results/rebootstrap.log
build/gpu_render_sample draw 2 > results/draw-after-boundary.log
build/buffer_copy_sample > results/bcs-after-boundary.log
build/gpu_lifecycle status > results/status-boundary.log
cat results/quiesce.log results/resume.log results/draw-needs-bootstrap.log results/status-boundary.log
echo RCS_BOUNDARIES_PASSED
