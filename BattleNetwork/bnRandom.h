#pragma once
#include <cstdint>
#include <random>
#include <SFML/System/Vector2.hpp>

template<typename T>
static T rand_val(const T& min, const T& max) {
  int sample = rand() % RAND_MAX;
  double frac = static_cast<double>(sample) / static_cast<double>(RAND_MAX);

  return static_cast<T>(((1.0 - frac) * min) + (frac * max));
}

static int rand_val(const int& min, const int& max) {
  return (rand() % (max - min + 1)) + (min);
}

static bool rand_val() {
  return rand() % RAND_MAX == 0;
}

static sf::Vector2f rand_val(const sf::Vector2f& min, const sf::Vector2f& max) {
  int sample = rand() % RAND_MAX;
  double frac = static_cast<double>(sample) / static_cast<double>(RAND_MAX);

  sf::Vector2f result{};
  result.x = (((1.0 - frac) * min.x) + (frac * max.x));

  // resample for y
  sample = rand() % RAND_MAX;
  frac = static_cast<double>(sample) / static_cast<double>(RAND_MAX);

  result.y = (((1.0 - frac) * min.y) + (frac * max.y));

  return result;
}

// for random values that need to be synced, use these in lockstep only where necessary

uint32_t SyncedRandBelow(uint32_t n);
float SyncedRandFloat();
void SeedSyncedRand(uint32_t seed);
