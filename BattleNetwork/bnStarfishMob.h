/*! \brief Spawns one or two Starfish enemies on the field
 * 
 */
#pragma once
#include "bnMobFactory.h"
#include "bnStarfish.h"
#include "bnStarfishIdleState.h"

class StarfishMob :
  public MobFactory
{
public:
  /**
   * @brief Builds and returns the mob
   * @return Mob pointer. must be deleted manually.
   */
  Mob* Build(Field* field);
};