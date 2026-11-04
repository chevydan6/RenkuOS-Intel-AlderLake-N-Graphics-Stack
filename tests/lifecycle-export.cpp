// SPDX-License-Identifier: MIT
// Test-only export; never linked into the production accelerant.
#include "accelerant.h"

extern "C" void
test_ring_lifecycle(ring_buffer* ring)
{
	setup_ring_buffer(*ring, "GPU lifecycle regression test");
	uninit_ring_buffer(*ring);
}
