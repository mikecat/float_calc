#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

static uint32_t x = UINT32_C(123456789);
static uint32_t y = UINT32_C(362436069);
static uint32_t z = UINT32_C(521288629);
static uint32_t w = UINT32_C(88675123);

uint32_t randint(void) {
	uint32_t t;
	t = (x ^ (x << 11));
	x = y;
	y = z;
	z = w;
	w = (w ^ (w >> 19)) ^ (t ^ (t >> 8));
	return w;
}

float uint32_to_float(uint32_t value) {
	union {
		uint32_t i;
		float f;
	} x;
	x.i = value;
	return x.f;
}

uint32_t float_to_uint32(float value) {
	union {
		uint32_t i;
		float f;
	} x;
	x.f = value;
	return x.i;
}

float str_to_float(const char* query) {
	uint32_t is_minus = 0;
	uint32_t value = 0, div_value = 1;
	int value_offset = 0, e_value = 0, e_value_is_minus = 0;
	int status = 0, digit_count = 0;
	int result_offset = 0;
	uint32_t result = 0, result_delta = UINT32_C(1) << 24;
	if (*query == '-') {
		is_minus = 1;
		query++;
	} else if (*query == '+') {
		query++;
	}
	while (status >= 0) {
		switch (status) {
			case 0:
				if (*query == '0') {
					if (0 < digit_count && digit_count < 9) {
						value = value * 10;
						digit_count++;
					} else if (digit_count >= 9) {
						value_offset++;
					}
				} else if ('1' <= *query && *query <= '9') {
					if (digit_count < 9) {
						value = value * 10 + (*query - '0');
						digit_count++;
					} else {
						value_offset++;
					}
				}
				else if (*query == '.') {
					status = 1;
				} else if (*query == 'e' || *query == 'E') {
					status = 2;
				} else {
					status = -1;
				}
				break;
			case 1:
				if (*query == '0') {
					if (digit_count == 0) {
						value_offset--;
					} else if (digit_count < 9) {
						value = value * 10;
						digit_count++;
						value_offset--;
					}
				} else if ('1' <= *query && *query <= '9') {
					if (digit_count < 9) {
						value = value * 10 + (*query - '0');
						digit_count++;
						value_offset--;
					}
				} else if (*query == 'e' || *query == 'E') {
					status = 2;
				} else {
					status = -1;
				}
				break;
			case 2:
				if (*query == '+') {
					status = 3;
				} else if (*query == '-') {
					e_value_is_minus = 1;
					status = 3;
				} else if ('0' <= *query && *query <= '9') {
					e_value = *query - '0';
					status = 3;
				} else {
					status = -1;
				}
				break;
			case 3:
				if ('0' <= *query && *query <= '9') {
					e_value = e_value * 10 + (*query - '0');
					if (e_value_is_minus) {
						if (value_offset - e_value < -100) e_value = value_offset + 100;
					} else {
						if (value_offset + e_value > 100) e_value = 100 - value_offset;
					}
				} else {
					status = -1;
				}
				break;
		}
		if (status >= 0) query++;
	}
	if (value == 0) {
		return uint32_to_float(is_minus ? UINT32_C(0x80000000) : 0);
	}
	if (e_value_is_minus) e_value = -e_value;
	e_value += value_offset;
	while (!(value & UINT32_C(0xc0000000))) {
		value <<= 1;
		result_offset--;
	}
	while (!(div_value & UINT32_C(0xc0000000))) {
		div_value <<= 1;
		result_offset++;
	}
	for (; e_value > 0; e_value--) {
		uint32_t v = value;
		result_offset += 3;
		value += value >> 2;
		if (value & UINT32_C(0x80000000)) {
			if ((value & 1) && ((v & 3) || (value & 2))) value += 2;
			value >>= 1;
			result_offset++;
		} else {
			if ((v & 2) && ((v & 1) || (value & 1))) value++;
			if (value & UINT32_C(0x80000000)) {
				value >>= 1;
				result_offset++;
			}
		}
	}
	for (; e_value < 0; e_value++) {
		uint32_t v = div_value;
		result_offset -= 3;
		div_value += div_value >> 2;
		if (div_value & UINT32_C(0x80000000)) {
			if ((div_value & 1) && ((v & 3) || (div_value & 2))) div_value += 2;
			div_value >>= 1;
			result_offset--;
		} else {
			if ((v & 2) && ((v & 1) || (div_value & 1))) div_value++;
			if (div_value & UINT32_C(0x80000000)) {
				div_value >>= 1;
				result_offset--;
			}
		}
	}
	while (value < div_value) {
		value <<= 1;
		result_offset--;
	}
	while (value >= div_value * 2) {
		div_value <<= 1;
		result_offset++;
	}
	while (result_delta > 0) {
		if (value >= div_value) {
			result |= result_delta;
			value -= div_value;
		}
		value <<= 1;
		result_delta >>= 1;
	}
	result_offset += 127;
	if (result_offset <= -24) {
		return uint32_to_float(is_minus ? UINT32_C(0x80000000) : 0);
	}
	if (result_offset <= 0) {
		result = (result + (UINT32_C(1) << (-result_offset + 1))) >> (-result_offset + 2);
		result_offset = result >> 23;
	} else {
		result = (result + 1) >> 1;
		if (result & UINT32_C(0xff000000)) {
			result >>= 1;
			result_offset++;
		}
	}
	if (result_offset >= 0xff) {
		return uint32_to_float((is_minus ? UINT32_C(0x80000000) : 0) | (UINT32_C(0xff) << 23));
	}
	return uint32_to_float(
		(is_minus ? UINT32_C(0x80000000) : 0) |
		((uint32_t)result_offset << 23) |
		(result & UINT32_C(0x007fffff))
	);
	return 0;
}

