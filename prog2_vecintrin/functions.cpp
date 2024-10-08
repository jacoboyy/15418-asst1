#include <math.h>
#include <stdio.h>

#include <algorithm>

#include "CMU418intrin.h"
#include "logger.h"
using namespace std;

void absSerial(float* values, float* output, int N) {
  for (int i = 0; i < N; i++) {
    float x = values[i];
    if (x < 0) {
      output[i] = -x;
    } else {
      output[i] = x;
    }
  }
}

// implementation of absolute value using 15418 instrinsics
void absVector(float* values, float* output, int N) {
  __cmu418_vec_float x;
  __cmu418_vec_float result;
  __cmu418_vec_float zero = _cmu418_vset_float(0.f);
  __cmu418_mask maskAll, maskIsNegative, maskIsNotNegative;

  //  Note: Take a careful look at this loop indexing.  This example
  //  code is not guaranteed to work when (N % VECTOR_WIDTH) != 0.
  //  Why is that the case?
  for (int i = 0; i < N; i += VECTOR_WIDTH) {
    // All ones
    maskAll = _cmu418_init_ones();

    // All zeros
    maskIsNegative = _cmu418_init_ones(0);

    // Load vector of values from contiguous memory addresses
    _cmu418_vload_float(x, values + i, maskAll);  // x = values[i];

    // Set mask according to predicate
    _cmu418_vlt_float(maskIsNegative, x, zero, maskAll);  // if (x < 0) {

    // Execute instruction using mask ("if" clause)
    _cmu418_vsub_float(result, zero, x, maskIsNegative);  //   output[i] = -x;

    // Inverse maskIsNegative to generate "else" mask
    maskIsNotNegative = _cmu418_mask_not(maskIsNegative);  // } else {

    // Execute instruction ("else" clause)
    _cmu418_vload_float(result, values + i,
                        maskIsNotNegative);  //   output[i] = x; }

    // Write results back to memory
    _cmu418_vstore_float(output + i, result, maskAll);
  }
}

// Accepts an array of values and an array of exponents
// For each element, compute values[i]^exponents[i] and clamp value to
// 4.18.  Store result in outputs.
// Uses iterative squaring, so that total iterations is proportional
// to the log_2 of the exponent
void clampedExpSerial(float* values, int* exponents, float* output, int N) {
  for (int i = 0; i < N; i++) {
    float x = values[i];
    float result = 1.f;
    int y = exponents[i];
    float xpower = x;
    while (y > 0) {
      if (y & 0x1) {
        result *= xpower;
      }
      xpower = xpower * xpower;
      y >>= 1;
    }
    if (result > 4.18f) {
      result = 4.18f;
    }
    output[i] = result;
  }
}

void clampedExpVector(float* values, int* exponents, float* output, int N) {
  __cmu418_vec_float result, x;
  __cmu418_vec_int exp, andOne;
  __cmu418_vec_int zero = _cmu418_vset_int(0);
  __cmu418_vec_int one = _cmu418_vset_int(1);
  __cmu418_vec_float bound = _cmu418_vset_float(4.18f);
  __cmu418_mask maskAll, maskLoop, maskClamp, maskAnd, maskNotAnd;
  for (int i = 0; i < N; i += VECTOR_WIDTH) {
    // handle case where N % VECTOR_WIDTH != 0
    int width = std::min(VECTOR_WIDTH, N - i);
    maskAll = _cmu418_init_ones(width);
    // load original values
    _cmu418_vload_float(x, values + i, maskAll);
    _cmu418_vload_int(exp, exponents + i, maskAll);
    result = _cmu418_vset_float(1.f);
    // loop control
    maskLoop = _cmu418_init_ones(0);
    _cmu418_vgt_int(maskLoop, exp, zero, maskAll);
    while (_cmu418_cntbits(maskLoop)) {  // while (y>0)
      maskNotAnd = _cmu418_init_ones(0);
      andOne = _cmu418_vset_int(0);
      _cmu418_vbitand_int(andOne, exp, one, maskAll);
      _cmu418_veq_int(maskNotAnd, andOne, zero, maskAll);
      maskAnd = _cmu418_mask_not(maskNotAnd);
      maskAnd = _cmu418_mask_and(maskAnd, maskAll);     // if (y & 0x1)
      _cmu418_vmult_float(result, result, x, maskAnd);  // result *= xpower
      _cmu418_vmult_float(x, x, x, maskAll);            // xpower *= xpower
      _cmu418_vshiftright_int(exp, exp, one, maskAll);  // y >> 1
      _cmu418_vgt_int(maskLoop, exp, zero, maskAll);
    }
    // clamp results
    _cmu418_vgt_float(maskClamp, result, bound,
                      maskAll);                    // if (result > 4.18f)
    _cmu418_vset_float(result, 4.18f, maskClamp);  // result = 4.18f
    // write results back to memory
    _cmu418_vstore_float(output + i, result, maskAll);  // output[i] = result;
  }
}

float arraySumSerial(float* values, int N) {
  float sum = 0;
  for (int i = 0; i < N; i++) {
    sum += values[i];
  }

  return sum;
}

// Assume N % VECTOR_WIDTH == 0
// Assume VECTOR_WIDTH is a power of 2
float arraySumVector(float* values, int N) {
  __cmu418_vec_float temp;
  __cmu418_vec_float result = _cmu418_vset_float(0.f);
  __cmu418_mask maskAll = _cmu418_init_ones();
  // vector sum
  for (int i = 0; i < N; i += VECTOR_WIDTH) {
    _cmu418_vload_float(temp, values + i, maskAll);
    _cmu418_vadd_float(result, result, temp, maskAll);
  }
  // sum of vector sum
  int width = VECTOR_WIDTH / 2;
  while (width) {
    _cmu418_hadd_float(result, result);
    _cmu418_interleave_float(result, result);
    width /= 2;
  }
  // store and return
  float output[VECTOR_WIDTH];
  _cmu418_vstore_float(output, result, maskAll);
  return output[0];
}
