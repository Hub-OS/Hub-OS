<<<<<<< HEAD
=======
/*! \brief Spawns one or two Starfish enemies on the field
 * 
 */

>>>>>>> b486e21e11627262088deae73097eaa7af56791c
#pragma once
#include "bnMobFactory.h"
#include "bnStarfish.h"
#include "bnStarfishIdleState.h"

class StarfishMob :
  public MobFactory
{
public:
  StarfishMob(Field* field);
  ~StarfishMob();

<<<<<<< HEAD
=======
  /**
   * @brief Builds and returns the mob
   * @return Mob pointer. must be deleted manually.
   */
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
  Mob* Build();
};