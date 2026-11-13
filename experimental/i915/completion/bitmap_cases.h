/* SPDX-License-Identifier: MIT */
#ifndef RENKU_BITMAP_CASES_H
#define RENKU_BITMAP_CASES_H
#include <linux/bitmap.h>
#include <asm/ioctl.h>
/* An independent bit-by-bit oracle, including dirty bits beyond the limit. */
static bool OracleBit(const unsigned long *a, const unsigned long *b,
	const unsigned long *c, unsigned bit, unsigned mode)
{
	bool x = (a[bit / 64] >> (bit % 64)) & 1;
	bool y = (b[bit / 64] >> (bit % 64)) & 1;
	bool z = (c[bit / 64] >> (bit % 64)) & 1;
	switch (mode) {
	case 0: return x;
	case 1: return !x;
	case 2: return x && y;
	case 3: return x && !y;
	case 4: return x || y;
	default: return x && y && !z;
	}
}
static unsigned long OracleFind(const unsigned long *a, const unsigned long *b,
	const unsigned long *c, unsigned size, unsigned start, unsigned nth, unsigned mode)
{
	for (unsigned i = start; i < size; i++)
		if (OracleBit(a, b, c, i, mode) && nth-- == 0) return i;
	return size;
}
static void BitmapCases(struct completion_report *r)
{
	_Static_assert(BITS_PER_LONG == 64 && BITS_PER_LONG_LONG == 64, "x86-64 bitmap widths");
	_Static_assert(sizeof(loff_t) == 8, "Linux seek offset width");
	_Static_assert(GENMASK(63, 0) == ~0UL && GENMASK_ULL(63, 63) == (1ULL << 63), "mask boundaries");
	_Static_assert(_IOC(_IOC_READ, 'd', 0x42, 8) == 0x80086442U, "Linux ioctl encoding");
	CHECK(r, _IOC_SIZE(_IOWR('d', 0x40, u64)) == sizeof(u64));
	CHECK(r, _IOC_DIR(_IOWR('d', 0x40, u64)) == (_IOC_READ | _IOC_WRITE));
	for (unsigned bit = 0; bit < 64; bit++) {
		unsigned long v = 1UL << bit;
		CHECK(r, __ffs(v) == bit && __fls(v) == bit && ffz(~v) == bit);
		CHECK(r, fls64(v) == bit + 1 && fls_long(v) == bit + 1);
		CHECK(r, hweight_long(v) == 1 && hweight_long(~v) == 63);
	}
	CHECK(r, fls(0) == 0 && fls64(0) == 0 && hweight_long(0) == 0);
	CHECK(r, hweight8(0xff) == 8 && hweight16(0xffff) == 16 && hweight32(~0u) == 32);
	DECLARE_BITMAP(bits, 192);
	for (unsigned bit = 0; bit < 192; bit++) {
		memset(bits, 0, sizeof(bits));
		CHECK(r, !test_and_set_bit(bit, bits) && test_and_set_bit(bit, bits));
		CHECK(r, test_bit_acquire(bit, bits) && test_and_clear_bit(bit, bits));
		CHECK(r, !test_and_change_bit(bit, bits) && test_and_change_bit(bit, bits));
		set_bit(bit, bits); change_bit(bit, bits); CHECK(r, !test_bit(bit, bits));
		__set_bit(bit, bits); CHECK(r, __test_and_clear_bit(bit, bits));
		__change_bit(bit, bits); CHECK(r, __test_and_change_bit(bit, bits));
		__assign_bit(bit, bits, true); assign_bit(bit, bits, false);
		CHECK(r, !test_bit(bit, bits) && !test_and_set_bit_lock(bit, bits));
		clear_bit_unlock(bit, bits); CHECK(r, !test_bit(bit, bits));
	}
	const unsigned sizes[] = {0, 1, 7, 63, 64, 65, 127, 128, 129, 191, 192};
	for (unsigned pattern = 0; pattern < 5; pattern++) {
		unsigned long a[3], b[3], c[3];
		for (unsigned j = 0; j < 3; j++) {
			a[j] = pattern == 0 ? 0 : pattern == 1 ? ~0UL :
				(0x915a076d1234f00dUL * (j + pattern)) ^ (1UL << (j * 29));
			b[j] = pattern == 0 ? ~0UL : 0x0101808080410201UL * (j + pattern);
			c[j] = ~b[j] ^ 0x5555555555555555UL;
		}
		for (unsigned s = 0; s < sizeof(sizes) / sizeof(sizes[0]); s++) {
			unsigned size = sizes[s];
			bool equal = true;
			for (unsigned i = 0; i < size; i++)
				if (((a[i / 64] ^ b[i / 64]) >> (i % 64)) & 1) equal = false;
			CHECK(r, __bitmap_equal(a, b, size) == equal);
			CHECK(r, bitmap_equal(a, b, size) == equal);
			unsigned long dirty[3];
			memcpy(dirty, a, sizeof(dirty));
			for (unsigned i = size; i < 192; i++) __change_bit(i, dirty);
			CHECK(r, __bitmap_equal(a, dirty, size));
			if (size) {
				__change_bit(size - 1, dirty);
				CHECK(r, !__bitmap_equal(a, dirty, size));
			}
			for (unsigned start = 0; start <= size + 1; start++) {
				CHECK(r, find_next_bit(a, size, start) == OracleFind(a,b,c,size,start,0,0));
				CHECK(r, find_next_zero_bit(a, size, start) == OracleFind(a,b,c,size,start,0,1));
				CHECK(r, find_next_and_bit(a,b,size,start) == OracleFind(a,b,c,size,start,0,2));
				CHECK(r, find_next_andnot_bit(a,b,size,start) == OracleFind(a,b,c,size,start,0,3));
				CHECK(r, find_next_or_bit(a,b,size,start) == OracleFind(a,b,c,size,start,0,4));
			}
			CHECK(r, find_first_bit(a,size) == OracleFind(a,b,c,size,0,0,0));
			CHECK(r, find_first_zero_bit(a,size) == OracleFind(a,b,c,size,0,0,1));
			CHECK(r, find_first_and_bit(a,b,size) == OracleFind(a,b,c,size,0,0,2));
			unsigned last = size;
			for (unsigned i = 0; i < size; i++) if (OracleBit(a,b,c,i,0)) last = i;
			CHECK(r, find_last_bit(a,size) == last);
			/* Linux nth searches may return any value >= size when absent. */
			for (unsigned nth = 0; nth <= size; nth++) {
				unsigned long found = find_nth_bit(a,size,nth);
				CHECK(r, min(found,(unsigned long)size) == OracleFind(a,b,c,size,0,nth,0));
				found = find_nth_and_bit(a,b,size,nth);
				CHECK(r, min(found,(unsigned long)size) == OracleFind(a,b,c,size,0,nth,2));
				found = find_nth_andnot_bit(a,b,size,nth);
				CHECK(r, min(found,(unsigned long)size) == OracleFind(a,b,c,size,0,nth,3));
				found = find_nth_and_andnot_bit(a,b,c,size,nth);
				CHECK(r, min(found,(unsigned long)size) == OracleFind(a,b,c,size,0,nth,5));
			}
		}
	}
	/* Constant-size calls exercise the original inline fast paths as well. */
	unsigned long single = 0x8000000000000081UL;
	CHECK(r, find_first_bit(&single, 7) == 0 && find_next_bit(&single, 7, 1) == 7);
	CHECK(r, find_last_bit(&single, 63) == 7 && find_next_zero_bit(&single, 1, 0) == 1);
	CHECK(r, __bitmap_equal(NULL, NULL, 0));
	CHECK(r, find_first_bit(NULL, 0) == 0 && find_nth_bit(NULL, 0, 0) == 0);
}
#endif
