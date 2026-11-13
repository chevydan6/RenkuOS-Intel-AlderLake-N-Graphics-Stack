# Driver implementation map

This directory is a navigation index only. **No production driver source has been moved.**

## Native RenkuOS driver
The numbered patch series is in [../patches](../patches/); native implementation candidates, test harnesses and headers are in [../workstreams/gpu-next](../workstreams/gpu-next/). The installed/known-working display build 057 and the original RenkuOS source checkout are separate from this repository. See [current evidence](../docs/CURRENT_STATUS.md).

## Experimental Linux i915 port
The experimental implementation is on branch [experiment/i915-linuxkpi](https://github.com/chevydan6/haiku-intel-gpu/tree/experiment/i915-linuxkpi), not merged into the native branch. Its PORT_PROGRESS and PORT_BLOCKERS documents describe the incomplete compatibility layer. Do not mix private ring/request layouts with complete Linux i915 objects.

## Migration gate
Do not relocate patch or source paths until the path reference map, patch application state, and native build/VM regression are verified. No accelerated GPU command execution is established.
