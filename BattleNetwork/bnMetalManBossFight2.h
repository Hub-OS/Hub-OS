#pragma once
#include "bnMobFactory.h"

<<<<<<< HEAD
=======
/*! \brief same as MetalManBossFight but every tile is now ICE */
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
class MetalManBossFight2 :
  public MobFactory
{
public:
  MetalManBossFight2(Field* field);
  ~MetalManBossFight2();

  Mob* Build();
};
