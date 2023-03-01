#include "float_calc.h"

#define FRACTION_BITS 23
#define EXPONENT_BITS 8

#define FRACTION_MASK ((UINT32_C(1) << FRACTION_BITS) - 1)
#define EXPONENT_MASK ((UINT32_C(1) << EXPONENT_BITS) - 1)
#define SIGN_MASK (UINT32_C(1) << (EXPONENT_BITS + FRACTION_BITS))
#define EXPONENT_BIAS 127

#define FRACTION_MASK_2X ((UINT64_C(1) << (FRACTION_BITS * 2)) - 1)
#define FRACTION_MASK_2X1 ((UINT64_C(1) << (FRACTION_BITS * 2 + 1)) - 1)
#define FRACTION_MASK_1 ((UINT32_C(1) << (FRACTION_BITS + 1)) - 1)
#define FRACTION_MASK_2 ((UINT32_C(1) << (FRACTION_BITS + 2)) - 1)
#define FRACTION_MASK_3 ((UINT32_C(1) << (FRACTION_BITS + 3)) - 1)
#define FRACTION_MASK_4 ((UINT32_C(1) << (FRACTION_BITS + 4)) - 1)

#define NAN_VALUE ((EXPONENT_MASK << FRACTION_BITS) | (UINT32_C(1) << (FRACTION_BITS - 1)))
#define INF_VALUE (EXPONENT_MASK << FRACTION_BITS)

uint32_t float2uint(float a) {
	union {
		float f;
		uint32_t u;
	} u;
	u.f = a;
	return u.u;
}

float uint2float(uint32_t a) {
	union {
		float f;
		uint32_t u;
	} u;
	u.u = a;
	return u.f;
}

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

