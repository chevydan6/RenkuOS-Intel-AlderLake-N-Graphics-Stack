/* SPDX-License-Identifier: MIT */
#include <native_cpumask.h>
#include <linux/errno.h>
_Static_assert(NR_CPUS == 64 && BITS_PER_LONG == 64, "native x86-64 mask capacity");
unsigned int nr_cpu_ids;
struct cpumask __cpu_possible_mask, __cpu_present_mask, __cpu_online_mask;
struct cpumask __cpu_active_mask, __cpu_dying_mask;
atomic_t __num_online_cpus = ATOMIC_INIT(0);
const DECLARE_BITMAP(cpu_all_bits, NR_CPUS) = CPU_BITS_ALL;
/* Original get_cpu_mask() indexes this constant one-bit lookup table. */
const unsigned long cpu_bit_bitmap[BITS_PER_LONG+1][BITS_TO_LONGS(NR_CPUS)] = {
	[1] = {1UL << 0},
	[2] = {1UL << 1},
	[3] = {1UL << 2},
	[4] = {1UL << 3},
	[5] = {1UL << 4},
	[6] = {1UL << 5},
	[7] = {1UL << 6},
	[8] = {1UL << 7},
	[9] = {1UL << 8},
	[10] = {1UL << 9},
	[11] = {1UL << 10},
	[12] = {1UL << 11},
	[13] = {1UL << 12},
	[14] = {1UL << 13},
	[15] = {1UL << 14},
	[16] = {1UL << 15},
	[17] = {1UL << 16},
	[18] = {1UL << 17},
	[19] = {1UL << 18},
	[20] = {1UL << 19},
	[21] = {1UL << 20},
	[22] = {1UL << 21},
	[23] = {1UL << 22},
	[24] = {1UL << 23},
	[25] = {1UL << 24},
	[26] = {1UL << 25},
	[27] = {1UL << 26},
	[28] = {1UL << 27},
	[29] = {1UL << 28},
	[30] = {1UL << 29},
	[31] = {1UL << 30},
	[32] = {1UL << 31},
	[33] = {1UL << 32},
	[34] = {1UL << 33},
	[35] = {1UL << 34},
	[36] = {1UL << 35},
	[37] = {1UL << 36},
	[38] = {1UL << 37},
	[39] = {1UL << 38},
	[40] = {1UL << 39},
	[41] = {1UL << 40},
	[42] = {1UL << 41},
	[43] = {1UL << 42},
	[44] = {1UL << 43},
	[45] = {1UL << 44},
	[46] = {1UL << 45},
	[47] = {1UL << 46},
	[48] = {1UL << 47},
	[49] = {1UL << 48},
	[50] = {1UL << 49},
	[51] = {1UL << 50},
	[52] = {1UL << 51},
	[53] = {1UL << 52},
	[54] = {1UL << 53},
	[55] = {1UL << 54},
	[56] = {1UL << 55},
	[57] = {1UL << 56},
	[58] = {1UL << 57},
	[59] = {1UL << 58},
	[60] = {1UL << 59},
	[61] = {1UL << 60},
	[62] = {1UL << 61},
	[63] = {1UL << 62},
	[64] = {1UL << 63},
};
int renku_linux_cpumask_publish(unsigned count, unsigned long enabled)
{
	if (nr_cpu_ids) return -EBUSY;
	if (!count || count > NR_CPUS || !enabled) return -EINVAL;
	unsigned long present = count == 64 ? ~0UL : (1UL << count) - 1;
	if (enabled & ~present) return -EINVAL;
	/* Existing native CPU IDs are dense [0,count); enabled state is obtained
	 * independently, so a disabled CPU is present but not online/active. */
	__cpu_possible_mask.bits[0] = __cpu_present_mask.bits[0] = present;
	__cpu_online_mask.bits[0] = __cpu_active_mask.bits[0] = enabled;
	__cpu_dying_mask.bits[0] = 0;
	atomic_set(&__num_online_cpus, hweight_long(enabled));
	nr_cpu_ids = count;
	return 0;
}
void renku_linux_cpumask_reset(void)
{
	/* All users have stopped; lifecycle serializes access to these globals. */
	nr_cpu_ids = 0;
	atomic_set(&__num_online_cpus, 0);
	memset(&__cpu_possible_mask, 0, sizeof(__cpu_possible_mask));
	memset(&__cpu_present_mask, 0, sizeof(__cpu_present_mask));
	memset(&__cpu_online_mask, 0, sizeof(__cpu_online_mask));
	memset(&__cpu_active_mask, 0, sizeof(__cpu_active_mask));
	memset(&__cpu_dying_mask, 0, sizeof(__cpu_dying_mask));
}
