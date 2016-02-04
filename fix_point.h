#ifndef _FIX_POINT_H_
#define _FIX_POINT_H_

#include <stdint.h>
#include <math.h>

#define FRAC_BITS 8

#define FIX_ONE ((int16_t) 1 << FRAC_BITS)
#define FIX_HALF (FIX_ONE / 2)

#define INT_TO_FIX(x) ((x) * FIX_ONE)
#define FIX_TO_INT(x) ((x) / FIX_ONE)
#define FIX_TO_NEAREST_INT(x) ((x) < 0 ? (((x) + FIX_HALF - 1) >> FRAC_BITS) : (((x) + FIX_HALF) >> FRAC_BITS))
#define FLOAT_TO_FIX(x) (lround((x) * FIX_ONE))
#define FIX_TO_FLOAT(x) ((float) (x) / FIX_ONE)

#define FIX_MUL(a, b) (((int32_t) (a) * (b)) / FIX_ONE)
#define FIX_DIV(a, b) (((int32_t) (a) * FIX_ONE) / (b))

#endif
