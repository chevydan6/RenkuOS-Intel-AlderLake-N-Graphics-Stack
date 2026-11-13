<!-- SPDX-License-Identifier: MIT -->
The complete unchanged DRM VMA offset manager, drm_mm and rbtree pass ASan/UBSan
with real pthread reader/writer locks. Tests cover offset boundaries/exhaustion,
access reference counts and allow-once revocation, OOM for new/existing records,
original Linux log2 helpers and 12,000 lock operations with two overlapping readers.
The heap backend is modeled here; native IRQ and deferred-free services are tested
separately in physical-vma-001. Changing the initial access count from 1 to 2 must
fail, demonstrating detection of an access record that survives revocation.
