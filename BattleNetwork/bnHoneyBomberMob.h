#pragma once
/*! \brief Spawns two or three honey bomber enemies on the field
 *
 */
#pragma once
#include "bnMobFactory.h"
#include "bnHoneyBomber.h"
#include "bnHoneyBomberIdleState.h"

class HoneyBomberMob :
  public MobFactory
{
public:
  /**
   * @brief Builds and returns the mob
   * @return Mob pointer. must be deleted manually.
   */
  Mob* Build(Field* field);
};