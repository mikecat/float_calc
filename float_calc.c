#include "float_calc.h"

#define FRACTION_BITS 23
#define EXPONENT_BITS 8

#define FRACTION_MASK ((UINT32_C(1) << FRACTION_BITS) - 1)
#define EXPONENT_MASK ((UINT32_C(1) << EXPONENT_BITS) - 1)
#define SIGN_MASK (UINT32_C(1) << (EXPONENT_BITS + FRACTION_BITS))
#define EXPONENT_BIAS 127

#define FRACTION_MASK_2 ((UINT32_C(1) << (FRACTION_BITS + 2)) - 1)
#define FRACTION_MASK_3 ((UINT32_C(1) << (FRACTION_BITS + 3)) - 1)
#define FRACTION_MASK_4 ((UINT32_C(1) << (FRACTION_BITS + 4)) - 1)

#define NAN_VALUE ((EXPONENT_MASK << FRACTION_BITS) | (UINT32_C(1) << (FRACTION_BITS - 1)))
#define INF_VALUE (EXPONENT_MASK << FRACTION_BITS)

int is_nan(uint32_t a) {
	uint32_t ae = (a >> FRACTION_BITS) & EXPONENT_MASK;
	uint32_t af = a & FRACTION_MASK;
	return ae == EXPONENT_MASK && af != 0;
}

int is_inf(uint32_t a) {
	uint32_t ae = (a >> FRACTION_BITS) & EXPONENT_MASK;
	uint32_t af = a & FRACTION_MASK;
	return ae == EXPONENT_MASK && af == 0;
}

int is_zero(uint32_t a) {
	return (a & ~SIGN_MASK) == 0;
}

uint32_t finalize_float(int sign, int exponent, uint32_t fraction_s2) {
	int re = exponent;
	uint32_t rf = fraction_s2;
	if (re <= 0) {
		int shift_width = 1 - re;
		if (shift_width >= FRACTION_BITS + 3) {
			rf = rf != 0;
		} else {
			rf = (rf >> shift_width) | ((rf & ((UINT32_C(1) << shift_width) - 1)) != 0);
		}
		re = 0;
	}
	if (rf & 2) {
		if (rf & 5) {
			rf += 4;
			if (rf & ~FRACTION_MASK_3) {
				re++;
				rf >>= 1;
			}
			if (re == 0 && (rf & ~FRACTION_MASK_2)) {
				re = 1;
			}
		}
	}
	if (re >= (int)EXPONENT_MASK) {
		return INF_VALUE | (sign ? SIGN_MASK : 0);
	}
	return (sign ? SIGN_MASK : 0) | ((uint32_t)re << FRACTION_BITS) | ((rf >> 2) & FRACTION_MASK);
}

uint32_t add_float(uint32_t a, uint32_t b) {
	int ae = (a >> FRACTION_BITS) & EXPONENT_MASK, be = (b >> FRACTION_BITS) & EXPONENT_MASK;
	uint32_t af = a & FRACTION_MASK, bf = b & FRACTION_MASK;
	int rs = 0, re;
	uint32_t rf;
	/* nan */
	if (is_nan(a) || is_nan(b)) return NAN_VALUE;
	/* inf */
	if (is_inf(a)) {
		if (is_inf(b) && ((a ^ b) & SIGN_MASK)) {
			/* inf + -inf */
			return NAN_VALUE;
		} else {
			return a;
		}
	}
	if (is_inf(b)) return add_float(b, a);
	/* 0 */
	if (is_zero(a)) {
		if (is_zero(b)) return a & b;
		return b;
	}
	if(is_zero(b)) return add_float(b, a);

	if (ae != 0) af |= UINT32_C(1) << FRACTION_BITS; else af <<= 1;
	if (be != 0) bf |= UINT32_C(1) << FRACTION_BITS; else bf <<= 1;
	af <<= 3;
	bf <<= 3;
	re = ae;
	if (ae <= be - (FRACTION_BITS + 4)) {
		re = be;
		af = 1;
	} else if (ae < be) {
		re = be;
		af = (af >> (be - ae)) | ((af & ((UINT32_C(1) << (be - ae)) - 1)) != 0);
	} else if (ae - (FRACTION_BITS + 4) >= be) {
		bf = 1;
	} else if (ae > be) {
		bf = (bf >> (ae - be)) | ((bf & ((UINT32_C(1) << (ae - be)) - 1)) != 0);
	}
	if (a & SIGN_MASK) af = -af;
	if (b & SIGN_MASK) bf = -bf;
	rf = af + bf;
	if (rf & UINT32_C(0x80000000)) { rf = -rf; rs = 1; }
	if (rf == 0) return 0;
	if (rf & ~FRACTION_MASK_4) {
		re++;
		if (rf & 1) rf |= 2;
		rf >>= 1;
	}
	while (!(rf & ~FRACTION_MASK_3)) {
		re--;
		rf <<= 1;
	}
	return finalize_float(rs, re, (rf >> 1) | (rf & 1));
}

