#pragma once
#include "bnMobFactory.h"

<<<<<<< HEAD
=======
/*! \brief Spawns metal man, toggles boss flag, uses custom music, and a custom background */
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
class MetalManBossFight :
  public MobFactory
{
public:
  MetalManBossFight(Field* field);
  ~MetalManBossFight();

  Mob* Build();
};

