/*! \brief Spawns two mettaurs on the field
 * 
 * May optionally spawn an empty tile for challenge
 */

#pragma once
#include "bnMobFactory.h"
#include "bnMettaur.h"
#include "bnMettaurIdleState.h"

class TwoMettaurMob : public MobFactory
{
public:

  /**
   * @brief Builds and returns the mob
   * @return Mob pointer. must be deleted manually.
   */
  Mob* Build(Field* field);
};