int main(int argc, char* argv[]) {
	int i;
	int allcnt = 0, ngcnt = 0, super_ngcnt = 0, p1cnt = 0, m1cnt = 0;
	for (i = -argc + 1; i < (argc > 1 ? 0 : 1000000); i++) {
		char str[128], *query;
		float actual, expect;
		uint32_t actual_i, expect_i;
		int is_ng, is_super_ng, is_p1, is_m1;
		if (i < 0) {
			query = argv[argc + i];
		} else {
			snprintf(str, sizeof(str), "%u.%09ue%d",
				randint() % 10, randint() % 1000000000u, (int)(randint() % 91) - 50);
			query = str;
		}
		actual = str_to_float(query);
		if (sscanf(query, "%f", &expect) != 1) expect = atof(query);
		actual_i = float_to_uint32(actual);
		expect_i = float_to_uint32(expect);
		is_ng = actual_i != expect_i;
		is_p1 = actual_i == expect_i + 1;
		is_m1 = actual_i == expect_i - 1;
		is_super_ng = actual_i != expect_i && actual_i + 1 != expect_i && expect_i + 1 != actual_i;
		if (i < 100 || (is_super_ng && super_ngcnt < 100) || (is_p1 && p1cnt < 100) || (is_m1 && m1cnt < 100)) {
			printf("%-16s %-16a %-16a %-16.9e %-16.9e %08" PRIx32 "  %08" PRIx32 "  %s\n",
				query, actual, expect, actual, expect, actual_i, expect_i,
				is_super_ng ? "SUPER NG!!!!!!!!!!" :
				is_p1 ? "NG!!!!! +1" :
				is_m1 ? "NG!!!!! -1" :
				"OK"
			);
		}
		allcnt++;
		if (is_ng) ngcnt++;
		if (is_super_ng) super_ngcnt++;
		if (is_p1) p1cnt++;
		if (is_m1) m1cnt++;
	}
	printf("found %d / %d NGs (%f %%) +1: %d, -1: %d\n", ngcnt, allcnt, ngcnt * 100.0 / allcnt, p1cnt, m1cnt);
	printf("found %d / %d super NGs (%f %%)\n", super_ngcnt, allcnt, super_ngcnt * 100.0 / allcnt);
	return 0;
}
