#ifndef FLOAT_CALC_H_GUARD_0AFDD16A_043C_478B_8193_8B5F882D9963
#define FLOAT_CALC_H_GUARD_0AFDD16A_043C_478B_8193_8B5F882D9963

#include <stdint.h>

uint32_t float2uint(float a);
float uint2float(uint32_t a);

uint32_t add_float(uint32_t a, uint32_t b);
uint32_t sub_float(uint32_t a, uint32_t b);
uint32_t mul_float(uint32_t a, uint32_t b);
uint32_t div_float(uint32_t a, uint32_t b);

#endif
