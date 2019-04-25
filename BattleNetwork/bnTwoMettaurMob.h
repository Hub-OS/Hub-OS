/*! \brief Spawns two mettaurs on the field
 *  \class TwoMettaurMob
 * 
 * May optionally spawn an empty tile for challenge
 */

#pragma once
#include "bnMobFactory.h"
#include "bnMettaur.h"
#include "bnMettaurIdleState.h"

class TwoMettaurMob :
  public MobFactory
{
public:
  TwoMettaurMob(Field* field);
  ~TwoMettaurMob();

  /**
   * @brief Builds and returns the mob
   * @return Mob pointer. must be deleted manually.
   */
  Mob* Build();
};

