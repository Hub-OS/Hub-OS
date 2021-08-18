#pragma once
#include "bnMobFactory.h"
#include "bnCanodumb.h"
#include "bnCanodumbIdleState.h"

/**
 * @class CanodumbMob
 * @author mav
 * @date 05/05/19
 * @brief Spawns 3 canodumbs with Rank 1, 2, and 3 respectively
 */
class CanodumbMob :
  public MobFactory
{
  /**
   * @brief Build the mob 
   * @return Mob*
   */
  Mob* Build(Field* field);
};
