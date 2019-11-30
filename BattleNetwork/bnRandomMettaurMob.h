/*! \brief Spawns random enemies on random tiles. Anything goes.
 */
#pragma once
#include "bnMobFactory.h"
#include "bnMettaur.h"
#include "bnMettaurIdleState.h"

class RandomMettaurMob :
  public MobFactory
{
public:
  RandomMettaurMob(Field* field);
  ~RandomMettaurMob();

  /**
   * @brief Builds and returns the generated mob
   * @return Mob*
   */
  Mob* Build();
};

