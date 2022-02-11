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

uint32_t SyncedRandBelow(uint32_t n) {
  std::uniform_int_distribution<> dist(0, n - 1);
  return dist(randomGenerator);
}

float SyncedRandFloat() {
  // https://stackoverflow.com/a/25669510
  double rd = std::generate_canonical<double, std::numeric_limits<float>::digits>(randomGenerator);
  float rf = rd;
  if (rf > rd) {
    rf = std::nextafter(rf, -std::numeric_limits<float>::infinity());
  }
  return rf;
}

void SeedSyncedRand(uint32_t seed) {
  randomGenerator.seed(seed);
}
