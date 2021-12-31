#pragma once
#include <cstdint>

// for random values that need to be synced, use these in lockstep only where necessary

uint32_t SyncedRand();
uint32_t SyncedRandMax();
void SeedSyncedRand(uint32_t seed);
