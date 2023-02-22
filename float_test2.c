#include <stdio.h>
#include <string.h>
#include "float_calc.h"

int read_float(float* out, const char* str) {
	char sign;
	unsigned int a, b;
	int c;
	if (strcmp(str, "-Zero") == 0) { *out = uint2float(0x80000000u); return 1; }
	if (strcmp(str, "+Zero") == 0) { *out = uint2float(0x00000000u); return 1; }
	if (strcmp(str, "-Inf") == 0) { *out = uint2float(0xff800000u); return 1; }
	if (strcmp(str, "+Inf") == 0) { *out = uint2float(0x7f800000u); return 1; }
	if (strcmp(str, "Q") == 0) { *out = uint2float(0x7fc00000u); return 1; }
	if (strcmp(str, "S") == 0) { *out = uint2float(0x7f800001u); return 1; }
	if (strcmp(str, "#") == 0) { *out = uint2float(0x7fc00000u); return 1; }
	if (sscanf(str, "%c%u.%xP%d", &sign, &a, &b, &c) == 4) {
		unsigned int value = sign == '-' ? 1u << 31 : 0u;
		if (sign != '+' && sign != '-') return 0;
		if (b & ~0x7fffff) return 0;
		value |= b;
		if (a == 0) {
			if (c != -126) return 0;
		} else {
			if (c < -126 || 127 < c) return 0;
			value |= (c + 127) << 23;
		}
		*out = uint2float(value);
		return 1;
	}
	return 0;
}

int main(void) {
	int add_cnt = 0, sub_cnt = 0, mul_cnt = 0;
	int add_fail = 0, sub_fail = 0, mul_fail = 0;
	char buf[1024], buf_bak[1024];
	while (fgets(buf, sizeof(buf), stdin) != NULL) {
		float params[16], res;
		unsigned int expected, actual;
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
			res = add_float(params[0], params[1]);
			expected = float2uint(params[2]);
			actual = float2uint(res);
			if (expected != actual) {
				printf("failed: expected 0x%08x actual 0x%08x\t%s\n", expected, actual, buf_bak);
				add_fail++;
			}
		} else if (strcmp(name, "b32-") == 0) {
			if (param_count != 3) {
				printf("invalid\t%s\n", buf_bak);
				continue;
			}
			sub_cnt++;
			res = sub_float(params[0], params[1]);
			expected = float2uint(params[2]);
			actual = float2uint(res);
			if (expected != actual) {
				printf("failed: expected 0x%08x actual 0x%08x\t%s\n", expected, actual, buf_bak);
				sub_fail++;
			}
		} else if (strcmp(name, "b32*") == 0) {
			if (param_count != 3) {
				printf("invalid\t%s\n", buf_bak);
				continue;
			}
			mul_cnt++;
			res = mul_float(params[0], params[1]);
			expected = float2uint(params[2]);
			actual = float2uint(res);
			if (expected != actual) {
				printf("failed: expected 0x%08x actual 0x%08x\t%s\n", expected, actual, buf_bak);
				mul_fail++;
			}
		}
	}
	printf("add: %d tested, %d failed\n", add_cnt, add_fail);
	printf("sub: %d tested, %d failed\n", sub_cnt, sub_fail);
	printf("mul: %d tested, %d failed\n", mul_cnt, mul_fail);
	return 0;
}
