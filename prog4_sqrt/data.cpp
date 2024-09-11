#include <algorithm>

// Generate random data
void initRandom(float *values, int N) {
  for (int i = 0; i < N; i++) {
    // random input values
    values[i] = .001f + 2.998f * static_cast<float>(rand()) / RAND_MAX;
  }
}

// Generate data that gives high relative speedup
void initGood(float *values, int N) {
  for (int i = 0; i < N; i++) {
    values[i] = 2.9999f;
  }
}

// Generate data that gives low relative speedup
void initBad(float *values, int N) {
  for (int i = 0; i < N; i++) {
    if (i % 8 == 0) {
      values[i] = 2.9999f;
    } else {
      values[i] = 1.0f;
    }
  }
}
