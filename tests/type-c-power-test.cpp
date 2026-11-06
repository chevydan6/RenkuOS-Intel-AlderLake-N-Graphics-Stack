/* SPDX-License-Identifier: MIT */
#include "TypeCPowerLifecycle.h"
#include <stdio.h>

struct Mock {
	bool power, owned, ready;
	int fail, releases, relinquishes, acquisitions;
};

static status_t Acquire(void* p, bool* held) {
	Mock& m = *(Mock*)p;
	if (m.fail == 1) { *held = false; return B_ERROR; }
	if (m.fail == 8) { *held = false; return B_OK; }
	m.power = true; m.acquisitions++; *held = true;
	return m.fail == 7 ? B_ERROR : B_OK;
}
static status_t Release(void* p) {
	Mock& m = *(Mock*)p;
	if (m.fail == 5) return B_ERROR;
	m.power = false; m.releases++; return B_OK;
}
static status_t IsOwned(void* p, bool* out) {
	Mock& m = *(Mock*)p;
	if (m.fail == 2) return B_ERROR;
	*out = m.owned; return B_OK;
}
static status_t SetOwned(void* p, bool value) {
	Mock& m = *(Mock*)p;
	if ((value && m.fail == 3) || (!value && m.fail == 6)) return B_ERROR;
	m.owned = value; if (!value) m.relinquishes++; return B_OK;
}
static status_t IsReady(void* p, bool* out) {
	Mock& m = *(Mock*)p;
	if (m.fail == 4) return B_ERROR;
	*out = m.ready; return B_OK;
}
static TypeCPowerOps Ops(Mock& m) {
	TypeCPowerOps ops = {&m, Acquire, Release, IsOwned, SetOwned, IsReady};
	return ops;
}
int main() {
	unsigned errors = 0;
	for (int preowned = 0; preowned <= 1; preowned++) {
		Mock m = {false, preowned != 0, true, 0, 0, 0, 0};
		TypeCPowerLifecycle p(Ops(m));
		if (p.Acquire() != B_OK || p.Acquire() != B_BUSY || !m.power
			|| !m.owned || p.Release() != B_OK || m.power
			|| m.owned != (preowned != 0) || m.relinquishes != !preowned
			|| p.Release() != B_OK || m.releases != 1)
			errors++;
	}
	for (int fail = 1; fail <= 4; fail++) {
		Mock m = {false, false, true, fail, 0, 0, 0};
		TypeCPowerLifecycle p(Ops(m));
		if (p.Acquire() == B_OK || m.power || m.owned
			|| m.releases != (fail != 1) || m.relinquishes != (fail == 4))
			errors++;
	}
	{
		Mock m = {false, false, false, 0, 0, 0, 0};
		TypeCPowerLifecycle p(Ops(m));
		if (p.Acquire() == B_OK || m.power || m.owned
			|| m.releases != 1 || m.relinquishes != 1) errors++;
	}
	for (int fail = 5; fail <= 6; fail++) {
		Mock m = {false, false, true, 0, 0, 0, 0};
		TypeCPowerLifecycle p(Ops(m));
		if (p.Acquire() != B_OK) errors++;
		m.fail = fail;
		if (p.Release() == B_OK || !m.power || p.Acquire() != B_BUSY) errors++;
		m.fail = 0;
		if (p.Release() != B_OK || m.power || m.owned || m.releases != 1)
			errors++;
	}
	{
		Mock m = {false, false, true, 0, 0, 0, 0};
		TypeCPowerOps ops = Ops(m); ops.isReady = NULL;
		TypeCPowerLifecycle p(ops);
		if (p.Acquire() != B_BAD_VALUE || m.power || m.acquisitions != 0)
			errors++;
	}
	{
		Mock m = {false, false, false, 6, 0, 0, 0};
		TypeCPowerLifecycle p(Ops(m));
		if (p.Acquire() == B_OK || !m.power || !m.owned || p.Acquire() != B_BUSY)
			errors++;
		m.fail = 0;
		if (p.Release() != B_OK || m.power || m.owned) errors++;
	}
	{
		Mock m = {false, false, true, 7, 0, 0, 0};
		TypeCPowerLifecycle p(Ops(m));
		if (p.Acquire() == B_OK || !m.power || p.Acquire() != B_BUSY) errors++;
		m.fail = 0;
		if (p.Release() != B_OK || m.power || m.releases != 1) errors++;
	}
	{
		Mock m = {false, false, true, 8, 0, 0, 0};
		TypeCPowerLifecycle p(Ops(m));
		if (p.Acquire() != B_ERROR || m.power || m.releases != 0) errors++;
	}
	if (errors) { fprintf(stderr, "%u lifecycle cases failed\n", errors); return 1; }
	puts("13 Type-C power acquisition, rollback and retry cases passed");
	return 0;
}
