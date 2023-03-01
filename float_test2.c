#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "float_calc.h"

int read_float(uint32_t* out, const char* str) {
	char sign;
	uint32_t a, b;
	int c;
	if (strcmp(str, "-Zero") == 0) { *out = UINT32_C(0x80000000); return 1; }
	if (strcmp(str, "+Zero") == 0) { *out = UINT32_C(0x00000000); return 1; }
	if (strcmp(str, "-Inf") == 0) { *out = UINT32_C(0xff800000); return 1; }
	if (strcmp(str, "+Inf") == 0) { *out = UINT32_C(0x7f800000); return 1; }
	if (strcmp(str, "Q") == 0) { *out = UINT32_C(0x7fc00000); return 1; }
	if (strcmp(str, "S") == 0) { *out = UINT32_C(0x7f800001); return 1; }
	if (strcmp(str, "#") == 0) { *out = UINT32_C(0x7fc00000); return 1; }
	if (sscanf(str, "%c%" SCNu32 ".%" SCNx32 "P%d", &sign, &a, &b, &c) == 4) {
		uint32_t value = sign == '-' ? UINT32_C(0x80000000) : 0;
		if (sign != '+' && sign != '-') return 0;
		if (b & ~UINT32_C(0x7fffff)) return 0;
		value |= b;
		if (a == 0) {
			if (c != -126) return 0;
		} else {
			if (c < -126 || 127 < c) return 0;
			value |= (uint32_t)(c + 127) << 23;
		}
		*out = value;
		return 1;
	}
	return 0;
}

int main(void) {
	int add_cnt = 0, sub_cnt = 0, mul_cnt = 0, div_cnt = 0;
	int add_fail = 0, sub_fail = 0, mul_fail = 0, div_fail = 0;
	char buf[1024], buf_bak[1024];
	while (fgets(buf, sizeof(buf), stdin) != NULL) {
		uint32_t params[16];
		uint32_t expected, actual;
		int param_count = 0;
		char *name, *p;
		strtok(buf, "\n");
		strcpy(buf_bak, buf);
		name = strtok(buf, " ");
		while (param_count < 16 && (p = strtok(NULL, " ")) != NULL) {
			if (read_float(&params[param_count], p)) param_count++;
		}
		if (strcmp(name, "b32+") == 0) {
			if (param_count != 3) {
				printf("invalid\t%s\n", buf_bak);
				continue;
			}
			add_cnt++;
			actual = add_float(params[0], params[1]);
			expected = params[2];
			if (expected != actual) {
				printf("failed: expected 0x%08" PRIx32 " actual 0x%08" PRIx32 "\t%s\n", expected, actual, buf_bak);
				add_fail++;
			}
		} else if (strcmp(name, "b32-") == 0) {
			if (param_count != 3) {
				printf("invalid\t%s\n", buf_bak);
				continue;
			}
			sub_cnt++;
			actual = sub_float(params[0], params[1]);
			expected = params[2];
			if (expected != actual) {
				printf("failed: expected 0x%08" PRIx32 " actual 0x%08" PRIx32 "\t%s\n", expected, actual, buf_bak);
				sub_fail++;
			}
		} else if (strcmp(name, "b32*") == 0) {
			if (param_count != 3) {
				printf("invalid\t%s\n", buf_bak);
				continue;
			}
			mul_cnt++;
			actual = mul_float(params[0], params[1]);
			expected = params[2];
			if (expected != actual) {
				printf("failed: expected 0x%08" PRIx32 " actual 0x%08" PRIx32 "\t%s\n", expected, actual, buf_bak);
				mul_fail++;
			}
		} else if (strcmp(name, "b32/") == 0) {
			if (param_count != 3) {
				printf("invalid\t%s\n", buf_bak);
				continue;
			}
			div_cnt++;
			actual = div_float(params[0], params[1]);
			expected = params[2];
			if (expected != actual) {
				printf("failed: expected 0x%08" PRIx32 " actual 0x%08" PRIx32 "\t%s\n", expected, actual, buf_bak);
				div_fail++;
			}
		}
	}
	printf("add: %d tested, %d failed\n", add_cnt, add_fail);
	printf("sub: %d tested, %d failed\n", sub_cnt, sub_fail);
	printf("mul: %d tested, %d failed\n", mul_cnt, mul_fail);
	printf("div: %d tested, %d failed\n", div_cnt, div_fail);
	return 0;
}
