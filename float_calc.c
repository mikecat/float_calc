#include "float_calc.h"

#define FRACTION_BITS 23
#define EXPONENT_BITS 8

#define FRACTION_MASK ((1u << FRACTION_BITS) - 1u)
#define EXPONENT_MASK ((1u << EXPONENT_BITS) - 1u)
#define SIGN_MASK (1u << (EXPONENT_BITS + FRACTION_BITS))
#define EXPONENT_BIAS 127

#define FRACTION_MASK_2X ((1ull << (FRACTION_BITS * 2)) - 1ull)
#define FRACTION_MASK_2X1 ((1ull << (FRACTION_BITS * 2 + 1)) - 1ull)
#define FRACTION_MASK_1 ((1u << (FRACTION_BITS + 1)) - 1u)
#define FRACTION_MASK_2 ((1u << (FRACTION_BITS + 2)) - 1u)
#define FRACTION_MASK_3 ((1u << (FRACTION_BITS + 3)) - 1u)
#define FRACTION_MASK_4 ((1u << (FRACTION_BITS + 4)) - 1u)

#define NAN_VALUE ((EXPONENT_MASK << FRACTION_BITS) | (1u << (FRACTION_BITS - 1)))
#define INF_VALUE (EXPONENT_MASK << FRACTION_BITS)

unsigned int float2uint(float a) {
	union {
		float f;
		unsigned int u;
	} u;
	u.f = a;
	return u.u;
}

float uint2float(unsigned int a) {
	union {
		float f;
		unsigned int u;
	} u;
	u.u = a;
	return u.f;
}

int is_nan(unsigned int a) {
	int ae = (a >> FRACTION_BITS) & EXPONENT_MASK;
	int af = a & FRACTION_MASK;
	return ae == EXPONENT_MASK && af != 0;
}

int is_inf(unsigned int a) {
	int ae = (a >> FRACTION_BITS) & EXPONENT_MASK;
	int af = a & FRACTION_MASK;
	return ae == EXPONENT_MASK && af == 0;
}

int is_zero(unsigned int a) {
	return (a & ~SIGN_MASK) == 0;
}

float finalize_float(int sign, int exponent, int fraction_s2) {
	int re = exponent, rf = fraction_s2;
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
	unsigned int au = float2uint(a), bu = float2uint(b);
	int ae = (au >> FRACTION_BITS) & EXPONENT_MASK, be = (bu >> FRACTION_BITS) & EXPONENT_MASK;
	int af = au & FRACTION_MASK, bf = bu & FRACTION_MASK;
	int re, rf;
	int rs = 0;
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
	if (rf < 0) { rf = -rf; rs = 1; }
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
	unsigned int au = float2uint(a), bu = float2uint(b);
	int ae = (au >> FRACTION_BITS) & EXPONENT_MASK, be = (bu >> FRACTION_BITS) & EXPONENT_MASK;
	unsigned long long af = au & FRACTION_MASK, bf = bu & FRACTION_MASK;
	int re;
	unsigned long long rf;
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
