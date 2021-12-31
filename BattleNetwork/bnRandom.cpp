#include "bnRandom.h"
#include <random>

// same as std::mt19937, but using uint32_t instead of uint32_fast_t
static std::mersenne_twister_engine<
  uint32_t, 32, 624, 397, 31,
  0x9908b0df, 11,
  0xffffffff, 7,
  0x9d2c5680, 15,
  0xefc60000, 18, 1812433253
> randomGenerator;

uint32_t SyncedRand() {
  return randomGenerator();
}

uint32_t SyncedRandMax() {
  return randomGenerator.max();
}

void SeedSyncedRand(uint32_t seed) {
  randomGenerator.seed(seed);
}