float finalize_float(int sign, int exponent, uint32_t fraction_s2) {
	int re = exponent;
	uint32_t rf = fraction_s2;
	if (re <= 0) {
		int shift_width = 1 - re;
		if (shift_width >= FRACTION_BITS + 3) {
			rf = rf != 0;
		} else {
			rf = (rf >> shift_width) | ((rf & ((1 << shift_width) - 1)) != 0);
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
		return uint2float(INF_VALUE | (sign ? SIGN_MASK : 0));
	}
	return uint2float((sign ? SIGN_MASK : 0) | (re << FRACTION_BITS) | ((rf >> 2) & FRACTION_MASK));
}

float add_float(float a, float b) {
	uint32_t au = float2uint(a), bu = float2uint(b);
	int ae = (au >> FRACTION_BITS) & EXPONENT_MASK, be = (bu >> FRACTION_BITS) & EXPONENT_MASK;
	uint32_t af = au & FRACTION_MASK, bf = bu & FRACTION_MASK;
	int rs = 0, re;
	uint32_t rf;
	/* nan */
	if (is_nan(au) || is_nan(bu)) return uint2float(NAN_VALUE);
	/* inf */
	if (is_inf(au)) {
		if (is_inf(bu) && ((au ^ bu) & SIGN_MASK)) {
			/* inf + -inf */
			return uint2float(NAN_VALUE);
		} else {
			return uint2float(au);
		}
	}
	if (is_inf(bu)) return add_float(b, a);
	/* 0 */
	if (is_zero(au)) {
		if (is_zero(bu)) return uint2float(au & bu);
		return uint2float(bu);
	}
	if(is_zero(bu)) return add_float(b, a);

	if (ae != 0) af |= 1 << FRACTION_BITS; else af <<= 1;
	if (be != 0) bf |= 1 << FRACTION_BITS; else bf <<= 1;
	af <<= 3;
	bf <<= 3;
	re = ae;
	if (ae <= be - (FRACTION_BITS + 4)) {
		re = be;
		af = 1;
	} else if (ae < be) {
		re = be;
		af = (af >> (be - ae)) | ((af & ((1 << (be - ae)) - 1)) != 0);
	} else if (ae - (FRACTION_BITS + 4) >= be) {
		bf = 1;
	} else if (ae > be) {
		bf = (bf >> (ae - be)) | ((bf & ((1 << (ae - be)) - 1)) != 0);
	}
	if (au & SIGN_MASK) af = -af;
	if (bu & SIGN_MASK) bf = -bf;
	rf = af + bf;
	if (rf & UINT32_C(0x80000000)) { rf = -rf; rs = 1; }
	if (rf == 0) return uint2float(0);
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

float sub_float(float a, float b) {
	return add_float(a, uint2float(float2uint(b) ^ SIGN_MASK));
}

float mul_float(float a, float b) {
	uint32_t au = float2uint(a), bu = float2uint(b);
	int ae = (au >> FRACTION_BITS) & EXPONENT_MASK, be = (bu >> FRACTION_BITS) & EXPONENT_MASK;
	uint64_t af = au & FRACTION_MASK, bf = bu & FRACTION_MASK;
	int re;
	uint64_t rf;
	/* nan */
	if (is_nan(au) || is_nan(bu)) return uint2float(NAN_VALUE);
	/* inf */
	if (is_inf(au)) {
		if (is_zero(bu)) {
			/* inf * 0 */
			return uint2float(NAN_VALUE);
		} else {
			return uint2float(au ^ (bu & SIGN_MASK));
		}
	}
	if (is_inf(bu)) return mul_float(b, a);
	/* 0 */
	if (is_zero(au)) {
		return uint2float((au ^ bu) & SIGN_MASK);
	}
	if (is_zero(bu)) return mul_float(b, a);

	if (ae != 0) af |= 1 << FRACTION_BITS; else af <<= 1;
	if (be != 0) bf |= 1 << FRACTION_BITS; else bf <<= 1;
	re = ae + be - EXPONENT_BIAS;
	rf = af * bf;
	if (rf & ~FRACTION_MASK_2X1) {
		re++;
		if (rf & 1) rf |= 2;
		rf >>= 1;
	}
	while (!(rf & ~FRACTION_MASK_2X)) {
		re--;
		rf <<= 1;
	}
	rf = (rf >> (FRACTION_BITS - 2)) | ((rf & ((1 << (FRACTION_BITS - 2)) - 1)) != 0);
	return finalize_float(((au ^ bu) & SIGN_MASK) != 0, re, rf);
}

float div_float(float a, float b) {
	uint32_t au = float2uint(a), bu = float2uint(b);
	int ae = (au >> FRACTION_BITS) & EXPONENT_MASK, be = (bu >> FRACTION_BITS) & EXPONENT_MASK;
	uint32_t af = au & FRACTION_MASK, bf = bu & FRACTION_MASK;
	int re;
	uint32_t rf;
	uint32_t left, delta;
	/* nan */
	if (is_nan(au) || is_nan(bu)) return uint2float(NAN_VALUE);
	/* 0 */
	if (is_zero(bu)) {
		if (is_zero(au)) {
			/* 0 / 0 */
			return uint2float(NAN_VALUE);
		} else {
			return uint2float(((au ^ bu) & SIGN_MASK) | INF_VALUE);
		}
	}
	if (is_zero(au)) {
		return uint2float((au ^ bu) & SIGN_MASK);
	}
	/* inf */
	if (is_inf(au)) {
		if (is_inf(bu)) {
			/* inf / inf */
			return uint2float(NAN_VALUE);
		} else {
			return uint2float(((au ^ bu) & SIGN_MASK) | INF_VALUE);
		}
	}
	if (is_inf(bu)) {
		return uint2float((au ^ bu) & SIGN_MASK);
	}

	if (ae != 0) af |= 1 << FRACTION_BITS; else af <<= 1;
	if (be != 0) bf |= 1 << FRACTION_BITS; else bf <<= 1;
	re = ae - be + EXPONENT_BIAS;
	rf = 0;
	left = af;
	delta = 1 << (FRACTION_BITS + 2);
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
	return finalize_float(((au ^ bu) & SIGN_MASK) != 0, re, rf);
}
