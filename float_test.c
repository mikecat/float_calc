#include <stdio.h>
#include "float_calc.h"

void test(float a, float b) {
	float add1 = a + b, add2 = add_float(a, b);
	float mul1 = a * b, mul2 = mul_float(a, b);
	unsigned int add1u = float2uint(add1), add2u = float2uint(add2);
	unsigned int mul1u = float2uint(mul1), mul2u = float2uint(mul2);
	printf("%-16.8g %-16.8g %-16.8g %-16.8g %-16.8g %-16.8g %-4s %-4s\n",
		a, b, add1, add2, mul1, mul2, add1u == add2u ? "OK" : "FAIL", mul1u == mul2u ? "OK" : "FAIL");
	if (add1u != add2u || mul1u != mul2u) {
		printf("0x%08x       0x%08x       0x%08x       0x%08x       0x%08x       0x%08x\n",
			float2uint(a), float2uint(b), add1u, add2u, mul1u, mul2u);
	}
}

int main(int argc, char* argv[]) {
	int i;
	printf("%-16s %-16s %-16s %-16s %-16s %-16s %-4s %-4s\n",
		"a", "b", "a + b", "add_float(a, b)", "a * b", "mul_float(a, b)", "add", "mul");
	test(1.234f, 5.678f);
	test(1e99, 1e99);
	test(1e99, -1e99);
	test(1e99, 0.0f);
	test(1e99, -0.0f);
	test(1e99, 1e-40f);
	test(0.0f, 0.0f);
	test(0.0f, -0.0f);
	test(-0.0f, 0.0f);
	test(-0.0f, -0.0f);
	test(0.0 / 0.0, 0.0f);
	test(0.0 / 0.0, 1e99);
	test(1e99, 0.0 / 0.0);
	test(0.0 / 0.0, 0.0 / 0.0);
	test(1e20f, 1e19f);
	test(1.777777e20f, 1.92e18f);
	test(1.777777e20f, 1.91e18f);
	test(1.777e20f, 1.92e18f);
	test(1e-38f, 1e-1f);
	test(1e-38f, 1e-3f);
	test(1e-38f, 1e-5f);
	test(1e-38f, 1e-6f);
	test(1e-38f, 1e-7f);
	test(1e-37f, 1e-1f);
	test(1e-37f, 1e-2f);
	test(1e-37f, 1e-2f);
	test(1e-37f, 1e-3f);
	test(1e-37f, 1e-5f);
	test(1e-37f, 1e-6f);
	test(1e-37f, 1e-7f);
	test(1e-37f, 1e-8f);
	test(1e-37f, 1e-9f);
	test(-1e-37f, 1e-9f);
	test(1.23e-5f, 4.56e-39f);
	test(1e-38f, 1e38f);
	test(1e-38f, 1.12345e38f);
	test(1.12345e-38f, 1.12345e38f);
	test(1.12345e-40f, 5.6789e35);
	test(5.234e-39, 9.678e-39);
	test(1.23e-36, 9.678e-39);
	test(5.234e-39, 1.23e-36);
	test(-5.234e-39, 9.678e-39);
	test(-1.23e-36, 9.678e-39);
	test(-5.234e-39, 1.23e-36);
	test(5.234e-39, -9.678e-39);
	test(1.23e-36, -9.678e-39);
	test(5.234e-39, -1.23e-36);
	test(1.0f, -1.000001f);
	test(-1.0f, 1.000001f);
	test(16777216.0f, 1.0f);
	test(16777216.0f, -1.0f);
	test(16777218.0f, 1.0f);
	test(16777218.0f, -1.0f);
	test(33554432.0f, 1.0f);
	test(33554432.0f, 2.0f);
	test(33554432.0f, 3.0f);
	test(33554436.0f, 1.0f);
	test(33554436.0f, 2.0f);
	test(33554436.0f, 3.0f);
	test(67108864.0f, 3.0f);
	test(67108864.0f, 4.0f);
	test(67108864.0f, 5.0f);
	test(67108872.0f, 3.0f);
	test(67108872.0f, 4.0f);
	test(67108872.0f, 5.0f);
	for (i = 1; i + 2 <= argc; i += 2) {
		float a, b;
		if (sscanf(argv[i], "%f", &a) == 1 && sscanf(argv[i + 1], "%f", &b) == 1) {
			test(a, b);
		}
	}
	return 0;
}
