#pragma once
#include <cstdint>

// for random values that need to be synced, use these in lockstep only where necessary

uint32_t SyncedRandBelow(uint32_t n);
float SyncedRandFloat();
void SeedSyncedRand(uint32_t seed);
