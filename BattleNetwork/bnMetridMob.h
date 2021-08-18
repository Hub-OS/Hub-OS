#pragma once
#include "bnMobFactory.h"
#include "bnMetrid.h"

/**
 * @class MetridMob
 * @author mav
 * @date 05/05/19
 * @brief Spawns a metrid mob mixed with canodumbs
 */
class MetridMob :
  public MobFactory
{
public:
  /**
   * @brief Build the mob
   * @return Mob*
   */
  Mob* Build(Field* field);
};