uint32_t sub_float(uint32_t a, uint32_t b) {
	return add_float(a, b ^ SIGN_MASK);
}

uint32_t mul_float(uint32_t a, uint32_t b) {
	int ae = (a >> FRACTION_BITS) & EXPONENT_MASK, be = (b >> FRACTION_BITS) & EXPONENT_MASK;
	uint32_t af = a & FRACTION_MASK, bf = b & FRACTION_MASK;
	int re;
	uint32_t rf;
	int i, prev;
	/* nan */
	if (is_nan(a) || is_nan(b)) return NAN_VALUE;
	/* inf */
	if (is_inf(a)) {
		if (is_zero(b)) {
			/* inf * 0 */
			return NAN_VALUE;
		} else {
			return a ^ (b & SIGN_MASK);
		}
	}
	if (is_inf(b)) return mul_float(b, a);
	/* 0 */
	if (is_zero(a)) {
		return (a ^ b) & SIGN_MASK;
	}
	if (is_zero(b)) return mul_float(b, a);

	if (ae != 0) af |= UINT32_C(1) << FRACTION_BITS; else af <<= 1;
	if (be != 0) bf |= UINT32_C(1) << FRACTION_BITS; else bf <<= 1;
	re = ae + be - EXPONENT_BIAS;
	while (!(af & ~FRACTION_MASK)) {
		re--;
		af <<= 1;
	}
	af <<= 3;
	rf = 0;
	for (i = prev = 0; i < FRACTION_BITS + 1; i++) {
		if (bf & 1) {
			int width = i - prev;
			rf = (rf >> width) | ((rf & ((UINT32_C(1) << width) - 1)) != 0);
			rf += af;
			prev = i;
		}
		bf >>= 1;
	}
	re -= FRACTION_BITS - prev;
	if (rf & ~FRACTION_MASK_4) {
		re++;
		if (rf & 1) rf |= 2;
		rf >>= 1;
	}
	while (!(rf & ~FRACTION_MASK_3)) {
		re--;
		rf <<= 1;
	}
	return finalize_float(((a ^ b) & SIGN_MASK) != 0, re, (rf >> 1) | (rf & 1));
}

uint32_t div_float(uint32_t a, uint32_t b) {
	int ae = (a >> FRACTION_BITS) & EXPONENT_MASK, be = (b >> FRACTION_BITS) & EXPONENT_MASK;
	uint32_t af = a & FRACTION_MASK, bf = b & FRACTION_MASK;
	int re;
	uint32_t rf;
	uint32_t left, delta;
	/* nan */
	if (is_nan(a) || is_nan(b)) return NAN_VALUE;
	/* 0 */
	if (is_zero(b)) {
		if (is_zero(a)) {
			/* 0 / 0 */
			return NAN_VALUE;
		} else {
			return ((a ^ b) & SIGN_MASK) | INF_VALUE;
		}
	}
	if (is_zero(a)) {
		return (a ^ b) & SIGN_MASK;
	}
	/* inf */
	if (is_inf(a)) {
		if (is_inf(b)) {
			/* inf / inf */
			return NAN_VALUE;
		} else {
			return ((a ^ b) & SIGN_MASK) | INF_VALUE;
		}
	}
	if (is_inf(b)) {
		return (a ^ b) & SIGN_MASK;
	}

	if (ae != 0) af |= UINT32_C(1) << FRACTION_BITS; else af <<= 1;
	if (be != 0) bf |= UINT32_C(1) << FRACTION_BITS; else bf <<= 1;
	re = ae - be + EXPONENT_BIAS;
	rf = 0;
	left = af;
	delta = UINT32_C(1) << (FRACTION_BITS + 2);
	while (!(bf & ~FRACTION_MASK)) {
		bf <<= 1;
		re++;
	}
	while (left < bf) {
		left <<= 1;
		re--;
	}
	while (left > 0 && delta > 0) {
		if (left >= bf) {
			rf |= delta;
			left -= bf;
		}
		left <<= 1;
		delta >>= 1;
	}
	if (left > 0) rf |= 1;
	return finalize_float(((a ^ b) & SIGN_MASK) != 0, re, rf);
}
