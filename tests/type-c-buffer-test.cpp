/* SPDX-License-Identifier: MIT */
#include "TypeCHdmiBuffer.h"
#include <stdio.h>

struct Mock { uint32 control; bool autoActive, autoIdle; unsigned writes, failWrite, pauses; };
static status_t Read(void* p, uint32 offset, uint32* value) {
	if (offset != 0x64400) return B_BAD_VALUE;
	*value = ((Mock*)p)->control; return B_OK;
}
static status_t Write(void* p, uint32 offset, uint32 value) {
	Mock& m = *(Mock*)p;
	if (offset != 0x64400) return B_BAD_VALUE;
	m.writes++;
	if (m.failWrite && m.writes == m.failWrite) return B_ERROR;
	m.control = (value & ~(1U << 7)) | (m.control & (1U << 7));
	if ((value & (1U << 31)) && m.autoActive) m.control &= ~(1U << 7);
	if (!(value & (1U << 31)) && m.autoIdle) m.control |= 1U << 7;
	return B_OK;
}
static void Pause(void* p) { ((Mock*)p)->pauses++; }
static AlderLakeTypeCIO IO(Mock& m) { AlderLakeTypeCIO io = {&m, Read, Write, Pause}; return io; }
int main() {
	unsigned errors = 0;
	const uint32 original = 0x10a100ceU;
	{
		Mock m = {original, true, true, 0, 0, 0}; TypeCHdmiBuffer b(IO(m), 2, 3);
		if (b.Enable(false) != B_OK || !(m.control & 0x80000000U)
			|| (m.control & 0x00f1000eU) != 0 || b.Enable(false) != B_BUSY
			|| b.Release() != B_OK || m.control != original) errors++;
	}
	{
		Mock m = {original, true, true, 0, 0, 0}; TypeCHdmiBuffer b(IO(m), 2, 3);
		if (b.Enable(true) != B_OK || !(m.control & (1U << 16))
			|| b.Release() != B_OK || m.control != original) errors++;
	}
	{
		Mock m = {original & ~(1U << 6), true, true, 0, 0, 0}; TypeCHdmiBuffer b(IO(m), 2, 3);
		if (b.Enable(false) != B_ERROR || m.writes != 0) errors++;
	}
	{
		Mock m = {original | 0x80000000U, true, true, 0, 0, 0}; TypeCHdmiBuffer b(IO(m), 2, 3);
		if (b.Enable(false) != B_BUSY || m.writes != 0) errors++;
	}
	{
		Mock m = {original, false, true, 0, 0, 0}; TypeCHdmiBuffer b(IO(m), 2, 3);
		if (b.Enable(false) != B_TIMED_OUT || m.control != original || m.pauses != 3) errors++;
	}
	{
		Mock m = {original, true, false, 0, 0, 0}; TypeCHdmiBuffer b(IO(m), 2, 3);
		if (b.Enable(false) != B_OK || b.Release() != B_TIMED_OUT
			|| b.Enable(false) != B_BUSY) errors++;
		m.autoIdle = true;
		if (b.Release() != B_OK || m.control != original) errors++;
	}
	{
		Mock m = {original, true, true, 0, 1, 0}; TypeCHdmiBuffer b(IO(m), 2, 3);
		if (b.Enable(false) != B_ERROR || m.control != original) errors++;
	}
	{
		Mock m = {0xffffffffU, true, true, 0, 0, 0}; TypeCHdmiBuffer b(IO(m), 2, 3);
		if (b.Enable(false) != B_ERROR || m.writes != 0) errors++;
	}
	if (errors) { fprintf(stderr, "%u HDMI buffer cases failed\n", errors); return 1; }
	puts("8 HDMI buffer ownership, timing and rollback cases passed; mock registers only");
	return 0;
}
