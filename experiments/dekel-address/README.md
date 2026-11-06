# Experimental Dekel HDMI groundwork

These C++ helpers are deliberately not linked into the accelerant. They do
not access device memory, set power states, acquire Type-C ownership, or
submit GPU work. They are not a working HDMI implementation.

`DekelAddress.h` calculates a banked PHY aperture address and HIP index.
`DekelPllPlan.h` calculates non-spread-spectrum HDMI divider and fractional
feedback values. It does not calculate all required PLL fields, program the
PLL, or select a PHY. The caller must not use these values on hardware without
the missing sequencing, synchronization, mapping, error handling, reference
clock and target validation.

The reference is Linux i915 at commit
`f66469d9905b39d2c260a15d7d6db450f4db67c1`, especially
`drivers/gpu/drm/i915/display/intel_dkl_phy_regs.h` and
`drivers/gpu/drm/i915/display/intel_dpll_mgr.c`; Intel's notices and license
are preserved in `INTEL-NOTICE.txt`.

Run on the development host:

```sh
c++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined test.cpp -o /tmp/dekel-address-test
/tmp/dekel-address-test
c++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined pll-test.cpp -o /tmp/dekel-pll-test
/tmp/dekel-pll-test
```
